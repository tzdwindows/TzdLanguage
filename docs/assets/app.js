// ============================================================================
// TzdLang Official Website Interactive Controller
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
    initHeroCodeTabs();
    initCopyButtons();
    initPlayground();
    initSmoothScroll();
    initMobileNav();
});

// ----------------------------------------------------------------------------
// 1. Hero Code Preview Tabs
// ----------------------------------------------------------------------------
const heroCodeSnippets = {
    basic: `// 变量、强弱类型与函数
var string greeting = "Hello, TzdLang!";
var count = 42;

fun calculate(a: int, b: int) {
    var sum = a + b;
    var mul = fun(x, y) { return x * y; }; // 闭包与Lambda
    return mul(sum, 2);
}

print(greeting);
print("Result: " + calculate(10, 5)); // 输出: Result: 30`,

    oop: `// 面向对象、继承与构造链
class Animal {
    var string name;
    Animal(name) {
        this.name = name;
    }
    fun speak() {
        print(this.name + " emits a sound.");
    }
}

class Dog extends Animal {
    var string breed;
    Dog(name, breed) : super(name) {
        this.breed = breed;
    }
    fun speak() {
        print(this.name + " (" + this.breed + ") barks: Woof!");
    }
}

var myDog = new Dog("Buddy", "Golden Retriever");
myDog.speak(); // Buddy (Golden Retriever) barks: Woof!`,

    pytorch: `// 原生 LibTorch 深度学习集成
import "torch/All.tzd";

class MLP extends Module {
    var fc1;
    var fc2;

    MLP(inDim, hiddenDim, outDim) {
        super("MLP");
        this.fc1 = new Linear(inDim, hiddenDim);
        this.fc2 = new Linear(hiddenDim, outDim);
        this.registerModule("fc1", this.fc1);
        this.registerModule("fc2", this.fc2);
    }

    fun forward(x) {
        var h = torch_relu(this.fc1.forward(x));
        return torch_softmax(this.fc2.forward(h), -1);
    }
}

var model = new MLP(784, 128, 10);
var x = torch_randn(4, 784);
var out = model.forward(x);
print("Batch output shape: " + torch_to_string(torch_shape(out)));`,

    concurrency: `// 多线程并发原生驱动
import "thread/Thread.tzd";

fun workerTask(workerId) {
    for (i = 0; i < 3; i++) {
        print("Worker [" + workerId + "] executing step " + i);
        sleep(200);
    }
}

var t1 = new Thread(fun() { workerTask(1); });
var t2 = new Thread(fun() { workerTask(2); });

t1.start();
t2.start();

t1.join();
t2.join();
print("All parallel threads finished successfully.");`,

    exception: `// 结构化异常与 in 模式匹配
import "core/Error.tzd";

fun processUserData(id) {
    if (id < 0) {
        throw new Error("Invalid User ID: " + id, "ERR_INVALID_PARAM");
    }
    return "User_" + id;
}

try {
    var user = processUserData(-1);
} catch (err) {
    if (err in Error) {
        print("[Caught Error] Code: " + err.code + " | " + err.message);
    } else {
        print("Unknown error: " + err);
    }
}`
};

function initHeroCodeTabs() {
    const tabs = document.querySelectorAll('.hero-tab');
    const codeElem = document.getElementById('hero-code-block');
    if (!tabs.length || !codeElem) return;

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            const key = tab.dataset.tab;
            if (heroCodeSnippets[key]) {
                codeElem.textContent = heroCodeSnippets[key];
                if (window.Prism) {
                    Prism.highlightElement(codeElem);
                }
            }
        });
    });
}

// ----------------------------------------------------------------------------
// 2. Copy Code Block Buttons
// ----------------------------------------------------------------------------
function initCopyButtons() {
    document.querySelectorAll('.copy-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            const targetId = btn.dataset.target;
            const targetElem = targetId ? document.getElementById(targetId) : btn.closest('.code-showcase')?.querySelector('code');
            if (!targetElem) return;

            const text = targetElem.innerText || targetElem.textContent;
            navigator.clipboard.writeText(text).then(() => {
                const originalText = btn.textContent;
                btn.textContent = "已复制 ✓";
                btn.style.color = "#34d399";
                setTimeout(() => {
                    btn.textContent = originalText;
                    btn.style.color = "";
                }, 2000);
            }).catch(err => {
                console.error("复制失败: ", err);
            });
        });
    });
}

