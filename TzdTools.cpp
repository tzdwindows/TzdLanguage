#include "Res/TzdStrings.h"

// TzdTools.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include "TzdCommandSystem.h"

#include <MinHook.h>

typedef void(__cdecl* TRASH_FREE)(void*);
TRASH_FREE pStaticFree = nullptr;

bool g_InJitCleanup = false;

// 使用 thread_local 防止递归重入
static thread_local bool g_InSafeCheck = false;

// 辅助函数：将 SEH 逻辑隔离，避免 C2712 错误
// 这个函数只负责询问系统：这个堆是否认识这个指针
bool IsValidHeapPointer(HANDLE hHeap, void* p) {
    __try {
        // HeapSize 是最安全的探测方式。
        // 如果指针不属于该堆，它返回 (SIZE_T)-1，且通常不会崩溃。
        if (hHeap != NULL && HeapSize(hHeap, 0, p) != (SIZE_T)-1) {
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // 捕获所有访问冲突，确保不崩溃
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

void __cdecl DetourFree(void* p) {
    if (p == nullptr) return;
    if (!IsSafePointer(p)) {
        return;
    }
    __try {
        if (pStaticFree) {
            pStaticFree(p);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

void InitHook() {
    if (MH_Initialize() != MH_OK) return;
    if (MH_CreateHook(&free, &DetourFree, reinterpret_cast<LPVOID*>(&pStaticFree)) == MH_OK) {
        MH_EnableHook(&free);
        //std::cout << TzdSys::MOD_FREE_HOOK << std::endl;
    }
}


int main(int argc, char* argv[]) {
    InitHook();
    Gdiplus::GdiplusStartupInput gsi;
    ULONG_PTR gToken;
    Gdiplus::GdiplusStartup(&gToken, &gsi, NULL);
    TzdCommandSystem system;
    system.start(argc, argv);
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
