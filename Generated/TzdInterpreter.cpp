#include "TzdInterpreter.h"
#include "../TzdDebugger.h"
#include <fstream>
#include <chrono>
#include <sstream>

//#include "TzdJitCompiler.h"

extern "C" void* rt_get_fatal_jmp();
extern "C" void rt_set_fatal_jmp(void* buf);

std::string formatSourcePath(const std::string& fullPath) {
    if (fullPath.empty() || fullPath == "memory") return "memory";

    // 统一转换为 Windows 风格反斜杠
    std::string path = fullPath;
    std::replace(path.begin(), path.end(), '/', '\\');

    // 检索标准库的关键段 "\stdlib\"
    size_t pos = path.find("\\stdlib\\");
    if (pos == std::string::npos && path.rfind("stdlib\\", 0) == 0) {
        pos = 0;
    }
    if (pos != std::string::npos) {
        return path.substr(pos + 1); // 截取 stdlib\xxx\xxx.tzd
    }
    return path; // 如果不是标准库，显示全绝对路径
}

std::string unescapeString(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            switch (input[i + 1]) {
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            case '\\': result += '\\'; break;
            case '"':  result += '"';  break;
            case '\'': result += '\''; break;
            default:   result += input[i + 1]; break;
            }
            ++i;
        }
        else {
            result += input[i];
        }
    }
    return result;
}

// #region agent log
static void agentLog(const char* hypothesisId, const char* location, const char* message,
    int lType, int rType, bool eq) {
    std::ofstream f("debug-2ea0b5.log", std::ios::app);
    if (!f) return;
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    f << "{\"sessionId\":\"2ea0b5\",\"hypothesisId\":\"" << hypothesisId
        << "\",\"location\":\"" << location << "\",\"message\":\"" << message
        << "\",\"data\":{\"lType\":" << lType << ",\"rType\":" << rType << ",\"eq\":" << (eq ? "true" : "false")
        << "},\"timestamp\":" << ts << "}\n";
}

static void agentLogCompile(const char* hypothesisId, const char* location, const char* detail) {
    std::ofstream f("debug-2ea0b5.log", std::ios::app);
    if (!f) return;
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    f << "{\"sessionId\":\"2ea0b5\",\"hypothesisId\":\"" << hypothesisId
        << "\",\"location\":\"" << location << "\",\"message\":\"jit compile fail\",\"data\":{\"detail\":\""
        << detail << "\"},\"timestamp\":" << ts << "}\n";
}

static TzdValue castAnyToTzdValue(const std::any& a, const char* where) {
    if (!a.has_value()) {
        agentLog("B", where, "empty any", -1, -1, false);
        throw std::runtime_error(std::string(where) + ": empty any");
    }
    if (a.type() != typeid(TzdValue)) {
        agentLog("B", where, "wrong any type", -1, -1, false);
        throw std::bad_any_cast();
    }
    return std::any_cast<TzdValue>(a);
}

static std::string getParamName(TzdLangParser::ParamContext* p) {
    if (!p) return "arg";
    if (p->IDENTIFIER()) return p->IDENTIFIER()->getText();
    if (p->T_INT()) return p->T_INT()->getText();
    if (p->T_STRING()) return p->T_STRING()->getText();
    if (p->T_FLOAT()) return p->T_FLOAT()->getText();
    if (p->T_BOOL()) return p->T_BOOL()->getText();
    if (p->T_VOID()) return p->T_VOID()->getText();
    if (p->T_PTR()) return p->T_PTR()->getText();
    if (p->KW_RET()) return p->KW_RET()->getText();
    return p->getText();
}

static std::string getParamType(TzdLangParser::ParamContext* p) {
    if (!p || !p->typeType()) return "";
    return p->typeType()->getText();
}

static void parseParamList(TzdLangParser::ParamListContext* paramList,
    std::vector<std::string>& names, std::vector<std::string>& types) {
    if (!paramList) return;
    for (auto p : paramList->param()) {
        names.push_back(getParamName(p));
        types.push_back(getParamType(p));
    }
}
// #endregion

HWND g_hPlotWnd = NULL;
Gdiplus::Bitmap* g_pGlobalBitmap = NULL;
std::mutex g_PlotMutex;

extern bool g_InJitCleanup;
TzdInterpreter* g_CurrentInterpreter = nullptr;
using JittedFunc = void (*)(void*, void*);

extern "C" TzdValue* g_LastJitValue;

bool IsUTF8(const std::string& str) {
    int i = 0;
    int nBytes = 0;
    unsigned char ch;
    bool bAllAscii = true;
    for (size_t i = 0; i < str.length(); i++) {
        ch = str[i];
        if ((ch & 0x80) != 0) bAllAscii = false;
        if (nBytes == 0) {
            if (ch >= 0x80) {
                if (ch >= 0xFC && ch <= 0xFD) nBytes = 6;
                else if (ch >= 0xF8) nBytes = 5;
                else if (ch >= 0xF0) nBytes = 4;
                else if (ch >= 0xE0) nBytes = 3;
                else if (ch >= 0xC0) nBytes = 2;
                else return false;
                nBytes--;
            }
        }
        else {
            if ((ch & 0xC0) != 0x80) return false;
            nBytes--;
        }
    }
    if (nBytes > 0) return false;
    if (bAllAscii) return true;
    return true;
}

LRESULT CALLBACK TzdWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        {
            std::lock_guard<std::mutex> lock(g_PlotMutex);
            if (g_pGlobalBitmap) {
                Gdiplus::Graphics graphics(hdc);
                graphics.DrawImage(g_pGlobalBitmap, 0, 0);
            }
        }
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        g_hPlotWnd = NULL;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowPlotWindowThread() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = TzdWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TzdPlotWnd";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    static bool registered = false;
    if (!registered) { RegisterClassW(&wc); registered = true; }

    g_hPlotWnd = CreateWindowExW(0, L"TzdPlotWnd", L"Tzd Tools - Plot Visualization",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 816, 639, NULL, NULL, wc.hInstance, NULL);

    ShowWindow(g_hPlotWnd, SW_SHOW);
    UpdateWindow(g_hPlotWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

std::string AnsiToUtf8(const std::string& str) {
#ifdef _WIN32
    if (str.empty()) return "";

    int wLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (wLen == 0) return str;

    std::wstring wStr(wLen, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wStr[0], wLen);

    int uLen = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (uLen == 0) return str;

    std::string uStr(uLen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, &uStr[0], uLen, nullptr, nullptr);

    if (!uStr.empty() && uStr.back() == '\0') uStr.pop_back();

    return uStr;
#else
    return str;
#endif
}

std::string Utf8ToAnsi(const std::string& utf8) {
    if (utf8.empty()) return "";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wlen);
    int alen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string ansi(alen, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &ansi[0], alen, nullptr, nullptr);
    return ansi;
}

double TzdInterpreter::getAsDouble(std::any value) {
    if (!value.has_value()) return 0.0;
    TzdValue v = (value.type() == typeid(TzdValue)) ? std::any_cast<TzdValue>(value) : TzdValue();

    switch (v.type) {
    case TzdValue::FLOAT: case TzdValue::DOUBLE: return v.dVal;
    case TzdValue::ULONG: return (double)v.ulVal;
    case TzdValue::BOOL:  return v.bVal ? 1.0 : 0.0;
    case TzdValue::POINTER: return (double)(uintptr_t)v.ptrVal;
    default: return (double)v.lVal;
    }
}

double TzdInterpreter::getAsDoubleInternal(const TzdValue& v) {
    switch (v.type) {
    case TzdValue::DOUBLE:
    case TzdValue::FLOAT:
        return v.dVal;

    case TzdValue::INT:
    case TzdValue::LONG:
        return (double)v.lVal;

    case TzdValue::BOOL:
        return v.bVal ? 1.0 : 0.0;

    case TzdValue::STRING: {
        try {
            // 尝试转换为 double
            return std::stod(v.sVal);
        }
        catch (...) {
            // 转换失败返回 0.0，这是 Safe Path 的要求
            return 0.0;
        }
    }

    case TzdValue::POINTER:
        return (double)(uintptr_t)v.ptrVal;
    default:
        return 0.0;
    }
}

bool TzdInterpreter::isTruthy(const TzdValue& v) {
    switch (v.type) {
    case TzdValue::NONE:
        return false;
    case TzdValue::BOOL:
        return v.bVal;
    case TzdValue::STRING:
        return !v.sVal.empty();
    case TzdValue::ARRAY:
        return !v.arrVal.empty();
    case TzdValue::MAP:
        return !v.mapVal.empty();
    case TzdValue::INSTANCE:
    case TzdValue::CLASS_DEF:
    case TzdValue::FUNCTION:
    case TzdValue::NATIVE_FUNCTION:
        return true;
    default:
        return getAsDoubleInternal(v) != 0.0;
    }
}

bool TzdInterpreter::valuesEqual(const TzdValue& l, const TzdValue& r) {
    if (l.type == r.type) {
        if (l.type == TzdValue::BOOL) return l.bVal == r.bVal;
        if (l.type == TzdValue::STRING) return l.sVal == r.sVal;
        if (l.type == TzdValue::FLOAT || l.type == TzdValue::DOUBLE) return l.dVal == r.dVal;
        if (l.type >= TzdValue::SBYTE && l.type <= TzdValue::ULONG) return l.lVal == r.lVal;
    }
    return getAsDoubleInternal(l) == getAsDoubleInternal(r);
}

std::string TzdInterpreter::getAsString(std::any value) {
    if (!value.has_value()) return "null";
    TzdValue v = (value.type() == typeid(TzdValue)) ? std::any_cast<TzdValue>(value) : TzdValue();
    if (v.type == TzdValue::NONE) return "null";

    switch (v.type) {
    case TzdValue::SBYTE: case TzdValue::BYTE:
    case TzdValue::SHORT: case TzdValue::USHORT:
    case TzdValue::INT:   case TzdValue::UINT:
    case TzdValue::LONG:  return std::to_string(v.lVal);
    case TzdValue::ULONG: return std::to_string(v.ulVal);
    case TzdValue::FLOAT: case TzdValue::DOUBLE: {
        std::string s = std::to_string(v.dVal);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    case TzdValue::POINTER: {
        std::stringstream ss;
        ss << "0x" << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << (uintptr_t)v.ptrVal;
        return ss.str();
    }
    case TzdValue::STRING: return v.sVal;
    case TzdValue::BOOL:   return v.bVal ? "true" : "false";
    case TzdValue::ARRAY: {
        if (v.arrVal.empty()) {
            return "[]";
        }

        // 1. 严格检查：数组内的每一个元素是否全都是真正的“字符”或“单字符字符串”
        bool isPureCharArray = true;
        for (const auto& item : v.arrVal) {
            if (item.type == TzdValue::STRING) {
                const std::string& s = item.sVal;
                int charCount = 0;
                for (size_t i = 0; i < s.length(); ++i) {
                    if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
                        charCount++;
                    }
                }
                if (charCount != 1) {
                    isPureCharArray = false;
                    break;
                }
            }
            else if (item.type == TzdValue::SBYTE || item.type == TzdValue::BYTE) {
            }
            else {
                isPureCharArray = false;
                break;
            }
        }
        if (isPureCharArray) {
            std::string textResult;
            for (const auto& item : v.arrVal) {
                if (item.type == TzdValue::STRING) {
                    textResult += item.sVal; // 拼接单个汉字或英文字符
                }
                else {
                    char c = (item.type == TzdValue::SBYTE) ? (char)item.lVal : (char)item.ulVal;
                    if (c == '\0') break;
                    textResult.push_back(c);
                }
            }
            return textResult;
        }

        std::string res = "[";
        for (size_t i = 0; i < v.arrVal.size(); ++i) {
            res += getAsString(v.arrVal[i]);
            if (i < v.arrVal.size() - 1) res += ", ";
        }
        return res + "]";
    }
    case TzdValue::MAP: {
        std::string res = "{";
        bool first = true;
        for (auto const& [key, val] : v.mapVal) {
            if (!first) res += ", ";
            res += "\"" + key + "\": " + getAsString(val);
            first = false;
        }
        return res + "}";
    }
    case TzdValue::INSTANCE: {
        if (!v.instanceVal) return "null instance";
        std::string res = "Instance of " + v.instanceVal->definition->fullName + " {";
        bool first = true;
        for (auto const& [name, fVal] : v.instanceVal->fields) {
            if (!first) res += ", ";
            res += name + ": " + getAsString(fVal);
            first = false;
        }
        return res + "}";
    }

    case TzdValue::CLASS_DEF: return "[Class: " + (v.classDefVal ? v.classDefVal->fullName : "null") + "]";
    case TzdValue::FUNCTION: return "[Function: " + v.name + "]";
    case TzdValue::NATIVE_FUNCTION: return "[Native Function: " + v.name + "]";
    default: return "unknown";
    }
}

void TzdInterpreter::compileCurrentContext() {
    g_CurrentInterpreter = this;

    // 1. 初始化编译器
    TzdCompiler compiler(*m_jitEngine, "GlobalModule_" + std::to_string(rand()));
    compiler.setupExternalFunctions();

    bool needsCompile = false;
    std::vector<TzdValue*> pendingValues;

    for (auto& scope : scopes) {
        for (auto& [name, val] : scope) {
            if (val.type == TzdValue::FUNCTION && val.funcBody && !val.jittedPtr) {
                auto* funcCtx = dynamic_cast<TzdLangParser::FunctionDeclarationContext*>(val.funcBody->parent);
                if (funcCtx) {
                    compiler.visitFunctionDeclaration(funcCtx);
                    needsCompile = true;
                    pendingValues.push_back(&val);
                }
            }
        }
    }

    if (needsCompile) {
        auto TSM = compiler.extractThreadSafeModule();

        if (TSM) {
            m_jitEngine->addModule(std::move(TSM));

            for (auto* valPtr : pendingValues) {
                auto symOrErr = m_jitEngine->lookupSymbol(valPtr->name);
                if (!symOrErr) {
                    consumeError(symOrErr.takeError());
                    continue;
                }

                llvm::orc::ExecutorAddr execAddr = *symOrErr; 
                auto rawAddr = execAddr.getValue();
                valPtr->jittedPtr = reinterpret_cast<void(*)(void*, void*)>(rawAddr);
            }
        }

    }
}

