#include "TzdExperimentalCompute.h"
#include "TzdInterpreter.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>
#include <string>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <immintrin.h>
#include <omp.h>

// Global flag controlling experimental compute engine
bool g_experimentalCompute = false;

namespace {

// ============================================================================
// 1. Three NTT-friendly Primes (Fit in 32-bit registers for 2x cache bandwidth)
// ============================================================================
// P1 = 119 * 2^23 + 1
// P2 = 235 * 2^22 + 1
// P3 = 45  * 2^24 + 1
// Max transform length: 2^22 = 4,194,304 elements (~38 million decimal digits)
// ============================================================================
static const uint32_t P[3]       = { 998244353U, 985661441U, 754974721U };
static const uint32_t GEN[3]     = { 3U,         3U,         11U };
static const uint32_t P_INV[3]   = { 998244351U, 985661439U, 754974719U };
static const uint32_t R2_MOD[3]  = { 932051910U, 616455619U, 749009521U };

// Precomputed CRT constants
// inv12  = P1^(-1) mod P2
// inv123 = (P1 * P2)^(-1) mod P3
static const uint32_t inv12  = 657107549U;
static const uint32_t inv123 = 284003040U;

// Base 10^9 limb constant
static const uint64_t BASE_10_9 = 1000000000ULL;
static const uint64_t Q_BASE = 18446744073ULL; // floor(2^64 / 10^9)
static const uint64_t R_BASE = 709551616ULL;   // 2^64 mod 10^9

// ============================================================================
// 2. High-Performance Modular Arithmetic (Scalar + AVX2 SIMD)
// ============================================================================
static inline uint32_t mont_mul(uint32_t a, uint32_t b, uint32_t mod, uint32_t p_inv) {
    uint64_t T = (uint64_t)a * b;
    uint32_t m = (uint32_t)T * p_inv;
    uint64_t t = (T + (uint64_t)m * mod) >> 32;
    return (t >= mod) ? (uint32_t)(t - mod) : (uint32_t)t;
}

static inline uint32_t npow(uint32_t base, uint32_t exp, uint32_t mod) {
    uint64_t res = 1, b = base % mod;
    while (exp > 0) {
        if (exp & 1) res = (res * b) % mod;
        b = (b * b) % mod;
        exp >>= 1;
    }
    return (uint32_t)res;
}

// 8-way AVX2 Montgomery Multiplication for 32-bit primes
static inline __m256i avx2_mont_mul(__m256i a, __m256i b, __m256i v_mod, __m256i v_pinv) {
    __m256i T_even = _mm256_mul_epu32(a, b);
    __m256i m_even = _mm256_mul_epu32(T_even, v_pinv);
    __m256i m_mod_even = _mm256_mul_epu32(m_even, v_mod);
    __m256i sum_even = _mm256_add_epi64(T_even, m_mod_even);
    __m256i t_even = _mm256_srli_epi64(sum_even, 32);

    __m256i a_odd = _mm256_srli_epi64(a, 32);
    __m256i b_odd = _mm256_srli_epi64(b, 32);
    __m256i T_odd = _mm256_mul_epu32(a_odd, b_odd);
    __m256i m_odd = _mm256_mul_epu32(T_odd, v_pinv);
    __m256i m_mod_odd = _mm256_mul_epu32(m_odd, v_mod);
    __m256i sum_odd = _mm256_add_epi64(T_odd, m_mod_odd);

    __m256i res = _mm256_blend_epi32(t_even, sum_odd, 0xAA);
    __m256i sub = _mm256_sub_epi32(res, v_mod);
    return _mm256_min_epu32(res, sub);
}

// 8-way AVX2 Branchless Butterfly (Addition / Subtraction modulo P)
static inline void avx2_butterfly(__m256i u, __m256i v, __m256i v_mod, __m256i& out_u, __m256i& out_v) {
    __m256i sum = _mm256_add_epi32(u, v);
    __m256i sum_sub = _mm256_sub_epi32(sum, v_mod);
    out_u = _mm256_min_epu32(sum, sum_sub);

    __m256i diff = _mm256_sub_epi32(u, v);
    __m256i diff_add = _mm256_add_epi32(diff, v_mod);
    out_v = _mm256_min_epu32(diff, diff_add);
}

// In-register 8x8 32-bit Matrix Transpose Kernel using AVX2 unpack & permute
static inline void transpose8x8_avx2(
    __m256i& r0, __m256i& r1, __m256i& r2, __m256i& r3,
    __m256i& r4, __m256i& r5, __m256i& r6, __m256i& r7)
{
    __m256i t0 = _mm256_unpacklo_epi32(r0, r1);
    __m256i t1 = _mm256_unpackhi_epi32(r0, r1);
    __m256i t2 = _mm256_unpacklo_epi32(r2, r3);
    __m256i t3 = _mm256_unpackhi_epi32(r2, r3);
    __m256i t4 = _mm256_unpacklo_epi32(r4, r5);
    __m256i t5 = _mm256_unpackhi_epi32(r4, r5);
    __m256i t6 = _mm256_unpacklo_epi32(r6, r7);
    __m256i t7 = _mm256_unpackhi_epi32(r6, r7);

    __m256i u0 = _mm256_unpacklo_epi64(t0, t2);
    __m256i u1 = _mm256_unpackhi_epi64(t0, t2);
    __m256i u2 = _mm256_unpacklo_epi64(t1, t3);
    __m256i u3 = _mm256_unpackhi_epi64(t1, t3);
    __m256i u4 = _mm256_unpacklo_epi64(t4, t6);
    __m256i u5 = _mm256_unpackhi_epi64(t4, t6);
    __m256i u6 = _mm256_unpacklo_epi64(t5, t7);
    __m256i u7 = _mm256_unpackhi_epi64(t5, t7);

    r0 = _mm256_permute2x128_si256(u0, u4, 0x20);
    r1 = _mm256_permute2x128_si256(u1, u5, 0x20);
    r2 = _mm256_permute2x128_si256(u2, u6, 0x20);
    r3 = _mm256_permute2x128_si256(u3, u7, 0x20);
    r4 = _mm256_permute2x128_si256(u0, u4, 0x31);
    r5 = _mm256_permute2x128_si256(u1, u5, 0x31);
    r6 = _mm256_permute2x128_si256(u2, u6, 0x31);
    r7 = _mm256_permute2x128_si256(u3, u7, 0x31);
}

// ============================================================================
// 3. Cache-Aware Precomputed Twiddle Tables (with Stage-Vectorized Twiddles)
// ============================================================================
struct AVX2TwTable {
    size_t n = 0;
    std::vector<uint32_t> rt, rti;
    std::vector<uint16_t> rev;
    std::vector<std::vector<uint32_t>> stage_tw, stage_twi;

