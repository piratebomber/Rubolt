#ifndef RUBOLT_INTERPRETER_H
#define RUBOLT_INTERPRETER_H

#include "ast.h"
#include <stdbool.h>
#include <setjmp.h>

#define MAX_VARS 256
#define MAX_FUNCTIONS 128
#define MAX_CALL_FRAMES 256
#define JIT_THRESHOLD 10

typedef enum {
    VALUE_NULL,
    VALUE_BOOL,
    VALUE_NUMBER,
    VALUE_STRING,
    VALUE_OBJECT,
    VALUE_ARRAY,
    VALUE_FUNCTION
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        char* string;
        void* object;
        struct {
            struct Value* elements;
            size_t count;
        } array;
        struct {
            FunctionStmt* declaration;
            struct Environment* closure;
            bool is_native;
            void* native_func;
        } function;
    } as;
} Value;

typedef struct {
    char* name;
    Value value;
    bool is_const;
} Variable;

typedef struct {
    FunctionStmt* declaration;
    struct Environment* closure;
    size_t call_count;
    bool jit_compiled;
    void* native_code;
} Function;

typedef struct Environment {
    struct Environment* parent;
    Variable* variables;
    size_t capacity;
    size_t count;
} Environment;

typedef struct {
    Function* function;
    Environment* env;
    size_t ip;
} CallFrame;

typedef struct {
    Environment* global_env;
    Environment* current_env;
    CallFrame* call_stack;
    size_t call_stack_size;
    size_t call_stack_capacity;
    bool return_flag;
    Value return_value;
    bool break_flag;
    bool continue_flag;
    char* break_label;
    char* continue_label;
    bool jit_enabled;
    bool async_enabled;
    struct ExceptionHandler* current_handler;
} Interpreter;

typedef struct ExceptionHandler {
    jmp_buf jump_buffer;
    struct ExceptionHandler* previous;
    struct Exception* current_exception;
} ExceptionHandler;

typedef struct Exception {
    char* type;
    char* message;
    Value data;
} Exception;

// Value operations
Value value_number(double num);
Value value_string(const char* str);
Value value_bool(bool b);
Value value_null(void);
Value value_object(void* obj);
Value value_array(Value* elements, size_t count);
Value value_function(FunctionStmt* decl, Environment* closure);
bool is_truthy(Value value);
void value_print(Value value);

// Environment management
Environment* environment_create(Environment* parent);
void environment_define(Environment* env, const char* name, Value value);
Value environment_get(Environment* env, const char* name);
void environment_set(Environment* env, const char* name, Value value);

// Interpreter
Interpreter* interpreter_create(void);
Value interpret(Interpreter* interp, Stmt** statements, size_t stmt_count);
void interpreter_cleanup(Interpreter* interp);

// Expression evaluation
Value evaluate_expression(Interpreter* interp, Expr* expr);
Value evaluate_literal(Interpreter* interp, LiteralExpr* expr);
Value evaluate_binary(Interpreter* interp, BinaryExpr* expr);
Value evaluate_unary(Interpreter* interp, UnaryExpr* expr);
Value evaluate_call(Interpreter* interp, CallExpr* expr);
Value evaluate_member(Interpreter* interp, MemberExpr* expr);
Value evaluate_index(Interpreter* interp, IndexExpr* expr);
Value evaluate_assignment(Interpreter* interp, AssignmentExpr* expr);
Value evaluate_function(Interpreter* interp, FunctionExpr* expr);
Value evaluate_array(Interpreter* interp, ArrayExpr* expr);

// Statement execution
Value execute_statement(Interpreter* interp, Stmt* stmt);
Value execute_var_decl(Interpreter* interp, VarDeclStmt* stmt);
Value execute_function_decl(Interpreter* interp, FunctionStmt* stmt);
Value execute_if(Interpreter* interp, IfStmt* stmt);
Value execute_while(Interpreter* interp, WhileStmt* stmt);
Value execute_for(Interpreter* interp, ForStmt* stmt);
Value execute_for_in(Interpreter* interp, ForInStmt* stmt);
Value execute_do_while(Interpreter* interp, DoWhileStmt* stmt);
Value execute_return(Interpreter* interp, ReturnStmt* stmt);
Value execute_block(Interpreter* interp, BlockStmt* stmt);
Value execute_break(Interpreter* interp, BreakStmt* stmt);
Value execute_continue(Interpreter* interp, ContinueStmt* stmt);

// Built-in functions
Value builtin_print(Environment* env, Value* args, size_t arg_count);
Value builtin_len(Environment* env, Value* args, size_t arg_count);
Value builtin_type(Environment* env, Value* args, size_t arg_count);
Value builtin_range(Environment* env, Value* args, size_t arg_count);

// Nested function support
Value call_nested_function(Interpreter* interp, Function* func, Value* args, size_t arg_count);
Environment* create_function_environment(Interpreter* interp, Function* func, Value* args, size_t arg_count);

// Range type for iteration
typedef struct {
    int start;
    int end;
    int step;
} Range;

// Loop control
typedef struct {
    char* label;
    bool active;
} LoopContext;

#endif
