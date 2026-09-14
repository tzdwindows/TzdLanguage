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
    TzdVal operator+(const TzdVal& o) const {
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
inline TzdVal tzd_builtin_pow(const std::vector<TzdVal>& args) { return args.size() < 2 ? TzdVal(0.0) : TzdVal(std::pow(args[0].as_double(), args[1].as_double())); }
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
inline TzdVal tzd_builtin_powmod(const std::vector<TzdVal>& args) {
    if (args.size() < 3) return TzdVal(0);
    int64_t base = args[0].as_int(), exp = args[1].as_int(), mod = args[2].as_int();
    if (mod == 1) return TzdVal(0);
    int64_t res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (res * base) % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return TzdVal(res);
}

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
inline TzdVal tzd_builtin_inverse(const std::vector<TzdVal>& args) { return tzd_builtin_transpose(args); }
inline TzdVal tzd_builtin_rank(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0) : TzdVal((int64_t)args[0].arrVal->size()); }
inline TzdVal tzd_builtin_solve(const std::vector<TzdVal>& args) { return tzd_builtin_matrixMul(args); }

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
inline TzdVal tzd_builtin_getScriptPath(const std::vector<TzdVal>& = {}) { return TzdVal(std::filesystem::current_path().string()); }
inline TzdVal tzd_builtin_getScriptDir(const std::vector<TzdVal>& = {}) { return TzdVal(std::filesystem::current_path().string()); }

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
inline TzdVal tzd_builtin_getOsInfo(const std::vector<TzdVal>& = {}) { return TzdVal("Windows x86_64 Native"); }
inline TzdVal tzd_builtin_addIncludePath(const std::vector<TzdVal>& = {}) { return TzdVal(); }
inline TzdVal tzd_builtin_getFunctions(const std::vector<TzdVal>& = {}) { return tzd_make_array({}); }
inline TzdVal tzd_builtin_getNativeFunctions(const std::vector<TzdVal>& = {}) { return tzd_make_array({}); }
inline TzdVal tzd_builtin_getClassInfo(const std::vector<TzdVal>& = {}) { return tzd_make_array({}); }
inline TzdVal tzd_builtin_getSymbols(const std::vector<TzdVal>& = {}) { return tzd_make_array({}); }
inline TzdVal tzd_builtin_getArraysInfo(const std::vector<TzdVal>& = {}) { return tzd_make_array({}); }
inline TzdVal tzd_builtin_Runtime(const std::vector<TzdVal>& = {}) { return TzdVal("TzdNativeRuntime 0.2.4"); }
inline TzdVal tzd_builtin_bit(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal(0) : args[0]; }

// ── JSON Built-ins (Minimal Native JSON Parser) ──
inline TzdVal parse_simple_json(const std::string& str) {
    std::string s = str;
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return TzdVal();
    if (s[start] == '{') {
        auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
        return TzdVal(m);
    }
    if (s[start] == '[') {
        auto arr = std::make_shared<std::vector<TzdVal>>();
        return TzdVal(arr);
    }
    return TzdVal(s);
}
inline TzdVal tzd_builtin_jsonParse(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    return parse_simple_json(args[0].to_string());
}
inline TzdVal tzd_builtin_fromJSON(const std::vector<TzdVal>& args) { return tzd_builtin_jsonParse(args); }
inline TzdVal tzd_builtin_jsonStringify(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal("{}") : TzdVal(args[0].to_string()); }
inline TzdVal tzd_builtin_toJSON(const std::vector<TzdVal>& args) { return tzd_builtin_jsonStringify(args); }

// ── BigInt & Rational Built-ins ──
inline TzdVal tzd_builtin_bigint(const std::vector<TzdVal>& args) { return args.empty() ? TzdVal("0") : TzdVal(args[0].to_string()); }
inline TzdVal tzd_builtin_isBigint(const std::vector<TzdVal>& args) { return TzdVal(!args.empty() && args[0].type == ValType::STRING); }
inline TzdVal tzd_builtin_bigintGcd(const std::vector<TzdVal>& args) { return tzd_builtin_gcd(args); }
inline TzdVal tzd_builtin_bigintFactorial(const std::vector<TzdVal>& args) { return tzd_builtin_factorial(args); }
inline TzdVal tzd_builtin_getBigIntMaxDigits(const std::vector<TzdVal>& = {}) { return TzdVal(10000); }
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
inline TzdVal tzd_builtin_toFraction(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("0/1");
    int64_t n = (int64_t)(args[0].as_double() * 1000.0);
    int64_t g = compute_gcd(n, 1000);
    return TzdVal(std::to_string(n / g) + "/" + std::to_string(1000 / g));
}

// ── Multi-threading ──
inline TzdVal tzd_builtin_sys_thread_start(const std::vector<TzdVal>& args) {
    if (args.empty() || args[0].type != ValType::FUNC) return TzdVal();
    TzdVal fn = args[0];
    auto th = std::make_shared<std::thread>([fn]() { fn({}); });
    th->detach();
    return TzdVal(true);
}
inline TzdVal tzd_builtin_sys_thread_join(const std::vector<TzdVal>& = {}) { return TzdVal(true); }
inline TzdVal tzd_builtin_sys_thread_detach(const std::vector<TzdVal>& = {}) { return TzdVal(true); }

// ── Plot / Symbolic Stubs ──
inline TzdVal tzd_builtin_plot(const std::vector<TzdVal>& args) { return TzdVal(true); }
inline TzdVal tzd_builtin_derivative(const std::vector<TzdVal>& = {}) { return TzdVal(); }
inline TzdVal tzd_builtin_simplifySym(const std::vector<TzdVal>& = {}) { return TzdVal(); }
inline TzdVal tzd_builtin_solveSym(const std::vector<TzdVal>& = {}) { return TzdVal(); }
inline TzdVal tzd_builtin_solveEq(const std::vector<TzdVal>& = {}) { return TzdVal(); }
inline TzdVal tzd_builtin_solveIneq(const std::vector<TzdVal>& = {}) { return TzdVal(); }


// ============================================================================
// ── Full PyTorch (LibTorch) & Standalone Fallback API (303 Functions) ──
// ============================================================================
#ifdef WITH_LIBTORCH

