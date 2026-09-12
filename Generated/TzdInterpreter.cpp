#include "TzdInterpreter.h"

#include "TzdGC.h"
#include "TzdTieringEngine.h"
#include "TzdBytecode.h"

bool tzdStackNearOverflow() {
    static thread_local ULONG_PTR s_low = 0;
    static thread_local ULONG_PTR s_high = 0;
    if (s_low == 0) {
        GetCurrentThreadStackLimits(&s_low, &s_high);
    }
    char probe;
    constexpr ULONG_PTR kGuard = 4ull * 1024 * 1024;
    return (reinterpret_cast<ULONG_PTR>(&probe) - s_low) < kGuard;
}

TzdValue::TzdValue(TzdInstance* i) : type(INSTANCE), instanceVal(i) {
    if (instanceVal) instanceVal->retain();
}

TzdValue::TzdValue(const TzdValue& o)
    : annotations(o.annotations), type(o.type), name(o.name),
      dVal(o.dVal), lVal(o.lVal), ulVal(o.ulVal), ptrVal(o.ptrVal),
      sVal(o.sVal), bVal(o.bVal), arrVal(o.arrVal), mapVal(o.mapVal),
      params(o.params), paramTypes(o.paramTypes), funcBody(o.funcBody),
      nativeFunc(o.nativeFunc), classDefVal(o.classDefVal),
      instanceVal(o.instanceVal), sourceFile(o.sourceFile),
      line(o.line), column(o.column),
      jitInternalName(o.jitInternalName), nativeArr(o.nativeArr),
      isNativeDoubleArr(o.isNativeDoubleArr), jittedPtr(o.jittedPtr) {
    if (instanceVal) instanceVal->retain();
    if (type == TzdValue::TENSOR && ptrVal) tzdTensorRetain(ptrVal);
}

TzdValue::TzdValue(TzdValue&& o) noexcept
    : annotations(std::move(o.annotations)), type(o.type), name(std::move(o.name)),
      dVal(o.dVal), lVal(o.lVal), ulVal(o.ulVal), ptrVal(o.ptrVal),
      sVal(std::move(o.sVal)), bVal(o.bVal), arrVal(std::move(o.arrVal)), mapVal(std::move(o.mapVal)),
      params(std::move(o.params)), paramTypes(std::move(o.paramTypes)), funcBody(o.funcBody),
      nativeFunc(std::move(o.nativeFunc)), classDefVal(o.classDefVal),
      instanceVal(o.instanceVal), sourceFile(std::move(o.sourceFile)),
      line(o.line), column(o.column),
      jitInternalName(std::move(o.jitInternalName)), nativeArr(std::move(o.nativeArr)),
      isNativeDoubleArr(o.isNativeDoubleArr), jittedPtr(o.jittedPtr) {
    o.instanceVal = nullptr;
    o.ptrVal = nullptr;
    o.jittedPtr = nullptr;
    o.type = NONE;
}

TzdValue& TzdValue::operator=(const TzdValue& o) {
    if (this == &o) return *this;
    if (instanceVal) instanceVal->release();
    if (type == TzdValue::TENSOR && ptrVal) tzdTensorRelease(ptrVal);

    annotations = o.annotations;
    type = o.type;
    name = o.name;
    dVal = o.dVal;
    lVal = o.lVal;
    ulVal = o.ulVal;
    ptrVal = o.ptrVal;
    sVal = o.sVal;
    bVal = o.bVal;
    arrVal = o.arrVal;
    mapVal = o.mapVal;
    params = o.params;
    paramTypes = o.paramTypes;
    funcBody = o.funcBody;
    nativeFunc = o.nativeFunc;
    classDefVal = o.classDefVal;
    instanceVal = o.instanceVal;
    sourceFile = o.sourceFile;
    line = o.line;
    column = o.column;
    jitInternalName = o.jitInternalName;
    nativeArr = o.nativeArr;
    isNativeDoubleArr = o.isNativeDoubleArr;
    jittedPtr = o.jittedPtr;

    if (instanceVal) instanceVal->retain();
    if (type == TzdValue::TENSOR && ptrVal) tzdTensorRetain(ptrVal);
    return *this;
}

TzdValue& TzdValue::operator=(TzdValue&& o) noexcept {
    if (this == &o) return *this;
    if (instanceVal) instanceVal->release();
    if (type == TzdValue::TENSOR && ptrVal) tzdTensorRelease(ptrVal);

    annotations = std::move(o.annotations);
    type = o.type;
    name = std::move(o.name);
    dVal = o.dVal;
    lVal = o.lVal;
    ulVal = o.ulVal;
    ptrVal = o.ptrVal;
    sVal = std::move(o.sVal);
    bVal = o.bVal;
    arrVal = std::move(o.arrVal);
    mapVal = std::move(o.mapVal);
    params = std::move(o.params);
    paramTypes = std::move(o.paramTypes);
    funcBody = o.funcBody;
    nativeFunc = std::move(o.nativeFunc);
    classDefVal = o.classDefVal;
    instanceVal = o.instanceVal;
    sourceFile = std::move(o.sourceFile);
    line = o.line;
    column = o.column;
    jitInternalName = std::move(o.jitInternalName);
    nativeArr = std::move(o.nativeArr);
    isNativeDoubleArr = o.isNativeDoubleArr;
    jittedPtr = o.jittedPtr;

    o.instanceVal = nullptr;
    o.ptrVal = nullptr;
    o.jittedPtr = nullptr;
    o.type = NONE;
    return *this;
}

TzdValue::~TzdValue() {
    if (instanceVal) {
        instanceVal->release();
        instanceVal = nullptr;
    }
    if (type == TzdValue::TENSOR && ptrVal) {
        tzdTensorRelease(ptrVal);
        ptrVal = nullptr;
    }
}

void TzdValue::setInstance(TzdInstance* i) {
    if (instanceVal == i) return;
    if (instanceVal) instanceVal->release();
    instanceVal = i;
    if (instanceVal) instanceVal->retain();
}

void tzdPoolSlotReleaseInstance(TzdValue* v) {
    if (!v) return;
    if (v->instanceVal) {
        v->instanceVal->release();
        v->instanceVal = nullptr;
    }
    if (v->type == TzdValue::TENSOR && v->ptrVal) {
        tzdTensorRelease(v->ptrVal);
        v->ptrVal = nullptr;
    }
}

// ============================================================================
// Big integer arithmetic — arbitrary precision using decimal strings
// Stored in MSB-first order, negative numbers prefixed with '-'
// ============================================================================

#include <algorithm>
#include <complex>
#include <unordered_map>
#include <stdint.h>
#include <intrin.h>  // _umul128, _udiv128 for 128-bit carry handling
#include <mutex>
#include <thread>

// ============================================================================
// uint64_t[] limb-based BIGINT arithmetic (base 10^9, LSB-first)
// Each limb stores a value < 10^9 (9 decimal digits). 2.25x fewer limbs than
// base 10^4, and integer ops are 10x faster than string ops.
// ============================================================================

static constexpr uint64_t LIMB_BASE = 1000000000ULL;  // 10^9
static constexpr int LIMB_DIGITS = 9;

// --- string ↔ limbs conversion (O(n), no allocation per limb, parallelized) ---
std::vector<uint64_t> limbs_from_str(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == '-' || s[start] == '0')) start++;
    if (start >= s.size()) return {};  // zero
    int len = (int)(s.size() - start);
    int n_limbs = (len + LIMB_DIGITS - 1) / LIMB_DIGITS;
    std::vector<uint64_t> limbs(n_limbs);
    const char* str = s.data() + start;

    #pragma omp parallel for schedule(static) if(n_limbs > 10000)
    for (int k = 0; k < n_limbs; k++) {
        int end_pos = len - k * LIMB_DIGITS;
        int start_pos = end_pos - LIMB_DIGITS;
        if (start_pos < 0) start_pos = 0;
        uint64_t v = 0;
        const char* p = str + start_pos;
        const char* p_end = str + end_pos;
        while (p < p_end) {
            v = v * 10 + (uint64_t)(*p++ - '0');
        }
        limbs[k] = v;
    }
    while (!limbs.empty() && limbs.back() == 0) limbs.pop_back();
    return limbs;
}

std::string limbs_to_str(const std::vector<uint64_t>& limbs) {
    if (limbs.empty()) return "0";
    size_t sz = limbs.size();
    uint64_t top = limbs.back();
    char tmp[16]; int tl = 0;
    if (top == 0) { tmp[tl++] = '0'; }
    else {
        uint64_t t = top;
        while (t > 0) { tmp[tl++] = '0' + (int)(t % 10); t /= 10; }
        std::reverse(tmp, tmp + tl);
    }
    size_t total_len = (size_t)tl + (sz - 1) * LIMB_DIGITS;
    std::string r;
    r.resize(total_len);
    memcpy(&r[0], tmp, tl);

    #pragma omp parallel for schedule(static) if(sz > 10000)
    for (int k = 0; k < (int)sz - 1; k++) {
        char* p = &r[0] + tl + (sz - 2 - k) * LIMB_DIGITS;
        uint64_t v = limbs[k];
        p[8] = '0' + (int)(v % 10); v /= 10;
        p[7] = '0' + (int)(v % 10); v /= 10;
        p[6] = '0' + (int)(v % 10); v /= 10;
        p[5] = '0' + (int)(v % 10); v /= 10;
        p[4] = '0' + (int)(v % 10); v /= 10;
        p[3] = '0' + (int)(v % 10); v /= 10;
        p[2] = '0' + (int)(v % 10); v /= 10;
        p[1] = '0' + (int)(v % 10); v /= 10;
        p[0] = '0' + (int)(v);
    }
    return r;
}

// --- limb arithmetic: add, sub, compare ---
static std::vector<uint64_t> limbs_add(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    std::vector<uint64_t> r((a.size() > b.size() ? a.size() : b.size()) + 1, 0);
    uint64_t carry = 0;
    for (size_t i = 0; i < r.size(); i++) {
        uint64_t v = carry;
        if (i < a.size()) v += a[i];
        if (i < b.size()) v += b[i];
        if (v >= LIMB_BASE) { r[i] = v - LIMB_BASE; carry = 1; }
        else { r[i] = v; carry = 0; }
    }
    while (!r.empty() && r.back() == 0) r.pop_back();
    return r;
}

// a - b, assumes |a| >= |b| (both non-negative)
static std::vector<uint64_t> limbs_sub(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    std::vector<uint64_t> r(a.size(), 0);
    int64_t borrow = 0;
    for (size_t i = 0; i < a.size(); i++) {
        int64_t v = (int64_t)a[i] - (borrow ? 1 : 0);
        if (i < b.size()) v -= (int64_t)b[i];
        if (v < 0) { v += LIMB_BASE; borrow = 1; } else borrow = 0;
        r[i] = (uint64_t)v;
    }
    while (!r.empty() && r.back() == 0) r.pop_back();
    return r;
}

// compare: -1 if a<b, 0 if a==b, 1 if a>b (both non-negative)
static int limbs_cmp(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.size() != b.size()) return a.size() > b.size() ? 1 : -1;
    for (int i = (int)a.size() - 1; i >= 0; i--)
        if (a[i] != b[i]) return a[i] > b[i] ? 1 : -1;
    return 0;
}

// schoolbook multiplication (for small numbers)
static std::vector<uint64_t> limbs_mul_school(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.empty() || b.empty()) return {};
    std::vector<uint64_t> r(a.size() + b.size(), 0);
    for (size_t i = 0; i < a.size(); i++) {
        uint64_t carry = 0;
        for (size_t j = 0; j < b.size(); j++) {
            uint64_t prod = a[i] * b[j] + r[i + j] + carry;
            r[i + j] = prod % LIMB_BASE;
            carry = prod / LIMB_BASE;
        }
        r[i + b.size()] += carry;
    }
    while (!r.empty() && r.back() == 0) r.pop_back();
    return r;
}

// Karatsuba multiplication (for medium numbers, > 32 limbs)
static std::vector<uint64_t> limbs_mul(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b);
static std::vector<uint64_t> limbs_mul_karatsuba(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    size_t n = a.size(), m = b.size();
    if (n < m) return limbs_mul_karatsuba(b, a);
    if (m <= 32 || n <= 32) return limbs_mul_school(a, b);
    size_t k = (n + 1) / 2;
    // Split a = a1 * B^k + a0
    std::vector<uint64_t> a0(a.begin(), a.begin() + (k < n ? k : n));
    std::vector<uint64_t> a1(k < n ? a.begin() + k : a.end(), a.end());
    // Split b = b1 * B^k + b0
    std::vector<uint64_t> b0(b.begin(), b.begin() + (k < m ? k : m));
    std::vector<uint64_t> b1(k < m ? b.begin() + k : b.end(), b.end());
    auto z0 = limbs_mul(a0, b0);
    auto z2 = limbs_mul(a1, b1);
    auto z1 = limbs_sub(limbs_mul(limbs_add(a0, a1), limbs_add(b0, b1)), limbs_add(z0, z2));
    // result = z2 * B^(2k) + z1 * B^k + z0
    std::vector<uint64_t> r(a.size() + b.size(), 0);
    for (size_t i = 0; i < z0.size(); i++) r[i] = z0[i];
    // Add z1 * B^k
    uint64_t carry = 0;
    for (size_t i = 0; i < z1.size() || carry; i++) {
        uint64_t v = (i < r.size() ? r[i + k] : 0) + (i < z1.size() ? z1[i] : 0) + carry;
        r[i + k] = v % LIMB_BASE;
        carry = v / LIMB_BASE;
    }
    // Add z2 * B^(2k)
    carry = 0;
    for (size_t i = 0; i < z2.size() || carry; i++) {
        uint64_t v = (i + 2*k < r.size() ? r[i + 2*k] : 0) + (i < z2.size() ? z2[i] : 0) + carry;
        r[i + 2*k] = v % LIMB_BASE;
        carry = v / LIMB_BASE;
    }
    while (!r.empty() && r.back() == 0) r.pop_back();
    return r;
}

// --- 3-prime NTT for base 10^9 multiplication ---
static const uint64_t NTT_P1 = 998244353, NTT_P2 = 985661441, NTT_P3 = 754974721;

// Barrett reduction: replaces (a*b)%P with __umulh + subtract (~5 cycles vs ~30 cycles for div)
// M = floor(2^64 / P). q = __umulh(prod, M) approximates prod/P. r = prod - q*P in [0, 2P).
static inline uint64_t compute_barrett_m(uint64_t p) {
    uint64_t d = UINT64_MAX / p, r = UINT64_MAX % p;
    return (r == p - 1) ? (d + 1) : d;
}
template<uint64_t P>
static inline uint64_t nmul(uint64_t a, uint64_t b) {
    static const uint64_t M = compute_barrett_m(P);
    uint64_t prod = a * b;
    uint64_t q = __umulh(prod, M);
    uint64_t r = prod - q * P;
    if (r >= P) r -= P;
    if (r >= P) r -= P;
    return r;
}
template<uint64_t P> static inline uint64_t nadd(uint64_t a, uint64_t b) { uint64_t r=a+b; return r>=P?r-P:r; }
template<uint64_t P> static inline uint64_t nsub(uint64_t a, uint64_t b) { return a>=b?a-b:a+P-b; }
template<uint64_t P> static uint64_t npow(uint64_t b, uint64_t e) { uint64_t r=1;b%=P;while(e){if(e&1)r=nmul<P>(r,b);b=nmul<P>(b,b);e>>=1;}return r; }

struct NR { std::vector<uint64_t> f, i; };
static NR g_nr[3][24];
static std::once_flag g_nr_once[3][24];

template<uint64_t P, uint64_t G>
static void ntt_init(int pi, int ln) {
    std::call_once(g_nr_once[pi][ln], [&]() {
        size_t n = (size_t)1 << ln;
        uint64_t w = npow<P>(G, (P-1)/n), wi = npow<P>(w, P-2);
        auto& r = g_nr[pi][ln];
        r.f.resize(n/2); r.i.resize(n/2);
        uint64_t a=1, ai=1;
        for (size_t j=0; j<n/2; j++) { r.f[j]=a; r.i[j]=ai; a=nmul<P>(a,w); ai=nmul<P>(ai,wi); }
    });
}

template<uint64_t P, uint64_t G>
static void ntt_xform(uint64_t* a, size_t n, bool inv, int pi) {
    if (n<=1) return;
    int ln=0; size_t m=n; while(m>1){m>>=1;ln++;}
    ntt_init<P,G>(pi, ln);
    const uint64_t* rt = inv ? g_nr[pi][ln].i.data() : g_nr[pi][ln].f.data();
    for (size_t i=1,j=0; i<n; ++i) { size_t b=n>>1; for(;j&b;b>>=1)j^=b; j^=b; if(i<j)std::swap(a[i],a[j]); }
    for (size_t len=2; len<=n; len<<=1) {
        size_t h=len/2, s=n/len;
        for (size_t i=0; i<n; i+=len)
            for (size_t j=0; j<h; ++j) {
                uint64_t w=rt[j*s], u=a[i+j], v=nmul<P>(a[i+j+h],w);
                a[i+j]=nadd<P>(u,v); a[i+j+h]=nsub<P>(u,v);
            }
    }
    if (inv) { uint64_t ni=npow<P>(n,P-2); for(size_t i=0;i<n;i++)a[i]=nmul<P>(a[i],ni); }
}

// ---- 4-step (cache-blocked) NTT for large transforms ----
// When N exceeds L2/L3 cache, the standard NTT suffers from cache misses on
// every butterfly. The 4-step Cooley-Tukey decomposition breaks N = N1*N2
// into sub-transforms that fit in L1 cache (8KB each for N1=N2=1024).
//
// Steps: (1) row NTT of size N2, (2) twiddle multiply w^(i*j),
//        (3) blocked transpose, (4) row NTT of size N1, (5) transpose back.

// Blocked transpose: src is N1 x N2 (row-major), dst is N2 x N1 (row-major)
// Block size 64x64 = 32KB (fits in L2 256KB), reduces block count vs 32x32
static void blocked_transpose(uint64_t* dst, const uint64_t* src, size_t N1, size_t N2) {
    const size_t B = 64;  // 64x64 block = 32KB, fits in L2
    #pragma omp parallel for schedule(static) if(N1 > 256)
    for (int bi = 0; bi < (int)N1; bi += (int)B) {
        size_t bi_end = bi + B; if (bi_end > N1) bi_end = N1;
        for (size_t bj = 0; bj < N2; bj += B) {
            size_t bj_end = bj + B; if (bj_end > N2) bj_end = N2;
            for (size_t i = bi; i < bi_end; i++) {
                const uint64_t* src_row = src + i * N2;
                for (size_t j = bj; j < bj_end; j++) {
                    dst[j * N1 + i] = src_row[j];
                }
            }
        }
    }
}

template<uint64_t P, uint64_t G>
static void ntt_xform_4step(uint64_t* a, size_t n, bool inv, int pi) {
    if (n <= 1) return;
    // Split n = N1 * N2, both powers of 2, roughly equal
    int k = 0; size_t m = n; while (m > 1) { m >>= 1; k++; }
    int k1 = (k + 1) / 2;
    size_t N1 = (size_t)1 << k1;
    size_t N2 = n / N1;

    // N-th root of unity
    uint64_t w = npow<P>(G, (P - 1) / n);
    if (inv) w = npow<P>(w, P - 2);

    // Step 1: N2-point NTT on each row (N1 rows, each 8KB for N2=1024)
    for (size_t i = 0; i < N1; i++) {
        ntt_xform<P, G>(a + i * N2, N2, inv, pi);
    }

    // Step 2: Twiddle multiplication: a[i*N2+j] *= w^(i*j)
    // w^(i*j) = (w^i)^j. Precompute w^i for each row.
    std::vector<uint64_t> wi(N1);
    wi[0] = 1;
    for (size_t i = 1; i < N1; i++) wi[i] = nmul<P>(wi[i - 1], w);
    for (size_t i = 0; i < N1; i++) {
        uint64_t wij = 1;  // w^(i*0)
        const uint64_t wi_i = wi[i];
        uint64_t* row = a + i * N2;
        for (size_t j = 0; j < N2; j++) {
            row[j] = nmul<P>(row[j], wij);
            wij = nmul<P>(wij, wi_i);  // w^(i*(j+1))
        }
    }

    // Step 3: Transpose N1 x N2 -> N2 x N1 (blocked for cache)
    // Use thread_local buffer to avoid repeated 8MB allocations
    static thread_local std::vector<uint64_t> tmp_buf;
    if (tmp_buf.size() < n) tmp_buf.resize(n);
    blocked_transpose(tmp_buf.data(), a, N1, N2);

    // Step 4: N1-point NTT on each row of transposed matrix (N2 rows)
    for (size_t j = 0; j < N2; j++) {
        ntt_xform<P, G>(tmp_buf.data() + j * N1, N1, inv, pi);
    }

    // Step 5: Transpose back N2 x N1 -> N1 x N2
    blocked_transpose(a, tmp_buf.data(), N2, N1);

    // Inverse normalization (multiply by n^(-1) mod P)
    if (inv) {
        uint64_t ni = npow<P>(n, P - 2);
        for (size_t i = 0; i < n; i++) a[i] = nmul<P>(a[i], ni);
    }
}

// Unified NTT entry point: uses 4-step for large n, standard for small
template<uint64_t P, uint64_t G>
static void ntt_xform_auto(uint64_t* a, size_t n, bool inv, int pi) {
    if (n <= 1) return;
    if (n >= (1u << 18)) {  // 262K+ elements, 2MB+ — exceeds L2, use 4-step
        ntt_xform_4step<P, G>(a, n, inv, pi);
    } else {
        ntt_xform<P, G>(a, n, inv, pi);
    }
}
// Primes: P1=998244353 (gen 3), P2=985661441 (gen 5), P3=754974721 (gen 11)
// CRT: x = r1 + P1*t1 + P1*P2*t2, carry handled with _umul128/_udiv128

// Helper: modular multiply for runtime prime with Barrett reduction
static inline uint64_t nmul_p(uint64_t p, uint64_t a, uint64_t b, uint64_t M) {
    uint64_t prod = a * b;
    uint64_t q = __umulh(prod, M);
    uint64_t r = prod - q * p;
    if (r >= p) r -= p;
    if (r >= p) r -= p;
    return r;
}

static std::vector<uint64_t> limbs_mul_ntt(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.empty() || b.empty()) return {};
    size_t tot = a.size() + b.size(), n = 1;
    while (n < tot) n <<= 1;
    if (n > (1u << 24)) return limbs_mul_karatsuba(a, b); // fallback for extremely large

    uint64_t P1P2 = NTT_P1 * NTT_P2;
    uint64_t inv12 = npow<NTT_P2>(NTT_P1, NTT_P2 - 2);
    uint64_t inv123 = npow<NTT_P3>(P1P2 % NTT_P3, NTT_P3 - 2);

    // Lambda: runs the full NTT pipeline for one prime, returns the inverse-transformed array
    auto run_prime = [&](int pi) -> std::vector<uint64_t> {
        uint64_t p = (pi == 0) ? NTT_P1 : (pi == 1) ? NTT_P2 : NTT_P3;
        uint64_t barrettM = compute_barrett_m(p);
        std::vector<uint64_t> ta(n, 0), tb(n, 0);
        for (size_t i = 0; i < a.size(); i++) ta[i] = a[i] % p;
        for (size_t i = 0; i < b.size(); i++) tb[i] = b[i] % p;
        if (pi == 0) { ntt_xform_auto<NTT_P1, 3>(ta.data(), n, false, 0); ntt_xform_auto<NTT_P1, 3>(tb.data(), n, false, 0); }
        else if (pi == 1) { ntt_xform_auto<NTT_P2, 3>(ta.data(), n, false, 1); ntt_xform_auto<NTT_P2, 3>(tb.data(), n, false, 1); }
        else { ntt_xform_auto<NTT_P3, 11>(ta.data(), n, false, 2); ntt_xform_auto<NTT_P3, 11>(tb.data(), n, false, 2); }
        for (size_t i = 0; i < n; i++) ta[i] = nmul_p(p, ta[i], tb[i], barrettM);
        if (pi == 0) ntt_xform_auto<NTT_P1, 3>(ta.data(), n, true, 0);
        else if (pi == 1) ntt_xform_auto<NTT_P2, 3>(ta.data(), n, true, 1);
        else ntt_xform_auto<NTT_P3, 11>(ta.data(), n, true, 2);
        return ta;
    };

    // Run 3 primes in parallel using std::thread (3x speedup on multi-core)
    std::vector<uint64_t> r0, r1, r2;
    std::thread t0([&]() { r0 = run_prime(0); });
    std::thread t1([&]() { r1 = run_prime(1); });
    r2 = run_prime(2);  // current thread does prime 3
    t0.join();
    t1.join();

    // CRT: combine r0 (mod P1), r1 (mod P2), r2 (mod P3) into result
    // 3-loop approach: better ILP than merged loop (carry chain is isolated)
    std::vector<uint64_t> result(n, 0);
    // Step 1: x12 = r0 + P1 * ((r1 - r0) * inv12 mod P2)
    for (size_t i = 0; i < n; i++) {
        uint64_t rv1 = r0[i], rv2 = r1[i] % NTT_P2;
        uint64_t t1v = nmul<NTT_P2>(nsub<NTT_P2>(rv2, rv1 % NTT_P2), inv12);
        result[i] = rv1 + NTT_P1 * t1v;
    }
    // Step 2: compute partial remainder and quotient for each element
    std::vector<uint64_t> partial_rem(n), partial_q(n);
    for (size_t i = 0; i < n; i++) {
        uint64_t x12 = result[i];
        uint64_t r3 = r2[i] % NTT_P3;
        uint64_t t2 = nmul<NTT_P3>(nsub<NTT_P3>(r3, x12 % NTT_P3), inv123);
        uint64_t prod_hi, prod_lo;
        prod_lo = _umul128(P1P2, t2, &prod_hi);
        uint64_t low = x12 + prod_lo;
        if (low < x12) prod_hi++;
        uint64_t rem;
        uint64_t q = _udiv128(prod_hi, low, LIMB_BASE, &rem);
        partial_rem[i] = rem;
        partial_q[i] = q;
    }
    // Step 3: sequential carry propagation (isolated dependency chain)
    uint64_t carry = 0;
    for (size_t i = 0; i < n; i++) {
        uint64_t v = partial_rem[i] + carry;
        result[i] = v % LIMB_BASE;
        carry = partial_q[i] + (v / LIMB_BASE);
    }
    size_t pos = n;
    while (carry > 0) {
        if (pos >= result.size()) result.push_back(0);
        uint64_t v = result[pos] + carry;
        result[pos] = v % LIMB_BASE;
        carry = v / LIMB_BASE;
        pos++;
    }
    while (!result.empty() && result.back() == 0) result.pop_back();
    return result;
}
static std::vector<uint64_t> limbs_mul(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    if (a.empty() || b.empty()) return {};
    size_t tot = a.size() + b.size();
    // NTT threshold: 128 limbs (~1152 digits) -- with OpenMP, NTT is faster than
    // Karatsuba for these sizes
    if (a.size() > 128 && b.size() > 128 && tot > 256) {
        return limbs_mul_ntt(a, b);
    }
    if (a.size() > 32 || b.size() > 32) {
        return limbs_mul_karatsuba(a, b);
    }
    return limbs_mul_school(a, b);
}

