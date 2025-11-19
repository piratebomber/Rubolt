#include "../src/module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char* json;
    size_t pos;
    size_t len;
} JsonParser;

static void skip_whitespace(JsonParser* p) {
    while (p->pos < p->len && isspace(p->json[p->pos])) p->pos++;
}

static Value parse_json_value(JsonParser* p);

static Value parse_json_string(JsonParser* p) {
    if (p->json[p->pos] != '"') return value_null();
    p->pos++; // Skip opening quote
    
    size_t start = p->pos;
    while (p->pos < p->len && p->json[p->pos] != '"') {
        if (p->json[p->pos] == '\\') p->pos++; // Skip escape
        p->pos++;
    }
    
    if (p->pos >= p->len) return value_null();
    
    size_t len = p->pos - start;
    char* str = malloc(len + 1);
    strncpy(str, p->json + start, len);
    str[len] = '\0';
    p->pos++; // Skip closing quote
    
    Value result = value_string(str);
    free(str);
    return result;
}

static Value parse_json_number(JsonParser* p) {
    size_t start = p->pos;
    if (p->json[p->pos] == '-') p->pos++;
    
    while (p->pos < p->len && (isdigit(p->json[p->pos]) || p->json[p->pos] == '.')) {
        p->pos++;
    }
    
    char* num_str = malloc(p->pos - start + 1);
    strncpy(num_str, p->json + start, p->pos - start);
    num_str[p->pos - start] = '\0';
    
    double num = atof(num_str);
    free(num_str);
    return value_number(num);
}

static Value parse_json_array(JsonParser* p) {
    if (p->json[p->pos] != '[') return value_null();
    p->pos++; // Skip '['
    
    Value array = value_list();
    skip_whitespace(p);
    
    if (p->pos < p->len && p->json[p->pos] == ']') {
        p->pos++;
        return array;
    }
    
    while (p->pos < p->len) {
        Value item = parse_json_value(p);
        list_append(&array, item);
        
        skip_whitespace(p);
        if (p->pos >= p->len) break;
        
        if (p->json[p->pos] == ']') {
            p->pos++;
            break;
        } else if (p->json[p->pos] == ',') {
            p->pos++;
            skip_whitespace(p);
        }
    }
    
    return array;
}

static Value parse_json_object(JsonParser* p) {
    if (p->json[p->pos] != '{') return value_null();
    p->pos++; // Skip '{'
    
    Value obj = value_dict();
    skip_whitespace(p);
    
    if (p->pos < p->len && p->json[p->pos] == '}') {
        p->pos++;
        return obj;
    }
    
    while (p->pos < p->len) {
        skip_whitespace(p);
        Value key = parse_json_string(p);
        if (key.type != VAL_STRING) break;
        
        skip_whitespace(p);
        if (p->pos >= p->len || p->json[p->pos] != ':') break;
        p->pos++; // Skip ':'
        
        skip_whitespace(p);
        Value value = parse_json_value(p);
        dict_set(&obj, key.as.string, value);
        
        skip_whitespace(p);
        if (p->pos >= p->len) break;
        
        if (p->json[p->pos] == '}') {
            p->pos++;
            break;
        } else if (p->json[p->pos] == ',') {
            p->pos++;
        }
    }
    
    return obj;
}

static Value parse_json_value(JsonParser* p) {
    skip_whitespace(p);
    if (p->pos >= p->len) return value_null();
    
    char c = p->json[p->pos];
    
    if (c == '"') return parse_json_string(p);
    if (c == '[') return parse_json_array(p);
    if (c == '{') return parse_json_object(p);
    if (c == '-' || isdigit(c)) return parse_json_number(p);
    
    // Handle literals
    if (strncmp(p->json + p->pos, "true", 4) == 0) {
        p->pos += 4;
        return value_bool(true);
    }
    if (strncmp(p->json + p->pos, "false", 5) == 0) {
        p->pos += 5;
        return value_bool(false);
    }
    if (strncmp(p->json + p->pos, "null", 4) == 0) {
        p->pos += 4;
        return value_null();
    }
    
    return value_null();
}

static Value json_parse(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_null();
    
    JsonParser parser = {
        .json = args[0].as.string,
        .pos = 0,
        .len = strlen(args[0].as.string)
    };
    
    return parse_json_value(&parser);
}

static void stringify_value(Value val, char** result, size_t* capacity, size_t* length) {
    char buffer[256];
    
    switch (val.type) {
        case VAL_NULL:
            strcpy(buffer, "null");
            break;
        case VAL_BOOL:
            strcpy(buffer, val.as.boolean ? "true" : "false");
            break;
        case VAL_NUMBER:
            snprintf(buffer, sizeof(buffer), "%.15g", val.as.number);
            break;
        case VAL_STRING:
            snprintf(buffer, sizeof(buffer), "\"%s\"", val.as.string);
            break;
        default:
            strcpy(buffer, "null");
            break;
    }
    
    size_t needed = strlen(buffer);
    if (*length + needed >= *capacity) {
        *capacity = (*capacity == 0) ? 1024 : *capacity * 2;
        *result = realloc(*result, *capacity);
    }
    
    strcpy(*result + *length, buffer);
    *length += needed;
}

static Value json_stringify(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1) return value_null();
    
    char* result = NULL;
    size_t capacity = 0;
    size_t length = 0;
    
    stringify_value(args[0], &result, &capacity, &length);
    
    Value json_str = value_string(result);
    free(result);
    return json_str;
}

void register_json_module(ModuleSystem* ms) {
    Module* m = module_system_load(ms, "json");
    module_register_native_function(m, "parse", json_parse);
    module_register_native_function(m, "stringify", json_stringify);
}