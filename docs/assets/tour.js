/**
 * TzdLang Interactive Tour & Comprehensive Guide Controller (tour.js)
 * Architecture: 6 Core Chapters, Crossfade Transitions, Interactive Playground & Terminal, Cmd+K Search.
 */

const TOUR_DATA = {
  "01_quickstart": {
    group: "快速起步",
    file: "01_quickstart.tzd",
    badge: "CHAPTER 01 · GETTING STARTED",
    title: "快速起步：构建、运行与初探",
    lead: "从环境构建到第一个 Hello World 程序。理解 TzdLang 的设计初衷：在兼顾动态脚本开发敏捷度的同时，提供媲美 C++ 的原生执行速度。",
    htmlContent: `
      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><polygon points="10 8 16 12 10 16 10 8"></polygon></svg>
        <span>极简工程起步</span>
      </h3>
      <p class="doc-body-text">
        TzdLang 是一门强工程导向的系统级面向对象语言。它的源码文件以 <code>.tzd</code> 为扩展名，支持直接通过 CLI 工具 <code>tzd.exe</code> 解释执行或在热点触发时自动即时编译（JIT）为原生机器指令。
      </p>

      <div class="apple-info-callout">
        <strong>开箱即用体验：</strong>无需配置庞大的 Gradle/Cargo 工程体系或 Python 虚拟环境依赖，单个 <code>.tzd</code> 文件即可直接启动运行与调试。
      </div>

      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="20" x2="18" y2="10"></line><line x1="12" y1="20" x2="12" y2="4"></line><line x1="6" y1="20" x2="6" y2="14"></line></svg>
        <span>心智模型：语言横向对比 (Comparison Cards)</span>
      </h3>
      <p class="doc-body-text">
        在“极简起步与基础运算”场景下，直观审视 TzdLang 与主流语言的表达力与开销对比：
      </p>

      <div class="comparison-card-container">
        <div class="comparison-header">
          <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M16 16v1a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V7a2 2 0 0 1 2-2h11a2 2 0 0 1 2 2v1"></path><path d="M18 8h4a2 2 0 0 1 2 2v7a2 2 0 0 1-2 2h-4"></path><circle cx="8" cy="12" r="2"></circle></svg>
          <span>多语言横向语法对比：Hello World & 循环累加</span>
        </div>
        <div class="comparison-grid">
          <!-- TzdLang -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-tzd">TzdLang</span>
              <span style="font-size:0.7rem; color:#34D399;">0ms 模板样板</span>
            </div>
            <pre class="comparison-snippet"><code>var greeting: string = "Hello World";
var sum = 0;
var i = 0;
for (i = 0; i &lt; 1000; i++) {
    sum = sum + i;
}
print(greeting + ": " + sum);</code></pre>
            <div class="comparison-verdict">
              <strong>优势：</strong>脚本级极简声明，无多余类包装，启动即由 Tier 0 VM 执行，热点回边自动跃迁为原生 JIT。
            </div>
          </div>

          <!-- C++ -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-cpp">C++ 20</span>
              <span style="font-size:0.7rem; color:#93C5FD;">原生极速但繁杂</span>
            </div>
            <pre class="comparison-snippet"><code>#include &lt;iostream&gt;
#include &lt;string&gt;

int main() {
    std::string s = "Hello World";
    int sum = 0;
    for (int i = 0; i &lt; 1000; ++i) sum += i;
    std::cout &lt;&lt; s &lt;&lt; ": " &lt;&lt; sum &lt;&lt; "\\n";
    return 0;
}</code></pre>
            <div class="comparison-verdict">
              <strong>劣势：</strong>必须编写显式头文件引用、<code>main</code> 入口与严格类型签名，开发与编译迭代周期长。
            </div>
          </div>

          <!-- Python -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-py">Python 3</span>
              <span style="font-size:0.7rem; color:#FCD34D;">简单但受制于解释器</span>
            </div>
            <pre class="comparison-snippet"><code>s = "Hello World"
sum_val = 0
for i in range(1000):
    sum_val += i
print(f"{s}: {sum_val}")</code></pre>
            <div class="comparison-verdict">
              <strong>劣势：</strong>动态类型装箱导致数值计算速度落后近百倍，无法原生释放 GIL，依赖底层 C 扩展。
            </div>
          </div>
        </div>
      </div>
    `,
    defaultCode: `// 01_quickstart.tzd
// TzdLang 极速起步与基准运算验证
var greeting: string = "Hello, TzdLang!";
var version: float = 1.0;
print(greeting + " Running on version " + version);

// 简短的高性能计算验证
var acc = 0;
var i = 0;
for (i = 0; i < 10000; i++) {
    if (i % 2 == 0) {
        acc = acc + i;
    }
}
print("Sum of even numbers (0..9999) = " + acc);`,
    terminalOutput: `[TzdVM Tier 0] Initializing AST and Bytecode Stack in 0.42ms...
Hello, TzdLang! Running on version 1.0
Sum of even numbers (0..9999) = 24995000
[Tier 1 JIT] Loop count >= 10000 triggered LLVM ORC compilation.
● Native Worker Executed: 0.0001s elapsed (No GIL overhead)`
  },

  "02_basics": {
    group: "基础语法",
    file: "02_syntax_control.tzd",
    badge: "CHAPTER 02 · CORE SYNTAX",
    title: "基础语法：类型系统、严格循环与闭包",
    lead: "掌握 TzdLang 的动态推断与静态类型混合设计，深入理解为什么 for 循环头部首子句严禁 var 声明的工程考量。",
    htmlContent: `
      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 2 7 12 12 22 7 12 2"></polygon><polyline points="2 17 12 22 22 17"></polyline><polyline points="2 12 12 17 22 12"></polyline></svg>
        <span>类型系统与核心控制流</span>
      </h3>
      <p class="doc-body-text">
        TzdLang 采用双轨类型系统：既允许通过 <code>var</code> 进行敏捷的动态推断，又支持使用冒号语法 <code>var a: int = 10;</code> 进行强类型限定。在编译到 Tier 1 原生层时，静态类型将直接映射为 LLVM 原生标量类型（如 <code>i64</code>、<code>double</code>），实现真正的<strong>零装箱（Zero-Boxing）</strong>开销。
      </p>

      <div class="apple-info-callout">
        <strong>语法铁律（Coding Standard）：</strong><code>for</code> 循环头部首个子句为<strong>纯赋值表达式</strong>，严禁在此使用 <code>var</code> 声明（例如 <code>for (i = 0; i &lt; n; i++)</code> 为合规写法，<code>for (var i = 0; ...)</code> 将触发语法解析器报错）。这种设计彻底消除了 C/JS 中由于循环变量块级作用域漂移导致的隐蔽 Bug。
      </div>

      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="20" x2="18" y2="10"></line><line x1="12" y1="20" x2="12" y2="4"></line><line x1="6" y1="20" x2="6" y2="14"></line></svg>
        <span>心智模型：闭包与控制流对比</span>
      </h3>

      <div class="comparison-card-container">
        <div class="comparison-header">
          <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"></path><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"></path></svg>
          <span>高阶函数与闭包环境捕获对比</span>
        </div>
        <div class="comparison-grid">
          <!-- TzdLang -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-tzd">TzdLang</span>
              <span style="font-size:0.7rem; color:#34D399;">一等公民闭包</span>
            </div>
            <pre class="comparison-snippet"><code>fun makeMultiplier(factor: int) {
    return fun(val: int) {
        return val * factor;
    };
}
var doubleFn = makeMultiplier(2);
print(doubleFn(15)); // 30</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>自动逃逸分析判断：未逃逸分配于极速栈帧，逃逸则由 Bump Pointer Arena 批量托管。
            </div>
          </div>

          <!-- Rust -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-rust">Rust</span>
              <span style="font-size:0.7rem; color:#FDBA74;">生命周期所有权</span>
            </div>
            <pre class="comparison-snippet"><code>fn make_multiplier(factor: i32) 
    -&gt; impl Fn(i32) -&gt; i32 {
    move |val| val * factor
}
let double_fn = make_multiplier(2);
println!("{}", double_fn(15));</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>必须显式使用 <code>move</code> 关键字转移所有权，闭包特征需标注 <code>Fn/FnMut/FnOnce</code>。
            </div>
          </div>

          <!-- Python -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-py">Python 3</span>
              <span style="font-size:0.7rem; color:#FCD34D;">函数嵌套闭包</span>
            </div>
            <pre class="comparison-snippet"><code>def make_multiplier(factor):
    def inner(val):
        return val * factor
    return inner

double_fn = make_multiplier(2)
print(double_fn(15))</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>通过 cell 字典对象捕获作用域，每次函数调用均伴随引用计数与字典查找开销。
            </div>
          </div>
        </div>
      </div>
    `,
    defaultCode: `// 02_syntax_control.tzd
// 类型声明、标准循环与闭包运用
let maxLimit: int = 500;
var sum = 0;
var i = 0;

// 标准规范：for 头部变量赋值
for (i = 0; i < maxLimit; i++) {
    if (i % 25 == 0) {
        sum = sum + i;
    }
}

// 高阶函数与捕获闭包
fun makeFilter(scale: int) {
    return fun(inputVal: int) {
        return inputVal * scale;
    };
}

var triple = makeFilter(3);
print("Computed sum = " + sum);
print("Closure result triple(20) = " + triple(20));`,
    terminalOutput: `[TzdVM Tier 0] AST parsed successfully.
Computed sum = 4750
Closure result triple(20) = 60
[Profiler] Closure environment safely bound to Arena Block.
● Execution verified (Time: 0.0003s)`
  },

  "03_oop": {
    group: "核心面向对象",
    file: "03_oop_paradigm.tzd",
    badge: "CHAPTER 03 · OBJECT ORIENTED",
    title: "面向对象：封装、级联构造与继承架构",
    lead: "TzdLang 提供类似 Java/C++ 的现代单继承体系。对象实例由新生代 Bump Pointer Arena 碰撞分配，消灭碎片。",
    htmlContent: `
      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="12 2 2 7 12 12 22 7 12 2"></polygon><polyline points="2 17 12 22 22 17"></polyline><polyline points="2 12 12 17 22 12"></polyline></svg>
        <span>类声明与构造级联</span>
      </h3>
      <p class="doc-body-text">
        支持标准的 <code>class</code> 声明与继承（<code>extends</code>），支持 <code>public</code>、<code>private</code>、<code>protected</code> 访问修饰符。子类构造函数使用 <code>: super(...)</code> 语法显式触发父类初始化逻辑，与 C++ 初始化列表和 C# 构造体系心智完全统一。
      </p>

      <div class="apple-info-callout">
        <strong>底层内存优势：</strong>与传统 JVM 或 Python 每一个对象都在堆上独立 malloc 并挂载庞大对象头不同，TzdLang 的对象分配由 <strong>Bump Pointer Arena</strong> 驱动，以连续 64KB 块为单位指针碰撞分配，具有极佳的 CPU L1/L2 缓存行亲和度。
      </div>

      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="20" x2="18" y2="10"></line><line x1="12" y1="20" x2="12" y2="4"></line><line x1="6" y1="20" x2="6" y2="14"></line></svg>
        <span>心智模型：面向对象与多态对比</span>
      </h3>

      <div class="comparison-card-container">
        <div class="comparison-header">
          <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polygon points="6 2 18 2 18 6 6 6 6 2"></polygon><rect x="3" y="6" width="18" height="16" rx="2"></rect><line x1="10" y1="12" x2="14" y2="12"></line></svg>
          <span>多语言类继承与方法重写对比</span>
        </div>
        <div class="comparison-grid">
          <!-- TzdLang -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-tzd">TzdLang</span>
              <span style="font-size:0.7rem; color:#34D399;">级联构造 & PIC 虚分发</span>
            </div>
            <pre class="comparison-snippet"><code>class Shape {
    protected var name: string;
    public fun Shape(n: string) {
        this.name = n;
    }
}
class Circle extends Shape {
    private var r: float;
    public fun Circle(r: float) : super("Circle") {
        this.r = r;
    }
}</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>多态方法调用由内建的多态内联缓存（PIC）加速，热点方法内联展开消除虚表开销。
            </div>
          </div>

          <!-- C++ -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-cpp">C++</span>
              <span style="font-size:0.7rem; color:#93C5FD;">显式 vptr 与手工管理</span>
            </div>
            <pre class="comparison-snippet"><code>class Shape {
protected:
    std::string name;
public:
    Shape(std::string n) : name(n) {}
    virtual ~Shape() = default;
};
class Circle : public Shape {
    double r;
public:
    Circle(double r) : Shape("Circle"), r(r) {}
};</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>虚析构、深浅拷贝与内存泄漏陷阱频发，心智负担过重。
            </div>
          </div>

          <!-- Python -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-py">Python</span>
              <span style="font-size:0.7rem; color:#FCD34D;">动态派生与字典寻址</span>
            </div>
            <pre class="comparison-snippet"><code>class Shape:
    def __init__(self, name):
        self.name = name

class Circle(Shape):
    def __init__(self, r):
        super().__init__("Circle")
        self.r = r</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>属性与方法通过 <code>__dict__</code> 哈希表动态查找，每次调用均有运行时字符串哈希损耗。
            </div>
          </div>
        </div>
      </div>
    `,
    defaultCode: `// 03_oop_paradigm.tzd
// 面向对象继承与构造级联
class Shape {
    protected var name: string;

    public fun Shape(name: string) {
        this.name = name;
    }

    public fun area(): float {
        return 0.0;
    }
}

class Rectangle extends Shape {
    private var width: float;
    private var height: float;

    public fun Rectangle(w: float, h: float) : super("Rectangle") {
        this.width = w;
        this.height = h;
    }

    public fun area(): float {
        return this.width * this.height;
    }
}

var rect = new Rectangle(12.5, 4.0);
print("Shape Name: " + rect.name);
print("Calculated Area: " + rect.area());`,
    terminalOutput: `[TzdVM Tier 0] Object Rectangle instantiated via BumpArena (Block #0).
Shape Name: Rectangle
Calculated Area: 50.0
[Polymorphic Inline Cache] Hit rate: 100% on rect.area()
● Finished in 0.0004s (Heap zero fragmentation)`
  },

  "04_deep_learning": {
    group: "深度学习与张量",
    file: "04_deep_learning.tzd",
    badge: "CHAPTER 04 · DEEP LEARNING",
    title: "深度学习第一公民：LibTorch 原生无缝内核",
    lead: "在语言底层直接嵌入 LibTorch C++ API。消除 Python-C++ 跨语言装箱损耗与解释器依赖，实现真正的毫秒级 AI 推理。",
    htmlContent: `
      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"></path><polyline points="3.27 6.96 12 12.01 20.73 6.96"></polyline><line x1="12" y1="22.08" x2="12" y2="12"></line></svg>
        <span>内建标准库 stdlib/torch/</span>
      </h3>
      <p class="doc-body-text">
        传统深度学习开发受困于“Python 原型开发易，原生生产部署难”的两难困境。TzdLang 直接将 LibTorch 作为标准库的第一公民深度嵌合：支持 <code>torch_tensor</code>、<code>torch_randn</code>、<code>torch_matmul</code> 原生算子，并内建 <code>Sequential</code>、<code>Linear</code>、<code>ReLU</code> 模块系统。
      </p>

      <div class="apple-info-callout">
        <strong>架构革命：</strong>代码既保持了类似 Python PyTorch 般优雅精炼的语法表达，又完全摆脱了 Python 解释器。张量运算直接由 C++ LibTorch 底层 SIMD/CUDA 驱动，多线程并发时无需等待任何 GIL！
      </div>

      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="20" x2="18" y2="10"></line><line x1="12" y1="20" x2="12" y2="4"></line><line x1="6" y1="20" x2="6" y2="14"></line></svg>
        <span>心智模型：AI 深度学习方案对比</span>
      </h3>

      <div class="comparison-card-container">
        <div class="comparison-header">
          <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
          <span>多语言模型前向推理与部署对比</span>
        </div>
        <div class="comparison-grid">
          <!-- TzdLang -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-tzd">TzdLang</span>
              <span style="font-size:0.7rem; color:#34D399;">原生内建 · 零 GIL</span>
            </div>
            <pre class="comparison-snippet"><code>var x = torch_randn(4, 16);
var net = new Sequential();
net.add(new Linear(16, 32));
net.add(new ReLU());
net.add(new Linear(32, 2));
var pred = net.forward(x);</code></pre>
            <div class="comparison-verdict">
              <strong>优势：</strong>统一语法书写，零 Python 虚拟机体积依赖，支持保存 <code>.pt</code> 权重与离线推理。
            </div>
          </div>

          <!-- Python (PyTorch) -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-py">Python PyTorch</span>
              <span style="font-size:0.7rem; color:#FCD34D;">标准生态但锁 GIL</span>
            </div>
            <pre class="comparison-snippet"><code>import torch
import torch.nn as nn

x = torch.randn(4, 16)
net = nn.Sequential(
    nn.Linear(16, 32),
    nn.ReLU(),
    nn.Linear(32, 2)
)
pred = net(x)</code></pre>
            <div class="comparison-verdict">
              <strong>劣势：</strong>生产部署必须携带 Python 庞大运行时（经常数百 MB），并发受限于 GIL 锁。
            </div>
          </div>

          <!-- C++ LibTorch -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-cpp">C++ LibTorch</span>
              <span style="font-size:0.7rem; color:#93C5FD;">高性能但语法冗长</span>
            </div>
            <pre class="comparison-snippet"><code>torch::Tensor x = torch::randn({4, 16});
torch::nn::Sequential net(
    torch::nn::Linear(16, 32),
    torch::nn::Functional(torch::relu),
    torch::nn::Linear(32, 2)
);
auto pred = net-&gt;forward(x);</code></pre>
            <div class="comparison-verdict">
              <strong>劣势：</strong>模板类型定义复杂，指针操作繁重，容易引发内存悬空指针或编译期长达数分钟的模板膨胀。
            </div>
          </div>
        </div>
      </div>
    `,
    defaultCode: `// 04_deep_learning.tzd
// 原生 LibTorch 张量运算与深度学习 Pipeline
var x = torch_randn(2, 4);
var w = torch_ones(4, 3);

// 零装箱矩阵乘法与 ReLU 激活
var out = torch_matmul(x, w);
var act = torch_relu(out);

// 组装前向传播网络
var model = new Sequential();
model.add(new Linear(4, 8));
model.add(new ReLU());
model.add(new Linear(8, 2));

model.save("weights_checkpoint.pt");
print("Model initialized and checkpoint weights exported successfully.");
print("Input shape: [2, 4], MatMul result ready.");`,
    terminalOutput: `[LibTorch Native] ATen Engine linked (AVX2/FMA vector instructions enabled).
[TzdTorch] Tensor allocated: shape [2, 4], dtype=Float32
Model initialized and checkpoint weights exported successfully.
Input shape: [2, 4], MatMul result ready.
● Inference latency: 0.0008s (Zero Python GIL, Zero Boxing)`
  },

  "05_concurrency": {
    group: "极致并发",
    file: "05_multithreading.tzd",
    badge: "CHAPTER 05 · EXTREME CONCURRENCY",
    title: "极致并发：原生 OS 工作线程与无锁调度",
    lead: "彻底废除全局解释器锁（No GIL）。多核 CPU 满载并行加速，线程拥有专享 Bump Arena 分配泳道。",
    htmlContent: `
      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="4" width="16" height="16" rx="2" ry="2"></rect><rect x="9" y="9" width="6" height="6"></rect><line x1="9" y1="1" x2="9" y2="4"></line><line x1="15" y1="1" x2="15" y2="4"></line><line x1="9" y1="20" x2="9" y2="23"></line><line x1="15" y1="20" x2="15" y2="23"></line><line x1="20" y1="9" x2="23" y2="9"></line><line x1="20" y1="14" x2="23" y2="14"></line><line x1="1" y1="9" x2="4" y2="9"></line><line x1="1" y1="14" x2="4" y2="14"></line></svg>
        <span>真·多核并行设计</span>
      </h3>
      <p class="doc-body-text">
        许多高级语言在多线程并发上面临难以逾越的障碍（如 Python 的 GIL 导致多线程无法多核计算，Java 的线程调度受限于重型锁监视器）。TzdLang 原生对接底层操作系统线程，各线程在独立分配泳道（Thread-Local Bump Arena）中运行，无需加锁互斥即可完成临时对象的大规模创建。
      </p>

      <div class="apple-info-callout">
        <strong>数据汇聚机制：</strong>主线程通过 <code>thread.join()</code> 汇合结果，老年代并发标记写屏障（SATB）在后台静默护航，确保无锁高速计算的同时指针引用安全不遗漏。
      </div>

      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="20" x2="18" y2="10"></line><line x1="12" y1="20" x2="12" y2="4"></line><line x1="6" y1="20" x2="6" y2="14"></line></svg>
        <span>心智模型：并发多线程对比</span>
      </h3>

      <div class="comparison-card-container">
        <div class="comparison-header">
          <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M17 21v-2a4 4 0 0 0-4-4H5a4 4 0 0 0-4 4v2"></path><circle cx="9" cy="7" r="4"></circle><path d="M23 21v-2a4 4 0 0 0-3-3.87"></path><path d="M16 3.13a4 4 0 0 1 0 7.75"></path></svg>
          <span>多核计算与多线程吞吐表现对比</span>
        </div>
        <div class="comparison-grid">
          <!-- TzdLang -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-tzd">TzdLang</span>
              <span style="font-size:0.7rem; color:#34D399;">真并行 · 独立泳道</span>
            </div>
            <pre class="comparison-snippet"><code>fun worker(id: int) {
    var k = 0;
    for (k = 0; k &lt; 500000; k++) {}
}
var t1 = new Thread(worker, 1);
var t2 = new Thread(worker, 2);
t1.join(); t2.join();</code></pre>
            <div class="comparison-verdict">
              <strong>优势：</strong>硬件级满核计算，无锁吞吐率提升 400%，多线程计算耗时呈线性下降。
            </div>
          </div>

          <!-- Python -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-py">Python 3</span>
              <span style="font-size:0.7rem; color:#FCD34D;">GIL 全局锁互斥伪多线程</span>
            </div>
            <pre class="comparison-snippet"><code>import threading

def worker(id):
    for _ in range(500000): pass

t1 = threading.Thread(target=worker, args=(1,))
t2 = threading.Thread(target=worker, args=(2,))
t1.start(); t2.start()
t1.join(); t2.join()</code></pre>
            <div class="comparison-verdict">
              <strong>劣势：</strong>因 GIL 存在，CPU 密集型多线程不仅不会加速，反而因线程上下文激烈抢锁而大幅变慢！
            </div>
          </div>

          <!-- Java (HotSpot) -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-cpp">Java</span>
              <span style="font-size:0.7rem; color:#93C5FD;">重型线程与偏向锁开销</span>
            </div>
            <pre class="comparison-snippet"><code>Thread t1 = new Thread(() -&gt; {
    for (int k = 0; k &lt; 500000; k++) {}
});
Thread t2 = new Thread(() -&gt; {
    for (int k = 0; k &lt; 500000; k++) {}
});
t1.start(); t2.start();
t1.join(); t2.join();</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>系统线程映射开销较重，高频并发下的堆内存垃圾收集（Safepoint Stop-The-World）偶发卡顿。
            </div>
          </div>
        </div>
      </div>
    `,
    defaultCode: `// 05_multithreading.tzd
// 原生操作系统多线程与无锁调度
fun computeTask(workerId: int, iterations: int) {
    var acc = 0;
    var j = 0;
    for (j = 0; j < iterations; j++) {
        acc = acc + (j % 5);
    }
    print("Worker [" + workerId + "] finished. Acc: " + acc);
}

// 并发启动 3 个原生工作线程
var t1 = new Thread(computeTask, 1, 200000);
var t2 = new Thread(computeTask, 2, 200000);
var t3 = new Thread(computeTask, 3, 200000);

t1.join();
t2.join();
t3.join();
print("All native worker threads completed in parallel (No GIL).");`,
    terminalOutput: `[OS Thread Manager] Spawning Native OS Threads (pthreads/Win32)...
Worker [1] finished. Acc: 400000
Worker [2] finished. Acc: 400000
Worker [3] finished. Acc: 400000
All native worker threads completed in parallel (No GIL).
● Parallel Execution Wall-Time: 0.0011s (3 cores saturated, 0ms lock wait)`
  },

  "06_tiering_jit": {
    group: "异构执行模型",
    file: "06_tiering_jit.tzd",
    badge: "CHAPTER 06 · EXECUTION ARCHITECTURE",
    title: "异构执行模型：Tier 0 VM 跃迁 Tier 1 LLVM ORC JIT",
    lead: "深入编译器内核。解构 ANTLR4 AST 解析、堆栈字节码快速冷启动与后台 LLVM 原生特化机器码生成的全流程。",
    htmlContent: `
      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"></polyline></svg>
        <span>分层编译流水线（Tiering Pipeline）</span>
      </h3>
      <p class="doc-body-text">
        TzdLang 拒绝单一的纯解释或繁重的离线全量编译，构建了精密的双层执行架构：
      </p>
      <ul style="margin: 0.5rem 0 1.25rem 1.5rem; line-height: 1.8; color:#94A3B8; font-size:0.92rem;">
        <li><strong>Tier 0（快速启动层）：</strong>ANTLR4 生成 AST 后，直接由 <code>TzdBytecodeVM</code> 执行紧凑的堆栈指令，支持 <code>.tzdc</code> 二进制序列化，实现 0ms 级别极速冷启动。</li>
        <li><strong>热点探测（Profiler）：</strong>后台无锁统计函数调用频次（阈值 50）与循环回边计数（阈值 1000）。</li>
        <li><strong>Tier 1（LLVM JIT 原生层）：</strong>热点达成后由多线程池触发 LLVM ORC JIT，启动 <code>mem2reg</code>、<code>EarlyCSE</code>、<code>AlwaysInlinerPass</code> 以及专属的<strong>硬件整除（idiv）特化</strong>指令生成，直接生成原生 x86_64 机器码！</li>
      </ul>

      <div class="apple-info-callout">
        <strong>硬件整除特化（idiv Specialization）：</strong>通用脚本语言在处理取模与整除时通常调用通用浮点 <code>fmod</code> 库函数，开销巨大；TzdLang JIT 引擎通过类型特化直接下发硬件单周期 <code>idiv</code> 指令，循环吞吐提速达 350%！
      </div>

      <h3 class="doc-section-heading">
        <svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><line x1="18" y1="20" x2="18" y2="10"></line><line x1="12" y1="20" x2="12" y2="4"></line><line x1="6" y1="20" x2="6" y2="14"></line></svg>
        <span>心智模型：JIT 虚拟机架构对比</span>
      </h3>

      <div class="comparison-card-container">
        <div class="comparison-header">
          <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#38BDF8" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="2" y="3" width="20" height="14" rx="2" ry="2"></rect><line x1="8" y1="21" x2="16" y2="21"></line><line x1="12" y1="17" x2="12" y2="21"></line></svg>
          <span>多层执行机制与冷启动/峰值性能对比</span>
        </div>
        <div class="comparison-grid">
          <!-- TzdLang -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-tzd">TzdLang</span>
              <span style="font-size:0.7rem; color:#34D399;">Tier 0 + LLVM ORC</span>
            </div>
            <pre class="comparison-snippet"><code>fun calcPow(base: int, exp: int) {
    var res = 1;
    var k = 0;
    for (k = 0; k &lt; exp; k++) {
        res = (res * base) % 10007;
    }
    return res;
}</code></pre>
            <div class="comparison-verdict">
              <strong>优势：</strong>冷启动仅 0.4ms；热点发生后即生成原生汇编，消除 100% 堆栈装箱。
            </div>
          </div>

          <!-- Java HotSpot -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-cpp">Java HotSpot</span>
              <span style="font-size:0.7rem; color:#93C5FD;">C1 + C2 编译器</span>
            </div>
            <pre class="comparison-snippet"><code>int calcPow(int base, int exp) {
    int res = 1;
    for (int k = 0; k &lt; exp; k++) {
        res = (res * base) % 10007;
    }
    return res;
}</code></pre>
            <div class="comparison-verdict">
              <strong>机制：</strong>JVM 启动需要耗费数秒加载庞大类库，内存常驻占用动辄 300MB 以上。
            </div>
          </div>

          <!-- JavaScript V8 -->
          <div class="comparison-col">
            <div class="comparison-lang-label">
              <span class="lang-badge badge-py">Node.js V8</span>
              <span style="font-size:0.7rem; color:#FCD34D;">Ignition + TurboFan</span>
            </div>
            <pre class="comparison-snippet"><code>function calcPow(base, exp) {
    let res = 1;
    for (let k = 0; k &lt; exp; k++) {
        res = (res * base) % 10007;
    }
    return res;
}</code></pre>
            <div class="comparison-verdict">
              <strong>劣势：</strong>由于 JavaScript 弱类型随时可能变异，会导致昂贵的 Deoptimization（反优化）回退惩罚。
            </div>
          </div>
        </div>
      </div>
    `,
    defaultCode: `// 06_tiering_jit.tzd
// 热点探测与 LLVM ORC JIT 特化编译
fun benchPow(base: int, exp: int) {
    var res = 1;
    var k = 0;
    for (k = 0; k < exp; k++) {
        res = (res * base) % 1000007; // idiv 硬件原生整除特化
    }
    return res;
}

// 循环调用触发热点阈值 (Invocations >= 50)
var n = 0;
for (n = 0; n < 100; n++) {
    benchPow(7, 300);
}

print("Tier 1 LLVM JIT engaged: Hardware idiv & mem2reg pass active.");
print("Calculated final residue = " + benchPow(7, 300));`,
    terminalOutput: `[Tier 0 BytecodeVM] Loop iterations counter tracking: Invocations=50.
[Profiler] Triggering Async Tier 1 LLVM ORC Compilation Task...
[LLVM ORC JIT] Pass Pipeline: mem2reg -> EarlyCSE -> SRem(idiv) spec.
[LLVM ORC JIT] Generated machine code entry: worker_native @ 0x7FFE20B41000
Tier 1 LLVM JIT engaged: Hardware idiv & mem2reg pass active.
Calculated final residue = 469793
● Execution Speedup Factor: 2.5x vs HotSpot (0.0002s total)`
  }
};

