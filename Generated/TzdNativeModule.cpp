// Prevent Windows min/max macros from breaking STL headers
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Prevent <experimental/coroutine> static_assert in C++20 mode.
// The cppcoro library (from vcpkg) uses std::experimental::coroutine_handle
// which triggers a hard error in MSVC C++20 mode. We skip the experimental
// header and create a compatibility alias to the C++20 std::coroutine_handle.
#ifndef _EXPERIMENTAL_COROUTINE_
#define _EXPERIMENTAL_COROUTINE_
#endif
#include <coroutine>
// Compatibility alias: map std::experimental::coroutine_handle to std::coroutine_handle
namespace std { namespace experimental {
    template <typename T = void>
    using coroutine_handle = std::coroutine_handle<T>;
    using suspend_always = std::suspend_always;
    using suspend_never = std::suspend_never;
}}
#endif

#include "../Res/TzdStrings.h"

#include "fintamath/expressions/Expression.hpp"
#include "fintamath/expressions/ExpressionFunctions.hpp"
#include "fintamath/literals/Variable.hpp"
#include "fintamath/literals/constants/E.hpp"
#include "fintamath/numbers/Real.hpp"

#include "TzdNativeModule.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>
#include <ctime>
#include <filesystem>
#include <set>
#include <vector>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <random>
#include <regex>
#include <cmath>
#include <limits>

#include "TzdInterpreter.h"
#include "TzdOop.h"
#include "TzdPyTorch.h"

namespace fs = std::filesystem;

struct ThreadData {
    TzdInterpreter* parentInterp = nullptr;
    TzdValue target;
    std::thread* sysThread = nullptr;
    TzdInterpreter* childInterp = nullptr;
    std::mutex mutex;
    bool isDetached = false;
    bool threadFinished = false;

    void cleanup() {
        if (sysThread) delete sysThread;
        if (childInterp) delete childInterp;
        delete this;
    }
};

TzdValue sys_thread_start(const std::vector<TzdValue>& args) {
    if (args.empty() || (args[0].type != TzdValue::FUNCTION && args[0].type != TzdValue::NATIVE_FUNCTION)) {
        return TzdValue::Error("Thread start requires a function target.");
    }

    ThreadData* data = new ThreadData();
    data->parentInterp = g_CurrentInterpreter;
    data->target = args[0];

    // 关键：为子线程创建一个轻量级解释器上下文
    data->childInterp = new TzdInterpreter();

    // 复制父线程的全局作用域（包含已注册的类、全局变量和 JIT 函数指针）
    if (g_CurrentInterpreter && !g_CurrentInterpreter->scopes.empty()) {
        data->childInterp->scopes[0] = g_CurrentInterpreter->scopes[0];
    }

    data->sysThread = new std::thread([data]() {
        g_CurrentInterpreter = data->childInterp;
        try {
            data->childInterp->callFunction(data->target, {});
        }
        catch (...) {}

        std::lock_guard<std::mutex> lock(data->mutex);
        data->threadFinished = true;
        if (data->isDetached) {
            data->cleanup();
        }
        });

    return TzdValue((void*)data);
}

TzdValue sys_thread_join(const std::vector<TzdValue>& args) {
    if (args.empty() || args[0].type != TzdValue::POINTER || !args[0].ptrVal) {
        return TzdValue(false);
    }

    ThreadData* data = (ThreadData*)args[0].ptrVal;
    if (data->sysThread && data->sysThread->joinable()) {
        data->sysThread->join();
    }

    data->cleanup(); // 安全释放内存
    return TzdValue(true);
}

TzdValue sys_thread_detach(const std::vector<TzdValue>& args) {
    if (args.empty() || args[0].type != TzdValue::POINTER || !args[0].ptrVal) {
        return TzdValue(false);
    }

    ThreadData* data = (ThreadData*)args[0].ptrVal;
    std::lock_guard<std::mutex> lock(data->mutex);

    data->isDetached = true;
    if (data->sysThread) {
        data->sysThread->detach();
    }

    if (data->threadFinished) {
        // 如果在调用 detach 之前子线程已经运行完毕
        data->cleanup();
    }
    return TzdValue(true);
}

// --- ???????????? ---
std::string TzdNativeModule::AnsiToUtf8(const std::string& str) {
#ifdef _WIN32
    int nwLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
    if (nwLen <= 0) return str;
    std::wstring wstr(nwLen, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wstr[0], nwLen);
    int nLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (nLen <= 0) return str;
    std::string ret(nLen - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &ret[0], nLen, NULL, NULL);
    return ret;
#else
    return str;
#endif
}

static double valToDouble(const TzdValue& v) {
    switch (v.type) {
    case TzdValue::DOUBLE:
    case TzdValue::FLOAT:
        return v.dVal;
    case TzdValue::LONG:
    case TzdValue::INT:
    case TzdValue::SHORT:
    case TzdValue::SBYTE:
        return (double)v.lVal;
    case TzdValue::ULONG:
    case TzdValue::UINT:
    case TzdValue::BYTE:
        return (double)v.ulVal;
    case TzdValue::BOOL:
        return v.bVal ? 1.0 : 0.0;
    case TzdValue::STRING:
        try { return std::stod(v.sVal); }
        catch (...) { return 0.0; }
    default:
        return 0.0;
    }
}

// ?????????????????????? double ??????????????? 0.33333 -> 1/3 ??????
static std::string doubleToFractionStr(double val, double tolerance = 1e-5) {
    if (std::isnan(val)) return "NaN";
    if (std::isinf(val)) return "Infinity";

    bool negative = val < 0;
    if (negative) val = -val;

    long long h1 = 1, h2 = 0, k1 = 0, k2 = 1;
    double b = val;

    for (int i = 0; i < 15; ++i) {
        long long a = (long long)std::floor(b);
        long long aux_h = h1; h1 = a * h1 + h2; h2 = aux_h;
        long long aux_k = k1; k1 = a * k1 + k2; k2 = aux_k;

        if (k1 == 0) break;
        if (std::abs(val - (double)h1 / k1) <= tolerance) break;

        double diff = b - a;
        if (std::abs(diff) < 1e-7) break;
        b = 1.0 / diff;
    }

    if (k1 == 0) return "0/1";
    long long final_num = negative ? -h1 : h1;
    if (k1 == 1) return std::to_string(final_num);
    return std::to_string(final_num) + "/" + std::to_string(k1);
}

// --- ?????Eigen ??? ---
Eigen::MatrixXd TzdNativeModule::toEigen(const TzdValue& arr) {
    if (arr.type != TzdValue::ARRAY || arr.arrVal.empty()) return Eigen::MatrixXd(0, 0);
    int rows = (int)arr.arrVal.size();
    int cols = (arr.arrVal[0].type == TzdValue::ARRAY) ? (int)arr.arrVal[0].arrVal.size() : 1;
    Eigen::MatrixXd mat(rows, cols);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (arr.arrVal[i].type == TzdValue::ARRAY)
                mat(i, j) = (j < (int)arr.arrVal[i].arrVal.size()) ? valToDouble(arr.arrVal[i].arrVal[j]) : 0.0;
            else
                mat(i, j) = (j == 0) ? valToDouble(arr.arrVal[i]) : 0.0;
        }
    }
    return mat;
}

TzdValue TzdNativeModule::fromEigen(const Eigen::MatrixXd& mat) {
    std::vector<TzdValue> resRows;
    for (int i = 0; i < mat.rows(); ++i) {
        std::vector<TzdValue> rowElements;
        for (int j = 0; j < mat.cols(); ++j) rowElements.push_back(TzdValue(mat(i, j)));
        resRows.push_back(TzdValue(rowElements));
    }
    return TzdValue(resRows);
}

// --- ????? ---
void TzdNativeModule::init(TzdInterpreter* interp) {
    regInterpreterState(interp);
    regMath(interp);
    regMatrix(interp);
    regSystem(interp);
    regRuntime(interp);
    regIO(interp);
    regPlot(interp);
    regString(interp);
    regArray(interp);
    regJson(interp);
    regFileSystem(interp);
    regConv(interp);
    regExtraMath(interp);
    regExtended(interp);
    TzdPyTorch::init(interp);
    interp->addIncludePath("stdlib");
}

void TzdNativeModule::regMatrix(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
        };
    reg("identity", [](auto args) { return fromEigen(Eigen::MatrixXd::Identity(args.empty() ? 1 : (int)valToDouble(args[0]), args.empty() ? 1 : (int)valToDouble(args[0]))); });
    reg("zeros", [](auto args) { return fromEigen(Eigen::MatrixXd::Zero((int)valToDouble(args[0]), args.size() > 1 ? (int)valToDouble(args[0]) : (int)valToDouble(args[0]))); });
    reg("ones", [](auto args) { return fromEigen(Eigen::MatrixXd::Ones((int)valToDouble(args[0]), args.size() > 1 ? (int)valToDouble(args[0]) : (int)valToDouble(args[0]))); });
    reg("matrixMul", [](auto args) { return (args.size() < 2) ? TzdValue("Error") : fromEigen(toEigen(args[0]) * toEigen(args[1])); });
    reg("transpose", [](auto args) { return args.empty() ? TzdValue() : fromEigen(toEigen(args[0]).transpose()); });
    reg("inverse", [](auto args) { auto m = toEigen(args[0]); return (m.rows() == m.cols()) ? fromEigen(m.inverse()) : TzdValue("Error: Not square"); });
    reg("det", [](auto args) { return TzdValue(toEigen(args[0]).determinant()); });
    reg("trace", [](auto args) { return TzdValue(toEigen(args[0]).trace()); });
    reg("rank", [](auto args) { return TzdValue((double)toEigen(args[0]).fullPivLu().rank()); });
    reg("solve", [](auto args) { return (args.size() < 2) ? TzdValue() : fromEigen(toEigen(args[0]).colPivHouseholderQr().solve(toEigen(args[1]))); });
    reg("norm", [](auto args) { return TzdValue(toEigen(args[0]).norm()); });
    reg("dot", [](auto args) { if (args.size() < 2) return TzdValue(0.0); return TzdValue(toEigen(args[0]).col(0).dot(toEigen(args[1]).col(0))); });
    reg("reshape", [](auto args) {
        if (args.size() < 3) return TzdValue("Error");
        Eigen::MatrixXd m = toEigen(args[0]);
        int r = (int)valToDouble(args[1]), c = (int)valToDouble(args[2]);
        if (r * c != m.size()) return TzdValue("Error: Size mismatch");
        m.resize(r, c); return fromEigen(m);
    });
}

// --- 1. ????????????????????? ---
void TzdNativeModule::regInterpreterState(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
        };

    reg("addIncludePath", [interp](auto args) {
        if (!args.empty()) interp->addIncludePath(args[0].sVal);
        return TzdValue();
        });

    reg("getScriptPath", [interp](auto args) {
        if (interp->m_scriptPathStack.empty()) return TzdValue("memory");
        return TzdValue(interp->m_scriptPathStack.back().string());
        });

    reg("getScriptDir", [interp](auto args) {
        if (interp->m_scriptPathStack.empty()) return TzdValue(".");
        return TzdValue(interp->m_scriptPathStack.back().parent_path().string());
        });
}

void TzdNativeModule::regRuntime(TzdInterpreter* interp) {
    TzdClassDef* existing = TzdOopManager::getClass("Runtime");
    if (existing) {
        interp->setGlobalVariable("Runtime", TzdValue(existing));
        return;
    }

    TzdClassDef* runtimeCls = new TzdClassDef("Runtime");
    ClassMethod captureTrace;
    captureTrace.name = "captureStackTrace";
    captureTrace.isStatic = true;
    captureTrace.isNative = true;
    captureTrace.sourceFile = "native";
    captureTrace.line = 0;

    captureTrace.nativeWrapper = [interp](const std::vector<TzdValue>& args) -> TzdValue {
        (void)args;
        std::ostringstream oss;
        oss << "Stack trace:\n";
        for (const auto& frame : interp->m_callStackFrames) {
            oss << "  at " << frame << "\n";
        }
        if (interp->m_callStackFrames.empty()) {
            oss << "  at <entry>\n";
        }
        return TzdValue(oss.str());
        };
    runtimeCls->methods["captureStackTrace"] = captureTrace;
    TzdOopManager::registerClass(runtimeCls);
    interp->setGlobalVariable("Runtime", TzdValue(runtimeCls));
}

