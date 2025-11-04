#ifndef RUBOLT_DEBUGGER_H
#define RUBOLT_DEBUGGER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Debugger states */
typedef enum {
    DEBUG_STATE_RUNNING,
    DEBUG_STATE_PAUSED,
    DEBUG_STATE_STEPPING,
    DEBUG_STATE_STOPPED
} DebugState;

/* Breakpoint types */
typedef enum {
    BP_TYPE_LINE,
    BP_TYPE_FUNCTION,
    BP_TYPE_CONDITIONAL
} BreakpointType;

/* Breakpoint structure */
typedef struct Breakpoint {
    int id;
    BreakpointType type;
    char *filename;
    int line_number;
    char *function_name;
    char *condition;
    int hit_count;
    bool enabled;
    struct Breakpoint *next;
} Breakpoint;

/* Call stack frame */
typedef struct StackFrame {
    char *function_name;
    char *filename;
    int line_number;
    void *local_vars;  /* HashMap of local variables */
    struct StackFrame *next;
} StackFrame;

/* Debugger context */
typedef struct Debugger {
    DebugState state;
    Breakpoint *breakpoints;
    int next_breakpoint_id;
    StackFrame *call_stack;
    int stack_depth;
    bool step_over;
    bool step_into;
    bool step_out;
    int step_target_depth;
    char *current_file;
    int current_line;
} Debugger;

/* ========== DEBUGGER LIFECYCLE ========== */

/* Initialize debugger */
void debugger_init(Debugger *dbg);

/* Shutdown debugger */
void debugger_shutdown(Debugger *dbg);

/* Enable/disable debugger */
void debugger_enable(Debugger *dbg);
void debugger_disable(Debugger *dbg);

/* ========== BREAKPOINT MANAGEMENT ========== */

/* Add line breakpoint */
int debugger_add_breakpoint(Debugger *dbg, const char *filename, int line);

/* Add function breakpoint */
int debugger_add_function_breakpoint(Debugger *dbg, const char *function_name);

/* Add conditional breakpoint */
int debugger_add_conditional_breakpoint(Debugger *dbg, const char *filename, 
                                        int line, const char *condition);

/* Remove breakpoint */
bool debugger_remove_breakpoint(Debugger *dbg, int breakpoint_id);

/* Enable/disable breakpoint */
void debugger_enable_breakpoint(Debugger *dbg, int breakpoint_id);
void debugger_disable_breakpoint(Debugger *dbg, int breakpoint_id);

/* List all breakpoints */
void debugger_list_breakpoints(Debugger *dbg);

/* Clear all breakpoints */
void debugger_clear_breakpoints(Debugger *dbg);

/* Check if should break at location */
bool debugger_should_break(Debugger *dbg, const char *filename, int line);

/* ========== EXECUTION CONTROL ========== */

/* Continue execution */
void debugger_continue(Debugger *dbg);

/* Step into (enter functions) */
void debugger_step_into(Debugger *dbg);

/* Step over (skip function calls) */
void debugger_step_over(Debugger *dbg);

/* Step out (exit current function) */
void debugger_step_out(Debugger *dbg);

/* Pause execution */
void debugger_pause(Debugger *dbg);

/* ========== STACK MANAGEMENT ========== */

/* Push stack frame */
void debugger_push_frame(Debugger *dbg, const char *function_name, 
                        const char *filename, int line);

/* Pop stack frame */
void debugger_pop_frame(Debugger *dbg);

/* Print call stack */
void debugger_print_stack(Debugger *dbg);

/* Get current frame */
StackFrame *debugger_current_frame(Debugger *dbg);

/* ========== VARIABLE INSPECTION ========== */

/* Inspect variable */
void debugger_inspect_var(Debugger *dbg, const char *var_name);

/* List all variables in current scope */
void debugger_list_vars(Debugger *dbg);

/* Set variable value */
void debugger_set_var(Debugger *dbg, const char *var_name, const char *value);

/* Watch variable */
void debugger_watch_var(Debugger *dbg, const char *var_name);

/* ========== SOURCE CODE DISPLAY ========== */

/* Show source code around current line */
void debugger_show_source(Debugger *dbg, int context_lines);

/* Show disassembly */
void debugger_show_disassembly(Debugger *dbg);

/* ========== HOOK FOR INTERPRETER ========== */

/* Called before executing each line */
void debugger_on_line(Debugger *dbg, const char *filename, int line);

/* Called on function entry */
void debugger_on_function_enter(Debugger *dbg, const char *function_name);

/* Called on function exit */
void debugger_on_function_exit(Debugger *dbg);

/* Global debugger instance */
extern Debugger *global_debugger;

#endif /* RUBOLT_DEBUGGER_H */
