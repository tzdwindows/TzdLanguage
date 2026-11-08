#include "Res/TzdStrings.h"
#include "TzdMemoryAsm.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <thread>
#include <string>
#include <algorithm>

// ImGui 核心
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
#include <io.h>

// 链接 D3D 库
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// 数据结构
struct TzdAsmLine {
    uint64_t address;
    std::string mnemonic;
    std::string op_str;
    std::string hex_bytes; // 预存 Hex 字符串方便显示
};

// 全局状态
std::vector<TzdAsmLine> g_AsmResults;
bool g_GuiRunning = false;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// --- Win32 窗口回调 ---
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// --- GUI 渲染线程函数 ---
void GuiThreadLoop() {
    // 1. 创建 Win32 窗口
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, TzdGui::MEM_CLASS_NAME_W, NULL };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(TzdGui::MEM_CLASS_NAME_W, TzdGui::MEM_WINDOW_TITLE_W, WS_OVERLAPPEDWINDOW, 100, 100, 1000, 700, NULL, NULL, wc.hInstance, NULL);

    // 2. 初始化 DirectX 11
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    D3D_FEATURE_LEVEL featureLevel;
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();

    // 3. 初始化 ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(TzdGui::FONT_PATH_MSYH, 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());

    if (font == NULL) {
        io.Fonts->AddFontFromFileTTF(TzdGui::FONT_PATH_SIMSUN, 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
    }
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    ImGui::StyleColorsDark();

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    g_GuiRunning = true;

    // 消息循环
    while (g_GuiRunning) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_GuiRunning = false;
        }

        // 渲染帧
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // --- GUI 内容展示 ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin(TzdGui::IMGUI_ANALYZER, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

        static char search[128] = "";
        ImGui::InputText(TzdGui::IMGUI_SEARCH_MEM, search, 128);
        ImGui::Separator();

        if (ImGui::BeginTable(TzdGui::IMGUI_ASM_TABLE, 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
            ImGui::TableSetupColumn(TzdGui::IMGUI_COL_ADDRESS, ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableSetupColumn(TzdGui::IMGUI_COL_HEX, ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn(TzdGui::IMGUI_COL_ASM_INSTRUCTIONS);
            ImGui::TableHeadersRow();

            for (auto& line : g_AsmResults) {
                // 简单的过滤逻辑
                if (strlen(search) > 0 && line.mnemonic.find(search) == std::string::npos &&
                    line.op_str.find(search) == std::string::npos) continue;

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "0x%012llX", line.address);

                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("%s", line.hex_bytes.c_str());

                ImGui::TableSetColumnIndex(2);
                // 高亮逻辑
                ImVec4 cmdColor = ImVec4(1, 1, 1, 1);
                if (line.mnemonic == "call") cmdColor = ImVec4(1, 0.4f, 0.4f, 1); // 红色高亮 call
                if (line.mnemonic[0] == 'j') cmdColor = ImVec4(0.4f, 1, 0.4f, 1); // 绿色高亮跳转

                ImGui::TextColored(cmdColor, "%-8s %s", line.mnemonic.c_str(), line.op_str.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::End();

        // D3D 渲染绘制
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0, 0, 0, 1 };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // 清理
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    DestroyWindow(hwnd);
}

void TzdMemoryAsm::analyzeAndDumpAsm(DWORD processId, size_t maxInstructions) {
    g_AsmResults.clear(); // 每次分析前清空

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    MEMORY_BASIC_INFORMATION mbi;
    LPVOID addr = si.lpMinimumApplicationAddress;

    while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT && (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE))) {
            SIZE_T bytesRead = 0;
            size_t sampleSize = 512; // 增加采样量以填充 GUI
            std::vector<uint8_t> buffer(sampleSize);

            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), sampleSize, &bytesRead)) {
                disassembleBuffer(hProcess, buffer.data(), (size_t)bytesRead, (uintptr_t)mbi.BaseAddress);
            }
        }
        addr = (LPVOID)((BYTE*)mbi.BaseAddress + mbi.RegionSize);
        if (addr >= si.lpMaximumApplicationAddress) break;
    }
    CloseHandle(hProcess);

    // 分析完成后，如果 GUI 没在跑，就开一个新线程弹出来
    if (!g_GuiRunning) {
        std::thread t(GuiThreadLoop);
        t.detach();
    }
}

void TzdMemoryAsm::disassembleBuffer(HANDLE hProcess, const uint8_t* code, size_t codeSize, uint64_t address) {
    csh handle;
    cs_insn* insn;
    BOOL isWow64 = FALSE;
    IsWow64Process(hProcess, &isWow64);
    cs_mode mode = isWow64 ? CS_MODE_32 : CS_MODE_64;

    if (cs_open(CS_ARCH_X86, mode, &handle) != CS_ERR_OK) return;

    // 解析所有指令
    size_t count = cs_disasm(handle, code, codeSize, address, 0, &insn);
    if (count > 0) {
        for (size_t j = 0; j < count; j++) {
            TzdAsmLine line;
            line.address = insn[j].address;
            line.mnemonic = insn[j].mnemonic;
            line.op_str = insn[j].op_str;

            // 转换 Hex 机器码
            char hex[64] = { 0 };
            for (int k = 0; k < insn[j].size; k++) {
                char tmp[4];
                sprintf_s(tmp, sizeof(tmp), "%02X ", insn[j].bytes[k]);
                strcat_s(hex, sizeof(hex), tmp);
            }
            line.hex_bytes = hex;

            g_AsmResults.push_back(line);
        }
        cs_free(insn, count);
    }
    cs_close(&handle);
}