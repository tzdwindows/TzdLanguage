// ─── TzdLang VS Code Extension ───────────────────────────────────────────────
// 为 TzdLang 提供语法支持、脚本运行和工具链管理。
// 底层解释器为 TzdTools.exe，支持 --runMainTzd=<file> 等命令行参数。
// ──────────────────────────────────────────────────────────────────────────────

const vscode = require("vscode");
const { LanguageClient, TransportKind } = require("vscode-languageclient/node");
const path = require("path");
const fs = require("fs");
const cp = require("child_process");
const { TzdDebugSession } = require("./debugAdapter");

// ─── 状态 ────────────────────────────────────────────────────────────────────
let outputChannel = null;
let statusBarItem = null;
let runningProcess = null;
let lspClient = null;
let recursionDecorationType = null;
const recursionCache = new Map();
let recursionDebounceTimer = null;

// ─── 配置键名 ────────────────────────────────────────────────────────────────
const CFG_SECTION = "tzdlang";
const CFG_TOOLS_PATH = "toolsPath";
const STORAGE_TOOLS_PATH = "tzdlang.toolsPath";

// ─── 帮助函数 ────────────────────────────────────────────────────────────────

/**
 * 尝试在常见位置查找 TzdTools.exe。
 * 优先级: 配置值 → workspace 相邻目录 → 扩展安装目录。
 */
function findTzdTools(context) {
  // 1. 检查 VS Code 配置
  const config = vscode.workspace.getConfiguration(CFG_SECTION);
  let configured = config.get(CFG_TOOLS_PATH);
  if (configured && fs.existsSync(configured)) {
    return path.resolve(configured);
  }

  // 2. 检查 globalState 中保存的路径
  const saved = context.globalState.get(STORAGE_TOOLS_PATH);
  if (saved && fs.existsSync(saved)) {
    return path.resolve(saved);
  }

  // 3. 遍历 workspace 文件夹，尝试在相邻的 TzdTools 项目中找 Release 构建
  const workspaces = vscode.workspace.workspaceFolders;
  if (workspaces) {
    for (const ws of workspaces) {
      const candidates = [
        path.join(ws.uri.fsPath, "..", "x64", "Release", "TzdTools.exe"),
        path.join(ws.uri.fsPath, "..", "..", "x64", "Release", "TzdTools.exe"),
        path.join(ws.uri.fsPath, "x64", "Release", "TzdTools.exe"),
        path.join(
          ws.uri.fsPath,
          "..",
          "TzdTools",
          "x64",
          "Release",
          "TzdTools.exe",
        ),
      ];
      for (const c of candidates) {
        const resolved = path.resolve(c);
        if (fs.existsSync(resolved)) {
          return resolved;
        }
      }
    }
  }

  // 4. 最后检查 PATH
  const inPath = "TzdTools.exe";
  const pathDirs = process.env.PATH
    ? process.env.PATH.split(path.delimiter)
    : [];
  for (const dir of pathDirs) {
    const candidate = path.join(dir, inPath);
    if (fs.existsSync(candidate)) {
      return candidate;
    }
  }

  return null;
}

/**
 * 返回 TzdTools.exe 所在的目录路径。
 */
function getToolsDir(toolsPath) {
  return path.dirname(toolsPath);
}

/**
 * 查找 stdlib 目录（通常位于 TzdTools.exe 同级或子目录）。
 */
function findStdlib(toolsPath) {
  const dir = getToolsDir(toolsPath);
  const candidates = [
    path.join(dir, "stdlib"),
    path.join(dir, "..", "stdlib"),
    path.join(dir, "..", "..", "stdlib"),
  ];
  for (const c of candidates) {
    const resolved = path.resolve(c);
    if (fs.existsSync(resolved)) {
      return resolved;
    }
  }
  return null;
}

/**
 * 构造 LSP ClientOptions 配置，携带 toolsPath 与 stdlibPath
 */
function getClientOptions(toolsPath, context) {
  let stdlib = toolsPath ? findStdlib(toolsPath) : null;
  if (!stdlib && context) {
    const bundled = context.asAbsolutePath("stdlib");
    if (fs.existsSync(bundled)) stdlib = bundled;
  }
  return {
    documentSelector: [{ scheme: "file", language: "tzdlang" }],
    diagnosticCollectionName: "tzdlang",
    initializationOptions: {
      toolsPath: toolsPath || "",
      stdlibPath: stdlib || "",
      extensionPath: context ? context.extensionPath : "",
    },
  };
}

