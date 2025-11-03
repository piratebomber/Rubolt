#include "typechecker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void typechecker_init(TypeChecker* tc) {
    tc->errors = NULL;
    tc->error_count = 0;
    tc->error_capacity = 0;
}

void typechecker_free(TypeChecker* tc) {
    for (size_t i = 0; i < tc->error_count; i++) {
        free(tc->errors[i].message);
        if (tc->errors[i].file) free(tc->errors[i].file);
        if (tc->errors[i].hint) free(tc->errors[i].hint);
    }
    free(tc->errors);
}

void typechecker_add_error(TypeChecker* tc, const char* message, int line, int column, const char* hint) {
    if (tc->error_count >= tc->error_capacity) {
        tc->error_capacity = tc->error_capacity == 0 ? 8 : tc->error_capacity * 2;
        tc->errors = realloc(tc->errors, tc->error_capacity * sizeof(TypeError));
    }
    
    TypeError* err = &tc->errors[tc->error_count++];
    err->message = strdup(message);
    err->line = line;
    err->column = column;
    err->file = NULL;
    err->hint = hint ? strdup(hint) : NULL;
}

Type* type_from_string(const char* type_name) {
    if (!type_name) return NULL;
    
    Type* type = malloc(sizeof(Type));
    type->name = strdup(type_name);
    
    if (strcmp(type_name, "number") == 0) {
        type->kind = TYPE_NUMBER;
    } else if (strcmp(type_name, "string") == 0) {
        type->kind = TYPE_STRING;
    } else if (strcmp(type_name, "bool") == 0) {
        type->kind = TYPE_BOOL;
    } else if (strcmp(type_name, "void") == 0) {
        type->kind = TYPE_VOID;
    } else if (strcmp(type_name, "any") == 0) {
        type->kind = TYPE_ANY;
    } else {
        type->kind = TYPE_UNKNOWN;
    }
    
    return type;
}

bool types_compatible(Type* expected, Type* actual) {
    if (!expected || !actual) return true;
    if (expected->kind == TYPE_ANY || actual->kind == TYPE_ANY) return true;
    if (expected->kind == TYPE_NULL || actual->kind == TYPE_NULL) return true;
    return expected->kind == actual->kind;
}

const char* type_to_string(Type* type) {
    if (!type) return "unknown";
    switch (type->kind) {
        case TYPE_NUMBER: return "number";
        case TYPE_STRING: return "string";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        case TYPE_ANY: return "any";
        case TYPE_NULL: return "null";
        case TYPE_FUNCTION: return "function";
        default: return "unknown";
    }
}

static Type* infer_expr_type(Expr* expr) {
    Type* type = malloc(sizeof(Type));
    type->name = NULL;
    
    switch (expr->type) {
        case EXPR_NUMBER:
            type->kind = TYPE_NUMBER;
            type->name = strdup("number");
            break;
        case EXPR_STRING:
            type->kind = TYPE_STRING;
            type->name = strdup("string");
            break;
        case EXPR_BOOL:
            type->kind = TYPE_BOOL;
            type->name = strdup("bool");
            break;
        case EXPR_NULL:
            type->kind = TYPE_NULL;
            type->name = strdup("null");
            break;
        case EXPR_BINARY: {
            Type* left = infer_expr_type(expr->as.binary.left);
            Type* right = infer_expr_type(expr->as.binary.right);
            
            const char* op = expr->as.binary.op;
            if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || 
                strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
                if (left->kind == TYPE_STRING || right->kind == TYPE_STRING) {
                    type->kind = TYPE_STRING;
                    type->name = strdup("string");
                } else {
                    type->kind = TYPE_NUMBER;
                    type->name = strdup("number");
                }
            } else {
                type->kind = TYPE_BOOL;
                type->name = strdup("bool");
            }
            
            free(left->name);
            free(left);
            free(right->name);
            free(right);
            break;
        }
        case EXPR_UNARY: {
            Type* operand = infer_expr_type(expr->as.unary.operand);
            if (strcmp(expr->as.unary.op, "!") == 0 || strcmp(expr->as.unary.op, "not") == 0) {
                type->kind = TYPE_BOOL;
                type->name = strdup("bool");
            } else {
                *type = *operand;
            }
            free(operand);
            break;
        }
        default:
            type->kind = TYPE_ANY;
            type->name = strdup("any");
            break;
    }
    
    return type;
}

