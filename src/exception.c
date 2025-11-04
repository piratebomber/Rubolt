#include "exception.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global exception state */
ExceptionState *global_exception_state = NULL;

/* Exception type names */
static const char *exception_type_names[] = {
    "None",
    "RuntimeError",
    "TypeError",
    "ValueError",
    "NameError",
    "IndexError",
    "KeyError",
    "AttributeError",
    "ZeroDivisionError",
    "AssertionError",
    "ImportError",
    "IOError",
    "MemoryError",
    "SystemError",
    "CustomException"
};

/* ========== EXCEPTION LIFECYCLE ========== */

void exception_init(ExceptionState *state) {
    state->handler_stack = NULL;
    state->last_exception = NULL;
    state->current_frame = NULL;
    state->exception_in_progress = false;
}

void exception_shutdown(ExceptionState *state) {
    /* Free all handlers */
    while (state->handler_stack) {
        exception_pop_handler(state);
    }
    
    /* Free last exception */
    if (state->last_exception) {
        exception_free(state->last_exception);
    }
    
    /* Free frames */
    while (state->current_frame) {
        exception_pop_frame(state);
    }
}

Exception *exception_new(ExceptionType type, const char *message) {
    Exception *exc = (Exception *)malloc(sizeof(Exception));
    if (!exc) return NULL;
    
    exc->type = type;
    exc->type_name = NULL;
    exc->message = message ? strdup(message) : NULL;
    exc->detailed_message = NULL;
    exc->traceback = NULL;
    exc->traceback_depth = 0;
    exc->cause = NULL;
    exc->custom_data = NULL;
    exc->handled = false;
    
    return exc;
}

Exception *exception_new_custom(const char *type_name, const char *message) {
    Exception *exc = exception_new(EXC_CUSTOM, message);
    if (exc) {
        exc->type_name = strdup(type_name);
    }
    return exc;
}

void exception_free(Exception *exc) {
    if (!exc) return;
    
    free(exc->type_name);
    free(exc->message);
    free(exc->detailed_message);
    
    /* Free traceback */
    StackFrameInfo *frame = exc->traceback;
    while (frame) {
        StackFrameInfo *next = frame->next;
        free(frame->filename);
        free(frame->function_name);
        free(frame->source_line);
        free(frame);
        frame = next;
    }
    
    /* Free cause */
    if (exc->cause) {
        exception_free(exc->cause);
    }
    
    free(exc);
}

Exception *exception_clone(Exception *exc) {
    if (!exc) return NULL;
    
    Exception *clone = exception_new(exc->type, exc->message);
    if (exc->type_name) {
        clone->type_name = strdup(exc->type_name);
    }
    if (exc->detailed_message) {
        clone->detailed_message = strdup(exc->detailed_message);
    }
    
    /* Clone traceback */
    StackFrameInfo *src_frame = exc->traceback;
    StackFrameInfo **dst_frame = &clone->traceback;
    while (src_frame) {
        *dst_frame = (StackFrameInfo *)malloc(sizeof(StackFrameInfo));
        (*dst_frame)->filename = strdup(src_frame->filename);
        (*dst_frame)->function_name = strdup(src_frame->function_name);
        (*dst_frame)->line_number = src_frame->line_number;
        (*dst_frame)->column_number = src_frame->column_number;
        (*dst_frame)->source_line = src_frame->source_line ? strdup(src_frame->source_line) : NULL;
        (*dst_frame)->next = NULL;
        
        dst_frame = &(*dst_frame)->next;
        src_frame = src_frame->next;
        clone->traceback_depth++;
    }
    
    return clone;
}

/* ========== EXCEPTION HANDLING ========== */

void exception_push_handler(ExceptionState *state, ExceptionHandler *handler) {
    handler->current_exception = NULL;
    handler->has_finally = false;
    handler->finally_block = NULL;
    handler->prev = state->handler_stack;
    state->handler_stack = handler;
}

void exception_pop_handler(ExceptionState *state) {
    if (!state->handler_stack) return;
    
    ExceptionHandler *handler = state->handler_stack;
    state->handler_stack = handler->prev;
    
    /* Execute finally block if present */
    if (handler->has_finally && handler->finally_block) {
        handler->finally_block();
    }
}

