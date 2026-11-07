#include "include/tzd/tzd.h"
#include "include/tzd/tzd_export.h"
#include "include/tzd/tzd_ti.h"
#include "include/tzd/tzd_tni.h"

#include <jni.h>

#include "Generated/TzdInterpreter.h"
#include "Generated/TzdJit.h"
#include "Generated/TzdGC.h"
#include "Generated/TzdOop.h"
#include "Generated/TzdThreadManager.h"
#include "TzdCommandSystem.h"
#include "TzdExeCompiler.h"
#include <colunwind/colunwind.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <windows.h>

#if defined(_MSC_VER)
#include <cstdlib>
extern "C" {
    double (*__imp_atof)(const char*) = atof;
}
#endif

// =========================================================================
// Global & Thread-Local States
// =========================================================================
static thread_local std::string g_lastError;
static bool g_globalJitEnabled = true;
static bool g_silentMode = false;
static std::vector<std::string> g_globalIncludePaths;

struct tzd_context {
    std::unique_ptr<TzdInterpreter> interp;
    std::string lastError;
    std::string lastResult;

    tzd_context() {
        interp = std::make_unique<TzdInterpreter>();
        interp->m_noJit = !g_globalJitEnabled;
        interp->m_silentMode = g_silentMode;
        for (const auto& p : g_globalIncludePaths) {
            interp->addIncludePath(p);
        }
    }
};

// =========================================================================
// Value Formatter Helpers
// =========================================================================
static std::string formatTzdValue(const TzdValue& val) {
    switch (val.type) {
    case TzdValue::NONE:
        return "null";
    case TzdValue::BOOL:
        return val.bVal ? "true" : "false";
    case TzdValue::SBYTE:
    case TzdValue::SHORT:
    case TzdValue::INT:
    case TzdValue::LONG:
        return std::to_string(val.lVal);
    case TzdValue::BYTE:
    case TzdValue::USHORT:
    case TzdValue::UINT:
    case TzdValue::ULONG:
        return std::to_string(val.ulVal);
    case TzdValue::FLOAT:
    case TzdValue::DOUBLE: {
        std::ostringstream ss;
        ss << val.dVal;
        return ss.str();
    }
    case TzdValue::STRING:
    case TzdValue::BIGINT:
        return val.sVal;
    case TzdValue::ARRAY: {
        std::string s = "[";
        for (size_t i = 0; i < val.arrVal.size(); ++i) {
            if (i > 0) s += ", ";
            s += formatTzdValue(val.arrVal[i]);
        }
        s += "]";
        return s;
    }
    case TzdValue::MAP: {
        std::string s = "{";
        bool first = true;
        for (const auto& kv : val.mapVal) {
            if (!first) s += ", ";
            first = false;
            s += "\"" + kv.first + "\": " + formatTzdValue(kv.second);
        }
        s += "}";
        return s;
    }
    case TzdValue::INSTANCE:
        return "<instance>";
    case TzdValue::FUNCTION:
    case TzdValue::NATIVE_FUNCTION:
        return "<function " + val.name + ">";
    default:
        return "<value>";
    }
}

static TzdValue parseStringArg(const char* s) {
    if (!s) return TzdValue();
    std::string str(s);
    if (str == "true") return TzdValue(true);
    if (str == "false") return TzdValue(false);
    if (str == "null") return TzdValue();

    // Check if numeric integer
    char* endp = nullptr;
    long long l = strtoll(s, &endp, 10);
    if (endp && *endp == '\0') {
        return TzdValue(l);
    }

    // Check if double
    double d = strtod(s, &endp);
    if (endp && *endp == '\0') {
        return TzdValue(d);
    }

    return TzdValue(str);
}

static void copyToBuffer(const std::string& src, char* out_buf, size_t out_buf_size) {
    if (!out_buf || out_buf_size == 0) return;
    size_t copyLen = (src.size() < out_buf_size - 1) ? src.size() : (out_buf_size - 1);
    memcpy(out_buf, src.c_str(), copyLen);
    out_buf[copyLen] = '\0';
}

// =========================================================================
// DLL Lifecycle
// =========================================================================
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// =========================================================================
// 1. Version & Metadata
// =========================================================================
TZD_API const char* tzd_get_version_string(void) {
    return "0.2.12";
}

TZD_API int tzd_get_version_major(void) {
    return 0;
}

TZD_API int tzd_get_version_minor(void) {
    return 2;
}

TZD_API int tzd_get_version_patch(void) {
    return 12;
}

TZD_API const char* tzd_get_build_info(void) {
    return "TzdLanguage 0.2.12 (x64, LLVM JIT, PyTorch, CUDA, NTT)";
}

// =========================================================================
// 2. Global Engine Lifecycle
// =========================================================================
TZD_API int tzd_init(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    return TZD_OK;
}

TZD_API void tzd_shutdown(void) {
    // Engine cleanup hook if needed
}

// =========================================================================
// 3. Global Engine Configuration
// =========================================================================
TZD_API void tzd_set_jit_enabled(int enabled) {
    g_globalJitEnabled = (enabled != 0);
}

TZD_API int tzd_is_jit_enabled(void) {
    return g_globalJitEnabled ? 1 : 0;
}

TZD_API void tzd_set_opt_level(int level) {
    TzdJitEngine::setOptLevel(level);
}

TZD_API int tzd_get_opt_level(void) {
    return TzdJitEngine::getConfig().optLevel;
}

TZD_API void tzd_set_silent_mode(int silent) {
    g_silentMode = (silent != 0);
}

TZD_API int tzd_is_silent_mode(void) {
    return g_silentMode ? 1 : 0;
}

TZD_API void tzd_add_global_include_path(const char* path) {
    if (path && *path) {
        g_globalIncludePaths.push_back(std::filesystem::absolute(path).string());
    }
}

TZD_API void tzd_set_project_directory(const char* path) {
    if (path && *path) {
        try {
            std::filesystem::current_path(path);
            tzd_add_global_include_path(path);
        } catch (...) {}
    }
}

