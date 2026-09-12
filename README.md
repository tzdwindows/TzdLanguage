# TzdTools & TzdLang (TZD)

<p align="center">
  <strong>A Modern, High-Performance Object-Oriented Hybrid Programming Language and Toolchain</strong>
</p>

<p align="center">
  <a href="README.md"><strong>English</strong></a> | <a href="README_zh.md"><strong>中文说明文档</strong></a>
</p>

<p align="center">
  <a href="https://tzdwindows.github.io/TzdLanguage/"><img src="https://img.shields.io/badge/Documentation-Online_Website-38bdf8?style=flat&logo=github" alt="Docs"></a>
  <a href="wiki/JIT-Compiler-Internals.md"><img src="https://img.shields.io/badge/JIT-LLVM_ORC-6366f1?style=flat" alt="LLVM JIT"></a>
  <a href="wiki/GPU-NTT-BigInt.md"><img src="https://img.shields.io/badge/BigInt-CUDA_GPU_NTT-76b900?style=flat&logo=nvidia" alt="CUDA NTT"></a>
  <a href="wiki/Deep-Learning-and-PyTorch.md"><img src="https://img.shields.io/badge/Deep_Learning-LibTorch-ee4c2c?style=flat&logo=pytorch" alt="LibTorch"></a>
  <a href="https://github.com/tzdwindows/TzdLanguage/blob/master/LICENSE"><img src="https://img.shields.io/badge/License-MIT-green.svg" alt="License"></a>
</p>

---

## 🌟 Highlights & Key Features

**TzdLang (TZD)** is an independently designed, modern object-oriented programming language with high-performance hybrid execution pipelines. It seamlessly integrates a lightweight bytecode virtual machine, an asynchronous tiered LLVM ORC JIT compiler, native PyTorch tensor operations, ultra-fast GPU-accelerated BigInt arithmetic, and a full-featured VS Code IDE development ecosystem.

- ⚡ **World-Class GPU BigInt Multiplication**: Powered by a custom 3-prime Chinese Remainder Theorem (CRT) Number Theoretic Transform (NTT) on CUDA. Multiplies **4.74-million-digit** integers in **29.20 ms** GPU computation time (**>10x faster than single-core GMP**).
- 🚀 **Tiered Hybrid Compilation**:
  - **Tier 0**: Low-latency, compact stack-based Bytecode VM.
  - **Tier 1**: Asynchronous LLVM ORC JIT compiler featuring function specialization (native double workers), partial evaluation, `mem2reg`, CSE, and aggressive inlining. **Outperforms JDK 20 HotSpot** in function call overhead and tight loop benchmarks.
- 🧠 **Native LibTorch Deep Learning Engine**: First-class Tensor types, autograd, neural network modules (`nn.Linear`, `nn.Sequential`), optimizers (SGD, Adam, AdamW), and GPU tensors directly within the language.
- 💎 **Modern Object-Oriented Semantics**: Single inheritance, virtual method polymorphism, constructor cascading (`super`), dynamic typing with optional static typing.
- 🛡️ **Robust Error & Concurrency Model**: Structured `try-catch-throw` exception handling, `in` type pattern matching, and native OS multi-threading (`Thread`).
- 🛠️ **Full VS Code Extension & DAP Debugger**: Syntax highlighting, code completion, interactive step-by-step debugging (breakpoints, call stack, variable watches, expression evaluation).
- 📦 **Dual Build Systems**: Native Visual Studio project (`.sln` / `.vcxproj`) and standalone cross-platform `CMakeLists.txt`.

---

## ⚡ Performance Benchmarks

### 1. Multi-Million-Digit BigInt Multiplication: TzdLang GPU NTT vs GMP

Benchmark multiplying two $4,741,006$-digit numbers on an NVIDIA P106-090 GPU (Pascal CC 6.1, 192 GB/s bandwidth):

