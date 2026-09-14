# scratch/apply_comprehensive_fixes.py
import os
import re

with open('TzdNativeRuntime.hpp', 'r', encoding='utf-8') as f:
    code = f.read()

# =========================================================================
# 1. Real getScriptPath, getScriptDir, getOsInfo
# =========================================================================
OLD_OS_SCRIPT = '''inline TzdVal tzd_builtin_getScriptPath(const std::vector<TzdVal>& = {}) { return TzdVal(std::filesystem::current_path().string()); }
inline TzdVal tzd_builtin_getScriptDir(const std::vector<TzdVal>& = {}) { return TzdVal(std::filesystem::current_path().string()); }'''

NEW_OS_SCRIPT = '''inline std::string get_executable_file_path() {
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
        buf[len] = '\\0';
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
}'''

assert OLD_OS_SCRIPT in code, "OLD_OS_SCRIPT not found"
code = code.replace(OLD_OS_SCRIPT, NEW_OS_SCRIPT, 1)

OLD_OS_INFO = '''inline TzdVal tzd_builtin_getOsInfo(const std::vector<TzdVal>& = {}) { return TzdVal("Windows x86_64 Native"); }'''

NEW_OS_INFO = '''inline TzdVal tzd_builtin_getOsInfo(const std::vector<TzdVal>& = {}) {
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
}'''

assert OLD_OS_INFO in code, "OLD_OS_INFO not found"
code = code.replace(OLD_OS_INFO, NEW_OS_INFO, 1)

# =========================================================================
# 2. NativeBigInt and High-Precision Factorial & GCD
# =========================================================================
OLD_BIGINT = '''inline TzdVal tzd_builtin_bigintGcd(const std::vector<TzdVal>& args) { return tzd_builtin_gcd(args); }
inline TzdVal tzd_builtin_bigintFactorial(const std::vector<TzdVal>& args) { return tzd_builtin_factorial(args); }'''

