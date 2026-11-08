#ifndef TZD_C_API_H
#define TZD_C_API_H

#include "tzd_export.h"
#include "tzd_ti.h"
#include "tzd_tni.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/* Error Codes & Status Codes                                                */
/* ------------------------------------------------------------------------- */
typedef enum tzd_status_e {
    TZD_OK                  =  0,
    TZD_ERR_GENERIC         = -1,
    TZD_ERR_SYNTAX          = -2,
    TZD_ERR_RUNTIME         = -3,
    TZD_ERR_FILE_NOT_FOUND  = -4,
    TZD_ERR_INVALID_ARG     = -5,
    TZD_ERR_BUFFER_OVERFLOW = -6,
    TZD_ERR_NOT_FOUND       = -7,
    TZD_ERR_TYPE_MISMATCH   = -8
} tzd_status_t;

/* Opaque Context Handle for isolated scripting environments */
typedef struct tzd_context tzd_context_t;

/* ------------------------------------------------------------------------- */
/* 1. Version & Metadata                                                     */
/* ------------------------------------------------------------------------- */
TZD_API const char* tzd_get_version_string(void);
TZD_API int         tzd_get_version_major(void);
TZD_API int         tzd_get_version_minor(void);
TZD_API int         tzd_get_version_patch(void);
TZD_API const char* tzd_get_build_info(void);

/* ------------------------------------------------------------------------- */
/* 2. Global Engine Lifecycle                                                */
/* ------------------------------------------------------------------------- */
TZD_API int  tzd_init(void);
TZD_API void tzd_shutdown(void);

/* ------------------------------------------------------------------------- */
/* 3. Global Engine Configuration                                            */
/* ------------------------------------------------------------------------- */
TZD_API void tzd_set_jit_enabled(int enabled);
TZD_API int  tzd_is_jit_enabled(void);
TZD_API void tzd_set_opt_level(int level);
TZD_API int  tzd_get_opt_level(void);
TZD_API void tzd_set_silent_mode(int silent);
TZD_API int  tzd_is_silent_mode(void);
TZD_API void tzd_add_global_include_path(const char* path);
TZD_API void tzd_set_project_directory(const char* path);

/* ------------------------------------------------------------------------- */
/* 4. One-Shot Script & Main CLI Execution                                   */
/* ------------------------------------------------------------------------- */
/* Execute a .tzd script file directly. Returns 0 (TZD_OK) on success. */
TZD_API int tzd_run_file(const char* filepath);

/* Execute script source code string directly. Returns 0 (TZD_OK) on success. */
TZD_API int tzd_run_string(const char* code);

/* Run full CLI/Interactive REPL with standard argc/argv arguments. Returns exit code. */
TZD_API int tzd_run_main(int argc, char** argv);

/* Check and run embedded executable if compiled with AOT stub */
TZD_API int tzd_try_run_embedded(int argc, char** argv, int* out_exit_code);

/* Compile script to bytecode file (.tzdc) */
TZD_API int tzd_compile_bytecode_file(const char* src_path, const char* out_bc_path);

/* Execute precompiled bytecode file (.tzdc) */
TZD_API int tzd_execute_bytecode_file(const char* bc_path);

/* ------------------------------------------------------------------------- */
/* 5. Stateful Execution Context                                             */
/* ------------------------------------------------------------------------- */
/* Create a new isolated script context / interpreter instance */
TZD_API tzd_context_t* tzd_create_context(void);

/* Destroy and free a script context */
TZD_API void           tzd_destroy_context(tzd_context_t* ctx);

/* Reset context state (clears variables and resets scopes) */
TZD_API void           tzd_context_reset(tzd_context_t* ctx);

/* Add include/search path for imports in this specific context */
TZD_API void           tzd_context_add_include_path(tzd_context_t* ctx, const char* path);

/* Evaluate a code string in context, writing result to out_buf (can be NULL if not needed) */
TZD_API int            tzd_eval_string(tzd_context_t* ctx, const char* code,
                                       char* out_buf, size_t out_buf_size);

/* Load and execute a script file in context */
TZD_API int            tzd_eval_file(tzd_context_t* ctx, const char* filepath);

/* Call a global function by name with string arguments */
TZD_API int            tzd_call_function(tzd_context_t* ctx, const char* func_name,
                                         int argc, const char** argv,
                                         char* out_buf, size_t out_buf_size);

/* Global variable access in context */
TZD_API int            tzd_set_global_double(tzd_context_t* ctx, const char* name, double val);
TZD_API int            tzd_get_global_double(tzd_context_t* ctx, const char* name, double* out_val);
TZD_API int            tzd_set_global_int(tzd_context_t* ctx, const char* name, int64_t val);
TZD_API int            tzd_get_global_int(tzd_context_t* ctx, const char* name, int64_t* out_val);
TZD_API int            tzd_set_global_string(tzd_context_t* ctx, const char* name, const char* val);
TZD_API int            tzd_get_global_string(tzd_context_t* ctx, const char* name,
                                             char* out_buf, size_t out_buf_size);
TZD_API int            tzd_has_global(tzd_context_t* ctx, const char* name);