void exception_raise(ExceptionState *state, Exception *exc) {
    if (!exc) return;
    
    /* Capture traceback */
    exc->traceback = exception_capture_traceback(state);
    
    /* Set as current exception */
    if (state->last_exception) {
        exception_free(state->last_exception);
    }
    state->last_exception = exc;
    state->exception_in_progress = true;
    
    /* Set in handler */
    if (state->handler_stack) {
        state->handler_stack->current_exception = exc;
    }
}

void exception_reraise(ExceptionState *state) {
    if (state->last_exception) {
        exception_raise(state, exception_clone(state->last_exception));
    }
}

bool exception_matches(Exception *exc, ExceptionType type) {
    return exc && exc->type == type;
}

bool exception_matches_custom(Exception *exc, const char *type_name) {
    return exc && exc->type == EXC_CUSTOM && exc->type_name && 
           strcmp(exc->type_name, type_name) == 0;
}

Exception *exception_current(ExceptionState *state) {
    return state->last_exception;
}

void exception_clear(ExceptionState *state) {
    if (state->last_exception) {
        exception_free(state->last_exception);
        state->last_exception = NULL;
    }
    state->exception_in_progress = false;
}

/* ========== TRACEBACK MANAGEMENT ========== */

void exception_push_frame(ExceptionState *state, const char *filename, 
                         const char *function, int line, int column) {
    StackFrameInfo *frame = (StackFrameInfo *)malloc(sizeof(StackFrameInfo));
    frame->filename = filename ? strdup(filename) : NULL;
    frame->function_name = function ? strdup(function) : NULL;
    frame->line_number = line;
    frame->column_number = column;
    frame->source_line = NULL;
    frame->next = state->current_frame;
    state->current_frame = frame;
}

void exception_pop_frame(ExceptionState *state) {
    if (!state->current_frame) return;
    
    StackFrameInfo *frame = state->current_frame;
    state->current_frame = frame->next;
    
    free(frame->filename);
    free(frame->function_name);
    free(frame->source_line);
    free(frame);
}

StackFrameInfo *exception_capture_traceback(ExceptionState *state) {
    StackFrameInfo *traceback = NULL;
    StackFrameInfo **tail = &traceback;
    
    StackFrameInfo *frame = state->current_frame;
    while (frame) {
        StackFrameInfo *copy = (StackFrameInfo *)malloc(sizeof(StackFrameInfo));
        copy->filename = frame->filename ? strdup(frame->filename) : NULL;
        copy->function_name = frame->function_name ? strdup(frame->function_name) : NULL;
        copy->line_number = frame->line_number;
        copy->column_number = frame->column_number;
        copy->source_line = frame->source_line ? strdup(frame->source_line) : NULL;
        copy->next = NULL;
        
        *tail = copy;
        tail = &copy->next;
        
        frame = frame->next;
    }
    
    return traceback;
}

void exception_print_traceback(Exception *exc) {
    if (!exc) return;
    
    fprintf(stderr, "Traceback (most recent call last):\n");
    
    StackFrameInfo *frame = exc->traceback;
    while (frame) {
        fprintf(stderr, "  File \"%s\", line %d", 
                frame->filename ? frame->filename : "<unknown>",
                frame->line_number);
        
        if (frame->function_name) {
            fprintf(stderr, ", in %s", frame->function_name);
        }
        fprintf(stderr, "\n");
        
        if (frame->source_line) {
            fprintf(stderr, "    %s\n", frame->source_line);
        }
        
        frame = frame->next;
    }
}

char *exception_format_traceback(Exception *exc) {
    if (!exc) return strdup("No exception");
    
    size_t size = 1024;
    char *buffer = (char *)malloc(size);
    size_t offset = 0;
    
    offset += snprintf(buffer + offset, size - offset,
                      "Traceback (most recent call last):\n");
    
    StackFrameInfo *frame = exc->traceback;
    while (frame && offset < size - 100) {
        offset += snprintf(buffer + offset, size - offset,
                          "  File \"%s\", line %d, in %s\n",
                          frame->filename ? frame->filename : "<unknown>",
                          frame->line_number,
                          frame->function_name ? frame->function_name : "<module>");
        frame = frame->next;
    }
    
    return buffer;
}

