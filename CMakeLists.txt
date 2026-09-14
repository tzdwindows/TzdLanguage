cmake_minimum_required(VERSION 3.20)

# ===========================================================================
# TzdTools & TzdLang - Modern Portable CMake Configuration
# ===========================================================================
project(TzdTools
    VERSION 0.2.5
    DESCRIPTION "TzdLang Compiler & Execution Runtime"
    LANGUAGES C CXX
)

# Enable MASM on MSVC toolchains
if(MSVC)
    enable_language(ASM_MASM)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# User-Configurable Build Options
# ---------------------------------------------------------------------------
option(TZD_BUILD_STATIC       "Build with static C/C++ runtime (/MT or /MTd)" ON)
option(TZD_ENABLE_CUDA        "Enable NVIDIA CUDA GPU acceleration & NTT operators" ON)
option(TZD_ENABLE_LIBTORCH    "Enable LibTorch deep learning tensor integration" ON)
option(TZD_ENABLE_LLVM        "Enable LLVM ORC JIT execution engine and compiler" ON)
option(TZD_ENABLE_FINTAMATH   "Enable Fintamath exact mathematics engine" ON)
option(TZD_ENABLE_AVX2        "Enable AVX2 instruction set vectorization" ON)
option(TZD_ENABLE_OPENMP      "Enable OpenMP multi-threading parallelization" ON)

# Configurable stack size (default 256MB)
set(TZD_STACK_SIZE "268435456" CACHE STRING "Stack reserve and commit size in bytes (default 256MB)")

# Target vcpkg triplet
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "Target triplet for vcpkg packages")
endif()

# Force static runtime (/MT or /MTd) matching MSVC configuration if enabled
if(MSVC AND TZD_BUILD_STATIC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ---------------------------------------------------------------------------
# Intelligent Dependency Auto-Discovery (Configurable, No Hardcoded Fallbacks)
# ---------------------------------------------------------------------------

# 1. vcpkg Root & Installed Directory
find_path(VCPKG_ROOT_DIR
    NAMES vcpkg.exe vcpkg
    HINTS
        "$ENV{VCPKG_ROOT}"
        "${VCPKG_ROOT}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg"
        "C:/vcpkg"
        "D:/vcpkg"
        "E:/vcpkg"
    DOC "Path to vcpkg installation root directory"
)

if(VCPKG_ROOT_DIR AND EXISTS "${VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}")
    set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT_DIR}/installed/${VCPKG_TARGET_TRIPLET}" CACHE PATH "Path to installed vcpkg packages")
elseif(NOT DEFINED VCPKG_INSTALLED_DIR)
    find_path(VCPKG_INSTALLED_DIR
        NAMES include/fmt/format.h
        HINTS
            "$ENV{VCPKG_INSTALLED_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}"
        DOC "Path to vcpkg installed packages directory"
    )
endif()

# 2. CUDA Toolkit
if(TZD_ENABLE_CUDA)
    find_path(CUDA_ROOT_DIR
        NAMES include/cuda.h
        HINTS
            "$ENV{CUDA_PATH}"
            "$ENV{CUDA_ROOT}"
            "${CUDA_TOOLKIT_ROOT_DIR}"
            "$ENV{CUDA_TOOLKIT_ROOT_DIR}"
            "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6"
            "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5"
            "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.4"
            "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0"
        DOC "Path to NVIDIA CUDA Toolkit root directory"
    )
    if(CUDA_ROOT_DIR AND EXISTS "${CUDA_ROOT_DIR}/include/cuda.h")
        set(HAVE_CUDA TRUE)
    else()
        set(HAVE_CUDA FALSE)
        message(STATUS "CUDA Toolkit not detected; CUDA GPU acceleration will be disabled.")
    endif()
else()
    set(HAVE_CUDA FALSE)
endif()

