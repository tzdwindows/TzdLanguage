/**
 * TzdLang Official Website — Interactive Engine (v2.0)
 * Native lightweight implementation: Xcode Workbench, Smooth Transitions, One-Click Copy, Mobile Drawer.
 */

// ---------------------------------------------------------------------------
// 1. Topic Catalog & Syntax Database
// ---------------------------------------------------------------------------
const WORKBENCH_DATA = {
  "01_variables": {
    file: "01_variables_and_types.tzd",
    title: "变量声明与混合类型系统",
    tag: "Core Syntax",
    desc: "支持三种变量声明范式：C 风格强类型（如 float ratio = 0.875;）、var 动态类型推断（如 var count = 42;）、冒号类型约束（如 ratio : float = 0.875;）；类体内支持 let/const 字段。",
    rule: "规范提示：局部语句层严禁使用 let/const（let/const 用于类成员）。声明类型支持 float ratio = 0.875; 或 var x = 10; 或 x : int = 10;。",
    raw: `// 1. 三种声明范式：显式强类型、var 动态推断与冒号类型
int count = 42;
var name = "TzdLang";
ratio : float = 0.875;

// 2. 原生数组与底层系统指针
var scores = [98, 95, 100];
ptr buffer = null;

// 3. 混合运算与字符串内建拼接
bool isActive = true;
if (isActive) {
    print("Welcome to " + name + "! Ratio: " + ratio + ", Score: " + scores[0]);
}`,
    code: `<span class="tzd-comment">// 1. 三种声明范式：显式强类型、var 动态推断与冒号类型</span>
<span class="tzd-type">int</span> count = <span class="tzd-number">42</span>;
<span class="tzd-keyword">var</span> name = <span class="tzd-string">"TzdLang"</span>;
ratio : <span class="tzd-type">float</span> = <span class="tzd-number">0.875</span>;

<span class="tzd-comment">// 2. 原生数组与底层系统指针</span>
<span class="tzd-keyword">var</span> scores = [<span class="tzd-number">98</span>, <span class="tzd-number">95</span>, <span class="tzd-number">100</span>];
<span class="tzd-type">ptr</span> buffer = <span class="tzd-keyword">null</span>;

<span class="tzd-comment">// 3. 混合运算与字符串内建拼接</span>
<span class="tzd-type">bool</span> isActive = <span class="tzd-keyword">true</span>;
<span class="tzd-keyword">if</span> (isActive) {
    <span class="tzd-fun">print</span>(<span class="tzd-string">"Welcome to "</span> + name + <span class="tzd-string">"! Ratio: "</span> + ratio + <span class="tzd-string">", Score: "</span> + scores[<span class="tzd-number">0</span>]);
}`
  },

  "02_control_flow": {
    file: "02_control_flow.tzd",
    title: "控制流与循环结构",
    tag: "Control Flow",
    desc: "涵盖 if-else 分支、while 循环以及严格规范的 for 循环与高效 switch-case 模式匹配。",
    rule: "规范警示：for 循环头部首个子句为变量赋值表达式，严禁在此使用 var 声明（正确: for (i = 0; i < n; i++)）。",
    raw: `// 标准 for 循环：外部预先声明控制变量
var sum = 0;
var i = 0;

for (i = 0; i < 1000; i++) {
    if (i % 2 == 0) {
        sum = sum + i;
    }
}

// switch-case 匹配模式
var code = 200;
switch (code) {
    case 200:
        print("Status: OK (HTTP 200)");
        break;
    case 404:
        print("Status: Not Found");
        break;
    default:
        print("Status: Other (" + code + ")");
}`,
    code: `<span class="tzd-comment">// 标准 for 循环：外部预先声明控制变量</span>
<span class="tzd-keyword">var</span> sum = <span class="tzd-number">0</span>;
<span class="tzd-keyword">var</span> i = <span class="tzd-number">0</span>;

<span class="tzd-keyword">for</span> (i = <span class="tzd-number">0</span>; i &lt; <span class="tzd-number">1000</span>; i++) {
    <span class="tzd-keyword">if</span> (i % <span class="tzd-number">2</span> == <span class="tzd-number">0</span>) {
        sum = sum + i;
    }
}

<span class="tzd-comment">// switch-case 匹配模式</span>
<span class="tzd-keyword">var</span> code = <span class="tzd-number">200</span>;
<span class="tzd-keyword">switch</span> (code) {
    <span class="tzd-keyword">case</span> <span class="tzd-number">200</span>:
        <span class="tzd-fun">print</span>(<span class="tzd-string">"Status: OK (HTTP 200)"</span>);
        <span class="tzd-keyword">break</span>;
    <span class="tzd-keyword">case</span> <span class="tzd-number">404</span>:
        <span class="tzd-fun">print</span>(<span class="tzd-string">"Status: Not Found"</span>);
        <span class="tzd-keyword">break</span>;
    <span class="tzd-keyword">default</span>:
        <span class="tzd-fun">print</span>(<span class="tzd-string">"Status: Other ("</span> + code + <span class="tzd-string">")"</span>);
}`
  },

  "03_oop_classes": {
    file: "03_oop_classes.tzd",
    title: "面向对象与继承架构",
    tag: "OOP Paradigm",
    desc: "支持类封装、单继承（extends）、构造函数与 super 继承链，方法调用由虚分发快速执行。",
    rule: "设计规范：构造函数使用类名同名构造 ClassName(params) { ... }，父类构造通过 super(...) 调用；成员使用 var/let/const 声明。",
    raw: `// 声明基类
class TensorLayer {
    var int units;

    TensorLayer(units) {
        this.units = units;
    }

    fun forward(x) {
        return x * 1.5;
    }
}

// 继承与构造函数级联
class Dense extends TensorLayer {
    var float bias;

    Dense(units, bias) {
        super(units);
        this.bias = bias;
    }

    fun forward(x) {
        return x * 1.5 + this.bias;
    }
}

var layer = new Dense(64, 0.05);
print("Layer units: " + layer.units + ", Forward result: " + layer.forward(2.0));`,
    code: `<span class="tzd-comment">// 声明基类</span>
<span class="tzd-keyword">class</span> <span class="tzd-class">TensorLayer</span> {
    <span class="tzd-keyword">var</span> <span class="tzd-type">int</span> units;

    <span class="tzd-fun">TensorLayer</span>(units) {
        <span class="tzd-keyword">this</span>.units = units;
    }

    <span class="tzd-keyword">fun</span> <span class="tzd-fun">forward</span>(x) {
        <span class="tzd-keyword">return</span> x * <span class="tzd-number">1.5</span>;
    }
}

<span class="tzd-comment">// 继承与构造函数级联</span>
<span class="tzd-keyword">class</span> <span class="tzd-class">Dense</span> <span class="tzd-keyword">extends</span> <span class="tzd-class">TensorLayer</span> {
    <span class="tzd-keyword">var</span> <span class="tzd-type">float</span> bias;

    <span class="tzd-fun">Dense</span>(units, bias) {
        <span class="tzd-keyword">super</span>(units);
        <span class="tzd-keyword">this</span>.bias = bias;
    }

    <span class="tzd-keyword">fun</span> <span class="tzd-fun">forward</span>(x) {
        <span class="tzd-keyword">return</span> x * <span class="tzd-number">1.5</span> + <span class="tzd-keyword">this</span>.bias;
    }
}

<span class="tzd-keyword">var</span> layer = <span class="tzd-keyword">new</span> <span class="tzd-class">Dense</span>(<span class="tzd-number">64</span>, <span class="tzd-number">0.05</span>);
<span class="tzd-fun">print</span>(<span class="tzd-string">"Layer units: "</span> + layer.units + <span class="tzd-string">", Forward result: "</span> + layer.<span class="tzd-fun">forward</span>(<span class="tzd-number">2.0</span>));`
  },

  "04_closures_meta": {
    file: "04_closures_meta.tzd",
    title: "静态成员与函数一等公民",
    tag: "Functional & Static",
    desc: "支持类静态方法（static fun）与静态常数（const），支持将函数作为第一公民（First-Class Citizens）传递与回调执行。",
    rule: "设计规范：函数使用 fun 关键字声明，无需标注返回类型；静态方法直接通过 Class.method(...) 访问。",
    raw: `// 1. 类静态方法与常数成员
class MathToolkit {
    const PI = 3.1415926535;

    static fun square(x) {
        return x * x;
    }

    static fun apply(op: function, val) {
        return op(val);
    }
}

// 2. 函数作为一等公民传递与回调
fun cube(n) {
    return n * n * n;
}

print("Square: " + MathToolkit.square(6));
print("Callback Cube: " + MathToolkit.apply(cube, 3));`,
    code: `<span class="tzd-comment">// 1. 类静态方法与常数成员</span>
<span class="tzd-keyword">class</span> <span class="tzd-class">MathToolkit</span> {
    <span class="tzd-keyword">const</span> PI = <span class="tzd-number">3.1415926535</span>;

    <span class="tzd-keyword">static</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">square</span>(x) {
        <span class="tzd-keyword">return</span> x * x;
    }

    <span class="tzd-keyword">static</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">apply</span>(op: <span class="tzd-type">function</span>, val) {
        <span class="tzd-keyword">return</span> op(val);
    }
}

<span class="tzd-comment">// 2. 函数作为一等公民传递与回调</span>
<span class="tzd-keyword">fun</span> <span class="tzd-fun">cube</span>(n) {
    <span class="tzd-keyword">return</span> n * n * n;
}

<span class="tzd-fun">print</span>(<span class="tzd-string">"Square: "</span> + <span class="tzd-class">MathToolkit</span>.<span class="tzd-fun">square</span>(<span class="tzd-number">6</span>));
<span class="tzd-fun">print</span>(<span class="tzd-string">"Callback Cube: "</span> + <span class="tzd-class">MathToolkit</span>.<span class="tzd-fun">apply</span>(cube, <span class="tzd-number">3</span>));`
  },

  "05_libtorch_dl": {
    file: "05_libtorch_dl.tzd",
    title: "LibTorch 深度学习第一公民",
    tag: "AI & LibTorch",
    desc: "将 PyTorch C++ (LibTorch) 作为核心第一公民内建于 stdlib/torch/，提供张量、矩阵运算、神经网络层及权重导出。",
    rule: "性能突破：彻底消除 Python-to-C++ 的跨语言装箱开销，无需 Python 运行时即可原生推理与训练。",
    raw: `// 创建张量与执行矩阵乘法
var x = torch_randn(3, 4);
var w = torch_ones(4, 2);
var pred = torch_matmul(x, w);
var act = torch_relu(pred);

// 构建 Sequential 神经网络拓扑
var net = new Sequential();
net.add(new Linear(64, 32));
net.add(new ReLU());
net.add(new Linear(32, 2));

// 导出与持久化模型权重
net.save("model_checkpoint.pt");
print("Inference model exported with zero Python runtime dependency.");`,
    code: `<span class="tzd-comment">// 创建张量与执行矩阵乘法</span>
<span class="tzd-keyword">var</span> x = <span class="tzd-fun">torch_randn</span>(<span class="tzd-number">3</span>, <span class="tzd-number">4</span>);
<span class="tzd-keyword">var</span> w = <span class="tzd-fun">torch_ones</span>(<span class="tzd-number">4</span>, <span class="tzd-number">2</span>);
<span class="tzd-keyword">var</span> pred = <span class="tzd-fun">torch_matmul</span>(x, w);
<span class="tzd-keyword">var</span> act = <span class="tzd-fun">torch_relu</span>(pred);

<span class="tzd-comment">// 构建 Sequential 神经网络拓扑</span>
<span class="tzd-keyword">var</span> net = <span class="tzd-keyword">new</span> <span class="tzd-class">Sequential</span>();
net.<span class="tzd-fun">add</span>(<span class="tzd-keyword">new</span> <span class="tzd-class">Linear</span>(<span class="tzd-number">64</span>, <span class="tzd-number">32</span>));
net.<span class="tzd-fun">add</span>(<span class="tzd-keyword">new</span> <span class="tzd-class">ReLU</span>());
net.<span class="tzd-fun">add</span>(<span class="tzd-keyword">new</span> <span class="tzd-class">Linear</span>(<span class="tzd-number">32</span>, <span class="tzd-number">2</span>));

<span class="tzd-comment">// 导出与持久化模型权重</span>
net.<span class="tzd-fun">save</span>(<span class="tzd-string">"model_checkpoint.pt"</span>);
<span class="tzd-fun">print</span>(<span class="tzd-string">"Inference model exported with zero Python runtime dependency."</span>);`
  },

  "06_multithreading": {
    file: "06_multithreading.tzd",
    title: "原生操作系统多线程 (No GIL)",
    tag: "Concurrency",
    desc: "摆脱传统解释型脚本语言的 GIL（全局解释器锁）瓶颈，标准库 stdlib/thread 映射到底层 OS 原生线程。",
    rule: "并发设计：通过 Thread 类封装工作函数，调用 start() 派生系统线程，join() 完成汇聚等待。",
    raw: `import "stdlib/thread/Thread.tzd";

fun workerTask() {
    var acc = 0;
    var k = 0;
    for (k = 0; k < 500000; k++) {
        acc = acc + (k % 7);
    }
    print("Worker computation finished. Acc: " + acc);
}

// 并行派生原生工作线程
var t1 = new Thread(workerTask);
var t2 = new Thread(workerTask);

t1.start();
t2.start();

t1.join();
t2.join();
print("All native worker threads joined successfully.");`,
    code: `<span class="tzd-keyword">import</span> <span class="tzd-string">"stdlib/thread/Thread.tzd"</span>;

<span class="tzd-keyword">fun</span> <span class="tzd-fun">workerTask</span>() {
    <span class="tzd-keyword">var</span> acc = <span class="tzd-number">0</span>;
    <span class="tzd-keyword">var</span> k = <span class="tzd-number">0</span>;
    <span class="tzd-keyword">for</span> (k = <span class="tzd-number">0</span>; k &lt; <span class="tzd-number">500000</span>; k++) {
        acc = acc + (k % <span class="tzd-number">7</span>);
    }
    <span class="tzd-fun">print</span>(<span class="tzd-string">"Worker computation finished. Acc: "</span> + acc);
}

<span class="tzd-comment">// 并行派生原生工作线程</span>
<span class="tzd-keyword">var</span> t1 = <span class="tzd-keyword">new</span> <span class="tzd-class">Thread</span>(workerTask);
<span class="tzd-keyword">var</span> t2 = <span class="tzd-keyword">new</span> <span class="tzd-class">Thread</span>(workerTask);

t1.<span class="tzd-fun">start</span>();
t2.<span class="tzd-fun">start</span>();

t1.<span class="tzd-fun">join</span>();
t2.<span class="tzd-fun">join</span>();
<span class="tzd-fun">print</span>(<span class="tzd-string">"All native worker threads joined successfully."</span>);`
  },

  "07_exception_match": {
    file: "07_exception_match.tzd",
    title: "结构化异常与类型模式匹配",
    tag: "Error Handling",
    desc: "使用 try-catch-throw 机制捕获异常，并使用 in 关键字执行精确的类型层次模式匹配。",
    rule: "最佳实践：继承自核心 Error 基类，结合模式匹配避免泛型异常吞噬。",
    raw: `import "stdlib/core/Error.tzd";

class NetworkError extends Error {
    NetworkError(msg) {
        super(msg, "NETWORK_ERROR");
    }
}

try {
    var isConnected = false;
    if (!isConnected) {
        throw new NetworkError("Connection timed out (10000ms)");
    }
} catch (err) {
    if (err in NetworkError) {
        print("[Network Recovery]: " + err.message);
    } else {
        print("[Fatal System Error]: Unexpected crash");
    }
}`,
    code: `<span class="tzd-keyword">import</span> <span class="tzd-string">"stdlib/core/Error.tzd"</span>;

<span class="tzd-keyword">class</span> <span class="tzd-class">NetworkError</span> <span class="tzd-keyword">extends</span> <span class="tzd-class">Error</span> {
    <span class="tzd-fun">NetworkError</span>(msg) {
        <span class="tzd-keyword">super</span>(msg, <span class="tzd-string">"NETWORK_ERROR"</span>);
    }
}

<span class="tzd-keyword">try</span> {
    <span class="tzd-keyword">var</span> isConnected = <span class="tzd-keyword">false</span>;
    <span class="tzd-keyword">if</span> (!isConnected) {
        <span class="tzd-keyword">throw</span> <span class="tzd-keyword">new</span> <span class="tzd-class">NetworkError</span>(<span class="tzd-string">"Connection timed out (10000ms)"</span>);
    }
} <span class="tzd-keyword">catch</span> (err) {
    <span class="tzd-keyword">if</span> (err <span class="tzd-keyword">in</span> <span class="tzd-class">NetworkError</span>) {
        <span class="tzd-fun">print</span>(<span class="tzd-string">"[Network Recovery]: "</span> + err.message);
    } <span class="tzd-keyword">else</span> {
        <span class="tzd-fun">print</span>(<span class="tzd-string">"[Fatal System Error]: Unexpected crash"</span>);
    }
}`
  },

  "08_memory_gc": {
    file: "08_memory_gc.tzd",
    title: "Bump Arena 与 SATB 分代 GC",
    tag: "Runtime Internals",
    desc: "双轨内存体系：新生代采用 64KB Bump Pointer Arena 碰撞指针分配，老年代采用带 SATB 写屏障的三色标记清除算法。",
    rule: "技术原理：SATB（原始快照）在并发标记阶段拦截指针写入，确保垃圾回收过程对象引用绝不丢失。",
    raw: `// 1. 新生代：Bump Pointer Arena 快速碰撞分配
class Point {
    var float x;
    var float y;
    Point(x, y) {
        this.x = x;
        this.y = y;
    }
}

var i = 0;
for (i = 0; i < 10000; i++) {
    var pt = new Point(i, i * 2);
}

// 2. 老年代：SATB (Snapshot-At-The-Beginning) 写屏障
print("Arena batch allocated 10000 instances. GC cycle healthy.");`,
    code: `<span class="tzd-comment">// 1. 新生代：Bump Pointer Arena 快速碰撞分配</span>
<span class="tzd-keyword">class</span> <span class="tzd-class">Point</span> {
    <span class="tzd-keyword">var</span> <span class="tzd-type">float</span> x;
    <span class="tzd-keyword">var</span> <span class="tzd-type">float</span> y;
    <span class="tzd-fun">Point</span>(x, y) {
        <span class="tzd-keyword">this</span>.x = x;
        <span class="tzd-keyword">this</span>.y = y;
    }
}

<span class="tzd-keyword">var</span> i = <span class="tzd-number">0</span>;
<span class="tzd-keyword">for</span> (i = <span class="tzd-number">0</span>; i &lt; <span class="tzd-number">10000</span>; i++) {
    <span class="tzd-keyword">var</span> pt = <span class="tzd-keyword">new</span> <span class="tzd-class">Point</span>(i, i * <span class="tzd-number">2</span>);
}

<span class="tzd-comment">// 2. 老年代：SATB (Snapshot-At-The-Beginning) 写屏障</span>
<span class="tzd-fun">print</span>(<span class="tzd-string">"Arena batch allocated 10000 instances. GC cycle healthy."</span>);`
  }
};

