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
    EXPR_ASSIGN,
    EXPR_FUNCTION,
    EXPR_ARRAY,
    EXPR_INDEX,
    EXPR_MEMBER
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

typedef struct {
    char** params;
    char** param_types;
    size_t param_count;
    char* return_type;
    Stmt** body;
    size_t body_count;
    bool is_nested;
} FunctionExpr;

typedef struct {
    Expr** elements;
    size_t count;
} ArrayExpr;

typedef struct {
    Expr* object;
    Expr* index;
} IndexExpr;

typedef struct {
    Expr* object;
    char* property;
} MemberExpr;

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
        FunctionExpr function;
        ArrayExpr array;
        IndexExpr index;
        MemberExpr member;
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
    STMT_FOR_IN,
    STMT_DO_WHILE,
    STMT_BLOCK,
    STMT_PRINT,
    STMT_IMPORT,
    STMT_BREAK,
    STMT_CONTINUE
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
    bool is_nested;
    struct FunctionStmt** nested_functions;
    size_t nested_count;
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
    char* variable;
    Expr* iterable;
    Stmt** body;
    size_t body_count;
} ForInStmt;

typedef struct {
    Stmt** body;
    size_t body_count;
    Expr* condition;
} DoWhileStmt;

typedef struct {
    char* label;
} BreakStmt;

typedef struct {
    char* label;
} ContinueStmt;

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
        ForInStmt for_in_stmt;
        DoWhileStmt do_while_stmt;
        BlockStmt block;
        PrintStmt print_stmt;
        ImportStmt import_stmt;
        BreakStmt break_stmt;
        ContinueStmt continue_stmt;
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
Expr* expr_function(char** params, char** param_types, size_t param_count, const char* return_type, Stmt** body, size_t body_count);
Expr* expr_array(Expr** elements, size_t count);
Expr* expr_index(Expr* object, Expr* index);
Expr* expr_member(Expr* object, const char* property);
void expr_free(Expr* expr);

// Statement constructors
Stmt* stmt_expression(Expr* expr);
Stmt* stmt_var_decl(const char* name, const char* type_name, bool is_const, Expr* initializer);
Stmt* stmt_function(const char* name, char** params, char** param_types, size_t param_count, const char* return_type, Stmt** body, size_t body_count);
Stmt* stmt_return(Expr* value);
Stmt* stmt_if(Expr* condition, Stmt** then_branch, size_t then_count, Stmt** else_branch, size_t else_count);
Stmt* stmt_while(Expr* condition, Stmt** body, size_t body_count);
Stmt* stmt_for(Stmt* init, Expr* condition, Expr* increment, Stmt** body, size_t body_count);
Stmt* stmt_for_in(const char* variable, Expr* iterable, Stmt** body, size_t body_count);
Stmt* stmt_do_while(Stmt** body, size_t body_count, Expr* condition);
Stmt* stmt_block(Stmt** statements, size_t count);
Stmt* stmt_print(Expr* expr);
Stmt* stmt_import(const char* spec);
Stmt* stmt_break(const char* label);
Stmt* stmt_continue(const char* label);
void stmt_free(Stmt* stmt);

// Scope management for nested functions
typedef struct Scope {
    struct Scope* parent;
    char** variables;
    size_t var_count;
    char** functions;
    size_t func_count;
} Scope;

Scope* scope_create(Scope* parent);
void scope_free(Scope* scope);
bool scope_define_var(Scope* scope, const char* name);
bool scope_define_func(Scope* scope, const char* name);
bool scope_lookup_var(Scope* scope, const char* name);
bool scope_lookup_func(Scope* scope, const char* name);

#endif
