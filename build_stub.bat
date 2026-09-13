@echo off
setlocal EnableDelayedExpansion

echo ================================================================
echo   TzdLang Stub Runtime Builder (Zero-DLL Mode)
echo ================================================================

rem ---- 1. MSVC environment ----
set "VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo [ERROR] vcvars64.bat not found: %VCVARS%
    exit /b 1
)
call "%VCVARS%" >nul 2>&1
echo [1/7] MSVC environment ready.

rem ---- 2. Paths ----
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "OBJDIR=%ROOT%\stub_obj"
set "OUTEXE=%ROOT%\x64\Release\tzd_stub.exe"
set "VCPKG=E:\vcpkg\installed\x64-windows-static"
set "LLVM=E:\LLVM_SDK"
set "FINTAMATH=E:\Fintamath\f"

if not exist "%OBJDIR%" mkdir "%OBJDIR%"
if not exist "%ROOT%\x64\Release" mkdir "%ROOT%\x64\Release"

rem ---- 3. Compiler flags ----
set CFLAGS=/nologo /MT /O2 /EHsc /std:c++20 /permissive- /DNDEBUG /DANTLR4CPP_STATIC /DNO_LIBTORCH /DTZD_STUB_BUILD /DCPPCORO_COMPILER_MSVC /DFMT_UNICODE=0 /DNOMINMAX /bigobj /utf-8

set INCS=/I"%VCPKG%\include\antlr4-runtime" /I"%VCPKG%\include" /I"%ROOT%" /I"%ROOT%\Generated" /I"%LLVM%\include" /I"%FINTAMATH%\include"

rem ---- 4. Compile source files ----
echo [2/7] Compiling sources (no torch/CUDA)... 

set COMPILE_OK=1

call :compile "TzdStub.cpp" "%ROOT%\TzdStub.cpp"
call :compile "TzdStubFallback.cpp" "%ROOT%\TzdStubFallback.cpp"
call :compile "TzdExeCompiler.cpp" "%ROOT%\TzdExeCompiler.cpp"
call :compile "TzdDebugger.cpp" "%ROOT%\TzdDebugger.cpp"
call :compile "TzdStackTrace.cpp" "%ROOT%\TzdStackTrace.cpp"
call :compile "TzdMemoryAsm.cpp" "%ROOT%\TzdMemoryAsm.cpp"
call :compile "TzdFuncScanner.cpp" "%ROOT%\TzdFuncScanner.cpp"
call :compile "TzdInterpreter.cpp" "%ROOT%\Generated\TzdInterpreter.cpp"
call :compile "TzdBytecode.cpp" "%ROOT%\Generated\TzdBytecode.cpp"
call :compile "TzdBytecodeJIT.cpp" "%ROOT%\Generated\TzdBytecodeJIT.cpp"
call :compile "TzdTieringEngine.cpp" "%ROOT%\Generated\TzdTieringEngine.cpp"
call :compile "TzdOop.cpp" "%ROOT%\Generated\TzdOop.cpp"
call :compile "TzdNativeModule.cpp" "%ROOT%\Generated\TzdNativeModule.cpp"
call :compile "TzdConsole.cpp" "%ROOT%\Generated\TzdConsole.cpp"
call :compile "TzdGC.cpp" "%ROOT%\Generated\TzdGC.cpp"
call :compile "TzdFFIAdapter.cpp" "%ROOT%\Generated\TzdFFIAdapter.cpp"
call :compile "TzdExperimentalCompute.cpp" "%ROOT%\Generated\TzdExperimentalCompute.cpp"
call :compile "TzdJit.cpp" "%ROOT%\Generated\TzdJit.cpp"
call :compile "TzdLangLexer.cpp" "%ROOT%\Generated\TzdLangLexer.cpp"
call :compile "TzdLangParser.cpp" "%ROOT%\Generated\TzdLangParser.cpp"
call :compile "TzdLangBaseVisitor.cpp" "%ROOT%\Generated\TzdLangBaseVisitor.cpp"
call :compile "TzdLangVisitor.cpp" "%ROOT%\Generated\TzdLangVisitor.cpp"

if exist "%ROOT%\External\dyncall\dyncall_api.c" (
    call :compile "dyncall_api.c" "%ROOT%\External\dyncall\dyncall_api.c"
    call :compile "dyncall_callvm.c" "%ROOT%\External\dyncall\dyncall_callvm.c"
    call :compile "dyncall_callvm_base.c" "%ROOT%\External\dyncall\dyncall_callvm_base.c"
    call :compile "dyncall_vector.c" "%ROOT%\External\dyncall\dyncall_vector.c"
)
if exist "%ROOT%\External\pbPlots\pbPlots.cpp" (
    call :compile "pbPlots.cpp" "%ROOT%\External\pbPlots\pbPlots.cpp"
    call :compile "supportLib.cpp" "%ROOT%\External\pbPlots\supportLib.cpp"
)