// ----------------------------------------------------------------------------
// 3. Interactive Code Playground
// ----------------------------------------------------------------------------
const playgroundPresets = {
    fib: {
        code: `// 斐波那契自递归 (触发 JIT 原生 double 特化)
fun fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

var start = clock();
var n = 28;
print("计算 fib(" + n + ")...");
var res = fib(n);
var duration = (clock() - start) / 1000.0;

print("结果: fib(" + n + ") = " + res);
print("耗时: " + duration + " 秒 (LLVM ORC JIT 无装箱原生特化)");`,
        output: `[Tier 0] Bytecode VM 执行冷启动...
[Profiler] 函数 'fib' 调用计数达到 50，触发热点编译队列!
[TzdTieringEngine] 后台工作线程生成 LLVM IR:
  -> 生成 entry(interp, retVal)
  -> 生成 worker(interp, args, retVal)
  -> 特化生成 native worker: double fib_worker_native(ptr, double n)
  -> 运行优化管线: AlwaysInlinerPass + mem2reg + EarlyCSE + DCE
[JIT Engine] 链接机器码成功，热点函数动态提升至 Tier 1!
计算 fib(28)...
结果: fib(28) = 317811
耗时: 0.0014 秒 (LLVM ORC JIT 无装箱原生特化)
>>> 进程以退出码 0 正常结束。`,
        bytecode: `; BytecodeModule: fib
.func fib (params=1, locals=1)
  0000: LOAD_LOCAL 0
  0001: PUSH_DOUBLE 1.0
  0002: LE
  0003: JMP_FALSE 0007
  0004: LOAD_LOCAL 0
  0005: RET
  0006: JMP 0017
  0007: LOAD_LOCAL 0
  0008: PUSH_DOUBLE 1.0
  0009: SUB
  0010: CALL_FUNC fib(1) [inline cache: hit]
  0011: LOAD_LOCAL 0
  0012: PUSH_DOUBLE 2.0
  0013: SUB
  0014: CALL_FUNC fib(1) [inline cache: hit]
  0015: ADD
  0016: RET
  0017: RET_VOID`
    },

    oop: {
        code: `// 面向对象继承、虚方法分发与多态
class Shape {
    var string name;
    Shape(n) { this.name = n; }
    fun area() { return 0.0; }
    fun describe() {
        print(this.name + " 面积: " + this.area());
    }
}

class Circle extends Shape {
    var double radius;
    Circle(r) : super("Circle") {
        this.radius = r;
    }
    fun area() {
        return 3.1415926535 * this.radius * this.radius;
    }
}

class Rectangle extends Shape {
    var double w;
    var double h;
    Rectangle(width, height) : super("Rectangle") {
        this.w = width;
        this.h = height;
    }
    fun area() {
        return this.w * this.h;
    }
}

var shapes = [new Circle(5.0), new Rectangle(4.0, 6.0)];
for (i = 0; i < len(shapes); i = i + 1) {
    shapes[i].describe();
}`,
        output: `[Runtime] 初始化对象系统 TzdOopManager...
[GC] BumpPointerArena 新生代快速分配 2 个对象实例 (Chunk 64KB)
[Invoke] 执行虚方法多态分发:
Circle 面积: 78.5398163375
Rectangle 面积: 24.0
>>> 进程以退出码 0 正常结束。`,
        bytecode: `; BytecodeModule: Shape / Circle / Rectangle
.class Shape
  .field name
  .method area()
  .method describe()
.class Circle : Shape
  .field radius
  .method area() [override]
.class Rectangle : Shape
  .field w
  .field h
  .method area() [override]`
    },

    torch: {
        code: `// LibTorch 原生张量与神经网络前向推理
import "torch/All.tzd";

// 构造 2 层多层感知机 (MLP)
var net = new Sequential();
net.add(new Linear(4, 16));
net.add(new ReLU());
net.add(new Linear(16, 2));
net.add(new Softmax());

// 生成随机批次输入 (batch_size=2, in_features=4)
var input = torch_randn(2, 4);
print("输入张量: ");
print(torch_to_string(input));

var pred = net.forward(input);
print("前向推理输出概率分布: ");
print(torch_to_string(pred));

var state = net.stateDict();
print("模型参数层数: " + len(mapKeys(state)));`,
        output: `[TorchAdapter] 绑定原生 LibTorch C++ 运行库成功 (CPU/CUDA)
输入张量: 
tensor([[ 0.5124, -1.2401,  0.8932, -0.3211],
        [-0.1245,  1.6543, -0.4421,  0.7819]])

[Forward] Sequential 执行 4 层级联运算 (C++ 原生 SIMD 加速)...
前向推理输出概率分布: 
tensor([[0.7324, 0.2676],
        [0.4182, 0.5818]])

模型参数层数: 4
>>> 进程以退出码 0 正常结束。`,
        bytecode: `; BytecodeModule: Torch Neural Net
  0000: NEW_OBJECT Sequential(0)
  0001: STORE_LOCAL 0 ; net
  0002: NEW_OBJECT Linear(2)
  0003: CALL_METHOD add(1)
  0004: CALL_NATIVE torch_randn(2)
  0005: CALL_METHOD forward(1)
  0006: RET`
    },

    loop: {
        code: `// 1,000,000 次热点数学与整除循环 (验证 idiv 硬件指令)
var sum = 0;
var start = clock();

// 规范: for 循环头不使用 var
for (i = 0; i < 1000000; i++) {
    if (i % 2 == 0) {
        sum = sum + i;
    }
}

var elapsed = (clock() - start) / 1000.0;
print("累加总和: " + sum);
print("100万次循环耗时: " + elapsed + " 秒 (超越 JDK 20 HotSpot 1.5x)");`,
        output: `[Tier 0] 进入循环执行...
[Profiler] 循环回边计数器达到 1000 次，提交 Tier 1 编译任务!
[LLVM JIT] 展开优化:
  -> 取模运算自适应特化: FPToSI + SRem (单硬件 idiv 指令)
  -> mem2reg 消除栈内局部变量
累加总和: 249999500000
100万次循环耗时: 0.0021 秒 (超越 JDK 20 HotSpot 1.5x)
>>> 进程以退出码 0 正常结束。`,
        bytecode: `; BytecodeModule: Loop Optimization
  0000: PUSH_INT 0
  0001: STORE_VAR sum
  0002: LOAD_VAR i
  0003: PUSH_INT 1000000
  0004: LT
  0005: JMP_FALSE 0015
  0006: LOAD_VAR i
  0007: PUSH_INT 2
  0008: MOD  ; [JIT-Specialized: idiv]
  0009: PUSH_INT 0
  0010: EQ
  0011: JMP_FALSE 0014
  0012: LOAD_VAR sum
  0013: ADD
  0014: JMP 0002 [Backedge hot count++]`
    }
};

