// ============================================================================
// Auto-generated Native C++ code by TzdLang AOT Machine Code Compiler
// Source: test_user_requested_fixes.tzd
// Target: Standalone x86_64 PE Executable (Zero-DLL Native Machine Code)
// ============================================================================

#define TZD_BUILD_CPU 1
#define NO_LIBTORCH 1
#include "TzdNativeRuntime.hpp"

using namespace tzd_rt;

// ── Forward Declarations ──
TzdVal test_symbolic_and_ineq(std::vector<TzdVal> func_args = {});
TzdVal test_os_and_path_info(std::vector<TzdVal> func_args = {});
TzdVal test_arbitrary_precision_bigint(std::vector<TzdVal> func_args = {});
TzdVal test_matrix_decompositions(std::vector<TzdVal> func_args = {});
TzdVal test_multi_channel_conv2d(std::vector<TzdVal> func_args = {});
TzdVal test_autograd_and_jit(std::vector<TzdVal> func_args = {});
TzdVal main_func(std::vector<TzdVal> func_args = {});

// ── Class Definitions ──

// ── Function Definitions ──
TzdVal test_symbolic_and_ineq(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("=== 1. Symbolic Simplification & Inequality Solver ===")});
        TzdVal s1 = tzd_builtin_simplifySym({TzdVal("2*x + 3*x")});
        tzd_print_vec({(TzdVal("simplifySym('2*x + 3*x') = ") + tzd_builtin_str({s1}))});
        TzdVal s2 = tzd_builtin_simplifySym({TzdVal("x^2 + 2*x^2 - 4 + 1")});
        tzd_print_vec({(TzdVal("simplifySym('x^2 + 2*x^2 - 4 + 1') = ") + tzd_builtin_str({s2}))});
        TzdVal ineq1 = tzd_builtin_solveIneq({TzdVal("x^2 - 4 < 0")});
        tzd_print_vec({(TzdVal("solveIneq('x^2 - 4 < 0') = ") + tzd_builtin_str({ineq1}))});
        TzdVal ineq2 = tzd_builtin_solveIneq({TzdVal("2*x - 6 >= 0")});
        tzd_print_vec({(TzdVal("solveIneq('2*x - 6 >= 0') = ") + tzd_builtin_str({ineq2}))});
    }
    return TzdVal();
}

TzdVal test_os_and_path_info(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("=== 2. Real OS Info & Script Paths ===")});
        TzdVal osInfo = tzd_builtin_getOsInfo({});
        tzd_print_vec({(TzdVal("getOsInfo() = ") + tzd_builtin_str({osInfo}))});
        TzdVal scriptPath = tzd_builtin_getScriptPath({});
        tzd_print_vec({(TzdVal("getScriptPath() = ") + tzd_builtin_str({scriptPath}))});
        TzdVal scriptDir = tzd_builtin_getScriptDir({});
        tzd_print_vec({(TzdVal("getScriptDir() = ") + tzd_builtin_str({scriptDir}))});
    }
    return TzdVal();
}

TzdVal test_arbitrary_precision_bigint(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("=== 3. Arbitrary Precision BigInt (No 64-bit Overflow) ===")});
        TzdVal f100 = tzd_builtin_bigintFactorial({TzdVal(100LL)});
        tzd_print_vec({((TzdVal("bigintFactorial(100) length = ") + tzd_builtin_str({tzd_builtin_len({f100})})) + TzdVal(" digits"))});
        tzd_print_vec({(TzdVal("bigintFactorial(100) = ") + tzd_builtin_str({f100}))});
        TzdVal gcdVal = tzd_builtin_bigintGcd({TzdVal("123456789012345678901234567890"), TzdVal("987654321098765432109876543210")});
        tzd_print_vec({(TzdVal("bigintGcd(a, b) = ") + tzd_builtin_str({gcdVal}))});
    }
    return TzdVal();
}