/* ------------------------------------------------------------------------- */
/* 5.1 Dynamic Class Hot-Swap and Object Introspection                      */
/* ------------------------------------------------------------------------- */
/* Retransform/redefine a registered class dynamically at runtime with new source */
TZD_API int            tzd_retransform_class(tzd_context_t* ctx, const char* class_name, const char* class_source);

/* Calculate the memory size (in bytes) of an object or value */
TZD_API int            tzd_get_object_size(const void* object_or_val, size_t* out_size);
TZD_API int            tzd_context_get_variable_size(tzd_context_t* ctx, const char* name, size_t* out_size);

/* ------------------------------------------------------------------------- */
/* 6. Error Handling                                                         */
/* ------------------------------------------------------------------------- */
/* Returns thread-local last error message */
TZD_API const char* tzd_get_last_error(void);

/* Returns context-specific last error message */
TZD_API const char* tzd_context_get_last_error(tzd_context_t* ctx);

/* Clears thread-local and context error messages */
TZD_API void        tzd_clear_last_error(void);

/* ========================================================================= */
/* 8. Thread Management API                                                  */
/* ========================================================================= */
typedef enum tzd_thread_state_e {
    TZD_THREAD_STATE_NEW        = 0,
    TZD_THREAD_STATE_RUNNING    = 1,
    TZD_THREAD_STATE_BLOCKED    = 2,
    TZD_THREAD_STATE_SLEEPING   = 3,
    TZD_THREAD_STATE_PAUSED     = 4,
    TZD_THREAD_STATE_TERMINATED = 5,
    TZD_THREAD_STATE_FAILED     = 6
} tzd_thread_state_t;

typedef struct tzd_thread_info_s {
    uint64_t id;
    uint32_t os_thread_id;
    char name[128];
    int state;
    char state_str[32];
    uint64_t parent_id;
    int is_alive;
    int has_error;
    int64_t start_time;
    int64_t end_time;
    int64_t duration_ms;
} tzd_thread_info_t;

/* Enumerate threads */
TZD_API size_t   tzd_thread_get_count(void);
TZD_API size_t   tzd_thread_get_all_ids(uint64_t* out_ids, size_t max_count);
TZD_API int      tzd_thread_get_info(uint64_t thread_id, tzd_thread_info_t* out_info);
TZD_API uint64_t tzd_thread_get_current_id(void);

/* Thread control */
TZD_API int      tzd_thread_pause(uint64_t thread_id);
TZD_API int      tzd_thread_resume(uint64_t thread_id);
TZD_API int      tzd_thread_join(uint64_t thread_id, int64_t timeout_ms);
TZD_API int      tzd_thread_detach(uint64_t thread_id);
TZD_API int      tzd_thread_get_dump(char* out_buf, size_t out_buf_size);
TZD_API int      tzd_thread_detect_deadlocks(char* out_buf, size_t out_buf_size);

/* ========================================================================= */
/* 9. Dynamic Compression API (动态压缩技术)                                 */
/* ========================================================================= */
typedef struct tzd_compression_stats_s {
    size_t total_objects_scanned;
    size_t instances_compressed;
    size_t strings_deduped;
    size_t containers_shrunk;
    size_t original_bytes;
    size_t compressed_bytes;
    size_t bytes_saved;
    double compression_ratio; /* Percentage: 0.0 to 100.0 */
} tzd_compression_stats_t;

/* Execute dynamic compression across active heap objects and scopes */
TZD_API int tzd_dynamic_compress(tzd_context_t* ctx, tzd_compression_stats_t* stats);
TZD_API int tzd_set_dynamic_compression(tzd_context_t* ctx, int enabled);
TZD_API int tzd_get_dynamic_compression(tzd_context_t* ctx, int* out_enabled);

/* Compress or decompress a specific instance object */
TZD_API int tzd_compress_instance(tzd_object_t inst, size_t* out_bytes_saved);
TZD_API int tzd_decompress_instance(tzd_object_t inst);

/* Dynamic Compression Event Types */
typedef enum tzd_compression_event_type_e {
    TZD_COMPRESSION_EVENT_START = 1,
    TZD_COMPRESSION_EVENT_FINISH = 2,
    TZD_COMPRESSION_EVENT_OBJECT_COMPRESSED = 3,
    TZD_COMPRESSION_EVENT_OBJECT_DECOMPRESSED = 4
} tzd_compression_event_type_t;

/* Dynamic Compression Event Information */
typedef struct tzd_compression_event_s {
    tzd_compression_event_type_t type;
    void* instance;
    const char* class_name;
    size_t original_bytes;
    size_t compressed_bytes;
    size_t bytes_saved;
    size_t non_default_fields;
    const tzd_compression_stats_t* stats;
} tzd_compression_event_t;

/* Dynamic Compression Listener Callback */
typedef void (*tzd_compression_listener_t)(tzd_context_t* ctx, const tzd_compression_event_t* event, void* user_data);

/* Register / unregister compression event listener */
TZD_API int tzd_set_compression_listener(tzd_context_t* ctx, tzd_compression_listener_t listener, void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* TZD_C_API_H */