// ---------------------------------------------------------------------------
// 2. Workbench Interaction Controller
// ---------------------------------------------------------------------------
let currentTopicKey = "01_variables";

function renderLineNumbers(count) {
  const lineCol = document.getElementById("line-numbers-col");
  if (!lineCol) return;
  let nums = "";
  for (let i = 1; i <= count; i++) {
    nums += i + "<br>";
  }
  lineCol.innerHTML = nums;
}

function switchTopic(topicKey) {
  if (!WORKBENCH_DATA[topicKey]) return;
  currentTopicKey = topicKey;
  const data = WORKBENCH_DATA[topicKey];

  // Update active sidebar pill
  document.querySelectorAll(".nav-item-btn").forEach((btn) => {
    const key = btn.getAttribute("data-topic");
    if (key === topicKey) {
      btn.classList.add("active");
      // Mobile horizontal scroll auto-centering
      if (window.innerWidth <= 860) {
        btn.scrollIntoView({ behavior: "smooth", block: "nearest", inline: "center" });
      }
    } else {
      btn.classList.remove("active");
    }
  });

  // Crossfade code block transition (0.2s)
  const codePane = document.getElementById("workbench-code-pane");
  if (codePane) {
    codePane.classList.add("fade-out");

    setTimeout(() => {
      // Update DOM
      document.getElementById("active-filename").textContent = data.file;
      document.getElementById("doc-title").textContent = data.title;
      document.getElementById("doc-tag").textContent = data.tag;
      document.getElementById("doc-desc").textContent = data.desc;
      document.getElementById("doc-rule").textContent = data.rule;

      codePane.innerHTML = data.code;
      const lineCount = data.raw.split("\n").length;
      renderLineNumbers(lineCount);

      // Fade in
      codePane.classList.remove("fade-out");
    }, 100);
  }
}

