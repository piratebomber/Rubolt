#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../src/lexer.h"
#include "../src/parser.h"

typedef struct {
    char* output;
    size_t capacity;
    size_t length;
    int indent_level;
    int indent_size;
    bool at_line_start;
} Formatter;

void formatter_init(Formatter* fmt) {
    fmt->capacity = 4096;
    fmt->output = malloc(fmt->capacity);
    fmt->length = 0;
    fmt->indent_level = 0;
    fmt->indent_size = 4;
    fmt->at_line_start = true;
}

void formatter_free(Formatter* fmt) {
    free(fmt->output);
}

void formatter_ensure_capacity(Formatter* fmt, size_t needed) {
    if (fmt->length + needed >= fmt->capacity) {
        fmt->capacity *= 2;
        fmt->output = realloc(fmt->output, fmt->capacity);
    }
}

void formatter_append(Formatter* fmt, const char* str) {
    size_t len = strlen(str);
    formatter_ensure_capacity(fmt, len + 1);
    strcpy(fmt->output + fmt->length, str);
    fmt->length += len;
    fmt->at_line_start = false;
}

void formatter_append_char(Formatter* fmt, char c) {
    formatter_ensure_capacity(fmt, 2);
    fmt->output[fmt->length++] = c;
    fmt->output[fmt->length] = '\0';
    fmt->at_line_start = (c == '\n');
}

void formatter_newline(Formatter* fmt) {
    formatter_append_char(fmt, '\n');
    fmt->at_line_start = true;
}

void formatter_indent(Formatter* fmt) {
    if (fmt->at_line_start) {
        for (int i = 0; i < fmt->indent_level * fmt->indent_size; i++) {
            formatter_append_char(fmt, ' ');
        }
        fmt->at_line_start = false;
    }
}

void formatter_increase_indent(Formatter* fmt) {
    fmt->indent_level++;
}

void formatter_decrease_indent(Formatter* fmt) {
    if (fmt->indent_level > 0) {
        fmt->indent_level--;
    }
}

void format_expr(Formatter* fmt, Expr* expr);
void format_stmt(Formatter* fmt, Stmt* stmt);

void format_expr(Formatter* fmt, Expr* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case EXPR_NUMBER: {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.15g", expr->as.number);
            formatter_append(fmt, buffer);
            break;
        }
        
        case EXPR_STRING:
            formatter_append(fmt, "\"");
            formatter_append(fmt, expr->as.string);
            formatter_append(fmt, "\"");
            break;
            
        case EXPR_BOOL:
            formatter_append(fmt, expr->as.boolean ? "true" : "false");
            break;
            
        case EXPR_NULL:
            formatter_append(fmt, "null");
            break;
            
        case EXPR_IDENTIFIER:
            formatter_append(fmt, expr->as.identifier);
            break;
            
        case EXPR_BINARY:
            format_expr(fmt, expr->as.binary.left);
            formatter_append(fmt, " ");
            formatter_append(fmt, expr->as.binary.op);
            formatter_append(fmt, " ");
            format_expr(fmt, expr->as.binary.right);
            break;
            
        case EXPR_UNARY:
            formatter_append(fmt, expr->as.unary.op);
            format_expr(fmt, expr->as.unary.operand);
            break;
            
        case EXPR_CALL:
            format_expr(fmt, expr->as.call.callee);
            formatter_append(fmt, "(");
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                if (i > 0) formatter_append(fmt, ", ");
                format_expr(fmt, expr->as.call.args[i]);
            }
            formatter_append(fmt, ")");
            break;
            
        case EXPR_ASSIGN:
            formatter_append(fmt, expr->as.assign.name);
            formatter_append(fmt, " = ");
            format_expr(fmt, expr->as.assign.value);
            break;
    }
}

