#include "Res/TzdStrings.h"
#include "TzdFuncScanner.h"

#include <iostream>
#include <iomanip>
#include <Psapi.h>
#include <algorithm>
#include <chrono>
#include <sstream>

#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <thread>
#include <atlbase.h>

std::vector<ScanResult> TzdFuncScanner::g_GuiResults;
bool TzdFuncScanner::g_GuiRunning = false;

// 辅助函数：二分查找最近的符号
// 返回指向 Entry 的指针，如果没有找到（即地址小于所有符号）返回 nullptr
static const ExportEntry* FindNearestSymbol(const std::vector<ExportEntry>& symbols, uintptr_t address) {
    if (symbols.empty()) return nullptr;

    ExportEntry target;
    target.va = address;

    // upper_bound 返回第一个 > address 的元素
    auto it = std::upper_bound(symbols.begin(), symbols.end(), target,
        [](const ExportEntry& a, const ExportEntry& b) {
            return a.va < b.va;
        });

    // 如果是 begin()，说明所有符号都比 address 大，找不到前一个
    if (it == symbols.begin()) {
        return nullptr;
    }

    // 返回前一个（即 <= address 的最大那个）
    return &(*std::prev(it));
}

// 1. 加载 PDB 符号 (仅填充 PDB 列表)
void TzdFuncScanner::loadPdbSymbols(HANDLE hProcess, uintptr_t modBase, const std::string& pdbPath, std::vector<ExportEntry>& pdbList) {
    if (pdbPath.empty()) return;

    std::vector<uint32_t> sectionRvas;
    IMAGE_DOS_HEADER dosHeader;
    if (ReadProcessMemory(hProcess, (LPCVOID)modBase, &dosHeader, sizeof(dosHeader), NULL)) {
        IMAGE_NT_HEADERS ntHeaders;
        if (ReadProcessMemory(hProcess, (LPCVOID)(modBase + dosHeader.e_lfanew), &ntHeaders, sizeof(ntHeaders), NULL)) {
            uintptr_t sectionHdrAddr = modBase + dosHeader.e_lfanew
                + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + ntHeaders.FileHeader.SizeOfOptionalHeader;

            for (int i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++) {
                IMAGE_SECTION_HEADER secHdr;
                if (ReadProcessMemory(hProcess, (LPCVOID)(sectionHdrAddr + i * sizeof(IMAGE_SECTION_HEADER)), &secHdr, sizeof(secHdr), NULL)) {
                    sectionRvas.push_back(secHdr.VirtualAddress);
                }
                else {
                    sectionRvas.push_back(0);
                }
            }
        }
    }
    if (sectionRvas.empty()) sectionRvas.push_back(0x1000);

    PdbReader reader(pdbPath);
    if (!reader.IsValid()) return;

    std::vector<PdbSymbol> pdbSyms;
    if (reader.ParsePublicSymbols(sectionRvas, pdbSyms)) {
        for (const auto& sym : pdbSyms) {
            if (!sym.name.empty()) {
                ExportEntry entry;
                entry.name = sym.name;
                entry.va = modBase + sym.rva;
                entry.isPdb = true;
                pdbList.push_back(entry);
            }
        }
    }
}

