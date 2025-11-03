#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void error_at(Parser* parser, Token* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    parser->had_error = true;
    
    fprintf(stderr, "[line %d] Error", token->line);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;
    
    for (;;) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;
        error_at(parser, &parser->current, parser->current.start);
    }
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static void consume(Parser* parser, TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }
    error_at(parser, &parser->current, message);
}

static void skip_newlines(Parser* parser) {
    while (match(parser, TOKEN_NEWLINE) || match(parser, TOKEN_SEMICOLON)) {}
}

static Expr* parse_primary(Parser* parser);
static Expr* parse_expression(Parser* parser);
static Stmt* parse_statement(Parser* parser);
static Stmt* parse_declaration(Parser* parser);

static Expr* parse_primary(Parser* parser) {
    if (match(parser, TOKEN_TRUE)) {
        return expr_bool(true);
    }
    
    if (match(parser, TOKEN_FALSE)) {
        return expr_bool(false);
    }
    
    if (match(parser, TOKEN_NULL)) {
        return expr_null();
    }
    
    if (match(parser, TOKEN_NUMBER)) {
        char* str = strndup(parser->previous.start, parser->previous.length);
        double value = atof(str);
        free(str);
        return expr_number(value);
    }
    
    if (match(parser, TOKEN_STRING)) {
        char* str = strndup(parser->previous.start, parser->previous.length);
        Expr* expr = expr_string(str);
        free(str);
        return expr;
    }
    
    if (match(parser, TOKEN_IDENTIFIER)) {
        char* name = strndup(parser->previous.start, parser->previous.length);
        Expr* expr = expr_identifier(name);
        free(name);
        return expr;
    }
    
    if (match(parser, TOKEN_LPAREN)) {
        Expr* expr = parse_expression(parser);
        consume(parser, TOKEN_RPAREN, "Expect ')' after expression.");
        return expr;
    }
    
    error_at(parser, &parser->current, "Expect expression.");
    return expr_null();
}

static Expr* parse_call(Parser* parser) {
    Expr* expr = parse_primary(parser);
    
    while (true) {
        if (match(parser, TOKEN_LPAREN)) {
            Expr** args = NULL;
            size_t arg_count = 0;
            
            if (!check(parser, TOKEN_RPAREN)) {
                do {
                    args = realloc(args, sizeof(Expr*) * (arg_count + 1));
                    args[arg_count++] = parse_expression(parser);
                } while (match(parser, TOKEN_COMMA));
            }
            
            consume(parser, TOKEN_RPAREN, "Expect ')' after arguments.");
            expr = expr_call(expr, args, arg_count);
        } else {
            break;
        }
    }
    
    return expr;
}

static Expr* parse_unary(Parser* parser) {
    if (match(parser, TOKEN_BANG) || match(parser, TOKEN_MINUS) || match(parser, TOKEN_NOT)) {
        char* op = strndup(parser->previous.start, parser->previous.length);
        Expr* operand = parse_unary(parser);
        Expr* expr = expr_unary(op, operand);
        free(op);
        return expr;
    }
    
    return parse_call(parser);
}

static Expr* parse_factor(Parser* parser) {
    Expr* expr = parse_unary(parser);
    
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
        char* op = strndup(parser->previous.start, parser->previous.length);
        Expr* right = parse_unary(parser);
        expr = expr_binary(op, expr, right);
        free(op);
    }
    
    return expr;
}

static Expr* parse_term(Parser* parser) {
    Expr* expr = parse_factor(parser);
    
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        char* op = strndup(parser->previous.start, parser->previous.length);
        Expr* right = parse_factor(parser);
        expr = expr_binary(op, expr, right);
        free(op);
    }
    
    return expr;
}

static Expr* parse_comparison(Parser* parser) {
    Expr* expr = parse_term(parser);
    
    while (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) ||
           match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL)) {
        char* op = strndup(parser->previous.start, parser->previous.length);
        Expr* right = parse_term(parser);
        expr = expr_binary(op, expr, right);
        free(op);
    }
    
    return expr;
}

