// ─── TzdLang Debug Adapter Protocol (DAP) Implementation ─────────────────────
// 完整的 Debug Adapter 协议实现，支持行断点、单步跳过/进入/跳出、调用栈、变量监视与表达式求值。
// ──────────────────────────────────────────────────────────────────────────────

const net = require("net");
const cp = require("child_process");
const path = require("path");
const fs = require("fs");
const { EventEmitter } = require("events");
const { TextDecoder } = require("util");

class TzdDebugSession {
  constructor(context, config) {
    this.context = context;
    this.config = config || {};

    this._onDidSendMessage = new EventEmitter();
    this.onDidSendMessage = (listener) => {
      this._onDidSendMessage.on("message", listener);
      return {
        dispose: () => this._onDidSendMessage.removeListener("message", listener),
      };
    };

    this.seq = 1;
    this.socket = null;
    this.childProcess = null;
    this.stopOnEntry = false;
    this.configured = false;
    this.isSuspended = false;
    this.hasReportedEntry = false;
    this.currentFile = "";
    this.currentLine = 1;
    this.currentFrame = "";

    // 命令请求队列 (用于处理 :stack, :locals 等需要等待响应的指令)
    this.pendingCommands = [];
    this.recvBuffer = "";

    // 缓存待同步的断点映射: Map<normalizedFilePath, Array<{ line: number }>>
    this.breakpointsMap = new Map();
  }

  // ─── DAP 消息基础结构 ──────────────────────────────────────────────────────

  send(msg) {
    msg.seq = this.seq++;
    this._onDidSendMessage.emit("message", msg);
  }

  sendResponse(request, body = {}, success = true, message = "") {
    this.send({
      type: "response",
      request_seq: request.seq,
      command: request.command,
      success: success,
      message: message,
      body: body,
    });
  }

  sendEvent(event, body = {}) {
    this.send({
      type: "event",
      event: event,
      body: body,
    });
  }

  sendOutput(text, category = "stdout") {
    this.sendEvent("output", {
      category: category,
      output: text.endsWith("\n") ? text : text + "\n",
    });
  }

  // ─── 消息入口 ──────────────────────────────────────────────────────────────

  async handleMessage(message) {
    if (message.type !== "request") return;

    const command = message.command;
    const args = message.arguments || {};

    try {
      switch (command) {
        case "initialize":
          this.handleInitialize(message);
          break;
        case "launch":
          await this.handleLaunch(message, args);
          break;
        case "attach":
          await this.handleAttach(message, args);
          break;
        case "setBreakpoints":
          await this.handleSetBreakpoints(message, args);
          break;
        case "setExceptionBreakpoints":
          this.sendResponse(message, {});
          break;
        case "configurationDone":
          await this.handleConfigurationDone(message);
          break;
        case "threads":
          this.handleThreads(message);
          break;
        case "stackTrace":
          await this.handleStackTrace(message);
          break;
        case "scopes":
          this.handleScopes(message);
          break;
        case "variables":
          await this.handleVariables(message, args);
          break;
        case "continue":
          this.handleContinue(message);
          break;
        case "next":
          this.handleNext(message);
          break;
        case "stepIn":
          this.handleStepIn(message);
          break;
        case "stepOut":
          this.handleStepOut(message);
          break;
        case "pause":
          this.handlePause(message);
          break;
        case "evaluate":
          await this.handleEvaluate(message, args);
          break;
        case "disconnect":
        case "terminate":
          this.handleDisconnect(message);
          break;
        default:
          this.sendResponse(message, {}, true);
          break;
      }
    } catch (err) {
      this.sendResponse(message, {}, false, err.message);
    }
  }

  // ─── DAP 各命令具体实现 ────────────────────────────────────────────────────

  handleInitialize(request) {
    this.sendResponse(request, {
      supportsConfigurationDoneRequest: true,
      supportsFunctionBreakpoints: false,
      supportsConditionalBreakpoints: false,
      supportsEvaluateForHovers: true,
      supportsStepBack: false,
      supportsSetVariable: false,
      supportsRestartFrame: false,
      supportsGotoTargetsRequest: false,
      supportsStepInTargetsRequest: false,
      supportsCompletionsRequest: false,
      supportsModulesRequest: false,
      supportsTerminateRequest: true,
      supportsExceptionInfoRequest: false,
      supportsDelayedStackTraceLoading: false,
    });

    // 告知客户端调试适配器已准备就绪，可以发送断点与配置
    this.sendEvent("initialized");
  }

