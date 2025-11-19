#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/ast.h"

typedef enum {
    LINT_ERROR,
    LINT_WARNING,
    LINT_INFO
} LintSeverity;

typedef struct {
    LintSeverity severity;
    int line;
    int column;
    char* message;
    char* rule_name;
} LintIssue;

typedef struct {
    LintIssue* issues;
    size_t issue_count;
    size_t capacity;
} LintReport;

typedef struct {
    bool check_naming_conventions;
    bool check_unused_variables;
    bool check_missing_return_types;
    bool check_dead_code;
    bool check_complexity;
    bool check_line_length;
    int max_line_length;
    int max_function_complexity;
} LintConfig;

void lint_report_init(LintReport* report) {
    report->issues = NULL;
    report->issue_count = 0;
    report->capacity = 0;
}

void lint_report_free(LintReport* report) {
    for (size_t i = 0; i < report->issue_count; i++) {
        free(report->issues[i].message);
        free(report->issues[i].rule_name);
    }
    free(report->issues);
}

void lint_report_add_issue(LintReport* report, LintSeverity severity, int line, int column,
                          const char* message, const char* rule_name) {
    if (report->issue_count >= report->capacity) {
        report->capacity = report->capacity == 0 ? 16 : report->capacity * 2;
        report->issues = realloc(report->issues, report->capacity * sizeof(LintIssue));
    }
    
    LintIssue* issue = &report->issues[report->issue_count];
    issue->severity = severity;
    issue->line = line;
    issue->column = column;
    issue->message = strdup(message);
    issue->rule_name = strdup(rule_name);
    
    report->issue_count++;
}

LintConfig default_lint_config() {
    LintConfig config;
    config.check_naming_conventions = true;
    config.check_unused_variables = true;
    config.check_missing_return_types = true;
    config.check_dead_code = true;
    config.check_complexity = true;
    config.check_line_length = true;
    config.max_line_length = 100;
    config.max_function_complexity = 10;
    return config;
}

bool is_snake_case(const char* name) {
    if (!name || !*name) return false;
    
    for (const char* p = name; *p; p++) {
        if (!islower(*p) && !isdigit(*p) && *p != '_') {
            return false;
        }
    }
    
    return islower(name[0]); // Must start with lowercase
}

bool is_pascal_case(const char* name) {
    if (!name || !*name) return false;
    
    if (!isupper(name[0])) return false;
    
    for (const char* p = name + 1; *p; p++) {
        if (!isalnum(*p)) return false;
    }
    
    return true;
}

bool is_camel_case(const char* name) {
    if (!name || !*name) return false;
    
    if (!islower(name[0])) return false;
    
    for (const char* p = name + 1; *p; p++) {
        if (!isalnum(*p)) return false;
    }
    
    return true;
}

void check_naming_conventions(LintReport* report, Stmt* stmt) {
    switch (stmt->type) {
        case STMT_VAR_DECL: {
            VarDeclStmt* var_decl = &stmt->as.var_decl;
            if (!is_snake_case(var_decl->name) && !is_camel_case(var_decl->name)) {
                char message[256];
                snprintf(message, sizeof(message), 
                        "Variable '%s' should use snake_case or camelCase naming", 
                        var_decl->name);
                lint_report_add_issue(report, LINT_WARNING, 0, 0, message, "naming-convention");
            }
            break;
        }
        
        case STMT_FUNCTION: {
            FunctionStmt* func = &stmt->as.function;
            if (!is_snake_case(func->name) && !is_camel_case(func->name)) {
                char message[256];
                snprintf(message, sizeof(message),
                        "Function '%s' should use snake_case or camelCase naming",
                        func->name);
                lint_report_add_issue(report, LINT_WARNING, 0, 0, message, "naming-convention");
            }
            
            // Check parameter naming
            for (size_t i = 0; i < func->param_count; i++) {
                if (!is_snake_case(func->params[i]) && !is_camel_case(func->params[i])) {
                    char message[256];
                    snprintf(message, sizeof(message),
                            "Parameter '%s' should use snake_case or camelCase naming",
                            func->params[i]);
                    lint_report_add_issue(report, LINT_WARNING, 0, 0, message, "naming-convention");
                }
            }
            break;
        }
        
        default:
            break;
    }
}