static Expr* parse_equality(Parser* parser) {
    Expr* expr = parse_comparison(parser);
    
    while (match(parser, TOKEN_EQUAL_EQUAL) || match(parser, TOKEN_BANG_EQUAL)) {
        char* op = strndup(parser->previous.start, parser->previous.length);
        Expr* right = parse_comparison(parser);
        expr = expr_binary(op, expr, right);
        free(op);
    }
    
    return expr;
}

static Expr* parse_logical_and(Parser* parser) {
    Expr* expr = parse_equality(parser);
    
    while (match(parser, TOKEN_AMPERSAND_AMPERSAND) || match(parser, TOKEN_AND)) {
        char* op = match(parser, TOKEN_AND) ? "and" : "&&";
        Expr* right = parse_equality(parser);
        expr = expr_binary(op, expr, right);
    }
    
    return expr;
}

static Expr* parse_logical_or(Parser* parser) {
    Expr* expr = parse_logical_and(parser);
    
    while (match(parser, TOKEN_PIPE_PIPE) || match(parser, TOKEN_OR)) {
        char* op = match(parser, TOKEN_OR) ? "or" : "||";
        Expr* right = parse_logical_and(parser);
        expr = expr_binary(op, expr, right);
    }
    
    return expr;
}

static Expr* parse_assignment(Parser* parser) {
    Expr* expr = parse_logical_or(parser);
    
    if (match(parser, TOKEN_EQUAL)) {
        if (expr->type == EXPR_IDENTIFIER) {
            char* name = expr->as.identifier;
            Expr* value = parse_assignment(parser);
            expr->as.identifier = NULL;
            expr_free(expr);
            return expr_assign(name, value);
        }
        error_at(parser, &parser->previous, "Invalid assignment target.");
    }
    
    return expr;
}

static Expr* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

static Stmt* parse_print_stmt(Parser* parser) {
    advance(parser);
    
    if (match(parser, TOKEN_LPAREN)) {
        Expr** args = NULL;
        size_t arg_count = 0;
        
        if (!check(parser, TOKEN_RPAREN)) {
            do {
                args = realloc(args, sizeof(Expr*) * (arg_count + 1));
                args[arg_count++] = parse_expression(parser);
            } while (match(parser, TOKEN_COMMA));
        }
        
        consume(parser, TOKEN_RPAREN, "Expect ')' after print arguments.");
        
        Expr* call = expr_call(expr_identifier("print"), args, arg_count);
        return stmt_expression(call);
    }
    
    Expr* expr = parse_expression(parser);
    skip_newlines(parser);
    return stmt_print(expr);
}

static Stmt* parse_return_stmt(Parser* parser) {
    advance(parser);
    
    Expr* value = NULL;
    if (!check(parser, TOKEN_NEWLINE) && !check(parser, TOKEN_SEMICOLON) && !check(parser, TOKEN_RBRACE)) {
        value = parse_expression(parser);
    }
    
    skip_newlines(parser);
    return stmt_return(value);
}

static Stmt* parse_if_stmt(Parser* parser) {
    advance(parser);
    consume(parser, TOKEN_LPAREN, "Expect '(' after 'if'.");
    Expr* condition = parse_expression(parser);
    consume(parser, TOKEN_RPAREN, "Expect ')' after condition.");
    skip_newlines(parser);
    
    Stmt** then_branch = NULL;
    size_t then_count = 0;
    
    if (match(parser, TOKEN_LBRACE)) {
        skip_newlines(parser);
        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            then_branch = realloc(then_branch, sizeof(Stmt*) * (then_count + 1));
            then_branch[then_count++] = parse_declaration(parser);
            skip_newlines(parser);
        }
        consume(parser, TOKEN_RBRACE, "Expect '}' after block.");
    } else if (check(parser, TOKEN_COLON)) {
        advance(parser);
        skip_newlines(parser);
        then_branch = realloc(then_branch, sizeof(Stmt*) * (then_count + 1));
        then_branch[then_count++] = parse_statement(parser);
    } else {
        then_branch = realloc(then_branch, sizeof(Stmt*) * (then_count + 1));
        then_branch[then_count++] = parse_statement(parser);
    }
    
    Stmt** else_branch = NULL;
    size_t else_count = 0;
    
    if (match(parser, TOKEN_ELSE)) {
        skip_newlines(parser);
        if (match(parser, TOKEN_LBRACE)) {
            skip_newlines(parser);
            while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
                else_branch = realloc(else_branch, sizeof(Stmt*) * (else_count + 1));
                else_branch[else_count++] = parse_declaration(parser);
                skip_newlines(parser);
            }
            consume(parser, TOKEN_RBRACE, "Expect '}' after block.");
        } else if (check(parser, TOKEN_COLON)) {
            advance(parser);
            skip_newlines(parser);
            else_branch = realloc(else_branch, sizeof(Stmt*) * (else_count + 1));
            else_branch[else_count++] = parse_statement(parser);
        } else {
            else_branch = realloc(else_branch, sizeof(Stmt*) * (else_count + 1));
            else_branch[else_count++] = parse_statement(parser);
        }
    }
    
    return stmt_if(condition, then_branch, then_count, else_branch, else_count);
}