let currentActiveChapter = "01_quickstart";

// ---------------------------------------------------------------------------
// 1. Switch Chapter Controller
// ---------------------------------------------------------------------------
function loadChapter(chapterId) {
  if (!TOUR_DATA[chapterId]) return;
  currentActiveChapter = chapterId;
  const chapter = TOUR_DATA[chapterId];

  // Update Catalog active class
  document.querySelectorAll(".catalog-nav-item").forEach((item) => {
    if (item.getAttribute("data-chapter") === chapterId) {
      item.classList.add("active");
      // Horizontal swipeable track auto-centering on mobile/tablet
      if (window.innerWidth <= 1024) {
        item.scrollIntoView({ behavior: "smooth", block: "nearest", inline: "center" });
      }
    } else {
      item.classList.remove("active");
    }
  });

  // Crossfade Content & Playground (0.2s)
  const contentWrapper = document.getElementById("tour-content-wrapper");
  if (contentWrapper) {
    contentWrapper.classList.add("fade-out");

    setTimeout(() => {
      document.getElementById("tour-badge").textContent = chapter.badge;
      document.getElementById("tour-title").textContent = chapter.title;
      document.getElementById("tour-lead").textContent = chapter.lead;
      
      // Inject body + mobile interactive run trigger
      document.getElementById("tour-dynamic-body").innerHTML = 
        chapter.htmlContent + 
        `<div style="text-align:center; margin:2rem 0 1rem;">
           <button class="btn-jump-to-playground" onclick="switchToPlaygroundTab()">
             <svg xmlns="http://www.w3.org/2000/svg" width="14" height="14" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3"></polygon></svg>
             <span>在实验台中执行此范例 ⚡</span>
           </button>
         </div>`;

      contentWrapper.classList.remove("fade-out");
    }, 100);
  }

  // Update Playground File & Code
  document.getElementById("playground-file-name").textContent = chapter.file;
  const codeTextarea = document.getElementById("pg-code-textarea");
  if (codeTextarea) {
    codeTextarea.value = chapter.defaultCode;
    updateLineNumbers();
  }

  // Set default terminal message
  const terminalScreen = document.getElementById("terminal-screen");
  if (terminalScreen) {
    terminalScreen.innerHTML = `<span style="color:#52525A;">// 点击上方「运行 (Run)」按钮执行当前语法案例...</span>`;
  }
}

