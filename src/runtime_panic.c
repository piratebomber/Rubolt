#include "runtime_panic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

PanicManager g_panic_manager = {0};

typedef struct MemoryBlock {
    void* ptr;
    size_t size;
    char* file;
    int line;
    char* function;
    struct MemoryBlock* next;
} MemoryBlock;

static MemoryBlock* g_memory_blocks = NULL;
static bool g_memory_debugging = false;
static size_t g_total_allocated = 0;
static size_t g_allocation_count = 0;

void panic_manager_init(PanicManager* pm) {
    pm->handlers = NULL;
    pm->handler_count = 0;
    pm->abort_on_panic = true;
    pm->print_stack_trace = true;
    pm->collect_core_dump = false;
    pm->log_file_path = NULL;
    pm->log_file = NULL;
    
    // Set up signal handlers for crashes
    signal(SIGSEGV, crash_signal_handler);
    signal(SIGABRT, crash_signal_handler);
    signal(SIGFPE, crash_signal_handler);
    signal(SIGILL, crash_signal_handler);
}

void panic_manager_free(PanicManager* pm) {
    for (size_t i = 0; i < pm->handler_count; i++) {
        // Handlers are function pointers, no cleanup needed
    }
    free(pm->handlers);
    
    if (pm->log_file) {
        fclose(pm->log_file);
    }
    free(pm->log_file_path);
}

void panic_manager_add_handler(PanicManager* pm, bool (*handler)(PanicInfo*), void* user_data) {
    pm->handlers = realloc(pm->handlers, (pm->handler_count + 1) * sizeof(PanicHandler));
    pm->handlers[pm->handler_count].handler = handler;
    pm->handlers[pm->handler_count].user_data = user_data;
    pm->handler_count++;
}

void panic_manager_set_log_file(PanicManager* pm, const char* path) {
    if (pm->log_file) {
        fclose(pm->log_file);
    }
    
    free(pm->log_file_path);
    pm->log_file_path = strdup(path);
    pm->log_file = fopen(path, "a");
    
    if (!pm->log_file) {
        fprintf(stderr, "Warning: Could not open panic log file: %s\n", path);
    }
}

