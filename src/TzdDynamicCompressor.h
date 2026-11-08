#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
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

enum class TzdCompressionEventType {
    START = 1,
    FINISH = 2,
    OBJECT_COMPRESSED = 3,
    OBJECT_DECOMPRESSED = 4
};

struct TzdCompressionEventInfo {
    TzdCompressionEventType type;
    void* instance = nullptr;
    std::string className;
    size_t originalBytes = 0;
    size_t compressedBytes = 0;
    size_t bytesSaved = 0;
    size_t nonDefaultFields = 0;
    TzdCompressionStats stats;
};

class TzdDynamicCompressor {
public:
    static TzdDynamicCompressor& instance();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // Event Listener management
    using ListenerFn = std::function<void(const TzdCompressionEventInfo&)>;
    uint64_t addListener(ListenerFn listener);
    void removeListener(uint64_t id);
    void clearListeners();

    // Event notification triggers
    void notifyCompressionStart();
    void notifyCompressionFinish(const TzdCompressionStats& stats);
    void notifyObjectCompressed(TzdInstance* inst, const char* className, size_t origBytes, size_t compBytes, size_t savedBytes, size_t nonDefaultFields);
    void notifyObjectDecompressed(TzdInstance* inst, const char* className);

    // Global String Deduplication & Interning pool (Theoretical limit packing)
    const std::string* internString(const std::string& str);
    void clearStringPool();

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

    // Listeners state
    std::mutex m_listenerMutex;
    uint64_t m_nextListenerId = 1;
    std::vector<std::pair<uint64_t, ListenerFn>> m_listeners;
    std::atomic<bool> m_hasListeners{false};

    // Shared interned string storage
    std::mutex m_stringPoolMutex;
    std::unordered_set<std::string> m_stringPool;
};
