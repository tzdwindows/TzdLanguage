// ─── TzdLang Language Server ─────────────────────────────────────────────
// 修复：局部变量带类型声明（var int a）和构造函数参数（Thread(target)）误报未声明的问题
// 新增：动态加载 C++ 解释器系统自带的 Native 类和函数

import antlr4 from "antlr4";
import TzdLangLexer from "../TzdLangLexer.mjs";
import TzdLangParser from "../TzdLangParser.mjs";
import { detectRecursions } from "./recursionDetector.mjs";

import path from "path";
import fs from "fs";
import { fileURLToPath } from "url";
import os from "os";
import { execFile } from "child_process";

import {
  createConnection,
  TextDocuments,
  Diagnostic,
  DiagnosticSeverity,
  CompletionItemKind,
  InsertTextFormat,
  Range,
  MarkupKind,
  TextDocumentSyncKind,
} from "vscode-languageserver/node";

import { TextDocument } from "vscode-languageserver-textdocument";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const connection = createConnection();
const documents = new TextDocuments(TextDocument);

const stdlibRoots = [
  path.resolve(__dirname, "..", "..", "stdlib"),
  path.resolve(__dirname, "..", "..", "..", "stdlib"),
  path.resolve(__dirname, "..", "..", "..", "x64", "Release", "stdlib"),
];

