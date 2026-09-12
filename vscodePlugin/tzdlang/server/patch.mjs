import fs from "fs";
import path from "path";

const filePath = "C:/Users/tzdwindows 7/source/repos/TzdTools/vscodePlugin/tzdlang/server/server.js";
let content = fs.readFileSync(filePath, "utf-8");

// 1. Add __dirname and stdlib roots
const importFsLine = 'import fs from "fs";';
const newImports = `import fs from "fs";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const stdlibRoots = [
  path.resolve(__dirname, "..", "..", "stdlib"),
  path.resolve(__dirname, "..", "..", "..", "stdlib"),
  path.resolve(__dirname, "..", "..", "..", "x64", "Release", "stdlib"),
];`;

content = content.replace(importFsLine, newImports);

// 2. After stdlib loading, add class cache
const stdlibEnd = `  }
}
// ─── 文档变更 → 诊断更新 ────────────────────────────────────────────────`;

const cacheInsert = `
}
// ─── 跨文件类定义缓存 ──────────────────────────────────────────────────

const classCache = new Map();

function resolveImportPath(importPath, docUri) {
  let docFsPath = docUri;
  if (docFsPath.startsWith("file:///")) {
    docFsPath = decodeURIComponent(docFsPath.slice(8).replace(/\\//g, "\\\\"));
  }
  const docDir = docFsPath.substring(0, docFsPath.lastIndexOf("\\\\"));
  const relPath = path.resolve(docDir, importPath.replace(/\\//g, "\\\\"));
  if (fs.existsSync(relPath)) return relPath;
  for (const root of stdlibRoots) {
    const cand = path.resolve(root, importPath.replace(/\\//g, "\\\\"));
    if (fs.existsSync(cand)) return cand;
  }
  return null;
}

function extractClassDefs(text, filePath) {
  const { tree } = parseTzd(text);
  if (!tree) return [];
  const defs = [];
  function walk(node) {
    if (!node || node.ruleIndex === undefined) return;
    if (node.ruleIndex === 8) {
      try {
        const qn = node.qualifiedName(0);
        if (!qn) { if (node.children) for (const c of node.children) if (typeof c === "object") walk(c); return; }
        const className = qn.getText();
        let parentName = null;
        const children = node.children || [];
        for (let i = 0; i < children.length; i++) {
          const c2 = children[i];
          if (c2 && c2.getText && (c2.getText() === "extends" || c2.getText() === ":")) {
            if (i + 1 < children.length && children[i+1].getText) {
              parentName = children[i+1].getText();
            }
          }
        }
        const members = [];
        function walkBody(bn) {
          if (!bn || bn.ruleIndex === undefined) return;
          const ri = bn.ruleIndex;
          const cn = bn.constructor.name;
          if (ri === 11) {
            try {
              const idNode2 = bn.IDENTIFIER ? bn.IDENTIFIER() : null;
              if (idNode2) {
                const name2 = idNode2.getText ? idNode2.getText() : "";
                if (cn === "MethodDeclContext" || cn === "MethodStaticDeclContext") {
                  members.push({name: name2, kind: "method", type: null});
                } else if (cn === "FieldVarDeclContext" || cn === "FieldLetDeclContext") {
                  const typeNode = bn.typeType ? bn.typeType() : null;
                  const typeName2 = typeNode ? typeNode.getText() : null;
                  members.push({name: name2, kind: "field", type: typeName2});
                } else if (cn === "ConstructorDeclContext") {
                  members.push({name: name2, kind: "constructor", type: null});
                }
              }
            } catch(e) {}
          }
          if (bn.children) for (const cc of bn.children) if (typeof cc === "object") walkBody(cc);
        }
        walkBody(node);
        defs.push({ className, parentName, file: filePath, members });
      } catch(e) {}
    } else if (node.children) {
      for (const c3 of node.children) if (typeof c3 === "object") walk(c3);
    }
  }
  walk(tree);
  return defs;
}

function loadImportClasses(docText, docUri) {
  const lines2 = docText.split("\\n");
  for (const line2 of lines2) {
    const m2 = line2.match(/import\\s+"(.+?)"\\s*;/);
    if (!m2) continue;
    const importPath = m2[1];
    const resolved = resolveImportPath(importPath, docUri);
    if (!resolved || classCache.has(resolved)) continue;
    try {
      const fileContent = fs.readFileSync(resolved, "utf-8");
      const defs = extractClassDefs(fileContent, resolved);
      for (const def of defs) {
        classCache.set(def.className, def);
      }
      classCache.set(resolved, { _file: true });
    } catch(e) { /* skip */ }
  }
}

function getMembersIncludingInherited(className) {
  const def = classCache.get(className);
  if (!def) return [];
  const result = [...def.members];
  if (def.parentName) {
    const parentMembers = getMembersIncludingInherited(def.parentName);
    const ownNames = new Set(result.map(m => m.name));
    for (const pm of parentMembers) {
      if (!ownNames.has(pm.name)) result.push(pm);
    }
  }
  return result;
}

function resolveVarType(varName, text) {
  const escaped = varName.replace(/[.*+?^\${}()|[\\]\\\\]/g, "\\\\$&");
  const re = new RegExp("^(?:var\\\\s+)?(\\\\w+)\\\\s+" + escaped + "\\\\s*=(?!=)", "m");
  const m3 = text.match(re);
  if (m3) return m3[1];
  return null;
}

// ─── 文档变更 → 诊断更新 ────────────────────────────────────────────────`;

content = content.replace(stdlibEnd, cacheInsert);

// 3. Update buildCompletions: add member completion for .
const oldCompletionsEnd = `  if (!currentWord) return items;
  return items.filter((item) =>
    item.label.toLowerCase().startsWith(currentWord),
  );
}`;

const newCompletionsEnd = `
  // 成员访问补全：当触发字符为 . 时
  const beforeChar = col > 0 ? currentLine[col - 1] : "";
  if (beforeChar === ".") {
    let dotStart = col - 2;
    while (dotStart >= 0 && /[a-zA-Z_0-9]/.test(currentLine[dotStart])) dotStart--;
    const varName = currentLine.substring(dotStart + 1, col - 1);
    if (varName) {
      const varType = resolveVarType(varName, text);
      if (varType) {
        const classDef = classCache.get(varType);
        if (classDef) {
          const allMembers = getMembersIncludingInherited(varType);
          for (const member of allMembers) {
            let kind = CompletionItemKind.Property;
            if (member.kind === "method") kind = CompletionItemKind.Method;
            if (member.kind === "field") kind = CompletionItemKind.Field;
            const detail = member.type ? (member.kind + ": " + member.type) : member.kind;
            items.push({ label: member.name, kind, detail, data: "member" });
          }
          if (!currentWord) return items;
          return items.filter(it => it.label.toLowerCase().startsWith(currentWord));
        }
      }
    }
  }

  if (!currentWord) return items;
  return items.filter((item) =>
    item.label.toLowerCase().startsWith(currentWord),
  );
`;

content = content.replace(oldCompletionsEnd, newCompletionsEnd);

// 4. Update semanticCheck to load import classes
const oldSemanticLine = '  // 检查 `TypeName varName = new OtherType(...)` 类型不匹配';
const newSemanticBlock = `  // 加载 import 文件到类缓存
  loadImportClasses(text, docUri);

  // 检查 \`TypeName varName = new OtherType(...)\` 类型不匹配`;

content = content.replace(oldSemanticLine, newSemanticBlock);

fs.writeFileSync(filePath, content, "utf-8");
console.log("PATCH COMPLETE");