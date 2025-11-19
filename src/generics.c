#include "generics.h"
#include <stdlib.h>
#include <string.h>

TypeParam* type_param_create(const char* name, const char* constraint) {
    TypeParam* param = malloc(sizeof(TypeParam));
    param->name = strdup(name);
    param->constraint = constraint ? strdup(constraint) : NULL;
    param->next = NULL;
    return param;
}

void type_param_free(TypeParam* param) {
    while (param) {
        TypeParam* next = param->next;
        free(param->name);
        free(param->constraint);
        free(param);
        param = next;
    }
}

TypeArg* type_arg_create(const char* name, const char* type) {
    TypeArg* arg = malloc(sizeof(TypeArg));
    arg->name = strdup(name);
    arg->type = strdup(type);
    arg->next = NULL;
    return arg;
}

void type_arg_free(TypeArg* arg) {
    while (arg) {
        TypeArg* next = arg->next;
        free(arg->name);
        free(arg->type);
        free(arg);
        arg = next;
    }
}

GenericFunction* generic_function_create(const char* name, TypeParam* type_params,
                                       char** param_names, char** param_types, size_t param_count,
                                       const char* return_type, Stmt** body, size_t body_count) {
    GenericFunction* func = malloc(sizeof(GenericFunction));
    func->name = strdup(name);
    func->type_params = type_params;
    func->param_names = param_names;
    func->param_types = param_types;
    func->param_count = param_count;
    func->return_type = strdup(return_type);
    func->body = body;
    func->body_count = body_count;
    return func;
}

void generic_function_free(GenericFunction* func) {
    if (!func) return;
    
    free(func->name);
    type_param_free(func->type_params);
    
    for (size_t i = 0; i < func->param_count; i++) {
        free(func->param_names[i]);
        free(func->param_types[i]);
    }
    free(func->param_names);
    free(func->param_types);
    free(func->return_type);
    
    for (size_t i = 0; i < func->body_count; i++) {
        stmt_free(func->body[i]);
    }
    free(func->body);
    free(func);
}

GenericClass* generic_class_create(const char* name, TypeParam* type_params,
                                 char** field_names, char** field_types, size_t field_count,
                                 Stmt** methods, size_t method_count) {
    GenericClass* class_def = malloc(sizeof(GenericClass));
    class_def->name = strdup(name);
    class_def->type_params = type_params;
    class_def->field_names = field_names;
    class_def->field_types = field_types;
    class_def->field_count = field_count;
    class_def->methods = methods;
    class_def->method_count = method_count;
    return class_def;
}

void generic_class_free(GenericClass* class_def) {
    if (!class_def) return;
    
    free(class_def->name);
    type_param_free(class_def->type_params);
    
    for (size_t i = 0; i < class_def->field_count; i++) {
        free(class_def->field_names[i]);
        free(class_def->field_types[i]);
    }
    free(class_def->field_names);
    free(class_def->field_types);
    
    for (size_t i = 0; i < class_def->method_count; i++) {
        stmt_free(class_def->methods[i]);
    }
    free(class_def->methods);
    free(class_def);
}

GenericType* generic_type_create(const char* base_name, TypeArg* type_args) {
    GenericType* type = malloc(sizeof(GenericType));
    type->base_name = strdup(base_name);
    type->type_args = type_args;
    return type;
}

void generic_type_free(GenericType* type) {
    if (!type) return;
    free(type->base_name);
    type_arg_free(type->type_args);
    free(type);
}

void generic_registry_init(GenericRegistry* registry) {
    registry->functions = NULL;
    registry->function_count = 0;
    registry->classes = NULL;
    registry->class_count = 0;
    registry->instances = NULL;
}

void generic_registry_free(GenericRegistry* registry) {
    for (size_t i = 0; i < registry->function_count; i++) {
        generic_function_free(&registry->functions[i]);
    }
    free(registry->functions);
    
    for (size_t i = 0; i < registry->class_count; i++) {
        generic_class_free(&registry->classes[i]);
    }
    free(registry->classes);
    
    GenericInstance* instance = registry->instances;
    while (instance) {
        GenericInstance* next = instance->next;
        free(instance->name);
        type_arg_free(instance->type_args);
        free(instance);
        instance = next;
    }
}

void generic_registry_add_function(GenericRegistry* registry, GenericFunction* func) {
    registry->functions = realloc(registry->functions, 
                                (registry->function_count + 1) * sizeof(GenericFunction));
    registry->functions[registry->function_count] = *func;
    registry->function_count++;
}

