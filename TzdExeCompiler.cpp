#ifndef NOMINMAX
#define NOMINMAX
#endif

// ANTLR4 & Tzd Runtime Headers (must be included before windows.h)
#include "antlr4-runtime.h"
#include "Generated/TzdLangParser.h"
#include "Generated/TzdLangLexer.h"
#include "Generated/TzdInterpreter.h"
#include "Generated/TzdBytecode.h"
#include "Generated/TzdNativeModule.h"
#ifdef WITH_LIBTORCH
#include "Generated/TzdPyTorch.h"
#endif
#include "Generated/TzdTieringEngine.h"
#include "Generated/TzdOop.h"

#include "TzdExeCompiler.h"
#include "TzdNativeCodegen.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <chrono>
#include <set>
#include <iomanip>

namespace fs = std::filesystem;

namespace tzd {

// ============================================================================
// 0. Progress bar & smart dependency helpers
// ============================================================================

static void printProgressBar(int pct, const std::string& stage) {
    const int barWidth = 30;
    int filled = (pct * barWidth) / 100;
    std::string bar;
    bar.reserve(barWidth + 2);
    for (int i = 0; i < filled; ++i) bar += "\xe2\x96\x88"; // UTF-8 block ████
    for (int i = filled; i < barWidth; ++i) bar += "\xe2\x96\x91"; // ░░░░

    // Pad / truncate stage string
    std::string stageShort = stage;
    if (stageShort.size() > 40) stageShort = stageShort.substr(0, 37) + "...";

    std::cout << "\r  [" << bar << "] " << std::setw(3) << pct << "%  " << stageShort;
    std::cout.flush();
}

// Keyword sets used for smart dependency detection
static const std::vector<std::string> k_torchKeywords = {
    "import torch", "import \"torch", "import 'torch",
    "tensor(", "Tensor(", "torch.", "loadModel(", "saveModel(",
    "Linear(", "Conv2d(", "relu(", "sigmoid(", "softmax(",
    "backward(", "optimizer(", "Adam(", "SGD(",
};

static const std::vector<std::string> k_gpuKeywords = {
    "cuda(", ".gpu(", "toGpu(", "toCuda(", ".cuda", "cudaDevice",
    "gpuAvailable(", "isCudaAvailable(", "tensorCuda(",
};

bool TzdExeCompiler::sourceUsesTorch(const std::string& sourceCode) {
    for (const auto& kw : k_torchKeywords) {
        if (sourceCode.find(kw) != std::string::npos)
            return true;
    }
    return false;
}

bool TzdExeCompiler::sourceUsesGpu(const std::string& sourceCode) {
    if (!sourceUsesTorch(sourceCode)) return false;
    for (const auto& kw : k_gpuKeywords) {
        if (sourceCode.find(kw) != std::string::npos)
            return true;
    }
    return false;
}

// ============================================================================
// 1. Embedded Virtual File System (EmbeddedVFS)
// ============================================================================

EmbeddedVFS& EmbeddedVFS::instance() {
    static EmbeddedVFS vfs;
    return vfs;
}

void EmbeddedVFS::addFile(const std::string& virtualPath, std::vector<uint8_t> data) {
    std::string norm = virtualPath;
    std::replace(norm.begin(), norm.end(), '\\', '/');
    m_files[norm] = std::move(data);
}

void EmbeddedVFS::addFile(const std::string& virtualPath, const std::string& text) {
    std::vector<uint8_t> data(text.begin(), text.end());
    addFile(virtualPath, std::move(data));
}

bool EmbeddedVFS::hasFile(const std::string& virtualPath) const {
    std::string norm = virtualPath;
    std::replace(norm.begin(), norm.end(), '\\', '/');
    return m_files.find(norm) != m_files.end();
}

std::string EmbeddedVFS::getFileContent(const std::string& virtualPath) const {
    std::string norm = virtualPath;
    std::replace(norm.begin(), norm.end(), '\\', '/');
    auto it = m_files.find(norm);
    if (it == m_files.end()) return "";
    return std::string(reinterpret_cast<const char*>(it->second.data()), it->second.size());
}

const std::vector<uint8_t>* EmbeddedVFS::getFileBytes(const std::string& virtualPath) const {
    std::string norm = virtualPath;
    std::replace(norm.begin(), norm.end(), '\\', '/');
    auto it = m_files.find(norm);
    if (it == m_files.end()) return nullptr;
    return &it->second;
}

std::vector<std::string> EmbeddedVFS::listFiles() const {
    std::vector<std::string> res;
    for (const auto& [p, _] : m_files) {
        res.push_back(p);
    }
    return res;
}

void EmbeddedVFS::extractToDirectory(const fs::path& targetDir) const {
    for (const auto& [relPath, bytes] : m_files) {
        fs::path fullPath = targetDir / relPath;
        fs::create_directories(fullPath.parent_path());
        std::ofstream out(fullPath, std::ios::binary);
        if (out.is_open()) {
            out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }
    }
}

void EmbeddedVFS::clear() {
    m_files.clear();
}

// ============================================================================
// 2. Helper Functions (Checksum, Bytecode Serialization, Stubs)
// ============================================================================

uint64_t TzdExeCompiler::computeChecksum(const uint8_t* data, size_t size) {
    // 64-bit FNV-1a hash
    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(data[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string TzdExeCompiler::findRuntimeStub() {
    // Get the directory of the currently running TzdTools.exe
    fs::path exeDir;
    wchar_t currentExe[MAX_PATH];
    if (GetModuleFileNameW(NULL, currentExe, MAX_PATH)) {
        exeDir = fs::path(currentExe).parent_path();
    }

    // ── Priority order ────────────────────────────────────────────────────────
    // tzd_stub.exe is a ZERO-DLL lightweight runner (no torch/cuda imports).
    // Always prefer it over TzdTools.exe which has torch/cuda in its IAT.
    // ─────────────────────────────────────────────────────────────────────────
    std::vector<fs::path> candidates;

    // 1. tzd_stub.exe alongside TzdTools.exe (same dir — produced by build_stub.bat)
    if (!exeDir.empty()) {
        candidates.push_back(exeDir / "tzd_stub.exe");
    }
    // 2. Common relative build output locations
    candidates.push_back(fs::path("x64") / "Release" / "tzd_stub.exe");
    candidates.push_back(fs::path("build_vs18") / "Release" / "tzd_stub.exe");
    candidates.push_back(fs::path("dist") / "TzdTools" / "tzd_stub.exe");
    candidates.push_back(fs::path("tzd_stub.exe"));

    // 3. Fallback: TzdTools.exe itself (will carry DLL dependencies — avoid if possible)
    if (!exeDir.empty()) {
        candidates.push_back(exeDir / "TzdTools.exe");
    }
    candidates.push_back(fs::path("x64") / "Release" / "TzdTools.exe");
    candidates.push_back(fs::path("build_vs18") / "Release" / "TzdTools.exe");
    candidates.push_back(fs::path("dist") / "TzdTools" / "TzdTools.exe");
    candidates.push_back(fs::path("TzdTools.exe"));

    std::error_code ec;
    for (const auto& cand : candidates) {
        if (fs::exists(cand, ec)) {
            std::string resolved = fs::canonical(cand, ec).string();
            if (!resolved.empty()) return resolved;
            return cand.string();
        }
    }

    return "";
}

bool TzdExeCompiler::copyRuntimeDependencies(const std::string& stubExeDir,
                                              const std::string& targetExeDir,
                                              const std::string& sourceCode,
                                              const ExeCompileOptions& opts)
{
    // If CPU-only mode: skip ALL torch / CUDA DLLs
    // If smart mode: only copy torch/CUDA DLLs when source actually uses them
    bool wantTorch = !opts.cpuOnly && (!opts.smartDeps || sourceUsesTorch(sourceCode));
    bool wantGpu   = wantTorch && !opts.cpuOnly && (!opts.smartDeps || sourceUsesGpu(sourceCode));

    // Patterns that identify GPU/CUDA-specific DLLs (case-insensitive prefix match)
    auto isGpuDll = [](const std::string& name) -> bool {
        static const std::vector<std::string> gpuPrefixes = {
            "cuda", "cubl", "cufft", "cusparse", "cusolver", "curand", "cupti",
            "cudnn", "nvrtc", "nvjit", "nvtool", "caffe2_nvrtc",
        };
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& p : gpuPrefixes) {
            if (lower.rfind(p, 0) == 0) return true;
        }
        return false;
    };

    // Patterns that identify torch/c10-specific DLLs
    auto isTorchDll = [](const std::string& name) -> bool {
        static const std::vector<std::string> torchPrefixes = {
            "torch", "c10", "fbgemm", "cpuinfo", "dnnl", "xnnpack",
            "pthreadpool", "pytorch", "libtorch",
        };
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        for (const auto& p : torchPrefixes) {
            if (lower.rfind(p, 0) == 0) return true;
        }
        return false;
    };

    try {
        fs::path stubP = stubExeDir.empty() ? fs::current_path() : fs::absolute(stubExeDir);
        fs::path targetP = targetExeDir.empty() ? fs::current_path() : fs::absolute(targetExeDir);

        std::error_code ec;
        if (fs::equivalent(stubP, targetP, ec)) return true;

        if (!fs::exists(targetP, ec)) {
            fs::create_directories(targetP, ec);
        }

        int copied = 0, skipped = 0;
        // Copy companion runtime DLLs from stub directory to target directory
        for (const auto& entry : fs::directory_iterator(stubP, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            if (entry.path().extension() != ".dll") continue;

            std::string fname = entry.path().filename().string();

            // Apply smart filtering
            if (isGpuDll(fname) && !wantGpu)  { ++skipped; continue; }
            if (isTorchDll(fname) && !wantTorch) { ++skipped; continue; }

            fs::path dst = targetP / entry.path().filename();
            if (!fs::exists(dst, ec)) {
                fs::copy_file(entry.path(), dst, fs::copy_options::overwrite_existing, ec);
                ++copied;
            }
        }

        if (copied > 0 || skipped > 0) {
            std::cout << "\n  [依赖拷贝] 已复制 " << copied << " 个 DLL"
                      << (skipped > 0 ? ("，跳过 " + std::to_string(skipped) + " 个 GPU/Torch DLL（未使用）") : "")
                      << std::endl;
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[TzdExeCompiler] Warning: Failed to copy companion DLLs: " << e.what() << std::endl;
        return false;
    }
}

std::string TzdExeCompiler::findMsvcVcvars() {
    static const std::vector<std::string> searchPaths = {
        "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\VC\\Auxiliary\\Build\\vcvars64.bat",
        "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Professional\\VC\\Auxiliary\\Build\\vcvars64.bat",
    };
    for (const auto& p : searchPaths) {
        if (fs::exists(p)) return p;
    }
    return "";
}

std::string TzdExeCompiler::findLlvmCompiler(std::string& outKind) {
    // 1. Check alongside current running executable
    wchar_t currentExe[MAX_PATH];
    if (GetModuleFileNameW(NULL, currentExe, MAX_PATH)) {
        fs::path p(currentExe);
        fs::path exeDir = p.parent_path();
        std::vector<fs::path> relCandidates = {
            exeDir / "llvm" / "bin" / "clang++.exe",
            exeDir / "bin" / "clang++.exe",
            exeDir / "clang++.exe",
            exeDir.parent_path() / "llvm" / "bin" / "clang++.exe",
            exeDir.parent_path() / "bin" / "clang++.exe",
        };
        for (const auto& c : relCandidates) {
            if (fs::exists(c)) { outKind = "clang++"; return c.string(); }
        }
    }

    // 2. Check known SDK install directories
    std::vector<std::string> knownPaths = {
        "E:\\LLVM_SDK\\bin\\clang++.exe",
        "C:\\Program Files\\LLVM\\bin\\clang++.exe",
        "C:\\LLVM\\bin\\clang++.exe",
        "D:\\LLVM\\bin\\clang++.exe",
        "E:\\LLVM\\bin\\clang++.exe",
    };
    for (const auto& p : knownPaths) {
        if (fs::exists(p)) { outKind = "clang++"; return p; }
    }

    // 3. Search in system PATH via where.exe
    FILE* pipe = _popen("where.exe clang++.exe 2>nul", "r");
    if (pipe) {
        char buf[512];
        if (fgets(buf, sizeof(buf), pipe)) {
            std::string s(buf);
            while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
            if (fs::exists(s)) { _pclose(pipe); outKind = "clang++"; return s; }
        }
        _pclose(pipe);
    }

    // 4. Fallback: MinGW g++
    std::string gcc = findGccCompiler();
    if (!gcc.empty()) {
        outKind = "g++";
        return gcc;
    }

    return "";
}

std::string TzdExeCompiler::findGccCompiler() {
    std::vector<std::string> knownGcc = {
        "C:\\msys64\\mingw64\\bin\\g++.exe",
        "C:\\mingw64\\bin\\g++.exe",
        "C:\\MinGW\\bin\\g++.exe",
        "D:\\msys64\\mingw64\\bin\\g++.exe",
        "D:\\mingw64\\bin\\g++.exe",
    };
    for (const auto& p : knownGcc) {
        if (fs::exists(p)) return p;
    }

    FILE* pipe = _popen("where.exe g++.exe 2>nul", "r");
    if (pipe) {
        char buf[512];
        if (fgets(buf, sizeof(buf), pipe)) {
            std::string s(buf);
            while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ')) s.pop_back();
            if (fs::exists(s)) { _pclose(pipe); return s; }
        }
        _pclose(pipe);
    }
    return "";
}

bool TzdExeCompiler::copyTorchDependencies(const std::string& targetExeDir, bool isGpu) {
    fs::path targetDir = fs::absolute(targetExeDir);
    std::error_code ec;
    if (!fs::exists(targetDir, ec)) {
        fs::create_directories(targetDir, ec);
    }

    std::vector<fs::path> srcDirs;
    wchar_t currentExe[MAX_PATH];
    if (GetModuleFileNameW(NULL, currentExe, MAX_PATH)) {
        fs::path p(currentExe);
        fs::path exeDir = p.parent_path();
        srcDirs.push_back(exeDir);
        srcDirs.push_back(exeDir / "lib");
        srcDirs.push_back(exeDir.parent_path() / "lib");
    }
    srcDirs.push_back(fs::path("dist") / (isGpu ? "TzdTools" : "TzdTools_CPU"));
    srcDirs.push_back(fs::path("dist") / "TzdTools");
    srcDirs.push_back(fs::path("External") / "libtorch" / "lib");
    srcDirs.push_back(fs::path("x64") / "Release");

    fs::path validSrcDir;
    for (const auto& d : srcDirs) {
        if (fs::exists(d / "torch_cpu.dll", ec) || fs::exists(d / "c10.dll", ec)) {
            validSrcDir = d;
            break;
        }
    }

    if (validSrcDir.empty()) {
        std::cerr << "  [PyTorch] 警告: 未找到 PyTorch 运行库源目录，跳过 DLL 拷贝。" << std::endl;
        return false;
    }

    std::vector<std::string> cpuDlls = {
        "torch_cpu.dll", "torch.dll", "c10.dll", "fbgemm.dll", "libiomp5md.dll",
        "asmjit.dll", "mkl_core.1.dll", "mkl_intel_thread.1.dll", "vcomp140.dll",
        "pytorch_jni.dll", "torch_global_deps.dll", "uv.dll"
    };

    std::vector<std::string> gpuDlls = {
        "torch_cuda.dll", "c10_cuda.dll", "caffe2_nvrtc.dll",
        "cublas64_12.dll", "cublasLt64_12.dll", "cudart64_12.dll",
        "cudnn64_8.dll", "cufft64_11.dll", "curand64_10.dll",
        "cusolver64_11.dll", "cusparse64_12.dll", "nvJitLink_120_0.dll",
        "nvrtc-builtins64_126.dll", "nvrtc-builtins64_121.dll", "nvrtc64_120_0.dll"
    };

    std::vector<std::string> toCopy = cpuDlls;
    if (isGpu) {
        for (const auto& g : gpuDlls) toCopy.push_back(g);
    }

    int copied = 0;
    for (const auto& dll : toCopy) {
        fs::path srcFile = validSrcDir / dll;
        fs::path dstFile = targetDir / dll;
        if (fs::exists(srcFile, ec) && !fs::equivalent(validSrcDir, targetDir, ec)) {
            fs::copy_file(srcFile, dstFile, fs::copy_options::overwrite_existing, ec);
            if (!ec) copied++;
        }
    }

    std::cout << "\n  [PyTorch 依赖配置] 成功将 " << copied << " 个核心运行库 DLL 部署至目标目录 ("
              << (isGpu ? "GPU CUDA 完整版" : "CPU 轻量版") << ")。" << std::endl;
    return true;
}

bool TzdExeCompiler::compileCppToExe(
    const std::string& cppPath,
    const std::string& exePath,
    const ExeCompileOptions& opts,
    std::string& err)
{
    fs::path targetExeP = fs::absolute(exePath);
    fs::path cppP = fs::absolute(cppPath);
    fs::path targetDir = targetExeP.parent_path();

    if (!fs::exists(targetDir)) {
        std::error_code ec;
        fs::create_directories(targetDir, ec);
    }

    // Locate repository directory where TzdNativeRuntime.hpp is stored
    wchar_t currentExe[MAX_PATH];
    fs::path repoDir = fs::current_path();
    if (GetModuleFileNameW(NULL, currentExe, MAX_PATH)) {
        fs::path p(currentExe);
        fs::path dir1 = p.parent_path();
        fs::path dir2 = dir1.parent_path();
        fs::path dir3 = dir2.parent_path();
        if (fs::exists(dir1 / "TzdNativeRuntime.hpp")) repoDir = dir1;
        else if (fs::exists(dir2 / "TzdNativeRuntime.hpp")) repoDir = dir2;
        else if (fs::exists(dir3 / "TzdNativeRuntime.hpp")) repoDir = dir3;
    }
    if (!fs::exists(repoDir / "TzdNativeRuntime.hpp")) {
        if (fs::exists(fs::current_path() / "TzdNativeRuntime.hpp")) {
            repoDir = fs::current_path();
        }
    }

    // Copy TzdNativeRuntime.hpp to target directory so include always succeeds
    std::error_code ec;
    bool copiedHeader = false;
    if (fs::exists(repoDir / "TzdNativeRuntime.hpp", ec)) {
        if (!fs::equivalent(repoDir, targetDir, ec)) {
            fs::copy_file(repoDir / "TzdNativeRuntime.hpp", targetDir / "TzdNativeRuntime.hpp",
                          fs::copy_options::overwrite_existing, ec);
            if (!ec) copiedHeader = true;
        }
    }

    // Select Toolchain
    std::string vcvars = findMsvcVcvars();
    std::string gccCompiler = findGccCompiler();
    std::string llvmKind;
    std::string llvmCompiler = findLlvmCompiler(llvmKind);

    bool useMsvc = false;
    bool useLlvm = false;
    std::string selectedCompiler;

    if (opts.toolchain == CompilerToolchain::MSVC) {
        useMsvc = true;
    } else if (opts.toolchain == CompilerToolchain::GCC) {
        useLlvm = true;
        selectedCompiler = !gccCompiler.empty() ? gccCompiler : "g++";
    } else if (opts.toolchain == CompilerToolchain::LLVM) {
        useLlvm = true;
        selectedCompiler = !llvmCompiler.empty() ? llvmCompiler : "clang++";
    } else {
        // AUTO mode
        if (!vcvars.empty()) {
            useMsvc = true;
        } else if (!gccCompiler.empty()) {
            useLlvm = true;
            selectedCompiler = gccCompiler;
            std::cout << "\n  [编译工具链] 未检测到 Visual Studio 环境，已自动启用内置 GCC/MinGW 编译器:\n               "
                      << gccCompiler << std::endl;
        } else if (!llvmCompiler.empty()) {
            useLlvm = true;
            selectedCompiler = llvmCompiler;
            std::cout << "\n  [编译工具链] 未检测到 Visual Studio 环境，已自动启用内置 LLVM / Clang 编译器:\n               "
                      << llvmCompiler << std::endl;
        } else {
            useMsvc = true; // Attempt cl.exe in PATH
        }
    }

    fs::path batPath = targetDir / "_tzd_compile.bat";
    fs::path logPath = targetDir / "_tzd_compile.log";

    // LibTorch configurations
    fs::path libtorchDir = repoDir / "External" / "libtorch";
    if (!fs::exists(libtorchDir, ec)) {
        if (fs::exists(fs::path("External") / "libtorch", ec)) libtorchDir = fs::path("External") / "libtorch";
    }

    {
        std::ofstream bat(batPath);
        bat << "@echo off\n";

        if (useMsvc) {
            if (!vcvars.empty()) {
                bat << "call \"" << vcvars << "\" >nul 2>&1\n";
            }
            std::string optFlag = "/O2";
            if (opts.optLevel == 0) optFlag = "/Od";
            else if (opts.optLevel == 1) optFlag = "/O1";
            else if (opts.optLevel == 2) optFlag = "/O2";
            else if (opts.optLevel >= 3) optFlag = "/Ox /fp:fast";

            std::string incFlag = "/I\"" + repoDir.string() + "\" /I\"" + targetDir.string() + "\"";
            std::string extraDefs = "";
            std::string linkLibs = "";

            if (opts.forceTorch && fs::exists(libtorchDir, ec)) {
                incFlag += " /I\"" + (libtorchDir / "include").string() + "\"";
                incFlag += " /I\"" + (libtorchDir / "include" / "torch" / "csrc" / "api" / "include").string() + "\"";
                extraDefs += " /DWITH_LIBTORCH /D_GLIBCXX_USE_CXX11_ABI=0";
                if (opts.torchGpu) {
                    extraDefs += " /DWITH_CUDA /I\"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.6\\include\"";
                    linkLibs += " /LIBPATH:\"" + (libtorchDir / "lib").string() + "\" torch.lib torch_cpu.lib torch_cuda.lib c10.lib c10_cuda.lib";
                } else {
                    linkLibs += " /LIBPATH:\"" + (libtorchDir / "lib").string() + "\" torch.lib torch_cpu.lib c10.lib";
                }
            }

            fs::path exactObjP = targetDir / (cppP.stem().string() + ".obj");
            bat << "cl.exe /nologo /MT " << optFlag << " /EHsc /std:c++20 /utf-8 "
                << extraDefs << " " << incFlag << " \"" << cppP.string() << "\" /Fe:\""
                << targetExeP.string() << "\" /Fo:\"" << exactObjP.string() << "\" "
                << (linkLibs.empty() ? "" : ("/link " + linkLibs)) << " >\""
                << logPath.string() << "\" 2>&1\n";
        } else {
            // LLVM / Clang++ or MinGW g++
            std::string optFlag = "-O2 -s";
            if (opts.optLevel == 0) optFlag = "-O0 -g";
            else if (opts.optLevel == 1) optFlag = "-O1";
            else if (opts.optLevel == 2) optFlag = "-O2 -s";
            else if (opts.optLevel >= 3) optFlag = "-O3 -s";

            std::string incFlag = "-I\"" + repoDir.string() + "\" -I\"" + targetDir.string() + "\"";
            std::string extraDefs = "";
            std::string linkLibs = "";

            if (opts.forceTorch && fs::exists(libtorchDir, ec)) {
                incFlag += " -I\"" + (libtorchDir / "include").string() + "\"";
                incFlag += " -I\"" + (libtorchDir / "include" / "torch" / "csrc" / "api" / "include").string() + "\"";
                extraDefs += " -DWITH_LIBTORCH -D_GLIBCXX_USE_CXX11_ABI=0";
                linkLibs += " -L\"" + (libtorchDir / "lib").string() + "\" -ltorch -ltorch_cpu -lc10";
                if (opts.torchGpu) {
                    extraDefs += " -DWITH_CUDA -I\"C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.6\\include\"";
                    linkLibs += " -ltorch_cuda -lc10_cuda";
                }
            }

            std::string compExe = !selectedCompiler.empty() ? selectedCompiler : (!llvmCompiler.empty() ? llvmCompiler : "clang++");
            bat << "\"" << compExe << "\" -std=c++20 " << optFlag << " -static -finput-charset=UTF-8 -fexec-charset=UTF-8 "
                << extraDefs << " " << incFlag << " \"" << cppP.string() << "\" -o \""
                << targetExeP.string() << "\" " << linkLibs << " >\""
                << logPath.string() << "\" 2>&1\n";
        }

        bat << "exit /b %ERRORLEVEL%\n";
    }

    // Delete any pre-existing target exe so stale binaries are never reported as success
    if (fs::exists(targetExeP, ec)) {
        fs::remove(targetExeP, ec);
    }

    std::string cmd = "cmd /c \"" + batPath.string() + "\"";
    int ret = std::system(cmd.c_str());

    // Clean up temporary obj, bat, and copied header
    fs::path objPath = targetDir / (cppP.stem().string() + ".obj");
    fs::remove(objPath, ec);
    fs::remove(batPath, ec);
    if (copiedHeader) {
        fs::remove(targetDir / "TzdNativeRuntime.hpp", ec);
    }

    if (ret == 0 && fs::exists(targetExeP, ec) && fs::file_size(targetExeP, ec) > 0) {
        fs::remove(logPath, ec);
        // If PyTorch was forced, deploy companion runtime DLLs to target directory
        if (opts.forceTorch) {
            copyTorchDependencies(targetDir.string(), opts.torchGpu);
        }
        return true;
    }

    std::string logContent;
    if (fs::exists(logPath, ec)) {
        std::ifstream lf(logPath);
        std::string line;
        while (std::getline(lf, line)) {
            if (!line.empty()) logContent += "  " + line + "\n";
        }
        fs::remove(logPath, ec);
    }

    err = (useMsvc ? "MSVC cl.exe" : ("LLVM / " + (!llvmCompiler.empty() ? llvmCompiler : "clang++"))) +
          " compiler returned code " + std::to_string(ret);
    if (!logContent.empty()) {
        err += ":\n" + logContent;
    }
    return false;
}

// ============================================================================
// 3. Recursive Import Scanning
// ============================================================================

std::vector<std::string> TzdExeCompiler::scanAndResolveImports(
    const std::string& sourceCode,
    const std::string& baseDir,
    const std::vector<std::string>& extraIncludePaths)
{
    std::vector<std::string> resolvedPaths;
    std::set<std::string> visited;

    std::function<void(const std::string&, const std::string&)> scanFile = [&](const std::string& code, const std::string& curDir) {
        std::regex importRegex(R"(import\s+["']([^"']+)["'];)");
        auto words_begin = std::sregex_iterator(code.begin(), code.end(), importRegex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string importPath = match[1].str();

            // Normalize path separator
            std::string normImport = importPath;
            if (normImport.find('/') == std::string::npos && normImport.find('\\') == std::string::npos && normImport.find('.') != std::string::npos) {
                std::replace(normImport.begin(), normImport.end(), '.', '/');
                normImport += ".tzd";
            }

            // Candidates to probe:
            std::vector<fs::path> candidates;
            candidates.push_back(fs::path(curDir) / normImport);
            candidates.push_back(fs::path("stdlib") / normImport);
            candidates.push_back(fs::path("dist/TzdTools/stdlib") / normImport);
            for (const auto& inc : extraIncludePaths) {
                candidates.push_back(fs::path(inc) / normImport);
            }

            // Also search parent directory stdlib
            candidates.push_back(fs::path(baseDir) / normImport);
            candidates.push_back(fs::path(baseDir) / "stdlib" / normImport);

            bool found = false;
            for (const auto& cand : candidates) {
                if (fs::exists(cand) && !fs::is_directory(cand)) {
                    std::string absPath = fs::weakly_canonical(cand).string();
                    if (visited.insert(absPath).second) {
                        resolvedPaths.push_back(absPath);
                        // Read and scan sub-imports transitively
                        std::ifstream subF(absPath);
                        if (subF.is_open()) {
                            std::stringstream ss;
                            ss << subF.rdbuf();
                            scanFile(ss.str(), fs::path(absPath).parent_path().string());
                        }
                    }
                    found = true;
                    break;
                }
            }
        }
    };

    scanFile(sourceCode, baseDir);
    return resolvedPaths;
}

// ============================================================================
// 4. Binary Payload Builder
// ============================================================================

std::vector<uint8_t> TzdExeCompiler::buildPayload(
    const std::string& entryName,
    const std::vector<uint8_t>& bytecodeBytes,
    const std::string& sourceCode,
    const std::vector<std::pair<std::string, std::string>>& embeddedFiles,
    const ExeCompileOptions& options)
{
    std::ostringstream ss(std::ios::binary);

    ExePayloadHeader header;
    header.magic = ExePayloadHeader::MAGIC;
    header.version = ExePayloadHeader::VERSION;
    header.format = (uint32_t)options.payloadFormat;
    header.flags = 0;
    if (options.silent) header.flags |= 1;
    if (!options.enableJit) header.flags |= 2;

    header.entryNameLength = static_cast<uint32_t>(entryName.size());
    header.mainPayloadSize = static_cast<uint64_t>(
        options.payloadFormat == PayloadFormat::BYTECODE ? bytecodeBytes.size() : sourceCode.size()
    );
    header.embeddedFileCount = static_cast<uint32_t>(embeddedFiles.size());

    // Reserve placeholder for header
    std::streampos headerPos = ss.tellp();
    ss.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // 1. Write entry script name
    if (header.entryNameLength > 0) {
        ss.write(entryName.data(), header.entryNameLength);
    }

    // 2. Write primary payload according to selected format
    if (options.payloadFormat == PayloadFormat::BYTECODE) {
        if (!bytecodeBytes.empty()) {
            ss.write(reinterpret_cast<const char*>(bytecodeBytes.data()), bytecodeBytes.size());
        }
    } else if (options.payloadFormat == PayloadFormat::SCRIPT_SOURCE) {
        if (!sourceCode.empty()) {
            ss.write(sourceCode.data(), sourceCode.size());
        }
    } else if (options.payloadFormat == PayloadFormat::HYBRID) {
        uint32_t srcLen = static_cast<uint32_t>(sourceCode.size());
        ss.write(reinterpret_cast<const char*>(&srcLen), sizeof(srcLen));
        if (srcLen > 0) {
            ss.write(sourceCode.data(), srcLen);
        }

        uint32_t bcLen = static_cast<uint32_t>(bytecodeBytes.size());
        ss.write(reinterpret_cast<const char*>(&bcLen), sizeof(bcLen));
        if (bcLen > 0) {
            ss.write(reinterpret_cast<const char*>(bytecodeBytes.data()), bcLen);
        }
    }

    // 4. Write embedded auxiliary files (stdlib / local modules)
    for (const auto& [relPath, content] : embeddedFiles) {
        uint32_t pathLen = static_cast<uint32_t>(relPath.size());
        ss.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));
        ss.write(relPath.data(), pathLen);

        uint64_t contentSize = static_cast<uint64_t>(content.size());
        ss.write(reinterpret_cast<const char*>(&contentSize), sizeof(contentSize));
        if (contentSize > 0) {
            ss.write(content.data(), contentSize);
        }
    }

    std::string payloadStr = ss.str();
    std::vector<uint8_t> payloadData(payloadStr.begin(), payloadStr.end());

    // Compute checksum over payload data (skipping header checksum field itself)
    uint64_t cksum = computeChecksum(payloadData.data() + sizeof(ExePayloadHeader), payloadData.size() - sizeof(ExePayloadHeader));
    reinterpret_cast<ExePayloadHeader*>(payloadData.data())->checksum = cksum;

    return payloadData;
}

// ============================================================================
// 5. PE Overlay Injection
// ============================================================================

bool TzdExeCompiler::injectPayloadIntoExe(
    const std::string& stubExePath,
    const std::string& targetExePath,
    const std::vector<uint8_t>& payloadData,
    std::string& err)
{
    std::ifstream stubFile(stubExePath, std::ios::binary);
    if (!stubFile.is_open()) {
        err = "Failed to open runtime stub executable: " + stubExePath;
        return false;
    }

    // Read stub bytes
    stubFile.seekg(0, std::ios::end);
    size_t stubSize = stubFile.tellg();
    stubFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> cleanStub(stubSize);
    stubFile.read(reinterpret_cast<char*>(cleanStub.data()), stubSize);
    stubFile.close();

    // If stub already has an embedded payload trailer, strip it to avoid recursive nesting
    if (stubSize >= sizeof(ExePayloadTrailer)) {
        const ExePayloadTrailer* existingTrailer = reinterpret_cast<const ExePayloadTrailer*>(
            cleanStub.data() + stubSize - sizeof(ExePayloadTrailer)
        );
        if (existingTrailer->trailerMagic == ExePayloadHeader::TRAILER_MAGIC &&
            existingTrailer->payloadOffset < stubSize)
        {
            cleanStub.resize(static_cast<size_t>(existingTrailer->payloadOffset));
            stubSize = cleanStub.size();
        }
    }

    // Create target directory
    std::error_code ec;
    fs::path parent = fs::path(targetExePath).parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
        fs::create_directories(parent, ec);
    }

    // Write clean stub + payload + trailer
    std::ofstream outExe(targetExePath, std::ios::binary | std::ios::trunc);
    if (!outExe.is_open()) {
        err = "Cannot create output executable: " + targetExePath;
        return false;
    }

    // 1. Write PE Base stub
    outExe.write(reinterpret_cast<const char*>(cleanStub.data()), cleanStub.size());

    // 2. Write Payload Data
    uint64_t payloadOffset = static_cast<uint64_t>(cleanStub.size());
    outExe.write(reinterpret_cast<const char*>(payloadData.data()), payloadData.size());

    // 3. Write Trailer Footer
    ExePayloadTrailer trailer;
    trailer.payloadOffset = payloadOffset;
    trailer.payloadSize = static_cast<uint64_t>(payloadData.size());
    trailer.trailerMagic = ExePayloadHeader::TRAILER_MAGIC;
    outExe.write(reinterpret_cast<const char*>(&trailer), sizeof(trailer));

    outExe.flush();
    outExe.close();

    return true;
}

// ============================================================================
// 6. Compiler Core Implementation
// ============================================================================

TzdExeCompiler::TzdExeCompiler() = default;
TzdExeCompiler::~TzdExeCompiler() = default;

ExeCompileResult TzdExeCompiler::compileTzd(
    const std::string& tzdFilePath,
    const std::string& outExePath,
    const ExeCompileOptions& options)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    ExeCompileResult res;