/**
 * 将文本写入输出通道并滚动到底部。
 */
function appendToOutput(text, isError = false) {
  if (!outputChannel) return;
  const prefix = isError ? "[错误] " : "";
  outputChannel.appendLine(prefix + text);
}

/**
 * 确保输出通道已创建并显示。
 */
function ensureOutputChannel() {
  if (!outputChannel) {
    outputChannel = vscode.window.createOutputChannel("TzdLang");
  }
  outputChannel.show(true);
}

/**
 * 更新状态栏文本。
 */
function updateStatusBar(text) {
  if (!statusBarItem) return;
  statusBarItem.text = text;
  statusBarItem.show();
}

// ─── 命令实现 ────────────────────────────────────────────────────────────────

/**
 * tzdlang.helloWorld: 简单的测试命令。
 */
function helloWorld() {
  vscode.window.showInformationMessage("Hello from TzdLang!");
}

/**
 * tzdlang.runCurrentFile: 运行当前打开的 Tzd 脚本。
 */
async function runCurrentFile(context) {
  const vscode = require("vscode");
  const cp = require("child_process");
  const path = require("path");
  const { TextDecoder } = require("util");

  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    vscode.window.showErrorMessage("请先打开一个 .tzd 文件");
    return;
  }

  const document = editor.document;
  // ⭐ 核心修复1：运行前强制保存文件，防止未保存时读取到空文件或旧内容导致无输出
  if (document.isDirty) {
    await document.save();
  }
  const filePath = document.uri.fsPath;

  const toolsPath = findTzdTools(context);
  if (!toolsPath) {
    const action = "设置路径";
    vscode.window
      .showErrorMessage(
        '找不到 TzdTools.exe，请先通过 "TzdLang: 重新设置 TzdTools 路径" 命令指定位置。',
        action,
      )
      .then((selected) => {
        if (selected === action) {
          vscode.commands.executeCommand("tzdlang.resetToolsPath");
        }
      });
    return;
  }

  // 准备并展示输出通道
  ensureOutputChannel();

  try {
    if (typeof outputChannel !== "undefined" && outputChannel) {
      outputChannel.clear();
    }
  } catch (e) {
    console.warn("清空面板失败，请检查 outputChannel 变量名");
  }

  appendToOutput("─".repeat(60));
  appendToOutput("> 运行: " + filePath);
  appendToOutput("   解释器: " + toolsPath);
  appendToOutput("");

  const args = [];
  const projectDir = path.dirname(filePath);
  args.push("--setProjectDirectory=" + projectDir);

  const stdlibPath = findStdlib(toolsPath);
  if (stdlibPath) {
    args.push("--addLibraryDirectory=" + stdlibPath);
    appendToOutput("   stdlib: " + stdlibPath);
  }

  args.push("--runMainTzd=" + filePath);

  updateStatusBar("$(loading~spin) TzdLang 运行中...");

  const spawned = cp.spawn(toolsPath, args, {
    cwd: getToolsDir(toolsPath),
    windowsHide: true,
    shell: false,
  });

  runningProcess = spawned;

  // ⭐ 核心修复2：智能双模式解码器（优先 UTF-8，失败自动回退 GBK，解决字符截断和乱码丢包）
  const utf8Decoder = new TextDecoder("utf-8", { fatal: true });
  const gbkDecoder = new TextDecoder("gbk", { fatal: false });

  function decodeBuffer(buf) {
    try {
      return utf8Decoder.decode(buf);
    } catch {
      return gbkDecoder.decode(buf);
    }
  }

  // ⭐ 核心修复3：流式行缓冲区，保留跨 chunk 的未结束行和空行，杜绝截断与丢行
  function createStreamHandler(isError = false) {
    let residual = "";
    return {
      onData(data) {
        const text = decodeBuffer(data);
        residual += text;
        const lines = residual.split(/\r?\n/);
        residual = lines.pop(); // 尚未遇到换行符的半行保留到下一次
        for (const line of lines) {
          appendToOutput(line, isError);
        }
      },
      onEnd() {
        if (residual.length > 0) {
          appendToOutput(residual, isError);
          residual = "";
        }
      },
    };
  }

  const stdoutHandler = createStreamHandler(false);
  const stderrHandler = createStreamHandler(true);

  // 收集 stdout
  spawned.stdout.on("data", (data) => {
    stdoutHandler.onData(data);
  });

  // 收集 stderr
  spawned.stderr.on("data", (data) => {
    stderrHandler.onData(data);
  });

  // 进程退出
  spawned.on("close", (code) => {
    runningProcess = null;

    // 刷新末尾缓冲区
    stdoutHandler.onEnd();
    stderrHandler.onEnd();

    const exitMsg =
      code === 0
        ? "OK 执行完成 (退出码: " + code + ")"
        : "ERROR 执行失败 (退出码: " + code + ")";
    appendToOutput("");
    appendToOutput(exitMsg);
    appendToOutput("─".repeat(60));
    updateStatusBar("$(check) TzdLang");
    setTimeout(() => {
      if (typeof statusBarItem !== "undefined" && statusBarItem) {
        statusBarItem.hide();
      }
    }, 5000);
  });

  // 进程错误
  spawned.on("error", (err) => {
    runningProcess = null;
    appendToOutput("无法启动 TzdTools.exe: " + err.message, true);
    appendToOutput("─".repeat(60));
    updateStatusBar("$(error) TzdLang 错误");
    vscode.window.showErrorMessage("启动 TzdTools 失败: " + err.message);
  });
}