# 3. LibTorch
if(TZD_ENABLE_LIBTORCH)
    if(NOT HAVE_CUDA)
        set(LIBTORCH_HINTS
            "$ENV{LIBTORCH_CPU_PATH}"
            "$ENV{LIBTORCH_PATH}"
            "$ENV{LIBTORCH_ROOT}"
            "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch_cpu_backup"
            "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch_cpu"
            "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch"
            "C:/libtorch_cpu"
            "C:/libtorch"
            "D:/libtorch"
            "E:/libtorch"
        )
    else()
        set(LIBTORCH_HINTS
            "$ENV{LIBTORCH_PATH}"
            "$ENV{LIBTORCH_ROOT}"
            "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch"
            "${CMAKE_CURRENT_SOURCE_DIR}/libtorch"
            "C:/libtorch"
            "D:/libtorch"
            "E:/libtorch"
        )
    endif()
    find_path(LIBTORCH_ROOT
        NAMES lib/torch.lib include/torch/csrc/api/include/torch/torch.h
        HINTS ${LIBTORCH_HINTS}
        DOC "Path to LibTorch root directory"
    )
    if(LIBTORCH_ROOT AND (EXISTS "${LIBTORCH_ROOT}/lib/torch.lib" OR EXISTS "${LIBTORCH_ROOT}/include/torch/csrc/api/include/torch/torch.h"))
        set(HAVE_LIBTORCH TRUE)
    else()
        set(HAVE_LIBTORCH FALSE)
        message(STATUS "LibTorch not detected; deep learning operators will be disabled.")
    endif()
else()
    set(HAVE_LIBTORCH FALSE)
endif()

# 4. LLVM SDK
if(TZD_ENABLE_LLVM)
    find_path(LLVM_ROOT_DIR
        NAMES include/llvm/IR/LLVMContext.h
        HINTS
            "$ENV{LLVM_DIR}"
            "$ENV{LLVM_ROOT}"
            "${VCPKG_INSTALLED_DIR}"
            "C:/Program Files/LLVM"
            "C:/LLVM"
            "D:/LLVM_SDK"
            "E:/LLVM_SDK"
        DOC "Path to LLVM SDK root directory"
    )
    if(LLVM_ROOT_DIR AND EXISTS "${LLVM_ROOT_DIR}/include/llvm/IR/LLVMContext.h")
        set(HAVE_LLVM TRUE)
    else()
        set(HAVE_LLVM FALSE)
        message(STATUS "LLVM SDK not detected; LLVM JIT engine will be disabled.")
    endif()
else()
    set(HAVE_LLVM FALSE)
endif()

# 5. Fintamath
if(TZD_ENABLE_FINTAMATH)
    find_path(FINTAMATH_ROOT_DIR
        NAMES include/fintamath/expressions/Expression.hpp f/include/fintamath/expressions/Expression.hpp f/build/lib/Release/fintamath.lib
        HINTS
            "$ENV{FINTAMATH_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/External/fintamath"
            "C:/Fintamath"
            "D:/Fintamath"
            "E:/Fintamath"
        DOC "Path to Fintamath root directory"
    )
    if(FINTAMATH_ROOT_DIR)
        set(HAVE_FINTAMATH TRUE)
    else()
        set(HAVE_FINTAMATH FALSE)
        message(STATUS "Fintamath not detected; exact mathematics engine will be disabled.")
    endif()
else()
    set(HAVE_FINTAMATH FALSE)
endif()

# 6. Java JDK (Optional for ANTLR / JNI tooling)
find_path(JDK_ROOT_DIR
    NAMES include/jni.h
    HINTS
        "$ENV{JAVA_HOME}"
        "C:/Program Files/Java/jdk-20"
        "C:/Program Files/Java/jdk-21"
        "C:/Program Files/Java/jdk-17"
    DOC "Path to Java JDK root directory (optional)"
)

# Configuration summary output
message(STATUS "------------------------------------------------------------")
message(STATUS "TzdTools Build Configuration:")
message(STATUS "  C++ Standard:    C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Static Runtime:  ${TZD_BUILD_STATIC}")
message(STATUS "  vcpkg Installed: ${VCPKG_INSTALLED_DIR}")
message(STATUS "  CUDA Support:    ${HAVE_CUDA} (${CUDA_ROOT_DIR})")
message(STATUS "  LibTorch:        ${HAVE_LIBTORCH} (${LIBTORCH_ROOT})")
message(STATUS "  LLVM JIT SDK:    ${HAVE_LLVM} (${LLVM_ROOT_DIR})")
message(STATUS "  Fintamath:       ${HAVE_FINTAMATH} (${FINTAMATH_ROOT_DIR})")
message(STATUS "  Java JDK:        ${JDK_ROOT_DIR}")
message(STATUS "------------------------------------------------------------")