// --- Limb-based division (Newton-Raphson, calls limbs_mul) ---
// For small divisors (≤ 32 limbs), uses schoolbook on limbs.
// For large divisors, uses Newton-Raphson with limbs_mul for the reciprocal.

// Schoolbook division on limbs (base 10^9)
static std::vector<uint64_t> limbs_divmod_school(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b, bool wantMod) {
    if (b.empty()) return {};
    if (limbs_cmp(a, b) < 0) return wantMod ? a : std::vector<uint64_t>{};
    // Knuth Algorithm D (simplified for base 10^9)
    int na = (int)a.size(), nb = (int)b.size();
    // Normalize: multiply both by norm = LIMB_BASE / (b.back() + 1) so that b.back() >= LIMB_BASE/2
    uint64_t norm = LIMB_BASE / (b.back() + 1);
    auto bn = limbs_mul(b, {norm});
    auto an = limbs_mul(a, {norm});
    an.resize(na + 1, 0);  // ensure room for extra digit
    int qn = na - nb + 1;
    std::vector<uint64_t> q(qn, 0);
    for (int j = qn - 1; j >= 0; j--) {
        // Estimate quotient digit from top 2 limbs of remainder
        uint64_t rem_top = (j + nb < (int)an.size() ? an[j + nb] : 0);
        uint64_t rem_next = (j + nb - 1 < (int)an.size() ? an[j + nb - 1] : 0);
        uint64_t q_est = (rem_top * LIMB_BASE + rem_next) / bn.back();
        if (q_est >= LIMB_BASE) q_est = LIMB_BASE - 1;
        // Multiply b by q_est and subtract
        auto prod = limbs_mul(bn, {q_est});
        // Check if prod > an[j..j+nb]
        // Simple approach: try q_est, adjust down if needed
        for (int attempt = 0; attempt < 3; attempt++) {
            // Compare prod with an[j..j+nb]
            bool too_big = false;
            for (int i = nb; i >= 0; i--) {
                uint64_t ai = (j + i < (int)an.size() ? an[j + i] : 0);
                uint64_t pi = (i < (int)prod.size() ? prod[i] : 0);
                if (pi > ai) { too_big = true; break; }
                if (pi < ai) break;
            }
            if (too_big) { q_est--; prod = limbs_mul(bn, {q_est}); continue; }
            break;
        }
        // Subtract prod from an[j..j+nb]
        int64_t borrow = 0;
        for (int i = 0; i <= nb; i++) {
            int64_t v = (int64_t)(j + i < (int)an.size() ? an[j + i] : 0) - (i < (int)prod.size() ? (int64_t)prod[i] : 0) - borrow;
            if (v < 0) { v += LIMB_BASE; borrow = 1; } else borrow = 0;
            if (j + i < (int)an.size()) an[j + i] = (uint64_t)v;
        }
        q[j] = q_est;
    }
    while (!q.empty() && q.back() == 0) q.pop_back();
    if (wantMod) {
        // Un-normalize: divide remainder by norm
        // Remainder is in an[0..nb-1]
        std::vector<uint64_t> rem(an.begin(), an.begin() + (nb < (int)an.size() ? nb : (int)an.size()));
        while (rem.size() > 1 && rem.back() == 0) rem.pop_back();
        // Divide rem by norm (single-limb division)
        std::vector<uint64_t> result;
        uint64_t carry = 0;
        for (int i = (int)rem.size() - 1; i >= 0; i--) {
            uint64_t v = carry * LIMB_BASE + rem[i];
            result.insert(result.begin(), v / norm);
            carry = v % norm;
        }
        while (!result.empty() && result.back() == 0) result.pop_back();
        return result;
    }
    return q;
}

// Newton-Raphson division on limbs (for large divisors)
static std::vector<uint64_t> limbs_divmod(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b, bool wantMod) {
    if (b.empty()) return {};
    if (limbs_cmp(a, b) < 0) return wantMod ? a : std::vector<uint64_t>{};
    if (b.size() <= 32) return limbs_divmod_school(a, b, wantMod);

    int k = (int)b.size();
    // Compute reciprocal v = floor(LIMB_BASE^(2k) / b) using Newton's method
    // Initial: use top limbs of b for a double-precision estimate
    uint64_t b_top = b.back();
    // v0 = LIMB_BASE^(k+1) / (b_top + 1) — single-limb division, underestimate
    std::vector<uint64_t> v;
    {
        // 10^(k+1) as a limb vector: (k+1) limbs of 0, then 1 at position k
        // Actually LIMB_BASE^(k+1) = 1 followed by (k+1) zero limbs
        std::vector<uint64_t> pow_ke(k + 2, 0);
        pow_ke[k + 1] = 1;  // LIMB_BASE^(k+1)
        v = limbs_divmod_school(pow_ke, {b_top + 1}, false);
    }
    // Newton iterations: v' = v * (2*B^(2k) - b*v) / B^(2k)
    // B^(2k) = 1 followed by 2k zero limbs
    std::vector<uint64_t> pow2k(2 * k + 1, 0);
    pow2k[2 * k] = 1;
    std::vector<uint64_t> two_pow2k = limbs_add(pow2k, pow2k);  // 2 * B^(2k)
    int prec = 1;  // initial precision in limbs
    for (int iter = 0; iter < 50 && prec < k; iter++) {
        auto t = limbs_mul(b, v);
        if (limbs_cmp(t, pow2k) > 0) { v = limbs_sub(v, {1}); continue; }
        auto u = limbs_sub(two_pow2k, t);
        auto vu = limbs_mul(v, u);
        std::vector<uint64_t> v_new;
        if ((int)vu.size() > 2 * k) {
            v_new.assign(vu.begin(), vu.end() - 2 * k);
        } else { break; }
        while (!v_new.empty() && v_new.back() == 0) v_new.pop_back();
        if (v_new.empty() || v_new == v) break;
        // Trim to k+1 limbs
        while ((int)v_new.size() > k + 1) v_new.erase(v_new.begin());
        while ((int)v_new.size() < k + 1) v_new.insert(v_new.begin(), 0);
        v = v_new;
        prec = (2 * prec < k) ? 2 * prec : k;
    }

    // Process dividend in blocks of k limbs
    int la = (int)a.size();
    int pad = (k - la % k) % k;
    std::vector<uint64_t> ap(pad, 0);
    ap.insert(ap.end(), a.begin(), a.end());
    int num_blocks = (int)ap.size() / k;

    std::vector<uint64_t> rem;
    std::vector<uint64_t> quotient;
    for (int blk = 0; blk < num_blocks; blk++) {
        // block = ap[blk*k .. (blk+1)*k - 1]
        std::vector<uint64_t> block(ap.begin() + blk * k, ap.begin() + (blk + 1) * k);
        // dividend = rem * B^k + block (concatenate limbs)
        std::vector<uint64_t> dividend = rem;
        dividend.insert(dividend.end(), block.begin(), block.end());
        while (!dividend.empty() && dividend.back() == 0) dividend.pop_back();
        if (dividend.empty()) dividend = {0};
        // q_block = floor(dividend * v / B^(2k))
        auto dv = limbs_mul(dividend, v);
        std::vector<uint64_t> q_block;
        if ((int)dv.size() > 2 * k) {
            q_block.assign(dv.begin(), dv.end() - 2 * k);
        } else { q_block = {0}; }
        while (!q_block.empty() && q_block.back() == 0) q_block.pop_back();
        if (q_block.empty()) q_block = {0};
        // rem = dividend - q_block * b
        auto qb = limbs_mul(q_block, b);
        if (limbs_cmp(dividend, qb) >= 0) {
            rem = limbs_sub(dividend, qb);
        } else {
            q_block = limbs_sub(q_block, {1});
            rem = limbs_sub(dividend, limbs_mul(q_block, b));
        }
        while (limbs_cmp(rem, b) >= 0) {
            rem = limbs_sub(rem, b);
            q_block = limbs_add(q_block, {1});
        }
        // Append q_block to quotient (pad to k limbs except first)
        if (blk > 0) {
            while (q_block.size() < (size_t)k) q_block.push_back(0);
        }
        quotient.insert(quotient.end(), q_block.begin(), q_block.end());
    }
    while (!quotient.empty() && quotient.back() == 0) quotient.pop_back();
    if (wantMod) { while (!rem.empty() && rem.back() == 0) rem.pop_back(); return rem; }
    return quotient;
}

// --- Optimized Cooley-Tukey FFT (no MKL dependency) ---
// Uses precomputed twiddle factors + raw double arrays with manual complex arithmetic.
// O(n log n) iterative radix-2 FFT. Data layout: data[2*i]=Re, data[2*i+1]=Im.

struct TwiddleCache {
    std::vector<double> cos_t, sin_t;
    explicit TwiddleCache(size_t n) {
        cos_t.resize(n / 2);
        sin_t.resize(n / 2);
        for (size_t i = 0; i < n / 2; i++) {
            double ang = -2.0 * 3.14159265358979323846 * (double)i / (double)n;
            cos_t[i] = std::cos(ang);
            sin_t[i] = std::sin(ang);
        }
    }
};
static thread_local std::unordered_map<size_t, TwiddleCache> g_twiddleCache;

static void tzd_fft_opt(double* data, size_t n, bool inverse) {
    if (n <= 1) return;
    // Get or create twiddle cache for this size
    auto it = g_twiddleCache.find(n);
    if (it == g_twiddleCache.end()) {
        it = g_twiddleCache.emplace(n, TwiddleCache(n)).first;
    }
    const double* cos_t = it->second.cos_t.data();
    const double* sin_t = it->second.sin_t.data();

    // Bit-reversal permutation
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            std::swap(data[2*i], data[2*j]);
            std::swap(data[2*i+1], data[2*j+1]);
        }
    }

    // Cooley-Tukey butterfly — fork/join OpenMP (MSVC caches the thread pool)
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len / 2;
        size_t stride = n / len;
        size_t groups = n / len;
        if (groups >= 4) {
            #pragma omp parallel for schedule(static)
            for (ptrdiff_t gi = 0; gi < (ptrdiff_t)groups; ++gi) {
                size_t base_i = (size_t)gi * len;
                for (size_t j = 0; j < half; ++j) {
                    size_t tw = j * stride;
                    double wr = cos_t[tw], wi = inverse ? -sin_t[tw] : sin_t[tw];
                    size_t ai = 2 * (base_i + j), bi = 2 * (base_i + j + half);
                    double ur = data[ai], ui = data[ai + 1];
                    double vr = data[bi], vi = data[bi + 1];
                    double tr = vr * wr - vi * wi, ti = vr * wi + vi * wr;
                    data[ai] = ur + tr; data[ai+1] = ui + ti;
                    data[bi] = ur - tr; data[bi+1] = ui - ti;
                }
            }
        } else {
            for (size_t i = 0; i < n; i += len) {
                for (size_t j = 0; j < half; ++j) {
                    size_t tw = j * stride;
                    double wr = cos_t[tw], wi = inverse ? -sin_t[tw] : sin_t[tw];
                    size_t ai = 2 * (i + j), bi = 2 * (i + j + half);
                    double ur = data[ai], ui = data[ai + 1];
                    double vr = data[bi], vi = data[bi + 1];
                    double tr = vr * wr - vi * wi, ti = vr * wi + vi * wr;
                    data[ai] = ur + tr; data[ai+1] = ui + ti;
                    data[bi] = ur - tr; data[bi+1] = ui - ti;
                }
            }
        }
    }
    if (inverse) {
        double inv = 1.0 / (double)n;
        #pragma omp parallel for schedule(static)
        for (ptrdiff_t i = 0; i < (ptrdiff_t)(2 * n); i += 2) {
            data[i] *= inv;
            data[i + 1] *= inv;
        }
    }
}

// SoA (Structure of Arrays) FFT — separate re[] and im[] arrays.
// The contiguous access pattern in the inner loop enables AVX2 auto-vectorization:
// MSVC packs 4 doubles per YMM register, giving ~4x throughput on butterfly stages.
static void tzd_fft_soa(double* re, double* im, size_t n, bool inverse) {
    if (n <= 1) return;
    auto it = g_twiddleCache.find(n);
    if (it == g_twiddleCache.end()) {
        it = g_twiddleCache.emplace(n, TwiddleCache(n)).first;
    }
    const double* cos_t = it->second.cos_t.data();
    const double* sin_t = it->second.sin_t.data();

    // Bit-reversal permutation (SoA: swap re[] and im[] independently)
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); }
    }

    // Cooley-Tukey butterfly — inner loop over j gives stride-1 access to re[i+j]
    // and re[i+j+half], enabling MSVC to auto-vectorize with AVX2 (4 doubles/vector).
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len / 2;
        size_t stride = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; ++j) {
                size_t tw = j * stride;
                double wr = cos_t[tw];
                double wi = inverse ? -sin_t[tw] : sin_t[tw];

                double ur = re[i + j], ui = im[i + j];
                double vr = re[i + j + half], vi = im[i + j + half];

                // v * w (complex multiply)
                double tr = vr * wr - vi * wi;
                double ti = vr * wi + vi * wr;

                // butterfly: u+v, u-v
                re[i + j]       = ur + tr;
                im[i + j]       = ui + ti;
                re[i + j + half] = ur - tr;
                im[i + j + half] = ui - ti;
            }
        }
    }
    if (inverse) {
        double inv = 1.0 / (double)n;
        for (size_t i = 0; i < n; i++) { re[i] *= inv; im[i] *= inv; }
    }
}

// Standalone FFT-based multiplication: O(n log n), no external dependencies
static std::string bigint_mul_fft_standalone(const std::string& a, const std::string& b) {
    const long long BASE = 10000;
    const int BASE_DIGITS = 4;

    // Manual digit parsing — no substr allocation
    auto to_limbs = [](const std::string& s, int bd) -> std::vector<double> {
        int pd = (bd - (int)(s.size() % bd)) % bd;
        std::string padded = std::string(pd, '0') + s;
        int sz = (int)padded.size();
        std::vector<double> limbs;
        limbs.reserve(sz / bd + 1);
        for (int i = sz - bd; i >= 0; i -= bd) {
            double v = 0;
            for (int j = 0; j < bd; j++) v = v * 10 + (double)(padded[i + j] - '0');
            limbs.push_back(v);
        }
        return limbs;
    };

    std::vector<double> a_limbs = to_limbs(a, BASE_DIGITS);
    std::vector<double> b_limbs = to_limbs(b, BASE_DIGITS);

    size_t total_len = a_limbs.size() + b_limbs.size();
    size_t fft_n = 1;
    while (fft_n < total_len) fft_n <<= 1;

    // Interleaved FFT with precomputed twiddle cache — best cache behavior for large n
    static thread_local std::vector<double> fa, fb;
    fa.assign(2 * fft_n, 0.0); fb.assign(2 * fft_n, 0.0);
    for (size_t i = 0; i < a_limbs.size(); i++) fa[2 * i] = a_limbs[i];
    for (size_t i = 0; i < b_limbs.size(); i++) fb[2 * i] = b_limbs[i];

    tzd_fft_opt(fa.data(), fft_n, false);
    tzd_fft_opt(fb.data(), fft_n, false);

    for (size_t i = 0; i < 2 * fft_n; i += 2) {
        double ar = fa[i], ai = fa[i + 1];
        double br = fb[i], bi = fb[i + 1];
        fa[i]     = ar * br - ai * bi;
        fa[i + 1] = ar * bi + ai * br;
    }

    tzd_fft_opt(fa.data(), fft_n, true);

    // Handle carries + build result string (direct char writes, no "0"+p)
    static thread_local std::vector<long long> result_limbs;
    result_limbs.assign(fft_n + 16, 0);
    long long carry = 0;
    for (size_t i = 0; i < fft_n; i++) {
        long long val = (long long)std::llround(fa[2 * i]) + carry;
        result_limbs[i] = val % BASE;
        carry = val / BASE;
    }
    size_t result_len = fft_n;
    while (carry > 0) { result_limbs[result_len] = carry % BASE; carry /= BASE; result_len++; }

    std::string result;
    result.resize(result_len * BASE_DIGITS);
    char* rp = &result[0];
    for (int i = (int)result_len - 1, pos = 0; i >= 0; i--, pos += BASE_DIGITS) {
        long long v = result_limbs[i];
        rp[pos + 3] = '0' + (int)(v % 10); v /= 10;
        rp[pos + 2] = '0' + (int)(v % 10); v /= 10;
        rp[pos + 1] = '0' + (int)(v % 10); v /= 10;
        rp[pos + 0] = '0' + (int)(v);
    }
    size_t st = 0; while (st < result.size() - 1 && result[st] == '0') st++;
    if (st > 0) result.erase(0, st);
    return result.empty() ? "0" : result;
}

// --- Safety limits for BIGINT operations ---
// Prevents uncontrolled memory growth that could freeze the system.
// Default: 10 million digits (~10MB per number string).
static size_t g_bigintMaxDigits = BIGINT_DEFAULT_MAX_DIGITS;

size_t getBigIntMaxDigits() { return g_bigintMaxDigits; }
void setBigIntMaxDigits(size_t n) {
    // Enforce a hard floor of 1000 digits and a hard ceiling of 100M digits
    if (n < 1000) n = 1000;
    if (n > 100000000) n = 100000000;
    g_bigintMaxDigits = n;
}
bool bigint_too_large(size_t digitCount) { return digitCount > g_bigintMaxDigits; }

bool bigint_is_neg(const std::string& s) { return !s.empty() && s[0] == '-'; }
std::string bigint_abs(const std::string& s) {
    return bigint_is_neg(s) ? s.substr(1) : s;
}

std::string bigint_normalize(const std::string& input) {
    std::string s = input;
    bool neg = bigint_is_neg(s);
    if (neg) s = s.substr(1);
    size_t i = 0;
    while (i < s.size() - 1 && s[i] == '0') ++i;
    s = s.substr(i);
    if (neg && s != "0") s = "-" + s;
    return s;
}

// Compare absolute values: -1 if a<b, 0 if a==b, 1 if a>b
static int bigint_abs_compare(std::string a, std::string b) {
    a = bigint_normalize(a); b = bigint_normalize(b);
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    return a.compare(b);
}

int bigint_compare(const std::string& a, const std::string& b) {
    bool an = bigint_is_neg(a), bn = bigint_is_neg(b);
    if (an && !bn) return -1;
    if (!an && bn) return 1;
    auto la = limbs_from_str(a), lb = limbs_from_str(b);
    int c = limbs_cmp(la, lb);
    return an ? -c : c;
}

// Add two non-negative digit strings
static std::string bigint_add_abs(std::string a, std::string b) {
    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());
    if (a.size() < b.size()) a.resize(b.size(), '0');
    if (b.size() < a.size()) b.resize(a.size(), '0');
    int carry = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        int s = (a[i] - '0') + (b[i] - '0') + carry;
        a[i] = '0' + (s % 10);
        carry = s / 10;
    }
    if (carry) a.push_back('0' + carry);
    std::reverse(a.begin(), a.end());
    return bigint_normalize(a);
}

// Subtract: a - b, assumes |a| >= |b|, both non-negative
static std::string bigint_sub_abs(std::string a, std::string b) {
    std::reverse(a.begin(), a.end());
    std::reverse(b.begin(), b.end());
    if (b.size() < a.size()) b.resize(a.size(), '0');
    int borrow = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        int d = (a[i] - '0') - (b[i] - '0') - borrow;
        if (d < 0) { d += 10; borrow = 1; } else borrow = 0;
        a[i] = '0' + d;
    }
    std::reverse(a.begin(), a.end());
    return bigint_normalize(a);
}

std::string bigint_add(const std::string& a, const std::string& b) {
    bool an = bigint_is_neg(a), bn = bigint_is_neg(b);
    auto la = limbs_from_str(a), lb = limbs_from_str(b);
    if (!an && !bn) return limbs_to_str(limbs_add(la, lb));
    if (an && bn) return "-" + limbs_to_str(limbs_add(la, lb));
    // Mixed signs
    int c = limbs_cmp(la, lb);
    if (c == 0) return "0";
    if (c > 0) return (an ? "-" : "") + limbs_to_str(limbs_sub(la, lb));
    return (bn ? "-" : "") + limbs_to_str(limbs_sub(lb, la));
}

std::string bigint_sub(const std::string& a, const std::string& b) {
    bool an = bigint_is_neg(a), bn = bigint_is_neg(b);
    auto la = limbs_from_str(a), lb = limbs_from_str(b);
    if (!an && !bn) {
        int c = limbs_cmp(la, lb);
        if (c == 0) return "0";
        return (c < 0 ? "-" : "") + limbs_to_str(c > 0 ? limbs_sub(la, lb) : limbs_sub(lb, la));
    }
    if (an && bn) {
        int c = limbs_cmp(la, lb);
        if (c == 0) return "0";
        return (c > 0 ? "-" : "") + limbs_to_str(c > 0 ? limbs_sub(la, lb) : limbs_sub(lb, la));
    }
    // Mixed signs: |a| + |b|
    return (an ? "-" : "") + limbs_to_str(limbs_add(la, lb));
}

// Schoolbook multiplication of non-negative digit strings
static std::string bigint_mul_school(const std::string& a, const std::string& b) {
    if (a == "0" || b == "0") return "0";
    std::vector<int> res(a.size() + b.size(), 0);
    for (int i = (int)a.size() - 1; i >= 0; --i) {
        for (int j = (int)b.size() - 1; j >= 0; --j) {
            int prod = (a[i] - '0') * (b[j] - '0');
            int pos = (a.size() - 1 - i) + (b.size() - 1 - j);
            res[pos] += prod;
            res[pos + 1] += res[pos] / 10;
            res[pos] %= 10;
        }
    }
    std::string r;
    for (int i = (int)res.size() - 1; i >= 0; --i) {
        if (!r.empty() || res[i] != 0 || i == 0) r.push_back('0' + res[i]);
    }
    return r.empty() ? "0" : r;
}

std::string bigint_mul(const std::string& a, const std::string& b) {
    extern TzdInterpreter* g_CurrentInterpreter;
    bool bigTime = g_CurrentInterpreter && g_CurrentInterpreter->m_bigTime;

    bool an = bigint_is_neg(a), bn = bigint_is_neg(b);
    size_t a_digits = (a.empty() || a[0] != '-') ? a.size() : a.size() - 1;
    size_t b_digits = (b.empty() || b[0] != '-') ? b.size() : b.size() - 1;
    if (bigint_too_large(a_digits + b_digits)) return "inf";
    std::string r;
    size_t maxDigits = a_digits > b_digits ? a_digits : b_digits;

    // GPU check (first call initializes CUDA runtime — keep out of timing)
    g_forceGPU = (g_CurrentInterpreter && g_CurrentInterpreter->m_forceGPU);
    g_forceCPU = (g_CurrentInterpreter && g_CurrentInterpreter->m_forceCPU);
    g_bigTime = bigTime;
    bool useGPU = bigint_gpu_suitable(maxDigits);

    auto bt0 = std::chrono::steady_clock::now();
    auto la = limbs_from_str(a), lb = limbs_from_str(b);
    auto bt1 = std::chrono::steady_clock::now();

    std::vector<uint64_t> lr;
    if (useGPU) {
        try {
            lr = bigint_mul_gpu_ntt_limbs(la, lb);
        } catch (const std::exception& e) {
            fprintf(stderr, "[GPU Error] %s\n", e.what());
            lr = limbs_mul(la, lb);
        } catch (...) {
            fprintf(stderr, "[GPU Error] unknown exception\n");
            lr = limbs_mul(la, lb);
        }
    } else {
        lr = limbs_mul(la, lb);
    }
    auto bt2 = std::chrono::steady_clock::now();

    r = limbs_to_str(lr);
    auto bt3 = std::chrono::steady_clock::now();

    if (bigTime) {
        fprintf(stderr, "[BigTime] digits=%zu  str2limb=%.2fms  ntt=%.2fms  limb2str=%.2fms  total=%.2fms\n",
                maxDigits,
                std::chrono::duration<double, std::milli>(bt1 - bt0).count(),
                std::chrono::duration<double, std::milli>(bt2 - bt1).count(),
                std::chrono::duration<double, std::milli>(bt3 - bt2).count(),
                std::chrono::duration<double, std::milli>(bt3 - bt0).count());
    }

    bool neg = an != bn;
    return (neg && r != "0") ? "-" + r : r;
}

static std::string bigint_mul_karatsuba(const std::string& a, const std::string& b) {
    int n = (int)a.size(); int bs = (int)b.size();
    if (n < bs) return bigint_mul_karatsuba(b, a); // ensure a.size() >= b.size()
    if (n <= 256) return bigint_mul_school(a, b);
    // Pad to equal length
    std::string aa = a, bb = b;
    while ((int)aa.size() < n) aa = "0" + aa;
    while ((int)bb.size() < n) bb = "0" + bb;
    int k = n / 2;
    // Split: a = a1 * 10^k + a0, b = b1 * 10^k + b0
    std::string a1 = aa.substr(0, n - k), a0 = aa.substr(n - k);
    std::string b1 = bb.substr(0, n - k), b0 = bb.substr(n - k);
    std::string z0 = bigint_mul(a0, b0);
    std::string z2 = bigint_mul(a1, b1);
    std::string z1 = bigint_sub(bigint_mul(bigint_add(a1, a0), bigint_add(b1, b0)),
                                bigint_add(z2, z0));
    // result = z2 * 10^(2k) + z1 * 10^k + z0
    std::string result = z2;
    result.append(2 * k, '0');
    std::string z1shifted = z1;
    z1shifted.append(k, '0');
    result = bigint_add(result, z1shifted);
    result = bigint_add(result, z0);
    return bigint_normalize(result);
}

// Division: a / b (integer division), both non-negative
std::string bigint_divmod_abs(const std::string& a, const std::string& b, bool wantMod) {
    if (b == "0") return "0"; // Guard against div by zero
    if (bigint_abs_compare(a, b) < 0) return wantMod ? a : "0";
    std::string quotient, remainder = "0";
    for (size_t i = 0; i < a.size(); ++i) {
        remainder = bigint_normalize(remainder + std::string(1, a[i]));
        // Binary search for the digit
        int lo = 0, hi = 9, digit = 0;
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            std::string test = bigint_mul_school(b, std::to_string(mid));
            if (bigint_abs_compare(test, remainder) <= 0) { digit = mid; lo = mid + 1; }
            else hi = mid - 1;
        }
        quotient.push_back('0' + digit);
        if (digit > 0)
            remainder = bigint_sub_abs(remainder, bigint_mul_school(b, std::to_string(digit)));
    }
    return wantMod ? bigint_normalize(remainder) : bigint_normalize(quotient);
}