/**
 * 编译指定的脚本代码（字符串）并立即执行或载入内存
 */
void TzdInterpreter::compileScriptToMemory(const std::string& code) {
    g_CurrentInterpreter = this;
    antlr4::ANTLRInputStream input(code);
    TzdLangLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    TzdLangParser parser(&tokens);
    auto* tree = parser.program();
    TzdCompiler compiler(*m_jitEngine, "DirectScript_" + std::to_string(rand()));
    compiler.setupExternalFunctions();
    compiler.visit(tree);
    auto TSM = compiler.extractThreadSafeModule();

    if (TSM) {
        m_jitEngine->addModule(std::move(TSM));
    }
    else {
        std::cerr << "[Error] Failed to extract ThreadSafeModule in compileScriptToMemory" << std::endl;
    }
}

/**
 * 编译指定名称的函数
 */
void TzdInterpreter::compileFunctionToMemory(const std::string& funcName) {
    g_CurrentInterpreter = this;
    TzdValue& val = scopes.back()[funcName];

    if (val.type == TzdValue::FUNCTION && val.funcBody) {
        TzdCompiler compiler(*m_jitEngine, "Mod_" + funcName);
        auto* funcCtx = dynamic_cast<TzdLangParser::FunctionDeclarationContext*>(val.funcBody->parent);
        if (funcCtx) {
            compiler.visitFunctionDeclaration(funcCtx);
            auto TSM = compiler.extractThreadSafeModule();
            if (TSM) {
                m_jitEngine->addModule(std::move(TSM));

                auto sym = m_jitEngine->lookupSymbol(funcName);
                if (sym) {
                    val.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(sym->getValue());
                }
            }
        }
    }
}


static std::vector<std::string> split_by_dot(const std::string& name) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = name.find('.');

    while (end != std::string::npos) {
        parts.push_back(name.substr(start, end - start));
        start = end + 1;
        end = name.find('.', start);
    }
    // 最后一个部分
    parts.push_back(name.substr(start));
    return parts;
}

TzdValue TzdInterpreter::getVariable(const std::string& name, antlr4::ParserRuleContext* ctx) {

    // **优化 2: 简单变量名快速路径**
    // 假设不含 '.' 的简单变量名查找是最常见的操作。
    size_t dot_pos = name.find('.');
    if (dot_pos == std::string::npos) {

        // --- 简单变量名查找逻辑 ---

        // 查找局部/全局作用域
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            // **优化 3: find() 代替 count() + []**
            auto scope_it = it->find(name);
            if (scope_it != it->end()) {
                return scope_it->second; // 最快路径：直接返回
            }
        }

        // 查找 this 实例成员
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto thisIt = it->find("this");
            if (thisIt != it->end() && thisIt->second.type == TzdValue::INSTANCE) {
                TzdInstance* inst = thisIt->second.instanceVal;
                try {
                    return inst->getMember(name);
                }
                catch (...) {
                    // 忽略异常，继续查找
                }
            }
        }

        // 查找 ClassDef
        TzdClassDef* cls = TzdOopManager::getClass(name);
        if (cls) {
            return TzdValue(cls);
        }

        // 未定义
        throw TzdRuntimeException("未定义的标识符: '" + name + "'", ctx ? ctx->getStart() : nullptr);
    }

    // ---------------------------------------------------------------------
    // 复杂路径：name 包含 '.' (例如：obj.member.submember)
    // ---------------------------------------------------------------------

    std::vector<std::string> parts = split_by_dot(name);

    TzdValue current;
    bool foundBase = false;
    // **优化 4: 使用引用** 避免拷贝 parts[0]
    std::string& baseName = parts[0];

    // 1. 查找 baseName
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        // **优化 3: find() 代替 count() + []**
        auto scope_it = it->find(baseName);
        if (scope_it != it->end()) {
            current = scope_it->second;
            foundBase = true;
            break;
        }
    }

    if (!foundBase) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto thisIt = it->find("this");
            if (thisIt != it->end() && thisIt->second.type == TzdValue::INSTANCE) {
                TzdInstance* inst = thisIt->second.instanceVal;
                try {
                    current = inst->getMember(baseName);
                    foundBase = true;
                    break;
                }
                catch (...) {
                }
            }
        }
    }

    if (!foundBase) {
        TzdClassDef* cls = TzdOopManager::getClass(baseName);
        if (cls) {
            current = TzdValue(cls);
            foundBase = true;
        }
    }

    if (!foundBase) {
        throw TzdRuntimeException("未定义的标识符: '" + baseName + "'", ctx ? ctx->getStart() : nullptr);
    }

    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string& memberName = parts[i];

        if (current.type == TzdValue::INSTANCE) {
            current = current.instanceVal->getMember(memberName);
        }
        else if (current.type == TzdValue::CLASS_DEF) {
            TzdClassDef* cls = current.classDefVal;
            auto static_it = cls->staticValues.find(memberName);
            if (static_it != cls->staticValues.end()) {
                current = static_it->second;
            }
            else {
                ClassMethod* m = cls->findMethod(memberName);
                if (m && m->isStatic) {
                    current.type = TzdValue::FUNCTION;
                    current.name = m->name;
                    current.params = m->params;
                    current.funcBody = m->body;
                    current.instanceVal = nullptr;
                }
                else {
                    throw TzdRuntimeException("类 '" + cls->fullName + "' 中不存在静态成员: " + memberName, ctx->getStart());
                }
            }
        }
        else {
            throw TzdRuntimeException("基础类型 '" + getAsString(current) + "' 无法访问成员 '" + memberName + "'", ctx->getStart());
        }
    }

    return current;
}

void TzdInterpreter::setVariable(const std::string& name, TzdValue val) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) {
            (*it)[name] = val;
            return;
        }
    }

    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count("this")) {
            TzdValue thisVal = (*it)["this"];
            if (thisVal.type == TzdValue::INSTANCE) {
                try {
                    thisVal.instanceVal->setMember(name, val);
                    return;
                }
                catch (...) {}
            }
        }
    }

    scopes.back()[name] = val;
}

void TzdInterpreter::loadScript(std::string code) {
    // --- 1. 状态保存 (支持递归 import 的核心) ---
    // 保存当前正在编译的 Module 环境，防止被子脚本的 loadScript 覆盖
    auto savedCompiler = std::move(m_compiler);
    auto savedPending = std::move(m_pendingJitFunctions);
    auto savedJitMap = std::move(m_jitNameToUserMap);

    // --- 2. 初始化当前脚本的新编译器上下文 ---
    // 注意：不再在这里调用 prepareForScript()，因为它会彻底清空状态
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "Module_" + std::to_string(rand()));
    m_compiler->setupExternalFunctions();
    m_pendingJitFunctions.clear();
    m_jitNameToUserMap.clear();

    // --- 3. 编码处理与 ANTLR 初始化 ---
    std::string utf8Code = IsUTF8(code) ? code : AnsiToUtf8(code);
    m_currentSource = utf8Code;

    ScriptModule* mod = new ScriptModule();
    mod->input = new antlr4::ANTLRInputStream(utf8Code);
    mod->lexer = new TzdLangLexer(mod->input);
    mod->tokens = new antlr4::CommonTokenStream(mod->lexer);
    mod->parser = new TzdLangParser(mod->tokens);

    TzdErrorListener err;
    mod->parser->removeErrorListeners();
    mod->parser->addErrorListener(&err);
    mod->tree = mod->parser->program();

    if (mod->parser->getNumberOfSyntaxErrors() > 0) {
        delete mod;
        // 恢复父级编译器状态并退出
        m_compiler = std::move(savedCompiler);
        m_pendingJitFunctions = std::move(savedPending);
        m_jitNameToUserMap = std::move(savedJitMap);
        throw std::runtime_error("脚本包含语法错误，未加载。");
    }

    loadedModules.push_back(mod);
    initNativeFunctions(); // 确保内建函数可用

    // --- 4. 执行访问（编译）与 JIT ---
    try {
        // visit 过程中如果遇到 import，会递归调用 loadScript，
        // 由于我们上面做了“状态保存”，所以递归是安全的。
        std::any result = this->visit(mod->tree);

        // 【关键点】：脚本访问完毕后，立即编译并提取当前模块的机器码
        // 这能解决主脚本找不到 import 脚本函数的问题
        jitPendingModule();

        // 仅在 REPL 顶层或有明确返回值时打印结果
        if (result.has_value() && result.type() == typeid(TzdValue)) {
            TzdValue val = std::any_cast<TzdValue>(result);
            if (val.type != TzdValue::NONE) {
                std::string outStr = getAsString(val);
                std::cout << ">>> ";
                if (val.type == TzdValue::STRING) {
                    std::cout << "\"" << Utf8ToAnsi(outStr) << "\"" << std::endl;
                }
                else {
                    std::cout << Utf8ToAnsi(outStr) << std::endl;
                }
            }
        }
    }
    catch (const TzdRuntimeException& e) {
        // 传入 e.stackTrace
        TzdErrorHandler::report("Tzd 运行时错误", e.line, e.column, e.what(), utf8Code, e.stackTrace);
    }
    catch (const TzdThrowException& e) {
        std::string msg = "Unknown error";
        if (e.value.type == TzdValue::INSTANCE && e.value.instanceVal) {
            try {
                TzdValue toStr = e.value.instanceVal->getMember("message");
                if (toStr.type == TzdValue::STRING && !toStr.sVal.empty()) msg = toStr.sVal;
            }
            catch (...) {}
        }
        else if (e.value.type == TzdValue::STRING) {
            msg = e.value.sVal;
        }
        // 传入 e.stackTrace
        TzdErrorHandler::report("Tzd 未捕获异常", 0, 0, msg, utf8Code, e.stackTrace);
    }
    catch (const std::exception& e) {
        TzdErrorHandler::report("系统异常", 0, 0, e.what(), "");
    }

    // --- 5. 还原父级编译器状态 ---
    // 当前脚本编译完了，把主脚本的编译器还给 m_compiler，让主流程继续
    m_compiler = std::move(savedCompiler);
    m_pendingJitFunctions = std::move(savedPending);
    m_jitNameToUserMap = std::move(savedJitMap);
}

void TzdInterpreter::loadScriptFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) throw std::runtime_error("无法打开文件: " + filePath);

    std::stringstream ss;
    ss << file.rdbuf();
    std::string code = ss.str();

    m_scriptPathStack.push_back(fs::absolute(filePath));

    try {
        this->loadScript(code);
    }
    catch (...) {
        m_scriptPathStack.pop_back();
        throw;
    }

    m_scriptPathStack.pop_back();
}

std::string TzdInterpreter::resolveImportPath(const std::string& inputPath) {
    fs::path target;

    // 1. 处理点号表示法: xxx.yyy -> xxx/yyy.tzd
    std::string processedPath = inputPath;
    if (inputPath.find('/') == std::string::npos && inputPath.find('\\') == std::string::npos && inputPath.find('.') != std::string::npos) {
        std::replace(processedPath.begin(), processedPath.end(), '.', '/');
        processedPath += ".tzd";
    }

    // 2. 尝试相对于当前脚本的路径
    if (!m_scriptPathStack.empty()) {
        fs::path currentDir = m_scriptPathStack.back().parent_path();
        target = currentDir / processedPath;
        // 【修改】：使用 weakly_canonical 确保路径规范化（消除 .. 和斜杠差异）
        if (fs::exists(target)) return fs::weakly_canonical(fs::absolute(target)).string();
    }

    // 3. 遍历用户定义的包含路径
    for (const auto& includeDir : m_includePaths) {
        target = fs::path(includeDir) / processedPath;
        if (fs::exists(target)) return fs::weakly_canonical(fs::absolute(target)).string();
    }

    // 4. 尝试作为绝对路径或直接相对路径
    target = fs::path(processedPath);
    if (fs::exists(target)) return fs::weakly_canonical(fs::absolute(target)).string();

    return ""; // 未找到
}

void TzdInterpreter::initNativeFunctions() {
    TzdNativeModule::init(this);
}

