// ============================================================================
// TzdGC.cpp - Garbage Collector and Debugger implementation
// ============================================================================

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "TzdGC.h"
#include "TzdOop.h"
#include "TzdInterpreter.h"
#include "../TzdDebugger.h"
#include <iostream>
#include <sstream>

// ============================================================================
// TzdGC.cpp - Tri-color Mark-Sweep GC with SATB + Concurrent Collection
// ============================================================================

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "TzdGC.h"
#include "TzdOop.h"
#include "TzdInterpreter.h"
#include "../TzdDebugger.h"
#include <iostream>
#include <cstdlib>
#include <new>

// ============================================================================
// BumpPointerArena
// ============================================================================

void BumpPointerArena::addChunk(size_t minSize) {
    size_t capacity = (minSize > CHUNK_SIZE) ? minSize : CHUNK_SIZE;
    char* base = static_cast<char*>(std::malloc(capacity));
    if (!base) throw std::bad_alloc();
    m_chunks.push_back({base, capacity, 0});
}

void* BumpPointerArena::allocate(size_t size, size_t alignment) {
    // Try existing chunks first
    for (auto& chunk : m_chunks) {
        size_t aligned = (chunk.used + alignment - 1) & ~(alignment - 1);
        if (aligned + size <= chunk.capacity) {
            void* ptr = chunk.base + aligned;
            chunk.used = aligned + size;
            m_totalAllocated += size;
            return ptr;
        }
    }
    // Need a new chunk
    addChunk(size + alignment);
    auto& chunk = m_chunks.back();
    size_t aligned = (chunk.used + alignment - 1) & ~(alignment - 1);
    chunk.used = aligned + size;
    m_totalAllocated += size;
    return chunk.base + aligned;
}

void BumpPointerArena::clear() {
    for (auto& chunk : m_chunks) {
        std::free(chunk.base);
    }
    m_chunks.clear();
    m_totalAllocated = 0;
}

// ============================================================================
// TzdGarbageCollector
// ============================================================================

TzdGarbageCollector& TzdGarbageCollector::getInstance() {
    static TzdGarbageCollector instance;
    return instance;
}

TzdGarbageCollector::TzdGarbageCollector() {
    // Start concurrent GC thread if enabled
    if (m_enabled) startConcurrentGC();
}

TzdGarbageCollector::~TzdGarbageCollector() {
    stopConcurrentGC();
}

// --- Instance lifecycle ---