void generic_registry_add_class(GenericRegistry* registry, GenericClass* class_def) {
    registry->classes = realloc(registry->classes,
                              (registry->class_count + 1) * sizeof(GenericClass));
    registry->classes[registry->class_count] = *class_def;
    registry->class_count++;
}

char* substitute_type(const char* type, TypeArg* type_args) {
    TypeArg* arg = type_args;
    while (arg) {
        if (strcmp(type, arg->name) == 0) {
            return strdup(arg->type);
        }
        arg = arg->next;
    }
    return strdup(type); // No substitution needed
}

Stmt* substitute_stmt(Stmt* stmt, TypeArg* type_args) {
    if (!stmt) return NULL;
    
    switch (stmt->type) {
        case STMT_VAR_DECL: {
            VarDeclStmt* var_decl = &stmt->as.var_decl;
            char* new_type = substitute_type(var_decl->type_name, type_args);
            Expr* new_init = substitute_expr(var_decl->initializer, type_args);
            return stmt_var_decl(var_decl->name, new_type, var_decl->is_const, new_init);
        }
        
        case STMT_FUNCTION: {
            FunctionStmt* func = &stmt->as.function;
            
            // Substitute parameter types
            char** new_param_types = malloc(func->param_count * sizeof(char*));
            for (size_t i = 0; i < func->param_count; i++) {
                new_param_types[i] = substitute_type(func->param_types[i], type_args);
            }
            
            // Substitute return type
            char* new_return_type = substitute_type(func->return_type, type_args);
            
            // Substitute body
            Stmt** new_body = malloc(func->body_count * sizeof(Stmt*));
            for (size_t i = 0; i < func->body_count; i++) {
                new_body[i] = substitute_stmt(func->body[i], type_args);
            }
            
            return stmt_function(func->name, func->params, new_param_types, func->param_count,
                               new_return_type, new_body, func->body_count);
        }
        
        case STMT_RETURN:
            return stmt_return(substitute_expr(stmt->as.return_stmt.value, type_args));
            
        case STMT_IF: {
            IfStmt* if_stmt = &stmt->as.if_stmt;
            Expr* new_condition = substitute_expr(if_stmt->condition, type_args);
            
            Stmt** new_then = malloc(if_stmt->then_count * sizeof(Stmt*));
            for (size_t i = 0; i < if_stmt->then_count; i++) {
                new_then[i] = substitute_stmt(if_stmt->then_branch[i], type_args);
            }
            
            Stmt** new_else = NULL;
            if (if_stmt->else_branch) {
                new_else = malloc(if_stmt->else_count * sizeof(Stmt*));
                for (size_t i = 0; i < if_stmt->else_count; i++) {
                    new_else[i] = substitute_stmt(if_stmt->else_branch[i], type_args);
                }
            }
            
            return stmt_if(new_condition, new_then, if_stmt->then_count, 
                          new_else, if_stmt->else_count);
        }
        
        case STMT_WHILE: {
            WhileStmt* while_stmt = &stmt->as.while_stmt;
            Expr* new_condition = substitute_expr(while_stmt->condition, type_args);
            
            Stmt** new_body = malloc(while_stmt->body_count * sizeof(Stmt*));
            for (size_t i = 0; i < while_stmt->body_count; i++) {
                new_body[i] = substitute_stmt(while_stmt->body[i], type_args);
            }
            
            return stmt_while(new_condition, new_body, while_stmt->body_count);
        }
        
        case STMT_FOR: {
            ForStmt* for_stmt = &stmt->as.for_stmt;
            Stmt* new_init = substitute_stmt(for_stmt->init, type_args);
            Expr* new_condition = substitute_expr(for_stmt->condition, type_args);
            Expr* new_increment = substitute_expr(for_stmt->increment, type_args);
            
            Stmt** new_body = malloc(for_stmt->body_count * sizeof(Stmt*));
            for (size_t i = 0; i < for_stmt->body_count; i++) {
                new_body[i] = substitute_stmt(for_stmt->body[i], type_args);
            }
            
            return stmt_for(new_init, new_condition, new_increment, new_body, for_stmt->body_count);
        }
        
        case STMT_BLOCK: {
            BlockStmt* block = &stmt->as.block;
            Stmt** new_statements = malloc(block->count * sizeof(Stmt*));
            for (size_t i = 0; i < block->count; i++) {
                new_statements[i] = substitute_stmt(block->statements[i], type_args);
            }
            return stmt_block(new_statements, block->count);
        }
        
        case STMT_PRINT:
            return stmt_print(substitute_expr(stmt->as.print_stmt.expression, type_args));
            
        case STMT_IMPORT:
            return stmt_import(stmt->as.import_stmt.spec);
            
        case STMT_EXPR:
            return stmt_expression(substitute_expr(stmt->as.expression, type_args));
            
        default:
            runtime_panic("Unknown statement type in generic substitution: %d", stmt->type);
            return NULL;
    }
}

