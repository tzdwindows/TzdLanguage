#include "Res/TzdStrings.h"
#include "TzdStackTrace.h"

#include <algorithm>

void TzdStackTrace::dumpProcessStack(DWORD processId) {
    std::printf(TzdStk::ANALYZING, processId); std::cout << std::endl;

    // 1. 获取进程句柄
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cout << TzdStk::OPEN_FAILED << std::endl;
        return;
    }

    // 2. 初始化符号表
    SymInitialize(hProcess, NULL, TRUE);

    // 3. 遍历线程
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap != INVALID_HANDLE_VALUE) {
        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(hThreadSnap, &te32)) {
            do {
                if (te32.th32OwnerProcessID == processId) {
                    std::printf(TzdStk::THREAD_LABEL, te32.th32ThreadID); std::cout << std::endl;
                    dumpThreadStack(hProcess, te32.th32ThreadID);
                }
            } while (Thread32Next(hThreadSnap, &te32));
        }
        CloseHandle(hThreadSnap);
    }

    SymCleanup(hProcess);
    CloseHandle(hProcess);
}


DWORD TzdStackTrace::getPidByName(const std::string& processName) {
    DWORD pid = 0;
    // 获取系统进程快照
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe32)) {
            do {
                // 转换当前的 pe32.szExeFile 为 std::string 方便比较
                // 兼容 Unicode 和 Multi-Byte
#ifdef UNICODE
                char mbs[MAX_PATH];
                size_t convertedChars = 0;
                wcstombs_s(&convertedChars, mbs, MAX_PATH, pe32.szExeFile, _TRUNCATE);
                std::string currentProcessName = mbs;
#else
                std::string currentProcessName = pe32.szExeFile;
#endif

                // 转为小写进行不区分大小写匹配
                std::string searchName = processName;
                std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
                std::transform(currentProcessName.begin(), currentProcessName.end(), currentProcessName.begin(), ::tolower);

                if (currentProcessName == searchName) {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }
    return pid;
}

void TzdStackTrace::dumpThreadStack(HANDLE hProcess, DWORD threadId) {
    HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SUSPEND_RESUME, FALSE, threadId);
    if (!hThread) return;

    // 必须挂起线程才能获取稳定的上下文
    SuspendThread(hThread);

    CONTEXT context;
    memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = CONTEXT_FULL;
    if (GetThreadContext(hThread, &context)) {
        STACKFRAME64 stackFrame;
        memset(&stackFrame, 0, sizeof(STACKFRAME64));

        DWORD machineType;
#ifdef _M_IX86
        machineType = IMAGE_FILE_MACHINE_I386;
        stackFrame.AddrPC.Offset = context.Eip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Ebp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Esp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
#else
        machineType = IMAGE_FILE_MACHINE_AMD64;
        stackFrame.AddrPC.Offset = context.Rip;
        stackFrame.AddrPC.Mode = AddrModeFlat;
        stackFrame.AddrFrame.Offset = context.Rbp;
        stackFrame.AddrFrame.Mode = AddrModeFlat;
        stackFrame.AddrStack.Offset = context.Rsp;
        stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

        while (StackWalk64(machineType, hProcess, hThread, &stackFrame, &context, NULL,
            SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            if (stackFrame.AddrPC.Offset == 0) break;

            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            DWORD64 displacement = 0; // 这里的 displacement 就是偏移量
            if (SymFromAddr(hProcess, stackFrame.AddrPC.Offset, &displacement, pSymbol)) {
                // 关键修改点：输出函数名 + 0x16进制偏移
                std::printf(TzdStk::SYMBOL_FMT, (unsigned long long)stackFrame.AddrPC.Offset, pSymbol->Name, (unsigned long long)displacement); std::cout << std::endl;
            }
            else {
                // 如果找不到符号，至少尝试获取模块名
                IMAGEHLP_MODULE64 moduleInfo;
                moduleInfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
                if (SymGetModuleInfo64(hProcess, stackFrame.AddrPC.Offset, &moduleInfo)) {
                    // 计算相对于模块基址的偏移
                    DWORD64 modOffset = stackFrame.AddrPC.Offset - moduleInfo.BaseOfImage;
                    std::printf(TzdStk::MODULE_FMT, (unsigned long long)stackFrame.AddrPC.Offset, moduleInfo.ModuleName, (unsigned long long)modOffset); std::cout << std::endl;
                }
                else {
                    std::printf(TzdStk::UNKNOWN_FMT, (unsigned long long)stackFrame.AddrPC.Offset); std::cout << std::endl;
                }
            }
        }
    }

    ResumeThread(hThread);
    CloseHandle(hThread);
}