#ifndef RUBOLT_RUNTIME_PANIC_H
#define RUBOLT_RUNTIME_PANIC_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PANIC_ASSERTION_FAILED,
    PANIC_NULL_POINTER,
    PANIC_INDEX_OUT_OF_BOUNDS,
    PANIC_DIVISION_BY_ZERO,
    PANIC_STACK_OVERFLOW,
    PANIC_HEAP_EXHAUSTED,
    PANIC_TYPE_ERROR,
    PANIC_INVALID_OPERATION,
    PANIC_CONSTRAINT_VIOLATION,
    PANIC_GENERIC_INSTANTIATION_FAILED,
    PANIC_PATTERN_MATCH_FAILED,
    PANIC_UNREACHABLE_CODE,
    PANIC_CUSTOM
} PanicType;

typedef struct StackFrame {
    char* function_name;
    char* file_name;
    int line_number;
    char* source_line;
    struct StackFrame* next;
} StackFrame;

typedef struct {
    PanicType type;
    char* message;
    char* file;
    int line;
    char* function;
    StackFrame* stack_trace;
    void* context_data;
    size_t context_size;
} PanicInfo;

typedef struct {
    bool (*handler)(PanicInfo* info);
    void* user_data;
} PanicHandler;

typedef struct {
    PanicHandler* handlers;
    size_t handler_count;
    bool abort_on_panic;
    bool print_stack_trace;
    bool collect_core_dump;
    char* log_file_path;
    FILE* log_file;
} PanicManager;

// Global panic manager
extern PanicManager g_panic_manager;

// Panic manager operations
void panic_manager_init(PanicManager* pm);
void panic_manager_free(PanicManager* pm);
void panic_manager_add_handler(PanicManager* pm, bool (*handler)(PanicInfo*), void* user_data);
void panic_manager_set_log_file(PanicManager* pm, const char* path);

// Stack trace operations
StackFrame* stack_trace_capture(void);
void stack_trace_free(StackFrame* frame);
void stack_trace_print(StackFrame* frame, FILE* output);
char* stack_trace_to_string(StackFrame* frame);

// Panic functions
void runtime_panic(const char* format, ...);
void runtime_panic_with_type(PanicType type, const char* format, ...);
void runtime_panic_with_context(PanicType type, void* context, size_t context_size, const char* format, ...);

// Assertion macros
#define ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            runtime_panic_with_type(PANIC_ASSERTION_FAILED, \
                "Assertion failed: %s at %s:%d in %s()", \
                message, __FILE__, __LINE__, __func__); \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr, name) \
    do { \
        if ((ptr) == NULL) { \
            runtime_panic_with_type(PANIC_NULL_POINTER, \
                "Null pointer: %s at %s:%d in %s()", \
                name, __FILE__, __LINE__, __func__); \
        } \
    } while(0)

#define ASSERT_BOUNDS(index, size, container) \
    do { \
        if ((index) < 0 || (index) >= (size)) { \
            runtime_panic_with_type(PANIC_INDEX_OUT_OF_BOUNDS, \
                "Index %d out of bounds [0, %zu) for %s at %s:%d in %s()", \
                (index), (size), (container), __FILE__, __LINE__, __func__); \
        } \
    } while(0)

#define UNREACHABLE(message) \
    runtime_panic_with_type(PANIC_UNREACHABLE_CODE, \
        "Unreachable code: %s at %s:%d in %s()", \
        message, __FILE__, __LINE__, __func__)

// Safe operations with panic handling
void* safe_malloc(size_t size);
void* safe_realloc(void* ptr, size_t size);
void* safe_calloc(size_t count, size_t size);
char* safe_strdup(const char* str);

// Memory debugging
void enable_memory_debugging(void);
void disable_memory_debugging(void);
void print_memory_leaks(void);

#endif