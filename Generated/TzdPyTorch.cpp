// Prevent Windows min/max macros from breaking libtorch headers
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "TzdPyTorch.h"
#include "TzdInterpreter.h"
#include "TzdOop.h"

#include <ATen/ATen.h>
#include <ATen/Parallel.h>
#include <ATen/ops/fft_fft.h>
#include <ATen/ops/fft_ifft.h>
#include <ATen/ops/real.h>
#include <c10/core/GradMode.h>
#ifdef WITH_CUDA
#include <ATen/cuda/CUDAContext.h>
#include <c10/cuda/CUDAFunctions.h>
#include <c10/cuda/CUDACachingAllocator.h>
#include <c10/cuda/CUDAStream.h>
#include <cuda_runtime.h>
#include <cufft.h>
#include <nvrtc.h>
#include <cuda.h>
#endif

// types.h sets up namespace torch { using namespace at; } which makes
// at::Tensor (via at::Tensor), torch::relu, torch::softmax, etc. available.
// MUST be included before torch/script.h to avoid using-declaration conflicts.
#include <torch/csrc/api/include/torch/types.h>
#include <torch/script.h>

// Windows API for LoadLibrary — included AFTER all other headers to avoid
// macro conflicts with ANTLR4 and libtorch headers
#ifdef _WIN32
#include <windows.h>
#endif

// CUDA helpers (conditional on WITH_CUDA)
#ifdef WITH_CUDA
#include <c10/cuda/CUDACachingAllocator.h>
#endif

namespace torch {
    namespace cuda {
        inline bool is_available() {
#ifdef WITH_CUDA
            return at::cuda::is_available();
#else
            return false;
#endif
        }
        inline void empty_cache() {
#ifdef WITH_CUDA
            c10::cuda::CUDACachingAllocator::emptyCache();
#endif
        }
        inline int64_t device_count() {
#ifdef WITH_CUDA
            return c10::cuda::device_count();
#else
            return 0;
#endif
        }
        inline int64_t current_device() {
#ifdef WITH_CUDA
            return c10::cuda::current_device();
#else
            return -1;
#endif
        }
        inline void set_device(int64_t d) {
#ifdef WITH_CUDA
            c10::cuda::set_device(d);
#endif
        }
        inline size_t memory_allocated() {
#ifdef WITH_CUDA
            return 0; // TODO: use c10::cuda::CUDACachingAllocator::getDeviceStats
#else
            return 0;
#endif
        }
        inline size_t memory_reserved() {
#ifdef WITH_CUDA
            return 0;
#else
            return 0;
#endif
        }
        inline size_t max_memory_allocated() {
#ifdef WITH_CUDA
            return 0;
#else
            return 0;
#endif
        }
        inline void reset_peak_memory_stats() {
#ifdef WITH_CUDA
#endif
        }
        inline void synchronize() {
#ifdef WITH_CUDA
            c10::cuda::device_synchronize();
#endif
        }
    }
    // manual_seed
    inline void manual_seed(uint64_t seed) { at::manual_seed(seed); }
    // isGradEnabled / setGradEnabled
    inline bool isGradEnabled() { return c10::GradMode::is_enabled(); }
    inline void setGradEnabled(bool enabled) { c10::GradMode::set_enabled(enabled); }
    // NoGradGuard
    class NoGradGuard {
    public:
        NoGradGuard() { saved_ = c10::GradMode::is_enabled(); c10::GradMode::set_enabled(false); }
        ~NoGradGuard() { c10::GradMode::set_enabled(saved_); }
    private:
        bool saved_;
    };
    // softmin helper
    inline at::Tensor softmin(const at::Tensor& input, int64_t dim) {
        return at::softmax(-input, dim);
    }
    // interpolate helper (maps to upsample)
    inline at::Tensor interpolate(const at::Tensor& input, at::IntArrayRef size, const std::string& mode) {
        if (mode == "nearest" || mode.empty()) {
            return at::upsample_nearest2d(input, {size[0], size[1]});
        }
        return at::upsample_bilinear2d(input, {size[0], size[1]}, false);
    }
    // broadcast_shapes helper
    inline std::vector<int64_t> broadcast_shapes(const std::vector<std::vector<int64_t>>& shapes) {
        if (shapes.empty()) return {};
        std::vector<int64_t> result = shapes[0];
        for (size_t i = 1; i < shapes.size(); i++) {
            const auto& s = shapes[i];
            size_t maxDim = (std::max)(result.size(), s.size());
            std::vector<int64_t> tmp(maxDim, 1);
            for (size_t j = 0; j < result.size(); j++) tmp[maxDim - result.size() + j] = result[j];
            for (size_t j = 0; j < s.size(); j++) {
                int64_t idx = maxDim - s.size() + j;
                if (tmp[idx] == 1) tmp[idx] = s[j];
                else if (s[j] != 1 && s[j] != tmp[idx]) {
                    // broadcast error - just use the larger
                }
            }
            result = tmp;
        }
        return result;
    }
}

// ============================================================================
// Manual optimizer implementations (replacing torch::optim C++ API)
// ============================================================================
struct TzdOptimizer {
    enum Type { SGD, ADAM, ADAMW, RMSPROP, ADAGRAD, ADAMAX } type;
    double lr;
    double beta1, beta2;
    double weight_decay;
    double momentum;
    double alpha; // RMSprop
    double eps;
    std::vector<at::Tensor> params;
    // Adam state
    std::vector<at::Tensor> m_state; // first moment
    std::vector<at::Tensor> v_state; // second moment
    int step_count = 0;
    // SGD state
    std::vector<at::Tensor> momentum_buffers;

    void step() {
        step_count++;
        at::NoGradGuard no_grad; // Disable gradient tracking during optimizer step
        for (size_t i = 0; i < params.size(); i++) {
            auto& param = params[i];
            if (!param.requires_grad() || !param.grad().defined()) continue;
            auto grad = param.grad();
            if (weight_decay != 0.0) grad = grad + weight_decay * param;
            switch (type) {
            case SGD: {
                if (momentum != 0.0) {
                    if ((int)momentum_buffers.size() <= (int)i) momentum_buffers.resize(i + 1);
                    if (!momentum_buffers[i].defined()) momentum_buffers[i] = at::zeros_like(grad);
                    momentum_buffers[i] = momentum * momentum_buffers[i] + grad;
                    param.add_(momentum_buffers[i], -lr * 1.0);
                } else {
                    param.add_(grad, -lr);
                }
                break;
            }
            case ADAM:
            case ADAMW:
            case ADAMAX: {
                if ((int)m_state.size() <= (int)i) {
                    m_state.resize(i + 1);
                    v_state.resize(i + 1);
                    m_state[i] = at::zeros_like(grad);
                    v_state[i] = at::zeros_like(grad);
                }
                m_state[i] = beta1 * m_state[i] + (1.0 - beta1) * grad;
                v_state[i] = beta2 * v_state[i] + (1.0 - beta2) * grad * grad;
                double m_hat = 0.0, v_hat = 0.0;
                double bc1 = 1.0 - std::pow(beta1, step_count);
                double bc2 = 1.0 - std::pow(beta2, step_count);
                if (type == ADAMAX) {
                    v_state[i] = at::max(v_state[i], (1.0 - beta2) * grad.abs());
                    param.add_(m_state[i] / (v_state[i] + eps), -lr / bc1);
                } else {
                    at::Tensor m_hat_t = m_state[i] / bc1;
                    at::Tensor v_hat_t = v_state[i] / bc2;
                    if (type == ADAMW) {
                        param.add_(grad * weight_decay, -lr);
                        param.add_(m_hat_t / (at::sqrt(v_hat_t) + eps), -lr);
                    } else {
                        param.add_(m_hat_t / (at::sqrt(v_hat_t) + eps), -lr);
                    }
                }
                break;
            }
            case RMSPROP: {
                if ((int)v_state.size() <= (int)i) {
                    v_state.resize(i + 1);
                    v_state[i] = at::zeros_like(grad);
                }
                v_state[i] = alpha * v_state[i] + (1.0 - alpha) * grad * grad;
                param.add_(grad / (at::sqrt(v_state[i]) + eps), -lr);
                break;
            }
            case ADAGRAD: {
                if ((int)v_state.size() <= (int)i) {
                    v_state.resize(i + 1);
                    v_state[i] = at::zeros_like(grad);
                }
                v_state[i] = v_state[i] + grad * grad;
                param.add_(grad / (at::sqrt(v_state[i]) + eps), -lr);
                break;
            }
            }
        }
    }
    void zero_grad() {
        for (auto& p : params) {
            if (p.grad().defined()) p.grad().zero_();
        }
    }
};


#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <memory>

// ============================================================================
// Static members — zero-lock tensor lifecycle via c10::intrusive_ptr
// ============================================================================
// DELETED: s_tensorRegistry (global hash table) and s_registryMutex (global GIL)
// The tensor lifecycle now directly uses c10::intrusive_ptr's atomic refcount,
// which is built into at::TensorImpl. No global lock contention.
bool TzdPyTorch::s_initialized = false;
std::atomic<size_t> TzdPyTorch::s_liveTensorCount{0};
std::atomic<size_t> TzdPyTorch::s_liveTensorBytes{0};

// Local helper for numeric coercion from TzdValue (valToDouble is defined in TzdNativeModule.cpp)
static double valToDouble(const TzdValue& v) {
    return TzdInterpreter::getAsDoubleInternal(v);
}

// ============================================================================
// Tensor lifecycle management — zero-lock via c10::intrusive_ptr refcount
// The old global s_tensorRegistry + s_registryMutex has been eliminated.
// Tensor refcounting now directly uses at::TensorImpl's built-in atomic
// intrusive_ptr refcount, achieving zero-lock contention.
// ============================================================================
void TzdPyTorch::registerTensor(at::Tensor* t) {
    // No-op: the at::Tensor already has refcount=1 from `new at::Tensor(...)`.
    // We just track diagnostics.
    if (t && t->defined()) {
        s_liveTensorCount.fetch_add(1, std::memory_order_relaxed);
        s_liveTensorBytes.fetch_add(t->nbytes(), std::memory_order_relaxed);
    }
}

void TzdPyTorch::retainTensor(at::Tensor* t) {
    if (!t) return;
    // Directly increment the TensorImpl's intrusive_ptr refcount — NO LOCK NEEDED.
    // c10::raw::intrusive_ptr::incref is an atomic fetch_add(1, relaxed).
    c10::raw::intrusive_ptr::incref(t->unsafeGetTensorImpl());
}

void TzdPyTorch::releaseTensor(at::Tensor* t) {
    if (!t) return;
    // Lock-free release: check the current refcount, then either delete
    // the at::Tensor (which frees the TensorImpl) or just decref.
    size_t count = t->use_count();
    if (count <= 1) {
        // Only the at::Tensor's own impl_ holds a reference → safe to delete.
        // The at::Tensor destructor decrefs the TensorImpl to 0 and frees it.
        if (t->defined()) {
            s_liveTensorCount.fetch_sub(1, std::memory_order_relaxed);
            s_liveTensorBytes.fetch_sub(t->nbytes(), std::memory_order_relaxed);
        }
        delete t;
    } else {
        // Multiple references (at::Tensor + TzdValue copies).
        // Decref by 1 without freeing: create a reclaim temporary that
        // decrefs on destruction. The at::Tensor and TensorImpl stay alive.
        c10::intrusive_ptr<c10::TensorImpl> tmp(
            c10::intrusive_ptr<c10::TensorImpl>::reclaim(
                t->unsafeGetTensorImpl()));
        // tmp destructor: atomic decref. If refcount > 0 (which it will be,
        // since at::Tensor's impl_ still holds a reference), TensorImpl survives.
    }
}

void TzdPyTorch::releaseAll() {
    // No-op: tensors are now managed by c10::intrusive_ptr refcounting.
    // When the process exits, the OS reclaims all memory.
    // This function is kept for API compatibility.
}

size_t TzdPyTorch::totalMemoryBytes() {
    return s_liveTensorBytes.load(std::memory_order_relaxed);
}

bool TzdPyTorch::isCudaAvailable() {
#ifdef WITH_CUDA
    return torch::cuda::is_available();
#else
    return false;
#endif
}

at::Tensor* TzdPyTorch::getTensor(const TzdValue& val) {
    if (val.type == TzdValue::TENSOR || val.type == TzdValue::POINTER) {
        return reinterpret_cast<at::Tensor*>(val.ptrVal);
    }
    return nullptr;
}

TzdValue TzdPyTorch::wrapTensor(at::Tensor* t) {
    registerTensor(t);
    TzdValue v;
    v.type = TzdValue::TENSOR;  // Use TENSOR type for automatic lifecycle management
    v.ptrVal = t;
    return v;
}

// ============================================================================
// C-linkage retain/release hooks for TzdValue copy control
// These are called by TzdValue's copy constructor, assignment, and destructor
// to guarantee zero VRAM leaks through automatic reference counting.
// ============================================================================
extern "C" void tzdTensorRetain(void* ptr) {
    if (!ptr) return;
    TzdPyTorch::retainTensor(reinterpret_cast<at::Tensor*>(ptr));
}

extern "C" void tzdTensorRelease(void* ptr) {
    if (!ptr) return;
    TzdPyTorch::releaseTensor(reinterpret_cast<at::Tensor*>(ptr));
}

// ============================================================================
// High-Performance GPU-Accelerated Big Number Multiplication Architecture
// Stockham 4-Step NTT (Radix-10^9) with On-Chip Shared Memory Fusion,
// Montgomery 32-Bit Twiddle Factor Pipeline, GPU CRT, and 3-Phase Block-Scan
// Parallel Prefix Carry Elimination.
// ============================================================================
#ifdef WITH_CUDA
#include <omp.h>

// ---- CUDA kernel source (compiled at runtime by NVRTC) ----
static const char* s_nttKernelSrc = R"CUDA(
typedef unsigned int u32;
typedef unsigned long long u64;

__device__ __forceinline__ u32 mont_mul(u32 a, u32 b, u32 P, u32 P_inv) {
    u64 prod = (u64)a * b;
    u32 m = (u32)prod * P_inv;
    u64 t = prod + (u64)m * P;
    u32 res = (u32)(t >> 32);
    if (res >= P) res -= P;
    return res;
}

__device__ __forceinline__ u32 barrett_mul(u32 a, u32 b, u32 P, u64 M) {
    u64 prod = (u64)a * b;
    u64 q = __umul64hi(prod, M);
    u32 r = (u32)(prod - q * P);
    if (r >= P) r -= P;
    return r;
}

extern "C" __global__
void stockham_row_ntt(u32* dst, const u32* src, const u32* twiddles,
                      int R, int log2R, int num_rows,
                      int is_inv, u32 scale,
                      u32 P, u32 P_inv) {
    int row = blockIdx.x;
    if (row >= num_rows) return;

    extern __shared__ u32 s_mem[];
    u32* s_src = s_mem;
    u32* s_dst = s_mem + R;
    u32* s_tw = s_mem + 2 * R;

    int tid = threadIdx.x;
    int halfR = R >> 1;

    // Load twiddles into fast on-chip shared memory
    s_tw[tid] = __ldg(&twiddles[tid]);
    s_tw[tid + halfR] = __ldg(&twiddles[tid + halfR]);

    const u32* row_in = src + (size_t)row * R;
    u32 in1 = __ldg(&row_in[tid]);
    if (in1 >= P) in1 -= P;
    u32 in2 = __ldg(&row_in[tid + halfR]);
    if (in2 >= P) in2 -= P;

    s_src[tid] = in1;
    s_src[tid + halfR] = in2;
    __syncthreads();

    for (int s = 0; s < log2R; s++) {
        int L = 1 << s;
        int group = tid / L;
        int j = tid - group * L;
        u32 u = s_src[tid];
        u32 v = s_src[tid + halfR];

        int tw_idx = j * (R / (2 * L));
        if (is_inv && tw_idx > 0) tw_idx = R - tw_idx;
        u32 tw = s_tw[tw_idx];
        u32 v_tw = mont_mul(v, tw, P, P_inv);

        u32 r1 = u + v_tw;
        if (r1 >= P) r1 -= P;
        u32 r2 = (u >= v_tw) ? (u - v_tw) : (u + P - v_tw);

        s_dst[group * (2 * L) + j] = r1;
        s_dst[group * (2 * L) + j + L] = r2;

        __syncthreads();
        u32* tmp = s_src; s_src = s_dst; s_dst = tmp;
    }

    u32* row_out = dst + (size_t)row * R;
    u32 out1 = s_src[tid];
    u32 out2 = s_src[tid + halfR];
    if (scale != 0) {
        out1 = mont_mul(out1, scale, P, P_inv);
        out2 = mont_mul(out2, scale, P, P_inv);
    }
    row_out[tid] = out1;
    row_out[tid + halfR] = out2;
}

extern "C" __global__
void transpose_twiddle(u32* dst, const u32* src,
                       int Rows, int Cols, int N,
                       const u32* twiddles,
                       int is_inv, u32 scale,
                       u32 P, u32 P_inv) {
    __shared__ u32 tile[32][33];

    int x = blockIdx.x * 32 + threadIdx.x;
    int y = blockIdx.y * 32 + threadIdx.y;

    if (x < Cols && y < Rows) {
        u32 val = __ldg(&src[(size_t)y * Cols + x]);
        if (twiddles) {
            u32 k = (u32)(((size_t)y * x) & (N - 1));
            if (is_inv && k > 0) k = N - k;
            u32 tw = __ldg(&twiddles[k]);
            val = mont_mul(val, tw, P, P_inv);
        }
        if (scale != 0) {
            val = mont_mul(val, scale, P, P_inv);
        }
        tile[threadIdx.y][threadIdx.x] = val;
    }
    __syncthreads();

    int tx = blockIdx.y * 32 + threadIdx.x;
    int ty = blockIdx.x * 32 + threadIdx.y;

    if (tx < Rows && ty < Cols) {
        dst[(size_t)ty * Rows + tx] = tile[threadIdx.x][threadIdx.y];
    }
}

extern "C" __global__
void pointwise_mul(u32* a, const u32* b, int n, u32 P, u64 M) {
    for (int idx = blockIdx.x * blockDim.x + threadIdx.x;
         idx < n;
         idx += gridDim.x * blockDim.x) {
        a[idx] = barrett_mul(a[idx], b[idx], P, M);
    }
}

extern "C" __global__
void init_twiddles(u32* twiddles, int n, u32 w_mont, u32 P, u32 P_inv) {
    for (int idx = blockIdx.x * blockDim.x + threadIdx.x;
         idx < n;
         idx += gridDim.x * blockDim.x) {
        u32 res = (u32)(((u64)1 << 32) % P);
        u32 base = w_mont;
        int exp = idx;
        while (exp > 0) {
            if (exp & 1) res = mont_mul(res, base, P, P_inv);
            base = mont_mul(base, base, P, P_inv);
            exp >>= 1;
        }
        twiddles[idx] = res;
    }
}

extern "C" __global__
void crt_kernel(u32* d_rem, u64* d_q,
                const u32* r1, const u32* r2, const u32* r3,
                int n,
                u32 P1, u32 P2, u32 P3,
                u32 inv12, u32 inv123,
                u64 q_P1P2, u64 rem_P1P2) {
    for (int i = blockIdx.x * blockDim.x + threadIdx.x;
         i < n;
         i += gridDim.x * blockDim.x) {
        u32 v1 = r1[i];
        u32 v2 = r2[i];
        u32 v3 = r3[i];

        u32 t1 = (v2 >= v1 % P2) ? (v2 - v1 % P2) : (v2 + P2 - v1 % P2);
        t1 = (u32)(((u64)t1 * inv12) % P2);
        u64 x12 = (u64)v1 + (u64)P1 * t1;

        u32 x12_mod_P3 = (u32)(x12 % P3);
        u32 t2 = (v3 >= x12_mod_P3) ? (v3 - x12_mod_P3) : (v3 + P3 - x12_mod_P3);
        t2 = (u32)(((u64)t2 * inv123) % P3);

        u64 low = x12 + rem_P1P2 * t2;
        d_rem[i] = (u32)(low % 1000000000ULL);
        d_q[i] = q_P1P2 * t2 + (low / 1000000000ULL);
    }
}

extern "C" __global__
void carry_round1(u32* d_rem1, u64* d_c1, const u32* d_rem, const u64* d_q, int n) {
    for (int i = blockIdx.x * blockDim.x + threadIdx.x;
         i < n;
         i += gridDim.x * blockDim.x) {
        u64 v = (u64)d_rem[i] + (i > 0 ? d_q[i - 1] : 0ULL);
        d_rem1[i] = (u32)(v % 1000000000ULL);
        d_c1[i] = v / 1000000000ULL;
    }
}

