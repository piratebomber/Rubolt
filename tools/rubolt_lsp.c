#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include "../src/lexer.h"
#include "../src/parser.h"
#include "../src/ast.h"

typedef struct {
    int line;
    int character;
} Position;

typedef struct {
    Position start;
    Position end;
} Range;

typedef struct {
    Range range;
    char* message;
    int severity; // 1=Error, 2=Warning, 3=Info, 4=Hint
} Diagnostic;

typedef struct {
    char* label;
    int kind; // CompletionItemKind
    char* detail;
    char* documentation;
} CompletionItem;

static int request_id = 0;

void send_response(json_object* result, int id) {
    json_object* response = json_object_new_object();
    json_object* jsonrpc = json_object_new_string("2.0");
    json_object* response_id = json_object_new_int(id);
    
    json_object_object_add(response, "jsonrpc", jsonrpc);
    json_object_object_add(response, "id", response_id);
    json_object_object_add(response, "result", result);
    
    const char* response_str = json_object_to_json_string(response);
    printf("Content-Length: %zu\r\n\r\n%s", strlen(response_str), response_str);
    fflush(stdout);
    
    json_object_put(response);
}

void send_notification(const char* method, json_object* params) {
    json_object* notification = json_object_new_object();
    json_object* jsonrpc = json_object_new_string("2.0");
    json_object* method_obj = json_object_new_string(method);
    
    json_object_object_add(notification, "jsonrpc", jsonrpc);
    json_object_object_add(notification, "method", method_obj);
    if (params) {
        json_object_object_add(notification, "params", params);
    }
    
    const char* notification_str = json_object_to_json_string(notification);
    printf("Content-Length: %zu\r\n\r\n%s", strlen(notification_str), notification_str);
    fflush(stdout);
    
    json_object_put(notification);
}

Diagnostic* validate_document(const char* text, int* diagnostic_count) {
    *diagnostic_count = 0;
    Diagnostic* diagnostics = malloc(sizeof(Diagnostic) * 100);
    
    Lexer lexer;
    lexer_init(&lexer, text);
    
    Token token;
    int line = 0;
    int prev_token_type = TOKEN_EOF;
    
    while ((token = lexer_next_token(&lexer)).type != TOKEN_EOF) {
        // Check for missing semicolons
        if (prev_token_type == TOKEN_IDENTIFIER || 
            prev_token_type == TOKEN_NUMBER || 
            prev_token_type == TOKEN_STRING) {
            
            if (token.type == TOKEN_NEWLINE || token.type == TOKEN_EOF) {
                // Look for statements that should end with semicolon
                diagnostics[*diagnostic_count] = (Diagnostic){
                    .range = {{token.line, 0}, {token.line, token.column}},
                    .message = strdup("Missing semicolon"),
                    .severity = 1 // Error
                };
                (*diagnostic_count)++;
            }
        }
        
        // Check for undefined variables (basic check)
        if (token.type == TOKEN_IDENTIFIER) {
            // This is a simplified check - in a real implementation,
            // you'd maintain a symbol table
            if (strcmp(token.lexeme, "undefined_var") == 0) {
                diagnostics[*diagnostic_count] = (Diagnostic){
                    .range = {{token.line, token.column}, {token.line, token.column + strlen(token.lexeme)}},
                    .message = strdup("Undefined variable"),
                    .severity = 1 // Error
                };
                (*diagnostic_count)++;
            }
        }
        
        prev_token_type = token.type;
    }
    
    return diagnostics;
}