void TzdInterpreter::internalRenderPlot(const std::vector<TzdValue>& functions, double start, double end, double step) {
    RGBABitmapImageReference* imageReference = CreateRGBABitmapImageReference();

    // 颜色表
    struct MyColor { float r, g, b; Gdiplus::Color gdiColor; };
    std::vector<MyColor> colorTable = {
        // 1. 蓝色 (Modern Blue)
        {0.12f, 0.47f, 0.71f, Gdiplus::Color(255, 31, 119, 180)},

        // 2. 橙色 (Orange)
        {1.00f, 0.50f, 0.05f, Gdiplus::Color(255, 255, 127, 14)},

        // 3. 绿色 (Modern Green)
        {0.17f, 0.63f, 0.17f, Gdiplus::Color(255, 44, 160, 44)},

        // 4. 红色 (Modern Red)
        {0.84f, 0.15f, 0.16f, Gdiplus::Color(255, 214, 39, 40)},

        // 5. 紫色 (Purple)
        {0.58f, 0.40f, 0.74f, Gdiplus::Color(255, 148, 103, 189)},

        // 6. 棕色 (Brown)
        {0.55f, 0.34f, 0.29f, Gdiplus::Color(255, 140, 86, 75)},

        // 7. 粉紫色 (Pink)
        {0.89f, 0.47f, 0.76f, Gdiplus::Color(255, 227, 119, 194)},

        // 8. 灰色 (Gray)
        {0.50f, 0.50f, 0.50f, Gdiplus::Color(255, 127, 127, 127)},

        // 9. 黄绿色 (Olive)
        {0.74f, 0.74f, 0.13f, Gdiplus::Color(255, 188, 189, 34)},

        // 10. 青色 (Cyan)
        {0.09f, 0.75f, 0.81f, Gdiplus::Color(255, 23, 190, 207)}
    };

    double xPadding = 70.0; // 必须和你设置的 settings->xPadding 一致
    double canvasW = 850.0; // 必须和你设置的 settings->width 一致
    double plotWidth = canvasW - (xPadding * 2.0);
    double unitPerPixel = (end - start) / plotWidth;

    // 算出屏幕最左边和最右边对应的数学 X 值
    double extendedStart = start - (xPadding * unitPerPixel);
    double extendedEnd = end + (xPadding * unitPerPixel);

    std::vector<ScatterPlotSeries*> allSeries;
    for (size_t fIdx = 0; fIdx < functions.size(); ++fIdx) {
        auto xs = new std::vector<double>();
        auto ys = new std::vector<double>();

        for (double val = extendedStart; val <= extendedEnd; val += step) {
            std::unordered_map<std::string, TzdValue> callScope;
            if (!functions[fIdx].params.empty()) {
                callScope[functions[fIdx].params[0]] = TzdValue(val);
            }
            this->scopes.push_back(callScope);

            TzdValue res(0.0);
            try {
                if (functions[fIdx].type == TzdValue::NATIVE_FUNCTION) {
                    res = functions[fIdx].nativeFunc({ TzdValue(val) });
                }
                else {
                    std::any v = this->visit(functions[fIdx].funcBody);
                    if (v.has_value() && v.type() == typeid(TzdValue)) res = std::any_cast<TzdValue>(v);
                }
            }
            catch (const TzdReturnException& e) { res = e.value; }
            catch (...) { res = TzdValue(0.0); }

            this->scopes.pop_back();
            xs->push_back(val);
            ys->push_back(res.dVal);
        }

        ScatterPlotSeries* series = GetDefaultScatterPlotSeriesSettings();
        series->xs = xs; series->ys = ys;
        series->linearInterpolation = true;
        auto& c = colorTable[fIdx % colorTable.size()];
        series->color = new RGBA{ c.r, c.g, c.b, 1.0 };
        allSeries.push_back(series);
    }

    ScatterPlotSettings* settings = GetDefaultScatterPlotSettings();
    settings->width = 850; settings->height = 600;
    settings->xPadding = 70;
    settings->autoBoundaries = true;
    settings->scatterPlotSeries = &allSeries;

    StringReference* errorMessage = CreateStringReferenceLengthValue(0, L' ');
    if (DrawScatterPlotFromSettings(imageReference, settings, errorMessage)) {
        std::vector<double>* pngdata = ConvertToPNG(imageReference->image);
        WriteToFile(pngdata, "tzd_temp.png");

        Gdiplus::Bitmap* localPreparedBmp = nullptr;
        {
            Gdiplus::Image* base = Gdiplus::Image::FromFile(L"tzd_temp.png");
            if (base && base->GetLastStatus() == Gdiplus::Ok) {
                localPreparedBmp = new Gdiplus::Bitmap(base->GetWidth(), base->GetHeight());
                Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(localPreparedBmp);
                g->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                g->Clear(Gdiplus::Color::White);
                g->DrawImage(base, 0, 0);

                Gdiplus::Font font(L"Consolas", 10, Gdiplus::FontStyleBold);
                Gdiplus::SolidBrush textB(Gdiplus::Color(255, 40, 40, 40));

                for (size_t i = 0; i < functions.size(); ++i) {
                    Gdiplus::Pen p(colorTable[i % colorTable.size()].gdiColor, 3);
                    int yPos = 65 + (int)i * 25;
                    g->DrawLine(&p, 630, yPos, 660, yPos);

                    // --- 使用真正的函数名 ---
                    std::string nameStr = functions[i].name;
                    if (nameStr.empty()) nameStr = "unnamed";

                    std::wstring lbl = std::wstring(nameStr.begin(), nameStr.end()) + L"(x)";
                    g->DrawString(lbl.c_str(), -1, &font, Gdiplus::PointF(665, (float)yPos - 8), &textB);
                }
                delete g;
            }
            if (base) delete base;
        }

        if (localPreparedBmp) {
            std::lock_guard<std::mutex> lock(g_PlotMutex);
            if (g_pGlobalBitmap) delete g_pGlobalBitmap;
            g_pGlobalBitmap = localPreparedBmp;
        }

        // 窗口显示与刷新逻辑
        if (g_hPlotWnd == NULL) {
            std::thread(ShowPlotWindowThread).detach();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        if (g_hPlotWnd != NULL) {
            InvalidateRect(g_hPlotWnd, NULL, FALSE);
            // 立即强制重绘以减少白屏感
            SendMessage(g_hPlotWnd, WM_PAINT, 0, 0);
        }
    }

    // 清理内存
    for (auto s : allSeries) {
        delete s->xs; delete s->ys;
        if (s->color) delete s->color;
    }
    FreeAllocations();
}

void TzdInterpreter::setGlobalVariable(const std::string& name, const TzdValue& val)
{
    {
        if (!scopes.empty()) {
            scopes[0][name] = val;
        }
    }
}

void TzdInterpreter::mapJitSymbolsToValue() {
    for (auto& scope : scopes) {
        for (auto& [name, val] : scope) {
            if (val.type == TzdValue::FUNCTION) {
                void* addr = reinterpret_cast<void(*)(void*, void*)>((m_jitEngine->lookupSymbol(name))->getValue());
                if (addr) {
                    val.jittedPtr = (void(*)(void*, void*))addr;
                }
            }
        }
    }
}

TzdValue TzdInterpreter::callFunction(const TzdValue& func, const std::vector<TzdValue>& args) {
    struct CallStackGuard {
        TzdInterpreter* self;
        explicit CallStackGuard(TzdInterpreter* interp, const std::string& frame, const std::string& sourceFile) : self(interp) {
            self->m_callStackFrames.push_back(frame);
            self->m_debugFileStack.push_back(sourceFile);
        }
        ~CallStackGuard() {
            if (!self->m_callStackFrames.empty()) self->m_callStackFrames.pop_back();
            if (!self->m_debugFileStack.empty()) self->m_debugFileStack.pop_back();
        }
    };

    std::string fileLoc = formatSourcePath(func.sourceFile);
    if (func.line > 0) {
        fileLoc += ":" + std::to_string(func.line);
    }

    std::string stackFrame = func.name.empty() ? "<anonymous>" : func.name;
    if (func.instanceVal && func.instanceVal->definition) {
        stackFrame = func.instanceVal->definition->fullName + "." + func.name;
    }

    stackFrame += " (" + fileLoc + ")";

    if (func.type == TzdValue::NATIVE_FUNCTION) stackFrame += " (Native Method)";
    else if (func.type == TzdValue::FUNCTION && func.jittedPtr) stackFrame += " (JIT Compiled)";
    else stackFrame += " (Interpreted)";

    CallStackGuard stackGuard(this, stackFrame, func.sourceFile);

    // 1. 类型校验
    if (func.type != TzdValue::FUNCTION && func.type != TzdValue::NATIVE_FUNCTION) {
        throw std::runtime_error("尝试调用一个非函数对象");
    }

    // 2. 参数个数校验 (针对脚本函数)
    if (func.type == TzdValue::FUNCTION) {
        if (args.size() != func.params.size()) {
            throw std::runtime_error("函数 '" + func.name + "' 参数不匹配: 期望 " +
                std::to_string(func.params.size()) + " 个，实际 " +
                std::to_string(args.size()) + " 个");
        }
        for (size_t i = 0; i < args.size(); ++i) {
            if (i < func.paramTypes.size() && !func.paramTypes[i].empty() &&
                !checkParamValueType(func.paramTypes[i], args[i])) {
                throw std::runtime_error(
                    "函数 '" + func.name + "' 第 " + std::to_string(i + 1) +
                    " 个参数类型不匹配: 期望 " + func.paramTypes[i]);
            }
        }
    }

    // --- 分支 A: JIT 机器码执行 ---
    // 只要 func.jittedPtr 不为空，就说明这个版本已经被编译成功
    // --- 分支 A: JIT 机器码执行 ---
    if (func.type == TzdValue::FUNCTION && func.jittedPtr && !TzdDebugger::g_DebugActive) {
        this->clearJitError();
        this->m_jitUnhandledThrow.reset(); // 清除上一次遗留的异常
        g_CurrentInterpreter = this;
        g_LastJitValue = nullptr;

        std::vector<TzdValue> jitArgs;
        g_JitPool.reset();
        if (func.instanceVal != nullptr) {
            jitArgs.push_back(TzdValue(func.instanceVal));
        }
        jitArgs.insert(jitArgs.end(), args.begin(), args.end());

        this->m_currentArgs = jitArgs;
        this->m_argPtrStack.push_back(this->m_currentArgs.data());

        std::unordered_map<std::string, TzdValue> jitScope;
        if (func.instanceVal) jitScope["this"] = TzdValue(func.instanceVal);
        scopes.push_back(jitScope);

        TzdValue result;
        try {
            // 自然执行 JIT 机器码
            func.jittedPtr(this, &result);

            // ============================================
            // 此时已经安全脱离 LLVM JIT 栈帧，回到了纯净的 C++ 栈！
            // 可以放心地使用纯 C++ 特性抛出错误。
            // ============================================

            // 1. 如果代码里写了 throw new Error()，它会被缓存在这里
            if (this->m_jitUnhandledThrow) {
                auto thrown = *this->m_jitUnhandledThrow;
                this->m_jitUnhandledThrow.reset();
                throw thrown; // 安全抛出！附带完整调用链
            }

            // 2. 如果是找不到类等 JIT 编译或执行时错误
            if (this->m_hasJitError) {
                // 将保存的 JIT 错误、堆栈和坐标转换为标准的 RuntimeException！
                TzdRuntimeException ex(m_lastJitError, nullptr, m_jitErrorTrace);
                ex.line = this->m_jitLine;
                ex.column = this->m_jitColumn;
                throw ex;
            }

            if (g_LastJitValue != nullptr) {
                result = *g_LastJitValue;
                g_LastJitValue = nullptr;
            }
        }
        catch (...) {
            scopes.pop_back();
            this->m_argPtrStack.pop_back();
            this->m_currentArgs.clear();
            throw;
        }

        scopes.pop_back();
        this->m_argPtrStack.pop_back();
        this->m_currentArgs.clear();

        if (scopes.size() <= 1) clearJitMemory();
        return result;
    }

    // --- 分支 B: 原生 C++ 函数 ---
    if (func.type == TzdValue::NATIVE_FUNCTION) {
        return func.nativeFunc(args);
    }

    // --- 分支 C: 解释执行 (JIT 未就绪或重定义后尚未链接) ---
    std::unordered_map<std::string, TzdValue> callScope;
    if (func.instanceVal != nullptr) callScope["this"] = TzdValue(func.instanceVal);
    for (size_t i = 0; i < func.params.size(); ++i) {
        callScope[func.params[i]] = args[i];
    }

    scopes.push_back(callScope);
    TzdValue result;
    try {
        if (func.funcBody) {
            std::any res = visit(func.funcBody);
            if (res.has_value() && res.type() == typeid(TzdValue))
                result = std::any_cast<TzdValue>(res);
        }
    }
    catch (const TzdReturnException& e) { result = e.value; }
    catch (const TzdThrowException&) { scopes.pop_back(); throw; }
    scopes.pop_back();
    return result;
}

// --- 程序结构 ---
std::any TzdInterpreter::visitProgram(TzdLangParser::ProgramContext* ctx) {
    std::any lastValue;

    // Phase 1: 只执行“声明类语句”（用于完成函数/类的编译与符号注册）。
    // 然后立即 jitPendingModule()，确保 Phase 2 中的第一次调用就能走 JIT。
    std::vector<TzdLangParser::StatementContext*> execStmts;
    for (auto stmt : ctx->statement()) {
        bool isJitDeclaration =
            dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt) ||
            dynamic_cast<TzdLangParser::ClassDeclStmtContext*>(stmt) ||
            dynamic_cast<TzdLangParser::NativeFunDeclStmtContext*>(stmt) ||
            dynamic_cast<TzdLangParser::ImportStmtContext*>(stmt);

        if (isJitDeclaration) {
            try {
                lastValue = visit(stmt);
            }
            catch (const TzdReturnException& e) { return e.value; }
        }
        else {
            execStmts.push_back(stmt);
        }
    }

    // 将当前编译器里 pending 的符号链接到 jittedPtr（全局函数 + 类方法）。
    jitPendingModule();

    // Phase 2: 执行所有其余语句（例如顶层的 test(); / runBenchmark(); 调用）。
    for (auto stmt : execStmts) {
        try {
            lastValue = visit(stmt);
        }
        catch (const TzdReturnException& e) { return e.value; }
    }
    return lastValue;
}

std::any TzdInterpreter::visitBlock(TzdLangParser::BlockContext* ctx) {
    std::any lastValue;
    for (auto stmt : ctx->statement()) {
        try {
            lastValue = visit(stmt);
        }
        catch (const TzdBreakException&) { throw; }
        catch (const TzdContinueException&) { throw; }
        catch (const TzdReturnException&) { throw; }
        catch (const TzdThrowException&) { throw; }
    }
    return lastValue;
}

std::any TzdInterpreter::visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) {
    TzdValue errVal = castAnyToTzdValue(visit(ctx->expression()), "visitThrowStmt");

    auto trace = this->m_callStackFrames;
    trace.push_back("<throw> at line " + std::to_string(ctx->getStart()->getLine()));

    throw TzdThrowException(std::move(errVal), trace);
}

std::any TzdInterpreter::visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) {
    std::string catchName = ctx->IDENTIFIER()->getText();
    try {
        visit(ctx->block(0));
    }
    catch (const TzdThrowException& e) {
        std::unordered_map<std::string, TzdValue> catchScope;
        catchScope[catchName] = e.value;
        scopes.push_back(catchScope);
        try {
            visit(ctx->block(1));
        }
        catch (const TzdBreakException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdContinueException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdReturnException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdThrowException&) {
            scopes.pop_back();
            throw;
        }
        scopes.pop_back();
    }
    return TzdValue();
}

