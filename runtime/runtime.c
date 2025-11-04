#include "runtime.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "../src/native_registry.h"
#include "../src/frozen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file_try(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buf = (char*)malloc((size_t)size + 1);
    if (!buf) { fclose(file); return NULL; }
    size_t n = fread(buf, 1, (size_t)size, file);
    buf[n] = '\0';
    fclose(file);
    return buf;
}

static char* join_sources(const char* a, const char* b) {
    size_t la = strlen(a), lb = strlen(b);
    char* out = (char*)malloc(la + lb + 2);
    memcpy(out, a, la);
    out[la] = '\n';
    memcpy(out + la + 1, b, lb + 1);
    return out;
}

static char* load_prelude(void) {
    // Check frozen first
    const char* f = frozen_get("StdLib/prelude.rbo");
    if (!f) f = frozen_get("prelude");
    if (f) {
        size_t l = strlen(f);
        char* buf = (char*)malloc(l + 1);
        memcpy(buf, f, l + 1);
        return buf;
    }
    // Try common locations
    const char* candidates[] = {
        "StdLib/prelude.rbo",
        "../StdLib/prelude.rbo",
        "../../StdLib/prelude.rbo"
    };
    for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); i++) {
        char* s = read_file_try(candidates[i]);
        if (s) return s;
    }
    // Fallback: empty prelude
    char* empty = (char*)malloc(1);
    empty[0] = '\0';
    return empty;
}

int runtime_run_source(const char* source) {
    native_registry_init();
    char* prelude = load_prelude();
    char* merged = join_sources(prelude, source);

    Lexer lexer;
    lexer_init(&lexer, merged);

    Parser parser;
    parser_init(&parser, &lexer);

    size_t stmt_count = 0;
    Stmt** statements = parse(&parser, &stmt_count);
    if (!parser.had_error) {
        interpret(statements, stmt_count);
    }

    for (size_t i = 0; i < stmt_count; i++) stmt_free(statements[i]);
    free(statements);
    free(merged);
    free(prelude);

    return parser.had_error ? 65 : 0;
}

int runtime_run_file(const char* path) {
    char* src = read_file_try(path);
    if (!src) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return 74;
    }
    int code = runtime_run_source(src);
    free(src);
    return code;
}