void format_stmt(Formatter* fmt, Stmt* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_EXPR:
            formatter_indent(fmt);
            format_expr(fmt, stmt->as.expression);
            formatter_append(fmt, ";");
            formatter_newline(fmt);
            break;
            
        case STMT_VAR_DECL: {
            VarDeclStmt* var_decl = &stmt->as.var_decl;
            formatter_indent(fmt);
            formatter_append(fmt, var_decl->is_const ? "const " : "let ");
            formatter_append(fmt, var_decl->name);
            if (var_decl->type_name) {
                formatter_append(fmt, ": ");
                formatter_append(fmt, var_decl->type_name);
            }
            if (var_decl->initializer) {
                formatter_append(fmt, " = ");
                format_expr(fmt, var_decl->initializer);
            }
            formatter_append(fmt, ";");
            formatter_newline(fmt);
            break;
        }
        
        case STMT_FUNCTION: {
            FunctionStmt* func = &stmt->as.function;
            formatter_indent(fmt);
            formatter_append(fmt, "def ");
            formatter_append(fmt, func->name);
            formatter_append(fmt, "(");
            
            for (size_t i = 0; i < func->param_count; i++) {
                if (i > 0) formatter_append(fmt, ", ");
                formatter_append(fmt, func->params[i]);
                if (func->param_types && func->param_types[i]) {
                    formatter_append(fmt, ": ");
                    formatter_append(fmt, func->param_types[i]);
                }
            }
            
            formatter_append(fmt, ")");
            if (func->return_type) {
                formatter_append(fmt, " -> ");
                formatter_append(fmt, func->return_type);
            }
            formatter_append(fmt, " {");
            formatter_newline(fmt);
            
            formatter_increase_indent(fmt);
            for (size_t i = 0; i < func->body_count; i++) {
                format_stmt(fmt, func->body[i]);
            }
            formatter_decrease_indent(fmt);
            
            formatter_indent(fmt);
            formatter_append(fmt, "}");
            formatter_newline(fmt);
            break;
        }
        
        case STMT_RETURN:
            formatter_indent(fmt);
            formatter_append(fmt, "return");
            if (stmt->as.return_stmt.value) {
                formatter_append(fmt, " ");
                format_expr(fmt, stmt->as.return_stmt.value);
            }
            formatter_append(fmt, ";");
            formatter_newline(fmt);
            break;
            
        case STMT_IF: {
            IfStmt* if_stmt = &stmt->as.if_stmt;
            formatter_indent(fmt);
            formatter_append(fmt, "if (");
            format_expr(fmt, if_stmt->condition);
            formatter_append(fmt, ") {");
            formatter_newline(fmt);
            
            formatter_increase_indent(fmt);
            for (size_t i = 0; i < if_stmt->then_count; i++) {
                format_stmt(fmt, if_stmt->then_branch[i]);
            }
            formatter_decrease_indent(fmt);
            
            formatter_indent(fmt);
            formatter_append(fmt, "}");
            
            if (if_stmt->else_branch && if_stmt->else_count > 0) {
                formatter_append(fmt, " else {");
                formatter_newline(fmt);
                
                formatter_increase_indent(fmt);
                for (size_t i = 0; i < if_stmt->else_count; i++) {
                    format_stmt(fmt, if_stmt->else_branch[i]);
                }
                formatter_decrease_indent(fmt);
                
                formatter_indent(fmt);
                formatter_append(fmt, "}");
            }
            formatter_newline(fmt);
            break;
        }
        
        case STMT_WHILE: {
            WhileStmt* while_stmt = &stmt->as.while_stmt;
            formatter_indent(fmt);
            formatter_append(fmt, "while (");
            format_expr(fmt, while_stmt->condition);
            formatter_append(fmt, ") {");
            formatter_newline(fmt);
            
            formatter_increase_indent(fmt);
            for (size_t i = 0; i < while_stmt->body_count; i++) {
                format_stmt(fmt, while_stmt->body[i]);
            }
            formatter_decrease_indent(fmt);
            
            formatter_indent(fmt);
            formatter_append(fmt, "}");
            formatter_newline(fmt);
            break;
        }
        
        case STMT_FOR: {
            ForStmt* for_stmt = &stmt->as.for_stmt;
            formatter_indent(fmt);
            formatter_append(fmt, "for (");
            if (for_stmt->init) {
                format_stmt(fmt, for_stmt->init);
                // Remove the newline and semicolon added by format_stmt
                if (fmt->length > 0 && fmt->output[fmt->length - 1] == '\n') {
                    fmt->length--;
                    fmt->output[fmt->length] = '\0';
                }
                if (fmt->length > 0 && fmt->output[fmt->length - 1] == ';') {
                    fmt->length--;
                    fmt->output[fmt->length] = '\0';
                }
            }
            formatter_append(fmt, "; ");
            if (for_stmt->condition) {
                format_expr(fmt, for_stmt->condition);
            }
            formatter_append(fmt, "; ");
            if (for_stmt->increment) {
                format_expr(fmt, for_stmt->increment);
            }
            formatter_append(fmt, ") {");
            formatter_newline(fmt);
            
            formatter_increase_indent(fmt);
            for (size_t i = 0; i < for_stmt->body_count; i++) {
                format_stmt(fmt, for_stmt->body[i]);
            }
            formatter_decrease_indent(fmt);
            
            formatter_indent(fmt);
            formatter_append(fmt, "}");
            formatter_newline(fmt);
            break;
        }
        
        case STMT_BLOCK: {
            BlockStmt* block = &stmt->as.block;
            formatter_indent(fmt);
            formatter_append(fmt, "{");
            formatter_newline(fmt);
            
            formatter_increase_indent(fmt);
            for (size_t i = 0; i < block->count; i++) {
                format_stmt(fmt, block->statements[i]);
            }
            formatter_decrease_indent(fmt);
            
            formatter_indent(fmt);
            formatter_append(fmt, "}");
            formatter_newline(fmt);
            break;
        }
        
        case STMT_PRINT:
            formatter_indent(fmt);
            formatter_append(fmt, "print(");
            format_expr(fmt, stmt->as.print_stmt.expression);
            formatter_append(fmt, ");");
            formatter_newline(fmt);
            break;
            
        case STMT_IMPORT:
            formatter_indent(fmt);
            formatter_append(fmt, "import ");
            formatter_append(fmt, stmt->as.import_stmt.spec);
            formatter_append(fmt, ";");
            formatter_newline(fmt);
            break;
    }
}

