#include "debug_info.h"
#include "runtime_panic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <libdwarf/libdwarf.h>
#include <libdwarf/dwarf.h>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

DebugInfo g_debug_info = {0};
static JitSourceMap* g_jit_source_map = NULL;

void debug_info_init(DebugInfo* debug) {
    debug->symbols = NULL;
    debug->line_numbers = NULL;
    debug->source_files = NULL;
    debug->source_lines = NULL;
    debug->line_counts = NULL;
    debug->file_count = 0;
    debug->debug_enabled = true;
    
    // Try to load debug info from current executable
    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        parse_dwarf_info(exe_path);
    }
}

void debug_info_free(DebugInfo* debug) {
    // Free symbols
    DebugSymbol* symbol = debug->symbols;
    while (symbol) {
        DebugSymbol* next = symbol->next;
        free(symbol->symbol_name);
        free(symbol->file_path);
        free(symbol);
        symbol = next;
    }
    
    // Free line numbers
    LineNumberEntry* entry = debug->line_numbers;
    while (entry) {
        LineNumberEntry* next = entry->next;
        free(entry->file_path);
        free(entry);
        entry = next;
    }
    
    // Free source files
    for (size_t i = 0; i < debug->file_count; i++) {
        free(debug->source_files[i]);
        
        if (debug->source_lines[i]) {
            for (size_t j = 0; j < debug->line_counts[i]; j++) {
                free(debug->source_lines[i][j]);
            }
            free(debug->source_lines[i]);
        }
    }
    
    free(debug->source_files);
    free(debug->source_lines);
    free(debug->line_counts);
    
    // Free JIT source map
    JitSourceMap* jit_map = g_jit_source_map;
    while (jit_map) {
        JitSourceMap* next = jit_map->next;
        free(jit_map->original_file);
        free(jit_map);
        jit_map = next;
    }
}

void debug_info_enable(bool enable) {
    g_debug_info.debug_enabled = enable;
}

bool load_source_file(const char* file_path) {
    if (!g_debug_info.debug_enabled) return false;
    
    // Check if already loaded
    for (size_t i = 0; i < g_debug_info.file_count; i++) {
        if (strcmp(g_debug_info.source_files[i], file_path) == 0) {
            return true;
        }
    }
    
    FILE* file = fopen(file_path, "r");
    if (!file) return false;
    
    // Count lines first
    size_t line_count = 0;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), file)) {
        line_count++;
    }
    
    rewind(file);
    
    // Allocate arrays
    g_debug_info.source_files = realloc(g_debug_info.source_files,
                                       (g_debug_info.file_count + 1) * sizeof(char*));
    g_debug_info.source_lines = realloc(g_debug_info.source_lines,
                                       (g_debug_info.file_count + 1) * sizeof(char**));
    g_debug_info.line_counts = realloc(g_debug_info.line_counts,
                                      (g_debug_info.file_count + 1) * sizeof(size_t));
    
    size_t file_index = g_debug_info.file_count;
    g_debug_info.source_files[file_index] = strdup(file_path);
    g_debug_info.line_counts[file_index] = line_count;
    g_debug_info.source_lines[file_index] = malloc(line_count * sizeof(char*));
    
    // Load all lines
    for (size_t i = 0; i < line_count; i++) {
        if (fgets(buffer, sizeof(buffer), file)) {
            // Remove newline
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }
            g_debug_info.source_lines[file_index][i] = strdup(buffer);
        } else {
            g_debug_info.source_lines[file_index][i] = strdup("");
        }
    }
    
    fclose(file);
    g_debug_info.file_count++;
    return true;
}

char* get_source_line(const char* file_path, int line_number) {
    if (!g_debug_info.debug_enabled || line_number <= 0) return NULL;
    
    // Find file
    for (size_t i = 0; i < g_debug_info.file_count; i++) {
        if (strcmp(g_debug_info.source_files[i], file_path) == 0) {
            if ((size_t)(line_number - 1) < g_debug_info.line_counts[i]) {
                return strdup(g_debug_info.source_lines[i][line_number - 1]);
            }
            break;
        }
    }
    
    // Try to load file if not found
    if (load_source_file(file_path)) {
        return get_source_line(file_path, line_number);
    }
    
    return NULL;
}