// =========================================================================
// 4. One-Shot Script & Main CLI Execution
// =========================================================================
TZD_API int tzd_run_file(const char* filepath) {
    if (!filepath || !*filepath) {
        g_lastError = "Invalid file path";
        return TZD_ERR_INVALID_ARG;
    }
    try {
        TzdInterpreter interp;
        interp.m_noJit = !g_globalJitEnabled;
        for (const auto& p : g_globalIncludePaths) {
            interp.addIncludePath(p);
        }
        interp.m_silentMode = g_silentMode;
        interp.loadScriptFromFile(filepath);

        // If main() is defined in global scope, call it
        if (!interp.scopes.empty() && interp.scopes[0].count("main")) {
            TzdValue mainFunc = interp.scopes[0]["main"];
            if (mainFunc.type == TzdValue::FUNCTION) {
                interp.callFunction(mainFunc, {});
            }
        }
        return TZD_OK;
    }
    catch (const std::exception& e) {
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
    catch (...) {
        g_lastError = "Unknown exception during execution";
        return TZD_ERR_RUNTIME;
    }
}

TZD_API int tzd_run_string(const char* code) {
    if (!code || !*code) {
        g_lastError = "Empty code string";
        return TZD_ERR_INVALID_ARG;
    }
    try {
        TzdInterpreter interp;
        interp.m_noJit = !g_globalJitEnabled;
        for (const auto& p : g_globalIncludePaths) {
            interp.addIncludePath(p);
        }
        interp.m_silentMode = g_silentMode;
        interp.loadScript(code);
        return TZD_OK;
    }
    catch (const std::exception& e) {
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
    catch (...) {
        g_lastError = "Unknown exception during execution";
        return TZD_ERR_RUNTIME;
    }
}

TZD_API int tzd_run_main(int argc, char** argv) {
    // Fix console encoding for UTF-8 Chinese display
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Initialize colunwind crash handler
    colunwind_config_t col_cfg;
    if (colunwind_config_init(&col_cfg) == COLUNWIND_SUCCESS) {
        col_cfg.enable_auto_dump = true;
        col_cfg.dump_directory = ".";
        if (colunwind_init(&col_cfg) == COLUNWIND_SUCCESS) {
            colunwind_install_crash_handler();
        }
    }

    // Check if running as a standalone compiled executable
    int embeddedExitCode = 0;
    if (tzd::TzdExeCompiler::tryRunEmbeddedExecutable(argc, argv, &embeddedExitCode)) {
        colunwind_uninstall_crash_handler();
        colunwind_shutdown();
        return embeddedExitCode;
    }

    if (argc > 1 && strcmp(argv[1], "--test-crash") == 0) {
        volatile int* null_ptr = nullptr;
        *null_ptr = 42;
    }

    TzdCommandSystem system;
    system.start(argc, argv);

    colunwind_uninstall_crash_handler();
    colunwind_shutdown();
    return 0;
}

TZD_API int tzd_try_run_embedded(int argc, char** argv, int* out_exit_code) {
    return tzd::TzdExeCompiler::tryRunEmbeddedExecutable(argc, argv, out_exit_code) ? 1 : 0;
}

TZD_API int tzd_compile_bytecode_file(const char* src_path, const char* out_bc_path) {
    if (!src_path || !out_bc_path) return TZD_ERR_INVALID_ARG;
    try {
        std::ifstream f(src_path);
        if (!f.is_open()) {
            g_lastError = "Cannot open source file: " + std::string(src_path);
            return TZD_ERR_FILE_NOT_FOUND;
        }
        std::string code((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        f.close();

        TzdInterpreter interp;
        if (interp.compileToBytecodeFile(code, out_bc_path)) {
            return TZD_OK;
        } else {
            g_lastError = "Compilation failed due to syntax errors";
            return TZD_ERR_SYNTAX;
        }
    }
    catch (const std::exception& e) {
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
}

TZD_API int tzd_execute_bytecode_file(const char* bc_path) {
    if (!bc_path) return TZD_ERR_INVALID_ARG;
    try {
        TzdInterpreter interp;
        if (interp.executeBytecodeFile(bc_path)) {
            return TZD_OK;
        } else {
            g_lastError = "Bytecode execution failed";
            return TZD_ERR_RUNTIME;
        }
    }
    catch (const std::exception& e) {
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
}

// =========================================================================
// 5. Stateful Execution Context
// =========================================================================
TZD_API tzd_context_t* tzd_create_context(void) {
    try {
        return new tzd_context();
    } catch (...) {
        return nullptr;
    }
}

TZD_API void tzd_destroy_context(tzd_context_t* ctx) {
    if (ctx) {
        delete ctx;
    }
}

TZD_API void tzd_context_reset(tzd_context_t* ctx) {
    if (ctx) {
        ctx->interp = std::make_unique<TzdInterpreter>();
        for (const auto& p : g_globalIncludePaths) {
            ctx->interp->addIncludePath(p);
        }
        ctx->lastError.clear();
        ctx->lastResult.clear();
    }
}

TZD_API void tzd_context_add_include_path(tzd_context_t* ctx, const char* path) {
    if (ctx && ctx->interp && path && *path) {
        ctx->interp->addIncludePath(std::filesystem::absolute(path).string());
    }
}

TZD_API int tzd_eval_string(tzd_context_t* ctx, const char* code,
                            char* out_buf, size_t out_buf_size)
{
    if (!ctx || !ctx->interp || !code) {
        g_lastError = "Invalid context or code";
        return TZD_ERR_INVALID_ARG;
    }
    try {
        ctx->lastError.clear();
        ctx->lastResult.clear();

        std::string scriptCode(code);
        // Automatically append semicolon if single statement without semicolon
        std::string trimmed = scriptCode;
        size_t first = trimmed.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) trimmed.erase(0, first);
        size_t last = trimmed.find_last_not_of(" \t\r\n");
        if (last != std::string::npos) trimmed.erase(last + 1);

        if (!trimmed.empty() && trimmed.back() != ';' && trimmed.back() != '}') {
            scriptCode += ";";
        }

        ctx->interp->loadScript(scriptCode);

        // Check if there is a special variable '__result__' or format last evaluated
        if (!ctx->interp->scopes.empty() && ctx->interp->scopes[0].count("__result__")) {
            ctx->lastResult = formatTzdValue(ctx->interp->scopes[0]["__result__"]);
        }

        if (out_buf && out_buf_size > 0) {
            copyToBuffer(ctx->lastResult, out_buf, out_buf_size);
        }
        return TZD_OK;
    }
    catch (const std::exception& e) {
        ctx->lastError = e.what();
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
    catch (...) {
        ctx->lastError = "Unknown exception during eval";
        g_lastError = ctx->lastError;
        return TZD_ERR_RUNTIME;
    }
}

TZD_API int tzd_eval_file(tzd_context_t* ctx, const char* filepath) {
    if (!ctx || !ctx->interp || !filepath) {
        g_lastError = "Invalid context or file path";
        return TZD_ERR_INVALID_ARG;
    }
    try {
        ctx->lastError.clear();
        ctx->interp->loadScriptFromFile(filepath);
        return TZD_OK;
    }
    catch (const std::exception& e) {
        ctx->lastError = e.what();
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
    catch (...) {
        ctx->lastError = "Unknown exception during file eval";
        g_lastError = ctx->lastError;
        return TZD_ERR_RUNTIME;
    }
}

TZD_API int tzd_call_function(tzd_context_t* ctx, const char* func_name,
                              int argc, const char** argv,
                              char* out_buf, size_t out_buf_size)
{
    if (!ctx || !ctx->interp || !func_name) {
        g_lastError = "Invalid context or function name";
        return TZD_ERR_INVALID_ARG;
    }
    if (ctx->interp->scopes.empty() || !ctx->interp->scopes[0].count(func_name)) {
        ctx->lastError = "Function not found: " + std::string(func_name);
        g_lastError = ctx->lastError;
        return TZD_ERR_NOT_FOUND;
    }

    try {
        ctx->lastError.clear();
        TzdValue funcVal = ctx->interp->scopes[0][func_name];
        std::vector<TzdValue> args;
        for (int i = 0; i < argc; ++i) {
            args.push_back(parseStringArg(argv[i]));
        }

        TzdValue retVal = ctx->interp->callFunction(funcVal, args);
        std::string retStr = formatTzdValue(retVal);
        ctx->lastResult = retStr;

        if (out_buf && out_buf_size > 0) {
            copyToBuffer(retStr, out_buf, out_buf_size);
        }
        return TZD_OK;
    }
    catch (const std::exception& e) {
        ctx->lastError = e.what();
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
}

TZD_API int tzd_set_global_double(tzd_context_t* ctx, const char* name, double val) {
    if (!ctx || !ctx->interp || !name) return TZD_ERR_INVALID_ARG;
    if (ctx->interp->scopes.empty()) ctx->interp->scopes.emplace_back();
    ctx->interp->scopes[0][name] = TzdValue(val);
    return TZD_OK;
}

TZD_API int tzd_get_global_double(tzd_context_t* ctx, const char* name, double* out_val) {
    if (!ctx || !ctx->interp || !name || !out_val) return TZD_ERR_INVALID_ARG;
    if (ctx->interp->scopes.empty() || !ctx->interp->scopes[0].count(name)) {
        return TZD_ERR_NOT_FOUND;
    }
    const auto& val = ctx->interp->scopes[0][name];
    if (val.type == TzdValue::DOUBLE || val.type == TzdValue::FLOAT) {
        *out_val = val.dVal;
        return TZD_OK;
    } else if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::LONG) {
        *out_val = (double)val.lVal;
        return TZD_OK;
    } else if (val.type >= TzdValue::BYTE && val.type <= TzdValue::ULONG) {
        *out_val = (double)val.ulVal;
        return TZD_OK;
    }
    return TZD_ERR_TYPE_MISMATCH;
}

TZD_API int tzd_set_global_int(tzd_context_t* ctx, const char* name, int64_t val) {
    if (!ctx || !ctx->interp || !name) return TZD_ERR_INVALID_ARG;
    if (ctx->interp->scopes.empty()) ctx->interp->scopes.emplace_back();
    ctx->interp->scopes[0][name] = TzdValue((long long)val);
    return TZD_OK;
}

TZD_API int tzd_get_global_int(tzd_context_t* ctx, const char* name, int64_t* out_val) {
    if (!ctx || !ctx->interp || !name || !out_val) return TZD_ERR_INVALID_ARG;
    if (ctx->interp->scopes.empty() || !ctx->interp->scopes[0].count(name)) {
        return TZD_ERR_NOT_FOUND;
    }
    const auto& val = ctx->interp->scopes[0][name];
    if (val.type >= TzdValue::SBYTE && val.type <= TzdValue::LONG) {
        *out_val = val.lVal;
        return TZD_OK;
    } else if (val.type >= TzdValue::BYTE && val.type <= TzdValue::ULONG) {
        *out_val = (int64_t)val.ulVal;
        return TZD_OK;
    } else if (val.type == TzdValue::DOUBLE || val.type == TzdValue::FLOAT) {
        *out_val = (int64_t)val.dVal;
        return TZD_OK;
    }
    return TZD_ERR_TYPE_MISMATCH;
}

TZD_API int tzd_set_global_string(tzd_context_t* ctx, const char* name, const char* val) {
    if (!ctx || !ctx->interp || !name) return TZD_ERR_INVALID_ARG;
    if (ctx->interp->scopes.empty()) ctx->interp->scopes.emplace_back();
    ctx->interp->scopes[0][name] = TzdValue(val ? std::string(val) : std::string(""));
    return TZD_OK;
}

TZD_API int tzd_get_global_string(tzd_context_t* ctx, const char* name,
                                  char* out_buf, size_t out_buf_size)
{
    if (!ctx || !ctx->interp || !name || !out_buf || out_buf_size == 0) return TZD_ERR_INVALID_ARG;
    if (ctx->interp->scopes.empty() || !ctx->interp->scopes[0].count(name)) {
        return TZD_ERR_NOT_FOUND;
    }
    const auto& val = ctx->interp->scopes[0][name];
    std::string s = formatTzdValue(val);
    copyToBuffer(s, out_buf, out_buf_size);
    return TZD_OK;
}

TZD_API int tzd_has_global(tzd_context_t* ctx, const char* name) {
    if (!ctx || !ctx->interp || !name) return 0;
    if (ctx->interp->scopes.empty()) return 0;
    return ctx->interp->scopes[0].count(name) ? 1 : 0;
}

// =========================================================================
// 6. Error Handling
// =========================================================================
TZD_API const char* tzd_get_last_error(void) {
    return g_lastError.c_str();
}

TZD_API const char* tzd_context_get_last_error(tzd_context_t* ctx) {
    if (ctx) return ctx->lastError.c_str();
    return g_lastError.c_str();
}

TZD_API void tzd_clear_last_error(void) {
    g_lastError.clear();
}

// =========================================================================
// 7. JVMTI-style Tool Interface (TTI - Debugging & Profiling Hooks)
// =========================================================================
static std::atomic<uint32_t> g_tzd_ti_event_mask{0};
static tzd_ti_callbacks_t g_tzd_ti_callbacks = {nullptr};
static std::mutex g_ti_mutex;
static std::set<std::pair<std::string, int>> g_breakpoints;

struct tzd_ti_env {
    int placeholder = 0;
};
static tzd_ti_env g_ti_env_instance;

TZD_API tzd_ti_env_t* tzd_ti_get_env(void) {
    return &g_ti_env_instance;
}

TZD_API int tzd_ti_set_callbacks(tzd_ti_env_t* env, const tzd_ti_callbacks_t* callbacks) {
    std::lock_guard<std::mutex> lock(g_ti_mutex);
    if (callbacks) {
        g_tzd_ti_callbacks = *callbacks;
    } else {
        memset(&g_tzd_ti_callbacks, 0, sizeof(g_tzd_ti_callbacks));
    }
    return TZD_OK;
}

TZD_API int tzd_ti_set_event_notification_mode(tzd_ti_env_t* env, uint32_t event_mask, int enable) {
    if (enable) {
        g_tzd_ti_event_mask.fetch_or(event_mask);
    } else {
        g_tzd_ti_event_mask.fetch_and(~event_mask);
    }
    return TZD_OK;
}

TZD_API int tzd_ti_is_event_enabled(uint32_t event_bit) {
    return (g_tzd_ti_event_mask.load(std::memory_order_relaxed) & event_bit) != 0;
}

TZD_API int tzd_ti_set_breakpoint(tzd_ti_env_t* env, const char* file, int line) {
    if (!file) return TZD_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(g_ti_mutex);
    g_breakpoints.insert({std::string(file), line});
    g_tzd_ti_event_mask.fetch_or(TZD_TI_EVENT_BREAKPOINT);
    return TZD_OK;
}

TZD_API int tzd_ti_clear_breakpoint(tzd_ti_env_t* env, const char* file, int line) {
    if (!file) return TZD_ERR_INVALID_ARG;
    std::lock_guard<std::mutex> lock(g_ti_mutex);
    g_breakpoints.erase({std::string(file), line});
    if (g_breakpoints.empty()) {
        g_tzd_ti_event_mask.fetch_and(~TZD_TI_EVENT_BREAKPOINT);
    }
    return TZD_OK;
}

TZD_API int tzd_ti_clear_all_breakpoints(tzd_ti_env_t* env) {
    std::lock_guard<std::mutex> lock(g_ti_mutex);
    g_breakpoints.clear();
    g_tzd_ti_event_mask.fetch_and(~TZD_TI_EVENT_BREAKPOINT);
    return TZD_OK;
}

TZD_API int tzd_ti_get_call_stack(tzd_ti_env_t* env, tzd_context_t* ctx,
                                  tzd_ti_frame_t* frames_out, int max_frames, int* frames_count_out)
{
    if (!frames_out || max_frames <= 0 || !frames_count_out) return TZD_ERR_INVALID_ARG;
    *frames_count_out = 0;

    TzdInterpreter* interp = ctx ? ctx->interp.get() : nullptr;
    if (!interp) return TZD_OK;

    auto snapshot = interp->getCallStackSnapshot();
    int count = 0;
    for (int i = (int)snapshot.size() - 1; i >= 0 && count < max_frames; --i) {
        const std::string& raw = snapshot[i];
        frames_out[count].class_name = _strdup(raw.c_str());
        frames_out[count].method_name = "";
        frames_out[count].file_name = "";
        frames_out[count].line = 0;
        frames_out[count].column = 0;
        frames_out[count].is_jit_compiled = (raw.find("JIT") != std::string::npos) ? 1 : 0;
        count++;
    }
    *frames_count_out = count;
    return TZD_OK;
}

TZD_API int tzd_ti_force_gc(tzd_ti_env_t* env, tzd_context_t* ctx) {
    std::vector<TzdValue*> roots;
    if (ctx && ctx->interp) {
        for (auto& scope : ctx->interp->scopes) {
            for (auto& kv : scope) {
                roots.push_back(&kv.second);
            }
        }
    }
    TzdGarbageCollector::getInstance().collect(roots);
    return TZD_OK;
}

TZD_API void tzd_ti_notify_method_entry(tzd_context_t* ctx, const char* cls, const char* method, const char* file, int line) {
    if ((g_tzd_ti_event_mask.load(std::memory_order_relaxed) & TZD_TI_EVENT_METHOD_ENTRY) && g_tzd_ti_callbacks.on_method_entry) {
        g_tzd_ti_callbacks.on_method_entry(ctx, cls, method, file, line);
    }
}

TZD_API void tzd_ti_notify_method_exit(tzd_context_t* ctx, const char* cls, const char* method, const char* ret_val_str) {
    if ((g_tzd_ti_event_mask.load(std::memory_order_relaxed) & TZD_TI_EVENT_METHOD_EXIT) && g_tzd_ti_callbacks.on_method_exit) {
        g_tzd_ti_callbacks.on_method_exit(ctx, cls, method, ret_val_str);
    }
}

TZD_API void tzd_ti_notify_single_step(tzd_context_t* ctx, const char* file, int line, int column) {
    uint32_t mask = g_tzd_ti_event_mask.load(std::memory_order_relaxed);
    if ((mask & TZD_TI_EVENT_SINGLE_STEP) && g_tzd_ti_callbacks.on_single_step) {
        g_tzd_ti_callbacks.on_single_step(ctx, file, line, column);
    }
    if ((mask & TZD_TI_EVENT_BREAKPOINT) && g_tzd_ti_callbacks.on_breakpoint && file) {
        std::lock_guard<std::mutex> lock(g_ti_mutex);
        if (g_breakpoints.count({std::string(file), line})) {
            g_tzd_ti_callbacks.on_breakpoint(ctx, file, line);
        }
    }
}

TZD_API void tzd_ti_notify_exception_throw(tzd_context_t* ctx, const char* msg, const char* file, int line) {
    if ((g_tzd_ti_event_mask.load(std::memory_order_relaxed) & TZD_TI_EVENT_EXCEPTION_THROW) && g_tzd_ti_callbacks.on_exception_throw) {
        g_tzd_ti_callbacks.on_exception_throw(ctx, msg, file, line);
    }
}

TZD_API void tzd_ti_notify_compiled_method(const char* method, const void* code, size_t size) {
    if ((g_tzd_ti_event_mask.load(std::memory_order_relaxed) & TZD_TI_EVENT_COMPILED_METHOD_LOAD) && g_tzd_ti_callbacks.on_compiled_method_load) {
        g_tzd_ti_callbacks.on_compiled_method_load(method, code, size);
    }
}

TZD_API void tzd_ti_notify_thread_start(tzd_context_t* ctx, uint64_t thread_id, const char* thread_name) {
    if ((g_tzd_ti_event_mask.load(std::memory_order_relaxed) & TZD_TI_EVENT_THREAD_START) && g_tzd_ti_callbacks.on_thread_start) {
        g_tzd_ti_callbacks.on_thread_start(ctx, thread_id, thread_name ? thread_name : "Thread");
    }
}

TZD_API void tzd_ti_notify_thread_end(tzd_context_t* ctx, uint64_t thread_id, const char* thread_name) {
    if ((g_tzd_ti_event_mask.load(std::memory_order_relaxed) & TZD_TI_EVENT_THREAD_END) && g_tzd_ti_callbacks.on_thread_end) {
        g_tzd_ti_callbacks.on_thread_end(ctx, thread_id, thread_name ? thread_name : "Thread");
    }
}

TZD_API int tzd_ti_get_objects_by_class(tzd_ti_env_t* env, tzd_context_t* ctx,
                                        const char* class_name,
                                        void** out_objects, size_t max_count, size_t* out_count)
{
    if (!class_name || !out_count) return TZD_ERR_INVALID_ARG;
    *out_count = 0;
    std::string targetClass = class_name;
    std::unordered_set<TzdInstance*> resultSet;

    auto inspectValue = [&](const TzdValue& val, auto& selfRef) -> void {
        if (val.type == TzdValue::INSTANCE && val.instanceVal) {
            TzdInstance* inst = val.instanceVal;
            if (inst->definition) {
                if (inst->definition->name == targetClass || inst->definition->isSubclassOf(targetClass)) {
                    resultSet.insert(inst);
                }
            }
        } else if (val.type == TzdValue::ARRAY) {
            for (const auto& elem : val.arrVal) {
                selfRef(elem, selfRef);
            }
        } else if (val.type == TzdValue::MAP) {
            for (const auto& kv : val.mapVal) {
                selfRef(kv.second, selfRef);
            }
        }
    };

    auto scanInterpreter = [&](TzdInterpreter* interp) {
        if (!interp) return;
        for (const auto& scope : interp->scopes) {
            for (const auto& kv : scope) {
                inspectValue(kv.second, inspectValue);
            }
        }
    };

    if (ctx && ctx->interp) {
        scanInterpreter(ctx->interp.get());
    }

    auto threadIds = TzdThreading::TzdThreadManager::instance().getAllThreadIds();
    for (uint64_t tid : threadIds) {
        auto tcb = TzdThreading::TzdThreadManager::instance().getThread(tid);
        if (tcb && tcb->childInterp) {
            scanInterpreter(tcb->childInterp);
        }
    }

    size_t count = 0;
    for (TzdInstance* inst : resultSet) {
        if (out_objects && count < max_count) {
            out_objects[count] = (void*)inst;
        }
        count++;
    }
    *out_count = count;
    return TZD_OK;
}

// =========================================================================
// 8. JNI-like Native Interface (TNI)
// =========================================================================
static thread_local tzd_context_t* tl_current_context = nullptr;

static int TNI_GetVersion(tzd_env_t env) {
    return 0x00010000;
}

static tzd_context_t* TNI_GetContext(tzd_env_t env) {
    return tl_current_context;
}

static tzd_class_t TNI_FindClass(tzd_env_t env, const char* name) {
    if (!name) return nullptr;
    return (tzd_class_t)TzdOopManager::getClass(name);
}

static tzd_class_t TNI_GetSuperclass(tzd_env_t env, tzd_class_t clazz) {
    if (!clazz) return nullptr;
    TzdClassDef* cls = (TzdClassDef*)clazz;
    if (cls->parentName.empty()) return nullptr;
    return (tzd_class_t)TzdOopManager::getClass(cls->parentName);
}

static int TNI_IsAssignableFrom(tzd_env_t env, tzd_class_t sub, tzd_class_t sup) {
    if (!sub || !sup) return 0;
    TzdClassDef* c1 = (TzdClassDef*)sub;
    TzdClassDef* c2 = (TzdClassDef*)sup;
    if (c1 == c2) return 1;
    return c1->isSubclassOf(c2->name) ? 1 : 0;
}

static const char* TNI_GetClassName(tzd_env_t env, tzd_class_t clazz) {
    if (!clazz) return "";
    return ((TzdClassDef*)clazz)->name.c_str();
}

static tzd_method_id_t TNI_GetMethodID(tzd_env_t env, tzd_class_t clazz, const char* name, const char* sig) {
    if (!clazz || !name) return nullptr;
    TzdClassDef* cls = (TzdClassDef*)clazz;
    return (tzd_method_id_t)cls->findMethod(name);
}

static tzd_object_t TNI_NewObject(tzd_env_t env, tzd_class_t clazz, int argc, const char** argv) {
    if (!clazz) return nullptr;
    TzdClassDef* cls = (TzdClassDef*)clazz;
    TzdInstance* inst = TzdInstance::create(cls);
    if (!inst) return nullptr;
    inst->retain();

    std::vector<TzdValue> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(parseStringArg(argv[i]));
    }

    ClassConstructor* ctor = cls->findConstructor(args.size());
    if (!ctor && args.empty() && !cls->constructors.empty()) {
        ctor = cls->findConstructor(0);
    }
    if (ctor) {
        tzd_context_t* ctx = env ? env->GetContext(env) : nullptr;
        TzdInterpreter* interp = ctx ? ctx->interp.get() : nullptr;
        std::unique_ptr<TzdInterpreter> localInterp;
        if (!interp) {
            localInterp = std::make_unique<TzdInterpreter>();
            interp = localInterp.get();
        }
        TzdValue ctorFunc(cls->simpleName, ctor->params, ctor->body);
        ctorFunc.paramTypes = ctor->paramTypes;
        ctorFunc.isVararg = ctor->isVararg;
        ctorFunc.jittedPtr = ctor->jittedPtr;
        ctorFunc.setInstance(inst);
        ctorFunc.sourceFile = ctor->sourceFile;
        ctorFunc.line = ctor->line;
        ctorFunc.column = ctor->column;
        interp->callFunction(ctorFunc, args);
    }
    return (tzd_object_t)inst;
}

static TzdValue invokeInstanceMethod(tzd_env_t env, TzdInstance* inst, ClassMethod* m, const std::vector<TzdValue>& args) {
    if (m->isNative) {
        if (m->nativeWrapper) {
            return m->nativeWrapper(args);
        }
        return TzdValue();
    }
    tzd_context_t* ctx = env ? env->GetContext(env) : nullptr;
    TzdInterpreter* interp = ctx ? ctx->interp.get() : nullptr;
    std::unique_ptr<TzdInterpreter> localInterp;
    if (!interp) {
        localInterp = std::make_unique<TzdInterpreter>();
        interp = localInterp.get();
    }
    return interp->callMethod(*m, inst, args);
}

static int TNI_CallMethod(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv, char* out_buf, size_t out_buf_size) {
    if (!obj || !methodID) return TZD_ERR_INVALID_ARG;
    TzdInstance* inst = (TzdInstance*)obj;
    ClassMethod* m = (ClassMethod*)methodID;
    std::vector<TzdValue> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(parseStringArg(argv[i]));
    }
    try {
        TzdValue ret = invokeInstanceMethod(env, inst, m, args);
        std::string s = formatTzdValue(ret);
        copyToBuffer(s, out_buf, out_buf_size);
        return TZD_OK;
    } catch (const std::exception& e) {
        g_lastError = e.what();
        return TZD_ERR_RUNTIME;
    }
}

static double TNI_CallDoubleMethod(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv) {
    if (!obj || !methodID) return 0.0;
    TzdInstance* inst = (TzdInstance*)obj;
    ClassMethod* m = (ClassMethod*)methodID;
    std::vector<TzdValue> args;
    for (int i = 0; i < argc; ++i) args.push_back(parseStringArg(argv[i]));
    TzdValue ret = invokeInstanceMethod(env, inst, m, args);
    return ret.dVal;
}

static int64_t TNI_CallLongMethod(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv) {
    if (!obj || !methodID) return 0;
    TzdInstance* inst = (TzdInstance*)obj;
    ClassMethod* m = (ClassMethod*)methodID;
    std::vector<TzdValue> args;
    for (int i = 0; i < argc; ++i) args.push_back(parseStringArg(argv[i]));
    TzdValue ret = invokeInstanceMethod(env, inst, m, args);
    return ret.lVal;
}

static int TNI_CallStringMethod(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv, char* out_buf, size_t out_buf_size) {
    return TNI_CallMethod(env, obj, methodID, argc, argv, out_buf, out_buf_size);
}

static tzd_field_id_t TNI_GetFieldID(tzd_env_t env, tzd_class_t clazz, const char* name) {
    if (!clazz || !name) return nullptr;
    TzdClassDef* cls = (TzdClassDef*)clazz;
    int idx = cls->getFieldIndex(name);
    if (idx < 0) return nullptr;
    return (tzd_field_id_t)(intptr_t)(idx + 1);
}

static double TNI_GetDoubleField(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID) {
    if (!obj || !fieldID) return 0.0;
    TzdInstance* inst = (TzdInstance*)obj;
    int idx = (int)(intptr_t)fieldID - 1;
    if (idx >= 0 && idx < (int)inst->fieldValues.size()) {
        return inst->fieldValues[idx].dVal;
    }
    return 0.0;
}

static void TNI_SetDoubleField(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, double val) {
    if (!obj || !fieldID) return;
    TzdInstance* inst = (TzdInstance*)obj;
    int idx = (int)(intptr_t)fieldID - 1;
    if (idx >= 0 && idx < (int)inst->fieldValues.size()) {
        inst->fieldValues[idx] = TzdValue(val);
    }
}

static int64_t TNI_GetIntField(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID) {
    if (!obj || !fieldID) return 0;
    TzdInstance* inst = (TzdInstance*)obj;
    int idx = (int)(intptr_t)fieldID - 1;
    if (idx >= 0 && idx < (int)inst->fieldValues.size()) {
        return inst->fieldValues[idx].lVal;
    }
    return 0;
}

static void TNI_SetIntField(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, int64_t val) {
    if (!obj || !fieldID) return;
    TzdInstance* inst = (TzdInstance*)obj;
    int idx = (int)(intptr_t)fieldID - 1;
    if (idx >= 0 && idx < (int)inst->fieldValues.size()) {
        inst->fieldValues[idx] = TzdValue((long long)val);
    }
}

static int TNI_GetStringField(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, char* out_buf, size_t out_buf_size) {
    if (!obj || !fieldID) return TZD_ERR_INVALID_ARG;
    TzdInstance* inst = (TzdInstance*)obj;
    int idx = (int)(intptr_t)fieldID - 1;
    if (idx >= 0 && idx < (int)inst->fieldValues.size()) {
        std::string s = formatTzdValue(inst->fieldValues[idx]);
        copyToBuffer(s, out_buf, out_buf_size);
        return TZD_OK;
    }
    return TZD_ERR_NOT_FOUND;
}

static void TNI_SetStringField(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, const char* val) {
    if (!obj || !fieldID) return;
    TzdInstance* inst = (TzdInstance*)obj;
    int idx = (int)(intptr_t)fieldID - 1;
    if (idx >= 0 && idx < (int)inst->fieldValues.size()) {
        inst->fieldValues[idx] = TzdValue(val ? std::string(val) : std::string(""));
    }
}

static tzd_string_t TNI_NewStringUTF(tzd_env_t env, const char* bytes) {
    return (tzd_string_t)_strdup(bytes ? bytes : "");
}

static const char* TNI_GetStringUTFChars(tzd_env_t env, tzd_string_t str) {
    return (const char*)str;
}

static void TNI_ReleaseStringUTFChars(tzd_env_t env, tzd_string_t str, const char* chars) {
    if (str) free(str);
}

static tzd_array_t TNI_NewArray(tzd_env_t env, size_t initial_capacity) {
    auto* v = new TzdValue();
    v->type = TzdValue::ARRAY;
    v->arrVal.reserve(initial_capacity);
    return (tzd_array_t)v;
}

static size_t TNI_GetArrayLength(tzd_env_t env, tzd_array_t arr) {
    if (!arr) return 0;
    return ((TzdValue*)arr)->arrVal.size();
}

static int TNI_GetArrayStringElement(tzd_env_t env, tzd_array_t arr, size_t index, char* out_buf, size_t out_buf_size) {
    if (!arr) return TZD_ERR_INVALID_ARG;
    TzdValue* v = (TzdValue*)arr;
    if (index >= v->arrVal.size()) return TZD_ERR_NOT_FOUND;
    std::string s = formatTzdValue(v->arrVal[index]);
    copyToBuffer(s, out_buf, out_buf_size);
    return TZD_OK;
}

static void TNI_ArrayPushDouble(tzd_env_t env, tzd_array_t arr, double val) {
    if (!arr) return;
    ((TzdValue*)arr)->arrVal.push_back(TzdValue(val));
}

static void TNI_ArrayPushString(tzd_env_t env, tzd_array_t arr, const char* val) {
    if (!arr) return;
    ((TzdValue*)arr)->arrVal.push_back(TzdValue(val ? std::string(val) : std::string("")));
}

static tzd_map_t TNI_NewMap(tzd_env_t env) {
    auto* v = new TzdValue();
    v->type = TzdValue::MAP;
    return (tzd_map_t)v;
}

static void TNI_MapPutDouble(tzd_env_t env, tzd_map_t map, const char* key, double val) {
    if (!map || !key) return;
    ((TzdValue*)map)->mapVal[key] = TzdValue(val);
}

static void TNI_MapPutString(tzd_env_t env, tzd_map_t map, const char* key, const char* val) {
    if (!map || !key) return;
    ((TzdValue*)map)->mapVal[key] = TzdValue(val ? std::string(val) : std::string(""));
}

static double TNI_MapGetDouble(tzd_env_t env, tzd_map_t map, const char* key) {
    if (!map || !key) return 0.0;
    auto& m = ((TzdValue*)map)->mapVal;
    auto it = m.find(key);
    return (it != m.end()) ? it->second.dVal : 0.0;
}

static int TNI_MapGetString(tzd_env_t env, tzd_map_t map, const char* key, char* out_buf, size_t out_buf_size) {
    if (!map || !key) return TZD_ERR_INVALID_ARG;
    auto& m = ((TzdValue*)map)->mapVal;
    auto it = m.find(key);
    if (it == m.end()) return TZD_ERR_NOT_FOUND;
    std::string s = formatTzdValue(it->second);
    copyToBuffer(s, out_buf, out_buf_size);
    return TZD_OK;
}

typedef tzd_object_t (*tzd_c_native_func_t)(tzd_env_t env, tzd_object_t thiz, int argc, const char** argv);

static int TNI_RegisterNatives(tzd_env_t env, tzd_class_t clazz, const tzd_native_method_t* methods, int nMethods) {
    if (!clazz || !methods) return TZD_ERR_INVALID_ARG;
    TzdClassDef* cls = (TzdClassDef*)clazz;
    for (int i = 0; i < nMethods; ++i) {
        const auto& m = methods[i];
        if (!m.name || !m.fnPtr) continue;
        ClassMethod cm;
        cm.name = m.name;
        cm.declaringClass = cls->name;
        cm.isNative = true;
        auto rawFn = (tzd_c_native_func_t)m.fnPtr;
        cm.nativeWrapper = [rawFn, env](const std::vector<TzdValue>& args) -> TzdValue {
            std::vector<std::string> strArgs;
            std::vector<const char*> argv;
            strArgs.reserve(args.size());
            argv.reserve(args.size());
            for (const auto& a : args) {
                strArgs.push_back(formatTzdValue(a));
            }
            for (const auto& s : strArgs) {
                argv.push_back(s.c_str());
            }
            tzd_object_t retObj = rawFn(env, nullptr, (int)argv.size(), argv.data());
            if (!retObj) return TzdValue();
            return *(TzdValue*)retObj;
        };
        cls->methods[m.name] = cm;
    }
    return TZD_OK;
}

static int TNI_ThrowNew(tzd_env_t env, const char* message) {
    g_lastError = message ? message : "Native error";
    return TZD_OK;
}

static int TNI_ExceptionOccurred(tzd_env_t env) {
    return g_lastError.empty() ? 0 : 1;
}

static const char* TNI_ExceptionDescribe(tzd_env_t env) {
    return g_lastError.c_str();
}

static void TNI_ExceptionClear(tzd_env_t env) {
    g_lastError.clear();
}

static const tzd_native_interface_t g_tni_interface = {
    TNI_GetVersion,
    TNI_GetContext,
    TNI_FindClass,
    TNI_GetSuperclass,
    TNI_IsAssignableFrom,
    TNI_GetClassName,
    TNI_GetMethodID,
    TNI_NewObject,
    TNI_CallMethod,
    TNI_CallDoubleMethod,
    TNI_CallLongMethod,
    TNI_CallStringMethod,
    TNI_GetFieldID,
    TNI_GetDoubleField,
    TNI_SetDoubleField,
    TNI_GetIntField,
    TNI_SetIntField,
    TNI_GetStringField,
    TNI_SetStringField,
    TNI_NewStringUTF,
    TNI_GetStringUTFChars,
    TNI_ReleaseStringUTFChars,
    TNI_NewArray,
    TNI_GetArrayLength,
    TNI_GetArrayStringElement,
    TNI_ArrayPushDouble,
    TNI_ArrayPushString,
    TNI_NewMap,
    TNI_MapPutDouble,
    TNI_MapPutString,
    TNI_MapGetDouble,
    TNI_MapGetString,
    TNI_RegisterNatives,
    TNI_ThrowNew,
    TNI_ExceptionOccurred,
    TNI_ExceptionDescribe,
    TNI_ExceptionClear
};

TZD_API tzd_env_t tzd_get_env(tzd_context_t* ctx) {
    if (ctx) {
        tl_current_context = ctx;
    }
    return &g_tni_interface;
}

// =========================================================================
// 9. Java Native Interface (JNI) Exports for com.tzd.TzdLanguage
// =========================================================================
extern "C" {

JNIEXPORT jstring JNICALL Java_com_tzd_TzdLanguage_getVersion(JNIEnv* env, jclass cls) {
    return env->NewStringUTF(tzd_get_version_string());
}

JNIEXPORT jint JNICALL Java_com_tzd_TzdLanguage_init(JNIEnv* env, jclass cls) {
    return (jint)tzd_init();
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_shutdown(JNIEnv* env, jclass cls) {
    tzd_shutdown();
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_setJitEnabled(JNIEnv* env, jclass cls, jboolean enable) {
    tzd_set_jit_enabled(enable ? 1 : 0);
}

JNIEXPORT jboolean JNICALL Java_com_tzd_TzdLanguage_isJitEnabled(JNIEnv* env, jclass cls) {
    return tzd_is_jit_enabled() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_setOptLevel(JNIEnv* env, jclass cls, jint level) {
    tzd_set_opt_level((int)level);
}

JNIEXPORT jint JNICALL Java_com_tzd_TzdLanguage_getOptLevel(JNIEnv* env, jclass cls) {
    return (jint)tzd_get_opt_level();
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_setSilentMode(JNIEnv* env, jclass cls, jboolean silent) {
    tzd_set_silent_mode(silent ? 1 : 0);
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_addGlobalIncludePath(JNIEnv* env, jclass cls, jstring path) {
    if (!path) return;
    const char* str = env->GetStringUTFChars(path, nullptr);
    tzd_add_global_include_path(str);
    env->ReleaseStringUTFChars(path, str);
}

JNIEXPORT jint JNICALL Java_com_tzd_TzdLanguage_runFile(JNIEnv* env, jclass cls, jstring path) {
    if (!path) return (jint)TZD_ERR_INVALID_ARG;
    const char* str = env->GetStringUTFChars(path, nullptr);
    int rc = tzd_run_file(str);
    env->ReleaseStringUTFChars(path, str);
    return (jint)rc;
}

JNIEXPORT jint JNICALL Java_com_tzd_TzdLanguage_runString(JNIEnv* env, jclass cls, jstring code) {
    if (!code) return (jint)TZD_ERR_INVALID_ARG;
    const char* str = env->GetStringUTFChars(code, nullptr);
    int rc = tzd_run_string(str);
    env->ReleaseStringUTFChars(code, str);
    return (jint)rc;
}

JNIEXPORT jlong JNICALL Java_com_tzd_TzdLanguage_createContext(JNIEnv* env, jclass cls) {
    return (jlong)(uintptr_t)tzd_create_context();
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_destroyContext(JNIEnv* env, jclass cls, jlong handle) {
    if (handle) {
        tzd_destroy_context((tzd_context_t*)(uintptr_t)handle);
    }
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_resetContext(JNIEnv* env, jclass cls, jlong handle) {
    if (handle) {
        tzd_context_reset((tzd_context_t*)(uintptr_t)handle);
    }
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_contextAddIncludePath(JNIEnv* env, jclass cls, jlong handle, jstring path) {
    if (handle && path) {
        const char* str = env->GetStringUTFChars(path, nullptr);
        tzd_context_add_include_path((tzd_context_t*)(uintptr_t)handle, str);
        env->ReleaseStringUTFChars(path, str);
    }
}

JNIEXPORT jstring JNICALL Java_com_tzd_TzdLanguage_eval(JNIEnv* env, jclass cls, jlong handle, jstring code) {
    if (!handle || !code) return nullptr;
    const char* str = env->GetStringUTFChars(code, nullptr);
    char buf[8192] = {0};
    int rc = tzd_eval_string((tzd_context_t*)(uintptr_t)handle, str, buf, sizeof(buf));
    env->ReleaseStringUTFChars(code, str);

    if (rc != TZD_OK) {
        jclass exCls = env->FindClass("com/tzd/TzdException");
        if (exCls) {
            const char* err = tzd_get_last_error();
            env->ThrowNew(exCls, err ? err : "Eval error");
        }
        return nullptr;
    }
    return env->NewStringUTF(buf);
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_loadFile(JNIEnv* env, jclass cls, jlong handle, jstring path) {
    if (!handle || !path) return;
    const char* str = env->GetStringUTFChars(path, nullptr);
    int rc = tzd_eval_file((tzd_context_t*)(uintptr_t)handle, str);
    env->ReleaseStringUTFChars(path, str);

    if (rc != TZD_OK) {
        jclass exCls = env->FindClass("com/tzd/TzdException");
        if (exCls) {
            const char* err = tzd_get_last_error();
            env->ThrowNew(exCls, err ? err : "Load file error");
        }
    }
}

JNIEXPORT jstring JNICALL Java_com_tzd_TzdLanguage_callFunction(JNIEnv* env, jclass cls, jlong handle, jstring funcName, jobjectArray args) {
    if (!handle || !funcName) return nullptr;
    const char* fnStr = env->GetStringUTFChars(funcName, nullptr);

    int argc = args ? env->GetArrayLength(args) : 0;
    std::vector<std::string> argStrings;
    std::vector<const char*> cArgs;
    for (int i = 0; i < argc; ++i) {
        jstring jArg = (jstring)env->GetObjectArrayElement(args, i);
        if (jArg) {
            const char* aStr = env->GetStringUTFChars(jArg, nullptr);
            argStrings.push_back(aStr ? aStr : "");
            env->ReleaseStringUTFChars(jArg, aStr);
        } else {
            argStrings.push_back("");
        }
    }
    for (const auto& s : argStrings) {
        cArgs.push_back(s.c_str());
    }

    char buf[8192] = {0};
    int rc = tzd_call_function((tzd_context_t*)(uintptr_t)handle, fnStr, (int)cArgs.size(), cArgs.data(), buf, sizeof(buf));
    env->ReleaseStringUTFChars(funcName, fnStr);

    if (rc != TZD_OK) {
        jclass exCls = env->FindClass("com/tzd/TzdException");
        if (exCls) {
            const char* err = tzd_get_last_error();
            env->ThrowNew(exCls, err ? err : "Call function error");
        }
        return nullptr;
    }
    return env->NewStringUTF(buf);
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_setDouble(JNIEnv* env, jclass cls, jlong handle, jstring name, jdouble val) {
    if (!handle || !name) return;
    const char* str = env->GetStringUTFChars(name, nullptr);
    tzd_set_global_double((tzd_context_t*)(uintptr_t)handle, str, (double)val);
    env->ReleaseStringUTFChars(name, str);
}

JNIEXPORT jdouble JNICALL Java_com_tzd_TzdLanguage_getDouble(JNIEnv* env, jclass cls, jlong handle, jstring name) {
    if (!handle || !name) return 0.0;
    const char* str = env->GetStringUTFChars(name, nullptr);
    double val = 0.0;
    tzd_get_global_double((tzd_context_t*)(uintptr_t)handle, str, &val);
    env->ReleaseStringUTFChars(name, str);
    return (jdouble)val;
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_setLong(JNIEnv* env, jclass cls, jlong handle, jstring name, jlong val) {
    if (!handle || !name) return;
    const char* str = env->GetStringUTFChars(name, nullptr);
    tzd_set_global_int((tzd_context_t*)(uintptr_t)handle, str, (int64_t)val);
    env->ReleaseStringUTFChars(name, str);
}

JNIEXPORT jlong JNICALL Java_com_tzd_TzdLanguage_getLong(JNIEnv* env, jclass cls, jlong handle, jstring name) {
    if (!handle || !name) return 0;
    const char* str = env->GetStringUTFChars(name, nullptr);
    int64_t val = 0;
    tzd_get_global_int((tzd_context_t*)(uintptr_t)handle, str, &val);
    env->ReleaseStringUTFChars(name, str);
    return (jlong)val;
}

JNIEXPORT void JNICALL Java_com_tzd_TzdLanguage_setString(JNIEnv* env, jclass cls, jlong handle, jstring name, jstring val) {
    if (!handle || !name) return;
    const char* nStr = env->GetStringUTFChars(name, nullptr);
    const char* vStr = val ? env->GetStringUTFChars(val, nullptr) : "";
    tzd_set_global_string((tzd_context_t*)(uintptr_t)handle, nStr, vStr);
    env->ReleaseStringUTFChars(name, nStr);
    if (val) env->ReleaseStringUTFChars(val, vStr);
}

JNIEXPORT jstring JNICALL Java_com_tzd_TzdLanguage_getString(JNIEnv* env, jclass cls, jlong handle, jstring name) {
    if (!handle || !name) return nullptr;
    const char* str = env->GetStringUTFChars(name, nullptr);
    char buf[8192] = {0};
    tzd_get_global_string((tzd_context_t*)(uintptr_t)handle, str, buf, sizeof(buf));
    env->ReleaseStringUTFChars(name, str);
    return env->NewStringUTF(buf);
}

JNIEXPORT jboolean JNICALL Java_com_tzd_TzdLanguage_hasGlobal(JNIEnv* env, jclass cls, jlong handle, jstring name) {
    if (!handle || !name) return JNI_FALSE;
    const char* str = env->GetStringUTFChars(name, nullptr);
    int res = tzd_has_global((tzd_context_t*)(uintptr_t)handle, str);
    env->ReleaseStringUTFChars(name, str);
    return res ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL Java_com_tzd_TzdLanguage_getLastError(JNIEnv* env, jclass cls) {
    return env->NewStringUTF(tzd_get_last_error());
}

} // extern "C"

// =========================================================================
// 10. Thread Management API Implementation
// =========================================================================
TZD_API size_t tzd_thread_get_count(void) {
    return TzdThreading::TzdThreadManager::instance().getAllThreadIds().size();
}

TZD_API size_t tzd_thread_get_all_ids(uint64_t* out_ids, size_t max_count) {
    auto ids = TzdThreading::TzdThreadManager::instance().getAllThreadIds();
    if (!out_ids || max_count == 0) return ids.size();
    size_t n = (std::min)(ids.size(), max_count);
    for (size_t i = 0; i < n; ++i) {
        out_ids[i] = ids[i];
    }
    return n;
}

TZD_API int tzd_thread_get_info(uint64_t thread_id, tzd_thread_info_t* out_info) {
    if (!out_info) return TZD_ERR_INVALID_ARG;
    auto tcb = TzdThreading::TzdThreadManager::instance().getThread(thread_id);
    if (!tcb) return TZD_ERR_NOT_FOUND;

    memset(out_info, 0, sizeof(tzd_thread_info_t));
    out_info->id = tcb->id;
    out_info->os_thread_id = tcb->osThreadId;
    strncpy_s(out_info->name, sizeof(out_info->name), tcb->name.c_str(), _TRUNCATE);
    out_info->state = (int)tcb->state.load();
    std::string stateStr = TzdThreading::threadStateToString(tcb->state.load());
    strncpy_s(out_info->state_str, sizeof(out_info->state_str), stateStr.c_str(), _TRUNCATE);
    out_info->parent_id = tcb->parentThreadId;
    out_info->is_alive = (out_info->state >= 1 && out_info->state <= 4) ? 1 : 0;
    out_info->has_error = tcb->hasError ? 1 : 0;
    out_info->start_time = tcb->startTimestamp;
    out_info->end_time = tcb->endTimestamp;
    int64_t dur = (tcb->endTimestamp > 0 ? tcb->endTimestamp : (tcb->startTimestamp > 0 ? TzdThreading::nowMs() : 0)) - tcb->startTimestamp;
    out_info->duration_ms = dur > 0 ? dur : 0;
    return TZD_OK;
}

TZD_API uint64_t tzd_thread_get_current_id(void) {
    return TzdThreading::TzdThreadManager::instance().getCurrentThreadId();
}

TZD_API int tzd_thread_pause(uint64_t thread_id) {
    auto tcb = TzdThreading::TzdThreadManager::instance().getThread(thread_id);
    if (!tcb) return TZD_ERR_NOT_FOUND;
    tcb->pause();
    return TZD_OK;
}

TZD_API int tzd_thread_resume(uint64_t thread_id) {
    auto tcb = TzdThreading::TzdThreadManager::instance().getThread(thread_id);
    if (!tcb) return TZD_ERR_NOT_FOUND;
    tcb->resume();
    return TZD_OK;
}

TZD_API int tzd_thread_join(uint64_t thread_id, int64_t timeout_ms) {
    auto tcb = TzdThreading::TzdThreadManager::instance().getThread(thread_id);
    if (!tcb) return TZD_ERR_NOT_FOUND;
    return tcb->join(timeout_ms) ? TZD_OK : TZD_ERR_RUNTIME;
}

TZD_API int tzd_thread_detach(uint64_t thread_id) {
    auto tcb = TzdThreading::TzdThreadManager::instance().getThread(thread_id);
    if (!tcb) return TZD_ERR_NOT_FOUND;
    tcb->detach();
    return TZD_OK;
}

TZD_API int tzd_thread_get_dump(char* out_buf, size_t out_buf_size) {
    if (!out_buf || out_buf_size == 0) return TZD_ERR_INVALID_ARG;
    std::string dump = TzdThreading::TzdThreadManager::instance().dumpAllStackTraces();
    copyToBuffer(dump, out_buf, out_buf_size);
    return TZD_OK;
}

TZD_API int tzd_thread_detect_deadlocks(char* out_buf, size_t out_buf_size) {
    if (!out_buf || out_buf_size == 0) return TZD_ERR_INVALID_ARG;
    auto deadlocks = TzdThreading::TzdThreadManager::instance().detectDeadlocks();
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < deadlocks.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "{";
        size_t j = 0;
        for (const auto& kv : deadlocks[i]) {
            if (j > 0) oss << ", ";
            oss << "\"" << kv.first << "\": " << formatTzdValue(kv.second);
            j++;
        }
        oss << "}";
    }
    oss << "]";
    std::string s = oss.str();
    copyToBuffer(s, out_buf, out_buf_size);
    return TZD_OK;
}