static void check_stmt(TypeChecker* tc, Stmt* stmt);

static void check_var_decl(TypeChecker* tc, VarDeclStmt* var_decl) {
    if (var_decl->type_name && var_decl->initializer) {
        Type* expected = type_from_string(var_decl->type_name);
        Type* actual = infer_expr_type(var_decl->initializer);
        
        if (!types_compatible(expected, actual)) {
            char msg[256];
            snprintf(msg, sizeof(msg), 
                    "Type mismatch for variable '%s': expected '%s', got '%s'",
                    var_decl->name, type_to_string(expected), type_to_string(actual));
            typechecker_add_error(tc, msg, 0, 0, 
                                "Consider changing the type annotation or the initializer value");
        }
        
        free(expected->name);
        free(expected);
        free(actual->name);
        free(actual);
    }
}

static void check_function(TypeChecker* tc, FunctionStmt* func) {
    for (size_t i = 0; i < func->body_count; i++) {
        check_stmt(tc, func->body[i]);
    }
    
    // Check return type if specified
    if (func->return_type) {
        Type* expected_return = type_from_string(func->return_type);
        // In a full implementation, we'd track actual return values
        free(expected_return->name);
        free(expected_return);
    }
}

static void check_stmt(TypeChecker* tc, Stmt* stmt) {
    switch (stmt->type) {
        case STMT_VAR_DECL:
            check_var_decl(tc, &stmt->as.var_decl);
            break;
        case STMT_FUNCTION:
            check_function(tc, &stmt->as.function);
            break;
        case STMT_IF:
            for (size_t i = 0; i < stmt->as.if_stmt.then_count; i++) {
                check_stmt(tc, stmt->as.if_stmt.then_branch[i]);
            }
            for (size_t i = 0; i < stmt->as.if_stmt.else_count; i++) {
                check_stmt(tc, stmt->as.if_stmt.else_branch[i]);
            }
            break;
        case STMT_WHILE:
            for (size_t i = 0; i < stmt->as.while_stmt.body_count; i++) {
                check_stmt(tc, stmt->as.while_stmt.body[i]);
            }
            break;
        case STMT_FOR:
            if (stmt->as.for_stmt.init) check_stmt(tc, stmt->as.for_stmt.init);
            for (size_t i = 0; i < stmt->as.for_stmt.body_count; i++) {
                check_stmt(tc, stmt->as.for_stmt.body[i]);
            }
            break;
        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->as.block.count; i++) {
                check_stmt(tc, stmt->as.block.statements[i]);
            }
            break;
        default:
            break;
    }
}

bool typechecker_check_program(TypeChecker* tc, Stmt** statements, size_t count) {
    for (size_t i = 0; i < count; i++) {
        check_stmt(tc, statements[i]);
    }
    return tc->error_count == 0;
}

void typechecker_print_errors(TypeChecker* tc) {
    if (tc->error_count == 0) return;
    
    fprintf(stderr, "\n\033[1;31mType Errors Found:\033[0m\n");
    fprintf(stderr, "─────────────────────────────────────────\n\n");
    
    for (size_t i = 0; i < tc->error_count; i++) {
        TypeError* err = &tc->errors[i];
        
        fprintf(stderr, "\033[1;31m✗ Error:\033[0m %s\n", err->message);
        if (err->line > 0) {
            fprintf(stderr, "  \033[90m→ at line %d, column %d\033[0m\n", err->line, err->column);
        }
        if (err->hint) {
            fprintf(stderr, "  \033[1;33m💡 Hint:\033[0m %s\n", err->hint);
        }
        fprintf(stderr, "\n");
    }
    
    fprintf(stderr, "─────────────────────────────────────────\n");
    fprintf(stderr, "\033[1;31m%zu error(s) found\033[0m\n\n", tc->error_count);
}
