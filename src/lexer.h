#ifndef RUBOLT_LEXER_H
#define RUBOLT_LEXER_H

#include <stddef.h>

typedef enum {
    // Literals
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    
    // Keywords
    TOKEN_LET,
    TOKEN_CONST,
    TOKEN_VAR,
    TOKEN_DEF,
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ELIF,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_CLASS,
    TOKEN_IMPORT,
    TOKEN_FROM,
    TOKEN_AS,
    TOKEN_PASS,
    TOKEN_PRINT,
    TOKEN_PRINTF,
    
    // Types
    TOKEN_STRING_TYPE,
    TOKEN_NUMBER_TYPE,
    TOKEN_BOOL_TYPE,
    TOKEN_VOID_TYPE,
    TOKEN_ANY_TYPE,
    
    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_AMPERSAND_AMPERSAND,
    TOKEN_PIPE_PIPE,
    TOKEN_BANG,
    TOKEN_ARROW,
    
    // Punctuation
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_NEWLINE,
    
    // Special
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    int line;
    int column;
} Token;

typedef struct {
    const char* source;
    const char* start;
    const char* current;
    int line;
    int column;
} Lexer;

void lexer_init(Lexer* lexer, const char* source);
Token lexer_next_token(Lexer* lexer);
const char* token_type_to_string(TokenType type);

#endif
