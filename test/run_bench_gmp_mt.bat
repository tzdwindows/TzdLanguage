@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > nul
echo Compiling bench_gmp_mt.cpp with MSVC /O2 /openmp...
cl.exe /O2 /openmp /EHsc /std:c++17 /I"E:\vcpkg\installed\x64-windows\include" test\bench_gmp_mt.cpp "E:\vcpkg\installed\x64-windows\lib\gmp.lib" /Fe:test\bench_gmp_mt.exe /Fo:test\
if %errorlevel% neq 0 (
    echo Compilation failed!
    exit /b %errorlevel%
)
set PATH=E:\vcpkg\installed\x64-windows\bin;%PATH%
echo Running bench_gmp_mt.exe...
test\bench_gmp_mt.exe
