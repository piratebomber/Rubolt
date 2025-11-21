#include "pattern_match.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

// Pattern matching engine
bool pattern_match(Pattern *pattern, Value value, Environment *env) {
    switch (pattern->type) {
        case PATTERN_LITERAL:
            return match_literal_pattern(&pattern->as.literal, value);
            
        case PATTERN_IDENTIFIER:
            return match_identifier_pattern(&pattern->as.identifier, value, env);
            
        case PATTERN_WILDCARD:
            return true; // Wildcard matches everything
            
        case PATTERN_TUPLE:
            return match_tuple_pattern(&pattern->as.tuple, value, env);
            
        case PATTERN_ARRAY:
            return match_array_pattern(&pattern->as.array, value, env);
            
        case PATTERN_OBJECT:
            return match_object_pattern(&pattern->as.object, value, env);
            
        case PATTERN_TYPE:
            return match_type_pattern(&pattern->as.type, value);
            
        case PATTERN_GUARD:
            return match_guard_pattern(&pattern->as.guard, value, env);
            
        case PATTERN_OR:
            return match_or_pattern(&pattern->as.or_pattern, value, env);
            
        case PATTERN_RANGE:
            return match_range_pattern(&pattern->as.range, value);
            
        case PATTERN_REGEX:
            return match_regex_pattern(&pattern->as.regex, value);
            
        case PATTERN_CONSTRUCTOR:
            return match_constructor_pattern(&pattern->as.constructor, value, env);
            
        default:
            return false;
    }
}

bool match_literal_pattern(LiteralPattern *pattern, Value value) {
    switch (pattern->value_type) {
        case VALUE_NUMBER:
            return value.type == VALUE_NUMBER && 
                   value.as.number == pattern->as.number;
                   
        case VALUE_STRING:
            return value.type == VALUE_STRING && 
                   strcmp(value.as.string, pattern->as.string) == 0;
                   
        case VALUE_BOOL:
            return value.type == VALUE_BOOL && 
                   value.as.boolean == pattern->as.boolean;
                   
        case VALUE_NULL:
            return value.type == VALUE_NULL;
            
        default:
            return false;
    }
}

bool match_identifier_pattern(IdentifierPattern *pattern, Value value, Environment *env) {
    // Bind the value to the identifier in the environment
    environment_define(env, pattern->name, value);
    
    // Check type constraint if present
    if (pattern->type_constraint) {
        return check_type_constraint(value, pattern->type_constraint);
    }
    
    return true;
}

bool match_tuple_pattern(TuplePattern *pattern, Value value, Environment *env) {
    if (value.type != VALUE_TUPLE) {
        return false;
    }
    
    Tuple *tuple = (Tuple *)value.as.object;
    
    // Check element count
    if (tuple->element_count != pattern->element_count) {
        return false;
    }
    
    // Match each element
    for (size_t i = 0; i < pattern->element_count; i++) {
        if (!pattern_match(pattern->elements[i], tuple->elements[i], env)) {
            return false;
        }
    }
    
    return true;
}

bool match_array_pattern(ArrayPattern *pattern, Value value, Environment *env) {
    if (value.type != VALUE_ARRAY) {
        return false;
    }
    
    Array *array = (Array *)value.as.object;
    
    // Handle rest pattern
    if (pattern->has_rest) {
        if (array->length < pattern->element_count - 1) {
            return false;
        }
        
        // Match fixed elements
        for (size_t i = 0; i < pattern->rest_index; i++) {
            if (!pattern_match(pattern->elements[i], array->elements[i], env)) {
                return false;
            }
        }
        
        // Create rest array
        size_t rest_start = pattern->rest_index;
        size_t rest_length = array->length - (pattern->element_count - 1);
        
        Array *rest_array = create_array(rest_length);
        for (size_t i = 0; i < rest_length; i++) {
            rest_array->elements[i] = array->elements[rest_start + i];
        }
        
        // Bind rest array to identifier
        environment_define(env, pattern->rest_name, value_object(rest_array));
        
        // Match remaining elements
        for (size_t i = pattern->rest_index + 1; i < pattern->element_count; i++) {
            size_t array_index = array->length - (pattern->element_count - i);
            if (!pattern_match(pattern->elements[i], array->elements[array_index], env)) {
                return false;
            }
        }
        
        return true;
    } else {
        // Exact match
        if (array->length != pattern->element_count) {
            return false;
        }
        
        for (size_t i = 0; i < pattern->element_count; i++) {
            if (!pattern_match(pattern->elements[i], array->elements[i], env)) {
                return false;
            }
        }
        
        return true;
    }
}

