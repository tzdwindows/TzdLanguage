cmake_minimum_required(VERSION 3.20)

# 启用 C、CXX 以及 MSVC 汇编 (MASM)
project(TzdTools LANGUAGES C CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制匹配 vcxproj 中的静态运行时 (/MT /MTd)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# 宏定义（与 Release|x64 严格对齐）
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

# 包含目录
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Generated
        ${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch/include
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6/include"
        "E:/LLVM_SDK/include"
        "C:/Program Files/LLVM/include"
        "E:/vcpkg/installed/x64-windows-static/include"
        "E:/vcpkg/buildtrees/antlr4/src/4.13.2-b64128cc53.clean/runtime/Cpp/runtime/src"
        "E:/Fintamath/include"
        "E:/Fintamath/f/include"
        "C:/Program Files/Java/jdk-20/include"
        "C:/Program Files/Java/jdk-20/include/win32"
)

# 库搜索目录
link_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/External/libtorch/lib"
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6/lib/x64"
        "E:/Fintamath/f/build/lib/Release"
        "E:/vcpkg/installed/x64-windows-static/lib"
        "E:/LLVM_SDK/lib"
)

# 收集源码（包含 C、C++ 和 MASM 汇编）
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.asm"
)

add_executable(TzdTools ${SOURCES})

# 编译参数与屏蔽告警（对应 vcxproj 的 AdditionalOptions 与 DisableSpecificWarnings）
if(MSVC)
    target_compile_options(TzdTools PRIVATE
            /FS
            /bigobj
            /utf-8
            /arch:AVX2
            /openmp
            /wd4146 /wd4244 /wd4267 /wd4624 /wd4005 /wd4819 /wd4996
    )

    # 链接选项：栈大小 256MB、忽略 MSVCRT、指定 WHOLEARCHIVE 强制导出符号
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

# 链接第三方库
target_link_libraries(TzdTools PRIVATE
        # 系统与基础库
        ws2_32.lib
        IPHLPAPI.lib
        zlib.lib
        zstd.lib
        fmt.lib
        asmjit.lib

        # CUDA
        cudart.lib
        cufft.lib
        cuda.lib
        nvrtc.lib

        # LibTorch 相关
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

        # Clang 库群
        clang-repl.lib clangAnalysis.lib clangAnalysisFlowSensitive.lib clangAnalysisFlowSensitiveModels.lib
        clangAPINotes.lib clangAST.lib clangASTMatchers.lib clangBasic.lib clangCodeGen.lib clangCrossTU.lib
        clangDependencyScanning.lib clangDirectoryWatcher.lib clangDriver.lib clangDynamicASTMatchers.lib
        clangEdit.lib clangExtractAPI.lib clangFormat.lib clangFrontend.lib clangFrontendTool.lib clangHandleCXX.lib
        clangHandleLLVM.lib clangIndex.lib clangIndexSerialization.lib clangInterpreter.lib clangLex.lib
        clangParse.lib clangRewrite.lib clangRewriteFrontend.lib clangSema.lib clangSerialization.lib
        clangStaticAnalyzerCheckers.lib clangStaticAnalyzerCore.lib clangStaticAnalyzerFrontend.lib clangSupport.lib
        clangTooling.lib clangToolingASTDiff.lib clangToolingCore.lib clangToolingInclusions.lib
        clangToolingInclusionsStdlib.lib clangToolingRefactoring.lib clangToolingSyntax.lib clangTransformer.lib libclang.lib

        # LLD 库群
        lldCOFF.lib lldCommon.lib lldELF.lib lldMachO.lib lldMinGW.lib lldWasm.lib

        # LLVM 库群
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

# 生成后事件：拷贝依赖 DLL
add_custom_command(TARGET TzdTools POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/stdlib"
        "$<TARGET_FILE_DIR:TzdTools>/stdlib"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.6/bin/nvrtc64_120_0.dll"
        "$<TARGET_FILE_DIR:TzdTools>/"
        COMMENT "Copying runtime dependencies..."
)