static Stmt* parse_while_stmt(Parser* parser) {
    advance(parser);
    consume(parser, TOKEN_LPAREN, "Expect '(' after 'while'.");
    Expr* condition = parse_expression(parser);
    consume(parser, TOKEN_RPAREN, "Expect ')' after condition.");
    skip_newlines(parser);
    
    Stmt** body = NULL;
    size_t body_count = 0;
    
    if (match(parser, TOKEN_LBRACE)) {
        skip_newlines(parser);
        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            body = realloc(body, sizeof(Stmt*) * (body_count + 1));
            body[body_count++] = parse_declaration(parser);
            skip_newlines(parser);
        }
        consume(parser, TOKEN_RBRACE, "Expect '}' after block.");
    } else {
        body = realloc(body, sizeof(Stmt*) * (body_count + 1));
        body[body_count++] = parse_statement(parser);
    }
    
    return stmt_while(condition, body, body_count);
}

static Stmt* parse_for_stmt(Parser* parser) {
    advance(parser);
    consume(parser, TOKEN_LPAREN, "Expect '(' after 'for'.");
    
    Stmt* init = NULL;
    if (!match(parser, TOKEN_SEMICOLON)) {
        if (match(parser, TOKEN_LET) || match(parser, TOKEN_VAR)) {
            init = parse_declaration(parser);
        } else {
            init = stmt_expression(parse_expression(parser));
            match(parser, TOKEN_SEMICOLON);
        }
    }
    
    Expr* condition = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        condition = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after loop condition.");
    
    Expr* increment = NULL;
    if (!check(parser, TOKEN_RPAREN)) {
        increment = parse_expression(parser);
    }
    consume(parser, TOKEN_RPAREN, "Expect ')' after for clauses.");
    skip_newlines(parser);
    
    Stmt** body = NULL;
    size_t body_count = 0;
    
    if (match(parser, TOKEN_LBRACE)) {
        skip_newlines(parser);
        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            body = realloc(body, sizeof(Stmt*) * (body_count + 1));
            body[body_count++] = parse_declaration(parser);
            skip_newlines(parser);
        }
        consume(parser, TOKEN_RBRACE, "Expect '}' after block.");
    } else {
        body = realloc(body, sizeof(Stmt*) * (body_count + 1));
        body[body_count++] = parse_statement(parser);
    }
    
    return stmt_for(init, condition, increment, body, body_count);
}

static Stmt* parse_statement(Parser* parser) {
    if (check(parser, TOKEN_PRINT) || check(parser, TOKEN_PRINTF)) {
        return parse_print_stmt(parser);
    }
    if (check(parser, TOKEN_RETURN)) {
        return parse_return_stmt(parser);
    }
    if (check(parser, TOKEN_IF)) {
        return parse_if_stmt(parser);
    }
    if (check(parser, TOKEN_WHILE)) {
        return parse_while_stmt(parser);
    }
    if (check(parser, TOKEN_FOR)) {
        return parse_for_stmt(parser);
    }
    
    Expr* expr = parse_expression(parser);
    skip_newlines(parser);
    return stmt_expression(expr);
}

