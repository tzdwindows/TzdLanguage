#include "SymbolDemangler.h"
#include <windows.h>
#include <dbghelp.h>
#include <vector>

// 指示链接器链接 Dbghelp.lib
#pragma comment(lib, "Dbghelp.lib")

std::string SymbolDemangler::demangle(const std::string& mangledName) {
    // 1. 基础检查：MSVC 修饰名通常以 '?' 开头
    if (mangledName.empty() || mangledName[0] != '?') {
        return mangledName;
    }

    // 2. 准备缓冲区
    // 大多数符号不会超过 4KB，但为了安全可以使用动态大小或足够大的栈空间
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    // 3. 调用 Windows API 进行反修饰
    // UNDNAME_COMPLETE:以此获取最完整的函数签名（包括返回类型、参数类型等）
    // UNDNAME_NAME_ONLY: 如果只想获取函数名，可以使用这个标志
    DWORD result = UnDecorateSymbolName(
        mangledName.c_str(),    // 修饰名
        buffer,                 // 输出缓冲区
        sizeof(buffer),         // 缓冲区大小
        UNDNAME_COMPLETE        // 标志位：输出完整声明
    );

    // 4. 检查结果
    if (result == 0) {
        // 如果返回 0，表示失败（可能不是有效的修饰名），根据要求返回原始内容
        return mangledName;
    }

    return std::string(buffer);
}