StackFrame* stack_trace_capture(void) {
    void* buffer[256];
    int frame_count = backtrace(buffer, 256);
    char** symbols = backtrace_symbols(buffer, frame_count);
    
    StackFrame* first_frame = NULL;
    StackFrame* current_frame = NULL;
    
    for (int i = 1; i < frame_count; i++) { // Skip current function
        StackFrame* frame = malloc(sizeof(StackFrame));
        
        // Parse symbol information
        char* symbol = symbols[i];
        char* function_start = strchr(symbol, '(');
        char* function_end = strchr(symbol, '+');
        
        if (function_start && function_end && function_start < function_end) {
            size_t func_len = function_end - function_start - 1;
            frame->function_name = malloc(func_len + 1);
            strncpy(frame->function_name, function_start + 1, func_len);
            frame->function_name[func_len] = '\0';
        } else {
            frame->function_name = strdup("unknown");
        }
        
        // Extract file name (simplified)
        char* file_start = symbol;
        char* file_end = strchr(symbol, '(');
        if (file_end) {
            size_t file_len = file_end - file_start;
            frame->file_name = malloc(file_len + 1);
            strncpy(frame->file_name, file_start, file_len);
            frame->file_name[file_len] = '\0';
        } else {\n            frame->file_name = strdup(symbol);\n        }\n        \n        frame->line_number = 0; // Would need debug info for actual line numbers\n        frame->source_line = NULL;\n        frame->next = NULL;\n        \n        if (!first_frame) {\n            first_frame = frame;\n            current_frame = frame;\n        } else {\n            current_frame->next = frame;\n            current_frame = frame;\n        }\n    }\n    \n    free(symbols);\n    return first_frame;\n}\n\nvoid stack_trace_free(StackFrame* frame) {\n    while (frame) {\n        StackFrame* next = frame->next;\n        free(frame->function_name);\n        free(frame->file_name);\n        free(frame->source_line);\n        free(frame);\n        frame = next;\n    }\n}\n\nvoid stack_trace_print(StackFrame* frame, FILE* output) {\n    fprintf(output, \"Stack trace:\\n\");\n    int frame_num = 0;\n    \n    while (frame) {\n        fprintf(output, \"  #%d: %s in %s\", frame_num, \n                frame->function_name ? frame->function_name : \"unknown\",\n                frame->file_name ? frame->file_name : \"unknown\");\n        \n        if (frame->line_number > 0) {\n            fprintf(output, \":%d\", frame->line_number);\n        }\n        \n        fprintf(output, \"\\n\");\n        \n        if (frame->source_line) {\n            fprintf(output, \"    %s\\n\", frame->source_line);\n        }\n        \n        frame = frame->next;\n        frame_num++;\n    }\n}\n\nchar* stack_trace_to_string(StackFrame* frame) {\n    size_t buffer_size = 4096;\n    char* buffer = malloc(buffer_size);\n    size_t offset = 0;\n    \n    offset += snprintf(buffer + offset, buffer_size - offset, \"Stack trace:\\n\");\n    \n    int frame_num = 0;\n    while (frame && offset < buffer_size - 100) {\n        offset += snprintf(buffer + offset, buffer_size - offset,\n                          \"  #%d: %s in %s\", frame_num,\n                          frame->function_name ? frame->function_name : \"unknown\",\n                          frame->file_name ? frame->file_name : \"unknown\");\n        \n        if (frame->line_number > 0) {\n            offset += snprintf(buffer + offset, buffer_size - offset, \":%d\", frame->line_number);\n        }\n        \n        offset += snprintf(buffer + offset, buffer_size - offset, \"\\n\");\n        \n        if (frame->source_line) {\n            offset += snprintf(buffer + offset, buffer_size - offset, \"    %s\\n\", frame->source_line);\n        }\n        \n        frame = frame->next;\n        frame_num++;\n    }\n    \n    return buffer;\n}\n\nstatic const char* panic_type_to_string(PanicType type) {\n    switch (type) {\n        case PANIC_ASSERTION_FAILED: return \"AssertionFailed\";\n        case PANIC_NULL_POINTER: return \"NullPointer\";\n        case PANIC_INDEX_OUT_OF_BOUNDS: return \"IndexOutOfBounds\";\n        case PANIC_DIVISION_BY_ZERO: return \"DivisionByZero\";\n        case PANIC_STACK_OVERFLOW: return \"StackOverflow\";\n        case PANIC_HEAP_EXHAUSTED: return \"HeapExhausted\";\n        case PANIC_TYPE_ERROR: return \"TypeError\";\n        case PANIC_INVALID_OPERATION: return \"InvalidOperation\";\n        case PANIC_CONSTRAINT_VIOLATION: return \"ConstraintViolation\";\n        case PANIC_GENERIC_INSTANTIATION_FAILED: return \"GenericInstantiationFailed\";\n        case PANIC_PATTERN_MATCH_FAILED: return \"PatternMatchFailed\";\n        case PANIC_UNREACHABLE_CODE: return \"UnreachableCode\";\n        case PANIC_CUSTOM: return \"Custom\";\n        default: return \"Unknown\";\n    }\n}\n\nvoid log_panic_info(PanicInfo* info) {\n    time_t now = time(NULL);\n    char* time_str = ctime(&now);\n    time_str[strlen(time_str) - 1] = '\\0'; // Remove newline\n    \n    FILE* output = g_panic_manager.log_file ? g_panic_manager.log_file : stderr;\n    \n    fprintf(output, \"\\n=== PANIC [%s] ===\\n\", time_str);\n    fprintf(output, \"Type: %s\\n\", panic_type_to_string(info->type));\n    fprintf(output, \"Message: %s\\n\", info->message);\n    \n    if (info->file && info->line > 0) {\n        fprintf(output, \"Location: %s:%d\", info->file, info->line);\n        if (info->function) {\n            fprintf(output, \" in %s()\", info->function);\n        }\n        fprintf(output, \"\\n\");\n    }\n    \n    if (info->stack_trace && g_panic_manager.print_stack_trace) {\n        stack_trace_print(info->stack_trace, output);\n    }\n    \n    if (info->context_data && info->context_size > 0) {\n        fprintf(output, \"Context data (%zu bytes):\\n\", info->context_size);\n        unsigned char* data = (unsigned char*)info->context_data;\n        for (size_t i = 0; i < info->context_size && i < 256; i++) {\n            if (i % 16 == 0) fprintf(output, \"  %04zx: \", i);\n            fprintf(output, \"%02x \", data[i]);\n            if (i % 16 == 15) fprintf(output, \"\\n\");\n        }\n        if (info->context_size % 16 != 0) fprintf(output, \"\\n\");\n    }\n    \n    fprintf(output, \"=== END PANIC ===\\n\\n\");\n    fflush(output);\n}\n\nvoid generate_core_dump(PanicInfo* info) {\n    if (!g_panic_manager.collect_core_dump) return;\n    \n    pid_t pid = fork();\n    if (pid == 0) {\n        // Child process - generate core dump\n        char core_file[256];\n        snprintf(core_file, sizeof(core_file), \"rubolt_core_%d_%ld.dump\", \n                getpid(), time(NULL));\n        \n        FILE* core = fopen(core_file, \"wb\");\n        if (core) {\n            // Write panic info\n            fwrite(info, sizeof(PanicInfo), 1, core);\n            \n            // Write stack trace\n            if (info->stack_trace) {\n                char* stack_str = stack_trace_to_string(info->stack_trace);\n                size_t stack_len = strlen(stack_str);\n                fwrite(&stack_len, sizeof(size_t), 1, core);\n                fwrite(stack_str, 1, stack_len, core);\n                free(stack_str);\n            }\n            \n            // Write context data\n            if (info->context_data && info->context_size > 0) {\n                fwrite(info->context_data, 1, info->context_size, core);\n            }\n            \n            fclose(core);\n            fprintf(stderr, \"Core dump written to: %s\\n\", core_file);\n        }\n        \n        _exit(0);\n    } else if (pid > 0) {\n        // Parent process - wait for child\n        int status;\n        waitpid(pid, &status, 0);\n    }\n}\n\nvoid runtime_panic_impl(PanicType type, void* context, size_t context_size, \n                       const char* file, int line, const char* function,\n                       const char* format, va_list args) {\n    PanicInfo info = {0};\n    info.type = type;\n    info.file = (char*)file;\n    info.line = line;\n    info.function = (char*)function;\n    info.context_data = context;\n    info.context_size = context_size;\n    \n    // Format message\n    char message_buffer[1024];\n    vsnprintf(message_buffer, sizeof(message_buffer), format, args);\n    info.message = message_buffer;\n    \n    // Capture stack trace\n    info.stack_trace = stack_trace_capture();\n    \n    // Log panic information\n    log_panic_info(&info);\n    \n    // Generate core dump if enabled\n    generate_core_dump(&info);\n    \n    // Call user handlers\n    bool handled = false;\n    for (size_t i = 0; i < g_panic_manager.handler_count; i++) {\n        if (g_panic_manager.handlers[i].handler(&info)) {\n            handled = true;\n            break;\n        }\n    }\n    \n    // Cleanup\n    stack_trace_free(info.stack_trace);\n    \n    // Abort if not handled or if configured to always abort\n    if (!handled || g_panic_manager.abort_on_panic) {\n        abort();\n    }\n}\n\nvoid runtime_panic(const char* format, ...) {\n    va_list args;\n    va_start(args, format);\n    runtime_panic_impl(PANIC_CUSTOM, NULL, 0, NULL, 0, NULL, format, args);\n    va_end(args);\n}\n\nvoid runtime_panic_with_type(PanicType type, const char* format, ...) {\n    va_list args;\n    va_start(args, format);\n    runtime_panic_impl(type, NULL, 0, NULL, 0, NULL, format, args);\n    va_end(args);\n}\n\nvoid runtime_panic_with_context(PanicType type, void* context, size_t context_size, \n                               const char* format, ...) {\n    va_list args;\n    va_start(args, format);\n    runtime_panic_impl(type, context, context_size, NULL, 0, NULL, format, args);\n    va_end(args);\n}\n\nvoid crash_signal_handler(int sig) {\n    const char* signal_name;\n    switch (sig) {\n        case SIGSEGV: signal_name = \"SIGSEGV (Segmentation fault)\"; break;\n        case SIGABRT: signal_name = \"SIGABRT (Abort)\"; break;\n        case SIGFPE: signal_name = \"SIGFPE (Floating point exception)\"; break;\n        case SIGILL: signal_name = \"SIGILL (Illegal instruction)\"; break;\n        default: signal_name = \"Unknown signal\"; break;\n    }\n    \n    runtime_panic_with_type(PANIC_CUSTOM, \"Received signal %d: %s\", sig, signal_name);\n}\n\n// Safe memory operations\nvoid* safe_malloc(size_t size) {\n    if (size == 0) {\n        runtime_panic_with_type(PANIC_INVALID_OPERATION, \"Attempted to allocate 0 bytes\");\n        return NULL;\n    }\n    \n    void* ptr = malloc(size);\n    if (!ptr) {\n        runtime_panic_with_type(PANIC_HEAP_EXHAUSTED, \n                               \"Failed to allocate %zu bytes (total allocated: %zu)\", \n                               size, g_total_allocated);\n        return NULL;\n    }\n    \n    if (g_memory_debugging) {\n        MemoryBlock* block = malloc(sizeof(MemoryBlock));\n        block->ptr = ptr;\n        block->size = size;\n        block->file = NULL; // Would need debug info\n        block->line = 0;\n        block->function = NULL;\n        block->next = g_memory_blocks;\n        g_memory_blocks = block;\n        \n        g_total_allocated += size;\n        g_allocation_count++;\n    }\n    \n    return ptr;\n}\n\nvoid* safe_realloc(void* ptr, size_t size) {\n    if (size == 0) {\n        free(ptr);\n        return NULL;\n    }\n    \n    void* new_ptr = realloc(ptr, size);\n    if (!new_ptr && size > 0) {\n        runtime_panic_with_type(PANIC_HEAP_EXHAUSTED,\n                               \"Failed to reallocate %zu bytes\", size);\n        return NULL;\n    }\n    \n    if (g_memory_debugging && ptr != new_ptr) {\n        // Update memory tracking\n        MemoryBlock* block = g_memory_blocks;\n        while (block) {\n            if (block->ptr == ptr) {\n                g_total_allocated = g_total_allocated - block->size + size;\n                block->ptr = new_ptr;\n                block->size = size;\n                break;\n            }\n            block = block->next;\n        }\n    }\n    \n    return new_ptr;\n}\n\nvoid* safe_calloc(size_t count, size_t size) {\n    if (count == 0 || size == 0) {\n        runtime_panic_with_type(PANIC_INVALID_OPERATION, \n                               \"Attempted to allocate 0 elements or 0 size\");\n        return NULL;\n    }\n    \n    // Check for overflow\n    if (count > SIZE_MAX / size) {\n        runtime_panic_with_type(PANIC_HEAP_EXHAUSTED,\n                               \"Integer overflow in calloc: %zu * %zu\", count, size);\n        return NULL;\n    }\n    \n    void* ptr = calloc(count, size);\n    if (!ptr) {\n        runtime_panic_with_type(PANIC_HEAP_EXHAUSTED,\n                               \"Failed to allocate %zu bytes (calloc)\", count * size);\n        return NULL;\n    }\n    \n    if (g_memory_debugging) {\n        MemoryBlock* block = malloc(sizeof(MemoryBlock));\n        block->ptr = ptr;\n        block->size = count * size;\n        block->file = NULL;\n        block->line = 0;\n        block->function = NULL;\n        block->next = g_memory_blocks;\n        g_memory_blocks = block;\n        \n        g_total_allocated += count * size;\n        g_allocation_count++;\n    }\n    \n    return ptr;\n}\n\nchar* safe_strdup(const char* str) {\n    ASSERT_NOT_NULL(str, \"string to duplicate\");\n    \n    size_t len = strlen(str) + 1;\n    char* copy = safe_malloc(len);\n    if (copy) {\n        memcpy(copy, str, len);\n    }\n    return copy;\n}\n\nvoid enable_memory_debugging(void) {\n    g_memory_debugging = true;\n}\n\nvoid disable_memory_debugging(void) {\n    g_memory_debugging = false;\n}\n\nvoid print_memory_leaks(void) {\n    if (!g_memory_debugging) {\n        printf(\"Memory debugging not enabled\\n\");\n        return;\n    }\n    \n    printf(\"Memory leak report:\\n\");\n    printf(\"Total allocated: %zu bytes in %zu allocations\\n\", \n           g_total_allocated, g_allocation_count);\n    \n    size_t leak_count = 0;\n    size_t leak_bytes = 0;\n    \n    MemoryBlock* block = g_memory_blocks;\n    while (block) {\n        printf(\"  Leak: %p (%zu bytes)\", block->ptr, block->size);\n        if (block->file) {\n            printf(\" at %s:%d\", block->file, block->line);\n        }\n        if (block->function) {\n            printf(\" in %s()\", block->function);\n        }\n        printf(\"\\n\");\n        \n        leak_count++;\n        leak_bytes += block->size;\n        block = block->next;\n    }\n    \n    if (leak_count == 0) {\n        printf(\"No memory leaks detected\\n\");\n    } else {\n        printf(\"Total leaks: %zu allocations, %zu bytes\\n\", leak_count, leak_bytes);\n    }\n}