// ============================================================================
// TzdBytecodeJIT.cpp - JIT bridge for bytecode VM
// ============================================================================

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "TzdBytecodeJIT.h"
#include "TzdJit.h"
#include <iostream>

TzdBytecodeJIT& TzdBytecodeJIT::getInstance() {
    static TzdBytecodeJIT instance;
    return instance;
}

void* TzdBytecodeJIT::onFunctionCall(const BytecodeModule& module,
                                     const std::string& funcName,
                                     TzdInterpreter* interp) {
    m_totalCalls.fetch_add(1, std::memory_order_relaxed);

    // Check if already JIT-compiled
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_jittedPtrs.find(funcName);
        if (it != m_jittedPtrs.end() && it->second) {
            return it->second;
        }
    }

    // Check if compilation was already attempted (don't retry)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_compilationAttempted.count(funcName)) {
            return nullptr; // Previous attempt failed
        }
    }

    // Increment call count
    int count;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        count = ++m_callCounts[funcName];
    }

    // Check if hot enough to trigger JIT compilation
    if (count < m_hotThreshold) {
        return nullptr; // Not hot enough yet
    }

    // Trigger JIT compilation
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Double-check under lock
        auto it = m_jittedPtrs.find(funcName);
        if (it != m_jittedPtrs.end() && it->second) {
            return it->second;
        }

        // Mark as attempted
        m_compilationAttempted.insert(funcName);

        // Try to find the function in the interpreter's global scope
        // and trigger JIT compilation
        if (interp) {
            // Search for the function in the interpreter's scopes
            for (auto& scope : interp->scopes) {
                auto valIt = scope.find(funcName);
                if (valIt != scope.end()) {
                    TzdValue& funcVal = valIt->second;
                    if (funcVal.type == TzdValue::FUNCTION && funcVal.funcBody) {
                        // Trigger on-demand JIT compilation
                        interp->tryJitCompile(funcVal);

                        if (funcVal.jittedPtr) {
                            // JIT compilation succeeded!
                            m_jittedPtrs[funcName] = (void*)funcVal.jittedPtr;
                            m_jitCompilations.fetch_add(1, std::memory_order_relaxed);
                            return (void*)funcVal.jittedPtr;
                        }
                    }
                    break;
                }
            }
        }

        // JIT compilation failed or function not found
        m_jitFailures.fetch_add(1, std::memory_order_relaxed);
    }

    return nullptr;
}

bool TzdBytecodeJIT::isJitted(const std::string& funcName) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    auto it = m_jittedPtrs.find(funcName);
    return it != m_jittedPtrs.end() && it->second != nullptr;
}

void* TzdBytecodeJIT::getJittedPtr(const std::string& funcName) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    auto it = m_jittedPtrs.find(funcName);
    if (it != m_jittedPtrs.end()) {
        return it->second;
    }
    return nullptr;
}

void TzdBytecodeJIT::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callCounts.clear();
    m_jittedPtrs.clear();
    m_compilationAttempted.clear();
    m_totalCalls.store(0, std::memory_order_relaxed);
    m_jitCompilations.store(0, std::memory_order_relaxed);
    m_jitFailures.store(0, std::memory_order_relaxed);
}