void check_missing_return_types(LintReport* report, Stmt* stmt) {
    if (stmt->type == STMT_FUNCTION) {
        FunctionStmt* func = &stmt->as.function;
        if (!func->return_type || strlen(func->return_type) == 0) {
            char message[256];
            snprintf(message, sizeof(message),
                    "Function '%s' is missing return type annotation",
                    func->name);
            lint_report_add_issue(report, LINT_WARNING, 0, 0, message, "missing-return-type");
        }
    }
}

int calculate_cyclomatic_complexity(Stmt** body, size_t body_count) {
    int complexity = 1; // Base complexity
    
    for (size_t i = 0; i < body_count; i++) {
        Stmt* stmt = body[i];
        
        switch (stmt->type) {
            case STMT_IF:
                complexity++; // +1 for if
                if (stmt->as.if_stmt.else_branch) {
                    complexity++; // +1 for else
                }
                complexity += calculate_cyclomatic_complexity(
                    stmt->as.if_stmt.then_branch, stmt->as.if_stmt.then_count);
                if (stmt->as.if_stmt.else_branch) {
                    complexity += calculate_cyclomatic_complexity(
                        stmt->as.if_stmt.else_branch, stmt->as.if_stmt.else_count);
                }
                break;
                
            case STMT_WHILE:
                complexity++; // +1 for while
                complexity += calculate_cyclomatic_complexity(
                    stmt->as.while_stmt.body, stmt->as.while_stmt.body_count);
                break;
                
            case STMT_FOR:
                complexity++; // +1 for for
                complexity += calculate_cyclomatic_complexity(
                    stmt->as.for_stmt.body, stmt->as.for_stmt.body_count);
                break;
                
            default:
                break;
        }
    }
    
    return complexity;
}

void check_function_complexity(LintReport* report, Stmt* stmt, LintConfig* config) {
    if (stmt->type == STMT_FUNCTION) {
        FunctionStmt* func = &stmt->as.function;
        int complexity = calculate_cyclomatic_complexity(func->body, func->body_count);
        
        if (complexity > config->max_function_complexity) {
            char message[256];
            snprintf(message, sizeof(message),
                    "Function '%s' has cyclomatic complexity of %d (max: %d)",
                    func->name, complexity, config->max_function_complexity);
            lint_report_add_issue(report, LINT_WARNING, 0, 0, message, "high-complexity");
        }
    }
}

bool has_return_statement(Stmt** body, size_t body_count) {
    for (size_t i = 0; i < body_count; i++) {
        if (body[i]->type == STMT_RETURN) {
            return true;
        }
        
        // Check nested blocks
        if (body[i]->type == STMT_IF) {
            IfStmt* if_stmt = &body[i]->as.if_stmt;
            if (has_return_statement(if_stmt->then_branch, if_stmt->then_count)) {
                return true;
            }
            if (if_stmt->else_branch && 
                has_return_statement(if_stmt->else_branch, if_stmt->else_count)) {
                return true;
            }
        }
    }
    return false;
}

void check_missing_return(LintReport* report, Stmt* stmt) {
    if (stmt->type == STMT_FUNCTION) {
        FunctionStmt* func = &stmt->as.function;
        
        // Skip void functions
        if (func->return_type && strcmp(func->return_type, "void") == 0) {
            return;
        }
        
        if (!has_return_statement(func->body, func->body_count)) {
            char message[256];
            snprintf(message, sizeof(message),
                    "Function '%s' is missing return statement",
                    func->name);
            lint_report_add_issue(report, LINT_ERROR, 0, 0, message, "missing-return");
        }
    }
}

void check_unreachable_code(LintReport* report, Stmt** body, size_t body_count) {
    bool found_return = false;
    
    for (size_t i = 0; i < body_count; i++) {
        if (found_return) {
            lint_report_add_issue(report, LINT_WARNING, 0, 0,
                                "Unreachable code after return statement",
                                "unreachable-code");
            break;
        }
        
        if (body[i]->type == STMT_RETURN) {
            found_return = true;
        }
    }
}

void check_line_length(LintReport* report, const char* source, LintConfig* config) {
    if (!config->check_line_length) return;
    
    const char* line_start = source;
    int line_number = 1;
    
    for (const char* p = source; *p; p++) {
        if (*p == '\n') {
            int line_length = p - line_start;
            if (line_length > config->max_line_length) {
                char message[256];
                snprintf(message, sizeof(message),
                        "Line %d exceeds maximum length of %d characters (%d)",
                        line_number, config->max_line_length, line_length);
                lint_report_add_issue(report, LINT_WARNING, line_number, 0, 
                                    message, "line-too-long");
            }
            line_start = p + 1;
            line_number++;
        }
    }
}

