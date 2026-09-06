#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <atomic>
#include <ATen/core/Tensor.h>

class TzdInterpreter;
struct TzdValue;

/**
 * TzdPyTorch: Native PyTorch (libtorch) bindings for TzdLang.
 * Provides tensor operations, neural network modules, and memory management.
 * Tensors are stored as heap-allocated at::Tensor* pointers in TzdValue::ptrVal
 * with type TzdValue::TENSOR, enabling automatic reference-counted lifecycle management.
 * A global registry tracks all live tensors with refcounts to guarantee zero memory leaks.
 */
class TzdPyTorch {
public:
    static void init(TzdInterpreter* interp);

    // Extract a at::Tensor* from a TzdValue (returns nullptr if not a tensor)
    static at::Tensor* getTensor(const TzdValue& val);

    // Wrap a heap-allocated tensor into a TzdValue (type=TENSOR, auto-managed)
    static TzdValue wrapTensor(at::Tensor* t);

    // Register a tensor pointer in the global registry (refcount=1)
    static void registerTensor(at::Tensor* t);

    // Increment reference count — uses c10::intrusive_ptr refcount directly (lock-free)
    static void retainTensor(at::Tensor* t);

    // Decrement reference count; deletes tensor when refcount reaches 0 (lock-free)
    static void releaseTensor(at::Tensor* t);

    // Release all tensors (called on shutdown — now a no-op since refcounting is automatic)
    static void releaseAll();

    // Get total memory used by all live tensors (best-effort estimate)
    static size_t totalMemoryBytes();

    // Get number of live tensors (best-effort estimate)
    static size_t numTensors();

    // Check if CUDA is available
    static bool isCudaAvailable();

private:
    static bool s_initialized;

    // Per-thread tensor count for diagnostics (lock-free, approximate)
    static std::atomic<size_t> s_liveTensorCount;
    static std::atomic<size_t> s_liveTensorBytes;

    static void regTensorCreation(TzdInterpreter* interp);
    static void regTensorOps(TzdInterpreter* interp);
    static void regTensorProperties(TzdInterpreter* interp);
    static void regReductionOps(TzdInterpreter* interp);
    static void regIndexingOps(TzdInterpreter* interp);
    static void regNNFunctions(TzdInterpreter* interp);
    static void regOptimizers(TzdInterpreter* interp);
    static void regDeviceMgmt(TzdInterpreter* interp);
    static void regAutograd(TzdInterpreter* interp);
    static void regSerialization(TzdInterpreter* interp);
    static void regConversion(TzdInterpreter* interp);
    static void regExtendedOps(TzdInterpreter* interp);
};
