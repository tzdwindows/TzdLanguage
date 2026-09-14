# scratch/patch_runtime_helpers.py
import sys

MATH_PARSER_CODE = '''
// ── Lightweight Mathematical Expression Evaluator (for string formulas like "x^2", "sin(x)") ──
struct MathExprParser {
    std::string s;
    size_t pos = 0;
    double x_val = 0.0;

    MathExprParser(std::string str, double x) : s(std::move(str)), x_val(x) {}

    void skip_ws() {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\\t')) pos++;
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
'''

NEW_PLOT_AND_SOLVE = '''
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
            svg << "<svg xmlns=\\"http://www.w3.org/2000/svg\\" width=\\"" << W << "\\" height=\\"" << H << "\\">\\n";
            svg << "<rect width=\\"100%\\" height=\\"100%\\" fill=\\"#1e1e1e\\"/>\\n";
            svg << "<polyline fill=\\"none\\" stroke=\\"#4ec9b0\\" stroke-width=\\"2\\" points=\\"";
            for (size_t i = 0; i < data.size(); ++i) {
                double px = pad + (double)i / (data.size() - 1) * (W - 2 * pad);
                double py = H - pad - (data[i] - minY) / (maxY - minY) * (H - 2 * pad);
                svg << px << "," << py << " ";
            }
            svg << "\\"/>\\n</svg>\\n";
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
    std::cout << "\\n--- ASCII Plot (Min: " << minY << ", Max: " << maxY << ") ---\\n";
    for (int r = 0; r < plotH; ++r) {
        double yVal = maxY - (double)r / (plotH - 1) * (maxY - minY);
        std::cout << std::setw(8) << std::setprecision(2) << yVal << " | " << canvas[r] << "\\n";
    }
    std::cout << "         +" << std::string(plotW, '-') << "\\n";
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
'''