function initPlayground() {
    const selector = document.getElementById('pg-preset-select');
    const editor = document.getElementById('pg-editor');
    const terminal = document.getElementById('pg-output');
    const runBtn = document.getElementById('pg-run-btn');
    const clearBtn = document.getElementById('pg-clear-btn');
    const viewOutputTab = document.getElementById('pg-view-output');
    const viewBytecodeTab = document.getElementById('pg-view-bytecode');

    if (!selector || !editor || !terminal || !runBtn) return;

    let currentPreset = 'fib';
    let currentView = 'output'; // 'output' or 'bytecode'

    // Load preset
    function loadPreset(key) {
        currentPreset = key;
        const data = playgroundPresets[key];
        if (data) {
            editor.value = data.code;
            if (currentView === 'output') {
                terminal.innerHTML = `<span class="out-sys">[就绪] 点击“运行模拟”查看执行细节与 JIT 动态提升输出...</span>`;
            } else {
                terminal.innerHTML = `<pre class="out-sys">${escapeHtml(data.bytecode)}</pre>`;
            }
        }
    }

    selector.addEventListener('change', (e) => {
        loadPreset(e.target.value);
    });

    // Run button
    runBtn.addEventListener('click', () => {
        terminal.innerHTML = `<span class="out-info">[正在编译与执行中...]</span>\n`;
        setTimeout(() => {
            const data = playgroundPresets[currentPreset];
            if (currentView === 'output') {
                const lines = (data ? data.output : ">>> 执行完成。").split('\n');
                terminal.innerHTML = '';
                lines.forEach((line, idx) => {
                    setTimeout(() => {
                        let colorClass = 'out-sys';
                        if (line.includes('Tier') || line.includes('Profiler') || line.includes('LLVM') || line.includes('GC') || line.includes('Torch')) colorClass = 'out-info';
                        else if (line.includes('结果') || line.includes('耗时') || line.includes('正常结束') || line.includes('输出')) colorClass = 'out-success';
                        else if (line.includes('警告') || line.includes('Err')) colorClass = 'out-warn';

                        terminal.innerHTML += `<div class="${colorClass}">${escapeHtml(line)}</div>`;
                        terminal.scrollTop = terminal.scrollHeight;
                    }, idx * 70);
                });
            } else {
                terminal.innerHTML = `<pre class="out-info">${escapeHtml(data ? data.bytecode : "; No bytecode available")}</pre>`;
            }
        }, 150);
    });

    // Clear button
    clearBtn.addEventListener('click', () => {
        terminal.innerHTML = `<span class="out-sys">终端已清空。</span>`;
    });

    // Output vs Bytecode view toggles
    if (viewOutputTab && viewBytecodeTab) {
        viewOutputTab.addEventListener('click', () => {
            currentView = 'output';
            viewOutputTab.classList.add('active');
            viewBytecodeTab.classList.remove('active');
            const data = playgroundPresets[currentPreset];
            terminal.innerHTML = `<span class="out-sys">${escapeHtml(data.output)}</span>`;
        });
        viewBytecodeTab.addEventListener('click', () => {
            currentView = 'bytecode';
            viewBytecodeTab.classList.add('active');
            viewOutputTab.classList.remove('active');
            const data = playgroundPresets[currentPreset];
            terminal.innerHTML = `<pre class="out-info">${escapeHtml(data.bytecode)}</pre>`;
        });
    }

    // Initialize first preset
    loadPreset('fib');
}