  async handleLaunch(request, args) {
    this.stopOnEntry = args.stopOnEntry === true;
    const host = args.debugHost || "127.0.0.1";
    const port = args.debugPort || args.debugServer || 54321;
    const program = args.program;

    if (!program || !fs.existsSync(program)) {
      this.sendResponse(request, {}, false, `目标脚本文件不存在: ${program}`);
      return;
    }

    const toolsPath = args.toolsPath || this.config.toolsPath;
    if (!toolsPath || !fs.existsSync(toolsPath)) {
      this.sendResponse(request, {}, false, `找不到 TzdTools.exe，请检查配置。`);
      return;
    }

    const cwd = args.cwd || path.dirname(program);
    const cmdArgs = [
      `--debug-port=${port}`,
      `--setProjectDirectory=${cwd}`,
      `--runMainTzd=${program}`,
    ];

    // 添加 stdlib 搜索路径
    const toolsDir = path.dirname(toolsPath);
    const candidateStdlibs = [
      path.join(toolsDir, "stdlib"),
      path.join(toolsDir, "..", "stdlib"),
      path.join(toolsDir, "..", "..", "stdlib"),
    ];
    for (const s of candidateStdlibs) {
      if (fs.existsSync(s)) {
        cmdArgs.push(`--addLibraryDirectory=${path.resolve(s)}`);
        break;
      }
    }

    // 附加额外自定义启动参数 (如 --jit, --experimental-compute 等)
    if (Array.isArray(args.args)) {
      for (const a of args.args) {
        if (a && typeof a === "string") {
          cmdArgs.push(a);
        }
      }
    }

    this.sendOutput(`[TzdDebugger] 启动调试目标: ${program}`);
    this.sendOutput(`[TzdDebugger] 命令行: ${toolsPath} ${cmdArgs.join(" ")}`);

    try {
      this.childProcess = cp.spawn(toolsPath, cmdArgs, {
        cwd: cwd,
        windowsHide: true,
        shell: false,
      });

      const stdoutDecoder = new TextDecoder("gbk", { fatal: false });
      const stderrDecoder = new TextDecoder("gbk", { fatal: false });

      this.childProcess.stdout.on("data", (data) => {
        const text = stdoutDecoder.decode(data, { stream: true });
        this.sendOutput(text, "stdout");
      });

      this.childProcess.stderr.on("data", (data) => {
        const text = stderrDecoder.decode(data, { stream: true });
        this.sendOutput(text, "stderr");
      });

      this.childProcess.on("close", (code) => {
        this.sendOutput(`[TzdDebugger] 进程已退出，退出码: ${code}`);
        this.sendEvent("terminated");
        this.dispose();
      });

      this.childProcess.on("error", (err) => {
        this.sendOutput(`[TzdDebugger] 启动失败: ${err.message}`, "stderr");
        this.sendEvent("terminated");
      });

      // 尝试连接调试服务器端口
      await this.connectToDebugServer(host, port, 40, 150);
      this.sendResponse(request, {});
    } catch (err) {
      this.sendResponse(request, {}, false, `连接调试服务器超时: ${err.message}`);
    }
  }

  async handleAttach(request, args) {
    const host = args.debugHost || "127.0.0.1";
    const port = args.debugPort || args.debugServer || 54321;

    this.sendOutput(`[TzdDebugger] 正在附加到调试服务器 ${host}:${port} ...`);
    try {
      await this.connectToDebugServer(host, port, 20, 200);
      this.sendResponse(request, {});
    } catch (err) {
      this.sendResponse(request, {}, false, `附加失败: ${err.message}`);
    }
  }

