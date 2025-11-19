#ifndef RUBOLT_JIT_ENGINE_H
#define RUBOLT_JIT_ENGINE_H

#include "ast.h"
#include "inline_cache.h"
#include <stdint.h>

typedef enum {
    JIT_OP_LOAD_CONST,
    JIT_OP_LOAD_VAR,
    JIT_OP_STORE_VAR,
    JIT_OP_LOAD_STRING,
    JIT_OP_ADD,
    JIT_OP_SUB,
    JIT_OP_MUL,
    JIT_OP_DIV,
    JIT_OP_NEG,
    JIT_OP_NOT,
    JIT_OP_SHIFT_LEFT,
    JIT_OP_CALL,
    JIT_OP_RETURN,
    JIT_OP_JUMP,
    JIT_OP_JUMP_IF_FALSE,
    JIT_OP_COMPARE_EQ,
    JIT_OP_COMPARE_LT,
    JIT_OP_COMPARE_GT,
    JIT_OP_PRINT
} JitOpcode;

typedef struct {
    JitOpcode opcode;
    union {
        int64_t int_operand;
        double float_operand;
        void* ptr_operand;
    } operand;
} JitInstruction;

typedef struct {
    JitInstruction* instructions;
    size_t instruction_count;
    size_t capacity;
    void* native_code;
    size_t native_size;
    int execution_count;
    double total_time;
} JitFunction;

typedef struct {
    uint8_t* memory;
    size_t size;
    size_t capacity;
    bool executable;
} JitCodeBuffer;

typedef struct {
    JitFunction* functions;
    size_t function_count;
    JitCodeBuffer code_buffer;
    InlineCache* caches;
    size_t cache_count;
} JitCompiler;

// JIT compiler operations
void jit_compiler_init(JitCompiler* compiler);
void jit_compiler_free(JitCompiler* compiler);

// Code buffer operations
void jit_buffer_init(JitCodeBuffer* buffer, size_t initial_size);
void jit_buffer_free(JitCodeBuffer* buffer);
void jit_buffer_ensure_capacity(JitCodeBuffer* buffer, size_t needed);
void jit_buffer_make_executable(JitCodeBuffer* buffer);

// JIT function operations
JitFunction* jit_function_create();
void jit_function_free(JitFunction* func);
void jit_function_add_instruction(JitFunction* func, JitOpcode opcode, int64_t operand);
void jit_function_compile(JitFunction* func, JitCodeBuffer* buffer);

// Bytecode generation from AST
JitFunction* compile_function_to_jit(FunctionStmt* func_stmt);
void compile_stmt_to_jit(Stmt* stmt, JitFunction* jit_func);
void compile_expr_to_jit(Expr* expr, JitFunction* jit_func);

// Native code generation (x86-64)
void emit_x86_prologue(JitCodeBuffer* buffer);
void emit_x86_epilogue(JitCodeBuffer* buffer);
void emit_x86_load_immediate(JitCodeBuffer* buffer, int reg, int64_t value);
void emit_x86_add_reg_reg(JitCodeBuffer* buffer, int dst, int src);
void emit_x86_sub_reg_reg(JitCodeBuffer* buffer, int dst, int src);
void emit_x86_mul_reg_reg(JitCodeBuffer* buffer, int dst, int src);
void emit_x86_div_reg_reg(JitCodeBuffer* buffer, int dst, int src);
void emit_x86_call(JitCodeBuffer* buffer, void* target);
void emit_x86_return(JitCodeBuffer* buffer);

// Additional x86 instruction emission
void emit_x86_call(JitCodeBuffer* buffer, void* target);
void emit_x86_jump(JitCodeBuffer* buffer, size_t target);
void emit_x86_test_rax(JitCodeBuffer* buffer);
void emit_x86_jump_if_zero(JitCodeBuffer* buffer, size_t target);
void emit_x86_compare_reg_reg(JitCodeBuffer* buffer, int reg1, int reg2);
void emit_x86_set_equal(JitCodeBuffer* buffer, int reg);
void emit_x86_set_less(JitCodeBuffer* buffer, int reg);
void emit_x86_set_greater(JitCodeBuffer* buffer, int reg);
void emit_x86_neg_reg(JitCodeBuffer* buffer, int reg);
void emit_x86_not_reg(JitCodeBuffer* buffer, int reg);
void emit_x86_shift_left_reg(JitCodeBuffer* buffer, int reg, int amount);

// String constant management
int64_t add_string_constant(JitFunction* func, const char* str);
void runtime_print_value(Value value);

// Advanced optimization functions
void constant_folding_advanced(JitFunction* func);
void strength_reduction(JitFunction* func);
void loop_optimization(JitFunction* func);
void optimize_loop_body(JitFunction* func, size_t start, size_t end);
bool is_loop_invariant(JitFunction* func, JitInstruction* instr, size_t start, size_t end);
void move_instruction_before_loop(JitFunction* func, size_t instr_pos, size_t loop_start);

// Hot path detection
bool is_hot_path(JitFunction* func);
void update_execution_stats(JitFunction* func, double execution_time);

// JIT execution
typedef Value (*JitNativeFunction)(Value* args, size_t arg_count);
Value execute_jit_function(JitFunction* func, Value* args, size_t arg_count);

// Optimization passes
void optimize_jit_function(JitFunction* func);
void dead_code_elimination(JitFunction* func);
void constant_folding(JitFunction* func);
void inline_expansion(JitFunction* func, JitCompiler* compiler);

#endif