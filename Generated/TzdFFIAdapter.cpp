#include "TzdFFIAdapter.h"
#include "TzdInterpreter.h"

// 内部结构：持有函数签名元数据
struct FFIData {
    std::string nativeFuncName;
    std::vector<std::string> typeNames;
    std::string returnTypeName;
};

// UTF-8 转 UTF-16 (宽字符)
 std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring wstr(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], size);
    return wstr;
}

std::string AnsiToUtf8(const char* ansiStr) {
    if (!ansiStr || strlen(ansiStr) == 0) return "";

    // 1. 测量 ANSI -> UTF-16 所需的空间 (CP_ACP 代表系统默认 ANSI 代码页)
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, nullptr, 0);
    if (wlen <= 0) return "";

    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, &wstr[0], wlen);

    // 2. 测量 UTF-16 -> UTF-8 所需的空间
    int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (ulen <= 0) return "";

    std::string utf8Str(ulen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8Str[0], ulen, nullptr, nullptr);

    // 移除 WideCharToMultiByte 自动添加的空终止符（std::string 不需要它作为长度的一部分）
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }

    return utf8Str;
}

std::string WideToUtf8(const wchar_t* wstr) {
    if (!wstr || wcslen(wstr) == 0) return "";

    // 1. 测量从 UTF-16 转换到 UTF-8 所需的缓冲区大小
    // CP_UTF8 表示目标编码为 UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return "";

    // 2. 分配空间并执行转换
    std::string utf8Str(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &utf8Str[0], size_needed, nullptr, nullptr);

    // 移除 WideCharToMultiByte 自动添加的空终止符 '\0'
    if (!utf8Str.empty() && utf8Str.back() == '\0') {
        utf8Str.pop_back();
    }

    return utf8Str;
}

TzdFFIAdapter::WrapperFunc TzdFFIAdapter::buildWrapper(
    void* procAddr,
    const std::string& funcName, 
    const std::string& typeStr,
    const std::string& returnType)
{
    auto data = std::make_shared<FFIData>();
    data->returnTypeName = returnType;

    // 1. 解析参数类型字符串
    std::stringstream ss(typeStr);
    std::string t;
    while (std::getline(ss, t, ',')) {
        if (!t.empty()) {
            data->typeNames.push_back(t);
        }
    }

    // 将 funcName 捕获到 Lambda 中
    return [procAddr, data, funcName](const std::vector<TzdValue>& args) -> TzdValue {
        static DCCallVM* vm = dcNewCallVM(4096);
        dcReset(vm);
        dcMode(vm, DC_SIGCHAR_CC_STDCALL);
        std::vector<std::wstring> wstring_pool;
        std::vector<std::string> ansi_pool;
        wstring_pool.reserve(args.size());
        ansi_pool.reserve(args.size());

        size_t n = data->typeNames.size();
        for (size_t i = 0; i < n && i < args.size(); ++i) {
            const std::string& type = data->typeNames[i];

            // 使用 else if 结构，确保互斥执行
            if (type == "string" || type == "wstring") {
                bool isWide = (type == "wstring") || (!funcName.empty() && funcName.back() == 'W');

                if (isWide) {
                    wstring_pool.push_back(Utf8ToWide(args[i].sVal));
                    dcArgPointer(vm, (DCpointer)wstring_pool.back().c_str());
                }
                else {
                    ansi_pool.push_back(Utf8ToAnsi(args[i].sVal));
                    dcArgPointer(vm, (DCpointer)ansi_pool.back().c_str());
                }
            }
            else if (type == "ptr" || type == "hwnd") {
                dcArgPointer(vm, (DCpointer)args[i].ptrVal);
            }
            else if (type == "long" || type == "longlong") {
                // 优先取 lVal
                long long v = (args[i].type == TzdValue::LONG) ? args[i].lVal : (long long)args[i].dVal;
                dcArgLongLong(vm, v);
            }
            else if (type == "int") {
                int v = (args[i].type == TzdValue::LONG) ? (int)args[i].lVal : (int)args[i].dVal;
                dcArgInt(vm, v);
            }
            else if (type == "float") {
                dcArgFloat(vm, (float)args[i].dVal);
            }
            else if (type == "double") {
                dcArgDouble(vm, args[i].dVal);
            }
            else if (type == "bool") {
                dcArgBool(vm, args[i].bVal ? DC_TRUE : DC_FALSE);
            }
        }

        // 4. 执行调用
        TzdValue result;
        const std::string& retType = data->returnTypeName;

        if (retType == "void") {
            dcCallVoid(vm, procAddr);
            result = TzdValue(); // 返回 NONE 类型
        }
        else if (retType == "int") {
            // 标准 32 位整数，转换为 double 存储以保持兼容性
            result = TzdValue((double)dcCallInt(vm, procAddr));
        }
        else if (retType == "long" || retType == "longlong") {
            // 新增：支持 64 位整数返回
            result = TzdValue((long long)dcCallLongLong(vm, procAddr));
        }
        else if (retType == "ptr" || retType == "hwnd" || retType == "pointer") {
            // 新增：支持指针/句柄返回
            void* p = dcCallPointer(vm, procAddr);
            result = TzdValue(p);
        }
        else if (retType == "string") {
            // 处理 A 系列 API 返回的 ANSI 字符串并转为 UTF-8
            const char* s = (const char*)dcCallPointer(vm, procAddr);
            result = TzdValue(s ? AnsiToUtf8(s) : "");
        }
        else if (retType == "wstring") {
            // 新增：处理 W 系列 API 返回的宽字符字符串并转为 UTF-8
            const wchar_t* ws = (const wchar_t*)dcCallPointer(vm, procAddr);
            result = TzdValue(ws ? WideToUtf8(ws) : "");
        }
        else if (retType == "bool") {
            // 支持布尔值返回
            result = TzdValue((bool)dcCallBool(vm, procAddr));
        }
        else if (retType == "float") {
            result = TzdValue((double)dcCallFloat(vm, procAddr));
        }
        else if (retType == "double") {
            result = TzdValue(dcCallDouble(vm, procAddr));
        }
        else {
            // 默认兜底：尝试以 double 接收
            result = TzdValue(dcCallDouble(vm, procAddr));
        }

        return result;
        };
}