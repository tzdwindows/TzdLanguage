// ─── TzdLang Recursion Detector ──────────────────────────────────────────────
// 分析 TzdLang 源代码中的函数及类方法调用关系，识别自递归与互相递归循环。
// 提供行号、列区间、递归类型及详细调用链提示，支持类似于 IntelliJ IDEA 的循环箭头显示。
// ──────────────────────────────────────────────────────────────────────────────

import antlr4 from "antlr4";
import TzdLangLexer from "../TzdLangLexer.mjs";
import TzdLangParser from "../TzdLangParser.mjs";

/**
 * 使用 ANTLR 解析代码并构建 ParseTree
 */
export function parseTzd(text) {
  const chars = new antlr4.InputStream(text);
  const lexer = new TzdLangLexer(chars);
  const tokens = new antlr4.CommonTokenStream(lexer);
  const parser = new TzdLangParser(tokens);
  parser.buildParseTrees = true;
  parser.removeErrorListeners();
  try {
    return parser.program();
  } catch (_) {
    return null;
  }
}

/**
 * 递归收集指定语法节点下的所有函数调用表达式（跳过嵌套函数/类，以保持作用域隔离）
 */
function collectCalls(node, calls) {
  if (!node) return;

  if (node instanceof TzdLangParser.CallExprContext) {
    const atom = node.atom ? node.atom() : null;
    if (atom) {
      if (atom instanceof TzdLangParser.IdExprContext && atom.IDENTIFIER && atom.IDENTIFIER()) {
        const id = atom.IDENTIFIER();
        const sym = id.symbol || (typeof id.getSymbol === "function" ? id.getSymbol() : null);
        calls.push({
          type: "identifier",
          name: id.getText(),
          line: sym ? sym.line - 1 : (node.start ? node.start.line - 1 : 0),
          column: sym ? sym.column : (node.start ? node.start.column : 0),
          length: id.getText().length,
          fullText: node.getText ? node.getText() : id.getText() + "()",
        });
      } else if (atom instanceof TzdLangParser.MemberAccessExprContext && atom.IDENTIFIER && atom.IDENTIFIER()) {
        const id = atom.IDENTIFIER();
        const sym = id.symbol || (typeof id.getSymbol === "function" ? id.getSymbol() : null);
        const recvAtom = atom.atom ? atom.atom() : null;
        const recvText = recvAtom ? (recvAtom.getText ? recvAtom.getText() : "") : "";
        calls.push({
          type: "member",
          receiver: recvText,
          name: id.getText(),
          line: sym ? sym.line - 1 : (node.start ? node.start.line - 1 : 0),
          column: sym ? sym.column : (node.start ? node.start.column : 0),
          length: id.getText().length,
          fullText: node.getText ? node.getText() : (recvText ? `${recvText}.${id.getText()}()` : `${id.getText()}()`),
        });
      }
    }
  }

  const children = node.children || [];
  for (const child of children) {
    // 遇到嵌套函数或内部类定义时，跳过该子树，由外层 walk 单独处理其作用域
    if (
      child instanceof TzdLangParser.FunctionDeclarationContext ||
      child instanceof TzdLangParser.ClassDeclarationContext
    ) {
      continue;
    }
    collectCalls(child, calls);
  }
}

/**
 * 遍历语法树提取所有函数与类方法定义
 */
