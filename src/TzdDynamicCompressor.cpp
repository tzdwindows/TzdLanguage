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
        if (val->arrVal.capacity() > val->arrVal.size() + 4) {
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
    }

    return saved;
}

TzdCompressionStats TzdDynamicCompressor::compressAll(TzdInterpreter* interp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    TzdCompressionStats stats;
    if (!m_enabled) return stats;

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

    return stats;
}