void TzdGarbageCollector::registerInstance(TzdInstance* inst) {
    if (!inst || !m_enabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_instances.insert(inst);
    m_allocSinceLastCollect.fetch_add(1, std::memory_order_relaxed);
}

void TzdGarbageCollector::unregisterInstance(TzdInstance* inst) {
    if (!inst || !m_enabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_instances.erase(inst);
}

// --- Generational allocation ---

TzdInstance* TzdGarbageCollector::allocateInstance(TzdClassDef* def) {
    // Allocate from the young-gen bump-pointer arena using placement new
    // The arena provides O(1) allocation with no fragmentation.
    std::lock_guard<std::mutex> lock(m_arenaMutex);
    void* mem = m_edenArena.allocate(sizeof(TzdInstance), alignof(TzdInstance));
    TzdInstance* inst = new (mem) TzdInstance(def);  // placement new
    // Mark as young generation
    inst->gcSetGeneration(TzdInstance::Gen::Young);
    return inst;
}

// --- SATB write barrier ---

void TzdGarbageCollector::writeBarrier(TzdInstance* holder, TzdValue* slot, const TzdValue& oldVal) {
    // Only during marking phase: snapshot the old value so it survives this cycle
    if (getPhase() != GCPhase::Marking) return;
    if (oldVal.type == TzdValue::INSTANCE && oldVal.instanceVal) {
        std::lock_guard<std::mutex> lock(m_satbMutex);
        m_satbQueue.push_back(oldVal);  // copy (retains instanceVal)
    } else if (oldVal.type == TzdValue::ARRAY || oldVal.type == TzdValue::MAP) {
        std::lock_guard<std::mutex> lock(m_satbMutex);
        m_satbQueue.push_back(oldVal);
    }
}

// --- Tri-color marking (BFS worklist) ---

void TzdGarbageCollector::markValue(const TzdValue& val) {
    if (val.type == TzdValue::INSTANCE && val.instanceVal) {
        markInstance(val.instanceVal);
    } else if (val.type == TzdValue::ARRAY) {
        for (const auto& elem : val.arrVal) markValue(elem);
    } else if (val.type == TzdValue::MAP) {
        for (const auto& [key, v] : val.mapVal) markValue(v);
    }
}

void TzdGarbageCollector::markInstance(TzdInstance* inst) {
    if (!inst) return;
    // Gray = enqueued for scanning; Black = scanned. Only White → Gray.
    auto expected = (uint8_t)TzdInstance::GCColor::White;
    if (inst->gcColor.compare_exchange_strong(expected, (uint8_t)TzdInstance::GCColor::Gray,
            std::memory_order_acq_rel)) {
        m_grayWorklist.push_back(inst);
    }
}

void TzdGarbageCollector::processGrayList() {
    while (!m_grayWorklist.empty()) {
        TzdInstance* inst = m_grayWorklist.front();
        m_grayWorklist.pop_front();

        if (!inst) continue;
        inst->gcMarkBlack();  // mark as scanned

        // Scan all fields — children get marked Gray
        for (const auto& fv : inst->fieldValues) {
            markValue(fv);
        }
    }
}

void TzdGarbageCollector::processSatbQueue() {
    std::lock_guard<std::mutex> lock(m_satbMutex);
    for (const auto& val : m_satbQueue) {
        markValue(val);
    }
    m_satbQueue.clear();
}

// --- Sweep ---

size_t TzdGarbageCollector::sweep() {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t collected = 0;

    for (auto it = m_instances.begin(); it != m_instances.end(); ) {
        TzdInstance* inst = *it;
        if (inst && inst->gcGetColor() == TzdInstance::GCColor::White) {
            // Unreachable: collect only if refcount is also 0
            if (inst->refCount.load(std::memory_order_relaxed) <= 0) {
                m_instances.erase(it++);
                // Unlock briefly to avoid deadlock if ~TzdInstance touches GC
                m_mutex.unlock();
                delete inst;
                m_mutex.lock();
                collected++;
            } else {
                // Refcount > 0 — keep alive, mark for next cycle
                inst->gcClearMark();
                ++it;
            }
        } else {
            inst->gcClearMark();  // reset to White for next cycle
            ++it;
        }
    }

    m_lastCollected = collected;
    m_allocSinceLastCollect.store(0, std::memory_order_relaxed);
    m_totalCollections.fetch_add(1, std::memory_order_relaxed);
    return collected;
}

size_t TzdGarbageCollector::sweepYoungGen() {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t collected = 0;

    for (auto it = m_instances.begin(); it != m_instances.end(); ) {
        TzdInstance* inst = *it;
        if (inst && inst->gcGetGeneration() == TzdInstance::Gen::Young) {
            if (inst->gcGetColor() == TzdInstance::GCColor::White &&
                inst->refCount.load(std::memory_order_relaxed) <= 0) {
                m_instances.erase(it++);
                m_mutex.unlock();
                delete inst;
                m_mutex.lock();
                collected++;
            } else {
                // Survive minor GC → promote to Survivor
                inst->gcSetGeneration(TzdInstance::Gen::Survivor);
                inst->gcClearMark();
                ++it;
            }
        } else {
            inst->gcClearMark();
            ++it;
        }
    }

    m_lastCollected = collected;
    m_allocSinceLastCollect.store(0, std::memory_order_relaxed);
    return collected;
}

// --- Collection entry points ---

void TzdGarbageCollector::collect(const std::vector<TzdValue*>& rootValues) {
    if (!m_enabled) return;

    m_gcPhase.store(GCPhase::Marking, std::memory_order_release);

    // 1. Mark roots
    for (TzdValue* val : rootValues) {
        if (val) markValue(*val);
    }

    // 2. Process SATB queue (snapshot values written during marking)
    processSatbQueue();

    // 3. Process gray worklist (BFS — no recursion, no stack overflow)
    processGrayList();

    // 4. Sweep
    m_gcPhase.store(GCPhase::Sweeping, std::memory_order_release);
    sweep();

    m_gcPhase.store(GCPhase::Idle, std::memory_order_release);
}

void TzdGarbageCollector::collectIfNeeded(const std::vector<TzdValue*>& rootValues) {
    if (!m_enabled) return;
    if (m_allocSinceLastCollect.load(std::memory_order_relaxed) >= m_autoThreshold) {
        // Minor collect (young gen only) for small thresholds
        minorCollect(rootValues);
    }
}

void TzdGarbageCollector::minorCollect(const std::vector<TzdValue*>& rootValues) {
    if (!m_enabled) return;

    // Lightweight: mark only, then sweep young gen
    m_gcPhase.store(GCPhase::Marking, std::memory_order_release);

    for (TzdValue* val : rootValues) {
        if (val) markValue(*val);
    }
    processSatbQueue();
    processGrayList();

    m_gcPhase.store(GCPhase::Sweeping, std::memory_order_release);
    sweepYoungGen();

    m_gcPhase.store(GCPhase::Idle, std::memory_order_release);
}

// --- Concurrent GC ---

void TzdGarbageCollector::startConcurrentGC() {
    if (m_gcThreadRunning.exchange(true)) return;
    m_shutdown.store(false, std::memory_order_relaxed);
    m_gcThread = std::thread([this]() { gcThreadLoop(); });
}

void TzdGarbageCollector::stopConcurrentGC() {
    m_shutdown.store(true, std::memory_order_relaxed);
    m_gcCv.notify_all();
    if (m_gcThread.joinable()) m_gcThread.join();
    m_gcThreadRunning.store(false, std::memory_order_relaxed);
}

void TzdGarbageCollector::requestConcurrentCollect(const std::vector<TzdValue*>& rootValues) {
    {
        std::lock_guard<std::mutex> lock(m_gcMutex);
        m_pendingRoots = rootValues;
    }
    m_collectRequested.store(true, std::memory_order_release);
    m_gcCv.notify_one();
}

void TzdGarbageCollector::gcThreadLoop() {
    while (!m_shutdown.load(std::memory_order_relaxed)) {
        // Wait for collect request or timeout
        {
            std::unique_lock<std::mutex> lock(m_gcMutex);
            m_gcCv.wait_for(lock, std::chrono::seconds(5),
                [this]() { return m_collectRequested.load(std::memory_order_relaxed) ||
                                  m_shutdown.load(std::memory_order_relaxed); });
        }

        if (m_shutdown.load(std::memory_order_relaxed)) break;
        if (!m_collectRequested.exchange(false, std::memory_order_acq_rel)) continue;

        std::vector<TzdValue*> roots;
        {
            std::lock_guard<std::mutex> lock(m_gcMutex);
            roots = m_pendingRoots;
        }

        // Run concurrent collection
        if (m_enabled && !roots.empty()) {
            collect(roots);
        }
    }
}

// --- Statistics ---

size_t TzdGarbageCollector::getTotalInstances() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_instances.size();
}

size_t TzdGarbageCollector::getYoungGenCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (auto* inst : m_instances) {
        if (inst && inst->gcGetGeneration() == TzdInstance::Gen::Young) count++;
    }
    return count;
}

