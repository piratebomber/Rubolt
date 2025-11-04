#ifndef RUBOLT_REPL_H
#define RUBOLT_REPL_H

#include <stddef.h>
#include <stdbool.h>

/* Maximum line length */
#define REPL_MAX_LINE 4096
#define REPL_HISTORY_SIZE 1000
#define REPL_MAX_COMPLETIONS 100

/* REPL command structure */
typedef struct ReplCommand {
    const char *name;
    const char *description;
    void (*handler)(const char *args);
} ReplCommand;

/* REPL history entry */
typedef struct ReplHistory {
    char **lines;
    size_t count;
    size_t capacity;
    int current_index;
} ReplHistory;

/* REPL state */
typedef struct ReplState {
    ReplHistory history;
    char *current_line;
    size_t cursor_pos;
    bool running;
    bool multiline_mode;
    char *multiline_buffer;
    size_t multiline_size;
} ReplState;

/* Autocompletion context */
typedef struct ReplCompletion {
    char **suggestions;
    size_t count;
    size_t current;
} ReplCompletion;

/* ========== REPL LIFECYCLE ========== */

/* Initialize REPL */
void repl_init(ReplState *repl);

/* Run REPL main loop */
void repl_run(ReplState *repl);

/* Shutdown REPL */
void repl_shutdown(ReplState *repl);

/* ========== HISTORY MANAGEMENT ========== */

/* Initialize history */
void repl_history_init(ReplHistory *history);

/* Add line to history */
void repl_history_add(ReplHistory *history, const char *line);

/* Get previous history entry */
const char *repl_history_prev(ReplHistory *history);

/* Get next history entry */
const char *repl_history_next(ReplHistory *history);

/* Save history to file */
bool repl_history_save(ReplHistory *history, const char *filename);

/* Load history from file */
bool repl_history_load(ReplHistory *history, const char *filename);

/* Clear history */
void repl_history_clear(ReplHistory *history);

/* Free history */
void repl_history_free(ReplHistory *history);

/* ========== AUTOCOMPLETION ========== */

/* Initialize completion */
ReplCompletion *repl_completion_new(void);

/* Get completions for input */
void repl_get_completions(ReplCompletion *comp, const char *input, size_t cursor_pos);

/* Free completion */
void repl_completion_free(ReplCompletion *comp);

/* ========== LINE EDITING ========== */

/* Read line with editing support */
char *repl_readline(ReplState *repl, const char *prompt);

/* Handle special keys (arrows, tab, etc.) */
bool repl_handle_special_key(ReplState *repl, int key);

/* ========== COMMANDS ========== */

/* Register REPL command */
void repl_register_command(const char *name, const char *desc, void (*handler)(const char *));

/* Execute REPL command */
bool repl_execute_command(const char *line);

/* Built-in commands */
void repl_cmd_help(const char *args);
void repl_cmd_exit(const char *args);
void repl_cmd_clear(const char *args);
void repl_cmd_history(const char *args);
void repl_cmd_debug(const char *args);
void repl_cmd_profile(const char *args);
void repl_cmd_break(const char *args);
void repl_cmd_step(const char *args);
void repl_cmd_continue(const char *args);
void repl_cmd_inspect(const char *args);

/* ========== UTILITIES ========== */

/* Check if line is complete (balanced brackets, etc.) */
bool repl_line_is_complete(const char *line);

/* Print welcome banner */
void repl_print_banner(void);

/* Print prompt */
void repl_print_prompt(bool multiline);

/* Highlight syntax (basic) */
void repl_highlight_syntax(const char *line);

#endif /* RUBOLT_REPL_H */
