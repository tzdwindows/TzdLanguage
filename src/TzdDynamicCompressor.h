#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <cstdint>

class TzdInstance;
class TzdInterpreter;
struct TzdValue;

struct TzdCompressionStats {
    size_t totalObjectsScanned = 0;
    size_t instancesCompressed = 0;
    size_t stringsDeduped = 0;
    size_t containersShrunk = 0;
    size_t originalBytes = 0;
    size_t compressedBytes = 0;
    size_t bytesSaved = 0;
    double compressionRatio = 0.0; // Percentage: 0.0% to 100.0%
};

class TzdDynamicCompressor {
public:
    static TzdDynamicCompressor& instance();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // Execute dynamic compression across active heap objects and interpreter scopes
    TzdCompressionStats compressAll(TzdInterpreter* interp = nullptr);

    // Compress a single instance (returns bytes saved)
    size_t compressInstance(TzdInstance* inst);

    // Decompress a single instance
    void decompressInstance(TzdInstance* inst);

    // Compress value recursively (arrays, instances, strings)
    size_t compressValue(TzdValue* val, TzdCompressionStats& stats, std::unordered_set<const void*>& visited);

private:
    TzdDynamicCompressor() = default;
    ~TzdDynamicCompressor() = default;

    bool m_enabled = true;
    std::mutex m_mutex;
};
