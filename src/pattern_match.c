#include "pattern_match.h"
#include "interpreter.h"
#include <stdlib.h>
#include <string.h>

Pattern* pattern_literal(Value value) {
    Pattern* pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_LITERAL;
    pattern->as.literal = value;
    return pattern;
}

Pattern* pattern_identifier(const char* name) {
    Pattern* pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_IDENTIFIER;
    pattern->as.identifier = strdup(name);
    return pattern;
}

Pattern* pattern_wildcard() {
    Pattern* pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_WILDCARD;
    return pattern;
}

Pattern* pattern_tuple(Pattern** patterns, size_t count) {
    Pattern* pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_TUPLE;
    pattern->as.tuple.patterns = patterns;
    pattern->as.tuple.count = count;
    return pattern;
}

Pattern* pattern_list(Pattern** patterns, size_t count, bool has_rest, const char* rest_name) {
    Pattern* pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_LIST;
    pattern->as.list.patterns = patterns;
    pattern->as.list.count = count;
    pattern->as.list.has_rest = has_rest;
    pattern->as.list.rest_name = rest_name ? strdup(rest_name) : NULL;
    return pattern;
}

Pattern* pattern_object(char** keys, Pattern** patterns, size_t count, bool has_rest) {
    Pattern* pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_OBJECT;
    pattern->as.object.keys = keys;
    pattern->as.object.patterns = patterns;
    pattern->as.object.count = count;
    pattern->as.object.has_rest = has_rest;
    return pattern;
}

Pattern* pattern_type(const char* type_name) {
    Pattern* pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_TYPE;
    pattern->as.type_name = strdup(type_name);
    return pattern;
}

Pattern* pattern_guard(Pattern* pattern, Expr* guard) {
    Pattern* guard_pattern = malloc(sizeof(Pattern));
    guard_pattern->type = PATTERN_GUARD;
    guard_pattern->as.guard.pattern = pattern;
    guard_pattern->as.guard.guard = guard;
    return guard_pattern;
}

Expr* expr_match(Expr* expr, MatchCase* cases, size_t case_count) {
    Expr* match_expr = malloc(sizeof(Expr));
    match_expr->type = EXPR_MATCH;
    match_expr->as.match.expr = expr;
    match_expr->as.match.cases = cases;
    match_expr->as.match.case_count = case_count;
    return match_expr;
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    
    switch (a.type) {
        case VAL_NULL: return true;
        case VAL_BOOL: return a.as.boolean == b.as.boolean;
        case VAL_NUMBER: return a.as.number == b.as.number;
        case VAL_STRING: return strcmp(a.as.string, b.as.string) == 0;
        default: return false;
    }
}

typedef struct PatternMatchContext {
    Environment* env;
    Environment* temp_bindings;
    bool strict_mode;
    size_t recursion_depth;
    size_t max_recursion_depth;
} PatternMatchContext;

typedef struct TypeConstraint {
    char* constraint_name;
    bool (*validator)(Value value, void* context);
    void* context;
} TypeConstraint;

static TypeConstraint type_constraints[] = {
    {"NonEmpty", validate_non_empty, NULL},
    {"Positive", validate_positive_number, NULL},
    {"NonNull", validate_non_null, NULL},
    {"ValidEmail", validate_email_format, NULL},
    {"InRange", validate_number_range, NULL},
    {"MinLength", validate_min_length, NULL},
    {"MaxLength", validate_max_length, NULL}
};

bool validate_non_empty(Value value, void* context) {
    switch (value.type) {
        case VAL_STRING:
            return strlen(value.as.string) > 0;
        case VAL_LIST:
            return value.as.list.count > 0;
        case VAL_DICT:
            return value.as.dict.count > 0;
        default:
            return false;
    }
}

bool validate_positive_number(Value value, void* context) {
    return value.type == VAL_NUMBER && value.as.number > 0;
}

bool validate_non_null(Value value, void* context) {
    return value.type != VAL_NULL;
}

bool validate_email_format(Value value, void* context) {
    if (value.type != VAL_STRING) return false;
    
    const char* email = value.as.string;
    const char* at_pos = strchr(email, '@');
    if (!at_pos) return false;
    
    const char* dot_pos = strrchr(at_pos, '.');
    if (!dot_pos || dot_pos <= at_pos + 1) return false;
    
    return strlen(dot_pos + 1) >= 2;
}