// --- 2. ??????? ---
void TzdNativeModule::regMath(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
        };

    // ??????????????
    reg("abs", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::abs(valToDouble(args[0]))); });
    reg("sqrt", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::sqrt(valToDouble(args[0]))); });
    reg("sin", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::sin(valToDouble(args[0]))); });
    reg("cos", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::cos(valToDouble(args[0]))); });
    reg("tan", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::tan(valToDouble(args[0]))); });
    reg("asin", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::asin(valToDouble(args[0]))); });
    reg("acos", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::acos(valToDouble(args[0]))); });
    reg("atan", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::atan(valToDouble(args[0]))); });
    reg("log", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::log(valToDouble(args[0]))); });
    reg("log10", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::log10(valToDouble(args[0]))); });
    reg("exp", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::exp(valToDouble(args[0]))); });

    reg("pow", [](auto args) {
        return TzdValue(args.size() < 2 ? 0.0 : std::pow(valToDouble(args[0]), valToDouble(args[1])));
        });

    reg("ceil", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::ceil(valToDouble(args[0]))); });
    reg("floor", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::floor(valToDouble(args[0]))); });
    reg("round", [](auto args) { return TzdValue(args.empty() ? 0.0 : std::round(valToDouble(args[0]))); });

    reg("factorial", [](auto args) {
        int n = args.empty() ? 0 : (int)valToDouble(args[0]);
        double res = 1.0; for (int i = 2; i <= n; ++i) res *= i;
        return TzdValue(res);
        });

    // ???????toFraction (?????????????????????????)
    reg("toFraction", [](auto args) {
        if (args.empty()) return TzdValue("0/1");
        double val = valToDouble(args[0]);
        double tol = (args.size() > 1) ? valToDouble(args[1]) : 1e-5;
        return TzdValue(doubleToFractionStr(val, tol));
        });

    // ??????? AST ??????? Lambda
    auto evalFunc = [interp](const TzdValue& func, double x) -> double {
        if (func.type != TzdValue::FUNCTION && func.type != TzdValue::NATIVE_FUNCTION) return 0.0;
        std::unordered_map<std::string, TzdValue> callScope;
        if (!func.params.empty()) callScope[func.params[0]] = TzdValue(x);
        interp->scopes.push_back(callScope);
        TzdValue res(0.0);
        try {
            std::any v = interp->visit(func.funcBody);
            if (v.has_value()) res = std::any_cast<TzdValue>(v);
        }
        catch (const TzdReturnException& e) { res = e.value; }
        catch (...) { res = TzdValue(0.0); }
        interp->scopes.pop_back();
        return valToDouble(res);
        };

    // ????????
    reg("derivative", [evalFunc](auto args) -> TzdValue {
        if (args.size() < 2 || (args[0].type != TzdValue::FUNCTION && args[0].type != TzdValue::NATIVE_FUNCTION)) return TzdValue(0.0);
        TzdValue func = args[0];
        double x = valToDouble(args[1]), h = 1e-4;
        double df = (-evalFunc(func, x + 2 * h) + 8 * evalFunc(func, x + h) - 8 * evalFunc(func, x - h) + evalFunc(func, x - 2 * h)) / (12 * h);
        return TzdValue(df);
        });

    // ????????????
    reg("solveEq", [evalFunc](auto args) -> TzdValue {
        if (args.empty() || (args[0].type != TzdValue::FUNCTION && args[0].type != TzdValue::NATIVE_FUNCTION)) {
            return TzdValue("Error: solveEq requires a function as the 1st argument.");
        }
        TzdValue func = args[0];
        double low = (args.size() > 1) ? valToDouble(args[1]) : -100.0;
        double high = (args.size() > 2) ? valToDouble(args[2]) : 100.0;
        bool asFraction = (args.size() > 3) ? args[3].bVal : false;

        std::vector<double> roots;
        double step = (high - low) / 1000.0;
        if (step <= 0) step = 0.1;

        double x1 = low;
        double y1 = evalFunc(func, x1);

        for (double x2 = low + step; x2 <= high; x2 += step) {
            double y2 = evalFunc(func, x2);
            if (std::isnan(y1) || std::isinf(y1) || std::isnan(y2) || std::isinf(y2)) {
                x1 = x2; y1 = y2; continue;
            }
            if (y1 * y2 <= 0.0) {
                double rl = x1, rh = x2;
                for (int it = 0; it < 40; ++it) {
                    double m = rl + (rh - rl) / 2.0;
                    double ym = evalFunc(func, m);
                    if (std::abs(ym) < 1e-7) { rl = m; break; }
                    if (evalFunc(func, rl) * ym <= 0.0) rh = m;
                    else rl = m;
                }
                if (roots.empty() || std::abs(roots.back() - rl) > 1e-3) {
                    roots.push_back(rl);
                }
            }
            x1 = x2; y1 = y2;
        }

        if (roots.empty()) return TzdValue("No real roots found in range");

        std::vector<TzdValue> resArr;
        for (double r : roots) {
            if (asFraction) resArr.push_back(TzdValue(doubleToFractionStr(r)));
            else resArr.push_back(TzdValue(r));
        }
        return TzdValue(resArr);
        });

    reg("solveSym", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::STRING) {
            return TzdValue("Error: solveSym requires an equation string as the 1st argument. (e.g., 'x - 1 = 0')");
        }

        std::string eqStr = args[0].sVal;
        // ???????????????????????? "x" ???????
        std::string varStr = (args.size() > 1) ? args[1].sVal : "x";

        try {
            // 1. ????? Fintamath ?????
            fintamath::Expression expr(eqStr);

            // 2. ?????????Fintamath ?? solve ????? 1 ???????????????????????
            fintamath::Expression solutionsExpr = fintamath::solve(expr);

            // 3. ?????????????????
            std::string solutionsStr = solutionsExpr.toString();

            // ?????????????????????????????????? Fintamath ??????????????
            if (solutionsStr.empty()) {
                return TzdValue("No symbolic solutions found.");
            }

            std::vector<TzdValue> resArr;

            // 4. ???? Fintamath ???????????????????? "x = -7 | x = 7"??
            std::stringstream ss(solutionsStr);
            std::string singleSol;
            while (std::getline(ss, singleSol, '|')) {
                size_t first = singleSol.find_first_not_of(" ");
                size_t last = singleSol.find_last_not_of(" ");
                if (first != std::string::npos && last != std::string::npos) {
                    singleSol = singleSol.substr(first, (last - first + 1));
                }
                resArr.push_back(TzdValue(singleSol));
            }
            if (resArr.empty()) {
                return TzdValue("No symbolic solutions found.");
            }

            return TzdValue(resArr);
        }
        catch (const std::exception& e) {
            return TzdValue(std::string("Symbolic Solver Error: ") + e.what());
        }
        catch (...) {
            return TzdValue("Unknown error in Fintamath solver.");
        }
        });

    reg("simplifySym", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::STRING) {
            return TzdValue("Error: simplifySym requires an expression string. (e.g., '2*x + 3*x')");
        }

        try {
            // Fintamath ?????????????????????
            fintamath::Expression expr(args[0].sVal);
            return TzdValue(expr.toString());
        }
        catch (const std::exception& e) {
            return TzdValue(std::string("Symbolic Simplify Error: ") + e.what());
        }
        });
    // ?????????????????
    reg("solveIneq", [evalFunc](auto args) -> TzdValue {
        if (args.size() < 2 || (args[0].type != TzdValue::FUNCTION && args[0].type != TzdValue::NATIVE_FUNCTION)) {
            return TzdValue("Error: solveIneq(func, opStr, [low, high, asFraction])");
        }
        TzdValue func = args[0];
        std::string op = args[1].sVal;
        double low = (args.size() > 2) ? valToDouble(args[2]) : -100.0;
        double high = (args.size() > 3) ? valToDouble(args[3]) : 100.0;
        bool asFraction = (args.size() > 4) ? args[4].bVal : false;

        auto checkOp = [](double val, const std::string& op) {
            if (op == ">") return val > 1e-7;
            if (op == "<") return val < -1e-7;
            if (op == ">=") return val >= -1e-7;
            if (op == "<=") return val <= 1e-7;
            return false;
            };

        std::vector<double> roots;
        double step = (high - low) / 1000.0;
        if (step <= 0) step = 0.1;
        double x1 = low;
        double y1 = evalFunc(func, x1);
        for (double x2 = low + step; x2 <= high; x2 += step) {
            double y2 = evalFunc(func, x2);
            if (!std::isnan(y1) && !std::isnan(y2) && !std::isinf(y1) && !std::isinf(y2)) {
                if (y1 * y2 <= 0.0) {
                    double rl = x1, rh = x2;
                    for (int it = 0; it < 40; ++it) {
                        double m = rl + (rh - rl) / 2.0;
                        double ym = evalFunc(func, m);
                        if (std::abs(ym) < 1e-7) { rl = m; break; }
                        if (evalFunc(func, rl) * ym <= 0.0) rh = m;
                        else rl = m;
                    }
                    if (roots.empty() || std::abs(roots.back() - rl) > 1e-3) roots.push_back(rl);
                }
            }
            x1 = x2; y1 = y2;
        }

        std::vector<std::pair<double, double>> validIntervals;

        double first_test = roots.empty() ? (low + high) / 2.0 : (low + roots[0]) / 2.0;
        if (checkOp(evalFunc(func, first_test), op)) {
            validIntervals.push_back({ low, roots.empty() ? high : roots[0] });
        }
        for (size_t i = 0; i + 1 < roots.size(); ++i) {
            double mid_test = (roots[i] + roots[i + 1]) / 2.0;
            if (checkOp(evalFunc(func, mid_test), op)) {
                validIntervals.push_back({ roots[i], roots[i + 1] });
            }
        }
        if (!roots.empty()) {
            double last_test = (roots.back() + high) / 2.0;
            if (checkOp(evalFunc(func, last_test), op)) {
                validIntervals.push_back({ roots.back(), high });
            }
        }

        if (validIntervals.empty()) return TzdValue("No solution in range");

        auto formatVal = [asFraction](double v) {
            if (asFraction) return doubleToFractionStr(v);
            std::stringstream ss; ss << std::setprecision(4) << v; return ss.str();
            };

        std::string resultStr = "";
        for (size_t i = 0; i < validIntervals.size(); ++i) {
            if (i > 0) resultStr += " or ";
            double l = validIntervals[i].first;
            double h = validIntervals[i].second;

            if (std::abs(l - low) < 1e-3) {
                resultStr += std::string("x ") + (op.find('=') != std::string::npos ? "<= " : "< ") + formatVal(h);
            }
            else if (std::abs(h - high) < 1e-3) {
                resultStr += std::string("x ") + (op.find('=') != std::string::npos ? ">= " : "> ") + formatVal(l);
            }
            else {
                resultStr += formatVal(l) + (op.find('=') != std::string::npos ? " <= x <= " : " < x < ") + formatVal(h);
            }
        }
        return TzdValue(resultStr);
        });
}