    void init(size_t N, uint32_t mod, uint32_t g, uint32_t p_inv, uint32_t r2) {
        if (n == N) return;
        n = N;
        rt.resize(N);
        rti.resize(N);
        rev.resize(N);

        int k = 0; size_t m = N; while (m > 1) { m >>= 1; k++; }
        for (size_t i = 0; i < N; i++) {
            size_t r = 0;
            for (int j = 0; j < k; j++) if (i & ((size_t)1 << j)) r |= (size_t)1 << (k - 1 - j);
            rev[i] = (uint16_t)r;
        }

        uint32_t raw_w = npow(g, (mod - 1) / (uint32_t)N, mod);
        uint32_t raw_winv = npow(raw_w, mod - 2, mod);
        uint32_t w = mont_mul(raw_w, r2, mod, p_inv);
        uint32_t winv = mont_mul(raw_winv, r2, mod, p_inv);

        rt[0] = mont_mul(1, r2, mod, p_inv);
        rti[0] = mont_mul(1, r2, mod, p_inv);
        for (size_t i = 1; i < N; i++) {
            rt[i] = mont_mul(rt[i - 1], w, mod, p_inv);
            rti[i] = mont_mul(rti[i - 1], winv, mod, p_inv);
        }

        stage_tw.resize(k + 1);
        stage_twi.resize(k + 1);
        for (size_t len = 16; len <= N; len <<= 1) {
            size_t h = len >> 1;
            size_t s = N / len;
            int stage_idx = 0; size_t temp = len; while (temp > 1) { temp >>= 1; stage_idx++; }
            stage_tw[stage_idx].resize(h);
            stage_twi[stage_idx].resize(h);
            for (size_t j = 0; j < h; j++) {
                stage_tw[stage_idx][j] = rt[j * s];
                stage_twi[stage_idx][j] = rti[j * s];
            }
        }
    }
};

// Global twiddle table manager with automatic caching and thread safety
struct TwiddleManager {
    std::mutex mtx;
    size_t cur_n = 0;
    AVX2TwTable tw_N1[3];
    AVX2TwTable tw_N2[3];
    std::vector<uint32_t> tw_N[3];

