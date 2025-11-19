#include "error_handling.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Built-in error types
const char* ERROR_TYPE_RUNTIME = "RuntimeError";
const char* ERROR_TYPE_TYPE = "TypeError";
const char* ERROR_TYPE_INDEX = "IndexError";
const char* ERROR_TYPE_KEY = "KeyError";
const char* ERROR_TYPE_NULL = "NullError";
const char* ERROR_TYPE_DIVISION_BY_ZERO = "DivisionByZeroError";
const char* ERROR_TYPE_FILE_NOT_FOUND = "FileNotFoundError";
const char* ERROR_TYPE_NETWORK = "NetworkError";

Result result_ok(Value value) {
    Result result;
    result.type = RESULT_OK;
    result.as.ok_value = value;
    return result;
}

Result result_error(const char* message, const char* error_type, int line, int column) {
    Result result;
    result.type = RESULT_ERROR;
    result.as.error.message = strdup(message);
    result.as.error.error_type = strdup(error_type);
    result.as.error.line = line;
    result.as.error.column = column;
    return result;
}

void result_free(Result* result) {
    if (result->type == RESULT_ERROR) {
        free(result->as.error.message);
        free(result->as.error.error_type);
    } else if (result->type == RESULT_OK) {
        value_free(&result->as.ok_value);
    }
}

bool result_is_ok(Result result) {
    return result.type == RESULT_OK;
}

bool result_is_error(Result result) {
    return result.type == RESULT_ERROR;
}

Value result_unwrap(Result result) {
    if (result.type == RESULT_OK) {
        return result.as.ok_value;
    } else {
        ErrorContext ctx = {
            .error_type = result.as.error.error_type,
            .message = result.as.error.message,
            .line = result.as.error.line,
            .column = result.as.error.column
        };
        
        runtime_panic_with_context(PANIC_CUSTOM, &ctx, sizeof(ErrorContext),
                                   "Result unwrap failed: %s: %s at line %d, column %d",
                                   result.as.error.error_type,
                                   result.as.error.message,
                                   result.as.error.line,
                                   result.as.error.column);
        
        // This should never be reached due to panic, but return null for safety
        return value_null();
    }
}

Value result_unwrap_or(Result result, Value default_value) {
    if (result.type == RESULT_OK) {
        return result.as.ok_value;
    } else {
        return default_value;
    }
}

ErrorHandler* error_handler_create(const char* error_type, Stmt** body, size_t count) {
    ErrorHandler* handler = malloc(sizeof(ErrorHandler));
    handler->error_type = strdup(error_type);
    handler->handler_body = body;
    handler->handler_count = count;
    handler->next = NULL;
    return handler;
}

void error_handler_free(ErrorHandler* handler) {
    while (handler) {
        ErrorHandler* next = handler->next;
        free(handler->error_type);
        
        for (size_t i = 0; i < handler->handler_count; i++) {
            stmt_free(handler->handler_body[i]);
        }
        free(handler->handler_body);
        free(handler);
        handler = next;
    }
}

Stmt* stmt_try(Stmt** try_body, size_t try_count, ErrorHandler* handlers,
               Stmt** finally_body, size_t finally_count) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_TRY;
    stmt->as.try_stmt.try_body = try_body;
    stmt->as.try_stmt.try_count = try_count;
    stmt->as.try_stmt.handlers = handlers;
    stmt->as.try_stmt.finally_body = finally_body;
    stmt->as.try_stmt.finally_count = finally_count;
    return stmt;
}

Stmt* stmt_throw(const char* error_type, Expr* message) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_THROW;
    stmt->as.throw_stmt.error_type = strdup(error_type);
    stmt->as.throw_stmt.message = message;
    return stmt;
}

Result propagate_error(Result result) {
    // In a full implementation, this would add stack trace information
    return result;
}

static char error_context_function[256] = "";
static int error_context_line = 0;

void set_error_context(const char* function, int line) {
    strncpy(error_context_function, function, sizeof(error_context_function) - 1);
    error_context_line = line;
}

// Safe division with error handling
Result safe_divide(double a, double b) {
    if (b == 0.0) {
        return result_error("Division by zero", ERROR_TYPE_DIVISION_BY_ZERO, 
                          error_context_line, 0);
    }
    return result_ok(value_number(a / b));
}

