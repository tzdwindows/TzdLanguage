const path = require("path");
const fs = require("fs");
const { TzdDebugSession } = require("../debugAdapter");

async function runTest() {
  console.log("=== [1/6] 准备 import + 断点 测试脚本 ===");
  const testScriptPath = path.resolve(__dirname, "test_import_bp.tzd");
  const testScriptContent = `import "time/DateTime.tzd";

fun testFunction() {
    var sw = new Stopwatch();
    sw.start();
    var sum = 0;
    for (var i = 0; i < 100; i++) {
        sum = sum + i;
    }
    sw.stop();
    print("sum = " + toString(sum));
}

testFunction();
`;
  fs.writeFileSync(testScriptPath, testScriptContent, "utf8");
  console.log(`生成测试脚本: ${testScriptPath}`);

  const toolsPath = path.resolve(__dirname, "..", "..", "..", "x64", "Release", "TzdTools.exe");
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

  console.log("\n=== [3/6] 发送 launch (使用动态端口) ===");
  await sendRequest("launch", {
    program: testScriptPath,
    args: ["--jit"],
    stopOnEntry: false,
  });

  console.log("\n=== [4/6] 设置断点 setBreakpoints (第 10 行: sw.stop()) ===");
  const bpRes = await sendRequest("setBreakpoints", {
    source: { path: testScriptPath },
    breakpoints: [{ line: 10 }],
  });
  console.log("  断点响应:", JSON.stringify(bpRes.body));

  console.log("\n=== [5/6] 发送 configurationDone ===");
  await sendRequest("configurationDone");

  console.log("\n=== [6/6] 等待命中断点 (第 10 行) ===");
  const stoppedEvent = await waitForEvent("stopped", 10000);
  console.log("  成功触发暂停事件:", JSON.stringify(stoppedEvent.body));

  if (stoppedEvent.body.reason === "breakpoint") {
    console.log("  断点正常命中！继续执行...");
    await sendRequest("continue", { threadId: 1 });
  }

  await waitForEvent("terminated", 5000);
  console.log("\n[SUCCESS] import + 断点回归测试完全通过！");
}

runTest()
  .then(() => process.exit(0))
  .catch((err) => {
    console.error("\n[FAILED] 测试失败:", err);
    process.exit(1);
  });