bool validate_number_range(Value value, void* context) {
    if (value.type != VAL_NUMBER) return false;
    
    double* range = (double*)context;
    return value.as.number >= range[0] && value.as.number <= range[1];
}

bool validate_min_length(Value value, void* context) {
    size_t min_len = *(size_t*)context;
    
    switch (value.type) {
        case VAL_STRING:
            return strlen(value.as.string) >= min_len;
        case VAL_LIST:
            return value.as.list.count >= min_len;
        default:
            return false;
    }
}

bool validate_max_length(Value value, void* context) {
    size_t max_len = *(size_t*)context;
    
    switch (value.type) {
        case VAL_STRING:
            return strlen(value.as.string) <= max_len;
        case VAL_LIST:
            return value.as.list.count <= max_len;
        default:
            return false;
    }
}

bool validate_type_constraints(Value value, const char* type_name) {
    for (size_t i = 0; i < sizeof(type_constraints) / sizeof(TypeConstraint); i++) {
        if (strstr(type_name, type_constraints[i].constraint_name)) {
            if (!type_constraints[i].validator(value, type_constraints[i].context)) {
                return false;
            }
        }
    }
    return true;
}

bool pattern_matches_complex(Pattern* pattern, Value value, PatternMatchContext* ctx) {
    if (ctx->recursion_depth >= ctx->max_recursion_depth) {
        runtime_panic_with_type(PANIC_STACK_OVERFLOW, 
                               "Pattern matching recursion depth exceeded: %zu", 
                               ctx->recursion_depth);
        return false;
    }
    
    ctx->recursion_depth++;
    bool result = false;
    
    switch (pattern->type) {
        case PATTERN_LITERAL: {
            result = values_equal_strict(pattern->as.literal, value, ctx->strict_mode);
            break;
        }
        
        case PATTERN_IDENTIFIER:
        case PATTERN_WILDCARD:
            result = true;
            break;
            
        case PATTERN_TUPLE: {
            if (value.type != VAL_TUPLE) {
                result = false;
                break;
            }
            
            if (pattern->as.tuple.count != value.as.tuple.count) {
                result = false;
                break;
            }
            
            result = true;
            for (size_t i = 0; i < pattern->as.tuple.count; i++) {
                if (!pattern_matches_complex(pattern->as.tuple.patterns[i], 
                                           value.as.tuple.elements[i], ctx)) {
                    result = false;
                    break;
                }
            }
            break;
        }
        
        case PATTERN_LIST: {
            if (value.type != VAL_LIST) {
                result = false;
                break;
            }
            
            size_t list_size = value.as.list.count;
            size_t pattern_size = pattern->as.list.count;
            
            if (pattern->as.list.has_rest) {
                if (list_size < pattern_size) {
                    result = false;
                    break;
                }
            } else {
                if (list_size != pattern_size) {
                    result = false;
                    break;
                }
            }
            
            result = true;
            for (size_t i = 0; i < pattern_size && i < list_size; i++) {
                if (!pattern_matches_complex(pattern->as.list.patterns[i],
                                           value.as.list.elements[i], ctx)) {
                    result = false;
                    break;
                }
            }
            
            // Validate rest elements if present
            if (result && pattern->as.list.has_rest && list_size > pattern_size) {
                for (size_t i = pattern_size; i < list_size; i++) {
                    // Rest elements must be valid for the pattern context
                    if (ctx->strict_mode && !validate_value_in_context(value.as.list.elements[i], ctx)) {
                        result = false;
                        break;
                    }
                }
            }
            break;
        }
        
        case PATTERN_OBJECT: {
            if (value.type != VAL_DICT) {
                result = false;
                break;
            }
            
            result = true;
            for (size_t i = 0; i < pattern->as.object.count; i++) {
                Value* field = dict_get(&value, pattern->as.object.keys[i]);
                if (!field) {
                    if (ctx->strict_mode) {
                        result = false;
                        break;
                    }
                    continue;
                }
                
                if (!pattern_matches_complex(pattern->as.object.patterns[i], *field, ctx)) {
                    result = false;
                    break;
                }
            }
            
            // In strict mode, check for extra fields
            if (result && ctx->strict_mode && !pattern->as.object.has_rest) {
                size_t dict_size = dict_size(&value);
                if (dict_size > pattern->as.object.count) {
                    result = false;
                }
            }
            break;
        }
        
        case PATTERN_TYPE: {
            const char* value_type = value_type_name(value);
            result = (strcmp(pattern->as.type_name, value_type) == 0 ||
                     type_is_subtype(value_type, pattern->as.type_name));
            
            if (result) {
                result = validate_type_constraints(value, pattern->as.type_name);
            }
            break;
        }
        
        case PATTERN_GUARD: {
            // First check if the base pattern matches
            if (!pattern_matches_complex(pattern->as.guard.pattern, value, ctx)) {
                result = false;
                break;
            }
            
            // Create temporary environment for guard evaluation
            Environment guard_env;
            environment_init(&guard_env);
            environment_set_parent(&guard_env, ctx->env);
            
            // Bind variables from the pattern
            pattern_bind_complex(pattern->as.guard.pattern, value, &guard_env, ctx);
            
            // Evaluate guard expression
            Value guard_result = evaluate_expr_safe(pattern->as.guard.guard, &guard_env);
            result = value_is_truthy(guard_result);
            
            environment_free(&guard_env);
            break;
        }
        
        default:
            runtime_panic_with_type(PANIC_PATTERN_MATCH_FAILED,
                                   "Unknown pattern type: %d", pattern->type);
            result = false;
            break;
    }
    
    ctx->recursion_depth--;
    return result;
}

