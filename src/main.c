#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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

static void run_file(const char* path) {
    char* source = read_file(path);
    if (source == NULL) {
        exit(74);
    }
    
    Lexer lexer;
    lexer_init(&lexer, source);
    
    Parser parser;
    parser_init(&parser, &lexer);
    
    size_t stmt_count;
    Stmt** statements = parse(&parser, &stmt_count);
    
    if (parser.had_error) {
        free(source);
        exit(65);
    }
    
    Interpreter* interp = interpreter_create();
    Value result = interpret(interp, statements, stmt_count);
    
    // Cleanup
    for (size_t i = 0; i < stmt_count; i++) {
        stmt_free(statements[i]);
    }
    free(statements);
    interpreter_cleanup(interp);
    free(source);
}

static void repl() {
    char line[1024];
    printf("Rubolt v1.0.0 Enhanced - Interactive REPL\n");
    printf("Features: Nested Functions, Enhanced Loops, Closures\n");
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

        Lexer lexer;
        lexer_init(&lexer, line);
        
        Parser parser;
        parser_init(&parser, &lexer);
        
        size_t stmt_count;
        Stmt** statements = parse(&parser, &stmt_count);
        
        if (!parser.had_error && stmt_count > 0) {
            Interpreter* interp = interpreter_create();
            Value result = interpret(interp, statements, stmt_count);
            
            // Print result if it's not null
            if (result.type != VALUE_NULL) {
                value_print(result);
                printf("\n");
            }
            
            // Cleanup
            for (size_t i = 0; i < stmt_count; i++) {
                stmt_free(statements[i]);
            }
            free(statements);
            interpreter_cleanup(interp);
        }
    }
}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: rubolt [path]\n");
        exit(64);
    }

    return 0;
}
