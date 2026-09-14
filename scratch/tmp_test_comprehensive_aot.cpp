// ============================================================================
// Auto-generated Native C++ code by TzdLang AOT Machine Code Compiler
// Source: test_comprehensive_aot.tzd
// Target: Standalone x86_64 PE Executable (Zero-DLL Native Machine Code)
// ============================================================================

#define TZD_BUILD_CPU 1
#define NO_LIBTORCH 1
#include "TzdNativeRuntime.hpp"

using namespace tzd_rt;

// ── Forward Declarations ──
TzdVal test_math_lin_alg(std::vector<TzdVal> func_args = {});
TzdVal test_fractions_and_bigint(std::vector<TzdVal> func_args = {});
TzdVal test_json_and_symbols(std::vector<TzdVal> func_args = {});
TzdVal test_numerical_and_plot(std::vector<TzdVal> func_args = {});
TzdVal test_multithreading(std::vector<TzdVal> func_args = {});
TzdVal test_pytorch_ops(std::vector<TzdVal> func_args = {});
TzdVal main_func(std::vector<TzdVal> func_args = {});

// ── Class Definitions ──

// ── Function Definitions ──
TzdVal test_math_lin_alg(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 1. Testing Linear Algebra (inverse, solve, rank) ---")});
        TzdVal A = tzd_make_array({tzd_make_array({TzdVal(1.0), TzdVal(2.0)}), tzd_make_array({TzdVal(3.0), TzdVal(4.0)})});
        TzdVal invA = tzd_builtin_inverse({A});
        tzd_print_vec({(TzdVal("inv([[1,2],[3,4]]) = ") + tzd_builtin_str({invA}))});
        TzdVal M = tzd_make_array({tzd_make_array({TzdVal(2.0), TzdVal(1.0)}), tzd_make_array({TzdVal(1.0), (-TzdVal(1.0))})});
        TzdVal b = tzd_make_array({TzdVal(5.0), TzdVal(1.0)});
        TzdVal x = tzd_builtin_solve({M, b});
        tzd_print_vec({(TzdVal("solve([[2,1],[1,-1]], [5,1]) = ") + tzd_builtin_str({x}))});
        TzdVal R = tzd_make_array({tzd_make_array({TzdVal(1.0), TzdVal(2.0)}), tzd_make_array({TzdVal(2.0), TzdVal(4.0)})});
        TzdVal rk = tzd_builtin_rank({R});
        tzd_print_vec({(TzdVal("rank([[1,2],[2,4]]) = ") + tzd_builtin_str({rk}))});
    }
    return TzdVal();
}

TzdVal test_fractions_and_bigint(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 2. Testing toFraction & isBigint ---")});
        TzdVal f1 = tzd_builtin_toFraction({TzdVal(0.125)});
        tzd_print_vec({(TzdVal("toFraction(0.125) = ") + tzd_builtin_str({f1}))});
        TzdVal f2 = tzd_builtin_toFraction({TzdVal(0.3333333333333333)});
        tzd_print_vec({(TzdVal("toFraction(1/3) = ") + tzd_builtin_str({f2}))});
        TzdVal is_b1 = tzd_builtin_isBigint({TzdVal("12345678901234567890")});
        TzdVal is_b2 = tzd_builtin_isBigint({TzdVal("123a456")});
        tzd_print_vec({(((TzdVal("isBigint('12345678901234567890') = ") + tzd_builtin_str({is_b1})) + TzdVal(", isBigint('123a') = ")) + tzd_builtin_str({is_b2}))});
    }
    return TzdVal();
}

TzdVal test_json_and_symbols(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 3. Testing JSON Parser, Serializer & Reflection ---")});
        TzdVal rawJson = TzdVal("{\"title\":\"TzdLang\",\"version\":2.0,\"features\":[\"AOT\",\"PyTorch\"],\"ready\":true}");
        TzdVal parsed = tzd_builtin_jsonParse({rawJson});
        tzd_print_vec({(TzdVal("jsonParse result keys = ") + tzd_builtin_str({tzd_builtin_keys({parsed})}))});
        TzdVal serialized = tzd_builtin_jsonStringify({parsed});
        tzd_print_vec({(TzdVal("jsonStringify result = ") + serialized)});
        TzdVal syms = tzd_builtin_getSymbols({});
        tzd_print_vec({(TzdVal("getSymbols() count = ") + tzd_builtin_str({tzd_builtin_len({syms})}))});
        TzdVal nfuncs = tzd_builtin_getNativeFunctions({});
        tzd_print_vec({(TzdVal("getNativeFunctions() count = ") + tzd_builtin_str({tzd_builtin_len({nfuncs})}))});
    }
    return TzdVal();
}

