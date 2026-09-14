// ============================================================================
// TzdNativeRuntime.hpp
// Standalone Native C++ Runtime for TzdLang AOT Compilation
// Supports MSVC (cl.exe), LLVM / Clang (clang++.exe), and MinGW (g++.exe)
// Supports Zero-DLL Standalone mode AND Optional Native PyTorch (CPU & GPU)
// ============================================================================

#ifndef TZD_NATIVE_RUNTIME_HPP
#define TZD_NATIVE_RUNTIME_HPP

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <cmath>
#include <sstream>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <iomanip>
#include <thread>
#include <random>
#include <filesystem>

#ifdef WITH_LIBTORCH
#include <torch/torch.h>
#ifdef WITH_CUDA
#include <c10/cuda/CUDAFunctions.h>
#endif
#endif

namespace tzd_rt {

struct TzdInstance;

enum class ValType : uint8_t {
    NIL = 0,
    BOOL,
    INT,
    FLOAT,
    STRING,
    ARRAY,
    MAP,
    INSTANCE,
    FUNC,
    TENSOR
};

struct TzdVal {
    ValType type = ValType::NIL;
    int64_t iVal = 0;
    double fVal = 0.0;
    bool bVal = false;
    std::string sVal;
    std::shared_ptr<std::vector<TzdVal>> arrVal;
    std::shared_ptr<std::unordered_map<std::string, TzdVal>> mapVal;
    std::shared_ptr<TzdInstance> instVal;
    std::function<TzdVal(std::vector<TzdVal>&)> funcVal;
#ifdef WITH_LIBTORCH
    std::shared_ptr<torch::Tensor> tensorVal;
#else
    std::shared_ptr<void> tensorVal;
#endif

    // Constructors
    TzdVal() : type(ValType::NIL) {}
    TzdVal(std::nullptr_t) : type(ValType::NIL) {}
    TzdVal(bool v) : type(ValType::BOOL), bVal(v), iVal(v ? 1 : 0), fVal(v ? 1.0 : 0.0) {}
    TzdVal(int v) : type(ValType::INT), iVal(v), fVal((double)v) {}
    TzdVal(long v) : type(ValType::INT), iVal(v), fVal((double)v) {}
    TzdVal(long long v) : type(ValType::INT), iVal(v), fVal((double)v) {}
    TzdVal(unsigned int v) : type(ValType::INT), iVal(v), fVal((double)v) {}
    TzdVal(unsigned long v) : type(ValType::INT), iVal(v), fVal((double)v) {}
    TzdVal(unsigned long long v) : type(ValType::INT), iVal((int64_t)v), fVal((double)v) {}
    TzdVal(float v) : type(ValType::FLOAT), fVal(v), iVal((int64_t)v) {}
    TzdVal(double v) : type(ValType::FLOAT), fVal(v), iVal((int64_t)v) {}
    TzdVal(const char* s) : type(ValType::STRING), sVal(s ? s : "") {}
    TzdVal(const std::string& s) : type(ValType::STRING), sVal(s) {}
    TzdVal(std::shared_ptr<TzdInstance> inst) : type(ValType::INSTANCE), instVal(std::move(inst)) {}
    TzdVal(TzdInstance* inst);
    TzdVal(std::shared_ptr<std::vector<TzdVal>> arr) : type(ValType::ARRAY), arrVal(std::move(arr)) {}
    TzdVal(std::shared_ptr<std::unordered_map<std::string, TzdVal>> m) : type(ValType::MAP), mapVal(std::move(m)) {}
    TzdVal(std::function<TzdVal(std::vector<TzdVal>&)> fn) : type(ValType::FUNC), funcVal(std::move(fn)) {}

#ifdef WITH_LIBTORCH
    TzdVal(torch::Tensor t) : type(ValType::TENSOR), tensorVal(std::make_shared<torch::Tensor>(std::move(t))) {}
#endif

    // Conversions
    double as_double() const {
        switch (type) {
            case ValType::FLOAT: return fVal;
            case ValType::INT: return (double)iVal;
            case ValType::BOOL: return bVal ? 1.0 : 0.0;
            case ValType::STRING: {
                try { return std::stod(sVal); } catch (...) { return 0.0; }
            }
#ifdef WITH_LIBTORCH
            case ValType::TENSOR: {
                if (tensorVal && tensorVal->numel() == 1) return tensorVal->item<double>();
                return 0.0;
            }
#endif
            default: return 0.0;
        }
    }

    int64_t as_int() const {
        switch (type) {
            case ValType::INT: return iVal;
            case ValType::FLOAT: return (int64_t)fVal;
            case ValType::BOOL: return bVal ? 1 : 0;
            case ValType::STRING: {
                try { return std::stoll(sVal); } catch (...) { return 0; }
            }
#ifdef WITH_LIBTORCH
            case ValType::TENSOR: {
                if (tensorVal && tensorVal->numel() == 1) return tensorVal->item<int64_t>();
                return 0;
            }
#endif
            default: return 0;
        }
    }

    bool as_bool() const {
        switch (type) {
            case ValType::BOOL: return bVal;
            case ValType::INT: return iVal != 0;
            case ValType::FLOAT: return fVal != 0.0;
            case ValType::STRING: return !sVal.empty();
            case ValType::ARRAY: return arrVal && !arrVal->empty();
            case ValType::MAP: return mapVal && !mapVal->empty();
            case ValType::INSTANCE: return instVal != nullptr;
            case ValType::FUNC: return (bool)funcVal;
            case ValType::TENSOR: return tensorVal != nullptr;
            case ValType::NIL: return false;
            default: return false;
        }
    }

    std::string to_string() const;