bool match_object_pattern(ObjectPattern *pattern, Value value, Environment *env) {
    if (value.type != VALUE_OBJECT) {
        return false;
    }
    
    Object *object = (Object *)value.as.object;
    
    // Match each property pattern
    for (size_t i = 0; i < pattern->property_count; i++) {
        PropertyPattern *prop_pattern = &pattern->properties[i];
        
        // Find property in object
        Value prop_value = object_get_property(object, prop_pattern->key);
        
        if (prop_value.type == VALUE_NULL && prop_pattern->required) {
            return false; // Required property not found
        }
        
        if (prop_value.type != VALUE_NULL) {
            if (prop_pattern->pattern) {
                // Match against nested pattern
                if (!pattern_match(prop_pattern->pattern, prop_value, env)) {
                    return false;
                }
            } else if (prop_pattern->identifier) {
                // Bind to identifier
                environment_define(env, prop_pattern->identifier, prop_value);
            }
        }
    }
    
    // Handle rest pattern
    if (pattern->has_rest) {
        Object *rest_object = create_object();
        
        // Add all properties not matched by patterns
        for (size_t i = 0; i < object->property_count; i++) {
            Property *prop = &object->properties[i];
            bool matched = false;
            
            for (size_t j = 0; j < pattern->property_count; j++) {
                if (strcmp(prop->key, pattern->properties[j].key) == 0) {
                    matched = true;
                    break;
                }
            }
            
            if (!matched) {
                object_set_property(rest_object, prop->key, prop->value);
            }
        }
        
        environment_define(env, pattern->rest_name, value_object(rest_object));
    }
    
    return true;
}

bool match_type_pattern(TypePattern *pattern, Value value) {
    switch (pattern->type) {
        case TYPE_NUMBER:
            return value.type == VALUE_NUMBER;
            
        case TYPE_STRING:
            return value.type == VALUE_STRING;
            
        case TYPE_BOOL:
            return value.type == VALUE_BOOL;
            
        case TYPE_NULL:
            return value.type == VALUE_NULL;
            
        case TYPE_ARRAY:
            return value.type == VALUE_ARRAY;
            
        case TYPE_OBJECT:
            return value.type == VALUE_OBJECT;
            
        case TYPE_FUNCTION:
            return value.type == VALUE_FUNCTION;
            
        case TYPE_CUSTOM:
            return check_custom_type(value, pattern->type_name);
            
        default:
            return false;
    }
}

bool match_guard_pattern(GuardPattern *pattern, Value value, Environment *env) {
    // First match the base pattern
    if (!pattern_match(pattern->base_pattern, value, env)) {
        return false;
    }
    
    // Then evaluate the guard condition
    Value guard_result = evaluate_expression(current_interpreter, pattern->guard_expr);
    return is_truthy(guard_result);
}

bool match_or_pattern(OrPattern *pattern, Value value, Environment *env) {
    // Try each alternative pattern
    for (size_t i = 0; i < pattern->pattern_count; i++) {
        Environment *temp_env = environment_create(env);
        
        if (pattern_match(pattern->patterns[i], value, temp_env)) {
            // Copy bindings from temp environment to main environment
            copy_environment_bindings(temp_env, env);
            return true;
        }
    }
    
    return false;
}

bool match_range_pattern(RangePattern *pattern, Value value) {
    if (value.type != VALUE_NUMBER) {
        return false;
    }
    
    double num = value.as.number;
    
    if (pattern->inclusive) {
        return num >= pattern->start && num <= pattern->end;
    } else {
        return num >= pattern->start && num < pattern->end;
    }
}

bool match_regex_pattern(RegexPattern *pattern, Value value) {
    if (value.type != VALUE_STRING) {
        return false;
    }
    
    regex_t regex;
    int result = regcomp(&regex, pattern->pattern, pattern->flags);
    
    if (result != 0) {
        return false; // Invalid regex
    }
    
    result = regexec(&regex, value.as.string, 0, NULL, 0);
    regfree(&regex);
    
    return result == 0;
}

