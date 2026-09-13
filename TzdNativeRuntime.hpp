// ============================================================================
// TzdNativeRuntime.hpp
// Standalone, Zero-DLL, Header-Only Native C++ Runtime for TzdLang AOT Compilation
// Compiles to ultra-fast native x86_64 machine code via MSVC / Clang
// ============================================================================

#ifndef TZD_NATIVE_RUNTIME_HPP
#define TZD_NATIVE_RUNTIME_HPP

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <iostream>
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
    FUNC
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

    // Conversions
    double as_double() const {
        switch (type) {
            case ValType::FLOAT: return fVal;
            case ValType::INT: return (double)iVal;
            case ValType::BOOL: return bVal ? 1.0 : 0.0;
            case ValType::STRING: {
                try { return std::stod(sVal); } catch (...) { return 0.0; }
            }
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
            case ValType::NIL: return false;
            default: return false;
        }
    }

    std::string to_string() const;

    // Arithmetic operators
    TzdVal operator+(const TzdVal& o) const {
        if (type == ValType::STRING || o.type == ValType::STRING) {
            return TzdVal(to_string() + o.to_string());
        }
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
            return TzdVal(as_double() + o.as_double());
        }
        return TzdVal(as_int() + o.as_int());
    }

    TzdVal operator-(const TzdVal& o) const {
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
            return TzdVal(as_double() - o.as_double());
        }
        return TzdVal(as_int() - o.as_int());
    }

    TzdVal operator*(const TzdVal& o) const {
        if (type == ValType::FLOAT || o.type == ValType::FLOAT) {
            return TzdVal(as_double() * o.as_double());
        }
        return TzdVal(as_int() * o.as_int());
    }

    TzdVal operator/(const TzdVal& o) const {
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
        if (name == "length" || name == "size") {
            return TzdVal((int64_t)arrVal->size());
        }
    }
    return TzdVal();
}

inline TzdVal TzdVal::get_index(const TzdVal& idx) {
    if (type == ValType::ARRAY && arrVal) {
        int64_t i = idx.as_int();
        if (i >= 0 && i < (int64_t)arrVal->size()) {
            return (*arrVal)[(size_t)i];
        }
        return TzdVal();
    }
    if (type == ValType::MAP && mapVal) {
        std::string key = idx.to_string();
        auto it = mapVal->find(key);
        return it != mapVal->end() ? it->second : TzdVal();
    }
    if (type == ValType::STRING) {
        int64_t i = idx.as_int();
        if (i >= 0 && i < (int64_t)sVal.size()) {
            return TzdVal(std::string(1, sVal[(size_t)i]));
        }
        return TzdVal("");
    }
    return TzdVal();
}

inline void TzdVal::set_index(const TzdVal& idx, const TzdVal& val) {
    if (type == ValType::ARRAY) {
        if (!arrVal) arrVal = std::make_shared<std::vector<TzdVal>>();
        int64_t i = idx.as_int();
        if (i >= 0) {
            if ((size_t)i >= arrVal->size()) {
                arrVal->resize((size_t)i + 1);
            }
            (*arrVal)[(size_t)i] = val;
        }
        return;
    }
    if (type == ValType::MAP) {
        if (!mapVal) mapVal = std::make_shared<std::unordered_map<std::string, TzdVal>>();
        (*mapVal)[idx.to_string()] = val;
        return;
    }
}

// ============================================================================
// Standard Built-in Functions
// ============================================================================

inline double tzd_clock() {
    using namespace std::chrono;
    return (double)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

inline void tzd_print_one(const TzdVal& v) {
    std::cout << v.to_string();
}

template<typename... Args>
inline void tzd_print(Args&&... args) {
    bool first = true;
    auto printArg = [&](const TzdVal& v) {
        if (!first) std::cout << " ";
        first = false;
        std::cout << v.to_string();
    };
    (printArg(std::forward<Args>(args)), ...);
    std::cout << std::endl;
}

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
    return TzdVal(0);
}

inline TzdVal tzd_toString(const TzdVal& v) {
    return TzdVal(v.to_string());
}

inline TzdVal tzd_toInt(const TzdVal& v) {
    return TzdVal(v.as_int());
}

inline TzdVal tzd_toFloat(const TzdVal& v) {
    return TzdVal(v.as_double());
}

// Math Builtins
namespace Math {
    inline TzdVal sqrt(const TzdVal& v) { return TzdVal(std::sqrt(v.as_double())); }
    inline TzdVal sin(const TzdVal& v)  { return TzdVal(std::sin(v.as_double())); }
    inline TzdVal cos(const TzdVal& v)  { return TzdVal(std::cos(v.as_double())); }
    inline TzdVal tan(const TzdVal& v)  { return TzdVal(std::tan(v.as_double())); }
    inline TzdVal abs(const TzdVal& v)  { return TzdVal(std::abs(v.as_double())); }
    inline TzdVal floor(const TzdVal& v){ return TzdVal(std::floor(v.as_double())); }
    inline TzdVal ceil(const TzdVal& v) { return TzdVal(std::ceil(v.as_double())); }
    inline TzdVal pow(const TzdVal& a, const TzdVal& b) { return TzdVal(std::pow(a.as_double(), b.as_double())); }
    inline TzdVal log(const TzdVal& v)  { return TzdVal(std::log(v.as_double())); }
    inline TzdVal round(const TzdVal& v){ return TzdVal(std::round(v.as_double())); }

    static const double PI = 3.14159265358979323846;
    static const double E  = 2.71828182845904523536;
}

// Standalone math functions
inline TzdVal sqrt(const TzdVal& v) { return Math::sqrt(v); }
inline TzdVal sin(const TzdVal& v)  { return Math::sin(v); }
inline TzdVal cos(const TzdVal& v)  { return Math::cos(v); }
inline TzdVal abs(const TzdVal& v)  { return Math::abs(v); }
inline TzdVal floor(const TzdVal& v){ return Math::floor(v); }
inline TzdVal ceil(const TzdVal& v) { return Math::ceil(v); }
inline TzdVal pow(const TzdVal& a, const TzdVal& b) { return Math::pow(a, b); }

// PyTorch / Tensor Stubs (Zero-DLL when LibTorch is omitted or --buildCpu is used)
#ifndef WITH_LIBTORCH
inline TzdVal torch_zeros(const TzdVal& s) {
    auto arr = std::make_shared<std::vector<TzdVal>>(s.as_int(), TzdVal(0.0));
    return TzdVal(arr);
}
inline TzdVal torch_is_tensor(const TzdVal& v) {
    return TzdVal(v.type == ValType::ARRAY);
}
inline TzdVal torch_shape(const TzdVal& v) {
    if (v.type == ValType::ARRAY && v.arrVal) {
        return TzdVal("[" + std::to_string(v.arrVal->size()) + "]");
    }
    return TzdVal("[]");
}
inline TzdVal jsonParse(const TzdVal& s) {
    auto m = std::make_shared<std::unordered_map<std::string, TzdVal>>();
    return TzdVal(m);
}
inline TzdVal mapKeys(const TzdVal& m) {
    auto keys = std::make_shared<std::vector<TzdVal>>();
    if (m.type == ValType::MAP && m.mapVal) {
        for (const auto& kv : *m.mapVal) {
            keys->push_back(TzdVal(kv.first));
        }
    }
    return TzdVal(keys);
}
#endif

// Main entry helper
inline void init_console() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

} // namespace tzd_rt

using namespace tzd_rt;

#endif // TZD_NATIVE_RUNTIME_HPP