| Engine / Implementation | Digit Count | Compute / NTT Time | Total Pipeline Time | Speedup vs GMP |
|---|---|---|---|---|
| **TzdLang GPU NTT (CUDA NVRTC)** | **4,741,006** | **29.20 ms** | **56.52 ms** | **9.2x ~ 17.8x** |
| Single-core GMP 6.3.0 (`mpz_mul`) | 4,741,006 | ~519.30 ms | ~519.30 ms | 1.0x (Baseline) |
| Python 3.12 (`int * int`) | 4,741,006 | >3,800 ms | >3,800 ms | ~0.14x |

> **Key Architectural Features of TzdLang GPU NTT:**
> - **Three 32-bit NTT Primes**: $P_1 = 469762049$, $P_2 = 167772161$, $P_3 = 754974721$.
> - **Bailey's 4-Step 2D NTT**: Decomposes $N = 2^{21}$ limbs into $2048 \times 1024$ 2D matrix transforms, utilizing on-chip shared-memory bank-conflict-free padding (`PAD(idx) = idx + (idx >> 5)`).
> - **Parallel Kogge-Stone Carry Chain**: 2-round carry reduction eliminating overflow before intra- and inter-block prefix scanning.

### 2. JIT Microbenchmarks: TzdLang vs JDK 20 HotSpot

| Benchmark Operation | TzdLang (LLVM JIT) | JDK 20 (HotSpot C2) | Comparison |
|---|---|---|---|
| Function Call Overhead `callOverhead(1M)` | **0.002 s** | 0.005 s | **TzdLang 2.5x faster** |
| Nested Loop `nestedLoop(1k × 1k)` | **0.003 s** | 0.005 s | **TzdLang 1.7x faster** |
| Accumulation Loop `sumLoop(1M)` | **0.002 s** | 0.003 s | **TzdLang 1.5x faster** |
| Ackermann Function `Ackermann(3, 6)` | **0.001 s** | 0.001 s | **TzdLang 1.2x faster** |
| Newton Square Root `sqrt(100k)` | **0.000006 s** | 0.000008 s | **TzdLang 1.3x faster** |
| Recursive Fibonacci `fib(35)` | 0.197 s | 0.061 s | JDK faster |

---

## 🚀 Quick Start

### 1. Requirements

- **Operating System**: Windows 10 / 11 (x64)
- **Compiler / Toolchain**:
  - Visual Studio 2022 / 2026 (MSVC v143 / v145) with C++20 support
  - Or standalone **CMake 3.20+**
- **Optional Accelerators**:
  - NVIDIA CUDA Toolkit 12.0+ (Driver supporting compute capability $\ge 6.0$)
  - LibTorch (included in `External/libtorch` or system-wide)
  - LLVM SDK (for ORC JIT execution)

### 2. Building from Source

#### Option A: Building with Visual Studio (Recommended)
```cmd
git clone https://github.com/tzdwindows/TzdLanguage.git
cd TzdLanguage

:: Build Release x64 using MSBuild
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" TzdTools.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

#### Option B: Building with CMake
```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release --parallel
```

### 3. Running Scripts

```cmd
:: Run a script using the default interpreter
TzdTools.exe --runMainTzd="examples/test.tzd"

:: Run with LLVM ORC JIT optimization
TzdTools.exe --jit --runMainTzd="bench.tzd"

:: Run large number multiplication with GPU acceleration and detailed timing
TzdTools.exe --runMainTzd="大数.tzd" --forceGPU --bigTime
```

---

## 💻 Language Syntax at a Glance

### Variables & Functions
```tzd
// Variable declaration (dynamic or with type hints)
var x = 42;
var string greeting = "Hello, TzdLang!";

// First-class functions
fun add(a, b) {
    return a + b;
}

print(greeting + " " + toString(add(x, 8)));
```

### Object-Oriented Programming (Classes & Inheritance)
```tzd
class Animal {
    var string name;
    Animal(name) {
        this.name = name;
    }
    fun speak() {
        print(this.name + " makes a sound.");
    }
}

class Dog extends Animal {
    Dog(name) : super(name) {}
    fun speak() {
        print(this.name + " barks: Woof! Woof!");
    }
}