Expr* substitute_expr(Expr* expr, TypeArg* type_args) {
    if (!expr) return NULL;
    
    switch (expr->type) {
        case EXPR_BINARY:
            return expr_binary(expr->as.binary.op,
                             substitute_expr(expr->as.binary.left, type_args),
                             substitute_expr(expr->as.binary.right, type_args));
            
        case EXPR_UNARY:
            return expr_unary(expr->as.unary.op,
                            substitute_expr(expr->as.unary.operand, type_args));
            
        case EXPR_CALL: {
            CallExpr* call = &expr->as.call;
            Expr* new_callee = substitute_expr(call->callee, type_args);
            
            Expr** new_args = malloc(call->arg_count * sizeof(Expr*));
            for (size_t i = 0; i < call->arg_count; i++) {
                new_args[i] = substitute_expr(call->args[i], type_args);
            }
            
            return expr_call(new_callee, new_args, call->arg_count);
        }
        
        case EXPR_ASSIGN:
            return expr_assign(expr->as.assign.name,
                             substitute_expr(expr->as.assign.value, type_args));
            
        case EXPR_IDENTIFIER: {
            // Check if identifier is a type parameter
            TypeArg* arg = type_args;
            while (arg) {
                if (strcmp(expr->as.identifier, arg->name) == 0) {
                    return expr_identifier(arg->type);
                }
                arg = arg->next;
            }
            return expr_identifier(expr->as.identifier);
        }
        
        case EXPR_NUMBER:
        case EXPR_STRING:
        case EXPR_BOOL:
        case EXPR_NULL:
            // Literals don't need substitution, create copies
            switch (expr->type) {
                case EXPR_NUMBER: return expr_number(expr->as.number);
                case EXPR_STRING: return expr_string(expr->as.string);
                case EXPR_BOOL: return expr_bool(expr->as.boolean);
                case EXPR_NULL: return expr_null();
                default: break;
            }
            break;
    }
}

GenericInstance* instantiate_generic_function(GenericRegistry* registry, 
                                            const char* name, TypeArg* type_args) {
    // Find generic function
    GenericFunction* generic_func = NULL;
    for (size_t i = 0; i < registry->function_count; i++) {
        if (strcmp(registry->functions[i].name, name) == 0) {
            generic_func = &registry->functions[i];
            break;
        }
    }
    
    if (!generic_func) return NULL;
    
    // Check if already instantiated
    GenericInstance* instance = registry->instances;
    while (instance) {
        if (instance->function == generic_func) {
            // Compare type arguments
            TypeArg* arg1 = instance->type_args;
            TypeArg* arg2 = type_args;
            bool match = true;
            
            while (arg1 && arg2) {
                if (strcmp(arg1->name, arg2->name) != 0 || 
                    strcmp(arg1->type, arg2->type) != 0) {
                    match = false;
                    break;
                }
                arg1 = arg1->next;
                arg2 = arg2->next;
            }
            
            if (match && !arg1 && !arg2) {
                return instance; // Already instantiated
            }
        }
        instance = instance->next;
    }
    
    // Check type constraints
    if (!check_type_constraints(generic_func->type_params, type_args)) {
        return NULL;
    }
    
    // Create new instance
    instance = malloc(sizeof(GenericInstance));
    
    // Generate instance name
    char* instance_name = malloc(strlen(name) + 100);
    strcpy(instance_name, name);
    strcat(instance_name, "<");
    
    TypeArg* arg = type_args;
    bool first = true;
    while (arg) {
        if (!first) strcat(instance_name, ",");
        strcat(instance_name, arg->type);
        first = false;
        arg = arg->next;
    }
    strcat(instance_name, ">");
    
    instance->name = instance_name;
    instance->function = generic_func;
    instance->class_def = NULL;
    instance->type_args = type_args;
    instance->compiled_code = NULL;
    instance->next = registry->instances;
    registry->instances = instance;
    
    return instance;
}

