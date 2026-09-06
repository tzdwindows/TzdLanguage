#include "Res/TzdStrings.h"

// TzdTools.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <atomic>
#include <cstring>

#include "TzdCommandSystem.h"

#include <MinHook.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

typedef void(__cdecl* TRASH_FREE)(void*);
TRASH_FREE pStaticFree = nullptr;

bool g_InJitCleanup = false;

// 使用 thread_local 防止递归重入
static thread_local bool g_InSafeCheck = false;

// ============================================================
// Free 调用历史记录器 (环形缓冲区)
// ============================================================
static constexpr int MAX_FREE_RECORDS = 64;
static constexpr int MAX_STACK_FRAMES = 32;

struct FreeRecord {
    void* ptr;
    USHORT frameCount;
    void* frames[MAX_STACK_FRAMES];
};

static FreeRecord g_freeRecords[MAX_FREE_RECORDS];
static std::atomic<int> g_freeRecordIdx{0};
static std::atomic<bool> g_crashPrinted{false};

// 辅助函数：将 SEH 逻辑隔离，避免 C2712 错误
bool IsValidHeapPointer(HANDLE hHeap, void* p) {
    __try {
        if (hHeap != NULL && HeapSize(hHeap, 0, p) != (SIZE_T)-1) {
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
    return false;
}

// 真正的 IsSafePointer 实现
bool IsSafePointer(void* p) {
    if (p == nullptr) return false;
#ifdef _WIN64
    if (((uintptr_t)p & 0xF) != 0) return false;
#else
    if (((uintptr_t)p & 0x7) != 0) return false;
#endif
    if (g_InSafeCheck) return true;
    g_InSafeCheck = true;
    bool isSafe = false;
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof(mbi)) != 0) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Type == MEM_PRIVATE || mbi.Type == MEM_MAPPED))
        {
            HANDLE hProcHeap = GetProcessHeap();
            if (IsValidHeapPointer(hProcHeap, p)) {
                isSafe = true;
            }
            else {
                HANDLE heaps[256];
                DWORD numHeaps = GetProcessHeaps(256, heaps);
                for (DWORD i = 0; i < numHeaps; ++i) {
                    if (heaps[i] == hProcHeap) continue;
                    if (IsValidHeapPointer(heaps[i], p)) {
                        isSafe = true;
                        break;
                    }
                }
            }
        }
    }

    g_InSafeCheck = false;
    return isSafe;
}

// 打印最近 N 条 free 记录 (含调用栈)
static void PrintFreeHistory(const char* reason, void* badPtr) {
    if (g_crashPrinted.exchange(true)) return;

    // 用低级 I/O 避免 std::cerr 在栈溢出时二次崩溃
    FILE* f = nullptr;
    fopen_s(&f, "free_crash_log.txt", "w");
    if (!f) return;

    fprintf(f, "=== %s ===\n", reason);
    fprintf(f, "Problematic pointer: %p\n\n", badPtr);

    int current = g_freeRecordIdx.load(std::memory_order_relaxed);
    fprintf(f, "Last %d free() calls (most recent first):\n", MAX_FREE_RECORDS);
    for (int i = 0; i < MAX_FREE_RECORDS; i++) {
        int idx = ((current - 1 - i) % MAX_FREE_RECORDS + MAX_FREE_RECORDS) % MAX_FREE_RECORDS;
        auto& r = g_freeRecords[idx];
        if (r.ptr == nullptr && r.frameCount == 0) continue;

        fprintf(f, "\n[%d] free(%p)\n", MAX_FREE_RECORDS - i, r.ptr);

        // 解析符号
        for (USHORT f2 = 0; f2 < r.frameCount; f2++) {
            DWORD64 disp = 0;
            char buffer[sizeof(SYMBOL_INFO) + 512];
            SYMBOL_INFO* sym = (SYMBOL_INFO*)buffer;
            sym->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym->MaxNameLen = 511;

            DWORD64 addr = (DWORD64)r.frames[f2];
            if (SymFromAddr(GetCurrentProcess(), addr, &disp, sym)) {
                IMAGEHLP_MODULE64 modInfo;
                memset(&modInfo, 0, sizeof(modInfo));
                modInfo.SizeOfStruct = sizeof(modInfo);
                SymGetModuleInfo64(GetCurrentProcess(), addr, &modInfo);

                fprintf(f, "  %s!%s+0x%llx  [0x%llx]\n",
                    modInfo.ModuleName,
                    sym->Name,
                    (unsigned long long)disp,
                    (unsigned long long)addr);
            } else {
                fprintf(f, "  0x%llx\n", (unsigned long long)addr);
            }
        }
    }

    fprintf(f, "\n=== END OF LOG ===\n");
    fflush(f);
    fclose(f);

    // 同时输出到 stderr
    std::cerr << "\n*** " << reason << " (ptr=" << badPtr << ") ***" << std::endl;
    std::cerr << "See free_crash_log.txt for full free history." << std::endl;
}

// 向量化异常处理：捕获栈溢出/堆损坏，在崩溃前输出记录
static LONG WINAPI CrashVEH(EXCEPTION_POINTERS* ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code == 0xC00000FD /*STATUS_STACK_OVERFLOW*/ ||
        code == 0xC0000374 /*STATUS_HEAP_CORRUPTION*/) {
        PrintFreeHistory("VEH: Stack overflow / Heap corruption", nullptr);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void __cdecl DetourFree(void* p) {
    if (p == nullptr) return;

    // 记录本次 free 调用 (环形缓冲区)
    int idx = g_freeRecordIdx.fetch_add(1, std::memory_order_relaxed) % MAX_FREE_RECORDS;
    g_freeRecords[idx].ptr = p;
    g_freeRecords[idx].frameCount = RtlCaptureStackBackTrace(
        1, MAX_STACK_FRAMES, g_freeRecords[idx].frames, NULL);

    // 始终调用真正的 free — 只记录，不跳过
    // 跳过 free 会破坏 LLVM 内部分配器状态
    __try {
        if (pStaticFree) {
            pStaticFree(p);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        PrintFreeHistory("Exception during free()", p);
    }
}

void InitHook() {
    // 注册向量异常处理器 (在崩溃前输出记录)
    AddVectoredExceptionHandler(0, CrashVEH);

    // 初始化符号服务器 (用于解析调用栈地址到函数名)
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(GetCurrentProcess(), NULL, TRUE);

    // Hook free()
    if (MH_Initialize() != MH_OK) return;
    if (MH_CreateHook(&free, &DetourFree, reinterpret_cast<LPVOID*>(&pStaticFree)) == MH_OK) {
        MH_EnableHook(&free);
    }
}


int main(int argc, char* argv[]) {
    // InitHook();  // Temporarily disabled for testing
    TzdCommandSystem system;
    system.start(argc, argv);
    return 0;
}
