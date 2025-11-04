#ifndef RUBOLT_AST_H
#define RUBOLT_AST_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    VAL_NULL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_STRING
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        char* string;
    } as;
} Value;

typedef enum {
    EXPR_NUMBER,
    EXPR_STRING,
    EXPR_BOOL,
    EXPR_NULL,
    EXPR_IDENTIFIER,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_ASSIGN
} ExprType;

typedef struct Expr Expr;

typedef struct {
    char* op;
    Expr* left;
    Expr* right;
} BinaryExpr;

typedef struct {
    char* op;
    Expr* operand;
} UnaryExpr;

typedef struct {
    Expr* callee;
    Expr** args;
    size_t arg_count;
} CallExpr;

typedef struct {
    char* name;
    Expr* value;
} AssignExpr;

struct Expr {
    ExprType type;
    union {
        double number;
        char* string;
        bool boolean;
        char* identifier;
        BinaryExpr binary;
        UnaryExpr unary;
        CallExpr call;
        AssignExpr assign;
    } as;
};

typedef enum {
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BLOCK,
    STMT_PRINT,
    STMT_IMPORT
} StmtType;

typedef struct Stmt Stmt;

typedef struct {
    char* name;
    char* type_name;
    bool is_const;
    Expr* initializer;
} VarDeclStmt;

typedef struct {
    char* name;
    char** params;
    char** param_types;
    size_t param_count;
    char* return_type;
    Stmt** body;
    size_t body_count;
} FunctionStmt;

typedef struct {
    Expr* value;
} ReturnStmt;

typedef struct {
    Expr* condition;
    Stmt** then_branch;
    size_t then_count;
    Stmt** else_branch;
    size_t else_count;
} IfStmt;

typedef struct {
    Expr* condition;
    Stmt** body;
    size_t body_count;
} WhileStmt;

typedef struct {
    Stmt* init;
    Expr* condition;
    Expr* increment;
    Stmt** body;
    size_t body_count;
} ForStmt;

typedef struct {
    Stmt** statements;
    size_t count;
} BlockStmt;

typedef struct {
    Expr* expression;
} PrintStmt;

typedef struct {
    char* spec; // import target string (e.g., "mylib.dll" or identifier)
} ImportStmt;

struct Stmt {
    StmtType type;
    union {
        Expr* expression;
        VarDeclStmt var_decl;
        FunctionStmt function;
        ReturnStmt return_stmt;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        BlockStmt block;
        PrintStmt print_stmt;
        ImportStmt import_stmt;
    } as;
};

// Value operations
Value value_null();
Value value_bool(bool value);
Value value_number(double value);
Value value_string(const char* value);
void value_free(Value* value);
bool value_is_truthy(Value value);
void value_print(Value value);

// Expression constructors
Expr* expr_number(double value);
Expr* expr_string(const char* value);
Expr* expr_bool(bool value);
Expr* expr_null();
Expr* expr_identifier(const char* name);
Expr* expr_binary(const char* op, Expr* left, Expr* right);
Expr* expr_unary(const char* op, Expr* operand);
Expr* expr_call(Expr* callee, Expr** args, size_t arg_count);
Expr* expr_assign(const char* name, Expr* value);
void expr_free(Expr* expr);

// Statement constructors
Stmt* stmt_expression(Expr* expr);
Stmt* stmt_var_decl(const char* name, const char* type_name, bool is_const, Expr* initializer);
Stmt* stmt_function(const char* name, char** params, char** param_types, size_t param_count, const char* return_type, Stmt** body, size_t body_count);
Stmt* stmt_return(Expr* value);
Stmt* stmt_if(Expr* condition, Stmt** then_branch, size_t then_count, Stmt** else_branch, size_t else_count);
Stmt* stmt_while(Expr* condition, Stmt** body, size_t body_count);
Stmt* stmt_for(Stmt* init, Expr* condition, Expr* increment, Stmt** body, size_t body_count);
Stmt* stmt_block(Stmt** statements, size_t count);
Stmt* stmt_print(Expr* expr);
Stmt* stmt_import(const char* spec);
void stmt_free(Stmt* stmt);

#endif
