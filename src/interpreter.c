#include "interpreter.h"
#include "ast.h"
#include "gc/gc.h"
#include "rc/rc.h"
#include "jit_compiler.h"
#include "pattern_match.h"
#include "async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Global interpreter state
static Interpreter *current_interpreter = NULL;

// Value creation functions
Value value_number(double num) {
    Value val = {VALUE_NUMBER, {.number = num}};
    return val;
}

Value value_string(const char *str) {
    Value val = {VALUE_STRING, {.string = strdup(str)}};
    return val;
}

Value value_bool(bool b) {
    Value val = {VALUE_BOOL, {.boolean = b}};
    return val;
}

Value value_null(void) {
    Value val = {VALUE_NULL, {.number = 0}};
    return val;
}

Value value_object(void *obj) {
    Value val = {VALUE_OBJECT, {.object = obj}};
    return val;
}

// Environment management
Environment *environment_create(Environment *parent) {
    Environment *env = malloc(sizeof(Environment));
    env->parent = parent;
    env->variables = malloc(sizeof(Variable) * 64);
    env->capacity = 64;
    env->count = 0;
    return env;
}

void environment_define(Environment *env, const char *name, Value value) {
    if (env->count >= env->capacity) {
        env->capacity *= 2;
        env->variables = realloc(env->variables, sizeof(Variable) * env->capacity);
    }
    
    env->variables[env->count].name = strdup(name);
    env->variables[env->count].value = value;
    env->count++;
}

Value environment_get(Environment *env, const char *name) {
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            return env->variables[i].value;
        }
    }
    
    if (env->parent) {
        return environment_get(env->parent, name);
    }
    
    return value_null();
}

void environment_set(Environment *env, const char *name, Value value) {
    for (size_t i = 0; i < env->count; i++) {
        if (strcmp(env->variables[i].name, name) == 0) {
            env->variables[i].value = value;
            return;
        }
    }
    
    if (env->parent) {
        environment_set(env->parent, name, value);
    }
}

// Interpreter initialization
Interpreter *interpreter_create(void) {
    Interpreter *interp = malloc(sizeof(Interpreter));
    interp->global_env = environment_create(NULL);
    interp->current_env = interp->global_env;
    interp->call_stack = malloc(sizeof(CallFrame) * 256);
    interp->call_stack_size = 0;
    interp->call_stack_capacity = 256;
    interp->jit_enabled = true;
    interp->async_enabled = true;
    
    // Initialize built-in functions
    environment_define(interp->global_env, "print", value_object(builtin_print));
    environment_define(interp->global_env, "len", value_object(builtin_len));
    environment_define(interp->global_env, "type", value_object(builtin_type));
    
    current_interpreter = interp;
    return interp;
}

// Built-in functions
Value builtin_print(Environment *env, Value *args, size_t arg_count) {
    for (size_t i = 0; i < arg_count; i++) {
        switch (args[i].type) {
            case VALUE_NUMBER:
                printf("%.6g", args[i].as.number);
                break;
            case VALUE_STRING:
                printf("%s", args[i].as.string);
                break;
            case VALUE_BOOL:
                printf("%s", args[i].as.boolean ? "true" : "false");
                break;
            case VALUE_NULL:
                printf("null");
                break;
            default:
                printf("[object]");
                break;
        }
        if (i < arg_count - 1) printf(" ");
    }
    printf("\n");
    return value_null();
}

Value builtin_len(Environment *env, Value *args, size_t arg_count) {
    if (arg_count != 1) return value_null();
    
    if (args[0].type == VALUE_STRING) {
        return value_number(strlen(args[0].as.string));
    }
    
    return value_null();
}

Value builtin_type(Environment *env, Value *args, size_t arg_count) {
    if (arg_count != 1) return value_null();
    
    switch (args[0].type) {
        case VALUE_NUMBER: return value_string("number");
        case VALUE_STRING: return value_string("string");
        case VALUE_BOOL: return value_string("bool");
        case VALUE_NULL: return value_string("null");
        default: return value_string("object");
    }
}

// Expression evaluation
Value evaluate_expression(Interpreter *interp, Expr *expr) {
    switch (expr->type) {
        case EXPR_LITERAL:
            return evaluate_literal(interp, &expr->as.literal);
            
        case EXPR_IDENTIFIER:
            return environment_get(interp->current_env, expr->as.identifier.name);
            
        case EXPR_BINARY:
            return evaluate_binary(interp, &expr->as.binary);
            
        case EXPR_UNARY:
            return evaluate_unary(interp, &expr->as.unary);
            
        case EXPR_CALL:
            return evaluate_call(interp, &expr->as.call);
            
        case EXPR_MEMBER:
            return evaluate_member(interp, &expr->as.member);
            
        case EXPR_INDEX:
            return evaluate_index(interp, &expr->as.index);
            
        case EXPR_ASSIGNMENT:
            return evaluate_assignment(interp, &expr->as.assignment);
            
        case EXPR_MATCH:
            return evaluate_match(interp, &expr->as.match);
            
        case EXPR_ASYNC:
            return evaluate_async(interp, &expr->as.async);
            
        default:
            return value_null();
    }
}

