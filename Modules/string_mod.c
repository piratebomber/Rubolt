#include "module.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static Value str_len(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_number(0);
    return value_number((double)strlen(args[0].as.string));
}

static Value str_upper(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_null();
    char* s = strdup(args[0].as.string);
    for (char* p = s; *p; ++p) *p = (char)toupper((unsigned char)*p);
    Value v = value_string(s);
    free(s);
    return v;
}

static Value str_lower(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_null();
    char* s = strdup(args[0].as.string);
    for (char* p = s; *p; ++p) *p = (char)tolower((unsigned char)*p);
    Value v = value_string(s);
    free(s);
    return v;
}

static Value str_concat(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) return value_null();
    size_t len = strlen(args[0].as.string) + strlen(args[1].as.string) + 1;
    char* s = malloc(len);
    strcpy(s, args[0].as.string);
    strcat(s, args[1].as.string);
    Value v = value_string(s);
    free(s);
    return v;
}

void register_mod_string(ModuleSystem* ms) {
    Module* m = module_system_load(ms, "string");
    module_register_native_function(m, "len", str_len);
    module_register_native_function(m, "upper", str_upper);
    module_register_native_function(m, "lower", str_lower);
    module_register_native_function(m, "concat", str_concat);
}
