#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Value operations
Value value_null() {
    Value v = {.type = VAL_NULL};
    return v;
}

Value value_bool(bool value) {
    Value v = {.type = VAL_BOOL, .as.boolean = value};
    return v;
}

Value value_number(double value) {
    Value v = {.type = VAL_NUMBER, .as.number = value};
    return v;
}

Value value_string(const char* value) {
    Value v = {.type = VAL_STRING, .as.string = strdup(value)};
    return v;
}

void value_free(Value* value) {
    if (value->type == VAL_STRING && value->as.string) {
        free(value->as.string);
    }
}

bool value_is_truthy(Value value) {
    switch (value.type) {
        case VAL_NULL: return false;
        case VAL_BOOL: return value.as.boolean;
        case VAL_NUMBER: return value.as.number != 0.0;
        case VAL_STRING: return value.as.string != NULL && value.as.string[0] != '\0';
        default: return false;
    }
}

void value_print(Value value) {
    switch (value.type) {
        case VAL_NULL:
            printf("null");
            break;
        case VAL_BOOL:
            printf("%s", value.as.boolean ? "true" : "false");
            break;
        case VAL_NUMBER:
            printf("%g", value.as.number);
            break;
        case VAL_STRING:
            printf("%s", value.as.string);
            break;
    }
}

// Expression constructors
Expr* expr_number(double value) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_NUMBER;
    expr->as.number = value;
    return expr;
}

Expr* expr_string(const char* value) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_STRING;
    expr->as.string = strdup(value);
    return expr;
}

Expr* expr_bool(bool value) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_BOOL;
    expr->as.boolean = value;
    return expr;
}

Expr* expr_null() {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_NULL;
    return expr;
}

Expr* expr_identifier(const char* name) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_IDENTIFIER;
    expr->as.identifier = strdup(name);
    return expr;
}

Expr* expr_binary(const char* op, Expr* left, Expr* right) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.op = strdup(op);
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    return expr;
}

Expr* expr_unary(const char* op, Expr* operand) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.op = strdup(op);
    expr->as.unary.operand = operand;
    return expr;
}

Expr* expr_call(Expr* callee, Expr** args, size_t arg_count) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_CALL;
    expr->as.call.callee = callee;
    expr->as.call.args = args;
    expr->as.call.arg_count = arg_count;
    return expr;
}

Expr* expr_assign(const char* name, Expr* value) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_ASSIGN;
    expr->as.assign.name = strdup(name);
    expr->as.assign.value = value;
    return expr;
}

Expr* expr_function(char** params, char** param_types, size_t param_count, const char* return_type, Stmt** body, size_t body_count) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_FUNCTION;
    expr->as.function.params = params;
    expr->as.function.param_types = param_types;
    expr->as.function.param_count = param_count;
    expr->as.function.return_type = return_type ? strdup(return_type) : NULL;
    expr->as.function.body = body;
    expr->as.function.body_count = body_count;
    expr->as.function.is_nested = true;
    return expr;
}

Expr* expr_array(Expr** elements, size_t count) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_ARRAY;
    expr->as.array.elements = elements;
    expr->as.array.count = count;
    return expr;
}

Expr* expr_index(Expr* object, Expr* index) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_INDEX;
    expr->as.index.object = object;
    expr->as.index.index = index;
    return expr;
}

Expr* expr_member(Expr* object, const char* property) {
    Expr* expr = malloc(sizeof(Expr));
    expr->type = EXPR_MEMBER;
    expr->as.member.object = object;
    expr->as.member.property = strdup(property);
    return expr;
}

void expr_free(Expr* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case EXPR_STRING:
            free(expr->as.string);
            break;
        case EXPR_IDENTIFIER:
            free(expr->as.identifier);
            break;
        case EXPR_BINARY:
            free(expr->as.binary.op);
            expr_free(expr->as.binary.left);
            expr_free(expr->as.binary.right);
            break;
        case EXPR_UNARY:
            free(expr->as.unary.op);
            expr_free(expr->as.unary.operand);
            break;
        case EXPR_CALL:
            expr_free(expr->as.call.callee);
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                expr_free(expr->as.call.args[i]);
            }
            free(expr->as.call.args);
            break;
        case EXPR_ASSIGN:
            free(expr->as.assign.name);
            expr_free(expr->as.assign.value);
            break;
        case EXPR_FUNCTION:
            for (size_t i = 0; i < expr->as.function.param_count; i++) {
                free(expr->as.function.params[i]);
                if (expr->as.function.param_types[i]) free(expr->as.function.param_types[i]);
            }
            free(expr->as.function.params);
            free(expr->as.function.param_types);
            if (expr->as.function.return_type) free(expr->as.function.return_type);
            for (size_t i = 0; i < expr->as.function.body_count; i++) {
                stmt_free(expr->as.function.body[i]);
            }
            free(expr->as.function.body);
            break;
        case EXPR_ARRAY:
            for (size_t i = 0; i < expr->as.array.count; i++) {
                expr_free(expr->as.array.elements[i]);
            }
            free(expr->as.array.elements);
            break;
        case EXPR_INDEX:
            expr_free(expr->as.index.object);
            expr_free(expr->as.index.index);
            break;
        case EXPR_MEMBER:
            expr_free(expr->as.member.object);
            free(expr->as.member.property);
            break;
        default:
            break;
    }
    free(expr);
}