Value evaluate_literal(Interpreter *interp, LiteralExpr *expr) {
    switch (expr->value_type) {
        case VALUE_NUMBER:
            return value_number(expr->as.number);
        case VALUE_STRING:
            return value_string(expr->as.string);
        case VALUE_BOOL:
            return value_bool(expr->as.boolean);
        case VALUE_NULL:
            return value_null();
        default:
            return value_null();
    }
}

Value evaluate_binary(Interpreter *interp, BinaryExpr *expr) {
    Value left = evaluate_expression(interp, expr->left);
    Value right = evaluate_expression(interp, expr->right);
    
    if (left.type == VALUE_NUMBER && right.type == VALUE_NUMBER) {
        double a = left.as.number;
        double b = right.as.number;
        
        if (strcmp(expr->operator, "+") == 0) return value_number(a + b);
        if (strcmp(expr->operator, "-") == 0) return value_number(a - b);
        if (strcmp(expr->operator, "*") == 0) return value_number(a * b);
        if (strcmp(expr->operator, "/") == 0) return value_number(a / b);
        if (strcmp(expr->operator, "%") == 0) return value_number(fmod(a, b));
        if (strcmp(expr->operator, "**") == 0) return value_number(pow(a, b));
        if (strcmp(expr->operator, "<") == 0) return value_bool(a < b);
        if (strcmp(expr->operator, "<=") == 0) return value_bool(a <= b);
        if (strcmp(expr->operator, ">") == 0) return value_bool(a > b);
        if (strcmp(expr->operator, ">=") == 0) return value_bool(a >= b);
        if (strcmp(expr->operator, "==") == 0) return value_bool(a == b);
        if (strcmp(expr->operator, "!=") == 0) return value_bool(a != b);
    }
    
    if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
        if (strcmp(expr->operator, "+") == 0) {
            char *result = malloc(strlen(left.as.string) + strlen(right.as.string) + 1);
            strcpy(result, left.as.string);
            strcat(result, right.as.string);
            return value_string(result);
        }
        if (strcmp(expr->operator, "==") == 0) {
            return value_bool(strcmp(left.as.string, right.as.string) == 0);
        }
        if (strcmp(expr->operator, "!=") == 0) {
            return value_bool(strcmp(left.as.string, right.as.string) != 0);
        }
    }
    
    if (strcmp(expr->operator, "&&") == 0) {
        return value_bool(is_truthy(left) && is_truthy(right));
    }
    if (strcmp(expr->operator, "||") == 0) {
        return value_bool(is_truthy(left) || is_truthy(right));
    }
    
    return value_null();
}

Value evaluate_unary(Interpreter *interp, UnaryExpr *expr) {
    Value operand = evaluate_expression(interp, expr->operand);
    
    if (strcmp(expr->operator, "-") == 0 && operand.type == VALUE_NUMBER) {
        return value_number(-operand.as.number);
    }
    if (strcmp(expr->operator, "!") == 0) {
        return value_bool(!is_truthy(operand));
    }
    
    return value_null();
}

Value evaluate_call(Interpreter *interp, CallExpr *expr) {
    Value callee = evaluate_expression(interp, expr->callee);
    
    if (callee.type != VALUE_OBJECT) {
        return value_null();
    }
    
    // Evaluate arguments
    Value *args = malloc(sizeof(Value) * expr->arg_count);
    for (size_t i = 0; i < expr->arg_count; i++) {
        args[i] = evaluate_expression(interp, expr->args[i]);
    }
    
    // Check if it's a built-in function
    if (callee.as.object == builtin_print) {
        Value result = builtin_print(interp->current_env, args, expr->arg_count);
        free(args);
        return result;
    }
    
    // Check if it's a user-defined function
    if (callee.as.object) {
        Function *func = (Function *)callee.as.object;
        
        // Check for JIT compilation
        if (interp->jit_enabled && func->call_count > JIT_THRESHOLD) {
            return jit_call_function(interp, func, args, expr->arg_count);
        }
        
        func->call_count++;
        
        // Create new environment for function
        Environment *func_env = environment_create(interp->current_env);
        
        // Bind parameters
        for (size_t i = 0; i < func->param_count && i < expr->arg_count; i++) {
            environment_define(func_env, func->params[i].name, args[i]);
        }
        
        // Execute function body
        Environment *prev_env = interp->current_env;
        interp->current_env = func_env;
        
        Value result = execute_statement(interp, func->body);
        
        interp->current_env = prev_env;
        free(args);
        
        return result;
    }
    
    free(args);
    return value_null();
}

