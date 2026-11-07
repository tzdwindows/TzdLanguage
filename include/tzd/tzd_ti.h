#ifndef TZD_TI_H
#define TZD_TI_H

#include "tzd_export.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tzd_context tzd_context_t;

/* ------------------------------------------------------------------------- */
/* JVMTI-style Event Flags (Bitmask)                                         */
/* ------------------------------------------------------------------------- */
typedef enum tzd_ti_event_e {
    TZD_TI_EVENT_VM_INIT              = (1 << 0),
    TZD_TI_EVENT_VM_DEATH             = (1 << 1),
    TZD_TI_EVENT_METHOD_ENTRY         = (1 << 2),
    TZD_TI_EVENT_METHOD_EXIT          = (1 << 3),
    TZD_TI_EVENT_SINGLE_STEP          = (1 << 4),
    TZD_TI_EVENT_BREAKPOINT           = (1 << 5),
    TZD_TI_EVENT_EXCEPTION_THROW      = (1 << 6),
    TZD_TI_EVENT_EXCEPTION_CATCH      = (1 << 7),
    TZD_TI_EVENT_CLASS_LOAD           = (1 << 8),
    TZD_TI_EVENT_COMPILED_METHOD_LOAD = (1 << 9),
    TZD_TI_EVENT_GARBAGE_COLLECTION   = (1 << 10),
    TZD_TI_EVENT_THREAD_START         = (1 << 11),
    TZD_TI_EVENT_THREAD_END           = (1 << 12)
} tzd_ti_event_t;

/* Stack frame structure for stack inspection */
typedef struct tzd_ti_frame_s {
    const char* class_name;
    const char* method_name;
    const char* file_name;
    int line;
    int column;
    int is_jit_compiled;
} tzd_ti_frame_t;

/* Callback table matching JVMTI event hooks */
typedef struct tzd_ti_callbacks_s {
    void (*on_vm_init)(tzd_context_t* ctx);
    void (*on_vm_death)(tzd_context_t* ctx);
    void (*on_method_entry)(tzd_context_t* ctx, const char* class_name, const char* method_name, const char* file, int line);
    void (*on_method_exit)(tzd_context_t* ctx, const char* class_name, const char* method_name, const char* return_val_str);
    void (*on_single_step)(tzd_context_t* ctx, const char* file, int line, int column);
    void (*on_breakpoint)(tzd_context_t* ctx, const char* file, int line);
    void (*on_exception_throw)(tzd_context_t* ctx, const char* exception_msg, const char* file, int line);
    void (*on_exception_catch)(tzd_context_t* ctx, const char* exception_msg, const char* file, int line);
    void (*on_class_load)(tzd_context_t* ctx, const char* class_name);
    void (*on_compiled_method_load)(const char* method_name, const void* code_addr, size_t code_size);
    void (*on_garbage_collection)(tzd_context_t* ctx, size_t freed_bytes, size_t total_alive);
    void (*on_thread_start)(tzd_context_t* ctx, uint64_t thread_id, const char* thread_name);
    void (*on_thread_end)(tzd_context_t* ctx, uint64_t thread_id, const char* thread_name);
} tzd_ti_callbacks_t;

/* Opaque Tool Interface Environment Handle */
typedef struct tzd_ti_env tzd_ti_env_t;

/* ------------------------------------------------------------------------- */
/* Tool Interface Management Functions                                       */
/* ------------------------------------------------------------------------- */

/* Retrieve the global or context-specific Tool Interface Environment */
TZD_API tzd_ti_env_t* tzd_ti_get_env(void);

/* Register event callbacks (pass NULL to clear) */
TZD_API int tzd_ti_set_callbacks(tzd_ti_env_t* env, const tzd_ti_callbacks_t* callbacks);

/* Enable or disable specific event notifications (bitmask of tzd_ti_event_t) */
TZD_API int tzd_ti_set_event_notification_mode(tzd_ti_env_t* env, uint32_t event_mask, int enable);

/* Query whether an event type is currently enabled (fast inline-friendly check) */
TZD_API int tzd_ti_is_event_enabled(uint32_t event_bit);

/* Set a code breakpoint at file and line */
TZD_API int tzd_ti_set_breakpoint(tzd_ti_env_t* env, const char* file, int line);

/* Clear a code breakpoint at file and line */
TZD_API int tzd_ti_clear_breakpoint(tzd_ti_env_t* env, const char* file, int line);

/* Clear all breakpoints */
TZD_API int tzd_ti_clear_all_breakpoints(tzd_ti_env_t* env);

/* Inspect the current call stack trace */
TZD_API int tzd_ti_get_call_stack(tzd_ti_env_t* env, tzd_context_t* ctx,
                                  tzd_ti_frame_t* frames_out, int max_frames, int* frames_count_out);

/* Force immediate Garbage Collection pass */
TZD_API int tzd_ti_force_gc(tzd_ti_env_t* env, tzd_context_t* ctx);

/* Query and retrieve all live declared instances of a given class by name.
 * Inspects all scopes across the interpreter and active threads.
 * Matches the specified class name or its subclasses.
 * out_objects receives pointers to the object instances (tzd_object_t).
 * Returns TZD_OK on success, and fills *out_count with the total number found.
 */
TZD_API int tzd_ti_get_objects_by_class(tzd_ti_env_t* env, tzd_context_t* ctx,
                                        const char* class_name,
                                        void** out_objects, size_t max_count, size_t* out_count);

/* Trigger hooks programmatically (used internally by JIT and interpreter) */
TZD_API void tzd_ti_notify_method_entry(tzd_context_t* ctx, const char* cls, const char* method, const char* file, int line);
TZD_API void tzd_ti_notify_method_exit(tzd_context_t* ctx, const char* cls, const char* method, const char* ret_val_str);
TZD_API void tzd_ti_notify_single_step(tzd_context_t* ctx, const char* file, int line, int column);
TZD_API void tzd_ti_notify_exception_throw(tzd_context_t* ctx, const char* msg, const char* file, int line);
TZD_API void tzd_ti_notify_compiled_method(const char* method, const void* code, size_t size);
TZD_API void tzd_ti_notify_thread_start(tzd_context_t* ctx, uint64_t thread_id, const char* thread_name);
TZD_API void tzd_ti_notify_thread_end(tzd_context_t* ctx, uint64_t thread_id, const char* thread_name);

#ifdef __cplusplus
}
#endif

#endif /* TZD_TI_H */
