#include <stdio.h>
#include <stdlib.h>
#include "../../src/lexer.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.rbo>\n", argv[0]);
        return 1;
    }
    const char* path = argv[1];

    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("open");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* src = (char*)malloc((size_t)size + 1);
    fread(src, 1, (size_t)size, f);
    src[size] = '\0';
    fclose(f);

    Lexer lex;
    lexer_init(&lex, src);

    for (;;) {
        Token t = lexer_next_token(&lex);
        printf("%d:%d %-16s '%.*s'\n", t.line, t.column, token_type_to_string(t.type), (int)t.length, t.start);
        if (t.type == TOKEN_EOF || t.type == TOKEN_ERROR) break;
    }

    free(src);
    return 0;
}