static Stmt* parse_var_declaration(Parser* parser, bool is_const) {
    advance(parser);
    
    consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");
    char* name = strndup(parser->previous.start, parser->previous.length);
    
    char* type_name = NULL;
    if (match(parser, TOKEN_COLON)) {
        consume(parser, TOKEN_IDENTIFIER, "Expect type name.");
        type_name = strndup(parser->previous.start, parser->previous.length);
    }
    
    Expr* initializer = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        initializer = parse_expression(parser);
    }
    
    skip_newlines(parser);
    Stmt* stmt = stmt_var_decl(name, type_name, is_const, initializer);
    free(name);
    if (type_name) free(type_name);
    return stmt;
}

static Stmt* parse_function_declaration(Parser* parser) {
    advance(parser);
    
    consume(parser, TOKEN_IDENTIFIER, "Expect function name.");
    char* name = strndup(parser->previous.start, parser->previous.length);
    
    consume(parser, TOKEN_LPAREN, "Expect '(' after function name.");
    
    char** params = NULL;
    char** param_types = NULL;
    size_t param_count = 0;
    
    if (!check(parser, TOKEN_RPAREN)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expect parameter name.");
            params = realloc(params, sizeof(char*) * (param_count + 1));
            param_types = realloc(param_types, sizeof(char*) * (param_count + 1));
            params[param_count] = strndup(parser->previous.start, parser->previous.length);
            param_types[param_count] = NULL;
            
            if (match(parser, TOKEN_COLON)) {
                consume(parser, TOKEN_IDENTIFIER, "Expect type name.");
                param_types[param_count] = strndup(parser->previous.start, parser->previous.length);
            }
            
            param_count++;
        } while (match(parser, TOKEN_COMMA));
    }
    
    consume(parser, TOKEN_RPAREN, "Expect ')' after parameters.");
    
    char* return_type = NULL;
    if (match(parser, TOKEN_ARROW) || match(parser, TOKEN_COLON)) {
        consume(parser, TOKEN_IDENTIFIER, "Expect return type.");
        return_type = strndup(parser->previous.start, parser->previous.length);
    }
    
    skip_newlines(parser);
    
    Stmt** body = NULL;
    size_t body_count = 0;
    
    if (match(parser, TOKEN_LBRACE)) {
        skip_newlines(parser);
        while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
            body = realloc(body, sizeof(Stmt*) * (body_count + 1));
            body[body_count++] = parse_declaration(parser);
            skip_newlines(parser);
        }
        consume(parser, TOKEN_RBRACE, "Expect '}' after block.");
    } else if (match(parser, TOKEN_COLON)) {
        skip_newlines(parser);
        while (!check(parser, TOKEN_EOF) && !check(parser, TOKEN_DEF) && 
               !check(parser, TOKEN_FUNCTION)) {
            body = realloc(body, sizeof(Stmt*) * (body_count + 1));
            body[body_count++] = parse_declaration(parser);
            skip_newlines(parser);
            if (check(parser, TOKEN_EOF)) break;
        }
    }
    
    Stmt* stmt = stmt_function(name, params, param_types, param_count, return_type, body, body_count);
    free(name);
    if (return_type) free(return_type);
    return stmt;
}

static Stmt* parse_declaration(Parser* parser) {
    if (check(parser, TOKEN_LET) || check(parser, TOKEN_VAR)) {
        return parse_var_declaration(parser, false);
    }
    if (check(parser, TOKEN_CONST)) {
        return parse_var_declaration(parser, true);
    }
    if (check(parser, TOKEN_DEF) || check(parser, TOKEN_FUNCTION)) {
        return parse_function_declaration(parser);
    }
    
    return parse_statement(parser);
}

void parser_init(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->had_error = false;
    parser->panic_mode = false;
    advance(parser);
}

Stmt** parse(Parser* parser, size_t* count) {
    Stmt** statements = NULL;
    *count = 0;
    
    skip_newlines(parser);
    
    while (!match(parser, TOKEN_EOF)) {
        statements = realloc(statements, sizeof(Stmt*) * (*count + 1));
        statements[*count] = parse_declaration(parser);
        (*count)++;
        skip_newlines(parser);
    }
    
    return statements;
}