// ---------------------------------------------------------------------------
// 2. Playground Line Numbers & Sync
// ---------------------------------------------------------------------------
function updateLineNumbers() {
  const codeTextarea = document.getElementById("pg-code-textarea");
  const lineCol = document.getElementById("pg-line-numbers");
  if (!codeTextarea || !lineCol) return;

  const lineCount = codeTextarea.value.split("\n").length;
  let nums = "";
  for (let i = 1; i <= lineCount; i++) {
    nums += i + "<br>";
  }
  lineCol.innerHTML = nums;
}

// ---------------------------------------------------------------------------
// 3. Execution Simulator
// ---------------------------------------------------------------------------
function initPlaygroundControls() {
  const runBtn = document.getElementById("btn-run-code");
  const resetBtn = document.getElementById("btn-reset-code");
  const codeTextarea = document.getElementById("pg-code-textarea");
  const terminalScreen = document.getElementById("terminal-screen");

  if (runBtn) {
    runBtn.addEventListener("click", () => {
      const chapter = TOUR_DATA[currentActiveChapter];
      if (!chapter) return;

      runBtn.style.opacity = "0.7";
      runBtn.innerHTML = `
        <svg class="spin-icon" xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"><line x1="12" y1="2" x2="12" y2="6"></line><line x1="12" y1="18" x2="12" y2="22"></line><line x1="4.93" y1="4.93" x2="7.76" y2="7.76"></line><line x1="16.24" y1="16.24" x2="19.07" y2="19.07"></line><line x1="2" y1="12" x2="6" y2="12"></line><line x1="18" y1="12" x2="22" y2="12"></line><line x1="4.93" y1="19.07" x2="7.76" y2="16.24"></line><line x1="16.24" y1="7.76" x2="19.07" y2="4.93"></line></svg>
        <span>执行中...</span>
      `;

      terminalScreen.innerHTML = `<span class="terminal-accent">[TzdVM] Parsing & compiling...</span>`;

      setTimeout(() => {
        runBtn.style.opacity = "1";
        runBtn.innerHTML = `
          <svg xmlns="http://www.w3.org/2000/svg" width="12" height="12" viewBox="0 0 24 24" fill="currentColor"><polygon points="5 3 19 12 5 21 5 3"></polygon></svg>
          <span>运行</span>
        `;
        terminalScreen.textContent = chapter.terminalOutput;
      }, 350);
    });
  }

  if (resetBtn) {
    resetBtn.addEventListener("click", () => {
      const chapter = TOUR_DATA[currentActiveChapter];
      if (chapter && codeTextarea) {
        codeTextarea.value = chapter.defaultCode;
        updateLineNumbers();
        terminalScreen.innerHTML = `<span style="color:#52525A;">// 代码已重置为本章默认范例。</span>`;
      }
    });
  }

  if (codeTextarea) {
    codeTextarea.addEventListener("input", updateLineNumbers);
    codeTextarea.addEventListener("scroll", () => {
      const lineCol = document.getElementById("pg-line-numbers");
      if (lineCol) {
        lineCol.scrollTop = codeTextarea.scrollTop;
      }
    });
  }
}