// Fast division using Newton-Raphson reciprocal + block-by-block quotient (Burnikel-Ziegler style)
// Computes short reciprocal of b (k=lb digits precision), then divides a in blocks of k digits.
// Each block does 2 multiplications — O(lb log lb) per block, O(la/lb) blocks total.
static std::string bigint_divmod_fast(const std::string& a, const std::string& b, bool wantMod) {
    if (b == "0") return "0";
    if (bigint_abs_compare(a, b) < 0) return wantMod ? a : "0";
    int lb = (int)b.size();
    int la = (int)a.size();
    if (lb <= 512) return bigint_divmod_abs(a, b, wantMod);

    int k = lb; // reciprocal precision = divisor length

    // Step 1: Compute short reciprocal v = floor(10^(2k) / b) using Newton's method
    int hd = (18 < k) ? 18 : k;
    std::string bh = bigint_add(b.substr(0, hd), "1"); // round up → underestimate
    std::string v = bigint_divmod_abs("1" + std::string(k + hd, '0'), bh, false);

    std::string pow2k = "1" + std::string(2 * k, '0');
    std::string two_pow2k = "2" + std::string(2 * k, '0');
    int prec = hd;
    for (int iter = 0; iter < 50 && prec < k; iter++) {
        std::string t = bigint_mul(b, v);
        if (bigint_abs_compare(t, pow2k) > 0) { v = bigint_sub_abs(v, "1"); continue; }
        std::string u = bigint_sub_abs(two_pow2k, t);
        std::string vu = bigint_mul(v, u);
        std::string v_new;
        if ((int)vu.size() > 2 * k) v_new = bigint_normalize(vu.substr(0, vu.size() - 2 * k));
        else break;
        if (v_new == "0" || v_new == v) break;
        // Trim to k+1 digits inside the loop (prevents v from growing)
        while ((int)v_new.size() > k + 1) v_new = v_new.substr(1);
        while ((int)v_new.size() < k + 1) v_new = "0" + v_new;
        v = v_new;
        prec = (2 * prec < k) ? 2 * prec : k;
    }

    // Step 2: Process dividend in blocks of k digits (MSB to LSB)
    // Pad a with leading zeros to make its length a multiple of k
    int pad = (k - la % k) % k;
    std::string ap = std::string(pad, '0') + a;
    int num_blocks = (int)ap.size() / k;

    std::string rem = "0";
    std::string quotient;
    for (int blk = 0; blk < num_blocks; blk++) {
        std::string block = ap.substr(blk * k, k);
        // dividend = rem * 10^k + block (string concatenation since rem < b < 10^k)
        std::string dividend = bigint_normalize(rem + block);
        // q_block = floor(dividend * v / 10^(2k))
        std::string dv = bigint_mul(dividend, v);
        std::string q_block;
        if ((int)dv.size() > 2 * k) q_block = bigint_normalize(dv.substr(0, dv.size() - 2 * k));
        else q_block = "0";
        // rem = dividend - q_block * b
        std::string qb = bigint_mul(q_block, b);
        if (bigint_abs_compare(dividend, qb) >= 0) rem = bigint_sub_abs(dividend, qb);
        else { q_block = bigint_sub_abs(q_block, "1"); rem = bigint_sub_abs(dividend, bigint_mul(q_block, b)); }
        // Adjust (at most 2 times)
        while (bigint_abs_compare(rem, b) >= 0) { rem = bigint_sub_abs(rem, b); q_block = bigint_add(q_block, "1"); }
        // Append q_block to quotient (zero-pad to k digits except first block)
        if (blk > 0) { while (q_block.size() < (size_t)k && q_block[0] != '-') q_block = "0" + q_block; }
        quotient += q_block;
    }

    quotient = bigint_normalize(quotient);
    if (wantMod) return bigint_normalize(rem);
    return quotient;
}

std::string bigint_div(const std::string& a, const std::string& b) {
    bool an = bigint_is_neg(a), bn = bigint_is_neg(b);
    // String-based Newton-Raphson (calls bigint_mul which uses limbs internally)
    std::string r = bigint_divmod_fast(bigint_abs(a), bigint_abs(b), false);
    return (an != bn && r != "0") ? "-" + r : r;
}

std::string bigint_mod(const std::string& a, const std::string& b) {
    bool an = bigint_is_neg(a);
    std::string r = bigint_divmod_fast(bigint_abs(a), bigint_abs(b), true);
    return (an && r != "0") ? "-" + r : r;
}

// Binary exponentiation with configurable result size limit
std::string bigint_pow(const std::string& base, const std::string& exp) {
    if (exp == "0") return "1";
    if (base == "0") return "0";
    if (base == "1") return "1";
    // Check if exponent is negative → return 0 (integer div)
    if (bigint_is_neg(exp)) return "0";

    // Pre-check: estimate result size to avoid wasted computation.
    // result_digits ≈ exp * log10(|base|)
    // If base has B digits, base^exp has approximately B*exp digits.
    std::string e = bigint_abs(exp);
    std::string b = bigint_normalize(base);

    // Quick reject for astronomically large exponents
    // (e.g., 10^1000000000 would need 10GB — reject immediately)
    if (e.size() > 9) return "inf"; // exponent > 999,999,999

    // Estimate: digits ≈ len(base) * exp_value
    // This is a conservative upper bound
    long long expVal = 0;
    try { expVal = std::stoll(e); } catch (...) { return "inf"; }
    if (expVal > 0) {
        long long estDigits = (long long)b.size() * expVal;
        if (bigint_too_large((size_t)estDigits)) return "inf";
    }

    std::string result = "1";

    while (e != "0") {
        // If e is odd, multiply result by b
        if ((e.back() - '0') % 2 == 1) {
            result = bigint_mul(result, b);
            if (bigint_too_large(result.size())) return "inf";
        }
        // Square b
        b = bigint_mul(b, b);
        if (bigint_too_large(b.size()) && e != "1") return "inf";
        // Halve e (integer division by 2)
        e = bigint_divmod_abs(e, "2", false);
    }
    bool neg = bigint_is_neg(base) && ((exp.back() - '0') % 2 == 1);
    return (neg && result != "0") ? "-" + result : result;
}

// Modular exponentiation: base^exp mod m
std::string bigint_powmod(const std::string& base, const std::string& exp, const std::string& mod) {
    if (mod == "0" || mod == "1") return "0";
    std::string result = "1";
    std::string b = bigint_divmod_abs(bigint_abs(base), bigint_abs(mod), true); // b = base mod m
    std::string e = bigint_abs(exp);
    while (e != "0") {
        if ((e.back() - '0') % 2 == 1)
            result = bigint_divmod_abs(bigint_mul(result, b), bigint_abs(mod), true);
        b = bigint_divmod_abs(bigint_mul(b, b), bigint_abs(mod), true);
        e = bigint_divmod_abs(e, "2", false);
    }
    return result;
}

// Convert a TzdValue to a bigint string (for arithmetic)
std::string to_bigint_str(const TzdValue& v) {
    if (v.type == TzdValue::BIGINT) return v.sVal;
    if (v.type == TzdValue::STRING) return v.sVal;
    if (v.type == TzdValue::DOUBLE || v.type == TzdValue::FLOAT) {
        double d = v.dVal;
        if (d == std::floor(d) && std::abs(d) < 1e18) {
            return std::to_string((long long)d);
        }
        // For non-integer doubles, truncate
        return std::to_string((long long)d);
    }
    // All integer types
    if (v.type == TzdValue::ULONG) return std::to_string(v.ulVal);
    return std::to_string(v.lVal);
}

TzdValue make_bigint(const std::string& digits) {
    TzdValue v;
    v.type = TzdValue::BIGINT;
    v.sVal = bigint_normalize(digits);
    if (v.sVal == "0" || v.sVal == "-0") { v.sVal = "0"; v.type = TzdValue::LONG; v.lVal = 0; }
    return v;
}

bool is_bigint(const TzdValue& v) {
    return v.type == TzdValue::BIGINT;
}

bool needs_bigint(const TzdValue& a, const TzdValue& b) {
    return a.type == TzdValue::BIGINT || b.type == TzdValue::BIGINT;
}

// ============================================================================
// Rational (exact fraction) arithmetic — "num/den" in sVal
// ============================================================================

// GCD helper for reduction
static std::string bigint_gcd_impl(std::string a, std::string b) {
    a = bigint_abs(a); b = bigint_abs(b);
    while (b != "0") {
        std::string r = bigint_divmod_abs(a, b, true);
        a = b; b = r;
    }
    return a.empty() ? "0" : a;
}

// Parse "num/den" → (num, den). If no '/', den = "1"
void rational_parse(const std::string& s, std::string& num, std::string& den) {
    size_t pos = s.find('/');
    if (pos == std::string::npos) { num = s; den = "1"; }
    else { num = s.substr(0, pos); den = s.substr(pos + 1); }
    if (num.empty()) num = "0";
    if (den.empty()) den = "1";
}

// Create "num/den" from two bigint strings, reduced by GCD
std::string rational_make(const std::string& num, const std::string& den) {
    if (den == "0") return "0"; // guard
    // Reduce by GCD
    std::string g = bigint_gcd_impl(num, den);
    std::string n = (g != "0" && g != "1") ? bigint_div(num, g) : num;
    std::string d = (g != "0" && g != "1") ? bigint_div(den, g) : den;
    // Normalize sign: denominator should be positive
    if (bigint_is_neg(d)) {
        n = bigint_sub("0", n);
        d = bigint_sub("0", d);
    }
    // If denominator is 1, return just the numerator
    if (d == "1") return n;
    return n + "/" + d;
}

// a/b + c/d = (ad + cb)/(bd)
std::string rational_add(const std::string& a, const std::string& b) {
    std::string an, ad, bn, bd;
    rational_parse(a, an, ad);
    rational_parse(b, bn, bd);
    std::string t1 = bigint_mul(an, bd), t2 = bigint_mul(bn, ad);
    if (t1 == "inf" || t2 == "inf") return "inf";
    std::string num = bigint_add(t1, t2);
    std::string den = bigint_mul(ad, bd);
    if (den == "inf") return "inf";
    return rational_make(num, den);
}

// a/b - c/d = (ad - cb)/(bd)
std::string rational_sub(const std::string& a, const std::string& b) {
    std::string an, ad, bn, bd;
    rational_parse(a, an, ad);
    rational_parse(b, bn, bd);
    std::string t1 = bigint_mul(an, bd), t2 = bigint_mul(bn, ad);
    if (t1 == "inf" || t2 == "inf") return "inf";
    std::string num = bigint_sub(t1, t2);
    std::string den = bigint_mul(ad, bd);
    if (den == "inf") return "inf";
    return rational_make(num, den);
}

// a/b * c/d = ac/bd
std::string rational_mul(const std::string& a, const std::string& b) {
    std::string an, ad, bn, bd;
    rational_parse(a, an, ad);
    rational_parse(b, bn, bd);
    std::string num = bigint_mul(an, bn);
    if (num == "inf") return "inf";
    std::string den = bigint_mul(ad, bd);
    if (den == "inf") return "inf";
    return rational_make(num, den);
}

// (a/b) / (c/d) = ad/bc
std::string rational_div(const std::string& a, const std::string& b) {
    std::string an, ad, bn, bd;
    rational_parse(a, an, ad);
    rational_parse(b, bn, bd);
    if (bn == "0") return "0"; // guard
    std::string num = bigint_mul(an, bd);
    if (num == "inf") return "inf";
    std::string den = bigint_mul(ad, bn);
    if (den == "inf") return "inf";
    return rational_make(num, den);
}

// (a/b)^n = a^n / b^n  (n must be integer, positive or negative)
std::string rational_pow(const std::string& base, const std::string& exp) {
    std::string an, ad;
    rational_parse(base, an, ad);
    bool neg_exp = bigint_is_neg(exp);
    std::string e = bigint_abs(exp);

    // For negative exponent: (a/b)^(-n) = b^n / a^n
    if (neg_exp) {
        std::swap(an, ad);
    }

    std::string num = bigint_pow(an, e);
    std::string den = bigint_pow(ad, e);
    if (num == "inf" || den == "inf") return "inf";
    if (den == "0") return "inf";
    return rational_make(num, den);
}

// Compare a/b vs c/d: cross-multiply → ad vs cb (assumes positive denominators)
int rational_compare(const std::string& a, const std::string& b) {
    std::string an, ad, bn, bd;
    rational_parse(a, an, ad);
    rational_parse(b, bn, bd);
    // ad vs cb (denominators are positive after rational_make)
    std::string left = bigint_mul(an, bd);
    std::string right = bigint_mul(bn, ad);
    return bigint_compare(left, right);
}

// Convert any TzdValue to a rational string "num/den"
std::string to_rational_str(const TzdValue& v) {
    if (v.type == TzdValue::RATIONAL) return v.sVal;
    if (v.type == TzdValue::BIGINT) return v.sVal + "/1";
    if (v.type == TzdValue::STRING) {
        // Try to parse as "a/b" or just a number
        if (v.sVal.find('/') != std::string::npos) return v.sVal;
        return v.sVal + "/1";
    }
    if (v.type == TzdValue::ULONG) return std::to_string(v.ulVal) + "/1";
    if (v.type == TzdValue::DOUBLE || v.type == TzdValue::FLOAT) {
        // Approximate double as fraction (limited precision)
        return std::to_string((long long)v.dVal) + "/1";
    }
    return std::to_string(v.lVal) + "/1";
}

TzdValue make_rational(const std::string& num, const std::string& den) {
    std::string r = rational_make(num, den);
    TzdValue v;
    // If result is an integer (no '/'), store as BIGINT or LONG
    if (r.find('/') == std::string::npos) {
        v.type = TzdValue::BIGINT;
        v.sVal = r;
    } else {
        v.type = TzdValue::RATIONAL;
        v.sVal = r;
    }
    return v;
}

bool needs_rational(const TzdValue& a, const TzdValue& b) {
    return a.type == TzdValue::RATIONAL || b.type == TzdValue::RATIONAL;
}

TzdInterpreter::TzdInterpreter() {
    m_jitEngine = std::make_unique<TzdJitEngine>();
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "main_module");
    scopes.push_back({});
    initNativeFunctions();
}

TzdInterpreter::~TzdInterpreter() {
    for (auto mod : loadedModules) delete mod;
    loadedModules.clear();
}

bool TzdInterpreter::compileToBytecodeFile(const std::string& code, const std::string& outPath) {
    auto _t0 = std::chrono::steady_clock::now();
    antlr4::ANTLRInputStream input(code);
    TzdLangLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();
    auto _t1 = std::chrono::steady_clock::now();
    TzdLangParser parser(&tokens);
    auto* tree = parser.program();
    auto _t2 = std::chrono::steady_clock::now();
    if (parser.getNumberOfSyntaxErrors() > 0) return false;
    TzdBytecodeCompiler compiler;
    BytecodeModule mod = compiler.compile(tree, code);
    auto _t3 = std::chrono::steady_clock::now();
    bool ok = compiler.saveToFile(mod, outPath);

    if (m_antlrTiming) {
        fprintf(stderr, "[ANTLR Timing] compileToBytecodeFile (size: %zu bytes)\n", code.size());
        fprintf(stderr, "  Lexer+Tokens:  %8.1f ms\n", std::chrono::duration<double, std::milli>(_t1 - _t0).count());
        fprintf(stderr, "  Parser:        %8.1f ms\n", std::chrono::duration<double, std::milli>(_t2 - _t1).count());
        fprintf(stderr, "  Bytecode:      %8.1f ms\n", std::chrono::duration<double, std::milli>(_t3 - _t2).count());
    }
    return ok;
}

bool TzdInterpreter::executeBytecodeFile(const std::string& bcPath) {
    // m_noJit prevents EAGER JIT compilation (all functions compiled upfront).
    // The on-demand bytecode JIT bridge (TzdBytecodeJIT) still works:
    // tryJitCompile bypasses m_noJit, and callFunction/callScriptFunction
    // execute already-compiled functions regardless of m_noJit.
    bool savedNoJit = m_noJit;
    m_noJit = true;

    // Ensure native functions are available
    if (scopes.empty()) scopes.push_back({});
    initNativeFunctions();

    TzdBytecodeCompiler bc;
    BytecodeModule mod = bc.loadFromFile(bcPath);

    // Process class/enum/annotation/import declarations from source code.
    // Keep the ANTLR parse tree alive for the entire function scope so that
    // constructor/method body pointers stored in class definitions remain valid
    // during bytecode VM execution.
    std::unique_ptr<antlr4::ANTLRInputStream> input;
    std::unique_ptr<TzdLangLexer> lexer;
    std::unique_ptr<antlr4::CommonTokenStream> tokens;
    std::unique_ptr<TzdLangParser> parser;
    TzdLangParser::ProgramContext* tree = nullptr;

    if (!mod.sourceCode.empty()) {
        input = std::make_unique<antlr4::ANTLRInputStream>(mod.sourceCode);
        lexer = std::make_unique<TzdLangLexer>(input.get());
        tokens = std::make_unique<antlr4::CommonTokenStream>(lexer.get());
        parser = std::make_unique<TzdLangParser>(tokens.get());
        tree = parser->program();
        if (parser->getNumberOfSyntaxErrors() == 0) {
            for (auto stmt : tree->statement()) {
                if (dynamic_cast<TzdLangParser::ClassDeclStmtContext*>(stmt) ||
                    dynamic_cast<TzdLangParser::EnumDeclStmtContext*>(stmt) ||
                    dynamic_cast<TzdLangParser::AnnotationDeclStmtContext*>(stmt) ||
                    dynamic_cast<TzdLangParser::ImportStmtContext*>(stmt) ||
                    dynamic_cast<TzdLangParser::NativeFunDeclStmtContext*>(stmt) ||
                    dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt)) {
                    // Visiting function declarations stores them in scopes with
                    // funcBody + params, enabling the on-demand JIT bridge to
                    // find and compile hot functions during bytecode execution.
                    // m_noJit=true prevents eager JIT compilation here.
                    visit(stmt);
                }
            }
        }
    }

    // Start background JIT tiering engine (async compilation like JDK HotSpot)
    TzdTieringEngine::getInstance().start(2);

    TzdBytecodeVM vm(this);
    vm.execute(mod);

    // Call main() if it exists (while tiering engine is still running)
    if (mod.funcIndex.count("main")) {
        vm.callFunction(mod, "main", {});
    }

    // Stop background threads after all execution is done
    TzdTieringEngine::getInstance().stop();

    m_noJit = savedNoJit;
    return true;
}
#include "../TzdDebugger.h"
#include <fstream>
#include <chrono>
#include <sstream>

//#include "TzdJitCompiler.h"

extern "C" void* rt_get_fatal_jmp();
extern "C" void rt_set_fatal_jmp(void* buf);

std::string formatSourcePath(const std::string& fullPath) {
    if (fullPath.empty() || fullPath == "memory") return "memory";

    // 统一转换为 Windows 风格反斜杠
    std::string path = fullPath;
    std::replace(path.begin(), path.end(), '/', '\\');

    // 检索标准库的关键段 "\stdlib\"
    size_t pos = path.find("\\stdlib\\");
    if (pos == std::string::npos && path.rfind("stdlib\\", 0) == 0) {
        pos = 0;
    }
    if (pos != std::string::npos) {
        return path.substr(pos + 1); // 截取 stdlib\xxx\xxx.tzd
    }
    return path; // 如果不是标准库，显示全绝对路径
}

std::string unescapeString(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && i + 1 < input.size()) {
            switch (input[i + 1]) {
            case 'n':  result += '\n'; break;
            case 'r':  result += '\r'; break;
            case 't':  result += '\t'; break;
            case '\\': result += '\\'; break;
            case '"':  result += '"';  break;
            case '\'': result += '\''; break;
            default:   result += input[i + 1]; break;
            }
            ++i;
        }
        else {
            result += input[i];
        }
    }
    return result;
}

// #region agent log
static void agentLog(const char* hypothesisId, const char* location, const char* message,
    int lType, int rType, bool eq) {
    std::ofstream f("debug-2ea0b5.log", std::ios::app);
    if (!f) return;
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    f << "{\"sessionId\":\"2ea0b5\",\"hypothesisId\":\"" << hypothesisId
        << "\",\"location\":\"" << location << "\",\"message\":\"" << message
        << "\",\"data\":{\"lType\":" << lType << ",\"rType\":" << rType << ",\"eq\":" << (eq ? "true" : "false")
        << "},\"timestamp\":" << ts << "}\n";
}

static void agentLogCompile(const char* hypothesisId, const char* location, const char* detail) {
    std::ofstream f("debug-2ea0b5.log", std::ios::app);
    if (!f) return;
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    f << "{\"sessionId\":\"2ea0b5\",\"hypothesisId\":\"" << hypothesisId
        << "\",\"location\":\"" << location << "\",\"message\":\"jit compile fail\",\"data\":{\"detail\":\""
        << detail << "\"},\"timestamp\":" << ts << "}\n";
}

static TzdValue castAnyToTzdValue(const std::any& a, const char* where) {
    if (!a.has_value()) {
        agentLog("B", where, "empty any", -1, -1, false);
        throw std::runtime_error(std::string(where) + ": empty any");
    }
    if (a.type() != typeid(TzdValue)) {
        agentLog("B", where, "wrong any type", -1, -1, false);
        throw std::bad_any_cast();
    }
    return std::any_cast<TzdValue>(a);
}

static std::string getParamName(TzdLangParser::ParamContext* p) {
    if (!p) return "arg";
    if (p->IDENTIFIER()) return p->IDENTIFIER()->getText();
    if (p->T_INT()) return p->T_INT()->getText();
    if (p->T_STRING()) return p->T_STRING()->getText();
    if (p->T_FLOAT()) return p->T_FLOAT()->getText();
    if (p->T_BOOL()) return p->T_BOOL()->getText();
    if (p->T_VOID()) return p->T_VOID()->getText();
    if (p->T_PTR()) return p->T_PTR()->getText();
    if (p->KW_RET()) return p->KW_RET()->getText();
    return p->getText();
}

static std::string getParamType(TzdLangParser::ParamContext* p) {
    if (!p || !p->typeType()) return "";
    return p->typeType()->getText();
}

static void parseParamList(TzdLangParser::ParamListContext* paramList,
    std::vector<std::string>& names, std::vector<std::string>& types) {
    if (!paramList) return;
    for (auto p : paramList->param()) {
        names.push_back(getParamName(p));
        types.push_back(getParamType(p));
    }
}
// #endregion

HWND g_hPlotWnd = NULL;
Gdiplus::Bitmap* g_pGlobalBitmap = NULL;
std::mutex g_PlotMutex;

extern bool g_InJitCleanup;
TzdInterpreter* g_CurrentInterpreter = nullptr;
using JittedFunc = void (*)(void*, void*);

extern "C" TzdValue* g_LastJitValue;

bool IsUTF8(const std::string& str) {
    int i = 0;
    int nBytes = 0;
    unsigned char ch;
    bool bAllAscii = true;
    for (size_t i = 0; i < str.length(); i++) {
        ch = str[i];
        if ((ch & 0x80) != 0) bAllAscii = false;
        if (nBytes == 0) {
            if (ch >= 0x80) {
                if (ch >= 0xFC && ch <= 0xFD) nBytes = 6;
                else if (ch >= 0xF8) nBytes = 5;
                else if (ch >= 0xF0) nBytes = 4;
                else if (ch >= 0xE0) nBytes = 3;
                else if (ch >= 0xC0) nBytes = 2;
                else return false;
                nBytes--;
            }
        }
        else {
            if ((ch & 0xC0) != 0x80) return false;
            nBytes--;
        }
    }
    if (nBytes > 0) return false;
    if (bAllAscii) return true;
    return true;
}

LRESULT CALLBACK TzdWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        {
            std::lock_guard<std::mutex> lock(g_PlotMutex);
            if (g_pGlobalBitmap) {
                Gdiplus::Graphics graphics(hdc);
                graphics.DrawImage(g_pGlobalBitmap, 0, 0);
            }
        }
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_DESTROY:
        g_hPlotWnd = NULL;
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowPlotWindowThread() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = TzdWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"TzdPlotWnd";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    static bool registered = false;
    if (!registered) { RegisterClassW(&wc); registered = true; }

    g_hPlotWnd = CreateWindowExW(0, L"TzdPlotWnd", L"Tzd Tools - Plot Visualization",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 816, 639, NULL, NULL, wc.hInstance, NULL);

    ShowWindow(g_hPlotWnd, SW_SHOW);
    UpdateWindow(g_hPlotWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

std::string AnsiToUtf8(const std::string& str) {
#ifdef _WIN32
    if (str.empty()) return "";

    int wLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    if (wLen == 0) return str;

    std::wstring wStr(wLen, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wStr[0], wLen);

    int uLen = WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (uLen == 0) return str;

    std::string uStr(uLen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wStr.c_str(), -1, &uStr[0], uLen, nullptr, nullptr);

    if (!uStr.empty() && uStr.back() == '\0') uStr.pop_back();

    return uStr;
#else
    return str;
#endif
}

std::string Utf8ToAnsi(const std::string& utf8) {
    if (utf8.empty()) return "";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
    if (wlen <= 0) return "";
    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), &wstr[0], wlen);
    int alen = WideCharToMultiByte(CP_ACP, 0, wstr.data(), wlen, nullptr, 0, nullptr, nullptr);
    if (alen <= 0) return "";
    std::string ansi(alen, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.data(), wlen, &ansi[0], alen, nullptr, nullptr);
    return ansi;
}

double TzdInterpreter::getAsDouble(std::any value) {
    if (!value.has_value()) return 0.0;
    TzdValue v = (value.type() == typeid(TzdValue)) ? std::any_cast<TzdValue>(value) : TzdValue();

    switch (v.type) {
    case TzdValue::FLOAT: case TzdValue::DOUBLE: return v.dVal;
    case TzdValue::ULONG: return (double)v.ulVal;
    case TzdValue::BOOL:  return v.bVal ? 1.0 : 0.0;
    case TzdValue::POINTER: return (double)(uintptr_t)v.ptrVal;
    default: return (double)v.lVal;
    }
}

double TzdInterpreter::getAsDoubleInternal(const TzdValue& v) {
    switch (v.type) {
    case TzdValue::DOUBLE:
    case TzdValue::FLOAT:
        return v.dVal;

    case TzdValue::INT:
    case TzdValue::LONG:
        return (double)v.lVal;

    case TzdValue::BOOL:
        return v.bVal ? 1.0 : 0.0;

    case TzdValue::STRING: {
        try {
            // 尝试转换为 double
            return std::stod(v.sVal);
        }
        catch (...) {
            // 转换失败返回 0.0，这是 Safe Path 的要求
            return 0.0;
        }
    }

    case TzdValue::POINTER:
        return (double)(uintptr_t)v.ptrVal;

    case TzdValue::BIGINT: {
        try { return std::stod(v.sVal); }
        catch (...) { return v.sVal.empty() ? 0.0 : (bigint_is_neg(v.sVal) ? -1e308 : 1e308); }
    }

    case TzdValue::RATIONAL: {
        std::string num, den;
        rational_parse(v.sVal, num, den);
        try {
            double n = std::stod(num);
            double d = std::stod(den);
            return (d == 0.0) ? 0.0 : n / d;
        } catch (...) { return 0.0; }
    }

    default:
        return 0.0;
    }
}