  async connectToDebugServer(host, port, maxRetries = 30, retryIntervalMs = 150) {
    for (let attempt = 1; attempt <= maxRetries; ++attempt) {
      try {
        await new Promise((resolve, reject) => {
          const socket = net.createConnection({ host, port }, () => {
            this.socket = socket;
            this.setupSocketHandlers();
            resolve();
          });

          socket.on("error", (err) => {
            socket.destroy();
            reject(err);
          });
        });
        this.sendOutput(`[TzdDebugger] 成功连接至调试服务 (${host}:${port})`);
        return;
      } catch (err) {
        if (attempt === maxRetries) {
          throw err;
        }
        await new Promise((r) => setTimeout(r, retryIntervalMs));
      }
    }
  }

  setupSocketHandlers() {
    if (!this.socket) return;

    this.socket.on("data", (data) => {
      const text = data.toString("utf8");
      this.recvBuffer += text;

      // 1. 抽取并处理所有的 *EXIT* 与 *BREAK* 事件行，避免污染命令响应缓冲区
      let exitIdx;
      while ((exitIdx = this.recvBuffer.indexOf("*EXIT*")) !== -1) {
        const lineEnd = this.recvBuffer.indexOf("\n", exitIdx);
        if (lineEnd === -1) break;
        this.recvBuffer = this.recvBuffer.substring(0, exitIdx) + this.recvBuffer.substring(lineEnd + 1);
        if (!this.isTerminated) {
          this.isTerminated = true;
          this.sendEvent("exited", { exitCode: 0 });
          this.sendEvent("terminated");
        }
      }

      let breakIdx;
      while ((breakIdx = this.recvBuffer.indexOf("*BREAK*")) !== -1) {
        const lineEnd = this.recvBuffer.indexOf("\n", breakIdx);
        if (lineEnd === -1) {
          // 当前行尚未接收完全，等待下一次数据包
          break;
        }
        const breakLine = this.recvBuffer.substring(breakIdx, lineEnd);
        this.recvBuffer = this.recvBuffer.substring(0, breakIdx) + this.recvBuffer.substring(lineEnd + 1);
        this.handleBreakLine(breakLine);
      }

      // 2. 处理已排队的交互命令响应 (:stack, :locals 等)
      this.drainCommandQueue();
    });

    this.socket.on("close", () => {
      if (!this.isTerminated) {
        this.isTerminated = true;
        this.sendEvent("exited", { exitCode: 0 });
        this.sendEvent("terminated");
      }
    });

    this.socket.on("error", (err) => {
      if (err.code === "ECONNRESET" || err.code === "EPIPE") {
        if (!this.isTerminated) {
          this.isTerminated = true;
          this.sendEvent("exited", { exitCode: 0 });
          this.sendEvent("terminated");
        }
        return;
      }
      this.sendOutput(`[TzdDebugger Socket 错误] ${err.message}`, "stderr");
    });
  }

  handleBreakLine(breakLine) {
    this.isSuspended = true;

    // 提取断点文件与行号
    const atMatch = /at\s+([^:\r\n]+):(\d+)/.exec(breakLine);
    if (atMatch) {
      this.currentFile = atMatch[1].trim();
      this.currentLine = parseInt(atMatch[2], 10);
    }

    const frameMatch = /\(frame:\s*([^)]+)\)/.exec(breakLine);
    if (frameMatch) {
      this.currentFrame = frameMatch[1].trim();
    }

    const isStartup = breakLine.includes("Attached") || breakLine.includes("Paused at startup");
    const isStep = breakLine.includes("step");

