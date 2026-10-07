#ifndef TZD_STACK_TRACE_H
#define TZD_STACK_TRACE_H

#include <windows.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <iostream>
#include <vector>
#include <string>

// 需要链接 DbgHelp.lib
#pragma comment(lib, "dbghelp.lib")

class TzdStackTrace {
public:
    // 获取指定 PID 进程的所有线程调用栈
    static void dumpProcessStack(DWORD processId);

    static DWORD getPidByName(const std::string& processName);
private:
    // 打印单个线程的调用栈
    static void dumpThreadStack(HANDLE hProcess, DWORD threadId);
};

#endif