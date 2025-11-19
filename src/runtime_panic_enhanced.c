#include "runtime_panic.h"
#include "debug_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <pthread.h>

typedef struct {
    int signal_number;
    const char* signal_name;
    pid_t process_id;
    pthread_t thread_id;
    time_t timestamp;
} CrashContext;

StackFrame* stack_trace_capture_enhanced(void) {
    void* buffer[256];
    int frame_count = backtrace(buffer, 256);
    
    StackFrame* first_frame = NULL;
    StackFrame* current_frame = NULL;
    
    for (int i = 1; i < frame_count; i++) {
        StackFrame* frame = malloc(sizeof(StackFrame));
        
        // Try to resolve with debug info first
        SourceLocation* location = resolve_source_location(buffer[i]);
        
        if (location) {
            frame->function_name = location->function_name ? 
                                  strdup(location->function_name) : strdup("unknown");
            frame->file_name = location->file_path ? 
                               strdup(location->file_path) : strdup("unknown");
            frame->line_number = location->line_number;
            frame->source_line = location->source_line ? 
                                strdup(location->source_line) : NULL;
            
            free(location->function_name);
            free(location->file_path);
            free(location->source_line);
            free(location);
        } else {
            // Fallback to backtrace_symbols
            char** symbols = backtrace_symbols(&buffer[i], 1);
            if (symbols) {
                char* symbol = symbols[0];
                
                char* function_start = strchr(symbol, '(');
                char* function_end = strchr(symbol, '+');
                
                if (function_start && function_end && function_start < function_end) {
                    size_t func_len = function_end - function_start - 1;
                    frame->function_name = malloc(func_len + 1);
                    strncpy(frame->function_name, function_start + 1, func_len);
                    frame->function_name[func_len] = '\0';
                } else {
                    Dl_info dl_info;
                    if (dladdr(buffer[i], &dl_info) && dl_info.dli_sname) {
                        frame->function_name = strdup(dl_info.dli_sname);
                    } else {
                        frame->function_name = strdup("unknown");
                    }
                }
                
                char* file_start = symbol;
                char* file_end = strchr(symbol, '(');
                if (file_end) {
                    size_t file_len = file_end - file_start;
                    frame->file_name = malloc(file_len + 1);
                    strncpy(frame->file_name, file_start, file_len);
                    frame->file_name[file_len] = '\0';
                } else {
                    Dl_info dl_info;
                    if (dladdr(buffer[i], &dl_info) && dl_info.dli_fname) {
                        frame->file_name = strdup(dl_info.dli_fname);
                    } else {
                        frame->file_name = strdup(symbol);
                    }
                }
                
                free(symbols);
            } else {
                frame->function_name = strdup("unknown");
                frame->file_name = strdup("unknown");
            }
            
            frame->line_number = 0;
            frame->source_line = NULL;
        }
        
        frame->next = NULL;
        
        if (!first_frame) {
            first_frame = frame;
            current_frame = frame;
        } else {
            current_frame->next = frame;
            current_frame = frame;
        }
    }
    
    return first_frame;
}

void log_enhanced_panic_info(PanicInfo* info) {
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';
    
    FILE* output = g_panic_manager.log_file ? g_panic_manager.log_file : stderr;
    
    fprintf(output, "\n=== ENHANCED PANIC [%s] ===\n", time_str);
    fprintf(output, "Type: %s\n", panic_type_to_string(info->type));
    fprintf(output, "Message: %s\n", info->message);
    fprintf(output, "Process: %d, Thread: %lu\n", getpid(), (unsigned long)pthread_self());
    
    if (info->file && info->line > 0) {
        fprintf(output, "Location: %s:%d", info->file, info->line);
        if (info->function) {
            fprintf(output, " in %s()", info->function);
        }
        fprintf(output, "\n");
        
        show_source_context(output, info->file, info->line, 3);
    }
    
    if (info->stack_trace && g_panic_manager.print_stack_trace) {
        stack_trace_print_enhanced(info->stack_trace, output);
    }
    
    if (info->context_data && info->context_size > 0) {
        fprintf(output, "\nContext data (%zu bytes):\n", info->context_size);
        print_context_data_formatted(output, info->context_data, info->context_size);
    }
    
    show_memory_statistics(output);
    
    fprintf(output, "=== END ENHANCED PANIC ===\n\n");
    fflush(output);
}

void show_source_context(FILE* output, const char* file, int line, int context_lines) {
    fprintf(output, "\nSource context:\n");
    
    int start_line = (line - context_lines > 1) ? line - context_lines : 1;
    int end_line = line + context_lines;
    
    for (int i = start_line; i <= end_line; i++) {
        char* source_line = get_source_line(file, i);
        if (source_line) {
            char marker = (i == line) ? '>' : ' ';
            fprintf(output, "  %c %4d: %s\n", marker, i, source_line);
            free(source_line);
        }
    }
}