    if (isStartup) {
      if (!this.hasReportedEntry) {
        this.hasReportedEntry = true;
        if (this.configured && !this.stopOnEntry) {
          // 已配置完成且不需暂停在入口，立即继续
          this.sendRawCommand(":c\n");
          this.isSuspended = false;
        } else {
          this.sendEvent("stopped", {
            reason: "entry",
            threadId: 1,
            allThreadsStopped: true,
            description: "已暂停在启动入口",
          });
        }
      }
    } else {
      this.sendEvent("stopped", {
        reason: isStep ? "step" : "breakpoint",
        threadId: 1,
        allThreadsStopped: true,
        description: breakLine.replace(/^\*BREAK\*\s*/, "").trim(),
      });
    }
  }

  drainCommandQueue() {
    if (this.pendingCommands.length === 0) return;

    const currentCmd = this.pendingCommands[0];
    const promptIdx = this.recvBuffer.indexOf("TzdDebug> ");
    if (promptIdx !== -1) {
      const output = this.recvBuffer.substring(0, promptIdx);
      this.recvBuffer = this.recvBuffer.substring(promptIdx + "TzdDebug> ".length);

      this.pendingCommands.shift();
      currentCmd.resolve(output);

      // 发送下一个指令
      if (this.pendingCommands.length > 0) {
        const next = this.pendingCommands[0];
        if (this.socket && !this.socket.destroyed) {
          this.socket.write(next.cmd);
        }
      }
    }
  }

  sendDebugCommand(cmd) {
    return new Promise((resolve, reject) => {
      const item = { cmd: cmd.endsWith("\n") ? cmd : cmd + "\n", resolve, reject };
      this.pendingCommands.push(item);
      if (this.pendingCommands.length === 1) {
        if (this.socket && !this.socket.destroyed) {
          this.socket.write(item.cmd);
        }
      }
    });
  }

  sendRawCommand(cmd) {
    if (this.socket && !this.socket.destroyed) {
      const s = cmd.endsWith("\n") ? cmd : cmd + "\n";
      this.socket.write(s);
    }
  }

  async handleSetBreakpoints(request, args) {
    const source = args.source || {};
    const filePath = source.path || "";
    const breakpoints = args.breakpoints || [];

    const normPath = filePath.replace(/\\/g, "/");
    this.breakpointsMap.set(normPath, breakpoints);

    // 如果 Socket 已经建立，立即与 TzdTools 进行断点同步并消费响应
    if (this.socket && !this.socket.destroyed) {
      try {
        await this.sendDebugCommand(`:bp clear "${filePath}"\n`);
        for (const bp of breakpoints) {
          await this.sendDebugCommand(`:bp add "${filePath}" ${bp.line}\n`);
        }
      } catch (_) {}
    }

    const responseBreakpoints = breakpoints.map((bp, index) => ({
      id: index + 1,
      verified: true,
      line: bp.line,
      source: source,
    }));

    this.sendResponse(request, {
      breakpoints: responseBreakpoints,
    });
  }

  async handleConfigurationDone(request) {
    this.configured = true;

    // 同步所有已缓存的文件断点
    if (this.socket && !this.socket.destroyed) {
      try {
        for (const [fPath, bps] of this.breakpointsMap.entries()) {
          await this.sendDebugCommand(`:bp clear "${fPath}"\n`);
          for (const bp of bps) {
            await this.sendDebugCommand(`:bp add "${fPath}" ${bp.line}\n`);
          }
        }
      } catch (_) {}
    }

    // 若非停在入口模式，继续执行
    if (this.isSuspended && !this.stopOnEntry) {
      this.sendRawCommand(":c\n");
      this.isSuspended = false;
    }

    this.sendResponse(request, {});
  }

  handleThreads(request) {
    this.sendResponse(request, {
      threads: [
        {
          id: 1,
          name: "Main Thread (主线程)",
        },
      ],
    });
  }

  async handleStackTrace(request) {
    let rawStack = "";
    try {
      rawStack = await this.sendDebugCommand(":stack\n");
    } catch (_) {
      rawStack = "";
    }

    // 解析调用栈文本
    // === Call Stack (depth: 2) ===
    //   [2] helper
    //   [1] main
    //   [0] <global_scope>
    const frames = [];
    const lines = rawStack.split("\n");
    let frameId = 1000;

    for (const line of lines) {
      const match = /^\s*\[(\d+)\]\s+(.*)$/.exec(line.trim());
      if (match) {
        const funcName = match[2].trim();

        // 栈顶帧使用当前中断的文件与行号
        const isTop = frames.length === 0;
        const sourcePath = isTop && this.currentFile ? this.currentFile : "";
        const sourceName = sourcePath ? path.basename(sourcePath) : "TzdScript";

        frames.push({
          id: frameId++,
          name: funcName,
          source: sourcePath
            ? {
                name: sourceName,
                path: sourcePath,
              }
            : undefined,
          line: isTop ? this.currentLine : 1,
          column: 1,
        });
      }
    }

    if (frames.length === 0) {
      frames.push({
        id: 1,
        name: this.currentFrame || "<global_scope>",
        source: this.currentFile
          ? {
              name: path.basename(this.currentFile),
              path: this.currentFile,
            }
          : undefined,
        line: this.currentLine,
        column: 1,
      });
    }

    this.sendResponse(request, {
      stackFrames: frames,
      totalFrames: frames.length,
    });
  }

  handleScopes(request) {
    this.sendResponse(request, {
      scopes: [
        {
          name: "局部变量 (Locals)",
          presentationHint: "locals",
          variablesReference: 1001,
          expensive: false,
        },
        {
          name: "全局变量 (Globals)",
          presentationHint: "globals",
          variablesReference: 1002,
          expensive: false,
        },
      ],
    });
  }

  async handleVariables(request, args) {
    const varRef = args.variablesReference;
    let cmd = ":locals\n";
    if (varRef === 1002) {
      cmd = ":globals\n";
    }

    let rawVars = "";
    try {
      rawVars = await this.sendDebugCommand(cmd);
    } catch (_) {
      rawVars = "";
    }

    const variables = [];
    const lines = rawVars.split("\n");

    for (const line of lines) {
      const trimLine = line.trim();
      if (!trimLine || trimLine.startsWith("===") || trimLine.startsWith("(")) {
        continue;
      }
      const eqPos = trimLine.indexOf(" = ");
      if (eqPos !== -1) {
        const name = trimLine.substring(0, eqPos).trim();
        const value = trimLine.substring(eqPos + 3).trim();
        let typeStr = "variable";
        if (value.startsWith('"')) typeStr = "string";
        else if (value === "true" || value === "false") typeStr = "boolean";
        else if (/^-?\d+(\.\d+)?$/.test(value)) typeStr = "number";
        else if (value.startsWith("array")) typeStr = "array";
        else if (value.startsWith("map")) typeStr = "map";
        else if (value.startsWith("function")) typeStr = "function";

        variables.push({
          name: name,
          value: value,
          type: typeStr,
          variablesReference: 0,
        });
      }
    }

    this.sendResponse(request, {
      variables: variables,
    });
  }

  handleContinue(request) {
    this.isSuspended = false;
    this.sendRawCommand(":c\n");
    this.sendResponse(request, {
      allThreadsContinued: true,
    });
  }

  handleNext(request) {
    this.isSuspended = false;
    this.sendRawCommand(":over\n");
    this.sendResponse(request, {});
  }

  handleStepIn(request) {
    this.isSuspended = false;
    this.sendRawCommand(":into\n");
    this.sendResponse(request, {});
  }

  handleStepOut(request) {
    this.isSuspended = false;
    this.sendRawCommand(":out\n");
    this.sendResponse(request, {});
  }

  handlePause(request) {
    this.sendResponse(request, {});
  }

  async handleEvaluate(request, args) {
    const expr = args.expression || "";
    if (!expr.trim()) {
      this.sendResponse(request, { result: "", variablesReference: 0 });
      return;
    }

    try {
      let output = await this.sendDebugCommand(expr);
      // 清除提示符
      output = output.replace(/TzdDebug>\s*$/g, "").trim();
      this.sendResponse(request, {
        result: output || "(no output)",
        variablesReference: 0,
      });
    } catch (err) {
      this.sendResponse(request, {
        result: `Error: ${err.message}`,
        variablesReference: 0,
      });
    }
  }

  handleDisconnect(request) {
    this.dispose();
    this.sendResponse(request, {});
  }

  dispose() {
    if (this.socket && !this.socket.destroyed) {
      try {
        this.socket.write("exit\n");
        this.socket.end();
        this.socket.destroy();
      } catch (_) {}
      this.socket = null;
    }

    if (this.childProcess) {
      try {
        this.childProcess.kill();
      } catch (_) {}
      this.childProcess = null;
    }
  }
}

module.exports = { TzdDebugSession };