// Safe array access with error handling
Result safe_array_get(Value array, int index) {
    if (array.type != VAL_LIST) {
        return result_error("Not a list", ERROR_TYPE_TYPE, error_context_line, 0);
    }
    
    if (index < 0 || index >= (int)array.as.list.count) {
        char message[256];
        snprintf(message, sizeof(message), "Index %d out of bounds for list of size %zu",
                index, array.as.list.count);
        return result_error(message, ERROR_TYPE_INDEX, error_context_line, 0);
    }
    
    return result_ok(array.as.list.elements[index]);
}

// Safe dictionary access with error handling
Result safe_dict_get(Value dict, const char* key) {
    if (dict.type != VAL_DICT) {
        return result_error("Not a dictionary", ERROR_TYPE_TYPE, error_context_line, 0);
    }
    
    Value* value = dict_get(&dict, key);
    if (!value) {
        char message[256];
        snprintf(message, sizeof(message), "Key '%s' not found", key);
        return result_error(message, ERROR_TYPE_KEY, error_context_line, 0);
    }
    
    return result_ok(*value);
}

// Safe null check
Result safe_null_check(Value value, const char* operation) {
    if (value.type == VAL_NULL) {
        char message[256];
        snprintf(message, sizeof(message), "Cannot perform %s on null value", operation);
        return result_error(message, ERROR_TYPE_NULL, error_context_line, 0);
    }
    return result_ok(value);
}

typedef struct ErrorContext {
    char* error_type;
    char* message;
    int line;
    int column;
} ErrorContext;

typedef struct ExceptionFrame {
    jmp_buf jump_buffer;
    ErrorHandler* handlers;
    Stmt** finally_body;
    size_t finally_count;
    Environment* env;
    struct ExceptionFrame* parent;
} ExceptionFrame;

static ExceptionFrame* g_exception_stack = NULL;

void push_exception_frame(ExceptionFrame* frame) {
    frame->parent = g_exception_stack;
    g_exception_stack = frame;
}

void pop_exception_frame(void) {
    if (g_exception_stack) {
        g_exception_stack = g_exception_stack->parent;
    }
}

Result execute_try_stmt_advanced(TryStmt* try_stmt, Environment* env) {
    ExceptionFrame frame = {0};
    frame.handlers = try_stmt->handlers;
    frame.finally_body = try_stmt->finally_body;
    frame.finally_count = try_stmt->finally_count;
    frame.env = env;
    
    push_exception_frame(&frame);
    
    Result result = result_ok(value_null());
    bool exception_thrown = false;
    
    // Set up exception handling
    if (setjmp(frame.jump_buffer) == 0) {
        // Execute try block
        for (size_t i = 0; i < try_stmt->try_count; i++) {
            result = execute_stmt_safe(try_stmt->try_body[i], env);
            if (result_is_error(result)) {
                exception_thrown = true;
                break;
            }
        }
    } else {
        // Exception was thrown, handled by longjmp
        exception_thrown = true;
        result = get_current_exception();
    }
    
    // Handle exceptions
    if (exception_thrown && result_is_error(result)) {
        ErrorHandler* handler = try_stmt->handlers;
        bool handled = false;
        
        while (handler && !handled) {
            if (error_type_matches(handler->error_type, result.as.error.error_type)) {
                // Create error binding environment
                Environment handler_env;
                environment_init(&handler_env);
                environment_set_parent(&handler_env, env);
                
                // Bind error object
                Value error_obj = create_error_object(&result.as.error);
                environment_define(&handler_env, "error", error_obj);
                environment_define(&handler_env, "e", error_obj);
                
                // Execute handler
                result_free(&result);
                result = result_ok(value_null());
                
                for (size_t i = 0; i < handler->handler_count; i++) {
                    Result handler_result = execute_stmt_safe(handler->handler_body[i], &handler_env);
                    if (result_is_error(handler_result)) {
                        result = handler_result;
                        break;
                    }
                    if (handler_result.type == RESULT_OK) {
                        result = handler_result;
                    }
                }
                
                environment_free(&handler_env);
                handled = true;
            }
            handler = handler->next;
        }
        
        // If not handled, propagate the exception
        if (!handled) {
            pop_exception_frame();
            if (g_exception_stack) {
                longjmp(g_exception_stack->jump_buffer, 1);
            } else {
                // No exception handler, convert to panic
                ErrorContext ctx = {
                    .error_type = result.as.error.error_type,
                    .message = result.as.error.message,
                    .line = result.as.error.line,
                    .column = result.as.error.column
                };
                
                runtime_panic_with_context(PANIC_CUSTOM, &ctx, sizeof(ErrorContext),
                                           "Unhandled exception: %s: %s",
                                           result.as.error.error_type,
                                           result.as.error.message);
            }
        }
    }
    
    // Execute finally block
    if (try_stmt->finally_body) {
        for (size_t i = 0; i < try_stmt->finally_count; i++) {
            Result finally_result = execute_stmt_safe(try_stmt->finally_body[i], env);
            
            // Finally block errors override try/catch results
            if (result_is_error(finally_result)) {
                if (result_is_ok(result)) {
                    result_free(&result);
                }
                result = finally_result;
            }
        }
    }
    
    pop_exception_frame();
    return result;
}