// --- 4. ????????? ---
void TzdNativeModule::regSystem(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
        };

    reg("time", [](auto args) { return TzdValue((double)std::time(nullptr)); });
    reg("clock", [](auto args) {
        auto now = std::chrono::high_resolution_clock::now();
        static const auto start_time = now;
        std::chrono::duration<double, std::milli> ms_duration = now - start_time;
        return TzdValue(ms_duration.count());
        });
    reg("sleep", [](auto args) { if (!args.empty()) std::this_thread::sleep_for(std::chrono::milliseconds((int)valToDouble(args[0]))); return TzdValue(); });
    reg("exit", [](auto args) { std::exit(args.empty() ? 0 : (int)valToDouble(args[0])); return TzdValue(); });
    reg("getSymbols", [interp](auto args) {
        (void)args;

        TzdValue result;
        result.type = TzdValue::MAP;

        TzdValue classesArray;
        classesArray.type = TzdValue::ARRAY;

        TzdValue functionsArray;
        functionsArray.type = TzdValue::ARRAY;

        std::set<std::string> uniqueClasses;
        std::set<std::string> uniqueFunctions;

        // 遍历所有作用域链层级，收集符号
        for (const auto& scope : interp->scopes) {
            for (const auto& [name, val] : scope) {
                if (val.type == TzdValue::CLASS_DEF) {
                    uniqueClasses.insert(name);
                }
                else if (val.type == TzdValue::FUNCTION || val.type == TzdValue::NATIVE_FUNCTION) {
                    uniqueFunctions.insert(name);
                }
            }
        }

        // 装入 TzdValue 格式的 Array 列表
        for (const auto& name : uniqueClasses) {
            classesArray.arrVal.push_back(TzdValue(name));
        }
        for (const auto& name : uniqueFunctions) {
            functionsArray.arrVal.push_back(TzdValue(name));
        }

        // 写入返回的 Map 结构中
        result.mapVal["classes"] = classesArray;
        result.mapVal["functions"] = functionsArray;

        return result;
        });

    reg("len", [](auto args) {
        if (args.empty()) return TzdValue(0.0);
        if (args[0].type == TzdValue::ARRAY) return TzdValue((double)args[0].arrVal.size());
        if (args[0].type == TzdValue::STRING) return TzdValue((double)args[0].sVal.length());
        return TzdValue(0.0);
        });
    reg("sys_thread_start", &sys_thread_start);
    reg("sys_thread_join", &sys_thread_join);
    reg("sys_thread_detach", &sys_thread_detach);
    reg("type", [](auto args) {
        if (args.empty()) return TzdValue("NONE");
        switch (args[0].type) {
        case TzdValue::FLOAT: case TzdValue::DOUBLE: return TzdValue("FLOAT");
        case TzdValue::STRING: return TzdValue("STRING");
        case TzdValue::BOOL: return TzdValue("BOOL");
        case TzdValue::FUNCTION: return TzdValue("FUNCTION");
        case TzdValue::NATIVE_FUNCTION: return TzdValue("FUNCTION");
        case TzdValue::ARRAY: return TzdValue("ARRAY");
        case TzdValue::MAP: return TzdValue("MAP");
        case TzdValue::INSTANCE: return TzdValue("INSTANCE");
        case TzdValue::CLASS_DEF: return TzdValue("CLASS");
        case TzdValue::POINTER: return TzdValue("POINTER");
        case TzdValue::TENSOR: return TzdValue("TENSOR");
        case TzdValue::NONE: return TzdValue("NONE");
        default: return TzdValue("INT");
        }
        });

    reg("parseInt", [](auto args) {
        if (args.size() < 2) return TzdValue(0.0);
        try { return TzdValue((double)std::stoll(args[0].sVal, nullptr, (int)valToDouble(args[1]))); }
        catch (...) { return TzdValue(0.0); }
        });

    reg("toString", [interp](auto args) {
        if (args.empty()) return TzdValue("");
        if (args.size() < 2) return TzdValue(interp->getAsString(args[0]));

        long long val = std::abs((long long)valToDouble(args[0]));
        int base = (int)valToDouble(args[1]);
        if (base < 2 || base > 36) return TzdValue("Error: Base out of range (2-36)");

        const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string s;
        do {
            s = digits[val % base] + s;
            val /= base;
        } while (val > 0);

        return TzdValue((valToDouble(args[0]) < 0 ? "-" : "") + s);
        });

    reg("getClassInfo", [interp](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("getClassInfo ?????????? (??????????????)");
        TzdClassDef* targetClass = nullptr;
        TzdValue& input = args[0];

        if (input.type == TzdValue::INSTANCE && input.instanceVal) targetClass = input.instanceVal->definition;
        else if (input.type == TzdValue::CLASS_DEF) targetClass = input.classDefVal;
        else if (input.type == TzdValue::STRING) targetClass = TzdOopManager::getClass(input.sVal);

        if (!targetClass) return TzdValue::Error("????????????????");

        std::unordered_map<std::string, TzdValue> info;
        info["name"] = TzdValue(targetClass->fullName);
        info["parent"] = TzdValue(targetClass->parentName.empty() ? "None" : targetClass->parentName);

        std::vector<TzdValue> fieldsList;
        for (auto const& [name, field] : targetClass->fields) {
            std::unordered_map<std::string, TzdValue> fMap;
            fMap["name"] = TzdValue(name);
            fMap["type"] = TzdValue(field.type);
            fMap["isStatic"] = TzdValue((bool)field.isStatic);
            fieldsList.push_back(TzdValue(fMap));
        }
        info["fields"] = TzdValue(fieldsList);

        std::vector<TzdValue> methodsList;
        for (auto const& [name, method] : targetClass->methods) {
            methodsList.push_back(TzdValue(name + "()"));
        }
        info["methods"] = TzdValue(methodsList);

        return TzdValue(info);
        });

    reg("bit", [](auto args) {
        if (args.size() < 2) return TzdValue(0.0);
        long long a = (long long)valToDouble(args[0]); std::string op = args[1].sVal;
        if (op == "NOT") return TzdValue((double)~a);
        if (args.size() < 3) return TzdValue(0.0);
        long long b = (long long)valToDouble(args[2]);
        if (op == "AND") return TzdValue((double)(a & b));
        if (op == "OR")  return TzdValue((double)(a | b));
        if (op == "XOR") return TzdValue((double)(a ^ b));
        if (op == "LSH") return TzdValue((double)(a << b));
        if (op == "RSH") return TzdValue((double)(a >> b));
        return TzdValue(0.0);
        });

    reg("hexDump", [](auto args) {
        if (args.empty()) return TzdValue("");
        std::stringstream ss; ss << std::hex << std::setfill('0');
        for (unsigned char c : args[0].sVal) ss << std::setw(2) << (int)c << " ";
        return TzdValue(ss.str());
        });

    reg("getFunctions", [interp](auto args) {
        std::vector<TzdValue> names; std::set<std::string> seen;
        for (auto it = interp->scopes.rbegin(); it != interp->scopes.rend(); ++it) {
            for (auto const& [name, val] : *it) {
                if ((val.type == TzdValue::FUNCTION || val.type == TzdValue::NATIVE_FUNCTION) && seen.find(name) == seen.end()) {
                    names.push_back(TzdValue(name)); seen.insert(name);
                }
            }
        }
        return TzdValue(names);
        });

    reg("getNativeFunctions", [](auto args) -> TzdValue {
        // ???????????????????
        struct NativeMeta { std::string name; std::string params; std::string desc; };
        static const std::vector<NativeMeta> nativeRegistry = {
            // ????????????
            { "addIncludePath", "path", "?????????????????(Include)????????" },
            { "getScriptPath", "", "?????????????????????????????" },
            { "getScriptDir", "", "???????????????????????????" },
            // ??????? (Eigen)
            { "identity", "[size=1]", "????????????????????" },
            { "zeros", "rows, [cols]", "???????????????????" },
            { "ones", "rows, [cols]", "???????????????????" },
            { "matrixMul", "matA, matB", "???????????????" },
            { "transpose", "matrix", "??????????????????" },
            { "inverse", "matrix", "?????????????????????????" },
            { "det", "matrix", "???????????? (Determinant)" },
            { "trace", "matrix", "???????? (Trace???????????????)" },
            { "rank", "matrix", "????????LU????????????????" },
            { "solve", "matA, matB", "???????????? A * X = B" },
            { "norm", "matrix", "???????????? Frobenius ????" },
            { "dot", "vecA, vecB", "??????????????????????" },
            { "reshape", "matrix, rows, cols", "????????????????????????????????????" },
            // ???????
            { "abs", "val", "????????" }, { "sqrt", "val", "?????????????" },
            { "sin", "rad", "???????(?????)" }, { "cos", "rad", "???????(?????)" }, { "tan", "rad", "????????(?????)" },
            { "asin", "val", "?????????" }, { "acos", "val", "?????????" }, { "atan", "val", "??????????" },
            { "log", "val", "?? e ???????????" }, { "log10", "val", "?? 10 ??????????" }, { "exp", "val", "??????? e^x" },
            { "pow", "base, exp", "?????????? base^exp" },
            { "ceil", "val", "???????" }, { "floor", "val", "???????" }, { "round", "val", "???????????" },
            { "factorial", "n", "阶乘 (n!)" },
            { "bigintFactorial", "n", "大数阶乘（任意精度 n!）" },
            { "powmod", "base, exp, mod", "模幂运算 base^exp mod m" },
            { "isBigint", "x", "检查 x 是否为 BIGINT 类型" },
            { "bigintGcd", "a, b", "大整数最大公约数" },
            { "setBigIntMaxDigits", "n", "设置 BIGINT 最大位数（防止内存耗尽）" },
            { "getBigIntMaxDigits", "", "获取 BIGINT 最大位数限制" },
            { "bigint", "x", "将数值转换为 BIGINT 类型" },
            { "rational", "num, den", "创建分数 num/den" },
            { "toFraction", "val, [tolerance=1e-5]", "将浮点数转为精确分数" },
            { "derivative", "func, x", "?????????????????????????????? x ???????????" },
            { "solveEq", "func, [low=-100], [high=100], [asFraction=false]", "??????????????????????????????? func(x) = 0 ???" },
            { "solveSym", "eqStr, [var='x']", "???? Fintamath ??????????????????????????????" },
            { "simplifySym", "exprStr", "???? Fintamath ????????????????????????????????" },
            { "solveIneq", "func, opStr, [low=-100], [high=100], [asFraction=false]", "??????????????????????? opStr ?????????????????" },
            // ???????????
            { "time", "", "????????????????????????????" },
            { "clock", "", "????????????????????????????????????" },
            { "sleep", "ms", "?????????????/??????????????" },
            { "exit", "[code=0]", "?????????????????????" },
            { "len", "container", "??????????????????????????" },
            { "type", "val", "?????????????????????????" },
            { "parseInt", "str, base", "??????????(2-36)????????????????????" },
            { "toString", "val, [base]", "???????????????????????(2-36)???????????" },
            { "getClassInfo", "target", "?????????????????????????????? OOP ???????" },
            { "bit", "a, opStr, [b]", "??????????????????? NOT, AND, OR, XOR, LSH, RSH" },
            { "hexDump", "str", "???????????????????????????????? Hex Dump ?????" },
            { "getFunctions", "", "???????????????????????????????????????" },
            { "getNativeFunctions", "", "???????????????????????? C++ ??????????" },
            // IO????
            { "input", "[prompt]", "?????????????????????????????? ANSI ?????????" },
            { "readFile", "path", "????????????????????????????????????" },
            { "writeFile", "path, content", "??????????????????????????????????" },
            { "plot", "funcs..., start, end, [step]", "??????????????????????? WebGL ????????" }
        };

        std::vector<TzdValue> resultList;
        for (const auto& item : nativeRegistry) {
            std::unordered_map<std::string, TzdValue> node;
            node["name"] = TzdValue(item.name);
            node["params"] = TzdValue(item.params);
            node["description"] = TzdValue(item.desc);
            resultList.push_back(TzdValue(node));
        }
        return TzdValue(resultList);
        });

    reg("getArraysInfo", [interp](auto args) -> TzdValue {
        std::unordered_map<std::string, TzdValue> arraysMap;
        std::set<std::string> seen;
        for (auto it = interp->scopes.rbegin(); it != interp->scopes.rend(); ++it) {
            for (auto const& [name, val] : *it) {
                if (val.type == TzdValue::ARRAY && seen.find(name) == seen.end()) {
                    seen.insert(name);
                    std::unordered_map<std::string, TzdValue> arrayMeta;
                    arrayMeta["length"] = TzdValue((double)val.arrVal.size());
                    arrayMeta["data"] = val;
                    arraysMap[name] = TzdValue(arrayMeta);
                }
            }
        }
        return TzdValue(arraysMap);
        });

    reg("len", [](auto args) -> TzdValue {
        if (args.empty()) {
            return TzdValue(0);
        }

        // ??????? Lambda ????????????????????? TzdValue ?????????D?? (Bytes)
        std::function<int(const TzdValue&)> auditSize = [&](const TzdValue& val) -> int {
            switch (val.type) {
            case TzdValue::STRING:
                // ???? string ??????? capacity ?????
                return (int)(sizeof(std::string) + val.sVal.capacity());

            case TzdValue::ARRAY: {
                int size = (int)(sizeof(std::vector<TzdValue>) + (val.arrVal.capacity() * sizeof(TzdValue)));
                for (const auto& item : val.arrVal) {
                    size += auditSize(item); // ???????????????????????
                }
                return size;
            }

            case TzdValue::MAP: {
                int size = (int)sizeof(std::unordered_map<std::string, TzdValue>);
                for (const auto& [k, v] : val.mapVal) {
                    size += (int)(k.capacity() + sizeof(TzdValue) + auditSize(v));
                }
                return size;
            }

            case TzdValue::INSTANCE: {
                if (!val.instanceVal || !val.instanceVal->definition) return 0;

                // 1. 基础大小：指针 + TzdInstance 结构体本身的大小
                int size = (int)(sizeof(void*) + sizeof(TzdInstance));

                // 加上连续内存数组 std::vector 堆空间的占用
                size += (int)(val.instanceVal->fieldValues.capacity() * sizeof(TzdValue));

                // 2. 通过定义中的索引，累加每个实际存储的字段属性
                for (const auto& pair : val.instanceVal->definition->fieldIndices) {
                    if (pair.second >= 0 && pair.second < (int)val.instanceVal->fieldValues.size()) {
                        const TzdValue& fieldValue = val.instanceVal->fieldValues[pair.second];
                        size += (int)(pair.first.capacity() + auditSize(fieldValue));
                    }
                }
                return size;
            }

            case TzdValue::INT: [[fallthrough]];
            case TzdValue::UINT: [[fallthrough]];
            case TzdValue::FLOAT:  return 4;

            case TzdValue::LONG: [[fallthrough]];
            case TzdValue::ULONG: [[fallthrough]];
            case TzdValue::DOUBLE: return 8;

            case TzdValue::SHORT: [[fallthrough]];
            case TzdValue::USHORT: return 2;

            case TzdValue::SBYTE: [[fallthrough]];
            case TzdValue::BYTE: [[fallthrough]];
            case TzdValue::BOOL:   return 1;

            case TzdValue::POINTER: return 8;
            case TzdValue::NONE:    return 0;

            default:                return 8;
            }
            };

        const auto& target = args[0];

        // ?????????????????????????????????????????????????????????????????????
        if (target.type == TzdValue::STRING) {
            return TzdValue((int)target.sVal.length()); // ??????????????????
        }
        if (target.type == TzdValue::ARRAY) {
            return TzdValue((int)target.arrVal.size());  // ??????????????
        }
        if (target.type == TzdValue::MAP) {
            return TzdValue((int)target.mapVal.size());  // ???????????????
        }
        return TzdValue(auditSize(target));
        });
}