bool pattern_matches(Pattern* pattern, Value value, Environment* env) {
    PatternMatchContext ctx = {
        .env = env,
        .temp_bindings = NULL,
        .strict_mode = false,
        .recursion_depth = 0,
        .max_recursion_depth = 1000
    };
    
    return pattern_matches_complex(pattern, value, &ctx);
}

bool values_equal_strict(Value a, Value b, bool strict) {
    if (a.type != b.type) {
        if (!strict) {
            // Allow some type coercion in non-strict mode
            if ((a.type == VAL_NUMBER && b.type == VAL_STRING) ||
                (a.type == VAL_STRING && b.type == VAL_NUMBER)) {
                return false; // Would implement coercion logic here
            }
        }
        return false;
    }
    
    switch (a.type) {
        case VAL_NULL:
            return true;
        case VAL_BOOL:
            return a.as.boolean == b.as.boolean;
        case VAL_NUMBER:
            if (strict) {
                return a.as.number == b.as.number;
            } else {
                return fabs(a.as.number - b.as.number) < 1e-10;
            }
        case VAL_STRING:
            return strcmp(a.as.string, b.as.string) == 0;
        case VAL_LIST:
            if (a.as.list.count != b.as.list.count) return false;
            for (size_t i = 0; i < a.as.list.count; i++) {
                if (!values_equal_strict(a.as.list.elements[i], b.as.list.elements[i], strict)) {
                    return false;
                }
            }
            return true;
        case VAL_DICT:
            return dict_equals(&a, &b, strict);
        default:
            return false;
    }
}

bool type_is_subtype(const char* subtype, const char* supertype) {
    // Check type hierarchy
    TypeHierarchy* type_info = find_type_hierarchy(subtype);
    if (!type_info) return false;
    
    for (size_t i = 0; i < type_info->parent_count; i++) {
        if (strcmp(type_info->parent_types[i], supertype) == 0) {
            return true;
        }
        // Recursive check for inheritance chains
        if (type_is_subtype(type_info->parent_types[i], supertype)) {
            return true;
        }
    }
    
    return false;
}

bool validate_value_in_context(Value value, PatternMatchContext* ctx) {
    // Validate value based on context constraints
    if (ctx->strict_mode) {
        // Perform additional validation in strict mode
        switch (value.type) {
            case VAL_STRING:
                // Check for valid UTF-8, no null bytes, etc.
                return validate_string_integrity(value.as.string);
            case VAL_NUMBER:
                // Check for NaN, infinity, etc.
                return isfinite(value.as.number);
            case VAL_LIST:
                // Recursively validate list elements
                for (size_t i = 0; i < value.as.list.count; i++) {
                    if (!validate_value_in_context(value.as.list.elements[i], ctx)) {
                        return false;
                    }
                }
                return true;
            default:
                return true;
        }
    }
    return true;
}

