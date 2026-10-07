#pragma once

#include <string>

class SymbolDemangler {
public:
    /**
     * @brief 将 MSVC 修饰名转换为可读的 C++ 签名
     *
     * @param mangledName 输入的修饰名 (例如: ?S_Destroy_range@...)
     * @return std::string 解码后的 C++ 签名。如果无法解码或输入不是修饰名，则返回原字符串。
     */
    static std::string demangle(const std::string& mangledName);
};