    // Arithmetic operators
    TzdVal operator+(const TzdVal& o) const {
#ifdef WITH_LIBTORCH
        if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
            return TzdVal(*tensorVal + *o.tensorVal);
        }
#endif
        if (type == ValType::STRING || o.type == ValType::STRING) {
            return TzdVal(to_string() + o.to_string());
        }
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
            return TzdVal(as_double() + o.as_double());
        }
        return TzdVal(as_int() + o.as_int());
    }

    TzdVal operator-(const TzdVal& o) const {
#ifdef WITH_LIBTORCH
        if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
            return TzdVal(*tensorVal - *o.tensorVal);
        }
#endif
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
            return TzdVal(as_double() - o.as_double());
        }
        return TzdVal(as_int() - o.as_int());
    }

    TzdVal operator*(const TzdVal& o) const {
#ifdef WITH_LIBTORCH
        if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
            return TzdVal(*tensorVal * *o.tensorVal);
        }
#endif
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
            return TzdVal(as_double() * o.as_double());
        }
        return TzdVal(as_int() * o.as_int());
    }

    TzdVal operator/(const TzdVal& o) const {
#ifdef WITH_LIBTORCH
        if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
            return TzdVal(*tensorVal / *o.tensorVal);
        }
#endif
        double d = o.as_double();
        return TzdVal(d != 0.0 ? as_double() / d : 0.0);
    }

    TzdVal operator%(const TzdVal& o) const {
        int64_t b = o.as_int();
        return TzdVal(b != 0 ? as_int() % b : 0);
    }

    TzdVal pow(const TzdVal& o) const {
        return TzdVal(std::pow(as_double(), o.as_double()));
    }

    TzdVal operator-() const {
#ifdef WITH_LIBTORCH
        if (type == ValType::TENSOR && tensorVal) {
            return TzdVal(-(*tensorVal));
        }
#endif
        if (type == ValType::FLOAT) return TzdVal(-fVal);
        return TzdVal(-as_int());
    }

    TzdVal operator!() const {
        return TzdVal(!as_bool());
    }

    // Comparison operators
    bool operator==(const TzdVal& o) const {
        if (type == ValType::NIL && o.type == ValType::NIL) return true;
        if (type == ValType::NIL || o.type == ValType::NIL) return false;
        if (type == ValType::STRING && o.type == ValType::STRING) return sVal == o.sVal;
        if (type == ValType::BOOL && o.type == ValType::BOOL) return bVal == o.bVal;
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) return as_double() == o.as_double();
        if (type == ValType::INT || o.type == ValType::INT) return as_int() == o.as_int();
        if (type == ValType::INSTANCE && o.type == ValType::INSTANCE) return instVal == o.instVal;
        return false;
    }

    bool operator!=(const TzdVal& o) const { return !(*this == o); }

    bool operator<(const TzdVal& o) const {
        if (type == ValType::STRING && o.type == ValType::STRING) return sVal < o.sVal;
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) return as_double() < o.as_double();
        return as_int() < o.as_int();
    }

    bool operator<=(const TzdVal& o) const {
        if (type == ValType::STRING && o.type == ValType::STRING) return sVal <= o.sVal;
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) return as_double() <= o.as_double();
        return as_int() <= o.as_int();
    }

    bool operator>(const TzdVal& o) const { return !(*this <= o); }
    bool operator>=(const TzdVal& o) const { return !(*this < o); }

    // In-place operators
    TzdVal& operator+=(const TzdVal& o) { *this = *this + o; return *this; }
    TzdVal& operator-=(const TzdVal& o) { *this = *this - o; return *this; }
    TzdVal& operator*=(const TzdVal& o) { *this = *this * o; return *this; }
    TzdVal& operator/=(const TzdVal& o) { *this = *this / o; return *this; }

    // Pre/Post inc/dec
    TzdVal& operator++() { *this = *this + TzdVal(1); return *this; }
    TzdVal operator++(int) { TzdVal tmp = *this; ++(*this); return tmp; }
    TzdVal& operator--() { *this = *this - TzdVal(1); return *this; }
    TzdVal operator--(int) { TzdVal tmp = *this; --(*this); return tmp; }

    // Member and Index access
    TzdVal get_member(const std::string& name);
    void set_member(const std::string& name, const TzdVal& val);
    TzdVal call_method(const std::string& name, std::vector<TzdVal> args);

    TzdVal get_index(const TzdVal& idx);
    void set_index(const TzdVal& idx, const TzdVal& val);

    // Call callable
    TzdVal operator()(std::vector<TzdVal> args) const {
        if (type == ValType::FUNC && funcVal) return funcVal(args);
        return TzdVal();
    }
};

// Base class for Tzd OOP Instances
struct TzdInstance : public std::enable_shared_from_this<TzdInstance> {
    std::string className;
    std::unordered_map<std::string, TzdVal> fields;

    virtual ~TzdInstance() = default;

    virtual TzdVal get_field(const std::string& name) {
        auto it = fields.find(name);
        return it != fields.end() ? it->second : TzdVal();
    }

    virtual void set_field(const std::string& name, const TzdVal& val) {
        fields[name] = val;
    }

    virtual TzdVal call_method(const std::string& name, std::vector<TzdVal> args) {
        return TzdVal();
    }
};

inline TzdVal::TzdVal(TzdInstance* inst) {
    if (inst) {
        type = ValType::INSTANCE;
        try {
            instVal = inst->shared_from_this();
        } catch (...) {
            instVal = std::shared_ptr<TzdInstance>(inst, [](TzdInstance*){});
        }
    } else {
        type = ValType::NIL;
    }
}