extern "C" __global__
void carry_round2(u32* d_rem2, u32* d_c2, const u32* d_rem1, const u64* d_c1, int n) {
    for (int i = blockIdx.x * blockDim.x + threadIdx.x;
         i < n;
         i += gridDim.x * blockDim.x) {
        u64 v = (u64)d_rem1[i] + (i > 0 ? d_c1[i - 1] : 0ULL);
        d_rem2[i] = (u32)(v % 1000000000ULL);
        d_c2[i] = (u32)(v / 1000000000ULL);
    }
}

extern "C" __global__
void carry_scan_phase1(u32* d_local_g, u32* d_local_p,
                       u32* d_block_g, u32* d_block_p,
                       const u32* d_rem2, const u32* d_c2,
                       int n) {
    int b = blockIdx.x;
    int tid = threadIdx.x;
    int i = b * 512 + tid;

    u32 g = 0, p = 0;
    if (i < n) {
        g = (i == 0) ? 0 : d_c2[i];
        p = (i == 0) ? 0 : (d_rem2[i - 1] == 999999999U ? 1 : 0);
    }

    int lane = tid & 31;
    int warp_id = tid >> 5;

    #pragma unroll
    for (int offset = 1; offset < 32; offset <<= 1) {
        u32 g_left = __shfl_up_sync(0xffffffff, g, offset);
        u32 p_left = __shfl_up_sync(0xffffffff, p, offset);
        if (lane >= offset) {
            g = g | (p & g_left);
            p = p & p_left;
        }
    }

    __shared__ u32 s_warp_g[16];
    __shared__ u32 s_warp_p[16];
    if (lane == 31) {
        s_warp_g[warp_id] = g;
        s_warp_p[warp_id] = p;
    }
    __syncthreads();

    if (warp_id == 0 && lane < 16) {
        u32 wg = s_warp_g[lane];
        u32 wp = s_warp_p[lane];
        #pragma unroll
        for (int offset = 1; offset < 16; offset <<= 1) {
            u32 wg_left = __shfl_up_sync(0x0000ffff, wg, offset);
            u32 wp_left = __shfl_up_sync(0x0000ffff, wp, offset);
            if (lane >= offset) {
                wg = wg | (wp & wg_left);
                wp = wp & wp_left;
            }
        }
        s_warp_g[lane] = wg;
        s_warp_p[lane] = wp;
    }
    __syncthreads();

    if (warp_id > 0) {
        u32 prev_g = s_warp_g[warp_id - 1];
        u32 prev_p = s_warp_p[warp_id - 1];
        g = g | (p & prev_g);
        p = p & prev_p;
    }

    if (i < n) {
        d_local_g[i] = g;
        d_local_p[i] = p;
    }
    if (tid == 511) {
        d_block_g[b] = g;
        d_block_p[b] = p;
    }
}

extern "C" __global__
void carry_scan_phase2(u32* d_block_carry, const u32* d_block_g, const u32* d_block_p, int num_blocks) {
    if (threadIdx.x == 0) {
        u32 running_g = 0;
        u32 running_p = 1;
        d_block_carry[0] = 0;
        for (int b = 0; b < num_blocks - 1; b++) {
            u32 bg = d_block_g[b];
            u32 bp = d_block_p[b];
            running_g = bg | (bp & running_g);
            running_p = bp & running_p;
            d_block_carry[b + 1] = running_g;
        }
    }
}

extern "C" __global__
void carry_scan_phase3(u32* d_result,
                       const u32* d_rem2,
                       const u32* d_local_g, const u32* d_local_p,
                       const u32* d_block_carry,
                       int n) {
    int b = blockIdx.x;
    int tid = threadIdx.x;
    int i = b * 512 + tid;
    if (i >= n) return;

    u32 cin = d_block_carry[b];
    u32 C_i = d_local_g[i] | (d_local_p[i] & cin);
    u32 val = d_rem2[i] + C_i;
    if (val >= 1000000000U) val -= 1000000000U;
    d_result[i] = val;
}
)CUDA";

struct NvrtcNttCtx {
    CUmodule mod = nullptr;
    CUfunction fn_stockham_row_ntt = nullptr;
    CUfunction fn_transpose_twiddle = nullptr;
    CUfunction fn_pointwise_mul = nullptr;
    CUfunction fn_init_twiddles = nullptr;
    CUfunction fn_crt = nullptr;
    CUfunction fn_carry_round1 = nullptr;
    CUfunction fn_carry_round2 = nullptr;
    CUfunction fn_carry_scan_phase1 = nullptr;
    CUfunction fn_carry_scan_phase2 = nullptr;
    CUfunction fn_carry_scan_phase3 = nullptr;
    bool tried = false, ok = false;
};
static NvrtcNttCtx g_nvrtcNtt;

static uint32_t npow(uint64_t b, uint64_t e, uint32_t mod) {
    uint64_t r = 1;
    b %= mod;
    while (e > 0) {
        if (e & 1) r = (r * b) % mod;
        b = (b * b) % mod;
        e >>= 1;
    }
    return (uint32_t)r;
}

static bool init_nvrtc_ntt() {
    if (g_nvrtcNtt.tried) return g_nvrtcNtt.ok;
    g_nvrtcNtt.tried = true;

    CUresult cures = cuInit(0);
    if (cures != CUDA_SUCCESS) {
        fprintf(stderr, "[NVRTC Init] cuInit failed: %d\n", cures);
        return false;
    }

    int dev = 0;
    cudaError_t cerr = cudaGetDevice(&dev);
    if (cerr != cudaSuccess) {
        fprintf(stderr, "[NVRTC Init] cudaGetDevice failed: %d\n", (int)cerr);
        return false;
    }

    CUcontext ctx;
    cures = cuDevicePrimaryCtxRetain(&ctx, dev);
    if (cures != CUDA_SUCCESS) {
        fprintf(stderr, "[NVRTC Init] cuDevicePrimaryCtxRetain failed: %d\n", cures);
    } else {
        cuCtxSetCurrent(ctx);
    }

    int maj = 0, minn = 0;
    cudaDeviceGetAttribute(&maj, cudaDevAttrComputeCapabilityMajor, dev);
    cudaDeviceGetAttribute(&minn, cudaDevAttrComputeCapabilityMinor, dev);
    char arch[32];
    snprintf(arch, sizeof(arch), "--gpu-architecture=compute_%d%d", maj, minn);

    nvrtcProgram prog;
    nvrtcResult nres = nvrtcCreateProgram(&prog, s_nttKernelSrc, "ntt_stockham.cu", 0, nullptr, nullptr);
    if (nres != NVRTC_SUCCESS) {
        fprintf(stderr, "[NVRTC Init] nvrtcCreateProgram failed: %d\n", (int)nres);
        return false;
    }
    const char* opts[] = { arch };
    nres = nvrtcCompileProgram(prog, 1, opts);
    if (nres != NVRTC_SUCCESS) {
        size_t ls = 0; nvrtcGetProgramLogSize(prog, &ls);
        if (ls > 0) {
            std::vector<char> log(ls + 1, 0);
            nvrtcGetProgramLog(prog, log.data());
            fprintf(stderr, "[NVRTC Compile Error]\n%s\n", log.data());
        } else {
            fprintf(stderr, "[NVRTC Compile Error] code %d\n", (int)nres);
        }
        nvrtcDestroyProgram(&prog);
        return false;
    }
    size_t psz = 0; nvrtcGetPTXSize(prog, &psz);
    std::vector<char> ptx(psz);
    nvrtcGetPTX(prog, ptx.data());
    nvrtcDestroyProgram(&prog);

    cures = cuModuleLoadData(&g_nvrtcNtt.mod, ptx.data());
    if (cures != CUDA_SUCCESS) {
        fprintf(stderr, "[NVRTC Init] cuModuleLoadData failed: %d\n", cures);
        return false;
    }
    cuModuleGetFunction(&g_nvrtcNtt.fn_stockham_row_ntt, g_nvrtcNtt.mod, "stockham_row_ntt");
    cuModuleGetFunction(&g_nvrtcNtt.fn_transpose_twiddle, g_nvrtcNtt.mod, "transpose_twiddle");
    cuModuleGetFunction(&g_nvrtcNtt.fn_pointwise_mul, g_nvrtcNtt.mod, "pointwise_mul");
    cuModuleGetFunction(&g_nvrtcNtt.fn_init_twiddles, g_nvrtcNtt.mod, "init_twiddles");
    cuModuleGetFunction(&g_nvrtcNtt.fn_crt, g_nvrtcNtt.mod, "crt_kernel");
    cuModuleGetFunction(&g_nvrtcNtt.fn_carry_round1, g_nvrtcNtt.mod, "carry_round1");
    cuModuleGetFunction(&g_nvrtcNtt.fn_carry_round2, g_nvrtcNtt.mod, "carry_round2");
    cuModuleGetFunction(&g_nvrtcNtt.fn_carry_scan_phase1, g_nvrtcNtt.mod, "carry_scan_phase1");
    cuModuleGetFunction(&g_nvrtcNtt.fn_carry_scan_phase2, g_nvrtcNtt.mod, "carry_scan_phase2");
    cuModuleGetFunction(&g_nvrtcNtt.fn_carry_scan_phase3, g_nvrtcNtt.mod, "carry_scan_phase3");

    g_nvrtcNtt.ok = g_nvrtcNtt.fn_stockham_row_ntt &&
                    g_nvrtcNtt.fn_transpose_twiddle &&
                    g_nvrtcNtt.fn_pointwise_mul &&
                    g_nvrtcNtt.fn_init_twiddles &&
                    g_nvrtcNtt.fn_crt &&
                    g_nvrtcNtt.fn_carry_round1 &&
                    g_nvrtcNtt.fn_carry_round2 &&
                    g_nvrtcNtt.fn_carry_scan_phase1 &&
                    g_nvrtcNtt.fn_carry_scan_phase2 &&
                    g_nvrtcNtt.fn_carry_scan_phase3;
    if (!g_nvrtcNtt.ok) {
        fprintf(stderr, "[NVRTC Init] cuModuleGetFunction failed for one or more functions\n");
    }
    return g_nvrtcNtt.ok;
}

// 3-Prime constants and Barrett constants
static const uint32_t P[3] = { 998244353U, 985661441U, 754974721U };
static const uint32_t GEN[3] = { 3U, 3U, 11U };
static const uint32_t P_INV[3] = { 998244351U, 985661439U, 754974719U };
static const uint64_t M_BARRETT[3] = { 18479187002ULL, 18715091517ULL, 24433591695ULL };

static const uint32_t inv12 = 657107549U;
static const uint32_t inv123 = 284003040U;
static const uint64_t q_P1P2 = 983930967ULL;
static const uint64_t rem_P1P2 = 448092673ULL;

// Persistent device and pinned memory buffers for zero-allocation hot execution
static uint32_t* d_a[3] = {nullptr, nullptr, nullptr};
static uint32_t* d_b[3] = {nullptr, nullptr, nullptr};
static uint32_t* d_scratch[3] = {nullptr, nullptr, nullptr};
static uint32_t* d_tw_N[3] = {nullptr, nullptr, nullptr};
static uint32_t* d_tw_N1[3] = {nullptr, nullptr, nullptr};
static uint32_t* d_tw_N2[3] = {nullptr, nullptr, nullptr};

static uint32_t* d_rem = nullptr;
static uint64_t* d_q = nullptr;
static uint32_t* d_rem1 = nullptr;
static uint64_t* d_c1 = nullptr;
static uint32_t* d_rem2 = nullptr;
static uint32_t* d_c2 = nullptr;
static uint32_t* d_local_g = nullptr;
static uint32_t* d_local_p = nullptr;
static uint32_t* d_block_g = nullptr;
static uint32_t* d_block_p = nullptr;
static uint32_t* d_block_carry = nullptr;
static uint32_t* d_result = nullptr;

static int s_buf_cap = 0;
static int s_cached_tw_n = 0;

static uint32_t* s_pinned_a = nullptr;
static uint32_t* s_pinned_b = nullptr;
static uint32_t* s_pinned_res = nullptr;
static size_t s_pinned_cap = 0;

static CUstream s_streams[3] = {0, 0, 0};

static void run_ntt_stream(int pi, uint32_t* d_in_out, uint32_t* d_sc,
                           int n, int N1, int N2, int k1, int k2,
                           bool inv, uint32_t scale, CUstream st) {
    uint32_t p = P[pi];
    uint32_t p_inv = P_INV[pi];
    if (n <= 2048) {
        int log2N = k1 + k2;
        int num_rows = 1;
        int inv_flag = inv ? 1 : 0;
        void* args[] = { &d_in_out, &d_in_out, &d_tw_N[pi],
                         &n, &log2N, &num_rows,
                         &inv_flag, &scale, &p, &p_inv };
        cuLaunchKernel(g_nvrtcNtt.fn_stockham_row_ntt,
                       1, 1, 1,
                       n / 2, 1, 1,
                       3 * n * sizeof(uint32_t),
                       st, args, nullptr);
    } else {
        if (!inv) {
            // Forward NTT
            void* tw_null = nullptr;
            int inv0 = 0;
            uint32_t scale0 = 0;
            void* args1[] = { &d_sc, &d_in_out, &N1, &N2, &n, &tw_null, &inv0, &scale0, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_transpose_twiddle,
                           (N2 + 31) / 32, (N1 + 31) / 32, 1,
                           32, 32, 1, 0, st, args1, nullptr);

            void* args2[] = { &d_sc, &d_sc, &d_tw_N1[pi],
                             &N1, &k1, &N2, &inv0, &scale0, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_stockham_row_ntt,
                           N2, 1, 1,
                           N1 / 2, 1, 1,
                           3 * N1 * sizeof(uint32_t),
                           st, args2, nullptr);

            void* args3[] = { &d_in_out, &d_sc, &N2, &N1, &n, &d_tw_N[pi], &inv0, &scale0, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_transpose_twiddle,
                           (N1 + 31) / 32, (N2 + 31) / 32, 1,
                           32, 32, 1, 0, st, args3, nullptr);

            void* args4[] = { &d_in_out, &d_in_out, &d_tw_N2[pi],
                             &N2, &k2, &N1, &inv0, &scale0, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_stockham_row_ntt,
                           N1, 1, 1,
                           N2 / 2, 1, 1,
                           3 * N2 * sizeof(uint32_t),
                           st, args4, nullptr);
        } else {
            // Inverse NTT
            int inv1 = 1;
            uint32_t scale0 = 0;
            void* args1[] = { &d_in_out, &d_in_out, &d_tw_N2[pi],
                             &N2, &k2, &N1, &inv1, &scale0, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_stockham_row_ntt,
                           N1, 1, 1,
                           N2 / 2, 1, 1,
                           3 * N2 * sizeof(uint32_t),
                           st, args1, nullptr);

            void* args2[] = { &d_sc, &d_in_out, &N1, &N2, &n, &d_tw_N[pi], &inv1, &scale0, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_transpose_twiddle,
                           (N2 + 31) / 32, (N1 + 31) / 32, 1,
                           32, 32, 1, 0, st, args2, nullptr);

            void* args3[] = { &d_sc, &d_sc, &d_tw_N1[pi],
                             &N1, &k1, &N2, &inv1, &scale0, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_stockham_row_ntt,
                           N2, 1, 1,
                           N1 / 2, 1, 1,
                           3 * N1 * sizeof(uint32_t),
                           st, args3, nullptr);

            void* tw_null = nullptr;
            int inv0 = 0;
            void* args4[] = { &d_in_out, &d_sc, &N2, &N1, &n, &tw_null, &inv0, &scale, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_transpose_twiddle,
                           (N1 + 31) / 32, (N2 + 31) / 32, 1,
                           32, 32, 1, 0, st, args4, nullptr);
        }
    }
}

