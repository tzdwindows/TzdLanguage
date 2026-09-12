# 编译构建与工具链配置指南

<p align="right">
  <a href="Building-and-Toolchain.md"><strong>English</strong></a> | <a href="Building-and-Toolchain-zh.md"><strong>中文</strong></a>
</p>

本指南详尽介绍如何在 Windows 环境下从源码独立编译构建 **TzdTools** 编译器可执行文件及 **TzdLang** 完整运行时套件。

---

## 1. 系统依赖与前置需求

| 组件名称 | 最低版本要求 | 推荐配置 | 备注说明 |
|---|---|---|---|
| 操作系统 | Windows 10 x64 | Windows 11 x64 | 必须为 64 位操作系统架构 |
| MSVC C++ 工具链 | Visual Studio 2022 (v143) | Visual Studio 2026 / 18 (v145) | 需开启 C++20 标准（`/std:c++20`） |
| CMake | 3.20+ | 3.28+ | 支持跨 IDE 与独立自动化构建 |
| NVIDIA CUDA Toolkit | 12.0+ | 12.6+ | 支撑 GPU NTT 百万位大数乘法引擎 |
| LLVM SDK | 17.0+ | 18.0+ | 支撑 Tier 1 LLVM ORC JIT 实时编译 |
| LibTorch | 2.0+ | 2.2+ (CUDA 12.x 版) | 存放于 `External/libtorch` 目录 |

---

## 2. 基于 Visual Studio MSBuild 编译（官方推荐）

主项目工程采用 `TzdTools.vcxproj` 管理：

```cmd
git clone https://github.com/tzdwindows/TzdLanguage.git
cd TzdLanguage

:: 编译 x64 Release 发布版本
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" TzdTools.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

编译生成的主程序位于：
`x64\Release\TzdTools.exe`

---

## 3. 基于独立 CMake 自动化构建

TzdTools 提供了重构后的独立 `CMakeLists.txt`，内置了全面的路径探测、环境变量回退与智能依赖注入机制：

```cmd
:: 1. 使用 CMake 生成 Visual Studio 工程文件
cmake -B build -G "Visual Studio 18 2026" -A x64

:: 2. 并行编译 Release 版本
cmake --build build --config Release --parallel
```

### 自定义第三方依赖检索路径
如果你的第三方库未安装在系统默认目录，可通过 CMake 参数或环境变量显式指定：

```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64 ^
    -DCUDA_PATH="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6" ^
    -DLIBTORCH_PATH="C:/Users/user/source/repos/TzdTools/External/libtorch" ^
    -DLLVM_DIR="E:/LLVM_SDK" ^
    -DVCPKG_ROOT="E:/vcpkg"
```

---

## 4. 构建后产物与运行时自动部署

编译成功后，构建脚本会自动将以下运行时必要组件部署至目标输出目录：
- `stdlib/`：TzdLang 官方标准库（包括 `core/`, `thread/`, `torch/` 等）。
- `nvrtc64_*.dll`：CUDA 实时运行时编译器动态链接库（支持运行时 JIT 编译 NTT GPU 核函数）。
- `torch*.dll`, `c10*.dll`：LibTorch 深度学习运行时动态库。

---

## 5. 常见构建报错与排查方案

### 5.1 `error C2026: string too big, trailing characters truncated`
- **产生原因**：MSVC 编译器限制单个静态字符串字面量长度不可超过 65,535 字节。
- **解决方案**：内嵌的 CUDA NTT 核函数源码已拆分为模块化字符串分段函数 `getNttKernelSrc()`（位于 `Generated/TzdPyTorch.cpp`）。

### 5.2 缺失 `atlbase.h` 头文件
- **产生原因**：部分精简安装的 Visual Studio BuildTools 缺少微软 ATL 库支持。
- **解决方案**：源码中已剔除 `TzdFuncScanner.cpp` 中无实质调用的 `atlbase.h` 头文件引用。

### 5.3 深度 JIT 递归执行导致 Stack Overflow（栈溢出）
- **产生原因**：多层深递归算法执行需要充足的系统调用栈空间。
- **解决方案**：MSBuild 与 CMake 工程配置中均强制指定了链接器参数 `/STACK:268435456,268435456`（提供高达 256 MB 的调用栈保留与提交空间）。
