#ifndef RUBOLT_INTERPRETER_H
#define RUBOLT_INTERPRETER_H

#include "ast.h"
#include <stdbool.h>

#define MAX_VARS 256
#define MAX_FUNCTIONS 128

typedef struct {
    char* name;
    Value value;
    bool is_const;
} Variable;

typedef struct {
    FunctionStmt* fn;
} Function;

typedef struct {
    Variable vars[MAX_VARS];
    size_t var_count;
    Function functions[MAX_FUNCTIONS];
    size_t function_count;
    bool has_return;
    Value return_value;
} Environment;

void env_init(Environment* env);
void env_free(Environment* env);
bool env_define_var(Environment* env, const char* name, Value value, bool is_const);
bool env_set_var(Environment* env, const char* name, Value value);
bool env_get_var(Environment* env, const char* name, Value* out);
bool env_define_function(Environment* env, FunctionStmt* fn);
Function* env_get_function(Environment* env, const char* name);

Value eval_expr(Environment* env, Expr* expr);
void exec_stmt(Environment* env, Stmt* stmt);
void interpret(Stmt** statements, size_t count);

#endif