GenericInstance* instantiate_generic_class(GenericRegistry* registry,
                                         const char* name, TypeArg* type_args) {
    GenericClass* generic_class = NULL;
    for (size_t i = 0; i < registry->class_count; i++) {
        if (strcmp(registry->classes[i].name, name) == 0) {
            generic_class = &registry->classes[i];
            break;
        }
    }
    
    if (!generic_class) {
        runtime_panic_with_type(PANIC_GENERIC_INSTANTIATION_FAILED,
                               "Generic class not found: %s", name);
        return NULL;
    }
    
    // Comprehensive constraint checking
    if (!check_type_constraints_advanced(generic_class->type_params, type_args, registry)) {
        runtime_panic_with_type(PANIC_CONSTRAINT_VIOLATION,
                               "Type constraints not satisfied for generic class: %s", name);
        return NULL;
    }
    
    // Check for circular dependencies in type parameters
    if (has_circular_type_dependency(type_args)) {
        runtime_panic_with_type(PANIC_GENERIC_INSTANTIATION_FAILED,
                               "Circular type dependency detected in generic class: %s", name);
        return NULL;
    }
    
    GenericInstance* instance = malloc(sizeof(GenericInstance));
    
    char* instance_name = malloc(strlen(name) + 100);
    strcpy(instance_name, name);
    strcat(instance_name, "<");
    
    TypeArg* arg = type_args;
    bool first = true;
    while (arg) {
        if (!first) strcat(instance_name, ",");
        strcat(instance_name, arg->type);
        first = false;
        arg = arg->next;
    }
    strcat(instance_name, ">");
    
    instance->name = instance_name;
    instance->function = NULL;
    instance->class_def = generic_class;
    instance->type_args = type_args;
    instance->compiled_code = NULL;
    instance->next = registry->instances;
    registry->instances = instance;
    
    return instance;
}

bool check_type_constraints_advanced(TypeParam* params, TypeArg* args, GenericRegistry* registry) {
    TypeParam* param = params;
    while (param) {
        TypeArg* arg = args;
        bool found = false;
        
        while (arg) {
            if (strcmp(param->name, arg->name) == 0) {
                found = true;
                
                // Check basic constraint
                if (param->constraint && 
                    !type_satisfies_constraint_advanced(arg->type, param->constraint, registry)) {
                    return false;
                }
                
                // Check constraint dependencies
                if (!validate_constraint_dependencies(param->constraint, arg->type, registry)) {
                    return false;
                }
                
                // Check type parameter bounds
                if (!validate_type_bounds(param, arg, registry)) {
                    return false;
                }
                
                break;
            }
            arg = arg->next;
        }
        
        if (!found) {
            runtime_panic_with_type(PANIC_CONSTRAINT_VIOLATION,
                                   "Missing type argument for parameter: %s", param->name);
            return false;
        }
        param = param->next;
    }
    
    // Cross-parameter constraint validation
    return validate_cross_parameter_constraints(params, args, registry);
}

bool type_satisfies_constraint_advanced(const char* type, const char* constraint, GenericRegistry* registry) {
    if (!constraint) return true;
    
    // Parse complex constraints like "Comparable + Serializable"
    char* constraint_copy = strdup(constraint);
    char* token = strtok(constraint_copy, "+&|");
    
    while (token) {
        // Trim whitespace
        while (*token == ' ') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';
        
        if (!type_satisfies_constraint(type, token)) {
            free(constraint_copy);
            return false;
        }
        
        token = strtok(NULL, "+&|");
    }
    
    free(constraint_copy);
    return true;
}

bool validate_constraint_dependencies(const char* constraint, const char* type, GenericRegistry* registry) {
    if (!constraint) return true;
    
    // Check if constraint has dependencies that must also be satisfied
    ConstraintRule* rule = find_constraint_rule(constraint);
    if (!rule) return true;
    
    // Validate that all required traits are implemented
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    for (size_t i = 0; i < rule->trait_count; i++) {
        bool trait_found = false;
        for (size_t j = 0; j < type_info->trait_count; j++) {
            if (strcmp(rule->required_traits[i], type_info->implemented_traits[j]) == 0) {
                trait_found = true;
                break;
            }
        }
        
        if (!trait_found) {
            // Check if trait can be derived
            if (!can_derive_trait(type, rule->required_traits[i], registry)) {
                return false;
            }
        }
    }
    
    return true;
}