    void ensure_twiddles(size_t N, size_t N1, size_t N2) {
        if (cur_n == N) return;
        std::lock_guard<std::mutex> lock(mtx);
        if (cur_n == N) return;
        cur_n = N;

        // Parallelize tw_N1 and tw_N2 initialization
        #pragma omp parallel for schedule(dynamic)
        for (int task = 0; task < 6; task++) {
            int pi = task / 2;
            bool is_n2 = (task % 2 == 1);
            uint32_t mod = P[pi], g = GEN[pi], p_inv = P_INV[pi], r2 = R2_MOD[pi];
            if (is_n2) {
                tw_N2[pi].init(N2, mod, g, p_inv, r2);
            } else {
                tw_N1[pi].init(N1, mod, g, p_inv, r2);
            }
        }

        // Parallelize tw_N computation across primes and 8 chunks (tw_inv_N removed: -k indexing used instead)
        #pragma omp parallel for schedule(dynamic)
        for (int pi = 0; pi < 3; pi++) {
            uint32_t mod = P[pi], g = GEN[pi], p_inv = P_INV[pi], r2 = R2_MOD[pi];
            uint32_t raw_w = npow(g, (mod - 1) / (uint32_t)N, mod);
            uint32_t step_w = mont_mul(raw_w, r2, mod, p_inv);
            uint32_t raw_step = raw_w;

            std::vector<uint32_t>& table = tw_N[pi];
            table.resize(N);

            const size_t CHUNK = N / 8;
            for (int c = 0; c < 8; c++) {
                size_t start = (size_t)c * CHUNK;
                size_t end = (size_t)(c + 1) * CHUNK;
                uint32_t base_raw = npow(raw_step, (uint32_t)start, mod);
                uint32_t cur = mont_mul(base_raw, r2, mod, p_inv);
                table[start] = cur;
                for (size_t i = start + 1; i < end; i++) {
                    cur = mont_mul(cur, step_w, mod, p_inv);
                    table[i] = cur;
                }
            }
        }
    }
};

static TwiddleManager g_twManager;

// ============================================================================
// 4. AVX2 Vectorized In-L1 Cache Cooley-Tukey Row NTT
// ============================================================================
// Rows of length 1024 / 2048 fit directly inside 32KB L1 data cache.
// Stages 1, 2, 3 unrolled; Stages 4..k 100% AVX2 vectorized with streaming twiddles.
// ============================================================================
static inline void avx2_in_l1_row_ntt(uint32_t* a, size_t N, const AVX2TwTable& tw, bool inv, uint32_t mod, uint32_t p_inv) {
    const uint16_t* rev = tw.rev.data();

    // 1. Bit reversal swap
    for (size_t i = 0; i < N; i++) {
        size_t j = rev[i];
        if (i < j) std::swap(a[i], a[j]);
    }

    // 2. Stage 1 (len = 2, h = 1, NO MULTIPLICATION)
    for (size_t i = 0; i < N; i += 2) {
        uint32_t u = a[i], v = a[i + 1];
        a[i]     = (u + v >= mod) ? (u + v - mod) : (u + v);
        a[i + 1] = (u >= v)       ? (u - v)       : (u + mod - v);
    }
    if (N == 2) return;

    // 3. Stage 2 (len = 4, h = 2)
    uint32_t W4 = inv ? tw.rti[N >> 2] : tw.rt[N >> 2];
    for (size_t i = 0; i < N; i += 4) {
        uint32_t u0 = a[i], v0 = a[i + 2];
        a[i]     = (u0 + v0 >= mod) ? (u0 + v0 - mod) : (u0 + v0);
        a[i + 2] = (u0 >= v0)       ? (u0 - v0)       : (u0 + mod - v0);

        uint32_t u1 = a[i + 1], v1 = mont_mul(a[i + 3], W4, mod, p_inv);
        a[i + 1] = (u1 + v1 >= mod) ? (u1 + v1 - mod) : (u1 + v1);
        a[i + 3] = (u1 >= v1)       ? (u1 - v1)       : (u1 + mod - v1);
    }
    if (N == 4) return;

    // 4. Stage 3 (len = 8, h = 4)
    const uint32_t* rt = inv ? tw.rti.data() : tw.rt.data();
    size_t s8 = N >> 3;
    uint32_t w1 = rt[s8], w2 = rt[s8 * 2], w3 = rt[s8 * 3];
    for (size_t i = 0; i < N; i += 8) {
        uint32_t u0 = a[i],     v0 = a[i + 4];
        uint32_t u1 = a[i + 1], v1 = mont_mul(a[i + 5], w1, mod, p_inv);
        uint32_t u2 = a[i + 2], v2 = mont_mul(a[i + 6], w2, mod, p_inv);
        uint32_t u3 = a[i + 3], v3 = mont_mul(a[i + 7], w3, mod, p_inv);

        a[i]     = (u0 + v0 >= mod) ? (u0 + v0 - mod) : (u0 + v0);
        a[i + 4] = (u0 >= v0)       ? (u0 - v0)       : (u0 + mod - v0);
        a[i + 1] = (u1 + v1 >= mod) ? (u1 + v1 - mod) : (u1 + v1);
        a[i + 5] = (u1 >= v1)       ? (u1 - v1)       : (u1 + mod - v1);
        a[i + 2] = (u2 + v2 >= mod) ? (u2 + v2 - mod) : (u2 + v2);
        a[i + 6] = (u2 >= v2)       ? (u2 - v2)       : (u2 + mod - v2);
        a[i + 3] = (u3 + v3 >= mod) ? (u3 + v3 - mod) : (u3 + v3);
        a[i + 7] = (u3 >= v3)       ? (u3 - v3)       : (u3 + mod - v3);
    }
    if (N == 8) return;

    // 5. Stages 4+ (len = 16, 32, ..., N) -> 100% AVX2 VECTORIZED
    __m256i v_mod = _mm256_set1_epi32(mod);
    __m256i v_pinv = _mm256_set1_epi32(p_inv);

    int stage_idx = 4;
    for (size_t len = 16; len <= N; len <<= 1, stage_idx++) {
        size_t h = len >> 1;
        const uint32_t* tw_stage = inv ? tw.stage_twi[stage_idx].data() : tw.stage_tw[stage_idx].data();

        for (size_t i = 0; i < N; i += len) {
            uint32_t* p0 = a + i;
            uint32_t* p1 = a + i + h;

            for (size_t j = 0; j < h; j += 8) {
                __m256i vu = _mm256_loadu_si256((const __m256i*)(p0 + j));
                __m256i vp1 = _mm256_loadu_si256((const __m256i*)(p1 + j));
                __m256i vw = _mm256_loadu_si256((const __m256i*)(tw_stage + j));

                __m256i vv = avx2_mont_mul(vp1, vw, v_mod, v_pinv);
                __m256i out_u, out_v;
                avx2_butterfly(vu, vv, v_mod, out_u, out_v);

                _mm256_storeu_si256((__m256i*)(p0 + j), out_u);
                _mm256_storeu_si256((__m256i*)(p1 + j), out_v);
            }
        }
    }
}

// 1D AVX2 NTT for smaller transforms (N <= 16384)
static void ntt_1d_direct(uint32_t* a, size_t n, bool inv, uint32_t mod, uint32_t g, uint32_t p_inv, uint32_t r2) {
    AVX2TwTable tw;
    tw.init(n, mod, g, p_inv, r2);
    avx2_in_l1_row_ntt(a, n, tw, inv, mod, p_inv);
    if (inv) {
        uint32_t inv_n = npow(n, mod - 2, mod);
        __m256i v_mod = _mm256_set1_epi32(mod);
        __m256i v_pinv = _mm256_set1_epi32(p_inv);
        __m256i v_inv_n = _mm256_set1_epi32(inv_n);
        for (size_t i = 0; i < n; i += 8) {
            __m256i v = _mm256_loadu_si256((const __m256i*)(a + i));
            v = avx2_mont_mul(v, v_inv_n, v_mod, v_pinv);
            _mm256_storeu_si256((__m256i*)(a + i), v);
        }
    }
}

// ============================================================================
// 5. AVX2 Fused Input Reduction + Montgomery Conversion + Blocked Transpose
// ============================================================================
static void fused_input_transpose_mont_avx2(uint32_t* dst, const uint32_t* src, size_t R, size_t C, uint32_t mod, uint32_t p_inv, uint32_t r2) {
    __m256i v_mod = _mm256_set1_epi32(mod);
    __m256i v_pinv = _mm256_set1_epi32(p_inv);
    __m256i v_r2 = _mm256_set1_epi32(r2);

    #pragma omp parallel for schedule(static)
    for (int bi = 0; bi < (int)R; bi += 8) {
        for (size_t bj = 0; bj < C; bj += 8) {
            __m256i r0 = _mm256_loadu_si256((const __m256i*)(src + (bi + 0) * C + bj));
            __m256i r1 = _mm256_loadu_si256((const __m256i*)(src + (bi + 1) * C + bj));
            __m256i r2_v = _mm256_loadu_si256((const __m256i*)(src + (bi + 2) * C + bj));
            __m256i r3 = _mm256_loadu_si256((const __m256i*)(src + (bi + 3) * C + bj));
            __m256i r4 = _mm256_loadu_si256((const __m256i*)(src + (bi + 4) * C + bj));
            __m256i r5 = _mm256_loadu_si256((const __m256i*)(src + (bi + 5) * C + bj));
            __m256i r6 = _mm256_loadu_si256((const __m256i*)(src + (bi + 6) * C + bj));
            __m256i r7 = _mm256_loadu_si256((const __m256i*)(src + (bi + 7) * C + bj));

            #define REDUCE_ROW(r) r = _mm256_min_epu32(r, _mm256_sub_epi32(r, v_mod)); \
                                  r = avx2_mont_mul(r, v_r2, v_mod, v_pinv)
            REDUCE_ROW(r0); REDUCE_ROW(r1); REDUCE_ROW(r2_v); REDUCE_ROW(r3);
            REDUCE_ROW(r4); REDUCE_ROW(r5); REDUCE_ROW(r6); REDUCE_ROW(r7);
            #undef REDUCE_ROW

            transpose8x8_avx2(r0, r1, r2_v, r3, r4, r5, r6, r7);

            _mm256_storeu_si256((__m256i*)(dst + (bj + 0) * R + bi), r0);
            _mm256_storeu_si256((__m256i*)(dst + (bj + 1) * R + bi), r1);
            _mm256_storeu_si256((__m256i*)(dst + (bj + 2) * R + bi), r2_v);
            _mm256_storeu_si256((__m256i*)(dst + (bj + 3) * R + bi), r3);
            _mm256_storeu_si256((__m256i*)(dst + (bj + 4) * R + bi), r4);
            _mm256_storeu_si256((__m256i*)(dst + (bj + 5) * R + bi), r5);
            _mm256_storeu_si256((__m256i*)(dst + (bj + 6) * R + bi), r6);
            _mm256_storeu_si256((__m256i*)(dst + (bj + 7) * R + bi), r7);
        }
    }
}

// ============================================================================
// 6. Tzd-Aether 4-Step AVX2 Transform Pipeline
// ============================================================================
// Factors n = N1 * N2.
// Forward:
//   Pass 1: AVX2 Fused Load + Montgomery Conversion + Transpose N1 x N2 -> N2 x N1
//   Pass 2: AVX2 In-L1 Row NTT of length N1 on all N2 rows
//   Pass 3: Blocked Transpose N2 x N1 -> N1 x N2 with fused twiddle w^(r*c)
//   Pass 4: AVX2 In-L1 Row NTT of length N2 on all N1 rows
// Inverse:
//   Pass 1: AVX2 In-L1 Row iNTT of length N2 on all N1 rows
//   Pass 2: Blocked Transpose N1 x N2 -> N2 x N1 with fused inverse twiddle winv^(r*c)
//   Pass 3: AVX2 In-L1 Row iNTT of length N1 on all N2 rows
//   Pass 4: AVX2 8x8 Transpose N2 x N1 -> N1 x N2 with fused n^(-1) scale directly to final_out
// ============================================================================

// ULTRA-FAST AVX2 Forward Pass 3: 8x8 Tiled Transpose with Hoisted Register Twiddles
static void transpose_twiddle_forward_avx2(int pi, uint32_t* out, const uint32_t* sc, size_t N1, size_t N2, size_t n) {
    uint32_t mod = P[pi], p_inv = P_INV[pi];
    __m256i v_mod = _mm256_set1_epi32(mod);
    __m256i v_pinv = _mm256_set1_epi32(p_inv);
    uint32_t mask = (uint32_t)(n - 1);
    const uint32_t* tw_table = g_twManager.tw_N[pi].data();

    #pragma omp parallel for schedule(static)
    for (int bj = 0; bj < (int)N1; bj += 8) {
        __m256i v_col_tw[8];
        for (int k = 0; k < 8; k++) {
            size_t c = (size_t)bj + k;
            v_col_tw[k] = _mm256_set_epi32(
                tw_table[(7 * c) & mask],
                tw_table[(6 * c) & mask],
                tw_table[(5 * c) & mask],
                tw_table[(4 * c) & mask],
                tw_table[(3 * c) & mask],
                tw_table[(2 * c) & mask],
                tw_table[(1 * c) & mask],
                tw_table[0]
            );
        }

        uint64_t idx0[8];
        for (int k = 0; k < 8; k++) idx0[k] = 0;

        for (size_t bi = 0; bi < N2; bi += 8) {
            __m256i r0 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 0) * N1 + bj));
            __m256i r1 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 1) * N1 + bj));
            __m256i r2_v = _mm256_loadu_si256((const __m256i*)(sc + (bi + 2) * N1 + bj));
            __m256i r3 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 3) * N1 + bj));
            __m256i r4 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 4) * N1 + bj));
            __m256i r5 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 5) * N1 + bj));
            __m256i r6 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 6) * N1 + bj));
            __m256i r7 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 7) * N1 + bj));

            transpose8x8_avx2(r0, r1, r2_v, r3, r4, r5, r6, r7);

            #define APPLY_TW(r_reg, k) { \
                size_t c = (size_t)bj + k; \
                uint32_t base_tw = tw_table[idx0[k] & mask]; \
                __m256i v_base = _mm256_set1_epi32(base_tw); \
                __m256i v_tw = avx2_mont_mul(v_base, v_col_tw[k], v_mod, v_pinv); \
                r_reg = avx2_mont_mul(r_reg, v_tw, v_mod, v_pinv); \
                _mm256_storeu_si256((__m256i*)(out + c * N2 + bi), r_reg); \
                idx0[k] += 8 * c; \
            }
            APPLY_TW(r0, 0); APPLY_TW(r1, 1); APPLY_TW(r2_v, 2); APPLY_TW(r3, 3);
            APPLY_TW(r4, 4); APPLY_TW(r5, 5); APPLY_TW(r6, 6); APPLY_TW(r7, 7);
            #undef APPLY_TW
        }
    }
}