/**
 * tzdlang.resetToolsPath: 让用户重新选择 TzdTools.exe 路径。
 */
async function resetToolsPath(context) {
  const options = {
    canSelectFiles: true,
    canSelectMany: false,
    filters: { Executable: ["exe"] },
    openLabel: "选择 TzdTools.exe",
    title: "请选择 TzdTools.exe 的位置",
  };

  const result = await vscode.window.showOpenDialog(options);

  if (result && result.length > 0) {
    const selectedPath = result[0].fsPath;

    // 统一键名保存
    const config = vscode.workspace.getConfiguration(CFG_SECTION);
    await config.update(
      CFG_TOOLS_PATH,
      selectedPath,
      vscode.ConfigurationTarget.Global,
    );
    await context.globalState.update(STORAGE_TOOLS_PATH, selectedPath);

    // 更新状态栏
    if (statusBarItem) {
      statusBarItem.text = "$(check) TzdLang";
      statusBarItem.tooltip = "TzdTools: " + selectedPath;
    }

    vscode.window.showInformationMessage(
      "TzdTools 路径已更新: " + selectedPath,
    );

    // 重启 LSP 客户端以加载新路径的 stdlib
    if (lspClient) {
      await lspClient.stop();
      lspClient = null;
    }

    const serverModule = context.asAbsolutePath(
      path.join("server", "server.js"),
    );
    const serverOptions = {
      run: { module: serverModule, transport: TransportKind.ipc },
      debug: {
        module: serverModule,
        transport: TransportKind.ipc,
        options: { execArgv: ["--nolazy", "--inspect=6009"] },
      },
    };
    const clientOptions = getClientOptions(selectedPath, context);

    lspClient = new LanguageClient(
      "tzdlang-lsp",
      "TzdLang Language Server",
      serverOptions,
      clientOptions,
    );
    lspClient
      .start()
      .then(() => {
        console.log("TzdLang LSP restarted with new toolsPath");
      })
      .catch((err) => {
        console.warn("TzdLang LSP restart failed:", err.message);
      });
  } else {
    vscode.window.showInformationMessage("未选择路径，设置已取消。");
  }
}

// ─── 启动 LSP 语言服务器 ──────────────────────────────────────────────
async function startLspClient(context) {
  const serverModule = context.asAbsolutePath(path.join("server", "server.js"));

  // 先拿工具路径（没有就弹框让用户选）
  let toolsPath = context.globalState.get("tzdToolsPath") || "";
  if (!toolsPath || !fs.existsSync(toolsPath)) {
    const result = await vscode.window.showOpenDialog({
      canSelectFiles: true,
      canSelectMany: false,
      filters: { Executable: ["exe"] },
      openLabel: "选择 TzdTools.exe",
      title: "TzdLang: 请选择 TzdTools.exe 的位置",
    });
    if (result && result.length > 0) {
      toolsPath = result[0].fsPath;
      await context.globalState.update("tzdToolsPath", toolsPath);
    }
  }

  const serverOptions = {
    run: { module: serverModule, transport: TransportKind.ipc },
    debug: {
      module: serverModule,
      transport: TransportKind.ipc,
      options: { execArgv: ["--nolazy", "--inspect=6009"] },
    },
  };

  const clientOptions = getClientOptions(toolsPath, context);

  lspClient = new LanguageClient(
    "tzdlang-lsp",
    "TzdLang Language Server",
    serverOptions,
    clientOptions,
  );

  lspClient
    .start()
    .then(() => {
      console.log("TzdLang LSP server ready");
    })
    .catch((err) => {
      console.warn("TzdLang LSP server failed to start:", err.message);
    });
}