    if (!fs::exists(tzdFilePath)) {
        res.errorMessage = "Source file does not exist: " + tzdFilePath;
        return res;
    }

    // Derive output executable name if not specified
    std::string targetExe = outExePath;
    if (targetExe.empty()) {
        fs::path p(tzdFilePath);
        targetExe = (p.parent_path() / (p.stem().string() + ".exe")).string();
    }

    if (options.onProgress) options.onProgress("Reading source file...", 10);

    std::ifstream srcFile(tzdFilePath);
    if (!srcFile.is_open()) {
        res.errorMessage = "Failed to open input script: " + tzdFilePath;
        return res;
    }
    std::stringstream ss;
    ss << srcFile.rdbuf();
    std::string sourceCode = ss.str();
    srcFile.close();

    std::string entryName = fs::path(tzdFilePath).filename().string();
    return compileSource(sourceCode, entryName, targetExe, options);
}

ExeCompileResult TzdExeCompiler::compileSource(
    const std::string& sourceCode,
    const std::string& entryName,
    const std::string& outExePath,
    const ExeCompileOptions& options)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    ExeCompileResult res;
    std::string targetExe = fs::absolute(outExePath).string();
    res.outputExePath = targetExe;

    printProgressBar(10, "正在读取并解析源代码 AST...");
    if (options.onProgress) options.onProgress("Parsing AST & compiling bytecode...", 25);

    // 1. Compile source to bytecode via ANTLR4 & TzdBytecodeCompiler
    antlr4::ANTLRInputStream input(sourceCode);
    TzdLangLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();

    TzdLangParser parser(&tokens);
    auto* tree = parser.program();
    if (parser.getNumberOfSyntaxErrors() > 0) {
        res.errorMessage = "Tzd compilation failed: " + std::to_string(parser.getNumberOfSyntaxErrors()) + " syntax error(s).";
        return res;
    }

    // ========================================================================
    // AOT Native Machine Code Compilation Mode (Default)
    // Translates AST directly into native C++20 and compiles to standalone .exe
    // ========================================================================
    if (options.targetMode == ExeTargetMode::NATIVE_MACHINE_CODE || options.targetMode == ExeTargetMode::NATIVE_CODEGEN) {
        printProgressBar(35, "正在分析语法树结构、类定义与符号表...");
        if (options.onProgress) options.onProgress("Analyzing AST & symbols...", 35);

        printProgressBar(60, "正在生成原生机器码 IR/C++ 高性能源码...");
        if (options.onProgress) options.onProgress("Generating native C++...", 60);

        TzdNativeCodegen codegen;
        std::string cppCode = codegen.generate(tree, entryName, options.cpuOnly);

        fs::path outPath(targetExe);
        fs::path outDir = outPath.parent_path();
        if (!outDir.empty() && !fs::exists(outDir)) {
            std::error_code ec;
            fs::create_directories(outDir, ec);
        }

        // If user requested only C++ source generation (--codegen):
        if (options.targetMode == ExeTargetMode::NATIVE_CODEGEN) {
            std::string cppOut = (outDir / (outPath.stem().string() + ".cpp")).string();
            std::ofstream f(cppOut);
            f << cppCode;
            f.close();
            res.success = true;
            res.outputExePath = cppOut;
            printProgressBar(100, "原生 C++ 代码生成完成！");
            std::cout << std::endl;
            auto endTime = std::chrono::high_resolution_clock::now();
            res.durationSeconds = std::chrono::duration<double>(endTime - startTime).count();
            return res;
        }

        // Invoke native MSVC/Clang compiler to produce machine code
        printProgressBar(80, "正在调用原生编译器编译为 x86_64 机器码...");
        if (options.onProgress) options.onProgress("Compiling to native machine code...", 80);

        std::string cppPath = (outDir / ("tmp_" + outPath.stem().string() + ".cpp")).string();
        std::ofstream cppFile(cppPath);
        cppFile << cppCode;
        cppFile.close();

        std::string compileErr;
        bool ok = compileCppToExe(cppPath, targetExe, options, compileErr);

        if (!options.keepCpp && fs::exists(cppPath)) {
            std::error_code ec;
            fs::remove(cppPath, ec);
        }

        if (!ok) {
            std::cout << std::endl;
            res.errorMessage = "Native machine code compilation failed: " + compileErr;
            return res;
        }

        printProgressBar(100, "独立原生机器码可执行文件生成完成！");
        std::cout << std::endl;
        if (options.onProgress) options.onProgress("Machine code compilation complete!", 100);

        res.success = true;
        std::error_code ec;
        res.totalExeSizeBytes = fs::file_size(targetExe, ec);
        auto endTime = std::chrono::high_resolution_clock::now();
        res.durationSeconds = std::chrono::duration<double>(endTime - startTime).count();
        return res;
    }

    // ========================================================================
    // Legacy PE Overlay Stub Mode (Fallback)
    // ========================================================================
    TzdBytecodeCompiler compiler;
    BytecodeModule mod = compiler.compile(tree, sourceCode);
    std::string verifyErr;
    if (!compiler.verifyModule(mod, &verifyErr)) {
        res.errorMessage = "Bytecode verification failed: " + verifyErr;
        return res;
    }

    // Serialize BytecodeModule into bytes using in-memory stringstream
    std::string tempBcPath = (fs::temp_directory_path() / ("tmp_bc_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".tzdc")).string();
    if (!compiler.saveToFile(mod, tempBcPath)) {
        res.errorMessage = "Failed to serialize bytecode module.";
        return res;
    }

    std::ifstream bcIn(tempBcPath, std::ios::binary);
    std::vector<uint8_t> bytecodeBytes((std::istreambuf_iterator<char>(bcIn)), std::istreambuf_iterator<char>());
    bcIn.close();
    fs::remove(tempBcPath);

    printProgressBar(40, "正在编译字节码（优化等级 O" + std::to_string(options.optLevel) + "）...");
    if (options.onProgress) options.onProgress("Resolving and embedding native module dependencies...", 50);

    // 2. Scan and bundle imported modules (including stdlib)
    std::vector<std::pair<std::string, std::string>> embeddedFiles;
    if (options.bundleStdlib) {
        fs::path baseDir = fs::current_path();
        std::vector<std::string> resolvedImports = scanAndResolveImports(sourceCode, baseDir.string(), options.extraIncludePaths);

        for (const auto& impFile : resolvedImports) {
            std::ifstream f(impFile);
            if (f.is_open()) {
                std::stringstream fss;
                fss << f.rdbuf();
                // Determine virtual path relative to stdlib or workspace
                std::string relPath = impFile;
                size_t pos = relPath.find("stdlib\\");
                if (pos == std::string::npos) pos = relPath.find("stdlib/");
                if (pos != std::string::npos) {
                    relPath = relPath.substr(pos);
                } else {
                    relPath = fs::path(impFile).filename().string();
                }
                std::replace(relPath.begin(), relPath.end(), '\\', '/');
                embeddedFiles.push_back({ relPath, fss.str() });
            }
        }
    }

    // Bundle extra requested assets
    for (const auto& asset : options.extraAssets) {
        if (fs::exists(asset)) {
            std::ifstream af(asset, std::ios::binary);
            if (af.is_open()) {
                std::string content((std::istreambuf_iterator<char>(af)), std::istreambuf_iterator<char>());
                embeddedFiles.push_back({ fs::path(asset).filename().string(), content });
            }
        }
    }

    res.bundledFilesCount = static_cast<int>(embeddedFiles.size());

    printProgressBar(70, "正在构建独立可执行文件载荷包...");
    if (options.onProgress) options.onProgress("Building binary payload...", 70);

    // 3. Build standalone payload
    std::vector<uint8_t> payload = buildPayload(entryName, bytecodeBytes, sourceCode, embeddedFiles, options);
    res.payloadSizeBytes = payload.size();

    // 4. Locate base runtime executable stub
    std::string stubPath = options.customStubPath;
    if (stubPath.empty()) {
        stubPath = findRuntimeStub();
    }
    if (stubPath.empty() || !fs::exists(stubPath)) {
        res.errorMessage = "Runtime base executable stub (TzdTools.exe) not found. Please specify customStubPath or ensure TzdTools.exe is in PATH.";
        return res;
    }

    printProgressBar(85, "正在注入载荷到 PE 可执行文件...");
    if (options.onProgress) options.onProgress("Injecting payload into PE executable...", 85);

    // 5. Inject payload into output executable
    std::string err;
    if (!injectPayloadIntoExe(stubPath, targetExe, payload, err)) {
        std::cout << std::endl;
        res.errorMessage = err;
        return res;
    }

    // 6. Optionally copy companion runtime DLLs (smart filtering applied)
    if (options.copyDependencies && !options.cpuOnly) {
        printProgressBar(93, "正在复制运行时依赖 DLL（智能过滤中）...");
        fs::path stubDir = fs::path(stubPath).parent_path();
        fs::path outDir = fs::path(targetExe).parent_path();
        copyRuntimeDependencies(stubDir.string(), outDir.string(), sourceCode, options);
    }

    printProgressBar(100, "可执行文件生成完成！");
    std::cout << std::endl;
    if (options.onProgress) options.onProgress("Executable generation complete!", 100);

    res.success = true;
    res.totalExeSizeBytes = fs::file_size(targetExe);
    auto endTime = std::chrono::high_resolution_clock::now();
    res.durationSeconds = std::chrono::duration<double>(endTime - startTime).count();

    return res;
}

ExeCompileResult TzdExeCompiler::compileBytecode(
    const std::string& tzdcFilePath,
    const std::string& outExePath,
    const ExeCompileOptions& options)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    ExeCompileResult res;

    if (!fs::exists(tzdcFilePath)) {
        res.errorMessage = "Bytecode file not found: " + tzdcFilePath;
        return res;
    }

    std::string targetExe = outExePath;
    if (targetExe.empty()) {
        fs::path p(tzdcFilePath);
        targetExe = (p.parent_path() / (p.stem().string() + ".exe")).string();
    }

    // Read raw bytecode bytes
    std::ifstream bcFile(tzdcFilePath, std::ios::binary);
    if (!bcFile.is_open()) {
        res.errorMessage = "Failed to open bytecode file: " + tzdcFilePath;
        return res;
    }
    std::vector<uint8_t> bytecodeBytes((std::istreambuf_iterator<char>(bcFile)), std::istreambuf_iterator<char>());
    bcFile.close();

    // Load BytecodeModule to inspect embedded sourceCode for dynamic classes and imports
    TzdBytecodeCompiler compiler;
    BytecodeModule mod = compiler.loadFromFile(tzdcFilePath);

    std::string entryName = fs::path(tzdcFilePath).stem().string() + ".tzd";
    return compileSource(mod.sourceCode, entryName, targetExe, options);
}