bool match_constructor_pattern(ConstructorPattern *pattern, Value value, Environment *env) {
    if (value.type != VALUE_OBJECT) {
        return false;
    }
    
    Object *object = (Object *)value.as.object;
    
    // Check if object is instance of the constructor
    if (!is_instance_of(object, pattern->constructor_name)) {
        return false;
    }
    
    // Match constructor arguments if present
    if (pattern->arg_patterns) {
        // Extract constructor arguments from object
        Value *args = extract_constructor_args(object, pattern->constructor_name);
        
        for (size_t i = 0; i < pattern->arg_count; i++) {
            if (!pattern_match(pattern->arg_patterns[i], args[i], env)) {
                free(args);
                return false;
            }
        }
        
        free(args);
    }
    
    return true;
}

// Advanced pattern matching features
bool match_when_pattern(WhenPattern *pattern, Value value, Environment *env) {
    // Match base pattern first
    if (!pattern_match(pattern->base_pattern, value, env)) {
        return false;
    }
    
    // Evaluate when conditions
    for (size_t i = 0; i < pattern->condition_count; i++) {
        Value condition_result = evaluate_expression(current_interpreter, pattern->conditions[i]);
        if (!is_truthy(condition_result)) {
            return false;
        }
    }
    
    return true;
}

bool match_slice_pattern(SlicePattern *pattern, Value value, Environment *env) {
    if (value.type != VALUE_ARRAY && value.type != VALUE_STRING) {
        return false;
    }
    
    size_t length;
    if (value.type == VALUE_ARRAY) {
        Array *array = (Array *)value.as.object;
        length = array->length;
    } else {
        length = strlen(value.as.string);
    }
    
    // Calculate slice bounds
    size_t start = pattern->start >= 0 ? pattern->start : length + pattern->start;
    size_t end = pattern->end >= 0 ? pattern->end : length + pattern->end;
    
    if (start >= length || end > length || start >= end) {
        return false;
    }
    
    // Extract slice
    Value slice_value;
    if (value.type == VALUE_ARRAY) {
        Array *array = (Array *)value.as.object;
        Array *slice_array = create_array(end - start);
        
        for (size_t i = start; i < end; i++) {
            slice_array->elements[i - start] = array->elements[i];
        }
        
        slice_value = value_object(slice_array);
    } else {
        char *slice_str = malloc(end - start + 1);
        strncpy(slice_str, value.as.string + start, end - start);
        slice_str[end - start] = '\0';
        slice_value = value_string(slice_str);
    }
    
    // Match against pattern
    return pattern_match(pattern->pattern, slice_value, env);
}

bool match_nested_pattern(NestedPattern *pattern, Value value, Environment *env) {
    // Create nested environment
    Environment *nested_env = environment_create(env);
    
    // Match outer pattern
    if (!pattern_match(pattern->outer_pattern, value, nested_env)) {
        return false;
    }
    
    // Get bound value from outer pattern
    Value bound_value = environment_get(nested_env, pattern->binding_name);
    
    // Match inner pattern against bound value
    bool result = pattern_match(pattern->inner_pattern, bound_value, nested_env);
    
    if (result) {
        // Copy bindings to parent environment
        copy_environment_bindings(nested_env, env);
    }
    
    return result;
}

// Pattern compilation and optimization
CompiledPattern *compile_pattern(Pattern *pattern) {
    CompiledPattern *compiled = malloc(sizeof(CompiledPattern));
    compiled->original_pattern = pattern;
    compiled->optimization_level = 0;
    compiled->match_function = NULL;
    compiled->fast_path = NULL;
    
    // Analyze pattern for optimizations
    analyze_pattern_complexity(pattern, compiled);
    
    // Generate optimized matching code
    if (compiled->optimization_level > 0) {
        generate_fast_matcher(pattern, compiled);
    }
    
    return compiled;
}

void analyze_pattern_complexity(Pattern *pattern, CompiledPattern *compiled) {
    switch (pattern->type) {
        case PATTERN_LITERAL:
        case PATTERN_WILDCARD:
        case PATTERN_TYPE:
            compiled->optimization_level = 3; // Highly optimizable
            break;
            
        case PATTERN_IDENTIFIER:
            compiled->optimization_level = 2; // Moderately optimizable
            break;
            
        case PATTERN_TUPLE:
        case PATTERN_ARRAY:
        case PATTERN_OBJECT:
            compiled->optimization_level = 1; // Somewhat optimizable
            break;
            
        case PATTERN_GUARD:
        case PATTERN_OR:
        case PATTERN_REGEX:
            compiled->optimization_level = 0; // Not easily optimizable
            break;
            
        default:
            compiled->optimization_level = 0;
            break;
    }
}

