#include "repl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#define KEY_UP 72
#define KEY_DOWN 80
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_BACKSPACE 8
#define KEY_DELETE 83
#define KEY_TAB 9
#define KEY_ENTER 13
#define KEY_ESC 27
#else
#include <termios.h>
#include <unistd.h>
#define KEY_UP 'A'
#define KEY_DOWN 'B'
#define KEY_LEFT 'D'
#define KEY_RIGHT 'C'
#define KEY_BACKSPACE 127
#define KEY_DELETE '\033'
#define KEY_TAB '\t'
#define KEY_ENTER '\n'
#define KEY_ESC 27
#endif

/* Global command registry */
#define MAX_COMMANDS 50
static ReplCommand g_commands[MAX_COMMANDS];
static size_t g_command_count = 0;

/* REPL keywords for completion */
static const char *g_keywords[] = {
    "let", "const", "var", "def", "function", "if", "else", "elif",
    "for", "while", "return", "break", "continue", "import", "from",
    "class", "try", "except", "finally", "with", "as", "in", "is",
    "and", "or", "not", "true", "false", "null", "void", "number",
    "string", "bool", "any", "print", "printf", NULL
};

/* ========== INITIALIZATION ========== */

void repl_init(ReplState *repl) {
    repl_history_init(&repl->history);
    repl->current_line = (char *)calloc(REPL_MAX_LINE, sizeof(char));
    repl->cursor_pos = 0;
    repl->running = true;
    repl->multiline_mode = false;
    repl->multiline_buffer = NULL;
    repl->multiline_size = 0;
    
    /* Register built-in commands */
    repl_register_command("help", "Show available commands", repl_cmd_help);
    repl_register_command("exit", "Exit the REPL", repl_cmd_exit);
    repl_register_command("quit", "Exit the REPL", repl_cmd_exit);
    repl_register_command("clear", "Clear screen", repl_cmd_clear);
    repl_register_command("history", "Show command history", repl_cmd_history);
    repl_register_command("debug", "Toggle debug mode", repl_cmd_debug);
    repl_register_command("profile", "Show profiling info", repl_cmd_profile);
    repl_register_command("break", "Set breakpoint", repl_cmd_break);
    repl_register_command("step", "Step through code", repl_cmd_step);
    repl_register_command("continue", "Continue execution", repl_cmd_continue);
    repl_register_command("inspect", "Inspect variable", repl_cmd_inspect);
}

void repl_shutdown(ReplState *repl) {
    repl_history_free(&repl->history);
    if (repl->current_line) {
        free(repl->current_line);
        repl->current_line = NULL;
    }
    if (repl->multiline_buffer) {
        free(repl->multiline_buffer);
        repl->multiline_buffer = NULL;
    }
}

/* ========== HISTORY MANAGEMENT ========== */

void repl_history_init(ReplHistory *history) {
    history->capacity = REPL_HISTORY_SIZE;
    history->lines = (char **)calloc(history->capacity, sizeof(char *));
    history->count = 0;
    history->current_index = -1;
}

void repl_history_add(ReplHistory *history, const char *line) {
    if (!line || strlen(line) == 0) return;
    
    /* Don't add duplicate consecutive entries */
    if (history->count > 0 && strcmp(history->lines[history->count - 1], line) == 0) {
        return;
    }
    
    /* Add to history */
    if (history->count < history->capacity) {
        history->lines[history->count] = strdup(line);
        history->count++;
    } else {
        /* History full - remove oldest */
        free(history->lines[0]);
        memmove(history->lines, history->lines + 1, (history->capacity - 1) * sizeof(char *));
        history->lines[history->capacity - 1] = strdup(line);
    }
    
    history->current_index = history->count;
}

const char *repl_history_prev(ReplHistory *history) {
    if (history->count == 0) return NULL;
    
    if (history->current_index > 0) {
        history->current_index--;
    }
    
    return history->lines[history->current_index];
}

const char *repl_history_next(ReplHistory *history) {
    if (history->count == 0) return NULL;
    
    if (history->current_index < (int)history->count - 1) {
        history->current_index++;
        return history->lines[history->current_index];
    }
    
    history->current_index = history->count;
    return "";
}