CompletionItem* get_completions(const char* text, Position pos, int* completion_count) {
    *completion_count = 0;
    CompletionItem* completions = malloc(sizeof(CompletionItem) * 50);
    
    // Keywords
    const char* keywords[] = {
        "def", "class", "if", "else", "for", "while", "return", 
        "import", "let", "const", "true", "false", "null"
    };
    
    for (int i = 0; i < 13; i++) {
        completions[*completion_count] = (CompletionItem){
            .label = strdup(keywords[i]),
            .kind = 14, // Keyword
            .detail = strdup("Rubolt keyword"),
            .documentation = strdup("Language keyword")
        };
        (*completion_count)++;
    }
    
    // Built-in functions
    const char* builtins[] = {"print", "len", "type", "str", "int", "float"};
    const char* builtin_docs[] = {
        "Print value to console",
        "Get length of collection",
        "Get type of value",
        "Convert to string",
        "Convert to integer", 
        "Convert to float"
    };
    
    for (int i = 0; i < 6; i++) {
        completions[*completion_count] = (CompletionItem){
            .label = strdup(builtins[i]),
            .kind = 3, // Function
            .detail = strdup("Built-in function"),
            .documentation = strdup(builtin_docs[i])
        };
        (*completion_count)++;
    }
    
    // Standard library modules
    const char* modules[] = {"file", "json", "time", "http", "string"};
    for (int i = 0; i < 5; i++) {
        completions[*completion_count] = (CompletionItem){
            .label = strdup(modules[i]),
            .kind = 9, // Module
            .detail = strdup("Standard library module"),
            .documentation = strdup("Rubolt standard library module")
        };
        (*completion_count)++;
    }
    
    return completions;
}

void handle_initialize(json_object* params, int id) {
    json_object* result = json_object_new_object();
    json_object* capabilities = json_object_new_object();
    
    // Text document sync
    json_object* text_sync = json_object_new_int(1); // Incremental
    json_object_object_add(capabilities, "textDocumentSync", text_sync);
    
    // Completion provider
    json_object* completion = json_object_new_object();
    json_object* resolve_provider = json_object_new_boolean(1);
    json_object* trigger_chars = json_object_new_array();
    json_object_array_add(trigger_chars, json_object_new_string("."));
    json_object_array_add(trigger_chars, json_object_new_string("("));
    json_object_object_add(completion, "resolveProvider", resolve_provider);
    json_object_object_add(completion, "triggerCharacters", trigger_chars);
    json_object_object_add(capabilities, "completionProvider", completion);
    
    // Hover provider
    json_object* hover = json_object_new_boolean(1);
    json_object_object_add(capabilities, "hoverProvider", hover);
    
    // Diagnostic provider
    json_object* diagnostic = json_object_new_object();
    json_object* inter_file = json_object_new_boolean(0);
    json_object* workspace_diag = json_object_new_boolean(0);
    json_object_object_add(diagnostic, "interFileDependencies", inter_file);
    json_object_object_add(diagnostic, "workspaceDiagnostics", workspace_diag);
    json_object_object_add(capabilities, "diagnosticProvider", diagnostic);
    
    json_object_object_add(result, "capabilities", capabilities);
    send_response(result, id);
}

void handle_completion(json_object* params, int id) {
    json_object* text_doc;
    json_object* position;
    json_object* uri;
    
    if (!json_object_object_get_ex(params, "textDocument", &text_doc) ||
        !json_object_object_get_ex(params, "position", &position) ||
        !json_object_object_get_ex(text_doc, "uri", &uri)) {
        send_response(json_object_new_array(), id);
        return;
    }
    
    Position pos = {0};
    json_object* line_obj, *char_obj;
    if (json_object_object_get_ex(position, "line", &line_obj) &&
        json_object_object_get_ex(position, "character", &char_obj)) {
        pos.line = json_object_get_int(line_obj);
        pos.character = json_object_get_int(char_obj);
    }
    
    int completion_count;
    CompletionItem* completions = get_completions("", pos, &completion_count);
    
    json_object* result = json_object_new_array();
    for (int i = 0; i < completion_count; i++) {
        json_object* item = json_object_new_object();
        json_object_object_add(item, "label", json_object_new_string(completions[i].label));
        json_object_object_add(item, "kind", json_object_new_int(completions[i].kind));
        json_object_object_add(item, "detail", json_object_new_string(completions[i].detail));
        json_object_object_add(item, "documentation", json_object_new_string(completions[i].documentation));
        json_object_array_add(result, item);
    }
    
    send_response(result, id);
    
    // Cleanup
    for (int i = 0; i < completion_count; i++) {
        free(completions[i].label);
        free(completions[i].detail);
        free(completions[i].documentation);
    }
    free(completions);
}

