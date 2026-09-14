# scratch/patch_optimized_bigint.py
import re

with open('TzdNativeRuntime.hpp', 'r', encoding='utf-8') as f:
    code = f.read()

# 1. Update TzdVal::pow and operator+, operator*, operator-
OLD_TZDVAL_OPS = '''    TzdVal operator+(const TzdVal& o) const {
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
    }'''

NEW_TZDVAL_OPS = '''    TzdVal operator+(const TzdVal& o) const;
    TzdVal operator-(const TzdVal& o) const;
    TzdVal operator*(const TzdVal& o) const;
    TzdVal operator/(const TzdVal& o) const;
    TzdVal operator%(const TzdVal& o) const;
    TzdVal pow(const TzdVal& o) const;'''

assert OLD_TZDVAL_OPS in code, "OLD_TZDVAL_OPS not found"
code = code.replace(OLD_TZDVAL_OPS, NEW_TZDVAL_OPS, 1)

# 2. Update powmod
OLD_POWMOD = '''inline TzdVal tzd_builtin_powmod(const std::vector<TzdVal>& args) {
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
}'''

NEW_POWMOD = '''// Forward declaration of powmod (implemented after NativeBigInt)
inline TzdVal tzd_builtin_powmod(const std::vector<TzdVal>& args);'''

assert OLD_POWMOD in code, "OLD_POWMOD not found"
code = code.replace(OLD_POWMOD, NEW_POWMOD, 1)

# 3. Replace NativeBigInt with high-performance Base 10^9 Karatsuba BigInt engine
bigint_pattern = re.compile(r'// ── Real Arbitrary Precision Integer Engine.*?inline TzdVal tzd_builtin_setBigIntMaxDigits\(const std::vector<TzdVal>& = \{\}\) \{ return TzdVal\(true\); \}', re.DOTALL)
match = bigint_pattern.search(code)
assert match, "NativeBigInt section not matched"

NEW_OPTIMIZED_BIGINT_ENGINE = '''// ── High-Performance Native BigInt Engine (Base 10^9 + Karatsuba Multiplication) ──
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
        s.erase(0, s.find_first_not_of(" \\t\\r\\n"));
        s.erase(s.find_last_not_of(" \\t\\r\\n") + 1);
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

inline TzdVal tzd_builtin_getBigIntMaxDigits(const std::vector<TzdVal>& = {}) { return TzdVal(10000000); }
inline TzdVal tzd_builtin_setBigIntMaxDigits(const std::vector<TzdVal>& = {}) { return TzdVal(true); }'''

code = code[:match.start()] + NEW_OPTIMIZED_BIGINT_ENGINE + code[match.end():]

# Make sure _mul128 is available on MSVC x64
if '#include <intrin.h>' not in code:
    code = '#include <intrin.h>\n' + code

with open('TzdNativeRuntime.hpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("Successfully patched Optimized BigInt Engine into TzdNativeRuntime.hpp!")
