#ifndef RUBOLT_PATTERN_MATCH_H
#define RUBOLT_PATTERN_MATCH_H

#include "ast.h"

typedef enum {
    PATTERN_LITERAL,     // 42, "hello", true
    PATTERN_IDENTIFIER,  // x, name
    PATTERN_WILDCARD,    // _
    PATTERN_TUPLE,       // (x, y, z)
    PATTERN_LIST,        // [head, ...tail]
    PATTERN_OBJECT,      // {name, age}
    PATTERN_TYPE,        // String, Number
    PATTERN_GUARD        // x if x > 0
} PatternType;

typedef struct Pattern Pattern;

typedef struct {
    Pattern** patterns;
    size_t count;
} TuplePattern;

typedef struct {
    Pattern** patterns;
    size_t count;
    bool has_rest;
    char* rest_name;
} ListPattern;

typedef struct {
    char** keys;
    Pattern** patterns;
    size_t count;
    bool has_rest;
} ObjectPattern;

typedef struct {
    Pattern* pattern;
    Expr* guard;
} GuardPattern;

struct Pattern {
    PatternType type;
    union {
        Value literal;
        char* identifier;
        TuplePattern tuple;
        ListPattern list;
        ObjectPattern object;
        char* type_name;
        GuardPattern guard;
    } as;
};

typedef struct {
    Pattern* pattern;
    Stmt** body;
    size_t body_count;
} MatchCase;

typedef struct {
    Expr* expr;
    MatchCase* cases;
    size_t case_count;
} MatchExpr;

// Pattern constructors
Pattern* pattern_literal(Value value);
Pattern* pattern_identifier(const char* name);
Pattern* pattern_wildcard();
Pattern* pattern_tuple(Pattern** patterns, size_t count);
Pattern* pattern_list(Pattern** patterns, size_t count, bool has_rest, const char* rest_name);
Pattern* pattern_object(char** keys, Pattern** patterns, size_t count, bool has_rest);
Pattern* pattern_type(const char* type_name);
Pattern* pattern_guard(Pattern* pattern, Expr* guard);
void pattern_free(Pattern* pattern);

// Match expression constructor
Expr* expr_match(Expr* expr, MatchCase* cases, size_t case_count);

// Pattern matching engine
bool pattern_matches(Pattern* pattern, Value value, Environment* env);
void pattern_bind(Pattern* pattern, Value value, Environment* env);

#endif