// ============================================================================
// 7. Embedded Application Runtime Execution
// ============================================================================

bool TzdExeCompiler::tryRunEmbeddedExecutable(int argc, char* argv[], int* outExitCode) {
    if (outExitCode) *outExitCode = 0;

    // 1. Get current process executable path
    wchar_t exePathBuf[MAX_PATH];
    if (!GetModuleFileNameW(NULL, exePathBuf, MAX_PATH)) {
        return false;
    }

    std::ifstream exeFile(exePathBuf, std::ios::binary);
    if (!exeFile.is_open()) return false;

    // 2. Read trailer at end of file
    exeFile.seekg(0, std::ios::end);
    size_t fileSize = exeFile.tellg();
    if (fileSize < sizeof(ExePayloadTrailer)) return false;

    exeFile.seekg(fileSize - sizeof(ExePayloadTrailer), std::ios::beg);
    ExePayloadTrailer trailer;
    exeFile.read(reinterpret_cast<char*>(&trailer), sizeof(trailer));

    // Verify trailer magic
    if (trailer.trailerMagic != ExePayloadHeader::TRAILER_MAGIC ||
        trailer.payloadOffset >= fileSize) {
        return false;
    }

    // 3. Read payload header
    exeFile.seekg(trailer.payloadOffset, std::ios::beg);
    ExePayloadHeader header;
    exeFile.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.magic != ExePayloadHeader::MAGIC || header.version != ExePayloadHeader::VERSION) {
        return false;
    }

    // 4. Read entry script name
    std::string entryName;
    if (header.entryNameLength > 0) {
        entryName.resize(header.entryNameLength);
        exeFile.read(&entryName[0], header.entryNameLength);
    }

    // 5. Read primary payload (bytecode or source)
    std::string sourceCode;
    std::vector<uint8_t> bytecodeBytes;

    if (header.format == (uint32_t)PayloadFormat::BYTECODE) {
        bytecodeBytes.resize(header.mainPayloadSize);
        exeFile.read(reinterpret_cast<char*>(bytecodeBytes.data()), header.mainPayloadSize);
    } else if (header.format == (uint32_t)PayloadFormat::SCRIPT_SOURCE) {
        sourceCode.resize(header.mainPayloadSize);
        exeFile.read(&sourceCode[0], header.mainPayloadSize);
    } else if (header.format == (uint32_t)PayloadFormat::HYBRID) {
        uint32_t srcLen = 0;
        exeFile.read(reinterpret_cast<char*>(&srcLen), sizeof(srcLen));
        if (srcLen > 0) {
            sourceCode.resize(srcLen);
            exeFile.read(&sourceCode[0], srcLen);
        }

        uint32_t bcLen = 0;
        exeFile.read(reinterpret_cast<char*>(&bcLen), sizeof(bcLen));
        if (bcLen > 0) {
            bytecodeBytes.resize(bcLen);
            exeFile.read(reinterpret_cast<char*>(bytecodeBytes.data()), bcLen);
        }
    }

    // 6. Read embedded auxiliary files (stdlib / imports)
    EmbeddedVFS& vfs = EmbeddedVFS::instance();
    vfs.clear();

    for (uint32_t i = 0; i < header.embeddedFileCount; ++i) {
        uint32_t pathLen = 0;
        exeFile.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
        std::string relPath(pathLen, '\0');
        if (pathLen > 0) {
            exeFile.read(&relPath[0], pathLen);
        }

        uint64_t contentSize = 0;
        exeFile.read(reinterpret_cast<char*>(&contentSize), sizeof(contentSize));
        std::string content(contentSize, '\0');
        if (contentSize > 0) {
            exeFile.read(&content[0], contentSize);
        }
        vfs.addFile(relPath, content);
    }

    exeFile.close();

    // 7. Extract embedded files to runtime temp directory for native/file access
    fs::path tempBundleDir = fs::temp_directory_path() / ("tzd_bundle_" + std::to_string(header.checksum));
    try {
        vfs.extractToDirectory(tempBundleDir);
    } catch (...) {}

    // 8. Initialize Tzd Runtime & Native Modules
    TzdInterpreter interpreter;
    interpreter.m_includePaths.push_back(tempBundleDir.string());
    interpreter.m_includePaths.push_back((tempBundleDir / "stdlib").string());
    interpreter.m_silentMode = (header.flags & 1) != 0;
    interpreter.m_noJit = (header.flags & 2) != 0;

    // Register all native modules (Math, System, IO, String, Array, FFI, Json, etc.)
    TzdNativeModule::init(&interpreter);
