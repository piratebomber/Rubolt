#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

void lexer_init(Lexer* lexer, const char* source) {
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
}

static bool is_at_end(Lexer* lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer* lexer) {
    lexer->current++;
    lexer->column++;
    return lexer->current[-1];
}

static char peek(Lexer* lexer) {
    return *lexer->current;
}

static char peek_next(Lexer* lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static bool match(Lexer* lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    lexer->column++;
    return true;
}

static Token make_token(Lexer* lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (size_t)(lexer->current - lexer->start);
    token.line = lexer->line;
    token.column = lexer->column - token.length;
    return token;
}

static Token error_token(Lexer* lexer, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static void skip_whitespace(Lexer* lexer) {
    for (;;) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                } else if (peek_next(lexer) == '*') {
                    advance(lexer);
                    advance(lexer);
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer);
                            advance(lexer);
                            break;
                        }
                        if (peek(lexer) == '\n') {
                            lexer->line++;
                            lexer->column = 0;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            case '#':
                while (peek(lexer) != '\n' && !is_at_end(lexer)) advance(lexer);
                break;
            default:
                return;
        }
    }
}

static Token string(Lexer* lexer, char quote) {
    while (peek(lexer) != quote && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            lexer->line++;
            lexer->column = 0;
        }
        if (peek(lexer) == '\\') advance(lexer);
        advance(lexer);
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }

    advance(lexer);
    return make_token(lexer, TOKEN_STRING);
}

static Token number(Lexer* lexer) {
    while (isdigit(peek(lexer))) advance(lexer);

    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer);
        while (isdigit(peek(lexer))) advance(lexer);
    }

    return make_token(lexer, TOKEN_NUMBER);
}

static TokenType check_keyword(Lexer* lexer, int start, int length, const char* rest, TokenType type) {
    if (lexer->current - lexer->start == start + length &&
        memcmp(lexer->start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Lexer* lexer) {
    switch (lexer->start[0]) {
        case 'a':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'n': return check_keyword(lexer, 2, 1, "d", TOKEN_AND);
                    case 's': return check_keyword(lexer, 2, 0, "", TOKEN_AS);
                    case 'n': return check_keyword(lexer, 2, 1, "y", TOKEN_ANY_TYPE);
                }
            }
            break;
        case 'b':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'o': return check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL_TYPE);
                    case 'r': return check_keyword(lexer, 2, 3, "eak", TOKEN_BREAK);
                }
            }
            break;
        case 'c':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'l': return check_keyword(lexer, 2, 3, "ass", TOKEN_CLASS);
                    case 'o':
                        if (lexer->current - lexer->start > 2) {
                            switch (lexer->start[2]) {
                                case 'n':
                                    if (lexer->current - lexer->start > 3) {
                                        switch (lexer->start[3]) {
                                            case 's': return check_keyword(lexer, 4, 1, "t", TOKEN_CONST);
                                            case 't': return check_keyword(lexer, 4, 4, "inue", TOKEN_CONTINUE);
                                        }
                                    }
                            }
                        }
                }
            }
            break;
        case 'd': return check_keyword(lexer, 1, 2, "ef", TOKEN_DEF);
        case 'e':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'l':
                        if (lexer->current - lexer->start > 2) {
                            switch (lexer->start[2]) {
                                case 's': return check_keyword(lexer, 3, 1, "e", TOKEN_ELSE);
                                case 'i': return check_keyword(lexer, 3, 1, "f", TOKEN_ELIF);
                            }
                        }
                }
            }
            break;
        case 'f':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'a': return check_keyword(lexer, 2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
                    case 'r': return check_keyword(lexer, 2, 2, "om", TOKEN_FROM);
                    case 'u': return check_keyword(lexer, 2, 6, "nction", TOKEN_FUNCTION);
                }
            }
            break;
        case 'i':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'f': return check_keyword(lexer, 2, 0, "", TOKEN_IF);
                    case 'm': return check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
                }
            }
            break;
        case 'l': return check_keyword(lexer, 1, 2, "et", TOKEN_LET);
        case 'n':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'o': return check_keyword(lexer, 2, 1, "t", TOKEN_NOT);
                    case 'u':
                        if (lexer->current - lexer->start > 2) {
                            switch (lexer->start[2]) {
                                case 'l': return check_keyword(lexer, 3, 1, "l", TOKEN_NULL);
                                case 'm': return check_keyword(lexer, 3, 3, "ber", TOKEN_NUMBER_TYPE);
                            }
                        }
                }
            }
            break;
        case 'o': return check_keyword(lexer, 1, 1, "r", TOKEN_OR);
        case 'p':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'a': return check_keyword(lexer, 2, 2, "ss", TOKEN_PASS);
                    case 'r': return check_keyword(lexer, 2, 3, "int", TOKEN_PRINT);
                }
            }
            if (lexer->current - lexer->start == 6) {
                return check_keyword(lexer, 1, 5, "rintf", TOKEN_PRINTF);
            }
            break;
        case 'r': return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
        case 's': return check_keyword(lexer, 1, 5, "tring", TOKEN_STRING_TYPE);
        case 't': return check_keyword(lexer, 1, 3, "rue", TOKEN_TRUE);
        case 'v':
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'a': return check_keyword(lexer, 2, 1, "r", TOKEN_VAR);
                    case 'o': return check_keyword(lexer, 2, 2, "id", TOKEN_VOID_TYPE);
                }
            }
            break;
        case 'w': return check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier(Lexer* lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') advance(lexer);
    return make_token(lexer, identifier_type(lexer));
}

Token lexer_next_token(Lexer* lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_at_end(lexer)) return make_token(lexer, TOKEN_EOF);

    char c = advance(lexer);

    if (isalpha(c) || c == '_') return identifier(lexer);
    if (isdigit(c)) return number(lexer);

    switch (c) {
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '.': return make_token(lexer, TOKEN_DOT);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ':': return make_token(lexer, TOKEN_COLON);
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '*': return make_token(lexer, TOKEN_STAR);
        case '%': return make_token(lexer, TOKEN_PERCENT);
        case '\n':
            lexer->line++;
            lexer->column = 1;
            return make_token(lexer, TOKEN_NEWLINE);
        case '-':
            if (match(lexer, '>')) return make_token(lexer, TOKEN_ARROW);
            return make_token(lexer, TOKEN_MINUS);
        case '/':
            return make_token(lexer, TOKEN_SLASH);
        case '!':
            return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return make_token(lexer, match(lexer, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return make_token(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return make_token(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '&':
            if (match(lexer, '&')) return make_token(lexer, TOKEN_AMPERSAND_AMPERSAND);
            break;
        case '|':
            if (match(lexer, '|')) return make_token(lexer, TOKEN_PIPE_PIPE);
            break;
        case '"':
        case '\'':
            return string(lexer, c);
    }

    return error_token(lexer, "Unexpected character");
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_LET: return "LET";
        case TOKEN_CONST: return "CONST";
        case TOKEN_DEF: return "DEF";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_NULL: return "NULL";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