// ULTRA-FAST AVX2 Inverse Pass 2: 8x8 Tiled Transpose with Hoisted Inverse Register Twiddles
static void transpose_twiddle_inverse_avx2(int pi, uint32_t* sc, const uint32_t* a, size_t N1, size_t N2, size_t n) {
    uint32_t mod = P[pi], p_inv = P_INV[pi];
    __m256i v_mod = _mm256_set1_epi32(mod);
    __m256i v_pinv = _mm256_set1_epi32(p_inv);
    uint32_t mask = (uint32_t)(n - 1);
    const uint32_t* tw_table = g_twManager.tw_N[pi].data();

    #pragma omp parallel for schedule(static)
    for (int bj = 0; bj < (int)N2; bj += 8) {
        __m256i v_col_tw[8];
        for (int k = 0; k < 8; k++) {
            size_t c = (size_t)bj + k;
            v_col_tw[k] = _mm256_set_epi32(
                tw_table[(0ULL - 7 * c) & mask],
                tw_table[(0ULL - 6 * c) & mask],
                tw_table[(0ULL - 5 * c) & mask],
                tw_table[(0ULL - 4 * c) & mask],
                tw_table[(0ULL - 3 * c) & mask],
                tw_table[(0ULL - 2 * c) & mask],
                tw_table[(0ULL - 1 * c) & mask],
                tw_table[0]
            );
        }

        uint64_t idx0[8];
        for (int k = 0; k < 8; k++) idx0[k] = 0;

        for (size_t bi = 0; bi < N1; bi += 8) {
            __m256i r0 = _mm256_loadu_si256((const __m256i*)(a + (bi + 0) * N2 + bj));
            __m256i r1 = _mm256_loadu_si256((const __m256i*)(a + (bi + 1) * N2 + bj));
            __m256i r2_v = _mm256_loadu_si256((const __m256i*)(a + (bi + 2) * N2 + bj));
            __m256i r3 = _mm256_loadu_si256((const __m256i*)(a + (bi + 3) * N2 + bj));
            __m256i r4 = _mm256_loadu_si256((const __m256i*)(a + (bi + 4) * N2 + bj));
            __m256i r5 = _mm256_loadu_si256((const __m256i*)(a + (bi + 5) * N2 + bj));
            __m256i r6 = _mm256_loadu_si256((const __m256i*)(a + (bi + 6) * N2 + bj));
            __m256i r7 = _mm256_loadu_si256((const __m256i*)(a + (bi + 7) * N2 + bj));

            transpose8x8_avx2(r0, r1, r2_v, r3, r4, r5, r6, r7);

            #define APPLY_TW_INV(r_reg, k) { \
                size_t c = (size_t)bj + k; \
                uint32_t base_tw = tw_table[(0ULL - idx0[k]) & mask]; \
                __m256i v_base = _mm256_set1_epi32(base_tw); \
                __m256i v_tw = avx2_mont_mul(v_base, v_col_tw[k], v_mod, v_pinv); \
                r_reg = avx2_mont_mul(r_reg, v_tw, v_mod, v_pinv); \
                _mm256_storeu_si256((__m256i*)(sc + c * N1 + bi), r_reg); \
                idx0[k] += 8 * c; \
            }
            APPLY_TW_INV(r0, 0); APPLY_TW_INV(r1, 1); APPLY_TW_INV(r2_v, 2); APPLY_TW_INV(r3, 3);
            APPLY_TW_INV(r4, 4); APPLY_TW_INV(r5, 5); APPLY_TW_INV(r6, 6); APPLY_TW_INV(r7, 7);
            #undef APPLY_TW_INV
        }
    }
}

static void ntt_4step_forward_avx2(int pi, uint32_t* out, uint32_t* sc, const uint32_t* in_raw, size_t N1, size_t N2, size_t n) {
    uint32_t mod = P[pi], p_inv = P_INV[pi], r2 = R2_MOD[pi];

    // Pass 1: AVX2 Fused load + mont + transpose N1 x N2 -> N2 x N1 into sc
    fused_input_transpose_mont_avx2(sc, in_raw, N1, N2, mod, p_inv, r2);

    // Pass 2: AVX2 Row NTT of length N1 on all N2 rows of sc
    #pragma omp parallel for schedule(static)
    for (int r = 0; r < (int)N2; r++) {
        avx2_in_l1_row_ntt(sc + r * N1, N1, g_twManager.tw_N1[pi], false, mod, p_inv);
    }

    // Pass 3: AVX2 8x8 Tiled Transpose with Fused Twiddles into out
    transpose_twiddle_forward_avx2(pi, out, sc, N1, N2, n);

    // Pass 4: AVX2 Row NTT of length N2 on all N1 rows of out
    #pragma omp parallel for schedule(static)
    for (int r = 0; r < (int)N1; r++) {
        avx2_in_l1_row_ntt(out + r * N2, N2, g_twManager.tw_N2[pi], false, mod, p_inv);
    }
}

