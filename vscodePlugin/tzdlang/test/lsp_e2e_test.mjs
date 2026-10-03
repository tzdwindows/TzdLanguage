import { spawn } from "child_process";
import path from "path";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const serverPath = path.join(__dirname, "..", "server", "server.js");

console.log("=== Starting LSP End-to-End Test ===");

const server = spawn("node", [serverPath, "--stdio"], {
  stdio: ["pipe", "pipe", "inherit"],
});

let buffer = Buffer.alloc(0);
let seq = 1;
const pending = new Map();

server.stdout.on("data", (chunk) => {
  buffer = Buffer.concat([buffer, chunk]);
  while (true) {
    const headerEnd = buffer.indexOf("\r\n\r\n");
    if (headerEnd === -1) break;
    const headerStr = buffer.slice(0, headerEnd).toString("utf-8");
    const match = headerStr.match(/Content-Length:\s*(\d+)/i);
    if (!match) break;
    const contentLength = parseInt(match[1], 10);
    const bodyStart = headerEnd + 4;
    if (buffer.length < bodyStart + contentLength) break;
    const bodyBuf = buffer.slice(bodyStart, bodyStart + contentLength);
    buffer = buffer.slice(bodyStart + contentLength);

    const msg = JSON.parse(bodyBuf.toString("utf-8"));
    if (msg.id && pending.has(msg.id)) {
      const { resolve } = pending.get(msg.id);
      pending.delete(msg.id);
      resolve(msg);
    }
  }
});

function sendRequest(method, params) {
  const id = seq++;
  const msg = { jsonrpc: "2.0", id, method, params };
  const jsonStr = JSON.stringify(msg);
  const header = `Content-Length: ${Buffer.byteLength(jsonStr, "utf-8")}\r\n\r\n`;
  server.stdin.write(header + jsonStr);
  return new Promise((resolve, reject) => {
    pending.set(id, { resolve, reject });
  });
}

function sendNotification(method, params) {
  const msg = { jsonrpc: "2.0", method, params };
  const jsonStr = JSON.stringify(msg);
  const header = `Content-Length: ${Buffer.byteLength(jsonStr, "utf-8")}\r\n\r\n`;
  server.stdin.write(header + jsonStr);
}

async function runTest() {
  try {
    console.log("[1/5] Initializing LSP server...");
    const initRes = await sendRequest("initialize", {
      processId: process.pid,
      rootUri: "file:///" + __dirname.replace(/\\/g, "/"),
      capabilities: {},
    });
    console.log("  -> Initialized successfully. Capabilities:", Object.keys(initRes.result.capabilities));
    sendNotification("initialized", {});

    const testUri = "file:///" + path.join(__dirname, "test_lsp_doc.tzd").replace(/\\/g, "/");
    const docText = `import "time/DateTime.tzd";

fun test() {
    var x = toString(123);
    var sw = new Stopwatch();
    sw.start();
    i
}
`;

    console.log("[2/5] Sending didOpen for test document...");
    sendNotification("textDocument/didOpen", {
      textDocument: {
        uri: testUri,
        languageId: "tzd",
        version: 1,
        text: docText,
      },
    });

    // Wait a brief moment for indexing
    await new Promise((r) => setTimeout(r, 600));

    console.log("[3/5] Testing Go to Definition on toString...");
    // line 3: var x = toString(123); col 14 (inside toString)
    const defRes = await sendRequest("textDocument/definition", {
      textDocument: { uri: testUri },
      position: { line: 3, character: 14 },
    });
    console.log("  -> Definition on toString result:", JSON.stringify(defRes.result));
    if (defRes.result !== null) {
      throw new Error(`Expected null definition for native toString, got: ${JSON.stringify(defRes.result)}`);
    }
    console.log("  [PASS] toString does NOT jump to DateTime.tzd!");

    console.log("[4/5] Testing Hover on toString and input...");
    const hoverRes = await sendRequest("textDocument/hover", {
      textDocument: { uri: testUri },
      position: { line: 3, character: 14 },
    });
    console.log("  -> Hover on toString result:\n", hoverRes.result?.contents?.value);
    if (!hoverRes.result || !hoverRes.result.contents.value.includes("内置本地函数")) {
      throw new Error("Expected hover documentation for toString");
    }
    console.log("  [PASS] Hover displays rich native function documentation!");

    console.log("[5/5] Testing Completion on prefix 'i' (line 6, col 5)...");
    // line 6: "    i" (after 4 spaces and 'i' at col 5)
    const compRes = await sendRequest("textDocument/completion", {
      textDocument: { uri: testUri },
      position: { line: 6, character: 5 },
    });
    const items = compRes.result || [];
    const ifItem = items.find((x) => x.label === "if");
    console.log("  -> Total completion items:", items.length);
    console.log("  -> Found 'if' item:", JSON.stringify(ifItem));
    if (!ifItem) {
      throw new Error("Expected 'if' in completions when typing 'i'");
    }
    console.log("  [PASS] 'if' keyword snippet autocompletes successfully!");

    console.log("\n============================================================");
    console.log(" ALL LSP TESTS PASSED SUCCESSFULLY! ");
    console.log("============================================================");
    server.kill();
    process.exit(0);
  } catch (err) {
    console.error("Test failed:", err);
    server.kill();
    process.exit(1);
  }
}

runTest();
