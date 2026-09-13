# TzdTools & TzdLang (TZD)

<p align="center">
  <strong>现代化、高性能面向对象混合编译编程语言与工业级工具链</strong>
</p>

<p align="center">
  <a href="README.md"><strong>English</strong></a> | <a href="README_zh.md"><strong>中文说明文档</strong></a>
</p>

<p align="center">
  <a href="https://tzdwindows.github.io/TzdLanguage/"><img src="https://img.shields.io/badge/官方文档-在线站点-38bdf8?style=flat&logo=github" alt="Docs"></a>
  <a href="wiki/JIT-Compiler-Internals.md"><img src="https://img.shields.io/badge/JIT编译-LLVM_ORC-6366f1?style=flat" alt="LLVM JIT"></a>
  <a href="wiki/GPU-NTT-BigInt.md"><img src="https://img.shields.io/badge/大数乘法-CUDA_GPU_NTT-76b900?style=flat&logo=nvidia" alt="CUDA NTT"></a>
  <a href="wiki/Deep-Learning-and-PyTorch.md"><img src="https://img.shields.io/badge/深度学习-LibTorch-ee4c2c?style=flat&logo=pytorch" alt="LibTorch"></a>
  <a href="https://github.com/tzdwindows/TzdLanguage/blob/master/LICENSE"><img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License"></a>
</p>

---

## 🌟 核心特性概览

**TzdLang (TZD)** 是一个自研的现代面向对象、高性能混合编译编程语言与开发工具链。系统融合了轻量字节码虚拟机、基于 LLVM ORC 的异步分层 JIT 编译系统、原生 LibTorch 深度学习引擎、基于 CUDA 的超高性能 GPU 大数乘法算子以及完整的 VSCode IDE 扩展生态。

- ⚡ **超强双模大数乘法流水线（GPU NTT 与 CPU 极限 NTT）**：
  - **CUDA GPU NTT**：基于三素数中国剩余定理（CRT）与 CUDA NVRTC 深度优化，在 NVIDIA P106-090 GPU 上计算 **474 万位大整数乘法仅需 29.20 ms** GPU 运算耗时（端到端 56.52 ms），**纯算超越单核 GMP 3.83 倍，端到端领先 45.1 倍**。
  - **CPU 极限 NTT (`--experimental-compute`)**：直击 x86_64 硬件物理极限，采用三素数 Montgomery AVX2 向量化模乘（单指令并发 8 通道）、$64 \times 64$ L1/L2 缓存分块 4-Step 矩阵转置、Direct Garner CRT 重构与定点数倒数无除法极速转换。在普通 4 核 CPU 上计算 **474 万位大数乘法纯算仅需 54.02 ms（超越单核 GMP 2.07 倍），端到端全流程仅需 175.41 ms（较原生 GMP 快 14.5 倍，较多线程 GMP 快 3 倍）**！
- 🚀 **分级混合执行架构（Tiered Execution）**：
  - **Tier 0 解释器/字节码虚拟机**：启动极快、占用显存极低、开箱即跑；
  - **Tier 1 LLVM ORC JIT 编译器**：支持函数特化（生成原生 double worker）、部分求值（Partial Evaluation）、LLVM `mem2reg`、公共子表达式消除（CSE）、无死角死代码消除与侵略性内联，**在循环与函数调用开销基准测试中超越 JDK 20 HotSpot**。
- 🧠 **原生 LibTorch 深度学习体系**：支持语言级一等公民张量（Tensor）、自动微分（Autograd）、神经网络常用模块（`nn.Linear`, `nn.Sequential`）、经典优化器（SGD, Adam, AdamW）以及 GPU 显存直通加速。
- 💎 **现代面向对象机制**：支持单继承、虚方法动态多态、构造函数级联（`super`）、动态类型与可选静态强类型标注。
- 🛡️ **健全的异常处理与并发模型**：结构化 `try-catch-throw` 异常体系、`in` 类型模式匹配以及原生多线程并发驱动（`Thread`）。
- 🛠️ **工业级 VS Code 扩展与 DAP 调试器**：支持语法高亮、代码补全、断点管理、单步步入/步过、调用栈查看与变量实时监视。
- 📦 **双构建体系支持**：原生 Visual Studio 工程（`.sln` / `.vcxproj`）与独立的跨平台 `CMakeLists.txt`。

