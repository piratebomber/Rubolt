#ifndef RUBOLT_ERROR_HANDLING_H
#define RUBOLT_ERROR_HANDLING_H

#include "ast.h"

typedef enum {
    RESULT_OK,
    RESULT_ERROR
} ResultType;

typedef struct {
    ResultType type;
    union {
        Value ok_value;
        struct {
            char* message;
            char* error_type;
            int line;
            int column;
        } error;
    } as;
} Result;

typedef struct ErrorHandler {
    char* error_type;
    Stmt** handler_body;
    size_t handler_count;
    struct ErrorHandler* next;
} ErrorHandler;

typedef struct {
    Stmt** try_body;
    size_t try_count;
    ErrorHandler* handlers;
    Stmt** finally_body;
    size_t finally_count;
} TryStmt;

typedef struct {
    char* error_type;
    Expr* message;
} ThrowStmt;

// Result operations
Result result_ok(Value value);
Result result_error(const char* message, const char* error_type, int line, int column);
void result_free(Result* result);
bool result_is_ok(Result result);
bool result_is_error(Result result);
Value result_unwrap(Result result);
Value result_unwrap_or(Result result, Value default_value);

// Error handling statements
Stmt* stmt_try(Stmt** try_body, size_t try_count, ErrorHandler* handlers, 
               Stmt** finally_body, size_t finally_count);
Stmt* stmt_throw(const char* error_type, Expr* message);

// Error handler operations
ErrorHandler* error_handler_create(const char* error_type, Stmt** body, size_t count);
void error_handler_free(ErrorHandler* handler);

// Built-in error types
extern const char* ERROR_TYPE_RUNTIME;
extern const char* ERROR_TYPE_TYPE;
extern const char* ERROR_TYPE_INDEX;
extern const char* ERROR_TYPE_KEY;
extern const char* ERROR_TYPE_NULL;
extern const char* ERROR_TYPE_DIVISION_BY_ZERO;
extern const char* ERROR_TYPE_FILE_NOT_FOUND;
extern const char* ERROR_TYPE_NETWORK;

// Error propagation
Result propagate_error(Result result);
void set_error_context(const char* function, int line);

#endif