std::any TzdInterpreter::visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) {
    std::vector<TzdValue> elements;
    std::string c_elements = "";

    if (ctx->exprList()) {
        auto exprs = ctx->exprList()->expression();
        for (size_t i = 0; i < exprs.size(); ++i) {
            TzdValue ev = std::any_cast<TzdValue>(visit(exprs[i]));
            elements.push_back(ev);
        }
    }

    TzdValue res(elements);
    return res;
}

std::any TzdInterpreter::visitIndexExpr(TzdLangParser::IndexExprContext* ctx) {
    TzdValue container = std::any_cast<TzdValue>(visit(ctx->expression(0)));
    TzdValue indexVal = std::any_cast<TzdValue>(visit(ctx->expression(1)));

    // 兼容可能被解析为 DOUBLE 或 INT 的下标
    int idx = (indexVal.type == TzdValue::DOUBLE) ? (int)indexVal.dVal : (int)indexVal.lVal;

    // --- 处理数组访问 ---
    if (container.type == TzdValue::ARRAY) {
        if (idx < 0 || idx >= (int)container.arrVal.size()) {
            throw TzdRuntimeException("数组索引越界: 尝试访问索引 " + std::to_string(idx) +
                ", 但数组长度为 " + std::to_string(container.arrVal.size()),
                ctx->getStart());
        }
        return container.arrVal[idx]; // ✨ 保持原有的值返回，REPL 打印和普通表达式完美绿灯通行
    }

    // --- 处理字符串访问 ---
    if (container.type == TzdValue::STRING) {
        if (idx < 0 || idx >= (int)container.sVal.length()) {
            throw TzdRuntimeException("字符串索引越界: 尝试访问索引 " + std::to_string(idx) +
                ", 但字符串长度为 " + std::to_string(container.sVal.length()),
                ctx->getStart());
        }
        return TzdValue(std::string(1, container.sVal[idx]));
    }

    // --- 处理 map 访问 (key 为 string) ---
    if (container.type == TzdValue::MAP) {
        std::string key = getAsString(indexVal);
        auto it = container.mapVal.find(key);
        if (it == container.mapVal.end()) {
            return TzdValue();
        }
        return it->second;
    }

    throw TzdRuntimeException("类型错误: 该类型不支持下标访问", ctx->getStart());
}

std::any TzdInterpreter::visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) {
    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    parseParamList(ctx->paramList(), params, paramTypes);
    TzdValue funcVal("", params, ctx->block());
    funcVal.paramTypes = paramTypes;
    funcVal.type = TzdValue::FUNCTION;
    return funcVal;
}

// 在解释器开始处理一个新脚本时调用
void TzdInterpreter::prepareForScript() {
    // 为新脚本创建一个全新的编译器和模块
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "MainScriptModule");
    m_compiler->setupExternalFunctions(); // 只需要设置一次
    m_pendingJitFunctions.clear();
}

// In your interpreter, call this when you're ready to run code.
void TzdInterpreter::jitAllFunctions() {
    std::unique_ptr<llvm::Module> module = m_compiler->extractModule();
    if (module) {
        m_jitEngine->jitModule(std::move(module));
    }
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "new_main_module");
}

std::any TzdInterpreter::visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext* ctx) {
    std::string funcName = ctx->IDENTIFIER()->getText();
    std::string internalJitName = funcName + "_v" + std::to_string(m_funcVersion++);

    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    parseParamList(ctx->paramList(), params, paramTypes);

    TzdValue funcVal(funcName, params, ctx->block());
    funcVal.paramTypes = paramTypes;
    funcVal.type = TzdValue::FUNCTION;
    funcVal.jitInternalName = internalJitName;

    funcVal.sourceFile = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
    funcVal.line = (int)ctx->getStart()->getLine();
    funcVal.column = (int)ctx->getStart()->getCharPositionInLine();

    if (m_jitEngine && m_compiler) {
        m_compiler->compileNamedFunction(ctx->block(), ctx->paramList(), internalJitName);
        m_pendingJitFunctions.insert(internalJitName);
        m_jitNameToUserMap[internalJitName] = funcName;
    }

    // --- 修改回调通知 ---
    // 必须在覆盖 scopes 之前，通过旧对象触发回调
    if (scopes.back().count(funcName)) {
        emitFunctionRedefined(funcName, funcVal, ctx);
    }

    scopes.back()[funcName] = funcVal;
    return funcVal;
}

void TzdInterpreter::jitPendingModule() {
    if (!m_jitEngine || !m_compiler || m_pendingJitFunctions.empty()) return;

    // 1. 提取模块并加入 JIT 引擎 (注意：此过程会触发编译)
    auto TSM = m_compiler->extractThreadSafeModule();
    if (TSM) {
        m_jitEngine->addModule(std::move(TSM));
    }

    // 2. 遍历本次编译的所有符号 (内部名，如 add_v1)
    for (const std::string& symbol : m_pendingJitFunctions) {
        auto symOrErr = m_jitEngine->lookupSymbol(symbol);
        if (!symOrErr) {
            llvm::consumeError(symOrErr.takeError());
            continue;
        }

        // 获取生成的机器码地址
        void* addr = reinterpret_cast<void*>(symOrErr->getValue());

        // 3. 映射回用户原始名称 (如 add_v1 -> add)
        std::string userName = symbol;
        if (m_jitNameToUserMap.count(symbol)) {
            userName = m_jitNameToUserMap[symbol];
        }

        // 4. 处理类成员方法 (逻辑：ClassName_MethodName)
        // 只有当名字包含下划线，且前半部分是已注册类时，视为类方法
        size_t sep = userName.find('_');
        bool isClassMember = false;
        if (sep != std::string::npos) {
            std::string potentialCls = userName.substr(0, sep);
            if (TzdOopManager::getClass(potentialCls)) isClassMember = true;
        }

        if (isClassMember) {
            std::string clsName = userName.substr(0, sep);
            std::string methName = userName.substr(sep + 1);
            TzdClassDef* cls = TzdOopManager::getClass(clsName);

            // 修正构造函数特殊处理：ClassName_CtorName_ctor_N
            if (methName.find("_ctor_") != std::string::npos) {
                size_t suffixPos = methName.rfind("_ctor_");
                std::string ctorBase = methName.substr(0, suffixPos);
                int paramCount = std::stoi(methName.substr(suffixPos + 6));
                if (cls) {
                    for (auto& ctor : cls->constructors) {
                        if (ctor.paramCount == paramCount) {
                            ctor.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                        }
                    }
                    if (cls->methods.count(ctorBase)) {
                        cls->methods[ctorBase].jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                    }
                }
                continue;
            }
            if (methName.size() > 5 && methName.substr(methName.size() - 5) == "_ctor")
                methName = methName.substr(0, methName.size() - 5);

            if (cls && cls->methods.count(methName)) {
                // 更新类定义中的指针，所有实例都会同步生效
                cls->methods[methName].jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                // std::cout << "[JIT] Method " << clsName << "::" << methName << " redefined." << std::endl;
            }
        }
        else {
            // 5. 处理全局函数 (支持重定义的核心逻辑)
            // 必须从 scopes 的末尾（最顶层作用域）向前查找。
            // 这样能确保我们找到的是用户最后一次定义的那个 "add" 对象。
            bool linked = false;
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                auto findResult = it->find(userName);
                if (findResult != it->end() && findResult->second.type == TzdValue::FUNCTION) {
                    if (findResult->second.jitInternalName == symbol) {
                        findResult->second.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                        linked = true;
                        // std::cout << "[JIT] Success: Linked " << symbol << std::endl;
                    }
                    else {
                        // 将新生成的机器码地址写给最新的函数对象
                        findResult->second.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                        linked = true;
                        // std::cout << "[JIT] Function " << userName << " redefined -> " << symbol << std::endl;
                        
                    }
                    break;
                }
            }
        }
    }

    // 6. 清理状态
    m_pendingJitFunctions.clear();
    m_jitNameToUserMap.clear();
}



std::any TzdInterpreter::visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) {
    return visit(ctx->functionDeclaration());
}

std::any TzdInterpreter::visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext* ctx) {
    auto decl = ctx->nativeFunctionDeclaration();
    std::string funcName = decl->IDENTIFIER()->getText();

    // --- 1. 解析属性列表 ---
    std::unordered_map<std::string, std::string> attrs;
    if (decl->nativeAttrList()) {
        for (auto* attr : decl->nativeAttrList()->nativeAttr()) {
            // 修正：不要直接调 IDENTIFIER()，因为 key 可能是关键字
            // 获取第一个子节点（即等号左边的内容）
            std::string key = attr->children[0]->getText();

            std::string val = "";
            // 获取第三个子节点（即等号右边的内容）
            if (attr->children.size() >= 3) {
                val = attr->children[2]->getText();
            }

            // 去掉字符串引号
            if (val.size() >= 2 && (val.front() == '"' || val.front() == '\'')) {
                val = val.substr(1, val.size() - 2);
            }

            attrs[key] = val;
        }
    }

    // --- 2. 提取关键属性 ---
    std::string dllPath = attrs.count("dll") ? attrs["dll"] : "";
    std::string libFuncName = attrs.count("fun") ? attrs["fun"] : funcName;
    int prototype = attrs.count("prototype") ? std::stoi(attrs["prototype"]) : 0;
    std::string typeStr = attrs.count("type") ? attrs["type"] : "";
    std::string returnType = attrs.count("return") ? attrs["return"] : "float";

    // --- 3. 加载 DLL/函数地址 ---
#ifdef _WIN32
    HMODULE hLib = LoadLibraryA(dllPath.c_str());
    if (!hLib) throw TzdRuntimeException("Cannot load DLL: " + dllPath, ctx->start);
    void* procAddr = (void*)GetProcAddress(hLib, libFuncName.c_str());
#else
    void* hLib = dlopen(dllPath.c_str(), RTLD_LAZY);
    if (!hLib) throw TzdRuntimeException("Cannot load lib: " + dllPath, ctx->start);
    void* procAddr = dlsym(hLib, libFuncName.c_str());
#endif

    if (!procAddr) throw TzdRuntimeException("Function not found: " + libFuncName, ctx->start);

    TzdValue::NativeFuncType wrapper;

    // --- 4. 分支处理：原生模式 vs Prototype 转换模式 ---
    if (prototype == 1) {
        // 调用我们新写的类
        wrapper = TzdFFIAdapter::buildWrapper(procAddr, libFuncName,typeStr, returnType);

        if (!wrapper) {
            throw TzdRuntimeException("FFI Error: Unsupported signature " + typeStr + "->" + returnType, ctx->start);
        }
    }
    else {
        typedef TzdValue(*RawFunc)(const std::vector<TzdValue>&);
        RawFunc target = reinterpret_cast<RawFunc>(procAddr);
        wrapper = [target](const std::vector<TzdValue>& args) -> TzdValue {
            return target(args);
            };
    }

    setVariable(funcName, TzdValue(wrapper, funcName));
    return nullptr;
}

/*std::any TzdInterpreter::visitFunCallExpr(TzdLangParser::FunCallExprContext* ctx) {
    auto callCtx = ctx->functionCall();
    std::string funcName = callCtx->IDENTIFIER()->getText();
    TzdValue funcVal = getVariable(funcName,ctx);

    // 1. 准备参数 (无论是脚本函数还是原生函数都需要先计算参数)
    std::vector<TzdValue> args;
    if (callCtx->exprList()) {
        for (auto expr : callCtx->exprList()->expression()) {
            args.push_back(std::any_cast<TzdValue>(visit(expr)));
        }
    }

    // 2. 分情况处理
    if (funcVal.type == TzdValue::NATIVE_FUNCTION) {
        // --- 情况 A: 调用原生 C++ 函数 ---
        try {
            // 直接调用存储的 std::function
            return funcVal.nativeFunc(args);
        }
        catch (const std::exception& e) {
            throw TzdRuntimeException("调用原生函数 '" + funcName + "' 时发生错误: " + e.what(), ctx->getStart());
        }
    }
    else if (funcVal.type == TzdValue::FUNCTION) {
        // --- 情况 B: 调用脚本定义的函数 (原有逻辑) ---

        // 检查参数数量
        if (args.size() != funcVal.params.size()) {
            std::string err = "函数 '" + funcName + "' 调用参数不匹配: "
                "需要 " + std::to_string(funcVal.params.size()) + " 个，"
                "实际提供 " + std::to_string(args.size()) + " 个。";
            throw TzdRuntimeException(err, ctx->getStart());
        }

        // 创建新作用域并推入参数
        std::unordered_map<std::string, TzdValue> newScope;
        for (size_t i = 0; i < args.size(); ++i) {
            newScope[funcVal.params[i]] = args[i];
        }
        scopes.push_back(newScope);

        TzdValue returnValue;
        try {
            visit(funcVal.funcBody);
        }
        catch (const TzdReturnException& ret) {
            returnValue = ret.value;
        }
        scopes.pop_back();
        return returnValue;
    }
    else {
        throw TzdRuntimeException("尝试调用非函数对象: " + funcName, ctx->getStart());
    }
}*/

std::any TzdInterpreter::visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) {
    TzdValue val;
    if (ctx->expression()) {
        val = std::any_cast<TzdValue>(visit(ctx->expression()));
    }

    // compilation mode removed

    throw TzdReturnException(val);
}

std::any TzdInterpreter::visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext* ctx) {
    return visit(ctx->classDeclaration());
}

std::any TzdInterpreter::visitExprStmt(TzdLangParser::ExprStmtContext* ctx) {
    TzdValue val = castAnyToTzdValue(visit(ctx->expression()), "visitExprStmt");
    return val;
}