int exception_traceback_depth(Exception *exc) {
    return exc ? exc->traceback_depth : 0;
}

/* ========== EXCEPTION CHAIN ========== */

void exception_set_cause(Exception *exc, Exception *cause) {
    if (exc) {
        exc->cause = cause;
    }
}

Exception *exception_get_cause(Exception *exc) {
    return exc ? exc->cause : NULL;
}

void exception_print_chain(Exception *exc) {
    if (!exc) return;
    
    exception_print(exc);
    
    if (exc->cause) {
        fprintf(stderr, "\nThe above exception was the direct cause of:\n\n");
        exception_print_chain(exc->cause);
    }
}

/* ========== PREDEFINED EXCEPTIONS ========== */

Exception *exception_runtime_error(const char *message) {
    return exception_new(EXC_RUNTIME_ERROR, message);
}

Exception *exception_type_error(const char *message) {
    return exception_new(EXC_TYPE_ERROR, message);
}

Exception *exception_value_error(const char *message) {
    return exception_new(EXC_VALUE_ERROR, message);
}

Exception *exception_name_error(const char *name) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "name '%s' is not defined", name);
    return exception_new(EXC_NAME_ERROR, buffer);
}

Exception *exception_index_error(int index) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "index %d out of range", index);
    return exception_new(EXC_INDEX_ERROR, buffer);
}

Exception *exception_key_error(const char *key) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "key '%s' not found", key);
    return exception_new(EXC_KEY_ERROR, buffer);
}

Exception *exception_attribute_error(const char *obj, const char *attr) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "'%s' object has no attribute '%s'", obj, attr);
    return exception_new(EXC_ATTRIBUTE_ERROR, buffer);
}

Exception *exception_zero_division(void) {
    return exception_new(EXC_ZERO_DIVISION_ERROR, "division by zero");
}

Exception *exception_assertion_error(const char *message) {
    return exception_new(EXC_ASSERTION_ERROR, message);
}

Exception *exception_import_error(const char *module) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "cannot import module '%s'", module);
    return exception_new(EXC_IMPORT_ERROR, buffer);
}

Exception *exception_io_error(const char *filename, const char *message) {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%s: %s", filename, message);
    return exception_new(EXC_IO_ERROR, buffer);
}

Exception *exception_memory_error(void) {
    return exception_new(EXC_MEMORY_ERROR, "out of memory");
}

/* ========== UTILITIES ========== */

const char *exception_type_name(ExceptionType type) {
    if (type >= 0 && type <= EXC_CUSTOM) {
        return exception_type_names[type];
    }
    return "UnknownException";
}

char *exception_format_message(Exception *exc) {
    if (!exc) return strdup("No exception");
    
    char buffer[1024];
    const char *type_name = exc->type == EXC_CUSTOM && exc->type_name ?
                           exc->type_name : exception_type_name(exc->type);
    
    snprintf(buffer, sizeof(buffer), "%s: %s", 
             type_name, exc->message ? exc->message : "");
    
    return strdup(buffer);
}

void exception_print(Exception *exc) {
    if (!exc) return;
    
    exception_print_traceback(exc);
    
    char *msg = exception_format_message(exc);
    fprintf(stderr, "%s\n", msg);
    free(msg);
}

bool exception_in_handler(ExceptionState *state) {
    return state->handler_stack != NULL;
}

char *exception_to_string(Exception *exc) {
    if (!exc) return strdup("No exception");
    
    char *traceback = exception_format_traceback(exc);
    char *message = exception_format_message(exc);
    
    size_t total_len = strlen(traceback) + strlen(message) + 10;
    char *result = (char *)malloc(total_len);
    snprintf(result, total_len, "%s%s\n", traceback, message);
    
    free(traceback);
    free(message);
    
    return result;
}