bool TzdInterpreter::isTruthy(const TzdValue& v) {
    switch (v.type) {
    case TzdValue::NONE:
        return false;
    case TzdValue::BOOL:
        return v.bVal;
    case TzdValue::STRING:
        return !v.sVal.empty();
    case TzdValue::ARRAY:
        return !v.arrVal.empty();
    case TzdValue::MAP:
        return !v.mapVal.empty();
    case TzdValue::INSTANCE:
    case TzdValue::CLASS_DEF:
    case TzdValue::FUNCTION:
    case TzdValue::NATIVE_FUNCTION:
        return true;
    case TzdValue::BIGINT:
        return v.sVal != "0" && v.sVal != "-0";
    case TzdValue::RATIONAL: {
        std::string num, den;
        rational_parse(v.sVal, num, den);
        return num != "0" && num != "-0";
    }
    default:
        return getAsDoubleInternal(v) != 0.0;
    }
}

bool TzdInterpreter::valuesEqual(const TzdValue& l, const TzdValue& r) {
    if (l.type == r.type) {
        if (l.type == TzdValue::BOOL) return l.bVal == r.bVal;
        if (l.type == TzdValue::STRING) return l.sVal == r.sVal;
        if (l.type == TzdValue::BIGINT) return bigint_compare(l.sVal, r.sVal) == 0;
        if (l.type == TzdValue::RATIONAL) return rational_compare(l.sVal, r.sVal) == 0;
        if (l.type == TzdValue::FLOAT || l.type == TzdValue::DOUBLE) return l.dVal == r.dVal;
        if (l.type >= TzdValue::SBYTE && l.type <= TzdValue::ULONG) return l.lVal == r.lVal;
    }
    // Mixed BIGINT/RATIONAL with other numeric types
    if (l.type == TzdValue::BIGINT || r.type == TzdValue::BIGINT)
        return bigint_compare(to_bigint_str(l), to_bigint_str(r)) == 0;
    if (l.type == TzdValue::RATIONAL || r.type == TzdValue::RATIONAL)
        return rational_compare(to_rational_str(l), to_rational_str(r)) == 0;
    return getAsDoubleInternal(l) == getAsDoubleInternal(r);
}

std::string TzdInterpreter::getAsString(std::any value) {
    if (!value.has_value()) return "null";
    TzdValue v = (value.type() == typeid(TzdValue)) ? std::any_cast<TzdValue>(value) : TzdValue();
    if (v.type == TzdValue::NONE) return "null";

    switch (v.type) {
    case TzdValue::SBYTE: case TzdValue::BYTE:
    case TzdValue::SHORT: case TzdValue::USHORT:
    case TzdValue::INT:   case TzdValue::UINT:
    case TzdValue::LONG:  return std::to_string(v.lVal);
    case TzdValue::ULONG: return std::to_string(v.ulVal);
    case TzdValue::FLOAT: case TzdValue::DOUBLE: {
        std::string s = std::to_string(v.dVal);
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    }
    case TzdValue::POINTER: {
        std::stringstream ss;
        ss << "0x" << std::setw(16) << std::setfill('0') << std::hex << std::uppercase << (uintptr_t)v.ptrVal;
        return ss.str();
    }
    case TzdValue::STRING: return v.sVal;
    case TzdValue::BIGINT: return v.sVal;  // Decimal digit string, directly displayable
    case TzdValue::RATIONAL: return v.sVal; // "num/den" string, directly displayable
    case TzdValue::BOOL:   return v.bVal ? "true" : "false";
    case TzdValue::ARRAY: {
        if (v.arrVal.empty()) {
            return "[]";
        }

        // 1. 严格检查：数组内的每一个元素是否全都是真正的"字符"或"单字符字符串"
        bool isPureCharArray = true;
        for (const auto& item : v.arrVal) {
            if (item.type == TzdValue::STRING) {
                const std::string& s = item.sVal;
                int charCount = 0;
                for (size_t i = 0; i < s.length(); ++i) {
                    if ((static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
                        charCount++;
                    }
                }
                if (charCount != 1) {
                    isPureCharArray = false;
                    break;
                }
            }
            else if (item.type == TzdValue::SBYTE || item.type == TzdValue::BYTE) {
            }
            else {
                isPureCharArray = false;
                break;
            }
        }
        if (isPureCharArray) {
            std::string textResult;
            for (const auto& item : v.arrVal) {
                if (item.type == TzdValue::STRING) {
                    textResult += item.sVal; // 拼接单个汉字或英文字符
                }
                else {
                    char c = (item.type == TzdValue::SBYTE) ? (char)item.lVal : (char)item.ulVal;
                    if (c == '\0') break;
                    textResult.push_back(c);
                }
            }
            return textResult;
        }

        std::string res = "[";
        for (size_t i = 0; i < v.arrVal.size(); ++i) {
            res += getAsString(v.arrVal[i]);
            if (i < v.arrVal.size() - 1) res += ", ";
        }
        return res + "]";
    }
    case TzdValue::MAP: {
        std::string res = "{";
        bool first = true;
        for (auto const& [key, val] : v.mapVal) {
            if (!first) res += ", ";
            res += "\"" + key + "\": " + getAsString(val);
            first = false;
        }
        return res + "}";
    }
    case TzdValue::INSTANCE: {
        if (!v.instanceVal) return "null instance";
        std::string res = "Instance of " + (v.instanceVal->definition ? v.instanceVal->definition->fullName : "Unknown") + " {";
        bool first = true;
        if (v.instanceVal->definition) {
            for (auto const& [name, field] : v.instanceVal->definition->fields) {
                if (!first) res += ", ";
                res += name + ": " + getAsString(v.instanceVal->getMember(name));
                first = false;
            }
        }
        return res + "}";
    }

    case TzdValue::CLASS_DEF: return "[Class: " + (v.classDefVal ? v.classDefVal->fullName : "null") + "]";
    case TzdValue::FUNCTION: return "[Function: " + v.name + "]";
    case TzdValue::NATIVE_FUNCTION: return "[Native Function: " + v.name + "]";
    default: return "unknown";
    }
}

void TzdInterpreter::compileCurrentContext() {
    g_CurrentInterpreter = this;

    // 1. 初始化编译器
    TzdCompiler compiler(*m_jitEngine, "GlobalModule_" + std::to_string(rand()));
    compiler.setupExternalFunctions();

    bool needsCompile = false;
    std::vector<TzdValue*> pendingValues;

    for (auto& scope : scopes) {
        for (auto& [name, val] : scope) {
            if (val.type == TzdValue::FUNCTION && val.funcBody && !val.jittedPtr) {
                auto* funcCtx = dynamic_cast<TzdLangParser::FunctionDeclarationContext*>(val.funcBody->parent);
                if (funcCtx) {
                    compiler.visitFunctionDeclaration(funcCtx);
                    needsCompile = true;
                    pendingValues.push_back(&val);
                }
            }
        }
    }

    if (needsCompile) {
        auto TSM = compiler.extractThreadSafeModule();

        if (TSM) {
            m_jitEngine->addModule(std::move(TSM));

            for (auto* valPtr : pendingValues) {
                auto symOrErr = m_jitEngine->lookupSymbol(valPtr->name);
                if (!symOrErr) {
                    consumeError(symOrErr.takeError());
                    continue;
                }

                llvm::orc::ExecutorAddr execAddr = *symOrErr; 
                auto rawAddr = execAddr.getValue();
                valPtr->jittedPtr = reinterpret_cast<void(*)(void*, void*)>(rawAddr);
            }
        }

    }
}

/**
 * 编译指定的脚本代码（字符串）并立即执行或载入内存
 */
void TzdInterpreter::compileScriptToMemory(const std::string& code) {
    g_CurrentInterpreter = this;
    auto _t0 = std::chrono::steady_clock::now();
    antlr4::ANTLRInputStream input(code);
    TzdLangLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();
    auto _t1 = std::chrono::steady_clock::now();
    TzdLangParser parser(&tokens);
    auto* tree = parser.program();
    auto _t2 = std::chrono::steady_clock::now();
    TzdCompiler compiler(*m_jitEngine, "DirectScript_" + std::to_string(rand()));
    compiler.setupExternalFunctions();
    compiler.visit(tree);
    auto TSM = compiler.extractThreadSafeModule();
    auto _t3 = std::chrono::steady_clock::now();

    if (m_antlrTiming) {
        fprintf(stderr, "[ANTLR Timing] compileScriptToMemory (size: %zu bytes)\n", code.size());
        fprintf(stderr, "  Lexer+Tokens:  %8.1f ms\n", std::chrono::duration<double, std::milli>(_t1 - _t0).count());
        fprintf(stderr, "  Parser:        %8.1f ms\n", std::chrono::duration<double, std::milli>(_t2 - _t1).count());
        fprintf(stderr, "  Compile+Visit: %8.1f ms\n", std::chrono::duration<double, std::milli>(_t3 - _t2).count());
    }

    if (TSM) {
        m_jitEngine->addModule(std::move(TSM));
    }
    else {
        std::cerr << "[Error] Failed to extract ThreadSafeModule in compileScriptToMemory" << std::endl;
    }
}

/**
 * 编译指定名称的函数
 */
void TzdInterpreter::compileFunctionToMemory(const std::string& funcName) {
    g_CurrentInterpreter = this;
    TzdValue& val = scopes.back()[funcName];

    if (val.type == TzdValue::FUNCTION && val.funcBody) {
        TzdCompiler compiler(*m_jitEngine, "Mod_" + funcName);
        auto* funcCtx = dynamic_cast<TzdLangParser::FunctionDeclarationContext*>(val.funcBody->parent);
        if (funcCtx) {
            compiler.visitFunctionDeclaration(funcCtx);
            auto TSM = compiler.extractThreadSafeModule();
            if (TSM) {
                m_jitEngine->addModule(std::move(TSM));

                auto sym = m_jitEngine->lookupSymbol(funcName);
                if (sym) {
                    val.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(sym->getValue());
                }
            }
        }
    }
}


static std::vector<std::string> split_by_dot(const std::string& name) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = name.find('.');

    while (end != std::string::npos) {
        parts.push_back(name.substr(start, end - start));
        start = end + 1;
        end = name.find('.', start);
    }
    // 最后一个部分
    parts.push_back(name.substr(start));
    return parts;
}

TzdValue TzdInterpreter::getVariable(const std::string& name, antlr4::ParserRuleContext* ctx) {

    // **优化 2: 简单变量名快速路径**
    // 假设不含 '.' 的简单变量名查找是最常见的操作。
    size_t dot_pos = name.find('.');
    if (dot_pos == std::string::npos) {

        // --- 简单变量名查找逻辑 ---

        // 查找局部/全局作用域
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            // **优化 3: find() 代替 count() + []**
            auto scope_it = it->find(name);
            if (scope_it != it->end()) {
                return scope_it->second; // 最快路径：直接返回
            }
        }

        // 查找 this 实例成员
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto thisIt = it->find("this");
            if (thisIt != it->end() && thisIt->second.type == TzdValue::INSTANCE) {
                TzdInstance* inst = thisIt->second.instanceVal;
                try {
                    return inst->getMember(name);
                }
                catch (...) {
                    // 忽略异常，继续查找
                }
            }
        }

        // 查找 ClassDef
        TzdClassDef* cls = TzdOopManager::getClass(name);
        if (cls) {
            return TzdValue(cls);
        }

        // 未定义
        throw TzdRuntimeException("未定义的标识符: '" + name + "'", ctx ? ctx->getStart() : nullptr);
    }

    // ---------------------------------------------------------------------
    // 复杂路径：name 包含 '.' (例如：obj.member.submember)
    // ---------------------------------------------------------------------

    std::vector<std::string> parts = split_by_dot(name);

    TzdValue current;
    bool foundBase = false;
    // **优化 4: 使用引用** 避免拷贝 parts[0]
    std::string& baseName = parts[0];

    // 1. 查找 baseName
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        // **优化 3: find() 代替 count() + []**
        auto scope_it = it->find(baseName);
        if (scope_it != it->end()) {
            current = scope_it->second;
            foundBase = true;
            break;
        }
    }

    if (!foundBase) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto thisIt = it->find("this");
            if (thisIt != it->end() && thisIt->second.type == TzdValue::INSTANCE) {
                TzdInstance* inst = thisIt->second.instanceVal;
                try {
                    current = inst->getMember(baseName);
                    foundBase = true;
                    break;
                }
                catch (...) {
                }
            }
        }
    }

    if (!foundBase) {
        TzdClassDef* cls = TzdOopManager::getClass(baseName);
        if (cls) {
            current = TzdValue(cls);
            foundBase = true;
        }
    }

    if (!foundBase) {
        throw TzdRuntimeException("未定义的标识符: '" + baseName + "'", ctx ? ctx->getStart() : nullptr);
    }

    for (size_t i = 1; i < parts.size(); ++i) {
        const std::string& memberName = parts[i];

        if (current.type == TzdValue::INSTANCE) {
            current = current.instanceVal->getMember(memberName);
        }
        else if (current.type == TzdValue::CLASS_DEF) {
            TzdClassDef* cls = current.classDefVal;
            auto static_it = cls->staticValues.find(memberName);
            if (static_it != cls->staticValues.end()) {
                current = static_it->second;
            }
            else {
                ClassMethod* m = cls->findMethod(memberName);
                if (m && m->isStatic) {
                    current.type = TzdValue::FUNCTION;
                    current.name = m->name;
                    current.params = m->params;
                    current.funcBody = m->body;
                    current.instanceVal = nullptr;
                }
                else {
                    throw TzdRuntimeException("类 '" + cls->fullName + "' 中不存在静态成员: " + memberName, ctx->getStart());
                }
            }
        }
        else {
            throw TzdRuntimeException("基础类型 '" + getAsString(current) + "' 无法访问成员 '" + memberName + "'", ctx->getStart());
        }
    }

    return current;
}

void TzdInterpreter::setVariable(const std::string& name, TzdValue val) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(name)) {
            (*it)[name] = val;
            return;
        }
    }

    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count("this")) {
            TzdValue thisVal = (*it)["this"];
            if (thisVal.type == TzdValue::INSTANCE) {
                try {
                    thisVal.instanceVal->setMember(name, val);
                    return;
                }
                catch (...) {}
            }
        }
    }

    scopes.back()[name] = val;
}

void TzdInterpreter::loadScript(std::string code) {
    // --- 1. 状态保存 (支持递归 import 的核心) ---
    // 保存当前正在编译的 Module 环境，防止被子脚本的 loadScript 覆盖
    auto savedCompiler = std::move(m_compiler);
    auto savedPending = std::move(m_pendingJitFunctions);
    auto savedJitMap = std::move(m_jitNameToUserMap);

    // --- 2. 初始化当前脚本的新编译器上下文 ---
    // 注意：不再在这里调用 prepareForScript()，因为它会彻底清空状态
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "Module_" + std::to_string(rand()));
    m_compiler->setupExternalFunctions();
    m_pendingJitFunctions.clear();
    m_jitNameToUserMap.clear();

    // --- 3. 编码处理与 ANTLR 初始化 ---
    std::string utf8Code = IsUTF8(code) ? code : AnsiToUtf8(code);
    m_currentSource = utf8Code;

    // Timing helper for --antlrTime
    #define TZD_ANTLR_TIMER(phase_name) \
        if (m_antlrTiming) { \
            auto _end = std::chrono::steady_clock::now(); \
            double _ms = std::chrono::duration<double, std::milli>(_end - _start).count(); \
            fprintf(stderr, "[ANTLR Timing] %-20s %10.1f ms  (script size: %zu bytes)\n", phase_name, _ms, utf8Code.size()); \
            _start = _end; \
        }

    auto _start = std::chrono::steady_clock::now();

    ScriptModule* mod = new ScriptModule();
    mod->input = new antlr4::ANTLRInputStream(utf8Code);
    mod->lexer = new TzdLangLexer(mod->input);
    mod->tokens = new antlr4::CommonTokenStream(mod->lexer);
    mod->parser = new TzdLangParser(mod->tokens);

    // Force token stream to fill — this triggers the actual lexing
    mod->tokens->fill();
    TZD_ANTLR_TIMER("Lexer+TokenStream")

    TzdErrorListener err;
    mod->parser->removeErrorListeners();
    mod->parser->addErrorListener(&err);
    mod->tree = mod->parser->program();
    size_t _tokenCount = mod->tokens->getNumberOfOnChannelTokens();
    TZD_ANTLR_TIMER("Parser (program)")

    if (mod->parser->getNumberOfSyntaxErrors() > 0) {
        delete mod;
        // 恢复父级编译器状态并退出
        m_compiler = std::move(savedCompiler);
        m_pendingJitFunctions = std::move(savedPending);
        m_jitNameToUserMap = std::move(savedJitMap);
        throw std::runtime_error("脚本包含语法错误，未加载。");
    }

    loadedModules.push_back(mod);
    initNativeFunctions(); // 确保内建函数可用

    // Compile to bytecode for faster function execution (non-JIT path).
    // When m_forceInterpreter (--interpreter / --tree-walk) is set, the
    // bytecode VM must NOT engage; loadScript() must not reset the flag.
    if (m_forceInterpreter) {
        m_useBytecodeVM = false;
    } else {
        try {
            TzdBytecodeCompiler bc;
            m_bytecodeModule = std::make_unique<BytecodeModule>(bc.compile(mod->tree, utf8Code));
            m_bytecodeVM = std::make_unique<TzdBytecodeVM>(this);
            m_useBytecodeVM = true;
        } catch (...) {
            m_useBytecodeVM = false;
        }
    }
    TZD_ANTLR_TIMER("Bytecode compile")

    // --- 4. 执行访问（编译）与 JIT ---
    try {
        // Set g_CurrentInterpreter so JIT compilation can register nested functions
        g_CurrentInterpreter = this;
        // visit 过程中如果遇到 import，会递归调用 loadScript，
        // 由于我们上面做了"状态保存"，所以递归是安全的。
        std::any result = this->visit(mod->tree);

        // 【关键点】：脚本访问完毕后，立即编译并提取当前模块的机器码
        // 这能解决主脚本找不到 import 脚本函数的问题
        jitPendingModule();
        if (m_antlrTiming) {
            auto _end = std::chrono::steady_clock::now();
            double _ms = std::chrono::duration<double, std::milli>(_end - _start).count();
            fprintf(stderr, "[ANTLR Timing] %-20s %10.1f ms  (tokens: %zu, statements: %zu)\n",
                    "Visit+JIT", _ms, _tokenCount, mod->tree->statement().size());
        }

        #undef TZD_ANTLR_TIMER

        // 仅在 REPL 顶层或有明确返回值时打印结果
        if (result.has_value() && result.type() == typeid(TzdValue)) {
            TzdValue val = std::any_cast<TzdValue>(result);
            if (val.type != TzdValue::NONE) {
                std::string outStr = getAsString(val);
                std::cout << ">>> ";
                if (val.type == TzdValue::STRING) {
                    std::cout << "\"" << Utf8ToAnsi(outStr) << "\"" << std::endl;
                }
                else {
                    std::cout << Utf8ToAnsi(outStr) << std::endl;
                }
            }
        }
    }
    catch (const TzdRuntimeException& e) {
        // 传入 e.stackTrace
        TzdErrorHandler::report("Tzd 运行时错误", e.line, e.column, e.what(), utf8Code, e.stackTrace);
    }
    catch (const TzdThrowException& e) {
        std::string msg = "Unknown error";
        if (e.value.type == TzdValue::INSTANCE && e.value.instanceVal) {
            try {
                TzdValue toStr = e.value.instanceVal->getMember("message");
                if (toStr.type == TzdValue::STRING && !toStr.sVal.empty()) msg = toStr.sVal;
            }
            catch (...) {}
        }
        else if (e.value.type == TzdValue::STRING) {
            msg = e.value.sVal;
        }
        // 传入 e.stackTrace
        TzdErrorHandler::report("Tzd 未捕获异常", 0, 0, msg, utf8Code, e.stackTrace);
    }
    catch (const std::exception& e) {
        TzdErrorHandler::report("系统异常", 0, 0, e.what(), "");
    }

    m_compiler = std::move(savedCompiler);
    m_pendingJitFunctions = std::move(savedPending);
    m_jitNameToUserMap = std::move(savedJitMap);
}

void TzdInterpreter::loadScriptFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) throw std::runtime_error("无法打开文件: " + filePath);

    std::stringstream ss;
    ss << file.rdbuf();
    std::string code = ss.str();

    m_scriptPathStack.push_back(fs::absolute(filePath));

    if (code.size() > 50000 && bigint_gpu_suitable(0)) {
        bigint_gpu_warmup(2097152);
    }

    try {
        this->loadScript(code);
    }
    catch (...) {
        m_scriptPathStack.pop_back();
        throw;
    }

    m_scriptPathStack.pop_back();
}

std::string TzdInterpreter::resolveImportPath(const std::string& inputPath) {
    fs::path target;

    // 1. 处理点号表示法: xxx.yyy -> xxx/yyy.tzd
    std::string processedPath = inputPath;
    if (inputPath.find('/') == std::string::npos && inputPath.find('\\') == std::string::npos && inputPath.find('.') != std::string::npos) {
        std::replace(processedPath.begin(), processedPath.end(), '.', '/');
        processedPath += ".tzd";
    }

    // 2. 尝试相对于当前脚本的路径
    if (!m_scriptPathStack.empty()) {
        fs::path currentDir = m_scriptPathStack.back().parent_path();
        target = currentDir / processedPath;
        // 【修改】：使用 weakly_canonical 确保路径规范化（消除 .. 和斜杠差异）
        if (fs::exists(target)) return fs::weakly_canonical(fs::absolute(target)).string();
    }

    // 3. 遍历用户定义的包含路径
    for (const auto& includeDir : m_includePaths) {
        target = fs::path(includeDir) / processedPath;
        if (fs::exists(target)) return fs::weakly_canonical(fs::absolute(target)).string();
    }

    // 4. 尝试作为绝对路径或直接相对路径
    target = fs::path(processedPath);
    if (fs::exists(target)) return fs::weakly_canonical(fs::absolute(target)).string();

    return ""; // 未找到
}

void TzdInterpreter::initNativeFunctions() {
    TzdNativeModule::init(this);
}