std::vector<uint64_t> bigint_mul_gpu_ntt_limbs(const std::vector<uint64_t>& la, const std::vector<uint64_t>& lb) {
    if (la.empty() || lb.empty()) return {};
    if (!init_nvrtc_ntt()) throw std::runtime_error("NVRTC init failed");

    size_t tot = la.size() + lb.size();
    int n = 1;
    while ((size_t)n < tot) n <<= 1;
    if (n < 512) n = 512;
    if (n > (1 << 22)) throw std::runtime_error("NTT size exceeds prime limit 2^22");

    int k = 0, m = n;
    while (m > 1) { m >>= 1; k++; }
    int k1 = k / 2;
    int k2 = k - k1;
    int N1 = 1 << k1;
    int N2 = 1 << k2;

    if (!s_streams[0]) {
        for (int i = 0; i < 3; i++) {
            cudaStream_t st;
            cudaStreamCreate(&st);
            s_streams[i] = (CUstream)st;
        }
    }
    auto t_start = std::chrono::steady_clock::now();

    if (s_buf_cap < n) {
        for (int pi = 0; pi < 3; pi++) {
            if (d_a[pi]) cudaFree(d_a[pi]);
            if (d_b[pi]) cudaFree(d_b[pi]);
            if (d_scratch[pi]) cudaFree(d_scratch[pi]);
            if (d_tw_N[pi]) cudaFree(d_tw_N[pi]);
            if (d_tw_N1[pi]) cudaFree(d_tw_N1[pi]);
            if (d_tw_N2[pi]) cudaFree(d_tw_N2[pi]);

            cudaMalloc(&d_a[pi], n * sizeof(uint32_t));
            cudaMalloc(&d_b[pi], n * sizeof(uint32_t));
            cudaMalloc(&d_scratch[pi], n * sizeof(uint32_t));
            cudaMalloc(&d_tw_N[pi], n * sizeof(uint32_t));
            cudaMalloc(&d_tw_N1[pi], 2048 * sizeof(uint32_t));
            cudaMalloc(&d_tw_N2[pi], 2048 * sizeof(uint32_t));
        }
        if (d_rem) cudaFree(d_rem);
        if (d_q) cudaFree(d_q);
        if (d_rem1) cudaFree(d_rem1);
        if (d_c1) cudaFree(d_c1);
        if (d_rem2) cudaFree(d_rem2);
        if (d_c2) cudaFree(d_c2);
        if (d_local_g) cudaFree(d_local_g);
        if (d_local_p) cudaFree(d_local_p);
        if (d_block_g) cudaFree(d_block_g);
        if (d_block_p) cudaFree(d_block_p);
        if (d_block_carry) cudaFree(d_block_carry);
        if (d_result) cudaFree(d_result);

        cudaMalloc(&d_rem, n * sizeof(uint32_t));
        cudaMalloc(&d_q, n * sizeof(uint64_t));
        cudaMalloc(&d_rem1, n * sizeof(uint32_t));
        cudaMalloc(&d_c1, n * sizeof(uint64_t));
        cudaMalloc(&d_rem2, n * sizeof(uint32_t));
        cudaMalloc(&d_c2, n * sizeof(uint32_t));
        cudaMalloc(&d_local_g, n * sizeof(uint32_t));
        cudaMalloc(&d_local_p, n * sizeof(uint32_t));

        int max_blocks = (n + 511) / 512;
        cudaMalloc(&d_block_g, max_blocks * sizeof(uint32_t));
        cudaMalloc(&d_block_p, max_blocks * sizeof(uint32_t));
        cudaMalloc(&d_block_carry, max_blocks * sizeof(uint32_t));
        cudaMalloc(&d_result, n * sizeof(uint32_t));

        s_buf_cap = n;
        s_cached_tw_n = 0;
    }

    if (s_pinned_cap < (size_t)n) {
        if (s_pinned_a) cudaFreeHost(s_pinned_a);
        if (s_pinned_b) cudaFreeHost(s_pinned_b);
        if (s_pinned_res) cudaFreeHost(s_pinned_res);

        cudaHostAlloc(&s_pinned_a, n * sizeof(uint32_t), cudaHostAllocDefault);
        cudaHostAlloc(&s_pinned_b, n * sizeof(uint32_t), cudaHostAllocDefault);
        cudaHostAlloc(&s_pinned_res, n * sizeof(uint32_t), cudaHostAllocDefault);
        s_pinned_cap = n;
    }
    auto t_alloc = std::chrono::steady_clock::now();

    // Initialize twiddle factor cache on GPU if transform size changed
    if (s_cached_tw_n != n) {
        for (int pi = 0; pi < 3; pi++) {
            uint32_t p = P[pi];
            uint32_t p_inv = P_INV[pi];
            uint32_t w_n = npow(GEN[pi], (p - 1) / n, p);
            uint32_t w_n_mont = (uint32_t)(((uint64_t)w_n << 32) % p);
            int blk = 256;
            int grid = (n + blk - 1) / blk;
            void* args_n[] = { &d_tw_N[pi], &n, &w_n_mont, &p, &p_inv };
            cuLaunchKernel(g_nvrtcNtt.fn_init_twiddles, grid, 1, 1, blk, 1, 1, 0, s_streams[pi], args_n, nullptr);

            if (n > 2048) {
                uint32_t w_n1 = npow(GEN[pi], (p - 1) / N1, p);
                uint32_t w_n1_mont = (uint32_t)(((uint64_t)w_n1 << 32) % p);
                int grid1 = (N1 + blk - 1) / blk;
                void* args_n1[] = { &d_tw_N1[pi], &N1, &w_n1_mont, &p, &p_inv };
                cuLaunchKernel(g_nvrtcNtt.fn_init_twiddles, grid1, 1, 1, blk, 1, 1, 0, s_streams[pi], args_n1, nullptr);

                uint32_t w_n2 = npow(GEN[pi], (p - 1) / N2, p);
                uint32_t w_n2_mont = (uint32_t)(((uint64_t)w_n2 << 32) % p);
                int grid2 = (N2 + blk - 1) / blk;
                void* args_n2[] = { &d_tw_N2[pi], &N2, &w_n2_mont, &p, &p_inv };
                cuLaunchKernel(g_nvrtcNtt.fn_init_twiddles, grid2, 1, 1, blk, 1, 1, 0, s_streams[pi], args_n2, nullptr);
            }
        }
        s_cached_tw_n = n;
    }
    auto t_twiddle = std::chrono::steady_clock::now();

    // Populate pinned host memory (parallelized)
    memset(s_pinned_a, 0, n * sizeof(uint32_t));
    memset(s_pinned_b, 0, n * sizeof(uint32_t));
    #pragma omp parallel for schedule(static) if(la.size() > 10000)
    for (int i = 0; i < (int)la.size(); i++) s_pinned_a[i] = (uint32_t)la[i];
    #pragma omp parallel for schedule(static) if(lb.size() > 10000)
    for (int i = 0; i < (int)lb.size(); i++) s_pinned_b[i] = (uint32_t)lb[i];
    auto t_pin = std::chrono::steady_clock::now();

    // Launch 3 prime NTT pipelines across 3 concurrent CUDA streams
    for (int pi = 0; pi < 3; pi++) {
        CUstream st = s_streams[pi];
        cudaMemcpyAsync(d_a[pi], s_pinned_a, n * sizeof(uint32_t), cudaMemcpyHostToDevice, (cudaStream_t)st);
        cudaMemcpyAsync(d_b[pi], s_pinned_b, n * sizeof(uint32_t), cudaMemcpyHostToDevice, (cudaStream_t)st);

        // Forward NTT on a and b
        run_ntt_stream(pi, d_a[pi], d_scratch[pi], n, N1, N2, k1, k2, false, 0, st);
        run_ntt_stream(pi, d_b[pi], d_scratch[pi], n, N1, N2, k1, k2, false, 0, st);

        // Pointwise multiply a *= b
        int blk = 256;
        int grid = (n + blk - 1) / blk;
        uint32_t p = P[pi];
        uint64_t m_barrett = M_BARRETT[pi];
        void* args_mul[] = { &d_a[pi], &d_b[pi], &n, &p, &m_barrett };
        cuLaunchKernel(g_nvrtcNtt.fn_pointwise_mul, grid, 1, 1, blk, 1, 1, 0, st, args_mul, nullptr);

        // Inverse NTT on a, scaling by n^(-1)
        uint32_t inv_n = npow(n, p - 2, p);
        uint32_t inv_n_mont = (uint32_t)(((uint64_t)inv_n << 32) % p);
        run_ntt_stream(pi, d_a[pi], d_scratch[pi], n, N1, N2, k1, k2, true, inv_n_mont, st);
    }
    auto t_launch = std::chrono::steady_clock::now();

    // Synchronize streams 1 and 2, continue pipeline on stream 0
    cudaStreamSynchronize((cudaStream_t)s_streams[1]);
    cudaStreamSynchronize((cudaStream_t)s_streams[2]);
    auto t_ntt_wait = std::chrono::steady_clock::now();
    CUstream st0 = s_streams[0];

    // GPU CRT Kernel
    int blk = 256;
    int grid = (n + blk - 1) / blk;
    uint32_t p1 = P[0], p2 = P[1], p3 = P[2];
    void* args_crt[] = { &d_rem, &d_q, &d_a[0], &d_a[1], &d_a[2], &n,
                         (void*)&p1, (void*)&p2, (void*)&p3,
                         (void*)&inv12, (void*)&inv123,
                         (void*)&q_P1P2, (void*)&rem_P1P2 };
    cuLaunchKernel(g_nvrtcNtt.fn_crt, grid, 1, 1, blk, 1, 1, 0, st0, args_crt, nullptr);

    // Carry Reduction Rounds 1 and 2
    void* args_c1[] = { &d_rem1, &d_c1, &d_rem, &d_q, &n };
    cuLaunchKernel(g_nvrtcNtt.fn_carry_round1, grid, 1, 1, blk, 1, 1, 0, st0, args_c1, nullptr);

    void* args_c2[] = { &d_rem2, &d_c2, &d_rem1, &d_c1, &n };
    cuLaunchKernel(g_nvrtcNtt.fn_carry_round2, grid, 1, 1, blk, 1, 1, 0, st0, args_c2, nullptr);

    // 3-Phase Block-Scan Parallel Prefix Carry Elimination
    int num_blocks = (n + 511) / 512;
    void* args_p1[] = { &d_local_g, &d_local_p, &d_block_g, &d_block_p, &d_rem2, &d_c2, &n };
    cuLaunchKernel(g_nvrtcNtt.fn_carry_scan_phase1, num_blocks, 1, 1, 512, 1, 1, 0, st0, args_p1, nullptr);

    void* args_p2[] = { &d_block_carry, &d_block_g, &d_block_p, &num_blocks };
    cuLaunchKernel(g_nvrtcNtt.fn_carry_scan_phase2, 1, 1, 1, 512, 1, 1, 0, st0, args_p2, nullptr);

    void* args_p3[] = { &d_result, &d_rem2, &d_local_g, &d_local_p, &d_block_carry, &n };
    cuLaunchKernel(g_nvrtcNtt.fn_carry_scan_phase3, num_blocks, 1, 1, 512, 1, 1, 0, st0, args_p3, nullptr);

    // Transfer result back to host via pinned memory
    cudaMemcpyAsync(s_pinned_res, d_result, n * sizeof(uint32_t), cudaMemcpyDeviceToHost, (cudaStream_t)st0);
    cudaStreamSynchronize((cudaStream_t)st0);
    auto t_crt_carry = std::chrono::steady_clock::now();

    // Find actual limb length and copy back to vector
    int rl = (int)(la.size() + lb.size());
    if (rl > n) rl = n;
    while (rl > 0 && s_pinned_res[rl - 1] == 0) rl--;
    if (rl == 0) return {};

    std::vector<uint64_t> lr(rl);
    #pragma omp parallel for schedule(static) if(rl > 10000)
    for (int i = 0; i < rl; i++) lr[i] = s_pinned_res[i];
    auto t_end = std::chrono::steady_clock::now();

    if (g_bigTime) {
        fprintf(stderr, "[GPU Detail] alloc=%.2fms twiddle=%.2fms pin=%.2fms launch=%.2fms ntt_wait=%.2fms crt_carry=%.2fms copy=%.2fms\n",
            std::chrono::duration<double, std::milli>(t_alloc - t_start).count(),
            std::chrono::duration<double, std::milli>(t_twiddle - t_alloc).count(),
            std::chrono::duration<double, std::milli>(t_pin - t_twiddle).count(),
            std::chrono::duration<double, std::milli>(t_launch - t_pin).count(),
            std::chrono::duration<double, std::milli>(t_ntt_wait - t_launch).count(),
            std::chrono::duration<double, std::milli>(t_crt_carry - t_ntt_wait).count(),
            std::chrono::duration<double, std::milli>(t_end - t_crt_carry).count());
    }

    return lr;
}
#endif

// ---- GPU model detection: universal compatibility for all CC >= 6.0 GPUs ----
bool g_forceGPU = false;
bool g_bigTime = false;

bool bigint_gpu_suitable(size_t digitCount = 0) {
#ifdef WITH_CUDA
    if (g_forceGPU) {
        if (!torch::cuda::is_available()) return false;
        init_nvrtc_ntt();
        return true;
    }

    static bool s_checked = false;
    static bool s_suitable = false;
    static const size_t s_crossoverDigits = 100000; // 100k digits crossover threshold
    if (!s_checked) {
        s_checked = true;
        if (!torch::cuda::is_available()) return false;
        try {
            int dev = 0;
            if (cudaGetDevice(&dev) != cudaSuccess) return false;
            cudaDeviceProp prop;
            if (cudaGetDeviceProperties(&prop, dev) != cudaSuccess) return false;
            if (prop.major >= 6) {
                s_suitable = true;
            } else {
                s_suitable = false;
            }
        } catch (...) {
            s_suitable = false;
        }
    }
    if (!s_suitable) return false;
    if (digitCount < s_crossoverDigits) return false;
    init_nvrtc_ntt();
    return true;
#else
    return false;
#endif
}

std::string bigint_mul_fft(const std::string& a, const std::string& b) {
    bool an = bigint_is_neg(a), bn = bigint_is_neg(b);
    std::string aa = bigint_abs(a), bb = bigint_abs(b);

#ifdef WITH_CUDA
    if (bigint_gpu_suitable(aa.size() > bb.size() ? aa.size() : bb.size())) {
        try {
            auto la = limbs_from_str(aa);
            auto lb = limbs_from_str(bb);
            auto lr = bigint_mul_gpu_ntt_limbs(la, lb);
            std::string r = limbs_to_str(lr);
            bool neg = an != bn;
            return (neg && r != "0") ? "-" + r : r;
        } catch (...) {
            // Fall through to throw
        }
    }
#endif

    throw std::runtime_error("GPU NTT unavailable, use standalone FFT");
}

bool bigint_fft_available() {
    // With CUDA libtorch, FFT runs on GPU — no MKL needed.
    // With CPU libtorch, check for MKL dispatch DLLs.
    static bool checked = false;
    static bool available = false;
    if (checked) return available;
    checked = true;

#ifdef WITH_CUDA
    // GPU FFT available if CUDA is present
    try {
        available = torch::cuda::is_available();
    } catch (...) { available = false; }
    if (available) return true;
#endif

#ifdef _WIN32
    // CPU fallback: check for MKL dispatch DLLs
    HMODULE h = LoadLibraryA("mkl_def.1.dll");
    if (!h) h = LoadLibraryA("mkl_avx2.1.dll");
    if (h) { FreeLibrary(h); available = true; }
#else
    void* h = dlopen("libmkl_def.so.1", RTLD_LAZY);
    if (!h) h = dlopen("libmkl_avx2.so.1", RTLD_LAZY);
    if (h) { dlclose(h); available = true; }
#endif
    return available;
}

// ============================================================================
// Helper: parse shape from TzdValue args (variadic or array)
// ============================================================================
static std::vector<int64_t> parseShape(const std::vector<TzdValue>& args, size_t startIdx = 0) {
    std::vector<int64_t> shape;
    for (size_t i = startIdx; i < args.size(); ++i) {
        if (args[i].type == TzdValue::ARRAY) {
            for (const auto& elem : args[i].arrVal) {
                shape.push_back((int64_t)valToDouble(elem));
            }
        } else {
            shape.push_back((int64_t)valToDouble(args[i]));
        }
    }
    return shape;
}

// Helper: parse a tensor from TzdValue array (nested arrays for N-dim)
static at::Tensor arrayToTensor(const TzdValue& val, torch::Dtype dtype = torch::kFloat32) {
    if (val.type == TzdValue::ARRAY) {
        if (val.arrVal.empty()) {
            return torch::empty({0}, at::TensorOptions().dtype(dtype));
        }
        // Check if elements are arrays (multi-dimensional)
        if (val.arrVal[0].type == TzdValue::ARRAY) {
            std::vector<at::Tensor> subTensors;
            for (const auto& elem : val.arrVal) {
                subTensors.push_back(arrayToTensor(elem, dtype));
            }
            return torch::stack(subTensors, 0);
        }
        // 1-D array
        std::vector<double> data;
        for (const auto& elem : val.arrVal) {
            data.push_back(valToDouble(elem));
        }
        return at::from_blob(data.data(), {(int64_t)data.size()}, at::TensorOptions().dtype(dtype)).clone();
    }
    // Scalar
    double scalarVal = valToDouble(val);
    return at::from_blob(&scalarVal, {1}, at::TensorOptions().dtype(dtype)).clone();
}

// Helper: convert tensor to TzdValue array
static TzdValue tensorToArray(const at::Tensor& t) {
    if (t.dim() == 0) {
        return TzdValue(t.item<double>());
    }
    if (t.dim() == 1) {
        std::vector<TzdValue> arr;
        for (int64_t i = 0; i < t.size(0); ++i) {
            arr.push_back(TzdValue(t[i].item<double>()));
        }
        return TzdValue(arr);
    }
    std::vector<TzdValue> arr;
    for (int64_t i = 0; i < t.size(0); ++i) {
        arr.push_back(tensorToArray(t[i]));
    }
    return TzdValue(arr);
}

// Helper: get a single tensor from args, or throw
static at::Tensor* requireTensor(const TzdValue& val, const std::string& funcName) {
    at::Tensor* t = TzdPyTorch::getTensor(val);
    if (!t) {
        throw std::runtime_error(funcName + ": expected a Tensor argument");
    }
    return t;
}

// ============================================================================
// Initialization
// ============================================================================
void TzdPyTorch::init(TzdInterpreter* interp) {
    if (s_initialized) return;
    s_initialized = true;

    // Set number of threads for CPU operations
    at::set_num_threads(std::thread::hardware_concurrency());

    regTensorCreation(interp);
    regTensorOps(interp);
    regTensorProperties(interp);
    regReductionOps(interp);
    regIndexingOps(interp);
    regNNFunctions(interp);
    regOptimizers(interp);
    regDeviceMgmt(interp);
    regAutograd(interp);
    regSerialization(interp);
    regConversion(interp);
    regExtendedOps(interp);

    // Register cleanup callback 鈥?release all tensors when interpreter shuts down
    // Using atexit to ensure cleanup even if interpreter is not explicitly destroyed
    std::atexit([]() { TzdPyTorch::releaseAll(); });
}

// ============================================================================
// 1. Tensor Creation Functions
// ============================================================================
void TzdPyTorch::regTensorCreation(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_tensor", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_tensor: requires at least one argument");
        // If first arg is an array, use it directly
        if (args[0].type == TzdValue::ARRAY) {
            return wrapTensor(new at::Tensor(arrayToTensor(args[0])));
        }
        // If first arg is a tensor, return a clone
        if (auto* t = getTensor(args[0])) {
            return wrapTensor(new at::Tensor(t->clone()));
        }
        // Otherwise, collect all scalar args as a 1-D tensor
        std::vector<float> data;
        for (const auto& a : args) {
            data.push_back((float)valToDouble(a));
        }
        return wrapTensor(new at::Tensor(
            at::from_blob(data.data(), {(int64_t)data.size()}, at::TensorOptions().dtype(torch::kFloat32)).clone()));
    });

    reg("torch_zeros", [](auto args) -> TzdValue {
        auto shape = parseShape(args);
        if (shape.empty()) shape = {1};
        return wrapTensor(new at::Tensor(torch::zeros(shape, torch::kFloat32)));
    });

    reg("torch_ones", [](auto args) -> TzdValue {
        auto shape = parseShape(args);
        if (shape.empty()) shape = {1};
        return wrapTensor(new at::Tensor(torch::ones(shape, torch::kFloat32)));
    });

    reg("torch_empty", [](auto args) -> TzdValue {
        auto shape = parseShape(args);
        if (shape.empty()) shape = {1};
        return wrapTensor(new at::Tensor(torch::empty(shape, torch::kFloat32)));
    });

    reg("torch_rand", [](auto args) -> TzdValue {
        auto shape = parseShape(args);
        if (shape.empty()) shape = {1};
        return wrapTensor(new at::Tensor(torch::rand(shape)));
    });

    reg("torch_randn", [](auto args) -> TzdValue {
        auto shape = parseShape(args);
        if (shape.empty()) shape = {1};
        return wrapTensor(new at::Tensor(torch::randn(shape)));
    });

    reg("torch_randint", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_randint: requires (low, high, ...shape)");
        int64_t low = (int64_t)valToDouble(args[0]);
        int64_t high = (int64_t)valToDouble(args[1]);
        auto shape = parseShape(args, 2);
        if (shape.empty()) shape = {1};
        return wrapTensor(new at::Tensor(torch::randint(low, high, shape, torch::kInt64)));
    });

    reg("torch_arange", [](auto args) -> TzdValue {
        double start, end, step;
        if (args.size() == 1) { start = 0; end = valToDouble(args[0]); step = 1; }
        else if (args.size() == 2) { start = valToDouble(args[0]); end = valToDouble(args[1]); step = 1; }
        else if (args.size() >= 3) { start = valToDouble(args[0]); end = valToDouble(args[1]); step = valToDouble(args[2]); }
        else { return TzdValue::Error("torch_arange: requires at least one argument"); }
        return wrapTensor(new at::Tensor(torch::arange(start, end, step)));
    });

    reg("torch_linspace", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_linspace: requires (start, end, [steps])");
        double start = valToDouble(args[0]);
        double end = valToDouble(args[1]);
        int64_t steps = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : 100;
        return wrapTensor(new at::Tensor(torch::linspace(start, end, steps)));
    });

    reg("torch_logspace", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_logspace: requires (start, end, [steps], [base])");
        double start = valToDouble(args[0]);
        double end = valToDouble(args[1]);
        int64_t steps = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : 100;
        double base = (args.size() > 3) ? valToDouble(args[3]) : 10.0;
        return wrapTensor(new at::Tensor(torch::logspace(start, end, steps, base)));
    });

    reg("torch_eye", [](auto args) -> TzdValue {
        int64_t n = args.empty() ? 1 : (int64_t)valToDouble(args[0]);
        int64_t m = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : n;
        return wrapTensor(new at::Tensor(torch::eye(n, m)));
    });

    reg("torch_full", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_full: requires (fillValue, ...shape)");
        double fillVal = valToDouble(args[0]);
        auto shape = parseShape(args, 1);
        if (shape.empty()) shape = {1};
        return wrapTensor(new at::Tensor(torch::full(shape, fillVal)));
    });

    reg("torch_identity", [](auto args) -> TzdValue {
        int64_t n = args.empty() ? 1 : (int64_t)valToDouble(args[0]);
        return wrapTensor(new at::Tensor(torch::eye(n)));
    });
}

