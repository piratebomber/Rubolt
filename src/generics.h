#ifndef RUBOLT_GENERICS_H
#define RUBOLT_GENERICS_H

#include "ast.h"
#include <stddef.h>

typedef struct TypeParam {
    char* name;
    char* constraint; // Optional constraint like "Comparable"
    struct TypeParam* next;
} TypeParam;

typedef struct TypeArg {
    char* name;
    char* type;
    struct TypeArg* next;
} TypeArg;

typedef struct {
    char* name;
    TypeParam* type_params;
    char** param_names;
    char** param_types;
    size_t param_count;
    char* return_type;
    Stmt** body;
    size_t body_count;
} GenericFunction;

typedef struct {
    char* name;
    TypeParam* type_params;
    char** field_names;
    char** field_types;
    size_t field_count;
    Stmt** methods;
    size_t method_count;
} GenericClass;

typedef struct {
    char* base_name;
    TypeArg* type_args;
} GenericType;

typedef struct GenericInstance {
    char* name;
    GenericFunction* function;
    GenericClass* class_def;
    TypeArg* type_args;
    void* compiled_code; // JIT compiled version
    struct GenericInstance* next;
} GenericInstance;

typedef struct {
    GenericFunction* functions;
    size_t function_count;
    GenericClass* classes;
    size_t class_count;
    GenericInstance* instances;
} GenericRegistry;

// Type parameter operations
TypeParam* type_param_create(const char* name, const char* constraint);
void type_param_free(TypeParam* param);

// Type argument operations
TypeArg* type_arg_create(const char* name, const char* type);
void type_arg_free(TypeArg* arg);

// Generic function operations
GenericFunction* generic_function_create(const char* name, TypeParam* type_params,
                                       char** param_names, char** param_types, size_t param_count,
                                       const char* return_type, Stmt** body, size_t body_count);
void generic_function_free(GenericFunction* func);

// Generic class operations
GenericClass* generic_class_create(const char* name, TypeParam* type_params,
                                 char** field_names, char** field_types, size_t field_count,
                                 Stmt** methods, size_t method_count);
void generic_class_free(GenericClass* class_def);

// Generic type operations
GenericType* generic_type_create(const char* base_name, TypeArg* type_args);
void generic_type_free(GenericType* type);

// Generic registry operations
void generic_registry_init(GenericRegistry* registry);
void generic_registry_free(GenericRegistry* registry);
void generic_registry_add_function(GenericRegistry* registry, GenericFunction* func);
void generic_registry_add_class(GenericRegistry* registry, GenericClass* class_def);

// Generic instantiation
GenericInstance* instantiate_generic_function(GenericRegistry* registry, 
                                            const char* name, TypeArg* type_args);
GenericInstance* instantiate_generic_class(GenericRegistry* registry,
                                         const char* name, TypeArg* type_args);

// Type substitution
char* substitute_type(const char* type, TypeArg* type_args);
Stmt* substitute_stmt(Stmt* stmt, TypeArg* type_args);
Expr* substitute_expr(Expr* expr, TypeArg* type_args);

// Type constraint checking
bool check_type_constraints(TypeParam* params, TypeArg* args);
bool type_satisfies_constraint(const char* type, const char* constraint);

#endif