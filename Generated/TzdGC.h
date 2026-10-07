// ============================================================================
// TzdGC.h - Garbage Collector and Debugger for TzdLang
// ============================================================================

#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>

class TzdInstance;
class TzdInterpreter;
struct TzdValue;

// ============================================================================
// TzdGC.h - Tri-color Mark-Sweep GC with SATB Write Barrier + Concurrent Collection
// ============================================================================

#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <deque>

class TzdInstance;
class TzdInterpreter;
struct TzdValue;
class TzdClassDef;

// ============================================================================
// BumpPointerArena - Simple bump-pointer allocator for young generation
// ============================================================================
class BumpPointerArena {
public:
    static constexpr size_t CHUNK_SIZE = 64 * 1024;  // 64KB chunks

    BumpPointerArena() = default;
    ~BumpPointerArena() { clear(); }

    // Allocate raw memory (bump pointer — O(1), no fragmentation)
    void* allocate(size_t size, size_t alignment = 16);

    // Free all chunks (arena is bulk-freed, not per-object)
    void clear();

    // Statistics
    size_t getAllocatedBytes() const { return m_totalAllocated; }
    size_t getChunkCount() const { return m_chunks.size(); }

private:
    struct Chunk {
        char* base;
        size_t capacity;
        size_t used;
    };
    std::vector<Chunk> m_chunks;
    size_t m_totalAllocated = 0;

    void addChunk(size_t minSize);
};

// ============================================================================
// TzdGarbageCollector - Tri-color mark-sweep with concurrent collection
// ============================================================================
class TzdGarbageCollector {
public:
    static TzdGarbageCollector& getInstance();

    // --- Instance lifecycle ---
    void registerInstance(TzdInstance* inst);
    void unregisterInstance(TzdInstance* inst);

    // --- Generational allocation ---
    // Allocate a TzdInstance in the young generation (bump pointer arena)
    // Falls back to regular new if arena is not suitable
    TzdInstance* allocateInstance(TzdClassDef* def);

    // --- SATB write barrier ---
    // Called BEFORE overwriting a field slot. If GC is in mark phase,
    // the old value is added to the SATB snapshot queue so it survives
    // the current collection cycle.
    void writeBarrier(TzdInstance* holder, TzdValue* slot, const TzdValue& oldVal);

    // --- Collection ---
    // Explicit full collection (stop-the-world)
    void collect(const std::vector<TzdValue*>& rootValues);

    // Auto-collect: check threshold, trigger if exceeded
    void collectIfNeeded(const std::vector<TzdValue*>& rootValues);

    // Lightweight check — does the threshold require a collection?
    bool shouldCollect() const {
        return m_enabled &&
               m_allocSinceLastCollect.load(std::memory_order_relaxed) >= m_autoThreshold;
    }

    // --- Concurrent GC ---
    void startConcurrentGC();
    void stopConcurrentGC();
    void requestConcurrentCollect(const std::vector<TzdValue*>& rootValues);
    bool isCollecting() const { return m_gcPhase.load() != GCPhase::Idle; }

    // --- Generational ---
    void minorCollect(const std::vector<TzdValue*>& rootValues);

    // --- Statistics ---
    size_t getTotalInstances() const;
    size_t getLastCollectedCount() const { return m_lastCollected; }
    size_t getYoungGenCount() const;
    size_t getTotalCollections() const { return m_totalCollections.load(); }

    // --- Configuration ---
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    void setAutoCollectThreshold(size_t threshold) { m_autoThreshold = threshold; }

    // --- Phase tracking (for write barrier) ---
    enum class GCPhase : uint8_t { Idle = 0, Marking = 1, Sweeping = 2 };
    GCPhase getPhase() const { return m_gcPhase.load(std::memory_order_acquire); }

private:
    TzdGarbageCollector();
    ~TzdGarbageCollector();

    // --- Instance registry ---
    std::unordered_set<TzdInstance*> m_instances;
    mutable std::mutex m_mutex;

    // --- Young generation bump-pointer arena ---
    BumpPointerArena m_edenArena;
    std::mutex m_arenaMutex;

    // --- Tri-color marking (BFS worklist, not recursive) ---
    std::deque<TzdInstance*> m_grayWorklist;
    void markValue(const TzdValue& val);
    void markInstance(TzdInstance* inst);
    void processGrayList();

    // --- SATB snapshot queue ---
    std::vector<TzdValue> m_satbQueue;
    std::mutex m_satbMutex;
    void processSatbQueue();

    // --- Sweep ---
    size_t sweep();
    size_t sweepYoungGen();

    // --- Concurrent GC thread ---
    std::thread m_gcThread;
    std::atomic<bool> m_gcThreadRunning{false};
    std::atomic<bool> m_shutdown{false};
    std::condition_variable m_gcCv;
    std::mutex m_gcMutex;
    std::vector<TzdValue*> m_pendingRoots;
    std::atomic<bool> m_collectRequested{false};
    void gcThreadLoop();

    // --- Phase & stats ---
    std::atomic<GCPhase> m_gcPhase{GCPhase::Idle};
    std::atomic<size_t> m_allocSinceLastCollect{0};
    size_t m_lastCollected = 0;
    std::atomic<size_t> m_totalCollections{0};

    // --- Config ---
    bool m_enabled = false;
    size_t m_autoThreshold = 1000000;
};

// ============================================================================
// TzdDebugger - Breakpoint debugging for TzdLang
// ============================================================================
class TzdDebugManager {
public:
    struct Breakpoint {
        std::string file;
        int line;
        bool enabled = true;
        std::string condition; // Optional condition expression
    };

    enum class StepMode {
        None,       // Normal execution
        StepInto,   // Step into next statement
        StepOver,   // Step over next statement (skip function calls)
        StepOut,    // Step out of current function
    };

    static TzdDebugManager& getInstance();

    // Breakpoint management
    int addBreakpoint(const std::string& file, int line);
    bool removeBreakpoint(int id);
    bool toggleBreakpoint(int id);
    void clearBreakpoints();
    std::vector<Breakpoint> getBreakpoints() const;

    // Check if should break at current location
    bool shouldBreak(const std::string& file, int line, int callDepth);

    // Step mode control
    void setStepMode(StepMode mode) { m_stepMode = mode; }
    StepMode getStepMode() const { return m_stepMode; }

    // Call depth tracking for step over/out
    void enterFunction() { m_callDepth++; }
    void exitFunction() { m_callDepth--; }
    int getCallDepth() const { return m_callDepth; }

    // Variable inspection
    std::string inspectVariable(TzdInterpreter* interp, const std::string& name);
    std::vector<std::pair<std::string, std::string>> inspectLocals(TzdInterpreter* interp);
    std::vector<std::pair<std::string, std::string>> inspectGlobals(TzdInterpreter* interp);

    // Debug session control
    void pause() { m_paused = true; }
    void resume() { m_paused = false; m_stepMode = StepMode::None; }
    bool isPaused() const { return m_paused; }

    // Debug output
    void setDebugOutput(bool enabled) { m_debugOutput = enabled; }
    bool hasDebugOutput() const { return m_debugOutput; }

    // Static flag for JIT to check - uses TzdDebugger::g_DebugActive from TzdDebugger.h

private:
    TzdDebugManager() = default;

    std::unordered_map<int, Breakpoint> m_breakpoints;
    std::atomic<int> m_nextBreakpointId{1};
    StepMode m_stepMode = StepMode::None;
    int m_callDepth = 0;
    int m_stepOverDepth = 0;
    std::atomic<bool> m_paused{false};
    bool m_debugOutput = false;
    std::mutex m_mutex;
};
