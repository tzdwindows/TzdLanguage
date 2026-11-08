#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include <map>
#include <capstone/capstone.h>
#include <DbgHelp.h>

#include "SymbolDemangler.h" 
#include "PdbReader.h"

#pragma comment(lib, "dbghelp.lib")

struct ExportEntry {
    std::string name;
    uintptr_t va;
    bool isPdb;
};

struct ScanResult {
    uintptr_t actualAddr;       // 实际扫描到的内存地址
    std::string instruction;    // 汇编指令
    uintptr_t baseAddr;         // 模块基址

    // 新增/修改的字段用于特定格式复制
    std::string demangledSymbol; // 反修饰后的人类可读名称
    std::string rawSymbol;       // 原始修饰名 (如 ?Func@...)
    uintptr_t offset;            // 偏移量 (actualAddr - symbolVa)

    // 用于列表显示的简略字符串
    std::string displayLocation;

    enum class Type { PDB_SYM, EXPORT, INTERNAL };
    Type type;
};

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class TzdFuncScanner {
public:
    TzdFuncScanner() = default;

    void scanProcess(DWORD processId,
        const std::string& targetModuleName = "",
        const std::string& pdbPath = "",
        std::vector<ScanResult>* guiResults = nullptr);

    static void GuiThreadLoop();
    static std::vector<ScanResult> g_GuiResults;
    static bool g_GuiRunning;

private:
    void loadPdbSymbols(HANDLE hProcess, uintptr_t modBase, const std::string& pdbPath, std::vector<ExportEntry>& exports);
    std::vector<ExportEntry> getModuleExports(HANDLE hProcess, HMODULE hMod, const std::string& modName);

    void scanMemoryRegion(
        HANDLE hProcess,
        MEMORY_BASIC_INFORMATION& mbi,
        const std::map<uintptr_t, std::vector<ExportEntry>>& exportMap,
        const std::map<uintptr_t, std::vector<ExportEntry>>& pdbMap,
        std::vector<ScanResult>* guiResults
    );

    std::string getFirstInstruction(const uint8_t* code, size_t codeSize, uintptr_t address);
    std::string getHexBytes(const uint8_t* data, size_t size);
};