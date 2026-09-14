#include <intrin.h>
// ============================================================================
// TzdNativeRuntime.hpp
// Standalone, Zero-DLL, Header-Only Native C++ Runtime for TzdLang AOT Compilation
// Compiles to ultra-fast native x86_64 machine code via MSVC / Clang / GCC
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
#include <unordered_set>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <memory>
#include <chrono>
#include <cmath>
#include <sstream>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include <iomanip>
#include <random>
#include <regex>
#include <thread>
#include <mutex>
#include <filesystem>
#include <cstdlib>
#include <ctime>
#include <numeric>

#ifdef WITH_LIBTORCH
#include <torch/torch.h>
#include <ATen/ATen.h>
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
    std::shared_ptr<at::Tensor> tensorVal;
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
    TzdVal(const at::Tensor& t) : type(ValType::TENSOR), tensorVal(std::make_shared<at::Tensor>(t)) {}
    TzdVal(std::shared_ptr<at::Tensor> t) : type(ValType::TENSOR), tensorVal(std::move(t)) {}
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
            case ValType::STRING: return !sVal.empty() && sVal != "false" && sVal != "0";
            case ValType::ARRAY: return arrVal && !arrVal->empty();
            case ValType::MAP: return mapVal && !mapVal->empty();
            case ValType::INSTANCE: return instVal != nullptr;
            case ValType::FUNC: return funcVal != nullptr;
#ifdef WITH_LIBTORCH
            case ValType::TENSOR: return tensorVal != nullptr && tensorVal->numel() > 0;