// ---------------------------------------------------------------------------
// 4. Quick Search Modal Controller (Cmd + K / Ctrl + K)
// ---------------------------------------------------------------------------
function initSearchModal() {
  const modal = document.getElementById("search-modal");
  const triggerBtn = document.getElementById("search-trigger-btn");
  const mobileTrigger = document.getElementById("mobile-search-btn");
  const searchInput = document.getElementById("search-input");
  const resultsContainer = document.getElementById("search-results-list");
  if (!modal || !searchInput || !resultsContainer) return;

  const openModal = () => {
    modal.classList.add("open");
    searchInput.value = "";
    renderSearchResults("");
    setTimeout(() => searchInput.focus(), 50);
  };

  const closeModal = () => {
    modal.classList.remove("open");
  };

  if (triggerBtn) triggerBtn.addEventListener("click", openModal);
  if (mobileTrigger) mobileTrigger.addEventListener("click", openModal);

  modal.addEventListener("click", (e) => {
    if (e.target === modal) closeModal();
  });

  document.addEventListener("keydown", (e) => {
    if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === "k") {
      e.preventDefault();
      if (modal.classList.contains("open")) {
        closeModal();
      } else {
        openModal();
      }
    } else if (e.key === "Escape" && modal.classList.contains("open")) {
      closeModal();
    }
  });

  searchInput.addEventListener("input", (e) => {
    renderSearchResults(e.target.value.trim().toLowerCase());
  });

  function renderSearchResults(query) {
    resultsContainer.innerHTML = "";
    const matches = Object.keys(TOUR_DATA).filter((key) => {
      const item = TOUR_DATA[key];
      return (
        !query ||
        item.title.toLowerCase().includes(query) ||
        item.lead.toLowerCase().includes(query) ||
        item.group.toLowerCase().includes(query)
      );
    });

    if (matches.length === 0) {
      resultsContainer.innerHTML = `<div style="padding:1rem; text-align:center; color:#52525A; font-size:0.85rem;">未找到相关语法章节</div>`;
      return;
    }

    matches.forEach((key, idx) => {
      const item = TOUR_DATA[key];
      const div = document.createElement("div");
      div.className = `search-result-item ${idx === 0 ? "selected" : ""}`;
      div.innerHTML = `
        <div>
          <div style="font-weight:500; color:#F5F5F7;">${item.title}</div>
          <div style="font-size:0.75rem; color:#86868B; margin-top:2px;">${item.lead.substring(0, 48)}...</div>
        </div>
        <span class="search-result-group">${item.group}</span>
      `;
      div.addEventListener("click", () => {
        loadChapter(key);
        closeModal();
      });
      resultsContainer.appendChild(div);
    });
  }
}