std::any TzdInterpreter::visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) {
    auto decl = ctx->variableDeclaration();
    std::string id = decl->IDENTIFIER()->getText();

    // --- 修复点 1：安全获取类型 ---
    std::string declaredType = "";
    bool isImplicit = false; // 标记是否为 var 推导类型

    if (decl->typeType()) {
        // 情况 A: 显式类型 (int a = 1;)
        declaredType = decl->typeType()->getText();
        if (!isTypeValid(declaredType)) {
            throw TzdRuntimeException("未定义的类型: '" + declaredType + "'", decl->typeType()->getStart());
        }
    }
    else {
        // 情况 B: 隐式类型 (var a = 1;)
        isImplicit = true;
    }

    TzdValue val;
    std::string c_init_code = "";

    // 计算初始化表达式
    if (decl->expression()) {
        val = std::any_cast<TzdValue>(visit(decl->expression()));
    }

    // --- 修复点 2：类型强制转换逻辑 (仅显式类型时执行) ---
    if (!isImplicit) {
        if (declaredType == "int" || declaredType == "long" || declaredType == "i32") {
            if (val.type != TzdValue::NONE) {
                long long intPart = (long long)getAsDouble(val);
                val = TzdValue(intPart);
            }
        }
        else if (declaredType == "float" || declaredType == "double") {
            if (val.type != TzdValue::NONE) {
                double floatPart = getAsDouble(val);
                val = TzdValue(floatPart);
            }
        }
        else {
            // 类实例校验
            TzdClassDef* cls = TzdOopManager::getClass(declaredType);
            if (cls && val.type == TzdValue::INSTANCE) {
                if (!TzdOopManager::isInstanceOf(val.instanceVal, declaredType)) {
                    throw TzdRuntimeException("类型不匹配: 无法将 " + val.instanceVal->definition->fullName + " 赋值给 " + declaredType, ctx->getStart());
                }
            }
        }
    }
    else {
        // var 声明：如果未初始化，给默认值 0
        if (val.type == TzdValue::NONE) val = TzdValue(0LL);
    }

    // =========================================================
    // --- 分支 2: 运行模式 (更新内存 + 捕获代码) ---
    // =========================================================

    // 更新运行时内存
    scopes.back()[id] = val;

    // 捕获 Top-Level C 代码 (用于 export_c_code)
    // compilation features removed; just return the runtime value
    return val;
}

std::any TzdInterpreter::visitIdExpr(TzdLangParser::IdExprContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    return getVariable(name, ctx);
}

std::any TzdInterpreter::visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) {
    // 1. 计算右侧表达式的值（由于上面恢复了，右边通过 visit 拿到的百分之百是标准的 TzdValue）
    std::any rAny = visit(ctx->expression(1));
    TzdValue rightVal = std::any_cast<TzdValue>(rAny);

    auto lhsCtx = ctx->expression(0);

    // 2. 运算符提取
    bool isCompound = true;
    std::string c_op_func = "";
    if (ctx->ASSIGN()) isCompound = false;
    else if (ctx->PLUS_ASSIGN()) c_op_func = "tzd_add";
    else if (ctx->MIN_ASSIGN()) c_op_func = "tzd_sub";
    else if (ctx->MUL_ASSIGN()) c_op_func = "tzd_mul";
    else if (ctx->DIV_ASSIGN()) c_op_func = "tzd_div";

    auto calculateCompound = [&](TzdValue oldV, TzdValue rightV) -> TzdValue {
        if (!isCompound) return rightV;
        if (ctx->PLUS_ASSIGN()) {
            if (oldV.type == TzdValue::STRING || rightV.type == TzdValue::STRING)
                return TzdValue(getAsString(oldV) + getAsString(rightV));
            return TzdValue(getAsDouble(oldV) + getAsDouble(rightV));
        }
        if (ctx->MIN_ASSIGN()) return TzdValue(getAsDouble(oldV) - getAsDouble(rightV));
        if (ctx->MUL_ASSIGN()) return TzdValue(getAsDouble(oldV) * getAsDouble(rightV));
        if (ctx->DIV_ASSIGN()) {
            if (getAsDouble(rightV) == 0) throw TzdRuntimeException("Division by zero", ctx->getStart());
            return TzdValue(getAsDouble(oldV) / getAsDouble(rightV));
        }
        return rightV;
        };

    // 3. ✨【核心黑科技：通过递归 Lambda 顺藤摸瓜，挖出符号表里最底层的物理地址指针】
    std::function<TzdValue* (antlr4::ParserRuleContext*)> getLValuePointer =
        [&](antlr4::ParserRuleContext* subCtx) -> TzdValue* {
        // 如果子节点依然是个下标访问表达式（支持多维数组如 matrix[0][1]）
        if (auto indexCtx = dynamic_cast<TzdLangParser::IndexExprContext*>(subCtx)) {
            TzdValue* containerPtr = getLValuePointer(indexCtx->expression(0));
            if (!containerPtr) return nullptr;

            TzdValue indexVal = std::any_cast<TzdValue>(visit(indexCtx->expression(1)));

            if (containerPtr->type == TzdValue::ARRAY) {
                int idx = (indexVal.type == TzdValue::DOUBLE) ? (int)indexVal.dVal : (int)indexVal.lVal;
                if (idx < 0 || idx >= (int)containerPtr->arrVal.size()) {
                    throw TzdRuntimeException("数组索引越界", indexCtx->getStart());
                }
                return &(containerPtr->arrVal[idx]);
            }
            if (containerPtr->type == TzdValue::MAP) {
                std::string key = getAsString(indexVal);
                return &(containerPtr->mapVal[key]);
            }
            return nullptr;
        }

        // 递归基底：普通的变量名标识符，直接去最真实的 scopes 里面抓取变量地址
        std::string id = subCtx->getText();
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(id)) return &((*it)[id]);
        }
        return &(scopes.back()[id]);
        };

    // 4. ✨【精准拦截下标赋值操作】
    if (dynamic_cast<TzdLangParser::IndexExprContext*>(lhsCtx)) {
        TzdValue* targetElemPtr = getLValuePointer(lhsCtx);
        if (targetElemPtr) {
            TzdValue finalVal = calculateCompound(*targetElemPtr, rightVal);
            *targetElemPtr = finalVal; // 🌟 顺着内存指针，直接将新值物理改写进作用域数组中！
            return finalVal;
        }
        throw TzdRuntimeException("无法对非法的下标位置进行赋值", ctx->getStart());
    }

    // 5. 处理对象属性访问或类静态数变量成员赋值 (保持你原本的代码不变)
    TzdLangParser::MemberAccessExprContext* memCtx = nullptr;
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx)) {
        memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomExpr->atom());
    }
    else {
        memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(lhsCtx);
    }

    if (memCtx) {
        TzdValue leftBase = std::any_cast<TzdValue>(visit(memCtx->atom()));
        std::string fieldName = memCtx->IDENTIFIER()->getText();

        if (leftBase.type == TzdValue::INSTANCE) {
            TzdValue oldVal = leftBase.instanceVal->getMember(fieldName);
            TzdValue finalVal = calculateCompound(oldVal, rightVal);
            leftBase.instanceVal->setMember(fieldName, finalVal);
            return finalVal;
        }
        else if (leftBase.type == TzdValue::CLASS_DEF) {
            TzdClassDef* cls = leftBase.classDefVal;
            TzdValue oldVal = cls->staticValues.count(fieldName) ? cls->staticValues[fieldName] : TzdValue(0.0);
            TzdValue finalVal = calculateCompound(oldVal, rightVal);
            cls->staticValues[fieldName] = finalVal;
            return finalVal;
        }
        throw TzdRuntimeException("Cannot assign to non-object member", ctx->getStart());
    }

    // 6. 普通纯变量赋值逻辑
    std::string id = lhsCtx->getText();

    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(id)) {
            TzdValue finalVal = calculateCompound((*it)[id], rightVal);
            (*it)[id] = finalVal;
            return finalVal;
        }
    }

    // 7. 面向对象隐式 `this` 查找与赋值
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto thisIt = it->find("this");
        if (thisIt != it->end() && thisIt->second.type == TzdValue::INSTANCE) {
            TzdInstance* inst = thisIt->second.instanceVal;
            TzdClassDef* curDef = inst->definition;
            bool isMember = false;
            while (curDef) {
                if (curDef->fields.count(id) || curDef->staticValues.count(id)) {
                    isMember = true; break;
                }
                if (curDef->parentName.empty()) break;
                curDef = TzdOopManager::getClass(curDef->parentName);
            }

            if (isMember) {
                TzdValue finalVal = calculateCompound(inst->getMember(id), rightVal);
                inst->setMember(id, finalVal);
                return finalVal;
            }
        }
    }

    // 8. 兜底隐式变量创建
    TzdValue finalVal = calculateCompound(TzdValue(0LL), rightVal);
    scopes.back()[id] = finalVal;
    return finalVal;
}

std::any TzdInterpreter::visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) {
    TzdValue left = std::any_cast<TzdValue>(visit(ctx->expression(0)));
    TzdValue right = std::any_cast<TzdValue>(visit(ctx->expression(1)));
    bool isPlus = ctx->PLUS() != nullptr;
    TzdValue result;

    // compilation mode removed

    // --- 运行时逻辑 (数值/字符串计算) ---
    if (isPlus && (left.type == TzdValue::STRING || right.type == TzdValue::STRING)) {
        // getAsString 现在处理的是剥离引号后的 sVal，不会再出现双引号
        result = TzdValue(getAsString(left) + getAsString(right));
    }
    else if (left.type == TzdValue::POINTER) {
        long long offset = (right.type == TzdValue::ULONG) ? (long long)right.ulVal : (long long)getAsDouble(right);
        result = TzdValue((void*)((char*)left.ptrVal + (isPlus ? offset : -offset)));
    }
    else {
        double l = getAsDouble(left), r = getAsDouble(right);
        result = TzdValue(isPlus ? l + r : l - r);
    }

    return result;
}

std::any TzdInterpreter::visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) {
    TzdValue left = std::any_cast<TzdValue>(visit(ctx->expression(0)));
    TzdValue right = std::any_cast<TzdValue>(visit(ctx->expression(1)));

    if (left.type == TzdValue::DOUBLE || right.type == TzdValue::DOUBLE || left.type == TzdValue::FLOAT || right.type == TzdValue::FLOAT) {
        double l = getAsDouble(left), r = getAsDouble(right);
        if (ctx->MUL()) return TzdValue(l * r);
        if (r == 0) return TzdValue(0.0);
        return ctx->DIV() ? TzdValue(l / r) : TzdValue(std::fmod(l, r));
    }

    if (left.type == TzdValue::ULONG || right.type == TzdValue::ULONG) {
        unsigned long long l = (left.type == TzdValue::ULONG) ? left.ulVal : (unsigned long long)left.lVal;
        unsigned long long r = (right.type == TzdValue::ULONG) ? right.ulVal : (unsigned long long)right.lVal;
        if (ctx->MUL()) return TzdValue(l * r);
        if (r == 0) return TzdValue(0ULL);
        return ctx->DIV() ? TzdValue(l / r) : TzdValue(l % r);
    }

    long long l = left.lVal, r = right.lVal;
    if (ctx->MUL()) return TzdValue(l * r);
    if (r == 0) return TzdValue(0LL);
    return ctx->DIV() ? TzdValue(l / r) : TzdValue(l % r);
}

std::any TzdInterpreter::visitPowerExpr(TzdLangParser::PowerExprContext* ctx) {
    return TzdValue(std::pow(getAsDouble(visit(ctx->expression(0))), getAsDouble(visit(ctx->expression(1)))));
}

std::any TzdInterpreter::visitParenExpr(TzdLangParser::ParenExprContext* ctx) {
    return visit(ctx->expression());
}

std::any TzdInterpreter::visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) {
    TzdValue val = castAnyToTzdValue(visit(ctx->expression()), "visitUnaryExpr");

    if (ctx->MINUS()) {
        if (val.type == TzdValue::LONG) return TzdValue(-val.lVal);
        return TzdValue(-val.dVal);
    }
    if (ctx->GXXX()) {
        double d = (val.type == TzdValue::LONG) ? (double)val.lVal : val.dVal;
        return TzdValue(std::sqrt(d));
    }
    if (ctx->NOT()) {
        return TzdValue(!isTruthy(val));
    }
    return val;
}

std::any TzdInterpreter::visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) {
    auto lhsCtx = ctx->expression();

    // 内部辅助 Lambda，用于正确地执行自增/自减
    auto performIncrement = [&](TzdValue& val) {
        if (val.type == TzdValue::LONG) {
            if (ctx->INC()) val.lVal++; else val.lVal--;
            val.dVal = static_cast<double>(val.lVal); // 保持同步
        }
        else if (val.type == TzdValue::FLOAT) {
            if (ctx->INC()) val.dVal++; else val.dVal--;
        }
        else {
            throw TzdRuntimeException("后缀自增/自减只能用于数值类型", ctx->getStart());
        }
        };

    // --- 情况 A: 成员访问 (myObj.field++ 或 MyClass.staticField++) ---
    if (auto atomCtx = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx)) {
        if (auto memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomCtx->atom())) {
            TzdValue leftObj = std::any_cast<TzdValue>(visit(memCtx->atom()));
            std::string fieldName = memCtx->IDENTIFIER()->getText();
            if (leftObj.type == TzdValue::INSTANCE) {
                TzdInstance* inst = leftObj.instanceVal;
                if (inst->fields.count(fieldName)) {
                    TzdValue& fieldRef = inst->fields.at(fieldName);
                    TzdValue oldVal = fieldRef;
                    performIncrement(fieldRef);
                    return oldVal;
                }
            }
            else if (leftObj.type == TzdValue::CLASS_DEF) {
                TzdClassDef* cls = leftObj.classDefVal;
                if (cls->staticValues.count(fieldName)) {
                    TzdValue& fieldRef = cls->staticValues.at(fieldName);
                    TzdValue oldVal = fieldRef;
                    performIncrement(fieldRef);
                    return oldVal;
                }
            }
            throw TzdRuntimeException("尝试对不存在的成员进行自增/自减: " + fieldName, ctx->getStart());
        }
    }

    // --- 情况 B: 数组索引 (arr[i]++) ---
    else if (auto indexCtx = dynamic_cast<TzdLangParser::IndexExprContext*>(lhsCtx)) {
        std::string arrayName = indexCtx->expression(0)->getText();
        int idx = (int)std::any_cast<TzdValue>(visit(indexCtx->expression(1))).dVal;
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(arrayName)) {
                TzdValue& arr = (*it)[arrayName];
                if (arr.type != TzdValue::ARRAY || idx < 0 || idx >= arr.arrVal.size()) {
                    throw TzdRuntimeException("数组索引越界或目标不是数组", ctx->getStart());
                }
                TzdValue& elemRef = arr.arrVal[idx];
                TzdValue oldVal = elemRef;
                performIncrement(elemRef);
                return oldVal;
            }
        }
        throw TzdRuntimeException("未找到数组: " + arrayName, ctx->getStart());
    }

    // --- 情况 C: 普通变量 (x++) ---
    std::string id = lhsCtx->getText();
    TzdValue oldVal = getVariable(id, ctx);
    TzdValue newVal = oldVal;
    performIncrement(newVal);
    setVariable(id, newVal);
    return oldVal;
}

