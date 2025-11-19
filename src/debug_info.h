#ifndef RUBOLT_DEBUG_INFO_H
#define RUBOLT_DEBUG_INFO_H

#include <stddef.h>
#include <stdbool.h>

typedef struct SourceLocation {
    char* file_path;
    char* function_name;
    int line_number;
    int column_number;
    char* source_line;
    char* module_name;
} SourceLocation;

typedef struct DebugSymbol {
    void* address;
    char* symbol_name;
    char* file_path;
    int line_number;
    size_t size;
    struct DebugSymbol* next;
} DebugSymbol;

typedef struct LineNumberEntry {
    void* address;
    char* file_path;
    int line_number;
    int column_number;
    struct LineNumberEntry* next;
} LineNumberEntry;

typedef struct DebugInfo {
    DebugSymbol* symbols;
    LineNumberEntry* line_numbers;
    char** source_files;
    char*** source_lines;
    size_t* line_counts;
    size_t file_count;
    bool debug_enabled;
} DebugInfo;

// Global debug info
extern DebugInfo g_debug_info;

// Debug info initialization
void debug_info_init(DebugInfo* debug);
void debug_info_free(DebugInfo* debug);
void debug_info_enable(bool enable);

// Source file management
bool load_source_file(const char* file_path);
void unload_source_file(const char* file_path);
char* get_source_line(const char* file_path, int line_number);
int get_line_count(const char* file_path);

// Symbol management
void add_debug_symbol(void* address, const char* symbol_name, 
                     const char* file_path, int line_number, size_t size);
DebugSymbol* find_debug_symbol(void* address);
void remove_debug_symbol(void* address);

// Line number mapping
void add_line_number_entry(void* address, const char* file_path, 
                          int line_number, int column_number);
LineNumberEntry* find_line_number_entry(void* address);
SourceLocation* resolve_source_location(void* address);

// DWARF debug info parsing
bool parse_dwarf_info(const char* executable_path);
bool extract_line_numbers_from_dwarf(const char* file_path);
bool extract_symbols_from_dwarf(const char* file_path);

// Address resolution
void* get_return_address(int frame_offset);
void* get_frame_pointer(int frame_offset);
char* resolve_symbol_name(void* address);
SourceLocation* get_current_location(void);

// Stack walking with debug info
typedef struct DebugStackFrame {
    void* instruction_pointer;
    void* frame_pointer;
    char* function_name;
    char* file_path;
    int line_number;
    int column_number;
    char* source_line;
    struct DebugStackFrame* next;
} DebugStackFrame;

DebugStackFrame* capture_debug_stack_trace(void);
void debug_stack_trace_free(DebugStackFrame* frame);
void print_debug_stack_trace(DebugStackFrame* frame, FILE* output);

// Enhanced panic with debug info
void runtime_panic_with_debug(const char* file, int line, const char* function,
                             PanicType type, const char* format, ...);

// Macros for automatic debug info
#define PANIC_WITH_DEBUG(type, ...) \
    runtime_panic_with_debug(__FILE__, __LINE__, __func__, type, __VA_ARGS__)

#define ASSERT_WITH_DEBUG(condition, message) \
    do { \
        if (!(condition)) { \
            PANIC_WITH_DEBUG(PANIC_ASSERTION_FAILED, \
                "Assertion failed: %s", message); \
        } \
    } while(0)

// Source mapping for JIT code
typedef struct JitSourceMap {
    void* jit_address;
    char* original_file;
    int original_line;
    int original_column;
    struct JitSourceMap* next;
} JitSourceMap;

void add_jit_source_mapping(void* jit_address, const char* file, 
                           int line, int column);
SourceLocation* resolve_jit_location(void* jit_address);

#endif