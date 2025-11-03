#include "interpreter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

void env_init(Environment* env) {
    env->var_count = 0;
    env->function_count = 0;
    env->has_return = false;
}

void env_free(Environment* env) {
    for (size_t i = 0; i < env->var_count; i++) {
        free(env->vars[i].name);
        value_free(&env->vars[i].value);
    }
    if (env->has_return) {
        value_free(&env->return_value);
    }
}

bool env_define_var(Environment* env, const char* name, Value value, bool is_const) {
    if (env->var_count >= MAX_VARS) return false;
    
    env->vars[env->var_count].name = strdup(name);
    env->vars[env->var_count].value = value;
    env->vars[env->var_count].is_const = is_const;
    env->var_count++;
    return true;
}

bool env_set_var(Environment* env, const char* name, Value value) {
    for (size_t i = 0; i < env->var_count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            if (env->vars[i].is_const) {
                fprintf(stderr, "Cannot assign to const variable '%s'\n", name);
                return false;
            }
            value_free(&env->vars[i].value);
            env->vars[i].value = value;
            return true;
        }
    }
    return false;
}

bool env_get_var(Environment* env, const char* name, Value* out) {
    for (size_t i = 0; i < env->var_count; i++) {
        if (strcmp(env->vars[i].name, name) == 0) {
            *out = env->vars[i].value;
            return true;
        }
    }
    return false;
}

bool env_define_function(Environment* env, FunctionStmt* fn) {
    if (env->function_count >= MAX_FUNCTIONS) return false;
    
    env->functions[env->function_count].fn = fn;
    env->function_count++;
    return true;
}

Function* env_get_function(Environment* env, const char* name) {
    for (size_t i = 0; i < env->function_count; i++) {
        if (strcmp(env->functions[i].fn->name, name) == 0) {
            return &env->functions[i];
        }
    }
    return NULL;
}

Value eval_expr(Environment* env, Expr* expr) {
    switch (expr->type) {
        case EXPR_NUMBER:
            return value_number(expr->as.number);
        
        case EXPR_STRING: {
            const char* str = expr->as.string;
            size_t len = strlen(str);
            if (len >= 2 && ((str[0] == '"' && str[len-1] == '"') || 
                             (str[0] == '\'' && str[len-1] == '\''))) {
                char* unquoted = malloc(len - 1);
                strncpy(unquoted, str + 1, len - 2);
                unquoted[len - 2] = '\0';
                Value v = value_string(unquoted);
                free(unquoted);
                return v;
            }
            return value_string(str);
        }
        
        case EXPR_BOOL:
            return value_bool(expr->as.boolean);
        
        case EXPR_NULL:
            return value_null();
        
        case EXPR_IDENTIFIER: {
            Value v;
            if (env_get_var(env, expr->as.identifier, &v)) {
                return v;
            }
            fprintf(stderr, "Undefined variable '%s'\n", expr->as.identifier);
            return value_null();
        }
        
        case EXPR_BINARY: {
            Value left = eval_expr(env, expr->as.binary.left);
            Value right = eval_expr(env, expr->as.binary.right);
            const char* op = expr->as.binary.op;
            
            if (strcmp(op, "+") == 0) {
                if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                    return value_number(left.as.number + right.as.number);
                } else if (left.type == VAL_STRING && right.type == VAL_STRING) {
                    size_t len = strlen(left.as.string) + strlen(right.as.string) + 1;
                    char* result = malloc(len);
                    strcpy(result, left.as.string);
                    strcat(result, right.as.string);
                    Value v = value_string(result);
                    free(result);
                    return v;
                }
            } else if (strcmp(op, "-") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return value_number(left.as.number - right.as.number);
            } else if (strcmp(op, "*") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return value_number(left.as.number * right.as.number);
            } else if (strcmp(op, "/") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                if (right.as.number == 0.0) {
                    fprintf(stderr, "Division by zero\n");
                    return value_null();
                }
                return value_number(left.as.number / right.as.number);
            } else if (strcmp(op, "%") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return value_number(fmod(left.as.number, right.as.number));
            } else if (strcmp(op, "==") == 0) {
                if (left.type != right.type) return value_bool(false);
                if (left.type == VAL_NUMBER) return value_bool(left.as.number == right.as.number);
                if (left.type == VAL_BOOL) return value_bool(left.as.boolean == right.as.boolean);
                if (left.type == VAL_STRING) return value_bool(strcmp(left.as.string, right.as.string) == 0);
                return value_bool(true);
            } else if (strcmp(op, "!=") == 0) {
                if (left.type != right.type) return value_bool(true);
                if (left.type == VAL_NUMBER) return value_bool(left.as.number != right.as.number);
                if (left.type == VAL_BOOL) return value_bool(left.as.boolean != right.as.boolean);
                if (left.type == VAL_STRING) return value_bool(strcmp(left.as.string, right.as.string) != 0);
                return value_bool(false);
            } else if (strcmp(op, "<") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return value_bool(left.as.number < right.as.number);
            } else if (strcmp(op, "<=") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return value_bool(left.as.number <= right.as.number);
            } else if (strcmp(op, ">") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return value_bool(left.as.number > right.as.number);
            } else if (strcmp(op, ">=") == 0 && left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
                return value_bool(left.as.number >= right.as.number);
            } else if (strcmp(op, "&&") == 0 || strcmp(op, "and") == 0) {
                return value_bool(value_is_truthy(left) && value_is_truthy(right));
            } else if (strcmp(op, "||") == 0 || strcmp(op, "or") == 0) {
                return value_bool(value_is_truthy(left) || value_is_truthy(right));
            }
            
            return value_null();
        }
        
        case EXPR_UNARY: {
            Value operand = eval_expr(env, expr->as.unary.operand);
            const char* op = expr->as.unary.op;
            
            if (strcmp(op, "-") == 0 && operand.type == VAL_NUMBER) {
                return value_number(-operand.as.number);
            } else if (strcmp(op, "!") == 0 || strcmp(op, "not") == 0) {
                return value_bool(!value_is_truthy(operand));
            }
            
            return value_null();
        }
        
        case EXPR_CALL: {
            if (expr->as.call.callee->type != EXPR_IDENTIFIER) {
                fprintf(stderr, "Can only call functions\n");
                return value_null();
            }
            
            const char* name = expr->as.call.callee->as.identifier;
            
            if (strcmp(name, "print") == 0) {
                for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                    Value arg = eval_expr(env, expr->as.call.args[i]);
                    value_print(arg);
                    if (i < expr->as.call.arg_count - 1) printf(" ");
                }
                printf("\n");
                return value_null();
            } else if (strcmp(name, "printf") == 0) {
                for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                    Value arg = eval_expr(env, expr->as.call.args[i]);
                    value_print(arg);
                }
                return value_null();
            }
            
            Function* fn = env_get_function(env, name);
            if (!fn) {
                fprintf(stderr, "Undefined function '%s'\n", name);
                return value_null();
            }
            
            if (expr->as.call.arg_count != fn->fn->param_count) {
                fprintf(stderr, "Function '%s' expects %zu arguments, got %zu\n", 
                        name, fn->fn->param_count, expr->as.call.arg_count);
                return value_null();
            }
            
            Environment fn_env;
            env_init(&fn_env);
            
            for (size_t i = 0; i < fn->fn->param_count; i++) {
                Value arg = eval_expr(env, expr->as.call.args[i]);
                env_define_var(&fn_env, fn->fn->params[i], arg, false);
            }
            
            for (size_t i = 0; i < fn->fn->body_count; i++) {
                exec_stmt(&fn_env, fn->fn->body[i]);
                if (fn_env.has_return) break;
            }
            
            Value result = fn_env.has_return ? fn_env.return_value : value_null();
            env_free(&fn_env);
            return result;
        }
        
        case EXPR_ASSIGN: {
            Value value = eval_expr(env, expr->as.assign.value);
            if (!env_set_var(env, expr->as.assign.name, value)) {
                fprintf(stderr, "Cannot assign to '%s'\n", expr->as.assign.name);
            }
            return value;
        }
        
        default:
            return value_null();
    }
}