---

## ⚡ 性能基准测试报告

### 1. 百万位大数乘法实测：TzdLang vs GMP vs Python

测试硬件：Intel Core i7-4790 CPU (4 核 8 线程 @ 3.60GHz) 与 NVIDIA P106-090 GPU (Pascal CC 6.1, 192 GB/s 显存带宽)，乘数规模为两个 $4,741,006$ 位大整数：

| 计算引擎 / 实现方案 | 数值位数 | 核心纯运算耗时 | 端到端全流程耗时 | 相较于单核 GMP 纯算倍率 | 端到端全流程倍率 |
|---|---|---|---|---|---|
| **TzdLang GPU NTT (CUDA)** | **4,741,006** | **29.20 ms** | **56.52 ms** | **3.83x** | **45.1x** |
| **TzdLang CPU NTT (`--experimental-compute`)** | **4,741,006** | **54.02 ms** | **175.41 ms** | **2.07x** | **14.5x** |
| 多线程 GMP (8 线程 Karatsuba) | 4,741,006 | 84.58 ms | 2,522.09 ms | 1.32x | 1.01x |
| 单核 GMP 6.3.0 (`mpz_mul`) | 4,741,006 | 111.78 ms | 2,549.30 ms | 1.0x (基准) | 1.0x (基准) |
| Python 3.12 (`int * int`) | 4,741,006 | >3,800 ms | >3,800 ms | ~0.03x | ~0.01x |

> **TzdLang NTT 核心算法优化要点：**
> - **三 32 位 NTT 素数系**：$P_1 = 469762049$, $P_2 = 167772161$, $P_3 = 754974721$。
> - **Bailey 4-Step 二维 NTT 分解**：将 $N = 2^{21}$ 个 Limb 分解为 $2048 \times 1024$ 的二维变换。CPU 上采用 $64 \times 64$ L1/L2 缓存瓦片分块转置与 AVX2 SIMD，缓存命中率达 99.8%；GPU 上结合片上共享内存与无冲突交错填充（`PAD(idx) = idx + (idx >> 5)`）。
> - **并行 Kogge-Stone 进位链与 Direct Garner CRT**：设计两轮进位规约彻底消除进位溢出风险，前缀扫描树以 $O(\log N)$ 深度瞬间完成进位广播。
> - **定点数倒数无除法进制转换**：采用定点数倒数乘法（`fast_div_1e9`）消除硬件除法，474 万位大数字符串解析仅需 10ms，结果格式化输出仅需 13ms。

### 2. JIT 微基准对比：TzdLang vs JDK 20 HotSpot

| 测试项 | TzdLang (LLVM JIT) | JDK 20 (HotSpot C2) | 性能对比 |
|---|---|---|---|
| 函数调用开销 `callOverhead(1M)` | **0.002 s** | 0.005 s | **TzdLang 快 2.5x** |
| 嵌套循环 `nestedLoop(1k × 1k)` | **0.003 s** | 0.005 s | **TzdLang 快 1.7x** |
| 累加循环 `sumLoop(1M)` | **0.002 s** | 0.003 s | **TzdLang 快 1.5x** |
| 阿克曼函数 `Ackermann(3, 6)` | **0.001 s** | 0.001 s | **TzdLang 快 1.2x** |
| 牛顿开平方根 `sqrt(100k)` | **0.000006 s** | 0.000008 s | **TzdLang 快 1.3x** |
| 递归斐波那契 `fib(35)` | 0.197 s | 0.061 s | JDK 略快 |

---

## 🚀 快速上手

### 1. 环境依赖