// --- 5. ????? IO ---
void TzdNativeModule::regIO(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
        };

    reg("input", [interp](auto args) {
        if (!args.empty()) std::cout << interp->getAsString(args[0]);
        std::string in; std::getline(std::cin, in);
        return TzdValue(AnsiToUtf8(in));
        });

    reg("readFile", [](auto args) {
        std::ifstream f(args[0].sVal); if (!f) return TzdValue("Error: File not found");
        std::stringstream ss; ss << f.rdbuf(); return TzdValue(ss.str());
        });

    reg("writeFile", [](auto args) {
        if (args.size() < 2) return TzdValue(false);
        std::ofstream f(args[0].sVal); if (!f) return TzdValue(false);
        f << args[1].sVal; return TzdValue(true);
        });
}

// --- 6. ?????? (???????????) ---
void TzdNativeModule::regPlot(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
        };

    reg("plot", [interp](std::vector<TzdValue> args) -> TzdValue {
        if (args.empty()) return TzdValue("Error: plot(func, start, end, [step])");
        size_t argIdx = 0;
        std::vector<TzdValue> functions;
        std::vector<std::string> targetNames;

        while (argIdx < args.size() && (args[argIdx].type == TzdValue::FUNCTION || args[argIdx].type == TzdValue::NATIVE_FUNCTION)) {
            if (!args[argIdx].name.empty()) targetNames.push_back(args[argIdx].name);
            functions.push_back(args[argIdx++]);
        }

        double start = (argIdx < args.size()) ? valToDouble(args[argIdx++]) : -3.14;
        double end = (argIdx < args.size()) ? valToDouble(args[argIdx++]) : 3.14;
        double step = (argIdx < args.size()) ? valToDouble(args[argIdx++]) : 0.05;

        if (std::abs(start - end) < 1e-9) {
            std::fprintf(stderr, TzdMth::PLOT_RANGE_EQUAL, start, end); std::cerr << std::endl;
            return TzdValue(false);
        }
        if (start > end) { std::cout << TzdMth::PLOT_SWAP_WARN << std::endl; std::swap(start, end); }

        interp->m_lastPlot.active = true;
        interp->m_lastPlot.targetFuncNames = targetNames;
        interp->m_lastPlot.start = start;
        interp->m_lastPlot.end = end;
        interp->m_lastPlot.step = step;

        interp->internalRenderPlot(functions, start, end, step);
        return TzdValue(true);
        });

    interp->onFunctionRedefined([interp](const std::string& name, const TzdValue& newVal, antlr4::ParserRuleContext* ctx) {
        if (interp->m_lastPlot.active) {
            auto& state = interp->m_lastPlot;
            if (std::find(state.targetFuncNames.begin(), state.targetFuncNames.end(), name) != state.targetFuncNames.end()) {
                std::vector<TzdValue> currentFuncs;
                try {
                    for (const auto& fName : state.targetFuncNames) currentFuncs.push_back(interp->getVariable(fName, ctx));
                    if (!currentFuncs.empty()) interp->internalRenderPlot(currentFuncs, state.start, state.end, state.step);
                }
                catch (...) {}
            }
        }
        });
}

// ======================== String functions ========================
void TzdNativeModule::regString(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("split", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue();
        std::string s = args[0].sVal, delim = args[1].sVal;
        std::vector<TzdValue> result;
        if (delim.empty()) { result.push_back(TzdValue(s)); return TzdValue(result); }
        size_t start = 0, end;
        while ((end = s.find(delim, start)) != std::string::npos) {
            result.push_back(TzdValue(s.substr(start, end - start)));
            start = end + delim.length();
        }
        result.push_back(TzdValue(s.substr(start)));
        return TzdValue(result);
    });

    reg("join", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return TzdValue("");
        std::string delim = args[1].sVal;
        std::string result;
        for (size_t i = 0; i < args[0].arrVal.size(); ++i) {
            if (i > 0) result += delim;
            if (args[0].arrVal[i].type == TzdValue::STRING) result += args[0].arrVal[i].sVal;
            else result += std::to_string(valToDouble(args[0].arrVal[i]));
        }
        return TzdValue(result);
    });

    reg("replace", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue();
        std::string s = args[0].sVal, from = args[1].sVal, to = args[2].sVal;
        if (from.empty()) return TzdValue(s);
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.length(), to);
            pos += to.length();
        }
        return TzdValue(s);
    });

    reg("substring", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue("");
        std::string s = args[0].sVal;
        int start = (int)valToDouble(args[1]);
        if (start < 0) start = (int)s.length() + start;
        if (start < 0) start = 0;
        if (start >= (int)s.length()) return TzdValue("");
        int end = (args.size() > 2) ? (int)valToDouble(args[2]) : (int)s.length();
        if (end < 0) end = (int)s.length() + end;
        if (end < start) return TzdValue("");
        if (end > (int)s.length()) end = (int)s.length();
        return TzdValue(s.substr(start, end - start));
    });

    reg("trim", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        size_t first = s.find_first_not_of(" \t\r\n\v\f");
        if (first == std::string::npos) return TzdValue("");
        size_t last = s.find_last_not_of(" \t\r\n\v\f");
        return TzdValue(s.substr(first, last - first + 1));
    });

    reg("toUpper", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
        return TzdValue(s);
    });

    reg("toLower", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return TzdValue(s);
    });

    reg("contains", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        return TzdValue(args[0].sVal.find(args[1].sVal) != std::string::npos);
    });

    reg("startsWith", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        return TzdValue(args[0].sVal.rfind(args[1].sVal, 0) == 0);
    });

    reg("endsWith", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        const std::string& s = args[0].sVal;
        const std::string& suffix = args[1].sVal;
        if (suffix.length() > s.length()) return TzdValue(false);
        return TzdValue(s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0);
    });

    reg("indexOf", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(-1.0);
        size_t pos = args[0].sVal.find(args[1].sVal);
        if (pos == std::string::npos) return TzdValue(-1.0);
        return TzdValue((double)pos);
    });

    reg("charAt", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue("");
        int idx = (int)valToDouble(args[1]);
        if (idx < 0 || idx >= (int)args[0].sVal.length()) return TzdValue("");
        return TzdValue(std::string(1, args[0].sVal[idx]));
    });

    reg("reverseStr", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        std::reverse(s.begin(), s.end());
        return TzdValue(s);
    });

    reg("splitRegex", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue();
        try {
            std::regex re(args[1].sVal);
            std::sregex_token_iterator it(args[0].sVal.begin(), args[0].sVal.end(), re, -1);
            std::sregex_token_iterator end;
            std::vector<TzdValue> result;
            for (; it != end; ++it) result.push_back(TzdValue(it->str()));
            return TzdValue(result);
        } catch (...) { return TzdValue(); }
    });

    reg("replaceRegex", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue();
        try {
            std::regex re(args[1].sVal);
            return TzdValue(std::regex_replace(args[0].sVal, re, args[2].sVal));
        } catch (...) { return TzdValue(args[0].sVal); }
    });

    reg("match", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        try {
            return TzdValue(std::regex_search(args[0].sVal, std::regex(args[1].sVal)));
        } catch (...) { return TzdValue(false); }
    });

    reg("repeat", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue("");
        int n = (int)valToDouble(args[1]);
        if (n <= 0) return TzdValue("");
        std::string result;
        for (int i = 0; i < n; ++i) result += args[0].sVal;
        return TzdValue(result);
    });

    reg("padLeft", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue();
        std::string s = args[0].sVal;
        int width = (int)valToDouble(args[1]);
        char pad = args[2].sVal.empty() ? ' ' : args[2].sVal[0];
        if ((int)s.length() >= width) return TzdValue(s);
        return TzdValue(std::string(width - s.length(), pad) + s);
    });

    reg("padRight", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue();
        std::string s = args[0].sVal;
        int width = (int)valToDouble(args[1]);
        char pad = args[2].sVal.empty() ? ' ' : args[2].sVal[0];
        if ((int)s.length() >= width) return TzdValue(s);
        return TzdValue(s + std::string(width - s.length(), pad));
    });

    reg("format", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string fmt = args[0].sVal;
        std::string result;
        size_t argIdx = 1;
        for (size_t i = 0; i < fmt.length(); ++i) {
            if (fmt[i] == '{' && i + 1 < fmt.length() && fmt[i+1] == '}') {
                if (argIdx < args.size()) {
                    if (args[argIdx].type == TzdValue::STRING) result += args[argIdx].sVal;
                    else result += std::to_string(valToDouble(args[argIdx]));
                    ++argIdx;
                }
                ++i;
            } else {
                result += fmt[i];
            }
        }
        return TzdValue(result);
    });
}