void add_debug_symbol(void* address, const char* symbol_name,
                     const char* file_path, int line_number, size_t size) {
    if (!g_debug_info.debug_enabled) return;
    
    DebugSymbol* symbol = malloc(sizeof(DebugSymbol));
    symbol->address = address;
    symbol->symbol_name = strdup(symbol_name);
    symbol->file_path = strdup(file_path);
    symbol->line_number = line_number;
    symbol->size = size;
    symbol->next = g_debug_info.symbols;
    g_debug_info.symbols = symbol;
}

DebugSymbol* find_debug_symbol(void* address) {
    DebugSymbol* symbol = g_debug_info.symbols;
    while (symbol) {
        if (symbol->address <= address && 
            (char*)address < (char*)symbol->address + symbol->size) {
            return symbol;
        }
        symbol = symbol->next;
    }
    return NULL;
}

void add_line_number_entry(void* address, const char* file_path,
                          int line_number, int column_number) {
    if (!g_debug_info.debug_enabled) return;
    
    LineNumberEntry* entry = malloc(sizeof(LineNumberEntry));
    entry->address = address;
    entry->file_path = strdup(file_path);
    entry->line_number = line_number;
    entry->column_number = column_number;
    entry->next = g_debug_info.line_numbers;
    g_debug_info.line_numbers = entry;
}

LineNumberEntry* find_line_number_entry(void* address) {
    LineNumberEntry* entry = g_debug_info.line_numbers;
    LineNumberEntry* best_match = NULL;
    
    while (entry) {
        if (entry->address <= address) {
            if (!best_match || entry->address > best_match->address) {
                best_match = entry;
            }
        }
        entry = entry->next;
    }
    
    return best_match;
}

SourceLocation* resolve_source_location(void* address) {
    if (!g_debug_info.debug_enabled) return NULL;
    
    SourceLocation* location = malloc(sizeof(SourceLocation));
    memset(location, 0, sizeof(SourceLocation));
    
    // Check JIT source map first
    SourceLocation* jit_location = resolve_jit_location(address);
    if (jit_location) {
        return jit_location;
    }
    
    // Try line number table
    LineNumberEntry* line_entry = find_line_number_entry(address);
    if (line_entry) {
        location->file_path = strdup(line_entry->file_path);
        location->line_number = line_entry->line_number;
        location->column_number = line_entry->column_number;
        location->source_line = get_source_line(line_entry->file_path, line_entry->line_number);
    }
    
    // Try symbol table
    DebugSymbol* symbol = find_debug_symbol(address);
    if (symbol) {
        if (!location->file_path) {
            location->file_path = strdup(symbol->file_path);
        }
        if (location->line_number == 0) {
            location->line_number = symbol->line_number;
        }
        location->function_name = strdup(symbol->symbol_name);
    }
    
    // Try dladdr as fallback
    if (!location->function_name) {
        Dl_info dl_info;
        if (dladdr(address, &dl_info)) {
            if (dl_info.dli_sname) {
                location->function_name = strdup(dl_info.dli_sname);
            }
            if (dl_info.dli_fname && !location->file_path) {
                location->file_path = strdup(dl_info.dli_fname);
            }
        }
    }
    
    return location;
}