function extractFunctionsFromTree(tree) {
  const functions = [];

  function walk(node, currentClass = null) {
    if (!node) return;

    if (node instanceof TzdLangParser.ClassDeclarationContext) {
      const qn = typeof node.qualifiedName === "function" ? node.qualifiedName(0) : null;
      const className = qn ? qn.getText() : "Unknown";
      const body = typeof node.classBody === "function" ? node.classBody() : null;
      if (body && typeof body.classMember === "function") {
        const members = body.classMember() || [];
        for (const m of members) {
          const md = typeof m.memberDecl === "function" ? m.memberDecl() : null;
          if (!md) continue;

          let fnName = "";
          let isStatic = false;
          let blockNode = null;
          let sym = null;

          if (md instanceof TzdLangParser.MethodDeclContext) {
            const idNode = typeof md.IDENTIFIER === "function" ? md.IDENTIFIER() : null;
            fnName = idNode ? idNode.getText() : "";
            sym = idNode ? (idNode.symbol || (typeof idNode.getSymbol === "function" ? idNode.getSymbol() : null)) : null;
            blockNode = typeof md.block === "function" ? md.block() : null;
          } else if (md instanceof TzdLangParser.MethodStaticDeclContext) {
            const idNode = typeof md.IDENTIFIER === "function" ? md.IDENTIFIER() : null;
            fnName = idNode ? idNode.getText() : "";
            sym = idNode ? (idNode.symbol || (typeof idNode.getSymbol === "function" ? idNode.getSymbol() : null)) : null;
            isStatic = true;
            blockNode = typeof md.block === "function" ? md.block() : null;
          } else if (md instanceof TzdLangParser.ConstructorDeclContext) {
            const idNode = typeof md.IDENTIFIER === "function" ? md.IDENTIFIER() : null;
            fnName = idNode ? idNode.getText() : "";
            sym = idNode ? (idNode.symbol || (typeof idNode.getSymbol === "function" ? idNode.getSymbol() : null)) : null;
            blockNode = typeof md.block === "function" ? md.block() : null;
          }

          if (fnName && blockNode) {
            const calls = [];
            collectCalls(blockNode, calls);
            const startLine = sym ? sym.line - 1 : (md.start ? md.start.line - 1 : 0);
            const stopLine = md.stop ? md.stop.line - 1 : startLine;
            functions.push({
              name: fnName,
              qualifiedName: `${className}.${fnName}`,
              className,
              isStatic,
              headerLine: startLine,
              headerCol: sym ? sym.column : 0,
              headerLen: fnName.length,
              startLine,
              endLine: stopLine,
              calls,
            });
          }
        }
      }
      return;
    }

    if (node instanceof TzdLangParser.FunctionDeclarationContext) {
      const id = typeof node.IDENTIFIER === "function" ? node.IDENTIFIER() : null;
      const fnName = id ? id.getText() : "";
      const sym = id ? (id.symbol || (typeof id.getSymbol === "function" ? id.getSymbol() : null)) : null;
      const blockNode = typeof node.block === "function" ? node.block() : null;
      if (fnName && blockNode) {
        const calls = [];
        collectCalls(blockNode, calls);
        const startLine = sym ? sym.line - 1 : (node.start ? node.start.line - 1 : 0);
        const stopLine = node.stop ? node.stop.line - 1 : startLine;
        functions.push({
          name: fnName,
          qualifiedName: fnName,
          className: null,
          isStatic: false,
          headerLine: startLine,
          headerCol: sym ? sym.column : 0,
          headerLen: fnName.length,
          startLine,
          endLine: stopLine,
          calls,
        });
      }
    }

    const children = node.children || [];
    for (const child of children) {
      walk(child, currentClass);
    }
  }

  walk(tree);
  return functions;
}

/**
 * 文本级快速回退扫描器（在语法树因输入尚未结束导致不完整时生效）
 */