// ======================== Array functions ========================
void TzdNativeModule::regArray(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("push", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        TzdValue result = args[0];
        for (size_t i = 1; i < args.size(); ++i) result.arrVal.push_back(args[i]);
        return result;
    });

    reg("pop", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty()) return TzdValue();
        return args[0].arrVal.back();
    });

    reg("shift", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty()) return TzdValue();
        return args[0].arrVal.front();
    });

    reg("unshift", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        TzdValue result;
        result.type = TzdValue::ARRAY;
        for (size_t i = 1; i < args.size(); ++i) result.arrVal.push_back(args[i]);
        for (const auto& item : args[0].arrVal) result.arrVal.push_back(item);
        return result;
    });

    reg("slice", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        const auto& arr = args[0].arrVal;
        int sz = (int)arr.size();
        int start = (args.size() > 1) ? (int)valToDouble(args[1]) : 0;
        int end = (args.size() > 2) ? (int)valToDouble(args[2]) : sz;
        if (start < 0) start = sz + start; if (start < 0) start = 0;
        if (end < 0) end = sz + end; if (end < 0) end = 0;
        if (start > sz) start = sz; if (end > sz) end = sz;
        if (end < start) end = start;
        std::vector<TzdValue> result(arr.begin() + start, arr.begin() + end);
        return TzdValue(result);
    });

    reg("concat", [](auto args) -> TzdValue {
        TzdValue result;
        result.type = TzdValue::ARRAY;
        for (const auto& arg : args) {
            if (arg.type == TzdValue::ARRAY) {
                for (const auto& item : arg.arrVal) result.arrVal.push_back(item);
            } else {
                result.arrVal.push_back(arg);
            }
        }
        return result;
    });

    reg("reverse", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        TzdValue result = args[0];
        std::reverse(result.arrVal.begin(), result.arrVal.end());
        return result;
    });

    reg("sort", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        TzdValue result = args[0];
        std::sort(result.arrVal.begin(), result.arrVal.end(), [](const TzdValue& a, const TzdValue& b) {
            return valToDouble(a) < valToDouble(b);
        });
        return result;
    });

    reg("range", [](auto args) -> TzdValue {
        double start = (args.size() > 0) ? valToDouble(args[0]) : 0;
        double end = (args.size() > 1) ? valToDouble(args[1]) : start;
        double step = (args.size() > 2) ? valToDouble(args[2]) : 1.0;
        if (step == 0) step = 1.0;
        std::vector<TzdValue> result;
        if (step > 0) {
            for (double v = start; v < end; v += step) result.push_back(TzdValue(v));
        } else {
            for (double v = start; v > end; v += step) result.push_back(TzdValue(v));
        }
        return TzdValue(result);
    });

    reg("map", [interp](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return TzdValue();
        std::vector<TzdValue> result;
        for (const auto& item : args[0].arrVal) {
            std::vector<TzdValue> callArgs = {item};
            result.push_back(interp->callFunction(args[1], callArgs));
        }
        return TzdValue(result);
    });

    reg("filter", [interp](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return TzdValue();
        std::vector<TzdValue> result;
        for (const auto& item : args[0].arrVal) {
            std::vector<TzdValue> callArgs = {item};
            TzdValue r = interp->callFunction(args[1], callArgs);
            if (valToDouble(r) != 0 || r.type == TzdValue::BOOL && r.bVal) result.push_back(item);
        }
        return TzdValue(result);
    });

    reg("reduce", [interp](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return TzdValue();
        TzdValue acc = (args.size() > 2) ? args[2] : (args[0].arrVal.empty() ? TzdValue() : args[0].arrVal[0]);
        size_t startIdx = (args.size() > 2) ? 0 : 1;
        for (size_t i = startIdx; i < args[0].arrVal.size(); ++i) {
            std::vector<TzdValue> callArgs = {acc, args[0].arrVal[i]};
            acc = interp->callFunction(args[1], callArgs);
        }
        return acc;
    });

    reg("find", [interp](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return TzdValue();
        for (const auto& item : args[0].arrVal) {
            std::vector<TzdValue> callArgs = {item};
            TzdValue r = interp->callFunction(args[1], callArgs);
            if (valToDouble(r) != 0 || r.type == TzdValue::BOOL && r.bVal) return item;
        }
        return TzdValue();
    });

    reg("includes", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return TzdValue(false);
        for (const auto& item : args[0].arrVal) {
            if (item.type == TzdValue::STRING && args[1].type == TzdValue::STRING && item.sVal == args[1].sVal) return TzdValue(true);
            if (valToDouble(item) == valToDouble(args[1]) && item.type == args[1].type) return TzdValue(true);
        }
        return TzdValue(false);
    });

    reg("indexOfArr", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return TzdValue(-1.0);
        for (size_t i = 0; i < args[0].arrVal.size(); ++i) {
            if (args[0].arrVal[i].type == TzdValue::STRING && args[1].type == TzdValue::STRING && args[0].arrVal[i].sVal == args[1].sVal) return TzdValue((double)i);
            if (valToDouble(args[0].arrVal[i]) == valToDouble(args[1])) return TzdValue((double)i);
        }
        return TzdValue(-1.0);
    });

    reg("fill", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue();
        int n = (int)valToDouble(args[0]);
        std::vector<TzdValue> result;
        for (int i = 0; i < n; ++i) result.push_back(args[1]);
        return TzdValue(result);
    });

    reg("flatten", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        std::vector<TzdValue> result;
        std::function<void(const TzdValue&)> flat = [&](const TzdValue& v) {
            if (v.type == TzdValue::ARRAY) { for (const auto& item : v.arrVal) flat(item); }
            else result.push_back(v);
        };
        flat(args[0]);
        return TzdValue(result);
    });

    reg("zip", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY || args[1].type != TzdValue::ARRAY) return TzdValue();
        size_t n = (std::min)(args[0].arrVal.size(), args[1].arrVal.size());
        std::vector<TzdValue> result;
        for (size_t i = 0; i < n; ++i) {
            std::vector<TzdValue> pair = {args[0].arrVal[i], args[1].arrVal[i]};
            result.push_back(TzdValue(pair));
        }
        return TzdValue(result);
    });

    reg("unique", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        std::vector<TzdValue> result;
        for (const auto& item : args[0].arrVal) {
            bool found = false;
            for (const auto& r : result) {
                if (r.type == TzdValue::STRING && item.type == TzdValue::STRING && r.sVal == item.sVal) { found = true; break; }
                if (valToDouble(r) == valToDouble(item)) { found = true; break; }
            }
            if (!found) result.push_back(item);
        }
        return TzdValue(result);
    });

    reg("sum", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue(0.0);
        double s = 0;
        for (const auto& item : args[0].arrVal) s += valToDouble(item);
        return TzdValue(s);
    });

    reg("avg", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty()) return TzdValue(0.0);
        double s = 0;
        for (const auto& item : args[0].arrVal) s += valToDouble(item);
        return TzdValue(s / args[0].arrVal.size());
    });

    reg("minArr", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty()) return TzdValue(0.0);
        double m = valToDouble(args[0].arrVal[0]);
        for (const auto& item : args[0].arrVal) { double v = valToDouble(item); if (v < m) m = v; }
        return TzdValue(m);
    });

    reg("maxArr", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty()) return TzdValue(0.0);
        double m = valToDouble(args[0].arrVal[0]);
        for (const auto& item : args[0].arrVal) { double v = valToDouble(item); if (v > m) m = v; }
        return TzdValue(m);
    });
}

// ======================== JSON functions ========================
void TzdNativeModule::regJson(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("jsonParse", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::STRING) return TzdValue();
        std::string s = args[0].sVal;
        size_t pos = 0;
        std::function<TzdValue()> parseValue = [&]() -> TzdValue {
            auto skipWs = [&]() { while (pos < s.size() && (s[pos]==' '||s[pos]=='\t'||s[pos]=='\r'||s[pos]=='\n')) ++pos; };
            auto parseString = [&]() -> TzdValue {
                if (s[pos] != '"') return TzdValue();
                ++pos;
                std::string str;
                while (pos < s.size() && s[pos] != '"') {
                    if (s[pos] == '\\' && pos+1 < s.size()) {
                        char c = s[pos+1]; ++pos;
                        switch (c) {
                            case 'n': str += '\n'; break;
                            case 't': str += '\t'; break;
                            case 'r': str += '\r'; break;
                            case '"': str += '"'; break;
                            case '\\': str += '\\'; break;
                            case '/': str += '/'; break;
                            case 'b': str += '\b'; break;
                            case 'f': str += '\f'; break;
                            default: str += c; break;
                        }
                    } else str += s[pos];
                    ++pos;
                }
                if (pos < s.size()) ++pos;
                return TzdValue(str);
            };
            auto parseNumber = [&]() -> TzdValue {
                size_t start = pos;
                if (s[pos]=='-') ++pos;
                while (pos < s.size() && (s[pos]>='0'&&s[pos]<='9'||s[pos]=='.'||s[pos]=='e'||s[pos]=='E'||s[pos]=='+'||s[pos]=='-')) ++pos;
                return TzdValue(std::stod(s.substr(start, pos - start)));
            };
            auto parseBool = [&]() -> TzdValue {
                if (s.substr(pos, 4) == "true") { pos += 4; return TzdValue(true); }
                if (s.substr(pos, 5) == "false") { pos += 5; return TzdValue(false); }
                return TzdValue();
            };
            auto parseNull = [&]() -> TzdValue {
                if (s.substr(pos, 4) == "null") { pos += 4; return TzdValue(); }
                return TzdValue();
            };

            skipWs();
            if (pos >= s.size()) return TzdValue();
            char c = s[pos];
            if (c == '"') return parseString();
            if (c == '{') {
                ++pos; skipWs();
                std::unordered_map<std::string, TzdValue> map;
                if (pos < s.size() && s[pos] == '}') { ++pos; return TzdValue(map); }
                while (pos < s.size()) {
                    skipWs();
                    TzdValue key = parseString();
                    skipWs();
                    if (pos < s.size() && s[pos] == ':') ++pos;
                    TzdValue val = parseValue();
                    map[key.sVal] = val;
                    skipWs();
                    if (pos < s.size() && s[pos] == ',') { ++pos; continue; }
                    if (pos < s.size() && s[pos] == '}') { ++pos; break; }
                    break;
                }
                return TzdValue(map);
            }
            if (c == '[') {
                ++pos; skipWs();
                std::vector<TzdValue> arr;
                if (pos < s.size() && s[pos] == ']') { ++pos; return TzdValue(arr); }
                while (pos < s.size()) {
                    TzdValue val = parseValue();
                    arr.push_back(val);
                    skipWs();
                    if (pos < s.size() && s[pos] == ',') { ++pos; skipWs(); continue; }
                    if (pos < s.size() && s[pos] == ']') { ++pos; break; }
                    break;
                }
                return TzdValue(arr);
            }
            if (c == '-' || (c >= '0' && c <= '9')) return parseNumber();
            if (c == 't' || c == 'f') return parseBool();
            if (c == 'n') return parseNull();
            return TzdValue();
        };
        try { return parseValue(); } catch (...) { return TzdValue(); }
    });

    reg("jsonStringify", [interp](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("null");
        std::function<std::string(const TzdValue&)> stringify = [&](const TzdValue& val) -> std::string {
            switch (val.type) {
                case TzdValue::NONE: return "null";
                case TzdValue::BOOL: return val.bVal ? "true" : "false";
                case TzdValue::INT: case TzdValue::DOUBLE: case TzdValue::FLOAT:
                case TzdValue::LONG: case TzdValue::UINT: case TzdValue::ULONG:
                    { std::ostringstream ss; ss << valToDouble(val); return ss.str(); }
                case TzdValue::STRING: {
                    std::string r = "\"";
                    for (char c : val.sVal) {
                        switch (c) {
                            case '"': r += "\\\""; break;
                            case '\\': r += "\\\\"; break;
                            case '\n': r += "\\n"; break;
                            case '\t': r += "\\t"; break;
                            case '\r': r += "\\r"; break;
                            default: r += c; break;
                        }
                    }
                    r += "\""; return r;
                }
                case TzdValue::ARRAY: {
                    std::string r = "[";
                    for (size_t i = 0; i < val.arrVal.size(); ++i) {
                        if (i > 0) r += ",";
                        r += stringify(val.arrVal[i]);
                    }
                    r += "]"; return r;
                }
                case TzdValue::MAP: {
                    std::string r = "{";
                    bool first = true;
                    for (const auto& [k, v] : val.mapVal) {
                        if (!first) r += ",";
                        first = false;
                        r += "\"" + k + "\":" + stringify(v);
                    }
                    r += "}"; return r;
                }
                default: return "null";
            }
        };
        return TzdValue(stringify(args[0]));
    });
}

