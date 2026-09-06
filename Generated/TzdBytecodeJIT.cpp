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
#include "TzdTieringEngine.h"
#include <iostream>

TzdBytecodeJIT& TzdBytecodeJIT::getInstance() {
    static TzdBytecodeJIT instance;
    return instance;
}

void* TzdBytecodeJIT::onFunctionCall(const BytecodeModule& module,
                                     const std::string& funcName,
                                     TzdInterpreter* interp) {
    m_totalCalls.fetch_add(1, std::memory_order_relaxed);

    // Single-lock fast path: check compiled, attempted, and increment count
    int count;
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Check if already JIT-compiled (by background thread)
        auto it = m_jittedPtrs.find(funcName);
        if (it != m_jittedPtrs.end() && it->second) {
            // Link: set funcVal.jittedPtr so callFunction uses the JIT path
            if (interp) {
                for (auto& scope : interp->scopes) {
                    auto vIt = scope.find(funcName);
                    if (vIt != scope.end() && vIt->second.type == TzdValue::FUNCTION) {
                        vIt->second.jittedPtr = reinterpret_cast<void(*)(void*,void*)>(it->second);
                        break;
                    }
                }
            }
            return it->second;
        }

        // Check if compilation was already submitted (don't re-submit)
        if (m_compilationAttempted.count(funcName)) {
            return nullptr;
        }

        // Increment call count
        count = ++m_callCounts[funcName];
    }

    // Check if hot enough to trigger JIT compilation (outside lock)
    if (count < m_hotThreshold) {
        return nullptr;
    }

    // Mark as submitted and submit to background tiering engine (non-blocking)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_compilationAttempted.count(funcName)) return nullptr;
        m_compilationAttempted.insert(funcName);
    }

    // Submit async compilation — main thread continues with bytecode
    auto& tiering = TzdTieringEngine::getInstance();
    if (tiering.isRunning()) {
        tiering.submitCompilation(interp, funcName, "", "");
        m_jitCompilations.fetch_add(1, std::memory_order_relaxed);
    } else {
        // Fallback: synchronous compilation if tiering engine not started
        if (interp) {
            for (auto& scope : interp->scopes) {
                auto vIt = scope.find(funcName);
                if (vIt != scope.end()) {
                    TzdValue& funcVal = vIt->second;
                    if (funcVal.type == TzdValue::FUNCTION && funcVal.funcBody) {
                        interp->tryJitCompile(funcVal);
                        if (funcVal.jittedPtr) {
                            m_jittedPtrs[funcName] = (void*)funcVal.jittedPtr;
                            return (void*)funcVal.jittedPtr;
                        }
                    }
                    break;
                }
            }
        }
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