void exec_stmt(Environment* env, Stmt* stmt) {
    if (env->has_return) return;
    
    switch (stmt->type) {
        case STMT_EXPR:
            eval_expr(env, stmt->as.expression);
            break;
        
        case STMT_VAR_DECL: {
            Value value = stmt->as.var_decl.initializer ? 
                          eval_expr(env, stmt->as.var_decl.initializer) : 
                          value_null();
            env_define_var(env, stmt->as.var_decl.name, value, stmt->as.var_decl.is_const);
            break;
        }
        
        case STMT_FUNCTION:
            env_define_function(env, &stmt->as.function);
            break;
        
        case STMT_RETURN:
            env->has_return = true;
            env->return_value = stmt->as.return_stmt.value ? 
                                eval_expr(env, stmt->as.return_stmt.value) : 
                                value_null();
            break;
        
        case STMT_IF: {
            Value condition = eval_expr(env, stmt->as.if_stmt.condition);
            if (value_is_truthy(condition)) {
                for (size_t i = 0; i < stmt->as.if_stmt.then_count; i++) {
                    exec_stmt(env, stmt->as.if_stmt.then_branch[i]);
                    if (env->has_return) break;
                }
            } else if (stmt->as.if_stmt.else_branch) {
                for (size_t i = 0; i < stmt->as.if_stmt.else_count; i++) {
                    exec_stmt(env, stmt->as.if_stmt.else_branch[i]);
                    if (env->has_return) break;
                }
            }
            break;
        }
        
        case STMT_WHILE: {
            while (true) {
                Value condition = eval_expr(env, stmt->as.while_stmt.condition);
                if (!value_is_truthy(condition)) break;
                
                for (size_t i = 0; i < stmt->as.while_stmt.body_count; i++) {
                    exec_stmt(env, stmt->as.while_stmt.body[i]);
                    if (env->has_return) break;
                }
                if (env->has_return) break;
            }
            break;
        }
        
        case STMT_FOR: {
            if (stmt->as.for_stmt.init) {
                exec_stmt(env, stmt->as.for_stmt.init);
            }
            
            while (true) {
                if (stmt->as.for_stmt.condition) {
                    Value condition = eval_expr(env, stmt->as.for_stmt.condition);
                    if (!value_is_truthy(condition)) break;
                }
                
                for (size_t i = 0; i < stmt->as.for_stmt.body_count; i++) {
                    exec_stmt(env, stmt->as.for_stmt.body[i]);
                    if (env->has_return) break;
                }
                if (env->has_return) break;
                
                if (stmt->as.for_stmt.increment) {
                    eval_expr(env, stmt->as.for_stmt.increment);
                }
            }
            break;
        }
        
        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->as.block.count; i++) {
                exec_stmt(env, stmt->as.block.statements[i]);
                if (env->has_return) break;
            }
            break;
        
        case STMT_PRINT: {
            Value value = eval_expr(env, stmt->as.print_stmt.expression);
            value_print(value);
            printf("\n");
            break;
        }
    }
}

void interpret(Stmt** statements, size_t count) {
    Environment env;
    env_init(&env);
    
    for (size_t i = 0; i < count; i++) {
        exec_stmt(&env, statements[i]);
    }
    
    env_free(&env);
}