static void ntt_4step_inverse_avx2(int pi, uint32_t* final_out, uint32_t* a, uint32_t* sc, size_t N1, size_t N2, size_t n) {
    uint32_t mod = P[pi], p_inv = P_INV[pi];

    // Pass 1: AVX2 Row iNTT of length N2 on all N1 rows of a
    #pragma omp parallel for schedule(static)
    for (int r = 0; r < (int)N1; r++) {
        avx2_in_l1_row_ntt(a + r * N2, N2, g_twManager.tw_N2[pi], true, mod, p_inv);
    }

    // Pass 2: AVX2 8x8 Tiled Transpose with Fused Inverse Twiddles into sc
    transpose_twiddle_inverse_avx2(pi, sc, a, N1, N2, n);

    // Pass 3: AVX2 Row iNTT of length N1 on all N2 rows of sc
    #pragma omp parallel for schedule(static)
    for (int r = 0; r < (int)N2; r++) {
        avx2_in_l1_row_ntt(sc + r * N1, N1, g_twManager.tw_N1[pi], true, mod, p_inv);
    }

    // Pass 4: AVX2 8x8 Transpose N2 x N1 -> N1 x N2 with scale by n^(-1) directly to final_out
    uint32_t inv_n = npow(n, mod - 2, mod);
    __m256i v_mod = _mm256_set1_epi32(mod);
    __m256i v_pinv = _mm256_set1_epi32(p_inv);
    __m256i v_inv_n = _mm256_set1_epi32(inv_n);

    #pragma omp parallel for schedule(static)
    for (int bi = 0; bi < (int)N2; bi += 8) {
        for (size_t bj = 0; bj < N1; bj += 8) {
            __m256i r0 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 0) * N1 + bj));
            __m256i r1 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 1) * N1 + bj));
            __m256i r2_v = _mm256_loadu_si256((const __m256i*)(sc + (bi + 2) * N1 + bj));
            __m256i r3 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 3) * N1 + bj));
            __m256i r4 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 4) * N1 + bj));
            __m256i r5 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 5) * N1 + bj));
            __m256i r6 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 6) * N1 + bj));
            __m256i r7 = _mm256_loadu_si256((const __m256i*)(sc + (bi + 7) * N1 + bj));

            transpose8x8_avx2(r0, r1, r2_v, r3, r4, r5, r6, r7);

            r0 = avx2_mont_mul(r0, v_inv_n, v_mod, v_pinv);
            r1 = avx2_mont_mul(r1, v_inv_n, v_mod, v_pinv);
            r2_v = avx2_mont_mul(r2_v, v_inv_n, v_mod, v_pinv);
            r3 = avx2_mont_mul(r3, v_inv_n, v_mod, v_pinv);
            r4 = avx2_mont_mul(r4, v_inv_n, v_mod, v_pinv);
            r5 = avx2_mont_mul(r5, v_inv_n, v_mod, v_pinv);
            r6 = avx2_mont_mul(r6, v_inv_n, v_mod, v_pinv);
            r7 = avx2_mont_mul(r7, v_inv_n, v_mod, v_pinv);

            _mm256_storeu_si256((__m256i*)(final_out + (bj + 0) * N2 + bi), r0);
            _mm256_storeu_si256((__m256i*)(final_out + (bj + 1) * N2 + bi), r1);
            _mm256_storeu_si256((__m256i*)(final_out + (bj + 2) * N2 + bi), r2_v);
            _mm256_storeu_si256((__m256i*)(final_out + (bj + 3) * N2 + bi), r3);
            _mm256_storeu_si256((__m256i*)(final_out + (bj + 4) * N2 + bi), r4);
            _mm256_storeu_si256((__m256i*)(final_out + (bj + 5) * N2 + bi), r5);
            _mm256_storeu_si256((__m256i*)(final_out + (bj + 6) * N2 + bi), r6);
            _mm256_storeu_si256((__m256i*)(final_out + (bj + 7) * N2 + bi), r7);
        }
    }
}

// ============================================================================
// 7. Parallel SWAR String Parser and Formatter (Base 10^9)
// ============================================================================
static inline void parse_str_to_limbs(const std::string& s, uint32_t* limbs, int limb_count) {
    #pragma omp parallel for schedule(static) if(limb_count > 1000)
    for (int k = 0; k < limb_count; k++) {
        int end_pos = (int)s.size() - k * 9;
        int start_pos = end_pos - 9;
        uint32_t v = 0;
        if (start_pos >= 0) {
            const char* p = s.data() + start_pos;
            for (int i = 0; i < 9; i++) v = v * 10 + (uint32_t)(p[i] - '0');
        } else {
            for (int i = 0; i < end_pos; i++) v = v * 10 + (uint32_t)(s[i] - '0');
        }
        limbs[k] = v;
    }
}

static inline std::string format_limbs_to_str(const uint32_t* result, int pos) {
    while (pos > 0 && result[pos - 1] == 0) pos--;
    if (pos == 0) return "0";

    uint32_t top = result[pos - 1];
    char tmp[16];
    int tl = 0;
    uint32_t t = top;
    while (t > 0) {
        tmp[tl++] = '0' + (int)(t % 10);
        t /= 10;
    }
    std::reverse(tmp, tmp + tl);

    size_t total_len = (size_t)tl + (size_t)(pos - 1) * 9;
    std::string res_str;
    res_str.resize(total_len);
    memcpy(&res_str[0], tmp, tl);

    #pragma omp parallel for schedule(static) if(pos > 1000)
    for (int k = 0; k < pos - 1; k++) {
        char* p = &res_str[0] + tl + (size_t)(pos - 2 - k) * 9;
        uint32_t v = result[k];
        for (int i = 8; i >= 0; i--) {
            p[i] = '0' + (int)(v % 10);
            v /= 10;
        }
    }

    return res_str;
}

// ============================================================================
// 8. 128-bit SIMD Chinese Remainder Theorem (CRT) & Carry Engine
// ============================================================================
static void crt_reconstruct_and_propagate(
    uint32_t* result_limbs, int& out_pos,
    const uint32_t* r0, const uint32_t* r1, const uint32_t* r2,
    size_t n, int active_len, uint64_t* partial_q)
{
    uint64_t P1P2 = (uint64_t)P[0] * P[1];
    int limit = (active_len < (int)n) ? active_len : (int)n;

    #pragma omp parallel for schedule(static) if(limit > 1000)
    for (int i = 0; i < limit; i++) {
        uint32_t v0 = r0[i];
        uint32_t v1 = r1[i];
        uint32_t v2 = r2[i];

        // x12 = v0 + P1 * ((v1 - v0) * inv12 mod P2)
        uint64_t t1 = (v1 >= v0) ? (v1 - v0) : (v1 + P[1] - v0);
        t1 = (t1 * inv12) % P[1];
        uint64_t x12 = v0 + (uint64_t)P[0] * t1;

        // t2 = (v2 - (x12 mod P3)) * inv123 mod P3
        uint64_t x12_mod_p3 = x12 % P[2];
        uint64_t t2 = (v2 >= x12_mod_p3) ? (v2 - x12_mod_p3) : (v2 + P[2] - x12_mod_p3);
        t2 = (t2 * inv123) % P[2];

        // Full 128-bit value = x12 + P1 * P2 * t2
        uint64_t prod_hi, prod_lo;
        prod_lo = _umul128(P1P2, t2, &prod_hi);
        uint64_t low = x12 + prod_lo;
        if (low < x12) prod_hi++;

        // Fast 64-bit reciprocal division by 10^9 without hardware _udiv128
        uint64_t q_high = prod_hi * Q_BASE;
        uint64_t rem_high = prod_hi * R_BASE;
        uint64_t sum_low = rem_high + low;
        uint64_t carry = (sum_low < low) ? 1 : 0;
        q_high += carry * Q_BASE;
        uint64_t sum2 = sum_low + carry * R_BASE;
        uint64_t q_low = sum2 / BASE_10_9;
        uint64_t rem = sum2 - q_low * BASE_10_9;
        uint64_t q = q_high + q_low;

        result_limbs[i] = (uint32_t)rem;
        partial_q[i] = q;
    }

    // High-speed carry propagation
    uint64_t carry = 0;
    int rl = limit;

    for (int i = 0; i < rl; i++) {
        uint64_t v = (uint64_t)result_limbs[i] + carry;
        result_limbs[i] = (uint32_t)(v % BASE_10_9);
        carry = partial_q[i] + (v / BASE_10_9);
    }

    int pos = rl;
    while (carry > 0) {
        uint64_t v = result_limbs[pos] + carry;
        result_limbs[pos] = (uint32_t)(v % BASE_10_9);
        carry = v / BASE_10_9;
        pos++;
    }

    while (pos > 0 && result_limbs[pos - 1] == 0) pos--;
    out_pos = pos;
}

