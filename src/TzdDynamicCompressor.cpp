#include "TzdDynamicCompressor.h"
#include "Generated/TzdOop.h"
#include "Generated/TzdInterpreter.h"
#include "Generated/TzdGC.h"
#include <algorithm>
#include <iostream>

TzdDynamicCompressor& TzdDynamicCompressor::instance() {
    static TzdDynamicCompressor s_instance;
    return s_instance;
}

uint64_t TzdDynamicCompressor::addListener(ListenerFn listener) {
    if (!listener) return 0;
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    uint64_t id = m_nextListenerId++;
    m_listeners.emplace_back(id, std::move(listener));
    m_hasListeners.store(true, std::memory_order_release);
    return id;
}

void TzdDynamicCompressor::removeListener(uint64_t id) {
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    auto it = std::remove_if(m_listeners.begin(), m_listeners.end(), [id](const auto& pair) {
        return pair.first == id;
    });
    m_listeners.erase(it, m_listeners.end());
    m_hasListeners.store(!m_listeners.empty(), std::memory_order_release);
}

void TzdDynamicCompressor::clearListeners() {
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    m_listeners.clear();
    m_hasListeners.store(false, std::memory_order_release);
}

void TzdDynamicCompressor::notifyCompressionStart() {
    if (!m_hasListeners.load(std::memory_order_relaxed)) return;
    TzdCompressionEventInfo evt;
    evt.type = TzdCompressionEventType::START;
    std::vector<ListenerFn> cbs;
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        for (const auto& [id, fn] : m_listeners) {
            if (fn) cbs.push_back(fn);
        }
    }
    for (const auto& cb : cbs) {
        cb(evt);
    }
}

void TzdDynamicCompressor::notifyCompressionFinish(const TzdCompressionStats& stats) {
    if (!m_hasListeners.load(std::memory_order_relaxed)) return;
    TzdCompressionEventInfo evt;
    evt.type = TzdCompressionEventType::FINISH;
    evt.stats = stats;
    std::vector<ListenerFn> cbs;
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        for (const auto& [id, fn] : m_listeners) {
            if (fn) cbs.push_back(fn);
        }
    }
    for (const auto& cb : cbs) {
        cb(evt);
    }
}

void TzdDynamicCompressor::notifyObjectCompressed(TzdInstance* inst, const char* className,
                                                  size_t origBytes, size_t compBytes,
                                                  size_t savedBytes, size_t nonDefaultFields) {
    if (!m_hasListeners.load(std::memory_order_relaxed)) return;
    TzdCompressionEventInfo evt;
    evt.type = TzdCompressionEventType::OBJECT_COMPRESSED;
    evt.instance = (void*)inst;
    evt.className = className ? className : "Object";
    evt.originalBytes = origBytes;
    evt.compressedBytes = compBytes;
    evt.bytesSaved = savedBytes;
    evt.nonDefaultFields = nonDefaultFields;
    std::vector<ListenerFn> cbs;
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        for (const auto& [id, fn] : m_listeners) {
            if (fn) cbs.push_back(fn);
        }
    }
    for (const auto& cb : cbs) {
        cb(evt);
    }
}

void TzdDynamicCompressor::notifyObjectDecompressed(TzdInstance* inst, const char* className) {
    if (!m_hasListeners.load(std::memory_order_relaxed)) return;
    TzdCompressionEventInfo evt;
    evt.type = TzdCompressionEventType::OBJECT_DECOMPRESSED;
    evt.instance = (void*)inst;
    evt.className = className ? className : "Object";
    std::vector<ListenerFn> cbs;
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        for (const auto& [id, fn] : m_listeners) {
            if (fn) cbs.push_back(fn);
        }
    }
    for (const auto& cb : cbs) {
        cb(evt);
    }
}

const std::string* TzdDynamicCompressor::internString(const std::string& str) {
    std::lock_guard<std::mutex> lock(m_stringPoolMutex);
    auto [it, inserted] = m_stringPool.insert(str);
    return &(*it);
}

void TzdDynamicCompressor::clearStringPool() {
    std::lock_guard<std::mutex> lock(m_stringPoolMutex);
    m_stringPool.clear();
}

