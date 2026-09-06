#pragma once

// ============================================================================
// TzdTieringEngine.h - Async JIT thread pool with hotspot detection
// ============================================================================
// Provides tiered compilation: functions start in the interpreter/bytecode
// VM (Tier 0), and are promoted to LLVM JIT (Tier 1) asynchronously when
// they become hot (invocation count exceeds threshold).
// ============================================================================

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <unordered_set>

class TzdInterpreter;
class TzdJitEngine;

class TzdTieringEngine {
public:
    static TzdTieringEngine& getInstance();

    // --- Lifecycle ---
    void start(size_t numThreads = 2);
    void stop();
    bool isRunning() const { return m_running.load(std::memory_order_relaxed); }

    // --- Hotspot tracking ---
    void recordInvocation(const std::string& funcName);
    void recordBackedge(const std::string& funcName);
    bool isHot(const std::string& funcName) const;

    // --- Async compilation ---
    // Submit a function for background JIT compilation.
    // The function source is re-parsed on the background thread.
    void submitCompilation(TzdInterpreter* interp,
                           const std::string& funcName,
                           const std::string& jitInternalName,
                           const std::string& funcSourceCode);

    // Check if a JIT'd pointer is available for a function
    void* getCompiledPtr(const std::string& jitInternalName) const;

    // --- Statistics ---
    struct Stats {
        uint64_t totalSubmits = 0;
        uint64_t totalCompiled = 0;
        uint64_t totalFailed = 0;
    };
    Stats getStats() const {
        Stats s;
        s.totalSubmits = m_totalSubmits.load(std::memory_order_relaxed);
        s.totalCompiled = m_totalCompiled.load(std::memory_order_relaxed);
        s.totalFailed = m_totalFailed.load(std::memory_order_relaxed);
        return s;
    }

    // --- Configuration ---
    static constexpr uint32_t INVOCATION_THRESHOLD = 50;
    static constexpr uint32_t BACKEDGE_THRESHOLD = 1000;

private:
    TzdTieringEngine() = default;
    ~TzdTieringEngine();

    // --- Worker threads ---
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdown{false};

    // --- Task queue ---
    struct CompileTask {
        TzdInterpreter* interp;
        std::string funcName;
        std::string jitInternalName;
        std::string funcSourceCode;
    };
    std::queue<CompileTask> m_tasks;
    mutable std::mutex m_taskMutex;
    std::condition_variable m_cv;

    // --- Compiled pointer cache ---
    std::unordered_map<std::string, void*> m_compiledPtrs;
    mutable std::shared_mutex m_ptrMutex;

    // --- Hotspot counters ---
    std::unordered_map<std::string, std::atomic<uint32_t>> m_invocationCounts;
    std::unordered_map<std::string, std::atomic<uint32_t>> m_backedgeCounts;
    mutable std::shared_mutex m_counterMutex;

    // --- Already-compiled tracking (avoid re-submission) ---
    std::unordered_set<std::string> m_submitted;
    mutable std::mutex m_submittedMutex;

    // --- Stats ---
    std::atomic<uint64_t> m_totalSubmits{0};
    std::atomic<uint64_t> m_totalCompiled{0};
    std::atomic<uint64_t> m_totalFailed{0};

    // --- Worker loop ---
    void workerLoop();
    void executeCompileTask(const CompileTask& task);
};