// ─── DAP 调试器支持 ───────────────────────────────────────────────────────────

class TzdConfigurationProvider {
  constructor(context) {
    this.context = context;
  }

  resolveDebugConfiguration(folder, config, token) {
    // 若没有配置 launch.json 则自动生成默认配置
    if (!config.type && !config.request && !config.name) {
      const editor = vscode.window.activeTextEditor;
      if (editor && editor.document.languageId === "tzdlang") {
        config.type = "tzdlang";
        config.name = "调试当前 Tzd 脚本";
        config.request = "launch";
        config.program = "${file}";
        config.stopOnEntry = false;
      }
    }

    if (config.request === "launch") {
      if (!config.program || config.program === "${file}") {
        const editor = vscode.window.activeTextEditor;
        if (editor && editor.document.languageId === "tzdlang") {
          config.program = editor.document.fileName;
        } else {
          vscode.window.showErrorMessage("请在编辑器中打开一个 .tzd 脚本文件再开始调试。");
          return undefined;
        }
      }

      if (!config.toolsPath) {
        config.toolsPath = findTzdTools(this.context);
      }

      if (!config.cwd) {
        config.cwd = folder ? folder.uri.fsPath : path.dirname(config.program);
      }

      if (!config.debugPort) {
        config.debugPort = 54321;
      }

      const extConfig = vscode.workspace.getConfiguration(CFG_SECTION);
      if (config.jit === undefined) {
        config.jit = extConfig.get("enableJit", true);
      }
      if (config.optLevel === undefined) {
        config.optLevel = extConfig.get("optLevel", 3);
      }
      if (config.enableAstInlining === undefined) {
        config.enableAstInlining = extConfig.get("enableAstInlining", true);
      }

      if (!config.args) {
        config.args = config.jit ? ["--jit", "--jit-debug", `-O${config.optLevel}`] : ["--no-jit"];
      }
    }

    return config;
  }
}

class TzdDebugAdapterDescriptorFactory {
  constructor(context) {
    this.context = context;
  }

  createDebugAdapterDescriptor(session) {
    const toolsPath = findTzdTools(this.context);
    return new vscode.DebugAdapterInlineImplementation(
      new TzdDebugSession(this.context, { toolsPath, ...session.configuration })
    );
  }
}

// ─── 递归调用检测与 Gutter 标记 (类似 IntelliJ IDEA 循环箭头) ─────────────────────

/**
 * 快速纯文本行级扫描器：当 LSP 服务启动中或未就绪时，作为零延迟即时回退
 */