void TzdInterpreter::internalRenderPlot(const std::vector<TzdValue>& functions, double start, double end, double step) {
    RGBABitmapImageReference* imageReference = CreateRGBABitmapImageReference();

    // 颜色表
    struct MyColor { float r, g, b; Gdiplus::Color gdiColor; };
    std::vector<MyColor> colorTable = {
        // 1. 蓝色 (Modern Blue)
        {0.12f, 0.47f, 0.71f, Gdiplus::Color(255, 31, 119, 180)},

        // 2. 橙色 (Orange)
        {1.00f, 0.50f, 0.05f, Gdiplus::Color(255, 255, 127, 14)},

        // 3. 绿色 (Modern Green)
        {0.17f, 0.63f, 0.17f, Gdiplus::Color(255, 44, 160, 44)},

        // 4. 红色 (Modern Red)
        {0.84f, 0.15f, 0.16f, Gdiplus::Color(255, 214, 39, 40)},

        // 5. 紫色 (Purple)
        {0.58f, 0.40f, 0.74f, Gdiplus::Color(255, 148, 103, 189)},

        // 6. 棕色 (Brown)
        {0.55f, 0.34f, 0.29f, Gdiplus::Color(255, 140, 86, 75)},

        // 7. 粉紫色 (Pink)
        {0.89f, 0.47f, 0.76f, Gdiplus::Color(255, 227, 119, 194)},

        // 8. 灰色 (Gray)
        {0.50f, 0.50f, 0.50f, Gdiplus::Color(255, 127, 127, 127)},

        // 9. 黄绿色 (Olive)
        {0.74f, 0.74f, 0.13f, Gdiplus::Color(255, 188, 189, 34)},

        // 10. 青色 (Cyan)
        {0.09f, 0.75f, 0.81f, Gdiplus::Color(255, 23, 190, 207)}
    };

    double xPadding = 70.0; // 必须和你设置的 settings->xPadding 一致
    double canvasW = 850.0; // 必须和你设置的 settings->width 一致
    double plotWidth = canvasW - (xPadding * 2.0);
    double unitPerPixel = (end - start) / plotWidth;

    // 算出屏幕最左边和最右边对应的数学 X 值
    double extendedStart = start - (xPadding * unitPerPixel);
    double extendedEnd = end + (xPadding * unitPerPixel);

    std::vector<ScatterPlotSeries*> allSeries;
    for (size_t fIdx = 0; fIdx < functions.size(); ++fIdx) {
        auto xs = new std::vector<double>();
        auto ys = new std::vector<double>();

        for (double val = extendedStart; val <= extendedEnd; val += step) {
            std::unordered_map<std::string, TzdValue> callScope;
            if (!functions[fIdx].params.empty()) {
                callScope[functions[fIdx].params[0]] = TzdValue(val);
            }
            this->scopes.push_back(callScope);

            TzdValue res(0.0);
            try {
                if (functions[fIdx].type == TzdValue::NATIVE_FUNCTION) {
                    res = functions[fIdx].nativeFunc({ TzdValue(val) });
                }
                else {
                    std::any v = this->visit(functions[fIdx].funcBody);
                    if (v.has_value() && v.type() == typeid(TzdValue)) res = std::any_cast<TzdValue>(v);
                }
            }
            catch (const TzdReturnException& e) { res = e.value; }
            catch (...) { res = TzdValue(0.0); }

            this->scopes.pop_back();
            xs->push_back(val);
            ys->push_back(res.dVal);
        }

        ScatterPlotSeries* series = GetDefaultScatterPlotSeriesSettings();
        series->xs = xs; series->ys = ys;
        series->linearInterpolation = true;
        auto& c = colorTable[fIdx % colorTable.size()];
        series->color = new RGBA{ c.r, c.g, c.b, 1.0 };
        allSeries.push_back(series);
    }

    ScatterPlotSettings* settings = GetDefaultScatterPlotSettings();
    settings->width = 850; settings->height = 600;
    settings->xPadding = 70;
    settings->autoBoundaries = true;
    settings->scatterPlotSeries = &allSeries;

    StringReference* errorMessage = CreateStringReferenceLengthValue(0, L' ');
    if (DrawScatterPlotFromSettings(imageReference, settings, errorMessage)) {
        std::vector<double>* pngdata = ConvertToPNG(imageReference->image);
        WriteToFile(pngdata, "tzd_temp.png");

        Gdiplus::Bitmap* localPreparedBmp = nullptr;
        {
            Gdiplus::Image* base = Gdiplus::Image::FromFile(L"tzd_temp.png");
            if (base && base->GetLastStatus() == Gdiplus::Ok) {
                localPreparedBmp = new Gdiplus::Bitmap(base->GetWidth(), base->GetHeight());
                Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(localPreparedBmp);
                g->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
                g->Clear(Gdiplus::Color::White);
                g->DrawImage(base, 0, 0);

                Gdiplus::Font font(L"Consolas", 10, Gdiplus::FontStyleBold);
                Gdiplus::SolidBrush textB(Gdiplus::Color(255, 40, 40, 40));

                for (size_t i = 0; i < functions.size(); ++i) {
                    Gdiplus::Pen p(colorTable[i % colorTable.size()].gdiColor, 3);
                    int yPos = 65 + (int)i * 25;
                    g->DrawLine(&p, 630, yPos, 660, yPos);

                    // --- 使用真正的函数名 ---
                    std::string nameStr = functions[i].name;
                    if (nameStr.empty()) nameStr = "unnamed";

                    std::wstring lbl = std::wstring(nameStr.begin(), nameStr.end()) + L"(x)";
                    g->DrawString(lbl.c_str(), -1, &font, Gdiplus::PointF(665, (float)yPos - 8), &textB);
                }
                delete g;
            }
            if (base) delete base;
        }

        if (localPreparedBmp) {
            std::lock_guard<std::mutex> lock(g_PlotMutex);
            if (g_pGlobalBitmap) delete g_pGlobalBitmap;
            g_pGlobalBitmap = localPreparedBmp;
        }

        // 窗口显示与刷新逻辑
        if (g_hPlotWnd == NULL) {
            std::thread(ShowPlotWindowThread).detach();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        if (g_hPlotWnd != NULL) {
            InvalidateRect(g_hPlotWnd, NULL, FALSE);
            // 立即强制重绘以减少白屏感
            SendMessage(g_hPlotWnd, WM_PAINT, 0, 0);
        }
    }

    // 清理内存
    for (auto s : allSeries) {
        delete s->xs; delete s->ys;
        if (s->color) delete s->color;
    }
    FreeAllocations();
}

void TzdInterpreter::setGlobalVariable(const std::string& name, const TzdValue& val)
{
    {
        if (!scopes.empty()) {
            scopes[0][name] = val;
        }
    }
}

void TzdInterpreter::mapJitSymbolsToValue() {
    for (auto& scope : scopes) {
        for (auto& [name, val] : scope) {
            if (val.type == TzdValue::FUNCTION) {
                void* addr = reinterpret_cast<void(*)(void*, void*)>((m_jitEngine->lookupSymbol(name))->getValue());
                if (addr) {
                    val.jittedPtr = (void(*)(void*, void*))addr;
                }
            }
        }
    }
}

// RAII guard for the interpreter call stack + debug file stack. Lives at file
// scope so that both callFunction() and callScriptFunction() can share it.
struct TzdCallStackGuard {
    TzdInterpreter* self;
    explicit TzdCallStackGuard(TzdInterpreter* interp, const std::string& frame, const std::string& sourceFile) : self(interp) {
        self->m_callStackFrames.push_back(frame);
        self->m_debugFileStack.push_back(sourceFile);
    }
    ~TzdCallStackGuard() {
        if (!self->m_callStackFrames.empty()) self->m_callStackFrames.pop_back();
        if (!self->m_debugFileStack.empty()) self->m_debugFileStack.pop_back();
    }
};

TzdValue TzdInterpreter::callFunction(const TzdValue& func, const std::vector<TzdValue>& args) {
    // 1. 类型校验 (no stack frame yet — a type error is not a runtime fault
    //    inside a function body and should not pollute the trace).
    if (func.type != TzdValue::FUNCTION && func.type != TzdValue::NATIVE_FUNCTION) {
        throw std::runtime_error("尝试调用一个非函数对象");
    }

    // Build the stack frame label.
    std::string fileLoc = formatSourcePath(func.sourceFile);
    if (func.line > 0) {
        fileLoc += ":" + std::to_string(func.line);
    }
    std::string stackFrame = func.name.empty() ? "<anonymous>" : func.name;
    if (func.instanceVal && func.instanceVal->definition) {
        stackFrame = func.instanceVal->definition->fullName + "." + func.name;
    }
    stackFrame += " (" + fileLoc + ")";
    if (func.type == TzdValue::NATIVE_FUNCTION) stackFrame += " (Native Method)";
    else if (func.type == TzdValue::FUNCTION && func.jittedPtr) stackFrame += " (JIT Compiled)";
    else stackFrame += " (Interpreted)";

    TzdCallStackGuard stackGuard(this, stackFrame, func.sourceFile);

    // 参数个数与类型校验 (针对脚本函数；原生函数跳过)。必须在字节码/JIT/解释
    // 分支之前执行，以保持与原有行为一致。
    if (func.type == TzdValue::FUNCTION) {
        if (args.size() != func.params.size()) {
            throw std::runtime_error("函数 '" + func.name + "' 参数不匹配: 期望 " +
                std::to_string(func.params.size()) + " 个，实际 " +
                std::to_string(args.size()) + " 个");
        }
        for (size_t i = 0; i < args.size(); ++i) {
            if (i < func.paramTypes.size() && !func.paramTypes[i].empty() &&
                !checkParamValueType(func.paramTypes[i], args[i])) {
                throw std::runtime_error(
                    "函数 '" + func.name + "' 第 " + std::to_string(i + 1) +
                    " 个参数类型不匹配: 期望 " + func.paramTypes[i]);
            }
        }
    }

    // --- 分支 B: 原生 C++ 函数 ---
    if (func.type == TzdValue::NATIVE_FUNCTION) {
        return func.nativeFunc(args);
    }

    // --- 分支 C: JIT 机器码执行 (如果已生成机器码且未被禁用，最高优先级直接执行) ---
    // Note: m_noJit blocks EAGER compilation, but if jittedPtr is already set
    // (by tryJitCompile/bytecode JIT bridge), we should use it regardless.
    if (func.jittedPtr && !TzdDebugger::g_DebugActive) {
        return callScriptFunction(func.name, func.params, func.paramTypes,
            func.funcBody, func.jittedPtr, func.instanceVal, args,
            func.sourceFile, func.line);
    }

    // --- 分支 D: 字节码 VM 快速路径 ---
    // IMPORTANT: only dispatch free (non-bound) functions by name. A bound
    // method (instanceVal != nullptr) must never be routed to an unrelated
    // free function that happens to share its name — it falls through to the
    // script-function core which executes func.funcBody directly.
    if (m_useBytecodeVM && m_bytecodeModule && m_bytecodeVM && func.instanceVal == nullptr) {
        auto it = m_bytecodeModule->funcIndex.find(func.name);
        if (it != m_bytecodeModule->funcIndex.end()) {
            return m_bytecodeVM->callFunction(*m_bytecodeModule, func.name, args);
        }
    }

    // --- 分支 E: 脚本函数 (解释执行)，走公共核心 ---
    return callScriptFunction(func.name, func.params, func.paramTypes,
        func.funcBody, func.jittedPtr, func.instanceVal, args,
        func.sourceFile, func.line);
}

TzdValue TzdInterpreter::callMethod(ClassMethod& method, TzdInstance* receiver,
    const std::vector<TzdValue>& args) {
    // Build the stack frame label.
    std::string fileLoc = formatSourcePath(method.sourceFile);
    if (method.line > 0) {
        fileLoc += ":" + std::to_string(method.line);
    }
    std::string stackFrame = method.name.empty() ? "<anonymous>" : method.name;
    if (receiver && receiver->definition) {
        stackFrame = receiver->definition->fullName + "." + method.name;
    }
    stackFrame += " (" + fileLoc + ")";
    if (method.isNative) stackFrame += " (Native Method)";
    else if (method.jittedPtr) stackFrame += " (JIT Compiled)";
    else stackFrame += " (Interpreted)";

    TzdCallStackGuard stackGuard(this, stackFrame, method.sourceFile);

    // 参数个数与类型校验 (脚本方法；原生方法跳过)。
    if (!method.isNative) {
        if (args.size() != method.params.size()) {
            throw std::runtime_error("函数 '" + method.name + "' 参数不匹配: 期望 " +
                std::to_string(method.params.size()) + " 个，实际 " +
                std::to_string(args.size()) + " 个");
        }
        for (size_t i = 0; i < args.size(); ++i) {
            if (i < method.paramTypes.size() && !method.paramTypes[i].empty() &&
                !checkParamValueType(method.paramTypes[i], args[i])) {
                throw std::runtime_error(
                    "函数 '" + method.name + "' 第 " + std::to_string(i + 1) +
                    " 个参数类型不匹配: 期望 " + method.paramTypes[i]);
            }
        }
    }

    // --- 原生方法 ---
    if (method.isNative) {
        return method.nativeWrapper(args);
    }

    // --- 脚本方法 (JIT 或解释执行)，走公共核心 ---
    return callScriptFunction(method.name, method.params, method.paramTypes,
        method.body, method.jittedPtr, receiver, args,
        method.sourceFile, method.line);
}

// Common script-function execution core shared by callFunction() and
// callMethod(). Takes the actual receiver separately (not embedded in a bound
// TzdValue) and reads metadata by reference — no template copy. Handles
// arg/type checks, stack traces, debugger fallback (skip JIT when
// g_DebugActive), JIT behaviour and the tree-walk interpreter path. Scope
// unwinding is exception-safe for every path (catch(...) pops before rethrow).
TzdValue TzdInterpreter::callScriptFunction(const std::string& name,
    const std::vector<std::string>& params,
    const std::vector<std::string>& paramTypes,
    TzdLangParser::BlockContext* funcBody,
    void (*jittedPtr)(void*, void*),
    TzdInstance* receiver,
    const std::vector<TzdValue>& args,
    const std::string& sourceFile,
    int line) {

    // Arg/type checks are performed by the callers (callFunction / callMethod)
    // before dispatching to this core, so that the bytecode VM and native
    // paths also honour them consistently.

    // --- 分支 A: JIT 机器码执行 (调试器激活时回退到解释执行) ---
    if (jittedPtr && !TzdDebugger::g_DebugActive) {
        this->clearJitError();
        this->m_jitUnhandledThrow.reset(); // 清除上一次遗留的异常
        g_CurrentInterpreter = this;
        g_LastJitValue = nullptr;

        std::vector<TzdValue> jitArgs;
        g_JitPool.reset();
        if (receiver != nullptr) {
            jitArgs.push_back(TzdValue(receiver));
        }
        jitArgs.insert(jitArgs.end(), args.begin(), args.end());

        this->m_currentArgs = jitArgs;
        this->m_argPtrStack.push_back(this->m_currentArgs.data());

        std::unordered_map<std::string, TzdValue> jitScope;
        if (receiver) jitScope["this"] = TzdValue(receiver);
        scopes.push_back(jitScope);

        TzdValue result;
        try {
            jittedPtr(this, &result);

            if (this->m_jitUnhandledThrow) {
                auto thrown = *this->m_jitUnhandledThrow;
                this->m_jitUnhandledThrow.reset();
                throw thrown;
            }

            if (this->m_hasJitError) {
                TzdRuntimeException ex(m_lastJitError, nullptr, m_jitErrorTrace);
                ex.line = this->m_jitLine;
                ex.column = this->m_jitColumn;
                throw ex;
            }

            if (g_LastJitValue != nullptr) {
                result = *g_LastJitValue;
                g_LastJitValue = nullptr;
            }
        }
        catch (...) {
            scopes.pop_back();
            this->m_argPtrStack.pop_back();
            this->m_currentArgs.clear();
            throw;
        }

        scopes.pop_back();
        this->m_argPtrStack.pop_back();
        this->m_currentArgs.clear();

        if (scopes.size() <= 1) clearJitMemory();
        return result;
    }

    // --- 分支 B: 解释执行 (JIT 未就绪、重定义后尚未链接或调试器激活) ---
    std::unordered_map<std::string, TzdValue> callScope;
    if (receiver != nullptr) callScope["this"] = TzdValue(receiver);
    for (size_t i = 0; i < params.size(); ++i) {
        callScope[params[i]] = args[i];
    }

    scopes.push_back(callScope);
    TzdValue result;
    try {
        if (funcBody) {
            std::any res = visit(funcBody);
            if (res.has_value() && res.type() == typeid(TzdValue))
                result = std::any_cast<TzdValue>(res);
        }
    }
    catch (const TzdReturnException& e) { result = e.value; }
    catch (...) {
        // Exception-safe scope unwinding for *every* escaping exception
        // (TzdThrowException, TzdRuntimeException, TzdBreakException, ...).
        scopes.pop_back();
        throw;
    }
    scopes.pop_back();

    // Auto-collect: lightweight check, only gather roots when threshold exceeded
    if (TzdGarbageCollector::getInstance().shouldCollect() && !scopes.empty()) {
        std::vector<TzdValue*> roots;
        for (auto& [k, v] : scopes[0]) roots.push_back(&v);
        TzdGarbageCollector::getInstance().collectIfNeeded(roots);
    }

    // Tiering: record invocation count for hotspot detection
    if (!name.empty()) {
        TzdTieringEngine::getInstance().recordInvocation(name);
    }

    return result;
}

// --- 程序结构 ---
std::any TzdInterpreter::visitProgram(TzdLangParser::ProgramContext* ctx) {
    std::any lastValue;

    // Phase 1: 只执行"声明类语句"（用于完成函数/类的编译与符号注册）。
    // 然后立即 jitPendingModule()，确保 Phase 2 中的第一次调用就能走 JIT。
    std::vector<TzdLangParser::StatementContext*> execStmts;
    for (auto stmt : ctx->statement()) {
        bool isJitDeclaration =
            dynamic_cast<TzdLangParser::FunDeclStmtContext*>(stmt) ||
            dynamic_cast<TzdLangParser::ClassDeclStmtContext*>(stmt) ||
            dynamic_cast<TzdLangParser::NativeFunDeclStmtContext*>(stmt) ||
            dynamic_cast<TzdLangParser::ImportStmtContext*>(stmt);

        if (isJitDeclaration) {
            try {
                lastValue = visit(stmt);
            }
            catch (const TzdReturnException& e) { return e.value; }
        }
        else {
            execStmts.push_back(stmt);
        }
    }

    // 将当前编译器里 pending 的符号链接到 jittedPtr（全局函数 + 类方法）。
    jitPendingModule();

    // Phase 2: 执行所有其余语句（例如顶层的 test(); / runBenchmark(); 调用）。
    for (auto stmt : execStmts) {
        try {
            lastValue = visit(stmt);
        }
        catch (const TzdReturnException& e) { return e.value; }
    }
    return lastValue;
}

std::any TzdInterpreter::visitBlock(TzdLangParser::BlockContext* ctx) {
    std::any lastValue;
    for (auto stmt : ctx->statement()) {
        try {
            lastValue = visit(stmt);
        }
        catch (const TzdBreakException&) { throw; }
        catch (const TzdContinueException&) { throw; }
        catch (const TzdReturnException&) { throw; }
        catch (const TzdThrowException&) { throw; }
    }
    return lastValue;
}

std::any TzdInterpreter::visitThrowStmt(TzdLangParser::ThrowStmtContext* ctx) {
    TzdValue errVal = castAnyToTzdValue(visit(ctx->expression()), "visitThrowStmt");

    auto trace = this->m_callStackFrames;
    trace.push_back("<throw> at line " + std::to_string(ctx->getStart()->getLine()));

    throw TzdThrowException(std::move(errVal), trace);
}

std::any TzdInterpreter::visitTryCatchStmt(TzdLangParser::TryCatchStmtContext* ctx) {
    std::string catchName = ctx->IDENTIFIER()->getText();
    try {
        visit(ctx->block(0));
    }
    catch (const TzdThrowException& e) {
        std::unordered_map<std::string, TzdValue> catchScope;
        catchScope[catchName] = e.value;
        scopes.push_back(catchScope);
        try {
            visit(ctx->block(1));
        }
        catch (const TzdBreakException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdContinueException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdReturnException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdThrowException&) {
            scopes.pop_back();
            throw;
        }
        scopes.pop_back();
    }
    return TzdValue();
}

std::any TzdInterpreter::visitArrayLiteralExpr(TzdLangParser::ArrayLiteralExprContext* ctx) {
    std::vector<TzdValue> elements;
    std::string c_elements = "";

    if (ctx->exprList()) {
        auto exprs = ctx->exprList()->expression();
        for (size_t i = 0; i < exprs.size(); ++i) {
            TzdValue ev = std::any_cast<TzdValue>(visit(exprs[i]));
            elements.push_back(ev);
        }
    }

    TzdValue res(elements);
    return res;
}

std::any TzdInterpreter::visitIndexExpr(TzdLangParser::IndexExprContext* ctx) {
    TzdValue container = std::any_cast<TzdValue>(visit(ctx->expression(0)));
    TzdValue indexVal = std::any_cast<TzdValue>(visit(ctx->expression(1)));

    // 兼容可能被解析为 DOUBLE 或 INT 的下标
    int idx = (indexVal.type == TzdValue::DOUBLE) ? (int)indexVal.dVal : (int)indexVal.lVal;

    // --- 处理数组访问 ---
    if (container.type == TzdValue::ARRAY) {
        if (idx < 0 || idx >= (int)container.arrVal.size()) {
            throw TzdRuntimeException("数组索引越界: 尝试访问索引 " + std::to_string(idx) +
                ", 但数组长度为 " + std::to_string(container.arrVal.size()),
                ctx->getStart());
        }
        return container.arrVal[idx]; // ✨ 保持原有的值返回，REPL 打印和普通表达式完美绿灯通行
    }

    // --- 处理字符串访问 ---
    if (container.type == TzdValue::STRING) {
        if (idx < 0 || idx >= (int)container.sVal.length()) {
            throw TzdRuntimeException("字符串索引越界: 尝试访问索引 " + std::to_string(idx) +
                ", 但字符串长度为 " + std::to_string(container.sVal.length()),
                ctx->getStart());
        }
        return TzdValue(std::string(1, container.sVal[idx]));
    }

    // --- 处理 map 访问 (key 为 string) ---
    if (container.type == TzdValue::MAP) {
        std::string key = getAsString(indexVal);
        auto it = container.mapVal.find(key);
        if (it == container.mapVal.end()) {
            return TzdValue();
        }
        return it->second;
    }

    throw TzdRuntimeException("类型错误: 该类型不支持下标访问", ctx->getStart());
}

std::any TzdInterpreter::visitLambdaExpr(TzdLangParser::LambdaExprContext* ctx) {
    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    parseParamList(ctx->paramList(), params, paramTypes);
    TzdValue funcVal("", params, ctx->block());
    funcVal.paramTypes = paramTypes;
    funcVal.type = TzdValue::FUNCTION;
    return funcVal;
}

// 在解释器开始处理一个新脚本时调用
void TzdInterpreter::prepareForScript() {
    // 为新脚本创建一个全新的编译器和模块
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "MainScriptModule");
    m_compiler->setupExternalFunctions(); // 只需要设置一次
    m_pendingJitFunctions.clear();
}

// In your interpreter, call this when you're ready to run code.
void TzdInterpreter::jitAllFunctions() {
    std::unique_ptr<llvm::Module> module = m_compiler->extractModule();
    if (module) {
        m_jitEngine->jitModule(std::move(module));
    }
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "new_main_module");
}

std::any TzdInterpreter::visitFunctionDeclaration(TzdLangParser::FunctionDeclarationContext* ctx) {
    std::string funcName = ctx->IDENTIFIER()->getText();
    std::string internalJitName = funcName + "_v" + std::to_string(m_funcVersion++);

    std::vector<std::string> params;
    std::vector<std::string> paramTypes;
    parseParamList(ctx->paramList(), params, paramTypes);

    TzdValue funcVal(funcName, params, ctx->block());
    funcVal.paramTypes = paramTypes;
    funcVal.type = TzdValue::FUNCTION;
    funcVal.jitInternalName = internalJitName;

    funcVal.sourceFile = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
    funcVal.line = (int)ctx->getStart()->getLine();
    funcVal.column = (int)ctx->getStart()->getCharPositionInLine();

    if (!m_noJit && m_jitEngine && m_compiler) {
        try {
            m_compiler->compileNamedFunction(ctx->block(), ctx->paramList(), internalJitName);
            m_pendingJitFunctions.insert(internalJitName);
            m_jitNameToUserMap[internalJitName] = funcName;
        } catch (const std::exception& e) {
            // JIT compilation failed (e.g. BIGINT literal) — function will run via interpreter
            agentLogCompile("D", "visitFunctionDeclaration", e.what());
        }
    }

    // --- 修改回调通知 ---
    // 必须在覆盖 scopes 之前，通过旧对象触发回调
    if (scopes.back().count(funcName)) {
        emitFunctionRedefined(funcName, funcVal, ctx);
    }

    scopes.back()[funcName] = funcVal;
    return funcVal;
}

void TzdInterpreter::jitPendingModule() {
    if (m_noJit || !m_jitEngine || !m_compiler || m_pendingJitFunctions.empty()) return;

    // 1. 提取模块并加入 JIT 引擎 (注意：此过程会触发编译)
    auto TSM = m_compiler->extractThreadSafeModule();
    if (TSM) {
        m_jitEngine->addModule(std::move(TSM));
    }

    // 2. 遍历本次编译的所有符号 (内部名，如 add_v1)
    for (const std::string& symbol : m_pendingJitFunctions) {
        auto symOrErr = m_jitEngine->lookupSymbol(symbol);
        if (!symOrErr) {
            llvm::consumeError(symOrErr.takeError());
            continue;
        }

        // 获取生成的机器码地址
        void* addr = reinterpret_cast<void*>(symOrErr->getValue());

        // 3. 映射回用户原始名称 (如 add_v1 -> add)
        std::string userName = symbol;
        if (m_jitNameToUserMap.count(symbol)) {
            userName = m_jitNameToUserMap[symbol];
        }

        // 4. 处理类成员方法 (逻辑：ClassName_MethodName)
        // 只有当名字包含下划线，且前半部分是已注册类时，视为类方法
        size_t sep = userName.find('_');
        bool isClassMember = false;
        if (sep != std::string::npos) {
            std::string potentialCls = userName.substr(0, sep);
            if (TzdOopManager::getClass(potentialCls)) isClassMember = true;
        }

        if (isClassMember) {
            std::string clsName = userName.substr(0, sep);
            std::string methName = userName.substr(sep + 1);
            TzdClassDef* cls = TzdOopManager::getClass(clsName);

            // 修正构造函数特殊处理：ClassName_CtorName_ctor_N
            if (methName.find("_ctor_") != std::string::npos) {
                size_t suffixPos = methName.rfind("_ctor_");
                std::string ctorBase = methName.substr(0, suffixPos);
                int paramCount = std::stoi(methName.substr(suffixPos + 6));
                if (cls) {
                    for (auto& ctor : cls->constructors) {
                        if (ctor.paramCount == paramCount) {
                            ctor.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                        }
                    }
                    if (cls->methods.count(ctorBase)) {
                        cls->methods[ctorBase].jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                    }
                }
                continue;
            }
            if (methName.size() > 5 && methName.substr(methName.size() - 5) == "_ctor")
                methName = methName.substr(0, methName.size() - 5);

            if (cls && cls->methods.count(methName)) {
                // 更新类定义中的指针，所有实例都会同步生效
                cls->methods[methName].jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                // std::cout << "[JIT] Method " << clsName << "::" << methName << " redefined." << std::endl;
            }
        }
        else {
            // 5. 处理全局函数 (支持重定义的核心逻辑)
            // 必须从 scopes 的末尾（最顶层作用域）向前查找。
            // 这样能确保我们找到的是用户最后一次定义的那个 "add" 对象。
            bool linked = false;
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                auto findResult = it->find(userName);
                if (findResult != it->end() && findResult->second.type == TzdValue::FUNCTION) {
                    if (findResult->second.jitInternalName == symbol) {
                        findResult->second.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                        linked = true;
                        // std::cout << "[JIT] Success: Linked " << symbol << std::endl;
                    }
                    else {
                        // 将新生成的机器码地址写给最新的函数对象
                        findResult->second.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
                        linked = true;
                        // std::cout << "[JIT] Function " << userName << " redefined -> " << symbol << std::endl;
                        
                    }
                    break;
                }
            }
        }
    }

    // 6. Register worker pointers for TCO cross-function calls
    for (const std::string& symbol : m_pendingJitFunctions) {
        m_jitEngine->registerWorkerForSymbol(symbol);
    }

    // 7. 清理状态
    m_pendingJitFunctions.clear();
    m_jitNameToUserMap.clear();
}



std::any TzdInterpreter::visitFunDeclStmt(TzdLangParser::FunDeclStmtContext* ctx) {
    return visit(ctx->functionDeclaration());
}

std::any TzdInterpreter::visitNativeFunDeclStmt(TzdLangParser::NativeFunDeclStmtContext* ctx) {
    auto decl = ctx->nativeFunctionDeclaration();
    std::string funcName = decl->IDENTIFIER()->getText();

    // --- 1. 解析属性列表 ---
    std::unordered_map<std::string, std::string> attrs;
    if (decl->nativeAttrList()) {
        for (auto* attr : decl->nativeAttrList()->nativeAttr()) {
            // 修正：不要直接调 IDENTIFIER()，因为 key 可能是关键字
            // 获取第一个子节点（即等号左边的内容）
            std::string key = attr->children[0]->getText();

            std::string val = "";
            // 获取第三个子节点（即等号右边的内容）
            if (attr->children.size() >= 3) {
                val = attr->children[2]->getText();
            }

            // 去掉字符串引号
            if (val.size() >= 2 && (val.front() == '"' || val.front() == '\'')) {
                val = val.substr(1, val.size() - 2);
            }

            attrs[key] = val;
        }
    }

    // --- 2. 提取关键属性 ---
    std::string dllPath = attrs.count("dll") ? attrs["dll"] : "";
    std::string libFuncName = attrs.count("fun") ? attrs["fun"] : funcName;
    int prototype = attrs.count("prototype") ? std::stoi(attrs["prototype"]) : 0;
    std::string typeStr = attrs.count("type") ? attrs["type"] : "";
    std::string returnType = attrs.count("return") ? attrs["return"] : "float";

    // --- 3. 加载 DLL/函数地址 ---
#ifdef _WIN32
    HMODULE hLib = LoadLibraryA(dllPath.c_str());
    if (!hLib) throw TzdRuntimeException("Cannot load DLL: " + dllPath, ctx->start);
    void* procAddr = (void*)GetProcAddress(hLib, libFuncName.c_str());
#else
    void* hLib = dlopen(dllPath.c_str(), RTLD_LAZY);
    if (!hLib) throw TzdRuntimeException("Cannot load lib: " + dllPath, ctx->start);
    void* procAddr = dlsym(hLib, libFuncName.c_str());
#endif

    if (!procAddr) throw TzdRuntimeException("Function not found: " + libFuncName, ctx->start);

    TzdValue::NativeFuncType wrapper;

    // --- 4. 分支处理：原生模式 vs Prototype 转换模式 ---
    if (prototype == 1) {
        // 调用我们新写的类
        wrapper = TzdFFIAdapter::buildWrapper(procAddr, libFuncName,typeStr, returnType);

        if (!wrapper) {
            throw TzdRuntimeException("FFI Error: Unsupported signature " + typeStr + "->" + returnType, ctx->start);
        }
    }
    else {
        typedef TzdValue(*RawFunc)(const std::vector<TzdValue>&);
        RawFunc target = reinterpret_cast<RawFunc>(procAddr);
        wrapper = [target](const std::vector<TzdValue>& args) -> TzdValue {
            return target(args);
            };
    }

    setVariable(funcName, TzdValue(wrapper, funcName));
    return nullptr;
}

