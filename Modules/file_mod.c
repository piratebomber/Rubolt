#include "../src/module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

static Value file_read(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_null();
    
    FILE* file = fopen(args[0].as.string, "r");
    if (!file) return value_null();
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';
    fclose(file);
    
    Value result = value_string(content);
    free(content);
    return result;
}

static Value file_write(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) 
        return value_bool(false);
    
    FILE* file = fopen(args[0].as.string, "w");
    if (!file) return value_bool(false);
    
    fputs(args[1].as.string, file);
    fclose(file);
    return value_bool(true);
}

static Value file_append(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) 
        return value_bool(false);
    
    FILE* file = fopen(args[0].as.string, "a");
    if (!file) return value_bool(false);
    
    fputs(args[1].as.string, file);
    fclose(file);
    return value_bool(true);
}

static Value file_exists(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_bool(false);
    
    struct stat st;
    return value_bool(stat(args[0].as.string, &st) == 0);
}

static Value file_size(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_number(-1);
    
    struct stat st;
    if (stat(args[0].as.string, &st) != 0) return value_number(-1);
    return value_number((double)st.st_size);
}

static Value file_delete(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_bool(false);
    return value_bool(remove(args[0].as.string) == 0);
}

static Value file_copy(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 2 || args[0].type != VAL_STRING || args[1].type != VAL_STRING) 
        return value_bool(false);
    
    FILE* src = fopen(args[0].as.string, "rb");
    if (!src) return value_bool(false);
    
    FILE* dst = fopen(args[1].as.string, "wb");
    if (!dst) {
        fclose(src);
        return value_bool(false);
    }
    
    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    
    fclose(src);
    fclose(dst);
    return value_bool(true);
}

static Value file_readlines(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_null();
    
    FILE* file = fopen(args[0].as.string, "r");
    if (!file) return value_null();
    
    // Create list to hold lines
    Value list = value_list();
    char* line = NULL;
    size_t len = 0;
    
    while (getline(&line, &len, file) != -1) {
        // Remove newline
        size_t line_len = strlen(line);
        if (line_len > 0 && line[line_len - 1] == '\n') {
            line[line_len - 1] = '\0';
        }
        list_append(&list, value_string(line));
    }
    
    free(line);
    fclose(file);
    return list;
}

void register_file_module(ModuleSystem* ms) {
    Module* m = module_system_load(ms, "file");
    module_register_native_function(m, "read", file_read);
    module_register_native_function(m, "write", file_write);
    module_register_native_function(m, "append", file_append);
    module_register_native_function(m, "exists", file_exists);
    module_register_native_function(m, "size", file_size);
    module_register_native_function(m, "delete", file_delete);
    module_register_native_function(m, "copy", file_copy);
    module_register_native_function(m, "readlines", file_readlines);
}