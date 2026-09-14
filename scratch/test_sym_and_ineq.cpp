#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <map>
#include <cctype>

// ── 1. Real Polynomial Symbolic Simplification ──
// Canonical polynomial: sum_{k=0}^M c_k * x^k
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
        // Output terms in descending order of power
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

// Parser for symbolic polynomial expressions
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

inline std::string real_simplify_symbolic(const std::string& expr) {
    try {
        SymbolicParser p(expr);
        SymbolicPoly poly = p.parse_expr();
        return poly.to_string();
    } catch (...) {
        return expr;
    }
}

// ── 2. Real Inequality Solver ──
struct MathExprEvaluator {
    std::string s;
    size_t pos = 0;
    double x_val = 0.0;
    MathExprEvaluator(std::string str, double x) : s(std::move(str)), x_val(x) {}
    void skip_ws() { while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) pos++; }
    double parse_primary() {
        skip_ws();
        if (pos >= s.size()) return 0.0;
        if (s[pos] == '+') { pos++; return parse_primary(); }
        if (s[pos] == '-') { pos++; return -parse_primary(); }
        if (s[pos] == '(') { pos++; double v = parse_expr(); skip_ws(); if (pos < s.size() && s[pos] == ')') pos++; return v; }
        if (pos < s.size() && (s[pos] == 'x' || s[pos] == 'X')) { pos++; return x_val; }
        if (std::isdigit((unsigned char)s[pos]) || s[pos] == '.') {
            size_t start = pos;
            while (pos < s.size() && (std::isdigit((unsigned char)s[pos]) || s[pos] == '.')) pos++;
            return std::stod(s.substr(start, pos - start));
        }
        return 0.0;
    }
    double parse_power() {
        double left = parse_primary();
        skip_ws();
        if (pos < s.size() && s[pos] == '^') { pos++; return std::pow(left, parse_power()); }
        return left;
    }
    double parse_term() {
        double left = parse_power();
        while (true) {
            skip_ws();
            if (pos < s.size() && s[pos] == '*') { pos++; left *= parse_power(); }
            else if (pos < s.size() && s[pos] == '/') { pos++; double d = parse_power(); left = (d == 0 ? 0 : left / d); }
            else break;
        }
        return left;
    }
    double parse_expr() {
        double left = parse_term();
        while (true) {
            skip_ws();
            if (pos < s.size() && s[pos] == '+') { pos++; left += parse_term(); }
            else if (pos < s.size() && s[pos] == '-') { pos++; left -= parse_term(); }
            else break;
        }
        return left;
    }
};

inline double eval_math(const std::string& expr, double x) {
    MathExprEvaluator p(expr, x);
    return p.parse_expr();
}

inline std::string real_solve_inequality(const std::string& ineq_str) {
    std::string op = "";
    size_t op_pos = std::string::npos;
    if ((op_pos = ineq_str.find("<=")) != std::string::npos) op = "<=";
    else if ((op_pos = ineq_str.find(">=")) != std::string::npos) op = ">=";
    else if ((op_pos = ineq_str.find("<")) != std::string::npos) op = "<";
    else if ((op_pos = ineq_str.find(">")) != std::string::npos) op = ">";
    if (op.empty()) return "Invalid inequality";

    std::string lhs = ineq_str.substr(0, op_pos);
    std::string rhs = ineq_str.substr(op_pos + op.size());
    lhs.erase(0, lhs.find_first_not_of(" \t")); lhs.erase(lhs.find_last_not_of(" \t") + 1);
    rhs.erase(0, rhs.find_first_not_of(" \t")); rhs.erase(rhs.find_last_not_of(" \t") + 1);
    if (rhs.empty()) rhs = "0";

    auto F = [&](double x) -> double {
        return eval_math(lhs, x) - eval_math(rhs, x);
    };

    auto check_cond = [&](double val) -> bool {
        if (op == "<") return val < -1e-9;
        if (op == "<=") return val <= 1e-9;
        if (op == ">") return val > 1e-9;
        if (op == ">=") return val >= -1e-9;
        return false;
    };

    // Find roots of F(x) = 0 via grid sampling and bisection
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

    // Sort and deduplicate roots
    std::sort(roots.begin(), roots.end());
    std::vector<double> uniq_roots;
    for (double r : roots) {
        if (uniq_roots.empty() || std::abs(r - uniq_roots.back()) > 1e-3) {
            uniq_roots.push_back(r);
        }
    }

    // Format double cleanly
    auto fmt_d = [](double d) -> std::string {
        if (std::abs(d - std::round(d)) < 1e-5) return std::to_string((int64_t)std::round(d));
        std::ostringstream ss;
        ss << d;
        return ss.str();
    };

    if (uniq_roots.empty()) {
        if (check_cond(F(0.0))) return "All real numbers";
        else return "No real solution";
    }

    std::vector<std::string> valid_intervals;
    // Test interval (-inf, r0)
    if (check_cond(F(uniq_roots[0] - 5.0))) {
        valid_intervals.push_back(std::string("x ") + (op.find('=') != std::string::npos ? "<= " : "< ") + fmt_d(uniq_roots[0]));
    }
    // Test intervals (r_i, r_{i+1})
    for (size_t i = 0; i + 1 < uniq_roots.size(); ++i) {
        double mid = (uniq_roots[i] + uniq_roots[i+1]) / 2.0;
        if (check_cond(F(mid))) {
            std::string left_op = (op.find('=') != std::string::npos ? " <= " : " < ");
            std::string right_op = (op.find('=') != std::string::npos ? " <= " : " < ");
            valid_intervals.push_back(fmt_d(uniq_roots[i]) + left_op + std::string("x") + right_op + fmt_d(uniq_roots[i+1]));
        }
    }
    // Test interval (r_last, +inf)
    if (check_cond(F(uniq_roots.back() + 5.0))) {
        valid_intervals.push_back(std::string("x ") + (op.find('=') != std::string::npos ? ">= " : "> ") + fmt_d(uniq_roots.back()));
    }

    if (valid_intervals.empty()) return "No real solution";
    std::string res;
    for (size_t i = 0; i < valid_intervals.size(); ++i) {
        if (i > 0) res += " or ";
        res += valid_intervals[i];
    }
    return res;
}

int main() {
    std::cout << "--- Symbolic Simplification Tests ---\n";
    std::cout << "2*x + 3*x -> " << real_simplify_symbolic("2*x + 3*x") << " (expected: 5*x)\n";
    std::cout << "x^2 + 2*x^2 - 4 + 1 -> " << real_simplify_symbolic("x^2 + 2*x^2 - 4 + 1") << " (expected: 3*x^2 - 3)\n";
    std::cout << "(x + 1) * (x - 1) -> " << real_simplify_symbolic("(x + 1) * (x - 1)") << " (expected: x^2 - 1)\n";
    std::cout << "x - x -> " << real_simplify_symbolic("x - x") << " (expected: 0)\n";

    std::cout << "\n--- Inequality Solver Tests ---\n";
    std::cout << "x^2 - 4 < 0 -> " << real_solve_inequality("x^2 - 4 < 0") << " (expected: -2 < x < 2)\n";
    std::cout << "x^2 - 4 > 0 -> " << real_solve_inequality("x^2 - 4 > 0") << " (expected: x < -2 or x > 2)\n";
    std::cout << "2*x - 6 >= 0 -> " << real_solve_inequality("2*x - 6 >= 0") << " (expected: x >= 3)\n";
    std::cout << "x^2 + 1 > 0 -> " << real_solve_inequality("x^2 + 1 > 0") << " (expected: All real numbers)\n";
    return 0;
}