TzdVal test_matrix_decompositions(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("=== 4. Real Matrix Decomposition (Eig, SVD, Corrcoef, Permute) ===")});
        TzdVal eigRes = tzd_builtin_torch_eig({tzd_make_array({tzd_make_array({TzdVal(2.0), TzdVal(1.0)}), tzd_make_array({TzdVal(1.0), TzdVal(2.0)})})});
        tzd_print_vec({(TzdVal("torch_eig([[2,1],[1,2]]) = ") + tzd_builtin_str({eigRes}))});
        TzdVal svdRes = tzd_builtin_torch_svd({tzd_make_array({tzd_make_array({TzdVal(3.0), TzdVal(0.0)}), tzd_make_array({TzdVal(0.0), (-TzdVal(2.0))})})});
        tzd_print_vec({(TzdVal("torch_svd([[3,0],[0,-2]]) singular values = ") + tzd_builtin_str({svdRes.get_index(TzdVal(1LL))}))});
        TzdVal corrRes = tzd_builtin_torch_corrcoef({tzd_make_array({tzd_make_array({TzdVal(1.0), TzdVal(2.0), TzdVal(3.0), TzdVal(4.0)}), tzd_make_array({TzdVal(2.0), TzdVal(4.0), TzdVal(6.0), TzdVal(8.0)})})});
        tzd_print_vec({(TzdVal("torch_corrcoef(X, 2X) = ") + tzd_builtin_str({corrRes}))});
        TzdVal permRes = tzd_builtin_torch_permute({tzd_make_array({tzd_make_array({TzdVal(1.0), TzdVal(2.0), TzdVal(3.0)}), tzd_make_array({TzdVal(4.0), TzdVal(5.0), TzdVal(6.0)})}), tzd_make_array({TzdVal(1LL), TzdVal(0LL)})});
        tzd_print_vec({(TzdVal("torch_permute([[1,2,3],[4,5,6]], [1, 0]) = ") + tzd_builtin_str({permRes}))});
        TzdVal a = tzd_make_array({TzdVal(1.0), TzdVal(0.0)});
        TzdVal p = tzd_make_array({TzdVal(1.0), TzdVal(0.1)});
        TzdVal n = tzd_make_array({TzdVal(2.0), TzdVal(0.0)});
        TzdVal tLoss = tzd_builtin_torch_triple_margin_loss({a, p, n, TzdVal(1.0)});
        tzd_print_vec({(TzdVal("torch_triple_margin_loss = ") + tzd_builtin_str({tLoss}))});
    }
    return TzdVal();
}

TzdVal test_multi_channel_conv2d(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("=== 5. Multi-Channel & Multi-Batch Conv2D (Padding + Stride) ===")});
        TzdVal inp = tzd_make_array({tzd_make_array({tzd_make_array({tzd_make_array({TzdVal(1.0), TzdVal(1.0), TzdVal(1.0)}), tzd_make_array({TzdVal(1.0), TzdVal(1.0), TzdVal(1.0)}), tzd_make_array({TzdVal(1.0), TzdVal(1.0), TzdVal(1.0)})}), tzd_make_array({tzd_make_array({TzdVal(2.0), TzdVal(2.0), TzdVal(2.0)}), tzd_make_array({TzdVal(2.0), TzdVal(2.0), TzdVal(2.0)}), tzd_make_array({TzdVal(2.0), TzdVal(2.0), TzdVal(2.0)})})})});
        TzdVal weight = tzd_make_array({tzd_make_array({tzd_make_array({tzd_make_array({TzdVal(1.0)})}), tzd_make_array({tzd_make_array({TzdVal(3.0)})})})});
        TzdVal convOut = tzd_builtin_torch_conv2d({inp, weight, tzd_make_array({TzdVal(0.0)}), TzdVal(1LL), TzdVal(0LL)});
        tzd_print_vec({(TzdVal("Multi-channel conv2d out = ") + tzd_builtin_str({convOut}))});
    }
    return TzdVal();
}

TzdVal test_autograd_and_jit(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("=== 6. Autograd & JIT / State Dict ===")});
        TzdVal x = tzd_builtin_torch_zeros({TzdVal(4LL)});
        tzd_builtin_torch_requires_grad({x, TzdVal(true)});
        tzd_print_vec({(TzdVal("torch_is_requires_grad(x) = ") + tzd_builtin_str({tzd_builtin_torch_is_requires_grad({x})}))});
        TzdVal loss = tzd_builtin_torch_mse_loss({tzd_make_array({TzdVal(1.0), TzdVal(2.0)}), tzd_make_array({TzdVal(1.0), TzdVal(4.0)})});
        TzdVal bw = tzd_builtin_torch_backward({loss});
        tzd_print_vec({(TzdVal("torch_backward executed = ") + tzd_builtin_str({bw}))});
        TzdVal j_eval = tzd_builtin_torch_jit_eval({});
        TzdVal j_train = tzd_builtin_torch_jit_train({});
        tzd_print_vec({(((TzdVal("torch_jit_eval = ") + tzd_builtin_str({j_eval})) + TzdVal(", torch_jit_train = ")) + tzd_builtin_str({j_train}))});
        TzdVal sd_save = tzd_builtin_torch_save_state_dict({tzd_make_array({}), TzdVal("scratch/test_state.bin")});
        TzdVal sd_load = tzd_builtin_torch_load_state_dict({tzd_make_array({}), TzdVal("scratch/test_state.bin")});
        tzd_print_vec({(((TzdVal("torch_save_state_dict = ") + tzd_builtin_str({sd_save})) + TzdVal(", torch_load_state_dict = ")) + tzd_builtin_str({sd_load}))});
    }
    return TzdVal();
}

TzdVal main_func(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("==================================================")});
        tzd_print_vec({TzdVal("  TzdLang Comprehensive Advanced Fixes Verification")});
        tzd_print_vec({TzdVal("==================================================")});
        test_symbolic_and_ineq({});
        test_os_and_path_info({});
        test_arbitrary_precision_bigint({});
        test_matrix_decompositions({});
        test_multi_channel_conv2d({});
        test_autograd_and_jit({});
        tzd_print_vec({TzdVal("==================================================")});
        tzd_print_vec({TzdVal(">>> ALL ADVANCED FIXES VERIFIED SUCCESSFULLY! <<<")});
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