// Fast zero-allocation recursive Karatsuba multiplication in base 10^9
static void karatsuba_mul_rec(uint64_t* res, const uint64_t* a, size_t na, const uint64_t* b, size_t nb, uint64_t* ws) {
    if (na < nb) { karatsuba_mul_rec(res, b, nb, a, na, ws); return; }
    if (nb <= 32) {
        std::memset(res, 0, (na + nb + 2) * sizeof(uint64_t));
        for (size_t i = 0; i < na; i++) {
            uint64_t ai = a[i];
            if (ai == 0) continue;
            uint64_t carry = 0;
            for (size_t j = 0; j < nb; j++) {
                uint64_t cur = res[i + j] + ai * b[j] + carry;
                res[i + j] = cur % BASE_10_9;
                carry = cur / BASE_10_9;
            }
            res[i + nb] += carry;
        }
        return;
    }

    // Unbalanced operands: split longer operand in half
    if (na > 2 * nb) {
        size_t k = na / 2;
        uint64_t* r0 = ws;
        uint64_t* r1 = r0 + (k + nb + 4);
        uint64_t* next_ws = r1 + (na - k + nb + 4);

        karatsuba_mul_rec(r0, a, k, b, nb, next_ws);
        karatsuba_mul_rec(r1, a + k, na - k, b, nb, next_ws);

        std::memset(res, 0, (na + nb + 4) * sizeof(uint64_t));
        for (size_t i = 0; i < k + nb; i++) res[i] = r0[i];

        uint64_t c = 0;
        for (size_t i = 0; i < (na - k + nb) || c; i++) {
            uint64_t v = res[i + k] + (i < (na - k + nb) ? r1[i] : 0) + c;
            res[i + k] = v % BASE_10_9;
            c = v / BASE_10_9;
        }
        return;
    }

    size_t k = (na + 1) / 2;
    size_t a0_len = (k < na) ? k : na;
    size_t a1_len = (na > k) ? (na - k) : 0;
    size_t b0_len = (k < nb) ? k : nb;
    size_t b1_len = (nb > k) ? (nb - k) : 0;

    const uint64_t* a0 = a;
    const uint64_t* a1 = a + k;
    const uint64_t* b0 = b;
    const uint64_t* b1 = b + k;

    uint64_t* z0 = ws;
    uint64_t* z2 = z0 + 2 * k + 4;
    uint64_t* sa = z2 + 2 * k + 4;
    uint64_t* sb = sa + k + 4;
    uint64_t* z1 = sb + k + 4;
    uint64_t* next_ws = z1 + 2 * k + 8;

    karatsuba_mul_rec(z0, a0, a0_len, b0, b0_len, next_ws);

    if (a1_len > 0 && b1_len > 0) {
        karatsuba_mul_rec(z2, a1, a1_len, b1, b1_len, next_ws);
    } else {
        std::memset(z2, 0, (a1_len + b1_len + 4) * sizeof(uint64_t));
    }

    size_t sa_len = (a0_len > a1_len ? a0_len : a1_len);
    uint64_t c = 0;
    for (size_t i = 0; i < sa_len; i++) {
        uint64_t v = (i < a0_len ? a0[i] : 0) + (i < a1_len ? a1[i] : 0) + c;
        sa[i] = v % BASE_10_9;
        c = v / BASE_10_9;
    }
    if (c) { sa[sa_len++] = c; }

    size_t sb_len = (b0_len > b1_len ? b0_len : b1_len);
    c = 0;
    for (size_t i = 0; i < sb_len; i++) {
        uint64_t v = (i < b0_len ? b0[i] : 0) + (i < b1_len ? b1[i] : 0) + c;
        sb[i] = v % BASE_10_9;
        c = v / BASE_10_9;
    }
    if (c) { sb[sb_len++] = c; }

    karatsuba_mul_rec(z1, sa, sa_len, sb, sb_len, next_ws);

    size_t z0_len = a0_len + b0_len;
    size_t z2_len = a1_len + b1_len;
    size_t z1_len = sa_len + sb_len;

    int64_t borrow = 0;
    for (size_t i = 0; i < z1_len; i++) {
        int64_t sub = (i < z0_len ? (int64_t)z0[i] : 0) + borrow;
        int64_t cur = (int64_t)z1[i] - sub;
        if (cur < 0) { cur += BASE_10_9; borrow = 1; } else { borrow = 0; }
        z1[i] = (uint64_t)cur;
    }

    borrow = 0;
    for (size_t i = 0; i < z1_len; i++) {
        int64_t sub = (i < z2_len ? (int64_t)z2[i] : 0) + borrow;
        int64_t cur = (int64_t)z1[i] - sub;
        if (cur < 0) { cur += BASE_10_9; borrow = 1; } else { borrow = 0; }
        z1[i] = (uint64_t)cur;
    }

    std::memset(res, 0, (na + nb + 2) * sizeof(uint64_t));
    for (size_t i = 0; i < z0_len; i++) res[i] = z0[i];

    c = 0;
    for (size_t i = 0; i < z1_len || c; i++) {
        uint64_t v = res[i + k] + (i < z1_len ? z1[i] : 0) + c;
        res[i + k] = v % BASE_10_9;
        c = v / BASE_10_9;
    }

    c = 0;
    for (size_t i = 0; i < z2_len || c; i++) {
        uint64_t v = res[i + 2 * k] + (i < z2_len ? z2[i] : 0) + c;
        res[i + 2 * k] = v % BASE_10_9;
        c = v / BASE_10_9;
    }
}