void generate_fast_matcher(Pattern *pattern, CompiledPattern *compiled) {
    switch (pattern->type) {
        case PATTERN_LITERAL:
            compiled->fast_path = generate_literal_matcher(&pattern->as.literal);
            break;
            
        case PATTERN_TYPE:
            compiled->fast_path = generate_type_matcher(&pattern->as.type);
            break;
            
        case PATTERN_IDENTIFIER:
            compiled->fast_path = generate_identifier_matcher(&pattern->as.identifier);
            break;
            
        default:
            // Fall back to general matcher
            compiled->fast_path = NULL;
            break;
    }
}

FastMatcher *generate_literal_matcher(LiteralPattern *pattern) {
    FastMatcher *matcher = malloc(sizeof(FastMatcher));
    matcher->type = FAST_LITERAL;
    matcher->data.literal = *pattern;
    
    return matcher;
}

FastMatcher *generate_type_matcher(TypePattern *pattern) {
    FastMatcher *matcher = malloc(sizeof(FastMatcher));
    matcher->type = FAST_TYPE;
    matcher->data.type = *pattern;
    
    return matcher;
}

FastMatcher *generate_identifier_matcher(IdentifierPattern *pattern) {
    FastMatcher *matcher = malloc(sizeof(FastMatcher));
    matcher->type = FAST_IDENTIFIER;
    matcher->data.identifier = *pattern;
    
    return matcher;
}

bool execute_fast_matcher(FastMatcher *matcher, Value value, Environment *env) {
    switch (matcher->type) {
        case FAST_LITERAL:
            return match_literal_pattern(&matcher->data.literal, value);
            
        case FAST_TYPE:
            return match_type_pattern(&matcher->data.type, value);
            
        case FAST_IDENTIFIER:
            return match_identifier_pattern(&matcher->data.identifier, value, env);
            
        default:
            return false;
    }
}

// Pattern matching statistics and profiling
void pattern_match_profile_start(PatternMatchProfile *profile) {
    profile->start_time = clock();
    profile->match_attempts++;
}

void pattern_match_profile_end(PatternMatchProfile *profile, bool matched) {
    profile->total_time += clock() - profile->start_time;
    
    if (matched) {
        profile->successful_matches++;
    } else {
        profile->failed_matches++;
    }
}

void print_pattern_match_statistics(PatternMatchProfile *profile) {
    printf("Pattern Match Statistics:\n");
    printf("  Total attempts: %zu\n", profile->match_attempts);
    printf("  Successful matches: %zu\n", profile->successful_matches);
    printf("  Failed matches: %zu\n", profile->failed_matches);
    printf("  Success rate: %.2f%%\n", 
           (double)profile->successful_matches / profile->match_attempts * 100);
    printf("  Average time: %.4f ms\n", 
           (double)profile->total_time / profile->match_attempts / CLOCKS_PER_SEC * 1000);
}

// Utility functions
bool check_type_constraint(Value value, TypeConstraint *constraint) {
    switch (constraint->type) {
        case CONSTRAINT_EXACT_TYPE:
            return value.type == constraint->expected_type;
            
        case CONSTRAINT_NUMERIC:
            return value.type == VALUE_NUMBER;
            
        case CONSTRAINT_COMPARABLE:
            return value.type == VALUE_NUMBER || value.type == VALUE_STRING;
            
        case CONSTRAINT_ITERABLE:
            return value.type == VALUE_ARRAY || value.type == VALUE_STRING;
            
        case CONSTRAINT_CUSTOM:
            return check_custom_constraint(value, constraint->custom_checker);
            
        default:
            return true;
    }
}

bool check_custom_type(Value value, const char *type_name) {
    if (value.type != VALUE_OBJECT) {
        return false;
    }
    
    Object *object = (Object *)value.as.object;
    return strcmp(object->type_name, type_name) == 0;
}

bool is_instance_of(Object *object, const char *constructor_name) {
    return strcmp(object->constructor_name, constructor_name) == 0;
}

Value *extract_constructor_args(Object *object, const char *constructor_name) {
    // This would extract the original constructor arguments from the object
    // Implementation depends on how constructor arguments are stored
    return NULL; // Simplified for now
}

void copy_environment_bindings(Environment *source, Environment *dest) {
    for (size_t i = 0; i < source->count; i++) {
        environment_define(dest, source->variables[i].name, source->variables[i].value);
    }
}

