// ============================================================================
// TzdTieringEngine.cpp - Async JIT thread pool with hotspot detection
// ============================================================================

#include "TzdTieringEngine.h"
#include "TzdInterpreter.h"
#include "TzdJit.h"
#include "TzdBytecodeJIT.h"
#include "TzdOop.h"
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
    // Background compilation for free functions AND class methods.
    // For free functions: search interpreter scopes for funcBody/params.
    // For class methods: search class definitions, prepend "this" to params.

    // Try free function first (search interpreter scopes)
    TzdLangParser::BlockContext* funcBody = nullptr;
    std::vector<std::string> params;
    std::string compileName = task.funcName;

    if (task.interp) {
        for (auto& scope : task.interp->scopes) {
            auto it = scope.find(task.funcName);
            if (it != scope.end()) {
                TzdValue& fv = it->second;
                if (fv.type == TzdValue::FUNCTION && fv.funcBody) {
                    funcBody = fv.funcBody;
                    params = fv.params;
                }
                break;
            }
        }
    }

    // If not found in scopes, try class method (ClassName_MethodName)
    if (!funcBody) {
        size_t sep = task.funcName.find('_');
        if (sep != std::string::npos && sep > 0) {
            std::string className = task.funcName.substr(0, sep);
            std::string methodName = task.funcName.substr(sep + 1);
            TzdClassDef* cls = TzdOopManager::getClass(className);
            if (cls && cls->methods.count(methodName)) {
                ClassMethod& m = cls->methods[methodName];
                if (m.body) {
                    funcBody = m.body;
                    // Prepend "this" — methods access instance via rt_get_arg(0)
                    params.push_back("this");
                    for (auto& p : m.params) params.push_back(p);
                }
            }
        }
    }

    if (!funcBody || !task.interp) return;

    // Compile in the background (thread-safe, independent LLVMContext)
    void* jitPtr = task.interp->compileFunctionInBackground(
        compileName, funcBody, params);

    if (jitPtr) {
        // Store in TzdBytecodeJIT's thread-safe map
        TzdBytecodeJIT::getInstance().storeJittedPtr(task.funcName, jitPtr);
    }
}
