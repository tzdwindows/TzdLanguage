#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "TzdInterpreter.h"
#include "TzdUiModule.h"

#include <windows.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <windowsx.h>

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL) return -1;
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return (int)j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

static HFONT g_hDefaultFont = NULL;
static bool g_uiClassRegistered = false;
static ULONG_PTR g_gdiplusToken = 0;

static std::unordered_map<HWND, int> g_uiProgressValues;
static std::unordered_map<HWND, COLORREF> g_uiWindowBgColors;
static std::unordered_map<HWND, HBRUSH> g_uiWindowBrushes;
static std::unordered_map<HWND, bool> g_uiWindowDarkMode;
static std::unordered_map<HWND, Gdiplus::Bitmap*> g_uiImageViews;

enum class UIStyleKind {
    WINDOWS,
    MACOS,
    CUSTOM
};

enum class UIThemeMode {
    LIGHT,
    DARK,
    MONOCHROME
};

struct UIThemeConfig {
    UIStyleKind style = UIStyleKind::WINDOWS;
    UIThemeMode mode = UIThemeMode::LIGHT;

    // Window & text
    COLORREF bg = RGB(243, 243, 243);
    COLORREF fg = RGB(30, 30, 30);
    COLORREF accent = RGB(0, 103, 192);

    // Buttons
    COLORREF buttonBg = RGB(255, 255, 255);
    COLORREF buttonHoverBg = RGB(236, 240, 248);
    COLORREF buttonPressedBg = RGB(218, 224, 236);
    COLORREF buttonFg = RGB(24, 24, 28);
    COLORREF buttonBorder = RGB(205, 210, 218);
    int buttonRadius = 4;

    // Inputs
    COLORREF inputBg = RGB(255, 255, 255);
    COLORREF inputFg = RGB(30, 30, 30);
    COLORREF inputBorder = RGB(215, 215, 220);

    // Header / Titlebar
    bool hasCustomHeader = false;
    bool hasTrafficLights = false;
    int headerHeight = 36;
    COLORREF headerBg = RGB(230, 230, 233);
    COLORREF headerFg = RGB(30, 30, 30);
    COLORREF headerBorder = RGB(210, 210, 215);

    // Font
    std::wstring fontFamily = L"Microsoft YaHei UI";
    int fontSize = 9;
};

struct ButtonAnimState {
    float hoverAlpha = 0.0f;     // 0.0 to 1.0
    float targetHover = 0.0f;    // 0.0 or 1.0
    bool isPressed = false;
    bool timerRunning = false;
};

static UIThemeConfig g_defaultTheme;
static std::unordered_map<HWND, UIThemeConfig> g_uiThemes;
static std::unordered_map<HWND, HBRUSH> g_uiInputBrushes;
static std::unordered_map<HWND, bool> g_trafficLightsHover;
static std::unordered_map<HWND, WNDPROC> g_buttonOrigProcs;
static std::unordered_map<HWND, ButtonAnimState> g_buttonAnim;
static std::unordered_map<HWND, bool> g_buttonHoverState;
static std::unordered_map<HWND, bool> g_buttonPressedState;
static std::unordered_map<HWND, bool> g_controlPrimary;
static std::unordered_map<HWND, int> g_uiAppliedHeaderOffset;

struct UIControlEvent {
    TzdValue onClickCallback;
    TzdValue onChangeCallback;
};

struct UIWindowEvent {
    TzdValue onCloseCallback;
    TzdValue onResizeCallback;
    TzdValue onErrorCallback;
    bool isClosed = false;
};

static std::unordered_map<HWND, UIControlEvent> g_uiControlEvents;
static std::unordered_map<HWND, UIWindowEvent> g_uiWindowEvents;
static TzdValue g_uiGlobalOnError;
static bool g_uiShowErrorDialog = true;
static std::mutex g_uiMutex;
static TzdInterpreter* g_uiInterp = nullptr;

static std::wstring utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], sizeNeeded);
    return wstr;
}

static std::string wideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], sizeNeeded, NULL, NULL);
    return str;
}

static void EnableVisualStyles() {
    static bool s_actCtxInit = false;
    if (s_actCtxInit) return;
    s_actCtxInit = true;

    WCHAR sysDir[MAX_PATH];
    if (GetSystemDirectoryW(sysDir, MAX_PATH)) {
        std::wstring shell32 = std::wstring(sysDir) + L"\\shell32.dll";
        ACTCTXW actCtx;
        ZeroMemory(&actCtx, sizeof(actCtx));
        actCtx.cbSize = sizeof(actCtx);
        actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        actCtx.lpSource = shell32.c_str();
        actCtx.lpResourceName = MAKEINTRESOURCEW(124);
        HANDLE hCtx = CreateActCtxW(&actCtx);
        if (hCtx != INVALID_HANDLE_VALUE) {
            ULONG_PTR cookie = 0;
            ActivateActCtx(hCtx, &cookie);
        }
    }
}

static void applyModernFont(HWND hwnd) {
    if (!g_hDefaultFont) {
        HDC hdc = GetDC(NULL);
        int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
        int fontHeight = -MulDiv(9, logPixelsY, 72);
        g_hDefaultFont = CreateFontW(
            fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI"
        );
        if (!g_hDefaultFont) {
            NONCLIENTMETRICSW ncm;
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
            g_hDefaultFont = CreateFontIndirectW(&ncm.lfMessageFont);
        }
    }
    if (g_hDefaultFont) {
        SendMessageW(hwnd, WM_SETFONT, (WPARAM)g_hDefaultFont, TRUE);
    }
}

// Forward declaration - defined later in the file
static void AddRoundedRect(Gdiplus::GraphicsPath& path, Gdiplus::RectF rect, float radius);