// 2. 加载导出表 (仅填充 Export 列表)
std::vector<ExportEntry> TzdFuncScanner::getModuleExports(HANDLE hProcess, HMODULE hMod, const std::string& modName) {
    std::vector<ExportEntry> exports;
    uintptr_t base = (uintptr_t)hMod;

    IMAGE_DOS_HEADER dosHeader;
    IMAGE_NT_HEADERS ntHeaders;
    if (!ReadProcessMemory(hProcess, (LPCVOID)base, &dosHeader, sizeof(dosHeader), NULL)) return exports;
    if (!ReadProcessMemory(hProcess, (LPCVOID)(base + dosHeader.e_lfanew), &ntHeaders, sizeof(ntHeaders), NULL)) return exports;

    auto exportDataDir = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (exportDataDir.Size == 0) return exports;

    IMAGE_EXPORT_DIRECTORY exportDir;
    if (!ReadProcessMemory(hProcess, (LPCVOID)(base + exportDataDir.VirtualAddress), &exportDir, sizeof(exportDir), NULL)) return exports;

    std::vector<DWORD> addrTable(exportDir.NumberOfFunctions);
    std::vector<DWORD> nameTable(exportDir.NumberOfNames);
    std::vector<WORD> ordinalTable(exportDir.NumberOfNames);

    ReadProcessMemory(hProcess, (LPCVOID)(base + exportDir.AddressOfFunctions), addrTable.data(), sizeof(DWORD) * exportDir.NumberOfFunctions, NULL);
    ReadProcessMemory(hProcess, (LPCVOID)(base + exportDir.AddressOfNames), nameTable.data(), sizeof(DWORD) * exportDir.NumberOfNames, NULL);
    ReadProcessMemory(hProcess, (LPCVOID)(base + exportDir.AddressOfNameOrdinals), ordinalTable.data(), sizeof(WORD) * exportDir.NumberOfNames, NULL);

    for (DWORD i = 0; i < exportDir.NumberOfNames; i++) {
        char nameBuf[256];
        if (ReadProcessMemory(hProcess, (LPCVOID)(base + nameTable[i]), nameBuf, sizeof(nameBuf), NULL)) {
            ExportEntry entry;
            // 格式要求：模块名.dll!函数名 (或者 模块名!函数名)
            // 这里为了符合通常习惯，用 ! 连接
            entry.name = modName + "!" + nameBuf;
            entry.va = base + addrTable[ordinalTable[i]];
            entry.isPdb = false;
            exports.push_back(entry);
        }
    }
    return exports;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// GUI 渲染循环
void TzdFuncScanner::GuiThreadLoop() {
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, TzdGui::FUNC_CLASS_NAME_W, NULL };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(TzdGui::FUNC_CLASS_NAME_W, TzdGui::FUNC_WINDOW_TITLE_W, WS_OVERLAPPEDWINDOW, 100, 100, 1100, 750, NULL, NULL, wc.hInstance, NULL);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* mainView = nullptr;
    D3D_FEATURE_LEVEL fl;
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &swapChain, &device, &fl, &context);

    ID3D11Texture2D* pBackBuffer;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    device->CreateRenderTargetView(pBackBuffer, NULL, &mainView);
    pBackBuffer->Release();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(TzdGui::FONT_PATH_MSYH, 17.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);
    ImGui::StyleColorsDark();

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    g_GuiRunning = true;

    static std::vector<int> filteredIndices;
    static char search[128] = "";
    static char lastSearch[128] = "";
    static int catIdx = 0;
    const char* cats[] = { TzdGui::COMBO_ALL_RESULTS, TzdGui::COMBO_HAS_PDB };
    bool needsRebuild = true;

    while (g_GuiRunning) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_GuiRunning = false;
        }
        if (!g_GuiRunning) break;

        RECT rc; GetClientRect(hwnd, &rc);
        UINT width = rc.right - rc.left;
        UINT height = rc.bottom - rc.top;

        DXGI_SWAP_CHAIN_DESC sd; swapChain->GetDesc(&sd);
        if (width != sd.BufferDesc.Width || height != sd.BufferDesc.Height) {
            if (mainView) { mainView->Release(); mainView = nullptr; }
            swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* pBackBuffer;
            swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
            device->CreateRenderTargetView(pBackBuffer, NULL, &mainView);
            pBackBuffer->Release();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin(TzdGui::IMGUI_SCANNER_MAIN, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

        ImGui::SetNextItemWidth(250);
        if (ImGui::InputText(TzdGui::IMGUI_SEARCH_FUNC, search, 128)) {
            if (strcmp(search, lastSearch) != 0) {
                needsRebuild = true;
                strcpy_s(lastSearch, search);
            }
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::Combo(TzdGui::IMGUI_FILTER, &catIdx, cats, 2)) { needsRebuild = true; }
        ImGui::SameLine();
        ImGui::TextDisabled(TzdGui::IMGUI_STATS_FMT, g_GuiResults.size(), filteredIndices.size());
        ImGui::Separator();

        if (needsRebuild) {
            filteredIndices.clear();
            std::string sLow = search;
            std::transform(sLow.begin(), sLow.end(), sLow.begin(), ::tolower);

            for (int i = 0; i < (int)g_GuiResults.size(); i++) {
                const auto& res = g_GuiResults[i];

                // 过滤：只看有 PDB 信息的
                if (catIdx == 1 && res.type != ScanResult::Type::PDB_SYM) continue;

                if (!sLow.empty()) {
                    std::string loc = res.displayLocation;
                    std::transform(loc.begin(), loc.end(), loc.begin(), ::tolower);
                    std::string ins = res.instruction;
                    std::transform(ins.begin(), ins.end(), ins.begin(), ::tolower);
                    if (loc.find(sLow) == std::string::npos && ins.find(sLow) == std::string::npos) continue;
                }
                filteredIndices.push_back(i);
            }
            needsRebuild = false;
        }

        if (ImGui::BeginTable(TzdGui::IMGUI_RESULT_TABLE, 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn(TzdGui::IMGUI_COL_ADDRESS, ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn(TzdGui::IMGUI_COL_ASSEMBLY, ImGuiTableColumnFlags_WidthFixed, 220.0f);
            ImGui::TableSetupColumn(TzdGui::IMGUI_COL_MODULE_BASE, ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableSetupColumn(TzdGui::IMGUI_COL_LOC_INFO, ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            ImGuiListClipper clipper;
            clipper.Begin((int)filteredIndices.size());
            while (clipper.Step()) {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    const auto& res = g_GuiResults[filteredIndices[i]];
                    ImGui::TableNextRow();
                    ImGui::PushID(i);

                    // Col 0: Address + Copy
                    ImGui::TableSetColumnIndex(0);
                    char addrBuf[32];
                    sprintf_s(addrBuf, "0x%llX", res.actualAddr);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.4f, 1.0f));

                    if (ImGui::Selectable(addrBuf, false, ImGuiSelectableFlags_SpanAllColumns)) {
                        // === [最终修正的复制格式] ===
                        std::stringstream ss;
                        // Line 1: Address:0x...
                        ss << TzdGui::DETAIL_ADDRESS << std::hex << std::uppercase << res.actualAddr << "\n";

                        // Line 2: Base address:0x...
                        ss << TzdGui::DETAIL_BASE_ADDRESS << res.baseAddr << "\n";

                        // Line 3: asm:xxx
                        ss << TzdGui::DETAIL_ASM << res.instruction << "\n";

                        // Line 4: Symbol: 导出名+偏移 (PEB: PDB名)
                        // displayLocation 已经在扫描阶段按照 "xxx + xxx（PEB：xxx）" 格式化好了
                        ss << TzdGui::DETAIL_SYMBOL << res.displayLocation << "\n";

                        // Line 5: Change of name before conversion:
                        ss << TzdGui::DETAIL_CONV_BEFORE;
                        // 如果有 PDB 的原始名，显示原始名；否则显示 N/A
                        if (!res.rawSymbol.empty()) {
                            ss << res.rawSymbol << TzdGui::DETAIL_PLUS_HEX << std::hex << res.offset;
                        }
                        else {
                            ss << TzdGui::DETAIL_NA_OFFSET << std::hex << res.offset; // 这里的 offset 是相对于导出的偏移
                        }

                        ImGui::SetClipboardText(ss.str().c_str());
                    }
                    ImGui::PopStyleColor();

                    // Col 1
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(res.instruction.c_str());

                    // Col 2
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextDisabled("0x%llX", res.baseAddr);

                    // Col 3
                    ImGui::TableSetColumnIndex(3);
                    ImVec4 typeColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    // 只要有 PEB 也就是 PDB 匹配，就变绿，否则蓝色/灰色
                    if (res.type == ScanResult::Type::PDB_SYM) typeColor = ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
                    else typeColor = ImVec4(0.7f, 0.8f, 1.0f, 1.0f);

                    ImGui::PushStyleColor(ImGuiCol_Text, typeColor);
                    ImGui::TextUnformatted(res.displayLocation.c_str());
                    ImGui::PopStyleColor();

                    if (ImGui::IsItemHovered() && !res.rawSymbol.empty()) {
                        ImGui::BeginTooltip();
                        ImGui::Text(TzdGui::DETAIL_RAW_PDB, res.rawSymbol.c_str());
                        ImGui::EndTooltip();
                    }

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        context->OMSetRenderTargets(1, &mainView, NULL);
        context->ClearRenderTargetView(mainView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (mainView) mainView->Release();
    if (swapChain) swapChain->Release();
    if (device) device->Release();
    if (context) context->Release();
    DestroyWindow(hwnd);
}

// 3. 扫描入口
void TzdFuncScanner::scanProcess(DWORD processId, const std::string& targetModuleName, const std::string& pdbPath, std::vector<ScanResult>* guiResults) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return;

    // 分开存储 Export 和 PDB，避免混淆
    std::map<uintptr_t, std::vector<ExportEntry>> moduleExportsMap;
    std::map<uintptr_t, std::vector<ExportEntry>> modulePdbMap;
    std::vector<uintptr_t> allowedBases;

    HMODULE hMods[1024];
    DWORD cbNeeded;
    std::string mainModuleName = "";

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        char nameBuf[MAX_PATH];
        if (GetModuleBaseNameA(hProcess, hMods[0], nameBuf, sizeof(nameBuf))) {
            mainModuleName = nameBuf;
        }
    }

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            char modName[MAX_PATH];
            if (GetModuleBaseNameA(hProcess, hMods[i], modName, sizeof(modName))) {
                std::string currentMod = modName;
                bool isTarget = targetModuleName.empty() || (_stricmp(currentMod.c_str(), targetModuleName.c_str()) == 0);

                if (isTarget) {
                    uintptr_t base = (uintptr_t)hMods[i];
                    std::vector<ExportEntry> exports;
                    std::vector<ExportEntry> pdbs;

                    // 1. 加载 PDB (如果符合条件)
                    bool shouldLoadPdb = false;
                    if (!pdbPath.empty()) {
                        if (!targetModuleName.empty()) shouldLoadPdb = true;
                        else if (_stricmp(currentMod.c_str(), mainModuleName.c_str()) == 0) shouldLoadPdb = true;
                    }
                    if (shouldLoadPdb) {
                        loadPdbSymbols(hProcess, base, pdbPath, pdbs);
                        // 排序 PDB
                        std::sort(pdbs.begin(), pdbs.end(), [](const ExportEntry& a, const ExportEntry& b) {
                            return a.va < b.va;
                            });
                        modulePdbMap[base] = pdbs;
                    }

                    // 2. 加载 Exports
                    exports = getModuleExports(hProcess, hMods[i], currentMod);
                    // 排序 Exports
                    std::sort(exports.begin(), exports.end(), [](const ExportEntry& a, const ExportEntry& b) {
                        return a.va < b.va;
                        });
                    moduleExportsMap[base] = exports;

                    allowedBases.push_back(base);
                }
            }
        }
    }

    SYSTEM_INFO si; GetSystemInfo(&si);
    MEMORY_BASIC_INFORMATION mbi;
    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;

    while (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            bool shouldScan = targetModuleName.empty();
            if (!shouldScan) {
                for (auto b : allowedBases) {
                    if ((uintptr_t)mbi.AllocationBase == b) { shouldScan = true; break; }
                }
            }
            if (shouldScan) {
                // 将两个 map 都传进去
                scanMemoryRegion(hProcess, mbi, moduleExportsMap, modulePdbMap, guiResults);
            }
        }
        addr += mbi.RegionSize;
        if (addr >= (uintptr_t)si.lpMaximumApplicationAddress) break;
    }

    if (!pdbPath.empty()) SymCleanup(hProcess);
    CloseHandle(hProcess);

    if (guiResults && !guiResults->empty()) {
        this->g_GuiResults = *guiResults;
        if (!this->g_GuiRunning) {
            std::thread t(&TzdFuncScanner::GuiThreadLoop);
            t.detach();
            while (!this->g_GuiRunning) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
        }
    }
}

std::string TzdFuncScanner::getFirstInstruction(const uint8_t* code, size_t codeSize, uintptr_t address) {
    csh handle; cs_insn* insn; std::string result = TzdGui::DETAIL_UNKNOWN;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) == CS_ERR_OK) {
        size_t count = cs_disasm(handle, code, codeSize, address, 1, &insn);
        if (count > 0) {
            std::stringstream ss;
            ss << insn[0].mnemonic << " " << insn[0].op_str;
            result = ss.str();
            cs_free(insn, count);
        }
        cs_close(&handle);
    }
    return result;
}

