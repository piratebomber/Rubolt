#ifndef RUBOLT_MODULE_H
#define RUBOLT_MODULE_H

#include "ast.h"
#include "interpreter.h"
#include <stdbool.h>

#define MAX_MODULES 64
#define MAX_MODULE_FUNCTIONS 128

typedef struct {
    char* name;
    Value (*native_func)(Environment* env, Value* args, size_t arg_count);
} NativeFunction;

typedef struct {
    char* name;
    char* path;
    NativeFunction functions[MAX_MODULE_FUNCTIONS];
    size_t function_count;
    bool is_loaded;
} Module;

typedef struct {
    Module modules[MAX_MODULES];
    size_t module_count;
    char** search_paths;
    size_t search_path_count;
} ModuleSystem;

void module_system_init(ModuleSystem* ms);
void module_system_free(ModuleSystem* ms);
void module_system_add_search_path(ModuleSystem* ms, const char* path);
Module* module_system_load(ModuleSystem* ms, const char* name);
Module* module_system_get(ModuleSystem* ms, const char* name);
void module_register_native_function(Module* mod, const char* name, 
                                     Value (*func)(Environment*, Value*, size_t));

// Standard library modules
void register_math_module(ModuleSystem* ms);
void register_os_module(ModuleSystem* ms);
void register_sys_module(ModuleSystem* ms);
void register_file_module(ModuleSystem* ms);
void register_json_module(ModuleSystem* ms);
void register_time_module(ModuleSystem* ms);

#endif