- **操作系统**：Windows 10 / 11 (x64)
- **编译工具链**：
  - Visual Studio 2022 / 2026 (MSVC v143 / v145)，开启 C++20 支持
  - 或独立的 **CMake 3.20+**
- **可选加速组件**：
  - NVIDIA CUDA Toolkit 12.0+（驱动需支持算力 $\ge 6.0$ 的显卡）
  - LibTorch（位于 `External/libtorch` 或系统环境）
  - LLVM SDK（用于启用 ORC JIT 实时编译执行）

### 2. 从源码编译

#### 方式 A：使用 Visual Studio MSBuild 编译（推荐）
```cmd
git clone https://github.com/tzdwindows/TzdLanguage.git
cd TzdLanguage

:: 使用 MSBuild 编译 Release x64
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" TzdTools.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

#### 方式 B：使用 CMake 独立构建
```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --parallel
```

### 3. 运行脚本

```cmd
:: 使用默认解释器执行脚本
TzdTools.exe --runMainTzd="examples/test.tzd"

:: 启用 LLVM ORC JIT 极速编译执行
TzdTools.exe --jit --runMainTzd="bench.tzd"

:: 启用 GPU 加速执行大数乘法运算并输出详细耗时统计
TzdTools.exe --runMainTzd="大数.tzd" --forceGPU --bigTime

:: 启用 CPU 极限数论变换引擎 (--experimental-compute) 执行大数乘法并输出统计
TzdTools.exe --runMainTzd="大数.tzd" --experimental-compute --bigTime
```

---

## 💻 基础语法速览

### 变量与函数定义
```tzd
// 变量声明（支持推导或指定类型）
var x = 42;
var string greeting = "你好，TzdLang！";

// 函数定义
fun add(a, b) {
    return a + b;
}

print(greeting + " 结果: " + toString(add(x, 8)));
```

### 面向对象编程（类与继承）
```tzd
class Animal {
    var string name;
    Animal(name) {
        this.name = name;
    }
    fun speak() {
        print(this.name + " 正在发声...");
    }
}

class Dog extends Animal {
    Dog(name) : super(name) {}
    fun speak() {
        print(this.name + " 汪汪叫！");
    }
}

var pet = new Dog("旺财");
pet.speak(); // 输出: 旺财 汪汪叫！
```

### 深度学习与张量操作（原生 LibTorch）
```tzd
import "stdlib/torch/nn.tzd";

// 创建张量
var a = torch_randn([3, 3]);
var b = torch_eye(3);
var c = torch_matmul(a, b);

print("张量形状: " + toString(c.shape));
print("迁移至 GPU: " + toString(c.cuda()));
```

### 异常处理与类型匹配
```tzd
import "core/Error.tzd";

try {
    throw new Error("磁盘读取故障", "E_IO");
} catch (err) {
    if (err in Error) {
        print("捕获到异常 [" + err.code + "]: " + err.message);
    }
}
```

### 原生多线程
```tzd
import "thread/Thread.tzd";

fun worker() {
    for (i = 0; i < 5; i++) {
        print("工作线程执行中: " + toString(i));
        sleep(500);
    }
}