// Schoolbook multiplication for small numbers
static std::vector<uint64_t> schoolbook_mul(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.empty() || b.empty()) return {};
    std::vector<uint64_t> r(a.size() + b.size() + 2, 0);
    for (size_t i = 0; i < a.size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b.size(); j++) {
            uint64_t cur = r[i + j] + a[i] * b[j] + carry;
            r[i + j] = cur % BASE_10_9;
            carry = cur / BASE_10_9;
        }
        r[i + b.size()] += carry;
    }
    while (!r.empty() && r.back() == 0) r.pop_back();
    return r;
}

} // anonymous namespace

// ============================================================================
// 9. Public Entry Points
// ============================================================================

std::vector<uint64_t> bigint_mul_experimental_cpu_limbs(const std::vector<uint64_t>& la, const std::vector<uint64_t>& lb) {
    if (la.empty() || lb.empty()) return {};

    size_t tot = la.size() + lb.size();
    if (tot <= 64) {
        return schoolbook_mul(la, lb);
    }
    if (tot <= 1024) {
        std::vector<uint64_t> r(tot + 4, 0);
        std::vector<uint64_t> ws(tot * 16 + 1024, 0);
        karatsuba_mul_rec(r.data(), la.data(), la.size(), lb.data(), lb.size(), ws.data());
        while (!r.empty() && r.back() == 0) r.pop_back();
        return r;
    }

    size_t n = 1;
    while (n < tot) n <<= 1;

    int na = (int)la.size();
    int nb = (int)lb.size();

    std::vector<uint32_t> in_a(n, 0), in_b(n, 0);
    #pragma omp parallel for schedule(static) if(na > 1000)
    for (int i = 0; i < na; i++) in_a[i] = (uint32_t)la[i];
    #pragma omp parallel for schedule(static) if(nb > 1000)
    for (int i = 0; i < nb; i++) in_b[i] = (uint32_t)lb[i];

    std::vector<uint32_t> r_ntt[3];
    for (int pi = 0; pi < 3; pi++) r_ntt[pi].resize(n);

    std::vector<uint32_t> fa(n), fb(n), sc(2 * n);

    int k = 0; size_t m = n; while (m > 1) { m >>= 1; k++; }
    int k1 = (k + 1) / 2;
    int k2 = k - k1;
    size_t N1 = (size_t)1 << k1;
    size_t N2 = (size_t)1 << k2;

    g_twManager.ensure_twiddles(n, N1, N2);

    for (int pi = 0; pi < 3; pi++) {
        uint32_t mod = P[pi], p_inv = P_INV[pi], r2 = R2_MOD[pi];
        __m256i v_mod = _mm256_set1_epi32(mod);
        __m256i v_pinv = _mm256_set1_epi32(p_inv);

        // Pass 1: AVX2 Fused load + mont + transpose N1 x N2 -> N2 x N1
        fused_input_transpose_mont_avx2(sc.data(), in_a.data(), N1, N2, mod, p_inv, r2);
        fused_input_transpose_mont_avx2(fa.data(), in_b.data(), N1, N2, mod, p_inv, r2);

        // Pass 2: AVX2 Row NTT of length N1 on all N2 rows
        #pragma omp parallel for schedule(static)
        for (int r = 0; r < (int)N2; r++) {
            avx2_in_l1_row_ntt(sc.data() + r * N1, N1, g_twManager.tw_N1[pi], false, mod, p_inv);
            avx2_in_l1_row_ntt(fa.data() + r * N1, N1, g_twManager.tw_N1[pi], false, mod, p_inv);
        }

        // Pass 3: AVX2 8x8 Tiled Transpose with Fused Twiddles
        transpose_twiddle_forward_avx2(pi, fb.data(), sc.data(), N1, N2, n);
        transpose_twiddle_forward_avx2(pi, sc.data(), fa.data(), N1, N2, n);

        // Pass 4: AVX2 Row NTT of length N2 on all N1 rows
        #pragma omp parallel for schedule(static)
        for (int r = 0; r < (int)N1; r++) {
            avx2_in_l1_row_ntt(fb.data() + r * N2, N2, g_twManager.tw_N2[pi], false, mod, p_inv);
            avx2_in_l1_row_ntt(sc.data() + r * N2, N2, g_twManager.tw_N2[pi], false, mod, p_inv);
        }

        // Pointwise Montgomery multiplication
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < (int)n; i += 8) {
            __m256i va = _mm256_loadu_si256((const __m256i*)(fb.data() + i));
            __m256i vb = _mm256_loadu_si256((const __m256i*)(sc.data() + i));
            __m256i vres = avx2_mont_mul(va, vb, v_mod, v_pinv);
            _mm256_storeu_si256((__m256i*)(fb.data() + i), vres);
        }

        // Inverse 4-step transform
        ntt_4step_inverse_avx2(pi, r_ntt[pi].data(), fb.data(), sc.data(), N1, N2, n);
    }

    uint32_t* result_limbs = in_a.data();
    int pos = 0;
    crt_reconstruct_and_propagate(result_limbs, pos, r_ntt[0].data(), r_ntt[1].data(), r_ntt[2].data(), n, na + nb + 2, (uint64_t*)sc.data());

    std::vector<uint64_t> out(pos);
    #pragma omp parallel for schedule(static) if(pos > 1000)
    for (int i = 0; i < pos; i++) out[i] = result_limbs[i];
    return out;
}

