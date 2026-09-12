// ─── TzdLang VS Code Extension ───────────────────────────────────────────────
// 为 TzdLang 提供语法支持、脚本运行和工具链管理。
// 底层解释器为 TzdTools.exe，支持 --runMainTzd=<file> 等命令行参数。
// ──────────────────────────────────────────────────────────────────────────────

const vscode = require("vscode");
const { LanguageClient, TransportKind } = require("vscode-languageclient/node");
const path = require("path");
const fs = require("fs");
const cp = require("child_process");

// ─── 状态 ────────────────────────────────────────────────────────────────────
let outputChannel = null;
let statusBarItem = null;
let runningProcess = null;
let lspClient = null;

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
function runCurrentFile(context) {
  const vscode = require("vscode");
  const cp = require("child_process");
  const path = require("path");
  const { TextDecoder } = require("util"); // ⭐ 引入原生文本解码器

  const editor = vscode.window.activeTextEditor;
  if (!editor) {
    vscode.window.showErrorMessage("请先打开一个 .tzd 文件");
    return;
  }

  const document = editor.document;
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

  // 准备输出通道
  ensureOutputChannel();

  // ⭐ 强制清空面板：确保你的全局变量叫 outputChannel
  // 如果你的全局输出频道对象不叫这个名字，请把它改成对应的变量名
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

  // ⭐ 终极 GBK 流式解码器（stream: true 保证了汉字不会被截断断层）
  const stdoutDecoder = new TextDecoder("gbk", { fatal: false });
  const stderrDecoder = new TextDecoder("gbk", { fatal: false });

  // 收集 stdout
  spawned.stdout.on("data", (data) => {
    const text = stdoutDecoder
      .decode(data, { stream: true })
      .replace(/\r\n/g, "\n")
      .replace(/\r/g, "\n");
    const lines = text.split("\n");
    for (const line of lines) {
      if (line.trim()) appendToOutput(line);
    }
  });

  // 收集 stderr
  spawned.stderr.on("data", (data) => {
    const text = stderrDecoder
      .decode(data, { stream: true })
      .replace(/\r\n/g, "\n")
      .replace(/\r/g, "\n");
    const lines = text.split("\n");
    for (const line of lines) {
      if (line.trim()) appendToOutput(line, true);
    }
  });

  // 进程退出
  spawned.on("close", (code) => {
    runningProcess = null;

    // 输出流缓冲区最后残留的一点点数据
    const outEnd = stdoutDecoder
      .decode()
      .replace(/\r\n/g, "\n")
      .replace(/\r/g, "\n");
    if (outEnd.trim()) appendToOutput(outEnd);

    const errEnd = stderrDecoder
      .decode()
      .replace(/\r\n/g, "\n")
      .replace(/\r/g, "\n");
    if (errEnd.trim()) appendToOutput(errEnd, true);

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
    const clientOptions = {
      documentSelector: [{ scheme: "file", language: "tzdlang" }],
      diagnosticCollectionName: "tzdlang",
      initializationOptions: { toolsPath: selectedPath },
    };

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

  const clientOptions = {
    documentSelector: [{ scheme: "file", language: "tzdlang" }],
    diagnosticCollectionName: "tzdlang",
    initializationOptions: {
      toolsPath: toolsPath,
    },
  };

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

// ─── 激活与停用 ──────────────────────────────────────────────────────────────

/**
 * 扩展激活入口。
 */
function activate(context) {
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

  // 注册命令
  const disposables = [
    vscode.commands.registerCommand("tzdlang.helloWorld", helloWorld),

    vscode.commands.registerCommand("tzdlang.runCurrentFile", () => {
      if (runningProcess) {
        vscode.window.showWarningMessage("已有脚本正在运行，请等待完成。");
        return;
      }
      runCurrentFile(context);
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

    const clientOptions = {
      documentSelector: [{ scheme: "file", language: "tzdlang" }],
      diagnosticCollectionName: "tzdlang",
      initializationOptions: {
        toolsPath: resolvedToolsPath,
      },
    };

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

  startLspClient();
  console.log("TzdLang 扩展已激活");
}

/**
 * 扩展停用入口。
 */
function deactivate() {
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