std::string TzdFuncScanner::getHexBytes(const uint8_t* data, size_t size) {
    std::stringstream ss;
    for (size_t i = 0; i < size; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    }
    return ss.str();
}

void TzdFuncScanner::scanMemoryRegion(
    HANDLE hProcess,
    MEMORY_BASIC_INFORMATION& mbi,
    const std::map<uintptr_t, std::vector<ExportEntry>>& exportMap,
    const std::map<uintptr_t, std::vector<ExportEntry>>& pdbMap,
    std::vector<ScanResult>* guiResults
) {
    std::vector<uint8_t> buffer(mbi.RegionSize);
    SIZE_T read;
    if (!ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &read)) return;

    uintptr_t allocBase = (uintptr_t)mbi.AllocationBase;

    // 获取当前模块的 export 列表和 pdb 列表
    const std::vector<ExportEntry>* currentExports = nullptr;
    const std::vector<ExportEntry>* currentPdbs = nullptr;

    auto itExp = exportMap.find(allocBase);
    if (itExp != exportMap.end()) currentExports = &itExp->second;

    auto itPdb = pdbMap.find(allocBase);
    if (itPdb != pdbMap.end()) currentPdbs = &itPdb->second;

    for (size_t i = 0; i < read - 15; i++) {
        bool isFunc = false;
        uint8_t* p = &buffer[i];

        // 特征码检测
        if (p[0] == 0x48 && p[1] == 0x89 && p[2] == 0x4C) isFunc = true;
        else if (p[0] == 0x48 && p[1] == 0x83 && p[2] == 0xEC) isFunc = true;
        else if (p[0] == 0x55 && p[1] == 0x48 && p[2] == 0x89) isFunc = true;
        else if (p[0] == 0x40 && p[1] == 0x53 && p[2] == 0x48) isFunc = true;
        else if (p[0] == 0x48 && p[1] == 0x8B && p[2] == 0x05) isFunc = true;
        else if (p[0] == 0x40 && p[1] == 0x55 && p[2] == 0x48 && p[3] == 0x83 && p[4] == 0xEC) isFunc = true;
        else if (p[0] == 0x48 && p[1] == 0x89 && p[2] == 0x5C && p[3] == 0x24) isFunc = true;
        else if (p[0] == 0x48 && p[1] == 0x81 && p[2] == 0xEC) isFunc = true;
        else if (p[0] == 0x48 && p[1] == 0x8D && p[2] == 0x05) isFunc = true;
        else if (p[0] == 0x48 && p[1] == 0x83 && p[2] == 0x79 && p[3] == 0x00) isFunc = true;
        else if (p[0] == 0x48 && p[1] == 0x8B && p[2] == 0x05 && buffer[i + 7] == 0x48 && buffer[i + 8] == 0x33 && buffer[i + 9] == 0xC4) isFunc = true;
        else if (p[0] == 0xCC && p[1] == 0x48 && p[2] == 0x83 && p[3] == 0xEC) isFunc = true;
        else if (p[0] == 0x90 && p[1] == 0x48 && p[2] == 0x83 && p[3] == 0xEC) isFunc = true;
        else if (p[0] == 0xE9 && (i == 0 || buffer[i - 1] == 0xCC)) isFunc = true;
        else if (p[0] == 0x55 && p[1] == 0x8B && p[2] == 0xEC) isFunc = true;
        else if (p[0] == 0x8B && p[1] == 0xFF && p[2] == 0x55) isFunc = true;

        if (isFunc) {
            uintptr_t actualAddr = (uintptr_t)mbi.BaseAddress + i;
            std::string asmStmt = getFirstInstruction(&buffer[i], 15, actualAddr);

            ScanResult res;
            res.actualAddr = actualAddr;
            res.instruction = asmStmt;
            res.baseAddr = allocBase;
            res.type = ScanResult::Type::INTERNAL; // 默认类型
            res.rawSymbol = "";
            res.demangledSymbol = "";
            res.offset = 0;

            // 构造显示字符串
            std::stringstream ssLoc;

            // 1. 查找最近的导出函数 (必须有)
            const ExportEntry* bestExport = nullptr;
            if (currentExports) {
                bestExport = FindNearestSymbol(*currentExports, actualAddr);
            }

            if (bestExport) {
                uintptr_t off = actualAddr - bestExport->va;
                // "xxx + xxx" 部分
                ssLoc << bestExport->name << TzdGui::DETAIL_PLUS_HEX << std::hex << off;

                // 这里我们用相对于导出的偏移作为默认 offset，为了复制时显示 N/A + offset
                res.offset = off;
            }
            else {
                // 如果连导出函数都找不到（罕见，除非模块没导出表），显示相对于基址
                ssLoc << "0x" << std::hex << allocBase << TzdGui::DETAIL_PLUS_HEX << (actualAddr - allocBase);
            }

            // 2. 查找最近的 PDB 符号 (可选)
            const ExportEntry* bestPdb = nullptr;
            if (currentPdbs) {
                bestPdb = FindNearestSymbol(*currentPdbs, actualAddr);
            }

            if (bestPdb) {
                uintptr_t offPdb = actualAddr - bestPdb->va;
                // 放宽 PDB 匹配条件：只要在合理范围内 (比如 1MB)
                if (offPdb < 0x100000) {
                    std::string demangled = SymbolDemangler::demangle(bestPdb->name);
                    // "（PEB：xxx）" 部分
                    ssLoc << TzdGui::LOC_PEB_PREFIX << demangled << TzdGui::LOC_PEB_SUFFIX;

                    // 保存原始信息供 tooltip 和复制使用
                    res.rawSymbol = bestPdb->name;
                    res.demangledSymbol = demangled;
                    // 如果找到了 PDB，就认为是 PDB 类型
                    res.type = ScanResult::Type::PDB_SYM;

                    // 注意：虽然找到了 PDB，但复制格式里的 offset 通常是指相对于符号的偏移
                    // 为了满足 "Change of name: N/A + offset" 这里的逻辑
                    // 我们保留 res.offset 为 "相对于导出函数的偏移" 还是 "相对于PDB的偏移"？
                    // 根据你要求的 "Change of name before conversion: xxx + xx"
                    // 这里通常是指 PDB 原始名 + 相对 PDB 的偏移
                    // 所以这里覆盖 offset
                    res.offset = offPdb;
                }
            }
            else {
                res.type = ScanResult::Type::EXPORT;
            }

            res.displayLocation = ssLoc.str();

            if (guiResults) {
                guiResults->push_back(res);
            }
            else {
                printf("0x%012llX | %s\n", actualAddr, res.displayLocation.c_str());
            }
        }
    }
}