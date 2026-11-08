// ============================================================================
// TzdStubFallback.cpp
// Fallback stubs for optional heavy dependencies (LibTorch, CUDA, Debugger).
// Compiled ONLY into tzd_stub.exe (NOT into TzdTools.exe main build).
// Ensures the standalone runner is completely DLL-independent (Zero-DLL).
// ============================================================================

#include <string>
#include <vector>
#include <cstdint>
#include <any>

// ── PyTorch stub ─────────────────────────────────────────────────────────────
// When tzd_stub.exe is running a payload, it should never call PyTorch APIs.
// These stubs satisfy the linker without pulling in torch/c10 DLLs.
#ifndef WITH_LIBTORCH
class TzdInterpreter; // forward decl
class TzdPyTorch {
public:
    static void init(TzdInterpreter*) {}
};
#endif

// ── Tensor memory management stubs ───────────────────────────────────────────
extern "C" {
    void tzdTensorRetain(void*) {}
    void tzdTensorRelease(void*) {}
}

// ── Global GPU/CPU mode flags (defined in TzdExperimentalCompute normally) ───
// Only define if not already defined by a compiled .obj
#ifndef TZD_EXPERIMENTAL_COMPUTE_DEFINED
bool g_forceGPU = false;
bool g_forceCPU = false;
bool g_bigTime  = false;
#endif

// ── BigInt GPU acceleration stubs ────────────────────────────────────────────
// (Real implementations are in TzdPyTorch.cpp which we skip in stub build)
bool bigint_fft_available() { return false; }
bool bigint_gpu_suitable(size_t) { return false; }
void bigint_gpu_warmup(int) {}
std::string bigint_mul_fft(const std::string&, const std::string&) { return ""; }
std::string bigint_mul_gpu_ntt_str(const std::string&, const std::string&) { return ""; }
std::vector<uint64_t> bigint_mul_gpu_ntt_limbs(
    const std::vector<uint64_t>&, const std::vector<uint64_t>&) { return {}; }

// ── Debugger stubs ────────────────────────────────────────────────────────────
// The real TzdDebugger is a separate .cpp (TzdDebugger.cpp) — but in stub
// builds we need to avoid it pulling in heavy headers. These stubs satisfy
// TzdInterpreter.obj's external references to TzdDebugger symbols.
namespace TzdDebugger {
    // NOTE: g_DebugActive, hasBreakpointsInFunction, isStepping are already
    // defined in TzdDebugger.cpp which IS compiled into the stub.
    // If you get LNK2005 for these, remove them from here.
}