TzdVal test_numerical_and_plot(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 4. Testing Derivative, Equation Solver & Plot ---")});
        TzdVal d = tzd_builtin_derivative({TzdVal("x^2"), TzdVal(3.0)});
        tzd_print_vec({(TzdVal("d/dx(x^2) at x=3 = ") + tzd_builtin_str({d}))});
        TzdVal root = tzd_builtin_solveEq({TzdVal("x^2 - 4"), TzdVal(1.5)});
        tzd_print_vec({(TzdVal("solveEq('x^2 - 4', 1.5) = ") + tzd_builtin_str({root}))});
        tzd_print_vec({TzdVal("Calling plot('sin(x)', 0, 6.28):")});
        tzd_builtin_plot({TzdVal("sin(x)"), TzdVal(0.0), TzdVal(6.2831853)});
    }
    return TzdVal();
}

TzdVal test_multithreading(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 5. Testing Multi-threading ---")});
        TzdVal t1 = tzd_builtin_sys_thread_start({TzdVal([=](std::vector<TzdVal> l_args) -> TzdVal {
        {
        tzd_print_vec({TzdVal("Worker thread running!")});
    }
        return TzdVal();
    })});
        tzd_print_vec({(TzdVal("Thread started, id = ") + tzd_builtin_str({t1}))});
        tzd_builtin_sys_thread_join({t1});
        tzd_print_vec({TzdVal("Thread joined successfully!")});
    }
    return TzdVal();
}

TzdVal test_pytorch_ops(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 6. Testing PyTorch Built-in Operators ---")});
        TzdVal z = tzd_builtin_torch_zeros({TzdVal(4LL)});
        tzd_print_vec({(TzdVal("torch_zeros(4) = ") + tzd_builtin_str({z}))});
        TzdVal o = tzd_builtin_torch_ones({TzdVal(4LL)});
        tzd_print_vec({(TzdVal("torch_ones(4) = ") + tzd_builtin_str({o}))});
        TzdVal s = tzd_builtin_torch_add({z, o});
        tzd_print_vec({(TzdVal("torch_add(z, o) = ") + tzd_builtin_str({s}))});
        TzdVal inp = tzd_make_array({tzd_make_array({tzd_make_array({tzd_make_array({TzdVal(1.0), TzdVal(2.0), TzdVal(3.0)}), tzd_make_array({TzdVal(4.0), TzdVal(5.0), TzdVal(6.0)}), tzd_make_array({TzdVal(7.0), TzdVal(8.0), TzdVal(9.0)})})})});
        TzdVal weight = tzd_make_array({tzd_make_array({tzd_make_array({tzd_make_array({TzdVal(1.0)})})})});
        TzdVal conv_out = tzd_builtin_torch_conv2d({inp, weight});
        tzd_print_vec({(TzdVal("torch_conv2d out = ") + tzd_builtin_str({conv_out}))});
        TzdVal pred = tzd_make_array({TzdVal(1.0), TzdVal(2.0), TzdVal(3.0)});
        TzdVal target = tzd_make_array({TzdVal(1.0), TzdVal(2.0), TzdVal(5.0)});
        TzdVal loss = tzd_builtin_torch_mse_loss({pred, target});
        tzd_print_vec({(TzdVal("torch_mse_loss([1,2,3], [1,2,5]) = ") + tzd_builtin_str({loss}))});
        TzdVal p = tzd_make_array({TzdVal(10.0), (-TzdVal(5.0))});
        TzdVal grad = tzd_make_array({TzdVal(1.0), (-TzdVal(0.5))});
        TzdVal opt_step = tzd_builtin_torch_adam({p, grad, TzdVal(0.1)});
        tzd_print_vec({(TzdVal("torch_adam step = ") + tzd_builtin_str({opt_step}))});
    }
    return TzdVal();
}

TzdVal main_func(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("==================================================")});
        tzd_print_vec({TzdVal("  TzdLang Comprehensive Builtin & PyTorch Test")});
        tzd_print_vec({TzdVal("==================================================")});
        test_math_lin_alg({});
        test_fractions_and_bigint({});
        test_json_and_symbols({});
        test_numerical_and_plot({});
        test_multithreading({});
        test_pytorch_ops({});
        tzd_print_vec({TzdVal("==================================================")});
        tzd_print_vec({TzdVal(">>> ALL COMPREHENSIVE TESTS FINISHED! <<<")});
    }
    return TzdVal();
}


// ── Top-Level Program Logic ──
void tzd_top_level() {
}

// ── Application Entry Point ──
int main(int argc, char* argv[]) {
    init_console();
    std::vector<TzdVal> argv_vec;
    argv_vec.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        argv_vec.push_back(TzdVal(argv[i]));
    }
    TzdVal ARGV = tzd_make_array({});
    ARGV.arrVal = std::make_shared<std::vector<TzdVal>>(argv_vec);
    TzdVal args = ARGV;

    try {
        tzd_top_level();
        main_func({ARGV});
    } catch (const TzdVal& ex) {
        std::cerr << "[Tzd Runtime Error] " << ex.to_string() << std::endl;
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "[System Exception] " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[Fatal Error] Unknown uncaught exception." << std::endl;
        return 1;
    }
    return 0;
}