#ifdef WITH_LIBTORCH
    TzdPyTorch::init(&interpreter);
#endif

    // Inject command line arguments (ARGV and args) into script environment
    std::vector<TzdValue> argsList;
    for (int i = 0; i < argc; ++i) {
        argsList.push_back(TzdValue(std::string(argv[i])));
    }
    if (interpreter.scopes.empty()) interpreter.scopes.push_back({});
    interpreter.scopes[0]["ARGV"] = TzdValue(argsList);
    interpreter.scopes[0]["args"] = TzdValue(argsList);

    // 9. Execute the bundled application
    try {
        if (!bytecodeBytes.empty()) {
            // Write temporary .tzdc and execute bytecode VM
            fs::path tempBcPath = tempBundleDir / "embedded.tzdc";
            std::ofstream bcOut(tempBcPath, std::ios::binary);
            bcOut.write(reinterpret_cast<const char*>(bytecodeBytes.data()), bytecodeBytes.size());
            bcOut.close();

            interpreter.executeBytecodeFile(tempBcPath.string());
        } else if (!sourceCode.empty()) {
            interpreter.loadScript(sourceCode);

            // Invoke main(args) if defined
            if (!interpreter.scopes.empty() && interpreter.scopes[0].count("main")) {
                TzdValue mainFunc = interpreter.scopes[0]["main"];
                if (mainFunc.type == TzdValue::FUNCTION) {
                    std::vector<TzdValue> mainArgs;
                    if (!mainFunc.params.empty()) {
                        mainArgs.push_back(TzdValue(argsList));
                    }
                    interpreter.callFunction(mainFunc, mainArgs);
                }
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[Tzd Application Error] " << e.what() << std::endl;
        if (outExitCode) *outExitCode = 1;
    }
    catch (...) {
        std::cerr << "[Tzd Application Error] Unhandled fatal exception." << std::endl;
        if (outExitCode) *outExitCode = 2;
    }

    fflush(stdout);
    fflush(stderr);
    return true;
}

// ============================================================================
// 8. Native C++ Code Generator Mode
// ============================================================================

std::string TzdExeCompiler::generateNativeCppSource(
    const std::string& entryName,
    const std::vector<uint8_t>& bytecodeData,
    const std::string& sourceCode,
    const std::vector<std::pair<std::string, std::string>>& embeddedFiles)
{
    std::ostringstream out;
    out << "// ============================================================================\n";
    out << "// Auto-generated standalone C++ wrapper by TzdExeCompiler\n";
    out << "// ============================================================================\n\n";
    out << "#define NOMINMAX\n#define WIN32_LEAN_AND_MEAN\n#include <windows.h>\n";
    out << "#include \"TzdExeCompiler.h\"\n";
    out << "#include \"Generated/TzdInterpreter.h\"\n";
    out << "#include \"Generated/TzdNativeModule.h\"\n";
    out << "#include \"Generated/TzdPyTorch.h\"\n\n";

    // Embed bytecode array
    out << "static const unsigned char g_tzd_bytecode[] = {\n";
    for (size_t i = 0; i < bytecodeData.size(); ++i) {
        out << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)bytecodeData[i] << ", ";
        if ((i + 1) % 16 == 0) out << "\n";
    }
    out << "\n};\n";
    out << "static const size_t g_tzd_bytecode_len = " << std::dec << bytecodeData.size() << ";\n\n";

    out << "int main(int argc, char* argv[]) {\n";
    out << "    SetConsoleOutputCP(CP_UTF8);\n";
    out << "    SetConsoleCP(CP_UTF8);\n";
    out << "    TzdInterpreter interp;\n";
    out << "    TzdNativeModule::init(&interp);\n";
    out << "    TzdPyTorch::init(&interp);\n";
    out << "    return 0;\n";
    out << "}\n";

    return out.str();
}

} // namespace tzd