bool parse_dwarf_info(const char* executable_path) {
    Dwarf_Debug debug_info;
    Dwarf_Error error;
    int fd;
    
    fd = open(executable_path, O_RDONLY);
    if (fd < 0) return false;
    
    int result = dwarf_init(fd, DW_DLC_READ, NULL, NULL, &debug_info, &error);
    if (result != DW_DLV_OK) {
        close(fd);
        return false;
    }
    
    // Extract line number information
    Dwarf_Line* line_buffer;
    Dwarf_Signed line_count;
    Dwarf_Unsigned cu_header_length, abbrev_offset, next_cu_header;
    Dwarf_Half version_stamp, address_size;
    Dwarf_Die cu_die;
    
    while (dwarf_next_cu_header(debug_info, &cu_header_length, &version_stamp,
                               &abbrev_offset, &address_size, &next_cu_header,
                               &error) == DW_DLV_OK) {
        
        if (dwarf_siblingof(debug_info, NULL, &cu_die, &error) == DW_DLV_OK) {
            if (dwarf_srclines(cu_die, &line_buffer, &line_count, &error) == DW_DLV_OK) {
                
                for (Dwarf_Signed i = 0; i < line_count; i++) {
                    Dwarf_Addr line_addr;
                    Dwarf_Unsigned line_no, column_no;
                    char* src_file;
                    
                    if (dwarf_lineaddr(line_buffer[i], &line_addr, &error) == DW_DLV_OK &&
                        dwarf_lineno(line_buffer[i], &line_no, &error) == DW_DLV_OK &&
                        dwarf_linesrc(line_buffer[i], &src_file, &error) == DW_DLV_OK) {
                        
                        // Get column number (may not be available)
                        if (dwarf_lineoff_b(line_buffer[i], &column_no, &error) != DW_DLV_OK) {
                            column_no = 0;
                        }
                        
                        add_line_number_entry((void*)line_addr, src_file, 
                                            (int)line_no, (int)column_no);
                        
                        // Load source file
                        load_source_file(src_file);
                        
                        dwarf_dealloc(debug_info, src_file, DW_DLA_STRING);
                    }
                }
                
                dwarf_srclines_dealloc(debug_info, line_buffer, line_count);
            }
            
            dwarf_dealloc(debug_info, cu_die, DW_DLA_DIE);
        }
    }
    
    // Extract symbol information
    Dwarf_Global* global_buffer;
    Dwarf_Signed global_count;
    
    if (dwarf_get_globals(debug_info, &global_buffer, &global_count, &error) == DW_DLV_OK) {
        for (Dwarf_Signed i = 0; i < global_count; i++) {
            char* symbol_name;
            Dwarf_Off die_offset;
            Dwarf_Die symbol_die;
            
            if (dwarf_global_name_offsets(global_buffer[i], &symbol_name, 
                                        &die_offset, &error) == DW_DLV_OK &&
                dwarf_offdie(debug_info, die_offset, &symbol_die, &error) == DW_DLV_OK) {
                
                Dwarf_Addr low_addr, high_addr;
                if (dwarf_lowpc(symbol_die, &low_addr, &error) == DW_DLV_OK &&
                    dwarf_highpc_b(symbol_die, &high_addr, NULL, NULL, &error) == DW_DLV_OK) {
                    
                    add_debug_symbol((void*)low_addr, symbol_name, executable_path, 
                                   0, (size_t)(high_addr - low_addr));
                }
                
                dwarf_dealloc(debug_info, symbol_die, DW_DLA_DIE);
                dwarf_dealloc(debug_info, symbol_name, DW_DLA_STRING);
            }
        }
        
        dwarf_globals_dealloc(debug_info, global_buffer, global_count);
    }
    
    dwarf_finish(debug_info, &error);
    close(fd);
    return true;
}