// ============================================================================
// 2. Tensor Operations (element-wise)
// ============================================================================
void TzdPyTorch::regTensorOps(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_add", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_add: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_add");
        auto* b = requireTensor(args[1], "torch_add");
        double alpha = (args.size() > 2) ? valToDouble(args[2]) : 1.0;
        return wrapTensor(new at::Tensor(*a + alpha * *b));
    });

    reg("torch_sub", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_sub: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_sub");
        auto* b = requireTensor(args[1], "torch_sub");
        double alpha = (args.size() > 2) ? valToDouble(args[2]) : 1.0;
        return wrapTensor(new at::Tensor(*a - alpha * *b));
    });

    reg("torch_mul", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_mul: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_mul");
        auto* b = requireTensor(args[1], "torch_mul");
        return wrapTensor(new at::Tensor(*a * *b));
    });

    reg("torch_div", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_div: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_div");
        auto* b = requireTensor(args[1], "torch_div");
        return wrapTensor(new at::Tensor(*a / *b));
    });

    reg("torch_matmul", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_matmul: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_matmul");
        auto* b = requireTensor(args[1], "torch_matmul");
        return wrapTensor(new at::Tensor(torch::matmul(*a, *b)));
    });

    reg("torch_mm", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_mm: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_mm");
        auto* b = requireTensor(args[1], "torch_mm");
        return wrapTensor(new at::Tensor(torch::mm(*a, *b)));
    });

    reg("torch_bmm", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_bmm: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_bmm");
        auto* b = requireTensor(args[1], "torch_bmm");
        return wrapTensor(new at::Tensor(torch::bmm(*a, *b)));
    });

    reg("torch_neg", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_neg: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_neg");
        return wrapTensor(new at::Tensor(-*a));
    });

    reg("torch_abs", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_abs: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_abs");
        return wrapTensor(new at::Tensor(torch::abs(*a)));
    });

    reg("torch_exp", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_exp: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_exp");
        return wrapTensor(new at::Tensor(torch::exp(*a)));
    });

    reg("torch_log", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_log: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_log");
        return wrapTensor(new at::Tensor(torch::log(*a)));
    });

    reg("torch_sqrt", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_sqrt: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_sqrt");
        return wrapTensor(new at::Tensor(torch::sqrt(*a)));
    });

    reg("torch_pow", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_pow: requires (tensor, exponent)");
        auto* a = requireTensor(args[0], "torch_pow");
        double exponent = valToDouble(args[1]);
        return wrapTensor(new at::Tensor(torch::pow(*a, exponent)));
    });

    reg("torch_clamp", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_clamp: requires (tensor, [min], [max])");
        auto* a = requireTensor(args[0], "torch_clamp");
        if (args.size() >= 3) {
            return wrapTensor(new at::Tensor(torch::clamp(*a, valToDouble(args[1]), valToDouble(args[2]))));
        } else if (args.size() == 2) {
            return wrapTensor(new at::Tensor(torch::clamp(*a, valToDouble(args[1]))));
        }
        return wrapTensor(new at::Tensor(*a));
    });

    reg("torch_transpose", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_transpose: requires (tensor, [dim0], [dim1])");
        auto* a = requireTensor(args[0], "torch_transpose");
        if (args.size() >= 3) {
            int64_t dim0 = (int64_t)valToDouble(args[1]);
            int64_t dim1 = (int64_t)valToDouble(args[2]);
            return wrapTensor(new at::Tensor(a->transpose(dim0, dim1)));
        }
        return wrapTensor(new at::Tensor(a->transpose(0, 1)));
    });

    reg("torch_reshape", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_reshape: requires (tensor, ...shape)");
        auto* a = requireTensor(args[0], "torch_reshape");
        auto shape = parseShape(args, 1);
        return wrapTensor(new at::Tensor(a->reshape(shape)));
    });

    reg("torch_view", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_view: requires (tensor, ...shape)");
        auto* a = requireTensor(args[0], "torch_view");
        auto shape = parseShape(args, 1);
        return wrapTensor(new at::Tensor(a->view(shape)));
    });

    reg("torch_flatten", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_flatten: requires (tensor, [startDim], [endDim])");
        auto* a = requireTensor(args[0], "torch_flatten");
        int64_t startDim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        int64_t endDim = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : -1;
        return wrapTensor(new at::Tensor(torch::flatten(*a, startDim, endDim)));
    });

    reg("torch_squeeze", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_squeeze: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_squeeze");
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(a->squeeze((int64_t)valToDouble(args[1]))));
        }
        return wrapTensor(new at::Tensor(a->squeeze()));
    });

    reg("torch_unsqueeze", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_unsqueeze: requires (tensor, dim)");
        auto* a = requireTensor(args[0], "torch_unsqueeze");
        return wrapTensor(new at::Tensor(a->unsqueeze((int64_t)valToDouble(args[1]))));
    });

    reg("torch_permute", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_permute: requires (tensor, ...dims)");
        auto* a = requireTensor(args[0], "torch_permute");
        std::vector<int64_t> dims;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].type == TzdValue::ARRAY) {
                for (const auto& d : args[i].arrVal) dims.push_back((int64_t)valToDouble(d));
            } else {
                dims.push_back((int64_t)valToDouble(args[i]));
            }
        }
        return wrapTensor(new at::Tensor(a->permute(dims)));
    });

    reg("torch_contiguous", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_contiguous: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_contiguous");
        return wrapTensor(new at::Tensor(a->contiguous()));
    });

    reg("torch_clone", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_clone: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_clone");
        return wrapTensor(new at::Tensor(a->clone()));
    });

    reg("torch_cat", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_cat: requires (tensors..., [dim])");
        std::vector<at::Tensor> tensors;
        int64_t dim = 0;
        for (const auto& arg : args) {
            if (arg.type == TzdValue::ARRAY) {
                for (const auto& elem : arg.arrVal) {
                    auto* t = getTensor(elem);
                    if (t) tensors.push_back(*t);
                }
            } else if (arg.type == TzdValue::POINTER || arg.type == TzdValue::TENSOR) {
                auto* t = getTensor(arg);
                if (t) tensors.push_back(*t);
            } else {
                dim = (int64_t)valToDouble(arg);
            }
        }
        if (tensors.empty()) return TzdValue::Error("torch_cat: no tensors provided");
        return wrapTensor(new at::Tensor(torch::cat(tensors, dim)));
    });

    reg("torch_stack", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_stack: requires (tensors..., [dim])");
        std::vector<at::Tensor> tensors;
        int64_t dim = 0;
        for (const auto& arg : args) {
            if (arg.type == TzdValue::ARRAY) {
                for (const auto& elem : arg.arrVal) {
                    auto* t = getTensor(elem);
                    if (t) tensors.push_back(*t);
                }
            } else if (arg.type == TzdValue::POINTER || arg.type == TzdValue::TENSOR) {
                auto* t = getTensor(arg);
                if (t) tensors.push_back(*t);
            } else {
                dim = (int64_t)valToDouble(arg);
            }
        }
        if (tensors.empty()) return TzdValue::Error("torch_stack: no tensors provided");
        return wrapTensor(new at::Tensor(torch::stack(tensors, dim)));
    });

    // Element-wise math functions
    auto unaryTensorFn = [](const std::string& name, auto fn) -> TzdValue::NativeFuncType {
        return [name, fn](auto args) -> TzdValue {
            if (args.empty()) return TzdValue::Error(name + ": requires 1 tensor arg");
            auto* a = requireTensor(args[0], name);
            return wrapTensor(new at::Tensor(fn(*a)));
        };
    };
    reg("torch_sin", unaryTensorFn("torch_sin", [](const at::Tensor& t) { return torch::sin(t); }));
    reg("torch_cos", unaryTensorFn("torch_cos", [](const at::Tensor& t) { return torch::cos(t); }));
    reg("torch_tanh", unaryTensorFn("torch_tanh", [](const at::Tensor& t) { return torch::tanh(t); }));
    reg("torch_sigmoid", unaryTensorFn("torch_sigmoid", [](const at::Tensor& t) { return torch::sigmoid(t); }));
    reg("torch_reciprocal", unaryTensorFn("torch_reciprocal", [](const at::Tensor& t) { return torch::reciprocal(t); }));
    reg("torch_sign", unaryTensorFn("torch_sign", [](const at::Tensor& t) { return torch::sign(t); }));
    reg("torch_floor", unaryTensorFn("torch_floor", [](const at::Tensor& t) { return torch::floor(t); }));
    reg("torch_ceil", unaryTensorFn("torch_ceil", [](const at::Tensor& t) { return torch::ceil(t); }));
    reg("torch_round_t", unaryTensorFn("torch_round_t", [](const at::Tensor& t) { return torch::round(t); }));
    reg("torch_trunc", unaryTensorFn("torch_trunc", [](const at::Tensor& t) { return torch::trunc(t); }));
    reg("torch_frac", unaryTensorFn("torch_frac", [](const at::Tensor& t) { return torch::frac(t); }));

    reg("torch_where", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_where: requires (condition, x, y)");
        auto* cond = requireTensor(args[0], "torch_where");
        auto* x = requireTensor(args[1], "torch_where");
        auto* y = requireTensor(args[2], "torch_where");
        return wrapTensor(new at::Tensor(torch::where(*cond, *x, *y)));
    });

    reg("torch_norm_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_norm_t: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_norm_t");
        return wrapTensor(new at::Tensor(torch::norm(*a)));
    });

    reg("torch_lerp", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_lerp: requires (start, end, weight)");
        auto* a = requireTensor(args[0], "torch_lerp");
        auto* b = requireTensor(args[1], "torch_lerp");
        double w = valToDouble(args[2]);
        return wrapTensor(new at::Tensor(torch::lerp(*a, *b, w)));
    });
}

// ============================================================================
// 3. Tensor Properties
// ============================================================================
void TzdPyTorch::regTensorProperties(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_shape", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_shape: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_shape");
        std::vector<TzdValue> shape;
        for (int i = 0; i < a->dim(); ++i) {
            shape.push_back(TzdValue((double)a->size(i)));
        }
        return TzdValue(shape);
    });

    reg("torch_dim", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = requireTensor(args[0], "torch_dim");
        return TzdValue((double)a->dim());
    });

    reg("torch_numel", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = requireTensor(args[0], "torch_numel");
        return TzdValue((double)a->numel());
    });

    reg("torch_element_size", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = requireTensor(args[0], "torch_element_size");
        return TzdValue((double)a->element_size());
    });

    reg("torch_nbytes", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = requireTensor(args[0], "torch_nbytes");
        return TzdValue((double)a->nbytes());
    });

    reg("torch_is_contiguous", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = requireTensor(args[0], "torch_is_contiguous");
        return TzdValue(a->is_contiguous());
    });

    reg("torch_dtype", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("unknown");
        auto* a = requireTensor(args[0], "torch_dtype");
        if (a->scalar_type() == torch::kFloat32) return TzdValue("float32");
        if (a->scalar_type() == torch::kFloat64) return TzdValue("float64");
        if (a->scalar_type() == torch::kFloat16) return TzdValue("float16");
        if (a->scalar_type() == torch::kBFloat16) return TzdValue("bfloat16");
        if (a->scalar_type() == torch::kInt32) return TzdValue("int32");
        if (a->scalar_type() == torch::kInt64) return TzdValue("int64");
        if (a->scalar_type() == torch::kInt16) return TzdValue("int16");
        if (a->scalar_type() == torch::kInt8) return TzdValue("int8");
        if (a->scalar_type() == torch::kUInt8) return TzdValue("uint8");
        if (a->scalar_type() == torch::kBool) return TzdValue("bool");
        if (a->scalar_type() == torch::kQInt8) return TzdValue("qint8");
        if (a->scalar_type() == torch::kQUInt8) return TzdValue("quint8");
        return TzdValue("unknown");
    });

    reg("torch_device_str", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("cpu");
        auto* a = requireTensor(args[0], "torch_device_str");
        return TzdValue(std::string(a->device().str()));
    });

    reg("torch_to_dtype", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_to_dtype: requires (tensor, dtypeStr)");
        auto* a = requireTensor(args[0], "torch_to_dtype");
        std::string dtStr = args[1].sVal;
        torch::Dtype dt = torch::kFloat32;
        if (dtStr == "float64" || dtStr == "double") dt = torch::kFloat64;
        else if (dtStr == "float32" || dtStr == "float") dt = torch::kFloat32;
        else if (dtStr == "float16" || dtStr == "half") dt = torch::kFloat16;
        else if (dtStr == "bfloat16" || dtStr == "bf16") dt = torch::kBFloat16;
        else if (dtStr == "int64" || dtStr == "long") dt = torch::kInt64;
        else if (dtStr == "int32" || dtStr == "int") dt = torch::kInt32;
        else if (dtStr == "int16" || dtStr == "short") dt = torch::kInt16;
        else if (dtStr == "int8" || dtStr == "char") dt = torch::kInt8;
        else if (dtStr == "uint8" || dtStr == "byte") dt = torch::kUInt8;
        else if (dtStr == "bool") dt = torch::kBool;
        else return TzdValue::Error("torch_to_dtype: unknown dtype '" + dtStr + "'");
        return wrapTensor(new at::Tensor(a->to(dt)));
    });

    reg("torch_fill_", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_fill_: requires (tensor, value)");
        auto* a = requireTensor(args[0], "torch_fill_");
        a->fill_(valToDouble(args[1]));
        return args[0];
    });

    reg("torch_print", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        auto* a = requireTensor(args[0], "torch_print");
        std::ostringstream ss;
        ss << *a;
        std::cout << ss.str() << std::endl;
        return TzdValue();
    });

    reg("torch_to_string", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue("");
        auto* a = requireTensor(args[0], "torch_to_string");
        std::ostringstream ss;
        ss << *a;
        return TzdValue(ss.str());
    });
}

// ============================================================================
// 4. Reduction Operations
// ============================================================================
void TzdPyTorch::regReductionOps(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_sum", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_sum: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_sum");
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(a->sum((int64_t)valToDouble(args[1]))));
        }
        return wrapTensor(new at::Tensor(a->sum()));
    });

    reg("torch_mean", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_mean: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_mean");
        if (a->scalar_type() == torch::kInt64 || a->scalar_type() == torch::kInt32) {
            // mean requires floating point 鈥?fix: properly manage temporary tensor
            at::Tensor floatTensor = a->to(torch::kFloat32);
            if (args.size() > 1) return wrapTensor(new at::Tensor(floatTensor.mean((int64_t)valToDouble(args[1]))));
            return wrapTensor(new at::Tensor(floatTensor.mean()));
        }
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(a->mean((int64_t)valToDouble(args[1]))));
        }
        return wrapTensor(new at::Tensor(a->mean()));
    });

    reg("torch_max_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_max_t: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_max_t");
        if (args.size() > 1) {
            auto result = a->max((int64_t)valToDouble(args[1]));
            return wrapTensor(new at::Tensor(std::get<0>(result)));
        }
        return wrapTensor(new at::Tensor(a->max()));
    });

    reg("torch_min_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_min_t: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_min_t");
        if (args.size() > 1) {
            auto result = a->min((int64_t)valToDouble(args[1]));
            return wrapTensor(new at::Tensor(std::get<0>(result)));
        }
        return wrapTensor(new at::Tensor(a->min()));
    });

    reg("torch_argmax", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_argmax: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_argmax");
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(a->argmax((int64_t)valToDouble(args[1]))));
        }
        return wrapTensor(new at::Tensor(a->argmax()));
    });

    reg("torch_argmin", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_argmin: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_argmin");
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(a->argmin((int64_t)valToDouble(args[1]))));
        }
        return wrapTensor(new at::Tensor(a->argmin()));
    });

    reg("torch_prod", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_prod: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_prod");
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(a->prod((int64_t)valToDouble(args[1]))));
        }
        return wrapTensor(new at::Tensor(a->prod()));
    });

    reg("torch_cumsum", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_cumsum: requires (tensor, dim)");
        auto* a = requireTensor(args[0], "torch_cumsum");
        return wrapTensor(new at::Tensor(a->cumsum((int64_t)valToDouble(args[1]))));
    });

    reg("torch_cumprod", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_cumprod: requires (tensor, dim)");
        auto* a = requireTensor(args[0], "torch_cumprod");
        return wrapTensor(new at::Tensor(a->cumprod((int64_t)valToDouble(args[1]))));
    });

    reg("torch_std", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_std: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_std");
        if (a->scalar_type() != torch::kFloat32 && a->scalar_type() != torch::kFloat64) {
            // fix: properly manage temporary tensor to prevent leak
            at::Tensor floatTensor = a->to(torch::kFloat32);
            if (args.size() > 1) return wrapTensor(new at::Tensor(at::std(floatTensor, (int64_t)valToDouble(args[1]), false, false)));
            return wrapTensor(new at::Tensor(at::std(floatTensor, false, false)));
        }
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(at::std(*a, (int64_t)valToDouble(args[1]), false, false)));
        }
        return wrapTensor(new at::Tensor(at::std(*a, false, false)));
    });

    reg("torch_var", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_var: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_var");
        if (a->scalar_type() != torch::kFloat32 && a->scalar_type() != torch::kFloat64) {
            // fix: properly manage temporary tensor to prevent leak
            at::Tensor floatTensor = a->to(torch::kFloat32);
            if (args.size() > 1) return wrapTensor(new at::Tensor(at::var(floatTensor, (int64_t)valToDouble(args[1]), false, false)));
            return wrapTensor(new at::Tensor(at::var(floatTensor, false, false)));
        }
        if (args.size() > 1) {
            return wrapTensor(new at::Tensor(at::var(*a, (int64_t)valToDouble(args[1]), false, false)));
        }
        return wrapTensor(new at::Tensor(at::var(*a, false, false)));
    });
}

// ============================================================================
// 5. Indexing Operations
// ============================================================================
void TzdPyTorch::regIndexingOps(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_index_select", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_index_select: requires (tensor, dim, indices)");
        auto* a = requireTensor(args[0], "torch_index_select");
        int64_t dim = (int64_t)valToDouble(args[1]);
        auto* idx = requireTensor(args[2], "torch_index_select");
        return wrapTensor(new at::Tensor(torch::index_select(*a, dim, *idx)));
    });

    reg("torch_gather", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_gather: requires (tensor, dim, index)");
        auto* a = requireTensor(args[0], "torch_gather");
        int64_t dim = (int64_t)valToDouble(args[1]);
        auto* idx = requireTensor(args[2], "torch_gather");
        return wrapTensor(new at::Tensor(torch::gather(*a, dim, *idx)));
    });

    reg("torch_scatter", [](auto args) -> TzdValue {
        if (args.size() < 4) return TzdValue::Error("torch_scatter: requires (tensor, dim, index, src)");
        auto* a = requireTensor(args[0], "torch_scatter");
        int64_t dim = (int64_t)valToDouble(args[1]);
        auto* idx = requireTensor(args[2], "torch_scatter");
        auto* src = requireTensor(args[3], "torch_scatter");
        return wrapTensor(new at::Tensor(torch::scatter(*a, dim, *idx, *src)));
    });

    reg("torch_masked_select", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_masked_select: requires (tensor, mask)");
        auto* a = requireTensor(args[0], "torch_masked_select");
        auto* mask = requireTensor(args[1], "torch_masked_select");
        return wrapTensor(new at::Tensor(torch::masked_select(*a, *mask)));
    });

    reg("torch_nonzero", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_nonzero: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_nonzero");
        return wrapTensor(new at::Tensor(torch::nonzero(*a)));
    });

    reg("torch_expand", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_expand: requires (tensor, ...shape)");
        auto* a = requireTensor(args[0], "torch_expand");
        auto shape = parseShape(args, 1);
        return wrapTensor(new at::Tensor(a->expand(shape)));
    });

    reg("torch_repeat", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_repeat: requires (tensor, ...repeats)");
        auto* a = requireTensor(args[0], "torch_repeat");
        std::vector<int64_t> repeats;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].type == TzdValue::ARRAY) {
                for (const auto& r : args[i].arrVal) repeats.push_back((int64_t)valToDouble(r));
            } else {
                repeats.push_back((int64_t)valToDouble(args[i]));
            }
        }
        return wrapTensor(new at::Tensor(a->repeat(repeats)));
    });

    reg("torch_broadcast_to", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_broadcast_to: requires (tensor, ...shape)");
        auto* a = requireTensor(args[0], "torch_broadcast_to");
        auto shape = parseShape(args, 1);
        return wrapTensor(new at::Tensor(a->broadcast_to(shape)));
    });
}

