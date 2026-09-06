// ============================================================================
// TzdLang Official Website Interactive Controller
// Liquid Glassmorphism, Fluid Motion & Custom Components
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
    initHeroCodeTabs();
    initCopyButtons();
    initPlayground();
    initScrollReveal();
    initSmoothScroll();
    initMobileNav();
    initSyntaxWorkbench();
});

// ----------------------------------------------------------------------------
// 1. Hero Code Preview Tabs with Sliding Capsule Indicator & Fluid Slide Motion
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
    const tabsGroup = document.getElementById('hero-tabs-group');
    const indicator = document.getElementById('tab-indicator-pill');
    const tabs = document.querySelectorAll('.hero-tab');
    const codeElem = document.getElementById('hero-code-block');
    if (!tabsGroup || !tabs.length || !codeElem) return;

    function moveIndicator(targetTab) {
        if (!targetTab || !indicator) return;
        indicator.style.transform = `translateX(${targetTab.offsetLeft}px)`;
        indicator.style.width = `${targetTab.offsetWidth}px`;
    }

    // Initialize indicator on active tab
    const activeTab = tabsGroup.querySelector('.hero-tab.active') || tabs[0];
    setTimeout(() => moveIndicator(activeTab), 50);

    window.addEventListener('resize', () => {
        const currentActive = tabsGroup.querySelector('.hero-tab.active');
        moveIndicator(currentActive);
    });

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            if (tab.classList.contains('active')) return;

            tabs.forEach(t => t.classList.remove('active'));
            tab.classList.add('active');
            moveIndicator(tab);

            const key = tab.dataset.tab;
            if (heroCodeSnippets[key]) {
                // Fluid slide-out transition
                codeElem.classList.remove('code-slide-in');
                codeElem.classList.add('code-slide-out');

                setTimeout(() => {
                    codeElem.textContent = heroCodeSnippets[key];
                    if (window.Prism) {
                        Prism.highlightElement(codeElem);
                    }
                    // Fluid slide-in transition
                    codeElem.classList.remove('code-slide-out');
                    codeElem.classList.add('code-slide-in');
                }, 140);
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
            const targetElem = targetId ? document.getElementById(targetId) : btn.closest('.code-front-window')?.querySelector('code');
            if (!targetElem) return;

            const text = targetElem.innerText || targetElem.textContent;
            navigator.clipboard.writeText(text).then(() => {
                const originalText = btn.textContent;
                btn.textContent = "已复制 ✓";
                btn.style.color = "#34D399";
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
// 3. Interactive Code Playground & Dynamic Visual Engine
// ----------------------------------------------------------------------------
const playgroundPresets = {
    fib: {
        title: "斐波那契自递归 (JIT 原生 Double 特化)",
        icon: "⚡",
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
  0017: RET_VOID`,
        metrics: {
            jitTier: "Tier 1 Native",
            execTime: "0.0014s",
            mem: "64 KB (Arena)",
            speedup: "+250% 🚀",
            hudIcon: "⚡",
            hudTitle: "JIT 原生特化已完成",
            hudSubtitle: "生成 worker_native(double) · 消除 100% 堆栈装箱",
            hudMetric: "0.0014s"
        }
    },

    torch: {
        title: "LibTorch 神经网络推理 (张量动态流转)",
        icon: "🧠",
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
  0006: RET`,
        metrics: {
            jitTier: "LibTorch C++",
            execTime: "0.0008s",
            mem: "128 KB (Tensors)",
            speedup: "No GIL ⚡",
            hudIcon: "🧠",
            hudTitle: "LibTorch 原生张量推理完成",
            hudSubtitle: "C++ SIMD 硬件矢量加速 · 0ms Python GIL 延迟",
            hudMetric: "0.0008s"
        }
    },

    threads: {
        title: "原生多线程并发调度 (无锁并行泳道)",
        icon: "🧵",
        code: `// 原生多线程并发驱动与无锁调度
import "thread/Thread.tzd";

fun workerTask(id: int, count: int) {
    var sum = 0;
    for (i = 0; i < count; i++) {
        sum = sum + (i * 2 + 1);
    }
    print("Worker [" + id + "] 向量计算完成，总和: " + sum);
}

// 启动 3 个内核级工作线程
var t1 = new Thread(fun() { workerTask(1, 200000); });
var t2 = new Thread(fun() { workerTask(2, 200000); });
var t3 = new Thread(fun() { workerTask(3, 200000); });

t1.start();
t2.start();
t3.start();

print("主线程正在分发任务...");

t1.join();
t2.join();
t3.join();
print("所有工作线程安全汇合，完成无锁并发运算。");`,
        output: `[ConcurrencyManager] 初始化系统级线程池...
[Kernel] 绑定 C++ std::thread 原生多核心调度
主线程正在分发任务...
[Worker 1] 分配至 CPU Core #2，执行向量计算...
[Worker 2] 分配至 CPU Core #3，执行向量计算...
[Worker 3] 分配至 CPU Core #4，执行向量计算...
Worker [1] 向量计算完成，总和: 40000000000
Worker [2] 向量计算完成，总和: 40000000000
Worker [3] 向量计算完成，总和: 40000000000
所有工作线程安全汇合，完成无锁并发运算。
[Metrics] 4 核心并发满载，零死锁 (Lock-Free Barrier Sync)
>>> 进程以退出码 0 正常结束。`,
        bytecode: `; BytecodeModule: Native Thread Concurrency
  0000: NEW_THREAD_OBJECT
  0001: STORE_VAR t1
  0002: CALL_METHOD start() [Native thread fork]
  0003: CALL_METHOD join()  [Barrier wait]
  0004: RET`,
        metrics: {
            jitTier: "4x Threads",
            execTime: "0.0032s",
            mem: "256 KB (Stack)",
            speedup: "4x Core 🚀",
            hudIcon: "🧵",
            hudTitle: "原生多线程无锁并发完成",
            hudSubtitle: "内核级 std::thread 驱动 · 4 核心 100% 满载运行",
            hudMetric: "4.8 Gops"
        }
    },

    loop: {
        title: "100万次循环与整除特化 (单硬件 idiv 指令)",
        icon: "🔄",
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
  0014: JMP 0002 [Backedge hot count++]`,
        metrics: {
            jitTier: "x86_64 idiv",
            execTime: "0.0021s",
            mem: "0 KB (Regs)",
            speedup: "+150% 🚀",
            hudIcon: "🔄",
            hudTitle: "100万次循环优化完毕",
            hudSubtitle: "单条硬件 idiv 指令特化 · mem2reg 消除栈局部变量",
            hudMetric: "0.0021s"
        }
    }
};

// ----------------------------------------------------------------------------
// TzdDynamicVisualEngine (HTML5 Canvas 60FPS Video-Grade Runtime Renderer)
// ----------------------------------------------------------------------------
class TzdDynamicVisualEngine {
    constructor(canvas, callbacks = {}) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.callbacks = callbacks;
        this.preset = 'fib';
        this.state = 'idle'; // 'idle' | 'running' | 'completed'
        this.progress = 0;   // 0.0 -> 1.0
        this.startTime = 0;
        this.duration = 2000; // 2 seconds total simulation
        this.jitTriggered = false;

        // FPS tracking
        this.lastFrameTime = performance.now();
        this.frameCount = 0;
        this.fps = 60;
        this.lastFpsCalc = performance.now();

        // Particles, Sparks and Shockwaves
        this.ambientStars = [];
        this.particles = [];
        this.sparks = [];
        this.shockwaves = [];

        this.width = 0;
        this.height = 0;

        this.initAmbientStars();
        this.resize();

        window.addEventListener('resize', () => this.resize());
        this.loop = this.loop.bind(this);
        requestAnimationFrame(this.loop);
    }

    initAmbientStars() {
        this.ambientStars = [];
        for (let i = 0; i < 35; i++) {
            this.ambientStars.push({
                x: Math.random(),
                y: Math.random(),
                size: 0.8 + Math.random() * 1.5,
                alpha: 0.1 + Math.random() * 0.35,
                speedY: -(0.00015 + Math.random() * 0.0003),
                pulse: Math.random() * Math.PI * 2
            });
        }
    }

    resize() {
        if (!this.canvas) return;
        const rect = this.canvas.getBoundingClientRect();
        if (rect.width === 0 || rect.height === 0) return;
        const dpr = Math.min(window.devicePixelRatio || 1, 2);
        this.width = rect.width;
        this.height = rect.height;
        this.canvas.width = rect.width * dpr;
        this.canvas.height = rect.height * dpr;
        this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    }

    setPreset(key) {
        this.preset = key;
        this.resetSimulation();
    }

    startSimulation() {
        this.state = 'running';
        this.startTime = performance.now();
        this.progress = 0;
        this.jitTriggered = false;
        this.particles = [];
        this.shockwaves = [];
        this.sparks = [];

        // Seed initial burst for specific presets
        if (this.preset === 'torch') {
            this.initTorchParticles();
        }
    }

    resetSimulation() {
        this.state = 'idle';
        this.progress = 0;
        this.jitTriggered = false;
        this.particles = [];
        this.shockwaves = [];
        this.sparks = [];
    }

    initTorchParticles() {
        this.particles = [];
        for (let i = 0; i < 36; i++) {
            this.particles.push({
                stage: Math.floor(Math.random() * 3), // 0: in->h1, 1: h1->h2, 2: h2->out
                fromIdx: Math.floor(Math.random() * 4),
                toIdx: Math.floor(Math.random() * 5),
                t: Math.random(),
                speed: 0.008 + Math.random() * 0.012,
                size: 2.2 + Math.random() * 1.8,
                color: i % 2 === 0 ? '#38BDF8' : '#818CF8'
            });
        }
    }

    triggerShockwave(x, y, color = '#00F2FE') {
        this.shockwaves.push({
            x, y,
            radius: 8,
            maxRadius: Math.max(this.width, this.height) * 0.85,
            color,
            alpha: 0.95
        });

        // Burst sparks
        for (let i = 0; i < 32; i++) {
            const angle = Math.random() * Math.PI * 2;
            const speed = 2.5 + Math.random() * 5.5;
            this.sparks.push({
                x, y,
                vx: Math.cos(angle) * speed,
                vy: Math.sin(angle) * speed,
                alpha: 1,
                decay: 0.02 + Math.random() * 0.03,
                size: 1.5 + Math.random() * 2.2,
                color: Math.random() > 0.3 ? '#00F2FE' : '#38BDF8'
            });
        }
    }

    loop(timestamp) {
        // Measure FPS
        this.frameCount++;
        if (timestamp - this.lastFpsCalc >= 500) {
            this.fps = Math.round((this.frameCount * 1000) / (timestamp - this.lastFpsCalc));
            this.frameCount = 0;
            this.lastFpsCalc = timestamp;
            if (this.callbacks.onFps) {
                this.callbacks.onFps(this.fps);
            }
        }

        // Update progress if running
        if (this.state === 'running') {
            const elapsed = timestamp - this.startTime;
            const rawP = Math.min(1, elapsed / this.duration);
            // Ease out cubic
            this.progress = 1 - Math.pow(1 - rawP, 3);

            if (this.callbacks.onProgress) {
                this.callbacks.onProgress(this.progress, rawP);
            }

            if (rawP >= 1) {
                this.state = 'completed';
                this.progress = 1;
                if (this.callbacks.onComplete) {
                    this.callbacks.onComplete(this.preset);
                }
            }
        }

        // Render Frame
        this.render(timestamp);

        requestAnimationFrame(this.loop);
    }

    render(time) {
        const ctx = this.ctx;
        const w = this.width;
        const h = this.height;
        if (!w || !h) return;

        // Clear canvas with subtle radial backdrop
        ctx.clearRect(0, 0, w, h);

        // Faint cyber grid lines
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.018)';
        ctx.lineWidth = 1;
        const gridStep = 40;
        for (let x = gridStep; x < w; x += gridStep) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, h);
            ctx.stroke();
        }
        for (let y = gridStep; y < h; y += gridStep) {
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(w, y);
            ctx.stroke();
        }

        // Ambient floating stars
        this.ambientStars.forEach(star => {
            star.y += star.speedY;
            if (star.y < 0) star.y = 1;
            const sx = star.x * w;
            const sy = star.y * h;
            const alpha = star.alpha * (0.6 + 0.4 * Math.sin(time * 0.002 + star.pulse));
            ctx.fillStyle = `rgba(148, 163, 184, ${alpha})`;
            ctx.beginPath();
            ctx.arc(sx, sy, star.size, 0, Math.PI * 2);
            ctx.fill();
        });

        // Draw Scenario Graphics
        if (this.preset === 'fib') {
            this.renderFib(time, this.progress);
        } else if (this.preset === 'torch') {
            this.renderTorch(time, this.progress);
        } else if (this.preset === 'threads') {
            this.renderThreads(time, this.progress);
        } else if (this.preset === 'loop') {
            this.renderLoop(time, this.progress);
        }

        // Render Shockwaves
        for (let i = this.shockwaves.length - 1; i >= 0; i--) {
            const sw = this.shockwaves[i];
            sw.radius += (sw.maxRadius - sw.radius) * 0.07 + 3.5;
            sw.alpha -= 0.022;

            if (sw.alpha <= 0 || sw.radius >= sw.maxRadius) {
                this.shockwaves.splice(i, 1);
                continue;
            }

            ctx.save();
            ctx.beginPath();
            ctx.arc(sw.x, sw.y, sw.radius, 0, Math.PI * 2);
            ctx.strokeStyle = `rgba(0, 242, 254, ${sw.alpha})`;
            ctx.lineWidth = 2.5;
            ctx.shadowColor = '#00F2FE';
            ctx.shadowBlur = 16;
            ctx.stroke();
            ctx.restore();
        }

        // Render Sparks
        for (let i = this.sparks.length - 1; i >= 0; i--) {
            const sp = this.sparks[i];
            sp.x += sp.vx;
            sp.y += sp.vy;
            sp.vx *= 0.94;
            sp.vy *= 0.94;
            sp.alpha -= sp.decay;

            if (sp.alpha <= 0) {
                this.sparks.splice(i, 1);
                continue;
            }

            ctx.save();
            ctx.fillStyle = sp.color;
            ctx.globalAlpha = Math.max(0, sp.alpha);
            ctx.shadowColor = sp.color;
            ctx.shadowBlur = 6;
            ctx.beginPath();
            ctx.arc(sp.x, sp.y, sp.size, 0, Math.PI * 2);
            ctx.fill();
            ctx.restore();
        }
    }

    // ------------------------------------------------------------------------
    // Scenario 1: Fibonacci Recursive Tree & Quantum JIT Surge
    // ------------------------------------------------------------------------
    renderFib(time, p) {
        const ctx = this.ctx;
        const w = this.width;
        const h = this.height;

        // Tree structure: 4 levels (1 + 2 + 4 + 8 = 15 nodes)
        const levels = 4;
        const nodes = [];
        const levelY = [55, 130, 215, 305];

        let nodeCounter = 0;
        for (let l = 0; l < levels; l++) {
            const count = Math.pow(2, l);
            const y = levelY[l] || (55 + l * 80);
            for (let i = 0; i < count; i++) {
                const x = (w / (count + 1)) * (i + 1);
                nodes.push({
                    idx: nodeCounter++,
                    level: l,
                    x, y,
                    label: l === 0 ? "fib(5)" : (l === 1 ? (i === 0 ? "fib(4)" : "fib(3)") : (l === 2 ? (i % 2 === 0 ? "fib(3)" : "fib(2)") : (i % 2 === 0 ? "fib(2)" : "fib(1)")))
                });
            }
        }

        // Trigger JIT shockwave at p >= 0.44
        const isJitted = p >= 0.44;
        if (this.state === 'running' && isJitted && !this.jitTriggered) {
            this.jitTriggered = true;
            this.triggerShockwave(nodes[0].x, nodes[0].y, '#00F2FE');
        }

        // Draw connections
        for (let i = 0; i < 7; i++) {
            const parent = nodes[i];
            const leftChild = nodes[2 * i + 1];
            const rightChild = nodes[2 * i + 2];

            [leftChild, rightChild].forEach(child => {
                if (!child) return;
                const isBranchActive = this.state === 'idle' 
                    ? false 
                    : (isJitted || child.idx <= Math.floor(p / 0.44 * 15));

                ctx.save();
                ctx.beginPath();
                ctx.moveTo(parent.x, parent.y);
                ctx.lineTo(child.x, child.y);

                if (isJitted) {
                    ctx.strokeStyle = 'rgba(0, 242, 254, 0.75)';
                    ctx.lineWidth = 2.2;
                    ctx.shadowColor = '#00F2FE';
                    ctx.shadowBlur = 10;
                } else if (isBranchActive) {
                    ctx.strokeStyle = 'rgba(56, 189, 248, 0.65)';
                    ctx.lineWidth = 1.8;
                    ctx.shadowColor = '#38BDF8';
                    ctx.shadowBlur = 6;
                } else {
                    ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
                    ctx.lineWidth = 1.2;
                }
                ctx.stroke();
                ctx.restore();
            });
        }

        // Draw nodes
        nodes.forEach((n) => {
            const isLit = this.state === 'idle'
                ? true
                : (isJitted || n.idx <= Math.floor(p / 0.44 * 15));

            const radius = n.level === 0 ? 17 : (n.level === 1 ? 15 : (n.level === 2 ? 13 : 11));
            const pulseScale = this.state === 'idle'
                ? 1 + 0.05 * Math.sin(time * 0.003 + n.idx)
                : (isJitted ? 1 + 0.08 * Math.sin(time * 0.008 + n.idx) : (isLit ? 1.05 : 1));

            ctx.save();
            ctx.translate(n.x, n.y);
            ctx.scale(pulseScale, pulseScale);

            // Outer glow ring
            ctx.beginPath();
            ctx.arc(0, 0, radius + 4, 0, Math.PI * 2);
            if (isJitted) {
                ctx.fillStyle = 'rgba(0, 242, 254, 0.22)';
                ctx.shadowColor = '#00F2FE';
                ctx.shadowBlur = 14;
            } else if (isLit && this.state !== 'idle') {
                ctx.fillStyle = 'rgba(56, 189, 248, 0.18)';
                ctx.shadowColor = '#38BDF8';
                ctx.shadowBlur = 8;
            } else {
                ctx.fillStyle = 'rgba(255, 255, 255, 0.04)';
            }
            ctx.fill();

            // Core node body
            ctx.beginPath();
            ctx.arc(0, 0, radius, 0, Math.PI * 2);
            if (isJitted) {
                const grad = ctx.createRadialGradient(0, 0, 1, 0, 0, radius);
                grad.addColorStop(0, '#FFFFFF');
                grad.addColorStop(0.5, '#00F2FE');
                grad.addColorStop(1, '#0284C7');
                ctx.fillStyle = grad;
                ctx.strokeStyle = '#38BDF8';
            } else if (isLit && this.state !== 'idle') {
                const grad = ctx.createRadialGradient(0, 0, 1, 0, 0, radius);
                grad.addColorStop(0, '#7DD3FC');
                grad.addColorStop(0.8, '#0284C7');
                grad.addColorStop(1, '#0C4A6E');
                ctx.fillStyle = grad;
                ctx.strokeStyle = '#38BDF8';
            } else {
                ctx.fillStyle = 'rgba(15, 23, 42, 0.8)';
                ctx.strokeStyle = 'rgba(255, 255, 255, 0.15)';
            }
            ctx.lineWidth = 1.5;
            ctx.fill();
            ctx.stroke();

            // Label text
            if (n.level < 3 || w > 480) {
                ctx.font = `${radius >= 15 ? 10 : 8.5}px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace`;
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillStyle = isJitted ? '#042F2E' : (isLit ? '#FFFFFF' : '#94A3B8');
                ctx.fillText(n.label, 0, 0);
            }

            ctx.restore();
        });

        // Top stage badge in canvas
        ctx.save();
        ctx.font = '11px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.textAlign = 'center';
        if (isJitted) {
            ctx.fillStyle = '#00F2FE';
            ctx.shadowColor = '#00F2FE';
            ctx.shadowBlur = 8;
            ctx.fillText("⚡ Tier 1 LLVM JIT: worker_native(ptr, double) 无装箱原生特化", w / 2, 22);
        } else if (this.state === 'running') {
            ctx.fillStyle = '#38BDF8';
            ctx.fillText(`[Tier 0 Bytecode VM] 递归树下钻中 · 调用计数: ${Math.min(50, Math.floor(p / 0.44 * 50))}/50`, w / 2, 22);
        } else {
            ctx.fillStyle = '#94A3B8';
            ctx.fillText("递归调用拓扑图 · 等待 JIT 热点提升", w / 2, 22);
        }
        ctx.restore();
    }

    // ------------------------------------------------------------------------
    // Scenario 2: LibTorch Neural Network & Forward Tensor Stream
    // ------------------------------------------------------------------------
    renderTorch(time, p) {
        const ctx = this.ctx;
        const w = this.width;
        const h = this.height;

        const layers = [
            { name: "输入 x: [2, 4]", count: 4, x: w * 0.16, color: '#38BDF8' },
            { name: "Linear+ReLU (16)", count: 5, x: w * 0.38, color: '#818CF8' },
            { name: "Linear+ReLU (8)",  count: 4, x: w * 0.62, color: '#A855F7' },
            { name: "Softmax y: [2, 2]",count: 2, x: w * 0.84, color: '#34D399' }
        ];

        // Draw connections between layers
        for (let l = 0; l < layers.length - 1; l++) {
            const curL = layers[l];
            const nextL = layers[l + 1];
            const curSpacing = 42;
            const nextSpacing = 42;
            const curStartY = (h / 2) - ((curL.count - 1) * curSpacing) / 2;
            const nextStartY = (h / 2) - ((nextL.count - 1) * nextSpacing) / 2;

            for (let i = 0; i < curL.count; i++) {
                const y1 = curStartY + i * curSpacing;
                for (let j = 0; j < nextL.count; j++) {
                    const y2 = nextStartY + j * nextSpacing;
                    ctx.save();
                    ctx.beginPath();
                    ctx.moveTo(curL.x, y1);
                    ctx.lineTo(nextL.x, y2);
                    const isActive = this.state !== 'idle' && (p * 3 > l);
                    ctx.strokeStyle = isActive ? 'rgba(129, 140, 248, 0.22)' : 'rgba(255, 255, 255, 0.05)';
                    ctx.lineWidth = isActive ? 1.2 : 0.8;
                    ctx.stroke();
                    ctx.restore();
                }
            }
        }

        // Draw traveling tensor particles
        if (this.state !== 'idle' && this.particles.length) {
            this.particles.forEach(pt => {
                pt.t += pt.speed * (this.state === 'running' ? 1.4 : 0.6);
                if (pt.t > 1) {
                    pt.t = 0;
                    pt.stage = (pt.stage + 1) % 3;
                    pt.fromIdx = Math.floor(Math.random() * layers[pt.stage].count);
                    pt.toIdx = Math.floor(Math.random() * layers[pt.stage + 1].count);
                }

                const l1 = layers[pt.stage];
                const l2 = layers[pt.stage + 1];
                const spacing1 = 42;
                const spacing2 = 42;
                const y1 = (h / 2) - ((l1.count - 1) * spacing1) / 2 + pt.fromIdx * spacing1;
                const y2 = (h / 2) - ((l2.count - 1) * spacing2) / 2 + pt.toIdx * spacing2;

                const px = l1.x + (l2.x - l1.x) * pt.t;
                const py = y1 + (y2 - y1) * pt.t;

                ctx.save();
                ctx.beginPath();
                ctx.arc(px, py, pt.size, 0, Math.PI * 2);
                ctx.fillStyle = pt.color;
                ctx.shadowColor = pt.color;
                ctx.shadowBlur = 8;
                ctx.fill();
                ctx.restore();
            });
        }

        // Draw neurons in each layer
        layers.forEach((l, lIdx) => {
            const spacing = 42;
            const startY = (h / 2) - ((l.count - 1) * spacing) / 2;

            // Column Header
            ctx.save();
            ctx.font = '10px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
            ctx.fillStyle = l.color;
            ctx.textAlign = 'center';
            ctx.fillText(l.name, l.x, startY - 26);
            ctx.restore();

            for (let i = 0; i < l.count; i++) {
                const ny = startY + i * spacing;
                const isActivated = this.state !== 'idle' && (p * 3.5 >= lIdx);

                ctx.save();
                ctx.translate(l.x, ny);

                // Halo
                ctx.beginPath();
                ctx.arc(0, 0, 16, 0, Math.PI * 2);
                ctx.fillStyle = isActivated ? 'rgba(56, 189, 248, 0.12)' : 'rgba(255, 255, 255, 0.03)';
                if (isActivated) {
                    ctx.shadowColor = l.color;
                    ctx.shadowBlur = 10;
                }
                ctx.fill();

                // Core
                ctx.beginPath();
                ctx.arc(0, 0, 11, 0, Math.PI * 2);
                ctx.fillStyle = isActivated ? l.color : 'rgba(30, 41, 59, 0.8)';
                ctx.strokeStyle = isActivated ? '#FFFFFF' : 'rgba(255, 255, 255, 0.2)';
                ctx.lineWidth = 1.5;
                ctx.fill();
                ctx.stroke();

                ctx.restore();
            }
        });

        // Top banner text
        ctx.save();
        ctx.font = '11px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.textAlign = 'center';
        if (p >= 0.8) {
            ctx.fillStyle = '#34D399';
            ctx.shadowColor = '#34D399';
            ctx.shadowBlur = 8;
            ctx.fillText("🧠 C++ LibTorch 原生 SIMD 张量前向推理完成 (零 Python GIL 延迟)", w / 2, 22);
        } else if (this.state === 'running') {
            ctx.fillStyle = '#818CF8';
            ctx.fillText("Sequential 级联前向传播中 · 原生张量数据流转", w / 2, 22);
        } else {
            ctx.fillStyle = '#94A3B8';
            ctx.fillText("多层神经网络拓扑结构 · 点击【运行模拟】推演张量计算图", w / 2, 22);
        }
        ctx.restore();
    }

    // ------------------------------------------------------------------------
    // Scenario 3: Native Multithreading Parallel Swimlanes
    // ------------------------------------------------------------------------
    renderThreads(time, p) {
        const ctx = this.ctx;
        const w = this.width;
        const h = this.height;

        const lanes = [
            { tag: "T0 (Main)", name: "主分发调度器", color: '#38BDF8', freq: 4.5 },
            { tag: "W1 (Core 2)", name: "工作线程: 向量加法", color: '#818CF8', freq: 5.2 },
            { tag: "W2 (Core 3)", name: "工作线程: 矩阵乘法", color: '#A855F7', freq: 6.0 },
            { tag: "W3 (Core 4)", name: "工作线程: 异步 I/O 汇合", color: '#10B981', freq: 4.8 }
        ];

        const trackX = 110;
        const trackW = w - trackX - 35;
        const laneH = 46;
        const startY = (h / 2) - ((lanes.length * 68) / 2) + 20;

        lanes.forEach((lane, idx) => {
            const ly = startY + idx * 68;

            // Lane Tag Pill
            ctx.save();
            ctx.font = '10.5px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
            ctx.textAlign = 'right';
            ctx.textBaseline = 'middle';
            ctx.fillStyle = lane.color;
            ctx.fillText(lane.tag, trackX - 14, ly + laneH / 2 - 8);
            ctx.font = '9px -apple-system, BlinkMacSystemFont, sans-serif';
            ctx.fillStyle = '#94A3B8';
            ctx.fillText(lane.name, trackX - 14, ly + laneH / 2 + 8);
            ctx.restore();

            // Track Rail Background
            ctx.save();
            ctx.fillStyle = 'rgba(255, 255, 255, 0.035)';
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
            ctx.lineWidth = 1;
            this.drawRoundRect(ctx, trackX, ly, trackW, laneH, 10);
            ctx.fill();
            ctx.stroke();
            ctx.restore();

            // Active Progress Fill
            const laneProgress = this.state === 'idle'
                ? 0.15
                : Math.min(1, p * (1 + (idx % 2 === 0 ? 0.04 : -0.03)));

            const currentW = trackW * laneProgress;

            if (currentW > 0) {
                ctx.save();
                // Clip within track
                ctx.beginPath();
                this.drawRoundRect(ctx, trackX, ly, currentW, laneH, 10);
                ctx.clip();

                // Gradient track body
                const grad = ctx.createLinearGradient(trackX, 0, trackX + currentW, 0);
                grad.addColorStop(0, 'rgba(255, 255, 255, 0.03)');
                grad.addColorStop(0.85, lane.color + '44');
                grad.addColorStop(1, lane.color + 'AA');
                ctx.fillStyle = grad;
                ctx.fillRect(trackX, ly, currentW, laneH);

                // Oscillating Harmonic Waveform inside track
                ctx.beginPath();
                for (let x = 0; x <= currentW; x += 3) {
                    const waveAmp = (this.state === 'running' ? 12 : 5) * Math.sin((x / trackW) * Math.PI);
                    const wy = ly + laneH / 2 + Math.sin((x * 0.06) + (time * 0.006 * lane.freq) + idx) * waveAmp;
                    if (x === 0) ctx.moveTo(trackX + x, wy);
                    else ctx.lineTo(trackX + x, wy);
                }
                ctx.strokeStyle = lane.color;
                ctx.lineWidth = 2;
                ctx.shadowColor = lane.color;
                ctx.shadowBlur = 6;
                ctx.stroke();

                // Leading glowing laser head
                ctx.beginPath();
                ctx.arc(trackX + currentW - 2, ly + laneH / 2, 4.5, 0, Math.PI * 2);
                ctx.fillStyle = '#FFFFFF';
                ctx.shadowColor = lane.color;
                ctx.shadowBlur = 12;
                ctx.fill();

                ctx.restore();
            }
        });

        // Lock-free sync barrier guide line (flashes when completed)
        if (p >= 0.95) {
            ctx.save();
            ctx.beginPath();
            ctx.moveTo(trackX + trackW, startY - 10);
            ctx.lineTo(trackX + trackW, startY + lanes.length * 68 - 10);
            ctx.strokeStyle = '#34D399';
            ctx.lineWidth = 2.5;
            ctx.shadowColor = '#34D399';
            ctx.shadowBlur = 16;
            ctx.stroke();
            ctx.restore();
        }

        // Top Status
        ctx.save();
        ctx.font = '11px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.textAlign = 'center';
        if (p >= 0.95) {
            ctx.fillStyle = '#10B981';
            ctx.shadowColor = '#10B981';
            ctx.shadowBlur = 8;
            ctx.fillText("🧵 4 核心并发满载已汇合 (Barrier Join 零锁死)", w / 2, 22);
        } else if (this.state === 'running') {
            ctx.fillStyle = '#38BDF8';
            ctx.fillText("内核级 C++ std::thread 无锁并行调度中...", w / 2, 22);
        } else {
            ctx.fillStyle = '#94A3B8';
            ctx.fillText("多线程并发泳道 · 点击【运行模拟】推演无锁流水线", w / 2, 22);
        }
        ctx.restore();
    }

    // ------------------------------------------------------------------------
    // Scenario 4: 1 Million Loop & Hardware idiv Specialization
    // ------------------------------------------------------------------------
    renderLoop(time, p) {
        const ctx = this.ctx;
        const w = this.width;
        const h = this.height;

        const cx = w / 2;
        const cy = h / 2 - 5;
        const radius = Math.min(w, h) * 0.29;

        const isJitted = p >= 0.35;
        const countVal = Math.floor(p * 1000000);

        // Rotating Stator Ring
        const spinSpeed = isJitted ? 0.005 : 0.0015;
        const baseAngle = time * spinSpeed;

        ctx.save();
        ctx.translate(cx, cy);

        // Outer Ring segments
        const segments = 24;
        for (let i = 0; i < segments; i++) {
            const a1 = baseAngle + (i * Math.PI * 2) / segments;
            const a2 = a1 + (Math.PI * 2) / segments * 0.65;
            ctx.beginPath();
            ctx.arc(0, 0, radius, a1, a2);
            ctx.strokeStyle = isJitted 
                ? (i % 2 === 0 ? '#00F2FE' : '#38BDF8') 
                : 'rgba(255, 255, 255, 0.12)';
            ctx.lineWidth = isJitted ? 3 : 2;
            if (isJitted) {
                ctx.shadowColor = '#00F2FE';
                ctx.shadowBlur = 8;
            }
            ctx.stroke();
        }

        // Inner Core Card
        const cardW = 120;
        const cardH = 80;
        ctx.beginPath();
        this.drawRoundRect(ctx, -cardW / 2, -cardH / 2, cardW, cardH, 12);
        ctx.fillStyle = 'rgba(15, 23, 42, 0.85)';
        ctx.strokeStyle = isJitted ? '#00F2FE' : 'rgba(255, 255, 255, 0.16)';
        ctx.lineWidth = 1.5;
        if (isJitted) {
            ctx.shadowColor = '#00F2FE';
            ctx.shadowBlur = 12;
        }
        ctx.fill();
        ctx.stroke();

        // Core Text inside Card
        ctx.font = '11px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.textAlign = 'center';
        ctx.fillStyle = isJitted ? '#00F2FE' : '#94A3B8';
        ctx.fillText("TzdCore ALU", 0, -16);

        ctx.font = 'bold 15px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.fillStyle = isJitted ? '#FFFFFF' : '#E2E8F0';
        ctx.fillText(isJitted ? "idiv (x86_64)" : "fmod() [VM]", 0, 6);

        ctx.font = '9.5px -apple-system, BlinkMacSystemFont, monospace';
        ctx.fillStyle = '#34D399';
        ctx.fillText("FPToSI + SRem", 0, 24);

        ctx.restore();

        // Number Counter below ring
        ctx.save();
        ctx.font = 'bold 18px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.textAlign = 'center';
        ctx.fillStyle = isJitted ? '#34D399' : '#F8FAFC';
        if (isJitted) {
            ctx.shadowColor = '#34D399';
            ctx.shadowBlur = 10;
        }
        ctx.fillText(`循环计数: ${countVal.toLocaleString()} / 1,000,000`, cx, cy + radius + 32);

        ctx.font = '11px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.fillStyle = '#94A3B8';
        ctx.fillText("累加结果: sum = 249,999,500,000", cx, cy + radius + 52);
        ctx.restore();

        // Top Status text
        ctx.save();
        ctx.font = '11px -apple-system, BlinkMacSystemFont, "JetBrains Mono", monospace';
        ctx.textAlign = 'center';
        if (isJitted) {
            ctx.fillStyle = '#00F2FE';
            ctx.shadowColor = '#00F2FE';
            ctx.shadowBlur = 8;
            ctx.fillText("⚡ 取模特化: 单条硬件 idiv 指令 (超越 HotSpot 1.5x)", w / 2, 22);
        } else if (this.state === 'running') {
            ctx.fillStyle = '#38BDF8';
            ctx.fillText("Tier 0 循环计数器累计中 ... 即将触发热点", w / 2, 22);
        } else {
            ctx.fillStyle = '#94A3B8';
            ctx.fillText("100万次循环基准 · 点击【运行模拟】观察硬件整除特化", w / 2, 22);
        }
        ctx.restore();
    }

    drawRoundRect(ctx, x, y, width, height, radius) {
        ctx.beginPath();
        ctx.moveTo(x + radius, y);
        ctx.lineTo(x + width - radius, y);
        ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
        ctx.lineTo(x + width, y + height - radius);
        ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
        ctx.lineTo(x + radius, y + height);
        ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
        ctx.lineTo(x, y + radius);
        ctx.quadraticCurveTo(x, y, x + radius, y);
        ctx.closePath();
    }
}

// ----------------------------------------------------------------------------
// Interactive Playground Controller
// ----------------------------------------------------------------------------
function initPlayground() {
    const editor = document.getElementById('pg-editor');
    const terminal = document.getElementById('pg-output');
    const runBtn = document.getElementById('pg-run-btn');
    const clearBtn = document.getElementById('pg-clear-btn');
    const canvas = document.getElementById('pg-visual-canvas');
    const idleHint = document.getElementById('pg-canvas-idle-hint');
    const hudOverlay = document.getElementById('pg-canvas-hud');
    const termOverlay = document.getElementById('pg-term-overlay');
    const statusPill = document.getElementById('pg-status-pill');
    const statusDot = document.getElementById('pg-status-dot');
    const statusText = document.getElementById('pg-status-text');

    // Micro Status Bar Elements
    const statJitTier = document.getElementById('stat-jit-tier');
    const statExecTime = document.getElementById('stat-exec-time');
    const statMem = document.getElementById('stat-mem');
    const statSpeedup = document.getElementById('stat-speedup');
    const statFps = document.getElementById('stat-fps');

    // Dual-mode switcher tabs
    const tabVisual = document.getElementById('pg-tab-visual');
    const tabTerminal = document.getElementById('pg-tab-terminal');
    const viewOutputSubtab = document.getElementById('pg-view-output');
    const viewBytecodeSubtab = document.getElementById('pg-view-bytecode');

    // Custom Dropdown Elements
    const dropdownContainer = document.getElementById('pg-custom-dropdown');
    const dropdownTrigger = document.getElementById('pg-dropdown-trigger');
    const triggerLabel = document.getElementById('pg-trigger-label');
    const triggerIcon = document.getElementById('pg-trigger-icon');
    const dropdownMenu = document.getElementById('pg-dropdown-menu');
    const dropdownOptions = document.querySelectorAll('.apple-dropdown-option');

    if (!editor || !canvas || !runBtn) return;

    let currentPreset = 'fib';
    let currentTermView = 'output'; // 'output' or 'bytecode'

    // Initialize HTML5 Canvas Visual Engine
    const visualEngine = new TzdDynamicVisualEngine(canvas, {
        onProgress: (p, rawP) => {
            const data = playgroundPresets[currentPreset];
            if (!data) return;

            // Update live metrics bar
            if (rawP < 0.44) {
                if (statJitTier) statJitTier.textContent = "Tier 0 (VM)";
            } else {
                if (statJitTier) statJitTier.textContent = data.metrics.jitTier;
            }

            if (statExecTime) {
                const targetMs = parseFloat(data.metrics.execTime);
                const currentMs = (targetMs * p).toFixed(4);
                statExecTime.textContent = currentMs + "s";
            }
        },
        onFps: (fpsVal) => {
            if (statFps) statFps.textContent = fpsVal + " FPS";
        },
        onComplete: (presetKey) => {
            const data = playgroundPresets[presetKey];
            if (!data) return;

            // Status Pill
            if (statusDot) {
                statusDot.className = 'status-dot completed';
            }
            if (statusText) statusText.textContent = "执行完成 (Tier 1 Native)";

            // Micro stats final values
            if (statJitTier) statJitTier.textContent = data.metrics.jitTier;
            if (statExecTime) statExecTime.textContent = data.metrics.execTime;
            if (statMem) statMem.textContent = data.metrics.mem;
            if (statSpeedup) statSpeedup.textContent = data.metrics.speedup;

            // Show HUD Card Overlay
            if (hudOverlay) {
                const hudIcon = document.getElementById('hud-icon');
                const hudTitle = document.getElementById('hud-title');
                const hudSubtitle = document.getElementById('hud-subtitle');
                const hudMetric = document.getElementById('hud-metric');

                if (hudIcon) hudIcon.textContent = data.metrics.hudIcon;
                if (hudTitle) hudTitle.textContent = data.metrics.hudTitle;
                if (hudSubtitle) hudSubtitle.textContent = data.metrics.hudSubtitle;
                if (hudMetric) hudMetric.textContent = data.metrics.hudMetric;

                hudOverlay.classList.add('show');
            }
        }
    });

    function loadPreset(key) {
        currentPreset = key;
        const data = playgroundPresets[key];
        if (!data) return;

        editor.value = data.code;
        if (terminal) {
            terminal.innerHTML = `<span class="out-sys">[就绪] 点击“运行模拟”观察动态渲染视窗与 JIT 提升...</span>`;
        }

        // Reset visual engine
        visualEngine.setPreset(key);

        // Reset HUD & Idle hints
        if (hudOverlay) hudOverlay.classList.remove('show');
        if (idleHint) idleHint.classList.remove('hide');

        // Reset status pill
        if (statusDot) statusDot.className = 'status-dot';
        if (statusText) statusText.textContent = "就绪 (Ready)";

        // Reset micro stats bar
        if (statJitTier) statJitTier.textContent = "Tier 0 (VM)";
        if (statExecTime) statExecTime.textContent = "--";
        if (statMem) statMem.textContent = data.metrics.mem;
        if (statSpeedup) statSpeedup.textContent = "--";
    }

    // Custom Dropdown Interactions
    if (dropdownContainer && dropdownTrigger && dropdownMenu) {
        dropdownTrigger.addEventListener('click', (e) => {
            e.stopPropagation();
            const isOpen = dropdownContainer.classList.contains('open');
            if (isOpen) {
                dropdownContainer.classList.remove('open');
                dropdownTrigger.setAttribute('aria-expanded', 'false');
            } else {
                dropdownContainer.classList.add('open');
                dropdownTrigger.setAttribute('aria-expanded', 'true');
            }
        });

        dropdownOptions.forEach(opt => {
            opt.addEventListener('click', (e) => {
                e.stopPropagation();
                dropdownOptions.forEach(o => o.classList.remove('selected'));
                opt.classList.add('selected');

                const val = opt.dataset.value;
                const data = playgroundPresets[val];
                if (data) {
                    if (triggerLabel) triggerLabel.textContent = data.title;
                    if (triggerIcon) triggerIcon.textContent = data.icon;
                    loadPreset(val);
                }

                dropdownContainer.classList.remove('open');
                dropdownTrigger.setAttribute('aria-expanded', 'false');
            });
        });

        document.addEventListener('click', (e) => {
            if (!dropdownContainer.contains(e.target)) {
                dropdownContainer.classList.remove('open');
                dropdownTrigger.setAttribute('aria-expanded', 'false');
            }
        });

        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && dropdownContainer.classList.contains('open')) {
                dropdownContainer.classList.remove('open');
                dropdownTrigger.setAttribute('aria-expanded', 'false');
            }
        });
    }

    // Run button triggers simulation
    runBtn.addEventListener('click', () => {
        // Hide idle hint & HUD
        if (idleHint) idleHint.classList.add('hide');
        if (hudOverlay) hudOverlay.classList.remove('show');

        // Switch status pill
        if (statusDot) statusDot.className = 'status-dot running';
        if (statusText) statusText.textContent = "正在模拟运行...";

        // Start visual canvas engine
        visualEngine.startSimulation();

        // Populate terminal in background simultaneously
        if (terminal) {
            const data = playgroundPresets[currentPreset];
            if (currentTermView === 'output') {
                const lines = (data ? data.output : ">>> 执行完成。").split('\n');
                terminal.innerHTML = '';
                lines.forEach((line, idx) => {
                    setTimeout(() => {
                        let colorClass = 'out-sys';
                        if (line.includes('Tier') || line.includes('Profiler') || line.includes('LLVM') || line.includes('GC') || line.includes('Torch')) colorClass = 'out-info';
                        else if (line.includes('结果') || line.includes('耗时') || line.includes('正常结束') || line.includes('完成') || line.includes('输出')) colorClass = 'out-success';
                        else if (line.includes('警告') || line.includes('Err')) colorClass = 'out-warn';

                        terminal.innerHTML += `<div class="${colorClass}">${escapeHtml(line)}</div>`;
                        terminal.scrollTop = terminal.scrollHeight;
                    }, idx * 60);
                });
            } else {
                terminal.innerHTML = `<pre class="out-info">${escapeHtml(data ? data.bytecode : "; No bytecode available")}</pre>`;
            }
        }
    });

    // Reset button
    if (clearBtn) {
        clearBtn.addEventListener('click', () => {
            loadPreset(currentPreset);
        });
    }

    // Dual-Mode View Switcher (Dynamic Visual Engine vs Terminal/IR)
    if (tabVisual && tabTerminal && termOverlay) {
        tabVisual.addEventListener('click', () => {
            tabVisual.classList.add('active');
            tabTerminal.classList.remove('active');
            termOverlay.style.display = 'none';
        });

        tabTerminal.addEventListener('click', () => {
            tabTerminal.classList.add('active');
            tabVisual.classList.remove('active');
            termOverlay.style.display = 'flex';
        });
    }

    // Terminal Sub-tabs (Output vs Bytecode)
    if (viewOutputSubtab && viewBytecodeSubtab && terminal) {
        viewOutputSubtab.addEventListener('click', () => {
            currentTermView = 'output';
            viewOutputSubtab.classList.add('active');
            viewBytecodeSubtab.classList.remove('active');
            const data = playgroundPresets[currentPreset];
            terminal.innerHTML = `<div class="out-sys">${escapeHtml(data.output)}</div>`;
        });
        viewBytecodeSubtab.addEventListener('click', () => {
            currentTermView = 'bytecode';
            viewBytecodeSubtab.classList.add('active');
            viewOutputSubtab.classList.remove('active');
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
// 4. Scroll Reveal with Staggered Cascades (IntersectionObserver)
// ----------------------------------------------------------------------------
function initScrollReveal() {
    const revealElements = document.querySelectorAll('.reveal-on-scroll');
    if (!revealElements.length) return;

    if (!('IntersectionObserver' in window)) {
        revealElements.forEach(el => el.classList.add('is-revealed'));
        return;
    }

    const observer = new IntersectionObserver((entries, obs) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.classList.add('is-revealed');
                obs.unobserve(entry.target);
            }
        });
    }, {
        threshold: 0.1,
        rootMargin: '0px 0px -40px 0px'
    });

    revealElements.forEach(el => observer.observe(el));
}

// ----------------------------------------------------------------------------
// 5. Smooth Scrolling & Sticky Sidebar Spy
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
    const sidebarLinks = document.querySelectorAll('.doc-sidebar-sticky a');
    const sections = document.querySelectorAll('.syntax-pod-card[id]');

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
// 6. Mobile Navigation
// ----------------------------------------------------------------------------
function initMobileNav() {
    const toggle = document.querySelector('.mobile-toggle');
    const nav = document.querySelector('.nav-links-row');
    if (!toggle || !nav) return;

    toggle.addEventListener('click', () => {
        const isShown = nav.style.display === 'flex';
        nav.style.display = isShown ? 'none' : 'flex';
        if (!isShown) {
            nav.style.flexDirection = 'column';
            nav.style.position = 'absolute';
            nav.style.top = '100%';
            nav.style.left = '1.5rem';
            nav.style.right = '1.5rem';
            nav.style.background = 'rgba(18, 20, 29, 0.95)';
            nav.style.backdropFilter = 'blur(20px)';
            nav.style.webkitBackdropFilter = 'blur(20px)';
            nav.style.padding = '1.5rem';
            nav.style.border = '1px solid rgba(255, 255, 255, 0.12)';
            nav.style.borderRadius = '18px';
            nav.style.marginTop = '0.5rem';
            nav.style.boxShadow = '0 20px 40px rgba(0, 0, 0, 0.6)';
        }
    });
}

// ----------------------------------------------------------------------------
// 7. Apple Xcode Documentation Workbench & Line Numbers Engine
// ----------------------------------------------------------------------------
function initSyntaxWorkbench() {
    const navItems = document.querySelectorAll('.xcode-nav-item');
    const panes = document.querySelectorAll('.xcode-doc-pane');
    const activeFileEl = document.getElementById('workbench-active-file');
    const activeTagEl = document.getElementById('workbench-active-tag');
    const copyBtn = document.getElementById('workbench-copy-code');

    function switchWorkbenchTopic(topicId, updateHistory = true) {
        let matchedItem = null;
        navItems.forEach(item => {
            const itemTopic = item.getAttribute('data-topic');
            if (itemTopic === topicId || itemTopic === `syntax-${topicId}` || `syntax-${itemTopic}` === topicId) {
                item.classList.add('active');
                matchedItem = item;
            } else {
                item.classList.remove('active');
            }
        });

        const targetPaneId = `pane-${matchedItem ? matchedItem.getAttribute('data-topic') : topicId}`;
        panes.forEach(pane => {
            if (pane.id === targetPaneId || pane.id === topicId) {
                pane.classList.add('active');
            } else {
                pane.classList.remove('active');
            }
        });

        if (matchedItem) {
            if (activeFileEl) {
                activeFileEl.textContent = matchedItem.getAttribute('data-file') || 'specification.tzd';
            }
            if (activeTagEl) {
                activeTagEl.textContent = matchedItem.getAttribute('data-tag') || 'Standard';
            }
            if (updateHistory) {
                const topicKey = matchedItem.getAttribute('data-topic');
                history.replaceState(null, null, `#${topicKey}`);
            }
        }
    }

    navItems.forEach(item => {
        item.addEventListener('click', () => {
            const topic = item.getAttribute('data-topic');
            if (topic) {
                switchWorkbenchTopic(topic, true);
            }
        });
    });

    // Check URL Hash on Load
    if (window.location.hash) {
        const hash = window.location.hash.substring(1);
        if (hash.startsWith('syntax-') || hash === 'syntax') {
            if (hash !== 'syntax') {
                switchWorkbenchTopic(hash, false);
            }
        }
    }

    // Topbar One-Click Code Copy
    if (copyBtn) {
        copyBtn.addEventListener('click', () => {
            const activePane = document.querySelector('.xcode-doc-pane.active');
            const codeEl = activePane ? activePane.querySelector('code') : null;
            if (codeEl) {
                const text = codeEl.textContent;
                const copySuccess = () => {
                    const label = copyBtn.querySelector('.copy-text-label');
                    if (label) label.textContent = '已复制!';
                    setTimeout(() => {
                        if (label) label.textContent = '复制代码';
                    }, 2000);
                };

                if (navigator.clipboard && navigator.clipboard.writeText) {
                    navigator.clipboard.writeText(text).then(copySuccess).catch(() => {
                        fallbackCopy(text);
                        copySuccess();
                    });
                } else {
                    fallbackCopy(text);
                    copySuccess();
                }
            }
        });
    }

    function fallbackCopy(text) {
        const ta = document.createElement('textarea');
        ta.value = text;
        ta.style.position = 'fixed';
        ta.style.left = '-9999px';
        document.body.appendChild(ta);
        ta.select();
        document.execCommand('copy');
        document.body.removeChild(ta);
    }

    // Auto-generate line numbers for all xcode code blocks
    initCodeLineNumbers();
}

function initCodeLineNumbers() {
    document.querySelectorAll('.xcode-code-block').forEach(block => {
        if (block.querySelector('.code-line-numbers')) return;

        const codeEl = block.querySelector('code');
        if (!codeEl) return;

        const text = codeEl.textContent;
        const rawLines = text.replace(/\r\n/g, '\n').replace(/\r/g, '\n').split('\n');
        let lineCount = rawLines.length;
        if (lineCount > 1 && rawLines[rawLines.length - 1].trim() === '') {
            lineCount--;
        }

        const numContainer = document.createElement('div');
        numContainer.className = 'code-line-numbers';
        numContainer.setAttribute('aria-hidden', 'true');

        let spans = '';
        for (let i = 1; i <= lineCount; i++) {
            spans += `<span>${i}</span>`;
        }
        numContainer.innerHTML = spans;
        block.insertBefore(numContainer, block.firstChild);
    });
}