Array *create_array(size_t length) {
    Array *array = malloc(sizeof(Array));
    array->elements = malloc(sizeof(Value) * length);
    array->length = length;
    array->capacity = length;
    
    return array;
}

Object *create_object(void) {
    Object *object = malloc(sizeof(Object));
    object->properties = NULL;
    object->property_count = 0;
    object->property_capacity = 0;
    object->type_name = "Object";
    object->constructor_name = "Object";
    
    return object;
}

Value object_get_property(Object *object, const char *key) {
    for (size_t i = 0; i < object->property_count; i++) {
        if (strcmp(object->properties[i].key, key) == 0) {
            return object->properties[i].value;
        }
    }
    
    return value_null();
}

void object_set_property(Object *object, const char *key, Value value) {
    // Check if property already exists
    for (size_t i = 0; i < object->property_count; i++) {
        if (strcmp(object->properties[i].key, key) == 0) {
            object->properties[i].value = value;
            return;
        }
    }
    
    // Add new property
    if (object->property_count >= object->property_capacity) {
        object->property_capacity = object->property_capacity ? object->property_capacity * 2 : 8;
        object->properties = realloc(object->properties, 
                                   sizeof(Property) * object->property_capacity);
    }
    
    object->properties[object->property_count].key = strdup(key);
    object->properties[object->property_count].value = value;
    object->property_count++;
}

bool check_custom_constraint(Value value, CustomConstraintChecker checker) {
    return checker(value);
}

// Pattern matching DSL compiler
Pattern *parse_pattern_string(const char *pattern_str) {
    PatternLexer lexer;
    pattern_lexer_init(&lexer, pattern_str);
    
    PatternParser parser;
    pattern_parser_init(&parser, &lexer);
    
    return pattern_parser_parse(&parser);
}

void pattern_lexer_init(PatternLexer *lexer, const char *input) {
    lexer->input = input;
    lexer->position = 0;
    lexer->length = strlen(input);
    lexer->current_token = pattern_lexer_next_token(lexer);
}

PatternToken pattern_lexer_next_token(PatternLexer *lexer) {
    // Skip whitespace
    while (lexer->position < lexer->length && 
           isspace(lexer->input[lexer->position])) {
        lexer->position++;
    }
    
    if (lexer->position >= lexer->length) {
        return (PatternToken){PATTERN_TOKEN_EOF, "", 0};
    }
    
    char c = lexer->input[lexer->position];
    
    switch (c) {
        case '(':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_LPAREN, "(", 1};
            
        case ')':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_RPAREN, ")", 1};
            
        case '[':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_LBRACKET, "[", 1};
            
        case ']':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_RBRACKET, "]", 1};
            
        case '{':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_LBRACE, "{", 1};
            
        case '}':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_RBRACE, "}", 1};
            
        case ',':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_COMMA, ",", 1};
            
        case '|':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_PIPE, "|", 1};
            
        case '_':
            lexer->position++;
            return (PatternToken){PATTERN_TOKEN_WILDCARD, "_", 1};
            
        default:
            if (isdigit(c) || c == '-') {
                return pattern_lexer_read_number(lexer);
            } else if (isalpha(c) || c == '_') {
                return pattern_lexer_read_identifier(lexer);
            } else if (c == '"' || c == '\'') {
                return pattern_lexer_read_string(lexer);
            } else {
                lexer->position++;
                return (PatternToken){PATTERN_TOKEN_UNKNOWN, &c, 1};
            }
    }
}

PatternToken pattern_lexer_read_number(PatternLexer *lexer) {
    size_t start = lexer->position;
    
    if (lexer->input[lexer->position] == '-') {
        lexer->position++;
    }
    
    while (lexer->position < lexer->length && 
           (isdigit(lexer->input[lexer->position]) || 
            lexer->input[lexer->position] == '.')) {
        lexer->position++;
    }
    
    size_t length = lexer->position - start;
    char *text = malloc(length + 1);
    strncpy(text, lexer->input + start, length);
    text[length] = '\0';
    
    return (PatternToken){PATTERN_TOKEN_NUMBER, text, length};
}

PatternToken pattern_lexer_read_identifier(PatternLexer *lexer) {
    size_t start = lexer->position;
    
    while (lexer->position < lexer->length && 
           (isalnum(lexer->input[lexer->position]) || 
            lexer->input[lexer->position] == '_')) {
        lexer->position++;
    }
    
    size_t length = lexer->position - start;
    char *text = malloc(length + 1);
    strncpy(text, lexer->input + start, length);
    text[length] = '\0';
    
    return (PatternToken){PATTERN_TOKEN_IDENTIFIER, text, length};
}