/*std::any TzdInterpreter::visitFunCallExpr(TzdLangParser::FunCallExprContext* ctx) {
    auto callCtx = ctx->functionCall();
    std::string funcName = callCtx->IDENTIFIER()->getText();
    TzdValue funcVal = getVariable(funcName,ctx);

    // 1. 准备参数 (无论是脚本函数还是原生函数都需要先计算参数)
    std::vector<TzdValue> args;
    if (callCtx->exprList()) {
        for (auto expr : callCtx->exprList()->expression()) {
            args.push_back(std::any_cast<TzdValue>(visit(expr)));
        }
    }

    // 2. 分情况处理
    if (funcVal.type == TzdValue::NATIVE_FUNCTION) {
        // --- 情况 A: 调用原生 C++ 函数 ---
        try {
            // 直接调用存储的 std::function
            return funcVal.nativeFunc(args);
        }
        catch (const std::exception& e) {
            throw TzdRuntimeException("调用原生函数 '" + funcName + "' 时发生错误: " + e.what(), ctx->getStart());
        }
    }
    else if (funcVal.type == TzdValue::FUNCTION) {
        // --- 情况 B: 调用脚本定义的函数 (原有逻辑) ---

        // 检查参数数量
        if (args.size() != funcVal.params.size()) {
            std::string err = "函数 '" + funcName + "' 调用参数不匹配: "
                "需要 " + std::to_string(funcVal.params.size()) + " 个，"
                "实际提供 " + std::to_string(args.size()) + " 个。";
            throw TzdRuntimeException(err, ctx->getStart());
        }

        // 创建新作用域并推入参数
        std::unordered_map<std::string, TzdValue> newScope;
        for (size_t i = 0; i < args.size(); ++i) {
            newScope[funcVal.params[i]] = args[i];
        }
        scopes.push_back(newScope);

        TzdValue returnValue;
        try {
            visit(funcVal.funcBody);
        }
        catch (const TzdReturnException& ret) {
            returnValue = ret.value;
        }
        scopes.pop_back();
        return returnValue;
    }
    else {
        throw TzdRuntimeException("尝试调用非函数对象: " + funcName, ctx->getStart());
    }
}*/

std::any TzdInterpreter::visitReturnStmt(TzdLangParser::ReturnStmtContext* ctx) {
    TzdValue val;
    if (ctx->expression()) {
        val = std::any_cast<TzdValue>(visit(ctx->expression()));
    }

    // compilation mode removed

    throw TzdReturnException(val);
}

std::any TzdInterpreter::visitClassDeclStmt(TzdLangParser::ClassDeclStmtContext* ctx) {
    return visit(ctx->classDeclaration());
}

std::any TzdInterpreter::visitExprStmt(TzdLangParser::ExprStmtContext* ctx) {
    TzdValue val = castAnyToTzdValue(visit(ctx->expression()), "visitExprStmt");
    return val;
}

std::any TzdInterpreter::visitVarDeclStmt(TzdLangParser::VarDeclStmtContext* ctx) {
    auto decl = ctx->variableDeclaration();
    std::string id = decl->IDENTIFIER()->getText();

    // --- 修复点 1：安全获取类型 ---
    std::string declaredType = "";
    bool isImplicit = false; // 标记是否为 var 推导类型

    if (decl->typeType()) {
        // 情况 A: 显式类型 (int a = 1;)
        declaredType = decl->typeType()->getText();
        if (!isTypeValid(declaredType)) {
            throw TzdRuntimeException("未定义的类型: '" + declaredType + "'", decl->typeType()->getStart());
        }
    }
    else {
        // 情况 B: 隐式类型 (var a = 1;)
        isImplicit = true;
    }

    TzdValue val;
    std::string c_init_code = "";

    // 计算初始化表达式
    if (decl->expression()) {
        val = std::any_cast<TzdValue>(visit(decl->expression()));
    }

    // --- 修复点 2：类型强制转换逻辑 (仅显式类型时执行) ---
    if (!isImplicit) {
        if (declaredType == "int" || declaredType == "long" || declaredType == "i32") {
            if (val.type != TzdValue::NONE) {
                long long intPart = (long long)getAsDouble(val);
                val = TzdValue(intPart);
            }
        }
        else if (declaredType == "float" || declaredType == "double") {
            if (val.type != TzdValue::NONE) {
                double floatPart = getAsDouble(val);
                val = TzdValue(floatPart);
            }
        }
        else {
            // 类实例校验
            TzdClassDef* cls = TzdOopManager::getClass(declaredType);
            if (cls && val.type == TzdValue::INSTANCE) {
                if (!TzdOopManager::isInstanceOf(val.instanceVal, declaredType)) {
                    throw TzdRuntimeException("类型不匹配: 无法将 " + val.instanceVal->definition->fullName + " 赋值给 " + declaredType, ctx->getStart());
                }
            }
        }
    }
    else {
        // var 声明：如果未初始化，给默认值 0
        if (val.type == TzdValue::NONE) val = TzdValue(0LL);
    }

    // =========================================================
    // --- 分支 2: 运行模式 (更新内存 + 捕获代码) ---
    // =========================================================

    // 更新运行时内存
    scopes.back()[id] = val;

    // 捕获 Top-Level C 代码 (用于 export_c_code)
    // compilation features removed; just return the runtime value
    return val;
}

std::any TzdInterpreter::visitIdExpr(TzdLangParser::IdExprContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    return getVariable(name, ctx);
}

std::any TzdInterpreter::visitAssignmentExpr(TzdLangParser::AssignmentExprContext* ctx) {
    // 1. 计算右侧表达式的值（由于上面恢复了，右边通过 visit 拿到的百分之百是标准的 TzdValue）
    std::any rAny = visit(ctx->expression(1));
    TzdValue rightVal = std::any_cast<TzdValue>(rAny);

    auto lhsCtx = ctx->expression(0);

    // 2. 运算符提取
    bool isCompound = true;
    std::string c_op_func = "";
    if (ctx->ASSIGN()) isCompound = false;
    else if (ctx->PLUS_ASSIGN()) c_op_func = "tzd_add";
    else if (ctx->MIN_ASSIGN()) c_op_func = "tzd_sub";
    else if (ctx->MUL_ASSIGN()) c_op_func = "tzd_mul";
    else if (ctx->DIV_ASSIGN()) c_op_func = "tzd_div";

    auto calculateCompound = [&](TzdValue oldV, TzdValue rightV) -> TzdValue {
        if (!isCompound) return rightV;
        if (ctx->PLUS_ASSIGN()) {
            if (oldV.type == TzdValue::STRING || rightV.type == TzdValue::STRING)
                return TzdValue(getAsString(oldV) + getAsString(rightV));
            return TzdValue(getAsDouble(oldV) + getAsDouble(rightV));
        }
        if (ctx->MIN_ASSIGN()) return TzdValue(getAsDouble(oldV) - getAsDouble(rightV));
        if (ctx->MUL_ASSIGN()) return TzdValue(getAsDouble(oldV) * getAsDouble(rightV));
        if (ctx->DIV_ASSIGN()) {
            if (getAsDouble(rightV) == 0) throw TzdRuntimeException("Division by zero", ctx->getStart());
            return TzdValue(getAsDouble(oldV) / getAsDouble(rightV));
        }
        return rightV;
        };

    // 3. ✨【核心黑科技：通过递归 Lambda 顺藤摸瓜，挖出符号表里最底层的物理地址指针】
    std::function<TzdValue* (antlr4::ParserRuleContext*)> getLValuePointer =
        [&](antlr4::ParserRuleContext* subCtx) -> TzdValue* {
        // 如果子节点依然是个下标访问表达式（支持多维数组如 matrix[0][1]）
        if (auto indexCtx = dynamic_cast<TzdLangParser::IndexExprContext*>(subCtx)) {
            TzdValue* containerPtr = getLValuePointer(indexCtx->expression(0));
            if (!containerPtr) return nullptr;

            TzdValue indexVal = std::any_cast<TzdValue>(visit(indexCtx->expression(1)));

            if (containerPtr->type == TzdValue::ARRAY) {
                int idx = (indexVal.type == TzdValue::DOUBLE) ? (int)indexVal.dVal : (int)indexVal.lVal;
                if (idx < 0 || idx >= (int)containerPtr->arrVal.size()) {
                    throw TzdRuntimeException("数组索引越界", indexCtx->getStart());
                }
                return &(containerPtr->arrVal[idx]);
            }
            if (containerPtr->type == TzdValue::MAP) {
                std::string key = getAsString(indexVal);
                return &(containerPtr->mapVal[key]);
            }
            return nullptr;
        }

        // 递归基底：普通的变量名标识符，直接去最真实的 scopes 里面抓取变量地址
        std::string id = subCtx->getText();
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(id)) return &((*it)[id]);
        }
        return &(scopes.back()[id]);
        };

    // 4. ✨【精准拦截下标赋值操作】
    if (dynamic_cast<TzdLangParser::IndexExprContext*>(lhsCtx)) {
        TzdValue* targetElemPtr = getLValuePointer(lhsCtx);
        if (targetElemPtr) {
            TzdValue finalVal = calculateCompound(*targetElemPtr, rightVal);
            *targetElemPtr = finalVal; // 🌟 顺着内存指针，直接将新值物理改写进作用域数组中！
            return finalVal;
        }
        throw TzdRuntimeException("无法对非法的下标位置进行赋值", ctx->getStart());
    }

    // 5. 处理对象属性访问或类静态数变量成员赋值 (保持你原本的代码不变)
    TzdLangParser::MemberAccessExprContext* memCtx = nullptr;
    if (auto atomExpr = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx)) {
        memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomExpr->atom());
    }
    else {
        memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(lhsCtx);
    }

    if (memCtx) {
        TzdValue leftBase = std::any_cast<TzdValue>(visit(memCtx->atom()));
        std::string fieldName = memCtx->IDENTIFIER()->getText();

        if (leftBase.type == TzdValue::INSTANCE) {
            TzdValue oldVal = leftBase.instanceVal->getMember(fieldName);
            TzdValue finalVal = calculateCompound(oldVal, rightVal);
            leftBase.instanceVal->setMember(fieldName, finalVal);
            return finalVal;
        }
        else if (leftBase.type == TzdValue::CLASS_DEF) {
            TzdClassDef* cls = leftBase.classDefVal;
            TzdValue oldVal = cls->staticValues.count(fieldName) ? cls->staticValues[fieldName] : TzdValue(0.0);
            TzdValue finalVal = calculateCompound(oldVal, rightVal);
            cls->staticValues[fieldName] = finalVal;
            return finalVal;
        }
        throw TzdRuntimeException("Cannot assign to non-object member", ctx->getStart());
    }

    // 6. 普通纯变量赋值逻辑
    std::string id = lhsCtx->getText();

    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->count(id)) {
            TzdValue finalVal = calculateCompound((*it)[id], rightVal);
            (*it)[id] = finalVal;
            return finalVal;
        }
    }

    // 7. 面向对象隐式 `this` 查找与赋值
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto thisIt = it->find("this");
        if (thisIt != it->end() && thisIt->second.type == TzdValue::INSTANCE) {
            TzdInstance* inst = thisIt->second.instanceVal;
            TzdClassDef* curDef = inst->definition;
            bool isMember = false;
            while (curDef) {
                if (curDef->fields.count(id) || curDef->staticValues.count(id)) {
                    isMember = true; break;
                }
                if (curDef->parentName.empty()) break;
                curDef = TzdOopManager::getClass(curDef->parentName);
            }

            if (isMember) {
                TzdValue finalVal = calculateCompound(inst->getMember(id), rightVal);
                inst->setMember(id, finalVal);
                return finalVal;
            }
        }
    }

    // 8. 兜底隐式变量创建
    TzdValue finalVal = calculateCompound(TzdValue(0LL), rightVal);
    scopes.back()[id] = finalVal;
    return finalVal;
}

std::any TzdInterpreter::visitAdditiveExpr(TzdLangParser::AdditiveExprContext* ctx) {
    TzdValue left = std::any_cast<TzdValue>(visit(ctx->expression(0)));
    TzdValue right = std::any_cast<TzdValue>(visit(ctx->expression(1)));
    bool isPlus = ctx->PLUS() != nullptr;
    TzdValue result;

    // --- String concatenation (highest priority for +) ---
    // Must check BEFORE BIGINT/RATIONAL so that "text" + bigint works correctly.
    if (isPlus && (left.type == TzdValue::STRING || right.type == TzdValue::STRING)) {
        result = TzdValue(getAsString(left) + getAsString(right));
    }
    // --- RATIONAL arithmetic (exact fractions) ---
    else if (needs_rational(left, right)) {
        std::string a = to_rational_str(left), b = to_rational_str(right);
        std::string r = isPlus ? rational_add(a, b) : rational_sub(a, b);
        if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
        result.type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT;
        result.sVal = r;
    }
    // --- BIGINT arithmetic (arbitrary precision) ---
    else if (needs_bigint(left, right)) {
        std::string a = to_bigint_str(left), b = to_bigint_str(right);
        result = make_bigint(isPlus ? bigint_add(a, b) : bigint_sub(a, b));
    }
    else if (left.type == TzdValue::POINTER) {
        long long offset = (right.type == TzdValue::ULONG) ? (long long)right.ulVal : (long long)getAsDouble(right);
        result = TzdValue((void*)((char*)left.ptrVal + (isPlus ? offset : -offset)));
    }
    else {
        double l = getAsDouble(left), r = getAsDouble(right);
        result = TzdValue(isPlus ? l + r : l - r);
    }

    return result;
}

std::any TzdInterpreter::visitMultiplicativeExpr(TzdLangParser::MultiplicativeExprContext* ctx) {
    TzdValue left = std::any_cast<TzdValue>(visit(ctx->expression(0)));
    TzdValue right = std::any_cast<TzdValue>(visit(ctx->expression(1)));

    // --- RATIONAL arithmetic ---
    if (needs_rational(left, right)) {
        std::string a = to_rational_str(left), b = to_rational_str(right);
        if (ctx->MUL()) {
            std::string r = rational_mul(a, b);
            if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
            TzdValue v; v.type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT; v.sVal = r;
            return v;
        }
        // DIV or MOD: (a/b) / (c/d) = ad/bc
        std::string bn, bd;
        rational_parse(b, bn, bd);
        if (bn == "0") return TzdValue(0LL);
        std::string r = rational_div(a, b);
        if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
        TzdValue v; v.type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT; v.sVal = r;
        return v;
    }

    // --- BIGINT arithmetic ---
    if (needs_bigint(left, right)) {
        std::string tmpA, tmpB;
        const std::string& a = (left.type == TzdValue::BIGINT || left.type == TzdValue::STRING) ? left.sVal : (tmpA = to_bigint_str(left));
        const std::string& b = (right.type == TzdValue::BIGINT || right.type == TzdValue::STRING) ? right.sVal : (tmpB = to_bigint_str(right));
        if (ctx->MUL()) {
            std::string r = bigint_mul(a, b);
            if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
            return make_bigint(r);
        }
        if (b == "0" || b == "-0") return TzdValue(0LL);
        if (ctx->DIV()) return make_bigint(bigint_div(a, b));
        return make_bigint(bigint_mod(a, b)); // MOD
    }

    if (left.type == TzdValue::DOUBLE || right.type == TzdValue::DOUBLE || left.type == TzdValue::FLOAT || right.type == TzdValue::FLOAT) {
        double l = getAsDouble(left), r = getAsDouble(right);
        if (ctx->MUL()) return TzdValue(l * r);
        if (r == 0) return TzdValue(0.0);
        return ctx->DIV() ? TzdValue(l / r) : TzdValue(std::fmod(l, r));
    }

    if (left.type == TzdValue::ULONG || right.type == TzdValue::ULONG) {
        unsigned long long l = (left.type == TzdValue::ULONG) ? left.ulVal : (unsigned long long)left.lVal;
        unsigned long long r = (right.type == TzdValue::ULONG) ? right.ulVal : (unsigned long long)right.lVal;
        if (ctx->MUL()) return TzdValue(l * r);
        if (r == 0) return TzdValue(0ULL);
        return ctx->DIV() ? TzdValue(l / r) : TzdValue(l % r);
    }

    long long l = left.lVal, r = right.lVal;
    if (ctx->MUL()) return TzdValue(l * r);
    if (r == 0) return TzdValue(0LL);
    return ctx->DIV() ? TzdValue(l / r) : TzdValue(l % r);
}

std::any TzdInterpreter::visitPowerExpr(TzdLangParser::PowerExprContext* ctx) {
    TzdValue left = std::any_cast<TzdValue>(visit(ctx->expression(0)));
    TzdValue right = std::any_cast<TzdValue>(visit(ctx->expression(1)));

    // --- RATIONAL power (exact fraction exponentiation) ---
    if (needs_rational(left, right)) {
        std::string base = to_rational_str(left), exp = to_bigint_str(right);
        std::string r = rational_pow(base, exp);
        if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
        TzdValue v; v.type = (r.find('/') != std::string::npos) ? TzdValue::RATIONAL : TzdValue::BIGINT; v.sVal = r;
        return v;
    }

    // --- BIGINT power (arbitrary precision) ---
    if (needs_bigint(left, right)) {
        std::string base = to_bigint_str(left), exp = to_bigint_str(right);
        std::string r = bigint_pow(base, exp);
        if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
        return make_bigint(r);
    }
    // --- Integer power: use BIGINT for exact results (like Python) ---
    // When both operands are integers, std::pow loses precision or overflows to inf.
    // Use bigint_pow for exact results, falling back to LONG if small enough.
    bool leftIsInt = left.type >= TzdValue::SBYTE && left.type <= TzdValue::ULONG;
    bool rightIsInt = right.type >= TzdValue::SBYTE && right.type <= TzdValue::ULONG;
    if (leftIsInt && rightIsInt) {
        std::string base = to_bigint_str(left), exp = to_bigint_str(right);
        // Negative exponent → fraction (e.g. 2^(-1) = 1/2)
        if (bigint_is_neg(exp)) {
            std::string num = "1";
            std::string den = bigint_pow(base, bigint_abs(exp));
            if (den == "inf") return TzdValue(std::numeric_limits<double>::infinity());
            return make_rational(num, den);
        }
        std::string r = bigint_pow(base, exp);
        if (r == "inf") return TzdValue(std::numeric_limits<double>::infinity());
        // If result fits in int64, return as LONG for efficiency
        try { return TzdValue(std::stoll(r)); }
        catch (...) { return make_bigint(r); }
    }

    return TzdValue(std::pow(getAsDouble(left), getAsDouble(right)));
}

std::any TzdInterpreter::visitParenExpr(TzdLangParser::ParenExprContext* ctx) {
    return visit(ctx->expression());
}

std::any TzdInterpreter::visitUnaryExpr(TzdLangParser::UnaryExprContext* ctx) {
    TzdValue val = castAnyToTzdValue(visit(ctx->expression()), "visitUnaryExpr");

    if (ctx->MINUS()) {
        if (val.type == TzdValue::LONG) return TzdValue(-val.lVal);
        return TzdValue(-val.dVal);
    }
    if (ctx->GXXX()) {
        double d = (val.type == TzdValue::LONG) ? (double)val.lVal : val.dVal;
        return TzdValue(std::sqrt(d));
    }
    if (ctx->NOT()) {
        return TzdValue(!isTruthy(val));
    }
    return val;
}

std::any TzdInterpreter::visitPostfixExpr(TzdLangParser::PostfixExprContext* ctx) {
    auto lhsCtx = ctx->expression();

    // 内部辅助 Lambda，用于正确地执行自增/自减
    auto performIncrement = [&](TzdValue& val) {
        if (val.type == TzdValue::LONG) {
            if (ctx->INC()) val.lVal++; else val.lVal--;
            val.dVal = static_cast<double>(val.lVal); // 保持同步
        }
        else if (val.type == TzdValue::FLOAT) {
            if (ctx->INC()) val.dVal++; else val.dVal--;
        }
        else {
            throw TzdRuntimeException("后缀自增/自减只能用于数值类型", ctx->getStart());
        }
        };

    // --- 情况 A: 成员访问 (myObj.field++ 或 MyClass.staticField++) ---
    if (auto atomCtx = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx)) {
        if (auto memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomCtx->atom())) {
            TzdValue leftObj = std::any_cast<TzdValue>(visit(memCtx->atom()));
            std::string fieldName = memCtx->IDENTIFIER()->getText();
            if (leftObj.type == TzdValue::INSTANCE) {
                TzdInstance* inst = leftObj.instanceVal;
                TzdValue* fieldRef = inst->getMemberPtr(fieldName);
                if (fieldRef) {
                    TzdValue oldVal = *fieldRef;
                    performIncrement(*fieldRef);
                    return oldVal;
                }
            }
            else if (leftObj.type == TzdValue::CLASS_DEF) {
                TzdClassDef* cls = leftObj.classDefVal;
                if (cls->staticValues.count(fieldName)) {
                    TzdValue& fieldRef = cls->staticValues.at(fieldName);
                    TzdValue oldVal = fieldRef;
                    performIncrement(fieldRef);
                    return oldVal;
                }
            }
            throw TzdRuntimeException("尝试对不存在的成员进行自增/自减: " + fieldName, ctx->getStart());
        }
    }

    // --- 情况 B: 数组索引 (arr[i]++) ---
    else if (auto indexCtx = dynamic_cast<TzdLangParser::IndexExprContext*>(lhsCtx)) {
        std::string arrayName = indexCtx->expression(0)->getText();
        int idx = (int)std::any_cast<TzdValue>(visit(indexCtx->expression(1))).dVal;
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            if (it->count(arrayName)) {
                TzdValue& arr = (*it)[arrayName];
                if (arr.type != TzdValue::ARRAY || idx < 0 || idx >= arr.arrVal.size()) {
                    throw TzdRuntimeException("数组索引越界或目标不是数组", ctx->getStart());
                }
                TzdValue& elemRef = arr.arrVal[idx];
                TzdValue oldVal = elemRef;
                performIncrement(elemRef);
                return oldVal;
            }
        }
        throw TzdRuntimeException("未找到数组: " + arrayName, ctx->getStart());
    }

    // --- 情况 C: 普通变量 (x++) ---
    std::string id = lhsCtx->getText();
    TzdValue oldVal = getVariable(id, ctx);
    TzdValue newVal = oldVal;
    performIncrement(newVal);
    setVariable(id, newVal);
    return oldVal;
}

std::any TzdInterpreter::visitForStmt(TzdLangParser::ForStmtContext* ctx) {
    scopes.push_back(std::unordered_map<std::string, TzdValue>());
    if (ctx->forInit()) visit(ctx->forInit());
    while (true) {
        if (ctx->cond) {
            TzdValue condition = std::any_cast<TzdValue>(visit(ctx->cond));
            if (!isTruthy(condition)) break;
        }
        try {
            if (ctx->statement()) visit(ctx->statement());
        }
        catch (const TzdBreakException&) {
            break;
        }
        catch (const TzdContinueException&) {
            // fall through to step
        }
        catch (const TzdReturnException&) {
            scopes.pop_back();
            throw;
        }
        catch (const TzdThrowException&) {
            scopes.pop_back();
            throw;
        }
        if (ctx->step) visit(ctx->step);
    }
    scopes.pop_back();
    return TzdValue();
}

std::any TzdInterpreter::visitPrefixExpr(TzdLangParser::PrefixExprContext* ctx) {
    auto lhsCtx = ctx->expression();

    auto performIncrement = [&](TzdValue& val) -> TzdValue& {
        if (val.type == TzdValue::LONG) {
            if (ctx->INC()) val.lVal++; else val.lVal--;
            val.dVal = static_cast<double>(val.lVal);
        }
        else if (val.type == TzdValue::FLOAT) {
            if (ctx->INC()) val.dVal++; else val.dVal--;
        }
        else {
            throw TzdRuntimeException("前缀自增/自减只能用于数值类型", ctx->getStart());
        }
        return val;
        };

    if (auto atomCtx = dynamic_cast<TzdLangParser::AtomExprContext*>(lhsCtx)) {
        if (auto memCtx = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(atomCtx->atom())) {
            TzdValue leftObj = std::any_cast<TzdValue>(visit(memCtx->atom()));
            std::string fieldName = memCtx->IDENTIFIER()->getText();
            if (leftObj.type == TzdValue::INSTANCE) {
                TzdInstance* inst = leftObj.instanceVal;
                TzdValue* fieldRef = inst->getMemberPtr(fieldName);
                if (fieldRef) {
                    return performIncrement(*fieldRef);
                }
            }
            else if (leftObj.type == TzdValue::CLASS_DEF) {
                TzdClassDef* cls = leftObj.classDefVal;
                if (cls->staticValues.count(fieldName)) {
                    return performIncrement(cls->staticValues.at(fieldName));
                }
            }
            throw TzdRuntimeException("尝试对不存在的成员进行自增/自减: " + fieldName, ctx->getStart());
        }
    }

    // --- 省略对数组索引的前缀操作，如果需要可以添加 ---

    // --- 情况 C: 普通变量 (++x) ---
    std::string id = lhsCtx->getText();
    TzdValue val = getVariable(id, ctx);
    performIncrement(val);
    setVariable(id, val);
    return val;
}

std::any TzdInterpreter::visitWhileStmt(TzdLangParser::WhileStmtContext* ctx) {
    while (true) {
        TzdValue condition = std::any_cast<TzdValue>(visit(ctx->expression()));
        if (!isTruthy(condition)) break;

        try {
            visit(ctx->statement());
        }
        catch (const TzdBreakException&) {
            break;
        }
        catch (const TzdContinueException&) {
            continue;
        }
        catch (const TzdReturnException&) {
            throw;
        }
        catch (const TzdThrowException&) {
            throw;
        }
    }
    return std::any(TzdValue());
}

std::any TzdInterpreter::visitBreakStmt(TzdLangParser::BreakStmtContext* ctx) {
    (void)ctx;
    throw TzdBreakException();
}

std::any TzdInterpreter::visitContinueStmt(TzdLangParser::ContinueStmtContext* ctx) {
    (void)ctx;
    throw TzdContinueException();
}

