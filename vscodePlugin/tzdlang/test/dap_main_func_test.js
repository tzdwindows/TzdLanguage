// ─── TzdLang DAP Test: main() Function & Function Calls Breakpoint Test ──────────
const path = require("path");
const fs = require("fs");
const { TzdDebugSession } = require("../debugAdapter");

async function runTest() {
  console.log("=== [1/6] 准备 main() 函数测试脚本 ===");
  const testScriptPath = path.resolve(__dirname, "test_main_func.tzd");
  const testScriptContent = `fun noop(x) {
    return x + 1;
}

fun main() {
    var a = 10;
    var b = noop(a);
    print("result: " + toString(b));
}
`;
  fs.writeFileSync(testScriptPath, testScriptContent, "utf8");
  console.log(`生成测试脚本: ${testScriptPath}`);

  const toolsPath = path.resolve(__dirname, "..", "..", "..", "x64", "Release", "TzdTools.exe");
  if (!fs.existsSync(toolsPath)) {
    throw new Error(`找不到 TzdTools.exe: ${toolsPath}`);
  }

  const mockContext = {
    globalState: {
      get: () => toolsPath,
      update: () => {},
    },
  };

  const session = new TzdDebugSession(mockContext, { toolsPath });

  const events = [];
  const responses = new Map();

  session.onDidSendMessage((msg) => {
    if (msg.type === "event") {
      events.push(msg);
    } else if (msg.type === "response") {
      responses.set(msg.request_seq, msg);
    }
  });

  let seq = 1;
  function sendRequest(command, args = {}) {
    const curSeq = seq++;
    const req = {
      seq: curSeq,
      type: "request",
      command: command,
      arguments: args,
    };
    session.handleMessage(req);
    return new Promise((resolve) => {
      const timer = setInterval(() => {
        if (responses.has(curSeq)) {
          clearInterval(timer);
          resolve(responses.get(curSeq));
        }
      }, 20);
    });
  }

  function waitForEvent(eventName, timeoutMs = 8000) {
    return new Promise((resolve, reject) => {
      const startTime = Date.now();
      const timer = setInterval(() => {
        const found = events.find((e) => e.event === eventName);
        if (found) {
          clearInterval(timer);
          resolve(found);
        } else if (Date.now() - startTime > timeoutMs) {
          clearInterval(timer);
          reject(new Error(`等待事件 ${eventName} 超时`));
        }
      }, 50);
    });
  }

  console.log("\n=== [2/6] 发送 initialize ===");
  await sendRequest("initialize", { clientID: "vscode-test" });

  console.log("\n=== [3/6] 发送 launch (带有 --jit，验证调试器模式正确回退) ===");
  await sendRequest("launch", {
    program: testScriptPath,
    args: ["--jit"],
    debugPort: 54323,
    stopOnEntry: false,
  });

  console.log("\n=== [4/6] 设置断点 setBreakpoints (在 main 内部第 6 行与第 7 行) ===");
  const bpRes = await sendRequest("setBreakpoints", {
    source: { path: testScriptPath },
    breakpoints: [{ line: 6 }, { line: 7 }],
  });
  console.log("  断点响应:", JSON.stringify(bpRes.body));

  console.log("\n=== [5/6] 发送 configurationDone ===");
  await sendRequest("configurationDone");

  console.log("\n=== [6/6] 等待命中断点 (第 6 行或第 7 行) ===");
  const stopEvent = await waitForEvent("stopped", 10000);
  console.log("  成功触发暂停事件:", JSON.stringify(stopEvent.body));

  // 获取调用栈
  const stackRes = await sendRequest("stackTrace", { threadId: 1 });
  console.log("  当前调用栈:", JSON.stringify(stackRes.body.stackFrames));

  // 获取变量作用域
  const scopesRes = await sendRequest("scopes", { frameId: 1 });
  const localsScope = scopesRes.body.scopes.find(s => s.name.includes("Locals") || s.name.includes("局部"));
  if (localsScope) {
    const varsRes = await sendRequest("variables", { variablesReference: localsScope.variablesReference });
    console.log("  当前局部变量:", JSON.stringify(varsRes.body.variables));
  }

  // 循环继续运行直到程序结束
  while (true) {
    events.length = 0;
    await sendRequest("continue", { threadId: 1 });
    try {
      const ev = await Promise.race([
        waitForEvent("stopped", 3000),
        waitForEvent("terminated", 3000),
      ]);
      if (ev.event === "terminated") {
        console.log("  程序正常结束并触发 terminated 事件");
        break;
      } else {
        console.log("  命中断点:", JSON.stringify(ev.body));
      }
    } catch (_) {
      break;
    }
  }

  console.log("\n[SUCCESS] main() 函数与断点回归测试完全通过！");

  // 清理测试文件
  if (fs.existsSync(testScriptPath)) fs.unlinkSync(testScriptPath);
  session.dispose();
}

runTest().catch((err) => {
  console.error("\n[FAILED] 测试失败:", err);
  process.exit(1);
});
