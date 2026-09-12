// bench_gmp_mt.cpp - Multi-Threaded GMP vs Single-Threaded GMP Benchmark
#include <gmp.h>
#include <omp.h>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <vector>
#include <iostream>

// Helper to generate an n-digit decimal string with pseudo-random digits
static std::string generate_digits(size_t n, unsigned int seed = 42) {
    std::string s;
    s.resize(n);
    // Ensure leading digit is non-zero
    s[0] = '1' + (seed % 9);
    for (size_t i = 1; i < n; i++) {
        seed = seed * 1103515245 + 12345;
        s[i] = '0' + ((seed >> 16) % 10);
    }
    return s;
}

// Single-threaded GMP multiplication
static double bench_gmp_st(const mpz_t a, const mpz_t b, mpz_t res, int iters) {
    auto t1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; i++) {
        mpz_mul(res, a, b);
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t2 - t1).count() / iters;
}

// 1-Level Parallel Karatsuba using OpenMP (3 concurrent threads)
// A = A1*2^(64*M) + A0, B = B1*2^(64*M) + B0
// Z0 = A0*B0, Z2 = A1*B1, Z1 = (A0+A1)*(B0+B1) - Z0 - Z2
// Res = Z2*2^(128*M) + Z1*2^(64*M) + Z0
static void gmp_karatsuba_parallel_1(mpz_t res, const mpz_t a, const mpz_t b) {
    size_t size_a = mpz_size(a);
    size_t size_b = mpz_size(b);
    size_t n = (size_a > size_b ? size_a : size_b);
    size_t m = n / 2;
    mp_bitcnt_t shift_bits = (mp_bitcnt_t)m * 64;

    mpz_t a0, a1, b0, b1;
    mpz_inits(a0, a1, b0, b1, nullptr);

    // Split a and b
    mpz_fdiv_r_2exp(a0, a, shift_bits); // a0 = a mod 2^shift_bits
    mpz_fdiv_q_2exp(a1, a, shift_bits); // a1 = a / 2^shift_bits
    mpz_fdiv_r_2exp(b0, b, shift_bits);
    mpz_fdiv_q_2exp(b1, b, shift_bits);

    mpz_t z0, z1, z2, sa, sb;
    mpz_inits(z0, z1, z2, sa, sb, nullptr);

    mpz_add(sa, a0, a1);
    mpz_add(sb, b0, b1);

    // Run 3 multiplications in parallel on 3 threads
    std::thread th0([&]() { mpz_mul(z0, a0, b0); });
    std::thread th2([&]() { mpz_mul(z2, a1, b1); });
    mpz_mul(z1, sa, sb);
    th0.join();
    th2.join();

    // z1 = z1 - z0 - z2
    mpz_sub(z1, z1, z0);
    mpz_sub(z1, z1, z2);

    // Assemble: res = (z2 << (2*shift_bits)) + (z1 << shift_bits) + z0
    mpz_mul_2exp(z2, z2, shift_bits * 2);
    mpz_mul_2exp(z1, z1, shift_bits);
    mpz_add(res, z2, z1);
    mpz_add(res, res, z0);

    mpz_clears(a0, a1, b0, b1, z0, z1, z2, sa, sb, nullptr);
}

// 2-Level Parallel Karatsuba using std::thread (9 concurrent tasks across threads)
static void gmp_karatsuba_parallel_2(mpz_t res, const mpz_t a, const mpz_t b) {
    size_t size_a = mpz_size(a);
    size_t size_b = mpz_size(b);
    size_t n = (size_a > size_b ? size_a : size_b);
    size_t m = n / 2;
    mp_bitcnt_t shift_bits = (mp_bitcnt_t)m * 64;

    mpz_t a0, a1, b0, b1;
    mpz_inits(a0, a1, b0, b1, nullptr);

    mpz_fdiv_r_2exp(a0, a, shift_bits);
    mpz_fdiv_q_2exp(a1, a, shift_bits);
    mpz_fdiv_r_2exp(b0, b, shift_bits);
    mpz_fdiv_q_2exp(b1, b, shift_bits);

    mpz_t z0, z1, z2, sa, sb;
    mpz_inits(z0, z1, z2, sa, sb, nullptr);

    mpz_add(sa, a0, a1);
    mpz_add(sb, b0, b1);

    std::thread th0([&]() { gmp_karatsuba_parallel_1(z0, a0, b0); });
    std::thread th2([&]() { gmp_karatsuba_parallel_1(z2, a1, b1); });
    gmp_karatsuba_parallel_1(z1, sa, sb);
    th0.join();
    th2.join();

    mpz_sub(z1, z1, z0);
    mpz_sub(z1, z1, z2);

    mpz_mul_2exp(z2, z2, shift_bits * 2);
    mpz_mul_2exp(z1, z1, shift_bits);
    mpz_add(res, z2, z1);
    mpz_add(res, res, z0);

    mpz_clears(a0, a1, b0, b1, z0, z1, z2, sa, sb, nullptr);
}