PatternToken pattern_lexer_read_string(PatternLexer *lexer) {
    char quote = lexer->input[lexer->position];
    lexer->position++; // Skip opening quote
    
    size_t start = lexer->position;
    
    while (lexer->position < lexer->length && 
           lexer->input[lexer->position] != quote) {
        if (lexer->input[lexer->position] == '\\') {
            lexer->position++; // Skip escape character
        }
        lexer->position++;
    }
    
    size_t length = lexer->position - start;
    lexer->position++; // Skip closing quote
    
    char *text = malloc(length + 1);
    strncpy(text, lexer->input + start, length);
    text[length] = '\0';
    
    return (PatternToken){PATTERN_TOKEN_STRING, text, length};
}

void pattern_parser_init(PatternParser *parser, PatternLexer *lexer) {
    parser->lexer = lexer;
    parser->had_error = false;
}

Pattern *pattern_parser_parse(PatternParser *parser) {
    return pattern_parser_parse_pattern(parser);
}

Pattern *pattern_parser_parse_pattern(PatternParser *parser) {
    return pattern_parser_parse_or_pattern(parser);
}

Pattern *pattern_parser_parse_or_pattern(PatternParser *parser) {
    Pattern *left = pattern_parser_parse_primary_pattern(parser);
    
    if (parser->lexer->current_token.type == PATTERN_TOKEN_PIPE) {
        OrPattern or_pattern;
        or_pattern.patterns = malloc(sizeof(Pattern *) * 16);
        or_pattern.patterns[0] = left;
        or_pattern.pattern_count = 1;
        
        while (parser->lexer->current_token.type == PATTERN_TOKEN_PIPE) {
            pattern_lexer_advance(parser->lexer);
            or_pattern.patterns[or_pattern.pattern_count++] = 
                pattern_parser_parse_primary_pattern(parser);
        }
        
        Pattern *pattern = malloc(sizeof(Pattern));
        pattern->type = PATTERN_OR;
        pattern->as.or_pattern = or_pattern;
        
        return pattern;
    }
    
    return left;
}

Pattern *pattern_parser_parse_primary_pattern(PatternParser *parser) {
    PatternToken token = parser->lexer->current_token;
    
    switch (token.type) {
        case PATTERN_TOKEN_WILDCARD:
            pattern_lexer_advance(parser->lexer);
            return create_wildcard_pattern();
            
        case PATTERN_TOKEN_NUMBER:
            pattern_lexer_advance(parser->lexer);
            return create_literal_pattern_number(atof(token.text));
            
        case PATTERN_TOKEN_STRING:
            pattern_lexer_advance(parser->lexer);
            return create_literal_pattern_string(token.text);
            
        case PATTERN_TOKEN_IDENTIFIER:
            pattern_lexer_advance(parser->lexer);
            return create_identifier_pattern(token.text);
            
        case PATTERN_TOKEN_LPAREN:
            return pattern_parser_parse_tuple_pattern(parser);
            
        case PATTERN_TOKEN_LBRACKET:
            return pattern_parser_parse_array_pattern(parser);
            
        case PATTERN_TOKEN_LBRACE:
            return pattern_parser_parse_object_pattern(parser);
            
        default:
            parser->had_error = true;
            return NULL;
    }
}

Pattern *create_wildcard_pattern(void) {
    Pattern *pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_WILDCARD;
    return pattern;
}

Pattern *create_literal_pattern_number(double value) {
    Pattern *pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_LITERAL;
    pattern->as.literal.value_type = VALUE_NUMBER;
    pattern->as.literal.as.number = value;
    return pattern;
}

Pattern *create_literal_pattern_string(const char *value) {
    Pattern *pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_LITERAL;
    pattern->as.literal.value_type = VALUE_STRING;
    pattern->as.literal.as.string = strdup(value);
    return pattern;
}

Pattern *create_identifier_pattern(const char *name) {
    Pattern *pattern = malloc(sizeof(Pattern));
    pattern->type = PATTERN_IDENTIFIER;
    pattern->as.identifier.name = strdup(name);
    pattern->as.identifier.type_constraint = NULL;
    return pattern;
}

void pattern_lexer_advance(PatternLexer *lexer) {
    lexer->current_token = pattern_lexer_next_token(lexer);
}