std::any TzdInterpreter::visitSwitchStmt(TzdLangParser::SwitchStmtContext* ctx) {
    TzdValue switchVal = std::any_cast<TzdValue>(visit(ctx->expression()));
    bool matched = false;

    for (auto* caseCtx : ctx->switchCase()) {
        TzdValue caseVal = std::any_cast<TzdValue>(visit(caseCtx->expression()));
        if (!matched && valuesEqual(switchVal, caseVal)) {
            matched = true;
        }
        if (matched) {
            for (auto* stmt : caseCtx->statement()) {
                try {
                    visit(stmt);
                }
                catch (const TzdBreakException&) {
                    return TzdValue();
                }
                catch (const TzdContinueException&) {
                    throw;
                }
                catch (const TzdReturnException&) {
                    throw;
                }
            }
        }
    }

    if (ctx->switchDefault()) {
        if (!matched) matched = true;
        if (matched) {
            for (auto* stmt : ctx->switchDefault()->statement()) {
                try {
                    visit(stmt);
                }
                catch (const TzdBreakException&) {
                    return TzdValue();
                }
                catch (const TzdContinueException&) {
                    throw;
                }
                catch (const TzdReturnException&) {
                    throw;
                }
            }
        }
    }
    return TzdValue();
}


std::any TzdInterpreter::visitIfStmt(TzdLangParser::IfStmtContext* ctx) {
    TzdValue condition = std::any_cast<TzdValue>(visit(ctx->expression()));

    if (isTruthy(condition)) {
        return visit(ctx->statement(0));
    }
    else if (ctx->KW_ELSE()) {
        return visit(ctx->statement(1));
    }

    return TzdValue();
}

std::any TzdInterpreter::visitForInit(TzdLangParser::ForInitContext* ctx) {
    if (ctx->variableDeclaration()) return visit(ctx->variableDeclaration());
    if (ctx->expression()) return visit(ctx->expression());
    return TzdValue();
}

std::any TzdInterpreter::visitRelationalExpr(TzdLangParser::RelationalExprContext* ctx) {
    TzdValue left = castAnyToTzdValue(visit(ctx->expression(0)), "visitRelationalExpr.l");
    TzdValue right = castAnyToTzdValue(visit(ctx->expression(1)), "visitRelationalExpr.r");

    // RATIONAL comparison (cross-multiply for exact comparison)
    if (needs_rational(left, right)) {
        int c = rational_compare(to_rational_str(left), to_rational_str(right));
        if (ctx->GT()) return TzdValue(c > 0);
        if (ctx->LT()) return TzdValue(c < 0);
        if (ctx->GE()) return TzdValue(c >= 0);
        return TzdValue(c <= 0);
    }

    // BIGINT comparison (preserves precision for large numbers)
    if (needs_bigint(left, right)) {
        int c = bigint_compare(to_bigint_str(left), to_bigint_str(right));
        if (ctx->GT()) return TzdValue(c > 0);
        if (ctx->LT()) return TzdValue(c < 0);
        if (ctx->GE()) return TzdValue(c >= 0);
        return TzdValue(c <= 0);
    }

    double l = getAsDouble(left), r = getAsDouble(right);
    if (ctx->GT()) return TzdValue(l > r);
    if (ctx->LT()) return TzdValue(l < r);
    if (ctx->GE()) return TzdValue(l >= r);
    return TzdValue(l <= r);
}

std::any TzdInterpreter::visitEqualityExpr(TzdLangParser::EqualityExprContext* ctx) {
    TzdValue l = castAnyToTzdValue(visit(ctx->expression(0)), "visitEqualityExpr.l");
    TzdValue r = castAnyToTzdValue(visit(ctx->expression(1)), "visitEqualityExpr.r");
    bool eq = valuesEqual(l, r);
    return TzdValue(ctx->EEQ() ? eq : !eq);
}

std::any TzdInterpreter::visitLogicalAndExpr(TzdLangParser::LogicalAndExprContext* ctx) {
    TzdValue l = castAnyToTzdValue(visit(ctx->expression(0)), "visitLogicalAndExpr.l");
    if (!isTruthy(l)) return TzdValue(bool{ false });
    TzdValue r = castAnyToTzdValue(visit(ctx->expression(1)), "visitLogicalAndExpr.r");
    return TzdValue(bool{ isTruthy(r) });
}

std::any TzdInterpreter::visitLogicalOrExpr(TzdLangParser::LogicalOrExprContext* ctx) {
    TzdValue l = castAnyToTzdValue(visit(ctx->expression(0)), "visitLogicalOrExpr.l");
    if (isTruthy(l)) return TzdValue(bool{ true });
    TzdValue r = castAnyToTzdValue(visit(ctx->expression(1)), "visitLogicalOrExpr.r");
    return TzdValue(bool{ isTruthy(r) });
}

std::any TzdInterpreter::visitCastExpr(TzdLangParser::CastExprContext* ctx) {
    std::string targetType = ctx->typeType()->getText();
    TzdValue val = std::any_cast<TzdValue>(visit(ctx->expression()));

    try {
        if (targetType == "int" || targetType == "i32" || targetType == "long" || targetType == "i64") {
            long long res;
            if (val.type == TzdValue::FLOAT || val.type == TzdValue::DOUBLE) res = (long long)val.dVal;
            else if (val.type == TzdValue::ULONG) res = (long long)val.ulVal;
            else if (val.type == TzdValue::POINTER) res = (long long)(uintptr_t)val.ptrVal;
            else if (val.type == TzdValue::STRING) res = std::stoll(val.sVal, nullptr, 0);
            else if (val.type == TzdValue::BOOL) res = val.bVal ? 1LL : 0LL;
            else res = val.lVal;
            return (targetType == "int" || targetType == "i32") ? TzdValue((int)res) : TzdValue(res);
        }
        else if (targetType == "byte" || targetType == "u8" || targetType == "ulong" || targetType == "u64") {
            unsigned long long res;
            if (val.type == TzdValue::FLOAT || val.type == TzdValue::DOUBLE) res = (unsigned long long)val.dVal;
            else if (val.type == TzdValue::ULONG) res = val.ulVal;
            else if (val.type == TzdValue::POINTER) res = (uintptr_t)val.ptrVal;
            else if (val.type == TzdValue::STRING) res = std::stoull(val.sVal, nullptr, 0);
            else res = (unsigned long long)val.lVal;
            return (targetType == "byte" || targetType == "u8") ? TzdValue((unsigned char)res) : TzdValue(res);
        }
        else if (targetType == "ptr" || targetType == "pointer" || targetType == "hwnd") {
            if (val.type == TzdValue::ULONG) return TzdValue((void*)val.ulVal);
            if (val.type == TzdValue::STRING) return TzdValue((void*)std::stoull(val.sVal, nullptr, 0));
            return TzdValue((void*)(uintptr_t)getAsDouble(val));
        }
        else if (targetType == "float" || targetType == "double") {
            return TzdValue(getAsDouble(val));
        }
        else if (targetType == "string") return TzdValue(getAsString(val));
        else if (targetType == "bool") return TzdValue(getAsDouble(val) != 0);
    }
    catch (...) {
        throw TzdRuntimeException("[编译错误] 无法转换 '" + getAsString(val) + "' 为 " + targetType, ctx->getStart());
    }
    throw TzdRuntimeException("[编译错误] 不支持的目标类型： " + targetType, ctx->getStart());
}

// --- 字面量 ---
std::any TzdInterpreter::visitIntExpr(TzdLangParser::IntExprContext* ctx) {
    std::string raw = ctx->getText();
    TzdValue res;

    // Fast path: if the number has too many digits for int64 (>19), skip stoll
    // entirely — it reads the WHOLE string before throwing, which is O(n) for
    // multi-megabyte literals. ANTLR4 already validated the token as digits.
    size_t digitStart = (raw.size() > 0 && (raw[0] == '-' || raw[0] == '+')) ? 1 : 0;
    bool isHex = (raw.size() > 2 + digitStart && raw[digitStart] == '0' &&
                  (raw[digitStart+1] == 'x' || raw[digitStart+1] == 'X'));
    if (!isHex && raw.size() - digitStart > 19) {
        // Directly create BIGINT — no stoll, no validation loop
        res.type = TzdValue::BIGINT;
        res.sVal = std::move(raw);
        return res;
    }

    try {
        if (isHex) {
            res = TzdValue((void*)std::stoull(raw, nullptr, 16));
        }
        else {
            res = TzdValue(std::stoll(raw));
        }
    }
    catch (...) {
        // Overflow: check if it's a valid big integer (all digits, optional leading -)
        bool valid = true;
        size_t start = 0;
        if (raw[0] == '-') start = 1;
        if (start >= raw.size()) valid = false;
        for (size_t i = start; i < raw.size(); ++i) {
            if (raw[i] < '0' || raw[i] > '9') { valid = false; break; }
        }
        if (valid && raw.size() > start + 1) {
            // Store as BIGINT (arbitrary precision)
            res.type = TzdValue::BIGINT;
            res.sVal = std::move(raw);
        } else {
            res = TzdValue(std::stod(raw));
        }
    }

    return res;
}
std::any TzdInterpreter::visitFloatExpr(TzdLangParser::FloatExprContext* ctx) {
    TzdValue res(std::stod(ctx->getText()));
    return res;
}
std::any TzdInterpreter::visitBoolTrueExpr(TzdLangParser::BoolTrueExprContext* ctx) {
    TzdValue res(true);
    return res;
}
std::any TzdInterpreter::visitBoolFalseExpr(TzdLangParser::BoolFalseExprContext* ctx) { return TzdValue(false); }
std::any TzdInterpreter::visitStringExpr(TzdLangParser::StringExprContext* ctx) {
    std::string s = ctx->getText();
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'')) {
        s = s.substr(1, s.size() - 2);
    }
    TzdValue res(unescapeString(s));
    return res;
}

std::any TzdInterpreter::visitImportStmt(TzdLangParser::ImportStmtContext* ctx) {
    std::string rawPath = ctx->importStatement()->STRING()->getText();
    if (rawPath.size() >= 2) rawPath = rawPath.substr(1, rawPath.size() - 2);

    std::string absolutePath = resolveImportPath(rawPath);

    if (absolutePath.empty()) {
        throw TzdRuntimeException("[Import Error] 找不到模块或文件: " + rawPath, ctx->getStart());
    }

    if (m_importedFiles.count(absolutePath)) {
        return TzdValue(true);
    }

    m_importedFiles.insert(absolutePath);
    this->loadScriptFromFile(absolutePath);

    return TzdValue(true);
}

// --- Print & Math ---
std::any TzdInterpreter::visitPrintFunExpr(TzdLangParser::PrintFunExprContext* ctx) {
    auto exprList = ctx->printFunction()->exprList();
    if (exprList) {
        auto expressions = exprList->expression();
        for (size_t i = 0; i < expressions.size(); ++i) {
            std::any res = visit(expressions[i]);
            std::string utf8Str = getAsString(res);
            std::string consoleStr = Utf8ToAnsi(utf8Str);
            std::cout << consoleStr;
            if (i < expressions.size() - 1) {
                std::cout << " ";
            }
        }
    }
    std::cout << std::endl;
    return TzdValue();
}

std::any TzdInterpreter::visitClassDeclaration(TzdLangParser::ClassDeclarationContext* ctx) {
    // --- 1. 获取基础信息 ---
    std::string fullName = ctx->qualifiedName(0)->getText();
    std::string parentName = (ctx->qualifiedName().size() > 1) ? ctx->qualifiedName(1)->getText() : "";

    // --- 2. 校验与安全检查 ---
    if (TzdOopManager::getClass(fullName)) {
        throw TzdRuntimeException("类重定义: '" + fullName + "' 已存在", ctx->getStart());
    }

    TzdClassDef* newClass = new TzdClassDef(fullName);
    newClass->parentName = parentName;

    // 处理类级注解
    if (ctx->annotationUsage()) {
        validateAnnotationUsage(ctx->annotationUsage());
        newClass->annotations.push_back(ctx->annotationUsage()->IDENTIFIER()->getText());
    }

    // --- 3. 初始化 C 代码生成器变量 ---
    TzdValue classVal(newClass);
    classVal.annotations = newClass->annotations;

    // --- 4. 开启编译模式 (防止 visit 方法体时报错) ---
    bool oldMode = m_isCompiling;
    m_isCompiling = true;

    // --- 5. 遍历解析类成员 ---
    for (auto member : ctx->classBody()->classMember()) {

        // A. 校验成员上的注解
        if (member->annotationUsage()) {
            validateAnnotationUsage(member->annotationUsage());
        }

        // B. 处理原生方法 (Native Function)
        if (member->nativeFunctionDeclaration()) {
            auto nDecl = member->nativeFunctionDeclaration();
            std::string funcName = nDecl->IDENTIFIER()->getText();
            std::unordered_map<std::string, std::string> attrs;
            if (nDecl->nativeAttrList()) {
                for (auto* attr : nDecl->nativeAttrList()->nativeAttr()) {
                    std::string key = attr->children[0]->getText();
                    std::string val = attr->children.size() >= 3 ? attr->children[2]->getText() : "";
                    if (val.size() >= 2) val = val.substr(1, val.size() - 2);
                    attrs[key] = val;
                }
            }

            void* procAddr = nullptr;
            std::string dll = attrs["dll"];
            std::string realFun = attrs.count("fun") ? attrs["fun"] : funcName;
#ifdef _WIN32
            HMODULE hLib = LoadLibraryA(dll.c_str());
            if (hLib) procAddr = (void*)GetProcAddress(hLib, realFun.c_str());
#endif
            if (!procAddr) throw TzdRuntimeException("FFI 错误: 无法加载原生方法 " + realFun, nDecl->getStart());

            ClassMethod m;
            m.name = funcName;
            m.isNative = true;
            m.isStatic = (attrs["static"] == "true");
            m.nativeWrapper = TzdFFIAdapter::buildWrapper(procAddr, realFun, attrs["type"], attrs["return"]);
            newClass->methods[funcName] = m;
            continue;
        }

        auto decl = member->memberDecl();
        if (!decl) continue;

        // C. 处理实例字段 (var)
        if (auto varCtx = dynamic_cast<TzdLangParser::FieldVarDeclContext*>(decl)) {
            std::string typeName = varCtx->typeType()->getText();
            std::string fieldName = varCtx->IDENTIFIER()->getText();
            if (!isTypeValid(typeName) && typeName != fullName && typeName != newClass->simpleName) {
                throw TzdRuntimeException("未知类型: " + typeName, varCtx->typeType()->getStart());
            }

            ClassField f; f.name = fieldName; f.type = typeName;
            if (varCtx->expression()) f.initExpr = varCtx->expression();
            newClass->fields[fieldName] = f;
        }

        // D. 处理静态字段 (let)
        else if (auto letCtx = dynamic_cast<TzdLangParser::FieldLetDeclContext*>(decl)) {
            std::string typeName = letCtx->typeType()->getText();
            std::string fieldName = letCtx->IDENTIFIER()->getText();
            if (!isTypeValid(typeName)) throw TzdRuntimeException("未知类型: " + typeName, letCtx->typeType()->getStart());

            ClassField f; f.name = fieldName; f.type = typeName; f.isStatic = true;
            newClass->fields[fieldName] = f;

            TzdValue staticVal(0LL);
            if (letCtx->expression()) {
                staticVal = std::any_cast<TzdValue>(visit(letCtx->expression()));
            }
            newClass->staticValues[fieldName] = staticVal;
        }

        // E. 处理普通方法 (fun)
        else if (auto methodCtx = dynamic_cast<TzdLangParser::MethodDeclContext*>(decl)) {
            std::string mName = methodCtx->IDENTIFIER()->getText();
            ClassMethod m;
            m.name = mName;
            m.body = methodCtx->block();
            parseParamList(methodCtx->paramList(), m.params, m.paramTypes);

            m.sourceFile = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
            m.line = (int)methodCtx->getStart()->getLine();
            m.column = (int)methodCtx->getStart()->getCharPositionInLine();

            // --- JIT 编译部分 ---
            if (!m_noJit && m_jitEngine && m_compiler) {
                std::string jitName = fullName + "_" + mName;
                try {
                    m_compiler->compileClassMethod(ctx, methodCtx, jitName);
                    m_pendingJitFunctions.insert(jitName);
                }
                catch (const std::exception& e) {
                    agentLogCompile("C", "compileClassMethod", (jitName + ": " + e.what()).c_str());
                    throw;
                }
            }

            newClass->methods[mName] = m;
        }

        // F. 处理静态方法 (static fun)
        else if (auto smCtx = dynamic_cast<TzdLangParser::MethodStaticDeclContext*>(decl)) {
            std::string mName = smCtx->IDENTIFIER()->getText();
            ClassMethod m;
            m.name = mName;
            m.isStatic = true;
            m.body = smCtx->block();
            parseParamList(smCtx->paramList(), m.params, m.paramTypes);

            // --- JIT 编译部分 ---
            if (!m_noJit && m_jitEngine && m_compiler) {
                std::string jitName = fullName + "_" + mName;
                // 静态方法不需要 this
                m_compiler->compileNamedFunction(smCtx->block(), smCtx->paramList(), jitName);
                m_pendingJitFunctions.insert(jitName);
            }

            newClass->methods[mName] = m;
        }

        // G. 处理构造函数（支持重载：按参数个数区分）
        else if (auto ctorCtx = dynamic_cast<TzdLangParser::ConstructorDeclContext*>(decl)) {
            std::string ctorName = ctorCtx->IDENTIFIER()->getText();
            if (ctorName != newClass->simpleName) {
                throw TzdRuntimeException("构造函数名 '" + ctorName + "' 必须与类名一致", ctorCtx->getStart());
            }

            ClassConstructor ctorInfo;
            ctorInfo.declCtx = ctorCtx;
            ctorInfo.body = ctorCtx->block();
            parseParamList(ctorCtx->paramList(), ctorInfo.params, ctorInfo.paramTypes);
            ctorInfo.paramCount = (int)ctorInfo.params.size();
            ctorInfo.sourceFile = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
            ctorInfo.line = (int)ctorCtx->getStart()->getLine();
            ctorInfo.column = (int)ctorCtx->getStart()->getCharPositionInLine();

            for (const auto& existing : newClass->constructors) {
                if (existing.paramCount == ctorInfo.paramCount) {
                    throw TzdRuntimeException(
                        "构造函数重载冲突: '" + fullName + "' 已有 " +
                        std::to_string(ctorInfo.paramCount) + " 个参数的构造函数",
                        ctorCtx->getStart());
                }
            }

            if (!m_noJit && m_jitEngine && m_compiler) {
                std::string jitName = fullName + "_" + ctorName + "_ctor_" + std::to_string(ctorInfo.paramCount);
                ctorInfo.jitSymbolName = jitName;
                try {
                    m_compiler->compileConstructor(ctx, ctorCtx, jitName);
                    m_pendingJitFunctions.insert(jitName);
                }
                catch (const std::exception& e) {
                    agentLogCompile("C", "compileConstructor", (jitName + ": " + e.what()).c_str());
                    throw;
                }
            }

            ClassMethod m;
            m.name = ctorName;
            m.body = ctorCtx->block();
            m.params = ctorInfo.params;
            m.sourceFile = ctorInfo.sourceFile;
            m.line = ctorInfo.line;
            m.column = ctorInfo.column;

            newClass->methods[ctorName] = m;

            newClass->constructors.push_back(std::move(ctorInfo));
        }
    }

    std::string classSource = m_scriptPathStack.empty() ? "memory" : m_scriptPathStack.back().string();
    for (auto& ctor : newClass->constructors) {
        if (ctor.sourceFile.empty() || ctor.sourceFile == "memory") {
            ctor.sourceFile = classSource;
        }
    }
    for (auto& [name, method] : newClass->methods) {
        if (method.sourceFile.empty() || method.sourceFile == "memory") {
            method.sourceFile = classSource;
        }
    }

    // --- 7. 注册并存入作用域 ---
    TzdOopManager::registerClass(newClass);
    setVariable(fullName, classVal);

    if (newClass->simpleName != fullName) {
        setVariable(newClass->simpleName, classVal);
    }

    return classVal;
}

void TzdInterpreter::validateAnnotationUsage(TzdLangParser::AnnotationUsageContext* ctx) {
    if (!ctx) return;

    std::string annoName = ctx->IDENTIFIER()->getText();
    TzdClassDef* cls = TzdOopManager::getClass(annoName);

    if (!cls || !cls->isAnnotation) {
        throw TzdRuntimeException(
            "未定义的注解: '@" + annoName + "'。请先定义该注解，例如: class @" + annoName + "();",
            ctx->getStart()
        );
    }
}

bool TzdInterpreter::isTypeValid(const std::string& typeName) {
    // 1. 基础类型白名单
    static const std::unordered_set<std::string> primitives = {
         "int", "float", "string", "bool", "long", "ptr", "void",
         "i32", "i64", "double", "byte", "u8", "ulong", "hwnd",
         "function", "fn"
    };
    if (primitives.count(typeName)) return true;

    // 2. 检查是否是已注册的类、枚举或注解
    if (TzdOopManager::getClass(typeName) != nullptr) return true;

    return false;
}

bool TzdInterpreter::checkParamValueType(const std::string& typeName, const TzdValue& val) {
    if (typeName.empty()) return true;
    if (typeName == "int" || typeName == "i32" || typeName == "long" || typeName == "i64" ||
        typeName == "float" || typeName == "double" || typeName == "byte" || typeName == "u8") {
        return val.type == TzdValue::INT || val.type == TzdValue::LONG ||
            val.type == TzdValue::FLOAT || val.type == TzdValue::DOUBLE ||
            val.type == TzdValue::SHORT || val.type == TzdValue::BYTE;
    }
    if (typeName == "function" || typeName == "fn") {
        return val.type == TzdValue::FUNCTION || val.type == TzdValue::NATIVE_FUNCTION;
    }
    if (typeName == "string") return val.type == TzdValue::STRING;
    if (typeName == "bool") return val.type == TzdValue::BOOL;
    if (typeName == "ptr" || typeName == "void" || typeName == "hwnd")
        return val.type == TzdValue::POINTER || val.type == TzdValue::NONE;
    if (val.type == TzdValue::INSTANCE && val.instanceVal && val.instanceVal->definition) {
        return val.instanceVal->definition->fullName == typeName ||
            val.instanceVal->definition->simpleName == typeName ||
            val.instanceVal->definition->isSubclassOf(typeName);
    }
    if (val.type == TzdValue::CLASS_DEF && val.classDefVal) {
        return val.classDefVal->fullName == typeName || val.classDefVal->simpleName == typeName;
    }
    return false;
}

// --- 实例化 (New) ---
std::any TzdInterpreter::visitNewExpr(TzdLangParser::NewExprContext* ctx) {
    if (!ctx->qualifiedName()) throw TzdRuntimeException("New 表达式缺失类名", ctx->getStart());
    std::string className = ctx->qualifiedName()->getText();

    TzdClassDef* cls = TzdOopManager::getClass(className);
    if (!cls) throw TzdRuntimeException("找不到类定义: " + className, ctx->getStart());

    TzdInstance* inst = new TzdInstance(cls);
    TzdValue instVal(inst);

    std::unordered_map<std::string, TzdValue> ctorScope;
    ctorScope["this"] = instVal;
    scopes.push_back(ctorScope);

    try {
        // --- 执行字段初始化器 (变量定义时的赋值) ---
        TzdClassDef* cur = cls;
        std::vector<TzdClassDef*> hierarchy;
        while (cur) {
            hierarchy.insert(hierarchy.begin(), cur);
            if (cur->parentName.empty()) break;
            cur = TzdOopManager::getClass(cur->parentName);
        }
        for (auto* currentCls : hierarchy) {
            for (auto const& [name, field] : currentCls->fields) {
                if (!field.isStatic && field.initExpr) {
                    TzdValue initVal = std::any_cast<TzdValue>(visit(field.initExpr));
                    inst->setMember(name, initVal);
                    scopes.back()[name] = initVal;
                }
            }
        }

        // --- 执行构造函数（按实参个数匹配重载） ---
        std::vector<TzdValue> args;
        if (ctx->exprList()) {
            for (auto expr : ctx->exprList()->expression()) {
                args.push_back(std::any_cast<TzdValue>(visit(expr)));
            }
        }

        ClassConstructor* ctor = cls->findConstructor(args.size());
        if (!ctor && args.empty() && !cls->constructors.empty()) {
            ctor = cls->findConstructor(0);
        }
        if (!ctor && !cls->constructors.empty()) {
            throw TzdRuntimeException(
                "找不到匹配的构造函数: '" + className + "' 需要 " +
                std::to_string(args.size()) + " 个参数",
                ctx->getStart());
        }
        if (ctor) {
            TzdValue ctorFunc(cls->simpleName, ctor->params, ctor->body);
            ctorFunc.jittedPtr = ctor->jittedPtr;
            ctorFunc.setInstance(inst);
            this->callFunction(ctorFunc, args);
        }
    }
    catch (...) {
        scopes.pop_back();
        throw;
    }
    scopes.pop_back();
    return instVal;
}

// --- 成员访问 (.) ---
// Resolve (and cache) the TzdSelector + member name for a member-access AST
// site. The cache is keyed by the AST node pointer and is stable for the
// lifetime of the loaded module; the selector/name are immutable once
// interned. Only the selector and name are cached — never an unguarded
// receiver/method pointer, since the receiver type at a site may change.
MemberAccessSiteCache& TzdInterpreter::resolveMemberAccessSite(
    antlr4::ParserRuleContext* ctx, const std::string& memberName) {
    auto it = m_memberSelectorCache.find(ctx);
    if (it != m_memberSelectorCache.end() && it->second.resolved) {
        return it->second;
    }
    auto& entry = m_memberSelectorCache[ctx];
    entry.name = memberName;
    entry.selector = tzdInternSelector(memberName);
    entry.resolved = true;
    return entry;
}

