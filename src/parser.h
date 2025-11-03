#ifndef RUBOLT_PARSER_H
#define RUBOLT_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

void parser_init(Parser* parser, Lexer* lexer);
Stmt** parse(Parser* parser, size_t* count);

#endif