bool validate_type_bounds(TypeParam* param, TypeArg* arg, GenericRegistry* registry) {
    // Validate upper and lower bounds for type parameters
    if (param->constraint) {
        // Check upper bound (T: UpperBound)
        if (strstr(param->constraint, ":")) {
            char* upper_bound = strchr(param->constraint, ':') + 1;
            while (*upper_bound == ' ') upper_bound++;
            
            if (!type_is_subtype(arg->type, upper_bound)) {
                return false;
            }
        }
        
        // Check lower bound (T super LowerBound)
        if (strstr(param->constraint, "super")) {
            char* super_pos = strstr(param->constraint, "super");
            char* lower_bound = super_pos + 5;
            while (*lower_bound == ' ') lower_bound++;
            
            if (!type_is_subtype(lower_bound, arg->type)) {
                return false;
            }
        }
    }
    
    return true;
}

bool validate_cross_parameter_constraints(TypeParam* params, TypeArg* args, GenericRegistry* registry) {
    // Validate constraints that involve multiple type parameters
    TypeParam* param1 = params;
    
    while (param1) {
        TypeParam* param2 = param1->next;
        
        while (param2) {
            // Find corresponding arguments
            TypeArg* arg1 = find_type_arg(args, param1->name);
            TypeArg* arg2 = find_type_arg(args, param2->name);
            
            if (arg1 && arg2) {
                // Check if types are compatible for operations
                if (!types_are_compatible(arg1->type, arg2->type, registry)) {
                    return false;
                }
                
                // Check variance constraints
                if (!validate_variance_constraints(param1, param2, arg1, arg2, registry)) {
                    return false;
                }
            }
            
            param2 = param2->next;
        }
        
        param1 = param1->next;
    }
    
    return true;
}

bool has_circular_type_dependency(TypeArg* args) {
    TypeArg* current = args;
    
    while (current) {
        if (type_references_itself(current->type, current->name, args)) {
            return true;
        }
        current = current->next;
    }
    
    return false;
}

bool type_references_itself(const char* type, const char* param_name, TypeArg* args) {
    // Check if type contains reference to param_name
    if (strstr(type, param_name)) {
        return true;
    }
    
    // Check indirect references through other type parameters
    TypeArg* arg = args;
    while (arg) {
        if (strcmp(arg->name, param_name) != 0 && strstr(type, arg->name)) {
            if (type_references_itself(arg->type, param_name, args)) {
                return true;
            }
        }
        arg = arg->next;
    }
    
    return false;
}

bool can_derive_trait(const char* type, const char* trait, GenericRegistry* registry) {
    // Check if trait can be automatically derived for the type
    if (strcmp(trait, "Clone") == 0) {
        // Clone can be derived if all fields are cloneable
        return type_has_cloneable_fields(type, registry);
    }
    
    if (strcmp(trait, "Eq") == 0) {
        // Eq can be derived if all fields are comparable
        return type_has_comparable_fields(type, registry);
    }
    
    if (strcmp(trait, "Serialize") == 0) {
        // Serialize can be derived if all fields are serializable
        return type_has_serializable_fields(type, registry);
    }
    
    return false;
}

bool type_has_cloneable_fields(const char* type, GenericRegistry* registry) {
    // Check if all fields of the type implement Clone
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    // For primitive types, cloning is always possible
    if (strcmp(type, "number") == 0 || strcmp(type, "string") == 0 || 
        strcmp(type, "bool") == 0) {
        return true;
    }
    
    // For complex types, check field types (simplified)
    return true; // Would need field type information
}

bool type_has_comparable_fields(const char* type, GenericRegistry* registry) {
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    // Check if type has compare method or all fields are comparable
    for (size_t i = 0; i < type_info->method_count; i++) {
        if (strcmp(type_info->available_methods[i], "compare") == 0) {
            return true;
        }
    }
    
    return false;
}

bool type_has_serializable_fields(const char* type, GenericRegistry* registry) {
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    // Check if type has serialize method
    for (size_t i = 0; i < type_info->method_count; i++) {
        if (strcmp(type_info->available_methods[i], "serialize") == 0) {
            return true;
        }
    }
    
    return false;
}

