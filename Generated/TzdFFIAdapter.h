#pragma once

#include <string>
#include <vector>
#include <functional>
#define DC__OS_Win64
#define DC__Arch_AMD64
extern "C" {
#include "../dyncall/dyncall.h"
}
#include <sstream>
#include <vector>

struct TzdValue;

class TzdFFIAdapter {
public:
    using WrapperFunc = std::function<TzdValue(const std::vector<TzdValue>&)>;

    static WrapperFunc buildWrapper(
        void* procAddr,
        const std::string& funcName,
        const std::string& typeStr,
        const std::string& returnType);
};