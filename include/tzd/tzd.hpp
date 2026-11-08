#ifndef TZD_CPP_API_HPP
#define TZD_CPP_API_HPP

#include "tzd.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

namespace tzd {

/**
 * @brief Exception thrown on Tzd evaluation or runtime errors
 */
class TzdException : public std::runtime_error {
public:
    int errorCode;
    TzdException(int code, const std::string& msg)
        : std::runtime_error(msg), errorCode(code) {}
};

/**
 * @brief High-level RAII Script Execution Context
 */
class Context {
public:
    Context() : ctx_(tzd_create_context()) {
        if (!ctx_) {
            throw TzdException(TZD_ERR_GENERIC, "Failed to create Tzd context");
        }
    }

    ~Context() {
        if (ctx_) {
            tzd_destroy_context(ctx_);
            ctx_ = nullptr;
        }
    }

    // Non-copyable, movable
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&& other) noexcept : ctx_(other.ctx_) {
        other.ctx_ = nullptr;
    }

    Context& operator=(Context&& other) noexcept {
        if (this != &other) {
            if (ctx_) tzd_destroy_context(ctx_);
            ctx_ = other.ctx_;
            other.ctx_ = nullptr;
        }
        return *this;
    }

    tzd_context_t* raw() const { return ctx_; }

    void reset() {
        tzd_context_reset(ctx_);
    }

    void addIncludePath(const std::string& path) {
        tzd_context_add_include_path(ctx_, path.c_str());
    }

    std::string eval(const std::string& code) {
        char buf[8192] = {0};
        int rc = tzd_eval_string(ctx_, code.c_str(), buf, sizeof(buf));
        if (rc != TZD_OK) {
            const char* err = tzd_context_get_last_error(ctx_);
            throw TzdException(rc, err ? err : "Evaluation failed");
        }
        return std::string(buf);
    }

    void loadFile(const std::string& filepath) {
        int rc = tzd_eval_file(ctx_, filepath.c_str());
        if (rc != TZD_OK) {
            const char* err = tzd_context_get_last_error(ctx_);
            throw TzdException(rc, err ? err : "Failed to load file: " + filepath);
        }
    }

    std::string call(const std::string& funcName, const std::vector<std::string>& args = {}) {
        std::vector<const char*> c_args;
        for (const auto& a : args) c_args.push_back(a.c_str());

        char buf[8192] = {0};
        int rc = tzd_call_function(ctx_, funcName.c_str(),
                                  (int)c_args.size(), c_args.data(),
                                  buf, sizeof(buf));
        if (rc != TZD_OK) {
            const char* err = tzd_context_get_last_error(ctx_);
            throw TzdException(rc, err ? err : "Failed to call function: " + funcName);
        }
        return std::string(buf);
    }

    void setDouble(const std::string& name, double val) {
        tzd_set_global_double(ctx_, name.c_str(), val);
    }

    double getDouble(const std::string& name) {
        double val = 0.0;
        int rc = tzd_get_global_double(ctx_, name.c_str(), &val);
        if (rc != TZD_OK) {
            throw TzdException(rc, "Variable not found or invalid type: " + name);
        }
        return val;
    }

    void setInt(const std::string& name, int64_t val) {
        tzd_set_global_int(ctx_, name.c_str(), val);
    }

    int64_t getInt(const std::string& name) {
        int64_t val = 0;
        int rc = tzd_get_global_int(ctx_, name.c_str(), &val);
        if (rc != TZD_OK) {
            throw TzdException(rc, "Variable not found or invalid type: " + name);
        }
        return val;
    }

    void setString(const std::string& name, const std::string& val) {
        tzd_set_global_string(ctx_, name.c_str(), val.c_str());
    }

    std::string getString(const std::string& name) {
        char buf[8192] = {0};
        int rc = tzd_get_global_string(ctx_, name.c_str(), buf, sizeof(buf));
        if (rc != TZD_OK) {
            throw TzdException(rc, "Variable not found or invalid type: " + name);
        }
        return std::string(buf);
    }

    bool has(const std::string& name) {
        return tzd_has_global(ctx_, name.c_str()) != 0;
    }

    bool retransformClass(const std::string& className, const std::string& classSource) {
        int rc = tzd_retransform_class(ctx_, className.c_str(), classSource.c_str());
        if (rc != TZD_OK) {
            const char* err = tzd_context_get_last_error(ctx_);
            throw TzdException(rc, err ? err : "Retransform class failed: " + className);
        }
        return true;
    }

