// ─── TzdLang Language Server ─────────────────────────────────────────────
// 修复：局部变量带类型声明（var int a）和构造函数参数（Thread(target)）误报未声明的问题
// 新增：动态加载 C++ 解释器系统自带的 Native 类和函数

import antlr4 from "antlr4";
import TzdLangLexer from "../TzdLangLexer.mjs";
import TzdLangParser from "../TzdLangParser.mjs";

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
const cachedRuntimeSymbols = { classes: [], functions: [] };

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

    if (error || stderr) {
      connection.console.error("[TzdLang] 动态同步 C++ 符号失败。");
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
        cachedRuntimeSymbols.classes = classesMatch[1]
          .replace(/"/g, "")
          .split(",")
          .map((item) => item.trim())
          .filter((item) => item.length > 0);
      }
      if (functionsMatch && functionsMatch[1]) {
        cachedRuntimeSymbols.functions = functionsMatch[1]
          .replace(/"/g, "")
          .split(",")
          .map((item) => item.trim())
          .filter((item) => item.length > 0);
      }

      connection.console.log(
        `[TzdLang] 成功同步 C++ 符号！加载系统函数: ${cachedRuntimeSymbols.functions.length} 个, 系统类: ${cachedRuntimeSymbols.classes.length} 个`,
      );
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
        const children = node.children || [];
        for (let i = 0; i < children.length; i++) {
          const txt = safeText(children[i]);
          if (txt === "extends" || txt === ":") {
            if (i + 1 < children.length)
              parentName = safeText(children[i + 1]) || null;
          }
        }

        const startLine = node.start ? Math.max(node.start.line - 1, 0) : 0;
        const stopLine = node.stop
          ? Math.max(node.stop.line - 1, 0)
          : lines.length - 1;
        const members = [];

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
              /^\s*(?:var|let|const)\s+(?:[a-zA-Z_]\w*\s+)?([a-zA-Z_]\w*)/,
            );
            if (m) {
              const col = lineStr.indexOf(m[1]);
              members.push({
                name: m[1],
                kind: "field",
                location: {
                  uri,
                  range: Range.create(i, col, i, col + m[1].length),
                },
              });
              continue;
            }
            m = lineStr.match(/^\s*fun\s+([a-zA-Z_]\w*)\s*\((.*?)\)/);
            if (m) {
              const col = lineStr.indexOf(m[1]);
              // ⭐ 保存 method 的参数个数，用于重载精确跳转
              const args = m[2]
                .split(",")
                .map((x) => x.trim())
                .filter((x) => x.length > 0);
              members.push({
                name: m[1],
                kind: "method",
                signature: m[2],
                argsCount: args.length,
                location: {
                  uri,
                  range: Range.create(i, col, i, col + m[1].length),
                },
              });
              continue;
            }
            m = lineStr.match(new RegExp(`^\\s*${className}\\s*\\((.*?)\\)`));
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

    // 3. 函数声明自身及参数 (如 fun aaa(x, y))
    m = line.match(/\bfun\s+([a-zA-Z_]\w*)\s*\((.*?)\)/);
    if (m) {
      locals.set(m[1], { kind: "function", line: i }); // ⭐ 修复1：将函数名 aaa 加入白名单
      m[2].split(",").forEach((arg) => {
        const p = arg.trim().split(/\s+/).pop();
        if (p) locals.set(p, { kind: "parameter", line: i });
      });
    }

    // 4. 全局隐式声明/赋值 (如 c = 10; 排除 A.c = 10)
    m = line.match(
      /(?:^|[{};]\s*)\b([a-zA-Z_]\w*)\s*(?:\+|-|\*|\/|%|&|\||\^)?=/,
    );
    if (m) locals.set(m[1], { kind: "variable", line: i }); // ⭐ 修复2：支持无关键字的变量直接赋值，例如 c = 10

    // 5. 构造函数参数 (如 Thread(target))
    m = line.match(/^\s*[A-Z]\w*\s*\((.*?)\)/);
    if (m) {
      m[1].split(",").forEach((arg) => {
        const p = arg.trim().split(/\s+/).pop();
        if (p) locals.set(p, { kind: "parameter", line: i });
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

  // 3. 字面量赋值推断 (例如: x = "100" / x = 12.3)
  const assignRegex = new RegExp(
    `\\b(?:var|let|const|)\\s*${varName}\\s*=\\s*(.*);?`,
  );
  for (const line of lines) {
    let m = line.match(assignRegex);
    if (m) {
      let val = m[1].trim();
      if (val.startsWith('"') || val.startsWith("'")) return "string";
      if (val === "true" || val === "false") return "bool";
      if (/^-?\d+\.\d+$/.test(val)) return "float";
      if (/^-?\d+$/.test(val)) return "int";
    }
  }

  return null;
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
  const items = [];
  const lines = text.split("\n");
  const before = (lines[line] || "").slice(0, col);

  if (before.endsWith("/**")) {
    return [
      {
        label: "/** */ (Doc block)",
        kind: CompletionItemKind.Snippet,
        insertTextFormat: InsertTextFormat.Snippet,
        insertText: " * $0\n */",
        sortText: "0",
      },
    ];
  }

  const classVarMatch = before.match(/([A-Z][a-zA-Z0-9_]*)\s+$/);
  if (classVarMatch && classCache.has(classVarMatch[1])) {
    const clsName = classVarMatch[1];
    const lowerName = clsName.charAt(0).toLowerCase() + clsName.slice(1);
    items.push({
      label: lowerName,
      kind: CompletionItemKind.Variable,
      detail: `推荐变量名`,
      sortText: "0",
    });
    items.push({
      label: lowerName + "1",
      kind: CompletionItemKind.Variable,
      sortText: "1",
    });
  }

  const newMatch =
    before.match(/([A-Z][a-zA-Z0-9_]*)\s+[a-zA-Z_]\w*\s*=\s*new\s*$/) ||
    before.match(/\bnew\s*$/);
  if (newMatch) {
    const expectedType = newMatch[1] || null;
    const expectedParent =
      expectedType && classCache.has(expectedType)
        ? classCache.get(expectedType).parentName
        : null;

    for (const [className, def] of classCache.entries()) {
      if (
        def._file ||
        !className ||
        className.startsWith("_") ||
        className.startsWith("file:///")
      )
        continue;
      let sort =
        className === expectedType
          ? "0"
          : className === expectedParent
            ? "1"
            : "3";
      items.push({
        label: className,
        kind: CompletionItemKind.Constructor,
        insertTextFormat: InsertTextFormat.Snippet,
        insertText: `${className}($0)`,
        detail: def.parentName ? `extends ${def.parentName}` : "class",
        sortText: sort,
      });
    }
    for (const c of cachedRuntimeSymbols.classes) {
      items.push({
        label: c,
        kind: CompletionItemKind.Class,
        insertTextFormat: InsertTextFormat.Snippet,
        insertText: `${c}($0)`,
        detail: "Native C++ Class",
        sortText: "4",
      });
    }
    return items;
  }

  const dotMatch = before.match(/([a-zA-Z_][a-zA-Z0-9_]*)\.$/);
  if (dotMatch) {
    const isSuper = dotMatch[1] === "super"; // ⭐ 标记当前是否是在写 super.
    const varType = resolveVarType(dotMatch[1], text, docUri, line);
    if (varType) {
      const allMembers = getMembersIncludingInherited(varType);
      const seen = new Set();
      for (const member of allMembers) {
        if (member.kind === "constructor" || seen.has(member.name)) continue;

        // ⭐ 新增功能：如果是 super.，直接跳过非函数的属性
        if (isSuper && member.kind !== "method") continue;

        seen.add(member.name);
        if (member.kind === "method") {
          items.push({
            label: member.name,
            kind: CompletionItemKind.Method,
            insertTextFormat: InsertTextFormat.Snippet,
            insertText: `${member.name}($0)`,
            detail: `fun ${member.name}(${member.signature || ""})`,
            sortText: "1",
          });
        } else {
          items.push({
            label: member.name,
            kind: CompletionItemKind.Field,
            detail: "property",
            sortText: "2",
          });
        }
      }
    }
    return items;
  }

  // ⭐ 新增功能：重写父类方法自动完成 (Override)
  const currentClass = findClassAtLine(docUri, line);
  if (currentClass && currentClass.parentName) {
    const parentMembers = getMembersIncludingInherited(currentClass.parentName);
    const seenOverride = new Set();
    for (const pm of parentMembers) {
      if (pm.kind === "method" && !seenOverride.has(pm.name)) {
        seenOverride.add(pm.name);
        items.push({
          label: pm.name,
          kind: CompletionItemKind.Snippet,
          insertTextFormat: InsertTextFormat.Snippet,
          insertText: `${pm.name}(${pm.signature || ""}) {\n\t$0\n}`, // 自动搭好函数壳子
          detail: `重写父类方法 (Override)`,
          sortText: "0", // 优先级排最高
        });
      }
    }
  }

  for (const [className, def] of classCache.entries()) {
    if (
      !def._file &&
      className &&
      !className.startsWith("file:///") &&
      !className.startsWith("_")
    ) {
      items.push({
        label: className,
        kind: CompletionItemKind.Class,
        detail: "class",
        sortText: "3",
      });
    }
  }

  for (const f of cachedRuntimeSymbols.functions) {
    items.push({
      label: f,
      kind: CompletionItemKind.Function,
      insertTextFormat: InsertTextFormat.Snippet,
      insertText: `${f}($0)`,
      detail: "Native C++ Function",
      sortText: "3",
    });
  }

  for (const v of extractVisibleLocals(text)) {
    if (!KEYWORDS.has(v.name)) {
      items.push({
        label: v.name,
        kind: CompletionItemKind.Variable,
        detail: v.type || v.kind,
        sortText: "4",
      });
    }
  }

  for (const [label, insert, doc] of KEYWORD_ITEMS) {
    items.push({
      label,
      kind: CompletionItemKind.Keyword,
      insertText: insert,
      insertTextFormat: InsertTextFormat.Snippet,
      detail: doc,
      sortText: "5",
    });
  }

  return items;
}

connection.onSignatureHelp((params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return null;
  const lines = doc.getText().split("\n");
  const beforeCursor = lines[params.position.line].substring(
    0,
    params.position.character,
  );

  const newMatch = beforeCursor.match(/\bnew\s+([A-Z]\w*)\s*\($/);
  if (newMatch) {
    const def = classCache.get(newMatch[1]);
    if (def) {
      const ctors = def.members.filter((m) => m.kind === "constructor");
      if (ctors.length > 0) {
        return {
          signatures: ctors.map((c) => ({
            label: `${newMatch[1]}(${c.signature || ""})`,
            documentation: `构造函数`,
            parameters: (c.signature || "")
              .split(",")
              .filter((p) => p.trim())
              .map((p) => ({ label: p.trim() })),
          })),
          activeSignature: 0,
          activeParameter: 0,
        };
      }
    }
  }
  return null;
});

connection.onDefinition(async (params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return null;
  const lines = doc.getText().split("\n");
  const currentLine = lines[params.position.line];
  let start = params.position.character;
  let end = params.position.character;
  while (start > 0 && /[a-zA-Z_0-9]/.test(currentLine[start - 1])) start--;
  while (end < currentLine.length && /[a-zA-Z_0-9]/.test(currentLine[end]))
    end++;
  if (start === end) return null;
  const word = currentLine.substring(start, end);

  const dotMatch = currentLine
    .slice(0, start)
    .match(/([a-zA-Z_][a-zA-Z0-9_]*)\.\s*$/);
  if (dotMatch) {
    const varType = resolveVarType(
      dotMatch[1],
      doc.getText(),
      doc.uri,
      params.position.line,
    );
    if (varType) {
      const m = getMembersIncludingInherited(varType).find(
        (x) => x.name === word,
      );
      if (m && m.location) return m.location;
    }
  }
  const def = classCache.get(word);
  if (def && def.file)
    return {
      uri: def.file,
      range: Range.create(
        def.range.start.line,
        0,
        def.range.start.line,
        word.length,
      ),
    };
  return null;
});

connection.onHover((params) => {
  const doc = documents.get(params.textDocument.uri);
  if (!doc) return null;
  const text = doc.getText();
  const currentLine = text.split("\n")[params.position.line];
  let start = params.position.character;
  let end = params.position.character;

  while (start > 0 && /[a-zA-Z_0-9]/.test(currentLine[start - 1])) start--;
  while (end < currentLine.length && /[a-zA-Z_0-9]/.test(currentLine[end]))
    end++;
  if (start === end) return null;
  const word = currentLine.substring(start, end);

  // ⭐ 优先级 1：点号访问的类成员 (例如 A.a 或 err.toString())
  const dotMatch = currentLine
    .slice(0, start)
    .match(/([a-zA-Z_][a-zA-Z0-9_]*)\.\s*$/);
  if (dotMatch) {
    const varType = resolveVarType(
      dotMatch[1],
      text,
      params.textDocument.uri,
      params.position.line,
    );
    if (varType) {
      const allMembers = getMembersIncludingInherited(varType);
      const member = allMembers.find((m) => m.name === word);
      if (member) {
        if (member.kind === "method")
          return {
            contents: {
              kind: MarkupKind.Markdown,
              value: `\`\`\`tzdlang\nfun ${member.name}(${member.signature || ""})\n\`\`\``,
            },
          };
        else
          return {
            contents: {
              kind: MarkupKind.Markdown,
              value: `\`\`\`tzdlang\nvar ${member.name}\n\`\`\``,
            },
          };
      }
    }
    return null; // 如果有点号，但没找到，直接返回空，绝不能当成全局系统函数
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

  // 把当前文件的所有类成员也加进去（防止类似 var target; this.target = target 误报）
  const localClasses = extractClassDefs(parseTzd(text).tree, text, docUri);
  for (const cls of localClasses) {
    for (const member of cls.members) allowed.add(member.name);
  }

  for (let i = 0; i < lines.length; i++) {
    let origLine = lines[i];
    let cleanLine = origLine
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
}

connection.onInitialize((params) => {
  const toolsPath = params.initializationOptions?.toolsPath || "";
  if (toolsPath) {
    fetchRuntimeSymbolsFromCpp(toolsPath);
  }
  return {
    capabilities: {
      textDocumentSync: { openClose: true, change: TextDocumentSyncKind.Full },
      completionProvider: { triggerCharacters: [".", '"', "'", ":", " ", "*"] },
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

  // 2. 点号成员跳转 (例如 e.message 或 A.aaa(10))
  const beforeCursor = currentLine.slice(0, start);
  const dotMatch = beforeCursor.match(/([a-zA-Z_][a-zA-Z0-9_]*)\.\s*$/);
  if (dotMatch) {
    const varType = resolveVarType(dotMatch[1], text, docUri, line);
    if (varType) {
      const allMembers = getMembersIncludingInherited(varType);
      const matches = allMembers.filter((x) => x.name === word);
      if (matches.length > 0) {
        // ⭐ 精确匹配重载方法参数个数
        let target = matches.find((m) => m.argsCount === calledArgsCount);
        if (!target) target = matches[0]; // 如果没完全匹配的，返回第一个兜底
        if (target.location) return target.location;
      }
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

  const escaped = word.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");

  // ⭐ 4. 本地/全局函数精准正则匹配
  if (calledArgsCount !== -1) {
    const fnPattern = new RegExp(`^\\s*fun\\s+${escaped}\\s*\\(([^)]*)\\)`);
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
        if (defArgsCount === calledArgsCount) return loc;
        if (!fallbackLoc) fallbackLoc = loc;
      }
    }
    if (fallbackLoc) return fallbackLoc; // 如果没有严格匹配的重载，就返回找到的第一个
  }

  // 5. 本地普通变量兜底匹配
  const varPatterns = [
    new RegExp(
      `\\b(?:var|let|const|string|int|float|bool|ptr)\\s+(${escaped})\\b`,
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
