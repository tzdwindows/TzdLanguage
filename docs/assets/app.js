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
    desc: "支持 var/let/const 动态类型推断，原生深度内建静态强类型（int, float, string, bool, void, ptr/hwnd, type[]）。",
    rule: "规范提示：声明类型使用冒号语法 var x: int = 10; 支持零开销原生指针与数组索引。",
    raw: `// 1. 动态声明与静态强类型推断
var count = 42;
let name: string = "TzdLang";
const PI: float = 3.1415926535;

// 2. 原生数组与底层系统指针
var scores: int[] = [98, 95, 100];
var buffer: ptr = null;

// 3. 混合运算与字符串内建拼接
var isActive: bool = true;
if (isActive) {
    print("Welcome to " + name + "! System counter: " + count);
}`,
    code: `<span class="tzd-comment">// 1. 动态声明与静态强类型推断</span>
<span class="tzd-keyword">var</span> count = <span class="tzd-number">42</span>;
<span class="tzd-keyword">let</span> name: <span class="tzd-type">string</span> = <span class="tzd-string">"TzdLang"</span>;
<span class="tzd-keyword">const</span> PI: <span class="tzd-type">float</span> = <span class="tzd-number">3.1415926535</span>;

<span class="tzd-comment">// 2. 原生数组与底层系统指针</span>
<span class="tzd-keyword">var</span> scores: <span class="tzd-type">int[]</span> = [<span class="tzd-number">98</span>, <span class="tzd-number">95</span>, <span class="tzd-number">100</span>];
<span class="tzd-keyword">var</span> buffer: <span class="tzd-type">ptr</span> = <span class="tzd-keyword">null</span>;

<span class="tzd-comment">// 3. 混合运算与字符串内建拼接</span>
<span class="tzd-keyword">var</span> isActive: <span class="tzd-type">bool</span> = <span class="tzd-keyword">true</span>;
<span class="tzd-keyword">if</span> (isActive) {
    <span class="tzd-fun">print</span>(<span class="tzd-string">"Welcome to "</span> + name + <span class="tzd-string">"! System counter: "</span> + count);
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
    desc: "支持类封装、单继承、访问修饰符（public/private/protected）、构造函数级联 : super(...) 与匿名派生块。",
    rule: "设计原则：基类构造函数调用通过 : super(...) 显式传递，成员变量由 Bump Arena 零碎片对齐分配。",
    raw: `// 声明基类
class TensorLayer {
    protected var units: int;

    public fun TensorLayer(units: int) {
        this.units = units;
    }

    public fun forward(x: float): float {
        return x * 1.5;
    }
}

// 继承与构造函数级联
class Dense extends TensorLayer {
    private var bias: float;

    public fun Dense(units: int, bias: float) : super(units) {
        this.bias = bias;
    }

    public fun forward(x: float): float {
        return super.forward(x) + this.bias;
    }
}

var layer = new Dense(64, 0.05);
print("Forward result: " + layer.forward(2.0));`,
    code: `<span class="tzd-comment">// 声明基类</span>
<span class="tzd-keyword">class</span> <span class="tzd-class">TensorLayer</span> {
    <span class="tzd-keyword">protected</span> <span class="tzd-keyword">var</span> units: <span class="tzd-type">int</span>;

    <span class="tzd-keyword">public</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">TensorLayer</span>(units: <span class="tzd-type">int</span>) {
        <span class="tzd-keyword">this</span>.units = units;
    }

    <span class="tzd-keyword">public</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">forward</span>(x: <span class="tzd-type">float</span>): <span class="tzd-type">float</span> {
        <span class="tzd-keyword">return</span> x * <span class="tzd-number">1.5</span>;
    }
}

<span class="tzd-comment">// 继承与构造函数级联</span>
<span class="tzd-keyword">class</span> <span class="tzd-class">Dense</span> <span class="tzd-keyword">extends</span> <span class="tzd-class">TensorLayer</span> {
    <span class="tzd-keyword">private</span> <span class="tzd-keyword">var</span> bias: <span class="tzd-type">float</span>;

    <span class="tzd-keyword">public</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">Dense</span>(units: <span class="tzd-type">int</span>, bias: <span class="tzd-type">float</span>) : <span class="tzd-keyword">super</span>(units) {
        <span class="tzd-keyword">this</span>.bias = bias;
    }

    <span class="tzd-keyword">public</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">forward</span>(x: <span class="tzd-type">float</span>): <span class="tzd-type">float</span> {
        <span class="tzd-keyword">return</span> <span class="tzd-keyword">super</span>.<span class="tzd-fun">forward</span>(x) + <span class="tzd-keyword">this</span>.bias;
    }
}