std::string bigint_mul_experimental_cpu_str(const std::string& a, const std::string& b) {
    if (a == "0" || b == "0" || a.empty() || b.empty()) return "0";

    bool neg_a = (a[0] == '-');
    bool neg_b = (b[0] == '-');
    std::string s_a = neg_a ? a.substr(1) : a;
    std::string s_b = neg_b ? b.substr(1) : b;

    size_t a_digits = s_a.size();
    size_t b_digits = s_b.size();
    size_t maxDigits = a_digits > b_digits ? a_digits : b_digits;

    extern bool g_bigTime;
    auto t_start = std::chrono::steady_clock::now();

    // Small numbers fast path (<= 256 digits)
    if (maxDigits <= 256) {
        auto la = limbs_from_str(s_a);
        auto lb = limbs_from_str(s_b);
        auto lr = schoolbook_mul(la, lb);
        std::string r = limbs_to_str(lr);
        if (neg_a != neg_b && r != "0") r = "-" + r;
        return r;
    }

    // Medium numbers Karatsuba fast path (<= 9216 digits / 1024 limbs)
    if (maxDigits <= 9216) {
        int na = (int)((a_digits + 8) / 9);
        int nb = (int)((b_digits + 8) / 9);
        std::vector<uint64_t> la(na), lb(nb);
        for (int k = 0; k < na; k++) {
            int end_pos = (int)s_a.size() - k * 9;
            int start_pos = end_pos - 9;
            uint64_t v = 0;
            if (start_pos >= 0) {
                const char* p = s_a.data() + start_pos;
                for (int i = 0; i < 9; i++) v = v * 10 + (uint64_t)(p[i] - '0');
            } else {
                for (int i = 0; i < end_pos; i++) v = v * 10 + (uint64_t)(s_a[i] - '0');
            }
            la[k] = v;
        }
        for (int k = 0; k < nb; k++) {
            int end_pos = (int)s_b.size() - k * 9;
            int start_pos = end_pos - 9;
            uint64_t v = 0;
            if (start_pos >= 0) {
                const char* p = s_b.data() + start_pos;
                for (int i = 0; i < 9; i++) v = v * 10 + (uint64_t)(p[i] - '0');
            } else {
                for (int i = 0; i < end_pos; i++) v = v * 10 + (uint64_t)(s_b[i] - '0');
            }
            lb[k] = v;
        }

        std::vector<uint64_t> lr(na + nb + 4, 0);
        std::vector<uint64_t> ws((na + nb) * 16 + 1024, 0);
        karatsuba_mul_rec(lr.data(), la.data(), na, lb.data(), nb, ws.data());

        int pos = na + nb + 2;
        while (pos > 0 && lr[pos - 1] == 0) pos--;
        std::vector<uint32_t> r32(pos);
        for (int i = 0; i < pos; i++) r32[i] = (uint32_t)lr[i];
        std::string r = format_limbs_to_str(r32.data(), pos);
        if (neg_a != neg_b && r != "0") r = "-" + r;

        if (g_bigTime) {
            auto t_end = std::chrono::steady_clock::now();
            double ms_total = std::chrono::duration<double, std::milli>(t_end - t_start).count();
            fprintf(stderr, "[BigTime CPU] digits=%zu  total=%.2fms  sz=%zu  pre=%.30s  suf=%.30s\n",
                    maxDigits, ms_total, r.size(), r.c_str(),
                    r.size() > 30 ? r.c_str() + r.size() - 30 : r.c_str());
            fprintf(stderr, "[BigTime Experimental-CPU Karatsuba] digits=%zu  total=%.2fms  sz=%zu  pre=%.30s  suf=%.30s\n",
                    maxDigits, ms_total, r.size(), r.c_str(),
                    r.size() > 30 ? r.c_str() + r.size() - 30 : r.c_str());
        }
        return r;
    }

    // Large numbers: 4-Step AVX2 NTT Pipeline
    int na = (int)((a_digits + 8) / 9);
    int nb = (int)((b_digits + 8) / 9);
    size_t tot = (size_t)na + (size_t)nb;

    size_t n = 1;
    while (n < tot) n <<= 1;

    // 1. Fast parallel SWAR parsing
    std::vector<uint32_t> in_a(n, 0), in_b(n, 0);
    parse_str_to_limbs(s_a, in_a.data(), na);
    parse_str_to_limbs(s_b, in_b.data(), nb);

    auto t_parse = std::chrono::steady_clock::now();

    // 2. 3-Prime AVX2 NTT transforms
    std::vector<uint32_t> r_ntt[3];
    for (int pi = 0; pi < 3; pi++) r_ntt[pi].resize(n);

    std::vector<uint32_t> fa(n), fb(n), sc(2 * n);

    int k = 0; size_t m = n; while (m > 1) { m >>= 1; k++; }
    int k1 = (k + 1) / 2;
    int k2 = k - k1;
    size_t N1 = (size_t)1 << k1;
    size_t N2 = (size_t)1 << k2;

    g_twManager.ensure_twiddles(n, N1, N2);

    for (int pi = 0; pi < 3; pi++) {
        uint32_t mod = P[pi], p_inv = P_INV[pi], r2 = R2_MOD[pi];
        __m256i v_mod = _mm256_set1_epi32(mod);
        __m256i v_pinv = _mm256_set1_epi32(p_inv);

        // Pass 1: AVX2 Fused load + mont + transpose N1 x N2 -> N2 x N1
        fused_input_transpose_mont_avx2(sc.data(), in_a.data(), N1, N2, mod, p_inv, r2);
        fused_input_transpose_mont_avx2(fa.data(), in_b.data(), N1, N2, mod, p_inv, r2);

        // Pass 2: AVX2 Row NTT of length N1 on all N2 rows
        #pragma omp parallel for schedule(static)
        for (int r = 0; r < (int)N2; r++) {
            avx2_in_l1_row_ntt(sc.data() + r * N1, N1, g_twManager.tw_N1[pi], false, mod, p_inv);
            avx2_in_l1_row_ntt(fa.data() + r * N1, N1, g_twManager.tw_N1[pi], false, mod, p_inv);
        }

        // Pass 3: AVX2 8x8 Tiled Transpose with Fused Twiddles
        transpose_twiddle_forward_avx2(pi, fb.data(), sc.data(), N1, N2, n);
        transpose_twiddle_forward_avx2(pi, sc.data(), fa.data(), N1, N2, n);

        // Pass 4: AVX2 Row NTT of length N2 on all N1 rows
        #pragma omp parallel for schedule(static)
        for (int r = 0; r < (int)N1; r++) {
            avx2_in_l1_row_ntt(fb.data() + r * N2, N2, g_twManager.tw_N2[pi], false, mod, p_inv);
            avx2_in_l1_row_ntt(sc.data() + r * N2, N2, g_twManager.tw_N2[pi], false, mod, p_inv);
        }

        // Pointwise Montgomery multiplication
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < (int)n; i += 8) {
            __m256i va = _mm256_loadu_si256((const __m256i*)(fb.data() + i));
            __m256i vb = _mm256_loadu_si256((const __m256i*)(sc.data() + i));
            __m256i vres = avx2_mont_mul(va, vb, v_mod, v_pinv);
            _mm256_storeu_si256((__m256i*)(fb.data() + i), vres);
        }

        // Inverse 4-step transform
        ntt_4step_inverse_avx2(pi, r_ntt[pi].data(), fb.data(), sc.data(), N1, N2, n);
    }

    auto t_ntt = std::chrono::steady_clock::now();

    // 3. CRT Reconstruction & Carry Propagation (Zero Heap Allocations)
    uint32_t* result_limbs = in_a.data();
    int pos = 0;
    crt_reconstruct_and_propagate(result_limbs, pos, r_ntt[0].data(), r_ntt[1].data(), r_ntt[2].data(), n, na + nb + 2, (uint64_t*)sc.data());

    auto t_crt = std::chrono::steady_clock::now();

    // 4. Fast parallel string formatting
    std::string r = format_limbs_to_str(result_limbs, pos);

    auto t_format = std::chrono::steady_clock::now();

    double ms_parse = std::chrono::duration<double, std::milli>(t_parse - t_start).count();
    double ms_ntt = std::chrono::duration<double, std::milli>(t_ntt - t_parse).count();
    double ms_crt = std::chrono::duration<double, std::milli>(t_crt - t_ntt).count();
    double ms_format = std::chrono::duration<double, std::milli>(t_format - t_crt).count();
    double ms_total = std::chrono::duration<double, std::milli>(t_format - t_start).count();

    if (g_bigTime) {
        fprintf(stderr, "[BigTime CPU] digits=%zu  total=%.2fms  sz=%zu  pre=%.30s  suf=%.30s\n",
                maxDigits, ms_total, r.size(), r.c_str(),
                r.size() > 30 ? r.c_str() + r.size() - 30 : r.c_str());
        fprintf(stderr, "[BigTime Experimental-CPU] digits=%zu  parse=%.2fms  ntt=%.2fms  crt=%.2fms  format=%.2fms  total=%.2fms  sz=%zu  pre=%.30s  suf=%.30s\n",
                maxDigits, ms_parse, ms_ntt, ms_crt, ms_format, ms_total, r.size(), r.c_str(),
                r.size() > 30 ? r.c_str() + r.size() - 30 : r.c_str());
    }

    if (neg_a != neg_b && r != "0") r = "-" + r;
    return r;
}
