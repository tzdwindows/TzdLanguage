cmake_minimum_required(VERSION 3.20)
project(TzdTools C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 宏定义（直接对应 vcxproj）
add_definitions(
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
        "E:/LLVM_SDK/include"
        "C:/Program Files/LLVM/include"
        "E:/vcpkg/installed/x64-windows-static/include"
        "E:/vcpkg/buildtrees/antlr4/src/4.13.2-b64128cc53.clean/runtime/Cpp/runtime/src"
        "E:/Fintamath/include"
        "E:/Fintamath/f/include"
        "C:/Program Files/Java/jdk-20/include"
        "C:/Program Files/Java/jdk-20/include/win32"
)

# 自动收集当前目录及子目录下的源码
file(GLOB_RECURSE SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_executable(TzdTools ${SOURCES}
        Generated/TzdTieringEngine.cpp)