char* format_rubolt_code(const char* source) {
    Lexer lexer;
    lexer_init(&lexer, source);
    
    Parser parser;
    parser_init(&parser, &lexer);
    
    Stmt** statements = NULL;
    size_t statement_count = 0;
    
    // Parse all statements
    while (!parser_is_at_end(&parser)) {
        Stmt* stmt = parse_statement(&parser);
        if (stmt) {
            statements = realloc(statements, (statement_count + 1) * sizeof(Stmt*));
            statements[statement_count++] = stmt;
        }
    }
    
    // Format statements
    Formatter formatter;
    formatter_init(&formatter);
    
    for (size_t i = 0; i < statement_count; i++) {
        format_stmt(&formatter, statements[i]);
        if (i < statement_count - 1) {
            formatter_newline(&formatter);
        }
    }
    
    char* result = strdup(formatter.output);
    
    // Cleanup
    formatter_free(&formatter);
    for (size_t i = 0; i < statement_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);
    
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file.rbo> [--output <output_file>]\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = NULL;
    
    // Parse command line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
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
    
    // Format code
    char* formatted = format_rubolt_code(source);
    
    // Write output
    if (output_file) {
        FILE* out = fopen(output_file, "w");
        if (out) {
            fputs(formatted, out);
            fclose(out);
            printf("Formatted code written to '%s'\n", output_file);
        } else {
            printf("Error: Cannot write to file '%s'\n", output_file);
            free(source);
            free(formatted);
            return 1;
        }
    } else {
        printf("%s", formatted);
    }
    
    free(source);
    free(formatted);
    return 0;
}