void lint_statement(LintReport* report, Stmt* stmt, LintConfig* config) {
    if (!stmt) return;
    
    if (config->check_naming_conventions) {
        check_naming_conventions(report, stmt);
    }
    
    if (config->check_missing_return_types) {
        check_missing_return_types(report, stmt);
    }
    
    if (config->check_complexity) {
        check_function_complexity(report, stmt, config);
    }
    
    check_missing_return(report, stmt);
    
    // Recursively check nested statements
    switch (stmt->type) {
        case STMT_FUNCTION: {
            FunctionStmt* func = &stmt->as.function;
            if (config->check_dead_code) {
                check_unreachable_code(report, func->body, func->body_count);
            }
            for (size_t i = 0; i < func->body_count; i++) {
                lint_statement(report, func->body[i], config);
            }
            break;
        }
        
        case STMT_IF: {
            IfStmt* if_stmt = &stmt->as.if_stmt;
            for (size_t i = 0; i < if_stmt->then_count; i++) {
                lint_statement(report, if_stmt->then_branch[i], config);
            }
            for (size_t i = 0; i < if_stmt->else_count; i++) {
                lint_statement(report, if_stmt->else_branch[i], config);
            }
            break;
        }
        
        case STMT_WHILE: {
            WhileStmt* while_stmt = &stmt->as.while_stmt;
            for (size_t i = 0; i < while_stmt->body_count; i++) {
                lint_statement(report, while_stmt->body[i], config);
            }
            break;
        }
        
        case STMT_FOR: {
            ForStmt* for_stmt = &stmt->as.for_stmt;
            for (size_t i = 0; i < for_stmt->body_count; i++) {
                lint_statement(report, for_stmt->body[i], config);
            }
            break;
        }
        
        case STMT_BLOCK: {
            BlockStmt* block = &stmt->as.block;
            for (size_t i = 0; i < block->count; i++) {
                lint_statement(report, block->statements[i], config);
            }
            break;
        }
        
        default:
            break;
    }
}

LintReport lint_rubolt_code(const char* source, LintConfig* config) {
    LintReport report;
    lint_report_init(&report);
    
    // Check line length
    check_line_length(&report, source, config);
    
    // Parse and analyze AST
    Lexer lexer;
    lexer_init(&lexer, source);
    
    Parser parser;
    parser_init(&parser, &lexer);
    
    while (!parser_is_at_end(&parser)) {
        Stmt* stmt = parse_statement(&parser);
        if (stmt) {
            lint_statement(&report, stmt, config);
            stmt_free(stmt);
        }
    }
    
    return report;
}

void print_lint_report(LintReport* report) {
    if (report->issue_count == 0) {
        printf("No issues found.\n");
        return;
    }
    
    size_t errors = 0, warnings = 0, info = 0;
    
    for (size_t i = 0; i < report->issue_count; i++) {
        LintIssue* issue = &report->issues[i];
        
        const char* severity_str;
        switch (issue->severity) {
            case LINT_ERROR:
                severity_str = "ERROR";
                errors++;
                break;
            case LINT_WARNING:
                severity_str = "WARNING";
                warnings++;
                break;
            case LINT_INFO:
                severity_str = "INFO";
                info++;
                break;
        }
        
        printf("%s:%d:%d: %s: %s [%s]\n",
               "file", issue->line, issue->column,
               severity_str, issue->message, issue->rule_name);
    }
    
    printf("\nSummary: %zu errors, %zu warnings, %zu info\n", errors, warnings, info);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file.rbo> [--config <config_file>]\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    LintConfig config = default_lint_config();
    
    // Parse command line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            // Load config from file (simplified)
            printf("Config file loading not implemented yet\n");
            i++;
        } else if (strcmp(argv[i], "--max-line-length") == 0 && i + 1 < argc) {
            config.max_line_length = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--max-complexity") == 0 && i + 1 < argc) {
            config.max_function_complexity = atoi(argv[i + 1]);
            i++;
        }
    }
    
    // Read input file
    FILE* file = fopen(input_file, "r");
    if (!file) {
        printf("Error: Cannot open file '%s'\n", input_file);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* source = malloc(size + 1);
    fread(source, 1, size, file);
    source[size] = '\0';
    fclose(file);
    
    // Lint code
    LintReport report = lint_rubolt_code(source, &config);
    
    // Print results
    print_lint_report(&report);
    
    // Cleanup
    free(source);
    lint_report_free(&report);
    
    return (report.issue_count > 0) ? 1 : 0;
}