std::any TzdInterpreter::visitForStmt(TzdLangParser::ForStmtContext* ctx) {
    scopes.push_back(std::unordered_map<std::string, TzdValue>());
    if (ctx->forInit()) visit(ctx->forInit());
    while (true) {
        if (ctx->cond) {
            TzdValue condition = std::any_cast<TzdValue>(visit(ctx->cond));
            if (!isTruthy(condition)) break;
        }
        try {
            if (ctx->statement()) visit(ctx->statement());
        }
        catch (const TzdBreakException&) {
            break;
        }
        catch (const TzdContinueException&) {
            // fall through to step
        }
        catch (const TzdReturnException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdThrowException&) {
            scopes.pop_back();
            throw;
        }
        if (ctx->step) visit(ctx->step);
    }
    scopes.pop_back();
    return TzdValue();
}

std::any TzdInterpreter::visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) {
    auto lhsCtx = ctx->expression();

    auto performIncrement = [&](TzdValue& val) -> TzdValue& {
        if (val.type == TzdValue::LONG) {
            if (ctx->INC()) val.lVal++; else val.lVal--;
            val.dVal = static_cast<double>(val.lVal);
        }
        else if (val.type == TzdValue::FLOAT) {
            if (ctx->INC()) val.dVal++; else val.dVal--;
        }
        else {
            throw TzdRuntimeException("前缀自增/自减只能用于数值类型", ctx->getStart());
        }
        return val;
        };

    if (auto atomCtx = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx)) {
        if (auto memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomCtx->atom())) {
            TzdValue leftObj = std::any_cast<TzdValue>(visit(memCtx->atom()));
            std::string fieldName = memCtx->IDENTIFIER()->getText();
            if (leftObj.type == TzdValue::INSTANCE) {
                TzdInstance* inst = leftObj.instanceVal;
                if (inst->fields.count(fieldName)) {
                    return performIncrement(inst->fields.at(fieldName));
                }
            }
            else if (leftObj.type == TzdValue::CLASS_DEF) {
                TzdClassDef* cls = leftObj.classDefVal;
                if (cls->staticValues.count(fieldName)) {
                    return performIncrement(cls->staticValues.at(fieldName));
                }
            }
            throw TzdRuntimeException("尝试对不存在的成员进行自增/自减: " + fieldName, ctx->getStart());
        }
    }

    // --- 省略对数组索引的前缀操作，如果需要可以添加 ---

    // --- 情况 C: 普通变量 (++x) ---
    std::string id = lhsCtx->getText();
    TzdValue val = getVariable(id, ctx);
    performIncrement(val);
    setVariable(id, val);
    return val;
}

std::any TzdInterpreter::visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) {
    while (true) {
        TzdValue condition = std::any_cast<TzdValue>(visit(ctx->expression()));
        if (!isTruthy(condition)) break;

        try {
            visit(ctx->statement());
        }
        catch (const TzdBreakException&) {
            break;
        }
        catch (const TzdContinueException&) {
            continue;
        }
        catch (const TzdReturnException&) {
            throw;
        }
        catch (const TzdThrowException&) {
            throw;
        }
    }
    return std::any(TzdValue());
}

std::any TzdInterpreter::visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) {
    (void)ctx;
    throw TzdBreakException();
}

std::any TzdInterpreter::visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) {
    (void)ctx;
    throw TzdContinueException();
}

std::any TzdInterpreter::visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) {
    TzdValue switchVal = std::any_cast<TzdValue>(visit(ctx->expression()));
    bool matched = false;

    for (auto* caseCtx : ctx->switchCase()) {
        TzdValue caseVal = std::any_cast<TzdValue>(visit(caseCtx->expression()));
        if (!matched && valuesEqual(switchVal, caseVal)) {
            matched = true;
        }
        if (matched) {
            for (auto* stmt : caseCtx->statement()) {
                try {
                    visit(stmt);
                }
                catch (const TzdBreakException&) {
                    return TzdValue();
                }
                catch (const TzdContinueException&) {
                    throw;
                }
                catch (const TzdReturnException&) {
                    throw;
                }
            }
        }
    }

    if (ctx->switchDefault()) {
        if (!matched) matched = true;
        if (matched) {
            for (auto* stmt : ctx->switchDefault()->statement()) {
                try {
                    visit(stmt);
                }
                catch (const TzdBreakException&) {
                    return TzdValue();
                }
                catch (const TzdContinueException&) {
                    throw;
                }
                catch (const TzdReturnException&) {
                    throw;
                }
            }
        }
    }
    return TzdValue();
}


std::any TzdInterpreter::visitIfStmt(TzdLangParser::IfStmtContext* ctx) {
    TzdValue condition = std::any_cast<TzdValue>(visit(ctx->expression()));

    if (isTruthy(condition)) {
        return visit(ctx->statement(0));
    }
    else if (ctx->KW_ELSE()) {
        return visit(ctx->statement(1));
    }

    return TzdValue();
}

std::any TzdInterpreter::visitForInit(TzdLangParser::ForInitContext* ctx) {
    if (ctx->variableDeclaration()) return visit(ctx->variableDeclaration());
    if (ctx->expression()) return visit(ctx->expression());
    return TzdValue();
}

std::any TzdInterpreter::visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) {
    double l = getAsDouble(visit(ctx->expression(0)));
    double r = getAsDouble(visit(ctx->expression(1)));
    if (ctx->GT()) return TzdValue(l > r);
    if (ctx->LT()) return TzdValue(l < r);
    if (ctx->GE()) return TzdValue(l >= r);
    return TzdValue(l <= r);
}

std::any TzdInterpreter::visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) {
    TzdValue l = castAnyToTzdValue(visit(ctx->expression(0)), "visitEqualityExpr.l");
    TzdValue r = castAnyToTzdValue(visit(ctx->expression(1)), "visitEqualityExpr.r");
    bool eq = valuesEqual(l, r);
    return TzdValue(ctx->EEQ() ? eq : !eq);
}

std::any TzdInterpreter::visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) {
    TzdValue l = castAnyToTzdValue(visit(ctx->expression(0)), "visitLogicalAndExpr.l");
    if (!isTruthy(l)) return TzdValue(bool{ false });
    TzdValue r = castAnyToTzdValue(visit(ctx->expression(1)), "visitLogicalAndExpr.r");
    return TzdValue(bool{ isTruthy(r) });
}

std::any TzdInterpreter::visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) {
    TzdValue l = castAnyToTzdValue(visit(ctx->expression(0)), "visitLogicalOrExpr.l");
    if (isTruthy(l)) return TzdValue(bool{ true });
    TzdValue r = castAnyToTzdValue(visit(ctx->expression(1)), "visitLogicalOrExpr.r");
    return TzdValue(bool{ isTruthy(r) });
}

std::any TzdInterpreter::visitCastExpr(TzdLangParser::CastExprContext* ctx) {
    std::string targetType = ctx->typeType()->getText();
    TzdValue val = std::any_cast<TzdValue>(visit(ctx->expression()));

    try {
        if (targetType == "int" || targetType == "i32" || targetType == "long" || targetType == "i64") {
            long long res;
            if (val.type == TzdValue::FLOAT || val.type == TzdValue::DOUBLE) res = (long long)val.dVal;
            else if (val.type == TzdValue::ULONG) res = (long long)val.ulVal;
            else if (val.type == TzdValue::POINTER) res = (long long)(uintptr_t)val.ptrVal;
            else if (val.type == TzdValue::STRING) res = std::stoll(val.sVal, nullptr, 0);
            else if (val.type == TzdValue::BOOL) res = val.bVal ? 1LL : 0LL;
            else res = val.lVal;
            return (targetType == "int" || targetType == "i32") ? TzdValue((int)res) : TzdValue(res);
        }
        else if (targetType == "byte" || targetType == "u8" || targetType == "ulong" || targetType == "u64") {
            unsigned long long res;
            if (val.type == TzdValue::FLOAT || val.type == TzdValue::DOUBLE) res = (unsigned long long)val.dVal;
            else if (val.type == TzdValue::ULONG) res = val.ulVal;
            else if (val.type == TzdValue::POINTER) res = (uintptr_t)val.ptrVal;
            else if (val.type == TzdValue::STRING) res = std::stoull(val.sVal, nullptr, 0);
            else res = (unsigned long long)val.lVal;
            return (targetType == "byte" || targetType == "u8") ? TzdValue((unsigned char)res) : TzdValue(res);
        }
        else if (targetType == "ptr" || targetType == "pointer" || targetType == "hwnd") {
            if (val.type == TzdValue::ULONG) return TzdValue((void*)val.ulVal);
            if (val.type == TzdValue::STRING) return TzdValue((void*)std::stoull(val.sVal, nullptr, 0));
            return TzdValue((void*)(uintptr_t)getAsDouble(val));
        }
        else if (targetType == "float" || targetType == "double") {
            return TzdValue(getAsDouble(val));
        }
        else if (targetType == "string") return TzdValue(getAsString(val));
        else if (targetType == "bool") return TzdValue(getAsDouble(val) != 0);
    }
    catch (...) {
        throw TzdRuntimeException("[编译错误] 无法转换 '" + getAsString(val) + "' 为 " + targetType, ctx->getStart());
    }
    throw TzdRuntimeException("[编译错误] 不支持的目标类型： " + targetType, ctx->getStart());
}

// --- 字面量 ---
std::any TzdInterpreter::visitIntExpr(TzdLangParser::IntExprContext* ctx) {
    std::string raw = ctx->getText();
    TzdValue res;
    try {
        if (raw.size() > 2 && (raw.substr(0, 2) == "0x" || raw.substr(0, 2) == "0X")) {
            res = TzdValue((void*)std::stoull(raw, nullptr, 16));
        }
        else {
            res = TzdValue(std::stoll(raw));
        }
    }
    catch (...) {
        res = TzdValue(std::stod(raw));
    }

    return res;
}
std::any TzdInterpreter::visitFloatExpr(TzdLangParser::FloatExprContext* ctx) {
    TzdValue res(std::stod(ctx->getText()));
    return res;
}
std::any TzdInterpreter::visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) {
    TzdValue res(true);
    return res;
}
std::any TzdInterpreter::visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) { return TzdValue(false); }
std::any TzdInterpreter::visitStringExpr(TzdLangParser::StringExprContext* ctx) {
    std::string s = ctx->getText();
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'')) {
        s = s.substr(1, s.size() - 2);
    }
    TzdValue res(unescapeString(s));
    return res;
}

std::any TzdInterpreter::visitImportStmt(TzdLangParser::ImportStmtContext* ctx) {
    std::string rawPath = ctx->importStatement()->STRING()->getText();
    if (rawPath.size() >= 2) rawPath = rawPath.substr(1, rawPath.size() - 2);

    std::string absolutePath = resolveImportPath(rawPath);

    if (absolutePath.empty()) {
        throw TzdRuntimeException("[Import Error] 找不到模块或文件: " + rawPath, ctx->getStart());
    }

    if (m_importedFiles.count(absolutePath)) {
        return TzdValue(true);
    }

    m_importedFiles.insert(absolutePath);
    this->loadScriptFromFile(absolutePath);

    return TzdValue(true);
}

// --- Print & Math ---
std::any TzdInterpreter::visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) {
    auto exprList = ctx->printFunction()->exprList();
    if (exprList) {
        auto expressions = exprList->expression();
        for (size_t i = 0; i < expressions.size(); ++i) {
            std::any res = visit(expressions[i]);
            std::string utf8Str = getAsString(res);
            std::string consoleStr = Utf8ToAnsi(utf8Str);
            std::cout << consoleStr;
            if (i < expressions.size() - 1) {
                std::cout << " ";
            }
        }
    }
    std::cout << std::endl;
    return TzdValue();
}