// ============================================================================
// TzdDebugger
// ============================================================================
TzdDebugManager& TzdDebugManager::getInstance() {
    static TzdDebugManager instance;
    return instance;
}

int TzdDebugManager::addBreakpoint(const std::string& file, int line) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int id = m_nextBreakpointId.fetch_add(1, std::memory_order_relaxed);
    m_breakpoints[id] = {file, line, true, ""};
    TzdDebugger::g_DebugActive = true;
    return id;
}

bool TzdDebugManager::removeBreakpoint(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_breakpoints.find(id);
    if (it == m_breakpoints.end()) return false;
    m_breakpoints.erase(it);
    if (m_breakpoints.empty()) {
        TzdDebugger::g_DebugActive = false;
    }
    return true;
}

bool TzdDebugManager::toggleBreakpoint(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_breakpoints.find(id);
    if (it == m_breakpoints.end()) return false;
    it->second.enabled = !it->second.enabled;
    return it->second.enabled;
}

void TzdDebugManager::clearBreakpoints() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_breakpoints.clear();
    TzdDebugger::g_DebugActive = false;
}

std::vector<TzdDebugManager::Breakpoint> TzdDebugManager::getBreakpoints() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    std::vector<Breakpoint> result;
    for (const auto& [id, bp] : m_breakpoints) {
        result.push_back(bp);
    }
    return result;
}

bool TzdDebugManager::shouldBreak(const std::string& file, int line, int callDepth) {
    // Check step mode
    if (m_stepMode == StepMode::StepInto) {
        m_paused.store(true, std::memory_order_relaxed);
        return true;
    }
    if (m_stepMode == StepMode::StepOver && callDepth <= m_stepOverDepth) {
        m_paused.store(true, std::memory_order_relaxed);
        return true;
    }
    if (m_stepMode == StepMode::StepOut && callDepth < m_stepOverDepth) {
        m_paused.store(true, std::memory_order_relaxed);
        return true;
    }

    // Check breakpoints
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [id, bp] : m_breakpoints) {
        if (!bp.enabled) continue;
        // Match by line number (file matching is optional since paths may differ)
        if (bp.line == line) {
            m_paused.store(true, std::memory_order_relaxed);
            return true;
        }
    }
    return false;
}

std::string TzdDebugManager::inspectVariable(TzdInterpreter* interp, const std::string& name) {
    if (!interp) return "<no interpreter>";
    // Search scopes from innermost to outermost
    for (auto it = interp->scopes.rbegin(); it != interp->scopes.rend(); ++it) {
        auto valIt = it->find(name);
        if (valIt != it->end()) {
            std::ostringstream ss;
            ss << name << " = " << interp->getAsString(valIt->second);
            return ss.str();
        }
    }
    return name + " = <undefined>";
}

std::vector<std::pair<std::string, std::string>> TzdDebugManager::inspectLocals(TzdInterpreter* interp) {
    std::vector<std::pair<std::string, std::string>> result;
    if (!interp || interp->scopes.empty()) return result;

    // Get the innermost scope (current function's locals)
    const auto& scope = interp->scopes.back();
    for (const auto& [name, val] : scope) {
        result.push_back({name, interp->getAsString(val)});
    }
    return result;
}

std::vector<std::pair<std::string, std::string>> TzdDebugManager::inspectGlobals(TzdInterpreter* interp) {
    std::vector<std::pair<std::string, std::string>> result;
    if (!interp || interp->scopes.empty()) return result;

    // Global scope is scopes[0]
    const auto& scope = interp->scopes[0];
    for (const auto& [name, val] : scope) {
        result.push_back({name, interp->getAsString(val)});
    }
    return result;
}