// ---------------------------------------------------------------------------
// 3. One-Click Copy Code
// ---------------------------------------------------------------------------
function initCopyButton() {
  const copyBtn = document.getElementById("btn-copy-active-code");
  if (!copyBtn) return;

  copyBtn.addEventListener("click", async () => {
    const data = WORKBENCH_DATA[currentTopicKey];
    if (!data) return;

    try {
      await navigator.clipboard.writeText(data.raw);
      const originalText = copyBtn.innerHTML;
      copyBtn.innerHTML = `
        <svg xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="#34D399" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg>
        <span style="color:#34D399;">已复制</span>
      `;
      setTimeout(() => {
        copyBtn.innerHTML = originalText;
      }, 2000);
    } catch (err) {
      console.error("Failed to copy code: ", err);
    }
  });
}

// ---------------------------------------------------------------------------
// 4. Mobile Navigation Drawer Controller
// ---------------------------------------------------------------------------
function initMobileNavigation() {
  const menuBtn = document.getElementById("mobile-menu-toggle");
  const drawer = document.getElementById("mobile-nav-drawer");
  if (!menuBtn || !drawer) return;

  const toggleDrawer = () => {
    const isOpen = drawer.classList.contains("open");
    if (isOpen) {
      drawer.classList.remove("open");
      menuBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" x2="20" y1="12" y2="12"></line><line x1="4" x2="20" y1="6" y2="6"></line><line x1="4" x2="20" y1="18" y2="18"></line></svg>`;
    } else {
      drawer.classList.add("open");
      menuBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" x2="6" y1="6" y2="18"></line><line x1="6" x2="18" y1="6" y2="18"></line></svg>`;
    }
  };

  menuBtn.addEventListener("click", (e) => {
    e.stopPropagation();
    toggleDrawer();
  });

  // Close when clicking nav items inside drawer
  drawer.querySelectorAll("a").forEach((link) => {
    link.addEventListener("click", () => {
      drawer.classList.remove("open");
      menuBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" x2="20" y1="12" y2="12"></line><line x1="4" x2="20" y1="6" y2="6"></line><line x1="4" x2="20" y1="18" y2="18"></line></svg>`;
    });
  });

  // Close when clicking outside
  document.addEventListener("click", (e) => {
    if (drawer.classList.contains("open") && !drawer.contains(e.target) && !menuBtn.contains(e.target)) {
      drawer.classList.remove("open");
      menuBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" x2="20" y1="12" y2="12"></line><line x1="4" x2="20" y1="6" y2="6"></line><line x1="4" x2="20" y1="18" y2="18"></line></svg>`;
    }
  });

  // Close on Escape key
  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape" && drawer.classList.contains("open")) {
      drawer.classList.remove("open");
      menuBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="4" x2="20" y1="12" y2="12"></line><line x1="4" x2="20" y1="6" y2="6"></line><line x1="4" x2="20" y1="18" y2="18"></line></svg>`;
    }
  });
}

// ---------------------------------------------------------------------------
// 5. Toast Notification Utility
// ---------------------------------------------------------------------------
function showToast(message, type = "info") {
  let container = document.getElementById("toast-container");
  if (!container) {
    container = document.createElement("div");
    container.id = "toast-container";
    container.className = "toast-container";
    document.body.appendChild(container);
  }

  const toast = document.createElement("div");
  toast.className = "toast-item";
  
  const iconSvg = type === "success" 
    ? `<svg class="toast-icon success" xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path><polyline points="22 4 12 14.01 9 11.01"></polyline></svg>`
    : `<svg class="toast-icon" xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>`;

  toast.innerHTML = `${iconSvg}<span>${message}</span>`;
  container.appendChild(toast);

  // Trigger animation
  requestAnimationFrame(() => {
    toast.classList.add("show");
  });

  setTimeout(() => {
    toast.classList.remove("show");
    toast.classList.add("hide");
    setTimeout(() => {
      if (toast.parentNode) {
        toast.parentNode.removeChild(toast);
      }
    }, 280);
  }, 3200);
}

// ---------------------------------------------------------------------------
// 6. Physics Smooth Scrolling & Motion (Lenis + GSAP 3)
// ---------------------------------------------------------------------------
let lenisInstance = null;

function initSmoothScroll() {
  if (typeof Lenis !== "undefined") {
    try {
      lenisInstance = new Lenis({
        duration: 1.2,
        easing: (t) => Math.min(1, 1.001 - Math.pow(2, -10 * t)),
        orientation: "vertical",
        gestureOrientation: "vertical",
        smoothWheel: true,
        wheelMultiplier: 1.0,
        touchMultiplier: 1.5,
        infinite: false,
      });

      // Synchronize Lenis with GSAP ScrollTrigger if both are present
      if (typeof gsap !== "undefined" && typeof ScrollTrigger !== "undefined") {
        lenisInstance.on("scroll", ScrollTrigger.update);
        gsap.ticker.add((time) => {
          lenisInstance.raf(time * 1000);
        });
        gsap.ticker.lagSmoothing(0);
      } else {
        function raf(time) {
          lenisInstance.raf(time);
          requestAnimationFrame(raf);
        }
        requestAnimationFrame(raf);
      }

      // Physics smooth scrolling for in-page anchors
      document.querySelectorAll('a[href^="#"]').forEach((anchor) => {
        anchor.addEventListener("click", function (e) {
          const href = this.getAttribute("href");
          if (!href || href === "#") return;
          const target = document.querySelector(href);
          if (target) {
            e.preventDefault();
            lenisInstance.scrollTo(target, { offset: -68, duration: 1.2 });
          }
        });
      });
    } catch (e) {
      console.warn("Lenis initialization skipped:", e);
    }
  }
}

function initScrollAnimations() {
  const cards = document.querySelectorAll(".download-cards-grid .dl-card");
  if (!cards.length) return;

  if (typeof gsap !== "undefined" && typeof ScrollTrigger !== "undefined") {
    gsap.registerPlugin(ScrollTrigger);

    gsap.from(".download-cards-grid .dl-card", {
      scrollTrigger: {
        trigger: ".download-cards-grid",
        start: "top 85%",
        toggleActions: "play none none none",
      },
      y: 40,
      scale: 0.96,
      opacity: 0,
      duration: 0.85,
      stagger: 0.12,
      ease: "power3.out",
    });

    if (document.querySelector(".hero-stats-row")) {
      gsap.from(".hero-stats-row .stat-card", {
        scrollTrigger: {
          trigger: ".hero-stats-row",
          start: "top 92%",
        },
        y: 20,
        opacity: 0,
        duration: 0.65,
        stagger: 0.08,
        ease: "power2.out",
      });
    }
  } else {
    // IntersectionObserver fallback for offline or CDN unreachability
    cards.forEach((c) => {
      c.style.opacity = "0";
      c.style.transform = "translateY(40px) scale(0.96)";
      c.style.transition = "opacity 0.85s cubic-bezier(0.16, 1, 0.3, 1), transform 0.85s cubic-bezier(0.16, 1, 0.3, 1)";
    });

    const observer = new IntersectionObserver((entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          cards.forEach((c, idx) => {
            setTimeout(() => {
              c.style.opacity = "1";
              c.style.transform = "translateY(0) scale(1)";
            }, idx * 120);
          });
          observer.disconnect();
        }
      });
    }, { threshold: 0.15 });

    const grid = document.querySelector(".download-cards-grid");
    if (grid) observer.observe(grid);
  }
}

// ---------------------------------------------------------------------------
// 7. Radial Gradient Spotlight Pointer Tracking (Apple / Linear / Raycast)
// ---------------------------------------------------------------------------
function initSpotlightCards() {
  const cards = document.querySelectorAll(".spotlight-card, .dl-card");
  cards.forEach((card) => {
    card.addEventListener("mousemove", (e) => {
      const rect = card.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;
      card.style.setProperty("--mouse-x", `${x}px`);
      card.style.setProperty("--mouse-y", `${y}px`);
    });
  });
}

// ---------------------------------------------------------------------------
// 7. Interactive Background Canvas Particle System
// ---------------------------------------------------------------------------
function initBackgroundParticles() {
  const canvas = document.getElementById("bg-canvas");
  if (!canvas) return;

  const ctx = canvas.getContext("2d");
  let width, height;
  let particles = [];
  const particleCount = 45;
  const maxDistance = 140;
  let mouse = { x: null, y: null, radius: 150 };

  function resize() {
    width = canvas.width = window.innerWidth;
    height = canvas.height = window.innerHeight;
  }

  window.addEventListener("resize", resize);
  resize();

  window.addEventListener("mousemove", (e) => {
    mouse.x = e.clientX;
    mouse.y = e.clientY;
  });

  window.addEventListener("mouseout", () => {
    mouse.x = null;
    mouse.y = null;
  });

  class Particle {
    constructor() {
      this.x = Math.random() * width;
      this.y = Math.random() * height;
      this.vx = (Math.random() - 0.5) * 0.45;
      this.vy = (Math.random() - 0.5) * 0.45;
      this.radius = Math.random() * 1.6 + 0.8;
      this.color = Math.random() > 0.4 ? "#38BDF8" : "#818CF8";
    }

    update() {
      this.x += this.vx;
      this.y += this.vy;

      if (this.x < 0 || this.x > width) this.vx = -this.vx;
      if (this.y < 0 || this.y > height) this.vy = -this.vy;

      // Mouse gentle interaction
      if (mouse.x != null && mouse.y != null) {
        const dx = mouse.x - this.x;
        const dy = mouse.y - this.y;
        const dist = Math.sqrt(dx * dx + dy * dy);
        if (dist < mouse.radius) {
          const force = (mouse.radius - dist) / mouse.radius;
          this.x -= (dx / dist) * force * 1.5;
          this.y -= (dy / dist) * force * 1.5;
        }
      }
    }

    draw() {
      ctx.beginPath();
      ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
      ctx.fillStyle = this.color;
      ctx.shadowBlur = 8;
      ctx.shadowColor = this.color;
      ctx.fill();
    }
  }

  for (let i = 0; i < particleCount; i++) {
    particles.push(new Particle());
  }

  function animate() {
    ctx.clearRect(0, 0, width, height);

    for (let i = 0; i < particles.length; i++) {
      particles[i].update();
      particles[i].draw();

      for (let j = i + 1; j < particles.length; j++) {
        const dx = particles[i].x - particles[j].x;
        const dy = particles[i].y - particles[j].y;
        const dist = Math.sqrt(dx * dx + dy * dy);

        if (dist < maxDistance) {
          const alpha = (1 - dist / maxDistance) * 0.18;
          ctx.beginPath();
          ctx.moveTo(particles[i].x, particles[i].y);
          ctx.lineTo(particles[j].x, particles[j].y);
          ctx.strokeStyle = `rgba(56, 189, 248, ${alpha})`;
          ctx.lineWidth = 0.75;
          ctx.stroke();
        }
      }
    }

    requestAnimationFrame(animate);
  }

  animate();
}

// ---------------------------------------------------------------------------
// 8. Download & Command Interactivity
// ---------------------------------------------------------------------------
function initDownloadInteractions() {
  // Download buttons feedback
  document.querySelectorAll(".dl-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      const fileName = btn.getAttribute("data-file") || "安装程序";
      showToast(`已开始下载 ${fileName}！请在下载完成后运行安装。`, "success");
    });
  });

  // Quick command copy
  const copyCmdBtn = document.getElementById("btn-copy-install-cmd");
  if (copyCmdBtn) {
    copyCmdBtn.addEventListener("click", () => {
      const cmdText = document.getElementById("install-cmd-text")?.innerText || "tzd --help";
      navigator.clipboard.writeText(cmdText).then(() => {
        showToast("命令已复制到剪贴板！", "success");
        copyCmdBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#FFFFFF" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="20 6 9 17 4 12"></polyline></svg><span style="color:#FFFFFF;">已复制</span>`;
        setTimeout(() => {
          copyCmdBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg><span>复制代码</span>`;
        }, 2000);
      });
    });
  }
}

// ---------------------------------------------------------------------------
// 9. App Initialization
// ---------------------------------------------------------------------------
document.addEventListener("DOMContentLoaded", () => {
  // Initialize Lenis physics smooth scroll & GSAP motion
  initSmoothScroll();
  initScrollAnimations();

  // Bind workbench sidebar items
  document.querySelectorAll(".nav-item-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      const topicKey = btn.getAttribute("data-topic");
      switchTopic(topicKey);
    });
  });

  // Initial render
  switchTopic("01_variables");
  initCopyButton();
  initMobileNavigation();
  initSpotlightCards();
  initBackgroundParticles();
  initDownloadInteractions();

  if (window.lucide) {
    window.lucide.createIcons();
  }
});