bool error_type_matches(const char* handler_type, const char* error_type) {
    // Exact match
    if (strcmp(handler_type, error_type) == 0) {
        return true;
    }
    
    // Catch-all handler
    if (strcmp(handler_type, "*") == 0 || strcmp(handler_type, "Exception") == 0) {
        return true;
    }
    
    // Check inheritance hierarchy
    return error_type_is_subtype(error_type, handler_type);
}

bool error_type_is_subtype(const char* error_type, const char* parent_type) {
    // Define error type hierarchy
    static const char* error_hierarchy[][2] = {
        {"TypeError", "RuntimeError"},
        {"IndexError", "RuntimeError"},
        {"KeyError", "RuntimeError"},
        {"NullError", "RuntimeError"},
        {"FileNotFoundError", "IOError"},
        {"NetworkError", "IOError"},
        {"IOError", "RuntimeError"},
        {"ValueError", "RuntimeError"},
        {"DivisionByZeroError", "ArithmeticError"},
        {"ArithmeticError", "RuntimeError"},
        {NULL, NULL}
    };
    
    for (size_t i = 0; error_hierarchy[i][0] != NULL; i++) {
        if (strcmp(error_type, error_hierarchy[i][0]) == 0) {
            if (strcmp(parent_type, error_hierarchy[i][1]) == 0) {
                return true;
            }
            // Recursive check
            return error_type_is_subtype(error_hierarchy[i][1], parent_type);
        }
    }
    
    return false;
}

Value create_error_object(ErrorInfo* error) {
    Value error_obj = value_dict();
    
    dict_set(&error_obj, "type", value_string(error->error_type));
    dict_set(&error_obj, "message", value_string(error->message));
    dict_set(&error_obj, "line", value_number((double)error->line));
    dict_set(&error_obj, "column", value_number((double)error->column));
    
    // Add stack trace if available
    StackFrame* stack = stack_trace_capture();
    if (stack) {
        char* stack_str = stack_trace_to_string(stack);
        dict_set(&error_obj, "stack_trace", value_string(stack_str));
        free(stack_str);
        stack_trace_free(stack);
    }
    
    return error_obj;
}

Result execute_stmt_safe(Stmt* stmt, Environment* env) {
    // Wrap statement execution with exception handling
    if (g_exception_stack) {
        // We're in a try block, use normal execution
        return execute_stmt(stmt, env);
    } else {
        // No exception handler, convert errors to panics
        Result result = execute_stmt(stmt, env);
        if (result_is_error(result)) {
            ErrorContext ctx = {
                .error_type = result.as.error.error_type,
                .message = result.as.error.message,
                .line = result.as.error.line,
                .column = result.as.error.column
            };
            
            runtime_panic_with_context(PANIC_CUSTOM, &ctx, sizeof(ErrorContext),
                                       "Unhandled error in statement execution: %s: %s",
                                       result.as.error.error_type,
                                       result.as.error.message);
        }
        return result;
    }
}

static Result g_current_exception = {0};

Result get_current_exception(void) {
    return g_current_exception;
}

void set_current_exception(Result exception) {
    if (g_current_exception.type == RESULT_ERROR) {
        result_free(&g_current_exception);
    }
    g_current_exception = exception;
}

Result execute_try_stmt(TryStmt* try_stmt, Environment* env) {
    return execute_try_stmt_advanced(try_stmt, env);
}

// Throw statement execution
Result execute_throw_stmt(ThrowStmt* throw_stmt, Environment* env) {
    Value message_value = evaluate_expr(throw_stmt->message, env);
    const char* message = (message_value.type == VAL_STRING) ? 
                         message_value.as.string : "Unknown error";
    
    return result_error(message, throw_stmt->error_type, error_context_line, 0);
}