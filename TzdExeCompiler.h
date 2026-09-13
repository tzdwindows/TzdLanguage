#ifndef TZD_EXE_COMPILER_H
#define TZD_EXE_COMPILER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <filesystem>

// Forward declarations
class TzdInterpreter;
struct BytecodeModule;

namespace tzd {

/**
 * Packaging & Compilation Target Mode
 */
enum class ExeTargetMode : uint32_t {
    NATIVE_MACHINE_CODE = 0, // AOT: Directly compile to native x86_64 machine code .exe via Clang/MSVC (Zero-DLL, tiny size)
    STANDALONE_PE = 1,       // Legacy PE overlay injection with embedded runtime stub
    NATIVE_CODEGEN = 2       // Only emit standalone native C++ source code
};

/**
 * Payload payload format
 */
enum class PayloadFormat : uint32_t {
    BYTECODE = 1,        // Pure serialized BytecodeModule
    SCRIPT_SOURCE = 2,   // Full .tzd source code
    HYBRID = 3           // Both BytecodeModule and source code for maximum performance & dynamic OOP
};

/**
 * Compilation options
 */
struct ExeCompileOptions {
    std::string outputPath;                 // Target executable path (empty = auto derive from input)
    std::string customStubPath;             // Custom runtime base executable (empty = auto detect)
    ExeTargetMode targetMode = ExeTargetMode::NATIVE_MACHINE_CODE; // Default: true AOT machine code!
    PayloadFormat payloadFormat = PayloadFormat::HYBRID;

    bool bundleStdlib = true;               // Automatically bundle referenced stdlib modules
    bool bundleAllStdlib = false;           // Bundle the entire standard library
    bool silent = false;                    // Suppress Tzd startup banners in generated executable
    bool enableJit = true;                  // Enable JIT tiering engine inside generated executable
    bool copyDependencies = false;          // Copy dependent runtime DLLs to output directory (default: OFF)
    int optLevel = 2;                       // Optimization level (0-3)

    // === Smart / CPU-only build flags ===
    bool cpuOnly = false;                   // --buildCpu: strip ALL GPU/CUDA/torch modules; pure CPU standalone
    bool smartDeps = true;                  // Auto-detect: only copy torch/CUDA DLLs if source actually uses them
    bool keepCpp = false;                   // --keep-cpp: preserve generated native C++ source file

    std::vector<std::string> extraIncludePaths; // Additional module search paths to bundle
    std::vector<std::string> extraAssets;       // Additional asset/data files to bundle

    std::function<void(const std::string& msg, int progressPercent)> onProgress = nullptr;
};

/**
 * Compilation result details
 */
struct ExeCompileResult {
    bool success = false;
    std::string outputExePath;
    size_t payloadSizeBytes = 0;
    size_t totalExeSizeBytes = 0;
    int bundledFilesCount = 0;
    std::string errorMessage;
    double durationSeconds = 0.0;
};

#pragma pack(push, 1)
/**
 * Binary header placed at the beginning of the embedded payload
 */
struct ExePayloadHeader {
    static constexpr uint64_t MAGIC = 0x3230455845445A54ULL; // "TZDEXE02" in little-endian
    static constexpr uint64_t TRAILER_MAGIC = 0x32544F4F46445A54ULL; // "TZDFOOT2" in little-endian
    static constexpr uint32_t VERSION = 2;

    uint64_t magic = MAGIC;
    uint32_t version = VERSION;
    uint32_t format = (uint32_t)PayloadFormat::HYBRID;
    uint32_t flags = 0;                     // Bit 0: Silent, Bit 1: NoJit, Bit 2: HasMain
    uint64_t checksum = 0;                  // 64-bit payload integrity checksum
    uint64_t mainPayloadSize = 0;           // Size of primary bytecode or source payload
    uint32_t entryNameLength = 0;           // Length of entry script name
    uint32_t embeddedFileCount = 0;         // Number of bundled auxiliary files (stdlib / imports)
};

/**
 * Fixed-size trailer at the very end of the file allowing O(1) detection and offset lookup
 */
struct ExePayloadTrailer {
    uint64_t payloadOffset = 0;             // Absolute byte offset in file where ExePayloadHeader starts
    uint64_t payloadSize = 0;               // Total size of payload excluding trailer
    uint64_t trailerMagic = ExePayloadHeader::TRAILER_MAGIC;
};
#pragma pack(pop)

/**
 * In-Memory Virtual File System for standalone bundled applications
 */
class EmbeddedVFS {
public:
    static EmbeddedVFS& instance();