function scanRecursionsFast(text) {
  if (!text) return [];
  const lines = text.split(/\r?\n/);
  const functions = [];
  let currentClass = null;
  let currentFunc = null;
  let braceDepth = 0;

  for (let i = 0; i < lines.length; i++) {
    const raw = lines[i];
    const line = raw.replace(/\/\/.*$/, "");

    const classMatch = line.match(/\bclass\s+([A-Za-z0-9_]+)/);
    if (classMatch && !currentFunc) {
      currentClass = classMatch[1];
    }

    const funcMatch = line.match(/(?:(?:public|private|protected|static)\s+)*fun\s+([A-Za-z0-9_]+)\s*\(/);
    if (funcMatch && !currentFunc) {
      const fnName = funcMatch[1];
      const col = raw.indexOf(fnName, raw.indexOf("fun"));
      currentFunc = {
        name: fnName,
        qualifiedName: currentClass ? `${currentClass}.${fnName}` : fnName,
        className: currentClass,
        headerLine: i,
        headerCol: col >= 0 ? col : 0,
        headerLen: fnName.length,
        startLine: i,
        endLine: i,
        calls: [],
      };
      braceDepth = 0;
    }

    for (let c = 0; c < line.length; c++) {
      if (line[c] === "{") braceDepth++;
      else if (line[c] === "}") {
        braceDepth--;
        if (currentFunc && braceDepth <= 0) {
          currentFunc.endLine = i;
          functions.push(currentFunc);
          currentFunc = null;
        }
      }
    }

    if (currentFunc) {
      const callRegex = /(?:(\bthis|\bnew\s+[A-Za-z0-9_]+\s*\([^)]*\)|[A-Za-z0-9_]+)\.)?\b([A-Za-z0-9_]+)\s*\(/g;
      let m;
      while ((m = callRegex.exec(line)) !== null) {
        const recv = m[1] || "";
        const calledName = m[2];
        if (["if", "while", "for", "switch", "catch", "fun", "return"].includes(calledName)) continue;
        currentFunc.calls.push({
          receiver: recv,
          name: calledName,
          line: i,
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

  const funcMap = new Map();
  for (const f of functions) funcMap.set(f.qualifiedName, f);

  for (const f of functions) {
    for (const c of f.calls) {
      let resolved = null;
      if (f.className) {
        const recvClean = c.receiver.replace(/\s+/g, "");
        if (recvClean === "this" || recvClean === f.className || recvClean === `new${f.className}()`) {
          if (funcMap.has(`${f.className}.${c.name}`)) resolved = `${f.className}.${c.name}`;
        } else if (!c.receiver) {
          if (funcMap.has(`${f.className}.${c.name}`)) resolved = `${f.className}.${c.name}`;
          else if (funcMap.has(c.name)) resolved = c.name;
        }
      } else {
        if (!c.receiver && funcMap.has(c.name)) resolved = c.name;
      }
      c.resolvedTarget = resolved;
    }
  }

  const adj = new Map();
  for (const f of functions) {
    adj.set(f.qualifiedName, f.calls.filter(c => c.resolvedTarget).map(c => ({ target: c.resolvedTarget, call: c })));
  }

  function findCycles(startNode) {
    const visited = new Set();
    const path = [];
    const cycles = [];
    function dfs(curr) {
      path.push(curr);
      visited.add(curr);
      const edges = adj.get(curr) || [];
      for (const e of edges) {
        if (e.target === startNode) cycles.push([...path, startNode]);
        else if (!visited.has(e.target)) dfs(e.target);
      }
      path.pop();
      visited.delete(curr);
    }
    dfs(startNode);
    return cycles;
  }

  const results = [];
  const decoratedCalls = new Set();

  for (const f of functions) {
    const cycles = findCycles(f.qualifiedName);
    if (cycles.length === 0) continue;
    const isSelf = cycles.some(c => c.length === 2 && c[0] === c[1]);
    const isMutual = cycles.some(c => c.length > 2);

    let headerMsg = isMutual && !isSelf
      ? `🔄 **递归函数（间接互递归）**：\`${f.name}\``
      : `🔄 **递归函数**：\`${f.name}\`（自递归）`;

    results.push({
      line: f.headerLine,
      colStart: f.headerCol,
      colEnd: f.headerCol + f.headerLen,
      targetName: f.name,
      kind: "function",
      isMutual,
      message: headerMsg,
    });

    for (const c of f.calls) {
      if (!c.resolvedTarget) continue;
      const onCycle = cycles.some(cyc => {
        for (let j = 0; j < cyc.length - 1; j++) {
          if (cyc[j] === f.qualifiedName && cyc[j + 1] === c.resolvedTarget) return true;
        }
        return false;
      });
      if (onCycle) {
        const key = `${c.line}:${c.column}:${c.length}`;
        if (decoratedCalls.has(key)) continue;
        decoratedCalls.add(key);
        results.push({
          line: c.line,
          colStart: c.column,
          colEnd: c.column + c.length,
          targetName: c.name,
          kind: "call",
          isMutual: c.resolvedTarget !== f.qualifiedName,
          message: c.resolvedTarget === f.qualifiedName ? `🔄 **递归调用**：\`${c.fullText}\`` : `🔄 **互递归调用**：\`${c.fullText}\``,
        });
      }
    }
  }

  return results;
}

/**
 * 将递归标记应用到指定编辑器，渲染左侧 Gutter 循环箭头
 */
function applyRecursionDecorations(editor, items) {
  if (!editor || !editor.document || !items || !recursionDecorationType) return;
  const decorations = items.map((item) => {
    const line = Math.max(0, Math.min(item.line, editor.document.lineCount - 1));
    const lineText = editor.document.lineAt(line).text;
    const colStart = Math.max(0, Math.min(item.colStart, lineText.length));
    const colEnd = Math.max(colStart, Math.min(item.colEnd, lineText.length));
    const range = new vscode.Range(line, colStart, line, colEnd);
    const hoverMessage = new vscode.MarkdownString(item.message);
    hoverMessage.isTrusted = true;
    return {
      range,
      hoverMessage,
    };
  });
  editor.setDecorations(recursionDecorationType, decorations);
}

/**
 * 更新 URI 缓存并在所有当前可见的匹配编辑器中刷新装饰
 */
function updateRecursionsForUri(uri, items) {
  recursionCache.set(uri, items);
  for (const editor of vscode.window.visibleTextEditors) {
    if (editor.document.uri.toString() === uri) {
      applyRecursionDecorations(editor, items);
    }
  }
}

/**
 * 为单个编辑器触发递归检测请求与渲染
 */
function requestRecursionsForEditor(editor) {
  if (!editor || !editor.document) return;
  const langId = editor.document.languageId;
  const fileName = editor.document.fileName;
  if (langId !== "tzdlang" && !fileName.endsWith(".tzd") && !fileName.endsWith(".tzdlang")) return;

  const uri = editor.document.uri.toString();
  if (lspClient && lspClient.isRunning && lspClient.isRunning()) {
    lspClient
      .sendRequest("tzdlang/getRecursions", { uri })
      .then((items) => {
        if (Array.isArray(items)) {
          updateRecursionsForUri(uri, items);
        }
      })
      .catch(() => {});
  } else if (!recursionCache.has(uri)) {
    const items = scanRecursionsFast(editor.document.getText());
    updateRecursionsForUri(uri, items);
  } else {
    applyRecursionDecorations(editor, recursionCache.get(uri));
  }
}

// ─── 激活与停用 ──────────────────────────────────────────────────────────────

/**
 * 扩展激活入口。
 */
function activate(context) {
  // 创建递归调用 Gutter 装饰类型（类似 IntelliJ IDEA 循环箭头）
  recursionDecorationType = vscode.window.createTextEditorDecorationType({
    dark: {
      gutterIconPath: context.asAbsolutePath(path.join("images", "recursion-dark.svg")),
    },
    light: {
      gutterIconPath: context.asAbsolutePath(path.join("images", "recursion-light.svg")),
    },
    gutterIconSize: "contain",
  });
  context.subscriptions.push(recursionDecorationType);

  // 监听活动编辑器与可见编辑器，应用递归标记
  context.subscriptions.push(
    vscode.window.onDidChangeActiveTextEditor((editor) => {
      if (editor) requestRecursionsForEditor(editor);
    }),
    vscode.window.onDidChangeVisibleTextEditors((editors) => {
      for (const editor of editors) {
        requestRecursionsForEditor(editor);
      }
    }),
    vscode.workspace.onDidChangeTextDocument((event) => {
      const activeEditor = vscode.window.activeTextEditor;
      if (activeEditor && activeEditor.document === event.document) {
        if (recursionDebounceTimer) clearTimeout(recursionDebounceTimer);
        recursionDebounceTimer = setTimeout(() => {
          requestRecursionsForEditor(activeEditor);
        }, 250);
      }
    }),
    vscode.workspace.onDidCloseTextDocument((doc) => {
      recursionCache.delete(doc.uri.toString());
    }),
  );

  // 立即为当前已打开的编辑器初始化递归分析
  if (vscode.window.activeTextEditor) {
    requestRecursionsForEditor(vscode.window.activeTextEditor);
  }

  // 创建状态栏
  statusBarItem = vscode.window.createStatusBarItem(
    vscode.StatusBarAlignment.Left,
    100,
  );
  statusBarItem.text = "$(circuit-board) TzdLang";
  statusBarItem.tooltip = "TzdLang 扩展已激活";
  statusBarItem.command = "tzdlang.runCurrentFile";
  context.subscriptions.push(statusBarItem);

  // 检测 TzdTools 是否存在，更新状态栏
  const toolsPath = findTzdTools(context);
  if (toolsPath) {
    statusBarItem.text = "$(check) TzdLang";
    statusBarItem.tooltip = "TzdTools: " + toolsPath;
  } else {
    statusBarItem.text = "$(warning) TzdLang";
    statusBarItem.tooltip = "未配置 TzdTools 路径";
  }
  statusBarItem.show();

  // 注册 DAP 调试适配器工厂与配置提供者 (同时支持 tzdlang 与 tzd 类型)
  const configProvider = new TzdConfigurationProvider(context);
  const debugFactory = new TzdDebugAdapterDescriptorFactory(context);

  context.subscriptions.push(
    vscode.debug.registerDebugConfigurationProvider("tzdlang", configProvider),
    vscode.debug.registerDebugConfigurationProvider("tzd", configProvider),
    vscode.debug.registerDebugAdapterDescriptorFactory("tzdlang", debugFactory),
    vscode.debug.registerDebugAdapterDescriptorFactory("tzd", debugFactory)
  );

  // 注册命令
  const disposables = [
    vscode.commands.registerCommand("tzdlang.helloWorld", helloWorld),

    vscode.commands.registerCommand("tzdlang.runCurrentFile", async () => {
      if (runningProcess) {
        vscode.window.showWarningMessage("已有脚本正在运行，请等待完成。");
        return;
      }
      await runCurrentFile(context);
    }),


    vscode.commands.registerCommand("tzdlang.debugCurrentFile", async () => {
      const editor = vscode.window.activeTextEditor;
      if (!editor) {
        vscode.window.showErrorMessage("请先在编辑器中打开一个 .tzd 脚本文件");
        return;
      }
      const doc = editor.document;
      if (doc.languageId !== "tzdlang") {
        vscode.window.showErrorMessage("当前打开的文件不是 .tzd 脚本");
        return;
      }
      await doc.save();
      const extConfig = vscode.workspace.getConfiguration(CFG_SECTION);
      const optLevel = extConfig.get("optLevel", 3);
      const jitEnabled = extConfig.get("enableJit", true);

      vscode.debug.startDebugging(undefined, {
        type: "tzdlang",
        name: `调试 ${path.basename(doc.fileName)}`,
        request: "launch",
        program: doc.fileName,
        jit: jitEnabled,
        optLevel: optLevel,
        args: jitEnabled ? ["--jit", "--jit-debug", `-O${optLevel}`] : ["--no-jit"],
        stopOnEntry: false,
      });
    }),

    vscode.commands.registerCommand("tzdlang.showJitStatus", async () => {
      const session = vscode.debug.activeDebugSession;
      if (!session || (session.type !== "tzdlang" && session.type !== "tzd")) {
        vscode.window.showInformationMessage("请先启动 TzdLang 调试会话以查看 JIT 引擎状态。");
        return;
      }
      try {
        const resp = await session.customRequest("evaluate", { expression: ":jit status" });
        if (!outputChannel) outputChannel = vscode.window.createOutputChannel("TzdLang");
        outputChannel.show(true);
        outputChannel.appendLine("\n" + (resp.result || ""));
      } catch (err) {
        vscode.window.showErrorMessage("获取 JIT 状态失败: " + err.message);
      }
    }),

    vscode.commands.registerCommand("tzdlang.listJitFunctions", async () => {
      const session = vscode.debug.activeDebugSession;
      if (!session || (session.type !== "tzdlang" && session.type !== "tzd")) {
        vscode.window.showInformationMessage("请先启动 TzdLang 调试会话以列出 JIT 函数。");
        return;
      }
      try {
        const resp = await session.customRequest("evaluate", { expression: ":jit list" });
        if (!outputChannel) outputChannel = vscode.window.createOutputChannel("TzdLang");
        outputChannel.show(true);
        outputChannel.appendLine("\n" + (resp.result || ""));
      } catch (err) {
        vscode.window.showErrorMessage("获取 JIT 函数列表失败: " + err.message);
      }
    }),

    vscode.commands.registerCommand("tzdlang.dumpJitIR", async () => {
      const session = vscode.debug.activeDebugSession;
      if (!session || (session.type !== "tzdlang" && session.type !== "tzd")) {
        vscode.window.showInformationMessage("请先启动 TzdLang 调试会话以导出 JIT LLVM IR。");
        return;
      }
      const funcName = await vscode.window.showInputBox({
        prompt: "输入要导出 LLVM IR 的函数名或符号名 (如 noop, main, fast_calc)",
        placeHolder: "noop",
      });
      if (!funcName) return;
      try {
        const resp = await session.customRequest("evaluate", { expression: `:jit ir ${funcName}` });
        const doc = await vscode.workspace.openTextDocument({
          content: resp.result || "; Empty IR",
          language: "llvm",
        });
        await vscode.window.showTextDocument(doc, { preview: true });
      } catch (err) {
        vscode.window.showErrorMessage("导出 LLVM IR 失败: " + err.message);
      }
    }),

    vscode.commands.registerCommand("tzdlang.resetToolsPath", () => {
      resetToolsPath(context);
    }),
  ];

  for (const d of disposables) {
    context.subscriptions.push(d);
  }

  // 输出通道在停用时清理
  context.subscriptions.push({
    dispose: () => {
      if (outputChannel) {
        outputChannel.dispose();
        outputChannel = null;
      }
    },
  });

  // ─── 启动 LSP 语言服务器 ──────────────────────────────────────────────
  async function startLspClient() {
    const serverModule = context.asAbsolutePath(
      path.join("server", "server.js"),
    );

    // 统一用 findTzdTools 读取路径（它内部已经检查了配置和 globalState）
    let resolvedToolsPath = findTzdTools(context) || "";

    if (!resolvedToolsPath || !fs.existsSync(resolvedToolsPath)) {
      const result = await vscode.window.showOpenDialog({
        canSelectFiles: true,
        canSelectMany: false,
        filters: { Executable: ["exe"] },
        openLabel: "选择 TzdTools.exe",
        title: "TzdLang: 请选择 TzdTools.exe 的位置",
      });
      if (result && result.length > 0) {
        resolvedToolsPath = result[0].fsPath;
        // 保存时用统一的键名
        const config = vscode.workspace.getConfiguration(CFG_SECTION);
        await config.update(
          CFG_TOOLS_PATH,
          resolvedToolsPath,
          vscode.ConfigurationTarget.Global,
        );
        await context.globalState.update(STORAGE_TOOLS_PATH, resolvedToolsPath);
        statusBarItem.text = "$(check) TzdLang";
        statusBarItem.tooltip = "TzdTools: " + resolvedToolsPath;
      } else {
        vscode.window.showWarningMessage(
          "未选择 TzdTools.exe，部分功能不可用。",
        );
      }
    }

    // 输出当前 exe 位置
    if (resolvedToolsPath) {
      vscode.window.showInformationMessage(
        "TzdTools 路径: " + resolvedToolsPath,
      );
    }

    const serverOptions = {
      run: { module: serverModule, transport: TransportKind.ipc },
      debug: {
        module: serverModule,
        transport: TransportKind.ipc,
        options: { execArgv: ["--nolazy", "--inspect=6009"] },
      },
    };

    const clientOptions = getClientOptions(resolvedToolsPath, context);

    lspClient = new LanguageClient(
      "tzdlang-lsp",
      "TzdLang Language Server",
      serverOptions,
      clientOptions,
    );

    lspClient
      .start()
      .then(() => {
        console.log("TzdLang LSP server ready");
        // ⭐ 监听 LSP 服务端发送的递归检测通知
        lspClient.onNotification("tzdlang/recursions", ({ uri, items }) => {
          updateRecursionsForUri(uri, items);
        });

        // LSP 启动后，全量刷新当前所有可见编辑器的递归分析结果
        for (const editor of vscode.window.visibleTextEditors) {
          requestRecursionsForEditor(editor);
        }
      })
      .catch((err) => {
        console.warn("TzdLang LSP server failed to start:", err.message);
      });
  }

  startLspClient();
  console.log("TzdLang 扩展已激活");
}

/**
 * 扩展停用入口。
 */
function deactivate() {
  if (recursionDecorationType) {
    recursionDecorationType.dispose();
    recursionDecorationType = null;
  }
  recursionCache.clear();
  if (outputChannel) {
    outputChannel.dispose();
    outputChannel = null;
  }
  if (lspClient) {
    try {
      lspClient.stop();
    } catch (_) {
      /* 忽略 */
    }
    lspClient = null;
  }
  if (runningProcess) {
    try {
      runningProcess.kill();
    } catch (_) {
      /* 忽略 */
    }
    runningProcess = null;
  }
  if (statusBarItem) {
    statusBarItem.dispose();
    statusBarItem = null;
  }
}

module.exports = { activate, deactivate };
