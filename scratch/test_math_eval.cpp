#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cctype>

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

        if (std::isalpha(s[pos])) {
            std::string fn;
            while (pos < s.size() && std::isalpha(s[pos])) fn += s[pos++];
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

        if (std::isdigit(s[pos]) || s[pos] == '.') {
            size_t start = pos;
            while (pos < s.size() && (std::isdigit(s[pos]) || s[pos] == '.')) pos++;
            return std::stod(s.substr(start, pos - start));
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

int main() {
    std::cout << "x^2 at 3: " << eval_simple_math_expr("x^2", 3.0) << " (expected 9)\n";
    std::cout << "x^2 - 4 at 2: " << eval_simple_math_expr("x^2 - 4", 2.0) << " (expected 0)\n";
    std::cout << "sin(x) at 0: " << eval_simple_math_expr("sin(x)", 0.0) << " (expected 0)\n";
    std::cout << "2 * x + 1 at 5: " << eval_simple_math_expr("2 * x + 1", 5.0) << " (expected 11)\n";
    return 0;
}