DebugStackFrame* capture_debug_stack_trace(void) {
    void* buffer[256];
    int frame_count = backtrace(buffer, 256);
    
    DebugStackFrame* first_frame = NULL;
    DebugStackFrame* current_frame = NULL;
    
    for (int i = 1; i < frame_count; i++) { // Skip current function
        DebugStackFrame* frame = malloc(sizeof(DebugStackFrame));
        memset(frame, 0, sizeof(DebugStackFrame));
        
        frame->instruction_pointer = buffer[i];
        
        // Resolve source location
        SourceLocation* location = resolve_source_location(buffer[i]);
        if (location) {
            if (location->function_name) {
                frame->function_name = strdup(location->function_name);
            }
            if (location->file_path) {
                frame->file_path = strdup(location->file_path);
            }
            frame->line_number = location->line_number;
            frame->column_number = location->column_number;
            if (location->source_line) {
                frame->source_line = strdup(location->source_line);
            }
            
            free(location->function_name);
            free(location->file_path);
            free(location->source_line);
            free(location);
        }
        
        // Fallback to dladdr if no debug info
        if (!frame->function_name) {
            Dl_info dl_info;
            if (dladdr(buffer[i], &dl_info)) {
                if (dl_info.dli_sname) {
                    frame->function_name = strdup(dl_info.dli_sname);
                }
                if (dl_info.dli_fname && !frame->file_path) {
                    frame->file_path = strdup(dl_info.dli_fname);
                }
            }
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

void debug_stack_trace_free(DebugStackFrame* frame) {
    while (frame) {
        DebugStackFrame* next = frame->next;
        free(frame->function_name);
        free(frame->file_path);
        free(frame->source_line);
        free(frame);
        frame = next;
    }
}

void print_debug_stack_trace(DebugStackFrame* frame, FILE* output) {
    fprintf(output, "Debug stack trace:\n");
    int frame_num = 0;
    
    while (frame) {
        fprintf(output, "  #%d: ", frame_num);
        
        if (frame->function_name) {
            fprintf(output, "%s", frame->function_name);
        } else {
            fprintf(output, "<unknown>");
        }
        
        if (frame->file_path) {
            fprintf(output, " at %s", frame->file_path);
            if (frame->line_number > 0) {
                fprintf(output, ":%d", frame->line_number);
                if (frame->column_number > 0) {
                    fprintf(output, ":%d", frame->column_number);
                }
            }
        }
        
        fprintf(output, " [%p]\n", frame->instruction_pointer);
        
        if (frame->source_line) {
            fprintf(output, "    %s\n", frame->source_line);
            
            // Add column indicator if available
            if (frame->column_number > 0) {
                fprintf(output, "    ");
                for (int i = 1; i < frame->column_number; i++) {
                    fprintf(output, " ");
                }
                fprintf(output, "^\n");
            }
        }
        
        frame = frame->next;
        frame_num++;
    }
}

void runtime_panic_with_debug(const char* file, int line, const char* function,
                             PanicType type, const char* format, ...) {
    PanicInfo info = {0};
    info.type = type;
    info.file = (char*)file;
    info.line = line;
    info.function = (char*)function;
    
    // Format message
    char message_buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    va_end(args);
    info.message = message_buffer;
    
    // Capture enhanced stack trace with debug info
    DebugStackFrame* debug_trace = capture_debug_stack_trace();
    
    // Convert to regular stack trace for compatibility
    info.stack_trace = convert_debug_trace_to_stack_trace(debug_trace);
    
    // Enhanced logging with source context
    log_panic_with_debug_info(&info, debug_trace);
    
    // Call user handlers
    bool handled = false;
    for (size_t i = 0; i < g_panic_manager.handler_count; i++) {
        if (g_panic_manager.handlers[i].handler(&info)) {
            handled = true;
            break;
        }
    }
    
    // Cleanup
    debug_stack_trace_free(debug_trace);
    stack_trace_free(info.stack_trace);
    
    if (!handled || g_panic_manager.abort_on_panic) {
        abort();
    }
}

void log_panic_with_debug_info(PanicInfo* info, DebugStackFrame* debug_trace) {
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';
    
    FILE* output = g_panic_manager.log_file ? g_panic_manager.log_file : stderr;
    
    fprintf(output, "\n=== ENHANCED PANIC [%s] ===\n", time_str);
    fprintf(output, "Type: %s\n", panic_type_to_string(info->type));
    fprintf(output, "Message: %s\n", info->message);
    
    if (info->file && info->line > 0) {
        fprintf(output, "Location: %s:%d", info->file, info->line);
        if (info->function) {
            fprintf(output, " in %s()", info->function);
        }
        fprintf(output, "\n");
        
        // Show source context
        char* source_line = get_source_line(info->file, info->line);
        if (source_line) {
            fprintf(output, "Source: %s\n", source_line);
            free(source_line);
        }
    }
    
    if (debug_trace) {
        print_debug_stack_trace(debug_trace, output);
    }
    
    fprintf(output, "=== END ENHANCED PANIC ===\n\n");
    fflush(output);
}

void add_jit_source_mapping(void* jit_address, const char* file, 
                           int line, int column) {
    JitSourceMap* mapping = malloc(sizeof(JitSourceMap));
    mapping->jit_address = jit_address;
    mapping->original_file = strdup(file);
    mapping->original_line = line;
    mapping->original_column = column;
    mapping->next = g_jit_source_map;
    g_jit_source_map = mapping;
}

SourceLocation* resolve_jit_location(void* jit_address) {
    JitSourceMap* mapping = g_jit_source_map;
    while (mapping) {
        if (mapping->jit_address == jit_address) {
            SourceLocation* location = malloc(sizeof(SourceLocation));
            memset(location, 0, sizeof(SourceLocation));
            
            location->file_path = strdup(mapping->original_file);
            location->line_number = mapping->original_line;
            location->column_number = mapping->original_column;
            location->source_line = get_source_line(mapping->original_file, 
                                                   mapping->original_line);
            location->function_name = strdup("<JIT compiled>");
            
            return location;
        }
        mapping = mapping->next;
    }
    return NULL;
}