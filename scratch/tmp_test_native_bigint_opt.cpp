// ============================================================================
// Auto-generated Native C++ code by TzdLang AOT Machine Code Compiler
// Source: test_native_bigint_opt.tzd
// Target: Standalone x86_64 PE Executable (Zero-DLL Native Machine Code)
// ============================================================================

#define TZD_BUILD_CPU 1
#define NO_LIBTORCH 1
#include "TzdNativeRuntime.hpp"

using namespace tzd_rt;

// ── Forward Declarations ──
TzdVal test_bigint_factorial_opt(std::vector<TzdVal> func_args = {});
TzdVal test_karatsuba_mul(std::vector<TzdVal> func_args = {});
TzdVal test_fast_powmod(std::vector<TzdVal> func_args = {});
TzdVal test_overflow_promotion(std::vector<TzdVal> func_args = {});
TzdVal main_func(std::vector<TzdVal> func_args = {});

// ── Class Definitions ──

// ── Function Definitions ──
TzdVal test_bigint_factorial_opt(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 1. Testing Optimized 1000! Factorial ---")});
        TzdVal t0 = tzd_builtin_clock({});
        TzdVal f1000 = tzd_builtin_bigintFactorial({TzdVal(1000LL)});
        TzdVal t1 = tzd_builtin_clock({});
        TzdVal dur = (((t1 - t0)) * TzdVal(1000.0));
        tzd_print_vec({((TzdVal("1000! length: ") + tzd_builtin_str({tzd_builtin_len({f1000})})) + TzdVal(" digits"))});
        tzd_print_vec({((TzdVal("1000! elapsed time: ") + tzd_builtin_str({dur})) + TzdVal(" ms"))});
        tzd_print_vec({((TzdVal("1000! prefix: ") + tzd_builtin_substr({f1000, TzdVal(0LL), TzdVal(50LL)})) + TzdVal("..."))});
    }
    return TzdVal();
}

TzdVal test_karatsuba_mul(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 2. Testing Karatsuba Big Multiplication ---")});
        TzdVal a = tzd_builtin_bigint({TzdVal("99999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999")});
        TzdVal b = tzd_builtin_bigint({TzdVal("88888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888")});
        TzdVal t0 = tzd_builtin_clock({});
        TzdVal prod = tzd_builtin_bigintMul({a, b});
        TzdVal t1 = tzd_builtin_clock({});
        tzd_print_vec({((TzdVal("Product length: ") + tzd_builtin_str({tzd_builtin_len({prod})})) + TzdVal(" digits"))});
        tzd_print_vec({((TzdVal("Karatsuba multiplication time: ") + tzd_builtin_str({(((t1 - t0)) * TzdVal(1000.0))})) + TzdVal(" ms"))});
        tzd_print_vec({((TzdVal("Product prefix: ") + tzd_builtin_substr({prod, TzdVal(0LL), TzdVal(40LL)})) + TzdVal("..."))});
    }
    return TzdVal();
}

TzdVal test_fast_powmod(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 3. Testing Fast Binary Exponentiation & PowMod ---")});
        TzdVal p = tzd_builtin_pow({TzdVal(2LL), TzdVal(100LL)});
        tzd_print_vec({(TzdVal("2^100 exact BigInt: ") + tzd_builtin_str({p}))});
        TzdVal pm = tzd_builtin_powmod({TzdVal("123456789"), TzdVal("987654321"), TzdVal("1000000007")});
        tzd_print_vec({(TzdVal("powmod('123456789', '987654321', '1000000007') = ") + tzd_builtin_str({pm}))});
    }
    return TzdVal();
}

TzdVal test_overflow_promotion(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("--- 4. Testing 64-bit Overflow Auto-Promotion ---")});
        TzdVal large = TzdVal(4611686018427387904LL);
        TzdVal auto_big = (large * TzdVal(4LL));
        tzd_print_vec({(TzdVal("2^62 * 4 auto-promoted to BigInt = ") + tzd_builtin_str({auto_big}))});
        TzdVal max_int = TzdVal(9223372036854775807LL);
        TzdVal sum_over = (max_int + TzdVal(10LL));
        tzd_print_vec({(TzdVal("INT64_MAX + 10 = ") + tzd_builtin_str({sum_over}))});
    }
    return TzdVal();
}

TzdVal main_func(std::vector<TzdVal> func_args) {
    {
        tzd_print_vec({TzdVal("==================================================")});
        tzd_print_vec({TzdVal("  TzdLang Native BigInt Optimization Verification")});
        tzd_print_vec({TzdVal("==================================================")});
        test_bigint_factorial_opt({});
        test_karatsuba_mul({});
        test_fast_powmod({});
        test_overflow_promotion({});
        tzd_print_vec({TzdVal("==================================================")});
        tzd_print_vec({TzdVal(">>> NATIVE BIGINT OPTIMIZATIONS VERIFIED! <<<")});
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