# ---------------------------------------------------------------------------
# Source File Registration
# ---------------------------------------------------------------------------
set(TZD_SOURCES
    # Core CLI, Debugger, Profiler & StackTrace
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdTools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdDebugger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdStackTrace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdCommandSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdExeCompiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdNativeCodegen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdFuncScanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdMemoryAsm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PdbReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SymbolDemangler.cpp

    # Interpreter, Bytecode VM, JIT Engine, GC, FFI, OOP, PyTorch
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdInterpreter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdBytecode.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdBytecodeJIT.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdJit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdTieringEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdConsole.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdFFIAdapter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdNativeModule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdOop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdPyTorch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdExperimentalCompute.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangLexer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangVisitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangBaseVisitor.cpp

    # dyncall (C FFI + MASM ASM)
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_callvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_callvm_base.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_vector.c

    # pbPlots plotting library
    ${CMAKE_CURRENT_SOURCE_DIR}/Plots/pbPlots.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Plots/supportLib.cpp
)

if(MSVC)
    list(APPEND TZD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_call_x64_generic_masm.asm)
endif()

add_executable(TzdTools ${TZD_SOURCES})

# ---------------------------------------------------------------------------
# Preprocessor Definitions
# ---------------------------------------------------------------------------
target_compile_definitions(TzdTools PRIVATE
    CPPCORO_COMPILER_MSVC
    FMT_UNICODE=0
    _SILENCE_CLANG_COROUTINE_MESSAGE
    _CRT_SECURE_NO_WARNINGS
    DC__Feature_Aggr=0
    ANTLR4CPP_STATIC
    _CONSOLE
    UNICODE
    _UNICODE
    $<$<BOOL:${HAVE_CUDA}>:WITH_CUDA>
    $<$<BOOL:${HAVE_LIBTORCH}>:WITH_LIBTORCH>
    $<$<CONFIG:Release>:NDEBUG _ITERATOR_DEBUG_LEVEL=0>
    $<$<CONFIG:RelWithDebInfo>:NDEBUG _ITERATOR_DEBUG_LEVEL=0>
)

# ---------------------------------------------------------------------------
# Include Directories & Link Directories
# ---------------------------------------------------------------------------
target_include_directories(TzdTools PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall
    ${CMAKE_CURRENT_SOURCE_DIR}/Plots
)

# Vcpkg includes & libs
if(VCPKG_INSTALLED_DIR AND EXISTS "${VCPKG_INSTALLED_DIR}")
    target_include_directories(TzdTools PRIVATE
        "${VCPKG_INSTALLED_DIR}/include"
        "${VCPKG_INSTALLED_DIR}/include/antlr4-runtime"
    )
    target_link_directories(TzdTools PRIVATE
        "$<$<CONFIG:Debug>:${VCPKG_INSTALLED_DIR}/debug/lib>"
        "$<$<NOT:$<CONFIG:Debug>>:${VCPKG_INSTALLED_DIR}/lib>"
    )
endif()

# CUDA includes & libs
if(HAVE_CUDA)
    target_include_directories(TzdTools PRIVATE "${CUDA_ROOT_DIR}/include")
    target_link_directories(TzdTools PRIVATE "${CUDA_ROOT_DIR}/lib/x64")
endif()

# LibTorch includes & libs
if(HAVE_LIBTORCH)
    target_include_directories(TzdTools PRIVATE
        "${LIBTORCH_ROOT}/include"
        "${LIBTORCH_ROOT}/include/torch/csrc/api/include"
    )
    target_link_directories(TzdTools PRIVATE "${LIBTORCH_ROOT}/lib")
endif()

# LLVM includes & libs
if(HAVE_LLVM)
    target_include_directories(TzdTools PRIVATE "${LLVM_ROOT_DIR}/include")
    target_link_directories(TzdTools PRIVATE "${LLVM_ROOT_DIR}/lib")
endif()

# Fintamath includes & libs
if(HAVE_FINTAMATH)
    if(EXISTS "${FINTAMATH_ROOT_DIR}/include")
        target_include_directories(TzdTools PRIVATE "${FINTAMATH_ROOT_DIR}/include")
    endif()
    if(EXISTS "${FINTAMATH_ROOT_DIR}/f/include")
        target_include_directories(TzdTools PRIVATE "${FINTAMATH_ROOT_DIR}/f/include")
    endif()
    if(EXISTS "${FINTAMATH_ROOT_DIR}/f/build/lib/Release")
        target_link_directories(TzdTools PRIVATE "${FINTAMATH_ROOT_DIR}/f/build/lib/Release")
    elseif(EXISTS "${FINTAMATH_ROOT_DIR}/lib")
        target_link_directories(TzdTools PRIVATE "${FINTAMATH_ROOT_DIR}/lib")
    endif()