// Statement constructors
Stmt* stmt_expression(Expr* expr) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_EXPR;
    stmt->as.expression = expr;
    return stmt;
}

Stmt* stmt_var_decl(const char* name, const char* type_name, bool is_const, Expr* initializer) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_VAR_DECL;
    stmt->as.var_decl.name = strdup(name);
    stmt->as.var_decl.type_name = type_name ? strdup(type_name) : NULL;
    stmt->as.var_decl.is_const = is_const;
    stmt->as.var_decl.initializer = initializer;
    return stmt;
}

Stmt* stmt_function(const char* name, char** params, char** param_types, size_t param_count, const char* return_type, Stmt** body, size_t body_count) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_FUNCTION;
    stmt->as.function.name = strdup(name);
    stmt->as.function.params = params;
    stmt->as.function.param_types = param_types;
    stmt->as.function.param_count = param_count;
    stmt->as.function.return_type = return_type ? strdup(return_type) : NULL;
    stmt->as.function.body = body;
    stmt->as.function.body_count = body_count;
    stmt->as.function.is_nested = false;
    stmt->as.function.nested_functions = NULL;
    stmt->as.function.nested_count = 0;
    return stmt;
}

Stmt* stmt_return(Expr* value) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_RETURN;
    stmt->as.return_stmt.value = value;
    return stmt;
}

Stmt* stmt_if(Expr* condition, Stmt** then_branch, size_t then_count, Stmt** else_branch, size_t else_count) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_IF;
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_branch = then_branch;
    stmt->as.if_stmt.then_count = then_count;
    stmt->as.if_stmt.else_branch = else_branch;
    stmt->as.if_stmt.else_count = else_count;
    return stmt;
}

Stmt* stmt_while(Expr* condition, Stmt** body, size_t body_count) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_WHILE;
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    stmt->as.while_stmt.body_count = body_count;
    return stmt;
}

Stmt* stmt_for(Stmt* init, Expr* condition, Expr* increment, Stmt** body, size_t body_count) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_FOR;
    stmt->as.for_stmt.init = init;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.increment = increment;
    stmt->as.for_stmt.body = body;
    stmt->as.for_stmt.body_count = body_count;
    return stmt;
}

Stmt* stmt_block(Stmt** statements, size_t count) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_BLOCK;
    stmt->as.block.statements = statements;
    stmt->as.block.count = count;
    return stmt;
}

Stmt* stmt_print(Expr* expr) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_PRINT;
    stmt->as.print_stmt.expression = expr;
    return stmt;
}

Stmt* stmt_import(const char* spec) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_IMPORT;
    stmt->as.import_stmt.spec = strdup(spec);
    return stmt;
}

Stmt* stmt_for_in(const char* variable, Expr* iterable, Stmt** body, size_t body_count) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_FOR_IN;
    stmt->as.for_in_stmt.variable = strdup(variable);
    stmt->as.for_in_stmt.iterable = iterable;
    stmt->as.for_in_stmt.body = body;
    stmt->as.for_in_stmt.body_count = body_count;
    return stmt;
}

Stmt* stmt_do_while(Stmt** body, size_t body_count, Expr* condition) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_DO_WHILE;
    stmt->as.do_while_stmt.body = body;
    stmt->as.do_while_stmt.body_count = body_count;
    stmt->as.do_while_stmt.condition = condition;
    return stmt;
}

Stmt* stmt_break(const char* label) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_BREAK;
    stmt->as.break_stmt.label = label ? strdup(label) : NULL;
    return stmt;
}

Stmt* stmt_continue(const char* label) {
    Stmt* stmt = malloc(sizeof(Stmt));
    stmt->type = STMT_CONTINUE;
    stmt->as.continue_stmt.label = label ? strdup(label) : NULL;
    return stmt;
}