bool validate_string_integrity(const char* str) {
    if (!str) return false;
    
    // Check for null bytes in the middle of string
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\0') return false;
    }
    
    // Basic UTF-8 validation
    const unsigned char* bytes = (const unsigned char*)str;
    for (size_t i = 0; i < len; i++) {
        if (bytes[i] > 127) {
            // Multi-byte UTF-8 character validation
            if ((bytes[i] & 0xE0) == 0xC0) {
                // 2-byte sequence
                if (i + 1 >= len || (bytes[i + 1] & 0xC0) != 0x80) return false;
                i += 1;
            } else if ((bytes[i] & 0xF0) == 0xE0) {
                // 3-byte sequence
                if (i + 2 >= len || (bytes[i + 1] & 0xC0) != 0x80 || (bytes[i + 2] & 0xC0) != 0x80) return false;
                i += 2;
            } else if ((bytes[i] & 0xF8) == 0xF0) {
                // 4-byte sequence
                if (i + 3 >= len || (bytes[i + 1] & 0xC0) != 0x80 || 
                    (bytes[i + 2] & 0xC0) != 0x80 || (bytes[i + 3] & 0xC0) != 0x80) return false;
                i += 3;
            } else {
                return false; // Invalid UTF-8
            }
        }
    }
    
    return true;
}

void pattern_bind(Pattern* pattern, Value value, Environment* env) {
    switch (pattern->type) {
        case PATTERN_IDENTIFIER:
            environment_define(env, pattern->as.identifier, value);
            break;
            
        case PATTERN_TUPLE:
            for (size_t i = 0; i < pattern->as.tuple.count; i++) {
                pattern_bind(pattern->as.tuple.patterns[i],
                           value.as.tuple.elements[i], env);
            }
            break;
            
        case PATTERN_LIST:
            for (size_t i = 0; i < pattern->as.list.count; i++) {
                pattern_bind(pattern->as.list.patterns[i],
                           value.as.list.elements[i], env);
            }
            
            if (pattern->as.list.has_rest) {
                // Bind rest elements to rest variable
                Value rest_list = value_list();
                for (size_t i = pattern->as.list.count; i < value.as.list.count; i++) {
                    list_append(&rest_list, value.as.list.elements[i]);
                }
                environment_define(env, pattern->as.list.rest_name, rest_list);
            }
            break;
            
        case PATTERN_OBJECT:
            for (size_t i = 0; i < pattern->as.object.count; i++) {
                Value* field = dict_get(&value, pattern->as.object.keys[i]);
                if (field) {
                    pattern_bind(pattern->as.object.patterns[i], *field, env);
                }
            }
            break;
            
        case PATTERN_GUARD:
            pattern_bind(pattern->as.guard.pattern, value, env);
            break;
            
        default:
            break;
    }
}

void pattern_free(Pattern* pattern) {
    if (!pattern) return;
    
    switch (pattern->type) {
        case PATTERN_LITERAL:
            value_free(&pattern->as.literal);
            break;
            
        case PATTERN_IDENTIFIER:
            free(pattern->as.identifier);
            break;
            
        case PATTERN_TUPLE:
            for (size_t i = 0; i < pattern->as.tuple.count; i++) {
                pattern_free(pattern->as.tuple.patterns[i]);
            }
            free(pattern->as.tuple.patterns);
            break;
            
        case PATTERN_LIST:
            for (size_t i = 0; i < pattern->as.list.count; i++) {
                pattern_free(pattern->as.list.patterns[i]);
            }
            free(pattern->as.list.patterns);
            free(pattern->as.list.rest_name);
            break;
            
        case PATTERN_OBJECT:
            for (size_t i = 0; i < pattern->as.object.count; i++) {
                free(pattern->as.object.keys[i]);
                pattern_free(pattern->as.object.patterns[i]);
            }
            free(pattern->as.object.keys);
            free(pattern->as.object.patterns);
            break;
            
        case PATTERN_TYPE:
            free(pattern->as.type_name);
            break;
            
        case PATTERN_GUARD:
            pattern_free(pattern->as.guard.pattern);
            expr_free(pattern->as.guard.guard);
            break;
    }
    
    free(pattern);
}