inline std::string TzdVal::to_string() const {
    switch (type) {
        case ValType::NIL: return "null";
        case ValType::BOOL: return bVal ? "true" : "false";
        case ValType::INT: return std::to_string(iVal);
        case ValType::FLOAT: {
            std::ostringstream ss;
            ss << fVal;
            return ss.str();
        }
        case ValType::STRING: return sVal;
        case ValType::ARRAY: {
            if (!arrVal) return "[]";
            std::string res = "[";
            for (size_t i = 0; i < arrVal->size(); ++i) {
                if (i > 0) res += ", ";
                res += (*arrVal)[i].to_string();
            }
            res += "]";
            return res;
        }
        case ValType::MAP: {
            if (!mapVal) return "{}";
            std::string res = "{";
            bool first = true;
            for (const auto& kv : *mapVal) {
                if (!first) res += ", ";
                first = false;
                res += "\"" + kv.first + "\": " + kv.second.to_string();
            }
            res += "}";
            return res;
        }
        case ValType::INSTANCE: {
            return instVal ? ("[" + instVal->className + " instance]") : "[null instance]";
        }
        case ValType::FUNC: return "[function]";
#ifdef WITH_LIBTORCH
        case ValType::TENSOR: {
            if (tensorVal) {
                std::ostringstream ss;
                ss << *tensorVal;
                return ss.str();
            }
            return "[tensor]";
        }
#endif
        default: return "";
    }
}

inline TzdVal TzdVal::get_member(const std::string& name) {
    if (type == ValType::INSTANCE && instVal) {
        return instVal->get_field(name);
    }
    if (type == ValType::MAP && mapVal) {
        auto it = mapVal->find(name);
        return it != mapVal->end() ? it->second : TzdVal();
    }
    if (type == ValType::ARRAY && arrVal) {
        if (name == "length" || name == "size") return TzdVal((int64_t)arrVal->size());
    }
    if (type == ValType::STRING) {
        if (name == "length" || name == "size") return TzdVal((int64_t)sVal.size());
    }
    return TzdVal();
}

inline void TzdVal::set_member(const std::string& name, const TzdVal& val) {
    if (type == ValType::INSTANCE && instVal) {
        instVal->set_field(name, val);
        return;
    }
    if (type == ValType::MAP) {
        if (!mapVal) mapVal = std::make_shared<std::unordered_map<std::string, TzdVal>>();
        (*mapVal)[name] = val;
        return;
    }
}