var pet = new Dog("Buddy");
pet.speak(); // Output: Buddy barks: Woof! Woof!
```

### Deep Learning & Tensors (LibTorch Integration)
```tzd
import "stdlib/torch/nn.tzd";

// Create tensors directly
var a = torch_randn([3, 3]);
var b = torch_eye(3);
var c = torch_matmul(a, b);

print("Tensor Shape: " + toString(c.shape));
print("Tensor on GPU: " + toString(c.cuda()));
```

### Exception Handling & Pattern Matching
```tzd
import "core/Error.tzd";

try {
    throw new Error("Disk read failure", "E_IO");
} catch (err) {
    if (err in Error) {
        print("Caught [" + err.code + "]: " + err.message);
    }
}
```

### Native Multi-Threading
```tzd
import "thread/Thread.tzd";

fun worker() {
    for (i = 0; i < 5; i++) {
        print("Worker thread running: " + toString(i));
        sleep(500);
    }
}

var t = new Thread(worker);
t.start();
t.join();
```

---

## 📖 Detailed Wiki Documentation

Comprehensive technical documentation and deep-dive design guides are available in the [`wiki/`](wiki/) directory:

- 📑 [**Wiki Home & Architecture Overview**](wiki/Home.md) - System-level architectural design and execution tiers.
- 📐 [**Language Specification & Syntax Guide**](wiki/Language-Specification.md) - Types, control flow, functions, OOP, and exceptions.
- 🚀 [**GPU NTT BigInt Multiplication Deep-Dive**](wiki/GPU-NTT-BigInt.md) - Mathematical formulation, CRT, 2D Stockham kernels, Kogge-Stone carry scan.
- ⚡ [**JIT Compiler Internals**](wiki/JIT-Compiler-Internals.md) - Tier 0 VM, Tier 1 LLVM ORC JIT, specialization passes, and optimizations.
- 🧠 [**Deep Learning with LibTorch**](wiki/Deep-Learning-and-PyTorch.md) - Tensor APIs, autograd, neural networks, CUDA acceleration.
- 🔨 [**Build & Toolchain Guide**](wiki/Building-and-Toolchain.md) - Detailed build instructions for MSBuild and CMake.
- 🔌 [**VS Code Extension & DAP Debugger**](wiki/VSCode-Extension-and-Debugger.md) - Language Server and Debug Adapter Protocol integration.
- 📚 [**Standard Library Reference**](wiki/Standard-Library-Reference.md) - Core, Math, Thread, and Torch libraries.

---

## 📁 Repository Structure

```text
TzdTools/
├── CMakeLists.txt             # Standalone CMake build configuration
├── TzdTools.sln               # Visual Studio Solution
├── TzdTools.vcxproj           # Visual Studio Project File
├── README.md                  # Project documentation (English)
├── README_zh.md               # Project documentation (Chinese)
├── wiki/                      # Complete technical Wiki documentation
├── Generated/                 # ANTLR4 parser, AST visitors, VM, JIT, PyTorch
│   ├── TzdInterpreter.cpp     # AST & VM execution engine
│   ├── TzdJit.cpp             # LLVM ORC JIT compiler
│   ├── TzdPyTorch.cpp         # LibTorch binding & GPU NTT BigInt multiplication
│   └── ...
├── dyncall/                   # C FFI & x86_64 assembly invocation
├── Plots/                     # pbPlots native charting library
├── stdlib/                    # TzdLang standard libraries
│   ├── core/                  # Error, IO, reflection
│   ├── thread/                # Threading and synchronization
│   └── torch/                 # Deep learning modules
├── vscodePlugin/              # VS Code Extension (DAP, syntax, LSP)
└── examples/                  # Language sample programs & benchmarks
```

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome!
1. Fork the project.
2. Create your feature branch (`git checkout -b feature/AmazingFeature`).
3. Commit your changes (`git commit -m 'feat: Add AmazingFeature'`).
4. Push to the branch (`git push origin feature/AmazingFeature`).
5. Open a Pull Request.

---

## 📄 License

This project is distributed under the **MIT License**. See `LICENSE` for more information.