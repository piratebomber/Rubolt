#ifndef RUBOLT_EXCEPTION_H
#define RUBOLT_EXCEPTION_H

#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>

/* Exception types */
typedef enum {
    EXC_NONE,
    EXC_RUNTIME_ERROR,
    EXC_TYPE_ERROR,
    EXC_VALUE_ERROR,
    EXC_NAME_ERROR,
    EXC_INDEX_ERROR,
    EXC_KEY_ERROR,
    EXC_ATTRIBUTE_ERROR,
    EXC_ZERO_DIVISION_ERROR,
    EXC_ASSERTION_ERROR,
    EXC_IMPORT_ERROR,
    EXC_IO_ERROR,
    EXC_MEMORY_ERROR,
    EXC_SYSTEM_ERROR,
    EXC_CUSTOM               /* User-defined exception */
} ExceptionType;

/* Stack frame info for traceback */
typedef struct StackFrameInfo {
    char *filename;
    char *function_name;
    int line_number;
    int column_number;
    char *source_line;
    struct StackFrameInfo *next;
} StackFrameInfo;

/* Exception object */
typedef struct Exception {
    ExceptionType type;
    char *type_name;         /* For custom exceptions */
    char *message;
    char *detailed_message;
    StackFrameInfo *traceback;
    int traceback_depth;
    struct Exception *cause; /* Exception that caused this one */
    void *custom_data;       /* User-defined data */
    bool handled;
} Exception;

/* Exception handler context */
typedef struct ExceptionHandler {
    jmp_buf env;
    Exception *current_exception;
    bool has_finally;
    void (*finally_block)(void);
    struct ExceptionHandler *prev;
} ExceptionHandler;

/* Exception state */
typedef struct ExceptionState {
    ExceptionHandler *handler_stack;
    Exception *last_exception;
    StackFrameInfo *current_frame;
    bool exception_in_progress;
} ExceptionState;

/* ========== EXCEPTION LIFECYCLE ========== */

/* Initialize exception system */
void exception_init(ExceptionState *state);

/* Shutdown exception system */
void exception_shutdown(ExceptionState *state);

/* Create new exception */
Exception *exception_new(ExceptionType type, const char *message);

/* Create custom exception */
Exception *exception_new_custom(const char *type_name, const char *message);

/* Free exception */
void exception_free(Exception *exc);

/* Clone exception */
Exception *exception_clone(Exception *exc);

/* ========== EXCEPTION HANDLING ========== */

/* Push exception handler (try block) */
void exception_push_handler(ExceptionState *state, ExceptionHandler *handler);

/* Pop exception handler */
void exception_pop_handler(ExceptionState *state);

/* Raise exception */
void exception_raise(ExceptionState *state, Exception *exc);

/* Re-raise current exception */
void exception_reraise(ExceptionState *state);

/* Check if exception matches type */
bool exception_matches(Exception *exc, ExceptionType type);

/* Check if exception matches custom type */
bool exception_matches_custom(Exception *exc, const char *type_name);

/* Get current exception */
Exception *exception_current(ExceptionState *state);

/* Clear exception */
void exception_clear(ExceptionState *state);

/* ========== TRACEBACK MANAGEMENT ========== */

/* Push stack frame */
void exception_push_frame(ExceptionState *state, const char *filename, 
                         const char *function, int line, int column);

/* Pop stack frame */
void exception_pop_frame(ExceptionState *state);

/* Capture traceback */
StackFrameInfo *exception_capture_traceback(ExceptionState *state);

/* Print traceback */
void exception_print_traceback(Exception *exc);

/* Format traceback to string */
char *exception_format_traceback(Exception *exc);

/* Get traceback depth */
int exception_traceback_depth(Exception *exc);

/* ========== EXCEPTION CHAIN ========== */

/* Set exception cause */
void exception_set_cause(Exception *exc, Exception *cause);

/* Get exception cause */
Exception *exception_get_cause(Exception *exc);

/* Print full exception chain */
void exception_print_chain(Exception *exc);

/* ========== PREDEFINED EXCEPTIONS ========== */

/* Runtime error */
Exception *exception_runtime_error(const char *message);

/* Type error */
Exception *exception_type_error(const char *message);

/* Value error */
Exception *exception_value_error(const char *message);

/* Name error */
Exception *exception_name_error(const char *name);

/* Index error */
Exception *exception_index_error(int index);

/* Key error */
Exception *exception_key_error(const char *key);

/* Attribute error */
Exception *exception_attribute_error(const char *obj, const char *attr);

/* Zero division error */
Exception *exception_zero_division(void);

/* Assertion error */
Exception *exception_assertion_error(const char *message);

/* Import error */
Exception *exception_import_error(const char *module);

/* IO error */
Exception *exception_io_error(const char *filename, const char *message);

/* Memory error */
Exception *exception_memory_error(void);

/* ========== UTILITIES ========== */

/* Get exception type name */
const char *exception_type_name(ExceptionType type);

/* Format exception message */
char *exception_format_message(Exception *exc);

/* Print exception */
void exception_print(Exception *exc);

/* Check if in exception handler */
bool exception_in_handler(ExceptionState *state);

/* Get exception info as string */
char *exception_to_string(Exception *exc);

/* ========== MACROS FOR EASY USAGE ========== */

/* Try-except-finally macros */
#define TRY(state) \
    do { \
        ExceptionHandler _handler; \
        exception_push_handler(state, &_handler); \
        if (setjmp(_handler.env) == 0) {

#define EXCEPT(state, exc_var) \
        } else { \
            Exception *exc_var = exception_current(state);

#define EXCEPT_TYPE(state, type, exc_var) \
        } else { \
            Exception *exc_var = exception_current(state); \
            if (exception_matches(exc_var, type)) {

#define EXCEPT_CUSTOM(state, type_name, exc_var) \
        } else { \
            Exception *exc_var = exception_current(state); \
            if (exception_matches_custom(exc_var, type_name)) {

#define FINALLY(state) \
        } \
        exception_pop_handler(state); \
    } while (0); \
    /* Finally block */

#define END_TRY \
        exception_pop_handler(state); \
    } while (0)

/* Raise macro */
#define RAISE(state, exc) \
    do { \
        exception_raise(state, exc); \
        if (state->handler_stack) longjmp(state->handler_stack->env, 1); \
    } while (0)

/* Assert macro */
#define ASSERT(state, cond, msg) \
    do { \
        if (!(cond)) { \
            RAISE(state, exception_assertion_error(msg)); \
        } \
    } while (0)

/* Global exception state */
extern ExceptionState *global_exception_state;

#endif /* RUBOLT_EXCEPTION_H */