endif()

# Java JDK includes
if(JDK_ROOT_DIR AND EXISTS "${JDK_ROOT_DIR}/include")
    target_include_directories(TzdTools PRIVATE
        "${JDK_ROOT_DIR}/include"
        "${JDK_ROOT_DIR}/include/win32"
    )
endif()

# ---------------------------------------------------------------------------
# Compiler & Linker Options
# ---------------------------------------------------------------------------
if(MSVC)
    target_compile_options(TzdTools PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX>:
            /FS
            /bigobj
            /utf-8
            $<$<BOOL:${TZD_ENABLE_AVX2}>:/arch:AVX2>
            $<$<BOOL:${TZD_ENABLE_OPENMP}>:/openmp>
            /wd4146 /wd4244 /wd4267 /wd4624 /wd4005 /wd4819 /wd4996
        >
    )

    target_link_options(TzdTools PRIVATE
        "/STACK:${TZD_STACK_SIZE},${TZD_STACK_SIZE}"
        $<$<AND:$<BOOL:${TZD_BUILD_STATIC}>,$<NOT:$<CONFIG:Debug>>>:/NODEFAULTLIB:MSVCRT>
    )

    if(HAVE_LLVM)
        target_link_options(TzdTools PRIVATE
            "/WHOLEARCHIVE:LLVMX86CodeGen"
            "/WHOLEARCHIVE:LLVMX86Desc"
            "/WHOLEARCHIVE:LLVMX86Info"
            "/WHOLEARCHIVE:LLVMX86AsmParser"
            "/WHOLEARCHIVE:LLVMOrcJIT"
            "/WHOLEARCHIVE:LLVMExecutionEngine"
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# Link Libraries
# ---------------------------------------------------------------------------
# Base Windows System & Utility Libraries
target_link_libraries(TzdTools PRIVATE
    ws2_32
    IPHLPAPI
    dbghelp
    d3d11
    dxgi
    zlib
    zstd
    fmt
    asmjit
)

# Optional Base Libraries if available in vcpkg
if(EXISTS "${VCPKG_INSTALLED_DIR}/lib/imgui.lib")
    target_link_libraries(TzdTools PRIVATE imgui)
endif()
if(EXISTS "${VCPKG_INSTALLED_DIR}/lib/minhook.x64.lib")
    target_link_libraries(TzdTools PRIVATE minhook.x64)
endif()

# CUDA Driver, Runtime, NVRTC, cuFFT
if(HAVE_CUDA)
    target_link_libraries(TzdTools PRIVATE
        cudart
        cufft
        cuda
        nvrtc
    )
endif()

# LibTorch & Deep Learning Dependencies
if(HAVE_LIBTORCH)
    target_link_libraries(TzdTools PRIVATE
        torch
        torch_cpu
        c10
        fbgemm
        cpuinfo
        dnnl
        XNNPACK
        pthreadpool
    )
    if(HAVE_CUDA)
        target_link_libraries(TzdTools PRIVATE
            torch_cuda
            c10_cuda
        )
    endif()
endif()

# Fintamath
if(HAVE_FINTAMATH)
    target_link_libraries(TzdTools PRIVATE fintamath)
endif()

# Clang, LLD, and LLVM Libraries
if(HAVE_LLVM)
    target_link_libraries(TzdTools PRIVATE
        # Clang Libraries
        clang-repl clangAnalysis clangAnalysisFlowSensitive clangAnalysisFlowSensitiveModels
        clangAPINotes clangAST clangASTMatchers clangBasic clangCodeGen clangCrossTU
        clangDependencyScanning clangDirectoryWatcher clangDriver clangDynamicASTMatchers
        clangEdit clangExtractAPI clangFormat clangFrontend clangFrontendTool clangHandleCXX
        clangHandleLLVM clangIndex clangIndexSerialization clangInterpreter clangLex
        clangParse clangRewrite clangRewriteFrontend clangSema clangSerialization
        clangStaticAnalyzerCheckers clangStaticAnalyzerCore clangStaticAnalyzerFrontend clangSupport
        clangTooling clangToolingASTDiff clangToolingCore clangToolingInclusions
        clangToolingInclusionsStdlib clangToolingRefactoring clangToolingSyntax clangTransformer libclang

        # LLD Linker Libraries
        lldCOFF lldCommon lldELF lldMachO lldMinGW lldWasm

        # LLVM Libraries
        LLVMAggressiveInstCombine LLVMAnalysis LLVMAsmParser LLVMAsmPrinter LLVMBinaryFormat
        LLVMBitReader LLVMBitstreamReader LLVMBitWriter LLVMCFGuard LLVMCFIVerify LLVMCodeGen
        LLVMCodeGenTypes LLVMCore LLVMCoroutines LLVMCoverage LLVMDebugInfoBTF LLVMDebugInfoCodeView
        LLVMDebuginfod LLVMDebugInfoDWARF LLVMDebugInfoGSYM LLVMDebugInfoLogicalView LLVMDebugInfoMSF
        LLVMDebugInfoPDB LLVMDemangle LLVMDiff LLVMDlltoolDriver LLVMDWARFLinker
        LLVMDWARFLinkerClassic LLVMDWARFLinkerParallel LLVMDWP LLVMExecutionEngine LLVMExegesis
        LLVMExegesisX86 LLVMExtensions LLVMFileCheck LLVMFrontendDriver LLVMFrontendHLSL
        LLVMFrontendOffloading LLVMFrontendOpenACC LLVMFrontendOpenMP LLVMFuzzerCLI LLVMFuzzMutate
        LLVMGlobalISel LLVMHipStdPar LLVMInstCombine LLVMInstrumentation LLVMInterfaceStub
        LLVMInterpreter LLVMipo LLVMIRPrinter LLVMIRReader LLVMJITLink LLVMLibDriver
        LLVMLineEditor LLVMLinker LLVMLTO LLVMMC LLVMMCA LLVMMCDisassembler LLVMMCJIT
        LLVMMCParser LLVMMIRParser LLVMObjCARCOpts LLVMObjCopy LLVMObject LLVMObjectYAML
        LLVMOption LLVMOrcDebugging LLVMOrcJIT LLVMOrcShared LLVMOrcTargetProcess LLVMPasses
        LLVMProfileData LLVMRemarks LLVMRuntimeDyld LLVMScalarOpts LLVMSelectionDAG LLVMSupport
        LLVMSymbolize LLVMTableGen LLVMTableGenCommon LLVMTableGenGlobalISel LLVMTarget
        LLVMTargetParser LLVMTextAPI LLVMTextAPIBinaryReader LLVMTransformUtils LLVMVectorize
        LLVMWindowsDriver LLVMWindowsManifest LLVMX86AsmParser LLVMX86CodeGen LLVMX86Desc
        LLVMX86Disassembler LLVMX86Info LLVMX86TargetMCA LLVMXRay LTO Remarks
    )
endif()

# ---------------------------------------------------------------------------
# Post-Build Events: Runtime Dependencies & Standard Library Deployment
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stdlib")
    add_custom_command(TARGET TzdTools POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/stdlib"
            "$<TARGET_FILE_DIR:TzdTools>/stdlib"
        COMMENT "Deploying TzdLang standard library..."
    )
endif()

# Optionally copy NVRTC and NVRTC-builtins DLLs if present
if(HAVE_CUDA)
    file(GLOB NVRTC_DLLS "${CUDA_ROOT_DIR}/bin/nvrtc*.dll")
    if(NVRTC_DLLS)
        add_custom_command(TARGET TzdTools POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${NVRTC_DLLS}
                "$<TARGET_FILE_DIR:TzdTools>/"
            COMMENT "Copying NVRTC and NVRTC builtins runtime DLLs..."
        )
    endif()
endif()

# Optionally copy LibTorch DLLs if present
if(HAVE_LIBTORCH)
    file(GLOB TORCH_DLLS "${LIBTORCH_ROOT}/lib/*.dll")
    if(TORCH_DLLS)
        add_custom_command(TARGET TzdTools POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${TORCH_DLLS}
                "$<TARGET_FILE_DIR:TzdTools>/"
            COMMENT "Copying LibTorch runtime DLLs..."
        )
    endif()
endif()