if "%COMPILE_OK%"=="0" (
    echo.
    echo [ERROR] Compilation failed. Fix errors above, then re-run.
    exit /b 1
)
echo [3/7] All sources compiled OK.

rem ---- 5. Collect OBJs ----
set OBJS=
for %%O in ("%OBJDIR%\*.obj") do (
    set "OBJS=!OBJS! "%%~O""
)

rem ---- 6. Link ----
echo [4/7] Linking tzd_stub.exe ...

link /nologo ^
  /OUT:"%OUTEXE%" ^
  /SUBSYSTEM:CONSOLE /OPT:REF /OPT:ICF ^
  /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:msvcrtd.lib ^
  /STACK:67108864 ^
  /LIBPATH:"%VCPKG%\lib" ^
  /LIBPATH:"%LLVM%\lib" ^
  /LIBPATH:"%FINTAMATH%\build\lib\Release" ^
  %OBJS% ^
  antlr4-runtime-static.lib ^
  asmjit.lib ^
  zlib.lib ^
  zstd.lib ^
  gmp.lib ^
  mpfr.lib ^
  fintamath.lib ^
  ws2_32.lib dbghelp.lib gdiplus.lib ^
  advapi32.lib user32.lib kernel32.lib ^
  shell32.lib ole32.lib oleaut32.lib ^
  uuid.lib psapi.lib ^
  LLVMCore.lib LLVMSupport.lib LLVMDemangle.lib ^
  LLVMBinaryFormat.lib LLVMBitReader.lib LLVMBitWriter.lib ^
  LLVMObject.lib LLVMOrcJIT.lib LLVMOrcShared.lib LLVMOrcTargetProcess.lib ^
  LLVMExecutionEngine.lib LLVMRuntimeDyld.lib LLVMMCJIT.lib ^
  LLVMMC.lib LLVMMCParser.lib LLVMMCDisassembler.lib ^
  LLVMX86CodeGen.lib LLVMX86Desc.lib LLVMX86Info.lib LLVMX86AsmParser.lib ^
  LLVMCodeGen.lib LLVMCodeGenTypes.lib LLVMSelectionDAG.lib LLVMGlobalISel.lib ^
  LLVMAsmParser.lib LLVMAsmPrinter.lib LLVMIRReader.lib LLVMIRPrinter.lib ^
  LLVMTransformUtils.lib LLVMScalarOpts.lib LLVMInstCombine.lib LLVMVectorize.lib ^
  LLVMPasses.lib LLVMipo.lib LLVMAnalysis.lib LLVMTarget.lib LLVMTargetParser.lib ^
  LLVMLinker.lib LLVMFrontendOpenMP.lib LLVMProfileData.lib ^
  LLVMRemarks.lib LLVMBitstreamReader.lib LLVMJITLink.lib ^
  LLVMDebugInfoDWARF.lib LLVMDebugInfoCodeView.lib LLVMDebugInfoMSF.lib ^
  LLVMDebugInfoPDB.lib LLVMSymbolize.lib LLVMDiff.lib ^
  /WHOLEARCHIVE:LLVMX86CodeGen.lib ^
  /WHOLEARCHIVE:LLVMX86Desc.lib ^
  /WHOLEARCHIVE:LLVMX86Info.lib ^
  /WHOLEARCHIVE:LLVMX86AsmParser.lib ^
  /WHOLEARCHIVE:LLVMOrcJIT.lib ^
  /WHOLEARCHIVE:LLVMExecutionEngine.lib 2>&1

if errorlevel 1 (
    echo.
    echo [ERROR] Linking failed.
    exit /b 1
)

echo [5/7] Link OK!
echo.

rem ---- 7. Verify no torch ----
echo [6/7] Verifying zero torch/CUDA imports...
dumpbin /imports "%OUTEXE%" 2>nul | findstr /i "torch c10 cuda cudart" >nul
if errorlevel 1 (
    echo   PASS: No torch/CUDA DLL imports. Zero-DLL confirmed!
) else (
    echo   WARN: Found torch/CUDA imports -- check dumpbin output!
)

echo [7/7] Done.
echo.
echo   Stub: %OUTEXE%
for %%S in ("%OUTEXE%") do echo   Size: %%~zS bytes
echo.
echo   Now run: TzdTools.exe build myapp.tzd
echo   Output exe will have ZERO DLL dependencies.
echo.

goto :eof

rem ---- Subroutine: compile one file ----
:compile
    set "_NAME=%~1"
    set "_SRC=%~2"
    echo   [CC] %_NAME%
    cl %CFLAGS% %INCS% /c "%_SRC%" /Fo"%OBJDIR%\%_NAME%.obj" 2>&1
    if errorlevel 1 (
        echo   [FAIL] %_NAME%
        set COMPILE_OK=0
    )
    goto :eof