<span class="tzd-keyword">var</span> layer = <span class="tzd-keyword">new</span> <span class="tzd-class">Dense</span>(<span class="tzd-number">64</span>, <span class="tzd-number">0.05</span>);
<span class="tzd-fun">print</span>(<span class="tzd-string">"Forward result: "</span> + layer.<span class="tzd-fun">forward</span>(<span class="tzd-number">2.0</span>));`
  },

  "04_closures_meta": {
    file: "04_closures_meta.tzd",
    title: "一等闭包与元编程注解",
    tag: "Metaprogramming",
    desc: "函数作为一等公民（First-Class Citizens），原生支持匿名 Lambda、环境捕获闭包、类/方法级元编程注解与枚举声明。",
    rule: "底层支持：闭包上下文由逃逸分析自动决定分配于快速执行栈或持久化堆区。",
    raw: `// 1. 高阶函数与捕获闭包
fun makeMultiplier(factor: int) {
    return fun(val: int) {
        return val * factor;
    };
}

var triple = makeMultiplier(3);
print("Triple of 9: " + triple(9)); // 27

// 2. 枚举定义与元编程注解
enum ComputeBackend { CPU, CUDA, MPS }

class @Optimized(tier = 1)
class Pipeline {
    @Route(path = "/predict")
    public fun predict(backend: ComputeBackend) {
        print("Executing on backend: " + backend);
    }
}`,
    code: `<span class="tzd-comment">// 1. 高阶函数与捕获闭包</span>
<span class="tzd-keyword">fun</span> <span class="tzd-fun">makeMultiplier</span>(factor: <span class="tzd-type">int</span>) {
    <span class="tzd-keyword">return</span> <span class="tzd-keyword">fun</span>(val: <span class="tzd-type">int</span>) {
        <span class="tzd-keyword">return</span> val * factor;
    };
}

<span class="tzd-keyword">var</span> triple = <span class="tzd-fun">makeMultiplier</span>(<span class="tzd-number">3</span>);
<span class="tzd-fun">print</span>(<span class="tzd-string">"Triple of 9: "</span> + <span class="tzd-fun">triple</span>(<span class="tzd-number">9</span>)); <span class="tzd-comment">// 27</span>

<span class="tzd-comment">// 2. 枚举定义与元编程注解</span>
<span class="tzd-keyword">enum</span> <span class="tzd-class">ComputeBackend</span> { CPU, CUDA, MPS }

<span class="tzd-keyword">class</span> <span class="tzd-annot">@Optimized</span>(tier = <span class="tzd-number">1</span>)
<span class="tzd-keyword">class</span> <span class="tzd-class">Pipeline</span> {
    <span class="tzd-annot">@Route</span>(path = <span class="tzd-string">"/predict"</span>)
    <span class="tzd-keyword">public</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">predict</span>(backend: <span class="tzd-class">ComputeBackend</span>) {
        <span class="tzd-fun">print</span>(<span class="tzd-string">"Executing on backend: "</span> + backend);
    }
}`
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
var out = torch_matmul(x, w);
var act = torch_relu(out);

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
<span class="tzd-keyword">var</span> out = <span class="tzd-fun">torch_matmul</span>(x, w);
<span class="tzd-keyword">var</span> act = <span class="tzd-fun">torch_relu</span>(out);

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
    desc: "摆脱传统解释型脚本语言的 GIL（全局解释器锁）瓶颈，映射到底层 OS 原生线程并提供高效线程池调度。",
    rule: "并发设计：各线程独立运行于无锁内存分配泳道中，数据汇合处支持高效原子操作与 join() 同步。",
    raw: `fun workerTask(id: int) {
    var acc = 0;
    var k = 0;
    for (k = 0; k < 500000; k++) {
        acc = acc + k;
    }
    print("Worker [" + id + "] computation finished. Result: " + acc);
}

// 并行派生 4 个原生工作线程
var t1 = new Thread(workerTask, 1);
var t2 = new Thread(workerTask, 2);
var t3 = new Thread(workerTask, 3);
var t4 = new Thread(workerTask, 4);

t1.join();
t2.join();
t3.join();
t4.join();
print("All native worker threads joined successfully.");`,
    code: `<span class="tzd-keyword">fun</span> <span class="tzd-fun">workerTask</span>(id: <span class="tzd-type">int</span>) {
    <span class="tzd-keyword">var</span> acc = <span class="tzd-number">0</span>;
    <span class="tzd-keyword">var</span> k = <span class="tzd-number">0</span>;
    <span class="tzd-keyword">for</span> (k = <span class="tzd-number">0</span>; k &lt; <span class="tzd-number">500000</span>; k++) {
        acc = acc + k;
    }
    <span class="tzd-fun">print</span>(<span class="tzd-string">"Worker ["</span> + id + <span class="tzd-string">"] computation finished. Result: "</span> + acc);
}