function scanFunctionsFromTextFallback(text) {
  const lines = text.split("\n");
  const functions = [];

  let currentClass = null;
  let classBraceDepth = 0;
  let currentFunc = null;
  let funcBraceDepth = 0;

  for (let lineIdx = 0; lineIdx < lines.length; lineIdx++) {
    const rawLine = lines[lineIdx];
    // 去除单行注释
    const line = rawLine.replace(/\/\/.*$/, "");

    // 检查类定义
    const classMatch = line.match(/\bclass\s+([A-Za-z0-9_]+)/);
    if (classMatch && !currentFunc) {
      currentClass = classMatch[1];
      classBraceDepth = 0;
    }

    // 检查函数定义 (fun name 或 static fun name)
    const funcMatch = line.match(/(?:(?:public|private|protected|static)\s+)*fun\s+([A-Za-z0-9_]+)\s*\(/);
    if (funcMatch && !currentFunc) {
      const fnName = funcMatch[1];
      const col = line.indexOf("fun " + fnName) + 4;
      currentFunc = {
        name: fnName,
        qualifiedName: currentClass ? `${currentClass}.${fnName}` : fnName,
        className: currentClass,
        isStatic: line.includes("static"),
        headerLine: lineIdx,
        headerCol: col > 0 ? col : 0,
        headerLen: fnName.length,
        startLine: lineIdx,
        endLine: lineIdx,
        calls: [],
      };
      funcBraceDepth = 0;
    }

    // 跟踪大括号
    for (let c = 0; c < line.length; c++) {
      if (line[c] === "{") {
        if (currentFunc) funcBraceDepth++;
        else if (currentClass) classBraceDepth++;
      } else if (line[c] === "}") {
        if (currentFunc) {
          funcBraceDepth--;
          if (funcBraceDepth <= 0) {
            currentFunc.endLine = lineIdx;
            functions.push(currentFunc);
            currentFunc = null;
          }
        } else if (currentClass) {
          classBraceDepth--;
          if (classBraceDepth <= 0) {
            currentClass = null;
          }
        }
      }
    }

    // 检查函数体内的调用
    if (currentFunc) {
      // 匹配 foo(...) 或 this.foo(...) 或 new Cls().foo(...)
      const callRegex = /(?:(\bthis|\bnew\s+[A-Za-z0-9_]+\s*\([^)]*\)|[A-Za-z0-9_]+)\.)?\b([A-Za-z0-9_]+)\s*\(/g;
      let m;
      while ((m = callRegex.exec(line)) !== null) {
        const recv = m[1] || "";
        const calledName = m[2];
        if (calledName === "if" || calledName === "while" || calledName === "for" || calledName === "switch" || calledName === "catch" || calledName === "fun") {
          continue;
        }
        currentFunc.calls.push({
          type: recv ? "member" : "identifier",
          receiver: recv,
          name: calledName,
          line: lineIdx,
          column: m.index + (recv ? recv.length + 1 : 0),
          length: calledName.length,
          fullText: m[0],
        });
      }
    }
  }

  if (currentFunc) {
    currentFunc.endLine = lines.length - 1;
    functions.push(currentFunc);
  }

  return functions;
}

/**
 * 递归检测核心入口
 * @param {string} text - 文档完整文本
 * @param {any} [tree] - 已解析的 ANTLR ParseTree（可选，避免重复解析）
 * @returns {Array<{ line: number, colStart: number, colEnd: number, targetName: string, kind: 'function'|'call', isMutual: boolean, message: string }>}
 */
export function detectRecursions(text, tree = null) {
  if (!text || typeof text !== "string") return [];

  let functions = [];
  try {
    const parseTree = tree || parseTzd(text);
    if (parseTree) {
      functions = extractFunctionsFromTree(parseTree);
    }
  } catch (_) {
    functions = [];
  }

  // 语法树解析为空或不完整时，回退到行级快速扫描
  if (!functions || functions.length === 0) {
    functions = scanFunctionsFromTextFallback(text);
  }

  if (functions.length === 0) return [];

  // 构建函数索引
  const funcMap = new Map();
  for (const f of functions) {
    funcMap.set(f.qualifiedName, f);
  }

  // 解析各个调用的具体目标
  for (const f of functions) {
    for (const c of f.calls) {
      let resolved = null;
      if (f.className) {
        // 类方法内部
        if (c.type === "member") {
          const recvClean = c.receiver.replace(/\s+/g, "");
          const isThis = (recvClean === "this");
          const isNewThisClass = (recvClean === `new${f.className}()`);
          const isStaticClass = (recvClean === f.className);

          if (isThis || isNewThisClass || isStaticClass) {
            if (funcMap.has(`${f.className}.${c.name}`)) {
              resolved = `${f.className}.${c.name}`;
            }
          } else {
            // 针对 new OtherClass().method() 的跨类调用
            for (const [qName, targetFn] of funcMap.entries()) {
              if (targetFn.className && c.name === targetFn.name) {
                if (recvClean === `new${targetFn.className}()` || recvClean === targetFn.className) {
                  resolved = qName;
                  break;
                }
              }
            }
          }
        } else if (c.type === "identifier") {
          // 隐式 this 调用优先匹配本类方法
          if (funcMap.has(`${f.className}.${c.name}`)) {
            resolved = `${f.className}.${c.name}`;
          } else if (funcMap.has(c.name)) {
            resolved = c.name;
          }
        }
      } else {
        // 全局函数内部
        if (c.type === "identifier") {
          if (funcMap.has(c.name)) {
            resolved = c.name;
          }
        } else if (c.type === "member") {
          const recvClean = c.receiver.replace(/\s+/g, "");
          for (const [qName, targetFn] of funcMap.entries()) {
            if (targetFn.className && c.name === targetFn.name) {
              if (recvClean === targetFn.className || recvClean === `new${targetFn.className}()`) {
                resolved = qName;
                break;
              }
            }
          }
        }
      }
      c.resolvedTarget = resolved;
    }
  }

  // 构建有向调用图
  const adj = new Map();
  for (const f of functions) {
    const targets = [];
    for (const c of f.calls) {
      if (c.resolvedTarget) {
        targets.push({ target: c.resolvedTarget, call: c });
      }
    }
    adj.set(f.qualifiedName, targets);
  }

  // 深度优先查找包含 startNode 的所有简单环路
  function findCyclesForNode(startNode) {
    const visited = new Set();
    const path = [];
    const cycles = [];

    function dfs(curr) {
      path.push(curr);
      visited.add(curr);

      const edges = adj.get(curr) || [];
      for (const edge of edges) {
        if (edge.target === startNode) {
          cycles.push([...path, startNode]);
        } else if (!visited.has(edge.target)) {
          dfs(edge.target);
        }
      }

      path.pop();
      visited.delete(curr);
    }

    dfs(startNode);
    return cycles;
  }

  // 判定递归函数并收集其环路
  const recursiveFunctions = new Map();
  for (const f of functions) {
    const cycles = findCyclesForNode(f.qualifiedName);
    if (cycles.length > 0) {
      const isSelf = cycles.some(c => c.length === 2 && c[0] === c[1]);
      const isMutual = cycles.some(c => c.length > 2);
      recursiveFunctions.set(f.qualifiedName, {
        isSelf,
        isMutual,
        cycles,
      });
    }
  }

  const results = [];
  const decoratedCallKeys = new Set();

  for (const [qName, info] of recursiveFunctions) {
    const f = funcMap.get(qName);
    if (!f) continue;

    // 1. 函数声明行气泡提示与图标
    let headerMsg = "";
    if (info.isMutual && !info.isSelf) {
      const otherNames = new Set();
      for (const cyc of info.cycles) {
        for (const n of cyc) {
          if (n !== qName) otherNames.add(n);
        }
      }
      headerMsg = `🔄 **递归函数（间接互递归）**：\`${f.name}\` 与 \`${Array.from(otherNames).join(", ")}\` 形成互递归环路`;
    } else if (info.isMutual && info.isSelf) {
      headerMsg = `🔄 **递归函数**：\`${f.name}\`（同时包含自递归与互递归）`;
    } else {
      headerMsg = `🔄 **递归函数**：\`${f.name}\`（自递归）`;
    }

    results.push({
      line: f.headerLine,
      colStart: f.headerCol,
      colEnd: f.headerCol + f.headerLen,
      targetName: f.name,
      kind: "function",
      isMutual: info.isMutual,
      message: headerMsg,
    });

    // 2. 递归调用点气泡提示与图标
    for (const c of f.calls) {
      if (!c.resolvedTarget) continue;

      const onCycle = info.cycles.some(cyc => {
        for (let i = 0; i < cyc.length - 1; i++) {
          if (cyc[i] === qName && cyc[i + 1] === c.resolvedTarget) return true;
        }
        return false;
      });

      if (onCycle) {
        const key = `${c.line}:${c.column}:${c.length}`;
        if (decoratedCallKeys.has(key)) continue;
        decoratedCallKeys.add(key);

        const isSelfCall = (c.resolvedTarget === qName);
        let callMsg = "";
        if (isSelfCall) {
          callMsg = `🔄 **递归调用**：\`${c.fullText}\``;
        } else {
          const matchedCycle = info.cycles.find(cyc => cyc[0] === qName && cyc[1] === c.resolvedTarget);
          const cycleStr = matchedCycle ? matchedCycle.join(" ➔ ") : `${qName} ➔ ${c.resolvedTarget}`;
          callMsg = `🔄 **互递归调用**：${cycleStr}`;
        }

        results.push({
          line: c.line,
          colStart: c.column,
          colEnd: c.column + c.length,
          targetName: c.name,
          kind: "call",
          isMutual: !isSelfCall,
          message: callMsg,
        });
      }
    }
  }

  return results;
}