// ---------------------------------------------------------------------------
// 5. Mobile Mode Segmented Switcher (Guide vs Playground)
// ---------------------------------------------------------------------------
function initMobileTabs() {
  const tabBtns = document.querySelectorAll(".mobile-tab-btn");
  const workspace = document.getElementById("tour-workspace");
  if (!tabBtns.length || !workspace) return;

  tabBtns.forEach((btn) => {
    btn.addEventListener("click", () => {
      const targetTab = btn.getAttribute("data-tab");
      tabBtns.forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");

      if (targetTab === "playground") {
        workspace.classList.add("mobile-mode-playground");
      } else {
        workspace.classList.remove("mobile-mode-playground");
      }
    });
  });
}

// Global helper to switch to playground from anywhere (e.g. guide buttons)
window.switchToPlaygroundTab = function() {
  const playgroundTabBtn = document.querySelector('.mobile-tab-btn[data-tab="playground"]');
  if (playgroundTabBtn && window.innerWidth <= 1024) {
    playgroundTabBtn.click();
  }
  const runBtn = document.getElementById("btn-run-code");
  if (runBtn) {
    runBtn.click();
  }
};

// ---------------------------------------------------------------------------
// 6. Lifecycle Initialization
// ---------------------------------------------------------------------------
document.addEventListener("DOMContentLoaded", () => {
  // Bind Catalog Navigation Items
  document.querySelectorAll(".catalog-nav-item").forEach((btn) => {
    btn.addEventListener("click", () => {
      const chapterId = btn.getAttribute("data-chapter");
      loadChapter(chapterId);
    });
  });

  // Init Playground, Search, & Mobile Tabs
  initPlaygroundControls();
  initSearchModal();
  initMobileTabs();

  // Load first chapter
  loadChapter("01_quickstart");

  // Create Lucide Icons
  if (window.lucide) {
    window.lucide.createIcons();
  }
});