static LRESULT CALLBACK GroupBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCHITTEST) {
        return HTTRANSPARENT;
    }

    if (msg == WM_ERASEBKGND) {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        HWND parent = GetParent(hwnd);
        COLORREF bg = RGB(243, 243, 243);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(parent);
            if (it != g_uiThemes.end()) bg = it->second.bg;
        }
        HBRUSH hbr = CreateSolidBrush(bg);
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);
        return 1;
    }

    if (msg == WM_PAINT) {
        HWND parent = GetParent(hwnd);
        UIThemeConfig theme = g_defaultTheme;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(parent);
            if (it != g_uiThemes.end()) theme = it->second;
        }

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        if (w > 4 && h > 4) {
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h);
            HGDIOBJ oldBM = SelectObject(memDC, memBM);

            // Step 1: Pristine solid background matching parent window
            HBRUSH bgBrush = CreateSolidBrush(theme.bg);
            FillRect(memDC, &rc, bgBrush);
            DeleteObject(bgBrush);

            Gdiplus::Graphics g(memDC);
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

            // Title text area height
            bool hasTitle = false;
            wchar_t titleBuf[128] = { 0 };
            GetWindowTextW(hwnd, titleBuf, 127);
            hasTitle = (titleBuf[0] != L'\0');
            float titleH = hasTitle ? 20.0f : 0.0f;

            // Border color: very soft, modern 1px flat border (no ugly 3D etched bevel!)
            // Light: RGB(225, 228, 234)
            // Dark: RGB(58, 60, 68)
            COLORREF borderCr = (theme.mode == UIThemeMode::DARK)
                ? RGB(58, 60, 68) : RGB(225, 228, 234);

            float radius = 6.0f;
            float topY = hasTitle ? (titleH * 0.5f) : 2.0f;
            Gdiplus::RectF borderRect(1.0f, topY, (float)(w - 2), (float)(h - topY - 2.0f));

            if (theme.style != UIStyleKind::MACOS) {
                Gdiplus::GraphicsPath path;
                AddRoundedRect(path, borderRect, radius);
                Gdiplus::Pen pen(Gdiplus::Color(255, GetRValue(borderCr), GetGValue(borderCr), GetBValue(borderCr)), 1.0f);
                g.DrawPath(&pen, &path);
            }

            if (hasTitle) {
                Gdiplus::FontFamily ff(theme.fontFamily.c_str());
                Gdiplus::Font* pFont = nullptr;
                float fontSize = (float)theme.fontSize;
                if (ff.IsAvailable()) {
                    pFont = new Gdiplus::Font(&ff, fontSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
                } else {
                    Gdiplus::FontFamily fallback(L"Microsoft YaHei UI");
                    pFont = new Gdiplus::Font(&fallback, fontSize, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
                }

                // Measure text
                Gdiplus::RectF measRect;
                Gdiplus::StringFormat sfMeas;
                g.MeasureString(titleBuf, -1, pFont, Gdiplus::PointF(0, 0), &sfMeas, &measRect);
                float textX = 14.0f;
                float textY = 0.0f;

                // In Windows style, erase border line behind title text
                if (theme.style != UIStyleKind::MACOS) {
                    Gdiplus::SolidBrush eraseBrush(Gdiplus::Color(255, GetRValue(theme.bg), GetGValue(theme.bg), GetBValue(theme.bg)));
                    g.FillRectangle(&eraseBrush, textX - 4.0f, 0.0f, measRect.Width + 8.0f, titleH);
                }

                COLORREF labelColor = (theme.mode == UIThemeMode::DARK)
                    ? RGB(180, 185, 195) : RGB(85, 90, 100);
                Gdiplus::SolidBrush textBrush(Gdiplus::Color(GetRValue(labelColor), GetGValue(labelColor), GetBValue(labelColor)));
                Gdiplus::RectF textRect(textX, textY, (float)(w - 28), titleH);
                g.DrawString(titleBuf, -1, pFont, textRect, nullptr, &textBrush);
                delete pFont;
            }

            BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBM);
            DeleteObject(memBM);
            DeleteDC(memDC);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    WNDPROC oldProc = (WNDPROC)GetPropW(hwnd, L"TzdOldProc");
    if (oldProc) {
        return CallWindowProcW(oldProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void handleUiCallbackError(
    const std::string& errType,
    const std::string& errMsg,
    size_t line,
    size_t column,
    const std::vector<std::string>& stackTrace,
    const TzdValue& callable,
    HWND hwndContext,
    const TzdValue& rawThrownVal)
{
    std::string filePath = callable.sourceFile;
    if (filePath.empty() && g_uiInterp && !g_uiInterp->m_scriptPathStack.empty()) {
        filePath = g_uiInterp->m_scriptPathStack.back().string();
    }

    std::string sourceCode;
    if (g_uiInterp) {
        sourceCode = g_uiInterp->m_currentSource;
    }
    if (sourceCode.empty() && !filePath.empty()) {
        std::ifstream ifs(filePath);
        if (ifs.is_open()) {
            std::stringstream ss;
            ss << ifs.rdbuf();
            sourceCode = ss.str();
        }
    }

    std::string sourceLine;
    std::string pointerLine;
    if (!sourceCode.empty() && line > 0) {
        std::istringstream iss(sourceCode);
        std::string cur;
        size_t curL = 1;
        while (std::getline(iss, cur)) {
            if (!cur.empty() && cur.back() == '\r') cur.pop_back();
            if (curL == line) {
                sourceLine = cur;
                for (size_t i = 0; i < column; ++i) {
                    if (i < cur.size() && cur[i] == '\t') pointerLine += '\t';
                    else pointerLine += ' ';
                }
                pointerLine += "^--- 这里";
                break;
            }
            curL++;
        }
    }

    std::string formattedTrace;
    for (auto it = stackTrace.rbegin(); it != stackTrace.rend(); ++it) {
        formattedTrace += "    at " + *it + "\n";
    }
    if (formattedTrace.empty() && !callable.name.empty()) {
        formattedTrace = "    at " + callable.name + (line > 0 ? (" (line " + std::to_string(line) + ")") : "") + "\n";
    }

    // Build structured error object for TzdLang
    std::unordered_map<std::string, TzdValue> errMap;
    errMap["type"] = TzdValue(errType);
    errMap["errorType"] = TzdValue(errType);
    errMap["message"] = TzdValue(errMsg);
    errMap["line"] = TzdValue((double)line);
    errMap["column"] = TzdValue((double)column);
    errMap["file"] = TzdValue(filePath);
    errMap["source"] = TzdValue(sourceLine);
    errMap["stackTraceStr"] = TzdValue(formattedTrace);

    std::vector<TzdValue> traceList;
    for (const auto& s : stackTrace) {
        traceList.push_back(TzdValue(s));
    }
    errMap["stackTrace"] = TzdValue(traceList);
    if (rawThrownVal.type != TzdValue::NONE) {
        errMap["value"] = rawThrownVal;
    }
    TzdValue errObj(errMap);

    // Look for window handler or global handler
    HWND rootHwnd = hwndContext;
    if (rootHwnd && (GetWindowLongPtrW(rootHwnd, GWL_STYLE) & WS_CHILD)) {
        HWND ancestor = GetAncestor(rootHwnd, GA_ROOT);
        if (ancestor) rootHwnd = ancestor;
    }

    TzdValue handler;
    {
        std::lock_guard<std::mutex> lock(g_uiMutex);
        if (rootHwnd) {
            auto it = g_uiWindowEvents.find(rootHwnd);
            if (it != g_uiWindowEvents.end()) handler = it->second.onErrorCallback;
        }
        if (handler.type != TzdValue::FUNCTION && handler.type != TzdValue::NATIVE_FUNCTION) {
            handler = g_uiGlobalOnError;
        }
    }

    if (handler.type == TzdValue::FUNCTION || handler.type == TzdValue::NATIVE_FUNCTION) {
        bool handled = false;
        try {
            g_uiInterp->callFunction(handler, { errObj });
            handled = true;
        } catch (const std::exception& hEx) {
            std::cerr << "[TzdUI ErrorHandler Failed] " << hEx.what() << std::endl;
        } catch (...) {
            std::cerr << "[TzdUI ErrorHandler Failed] Unknown exception" << std::endl;
        }
        if (handled) return;
    }

    // Unhandled: Terminal colorized report
    TzdErrorHandler::report(errType, line, column, errMsg, sourceCode, stackTrace);

    // Development MessageBox dialog if enabled
    if (g_uiShowErrorDialog) {
        std::wstring wType = utf8ToWide(errType);
        std::wstring wMsg = utf8ToWide(errMsg);
        std::wstring wFile = utf8ToWide(filePath);
        std::wstring wSource = utf8ToWide(sourceLine);
        std::wstring wPointer = utf8ToWide(pointerLine);
        std::wstring wTrace = utf8ToWide(formattedTrace);

        std::wstring dialogText = L"发生未捕获的 UI 事件异常 (TzdUI Callback Exception):\n\n";
        dialogText += L"【错误类型】: " + wType + L"\n";
        dialogText += L"【错误信息】: " + wMsg + L"\n";
        if (line > 0) {
            dialogText += L"【错误位置】: 第 " + std::to_wstring(line) + L" 行, 第 " + std::to_wstring(column) + L" 列\n";
        }
        if (!wFile.empty()) {
            dialogText += L"【源文件】: " + wFile + L"\n";
        }
        if (!wSource.empty()) {
            dialogText += L"\n【代码定位】:\n    " + wSource + L"\n    " + wPointer + L"\n";
        }
        if (!wTrace.empty()) {
            dialogText += L"\n【调用栈】:\n" + wTrace;
        }
        dialogText += L"\n--------------------------------------------------\n";
        dialogText += L"提示: 可使用 win.onError(fun(err) { ... }) 或 App.onError(...) 捕获并处理此异常。";

        HWND dialogParent = rootHwnd ? rootHwnd : GetActiveWindow();
        MessageBoxW(dialogParent, dialogText.c_str(), L"TzdUI 运行时错误", MB_ICONERROR | MB_OK | MB_SETFOREGROUND);
    }
}

static TzdValue invokeUiCallback(const TzdValue& cb, std::vector<TzdValue> args, HWND hwndContext = NULL) {
    if (!g_uiInterp) return TzdValue();
    if (cb.type != TzdValue::FUNCTION && cb.type != TzdValue::NATIVE_FUNCTION) return TzdValue();

    g_CurrentInterpreter = g_uiInterp;
    TzdValue callable = cb;
    std::vector<TzdValue> actualArgs = args;

    if (callable.type == TzdValue::FUNCTION) {
        if (!callable.params.empty()) {
            if (actualArgs.size() > callable.params.size()) {
                actualArgs.resize(callable.params.size());
            } else {
                while (actualArgs.size() < callable.params.size()) {
                    actualArgs.push_back(TzdValue());
                }
            }
        } else if (callable.jittedPtr != nullptr) {
            callable.params.resize(actualArgs.size());
            for (size_t i = 0; i < actualArgs.size(); ++i) {
                callable.params[i] = "arg" + std::to_string(i);
            }
        } else {
            actualArgs.clear();
        }
    }

    try {
        return g_uiInterp->callFunction(callable, actualArgs);
    } catch (const TzdRuntimeException& e) {
        handleUiCallbackError("Tzd 运行时错误", e.what(), e.line, e.column, e.stackTrace, callable, hwndContext, TzdValue());
    } catch (const TzdThrowException& e) {
        std::string msg = "Unknown error";
        if (e.value.type == TzdValue::INSTANCE && e.value.instanceVal) {
            try {
                TzdValue toStr = e.value.instanceVal->getMember("message");
                if (toStr.type == TzdValue::STRING && !toStr.sVal.empty()) msg = toStr.sVal;
            } catch (...) {}
        } else if (e.value.type == TzdValue::STRING) {
            msg = e.value.sVal;
        } else {
            msg = TzdInterpreter::getAsString(e.value);
        }
        handleUiCallbackError("Tzd 未捕获异常", msg, 0, 0, e.stackTrace, callable, hwndContext, e.value);
    } catch (const std::exception& e) {
        handleUiCallbackError("系统异常", e.what(), 0, 0, {}, callable, hwndContext, TzdValue(std::string(e.what())));
    } catch (...) {
        handleUiCallbackError("未知异常", "An unexpected native exception occurred", 0, 0, {}, callable, hwndContext, TzdValue());
    }
    return TzdValue();
}

static COLORREF parseHexColor(const std::string& str, COLORREF defaultCol) {
    if (str.empty()) return defaultCol;
    std::string s = str;
    if (s[0] == '#') s = s.substr(1);
    try {
        if (s.length() == 3) {
            int r = std::stoi(std::string(2, s[0]), nullptr, 16);
            int g = std::stoi(std::string(2, s[1]), nullptr, 16);
            int b = std::stoi(std::string(2, s[2]), nullptr, 16);
            return RGB(r, g, b);
        }
        if (s.length() == 6) {
            int r = std::stoi(s.substr(0, 2), nullptr, 16);
            int g = std::stoi(s.substr(2, 2), nullptr, 16);
            int b = std::stoi(s.substr(4, 2), nullptr, 16);
            return RGB(r, g, b);
        }
    } catch (...) {}
    return defaultCol;
}

static COLORREF getMapColor(const std::unordered_map<std::string, TzdValue>& m, const std::string& key, COLORREF defaultCol) {
    auto it = m.find(key);
    if (it != m.end()) {
        if (it->second.type == TzdValue::STRING) {
            return parseHexColor(it->second.sVal, defaultCol);
        } else if (it->second.type == TzdValue::INT || it->second.type == TzdValue::DOUBLE || it->second.type == TzdValue::LONG || it->second.type == TzdValue::FLOAT) {
            return (COLORREF)TzdInterpreter::getAsDoubleInternal(it->second);
        } else if (it->second.type == TzdValue::ARRAY && it->second.arrVal.size() >= 3) {
            return RGB((int)TzdInterpreter::getAsDoubleInternal(it->second.arrVal[0]),
                       (int)TzdInterpreter::getAsDoubleInternal(it->second.arrVal[1]),
                       (int)TzdInterpreter::getAsDoubleInternal(it->second.arrVal[2]));
        }
    }
    return defaultCol;
}

static double getMapNumber(const std::unordered_map<std::string, TzdValue>& m, const std::string& key, double defaultVal) {
    auto it = m.find(key);
    if (it != m.end() && (it->second.type == TzdValue::INT || it->second.type == TzdValue::DOUBLE || it->second.type == TzdValue::LONG || it->second.type == TzdValue::FLOAT)) {
        return TzdInterpreter::getAsDoubleInternal(it->second);
    }
    return defaultVal;
}

static void AddRoundedRect(Gdiplus::GraphicsPath& path, Gdiplus::RectF rect, float radius) {
    if (radius <= 0.5f) {
        path.AddRectangle(rect);
        return;
    }
    float d = radius * 2.0f;
    if (d > rect.Width) d = rect.Width;
    if (d > rect.Height) d = rect.Height;
    path.AddArc(rect.X, rect.Y, d, d, 180.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - d, rect.Y, d, d, 270.0f, 90.0f);
    path.AddArc(rect.X + rect.Width - d, rect.Y + rect.Height - d, d, d, 0.0f, 90.0f);
    path.AddArc(rect.X, rect.Y + rect.Height - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

static inline COLORREF blendColor(COLORREF c1, COLORREF c2, float t) {
    if (t <= 0.0f) return c1;
    if (t >= 1.0f) return c2;
    int r = (int)(GetRValue(c1) + (GetRValue(c2) - GetRValue(c1)) * t);
    int g = (int)(GetGValue(c1) + (GetGValue(c2) - GetGValue(c1)) * t);
    int b = (int)(GetBValue(c1) + (GetBValue(c2) - GetBValue(c1)) * t);
    return RGB(std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
}

static void setupThemePalette(UIThemeConfig& theme) {
    if (theme.style == UIStyleKind::MACOS) {
        theme.hasCustomHeader = true;
        theme.hasTrafficLights = true;
        theme.headerHeight = 38;
        theme.buttonRadius = 6;
        theme.fontFamily = L"Microsoft YaHei UI";

        if (theme.mode == UIThemeMode::LIGHT) {
            // macOS Sonoma / Sequoia light - authentic background layering
            theme.bg = RGB(242, 242, 247);     // system grouped background
            theme.fg = RGB(20, 20, 22);
            theme.accent = RGB(0, 122, 255);   // macOS blue

            // Buttons: light gray — visible on gray bg without needing a border
            theme.buttonBg = RGB(229, 229, 234);        // macOS secondary button color
            theme.buttonHoverBg = RGB(218, 218, 224);   // darker on hover
            theme.buttonPressedBg = RGB(205, 205, 212); // even darker on press
            theme.buttonFg = RGB(20, 20, 22);
            theme.buttonBorder = RGB(200, 200, 210);    // kept for non-macOS fallback

            // Input: pure white with soft border
            theme.inputBg = RGB(255, 255, 255);
            theme.inputFg = RGB(20, 20, 22);
            theme.inputBorder = RGB(180, 180, 195);

            // Header: unified toolbar look, slightly darker than bg
            theme.headerBg = RGB(225, 225, 230);
            theme.headerFg = RGB(20, 20, 22);
            theme.headerBorder = RGB(196, 196, 206);
        } else if (theme.mode == UIThemeMode::DARK) {
            // macOS Sonoma dark
            theme.bg = RGB(28, 28, 30);
            theme.fg = RGB(242, 242, 247);
            theme.accent = RGB(10, 132, 255);

            theme.buttonBg = RGB(52, 52, 56);
            theme.buttonHoverBg = RGB(66, 66, 72);
            theme.buttonPressedBg = RGB(40, 40, 44);
            theme.buttonFg = RGB(242, 242, 247);
            theme.buttonBorder = RGB(68, 68, 76);

            theme.inputBg = RGB(38, 38, 42);
            theme.inputFg = RGB(242, 242, 247);
            theme.inputBorder = RGB(62, 62, 70);

            theme.headerBg = RGB(36, 36, 38);
            theme.headerFg = RGB(242, 242, 247);
            theme.headerBorder = RGB(52, 52, 58);
        } else { // MONOCHROME
            theme.bg = RGB(255, 255, 255);
            theme.fg = RGB(0, 0, 0);
            theme.accent = RGB(0, 0, 0);

            theme.buttonBg = RGB(255, 255, 255);
            theme.buttonHoverBg = RGB(0, 0, 0);
            theme.buttonPressedBg = RGB(50, 50, 50);
            theme.buttonFg = RGB(0, 0, 0);
            theme.buttonBorder = RGB(0, 0, 0);

            theme.inputBg = RGB(255, 255, 255);
            theme.inputFg = RGB(0, 0, 0);
            theme.inputBorder = RGB(0, 0, 0);

            theme.headerBg = RGB(245, 245, 245);
            theme.headerFg = RGB(0, 0, 0);
            theme.headerBorder = RGB(0, 0, 0);
        }
    } else if (theme.style == UIStyleKind::WINDOWS) {
        theme.hasCustomHeader = false;
        theme.hasTrafficLights = false;
        theme.buttonRadius = 4;
        theme.fontFamily = L"Microsoft YaHei UI";

        if (theme.mode == UIThemeMode::LIGHT) {
            theme.bg = RGB(243, 243, 243);
            theme.fg = RGB(32, 32, 32);
            theme.accent = RGB(0, 103, 192);

            theme.buttonBg = RGB(255, 255, 255);
            theme.buttonHoverBg = RGB(236, 240, 248);
            theme.buttonPressedBg = RGB(218, 224, 236);
            theme.buttonFg = RGB(24, 24, 28);
            theme.buttonBorder = RGB(205, 210, 218);

            theme.inputBg = RGB(255, 255, 255);
            theme.inputFg = RGB(32, 32, 32);
            theme.inputBorder = RGB(218, 218, 222);
        } else if (theme.mode == UIThemeMode::DARK) {
            theme.bg = RGB(32, 32, 32);
            theme.fg = RGB(240, 240, 240);
            theme.accent = RGB(96, 205, 255);

            theme.buttonBg = RGB(45, 45, 48);
            theme.buttonHoverBg = RGB(64, 64, 72);
            theme.buttonPressedBg = RGB(32, 32, 36);
            theme.buttonFg = RGB(240, 240, 245);
            theme.buttonBorder = RGB(72, 72, 78);

            theme.inputBg = RGB(38, 38, 42);
            theme.inputFg = RGB(240, 240, 240);
            theme.inputBorder = RGB(65, 65, 70);
        } else { // MONOCHROME
            theme.bg = RGB(255, 255, 255);
            theme.fg = RGB(0, 0, 0);
            theme.accent = RGB(0, 0, 0);

            theme.buttonBg = RGB(255, 255, 255);
            theme.buttonHoverBg = RGB(0, 0, 0);
            theme.buttonPressedBg = RGB(40, 40, 40);
            theme.buttonFg = RGB(0, 0, 0);
            theme.buttonBorder = RGB(0, 0, 0);

            theme.inputBg = RGB(255, 255, 255);
            theme.inputFg = RGB(0, 0, 0);
            theme.inputBorder = RGB(0, 0, 0);
        }
    }
}

static int getEffectiveControlY(HWND parent, int y) {
    std::lock_guard<std::mutex> lock(g_uiMutex);
    auto it = g_uiThemes.find(parent);
    if (it != g_uiThemes.end() && it->second.hasCustomHeader) {
        return y + it->second.headerHeight;
    }
    return y;
}

static void paintStyledButton(HWND hwnd, HDC hdc) {
    HWND parent = GetParent(hwnd);
    UIThemeConfig theme;
    {
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_uiThemes.find(parent);
        if (it != g_uiThemes.end()) {
            theme = it->second;
        } else {
            theme = g_defaultTheme;
        }
    }

    bool isPrimary = false;
    {
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_controlPrimary.find(hwnd);
        if (it != g_controlPrimary.end()) isPrimary = it->second;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBM = CreateCompatibleBitmap(hdc, w, h);
    HGDIOBJ oldBM = SelectObject(memDC, memBM);
    HBRUSH hParentBrush = CreateSolidBrush(theme.bg);
    FillRect(memDC, &rc, hParentBrush);
    DeleteObject(hParentBrush);

    {
        Gdiplus::Graphics g(memDC);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

        float hoverAlpha = 0.0f;
        bool isPressed = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_buttonAnim.find(hwnd);
            if (it != g_buttonAnim.end()) {
                hoverAlpha = it->second.hoverAlpha;
                isPressed = it->second.isPressed;
            } else {
                hoverAlpha = g_buttonHoverState[hwnd] ? 1.0f : 0.0f;
                isPressed = g_buttonPressedState[hwnd];
            }
        }
        bool isEnabled = (IsWindowEnabled(hwnd) != FALSE);

        COLORREF bgCol = theme.buttonBg;
        COLORREF fgCol = theme.buttonFg;
        COLORREF borderCol = theme.buttonBorder;
        int radius = theme.buttonRadius;

        if (theme.style == UIStyleKind::MACOS) {
            // macOS pill-ish radius but not too extreme
            radius = std::min(6, h / 2 - 1);
        }

        if (isPrimary) {
            bgCol = theme.accent;
            fgCol = RGB(255, 255, 255);
            borderCol = theme.accent;
            if (theme.mode == UIThemeMode::MONOCHROME) {
                bgCol = RGB(0, 0, 0);
                fgCol = RGB(255, 255, 255);
                borderCol = RGB(0, 0, 0);
            }
        }

        if (!isEnabled) {
            bgCol = (theme.mode == UIThemeMode::DARK) ? RGB(45, 45, 45) : RGB(235, 235, 238);
            fgCol = (theme.mode == UIThemeMode::DARK) ? RGB(110, 110, 110) : RGB(160, 160, 165);
            borderCol = (theme.mode == UIThemeMode::DARK) ? RGB(60, 60, 60) : RGB(215, 215, 220);
        } else if (isPressed) {
            if (isPrimary && theme.mode != UIThemeMode::MONOCHROME) {
                int r = std::max(0, (int)GetRValue(theme.accent) - 35);
                int gg = std::max(0, (int)GetGValue(theme.accent) - 35);
                int b = std::max(0, (int)GetBValue(theme.accent) - 35);
                bgCol = RGB(r, gg, b);
                borderCol = bgCol;
            } else {
                bgCol = theme.buttonPressedBg;
                borderCol = (theme.mode == UIThemeMode::DARK)
                    ? RGB(55, 55, 62) : RGB(160, 172, 190);
            }
        } else {
            // Smooth hover animation & color transition
            if (theme.mode == UIThemeMode::MONOCHROME) {
                if (hoverAlpha > 0.5f) {
                    if (!isPrimary) { bgCol = RGB(0, 0, 0); fgCol = RGB(255, 255, 255); }
                    else { bgCol = RGB(255, 255, 255); fgCol = RGB(0, 0, 0); }
                }
            } else if (isPrimary) {
                int r = std::min(255, (int)GetRValue(theme.accent) + 20);
                int gg = std::min(255, (int)GetGValue(theme.accent) + 20);
                int b = std::min(255, (int)GetBValue(theme.accent) + 20);
                COLORREF hoverAccent = RGB(r, gg, b);
                bgCol = blendColor(theme.accent, hoverAccent, hoverAlpha);
                borderCol = bgCol;
            } else {
                bgCol = blendColor(theme.buttonBg, theme.buttonHoverBg, hoverAlpha);
                COLORREF hoverBorder = (theme.mode == UIThemeMode::DARK)
                    ? RGB(95, 95, 105) : RGB(170, 180, 195);
                borderCol = blendColor(theme.buttonBorder, hoverBorder, hoverAlpha);
            }
        }

        Gdiplus::GraphicsPath path;
        Gdiplus::RectF btnRect = (theme.style == UIStyleKind::MACOS)
            ? Gdiplus::RectF(0.0f, 0.0f, (float)w, (float)h)
            : Gdiplus::RectF(0.5f, 0.5f, (float)(w - 1), (float)(h - 1));
        AddRoundedRect(path, btnRect, (float)radius);

        // === macOS gradient: subtle top highlight ===
        if (theme.style == UIStyleKind::MACOS && isEnabled && !isPressed) {
            BYTE br = GetRValue(bgCol), bg2 = GetGValue(bgCol), bb = GetBValue(bgCol);
            int lift = isPrimary ? 20 : 14;
            BYTE tr = (BYTE)std::min(255, (int)br + lift);
            BYTE tg = (BYTE)std::min(255, (int)bg2 + lift);
            BYTE tb = (BYTE)std::min(255, (int)bb + lift);
            Gdiplus::LinearGradientBrush gradBrush(
                Gdiplus::PointF(0.f, 0.f),
                Gdiplus::PointF(0.f, (float)h),
                Gdiplus::Color(255, tr, tg, tb),
                Gdiplus::Color(255, br, bg2, bb)
            );
            g.FillPath(&gradBrush, &path);
        } else {
            // Solid fill for pressed/disabled/non-macOS
            Gdiplus::SolidBrush fillBrush(Gdiplus::Color(GetRValue(bgCol), GetGValue(bgCol), GetBValue(bgCol)));
            g.FillPath(&fillBrush, &path);
        }

        // Border
        if (theme.style == UIStyleKind::MACOS) {
            // No border drawn at all — shape defined by background fill only
        } else if (theme.mode == UIThemeMode::MONOCHROME) {
            Gdiplus::Pen bPen(Gdiplus::Color(GetRValue(borderCol), GetGValue(borderCol), GetBValue(borderCol)), 1.5f);
            g.DrawPath(&bPen, &path);
        } else {
            // Windows style: crisp modern border with hover transition
            int bAlpha = isPressed ? 255 : (int)(160.0f + 95.0f * hoverAlpha);
            Gdiplus::Pen bPen(Gdiplus::Color((BYTE)std::min(255, bAlpha), GetRValue(borderCol), GetGValue(borderCol), GetBValue(borderCol)), 1.0f);
            g.DrawPath(&bPen, &path);
        }

        // Text
        wchar_t textBuf[256] = { 0 };
        GetWindowTextW(hwnd, textBuf, 255);
        if (textBuf[0] != L'\0') {
            Gdiplus::FontFamily ff(theme.fontFamily.c_str());
            Gdiplus::Font* pFont = nullptr;
            // macOS uses slightly heavier weight for buttons
            Gdiplus::FontStyle fStyle = (isPrimary || theme.style == UIStyleKind::MACOS)
                ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
            if (ff.IsAvailable()) {
                pFont = new Gdiplus::Font(&ff, (float)theme.fontSize, fStyle, Gdiplus::UnitPoint);
            } else {
                Gdiplus::FontFamily fallback(L"Microsoft YaHei UI");
                pFont = new Gdiplus::Font(&fallback, (float)theme.fontSize, fStyle, Gdiplus::UnitPoint);
            }

            Gdiplus::SolidBrush textBrush(Gdiplus::Color(GetRValue(fgCol), GetGValue(fgCol), GetBValue(fgCol)));
            Gdiplus::StringFormat sf;
            sf.SetAlignment(Gdiplus::StringAlignmentCenter);
            sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            Gdiplus::RectF textRect = btnRect;
            if (isPressed) {
                textRect.Y += 1.0f; // Visual tactile press feedback
            }
            g.DrawString(textBuf, -1, pFont, textRect, &sf, &textBrush);
            delete pFont;
        }
    }

    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBM);
    DeleteObject(memBM);
    DeleteDC(memDC);
}

static LRESULT CALLBACK StyledButtonWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WNDPROC origProc = DefWindowProcW;
    {
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_buttonOrigProcs.find(hwnd);
        if (it != g_buttonOrigProcs.end()) origProc = it->second;
    }

    switch (msg) {
    case WM_NCCALCSIZE:
        return 0;

    case WM_NCPAINT:
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        paintStyledButton(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_PRINTCLIENT:
    case WM_PRINT: {
        HDC hdc = (HDC)wParam;
        paintStyledButton(hwnd, hdc);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);

        if (GetCapture() == hwnd) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            bool inRect = PtInRect(&rc, pt) != FALSE;
            bool changed = false;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                if (g_buttonAnim[hwnd].isPressed != inRect) {
                    g_buttonAnim[hwnd].isPressed = inRect;
                    g_buttonPressedState[hwnd] = inRect;
                    changed = true;
                }
            }
            if (changed) {
                InvalidateRect(hwnd, NULL, FALSE);
                UpdateWindow(hwnd);
            }
        }

        bool startTimer = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto& st = g_buttonAnim[hwnd];
            g_buttonHoverState[hwnd] = true;
            st.targetHover = 1.0f;
            if (!st.timerRunning && st.hoverAlpha < 1.0f) {
                st.timerRunning = true;
                startTimer = true;
            }
        }
        if (startTimer) {
            SetTimer(hwnd, 1001, 16, NULL);
        }
        break;
    }

    case WM_MOUSELEAVE: {
        bool startTimer = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto& st = g_buttonAnim[hwnd];
            g_buttonHoverState[hwnd] = false;
            st.targetHover = 0.0f;
            if (!st.timerRunning && st.hoverAlpha > 0.0f) {
                st.timerRunning = true;
                startTimer = true;
            }
        }
        if (startTimer) {
            SetTimer(hwnd, 1001, 16, NULL);
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == 1001) {
            bool keepGoing = false;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                auto& st = g_buttonAnim[hwnd];
                float step = 0.18f; // ~90ms smooth transition
                if (st.hoverAlpha < st.targetHover) {
                    st.hoverAlpha += step;
                    if (st.hoverAlpha >= st.targetHover) {
                        st.hoverAlpha = st.targetHover;
                    } else {
                        keepGoing = true;
                    }
                } else if (st.hoverAlpha > st.targetHover) {
                    st.hoverAlpha -= step;
                    if (st.hoverAlpha <= st.targetHover) {
                        st.hoverAlpha = st.targetHover;
                    } else {
                        keepGoing = true;
                    }
                }
                if (!keepGoing) st.timerRunning = false;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            if (!keepGoing) {
                KillTimer(hwnd, 1001);
            }
            return 0;
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_buttonAnim[hwnd].isPressed = true;
            g_buttonPressedState[hwnd] = true;
        }
        SetCapture(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd); // IMMEDIATELY SHOW PRESSED STATE!
        return 0;
    }

    case WM_LBUTTONUP: {
        bool wasPressed = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto& st = g_buttonAnim[hwnd];
            wasPressed = st.isPressed;
            st.isPressed = false;
            g_buttonPressedState[hwnd] = false;
        }
        if (GetCapture() == hwnd) ReleaseCapture();
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd); // IMMEDIATELY SHOW RESTORED STATE!
        if (wasPressed) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            if (PtInRect(&rc, pt)) {
                HWND parent = GetParent(hwnd);
                if (parent) {
                    SendMessageW(parent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), BN_CLICKED), (LPARAM)hwnd);
                }
            }
        }
        return 0;
    }

    case WM_CAPTURECHANGED:
    case WM_CANCELMODE: {
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_buttonAnim[hwnd].isPressed = false;
            g_buttonPressedState[hwnd] = false;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        break;
    }

    case BM_SETSTATE: {
        bool pressed = (wParam != 0);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_buttonAnim[hwnd].isPressed = pressed;
            g_buttonPressedState[hwnd] = pressed;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        return 0;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_SPACE) {
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                g_buttonAnim[hwnd].isPressed = true;
                g_buttonPressedState[hwnd] = true;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }
        return CallWindowProcW(origProc, hwnd, msg, wParam, lParam);
    }

    case WM_KEYUP: {
        if (wParam == VK_SPACE) {
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                g_buttonAnim[hwnd].isPressed = false;
                g_buttonPressedState[hwnd] = false;
            }
            InvalidateRect(hwnd, NULL, FALSE);
            UpdateWindow(hwnd);
        }
        return CallWindowProcW(origProc, hwnd, msg, wParam, lParam);
    }

    case BM_CLICK: {
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_buttonAnim[hwnd].isPressed = true;
            g_buttonPressedState[hwnd] = true;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        Sleep(50);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_buttonAnim[hwnd].isPressed = false;
            g_buttonPressedState[hwnd] = false;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        HWND parent = GetParent(hwnd);
        if (parent) {
            SendMessageW(parent, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hwnd), BN_CLICKED), (LPARAM)hwnd);
        }
        return 0;
    }

    case WM_DESTROY: {
        KillTimer(hwnd, 1001);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_buttonAnim.erase(hwnd);
            g_buttonHoverState.erase(hwnd);
            g_buttonPressedState.erase(hwnd);
            g_buttonOrigProcs.erase(hwnd);
            g_controlPrimary.erase(hwnd);
        }
        break;
    }

    case WM_SETTEXT: {
        LRESULT res = CallWindowProcW(origProc, hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return res;
    }

    case WM_ENABLE: {
        LRESULT res = CallWindowProcW(origProc, hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return res;
    }
    }

    return CallWindowProcW(origProc, hwnd, msg, wParam, lParam);
}