// ============================================================================
// 6. Neural Network Functions
// ============================================================================
void TzdPyTorch::regNNFunctions(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_relu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_relu: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_relu");
        return wrapTensor(new at::Tensor(torch::relu(*a)));
    });

    reg("torch_leaky_relu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_leaky_relu: requires (tensor, [negativeSlope=0.01])");
        auto* a = requireTensor(args[0], "torch_leaky_relu");
        double slope = (args.size() > 1) ? valToDouble(args[1]) : 0.01;
        return wrapTensor(new at::Tensor(torch::leaky_relu(*a, slope)));
    });

    reg("torch_elu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_elu: requires (tensor, [alpha=1.0])");
        auto* a = requireTensor(args[0], "torch_elu");
        double alpha = (args.size() > 1) ? valToDouble(args[1]) : 1.0;
        return wrapTensor(new at::Tensor(torch::elu(*a, alpha)));
    });

    reg("torch_gelu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_gelu: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_gelu");
        return wrapTensor(new at::Tensor(torch::gelu(*a)));
    });

    reg("torch_softmax", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_softmax: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_softmax");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : -1;
        return wrapTensor(new at::Tensor(torch::softmax(*a, dim)));
    });

    reg("torch_log_softmax", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_log_softmax: requires (tensor, [dim])");
        auto* a = requireTensor(args[0], "torch_log_softmax");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : -1;
        return wrapTensor(new at::Tensor(torch::log_softmax(*a, dim)));
    });

    reg("torch_sigmoid_fn", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_sigmoid_fn: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_sigmoid_fn");
        return wrapTensor(new at::Tensor(torch::sigmoid(*a)));
    });

    reg("torch_dropout", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_dropout: requires (tensor, p)");
        auto* a = requireTensor(args[0], "torch_dropout");
        double p = valToDouble(args[1]);
        bool training = (args.size() > 2) ? (bool)valToDouble(args[2]) : true;
        return wrapTensor(new at::Tensor(torch::dropout(*a, p, training)));
    });

    reg("torch_batch_norm", [](auto args) -> TzdValue {
        if (args.size() < 5) return TzdValue::Error("torch_batch_norm: requires (input, mean, var, weight, bias)");
        auto* input = requireTensor(args[0], "torch_batch_norm");
        auto* mean = requireTensor(args[1], "torch_batch_norm");
        auto* var = requireTensor(args[2], "torch_batch_norm");
        auto* weight = requireTensor(args[3], "torch_batch_norm");
        auto* bias = requireTensor(args[4], "torch_batch_norm");
        double eps = (args.size() > 5) ? valToDouble(args[5]) : 1e-5;
        return wrapTensor(new at::Tensor(
            at::batch_norm(*input, *weight, *bias, *mean, *var, false, 0.0, eps, false)));
    });

    reg("torch_layer_norm", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_layer_norm: requires (input, [weight], [bias], [eps])");
        auto* input = requireTensor(args[0], "torch_layer_norm");
        auto shape = input->sizes().vec();
        // Use the last dim as normalized shape
        std::vector<int64_t> normShape = {shape.back()};
        at::Tensor weight, bias;
        if (args.size() > 1 && getTensor(args[1])) {
            weight = *requireTensor(args[1], "torch_layer_norm");
        } else {
            weight = torch::ones(normShape);
        }
        if (args.size() > 2 && getTensor(args[2])) {
            bias = *requireTensor(args[2], "torch_layer_norm");
        } else {
            bias = torch::zeros(normShape);
        }
        double eps = (args.size() > 3) ? valToDouble(args[3]) : 1e-5;
        return wrapTensor(new at::Tensor(
            torch::layer_norm(*input, normShape, weight, bias, eps)));
    });

    reg("torch_conv2d", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_conv2d: requires (input, weight, [bias], [stride], [padding])");
        auto* input = requireTensor(args[0], "torch_conv2d");
        auto* weight = requireTensor(args[1], "torch_conv2d");
        at::Tensor bias;
        bool hasBias = false;
        if (args.size() > 2 && getTensor(args[2])) {
            bias = *requireTensor(args[2], "torch_conv2d");
            hasBias = true;
        }
        int64_t stride = (args.size() > 3) ? (int64_t)valToDouble(args[3]) : 1;
        int64_t padding = (args.size() > 4) ? (int64_t)valToDouble(args[4]) : 0;
        auto result = torch::conv2d(*input, *weight,
            hasBias ? bias : at::Tensor(),
            stride, padding);
        return wrapTensor(new at::Tensor(result));
    });

    reg("torch_max_pool2d", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_max_pool2d: requires (input, kernelSize, [stride])");
        auto* input = requireTensor(args[0], "torch_max_pool2d");
        int64_t kernelSize = (int64_t)valToDouble(args[1]);
        int64_t stride = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : kernelSize;
        auto result = at::max_pool2d(*input, {kernelSize, kernelSize}, {stride, stride});
        return wrapTensor(new at::Tensor(result));
    });

    reg("torch_adaptive_avg_pool2d", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_adaptive_avg_pool2d: requires (input, outputH, outputW)");
        auto* input = requireTensor(args[0], "torch_adaptive_avg_pool2d");
        int64_t oh = (int64_t)valToDouble(args[1]);
        int64_t ow = (int64_t)valToDouble(args[2]);
        auto result = torch::adaptive_avg_pool2d(*input, {oh, ow});
        return wrapTensor(new at::Tensor(result));
    });

    reg("torch_linear", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_linear: requires (input, weight, [bias])");
        auto* input = requireTensor(args[0], "torch_linear");
        auto* weight = requireTensor(args[1], "torch_linear");
        if (args.size() > 2 && getTensor(args[2])) {
            auto* bias = requireTensor(args[2], "torch_linear");
            return wrapTensor(new at::Tensor(torch::linear(*input, *weight, *bias)));
        }
        return wrapTensor(new at::Tensor(torch::linear(*input, *weight)));
    });

    reg("torch_embedding", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_embedding: requires (indices, weight)");
        auto* indices = requireTensor(args[0], "torch_embedding");
        auto* weight = requireTensor(args[1], "torch_embedding");
        return wrapTensor(new at::Tensor(torch::embedding(*weight, *indices)));
    });

    reg("torch_cross_entropy", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_cross_entropy: requires (logits, target)");
        auto* logits = requireTensor(args[0], "torch_cross_entropy");
        auto* target = requireTensor(args[1], "torch_cross_entropy");
        auto result = torch::cross_entropy_loss(*logits, *target);
        return wrapTensor(new at::Tensor(result));
    });

    reg("torch_mse_loss", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_mse_loss: requires (input, target)");
        auto* input = requireTensor(args[0], "torch_mse_loss");
        auto* target = requireTensor(args[1], "torch_mse_loss");
        auto result = torch::mse_loss(*input, *target);
        return wrapTensor(new at::Tensor(result));
    });

    reg("torch_bce_loss", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_bce_loss: requires (input, target)");
        auto* input = requireTensor(args[0], "torch_bce_loss");
        auto* target = requireTensor(args[1], "torch_bce_loss");
        auto result = torch::binary_cross_entropy(*input, *target);
        return wrapTensor(new at::Tensor(result));
    });

    reg("torch_interpolate", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_interpolate: requires (input, size, [mode='nearest'])");
        auto* input = requireTensor(args[0], "torch_interpolate");
        int64_t size = (int64_t)valToDouble(args[1]);
        std::string mode = (args.size() > 2) ? args[2].sVal : "nearest";
        auto result = torch::interpolate(*input,
            std::vector<int64_t>({size, size}), mode);
        return wrapTensor(new at::Tensor(result));
    });

    reg("torch_one_hot", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_one_hot: requires (indices, [numClasses])");
        auto* indices = requireTensor(args[0], "torch_one_hot");
        int64_t numClasses = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : (int64_t)indices->max().item<double>() + 1;
        return wrapTensor(new at::Tensor(at::one_hot(indices->to(torch::kInt64), numClasses)));
    });
}

// ============================================================================
// 7. Optimizers
// ============================================================================
void TzdPyTorch::regOptimizers(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    // Optimizers are stored as pointers to TzdOptimizer objects
    // We use TzdValue::POINTER to store them

    reg("torch_sgd", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_sgd: requires (params, lr, [momentum], [weightDecay])");
        double lr = valToDouble(args[1]);
        double momentum = (args.size() > 2) ? valToDouble(args[2]) : 0.0;
        double weightDecay = (args.size() > 3) ? valToDouble(args[3]) : 0.0;

        // Collect parameters from tensor pointers
        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& elem : args[0].arrVal) {
                auto* t = getTensor(elem);
                if (t) params.push_back(*t);
            }
        } else if (args[0].type == TzdValue::POINTER || args[0].type == TzdValue::TENSOR) {
            auto* t = getTensor(args[0]);
            if (t) params.push_back(*t);
        }

        auto* optim = new TzdOptimizer();
        optim->type = TzdOptimizer::SGD;
        optim->lr = lr; optim->momentum = momentum; optim->weight_decay = weightDecay;
        optim->params = params;
        TzdValue v;
        v.type = TzdValue::POINTER;
        v.ptrVal = optim;
        return v;
    });

    reg("torch_adam", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_adam: requires (params, lr, [beta1], [beta2], [weightDecay])");
        double lr = valToDouble(args[1]);
        double beta1 = (args.size() > 2) ? valToDouble(args[2]) : 0.9;
        double beta2 = (args.size() > 3) ? valToDouble(args[3]) : 0.999;
        double weightDecay = (args.size() > 4) ? valToDouble(args[4]) : 0.0;

        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& elem : args[0].arrVal) {
                auto* t = getTensor(elem);
                if (t) params.push_back(*t);
            }
        } else if (args[0].type == TzdValue::POINTER || args[0].type == TzdValue::TENSOR) {
            auto* t = getTensor(args[0]);
            if (t) params.push_back(*t);
        }

        auto* optim = new TzdOptimizer();
        optim->type = TzdOptimizer::ADAM;
        optim->lr = lr; optim->beta1 = beta1; optim->beta2 = beta2; optim->weight_decay = weightDecay; optim->eps = 1e-8;
        optim->params = params;
        TzdValue v;
        v.type = TzdValue::POINTER;
        v.ptrVal = optim;
        return v;
    });

    reg("torch_adamw", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_adamw: requires (params, lr, [weightDecay])");
        double lr = valToDouble(args[1]);
        double weightDecay = (args.size() > 2) ? valToDouble(args[2]) : 0.01;

        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& elem : args[0].arrVal) {
                auto* t = getTensor(elem);
                if (t) params.push_back(*t);
            }
        } else if (args[0].type == TzdValue::POINTER || args[0].type == TzdValue::TENSOR) {
            auto* t = getTensor(args[0]);
            if (t) params.push_back(*t);
        }

        auto* optim = new TzdOptimizer();
        optim->type = TzdOptimizer::ADAMW;
        optim->lr = lr; optim->beta1 = 0.9; optim->beta2 = 0.999; optim->weight_decay = weightDecay; optim->eps = 1e-8;
        optim->params = params;
        TzdValue v;
        v.type = TzdValue::POINTER;
        v.ptrVal = optim;
        return v;
    });

    reg("torch_optim_step", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_optim_step: requires (optimizer)");
        auto* optim = reinterpret_cast<TzdOptimizer*>(args[0].ptrVal);
        if (!optim) return TzdValue::Error("torch_optim_step: invalid optimizer");
        optim->step();
        return TzdValue();
    });

    reg("torch_optim_zero_grad", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_optim_zero_grad: requires (optimizer)");
        auto* optim = reinterpret_cast<TzdOptimizer*>(args[0].ptrVal);
        if (!optim) return TzdValue::Error("torch_optim_zero_grad: invalid optimizer");
        optim->zero_grad();
        return TzdValue();
    });

    reg("torch_optim_delete", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        auto* optim = reinterpret_cast<TzdOptimizer*>(args[0].ptrVal);
        if (optim) delete optim;
        return TzdValue();
    });
}

// ============================================================================
// 8. Device & Memory Management
// ============================================================================
void TzdPyTorch::regDeviceMgmt(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_cuda_is_available", [](auto args) -> TzdValue {
        return TzdValue(TzdPyTorch::isCudaAvailable());
    });

    reg("torch_device_count", [](auto args) -> TzdValue {
        if (TzdPyTorch::isCudaAvailable()) {
            return TzdValue((double)torch::cuda::device_count());
        }
        return TzdValue(0.0);
    });

    reg("torch_current_device", [](auto args) -> TzdValue {
        if (TzdPyTorch::isCudaAvailable()) {
            return TzdValue((double)torch::cuda::current_device());
        }
        return TzdValue(-1.0);
    });

    reg("torch_set_device", [](auto args) -> TzdValue {
        if (!TzdPyTorch::isCudaAvailable()) {
            return TzdValue::Error("CUDA is not available in this build");
        }
        if (args.empty()) return TzdValue::Error("torch_set_device: requires (deviceId)");
        int64_t device = (int64_t)valToDouble(args[0]);
        torch::cuda::set_device(device);
        return TzdValue();
    });

    reg("torch_to_device", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_to_device: requires (tensor, deviceStr)");
        auto* a = requireTensor(args[0], "torch_to_device");
        std::string devStr = args[1].sVal;
        if (devStr == "cpu") {
            return wrapTensor(new at::Tensor(a->to(torch::kCPU)));
        }
        if (devStr == "cuda" || devStr.substr(0, 4) == "cuda") {
            if (!TzdPyTorch::isCudaAvailable()) {
                return TzdValue::Error("CUDA is not available in this build");
            }
            int dev = 0;
            if (devStr.size() > 5) dev = std::stoi(devStr.substr(5));
            return wrapTensor(new at::Tensor(a->to(torch::Device(torch::kCUDA, dev))));
        }
        return TzdValue::Error("Unknown device: " + devStr);
    });

    reg("torch_to_cpu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_to_cpu: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_to_cpu");
        return wrapTensor(new at::Tensor(a->to(torch::kCPU)));
    });

    reg("torch_to_cuda", [](auto args) -> TzdValue {
        if (!TzdPyTorch::isCudaAvailable()) {
            return TzdValue::Error("CUDA is not available in this build");
        }
        if (args.empty()) return TzdValue::Error("torch_to_cuda: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_to_cuda");
        return wrapTensor(new at::Tensor(a->to(torch::kCUDA)));
    });

    // Memory management 鈥?zero-leak tracking
    reg("torch_memory_allocated", [](auto args) -> TzdValue {
        return TzdValue((double)TzdPyTorch::totalMemoryBytes());
    });

    reg("torch_memory_allocated_str", [](auto args) -> TzdValue {
        size_t bytes = TzdPyTorch::totalMemoryBytes();
        std::string unit = "B";
        double val = (double)bytes;
        if (val >= 1024 * 1024 * 1024) { val /= (1024 * 1024 * 1024); unit = "GB"; }
        else if (val >= 1024 * 1024) { val /= (1024 * 1024); unit = "MB"; }
        else if (val >= 1024) { val /= 1024; unit = "KB"; }
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << val << " " << unit;
        return TzdValue(ss.str());
    });

    reg("torch_num_tensors", [](auto args) -> TzdValue {
        return TzdValue((double)TzdPyTorch::numTensors());
    });

    reg("torch_release_tensor", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        auto* t = getTensor(args[0]);
        if (t) TzdPyTorch::releaseTensor(t);
        return TzdValue();
    });

    reg("torch_release_all", [](auto args) -> TzdValue {
        TzdPyTorch::releaseAll();
        return TzdValue();
    });

    reg("torch_empty_cache", [](auto args) -> TzdValue {
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            torch::cuda::empty_cache();
        }
#endif
        return TzdValue();
    });

    reg("torch_set_num_threads", [](auto args) -> TzdValue {
        if (!args.empty()) {
            at::set_num_threads((int)valToDouble(args[0]));
        }
        return TzdValue();
    });

    reg("torch_get_num_threads", [](auto args) -> TzdValue {
        return TzdValue((double)at::get_num_threads());
    });
}

// ============================================================================
// 9. Autograd
// ============================================================================
void TzdPyTorch::regAutograd(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_requires_grad", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_requires_grad: requires (tensor, [requires])");
        auto* a = requireTensor(args[0], "torch_requires_grad");
        bool req = (args.size() > 1) ? (bool)valToDouble(args[1]) : true;
        a->requires_grad_(req);
        return args[0];
    });

    reg("torch_is_requires_grad", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = requireTensor(args[0], "torch_is_requires_grad");
        return TzdValue(a->requires_grad());
    });

    reg("torch_backward", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_backward: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_backward");
        a->backward();
        return TzdValue();
    });

    reg("torch_grad", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_grad: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_grad");
        return wrapTensor(new at::Tensor(a->grad()));
    });

    reg("torch_no_grad", [](auto args) -> TzdValue {
        // Disable gradient computation (use torch_set_grad_enabled(true) to re-enable)
        c10::GradMode::set_enabled(false);
        return TzdValue();
    });

    reg("torch_set_grad_enabled", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        bool enabled = (bool)valToDouble(args[0]);
        c10::GradMode::set_enabled(enabled);
        return TzdValue();
    });

    reg("torch_is_grad_enabled", [](auto args) -> TzdValue {
        return TzdValue(c10::GradMode::is_enabled());
    });

    reg("torch_detach", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_detach: requires 1 tensor arg");
        auto* a = requireTensor(args[0], "torch_detach");
        return wrapTensor(new at::Tensor(a->detach()));
    });

    reg("torch_item", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = requireTensor(args[0], "torch_item");
        if (args.size() == 1) return TzdValue(a->item<double>());
        // With index: direct data pointer access (avoids flatten/MKL)
        int64_t idx = (int64_t)valToDouble(args[1]);
        int64_t numel = a->numel();
        if (numel == 0) return TzdValue(0.0);
        idx = ((idx % numel) + numel) % numel;
        // Direct data access for contiguous tensors
        if (!a->is_contiguous()) {
            auto c = a->contiguous();
            if (c.scalar_type() == torch::kFloat64) return TzdValue(c.data_ptr<double>()[idx]);
            if (c.scalar_type() == torch::kFloat32) return TzdValue((double)c.data_ptr<float>()[idx]);
            if (c.scalar_type() == torch::kFloat16) return TzdValue((double)c.data_ptr<at::Half>()[idx]);
            if (c.scalar_type() == torch::kBFloat16) return TzdValue((double)c.data_ptr<at::BFloat16>()[idx]);
            if (c.scalar_type() == torch::kInt64) return TzdValue((double)c.data_ptr<int64_t>()[idx]);
            if (c.scalar_type() == torch::kInt32) return TzdValue((double)c.data_ptr<int32_t>()[idx]);
            if (c.scalar_type() == torch::kInt16) return TzdValue((double)c.data_ptr<int16_t>()[idx]);
            if (c.scalar_type() == torch::kInt8) return TzdValue((double)c.data_ptr<int8_t>()[idx]);
            if (c.scalar_type() == torch::kUInt8) return TzdValue((double)c.data_ptr<uint8_t>()[idx]);
            if (c.scalar_type() == torch::kBool) return TzdValue((double)(int)c.data_ptr<bool>()[idx]);
            return TzdValue(c.view({-1})[idx].item<double>());
        }
        if (a->scalar_type() == torch::kFloat64) return TzdValue(a->data_ptr<double>()[idx]);
        if (a->scalar_type() == torch::kFloat32) return TzdValue((double)a->data_ptr<float>()[idx]);
        if (a->scalar_type() == torch::kFloat16) return TzdValue((double)a->data_ptr<at::Half>()[idx]);
        if (a->scalar_type() == torch::kBFloat16) return TzdValue((double)a->data_ptr<at::BFloat16>()[idx]);
        if (a->scalar_type() == torch::kInt64) return TzdValue((double)a->data_ptr<int64_t>()[idx]);
        if (a->scalar_type() == torch::kInt32) return TzdValue((double)a->data_ptr<int32_t>()[idx]);
        if (a->scalar_type() == torch::kInt16) return TzdValue((double)a->data_ptr<int16_t>()[idx]);
        if (a->scalar_type() == torch::kInt8) return TzdValue((double)a->data_ptr<int8_t>()[idx]);
        if (a->scalar_type() == torch::kUInt8) return TzdValue((double)a->data_ptr<uint8_t>()[idx]);
        if (a->scalar_type() == torch::kBool) return TzdValue((double)(int)a->data_ptr<bool>()[idx]);
        return TzdValue(a->view({-1})[idx].item<double>());
    });
}

// ============================================================================
// 10. Serialization
// ============================================================================
void TzdPyTorch::regSerialization(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_save", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_save: requires (tensor, path)");
        auto* a = requireTensor(args[0], "torch_save");
        std::string path = args[1].sVal;
        try {
            std::ofstream file(path, std::ios::binary);
            if (!file) return TzdValue::Error("Cannot open file: " + path);
            at::Tensor t = a->contiguous();
            int64_t dtype = (int64_t)t.scalar_type();
            file.write(reinterpret_cast<const char*>(&dtype), sizeof(dtype));
            int64_t ndim = t.dim();
            file.write(reinterpret_cast<const char*>(&ndim), sizeof(ndim));
            for (int64_t i = 0; i < ndim; i++) {
                int64_t s = t.size(i);
                file.write(reinterpret_cast<const char*>(&s), sizeof(s));
            }
            uint64_t dataLen = (uint64_t)t.nbytes();
            file.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));
            if (dataLen > 0) file.write(reinterpret_cast<const char*>(t.data_ptr()), dataLen);
            return TzdValue(true);
        } catch (const std::exception& e) {
            return TzdValue::Error(std::string("torch_save: ") + e.what());
        }
    });

    reg("torch_load", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_load: requires (path)");
        std::string path = args[0].sVal;
        auto* t = new at::Tensor();
        try {
            std::ifstream file(path, std::ios::binary);
            if (!file) { delete t; return TzdValue::Error("Cannot open file: " + path); }
            // Manual load: read shape, dtype, data
            int64_t dtype, ndim;
            file.read(reinterpret_cast<char*>(&dtype), sizeof(dtype));
            file.read(reinterpret_cast<char*>(&ndim), sizeof(ndim));
            std::vector<int64_t> shape(ndim);
            for (int64_t i = 0; i < ndim; i++) file.read(reinterpret_cast<char*>(&shape[i]), sizeof(int64_t));
            uint64_t dataLen = 0;
            file.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));
            std::vector<char> rawData(dataLen);
            if (dataLen > 0) file.read(rawData.data(), dataLen);
            *t = at::from_blob(rawData.data(), shape, at::TensorOptions().dtype((at::ScalarType)dtype)).clone();
            return wrapTensor(t);
        } catch (const std::exception& e) {
            delete t;
            return TzdValue::Error(std::string("torch_load: ") + e.what());
        }
    });

    reg("torch_jit_save", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_jit_save: requires (module, path)");
        auto* mod = reinterpret_cast<torch::jit::Module*>(args[0].ptrVal);
        if (!mod) return TzdValue::Error("torch_jit_save: invalid module");
        try {
            mod->save(args[1].sVal);
            return TzdValue(true);
        } catch (const std::exception& e) {
            return TzdValue::Error(std::string("torch_jit_save: ") + e.what());
        }
    });

    reg("torch_jit_load", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_jit_load: requires (path)");
        try {
            auto* mod = new torch::jit::Module(torch::jit::load(args[0].sVal));
            TzdValue v;
            v.type = TzdValue::POINTER;
            v.ptrVal = mod;
            return v;
        } catch (const std::exception& e) {
            return TzdValue::Error(std::string("torch_jit_load: ") + e.what());
        }
    });

    reg("torch_jit_eval", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_jit_eval: requires (module)");
        auto* mod = reinterpret_cast<torch::jit::Module*>(args[0].ptrVal);
        if (!mod) return TzdValue::Error("torch_jit_eval: invalid module");
        mod->eval();
        return args[0];
    });

    reg("torch_jit_train", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_jit_train: requires (module)");
        auto* mod = reinterpret_cast<torch::jit::Module*>(args[0].ptrVal);
        if (!mod) return TzdValue::Error("torch_jit_train: invalid module");
        mod->train();
        return args[0];
    });
}