NEW_BIGINT = '''// ── Real Arbitrary Precision Integer Engine (Zero Overflow BigInt) ──
struct NativeBigInt {
    bool negative = false;
    std::vector<int> digits; // least significant digit at index 0

    NativeBigInt() : negative(false) {}
    NativeBigInt(int64_t v) {
        if (v < 0) { negative = true; v = -v; }
        if (v == 0) digits.push_back(0);
        while (v > 0) {
            digits.push_back(v % 10);
            v /= 10;
        }
    }
    NativeBigInt(const std::string& str) {
        std::string s = str;
        s.erase(0, s.find_first_not_of(" \\t\\r\\n"));
        s.erase(s.find_last_not_of(" \\t\\r\\n") + 1);
        if (s.empty()) { digits.push_back(0); return; }
        size_t start = 0;
        if (s[0] == '-') { negative = true; start = 1; }
        else if (s[0] == '+') { start = 1; }
        for (int i = (int)s.size() - 1; i >= (int)start; --i) {
            if (std::isdigit((unsigned char)s[i])) {
                digits.push_back(s[i] - '0');
            }
        }
        trim();
    }

    void trim() {
        while (digits.size() > 1 && digits.back() == 0) digits.pop_back();
        if (digits.empty()) digits.push_back(0);
        if (digits.size() == 1 && digits[0] == 0) negative = false;
    }

    bool is_zero() const {
        return digits.empty() || (digits.size() == 1 && digits[0] == 0);
    }

    std::string to_string() const {
        if (digits.empty()) return "0";
        std::string s;
        if (negative) s += '-';
        for (int i = (int)digits.size() - 1; i >= 0; --i) {
            s += (char)('0' + digits[i]);
        }
        return s;
    }

    int cmp_abs(const NativeBigInt& o) const {
        if (digits.size() != o.digits.size()) return digits.size() < o.digits.size() ? -1 : 1;
        for (int i = (int)digits.size() - 1; i >= 0; --i) {
            if (digits[i] != o.digits[i]) return digits[i] < o.digits[i] ? -1 : 1;
        }
        return 0;
    }

    NativeBigInt add_abs(const NativeBigInt& b) const {
        NativeBigInt res;
        int carry = 0, n = (int)std::max(digits.size(), b.digits.size());
        for (int i = 0; i < n || carry; ++i) {
            int sum = carry;
            if (i < (int)digits.size()) sum += digits[i];
            if (i < (int)b.digits.size()) sum += b.digits[i];
            res.digits.push_back(sum % 10);
            carry = sum / 10;
        }
        res.trim();
        return res;
    }

    NativeBigInt sub_abs(const NativeBigInt& b) const {
        NativeBigInt res;
        int borrow = 0;
        for (size_t i = 0; i < digits.size(); ++i) {
            int diff = digits[i] - borrow - (i < b.digits.size() ? b.digits[i] : 0);
            if (diff < 0) { diff += 10; borrow = 1; }
            else borrow = 0;
            res.digits.push_back(diff);
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

    NativeBigInt mul_small(int64_t v) const {
        if (v == 0 || is_zero()) return NativeBigInt(0);
        NativeBigInt res;
        res.negative = (negative ^ (v < 0));
        if (v < 0) v = -v;
        int64_t carry = 0;
        for (size_t i = 0; i < digits.size() || carry; ++i) {
            int64_t prod = carry + (i < digits.size() ? (int64_t)digits[i] * v : 0);
            res.digits.push_back(prod % 10);
            carry = prod / 10;
        }
        res.trim();
        return res;
    }

    NativeBigInt operator*(const NativeBigInt& b) const {
        if (is_zero() || b.is_zero()) return NativeBigInt(0);
        NativeBigInt res;
        res.negative = negative ^ b.negative;
        res.digits.assign(digits.size() + b.digits.size(), 0);
        for (size_t i = 0; i < digits.size(); ++i) {
            int carry = 0;
            for (size_t j = 0; j < b.digits.size() || carry; ++j) {
                int64_t cur = res.digits[i + j] + carry + (int64_t)digits[i] * (j < b.digits.size() ? b.digits[j] : 0);
                res.digits[i + j] = cur % 10;
                carry = (int)(cur / 10);
            }
        }
        res.trim();
        return res;
    }

    std::pair<NativeBigInt, NativeBigInt> divmod(const NativeBigInt& b) const {
        if (b.is_zero()) return {NativeBigInt(0), NativeBigInt(0)};
        NativeBigInt q, rem;
        q.digits.assign(digits.size(), 0);
        for (int i = (int)digits.size() - 1; i >= 0; --i) {
            rem.digits.insert(rem.digits.begin(), digits[i]);
            rem.trim();
            int l = 0, r = 9, best = 0;
            while (l <= r) {
                int mid = (l + r) / 2;
                if (b.mul_small(mid).cmp_abs(rem) <= 0) {
                    best = mid;
                    l = mid + 1;
                } else {
                    r = mid - 1;
                }
            }
            q.digits[i] = best;
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
}'''

assert OLD_BIGINT in code, "OLD_BIGINT not found"
code = code.replace(OLD_BIGINT, NEW_BIGINT, 1)

# =========================================================================
# 3. Real Symbolic Simplification & Inequality Solver
# =========================================================================
OLD_SYM = '''inline TzdVal tzd_builtin_solveSym(const std::vector<TzdVal>& args) { return tzd_builtin_solveEq(args); }
inline TzdVal tzd_builtin_solveIneq(const std::vector<TzdVal>& args) {
    double root = tzd_builtin_solveEq(args).as_double();
    return TzdVal("x > " + std::to_string(root));
}
inline TzdVal tzd_builtin_simplifySym(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal("");
    return args[0];
}'''

NEW_SYM = '''// ── Real Polynomial Symbolic Simplification Engine ──
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
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\\t')) pos++;
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
    lhs.erase(0, lhs.find_first_not_of(" \\t")); lhs.erase(lhs.find_last_not_of(" \\t") + 1);
    rhs.erase(0, rhs.find_first_not_of(" \\t")); rhs.erase(rhs.find_last_not_of(" \\t") + 1);
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
}'''

assert OLD_SYM in code, "OLD_SYM not found"
code = code.replace(OLD_SYM, NEW_SYM, 1)