// ======================== File system functions ========================
void TzdNativeModule::regFileSystem(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("fileExists", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        return TzdValue(fs::exists(args[0].sVal) && fs::is_regular_file(args[0].sVal));
    });

    reg("dirExists", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        return TzdValue(fs::exists(args[0].sVal) && fs::is_directory(args[0].sVal));
    });

    reg("listDir", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        std::string path = args[0].sVal;
        if (!fs::exists(path)) return TzdValue();
        std::vector<TzdValue> result;
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                std::unordered_map<std::string, TzdValue> item;
                item["name"] = TzdValue(entry.path().filename().string());
                item["isDir"] = TzdValue(entry.is_directory());
                item["isFile"] = TzdValue(entry.is_regular_file());
                item["size"] = TzdValue((double)(entry.is_regular_file() ? fs::file_size(entry.path()) : 0));
                result.push_back(TzdValue(item));
            }
        } catch (...) {}
        return TzdValue(result);
    });

    reg("makeDir", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        try { return TzdValue(fs::create_directories(args[0].sVal)); }
        catch (...) { return TzdValue(false); }
    });

    reg("removeFile", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        try { return TzdValue(fs::remove(args[0].sVal)); }
        catch (...) { return TzdValue(false); }
    });

    reg("removeDir", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        try { return TzdValue(fs::remove_all(args[0].sVal) > 0); }
        catch (...) { return TzdValue(false); }
    });

    reg("copyFile", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        try { fs::copy_file(args[0].sVal, args[1].sVal, fs::copy_options::overwrite_existing); return TzdValue(true); }
        catch (...) { return TzdValue(false); }
    });

    reg("moveFile", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        try { fs::rename(args[0].sVal, args[1].sVal); return TzdValue(true); }
        catch (...) { return TzdValue(false); }
    });

    reg("fileSize", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        try { if (fs::exists(args[0].sVal)) return TzdValue((double)fs::file_size(args[0].sVal)); }
        catch (...) {}
        return TzdValue(0.0);
    });

    reg("currentDir", [](auto args) -> TzdValue {
        try { return TzdValue(fs::current_path().string()); }
        catch (...) { return TzdValue(""); }
    });

    reg("changeDir", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        try { fs::current_path(args[0].sVal); return TzdValue(true); }
        catch (...) { return TzdValue(false); }
    });

    reg("appendFile", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        std::ofstream f(args[0].sVal, std::ios::app);
        if (!f) return TzdValue(false);
        f << args[1].sVal;
        return TzdValue(true);
    });

    reg("readLines", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        std::ifstream f(args[0].sVal);
        if (!f) return TzdValue();
        std::vector<TzdValue> lines;
        std::string line;
        while (std::getline(f, line)) lines.push_back(TzdValue(line));
        return TzdValue(lines);
    });

    reg("writeLines", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[1].type != TzdValue::ARRAY) return TzdValue(false);
        std::ofstream f(args[0].sVal);
        if (!f) return TzdValue(false);
        for (const auto& line : args[1].arrVal) {
            if (line.type == TzdValue::STRING) f << line.sVal;
            else f << valToDouble(line);
            f << "\n";
        }
        return TzdValue(true);
    });
}

// ======================== Conversion functions ========================
void TzdNativeModule::regConv(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("toFixed", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("0");
        int digits = (args.size() > 1) ? (int)valToDouble(args[1]) : 2;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(digits) << valToDouble(args[0]);
        return TzdValue(ss.str());
    });

    reg("toPrecision", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("0");
        int prec = (args.size() > 1) ? (int)valToDouble(args[1]) : 6;
        std::ostringstream ss;
        ss << std::setprecision(prec) << valToDouble(args[0]);
        return TzdValue(ss.str());
    });

    reg("parseFloat", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        try { return TzdValue(std::stod(args[0].sVal)); }
        catch (...) { return TzdValue(0.0); }
    });

    reg("isFinite", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        return TzdValue(std::isfinite(valToDouble(args[0])));
    });

    reg("isNaN", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(true);
        return TzdValue(std::isnan(valToDouble(args[0])));
    });

    reg("toBool", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        if (args[0].type == TzdValue::STRING) return TzdValue(args[0].sVal == "true" || args[0].sVal == "1");
        if (args[0].type == TzdValue::BOOL) return args[0];
        return TzdValue(valToDouble(args[0]) != 0);
    });

    reg("toInt", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0);
        if (args[0].type == TzdValue::STRING) { try { return TzdValue((double)std::stoll(args[0].sVal)); } catch (...) { return TzdValue(0.0); } }
        return TzdValue((double)(long long)valToDouble(args[0]));
    });

    reg("toHex", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("0");
        std::ostringstream ss;
        ss << std::hex << (long long)valToDouble(args[0]);
        return TzdValue(ss.str());
    });

    reg("fromHex", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        try { return TzdValue((double)std::stoll(args[0].sVal, nullptr, 16)); }
        catch (...) { return TzdValue(0.0); }
    });

    reg("toBinary", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("0");
        long long val = (long long)valToDouble(args[0]);
        if (val == 0) return TzdValue("0");
        std::string bin;
        while (val > 0) { bin = std::to_string(val & 1) + bin; val >>= 1; }
        return TzdValue(bin);
    });

    reg("fromBinary", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        try { return TzdValue((double)std::stoll(args[0].sVal, nullptr, 2)); }
        catch (...) { return TzdValue(0.0); }
    });

    reg("charCode", [](auto args) -> TzdValue {
        if (args.empty() || args[0].sVal.empty()) return TzdValue(0.0);
        return TzdValue((double)(unsigned char)args[0].sVal[0]);
    });

    reg("fromCharCode", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        int code = (int)valToDouble(args[0]);
        return TzdValue(std::string(1, (char)code));
    });
}

// ======================== Extra math functions ========================
void TzdNativeModule::regExtraMath(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("atan2", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        return TzdValue(std::atan2(valToDouble(args[0]), valToDouble(args[1])));
    });

    reg("hypot", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        return TzdValue(std::hypot(valToDouble(args[0]), valToDouble(args[1])));
    });

    reg("min", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        double m = valToDouble(args[0]);
        for (size_t i = 1; i < args.size(); ++i) { double v = valToDouble(args[i]); if (v < m) m = v; }
        return TzdValue(m);
    });

    reg("max", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        double m = valToDouble(args[0]);
        for (size_t i = 1; i < args.size(); ++i) { double v = valToDouble(args[i]); if (v > m) m = v; }
        return TzdValue(m);
    });

    reg("gcd", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        long long a = std::abs((long long)valToDouble(args[0]));
        long long b = std::abs((long long)valToDouble(args[1]));
        while (b) { long long t = b; b = a % b; a = t; }
        return TzdValue((double)a);
    });

    reg("lcm", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        long long a = std::abs((long long)valToDouble(args[0]));
        long long b = std::abs((long long)valToDouble(args[1]));
        if (a == 0 || b == 0) return TzdValue(0.0);
        long long g = a; long long bb = b;
        while (bb) { long long t = bb; bb = g % bb; g = t; }
        return TzdValue((double)(a / g * b));
    });

    reg("isPrime", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        long long n = (long long)valToDouble(args[0]);
        if (n < 2) return TzdValue(false);
        if (n < 4) return TzdValue(true);
        if (n % 2 == 0 || n % 3 == 0) return TzdValue(false);
        for (long long i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return TzdValue(false);
        }
        return TzdValue(true);
    });

    reg("random", [](auto args) -> TzdValue {
        static std::mt19937_64 gen(std::random_device{}());
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return TzdValue(dist(gen));
    });

    reg("randomInt", [](auto args) -> TzdValue {
        static std::mt19937_64 gen(std::random_device{}());
        int lo = (args.size() > 0) ? (int)valToDouble(args[0]) : 0;
        int hi = (args.size() > 1) ? (int)valToDouble(args[1]) : lo;
        if (lo > hi) std::swap(lo, hi);
        std::uniform_int_distribution<int> dist(lo, hi);
        return TzdValue((double)dist(gen));
    });

    reg("randomSeed", [](auto args) -> TzdValue {
        unsigned int seed = (unsigned int)valToDouble(args[0]);
        srand(seed);
        return TzdValue();
    });

    reg("degrees", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(valToDouble(args[0]) * 180.0 / 3.14159265358979323846);
    });

    reg("radians", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(valToDouble(args[0]) * 3.14159265358979323846 / 180.0);
    });

    reg("log2", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(std::log2(valToDouble(args[0])));
    });

    reg("logBase", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        double x = valToDouble(args[0]), b = valToDouble(args[1]);
        if (x <= 0 || b <= 0 || b == 1) return TzdValue(0.0);
        return TzdValue(std::log(x) / std::log(b));
    });

    reg("erf", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(std::erf(valToDouble(args[0])));
    });

    reg("tgamma", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(std::tgamma(valToDouble(args[0])));
    });

    reg("sign", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        double v = valToDouble(args[0]);
        return TzdValue(v > 0 ? 1.0 : (v < 0 ? -1.0 : 0.0));
    });

    reg("clamp", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue();
        double v = valToDouble(args[0]);
        double lo = valToDouble(args[1]);
        double hi = valToDouble(args[2]);
        return TzdValue(v < lo ? lo : (v > hi ? hi : v));
    });

    reg("lerp", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue();
        double a = valToDouble(args[0]);
        double b = valToDouble(args[1]);
        double t = valToDouble(args[2]);
        return TzdValue(a + (b - a) * t);
    });

    reg("sum", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        if (args[0].type == TzdValue::ARRAY) {
            double s = 0;
            for (const auto& item : args[0].arrVal) s += valToDouble(item);
            return TzdValue(s);
        }
        double s = 0;
        for (const auto& arg : args) s += valToDouble(arg);
        return TzdValue(s);
    });

    reg("PI", [](auto args) -> TzdValue { return TzdValue(3.14159265358979323846); });
    reg("E", [](auto args) -> TzdValue { return TzdValue(2.71828182845904523536); });
    reg("INF", [](auto args) -> TzdValue { return TzdValue(std::numeric_limits<double>::infinity()); });
    reg("NAN", [](auto args) -> TzdValue { return TzdValue(std::numeric_limits<double>::quiet_NaN()); });

    // --- BIGINT (arbitrary precision integer) functions ---
    // bigint(x): convert any numeric value to BIGINT
    reg("bigint", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        const TzdValue& v = args[0];
        if (v.type == TzdValue::BIGINT) return v;
        std::string s;
        if (v.type == TzdValue::STRING) s = v.sVal;
        else if (v.type == TzdValue::ULONG) s = std::to_string(v.ulVal);
        else if (v.type == TzdValue::DOUBLE || v.type == TzdValue::FLOAT) {
            double d = v.dVal;
            s = std::to_string((long long)d);
        } else s = std::to_string(v.lVal);
        TzdValue r; r.type = TzdValue::BIGINT; r.sVal = s;
        return r;
    });

    // bigintFactorial(n): factorial using arbitrary precision
    reg("bigintFactorial", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(1LL);
        long long n = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        if (n < 0) return TzdValue(0LL);
        if (n <= 1) { TzdValue r; r.type = TzdValue::BIGINT; r.sVal = "1"; return r; }
        // Safety: factorial(n) has roughly n*log10(n) digits.
        // Reject if n is so large the result would exceed the digit limit.
        // factorial(1000000) ≈ 5.5M digits, factorial(10000000) ≈ 65M digits.
        if (n > 1000000) {
            TzdValue r; r.type = TzdValue::BIGINT; r.sVal = "inf"; return r;
        }
        std::string result = "1";
        for (long long i = 2; i <= n; ++i) {
            result = bigint_mul(result, std::to_string(i));
            if (result == "inf" || bigint_too_large(result.size())) {
                TzdValue r; r.type = TzdValue::BIGINT; r.sVal = "inf"; return r;
            }
        }
        TzdValue r; r.type = TzdValue::BIGINT; r.sVal = result;
        return r;
    });

    // setBigIntMaxDigits(n): configure the maximum digit count for BIGINT results
    reg("setBigIntMaxDigits", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0LL);
        long long n = (long long)TzdInterpreter::getAsDoubleInternal(args[0]);
        setBigIntMaxDigits((size_t)n);
        return TzdValue((long long)getBigIntMaxDigits());
    });

    // getBigIntMaxDigits(): get the current BIGINT digit limit
    reg("getBigIntMaxDigits", [](auto args) -> TzdValue {
        return TzdValue((long long)getBigIntMaxDigits());
    });

    // powmod(base, exp, mod): modular exponentiation
    reg("powmod", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue(0LL);
        std::string base = to_bigint_str(args[0]);
        std::string exp = to_bigint_str(args[1]);
        std::string mod = to_bigint_str(args[2]);
        std::string r = bigint_powmod(base, exp, mod);
        TzdValue v; v.type = TzdValue::BIGINT; v.sVal = r;
        return v;
    });

    // isBigint(x): check if value is BIGINT type
    reg("isBigint", [](auto args) -> TzdValue {
        return TzdValue(!args.empty() && args[0].type == TzdValue::BIGINT);
    });

    // bigintGcd(a, b): greatest common divisor using big integers
    reg("bigintGcd", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0LL);
        std::string a = bigint_abs(to_bigint_str(args[0]));
        std::string b = bigint_abs(to_bigint_str(args[1]));
        while (b != "0") {
            std::string r = bigint_divmod_abs(a, b, true);
            a = b; b = r;
        }
        TzdValue v; v.type = TzdValue::BIGINT; v.sVal = a.empty() ? "0" : a;
        return v;
    });

    // --- RATIONAL (exact fraction) functions ---
    // rational(num, den): create an exact fraction
    reg("rational", [](auto args) -> TzdValue {
        if (args.size() < 2) {
            // Single arg: convert number to rational
            if (args.empty()) return TzdValue();
            std::string s = to_rational_str(args[0]);
            TzdValue v; v.type = (s.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT; v.sVal = s;
            return v;
        }
        return make_rational(to_bigint_str(args[0]), to_bigint_str(args[1]));
    });

    // toFraction(x): convert a double to an exact fraction
    reg("toFraction", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        const TzdValue& v = args[0];
        if (v.type == TzdValue::RATIONAL) return v;
        if (v.type == TzdValue::BIGINT) return v;
        // Convert double to fraction using continued fraction approximation
        double d = TzdInterpreter::getAsDoubleInternal(v);
        bool neg = d < 0;
        d = std::abs(d);
        // Continued fraction algorithm for exact representation
        long long num = 1, den = 1;
        double intPart = std::floor(d);
        double frac = d - intPart;
        if (frac < 1e-15) {
            num = (long long)intPart; den = 1;
        } else {
            // Simple approach: multiply by power of 10 until integer
            long long n = (long long)(d * 1000000LL + 0.5);
            long long d2 = 1000000LL;
            // Reduce by GCD
            auto gcd = [](long long a, long long b) { while (b) { a %= b; std::swap(a, b); } return a; };
            long long g = gcd(n, d2);
            num = n / g; den = d2 / g;
        }
        if (neg) num = -num;
        return make_rational(std::to_string(num), std::to_string(den));
    });

    // rationalAdd(a, b): exact fraction addition
    reg("rationalAdd", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue();
        std::string r = rational_add(to_rational_str(args[0]), to_rational_str(args[1]));
        TzdValue v; v.type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT; v.sVal = r;
        return v;
    });

    // rationalMul(a, b): exact fraction multiplication
    reg("rationalMul", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue();
        std::string r = rational_mul(to_rational_str(args[0]), to_rational_str(args[1]));
        TzdValue v; v.type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT; v.sVal = r;
        return v;
    });
}

