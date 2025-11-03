#ifndef RUBOLT_TYPECHECKER_H
#define RUBOLT_TYPECHECKER_H

#include "ast.h"
#include <stdbool.h>

typedef enum {
    TYPE_UNKNOWN,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_ANY,
    TYPE_NULL,
    TYPE_FUNCTION
} TypeKind;

typedef struct {
    TypeKind kind;
    char* name;
} Type;

typedef struct {
    char* message;
    int line;
    int column;
    char* file;
    char* hint;
} TypeError;

typedef struct {
    TypeError* errors;
    size_t error_count;
    size_t error_capacity;
} TypeChecker;

void typechecker_init(TypeChecker* tc);
void typechecker_free(TypeChecker* tc);
void typechecker_add_error(TypeChecker* tc, const char* message, int line, int column, const char* hint);
bool typechecker_check_program(TypeChecker* tc, Stmt** statements, size_t count);
void typechecker_print_errors(TypeChecker* tc);
Type* type_from_string(const char* type_name);
bool types_compatible(Type* expected, Type* actual);
const char* type_to_string(Type* type);

#endif