var t = new Thread(worker);
t.start();
t.join();
```

---

## 📖 技术 Wiki 文档导航

完整系统的底层架构与设计文档已全部归档至 [`wiki/`](wiki/) 目录：

- 📑 [**Wiki 首页与架构全景**](wiki/Home-zh.md) - 系统级分层架构与双执行引擎设计。
- 📐 [**语言标准语法规范手册**](wiki/Language-Specification-zh.md) - 类型系统、控制流、函数、面向对象与异常体系。
- 🚀 [**GPU NTT 大数乘法底层深度剖析**](wiki/GPU-NTT-BigInt-zh.md) - 数学原理、CRT、二维 Stockham 核函数与并行进位链。
- 🏎️ [**CPU NTT 极限计算引擎底层剖析 (--experimental-compute)**](wiki/CPU-NTT-BigInt-zh.md) - 三素数 Montgomery AVX2 向量化、4-Step 缓存分块转置、Direct Garner CRT、Kogge-Stone 进位链。
- ⚡ [**JIT 编译器核心技术与实现**](wiki/JIT-Compiler-Internals-zh.md) - Tier 0 VM、Tier 1 LLVM ORC JIT、-O0~-O3 优化等级、混合多级内联流水线与零开销调试选择性回退。
- 🧠 [**LibTorch 深度学习引擎集成**](wiki/Deep-Learning-and-PyTorch-zh.md) - 原生 Tensor 抽象、自动微分、神经网络层与 CUDA 后端。
- 🔨 [**构建指南与工具链环境搭建**](wiki/Building-and-Toolchain-zh.md) - MSBuild 与 CMake 构建配置指南。
- 🔌 [**VS Code 扩展与 DAP 调试器**](wiki/VSCode-Extension-and-Debugger-zh.md) - 官方插件 v0.2.3、JIT 调试支持、选择性回退断点、JIT Engine 变量面板与 LLVM IR 导出。
- 📚 [**标准库开发与参考手册**](wiki/Standard-Library-Reference-zh.md) - Core、Math、Thread、Torch 标准模块详解。
- 📖 [**自带内置函数自查大全**](wiki/Builtin-Functions-Reference-zh.md) - 350+ 个原生内置函数与算子速查手册。
- 🎛️ [**启动参数与命令行体系完整参考手册**](wiki/CLI-and-Startup-Options-zh.md) - 全量命令行参数、执行引擎选项与底层系统指令详解。
- 🛡️ [**AOT 原生独立机器码编译器**](wiki/AOT-Compiler-zh.md) - 真正 AOT 独立机器码编译、零外部 DLL 依赖（仅依赖 KERNEL32）、~300KB 极致受控体积、动态字符进度条与多级优化支持。

---

## 📁 代码工程目录结构

```text
TzdTools/
├── CMakeLists.txt             # 独立的 CMake 构建配置文件
├── TzdTools.sln               # Visual Studio 解决方案
├── TzdTools.vcxproj           # Visual Studio 工程文件
├── README.md                  # 英文主文档 (默认)
├── README_zh.md               # 中文主文档
├── wiki/                      # 深度技术 Wiki 系列文档
├── Generated/                 # ANTLR4 解析器、AST 遍历器、VM、JIT、PyTorch 源码
│   ├── TzdInterpreter.cpp     # AST 与 VM 运行时执行引擎
│   ├── TzdExperimentalCompute.cpp # 高性能 CPU AVX2 Montgomery NTT 大数引擎
│   ├── TzdExperimentalCompute.h   # 实验性运算引擎头文件与对外接口
│   ├── TzdJit.cpp             # LLVM ORC JIT 核心编译器
│   ├── TzdPyTorch.cpp         # LibTorch 绑定与 GPU NTT 大数乘法算子
│   └── ...
├── dyncall/                   # C 语言动态 FFI 与 x86_64 汇编桥接
├── Plots/                     # pbPlots 原生图表绘制引擎
├── stdlib/                    # TzdLang 标准库实现
│   ├── core/                  # Error, IO, 反射机制
│   ├── thread/                # 线程并发库
│   └── torch/                 # 深度学习模块与算子
├── vscodePlugin/              # VS Code 扩展 (DAP 调试、语法高亮、LSP)
└── examples/                  # 示例脚本与基准测试代码
```

---

## 🤝 参与贡献

欢迎提交 Issue 和 Pull Request！
1. Fork 本仓库；
2. 新建特性分支 (`git checkout -b feature/MyFeature`)；
3. 提交修改 (`git commit -m 'feat: Add MyFeature'`)；
4. 推送分支 (`git push origin feature/MyFeature`)；
5. 发起 Pull Request。

---

## 📄 开源许可证

本项目基于 **MIT 许可证** 授权。详情参见 `LICENSE`。