inline TzdVal TzdVal::call_method(const std::string& name, std::vector<TzdVal> args) {
    if (type == ValType::INSTANCE && instVal) {
        return instVal->call_method(name, args);
    }
    if (type == ValType::ARRAY && arrVal) {
        if (name == "push" && !args.empty()) {
            arrVal->push_back(args[0]);
            return TzdVal();
        }
        if (name == "pop" && !arrVal->empty()) {
            TzdVal back = arrVal->back();
            arrVal->pop_back();
            return back;
        }
        if (name == "insert" && args.size() >= 2) {
            size_t idx = (size_t)args[0].as_int();
            if (idx <= arrVal->size()) {
                arrVal->insert(arrVal->begin() + idx, args[1]);
            }
            return TzdVal();
        }
        if (name == "remove" && !args.empty()) {
            size_t idx = (size_t)args[0].as_int();
            if (idx < arrVal->size()) {
                TzdVal removed = (*arrVal)[idx];
                arrVal->erase(arrVal->begin() + idx);
                return removed;
            }
            return TzdVal();
        }
        if (name == "clear") {
            arrVal->clear();
            return TzdVal();
        }
        if (name == "contains" && !args.empty()) {
            for (const auto& item : *arrVal) {
                if (item == args[0]) return TzdVal(true);
            }
            return TzdVal(false);
        }
        if (name == "indexOf" && !args.empty()) {
            for (size_t i = 0; i < arrVal->size(); ++i) {
                if ((*arrVal)[i] == args[0]) return TzdVal((int64_t)i);
            }
            return TzdVal((int64_t)-1);
        }
        if (name == "slice") {
            int64_t start = args.size() > 0 ? args[0].as_int() : 0;
            int64_t end = args.size() > 1 ? args[1].as_int() : (int64_t)arrVal->size();
            if (start < 0) start = std::max<int64_t>(0, (int64_t)arrVal->size() + start);
            if (end < 0) end = std::max<int64_t>(0, (int64_t)arrVal->size() + end);
            start = std::min<int64_t>(start, (int64_t)arrVal->size());
            end = std::min<int64_t>(end, (int64_t)arrVal->size());
            auto sub = std::make_shared<std::vector<TzdVal>>();
            for (int64_t i = start; i < end; ++i) sub->push_back((*arrVal)[i]);
            return TzdVal(sub);
        }
        if (name == "join") {
            std::string sep = args.size() > 0 ? args[0].to_string() : ",";
            std::string res;
            for (size_t i = 0; i < arrVal->size(); ++i) {
                if (i > 0) res += sep;
                res += (*arrVal)[i].to_string();
            }
            return TzdVal(res);
        }
        if (name == "reverse") {
            std::reverse(arrVal->begin(), arrVal->end());
            return *this;
        }
        if (name == "sort") {
            std::sort(arrVal->begin(), arrVal->end(), [](const TzdVal& a, const TzdVal& b) {
                return a < b;
            });
            return *this;
        }
        if (name == "length" || name == "size" || name == "len") {
            return TzdVal((int64_t)arrVal->size());
        }
    }
    if (type == ValType::STRING) {
        if (name == "len" || name == "length" || name == "size") {
            return TzdVal((int64_t)sVal.size());
        }
        if (name == "substr" || name == "substring") {
            size_t start = args.size() > 0 ? (size_t)std::max<int64_t>(0, args[0].as_int()) : 0;
            if (args.size() > 1) {
                size_t len = (size_t)std::max<int64_t>(0, args[1].as_int());
                return TzdVal(start < sVal.size() ? sVal.substr(start, len) : "");
            }
            return TzdVal(start < sVal.size() ? sVal.substr(start) : "");
        }
        if (name == "charAt" && !args.empty()) {
            size_t idx = (size_t)std::max<int64_t>(0, args[0].as_int());
            return TzdVal(idx < sVal.size() ? std::string(1, sVal[idx]) : "");
        }
        if (name == "indexOf" && !args.empty()) {
            size_t pos = sVal.find(args[0].to_string());
            return TzdVal(pos != std::string::npos ? (int64_t)pos : (int64_t)-1);
        }
        if (name == "contains" && !args.empty()) {
            return TzdVal(sVal.find(args[0].to_string()) != std::string::npos);
        }
        if (name == "split") {
            std::string sep = args.size() > 0 ? args[0].to_string() : " ";
            auto res = std::make_shared<std::vector<TzdVal>>();
            if (sep.empty()) {
                for (char c : sVal) res->push_back(TzdVal(std::string(1, c)));
            } else {
                size_t start = 0, end = 0;
                while ((end = sVal.find(sep, start)) != std::string::npos) {
                    res->push_back(TzdVal(sVal.substr(start, end - start)));
                    start = end + sep.length();
                }
                res->push_back(TzdVal(sVal.substr(start)));
            }
            return TzdVal(res);
        }
        if (name == "replace" && args.size() >= 2) {
            std::string from = args[0].to_string();
            std::string to = args[1].to_string();
            std::string res = sVal;
            size_t pos = 0;
            while ((pos = res.find(from, pos)) != std::string::npos) {
                res.replace(pos, from.length(), to);
                pos += to.length();
            }
            return TzdVal(res);
        }
        if (name == "toUpper" || name == "toUpperCase") {
            std::string res = sVal;
            for (char& c : res) c = (char)::toupper(c);
            return TzdVal(res);
        }
        if (name == "toLower" || name == "toLowerCase") {
            std::string res = sVal;
            for (char& c : res) c = (char)::tolower(c);
            return TzdVal(res);
        }
        if (name == "trim") {
            size_t first = sVal.find_first_not_of(" \t\r\n");
            if (first == std::string::npos) return TzdVal("");
            size_t last = sVal.find_last_not_of(" \t\r\n");
            return TzdVal(sVal.substr(first, last - first + 1));
        }
        if (name == "startsWith" && !args.empty()) {
            std::string prefix = args[0].to_string();
            return TzdVal(sVal.rfind(prefix, 0) == 0);
        }
        if (name == "endsWith" && !args.empty()) {
            std::string suffix = args[0].to_string();
            if (suffix.size() > sVal.size()) return TzdVal(false);
            return TzdVal(sVal.compare(sVal.size() - suffix.size(), suffix.size(), suffix) == 0);
        }
    }
    if (type == ValType::MAP && mapVal) {
        if (name == "keys") {
            auto keys = std::make_shared<std::vector<TzdVal>>();
            for (const auto& kv : *mapVal) keys->push_back(TzdVal(kv.first));
            return TzdVal(keys);
        }
        if (name == "values") {
            auto vals = std::make_shared<std::vector<TzdVal>>();
            for (const auto& kv : *mapVal) vals->push_back(kv.second);
            return TzdVal(vals);
        }
        if ((name == "has" || name == "hasKey") && !args.empty()) {
            return TzdVal(mapVal->find(args[0].to_string()) != mapVal->end());
        }
        if (name == "remove" && !args.empty()) {
            mapVal->erase(args[0].to_string());
            return TzdVal();
        }
        if (name == "clear") {
            mapVal->clear();
            return TzdVal();
        }
        if (name == "len" || name == "length" || name == "size") {
            return TzdVal((int64_t)mapVal->size());
        }
    }
#ifdef WITH_LIBTORCH
    if (type == ValType::TENSOR && tensorVal) {
        if (name == "shape" || name == "size") {
            auto sizes = tensorVal->sizes();
            auto res = std::make_shared<std::vector<TzdVal>>();
            for (auto s : sizes) res->push_back(TzdVal((int64_t)s));
            return TzdVal(res);
        }
        if (name == "item" || name == "scalar") {
            return TzdVal(tensorVal->item<double>());
        }
        if (name == "cuda") {
#ifdef WITH_CUDA
            return TzdVal(tensorVal->cuda());
#else
            return *this;
#endif
        }
        if (name == "cpu") {
            return TzdVal(tensorVal->cpu());
        }
        if (name == "relu") return TzdVal(torch::relu(*tensorVal));
        if (name == "sigmoid") return TzdVal(torch::sigmoid(*tensorVal));
        if (name == "mean") return TzdVal(tensorVal->mean());
        if (name == "sum") return TzdVal(tensorVal->sum());
    }
#endif
    return TzdVal();
}

inline TzdVal TzdVal::get_index(const TzdVal& idx) {
    if (type == ValType::ARRAY && arrVal) {
        size_t i = (size_t)idx.as_int();
        if (i < arrVal->size()) return (*arrVal)[i];
        return TzdVal();
    }
    if (type == ValType::MAP && mapVal) {
        auto it = mapVal->find(idx.to_string());
        return it != mapVal->end() ? it->second : TzdVal();
    }
    if (type == ValType::STRING) {
        size_t i = (size_t)idx.as_int();
        if (i < sVal.size()) return TzdVal(std::string(1, sVal[i]));
        return TzdVal();
    }
#ifdef WITH_LIBTORCH
    if (type == ValType::TENSOR && tensorVal) {
        int64_t i = idx.as_int();
        if (i >= 0 && i < tensorVal->size(0)) {
            return TzdVal((*tensorVal)[i]);
        }
        return TzdVal();
    }
#endif
    return TzdVal();
}