void handle_diagnostic(json_object* params, int id) {
    json_object* text_doc;
    json_object* uri;
    
    if (!json_object_object_get_ex(params, "textDocument", &text_doc) ||
        !json_object_object_get_ex(text_doc, "uri", &uri)) {
        send_response(json_object_new_object(), id);
        return;
    }
    
    // In a real implementation, you'd read the file content
    const char* dummy_text = "def test() { print(\"hello\") }";
    
    int diagnostic_count;
    Diagnostic* diagnostics = validate_document(dummy_text, &diagnostic_count);
    
    json_object* result = json_object_new_object();
    json_object* kind = json_object_new_string("full");
    json_object* items = json_object_new_array();
    
    for (int i = 0; i < diagnostic_count; i++) {
        json_object* diag = json_object_new_object();
        json_object* range = json_object_new_object();
        json_object* start = json_object_new_object();
        json_object* end = json_object_new_object();
        
        json_object_object_add(start, "line", json_object_new_int(diagnostics[i].range.start.line));
        json_object_object_add(start, "character", json_object_new_int(diagnostics[i].range.start.character));
        json_object_object_add(end, "line", json_object_new_int(diagnostics[i].range.end.line));
        json_object_object_add(end, "character", json_object_new_int(diagnostics[i].range.end.character));
        
        json_object_object_add(range, "start", start);
        json_object_object_add(range, "end", end);
        json_object_object_add(diag, "range", range);
        json_object_object_add(diag, "message", json_object_new_string(diagnostics[i].message));
        json_object_object_add(diag, "severity", json_object_new_int(diagnostics[i].severity));
        
        json_object_array_add(items, diag);
    }
    
    json_object_object_add(result, "kind", kind);
    json_object_object_add(result, "items", items);
    
    send_response(result, id);
    
    // Cleanup
    for (int i = 0; i < diagnostic_count; i++) {
        free(diagnostics[i].message);
    }
    free(diagnostics);
}

void process_message(const char* message) {
    json_object* root = json_tokener_parse(message);
    if (!root) return;
    
    json_object* method_obj, *id_obj, *params_obj;
    const char* method = NULL;
    int id = -1;
    
    if (json_object_object_get_ex(root, "method", &method_obj)) {
        method = json_object_get_string(method_obj);
    }
    
    if (json_object_object_get_ex(root, "id", &id_obj)) {
        id = json_object_get_int(id_obj);
    }
    
    json_object_object_get_ex(root, "params", &params_obj);
    
    if (method) {
        if (strcmp(method, "initialize") == 0) {
            handle_initialize(params_obj, id);
        } else if (strcmp(method, "textDocument/completion") == 0) {
            handle_completion(params_obj, id);
        } else if (strcmp(method, "textDocument/diagnostic") == 0) {
            handle_diagnostic(params_obj, id);
        } else if (strcmp(method, "initialized") == 0) {
            // No response needed
        } else if (strcmp(method, "shutdown") == 0) {
            send_response(json_object_new_null(), id);
        } else if (strcmp(method, "exit") == 0) {
            exit(0);
        }
    }
    
    json_object_put(root);
}

int main() {
    char buffer[8192];
    char* content = NULL;
    size_t content_length = 0;
    
    while (fgets(buffer, sizeof(buffer), stdin)) {
        if (strncmp(buffer, "Content-Length:", 15) == 0) {
            content_length = atoi(buffer + 16);
        } else if (strcmp(buffer, "\r\n") == 0 || strcmp(buffer, "\n") == 0) {
            if (content_length > 0) {
                content = malloc(content_length + 1);
                fread(content, 1, content_length, stdin);
                content[content_length] = '\0';
                
                process_message(content);
                
                free(content);
                content_length = 0;
            }
        }
    }
    
    return 0;
}