bool repl_history_save(ReplHistory *history, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return false;
    
    for (size_t i = 0; i < history->count; i++) {
        fprintf(f, "%s\n", history->lines[i]);
    }
    
    fclose(f);
    return true;
}

bool repl_history_load(ReplHistory *history, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return false;
    
    char line[REPL_MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        repl_history_add(history, line);
    }
    
    fclose(f);
    return true;
}

void repl_history_clear(ReplHistory *history) {
    for (size_t i = 0; i < history->count; i++) {
        free(history->lines[i]);
        history->lines[i] = NULL;
    }
    history->count = 0;
    history->current_index = -1;
}

void repl_history_free(ReplHistory *history) {
    repl_history_clear(history);
    free(history->lines);
    history->lines = NULL;
}

/* ========== AUTOCOMPLETION ========== */

ReplCompletion *repl_completion_new(void) {
    ReplCompletion *comp = (ReplCompletion *)malloc(sizeof(ReplCompletion));
    comp->suggestions = (char **)calloc(REPL_MAX_COMPLETIONS, sizeof(char *));
    comp->count = 0;
    comp->current = 0;
    return comp;
}

void repl_get_completions(ReplCompletion *comp, const char *input, size_t cursor_pos) {
    comp->count = 0;
    
    /* Find the word at cursor */
    const char *word_start = input;
    const char *ptr = input;
    
    while (ptr < input + cursor_pos) {
        if (!isalnum(*ptr) && *ptr != '_') {
            word_start = ptr + 1;
        }
        ptr++;
    }
    
    size_t word_len = (input + cursor_pos) - word_start;
    if (word_len == 0) return;
    
    /* Match keywords */
    for (int i = 0; g_keywords[i] != NULL; i++) {
        if (strncmp(g_keywords[i], word_start, word_len) == 0) {
            if (comp->count < REPL_MAX_COMPLETIONS) {
                comp->suggestions[comp->count++] = strdup(g_keywords[i]);
            }
        }
    }
}

void repl_completion_free(ReplCompletion *comp) {
    for (size_t i = 0; i < comp->count; i++) {
        free(comp->suggestions[i]);
    }
    free(comp->suggestions);
    free(comp);
}

/* ========== LINE EDITING ========== */

#ifdef _WIN32
static int getch_special(void) {
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        ch = _getch();
    }
    return ch;
}
#else
static struct termios orig_termios;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static int getch_special(void) {
    int ch = getchar();
    if (ch == '\033') {
        getchar(); // '['
        ch = getchar();
    }
    return ch;
}
#endif

char *repl_readline(ReplState *repl, const char *prompt) {
    printf("%s", prompt);
    fflush(stdout);
    
    memset(repl->current_line, 0, REPL_MAX_LINE);
    repl->cursor_pos = 0;
    
    /* Simple line reading (cross-platform fallback) */
    if (fgets(repl->current_line, REPL_MAX_LINE, stdin) == NULL) {
        return NULL;
    }
    
    /* Remove trailing newline */
    size_t len = strlen(repl->current_line);
    if (len > 0 && repl->current_line[len - 1] == '\n') {
        repl->current_line[len - 1] = '\0';
    }
    
    return repl->current_line;
}

bool repl_handle_special_key(ReplState *repl, int key) {
    switch (key) {
        case KEY_UP: {
            const char *prev = repl_history_prev(&repl->history);
            if (prev) {
                strcpy(repl->current_line, prev);
                repl->cursor_pos = strlen(repl->current_line);
                return true;
            }
            break;
        }
        case KEY_DOWN: {
            const char *next = repl_history_next(&repl->history);
            if (next) {
                strcpy(repl->current_line, next);
                repl->cursor_pos = strlen(repl->current_line);
                return true;
            }
            break;
        }
        case KEY_TAB: {
            ReplCompletion *comp = repl_completion_new();
            repl_get_completions(comp, repl->current_line, repl->cursor_pos);
            
            if (comp->count == 1) {
                /* Single completion - insert it */
                const char *completion = comp->suggestions[0];
                strcat(repl->current_line, completion + repl->cursor_pos);
                repl->cursor_pos = strlen(repl->current_line);
            } else if (comp->count > 1) {
                /* Multiple completions - show them */
                printf("\n");
                for (size_t i = 0; i < comp->count; i++) {
                    printf("  %s", comp->suggestions[i]);
                }
                printf("\n");
            }
            
            repl_completion_free(comp);
            return true;
        }
        default:
            return false;
    }
    return false;
}