std::any TzdInterpreter::visitClassDeclaration(TzdLangParser::ClassDeclarationContext* ctx) {
    // --- 1. 获取基础信息 ---
    std::string fullName = ctx->qualifiedName(0)->getText();
    std::string parentName = (ctx->qualifiedName().size() > 1) ? ctx->qualifiedName(1)->getText() : "";

    // --- 2. 校验与安全检查 ---
    if (TzdOopManager::getClass(fullName)) {
        throw TzdRuntimeException("类重定义: '" + fullName + "' 已存在", ctx->getStart());
    }

    TzdClassDef* newClass = new TzdClassDef(fullName);
    newClass->parentName = parentName;

    // 处理类级注解
    if (ctx->annotationUsage()) {
        validateAnnotationUsage(ctx->annotationUsage());
        newClass->annotations.push_back(ctx->annotationUsage()->IDENTIFIER()->getText());
    }

    // --- 3. 初始化 C 代码生成器变量 ---
    TzdValue classVal(newClass);
    classVal.annotations = newClass->annotations;

    // --- 4. 开启编译模式 (防止 visit 方法体时报错) ---
    bool oldMode = m_isCompiling;
    m_isCompiling = true;

    // --- 5. 遍历解析类成员 ---
    for (auto member : ctx->classBody()->classMember()) {

        // A. 校验成员上的注解
        if (member->annotationUsage()) {
            validateAnnotationUsage(member->annotationUsage());
        }

        // B. 处理原生方法 (Native Function)
        if (member->nativeFunctionDeclaration()) {
            auto nDecl = member->nativeFunctionDeclaration();
            std::string funcName = nDecl->IDENTIFIER()->getText();
            std::unordered_map<std::string, std::string> attrs;
            if (nDecl->nativeAttrList()) {
                for (auto* attr : nDecl->nativeAttrList()->nativeAttr()) {
                    std::string key = attr->children[0]->getText();
                    std::string val = attr->children.size() >= 3 ? attr->children[2]->getText() : "";
                    if (val.size() >= 2) val = val.substr(1, val.size() - 2);
                    attrs[key] = val;
                }
            }

            void* procAddr = nullptr;
            std::string dll = attrs["dll"];
            std::string realFun = attrs.count("fun") ? attrs["fun"] : funcName;
#ifdef _WIN32
            HMODULE hLib = LoadLibraryA(dll.c_str());
            if (hLib) procAddr = (void*)GetProcAddress(hLib, realFun.c_str());
#endif
            if (!procAddr) throw TzdRuntimeException("FFI 错误: 无法加载原生方法 " + realFun, nDecl->getStart());

            ClassMethod m;
            m.name = funcName;
            m.isNative = true;
            m.isStatic = (attrs["static"] == "true");
            m.nativeWrapper = TzdFFIAdapter::buildWrapper(procAddr, realFun, attrs["type"], attrs["return"]);
            newClass->methods[funcName] = m;
            continue;
        }

        auto decl = member->memberDecl();
        if (!decl) continue;

        // C. 处理实例字段 (var)
        if (auto varCtx = dynamic_cast<TzdLangParser::FieldVarDeclContext*>(decl)) {
            std::string typeName = varCtx->typeType()->getText();
            std::string fieldName = varCtx->IDENTIFIER()->getText();
            if (!isTypeValid(typeName) && typeName != fullName && typeName != newClass->simpleName) {
                throw TzdRuntimeException("未知类型: " + typeName, varCtx->typeType()->getStart());
            }

            ClassField f; f.name = fieldName; f.type = typeName;
            if (varCtx->expression()) f.initExpr = varCtx->expression();
            newClass->fields[fieldName] = f;
        }

        // D. 处理静态字段 (let)
        else if (auto letCtx = dynamic_cast<TzdLangParser::FieldLetDeclContext*>(decl)) {
            std::string typeName = letCtx->typeType()->getText();
            std::string fieldName = letCtx->IDENTIFIER()->getText();
            if (!isTypeValid(typeName)) throw TzdRuntimeException("未知类型: " + typeName, letCtx->typeType()->getStart());

            ClassField f; f.name = fieldName; f.type = typeName; f.isStatic = true;
            newClass->fields[fieldName] = f;

            TzdValue staticVal(0LL);
            if (letCtx->expression()) {
                staticVal = std::any_cast<TzdValue>(visit(letCtx->expression()));
            }
            newClass->staticValues[fieldName] = staticVal;
        }

        // E. 处理普通方法 (fun)
        else if (auto methodCtx = dynamic_cast<TzdLangParser::MethodDeclContext*>(decl)) {
            std::string mName = methodCtx->IDENTIFIER()->getText();
            ClassMethod m;
            m.name = mName;
            m.body = methodCtx->block();
            parseParamList(methodCtx->paramList(), m.params, m.paramTypes);

            m.sourceFile = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
            m.line = (int)methodCtx->getStart()->getLine();
            m.column = (int)methodCtx->getStart()->getCharPositionInLine();

            // --- JIT 编译部分 ---
            if (m_jitEngine && m_compiler) {
                std::string jitName = fullName + "_" + mName;
                try {
                    m_compiler->compileClassMethod(ctx, methodCtx, jitName);
                    m_pendingJitFunctions.insert(jitName);
                }
                catch (const std::exception& e) {
                    agentLogCompile("C", "compileClassMethod", (jitName + ": " + e.what()).c_str());
                    throw;
                }
            }

            newClass->methods[mName] = m;
        }

        // F. 处理静态方法 (static fun)
        else if (auto smCtx = dynamic_cast<TzdLangParser::MethodStaticDeclContext*>(decl)) {
            std::string mName = smCtx->IDENTIFIER()->getText();
            ClassMethod m;
            m.name = mName;
            m.isStatic = true;
            m.body = smCtx->block();
            parseParamList(smCtx->paramList(), m.params, m.paramTypes);

            // --- JIT 编译部分 ---
            if (m_jitEngine && m_compiler) {
                std::string jitName = fullName + "_" + mName;
                // 静态方法不需要 this
                m_compiler->compileNamedFunction(smCtx->block(), smCtx->paramList(), jitName);
                m_pendingJitFunctions.insert(jitName);
            }

            newClass->methods[mName] = m;
        }

        // G. 处理构造函数（支持重载：按参数个数区分）
        else if (auto ctorCtx = dynamic_cast<TzdLangParser::ConstructorDeclContext*>(decl)) {
            std::string ctorName = ctorCtx->IDENTIFIER()->getText();
            if (ctorName != newClass->simpleName) {
                throw TzdRuntimeException("构造函数名 '" + ctorName + "' 必须与类名一致", ctorCtx->getStart());
            }

            ClassConstructor ctorInfo;
            ctorInfo.declCtx = ctorCtx;
            ctorInfo.body = ctorCtx->block();
            parseParamList(ctorCtx->paramList(), ctorInfo.params, ctorInfo.paramTypes);
            ctorInfo.paramCount = (int)ctorInfo.params.size();
            ctorInfo.sourceFile = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
            ctorInfo.line = (int)ctorCtx->getStart()->getLine();
            ctorInfo.column = (int)ctorCtx->getStart()->getCharPositionInLine();

            for (const auto& existing : newClass->constructors) {
                if (existing.paramCount == ctorInfo.paramCount) {
                    throw TzdRuntimeException(
                        "构造函数重载冲突: '" + fullName + "' 已有 " +
                        std::to_string(ctorInfo.paramCount) + " 个参数的构造函数",
                        ctorCtx->getStart());
                }
            }

            if (m_jitEngine && m_compiler) {
                std::string jitName = fullName + "_" + ctorName + "_ctor_" + std::to_string(ctorInfo.paramCount);
                ctorInfo.jitSymbolName = jitName;
                try {
                    m_compiler->compileConstructor(ctx, ctorCtx, jitName);
                    m_pendingJitFunctions.insert(jitName);
                }
                catch (const std::exception& e) {
                    agentLogCompile("C", "compileConstructor", (jitName + ": " + e.what()).c_str());
                    throw;
                }
            }

            ClassMethod m;
            m.name = ctorName;
            m.body = ctorCtx->block();
            m.params = ctorInfo.params;
            m.sourceFile = ctorInfo.sourceFile;
            m.line = ctorInfo.line;
            m.column = ctorInfo.column;

            newClass->methods[ctorName] = m;

            newClass->constructors.push_back(std::move(ctorInfo));
        }
    }

    std::string classSource = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
    for (auto& ctor : newClass->constructors) {
        if (ctor.sourceFile.empty() || ctor.sourceFile == "memory") {
            ctor.sourceFile = classSource;
        }
    }
    for (auto& [name, method] : newClass->methods) {
        if (method.sourceFile.empty() || method.sourceFile == "memory") {
            method.sourceFile = classSource;
        }
    }

    // --- 7. 注册并存入作用域 ---
    TzdOopManager::registerClass(newClass);
    setVariable(fullName, classVal);

    if (newClass->simpleName != fullName) {
        setVariable(newClass->simpleName, classVal);
    }

    return classVal;
}

void TzdInterpreter::validateAnnotationUsage(TzdLangParser::AnnotationUsageContext* ctx) {
    if (!ctx) return;

    std::string annoName = ctx->IDENTIFIER()->getText();
    TzdClassDef* cls = TzdOopManager::getClass(annoName);

    if (!cls || !cls->isAnnotation) {
        throw TzdRuntimeException(
            "未定义的注解: '@" + annoName + "'。请先定义该注解，例如: class @" + annoName + "();",
            ctx->getStart()
        );
    }
}

bool TzdInterpreter::isTypeValid(const std::string& typeName) {
    // 1. 基础类型白名单
    static const std::unordered_set<std::string> primitives = {
         "int", "float", "string", "bool", "long", "ptr", "void",
         "i32", "i64", "double", "byte", "u8", "ulong", "hwnd",
         "function", "fn"
    };
    if (primitives.count(typeName)) return true;

    // 2. 检查是否是已注册的类、枚举或注解
    if (TzdOopManager::getClass(typeName) != nullptr) return true;

    return false;
}

bool TzdInterpreter::checkParamValueType(const std::string& typeName, const TzdValue& val) {
    if (typeName.empty()) return true;
    if (typeName == "int" || typeName == "i32" || typeName == "long" || typeName == "i64" ||
        typeName == "float" || typeName == "double" || typeName == "byte" || typeName == "u8") {
        return val.type == TzdValue::INT || val.type == TzdValue::LONG ||
            val.type == TzdValue::FLOAT || val.type == TzdValue::DOUBLE ||
            val.type == TzdValue::SHORT || val.type == TzdValue::BYTE;
    }
    if (typeName == "function" || typeName == "fn") {
        return val.type == TzdValue::FUNCTION || val.type == TzdValue::NATIVE_FUNCTION;
    }
    if (typeName == "string") return val.type == TzdValue::STRING;
    if (typeName == "bool") return val.type == TzdValue::BOOL;
    if (typeName == "ptr" || typeName == "void" || typeName == "hwnd")
        return val.type == TzdValue::POINTER || val.type == TzdValue::NONE;
    if (val.type == TzdValue::INSTANCE && val.instanceVal && val.instanceVal->definition) {
        return val.instanceVal->definition->fullName == typeName ||
            val.instanceVal->definition->simpleName == typeName ||
            val.instanceVal->definition->isSubclassOf(typeName);
    }
    if (val.type == TzdValue::CLASS_DEF && val.classDefVal) {
        return val.classDefVal->fullName == typeName || val.classDefVal->simpleName == typeName;
    }
    return false;
}

// --- 实例化 (New) ---
std::any TzdInterpreter::visitNewExpr(TzdLangParser::NewExprContext* ctx) {
    if (!ctx->qualifiedName()) throw TzdRuntimeException("New 表达式缺失类名", ctx->getStart());
    std::string className = ctx->qualifiedName()->getText();

    TzdClassDef* cls = TzdOopManager::getClass(className);
    if (!cls) throw TzdRuntimeException("找不到类定义: " + className, ctx->getStart());

    TzdInstance* inst = new TzdInstance(cls);
    TzdValue instVal(inst);

    std::unordered_map<std::string, TzdValue> ctorScope;
    ctorScope["this"] = instVal;
    scopes.push_back(ctorScope);

    try {
        // --- 执行字段初始化器 (变量定义时的赋值) ---
        TzdClassDef* cur = cls;
        std::vector<TzdClassDef*> hierarchy;
        while (cur) {
            hierarchy.insert(hierarchy.begin(), cur);
            if (cur->parentName.empty()) break;
            cur = TzdOopManager::getClass(cur->parentName);
        }
        for (auto* currentCls : hierarchy) {
            for (auto const& [name, field] : currentCls->fields) {
                if (!field.isStatic && field.initExpr) {
                    TzdValue initVal = std::any_cast<TzdValue>(visit(field.initExpr));
                    inst->fields[name] = initVal;
                    scopes.back()[name] = initVal;
                }
            }
        }

        // --- 执行构造函数（按实参个数匹配重载） ---
        std::vector<TzdValue> args;
        if (ctx->exprList()) {
            for (auto expr : ctx->exprList()->expression()) {
                args.push_back(std::any_cast<TzdValue>(visit(expr)));
            }
        }

        ClassConstructor* ctor = cls->findConstructor(args.size());
        if (!ctor && args.empty() && !cls->constructors.empty()) {
            ctor = cls->findConstructor(0);
        }
        if (!ctor && !cls->constructors.empty()) {
            throw TzdRuntimeException(
                "找不到匹配的构造函数: '" + className + "' 需要 " +
                std::to_string(args.size()) + " 个参数",
                ctx->getStart());
        }
        if (ctor) {
            TzdValue ctorFunc(cls->simpleName, ctor->params, ctor->body);
            ctorFunc.jittedPtr = ctor->jittedPtr;
            ctorFunc.instanceVal = inst;
            this->callFunction(ctorFunc, args);
        }
    }
    catch (...) {
        scopes.pop_back();
        throw;
    }
    scopes.pop_back();
    return instVal;
}

// --- 成员访问 (.) ---
std::any TzdInterpreter::visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) {

    // 1. 获取左侧 atom 的原始文本 (比如 "this" 或 "t2")
    std::string leftText = ctx->atom()->getText();

    // 2. 解析左侧基础对象
    TzdValue base;
    bool baseResolved = false;
    try {
        std::any val = visit(ctx->atom());
        if (val.has_value()) {
            base = std::any_cast<TzdValue>(val);
            baseResolved = true;
        }
    }
    catch (...) {
        baseResolved = false;
    }

    std::string memberName = ctx->IDENTIFIER()->getText();

    if (baseResolved) {
        // --- 情况 A: 静态成员访问 (Class.staticVar) ---
        if (base.type == TzdValue::CLASS_DEF && base.classDefVal) {
            TzdClassDef* cls = base.classDefVal;
            // 找静态变量
            if (cls->staticValues.count(memberName)) {
                TzdValue res = cls->staticValues[memberName];
                return res;
            }
            // 找静态方法
            ClassMethod* method = cls->findMethod(memberName);
            if (method && method->isStatic) {
                TzdValue f;
                f.type = TzdValue::FUNCTION;
                f.name = method->name;
                f.params = method->params;
                f.funcBody = method->body;
                return f;
            }
        }

        // --- 情况 B: 实例成员访问 (obj.var 或 this.var) ---
        // 关键点：不仅检查 INSTANCE 类型，还检查 instanceVal 是否存在（防止类型标识丢失）
        if ((base.type == TzdValue::INSTANCE || base.instanceVal != nullptr) && base.type != TzdValue::CLASS_DEF) {
            try {
                return base.instanceVal->getMember(memberName);
            }
            catch (const std::exception& e) {
                // 如果 getMember 抛出错误，直接向上抛出详细的 OOP 错误，而不是被末尾覆盖
                throw TzdRuntimeException(e.what(), ctx->getStart());
            }
        }
    }

    // --- 情况 C: 符号降级（处理包名 org.tzd.Test） ---
    std::string combinedPath = leftText + "." + memberName;
    TzdClassDef* cls = TzdOopManager::getClass(combinedPath);
    if (cls) {
        TzdValue classRes(cls);
        return classRes;
    }

    // --- 情况 D: 最终报错 ---
    throw TzdRuntimeException("无法解析符号: " + (baseResolved ? (leftText + "." + memberName) : combinedPath), ctx->getStart());
}

