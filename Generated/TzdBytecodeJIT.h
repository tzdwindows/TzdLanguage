// ============================================================================
// TzdBytecodeJIT.h - JIT bridge for bytecode VM: compile hot bytecode functions
//                    to native code via LLVM ORC, using existing TzdJitEngine
// ============================================================================

#pragma once

#include "TzdBytecode.h"
#include "TzdInterpreter.h"
#include <atomic>
#include <unordered_map>

// JIT bridge: monitors bytecode function call counts and triggers
// LLVM ORC JIT compilation for hot functions.
class TzdBytecodeJIT {
public:
    static TzdBytecodeJIT& getInstance();

    // Called by the VM on each function call.
    // Returns a non-null function pointer if the function has been JIT-compiled.
    // If the function is hot enough and not yet compiled, triggers compilation.
    void* onFunctionCall(const BytecodeModule& module, const std::string& funcName,
                         TzdInterpreter* interp);

    // Check if a function has been JIT-compiled
    bool isJitted(const std::string& funcName) const;

    // Get the JIT-compiled function pointer
    void* getJittedPtr(const std::string& funcName) const;

    // Clear all JIT caches (for testing)
    void clear();

    // Configuration
    void setHotThreshold(int threshold) { m_hotThreshold = threshold; }
    int getHotThreshold() const { return m_hotThreshold; }

    // Statistics
    size_t getTotalCalls() const { return m_totalCalls.load(); }
    size_t getJitCompilations() const { return m_jitCompilations.load(); }
    size_t getJitFailures() const { return m_jitFailures.load(); }

private:
    TzdBytecodeJIT() = default;

    // Function name → call count
    std::unordered_map<std::string, std::atomic<int>> m_callCounts;

    // Function name → JIT-compiled function pointer
    std::unordered_map<std::string, void*> m_jittedPtrs;

    // Function name → compilation attempted (don't retry)
    std::unordered_set<std::string> m_compilationAttempted;

    int m_hotThreshold = 999999; // Temporarily disable JIT for benchmarking
    std::atomic<size_t> m_totalCalls{0};
    std::atomic<size_t> m_jitCompilations{0};
    std::atomic<size_t> m_jitFailures{0};
    std::mutex m_mutex;
};