function uriToFsPath(uri) {
  if (!uri) return "";
  if (uri.startsWith("file:///"))
    return decodeURIComponent(uri.slice(8).replace(/\//g, path.sep));
  return uri;
}

function fsPathToUri(fsPath) {
  return "file:///" + fsPath.replace(/\\/g, "/");
}

function safeText(node) {
  try {
    if (!node) return "";
    if (typeof node.getText === "function") return node.getText();
    if (node.symbol && typeof node.symbol.text === "string")
      return node.symbol.text;
    return String(node.text ?? "");
  } catch {
    return "";
  }
}

function fileExistsSafe(filePath) {
  try {
    return fs.existsSync(filePath);
  } catch {
    return false;
  }
}

// ──────────────────────────────────────────────────────────────────────────
// 全局缓存与白名单
// ──────────────────────────────────────────────────────────────────────────
const classCache = new Map();
const globalFunctionsCache = new Set();

let defaultBuiltins = { classes: ["Runtime"], functions: [] };
try {
  defaultBuiltins = require("./builtins.json");
} catch (_) {}

const cachedRuntimeSymbols = {
  classes: Array.isArray(defaultBuiltins.classes) ? [...defaultBuiltins.classes] : ["Runtime"],
  functions: Array.isArray(defaultBuiltins.functions) ? [...defaultBuiltins.functions] : [],
};

const KEYWORDS = new Set([
  "var",
  "let",
  "const",
  "class",
  "fun",
  "return",
  "ret",
  "import",
  "if",
  "else",
  "while",
  "for",
  "break",
  "continue",
  "switch",
  "case",
  "default",
  "try",
  "catch",
  "throw",
  "new",
  "this",
  "super",
  "true",
  "false",
  "null",
  "print",
  "printf",
  "out",
  "int",
  "float",
  "string",
  "bool",
  "void",
  "ptr",
  "pointer",
  "function",
  "in",
  "native",
  "abstract",
  "static",
  "public",
  "private",
  "protected",
  "Runtime",
  "extends",
]);

// ──────────────────────────────────────────────────────────────────────────
// C++ 动态反射提取
// ──────────────────────────────────────────────────────────────────────────

function fetchRuntimeSymbolsFromCpp(toolsPath) {
  if (!toolsPath || !fileExistsSafe(toolsPath)) return;

  const tempDir = os.tmpdir();
  const tempFilePath = path.join(tempDir, "tzd_get_symbols_temp.tzd");
  const scriptContent = "print(getSymbols());\n";

  try {
    fs.writeFileSync(tempFilePath, scriptContent, "utf-8");
  } catch (e) {
    connection.console.error("[TzdLang] 无法创建临时反射脚本文件:" + e);
    return;
  }

  const args = [`--runMainTzd=${tempFilePath}`];

  execFile(toolsPath, args, (error, stdout, stderr) => {
    fs.unlink(tempFilePath, () => {});

    if (error) {
      connection.console.error("[TzdLang] 动态同步 C++ 符号失败: " + error.message);
      return;
    }

    if (!stdout || !stdout.trim()) return;

    try {
      const cleanOutput = stdout.trim();
      const classesMatch = cleanOutput.match(/"classes"\s*:\s*\[([^\]]*)\]/);
      const functionsMatch = cleanOutput.match(
        /"functions"\s*:\s*\[([^\]]*)\]/,
      );

      if (classesMatch && classesMatch[1]) {
        const parsedClasses = classesMatch[1]
          .replace(/"/g, "")
          .split(",")
          .map((item) => item.trim())
          .filter((item) => item.length > 0);
        for (const c of parsedClasses) {
          if (!cachedRuntimeSymbols.classes.includes(c)) {
            cachedRuntimeSymbols.classes.push(c);
          }
        }
      }
      if (functionsMatch && functionsMatch[1]) {
        const parsedFunctions = functionsMatch[1]
          .replace(/"/g, "")
          .split(",")
          .map((item) => item.trim())
          .filter((item) => item.length > 0);
        for (const f of parsedFunctions) {
          if (!cachedRuntimeSymbols.functions.includes(f)) {
            cachedRuntimeSymbols.functions.push(f);
          }
        }
      }

      connection.console.log(
        `[TzdLang] 成功同步 C++ 符号！加载系统函数: ${cachedRuntimeSymbols.functions.length} 个, 系统类: ${cachedRuntimeSymbols.classes.length} 个`,
      );

      // 同步完成后重新校验所有已打开的文档，立即清除误报的未声明错误
      for (const doc of documents.all()) {
        validateDocument(doc);
      }
    } catch (e) {
      connection.console.error("[TzdLang] 解析 C++ 符号失败: " + e);
    }
  });
}

// ──────────────────────────────────────────────────────────────────────────
// 解析器与 AST 提取
// ──────────────────────────────────────────────────────────────────────────

function parseTzd(text) {
  const chars = new antlr4.InputStream(text);
  const lexer = new TzdLangLexer(chars);
  const tokens = new antlr4.CommonTokenStream(lexer);
  const parser = new TzdLangParser(tokens);
  parser.buildParseTrees = true;

  const errListener = new (class extends antlr4.error.ErrorListener {
    constructor() {
      super();
      this.errors = [];
    }
    syntaxError(recognizer, sym, line, col, msg) {
      this.errors.push({
        line: Math.max(line - 1, 0),
        column: Math.max(col, 0),
        message: msg,
        sym,
      });
    }
  })();
  parser.removeErrorListeners();
  parser.addErrorListener(errListener);
  let tree = null;
  try {
    tree = parser.program();
  } catch (e) {}
  const diagnostics = errListener.errors.map((e) => {
    const line = e.line;
    const col = e.column;
    const len =
      e.sym && e.sym.stop >= e.sym.start ? e.sym.stop - e.sym.start + 1 : 1;
    return Diagnostic.create(
      Range.create(line, col, line, col + Math.max(len, 1)),
      e.message || "Syntax error",
      DiagnosticSeverity.Error,
      "tzdlang",
    );
  });
  return { tree, diagnostics };
}

function collectEveryNode(tree, cb) {
  if (!tree) return;
  const stack = [tree];
  while (stack.length) {
    const node = stack.pop();
    if (!node || node.ruleIndex === undefined) continue;
    cb(node);
    if (node.children && node.children.length) {
      for (let i = node.children.length - 1; i >= 0; i--) {
        const c = node.children[i];
        if (c && typeof c === "object") stack.push(c);
      }
    }
  }
}

function extractClassDefs(tree, text, uri) {
  const defs = [];
  if (!tree) return defs;
  const lines = text.split("\n");

  collectEveryNode(tree, (node) => {
    if (node.ruleIndex === 8) {
      // ClassDecl
      try {
        const qn =
          typeof node.qualifiedName === "function"
            ? node.qualifiedName(0)
            : null;
        const className = qn ? safeText(qn) : "";
        if (!className) return;

        let parentName = null;
        if (
          typeof node.qualifiedName === "function" &&
          node.qualifiedName().length > 1
        ) {
          parentName = safeText(node.qualifiedName(1)) || null;
        } else {
          const children = node.children || [];
          for (let i = 0; i < children.length; i++) {
            const txt = safeText(children[i]);
            if (txt === "extends" || txt === ":") {
              if (i + 1 < children.length)
                parentName = safeText(children[i + 1]) || null;
            }
          }
        }

        const startLine = node.start ? Math.max(node.start.line - 1, 0) : 0;
        const stopLine = node.stop
          ? Math.max(node.stop.line - 1, 0)
          : lines.length - 1;
        const members = [];

        // ⭐ 优先从 AST 语法树提取成员（精准捕获静态方法、普通方法、构造函数、属性）
        const body =
          typeof node.classBody === "function" ? node.classBody() : null;
        if (body && typeof body.classMember === "function") {
          const cms = body.classMember();
          for (const cm of cms) {
            const md =
              typeof cm.memberDecl === "function" ? cm.memberDecl() : null;
            if (!md) continue;

            if (
              md instanceof TzdLangParser.MethodStaticDeclContext ||
              md instanceof TzdLangParser.MethodDeclContext ||
              md instanceof TzdLangParser.MethodAbstractDeclContext
            ) {
              const idNode =
                typeof md.IDENTIFIER === "function" ? md.IDENTIFIER() : null;
              if (!idNode) continue;
              const singleId = Array.isArray(idNode) ? idNode[0] : idNode;
              if (!singleId) continue;
              const name = singleId.getText();
              const sym =
                singleId.symbol ||
                (typeof singleId.getSymbol === "function"
                  ? singleId.getSymbol()
                  : null);
              const mLine = sym ? sym.line - 1 : 0;
              const mCol = sym ? sym.column : 0;

              const pl =
                typeof md.paramList === "function" ? md.paramList() : null;
              const signature = pl ? pl.getText() : "";
              const argsCount =
                pl && typeof pl.param === "function" ? pl.param().length : 0;
              const isStatic =
                md instanceof TzdLangParser.MethodStaticDeclContext;

              let bodyText = "";
              const b = typeof md.block === "function" ? md.block() : null;
              if (b && b.start && b.stop) {
                bodyText = text.slice(b.start.start, b.stop.stop + 1);
              }

              members.push({
                name,
                kind: "method",
                isStatic,
                signature,
                argsCount,
                bodyText,
                location: {
                  uri,
                  range: Range.create(mLine, mCol, mLine, mCol + name.length),
                },
              });
            } else if (md instanceof TzdLangParser.ConstructorDeclContext) {
              const idNode =
                typeof md.IDENTIFIER === "function" ? md.IDENTIFIER() : null;
              if (!idNode) continue;
              const singleId = Array.isArray(idNode) ? idNode[0] : idNode;
              if (!singleId) continue;
              const name = singleId.getText();
              const sym =
                singleId.symbol ||
                (typeof singleId.getSymbol === "function"
                  ? singleId.getSymbol()
                  : null);
              const mLine = sym ? sym.line - 1 : 0;
              const mCol = sym ? sym.column : 0;

              const pl =
                typeof md.paramList === "function" ? md.paramList() : null;
              const signature = pl ? pl.getText() : "";
              const argsCount =
                pl && typeof pl.param === "function" ? pl.param().length : 0;

              members.push({
                name,
                kind: "constructor",
                signature,
                argsCount,
                location: {
                  uri,
                  range: Range.create(mLine, mCol, mLine, mCol + name.length),
                },
              });
            } else if (
              md instanceof TzdLangParser.FieldVarDeclContext ||
              md instanceof TzdLangParser.FieldLetDeclContext ||
              md instanceof TzdLangParser.FieldConstDeclContext
            ) {
              const idNode =
                typeof md.IDENTIFIER === "function" ? md.IDENTIFIER() : null;
              if (idNode) {
                const singleId = Array.isArray(idNode) ? idNode[0] : idNode;
                if (singleId) {
                  const name = singleId.getText();
                  const sym =
                    singleId.symbol ||
                    (typeof singleId.getSymbol === "function"
                      ? singleId.getSymbol()
                      : null);
                  const mLine = sym ? sym.line - 1 : 0;
                  const mCol = sym ? sym.column : 0;
                  const tt =
                    typeof md.typeType === "function" ? md.typeType() : null;
                  let type = tt ? tt.getText() : null;
                  const expr =
                    typeof md.expression === "function"
                      ? md.expression()
                      : null;
                  if (!type && expr && expr.start && expr.stop) {
                    const initStr = text
                      .slice(expr.start.start, expr.stop.stop + 1)
                      .trim();
                    const newM = initStr.match(/^new\s+([A-Z][a-zA-Z0-9_]*)/);
                    if (newM) type = newM[1];
                    else if (/^-?\d+\.\d+$/.test(initStr)) type = "float";
                    else if (/^-?\d+$/.test(initStr)) type = "int";
                    else if (/^"(?:[^"\\]|\\.)*"$/.test(initStr)) type = "string";
                    else if (initStr === "true" || initStr === "false")
                      type = "bool";
                  }
                  members.push({
                    name,
                    kind: "field",
                    type,
                    location: {
                      uri,
                      range: Range.create(
                        mLine,
                        mCol,
                        mLine,
                        mCol + name.length,
                      ),
                    },
                  });
                }
              }
            }
          }
        }

        // 回退机制：若 AST 无法提取到成员，使用支持所有修饰符的行正则提取
        if (members.length === 0) {
          let depth = 0;
          for (let i = startLine; i <= stopLine; i++) {
            const lineStr = lines[i];
            const cleanLine = lineStr
              .replace(/"(?:[^"\\]|\\.)*"/g, '""')
              .replace(/\/\/.*/, "");
            const lineStartDepth = depth;
            for (let char of cleanLine) {
              if (char === "{") depth++;
              else if (char === "}") depth--;
            }
            if (lineStartDepth === 1) {
              let m = lineStr.match(
                /^\s*(?:(?:public|private|protected|static)\s+)*(?:var|let|const)\s+(?:([a-zA-Z_]\w*)\s+)?([a-zA-Z_]\w*)(?:\s*=\s*(.*?))?(?:;|$)/,
              );
              if (m) {
                const name = m[2];
                let type = m[1] || null;
                const initExpr = m[3] ? m[3].trim() : "";
                if (!type && initExpr) {
                  const newM = initExpr.match(/^new\s+([A-Z][a-zA-Z0-9_]*)/);
                  if (newM) type = newM[1];
                  else if (/^-?\d+\.\d+$/.test(initExpr)) type = "float";
                  else if (/^-?\d+$/.test(initExpr)) type = "int";
                  else if (/^"(?:[^"\\]|\\.)*"$/.test(initExpr))
                    type = "string";
                  else if (initExpr === "true" || initExpr === "false")
                    type = "bool";
                }
                const col = lineStr.indexOf(name);
                members.push({
                  name,
                  kind: "field",
                  type,
                  location: {
                    uri,
                    range: Range.create(i, col, i, col + name.length),
                  },
                });
                continue;
              }
              m = lineStr.match(
                /^\s*(?:(?:public|private|protected|static|abstract)\s+)*fun\s+([a-zA-Z_]\w*)\s*\((.*?)\)/,
              );
              if (m) {
                const col = lineStr.indexOf(m[1]);
                const args = m[2]
                  .split(",")
                  .map((x) => x.trim())
                  .filter((x) => x.length > 0);

                let bodyLines = [];
                let bDepth = 0;
                let started = false;
                for (let j = i; j <= stopLine; j++) {
                  const l = lines[j];
                  bodyLines.push(l);
                  for (const c of l
                    .replace(/"(?:[^"\\]|\\.)*"/g, '""')
                    .replace(/\/\/.*/, "")) {
                    if (c === "{") {
                      bDepth++;
                      started = true;
                    } else if (c === "}") {
                      bDepth--;
                    }
                  }
                  if (started && bDepth === 0) break;
                }

                members.push({
                  name: m[1],
                  kind: "method",
                  isStatic:
                    /^\s*static\b/.test(lineStr) ||
                    /\bstatic\s+fun\b/.test(lineStr),
                  signature: m[2],
                  argsCount: args.length,
                  bodyText: bodyLines.join("\n"),
                  location: {
                    uri,
                    range: Range.create(i, col, i, col + m[1].length),
                  },
                });
                continue;
              }
              m = lineStr.match(
                new RegExp(
                  `^\\s*(?:(?:public|private|protected)\\s+)*${className}\\s*\\((.*?)\\)`,
                ),
              );
              if (m) {
                const col = lineStr.indexOf(className);
                const args = m[1]
                  .split(",")
                  .map((x) => x.trim())
                  .filter((x) => x.length > 0);
                members.push({
                  name: className,
                  kind: "constructor",
                  signature: m[1],
                  argsCount: args.length,
                  location: {
                    uri,
                    range: Range.create(i, col, i, col + className.length),
                  },
                });
              }
            }
          }
        }

        defs.push({
          className,
          parentName,
          file: uri,
          members,
          range: {
            start: { line: startLine, character: 0 },
            end: { line: stopLine, character: 0 },
          },
        });
      } catch {}
    }
  });
  return defs;
}