void stmt_free(Stmt* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_EXPR:
            expr_free(stmt->as.expression);
            break;
        case STMT_VAR_DECL:
            free(stmt->as.var_decl.name);
            if (stmt->as.var_decl.type_name) free(stmt->as.var_decl.type_name);
            expr_free(stmt->as.var_decl.initializer);
            break;
        case STMT_FUNCTION:
            free(stmt->as.function.name);
            for (size_t i = 0; i < stmt->as.function.param_count; i++) {
                free(stmt->as.function.params[i]);
                if (stmt->as.function.param_types[i]) free(stmt->as.function.param_types[i]);
            }
            free(stmt->as.function.params);
            free(stmt->as.function.param_types);
            if (stmt->as.function.return_type) free(stmt->as.function.return_type);
            for (size_t i = 0; i < stmt->as.function.body_count; i++) {
                stmt_free(stmt->as.function.body[i]);
            }
            free(stmt->as.function.body);
            for (size_t i = 0; i < stmt->as.function.nested_count; i++) {
                stmt_free((Stmt*)stmt->as.function.nested_functions[i]);
            }
            free(stmt->as.function.nested_functions);
            break;
        case STMT_RETURN:
            expr_free(stmt->as.return_stmt.value);
            break;
        case STMT_IF:
            expr_free(stmt->as.if_stmt.condition);
            for (size_t i = 0; i < stmt->as.if_stmt.then_count; i++) {
                stmt_free(stmt->as.if_stmt.then_branch[i]);
            }
            free(stmt->as.if_stmt.then_branch);
            for (size_t i = 0; i < stmt->as.if_stmt.else_count; i++) {
                stmt_free(stmt->as.if_stmt.else_branch[i]);
            }
            free(stmt->as.if_stmt.else_branch);
            break;
        case STMT_WHILE:
            expr_free(stmt->as.while_stmt.condition);
            for (size_t i = 0; i < stmt->as.while_stmt.body_count; i++) {
                stmt_free(stmt->as.while_stmt.body[i]);
            }
            free(stmt->as.while_stmt.body);
            break;
        case STMT_FOR:
            stmt_free(stmt->as.for_stmt.init);
            expr_free(stmt->as.for_stmt.condition);
            expr_free(stmt->as.for_stmt.increment);
            for (size_t i = 0; i < stmt->as.for_stmt.body_count; i++) {
                stmt_free(stmt->as.for_stmt.body[i]);
            }
            free(stmt->as.for_stmt.body);
            break;
        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->as.block.count; i++) {
                stmt_free(stmt->as.block.statements[i]);
            }
            free(stmt->as.block.statements);
            break;
        case STMT_PRINT:
            expr_free(stmt->as.print_stmt.expression);
            break;
        case STMT_IMPORT:
            free(stmt->as.import_stmt.spec);
            break;
        case STMT_FOR_IN:
            free(stmt->as.for_in_stmt.variable);
            expr_free(stmt->as.for_in_stmt.iterable);
            for (size_t i = 0; i < stmt->as.for_in_stmt.body_count; i++) {
                stmt_free(stmt->as.for_in_stmt.body[i]);
            }
            free(stmt->as.for_in_stmt.body);
            break;
        case STMT_DO_WHILE:
            for (size_t i = 0; i < stmt->as.do_while_stmt.body_count; i++) {
                stmt_free(stmt->as.do_while_stmt.body[i]);
            }
            free(stmt->as.do_while_stmt.body);
            expr_free(stmt->as.do_while_stmt.condition);
            break;
        case STMT_BREAK:
            if (stmt->as.break_stmt.label) free(stmt->as.break_stmt.label);
            break;
        case STMT_CONTINUE:
            if (stmt->as.continue_stmt.label) free(stmt->as.continue_stmt.label);
            break;
    }
    free(stmt);
}

// Scope management implementation
Scope* scope_create(Scope* parent) {
    Scope* scope = malloc(sizeof(Scope));
    scope->parent = parent;
    scope->variables = malloc(sizeof(char*) * 64);
    scope->var_count = 0;
    scope->functions = malloc(sizeof(char*) * 32);
    scope->func_count = 0;
    return scope;
}

void scope_free(Scope* scope) {
    if (!scope) return;
    
    for (size_t i = 0; i < scope->var_count; i++) {
        free(scope->variables[i]);
    }
    free(scope->variables);
    
    for (size_t i = 0; i < scope->func_count; i++) {
        free(scope->functions[i]);
    }
    free(scope->functions);
    
    free(scope);
}

bool scope_define_var(Scope* scope, const char* name) {
    scope->variables[scope->var_count] = strdup(name);
    scope->var_count++;
    return true;
}

bool scope_define_func(Scope* scope, const char* name) {
    scope->functions[scope->func_count] = strdup(name);
    scope->func_count++;
    return true;
}

bool scope_lookup_var(Scope* scope, const char* name) {
    for (size_t i = 0; i < scope->var_count; i++) {
        if (strcmp(scope->variables[i], name) == 0) {
            return true;
        }
    }
    
    if (scope->parent) {
        return scope_lookup_var(scope->parent, name);
    }
    
    return false;
}

bool scope_lookup_func(Scope* scope, const char* name) {
    for (size_t i = 0; i < scope->func_count; i++) {
        if (strcmp(scope->functions[i], name) == 0) {
            return true;
        }
    }
    
    if (scope->parent) {
        return scope_lookup_func(scope->parent, name);
    }
    
    return false;
}