Value evaluate_member(Interpreter *interp, MemberExpr *expr) {
    Value object = evaluate_expression(interp, expr->object);
    
    // Handle built-in object methods
    if (object.type == VALUE_STRING && strcmp(expr->property, "length") == 0) {
        return value_number(strlen(object.as.string));
    }
    
    return value_null();
}

Value evaluate_index(Interpreter *interp, IndexExpr *expr) {
    Value object = evaluate_expression(interp, expr->object);
    Value index = evaluate_expression(interp, expr->index);
    
    // Handle string indexing
    if (object.type == VALUE_STRING && index.type == VALUE_NUMBER) {
        int idx = (int)index.as.number;
        const char *str = object.as.string;
        int len = strlen(str);
        
        if (idx >= 0 && idx < len) {
            char *result = malloc(2);
            result[0] = str[idx];
            result[1] = '\0';
            return value_string(result);
        }
    }
    
    return value_null();
}

Value evaluate_assignment(Interpreter *interp, AssignmentExpr *expr) {
    Value value = evaluate_expression(interp, expr->value);
    
    if (expr->target->type == EXPR_IDENTIFIER) {
        environment_set(interp->current_env, expr->target->as.identifier.name, value);
    }
    
    return value;
}

Value evaluate_match(Interpreter *interp, MatchExpr *expr) {
    Value value = evaluate_expression(interp, expr->value);
    
    for (size_t i = 0; i < expr->case_count; i++) {
        MatchCase *case_expr = &expr->cases[i];
        
        if (pattern_match(case_expr->pattern, value, interp->current_env)) {
            // Check guard condition if present
            if (case_expr->guard) {
                Value guard_result = evaluate_expression(interp, case_expr->guard);
                if (!is_truthy(guard_result)) {
                    continue;
                }
            }
            
            return evaluate_expression(interp, case_expr->body);
        }
    }
    
    return value_null();
}

Value evaluate_async(Interpreter *interp, AsyncExpr *expr) {
    if (!interp->async_enabled) {
        return evaluate_expression(interp, expr->expression);
    }
    
    // Create async task
    AsyncTask *task = async_create_task(interp, expr->expression);
    async_schedule_task(task);
    
    return value_object(task);
}

// Statement execution
Value execute_statement(Interpreter *interp, Stmt *stmt) {
    switch (stmt->type) {
        case STMT_EXPRESSION:
            return evaluate_expression(interp, stmt->as.expression.expr);
            
        case STMT_VAR_DECL:
            return execute_var_decl(interp, &stmt->as.var_decl);
            
        case STMT_FUNCTION_DECL:
            return execute_function_decl(interp, &stmt->as.function_decl);
            
        case STMT_IF:
            return execute_if(interp, &stmt->as.if_stmt);
            
        case STMT_WHILE:
            return execute_while(interp, &stmt->as.while_stmt);
            
        case STMT_FOR:
            return execute_for(interp, &stmt->as.for_stmt);
            
        case STMT_RETURN:
            return execute_return(interp, &stmt->as.return_stmt);
            
        case STMT_BLOCK:
            return execute_block(interp, &stmt->as.block);
            
        case STMT_MATCH:
            return execute_match(interp, &stmt->as.match_stmt);
            
        case STMT_TRY:
            return execute_try(interp, &stmt->as.try_stmt);
            
        default:
            return value_null();
    }
}

Value execute_var_decl(Interpreter *interp, VarDeclStmt *stmt) {
    Value value = value_null();
    if (stmt->initializer) {
        value = evaluate_expression(interp, stmt->initializer);
    }
    
    environment_define(interp->current_env, stmt->name, value);
    return value_null();
}

Value execute_function_decl(Interpreter *interp, FunctionDeclStmt *stmt) {
    Function *func = malloc(sizeof(Function));
    func->name = strdup(stmt->name);
    func->param_count = stmt->param_count;
    func->params = stmt->params;
    func->body = stmt->body;
    func->call_count = 0;
    func->jit_compiled = false;
    func->native_code = NULL;
    
    environment_define(interp->current_env, stmt->name, value_object(func));
    return value_null();
}

Value execute_if(Interpreter *interp, IfStmt *stmt) {
    Value condition = evaluate_expression(interp, stmt->condition);
    
    if (is_truthy(condition)) {
        return execute_statement(interp, stmt->then_branch);
    } else if (stmt->else_branch) {
        return execute_statement(interp, stmt->else_branch);
    }
    
    return value_null();
}