std::any TzdInterpreter::visitMemberAccessExpr(TzdLangParser::MemberAccessExprContext* ctx) {

    // 1. 获取左侧 atom 的原始文本 (比如 "this" 或 "t2")
    std::string leftText = ctx->atom()->getText();

    // 2. 解析左侧基础对象
    TzdValue base;
    bool baseResolved = false;
    try {
        std::any val = visit(ctx->atom());
        if (val.has_value()) {
            base = std::any_cast<TzdValue>(val);
            baseResolved = true;
        }
    }
    catch (...) {
        baseResolved = false;
    }

    std::string memberName = ctx->IDENTIFIER()->getText();
    // Intern once per AST site; subsequent visits skip re-extraction.
    MemberAccessSiteCache& site = resolveMemberAccessSite(ctx, memberName);
    TzdSelector selector = site.selector;

    if (baseResolved) {
        // --- 情况 A: 静态成员访问 (Class.staticVar) ---
        if (base.type == TzdValue::CLASS_DEF && base.classDefVal) {
            TzdClassDef* cls = base.classDefVal;
            // Selector-based dispatch table lookup (immutable once registered).
            if (const TzdMemberSlot* slot = cls->tzdDispatch.find(selector)) {
                if (slot->staticValue) {
                    TzdValue res = *slot->staticValue;
                    return res;
                }
                if (slot->method && slot->method->isStatic) {
                    TzdValue f;
                    f.type = TzdValue::FUNCTION;
                    f.name = slot->method->name;
                    f.params = slot->method->params;
                    f.funcBody = slot->method->body;
                    return f;
                }
            }
            // Fallback to string-based lookup (bridges dispatch-table gaps).
            if (cls->staticValues.count(memberName)) {
                TzdValue res = cls->staticValues[memberName];
                return res;
            }
            ClassMethod* method = cls->findMethod(memberName);
            if (method && method->isStatic) {
                TzdValue f;
                f.type = TzdValue::FUNCTION;
                f.name = method->name;
                f.params = method->params;
                f.funcBody = method->body;
                return f;
            }
        }

        // --- 情况 B: 实例成员访问 (obj.var 或 this.var) ---
        // 关键点：不仅检查 INSTANCE 类型，还检查 instanceVal 是否存在（防止类型标识丢失）
        if ((base.type == TzdValue::INSTANCE || base.instanceVal != nullptr) && base.type != TzdValue::CLASS_DEF) {
            try {
                // Selector-based member lookup; precedence: instance field,
                // inherited static value, then method. The string name is
                // passed alongside for error messages / package fallback.
                return base.instanceVal->getMember(selector, memberName);
            }
            catch (const std::exception& e) {
                // 如果 getMember 抛出错误，直接向上抛出详细的 OOP 错误，而不是被末尾覆盖
                throw TzdRuntimeException(e.what(), ctx->getStart());
            }
        }
    }

    // --- 情况 C: 符号降级（处理包名 org.tzd.Test） ---
    std::string combinedPath = leftText + "." + memberName;
    TzdClassDef* cls = TzdOopManager::getClass(combinedPath);
    if (cls) {
        TzdValue classRes(cls);
        return classRes;
    }

    // --- 情况 D: 最终报错 ---
    throw TzdRuntimeException("无法解析符号: " + (baseResolved ? (leftText + "." + memberName) : combinedPath), ctx->getStart());
}

// --- 类型检查 (in) ---
std::any TzdInterpreter::visitTypeCheckExpr(TzdLangParser::TypeCheckExprContext* ctx) {
    TzdValue obj = std::any_cast<TzdValue>(visit(ctx->expression()));
    // 获取 targetType，例如 "int" 或 "float"
    std::string targetType = ctx->qualifiedName() ? ctx->qualifiedName()->getText() : ctx->typeType()->getText();
    std::string targetName = ctx->qualifiedName() ? ctx->qualifiedName()->getText() : ctx->typeType()->getText();

    // 1. 检查目标是否是一个"注解类"
    TzdClassDef* targetCls = TzdOopManager::getClass(targetName);
    bool isTargetAnnotation = (targetCls && targetCls->isAnnotation);

    if (isTargetAnnotation) {
        // --- 【关键修复：判断注解包含关系】 ---
        for (const auto& anno : obj.annotations) {
            if (anno == targetName) return TzdValue(true);
        }
        return TzdValue(false);
    }

    // 1. 严格检查浮点类型
    if (targetType == "float" || targetType == "double") {
        // 只有值本身确实是 FLOAT 或 DOUBLE 类型时才返回 true
        return TzdValue(obj.type == TzdValue::FLOAT || obj.type == TzdValue::DOUBLE);
    }

    // 2. 严格检查整数类型
    if (targetType == "int" || targetType == "long" || targetType == "i32" || targetType == "i64") {
        // 检查是否属于整数枚举范围 (SBYTE 到 ULONG)
        return TzdValue(obj.type >= TzdValue::SBYTE && obj.type <= TzdValue::ULONG);
    }

    // 3. 检查字符串
    if (targetType == "string") return TzdValue(obj.type == TzdValue::STRING);

    // 4. 类实例检查 (必须使用我们在上一步修复的严格 isInstanceOf)
    if (obj.type == TzdValue::INSTANCE) {
        return TzdValue(TzdOopManager::isInstanceOf(obj.instanceVal, targetType));
    }

    return TzdValue(false);
}

void TzdInterpreter::checkSymbolCollision(const std::string& name, antlr4::ParserRuleContext* ctx) {
    // 1. 检查当前作用域是否有同名变量/函数
    if (scopes.back().count(name)) {
        throw TzdRuntimeException("符号重定义: '" + name + "' 已在当前作用域定义", ctx->getStart());
    }
    // 2. 检查是否与已注册的类名/枚举名/注解名冲突
    if (TzdOopManager::getClass(name)) {
        throw TzdRuntimeException("符号冲突: '" + name + "' 已被定义为类、枚举或注解", ctx->getStart());
    }
}

// --- 注解声明 ---
std::any TzdInterpreter::visitAnnotationDeclaration(TzdLangParser::AnnotationDeclarationContext* ctx) {
    std::string name = ctx->IDENTIFIER()->getText();
    TzdClassDef* anno = new TzdClassDef(name);
    anno->isAnnotation = true;

    TzdOopManager::registerClass(anno);
    setVariable(name, TzdValue(anno));
    return TzdValue();
}

// --- 枚举 ---
std::any TzdInterpreter::visitEnumDeclaration(TzdLangParser::EnumDeclarationContext* ctx) {
    std::string enumName = ctx->IDENTIFIER()->getText();
    TzdClassDef* enumClass = new TzdClassDef(enumName);
    enumClass->isEnum = true;

    if (ctx->enumList()) {
        int idx = 0;
        for (auto idNode : ctx->enumList()->IDENTIFIER()) {
            std::string itemName = idNode->getText();
            TzdValue itemVal((long long)idx++);
            enumClass->staticValues[itemName] = itemVal;
        }
    }
    TzdOopManager::registerClass(enumClass);
    setVariable(enumName, TzdValue(enumClass));
    return TzdValue();
}

// --- Super 调用 (实现 visitSuperExpr) ---
std::any TzdInterpreter::visitSuperExpr(TzdLangParser::SuperExprContext* ctx) {
    antlr4::tree::ParseTree* p = ctx->parent;
    TzdLangParser::ClassDeclarationContext* classDeclCtx = nullptr;

    // 向上查找当前所在的类定义
    while (p != nullptr) {
        classDeclCtx = dynamic_cast<TzdLangParser::ClassDeclarationContext*>(p);
        if (classDeclCtx) break;
        p = p->parent;
    }

    if (!classDeclCtx) {
        throw TzdRuntimeException("super() 必须在类定义的内部使用", ctx->getStart());
    }

    // 修改：使用 qualifiedName(0) 获取当前类全名
    std::string currentClassName = classDeclCtx->qualifiedName(0)->getText();
    TzdClassDef* currentClass = TzdOopManager::getClass(currentClassName);

    if (!currentClass || currentClass->parentName.empty()) {
        throw TzdRuntimeException("类 '" + currentClassName + "' 没有父类，无法调用 super()", ctx->getStart());
    }

    TzdClassDef* parentClass = TzdOopManager::getClass(currentClass->parentName);
    if (!parentClass) {
        throw TzdRuntimeException("找不到父类定义: " + currentClass->parentName, ctx->getStart());
    }

    TzdValue thisVal;
    try {
        thisVal = getVariable("this", ctx);
    }
    catch (...) {
        throw TzdRuntimeException("super() 调用上下文丢失 'this' 指针", ctx->getStart());
    }

    if (parentClass->constructors.empty()) {
        if (ctx->exprList() && !ctx->exprList()->expression().empty()) {
            throw TzdRuntimeException("父类 '" + parentClass->fullName + "' 无构造函数，但 super() 传入了参数", ctx->getStart());
        }
        return TzdValue();
    }

    std::vector<TzdValue> args;
    if (ctx->exprList()) {
        for (auto expr : ctx->exprList()->expression()) {
            args.push_back(std::any_cast<TzdValue>(visit(expr)));
        }
    }

    const ClassConstructor* parentCtor = parentClass->findConstructor(args.size());
    if (!parentCtor && !parentClass->constructors.empty()) {
        throw TzdRuntimeException("super() 找不到匹配的父类构造函数", ctx->getStart());
    }
    if (!parentCtor) {
        return TzdValue();
    }

    TzdValue ctorFunc(parentClass->simpleName, parentCtor->params, parentCtor->body);
    ctorFunc.jittedPtr = parentCtor->jittedPtr;
    ctorFunc.setInstance(thisVal.instanceVal);

    std::unordered_map<std::string, TzdValue> superScope;
    superScope["this"] = thisVal;
    for (size_t i = 0; i < args.size(); ++i) {
        superScope[parentCtor->params[i]] = args[i];
    }

    scopes.push_back(superScope);
    try {
        callFunction(ctorFunc, args);
    }
    catch (...) {
        scopes.pop_back();
        throw;
    }
    scopes.pop_back();

    return TzdValue();
}


std::any TzdInterpreter::visitCallExpr(TzdLangParser::CallExprContext* ctx) {
    // 1. 检测是否为直接成员调用 obj.method(args) 或 Class.method(args)。
    //    若是且接收者为实例/类，则直接分发，避免构造并拷贝 bound TzdValue。
    auto* memAccess = dynamic_cast<TzdLangParser::MemberAccessExprContext*>(ctx->atom());

    if (memAccess) {
        // callee-before-args: 先解析接收者，再求值实参。
        TzdValue base;
        bool baseResolved = false;
        try {
            std::any val = visit(memAccess->atom());
            if (val.has_value()) {
                base = std::any_cast<TzdValue>(val);
                baseResolved = true;
            }
        }
        catch (...) {
            baseResolved = false;
        }

        if (baseResolved) {
            std::string memberName = memAccess->IDENTIFIER()->getText();
            MemberAccessSiteCache& site = resolveMemberAccessSite(memAccess, memberName);
            TzdSelector selector = site.selector;

            // --- 实例成员直接调用 ---
            if ((base.type == TzdValue::INSTANCE || base.instanceVal != nullptr) &&
                base.type != TzdValue::CLASS_DEF && base.instanceVal != nullptr) {
                TzdInstance* inst = base.instanceVal;
                const TzdMemberSlot* slot =
                    inst->definition ? inst->definition->tzdDispatch.find(selector) : nullptr;

                // callable fields take precedence over methods.
                if (slot && slot->fieldIndex >= 0) {
                    TzdValue* fieldPtr = &inst->fieldValues[slot->fieldIndex];
                    if (fieldPtr->type == TzdValue::FUNCTION ||
                        fieldPtr->type == TzdValue::NATIVE_FUNCTION) {
                        std::vector<TzdValue> args;
                        if (ctx->exprList()) {
                            for (auto expr : ctx->exprList()->expression())
                                args.push_back(std::any_cast<TzdValue>(visit(expr)));
                        }
                        try { return callFunction(*fieldPtr, args); }
                        catch (const TzdReturnException& e) { return e.value; }
                        catch (const TzdRuntimeException&) { throw; }
                        catch (const TzdThrowException&) { throw; }
                        catch (const std::exception& e) {
                            throw TzdRuntimeException(e.what(), ctx->getStart());
                        }
                    }
                }
                if (slot && slot->staticValue &&
                    (slot->staticValue->type == TzdValue::FUNCTION ||
                     slot->staticValue->type == TzdValue::NATIVE_FUNCTION)) {
                    std::vector<TzdValue> args;
                    if (ctx->exprList()) {
                        for (auto expr : ctx->exprList()->expression())
                            args.push_back(std::any_cast<TzdValue>(visit(expr)));
                    }
                    try { return callFunction(*slot->staticValue, args); }
                    catch (const TzdReturnException& e) { return e.value; }
                    catch (const TzdRuntimeException&) { throw; }
                    catch (const TzdThrowException&) { throw; }
                    catch (const std::exception& e) {
                        throw TzdRuntimeException(e.what(), ctx->getStart());
                    }
                }

                // 方法：直接调用，避免 bound TzdValue 拷贝。
                if (slot && slot->method) {
                    std::vector<TzdValue> args;
                    if (ctx->exprList()) {
                        for (auto expr : ctx->exprList()->expression())
                            args.push_back(std::any_cast<TzdValue>(visit(expr)));
                    }
                    try { return callMethod(*slot->method, inst, args); }
                    catch (const TzdReturnException& e) { return e.value; }
                    catch (const TzdRuntimeException&) { throw; }
                    catch (const TzdThrowException&) { throw; }
                    catch (const std::exception& e) {
                        throw TzdRuntimeException(e.what(), ctx->getStart());
                    }
                }

                // 分发表未命中：回退到 getMember(selector, name)（已避免重复求值接收者）。
                TzdValue bound;
                try {
                    bound = inst->getMember(selector, memberName);
                }
                catch (const std::exception& e) {
                    throw TzdRuntimeException(e.what(), ctx->getStart());
                }
                std::vector<TzdValue> args;
                if (ctx->exprList()) {
                    for (auto expr : ctx->exprList()->expression())
                        args.push_back(std::any_cast<TzdValue>(visit(expr)));
                }
                try { return callFunction(bound, args); }
                catch (const TzdReturnException& e) { return e.value; }
                catch (const TzdRuntimeException&) { throw; }
                catch (const TzdThrowException&) { throw; }
                catch (const std::exception& e) {
                    throw TzdRuntimeException(e.what(), ctx->getStart());
                }
            }

            // --- 类静态成员直接调用 ---
            if (base.type == TzdValue::CLASS_DEF && base.classDefVal) {
                TzdClassDef* cls = base.classDefVal;
                const TzdMemberSlot* slot = cls->tzdDispatch.find(selector);

                // callable static field precedence.
                if (slot && slot->staticValue &&
                    (slot->staticValue->type == TzdValue::FUNCTION ||
                     slot->staticValue->type == TzdValue::NATIVE_FUNCTION)) {
                    std::vector<TzdValue> args;
                    if (ctx->exprList()) {
                        for (auto expr : ctx->exprList()->expression())
                            args.push_back(std::any_cast<TzdValue>(visit(expr)));
                    }
                    try { return callFunction(*slot->staticValue, args); }
                    catch (const TzdReturnException& e) { return e.value; }
                    catch (const TzdRuntimeException&) { throw; }
                    catch (const TzdThrowException&) { throw; }
                    catch (const std::exception& e) {
                        throw TzdRuntimeException(e.what(), ctx->getStart());
                    }
                }
                if (slot && slot->method && slot->method->isStatic) {
                    std::vector<TzdValue> args;
                    if (ctx->exprList()) {
                        for (auto expr : ctx->exprList()->expression())
                            args.push_back(std::any_cast<TzdValue>(visit(expr)));
                    }
                    try { return callMethod(*slot->method, nullptr, args); }
                    catch (const TzdReturnException& e) { return e.value; }
                    catch (const TzdRuntimeException&) { throw; }
                    catch (const TzdThrowException&) { throw; }
                    catch (const std::exception& e) {
                        throw TzdRuntimeException(e.what(), ctx->getStart());
                    }
                }

                // 分发表未命中：回退到字符串查找（已避免重复求值接收者）。
                TzdValue val;
                bool found = false;
                if (cls->staticValues.count(memberName)) {
                    val = cls->staticValues[memberName];
                    found = true;
                }
                if (!found) {
                    ClassMethod* m = cls->findMethod(memberName);
                    if (m && m->isStatic) {
                        val.type = TzdValue::FUNCTION;
                        val.name = m->name;
                        val.params = m->params;
                        val.funcBody = m->body;
                        found = true;
                    }
                }
                if (found) {
                    std::vector<TzdValue> args;
                    if (ctx->exprList()) {
                        for (auto expr : ctx->exprList()->expression())
                            args.push_back(std::any_cast<TzdValue>(visit(expr)));
                    }
                    try { return callFunction(val, args); }
                    catch (const TzdReturnException& e) { return e.value; }
                    catch (const TzdRuntimeException&) { throw; }
                    catch (const TzdThrowException&) { throw; }
                    catch (const std::exception& e) {
                        throw TzdRuntimeException(e.what(), ctx->getStart());
                    }
                }
            }
            // base 已解析但非实例/类：落入普通路径（包名降级由 visitMemberAccessExpr 处理）。
        }
        // base 未解析（包名场景）：落入普通路径。
    }

    // 2. 普通调用路径 (非成员调用、逃逸的 bound 方法、包名降级等均保持原行为)
    TzdValue funcVal = std::any_cast<TzdValue>(visit(ctx->atom()));
    std::string funcNameForError = ctx->atom()->getText();

    std::vector<TzdValue> args;
    if (ctx->exprList()) {
        for (auto expr : ctx->exprList()->expression()) {
            args.push_back(std::any_cast<TzdValue>(visit(expr)));
        }
    }

    try {
        return callFunction(funcVal, args);
    }
    catch (const TzdReturnException& e) {
        return e.value;
    }
    catch (const TzdRuntimeException& e) {
        throw;
    }
    catch (const TzdThrowException& e) {
        // 抛出的 Error 异常也同样直接向上穿透
        throw;
    }
    catch (const std::exception& e) {
        // 只有那些未被包装过的底层 C++ 异常，才在这里绑定当前调用的 AST 上下文
        throw TzdRuntimeException(e.what(), ctx->getStart());
    }
}

void TzdErrorListener::syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol,
    size_t line, size_t charPositionInLine,
    const std::string& msg, std::exception_ptr e) {

    std::string translatedMsg = msg;
    if (msg.find("mismatched input") != std::string::npos) translatedMsg = "输入符号不匹配 (语法错误)";
    else if (msg.find("extraneous input") != std::string::npos) translatedMsg = "发现多余的输入字符";
    else if (msg.find("missing") != std::string::npos) {
        size_t pos = msg.find("missing");
        translatedMsg = "缺少" + msg.substr(pos + 7);
    }
    else if (msg.find("no viable alternative") != std::string::npos) translatedMsg = "无法识别的语法结构";

    std::string sourceText = "";
    antlr4::Parser* parser = dynamic_cast<antlr4::Parser*>(recognizer);
    if (parser) {
        sourceText = parser->getTokenStream()->getTokenSource()->getInputStream()->toString();
    }

    TzdErrorHandler::report("Tzd 语法错误", line, charPositionInLine, translatedMsg, sourceText);
}

void TzdErrorHandler::report(const std::string& type, size_t line, size_t column, const std::string& msg, const std::string& sourceCode, const std::vector<std::string>& stackTrace)
{
    std::cerr << "Exception in thread \"main\" " << type << ": " << msg << std::endl;

    for (auto it = stackTrace.rbegin(); it != stackTrace.rend(); ++it) {
        std::cerr << "\tat " << *it << std::endl;
    }

    if (!sourceCode.empty() && line > 0) {
        std::istringstream iss(sourceCode);
        std::string codeLine;
        size_t currentLine = 1;
        while (std::getline(iss, codeLine)) {
            if (!codeLine.empty() && codeLine.back() == '\r') codeLine.pop_back();

            if (currentLine == line) {
                std::cerr << "    " << codeLine << std::endl;
                std::cerr << "    ";
                for (size_t i = 0; i < column; ++i) {
                    if (i < codeLine.size() && codeLine[i] == '\t') std::cerr << '\t';
                    else std::cerr << ' ';
                }
                std::cerr << "^--- 这里" << std::endl;
                break;
            }
            currentLine++;
        }
    }
}

std::any TzdInterpreter::visitNullExpr(TzdLangParser::NullExprContext* ctx) {
    return TzdValue();
}

// 2. 实现 tryJitCompile: on-demand JIT compilation for hot bytecode functions.
// Creates a fresh TzdCompiler, compiles the function's AST body to LLVM IR,
// adds the module to the JIT engine, and sets funcVal.jittedPtr.
void TzdInterpreter::tryJitCompile(TzdValue& funcVal) {
    if (!m_jitEngine || !funcVal.funcBody || funcVal.type != TzdValue::FUNCTION) return;
    if (funcVal.jittedPtr) return; // Already compiled

    // Generate a unique internal name matching visitFunctionDeclaration's
    // convention (funcName_vN) so version-stripping yields the user-visible
    // name, enabling rt_get_worker_ptr and s_compiledWorkers to find it.
    std::string internalName = funcVal.name + "_v" + std::to_string(m_funcVersion++);

    // Save and restore global state so compilation doesn't corrupt the
    // interpreter's current JIT module (if any pending functions exist).
    auto savedCompiler = std::move(m_compiler);
    auto savedPending = std::move(m_pendingJitFunctions);
    auto savedJitMap = std::move(m_jitNameToUserMap);

    // Create a fresh compiler for this single function
    m_compiler = std::make_unique<TzdCompiler>(*m_jitEngine, "BCJit_" + internalName);
    m_compiler->setupExternalFunctions();

    // Set g_CurrentInterpreter so nested function registrations work
    auto* savedGlobalInterp = g_CurrentInterpreter;
    g_CurrentInterpreter = this;

    bool compileOk = false;
    try {
        m_compiler->compileNamedFunction(funcVal.funcBody, funcVal.params, internalName);
        compileOk = true;
    } catch (...) {
        compileOk = false;
    }

    g_CurrentInterpreter = savedGlobalInterp;

    if (!compileOk) {
        // Restore saved state
        m_compiler = std::move(savedCompiler);
        m_pendingJitFunctions = std::move(savedPending);
        m_jitNameToUserMap = std::move(savedJitMap);
        return;
    }

    // Extract module and add to JIT engine (triggers LLVM compilation)
    auto TSM = m_compiler->extractThreadSafeModule();
    if (TSM) {
        m_jitEngine->addModule(std::move(TSM));
    }

    // Look up the compiled symbol and link it to funcVal
    auto symOrErr = m_jitEngine->lookupSymbol(internalName);
    if (symOrErr) {
        void* addr = reinterpret_cast<void*>(symOrErr->getValue());
        funcVal.jittedPtr = reinterpret_cast<void(*)(void*, void*)>(addr);
        funcVal.jitInternalName = internalName;

        // Register worker pointer for TCO cross-function calls
        m_jitEngine->registerWorkerForSymbol(internalName);
    } else {
        llvm::consumeError(symOrErr.takeError());
    }

    // Restore saved JIT state (the fresh compiler is discarded)
    m_compiler = std::move(savedCompiler);
    m_pendingJitFunctions = std::move(savedPending);
    m_jitNameToUserMap = std::move(savedJitMap);
}

// Background compilation: thread-safe, independent LLVMContext.
// Does NOT touch interpreter scopes or funcVal — caller links the result.
void* TzdInterpreter::compileFunctionInBackground(const std::string& funcName,
                                                   TzdLangParser::BlockContext* funcBody,
                                                   const std::vector<std::string>& params) {
    if (!m_jitEngine || !funcBody) return nullptr;

    // Generate unique internal name (funcName_vN convention for worker lookup)
    std::string internalName = funcName + "_v" + std::to_string(m_funcVersion++);

    // Create a fresh compiler with its own LLVMContext — fully thread-safe
    TzdCompiler compiler(*m_jitEngine, "AsyncJIT_" + internalName);
    compiler.setupExternalFunctions();

    // Compile the function body to LLVM IR
    try {
        compiler.compileNamedFunction(funcBody, params, internalName);
    } catch (...) {
        return nullptr;
    }

    // Extract and add module to JIT engine (LLJIT serializes addModule internally)
    auto TSM = compiler.extractThreadSafeModule();
    if (!TSM) return nullptr;
    m_jitEngine->addModule(std::move(TSM));

    // Look up the compiled symbol
    auto symOrErr = m_jitEngine->lookupSymbol(internalName);
    if (!symOrErr) {
        llvm::consumeError(symOrErr.takeError());
        return nullptr;
    }

    void* addr = reinterpret_cast<void*>(symOrErr->getValue());

    // Register worker pointer for recursive call optimization
    m_jitEngine->registerWorkerForSymbol(internalName);

    return addr;
}

static double GetAsDouble(TzdValue* v) {
    switch (v->type) {
    case TzdValue::DOUBLE: return v->dVal;
    case TzdValue::FLOAT:  return v->dVal;
    case TzdValue::LONG:   return (double)v->lVal;
    case TzdValue::ULONG:  return (double)v->ulVal;
    case TzdValue::INT:    return (double)v->lVal;
    case TzdValue::UINT:   return (double)v->ulVal;
    case TzdValue::SHORT:  return (double)v->lVal;
    case TzdValue::USHORT: return (double)v->ulVal;
    case TzdValue::SBYTE:  return (double)v->lVal;
    case TzdValue::BYTE:   return (double)v->ulVal;
    case TzdValue::BOOL:   return v->bVal ? 1.0 : 0.0;
    default: return 0.0;
    }
}

TzdValue TzdInterpreter::executeFunction(const TzdValue& funcVal, const std::vector<TzdValue>& args) {
    TzdValue finalFunc = funcVal;

    // --- 新增：自动从类定义中提取静态方法 ---
    if (finalFunc.type == TzdValue::CLASS_DEF && !args.empty()) {
        // 如果 funcVal 是类且被调用，尝试查找其构造函数或静态方法
        // 这里的逻辑可以根据你的需求调整，通常在 visit 层就该处理好
    }

    // 1. 处理原生函数 (NATIVE_FUNCTION)
    if (finalFunc.type == TzdValue::NATIVE_FUNCTION) {
        return finalFunc.nativeFunc(args);
    }

    // 2. 处理脚本函数 (FUNCTION)
    if (finalFunc.type == TzdValue::FUNCTION) {
        // 检查参数数量
        if (args.size() != finalFunc.params.size()) {
            throw std::runtime_error("函数调用参数不匹配: " + finalFunc.name +
                " 需要 " + std::to_string(finalFunc.params.size()) + " 个");
        }

        // --- 准备作用域 ---
        std::unordered_map<std::string, TzdValue> newScope;

        // 如果是类实例方法，绑定 "this"
        if (finalFunc.instanceVal != nullptr) {
            newScope["this"] = TzdValue(finalFunc.instanceVal);
        }

        // 绑定参数
        for (size_t i = 0; i < args.size(); ++i) {
            newScope[finalFunc.params[i]] = args[i];
        }

        scopes.push_back(newScope);
        TzdValue returnValue;

        try {
            if (finalFunc.funcBody) {
                visit(finalFunc.funcBody);
            }
        }
        catch (const TzdReturnException& ret) {
            returnValue = ret.value;
        }
        catch (...) {
            scopes.pop_back();
            throw;
        }

        scopes.pop_back();
        return returnValue;
    }

    // --- 调试信息：报错时打印实际收到的类型 ---
    std::string typeNames[] = { "NONE", "SBYTE", "BYTE", "SHORT", "USHORT", "INT", "UINT", "LONG", "ULONG", "FLOAT", "DOUBLE", "BOOL", "STRING", "POINTER", "ARRAY", "MAP", "FUNCTION", "NATIVE_FUNCTION", "CLASS_DEF", "INSTANCE" };
    std::string actualType = (finalFunc.type >= 0 && finalFunc.type <= 19) ? typeNames[finalFunc.type] : "UNKNOWN";

    throw std::runtime_error("尝试调用一个非函数类型对象 (实际类型: " + actualType + ")");
}
