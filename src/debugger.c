#include "debugger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Debugger *global_debugger = NULL;

static Breakpoint *bp_new(int id, BreakpointType type) {
    Breakpoint *bp = (Breakpoint *)calloc(1, sizeof(Breakpoint));
    if (!bp) return NULL;
    bp->id = id;
    bp->type = type;
    bp->enabled = true;
    return bp;
}

void debugger_init(Debugger *dbg) {
    dbg->state = DEBUG_STATE_RUNNING;
    dbg->breakpoints = NULL;
    dbg->next_breakpoint_id = 1;
    dbg->call_stack = NULL;
    dbg->stack_depth = 0;
    dbg->step_over = false;
    dbg->step_into = false;
    dbg->step_out = false;
    dbg->step_target_depth = 0;
    dbg->current_file = NULL;
    dbg->current_line = 0;
}

void debugger_shutdown(Debugger *dbg) {
    Breakpoint *bp = dbg->breakpoints;
    while (bp) {
        Breakpoint *next = bp->next;
        free(bp->filename);
        free(bp->function_name);
        free(bp->condition);
        free(bp);
        bp = next;
    }
    while (dbg->call_stack) {
        debugger_pop_frame(dbg);
    }
}

void debugger_enable(Debugger *dbg) { dbg->state = DEBUG_STATE_RUNNING; }
void debugger_disable(Debugger *dbg) { dbg->state = DEBUG_STATE_STOPPED; }

int debugger_add_breakpoint(Debugger *dbg, const char *filename, int line) {
    Breakpoint *bp = bp_new(dbg->next_breakpoint_id++, BP_TYPE_LINE);
    if (filename) bp->filename = _strdup(filename);
#ifdef _WIN32
    if (!bp->filename && filename) bp->filename = _strdup(filename);
#endif
    bp->line_number = line;
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;
    return bp->id;
}

int debugger_add_function_breakpoint(Debugger *dbg, const char *function_name) {
    Breakpoint *bp = bp_new(dbg->next_breakpoint_id++, BP_TYPE_FUNCTION);
    if (function_name) bp->function_name = _strdup(function_name);
#ifdef _WIN32
    if (!bp->function_name && function_name) bp->function_name = _strdup(function_name);
#endif
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;
    return bp->id;
}

int debugger_add_conditional_breakpoint(Debugger *dbg, const char *filename, int line, const char *condition) {
    Breakpoint *bp = bp_new(dbg->next_breakpoint_id++, BP_TYPE_CONDITIONAL);
    if (filename) bp->filename = _strdup(filename);
#ifdef _WIN32
    if (!bp->filename && filename) bp->filename = _strdup(filename);
#endif
    bp->line_number = line;
    if (condition) bp->condition = _strdup(condition);
#ifdef _WIN32
    if (!bp->condition && condition) bp->condition = _strdup(condition);
#endif
    bp->next = dbg->breakpoints;
    dbg->breakpoints = bp;
    return bp->id;
}

static Breakpoint *find_bp(Debugger *dbg, int id) {
    Breakpoint *bp = dbg->breakpoints;
    while (bp) { if (bp->id == id) return bp; bp = bp->next; }
    return NULL;
}

bool debugger_remove_breakpoint(Debugger *dbg, int breakpoint_id) {
    Breakpoint *prev = NULL, *bp = dbg->breakpoints;
    while (bp) {
        if (bp->id == breakpoint_id) {
            if (prev) prev->next = bp->next; else dbg->breakpoints = bp->next;
            free(bp->filename); free(bp->function_name); free(bp->condition); free(bp);
            return true;
        }
        prev = bp; bp = bp->next;
    }
    return false;
}

void debugger_enable_breakpoint(Debugger *dbg, int breakpoint_id) {
    Breakpoint *bp = find_bp(dbg, breakpoint_id);
    if (bp) bp->enabled = true;
}

void debugger_disable_breakpoint(Debugger *dbg, int breakpoint_id) {
    Breakpoint *bp = find_bp(dbg, breakpoint_id);
    if (bp) bp->enabled = false;
}

void debugger_list_breakpoints(Debugger *dbg) {
    printf("Breakpoints:\n");
    Breakpoint *bp = dbg->breakpoints;
    while (bp) {
        printf("  #%d %s ", bp->id, bp->enabled ? "ENABLED" : "DISABLED");
        if (bp->type == BP_TYPE_LINE) {
            printf("%s:%d", bp->filename ? bp->filename : "<unknown>", bp->line_number);
        } else if (bp->type == BP_TYPE_FUNCTION) {
            printf("fn %s", bp->function_name ? bp->function_name : "<unknown>");
        } else {
            printf("%s:%d if %s", bp->filename ? bp->filename : "<unknown>", bp->line_number,
                   bp->condition ? bp->condition : "<expr>");
        }
        printf(" (hits=%d)\n", bp->hit_count);
        bp = bp->next;
    }
}