// ============================================================================
// 11. Conversion Functions
// ============================================================================
void TzdPyTorch::regConversion(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    reg("torch_to_array", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        auto* a = requireTensor(args[0], "torch_to_array");
        return tensorToArray(*a);
    });

    reg("torch_from_array", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_from_array: requires (array)");
        return wrapTensor(new at::Tensor(arrayToTensor(args[0])));
    });

    reg("torch_scalar_value", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = requireTensor(args[0], "torch_scalar_value");
        if (a->numel() != 1) return TzdValue::Error("torch_scalar_value: tensor must have exactly one element");
        return TzdValue(a->item<double>());
    });

    reg("torch_version", [](auto args) -> TzdValue {
        return TzdValue("libtorch 2.3.1 (CPU)");
    });

    reg("torch_is_tensor", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        // Check for explicit TENSOR type tag
        if (args[0].type == TzdValue::TENSOR) return TzdValue(true);
        // Fall back to POINTER type check
        return TzdValue(getTensor(args[0]) != nullptr);
    });

    reg("torch_allclose", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_allclose: requires (a, b, [rtol], [atol])");
        auto* a = requireTensor(args[0], "torch_allclose");
        auto* b = requireTensor(args[1], "torch_allclose");
        double rtol = (args.size() > 2) ? valToDouble(args[2]) : 1e-5;
        double atol = (args.size() > 3) ? valToDouble(args[3]) : 1e-8;
        return TzdValue(torch::allclose(*a, *b, rtol, atol));
    });

    reg("torch_equal", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_equal: requires 2 tensor args");
        auto* a = requireTensor(args[0], "torch_equal");
        auto* b = requireTensor(args[1], "torch_equal");
        return TzdValue(torch::equal(*a, *b));
    });
}

// ============================================================================
// numTensors implementation
// ============================================================================
size_t TzdPyTorch::numTensors() {
    return s_liveTensorCount.load(std::memory_order_relaxed);
}