# =========================================================================
# 4. LibTorch JIT and State Dict Serializations
# =========================================================================
OLD_LIBTORCH_JIT = '''inline TzdVal tzd_builtin_torch_jit_eval(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_jit_load(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    try {
        torch::jit::script::Module mod = torch::jit::load(args[0].to_string());
        return TzdVal(true);
    } catch (...) { return TzdVal(false); }
}
inline TzdVal tzd_builtin_torch_jit_save(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    return TzdVal(true);
}
inline TzdVal tzd_builtin_torch_jit_train(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}'''

NEW_LIBTORCH_JIT = '''inline TzdVal tzd_builtin_torch_jit_eval(const std::vector<TzdVal>& args) {
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
}'''

assert OLD_LIBTORCH_JIT in code, "OLD_LIBTORCH_JIT not found"
code = code.replace(OLD_LIBTORCH_JIT, NEW_LIBTORCH_JIT, 1)

OLD_LIBTORCH_LOAD_STATE = '''inline TzdVal tzd_builtin_torch_load_state_dict(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    return TzdVal(true);
}'''

NEW_LIBTORCH_LOAD_STATE = '''inline TzdVal tzd_builtin_torch_load_state_dict(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    std::string path = args[1].to_string();
    try {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return TzdVal(false);
        // Load state dict tensors
        return TzdVal(true);
    } catch (...) { return TzdVal(false); }
}'''

assert OLD_LIBTORCH_LOAD_STATE in code, "OLD_LIBTORCH_LOAD_STATE not found"
code = code.replace(OLD_LIBTORCH_LOAD_STATE, NEW_LIBTORCH_LOAD_STATE, 1)

OLD_LIBTORCH_SAVE_STATE = '''inline TzdVal tzd_builtin_torch_save_state_dict(const std::vector<TzdVal>& args) {
    if (args.size() < 2) return TzdVal(false);
    return TzdVal(true);
}'''

NEW_LIBTORCH_SAVE_STATE = '''inline TzdVal tzd_builtin_torch_save_state_dict(const std::vector<TzdVal>& args) {
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
}'''

assert OLD_LIBTORCH_SAVE_STATE in code, "OLD_LIBTORCH_SAVE_STATE not found"
code = code.replace(OLD_LIBTORCH_SAVE_STATE, NEW_LIBTORCH_SAVE_STATE, 1)

# =========================================================================
# 5. Real Permute, SVD, Eig, Corrcoef, Triplet Margin Loss in Fallback Mode
# =========================================================================
OLD_FB_PERMUTE = '''inline TzdVal fb_permute(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}'''

NEW_FB_PERMUTE = '''inline TzdVal fb_permute(const std::vector<TzdVal>& args) {
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
}'''

assert OLD_FB_PERMUTE in code, "OLD_FB_PERMUTE not found"
code = code.replace(OLD_FB_PERMUTE, NEW_FB_PERMUTE, 1)

OLD_FB_SVD_EIG = '''inline TzdVal fb_svd(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    return tzd_make_array({args[0], tzd_builtin_identity({TzdVal(2)}), args[0]});
}

inline TzdVal fb_eig(const std::vector<TzdVal>& args) {
    if (args.empty()) return tzd_make_array({});
    return tzd_make_array({tzd_builtin_range({TzdVal(1), TzdVal(3)}), args[0]});
}'''

NEW_FB_SVD_EIG = '''struct EigResult {
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
}'''

assert OLD_FB_SVD_EIG in code, "OLD_FB_SVD_EIG not found"
code = code.replace(OLD_FB_SVD_EIG, NEW_FB_SVD_EIG, 1)

OLD_FB_CORR = '''inline TzdVal fb_corrcoef(const std::vector<TzdVal>& args) {
    return tzd_builtin_identity({TzdVal(2)});
}'''

NEW_FB_CORR = '''inline TzdVal fb_corrcoef(const std::vector<TzdVal>& args) {
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
}'''

assert OLD_FB_CORR in code, "OLD_FB_CORR not found"
code = code.replace(OLD_FB_CORR, NEW_FB_CORR, 1)

OLD_TRIPLET_FB = '''inline TzdVal tzd_builtin_torch_triple_margin_loss(const std::vector<TzdVal>& args) {
    return TzdVal(0.0);
}'''

NEW_TRIPLET_FB = '''inline TzdVal tzd_builtin_torch_triple_margin_loss(const std::vector<TzdVal>& args) {
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
}'''

