// ============================================================================
// TzdTieringEngine.cpp - Async JIT thread pool with hotspot detection
// ============================================================================

#include "TzdTieringEngine.h"
#include "TzdInterpreter.h"
#include "TzdJit.h"
#include <iostream>

// ============================================================================
// Singleton
// ============================================================================

TzdTieringEngine& TzdTieringEngine::getInstance() {
    static TzdTieringEngine instance;
    return instance;
}

TzdTieringEngine::~TzdTieringEngine() {
    stop();
}

// ============================================================================
// Lifecycle
// ============================================================================

void TzdTieringEngine::start(size_t numThreads) {
    if (m_running.exchange(true)) return;
    m_shutdown.store(false, std::memory_order_relaxed);
    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back([this]() { workerLoop(); });
    }
}

void TzdTieringEngine::stop() {
    if (!m_running.exchange(false)) return;
    m_shutdown.store(true, std::memory_order_relaxed);
    m_cv.notify_all();
    for (auto& t : m_workers) {
        if (t.joinable()) t.join();
    }
    m_workers.clear();
}

// ============================================================================
// Hotspot tracking
// ============================================================================

void TzdTieringEngine::recordInvocation(const std::string& funcName) {
    std::shared_lock<std::shared_mutex> lock(m_counterMutex);
    auto it = m_invocationCounts.find(funcName);
    if (it != m_invocationCounts.end()) {
        it->second.fetch_add(1, std::memory_order_relaxed);
    } else {
        lock.unlock();
        std::unique_lock<std::shared_mutex> wlock(m_counterMutex);
        m_invocationCounts[funcName].fetch_add(1, std::memory_order_relaxed);
    }
}

void TzdTieringEngine::recordBackedge(const std::string& funcName) {
    std::shared_lock<std::shared_mutex> lock(m_counterMutex);
    auto it = m_backedgeCounts.find(funcName);
    if (it != m_backedgeCounts.end()) {
        it->second.fetch_add(1, std::memory_order_relaxed);
    } else {
        lock.unlock();
        std::unique_lock<std::shared_mutex> wlock(m_counterMutex);
        m_backedgeCounts[funcName].fetch_add(1, std::memory_order_relaxed);
    }
}

bool TzdTieringEngine::isHot(const std::string& funcName) const {
    std::shared_lock<std::shared_mutex> lock(m_counterMutex);
    auto it = m_invocationCounts.find(funcName);
    if (it == m_invocationCounts.end()) return false;
    return it->second.load(std::memory_order_relaxed) >= INVOCATION_THRESHOLD;
}

// ============================================================================
// Async compilation
// ============================================================================

void TzdTieringEngine::submitCompilation(TzdInterpreter* interp,
                                          const std::string& funcName,
                                          const std::string& jitInternalName,
                                          const std::string& funcSourceCode) {
    // Avoid duplicate submissions
    {
        std::lock_guard<std::mutex> lock(m_submittedMutex);
        if (m_submitted.count(jitInternalName)) return;
        m_submitted.insert(jitInternalName);
    }

    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_tasks.push({interp, funcName, jitInternalName, funcSourceCode});
    }
    m_totalSubmits.fetch_add(1, std::memory_order_relaxed);
    m_cv.notify_one();
}

void* TzdTieringEngine::getCompiledPtr(const std::string& jitInternalName) const {
    std::shared_lock<std::shared_mutex> lock(m_ptrMutex);
    auto it = m_compiledPtrs.find(jitInternalName);
    if (it == m_compiledPtrs.end()) return nullptr;
    return it->second;
}

// ============================================================================
// Worker loop
// ============================================================================

void TzdTieringEngine::workerLoop() {
    while (!m_shutdown.load(std::memory_order_relaxed)) {
        CompileTask task;
        {
            std::unique_lock<std::mutex> lock(m_taskMutex);
            m_cv.wait_for(lock, std::chrono::seconds(1),
                [this]() { return !m_tasks.empty() || m_shutdown.load(std::memory_order_relaxed); });
            if (m_shutdown.load(std::memory_order_relaxed)) break;
            if (m_tasks.empty()) continue;
            task = m_tasks.front();
            m_tasks.pop();
        }

        try {
            executeCompileTask(task);
            m_totalCompiled.fetch_add(1, std::memory_order_relaxed);
        } catch (...) {
            m_totalFailed.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void TzdTieringEngine::executeCompileTask(const CompileTask& task) {
    // Background compilation: create an independent LLVMContext,
    // parse the function source, compile to IR, pre-compile to object file.
    //
    // The actual JIT module addition (addObjectFile) is NOT thread-safe
    // with the main thread's JIT operations.  To avoid races, we store
    // the compiled function source and let the main thread handle the
    // final addObjectFile call when it checks getCompiledPtr().
    //
    // For now, this is a stub — the tiering infrastructure is in place
    // but the actual background compilation requires careful integration
    // with the TzdCompiler and LLJIT thread-safety model.

    // TODO: Implement background compilation with:
    // 1. Create independent LLVMContext
    // 2. Parse funcSourceCode with ANTLR
    // 3. Compile with TzdCompiler (needs TzdJitEngine reference)
    // 4. Pre-compile to object buffer (using JIT's getCompiler())
    // 5. Store object buffer for main thread to add via addObjectFile

    // For now, just record that compilation was attempted
    (void)task;
}