void print_context_data_formatted(FILE* output, void* data, size_t size) {
    unsigned char* bytes = (unsigned char*)data;
    
    if (size == sizeof(CrashContext)) {
        CrashContext* crash = (CrashContext*)data;
        fprintf(output, "  Crash Context:\n");
        fprintf(output, "    Signal: %d (%s)\n", crash->signal_number, crash->signal_name);
        fprintf(output, "    Process ID: %d\n", crash->process_id);
        fprintf(output, "    Thread ID: %lu\n", (unsigned long)crash->thread_id);
        fprintf(output, "    Timestamp: %ld\n", crash->timestamp);
        return;
    }
    
    for (size_t i = 0; i < size && i < 256; i += 16) {
        fprintf(output, "  %04zx: ", i);
        
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            fprintf(output, "%02x ", bytes[i + j]);
        }
        
        for (size_t j = size - i; j < 16; j++) {
            fprintf(output, "   ");
        }
        
        fprintf(output, " |");
        
        for (size_t j = 0; j < 16 && i + j < size; j++) {
            char c = bytes[i + j];
            fprintf(output, "%c", (c >= 32 && c <= 126) ? c : '.');
        }
        
        fprintf(output, "|\n");
    }
    
    if (size > 256) {
        fprintf(output, "  ... (%zu more bytes)\n", size - 256);
    }
}

void show_memory_statistics(FILE* output) {
    extern size_t g_total_allocated;
    extern size_t g_allocation_count;
    extern bool g_memory_debugging;
    
    fprintf(output, "\nMemory statistics:\n");
    fprintf(output, "  Total allocated: %zu bytes\n", g_total_allocated);
    fprintf(output, "  Allocation count: %zu\n", g_allocation_count);
    
    if (g_memory_debugging) {
        fprintf(output, "  Active allocations: %d\n", count_active_allocations());
    }
}

int count_active_allocations(void) {
    extern MemoryBlock* g_memory_blocks;
    
    int count = 0;
    MemoryBlock* block = g_memory_blocks;
    while (block) {
        count++;
        block = block->next;
    }
    return count;
}

void stack_trace_print_enhanced(StackFrame* frame, FILE* output) {
    fprintf(output, "Enhanced stack trace:\n");
    int frame_num = 0;
    
    while (frame) {
        fprintf(output, "  #%d: %s", frame_num,
                frame->function_name ? frame->function_name : "<unknown>");
        
        if (frame->file_name) {
            char* filename = strrchr(frame->file_name, '/');
            filename = filename ? filename + 1 : frame->file_name;
            fprintf(output, " at %s", filename);
            
            if (frame->line_number > 0) {
                fprintf(output, ":%d", frame->line_number);
            }
        }
        
        fprintf(output, "\n");
        
        if (frame->source_line) {
            fprintf(output, "    %s\n", frame->source_line);
        }
        
        frame = frame->next;
        frame_num++;
        
        if (frame_num >= 20) {
            if (frame) {
                int remaining = 0;
                StackFrame* temp = frame;
                while (temp) {
                    remaining++;
                    temp = temp->next;
                }
                fprintf(output, "  ... (%d more frames)\n", remaining);
            }
            break;
        }
    }
}

void crash_signal_handler_enhanced(int sig) {
    const char* signal_name;
    PanicType panic_type;
    
    switch (sig) {
        case SIGSEGV: 
            signal_name = "SIGSEGV (Segmentation fault)";
            panic_type = PANIC_NULL_POINTER;
            break;
        case SIGABRT: 
            signal_name = "SIGABRT (Abort)";
            panic_type = PANIC_CUSTOM;
            break;
        case SIGFPE: 
            signal_name = "SIGFPE (Floating point exception)";
            panic_type = PANIC_DIVISION_BY_ZERO;
            break;
        case SIGILL: 
            signal_name = "SIGILL (Illegal instruction)";
            panic_type = PANIC_INVALID_OPERATION;
            break;
        default: 
            signal_name = "Unknown signal";
            panic_type = PANIC_CUSTOM;
            break;
    }
    
    CrashContext crash_ctx = {
        .signal_number = sig,
        .signal_name = signal_name,
        .process_id = getpid(),
        .thread_id = pthread_self(),
        .timestamp = time(NULL)
    };
    
    runtime_panic_with_context(panic_type, &crash_ctx, sizeof(CrashContext),
                               "Fatal signal received: %d (%s)", sig, signal_name);
}

void runtime_panic_with_debug(const char* file, int line, const char* function,
                             PanicType type, const char* format, ...) {
    PanicInfo info = {0};
    info.type = type;
    info.file = (char*)file;
    info.line = line;
    info.function = (char*)function;
    
    char message_buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    va_end(args);
    info.message = message_buffer;
    
    // Load source file if not already loaded
    if (file) {
        load_source_file(file);
    }
    
    // Capture enhanced stack trace with debug info
    info.stack_trace = stack_trace_capture_enhanced();
    
    // Enhanced logging with source context
    log_enhanced_panic_info(&info);
    
    // Generate core dump if enabled
    generate_core_dump(&info);
    
    // Call user handlers
    bool handled = false;
    for (size_t i = 0; i < g_panic_manager.handler_count; i++) {
        if (g_panic_manager.handlers[i].handler(&info)) {
            handled = true;
            break;
        }
    }
    
    // Cleanup
    stack_trace_free(info.stack_trace);
    
    if (!handled || g_panic_manager.abort_on_panic) {
        abort();
    }
}

void panic_manager_init_enhanced(PanicManager* pm) {
    pm->handlers = NULL;
    pm->handler_count = 0;
    pm->abort_on_panic = true;
    pm->print_stack_trace = true;
    pm->collect_core_dump = false;
    pm->log_file_path = NULL;
    pm->log_file = NULL;
    
    // Initialize debug info system
    debug_info_init(&g_debug_info);
    
    // Set up enhanced signal handlers for crashes
    struct sigaction sa;
    sa.sa_handler = crash_signal_handler_enhanced;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
}