function loadImportClasses(docText, docUri) {
  const lines = docText.split("\n");
  for (const line of lines) {
    const m = line.match(/import\s+"(.+?)"\s*;/);
    if (!m) continue;
    const importPath = m[1];
    let resolved = null;

    const docFsPath = uriToFsPath(docUri);
    const docDir = path.dirname(docFsPath);
    const relPath = path.resolve(docDir, importPath.replace(/\//g, path.sep));

    if (fs.existsSync(relPath)) resolved = relPath;
    else {
      for (const root of stdlibRoots) {
        const cand = path.resolve(root, importPath.replace(/\//g, path.sep));
        if (fs.existsSync(cand)) {
          resolved = cand;
          break;
        }
      }
    }
    if (!resolved || classCache.has(resolved)) continue;

    try {
      const fileContent = fs.readFileSync(resolved, "utf-8");
      const fLines = fileContent.split("\n");
      for (let fl of fLines) {
        const fm = fl.match(/^\s*fun\s+([a-zA-Z_]\w*)/);
        if (fm) globalFunctionsCache.add(fm[1]);
      }
      const { tree } = parseTzd(fileContent);
      const uri = fsPathToUri(resolved);
      const defs = extractClassDefs(tree, fileContent, uri);
      for (const def of defs) classCache.set(def.className, def);
      classCache.set(resolved, { _file: true });
    } catch {}
  }
}

function getMembersIncludingInherited(className, seen = new Set()) {
  if (!className || seen.has(className)) return [];
  seen.add(className);
  const def = classCache.get(className);
  if (!def) return [];

  const result = Array.isArray(def.members) ? [...def.members] : [];
  if (def.parentName) {
    const parentMembers = getMembersIncludingInherited(def.parentName, seen);
    const ownNames = new Set(result.map((m) => m.name));
    for (const pm of parentMembers) {
      if (!ownNames.has(pm.name)) result.push(pm);
    }
  }
  return result;
}

/**
 * 提取某行前面紧邻的多行注释块（/** ... * / 或 /* ... * /）作为文档字符串。
 * lineNum 是声明所在行（0-indexed），会向上扫描查找注释。
 */
function extractDocComment(text, lineNum) {
  if (lineNum == null || lineNum < 0) return null;
  const lines = text.split("\n");

  // 向上跳过空行
  let i = lineNum - 1;
  while (i >= 0 && /^\s*$/.test(lines[i])) i--;
  if (i < 0) return null;

  // 找到以 */ 结尾的行
  if (!/\*\/\s*$/.test(lines[i])) return null;
  const closeIdx = i;

  // 向上找 /*
  let openIdx = -1;
  for (let j = closeIdx; j >= 0; j--) {
    if (/\/\*/.test(lines[j])) {
      openIdx = j;
      break;
    }
  }
  if (openIdx < 0) return null;

  // 取出注释内容，清理前缀符号（* 等）
  const commentLines = lines.slice(openIdx, closeIdx + 1);
  const cleaned = commentLines
    .map((l) =>
      l
        .replace(/^\s*\/\*+\s?/, "")
        .replace(/\s*\*+\/\s*$/, "")
        .replace(/^\s*\*\s?/, "")
    )
    .filter((l) => l.trim() !== "" || commentLines.length > 2);

  const content = cleaned.join("\n").trim();
  return content || null;
}

function extractParamInfo(arg) {
  let s = arg.trim();
  if (!s) return null;
  if (s.includes("=")) {
    s = s.split("=")[0].trim();
  }
  let type = "any";
  let name = "";
  if (s.includes(":")) {
    const parts = s.split(":");
    name = parts[0].trim();
    type = parts[1].trim();
  } else {
    const parts = s.split(/\s+/);
    if (parts.length > 1) {
      type = parts[0];
      name = parts[parts.length - 1];
    } else {
      name = parts[0];
    }
  }
  name = name.replace(/^[^\w]+|[^\w]+$/g, "");
  return name ? { name, type } : null;
}

// ⭐ 彻底修复：增强局部变量、带类型变量、函数参数的提取
function extractVisibleLocals(text) {
  const lines = text.split("\n");
  const locals = new Map();
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];

    // 1. var / let / const [type] name (如 var int a;)
    let m = line.match(
      /\b(?:var|let|const)\s+(?:[a-zA-Z_]\w*\s+)?([a-zA-Z_]\w*)\b/,
    );
    if (m) locals.set(m[1], { kind: "variable", line: i });

    // 2. Type name = ... (如 AAA A = new AAA();)
    m = line.match(
      /\b([A-Z]\w*|int|float|string|bool|ptr|function)\s+([a-zA-Z_]\w*)\s*(?:=|;|$)/,
    );
    if (m) locals.set(m[2], { kind: "variable", type: m[1], line: i });

    // 3. 函数声明自身及参数 (如 fun aaa(x, y), static fun apply(op: function, val))
    m = line.match(/\bfun\s+([a-zA-Z_]\w*)\s*\((.*?)\)/);
    if (m) {
      locals.set(m[1], { kind: "function", line: i });
      m[2].split(",").forEach((arg) => {
        const info = extractParamInfo(arg);
        if (info) locals.set(info.name, { kind: "parameter", type: info.type, line: i });
      });
    }

    // 4. 全局隐式声明/赋值 (如 c = 10; 排除 A.c = 10)
    m = line.match(
      /(?:^|[{};]\s*)\b([a-zA-Z_]\w*)\s*(?:\+|-|\*|\/|%|&|\||\^)?=/,
    );
    if (m) locals.set(m[1], { kind: "variable", line: i });

    // 5. 构造函数参数 (如 Thread(target))
    m = line.match(/^\s*[A-Z]\w*\s*\((.*?)\)/);
    if (m) {
      m[1].split(",").forEach((arg) => {
        const info = extractParamInfo(arg);
        if (info) locals.set(info.name, { kind: "parameter", type: info.type, line: i });
      });
    }

    // 6. catch(e) 参数
    m = line.match(/\bcatch\s*\(\s*([a-zA-Z_]\w*)\s*\)/);
    if (m) locals.set(m[1], { kind: "variable", line: i });
  }
  return [...locals.entries()].map(([name, info]) => ({ name, ...info }));
}

function findClassAtLine(docUri, lineNum) {
  for (const def of classCache.values()) {
    if (
      def.file === docUri &&
      def.range &&
      def.range.start.line <= lineNum &&
      def.range.end.line >= lineNum
    ) {
      return def;
    }
  }
  return null;
}

function resolveVarType(varName, text, docUri, lineNum) {
  if (!varName) return null;
  if (varName === "this")
    return findClassAtLine(docUri, lineNum)?.className || null;
  if (varName === "super")
    return findClassAtLine(docUri, lineNum)?.parentName || null;

  // ⭐ 核心修复：如果 varName 本身就是已知类名（例如静态方法/属性访问 MathToolkit.square），直接解析为其自身类型
  if (classCache.has(varName)) return varName;
  if (new RegExp(`\\bclass\\s+${varName}\\b`).test(text)) return varName;

  const lines = text.split("\n");


  // 1. 显式类型声明 (例如: int a = ... / TestError erro = ...)
  const exactPattern = new RegExp(
    `^\\s*([A-Z][A-Za-z0-9_]*|int|float|string|bool|ptr|function)\\s+${varName}\\b`,
  );
  for (const line of lines) {
    let m = line.match(exactPattern);
    if (m) return m[1];
  }

  // 2. new 实例化推断 (例如: var err = new TestError / err = new TestError)
  const newPattern = new RegExp(
    `\\b(?:var|let|const|)\\s*${varName}\\s*=\\s*new\\s+([A-Z][A-Za-z0-9_]*)`,
  );
  for (const line of lines) {
    let m = line.match(newPattern);
    if (m) return m[1];
  }

  // 3. 字面量或表达式赋值推断 (例如: x = "100" / x = 12.3 / x = new Test().a())
  const assignRegex = new RegExp(
    `\\b(?:var|let|const|)\\s*${varName}\\s*=\\s*(.*);?`,
  );
  for (let i = 0; i < lines.length; i++) {
    let m = lines[i].match(assignRegex);
    if (m) {
      let val = m[1].trim().replace(/;$/, "").trim();
      if (val.startsWith('"') || val.startsWith("'")) return "string";
      if (val === "true" || val === "false") return "bool";
      if (/^-?\d+\.\d+$/.test(val)) return "float";
      if (/^-?\d+$/.test(val)) return "int";
      if (val.startsWith("new ")) {
        const nm = val.match(/^new\s+([A-Z][a-zA-Z0-9_]*)/);
        if (nm && !val.includes(".")) return nm[1];
      }
      if (val.includes(".") || val.endsWith(")")) {
        const t = evaluateChainType(val, docUri, i, text);
        if (t) return t;
      }
    }
  }

  return null;
}

function extractExpressionBeforeDot(text) {
  let i = text.length - 1;
  while (i >= 0 && /\s/.test(text[i])) i--;
  if (i < 0 || text[i] !== ".") return "";
  i--; // skip dot
  while (i >= 0 && /\s/.test(text[i])) i--;
  if (i < 0) return "";

  let end = i + 1;
  let parenDepth = 0;
  let bracketDepth = 0;
  let inString = false;
  let stringChar = "";

  while (i >= 0) {
    const ch = text[i];
    if (inString) {
      if (ch === stringChar && (i === 0 || text[i - 1] !== "\\")) {
        inString = false;
      }
      i--;
      continue;
    }
    if (ch === '"' || ch === "'") {
      inString = true;
      stringChar = ch;
      i--;
      continue;
    }
    if (ch === ")" || ch === "]") {
      if (ch === ")") parenDepth++;
      else bracketDepth++;
      i--;
      continue;
    }
    if (ch === "(" || ch === "[") {
      if (ch === "(") {
        if (parenDepth > 0) parenDepth--;
        else break;
      } else {
        if (bracketDepth > 0) bracketDepth--;
        else break;
      }
      i--;
      continue;
    }

    if (parenDepth > 0 || bracketDepth > 0) {
      i--;
      continue;
    }

    if (/[a-zA-Z0-9_]/.test(ch) || ch === "." || /\s/.test(ch)) {
      i--;
    } else {
      break;
    }
  }

  let expr = text.slice(i + 1, end).trim();
  expr = expr.replace(/^(?:return|ret|var|let|const|case|throw)\s+/, "").trim();
  return expr;
}

function splitChainSegments(expr) {
  const segments = [];
  let current = "";
  let parenDepth = 0;
  let bracketDepth = 0;
  let inString = false;
  let stringChar = "";

  for (let i = 0; i < expr.length; i++) {
    const ch = expr[i];
    if (inString) {
      current += ch;
      if (ch === stringChar && (i === 0 || expr[i - 1] !== "\\")) inString = false;
      continue;
    }
    if (ch === '"' || ch === "'") {
      inString = true;
      stringChar = ch;
      current += ch;
      continue;
    }
    if (ch === "(") parenDepth++;
    else if (ch === ")") parenDepth--;
    else if (ch === "[") bracketDepth++;
    else if (ch === "]") bracketDepth--;

    if (ch === "." && parenDepth === 0 && bracketDepth === 0) {
      if (current.trim()) segments.push(current.trim());
      current = "";
    } else {
      current += ch;
    }
  }
  if (current.trim()) segments.push(current.trim());
  return segments;
}

function inferMethodReturnType(member, className, docUri, text, seen = new Set()) {
  if (!member) return null;
  if (member.returnType) return member.returnType;
  if (!member.bodyText) return null;

  const key = `${className || ""}.${member.name}`;
  if (seen.has(key)) return null;
  seen.add(key);

  const body = member.bodyText;
  const retRegex = /\b(?:return|ret)\s+([^;}\n\r]+)/g;
  let match;
  const returnExprs = [];
  while ((match = retRegex.exec(body)) !== null) {
    const expr = match[1].trim();
    if (expr) returnExprs.push(expr);
  }

  if (returnExprs.length === 0) {
    member.returnType = "void";
    return "void";
  }

  for (const expr of returnExprs) {
    const newMatch = expr.match(/^new\s+([A-Z][a-zA-Z0-9_]*)/);
    if (newMatch && !expr.includes(").")) {
      member.returnType = newMatch[1];
      return newMatch[1];
    }
    if (expr === "this") {
      member.returnType = className;
      return className;
    }
    if (expr === "super") {
      const def = className ? classCache.get(className) : null;
      if (def && def.parentName) {
        member.returnType = def.parentName;
        return def.parentName;
      }
    }
    if (/^"(?:[^"\\]|\\.)*"$/.test(expr) || /^'(?:[^'\\]|\\.)*'$/.test(expr)) {
      member.returnType = "string";
      return "string";
    }
    if (/^-?\d+\.\d+$/.test(expr)) {
      member.returnType = "float";
      return "float";
    }
    if (/^-?\d+$/.test(expr)) {
      member.returnType = "int";
      return "int";
    }
    if (expr === "true" || expr === "false") {
      member.returnType = "bool";
      return "bool";
    }
    if (/^\[.*\]$/.test(expr)) {
      member.returnType = "array";
      return "array";
    }
    if (/^\{.*\}$/.test(expr)) {
      member.returnType = "map";
      return "map";
    }
    if (expr === "null") continue;

    // Check local variable inside method body
    const varNameMatch = expr.match(/^([a-zA-Z_]\w*)$/);
    if (varNameMatch) {
      const vName = varNameMatch[1];
      const nvMatch = body.match(
        new RegExp(
          `\\b(?:var|let|const|)\\s*${vName}\\s*=\\s*new\\s+([A-Z][a-zA-Z0-9_]*)`
        )
      );
      if (nvMatch) {
        member.returnType = nvMatch[1];
        return nvMatch[1];
      }
      const tvMatch = body.match(
        new RegExp(
          `\\b([A-Z][a-zA-Z0-9_]*|int|float|string|bool)\\s+${vName}\\b`
        )
      );
      if (tvMatch) {
        member.returnType = tvMatch[1];
        return tvMatch[1];
      }
      const assignM = body.match(
        new RegExp(`\\b(?:var|let|const|)\\s*${vName}\\s*=\\s*(.*);?`)
      );
      if (assignM) {
        const aVal = assignM[1].trim().replace(/;$/, "").trim();
        if (aVal.startsWith('"') || aVal.startsWith("'")) {
          member.returnType = "string";
          return "string";
        }
        if (/^-?\d+$/.test(aVal)) {
          member.returnType = "int";
          return "int";
        }
        if (/^-?\d+\.\d+$/.test(aVal)) {
          member.returnType = "float";
          return "float";
        }
        if (aVal === "true" || aVal === "false") {
          member.returnType = "bool";
          return "bool";
        }
        if (aVal.includes(".") || aVal.endsWith(")")) {
          const subT = evaluateChainType(
            aVal,
            docUri,
            member.location?.range?.start?.line || 0,
            text,
            seen
          );
          if (subT) {
            member.returnType = subT;
            return subT;
          }
        }
      }
    }

    // Chained call in return
    if (expr.includes(".") || expr.endsWith(")")) {
      const subT = evaluateChainType(
        expr,
        docUri,
        member.location?.range?.start?.line || 0,
        text,
        seen
      );
      if (subT) {
        member.returnType = subT;
        return subT;
      }
    }
  }

  return null;
}