<span class="tzd-comment">// 并行派生 4 个原生工作线程</span>
<span class="tzd-keyword">var</span> t1 = <span class="tzd-keyword">new</span> <span class="tzd-class">Thread</span>(workerTask, <span class="tzd-number">1</span>);
<span class="tzd-keyword">var</span> t2 = <span class="tzd-keyword">new</span> <span class="tzd-class">Thread</span>(workerTask, <span class="tzd-number">2</span>);
<span class="tzd-keyword">var</span> t3 = <span class="tzd-keyword">new</span> <span class="tzd-class">Thread</span>(workerTask, <span class="tzd-number">3</span>);
<span class="tzd-keyword">var</span> t4 = <span class="tzd-keyword">new</span> <span class="tzd-class">Thread</span>(workerTask, <span class="tzd-number">4</span>);

t1.<span class="tzd-fun">join</span>();
t2.<span class="tzd-fun">join</span>();
t3.<span class="tzd-fun">join</span>();
t4.<span class="tzd-fun">join</span>();
<span class="tzd-fun">print</span>(<span class="tzd-string">"All native worker threads joined successfully."</span>);`
  },

  "07_exception_match": {
    file: "07_exception_match.tzd",
    title: "结构化异常与类型模式匹配",
    tag: "Error Handling",
    desc: "使用 try-catch-throw 机制捕获异常，并使用 in 关键字执行精确的类型层次模式匹配。",
    rule: "最佳实践：继承自核心 Error 基类，结合模式匹配避免泛型异常吞噬。",
    raw: `class NetworkError extends Error {
    public fun NetworkError(msg: string) : super(msg) {}
}

try {
    var client = connectPeer("192.168.1.100", 9000);
    if (client == null) {
        throw new NetworkError("Connection timed out (10000ms)");
    }
} catch (err) {
    if (err in NetworkError) {
        print("[Network Recovery]: " + err.message);
    } else {
        print("[Fatal System Error]: Unexpected crash");
    }
}`,
    code: `<span class="tzd-keyword">class</span> <span class="tzd-class">NetworkError</span> <span class="tzd-keyword">extends</span> <span class="tzd-class">Error</span> {
    <span class="tzd-keyword">public</span> <span class="tzd-keyword">fun</span> <span class="tzd-fun">NetworkError</span>(msg: <span class="tzd-type">string</span>) : <span class="tzd-keyword">super</span>(msg) {}
}

<span class="tzd-keyword">try</span> {
    <span class="tzd-keyword">var</span> client = <span class="tzd-fun">connectPeer</span>(<span class="tzd-string">"192.168.1.100"</span>, <span class="tzd-number">9000</span>);
    <span class="tzd-keyword">if</span> (client == <span class="tzd-keyword">null</span>) {
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
// 无需遍历空闲链表，O(1) 批量清空，彻底消除碎片
var i = 0;
for (i = 0; i < 100000; i++) {
    var pt = new Point(i, i * 2);
}

// 2. 老年代：SATB (Snapshot-At-The-Beginning) 写屏障
// 并发写屏障保护全局长生命周期对象图
var rootRegistry = new GlobalRegistry();
rootRegistry.bind("engine", new InferenceEngine());
print("Arena batch cleaned. SATB concurrent cycle completed.");`,
    code: `<span class="tzd-comment">// 1. 新生代：Bump Pointer Arena 快速碰撞分配</span>
<span class="tzd-comment">// 无需遍历空闲链表，O(1) 批量清空，彻底消除碎片</span>
<span class="tzd-keyword">var</span> i = <span class="tzd-number">0</span>;
<span class="tzd-keyword">for</span> (i = <span class="tzd-number">0</span>; i &lt; <span class="tzd-number">100000</span>; i++) {
    <span class="tzd-keyword">var</span> pt = <span class="tzd-keyword">new</span> <span class="tzd-class">Point</span>(i, i * <span class="tzd-number">2</span>);
}

<span class="tzd-comment">// 2. 老年代：SATB (Snapshot-At-The-Beginning) 写屏障</span>
<span class="tzd-comment">// 并发写屏障保护全局长生命周期对象图</span>
<span class="tzd-keyword">var</span> rootRegistry = <span class="tzd-keyword">new</span> <span class="tzd-class">GlobalRegistry</span>();
rootRegistry.<span class="tzd-fun">bind</span>(<span class="tzd-string">"engine"</span>, <span class="tzd-keyword">new</span> <span class="tzd-class">InferenceEngine</span>());
<span class="tzd-fun">print</span>(<span class="tzd-string">"Arena batch cleaned. SATB concurrent cycle completed."</span>);`
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
// 5. App Initialization
// ---------------------------------------------------------------------------
document.addEventListener("DOMContentLoaded", () => {
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

  if (window.lucide) {
    window.lucide.createIcons();
  }
});
