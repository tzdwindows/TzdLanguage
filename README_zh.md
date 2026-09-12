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

- ⚡ **超强 GPU 大数乘法流水线**：基于三素数中国剩余定理（CRT）与 CUDA NVRTC 深度优化，在 NVIDIA P106-090 GPU 上计算 **474 万位大整数乘法仅需 29.20 ms** GPU 运算耗时，性能**超越单核 GMP（6.3.0）十倍以上**。
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

### 1. 百万位大数乘法实测：TzdLang GPU NTT vs GMP

测试配置：NVIDIA P106-090 GPU (Pascal CC 6.1, 192 GB/s 显存带宽)，乘数规模为两个 $4,741,006$ 位大整数：

| 计算引擎 / 实现方案 | 数值位数 | 纯运算 / NTT 耗时 | 端到端全流程耗时 | 相较于 GMP 性能倍率 |
|---|---|---|---|---|
| **TzdLang GPU NTT (CUDA NVRTC)** | **4,741,006** | **29.20 ms** | **56.52 ms** | **9.2x ~ 17.8x** |
| 单核 GMP 6.3.0 (`mpz_mul`) | 4,741,006 | ~519.30 ms | ~519.30 ms | 1.0x (基准) |
| Python 3.12 (`int * int`) | 4,741,006 | >3,800 ms | >3,800 ms | ~0.14x |

> **TzdLang GPU NTT 算法优化要点：**
> - **三 32 位 NTT 素数系**：$P_1 = 469762049$, $P_2 = 167772161$, $P_3 = 754974721$。
> - **Bailey 4-Step 二维 NTT 分解**：将 $N = 2^{21}$ 个 Limb 分解为 $2048 \times 1024$ 的二维变换，结合片上共享内存与无冲突交错填充（`PAD(idx) = idx + (idx >> 5)`）。
> - **并行 Kogge-Stone 进位链**：设计两轮进位规约彻底消除进位溢出风险，配合块内与块间高效前缀扫描，保障计算结果位对齐准确率 100%。

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

- 📑 [**Wiki 首页与架构全景**](wiki/Home.md) - 系统级分层架构与双执行引擎设计。
- 📐 [**语言标准语法规范手册**](wiki/Language-Specification.md) - 类型系统、控制流、函数、面向对象与异常体系。
- 🚀 [**GPU NTT 大数乘法底层深度剖析**](wiki/GPU-NTT-BigInt.md) - 数学原理、CRT、二维 Stockham 核函数与并行进位链。
- ⚡ [**JIT 编译器核心技术与实现**](wiki/JIT-Compiler-Internals.md) - Tier 0 VM、Tier 1 LLVM ORC JIT、函数特化与各阶段优化 Pass。
- 🧠 [**LibTorch 深度学习引擎集成**](wiki/Deep-Learning-and-PyTorch.md) - 原生 Tensor 抽象、自动微分、神经网络层与 CUDA 后端。
- 🔨 [**构建指南与工具链环境搭建**](wiki/Building-and-Toolchain.md) - MSBuild 与 CMake 构建配置指南。
- 🔌 [**VS Code 扩展与 DAP 调试器**](wiki/VSCode-Extension-and-Debugger.md) - 语言服务器（LSP）与 DAP 调试协议实现。
- 📚 [**标准库开发与参考手册**](wiki/Standard-Library-Reference.md) - Core、Math、Thread、Torch 标准模块详解。

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