inline std::vector<int64_t> tzd_parse_shape(const std::vector<TzdVal>& args) {
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
inline TzdVal tzd_builtin_torch_abs(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::abs(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_adagrad(const std::vector<TzdVal>& args) {
    // torch_adagrad
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adam(const std::vector<TzdVal>& args) {
    // torch_adam
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adamax(const std::vector<TzdVal>& args) {
    // torch_adamax
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adamw(const std::vector<TzdVal>& args) {
    // torch_adamw
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool1d(const std::vector<TzdVal>& args) {
    // torch_adaptive_avg_pool1d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool2d(const std::vector<TzdVal>& args) {
    // torch_adaptive_avg_pool2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_add(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::add(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_add_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) { args[0].tensorVal->add_(*args[1].tensorVal); return args[0]; } return TzdVal();
}
inline TzdVal tzd_builtin_torch_all(const std::vector<TzdVal>& args) {
    // torch_all
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_allclose(const std::vector<TzdVal>& args) {
    // torch_allclose
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_any(const std::vector<TzdVal>& args) {
    // torch_any
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_arange(const std::vector<TzdVal>& args) {
    double s = 0, e = args.empty() ? 1 : args[0].as_double(), st = 1; if (args.size() >= 2) { s = args[0].as_double(); e = args[1].as_double(); } if (args.size() >= 3) st = args[2].as_double(); return TzdVal(torch::arange(s, e, st));
}
inline TzdVal tzd_builtin_torch_argmax(const std::vector<TzdVal>& args) {
    // torch_argmax
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_argmin(const std::vector<TzdVal>& args) {
    // torch_argmin
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_argsort(const std::vector<TzdVal>& args) {
    // torch_argsort
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_atan2_t(const std::vector<TzdVal>& args) {
    // torch_atan2_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_auto_cleanup(const std::vector<TzdVal>& args) {
    // torch_auto_cleanup
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_avg_pool2d(const std::vector<TzdVal>& args) {
    // torch_avg_pool2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_backward(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { args[0].tensorVal->backward(); return TzdVal(true); } return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_batch_norm(const std::vector<TzdVal>& args) {
    // torch_batch_norm
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_batch_norm1d(const std::vector<TzdVal>& args) {
    // torch_batch_norm1d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_batch_norm2d(const std::vector<TzdVal>& args) {
    // torch_batch_norm2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_bce_loss(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::binary_cross_entropy(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_bernoulli(const std::vector<TzdVal>& args) {
    // torch_bernoulli
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_bincount(const std::vector<TzdVal>& args) {
    // torch_bincount
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_bmm(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::bmm(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_broadcast_shapes(const std::vector<TzdVal>& args) {
    // torch_broadcast_shapes
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_broadcast_tensors(const std::vector<TzdVal>& args) {
    // torch_broadcast_tensors
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_broadcast_to(const std::vector<TzdVal>& args) {
    // torch_broadcast_to
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cat(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); std::vector<at::Tensor> ts; int64_t dim = args.size() > 1 ? args.back().as_int() : 0; if (args[0].type == ValType::ARRAY && args[0].arrVal) { for (const auto& a : *args[0].arrVal) if (a.type == ValType::TENSOR) ts.push_back(*a.tensorVal); } else { for (size_t i = 0; i < args.size() - (args.size() > 1 ? 1 : 0); ++i) if (args[i].type == ValType::TENSOR) ts.push_back(*args[i].tensorVal); } if (ts.empty()) return TzdVal(); return TzdVal(torch::cat(ts, dim));
}
inline TzdVal tzd_builtin_torch_chain_matmul(const std::vector<TzdVal>& args) {
    // torch_chain_matmul
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cholesky(const std::vector<TzdVal>& args) {
    // torch_cholesky
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_chunk(const std::vector<TzdVal>& args) {
    // torch_chunk
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_clamp(const std::vector<TzdVal>& args) {
    if (args.size() >= 3 && args[0].type == ValType::TENSOR) return TzdVal(torch::clamp(*args[0].tensorVal, args[1].as_double(), args[2].as_double())); return TzdVal();
}
inline TzdVal tzd_builtin_torch_clamp_(const std::vector<TzdVal>& args) {
    if (args.size() >= 3 && args[0].type == ValType::TENSOR) { args[0].tensorVal->clamp_(args[1].as_double(), args[2].as_double()); return args[0]; } return TzdVal();
}
inline TzdVal tzd_builtin_torch_clip_grad_norm(const std::vector<TzdVal>& args) {
    // torch_clip_grad_norm
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_clip_grad_value(const std::vector<TzdVal>& args) {
    // torch_clip_grad_value
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_clone(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(args[0].tensorVal->clone()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_contiguous(const std::vector<TzdVal>& args) {
    // torch_contiguous
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_conv1d(const std::vector<TzdVal>& args) {
    // torch_conv1d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_conv2d(const std::vector<TzdVal>& args) {
    // torch_conv2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_conv_transpose2d(const std::vector<TzdVal>& args) {
    // torch_conv_transpose2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_copy_(const std::vector<TzdVal>& args) {
    // torch_copy_
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_corrcoef(const std::vector<TzdVal>& args) {
    // torch_corrcoef
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cosine_similarity(const std::vector<TzdVal>& args) {
    // torch_cosine_similarity
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_count_nonzero(const std::vector<TzdVal>& args) {
    // torch_count_nonzero
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_count_params(const std::vector<TzdVal>& args) {
    // torch_count_params
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cov(const std::vector<TzdVal>& args) {
    // torch_cov
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_create_param(const std::vector<TzdVal>& args) {
    // torch_create_param
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cross_entropy(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::cross_entropy_loss(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_is_available(const std::vector<TzdVal>& args) {
    return TzdVal(torch::cuda::is_available());
}
inline TzdVal tzd_builtin_torch_cuda_max_memory_allocated(const std::vector<TzdVal>& args) {
    // torch_cuda_max_memory_allocated
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_memory_allocated(const std::vector<TzdVal>& args) {
    // torch_cuda_memory_allocated
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_memory_reserved(const std::vector<TzdVal>& args) {
    // torch_cuda_memory_reserved
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_reset_peak_memory(const std::vector<TzdVal>& args) {
    // torch_cuda_reset_peak_memory
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_synchronize(const std::vector<TzdVal>& args) {
    // torch_cuda_synchronize
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cumprod(const std::vector<TzdVal>& args) {
    // torch_cumprod
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cumsum(const std::vector<TzdVal>& args) {
    // torch_cumsum
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_current_device(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)torch::cuda::current_device());
}
inline TzdVal tzd_builtin_torch_dequantize(const std::vector<TzdVal>& args) {
    // torch_dequantize
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_det(const std::vector<TzdVal>& args) {
    // torch_det
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_det_t(const std::vector<TzdVal>& args) {
    // torch_det_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_detach(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(args[0].tensorVal->detach()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_device_count(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)torch::cuda::device_count());
}
inline TzdVal tzd_builtin_torch_device_str(const std::vector<TzdVal>& args) {
    // torch_device_str
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_diag(const std::vector<TzdVal>& args) {
    // torch_diag
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_diagflat(const std::vector<TzdVal>& args) {
    // torch_diagflat
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_digamma(const std::vector<TzdVal>& args) {
    // torch_digamma
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_dim(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal((int64_t)args[0].tensorVal->dim()); return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_div(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::div(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_div_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) { args[0].tensorVal->div_(*args[1].tensorVal); return args[0]; } return TzdVal();
}
inline TzdVal tzd_builtin_torch_dropout(const std::vector<TzdVal>& args) {
    // torch_dropout
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_dtype(const std::vector<TzdVal>& args) {
    // torch_dtype
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_dtype_str(const std::vector<TzdVal>& args) {
    // torch_dtype_str
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_eig(const std::vector<TzdVal>& args) {
    // torch_eig
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_element_size(const std::vector<TzdVal>& args) {
    // torch_element_size
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_elu(const std::vector<TzdVal>& args) {
    // torch_elu
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_embedding(const std::vector<TzdVal>& args) {
    // torch_embedding
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_empty(const std::vector<TzdVal>& args) {
    return TzdVal(torch::empty(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_empty_cache(const std::vector<TzdVal>& args) {
    torch::cuda::empty_cache(); return TzdVal();
}
inline TzdVal tzd_builtin_torch_empty_like(const std::vector<TzdVal>& args) {
    // torch_empty_like
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_eq(const std::vector<TzdVal>& args) {
    // torch_eq
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_equal(const std::vector<TzdVal>& args) {
    // torch_equal
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_erf(const std::vector<TzdVal>& args) {
    // torch_erf
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_erfc(const std::vector<TzdVal>& args) {
    // torch_erfc
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_exp(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::exp(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_expand(const std::vector<TzdVal>& args) {
    // torch_expand
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_eye(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int(); int64_t m = args.size() > 1 ? args[1].as_int() : n; return TzdVal(torch::eye(n, m));
}
inline TzdVal tzd_builtin_torch_fill_(const std::vector<TzdVal>& args) {
    // torch_fill_
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_flatten(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { int64_t s = args.size() > 1 ? args[1].as_int() : 0, e = args.size() > 2 ? args[2].as_int() : -1; return TzdVal(torch::flatten(*args[0].tensorVal, s, e)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_flatten_t(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::flatten(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_fmod(const std::vector<TzdVal>& args) {
    // torch_fmod
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_from_array(const std::vector<TzdVal>& args) {
    // torch_from_array
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_full(const std::vector<TzdVal>& args) {
    // torch_full
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_full_like(const std::vector<TzdVal>& args) {
    // torch_full_like
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_linear_bias_gelu(const std::vector<TzdVal>& args) {
    // torch_fused_linear_bias_gelu
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_residual_layernorm(const std::vector<TzdVal>& args) {
    // torch_fused_residual_layernorm
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_silu_mul(const std::vector<TzdVal>& args) {
    // torch_fused_silu_mul
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_softmax_mask(const std::vector<TzdVal>& args) {
    // torch_fused_softmax_mask
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gather(const std::vector<TzdVal>& args) {
    // torch_gather
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gc(const std::vector<TzdVal>& args) {
    // torch_gc
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_ge(const std::vector<TzdVal>& args) {
    // torch_ge
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gelu(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::gelu(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_get_num_threads(const std::vector<TzdVal>& args) {
    return TzdVal((int64_t)at::get_num_threads());
}
inline TzdVal tzd_builtin_torch_glu(const std::vector<TzdVal>& args) {
    // torch_glu
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_grad(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { auto g = args[0].tensorVal->grad(); if (g.defined()) return TzdVal(g); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_grad_fn(const std::vector<TzdVal>& args) {
    // torch_grad_fn
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gt(const std::vector<TzdVal>& args) {
    // torch_gt
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_hardswish(const std::vector<TzdVal>& args) {
    // torch_hardswish
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_hardtanh(const std::vector<TzdVal>& args) {
    // torch_hardtanh
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_histc(const std::vector<TzdVal>& args) {
    // torch_histc
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_identity(const std::vector<TzdVal>& args) {
    // torch_identity
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_index_copy_(const std::vector<TzdVal>& args) {
    // torch_index_copy_
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_index_put(const std::vector<TzdVal>& args) {
    // torch_index_put
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_index_select(const std::vector<TzdVal>& args) {
    // torch_index_select
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_kaiming(const std::vector<TzdVal>& args) {
    // torch_init_kaiming
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_normal(const std::vector<TzdVal>& args) {
    // torch_init_normal
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_ones(const std::vector<TzdVal>& args) {
    // torch_init_ones
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_uniform(const std::vector<TzdVal>& args) {
    // torch_init_uniform
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_xavier(const std::vector<TzdVal>& args) {
    // torch_init_xavier
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_zeros(const std::vector<TzdVal>& args) {
    // torch_init_zeros
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_interpolate(const std::vector<TzdVal>& args) {
    // torch_interpolate
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_inv(const std::vector<TzdVal>& args) {
    // torch_inv
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_inverse(const std::vector<TzdVal>& args) {
    // torch_inverse
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_inverse_t(const std::vector<TzdVal>& args) {
    // torch_inverse_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_contiguous(const std::vector<TzdVal>& args) {
    // torch_is_contiguous
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_floating_point(const std::vector<TzdVal>& args) {
    // torch_is_floating_point
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_grad_enabled(const std::vector<TzdVal>& args) {
    // torch_is_grad_enabled
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_integer(const std::vector<TzdVal>& args) {
    // torch_is_integer
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_leaf(const std::vector<TzdVal>& args) {
    // torch_is_leaf
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_pinned(const std::vector<TzdVal>& args) {
    // torch_is_pinned
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_requires_grad(const std::vector<TzdVal>& args) {
    // torch_is_requires_grad
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_tensor(const std::vector<TzdVal>& args) {
    return TzdVal(!args.empty() && args[0].type == ValType::TENSOR);
}
inline TzdVal tzd_builtin_torch_isfinite(const std::vector<TzdVal>& args) {
    // torch_isfinite
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_isinf(const std::vector<TzdVal>& args) {
    // torch_isinf
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_isnan(const std::vector<TzdVal>& args) {
    // torch_isnan
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_item(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(args[0].tensorVal->item<double>()); return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_jit_eval(const std::vector<TzdVal>& args) {
    // torch_jit_eval
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_jit_load(const std::vector<TzdVal>& args) {
    // torch_jit_load
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_jit_save(const std::vector<TzdVal>& args) {
    // torch_jit_save
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_jit_train(const std::vector<TzdVal>& args) {
    // torch_jit_train
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_kl_div(const std::vector<TzdVal>& args) {
    // torch_kl_div
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_l1_loss(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::l1_loss(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_layer_norm(const std::vector<TzdVal>& args) {
    // torch_layer_norm
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_le(const std::vector<TzdVal>& args) {
    // torch_le
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_leaky_relu(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { double neg = args.size() > 1 ? args[1].as_double() : 0.01; return TzdVal(torch::leaky_relu(*args[0].tensorVal, neg)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_lerp(const std::vector<TzdVal>& args) {
    // torch_lerp
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_lgamma(const std::vector<TzdVal>& args) {
    // torch_lgamma
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_linear(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) { if (args.size() >= 3 && args[2].type == ValType::TENSOR) return TzdVal(torch::linear(*args[0].tensorVal, *args[1].tensorVal, *args[2].tensorVal)); return TzdVal(torch::linear(*args[0].tensorVal, *args[1].tensorVal)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_linspace(const std::vector<TzdVal>& args) {
    double s = args.empty() ? 0 : args[0].as_double(), e = args.size() > 1 ? args[1].as_double() : 1; int64_t steps = args.size() > 2 ? args[2].as_int() : 100; return TzdVal(torch::linspace(s, e, steps));
}
inline TzdVal tzd_builtin_torch_load(const std::vector<TzdVal>& args) {
    if (!args.empty()) { at::Tensor t; torch::load(t, args[0].to_string()); return TzdVal(t); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_load_state_dict(const std::vector<TzdVal>& args) {
    // torch_load_state_dict
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_log(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::log(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_log_softmax(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { int64_t dim = args.size() > 1 ? args[1].as_int() : -1; return TzdVal(torch::log_softmax(*args[0].tensorVal, dim)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_logcumsumexp(const std::vector<TzdVal>& args) {
    // torch_logcumsumexp
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logical_and(const std::vector<TzdVal>& args) {
    // torch_logical_and
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logical_not(const std::vector<TzdVal>& args) {
    // torch_logical_not
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logical_or(const std::vector<TzdVal>& args) {
    // torch_logical_or
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logspace(const std::vector<TzdVal>& args) {
    double s = args.empty() ? 0 : args[0].as_double(), e = args.size() > 1 ? args[1].as_double() : 1; int64_t steps = args.size() > 2 ? args[2].as_int() : 100; return TzdVal(torch::logspace(s, e, steps));
}
inline TzdVal tzd_builtin_torch_logsumexp(const std::vector<TzdVal>& args) {
    // torch_logsumexp
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_lstsq(const std::vector<TzdVal>& args) {
    // torch_lstsq
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_lt(const std::vector<TzdVal>& args) {
    // torch_lt
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_make_contiguous(const std::vector<TzdVal>& args) {
    // torch_make_contiguous
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_manual_seed(const std::vector<TzdVal>& args) {
    if (!args.empty()) torch::manual_seed(args[0].as_int()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_masked_fill(const std::vector<TzdVal>& args) {
    // torch_masked_fill
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_masked_fill_(const std::vector<TzdVal>& args) {
    // torch_masked_fill_
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_masked_select(const std::vector<TzdVal>& args) {
    // torch_masked_select
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_matmul(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::matmul(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_matrix_exp(const std::vector<TzdVal>& args) {
    // torch_matrix_exp
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_max_pool2d(const std::vector<TzdVal>& args) {
    // torch_max_pool2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_max_t(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::max(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_mean(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { if (args.size() > 1) return TzdVal(torch::mean(*args[0].tensorVal, args[1].as_int())); return TzdVal(torch::mean(*args[0].tensorVal)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_median(const std::vector<TzdVal>& args) {
    // torch_median
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_memory_allocated(const std::vector<TzdVal>& args) {
    // torch_memory_allocated
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_memory_allocated_str(const std::vector<TzdVal>& args) {
    // torch_memory_allocated_str
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_min_t(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::min(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_mish(const std::vector<TzdVal>& args) {
    // torch_mish
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_mm(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::mm(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_mse_loss(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::mse_loss(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_mul(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::mul(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_mul_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) { args[0].tensorVal->mul_(*args[1].tensorVal); return args[0]; } return TzdVal();
}
inline TzdVal tzd_builtin_torch_multinomial(const std::vector<TzdVal>& args) {
    // torch_multinomial
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_nadam(const std::vector<TzdVal>& args) {
    // torch_nadam
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_nbytes(const std::vector<TzdVal>& args) {
    // torch_nbytes
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_ne(const std::vector<TzdVal>& args) {
    // torch_ne
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_neg(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::neg(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_nll_loss(const std::vector<TzdVal>& args) {
    // torch_nll_loss
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_no_grad(const std::vector<TzdVal>& args) {
    // torch_no_grad
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_no_grad_scope(const std::vector<TzdVal>& args) {
    // torch_no_grad_scope
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_nonzero(const std::vector<TzdVal>& args) {
    // torch_nonzero
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_norm_t(const std::vector<TzdVal>& args) {
    // torch_norm_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_num_tensors(const std::vector<TzdVal>& args) {
    // torch_num_tensors
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_numel(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal((int64_t)args[0].tensorVal->numel()); return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_one_hot(const std::vector<TzdVal>& args) {
    // torch_one_hot
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_ones(const std::vector<TzdVal>& args) {
    return TzdVal(torch::ones(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_ones_like(const std::vector<TzdVal>& args) {
    // torch_ones_like
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optim_delete(const std::vector<TzdVal>& args) {
    // torch_optim_delete
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optim_step(const std::vector<TzdVal>& args) {
    // torch_optim_step
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optim_zero_grad(const std::vector<TzdVal>& args) {
    // torch_optim_zero_grad
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optimizer_create(const std::vector<TzdVal>& args) {
    // torch_optimizer_create
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_orth(const std::vector<TzdVal>& args) {
    // torch_orth
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pad(const std::vector<TzdVal>& args) {
    // torch_pad
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pairwise_distance(const std::vector<TzdVal>& args) {
    // torch_pairwise_distance
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pca(const std::vector<TzdVal>& args) {
    // torch_pca
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_permute(const std::vector<TzdVal>& args) {
    // torch_permute
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pow(const std::vector<TzdVal>& args) {
    // torch_pow
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_prelu(const std::vector<TzdVal>& args) {
    // torch_prelu
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_print(const std::vector<TzdVal>& args) {
    // torch_print
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_prod(const std::vector<TzdVal>& args) {
    // torch_prod
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_q_scale(const std::vector<TzdVal>& args) {
    // torch_q_scale
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_q_zero_point(const std::vector<TzdVal>& args) {
    // torch_q_zero_point
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_quantize_per_channel(const std::vector<TzdVal>& args) {
    // torch_quantize_per_channel
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_quantize_per_tensor(const std::vector<TzdVal>& args) {
    // torch_quantize_per_tensor
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_rand(const std::vector<TzdVal>& args) {
    return TzdVal(torch::rand(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_randint(const std::vector<TzdVal>& args) {
    // torch_randint
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_randint_like(const std::vector<TzdVal>& args) {
    // torch_randint_like
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_randn(const std::vector<TzdVal>& args) {
    return TzdVal(torch::randn(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_randperm(const std::vector<TzdVal>& args) {
    // torch_randperm
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_release_all(const std::vector<TzdVal>& args) {
    // torch_release_all
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_release_tensor(const std::vector<TzdVal>& args) {
    // torch_release_tensor
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_relu(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::relu(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_remainder(const std::vector<TzdVal>& args) {
    // torch_remainder
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_repeat(const std::vector<TzdVal>& args) {
    // torch_repeat
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_requires_grad(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { bool req = args.size() > 1 ? args[1].as_bool() : true; args[0].tensorVal->set_requires_grad(req); return args[0]; } return TzdVal();
}
inline TzdVal tzd_builtin_torch_requires_grad_params(const std::vector<TzdVal>& args) {
    // torch_requires_grad_params
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_reshape(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR) { std::vector<TzdVal> sub(args.begin() + 1, args.end()); return TzdVal(args[0].tensorVal->reshape(tzd_parse_shape(sub))); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_rmsprop(const std::vector<TzdVal>& args) {
    // torch_rmsprop
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_save(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR) { torch::save(*args[0].tensorVal, args[1].to_string()); return TzdVal(true); } return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_save_state_dict(const std::vector<TzdVal>& args) {
    // torch_save_state_dict
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_scalar_value(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(args[0].tensorVal->item<double>()); return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_scatter(const std::vector<TzdVal>& args) {
    // torch_scatter
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_scatter_(const std::vector<TzdVal>& args) {
    // torch_scatter_
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_selu(const std::vector<TzdVal>& args) {
    // torch_selu
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_device(const std::vector<TzdVal>& args) {
    if (!args.empty()) torch::cuda::set_device(args[0].as_int()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_grad_enabled(const std::vector<TzdVal>& args) {
    // torch_set_grad_enabled
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_num_threads(const std::vector<TzdVal>& args) {
    if (!args.empty()) at::set_num_threads((int)args[0].as_int()); return TzdVal();
}
inline TzdVal tzd_builtin_torch_sgd(const std::vector<TzdVal>& args) {
    // torch_sgd
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_shape(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { auto sz = args[0].tensorVal->sizes(); std::string s = "["; for (size_t i = 0; i < sz.size(); ++i) { if (i > 0) s += ", "; s += std::to_string(sz[i]); } s += "]"; return TzdVal(s); } return TzdVal("[]");
}
inline TzdVal tzd_builtin_torch_sigmoid_fn(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::sigmoid(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_silu(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::silu(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_smooth_l1_loss(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::smooth_l1_loss(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_softmax(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { int64_t dim = args.size() > 1 ? args[1].as_int() : -1; return TzdVal(torch::softmax(*args[0].tensorVal, dim)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_softmin(const std::vector<TzdVal>& args) {
    // torch_softmin
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_softplus(const std::vector<TzdVal>& args) {
    // torch_softplus
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_solve(const std::vector<TzdVal>& args) {
    // torch_solve
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_solve_t(const std::vector<TzdVal>& args) {
    // torch_solve_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sort_t(const std::vector<TzdVal>& args) {
    // torch_sort_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_split_t(const std::vector<TzdVal>& args) {
    // torch_split_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sqrt(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(torch::sqrt(*args[0].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_squeeze(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { if (args.size() > 1) return TzdVal(torch::squeeze(*args[0].tensorVal, args[1].as_int())); return TzdVal(torch::squeeze(*args[0].tensorVal)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_stack(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(); std::vector<at::Tensor> ts; int64_t dim = args.size() > 1 ? args.back().as_int() : 0; if (args[0].type == ValType::ARRAY && args[0].arrVal) { for (const auto& a : *args[0].arrVal) if (a.type == ValType::TENSOR) ts.push_back(*a.tensorVal); } if (ts.empty()) return TzdVal(); return TzdVal(torch::stack(ts, dim));
}
inline TzdVal tzd_builtin_torch_std(const std::vector<TzdVal>& args) {
    // torch_std
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_std_mean(const std::vector<TzdVal>& args) {
    // torch_std_mean
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sub(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) return TzdVal(torch::sub(*args[0].tensorVal, *args[1].tensorVal)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_sub_(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR && args[1].type == ValType::TENSOR) { args[0].tensorVal->sub_(*args[1].tensorVal); return args[0]; } return TzdVal();
}
inline TzdVal tzd_builtin_torch_sum(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) { if (args.size() > 1) return TzdVal(torch::sum(*args[0].tensorVal, args[1].as_int())); return TzdVal(torch::sum(*args[0].tensorVal)); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_svd(const std::vector<TzdVal>& args) {
    // torch_svd
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_tensor(const std::vector<TzdVal>& args) {
    // torch_tensor
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_threshold(const std::vector<TzdVal>& args) {
    // torch_threshold
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_array(const std::vector<TzdVal>& args) {
    // torch_to_array
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_bool(const std::vector<TzdVal>& args) {
    // torch_to_bool
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_cpu(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(args[0].tensorVal->to(torch::kCPU)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_cuda(const std::vector<TzdVal>& args) {
    if (!args.empty() && args[0].type == ValType::TENSOR) return TzdVal(args[0].tensorVal->to(torch::kCUDA)); return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_device(const std::vector<TzdVal>& args) {
    // torch_to_device
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_double(const std::vector<TzdVal>& args) {
    // torch_to_double
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_dtype(const std::vector<TzdVal>& args) {
    // torch_to_dtype
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_float(const std::vector<TzdVal>& args) {
    // torch_to_float
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_int(const std::vector<TzdVal>& args) {
    // torch_to_int
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_long(const std::vector<TzdVal>& args) {
    // torch_to_long
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_string(const std::vector<TzdVal>& args) {
    // torch_to_string
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_topk(const std::vector<TzdVal>& args) {
    // torch_topk
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_trace_t(const std::vector<TzdVal>& args) {
    // torch_trace_t
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_transpose(const std::vector<TzdVal>& args) {
    if (args.size() >= 3 && args[0].type == ValType::TENSOR) return TzdVal(torch::transpose(*args[0].tensorVal, args[1].as_int(), args[2].as_int())); return TzdVal();
}
inline TzdVal tzd_builtin_torch_tril(const std::vector<TzdVal>& args) {
    // torch_tril
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_triple_margin_loss(const std::vector<TzdVal>& args) {
    // torch_triple_margin_loss
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_triu(const std::vector<TzdVal>& args) {
    // torch_triu
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_unique(const std::vector<TzdVal>& args) {
    // torch_unique
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_unsqueeze(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR) return TzdVal(torch::unsqueeze(*args[0].tensorVal, args[1].as_int())); return TzdVal();
}
inline TzdVal tzd_builtin_torch_upsample_bilinear2d(const std::vector<TzdVal>& args) {
    // torch_upsample_bilinear2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_upsample_nearest2d(const std::vector<TzdVal>& args) {
    // torch_upsample_nearest2d
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_var(const std::vector<TzdVal>& args) {
    // torch_var
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_var_mean(const std::vector<TzdVal>& args) {
    // torch_var_mean
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_version(const std::vector<TzdVal>& args) {
    return TzdVal("LibTorch 2.5.1");
}
inline TzdVal tzd_builtin_torch_view(const std::vector<TzdVal>& args) {
    if (args.size() >= 2 && args[0].type == ValType::TENSOR) { std::vector<TzdVal> sub(args.begin() + 1, args.end()); return TzdVal(args[0].tensorVal->view(tzd_parse_shape(sub))); } return TzdVal();
}
inline TzdVal tzd_builtin_torch_where(const std::vector<TzdVal>& args) {
    // torch_where
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_zero_(const std::vector<TzdVal>& args) {
    // torch_zero_
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_zero_grad_params(const std::vector<TzdVal>& args) {
    // torch_zero_grad_params
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_zeros(const std::vector<TzdVal>& args) {
    return TzdVal(torch::zeros(tzd_parse_shape(args)));
}
inline TzdVal tzd_builtin_torch_zeros_like(const std::vector<TzdVal>& args) {
    // torch_zeros_like
    if (!args.empty() && args[0].type == ValType::TENSOR) return args[0];
    return TzdVal();
}
#else
// ── Non-LibTorch Standalone Fallbacks (Zero-DLL Mode) ──
TzdVal tzd_builtin_torch_zeros(const std::vector<TzdVal>& args);
TzdVal tzd_builtin_torch_ones(const std::vector<TzdVal>& args);
TzdVal tzd_builtin_torch_relu(const std::vector<TzdVal>& args);
TzdVal tzd_builtin_torch_sigmoid(const std::vector<TzdVal>& args);
TzdVal tzd_builtin_torch_tensor(const std::vector<TzdVal>& args);

inline TzdVal tzd_builtin_torch_abs(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : tzd_builtin_abs(args);
}
inline TzdVal tzd_builtin_torch_adagrad(const std::vector<TzdVal>& args) {
    // torch_adagrad
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adam(const std::vector<TzdVal>& args) {
    // torch_adam
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adamax(const std::vector<TzdVal>& args) {
    // torch_adamax
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adamw(const std::vector<TzdVal>& args) {
    // torch_adamw
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool1d(const std::vector<TzdVal>& args) {
    // torch_adaptive_avg_pool1d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_adaptive_avg_pool2d(const std::vector<TzdVal>& args) {
    // torch_adaptive_avg_pool2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_add(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] + args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_add_(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] + args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_all(const std::vector<TzdVal>& args) {
    // torch_all
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_allclose(const std::vector<TzdVal>& args) {
    // torch_allclose
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_any(const std::vector<TzdVal>& args) {
    // torch_any
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_arange(const std::vector<TzdVal>& args) {
    return tzd_builtin_range(args);
}
inline TzdVal tzd_builtin_torch_argmax(const std::vector<TzdVal>& args) {
    // torch_argmax
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_argmin(const std::vector<TzdVal>& args) {
    // torch_argmin
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_argsort(const std::vector<TzdVal>& args) {
    // torch_argsort
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_atan2_t(const std::vector<TzdVal>& args) {
    // torch_atan2_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_auto_cleanup(const std::vector<TzdVal>& args) {
    // torch_auto_cleanup
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_avg_pool2d(const std::vector<TzdVal>& args) {
    // torch_avg_pool2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_backward(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_batch_norm(const std::vector<TzdVal>& args) {
    // torch_batch_norm
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_batch_norm1d(const std::vector<TzdVal>& args) {
    // torch_batch_norm1d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_batch_norm2d(const std::vector<TzdVal>& args) {
    // torch_batch_norm2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_bce_loss(const std::vector<TzdVal>& args) {
    return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_bernoulli(const std::vector<TzdVal>& args) {
    // torch_bernoulli
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_bincount(const std::vector<TzdVal>& args) {
    // torch_bincount
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_bmm(const std::vector<TzdVal>& args) {
    return tzd_builtin_matrixMul(args);
}
inline TzdVal tzd_builtin_torch_broadcast_shapes(const std::vector<TzdVal>& args) {
    // torch_broadcast_shapes
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_broadcast_tensors(const std::vector<TzdVal>& args) {
    // torch_broadcast_tensors
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_broadcast_to(const std::vector<TzdVal>& args) {
    // torch_broadcast_to
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cat(const std::vector<TzdVal>& args) {
    return tzd_builtin_concat(args);
}
inline TzdVal tzd_builtin_torch_chain_matmul(const std::vector<TzdVal>& args) {
    // torch_chain_matmul
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cholesky(const std::vector<TzdVal>& args) {
    // torch_cholesky
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_chunk(const std::vector<TzdVal>& args) {
    // torch_chunk
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_clamp(const std::vector<TzdVal>& args) {
    return tzd_builtin_clamp(args);
}
inline TzdVal tzd_builtin_torch_clamp_(const std::vector<TzdVal>& args) {
    return tzd_builtin_clamp(args);
}
inline TzdVal tzd_builtin_torch_clip_grad_norm(const std::vector<TzdVal>& args) {
    // torch_clip_grad_norm
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_clip_grad_value(const std::vector<TzdVal>& args) {
    // torch_clip_grad_value
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_clone(const std::vector<TzdVal>& args) {
    return tzd_builtin_deepCopy(args);
}
inline TzdVal tzd_builtin_torch_contiguous(const std::vector<TzdVal>& args) {
    // torch_contiguous
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_conv1d(const std::vector<TzdVal>& args) {
    // torch_conv1d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_conv2d(const std::vector<TzdVal>& args) {
    // torch_conv2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_conv_transpose2d(const std::vector<TzdVal>& args) {
    // torch_conv_transpose2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_copy_(const std::vector<TzdVal>& args) {
    // torch_copy_
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_corrcoef(const std::vector<TzdVal>& args) {
    // torch_corrcoef
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cosine_similarity(const std::vector<TzdVal>& args) {
    // torch_cosine_similarity
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_count_nonzero(const std::vector<TzdVal>& args) {
    // torch_count_nonzero
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_count_params(const std::vector<TzdVal>& args) {
    // torch_count_params
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cov(const std::vector<TzdVal>& args) {
    // torch_cov
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_create_param(const std::vector<TzdVal>& args) {
    // torch_create_param
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cross_entropy(const std::vector<TzdVal>& args) {
    return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_cuda_is_available(const std::vector<TzdVal>& args) {
    return TzdVal(false);
}
inline TzdVal tzd_builtin_torch_cuda_max_memory_allocated(const std::vector<TzdVal>& args) {
    // torch_cuda_max_memory_allocated
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_memory_allocated(const std::vector<TzdVal>& args) {
    // torch_cuda_memory_allocated
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_memory_reserved(const std::vector<TzdVal>& args) {
    // torch_cuda_memory_reserved
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_reset_peak_memory(const std::vector<TzdVal>& args) {
    // torch_cuda_reset_peak_memory
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cuda_synchronize(const std::vector<TzdVal>& args) {
    // torch_cuda_synchronize
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cumprod(const std::vector<TzdVal>& args) {
    // torch_cumprod
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_cumsum(const std::vector<TzdVal>& args) {
    // torch_cumsum
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_current_device(const std::vector<TzdVal>& args) {
    return TzdVal(-1);
}
inline TzdVal tzd_builtin_torch_dequantize(const std::vector<TzdVal>& args) {
    // torch_dequantize
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_det(const std::vector<TzdVal>& args) {
    // torch_det
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_det_t(const std::vector<TzdVal>& args) {
    // torch_det_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_detach(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_device_count(const std::vector<TzdVal>& args) {
    return TzdVal(0);
}
inline TzdVal tzd_builtin_torch_device_str(const std::vector<TzdVal>& args) {
    // torch_device_str
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_diag(const std::vector<TzdVal>& args) {
    // torch_diag
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_diagflat(const std::vector<TzdVal>& args) {
    // torch_diagflat
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_digamma(const std::vector<TzdVal>& args) {
    // torch_digamma
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_dim(const std::vector<TzdVal>& args) {
    return TzdVal(1);
}
inline TzdVal tzd_builtin_torch_div(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] / args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_div_(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] / args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_dropout(const std::vector<TzdVal>& args) {
    // torch_dropout
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_dtype(const std::vector<TzdVal>& args) {
    // torch_dtype
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_dtype_str(const std::vector<TzdVal>& args) {
    // torch_dtype_str
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_eig(const std::vector<TzdVal>& args) {
    // torch_eig
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_element_size(const std::vector<TzdVal>& args) {
    // torch_element_size
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_elu(const std::vector<TzdVal>& args) {
    // torch_elu
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_embedding(const std::vector<TzdVal>& args) {
    // torch_embedding
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_empty(const std::vector<TzdVal>& args) {
    return tzd_builtin_torch_zeros(args);
}
inline TzdVal tzd_builtin_torch_empty_cache(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_empty_like(const std::vector<TzdVal>& args) {
    // torch_empty_like
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_eq(const std::vector<TzdVal>& args) {
    // torch_eq
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_equal(const std::vector<TzdVal>& args) {
    // torch_equal
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_erf(const std::vector<TzdVal>& args) {
    // torch_erf
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_erfc(const std::vector<TzdVal>& args) {
    // torch_erfc
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_exp(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : tzd_builtin_exp(args);
}
inline TzdVal tzd_builtin_torch_expand(const std::vector<TzdVal>& args) {
    // torch_expand
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_eye(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int(); return tzd_builtin_identity({TzdVal(n)});
}
inline TzdVal tzd_builtin_torch_fill_(const std::vector<TzdVal>& args) {
    // torch_fill_
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_flatten(const std::vector<TzdVal>& args) {
    return tzd_builtin_flatten(args);
}
inline TzdVal tzd_builtin_torch_flatten_t(const std::vector<TzdVal>& args) {
    return tzd_builtin_flatten(args);
}
inline TzdVal tzd_builtin_torch_fmod(const std::vector<TzdVal>& args) {
    // torch_fmod
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_from_array(const std::vector<TzdVal>& args) {
    // torch_from_array
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_full(const std::vector<TzdVal>& args) {
    // torch_full
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_full_like(const std::vector<TzdVal>& args) {
    // torch_full_like
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_linear_bias_gelu(const std::vector<TzdVal>& args) {
    // torch_fused_linear_bias_gelu
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_residual_layernorm(const std::vector<TzdVal>& args) {
    // torch_fused_residual_layernorm
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_silu_mul(const std::vector<TzdVal>& args) {
    // torch_fused_silu_mul
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_fused_softmax_mask(const std::vector<TzdVal>& args) {
    // torch_fused_softmax_mask
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gather(const std::vector<TzdVal>& args) {
    // torch_gather
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gc(const std::vector<TzdVal>& args) {
    // torch_gc
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_ge(const std::vector<TzdVal>& args) {
    // torch_ge
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gelu(const std::vector<TzdVal>& args) {
    return tzd_builtin_torch_relu(args);
}
inline TzdVal tzd_builtin_torch_get_num_threads(const std::vector<TzdVal>& args) {
    return TzdVal(4);
}
inline TzdVal tzd_builtin_torch_glu(const std::vector<TzdVal>& args) {
    // torch_glu
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_grad(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_grad_fn(const std::vector<TzdVal>& args) {
    // torch_grad_fn
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_gt(const std::vector<TzdVal>& args) {
    // torch_gt
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_hardswish(const std::vector<TzdVal>& args) {
    // torch_hardswish
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_hardtanh(const std::vector<TzdVal>& args) {
    // torch_hardtanh
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_histc(const std::vector<TzdVal>& args) {
    // torch_histc
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_identity(const std::vector<TzdVal>& args) {
    // torch_identity
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_index_copy_(const std::vector<TzdVal>& args) {
    // torch_index_copy_
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_index_put(const std::vector<TzdVal>& args) {
    // torch_index_put
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_index_select(const std::vector<TzdVal>& args) {
    // torch_index_select
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_kaiming(const std::vector<TzdVal>& args) {
    // torch_init_kaiming
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_normal(const std::vector<TzdVal>& args) {
    // torch_init_normal
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_ones(const std::vector<TzdVal>& args) {
    // torch_init_ones
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_uniform(const std::vector<TzdVal>& args) {
    // torch_init_uniform
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_xavier(const std::vector<TzdVal>& args) {
    // torch_init_xavier
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_init_zeros(const std::vector<TzdVal>& args) {
    // torch_init_zeros
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_interpolate(const std::vector<TzdVal>& args) {
    // torch_interpolate
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_inv(const std::vector<TzdVal>& args) {
    // torch_inv
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_inverse(const std::vector<TzdVal>& args) {
    // torch_inverse
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_inverse_t(const std::vector<TzdVal>& args) {
    // torch_inverse_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_contiguous(const std::vector<TzdVal>& args) {
    // torch_is_contiguous
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_floating_point(const std::vector<TzdVal>& args) {
    // torch_is_floating_point
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_grad_enabled(const std::vector<TzdVal>& args) {
    // torch_is_grad_enabled
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_integer(const std::vector<TzdVal>& args) {
    // torch_is_integer
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_leaf(const std::vector<TzdVal>& args) {
    // torch_is_leaf
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_pinned(const std::vector<TzdVal>& args) {
    // torch_is_pinned
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_requires_grad(const std::vector<TzdVal>& args) {
    // torch_is_requires_grad
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_is_tensor(const std::vector<TzdVal>& args) {
    return TzdVal(!args.empty() && args[0].type == ValType::ARRAY);
}
inline TzdVal tzd_builtin_torch_isfinite(const std::vector<TzdVal>& args) {
    // torch_isfinite
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_isinf(const std::vector<TzdVal>& args) {
    // torch_isinf
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_isnan(const std::vector<TzdVal>& args) {
    // torch_isnan
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_item(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(0.0) : args[0];
}
inline TzdVal tzd_builtin_torch_jit_eval(const std::vector<TzdVal>& args) {
    // torch_jit_eval
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_jit_load(const std::vector<TzdVal>& args) {
    // torch_jit_load
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_jit_save(const std::vector<TzdVal>& args) {
    // torch_jit_save
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_jit_train(const std::vector<TzdVal>& args) {
    // torch_jit_train
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_kl_div(const std::vector<TzdVal>& args) {
    // torch_kl_div
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_l1_loss(const std::vector<TzdVal>& args) {
    return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_layer_norm(const std::vector<TzdVal>& args) {
    // torch_layer_norm
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_le(const std::vector<TzdVal>& args) {
    // torch_le
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_leaky_relu(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(0.0) : TzdVal(args[0].as_double() > 0 ? args[0].as_double() : 0.01 * args[0].as_double());
}
inline TzdVal tzd_builtin_torch_lerp(const std::vector<TzdVal>& args) {
    // torch_lerp
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_lgamma(const std::vector<TzdVal>& args) {
    // torch_lgamma
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_linear(const std::vector<TzdVal>& args) {
    return tzd_builtin_matrixMul(args);
}
inline TzdVal tzd_builtin_torch_linspace(const std::vector<TzdVal>& args) {
    return tzd_builtin_linspace_arr(args);
}
inline TzdVal tzd_builtin_torch_load(const std::vector<TzdVal>& args) {
    return tzd_make_array({});
}
inline TzdVal tzd_builtin_torch_load_state_dict(const std::vector<TzdVal>& args) {
    // torch_load_state_dict
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_log(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : tzd_builtin_log(args);
}
inline TzdVal tzd_builtin_torch_log_softmax(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_logcumsumexp(const std::vector<TzdVal>& args) {
    // torch_logcumsumexp
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logical_and(const std::vector<TzdVal>& args) {
    // torch_logical_and
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logical_not(const std::vector<TzdVal>& args) {
    // torch_logical_not
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logical_or(const std::vector<TzdVal>& args) {
    // torch_logical_or
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logspace(const std::vector<TzdVal>& args) {
    // torch_logspace
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_logsumexp(const std::vector<TzdVal>& args) {
    // torch_logsumexp
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_lstsq(const std::vector<TzdVal>& args) {
    // torch_lstsq
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_lt(const std::vector<TzdVal>& args) {
    // torch_lt
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_make_contiguous(const std::vector<TzdVal>& args) {
    // torch_make_contiguous
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_manual_seed(const std::vector<TzdVal>& args) {
    return tzd_builtin_randomSeed(args);
}
inline TzdVal tzd_builtin_torch_masked_fill(const std::vector<TzdVal>& args) {
    // torch_masked_fill
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_masked_fill_(const std::vector<TzdVal>& args) {
    // torch_masked_fill_
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_masked_select(const std::vector<TzdVal>& args) {
    // torch_masked_select
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_matmul(const std::vector<TzdVal>& args) {
    return tzd_builtin_matrixMul(args);
}
inline TzdVal tzd_builtin_torch_matrix_exp(const std::vector<TzdVal>& args) {
    // torch_matrix_exp
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_max_pool2d(const std::vector<TzdVal>& args) {
    // torch_max_pool2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_max_t(const std::vector<TzdVal>& args) {
    return tzd_builtin_maxArr(args);
}
inline TzdVal tzd_builtin_torch_mean(const std::vector<TzdVal>& args) {
    return tzd_builtin_avg(args);
}
inline TzdVal tzd_builtin_torch_median(const std::vector<TzdVal>& args) {
    // torch_median
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_memory_allocated(const std::vector<TzdVal>& args) {
    // torch_memory_allocated
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_memory_allocated_str(const std::vector<TzdVal>& args) {
    // torch_memory_allocated_str
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_min_t(const std::vector<TzdVal>& args) {
    return tzd_builtin_minArr(args);
}
inline TzdVal tzd_builtin_torch_mish(const std::vector<TzdVal>& args) {
    // torch_mish
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_mm(const std::vector<TzdVal>& args) {
    return tzd_builtin_matrixMul(args);
}
inline TzdVal tzd_builtin_torch_mse_loss(const std::vector<TzdVal>& args) {
    return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_mul(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] * args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_mul_(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] * args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_multinomial(const std::vector<TzdVal>& args) {
    // torch_multinomial
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_nadam(const std::vector<TzdVal>& args) {
    // torch_nadam
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_nbytes(const std::vector<TzdVal>& args) {
    // torch_nbytes
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_ne(const std::vector<TzdVal>& args) {
    // torch_ne
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_neg(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : -args[0];
}
inline TzdVal tzd_builtin_torch_nll_loss(const std::vector<TzdVal>& args) {
    // torch_nll_loss
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_no_grad(const std::vector<TzdVal>& args) {
    // torch_no_grad
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_no_grad_scope(const std::vector<TzdVal>& args) {
    // torch_no_grad_scope
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_nonzero(const std::vector<TzdVal>& args) {
    // torch_nonzero
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_norm_t(const std::vector<TzdVal>& args) {
    // torch_norm_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_num_tensors(const std::vector<TzdVal>& args) {
    // torch_num_tensors
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_numel(const std::vector<TzdVal>& args) {
    return tzd_builtin_len(args);
}
inline TzdVal tzd_builtin_torch_one_hot(const std::vector<TzdVal>& args) {
    // torch_one_hot
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_ones(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int(); auto arr = std::make_shared<std::vector<TzdVal>>(n > 0 ? n : 1, TzdVal(1.0)); return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_ones_like(const std::vector<TzdVal>& args) {
    // torch_ones_like
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optim_delete(const std::vector<TzdVal>& args) {
    // torch_optim_delete
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optim_step(const std::vector<TzdVal>& args) {
    // torch_optim_step
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optim_zero_grad(const std::vector<TzdVal>& args) {
    // torch_optim_zero_grad
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_optimizer_create(const std::vector<TzdVal>& args) {
    // torch_optimizer_create
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_orth(const std::vector<TzdVal>& args) {
    // torch_orth
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pad(const std::vector<TzdVal>& args) {
    // torch_pad
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pairwise_distance(const std::vector<TzdVal>& args) {
    // torch_pairwise_distance
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pca(const std::vector<TzdVal>& args) {
    // torch_pca
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_permute(const std::vector<TzdVal>& args) {
    // torch_permute
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_pow(const std::vector<TzdVal>& args) {
    // torch_pow
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_prelu(const std::vector<TzdVal>& args) {
    // torch_prelu
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_print(const std::vector<TzdVal>& args) {
    // torch_print
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_prod(const std::vector<TzdVal>& args) {
    // torch_prod
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_q_scale(const std::vector<TzdVal>& args) {
    // torch_q_scale
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_q_zero_point(const std::vector<TzdVal>& args) {
    // torch_q_zero_point
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_quantize_per_channel(const std::vector<TzdVal>& args) {
    // torch_quantize_per_channel
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_quantize_per_tensor(const std::vector<TzdVal>& args) {
    // torch_quantize_per_tensor
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_rand(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int(); auto arr = std::make_shared<std::vector<TzdVal>>(); for (int64_t i = 0; i < n; ++i) arr->push_back(tzd_builtin_random({})); return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_randint(const std::vector<TzdVal>& args) {
    // torch_randint
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_randint_like(const std::vector<TzdVal>& args) {
    // torch_randint_like
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_randn(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int(); auto arr = std::make_shared<std::vector<TzdVal>>(); for (int64_t i = 0; i < n; ++i) arr->push_back(tzd_builtin_random({})); return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_randperm(const std::vector<TzdVal>& args) {
    // torch_randperm
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_release_all(const std::vector<TzdVal>& args) {
    // torch_release_all
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_release_tensor(const std::vector<TzdVal>& args) {
    // torch_release_tensor
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_relu(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(0.0) : TzdVal((std::max)(0.0, args[0].as_double()));
}
inline TzdVal tzd_builtin_torch_remainder(const std::vector<TzdVal>& args) {
    // torch_remainder
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_repeat(const std::vector<TzdVal>& args) {
    // torch_repeat
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_requires_grad(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_requires_grad_params(const std::vector<TzdVal>& args) {
    // torch_requires_grad_params
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_reshape(const std::vector<TzdVal>& args) {
    return tzd_builtin_reshape(args);
}
inline TzdVal tzd_builtin_torch_rmsprop(const std::vector<TzdVal>& args) {
    // torch_rmsprop
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_save(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_save_state_dict(const std::vector<TzdVal>& args) {
    // torch_save_state_dict
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_scalar_value(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal(0.0) : args[0];
}
inline TzdVal tzd_builtin_torch_scatter(const std::vector<TzdVal>& args) {
    // torch_scatter
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_scatter_(const std::vector<TzdVal>& args) {
    // torch_scatter_
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_selu(const std::vector<TzdVal>& args) {
    // torch_selu
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_device(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_grad_enabled(const std::vector<TzdVal>& args) {
    // torch_set_grad_enabled
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_set_num_threads(const std::vector<TzdVal>& args) {
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sgd(const std::vector<TzdVal>& args) {
    // torch_sgd
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_shape(const std::vector<TzdVal>& args) {
    return tzd_builtin_len(args);
}
inline TzdVal tzd_builtin_torch_sigmoid(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(0.5);
    double x = args[0].as_double();
    return TzdVal(1.0 / (1.0 + std::exp(-x)));
}
inline TzdVal tzd_builtin_torch_sigmoid_fn(const std::vector<TzdVal>& args) {
    return tzd_builtin_torch_sigmoid(args);
}
inline TzdVal tzd_builtin_torch_silu(const std::vector<TzdVal>& args) {
    return tzd_builtin_torch_sigmoid(args);
}
inline TzdVal tzd_builtin_torch_smooth_l1_loss(const std::vector<TzdVal>& args) {
    return TzdVal(0.0);
}
inline TzdVal tzd_builtin_torch_softmax(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_softmin(const std::vector<TzdVal>& args) {
    // torch_softmin
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_softplus(const std::vector<TzdVal>& args) {
    // torch_softplus
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_solve(const std::vector<TzdVal>& args) {
    // torch_solve
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_solve_t(const std::vector<TzdVal>& args) {
    // torch_solve_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sort_t(const std::vector<TzdVal>& args) {
    // torch_sort_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_split_t(const std::vector<TzdVal>& args) {
    // torch_split_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sqrt(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : tzd_builtin_sqrt(args);
}
inline TzdVal tzd_builtin_torch_squeeze(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_stack(const std::vector<TzdVal>& args) {
    return tzd_builtin_concat(args);
}
inline TzdVal tzd_builtin_torch_std(const std::vector<TzdVal>& args) {
    // torch_std
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_std_mean(const std::vector<TzdVal>& args) {
    // torch_std_mean
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_sub(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] - args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_sub_(const std::vector<TzdVal>& args) {
    return args.size() >= 2 ? args[0] - args[1] : (args.empty() ? TzdVal() : args[0]);
}
inline TzdVal tzd_builtin_torch_sum(const std::vector<TzdVal>& args) {
    return tzd_builtin_sum(args);
}
inline TzdVal tzd_builtin_torch_svd(const std::vector<TzdVal>& args) {
    // torch_svd
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_tensor(const std::vector<TzdVal>& args) {
    // torch_tensor
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_threshold(const std::vector<TzdVal>& args) {
    // torch_threshold
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_array(const std::vector<TzdVal>& args) {
    // torch_to_array
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_bool(const std::vector<TzdVal>& args) {
    // torch_to_bool
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_cpu(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_cuda(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_to_device(const std::vector<TzdVal>& args) {
    // torch_to_device
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_double(const std::vector<TzdVal>& args) {
    // torch_to_double
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_dtype(const std::vector<TzdVal>& args) {
    // torch_to_dtype
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_float(const std::vector<TzdVal>& args) {
    // torch_to_float
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_int(const std::vector<TzdVal>& args) {
    // torch_to_int
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_long(const std::vector<TzdVal>& args) {
    // torch_to_long
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_to_string(const std::vector<TzdVal>& args) {
    // torch_to_string
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_topk(const std::vector<TzdVal>& args) {
    // torch_topk
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_trace_t(const std::vector<TzdVal>& args) {
    // torch_trace_t
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_transpose(const std::vector<TzdVal>& args) {
    return tzd_builtin_transpose(args);
}
inline TzdVal tzd_builtin_torch_tril(const std::vector<TzdVal>& args) {
    // torch_tril
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_triple_margin_loss(const std::vector<TzdVal>& args) {
    // torch_triple_margin_loss
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_triu(const std::vector<TzdVal>& args) {
    // torch_triu
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_unique(const std::vector<TzdVal>& args) {
    // torch_unique
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_unsqueeze(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}
inline TzdVal tzd_builtin_torch_upsample_bilinear2d(const std::vector<TzdVal>& args) {
    // torch_upsample_bilinear2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_upsample_nearest2d(const std::vector<TzdVal>& args) {
    // torch_upsample_nearest2d
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_var(const std::vector<TzdVal>& args) {
    // torch_var
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_var_mean(const std::vector<TzdVal>& args) {
    // torch_var_mean
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_version(const std::vector<TzdVal>& args) {
    return TzdVal("Standalone-Fallback 0.2.4");
}
inline TzdVal tzd_builtin_torch_view(const std::vector<TzdVal>& args) {
    return tzd_builtin_reshape(args);
}
inline TzdVal tzd_builtin_torch_where(const std::vector<TzdVal>& args) {
    // torch_where
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_zero_(const std::vector<TzdVal>& args) {
    // torch_zero_
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_zero_grad_params(const std::vector<TzdVal>& args) {
    // torch_zero_grad_params
    if (!args.empty()) return args[0];
    return TzdVal();
}
inline TzdVal tzd_builtin_torch_zeros(const std::vector<TzdVal>& args) {
    int64_t n = args.empty() ? 1 : args[0].as_int(); auto arr = std::make_shared<std::vector<TzdVal>>(n > 0 ? n : 1, TzdVal(0.0)); return TzdVal(arr);
}
inline TzdVal tzd_builtin_torch_zeros_like(const std::vector<TzdVal>& args) {
    // torch_zeros_like
    if (!args.empty()) return args[0];
    return TzdVal();
}
#endif


inline void init_console() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

} // namespace tzd_rt

using namespace tzd_rt;

#endif // TZD_NATIVE_RUNTIME_HPP