    void addFile(const std::string& virtualPath, std::vector<uint8_t> data);
    void addFile(const std::string& virtualPath, const std::string& text);
    bool hasFile(const std::string& virtualPath) const;
    std::string getFileContent(const std::string& virtualPath) const;
    const std::vector<uint8_t>* getFileBytes(const std::string& virtualPath) const;
    std::vector<std::string> listFiles() const;
    void extractToDirectory(const std::filesystem::path& targetDir) const;
    void clear();

private:
    std::unordered_map<std::string, std::vector<uint8_t>> m_files;
};

/**
 * High-performance standalone executable compiler for TzdLang
 */
class TzdExeCompiler {
public:
    TzdExeCompiler();
    ~TzdExeCompiler();

    /**
     * Compile a .tzd source file into a standalone executable.
     */
    ExeCompileResult compileTzd(const std::string& tzdFilePath,
                                const std::string& outExePath = "",
                                const ExeCompileOptions& options = ExeCompileOptions());

    /**
     * Compile a .tzdc bytecode file into a standalone executable.
     */
    ExeCompileResult compileBytecode(const std::string& tzdcFilePath,
                                     const std::string& outExePath = "",
                                     const ExeCompileOptions& options = ExeCompileOptions());

    /**
     * Compile in-memory source code into a standalone executable.
     */
    ExeCompileResult compileSource(const std::string& sourceCode,
                                   const std::string& entryName,
                                   const std::string& outExePath,
                                   const ExeCompileOptions& options = ExeCompileOptions());

    /**
     * Startup detector:
     * Checks if the currently running process contains an embedded payload.
     * If true, executes the bundled program immediately with all native APIs and returns true.
     * If false, returns false so standard CLI/REPL starts.
     */
    static bool tryRunEmbeddedExecutable(int argc, char* argv[], int* outExitCode = nullptr);

    /**
     * Auto-detect the base runtime executable stub.
     */
    static std::string findRuntimeStub();

    /**
     * Analyse source code and return whether it uses PyTorch/GPU APIs.
     * Used for smart dependency detection: only copy torch DLLs when actually needed.
     */
    static bool sourceUsesTorch(const std::string& sourceCode);
    static bool sourceUsesGpu(const std::string& sourceCode);

    /**
     * Copy required companion DLLs (e.g. torch, cublas, etc.) to target executable directory.
     * When smartDeps=true and cpuOnly=false, only copies torch/CUDA DLLs if sourceCode uses them.
     */
    static bool copyRuntimeDependencies(const std::string& stubExeDir,
                                        const std::string& targetExeDir,
                                        const std::string& sourceCode = "",
                                        const ExeCompileOptions& opts = ExeCompileOptions());

    /**
     * Compile native C++ source file into standalone x86_64 machine code .exe via MSVC / Clang.
     */
    static bool compileCppToExe(const std::string& cppPath,
                                const std::string& exePath,
                                const ExeCompileOptions& opts,
                                std::string& err);

    /**
     * Locate MSVC vcvars64.bat environment script.
     */
    static std::string findMsvcVcvars();

    /**
     * Generate standalone C++ source code embedding the bytecode and runtime initialization.
     */
    static std::string generateNativeCppSource(const std::string& entryName,
                                               const std::vector<uint8_t>& bytecodeData,
                                               const std::string& sourceCode,
                                               const std::vector<std::pair<std::string, std::string>>& embeddedFiles);

private:
    std::vector<std::string> scanAndResolveImports(const std::string& sourceCode,
                                                   const std::string& baseDir,
                                                   const std::vector<std::string>& extraIncludePaths);

    std::vector<uint8_t> buildPayload(const std::string& entryName,
                                      const std::vector<uint8_t>& bytecodeBytes,
                                      const std::string& sourceCode,
                                      const std::vector<std::pair<std::string, std::string>>& embeddedFiles,
                                      const ExeCompileOptions& options);

    bool injectPayloadIntoExe(const std::string& stubExePath,
                              const std::string& targetExePath,
                              const std::vector<uint8_t>& payloadData,
                              std::string& err);

    static uint64_t computeChecksum(const uint8_t* data, size_t size);
};

} // namespace tzd

#endif // TZD_EXE_COMPILER_H