static void subclassStyledButton(HWND btn) {
    std::lock_guard<std::mutex> lock(g_uiMutex);
    if (g_buttonOrigProcs.find(btn) == g_buttonOrigProcs.end()) {
        WNDPROC old = (WNDPROC)SetWindowLongPtrW(btn, GWLP_WNDPROC, (LONG_PTR)StyledButtonWndProc);
        g_buttonOrigProcs[btn] = old;
    }
}

static void paintMacHeader(HWND hwnd, HDC hdc, const UIThemeConfig& theme) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int w = rcClient.right - rcClient.left;
    int h = theme.headerHeight;

    Gdiplus::Graphics g(hdc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    // === Header: subtle top-to-bottom gradient (macOS toolbar vibrancy look) ===
    bool isDark = (theme.mode == UIThemeMode::DARK);
    COLORREF hbg = theme.headerBg;
    // top of gradient: slightly lighter
    BYTE htR = (BYTE)std::min(255, (int)GetRValue(hbg) + (isDark ? 6 : 8));
    BYTE htG = (BYTE)std::min(255, (int)GetGValue(hbg) + (isDark ? 6 : 8));
    BYTE htB = (BYTE)std::min(255, (int)GetBValue(hbg) + (isDark ? 6 : 8));
    Gdiplus::LinearGradientBrush headerGrad(
        Gdiplus::PointF(0.f, 0.f), Gdiplus::PointF(0.f, (float)h),
        Gdiplus::Color(255, htR, htG, htB),
        Gdiplus::Color(255, GetRValue(hbg), GetGValue(hbg), GetBValue(hbg))
    );
    g.FillRectangle(&headerGrad, 0, 0, w, h);

    // Header Bottom Border Line
    Gdiplus::Color borderCol(GetRValue(theme.headerBorder), GetGValue(theme.headerBorder), GetBValue(theme.headerBorder));
    Gdiplus::Pen borderPen(borderCol, 1.0f);
    g.DrawLine(&borderPen, 0.0f, (float)(h - 1), (float)w, (float)(h - 1));

    bool isHovered = false;
    {
        std::lock_guard<std::mutex> lock(g_uiMutex);
        isHovered = g_trafficLightsHover[hwnd];
    }
    bool isMonochrome = (theme.mode == UIThemeMode::MONOCHROME);

    // Traffic light positioning: centered vertically in header, spaced 20px apart
    float tly = (float)(h - 13) / 2.0f;  // vertical center
    float tlSize = 13.0f;
    float tlX1 = 16.0f, tlX2 = 36.0f, tlX3 = 56.0f;

    Gdiplus::Color redCol   = isMonochrome ? Gdiplus::Color(220, 220, 220) : Gdiplus::Color(255, 95, 86);
    Gdiplus::Color yellowCol = isMonochrome ? Gdiplus::Color(190, 190, 190) : Gdiplus::Color(255, 189, 46);
    Gdiplus::Color greenCol  = isMonochrome ? Gdiplus::Color(160, 160, 160) : Gdiplus::Color(39, 201, 63);

    // 🔴 Red Close
    Gdiplus::SolidBrush redBrush(redCol);
    g.FillEllipse(&redBrush, tlX1, tly, tlSize, tlSize);
    Gdiplus::Pen redPen(isMonochrome ? Gdiplus::Color(0,0,0) : Gdiplus::Color(60, 200, 70, 55), 0.75f);
    g.DrawEllipse(&redPen, tlX1, tly, tlSize, tlSize);

    // 🟡 Yellow Minimize
    Gdiplus::SolidBrush yellowBrush(yellowCol);
    g.FillEllipse(&yellowBrush, tlX2, tly, tlSize, tlSize);
    Gdiplus::Pen yellowPen(isMonochrome ? Gdiplus::Color(0,0,0) : Gdiplus::Color(60, 200, 150, 30), 0.75f);
    g.DrawEllipse(&yellowPen, tlX2, tly, tlSize, tlSize);

    // 🟢 Green Maximize
    Gdiplus::SolidBrush greenBrush(greenCol);
    g.FillEllipse(&greenBrush, tlX3, tly, tlSize, tlSize);
    Gdiplus::Pen greenPen(isMonochrome ? Gdiplus::Color(0,0,0) : Gdiplus::Color(60, 20, 160, 40), 0.75f);
    g.DrawEllipse(&greenPen, tlX3, tly, tlSize, tlSize);

    // Hover glyphs (draw icons inside dots)
    if (isHovered) {
        Gdiplus::Pen glyphPen(Gdiplus::Color(160, 0, 0, 0), 1.3f);
        float cx1 = tlX1 + tlSize/2, cy = tly + tlSize/2;
        float cx2 = tlX2 + tlSize/2, cx3 = tlX3 + tlSize/2;
        float d = 3.0f;
        // × close
        g.DrawLine(&glyphPen, cx1-d, cy-d, cx1+d, cy+d);
        g.DrawLine(&glyphPen, cx1+d, cy-d, cx1-d, cy+d);
        // − minimize
        g.DrawLine(&glyphPen, cx2-d, cy, cx2+d, cy);
        // ⤢ maximize (diagonal arrows hint)
        g.DrawLine(&glyphPen, cx3-d, cy-d, cx3+d, cy+d);
        g.DrawLine(&glyphPen, cx3-d, cy+d, cx3+d, cy-d);
    }

    // Centered Window Title Text — lighter weight, macOS style
    wchar_t titleBuf[256] = { 0 };
    GetWindowTextW(hwnd, titleBuf, 255);
    if (titleBuf[0] != L'\0') {
        Gdiplus::FontFamily ff(theme.fontFamily.c_str());
        Gdiplus::Font* pFont = nullptr;
        // macOS: Regular weight title, 10pt
        if (ff.IsAvailable()) {
            pFont = new Gdiplus::Font(&ff, 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        } else {
            Gdiplus::FontFamily fallback(L"Microsoft YaHei UI");
            pFont = new Gdiplus::Font(&fallback, 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        }

        COLORREF tfg = theme.headerFg;
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(GetRValue(tfg), GetGValue(tfg), GetBValue(tfg)));
        Gdiplus::StringFormat sf;
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        sf.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        Gdiplus::RectF layoutRect(80.0f, 0.0f, (float)(w - 160), (float)h);
        g.DrawString(titleBuf, -1, pFont, layoutRect, &sf, &textBrush);
        delete pFont;
    }
}

static void applyWindowTheme(HWND hwnd) {
    UIThemeConfig theme;
    {
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_uiThemes.find(hwnd);
        if (it != g_uiThemes.end()) {
            theme = it->second;
        } else {
            theme = g_defaultTheme;
            g_uiThemes[hwnd] = theme;
        }

        // Update brushes
        g_uiWindowBgColors[hwnd] = theme.bg;
        if (g_uiWindowBrushes.count(hwnd) && g_uiWindowBrushes[hwnd]) {
            DeleteObject(g_uiWindowBrushes[hwnd]);
        }
        g_uiWindowBrushes[hwnd] = CreateSolidBrush(theme.bg);

        if (g_uiInputBrushes.count(hwnd) && g_uiInputBrushes[hwnd]) {
            DeleteObject(g_uiInputBrushes[hwnd]);
        }
        g_uiInputBrushes[hwnd] = CreateSolidBrush(theme.inputBg);
        g_uiWindowDarkMode[hwnd] = (theme.mode == UIThemeMode::DARK);
    }

    // Adjust child positions if header offset changed
    int currentOffset = g_uiAppliedHeaderOffset[hwnd];
    int desiredOffset = theme.hasCustomHeader ? theme.headerHeight : 0;
    int diff = desiredOffset - currentOffset;
    if (diff != 0) {
        g_uiAppliedHeaderOffset[hwnd] = desiredOffset;
        struct EnumChildParam {
            HWND hwndRoot;
            int dy;
        } param = { hwnd, diff };
        EnumChildWindows(hwnd, [](HWND child, LPARAM lParam) -> BOOL {
            EnumChildParam* p = (EnumChildParam*)lParam;
            if (GetParent(child) != p->hwndRoot) return TRUE;
            RECT rc;
            GetWindowRect(child, &rc);
            POINT pt = { rc.left, rc.top };
            ScreenToClient(p->hwndRoot, &pt);
            SetWindowPos(child, NULL, pt.x, pt.y + p->dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            return TRUE;
        }, (LPARAM)&param);
    }

    BOOL dwmDark = (theme.mode == UIThemeMode::DARK) ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &dwmDark, sizeof(dwmDark));
    DwmSetWindowAttribute(hwnd, 19, &dwmDark, sizeof(dwmDark));

    if (theme.hasCustomHeader) {
        MARGINS margins = { 0, 0, 1, 0 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);
    }

    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    bool isMacOS = (theme.style == UIStyleKind::MACOS);
    EnumChildWindows(hwnd, [](HWND child, LPARAM lp) -> BOOL {
        bool macOS = (bool)lp;
        wchar_t cls[64] = { 0 };
        GetClassNameW(child, cls, 63);
        if (_wcsicmp(cls, L"BUTTON") == 0) {
            LONG_PTR style = GetWindowLongPtrW(child, GWL_STYLE);
            LONG_PTR btnType = (style & BS_TYPEMASK);
            if (btnType == BS_PUSHBUTTON || btnType == BS_DEFPUSHBUTTON || btnType == BS_OWNERDRAW) {
                SetWindowTheme(child, L"", L"");
                if (btnType != BS_OWNERDRAW) {
                    SetWindowLongPtrW(child, GWL_STYLE, (style & ~BS_TYPEMASK) | BS_OWNERDRAW);
                }
                subclassStyledButton(child);
            }
        }
        // For macOS: strip system-drawn borders from input controls
        if (macOS) {
            if (_wcsicmp(cls, L"EDIT") == 0 ||
                _wcsicmp(cls, L"LISTBOX") == 0 ||
                _wcsicmp(cls, L"COMBOBOX") == 0) {
                // Remove WS_EX_CLIENTEDGE if present
                LONG_PTR exStyle = GetWindowLongPtrW(child, GWL_EXSTYLE);
                if (exStyle & WS_EX_CLIENTEDGE) {
                    SetWindowLongPtrW(child, GWL_EXSTYLE, exStyle & ~WS_EX_CLIENTEDGE);
                    SetWindowPos(child, NULL, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
                // Strip theme so UxTheme doesn't draw its own border
                SetWindowTheme(child, L"", L"");
            }
        }
        return TRUE;
    }, (LPARAM)isMacOS);

    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

static LRESULT CALLBACK UIWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_NCCALCSIZE: {
        if (wParam == TRUE) {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            if (it != g_uiThemes.end() && it->second.hasCustomHeader) {
                return 0; // Removes Windows titlebar frame completely!
            }
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProcW(hwnd, msg, wParam, lParam);
        if (hit != HTCLIENT) return hit;

        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_uiThemes.find(hwnd);
        if (it != g_uiThemes.end() && it->second.hasCustomHeader) {
            POINT pt = { (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam) };
            ScreenToClient(hwnd, &pt);
            if (pt.y >= 0 && pt.y < it->second.headerHeight) {
                if (pt.x >= 10 && pt.x <= 75) {
                    return HTCLIENT; // Clickable traffic lights
                }
                return HTCAPTION; // Draggable titlebar
            }
        }
        return hit;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        UIThemeConfig theme;
        bool hasCustomHeader = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            if (it != g_uiThemes.end()) {
                theme = it->second;
                hasCustomHeader = theme.hasCustomHeader;
            }
        }
        if (hasCustomHeader) {
            paintMacHeader(hwnd, hdc, theme);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        UIThemeConfig theme;
        bool hasCustomHeader = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            if (it != g_uiThemes.end()) {
                theme = it->second;
                hasCustomHeader = theme.hasCustomHeader;
            }
        }
        if (hasCustomHeader) {
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);
            bool overTL = (x >= 10 && x <= 75 && y >= 6 && y <= 30);
            bool changed = false;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                if (overTL != g_trafficLightsHover[hwnd]) {
                    g_trafficLightsHover[hwnd] = overTL;
                    changed = true;
                }
            }
            if (changed) {
                RECT rcHeader = { 0, 0, 80, theme.headerHeight };
                InvalidateRect(hwnd, &rcHeader, FALSE);
            }
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
        }
        break;
    }

    case WM_MOUSELEAVE: {
        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            if (g_trafficLightsHover[hwnd]) {
                g_trafficLightsHover[hwnd] = false;
                changed = true;
            }
        }
        if (changed) {
            RECT rcHeader = { 0, 0, 80, 40 };
            InvalidateRect(hwnd, &rcHeader, FALSE);
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        UIThemeConfig theme;
        bool hasCustomHeader = false;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            if (it != g_uiThemes.end()) {
                theme = it->second;
                hasCustomHeader = theme.hasCustomHeader;
            }
        }
        if (hasCustomHeader) {
            int x = (int)(short)LOWORD(lParam);
            int y = (int)(short)HIWORD(lParam);
            int dRed = (x - 20)*(x - 20) + (y - 18)*(y - 18);
            int dYellow = (x - 40)*(x - 40) + (y - 18)*(y - 18);
            int dGreen = (x - 60)*(x - 60) + (y - 18)*(y - 18);
            if (dRed <= 64) {
                PostMessageW(hwnd, WM_CLOSE, 0, 0);
                return 0;
            } else if (dYellow <= 64) {
                ShowWindow(hwnd, SW_MINIMIZE);
                return 0;
            } else if (dGreen <= 64) {
                if (IsZoomed(hwnd)) ShowWindow(hwnd, SW_RESTORE);
                else ShowWindow(hwnd, SW_MAXIMIZE);
                return 0;
            }
        }
        break;
    }

    case WM_DRAWITEM:
        return TRUE;

    case WM_COMMAND: {
        HWND ctrlHwnd = (HWND)lParam;
        WORD notifyCode = HIWORD(wParam);
        if (ctrlHwnd && g_uiInterp) {
            TzdValue cb;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                auto it = g_uiControlEvents.find(ctrlHwnd);
                if (it != g_uiControlEvents.end()) {
                    if (notifyCode == BN_CLICKED) {
                        if (it->second.onClickCallback.type == TzdValue::FUNCTION || it->second.onClickCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onClickCallback;
                        } else if (it->second.onChangeCallback.type == TzdValue::FUNCTION || it->second.onChangeCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onChangeCallback;
                        }
                    } else if (notifyCode == EN_CHANGE || notifyCode == CBN_SELCHANGE || notifyCode == LBN_SELCHANGE) {
                        if (it->second.onChangeCallback.type == TzdValue::FUNCTION || it->second.onChangeCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onChangeCallback;
                        } else if (it->second.onClickCallback.type == TzdValue::FUNCTION || it->second.onClickCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onClickCallback;
                        }
                    }
                }
            }
            if (cb.type == TzdValue::FUNCTION || cb.type == TzdValue::NATIVE_FUNCTION) {
                invokeUiCallback(cb, { TzdValue((void*)ctrlHwnd) }, hwnd);
            }
        }
        return 0;
    }

    case WM_HSCROLL:
    case WM_VSCROLL: {
        HWND ctrlHwnd = (HWND)lParam;
        if (ctrlHwnd && g_uiInterp) {
            TzdValue cb;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                auto it = g_uiControlEvents.find(ctrlHwnd);
                if (it != g_uiControlEvents.end()) {
                    if (it->second.onChangeCallback.type == TzdValue::FUNCTION || it->second.onChangeCallback.type == TzdValue::NATIVE_FUNCTION) {
                        cb = it->second.onChangeCallback;
                    } else if (it->second.onClickCallback.type == TzdValue::FUNCTION || it->second.onClickCallback.type == TzdValue::NATIVE_FUNCTION) {
                        cb = it->second.onClickCallback;
                    }
                }
            }
            if (cb.type == TzdValue::FUNCTION || cb.type == TzdValue::NATIVE_FUNCTION) {
                invokeUiCallback(cb, { TzdValue((void*)ctrlHwnd) }, hwnd);
            }
        }
        return 0;
    }

    case WM_NOTIFY: {
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm && g_uiInterp) {
            TzdValue cb;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                auto it = g_uiControlEvents.find(pnm->hwndFrom);
                if (it != g_uiControlEvents.end()) {
                    if (pnm->code == TCN_SELCHANGE || pnm->code == DTN_DATETIMECHANGE) {
                        if (it->second.onChangeCallback.type == TzdValue::FUNCTION || it->second.onChangeCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onChangeCallback;
                        } else if (it->second.onClickCallback.type == TzdValue::FUNCTION || it->second.onClickCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onClickCallback;
                        }
                    } else if (pnm->code == NM_CLICK) {
                        if (it->second.onClickCallback.type == TzdValue::FUNCTION || it->second.onClickCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onClickCallback;
                        } else if (it->second.onChangeCallback.type == TzdValue::FUNCTION || it->second.onChangeCallback.type == TzdValue::NATIVE_FUNCTION) {
                            cb = it->second.onChangeCallback;
                        }
                    }
                }
            }
            if (cb.type == TzdValue::FUNCTION || cb.type == TzdValue::NATIVE_FUNCTION) {
                invokeUiCallback(cb, { TzdValue((void*)pnm->hwndFrom) }, hwnd);
            }
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_uiThemes.find(hwnd);
        const UIThemeConfig& theme = (it != g_uiThemes.end()) ? it->second : g_defaultTheme;
        SetTextColor(hdcStatic, theme.fg);
        SetBkColor(hdcStatic, theme.bg);
        SetBkMode(hdcStatic, TRANSPARENT);
        auto bit = g_uiWindowBrushes.find(hwnd);
        if (bit != g_uiWindowBrushes.end() && bit->second) return (INT_PTR)bit->second;
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_uiThemes.find(hwnd);
        const UIThemeConfig& theme = (it != g_uiThemes.end()) ? it->second : g_defaultTheme;
        SetTextColor(hdcEdit, theme.inputFg);
        SetBkColor(hdcEdit, theme.inputBg);
        auto bit = g_uiInputBrushes.find(hwnd);
        if (bit != g_uiInputBrushes.end() && bit->second) return (INT_PTR)bit->second;
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdcLb = (HDC)wParam;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_uiThemes.find(hwnd);
        const UIThemeConfig& theme = (it != g_uiThemes.end()) ? it->second : g_defaultTheme;
        SetTextColor(hdcLb, theme.inputFg);
        SetBkColor(hdcLb, theme.inputBg);
        auto bit = g_uiInputBrushes.find(hwnd);
        if (bit != g_uiInputBrushes.end() && bit->second) return (INT_PTR)bit->second;
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }

    case WM_SIZE: {
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            if (it != g_uiThemes.end() && it->second.hasCustomHeader) {
                RECT rcHeader = { 0, 0, (int)LOWORD(lParam), it->second.headerHeight };
                InvalidateRect(hwnd, &rcHeader, TRUE);
            }
        }
        if (g_uiInterp) {
            TzdValue cb;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                auto it = g_uiWindowEvents.find(hwnd);
                if (it != g_uiWindowEvents.end()) cb = it->second.onResizeCallback;
            }
            if (cb.type == TzdValue::FUNCTION || cb.type == TzdValue::NATIVE_FUNCTION) {
                invokeUiCallback(cb, { TzdValue((double)LOWORD(lParam)), TzdValue((double)HIWORD(lParam)) }, hwnd);
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);
        EnumChildWindows(hwnd, [](HWND child, LPARAM) -> BOOL {
            InvalidateRect(child, NULL, TRUE);
            return TRUE;
        }, 0);
        return 0;
    }

    case WM_CLOSE: {
        bool cancelClose = false;
        if (g_uiInterp) {
            TzdValue cb;
            {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                auto it = g_uiWindowEvents.find(hwnd);
                if (it != g_uiWindowEvents.end()) {
                    it->second.isClosed = true;
                    cb = it->second.onCloseCallback;
                }
            }
            if (cb.type == TzdValue::FUNCTION || cb.type == TzdValue::NATIVE_FUNCTION) {
                TzdValue res = invokeUiCallback(cb, { TzdValue((void*)hwnd) }, hwnd);
                if (res.type == TzdValue::BOOL && !res.bVal) cancelClose = true;
            }
        }
        if (!cancelClose) {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);

        UIThemeConfig theme;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            theme = (it != g_uiThemes.end()) ? it->second : g_defaultTheme;
        }

        // macOS: subtle gradient background (top slightly lighter)
        if (theme.style == UIStyleKind::MACOS) {
            Gdiplus::Graphics gfx(hdc);
            COLORREF bg = theme.bg;
            bool isDark = (theme.mode == UIThemeMode::DARK);
            int lift = isDark ? 4 : 6;
            BYTE tr = (BYTE)std::min(255, (int)GetRValue(bg) + lift);
            BYTE tg2 = (BYTE)std::min(255, (int)GetGValue(bg) + lift);
            BYTE tb = (BYTE)std::min(255, (int)GetBValue(bg) + lift);
            Gdiplus::LinearGradientBrush grad(
                Gdiplus::PointF(0.f, 0.f),
                Gdiplus::PointF(0.f, (float)(rc.bottom - rc.top)),
                Gdiplus::Color(255, tr, tg2, tb),
                Gdiplus::Color(255, GetRValue(bg), GetGValue(bg), GetBValue(bg))
            );
            gfx.FillRectangle(&grad, Gdiplus::RectF((float)rc.left, (float)rc.top, (float)(rc.right - rc.left), (float)(rc.bottom - rc.top)));
            return 1;
        }

        // Non-macOS: fill solid
        HBRUSH hbr = NULL;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiWindowBrushes.find(hwnd);
            if (it != g_uiWindowBrushes.end() && it->second) hbr = it->second;
        }
        if (hbr) {
            FillRect(hdc, &rc, hbr);
            return 1;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    case WM_DESTROY: {
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiWindowEvents.find(hwnd);
            if (it != g_uiWindowEvents.end()) it->second.isClosed = true;
            auto bit = g_uiWindowBrushes.find(hwnd);
            if (bit != g_uiWindowBrushes.end() && bit->second) {
                DeleteObject(bit->second);
                g_uiWindowBrushes.erase(bit);
            }
            auto ibit = g_uiInputBrushes.find(hwnd);
            if (ibit != g_uiInputBrushes.end() && ibit->second) {
                DeleteObject(ibit->second);
                g_uiInputBrushes.erase(ibit);
            }
            g_uiThemes.erase(hwnd);
            g_uiAppliedHeaderOffset.erase(hwnd);
        }
        PostQuitMessage(0);
        return 0;
    }

    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// In-memory Prompt Dialog Procedure