size_t TzdDynamicCompressor::compressInstance(TzdInstance* inst) {
    if (!inst) return 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    return inst->compress();
}

void TzdDynamicCompressor::decompressInstance(TzdInstance* inst) {
    if (!inst) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    inst->decompress();
}

size_t TzdDynamicCompressor::compressValue(TzdValue* val, TzdCompressionStats& stats, std::unordered_set<const void*>& visited) {
    if (!val || visited.count(val)) return 0;
    visited.insert(val);
    stats.totalObjectsScanned++;

    size_t saved = 0;

    if (val->type == TzdValue::INSTANCE && val->instanceVal) {
        if (!visited.count(val->instanceVal)) {
            visited.insert(val->instanceVal);
            stats.totalObjectsScanned++;
            if (!val->instanceVal->isCompressed()) {
                size_t before = sizeof(TzdInstance) + val->instanceVal->fieldValues.capacity() * sizeof(TzdValue);
                size_t s = val->instanceVal->compress();
                if (s > 0) {
                    stats.instancesCompressed++;
                    stats.originalBytes += before;
                    size_t after = before > s ? (before - s) : sizeof(TzdInstance);
                    stats.compressedBytes += after;
                    stats.bytesSaved += s;
                    saved += s;
                }
            }
        }
    } else if (val->type == TzdValue::ARRAY) {
        if (val->arrVal.capacity() > val->arrVal.size() + 2) {
            size_t before = val->arrVal.capacity() * sizeof(TzdValue);
            val->arrVal.shrink_to_fit();
            size_t after = val->arrVal.capacity() * sizeof(TzdValue);
            if (before > after) {
                stats.containersShrunk++;
                stats.originalBytes += before;
                stats.compressedBytes += after;
                stats.bytesSaved += (before - after);
                saved += (before - after);
            }
        }
        for (auto& item : val->arrVal) {
            saved += compressValue(&item, stats, visited);
        }
    } else if (val->type == TzdValue::MAP) {
        for (auto& [k, v] : val->mapVal) {
            saved += compressValue(&v, stats, visited);
        }
    } else if (val->type == TzdValue::STRING && val->sVal.capacity() > val->sVal.size() + 16) {
        size_t before = val->sVal.capacity();
        val->sVal.shrink_to_fit();
        size_t after = val->sVal.capacity();
        if (before > after) {
            stats.stringsDeduped++;
            stats.originalBytes += before;
            stats.compressedBytes += after;
            stats.bytesSaved += (before - after);
            saved += (before - after);
        }
    }

    return saved;
}

TzdCompressionStats TzdDynamicCompressor::compressAll(TzdInterpreter* interp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    TzdCompressionStats stats;
    if (!m_enabled) return stats;

    notifyCompressionStart();

    std::unordered_set<const void*> visited;

    // 1. Scan GC registered instances
    std::vector<TzdInstance*> gcInstances;
    TzdGarbageCollector::getInstance().getRegisteredInstances(gcInstances);
    for (auto* inst : gcInstances) {
        if (!inst || visited.count(inst)) continue;
        visited.insert(inst);
        stats.totalObjectsScanned++;

        if (!inst->isCompressed()) {
            size_t before = sizeof(TzdInstance) + inst->fieldValues.capacity() * sizeof(TzdValue);
            size_t s = inst->compress();
            if (s > 0) {
                stats.instancesCompressed++;
                stats.originalBytes += before;
                size_t after = before > s ? (before - s) : sizeof(TzdInstance);
                stats.compressedBytes += after;
                stats.bytesSaved += s;
            }
        }
    }

    // 2. Scan interpreter active scopes
    if (interp) {
        for (auto& scope : interp->scopes) {
            for (auto& [varName, varVal] : scope) {
                compressValue(&varVal, stats, visited);
            }
        }
    }

    if (stats.originalBytes > 0) {
        stats.compressionRatio = ((double)stats.bytesSaved / (double)stats.originalBytes) * 100.0;
    } else {
        stats.compressionRatio = 0.0;
    }

    notifyCompressionFinish(stats);

    return stats;
}