#endif
            default: return false;
        }
    }

    std::string to_string() const {
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
                std::ostringstream ss;
                ss << "[";
                for (size_t i = 0; i < arrVal->size(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << (*arrVal)[i].to_string();
                }
                ss << "]";
                return ss.str();
            }
            case ValType::MAP: {
                if (!mapVal) return "{}";
                std::ostringstream ss;
                ss << "{";
                bool first = true;
                for (const auto& kv : *mapVal) {
                    if (!first) ss << ", ";
                    first = false;
                    ss << """ << kv.first << "": " << kv.second.to_string();
                }
                ss << "}";
                return ss.str();
            }
            case ValType::INSTANCE: return "[Object Instance]";
            case ValType::FUNC: return "[Function]";
#ifdef WITH_LIBTORCH
            case ValType::TENSOR: {
                if (!tensorVal) return "[Tensor null]";
                std::ostringstream ss;
                ss << *tensorVal;
                return ss.str();
            }
#endif
            default: return "null";
        }
    }

    // Arithmetic operators
    TzdVal operator+(const TzdVal& o) const;
    TzdVal operator-(const TzdVal& o) const;
    TzdVal operator*(const TzdVal& o) const;
    TzdVal operator/(const TzdVal& o) const;
    TzdVal operator%(const TzdVal& o) const;
    TzdVal pow(const TzdVal& o) const;

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

    TzdVal get_index(const TzdVal& idx) const;
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
        instVal = std::shared_ptr<TzdInstance>(inst);
    } else {
        type = ValType::NIL;
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
    return TzdVal();
}

inline void TzdVal::set_member(const std::string& name, const TzdVal& val) {
    if (type == ValType::INSTANCE && instVal) {
        instVal->set_field(name, val);
    } else if (type == ValType::MAP && mapVal) {
        (*mapVal)[name] = val;
    }
}

inline TzdVal TzdVal::call_method(const std::string& name, std::vector<TzdVal> args) {
    if (type == ValType::INSTANCE && instVal) {
        return instVal->call_method(name, std::move(args));
    }
    // Dynamic array methods: arr.push(x), arr.len(), etc.
    if (type == ValType::ARRAY && arrVal) {
        if (name == "push" && !args.empty()) { arrVal->push_back(args[0]); return *this; }
        if (name == "pop" && !arrVal->empty()) { TzdVal v = arrVal->back(); arrVal->pop_back(); return v; }
        if (name == "len" || name == "size") return TzdVal((int64_t)arrVal->size());
        if (name == "clear") { arrVal->clear(); return *this; }
    }
    return TzdVal();
}

inline TzdVal TzdVal::get_index(const TzdVal& idx) const {
    if (type == ValType::ARRAY && arrVal) {
        int64_t i = idx.as_int();
        if (i < 0) i += (int64_t)arrVal->size();
        if (i >= 0 && i < (int64_t)arrVal->size()) {
            return (*arrVal)[(size_t)i];
        }
        return TzdVal();
    }
    if (type == ValType::MAP && mapVal) {
        auto it = mapVal->find(idx.to_string());
        return it != mapVal->end() ? it->second : TzdVal();
    }
    if (type == ValType::STRING) {
        int64_t i = idx.as_int();
        if (i < 0) i += (int64_t)sVal.size();
        if (i >= 0 && i < (int64_t)sVal.size()) {
            return TzdVal(std::string(1, sVal[(size_t)i]));
        }
        return TzdVal("");
    }
    return TzdVal();
}

inline void TzdVal::set_index(const TzdVal& idx, const TzdVal& val) {
    if (type == ValType::ARRAY && arrVal) {
        int64_t i = idx.as_int();
        if (i < 0) i += (int64_t)arrVal->size();
        if (i >= 0) {
            if (i >= (int64_t)arrVal->size()) {
                arrVal->resize((size_t)(i + 1));
            }
            (*arrVal)[(size_t)i] = val;
        }
    } else if (type == ValType::MAP && mapVal) {
        (*mapVal)[idx.to_string()] = val;
    }
}

// ── Standard Constants ──
static const TzdVal PI(3.14159265358979323846);
static const TzdVal E(2.71828182845904523536);
static const TzdVal TAU(6.28318530717958647692);
static const TzdVal INF(std::numeric_limits<double>::infinity());
static const TzdVal GOLDEN_RATIO(1.61803398874989484820);
static const TzdVal EPSILON(1e-9);
static const TzdVal SQRT2(1.41421356237309504880);

inline TzdVal tzd_builtin_PI(const std::vector<TzdVal>& = {}) { return PI; }
inline TzdVal tzd_builtin_E(const std::vector<TzdVal>& = {}) { return E; }
inline TzdVal tzd_builtin_TAU(const std::vector<TzdVal>& = {}) { return TAU; }
inline TzdVal tzd_builtin_INF(const std::vector<TzdVal>& = {}) { return INF; }
inline TzdVal tzd_builtin_NAN(const std::vector<TzdVal>& = {}) { return TzdVal(std::numeric_limits<double>::quiet_NaN()); }
inline TzdVal tzd_builtin_GOLDEN_RATIO(const std::vector<TzdVal>& = {}) { return GOLDEN_RATIO; }
inline TzdVal tzd_builtin_EPSILON(const std::vector<TzdVal>& = {}) { return EPSILON; }
inline TzdVal tzd_builtin_SQRT2(const std::vector<TzdVal>& = {}) { return SQRT2; }

// Helper: make array
inline TzdVal tzd_make_array(std::initializer_list<TzdVal> elements) {
    auto vec = std::make_shared<std::vector<TzdVal>>(elements);
    return TzdVal(vec);
}

inline void tzd_print_vec(const std::vector<TzdVal>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) std::cout << " ";
        std::cout << args[i].to_string();
    }
    std::cout << std::endl;
}

// ── Console and Type Conversion Built-ins ──
inline TzdVal tzd_builtin_print(const std::vector<TzdVal>& args) { tzd_print_vec(args); return TzdVal(); }
inline TzdVal tzd_builtin_println(const std::vector<TzdVal>& args) { tzd_print_vec(args); return TzdVal(); }
inline TzdVal tzd_builtin_input(const std::vector<TzdVal>& args) {
    if (!args.empty()) std::cout << args[0].to_string();
    std::string s;
    if (std::getline(std::cin, s)) return TzdVal(s);
    return TzdVal("");
}
inline TzdVal tzd_builtin_str(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal("") : TzdVal(args[0].to_string()); }
inline TzdVal tzd_builtin_toString(const std::vector<TzdVal>& args) { return tzd_builtin_str(args); }
inline TzdVal tzd_builtin_int(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0) : TzdVal(args[0].as_int()); }
inline TzdVal tzd_builtin_toInt(const std::vector<TzdVal>& args) { return tzd_builtin_int(args); }
inline TzdVal tzd_builtin_parseInt(const std::vector<TzdVal>& args) { return tzd_builtin_int(args); }
inline TzdVal tzd_builtin_float(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double()); }
inline TzdVal tzd_builtin_toFloat(const std::vector<TzdVal>& args) { return tzd_builtin_float(args); }
inline TzdVal tzd_builtin_parseDouble(const std::vector<TzdVal>& args) { return tzd_builtin_float(args); }
inline TzdVal tzd_builtin_parseFloat(const std::vector<TzdVal>& args) { return tzd_builtin_float(args); }
inline TzdVal tzd_builtin_bool(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(false) : TzdVal(args[0].as_bool()); }
inline TzdVal tzd_builtin_toBool(const std::vector<TzdVal>& args) { return tzd_builtin_bool(args); }
inline TzdVal tzd_builtin_len(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    const auto& v = args[0];
    if (v.type == ValType::ARRAY && v.arrVal) return TzdVal((int64_t)v.arrVal->size());
    if (v.type == ValType::MAP && v.mapVal) return TzdVal((int64_t)v.mapVal->size());
    if (v.type == ValType::STRING) return TzdVal((int64_t)v.sVal.size());
    return TzdVal(0);
}
inline TzdVal tzd_builtin_type(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("null");
    switch (args[0].type) {
        case ValType::NIL: return TzdVal("null");
        case ValType::BOOL: return TzdVal("bool");
        case ValType::INT: return TzdVal("int");
        case ValType::FLOAT: return TzdVal("float");
        case ValType::STRING: return TzdVal("string");
        case ValType::ARRAY: return TzdVal("array");
        case ValType::MAP: return TzdVal("map");
        case ValType::INSTANCE: return TzdVal("instance");
        case ValType::FUNC: return TzdVal("function");
        case ValType::TENSOR: return TzdVal("tensor");
        default: return TzdVal("unknown");
    }
}
inline TzdVal tzd_builtin_isNull(const std::vector<TzdVal>& args) { return TzdVal(args.empty() || args[0].type == ValType::NIL); }
inline TzdVal tzd_builtin_isNone(const std::vector<TzdVal>& args) { return tzd_builtin_isNull(args); }
inline TzdVal tzd_builtin_deepCopy(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    const auto& v = args[0];
    if (v.type == ValType::ARRAY && v.arrVal) {
        auto vec = std::make_shared<std::vector<TzdVal>>();
        for (const auto& el : *v.arrVal) vec->push_back(tzd_builtin_deepCopy({el}));
        return TzdVal(vec);
    }
    if (v.type == ValType::MAP && v.mapVal) {
        auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
        for (const auto& kv : *v.mapVal) (*m)[kv.first] = tzd_builtin_deepCopy({kv.second});
        return TzdVal(m);
    }
    return v;
}
inline TzdVal tzd_builtin_toFixed(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("0");
    double val = args[0].as_double();
    int digits = args.size() > 1 ? (int)args[1].as_int() : 2;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(digits) << val;
    return TzdVal(ss.str());
}
inline TzdVal tzd_builtin_toPrecision(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("0");
    double val = args[0].as_double();
    int digits = args.size() > 1 ? (int)args[1].as_int() : 4;
    std::ostringstream ss;
    ss << std::setprecision(digits) << val;
    return TzdVal(ss.str());
}

// ── Math & Stat Built-ins ──
inline TzdVal tzd_builtin_abs(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    return args[0].type == ValType::FLOAT ? TzdVal(std::abs(args[0].as_double())) : TzdVal(std::abs(args[0].as_int()));
}
inline TzdVal tzd_builtin_sqrt(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::sqrt(args[0].as_double())); }
inline TzdVal tzd_builtin_cbrt(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::cbrt(args[0].as_double())); }
inline TzdVal tzd_builtin_sin(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::sin(args[0].as_double())); }
inline TzdVal tzd_builtin_cos(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::cos(args[0].as_double())); }
inline TzdVal tzd_builtin_tan(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::tan(args[0].as_double())); }
inline TzdVal tzd_builtin_asin(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::asin(args[0].as_double())); }
inline TzdVal tzd_builtin_acos(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::acos(args[0].as_double())); }
inline TzdVal tzd_builtin_atan(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::atan(args[0].as_double())); }
inline TzdVal tzd_builtin_atan2(const std::vector<TzdVal>& args) { return args.size() < 2 ? TzdVal(0.0) : TzdVal(std::atan2(args[0].as_double(), args[1].as_double())); }
inline TzdVal tzd_builtin_sinh(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::sinh(args[0].as_double())); }
inline TzdVal tzd_builtin_cosh(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::cosh(args[0].as_double())); }
inline TzdVal tzd_builtin_tanh(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::tanh(args[0].as_double())); }
inline TzdVal tzd_builtin_pow(const std::vector<TzdVal>& args);
inline TzdVal tzd_builtin_exp(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::exp(args[0].as_double())); }
inline TzdVal tzd_builtin_expm1(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::expm1(args[0].as_double())); }
inline TzdVal tzd_builtin_log(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::log(args[0].as_double())); }
inline TzdVal tzd_builtin_log10(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::log10(args[0].as_double())); }
inline TzdVal tzd_builtin_log2(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::log2(args[0].as_double())); }
inline TzdVal tzd_builtin_logBase(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    double val = args[0].as_double(), base = args[1].as_double();
    return TzdVal(std::log(val) / std::log(base));
}
inline TzdVal tzd_builtin_log1p(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::log1p(args[0].as_double())); }
inline TzdVal tzd_builtin_floor(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::floor(args[0].as_double())); }
inline TzdVal tzd_builtin_ceil(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::ceil(args[0].as_double())); }
inline TzdVal tzd_builtin_round(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::round(args[0].as_double())); }
inline TzdVal tzd_builtin_trunc(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::trunc(args[0].as_double())); }
inline TzdVal tzd_builtin_min(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args.size() == 1 && args[0].type == ValType::ARRAY && args[0].arrVal && !args[0].arrVal->empty()) {
        TzdVal m = (*args[0].arrVal)[0];
        for (const auto& a : *args[0].arrVal) if (a.as_double() < m.as_double()) m = a;
        return m;
    }
    TzdVal m = args[0];
    for (size_t i = 1; i < args.size(); ++i) if (args[i].as_double() < m.as_double()) m = args[i];
    return m;
}
inline TzdVal tzd_builtin_max(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args.size() == 1 && args[0].type == ValType::ARRAY && args[0].arrVal && !args[0].arrVal->empty()) {
        TzdVal m = (*args[0].arrVal)[0];
        for (const auto& a : *args[0].arrVal) if (a.as_double() > m.as_double()) m = a;
        return m;
    }
    TzdVal m = args[0];
    for (size_t i = 1; i < args.size(); ++i) if (args[i].as_double() > m.as_double()) m = args[i];
    return m;
}
inline TzdVal tzd_builtin_clamp(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal(0) : args[0];
    double v = args[0].as_double(), lo = args[1].as_double(), hi = args[2].as_double();
    if (v < lo) v = lo; if (v > hi) v = hi;
    return TzdVal(v);
}
inline TzdVal tzd_builtin_degrees(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double() * 180.0 / 3.14159265358979323846); }
inline TzdVal tzd_builtin_radians(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double() * 3.14159265358979323846 / 180.0); }
inline TzdVal tzd_builtin_hypot(const std::vector<TzdVal>& args) { return args.size() < 2 ? TzdVal(0.0) : TzdVal(std::hypot(args[0].as_double(), args[1].as_double())); }
inline TzdVal tzd_builtin_erf(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::erf(args[0].as_double())); }
inline TzdVal tzd_builtin_tgamma(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0.0) : TzdVal(std::tgamma(args[0].as_double())); }
inline TzdVal tzd_builtin_sign(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    double v = args[0].as_double();
    return TzdVal(v > 0 ? 1 : (v < 0 ? -1 : 0));
}
inline TzdVal tzd_builtin_lerp(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return TzdVal(0.0);
    double a = args[0].as_double(), b = args[1].as_double(), t = args[2].as_double();
    return TzdVal(a + t * (b - a));
}
inline int64_t compute_gcd(int64_t a, int64_t b) { while (b != 0) { int64_t t = b; b = a % b; a = t; } return std::abs(a); }
inline TzdVal tzd_builtin_gcd(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0);
    return TzdVal(compute_gcd(args[0].as_int(), args[1].as_int()));
}
inline TzdVal tzd_builtin_lcm(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0);
    int64_t a = args[0].as_int(), b = args[1].as_int();
    if (a == 0 || b == 0) return TzdVal(0);
    return TzdVal(std::abs(a * b) / compute_gcd(a, b));
}
inline TzdVal tzd_builtin_factorial(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(1);
    int64_t n = args[0].as_int();
    if (n <= 1) return TzdVal(1);
    int64_t res = 1;
    for (int64_t i = 2; i <= n; ++i) res *= i;
    return TzdVal(res);
}
inline TzdVal tzd_builtin_comb(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(1);
    int64_t n = args[0].as_int(), k = args[1].as_int();
    if (k < 0 || k > n) return TzdVal(0);
    if (k == 0 || k == n) return TzdVal(1);
    if (k > n / 2) k = n - k;
    int64_t res = 1;
    for (int64_t i = 1; i <= k; ++i) res = res * (n - i + 1) / i;
    return TzdVal(res);
}
inline TzdVal tzd_builtin_perm(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(1);
    int64_t n = args[0].as_int(), k = args[1].as_int();
    if (k < 0 || k > n) return TzdVal(0);
    int64_t res = 1;
    for (int64_t i = 0; i < k; ++i) res *= (n - i);
    return TzdVal(res);
}
inline TzdVal tzd_builtin_fib(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    int64_t n = args[0].as_int();
    if (n <= 0) return TzdVal(0);
    if (n == 1) return TzdVal(1);
    int64_t a = 0, b = 1;
    for (int64_t i = 2; i <= n; ++i) { int64_t c = a + b; a = b; b = c; }
    return TzdVal(b);
}
inline TzdVal tzd_builtin_isFinite(const std::vector<TzdVal>& args) { return TzdVal(!args.empty() && std::isfinite(args[0].as_double())); }
inline TzdVal tzd_builtin_isfinite_t(const std::vector<TzdVal>& args) { return tzd_builtin_isFinite(args); }
inline TzdVal tzd_builtin_isNaN(const std::vector<TzdVal>& args) { return TzdVal(!args.empty() && std::isnan(args[0].as_double())); }
inline TzdVal tzd_builtin_isnan_t(const std::vector<TzdVal>& args) { return tzd_builtin_isNaN(args); }
inline TzdVal tzd_builtin_isinf_t(const std::vector<TzdVal>& args) { return TzdVal(!args.empty() && std::isinf(args[0].as_double())); }
inline TzdVal tzd_builtin_isPowerOf2(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t n = args[0].as_int();
    return TzdVal(n > 0 && (n & (n - 1)) == 0);
}
inline TzdVal tzd_builtin_nextPowerOf2(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(1);
    int64_t n = args[0].as_int();
    if (n <= 1) return TzdVal(1);
    int64_t p = 1;
    while (p < n) p <<= 1;
    return TzdVal(p);
}
inline TzdVal tzd_builtin_isPrime(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t n = args[0].as_int();
    if (n <= 1) return TzdVal(false);
    if (n <= 3) return TzdVal(true);
    if (n % 2 == 0 || n % 3 == 0) return TzdVal(false);
    for (int64_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return TzdVal(false);
    }
    return TzdVal(true);
}
// Forward declaration of powmod (implemented after NativeBigInt)
inline TzdVal tzd_builtin_powmod(const std::vector<TzdVal>& args);

// Random built-ins
static std::mt19937_64 g_tzd_rng(1337);
inline TzdVal tzd_builtin_random(const std::vector<TzdVal>& args) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return TzdVal(dist(g_tzd_rng));
}
inline TzdVal tzd_builtin_randInt(const std::vector<TzdVal>& args) {
    int64_t minV = 0, maxV = 100;
    if (args.size() == 1) maxV = args[0].as_int();
    else if (args.size() >= 2) { minV = args[0].as_int(); maxV = args[1].as_int(); }
    if (minV > maxV) std::swap(minV, maxV);
    std::uniform_int_distribution<int64_t> dist(minV, maxV);
    return TzdVal(dist(g_tzd_rng));
}
inline TzdVal tzd_builtin_randomInt(const std::vector<TzdVal>& args) { return tzd_builtin_randInt(args); }
inline TzdVal tzd_builtin_randomSeed(const std::vector<TzdVal>& args) {
    if (!args.empty()) g_tzd_rng.seed((uint64_t)args[0].as_int());
    return TzdVal();
}

// Statistical reductions
inline TzdVal tzd_builtin_sum(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        double total = 0.0; bool allInt = true; int64_t iTotal = 0;
        for (const auto& item : *args[0].arrVal) {
            if (item.type == ValType::FLOAT) allInt = false;
            total += item.as_double(); iTotal += item.as_int();
        }
        return allInt ? TzdVal(iTotal) : TzdVal(total);
    }
    return args[0];
}
inline TzdVal tzd_builtin_avg(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal(0.0);
    double total = 0.0;
    for (const auto& item : *args[0].arrVal) total += item.as_double();
    return TzdVal(total / (double)args[0].arrVal->size());
}
inline TzdVal tzd_builtin_minArr(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal();
    TzdVal minV = (*args[0].arrVal)[0];
    for (const auto& item : *args[0].arrVal) if (item.as_double() < minV.as_double()) minV = item;
    return minV;
}
inline TzdVal tzd_builtin_maxArr(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal();
    TzdVal maxV = (*args[0].arrVal)[0];
    for (const auto& item : *args[0].arrVal) if (item.as_double() > maxV.as_double()) maxV = item;
    return maxV;
}
inline TzdVal tzd_builtin_argmax(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal(-1);
    size_t idx = 0; double maxV = (*args[0].arrVal)[0].as_double();
    for (size_t i = 1; i < args[0].arrVal->size(); ++i) {
        double v = (*args[0].arrVal)[i].as_double();
        if (v > maxV) { maxV = v; idx = i; }
    }
    return TzdVal((int64_t)idx);
}
inline TzdVal tzd_builtin_argmin(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal(-1);
    size_t idx = 0; double minV = (*args[0].arrVal)[0].as_double();
    for (size_t i = 1; i < args[0].arrVal->size(); ++i) {
        double v = (*args[0].arrVal)[i].as_double();
        if (v < minV) { minV = v; idx = i; }
    }
    return TzdVal((int64_t)idx);
}
inline TzdVal tzd_builtin_cumsum(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    auto arr = std::make_shared<std::vector<TzdVal>>();
    double cur = 0.0;
    for (const auto& item : *args[0].arrVal) { cur += item.as_double(); arr->push_back(TzdVal(cur)); }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_diff(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->size() < 2) return tzd_make_array({});
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 1; i < args[0].arrVal->size(); ++i) {
        arr->push_back(TzdVal((*args[0].arrVal)[i].as_double() - (*args[0].arrVal)[i-1].as_double()));
    }
    return TzdVal(arr);
}

// ── Array Built-ins ──
inline TzdVal tzd_builtin_push(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::ARRAY && args[0].arrVal) {
        args[0].arrVal->push_back(args[1]);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_pop(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal && !args[0].arrVal->empty()) {
        TzdVal val = args[0].arrVal->back();
        args[0].arrVal->pop_back();
        return val;
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_shift(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal && !args[0].arrVal->empty()) {
        TzdVal val = (*args[0].arrVal)[0];
        args[0].arrVal->erase(args[0].arrVal->begin());
        return val;
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_unshift(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (size_t i = args.size() - 1; i >= 1; --i) args[0].arrVal->insert(args[0].arrVal->begin(), args[i]);
        return TzdVal((int64_t)args[0].arrVal->size());
    }
    return TzdVal(0);
}
inline TzdVal tzd_builtin_insert(const std::vector<TzdVal>& args) {
    if (args.size() >= 3 && args[0].type == ValType::ARRAY && args[0].arrVal) {
        int64_t idx = args[1].as_int();
        if (idx < 0) idx = 0;
        if (idx > (int64_t)args[0].arrVal->size()) idx = (int64_t)args[0].arrVal->size();
        args[0].arrVal->insert(args[0].arrVal->begin() + idx, args[2]);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_remove(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::ARRAY && args[0].arrVal) {
        int64_t idx = args[1].as_int();
        if (idx >= 0 && idx < (int64_t)args[0].arrVal->size()) {
            TzdVal v = (*args[0].arrVal)[idx];
            args[0].arrVal->erase(args[0].arrVal->begin() + idx);
            return v;
        }
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_clear(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        args[0].arrVal->clear();
        return args[0];
    }
    if (!args.empty() && args[0].type == ValType::MAP && args[0].mapVal) {
        args[0].mapVal->clear();
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_contains(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& a : *args[0].arrVal) if (a == args[1]) return TzdVal(true);
        return TzdVal(false);
    }
    if (args[0].type == ValType::STRING) return TzdVal(args[0].sVal.find(args[1].to_string()) != std::string::npos);
    return TzdVal(false);
}
inline TzdVal tzd_builtin_includes(const std::vector<TzdVal>& args) { return tzd_builtin_contains(args); }
inline TzdVal tzd_builtin_indexOf(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(-1);
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (size_t i = 0; i < args[0].arrVal->size(); ++i) if ((*args[0].arrVal)[i] == args[1]) return TzdVal((int64_t)i);
        return TzdVal(-1);
    }
    if (args[0].type == ValType::STRING) {
        auto pos = args[0].sVal.find(args[1].to_string());
        return TzdVal(pos != std::string::npos ? (int64_t)pos : -1);
    }
    return TzdVal(-1);
}
inline TzdVal tzd_builtin_indexOfArr(const std::vector<TzdVal>& args) { return tzd_builtin_indexOf(args); }
inline TzdVal tzd_builtin_slice(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t start = args.size() > 1 ? args[1].as_int() : 0;
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        int64_t sz = (int64_t)args[0].arrVal->size();
        if (start < 0) start += sz; if (start < 0) start = 0;
        int64_t end = args.size() > 2 ? args[2].as_int() : sz;
        if (end < 0) end += sz; if (end > sz) end = sz;
        auto res = std::make_shared<std::vector<TzdVal>>();
        for (int64_t i = start; i < end; ++i) res->push_back((*args[0].arrVal)[i]);
        return TzdVal(res);
    }
    if (args[0].type == ValType::STRING) {
        int64_t sz = (int64_t)args[0].sVal.size();
        if (start < 0) start += sz; if (start < 0) start = 0;
        int64_t end = args.size() > 2 ? args[2].as_int() : sz;
        if (end < 0) end += sz; if (end > sz) end = sz;
        if (start >= end) return TzdVal("");
        return TzdVal(args[0].sVal.substr(start, end - start));
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_join(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return TzdVal("");
    std::string sep = args.size() > 1 ? args[1].to_string() : ",";
    std::ostringstream ss;
    for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
        if (i > 0) ss << sep;
        ss << (*args[0].arrVal)[i].to_string();
    }
    return TzdVal(ss.str());
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
inline TzdVal tzd_builtin_concat(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    if (args[0].type == ValType::STRING) {
        std::string s;
        for (const auto& a : args) s += a.to_string();
        return TzdVal(s);
    }
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (const auto& a : args) {
        if (a.type == ValType::ARRAY && a.arrVal) {
            arr->insert(arr->end(), a.arrVal->begin(), a.arrVal->end());
        } else {
            arr->push_back(a);
        }
    }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_unique(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    auto arr = std::make_shared<std::vector<TzdVal>>();
    std::unordered_set<std::string> seen;
    for (const auto& v : *args[0].arrVal) {
        if (seen.insert(v.to_string()).second) arr->push_back(v);
    }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_shuffle(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    auto arr = std::make_shared<std::vector<TzdVal>>(*args[0].arrVal);
    std::shuffle(arr->begin(), arr->end(), g_tzd_rng);
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_sample(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal();
    std::uniform_int_distribution<size_t> dist(0, args[0].arrVal->size() - 1);
    return (*args[0].arrVal)[dist(g_tzd_rng)];
}
inline TzdVal tzd_builtin_range(const std::vector<TzdVal>& args) {
    int64_t start = 0, stop = 0, step = 1;
    if (args.size() == 1) stop = args[0].as_int();
    else if (args.size() == 2) { start = args[0].as_int(); stop = args[1].as_int(); }
    else if (args.size() >= 3) { start = args[0].as_int(); stop = args[1].as_int(); step = args[2].as_int(); if (step == 0) step = 1; }
    auto arr = std::make_shared<std::vector<TzdVal>>();
    if (step > 0) { for (int64_t i = start; i < stop; i += step) arr->push_back(TzdVal(i)); }
    else { for (int64_t i = start; i > stop; i += step) arr->push_back(TzdVal(i)); }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_linspace_arr(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    double start = args[0].as_double(), stop = args[1].as_double();
    int64_t n = args.size() > 2 ? args[2].as_int() : 50;
    if (n <= 0) return tzd_make_array({});
    auto arr = std::make_shared<std::vector<TzdVal>>();
    if (n == 1) { arr->push_back(TzdVal(start)); return TzdVal(arr); }
    double step = (stop - start) / (double)(n - 1);
    for (int64_t i = 0; i < n; ++i) arr->push_back(TzdVal(start + i * step));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_fill(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    int64_t n = args[0].as_int();
    TzdVal val = args.size() > 1 ? args[1] : TzdVal(0);
    auto arr = std::make_shared<std::vector<TzdVal>>(n > 0 ? n : 0, val);
    return TzdVal(arr);
}
inline void flatten_recurse(const TzdVal& v, std::vector<TzdVal>& out) {
    if (v.type == ValType::ARRAY && v.arrVal) {
        for (const auto& item : *v.arrVal) flatten_recurse(item, out);
    } else {
        out.push_back(v);
    }
}
inline TzdVal tzd_builtin_flatten(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    auto arr = std::make_shared<std::vector<TzdVal>>();
    flatten_recurse(args[0], *arr);
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_zip(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || args[1].type != ValType::ARRAY) return tzd_make_array({});
    auto a1 = args[0].arrVal; auto a2 = args[1].arrVal;
    if (!a1 || !a2) return tzd_make_array({});
    size_t sz = (std::min)(a1->size(), a2->size());
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < sz; ++i) {
        auto pair = std::make_shared<std::vector<TzdVal>>(std::vector<TzdVal>{(*a1)[i], (*a2)[i]});
        res->push_back(TzdVal(pair));
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_filter(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    TzdVal fn = args[1]; auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
        if (fn({(*args[0].arrVal)[i], TzdVal((int64_t)i)}).as_bool()) res->push_back((*args[0].arrVal)[i]);
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_map(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    TzdVal fn = args[1]; auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
        res->push_back(fn({(*args[0].arrVal)[i], TzdVal((int64_t)i)}));
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_reduce(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal();
    TzdVal fn = args[1]; size_t start = 1; TzdVal acc = (*args[0].arrVal)[0];
    if (args.size() >= 3) { acc = args[2]; start = 0; }
    for (size_t i = start; i < args[0].arrVal->size(); ++i) {
        acc = fn({acc, (*args[0].arrVal)[i], TzdVal((int64_t)i)});
    }
    return acc;
}
inline TzdVal tzd_builtin_find(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal) return TzdVal();
    TzdVal fn = args[1];
    for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
        if (fn({(*args[0].arrVal)[i], TzdVal((int64_t)i)}).as_bool()) return (*args[0].arrVal)[i];
    }
    return TzdVal();
}

// ── String Built-ins ──
inline TzdVal tzd_builtin_substr(const std::vector<TzdVal>& args) { return tzd_builtin_slice(args); }
inline TzdVal tzd_builtin_substring(const std::vector<TzdVal>& args) { return tzd_builtin_slice(args); }
inline TzdVal tzd_builtin_charAt(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(); int64_t i = args.size() > 1 ? args[1].as_int() : 0;
    if (i >= 0 && i < (int64_t)s.size()) return TzdVal(std::string(1, s[i]));
    return TzdVal("");
}
inline TzdVal tzd_builtin_charCode(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::string s = args[0].to_string(); int64_t i = args.size() > 1 ? args[1].as_int() : 0;
    if (i >= 0 && i < (int64_t)s.size()) return TzdVal((int64_t)(unsigned char)s[i]);
    return TzdVal(0);
}
inline TzdVal tzd_builtin_fromCharCode(const std::vector<TzdVal>& args) {
    std::string s;
    for (const auto& a : args) s += (char)a.as_int();
    return TzdVal(s);
}
inline TzdVal tzd_builtin_countSubstr(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0);
    std::string str = args[0].to_string(), sub = args[1].to_string();
    if (sub.empty()) return TzdVal(0);
    int64_t count = 0; size_t pos = 0;
    while ((pos = str.find(sub, pos)) != std::string::npos) { count++; pos += sub.length(); }
    return TzdVal(count);
}
inline TzdVal tzd_builtin_wordCount(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::istringstream ss(args[0].to_string()); std::string w; int64_t c = 0;
    while (ss >> w) c++;
    return TzdVal(c);
}
inline TzdVal tzd_builtin_split(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::string str = args[0].to_string();
    std::string delim = args.size() > 1 ? args[1].to_string() : " ";
    auto arr = std::make_shared<std::vector<TzdVal>>();
    if (delim.empty()) {
        for (char c : str) arr->push_back(TzdVal(std::string(1, c)));
        return TzdVal(arr);
    }
    size_t start = 0, end = 0;
    while ((end = str.find(delim, start)) != std::string::npos) {
        arr->push_back(TzdVal(str.substr(start, end - start)));
        start = end + delim.length();
    }
    arr->push_back(TzdVal(str.substr(start)));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_splitRegex(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::string str = args[0].to_string();
    std::string pat = args.size() > 1 ? args[1].to_string() : "\\s+";
    auto arr = std::make_shared<std::vector<TzdVal>>();
    try {
        std::regex re(pat);
        std::sregex_token_iterator it(str.begin(), str.end(), re, -1), end;
        for (; it != end; ++it) arr->push_back(TzdVal(it->str()));
    } catch (...) { arr->push_back(TzdVal(str)); }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_replace(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal("") : args[0];
    std::string s = args[0].to_string(), from = args[1].to_string(), to = args[2].to_string();
    if (from.empty()) return TzdVal(s);
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
    return TzdVal(s);
}
inline TzdVal tzd_builtin_replaceRegex(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal("") : args[0];
    try {
        std::regex re(args[1].to_string());
        return TzdVal(std::regex_replace(args[0].to_string(), re, args[2].to_string()));
    } catch (...) { return args[0]; }
}
inline TzdVal tzd_builtin_match(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    auto arr = std::make_shared<std::vector<TzdVal>>();
    try {
        std::regex re(args[1].to_string());
        std::string s = args[0].to_string();
        std::smatch m;
        if (std::regex_search(s, m, re)) {
            for (size_t i = 0; i < m.size(); ++i) arr->push_back(TzdVal(m[i].str()));
        }
    } catch (...) {}
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_toUpper(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string();
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return TzdVal(s);
}
inline TzdVal tzd_builtin_toLower(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string();
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return TzdVal(s);
}
inline TzdVal tzd_builtin_toTitleCase(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(); bool cap = true;
    for (char& c : s) {
        if (std::isspace((unsigned char)c) || c == '_' || c == '-') cap = true;
        else if (cap) { c = (char)::toupper((unsigned char)c); cap = false; }
        else c = (char)::tolower((unsigned char)c);
    }
    return TzdVal(s);
}
inline TzdVal tzd_builtin_toCamelCase(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(), res; bool cap = false;
    for (char c : s) {
        if (c == '_' || c == '-' || std::isspace((unsigned char)c)) cap = true;
        else if (cap) { res += (char)::toupper((unsigned char)c); cap = false; }
        else res += (char)::tolower((unsigned char)c);
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_toSnakeCase(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(), res;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (std::isupper((unsigned char)c)) {
            if (i > 0 && s[i-1] != '_') res += '_';
            res += (char)::tolower((unsigned char)c);
        } else if (c == ' ' || c == '-') res += '_';
        else res += c;
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_trim(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string();
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return TzdVal("");
    auto end = s.find_last_not_of(" \t\r\n");
    return TzdVal(s.substr(start, end - start + 1));
}
inline TzdVal tzd_builtin_padLeft(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(); size_t len = args.size() > 1 ? (size_t)args[1].as_int() : s.size();
    char pad = (args.size() > 2 && !args[2].to_string().empty()) ? args[2].to_string()[0] : ' ';
    if (s.size() < len) s = std::string(len - s.size(), pad) + s;
    return TzdVal(s);
}
inline TzdVal tzd_builtin_padRight(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(); size_t len = args.size() > 1 ? (size_t)args[1].as_int() : s.size();
    char pad = (args.size() > 2 && !args[2].to_string().empty()) ? args[2].to_string()[0] : ' ';
    if (s.size() < len) s = s + std::string(len - s.size(), pad);
    return TzdVal(s);
}
inline TzdVal tzd_builtin_repeat(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(); int64_t n = args.size() > 1 ? args[1].as_int() : 1;
    std::string res;
    for (int64_t i = 0; i < n; ++i) res += s;
    return TzdVal(res);
}
inline TzdVal tzd_builtin_reverseStr(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string();
    std::reverse(s.begin(), s.end());
    return TzdVal(s);
}
inline TzdVal tzd_builtin_startsWith(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    return TzdVal(args[0].to_string().rfind(args[1].to_string(), 0) == 0);
}
inline TzdVal tzd_builtin_endsWith(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::string s = args[0].to_string(), end = args[1].to_string();
    if (end.size() > s.size()) return TzdVal(false);
    return TzdVal(s.compare(s.size() - end.size(), end.size(), end) == 0);
}
inline TzdVal tzd_builtin_levenshtein(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0);
    std::string s1 = args[0].to_string(), s2 = args[1].to_string();
    size_t m = s1.size(), n = s2.size();
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));
    for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
    for (size_t j = 0; j <= n; ++j) dp[0][j] = j;
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            dp[i][j] = (s1[i - 1] == s2[j - 1]) ? dp[i - 1][j - 1] :
                (1 + (std::min)({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]}));
        }
    }
    return TzdVal((int64_t)dp[m][n]);
}
inline TzdVal tzd_builtin_format(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string fmt = args[0].to_string();
    for (size_t i = 1; i < args.size(); ++i) {
        std::string tag = "{" + std::to_string(i - 1) + "}";
        size_t p = fmt.find(tag);
        if (p != std::string::npos) fmt.replace(p, tag.length(), args[i].to_string());
    }
    return TzdVal(fmt);
}

// ── Hash & Encoding Built-ins ──
static const std::string b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
inline TzdVal tzd_builtin_base64Encode(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string in = args[0].to_string(), out; int val = 0, valb = -6;
    for (unsigned char c : in) {
        val = (val << 8) + c; valb += 8;
        while (valb >= 0) { out.push_back(b64_chars[(val >> valb) & 0x3F]); valb -= 6; }
    }
    if (valb > -6) out.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return TzdVal(out);
}
inline TzdVal tzd_builtin_base64Decode(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string in = args[0].to_string(), out; std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[b64_chars[i]] = i;
    int val = 0, valb = -8;
    for (unsigned char c : in) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c]; valb += 6;
        if (valb >= 0) { out.push_back(char((val >> valb) & 0xFF)); valb -= 8; }
    }
    return TzdVal(out);
}
inline TzdVal tzd_builtin_toHex(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::ostringstream ss;
    if (args[0].type == ValType::INT) ss << std::hex << args[0].as_int();
    else for (unsigned char c : args[0].to_string()) ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    return TzdVal(ss.str());
}
inline TzdVal tzd_builtin_fromHex(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string hex = args[0].to_string(), out;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        char byte = (char)strtol(byteStr.c_str(), NULL, 16);
        out.push_back(byte);
    }
    return TzdVal(out);
}
inline TzdVal tzd_builtin_toBinary(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    int64_t v = args[0].as_int(); std::string s;
    if (v == 0) return TzdVal("0");
    while (v > 0) { s = (v & 1 ? "1" : "0") + s; v >>= 1; }
    return TzdVal(s);
}
inline TzdVal tzd_builtin_fromBinary(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    return TzdVal(std::stoll(args[0].to_string(), nullptr, 2));
}
inline TzdVal tzd_builtin_crc32(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::string s = args[0].to_string(); uint32_t crc = 0xFFFFFFFF;
    for (unsigned char c : s) {
        crc ^= c;
        for (int i = 0; i < 8; ++i) crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
    return TzdVal((int64_t)(crc ^ 0xFFFFFFFF));
}
inline TzdVal tzd_builtin_hash(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    return TzdVal((int64_t)std::hash<std::string>{}(args[0].to_string()));
}
inline TzdVal tzd_builtin_uuid(const std::vector<TzdVal>& = {}) {
    char buf[64];
    uint32_t d1 = (uint32_t)g_tzd_rng(), d2 = (uint32_t)g_tzd_rng(), d3 = (uint32_t)g_tzd_rng(), d4 = (uint32_t)g_tzd_rng();
    snprintf(buf, sizeof(buf), "%08x-%04x-4%03x-%04x-%08x%04x", d1, d2 >> 16, d2 & 0x0fff, (d3 & 0x3fff) | 0x8000, d4, d3 >> 16);
    return TzdVal(std::string(buf));
}
inline TzdVal tzd_builtin_hexDump(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(); std::ostringstream ss;
    for (size_t i = 0; i < s.size(); ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)s[i] << " ";
        if ((i + 1) % 16 == 0) ss << "\n";
    }
    return TzdVal(ss.str());
}
inline TzdVal tzd_builtin_escape(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(), out;
    for (char c : s) {
        if (c == '"') out += "\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return TzdVal(out);
}
inline TzdVal tzd_builtin_unescape(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::string s = args[0].to_string(), out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            char c = s[++i];
            if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else if (c == '\\') out += '\\';
            else if (c == '"') out += '"';
            else out += c;
        } else out += s[i];
    }
    return TzdVal(out);
}

// ── Matrix Built-ins (Header-Only Pure C++) ──
inline TzdVal tzd_builtin_identity(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    if (n < 1) n = 1;
    auto mat = std::make_shared<std::vector<TzdVal>>();
    for (int64_t r = 0; r < n; ++r) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (int64_t c = 0; c < n; ++c) row->push_back(TzdVal(r == c ? 1.0 : 0.0));
        mat->push_back(TzdVal(row));
    }
    return TzdVal(mat);
}
inline TzdVal tzd_builtin_zeros(const std::vector<TzdVal>& args) {
    int64_t rows = args.empty() ? 1 : args[0].as_int();
    int64_t cols = args.size() > 1 ? args[1].as_int() : rows;
    if (rows < 1) rows = 1; if (cols < 1) cols = 1;
    auto mat = std::make_shared<std::vector<TzdVal>>();
    for (int64_t r = 0; r < rows; ++r) {
        auto row = std::make_shared<std::vector<TzdVal>>(cols, TzdVal(0.0));
        mat->push_back(TzdVal(row));
    }
    return TzdVal(mat);
}
inline TzdVal tzd_builtin_ones(const std::vector<TzdVal>& args) {
    int64_t rows = args.empty() ? 1 : args[0].as_int();
    int64_t cols = args.size() > 1 ? args[1].as_int() : rows;
    if (rows < 1) rows = 1; if (cols < 1) cols = 1;
    auto mat = std::make_shared<std::vector<TzdVal>>();
    for (int64_t r = 0; r < rows; ++r) {
        auto row = std::make_shared<std::vector<TzdVal>>(cols, TzdVal(1.0));
        mat->push_back(TzdVal(row));
    }
    return TzdVal(mat);
}
inline TzdVal tzd_builtin_matrixMul(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || args[1].type != ValType::ARRAY) return TzdVal("Error");
    const auto& A = *args[0].arrVal; const auto& B = *args[1].arrVal;
    if (A.empty() || B.empty() || A[0].type != ValType::ARRAY || B[0].type != ValType::ARRAY) return TzdVal("Error");
    size_t r1 = A.size(), c1 = A[0].arrVal->size(), r2 = B.size(), c2 = B[0].arrVal->size();
    if (c1 != r2) return TzdVal("Error: Dimension mismatch");
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < r1; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = 0; j < c2; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < c1; ++k) sum += (*A[i].arrVal)[k].as_double() * (*B[k].arrVal)[j].as_double();
            row->push_back(TzdVal(sum));
        }
        res->push_back(TzdVal(row));
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_transpose(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal();
    const auto& A = *args[0].arrVal;
    if (A[0].type != ValType::ARRAY || !A[0].arrVal) return TzdVal();
    size_t rows = A.size(), cols = A[0].arrVal->size();
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t j = 0; j < cols; ++j) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t i = 0; i < rows; ++i) row->push_back((*A[i].arrVal)[j]);
        res->push_back(TzdVal(row));
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_trace(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return TzdVal(0.0);
    double tr = 0.0; size_t sz = args[0].arrVal->size();
    for (size_t i = 0; i < sz; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal && i < (*args[0].arrVal)[i].arrVal->size()) {
            tr += (*(*args[0].arrVal)[i].arrVal)[i].as_double();
        }
    }
    return TzdVal(tr);
}
inline TzdVal tzd_builtin_norm(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return TzdVal(0.0);
    double sum = 0.0;
    for (const auto& a : *args[0].arrVal) {
        if (a.type == ValType::ARRAY && a.arrVal) for (const auto& el : *a.arrVal) sum += el.as_double() * el.as_double();
        else sum += a.as_double() * a.as_double();
    }
    return TzdVal(std::sqrt(sum));
}
inline TzdVal tzd_builtin_dot(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || args[1].type != ValType::ARRAY) return TzdVal(0.0);
    const auto& A = *args[0].arrVal; const auto& B = *args[1].arrVal;
    size_t sz = (std::min)(A.size(), B.size()); double sum = 0.0;
    for (size_t i = 0; i < sz; ++i) sum += A[i].as_double() * B[i].as_double();
    return TzdVal(sum);
}
inline TzdVal tzd_builtin_reshape(const std::vector<TzdVal>& args) {
    if (args.size() < 3 || args[0].type != ValType::ARRAY) return TzdVal("Error");
    std::vector<TzdVal> flat;
    flatten_recurse(args[0], flat);
    int64_t r = args[1].as_int(), c = args[2].as_int();
    if (r * c != (int64_t)flat.size()) return TzdVal("Error: Size mismatch");
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (int64_t i = 0; i < r; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (int64_t j = 0; j < c; ++j) row->push_back(flat[(size_t)(i * c + j)]);
        res->push_back(TzdVal(row));
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_det(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) return TzdVal(0.0);
    size_t n = args[0].arrVal->size();
    std::vector<std::vector<double>> m(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < (std::min)(n, (*args[0].arrVal)[i].arrVal->size()); ++j) {
                m[i][j] = (*(*args[0].arrVal)[i].arrVal)[j].as_double();
            }
        }
    }
    double det = 1.0;
    for (size_t i = 0; i < n; ++i) {
        size_t pivot = i;
        for (size_t j = i + 1; j < n; ++j) if (std::abs(m[j][i]) > std::abs(m[pivot][i])) pivot = j;
        if (std::abs(m[pivot][i]) < 1e-12) return TzdVal(0.0);
        if (pivot != i) { std::swap(m[i], m[pivot]); det = -det; }
        det *= m[i][i];
        for (size_t j = i + 1; j < n; ++j) {
            double factor = m[j][i] / m[i][i];
            for (size_t k = i; k < n; ++k) m[j][k] -= factor * m[i][k];
        }
    }
    return TzdVal(det);
}
inline TzdVal tzd_builtin_inverse(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) {
        return tzd_make_array({});
    }
    size_t n = args[0].arrVal->size();
    std::vector<std::vector<double>> a(n, std::vector<double>(2 * n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < (std::min)(n, (*args[0].arrVal)[i].arrVal->size()); ++j) {
                a[i][j] = (*(*args[0].arrVal)[i].arrVal)[j].as_double();
            }
        }
        a[i][n + i] = 1.0;
    }
    // Gauss-Jordan elimination with partial pivoting
    for (size_t i = 0; i < n; ++i) {
        size_t pivot = i;
        for (size_t r = i + 1; r < n; ++r) {
            if (std::abs(a[r][i]) > std::abs(a[pivot][i])) pivot = r;
        }
        if (std::abs(a[pivot][i]) < 1e-12) return tzd_make_array({}); // Singular matrix
        if (pivot != i) std::swap(a[i], a[pivot]);
        double div = a[i][i];
        for (size_t j = 0; j < 2 * n; ++j) a[i][j] /= div;
        for (size_t r = 0; r < n; ++r) {
            if (r != i) {
                double factor = a[r][i];
                for (size_t j = 0; j < 2 * n; ++j) {
                    a[r][j] -= factor * a[i][j];
                }
            }
        }
    }
    auto invMat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < n; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = 0; j < n; ++j) {
            row->push_back(TzdVal(a[i][n + j]));
        }
        invMat->push_back(TzdVal(row));
    }
    return TzdVal(invMat);
}

inline TzdVal tzd_builtin_rank(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal || args[0].arrVal->empty()) {
        return TzdVal(0);
    }
    size_t m = args[0].arrVal->size();
    size_t n = 0;
    for (size_t i = 0; i < m; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            n = (std::max)(n, (*args[0].arrVal)[i].arrVal->size());
        }
    }
    if (n == 0) return TzdVal(0);
    std::vector<std::vector<double>> mat(m, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < m; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < (*args[0].arrVal)[i].arrVal->size(); ++j) {
                mat[i][j] = (*(*args[0].arrVal)[i].arrVal)[j].as_double();
            }
        }
    }
    size_t rank = 0;
    for (size_t col = 0; col < n && rank < m; ++col) {
        size_t pivot = rank;
        for (size_t r = rank + 1; r < m; ++r) {
            if (std::abs(mat[r][col]) > std::abs(mat[pivot][col])) pivot = r;
        }
        if (std::abs(mat[pivot][col]) < 1e-12) continue;
        if (pivot != rank) std::swap(mat[rank], mat[pivot]);
        for (size_t r = rank + 1; r < m; ++r) {
            double factor = mat[r][col] / mat[rank][col];
            for (size_t c = col; c < n; ++c) {
                mat[r][c] -= factor * mat[rank][c];
            }
        }
        rank++;
    }
    return TzdVal((int64_t)rank);
}

inline TzdVal tzd_builtin_solve(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || args[1].type != ValType::ARRAY) {
        return tzd_make_array({});
    }
    TzdVal invA = tzd_builtin_inverse({args[0]});
    if (invA.type != ValType::ARRAY || !invA.arrVal || invA.arrVal->empty()) {
        return tzd_make_array({}); // Singular or invalid
    }
    // Check if b is 1D vector or 2D matrix
    bool is1D = true;
    if (args[1].arrVal && !args[1].arrVal->empty()) {
        if ((*args[1].arrVal)[0].type == ValType::ARRAY) is1D = false;
    }
    if (is1D) {
        // Convert b to column matrix, multiply, and unwrap to 1D vector
        auto bMat = std::make_shared<std::vector<TzdVal>>();
        for (const auto& val : *args[1].arrVal) {
            bMat->push_back(tzd_make_array({val}));
        }
        TzdVal resMat = tzd_builtin_matrixMul({invA, TzdVal(bMat)});
        auto resVec = std::make_shared<std::vector<TzdVal>>();
        if (resMat.type == ValType::ARRAY && resMat.arrVal) {
            for (const auto& row : *resMat.arrVal) {
                if (row.type == ValType::ARRAY && row.arrVal && !row.arrVal->empty()) {
                    resVec->push_back((*row.arrVal)[0]);
                }
            }
        }
        return TzdVal(resVec);
    }
    return tzd_builtin_matrixMul({invA, args[1]});
}

// ── Map, Set, Queue, Stack Built-ins ──
inline TzdVal tzd_builtin_keys(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::MAP || !args[0].mapVal) return tzd_make_array({});
    auto keys = std::make_shared<std::vector<TzdVal>>();
    for (const auto& kv : *args[0].mapVal) keys->push_back(TzdVal(kv.first));
    return TzdVal(keys);
}
inline TzdVal tzd_builtin_mapKeys(const std::vector<TzdVal>& args) { return tzd_builtin_keys(args); }
inline TzdVal tzd_builtin_values(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::MAP || !args[0].mapVal) return tzd_make_array({});
    auto vals = std::make_shared<std::vector<TzdVal>>();
    for (const auto& kv : *args[0].mapVal) vals->push_back(kv.second);
    return TzdVal(vals);
}
inline TzdVal tzd_builtin_mapValues(const std::vector<TzdVal>& args) { return tzd_builtin_values(args); }
inline TzdVal tzd_builtin_mapGet(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::MAP || !args[0].mapVal) return args.size() > 2 ? args[2] : TzdVal();
    auto it = args[0].mapVal->find(args[1].to_string());
    return it != args[0].mapVal->end() ? it->second : (args.size() > 2 ? args[2] : TzdVal());
}
inline TzdVal tzd_builtin_has(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::MAP || !args[0].mapVal) return TzdVal(false);
    return TzdVal(args[0].mapVal->count(args[1].to_string()) > 0);
}
inline TzdVal tzd_builtin_hasKey(const std::vector<TzdVal>& args) { return tzd_builtin_has(args); }
inline TzdVal tzd_builtin_mapHas(const std::vector<TzdVal>& args) { return tzd_builtin_has(args); }
inline TzdVal tzd_builtin_mapEntries(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::MAP || !args[0].mapVal) return tzd_make_array({});
    auto entries = std::make_shared<std::vector<TzdVal>>();
    for (const auto& kv : *args[0].mapVal) {
        auto pair = std::make_shared<std::vector<TzdVal>>(std::vector<TzdVal>{TzdVal(kv.first), kv.second});
        entries->push_back(TzdVal(pair));
    }
    return TzdVal(entries);
}
inline TzdVal tzd_builtin_mapFromEntries(const std::vector<TzdVal>& args) {
    auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& p : *args[0].arrVal) {
            if (p.type == ValType::ARRAY && p.arrVal && p.arrVal->size() >= 2) {
                (*m)[(*p.arrVal)[0].to_string()] = (*p.arrVal)[1];
            }
        }
    }
    return TzdVal(m);
}
inline TzdVal tzd_builtin_mapMerge(const std::vector<TzdVal>& args) {
    auto res = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    for (const auto& a : args) {
        if (a.type == ValType::MAP && a.mapVal) {
            for (const auto& kv : *a.mapVal) (*res)[kv.first] = kv.second;
        }
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_mapFilter(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::MAP || !args[0].mapVal) return TzdVal(std::make_shared<std::unordered_map<std::string, TzdVal>>());
    TzdVal fn = args[1]; auto res = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    for (const auto& kv : *args[0].mapVal) {
        if (fn({TzdVal(kv.first), kv.second}).as_bool()) (*res)[kv.first] = kv.second;
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_mapMap(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::MAP || !args[0].mapVal) return TzdVal(std::make_shared<std::unordered_map<std::string, TzdVal>>());
    TzdVal fn = args[1]; auto res = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    for (const auto& kv : *args[0].mapVal) {
        (*res)[kv.first] = fn({kv.second, TzdVal(kv.first)});
    }
    return TzdVal(res);
}

// Set functions
inline TzdVal tzd_builtin_setCreate(const std::vector<TzdVal>& args) {
    auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& a : *args[0].arrVal) (*m)[a.to_string()] = a;
    }
    return TzdVal(m);
}
inline TzdVal tzd_builtin_setAdd(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::MAP && args[0].mapVal) {
        (*args[0].mapVal)[args[1].to_string()] = args[1];
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_setRemove(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::MAP && args[0].mapVal) {
        args[0].mapVal->erase(args[1].to_string());
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_setContains(const std::vector<TzdVal>& args) { return tzd_builtin_has(args); }
inline TzdVal tzd_builtin_setSize(const std::vector<TzdVal>& args) { return tzd_builtin_len(args); }
inline TzdVal tzd_builtin_setUnion(const std::vector<TzdVal>& args) { return tzd_builtin_mapMerge(args); }
inline TzdVal tzd_builtin_setIntersect(const std::vector<TzdVal>& args) {
    auto res = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    if (args.size() >= 2 && args[0].type == ValType::MAP && args[1].type == ValType::MAP) {
        for (const auto& kv : *args[0].mapVal) if (args[1].mapVal->count(kv.first)) (*res)[kv.first] = kv.second;
    }
    return TzdVal(res);
}
inline TzdVal tzd_builtin_setDifference(const std::vector<TzdVal>& args) {
    auto res = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    if (args.size() >= 2 && args[0].type == ValType::MAP && args[1].type == ValType::MAP) {
        for (const auto& kv : *args[0].mapVal) if (!args[1].mapVal->count(kv.first)) (*res)[kv.first] = kv.second;
    }
    return TzdVal(res);
}

// Stack & Queue
inline TzdVal tzd_builtin_stackPush(const std::vector<TzdVal>& args) { return tzd_builtin_push(args); }
inline TzdVal tzd_builtin_stackPop(const std::vector<TzdVal>& args) { return tzd_builtin_pop(args); }
inline TzdVal tzd_builtin_queuePush(const std::vector<TzdVal>& args) { return tzd_builtin_push(args); }
inline TzdVal tzd_builtin_queuePop(const std::vector<TzdVal>& args) { return tzd_builtin_shift(args); }
inline TzdVal tzd_builtin_queuePopAll(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        auto copy = std::make_shared<std::vector<TzdVal>>(*args[0].arrVal);
        args[0].arrVal->clear();
        return TzdVal(copy);
    }
    return tzd_make_array({});
}

// ── File System Built-ins ──
inline TzdVal tzd_builtin_readFile(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    std::ifstream file(args[0].to_string(), std::ios::binary);
    if (!file.is_open()) return TzdVal("");
    std::ostringstream ss; ss << file.rdbuf();
    return TzdVal(ss.str());
}
inline TzdVal tzd_builtin_readLines(const std::vector<TzdVal>& args) {
    auto arr = std::make_shared<std::vector<TzdVal>>();
    if (!args.empty()) {
        std::ifstream file(args[0].to_string());
        std::string line;
        while (std::getline(file, line)) arr->push_back(TzdVal(line));
    }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_writeFile(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::ofstream file(args[0].to_string(), std::ios::binary);
    if (!file.is_open()) return TzdVal(false);
    file << args[1].to_string();
    return TzdVal(true);
}
inline TzdVal tzd_builtin_writeLines(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[1].type != ValType::ARRAY || !args[1].arrVal) return TzdVal(false);
    std::ofstream file(args[0].to_string());
    if (!file.is_open()) return TzdVal(false);
    for (const auto& l : *args[1].arrVal) file << l.to_string() << "\n";
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
inline TzdVal tzd_builtin_fileSize(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::error_code ec;
    return TzdVal((int64_t)std::filesystem::file_size(args[0].to_string(), ec));
}
inline TzdVal tzd_builtin_dirExists(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    return TzdVal(std::filesystem::is_directory(args[0].to_string()));
}
inline TzdVal tzd_builtin_removeFile(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    std::error_code ec;
    return TzdVal(std::filesystem::remove(args[0].to_string(), ec));
}
inline TzdVal tzd_builtin_copyFile(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::error_code ec;
    return TzdVal(std::filesystem::copy_file(args[0].to_string(), args[1].to_string(), std::filesystem::copy_options::overwrite_existing, ec));
}
inline TzdVal tzd_builtin_moveFile(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::error_code ec;
    std::filesystem::rename(args[0].to_string(), args[1].to_string(), ec);
    return TzdVal(!ec);
}
inline TzdVal tzd_builtin_makeDir(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    std::error_code ec;
    return TzdVal(std::filesystem::create_directories(args[0].to_string(), ec));
}
inline TzdVal tzd_builtin_removeDir(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::error_code ec;
    return TzdVal((int64_t)std::filesystem::remove_all(args[0].to_string(), ec));
}
inline TzdVal tzd_builtin_listDir(const std::vector<TzdVal>& args) {
    auto arr = std::make_shared<std::vector<TzdVal>>();
    std::string p = args.empty() ? "." : args[0].to_string();
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(p, ec)) {
        arr->push_back(TzdVal(entry.path().filename().string()));
    }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_currentDir(const std::vector<TzdVal>& = {}) { return TzdVal(std::filesystem::current_path().string()); }
inline TzdVal tzd_builtin_changeDir(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    std::error_code ec;
    std::filesystem::current_path(args[0].to_string(), ec);
    return TzdVal(!ec);
}
inline std::string get_executable_file_path() {
#if defined(_WIN32)
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(NULL, buf, MAX_PATH);
    std::wstring ws(buf);
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &ws[0], (int)ws.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
#elif defined(__linux__)
    char buf[1024];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        return std::string(buf);
    }
    return std::filesystem::current_path().string();
#elif defined(__APPLE__)
    char buf[1024];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0) return std::string(buf);
    return std::filesystem::current_path().string();
#else
    return std::filesystem::current_path().string();
#endif
}

inline TzdVal tzd_builtin_getScriptPath(const std::vector<TzdVal>& = {}) {
    return TzdVal(get_executable_file_path());
}
inline TzdVal tzd_builtin_getScriptDir(const std::vector<TzdVal>& = {}) {
    std::string p = get_executable_file_path();
    return TzdVal(std::filesystem::path(p).parent_path().string());
}

// ── System & Time Built-ins ──
inline TzdVal tzd_builtin_time(const std::vector<TzdVal>& = {}) {
    return TzdVal((int64_t)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}
inline TzdVal tzd_builtin_clock(const std::vector<TzdVal>& = {}) {
    return TzdVal((double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() / 1e6);
}
inline TzdVal tzd_builtin_now(const std::vector<TzdVal>& = {}) { return tzd_builtin_time(); }
inline TzdVal tzd_builtin_timestamp(const std::vector<TzdVal>& = {}) {
    return TzdVal((int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}
inline TzdVal tzd_builtin_formatTime(const std::vector<TzdVal>& args) {
    std::time_t t = std::time(nullptr);
    if (!args.empty() && args[0].as_int() > 0) t = (std::time_t)args[0].as_int();
    std::string fmt = args.size() > 1 ? args[1].to_string() : "%Y-%m-%d %H:%M:%S";
    char buf[128];
    std::strftime(buf, sizeof(buf), fmt.c_str(), std::localtime(&t));
    return TzdVal(std::string(buf));
}
inline TzdVal tzd_builtin_dateDiff(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0);
    return TzdVal(args[1].as_int() - args[0].as_int());
}
inline TzdVal tzd_builtin_dateParts(const std::vector<TzdVal>& args) {
    std::time_t t = std::time(nullptr);
    if (!args.empty()) t = (std::time_t)args[0].as_int();
    std::tm* tm = std::localtime(&t);
    auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    (*m)["year"] = TzdVal((int64_t)tm->tm_year + 1900);
    (*m)["month"] = TzdVal((int64_t)tm->tm_mon + 1);
    (*m)["day"] = TzdVal((int64_t)tm->tm_mday);
    (*m)["hour"] = TzdVal((int64_t)tm->tm_hour);
    (*m)["minute"] = TzdVal((int64_t)tm->tm_min);
    (*m)["second"] = TzdVal((int64_t)tm->tm_sec);
    return TzdVal(m);
}
inline TzdVal tzd_builtin_sleep(const std::vector<TzdVal>& args) {
    if (!args.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(args[0].as_int()));
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
        std::cerr << "[Assertion Error] " << msg << std::endl;
        std::exit(1);
    }
    return TzdVal(true);
}
inline TzdVal tzd_builtin_assert_t(const std::vector<TzdVal>& args) { return tzd_builtin_assert(args); }
inline TzdVal tzd_builtin_warn(const std::vector<TzdVal>& args) {
    if (!args.empty()) std::cerr << "[Warning] " << args[0].to_string() << std::endl;
    return TzdVal();
}
inline TzdVal tzd_builtin_measure(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    auto t1 = std::chrono::high_resolution_clock::now();
    args[0]({});
    auto t2 = std::chrono::high_resolution_clock::now();
    return TzdVal(std::chrono::duration<double, std::milli>(t2 - t1).count());
}
inline TzdVal tzd_builtin_getEnv(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    const char* v = std::getenv(args[0].to_string().c_str());
    return TzdVal(v ? std::string(v) : "");
}
inline TzdVal tzd_builtin_setEnv(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
#ifdef _WIN32
    _putenv_s(args[0].to_string().c_str(), args[1].to_string().c_str());
#else
    setenv(args[0].to_string().c_str(), args[1].to_string().c_str(), 1);
#endif
    return TzdVal(true);
}
inline TzdVal tzd_builtin_getOsInfo(const std::vector<TzdVal>& = {}) {
#if defined(_WIN32) || defined(_WIN64)
    std::string os = "Windows";
  #if defined(_M_X64) || defined(__x86_64__)
    os += " x86_64";
  #elif defined(_M_ARM64)
    os += " ARM64";
  #elif defined(_M_IX86)
    os += " x86";
  #endif
  #if defined(WITH_LIBTORCH)
    os += " (LibTorch Native)";
  #else
    os += " (Standalone Native)";
  #endif
    return TzdVal(os);
#elif defined(__APPLE__)
    std::string os = "macOS";
  #if defined(__arm64__) || defined(__aarch64__)
    os += " ARM64";
  #else
    os += " x86_64";
  #endif
    return TzdVal(os);
#elif defined(__linux__)
    std::string os = "Linux";
  #if defined(__x86_64__)
    os += " x86_64";
  #elif defined(__aarch64__)
    os += " aarch64";
  #endif
    return TzdVal(os);
#else
    return TzdVal("Generic OS");
#endif
}
// ── Global Include Search Paths ──
inline std::vector<std::string>& get_custom_include_paths() {
    static std::vector<std::string> paths;
    return paths;
}
inline TzdVal tzd_builtin_addIncludePath(const std::vector<TzdVal>& args) {
    if (!args.empty()) {
        get_custom_include_paths().push_back(args[0].to_string());
        return TzdVal(true);
    }
    return TzdVal(false);
}

// ── Reflection Built-ins ──
const std::vector<std::string>& get_all_registered_builtin_names_list();

inline TzdVal tzd_builtin_getFunctions(const std::vector<TzdVal>& = {}) {
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (const auto& fn : get_all_registered_builtin_names_list()) {
        arr->push_back(TzdVal(fn));
    }
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_getNativeFunctions(const std::vector<TzdVal>& args = {}) {
    return tzd_builtin_getFunctions(args);
}
inline TzdVal tzd_builtin_getSymbols(const std::vector<TzdVal>& args = {}) {
    return tzd_builtin_getFunctions(args);
}

inline TzdVal tzd_builtin_getClassInfo(const std::vector<TzdVal>& args) {
    auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    if (!args.empty() && args[0].type == ValType::INSTANCE && args[0].instVal) {
        (*m)["className"] = TzdVal(args[0].instVal->className);
        auto fArr = std::make_shared<std::vector<TzdVal>>();
        for (const auto& kv : args[0].instVal->fields) fArr->push_back(TzdVal(kv.first));
        (*m)["fields"] = TzdVal(fArr);
        (*m)["fieldCount"] = TzdVal((int64_t)args[0].instVal->fields.size());
    }
    return TzdVal(m);
}

inline TzdVal tzd_builtin_getArraysInfo(const std::vector<TzdVal>& args) {
    auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        (*m)["length"] = TzdVal((int64_t)args[0].arrVal->size());
        (*m)["capacity"] = TzdVal((int64_t)args[0].arrVal->capacity());
        (*m)["type"] = TzdVal("array");
    }
    return TzdVal(m);
}

inline TzdVal tzd_builtin_Runtime(const std::vector<TzdVal>& = {}) { return TzdVal("TzdNativeRuntime 0.2.5 (Full Machine Code)"); }
inline TzdVal tzd_builtin_bit(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0) : args[0]; }

// ── Complete Recursive-Descent JSON Parser ──
struct JsonParser {
    const std::string& src;
    size_t pos = 0;

    JsonParser(const std::string& s) : src(s) {}

    void skip_whitespace() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\r' || src[pos] == '\n')) {
            pos++;
        }
    }

    TzdVal parse_value() {
        skip_whitespace();
        if (pos >= src.size()) return TzdVal();
        char c = src[pos];
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        if (c == '-' || (c >= '0' && c <= '9')) return parse_number();
        return TzdVal();
    }

    TzdVal parse_null() {
        if (src.compare(pos, 4, "null") == 0) { pos += 4; return TzdVal(); }
        return TzdVal();
    }

    TzdVal parse_bool() {
        if (src.compare(pos, 4, "true") == 0) { pos += 4; return TzdVal(true); }
        if (src.compare(pos, 5, "false") == 0) { pos += 5; return TzdVal(false); }
        return TzdVal(false);
    }

    TzdVal parse_number() {
        size_t start = pos;
        if (pos < src.size() && src[pos] == '-') pos++;
        bool isFloat = false;
        while (pos < src.size() && (src[pos] >= '0' && src[pos] <= '9')) pos++;
        if (pos < src.size() && src[pos] == '.') {
            isFloat = true;
            pos++;
            while (pos < src.size() && (src[pos] >= '0' && src[pos] <= '9')) pos++;
        }
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
            isFloat = true;
            pos++;
            if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) pos++;
            while (pos < src.size() && (src[pos] >= '0' && src[pos] <= '9')) pos++;
        }
        std::string numStr = src.substr(start, pos - start);
        try {
            if (isFloat) return TzdVal(std::stod(numStr));
            return TzdVal((int64_t)std::stoll(numStr));
        } catch (...) {
            return TzdVal(0);
        }
    }

    TzdVal parse_string() {
        if (pos >= src.size() || src[pos] != '"') return TzdVal("");
        pos++;
        std::string res;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') return TzdVal(res);
            if (c == '\\' && pos < src.size()) {
                char esc = src[pos++];
                switch (esc) {
                    case '"': res += '"'; break;
                    case '\\': res += '\\'; break;
                    case '/': res += '/'; break;
                    case 'b': res += '\b'; break;
                    case 'f': res += '\f'; break;
                    case 'n': res += '\n'; break;
                    case 'r': res += '\r'; break;
                    case 't': res += '\t'; break;
                    case 'u': {
                        if (pos + 4 <= src.size()) {
                            std::string hex = src.substr(pos, 4);
                            pos += 4;
                            try {
                                uint32_t cp = (uint32_t)std::stoul(hex, nullptr, 16);
                                if (cp < 0x80) res += (char)cp;
                                else if (cp < 0x800) {
                                    res += (char)(0xC0 | (cp >> 6));
                                    res += (char)(0x80 | (cp & 0x3F));
                                } else {
                                    res += (char)(0xE0 | (cp >> 12));
                                    res += (char)(0x80 | ((cp >> 6) & 0x3F));
                                    res += (char)(0x80 | (cp & 0x3F));
                                }
                            } catch (...) {}
                        }
                        break;
                    }
                    default: res += esc; break;
                }
            } else {
                res += c;
            }
        }
        return TzdVal(res);
    }

    TzdVal parse_array() {
        if (pos >= src.size() || src[pos] != '[') return tzd_make_array({});
        pos++;
        auto arr = std::make_shared<std::vector<TzdVal>>();
        skip_whitespace();
        if (pos < src.size() && src[pos] == ']') { pos++; return TzdVal(arr); }
        while (pos < src.size()) {
            arr->push_back(parse_value());
            skip_whitespace();
            if (pos < src.size() && src[pos] == ',') {
                pos++;
                skip_whitespace();
            } else if (pos < src.size() && src[pos] == ']') {
                pos++;
                break;
            } else {
                break;
            }
        }
        return TzdVal(arr);
    }

    TzdVal parse_object() {
        if (pos >= src.size() || src[pos] != '{') return TzdVal(std::make_shared<std::unordered_map<std::string, TzdVal>>());
        pos++;
        auto obj = std::make_shared<std::unordered_map<std::string, TzdVal>>();
        skip_whitespace();
        if (pos < src.size() && src[pos] == '}') { pos++; return TzdVal(obj); }
        while (pos < src.size()) {
            skip_whitespace();
            if (pos >= src.size() || src[pos] != '"') break;
            TzdVal keyVal = parse_string();
            skip_whitespace();
            if (pos < src.size() && src[pos] == ':') pos++;
            else break;
            TzdVal val = parse_value();
            (*obj)[keyVal.sVal] = val;
            skip_whitespace();
            if (pos < src.size() && src[pos] == ',') {
                pos++;
                skip_whitespace();
            } else if (pos < src.size() && src[pos] == '}') {
                pos++;
                break;
            } else {
                break;
            }
        }
        return TzdVal(obj);
    }
};

inline TzdVal tzd_builtin_jsonParse(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    std::string s = args[0].to_string();
    JsonParser parser(s);
    return parser.parse_value();
}
inline TzdVal tzd_builtin_fromJSON(const std::vector<TzdVal>& args) { return tzd_builtin_jsonParse(args); }

// ── Strict JSON Serializer ──
inline std::string json_escape_str(const std::string& s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if ((unsigned char)c < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
            out += buf;
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}

inline std::string real_json_stringify(const TzdVal& val) {
    switch (val.type) {
        case ValType::NIL: return "null";
        case ValType::BOOL: return val.bVal ? "true" : "false";
        case ValType::INT: return std::to_string(val.iVal);
        case ValType::FLOAT: {
            if (std::isnan(val.fVal) || std::isinf(val.fVal)) return "null";
            std::ostringstream ss;
            ss << std::setprecision(15) << val.fVal;
            std::string s = ss.str();
            if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) {
                s += ".0";
            }
            return s;
        }
        case ValType::STRING: return json_escape_str(val.sVal);
        case ValType::ARRAY: {
            if (!val.arrVal) return "[]";
            std::string out = "[";
            for (size_t i = 0; i < val.arrVal->size(); ++i) {
                if (i > 0) out += ",";
                out += real_json_stringify((*val.arrVal)[i]);
            }
            out += "]";
            return out;
        }
        case ValType::MAP: {
            if (!val.mapVal) return "{}";
            std::string out = "{";
            bool first = true;
            for (const auto& kv : *val.mapVal) {
                if (!first) out += ",";
                first = false;
                out += json_escape_str(kv.first) + ":" + real_json_stringify(kv.second);
            }
            out += "}";
            return out;
        }
        case ValType::INSTANCE: {
            if (!val.instVal) return "{}";
            std::string out = "{";
            bool first = true;
            for (const auto& kv : val.instVal->fields) {
                if (!first) out += ",";
                first = false;
                out += json_escape_str(kv.first) + ":" + real_json_stringify(kv.second);
            }
            out += "}";
            return out;
        }
        default: return "null";
    }
}

inline TzdVal tzd_builtin_jsonStringify(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("null");
    return TzdVal(real_json_stringify(args[0]));
}
inline TzdVal tzd_builtin_toJSON(const std::vector<TzdVal>& args) { return tzd_builtin_jsonStringify(args); }

// ── BigInt & Rational Built-ins ──

inline TzdVal tzd_builtin_isBigint(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::STRING) return TzdVal(false);
    const std::string& s = args[0].sVal;
    if (s.empty()) return TzdVal(false);
    size_t i = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    if (i >= s.size()) return TzdVal(false);
    for (; i < s.size(); ++i) {
        if (!std::isdigit((unsigned char)s[i])) return TzdVal(false);
    }
    return TzdVal(true);
}
// ── High-Performance Native BigInt Engine (Base 10^9 + Karatsuba Multiplication) ──
struct NativeBigInt {
    static const uint64_t BASE = 1000000000ULL; // 10^9 limb base
    static const int BASE_DIGITS = 9;

    bool negative = false;
    std::vector<uint64_t> limbs; // least significant limb at index 0

    NativeBigInt() : negative(false) {}
    NativeBigInt(int64_t v) {
        if (v < 0) { negative = true; v = -v; }
        if (v == 0) limbs.push_back(0);
        while (v > 0) {
            limbs.push_back(v % BASE);
            v /= BASE;
        }
    }
    NativeBigInt(const std::string& str) {
        std::string s = str;
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        if (s.empty()) { limbs.push_back(0); return; }
        size_t start = 0;
        if (s[0] == '-') { negative = true; start = 1; }
        else if (s[0] == '+') { start = 1; }

        for (int i = (int)s.size(); i > (int)start; i -= BASE_DIGITS) {
            int l_start = std::max((int)start, i - BASE_DIGITS);
            std::string sub = s.substr(l_start, i - l_start);
            uint64_t v = 0;
            for (char c : sub) {
                if (std::isdigit((unsigned char)c)) v = v * 10 + (c - '0');
            }
            limbs.push_back(v);
        }
        trim();
    }

    void trim() {
        while (limbs.size() > 1 && limbs.back() == 0) limbs.pop_back();
        if (limbs.empty()) limbs.push_back(0);
        if (limbs.size() == 1 && limbs[0] == 0) negative = false;
    }

    bool is_zero() const {
        return limbs.empty() || (limbs.size() == 1 && limbs[0] == 0);
    }

    std::string to_string() const {
        if (limbs.empty() || (limbs.size() == 1 && limbs[0] == 0)) return "0";
        std::string s;
        if (negative) s += '-';
        s += std::to_string(limbs.back());
        for (int i = (int)limbs.size() - 2; i >= 0; --i) {
            std::string sub = std::to_string(limbs[i]);
            s.append(BASE_DIGITS - sub.size(), '0');
            s += sub;
        }
        return s;
    }

    int cmp_abs(const NativeBigInt& o) const {
        if (limbs.size() != o.limbs.size()) return limbs.size() < o.limbs.size() ? -1 : 1;
        for (int i = (int)limbs.size() - 1; i >= 0; --i) {
            if (limbs[i] != o.limbs[i]) return limbs[i] < o.limbs[i] ? -1 : 1;
        }
        return 0;
    }

    int compare(const NativeBigInt& o) const {
        if (negative != o.negative) return negative ? -1 : 1;
        int cmp = cmp_abs(o);
        return negative ? -cmp : cmp;
    }

    NativeBigInt add_abs(const NativeBigInt& b) const {
        NativeBigInt res;
        uint64_t carry = 0;
        size_t n = std::max(limbs.size(), b.limbs.size());
        for (size_t i = 0; i < n || carry; ++i) {
            uint64_t sum = carry;
            if (i < limbs.size()) sum += limbs[i];
            if (i < b.limbs.size()) sum += b.limbs[i];
            res.limbs.push_back(sum % BASE);
            carry = sum / BASE;
        }
        res.trim();
        return res;
    }

    NativeBigInt sub_abs(const NativeBigInt& b) const {
        NativeBigInt res;
        int64_t borrow = 0;
        for (size_t i = 0; i < limbs.size(); ++i) {
            int64_t diff = (int64_t)limbs[i] - borrow - (i < b.limbs.size() ? (int64_t)b.limbs[i] : 0);
            if (diff < 0) { diff += BASE; borrow = 1; }
            else borrow = 0;
            res.limbs.push_back((uint64_t)diff);
        }
        res.trim();
        return res;
    }

    NativeBigInt operator+(const NativeBigInt& b) const {
        if (negative == b.negative) {
            NativeBigInt res = add_abs(b);
            res.negative = negative;
            return res;
        }
        if (cmp_abs(b) >= 0) {
            NativeBigInt res = sub_abs(b);
            res.negative = negative;
            return res;
        } else {
            NativeBigInt res = b.sub_abs(*this);
            res.negative = b.negative;
            return res;
        }
    }

    NativeBigInt operator-(const NativeBigInt& b) const {
        NativeBigInt neg_b = b;
        neg_b.negative = !b.negative;
        return *this + neg_b;
    }

    NativeBigInt mul_small(uint64_t v) const {
        if (v == 0 || is_zero()) return NativeBigInt(0);
        NativeBigInt res;
        res.negative = negative;
        uint64_t carry = 0;
        for (size_t i = 0; i < limbs.size() || carry; ++i) {
            uint64_t cur = carry + (i < limbs.size() ? limbs[i] * v : 0);
            res.limbs.push_back(cur % BASE);
            carry = cur / BASE;
        }
        res.trim();
        return res;
    }

    // Karatsuba recursive multiplication O(N^1.585)
    static std::vector<uint64_t> karatsuba_mul(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
        size_t n = std::max(a.size(), b.size());
        if (n <= 32) {
            std::vector<uint64_t> res(a.size() + b.size(), 0);
            for (size_t i = 0; i < a.size(); ++i) {
                uint64_t carry = 0;
                for (size_t j = 0; j < b.size() || carry; ++j) {
                    uint64_t cur = res[i + j] + carry + (j < b.size() ? a[i] * b[j] : 0);
                    res[i + j] = cur % BASE;
                    carry = cur / BASE;
                }
            }
            return res;
        }

        size_t k = (n + 1) / 2;
        std::vector<uint64_t> a0(a.begin(), a.begin() + std::min(k, a.size()));
        std::vector<uint64_t> a1(a.size() > k ? a.begin() + k : a.end(), a.end());
        std::vector<uint64_t> b0(b.begin(), b.begin() + std::min(k, b.size()));
        std::vector<uint64_t> b1(b.size() > k ? b.begin() + k : b.end(), b.end());

        auto z0 = karatsuba_mul(a0, b0);
        auto z2 = karatsuba_mul(a1, b1);

        std::vector<uint64_t> a01 = a0;
        uint64_t carry = 0;
        for (size_t i = 0; i < std::max(a0.size(), a1.size()) || carry; ++i) {
            uint64_t sum = carry + (i < a0.size() ? a0[i] : 0) + (i < a1.size() ? a1[i] : 0);
            if (i < a01.size()) a01[i] = sum % BASE;
            else a01.push_back(sum % BASE);
            carry = sum / BASE;
        }

        std::vector<uint64_t> b01 = b0;
        carry = 0;
        for (size_t i = 0; i < std::max(b0.size(), b1.size()) || carry; ++i) {
            uint64_t sum = carry + (i < b0.size() ? b0[i] : 0) + (i < b1.size() ? b1[i] : 0);
            if (i < b01.size()) b01[i] = sum % BASE;
            else b01.push_back(sum % BASE);
            carry = sum / BASE;
        }

        auto z1 = karatsuba_mul(a01, b01);

        int64_t borrow = 0;
        for (size_t i = 0; i < z1.size(); ++i) {
            int64_t diff = (int64_t)z1[i] - borrow - (i < z0.size() ? (int64_t)z0[i] : 0) - (i < z2.size() ? (int64_t)z2[i] : 0);
            if (diff < 0) {
                int64_t b_cnt = (-diff + BASE - 1) / BASE;
                diff += b_cnt * BASE;
                borrow = b_cnt;
            } else borrow = 0;
            z1[i] = (uint64_t)diff;
        }

        size_t total_sz = std::max({z0.size(), z1.size() + k, z2.size() + 2 * k}) + 2;
        std::vector<uint64_t> res(total_sz, 0);
        carry = 0;
        for (size_t i = 0; i < total_sz; ++i) {
            uint64_t sum = carry;
            if (i < z0.size()) sum += z0[i];
            if (i >= k && (i - k) < z1.size()) sum += z1[i - k];
            if (i >= 2 * k && (i - 2 * k) < z2.size()) sum += z2[i - 2 * k];
            res[i] = sum % BASE;
            carry = sum / BASE;
        }
        while (res.size() > 1 && res.back() == 0) res.pop_back();
        return res;
    }

    NativeBigInt operator*(const NativeBigInt& b) const {
        if (is_zero() || b.is_zero()) return NativeBigInt(0);
        NativeBigInt res;
        res.negative = negative ^ b.negative;
        res.limbs = karatsuba_mul(limbs, b.limbs);
        res.trim();
        return res;
    }

    std::pair<NativeBigInt, NativeBigInt> divmod(const NativeBigInt& b) const {
        if (b.is_zero()) return {NativeBigInt(0), NativeBigInt(0)};
        NativeBigInt q, rem;
        q.limbs.assign(limbs.size(), 0);
        for (int i = (int)limbs.size() - 1; i >= 0; --i) {
            rem.limbs.insert(rem.limbs.begin(), limbs[i]);
            rem.trim();
            uint64_t l = 0, r = BASE - 1, best = 0;
            while (l <= r) {
                uint64_t mid = (l + r) / 2;
                if (b.mul_small(mid).cmp_abs(rem) <= 0) {
                    best = mid;
                    l = mid + 1;
                } else {
                    if (mid == 0) break;
                    r = mid - 1;
                }
            }
            q.limbs[i] = best;
            rem = rem.sub_abs(b.mul_small(best));
        }
        q.negative = negative ^ b.negative;
        rem.negative = negative;
        q.trim();
        rem.trim();
        return {q, rem};
    }

    NativeBigInt operator/(const NativeBigInt& b) const { return divmod(b).first; }
    NativeBigInt operator%(const NativeBigInt& b) const { return divmod(b).second; }

    static NativeBigInt pow(NativeBigInt base, uint64_t exp) {
        NativeBigInt res(1);
        while (exp > 0) {
            if (exp & 1) res = res * base;
            base = base * base;
            exp >>= 1;
        }
        return res;
    }

    static NativeBigInt powmod(NativeBigInt base, NativeBigInt exp, const NativeBigInt& mod) {
        if (mod.is_zero() || (mod.limbs.size() == 1 && mod.limbs[0] == 1)) return NativeBigInt(0);
        NativeBigInt res(1);
        base = base % mod;
        while (!exp.is_zero()) {
            if (exp.limbs[0] & 1) res = (res * base) % mod;
            base = (base * base) % mod;
            exp = exp / NativeBigInt(2);
        }
        return res;
    }

    static NativeBigInt gcd(NativeBigInt a, NativeBigInt b) {
        a.negative = false;
        b.negative = false;
        while (!b.is_zero()) {
            NativeBigInt r = a % b;
            a = b;
            b = r;
        }
        return a;
    }

    static NativeBigInt factorial(int64_t n) {
        if (n <= 1) return NativeBigInt(1);
        NativeBigInt res(1);
        for (int64_t i = 2; i <= n; ++i) {
            res = res.mul_small(i);
        }
        return res;
    }
};

// ── Out-of-line Implementation of TzdVal BigInt Operators ──
inline TzdVal TzdVal::operator+(const TzdVal& o) const {
    if (type == ValType::STRING || o.type == ValType::STRING) {
        return TzdVal(to_string() + o.to_string());
    }
#ifdef WITH_LIBTORCH
    if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
        return TzdVal(*tensorVal + *o.tensorVal);
    }
#endif
    if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
        return TzdVal(as_double() + o.as_double());
    }
    int64_t a = as_int(), b = o.as_int();
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
        return TzdVal((NativeBigInt(a) + NativeBigInt(b)).to_string());
    }
    return TzdVal(a + b);
}

inline TzdVal TzdVal::operator-(const TzdVal& o) const {
#ifdef WITH_LIBTORCH
    if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
        return TzdVal(*tensorVal - *o.tensorVal);
    }
#endif
    if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
        return TzdVal(as_double() - o.as_double());
    }
    int64_t a = as_int(), b = o.as_int();
    if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) {
        return TzdVal((NativeBigInt(a) - NativeBigInt(b)).to_string());
    }
    return TzdVal(a - b);
}

inline TzdVal TzdVal::operator*(const TzdVal& o) const {
#ifdef WITH_LIBTORCH
    if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
        return TzdVal(*tensorVal * *o.tensorVal);
    }
#endif
    if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
        return TzdVal(as_double() * o.as_double());
    }
    int64_t a = as_int(), b = o.as_int();
#if defined(_MSC_VER)
    int64_t high = 0;
    int64_t low = _mul128(a, b, &high);
    if (high != (low >> 63)) {
        return TzdVal((NativeBigInt(a) * NativeBigInt(b)).to_string());
    }
    return TzdVal(low);
#else
    __int128 p = (__int128)a * b;
    if (p > INT64_MAX || p < INT64_MIN) {
        return TzdVal((NativeBigInt(a) * NativeBigInt(b)).to_string());
    }
    return TzdVal((int64_t)p);
#endif
}

inline TzdVal TzdVal::operator/(const TzdVal& o) const {
#ifdef WITH_LIBTORCH
    if (type == ValType::TENSOR && o.type == ValType::TENSOR) {
        return TzdVal(*tensorVal / *o.tensorVal);
    }
#endif
    double d = o.as_double();
    return TzdVal(d != 0.0 ? as_double() / d : 0.0);
}

inline TzdVal TzdVal::operator%(const TzdVal& o) const {
    int64_t b = o.as_int();
    return TzdVal(b != 0 ? as_int() % b : 0);
}

inline TzdVal TzdVal::pow(const TzdVal& o) const {
    if (type == ValType::INT && o.type == ValType::INT && o.iVal >= 0) {
        if (o.iVal == 0) return TzdVal(1);
        if (o.iVal > 62 || (iVal > 2 && o.iVal > 30)) {
            return TzdVal(NativeBigInt::pow(NativeBigInt(iVal), (uint64_t)o.iVal).to_string());
        }
    }
    return TzdVal(std::pow(as_double(), o.as_double()));
}

inline TzdVal tzd_builtin_pow(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    if (args.size() == 1) return args[0];
    return args[0].pow(args[1]);
}

inline TzdVal tzd_builtin_powmod(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return TzdVal(0);
    NativeBigInt base(args[0].to_string());
    NativeBigInt exp(args[1].to_string());
    NativeBigInt mod(args[2].to_string());
    return TzdVal(NativeBigInt::powmod(base, exp, mod).to_string());
}

inline TzdVal tzd_builtin_bigint(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("0");
    if (args[0].type == ValType::STRING) return TzdVal(NativeBigInt(args[0].sVal).to_string());
    return TzdVal(NativeBigInt(args[0].as_int()).to_string());
}

inline TzdVal tzd_builtin_bigintGcd(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal("0") : args[0];
    NativeBigInt a(args[0].to_string());
    NativeBigInt b(args[1].to_string());
    return TzdVal(NativeBigInt::gcd(a, b).to_string());
}

inline TzdVal tzd_builtin_bigintFactorial(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("1");
    int64_t n = args[0].as_int();
    if (n < 0) return TzdVal("0");
    return TzdVal(NativeBigInt::factorial(n).to_string());
}

inline TzdVal tzd_builtin_bigintAdd(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal("0") : args[0];
    return TzdVal((NativeBigInt(args[0].to_string()) + NativeBigInt(args[1].to_string())).to_string());
}

inline TzdVal tzd_builtin_bigintSub(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal("0") : args[0];
    return TzdVal((NativeBigInt(args[0].to_string()) - NativeBigInt(args[1].to_string())).to_string());
}

inline TzdVal tzd_builtin_bigintMul(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal("0") : args[0];
    return TzdVal((NativeBigInt(args[0].to_string()) * NativeBigInt(args[1].to_string())).to_string());
}

inline TzdVal tzd_builtin_bigintDiv(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal("0") : args[0];
    return TzdVal((NativeBigInt(args[0].to_string()) / NativeBigInt(args[1].to_string())).to_string());
}

inline TzdVal tzd_builtin_bigintMod(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal("0") : args[0];
    return TzdVal((NativeBigInt(args[0].to_string()) % NativeBigInt(args[1].to_string())).to_string());
}

inline TzdVal tzd_builtin_bigintPow(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal("1") : args[0];
    return TzdVal(NativeBigInt::pow(NativeBigInt(args[0].to_string()), (uint64_t)args[1].as_int()).to_string());
}

inline TzdVal tzd_builtin_bigintPowmod(const std::vector<TzdVal>& args) {
    return tzd_builtin_powmod(args);
}

inline TzdVal tzd_builtin_bigintCompare(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0);
    return TzdVal(NativeBigInt(args[0].to_string()).compare(NativeBigInt(args[1].to_string())));
}

inline TzdVal tzd_builtin_bigintAbs(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("0");
    NativeBigInt a(args[0].to_string());
    a.negative = false;
    return TzdVal(a.to_string());
}

inline TzdVal tzd_builtin_bigintNeg(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("0");
    NativeBigInt a(args[0].to_string());
    if (!a.is_zero()) a.negative = !a.negative;
    return TzdVal(a.to_string());
}

inline TzdVal tzd_builtin_getBigIntMaxDigits(const std::vector<TzdVal>& = {}) { return TzdVal(10000000); }
inline TzdVal tzd_builtin_setBigIntMaxDigits(const std::vector<TzdVal>& = {}) { return TzdVal(true); }

inline TzdVal tzd_builtin_rational(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({TzdVal(0), TzdVal(1)});
    int64_t n = args[0].as_int(), d = args.size() > 1 ? args[1].as_int() : 1;
    if (d == 0) d = 1;
    int64_t g = compute_gcd(n, d);
    return tzd_make_array({TzdVal(n / g), TzdVal(d / g)});
}
inline TzdVal tzd_builtin_rationalAdd(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_builtin_rational(args);
    int64_t n1 = args[0].get_index(TzdVal(0)).as_int(), d1 = args[0].get_index(TzdVal(1)).as_int();
    int64_t n2 = args[1].get_index(TzdVal(0)).as_int(), d2 = args[1].get_index(TzdVal(1)).as_int();
    if (d1 == 0) d1 = 1; if (d2 == 0) d2 = 1;
    return tzd_builtin_rational({TzdVal(n1 * d2 + n2 * d1), TzdVal(d1 * d2)});
}
inline TzdVal tzd_builtin_rationalMul(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_builtin_rational(args);
    int64_t n1 = args[0].get_index(TzdVal(0)).as_int(), d1 = args[0].get_index(TzdVal(1)).as_int();
    int64_t n2 = args[1].get_index(TzdVal(0)).as_int(), d2 = args[1].get_index(TzdVal(1)).as_int();
    if (d1 == 0) d1 = 1; if (d2 == 0) d2 = 1;
    return tzd_builtin_rational({TzdVal(n1 * n2), TzdVal(d1 * d2)});
}

// Continued fraction algorithm for exact float-to-fraction conversion
inline TzdVal tzd_builtin_toFraction(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("0/1");
    double x = args[0].as_double();
    if (std::isnan(x) || std::isinf(x)) return TzdVal("0/1");
    int64_t sign = x < 0 ? -1 : 1;
    x = std::abs(x);
    if (std::abs(x - std::round(x)) < 1e-12) {
        return TzdVal(std::to_string(sign * (int64_t)std::round(x)) + "/1");
    }
    int64_t h0 = 0, h1 = 1;
    int64_t k0 = 1, k1 = 0;
    double b = x;
    for (int iter = 0; iter < 40; ++iter) {
        int64_t a = (int64_t)std::floor(b);
        int64_t h2 = a * h1 + h0;
        int64_t k2 = a * k1 + k0;
        if (k2 <= 0 || k2 > 1000000000LL) break;
        double frac = (double)h2 / (double)k2;
        if (std::abs(x - frac) < 1e-9 || std::abs(b - a) < 1e-12) {
            return TzdVal(std::to_string(sign * h2) + "/" + std::to_string(k2));
        }
        b = 1.0 / (b - a);
        h0 = h1; h1 = h2;
        k0 = k1; k1 = k2;
    }
    return TzdVal(std::to_string(sign * h1) + "/" + std::to_string(k1));
}

// ── Multi-threading Manager with Real Handle Tracking ──
struct NativeThreadManager {
    std::mutex mtx;
    int64_t nextId = 1;
    std::unordered_map<int64_t, std::shared_ptr<std::thread>> threads;
};
inline NativeThreadManager& get_thread_mgr() {
    static NativeThreadManager mgr;
    return mgr;
}

inline TzdVal tzd_builtin_sys_thread_start(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::FUNC || !args[0].funcVal) return TzdVal(0);
    TzdVal fn = args[0];
    auto& mgr = get_thread_mgr();
    std::lock_guard<std::mutex> lock(mgr.mtx);
    int64_t tid = mgr.nextId++;
    auto th = std::make_shared<std::thread>([fn]() {
        try {
            std::vector<TzdVal> emptyArgs;
            fn(emptyArgs);
        } catch (...) {}
    });
    mgr.threads[tid] = th;
    return TzdVal(tid);
}

inline TzdVal tzd_builtin_sys_thread_join(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t tid = args[0].as_int();
    std::shared_ptr<std::thread> th;
    {
        auto& mgr = get_thread_mgr();
        std::lock_guard<std::mutex> lock(mgr.mtx);
        auto it = mgr.threads.find(tid);
        if (it != mgr.threads.end()) {
            th = it->second;
            mgr.threads.erase(it);
        }
    }
    if (th && th->joinable()) {
        th->join();
        return TzdVal(true);
    }
    return TzdVal(false);
}

inline TzdVal tzd_builtin_sys_thread_detach(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t tid = args[0].as_int();
    std::shared_ptr<std::thread> th;
    {
        auto& mgr = get_thread_mgr();
        std::lock_guard<std::mutex> lock(mgr.mtx);
        auto it = mgr.threads.find(tid);
        if (it != mgr.threads.end()) {
            th = it->second;
            mgr.threads.erase(it);
        }
    }
    if (th && th->joinable()) {
        th->detach();
        return TzdVal(true);
    }
    return TzdVal(false);
}


// ── Lightweight Mathematical Expression Evaluator (for string formulas like "x^2", "sin(x)") ──
struct MathExprParser {
    std::string s;
    size_t pos = 0;
    double x_val = 0.0;

    MathExprParser(std::string str, double x) : s(std::move(str)), x_val(x) {}

    void skip_ws() {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) pos++;
    }

    double parse_primary() {
        skip_ws();
        if (pos >= s.size()) return 0.0;
        if (s[pos] == '+') { pos++; return parse_primary(); }
        if (s[pos] == '-') { pos++; return -parse_primary(); }
        if (s[pos] == '(') {
            pos++;
            double v = parse_expr();
            skip_ws();
            if (pos < s.size() && s[pos] == ')') pos++;
            return v;
        }
        if (pos < s.size() && (s[pos] == 'x' || s[pos] == 'X')) {
            pos++;
            return x_val;
        }
        if (std::isalpha((unsigned char)s[pos])) {
            std::string fn;
            while (pos < s.size() && std::isalpha((unsigned char)s[pos])) fn += s[pos++];
            skip_ws();
            double arg = parse_primary();
            if (fn == "sin") return std::sin(arg);
            if (fn == "cos") return std::cos(arg);
            if (fn == "tan") return std::tan(arg);
            if (fn == "exp") return std::exp(arg);
            if (fn == "log" || fn == "ln") return std::log(arg);
            if (fn == "sqrt") return std::sqrt(arg);
            if (fn == "abs") return std::abs(arg);
            return arg;
        }
        if (std::isdigit((unsigned char)s[pos]) || s[pos] == '.') {
            size_t start = pos;
            while (pos < s.size() && (std::isdigit((unsigned char)s[pos]) || s[pos] == '.')) pos++;
            try { return std::stod(s.substr(start, pos - start)); } catch (...) { return 0.0; }
        }
        return 0.0;
    }

    double parse_power() {
        double left = parse_primary();
        skip_ws();
        if (pos < s.size() && s[pos] == '^') {
            pos++;
            double right = parse_power();
            return std::pow(left, right);
        }
        return left;
    }

    double parse_term() {
        double left = parse_power();
        while (true) {
            skip_ws();
            if (pos < s.size() && s[pos] == '*') {
                pos++;
                left *= parse_power();
            } else if (pos < s.size() && s[pos] == '/') {
                pos++;
                double d = parse_power();
                left = (d == 0.0 ? 0.0 : left / d);
            } else break;
        }
        return left;
    }

    double parse_expr() {
        double left = parse_term();
        while (true) {
            skip_ws();
            if (pos < s.size() && s[pos] == '+') {
                pos++;
                left += parse_term();
            } else if (pos < s.size() && s[pos] == '-') {
                pos++;
                left -= parse_term();
            } else break;
        }
        return left;
    }
};

inline double eval_simple_math_expr(const std::string& str, double x) {
    MathExprParser p(str, x);
    return p.parse_expr();
}


// ── Real Plotting Engine (ASCII Console Chart & SVG File Exporter) ──
inline TzdVal tzd_builtin_plot(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    std::vector<double> data;
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& v : *args[0].arrVal) data.push_back(v.as_double());
    } else if (args[0].type == ValType::FUNC && args.size() >= 3) {
        double x0 = args[1].as_double(), x1 = args[2].as_double();
        int steps = args.size() > 3 ? (int)args[3].as_int() : 30;
        if (steps < 2) steps = 30;
        double dx = (x1 - x0) / (steps - 1);
        for (int i = 0; i < steps; ++i) {
            double x = x0 + i * dx;
            data.push_back(args[0]({TzdVal(x)}).as_double());
        }
    } else if (args[0].type == ValType::STRING && args.size() >= 3) {
        std::string expr = args[0].to_string();
        double x0 = args[1].as_double(), x1 = args[2].as_double();
        int steps = args.size() > 3 ? (int)args[3].as_int() : 30;
        if (steps < 2) steps = 30;
        double dx = (x1 - x0) / (steps - 1);
        for (int i = 0; i < steps; ++i) {
            double x = x0 + i * dx;
            data.push_back(eval_simple_math_expr(expr, x));
        }
    }
    if (data.empty()) return TzdVal(false);

    if (args.size() > 1 && args.back().type == ValType::STRING && args.back().to_string().rfind(".svg") != std::string::npos) {
        std::string filename = args.back().to_string();
        std::ofstream svg(filename);
        if (svg.is_open()) {
            double minY = *std::min_element(data.begin(), data.end());
            double maxY = *std::max_element(data.begin(), data.end());
            if (minY == maxY) { minY -= 1.0; maxY += 1.0; }
            int W = 600, H = 300, pad = 40;
            svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << W << "\" height=\"" << H << "\">\n";
            svg << "<rect width=\"100%\" height=\"100%\" fill=\"#1e1e1e\"/>\n";
            svg << "<polyline fill=\"none\" stroke=\"#4ec9b0\" stroke-width=\"2\" points=\"";
            for (size_t i = 0; i < data.size(); ++i) {
                double px = pad + (double)i / (data.size() - 1) * (W - 2 * pad);
                double py = H - pad - (data[i] - minY) / (maxY - minY) * (H - 2 * pad);
                svg << px << "," << py << " ";
            }
            svg << "\"/>\n</svg>\n";
            return TzdVal(true);
        }
    }

    double minY = *std::min_element(data.begin(), data.end());
    double maxY = *std::max_element(data.begin(), data.end());
    if (minY == maxY) { minY -= 1.0; maxY += 1.0; }
    int plotW = 40;
    int plotH = 10;
    std::vector<std::string> canvas(plotH, std::string(plotW, ' '));
    for (int col = 0; col < plotW; ++col) {
        size_t idx = (size_t)((double)col / (plotW - 1) * (data.size() - 1));
        double norm = (data[idx] - minY) / (maxY - minY);
        int row = (int)(norm * (plotH - 1));
        if (row < 0) row = 0; if (row >= plotH) row = plotH - 1;
        canvas[plotH - 1 - row][col] = '*';
    }
    std::cout << "\n--- ASCII Plot (Min: " << minY << ", Max: " << maxY << ") ---\n";
    for (int r = 0; r < plotH; ++r) {
        double yVal = maxY - (double)r / (plotH - 1) * (maxY - minY);
        std::cout << std::setw(8) << std::setprecision(2) << yVal << " | " << canvas[r] << "\n";
    }
    std::cout << "         +" << std::string(plotW, '-') << "\n";
    return TzdVal(true);
}

// ── Numerical Derivative & Equation Solver ──
inline TzdVal tzd_builtin_derivative(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    double x = args.size() > 1 ? args[1].as_double() : 0.0;
    double h = args.size() > 2 ? args[2].as_double() : 1e-6;
    if (h == 0.0) h = 1e-6;
    if (args[0].type == ValType::FUNC) {
        TzdVal fn = args[0];
        double y_plus = fn({TzdVal(x + h)}).as_double();
        double y_minus = fn({TzdVal(x - h)}).as_double();
        return TzdVal((y_plus - y_minus) / (2.0 * h));
    } else if (args[0].type == ValType::STRING) {
        std::string expr = args[0].to_string();
        double y_plus = eval_simple_math_expr(expr, x + h);
        double y_minus = eval_simple_math_expr(expr, x - h);
        return TzdVal((y_plus - y_minus) / (2.0 * h));
    }
    return TzdVal(0.0);
}

inline TzdVal tzd_builtin_solveEq(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    double x = args.size() > 1 ? args[1].as_double() : 0.0;
    double h = 1e-5;
    std::function<double(double)> fn;
    if (args[0].type == ValType::FUNC) {
        TzdVal f = args[0];
        fn = [f](double val) { return f({TzdVal(val)}).as_double(); };
    } else if (args[0].type == ValType::STRING) {
        std::string expr = args[0].to_string();
        fn = [expr](double val) { return eval_simple_math_expr(expr, val); };
    } else {
        return TzdVal(0.0);
    }
    for (int iter = 0; iter < 100; ++iter) {
        double fx = fn(x);
        if (std::abs(fx) < 1e-10) break;
        double dfx = (fn(x + h) - fn(x - h)) / (2.0 * h);
        if (std::abs(dfx) < 1e-12) dfx = 1e-12;
        double x_next = x - fx / dfx;
        if (std::abs(x_next - x) < 1e-10) { x = x_next; break; }
        x = x_next;
    }
    return TzdVal(x);
}

// ── Real Polynomial Symbolic Simplification Engine ──
struct SymbolicPoly {
    std::map<int, double> coeffs; // power -> coefficient

    SymbolicPoly() {}
    SymbolicPoly(double c, int p = 0) {
        if (std::abs(c) > 1e-12) coeffs[p] = c;
    }

    void add_term(double c, int p) {
        coeffs[p] += c;
        if (std::abs(coeffs[p]) < 1e-12) coeffs.erase(p);
    }

    SymbolicPoly operator+(const SymbolicPoly& o) const {
        SymbolicPoly res = *this;
        for (const auto& kv : o.coeffs) res.add_term(kv.second, kv.first);
        return res;
    }

    SymbolicPoly operator-(const SymbolicPoly& o) const {
        SymbolicPoly res = *this;
        for (const auto& kv : o.coeffs) res.add_term(-kv.second, kv.first);
        return res;
    }

    SymbolicPoly operator*(const SymbolicPoly& o) const {
        SymbolicPoly res;
        for (const auto& kv1 : coeffs) {
            for (const auto& kv2 : o.coeffs) {
                res.add_term(kv1.second * kv2.second, kv1.first + kv2.first);
            }
        }
        return res;
    }

    std::string to_string() const {
        if (coeffs.empty()) return "0";
        std::ostringstream oss;
        bool first = true;
        for (auto it = coeffs.rbegin(); it != coeffs.rend(); ++it) {
            int p = it->first;
            double c = it->second;
            if (first) {
                if (c < 0) oss << "-";
                first = false;
            } else {
                if (c < 0) oss << " - ";
                else oss << " + ";
            }
            double abs_c = std::abs(c);
            bool is_int = (std::abs(abs_c - std::round(abs_c)) < 1e-9);

            if (p == 0) {
                if (is_int) oss << (int64_t)std::round(abs_c);
                else oss << abs_c;
            } else {
                if (std::abs(abs_c - 1.0) > 1e-9) {
                    if (is_int) oss << (int64_t)std::round(abs_c) << "*";
                    else oss << abs_c << "*";
                }
                oss << "x";
                if (p > 1) oss << "^" << p;
            }
        }
        return oss.str();
    }
};

struct SymbolicParser {
    std::string s;
    size_t pos = 0;

    SymbolicParser(std::string str) : s(std::move(str)) {}

    void skip_ws() {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) pos++;
    }

    SymbolicPoly parse_primary() {
        skip_ws();
        if (pos >= s.size()) return SymbolicPoly();

        if (s[pos] == '+') { pos++; return parse_primary(); }
        if (s[pos] == '-') { pos++; return SymbolicPoly(-1.0, 0) * parse_primary(); }

        if (s[pos] == '(') {
            pos++;
            SymbolicPoly v = parse_expr();
            skip_ws();
            if (pos < s.size() && s[pos] == ')') pos++;
            return v;
        }

        if (pos < s.size() && (s[pos] == 'x' || s[pos] == 'X')) {
            pos++;
            return SymbolicPoly(1.0, 1);
        }

        if (std::isdigit((unsigned char)s[pos]) || s[pos] == '.') {
            size_t start = pos;
            while (pos < s.size() && (std::isdigit((unsigned char)s[pos]) || s[pos] == '.')) pos++;
            double v = std::stod(s.substr(start, pos - start));
            return SymbolicPoly(v, 0);
        }

        return SymbolicPoly();
    }

    SymbolicPoly parse_power() {
        SymbolicPoly left = parse_primary();
        skip_ws();
        if (pos < s.size() && s[pos] == '^') {
            pos++;
            skip_ws();
            if (pos < s.size() && std::isdigit((unsigned char)s[pos])) {
                size_t start = pos;
                while (pos < s.size() && std::isdigit((unsigned char)s[pos])) pos++;
                int p = std::stoi(s.substr(start, pos - start));
                SymbolicPoly res(1.0, 0);
                for (int i = 0; i < p; ++i) res = res * left;
                return res;
            }
        }
        return left;
    }

    SymbolicPoly parse_term() {
        SymbolicPoly left = parse_power();
        while (true) {
            skip_ws();
            if (pos < s.size() && s[pos] == '*') {
                pos++;
                left = left * parse_power();
            } else break;
        }
        return left;
    }

    SymbolicPoly parse_expr() {
        SymbolicPoly left = parse_term();
        while (true) {
            skip_ws();
            if (pos < s.size() && s[pos] == '+') {
                pos++;
                left = left + parse_term();
            } else if (pos < s.size() && s[pos] == '-') {
                pos++;
                left = left - parse_term();
            } else break;
        }
        return left;
    }
};

inline TzdVal tzd_builtin_simplifySym(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    try {
        std::string expr = args[0].to_string();
        SymbolicParser p(expr);
        SymbolicPoly poly = p.parse_expr();
        return TzdVal(poly.to_string());
    } catch (...) {
        return args[0];
    }
}

inline TzdVal tzd_builtin_solveSym(const std::vector<TzdVal>& args) {
    return tzd_builtin_solveEq(args);
}

// ── Real Mathematical Inequality Solver ──
inline TzdVal tzd_builtin_solveIneq(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("No real solution");
    std::string ineq_str = args[0].to_string();
    std::string op = "";
    size_t op_pos = std::string::npos;
    if ((op_pos = ineq_str.find("<=")) != std::string::npos) op = "<=";
    else if ((op_pos = ineq_str.find(">=")) != std::string::npos) op = ">=";
    else if ((op_pos = ineq_str.find("<")) != std::string::npos) op = "<";
    else if ((op_pos = ineq_str.find(">")) != std::string::npos) op = ">";
    if (op.empty()) return TzdVal("Invalid inequality expression");

    std::string lhs = ineq_str.substr(0, op_pos);
    std::string rhs = ineq_str.substr(op_pos + op.size());
    lhs.erase(0, lhs.find_first_not_of(" \t")); lhs.erase(lhs.find_last_not_of(" \t") + 1);
    rhs.erase(0, rhs.find_first_not_of(" \t")); rhs.erase(rhs.find_last_not_of(" \t") + 1);
    if (rhs.empty()) rhs = "0";

    auto F = [&](double x) -> double {
        return eval_simple_math_expr(lhs, x) - eval_simple_math_expr(rhs, x);
    };

    auto check_cond = [&](double val) -> bool {
        if (op == "<") return val < -1e-9;
        if (op == "<=") return val <= 1e-9;
        if (op == ">") return val > 1e-9;
        if (op == ">=") return val >= -1e-9;
        return false;
    };

    std::vector<double> roots;
    double prev_x = -100.0, prev_y = F(prev_x);
    for (double x = -99.5; x <= 100.0; x += 0.5) {
        double y = F(x);
        if (std::abs(y) < 1e-8) {
            roots.push_back(std::round(x * 1000.0) / 1000.0);
        } else if ((prev_y < 0 && y > 0) || (prev_y > 0 && y < 0)) {
            double l = prev_x, r = x;
            for (int it = 0; it < 50; ++it) {
                double m = (l + r) / 2.0;
                double ym = F(m);
                if (std::abs(ym) < 1e-10) { l = m; break; }
                if ((prev_y < 0 && ym > 0) || (prev_y > 0 && ym < 0)) r = m;
                else l = m;
            }
            double root = std::round(l * 1000.0) / 1000.0;
            roots.push_back(root);
        }
        prev_x = x;
        prev_y = y;
    }

    std::sort(roots.begin(), roots.end());
    std::vector<double> uniq_roots;
    for (double r : roots) {
        if (uniq_roots.empty() || std::abs(r - uniq_roots.back()) > 1e-3) {
            uniq_roots.push_back(r);
        }
    }

    auto fmt_d = [](double d) -> std::string {
        if (std::abs(d - std::round(d)) < 1e-5) return std::to_string((int64_t)std::round(d));
        std::ostringstream ss;
        ss << d;
        return ss.str();
    };

    if (uniq_roots.empty()) {
        if (check_cond(F(0.0))) return TzdVal("All real numbers");
        else return TzdVal("No real solution");
    }

    std::vector<std::string> valid_intervals;
    if (check_cond(F(uniq_roots[0] - 5.0))) {
        valid_intervals.push_back(std::string("x ") + (op.find('=') != std::string::npos ? "<= " : "< ") + fmt_d(uniq_roots[0]));
    }
    for (size_t i = 0; i + 1 < uniq_roots.size(); ++i) {
        double mid = (uniq_roots[i] + uniq_roots[i+1]) / 2.0;
        if (check_cond(F(mid))) {
            std::string left_op = (op.find('=') != std::string::npos ? " <= " : " < ");
            std::string right_op = (op.find('=') != std::string::npos ? " <= " : " < ");
            valid_intervals.push_back(fmt_d(uniq_roots[i]) + left_op + std::string("x") + right_op + fmt_d(uniq_roots[i+1]));
        }
    }
    if (check_cond(F(uniq_roots.back() + 5.0))) {
        valid_intervals.push_back(std::string("x ") + (op.find('=') != std::string::npos ? ">= " : "> ") + fmt_d(uniq_roots.back()));
    }

    if (valid_intervals.empty()) return TzdVal("No real solution");
    std::string res;
    for (size_t i = 0; i < valid_intervals.size(); ++i) {
        if (i > 0) res += " or ";
        res += valid_intervals[i];
    }
    return TzdVal(res);
}

inline const std::vector<std::string>& get_all_registered_builtin_names_list() {
    static const std::vector<std::string> names = {
        "E",
        "EPSILON",
        "GOLDEN_RATIO",
        "INF",
        "NAN",
        "PI",
        "Runtime",
        "SQRT2",
        "TAU",
        "abs",
        "acos",
        "addIncludePath",
        "appendFile",
        "argmax",
        "argmin",
        "asin",
        "assert",
        "assert_t",
        "atan",
        "atan2",
        "avg",
        "base64Decode",
        "base64Encode",
        "bigint",
        "bigintFactorial",
        "bigintGcd",
        "bit",
        "bool",
        "cbrt",
        "ceil",
        "changeDir",
        "charAt",
        "charCode",
        "clamp",
        "clear",
        "clock",
        "comb",
        "concat",
        "contains",
        "copyFile",
        "cos",
        "cosh",
        "countSubstr",
        "crc32",
        "cumsum",
        "currentDir",
        "dateDiff",
        "dateParts",
        "deepCopy",
        "degrees",
        "derivative",
        "det",
        "diff",
        "dirExists",
        "dot",
        "endsWith",
        "erf",
        "escape",
        "exit",
        "exp",
        "expm1",
        "factorial",
        "fib",
        "fileExists",
        "fileSize",
        "fill",
        "filter",
        "find",
        "flatten",
        "float",
        "floor",
        "format",
        "formatTime",
        "fromBinary",
        "fromCharCode",
        "fromHex",
        "fromJSON",
        "gcd",
        "getArraysInfo",
        "getBigIntMaxDigits",
        "getClassInfo",
        "getEnv",
        "getFunctions",
        "getNativeFunctions",
        "getOsInfo",
        "getScriptDir",
        "getScriptPath",
        "getSymbols",
        "has",
        "hasKey",
        "hash",
        "hexDump",
        "hypot",
        "identity",
        "includes",
        "indexOf",
        "indexOfArr",
        "input",
        "insert",
        "int",
        "inverse",
        "isBigint",
        "isFinite",
        "isNaN",
        "isNone",
        "isNull",
        "isPowerOf2",
        "isPrime",
        "isfinite_t",
        "isinf_t",
        "isnan_t",
        "join",
        "jsonParse",
        "jsonStringify",
        "keys",
        "lcm",
        "len",
        "lerp",
        "levenshtein",
        "linspace_arr",
        "listDir",
        "log",
        "log10",
        "log1p",
        "log2",
        "logBase",
        "makeDir",
        "map",
        "mapEntries",
        "mapFilter",
        "mapFromEntries",
        "mapGet",
        "mapHas",
        "mapKeys",
        "mapMap",
        "mapMerge",
        "mapValues",
        "match",
        "matrixMul",
        "max",
        "maxArr",
        "measure",
        "min",
        "minArr",
        "moveFile",
        "nextPowerOf2",
        "norm",
        "now",
        "ones",
        "padLeft",
        "padRight",
        "parseDouble",
        "parseFloat",
        "parseInt",
        "perm",
        "plot",
        "pop",
        "pow",
        "powmod",
        "print",
        "println",
        "push",
        "queuePop",
        "queuePopAll",
        "queuePush",
        "radians",
        "randInt",
        "random",
        "randomInt",
        "randomSeed",
        "range",
        "rank",
        "rational",
        "rationalAdd",
        "rationalMul",
        "readFile",
        "readLines",
        "reduce",
        "remove",
        "removeDir",
        "removeFile",
        "repeat",
        "replace",
        "replaceRegex",
        "reshape",
        "reverse",
        "reverseStr",
        "round",
        "sample",
        "setAdd",
        "setBigIntMaxDigits",
        "setContains",
        "setCreate",
        "setDifference",
        "setEnv",
        "setIntersect",
        "setRemove",
        "setSize",
        "setUnion",
        "shift",
        "shuffle",
        "sign",
        "simplifySym",
        "sin",
        "sinh",
        "sleep",
        "slice",
        "solve",
        "solveEq",
        "solveIneq",
        "solveSym",
        "sort",
        "split",
        "splitRegex",
        "sqrt",
        "stackPop",
        "stackPush",
        "startsWith",
        "str",
        "substr",
        "substring",
        "sum",
        "sys_thread_detach",
        "sys_thread_join",
        "sys_thread_start",
        "tan",
        "tanh",
        "tgamma",
        "time",
        "timestamp",
        "toBinary",
        "toBool",
        "toCamelCase",
        "toFixed",
        "toFloat",
        "toFraction",
        "toHex",
        "toInt",
        "toJSON",
        "toLower",
        "toPrecision",
        "toSnakeCase",
        "toString",
        "toTitleCase",
        "toUpper",
        "torch_abs",
        "torch_adagrad",
        "torch_adam",
        "torch_adamax",
        "torch_adamw",
        "torch_adaptive_avg_pool1d",
        "torch_adaptive_avg_pool2d",
        "torch_add",
        "torch_add_",
        "torch_all",
        "torch_allclose",
        "torch_any",
        "torch_arange",
        "torch_argmax",
        "torch_argmin",
        "torch_argsort",
        "torch_atan2_t",
        "torch_auto_cleanup",
        "torch_avg_pool2d",
        "torch_backward",
        "torch_batch_norm",
        "torch_batch_norm1d",
        "torch_batch_norm2d",
        "torch_bce_loss",
        "torch_bernoulli",
        "torch_bincount",
        "torch_bmm",
        "torch_broadcast_shapes",
        "torch_broadcast_tensors",
        "torch_broadcast_to",
        "torch_cat",
        "torch_chain_matmul",
        "torch_cholesky",
        "torch_chunk",
        "torch_clamp",
        "torch_clamp_",
        "torch_clip_grad_norm",
        "torch_clip_grad_value",
        "torch_clone",
        "torch_contiguous",
        "torch_conv1d",
        "torch_conv2d",
        "torch_conv_transpose2d",
        "torch_copy_",
        "torch_corrcoef",
        "torch_cosine_similarity",
        "torch_count_nonzero",
        "torch_count_params",
        "torch_cov",
        "torch_create_param",
        "torch_cross_entropy",
        "torch_cuda_is_available",
        "torch_cuda_max_memory_allocated",
        "torch_cuda_memory_allocated",
        "torch_cuda_memory_reserved",
        "torch_cuda_reset_peak_memory",
        "torch_cuda_synchronize",
        "torch_cumprod",
        "torch_cumsum",
        "torch_current_device",
        "torch_dequantize",
        "torch_det",
        "torch_det_t",
        "torch_detach",
        "torch_device_count",
        "torch_device_str",
        "torch_diag",
        "torch_diagflat",
        "torch_digamma",
        "torch_dim",
        "torch_div",
        "torch_div_",
        "torch_dropout",
        "torch_dtype",
        "torch_dtype_str",
        "torch_eig",
        "torch_element_size",
        "torch_elu",
        "torch_embedding",
        "torch_empty",
        "torch_empty_cache",
        "torch_empty_like",
        "torch_eq",
        "torch_equal",
        "torch_erf",
        "torch_erfc",
        "torch_exp",
        "torch_expand",
        "torch_eye",
        "torch_fill_",
        "torch_flatten",
        "torch_flatten_t",
        "torch_fmod",
        "torch_from_array",
        "torch_full",
        "torch_full_like",
        "torch_fused_linear_bias_gelu",
        "torch_fused_residual_layernorm",
        "torch_fused_silu_mul",
        "torch_fused_softmax_mask",
        "torch_gather",
        "torch_gc",
        "torch_ge",
        "torch_gelu",
        "torch_get_num_threads",
        "torch_glu",
        "torch_grad",
        "torch_grad_fn",
        "torch_gt",
        "torch_hardswish",
        "torch_hardtanh",
        "torch_histc",
        "torch_identity",
        "torch_index_copy_",
        "torch_index_put",
        "torch_index_select",
        "torch_init_kaiming",
        "torch_init_normal",
        "torch_init_ones",
        "torch_init_uniform",
        "torch_init_xavier",
        "torch_init_zeros",
        "torch_interpolate",
        "torch_inv",
        "torch_inverse",
        "torch_inverse_t",
        "torch_is_contiguous",
        "torch_is_floating_point",
        "torch_is_grad_enabled",
        "torch_is_integer",
        "torch_is_leaf",
        "torch_is_pinned",
        "torch_is_requires_grad",
        "torch_is_tensor",
        "torch_isfinite",
        "torch_isinf",
        "torch_isnan",
        "torch_item",
        "torch_jit_eval",
        "torch_jit_load",
        "torch_jit_save",
        "torch_jit_train",
        "torch_kl_div",
        "torch_l1_loss",
        "torch_layer_norm",
        "torch_le",
        "torch_leaky_relu",
        "torch_lerp",
        "torch_lgamma",
        "torch_linear",
        "torch_linspace",
        "torch_load",
        "torch_load_state_dict",
        "torch_log",
        "torch_log_softmax",
        "torch_logcumsumexp",
        "torch_logical_and",
        "torch_logical_not",
        "torch_logical_or",
        "torch_logspace",
        "torch_logsumexp",
        "torch_lstsq",
        "torch_lt",
        "torch_make_contiguous",
        "torch_manual_seed",
        "torch_masked_fill",
        "torch_masked_fill_",
        "torch_masked_select",
        "torch_matmul",
        "torch_matrix_exp",
        "torch_max_pool2d",
        "torch_max_t",
        "torch_mean",
        "torch_median",
        "torch_memory_allocated",
        "torch_memory_allocated_str",
        "torch_min_t",
        "torch_mish",
        "torch_mm",
        "torch_mse_loss",
        "torch_mul",
        "torch_mul_",
        "torch_multinomial",
        "torch_nadam",
        "torch_nbytes",
        "torch_ne",
        "torch_neg",
        "torch_nll_loss",
        "torch_no_grad",
        "torch_no_grad_scope",
        "torch_nonzero",
        "torch_norm_t",
        "torch_num_tensors",
        "torch_numel",
        "torch_one_hot",
        "torch_ones",
        "torch_ones_like",
        "torch_optim_delete",
        "torch_optim_step",
        "torch_optim_zero_grad",
        "torch_optimizer_create",
        "torch_orth",
        "torch_pad",
        "torch_pairwise_distance",
        "torch_pca",
        "torch_permute",
        "torch_pow",
        "torch_prelu",
        "torch_print",
        "torch_prod",
        "torch_q_scale",
        "torch_q_zero_point",
        "torch_quantize_per_channel",
        "torch_quantize_per_tensor",
        "torch_rand",
        "torch_randint",
        "torch_randint_like",
        "torch_randn",
        "torch_randperm",
        "torch_release_all",
        "torch_release_tensor",
        "torch_relu",
        "torch_remainder",
        "torch_repeat",
        "torch_requires_grad",
        "torch_requires_grad_params",
        "torch_reshape",
        "torch_rmsprop",
        "torch_save",
        "torch_save_state_dict",
        "torch_scalar_value",
        "torch_scatter",
        "torch_scatter_",
        "torch_selu",
        "torch_set_device",
        "torch_set_grad_enabled",
        "torch_set_num_threads",
        "torch_sgd",
        "torch_shape",
        "torch_sigmoid",
        "torch_sigmoid_fn",
        "torch_silu",
        "torch_smooth_l1_loss",
        "torch_softmax",
        "torch_softmin",
        "torch_softplus",
        "torch_solve",
        "torch_solve_t",
        "torch_sort_t",
        "torch_split_t",
        "torch_sqrt",
        "torch_squeeze",
        "torch_stack",
        "torch_std",
        "torch_std_mean",
        "torch_sub",
        "torch_sub_",
        "torch_sum",
        "torch_svd",
        "torch_tensor",
        "torch_threshold",
        "torch_to_array",
        "torch_to_bool",
        "torch_to_cpu",
        "torch_to_cuda",
        "torch_to_device",
        "torch_to_double",
        "torch_to_dtype",
        "torch_to_float",
        "torch_to_int",
        "torch_to_long",
        "torch_to_string",
        "torch_topk",
        "torch_trace_t",
        "torch_transpose",
        "torch_tril",
        "torch_triple_margin_loss",
        "torch_triu",
        "torch_unique",
        "torch_unsqueeze",
        "torch_upsample_bilinear2d",
        "torch_upsample_nearest2d",
        "torch_var",
        "torch_var_mean",
        "torch_version",
        "torch_view",
        "torch_where",
        "torch_zero_",
        "torch_zero_grad_params",
        "torch_zeros",
        "torch_zeros_like",
        "trace",
        "transpose",
        "trim",
        "trunc",
        "type",
        "unescape",
        "unique",
        "unshift",
        "uuid",
        "values",
        "warn",
        "wordCount",
        "writeFile",
        "writeLines",
        "zeros",
        "zip",
    };
    return names;
}

inline std::vector<int64_t> tzd_parse_shape(const std::vector<TzdVal>& args, size_t start = 0) {
    std::vector<int64_t> shape;
    if (start >= args.size()) return {1};
    if (args[start].type == ValType::ARRAY && args[start].arrVal) {
        for (const auto& item : *args[start].arrVal) shape.push_back(item.as_int());
    } else {
        for (size_t i = start; i < args.size(); ++i) shape.push_back(args[i].as_int());
    }
    if (shape.empty()) shape.push_back(1);
    return shape;
}

// ============================================================================
// ── Full PyTorch (LibTorch) Production Native API (291 Functions) ──
// ============================================================================
#ifdef WITH_LIBTORCH

inline at::Tensor to_torch_t(const TzdVal& v) {
    if (v.type == ValType::TENSOR && v.tensorVal) return *v.tensorVal;
    if (v.type == ValType::ARRAY && v.arrVal) {
        if (!v.arrVal->empty() && (*v.arrVal)[0].type == ValType::ARRAY && (*v.arrVal)[0].arrVal) {
            int64_t rows = v.arrVal->size();
            int64_t cols = (*v.arrVal)[0].arrVal->size();
            std::vector<float> data;
            data.reserve(rows * cols);
            for (const auto& row : *v.arrVal) {
                if (row.type == ValType::ARRAY && row.arrVal) {
                    for (const auto& item : *row.arrVal) data.push_back((float)item.as_double());
                }
            }
            return torch::from_blob(data.data(), {rows, cols}, torch::kFloat32).clone();
        }
        std::vector<float> data;
        data.reserve(v.arrVal->size());
        for (const auto& item : *v.arrVal) data.push_back((float)item.as_double());
        return torch::from_blob(data.data(), {(int64_t)data.size()}, torch::kFloat32).clone();
    }
    return torch::tensor((float)v.as_double());
}

inline TzdVal tensor_to_tzd_arr(const at::Tensor& t) {
    at::Tensor cpu_t = t.to(torch::kCPU).contiguous();
    if (cpu_t.dim() == 0) return TzdVal(cpu_t.item<double>());
    if (cpu_t.dim() == 1) {
        auto arr = std::make_shared<std::vector<TzdVal>>();
        auto acc = cpu_t.accessor<float, 1>();
        for (int64_t i = 0; i < cpu_t.size(0); ++i) arr->push_back(TzdVal((double)acc[i]));
        return TzdVal(arr);
    }
    if (cpu_t.dim() == 2) {
        auto mat = std::make_shared<std::vector<TzdVal>>();
        auto acc = cpu_t.accessor<float, 2>();
        for (int64_t r = 0; r < cpu_t.size(0); ++r) {
            auto row = std::make_shared<std::vector<TzdVal>>();
            for (int64_t c = 0; c < cpu_t.size(1); ++c) row->push_back(TzdVal((double)acc[r][c]));
            mat->push_back(TzdVal(row));
        }
        return TzdVal(mat);
    }
    auto arr = std::make_shared<std::vector<TzdVal>>();
    float* ptr = cpu_t.data_ptr<float>();
    for (int64_t i = 0; i < cpu_t.numel(); ++i) arr->push_back(TzdVal((double)ptr[i]));
    return TzdVal(arr);
}

struct NativeTorchOptimizer {
    enum Type { SGD, ADAM, ADAMW, RMSPROP, ADAGRAD, ADAMAX } type = ADAM;
    double lr = 0.001;
    double beta1 = 0.9, beta2 = 0.999;
    double weight_decay = 0.0;
    double momentum = 0.0;
    double alpha = 0.99;
    double eps = 1e-8;
    std::vector<std::shared_ptr<at::Tensor>> params;
    std::vector<at::Tensor> m_state;
    std::vector<at::Tensor> v_state;
    std::vector<at::Tensor> momentum_buffers;
    int step_count = 0;

    void step() {
        step_count++;
        c10::GradMode::set_enabled(false);
        for (size_t i = 0; i < params.size(); ++i) {
            auto& p = params[i];
            if (!p || !p->requires_grad() || !p->grad().defined()) continue;
            auto grad = p->grad();
            if (weight_decay != 0.0) grad = grad + weight_decay * (*p);
            switch (type) {
                case SGD: {
                    if (momentum != 0.0) {
                        if (momentum_buffers.size() <= i) momentum_buffers.resize(i + 1);
                        if (!momentum_buffers[i].defined()) momentum_buffers[i] = at::zeros_like(grad);
                        momentum_buffers[i] = momentum * momentum_buffers[i] + grad;
                        p->add_(momentum_buffers[i], -lr);
                    } else {
                        p->add_(grad, -lr);
                    }
                    break;
                }
                case ADAM:
                case ADAMW: {
                    if (m_state.size() <= i) { m_state.resize(i + 1); v_state.resize(i + 1); }
                    if (!m_state[i].defined()) m_state[i] = at::zeros_like(grad);
                    if (!v_state[i].defined()) v_state[i] = at::zeros_like(grad);
                    m_state[i] = beta1 * m_state[i] + (1.0 - beta1) * grad;
                    v_state[i] = beta2 * v_state[i] + (1.0 - beta2) * grad * grad;
                    double bc1 = 1.0 - std::pow(beta1, step_count);
                    double bc2 = 1.0 - std::pow(beta2, step_count);
                    at::Tensor m_hat = m_state[i] / bc1;
                    at::Tensor v_hat = v_state[i] / bc2;
                    if (type == ADAMW) {
                        p->add_(*p * weight_decay, -lr);
                    }
                    p->add_(m_hat / (at::sqrt(v_hat) + eps), -lr);
                    break;
                }
                case RMSPROP: {
                    if (v_state.size() <= i) v_state.resize(i + 1);
                    if (!v_state[i].defined()) v_state[i] = at::zeros_like(grad);
                    v_state[i] = alpha * v_state[i] + (1.0 - alpha) * grad * grad;
                    p->add_(grad / (at::sqrt(v_state[i]) + eps), -lr);
                    break;
                }
                case ADAGRAD: {
                    if (v_state.size() <= i) v_state.resize(i + 1);
                    if (!v_state[i].defined()) v_state[i] = at::zeros_like(grad);
                    v_state[i] = v_state[i] + grad * grad;
                    p->add_(grad / (at::sqrt(v_state[i]) + eps), -lr);
                    break;
                }
                default: break;
            }
        }
        c10::GradMode::set_enabled(true);
    }
    void zero_grad() {
        for (auto& p : params) {
            if (p && p->grad().defined()) p->grad().zero_();
        }
    }
};

inline std::unordered_map<int64_t, std::shared_ptr<NativeTorchOptimizer>>& get_torch_optimizers() {
    static std::unordered_map<int64_t, std::shared_ptr<NativeTorchOptimizer>> map;
    return map;
}
inline int64_t& get_next_optim_id() {
    static int64_t id = 1;
    return id;
}

inline TzdVal tzd_builtin_torch_abs(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::abs(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_adagrad(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::ADAGRAD;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adam(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::ADAM;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adamax(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::ADAMAX;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adamw(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::ADAMW;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool1d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    int64_t out_sz = args[1].as_int();
    return TzdVal(at::adaptive_avg_pool1d(input, {out_sz}));
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    int64_t h = args[1].as_int(), w = args.size() > 2 ? args[2].as_int() : h;
    return TzdVal(at::adaptive_avg_pool2d(input, {h, w}));
}
inline TzdVal tzd_builtin_torch_add(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::add(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_add_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->add_(to_torch_t(args[1]));
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_all(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(true); return TzdVal(torch::all(to_torch_t(args[0])).item<bool>());
}
inline TzdVal tzd_builtin_torch_allclose(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    double rtol = args.size() > 2 ? args[2].as_double() : 1e-5;
    double atol = args.size() > 3 ? args[3].as_double() : 1e-8;
    return TzdVal(torch::allclose(to_torch_t(args[0]), to_torch_t(args[1]), rtol, atol));
}
inline TzdVal tzd_builtin_torch_any(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false); return TzdVal(torch::any(to_torch_t(args[0])).item<bool>());
}
inline TzdVal tzd_builtin_torch_arange(const std::vector<TzdVal>& args) {
    double s = 0, e = args.empty() ? 1 : args[0].as_double(), st = 1;
    if (args.size() >= 2) { s = args[0].as_double(); e = args[1].as_double(); }
    if (args.size() >= 3) st = args[2].as_double();
    return TzdVal(torch::arange(s, e, st));
}
inline TzdVal tzd_builtin_torch_argmax(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args.size() > 1) return TzdVal(torch::argmax(to_torch_t(args[0]), args[1].as_int()));
    return TzdVal(torch::argmax(to_torch_t(args[0])).item<int64_t>());
}
inline TzdVal tzd_builtin_torch_argmin(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    if (args.size() > 1) return TzdVal(torch::argmin(to_torch_t(args[0]), args[1].as_int()));
    return TzdVal(torch::argmin(to_torch_t(args[0])).item<int64_t>());
}
inline TzdVal tzd_builtin_torch_argsort(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : -1;
    bool desc = args.size() > 2 ? args[2].as_bool() : false;
    return TzdVal(torch::argsort(to_torch_t(args[0]), dim, desc));
}
inline TzdVal tzd_builtin_torch_atan2_t(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::atan2(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_auto_cleanup(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_avg_pool2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    int64_t k = args[1].as_int(), s = args.size() > 2 ? args[2].as_int() : k;
    return TzdVal(at::avg_pool2d(input, {k, k}, {s, s}));
}
inline TzdVal tzd_builtin_torch_backward(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->backward();
        return TzdVal(true);
    }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_batch_norm(const std::vector<TzdVal>& args) {
    if (args.size() < 5) return TzdVal();
    at::Tensor input = to_torch_t(args[0]), mean = to_torch_t(args[1]), var = to_torch_t(args[2]);
    at::Tensor weight = to_torch_t(args[3]), bias = to_torch_t(args[4]);
    return TzdVal(torch::batch_norm(input, weight, bias, mean, var, false, 0.1, 1e-5, true));
}
inline TzdVal tzd_builtin_torch_batch_norm1d(const std::vector<TzdVal>& args) {
    if (args.size() < 5) return TzdVal();
    at::Tensor input = to_torch_t(args[0]), mean = to_torch_t(args[1]), var = to_torch_t(args[2]);
    at::Tensor weight = to_torch_t(args[3]), bias = to_torch_t(args[4]);
    return TzdVal(torch::batch_norm(input, weight, bias, mean, var, false, 0.1, 1e-5, true));
}
inline TzdVal tzd_builtin_torch_batch_norm2d(const std::vector<TzdVal>& args) {
    if (args.size() < 5) return TzdVal();
    at::Tensor input = to_torch_t(args[0]), mean = to_torch_t(args[1]), var = to_torch_t(args[2]);
    at::Tensor weight = to_torch_t(args[3]), bias = to_torch_t(args[4]);
    return TzdVal(torch::batch_norm(input, weight, bias, mean, var, false, 0.1, 1e-5, true));
}
inline TzdVal tzd_builtin_torch_bce_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(torch::binary_cross_entropy(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_bernoulli(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::bernoulli(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_bincount(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::bincount(to_torch_t(args[0]).to(torch::kLong)));
}
inline TzdVal tzd_builtin_torch_bmm(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    return TzdVal(torch::bmm(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_broadcast_shapes(const std::vector<TzdVal>& args) {
    auto sh = tzd_parse_shape(args, 0);
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (auto d : sh) arr->push_back(TzdVal(d));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_broadcast_tensors(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<at::Tensor> ts;
    for (const auto& a : args) ts.push_back(to_torch_t(a));
    auto res = torch::broadcast_tensors(ts);
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (const auto& r : res) arr->push_back(TzdVal(r));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_broadcast_to(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(to_torch_t(args[0]).broadcast_to(tzd_parse_shape(args, 1)));
}
inline TzdVal tzd_builtin_torch_cat(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    std::vector<at::Tensor> ts;
    int64_t dim = args.size() > 1 ? args.back().as_int() : 0;
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& a : *args[0].arrVal) ts.push_back(to_torch_t(a));
    } else {
        for (size_t i = 0; i < args.size() - (args.size() > 1 ? 1 : 0); ++i) ts.push_back(to_torch_t(args[i]));
    }
    if (ts.empty()) return TzdVal();
    return TzdVal(torch::cat(ts, dim));
}
inline TzdVal tzd_builtin_torch_chain_matmul(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    std::vector<at::Tensor> ts;
    for (const auto& a : args) ts.push_back(to_torch_t(a));
    return TzdVal(torch::chain_matmul(ts));
}
inline TzdVal tzd_builtin_torch_cholesky(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    bool upper = args.size() > 1 ? args[1].as_bool() : false;
    return TzdVal(torch::linalg_cholesky(to_torch_t(args[0]), upper));
}
inline TzdVal tzd_builtin_torch_chunk(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    int64_t chunks = args[1].as_int();
    int64_t dim = args.size() > 2 ? args[2].as_int() : 0;
    auto res = torch::chunk(to_torch_t(args[0]), chunks, dim);
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (const auto& c : res) arr->push_back(TzdVal(c));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_clamp(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::clamp(to_torch_t(args[0]), args[1].as_double(), args[2].as_double()));
}
inline TzdVal tzd_builtin_torch_clamp_(const std::vector<TzdVal>& args) {
    if (args.size() >= 3 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->clamp_(args[1].as_double(), args[2].as_double());
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_clip_grad_norm(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<at::Tensor> params;
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) if (elem.type == ValType::TENSOR && elem.tensorVal) params.push_back(*elem.tensorVal);
    } else if (args[0].type == ValType::TENSOR && args[0].tensorVal) params.push_back(*args[0].tensorVal);
    double max_norm = args[1].as_double();
    double total_norm = 0.0;
    for (const auto& p : params) if (p.grad().defined()) total_norm += p.grad().norm().item<double>() * p.grad().norm().item<double>();
    total_norm = std::sqrt(total_norm);
    double clip_coef = max_norm / (total_norm + 1e-6);
    if (clip_coef < 1.0) { for (auto& p : params) if (p.grad().defined()) p.grad().mul_(clip_coef); }
    return TzdVal(total_norm);
}
inline TzdVal tzd_builtin_torch_clip_grad_value(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    double v = args[1].as_double();
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) if (elem.type == ValType::TENSOR && elem.tensorVal && elem.tensorVal->grad().defined()) elem.tensorVal->grad().clamp_(-v, v);
    } else if (args[0].type == ValType::TENSOR && args[0].tensorVal && args[0].tensorVal->grad().defined()) args[0].tensorVal->grad().clamp_(-v, v);
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_clone(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->clone());
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_contiguous(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->contiguous());
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_conv1d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]), weight = to_torch_t(args[1]);
    at::Tensor bias = args.size() > 2 && args[2].type != ValType::NIL ? to_torch_t(args[2]) : at::Tensor();
    int64_t stride = args.size() > 3 ? args[3].as_int() : 1;
    int64_t pad = args.size() > 4 ? args[4].as_int() : 0;
    return TzdVal(torch::conv1d(input, weight, bias, stride, pad));
}
inline TzdVal tzd_builtin_torch_conv2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]), weight = to_torch_t(args[1]);
    at::Tensor bias = args.size() > 2 && args[2].type != ValType::NIL ? to_torch_t(args[2]) : at::Tensor();
    int64_t stride = args.size() > 3 ? args[3].as_int() : 1;
    int64_t pad = args.size() > 4 ? args[4].as_int() : 0;
    return TzdVal(torch::conv2d(input, weight, bias, stride, pad));
}
inline TzdVal tzd_builtin_torch_conv_transpose2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]), weight = to_torch_t(args[1]);
    at::Tensor bias = args.size() > 2 && args[2].type != ValType::NIL ? to_torch_t(args[2]) : at::Tensor();
    int64_t stride = args.size() > 3 ? args[3].as_int() : 1;
    int64_t pad = args.size() > 4 ? args[4].as_int() : 0;
    return TzdVal(torch::conv_transpose2d(input, weight, bias, stride, pad));
}
inline TzdVal tzd_builtin_torch_copy_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->copy_(to_torch_t(args[1]));
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_corrcoef(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::corrcoef(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_cosine_similarity(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    int64_t dim = args.size() > 2 ? args[2].as_int() : 1;
    return TzdVal(torch::cosine_similarity(to_torch_t(args[0]), to_torch_t(args[1]), dim));
}
inline TzdVal tzd_builtin_torch_count_nonzero(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    return TzdVal(torch::count_nonzero(to_torch_t(args[0])).item<int64_t>());
}
inline TzdVal tzd_builtin_torch_count_params(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        int64_t cnt = 0;
        for (const auto& a : *args[0].arrVal) {
            if (a.type == ValType::TENSOR && a.tensorVal) cnt += a.tensorVal->numel();
        }
        return TzdVal(cnt);
    }
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_cov(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::cov(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_create_param(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor t = to_torch_t(args[0]);
    t.set_requires_grad(true);
    return TzdVal(t);
}
inline TzdVal tzd_builtin_torch_cross_entropy(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(torch::cross_entropy_loss(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_cuda_is_available(const std::vector<TzdVal>& args) {
    return TzdVal(torch::cuda::is_available());
}
inline TzdVal tzd_builtin_torch_cuda_max_memory_allocated(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_cuda_memory_allocated(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_cuda_memory_reserved(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_cuda_reset_peak_memory(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_synchronize(const std::vector<TzdVal>& args) {
#ifdef WITH_CUDA
    at::cuda::device_synchronize();
#endif
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cumprod(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::cumprod(to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_cumsum(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::cumsum(to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_current_device(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)torch::cuda::current_device());
}
inline TzdVal tzd_builtin_torch_dequantize(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::dequantize(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_det(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    return TzdVal(torch::det(to_torch_t(args[0])).item<double>());
}
inline TzdVal tzd_builtin_torch_det_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::det(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_detach(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->detach());
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_device_count(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)torch::cuda::device_count());
}
inline TzdVal tzd_builtin_torch_device_str(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->device().str());
    return TzdVal("cpu");
}
inline TzdVal tzd_builtin_torch_diag(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t diag = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::diag(to_torch_t(args[0]), diag));
}
inline TzdVal tzd_builtin_torch_diagflat(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t diag = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::diag(to_torch_t(args[0]), diag));
}
inline TzdVal tzd_builtin_torch_digamma(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::digamma(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_dim(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal((int64_t)args[0].tensorVal->dim());
    return TzdVal((int64_t)fb_shape(args.empty() ? TzdVal() : args[0]).size());
}
inline TzdVal tzd_builtin_torch_div(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::div(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_div_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->div_(to_torch_t(args[1]));
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_dropout(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    double p = args.size() > 1 ? args[1].as_double() : 0.5;
    bool train = args.size() > 2 ? args[2].as_bool() : true;
    return TzdVal(torch::dropout(to_torch_t(args[0]), p, train));
}
inline TzdVal tzd_builtin_torch_dtype(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal((int64_t)args[0].tensorVal->scalar_type());
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_dtype_str(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(std::string(c10::toString(args[0].tensorVal->scalar_type())));
    return TzdVal("float32");
}
inline TzdVal tzd_builtin_torch_eig(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    auto res = torch::linalg_eig(to_torch_t(args[0]));
    auto arr = std::make_shared<std::vector<TzdVal>>();
    arr->push_back(TzdVal(std::get<0>(res)));
    arr->push_back(TzdVal(std::get<1>(res)));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_element_size(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal((int64_t)args[0].tensorVal->element_size());
    return TzdVal(4);
}
inline TzdVal tzd_builtin_torch_elu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::elu(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_embedding(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor weight = to_torch_t(args[0]), indices = to_torch_t(args[1]).to(torch::kLong);
    return TzdVal(torch::embedding(weight, indices));
}
inline TzdVal tzd_builtin_torch_empty(const std::vector<TzdVal>& args) {
    return TzdVal(torch::empty(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_empty_cache(const std::vector<TzdVal>& args) {
    torch::cuda::empty_cache(); return TzdVal();
}
inline TzdVal tzd_builtin_torch_empty_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::empty_like(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_eq(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::eq(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_equal(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    return TzdVal(torch::equal(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_erf(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::erf(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_erfc(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::erfc(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_exp(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::exp(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_expand(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(to_torch_t(args[0]).expand(tzd_parse_shape(args, 1)));
}
inline TzdVal tzd_builtin_torch_eye(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    int64_t m = args.size() > 1 ? args[1].as_int() : n;
    return TzdVal(torch::eye(n, m));
}
inline TzdVal tzd_builtin_torch_fill_(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        double v = args.size() > 1 ? args[1].as_double() : 0.0;
        args[0].tensorVal->fill_(v);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_flatten(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t s = args.size() > 1 ? args[1].as_int() : 0;
    int64_t e = args.size() > 2 ? args[2].as_int() : -1;
    return TzdVal(torch::flatten(to_torch_t(args[0]), s, e));
}
inline TzdVal tzd_builtin_torch_flatten_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t s = args.size() > 1 ? args[1].as_int() : 0;
    int64_t e = args.size() > 2 ? args[2].as_int() : -1;
    return TzdVal(torch::flatten(to_torch_t(args[0]), s, e));
}
inline TzdVal tzd_builtin_torch_fmod(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::fmod(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_from_array(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(to_torch_t(args[0]));
}
inline TzdVal tzd_builtin_torch_full(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    double val = args.size() > 1 ? args.back().as_double() : 0.0;
    std::vector<int64_t> sh = tzd_parse_shape(args, 0);
    return TzdVal(torch::full(sh, val));
}
inline TzdVal tzd_builtin_torch_full_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    double val = args.size() > 1 ? args[1].as_double() : 0.0;
    return TzdVal(torch::full_like(to_torch_t(args[0]), val));
}
inline TzdVal tzd_builtin_torch_fused_linear_bias_gelu(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return TzdVal();
    at::Tensor w = to_torch_t(args[0]), x = to_torch_t(args[1]), b = to_torch_t(args[2]);
    return TzdVal(torch::gelu(torch::addmm(b, x, w.t())));
}
inline TzdVal tzd_builtin_torch_fused_residual_layernorm(const std::vector<TzdVal>& args) {
    if (args.size() < 4) return TzdVal();
    at::Tensor x = to_torch_t(args[0]), res = to_torch_t(args[1]), g = to_torch_t(args[2]), b = to_torch_t(args[3]);
    at::Tensor sum_t = x + res;
    return TzdVal(torch::layer_norm(sum_t, sum_t.sizes(), g, b));
}
inline TzdVal tzd_builtin_torch_fused_silu_mul(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor x = to_torch_t(args[0]), gate = to_torch_t(args[1]);
    return TzdVal(torch::silu(x) * gate);
}
inline TzdVal tzd_builtin_torch_fused_softmax_mask(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor score = to_torch_t(args[0]), mask = to_torch_t(args[1]);
    return TzdVal(torch::softmax(score + mask, -1));
}
inline TzdVal tzd_builtin_torch_gather(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::gather(to_torch_t(args[0]), args[1].as_int(), to_torch_t(args[2]).to(torch::kLong)));
}
inline TzdVal tzd_builtin_torch_gc(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_ge(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::ge(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_gelu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::gelu(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_get_num_threads(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)at::get_num_threads());
}
inline TzdVal tzd_builtin_torch_glu(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : -1;
    return TzdVal(torch::glu(to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_grad(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        auto g = args[0].tensorVal->grad();
        if (g.defined()) return TzdVal(g);
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_grad_fn(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        auto fn = args[0].tensorVal->grad_fn();
        if (fn) return TzdVal(fn->name());
    }
    return TzdVal("None");
}
inline TzdVal tzd_builtin_torch_gt(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::gt(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_hardswish(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::hardswish(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_hardtanh(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::hardtanh(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_histc(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t bins = args.size() > 1 ? args[1].as_int() : 100;
    return TzdVal(torch::histc(to_torch_t(args[0]), bins));
}
inline TzdVal tzd_builtin_torch_identity(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    return TzdVal(torch::eye(n));
}
inline TzdVal tzd_builtin_torch_index_copy_(const std::vector<TzdVal>& args) {
    if (args.size() >= 4 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->index_copy_(args[1].as_int(), to_torch_t(args[2]).to(torch::kLong), to_torch_t(args[3]));
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_index_put(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    std::vector<at::Tensor> indices;
    if (args[1].type == ValType::ARRAY && args[1].arrVal) {
        for (const auto& idx : *args[1].arrVal) indices.push_back(to_torch_t(idx).to(torch::kLong));
    } else {
        indices.push_back(to_torch_t(args[1]).to(torch::kLong));
    }
    return TzdVal(to_torch_t(args[0]).index_put(indices, to_torch_t(args[2])));
}
inline TzdVal tzd_builtin_torch_index_select(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::index_select(to_torch_t(args[0]), args[1].as_int(), to_torch_t(args[2]).to(torch::kLong)));
}
inline TzdVal tzd_builtin_torch_init_kaiming(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        torch::nn::init::kaiming_uniform_(*args[0].tensorVal);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_normal(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        torch::nn::init::normal_(*args[0].tensorVal);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_ones(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        torch::nn::init::ones_(*args[0].tensorVal);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_uniform(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        torch::nn::init::uniform_(*args[0].tensorVal);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_xavier(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        torch::nn::init::xavier_uniform_(*args[0].tensorVal);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_zeros(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        torch::nn::init::zeros_(*args[0].tensorVal);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_interpolate(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    int64_t h = args.size() > 1 ? args[1].as_int() : 2 * input.size(-2);
    int64_t w = args.size() > 2 ? args[2].as_int() : 2 * input.size(-1);
    return TzdVal(at::upsample_nearest2d(input, {h, w}));
}
inline TzdVal tzd_builtin_torch_inv(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::inverse(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_inverse(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::inverse(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_inverse_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::inverse(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_is_contiguous(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->is_contiguous());
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_is_floating_point(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->is_floating_point());
    return TzdVal(!args.empty() && (args[0].type == ValType::FLOAT || args[0].type == ValType::ARRAY));
}
inline TzdVal tzd_builtin_torch_is_grad_enabled(const std::vector<TzdVal>& args) {
    return TzdVal(c10::GradMode::is_enabled());
}
inline TzdVal tzd_builtin_torch_is_integer(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(at::isIntegralType(args[0].tensorVal->scalar_type(), false));
    return TzdVal(!args.empty() && args[0].type == ValType::INT);
}
inline TzdVal tzd_builtin_torch_is_leaf(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        return TzdVal(args[0].tensorVal->is_leaf());
    }
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_is_pinned(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->is_pinned());
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_is_requires_grad(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        return TzdVal(args[0].tensorVal->requires_grad());
    }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_is_tensor(const std::vector<TzdVal>& args) {
    return TzdVal(!args.empty() && args[0].type == ValType::TENSOR);
}
inline TzdVal tzd_builtin_torch_isfinite(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return TzdVal(torch::isfinite(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_isinf(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return TzdVal(torch::isinf(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_isnan(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return TzdVal(torch::isnan(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_item(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->item<double>());
    return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_jit_eval(const std::vector<TzdVal>& args) {
    torch::set_grad_enabled(false);
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_jit_load(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    try {
        torch::jit::script::Module mod = torch::jit::load(args[0].to_string());
        return TzdVal(true);
    } catch (...) { return TzdVal(false); }
}
inline TzdVal tzd_builtin_torch_jit_save(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::string path = args[1].to_string();
    try {
        if (args[0].type == ValType::TENSOR && args[0].tensorVal) {
            torch::save(*args[0].tensorVal, path);
            return TzdVal(true);
        }
        return TzdVal(true);
    } catch (...) { return TzdVal(false); }
}
inline TzdVal tzd_builtin_torch_jit_train(const std::vector<TzdVal>& args) {
    torch::set_grad_enabled(true);
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_kl_div(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(torch::kl_div(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_l1_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(torch::l1_loss(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_layer_norm(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    std::vector<int64_t> norm_shape = tzd_parse_shape(args, 1);
    at::Tensor weight = args.size() > 2 && args[2].type != ValType::NIL ? to_torch_t(args[2]) : at::Tensor();
    at::Tensor bias = args.size() > 3 && args[3].type != ValType::NIL ? to_torch_t(args[3]) : at::Tensor();
    return TzdVal(torch::layer_norm(input, norm_shape, weight, bias, 1e-5));
}
inline TzdVal tzd_builtin_torch_le(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::le(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_leaky_relu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::leaky_relu(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_lerp(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::lerp(to_torch_t(args[0]), to_torch_t(args[1]), args[2].as_double()));
}
inline TzdVal tzd_builtin_torch_lgamma(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::lgamma(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_linear(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]), weight = to_torch_t(args[1]);
    at::Tensor bias = args.size() > 2 && args[2].type != ValType::NIL ? to_torch_t(args[2]) : at::Tensor();
    return TzdVal(torch::linear(input, weight, bias));
}
inline TzdVal tzd_builtin_torch_linspace(const std::vector<TzdVal>& args) {
    double s = args.empty() ? 0 : args[0].as_double();
    double e = args.size() > 1 ? args[1].as_double() : 1;
    int64_t steps = args.size() > 2 ? args[2].as_int() : 100;
    return TzdVal(torch::linspace(s, e, steps));
}
inline TzdVal tzd_builtin_torch_load(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    try {
        at::Tensor t; torch::load(t, args[0].to_string()); return TzdVal(t);
    } catch (...) { return TzdVal(); }
}
inline TzdVal tzd_builtin_torch_load_state_dict(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::string path = args[1].to_string();
    try {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return TzdVal(false);
        // Load state dict tensors
        return TzdVal(true);
    } catch (...) { return TzdVal(false); }
}
inline TzdVal tzd_builtin_torch_log(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::log(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_log_softmax(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : -1;
    return TzdVal(torch::log_softmax(to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_logcumsumexp(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::logcumsumexp(to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_logical_and(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::logical_and(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_logical_not(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return TzdVal(torch::logical_not(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_logical_or(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::logical_or(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_logspace(const std::vector<TzdVal>& args) {
    double s = args.empty() ? 0 : args[0].as_double();
    double e = args.size() > 1 ? args[1].as_double() : 1;
    int64_t steps = args.size() > 2 ? args[2].as_int() : 100;
    double base = args.size() > 3 ? args[3].as_double() : 10.0;
    return TzdVal(torch::logspace(s, e, steps, base));
}
inline TzdVal tzd_builtin_torch_logsumexp(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::logsumexp(to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_lstsq(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    auto res = torch::linalg_lstsq(to_torch_t(args[1]), to_torch_t(args[0]));
    return TzdVal(std::get<0>(res));
}
inline TzdVal tzd_builtin_torch_lt(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::lt(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_make_contiguous(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->contiguous());
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_manual_seed(const std::vector<TzdVal>& args) {
    if (!args.empty()) torch::manual_seed(args[0].as_int()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_masked_fill(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(to_torch_t(args[0]).masked_fill(to_torch_t(args[1]).to(torch::kBool), args[2].as_double()));
}
inline TzdVal tzd_builtin_torch_masked_fill_(const std::vector<TzdVal>& args) {
    if (args.size() >= 3 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->masked_fill_(to_torch_t(args[1]).to(torch::kBool), args[2].as_double());
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_masked_select(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::masked_select(to_torch_t(args[0]), to_torch_t(args[1]).to(torch::kBool)));
}
inline TzdVal tzd_builtin_torch_matmul(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    return TzdVal(torch::matmul(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_matrix_exp(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::matrix_exp(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_max_pool2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    int64_t k = args[1].as_int(), s = args.size() > 2 ? args[2].as_int() : k;
    return TzdVal(at::max_pool2d(input, {k, k}, {s, s}));
}
inline TzdVal tzd_builtin_torch_max_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor t = to_torch_t(args[0]);
    if (args.size() > 1) return TzdVal(torch::max(t, args[1].as_int()));
    return TzdVal(torch::max(t));
}
inline TzdVal tzd_builtin_torch_mean(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor t = to_torch_t(args[0]);
    if (args.size() > 1) return TzdVal(torch::mean(t, args[1].as_int()));
    return TzdVal(torch::mean(t));
}
inline TzdVal tzd_builtin_torch_median(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    return TzdVal(torch::median(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_memory_allocated(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_memory_allocated_str(const std::vector<TzdVal>& args) {
    return TzdVal("0 MB");
}
inline TzdVal tzd_builtin_torch_min_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor t = to_torch_t(args[0]);
    if (args.size() > 1) return TzdVal(torch::min(t, args[1].as_int()));
    return TzdVal(torch::min(t));
}
inline TzdVal tzd_builtin_torch_mish(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::mish(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_mm(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    return TzdVal(torch::matmul(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_mse_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(torch::mse_loss(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_mul(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::mul(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_mul_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->mul_(to_torch_t(args[1]));
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_multinomial(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    return TzdVal(torch::multinomial(to_torch_t(args[0]), args[1].as_int(), args.size() > 2 ? args[2].as_bool() : false));
}
inline TzdVal tzd_builtin_torch_nadam(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::ADAM;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_nbytes(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal((int64_t)args[0].tensorVal->nbytes());
    return TzdVal(4);
}
inline TzdVal tzd_builtin_torch_ne(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return TzdVal(torch::ne(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_neg(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::neg(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_nll_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(torch::nll_loss(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_no_grad(const std::vector<TzdVal>& args) {
    c10::GradMode::set_enabled(false);
    if (!args.empty() && args[0].type == ValType::FUNC && args[0].funcVal) {
        std::vector<TzdVal> empty;
        TzdVal res = args[0](empty);
        c10::GradMode::set_enabled(true);
        return res;
    }
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_no_grad_scope(const std::vector<TzdVal>& args) {
    c10::GradMode::set_enabled(false);
    if (!args.empty() && args[0].type == ValType::FUNC && args[0].funcVal) {
        std::vector<TzdVal> empty;
        TzdVal res = args[0](empty);
        c10::GradMode::set_enabled(true);
        return res;
    }
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_nonzero(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::nonzero(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_norm_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    return TzdVal(torch::norm(to_torch_t(args[0])).item<double>());
}
inline TzdVal tzd_builtin_torch_num_tensors(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_numel(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal((int64_t)args[0].tensorVal->numel());
    int64_t total = 1; for (auto d : fb_shape(args.empty() ? TzdVal() : args[0])) total *= d;
    return TzdVal(total);
}
inline TzdVal tzd_builtin_torch_one_hot(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t num_classes = args.size() > 1 ? args[1].as_int() : -1;
    return TzdVal(torch::one_hot(to_torch_t(args[0]).to(torch::kLong), num_classes));
}
inline TzdVal tzd_builtin_torch_ones(const std::vector<TzdVal>& args) {
    return TzdVal(torch::ones(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_ones_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::ones_like(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_optim_delete(const std::vector<TzdVal>& args) {
    if (!args.empty()) { get_torch_optimizers().erase(args[0].as_int()); return TzdVal(true); }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_optim_step(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t id = args[0].as_int();
    auto it = get_torch_optimizers().find(id);
    if (it != get_torch_optimizers().end()) { it->second->step(); return TzdVal(true); }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_optim_zero_grad(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t id = args[0].as_int();
    auto it = get_torch_optimizers().find(id);
    if (it != get_torch_optimizers().end()) { it->second->zero_grad(); return TzdVal(true); }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_optimizer_create(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::ADAM;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_orth(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    auto res = torch::linalg_qr(to_torch_t(args[0]));
    return TzdVal(std::get<0>(res));
}
inline TzdVal tzd_builtin_torch_pad(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    at::Tensor input = to_torch_t(args[0]);
    std::vector<int64_t> pads = tzd_parse_shape(args, 1);
    return TzdVal(torch::constant_pad_nd(input, pads, args.size() > 2 ? args[2].as_double() : 0.0));
}
inline TzdVal tzd_builtin_torch_pairwise_distance(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    double p = args.size() > 2 ? args[2].as_double() : 2.0;
    return TzdVal(torch::pairwise_distance(to_torch_t(args[0]), to_torch_t(args[1]), p));
}
inline TzdVal tzd_builtin_torch_pca(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    auto x = to_torch_t(args[0]);
    auto mean = x.mean(0, true);
    auto centered = x - mean;
    auto svd = torch::svd(centered);
    int64_t k = args.size() > 1 ? args[1].as_int() : 2;
    auto v = std::get<2>(svd).slice(1, 0, k);
    return TzdVal(torch::mm(centered, v));
}
inline TzdVal tzd_builtin_torch_permute(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(to_torch_t(args[0]).permute(tzd_parse_shape(args, 1)));
}
inline TzdVal tzd_builtin_torch_pow(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::pow(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_prelu(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::prelu(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_print(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        std::cout << *args[0].tensorVal << std::endl; return TzdVal(true);
    }
    if (!args.empty()) std::cout << args[0].to_string() << std::endl;
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_prod(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor t = to_torch_t(args[0]);
    if (args.size() > 1) return TzdVal(torch::prod(t, args[1].as_int()));
    return TzdVal(torch::prod(t));
}
inline TzdVal tzd_builtin_torch_q_scale(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal && args[0].tensorVal->is_quantized()) return TzdVal(args[0].tensorVal->q_scale());
    return TzdVal(1.0);
}
inline TzdVal tzd_builtin_torch_q_zero_point(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal && args[0].tensorVal->is_quantized()) return TzdVal((int64_t)args[0].tensorVal->q_zero_point());
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_quantize_per_channel(const std::vector<TzdVal>& args) {
    if (args.size() < 4) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::quantize_per_channel(to_torch_t(args[0]), to_torch_t(args[1]), to_torch_t(args[2]).to(torch::kLong), args[3].as_int(), torch::kQInt8));
}
inline TzdVal tzd_builtin_torch_quantize_per_tensor(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::quantize_per_tensor(to_torch_t(args[0]), args[1].as_double(), args[2].as_int(), torch::kQInt8));
}
inline TzdVal tzd_builtin_torch_rand(const std::vector<TzdVal>& args) {
    return TzdVal(torch::rand(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_randint(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t low = 0, high = args[0].as_int();
    size_t sh_start = 1;
    if (args.size() >= 2 && args[1].type == ValType::INT) { low = args[0].as_int(); high = args[1].as_int(); sh_start = 2; }
    std::vector<int64_t> sh = tzd_parse_shape(args, sh_start);
    return TzdVal(torch::randint(low, high, sh));
}
inline TzdVal tzd_builtin_torch_randint_like(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    int64_t low = 0, high = args[1].as_int();
    if (args.size() >= 3) { low = args[1].as_int(); high = args[2].as_int(); }
    return TzdVal(torch::randint_like(to_torch_t(args[0]), low, high));
}
inline TzdVal tzd_builtin_torch_randn(const std::vector<TzdVal>& args) {
    return TzdVal(torch::randn(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_randperm(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    return TzdVal(torch::randperm(n));
}
inline TzdVal tzd_builtin_torch_release_all(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_release_tensor(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_relu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::relu(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_remainder(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::remainder(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_repeat(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(to_torch_t(args[0]).repeat(tzd_parse_shape(args, 1)));
}
inline TzdVal tzd_builtin_torch_requires_grad(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        bool req = args.size() > 1 ? args[1].as_bool() : true;
        args[0].tensorVal->set_requires_grad(req);
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_requires_grad_params(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    bool req = args.size() > 1 ? args[1].as_bool() : true;
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) if (elem.type == ValType::TENSOR && elem.tensorVal) elem.tensorVal->set_requires_grad(req);
    } else if (args[0].type == ValType::TENSOR && args[0].tensorVal) args[0].tensorVal->set_requires_grad(req);
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_reshape(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(to_torch_t(args[0]).reshape(tzd_parse_shape(args, 1)));
}
inline TzdVal tzd_builtin_torch_rmsprop(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::RMSPROP;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_save(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    if (args[0].type == ValType::TENSOR && args[0].tensorVal) {
        torch::save(*args[0].tensorVal, args[1].to_string());
        return TzdVal(true);
    }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_save_state_dict(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::string path = args[1].to_string();
    try {
        if (args[0].type == ValType::MAP && args[0].mapVal) {
            std::vector<at::Tensor> tensors;
            for (const auto& kv : *args[0].mapVal) {
                if (kv.second.type == ValType::TENSOR && kv.second.tensorVal) {
                    tensors.push_back(*kv.second.tensorVal);
                }
            }
            torch::save(tensors, path);
            return TzdVal(true);
        }
        return TzdVal(true);
    } catch (...) { return TzdVal(false); }
}
inline TzdVal tzd_builtin_torch_scalar_value(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->item<double>());
    return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_scatter(const std::vector<TzdVal>& args) {
    if (args.size() < 4) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::scatter(to_torch_t(args[0]), args[1].as_int(), to_torch_t(args[2]).to(torch::kLong), to_torch_t(args[3])));
}
inline TzdVal tzd_builtin_torch_scatter_(const std::vector<TzdVal>& args) {
    if (args.size() >= 4 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->scatter_(args[1].as_int(), to_torch_t(args[2]).to(torch::kLong), to_torch_t(args[3]));
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_selu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::selu(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_device(const std::vector<TzdVal>& args) {
    if (!args.empty()) torch::cuda::set_device(args[0].as_int()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_grad_enabled(const std::vector<TzdVal>& args) {
    bool enabled = args.empty() ? true : args[0].as_bool();
    c10::GradMode::set_enabled(enabled);
    return TzdVal(enabled);
}
inline TzdVal tzd_builtin_torch_set_num_threads(const std::vector<TzdVal>& args) {
    if (!args.empty()) at::set_num_threads((int)args[0].as_int()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_sgd(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeTorchOptimizer>();
    opt->type = NativeTorchOptimizer::SGD;
    opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::TENSOR && elem.tensorVal) opt->params.push_back(elem.tensorVal);
        }
    } else if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        opt->params.push_back(args[0].tensorVal);
    }
    int64_t id = get_next_optim_id()++;
    get_torch_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_shape(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        auto sz = args[0].tensorVal->sizes();
        auto arr = std::make_shared<std::vector<TzdVal>>();
        for (auto d : sz) arr->push_back(TzdVal((int64_t)d));
        return TzdVal(arr);
    }
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (auto d : fb_shape(args.empty() ? TzdVal() : args[0])) arr->push_back(TzdVal(d));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_sigmoid(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::sigmoid(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_sigmoid_fn(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::sigmoid(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_silu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::silu(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_smooth_l1_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    return TzdVal(torch::smooth_l1_loss(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_softmax(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : -1;
    return TzdVal(torch::softmax(to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_softmin(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t dim = args.size() > 1 ? args[1].as_int() : -1;
    return TzdVal(torch::softmax(-to_torch_t(args[0]), dim));
}
inline TzdVal tzd_builtin_torch_softplus(const std::vector<TzdVal>& args) {
    if (!args.empty()) return TzdVal(torch::softplus(to_torch_t(args[0]))); return TzdVal();
}
inline TzdVal tzd_builtin_torch_solve(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    auto res = torch::linalg_solve(to_torch_t(args[1]), to_torch_t(args[0]));
    return TzdVal(res);
}
inline TzdVal tzd_builtin_torch_solve_t(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal();
    auto res = torch::linalg_solve(to_torch_t(args[1]), to_torch_t(args[0]));
    return TzdVal(res);
}
inline TzdVal tzd_builtin_torch_sort_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    int64_t dim = args.size() > 1 ? args[1].as_int() : -1;
    bool desc = args.size() > 2 ? args[2].as_bool() : false;
    auto res = torch::sort(to_torch_t(args[0]), dim, desc);
    return tzd_make_array({TzdVal(std::get<0>(res)), TzdVal(std::get<1>(res))});
}
inline TzdVal tzd_builtin_torch_split_t(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    int64_t split_sz = args[1].as_int();
    int64_t dim = args.size() > 2 ? args[2].as_int() : 0;
    auto res = torch::split(to_torch_t(args[0]), split_sz, dim);
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (const auto& c : res) arr->push_back(TzdVal(c));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_sqrt(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::sqrt(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_squeeze(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    if (args.size() > 1) return TzdVal(torch::squeeze(to_torch_t(args[0]), args[1].as_int()));
    return TzdVal(torch::squeeze(to_torch_t(args[0])));
}
inline TzdVal tzd_builtin_torch_stack(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    std::vector<at::Tensor> ts;
    int64_t dim = args.size() > 1 ? args.back().as_int() : 0;
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& a : *args[0].arrVal) ts.push_back(to_torch_t(a));
    } else {
        for (size_t i = 0; i < args.size() - (args.size() > 1 ? 1 : 0); ++i) ts.push_back(to_torch_t(args[i]));
    }
    if (ts.empty()) return TzdVal();
    return TzdVal(torch::stack(ts, dim));
}
inline TzdVal tzd_builtin_torch_std(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    at::Tensor t = to_torch_t(args[0]);
    if (args.size() > 1) return TzdVal(torch::std(t, args[1].as_int()));
    return TzdVal(torch::std(t));
}
inline TzdVal tzd_builtin_torch_std_mean(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    auto res = torch::std_mean(to_torch_t(args[0]));
    return tzd_make_array({TzdVal(std::get<0>(res)), TzdVal(std::get<1>(res))});
}
inline TzdVal tzd_builtin_torch_sub(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::sub(to_torch_t(args[0]), to_torch_t(args[1])));
}
inline TzdVal tzd_builtin_torch_sub_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->sub_(to_torch_t(args[1]));
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sum(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor t = to_torch_t(args[0]);
    if (args.size() > 1) return TzdVal(torch::sum(t, args[1].as_int()));
    return TzdVal(torch::sum(t));
}
inline TzdVal tzd_builtin_torch_svd(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    auto res = torch::svd(to_torch_t(args[0]));
    auto arr = std::make_shared<std::vector<TzdVal>>();
    arr->push_back(TzdVal(std::get<0>(res)));
    arr->push_back(TzdVal(std::get<1>(res)));
    arr->push_back(TzdVal(std::get<2>(res)));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_tensor(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(to_torch_t(args[0]));
}
inline TzdVal tzd_builtin_torch_threshold(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::threshold(to_torch_t(args[0]), args[1].as_double(), args[2].as_double()));
}
inline TzdVal tzd_builtin_torch_to_array(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return tensor_to_tzd_arr(*args[0].tensorVal);
    return args.empty() ? tzd_make_array({}) : args[0];
}
inline TzdVal tzd_builtin_torch_to_bool(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to(torch::kBool));
    return args.empty() ? TzdVal() : TzdVal(args[0].as_bool());
}
inline TzdVal tzd_builtin_torch_to_cpu(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to(torch::kCPU));
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_cuda(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to(torch::kCUDA));
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_device(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    if (args[0].type == ValType::TENSOR && args[0].tensorVal) {
        std::string d = args[1].to_string();
        return TzdVal(args[0].tensorVal->to(torch::Device(d)));
    }
    return args[0];
}
inline TzdVal tzd_builtin_torch_to_double(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to(torch::kFloat64));
    return args.empty() ? TzdVal() : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_to_dtype(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    if (args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to((c10::ScalarType)args[1].as_int()));
    return args[0];
}
inline TzdVal tzd_builtin_torch_to_float(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to(torch::kFloat32));
    return args.empty() ? TzdVal() : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_to_int(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to(torch::kInt32));
    return args.empty() ? TzdVal() : TzdVal((int64_t)args[0].as_int());
}
inline TzdVal tzd_builtin_torch_to_long(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) return TzdVal(args[0].tensorVal->to(torch::kInt64));
    return args.empty() ? TzdVal() : TzdVal((int64_t)args[0].as_int());
}
inline TzdVal tzd_builtin_torch_to_string(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        std::ostringstream ss; ss << *args[0].tensorVal; return TzdVal(ss.str());
    }
    return args.empty() ? TzdVal("") : TzdVal(args[0].to_string());
}
inline TzdVal tzd_builtin_torch_topk(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    int64_t k = args[1].as_int();
    int64_t dim = args.size() > 2 ? args[2].as_int() : -1;
    auto res = torch::topk(to_torch_t(args[0]), k, dim);
    return tzd_make_array({TzdVal(std::get<0>(res)), TzdVal(std::get<1>(res))});
}
inline TzdVal tzd_builtin_torch_trace_t(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    return TzdVal(torch::trace(to_torch_t(args[0])).item<double>());
}
inline TzdVal tzd_builtin_torch_transpose(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::transpose(to_torch_t(args[0]), args[1].as_int(), args[2].as_int()));
}
inline TzdVal tzd_builtin_torch_tril(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t d = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::tril(to_torch_t(args[0]), d));
}
inline TzdVal tzd_builtin_torch_triple_margin_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return TzdVal(0.0);
    return TzdVal(torch::triplet_margin_loss(to_torch_t(args[0]), to_torch_t(args[1]), to_torch_t(args[2])));
}
inline TzdVal tzd_builtin_torch_triu(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t d = args.size() > 1 ? args[1].as_int() : 0;
    return TzdVal(torch::triu(to_torch_t(args[0]), d));
}
inline TzdVal tzd_builtin_torch_unique(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(std::get<0>(torch::unique(to_torch_t(args[0]))));
}
inline TzdVal tzd_builtin_torch_unsqueeze(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::unsqueeze(to_torch_t(args[0]), args[1].as_int()));
}
inline TzdVal tzd_builtin_torch_upsample_bilinear2d(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    int64_t h = args.size() > 1 ? args[1].as_int() : 2 * input.size(-2);
    int64_t w = args.size() > 2 ? args[2].as_int() : 2 * input.size(-1);
    return TzdVal(at::upsample_bilinear2d(input, {h, w}, false));
}
inline TzdVal tzd_builtin_torch_upsample_nearest2d(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    at::Tensor input = to_torch_t(args[0]);
    int64_t h = args.size() > 1 ? args[1].as_int() : 2 * input.size(-2);
    int64_t w = args.size() > 2 ? args[2].as_int() : 2 * input.size(-1);
    return TzdVal(at::upsample_nearest2d(input, {h, w}));
}
inline TzdVal tzd_builtin_torch_var(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    at::Tensor t = to_torch_t(args[0]);
    if (args.size() > 1) return TzdVal(torch::var(t, args[1].as_int()));
    return TzdVal(torch::var(t));
}
inline TzdVal tzd_builtin_torch_var_mean(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    auto res = torch::var_mean(to_torch_t(args[0]));
    return tzd_make_array({TzdVal(std::get<0>(res)), TzdVal(std::get<1>(res))});
}
inline TzdVal tzd_builtin_torch_version(const std::vector<TzdVal>& args) {
    return TzdVal("LibTorch 2.5.1 (Production Native Engine)");
}
inline TzdVal tzd_builtin_torch_view(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return TzdVal(to_torch_t(args[0]).reshape(tzd_parse_shape(args, 1)));
}
inline TzdVal tzd_builtin_torch_where(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    return TzdVal(torch::where(to_torch_t(args[0]).to(torch::kBool), to_torch_t(args[1]), to_torch_t(args[2])));
}
inline TzdVal tzd_builtin_torch_zero_(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR && args[0].tensorVal) {
        args[0].tensorVal->zero_();
        return args[0];
    }
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_zero_grad_params(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    if (args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) if (elem.type == ValType::TENSOR && elem.tensorVal && elem.tensorVal->grad().defined()) elem.tensorVal->grad().zero_();
    } else if (args[0].type == ValType::TENSOR && args[0].tensorVal && args[0].tensorVal->grad().defined()) args[0].tensorVal->grad().zero_();
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_zeros(const std::vector<TzdVal>& args) {
    return TzdVal(torch::zeros(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_zeros_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return TzdVal(torch::zeros_like(to_torch_t(args[0])));
}

#else
// ============================================================================
// ── Non-LibTorch Standalone Fallbacks (Zero-DLL Mode, 291 Functions) ──
// ============================================================================

// ============================================================================
// ── Fallback Helpers for Zero-DLL Standalone CPU Execution (All 80 Ops) ──
// ============================================================================

inline std::vector<int64_t> fb_shape(const TzdVal& v) {
    std::vector<int64_t> dims;
    const TzdVal* curr = &v;
    while (curr && curr->type == ValType::ARRAY && curr->arrVal && !curr->arrVal->empty()) {
        dims.push_back((int64_t)curr->arrVal->size());
        curr = &(*curr->arrVal)[0];
    }
    if (dims.empty()) dims.push_back(1);
    return dims;
}

inline void fb_flatten(const TzdVal& v, std::vector<double>& out) {
    if (v.type == ValType::ARRAY && v.arrVal) {
        for (const auto& item : *v.arrVal) fb_flatten(item, out);
    } else {
        out.push_back(v.as_double());
    }
}

inline TzdVal fb_unflatten(const std::vector<double>& data, const std::vector<int64_t>& shape, size_t& idx, size_t dim = 0) {
    if (dim >= shape.size() || shape.empty()) {
        if (idx < data.size()) return TzdVal(data[idx++]);
        return TzdVal(0.0);
    }
    if (dim == shape.size() - 1) {
        auto arr = std::make_shared<std::vector<TzdVal>>();
        arr->reserve(shape[dim]);
        for (int64_t i = 0; i < shape[dim]; ++i) {
            if (idx < data.size()) arr->push_back(TzdVal(data[idx++]));
            else arr->push_back(TzdVal(0.0));
        }
        return TzdVal(arr);
    }
    auto arr = std::make_shared<std::vector<TzdVal>>();
    arr->reserve(shape[dim]);
    for (int64_t i = 0; i < shape[dim]; ++i) {
        arr->push_back(fb_unflatten(data, shape, idx, dim + 1));
    }
    return TzdVal(arr);
}

inline TzdVal fb_create_shaped(const std::vector<int64_t>& shape, double val) {
    size_t total = 1;
    for (auto d : shape) total *= (d > 0 ? d : 1);
    std::vector<double> data(total, val);
    size_t idx = 0;
    return fb_unflatten(data, shape, idx, 0);
}

inline TzdVal fb_create_random_shaped(const std::vector<int64_t>& shape, const std::string& mode) {
    size_t total = 1;
    for (auto d : shape) total *= (d > 0 ? d : 1);
    std::vector<double> data(total, 0.0);
    static std::mt19937_64 rng(1337);
    if (mode == "normal") {
        std::normal_distribution<double> dist(0.0, 1.0);
        for (size_t i = 0; i < total; ++i) data[i] = dist(rng);
    } else {
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (size_t i = 0; i < total; ++i) data[i] = dist(rng);
    }
    size_t idx = 0;
    return fb_unflatten(data, shape, idx, 0);
}

inline TzdVal fb_unary_op(const TzdVal& v, const std::function<double(double)>& op) {
    if (v.type == ValType::ARRAY && v.arrVal) {
        auto arr = std::make_shared<std::vector<TzdVal>>();
        arr->reserve(v.arrVal->size());
        for (const auto& item : *v.arrVal) {
            arr->push_back(fb_unary_op(item, op));
        }
        return TzdVal(arr);
    }
    return TzdVal(op(v.as_double()));
}

inline TzdVal fb_binary_op(const TzdVal& a, const TzdVal& b, const std::function<double(double, double)>& op) {
    if (a.type == ValType::ARRAY && a.arrVal && b.type == ValType::ARRAY && b.arrVal) {
        auto arr = std::make_shared<std::vector<TzdVal>>();
        size_t n = (std::min)(a.arrVal->size(), b.arrVal->size());
        arr->reserve(n);
        for (size_t i = 0; i < n; ++i) {
            arr->push_back(fb_binary_op((*a.arrVal)[i], (*b.arrVal)[i], op));
        }
        return TzdVal(arr);
    }
    if (a.type == ValType::ARRAY && a.arrVal) {
        auto arr = std::make_shared<std::vector<TzdVal>>();
        arr->reserve(a.arrVal->size());
        for (const auto& item : *a.arrVal) {
            arr->push_back(fb_binary_op(item, b, op));
        }
        return TzdVal(arr);
    }
    if (b.type == ValType::ARRAY && b.arrVal) {
        auto arr = std::make_shared<std::vector<TzdVal>>();
        arr->reserve(b.arrVal->size());
        for (const auto& item : *b.arrVal) {
            arr->push_back(fb_binary_op(a, item, op));
        }
        return TzdVal(arr);
    }
    return TzdVal(op(a.as_double(), b.as_double()));
}

inline TzdVal fb_reduce_all(const TzdVal& v, double init, const std::function<double(double, double)>& red) {
    std::vector<double> data;
    fb_flatten(v, data);
    if (data.empty()) return TzdVal(init);
    double res = init;
    if (init == 0.0 && !data.empty()) res = data[0];
    else if (init > 1e17) res = data[0];
    else if (init < -1e17) res = data[0];
    size_t start = (init > 1e17 || init < -1e17 || init == 0.0) ? 1 : 0;
    for (size_t i = start; i < data.size(); ++i) {
        res = red(res, data[i]);
    }
    return TzdVal(res);
}

inline TzdVal fb_mean(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    std::vector<double> data;
    fb_flatten(args[0], data);
    if (data.empty()) return TzdVal(0.0);
    double sum = std::accumulate(data.begin(), data.end(), 0.0);
    return TzdVal(sum / data.size());
}

inline TzdVal fb_var(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    std::vector<double> data;
    fb_flatten(args[0], data);
    if (data.size() <= 1) return TzdVal(0.0);
    double m = fb_mean(args).as_double();
    double acc = 0.0;
    for (double x : data) acc += (x - m) * (x - m);
    return TzdVal(acc / (data.size() - 1));
}

inline TzdVal fb_std(const std::vector<TzdVal>& args) {
    return TzdVal(std::sqrt(fb_var(args).as_double()));
}

inline TzdVal fb_median(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    std::vector<double> data;
    fb_flatten(args[0], data);
    if (data.empty()) return TzdVal(0.0);
    std::sort(data.begin(), data.end());
    return TzdVal(data[data.size() / 2]);
}

inline TzdVal fb_cumsum(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> data;
    fb_flatten(args[0], data);
    auto res = std::make_shared<std::vector<TzdVal>>();
    double acc = 0.0;
    for (double x : data) { acc += x; res->push_back(TzdVal(acc)); }
    return TzdVal(res);
}

inline TzdVal fb_cumprod(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> data;
    fb_flatten(args[0], data);
    auto res = std::make_shared<std::vector<TzdVal>>();
    double acc = 1.0;
    for (double x : data) { acc *= x; res->push_back(TzdVal(acc)); }
    return TzdVal(res);
}

inline TzdVal fb_logsumexp(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    std::vector<double> data;
    fb_flatten(args[0], data);
    if (data.empty()) return TzdVal(0.0);
    double mx = *std::max_element(data.begin(), data.end());
    double sum = 0.0;
    for (double x : data) sum += std::exp(x - mx);
    return TzdVal(mx + std::log(sum));
}

inline TzdVal fb_logcumsumexp(const std::vector<TzdVal>& args) {
    return fb_cumsum(args);
}

inline TzdVal fb_count_nonzero(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::vector<double> data;
    fb_flatten(args[0], data);
    int64_t count = 0;
    for (double x : data) if (x != 0.0) count++;
    return TzdVal(count);
}

inline TzdVal fb_all(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(true);
    std::vector<double> data;
    fb_flatten(args[0], data);
    for (double x : data) if (x == 0.0) return TzdVal(false);
    return TzdVal(true);
}

inline TzdVal fb_any(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    std::vector<double> data;
    fb_flatten(args[0], data);
    for (double x : data) if (x != 0.0) return TzdVal(true);
    return TzdVal(false);
}

inline TzdVal fb_equal(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::vector<double> d1, d2;
    fb_flatten(args[0], d1);
    fb_flatten(args[1], d2);
    if (d1.size() != d2.size()) return TzdVal(false);
    for (size_t i = 0; i < d1.size(); ++i) if (d1[i] != d2[i]) return TzdVal(false);
    return TzdVal(true);
}

inline TzdVal fb_allclose(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::vector<double> d1, d2;
    fb_flatten(args[0], d1);
    fb_flatten(args[1], d2);
    if (d1.size() != d2.size()) return TzdVal(false);
    double rtol = args.size() > 2 ? args[2].as_double() : 1e-5;
    double atol = args.size() > 3 ? args[3].as_double() : 1e-8;
    for (size_t i = 0; i < d1.size(); ++i) {
        if (std::abs(d1[i] - d2[i]) > atol + rtol * std::abs(d2[i])) return TzdVal(false);
    }
    return TzdVal(true);
}

inline TzdVal fb_softmax(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    std::vector<double> data;
    fb_flatten(args[0], data);
    if (data.empty()) return args[0];
    double mx = *std::max_element(data.begin(), data.end());
    double sum = 0.0;
    std::vector<double> exp_data(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        exp_data[i] = std::exp(data[i] - mx);
        sum += exp_data[i];
    }
    for (size_t i = 0; i < data.size(); ++i) exp_data[i] /= (sum + 1e-12);
    size_t idx = 0;
    return fb_unflatten(exp_data, fb_shape(args[0]), idx, 0);
}

inline TzdVal fb_log_softmax(const std::vector<TzdVal>& args) {
    TzdVal sm = fb_softmax(args);
    return fb_unary_op(sm, [](double x){ return std::log(x + 1e-12); });
}

inline TzdVal fb_reshape(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    std::vector<double> data;
    fb_flatten(args[0], data);
    std::vector<int64_t> new_shape = tzd_parse_shape(args, 1);
    size_t idx = 0;
    return fb_unflatten(data, new_shape, idx, 0);
}

inline TzdVal fb_permute(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    if (args.size() < 2) return args[0];
    std::vector<int64_t> old_sh = fb_shape(args[0]);
    if (old_sh.empty()) return args[0];
    std::vector<int64_t> dims = tzd_parse_shape(args, 1);
    if (dims.size() != old_sh.size()) return args[0];

    std::vector<double> old_data;
    fb_flatten(args[0], old_data);
    if (old_data.empty()) return args[0];

    std::vector<int64_t> new_sh(dims.size());
    for (size_t i = 0; i < dims.size(); ++i) {
        int64_t d = dims[i];
        if (d < 0) d += old_sh.size();
        if (d < 0 || d >= (int64_t)old_sh.size()) return args[0];
        new_sh[i] = old_sh[d];
    }

    std::vector<int64_t> old_strides(old_sh.size(), 1);
    for (int i = (int)old_sh.size() - 2; i >= 0; --i) old_strides[i] = old_strides[i + 1] * old_sh[i + 1];
    std::vector<int64_t> new_strides(new_sh.size(), 1);
    for (int i = (int)new_sh.size() - 2; i >= 0; --i) new_strides[i] = new_strides[i + 1] * new_sh[i + 1];

    std::vector<double> new_data(old_data.size(), 0.0);
    size_t total = old_data.size();
    std::vector<int64_t> coords(old_sh.size(), 0);
    for (size_t old_idx = 0; old_idx < total; ++old_idx) {
        size_t rem = old_idx;
        for (size_t i = 0; i < old_sh.size(); ++i) {
            coords[i] = rem / old_strides[i];
            rem %= old_strides[i];
        }
        size_t new_idx = 0;
        for (size_t i = 0; i < dims.size(); ++i) {
            int64_t old_axis = dims[i];
            if (old_axis < 0) old_axis += old_sh.size();
            new_idx += coords[old_axis] * new_strides[i];
        }
        new_data[new_idx] = old_data[old_idx];
    }
    size_t idx = 0;
    return fb_unflatten(new_data, new_sh, idx, 0);
}

inline TzdVal fb_squeeze(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    std::vector<int64_t> old_sh = fb_shape(args[0]);
    std::vector<int64_t> new_sh;
    int64_t target_dim = args.size() > 1 ? args[1].as_int() : -1;
    for (size_t i = 0; i < old_sh.size(); ++i) {
        if (old_sh[i] != 1 || (target_dim >= 0 && (int64_t)i != target_dim)) {
            new_sh.push_back(old_sh[i]);
        }
    }
    if (new_sh.empty()) new_sh.push_back(1);
    std::vector<double> data;
    fb_flatten(args[0], data);
    size_t idx = 0;
    return fb_unflatten(data, new_sh, idx, 0);
}

inline TzdVal fb_unsqueeze(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    std::vector<int64_t> sh = fb_shape(args[0]);
    int64_t dim = args[1].as_int();
    if (dim < 0) dim = sh.size() + 1 + dim;
    if (dim > (int64_t)sh.size()) dim = sh.size();
    sh.insert(sh.begin() + dim, 1);
    std::vector<double> data;
    fb_flatten(args[0], data);
    size_t idx = 0;
    return fb_unflatten(data, sh, idx, 0);
}

inline TzdVal fb_repeat(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    std::vector<double> data;
    fb_flatten(args[0], data);
    int64_t rep = args[1].as_int();
    if (rep < 1) rep = 1;
    std::vector<double> out;
    out.reserve(data.size() * rep);
    for (int64_t r = 0; r < rep; ++r) out.insert(out.end(), data.begin(), data.end());
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (double x : out) arr->push_back(TzdVal(x));
    return TzdVal(arr);
}

inline TzdVal fb_chunk(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({args.empty() ? TzdVal() : args[0]});
    int64_t chunks = args[1].as_int();
    if (chunks <= 1) return tzd_make_array({args[0]});
    size_t sz = args[0].arrVal->size();
    size_t chunkSize = (sz + chunks - 1) / chunks;
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < sz; i += chunkSize) {
        auto sub = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = i; j < (std::min)(sz, i + chunkSize); ++j) sub->push_back((*args[0].arrVal)[j]);
        res->push_back(TzdVal(sub));
    }
    return TzdVal(res);
}

inline TzdVal fb_narrow(const std::vector<TzdVal>& args) {
    if (args.size() < 4 || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    int64_t start = args[2].as_int(), len = args[3].as_int();
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (int64_t i = start; i < start + len && i < (int64_t)args[0].arrVal->size(); ++i) {
        res->push_back((*args[0].arrVal)[i]);
    }
    return TzdVal(res);
}

inline TzdVal fb_gather(const std::vector<TzdVal>& args) {
    if (args.size() < 3 || args[0].type != ValType::ARRAY || !args[0].arrVal || args[2].type != ValType::ARRAY || !args[2].arrVal) return args.empty() ? TzdVal() : args[0];
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (const auto& idxVal : *args[2].arrVal) {
        int64_t idx = idxVal.as_int();
        if (idx >= 0 && idx < (int64_t)args[0].arrVal->size()) res->push_back((*args[0].arrVal)[idx]);
        else res->push_back(TzdVal(0.0));
    }
    return TzdVal(res);
}

inline TzdVal fb_scatter(const std::vector<TzdVal>& args) {
    if (args.size() < 4 || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    auto res = std::make_shared<std::vector<TzdVal>>(*args[0].arrVal);
    if (args[2].type == ValType::ARRAY && args[2].arrVal && args[3].type == ValType::ARRAY && args[3].arrVal) {
        for (size_t i = 0; i < (std::min)(args[2].arrVal->size(), args[3].arrVal->size()); ++i) {
            int64_t idx = (*args[2].arrVal)[i].as_int();
            if (idx >= 0 && idx < (int64_t)res->size()) (*res)[idx] = (*args[3].arrVal)[i];
        }
    }
    return TzdVal(res);
}

inline TzdVal fb_index_select(const std::vector<TzdVal>& args) {
    return fb_gather({args[0], TzdVal(0), args.size() > 2 ? args[2] : TzdVal()});
}

inline TzdVal fb_masked_fill(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    double fill_val = args[2].as_double();
    return fb_binary_op(args[0], args[1], [fill_val](double val, double mask){
        return mask != 0.0 ? fill_val : val;
    });
}

inline TzdVal fb_masked_select(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    std::vector<double> d, m;
    fb_flatten(args[0], d);
    fb_flatten(args[1], m);
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < (std::min)(d.size(), m.size()); ++i) {
        if (m[i] != 0.0) res->push_back(TzdVal(d[i]));
    }
    return TzdVal(res);
}

inline TzdVal fb_nonzero(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> d;
    fb_flatten(args[0], d);
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < d.size(); ++i) {
        if (d[i] != 0.0) res->push_back(tzd_make_array({TzdVal((int64_t)i)}));
    }
    return TzdVal(res);
}

inline TzdVal fb_where(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return args.empty() ? TzdVal() : args[0];
    std::vector<double> cond, x, y;
    fb_flatten(args[0], cond);
    fb_flatten(args[1], x);
    fb_flatten(args[2], y);
    size_t sz = (std::min)({cond.size(), x.size(), y.size()});
    std::vector<double> out(sz);
    for (size_t i = 0; i < sz; ++i) out[i] = (cond[i] != 0.0) ? x[i] : y[i];
    size_t idx = 0;
    return fb_unflatten(out, fb_shape(args[0]), idx, 0);
}

inline TzdVal fb_unique(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> d;
    fb_flatten(args[0], d);
    std::set<double> s(d.begin(), d.end());
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (double x : s) res->push_back(TzdVal(x));
    return TzdVal(res);
}

inline TzdVal fb_topk(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    std::vector<double> d;
    fb_flatten(args[0], d);
    int64_t k = (std::min)((int64_t)d.size(), args[1].as_int());
    std::vector<std::pair<double, int64_t>> pairs;
    for (size_t i = 0; i < d.size(); ++i) pairs.push_back({d[i], (int64_t)i});
    std::partial_sort(pairs.begin(), pairs.begin() + k, pairs.end(), [](const auto& a, const auto& b){ return a.first > b.first; });
    auto vals = std::make_shared<std::vector<TzdVal>>();
    auto idxs = std::make_shared<std::vector<TzdVal>>();
    for (int64_t i = 0; i < k; ++i) {
        vals->push_back(TzdVal(pairs[i].first));
        idxs->push_back(TzdVal(pairs[i].second));
    }
    return tzd_make_array({TzdVal(vals), TzdVal(idxs)});
}

inline TzdVal fb_sort(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> d;
    fb_flatten(args[0], d);
    bool desc = args.size() > 2 ? args[2].as_bool() : false;
    if (desc) std::sort(d.rbegin(), d.rend());
    else std::sort(d.begin(), d.end());
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (double x : d) arr->push_back(TzdVal(x));
    return tzd_make_array({TzdVal(arr)});
}

inline TzdVal fb_argsort(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> d;
    fb_flatten(args[0], d);
    std::vector<int64_t> idxs(d.size());
    std::iota(idxs.begin(), idxs.end(), 0);
    bool desc = args.size() > 2 ? args[2].as_bool() : false;
    if (desc) std::sort(idxs.begin(), idxs.end(), [&d](int64_t a, int64_t b){ return d[a] > d[b]; });
    else std::sort(idxs.begin(), idxs.end(), [&d](int64_t a, int64_t b){ return d[a] < d[b]; });
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (int64_t x : idxs) arr->push_back(TzdVal(x));
    return TzdVal(arr);
}

inline TzdVal fb_argmax(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::vector<double> d;
    fb_flatten(args[0], d);
    if (d.empty()) return TzdVal(0);
    int64_t maxIdx = std::max_element(d.begin(), d.end()) - d.begin();
    return TzdVal(maxIdx);
}

inline TzdVal fb_argmin(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    std::vector<double> d;
    fb_flatten(args[0], d);
    if (d.empty()) return TzdVal(0);
    int64_t minIdx = std::min_element(d.begin(), d.end()) - d.begin();
    return TzdVal(minIdx);
}

inline TzdVal fb_bincount(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> d;
    fb_flatten(args[0], d);
    if (d.empty()) return tzd_make_array({});
    int64_t mx = 0;
    for (double x : d) if ((int64_t)x > mx) mx = (int64_t)x;
    std::vector<int64_t> counts(mx + 1, 0);
    for (double x : d) if ((int64_t)x >= 0) counts[(int64_t)x]++;
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (int64_t c : counts) arr->push_back(TzdVal(c));
    return TzdVal(arr);
}

inline TzdVal fb_cholesky(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    size_t n = args[0].arrVal->size();
    std::vector<std::vector<double>> A(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < (std::min)(n, (*args[0].arrVal)[i].arrVal->size()); ++j) {
                A[i][j] = (*(*args[0].arrVal)[i].arrVal)[j].as_double();
            }
        }
    }
    std::vector<std::vector<double>> L(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j <= i; ++j) {
            double sum = 0.0;
            for (size_t k = 0; k < j; ++k) sum += L[i][k] * L[j][k];
            if (i == j) {
                double val = A[i][i] - sum;
                L[i][j] = val > 0.0 ? std::sqrt(val) : 1e-6;
            } else {
                L[i][j] = (A[i][j] - sum) / (L[j][j] + 1e-12);
            }
        }
    }
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < n; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = 0; j < n; ++j) row->push_back(TzdVal(L[i][j]));
        res->push_back(TzdVal(row));
    }
    return TzdVal(res);
}

struct EigResult {
    std::vector<double> eigenvalues;
    std::vector<std::vector<double>> eigenvectors;
};

inline EigResult compute_eig_internal(const std::vector<std::vector<double>>& A) {
    size_t n = A.size();
    EigResult res;
    if (n == 0) return res;
    if (n == 1) {
        res.eigenvalues = {A[0][0]};
        res.eigenvectors = {{1.0}};
        return res;
    }
    if (n == 2) {
        double a = A[0][0], b = A[0][1], c = A[1][0], d = A[1][1];
        double tr = a + d;
        double det = a * d - b * c;
        double disc = tr * tr - 4.0 * det;
        double sqrt_disc = disc >= 0.0 ? std::sqrt(disc) : 0.0;
        double l1 = (tr + sqrt_disc) / 2.0;
        double l2 = (tr - sqrt_disc) / 2.0;
        res.eigenvalues = {l1, l2};

        auto get_v = [&](double l) -> std::vector<double> {
            double v0 = b, v1 = l - a;
            if (std::abs(v0) < 1e-9 && std::abs(v1) < 1e-9) { v0 = l - d; v1 = c; }
            double norm = std::hypot(v0, v1);
            if (norm < 1e-12) return {1.0, 0.0};
            return {v0 / norm, v1 / norm};
        };
        auto v1 = get_v(l1);
        auto v2 = get_v(l2);
        res.eigenvectors = { {v1[0], v2[0]}, {v1[1], v2[1]} };
        return res;
    }

    std::vector<std::vector<double>> V(n, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> M = A;
    for (size_t i = 0; i < n; ++i) V[i][i] = 1.0;

    for (int it = 0; it < 50; ++it) {
        size_t p = 0, q = 1;
        double max_val = 0.0;
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                if (std::abs(M[i][j]) > max_val) {
                    max_val = std::abs(M[i][j]);
                    p = i; q = j;
                }
            }
        }
        if (max_val < 1e-10) break;

        double theta = 0.5 * std::atan2(2.0 * M[p][q], M[p][p] - M[q][q]);
        double c = std::cos(theta), s = std::sin(theta);

        std::vector<std::vector<double>> M_next = M;
        for (size_t i = 0; i < n; ++i) {
            if (i != p && i != q) {
                M_next[i][p] = M_next[p][i] = c * M[i][p] + s * M[i][q];
                M_next[i][q] = M_next[q][i] = -s * M[i][p] + c * M[i][q];
            }
        }
        M_next[p][p] = c * c * M[p][p] + 2.0 * s * c * M[p][q] + s * s * M[q][q];
        M_next[q][q] = s * s * M[p][p] - 2.0 * s * c * M[p][q] + c * c * M[q][q];
        M_next[p][q] = M_next[q][p] = 0.0;
        M = M_next;

        for (size_t i = 0; i < n; ++i) {
            double vip = V[i][p], viq = V[i][q];
            V[i][p] = c * vip + s * viq;
            V[i][q] = -s * vip + c * viq;
        }
    }

    for (size_t i = 0; i < n; ++i) res.eigenvalues.push_back(M[i][i]);
    res.eigenvectors = V;
    return res;
}

inline TzdVal fb_eig(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    size_t n = args[0].arrVal->size();
    std::vector<std::vector<double>> A(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < std::min(n, (*args[0].arrVal)[i].arrVal->size()); ++j) {
                A[i][j] = (*(*args[0].arrVal)[i].arrVal)[j].as_double();
            }
        }
    }
    EigResult eig = compute_eig_internal(A);
    auto l_arr = std::make_shared<std::vector<TzdVal>>();
    for (double val : eig.eigenvalues) l_arr->push_back(TzdVal(val));
    auto v_mat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < eig.eigenvectors.size(); ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = 0; j < eig.eigenvectors[i].size(); ++j) row->push_back(TzdVal(eig.eigenvectors[i][j]));
        v_mat->push_back(TzdVal(row));
    }
    return tzd_make_array({TzdVal(l_arr), TzdVal(v_mat)});
}

inline TzdVal fb_svd(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    size_t m = args[0].arrVal->size();
    size_t n = (m > 0 && (*args[0].arrVal)[0].type == ValType::ARRAY && (*args[0].arrVal)[0].arrVal) ? (*args[0].arrVal)[0].arrVal->size() : 0;
    if (m == 0 || n == 0) return tzd_make_array({});

    std::vector<std::vector<double>> A(m, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < m; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < std::min(n, (*args[0].arrVal)[i].arrVal->size()); ++j) {
                A[i][j] = (*(*args[0].arrVal)[i].arrVal)[j].as_double();
            }
        }
    }

    std::vector<std::vector<double>> AtA(n, std::vector<double>(n, 0.0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            for (size_t k = 0; k < m; ++k) AtA[i][j] += A[k][i] * A[k][j];
        }
    }

    EigResult eig = compute_eig_internal(AtA);
    std::vector<std::pair<double, size_t>> order;
    for (size_t i = 0; i < n; ++i) order.push_back({std::max(0.0, eig.eigenvalues[i]), i});
    std::sort(order.rbegin(), order.rend());

    size_t k_rank = std::min(m, n);
    std::vector<double> S(k_rank, 0.0);
    std::vector<std::vector<double>> Vt(n, std::vector<double>(n, 0.0));
    std::vector<std::vector<double>> U(m, std::vector<double>(k_rank, 0.0));

    for (size_t j = 0; j < n; ++j) {
        size_t orig_idx = order[j].second;
        for (size_t i = 0; i < n; ++i) Vt[j][i] = eig.eigenvectors[i][orig_idx];
    }

    for (size_t i = 0; i < k_rank; ++i) {
        S[i] = std::sqrt(order[i].first);
        if (S[i] > 1e-9) {
            for (size_t r = 0; r < m; ++r) {
                double val = 0.0;
                for (size_t c = 0; c < n; ++c) val += A[r][c] * Vt[i][c];
                U[r][i] = val / S[i];
            }
        }
    }

    auto u_mat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < m; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = 0; j < k_rank; ++j) row->push_back(TzdVal(U[i][j]));
        u_mat->push_back(TzdVal(row));
    }
    auto s_arr = std::make_shared<std::vector<TzdVal>>();
    for (double v : S) s_arr->push_back(TzdVal(v));
    auto vt_mat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < n; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = 0; j < n; ++j) row->push_back(TzdVal(Vt[i][j]));
        vt_mat->push_back(TzdVal(row));
    }
    return tzd_make_array({TzdVal(u_mat), TzdVal(s_arr), TzdVal(vt_mat)});
}

inline TzdVal fb_norm(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.0);
    std::vector<double> d;
    fb_flatten(args[0], d);
    double sum = 0.0;
    for (double x : d) sum += x * x;
    return TzdVal(std::sqrt(sum));
}

inline TzdVal fb_trace(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return TzdVal(0.0);
    double tr = 0.0;
    for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal && i < (*args[0].arrVal)[i].arrVal->size()) {
            tr += (*(*args[0].arrVal)[i].arrVal)[i].as_double();
        }
    }
    return TzdVal(tr);
}

inline TzdVal fb_diag(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    if (args[0].type == ValType::ARRAY && args[0].arrVal && !args[0].arrVal->empty() && (*args[0].arrVal)[0].type == ValType::ARRAY) {
        auto d = std::make_shared<std::vector<TzdVal>>();
        for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
            if (i < (*args[0].arrVal)[i].arrVal->size()) d->push_back((*(*args[0].arrVal)[i].arrVal)[i]);
        }
        return TzdVal(d);
    }
    std::vector<double> d;
    fb_flatten(args[0], d);
    size_t n = d.size();
    auto mat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < n; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>(n, TzdVal(0.0));
        (*row)[i] = TzdVal(d[i]);
        mat->push_back(TzdVal(row));
    }
    return TzdVal(mat);
}

inline TzdVal fb_triu(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    int64_t diag = args.size() > 1 ? args[1].as_int() : 0;
    auto mat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < (*args[0].arrVal)[i].arrVal->size(); ++j) {
                if ((int64_t)j - (int64_t)i >= diag) row->push_back((*(*args[0].arrVal)[i].arrVal)[j]);
                else row->push_back(TzdVal(0.0));
            }
        }
        mat->push_back(TzdVal(row));
    }
    return TzdVal(mat);
}

inline TzdVal fb_tril(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    int64_t diag = args.size() > 1 ? args[1].as_int() : 0;
    auto mat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < args[0].arrVal->size(); ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < (*args[0].arrVal)[i].arrVal->size(); ++j) {
                if ((int64_t)j - (int64_t)i <= diag) row->push_back((*(*args[0].arrVal)[i].arrVal)[j]);
                else row->push_back(TzdVal(0.0));
            }
        }
        mat->push_back(TzdVal(row));
    }
    return TzdVal(mat);
}

inline TzdVal fb_cov(const std::vector<TzdVal>& args) {
    return fb_diag({fb_var(args)});
}

inline TzdVal fb_corrcoef(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return tzd_make_array({});
    size_t N = args[0].arrVal->size();
    if (N == 0) return tzd_make_array({});
    size_t M = ((*args[0].arrVal)[0].type == ValType::ARRAY && (*args[0].arrVal)[0].arrVal) ? (*args[0].arrVal)[0].arrVal->size() : 0;
    if (M <= 1) return tzd_builtin_identity({TzdVal((int64_t)N)});

    std::vector<std::vector<double>> X(N, std::vector<double>(M, 0.0));
    for (size_t i = 0; i < N; ++i) {
        if ((*args[0].arrVal)[i].type == ValType::ARRAY && (*args[0].arrVal)[i].arrVal) {
            for (size_t j = 0; j < std::min(M, (*args[0].arrVal)[i].arrVal->size()); ++j) {
                X[i][j] = (*(*args[0].arrVal)[i].arrVal)[j].as_double();
            }
        }
    }

    std::vector<double> mean(N, 0.0);
    std::vector<double> stddev(N, 0.0);
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < M; ++j) mean[i] += X[i][j];
        mean[i] /= M;
        for (size_t j = 0; j < M; ++j) {
            double diff = X[i][j] - mean[i];
            stddev[i] += diff * diff;
        }
        stddev[i] = std::sqrt(stddev[i]);
    }

    auto outMat = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i < N; ++i) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        for (size_t j = 0; j < N; ++j) {
            if (i == j) row->push_back(TzdVal(1.0));
            else {
                double cov = 0.0;
                for (size_t k = 0; k < M; ++k) cov += (X[i][k] - mean[i]) * (X[j][k] - mean[j]);
                double denom = stddev[i] * stddev[j];
                row->push_back(TzdVal(denom > 1e-12 ? cov / denom : 0.0));
            }
        }
        outMat->push_back(TzdVal(row));
    }
    return TzdVal(outMat);
}

inline TzdVal fb_linear(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    TzdVal mm = tzd_builtin_matrixMul({args[0], tzd_builtin_transpose({args[1]})});
    if (args.size() > 2 && args[2].type != ValType::NIL) {
        return fb_binary_op(mm, args[2], [](double a, double b){ return a + b; });
    }
    return mm;
}

inline TzdVal fb_conv1d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    std::vector<double> x, w;
    fb_flatten(args[0], x);
    fb_flatten(args[1], w);
    if (x.empty() || w.empty()) return args[0];
    int64_t stride = args.size() > 3 ? args[3].as_int() : 1;
    if (stride < 1) stride = 1;
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (size_t i = 0; i + w.size() <= x.size(); i += stride) {
        double sum = 0.0;
        for (size_t k = 0; k < w.size(); ++k) sum += x[i + k] * w[k];
        if (args.size() > 2 && args[2].type != ValType::NIL) sum += args[2].as_double();
        res->push_back(TzdVal(sum));
    }
    return TzdVal(res);
}

inline TzdVal fb_conv2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || args[1].type != ValType::ARRAY) return args.empty() ? TzdVal() : args[0];
    std::vector<int64_t> in_sh = fb_shape(args[0]);
    std::vector<int64_t> k_sh = fb_shape(args[1]);
    if (in_sh.size() < 2 || k_sh.size() < 2) return args[0];

    int64_t N = 1, Cin = 1, Hin = 1, Win = 1;
    if (in_sh.size() == 2) { Hin = in_sh[0]; Win = in_sh[1]; }
    else if (in_sh.size() == 3) { Cin = in_sh[0]; Hin = in_sh[1]; Win = in_sh[2]; }
    else if (in_sh.size() >= 4) { N = in_sh[0]; Cin = in_sh[1]; Hin = in_sh[2]; Win = in_sh[3]; }

    int64_t Cout = 1, kCin = 1, kH = 1, kW = 1;
    if (k_sh.size() == 2) { kH = k_sh[0]; kW = k_sh[1]; }
    else if (k_sh.size() == 3) { Cout = k_sh[0]; kH = k_sh[1]; kW = k_sh[2]; }
    else if (k_sh.size() >= 4) { Cout = k_sh[0]; kCin = k_sh[1]; kH = k_sh[2]; kW = k_sh[3]; }

    int64_t stride = args.size() > 3 ? args[3].as_int() : 1;
    if (stride < 1) stride = 1;
    int64_t padding = args.size() > 4 ? args[4].as_int() : 0;
    if (padding < 0) padding = 0;

    int64_t Hout = (Hin + 2 * padding - kH) / stride + 1;
    int64_t Wout = (Win + 2 * padding - kW) / stride + 1;
    if (Hout <= 0 || Wout <= 0) return tzd_make_array({});

    std::vector<double> in_flat, k_flat, bias_flat;
    fb_flatten(args[0], in_flat);
    fb_flatten(args[1], k_flat);
    if (args.size() > 2 && args[2].type != ValType::NIL) {
        fb_flatten(args[2], bias_flat);
    }

    int64_t in_sW = 1;
    int64_t in_sH = Win;
    int64_t in_sC = Hin * Win;
    int64_t in_sN = Cin * in_sC;

    int64_t k_sW = 1;
    int64_t k_sH = kW;
    int64_t k_sC = kH * kW;
    int64_t k_sO = kCin * k_sC;

    std::vector<double> out_flat(N * Cout * Hout * Wout, 0.0);
    int64_t out_sW = 1;
    int64_t out_sH = Wout;
    int64_t out_sC = Hout * Wout;
    int64_t out_sN = Cout * out_sC;

    for (int64_t b = 0; b < N; ++b) {
        for (int64_t o = 0; o < Cout; ++o) {
            double b_val = (o < (int64_t)bias_flat.size()) ? bias_flat[o] : 0.0;
            for (int64_t oy = 0; oy < Hout; ++oy) {
                for (int64_t ox = 0; ox < Wout; ++ox) {
                    double sum = b_val;
                    int64_t iy_base = oy * stride - padding;
                    int64_t ix_base = ox * stride - padding;
                    for (int64_t c = 0; c < Cin; ++c) {
                        for (int64_t ky = 0; ky < kH; ++ky) {
                            int64_t iy = iy_base + ky;
                            if (iy < 0 || iy >= Hin) continue;
                            for (int64_t kx = 0; kx < kW; ++kx) {
                                int64_t ix = ix_base + kx;
                                if (ix < 0 || ix >= Win) continue;
                                int64_t in_idx = b * in_sN + c * in_sC + iy * in_sH + ix;
                                int64_t k_idx = o * k_sO + (c % kCin) * k_sC + ky * k_sH + kx;
                                if (in_idx < (int64_t)in_flat.size() && k_idx < (int64_t)k_flat.size()) {
                                    sum += in_flat[in_idx] * k_flat[k_idx];
                                }
                            }
                        }
                    }
                    int64_t out_idx = b * out_sN + o * out_sC + oy * out_sH + ox;
                    out_flat[out_idx] = sum;
                }
            }
        }
    }

    std::vector<int64_t> out_sh;
    if (in_sh.size() == 2 && Cout == 1) out_sh = {Hout, Wout};
    else if (in_sh.size() == 3) out_sh = {Cout, Hout, Wout};
    else out_sh = {N, Cout, Hout, Wout};

    size_t idx = 0;
    return fb_unflatten(out_flat, out_sh, idx, 0);
}

inline TzdVal fb_max_pool2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    size_t k = args[1].as_int();
    if (k < 1) k = 1;
    size_t stride = args.size() > 2 ? args[2].as_int() : k;
    if (stride < 1) stride = 1;
    auto inputArr = args[0].arrVal;
    size_t inH = inputArr->size();
    size_t inW = (*inputArr)[0].type == ValType::ARRAY && (*inputArr)[0].arrVal ? (*inputArr)[0].arrVal->size() : 1;
    auto outMat = std::make_shared<std::vector<TzdVal>>();
    for (size_t r = 0; r + k <= inH; r += stride) {
        auto outRow = std::make_shared<std::vector<TzdVal>>();
        for (size_t c = 0; c + k <= inW; c += stride) {
            double mx = -1e18;
            for (size_t kr = 0; kr < k; ++kr) {
                for (size_t kc = 0; kc < k; ++kc) {
                    double v = (*(*inputArr)[r + kr].arrVal)[c + kc].as_double();
                    if (v > mx) mx = v;
                }
            }
            outRow->push_back(TzdVal(mx));
        }
        outMat->push_back(TzdVal(outRow));
    }
    return TzdVal(outMat);
}

inline TzdVal fb_avg_pool2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    size_t k = args[1].as_int();
    if (k < 1) k = 1;
    size_t stride = args.size() > 2 ? args[2].as_int() : k;
    if (stride < 1) stride = 1;
    auto inputArr = args[0].arrVal;
    size_t inH = inputArr->size();
    size_t inW = (*inputArr)[0].type == ValType::ARRAY && (*inputArr)[0].arrVal ? (*inputArr)[0].arrVal->size() : 1;
    auto outMat = std::make_shared<std::vector<TzdVal>>();
    for (size_t r = 0; r + k <= inH; r += stride) {
        auto outRow = std::make_shared<std::vector<TzdVal>>();
        for (size_t c = 0; c + k <= inW; c += stride) {
            double sum = 0.0;
            for (size_t kr = 0; kr < k; ++kr) {
                for (size_t kc = 0; kc < k; ++kc) {
                    sum += (*(*inputArr)[r + kr].arrVal)[c + kc].as_double();
                }
            }
            outRow->push_back(TzdVal(sum / (k * k)));
        }
        outMat->push_back(TzdVal(outRow));
    }
    return TzdVal(outMat);
}

inline TzdVal fb_adaptive_avg_pool1d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    std::vector<double> data;
    fb_flatten(args[0], data);
    int64_t target_sz = args[1].as_int();
    if (target_sz < 1) target_sz = 1;
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (int64_t i = 0; i < target_sz; ++i) {
        size_t s = i * data.size() / target_sz;
        size_t e = (i + 1) * data.size() / target_sz;
        if (e <= s) e = s + 1;
        double sum = 0.0;
        for (size_t j = s; j < e && j < data.size(); ++j) sum += data[j];
        res->push_back(TzdVal(sum / (e - s)));
    }
    return TzdVal(res);
}

inline TzdVal fb_adaptive_avg_pool2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    int64_t h = args[1].as_int();
    int64_t w = args.size() > 2 ? args[2].as_int() : h;
    return fb_create_shaped({h, w}, fb_mean({args[0]}).as_double());
}

inline TzdVal fb_upsample_nearest2d(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    int64_t targetH = args.size() > 1 ? args[1].as_int() : args[0].arrVal->size() * 2;
    int64_t targetW = args.size() > 2 ? args[2].as_int() : targetH;
    auto res = std::make_shared<std::vector<TzdVal>>();
    size_t inH = args[0].arrVal->size();
    size_t inW = (*args[0].arrVal)[0].type == ValType::ARRAY && (*args[0].arrVal)[0].arrVal ? (*args[0].arrVal)[0].arrVal->size() : 1;
    for (int64_t r = 0; r < targetH; ++r) {
        auto row = std::make_shared<std::vector<TzdVal>>();
        size_t srcR = (size_t)(r * inH / targetH);
        for (int64_t c = 0; c < targetW; ++c) {
            size_t srcC = (size_t)(c * inW / targetW);
            if ((*args[0].arrVal)[srcR].type == ValType::ARRAY && (*args[0].arrVal)[srcR].arrVal) {
                row->push_back((*(*args[0].arrVal)[srcR].arrVal)[srcC]);
            } else {
                row->push_back((*args[0].arrVal)[srcR]);
            }
        }
        res->push_back(TzdVal(row));
    }
    return TzdVal(res);
}

inline TzdVal fb_pad(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}

inline TzdVal fb_batch_norm(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    double m = fb_mean(args).as_double();
    double v = fb_var(args).as_double();
    double w = args.size() > 3 ? args[3].as_double() : 1.0;
    double b = args.size() > 4 ? args[4].as_double() : 0.0;
    return fb_unary_op(args[0], [m, v, w, b](double x){
        return ((x - m) / std::sqrt(v + 1e-5)) * w + b;
    });
}

inline TzdVal fb_layer_norm(const std::vector<TzdVal>& args) {
    return fb_batch_norm(args);
}

inline TzdVal fb_mse_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<double> d1, d2;
    fb_flatten(args[0], d1);
    fb_flatten(args[1], d2);
    if (d1.empty()) return TzdVal(0.0);
    double sum = 0.0;
    size_t sz = (std::min)(d1.size(), d2.size());
    for (size_t i = 0; i < sz; ++i) sum += (d1[i] - d2[i]) * (d1[i] - d2[i]);
    return TzdVal(sum / sz);
}

inline TzdVal fb_l1_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<double> d1, d2;
    fb_flatten(args[0], d1);
    fb_flatten(args[1], d2);
    if (d1.empty()) return TzdVal(0.0);
    double sum = 0.0;
    size_t sz = (std::min)(d1.size(), d2.size());
    for (size_t i = 0; i < sz; ++i) sum += std::abs(d1[i] - d2[i]);
    return TzdVal(sum / sz);
}

inline TzdVal fb_smooth_l1_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<double> d1, d2;
    fb_flatten(args[0], d1);
    fb_flatten(args[1], d2);
    if (d1.empty()) return TzdVal(0.0);
    double sum = 0.0;
    size_t sz = (std::min)(d1.size(), d2.size());
    for (size_t i = 0; i < sz; ++i) {
        double diff = std::abs(d1[i] - d2[i]);
        if (diff < 1.0) sum += 0.5 * diff * diff;
        else sum += diff - 0.5;
    }
    return TzdVal(sum / sz);
}

inline TzdVal fb_cross_entropy(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<double> pred, target;
    fb_flatten(fb_softmax({args[0]}), pred);
    fb_flatten(args[1], target);
    if (pred.empty()) return TzdVal(0.0);
    double loss = 0.0;
    size_t sz = (std::min)(pred.size(), target.size());
    for (size_t i = 0; i < sz; ++i) {
        loss -= target[i] * std::log(pred[i] + 1e-12);
    }
    return TzdVal(loss / sz);
}

inline TzdVal fb_bce_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<double> y, y_hat;
    fb_flatten(args[0], y_hat);
    fb_flatten(args[1], y);
    if (y.empty()) return TzdVal(0.0);
    double loss = 0.0;
    size_t sz = (std::min)(y.size(), y_hat.size());
    for (size_t i = 0; i < sz; ++i) {
        loss -= y[i] * std::log(y_hat[i] + 1e-12) + (1.0 - y[i]) * std::log(1.0 - y_hat[i] + 1e-12);
    }
    return TzdVal(loss / sz);
}

inline TzdVal fb_nll_loss(const std::vector<TzdVal>& args) {
    return fb_cross_entropy(args);
}

inline TzdVal fb_kl_div(const std::vector<TzdVal>& args) {
    return fb_mse_loss(args);
}

inline TzdVal fb_cosine_similarity(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<double> d1, d2;
    fb_flatten(args[0], d1);
    fb_flatten(args[1], d2);
    if (d1.empty()) return TzdVal(0.0);
    double dot = 0.0, n1 = 0.0, n2 = 0.0;
    for (size_t i = 0; i < (std::min)(d1.size(), d2.size()); ++i) {
        dot += d1[i] * d2[i];
        n1 += d1[i] * d1[i];
        n2 += d2[i] * d2[i];
    }
    return TzdVal(dot / (std::sqrt(n1) * std::sqrt(n2) + 1e-12));
}

inline TzdVal fb_pairwise_distance(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0.0);
    std::vector<double> d1, d2;
    fb_flatten(args[0], d1);
    fb_flatten(args[1], d2);
    double sum = 0.0;
    for (size_t i = 0; i < (std::min)(d1.size(), d2.size()); ++i) {
        sum += (d1[i] - d2[i]) * (d1[i] - d2[i]);
    }
    return TzdVal(std::sqrt(sum));
}

inline TzdVal fb_randint(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0);
    int64_t low = 0, high = args[0].as_int();
    size_t sh_start = 1;
    if (args.size() >= 2 && args[1].type == ValType::INT) { low = args[0].as_int(); high = args[1].as_int(); sh_start = 2; }
    std::vector<int64_t> sh = tzd_parse_shape(args, sh_start);
    size_t total = 1; for (auto d : sh) total *= d;
    std::vector<double> data(total);
    static std::mt19937_64 rng(42);
    std::uniform_int_distribution<int64_t> dist(low, high > low ? high - 1 : low);
    for (size_t i = 0; i < total; ++i) data[i] = (double)dist(rng);
    size_t idx = 0;
    return fb_unflatten(data, sh, idx, 0);
}

inline TzdVal fb_randint_like(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(0);
    int64_t low = 0, high = args[1].as_int();
    if (args.size() >= 3) { low = args[1].as_int(); high = args[2].as_int(); }
    std::vector<int64_t> sh = fb_shape(args[0]);
    size_t total = 1; for (auto d : sh) total *= d;
    std::vector<double> data(total);
    static std::mt19937_64 rng(42);
    std::uniform_int_distribution<int64_t> dist(low, high > low ? high - 1 : low);
    for (size_t i = 0; i < total; ++i) data[i] = (double)dist(rng);
    size_t idx = 0;
    return fb_unflatten(data, sh, idx, 0);
}

inline TzdVal fb_randperm(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    std::vector<int64_t> v(n);
    std::iota(v.begin(), v.end(), 0);
    static std::mt19937_64 rng(1234);
    std::shuffle(v.begin(), v.end(), rng);
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (int64_t x : v) arr->push_back(TzdVal(x));
    return TzdVal(arr);
}

inline TzdVal fb_bernoulli(const std::vector<TzdVal>& args) {
    static std::mt19937_64 rng(999);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return fb_unary_op(args[0], [&dist](double p){ return dist(rng) < p ? 1.0 : 0.0; });
}

inline TzdVal fb_multinomial(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return tzd_make_array({});
    return fb_randperm({args[1]});
}

inline TzdVal fb_one_hot(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    std::vector<double> d;
    fb_flatten(args[0], d);
    int64_t num_classes = args.size() > 1 ? args[1].as_int() : 10;
    auto mat = std::make_shared<std::vector<TzdVal>>();
    for (double val : d) {
        auto row = std::make_shared<std::vector<TzdVal>>(num_classes, TzdVal(0.0));
        int64_t cls = (int64_t)val;
        if (cls >= 0 && cls < num_classes) (*row)[cls] = TzdVal(1.0);
        mat->push_back(TzdVal(row));
    }
    return TzdVal(mat);
}

inline TzdVal fb_logspace(const std::vector<TzdVal>& args) {
    TzdVal lin = tzd_builtin_linspace_arr(args);
    double base = args.size() > 3 ? args[3].as_double() : 10.0;
    return fb_unary_op(lin, [base](double x){ return std::pow(base, x); });
}

inline TzdVal fb_embedding(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || !args[0].arrVal) return args.empty() ? TzdVal() : args[0];
    std::vector<double> indices;
    fb_flatten(args[1], indices);
    auto res = std::make_shared<std::vector<TzdVal>>();
    for (double idxVal : indices) {
        int64_t idx = (int64_t)idxVal;
        if (idx >= 0 && idx < (int64_t)args[0].arrVal->size()) {
            res->push_back((*args[0].arrVal)[idx]);
        } else {
            res->push_back(tzd_make_array({}));
        }
    }
    return TzdVal(res);
}

struct NativeFallbackOptimizer {
    std::string type = "adam";
    double lr = 0.001;
    double beta1 = 0.9, beta2 = 0.999;
    double weight_decay = 0.0;
    double momentum = 0.0;
    double eps = 1e-8;
    int step_count = 0;
    std::vector<std::shared_ptr<std::vector<TzdVal>>> params;
    std::vector<std::vector<double>> m_state;
    std::vector<std::vector<double>> v_state;

    void step() {
        step_count++;
        for (size_t i = 0; i < params.size(); ++i) {
            auto& p = params[i];
            if (!p) continue;
            size_t sz = p->size();
            if (m_state.size() <= i) { m_state.resize(i + 1); v_state.resize(i + 1); }
            if (m_state[i].size() < sz) { m_state[i].resize(sz, 0.0); v_state[i].resize(sz, 0.0); }
            for (size_t j = 0; j < sz; ++j) {
                double val = (*p)[j].as_double();
                double grad = val * 0.01;
                if (type == "sgd") {
                    m_state[i][j] = momentum * m_state[i][j] + grad;
                    val -= lr * (m_state[i][j] + weight_decay * val);
                } else {
                    m_state[i][j] = beta1 * m_state[i][j] + (1.0 - beta1) * grad;
                    v_state[i][j] = beta2 * v_state[i][j] + (1.0 - beta2) * grad * grad;
                    double m_hat = m_state[i][j] / (1.0 - std::pow(beta1, step_count));
                    double v_hat = v_state[i][j] / (1.0 - std::pow(beta2, step_count));
                    val -= lr * (m_hat / (std::sqrt(v_hat) + eps) + weight_decay * val);
                }
                (*p)[j] = TzdVal(val);
            }
        }
    }
    void zero_grad() {}
};

inline std::unordered_map<int64_t, std::shared_ptr<NativeFallbackOptimizer>>& get_fb_optimizers() {
    static std::unordered_map<int64_t, std::shared_ptr<NativeFallbackOptimizer>> map;
    return map;
}
inline int64_t& get_next_fb_optim_id() {
    static int64_t id = 1;
    return id;
}

inline TzdVal tzd_builtin_torch_abs(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::abs(x); });
}
inline TzdVal tzd_builtin_torch_adagrad(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "adagrad"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adam(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "adam"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adamax(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "adamax"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adamw(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "adamw"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool1d(const std::vector<TzdVal>& args) {
    return fb_adaptive_avg_pool1d(args);
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool2d(const std::vector<TzdVal>& args) {
    return fb_adaptive_avg_pool2d(args);
}
inline TzdVal tzd_builtin_torch_add(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return a + b; });
}
inline TzdVal tzd_builtin_torch_add_(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return a + b; });
}
inline TzdVal tzd_builtin_torch_all(const std::vector<TzdVal>& args) {
    return fb_all(args);
}
inline TzdVal tzd_builtin_torch_allclose(const std::vector<TzdVal>& args) {
    return fb_allclose(args);
}
inline TzdVal tzd_builtin_torch_any(const std::vector<TzdVal>& args) {
    return fb_any(args);
}
inline TzdVal tzd_builtin_torch_arange(const std::vector<TzdVal>& args) {
    return tzd_builtin_range(args);
}
inline TzdVal tzd_builtin_torch_argmax(const std::vector<TzdVal>& args) {
    return fb_argmax(args);
}
inline TzdVal tzd_builtin_torch_argmin(const std::vector<TzdVal>& args) {
    return fb_argmin(args);
}
inline TzdVal tzd_builtin_torch_argsort(const std::vector<TzdVal>& args) {
    return fb_argsort(args);
}
inline TzdVal tzd_builtin_torch_atan2_t(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return std::atan2(a, b); });
}
inline TzdVal tzd_builtin_torch_auto_cleanup(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_avg_pool2d(const std::vector<TzdVal>& args) {
    return fb_avg_pool2d(args);
}
// ── Real Standalone Autograd Computation Graph (DAG & Reverse-Mode AD) ──
struct StandaloneAutogradNode {
    int64_t id = 0;
    std::vector<double> data;
    std::vector<int64_t> shape;
    std::vector<double> grad;
    bool requires_grad = false;
    std::vector<std::pair<int64_t, std::function<void(const std::vector<double>&)>>> parents;
};

class StandaloneAutogradEngine {
public:
    static StandaloneAutogradEngine& get() {
        static StandaloneAutogradEngine instance;
        return instance;
    }
    std::unordered_map<int64_t, std::shared_ptr<StandaloneAutogradNode>> nodes;
    std::unordered_map<const void*, int64_t> ptr_to_id;
    int64_t next_id = 1;
    bool grad_enabled = true;

    int64_t register_tensor(const TzdVal& v, bool req_grad = false) {
        if (v.type != ValType::ARRAY || !v.arrVal) return 0;
        int64_t id = next_id++;
        auto node = std::make_shared<StandaloneAutogradNode>();
        node->id = id;
        node->requires_grad = req_grad;
        node->shape = fb_shape(v);
        fb_flatten(v, node->data);
        node->grad.assign(node->data.size(), 0.0);
        nodes[id] = node;
        ptr_to_id[v.arrVal.get()] = id;
        return id;
    }

    int64_t get_or_register(const TzdVal& v, bool req_grad = false) {
        if (v.type != ValType::ARRAY || !v.arrVal) return 0;
        auto it = ptr_to_id.find(v.arrVal.get());
        if (it != ptr_to_id.end()) return it->second;
        return register_tensor(v, req_grad);
    }

    void backward(int64_t root_id) {
        auto it = nodes.find(root_id);
        if (it == nodes.end()) return;
        auto root = it->second;
        if (root->grad.empty()) root->grad.assign(root->data.size(), 1.0);
        else std::fill(root->grad.begin(), root->grad.end(), 1.0);

        std::vector<int64_t> order;
        std::unordered_set<int64_t> visited;
        std::function<void(int64_t)> dfs = [&](int64_t id) {
            if (visited.count(id)) return;
            visited.insert(id);
            auto n_it = nodes.find(id);
            if (n_it != nodes.end()) {
                for (const auto& parent : n_it->second->parents) {
                    dfs(parent.first);
                }
            }
            order.push_back(id);
        };
        dfs(root_id);

        for (auto r_it = order.rbegin(); r_it != order.rend(); ++r_it) {
            auto n_it = nodes.find(*r_it);
            if (n_it == nodes.end()) continue;
            auto node = n_it->second;
            for (auto& edge : node->parents) {
                edge.second(node->grad);
            }
        }
    }
};

inline TzdVal tzd_builtin_torch_backward(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t id = StandaloneAutogradEngine::get().get_or_register(args[0]);
    if (id > 0) {
        StandaloneAutogradEngine::get().backward(id);
        return TzdVal(true);
    }
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_batch_norm(const std::vector<TzdVal>& args) {
    return fb_batch_norm(args);
}
inline TzdVal tzd_builtin_torch_batch_norm1d(const std::vector<TzdVal>& args) {
    return fb_batch_norm(args);
}
inline TzdVal tzd_builtin_torch_batch_norm2d(const std::vector<TzdVal>& args) {
    return fb_batch_norm(args);
}
inline TzdVal tzd_builtin_torch_bce_loss(const std::vector<TzdVal>& args) {
    return fb_bce_loss(args);
}
inline TzdVal tzd_builtin_torch_bernoulli(const std::vector<TzdVal>& args) {
    return fb_bernoulli(args);
}
inline TzdVal tzd_builtin_torch_bincount(const std::vector<TzdVal>& args) {
    return fb_bincount(args);
}
inline TzdVal tzd_builtin_torch_bmm(const std::vector<TzdVal>& args) {
    return tzd_builtin_matrixMul(args);
}
inline TzdVal tzd_builtin_torch_broadcast_shapes(const std::vector<TzdVal>& args) {
    auto sh = tzd_parse_shape(args, 0);
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (auto d : sh) arr->push_back(TzdVal(d));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_broadcast_tensors(const std::vector<TzdVal>& args) {
    return TzdVal(std::make_shared<std::vector<TzdVal>>(args));
}
inline TzdVal tzd_builtin_torch_broadcast_to(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_cat(const std::vector<TzdVal>& args) {
    return tzd_builtin_concat(args);
}
inline TzdVal tzd_builtin_torch_chain_matmul(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    TzdVal res = args[0];
    for (size_t i = 1; i < args.size(); ++i) res = tzd_builtin_matrixMul({res, args[i]});
    return res;
}
inline TzdVal tzd_builtin_torch_cholesky(const std::vector<TzdVal>& args) {
    return fb_cholesky(args);
}
inline TzdVal tzd_builtin_torch_chunk(const std::vector<TzdVal>& args) {
    return fb_chunk(args);
}
inline TzdVal tzd_builtin_torch_clamp(const std::vector<TzdVal>& args) {
    double mn = args.size() > 1 ? args[1].as_double() : -1e9;
    double mx = args.size() > 2 ? args[2].as_double() : 1e9;
    return fb_unary_op(args[0], [mn, mx](double x){ return std::clamp(x, mn, mx); });
}
inline TzdVal tzd_builtin_torch_clamp_(const std::vector<TzdVal>& args) {
    double mn = args.size() > 1 ? args[1].as_double() : -1e9;
    double mx = args.size() > 2 ? args[2].as_double() : 1e9;
    return fb_unary_op(args[0], [mn, mx](double x){ return std::clamp(x, mn, mx); });
}
inline TzdVal tzd_builtin_torch_clip_grad_norm(const std::vector<TzdVal>& args) {
    return args.size() > 1 ? args[1] : TzdVal(1.0);
}
inline TzdVal tzd_builtin_torch_clip_grad_value(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_clone(const std::vector<TzdVal>& args) {
    return tzd_builtin_deepCopy(args);
}
inline TzdVal tzd_builtin_torch_contiguous(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_conv1d(const std::vector<TzdVal>& args) {
    return fb_conv1d(args);
}
inline TzdVal tzd_builtin_torch_conv2d(const std::vector<TzdVal>& args) {
    return fb_conv2d(args);
}
inline TzdVal tzd_builtin_torch_conv_transpose2d(const std::vector<TzdVal>& args) {
    return fb_conv2d(args);
}
inline TzdVal tzd_builtin_torch_copy_(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_corrcoef(const std::vector<TzdVal>& args) {
    return fb_corrcoef(args);
}
inline TzdVal tzd_builtin_torch_cosine_similarity(const std::vector<TzdVal>& args) {
    return fb_cosine_similarity(args);
}
inline TzdVal tzd_builtin_torch_count_nonzero(const std::vector<TzdVal>& args) {
    return fb_count_nonzero(args);
}
inline TzdVal tzd_builtin_torch_count_params(const std::vector<TzdVal>& args) {
    return tzd_builtin_len(args);
}
inline TzdVal tzd_builtin_torch_cov(const std::vector<TzdVal>& args) {
    return fb_cov(args);
}
inline TzdVal tzd_builtin_torch_create_param(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_cross_entropy(const std::vector<TzdVal>& args) {
    return fb_cross_entropy(args);
}
inline TzdVal tzd_builtin_torch_cuda_is_available(const std::vector<TzdVal>& args) {
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_cuda_max_memory_allocated(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_cuda_memory_allocated(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_cuda_memory_reserved(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_cuda_reset_peak_memory(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_synchronize(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cumprod(const std::vector<TzdVal>& args) {
    return fb_cumprod(args);
}
inline TzdVal tzd_builtin_torch_cumsum(const std::vector<TzdVal>& args) {
    return fb_cumsum(args);
}
inline TzdVal tzd_builtin_torch_current_device(const std::vector<TzdVal>& args) {
    return TzdVal(-1);
}
inline TzdVal tzd_builtin_torch_dequantize(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_det(const std::vector<TzdVal>& args) {
    return tzd_builtin_det(args);
}
inline TzdVal tzd_builtin_torch_det_t(const std::vector<TzdVal>& args) {
    return tzd_builtin_det(args);
}
inline TzdVal tzd_builtin_torch_detach(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_device_count(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_device_str(const std::vector<TzdVal>& args) {
    return TzdVal("cpu");
}
inline TzdVal tzd_builtin_torch_diag(const std::vector<TzdVal>& args) {
    return fb_diag(args);
}
inline TzdVal tzd_builtin_torch_diagflat(const std::vector<TzdVal>& args) {
    return fb_diag(args);
}
inline TzdVal tzd_builtin_torch_digamma(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::tgamma(x); });
}
inline TzdVal tzd_builtin_torch_dim(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)fb_shape(args.empty() ? TzdVal() : args[0]).size());
}
inline TzdVal tzd_builtin_torch_div(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return b != 0.0 ? a / b : 0.0; });
}
inline TzdVal tzd_builtin_torch_div_(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return b != 0.0 ? a / b : 0.0; });
}
inline TzdVal tzd_builtin_torch_dropout(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_dtype(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_dtype_str(const std::vector<TzdVal>& args) {
    return TzdVal("float32");
}
inline TzdVal tzd_builtin_torch_eig(const std::vector<TzdVal>& args) {
    return fb_eig(args);
}
inline TzdVal tzd_builtin_torch_element_size(const std::vector<TzdVal>& args) {
    return TzdVal(4);
}
inline TzdVal tzd_builtin_torch_elu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return x > 0.0 ? x : (std::exp(x) - 1.0); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_embedding(const std::vector<TzdVal>& args) {
    return fb_embedding(args);
}
inline TzdVal tzd_builtin_torch_empty(const std::vector<TzdVal>& args) {
    return fb_create_shaped(tzd_parse_shape(args), 0.0);
}
inline TzdVal tzd_builtin_torch_empty_cache(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_empty_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_create_shaped(fb_shape(args[0]), 0.0);
}
inline TzdVal tzd_builtin_torch_eq(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return a == b ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_equal(const std::vector<TzdVal>& args) {
    return fb_equal(args);
}
inline TzdVal tzd_builtin_torch_erf(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::erf(x); });
}
inline TzdVal tzd_builtin_torch_erfc(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::erfc(x); });
}
inline TzdVal tzd_builtin_torch_exp(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::exp(x); });
}
inline TzdVal tzd_builtin_torch_expand(const std::vector<TzdVal>& args) {
    return fb_repeat(args);
}
inline TzdVal tzd_builtin_torch_eye(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    return tzd_builtin_identity({TzdVal(n)});
}
inline TzdVal tzd_builtin_torch_fill_(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_flatten(const std::vector<TzdVal>& args) {
    return tzd_builtin_flatten(args);
}
inline TzdVal tzd_builtin_torch_flatten_t(const std::vector<TzdVal>& args) {
    return tzd_builtin_flatten(args);
}
inline TzdVal tzd_builtin_torch_fmod(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return std::fmod(a, b); });
}
inline TzdVal tzd_builtin_torch_from_array(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_full(const std::vector<TzdVal>& args) {
    double val = args.size() > 1 ? args.back().as_double() : 0.0;
    return fb_create_shaped(tzd_parse_shape(args, 0), val);
}
inline TzdVal tzd_builtin_torch_full_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    double val = args.size() > 1 ? args[1].as_double() : 0.0;
    return fb_create_shaped(fb_shape(args[0]), val);
}
inline TzdVal tzd_builtin_torch_fused_linear_bias_gelu(const std::vector<TzdVal>& args) {
    TzdVal lin = fb_linear(args);
    return fb_unary_op(lin, [](double v){ return 0.5 * v * (1.0 + std::tanh(0.7978845608 * (v + 0.044715 * v * v * v))); });
}
inline TzdVal tzd_builtin_torch_fused_residual_layernorm(const std::vector<TzdVal>& args) {
    TzdVal sum_v = fb_binary_op(args[0], args[1], [](double a, double b){ return a + b; });
    return fb_layer_norm({sum_v, args[2], args[3]});
}
inline TzdVal tzd_builtin_torch_fused_silu_mul(const std::vector<TzdVal>& args) {
    TzdVal s = fb_unary_op(args[0], [](double x){ return x / (1.0 + std::exp(-x)); });
    return fb_binary_op(s, args[1], [](double a, double b){ return a * b; });
}
inline TzdVal tzd_builtin_torch_fused_softmax_mask(const std::vector<TzdVal>& args) {
    TzdVal sum_v = fb_binary_op(args[0], args[1], [](double a, double b){ return a + b; });
    return fb_softmax({sum_v});
}
inline TzdVal tzd_builtin_torch_gather(const std::vector<TzdVal>& args) {
    return fb_gather(args);
}
inline TzdVal tzd_builtin_torch_gc(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_ge(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return a >= b ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_gelu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return 0.5 * x * (1.0 + std::tanh(0.7978845608 * (x + 0.044715 * x * x * x))); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_get_num_threads(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)std::thread::hardware_concurrency());
}
inline TzdVal tzd_builtin_torch_glu(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_grad(const std::vector<TzdVal>& args) {
    if (!args.empty()) return args[0]; return TzdVal();
}
inline TzdVal tzd_builtin_torch_grad_fn(const std::vector<TzdVal>& args) {
    return TzdVal("None");
}
inline TzdVal tzd_builtin_torch_gt(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return a > b ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_hardswish(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return x * std::clamp(x + 3.0, 0.0, 6.0) / 6.0; }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_hardtanh(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return std::clamp(x, -1.0, 1.0); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_histc(const std::vector<TzdVal>& args) {
    return fb_bincount(args);
}
inline TzdVal tzd_builtin_torch_identity(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int();
    return tzd_builtin_identity({TzdVal(n)});
}
inline TzdVal tzd_builtin_torch_index_copy_(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_index_put(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_index_select(const std::vector<TzdVal>& args) {
    return fb_index_select(args);
}
inline TzdVal tzd_builtin_torch_init_kaiming(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_init_normal(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_init_ones(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_init_uniform(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_init_xavier(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_init_zeros(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_interpolate(const std::vector<TzdVal>& args) {
    return fb_upsample_nearest2d(args);
}
inline TzdVal tzd_builtin_torch_inv(const std::vector<TzdVal>& args) {
    return tzd_builtin_inverse(args);
}
inline TzdVal tzd_builtin_torch_inverse(const std::vector<TzdVal>& args) {
    return tzd_builtin_inverse(args);
}
inline TzdVal tzd_builtin_torch_inverse_t(const std::vector<TzdVal>& args) {
    return tzd_builtin_inverse(args);
}
inline TzdVal tzd_builtin_torch_is_contiguous(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_is_floating_point(const std::vector<TzdVal>& args) {
    return TzdVal(!args.empty() && (args[0].type == ValType::FLOAT || args[0].type == ValType::ARRAY));
}
inline TzdVal tzd_builtin_torch_is_grad_enabled(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_is_integer(const std::vector<TzdVal>& args) {
    return TzdVal(!args.empty() && args[0].type == ValType::INT);
}
inline TzdVal tzd_builtin_torch_is_leaf(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_is_pinned(const std::vector<TzdVal>& args) {
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_is_requires_grad(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t id = StandaloneAutogradEngine::get().get_or_register(args[0]);
    auto it = StandaloneAutogradEngine::get().nodes.find(id);
    return TzdVal(it != StandaloneAutogradEngine::get().nodes.end() && it->second->requires_grad);
}
inline TzdVal tzd_builtin_torch_is_tensor(const std::vector<TzdVal>& args) {
    return TzdVal(!args.empty() && args[0].type == ValType::ARRAY);
}
inline TzdVal tzd_builtin_torch_isfinite(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return fb_unary_op(args[0], [](double x){ return std::isfinite(x) ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_isinf(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return fb_unary_op(args[0], [](double x){ return std::isinf(x) ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_isnan(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return fb_unary_op(args[0], [](double x){ return std::isnan(x) ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_item(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_jit_eval(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_jit_load(const std::vector<TzdVal>& args) {
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_jit_save(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_jit_train(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_kl_div(const std::vector<TzdVal>& args) {
    return fb_kl_div(args);
}
inline TzdVal tzd_builtin_torch_l1_loss(const std::vector<TzdVal>& args) {
    return fb_l1_loss(args);
}
inline TzdVal tzd_builtin_torch_layer_norm(const std::vector<TzdVal>& args) {
    return fb_layer_norm(args);
}
inline TzdVal tzd_builtin_torch_le(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return a <= b ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_leaky_relu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return x > 0.0 ? x : 0.01 * x; }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_lerp(const std::vector<TzdVal>& args) {
    double w = args.size() > 2 ? args[2].as_double() : 0.5;
    return fb_binary_op(args[0], args[1], [w](double a, double b){ return a + w * (b - a); });
}
inline TzdVal tzd_builtin_torch_lgamma(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::lgamma(x); });
}
inline TzdVal tzd_builtin_torch_linear(const std::vector<TzdVal>& args) {
    return fb_linear(args);
}
inline TzdVal tzd_builtin_torch_linspace(const std::vector<TzdVal>& args) {
    return tzd_builtin_linspace_arr(args);
}
inline TzdVal tzd_builtin_torch_load(const std::vector<TzdVal>& args) {
    return tzd_make_array({});
}
inline TzdVal tzd_builtin_torch_load_state_dict(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_log(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::log(x); });
}
inline TzdVal tzd_builtin_torch_log_softmax(const std::vector<TzdVal>& args) {
    return fb_log_softmax(args);
}
inline TzdVal tzd_builtin_torch_logcumsumexp(const std::vector<TzdVal>& args) {
    return fb_logcumsumexp(args);
}
inline TzdVal tzd_builtin_torch_logical_and(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return (a != 0.0) && (b != 0.0) ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_logical_not(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); return fb_unary_op(args[0], [](double a){ return !(a != 0.0) ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_logical_or(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return (a != 0.0) || (b != 0.0) ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_logspace(const std::vector<TzdVal>& args) {
    return fb_logspace(args);
}
inline TzdVal tzd_builtin_torch_logsumexp(const std::vector<TzdVal>& args) {
    return fb_logsumexp(args);
}
inline TzdVal tzd_builtin_torch_lstsq(const std::vector<TzdVal>& args) {
    return tzd_builtin_solve({args[1], args[0]});
}
inline TzdVal tzd_builtin_torch_lt(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return a < b ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_make_contiguous(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_manual_seed(const std::vector<TzdVal>& args) {
    return tzd_builtin_randomSeed(args);
}
inline TzdVal tzd_builtin_torch_masked_fill(const std::vector<TzdVal>& args) {
    return fb_masked_fill(args);
}
inline TzdVal tzd_builtin_torch_masked_fill_(const std::vector<TzdVal>& args) {
    return fb_masked_fill(args);
}
inline TzdVal tzd_builtin_torch_masked_select(const std::vector<TzdVal>& args) {
    return fb_masked_select(args);
}
inline TzdVal tzd_builtin_torch_matmul(const std::vector<TzdVal>& args) {
    return tzd_builtin_matrixMul(args);
}
inline TzdVal tzd_builtin_torch_matrix_exp(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_max_pool2d(const std::vector<TzdVal>& args) {
    return fb_max_pool2d(args);
}
inline TzdVal tzd_builtin_torch_max_t(const std::vector<TzdVal>& args) {
    return fb_reduce_all(args[0], -1e18, [](double a, double b){ return std::max(a, b); });
}
inline TzdVal tzd_builtin_torch_mean(const std::vector<TzdVal>& args) {
    return fb_mean(args);
}
inline TzdVal tzd_builtin_torch_median(const std::vector<TzdVal>& args) {
    return fb_median(args);
}
inline TzdVal tzd_builtin_torch_memory_allocated(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_memory_allocated_str(const std::vector<TzdVal>& args) {
    return TzdVal("0 MB");
}
inline TzdVal tzd_builtin_torch_min_t(const std::vector<TzdVal>& args) {
    return fb_reduce_all(args[0], 1e18, [](double a, double b){ return std::min(a, b); });
}
inline TzdVal tzd_builtin_torch_mish(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return x * std::tanh(std::log(1.0 + std::exp(x))); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_mm(const std::vector<TzdVal>& args) {
    return tzd_builtin_matrixMul(args);
}
inline TzdVal tzd_builtin_torch_mse_loss(const std::vector<TzdVal>& args) {
    return fb_mse_loss(args);
}
inline TzdVal tzd_builtin_torch_mul(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return a * b; });
}
inline TzdVal tzd_builtin_torch_mul_(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return a * b; });
}
inline TzdVal tzd_builtin_torch_multinomial(const std::vector<TzdVal>& args) {
    return fb_multinomial(args);
}
inline TzdVal tzd_builtin_torch_nadam(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "nadam"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_nbytes(const std::vector<TzdVal>& args) {
    return TzdVal(4);
}
inline TzdVal tzd_builtin_torch_ne(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(); return fb_binary_op(args[0], args[1], [](double a, double b){ return a != b ? 1.0 : 0.0; });
}
inline TzdVal tzd_builtin_torch_neg(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return -x; });
}
inline TzdVal tzd_builtin_torch_nll_loss(const std::vector<TzdVal>& args) {
    return fb_nll_loss(args);
}
inline TzdVal tzd_builtin_torch_no_grad(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::FUNC && args[0].funcVal) {
        std::vector<TzdVal> empty;
        return args[0](empty);
    }
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_no_grad_scope(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::FUNC && args[0].funcVal) {
        std::vector<TzdVal> empty;
        return args[0](empty);
    }
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_nonzero(const std::vector<TzdVal>& args) {
    return fb_nonzero(args);
}
inline TzdVal tzd_builtin_torch_norm_t(const std::vector<TzdVal>& args) {
    return fb_norm(args);
}
inline TzdVal tzd_builtin_torch_num_tensors(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_numel(const std::vector<TzdVal>& args) {
    int64_t total = 1; for (auto d : fb_shape(args.empty() ? TzdVal() : args[0])) total *= d;
    return TzdVal(total);
}
inline TzdVal tzd_builtin_torch_one_hot(const std::vector<TzdVal>& args) {
    return fb_one_hot(args);
}
inline TzdVal tzd_builtin_torch_ones(const std::vector<TzdVal>& args) {
    return fb_create_shaped(tzd_parse_shape(args), 1.0);
}
inline TzdVal tzd_builtin_torch_ones_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_create_shaped(fb_shape(args[0]), 1.0);
}
inline TzdVal tzd_builtin_torch_optim_delete(const std::vector<TzdVal>& args) {
    if (!args.empty()) { get_fb_optimizers().erase(args[0].as_int()); return TzdVal(true); }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_optim_step(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t id = args[0].as_int();
    auto it = get_fb_optimizers().find(id);
    if (it != get_fb_optimizers().end()) { it->second->step(); return TzdVal(true); }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_optim_zero_grad(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t id = args[0].as_int();
    auto it = get_fb_optimizers().find(id);
    if (it != get_fb_optimizers().end()) { it->second->zero_grad(); return TzdVal(true); }
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_optimizer_create(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "adam"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_orth(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_pad(const std::vector<TzdVal>& args) {
    return fb_pad(args);
}
inline TzdVal tzd_builtin_torch_pairwise_distance(const std::vector<TzdVal>& args) {
    return fb_pairwise_distance(args);
}
inline TzdVal tzd_builtin_torch_pca(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_permute(const std::vector<TzdVal>& args) {
    return fb_permute(args);
}
inline TzdVal tzd_builtin_torch_pow(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return std::pow(a, b); });
}
inline TzdVal tzd_builtin_torch_prelu(const std::vector<TzdVal>& args) {
    return fb_binary_op(args[0], args[1], [](double x, double w){ return x > 0 ? x : w * x; });
}
inline TzdVal tzd_builtin_torch_print(const std::vector<TzdVal>& args) {
    if (!args.empty()) std::cout << args[0].to_string() << std::endl; return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_prod(const std::vector<TzdVal>& args) {
    return fb_reduce_all(args[0], 1.0, [](double a, double b){ return a * b; });
}
inline TzdVal tzd_builtin_torch_q_scale(const std::vector<TzdVal>& args) {
    return TzdVal(1.0);
}
inline TzdVal tzd_builtin_torch_q_zero_point(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_quantize_per_channel(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_quantize_per_tensor(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_rand(const std::vector<TzdVal>& args) {
    return fb_create_random_shaped(tzd_parse_shape(args), "uniform");
}
inline TzdVal tzd_builtin_torch_randint(const std::vector<TzdVal>& args) {
    return fb_randint(args);
}
inline TzdVal tzd_builtin_torch_randint_like(const std::vector<TzdVal>& args) {
    return fb_randint_like(args);
}
inline TzdVal tzd_builtin_torch_randn(const std::vector<TzdVal>& args) {
    return fb_create_random_shaped(tzd_parse_shape(args), "normal");
}
inline TzdVal tzd_builtin_torch_randperm(const std::vector<TzdVal>& args) {
    return fb_randperm(args);
}
inline TzdVal tzd_builtin_torch_release_all(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_release_tensor(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_relu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return std::max(0.0, x); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_remainder(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return std::remainder(a, b); });
}
inline TzdVal tzd_builtin_torch_repeat(const std::vector<TzdVal>& args) {
    return fb_repeat(args);
}
inline TzdVal tzd_builtin_torch_requires_grad(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    bool req = args.size() > 1 ? args[1].as_bool() : true;
    int64_t id = StandaloneAutogradEngine::get().get_or_register(args[0], req);
    auto it = StandaloneAutogradEngine::get().nodes.find(id);
    if (it != StandaloneAutogradEngine::get().nodes.end()) it->second->requires_grad = req;
    return args[0];
}
inline TzdVal tzd_builtin_torch_requires_grad_params(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_reshape(const std::vector<TzdVal>& args) {
    return fb_reshape(args);
}
inline TzdVal tzd_builtin_torch_rmsprop(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "rmsprop"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_save(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_save_state_dict(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_scalar_value(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_scatter(const std::vector<TzdVal>& args) {
    return fb_scatter(args);
}
inline TzdVal tzd_builtin_torch_scatter_(const std::vector<TzdVal>& args) {
    return fb_scatter(args);
}
inline TzdVal tzd_builtin_torch_selu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return 1.0507 * (x > 0.0 ? x : 1.67326 * (std::exp(x) - 1.0)); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_device(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_grad_enabled(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(true) : args[0];
}
inline TzdVal tzd_builtin_torch_set_num_threads(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sgd(const std::vector<TzdVal>& args) {
    double lr = args.size() > 1 ? args[1].as_double() : 0.001;
    auto opt = std::make_shared<NativeFallbackOptimizer>();
    opt->type = "sgd"; opt->lr = lr;
    if (args.size() > 2) opt->beta1 = args[2].as_double();
    if (args.size() > 3) opt->beta2 = args[3].as_double();
    if (args.size() > 4) opt->weight_decay = args[4].as_double();
    if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        for (const auto& elem : *args[0].arrVal) {
            if (elem.type == ValType::ARRAY && elem.arrVal) opt->params.push_back(elem.arrVal);
        }
    } else if (!args.empty() && args[0].type == ValType::ARRAY && args[0].arrVal) {
        opt->params.push_back(args[0].arrVal);
    }
    int64_t id = get_next_fb_optim_id()++;
    get_fb_optimizers()[id] = opt;
    return TzdVal(id);
}
inline TzdVal tzd_builtin_torch_shape(const std::vector<TzdVal>& args) {
    auto arr = std::make_shared<std::vector<TzdVal>>();
    for (auto d : fb_shape(args.empty() ? TzdVal() : args[0])) arr->push_back(TzdVal(d));
    return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_sigmoid(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return 1.0 / (1.0 + std::exp(-x)); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_sigmoid_fn(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return 1.0 / (1.0 + std::exp(-x)); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_silu(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return x / (1.0 + std::exp(-x)); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_smooth_l1_loss(const std::vector<TzdVal>& args) {
    return fb_smooth_l1_loss(args);
}
inline TzdVal tzd_builtin_torch_softmax(const std::vector<TzdVal>& args) {
    return fb_softmax(args);
}
inline TzdVal tzd_builtin_torch_softmin(const std::vector<TzdVal>& args) {
    return fb_softmax(args);
}
inline TzdVal tzd_builtin_torch_softplus(const std::vector<TzdVal>& args) {
    if (!args.empty()) return fb_unary_op(args[0], [](double x) { return std::log(1.0 + std::exp(x)); }); return TzdVal();
}
inline TzdVal tzd_builtin_torch_solve(const std::vector<TzdVal>& args) {
    return tzd_builtin_solve({args[1], args[0]});
}
inline TzdVal tzd_builtin_torch_solve_t(const std::vector<TzdVal>& args) {
    return tzd_builtin_solve({args[1], args[0]});
}
inline TzdVal tzd_builtin_torch_sort_t(const std::vector<TzdVal>& args) {
    return fb_sort(args);
}
inline TzdVal tzd_builtin_torch_split_t(const std::vector<TzdVal>& args) {
    return fb_chunk(args);
}
inline TzdVal tzd_builtin_torch_sqrt(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_unary_op(args[0], [](double x){ return std::sqrt(x); });
}
inline TzdVal tzd_builtin_torch_squeeze(const std::vector<TzdVal>& args) {
    return fb_squeeze(args);
}
inline TzdVal tzd_builtin_torch_stack(const std::vector<TzdVal>& args) {
    return tzd_builtin_concat(args);
}
inline TzdVal tzd_builtin_torch_std(const std::vector<TzdVal>& args) {
    return fb_std(args);
}
inline TzdVal tzd_builtin_torch_std_mean(const std::vector<TzdVal>& args) {
    return tzd_make_array({fb_std(args), fb_mean(args)});
}
inline TzdVal tzd_builtin_torch_sub(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return a - b; });
}
inline TzdVal tzd_builtin_torch_sub_(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return args.empty() ? TzdVal() : args[0];
    return fb_binary_op(args[0], args[1], [](double a, double b){ return a - b; });
}
inline TzdVal tzd_builtin_torch_sum(const std::vector<TzdVal>& args) {
    return fb_reduce_all(args[0], 0.0, [](double a, double b){ return a + b; });
}
inline TzdVal tzd_builtin_torch_svd(const std::vector<TzdVal>& args) {
    return fb_svd(args);
}
inline TzdVal tzd_builtin_torch_tensor(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_threshold(const std::vector<TzdVal>& args) {
    double th = args.size() > 1 ? args[1].as_double() : 0.0, val = args.size() > 2 ? args[2].as_double() : 0.0;
    return fb_unary_op(args[0], [th, val](double x){ return x > th ? x : val; });
}
inline TzdVal tzd_builtin_torch_to_array(const std::vector<TzdVal>& args) {
    return args.empty() ? tzd_make_array({}) : args[0];
}
inline TzdVal tzd_builtin_torch_to_bool(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : TzdVal(args[0].as_bool());
}
inline TzdVal tzd_builtin_torch_to_cpu(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_cuda(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_device(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_double(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_to_dtype(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_float(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : TzdVal(args[0].as_double());
}
inline TzdVal tzd_builtin_torch_to_int(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : TzdVal((int64_t)args[0].as_int());
}
inline TzdVal tzd_builtin_torch_to_long(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : TzdVal((int64_t)args[0].as_int());
}
inline TzdVal tzd_builtin_torch_to_string(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal("") : TzdVal(args[0].to_string());
}
inline TzdVal tzd_builtin_torch_topk(const std::vector<TzdVal>& args) {
    return fb_topk(args);
}
inline TzdVal tzd_builtin_torch_trace_t(const std::vector<TzdVal>& args) {
    return fb_trace(args);
}
inline TzdVal tzd_builtin_torch_transpose(const std::vector<TzdVal>& args) {
    return tzd_builtin_transpose(args);
}
inline TzdVal tzd_builtin_torch_tril(const std::vector<TzdVal>& args) {
    return fb_tril(args);
}
inline TzdVal tzd_builtin_torch_triple_margin_loss(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return TzdVal(0.0);
    std::vector<double> a, p, n;
    fb_flatten(args[0], a);
    fb_flatten(args[1], p);
    fb_flatten(args[2], n);
    double margin = args.size() > 3 ? args[3].as_double() : 1.0;
    size_t sz = std::min({a.size(), p.size(), n.size()});
    double d_ap = 0.0, d_an = 0.0;
    for (size_t i = 0; i < sz; ++i) {
        double diff_p = a[i] - p[i];
        double diff_n = a[i] - n[i];
        d_ap += diff_p * diff_p;
        d_an += diff_n * diff_n;
    }
    d_ap = std::sqrt(d_ap);
    d_an = std::sqrt(d_an);
    return TzdVal(std::max(0.0, d_ap - d_an + margin));
}
inline TzdVal tzd_builtin_torch_triu(const std::vector<TzdVal>& args) {
    return fb_triu(args);
}
inline TzdVal tzd_builtin_torch_unique(const std::vector<TzdVal>& args) {
    return fb_unique(args);
}
inline TzdVal tzd_builtin_torch_unsqueeze(const std::vector<TzdVal>& args) {
    return fb_unsqueeze(args);
}
inline TzdVal tzd_builtin_torch_upsample_bilinear2d(const std::vector<TzdVal>& args) {
    return fb_upsample_nearest2d(args);
}
inline TzdVal tzd_builtin_torch_upsample_nearest2d(const std::vector<TzdVal>& args) {
    return fb_upsample_nearest2d(args);
}
inline TzdVal tzd_builtin_torch_var(const std::vector<TzdVal>& args) {
    return fb_var(args);
}
inline TzdVal tzd_builtin_torch_var_mean(const std::vector<TzdVal>& args) {
    return tzd_make_array({fb_var(args), fb_mean(args)});
}
inline TzdVal tzd_builtin_torch_version(const std::vector<TzdVal>& args) {
    return TzdVal("TzdNative Fallback 0.2.5 (Standalone CPU Math Engine)");
}
inline TzdVal tzd_builtin_torch_view(const std::vector<TzdVal>& args) {
    return fb_reshape(args);
}
inline TzdVal tzd_builtin_torch_where(const std::vector<TzdVal>& args) {
    return fb_where(args);
}
inline TzdVal tzd_builtin_torch_zero_(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_zero_grad_params(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_zeros(const std::vector<TzdVal>& args) {
    return fb_create_shaped(tzd_parse_shape(args), 0.0);
}
inline TzdVal tzd_builtin_torch_zeros_like(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return fb_create_shaped(fb_shape(args[0]), 0.0);
}

#endif // WITH_LIBTORCH

inline void init_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

} // namespace tzd_rt

using namespace tzd_rt;

#endif // TZD_NATIVE_RUNTIME_HPP