struct PromptDialogData {
    std::wstring title;
    std::wstring message;
    std::wstring defaultVal;
    std::wstring resultText;
    bool isOk = false;
};

static LRESULT CALLBACK PromptWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PromptDialogData* data = (PromptDialogData*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        data = (PromptDialogData*)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)data);

        HWND hLabel = CreateWindowExW(0, L"STATIC", data->message.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      20, 15, 360, 40, hwnd, NULL, cs->hInstance, NULL);
        applyModernFont(hLabel);

        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", data->defaultVal.c_str(),
                                     WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                     20, 60, 360, 26, hwnd, (HMENU)101, cs->hInstance, NULL);
        applyModernFont(hEdit);
        SetFocus(hEdit);
        SendMessageW(hEdit, EM_SETSEL, 0, -1);

        HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                      190, 100, 90, 30, hwnd, (HMENU)IDOK, cs->hInstance, NULL);
        applyModernFont(hBtnOk);

        HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                          290, 100, 90, 30, hwnd, (HMENU)IDCANCEL, cs->hInstance, NULL);
        applyModernFont(hBtnCancel);
        return 0;
    }
    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id == IDOK) {
            HWND hEdit = GetDlgItem(hwnd, 101);
            int len = GetWindowTextLengthW(hEdit);
            std::vector<wchar_t> buf(len + 1, 0);
            GetWindowTextW(hEdit, buf.data(), len + 1);
            if (data) {
                data->resultText = buf.data();
                data->isOk = true;
            }
            DestroyWindow(hwnd);
        } else if (id == IDCANCEL) {
            if (data) data->isOk = false;
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        if (data) data->isOk = false;
        DestroyWindow(hwnd);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

static LRESULT CALLBACK ImageViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Gdiplus::Bitmap* bmp = nullptr;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiImageViews.find(hwnd);
            if (it != g_uiImageViews.end()) bmp = it->second;
        }
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w > 0 && h > 0) {
            Gdiplus::Graphics g(hdc);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            if (bmp) {
                g.DrawImage(bmp, 0, 0, w, h);
                // Subtle 1px clean border around 3D viewport
                Gdiplus::Pen pen(Gdiplus::Color(60, 128, 128, 128), 1.0f);
                g.DrawRectangle(&pen, 0, 0, w - 1, h - 1);
            }
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY: {
        std::lock_guard<std::mutex> lock(g_uiMutex);
        auto it = g_uiImageViews.find(hwnd);
        if (it != g_uiImageViews.end()) {
            if (it->second) delete it->second;
            g_uiImageViews.erase(it);
        }
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void TzdUiModule::init(TzdInterpreter* interp) {
    EnableVisualStyles();
    g_uiInterp = interp;

    if (!g_gdiplusToken) {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    }

    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS | ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_DATE_CLASSES | ICC_LINK_CLASS | ICC_USEREX_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icc);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    if (!g_uiClassRegistered) {
        WNDCLASSEXW wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = UIWndProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = L"TzdUIWindowClass";
        RegisterClassExW(&wc);

        WNDCLASSEXW pwc;
        ZeroMemory(&pwc, sizeof(pwc));
        pwc.cbSize = sizeof(pwc);
        pwc.style = CS_HREDRAW | CS_VREDRAW;
        pwc.lpfnWndProc = PromptWndProc;
        pwc.hInstance = hInstance;
        pwc.hCursor = LoadCursor(NULL, IDC_ARROW);
        pwc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        pwc.lpszClassName = L"TzdPromptWindowClass";
        RegisterClassExW(&pwc);

        WNDCLASSEXW iwc;
        ZeroMemory(&iwc, sizeof(iwc));
        iwc.cbSize = sizeof(iwc);
        iwc.style = CS_HREDRAW | CS_VREDRAW;
        iwc.lpfnWndProc = ImageViewWndProc;
        iwc.hInstance = hInstance;
        iwc.hCursor = LoadCursor(NULL, IDC_ARROW);
        iwc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        iwc.lpszClassName = L"TzdImageViewClass";
        RegisterClassExW(&iwc);

        g_uiClassRegistered = true;
    }

    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f);
        v.name = name;
        interp->setGlobalVariable(name, v);
    };

    // -------------------------------------------------------------
    // System Dialogs
    // -------------------------------------------------------------
    reg("ui_alert", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "Alert";
        std::string msg = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "";
        std::string iconType = (args.size() > 2) ? TzdInterpreter::getAsString(args[2]) : "info";

        UINT flags = MB_OK;
        if (iconType == "error") flags |= MB_ICONERROR;
        else if (iconType == "warn" || iconType == "warning") flags |= MB_ICONWARNING;
        else flags |= MB_ICONINFORMATION;

        MessageBoxW(NULL, utf8ToWide(msg).c_str(), utf8ToWide(title).c_str(), flags);
        return TzdValue(true);
    });

    reg("ui_confirm", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "Confirm";
        std::string msg = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "";
        int res = MessageBoxW(NULL, utf8ToWide(msg).c_str(), utf8ToWide(title).c_str(), MB_YESNO | MB_ICONQUESTION);
        return TzdValue(res == IDYES);
    });

    reg("ui_prompt", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "Prompt";
        std::string msg = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "Enter value:";
        std::string defaultVal = (args.size() > 2) ? TzdInterpreter::getAsString(args[2]) : "";

        PromptDialogData data;
        data.title = utf8ToWide(title);
        data.message = utf8ToWide(msg);
        data.defaultVal = utf8ToWide(defaultVal);

        HINSTANCE hInst = GetModuleHandle(NULL);
        HWND hwnd = CreateWindowExW(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            L"TzdPromptWindowClass",
            data.title.c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 415, 180,
            NULL, NULL, hInst, &data
        );

        // Center on screen
        RECT rc;
        GetWindowRect(hwnd, &rc);
        int scrW = GetSystemMetrics(SM_CXSCREEN);
        int scrH = GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(hwnd, HWND_TOP, (scrW - (rc.right - rc.left)) / 2, (scrH - (rc.bottom - rc.top)) / 2, 0, 0, SWP_NOSIZE);

        MSG m;
        while (IsWindow(hwnd) && GetMessageW(&m, NULL, 0, 0)) {
            if (!IsDialogMessageW(hwnd, &m)) {
                TranslateMessage(&m);
                DispatchMessageW(&m);
            }
        }

        if (data.isOk) {
            return TzdValue(wideToUtf8(data.resultText));
        }
        return TzdValue(); // null
    });

    reg("ui_open_file", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "Open File";
        std::string filter = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "All Files (*.*)\0*.*\0";
        std::string defaultDir = (args.size() > 2) ? TzdInterpreter::getAsString(args[2]) : "";

        wchar_t filename[1024] = {0};
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = sizeof(filename) / sizeof(wchar_t);

        // Build filter with double null terminators
        std::wstring wFilter = utf8ToWide(filter);
        if (wFilter.empty() || wFilter.back() != L'\0') {
            wFilter.push_back(L'\0');
            wFilter.push_back(L'\0');
        }
        ofn.lpstrFilter = wFilter.c_str();
        ofn.nFilterIndex = 1;
        std::wstring wTitle = utf8ToWide(title);
        ofn.lpstrTitle = wTitle.c_str();
        std::wstring wDefDir = utf8ToWide(defaultDir);
        if (!defaultDir.empty()) ofn.lpstrInitialDir = wDefDir.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&ofn)) {
            return TzdValue(wideToUtf8(filename));
        }
        return TzdValue("");
    });

    reg("ui_save_file", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "Save File";
        std::string filter = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "All Files (*.*)\0*.*\0";
        std::string defaultName = (args.size() > 2) ? TzdInterpreter::getAsString(args[2]) : "";
        std::string defaultDir = (args.size() > 3) ? TzdInterpreter::getAsString(args[3]) : "";

        wchar_t filename[1024] = {0};
        std::wstring wDefName = utf8ToWide(defaultName);
        if (!wDefName.empty()) {
            wcsncpy_s(filename, wDefName.c_str(), sizeof(filename)/sizeof(wchar_t) - 1);
        }

        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = sizeof(filename) / sizeof(wchar_t);

        std::wstring wFilter = utf8ToWide(filter);
        if (wFilter.empty() || wFilter.back() != L'\0') {
            wFilter.push_back(L'\0');
            wFilter.push_back(L'\0');
        }
        ofn.lpstrFilter = wFilter.c_str();
        ofn.nFilterIndex = 1;
        std::wstring wTitle = utf8ToWide(title);
        ofn.lpstrTitle = wTitle.c_str();
        std::wstring wDefDir = utf8ToWide(defaultDir);
        if (!defaultDir.empty()) ofn.lpstrInitialDir = wDefDir.c_str();
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

        if (GetSaveFileNameW(&ofn)) {
            return TzdValue(wideToUtf8(filename));
        }
        return TzdValue("");
    });

    reg("ui_choose_folder", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "Choose Folder";
        BROWSEINFOW bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.hwndOwner = NULL;
        std::wstring wTitle = utf8ToWide(title);
        bi.lpszTitle = wTitle.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if (pidl != NULL) {
            wchar_t path[MAX_PATH] = {0};
            SHGetPathFromIDListW(pidl, path);
            CoTaskMemFree(pidl);
            return TzdValue(wideToUtf8(path));
        }
        return TzdValue("");
    });

    // -------------------------------------------------------------
    // Windows & Controls API
    // -------------------------------------------------------------
    reg("ui_create_window", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string title = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "TzdLang Window";
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : CW_USEDEFAULT;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : CW_USEDEFAULT;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 640;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 480;

        HINSTANCE hInstance = GetModuleHandle(NULL);
        std::wstring wTitle = utf8ToWide(title);
        HWND hwnd = CreateWindowExW(
            WS_EX_APPWINDOW,
            L"TzdUIWindowClass",
            wTitle.c_str(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, hInstance, NULL
        );

        if (!hwnd) return TzdValue();

        // 无论父进程是否以 SW_HIDE / windowsHide 模式启动，均强制将窗口正常显示
        ShowWindow(hwnd, SW_SHOW);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
        SetForegroundWindow(hwnd);

        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_uiWindowEvents[hwnd] = UIWindowEvent();
            g_uiThemes[hwnd] = g_defaultTheme;
            g_uiAppliedHeaderOffset[hwnd] = 0;
        }

        applyWindowTheme(hwnd);

        return TzdValue((void*)hwnd);
    });

    reg("ui_show_window", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        bool show = (args.size() > 1) ? args[1].bVal : true;
        ShowWindow(hwnd, show ? SW_SHOW : SW_HIDE);
        return TzdValue(true);
    });

    reg("ui_maximize_window", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        ShowWindow(hwnd, SW_MAXIMIZE);
        return TzdValue(true);
    });

    reg("ui_restore_window", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        ShowWindow(hwnd, SW_RESTORE);
        return TzdValue(true);
    });

    reg("ui_close_window", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        DestroyWindow(hwnd);
        return TzdValue(true);
    });

    reg("ui_set_window_title", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string title = TzdInterpreter::getAsString(args[1]);
        SetWindowTextW(hwnd, utf8ToWide(title).c_str());
        RECT rc = { 0, 0, 10000, 50 };
        InvalidateRect(hwnd, &rc, TRUE);
        return TzdValue(true);
    });

    reg("ui_get_window_title", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue("");
        HWND hwnd = (HWND)args[0].ptrVal;
        int len = GetWindowTextLengthW(hwnd);
        std::vector<wchar_t> buf(len + 1, 0);
        GetWindowTextW(hwnd, buf.data(), len + 1);
        return TzdValue(wideToUtf8(buf.data()));
    });

    reg("ui_set_dark_mode", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        BOOL darkMode = args[1].bVal ? TRUE : FALSE;
        UIThemeConfig theme;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            theme = (it != g_uiThemes.end()) ? it->second : g_defaultTheme;
        }
        theme.mode = darkMode ? UIThemeMode::DARK : UIThemeMode::LIGHT;
        setupThemePalette(theme);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_uiThemes[hwnd] = theme;
        }
        applyWindowTheme(hwnd);
        return TzdValue(true);
    });

    reg("ui_set_style", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string styleName = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "windows";

        UIThemeConfig theme;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            theme = (it != g_uiThemes.end()) ? it->second : g_defaultTheme;
        }

        std::string lowerStyle = styleName;
        for (char& c : lowerStyle) c = (char)tolower(c);

        if (lowerStyle == "macos" || lowerStyle == "mac" || lowerStyle == "apple") {
            theme.style = UIStyleKind::MACOS;
        } else if (lowerStyle == "windows" || lowerStyle == "win" || lowerStyle == "fluent") {
            theme.style = UIStyleKind::WINDOWS;
        } else if (lowerStyle == "custom") {
            theme.style = UIStyleKind::CUSTOM;
        }

        setupThemePalette(theme);

        const TzdValue* mapArg = nullptr;
        if (args.size() > 2 && args[2].type == TzdValue::MAP) {
            mapArg = &args[2];
        } else if (args.size() > 1 && args[1].type == TzdValue::MAP) {
            mapArg = &args[1];
            theme.style = UIStyleKind::CUSTOM;
        }

        if (mapArg) {
            const auto& m = mapArg->mapVal;
            theme.bg = getMapColor(m, "backgroundColor", getMapColor(m, "bg", theme.bg));
            theme.fg = getMapColor(m, "textColor", getMapColor(m, "fg", theme.fg));
            theme.accent = getMapColor(m, "accentColor", getMapColor(m, "accent", theme.accent));

            theme.buttonBg = getMapColor(m, "buttonBg", getMapColor(m, "buttonBackground", theme.buttonBg));
            theme.buttonFg = getMapColor(m, "buttonText", getMapColor(m, "buttonFg", theme.buttonFg));
            theme.buttonHoverBg = getMapColor(m, "buttonHover", getMapColor(m, "buttonHoverBg", theme.buttonHoverBg));
            theme.buttonPressedBg = getMapColor(m, "buttonPressed", getMapColor(m, "buttonPressedBg", theme.buttonPressedBg));
            theme.buttonBorder = getMapColor(m, "buttonBorder", theme.buttonBorder);
            theme.buttonRadius = (int)getMapNumber(m, "buttonRadius", (double)theme.buttonRadius);

            theme.inputBg = getMapColor(m, "inputBg", theme.inputBg);
            theme.inputFg = getMapColor(m, "inputFg", theme.inputFg);
            theme.inputBorder = getMapColor(m, "inputBorder", theme.inputBorder);

            theme.headerBg = getMapColor(m, "headerBg", theme.headerBg);
            theme.headerFg = getMapColor(m, "headerFg", theme.headerFg);
            theme.headerBorder = getMapColor(m, "headerBorder", theme.headerBorder);

            auto itTb = m.find("titlebar");
            if (itTb != m.end() && itTb->second.type == TzdValue::STRING) {
                std::string tb = itTb->second.sVal;
                if (tb == "macos" || tb == "mac") {
                    theme.hasCustomHeader = true;
                    theme.hasTrafficLights = true;
                } else if (tb == "windows" || tb == "native") {
                    theme.hasCustomHeader = false;
                    theme.hasTrafficLights = false;
                }
            }

            auto itTL = m.find("trafficLights");
            if (itTL != m.end() && itTL->second.type == TzdValue::BOOL) {
                theme.hasTrafficLights = itTL->second.bVal;
                theme.hasCustomHeader = itTL->second.bVal;
            }

            auto itFont = m.find("fontFamily");
            if (itFont != m.end() && itFont->second.type == TzdValue::STRING) {
                theme.fontFamily = utf8ToWide(itFont->second.sVal);
            }

            auto itFontSize = m.find("fontSize");
            if (itFontSize != m.end() && (itFontSize->second.type == TzdValue::INT || itFontSize->second.type == TzdValue::DOUBLE || itFontSize->second.type == TzdValue::LONG)) {
                theme.fontSize = (int)TzdInterpreter::getAsDoubleInternal(itFontSize->second);
            }
        }

        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_uiThemes[hwnd] = theme;
        }
        applyWindowTheme(hwnd);
        return TzdValue(true);
    });

    reg("ui_set_theme", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string modeStr = TzdInterpreter::getAsString(args[1]);
        std::string lowerMode = modeStr;
        for (char& c : lowerMode) c = (char)tolower(c);

        UIThemeConfig theme;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            theme = (it != g_uiThemes.end()) ? it->second : g_defaultTheme;
        }

        if (lowerMode == "dark") {
            theme.mode = UIThemeMode::DARK;
        } else if (lowerMode == "monochrome" || lowerMode == "bw" || lowerMode == "blackwhite" || lowerMode == "highcontrast") {
            theme.mode = UIThemeMode::MONOCHROME;
        } else {
            theme.mode = UIThemeMode::LIGHT;
        }

        setupThemePalette(theme);

        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_uiThemes[hwnd] = theme;
        }
        applyWindowTheme(hwnd);
        return TzdValue(true);
    });

    reg("ui_set_global_style", [](const std::vector<TzdValue>& args) -> TzdValue {
        std::string styleName = (args.size() > 0) ? TzdInterpreter::getAsString(args[0]) : "windows";
        std::string lowerStyle = styleName;
        for (char& c : lowerStyle) c = (char)tolower(c);

        if (lowerStyle == "macos" || lowerStyle == "mac" || lowerStyle == "apple") {
            g_defaultTheme.style = UIStyleKind::MACOS;
        } else if (lowerStyle == "windows" || lowerStyle == "win") {
            g_defaultTheme.style = UIStyleKind::WINDOWS;
        } else if (lowerStyle == "custom") {
            g_defaultTheme.style = UIStyleKind::CUSTOM;
        }
        setupThemePalette(g_defaultTheme);
        return TzdValue(true);
    });

    reg("ui_set_global_theme", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        std::string modeStr = TzdInterpreter::getAsString(args[0]);
        std::string lowerMode = modeStr;
        for (char& c : lowerMode) c = (char)tolower(c);

        if (lowerMode == "dark") {
            g_defaultTheme.mode = UIThemeMode::DARK;
        } else if (lowerMode == "monochrome" || lowerMode == "bw" || lowerMode == "blackwhite" || lowerMode == "highcontrast") {
            g_defaultTheme.mode = UIThemeMode::MONOCHROME;
        } else {
            g_defaultTheme.mode = UIThemeMode::LIGHT;
        }
        setupThemePalette(g_defaultTheme);
        return TzdValue(true);
    });

    reg("ui_set_control_primary", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        bool isPrimary = (args.size() > 1) ? (args[1].type == TzdValue::BOOL ? args[1].bVal : (TzdInterpreter::getAsDoubleInternal(args[1]) != 0)) : true;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_controlPrimary[hwnd] = isPrimary;
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return TzdValue(true);
    });

    reg("ui_set_accent_color", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 4 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int r = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int g = (int)TzdInterpreter::getAsDoubleInternal(args[2]);
        int b = (int)TzdInterpreter::getAsDoubleInternal(args[3]);
        COLORREF cr = RGB(r, g, b);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            if (it != g_uiThemes.end()) {
                it->second.accent = cr;
            }
        }
        applyWindowTheme(hwnd);
        return TzdValue(true);
    });

    reg("ui_set_window_bg", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 4 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int r = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int g = (int)TzdInterpreter::getAsDoubleInternal(args[2]);
        int b = (int)TzdInterpreter::getAsDoubleInternal(args[3]);
        COLORREF cr = RGB(r, g, b);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(hwnd);
            if (it != g_uiThemes.end()) it->second.bg = cr;
            g_uiWindowBgColors[hwnd] = cr;
            if (g_uiWindowBrushes.count(hwnd) && g_uiWindowBrushes[hwnd]) {
                DeleteObject(g_uiWindowBrushes[hwnd]);
            }
            g_uiWindowBrushes[hwnd] = CreateSolidBrush(cr);
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return TzdValue(true);
    });

    reg("ui_choose_color", [](const std::vector<TzdValue>& args) -> TzdValue {
        HWND parent = (args.size() > 0 && args[0].type == TzdValue::POINTER) ? (HWND)args[0].ptrVal : NULL;
        CHOOSECOLORW cc;
        static COLORREF customColors[16] = { 0 };
        ZeroMemory(&cc, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = parent;
        cc.lpCustColors = customColors;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColorW(&cc)) {
            int r = GetRValue(cc.rgbResult);
            int g = GetGValue(cc.rgbResult);
            int b = GetBValue(cc.rgbResult);
            std::vector<TzdValue> res = { TzdValue((double)r), TzdValue((double)g), TzdValue((double)b) };
            return TzdValue(res);
        }
        return TzdValue();
    });

    reg("ui_save_screenshot", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string path = TzdInterpreter::getAsString(args[1]);
        if (path.empty()) return TzdValue(false);

        std::filesystem::path p(utf8ToWide(path));
        if (p.is_relative()) {
            std::filesystem::path cur = std::filesystem::current_path();
            if (!std::filesystem::exists(cur / p.parent_path()) && std::filesystem::exists(cur.parent_path() / p.parent_path())) {
                p = cur.parent_path() / p;
            } else {
                p = cur / p;
            }
        }
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
        }

        RECT rcWin;
        GetWindowRect(hwnd, &rcWin);
        int w = rcWin.right - rcWin.left;
        int h = rcWin.bottom - rcWin.top;
        if (w <= 0 || h <= 0) return TzdValue(false);

        HDC hdcScreen = GetDC(NULL);
        HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
        HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, w, h);
        HGDIOBJ hbmOld = SelectObject(hdcMemDC, hbmScreen);

        UpdateWindow(hwnd);
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

        BOOL captured = PrintWindow(hwnd, hdcMemDC, 2 /* PW_RENDERFULLCONTENT */);
        if (!captured) {
            captured = PrintWindow(hwnd, hdcMemDC, 0);
        }
        if (!captured && IsWindowVisible(hwnd)) {
            captured = BitBlt(hdcMemDC, 0, 0, w, h, hdcScreen, rcWin.left, rcWin.top, SRCCOPY);
        }
        if (!captured) {
            captured = PrintWindow(hwnd, hdcMemDC, 0);
        }
        if (!captured) {
            SendMessageW(hwnd, WM_PRINT, (WPARAM)hdcMemDC, PRF_NONCLIENT | PRF_CLIENT | PRF_CHILDREN | PRF_ERASEBKGND);
        }

        Gdiplus::Bitmap bitmap(hbmScreen, NULL);
        CLSID pngClsid;
        Gdiplus::Status st = Gdiplus::GenericError;
        if (GetEncoderClsid(L"image/png", &pngClsid) >= 0) {
            st = bitmap.Save(p.c_str(), &pngClsid, NULL);
        }

        SelectObject(hdcMemDC, hbmOld);
        DeleteObject(hbmScreen);
        DeleteDC(hdcMemDC);
        ReleaseDC(NULL, hdcScreen);

        return TzdValue(st == Gdiplus::Ok);
    });

    reg("ui_set_button_anim", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 3 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        float hover = (float)TzdInterpreter::getAsDoubleInternal(args[1]);
        bool pressed = (args[2].type == TzdValue::BOOL) ? args[2].bVal : (TzdInterpreter::getAsDoubleInternal(args[2]) != 0);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto& st = g_buttonAnim[hwnd];
            st.hoverAlpha = hover;
            st.targetHover = hover;
            st.isPressed = pressed;
            g_buttonPressedState[hwnd] = pressed;
            g_buttonHoverState[hwnd] = (hover > 0.0f);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        return TzdValue(true);
    });

    reg("ui_send_message", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 4 || args[0].type != TzdValue::POINTER) return TzdValue(0.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        UINT msg = (UINT)TzdInterpreter::getAsDoubleInternal(args[1]);
        WPARAM wp = (WPARAM)TzdInterpreter::getAsDoubleInternal(args[2]);
        LPARAM lp = (LPARAM)TzdInterpreter::getAsDoubleInternal(args[3]);
        LRESULT res = SendMessageW(hwnd, msg, wp, lp);
        return TzdValue((double)res);
    });

    reg("ui_create_button", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        std::string text = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "Button";
        int x = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int y = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 10;
        int w = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 100;
        int h = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 30;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        std::wstring wText = utf8ToWide(text);
        HWND btn = CreateWindowExW(
            0, L"BUTTON", wText.c_str(),
            WS_TABSTOP | WS_CHILD | BS_OWNERDRAW,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        SetWindowTheme(btn, L"", L"");
        applyModernFont(btn);
        subclassStyledButton(btn);
        ShowWindow(btn, SW_SHOW);
        return TzdValue((void*)btn);
    });

    reg("ui_create_label", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        std::string text = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "Label";
        int x = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int y = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 10;
        int w = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 100;
        int h = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 24;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        std::wstring wText = utf8ToWide(text);
        HWND lbl = CreateWindowExW(
            0, L"STATIC", wText.c_str(),
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(lbl);
        return TzdValue((void*)lbl);
    });

    reg("ui_create_textbox", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        std::string text = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "";
        int x = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int y = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 10;
        int w = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 200;
        int h = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 26;
        bool isMulti = (args.size() > 6) ? args[6].bVal : false;

        int effY = getEffectiveControlY(parent, y);
        DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | (isMulti ? (ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN) : ES_AUTOHSCROLL);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        std::wstring wText = utf8ToWide(text);
        HWND edit = CreateWindowExW(
            0, L"EDIT", wText.c_str(),
            style,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(edit);
        return TzdValue((void*)edit);
    });

    reg("ui_create_checkbox", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        std::string text = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "CheckBox";
        int x = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int y = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 10;
        int w = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 120;
        int h = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 24;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        std::wstring wText = utf8ToWide(text);
        HWND chk = CreateWindowExW(
            0, L"BUTTON", wText.c_str(),
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(chk);
        return TzdValue((void*)chk);
    });

    reg("ui_set_text", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string text = TzdInterpreter::getAsString(args[1]);
        SetWindowTextW(hwnd, utf8ToWide(text).c_str());
        InvalidateRect(hwnd, NULL, TRUE);
        return TzdValue(true);
    });

    reg("ui_get_text", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue("");
        HWND hwnd = (HWND)args[0].ptrVal;
        int len = GetWindowTextLengthW(hwnd);
        std::vector<wchar_t> buf(len + 1, 0);
        GetWindowTextW(hwnd, buf.data(), len + 1);
        return TzdValue(wideToUtf8(buf.data()));
    });

    reg("ui_set_checked", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        bool checked = args[1].bVal;
        SendMessageW(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        return TzdValue(true);
    });

    reg("ui_get_checked", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT res = SendMessageW(hwnd, BM_GETCHECK, 0, 0);
        return TzdValue(res == BST_CHECKED);
    });

    reg("ui_set_enabled", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        EnableWindow(hwnd, args[1].bVal);
        InvalidateRect(hwnd, NULL, TRUE);
        return TzdValue(true);
    });

    reg("ui_set_bounds", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 5 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int x = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int y = (int)TzdInterpreter::getAsDoubleInternal(args[2]);
        int w = (int)TzdInterpreter::getAsDoubleInternal(args[3]);
        int h = (int)TzdInterpreter::getAsDoubleInternal(args[4]);
        int effY = getEffectiveControlY(GetParent(hwnd), y);
        MoveWindow(hwnd, x, effY, w, h, TRUE);
        return TzdValue(true);
    });

    reg("ui_get_bounds", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND hwnd = (HWND)args[0].ptrVal;
        RECT rc;
        GetWindowRect(hwnd, &rc);
        HWND parent = GetParent(hwnd);
        if (parent) {
            POINT ptTL = { rc.left, rc.top };
            POINT ptBR = { rc.right, rc.bottom };
            ScreenToClient(parent, &ptTL);
            ScreenToClient(parent, &ptBR);
            rc.left = ptTL.x;
            rc.top = ptTL.y;
            rc.right = ptBR.x;
            rc.bottom = ptBR.y;

            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiThemes.find(parent);
            if (it != g_uiThemes.end() && it->second.hasCustomHeader) {
                rc.top -= it->second.headerHeight;
                rc.bottom -= it->second.headerHeight;
            }
        }
        std::vector<TzdValue> res = {
            TzdValue((double)rc.left),
            TzdValue((double)rc.top),
            TzdValue((double)(rc.right - rc.left)),
            TzdValue((double)(rc.bottom - rc.top))
        };
        return TzdValue(res);
    });

    reg("ui_set_visible", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        ShowWindow(hwnd, args[1].bVal ? SW_SHOW : SW_HIDE);
        return TzdValue(true);
    });

    reg("ui_get_visible", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        return TzdValue((bool)IsWindowVisible(hwnd));
    });

    reg("ui_set_readonly", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        SendMessageW(hwnd, EM_SETREADONLY, args[1].bVal ? TRUE : FALSE, 0);
        return TzdValue(true);
    });

    reg("ui_set_window_size", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 3 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int w = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int h = (int)TzdInterpreter::getAsDoubleInternal(args[2]);
        SetWindowPos(hwnd, NULL, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER);
        return TzdValue(true);
    });

    reg("ui_get_window_size", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND hwnd = (HWND)args[0].ptrVal;
        RECT rc;
        GetWindowRect(hwnd, &rc);
        std::vector<TzdValue> res = { TzdValue((double)(rc.right - rc.left)), TzdValue((double)(rc.bottom - rc.top)) };
        return TzdValue(res);
    });

    reg("ui_set_window_pos", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 3 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int x = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int y = (int)TzdInterpreter::getAsDoubleInternal(args[2]);
        SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        return TzdValue(true);
    });

    reg("ui_get_window_pos", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND hwnd = (HWND)args[0].ptrVal;
        RECT rc;
        GetWindowRect(hwnd, &rc);
        std::vector<TzdValue> res = { TzdValue((double)rc.left), TzdValue((double)rc.top) };
        return TzdValue(res);
    });

    // -------------------------------------------------------------
    // Extended Controls: RadioButton, GroupBox, ProgressBar, Slider, ComboBox, ListBox
    // -------------------------------------------------------------
    reg("ui_create_radio", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        std::string text = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "RadioButton";
        int x = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int y = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 10;
        int w = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 120;
        int h = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 24;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        std::wstring wText = utf8ToWide(text);
        HWND radio = CreateWindowExW(
            0, L"BUTTON", wText.c_str(),
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(radio);
        return TzdValue((void*)radio);
    });

    reg("ui_create_groupbox", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        std::string text = (args.size() > 1) ? TzdInterpreter::getAsString(args[1]) : "GroupBox";
        int x = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int y = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 10;
        int w = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 200;
        int h = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 150;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        std::wstring wText = utf8ToWide(text);
        HWND gb = CreateWindowExW(
            0, L"BUTTON", wText.c_str(),
            WS_VISIBLE | WS_CHILD | BS_GROUPBOX | WS_CLIPSIBLINGS,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(gb);
        WNDPROC old = (WNDPROC)SetWindowLongPtrW(gb, GWLP_WNDPROC, (LONG_PTR)GroupBoxWndProc);
        SetPropW(gb, L"TzdOldProc", (HANDLE)old);
        SetWindowPos(gb, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        return TzdValue((void*)gb);
    });

    reg("ui_perform_click", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        SendMessageW(hwnd, BM_CLICK, 0, 0);
        return TzdValue(true);
    });

    reg("ui_create_progressbar", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 10;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 200;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 24;
        int minVal = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 0;
        int maxVal = (args.size() > 6) ? (int)TzdInterpreter::getAsDoubleInternal(args[6]) : 100;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND pb = CreateWindowExW(
            0, PROGRESS_CLASSW, L"",
            WS_VISIBLE | WS_CHILD,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        SendMessageW(pb, PBM_SETRANGE32, (WPARAM)minVal, (LPARAM)maxVal);
        SendMessageW(pb, PBM_SETPOS, (WPARAM)minVal, 0);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_uiProgressValues[pb] = minVal;
        }
        return TzdValue((void*)pb);
    });

    reg("ui_set_progress", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int pos = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            g_uiProgressValues[hwnd] = pos;
        }
        SendMessageW(hwnd, PBM_SETPOS, (WPARAM)pos, 0);
        return TzdValue(true);
    });

    reg("ui_get_progress", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(0.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiProgressValues.find(hwnd);
            if (it != g_uiProgressValues.end()) {
                return TzdValue((double)it->second);
            }
        }
        LRESULT pos = SendMessageW(hwnd, PBM_GETPOS, 0, 0);
        return TzdValue((double)pos);
    });

    reg("ui_set_progress_range", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 3 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int minVal = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int maxVal = (int)TzdInterpreter::getAsDoubleInternal(args[2]);
        SendMessageW(hwnd, PBM_SETRANGE32, (WPARAM)minVal, (LPARAM)maxVal);
        return TzdValue(true);
    });

    reg("ui_step_progress", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        SendMessageW(hwnd, PBM_STEPIT, 0, 0);
        LRESULT pos = SendMessageW(hwnd, PBM_GETPOS, 0, 0);
        {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            if (pos > 0) {
                g_uiProgressValues[hwnd] = (int)pos;
            } else {
                g_uiProgressValues[hwnd] += 10;
            }
        }
        return TzdValue(true);
    });

    reg("ui_create_slider", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 10;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 200;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 30;
        int minVal = (args.size() > 5) ? (int)TzdInterpreter::getAsDoubleInternal(args[5]) : 0;
        int maxVal = (args.size() > 6) ? (int)TzdInterpreter::getAsDoubleInternal(args[6]) : 100;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND tb = CreateWindowExW(
            0, TRACKBAR_CLASSW, L"",
            WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | WS_TABSTOP,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        SendMessageW(tb, TBM_SETRANGEMIN, TRUE, (LPARAM)minVal);
        SendMessageW(tb, TBM_SETRANGEMAX, TRUE, (LPARAM)maxVal);
        SendMessageW(tb, TBM_SETPOS, TRUE, (LPARAM)minVal);
        return TzdValue((void*)tb);
    });

    reg("ui_set_slider_val", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int pos = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        SendMessageW(hwnd, TBM_SETPOS, TRUE, (LPARAM)pos);
        return TzdValue(true);
    });

    reg("ui_get_slider_val", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(0.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT pos = SendMessageW(hwnd, TBM_GETPOS, 0, 0);
        return TzdValue((double)pos);
    });

    reg("ui_set_slider_range", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 3 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int minVal = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        int maxVal = (int)TzdInterpreter::getAsDoubleInternal(args[2]);
        SendMessageW(hwnd, TBM_SETRANGEMIN, TRUE, (LPARAM)minVal);
        SendMessageW(hwnd, TBM_SETRANGEMAX, TRUE, (LPARAM)maxVal);
        return TzdValue(true);
    });

    reg("ui_create_combobox", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 10;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 200;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 200;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND cb = CreateWindowExW(
            0, L"COMBOBOX", L"",
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(cb);
        return TzdValue((void*)cb);
    });

    reg("ui_combobox_add_item", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(-1.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string text = TzdInterpreter::getAsString(args[1]);
        std::wstring wText = utf8ToWide(text);
        LRESULT idx = SendMessageW(hwnd, CB_ADDSTRING, 0, (LPARAM)wText.c_str());
        return TzdValue((double)idx);
    });

    reg("ui_combobox_remove_item", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int idx = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        SendMessageW(hwnd, CB_DELETESTRING, (WPARAM)idx, 0);
        return TzdValue(true);
    });

    reg("ui_combobox_get_selected_index", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(-1.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT idx = SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
        return TzdValue((double)idx);
    });

    reg("ui_combobox_set_selected_index", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int idx = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        SendMessageW(hwnd, CB_SETCURSEL, (WPARAM)idx, 0);
        return TzdValue(true);
    });

    reg("ui_combobox_get_selected_item", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue("");
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT idx = SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
        if (idx == CB_ERR) return TzdValue("");
        int len = (int)SendMessageW(hwnd, CB_GETLBTEXTLEN, (WPARAM)idx, 0);
        if (len <= 0) return TzdValue("");
        std::vector<wchar_t> buf(len + 1, 0);
        SendMessageW(hwnd, CB_GETLBTEXT, (WPARAM)idx, (LPARAM)buf.data());
        return TzdValue(wideToUtf8(buf.data()));
    });

    reg("ui_combobox_get_count", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(0.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT count = SendMessageW(hwnd, CB_GETCOUNT, 0, 0);
        return TzdValue((double)count);
    });

    reg("ui_combobox_clear", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        SendMessageW(hwnd, CB_RESETCONTENT, 0, 0);
        return TzdValue(true);
    });

    reg("ui_create_listbox", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 10;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 200;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 150;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND lb = CreateWindowExW(
            0, L"LISTBOX", L"",
            WS_VISIBLE | WS_CHILD | LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(lb);
        return TzdValue((void*)lb);
    });

    reg("ui_listbox_add_item", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(-1.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string text = TzdInterpreter::getAsString(args[1]);
        std::wstring wText = utf8ToWide(text);
        LRESULT idx = SendMessageW(hwnd, LB_ADDSTRING, 0, (LPARAM)wText.c_str());
        return TzdValue((double)idx);
    });

    reg("ui_listbox_remove_item", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int idx = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        SendMessageW(hwnd, LB_DELETESTRING, (WPARAM)idx, 0);
        return TzdValue(true);
    });

    reg("ui_listbox_get_selected_index", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(-1.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT idx = SendMessageW(hwnd, LB_GETCURSEL, 0, 0);
        return TzdValue((double)idx);
    });

    reg("ui_listbox_set_selected_index", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int idx = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        SendMessageW(hwnd, LB_SETCURSEL, (WPARAM)idx, 0);
        return TzdValue(true);
    });

    reg("ui_listbox_get_selected_item", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue("");
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT idx = SendMessageW(hwnd, LB_GETCURSEL, 0, 0);
        if (idx == LB_ERR) return TzdValue("");
        int len = (int)SendMessageW(hwnd, LB_GETTEXTLEN, (WPARAM)idx, 0);
        if (len <= 0) return TzdValue("");
        std::vector<wchar_t> buf(len + 1, 0);
        SendMessageW(hwnd, LB_GETTEXT, (WPARAM)idx, (LPARAM)buf.data());
        return TzdValue(wideToUtf8(buf.data()));
    });

    reg("ui_listbox_get_count", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(0.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT count = SendMessageW(hwnd, LB_GETCOUNT, 0, 0);
        return TzdValue((double)count);
    });

    reg("ui_listbox_clear", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        SendMessageW(hwnd, LB_RESETCONTENT, 0, 0);
        return TzdValue(true);
    });

    // -------------------------------------------------------------
    // Modern Advanced Controls: TabControl, DateTimePicker, StatusBar, ImageView
    // -------------------------------------------------------------
    reg("ui_create_tabcontrol", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 10;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 300;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 200;

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND tab = CreateWindowExW(
            0, WC_TABCONTROLW, L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(tab);
        return TzdValue((void*)tab);
    });

    reg("ui_tab_add", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(-1.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string title = TzdInterpreter::getAsString(args[1]);
        std::wstring wTitle = utf8ToWide(title);
        int index = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : (int)SendMessageW(hwnd, TCM_GETITEMCOUNT, 0, 0);

        TCITEMW tie;
        tie.mask = TCIF_TEXT;
        tie.pszText = (LPWSTR)wTitle.c_str();
        LRESULT res = SendMessageW(hwnd, TCM_INSERTITEMW, (WPARAM)index, (LPARAM)&tie);
        return TzdValue((double)res);
    });

    reg("ui_tab_get_cur_sel", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue(-1.0);
        HWND hwnd = (HWND)args[0].ptrVal;
        LRESULT res = SendMessageW(hwnd, TCM_GETCURSEL, 0, 0);
        return TzdValue((double)res);
    });

    reg("ui_tab_set_cur_sel", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        int idx = (int)TzdInterpreter::getAsDoubleInternal(args[1]);
        SendMessageW(hwnd, TCM_SETCURSEL, (WPARAM)idx, 0);
        return TzdValue(true);
    });

    reg("ui_create_datetime_picker", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 10;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 160;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 26;
        bool isTime = (args.size() > 5) ? args[5].bVal : false;

        int effY = getEffectiveControlY(parent, y);
        DWORD style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | (isTime ? DTS_TIMEFORMAT : DTS_SHORTDATEFORMAT);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND dtp = CreateWindowExW(
            0, DATETIMEPICK_CLASSW, L"",
            style,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(dtp);
        return TzdValue((void*)dtp);
    });

    reg("ui_get_datetime", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue("");
        HWND hwnd = (HWND)args[0].ptrVal;
        SYSTEMTIME st;
        if (SendMessageW(hwnd, DTM_GETSYSTEMTIME, 0, (LPARAM)&st) == GDT_VALID) {
            char buf[64];
            sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
            return TzdValue(std::string(buf));
        }
        return TzdValue("");
    });

    reg("ui_set_datetime", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 4 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        SYSTEMTIME st = { 0 };
        st.wYear = (WORD)TzdInterpreter::getAsDoubleInternal(args[1]);
        st.wMonth = (WORD)TzdInterpreter::getAsDoubleInternal(args[2]);
        st.wDay = (WORD)TzdInterpreter::getAsDoubleInternal(args[3]);
        if (args.size() > 4) st.wHour = (WORD)TzdInterpreter::getAsDoubleInternal(args[4]);
        if (args.size() > 5) st.wMinute = (WORD)TzdInterpreter::getAsDoubleInternal(args[5]);
        if (args.size() > 6) st.wSecond = (WORD)TzdInterpreter::getAsDoubleInternal(args[6]);
        SendMessageW(hwnd, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
        return TzdValue(true);
    });

    reg("ui_create_statusbar", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND sb = CreateWindowExW(
            0, STATUSCLASSNAMEW, L"",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            parent, NULL, hInstance, NULL
        );
        applyModernFont(sb);
        return TzdValue((void*)sb);
    });

    reg("ui_set_statusbar_text", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string text = TzdInterpreter::getAsString(args[1]);
        int part = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 0;
        std::wstring wText = utf8ToWide(text);
        SendMessageW(hwnd, SB_SETTEXTW, (WPARAM)part, (LPARAM)wText.c_str());
        return TzdValue(true);
    });

    reg("ui_create_image_view", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::POINTER) return TzdValue();
        HWND parent = (HWND)args[0].ptrVal;
        int x = (args.size() > 1) ? (int)TzdInterpreter::getAsDoubleInternal(args[1]) : 10;
        int y = (args.size() > 2) ? (int)TzdInterpreter::getAsDoubleInternal(args[2]) : 10;
        int w = (args.size() > 3) ? (int)TzdInterpreter::getAsDoubleInternal(args[3]) : 100;
        int h = (args.size() > 4) ? (int)TzdInterpreter::getAsDoubleInternal(args[4]) : 100;
        std::string path = (args.size() > 5) ? TzdInterpreter::getAsString(args[5]) : "";

        int effY = getEffectiveControlY(parent, y);
        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
        HWND iv = CreateWindowExW(
            0, L"TzdImageViewClass", L"",
            WS_CHILD | WS_VISIBLE,
            x, effY, w, h,
            parent, NULL, hInstance, NULL
        );

        if (!path.empty()) {
            std::filesystem::path p(utf8ToWide(path));
            if (p.is_relative()) {
                std::filesystem::path cur = std::filesystem::current_path();
                if (!std::filesystem::exists(cur / p) && std::filesystem::exists(cur.parent_path() / p)) {
                    p = cur.parent_path() / p;
                } else {
                    p = cur / p;
                }
            }
            Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(p.c_str());
            if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) {
                std::lock_guard<std::mutex> lock(g_uiMutex);
                g_uiImageViews[iv] = bmp;
            } else if (bmp) {
                delete bmp;
            }
        }

        return TzdValue((void*)iv);
    });

    reg("ui_set_image", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::string path = TzdInterpreter::getAsString(args[1]);
        std::filesystem::path p(utf8ToWide(path));
        if (p.is_relative()) {
            std::filesystem::path cur = std::filesystem::current_path();
            if (!std::filesystem::exists(cur / p) && std::filesystem::exists(cur.parent_path() / p)) {
                p = cur.parent_path() / p;
            } else {
                p = cur / p;
            }
        }
        Gdiplus::Bitmap* newBmp = Gdiplus::Bitmap::FromFile(p.c_str());
        if (newBmp && newBmp->GetLastStatus() == Gdiplus::Ok) {
            std::lock_guard<std::mutex> lock(g_uiMutex);
            auto it = g_uiImageViews.find(hwnd);
            if (it != g_uiImageViews.end() && it->second) {
                delete it->second;
            }
            g_uiImageViews[hwnd] = newBmp;
            InvalidateRect(hwnd, NULL, TRUE);
            return TzdValue(true);
        }
        if (newBmp) delete newBmp;
        return TzdValue(false);
    });

    // Event Bindings
    reg("ui_on_click", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        g_uiControlEvents[hwnd].onClickCallback = args[1];
        return TzdValue(true);
    });

    reg("ui_on_change", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        g_uiControlEvents[hwnd].onChangeCallback = args[1];
        return TzdValue(true);
    });

    reg("ui_on_close", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        g_uiWindowEvents[hwnd].onCloseCallback = args[1];
        return TzdValue(true);
    });

    reg("ui_on_resize", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        g_uiWindowEvents[hwnd].onResizeCallback = args[1];
        return TzdValue(true);
    });

    reg("ui_on_error", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::POINTER) return TzdValue(false);
        HWND hwnd = (HWND)args[0].ptrVal;
        std::lock_guard<std::mutex> lock(g_uiMutex);
        g_uiWindowEvents[hwnd].onErrorCallback = args[1];
        return TzdValue(true);
    });

    reg("ui_set_global_error_handler", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        std::lock_guard<std::mutex> lock(g_uiMutex);
        g_uiGlobalOnError = args[0];
        return TzdValue(true);
    });

    reg("ui_set_show_error_dialog", [](const std::vector<TzdValue>& args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        g_uiShowErrorDialog = (args[0].type == TzdValue::BOOL) ? args[0].bVal : (TzdInterpreter::getAsDoubleInternal(args[0]) != 0);
        return TzdValue(true);
    });

    reg("ui_run_loop", [](const std::vector<TzdValue>& args) -> TzdValue {
        HWND winHwnd = (args.size() > 0 && args[0].type == TzdValue::POINTER) ? (HWND)args[0].ptrVal : NULL;
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (winHwnd && !IsWindow(winHwnd)) break;
        }
        return TzdValue(true);
    });

    reg("ui_poll_events", [](const std::vector<TzdValue>&) -> TzdValue {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return TzdValue(true);
    });
}