    size_t getObjectSize(const void* objectOrVal) const {
        size_t sz = 0;
        int rc = tzd_get_object_size(objectOrVal, &sz);
        if (rc != TZD_OK) {
            throw TzdException(rc, "Failed to get object size");
        }
        return sz;
    }

    size_t getVariableSize(const std::string& name) const {
        size_t sz = 0;
        int rc = tzd_context_get_variable_size(ctx_, name.c_str(), &sz);
        if (rc != TZD_OK) {
            throw TzdException(rc, "Failed to get variable size: " + name);
        }
        return sz;
    }

    tzd_context_t* getContext() const {
        return ctx_;
    }

private:
    tzd_context_t* ctx_ = nullptr;
};

/**
 * @brief Global Engine Utilities
 */
class Engine {
public:
    static void init() { tzd_init(); }
    static void shutdown() { tzd_shutdown(); }
    static void setJitEnabled(bool enable) { tzd_set_jit_enabled(enable ? 1 : 0); }
    static bool isJitEnabled() { return tzd_is_jit_enabled() != 0; }
    static void setOptLevel(int level) { tzd_set_opt_level(level); }
    static int getOptLevel() { return tzd_get_opt_level(); }
    static void setSilent(bool silent) { tzd_set_silent_mode(silent ? 1 : 0); }
    static bool isSilent() { return tzd_is_silent_mode() != 0; }
    static void addIncludePath(const std::string& path) { tzd_add_global_include_path(path.c_str()); }
    static void setProjectDirectory(const std::string& path) { tzd_set_project_directory(path.c_str()); }
    static int runFile(const std::string& path) { return tzd_run_file(path.c_str()); }
    static int runString(const std::string& code) { return tzd_run_string(code.c_str()); }
    static int runMain(int argc, char** argv) { return tzd_run_main(argc, argv); }
    static std::string getVersion() { return tzd_get_version_string(); }
};

// =========================================================================
// Thread Management Wrapper
// =========================================================================
struct ThreadInfo {
    uint64_t id = 0;
    uint32_t osThreadId = 0;
    std::string name;
    int state = 0;
    std::string stateStr;
    uint64_t parentId = 0;
    bool isAlive = false;
    bool hasError = false;
    int64_t startTime = 0;
    int64_t endTime = 0;
    int64_t durationMs = 0;
};

class ThreadManager {
public:
    static size_t getCount() {
        return tzd_thread_get_count();
    }

    static std::vector<uint64_t> getAllIds() {
        size_t count = getCount();
        if (count == 0) return {};
        std::vector<uint64_t> ids(count);
        size_t n = tzd_thread_get_all_ids(ids.data(), count);
        ids.resize(n);
        return ids;
    }

    static bool getInfo(uint64_t threadId, ThreadInfo& outInfo) {
        tzd_thread_info_t info{};
        if (tzd_thread_get_info(threadId, &info) != TZD_OK) return false;
        outInfo.id = info.id;
        outInfo.osThreadId = info.os_thread_id;
        outInfo.name = info.name;
        outInfo.state = info.state;
        outInfo.stateStr = info.state_str;
        outInfo.parentId = info.parent_id;
        outInfo.isAlive = info.is_alive != 0;
        outInfo.hasError = info.has_error != 0;
        outInfo.startTime = info.start_time;
        outInfo.endTime = info.end_time;
        outInfo.durationMs = info.duration_ms;
        return true;
    }

    static uint64_t getCurrentId() {
        return tzd_thread_get_current_id();
    }

    static bool pause(uint64_t threadId) {
        return tzd_thread_pause(threadId) == TZD_OK;
    }

    static bool resume(uint64_t threadId) {
        return tzd_thread_resume(threadId) == TZD_OK;
    }

    static bool join(uint64_t threadId, int64_t timeoutMs = -1) {
        return tzd_thread_join(threadId, timeoutMs) == TZD_OK;
    }

    static bool detach(uint64_t threadId) {
        return tzd_thread_detach(threadId) == TZD_OK;
    }

    static std::string getDump() {
        std::vector<char> buf(32768, 0);
        if (tzd_thread_get_dump(buf.data(), buf.size()) == TZD_OK) {
            return std::string(buf.data());
        }
        return "";
    }

    static std::string detectDeadlocks() {
        std::vector<char> buf(16384, 0);
        if (tzd_thread_detect_deadlocks(buf.data(), buf.size()) == TZD_OK) {
            return std::string(buf.data());
        }
        return "";
    }
};

} // namespace tzd

#endif /* TZD_CPP_API_HPP */