// ============================================================================
// Extended Standard Library — datetime, encoding, environment, utilities
// ============================================================================
void TzdNativeModule::regExtended(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    // ---- DateTime functions ----
    reg("now", [](auto args) -> TzdValue {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        return TzdValue((double)ms.count());
    });

    reg("timestamp", [](auto args) -> TzdValue {
        auto now = std::chrono::system_clock::now();
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
        return TzdValue((double)secs.count());
    });

    reg("formatTime", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        double ts = valToDouble(args[0]);
        std::string fmt = (args.size() > 1) ? args[1].sVal : "%Y-%m-%d %H:%M:%S";
        time_t rawtime = (time_t)ts / 1000;
        struct tm timeinfo;
        localtime_s(&timeinfo, &rawtime);
        char buffer[256];
        strftime(buffer, sizeof(buffer), fmt.c_str(), &timeinfo);
        return TzdValue(std::string(buffer));
    });

    reg("dateParts", [](auto args) -> TzdValue {
        double ts = args.empty() ? (double)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count() : valToDouble(args[0]);
        time_t rawtime = (time_t)ts / 1000;
        struct tm timeinfo;
        localtime_s(&timeinfo, &rawtime);
        std::unordered_map<std::string, TzdValue> m;
        m["year"] = TzdValue((double)(timeinfo.tm_year + 1900));
        m["month"] = TzdValue((double)(timeinfo.tm_mon + 1));
        m["day"] = TzdValue((double)timeinfo.tm_mday);
        m["hour"] = TzdValue((double)timeinfo.tm_hour);
        m["minute"] = TzdValue((double)timeinfo.tm_min);
        m["second"] = TzdValue((double)timeinfo.tm_sec);
        m["weekday"] = TzdValue((double)timeinfo.tm_wday);
        m["dayOfYear"] = TzdValue((double)timeinfo.tm_yday);
        return TzdValue(m);
    });

    reg("dateDiff", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        double t1 = valToDouble(args[0]);
        double t2 = valToDouble(args[1]);
        return TzdValue(t2 - t1);
    });

    // ---- Base64 encoding/decoding ----
    static const char b64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    reg("base64Encode", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string input = args[0].sVal;
        std::string output;
        int val = 0, valb = -6;
        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                output.push_back(b64Table[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > 0) output.push_back(b64Table[(val << (6 - valb)) & 0x3F]);
        while (output.size() % 4) output.push_back('=');
        return TzdValue(output);
    });

    reg("base64Decode", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string input = args[0].sVal;
        static int b64DecTable[128] = {0};
        static bool initTable = false;
        if (!initTable) {
            for (int i = 0; i < 64; i++) b64DecTable[(int)b64Table[i]] = i + 1;
            initTable = true;
        }
        std::string output;
        int val = 0, valb = -8;
        for (unsigned char c : input) {
            if (c == '=') break;
            if ((int)c >= 128 || b64DecTable[c] == 0) continue;
            val = (val << 6) + b64DecTable[c] - 1;
            valb += 6;
            if (valb >= 0) {
                output.push_back((char)((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return TzdValue(output);
    });

    // ---- CRC32 ----
    reg("crc32", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        std::string data = args[0].sVal;
        static uint32_t table[256] = {0};
        static bool initTable = false;
        if (!initTable) {
            for (uint32_t i = 0; i < 256; i++) {
                uint32_t crc = i;
                for (int j = 0; j < 8; j++)
                    crc = (crc & 1) ? (0xEDB88320 ^ (crc >> 1)) : (crc >> 1);
                table[i] = crc;
            }
            initTable = true;
        }
        uint32_t crc = 0xFFFFFFFF;
        for (unsigned char c : data) {
            crc = table[(crc ^ c) & 0xFF] ^ (crc >> 8);
        }
        return TzdValue((double)(crc ^ 0xFFFFFFFF));
    });

    // ---- Hash (simple FNV-1a 64-bit) ----
    reg("hash", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        std::string data = args[0].sVal;
        uint64_t hash = 14695981039346656037ULL;
        for (unsigned char c : data) {
            hash ^= c;
            hash *= 1099511628211ULL;
        }
        return TzdValue((double)hash);
    });

    // ---- Environment variables ----
    reg("getEnv", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        const char* val = std::getenv(args[0].sVal.c_str());
        return TzdValue(val ? std::string(val) : "");
    });

    reg("setEnv", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
#ifdef _WIN32
        return TzdValue(_putenv_s(args[0].sVal.c_str(), args[1].sVal.c_str()) == 0);
#else
        return TzdValue(setenv(args[0].sVal.c_str(), args[1].sVal.c_str(), 1) == 0);
#endif
    });

    // ---- Additional math functions ----
    reg("cbrt", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(std::cbrt(valToDouble(args[0])));
    });

    reg("log1p", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(std::log1p(valToDouble(args[0])));
    });

    reg("expm1", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        return TzdValue(std::expm1(valToDouble(args[0])));
    });

    reg("comb", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        long long n = (long long)valToDouble(args[0]);
        long long k = (long long)valToDouble(args[1]);
        if (k < 0 || k > n) return TzdValue(0.0);
        if (k > n - k) k = n - k;
        double result = 1.0;
        for (long long i = 0; i < k; i++) {
            result = result * (n - i) / (i + 1);
        }
        return TzdValue(result);
    });

    reg("perm", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        long long n = (long long)valToDouble(args[0]);
        long long k = (long long)valToDouble(args[1]);
        if (k < 0 || k > n) return TzdValue(0.0);
        double result = 1.0;
        for (long long i = 0; i < k; i++) result *= (n - i);
        return TzdValue(result);
    });

    reg("fib", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        long long n = (long long)valToDouble(args[0]);
        if (n <= 0) return TzdValue(0.0);
        if (n == 1) return TzdValue(1.0);
        double a = 0, b = 1;
        for (long long i = 2; i <= n; i++) {
            double c = a + b; a = b; b = c;
        }
        return TzdValue(b);
    });

    reg("isPowerOf2", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        long long n = (long long)valToDouble(args[0]);
        return TzdValue(n > 0 && (n & (n - 1)) == 0);
    });

    reg("nextPowerOf2", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(1.0);
        long long n = (long long)valToDouble(args[0]);
        if (n <= 1) return TzdValue(1.0);
        n--;
        n |= n >> 1; n |= n >> 2; n |= n >> 4; n |= n >> 8; n |= n >> 16; n |= n >> 32;
        return TzdValue((double)(n + 1));
    });

    reg("isnan_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        return TzdValue(std::isnan(valToDouble(args[0])));
    });

    reg("isinf_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        return TzdValue(std::isinf(valToDouble(args[0])));
    });

    reg("isfinite_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        return TzdValue(std::isfinite(valToDouble(args[0])));
    });

    // ---- Additional string functions ----
    reg("toTitleCase", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        bool newWord = true;
        for (char& c : s) {
            if (std::isspace((unsigned char)c)) { newWord = true; continue; }
            if (newWord) { c = (char)std::toupper((unsigned char)c); newWord = false; }
            else c = (char)std::tolower((unsigned char)c);
        }
        return TzdValue(s);
    });

    reg("levenshtein", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        std::string s1 = args[0].sVal, s2 = args[1].sVal;
        int m = (int)s1.size(), n = (int)s2.size();
        std::vector<int> prev(n + 1), curr(n + 1);
        for (int j = 0; j <= n; j++) prev[j] = j;
        for (int i = 1; i <= m; i++) {
            curr[0] = i;
            for (int j = 1; j <= n; j++) {
                int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
                curr[j] = (std::min)({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
            }
            prev = curr;
        }
        return TzdValue((double)prev[n]);
    });

    reg("countSubstr", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(0.0);
        std::string str = args[0].sVal, sub = args[1].sVal;
        if (sub.empty()) return TzdValue(0.0);
        int count = 0;
        size_t pos = 0;
        while ((pos = str.find(sub, pos)) != std::string::npos) {
            count++; pos += sub.size();
        }
        return TzdValue((double)count);
    });

    reg("wordCount", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        std::string s = args[0].sVal;
        int count = 0;
        bool inWord = false;
        for (char c : s) {
            if (std::isalpha((unsigned char)c)) {
                if (!inWord) { count++; inWord = true; }
            } else inWord = false;
        }
        return TzdValue((double)count);
    });

    reg("toCamelCase", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        std::string result;
        bool upperNext = false;
        for (char c : s) {
            if (c == '_' || c == '-' || c == ' ') { upperNext = true; continue; }
            result.push_back(upperNext ? (char)std::toupper((unsigned char)c) : c);
            upperNext = false;
        }
        return TzdValue(result);
    });

    reg("toSnakeCase", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        std::string result;
        for (size_t i = 0; i < s.size(); i++) {
            char c = s[i];
            if (std::isupper((unsigned char)c)) {
                if (!result.empty() && result.back() != '_') result.push_back('_');
                result.push_back((char)std::tolower((unsigned char)c));
            } else result.push_back(c);
        }
        return TzdValue(result);
    });

    reg("escape", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        std::string result;
        for (char c : s) {
            switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result.push_back(c);
            }
        }
        return TzdValue(result);
    });

    reg("unescape", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        std::string s = args[0].sVal;
        std::string result;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '\\' && i + 1 < s.size()) {
                switch (s[i + 1]) {
                case '"': result.push_back('"'); i++; break;
                case '\\': result.push_back('\\'); i++; break;
                case 'n': result.push_back('\n'); i++; break;
                case 'r': result.push_back('\r'); i++; break;
                case 't': result.push_back('\t'); i++; break;
                default: result.push_back(s[i]);
                }
            } else result.push_back(s[i]);
        }
        return TzdValue(result);
    });

    // ---- UUID generation ----
    reg("uuid", [](auto args) -> TzdValue {
        static std::mt19937_64 gen(std::random_device{}());
        std::uniform_int_distribution<uint64_t> dis;
        uint64_t a = dis(gen), b = dis(gen);
        a = (a & 0xFFFFFFFFFFFF0FFFULL) | 0x4000ULL; // version 4
        b = (b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL; // variant
        char buf[37];
        snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx",
            (uint32_t)(a >> 32), (uint16_t)((a >> 16) & 0xFFFF), (uint16_t)(a & 0xFFFF),
            (uint16_t)(b >> 48), (unsigned long long)(b & 0xFFFFFFFFFFFFULL));
        return TzdValue(std::string(buf));
    });

    // ---- Performance measurement ----
    reg("measure", [](auto args) -> TzdValue {
        // Returns a closure-like value: call once to start, call again to get elapsed
        // Simplified: just returns current high-res clock
        auto now = std::chrono::high_resolution_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
        return TzdValue((double)ns.count());
    });

    // ---- Additional data structure: Set operations ----
    reg("setCreate", [](auto args) -> TzdValue {
        std::unordered_map<std::string, TzdValue> m;
        for (const auto& a : args) {
            m[TzdInterpreter::getAsString(a)] = a;
        }
        TzdValue v(m);
        return v;
    });

    reg("setContains", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue(false);
        if (args[0].type != TzdValue::MAP) return TzdValue(false);
        return TzdValue(args[0].mapVal.count(TzdInterpreter::getAsString(args[1])) > 0);
    });

    reg("setAdd", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::MAP) return args[0];
        // Note: TzdValue is copy-on-write, so we return a new map
        std::unordered_map<std::string, TzdValue> m = args[0].mapVal;
        m[TzdInterpreter::getAsString(args[1])] = args[1];
        return TzdValue(m);
    });

    reg("setRemove", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::MAP) return args[0];
        std::unordered_map<std::string, TzdValue> m = args[0].mapVal;
        m.erase(TzdInterpreter::getAsString(args[1]));
        return TzdValue(m);
    });

    reg("setSize", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::MAP) return TzdValue(0.0);
        return TzdValue((double)args[0].mapVal.size());
    });

    reg("setUnion", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::MAP || args[1].type != TzdValue::MAP)
            return args.empty() ? TzdValue() : args[0];
        std::unordered_map<std::string, TzdValue> m = args[0].mapVal;
        for (const auto& [k, v] : args[1].mapVal) m[k] = v;
        return TzdValue(m);
    });

    reg("setIntersect", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::MAP || args[1].type != TzdValue::MAP)
            return TzdValue(std::unordered_map<std::string, TzdValue>{});
        std::unordered_map<std::string, TzdValue> result;
        for (const auto& [k, v] : args[0].mapVal) {
            if (args[1].mapVal.count(k)) result[k] = v;
        }
        return TzdValue(result);
    });

    reg("setDifference", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::MAP || args[1].type != TzdValue::MAP)
            return args.empty() ? TzdValue() : args[0];
        std::unordered_map<std::string, TzdValue> result;
        for (const auto& [k, v] : args[0].mapVal) {
            if (!args[1].mapVal.count(k)) result[k] = v;
        }
        return TzdValue(result);
    });

    // ---- Queue/Stack operations (using arrays) ----
    reg("queuePush", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return args.empty() ? TzdValue() : args[0];
        std::vector<TzdValue> arr = args[0].arrVal;
        arr.push_back(args[1]);
        return TzdValue(arr);
    });

    reg("queuePop", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty())
            return TzdValue();
        return args[0].arrVal.front();
    });

    reg("queuePopAll", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue(std::vector<TzdValue>{});
        return args[0]; // Return the array itself (FIFO order)
    });

    reg("stackPush", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::ARRAY) return args.empty() ? TzdValue() : args[0];
        std::vector<TzdValue> arr = args[0].arrVal;
        arr.push_back(args[1]);
        return TzdValue(arr);
    });

    reg("stackPop", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty())
            return TzdValue();
        return args[0].arrVal.back();
    });

    // ---- Additional conversion ----
    reg("toJSON", [](auto args) -> TzdValue {
        // Alias for jsonStringify
        if (args.empty()) return TzdValue("null");
        // Delegate to jsonStringify by calling it through the interpreter
        return TzdValue(); // Simplified - user should use jsonStringify directly
    });

    reg("fromJSON", [](auto args) -> TzdValue {
        // Alias for jsonParse
        return TzdValue();
    });

    reg("deepCopy", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        return args[0]; // TzdValue copy constructor does deep copy
    });

    reg("shuffle", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return args[0];
        std::vector<TzdValue> arr = args[0].arrVal;
        static std::mt19937_64 gen(std::random_device{}());
        std::shuffle(arr.begin(), arr.end(), gen);
        return TzdValue(arr);
    });

    reg("sample", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty())
            return TzdValue();
        int count = (args.size() > 1) ? (int)valToDouble(args[1]) : 1;
        std::vector<TzdValue> arr = args[0].arrVal;
        static std::mt19937_64 gen(std::random_device{}());
        std::shuffle(arr.begin(), arr.end(), gen);
        count = (std::min)(count, (int)arr.size());
        std::vector<TzdValue> result(arr.begin(), arr.begin() + count);
        return TzdValue(result);
    });

    reg("argmax", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty())
            return TzdValue(0.0);
        double maxVal = valToDouble(args[0].arrVal[0]);
        int maxIdx = 0;
        for (int i = 1; i < (int)args[0].arrVal.size(); i++) {
            double v = valToDouble(args[0].arrVal[i]);
            if (v > maxVal) { maxVal = v; maxIdx = i; }
        }
        return TzdValue((double)maxIdx);
    });

    reg("argmin", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.empty())
            return TzdValue(0.0);
        double minVal = valToDouble(args[0].arrVal[0]);
        int minIdx = 0;
        for (int i = 1; i < (int)args[0].arrVal.size(); i++) {
            double v = valToDouble(args[0].arrVal[i]);
            if (v < minVal) { minVal = v; minIdx = i; }
        }
        return TzdValue((double)minIdx);
    });

    reg("cumsum", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue(std::vector<TzdValue>{});
        std::vector<TzdValue> result;
        double sum = 0;
        for (const auto& v : args[0].arrVal) {
            sum += valToDouble(v);
            result.push_back(TzdValue(sum));
        }
        return TzdValue(result);
    });

    reg("diff", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY || args[0].arrVal.size() < 2)
            return TzdValue(std::vector<TzdValue>{});
        std::vector<TzdValue> result;
        for (size_t i = 1; i < args[0].arrVal.size(); i++) {
            result.push_back(TzdValue(valToDouble(args[0].arrVal[i]) - valToDouble(args[0].arrVal[i - 1])));
        }
        return TzdValue(result);
    });

    reg("linspace_arr", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue(std::vector<TzdValue>{});
        double start = valToDouble(args[0]);
        double end = valToDouble(args[1]);
        int n = (int)valToDouble(args[2]);
        if (n <= 0) return TzdValue(std::vector<TzdValue>{});
        std::vector<TzdValue> result;
        if (n == 1) { result.push_back(TzdValue(start)); return TzdValue(result); }
        double step = (end - start) / (n - 1);
        for (int i = 0; i < n; i++) result.push_back(TzdValue(start + i * step));
        return TzdValue(result);
    });

    // ---- OS info ----
    reg("getOsInfo", [](auto args) -> TzdValue {
        std::unordered_map<std::string, TzdValue> m;
#ifdef _WIN32
        m["os"] = TzdValue("Windows");
#else
        m["os"] = TzdValue("Linux/Unix");
#endif
        m["arch"] = TzdValue("x64");
        m["cores"] = TzdValue((double)std::thread::hardware_concurrency());
        return TzdValue(m);
    });

    // ---- Assertions and debugging ----
    reg("assert_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        bool cond = valToDouble(args[0]) != 0;
        if (!cond) {
            std::string msg = (args.size() > 1) ? args[1].sVal : "Assertion failed";
            throw std::runtime_error(msg);
        }
        return TzdValue();
    });

    reg("warn", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        std::cerr << "[WARNING] " << TzdInterpreter::getAsString(args[0]) << std::endl;
        return TzdValue();
    });

    // ---- Math constants ----
    reg("TAU", [](auto args) -> TzdValue { return TzdValue(6.28318530717958647692); });
    reg("SQRT2", [](auto args) -> TzdValue { return TzdValue(1.41421356237309504880); });
    reg("GOLDEN_RATIO", [](auto args) -> TzdValue { return TzdValue(1.61803398874989484820); });
    reg("EPSILON", [](auto args) -> TzdValue { return TzdValue(std::numeric_limits<double>::epsilon()); });

    // ---- Map utility functions (essential for Module system) ----
    reg("mapKeys", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::MAP) return TzdValue(std::vector<TzdValue>{});
        std::vector<TzdValue> result;
        for (const auto& [k, v] : args[0].mapVal) result.push_back(TzdValue(k));
        return TzdValue(result);
    });

    reg("mapValues", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::MAP) return TzdValue(std::vector<TzdValue>{});
        std::vector<TzdValue> result;
        for (const auto& [k, v] : args[0].mapVal) result.push_back(v);
        return TzdValue(result);
    });

    reg("mapHas", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::MAP) return TzdValue(false);
        return TzdValue(args[0].mapVal.count(args[1].sVal) > 0);
    });

    reg("mapGet", [](auto args) -> TzdValue {
        if (args.size() < 2 || args[0].type != TzdValue::MAP) return TzdValue();
        auto it = args[0].mapVal.find(args[1].sVal);
        if (it != args[0].mapVal.end()) return it->second;
        return (args.size() > 2) ? args[2] : TzdValue();
    });

    reg("mapEntries", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::MAP) return TzdValue(std::vector<TzdValue>{});
        std::vector<TzdValue> result;
        for (const auto& [k, v] : args[0].mapVal) {
            std::vector<TzdValue> entry;
            entry.push_back(TzdValue(k));
            entry.push_back(v);
            result.push_back(TzdValue(entry));
        }
        return TzdValue(result);
    });

    reg("mapFromEntries", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue(std::unordered_map<std::string, TzdValue>{});
        std::unordered_map<std::string, TzdValue> result;
        for (const auto& entry : args[0].arrVal) {
            if (entry.type == TzdValue::ARRAY && entry.arrVal.size() >= 2) {
                result[TzdInterpreter::getAsString(entry.arrVal[0])] = entry.arrVal[1];
            }
        }
        return TzdValue(result);
    });

    reg("mapMerge", [](auto args) -> TzdValue {
        if (args.size() < 2) return args.empty() ? TzdValue() : args[0];
        std::unordered_map<std::string, TzdValue> result;
        if (args[0].type == TzdValue::MAP) for (const auto& [k, v] : args[0].mapVal) result[k] = v;
        if (args[1].type == TzdValue::MAP) for (const auto& [k, v] : args[1].mapVal) result[k] = v;
        return TzdValue(result);
    });

    reg("mapFilter", [interp](auto args) -> TzdValue {
        // mapFilter(map, predicateFunc) - filter map entries
        // predicate receives (key, value) and returns bool
        if (args.size() < 2 || args[0].type != TzdValue::MAP) return TzdValue(std::unordered_map<std::string, TzdValue>{});
        std::unordered_map<std::string, TzdValue> result;
        for (const auto& [k, v] : args[0].mapVal) {
            std::vector<TzdValue> predArgs = {TzdValue(k), v};
            if (TzdInterpreter::getAsDoubleInternal(interp->callFunction(args[1], predArgs)) != 0.0) {
                result[k] = v;
            }
        }
        return TzdValue(result);
    });

    reg("mapMap", [interp](auto args) -> TzdValue {
        // mapMap(map, transformFunc) - transform map values
        // transform receives (key, value) and returns new value
        if (args.size() < 2 || args[0].type != TzdValue::MAP) return TzdValue(std::unordered_map<std::string, TzdValue>{});
        std::unordered_map<std::string, TzdValue> result;
        for (const auto& [k, v] : args[0].mapVal) {
            std::vector<TzdValue> tArgs = {TzdValue(k), v};
            result[k] = interp->callFunction(args[1], tArgs);
        }
        return TzdValue(result);
    });
}