Value execute_while(Interpreter *interp, WhileStmt *stmt) {
    Value result = value_null();
    
    while (true) {
        Value condition = evaluate_expression(interp, stmt->condition);
        if (!is_truthy(condition)) break;
        
        result = execute_statement(interp, stmt->body);
        
        // Handle break/continue (simplified)
        if (interp->break_flag) {
            interp->break_flag = false;
            break;
        }
        if (interp->continue_flag) {
            interp->continue_flag = false;
            continue;
        }
    }
    
    return result;
}

Value execute_for(Interpreter *interp, ForStmt *stmt) {
    Value iterable = evaluate_expression(interp, stmt->iterable);
    Value result = value_null();
    
    // Handle range iteration (simplified)
    if (iterable.type == VALUE_OBJECT) {
        Range *range = (Range *)iterable.as.object;
        
        for (int i = range->start; i < range->end; i += range->step) {
            environment_define(interp->current_env, stmt->variable, value_number(i));
            result = execute_statement(interp, stmt->body);
            
            if (interp->break_flag) {
                interp->break_flag = false;
                break;
            }
            if (interp->continue_flag) {
                interp->continue_flag = false;
                continue;
            }
        }
    }
    
    return result;
}

Value execute_return(Interpreter *interp, ReturnStmt *stmt) {
    Value value = value_null();
    if (stmt->value) {
        value = evaluate_expression(interp, stmt->value);
    }
    
    interp->return_flag = true;
    interp->return_value = value;
    return value;
}

Value execute_block(Interpreter *interp, BlockStmt *stmt) {
    Environment *block_env = environment_create(interp->current_env);
    Environment *prev_env = interp->current_env;
    interp->current_env = block_env;
    
    Value result = value_null();
    for (size_t i = 0; i < stmt->stmt_count; i++) {
        result = execute_statement(interp, stmt->statements[i]);
        
        if (interp->return_flag || interp->break_flag || interp->continue_flag) {
            break;
        }
    }
    
    interp->current_env = prev_env;
    return result;
}

Value execute_match(Interpreter *interp, MatchStmt *stmt) {
    Value value = evaluate_expression(interp, stmt->value);
    
    for (size_t i = 0; i < stmt->case_count; i++) {
        MatchCase *case_stmt = &stmt->cases[i];
        
        if (pattern_match(case_stmt->pattern, value, interp->current_env)) {
            if (case_stmt->guard) {
                Value guard_result = evaluate_expression(interp, case_stmt->guard);
                if (!is_truthy(guard_result)) {
                    continue;
                }
            }
            
            return execute_statement(interp, case_stmt->body);
        }
    }
    
    return value_null();
}

Value execute_try(Interpreter *interp, TryStmt *stmt) {
    // Set up exception handler
    ExceptionHandler handler;
    handler.previous = interp->current_handler;
    interp->current_handler = &handler;
    
    Value result = value_null();
    
    if (setjmp(handler.jump_buffer) == 0) {
        // Execute try block
        result = execute_statement(interp, stmt->try_block);
    } else {
        // Exception occurred, execute catch blocks
        Exception *ex = handler.current_exception;
        
        for (size_t i = 0; i < stmt->catch_count; i++) {
            CatchClause *catch_clause = &stmt->catch_clauses[i];
            
            if (strcmp(ex->type, catch_clause->exception_type) == 0) {
                environment_define(interp->current_env, catch_clause->variable, value_object(ex));
                result = execute_statement(interp, catch_clause->body);
                break;
            }
        }
    }
    
    // Execute finally block
    if (stmt->finally_block) {
        execute_statement(interp, stmt->finally_block);
    }
    
    interp->current_handler = handler.previous;
    return result;
}

// Utility functions
bool is_truthy(Value value) {
    switch (value.type) {
        case VALUE_NULL:
            return false;
        case VALUE_BOOL:
            return value.as.boolean;
        case VALUE_NUMBER:
            return value.as.number != 0.0;
        case VALUE_STRING:
            return strlen(value.as.string) > 0;
        default:
            return true;
    }
}

void interpreter_cleanup(Interpreter *interp) {
    // Cleanup environments, call stack, etc.
    free(interp->call_stack);
    free(interp);
}

// Main interpreter entry point
Value interpret(Interpreter *interp, Stmt **statements, size_t stmt_count) {
    Value result = value_null();
    
    for (size_t i = 0; i < stmt_count; i++) {
        result = execute_statement(interp, statements[i]);
        
        if (interp->return_flag) {
            result = interp->return_value;
            interp->return_flag = false;
            break;
        }
    }
    
    return result;
}