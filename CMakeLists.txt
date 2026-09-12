cmake_minimum_required(VERSION 3.20)

# TzdTools & TzdLang Standalone Build
project(TzdTools LANGUAGES C CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force static runtime (/MT or /MTd) matching MSVC vcxproj configuration
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Preprocessor definitions
add_definitions(
    -DWITH_CUDA
    -DCPPCORO_COMPILER_MSVC
    -DFMT_UNICODE=0
    -D_SILENCE_CLANG_COROUTINE_MESSAGE
    -D_CRT_SECURE_NO_WARNINGS
    -D_ITERATOR_DEBUG_LEVEL=0
    -DDC__Feature_Aggr=0
    -DANTLR4CPP_STATIC
    -DNDEBUG
    -D_CONSOLE
    -DUNICODE
    -D_UNICODE
)

# ---------------------------------------------------------------------------
# Dependency Path Auto-Discovery with Graceful Fallbacks
# ---------------------------------------------------------------------------

# 1. CUDA Toolkit
if(DEFINED ENV{CUDA_PATH})
    set(CUDA_ROOT_DIR "$ENV{CUDA_PATH}")
elseif(EXISTS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6")
    set(CUDA_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6")
elseif(EXISTS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5")
    set(CUDA_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5")
elseif(EXISTS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0")
    set(CUDA_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.0")
else()
    set(CUDA_ROOT_DIR "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6")
endif()

# 2. LibTorch
if(DEFINED ENV{LIBTORCH_PATH} AND EXISTS "$ENV{LIBTORCH_PATH}")
    set(LIBTORCH_ROOT "$ENV{LIBTORCH_PATH}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch")
    set(LIBTORCH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch")
else()
    set(LIBTORCH_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch")
endif()

# 3. LLVM SDK
if(DEFINED ENV{LLVM_DIR} AND EXISTS "$ENV{LLVM_DIR}")
    set(LLVM_ROOT_DIR "$ENV{LLVM_DIR}")
elseif(EXISTS "E:/LLVM_SDK")
    set(LLVM_ROOT_DIR "E:/LLVM_SDK")
elseif(EXISTS "C:/Program Files/LLVM")
    set(LLVM_ROOT_DIR "C:/Program Files/LLVM")
else()
    set(LLVM_ROOT_DIR "E:/LLVM_SDK")
endif()

# 4. vcpkg packages (fmt, asmjit, zlib, zstd, antlr4)
if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}")
    set(VCPKG_ROOT_DIR "$ENV{VCPKG_ROOT}")
elseif(EXISTS "E:/vcpkg")
    set(VCPKG_ROOT_DIR "E:/vcpkg")
elseif(EXISTS "C:/vcpkg")
    set(VCPKG_ROOT_DIR "C:/vcpkg")
else()
    set(VCPKG_ROOT_DIR "E:/vcpkg")
endif()

# 5. Fintamath
if(DEFINED ENV{FINTAMATH_DIR} AND EXISTS "$ENV{FINTAMATH_DIR}")
    set(FINTAMATH_ROOT_DIR "$ENV{FINTAMATH_DIR}")
elseif(EXISTS "E:/Fintamath")
    set(FINTAMATH_ROOT_DIR "E:/Fintamath")
else()
    set(FINTAMATH_ROOT_DIR "E:/Fintamath")
endif()

# 6. Java JDK
if(DEFINED ENV{JAVA_HOME} AND EXISTS "$ENV{JAVA_HOME}")
    set(JDK_ROOT_DIR "$ENV{JAVA_HOME}")
elseif(EXISTS "C:/Program Files/Java/jdk-20")
    set(JDK_ROOT_DIR "C:/Program Files/Java/jdk-20")
elseif(EXISTS "C:/Program Files/Java/jdk-17")
    set(JDK_ROOT_DIR "C:/Program Files/Java/jdk-17")
elseif(EXISTS "C:/Program Files/Java/jdk-21")
    set(JDK_ROOT_DIR "C:/Program Files/Java/jdk-21")
else()
    set(JDK_ROOT_DIR "C:/Program Files/Java/jdk-20")
endif()

message(STATUS "TzdTools Build Configuration:")
message(STATUS "  CUDA Root:     ${CUDA_ROOT_DIR}")
message(STATUS "  LibTorch:      ${LIBTORCH_ROOT}")
message(STATUS "  LLVM SDK:      ${LLVM_ROOT_DIR}")
message(STATUS "  vcpkg Root:    ${VCPKG_ROOT_DIR}")
message(STATUS "  Fintamath:     ${FINTAMATH_ROOT_DIR}")
message(STATUS "  Java JDK:      ${JDK_ROOT_DIR}")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall
    ${CMAKE_CURRENT_SOURCE_DIR}/Plots
    "${LIBTORCH_ROOT}/include"
    "${LIBTORCH_ROOT}/include/torch/csrc/api/include"
    "${CUDA_ROOT_DIR}/include"
    "${LLVM_ROOT_DIR}/include"
    "C:/Program Files/LLVM/include"
    "${VCPKG_ROOT_DIR}/installed/x64-windows-static/include"
    "${VCPKG_ROOT_DIR}/buildtrees/antlr4/src/4.13.2-b64128cc53.clean/runtime/Cpp/runtime/src"
    "${FINTAMATH_ROOT_DIR}/include"
    "${FINTAMATH_ROOT_DIR}/f/include"
    "${JDK_ROOT_DIR}/include"
    "${JDK_ROOT_DIR}/include/win32"
)

# Library search directories
link_directories(
    "${LIBTORCH_ROOT}/lib"
    "${CUDA_ROOT_DIR}/lib/x64"
    "${FINTAMATH_ROOT_DIR}/f/build/lib/Release"
    "${VCPKG_ROOT_DIR}/installed/x64-windows-static/lib"
    "${LLVM_ROOT_DIR}/lib"
)

# ---------------------------------------------------------------------------
# Explicit Source List (prevents accidental compilation of benchmark/test files)
# ---------------------------------------------------------------------------
set(TZD_SOURCES
    # Core CLI & Runtime
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdTools.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdDebugger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdStackTrace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdCommandSystem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdFuncScanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TzdMemoryAsm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PdbReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SymbolDemangler.cpp

    # Generated Interpreter, VM, JIT, PyTorch, Parser
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
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangLexer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangVisitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Generated/TzdLangBaseVisitor.cpp

    # dyncall (C FFI + MASM ASM)
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_callvm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_callvm_base.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_vector.c
    ${CMAKE_CURRENT_SOURCE_DIR}/dyncall/dyncall_call_x64_generic_masm.asm

    # pbPlots plotting library
    ${CMAKE_CURRENT_SOURCE_DIR}/Plots/pbPlots.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Plots/supportLib.cpp
)

add_executable(TzdTools ${TZD_SOURCES})

# ---------------------------------------------------------------------------
# Compiler & Linker Flags
# ---------------------------------------------------------------------------
if(MSVC)
    # Apply MSVC C/C++ compiler options only to C and CXX (NOT to MASM assembler)
    target_compile_options(TzdTools PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX>:
            /FS
            /bigobj
            /utf-8
            /arch:AVX2
            /openmp
            /wd4146 /wd4244 /wd4267 /wd4624 /wd4005 /wd4819 /wd4996
        >
    )

    # Linker options: 256MB stack, ignore dynamic MSVCRT, whole archive for LLVM JIT symbols
    target_link_options(TzdTools PRIVATE
        "/STACK:268435456,268435456"
        "/NODEFAULTLIB:MSVCRT"
        "/WHOLEARCHIVE:LLVMX86CodeGen.lib"
        "/WHOLEARCHIVE:LLVMX86Desc.lib"
        "/WHOLEARCHIVE:LLVMX86Info.lib"
        "/WHOLEARCHIVE:LLVMX86AsmParser.lib"
        "/WHOLEARCHIVE:LLVMOrcJIT.lib"
        "/WHOLEARCHIVE:LLVMExecutionEngine.lib"
    )
endif()

# ---------------------------------------------------------------------------
# Link Libraries
# ---------------------------------------------------------------------------
target_link_libraries(TzdTools PRIVATE
    # Windows System & Base Libraries
    ws2_32.lib
    IPHLPAPI.lib
    dbghelp.lib
    d3d11.lib
    dxgi.lib
    zlib.lib
    zstd.lib
    fmt.lib
    asmjit.lib
    imgui.lib
    minhook.x64.lib

    # CUDA Driver, Runtime, NVRTC, cuFFT
    cudart.lib
    cufft.lib
    cuda.lib
    nvrtc.lib

    # LibTorch & Deep Learning Dependencies
    torch.lib
    torch_cpu.lib
    torch_cuda.lib
    c10.lib
    c10_cuda.lib
    fbgemm.lib
    cpuinfo.lib
    dnnl.lib
    XNNPACK.lib
    pthreadpool.lib

    # Fintamath
    fintamath.lib

    # Clang Libraries
    clang-repl.lib clangAnalysis.lib clangAnalysisFlowSensitive.lib clangAnalysisFlowSensitiveModels.lib
    clangAPINotes.lib clangAST.lib clangASTMatchers.lib clangBasic.lib clangCodeGen.lib clangCrossTU.lib
    clangDependencyScanning.lib clangDirectoryWatcher.lib clangDriver.lib clangDynamicASTMatchers.lib
    clangEdit.lib clangExtractAPI.lib clangFormat.lib clangFrontend.lib clangFrontendTool.lib clangHandleCXX.lib
    clangHandleLLVM.lib clangIndex.lib clangIndexSerialization.lib clangInterpreter.lib clangLex.lib
    clangParse.lib clangRewrite.lib clangRewriteFrontend.lib clangSema.lib clangSerialization.lib
    clangStaticAnalyzerCheckers.lib clangStaticAnalyzerCore.lib clangStaticAnalyzerFrontend.lib clangSupport.lib
    clangTooling.lib clangToolingASTDiff.lib clangToolingCore.lib clangToolingInclusions.lib
    clangToolingInclusionsStdlib.lib clangToolingRefactoring.lib clangToolingSyntax.lib clangTransformer.lib libclang.lib

    # LLD Linker Libraries
    lldCOFF.lib lldCommon.lib lldELF.lib lldMachO.lib lldMinGW.lib lldWasm.lib

    # LLVM Libraries
    LLVMAggressiveInstCombine.lib LLVMAnalysis.lib LLVMAsmParser.lib LLVMAsmPrinter.lib LLVMBinaryFormat.lib
    LLVMBitReader.lib LLVMBitstreamReader.lib LLVMBitWriter.lib LLVMCFGuard.lib LLVMCFIVerify.lib LLVMCodeGen.lib
    LLVMCodeGenTypes.lib LLVMCore.lib LLVMCoroutines.lib LLVMCoverage.lib LLVMDebugInfoBTF.lib LLVMDebugInfoCodeView.lib
    LLVMDebuginfod.lib LLVMDebugInfoDWARF.lib LLVMDebugInfoGSYM.lib LLVMDebugInfoLogicalView.lib LLVMDebugInfoMSF.lib
    LLVMDebugInfoPDB.lib LLVMDemangle.lib LLVMDiff.lib LLVMDlltoolDriver.lib LLVMDWARFLinker.lib
    LLVMDWARFLinkerClassic.lib LLVMDWARFLinkerParallel.lib LLVMDWP.lib LLVMExecutionEngine.lib LLVMExegesis.lib
    LLVMExegesisX86.lib LLVMExtensions.lib LLVMFileCheck.lib LLVMFrontendDriver.lib LLVMFrontendHLSL.lib
    LLVMFrontendOffloading.lib LLVMFrontendOpenACC.lib LLVMFrontendOpenMP.lib LLVMFuzzerCLI.lib LLVMFuzzMutate.lib
    LLVMGlobalISel.lib LLVMHipStdPar.lib LLVMInstCombine.lib LLVMInstrumentation.lib LLVMInterfaceStub.lib
    LLVMInterpreter.lib LLVMipo.lib LLVMIRPrinter.lib LLVMIRReader.lib LLVMJITLink.lib LLVMLibDriver.lib
    LLVMLineEditor.lib LLVMLinker.lib LLVMLTO.lib LLVMMC.lib LLVMMCA.lib LLVMMCDisassembler.lib LLVMMCJIT.lib
    LLVMMCParser.lib LLVMMIRParser.lib LLVMObjCARCOpts.lib LLVMObjCopy.lib LLVMObject.lib LLVMObjectYAML.lib
    LLVMOption.lib LLVMOrcDebugging.lib LLVMOrcJIT.lib LLVMOrcShared.lib LLVMOrcTargetProcess.lib LLVMPasses.lib
    LLVMProfileData.lib LLVMRemarks.lib LLVMRuntimeDyld.lib LLVMScalarOpts.lib LLVMSelectionDAG.lib LLVMSupport.lib
    LLVMSymbolize.lib LLVMTableGen.lib LLVMTableGenCommon.lib LLVMTableGenGlobalISel.lib LLVMTarget.lib
    LLVMTargetParser.lib LLVMTextAPI.lib LLVMTextAPIBinaryReader.lib LLVMTransformUtils.lib LLVMVectorize.lib
    LLVMWindowsDriver.lib LLVMWindowsManifest.lib LLVMX86AsmParser.lib LLVMX86CodeGen.lib LLVMX86Desc.lib
    LLVMX86Disassembler.lib LLVMX86Info.lib LLVMX86TargetMCA.lib LLVMXRay.lib LTO.lib Remarks.lib
)

# ---------------------------------------------------------------------------
# Post-Build Event: Copy Runtime Dependencies & Standard Library
# ---------------------------------------------------------------------------
add_custom_command(TARGET TzdTools POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/stdlib"
        "$<TARGET_FILE_DIR:TzdTools>/stdlib"
    COMMENT "Deploying TzdLang standard library..."
)

# Optionally copy NVRTC DLL if present
file(GLOB NVRTC_DLLS "${CUDA_ROOT_DIR}/bin/nvrtc64_*.dll")
if(NVRTC_DLLS)
    list(GET NVRTC_DLLS 0 NVRTC_FIRST_DLL)
    add_custom_command(TARGET TzdTools POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NVRTC_FIRST_DLL}"
            "$<TARGET_FILE_DIR:TzdTools>/"
        COMMENT "Copying NVRTC runtime DLL..."
    )
endif()

# Copy LibTorch DLLs if present
file(GLOB TORCH_DLLS "${LIBTORCH_ROOT}/lib/*.dll")
if(TORCH_DLLS)
    add_custom_command(TARGET TzdTools POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TORCH_DLLS}
            "$<TARGET_FILE_DIR:TzdTools>/"
        COMMENT "Copying LibTorch runtime DLLs..."
    )
endif()