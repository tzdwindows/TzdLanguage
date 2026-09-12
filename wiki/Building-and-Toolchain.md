# Building and Toolchain Guide

<p align="right">
  <a href="Building-and-Toolchain.md"><strong>English</strong></a> | <a href="Building-and-Toolchain-zh.md"><strong>中文</strong></a>
</p>

This guide describes how to build **TzdTools** and the **TzdLang** runtime environment on Windows from source.

---

## 1. System Prerequisites

| Component | Minimum Version | Recommended | Notes |
|---|---|---|---|
| Operating System | Windows 10 x64 | Windows 11 x64 | x64 architecture required |
| MSVC C++ Toolchain | Visual Studio 2022 (v143) | Visual Studio 2026 / 18 (v145) | Requires C++20 support (`stdcpp20`) |
| CMake | 3.20+ | 3.28+ | For standalone cross-IDE builds |
| NVIDIA CUDA Toolkit | 12.0+ | 12.6+ | Required for GPU NTT BigInt multiplication |
| LLVM SDK | 17.0+ | 18.0+ | Required for Tier 1 ORC JIT compilation |
| LibTorch | 2.0+ | 2.2+ (CUDA 12.x) | Extracted to `External/libtorch` |

---

## 2. Building with Visual Studio MSBuild (Recommended)

The primary build project is defined in `TzdTools.vcxproj`:

```cmd
git clone https://github.com/tzdwindows/TzdLanguage.git
cd TzdLanguage

:: Build Release configuration
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" TzdTools.vcxproj /p:Configuration=Release /p:Platform=x64 /m
```

The output executable is generated at:
`x64\Release\TzdTools.exe`

---

## 3. Building with Standalone CMake

TzdTools provides an overhauled `CMakeLists.txt` featuring automatic path detection and fallback heuristics:

```cmd
:: 1. Generate Visual Studio project files using CMake
cmake -B build -G "Visual Studio 18 2026" -A x64

:: 2. Compile Release binary
cmake --build build --config Release --parallel
```

### Customizing Dependency Search Paths
You can override default dependency locations via environment variables or CMake definitions:

```cmd
cmake -B build -G "Visual Studio 18 2026" -A x64 ^
    -DCUDA_PATH="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6" ^
    -DLIBTORCH_PATH="C:/Users/user/source/repos/TzdTools/External/libtorch" ^
    -DLLVM_DIR="E:/LLVM_SDK" ^
    -DVCPKG_ROOT="E:/vcpkg"
```

---

## 4. Post-Build Artifacts & Runtime Deployment

Upon successful compilation, the build system automatically deploys runtime prerequisites to the output directory:
- `stdlib/`: TzdLang standard library modules (`core/`, `thread/`, `torch/`).
- `nvrtc64_*.dll`: CUDA runtime compiler library for JIT-compiling NTT CUDA kernels.
- `torch*.dll`, `c10*.dll`: LibTorch runtime shared libraries.

---

## 5. Troubleshooting Common Build Issues

### 5.1 `error C2026: string too big, trailing characters truncated`
- **Cause**: MSVC restricts single string literals to 65,535 bytes.
- **Fix**: Embedded CUDA source code has been partitioned into modular string chunks via `getNttKernelSrc()` in `Generated/TzdPyTorch.cpp`.

### 5.2 Missing `atlbase.h`
- **Cause**: Minimal MSVC BuildTools installations may omit Microsoft Active Template Library (ATL).
- **Fix**: The unused `atlbase.h` include in `TzdFuncScanner.cpp` has been eliminated.

### 5.3 Stack Overflow During Deep JIT Recursion
- **Cause**: Deeply recursive functions require expanded stack reservation.
- **Fix**: Both MSBuild and CMake enforce `/STACK:268435456,268435456` (256 MB reserve & commit).