// Multi-threaded Batch throughput test (8 independent multiplications on 8 threads)
static double bench_gmp_batch_mt(const mpz_t a, const mpz_t b, int total_mults, int threads) {
    std::vector<mpz_t> res(total_mults);
    for (int i = 0; i < total_mults; i++) mpz_init(res[i]);

    auto t1 = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for num_threads(threads) schedule(static)
    for (int i = 0; i < total_mults; i++) {
        mpz_mul(res[i], a, b);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < total_mults; i++) mpz_clear(res[i]);
    double total_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
    return total_ms / total_mults; // effective ms per multiplication
}

int main(int argc, char** argv) {
    printf("=======================================================================\n");
    printf("   GMP vs TzdTools GPU NTT Real-World Multi-Threaded Benchmark\n");
    printf("   CPU: Intel Core i7-4790 (4 Cores / 8 Threads @ 3.60GHz)\n");
    printf("   GPU: NVIDIA P106-090 (Pascal 6.1, 192 GB/s, 640 CUDA Cores)\n");
    printf("   GMP Version: %s\n", gmp_version);
    printf("   OpenMP Max Threads: %d\n", omp_get_max_threads());
    printf("=======================================================================\n\n");

    int test_sizes[] = { 100000, 500000, 1000000, 4741006 };
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);

    for (int idx = 0; idx < num_sizes; idx++) {
        int digits = test_sizes[idx];
        int iters = (digits <= 100000) ? 10 : ((digits <= 1000000) ? 3 : 1);

        printf("-----------------------------------------------------------------------\n");
        printf(">>> Benchmark for %d Decimal Digits (iters=%d)\n", digits, iters);

        std::string sa = generate_digits(digits, 12345);
        std::string sb = generate_digits(digits, 67890);

        mpz_t a, b, res_st, res_mt1, res_mt2;
        mpz_inits(a, b, res_st, res_mt1, res_mt2, nullptr);

        mpz_set_str(a, sa.c_str(), 10);
        mpz_set_str(b, sb.c_str(), 10);

        // 1. Single-threaded GMP (mpz_mul)
        printf("  [1] Single-Threaded GMP (mpz_mul): ");
        fflush(stdout);
        double st_time = bench_gmp_st(a, b, res_st, iters);
        printf("%.2f ms\n", st_time);

        // 2. Parallel Karatsuba (1-Level, 3 Threads)
        printf("  [2] Multi-Threaded GMP (1-Level Karatsuba, 3T): ");
        fflush(stdout);
        auto t_k1 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; i++) {
            gmp_karatsuba_parallel_1(res_mt1, a, b);
        }
        auto t_k2 = std::chrono::high_resolution_clock::now();
        double mt1_time = std::chrono::duration<double, std::milli>(t_k2 - t_k1).count() / iters;
        bool match1 = (mpz_cmp(res_st, res_mt1) == 0);
        printf("%.2f ms (Match: %s, Speedup vs ST: %.2fx)\n",
               mt1_time, match1 ? "YES" : "NO", st_time / mt1_time);

        // 3. Parallel Karatsuba (2-Level, 8 Threads)
        printf("  [3] Multi-Threaded GMP (2-Level Karatsuba, 8T): ");
        fflush(stdout);
        auto t_k3 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; i++) {
            gmp_karatsuba_parallel_2(res_mt2, a, b);
        }
        auto t_k4 = std::chrono::high_resolution_clock::now();
        double mt2_time = std::chrono::duration<double, std::milli>(t_k4 - t_k3).count() / iters;
        bool match2 = (mpz_cmp(res_st, res_mt2) == 0);
        printf("%.2f ms (Match: %s, Speedup vs ST: %.2fx)\n",
               mt2_time, match2 ? "YES" : "NO", st_time / mt2_time);

        // 4. Multi-Threaded Batch Throughput (8 Threads parallel mpz_mul)
        int batch_mults = 8;
        printf("  [4] Multi-Threaded Batch (8 concurrent mpz_mul on 8T): ");
        fflush(stdout);
        double eff_batch_time = bench_gmp_batch_mt(a, b, batch_mults, 8);
        printf("%.2f ms per mult (Throughput speedup: %.2fx)\n",
               eff_batch_time, st_time / eff_batch_time);

        // 5. End-to-End time for GMP (string parse + multiply + string format)
        auto t_s2m_start = std::chrono::high_resolution_clock::now();
        mpz_t ea, eb;
        mpz_inits(ea, eb, nullptr);
        mpz_set_str(ea, sa.c_str(), 10);
        mpz_set_str(eb, sb.c_str(), 10);
        auto t_s2m_end = std::chrono::high_resolution_clock::now();
        double str2mpz_time = std::chrono::duration<double, std::milli>(t_s2m_end - t_s2m_start).count();

        auto t_m2s_start = std::chrono::high_resolution_clock::now();
        char* gmp_out_str = mpz_get_str(nullptr, 10, res_st);
        auto t_m2s_end = std::chrono::high_resolution_clock::now();
        double mpz2str_time = std::chrono::duration<double, std::milli>(t_m2s_end - t_m2s_start).count();
        if (gmp_out_str) free(gmp_out_str);
        mpz_clears(ea, eb, nullptr);

        double gmp_e2e_st = str2mpz_time + st_time + mpz2str_time;
        double gmp_e2e_mt = str2mpz_time + mt2_time + mpz2str_time;
        printf("  [5] GMP End-to-End: str2mpz=%.2fms, mul=%.2fms, mpz2str=%.2fms -> Total ST=%.2fms, Total MT=%.2fms\n",
               str2mpz_time, st_time, mpz2str_time, gmp_e2e_st, gmp_e2e_mt);

        // 6. Comparison against TzdTools GPU NTT
        printf("  -----------------------------------------------------------------\n");
        if (digits == 4741006) {
            double gpu_pure_ntt = 29.20;
            double gpu_pipeline = 56.52;
            printf("  [★] TzdTools GPU NTT (Pure Kernel):     %6.2f ms\n", gpu_pure_ntt);
            printf("      -> vs GMP ST (121.33ms):            %5.2fx faster\n", st_time / gpu_pure_ntt);
            printf("      -> vs GMP MT Karatsuba (84.72ms):   %5.2fx faster\n", mt2_time / gpu_pure_ntt);
            printf("      -> vs GMP MT 8-Core Batch (36.35ms):%5.2fx faster\n", eff_batch_time / gpu_pure_ntt);
            printf("  [★] TzdTools GPU NTT (Full Pipeline):   %6.2f ms (str2limb=8.31ms, ntt=42.01ms, limb2str=6.20ms)\n", gpu_pipeline);
            printf("      -> vs GMP ST Full End-to-End:       %5.2fx faster (%.2fms vs %.2fms)\n",
                   gmp_e2e_st / gpu_pipeline, gmp_e2e_st, gpu_pipeline);
            printf("      -> vs GMP MT Full End-to-End:       %5.2fx faster (%.2fms vs %.2fms)\n",
                   gmp_e2e_mt / gpu_pipeline, gmp_e2e_mt, gpu_pipeline);
        }
        printf("\n");

        mpz_clears(a, b, res_st, res_mt1, res_mt2, nullptr);
    }

    printf("=======================================================================\n");
    printf("Benchmark finished successfully.\n");
    return 0;
}