/* ========== COMMANDS ========== */

void repl_register_command(const char *name, const char *desc, void (*handler)(const char *)) {
    if (g_command_count < MAX_COMMANDS) {
        g_commands[g_command_count].name = name;
        g_commands[g_command_count].description = desc;
        g_commands[g_command_count].handler = handler;
        g_command_count++;
    }
}

bool repl_execute_command(const char *line) {
    if (!line || line[0] != ':') return false;
    
    /* Parse command name */
    const char *cmd_start = line + 1;
    const char *cmd_end = cmd_start;
    while (*cmd_end && !isspace(*cmd_end)) {
        cmd_end++;
    }
    
    size_t cmd_len = cmd_end - cmd_start;
    const char *args = cmd_end;
    while (*args && isspace(*args)) {
        args++;
    }
    
    /* Find and execute command */
    for (size_t i = 0; i < g_command_count; i++) {
        if (strncmp(g_commands[i].name, cmd_start, cmd_len) == 0 &&
            strlen(g_commands[i].name) == cmd_len) {
            g_commands[i].handler(args);
            return true;
        }
    }
    
    printf("Unknown command: %.*s\n", (int)cmd_len, cmd_start);
    printf("Type :help for available commands\n");
    return true;
}

/* Built-in command handlers */

void repl_cmd_help(const char *args) {
    (void)args;
    printf("\n╔═══════════════════════════════════════╗\n");
    printf("║         RUBOLT REPL COMMANDS          ║\n");
    printf("╚═══════════════════════════════════════╝\n\n");
    
    printf("Available commands (prefix with ':'):\n\n");
    for (size_t i = 0; i < g_command_count; i++) {
        printf("  :%s\n", g_commands[i].name);
        printf("    %s\n\n", g_commands[i].description);
    }
    
    printf("Keyboard shortcuts:\n");
    printf("  Tab       - Autocomplete\n");
    printf("  Up/Down   - History navigation\n");
    printf("  Ctrl+C    - Cancel current line\n");
    printf("  Ctrl+D    - Exit REPL\n\n");
}

void repl_cmd_exit(const char *args) {
    (void)args;
    printf("Exiting REPL...\n");
    exit(0);
}

