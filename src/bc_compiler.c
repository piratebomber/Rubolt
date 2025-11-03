#include "bc_compiler.h"
#include "lexer.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void emit_byte(FILE* out, unsigned char b){ fputc(b, out); }
static void emit_double(FILE* out, double v){ fwrite(&v, 1, 8, out); }

// Parse a very small subset: lines of the form
//   print(<expr>);
// and <expr> ::= NUMBER (('+' NUMBER)*)
// We compile to: [expr code] PRINT ;

static int compile_expr_number_add(Lexer* lex, FILE* out){
    Token t = lexer_next_token(lex);
    if (t.type != TOKEN_NUMBER) return 0;
    double first = atof(strndup(t.start, t.length));
    emit_byte(out, OP_CONST); emit_double(out, first);
    for(;;){
        Token op = lexer_next_token(lex);
        if (op.type != TOKEN_PLUS) { lex->current = op.start; lex->column -= (int)op.length; break; }
        Token n = lexer_next_token(lex);
        if (n.type != TOKEN_NUMBER) return 0;
        double val = atof(strndup(n.start, n.length));
        emit_byte(out, OP_CONST); emit_double(out, val);
        emit_byte(out, OP_ADD);
    }
    return 1;
}

int bc_compile_file(const char* in_path, const char* out_path){
    FILE* in=fopen(in_path,"rb"); if(!in){ perror("open input"); return 1; }
    fseek(in,0,SEEK_END); long sz=ftell(in); rewind(in);
    char* src=(char*)malloc(sz+1); fread(src,1,sz,in); src[sz]='\0'; fclose(in);

    FILE* out=fopen(out_path,"wb"); if(!out){ perror("open out"); free(src); return 1; }

    Lexer lex; lexer_init(&lex, src);
    for(;;){
        Token t = lexer_next_token(&lex);
        if (t.type == TOKEN_EOF) break;
        if (t.type == TOKEN_PRINT){
            Token lp = lexer_next_token(&lex); if (lp.type!=TOKEN_LPAREN){ fclose(out); free(src); return 1; }
            if (!compile_expr_number_add(&lex, out)){ fclose(out); free(src); return 1; }
            Token rp = lexer_next_token(&lex); if (rp.type!=TOKEN_RPAREN){ fclose(out); free(src); return 1; }
            emit_byte(out, OP_PRINT);
            // consume optional ; or newline
            Token end = lexer_next_token(&lex);
            (void)end;
        } else {
            // skip tokens until newline/semicolon
            if (t.type==TOKEN_NEWLINE || t.type==TOKEN_SEMICOLON) continue;
        }
    }
    emit_byte(out, OP_HALT);
    fclose(out);
    free(src);
    return 0;
}
