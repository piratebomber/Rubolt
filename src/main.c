#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "../runtime/runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

static void run_file_compat(const char* path) {
    int code = runtime_run_file(path);
    if (code != 0) exit(code);
}

static void repl() {
    char line[1024];
    printf("Rubolt v1.0.0 - Interactive REPL\n");
    printf("Type 'exit' to quit.\n\n");

    while (true) {
        printf(">>> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        if (strcmp(line, "exit\n") == 0 || strcmp(line, "quit\n") == 0) {
            break;
        }

        (void)runtime_run_source(line);
    }
}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file_compat(argv[1]);
    } else {
        fprintf(stderr, "Usage: rubolt [path]\n");
        exit(64);
    }

    return 0;
}
