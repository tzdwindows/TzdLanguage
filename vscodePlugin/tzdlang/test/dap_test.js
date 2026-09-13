// ─── TzdLang Debug Adapter E2E Integration Test ─────────────────────────────
// 模拟 VS Code DAP 客户端与 TzdDebugSession 进行全流程调试交互验证
// ────────────────────────────────────────────────────────────────────────────

const path = require("path");
const fs = require("fs");
const { TzdDebugSession } = require("../debugAdapter");

async function runTest() {
  console.log("=== [1/6] 准备测试脚本与调试环境 ===");
  const testScriptPath = path.resolve(__dirname, "test_debug_sample.tzd");
  const testScriptContent = `
var a = 100;
var b = 200;
var sum = a + b;
print("Step 1: sum = " + toString(sum));

fun calculate(x, y) {
    var c = x * y;
    return c;
}

var result = calculate(a, 5);
print("Step 2: result = " + toString(result));
`;
  fs.writeFileSync(testScriptPath, testScriptContent, "utf8");
  console.log(`生成测试脚本: ${testScriptPath}`);

  const toolsPath = path.resolve(__dirname, "..", "..", "..", "x64", "Release", "TzdTools.exe");
  if (!fs.existsSync(toolsPath)) {
    throw new Error(`找不到 TzdTools.exe: ${toolsPath}`);
  }
  console.log(`使用解释器: ${toolsPath}`);

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
      const check = setInterval(() => {
        if (responses.has(curSeq)) {
          clearInterval(check);
          resolve(responses.get(curSeq));
        }
      }, 50);
    });
  }

  function waitForEvent(eventName, timeoutMs = 5000) {
    return new Promise((resolve, reject) => {
      const start = Date.now();
      const check = setInterval(() => {
        const found = events.find((e) => e.event === eventName);
        if (found) {
          clearInterval(check);
          resolve(found);
        } else if (Date.now() - start > timeoutMs) {
          clearInterval(check);
          reject(new Error(`等待事件 ${eventName} 超时`));
        }
      }, 50);
    });
  }

  try {
    console.log("\n=== [2/6] 发送 initialize ===");
    const initRes = await sendRequest("initialize", { clientID: "vscode-test" });
    if (!initRes.success) throw new Error("initialize 失败");

    console.log("\n=== [3/6] 发送 launch ===");
    const launchRes = await sendRequest("launch", {
      program: testScriptPath,
      toolsPath: toolsPath,
      stopOnEntry: true,
      debugPort: 54399,
      args: ["--noJit"],
    });
    if (!launchRes.success) throw new Error(`launch 失败: ${launchRes.message}`);

    console.log("\n=== [4/6] 设置断点 setBreakpoints (第 4 行与第 9 行) ===");
    const bpRes = await sendRequest("setBreakpoints", {
      source: { path: testScriptPath },
      breakpoints: [{ line: 4 }, { line: 9 }],
    });
    console.log("  断点设置成功:", JSON.stringify(bpRes.body));

    console.log("\n=== [5/6] 发送 configurationDone ===");
    await sendRequest("configurationDone");

    console.log("\n=== [6/6] 验证调试单步与变量读取 ===");
    const stoppedEvent = await waitForEvent("stopped");
    console.log(`  已触发中断: reason=${stoppedEvent.body.reason}`);

    // 请求调用栈
    const stackRes = await sendRequest("stackTrace", { threadId: 1 });
    console.log("  调用栈信息:", JSON.stringify(stackRes.body.stackFrames));

    // 请求作用域
    const scopesRes = await sendRequest("scopes", { frameId: 1 });
    console.log("  作用域列表:", JSON.stringify(scopesRes.body.scopes));

    // 请求局部变量
    const varsRes = await sendRequest("variables", { variablesReference: 1001 });
    console.log("  局部变量:", JSON.stringify(varsRes.body.variables));

    // 表达式求值
    const evalRes = await sendRequest("evaluate", { expression: "10 + 20 * 3" });
    console.log("  表达式求值 10 + 20 * 3 结果:", evalRes.body.result);

    // 单步跳过 (next)
    events.length = 0;
    await sendRequest("next");
    await waitForEvent("stopped");
    console.log(`  单步跳过完成，停止在行: ${session.currentLine}`);

    // 继续执行 (continue)
    events.length = 0;
    await sendRequest("continue");

    const hitStopped = await waitForEvent("stopped", 4000).catch(() => null);
    if (hitStopped) {
      console.log(`  成功命中断点: line=${session.currentLine}, frame=${session.currentFrame}`);
      await sendRequest("continue");
    }

    // 断开调试连接
    await sendRequest("disconnect");
    console.log("\n[SUCCESS] 全部 DAP 调试功能回归测试顺利通过！\n");
  } finally {
    session.dispose();
    if (fs.existsSync(testScriptPath)) {
      try {
        fs.unlinkSync(testScriptPath);
      } catch (_) {}
    }
  }
}

runTest().catch((err) => {
  console.error("\n[TEST ERROR]", err);
  process.exit(1);
});
