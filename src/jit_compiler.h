#ifndef RUBOLT_JIT_COMPILER_H
#define RUBOLT_JIT_COMPILER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* JIT compilation tiers */
typedef enum {
    JIT_TIER_NONE,              /* No compilation */
    JIT_TIER_BASELINE,          /* Simple translation */
    JIT_TIER_OPTIMIZED          /* Optimized with inlining, constant folding */
} JitTier;

/* JIT compiled function */
typedef struct JitFunction {
    char *name;
    void *bytecode;             /* Original bytecode */
    size_t bytecode_size;
    void *native_code;          /* JIT compiled native code */
    size_t native_code_size;
    JitTier tier;
    uint64_t call_count;
    uint64_t compile_time_ns;
    bool valid;
    struct JitFunction *next;
} JitFunction;

/* JIT compiler state */
typedef struct JitCompiler {
    JitFunction *functions;
    size_t function_count;
    bool enabled;
    JitTier default_tier;
    uint64_t hot_threshold;     /* Calls before JIT compilation */
    uint64_t total_compiled;
    uint64_t total_compile_time_ns;
    void *code_buffer;          /* Executable memory buffer */
    size_t code_buffer_size;
    size_t code_buffer_used;
} JitCompiler;

/* Optimization flags */
typedef enum {
    JIT_OPT_NONE           = 0,
    JIT_OPT_CONSTANT_FOLD  = 1 << 0,
    JIT_OPT_DEAD_CODE_ELIM = 1 << 1,
    JIT_OPT_INLINE_CALLS   = 1 << 2,
    JIT_OPT_LOOP_UNROLL    = 1 << 3,
    JIT_OPT_REGISTER_ALLOC = 1 << 4,
    JIT_OPT_ALL            = 0xFFFF
} JitOptFlags;

/* ========== JIT COMPILER LIFECYCLE ========== */

/* Initialize JIT compiler */
void jit_init(JitCompiler *jit);

/* Shutdown JIT compiler */
void jit_shutdown(JitCompiler *jit);

/* Enable/disable JIT compilation */
void jit_enable(JitCompiler *jit);
void jit_disable(JitCompiler *jit);

/* ========== COMPILATION ========== */

/* Compile function to native code */
JitFunction *jit_compile_function(JitCompiler *jit, const char *name, 
                                  void *bytecode, size_t bytecode_size,
                                  JitTier tier);

/* Recompile with higher tier */
bool jit_recompile(JitCompiler *jit, JitFunction *func, JitTier new_tier);

/* Check if function is compiled */
JitFunction *jit_get_function(JitCompiler *jit, const char *name);

/* Invalidate compiled function */
void jit_invalidate_function(JitCompiler *jit, const char *name);

/* ========== EXECUTION ========== */

/* Execute JIT compiled function */
void *jit_execute(JitCompiler *jit, JitFunction *func, void *args);

/* Check if function should be JIT compiled */
bool jit_should_compile(JitCompiler *jit, const char *name, uint64_t call_count);

/* ========== CODE GENERATION ========== */

/* Allocate executable memory */
void *jit_alloc_code_memory(JitCompiler *jit, size_t size);

/* Free executable memory */
void jit_free_code_memory(void *ptr, size_t size);

/* Make memory executable */
bool jit_make_executable(void *ptr, size_t size);

/* ========== OPTIMIZATION ========== */

/* Apply optimizations to bytecode */
void *jit_optimize_bytecode(void *bytecode, size_t size, JitOptFlags flags);

/* Inline function calls */
void *jit_inline_calls(void *bytecode, size_t size);

/* Constant folding */
void *jit_constant_fold(void *bytecode, size_t size);

/* Dead code elimination */
void *jit_eliminate_dead_code(void *bytecode, size_t size);

/* ========== STATISTICS ========== */

/* Print JIT statistics */
void jit_print_stats(JitCompiler *jit);

/* Get compilation statistics */
typedef struct JitStats {
    size_t total_functions;
    size_t baseline_count;
    size_t optimized_count;
    uint64_t total_compile_time_ns;
    uint64_t total_speedup;     /* Estimated */
    size_t code_cache_size;
    size_t code_cache_used;
} JitStats;

void jit_get_stats(JitCompiler *jit, JitStats *stats);

/* ========== DISASSEMBLY ========== */

/* Disassemble JIT compiled code */
void jit_disassemble(JitFunction *func);

/* Dump bytecode and native code side-by-side */
void jit_dump_comparison(JitFunction *func);

/* ========== PLATFORM SPECIFICS ========== */

#ifdef _WIN32
/* Windows specific */
#include <windows.h>
#define JIT_PAGE_SIZE 4096
#else
/* Unix specific */
#include <sys/mman.h>
#include <unistd.h>
#define JIT_PAGE_SIZE sysconf(_SC_PAGESIZE)
#endif

/* Platform-specific code generation */
void *jit_generate_x86_64(void *bytecode, size_t size, JitOptFlags flags);
void *jit_generate_arm64(void *bytecode, size_t size, JitOptFlags flags);

/* Global JIT compiler instance */
extern JitCompiler *global_jit;

#endif /* RUBOLT_JIT_COMPILER_H */