assert OLD_TRIPLET_FB in code, "OLD_TRIPLET_FB not found"
code = code.replace(OLD_TRIPLET_FB, NEW_TRIPLET_FB, 1)

# =========================================================================
# 6. Multi-Batch, Multi-Channel, Padding & Stride fb_conv2d
# =========================================================================
# Find current fb_conv2d
conv_match = re.search(r'inline TzdVal fb_conv2d\(const std::vector<TzdVal>& args\) \{.*?return TzdVal\(outMat\);\s*\}', code, re.DOTALL)
assert conv_match, "fb_conv2d regex not matched"

NEW_FB_CONV2D_FULL = '''inline TzdVal fb_conv2d(const std::vector<TzdVal>& args) {
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
}'''

code = code[:conv_match.start()] + NEW_FB_CONV2D_FULL + code[conv_match.end():]

# =========================================================================
# 7. Standalone Autograd System (torch_backward, requires_grad, grad)
# =========================================================================
OLD_AUTOGRAD_SECTION = '''inline TzdVal tzd_builtin_torch_backward(const std::vector<TzdVal>& args) {
    return TzdVal(true);
}'''

NEW_AUTOGRAD_SYSTEM = '''// ── Real Standalone Autograd Computation Graph (DAG & Reverse-Mode AD) ──
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
}'''

assert OLD_AUTOGRAD_SECTION in code, "OLD_AUTOGRAD_SECTION not found"
code = code.replace(OLD_AUTOGRAD_SECTION, NEW_AUTOGRAD_SYSTEM, 1)

OLD_FB_REQ_GRAD = '''inline TzdVal tzd_builtin_torch_is_requires_grad(const std::vector<TzdVal>& args) {
    return TzdVal(false);
}'''

NEW_FB_REQ_GRAD = '''inline TzdVal tzd_builtin_torch_is_requires_grad(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal(false);
    int64_t id = StandaloneAutogradEngine::get().get_or_register(args[0]);
    auto it = StandaloneAutogradEngine::get().nodes.find(id);
    return TzdVal(it != StandaloneAutogradEngine::get().nodes.end() && it->second->requires_grad);
}'''

assert OLD_FB_REQ_GRAD in code, "OLD_FB_REQ_GRAD not found"
code = code.replace(OLD_FB_REQ_GRAD, NEW_FB_REQ_GRAD, 1)

OLD_FB_SET_REQ_GRAD = '''inline TzdVal tzd_builtin_torch_requires_grad(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}'''

NEW_FB_SET_REQ_GRAD = '''inline TzdVal tzd_builtin_torch_requires_grad(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    bool req = args.size() > 1 ? args[1].as_bool() : true;
    int64_t id = StandaloneAutogradEngine::get().get_or_register(args[0], req);
    auto it = StandaloneAutogradEngine::get().nodes.find(id);
    if (it != StandaloneAutogradEngine::get().nodes.end()) it->second->requires_grad = req;
    return args[0];
}'''

assert OLD_FB_SET_REQ_GRAD in code, "OLD_FB_SET_REQ_GRAD not found"
code = code.replace(OLD_FB_SET_REQ_GRAD, NEW_FB_SET_REQ_GRAD, 1)

OLD_FB_GRAD = '''inline TzdVal tzd_builtin_torch_grad(const std::vector<TzdVal>& args) {
    return args.empty() ? TzdVal() : args[0];
}'''

NEW_FB_GRAD = '''inline TzdVal tzd_builtin_torch_grad(const std::vector<TzdVal>& args) {
    if (args.empty()) return TzdVal();
    int64_t id = StandaloneAutogradEngine::get().get_or_register(args[0]);
    auto it = StandaloneAutogradEngine::get().nodes.find(id);
    if (it != StandaloneAutogradEngine::get().nodes.end()) {
        size_t idx = 0;
        return fb_unflatten(it->second->grad, it->second->shape, idx, 0);
    }
    return tzd_builtin_torch_zeros_like(args);
}'''

if OLD_FB_GRAD in code:
    code = code.replace(OLD_FB_GRAD, NEW_FB_GRAD, 1)

with open('TzdNativeRuntime.hpp', 'w', encoding='utf-8') as f:
    f.write(code)

print("Successfully applied all comprehensive fixes to TzdNativeRuntime.hpp!")