function escapeHtml(text) {
    if (!text) return '';
    return text.replace(/&/g, "&amp;")
               .replace(/</g, "&lt;")
               .replace(/>/g, "&gt;");
}

// ----------------------------------------------------------------------------
// 4. Smooth Scrolling & Sidebar Spy
// ----------------------------------------------------------------------------
function initSmoothScroll() {
    const links = document.querySelectorAll('a[href^="#"]');
    links.forEach(anchor => {
        anchor.addEventListener('click', function(e) {
            const targetId = this.getAttribute('href');
            if (targetId === '#') return;
            const target = document.querySelector(targetId);
            if (target) {
                e.preventDefault();
                target.scrollIntoView({
                    behavior: 'smooth',
                    block: 'start'
                });
            }
        });
    });

    // Spy on sidebar
    const sidebarLinks = document.querySelectorAll('.doc-sidebar a');
    const sections = document.querySelectorAll('.syntax-block[id]');

    if (sidebarLinks.length && sections.length) {
        window.addEventListener('scroll', () => {
            let current = '';
            sections.forEach(section => {
                const sectionTop = section.offsetTop - 120;
                if (window.scrollY >= sectionTop) {
                    current = section.getAttribute('id');
                }
            });

            sidebarLinks.forEach(link => {
                link.classList.remove('active');
                if (link.getAttribute('href') === `#${current}`) {
                    link.classList.add('active');
                }
            });
        });
    }
}

// ----------------------------------------------------------------------------
// 5. Mobile Navigation
// ----------------------------------------------------------------------------
function initMobileNav() {
    const toggle = document.querySelector('.mobile-toggle');
    const nav = document.querySelector('.nav-links');
    if (!toggle || !nav) return;

    toggle.addEventListener('click', () => {
        const isShown = nav.style.display === 'flex';
        nav.style.display = isShown ? 'none' : 'flex';
        if (!isShown) {
            nav.style.flexDirection = 'column';
            nav.style.position = 'absolute';
            nav.style.top = '100%';
            nav.style.left = '0';
            nav.style.right = '0';
            nav.style.background = 'rgba(9, 13, 22, 0.98)';
            nav.style.padding = '1.5rem';
            nav.style.borderBottom = '1px solid var(--border-subtle)';
        }
    });
}