inline void TzdVal::set_index(const TzdVal& idx, const TzdVal& val) {
    if (type == ValType::ARRAY && arrVal) {
        size_t i = (size_t)idx.as_int();
        if (i >= arrVal->size()) {
            arrVal->resize(i + 1, TzdVal());
        }
        (*arrVal)[i] = val;
        return;
    }
    if (type == ValType::MAP) {
        if (!mapVal) mapVal = std::make_shared<std::unordered_map<std::string, TzdVal>>();
        (*mapVal)[idx.to_string()] = val;
        return;
    }
#ifdef WITH_LIBTORCH
    if (type == ValType::TENSOR && tensorVal && val.type == ValType::TENSOR && val.tensorVal) {
        int64_t i = idx.as_int();
        if (i >= 0 && i < tensorVal->size(0)) {
            (*tensorVal)[i] = *val.tensorVal;
        }
    }
#endif
}

// ── Helpers ──
inline void tzd_print_vec(const std::vector<TzdVal>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << args[i].to_string();
    }
    std::cout << std::endl;
}

inline TzdVal tzd_make_array(std::initializer_list<TzdVal> elements) {
    auto vec = std::make_shared<std::vector<TzdVal>>(elements);
    return TzdVal(vec);
}

inline TzdVal tzd_len(const TzdVal& v) {
    if (v.type == ValType::ARRAY && v.arrVal) return TzdVal((int64_t)v.arrVal->size());
    if (v.type == ValType::MAP && v.mapVal) return TzdVal((int64_t)v.mapVal->size());
    if (v.type == ValType::STRING) return TzdVal((int64_t)v.sVal.size());
#ifdef WITH_LIBTORCH
    if (v.type == ValType::TENSOR && v.tensorVal) return TzdVal((int64_t)v.tensorVal->size(0));
#endif
    return TzdVal(0);
}

inline double tzd_clock() {
    static auto start_time = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms = now - start_time;
    return ms.count();
}

// ============================================================================
// Complete Built-in Functions Library (Callable via tzd_builtin_<name>(args))
// ============================================================================

// 1. Console & I/O
inline TzdVal tzd_builtin_print(const std::vector<TzdVal>& args) {
    tzd_print_vec(args);
    return TzdVal();
}
inline TzdVal tzd_builtin_println(const std::vector<TzdVal>& args) {
    tzd_print_vec(args);
    return TzdVal();
}
inline TzdVal tzd_builtin_input(const std::vector<TzdVal>& args) {
    if (!args.empty()) {
        std::cout << args[0].to_string();
        std::cout.flush();
    }
    std::string line;
    if (std::getline(std::cin, line)) return TzdVal(line);
    return TzdVal("");
}

// 2. Type conversions & checking
inline TzdVal tzd_builtin_str(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? "" : args[0].to_string());
}
inline TzdVal tzd_builtin_toString(const std::vector<TzdVal>& args) {
    return tzd_builtin_str(args);
}
inline TzdVal tzd_builtin_int(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? (int64_t)0 : args[0].as_int());
}
inline TzdVal tzd_builtin_toInt(const std::vector<TzdVal>& args) {
    return tzd_builtin_int(args);
}
inline TzdVal tzd_builtin_parseInt(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal((int64_t)0);
    int base = args.size() > 1 ? (int)args[1].as_int() : 10;
    try {
        return TzdVal((int64_t)std::stoll(args[0].to_string(), nullptr, base));
    } catch (...) { return TzdVal((int64_t)0); }
}
inline TzdVal tzd_builtin_float(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : args[0].as_double());
}
inline TzdVal tzd_builtin_toFloat(const std::vector<TzdVal>& args) {
    return tzd_builtin_float(args);
}
inline TzdVal tzd_builtin_parseDouble(const std::vector<TzdVal>& args) {
    return tzd_builtin_float(args);
}
inline TzdVal tzd_builtin_bool(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? false : args[0].as_bool());
}
inline TzdVal tzd_builtin_toBool(const std::vector<TzdVal>& args) {
    return tzd_builtin_bool(args);
}
inline TzdVal tzd_builtin_len(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(0) : tzd_len(args[0]);
}
inline TzdVal tzd_builtin_type(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("NIL");
    switch (args[0].type) {
        case ValType::NIL: return TzdVal("NIL");
        case ValType::BOOL: return TzdVal("BOOL");
        case ValType::INT: return TzdVal("INT");
        case ValType::FLOAT: return TzdVal("FLOAT");
        case ValType::STRING: return TzdVal("STRING");
        case ValType::ARRAY: return TzdVal("ARRAY");
        case ValType::MAP: return TzdVal("MAP");
        case ValType::INSTANCE: return TzdVal(args[0].instVal ? args[0].instVal->className : "INSTANCE");
        case ValType::FUNC: return TzdVal("FUNC");
        case ValType::TENSOR: return TzdVal("TENSOR");
        default: return TzdVal("UNKNOWN");
    }
}
inline TzdVal tzd_builtin_isNone(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() || args[0].type == ValType::NIL);
}
inline TzdVal tzd_builtin_isNull(const std::vector<TzdVal>& args) {
    return tzd_builtin_isNone(args);
}

