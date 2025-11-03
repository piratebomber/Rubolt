#ifndef RUBOLT_SDK_API_H
#define RUBOLT_SDK_API_H

#include <stdint.h>
#include <stdbool.h>

// Rubolt SDK Native Extension API v1.0

// Value types
typedef enum {
    RB_TYPE_NULL,
    RB_TYPE_INT,
    RB_TYPE_FLOAT,
    RB_TYPE_STRING,
    RB_TYPE_BOOL,
    RB_TYPE_LIST,
    RB_TYPE_DICT,
    RB_TYPE_FUNCTION
} RbType;

// Opaque value handle
typedef struct RbValue* RbValue;

// Function signature for native functions
typedef RbValue (*RbNativeFunc)(int argc, RbValue* argv);

// Value creation
RbValue rb_int(int64_t val);
RbValue rb_float(double val);
RbValue rb_string(const char* str);
RbValue rb_bool(bool val);
RbValue rb_null(void);
RbValue rb_list(int size);
RbValue rb_dict(void);

// Value access
int64_t rb_as_int(RbValue v);
double rb_as_float(RbValue v);
const char* rb_as_string(RbValue v);
bool rb_as_bool(RbValue v);
RbType rb_typeof(RbValue v);

// List operations
void rb_list_append(RbValue list, RbValue item);
RbValue rb_list_get(RbValue list, int index);
int rb_list_len(RbValue list);

// Dict operations
void rb_dict_set(RbValue dict, const char* key, RbValue val);
RbValue rb_dict_get(RbValue dict, const char* key);

// Error handling
void rb_raise_error(const char* message);
bool rb_has_error(void);

// Module registration
typedef struct {
    const char* name;
    RbNativeFunc func;
} RbModuleFunc;

void rb_register_module(const char* module_name, RbModuleFunc* funcs, int count);

// Memory management
void rb_incref(RbValue v);
void rb_decref(RbValue v);

// Utility
void rb_print(RbValue v);
RbValue rb_call(RbValue func, int argc, RbValue* argv);

#endif // RUBOLT_SDK_API_H
