// ─── TzdLang Debug Adapter Protocol (DAP) Implementation ─────────────────────
// 完整的 Debug Adapter 协议实现，支持行断点、单步跳过/进入/跳出、调用栈、变量监视与表达式求值。
// ──────────────────────────────────────────────────────────────────────────────

const net = require("net");
const cp = require("child_process");
const path = require("path");
const fs = require("fs");
const { EventEmitter } = require("events");
const { TextDecoder } = require("util");

// ─── 文件日志 (用于VS Code调试诊断) ──────────────────────────────────────────
const LOG_FILE = require("os").homedir() + "\\tzdadapter.log";
function flog(...args) {
  try {
    const line = new Date().toISOString() + " " + args.join(" ") + "\n";
    fs.appendFileSync(LOG_FILE, line);
  } catch(_) {}
}
flog("=== debugAdapter.js v0.2.2 loaded ===");


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
    this.isStartupPaused = false;
    this.hasReportedEntry = false;
    this.isTerminated = false;
    this.breakpointsSynced = false;

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
    flog(`REQ ${command}`, JSON.stringify(args).substring(0, 200));

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
    flog(`handleLaunch: program=${program} port=${port} stopOnEntry=${this.stopOnEntry}`);

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

    this.jitEnabled = args.jit !== false;

    // 默认开启 JIT 极速运行与 JIT 调试支持
    if (this.jitEnabled) {
      if (!cmdArgs.includes("--jit")) cmdArgs.push("--jit");
      if (!cmdArgs.includes("--jit-debug")) cmdArgs.push("--jit-debug");
      const optLevel = args.optLevel !== undefined ? args.optLevel : 3;
      cmdArgs.push(`-O${optLevel}`);
      if (args.inlineThreshold !== undefined) {
        cmdArgs.push(`--inline-threshold=${args.inlineThreshold}`);
      }
      if (args.enableAstInlining === false) {
        cmdArgs.push("--no-inline");
      }
      if (args.enableMathIntrinsics === false) {
        cmdArgs.push("--no-jit-intrinsics");
      }
      if (args.enableLoopUnroll === false) {
        cmdArgs.push("--no-unroll");
      }
    } else {
      cmdArgs.push("--noJit");
    }

    // 附加额外自定义启动参数 (如 --experimental-compute 等)
    if (Array.isArray(args.args)) {
      for (const a of args.args) {
        if (a && typeof a === "string" && !cmdArgs.includes(a)) {
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

      const utf8Decoder = new TextDecoder("utf-8", { fatal: true });
      const gbkDecoder = new TextDecoder("gbk", { fatal: false });
      const decodeBuffer = (buf) => {
        try {
          return utf8Decoder.decode(buf);
        } catch {
          return gbkDecoder.decode(buf);
        }
      };

      this.childProcess.stdout.on("data", (data) => {
        const text = decodeBuffer(data);
        this.sendOutput(text, "stdout");
      });

      this.childProcess.stderr.on("data", (data) => {
        const text = decodeBuffer(data);
        this.sendOutput(text, "stderr");
      });

      this.childProcess.on("close", (code) => {
        this.sendOutput(`[TzdDebugger] 进程已退出，退出码: ${code}`);
        if (!this.isTerminated) {
          this.isTerminated = true;
          this.sendEvent("exited", { exitCode: code || 0 });
          this.sendEvent("terminated");
        }
        this.dispose();
      });

      this.childProcess.on("error", (err) => {
        this.sendOutput(`[TzdDebugger] 启动失败: ${err.message}`, "stderr");
        if (!this.isTerminated) {
          this.isTerminated = true;
          this.sendEvent("terminated");
        }
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

      // 1. 处理所有 *EXIT* 行
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

      // 2. 处理所有 *BREAK* 事件块 (包含伴随的 *FILE*, *LINE*, *FRAME* 行)
      while (true) {
        const breakIdx = this.recvBuffer.indexOf("*BREAK*");
        if (breakIdx === -1) break;

        const lineEnd = this.recvBuffer.indexOf("\n", breakIdx);
        if (lineEnd === -1) {
          // 当前 *BREAK* 行尚未完整接收，等待后续数据
          break;
        }

        const breakLine = this.recvBuffer.substring(breakIdx, lineEnd).trim();
        let file = "";
        let line = 1;
        let frame = "";
        let removeEnd = lineEnd + 1;

        // 向下探测 *FILE*, *LINE*, *FRAME* 属性行
        let remainder = this.recvBuffer.substring(removeEnd);
        let incompleteSubline = false;

        while (remainder.length > 0) {
          const nextNl = remainder.indexOf("\n");
          if (nextNl === -1) {
            if (remainder.startsWith("*FILE*") || remainder.startsWith("*LINE*") || remainder.startsWith("*FRAME*")) {
              incompleteSubline = true;
            }
            break;
          }
          const subLine = remainder.substring(0, nextNl).trim();
          if (subLine.startsWith("*FILE*")) {
            file = subLine.substring(6).trim();
            removeEnd += nextNl + 1;
            remainder = this.recvBuffer.substring(removeEnd);
          } else if (subLine.startsWith("*LINE*")) {
            line = parseInt(subLine.substring(6).trim(), 10);
            removeEnd += nextNl + 1;
            remainder = this.recvBuffer.substring(removeEnd);
          } else if (subLine.startsWith("*FRAME*")) {
            frame = subLine.substring(7).trim();
            removeEnd += nextNl + 1;
            remainder = this.recvBuffer.substring(removeEnd);
          } else {
            break;
          }
        }

        if (incompleteSubline) {
          // 等待属性行接收完整
          break;
        }

        this.recvBuffer = this.recvBuffer.substring(0, breakIdx) + this.recvBuffer.substring(removeEnd);
        flog(`BREAK_EVENT: line=${line} file=${file} breakLine=${breakLine.substring(0,80)}`);
        this.handleBreakEvent(breakLine, file, line, frame);
      }

      // 3. 处理已排队的交互命令响应 (:stack, :locals 等)
      this.drainCommandQueue();
    });

    this.socket.on("close", () => {
      while (this.pendingCommands.length > 0) {
        const cmd = this.pendingCommands.shift();
        cmd.reject(new Error("Socket closed"));
      }
      if (!this.isTerminated) {
        this.isTerminated = true;
        this.sendEvent("exited", { exitCode: 0 });
        this.sendEvent("terminated");
      }
    });

    this.socket.on("error", (err) => {
      while (this.pendingCommands.length > 0) {
        const cmd = this.pendingCommands.shift();
        cmd.reject(err);
      }
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

  async syncAllBreakpoints() {
    if (!this.socket || this.socket.destroyed) return;
    flog(`syncAllBreakpoints: starting sync for ${this.breakpointsMap.size} files`);
    for (const [fPath, bps] of this.breakpointsMap.entries()) {
      flog(`syncAllBreakpoints: clearing ${fPath}`);
      try {
        await this.sendDebugCommand(`:bp clear "${fPath}"\n`);
      } catch (e) {
        flog(`syncAllBreakpoints :bp clear error: ${e.message}`);
      }
      for (const bp of bps) {
        flog(`syncAllBreakpoints: adding ${fPath}:${bp.line}`);
        try {
          await this.sendDebugCommand(`:bp add "${fPath}" ${bp.line}\n`);
        } catch (e) {
          flog(`syncAllBreakpoints :bp add error: ${e.message}`);
        }
      }
    }
    this.breakpointsSynced = true;
    flog(`syncAllBreakpoints: completed, breakpointsSynced=true`);
  }

  async handleBreakEvent(breakLine, file, line, frame) {
    this.isSuspended = true;

    // 若 C++ 未发送独立的 *FILE* / *LINE*，使用兼容正则回退解析
    if (!file) {
      // 匹配 Windows 盘符 C:\path:11 或 Unix /path:11
      const atMatch = /at\s+([a-zA-Z]:[\\/][^:\r\n]+|\S+?):(\d+)/.exec(breakLine);
      if (atMatch) {
        file = atMatch[1].trim();
        line = parseInt(atMatch[2], 10);
      }
    }

    if (!frame) {
      const frameMatch = /\(frame:\s*([^)]+(?:\([^)]*\))?[^)]*)\)/.exec(breakLine);
      if (frameMatch) {
        frame = frameMatch[1].trim();
      }
    }

    if (file) this.currentFile = path.normalize(file);
    if (line) this.currentLine = line;
    if (frame) this.currentFrame = frame;

    const isStartup = breakLine.includes("Attached") || breakLine.includes("Paused at startup");
    const isStep = breakLine.includes("step");
    flog(`handleBreakEvent: isStartup=${isStartup} isStep=${isStep} configured=${this.configured} stopOnEntry=${this.stopOnEntry} isSuspended=${this.isSuspended} breakpointsSynced=${this.breakpointsSynced}`);

    if (isStartup) {
      this.isStartupPaused = true;
      if (!this.hasReportedEntry) {
        this.hasReportedEntry = true;

        // ⭐ 关键：程序在启动第一条语句挂起时，必须先将所有断点全部同步给 C++！
        if (!this.breakpointsSynced) {
          flog("handleBreakEvent: startup pause reached, syncing all breakpoints before resuming...");
          try {
            await this.syncAllBreakpoints();
          } catch (e) {
            flog("handleBreakEvent: syncAllBreakpoints EXCEPTION " + e.message);
          }
        }

        if (this.stopOnEntry) {
          // 用户明确要求在程序入口暂停
          flog("handleBreakEvent: sending stopped(entry)");
          this.sendEvent("stopped", {
            reason: "entry",
            threadId: 1,
            allThreadsStopped: true,
            description: "已暂停在启动入口",
          });
        } else if (this.configured) {
          // 已完成配置且无需入口暂停，直接继续执行
          flog("handleBreakEvent: already configured and breakpoints synced, sending :c to resume");
          this.sendRawCommand(":c\n");
          this.isSuspended = false;
          this.isStartupPaused = false;
        } else {
          flog("handleBreakEvent: not yet configured, waiting for configurationDone");
        }
      }
    } else {
      flog(`handleBreakEvent: sending stopped(${isStep ? "step" : "breakpoint"}) at line ${this.currentLine}`);
      this.sendEvent("stopped", {
        reason: isStep ? "step" : "breakpoint",
        threadId: 1,
        allThreadsStopped: true,
        description: breakLine.replace(/^\*BREAK\*\s*/, "").trim(),
      });
    }
  }

  drainCommandQueue() {
    if (this.pendingCommands.length === 0) {
      // 没有等待响应的指令，清理多余的提示符标记，防止后续指令误命中历史提示符
      const idx = this.recvBuffer.lastIndexOf("TzdDebug> ");
      if (idx !== -1) {
        this.recvBuffer = this.recvBuffer.substring(idx + "TzdDebug> ".length);
      }
      return;
    }

    const promptIdx = this.recvBuffer.indexOf("TzdDebug> ");
    if (promptIdx !== -1) {
      const output = this.recvBuffer.substring(0, promptIdx);
      this.recvBuffer = this.recvBuffer.substring(promptIdx + "TzdDebug> ".length);

      const currentCmd = this.pendingCommands.shift();
      currentCmd.resolve(output);

      // 发送下一个排队指令
      if (this.pendingCommands.length > 0) {
        const nextIdx = this.recvBuffer.lastIndexOf("TzdDebug> ");
        if (nextIdx !== -1) {
          this.recvBuffer = this.recvBuffer.substring(nextIdx + "TzdDebug> ".length);
        }
        const next = this.pendingCommands[0];
        if (this.socket && !this.socket.destroyed) {
          this.socket.write(next.cmd);
        }
      }
    }
  }

  sendDebugCommand(cmd, timeoutMs = 8000) {
    return new Promise((resolve, reject) => {
      let timer = null;
      const item = {
        cmd: cmd.endsWith("\n") ? cmd : cmd + "\n",
        resolve: (val) => {
          if (timer) clearTimeout(timer);
          resolve(val);
        },
        reject: (err) => {
          if (timer) clearTimeout(timer);
          reject(err);
        },
      };
      if (timeoutMs > 0) {
        timer = setTimeout(() => {
          const idx = this.pendingCommands.indexOf(item);
          if (idx !== -1) {
            this.pendingCommands.splice(idx, 1);
            flog(`sendDebugCommand TIMEOUT for: ${item.cmd.trim()}`);
            reject(new Error(`Command timed out: ${item.cmd.trim()}`));
          }
        }, timeoutMs);
      }
      this.pendingCommands.push(item);
      if (this.pendingCommands.length === 1) {
        // 清理当前缓冲区残留的历史提示符，确保本次指令匹配到的是 C++ 后端执行本次指令产生的响应
        const idx = this.recvBuffer.lastIndexOf("TzdDebug> ");
        if (idx !== -1) {
          this.recvBuffer = this.recvBuffer.substring(idx + "TzdDebug> ".length);
        }
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
    flog(`handleSetBreakpoints: file=${normPath} bps=${JSON.stringify(breakpoints)} socketOk=${!!(this.socket && !this.socket.destroyed)} isSuspended=${this.isSuspended}`);

    // 如果 Socket 已经建立，立即同步断点并消费响应
    if (this.socket && !this.socket.destroyed) {
      try {
        flog(`handleSetBreakpoints: syncing breakpoints for ${normPath}...`);
        await this.sendDebugCommand(`:bp clear "${normPath}"\n`);
        for (const bp of breakpoints) {
          flog(`handleSetBreakpoints: sending :bp add line ${bp.line}`);
          await this.sendDebugCommand(`:bp add "${normPath}" ${bp.line}\n`);
        }
        flog(`handleSetBreakpoints: breakpointsSynced=true`);
        this.breakpointsSynced = true;
      } catch (e) {
        flog(`handleSetBreakpoints: EXCEPTION ${e.message}`);
      }
    } else {
      // 标记尚未同步，稍后由 startup pause 或 configurationDone 触发同步
      this.breakpointsSynced = false;
      flog(`handleSetBreakpoints: deferred sync until socket ready/startup pause`);
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

  async syncAllBreakpoints() {
    if (!this.socket || this.socket.destroyed) return;
    flog("syncAllBreakpoints: starting sync...");
    for (const [normPath, bps] of this.breakpointsMap.entries()) {
      flog(`syncAllBreakpoints: syncing ${bps.length} breakpoints for ${normPath}`);
      try {
        await this.sendDebugCommand(`:bp clear "${normPath}"\n`);
        for (const bp of bps) {
          flog(`syncAllBreakpoints: sending :bp add "${normPath}" ${bp.line}`);
          await this.sendDebugCommand(`:bp add "${normPath}" ${bp.line}\n`);
        }
      } catch (e) {
        flog(`syncAllBreakpoints EXCEPTION for ${normPath}: ${e.message}`);
      }
    }
    this.breakpointsSynced = true;
    flog("syncAllBreakpoints: finished successfully");
  }

  async handleConfigurationDone(request) {
    this.configured = true;
    flog(`handleConfigurationDone: breakpointsSynced=${this.breakpointsSynced} isSuspended=${this.isSuspended} stopOnEntry=${this.stopOnEntry}`);

    // 若尚未在 setBreakpoints 中同步，且 socket 已就绪，在此处集中同步所有断点
    if (!this.breakpointsSynced && this.socket && !this.socket.destroyed) {
      try {
        await this.syncAllBreakpoints();
      } catch (e) {
        flog(`handleConfigurationDone: syncAllBreakpoints EXCEPTION ${e.message}`);
      }
    }

    // 若非停在入口模式，且当前处于暂停状态，继续执行
    if (this.isSuspended && !this.stopOnEntry) {
      flog(`handleConfigurationDone: sending :c to resume`);
      this.sendRawCommand(":c\n");
      this.isSuspended = false;
      this.isStartupPaused = false;
    } else {
      flog(`handleConfigurationDone: NOT sending :c (isSuspended=${this.isSuspended} stopOnEntry=${this.stopOnEntry})`);
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
      flog(`handleStackTrace: rawStack=${rawStack.replace(/\r?\n/g, ' -- ')}`);
    } catch (e) {
      flog(`handleStackTrace: EXCEPTION ${e.message}`);
      rawStack = "";
    }

    // 解析调用栈文本
    // === Call Stack (depth: 2) ===
    //   [2] helper (C:\path\test.tzd:10) (Interpreted)
    //   [1] main (C:\path\test.tzd:4) (Interpreted)
    //   [0] <global_scope>
    const frames = [];
    const lines = rawStack.split("\n");
    let frameId = 1000;

    for (const line of lines) {
      const match = /^\s*\[(\d+)\]\s+(.*)$/.exec(line.trim());
      if (match) {
        const fullFrame = match[2].trim();
        const isTop = frames.length === 0;

        let frameSource = isTop && this.currentFile ? this.currentFile : "";
        let frameLine = isTop && this.currentLine ? this.currentLine : 1;

        // 从帧描述中尝试提取源文件与行号
        const locMatch = /\(([a-zA-Z]:[\\/][^:\r\n]+|\S+?):(\d+)\)/.exec(fullFrame);
        if (locMatch) {
          if (!isTop || !frameSource) {
            frameSource = locMatch[1].trim();
            frameLine = parseInt(locMatch[2], 10);
          }
        }

        // 提取干净的函数名称
        let displayName = fullFrame;
        const parenIdx = displayName.indexOf(" (");
        if (parenIdx !== -1) {
          displayName = displayName.substring(0, parenIdx).trim();
        }

        const sourceName = frameSource ? path.basename(frameSource) : "TzdScript";

        frames.push({
          id: frameId++,
          name: displayName || fullFrame,
          source: frameSource
            ? {
                name: sourceName,
                path: path.normalize(frameSource),
              }
            : undefined,
          line: frameLine,
          column: 1,
        });
      }
    }

    if (frames.length === 0) {
      const src = this.currentFile || "";
      frames.push({
        id: 1000,
        name: this.currentFrame || "main",
        source: src
          ? {
              name: path.basename(src),
              path: path.normalize(src),
            }
          : undefined,
        line: this.currentLine || 1,
        column: 1,
      });
    }

    this.sendResponse(request, {
      stackFrames: frames,
      totalFrames: frames.length,
    });
  }

  handleScopes(request) {
    const scopes = [
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
    ];

    if (this.jitEnabled !== false) {
      scopes.push({
        name: "JIT 引擎 (JIT Engine)",
        presentationHint: "registers",
        variablesReference: 1003,
        expensive: false,
      });
    }

    this.sendResponse(request, {
      scopes: scopes,
    });
  }

  async handleVariables(request, args) {
    const varRef = args.variablesReference;

    if (varRef === 1003) {
      // 查询 JIT 状态及编译函数列表
      const variables = [];
      try {
        const rawStatus = await this.sendDebugCommand(":jit status\n");
        const statusLines = rawStatus.split("\n");
        for (const line of statusLines) {
          const trimLine = line.trim();
          if (!trimLine || trimLine.startsWith("===") || trimLine.startsWith("---") || trimLine.startsWith("Use ':'")) continue;
          const colonPos = trimLine.indexOf(": ");
          if (colonPos !== -1) {
            const key = trimLine.substring(0, colonPos).trim();
            const val = trimLine.substring(colonPos + 2).trim();
            variables.push({
              name: key,
              value: val,
              type: "jit_status",
              variablesReference: 0,
            });
          }
        }

        // 查询已编译的 JIT 函数列表
        const rawList = await this.sendDebugCommand(":jit list\n");
        const listLines = rawList.split("\n");
        for (const line of listLines) {
          const trimLine = line.trim();
          if (!trimLine || trimLine.startsWith("===") || trimLine.includes("(No JIT functions")) continue;
          if (trimLine.startsWith("[")) {
            const closeBracket = trimLine.indexOf("]");
            if (closeBracket !== -1) {
              const idxStr = trimLine.substring(0, closeBracket + 1);
              const rest = trimLine.substring(closeBracket + 1).trim();
              const pipePos = rest.indexOf("|");
              const funcName = pipePos !== -1 ? rest.substring(0, pipePos).trim() : rest;
              const meta = pipePos !== -1 ? rest.substring(pipePos).trim() : "";
              variables.push({
                name: `${idxStr} ${funcName}`,
                value: meta,
                type: "jit_symbol",
                variablesReference: 0,
              });
            }
          }
        }
      } catch (e) {
        variables.push({
          name: "JIT Status",
          value: `Error: ${e.message}`,
          type: "error",
          variablesReference: 0,
        });
      }

      this.sendResponse(request, {
        variables: variables,
      });
      return;
    }

    let cmd = ":locals\n";
    if (varRef === 1002) {
      cmd = ":globals\n";
    }

    let rawVars = "";
    try {
      rawVars = await this.sendDebugCommand(cmd);
      flog(`handleVariables: cmd=${cmd.trim()} rawVars=${rawVars.replace(/\r?\n/g, ' -- ')}`);
    } catch (e) {
      flog(`handleVariables: EXCEPTION ${e.message}`);
      rawVars = "";
    }

    const variables = [];
    const lines = rawVars.split("\n");

    for (const line of lines) {
      const trimLine = line.trim();
      if (!trimLine || trimLine.startsWith("===") || trimLine.startsWith("(") || trimLine.startsWith("No active scope") || trimLine.startsWith("No user-defined")) {
        continue;
      }
      const eqPos = trimLine.indexOf(" = ");
      if (eqPos !== -1) {
        const name = trimLine.substring(0, eqPos).trim();
        let value = trimLine.substring(eqPos + 3).trim();
        let typeStr = "variable";
        if (value.startsWith('"')) typeStr = "string";
        else if (value === "true" || value === "false") typeStr = "boolean";
        else if (/^-?\d+(\.\d+)?$/.test(value)) typeStr = "number";
        else if (value.startsWith("array")) typeStr = "array";
        else if (value.startsWith("map")) typeStr = "map";
        else if (value.startsWith("function")) typeStr = "function";
        else if (value.startsWith("native_function")) typeStr = "native_function";
        else if (value.startsWith("class")) typeStr = "class";
        else if (value.startsWith("instance")) typeStr = "instance";
        else if (value.startsWith("pointer")) typeStr = "pointer";

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
    const rawExpr = (args.expression || "").trim();
    if (!rawExpr) {
      this.sendResponse(request, { result: "", variablesReference: 0 });
      return;
    }

    if (rawExpr.startsWith(":") || rawExpr === "jit" || rawExpr.startsWith("jit ")) {
      const execCmd = rawExpr.startsWith(":") ? rawExpr : ":" + rawExpr;
      try {
        let output = await this.sendDebugCommand(execCmd);
        output = output.replace(/TzdDebug>\s*$/g, "").trim();
        this.sendResponse(request, { result: output || "(no output)", variablesReference: 0 });
      } catch (err) {
        this.sendResponse(request, { result: `Error: ${err.message}`, variablesReference: 0 });
      }
      return;
    }

    // 智能处理表达式求值：若是纯表达式，包装为 print(...) 执行以便在监视/悬停中回显求值结果
    const isStmt = /^(var|int|float|double|string|bool|void|fun|class|if|while|for|switch|return|print|println)\b/.test(rawExpr) || rawExpr.endsWith(";");
    const evalCmd = isStmt ? (rawExpr.endsWith(";") ? rawExpr : rawExpr + ";") : `print(${rawExpr});`;

    try {
      let output = await this.sendDebugCommand(evalCmd);
      output = output.replace(/TzdDebug>\s*$/g, "").trim();
      if (output.includes("语法错误") && !isStmt) {
        let fallback = await this.sendDebugCommand(rawExpr + ";");
        output = fallback.replace(/TzdDebug>\s*$/g, "").trim();
      }
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
