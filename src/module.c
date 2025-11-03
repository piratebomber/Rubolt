#include "module.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "modules_registry.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

void module_system_init(ModuleSystem* ms) {
    ms->module_count = 0;
    ms->search_paths = NULL;
    ms->search_path_count = 0;
    
    // Add default search paths
    module_system_add_search_path(ms, "./lib");
    module_system_add_search_path(ms, "./stdlib");
    
    // Register standard library modules
    register_math_module(ms);
    register_os_module(ms);
    register_sys_module(ms);
    register_file_module(ms);
    register_time_module(ms);

    // Register custom C modules from Modules/
    register_custom_modules(ms);
}

void module_system_free(ModuleSystem* ms) {
    for (size_t i = 0; i < ms->module_count; i++) {
        free(ms->modules[i].name);
        if (ms->modules[i].path) free(ms->modules[i].path);
        for (size_t j = 0; j < ms->modules[i].function_count; j++) {
            free(ms->modules[i].functions[j].name);
        }
    }
    for (size_t i = 0; i < ms->search_path_count; i++) {
        free(ms->search_paths[i]);
    }
    free(ms->search_paths);
}

void module_system_add_search_path(ModuleSystem* ms, const char* path) {
    ms->search_paths = realloc(ms->search_paths, sizeof(char*) * (ms->search_path_count + 1));
    ms->search_paths[ms->search_path_count++] = strdup(path);
}

Module* module_system_get(ModuleSystem* ms, const char* name) {
    for (size_t i = 0; i < ms->module_count; i++) {
        if (strcmp(ms->modules[i].name, name) == 0) {
            return &ms->modules[i];
        }
    }
    return NULL;
}

Module* module_system_load(ModuleSystem* ms, const char* name) {
    Module* existing = module_system_get(ms, name);
    if (existing && existing->is_loaded) {
        return existing;
    }
    
    if (ms->module_count >= MAX_MODULES) {
        fprintf(stderr, "Error: Maximum number of modules reached\n");
        return NULL;
    }
    
    Module* mod = &ms->modules[ms->module_count++];
    mod->name = strdup(name);
    mod->path = NULL;
    mod->function_count = 0;
    mod->is_loaded = true;
    
    return mod;
}

void module_register_native_function(Module* mod, const char* name, 
                                     Value (*func)(Environment*, Value*, size_t)) {
    if (mod->function_count >= MAX_MODULE_FUNCTIONS) return;
    
    mod->functions[mod->function_count].name = strdup(name);
    mod->functions[mod->function_count].native_func = func;
    mod->function_count++;
}

// Math module
static Value math_sqrt(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) {
        return value_null();
    }
    return value_number(sqrt(args[0].as.number));
}

static Value math_pow(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_NUMBER || args[1].type != VAL_NUMBER) {
        return value_null();
    }
    return value_number(pow(args[0].as.number, args[1].as.number));
}

static Value math_abs(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) {
        return value_null();
    }
    return value_number(fabs(args[0].as.number));
}

static Value math_floor(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) {
        return value_null();
    }
    return value_number(floor(args[0].as.number));
}

static Value math_ceil(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) {
        return value_null();
    }
    return value_number(ceil(args[0].as.number));
}

static Value math_sin(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) {
        return value_null();
    }
    return value_number(sin(args[0].as.number));
}

static Value math_cos(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) {
        return value_null();
    }
    return value_number(cos(args[0].as.number));
}

void register_math_module(ModuleSystem* ms) {
    Module* mod = module_system_load(ms, "math");
    module_register_native_function(mod, "sqrt", math_sqrt);
    module_register_native_function(mod, "pow", math_pow);
    module_register_native_function(mod, "abs", math_abs);
    module_register_native_function(mod, "floor", math_floor);
    module_register_native_function(mod, "ceil", math_ceil);
    module_register_native_function(mod, "sin", math_sin);
    module_register_native_function(mod, "cos", math_cos);
}

// OS module
static Value os_getcwd(Environment* env, Value* args, size_t arg_count) {
    char buffer[1024];
    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        return value_string(buffer);
    }
    return value_null();
}

static Value os_getenv(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) {
        return value_null();
    }
    const char* val = getenv(args[0].as.string);
    return val ? value_string(val) : value_null();
}

static Value os_system(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) {
        return value_null();
    }
    int result = system(args[0].as.string);
    return value_number(result);
}

void register_os_module(ModuleSystem* ms) {
    Module* mod = module_system_load(ms, "os");
    module_register_native_function(mod, "getcwd", os_getcwd);
    module_register_native_function(mod, "getenv", os_getenv);
    module_register_native_function(mod, "system", os_system);
}

// Sys module
static Value sys_exit(Environment* env, Value* args, size_t arg_count) {
    int code = 0;
    if (arg_count > 0 && args[0].type == VAL_NUMBER) {
        code = (int)args[0].as.number;
    }
    exit(code);
    return value_null();
}

static Value sys_version(Environment* env, Value* args, size_t arg_count) {
    return value_string("Rubolt 1.0.0");
}

void register_sys_module(ModuleSystem* ms) {
    Module* mod = module_system_load(ms, "sys");
    module_register_native_function(mod, "exit", sys_exit);
    module_register_native_function(mod, "version", sys_version);
}

// File module
static Value file_read(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) {
        return value_null();
    }
    
    FILE* file = fopen(args[0].as.string, "rb");
    if (!file) return value_null();
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    
    char* buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    Value result = value_string(buffer);
    free(buffer);
    return result;
}

static Value file_write(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) {
        return value_bool(false);
    }
    
    FILE* file = fopen(args[0].as.string, "w");
    if (!file) return value_bool(false);
    
    fputs(args[1].as.string, file);
    fclose(file);
    return value_bool(true);
}

static Value file_exists(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) {
        return value_bool(false);
    }
    
    FILE* file = fopen(args[0].as.string, "r");
    if (file) {
        fclose(file);
        return value_bool(true);
    }
    return value_bool(false);
}

void register_file_module(ModuleSystem* ms) {
    Module* mod = module_system_load(ms, "file");
    module_register_native_function(mod, "read", file_read);
    module_register_native_function(mod, "write", file_write);
    module_register_native_function(mod, "exists", file_exists);
}

// Time module
static Value time_now(Environment* env, Value* args, size_t arg_count) {
    return value_number((double)time(NULL));
}

static Value time_sleep(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_NUMBER) {
        return value_null();
    }
    
    int seconds = (int)args[0].as.number;
#ifdef _WIN32
    Sleep(seconds * 1000);
#else
    sleep(seconds);
#endif
    return value_null();
}

void register_time_module(ModuleSystem* ms) {
    Module* mod = module_system_load(ms, "time");
    module_register_native_function(mod, "now", time_now);
    module_register_native_function(mod, "sleep", time_sleep);
}

void register_json_module(ModuleSystem* ms) {
    // Placeholder for JSON module
    module_system_load(ms, "json");
}