void debugger_clear_breakpoints(Debugger *dbg) {
    while (dbg->breakpoints) {
        Breakpoint *next = dbg->breakpoints->next;
        free(dbg->breakpoints->filename);
        free(dbg->breakpoints->function_name);
        free(dbg->breakpoints->condition);
        free(dbg->breakpoints);
        dbg->breakpoints = next;
    }
}

bool debugger_should_break(Debugger *dbg, const char *filename, int line) {
    Breakpoint *bp = dbg->breakpoints;
    while (bp) {
        if (bp->enabled) {
            if (bp->type == BP_TYPE_LINE) {
                if (bp->line_number == line && (!bp->filename || (filename && strcmp(bp->filename, filename) == 0))) {
                    bp->hit_count++;
                    return true;
                }
            } else if (bp->type == BP_TYPE_CONDITIONAL) {
                if (bp->line_number == line && (!bp->filename || (filename && strcmp(bp->filename, filename) == 0))) {
                    bp->hit_count++; /* TODO: Evaluate condition */
                    return true;
                }
            }
        }
        bp = bp->next;
    }
    if (dbg->state == DEBUG_STATE_STEPPING) return true;
    if (dbg->step_over && dbg->stack_depth <= dbg->step_target_depth) return true;
    if (dbg->step_out && dbg->stack_depth < dbg->step_target_depth) return true;
    return false;
}

void debugger_continue(Debugger *dbg) { dbg->state = DEBUG_STATE_RUNNING; dbg->step_over = dbg->step_into = dbg->step_out = false; }
void debugger_step_into(Debugger *dbg) { dbg->state = DEBUG_STATE_STEPPING; dbg->step_into = true; }
void debugger_step_over(Debugger *dbg) { dbg->state = DEBUG_STATE_STEPPING; dbg->step_over = true; dbg->step_target_depth = dbg->stack_depth; }
void debugger_step_out(Debugger *dbg) { dbg->state = DEBUG_STATE_STEPPING; dbg->step_out = true; dbg->step_target_depth = dbg->stack_depth - 1; }
void debugger_pause(Debugger *dbg) { dbg->state = DEBUG_STATE_PAUSED; }

void debugger_push_frame(Debugger *dbg, const char *function_name, const char *filename, int line) {
    StackFrame *f = (StackFrame *)calloc(1, sizeof(StackFrame));
    if (function_name) f->function_name = _strdup(function_name);
    if (filename) f->filename = _strdup(filename);
#ifdef _WIN32
    if (!f->function_name && function_name) f->function_name = _strdup(function_name);
    if (!f->filename && filename) f->filename = _strdup(filename);
#endif
    f->line_number = line;
    f->next = dbg->call_stack;
    dbg->call_stack = f;
    dbg->stack_depth++;
}

void debugger_pop_frame(Debugger *dbg) {
    if (!dbg->call_stack) return;
    StackFrame *f = dbg->call_stack;
    dbg->call_stack = f->next;
    dbg->stack_depth--;
    free(f->function_name);
    free(f->filename);
    free(f);
}

void debugger_print_stack(Debugger *dbg) {
    printf("Call stack (most recent first):\n");
    StackFrame *f = dbg->call_stack;
    int i = 0;
    while (f) {
        printf("  #%d %s (%s:%d)\n", i++, f->function_name ? f->function_name : "<fn>",
               f->filename ? f->filename : "<file>", f->line_number);
        f = f->next;
    }
}

StackFrame *debugger_current_frame(Debugger *dbg) { return dbg->call_stack; }

void debugger_inspect_var(Debugger *dbg, const char *var_name) {
    (void)dbg; (void)var_name; /* Placeholder: integrate with interpreter symbol table */
    printf("inspect: %s = <unimplemented>\n", var_name);
}

void debugger_list_vars(Debugger *dbg) {
    (void)dbg; /* Placeholder */
    printf("locals: <unimplemented>\n");
}

void debugger_set_var(Debugger *dbg, const char *var_name, const char *value) {
    (void)dbg; (void)var_name; (void)value; /* Placeholder */
    printf("set %s = %s (unimplemented)\n", var_name, value);
}

void debugger_watch_var(Debugger *dbg, const char *var_name) {
    (void)dbg; (void)var_name; /* Placeholder */
}

void debugger_show_source(Debugger *dbg, int context_lines) {
    (void)dbg; (void)context_lines; /* Placeholder: read file and print around line */
}

void debugger_show_disassembly(Debugger *dbg) {
    (void)dbg; /* Placeholder: requires VM bytecode context */
}

void debugger_on_line(Debugger *dbg, const char *filename, int line) {
    dbg->current_file = (char *)filename;
    dbg->current_line = line;
    if (debugger_should_break(dbg, filename, line)) {
        dbg->state = DEBUG_STATE_PAUSED;
        printf("Paused at %s:%d\n", filename ? filename : "<file>", line);
    }
}

void debugger_on_function_enter(Debugger *dbg, const char *function_name) {
    (void)function_name; /* Integrated via push_frame in interpreter */
}

void debugger_on_function_exit(Debugger *dbg) {
    /* Integrated via pop_frame in interpreter */
}
