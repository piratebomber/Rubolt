#include "compiler.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

// Very simple "compiler": writes token types as bytes into output file
// This is a placeholder for a real machine code generator.

int compile_file_to_bin(const char* in_path, const char* out_path) {
    FILE* in = fopen(in_path, "rb");
    if (!in) {
        perror("open input");
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long size = ftell(in);
    rewind(in);
    char* src = (char*)malloc((size_t)size + 1);
    fread(src, 1, (size_t)size, in);
    src[size] = '\0';
    fclose(in);

    FILE* out = fopen(out_path, "wb");
    if (!out) {
        perror("open output");
        free(src);
        return 1;
    }

    Lexer lex;
    lexer_init(&lex, src);

    for (;;) {
        Token t = lexer_next_token(&lex);
        unsigned char byte = (unsigned char)t.type;
        fwrite(&byte, 1, 1, out);
        if (t.type == TOKEN_EOF || t.type == TOKEN_ERROR) break;
    }

    fclose(out);
    free(src);
    return 0;
}
