#ifndef TZD_TNI_H
#define TZD_TNI_H

#include "tzd_export.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tzd_context tzd_context_t;

/* Opaque handles mirroring JNI handles */
typedef void* tzd_object_t;
typedef void* tzd_class_t;
typedef void* tzd_method_id_t;
typedef void* tzd_field_id_t;
typedef void* tzd_string_t;
typedef void* tzd_array_t;
typedef void* tzd_map_t;

/* Native method descriptor for registering C functions into Tzd classes */
typedef struct tzd_native_method_s {
    const char* name;
    const char* signature;
    void* fnPtr;
} tzd_native_method_t;

/* Forward declaration of the TNI environment */
typedef struct tzd_native_interface_s tzd_native_interface_t;
typedef const tzd_native_interface_t* tzd_env_t;

/* The TNI Function Table - identical architecture to JNIEnv */
struct tzd_native_interface_s {
    /* 1. Version & State */
    int         (*GetVersion)(tzd_env_t env);
    tzd_context_t* (*GetContext)(tzd_env_t env);

    /* 2. Class Operations */
    tzd_class_t (*FindClass)(tzd_env_t env, const char* name);
    tzd_class_t (*GetSuperclass)(tzd_env_t env, tzd_class_t clazz);
    int         (*IsAssignableFrom)(tzd_env_t env, tzd_class_t sub, tzd_class_t sup);
    const char* (*GetClassName)(tzd_env_t env, tzd_class_t clazz);

    /* 3. Method ID and Invocation */
    tzd_method_id_t (*GetMethodID)(tzd_env_t env, tzd_class_t clazz, const char* name, const char* sig);
    tzd_object_t    (*NewObject)(tzd_env_t env, tzd_class_t clazz, int argc, const char** argv);
    int             (*CallMethod)(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv, char* out_buf, size_t out_buf_size);
    int             (*CallVoidMethod)(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv);
    int             (*CallBooleanMethod)(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv);
    double          (*CallDoubleMethod)(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv);
    int64_t         (*CallLongMethod)(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv);
    int             (*CallStringMethod)(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv, char* out_buf, size_t out_buf_size);
    tzd_object_t    (*CallObjectMethod)(tzd_env_t env, tzd_object_t obj, tzd_method_id_t methodID, int argc, const char** argv);

    /* 4. Field Access */
    tzd_field_id_t (*GetFieldID)(tzd_env_t env, tzd_class_t clazz, const char* name);
    int            (*GetBooleanField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID);
    void           (*SetBooleanField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, int val);
    double         (*GetDoubleField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID);
    void           (*SetDoubleField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, double val);
    int64_t        (*GetIntField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID);
    void           (*SetIntField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, int64_t val);
    int            (*GetStringField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, char* out_buf, size_t out_buf_size);
    void           (*SetStringField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, const char* val);
    tzd_object_t   (*GetObjectField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID);
    void           (*SetObjectField)(tzd_env_t env, tzd_object_t obj, tzd_field_id_t fieldID, tzd_object_t val);

    /* 5. String Utilities */
    tzd_string_t (*NewStringUTF)(tzd_env_t env, const char* bytes);
    const char*  (*GetStringUTFChars)(tzd_env_t env, tzd_string_t str);
    void         (*ReleaseStringUTFChars)(tzd_env_t env, tzd_string_t str, const char* chars);

    /* 6. Dynamic Arrays and Maps */
    tzd_array_t  (*NewArray)(tzd_env_t env, size_t initial_capacity);
    size_t       (*GetArrayLength)(tzd_env_t env, tzd_array_t arr);
    int          (*GetArrayStringElement)(tzd_env_t env, tzd_array_t arr, size_t index, char* out_buf, size_t out_buf_size);
    void         (*ArrayPushDouble)(tzd_env_t env, tzd_array_t arr, double val);
    void         (*ArrayPushString)(tzd_env_t env, tzd_array_t arr, const char* val);

    tzd_map_t    (*NewMap)(tzd_env_t env);
    void         (*MapPutDouble)(tzd_env_t env, tzd_map_t map, const char* key, double val);
    void         (*MapPutString)(tzd_env_t env, tzd_map_t map, const char* key, const char* val);
    double       (*MapGetDouble)(tzd_env_t env, tzd_map_t map, const char* key);
    int          (*MapGetString)(tzd_env_t env, tzd_map_t map, const char* key, char* out_buf, size_t out_buf_size);

    /* 7. Native Method Registration (Register C functions into Tzd class) */
    int          (*RegisterNatives)(tzd_env_t env, tzd_class_t clazz, const tzd_native_method_t* methods, int nMethods);

    /* 8. Exception Handling */
    int          (*ThrowNew)(tzd_env_t env, const char* message);
    int          (*ExceptionOccurred)(tzd_env_t env);
    const char*  (*ExceptionDescribe)(tzd_env_t env);
    void         (*ExceptionClear)(tzd_env_t env);

    /* 9. Object and Class Retransformation */
    size_t       (*GetObjectSize)(tzd_env_t env, tzd_object_t obj);
    int          (*RetransformClass)(tzd_env_t env, const char* name, const char* class_source);

    /* 10. Global Reference Management */
    tzd_object_t (*NewGlobalRef)(tzd_env_t env, tzd_object_t lobj);
    void         (*DeleteGlobalRef)(tzd_env_t env, tzd_object_t gref);

    /* 11. Direct Byte Buffer Operations */
    tzd_object_t (*NewDirectByteBuffer)(tzd_env_t env, void* address, size_t capacity);
    void*        (*GetDirectBufferAddress)(tzd_env_t env, tzd_object_t buf);
    size_t       (*GetDirectBufferCapacity)(tzd_env_t env, tzd_object_t buf);
};

/* Obtain the TNI environment from a context */
TZD_API tzd_env_t tzd_get_env(tzd_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* TZD_TNI_H */
