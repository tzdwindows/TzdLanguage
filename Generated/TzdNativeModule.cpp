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

#include "TzdInterpreter.h"
#include "TzdOop.h"

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
        static const auto start_time = std::chrono::high_resolution_clock::now();
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
        case TzdValue::FLOAT: return TzdValue("FLOAT");
        case TzdValue::STRING: return TzdValue("STRING");
        case TzdValue::BOOL: return TzdValue("BOOL");
        case TzdValue::FUNCTION: return TzdValue("FUNCTION");
        case TzdValue::ARRAY: return TzdValue("ARRAY");
        default: return TzdValue("UNKNOWN");
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
            { "factorial", "n", "?????? (n!)" },
            { "toFraction", "val, [tolerance=1e-5]", "???????????????????????????????????????" },
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
                if (!val.instanceVal) return 0;

                // 1. ???????????? TzdInstance ????????????????
                int size = (int)(sizeof(void*) + sizeof(TzdInstance));

                // 2. ??????? TzdInstance ??? std::unordered_map<std::string, struct TzdValue> fields
                for (const auto& [fieldName, fieldValue] : val.instanceVal->fields) {
                    // ?????????????????????? + ????????
                    size += (int)(fieldName.capacity() + auditSize(fieldValue));
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