with open('TzdNativeRuntime.hpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace plot & derivative & solveEq
old_plot_marker = "// ── Real Plotting Engine (ASCII Console Chart & SVG File Exporter) ──"
old_plot_end_marker = "inline TzdVal tzd_builtin_solveSym(const std::vector<TzdVal>& args) { return tzd_builtin_solveEq(args); }"

start_idx = content.find(old_plot_marker)
end_idx = content.find(old_plot_end_marker)

if start_idx != -1 and end_idx != -1:
    replacement = MATH_PARSER_CODE + "\n" + NEW_PLOT_AND_SOLVE + "\n"
    content = content[:start_idx] + replacement + content[end_idx:]
    print("Successfully replaced plot, derivative, solveEq!")
else:
    print(f"Failed to find plot markers: {start_idx}, {end_idx}")

# Also update fb_conv2d, fb_max_pool2d, fb_avg_pool2d
OLD_FB_CONV2D = '''inline TzdVal fb_conv2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || args[1].type != ValType::ARRAY) return args.empty() ? TzdVal() : args[0];
    auto inputArr = args[0].arrVal;
    auto kernelArr = args[1].arrVal;
    if (!inputArr || !kernelArr || inputArr->empty() || kernelArr->empty()) return args[0];
    size_t inH = inputArr->size();
    size_t inW = (*inputArr)[0].type == ValType::ARRAY && (*inputArr)[0].arrVal ? (*inputArr)[0].arrVal->size() : 1;
    size_t kH = kernelArr->size();
    size_t kW = (*kernelArr)[0].type == ValType::ARRAY && (*kernelArr)[0].arrVal ? (*kernelArr)[0].arrVal->size() : 1;
    int64_t stride = args.size() > 3 ? args[3].as_int() : 1;
    if (stride < 1) stride = 1;
    auto outMat = std::make_shared<std::vector<TzdVal>>();
    for (size_t r = 0; r + kH <= inH; r += stride) {
        auto outRow = std::make_shared<std::vector<TzdVal>>();
        for (size_t c = 0; c + kW <= inW; c += stride) {
            double sum = 0.0;
            for (size_t kr = 0; kr < kH; ++kr) {
                for (size_t kc = 0; kc < kW; ++kc) {
                    double inVal = (*(*inputArr)[r + kr].arrVal)[c + kc].as_double();
                    double kVal = (*(*kernelArr)[kr].arrVal)[kc].as_double();
                    sum += inVal * kVal;
                }
            }
            if (args.size() > 2 && args[2].type != ValType::NIL) sum += args[2].as_double();
            outRow->push_back(TzdVal(sum));
        }
        outMat->push_back(TzdVal(outRow));
    }
    return TzdVal(outMat);
}'''

NEW_FB_CONV2D = '''inline TzdVal fb_conv2d(const std::vector<TzdVal>& args) {
    if (args.size() < 2 || args[0].type != ValType::ARRAY || args[1].type != ValType::ARRAY) return args.empty() ? TzdVal() : args[0];
    auto inputArr = args[0].arrVal;
    auto kernelArr = args[1].arrVal;
    if (!inputArr || !kernelArr || inputArr->empty() || kernelArr->empty()) return args[0];
    // Drill down extra singleton batch/channel dimensions (e.g. 4D -> 2D)
    while (inputArr && inputArr->size() == 1 && (*inputArr)[0].type == ValType::ARRAY && (*inputArr)[0].arrVal && !(*inputArr)[0].arrVal->empty() && (*(*inputArr)[0].arrVal)[0].type == ValType::ARRAY) {
        inputArr = (*inputArr)[0].arrVal;
    }
    while (kernelArr && kernelArr->size() == 1 && (*kernelArr)[0].type == ValType::ARRAY && (*kernelArr)[0].arrVal && !(*kernelArr)[0].arrVal->empty() && (*(*kernelArr)[0].arrVal)[0].type == ValType::ARRAY) {
        kernelArr = (*kernelArr)[0].arrVal;
    }
    size_t inH = inputArr->size();
    size_t inW = (*inputArr)[0].type == ValType::ARRAY && (*inputArr)[0].arrVal ? (*inputArr)[0].arrVal->size() : 1;
    size_t kH = kernelArr->size();
    size_t kW = (*kernelArr)[0].type == ValType::ARRAY && (*kernelArr)[0].arrVal ? (*kernelArr)[0].arrVal->size() : 1;
    int64_t stride = args.size() > 3 ? args[3].as_int() : 1;
    if (stride < 1) stride = 1;
    auto outMat = std::make_shared<std::vector<TzdVal>>();
    for (size_t r = 0; r + kH <= inH; r += stride) {
        auto outRow = std::make_shared<std::vector<TzdVal>>();
        for (size_t c = 0; c + kW <= inW; c += stride) {
            double sum = 0.0;
            for (size_t kr = 0; kr < kH; ++kr) {
                for (size_t kc = 0; kc < kW; ++kc) {
                    double inVal = ((*inputArr)[r + kr].type == ValType::ARRAY && (*inputArr)[r + kr].arrVal) ? (*(*inputArr)[r + kr].arrVal)[c + kc].as_double() : (*inputArr)[r + kr].as_double();
                    double kVal = ((*kernelArr)[kr].type == ValType::ARRAY && (*kernelArr)[kr].arrVal) ? (*(*kernelArr)[kr].arrVal)[kc].as_double() : (*kernelArr)[kr].as_double();
                    sum += inVal * kVal;
                }
            }
            if (args.size() > 2 && args[2].type != ValType::NIL) sum += args[2].as_double();
            outRow->push_back(TzdVal(sum));
        }
        outMat->push_back(TzdVal(outRow));
    }
    return TzdVal(outMat);
}'''

if OLD_FB_CONV2D in content:
    content = content.replace(OLD_FB_CONV2D, NEW_FB_CONV2D)
    print("Successfully updated fb_conv2d!")
else:
    print("Could not find exact OLD_FB_CONV2D")

with open('TzdNativeRuntime.hpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("Saved updated TzdNativeRuntime.hpp")