// --- 类型检查 (in) ---
std::any TzdInterpreter::visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) {
    TzdValue obj = std::any_cast<TzdValue>(visit(ctx->expression()));
    // 获取 targetType，例如 "int" 或 "float"
    std::string targetType = ctx->qualifiedName() ? ctx->qualifiedName()->getText() : ctx->typeType()->getText();
    std::string targetName = ctx->qualifiedName() ? ctx->qualifiedName()->getText() : ctx->typeType()->getText();

    // 1. 检查目标是否是一个“注解类”
    TzdClassDef* targetCls = TzdOopManager::getClass(targetName);
    bool isTargetAnnotation = (targetCls && targetCls->isAnnotation);

    if (isTargetAnnotation) {
        // --- 【关键修复：判断注解包含关系】 ---
        for (const auto& anno : obj.annotations) {
            if (anno == targetName) return TzdValue(true);
        }
        return TzdValue(false);
    }

    // 1. 严格检查浮点类型
    if (targetType == "float" || targetType == "double") {
        // 只有值本身确实是 FLOAT 或 DOUBLE 类型时才返回 true
        return TzdValue(obj.type == TzdValue::FLOAT || obj.type == TzdValue::DOUBLE);
    }

    // 2. 严格检查整数类型
    if (targetType == "int" || targetType == "long" || targetType == "i32" || targetType == "i64") {
        // 检查是否属于整数枚举范围 (SBYTE 到 ULONG)
        return TzdValue(obj.type >= TzdValue::SBYTE && obj.type <= TzdValue::ULONG);
    }

    // 3. 检查字符串
    if (targetType == "string") return TzdValue(obj.type == TzdValue::STRING);

    // 4. 类实例检查 (必须使用我们在上一步修复的严格 isInstanceOf)
    if (obj.type == TzdValue::INSTANCE) {
        return TzdValue(TzdOopManager::isInstanceOf(obj.instanceVal, targetType));
    }

    return TzdValue(false);
}

void TzdInterpreter::checkSymbolCollision(const std::string& name, antlr4::ParserRuleContext* ctx) {
    // 1. 检查当前作用域是否有同名变量/函数
    if (scopes.back().count(name)) {
        throw TzdRuntimeException("符号重定义: '" + name + "' 已在当前作用域定义", ctx->getStart());
    }
    // 2. 检查是否与已注册的类名/枚举名/注解名冲突
    if (TzdOopManager::getClass(name)) {
        throw TzdRuntimeException("符号冲突: '" + name + "' 已被定义为类、枚举或注解", ctx->getStart());
    }
}

// --- 注解声明 ---
std::any TzdInterpreter::visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    TzdClassDef* anno = new TzdClassDef(name);
    anno->isAnnotation = true;

    TzdOopManager::registerClass(anno);
    setVariable(name, TzdValue(anno));
    return TzdValue();
}

// --- 枚举 ---
std::any TzdInterpreter::visitEnumDeclaration(TzdLangParser::EnumDeclarationContext* ctx) {
    std::string enumName = ctx->IDENTIFIER()->getText();
    TzdClassDef* enumClass = new TzdClassDef(enumName);
    enumClass->isEnum = true;

    if (ctx->enumList()) {
        int idx = 0;
        for (auto idNode : ctx->enumList()->IDENTIFIER()) {
            std::string itemName = idNode->getText();
            TzdValue itemVal((long long)idx++);
            enumClass->staticValues[itemName] = itemVal;
        }
    }
    TzdOopManager::registerClass(enumClass);
    setVariable(enumName, TzdValue(enumClass));
    return TzdValue();
}

// --- Super 调用 (实现 visitSuperExpr) ---
std::any TzdInterpreter::visitSuperExpr(TzdLangParser::SuperExprContext* ctx) {
    antlr4::tree::ParseTree* p = ctx->parent;
    TzdLangParser::ClassDeclarationContext* classDeclCtx = nullptr;

    // 向上查找当前所在的类定义
    while (p != nullptr) {
        classDeclCtx = dynamic_cast<TzdLangParser::ClassDeclarationContext*>(p);
        if (classDeclCtx) break;
        p = p->parent;
    }

    if (!classDeclCtx) {
        throw TzdRuntimeException("super() 必须在类定义的内部使用", ctx->getStart());
    }

    // 修改：使用 qualifiedName(0) 获取当前类全名
    std::string currentClassName = classDeclCtx->qualifiedName(0)->getText();
    TzdClassDef* currentClass = TzdOopManager::getClass(currentClassName);

    if (!currentClass || currentClass->parentName.empty()) {
        throw TzdRuntimeException("类 '" + currentClassName + "' 没有父类，无法调用 super()", ctx->getStart());
    }

    TzdClassDef* parentClass = TzdOopManager::getClass(currentClass->parentName);
    if (!parentClass) {
        throw TzdRuntimeException("找不到父类定义: " + currentClass->parentName, ctx->getStart());
    }

    TzdValue thisVal;
    try {
        thisVal = getVariable("this", ctx);
    }
    catch (...) {
        throw TzdRuntimeException("super() 调用上下文丢失 'this' 指针", ctx->getStart());
    }

    if (parentClass->constructors.empty()) {
        if (ctx->exprList() && !ctx->exprList()->expression().empty()) {
            throw TzdRuntimeException("父类 '" + parentClass->fullName + "' 无构造函数，但 super() 传入了参数", ctx->getStart());
        }
        return TzdValue();
    }

    std::vector<TzdValue> args;
    if (ctx->exprList()) {
        for (auto expr : ctx->exprList()->expression()) {
            args.push_back(std::any_cast<TzdValue>(visit(expr)));
        }
    }

    const ClassConstructor* parentCtor = parentClass->findConstructor(args.size());
    if (!parentCtor && !parentClass->constructors.empty()) {
        throw TzdRuntimeException("super() 找不到匹配的父类构造函数", ctx->getStart());
    }
    if (!parentCtor) {
        return TzdValue();
    }

    TzdValue ctorFunc(parentClass->simpleName, parentCtor->params, parentCtor->body);
    ctorFunc.jittedPtr = parentCtor->jittedPtr;
    ctorFunc.instanceVal = thisVal.instanceVal;

    std::unordered_map<std::string, TzdValue> superScope;
    superScope["this"] = thisVal;
    for (size_t i = 0; i < args.size(); ++i) {
        superScope[parentCtor->params[i]] = args[i];
    }

    scopes.push_back(superScope);
    try {
        callFunction(ctorFunc, args);
    }
    catch (...) {
        scopes.pop_back();
        throw;
    }
    scopes.pop_back();

    return TzdValue();
}


std::any TzdInterpreter::visitCallExpr(TzdLangParser::CallExprContext* ctx) {
    // 1. 获取函数对象 (通过 getVariable，它会自动根据重定义后的 scopes 找到最新版本)
    TzdValue funcVal = std::any_cast<TzdValue>(visit(ctx->atom()));
    std::string funcNameForError = ctx->atom()->getText();

    // 2. 解析本次调用的实参
    std::vector<TzdValue> args;
    if (ctx->exprList()) {
        for (auto expr : ctx->exprList()->expression()) {
            args.push_back(std::any_cast<TzdValue>(visit(expr)));
        }
    }

    // 3. 统一分发调用逻辑
    try {
        return callFunction(funcVal, args);
    }
    catch (const TzdReturnException& e) {
        return e.value;
    }
    catch (const TzdRuntimeException& e) {
        throw;
    }
    catch (const TzdThrowException& e) {
        // 抛出的 Error 异常也同样直接向上穿透
        throw;
    }
    catch (const std::exception& e) {
        // 只有那些未被包装过的底层 C++ 异常，才在这里绑定当前调用的 AST 上下文
        throw TzdRuntimeException(e.what(), ctx->getStart());
    }
}

void TzdErrorListener::syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol,
    size_t line, size_t charPositionInLine,
    const std::string& msg, std::exception_ptr e) {

    std::string translatedMsg = msg;
    if (msg.find("mismatched input") != std::string::npos) translatedMsg = "输入符号不匹配 (语法错误)";
    else if (msg.find("extraneous input") != std::string::npos) translatedMsg = "发现多余的输入字符";
    else if (msg.find("missing") != std::string::npos) {
        size_t pos = msg.find("missing");
        translatedMsg = "缺少" + msg.substr(pos + 7);
    }
    else if (msg.find("no viable alternative") != std::string::npos) translatedMsg = "无法识别的语法结构";

    std::string sourceText = "";
    antlr4::Parser* parser = dynamic_cast<antlr4::Parser*>(recognizer);
    if (parser) {
        sourceText = parser->getTokenStream()->getTokenSource()->getInputStream()->toString();
    }

    TzdErrorHandler::report("Tzd 语法错误", line, charPositionInLine, translatedMsg, sourceText);
}

void TzdErrorHandler::report(const std::string& type, size_t line, size_t column, const std::string& msg, const std::string& sourceCode, const std::vector<std::string>& stackTrace)
{
    std::cerr << "Exception in thread \"main\" " << type << ": " << msg << std::endl;

    for (auto it = stackTrace.rbegin(); it != stackTrace.rend(); ++it) {
        std::cerr << "\tat " << *it << std::endl;
    }

    if (!sourceCode.empty() && line > 0) {
        std::istringstream iss(sourceCode);
        std::string codeLine;
        size_t currentLine = 1;
        while (std::getline(iss, codeLine)) {
            if (!codeLine.empty() && codeLine.back() == '\r') codeLine.pop_back();

            if (currentLine == line) {
                std::cerr << "    " << codeLine << std::endl;
                std::cerr << "    ";
                for (size_t i = 0; i < column; ++i) {
                    if (i < codeLine.size() && codeLine[i] == '\t') std::cerr << '\t';
                    else std::cerr << ' ';
                }
                std::cerr << "^--- 这里" << std::endl;
                break;
            }
            currentLine++;
        }
    }
}

std::any TzdInterpreter::visitNullExpr(TzdLangParser::NullExprContext* ctx) {
    return TzdValue();
}

// 2. 实现 tryJitCompile
void TzdInterpreter::tryJitCompile(TzdValue& funcVal) {
}

static double GetAsDouble(TzdValue* v) {
    switch (v->type) {
    case TzdValue::DOUBLE: return v->dVal;
    case TzdValue::FLOAT:  return v->dVal;
    case TzdValue::LONG:   return (double)v->lVal;
    case TzdValue::ULONG:  return (double)v->ulVal;
    case TzdValue::INT:    return (double)v->lVal;
    case TzdValue::UINT:   return (double)v->ulVal;
    case TzdValue::SHORT:  return (double)v->lVal;
    case TzdValue::USHORT: return (double)v->ulVal;
    case TzdValue::SBYTE:  return (double)v->lVal;
    case TzdValue::BYTE:   return (double)v->ulVal;
    case TzdValue::BOOL:   return v->bVal ? 1.0 : 0.0;
    default: return 0.0;
    }
}

TzdValue TzdInterpreter::executeFunction(const TzdValue& funcVal, const std::vector<TzdValue>& args) {
    TzdValue finalFunc = funcVal;

    // --- 新增：自动从类定义中提取静态方法 ---
    if (finalFunc.type == TzdValue::CLASS_DEF && !args.empty()) {
        // 如果 funcVal 是类且被调用，尝试查找其构造函数或静态方法
        // 这里的逻辑可以根据你的需求调整，通常在 visit 层就该处理好
    }

    // 1. 处理原生函数 (NATIVE_FUNCTION)
    if (finalFunc.type == TzdValue::NATIVE_FUNCTION) {
        return finalFunc.nativeFunc(args);
    }

    // 2. 处理脚本函数 (FUNCTION)
    if (finalFunc.type == TzdValue::FUNCTION) {
        // 检查参数数量
        if (args.size() != finalFunc.params.size()) {
            throw std::runtime_error("函数调用参数不匹配: " + finalFunc.name +
                " 需要 " + std::to_string(finalFunc.params.size()) + " 个");
        }

        // --- 准备作用域 ---
        std::unordered_map<std::string, TzdValue> newScope;

        // 如果是类实例方法，绑定 "this"
        if (finalFunc.instanceVal != nullptr) {
            newScope["this"] = TzdValue(finalFunc.instanceVal);
        }

        // 绑定参数
        for (size_t i = 0; i < args.size(); ++i) {
            newScope[finalFunc.params[i]] = args[i];
        }

        scopes.push_back(newScope);
        TzdValue returnValue;

        try {
            if (finalFunc.funcBody) {
                visit(finalFunc.funcBody);
            }
        }
        catch (const TzdReturnException& ret) {
            returnValue = ret.value;
        }
        catch (...) {
            scopes.pop_back();
            throw;
        }

        scopes.pop_back();
        return returnValue;
    }

    // --- 调试信息：报错时打印实际收到的类型 ---
    std::string typeNames[] = { "NONE", "SBYTE", "BYTE", "SHORT", "USHORT", "INT", "UINT", "LONG", "ULONG", "FLOAT", "DOUBLE", "BOOL", "STRING", "POINTER", "ARRAY", "MAP", "FUNCTION", "NATIVE_FUNCTION", "CLASS_DEF", "INSTANCE" };
    std::string actualType = (finalFunc.type >= 0 && finalFunc.type <= 19) ? typeNames[finalFunc.type] : "UNKNOWN";

    throw std::runtime_error("尝试调用一个非函数类型对象 (实际类型: " + actualType + ")");
}
