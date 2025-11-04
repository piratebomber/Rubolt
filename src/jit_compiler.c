#include "jit_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
static void *alloc_executable(size_t size) {
    return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}
static void free_executable(void *ptr, size_t size) {
    (void)size; if (ptr) VirtualFree(ptr, 0, MEM_RELEASE);
}
#else
#include <sys/mman.h>
#include <unistd.h>
static void *alloc_executable(size_t size) {
    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
    return mem == MAP_FAILED ? NULL : mem;
}
static void free_executable(void *ptr, size_t size) {
    if (ptr) munmap(ptr, size);
}
#endif

JitCompiler *global_jit = NULL;

static JitFunction *find_func(JitCompiler *jit, const char *name) {
    for (JitFunction *f = jit->functions; f; f = f->next) {
        if (strcmp(f->name, name) == 0) return f;
    }
    return NULL;
}

void jit_init(JitCompiler *jit) {
    jit->functions = NULL;
    jit->function_count = 0;
    jit->enabled = true;
    jit->default_tier = JIT_TIER_BASELINE;
    jit->hot_threshold = 1000;
    jit->total_compiled = 0;
    jit->total_compile_time_ns = 0;
    jit->code_buffer = NULL;
    jit->code_buffer_size = 0;
    jit->code_buffer_used = 0;
}

void jit_shutdown(JitCompiler *jit) {
    JitFunction *f = jit->functions;
    while (f) {
        JitFunction *next = f->next;
        free(f->name);
        if (f->native_code) free_executable(f->native_code, f->native_code_size);
        free(f);
        f = next;
    }
    if (jit->code_buffer) free_executable(jit->code_buffer, jit->code_buffer_size);
}

void jit_enable(JitCompiler *jit) { jit->enabled = true; }
void jit_disable(JitCompiler *jit) { jit->enabled = false; }

JitFunction *jit_compile_function(JitCompiler *jit, const char *name, void *bytecode, size_t bytecode_size, JitTier tier) {
    if (!jit->enabled) return NULL;
    JitFunction *f = find_func(jit, name);
    if (!f) {
        f = (JitFunction *)calloc(1, sizeof(JitFunction));
        if (!f) return NULL;
        f->name = _strdup(name);
#ifdef _WIN32
        if (!f->name && name) f->name = _strdup(name);
#endif
        f->next = jit->functions;
        jit->functions = f;
        jit->function_count++;
    }
    f->bytecode = bytecode;
    f->bytecode_size = bytecode_size;
    f->tier = tier;
    /* Placeholder: no real codegen, allocate 1 byte NOP-like stub */
    if (!f->native_code) {
        f->native_code_size = 16;
        f->native_code = alloc_executable(f->native_code_size);
    }
    f->valid = true;
    jit->total_compiled++;
    return f;
}

bool jit_recompile(JitCompiler *jit, JitFunction *func, JitTier new_tier) {
    if (!jit->enabled || !func) return false;
    func->tier = new_tier;
    func->valid = true;
    return true;
}

JitFunction *jit_get_function(JitCompiler *jit, const char *name) { return find_func(jit, name); }

void jit_invalidate_function(JitCompiler *jit, const char *name) {
    JitFunction *f = find_func(jit, name); if (f) f->valid = false;
}

void *jit_execute(JitCompiler *jit, JitFunction *func, void *args) {
    (void)jit; (void)args;
    if (!func || !func->valid) return NULL;
    /* Placeholder: actual execution requires calling generated code */
    return NULL;
}

bool jit_should_compile(JitCompiler *jit, const char *name, uint64_t call_count) {
    return jit->enabled && call_count >= jit->hot_threshold;
}

void *jit_alloc_code_memory(JitCompiler *jit, size_t size) { (void)jit; return alloc_executable(size); }
void jit_free_code_memory(void *ptr, size_t size) { free_executable(ptr, size); }
bool jit_make_executable(void *ptr, size_t size) { (void)ptr; (void)size; return true; }

void *jit_optimize_bytecode(void *bytecode, size_t size, JitOptFlags flags) { (void)flags; return bytecode; }
void *jit_inline_calls(void *bytecode, size_t size) { return bytecode; }
void *jit_constant_fold(void *bytecode, size_t size) { return bytecode; }
void *jit_eliminate_dead_code(void *bytecode, size_t size) { return bytecode; }

void jit_print_stats(JitCompiler *jit) {
    printf("JIT: functions=%zu compiled=%llu\n", jit->function_count, (unsigned long long)jit->total_compiled);
}

void jit_get_stats(JitCompiler *jit, JitStats *stats) {
    memset(stats, 0, sizeof(*stats));
    stats->total_functions = jit->function_count;
    for (JitFunction *f = jit->functions; f; f = f->next) {
        if (f->tier == JIT_TIER_BASELINE) stats->baseline_count++;
        else if (f->tier == JIT_TIER_OPTIMIZED) stats->optimized_count++;
    }
    stats->total_compile_time_ns = jit->total_compile_time_ns;
    stats->code_cache_size = jit->code_buffer_size;
    stats->code_cache_used = jit->code_buffer_used;
}

void jit_disassemble(JitFunction *func) { (void)func; }
void jit_dump_comparison(JitFunction *func) { (void)func; }

void *jit_generate_x86_64(void *bytecode, size_t size, JitOptFlags flags) { (void)bytecode; (void)size; (void)flags; return NULL; }
void *jit_generate_arm64(void *bytecode, size_t size, JitOptFlags flags) { (void)bytecode; (void)size; (void)flags; return NULL; }