bool types_are_compatible(const char* type1, const char* type2, GenericRegistry* registry) {
    // Check if two types can be used together in operations
    if (strcmp(type1, type2) == 0) return true;
    
    // Check if one is subtype of the other
    if (type_is_subtype(type1, type2) || type_is_subtype(type2, type1)) {
        return true;
    }
    
    // Check if both implement common traits
    TypeHierarchy* info1 = find_type_hierarchy(type1);
    TypeHierarchy* info2 = find_type_hierarchy(type2);
    
    if (!info1 || !info2) return false;
    
    // Find common traits
    for (size_t i = 0; i < info1->trait_count; i++) {
        for (size_t j = 0; j < info2->trait_count; j++) {
            if (strcmp(info1->implemented_traits[i], info2->implemented_traits[j]) == 0) {
                return true;
            }
        }
    }
    
    return false;
}

bool validate_variance_constraints(TypeParam* param1, TypeParam* param2, 
                                  TypeArg* arg1, TypeArg* arg2, 
                                  GenericRegistry* registry) {
    // Validate covariance/contravariance constraints
    // This is a simplified implementation
    return true;
}

TypeArg* find_type_arg(TypeArg* args, const char* name) {
    TypeArg* current = args;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

bool check_type_constraints(TypeParam* params, TypeArg* args) {
    return check_type_constraints_advanced(params, args, NULL);
}

typedef struct ConstraintRule {
    char* constraint_name;
    char** required_methods;
    size_t method_count;
    char** required_traits;
    size_t trait_count;
    bool (*validator)(const char* type, void* context);
} ConstraintRule;

typedef struct TypeHierarchy {
    char* type_name;
    char** parent_types;
    size_t parent_count;
    char** implemented_traits;
    size_t trait_count;
    char** available_methods;
    size_t method_count;
} TypeHierarchy;

static ConstraintRule constraint_rules[] = {
    {
        .constraint_name = "Comparable",
        .required_methods = (char*[]){"compare", "equals", "hash"},
        .method_count = 3,
        .required_traits = (char*[]){"Ord", "Eq"},
        .trait_count = 2,
        .validator = validate_comparable_constraint
    },
    {
        .constraint_name = "Numeric",
        .required_methods = (char*[]){"add", "subtract", "multiply", "divide", "modulo"},
        .method_count = 5,
        .required_traits = (char*[]){"Add", "Sub", "Mul", "Div", "Rem"},
        .trait_count = 5,
        .validator = validate_numeric_constraint
    },
    {
        .constraint_name = "Iterable",
        .required_methods = (char*[]){"iterator", "length", "get"},
        .method_count = 3,
        .required_traits = (char*[]){"Iterator"},
        .trait_count = 1,
        .validator = validate_iterable_constraint
    },
    {
        .constraint_name = "Serializable",
        .required_methods = (char*[]){"serialize", "deserialize", "to_json", "from_json"},
        .method_count = 4,
        .required_traits = (char*[]){"Serialize", "Deserialize"},
        .trait_count = 2,
        .validator = validate_serializable_constraint
    },
    {
        .constraint_name = "Cloneable",
        .required_methods = (char*[]){"clone", "deep_clone"},
        .method_count = 2,
        .required_traits = (char*[]){"Clone"},
        .trait_count = 1,
        .validator = validate_cloneable_constraint
    }
};

static TypeHierarchy type_hierarchy[] = {
    {
        .type_name = "number",
        .parent_types = (char*[]){"Numeric", "Comparable", "Serializable"},
        .parent_count = 3,
        .implemented_traits = (char*[]){"Add", "Sub", "Mul", "Div", "Rem", "Ord", "Eq", "Clone"},
        .trait_count = 8,
        .available_methods = (char*[]){"add", "subtract", "multiply", "divide", "modulo", "compare", "equals", "hash", "clone", "to_string"},
        .method_count = 10
    },
    {
        .type_name = "string",
        .parent_types = (char*[]){"Comparable", "Iterable", "Serializable"},
        .parent_count = 3,
        .implemented_traits = (char*[]){"Ord", "Eq", "Iterator", "Clone"},
        .trait_count = 4,
        .available_methods = (char*[]){"compare", "equals", "hash", "iterator", "length", "get", "substring", "split", "clone"},
        .method_count = 9
    },
    {
        .type_name = "list",
        .parent_types = (char*[]){"Iterable", "Serializable", "Cloneable"},
        .parent_count = 3,
        .implemented_traits = (char*[]){"Iterator", "Clone"},
        .trait_count = 2,
        .available_methods = (char*[]){"iterator", "length", "get", "set", "append", "remove", "clone", "deep_clone"},
        .method_count = 8
    }
};

bool validate_comparable_constraint(const char* type, void* context) {
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    bool has_compare = false, has_equals = false, has_hash = false;
    for (size_t i = 0; i < type_info->method_count; i++) {
        if (strcmp(type_info->available_methods[i], "compare") == 0) has_compare = true;
        if (strcmp(type_info->available_methods[i], "equals") == 0) has_equals = true;
        if (strcmp(type_info->available_methods[i], "hash") == 0) has_hash = true;
    }
    
    return has_compare && has_equals && has_hash;
}

bool validate_numeric_constraint(const char* type, void* context) {
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    char* required[] = {"add", "subtract", "multiply", "divide", "modulo"};
    for (size_t i = 0; i < 5; i++) {
        bool found = false;
        for (size_t j = 0; j < type_info->method_count; j++) {
            if (strcmp(type_info->available_methods[j], required[i]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    return true;
}

bool validate_iterable_constraint(const char* type, void* context) {
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    bool has_iterator = false, has_length = false;
    for (size_t i = 0; i < type_info->method_count; i++) {
        if (strcmp(type_info->available_methods[i], "iterator") == 0) has_iterator = true;
        if (strcmp(type_info->available_methods[i], "length") == 0) has_length = true;
    }
    
    return has_iterator && has_length;
}

bool validate_serializable_constraint(const char* type, void* context) {
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    bool has_serialize = false, has_deserialize = false;
    for (size_t i = 0; i < type_info->method_count; i++) {
        if (strcmp(type_info->available_methods[i], "serialize") == 0) has_serialize = true;
        if (strcmp(type_info->available_methods[i], "deserialize") == 0) has_deserialize = true;
    }
    
    return has_serialize && has_deserialize;
}

bool validate_cloneable_constraint(const char* type, void* context) {
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) return false;
    
    for (size_t i = 0; i < type_info->method_count; i++) {
        if (strcmp(type_info->available_methods[i], "clone") == 0) {
            return true;
        }
    }
    
    return false;
}

TypeHierarchy* find_type_hierarchy(const char* type) {
    for (size_t i = 0; i < sizeof(type_hierarchy) / sizeof(TypeHierarchy); i++) {
        if (strcmp(type_hierarchy[i].type_name, type) == 0) {
            return &type_hierarchy[i];
        }
    }
    return NULL;
}

bool type_satisfies_constraint(const char* type, const char* constraint) {
    ConstraintRule* rule = find_constraint_rule(constraint);
    if (!rule) {
        runtime_panic("Unknown constraint: %s", constraint);
        return false;
    }
    
    TypeHierarchy* type_info = find_type_hierarchy(type);
    if (!type_info) {
        runtime_panic("Unknown type: %s", type);
        return false;
    }
    
    // Check if type directly implements constraint
    for (size_t i = 0; i < type_info->parent_count; i++) {
        if (strcmp(type_info->parent_types[i], constraint) == 0) {
            return true;
        }
    }
    
    // Check required methods
    for (size_t i = 0; i < rule->method_count; i++) {
        bool method_found = false;
        for (size_t j = 0; j < type_info->method_count; j++) {
            if (strcmp(rule->required_methods[i], type_info->available_methods[j]) == 0) {
                method_found = true;
                break;
            }
        }
        if (!method_found) {
            return false;
        }
    }
    
    // Check required traits
    for (size_t i = 0; i < rule->trait_count; i++) {
        bool trait_found = false;
        for (size_t j = 0; j < type_info->trait_count; j++) {
            if (strcmp(rule->required_traits[i], type_info->implemented_traits[j]) == 0) {
                trait_found = true;
                break;
            }
        }
        if (!trait_found) {
            return false;
        }
    }
    
    // Run custom validator if present
    if (rule->validator) {
        return rule->validator(type, NULL);
    }
    
    return true;
}

ConstraintRule* find_constraint_rule(const char* constraint) {
    for (size_t i = 0; i < sizeof(constraint_rules) / sizeof(ConstraintRule); i++) {
        if (strcmp(constraint_rules[i].constraint_name, constraint) == 0) {
            return &constraint_rules[i];
        }
    }
    return NULL;
}