// 3. Math library
inline TzdVal tzd_builtin_abs(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args[0].type == ValType::FLOAT) return TzdVal(std::abs(args[0].as_double()));
    return TzdVal(std::abs(args[0].as_int()));
}
inline TzdVal tzd_builtin_sqrt(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::sqrt(args[0].as_double()));
}
inline TzdVal tzd_builtin_cbrt(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::cbrt(args[0].as_double()));
}
inline TzdVal tzd_builtin_sin(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::sin(args[0].as_double()));
}
inline TzdVal tzd_builtin_cos(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::cos(args[0].as_double()));
}
inline TzdVal tzd_builtin_tan(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::tan(args[0].as_double()));
}
inline TzdVal tzd_builtin_asin(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::asin(args[0].as_double()));
}
inline TzdVal tzd_builtin_acos(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::acos(args[0].as_double()));
}
inline TzdVal tzd_builtin_atan(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::atan(args[0].as_double()));
}
inline TzdVal tzd_builtin_atan2(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(std::atan2(args[0].as_double(), args[1].as_double()));
}
inline TzdVal tzd_builtin_sinh(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::sinh(args[0].as_double()));
}
inline TzdVal tzd_builtin_cosh(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::cosh(args[0].as_double()));
}
inline TzdVal tzd_builtin_tanh(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::tanh(args[0].as_double()));
}
inline TzdVal tzd_builtin_pow(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(std::pow(args[0].as_double(), args[1].as_double()));
}
inline TzdVal tzd_builtin_exp(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::exp(args[0].as_double()));
}
inline TzdVal tzd_builtin_log(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::log(args[0].as_double()));
}
inline TzdVal tzd_builtin_log10(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::log10(args[0].as_double()));
}
inline TzdVal tzd_builtin_log2(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::log2(args[0].as_double()));
}
inline TzdVal tzd_builtin_floor(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::floor(args[0].as_double()));
}
inline TzdVal tzd_builtin_ceil(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::ceil(args[0].as_double()));
}
inline TzdVal tzd_builtin_round(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::round(args[0].as_double()));
}
inline TzdVal tzd_builtin_trunc(const std::vector<TzdVal>& args) {
    return TzdVal(args.empty() ? 0.0 : std::trunc(args[0].as_double()));
}
inline TzdVal tzd_builtin_min(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args.size() == 1) return args[0];
    return args[0] < args[1] ? args[0] : args[1];
}
inline TzdVal tzd_builtin_max(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args.size() == 1) return args[0];
    return args[0] > args[1] ? args[0] : args[1];
}
inline TzdVal tzd_builtin_clamp(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal(0) : args[0];
    double v = args[0].as_double();
    double lo = args[1].as_double();
    double hi = args[2].as_double();
    return TzdVal(std::clamp(v, lo, hi));
}
inline TzdVal tzd_builtin_random(const std::vector<TzdVal>& args) {
    static std::mt19937_64 rng((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return TzdVal(dist(rng));
}
inline TzdVal tzd_builtin_randInt(const std::vector<TzdVal>& args) {
    static std::mt19937_64 rng((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
    int64_t lo = args.size() > 0 ? args[0].as_int() : 0;
    int64_t hi = args.size() > 1 ? args[1].as_int() : 100;
    if (lo > hi) std::swap(lo, hi);
    std::uniform_int_distribution<int64_t> dist(lo, hi);
    return TzdVal(dist(rng));
}
inline TzdVal tzd_builtin_factorial(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 0 : args[0].as_int();
    if (n <= 1) return TzdVal((int64_t)1);
    double res = 1.0;
    for (int64_t i = 2; i <= n; ++i) res *= (double)i;
    return TzdVal(res);
}

// 4. Time & System
inline TzdVal tzd_builtin_time(const std::vector<TzdVal>& args) {
    auto now = std::chrono::system_clock::now();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    return TzdVal((int64_t)sec);
}
inline TzdVal tzd_builtin_clock(const std::vector<TzdVal>& args) {
    return TzdVal(tzd_clock());
}
inline TzdVal tzd_builtin_sleep(const std::vector<TzdVal>& args) {
    if (!args.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(args[0].as_int()));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_exit(const std::vector<TzdVal>& args) {
    int code = args.empty() ? 0 : (int)args[0].as_int();
    std::exit(code);
    return TzdVal();
}
inline TzdVal tzd_builtin_assert(const std::vector<TzdVal>& args) {
    if (args.empty() || !args[0].as_bool()) {
        std::string msg = args.size() > 1 ? args[1].to_string() : "Assertion failed";
        std::cerr << "[Tzd Assertion Error] " << msg << std::endl;
        std::exit(1);
    }
    return TzdVal(true);
}

// 5. Array Operations
inline TzdVal tzd_builtin_push(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::ARRAY && args[0].arrVal) {
        args[0].arrVal->push_back(args[1]);
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_pop(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal && !args[0].arrVal->empty()) {
        TzdVal back = args[0].arrVal->back();
        args[0].arrVal->pop_back();
        return back;
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_insert(const std::vector<TzdVal>& args) {
    if (args.size() >= 3 && args[0].type == ValType::ARRAY && args[0].arrVal) {
        size_t idx = (size_t)args[1].as_int();
        if (idx <= args[0].arrVal->size()) {
            args[0].arrVal->insert(args[0].arrVal->begin() + idx, args[2]);
        }
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_remove(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::ARRAY && args[0].arrVal) {
        size_t idx = (size_t)args[1].as_int();
        if (idx < args[0].arrVal->size()) {
            TzdVal removed = (*args[0].arrVal)[idx];
            args[0].arrVal->erase(args[0].arrVal->begin() + idx);
            return removed;
        }
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_clear(const std::vector<TzdVal>& args) {
    if (!args.empty()) {
        if (args[0].type == ValType::ARRAY && args[0].arrVal) args[0].arrVal->clear();
        else if (args[0].type == ValType::MAP && args[0].mapVal) args[0].mapVal->clear();
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_contains(const std::vector<TzdVal>& args) {
    if (args.size() >= 2) {
        if (args[0].type == ValType::ARRAY && args[0].arrVal) {
            for (const auto& it : *args[0].arrVal) {
                if (it == args[1]) return TzdVal(true);
            }
            return TzdVal(false);
        }
        if (args[0].type == ValType::STRING) {
            return TzdVal(args[0].sVal.find(args[1].to_string()) != std::string::npos);
        }
        if (args[0].type == ValType::MAP && args[0].mapVal) {
            return TzdVal(args[0].mapVal->find(args[1].to_string()) != args[0].mapVal->end());
        }
    }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_indexOf(const std::vector<TzdVal>& args) {
    if (args.size() >= 2) {
        if (args[0].type == ValType::ARRAY && args[0].arrVal) {
            for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
                if ((*args[0].arrVal)[i] == args[1]) return TzdVal((int64_t)i);
            }
            return TzdVal((int64_t)-1);
        }
        if (args[0].type == ValType::STRING) {
            size_t p = args[0].sVal.find(args[1].to_string());
            return TzdVal(p != std::string::npos ? (int64_t)p : (int64_t)-1);
        }
    }
    return TzdVal((int64_t)-1);
}
inline TzdVal tzd_builtin_slice(const std::vector<TzdVal>& args) {
    if (!args.empty()) {
        std::vector<TzdVal> passArgs;
        for (size_t i = 1; i < args.size(); ++i) passArgs.push_back(args[i]);
        return const_cast<TzdVal&>(args[0]).call_method("slice", passArgs);
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_join(const std::vector<TzdVal>& args) {
    if (!args.empty()) {
        std::vector<TzdVal> passArgs;
        for (size_t i = 1; i < args.size(); ++i) passArgs.push_back(args[i]);
        return const_cast<TzdVal&>(args[0]).call_method("join", passArgs);
    }
    return TzdVal("");
}
inline TzdVal tzd_builtin_reverse(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        std::reverse(args[0].arrVal->begin(), args[0].arrVal->end());
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_sort(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        std::sort(args[0].arrVal->begin(), args[0].arrVal->end(), [](const TzdVal& a, const TzdVal& b) {
            return a < b;
        });
        return args[0];
    }
    return TzdVal();
}

// 6. Map Operations
inline TzdVal tzd_builtin_keys(const std::vector<TzdVal>& args) {
    auto keys = std::make_shared<std::vector<TzdVal>>();
    if (!args.empty() && args[0].type == ValType::MAP && args[0].mapVal) {
        for (const auto& kv : *args[0].mapVal) keys->push_back(TzdVal(kv.first));
    }
    return TzdVal(keys);
}
inline TzdVal tzd_builtin_values(const std::vector<TzdVal>& args) {
    auto vals = std::make_shared<std::vector<TzdVal>>();
    if (!args.empty() && args[0].type == ValType::MAP && args[0].mapVal) {
        for (const auto& kv : *args[0].mapVal) vals->push_back(kv.second);
    }
    return TzdVal(vals);
}
inline TzdVal tzd_builtin_hasKey(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::MAP && args[0].mapVal) {
        return TzdVal(args[0].mapVal->find(args[1].to_string()) != args[0].mapVal->end());
    }
    return TzdVal(false);
}

// 7. File System
inline TzdVal tzd_builtin_readFile(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::ifstream file(args[0].to_string(), std::ios::binary);
    if (!file.is_open()) return TzdVal("");
    std::ostringstream ss;
    ss << file.rdbuf();
    return TzdVal(ss.str());
}
inline TzdVal tzd_builtin_writeFile(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::ofstream file(args[0].to_string(), std::ios::binary);
    if (!file.is_open()) return TzdVal(false);
    file << args[1].to_string();
    return TzdVal(true);
}
inline TzdVal tzd_builtin_appendFile(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::ofstream file(args[0].to_string(), std::ios::app | std::ios::binary);
    if (!file.is_open()) return TzdVal(false);
    file << args[1].to_string();
    return TzdVal(true);
}
inline TzdVal tzd_builtin_fileExists(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    return TzdVal(std::filesystem::exists(args[0].to_string()));
}
inline TzdVal tzd_builtin_removeFile(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    std::error_code ec;
    return TzdVal(std::filesystem::remove(args[0].to_string(), ec));
}

// 8. PyTorch / Tensor Native & Fallback API
#ifdef WITH_LIBTORCH
inline TzdVal tzd_builtin_torch_cuda_is_available(const std::vector<TzdVal>& args) {
#ifdef WITH_CUDA
    return TzdVal(torch::cuda::is_available());
#else
    return TzdVal(false);
#endif
}

inline TzdVal tzd_builtin_torch_tensor(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(torch::empty({0}));
    // Helper to recursively parse 1D/2D arrays
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        const auto& arr = *args[0].arrVal;
        if (arr.empty()) return TzdVal(torch::empty({0}));
        if (arr[0].type == ValType::ARRAY && arr[0].arrVal) {
            // 2D Tensor
            int64_t rows = (int64_t)arr.size();
            int64_t cols = (int64_t)arr[0].arrVal->size();
            std::vector<float> data;
            data.reserve(rows * cols);
            for (const auto& row : arr) {
                if (row.arrVal) {
                    for (const auto& col : *row.arrVal) {
                        data.push_back((float)col.as_double());
                    }
                }
            }
            auto opts = torch::TensorOptions().dtype(torch::kFloat32);
            return TzdVal(torch::from_blob(data.data(), {rows, cols}, opts).clone());
        } else {
            // 1D Tensor
            int64_t size = (int64_t)arr.size();
            std::vector<float> data;
            data.reserve(size);
            for (const auto& item : arr) {
                data.push_back((float)item.as_double());
            }
            auto opts = torch::TensorOptions().dtype(torch::kFloat32);
            return TzdVal(torch::from_blob(data.data(), {size}, opts).clone());
        }
    }
    return TzdVal(torch::tensor((float)args[0].as_double()));
}

inline std::vector<int64_t> parse_shape(const std::vector<TzdVal>& args) {
    std::vector<int64_t> shape;
    if (args.empty()) return {1};
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& item : *args[0].arrVal) shape.push_back(item.as_int());
    } else {
        for (const auto& a : args) shape.push_back(a.as_int());
    }
    if (shape.empty()) shape.push_back(1);
    return shape;
}

inline TzdVal tzd_builtin_torch_zeros(const std::vector<TzdVal>& args) {
    return TzdVal(torch::zeros(parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_ones(const std::vector<TzdVal>& args) {
    return TzdVal(torch::ones(parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_randn(const std::vector<TzdVal>& args) {
    return TzdVal(torch::randn(parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_empty(const std::vector<TzdVal>& args) {
    return TzdVal(torch::empty(parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_matmul(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) {
        return TzdVal(torch::matmul(*args[0].tensorVal, *args[1].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_add(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) {
        return TzdVal(torch::add(*args[0].tensorVal, *args[1].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sub(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) {
        return TzdVal(torch::sub(*args[0].tensorVal, *args[1].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_mul(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) {
        return TzdVal(torch::mul(*args[0].tensorVal, *args[1].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_div(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) {
        return TzdVal(torch::div(*args[0].tensorVal, *args[1].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sigmoid(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) {
        return TzdVal(torch::sigmoid(*args[0].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_relu(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) {
        return TzdVal(torch::relu(*args[0].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_softmax(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) {
        int64_t dim = args.size() > 1 ? args[1].as_int() : -1;
        return TzdVal(torch::softmax(*args[0].tensorVal, dim));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_mean(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) {
        return TzdVal(torch::mean(*args[0].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sum(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) {
        return TzdVal(torch::sum(*args[0].tensorVal));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_scalar_value(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) {
        return TzdVal(args[0].tensorVal->item<double>());
    }
    return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_shape(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) {
        auto sizes = args[0].tensorVal->sizes();
        std::string s = "[";
        for (size_t i = 0; i < sizes.size(); ++i) {
            if (i > 0) s += ", ";
            s += std::to_string(sizes[i]);
        }
        s += "]";
        return TzdVal(s);
    }
    return TzdVal("[]");
}
inline TzdVal tzd_builtin_torch_is_tensor(const std::vector<TzdVal>& args) {
    return TzdVal(!args.empty() && args[0].type == ValType::TENSOR);
}
#else
// Zero-DLL Fallback implementations when LibTorch is omitted
inline TzdVal tzd_builtin_torch_cuda_is_available(const std::vector<TzdVal>& args) { return TzdVal(false); }
inline TzdVal tzd_builtin_torch_tensor(const std::vector<TzdVal>& args) { return args.empty() ? tzd_make_array({}) : args[0]; }
inline TzdVal tzd_builtin_torch_zeros(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    auto arr = std::make_shared<std::vector<TzdVal>>(n > 0 ? n : 1, TzdVal(0.0));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_ones(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    auto arr = std::make_shared<std::vector<TzdVal>>(n > 0 ? n : 1, TzdVal(1.0));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_randn(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (int64_t i = 0; i < n; ++i) arr->push_back(tzd_builtin_random({}));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_empty(const std::vector<TzdVal>& args) { return tzd_builtin_torch_zeros(args); }
inline TzdVal tzd_builtin_torch_matmul(const std::vector<TzdVal>& args) { return TzdVal(); }
inline TzdVal tzd_builtin_torch_add(const std::vector<TzdVal>& args) { return args.size() >= 2 ? args[0] + args[1] : TzdVal(); }
inline TzdVal tzd_builtin_torch_sub(const std::vector<TzdVal>& args) { return args.size() >= 2 ? args[0] - args[1] : TzdVal(); }
inline TzdVal tzd_builtin_torch_mul(const std::vector<TzdVal>& args) { return args.size() >= 2 ? args[0] * args[1] : TzdVal(); }
inline TzdVal tzd_builtin_torch_div(const std::vector<TzdVal>& args) { return args.size() >= 2 ? args[0] / args[1] : TzdVal(); }
inline TzdVal tzd_builtin_torch_sigmoid(const std::vector<TzdVal>& args) { return TzdVal(0.5); }
inline TzdVal tzd_builtin_torch_relu(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : args[0]; }
inline TzdVal tzd_builtin_torch_softmax(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal() : args[0]; }
inline TzdVal tzd_builtin_torch_mean(const std::vector<TzdVal>& args) { return TzdVal(0.0); }
inline TzdVal tzd_builtin_torch_sum(const std::vector<TzdVal>& args) { return TzdVal(0.0); }
inline TzdVal tzd_builtin_torch_scalar_value(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : args[0]; }
inline TzdVal tzd_builtin_torch_shape(const std::vector<TzdVal>& args) { return TzdVal("[]"); }
inline TzdVal tzd_builtin_torch_is_tensor(const std::vector<TzdVal>& args) { return TzdVal(false); }
#endif

// Main entry helper
inline void init_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

} // namespace tzd_rt

using namespace tzd_rt;

#endif // TZD_NATIVE_RUNTIME_HPP