function evaluateChainType(exprStr, docUri, line, text, seen = new Set()) {
  if (!exprStr) return null;
  const segments = splitChainSegments(exprStr);
  if (segments.length === 0) return null;

  let currentType = null;
  const root = segments[0];

  // 1. new ClassName(...)
  const newMatch = root.match(/^new\s+([A-Z][a-zA-Z0-9_]*)/);
  if (newMatch) {
    currentType = newMatch[1];
  } else if (root === "this") {
    currentType = findClassAtLine(docUri, line)?.className || null;
  } else if (root === "super") {
    currentType = findClassAtLine(docUri, line)?.parentName || null;
  } else if (classCache.has(root)) {
    currentType = root;
  } else {
    // Check if root is a function call like foo()
    const callMatch = root.match(/^([a-zA-Z_]\w*)\s*\(/);
    if (callMatch) {
      const fnName = callMatch[1];
      const curClass = findClassAtLine(docUri, line);
      if (curClass) {
        const curMembers = getMembersIncludingInherited(curClass.className);
        const m = curMembers.find((x) => x.name === fnName);
        if (m && m.kind === "method") {
          currentType = inferMethodReturnType(
            m,
            curClass.className,
            docUri,
            text,
            seen
          );
        }
      }
      if (!currentType && text) {
        const fnRegex = new RegExp(
          `(?:^|\\n)\\s*(?:(?:public|private|protected|static)\\s+)*fun\\s+${fnName}\\s*\\([^)]*\\)\\s*(\\{[\\s\\S]*?\\})`
        );
        const fm = text.match(fnRegex);
        if (fm) {
          const fakeMember = { name: fnName, bodyText: fm[1] };
          currentType = inferMethodReturnType(
            fakeMember,
            null,
            docUri,
            text,
            seen
          );
        }
      }
    } else {
      currentType = resolveVarType(root, text, docUri, line);
    }
  }

  if (!currentType) return null;

  for (let i = 1; i < segments.length; i++) {
    if (!currentType) return null;
    if (
      [
        "int",
        "float",
        "string",
        "bool",
        "void",
        "null",
        "array",
        "map",
      ].includes(currentType)
    ) {
      return null;
    }

    const allMembers = getMembersIncludingInherited(currentType);
    if (!allMembers.length) return null;

    const seg = segments[i];
    const callMatch = seg.match(/^([a-zA-Z_]\w*)\s*(?:\(|$)/);
    if (!callMatch) return null;
    const memberName = callMatch[1];
    const member = allMembers.find((m) => m.name === memberName);
    if (!member) return null;

    if (member.kind === "method") {
      currentType = inferMethodReturnType(
        member,
        currentType,
        docUri,
        text,
        seen
      );
    } else {
      currentType = member.type || null;
    }
  }

  return currentType;
}

function refreshLocalClassCache(text, docUri) {
  if (!text || !docUri) return;
  try {
    const { tree } = parseTzd(text);
    const localDefs = extractClassDefs(tree, text, docUri);
    for (const def of localDefs) classCache.set(def.className, def);
  } catch (_) {}
}

// ──────────────────────────────────────────────────────────────────────────
// 智能提示、签名、跳转、Hover
// ──────────────────────────────────────────────────────────────────────────

const KEYWORD_ITEMS = [
  ["fun", "fun ${1:name}($0)", "函数声明"],
  ["var", "var ${1:name} = $0;", "变量声明"],
  ["class", "class ${1:Name} {\n\t$0\n}", "类声明"],
  ["return", "return $0;", "返回值"],
  ["import", 'import "$0";', "导入文件"],
  ["this", "this", "当前实例"],
  ["super", "super($0)", "父类调用"],
];

function buildCompletions(text, line, col, docUri) {
  refreshLocalClassCache(text, docUri);
  const lines = text.split("\n");
  const lineText = lines[line] || "";
  const before = lineText.slice(0, col);

  // ─── 1. 提取当前前缀（光标左侧的标识符）───────────────────────────────────
  const prefixMatch = before.match(/([a-zA-Z_][\w]*)$/);
  const prefix = prefixMatch ? prefixMatch[1] : "";
  // before 去掉 prefix 之后的部分（即前缀开始前的文字）
  const beforePrefix = before.slice(0, before.length - prefix.length);

  // 帮助函数：按 prefix 过滤并评分排序
  // 精确匹配=0, 前缀匹配=1, 包含=2
  function filterItems(items) {
    if (!prefix) return items;
    const p = prefix.toLowerCase();
    return items
      .filter(it => it.label.toLowerCase().includes(p))
      .map(it => {
        const lbl = it.label.toLowerCase();
        const score = lbl === p ? "0" : lbl.startsWith(p) ? "1" : "2";
        return { ...it, sortText: score + (it.sortText || "9") };
      });
  }

  // ─── 2. 注释 / 字符串内 → 不补全 ──────────────────────────────────────────
  // 检查全文直到当前位置是否在块注释内
  const textUpToCursor = lines.slice(0, line).join("\n") + "\n" + before;
  {
    // 剥离字符串后检测 /* ... */
    const stripped = textUpToCursor.replace(/"(?:[^"\\]|\\.)*"/g, '""');
    const lastOpen = stripped.lastIndexOf("/*");
    const lastClose = stripped.lastIndexOf("*/");
    if (lastOpen > lastClose) return []; // 在块注释内
  }
  // 行内行注释 //
  if (/\/\//.test(before.replace(/"(?:[^"\\]|\\.)*"/g, '""'))) return [];
  // 行内字符串末尾未闭合（光标在字符串里）
  {
    let inStr = false, q = "";
    for (let i = 0; i < before.length; i++) {
      const ch = before[i];
      if (inStr) { if (ch === "\\" ) { i++; } else if (ch === q) inStr = false; }
      else if (ch === '"' || ch === "'") { inStr = true; q = ch; }
    }
    if (inStr) return [];
  }

  const items = [];

  // ─── 3. /** 块注释补全 ─────────────────────────────────────────────────────
  if (before.trimEnd().endsWith("/**")) {
    return [{ label: "/** */ (Doc block)", kind: 15 /*Snippet*/,
      insertTextFormat: 2, insertText: " * $0\n */", sortText: "0" }];
  }

  // ─── 4. 成员访问（点号）上下文 ───────────────────────────────────────────────
  // 判断：去掉 prefix 后以 . 结尾（如 "obj." 或 "new A().b." 或 "obj.m")
  {
    const dotCtx = /\.\s*$/.test(beforePrefix);
    // 或者前缀本身前面紧跟着点
    const dotBeforePrefix = /\.\s*$/.test(beforePrefix);
    if (dotCtx || /\.\s*$/.test(beforePrefix)) {
      const exprPart = extractExpressionBeforeDot(beforePrefix);
      if (exprPart) {
        const isSuper = exprPart === "super";
        const targetType = evaluateChainType(exprPart, docUri, line, text);
        if (targetType) {
          const allMembers = getMembersIncludingInherited(targetType);
          const seen = new Set();
          for (const member of allMembers) {
            if (member.kind === "constructor" || seen.has(member.name)) continue;
            if (isSuper && member.kind !== "method") continue;
            seen.add(member.name);
            if (member.kind === "method") {
              const retType = member.returnType ||
                inferMethodReturnType(member, targetType, docUri, text) || "";
              const retLabel = retType && !["void","null"].includes(retType) ? ` → ${retType}` : "";
              const docComment = extractDocComment(text, member.location?.range?.start?.line);
              items.push({
                label: member.name,
                kind: 2 /*Method*/,
                insertTextFormat: 2,
                insertText: `${member.name}($0)`,
                detail: `fun ${member.name}(${member.signature || ""})${retLabel}`,
                documentation: docComment ? { kind: "markdown", value: docComment } : undefined,
                sortText: "1",
              });
            } else {
              items.push({
                label: member.name,
                kind: 5 /*Field*/,
                detail: member.type ? `${member.type} field` : "field",
                sortText: "2",
              });
            }
          }
          return filterItems(items);
        }
      }
      // 点号上下文但目标类型未能推断 → 返回空，避免乱提示
      return [];
    }
  }

  // ─── 5. new 之后 → 只提示类名 ────────────────────────────────────────────
  if (/\bnew\s+$/.test(beforePrefix) || /\bnew\s+$/.test(before)) {
    const expectedTypeMatch = beforePrefix.match(/([A-Z][a-zA-Z0-9_]*)\s+[a-zA-Z_]\w*\s*=\s*new\s*$/);
    const expectedType = expectedTypeMatch ? expectedTypeMatch[1] : null;
    for (const [className, def] of classCache.entries()) {
      if (def._file || !className || className.startsWith("_") || className.startsWith("file:///")) continue;
      const sort = className === expectedType ? "0" : "1";
      items.push({
        label: className, kind: 4 /*Constructor*/,
        insertTextFormat: 2, insertText: `${className}($0)`,
        detail: def.parentName ? `extends ${def.parentName}` : "class",
        sortText: sort,
      });
    }
    for (const c of cachedRuntimeSymbols.classes) {
      items.push({ label: c, kind: 7 /*Class*/, insertTextFormat: 2, insertText: `${c}($0)`, detail: "Native C++ Class", sortText: "2" });
    }
    return filterItems(items);
  }

  // ─── 6. var/let/const 类型位置 → 只提示类型名 ──────────────────────────
  if (/\b(?:var|let|const)\s+$/.test(before) || /\b(?:var|let|const)\s+[a-zA-Z_]\w*$/.test(before)) {
    // 在 var/let 关键词后面紧跟一个词 → 提示类型（但本语言的写法是 var T name，类型可以是类名）
    // 先用 prefix 来判断是否在输入类型
    // 这里只在 "var " 后面有前缀才提示
    if (/\b(?:var|let|const)\s+$/.test(before)) {
      const typeItems = [];
      const PRIMITIVE_TYPES = ["int","string","bool","float","void"];
      for (const t of PRIMITIVE_TYPES) typeItems.push({ label: t, kind: 25 /*TypeParameter*/, detail: "primitive type", sortText: "0" });
      for (const [cn] of classCache.entries()) {
        if (!cn.startsWith("file:///") && !cn.startsWith("_")) {
          typeItems.push({ label: cn, kind: 7 /*Class*/, detail: "class type", sortText: "1" });
        }
      }
      return filterItems(typeItems);
    }
  }

  // ─── 7. 没有前缀（空格/纯触发）→ 不返回任何补全（避免乱显示）────────────
  // 只要用户没有在输入标识符词，直接返回空
  if (!prefix) return [];

  // ─── 8. 有前缀（用户在输入词）→ 智能组装候选集 ──────────────────────────
  //   a. 当前类成员（如果在类里面）
  const currentClass = findClassAtLine(docUri, line);
  if (currentClass) {
    // 类体内成员补全
    const allMembers = getMembersIncludingInherited(currentClass.className);
    const seenM = new Set();
    for (const m of allMembers) {
      if (m.kind === "constructor" || seenM.has(m.name)) continue;
      seenM.add(m.name);
      if (m.kind === "method") {
        items.push({ label: m.name, kind: 2 /*Method*/,
          insertTextFormat: 2, insertText: `${m.name}($0)`,
          detail: `fun ${m.name}(${m.signature || ""})`,
          sortText: "0" });
      } else {
        items.push({ label: m.name, kind: 5 /*Field*/,
          detail: m.type ? `${m.type} field` : "field", sortText: "1" });
      }
    }
    // 父类方法覆写建议（只在类体顶层，不在方法体内）
    if (currentClass.parentName) {
      const parentMembers = getMembersIncludingInherited(currentClass.parentName);
      const seenOverride = new Set(allMembers.map(m => m.name));
      for (const pm of parentMembers) {
        if (pm.kind === "method" && !seenOverride.has(pm.name)) {
          items.push({
            label: `↩ ${pm.name}`, kind: 15 /*Snippet*/,
            insertTextFormat: 2,
            insertText: `${pm.name}(${pm.signature || ""}) {\n\t$0\n}`,
            detail: `Override: fun ${pm.name}(${pm.signature || ""})`,
            sortText: "0",
          });
        }
      }
    }
  }

  //   b. 可见局部变量（作用域相关，优先级高）
  for (const v of extractVisibleLocals(text)) {
    if (!KEYWORDS.has(v.name)) {
      items.push({ label: v.name, kind: 6 /*Variable*/,
        detail: v.type ? `${v.type}` : v.kind, sortText: "1" });
    }
  }

  //   c. 全局函数
  for (const fn of globalFunctionsCache) {
    items.push({ label: fn, kind: 3 /*Function*/,
      insertTextFormat: 2, insertText: `${fn}($0)`,
      detail: "function", sortText: "2" });
  }

  //   d. 类名
  for (const [cn, def] of classCache.entries()) {
    if (!def._file && cn && !cn.startsWith("file:///") && !cn.startsWith("_")) {
      items.push({ label: cn, kind: 7 /*Class*/, detail: "class", sortText: "3" });
    }
  }

  //   e. 运行时内置函数
  for (const f of cachedRuntimeSymbols.functions) {
    items.push({ label: f, kind: 3 /*Function*/,
      insertTextFormat: 2, insertText: `${f}($0)`,
      detail: "Native C++ Function", sortText: "4" });
  }

  //   f. 运行时内置类
  for (const c of cachedRuntimeSymbols.classes) {
    items.push({ label: c, kind: 7 /*Class*/, detail: "Native C++ Class", sortText: "4" });
  }

  //   g. 关键字（最低优先）
  for (const [label, insert, doc] of KEYWORD_ITEMS) {
    items.push({ label, kind: 14 /*Keyword*/,
      insertText: insert, insertTextFormat: 2,
      detail: doc, sortText: "5" });
  }

  return filterItems(items);
}

connection.onSignatureHelp((params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return null;
  const text = doc.getText();
  const textLines = text.split("\n");
  const beforeCursor = textLines[params.position.line].substring(
    0,
    params.position.character,
  );

  function buildSigResult(members, className) {
    if (!members || members.length === 0) return null;
    return {
      signatures: members.map((m) => {
        const doc = extractDocComment(text, m.location?.range?.start?.line);
        return {
          label: `fun ${m.name}(${m.signature || ""})`,
          documentation: doc
            ? { kind: "markdown", value: doc }
            : `${className ? className + "." : ""}${m.name}`,
          parameters: (m.signature || "")
            .split(",")
            .filter((p) => p.trim())
            .map((p) => ({ label: p.trim() })),
        };
      }),
      activeSignature: 0,
      activeParameter: countActiveParam(beforeCursor),
    };
  }

  // 计算当前光标是第几个参数
  function countActiveParam(before) {
    let depth = 0;
    let commas = 0;
    for (let i = before.length - 1; i >= 0; i--) {
      const ch = before[i];
      if (ch === ")" || ch === "]") depth++;
      else if (ch === "(" || ch === "[") {
        if (depth === 0) break;
        depth--;
      } else if (ch === "," && depth === 0) commas++;
    }
    return commas;
  }

  // 1. new ClassName(  → 构造函数签名
  const newMatch = beforeCursor.match(/\bnew\s+([A-Z]\w*)\s*\($/);
  if (newMatch) {
    const def = classCache.get(newMatch[1]);
    if (def) {
      const ctors = def.members.filter((m) => m.kind === "constructor");
      if (ctors.length > 0) return buildSigResult(ctors, newMatch[1]);
    }
  }

  // 2. obj.method(  → 成员方法签名（支持链式推断）
  const memberCallMatch = beforeCursor.match(/^(.*)\.\s*([a-zA-Z_]\w*)\s*\($/);
  if (memberCallMatch) {
    const exprPart = memberCallMatch[1];
    const methodName = memberCallMatch[2];
    refreshLocalClassCache(text, params.textDocument.uri);
    const targetType = evaluateChainType(
      exprPart,
      params.textDocument.uri,
      params.position.line,
      text,
    );
    if (targetType) {
      const allMembers = getMembersIncludingInherited(targetType);
      const methods = allMembers.filter(
        (m) => m.name === methodName && m.kind === "method",
      );
      if (methods.length > 0) return buildSigResult(methods, targetType);
    }
  }

  // 3. 全局函数 fname(
  const globalFnMatch = beforeCursor.match(/\b([a-zA-Z_]\w*)\s*\($/);
  if (globalFnMatch) {
    const fnName = globalFnMatch[1];
    // 在 classCache 里找
    for (const def of classCache.values()) {
      if (!def._file && Array.isArray(def.members)) {
        const curClass = findClassAtLine(params.textDocument.uri, params.position.line);
        if (!curClass || def.className !== curClass.className) continue;
        const methods = def.members.filter(
          (m) => m.name === fnName && m.kind === "method",
        );
        if (methods.length > 0) return buildSigResult(methods, def.className);
      }
    }
    // 直接正则扫描全局 fun fnName(...)
    const lines2 = text.split("\n");
    const funRegex = new RegExp(
      `^\\s*(?:(?:public|private|protected|static|abstract)\\s+)*fun\\s+${fnName}\\s*\\(([^)]*)\\)`,
    );
    for (let i = 0; i < lines2.length; i++) {
      const fm = funRegex.exec(lines2[i]);
      if (fm) {
        const docStr = extractDocComment(text, i);
        const sig = fm[1].trim();
        return {
          signatures: [
            {
              label: `fun ${fnName}(${sig})`,
              documentation: docStr
                ? { kind: "markdown", value: docStr }
                : `fun ${fnName}`,
              parameters: sig
                .split(",")
                .filter((p) => p.trim())
                .map((p) => ({ label: p.trim() })),
            },
          ],
          activeSignature: 0,
          activeParameter: countActiveParam(beforeCursor),
        };
      }
    }
  }

  return null;
});

connection.onHover((params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return null;
  const text = doc.getText();
  refreshLocalClassCache(text, params.textDocument.uri);
  const currentLine = text.split("\n")[params.position.line];
  let start = params.position.character;
  let end = params.position.character;

  while (start > 0 && /[a-zA-Z_0-9]/.test(currentLine[start - 1])) start--;
  while (end < currentLine.length && /[a-zA-Z_0-9]/.test(currentLine[end]))
    end++;
  if (start === end) return null;
  const word = currentLine.substring(start, end);

  // ⭐ 优先级 1：点号访问的类成员 (例如 A.a 或 err.toString() 或 MathToolkit.square() 或 new Test().a() 或 new Test().a().b())
  const beforeHover = currentLine.slice(0, start);
  const isMemberHover = /\.\s*$/.test(beforeHover);
  if (isMemberHover) {
    const exprBeforeDot = extractExpressionBeforeDot(beforeHover);
    let hoverTargetClass = null;
    if (exprBeforeDot) {
      hoverTargetClass = evaluateChainType(
        exprBeforeDot,
        params.textDocument.uri,
        params.position.line,
        text,
      );
    }
    if (hoverTargetClass) {
      const allMembers = getMembersIncludingInherited(hoverTargetClass);
      const member = allMembers.find((m) => m.name === word);
      if (member) {
        const docComment = extractDocComment(text, member.location?.range?.start?.line);
        if (member.kind === "method") {
          const retType =
            member.returnType ||
            inferMethodReturnType(member, hoverTargetClass, params.textDocument.uri, text) ||
            "";
          const retPart = retType && !["void", "null"].includes(retType) ? `: ${retType}` : "";
          let mdValue = `\`\`\`tzdlang\n${member.isStatic ? "static " : ""}fun ${member.name}(${member.signature || ""})${retPart}\n\`\`\``;
          if (docComment) mdValue += `\n\n---\n${docComment}`;
          return { contents: { kind: MarkupKind.Markdown, value: mdValue } };
        } else {
          const typePart = member.type ? ` ${member.type}` : "";
          let mdValue = `\`\`\`tzdlang\nvar${typePart} ${member.name}\n\`\`\``;
          if (docComment) mdValue += `\n\n---\n${docComment}`;
          return { contents: { kind: MarkupKind.Markdown, value: mdValue } };
        }
      }
    }
    // ⭐ 核心修复：如果是点号访问，目标类型不是类或未匹配到成员，返回 null，绝不误报全局同名变量或类
    return null;
  }

  // ⭐ 优先级 2：当前文件或导入的类定义
  const classDef = classCache.get(word);
  if (classDef && !classDef._file) {
    return {
      contents: {
        kind: MarkupKind.Markdown,
        value: `\`\`\`tzdlang\nclass ${word}\n\`\`\``,
      },
    };
  }

  // ⭐ 优先级 3：本文件内的局部变量或函数（调用推断模块预测类型）
  const visibleLocals = extractVisibleLocals(text);
  const local = visibleLocals.find((v) => v.name === word);
  if (local) {
    if (local.kind === "function") {
      return {
        contents: {
          kind: MarkupKind.Markdown,
          value: `\`\`\`tzdlang\nfun ${word}()\n\`\`\``,
        },
      };
    } else {
      const inferredType =
        resolveVarType(
          word,
          text,
          params.textDocument.uri,
          params.position.line,
        ) || "any";
      return {
        contents: {
          kind: MarkupKind.Markdown,
          value: `\`\`\`tzdlang\n${inferredType} ${word}\n\`\`\``,
        },
      };
    }
  }

  // ⭐ 优先级 4 (最后兜底)：Native C++ 系统函数或类
  if (cachedRuntimeSymbols.functions.includes(word)) {
    return {
      contents: {
        kind: MarkupKind.Markdown,
        value: `\`\`\`cpp\nNative C++ Function: ${word}()\n\`\`\``,
      },
    };
  }
  if (cachedRuntimeSymbols.classes.includes(word)) {
    return {
      contents: {
        kind: MarkupKind.Markdown,
        value: `\`\`\`cpp\nNative C++ Class: ${word}\n\`\`\``,
      },
    };
  }

  return null;
});

// ──────────────────────────────────────────────────────────────────────────
// 全局检查：未声明变量/函数 + 语法错误
// ──────────────────────────────────────────────────────────────────────────

function checkUndeclared(text, docUri) {
  const diags = [];
  const lines = text.split("\n");
  const allowed = new Set(KEYWORDS);

  for (const cls of classCache.keys()) {
    if (!cls.startsWith("file:///") && !cls.startsWith("_")) allowed.add(cls);
  }
  for (const fn of globalFunctionsCache) allowed.add(fn);
  for (const c of cachedRuntimeSymbols.classes) allowed.add(c);
  for (const f of cachedRuntimeSymbols.functions) allowed.add(f);

  const visibleLocals = extractVisibleLocals(text);
  for (const v of visibleLocals) allowed.add(v.name);

  // 把当前文件的所有类定义以及类成员、参数也加进去（防止类似 MathToolkit 或 op 误报）
  const localClasses = extractClassDefs(parseTzd(text).tree, text, docUri);
  for (const cls of localClasses) {
    allowed.add(cls.className);
    for (const member of cls.members) {
      allowed.add(member.name);
      if (member.signature) {
        member.signature.split(",").forEach((arg) => {
          const info = extractParamInfo(arg);
          if (info) allowed.add(info.name);
        });
      }
    }
  }

  // ⭐ 先整体剥离多行块注释 /* ... */ 再按行扫描，防止注释内单词触发误报
  let strippedText = text.replace(/\/\*[\s\S]*?\*\//g, (m) =>
    m.split("\n").map((ln, i) => (i === 0 ? "" : " ".repeat(ln.length))).join("\n")
  );
  const strippedLines = strippedText.split("\n");

  for (let i = 0; i < lines.length; i++) {
    let origLine = lines[i];
    let cleanLine = strippedLines[i]
      .replace(/"(?:[^"\\]|\\.)*"/g, '""')
      .replace(/\/\/.*/, "");

    let fnRegex = /(?:^|[^\w\.])([a-zA-Z_]\w*)\s*\(/g;
    let match;
    while ((match = fnRegex.exec(cleanLine)) !== null) {
      let name = match[1];
      if (!allowed.has(name)) {
        let col = origLine.indexOf(name, match.index);
        diags.push(
          Diagnostic.create(
            Range.create(i, col, i, col + name.length),
            `未声明的函数：'${name}'`,
            DiagnosticSeverity.Error,
            "tzdlang",
          ),
        );
      }
    }

    let varRegex = /(?:^|[^\w\.])([a-zA-Z_]\w*)(?![\w\(\.:])/g;
    while ((match = varRegex.exec(cleanLine)) !== null) {
      let name = match[1];
      if (!allowed.has(name)) {
        // 如果当前是 class X 声明中的类名，或者 fun X 中的函数名，不误报
        if (new RegExp(`^\\s*class\\s+${name}\\b`).test(cleanLine)) continue;
        if (
          new RegExp(
            `^\\s*(?:(?:public|private|protected|static|abstract)\\s+)*fun\\s+${name}\\b`,
          ).test(cleanLine)
        )
          continue;

        let col = origLine.indexOf(name, match.index);
        diags.push(
          Diagnostic.create(
            Range.create(i, col, i, col + name.length),
            `未声明的变量：'${name}'`,
            DiagnosticSeverity.Error,
            "tzdlang",
          ),
        );
      }
    }
  }
  return diags;
}

function semanticCheck(text, docUri) {
  const diags = [];
  const lines = text.split("\n");
  const localTypes = new Map();

  for (let i = 0; i < lines.length; i++) {
    const invalidOpMatch = lines[i].match(
      /([a-zA-Z_]\w*)\s*(?:\+\+|--)|(?:\+\+|--)\s*([a-zA-Z_]\w*)/,
    );
    if (invalidOpMatch) {
      const varName = invalidOpMatch[1] || invalidOpMatch[2];
      if (localTypes.get(varName) === "string") {
        const idx = lines[i].indexOf(invalidOpMatch[0]);
        diags.push(
          Diagnostic.create(
            Range.create(i, idx, i, idx + invalidOpMatch[0].length),
            `非法操作：不能对字符串变量 '${varName}' 使用自增或自减运算。`,
            DiagnosticSeverity.Error,
            "tzdlang",
          ),
        );
      }
    }
  }
  diags.push(...checkUndeclared(text, docUri));
  return diags;
}

async function validateDocument(document) {
  const text = document.getText();
  const { tree, diagnostics } = parseTzd(text);

  loadImportClasses(text, document.uri);
  const localDefs = extractClassDefs(tree, text, document.uri);
  for (const def of localDefs) classCache.set(def.className, def);

  const allDiags = [...diagnostics, ...semanticCheck(text, document.uri)];
  connection.sendDiagnostics({ uri: document.uri, diagnostics: allDiags });

  // ⭐ 递归检测：分析函数与类方法递归调用，向客户端发送可视化箭头标记
  try {
    const recursions = detectRecursions(text, tree);
    connection.sendNotification("tzdlang/recursions", {
      uri: document.uri,
      items: recursions,
    });
  } catch (err) {
    console.error("Recursion detection error:", err);
  }
}

connection.onRequest("tzdlang/getRecursions", (params) => {
  if (!params || !params.uri) return [];
  const doc = documents.get(params.uri);
  if (!doc) return [];
  try {
    const text = doc.getText();
    const { tree } = parseTzd(text);
    return detectRecursions(text, tree);
  } catch (_) {
    return [];
  }
});

connection.onInitialize((params) => {
  const toolsPath = params.initializationOptions?.toolsPath || "";
  if (toolsPath) {
    fetchRuntimeSymbolsFromCpp(toolsPath);
  }
  return {
    capabilities: {
      textDocumentSync: { openClose: true, change: TextDocumentSyncKind.Full },
      completionProvider: { triggerCharacters: [".", ":"] },
      signatureHelpProvider: { triggerCharacters: ["("] },
      hoverProvider: true,
      definitionProvider: true,
    },
  };
});

documents.onDidOpen((e) => validateDocument(e.document));
documents.onDidChangeContent((e) => validateDocument(e.document));

connection.onCompletion(async (params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return [];
  return buildCompletions(
    doc.getText(),
    params.position.line,
    params.position.character,
    doc.uri,
  );
});

function findDefinition(docUri, text, line, col) {
  refreshLocalClassCache(text, docUri);
  const lines = text.split("\n");
  if (line >= lines.length) return null;

  const currentLine = lines[line];
  let start = col;
  while (start > 0 && /[a-zA-Z_0-9]/.test(currentLine[start - 1])) start--;
  let end = col;
  while (end < currentLine.length && /[a-zA-Z_0-9]/.test(currentLine[end]))
    end++;

  if (start === end) return null;
  const word = currentLine.substring(start, end);
  const escaped = word.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");

  // ⭐ 新增功能：计算鼠标所在位置的方法调用参数个数
  const afterWord = currentLine.substring(start);
  const callMatch = afterWord.match(/^[a-zA-Z_]\w*\s*\(([^)]*)\)/);
  let calledArgsCount = -1; // -1 表示这不是一个函数调用，或者无法识别参数
  if (callMatch) {
    const argsStr = callMatch[1].trim();
    calledArgsCount = argsStr.length > 0 ? argsStr.split(",").length : 0;
  }

  // 1. Super 构造函数跳转
  if (word === "super") {
    const currentClass = findClassAtLine(docUri, line);
    if (
      currentClass &&
      currentClass.parentName &&
      classCache.has(currentClass.parentName)
    ) {
      const pDef = classCache.get(currentClass.parentName);
      const ctors = pDef.members.filter((m) => m.kind === "constructor");
      // 尝试匹配相同参数个数的构造函数
      let target = ctors.find((c) => c.argsCount === calledArgsCount);
      if (!target) target = ctors[0];
      if (target && target.location) return target.location;
      return {
        uri: pDef.file,
        range: Range.create(pDef.range.start.line, 0, pDef.range.start.line, 5),
      };
    }
    return null;
  }

  // 2. 点号成员跳转 (例如 e.message 或 MathToolkit.square(6) 或 new Test().a() 或 new Test().a().b())
  const beforeCursor = currentLine.slice(0, start);
  const isMemberAccess = /\.\s*$/.test(beforeCursor);

  if (isMemberAccess) {
    const exprBeforeDot = extractExpressionBeforeDot(beforeCursor);
    let targetClass = null;
    if (exprBeforeDot) {
      targetClass = evaluateChainType(exprBeforeDot, docUri, line, text);
    }
    if (targetClass) {
      const allMembers = getMembersIncludingInherited(targetClass);
      const matches = allMembers.filter((x) => x.name === word);
      if (matches.length > 0) {
        // ⭐ 精确匹配重载方法参数个数
        let target = matches.find((m) => m.argsCount === calledArgsCount);
        if (!target) target = matches[0]; // 如果没完全匹配的，返回第一个兜底
        if (target.location) return target.location;
      }
    }
    // ⭐ 核心修复：如果是点号成员访问，但在目标类或非对象类型（如 int）中未匹配到该成员，绝不能向下兜底匹配其它无关类的同名成员！
    return null;
  }

  // 2.5 如果当前光标在某个类内，且调用的方法/属性属于该类或其父类（支持类似 fun a() { b(); } 的直接成员调用）
  const curClassDef = findClassAtLine(docUri, line);
  if (curClassDef) {
    const classMembers = getMembersIncludingInherited(curClassDef.className);
    const matches = classMembers.filter((x) => x.name === word);
    if (matches.length > 0) {
      let target = matches.find((m) => m.argsCount === calledArgsCount);
      if (!target) target = matches[0];
      if (target.location) return target.location;
    }
  }

  // 3. 类名跳转
  const def = classCache.get(word);
  if (def && def.file) {
    return {
      uri: def.file,
      range: Range.create(
        def.range.start.line,
        0,
        def.range.start.line,
        word.length,
      ),
    };
  }

  // 4. 当前函数/方法内的参数跳转 (例如 static fun apply(op: function, val) 内的 op 或 val)
  for (let i = line; i >= 0; i--) {
    const l = lines[i];
    const fnMatch =
      l.match(
        /\b(?:(?:public|private|protected|static|abstract)\s+)*fun\s+([a-zA-Z_]\w*)\s*\((.*?)\)/,
      ) || l.match(/^\s*[A-Z]\w*\s*\((.*?)\)/);
    if (fnMatch) {
      const paramStr = fnMatch[2] || fnMatch[1];
      const paramIdx = l.indexOf(paramStr);
      const pParts = paramStr.split(",");
      let pOffset = paramIdx;
      for (const p of pParts) {
        const pRegex = new RegExp(`\\b${escaped}\\b`);
        const m = p.match(pRegex);
        if (m) {
          const colPos = pOffset + m.index;
          return {
            uri: docUri,
            range: Range.create(i, colPos, i, colPos + word.length),
          };
        }
        pOffset += p.length + 1;
      }
      break;
    }
  }

  // ⭐ 5. 本地/全局函数精准正则匹配 (支持 static fun, public fun 等修饰符)
  const fnPattern = new RegExp(
    `^\\s*(?:(?:public|private|protected|static|abstract)\\s+)*fun\\s+${escaped}\\s*\\(([^)]*)\\)`,
  );
  let fallbackLoc = null;
  for (let i = 0; i < lines.length; i++) {
    const fm = fnPattern.exec(lines[i]);
    if (fm) {
      const defArgsStr = fm[1].trim();
      const defArgsCount =
        defArgsStr.length > 0 ? defArgsStr.split(",").length : 0;
      const matchCol = lines[i].indexOf(word);
      const loc = {
        uri: docUri,
        range: Range.create(i, matchCol, i, matchCol + word.length),
      };

      // 如果参数严格匹配，直接返回
      if (calledArgsCount !== -1 && defArgsCount === calledArgsCount)
        return loc;
      if (!fallbackLoc) fallbackLoc = loc;
    }
  }
  if (fallbackLoc) return fallbackLoc; // 如果没有严格匹配的重载，就返回找到的第一个

  // 6. 如果在某个类内，且上述未匹配到，则在当前类或其继承链中尝试兜底查找
  // 避免在顶层调用未定义函数时错误跳转到不相关的类方法
  if (curClassDef) {
    const classMembers = getMembersIncludingInherited(curClassDef.className);
    const m = classMembers.find((x) => x.name === word);
    if (m && m.location) return m.location;
  }

  // 7. 本地普通变量兜底匹配
  const varPatterns = [
    new RegExp(
      `\\b(?:var|let|const|string|int|float|bool|ptr|function)\\s+(${escaped})\\b`,
    ),
    new RegExp(`^\\s*[A-Z][a-zA-Z0-9_]*\\s+(${escaped})\\s*=`),
  ];

  for (let i = 0; i < lines.length; i++) {
    for (const pat of varPatterns) {
      const m = pat.exec(lines[i]);
      if (m) {
        const matchCol = lines[i].indexOf(m[1]);
        return {
          uri: docUri,
          range: Range.create(i, matchCol, i, matchCol + word.length),
        };
      }
    }
  }

  return null;
}

connection.onDefinition(async (params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return null;
  return findDefinition(
    doc.uri,
    doc.getText(),
    params.position.line,
    params.position.character,
  );
});

documents.listen(connection);
connection.listen();
console.log("TzdLang Language Server started");