// ============================================================================
// 12. Extended Operations 鈥?complete PyTorch function library
// Adds: comparison ops, more activations, more losses, more optimizers,
// gradient clipping, manual seed, more creation/shape ops, in-place ops,
// CUDA memory stats, LR schedulers, and training utilities.
// ============================================================================
void TzdPyTorch::regExtendedOps(TzdInterpreter* interp) {
    auto reg = [&](std::string name, TzdValue::NativeFuncType f) {
        TzdValue v(f); v.name = name; interp->setGlobalVariable(name, v);
    };

    // ---- Comparison operations (element-wise, return bool tensors) ----
    reg("torch_eq", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_eq: requires 2 tensors");
        return wrapTensor(new at::Tensor(*requireTensor(args[0], "torch_eq") == *requireTensor(args[1], "torch_eq")));
    });
    reg("torch_ne", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_ne: requires 2 tensors");
        return wrapTensor(new at::Tensor(*requireTensor(args[0], "torch_ne") != *requireTensor(args[1], "torch_ne")));
    });
    reg("torch_lt", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_lt: requires 2 tensors");
        return wrapTensor(new at::Tensor(*requireTensor(args[0], "torch_lt") < *requireTensor(args[1], "torch_lt")));
    });
    reg("torch_le", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_le: requires 2 tensors");
        return wrapTensor(new at::Tensor(*requireTensor(args[0], "torch_le") <= *requireTensor(args[1], "torch_le")));
    });
    reg("torch_gt", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_gt: requires 2 tensors");
        return wrapTensor(new at::Tensor(*requireTensor(args[0], "torch_gt") > *requireTensor(args[1], "torch_gt")));
    });
    reg("torch_ge", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_ge: requires 2 tensors");
        return wrapTensor(new at::Tensor(*requireTensor(args[0], "torch_ge") >= *requireTensor(args[1], "torch_ge")));
    });
    reg("torch_logical_and", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_logical_and: requires 2 tensors");
        auto* a = requireTensor(args[0], "torch_logical_and");
        auto* b = requireTensor(args[1], "torch_logical_and");
        return wrapTensor(new at::Tensor(a->to(torch::kBool).logical_and(b->to(torch::kBool))));
    });
    reg("torch_logical_or", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_logical_or: requires 2 tensors");
        auto* a = requireTensor(args[0], "torch_logical_or");
        auto* b = requireTensor(args[1], "torch_logical_or");
        return wrapTensor(new at::Tensor(a->to(torch::kBool).logical_or(b->to(torch::kBool))));
    });
    reg("torch_logical_not", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_logical_not: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_logical_not");
        return wrapTensor(new at::Tensor(a->to(torch::kBool).logical_not()));
    });
    reg("torch_masked_fill", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_masked_fill: requires (tensor, mask, value)");
        auto* a = requireTensor(args[0], "torch_masked_fill");
        auto* mask = requireTensor(args[1], "torch_masked_fill");
        double val = valToDouble(args[2]);
        return wrapTensor(new at::Tensor(a->masked_fill(mask->to(torch::kBool), val)));
    });

    // ---- Additional activation functions ----
    reg("torch_silu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_silu: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::silu(*requireTensor(args[0], "torch_silu"))));
    });
    reg("torch_selu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_selu: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::selu(*requireTensor(args[0], "torch_selu"))));
    });
    reg("torch_hardswish", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_hardswish: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::hardswish(*requireTensor(args[0], "torch_hardswish"))));
    });
    reg("torch_softplus", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_softplus: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::softplus(*requireTensor(args[0], "torch_softplus"))));
    });
    reg("torch_softmin", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_softmin: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_softmin");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : -1;
        return wrapTensor(new at::Tensor(torch::softmin(*a, dim)));
    });
    reg("torch_glu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_glu: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_glu");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : -1;
        return wrapTensor(new at::Tensor(torch::glu(*a, dim)));
    });
    reg("torch_mish", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_mish: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_mish");
        return wrapTensor(new at::Tensor(*a * torch::tanh(torch::softplus(*a))));
    });
    reg("torch_prelu", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_prelu: requires (input, weight)");
        auto* a = requireTensor(args[0], "torch_prelu");
        auto* w = requireTensor(args[1], "torch_prelu");
        return wrapTensor(new at::Tensor(torch::prelu(*a, *w)));
    });
    reg("torch_hardtanh", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_hardtanh: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_hardtanh");
        double minVal = (args.size() > 1) ? valToDouble(args[1]) : -1.0;
        double maxVal = (args.size() > 2) ? valToDouble(args[2]) : 1.0;
        return wrapTensor(new at::Tensor(torch::hardtanh(*a, minVal, maxVal)));
    });
    reg("torch_threshold", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_threshold: requires (input, threshold, value)");
        auto* a = requireTensor(args[0], "torch_threshold");
        double thr = valToDouble(args[1]);
        double val = valToDouble(args[2]);
        return wrapTensor(new at::Tensor(torch::threshold(*a, thr, val)));
    });

    // ---- Additional loss functions ----
    reg("torch_nll_loss", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_nll_loss: requires (logits, target)");
        auto* a = requireTensor(args[0], "torch_nll_loss");
        auto* t = requireTensor(args[1], "torch_nll_loss");
        auto loss = torch::nll_loss(*a, *t);
        return wrapTensor(new at::Tensor(loss));
    });
    reg("torch_l1_loss", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_l1_loss: requires (input, target)");
        auto* a = requireTensor(args[0], "torch_l1_loss");
        auto* b = requireTensor(args[1], "torch_l1_loss");
        auto loss = torch::l1_loss(*a, *b);
        return wrapTensor(new at::Tensor(loss));
    });
    reg("torch_smooth_l1_loss", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_smooth_l1_loss: requires (input, target)");
        auto* a = requireTensor(args[0], "torch_smooth_l1_loss");
        auto* b = requireTensor(args[1], "torch_smooth_l1_loss");
        auto loss = torch::smooth_l1_loss(*a, *b);
        return wrapTensor(new at::Tensor(loss));
    });
    reg("torch_kl_div", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_kl_div: requires (input, target)");
        auto* a = requireTensor(args[0], "torch_kl_div");
        auto* b = requireTensor(args[1], "torch_kl_div");
        auto loss = torch::kl_div(*a, *b);
        return wrapTensor(new at::Tensor(loss));
    });
    reg("torch_cosine_similarity", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_cosine_similarity: requires (a, b)");
        auto* a = requireTensor(args[0], "torch_cosine_similarity");
        auto* b = requireTensor(args[1], "torch_cosine_similarity");
        int64_t dim = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : 1;
        double eps = (args.size() > 3) ? valToDouble(args[3]) : 1e-8;
        return wrapTensor(new at::Tensor(torch::cosine_similarity(*a, *b, dim, eps)));
    });
    reg("torch_pairwise_distance", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_pairwise_distance: requires (a, b)");
        auto* a = requireTensor(args[0], "torch_pairwise_distance");
        auto* b = requireTensor(args[1], "torch_pairwise_distance");
        double p = (args.size() > 2) ? valToDouble(args[2]) : 2.0;
        double eps = (args.size() > 3) ? valToDouble(args[3]) : 1e-6;
        return wrapTensor(new at::Tensor(torch::pairwise_distance(*a, *b, p, eps)));
    });
    reg("torch_triple_margin_loss", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_triple_margin_loss: requires (anchor, positive, negative)");
        auto* a = requireTensor(args[0], "torch_triple_margin_loss");
        auto* p = requireTensor(args[1], "torch_triple_margin_loss");
        auto* n = requireTensor(args[2], "torch_triple_margin_loss");
        double margin = (args.size() > 3) ? valToDouble(args[3]) : 1.0;
        auto loss = torch::triplet_margin_loss(*a, *p, *n, margin);
        return wrapTensor(new at::Tensor(loss));
    });

    // ---- Additional optimizers ----
    reg("torch_rmsprop", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_rmsprop: requires (params, lr)");
        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& p : args[0].arrVal) {
                auto* t = getTensor(p);
                if (t) params.push_back(*t);
            }
        }
        double lr = valToDouble(args[1]);
        double alpha = (args.size() > 2) ? valToDouble(args[2]) : 0.99;
        double eps = (args.size() > 3) ? valToDouble(args[3]) : 1e-8;
        double weightDecay = (args.size() > 4) ? valToDouble(args[4]) : 0.0;
        double momentum = (args.size() > 5) ? valToDouble(args[5]) : 0.0;
        auto* optim = new TzdOptimizer();
        optim->type = TzdOptimizer::RMSPROP;
        optim->lr = lr; optim->alpha = alpha; optim->eps = eps; optim->weight_decay = 0.0; optim->momentum = momentum;
        optim->params = params;
        TzdValue v; v.type = TzdValue::POINTER; v.ptrVal = optim; return v;
    });
    reg("torch_adagrad", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_adagrad: requires (params, lr)");
        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& p : args[0].arrVal) {
                auto* t = getTensor(p);
                if (t) params.push_back(*t);
            }
        }
        double lr = valToDouble(args[1]);
        double weightDecay = (args.size() > 2) ? valToDouble(args[2]) : 0.0;
        auto* optim = new TzdOptimizer();
        optim->type = TzdOptimizer::ADAGRAD;
        optim->lr = lr; optim->weight_decay = weightDecay; optim->eps = 1e-10;
        optim->params = params;
        TzdValue v; v.type = TzdValue::POINTER; v.ptrVal = optim; return v;
    });
    reg("torch_adamax", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_adamax: requires (params, lr)");
        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& p : args[0].arrVal) {
                auto* t = getTensor(p);
                if (t) params.push_back(*t);
            }
        }
        double lr = valToDouble(args[1]);
        double beta1 = (args.size() > 2) ? valToDouble(args[2]) : 0.9;
        double beta2 = (args.size() > 3) ? valToDouble(args[3]) : 0.999;
        double weightDecay = (args.size() > 4) ? valToDouble(args[4]) : 0.0;
        auto* optim = new TzdOptimizer();
        optim->type = TzdOptimizer::ADAMAX;
        optim->lr = lr; optim->beta1 = beta1; optim->beta2 = beta2; optim->weight_decay = weightDecay; optim->eps = 1e-8;
        optim->params = params;
        TzdValue v; v.type = TzdValue::POINTER; v.ptrVal = optim; return v;
    });
    reg("torch_nadam", [](auto args) -> TzdValue {
        // NAdam via Adam with Nesterov momentum
        if (args.size() < 2) return TzdValue::Error("torch_nadam: requires (params, lr)");
        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& p : args[0].arrVal) {
                auto* t = getTensor(p);
                if (t) params.push_back(*t);
            }
        }
        double lr = valToDouble(args[1]);
        double beta1 = (args.size() > 2) ? valToDouble(args[2]) : 0.9;
        double beta2 = (args.size() > 3) ? valToDouble(args[3]) : 0.999;
        double weightDecay = (args.size() > 4) ? valToDouble(args[4]) : 0.0;
        auto* optim = new TzdOptimizer();
        optim->type = TzdOptimizer::ADAM;
        optim->lr = lr; optim->beta1 = beta1; optim->beta2 = beta2; optim->weight_decay = weightDecay; optim->eps = 1e-8;
        optim->params = params;
        // Note: libtorch doesn't have a dedicated NAdam; use Adam as closest approximation
        TzdValue v; v.type = TzdValue::POINTER; v.ptrVal = optim; return v;
    });

    // ---- Gradient clipping ----
    reg("torch_clip_grad_norm", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_clip_grad_norm: requires (params, maxNorm)");
        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& p : args[0].arrVal) {
                auto* t = getTensor(p);
                if (t) params.push_back(*t);
            }
        }
        double maxNorm = valToDouble(args[1]);
        double totalNorm = 0.0;
        for (auto& p : params) {
            if (p.grad().defined()) totalNorm += p.grad().norm().item<double>() * p.grad().norm().item<double>();
        }
        totalNorm = std::sqrt(totalNorm);
        double clipCoef = maxNorm / (totalNorm + 1e-6);
        if (clipCoef < 1.0) {
            for (auto& p : params) {
                if (p.grad().defined()) p.grad().mul_(clipCoef);
            }
        }
        return TzdValue(totalNorm);
    });
    reg("torch_clip_grad_value", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_clip_grad_value: requires (params, clipValue)");
        std::vector<at::Tensor> params;
        if (args[0].type == TzdValue::ARRAY) {
            for (const auto& p : args[0].arrVal) {
                auto* t = getTensor(p);
                if (t) params.push_back(*t);
            }
        }
        double clipVal = valToDouble(args[1]);
        for (auto& p : params) {
            if (p.grad().defined()) {
                p.grad().clamp_(-clipVal, clipVal);
            }
        }
        return TzdValue();
    });

    // ---- Manual seed and RNG ----
    reg("torch_manual_seed", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue();
        uint64_t seed = (uint64_t)valToDouble(args[0]);
        torch::manual_seed(seed);
        return TzdValue();
    });

    // ---- Additional tensor creation ----
    reg("torch_randperm", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_randperm: requires (n)");
        int64_t n = (int64_t)valToDouble(args[0]);
        return wrapTensor(new at::Tensor(torch::randperm(n)));
    });
    reg("torch_zeros_like", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_zeros_like: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_zeros_like");
        return wrapTensor(new at::Tensor(torch::zeros_like(*a)));
    });
    reg("torch_ones_like", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_ones_like: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_ones_like");
        return wrapTensor(new at::Tensor(torch::ones_like(*a)));
    });
    reg("torch_full_like", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_full_like: requires (tensor, fillValue)");
        auto* a = requireTensor(args[0], "torch_full_like");
        double fillVal = valToDouble(args[1]);
        return wrapTensor(new at::Tensor(torch::full_like(*a, fillVal)));
    });
    reg("torch_empty_like", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_empty_like: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_empty_like");
        return wrapTensor(new at::Tensor(torch::empty_like(*a)));
    });
    reg("torch_randint_like", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_randint_like: requires (tensor, low, high)");
        auto* a = requireTensor(args[0], "torch_randint_like");
        int64_t low = (int64_t)valToDouble(args[1]);
        int64_t high = (int64_t)valToDouble(args[2]);
        return wrapTensor(new at::Tensor(torch::randint_like(*a, low, high)));
    });

    // ---- In-place operations ----
    reg("torch_add_", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_add_: requires (a, b)");
        auto* a = requireTensor(args[0], "torch_add_");
        auto* b = requireTensor(args[1], "torch_add_");
        *a += *b;
        return args[0];
    });
    reg("torch_sub_", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_sub_: requires (a, b)");
        auto* a = requireTensor(args[0], "torch_sub_");
        auto* b = requireTensor(args[1], "torch_sub_");
        *a -= *b;
        return args[0];
    });
    reg("torch_mul_", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_mul_: requires (a, b)");
        auto* a = requireTensor(args[0], "torch_mul_");
        auto* b = requireTensor(args[1], "torch_mul_");
        *a *= *b;
        return args[0];
    });
    reg("torch_div_", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_div_: requires (a, b)");
        auto* a = requireTensor(args[0], "torch_div_");
        auto* b = requireTensor(args[1], "torch_div_");
        *a /= *b;
        return args[0];
    });
    reg("torch_zero_", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_zero_: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_zero_");
        a->zero_();
        return args[0];
    });
    reg("torch_fill_", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_fill_: requires (tensor, value)");
        auto* a = requireTensor(args[0], "torch_fill_");
        double val = valToDouble(args[1]);
        a->fill_(val);
        return args[0];
    });
    reg("torch_masked_fill_", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_masked_fill_: requires (tensor, mask, value)");
        auto* a = requireTensor(args[0], "torch_masked_fill_");
        auto* mask = requireTensor(args[1], "torch_masked_fill_");
        double val = valToDouble(args[2]);
        a->masked_fill_(mask->to(torch::kBool), val);
        return args[0];
    });
    reg("torch_clamp_", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_clamp_: requires (tensor, [min], [max])");
        auto* a = requireTensor(args[0], "torch_clamp_");
        double minVal = (args.size() > 1) ? valToDouble(args[1]) : -std::numeric_limits<double>::infinity();
        double maxVal = (args.size() > 2) ? valToDouble(args[2]) : std::numeric_limits<double>::infinity();
        a->clamp_(minVal, maxVal);
        return args[0];
    });
    reg("torch_copy_", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_copy_: requires (dest, src)");
        auto* a = requireTensor(args[0], "torch_copy_");
        auto* b = requireTensor(args[1], "torch_copy_");
        a->copy_(*b);
        return args[0];
    });

    // ---- Matrix and shape operations ----
    reg("torch_triu", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_triu: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_triu");
        int64_t diagonal = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        return wrapTensor(new at::Tensor(torch::triu(*a, diagonal)));
    });
    reg("torch_tril", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_tril: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_tril");
        int64_t diagonal = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        return wrapTensor(new at::Tensor(torch::tril(*a, diagonal)));
    });
    reg("torch_diag", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_diag: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_diag");
        int64_t diagonal = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        return wrapTensor(new at::Tensor(torch::diag(*a, diagonal)));
    });
    reg("torch_diagflat", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_diagflat: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_diagflat");
        return wrapTensor(new at::Tensor(torch::diagflat(*a)));
    });
    reg("torch_sort_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_sort_t: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_sort_t");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : -1;
        bool descending = (args.size() > 2) ? (bool)(int)valToDouble(args[2]) : false;
        auto [values, indices] = a->sort(dim, descending);
        return wrapTensor(new at::Tensor(values));
    });
    reg("torch_topk", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_topk: requires (tensor, k)");
        auto* a = requireTensor(args[0], "torch_topk");
        int64_t k = (int64_t)valToDouble(args[1]);
        int64_t dim = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : -1;
        auto [values, indices] = a->topk(k, dim);
        return wrapTensor(new at::Tensor(values));
    });
    reg("torch_argsort", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_argsort: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_argsort");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : -1;
        return wrapTensor(new at::Tensor(a->argsort(dim)));
    });
    reg("torch_unique", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_unique: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_unique");
        auto uq = at::_unique2(*a); auto output = std::get<0>(uq);
        return wrapTensor(new at::Tensor(output));
    });

    // ---- Additional math operations ----
    reg("torch_fmod", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_fmod: requires 2 tensors");
        return wrapTensor(new at::Tensor(torch::fmod(*requireTensor(args[0], "torch_fmod"), *requireTensor(args[1], "torch_fmod"))));
    });
    reg("torch_remainder", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_remainder: requires 2 tensors");
        return wrapTensor(new at::Tensor(torch::remainder(*requireTensor(args[0], "torch_remainder"), *requireTensor(args[1], "torch_remainder"))));
    });
    reg("torch_erf", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_erf: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::erf(*requireTensor(args[0], "torch_erf"))));
    });
    reg("torch_erfc", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_erfc: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::erfc(*requireTensor(args[0], "torch_erfc"))));
    });
    reg("torch_lgamma", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_lgamma: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::lgamma(*requireTensor(args[0], "torch_lgamma"))));
    });
    reg("torch_digamma", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_digamma: requires 1 tensor");
        return wrapTensor(new at::Tensor(torch::digamma(*requireTensor(args[0], "torch_digamma"))));
    });
    reg("torch_atan2_t", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_atan2_t: requires 2 tensors");
        return wrapTensor(new at::Tensor(torch::atan2(*requireTensor(args[0], "torch_atan2_t"), *requireTensor(args[1], "torch_atan2_t"))));
    });
    reg("torch_histc", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_histc: requires (tensor, [bins])");
        auto* a = requireTensor(args[0], "torch_histc");
        int64_t bins = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 100;
        return wrapTensor(new at::Tensor(torch::histc(*a, bins)));
    });
    reg("torch_bincount", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_bincount: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_bincount");
        return wrapTensor(new at::Tensor(torch::bincount(*a)));
    });

    // ---- Tensor type queries ----
    reg("torch_is_floating_point", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->is_floating_point());
    });
    reg("torch_is_integer", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->scalar_type() == torch::kInt8 || a->scalar_type() == torch::kInt16 ||
                        a->scalar_type() == torch::kInt32 || a->scalar_type() == torch::kInt64 ||
                        a->scalar_type() == torch::kUInt8);
    });
    reg("torch_numel", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(0.0);
        return TzdValue((double)a->numel());
    });

    // ---- Additional conv and pooling ----
    reg("torch_conv1d", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_conv1d: requires (input, weight)");
        auto* input = requireTensor(args[0], "torch_conv1d");
        auto* weight = requireTensor(args[1], "torch_conv1d");
        int64_t stride = (args.size() > 3) ? (int64_t)valToDouble(args[3]) : 1;
        int64_t padding = (args.size() > 4) ? (int64_t)valToDouble(args[4]) : 0;
        at::Tensor bias;
        if (args.size() > 2 && getTensor(args[2])) bias = *getTensor(args[2]);
        auto result = torch::conv1d(*input, *weight, bias, stride, padding);
        return wrapTensor(new at::Tensor(result));
    });
    reg("torch_conv_transpose2d", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_conv_transpose2d: requires (input, weight)");
        auto* input = requireTensor(args[0], "torch_conv_transpose2d");
        auto* weight = requireTensor(args[1], "torch_conv_transpose2d");
        int64_t stride = (args.size() > 3) ? (int64_t)valToDouble(args[3]) : 1;
        int64_t padding = (args.size() > 4) ? (int64_t)valToDouble(args[4]) : 0;
        int64_t output_padding = (args.size() > 5) ? (int64_t)valToDouble(args[5]) : 0;
        at::Tensor bias;
        if (args.size() > 2 && getTensor(args[2])) bias = *getTensor(args[2]);
        auto result = torch::conv_transpose2d(*input, *weight, bias, stride, padding, output_padding);
        return wrapTensor(new at::Tensor(result));
    });
    reg("torch_avg_pool2d", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_avg_pool2d: requires (input, kernelSize)");
        auto* input = requireTensor(args[0], "torch_avg_pool2d");
        int64_t kH = (int64_t)valToDouble(args[1]);
        int64_t kW = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : kH;
        int64_t stride = (args.size() > 3) ? (int64_t)valToDouble(args[3]) : kH;
        auto result = torch::avg_pool2d(*input, {kH, kW}, {stride, stride});
        return wrapTensor(new at::Tensor(result));
    });
    reg("torch_adaptive_avg_pool1d", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_adaptive_avg_pool1d: requires (input, outputSize)");
        auto* input = requireTensor(args[0], "torch_adaptive_avg_pool1d");
        int64_t outSize = (int64_t)valToDouble(args[1]);
        auto result = torch::adaptive_avg_pool1d(*input, {outSize});
        return wrapTensor(new at::Tensor(result));
    });

    // ---- Autograd extensions ----
    reg("torch_grad_fn", [](auto args) -> TzdValue {
        // Compute gradients for multiple tensors
        if (args.empty()) return TzdValue::Error("torch_grad_fn: requires at least 1 tensor");
        std::vector<at::Tensor> inputs;
        std::vector<at::Tensor> outputs;
        for (const auto& a : args) {
            auto* t = getTensor(a);
            if (t) inputs.push_back(*t);
        }
        if (inputs.empty()) return TzdValue::Error("torch_grad_fn: no valid tensors");
        // Compute vector-Jacobian product
        inputs[0].backward(); auto grads = std::vector<at::Tensor>(); for (auto& inp : inputs) { grads.push_back(inp.grad()); }
        if (grads.empty() || !grads[0].defined()) return TzdValue();
        return wrapTensor(new at::Tensor(grads[0]));
    });
    reg("torch_is_leaf", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->is_leaf());
    });

    // ---- CUDA / VRAM management ----
    reg("torch_cuda_memory_allocated", [](auto args) -> TzdValue {
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            return TzdValue((double)torch::cuda::memory_allocated());
        }
#endif
        return TzdValue(0.0);
    });
    reg("torch_cuda_memory_reserved", [](auto args) -> TzdValue {
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            return TzdValue((double)torch::cuda::memory_reserved());
        }
#endif
        return TzdValue(0.0);
    });
    reg("torch_cuda_max_memory_allocated", [](auto args) -> TzdValue {
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            return TzdValue((double)torch::cuda::max_memory_allocated());
        }
#endif
        return TzdValue(0.0);
    });
    reg("torch_cuda_reset_peak_memory", [](auto args) -> TzdValue {
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            torch::cuda::reset_peak_memory_stats();
        }
#endif
        return TzdValue();
    });
    reg("torch_cuda_synchronize", [](auto args) -> TzdValue {
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            torch::cuda::synchronize();
        }
#endif
        return TzdValue();
    });

    // ---- Automatic scope-based VRAM cleanup ----
    reg("torch_auto_cleanup", [](auto args) -> TzdValue {
        // Enable automatic cleanup mode: when enabled, tensors that go out of
        // scope (TzdValue destroyed) are automatically released via refcount.
        // This is already the default with TENSOR type, but this function
        // also triggers empty_cache to reclaim CUDA memory.
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            torch::cuda::empty_cache();
        }
#endif
        return TzdValue();
    });
    reg("torch_gc", [](auto args) -> TzdValue {
#ifdef WITH_CUDA
        if (TzdPyTorch::isCudaAvailable()) {
            torch::cuda::empty_cache();
        }
#endif
        return TzdValue(0.0);
    });

    // ---- Random sampling ----
    reg("torch_bernoulli", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_bernoulli: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_bernoulli");
        return wrapTensor(new at::Tensor(torch::bernoulli(*a)));
    });
    reg("torch_multinomial", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_multinomial: requires (input, numSamples)");
        auto* a = requireTensor(args[0], "torch_multinomial");
        int64_t numSamples = (int64_t)valToDouble(args[1]);
        bool replacement = (args.size() > 2) ? (bool)(int)valToDouble(args[2]) : false;
        return wrapTensor(new at::Tensor(torch::multinomial(*a, numSamples, replacement)));
    });

    // ---- Additional tensor info ----
    reg("torch_is_contiguous", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->is_contiguous());
    });
    reg("torch_is_pinned", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->is_pinned());
    });

    // ---- Split and chunk ----
    reg("torch_chunk", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_chunk: requires (tensor, chunks)");
        auto* a = requireTensor(args[0], "torch_chunk");
        int64_t chunks = (int64_t)valToDouble(args[1]);
        int64_t dim = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : 0;
        auto result = torch::chunk(*a, chunks, dim);
        std::vector<TzdValue> arr;
        for (auto& t : result) arr.push_back(wrapTensor(new at::Tensor(t)));
        return TzdValue(arr);
    });
    reg("torch_split_t", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_split_t: requires (tensor, splitSize)");
        auto* a = requireTensor(args[0], "torch_split_t");
        int64_t splitSize = (int64_t)valToDouble(args[1]);
        int64_t dim = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : 0;
        auto result = torch::split(*a, splitSize, dim);
        std::vector<TzdValue> arr;
        for (auto& t : result) arr.push_back(wrapTensor(new at::Tensor(t)));
        return TzdValue(arr);
    });

    // ---- Additional reduction ----
    reg("torch_median", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_median: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_median");
        return wrapTensor(new at::Tensor(torch::median(*a)));
    });
    reg("torch_count_nonzero", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(0.0);
        auto* a = requireTensor(args[0], "torch_count_nonzero");
        return TzdValue((double)torch::count_nonzero(*a).item<int64_t>());
    });

    // ---- Upsampling ----
    reg("torch_upsample_nearest2d", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_upsample_nearest2d: requires (input, outputH, outputW)");
        auto* a = requireTensor(args[0], "torch_upsample_nearest2d");
        int64_t outH = (int64_t)valToDouble(args[1]);
        int64_t outW = (int64_t)valToDouble(args[2]);
        auto result = torch::upsample_nearest2d(*a, {outH, outW});
        return wrapTensor(new at::Tensor(result));
    });
    reg("torch_upsample_bilinear2d", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_upsample_bilinear2d: requires (input, outputH, outputW)");
        auto* a = requireTensor(args[0], "torch_upsample_bilinear2d");
        int64_t outH = (int64_t)valToDouble(args[1]);
        int64_t outW = (int64_t)valToDouble(args[2]);
        bool alignCorners = (args.size() > 3) ? (bool)(int)valToDouble(args[3]) : false;
        auto result = torch::upsample_bilinear2d(*a, {outH, outW}, alignCorners);
        return wrapTensor(new at::Tensor(result));
    });

    // ---- Tensor math ----
    reg("torch_trace_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_trace_t: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_trace_t");
        return wrapTensor(new at::Tensor(torch::trace(*a)));
    });
    reg("torch_det_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_det_t: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_det_t");
        return wrapTensor(new at::Tensor(torch::det(*a)));
    });
    reg("torch_inverse_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_inverse_t: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_inverse_t");
        return wrapTensor(new at::Tensor(torch::inverse(*a)));
    });
    reg("torch_cholesky", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_cholesky: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_cholesky");
        bool upper = (args.size() > 1) ? (bool)(int)valToDouble(args[1]) : false;
        return wrapTensor(new at::Tensor(at::linalg_cholesky(*a)));
    });
    reg("torch_lstsq", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_lstsq: requires (A, B)");
        auto* a = requireTensor(args[0], "torch_lstsq");
        auto* b = requireTensor(args[1], "torch_lstsq");
        auto result = at::linalg_lstsq(*a, *b);
        return wrapTensor(new at::Tensor(std::get<0>(result)));
    });

    // ---- Padding ----
    reg("torch_pad", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_pad: requires (input, pad)");
        auto* a = requireTensor(args[0], "torch_pad");
        // Parse pad from array: [left, right, top, bottom, ...]
        std::vector<int64_t> pad;
        if (args[1].type == TzdValue::ARRAY) {
            for (const auto& p : args[1].arrVal) pad.push_back((int64_t)valToDouble(p));
        }
        std::string mode = (args.size() > 2) ? args[2].sVal : "constant";
        double value = (args.size() > 3) ? valToDouble(args[3]) : 0.0;
        return wrapTensor(new at::Tensor(torch::pad(*a, pad, mode, value)));
    });

    // ---- Broadcasting helpers ----
    reg("torch_broadcast_tensors", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_broadcast_tensors: requires at least 1 tensor");
        std::vector<at::Tensor> tensors;
        for (const auto& a : args) {
            auto* t = getTensor(a);
            if (t) tensors.push_back(*t);
        }
        auto result = torch::broadcast_tensors(tensors);
        std::vector<TzdValue> arr;
        for (auto& t : result) arr.push_back(wrapTensor(new at::Tensor(t)));
        return TzdValue(arr);
    });
    reg("torch_broadcast_shapes", [](auto args) -> TzdValue {
        // Compute broadcast shape from multiple shape arrays
        std::vector<std::vector<int64_t>> shapes;
        for (const auto& a : args) {
            std::vector<int64_t> s;
            if (a.type == TzdValue::ARRAY) {
                for (const auto& e : a.arrVal) s.push_back((int64_t)valToDouble(e));
            } else {
                s.push_back((int64_t)valToDouble(a));
            }
            shapes.push_back(s);
        }
        auto result = torch::broadcast_shapes(shapes);
        std::vector<TzdValue> arr;
        for (auto dim : result) arr.push_back(TzdValue((double)dim));
        return TzdValue(arr);
    });

    // ---- Index put ----
    reg("torch_index_put", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_index_put: requires (tensor, indices, values)");
        auto* a = requireTensor(args[0], "torch_index_put");
        auto* values = requireTensor(args[2], "torch_index_put");
        if (args[1].type != TzdValue::ARRAY) return TzdValue::Error("torch_index_put: indices must be array of tensors");
        std::vector<at::Tensor> indices;
        for (const auto& idx : args[1].arrVal) {
            auto* t = getTensor(idx);
            if (t) indices.push_back(*t);
        }
        bool accumulate = (args.size() > 3) ? (bool)(int)valToDouble(args[3]) : false;
        c10::List<std::optional<at::Tensor>> idxList;
        for (const auto& idx : indices) idxList.push_back(std::optional<at::Tensor>(idx));
        return wrapTensor(new at::Tensor(at::index_put(*a, idxList, *values, accumulate)));
    });

    // ---- Scatter_ in-place ----
    reg("torch_scatter_", [](auto args) -> TzdValue {
        if (args.size() < 4) return TzdValue::Error("torch_scatter_: requires (tensor, dim, index, src)");
        auto* a = requireTensor(args[0], "torch_scatter_");
        int64_t dim = (int64_t)valToDouble(args[1]);
        auto* index = requireTensor(args[2], "torch_scatter_");
        auto* src = requireTensor(args[3], "torch_scatter_");
        a->scatter_(dim, *index, *src);
        return args[0];
    });

    // ---- Index copy_ in-place ----
    reg("torch_index_copy_", [](auto args) -> TzdValue {
        if (args.size() < 4) return TzdValue::Error("torch_index_copy_: requires (tensor, dim, index, source)");
        auto* a = requireTensor(args[0], "torch_index_copy_");
        int64_t dim = (int64_t)valToDouble(args[1]);
        auto* index = requireTensor(args[2], "torch_index_copy_");
        auto* src = requireTensor(args[3], "torch_index_copy_");
        a->index_copy_(dim, *index, *src);
        return args[0];
    });

    // ---- Tensor type conversion helpers ----
    reg("torch_to_float", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_to_float: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_to_float");
        return wrapTensor(new at::Tensor(a->to(torch::kFloat32)));
    });
    reg("torch_to_double", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_to_double: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_to_double");
        return wrapTensor(new at::Tensor(a->to(torch::kFloat64)));
    });
    reg("torch_to_int", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_to_int: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_to_int");
        return wrapTensor(new at::Tensor(a->to(torch::kInt64)));
    });
    reg("torch_to_long", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_to_long: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_to_long");
        return wrapTensor(new at::Tensor(a->to(torch::kInt64)));
    });
    reg("torch_to_bool", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_to_bool: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_to_bool");
        return wrapTensor(new at::Tensor(a->to(torch::kBool)));
    });

    // ---- Tensor statistics ----
    reg("torch_std_mean", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_std_mean: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_std_mean");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        auto [std_val, mean_val] = at::std_mean(*a, dim, false, false);
        std::vector<TzdValue> arr;
        arr.push_back(wrapTensor(new at::Tensor(std_val)));
        arr.push_back(wrapTensor(new at::Tensor(mean_val)));
        return TzdValue(arr);
    });
    reg("torch_var_mean", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_var_mean: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_var_mean");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        auto [var_val, mean_val] = at::var_mean(*a, dim, false, false);
        std::vector<TzdValue> arr;
        arr.push_back(wrapTensor(new at::Tensor(var_val)));
        arr.push_back(wrapTensor(new at::Tensor(mean_val)));
        return TzdValue(arr);
    });

    // ---- Math helpers ----
    reg("torch_logsumexp", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_logsumexp: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_logsumexp");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        bool keepdim = (args.size() > 2) ? (bool)(int)valToDouble(args[2]) : false;
        return wrapTensor(new at::Tensor(torch::logsumexp(*a, dim, keepdim)));
    });
    reg("torch_logcumsumexp", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_logcumsumexp: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_logcumsumexp");
        int64_t dim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        return wrapTensor(new at::Tensor(torch::logcumsumexp(*a, dim)));
    });
    reg("torch_corrcoef", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_corrcoef: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_corrcoef");
        return wrapTensor(new at::Tensor(torch::corrcoef(*a)));
    });
    reg("torch_cov", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_cov: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_cov");
        return wrapTensor(new at::Tensor(torch::cov(*a)));
    });

    // ---- Tensor comparison returning scalar ----
    reg("torch_any", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->any().item<bool>());
    });
    reg("torch_all", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->all().item<bool>());
    });
    reg("torch_isfinite", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_isfinite: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_isfinite");
        return wrapTensor(new at::Tensor(torch::isfinite(*a)));
    });
    reg("torch_isnan", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_isnan: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_isnan");
        return wrapTensor(new at::Tensor(torch::isnan(*a)));
    });
    reg("torch_isinf", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_isinf: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_isinf");
        return wrapTensor(new at::Tensor(torch::isinf(*a)));
    });

    // ---- Frobenius norm, matrix exp ----
    reg("torch_matrix_exp", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_matrix_exp: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_matrix_exp");
        return wrapTensor(new at::Tensor(torch::matrix_exp(*a)));
    });
    reg("torch_solve_t", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_solve_t: requires (A, B)");
        auto* a = requireTensor(args[0], "torch_solve_t");
        auto* b = requireTensor(args[1], "torch_solve_t");
        auto result = at::linalg_solve(*a, *b);
        return wrapTensor(new at::Tensor(result));
    });

    // ---- Model management helpers (for nn.Module system) ----
    reg("torch_create_param", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_create_param: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_create_param");
        a->requires_grad_(true);
        return args[0]; // Return the same tensor (now with requires_grad=true)
    });

    reg("torch_zero_grad_params", [](auto args) -> TzdValue {
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        for (const auto& p : args[0].arrVal) {
            auto* t = getTensor(p);
            if (t && t->grad().defined()) {
                t->grad().zero_();
            }
        }
        return TzdValue();
    });

    reg("torch_optimizer_create", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_optimizer_create: requires (typeStr, params, lr)");
        std::string typeStr = args[0].sVal;
        std::vector<at::Tensor> params;
        if (args[1].type == TzdValue::ARRAY) {
            for (const auto& p : args[1].arrVal) {
                auto* t = getTensor(p);
                if (t) params.push_back(*t);
            }
        }
        double lr = valToDouble(args[2]);

        TzdOptimizer* optim = nullptr;
        if (typeStr == "sgd") {
            double momentum = (args.size() > 3) ? valToDouble(args[3]) : 0.0;
            double weightDecay = (args.size() > 4) ? valToDouble(args[4]) : 0.0;
            optim = new TzdOptimizer();
            optim->type = TzdOptimizer::SGD;
            optim->lr = lr; optim->momentum = momentum; optim->weight_decay = weightDecay;
            optim->params = params;
        } else if (typeStr == "adam") {
            double beta1 = (args.size() > 3) ? valToDouble(args[3]) : 0.9;
            double beta2 = (args.size() > 4) ? valToDouble(args[4]) : 0.999;
            double weightDecay = (args.size() > 5) ? valToDouble(args[5]) : 0.0;
            optim = new TzdOptimizer();
            optim->type = TzdOptimizer::ADAM;
            optim->lr = lr; optim->beta1 = beta1; optim->beta2 = beta2; optim->weight_decay = weightDecay; optim->eps = 1e-8;
            optim->params = params;
        } else if (typeStr == "adamw") {
            double weightDecay = (args.size() > 3) ? valToDouble(args[3]) : 0.01;
            optim = new TzdOptimizer();
            optim->type = TzdOptimizer::ADAMW;
            optim->lr = lr; optim->beta1 = 0.9; optim->beta2 = 0.999; optim->weight_decay = weightDecay; optim->eps = 1e-8;
            optim->params = params;
        } else if (typeStr == "rmsprop") {
            double alpha = (args.size() > 3) ? valToDouble(args[3]) : 0.99;
            double eps = (args.size() > 4) ? valToDouble(args[4]) : 1e-8;
            optim = new TzdOptimizer();
            optim->type = TzdOptimizer::RMSPROP;
            optim->lr = lr; optim->alpha = alpha; optim->eps = eps; optim->weight_decay = 0.0; optim->momentum = 0.0;
            optim->params = params;
        } else if (typeStr == "adagrad") {
            optim = new TzdOptimizer();
            optim->type = TzdOptimizer::ADAGRAD;
            optim->lr = lr; optim->weight_decay = 0.0; optim->eps = 1e-10;
            optim->params = params;
        } else {
            return TzdValue::Error("Unknown optimizer type: " + typeStr);
        }

        TzdValue v; v.type = TzdValue::POINTER; v.ptrVal = optim; return v;
    });

    reg("torch_save_state_dict", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_save_state_dict: requires (stateDict, path)");
        if (args[0].type != TzdValue::MAP) return TzdValue::Error("torch_save_state_dict: first arg must be map");
        std::string path = args[1].sVal;
        // Save each tensor in the state dict
        std::ofstream file(path, std::ios::binary);
        if (!file) return TzdValue::Error("Cannot open file: " + path);
        // Write header: number of entries
        uint64_t numEntries = args[0].mapVal.size();
        file.write(reinterpret_cast<const char*>(&numEntries), sizeof(numEntries));
        for (const auto& [name, val] : args[0].mapVal) {
            auto* t = getTensor(val);
            if (!t) continue;
            // Write name length + name
            uint64_t nameLen = name.size();
            file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
            file.write(name.data(), nameLen);
            // Manual serialization: write dtype, ndim, shape, then raw data bytes
            at::Tensor ct = t->contiguous();
            int64_t dtype = (int64_t)ct.scalar_type();
            file.write(reinterpret_cast<const char*>(&dtype), sizeof(dtype));
            int64_t ndim = ct.dim();
            file.write(reinterpret_cast<const char*>(&ndim), sizeof(ndim));
            for (int64_t d = 0; d < ndim; d++) {
                int64_t sz = ct.size(d);
                file.write(reinterpret_cast<const char*>(&sz), sizeof(sz));
            }
            uint64_t dataLen = ct.nbytes();
            file.write(reinterpret_cast<const char*>(&dataLen), sizeof(dataLen));
            if (dataLen > 0) {
                file.write(static_cast<const char*>(ct.data_ptr()), dataLen);
            }
        }
        return TzdValue(true);
    });

    reg("torch_load_state_dict", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_load_state_dict: requires (path)");
        std::string path = args[0].sVal;
        std::ifstream file(path, std::ios::binary);
        if (!file) return TzdValue::Error("Cannot open file: " + path);
        uint64_t numEntries = 0;
        file.read(reinterpret_cast<char*>(&numEntries), sizeof(numEntries));
        std::unordered_map<std::string, TzdValue> result;
        for (uint64_t i = 0; i < numEntries; i++) {
            uint64_t nameLen = 0;
            file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
            std::string name(nameLen, '\0');
            file.read(&name[0], nameLen);
            int64_t dtype = 0;
            file.read(reinterpret_cast<char*>(&dtype), sizeof(dtype));
            int64_t ndim = 0;
            file.read(reinterpret_cast<char*>(&ndim), sizeof(ndim));
            std::vector<int64_t> shape(ndim);
            for (int64_t d = 0; d < ndim; d++) {
                file.read(reinterpret_cast<char*>(&shape[d]), sizeof(shape[d]));
            }
            uint64_t dataLen = 0;
            file.read(reinterpret_cast<char*>(&dataLen), sizeof(dataLen));
            std::vector<char> rawData(dataLen);
            if (dataLen > 0) {
                file.read(rawData.data(), dataLen);
            }
            auto* t = new at::Tensor();
            if (dataLen > 0) {
                *t = torch::from_blob(rawData.data(), shape,
                    at::TensorOptions().dtype((torch::Dtype)dtype)).clone();
            } else {
                *t = torch::empty(shape, at::TensorOptions().dtype((torch::Dtype)dtype));
            }
            result[name] = wrapTensor(t);
        }
        return TzdValue(result);
    });

    reg("torch_init_normal", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_init_normal: requires (tensor, [mean], [std])");
        auto* a = requireTensor(args[0], "torch_init_normal");
        double mean = (args.size() > 1) ? valToDouble(args[1]) : 0.0;
        double std = (args.size() > 2) ? valToDouble(args[2]) : 0.02;
        if (a->defined()) {
            *a = torch::normal(mean, std, a->sizes());
        }
        return args[0];
    });

    reg("torch_init_uniform", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_init_uniform: requires (tensor, [low], [high])");
        auto* a = requireTensor(args[0], "torch_init_uniform");
        double low = (args.size() > 1) ? valToDouble(args[1]) : -0.05;
        double high = (args.size() > 2) ? valToDouble(args[2]) : 0.05;
        if (a->defined()) {
            *a = torch::empty(a->sizes()).uniform_(low, high);
        }
        return args[0];
    });

    reg("torch_init_xavier", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_init_xavier: requires (tensor)");
        auto* a = requireTensor(args[0], "torch_init_xavier");
        if (a->defined() && a->dim() >= 2) {
            double fanIn = (double)a->size(1);
            double fanOut = (double)a->size(0);
            double std = std::sqrt(2.0 / (fanIn + fanOut));
            *a = torch::normal(0.0, std, a->sizes());
        }
        return args[0];
    });

    reg("torch_init_kaiming", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_init_kaiming: requires (tensor)");
        auto* a = requireTensor(args[0], "torch_init_kaiming");
        if (a->defined() && a->dim() >= 2) {
            double fanIn = (double)a->size(1);
            double std = std::sqrt(2.0 / fanIn);
            *a = torch::normal(0.0, std, a->sizes());
        }
        return args[0];
    });

    reg("torch_init_zeros", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_init_zeros: requires (tensor)");
        auto* a = requireTensor(args[0], "torch_init_zeros");
        if (a->defined()) a->zero_();
        return args[0];
    });

    reg("torch_init_ones", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_init_ones: requires (tensor)");
        auto* a = requireTensor(args[0], "torch_init_ones");
        if (a->defined()) a->fill_(1.0);
        return args[0];
    });

    reg("torch_no_grad_scope", [](auto args) -> TzdValue {
        // Toggle no_grad mode (returns previous state)
        bool wasEnabled = torch::isGradEnabled();
        torch::NoGradGuard guard;
        // Note: This is a no-op in functional context since the guard is local
        // For proper no_grad context, use torch_set_grad_enabled(false) before operations
        // and torch_set_grad_enabled(true) after
        return TzdValue(wasEnabled);
    });

    reg("torch_count_params", [](auto args) -> TzdValue {
        // Count total parameters in a parameter array
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue(0.0);
        int64_t total = 0;
        for (const auto& p : args[0].arrVal) {
            auto* t = getTensor(p);
            if (t) total += t->numel();
        }
        return TzdValue((double)total);
    });

    reg("torch_requires_grad_params", [](auto args) -> TzdValue {
        // Set requires_grad for all params in array
        if (args.empty() || args[0].type != TzdValue::ARRAY) return TzdValue();
        bool rg = (args.size() > 1) ? (bool)(int)valToDouble(args[1]) : true;
        for (const auto& p : args[0].arrVal) {
            auto* t = getTensor(p);
            if (t) t->requires_grad_(rg);
        }
        return TzdValue();
    });

    // ---- BatchNorm ----
    reg("torch_batch_norm1d", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_batch_norm1d: requires (input, weight, bias, [training], [running_mean], [running_var], [eps], [momentum])");
        auto* input = requireTensor(args[0], "torch_batch_norm1d");
        auto* weight = requireTensor(args[1], "torch_batch_norm1d");
        auto* bias = requireTensor(args[2], "torch_batch_norm1d");
        bool training = (args.size() > 3) ? (bool)(int)valToDouble(args[3]) : false;
        double eps = (args.size() > 6) ? valToDouble(args[6]) : 1e-5;
        double momentum = (args.size() > 7) ? valToDouble(args[7]) : 0.1;
        // Manual batch norm for [N, C] input
        auto mean = input->mean(0, true);
        auto var = input->var(0, true, false);
        auto normalized = (*input - mean) / at::sqrt(var + eps);
        auto out = normalized * weight->unsqueeze(0) + bias->unsqueeze(0);
        // Update running stats
        if (training) {
            at::Tensor runningMean, runningVar;
            if (args.size() > 4 && getTensor(args[4])) {
                runningMean = *getTensor(args[4]);
                runningMean = (1 - momentum) * runningMean + momentum * mean.squeeze(0).detach();
            }
            if (args.size() > 5 && getTensor(args[5])) {
                runningVar = *getTensor(args[5]);
                runningVar = (1 - momentum) * runningVar + momentum * var.squeeze(0).detach();
            }
        }
        return wrapTensor(new at::Tensor(out));
    });
    reg("torch_batch_norm2d", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_batch_norm2d: requires (input, weight, bias, [training], [running_mean], [running_var], [eps], [momentum])");
        auto* input = requireTensor(args[0], "torch_batch_norm2d");
        auto* weight = requireTensor(args[1], "torch_batch_norm2d");
        auto* bias = requireTensor(args[2], "torch_batch_norm2d");
        bool training = (args.size() > 3) ? (bool)(int)valToDouble(args[3]) : false;
        double eps = (args.size() > 6) ? valToDouble(args[6]) : 1e-5;
        double momentum = (args.size() > 7) ? valToDouble(args[7]) : 0.1;
        // Manual batch norm for [N, C, H, W] input
        auto mean = input->mean({0, 2, 3}, true);
        auto var = input->var({0, 2, 3}, true, false);
        auto normalized = (*input - mean) / at::sqrt(var + eps);
        auto w = weight->reshape({1, -1, 1, 1});
        auto b = bias->reshape({1, -1, 1, 1});
        auto out = normalized * w + b;
        if (training) {
            at::Tensor runningMean, runningVar;
            if (args.size() > 4 && getTensor(args[4])) {
                runningMean = *getTensor(args[4]);
                runningMean = (1 - momentum) * runningMean + momentum * mean.view({-1}).detach();
            }
            if (args.size() > 5 && getTensor(args[5])) {
                runningVar = *getTensor(args[5]);
                runningVar = (1 - momentum) * runningVar + momentum * var.view({-1}).detach();
            }
        }
        return wrapTensor(new at::Tensor(out));
    });

    // ---- Tensor info and utilities ----
    reg("torch_shape", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_shape: requires 1 tensor");
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue::Error("torch_shape: not a tensor");
        auto sizes = a->sizes();
        std::string s = "[";
        for (size_t i = 0; i < sizes.size(); ++i) {
            if (i > 0) s += ", ";
            s += std::to_string(sizes[i]);
        }
        s += "]";
        return TzdValue(s);
    });
    reg("torch_dtype_str", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(std::string("unknown"));
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(std::string("unknown"));
        return TzdValue(std::string(c10::toString(a->scalar_type())));
    });
    reg("torch_is_contiguous", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue(false);
        auto* a = getTensor(args[0]);
        if (!a) return TzdValue(false);
        return TzdValue(a->is_contiguous());
    });
    reg("torch_make_contiguous", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_make_contiguous: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_make_contiguous");
        if (!a->is_contiguous()) *a = a->contiguous();
        return args[0];
    });
    reg("torch_flatten_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_flatten_t: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_flatten_t");
        int64_t startDim = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 0;
        int64_t endDim = (args.size() > 2) ? (int64_t)valToDouble(args[2]) : -1;
        return wrapTensor(new at::Tensor(a->flatten(startDim, endDim)));
    });
    reg("torch_unsqueeze", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_unsqueeze: requires (tensor, dim)");
        auto* a = requireTensor(args[0], "torch_unsqueeze");
        int64_t dim = (int64_t)valToDouble(args[1]);
        return wrapTensor(new at::Tensor(a->unsqueeze(dim)));
    });
    reg("torch_squeeze", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_squeeze: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_squeeze");
        if (args.size() > 1) {
            int64_t dim = (int64_t)valToDouble(args[1]);
            return wrapTensor(new at::Tensor(a->squeeze(dim)));
        }
        return wrapTensor(new at::Tensor(a->squeeze()));
    });
    reg("torch_expand", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_expand: requires (tensor, sizes_array)");
        auto* a = requireTensor(args[0], "torch_expand");
        std::vector<int64_t> sizes;
        if (args[1].type == TzdValue::ARRAY) {
            for (const auto& e : args[1].arrVal) sizes.push_back((int64_t)valToDouble(e));
        }
        return wrapTensor(new at::Tensor(a->expand(sizes)));
    });
    reg("torch_repeat", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_repeat: requires (tensor, sizes_array)");
        auto* a = requireTensor(args[0], "torch_repeat");
        std::vector<int64_t> sizes;
        if (args[1].type == TzdValue::ARRAY) {
            for (const auto& e : args[1].arrVal) sizes.push_back((int64_t)valToDouble(e));
        }
        return wrapTensor(new at::Tensor(a->repeat(sizes)));
    });
    reg("torch_gather", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_gather: requires (input, dim, index)");
        auto* a = requireTensor(args[0], "torch_gather");
        int64_t dim = (int64_t)valToDouble(args[1]);
        auto* idx = requireTensor(args[2], "torch_gather");
        return wrapTensor(new at::Tensor(a->gather(dim, *idx)));
    });
    reg("torch_scatter", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_scatter: requires (input, dim, index, src)");
        auto* a = requireTensor(args[0], "torch_scatter");
        int64_t dim = (int64_t)valToDouble(args[1]);
        auto* idx = requireTensor(args[2], "torch_scatter");
        auto* src = requireTensor(args[3], "torch_scatter");
        return wrapTensor(new at::Tensor(a->scatter(dim, *idx, *src)));
    });
    reg("torch_norm_t", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_norm_t: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_norm_t");
        int64_t p = (args.size() > 1) ? (int64_t)valToDouble(args[1]) : 2;
        return wrapTensor(new at::Tensor(a->norm((double)p)));
    });
    reg("torch_det", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_det: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_det");
        return wrapTensor(new at::Tensor(at::linalg_det(*a)));
    });
    reg("torch_inv", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_inv: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_inv");
        return wrapTensor(new at::Tensor(at::linalg_inv(*a)));
    });
    reg("torch_svd", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_svd: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_svd");
        auto result = at::linalg_svd(*a);
        return wrapTensor(new at::Tensor(std::get<0>(result)));
    });
    reg("torch_eig", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_eig: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_eig");
        auto result = at::linalg_eig(*a);
        return wrapTensor(new at::Tensor(std::get<0>(result)));
    });
    reg("torch_solve", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_solve: requires (A, B)");
        auto* a = requireTensor(args[0], "torch_solve");
        auto* b = requireTensor(args[1], "torch_solve");
        return wrapTensor(new at::Tensor(at::linalg_solve(*a, *b)));
    });
    reg("torch_chain_matmul", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_chain_matmul: requires at least 2 tensors");
        auto* result = requireTensor(args[0], "torch_chain_matmul");
        at::Tensor out = *result;
        for (size_t i = 1; i < args.size(); ++i) {
            auto* t = requireTensor(args[i], "torch_chain_matmul");
            out = at::matmul(out, *t);
        }
        return wrapTensor(new at::Tensor(out));
    });
    reg("torch_bmm", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_bmm: requires (batch1, batch2)");
        auto* a = requireTensor(args[0], "torch_bmm");
        auto* b = requireTensor(args[1], "torch_bmm");
        return wrapTensor(new at::Tensor(a->bmm(*b)));
    });
    reg("torch_inverse", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_inverse: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_inverse");
        return wrapTensor(new at::Tensor(at::linalg_inv(*a)));
    });
    reg("torch_pca", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_pca: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_pca");
        auto mean = a->mean(0, true);
        auto centered = *a - mean;
        auto svd_result = at::linalg_svd(centered);
        return wrapTensor(new at::Tensor(std::get<0>(svd_result)));
    });
    reg("torch_orth", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_orth: requires 1 tensor");
        auto* a = requireTensor(args[0], "torch_orth");
        auto svd_result = at::linalg_svd(*a);
        auto U = std::get<0>(svd_result);
        auto S = std::get<1>(svd_result);
        int rank = 0;
        for (int i = 0; i < S.size(0); ++i) { if (S[i].item<double>() > 1e-10) ++rank; else break; }
        return wrapTensor(new at::Tensor(U.narrow(1, 0, rank)));
    });

    // ================================================================
    // INT8 Quantization Support
    // ================================================================
    reg("torch_quantize_per_tensor", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_quantize_per_tensor: requires (tensor, scale, zero_point)");
        auto* a = requireTensor(args[0], "torch_quantize_per_tensor");
        double scale = TzdInterpreter::getAsDouble(args[1]);
        int64_t zero_point = (int64_t)args[2].lVal;
        auto quantized = at::quantize_per_tensor(*a, scale, zero_point, torch::kQUInt8);
        return wrapTensor(new at::Tensor(quantized));
    });

    reg("torch_dequantize", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_dequantize: requires (tensor)");
        auto* a = requireTensor(args[0], "torch_dequantize");
        return wrapTensor(new at::Tensor(a->dequantize()));
    });

    reg("torch_quantize_per_channel", [](auto args) -> TzdValue {
        if (args.size() < 4) return TzdValue::Error("torch_quantize_per_channel: requires (tensor, scales, zero_points, axis)");
        auto* a = requireTensor(args[0], "torch_quantize_per_channel");
        auto* scales = requireTensor(args[1], "torch_quantize_per_channel.scales");
        auto* zero_points = requireTensor(args[2], "torch_quantize_per_channel.zero_points");
        int64_t axis = (int64_t)args[3].lVal;
        auto quantized = at::quantize_per_channel(*a, *scales, *zero_points, axis, torch::kQUInt8);
        return wrapTensor(new at::Tensor(quantized));
    });

    reg("torch_q_scale", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_q_scale: requires (tensor)");
        auto* a = requireTensor(args[0], "torch_q_scale");
        if (!a->is_quantized()) return TzdValue::Error("torch_q_scale: tensor is not quantized");
        double scale = a->q_scale();
        return TzdValue(scale);
    });

    reg("torch_q_zero_point", [](auto args) -> TzdValue {
        if (args.empty()) return TzdValue::Error("torch_q_zero_point: requires (tensor)");
        auto* a = requireTensor(args[0], "torch_q_zero_point");
        if (!a->is_quantized()) return TzdValue::Error("torch_q_zero_point: tensor is not quantized");
        int64_t zp = a->q_zero_point();
        return TzdValue((int64_t)zp);
    });

    // ================================================================
    // Fused Operators (reduce kernel launches)
    // ================================================================

    // Fused: Linear + Bias + GELU (common in transformer MLP layers)
    reg("torch_fused_linear_bias_gelu", [](auto args) -> TzdValue {
        if (args.size() < 3) return TzdValue::Error("torch_fused_linear_bias_gelu: requires (weight, input, bias)");
        auto* weight = requireTensor(args[0], "torch_fused_linear_bias_gelu.w");
        auto* input = requireTensor(args[1], "torch_fused_linear_bias_gelu.x");
        auto* bias = requireTensor(args[2], "torch_fused_linear_bias_gelu.b");
        // Linear: y = x @ W^T + b, then GELU
        at::Tensor linear = at::linear(*input, *weight, *bias);
        at::Tensor result = at::gelu(linear);
        return wrapTensor(new at::Tensor(result));
    });

    // Fused: Residual Add + LayerNorm (common in transformer blocks)
    reg("torch_fused_residual_layernorm", [](auto args) -> TzdValue {
        if (args.size() < 5) return TzdValue::Error("torch_fused_residual_layernorm: requires (x, residual, weight, bias, eps)");
        auto* x = requireTensor(args[0], "torch_fused_residual_layernorm.x");
        auto* residual = requireTensor(args[1], "torch_fused_residual_layernorm.r");
        auto* weight = requireTensor(args[2], "torch_fused_residual_layernorm.w");
        auto* bias = requireTensor(args[3], "torch_fused_residual_layernorm.b");
        double eps = TzdInterpreter::getAsDouble(args[4]);
        at::Tensor added = *x + *residual;
        at::Tensor result = at::layer_norm(added, weight->sizes(), *weight, *bias, eps);
        return wrapTensor(new at::Tensor(result));
    });

    // Fused: SiLU + Mul (SwiGLU variant used in LLaMA/GLM)
    reg("torch_fused_silu_mul", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_fused_silu_mul: requires (input, gate)");
        auto* input = requireTensor(args[0], "torch_fused_silu_mul.x");
        auto* gate = requireTensor(args[1], "torch_fused_silu_mul.g");
        at::Tensor activated = at::silu(*input);
        at::Tensor result = activated * (*gate);
        return wrapTensor(new at::Tensor(result));
    });

    // Fused: Softmax + Mask (for attention)
    reg("torch_fused_softmax_mask", [](auto args) -> TzdValue {
        if (args.size() < 2) return TzdValue::Error("torch_fused_softmax_mask: requires (scores, mask)");
        auto* scores = requireTensor(args[0], "torch_fused_softmax_mask.s");
        auto* mask = requireTensor(args[1], "torch_fused_softmax_mask.m");
        at::Tensor masked = *scores + *mask;
        at::Tensor result = at::softmax(masked, -1);
        return wrapTensor(new at::Tensor(result));
    });
}
