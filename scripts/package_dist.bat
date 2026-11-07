@echo off
setlocal EnableDelayedExpansion

rem =========================================================================
rem   TzdLanguage Release Distribution Packager
rem   Organizes output artifacts into clean structured directories:
rem     dist/bin, dist/include, dist/lib, dist/licenses, dist/libs
rem =========================================================================

set "SOLUTION_DIR=%~1"
if "%SOLUTION_DIR%"=="" set "SOLUTION_DIR=%~dp0..\"
if "%SOLUTION_DIR:~-1%"=="\" set "SOLUTION_DIR=%SOLUTION_DIR:~0,-1%"

set "OUT_DIR=%~2"
if "%OUT_DIR%"=="" set "OUT_DIR=%SOLUTION_DIR%\x64\Release\"
if "%OUT_DIR:~-1%"=="\" set "OUT_DIR=%OUT_DIR:~0,-1%"

set "DIST_DIR=%SOLUTION_DIR%\dist"

echo [PackageDist] Solution directory: %SOLUTION_DIR%
echo [PackageDist] Build output:       %OUT_DIR%
echo [PackageDist] Target dist layout: %DIST_DIR%

rem 1. Ensure target directory tree exists
if not exist "%DIST_DIR%\bin" mkdir "%DIST_DIR%\bin"
if not exist "%DIST_DIR%\include\tzd" mkdir "%DIST_DIR%\include\tzd"
if not exist "%DIST_DIR%\lib" mkdir "%DIST_DIR%\lib"
if not exist "%DIST_DIR%\licenses" mkdir "%DIST_DIR%\licenses"
if not exist "%DIST_DIR%\libs\java" mkdir "%DIST_DIR%\libs\java"

rem 2. Populate bin/ directory (Executables, DLLs, Standard Library)
echo [PackageDist] (1/5) Populating bin/...
if exist "%OUT_DIR%\TzdTools.exe" copy "%OUT_DIR%\TzdTools.exe" "%DIST_DIR%\bin\" /Y >nul
if exist "%OUT_DIR%\tzd.dll" copy "%OUT_DIR%\tzd.dll" "%DIST_DIR%\bin\" /Y >nul
if exist "%SOLUTION_DIR%\stdlib" robocopy "%SOLUTION_DIR%\stdlib" "%DIST_DIR%\bin\stdlib" /XO /E /NDL /NFL /NJH /NJS >nul
if exist "%SOLUTION_DIR%\External\libtorch\lib" robocopy "%SOLUTION_DIR%\External\libtorch\lib" "%DIST_DIR%\bin" *.dll /XO /NDL /NFL /NJH /NJS >nul

rem Copy CUDA runtime dlls if present
if exist "%OUT_DIR%\nvrtc64_120_0.dll" copy "%OUT_DIR%\nvrtc64_120_0.dll" "%DIST_DIR%\bin\" /Y >nul
if exist "%OUT_DIR%\nvrtc-builtins64_126.dll" copy "%OUT_DIR%\nvrtc-builtins64_126.dll" "%DIST_DIR%\bin\" /Y >nul
if not exist "%DIST_DIR%\bin\nvrtc64_120_0.dll" if exist "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin\nvrtc64_120_0.dll" copy "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin\nvrtc64_120_0.dll" "%DIST_DIR%\bin\" /Y >nul
if not exist "%DIST_DIR%\bin\nvrtc-builtins64_126.dll" if exist "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin\nvrtc-builtins64_126.dll" copy "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6\bin\nvrtc-builtins64_126.dll" "%DIST_DIR%\bin\" /Y >nul

rem 3. Populate include/ directory (Public C/C++ API, JVMTI hooks, TNI)
echo [PackageDist] (2/5) Populating include/...
if exist "%SOLUTION_DIR%\include\tzd" robocopy "%SOLUTION_DIR%\include\tzd" "%DIST_DIR%\include\tzd" /XO /E /NDL /NFL /NJH /NJS >nul
if exist "%SOLUTION_DIR%\include\tzd.h" copy "%SOLUTION_DIR%\include\tzd.h" "%DIST_DIR%\include\" /Y >nul
if exist "%SOLUTION_DIR%\include\tzd.hpp" copy "%SOLUTION_DIR%\include\tzd.hpp" "%DIST_DIR%\include\" /Y >nul

rem 4. Populate lib/ directory (Static import libraries)
echo [PackageDist] (3/5) Populating lib/...
if exist "%OUT_DIR%\tzd.lib" copy "%OUT_DIR%\tzd.lib" "%DIST_DIR%\lib\" /Y >nul
if exist "%OUT_DIR%\TzdTools.lib" copy "%OUT_DIR%\TzdTools.lib" "%DIST_DIR%\lib\" /Y >nul

rem 5. Populate licenses/ directory (License texts)
echo [PackageDist] (4/5) Populating licenses/...
if exist "%SOLUTION_DIR%\LICENSE" copy "%SOLUTION_DIR%\LICENSE" "%DIST_DIR%\licenses\LICENSE.txt" /Y >nul
if exist "%SOLUTION_DIR%\licenses" robocopy "%SOLUTION_DIR%\licenses" "%DIST_DIR%\licenses" /XO /E /NDL /NFL /NJH /NJS >nul

rem 6. Populate libs/ directory (Java bindings & external dependency modules)
echo [PackageDist] (5/5) Populating libs/...
if exist "%SOLUTION_DIR%\java\bin" robocopy "%SOLUTION_DIR%\java\bin" "%DIST_DIR%\libs\java" /XO /E /NDL /NFL /NJH /NJS >nul
if exist "%SOLUTION_DIR%\java\src" robocopy "%SOLUTION_DIR%\java\src" "%DIST_DIR%\libs\java\src" /XO /E /NDL /NFL /NJH /NJS >nul

echo =========================================================================
echo [PackageDist] SUCCESS: Distribution package organized successfully at:
echo               %DIST_DIR%
echo =========================================================================
exit /b 0