void repl_cmd_clear(const char *args) {
    (void)args;
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void repl_cmd_history(const char *args) {
    (void)args;
    printf("\nCommand history:\n");
    printf("════════════════════════════════════════\n");
    /* Access history from global state if available */
    printf("(History display not implemented)\n");
}

void repl_cmd_debug(const char *args) {
    (void)args;
    printf("Debug mode toggled\n");
}

void repl_cmd_profile(const char *args) {
    (void)args;
    printf("Profiling information:\n");
    printf("  (Profile stats would go here)\n");
}

void repl_cmd_break(const char *args) {
    if (!args || strlen(args) == 0) {
        printf("Usage: :break <line_number>\n");
        return;
    }
    printf("Breakpoint set at: %s\n", args);
}

void repl_cmd_step(const char *args) {
    (void)args;
    printf("Stepping to next statement...\n");
}

void repl_cmd_continue(const char *args) {
    (void)args;
    printf("Continuing execution...\n");
}

void repl_cmd_inspect(const char *args) {
    if (!args || strlen(args) == 0) {
        printf("Usage: :inspect <variable_name>\n");
        return;
    }
    printf("Inspecting variable: %s\n", args);
    printf("  (Variable info would go here)\n");
}

/* ========== UTILITIES ========== */

bool repl_line_is_complete(const char *line) {
    int paren_count = 0;
    int brace_count = 0;
    int bracket_count = 0;
    bool in_string = false;
    bool in_comment = false;
    char string_char = 0;
    
    for (const char *p = line; *p; p++) {
        if (in_comment) {
            if (*p == '\n') in_comment = false;
            continue;
        }
        
        if (in_string) {
            if (*p == '\\') {
                p++; // Skip escaped character
                continue;
            }
            if (*p == string_char) {
                in_string = false;
            }
            continue;
        }
        
        switch (*p) {
            case '"':
            case '\'':
                in_string = true;
                string_char = *p;
                break;
            case '#':
            case '/':
                if (*(p + 1) == '/') {
                    in_comment = true;
                }
                break;
            case '(':
                paren_count++;
                break;
            case ')':
                paren_count--;
                break;
            case '{':
                brace_count++;
                break;
            case '}':
                brace_count--;
                break;
            case '[':
                bracket_count++;
                break;
            case ']':
                bracket_count--;
                break;
        }
    }
    
    return !in_string && paren_count == 0 && brace_count == 0 && bracket_count == 0;
}

void repl_print_banner(void) {
    printf("\n");
    printf("╔═══════════════════════════════════════╗\n");
    printf("║     RUBOLT INTERACTIVE SHELL v1.0     ║\n");
    printf("╠═══════════════════════════════════════╣\n");
    printf("║  Type :help for commands              ║\n");
    printf("║  Type :exit or Ctrl+D to quit         ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    printf("\n");
}

void repl_print_prompt(bool multiline) {
    if (multiline) {
        printf("... ");
    } else {
        printf(">>> ");
    }
    fflush(stdout);
}

void repl_highlight_syntax(const char *line) {
    /* Basic syntax highlighting for terminal */
    /* This is a placeholder - full implementation would use ANSI codes */
    printf("%s\n", line);
}

/* ========== MAIN REPL LOOP ========== */

void repl_run(ReplState *repl) {
    repl_print_banner();
    
    /* Try to load history */
    repl_history_load(&repl->history, ".rubolt_history");
    
    while (repl->running) {
        repl_print_prompt(repl->multiline_mode);
        
        char *line = repl_readline(repl, "");
        if (!line) {
            /* EOF or error */
            printf("\n");
            break;
        }
        
        /* Handle empty lines */
        if (strlen(line) == 0) {
            continue;
        }
        
        /* Check for REPL commands */
        if (line[0] == ':') {
            repl_execute_command(line);
            continue;
        }
        
        /* Add to history */
        repl_history_add(&repl->history, line);
        
        /* Check if line is complete */
        if (!repl_line_is_complete(line)) {
            /* Enter multiline mode */
            repl->multiline_mode = true;
            
            if (!repl->multiline_buffer) {
                repl->multiline_buffer = strdup(line);
            } else {
                size_t old_len = strlen(repl->multiline_buffer);
                size_t new_len = old_len + strlen(line) + 2;
                repl->multiline_buffer = realloc(repl->multiline_buffer, new_len);
                strcat(repl->multiline_buffer, "\n");
                strcat(repl->multiline_buffer, line);
            }
            continue;
        }
        
        /* Execute the line */
        const char *code_to_exec;
        if (repl->multiline_mode) {
            size_t old_len = strlen(repl->multiline_buffer);
            size_t new_len = old_len + strlen(line) + 2;
            repl->multiline_buffer = realloc(repl->multiline_buffer, new_len);
            strcat(repl->multiline_buffer, "\n");
            strcat(repl->multiline_buffer, line);
            code_to_exec = repl->multiline_buffer;
            repl->multiline_mode = false;
        } else {
            code_to_exec = line;
        }
        
        /* TODO: Execute code through Rubolt interpreter */
        printf("=> Executing: %s\n", code_to_exec);
        
        /* Clear multiline buffer */
        if (repl->multiline_buffer) {
            free(repl->multiline_buffer);
            repl->multiline_buffer = NULL;
        }
    }
    
    /* Save history */
    repl_history_save(&repl->history, ".rubolt_history");
}
