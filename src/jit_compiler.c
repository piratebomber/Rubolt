#include "jit_compiler.h"
#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// x86-64 instruction encoding
#define REX_W 0x48
#define MOV_REG_IMM 0xB8
#define ADD_REG_REG 0x01
#define SUB_REG_REG 0x29
#define MUL_REG 0xF7
#define RET 0xC3
#define PUSH_REG 0x50
#define POP_REG 0x58
#define CALL_REL32 0xE8

// Register encoding
#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7

typedef struct CodeBuffer {
    uint8_t *code;
    size_t size;
    size_t capacity;
    size_t position;
} CodeBuffer;

typedef struct JITContext {
    CodeBuffer buffer;
    Function *function;
    size_t *label_positions;
    size_t label_count;
} JITContext;

// JIT compiler state
static JITCompiler *global_jit = NULL;

JITCompiler *jit_create(void) {
    JITCompiler *jit = malloc(sizeof(JITCompiler));
    jit->code_cache = malloc(sizeof(CompiledFunction) * 1024);
    jit->cache_size = 0;
    jit->cache_capacity = 1024;
    jit->total_compilations = 0;
    jit->total_compile_time = 0;
    
    // Allocate executable memory
    jit->executable_memory = mmap(NULL, JIT_CODE_SIZE, 
                                  PROT_READ | PROT_WRITE | PROT_EXEC,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    jit->memory_position = 0;
    
    global_jit = jit;
    return jit;
}

void jit_destroy(JITCompiler *jit) {
    munmap(jit->executable_memory, JIT_CODE_SIZE);
    free(jit->code_cache);
    free(jit);
}

// Code buffer operations
static void buffer_init(CodeBuffer *buffer) {
    buffer->capacity = 4096;
    buffer->code = malloc(buffer->capacity);
    buffer->size = 0;
    buffer->position = 0;
}

static void buffer_emit_byte(CodeBuffer *buffer, uint8_t byte) {
    if (buffer->position >= buffer->capacity) {
        buffer->capacity *= 2;
        buffer->code = realloc(buffer->code, buffer->capacity);
    }
    buffer->code[buffer->position++] = byte;
}

static void buffer_emit_int32(CodeBuffer *buffer, int32_t value) {
    buffer_emit_byte(buffer, value & 0xFF);
    buffer_emit_byte(buffer, (value >> 8) & 0xFF);
    buffer_emit_byte(buffer, (value >> 16) & 0xFF);
    buffer_emit_byte(buffer, (value >> 24) & 0xFF);
}

static void buffer_emit_int64(CodeBuffer *buffer, int64_t value) {
    for (int i = 0; i < 8; i++) {
        buffer_emit_byte(buffer, (value >> (i * 8)) & 0xFF);
    }
}

// x86-64 instruction emission
static void emit_mov_reg_imm64(CodeBuffer *buffer, int reg, int64_t imm) {
    buffer_emit_byte(buffer, REX_W);
    buffer_emit_byte(buffer, MOV_REG_IMM + reg);
    buffer_emit_int64(buffer, imm);
}

static void emit_mov_reg_reg(CodeBuffer *buffer, int dst, int src) {
    buffer_emit_byte(buffer, REX_W);
    buffer_emit_byte(buffer, 0x89);
    buffer_emit_byte(buffer, 0xC0 | (src << 3) | dst);
}

static void emit_add_reg_reg(CodeBuffer *buffer, int dst, int src) {
    buffer_emit_byte(buffer, REX_W);
    buffer_emit_byte(buffer, ADD_REG_REG);
    buffer_emit_byte(buffer, 0xC0 | (src << 3) | dst);
}

static void emit_sub_reg_reg(CodeBuffer *buffer, int dst, int src) {
    buffer_emit_byte(buffer, REX_W);
    buffer_emit_byte(buffer, SUB_REG_REG);
    buffer_emit_byte(buffer, 0xC0 | (src << 3) | dst);
}

static void emit_push_reg(CodeBuffer *buffer, int reg) {
    buffer_emit_byte(buffer, PUSH_REG + reg);
}

static void emit_pop_reg(CodeBuffer *buffer, int reg) {
    buffer_emit_byte(buffer, POP_REG + reg);
}

static void emit_ret(CodeBuffer *buffer) {
    buffer_emit_byte(buffer, RET);
}

static void emit_call_rel32(CodeBuffer *buffer, int32_t offset) {
    buffer_emit_byte(buffer, CALL_REL32);
    buffer_emit_int32(buffer, offset);
}

// Expression compilation
static void compile_expression(JITContext *ctx, Expr *expr) {
    switch (expr->type) {
        case EXPR_LITERAL:
            compile_literal(ctx, &expr->as.literal);
            break;
            
        case EXPR_BINARY:
            compile_binary(ctx, &expr->as.binary);
            break;
            
        case EXPR_UNARY:
            compile_unary(ctx, &expr->as.unary);
            break;
            
        case EXPR_CALL:
            compile_call(ctx, &expr->as.call);
            break;
            
        case EXPR_IDENTIFIER:
            compile_identifier(ctx, &expr->as.identifier);
            break;
            
        default:
            // Fallback to interpreter
            emit_call_interpreter(ctx, expr);
            break;
    }
}

static void compile_literal(JITContext *ctx, LiteralExpr *expr) {
    switch (expr->value_type) {
        case VALUE_NUMBER: {
            // Load number into RAX
            union { double d; int64_t i; } converter;
            converter.d = expr->as.number;
            emit_mov_reg_imm64(&ctx->buffer, RAX, converter.i);
            break;
        }
        
        case VALUE_BOOL:
            emit_mov_reg_imm64(&ctx->buffer, RAX, expr->as.boolean ? 1 : 0);
            break;
            
        case VALUE_NULL:
            emit_mov_reg_imm64(&ctx->buffer, RAX, 0);
            break;
            
        default:
            // Complex literals need interpreter support
            emit_call_interpreter(ctx, (Expr *)expr);
            break;
    }
}

static void compile_binary(JITContext *ctx, BinaryExpr *expr) {
    // Compile left operand (result in RAX)
    compile_expression(ctx, expr->left);
    emit_push_reg(&ctx->buffer, RAX);
    
    // Compile right operand (result in RAX)
    compile_expression(ctx, expr->right);
    emit_mov_reg_reg(&ctx->buffer, RCX, RAX);
    
    // Pop left operand into RAX
    emit_pop_reg(&ctx->buffer, RAX);
    
    // Perform operation
    if (strcmp(expr->operator, "+") == 0) {
        emit_add_reg_reg(&ctx->buffer, RAX, RCX);
    } else if (strcmp(expr->operator, "-") == 0) {
        emit_sub_reg_reg(&ctx->buffer, RAX, RCX);
    } else if (strcmp(expr->operator, "*") == 0) {
        // Multiplication is more complex, use interpreter
        emit_call_interpreter(ctx, (Expr *)expr);
    } else {
        // Other operations use interpreter
        emit_call_interpreter(ctx, (Expr *)expr);
    }
}

static void compile_unary(JITContext *ctx, UnaryExpr *expr) {
    compile_expression(ctx, expr->operand);
    
    if (strcmp(expr->operator, "-") == 0) {
        // Negate RAX
        buffer_emit_byte(&ctx->buffer, REX_W);
        buffer_emit_byte(&ctx->buffer, 0xF7);
        buffer_emit_byte(&ctx->buffer, 0xD8); // NEG RAX
    } else {
        // Other unary operations use interpreter
        emit_call_interpreter(ctx, (Expr *)expr);
    }
}

static void compile_call(JITContext *ctx, CallExpr *expr) {
    // Function calls are complex, use interpreter for now
    emit_call_interpreter(ctx, (Expr *)expr);
}

static void compile_identifier(JITContext *ctx, IdentifierExpr *expr) {
    // Variable access needs environment lookup, use interpreter
    emit_call_interpreter(ctx, (Expr *)expr);
}

static void emit_call_interpreter(JITContext *ctx, Expr *expr) {
    // Save registers
    emit_push_reg(&ctx->buffer, RDI);
    emit_push_reg(&ctx->buffer, RSI);
    
    // Set up arguments for interpreter call
    emit_mov_reg_imm64(&ctx->buffer, RDI, (int64_t)current_interpreter);
    emit_mov_reg_imm64(&ctx->buffer, RSI, (int64_t)expr);
    
    // Call interpreter
    int32_t offset = (int32_t)((uint8_t *)evaluate_expression - 
                               (ctx->buffer.code + ctx->buffer.position + 5));
    emit_call_rel32(&ctx->buffer, offset);
    
    // Restore registers
    emit_pop_reg(&ctx->buffer, RSI);
    emit_pop_reg(&ctx->buffer, RDI);
}

// Statement compilation
static void compile_statement(JITContext *ctx, Stmt *stmt) {
    switch (stmt->type) {
        case STMT_EXPRESSION:
            compile_expression(ctx, stmt->as.expression.expr);
            break;
            
        case STMT_RETURN:
            if (stmt->as.return_stmt.value) {
                compile_expression(ctx, stmt->as.return_stmt.value);
            } else {
                emit_mov_reg_imm64(&ctx->buffer, RAX, 0);
            }
            emit_ret(&ctx->buffer);
            break;
            
        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->as.block.stmt_count; i++) {
                compile_statement(ctx, stmt->as.block.statements[i]);
            }
            break;
            
        default:
            // Complex statements use interpreter
            emit_call_interpreter_stmt(ctx, stmt);
            break;
    }
}

static void emit_call_interpreter_stmt(JITContext *ctx, Stmt *stmt) {
    // Save registers
    emit_push_reg(&ctx->buffer, RDI);
    emit_push_reg(&ctx->buffer, RSI);
    
    // Set up arguments
    emit_mov_reg_imm64(&ctx->buffer, RDI, (int64_t)current_interpreter);
    emit_mov_reg_imm64(&ctx->buffer, RSI, (int64_t)stmt);
    
    // Call interpreter
    int32_t offset = (int32_t)((uint8_t *)execute_statement - 
                               (ctx->buffer.code + ctx->buffer.position + 5));
    emit_call_rel32(&ctx->buffer, offset);
    
    // Restore registers
    emit_pop_reg(&ctx->buffer, RSI);
    emit_pop_reg(&ctx->buffer, RDI);
}

// Function compilation
CompiledFunction *jit_compile_function(JITCompiler *jit, Function *func) {
    clock_t start_time = clock();
    
    JITContext ctx;
    buffer_init(&ctx.buffer);
    ctx.function = func;
    ctx.label_positions = malloc(sizeof(size_t) * 64);
    ctx.label_count = 0;
    
    // Function prologue
    emit_push_reg(&ctx.buffer, RBP);
    emit_mov_reg_reg(&ctx.buffer, RBP, RSP);
    
    // Compile function body
    compile_statement(&ctx, func->body);
    
    // Function epilogue (if no explicit return)
    emit_mov_reg_imm64(&ctx.buffer, RAX, 0); // Default return value
    emit_mov_reg_reg(&ctx.buffer, RSP, RBP);
    emit_pop_reg(&ctx.buffer, RBP);
    emit_ret(&ctx.buffer);
    
    // Copy code to executable memory
    if (jit->memory_position + ctx.buffer.position > JIT_CODE_SIZE) {
        // Out of memory, return NULL
        free(ctx.buffer.code);
        free(ctx.label_positions);
        return NULL;
    }
    
    void *code_ptr = (uint8_t *)jit->executable_memory + jit->memory_position;
    memcpy(code_ptr, ctx.buffer.code, ctx.buffer.position);
    jit->memory_position += ctx.buffer.position;
    
    // Create compiled function entry
    CompiledFunction *compiled = &jit->code_cache[jit->cache_size++];
    compiled->function = func;
    compiled->native_code = code_ptr;
    compiled->code_size = ctx.buffer.position;
    compiled->call_count = 0;
    
    // Update function
    func->jit_compiled = true;
    func->native_code = code_ptr;
    
    // Update statistics
    jit->total_compilations++;
    jit->total_compile_time += clock() - start_time;
    
    // Cleanup
    free(ctx.buffer.code);
    free(ctx.label_positions);
    
    return compiled;
}

// JIT function call
Value jit_call_function(Interpreter *interp, Function *func, Value *args, size_t arg_count) {
    if (!func->jit_compiled) {
        CompiledFunction *compiled = jit_compile_function(global_jit, func);
        if (!compiled) {
            // Compilation failed, fall back to interpreter
            return call_function_interpreted(interp, func, args, arg_count);
        }
    }
    
    // Call native code
    typedef Value (*NativeFunction)(Interpreter *, Value *, size_t);
    NativeFunction native_func = (NativeFunction)func->native_code;
    
    return native_func(interp, args, arg_count);
}

// Optimization passes
void jit_optimize_function(JITCompiler *jit, Function *func) {
    // Dead code elimination
    eliminate_dead_code(func);
    
    // Constant folding
    fold_constants(func);
    
    // Loop unrolling
    unroll_loops(func);
    
    // Inline small functions
    inline_functions(func);
}

static void eliminate_dead_code(Function *func) {
    // Mark reachable statements
    bool *reachable = malloc(sizeof(bool) * count_statements(func->body));
    memset(reachable, false, sizeof(bool) * count_statements(func->body));
    
    mark_reachable_statements(func->body, reachable, 0);
    
    // Remove unreachable statements
    remove_unreachable_statements(func->body, reachable);
    
    free(reachable);
}

static void fold_constants(Function *func) {
    fold_constants_in_statement(func->body);
}

static void fold_constants_in_statement(Stmt *stmt) {
    switch (stmt->type) {
        case STMT_EXPRESSION:
            fold_constants_in_expression(stmt->as.expression.expr);
            break;
            
        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->as.block.stmt_count; i++) {
                fold_constants_in_statement(stmt->as.block.statements[i]);
            }
            break;
            
        case STMT_IF:
            fold_constants_in_expression(stmt->as.if_stmt.condition);
            fold_constants_in_statement(stmt->as.if_stmt.then_branch);
            if (stmt->as.if_stmt.else_branch) {
                fold_constants_in_statement(stmt->as.if_stmt.else_branch);
            }
            break;
            
        default:
            break;
    }
}

static void fold_constants_in_expression(Expr *expr) {
    switch (expr->type) {
        case EXPR_BINARY: {
            BinaryExpr *binary = &expr->as.binary;
            fold_constants_in_expression(binary->left);
            fold_constants_in_expression(binary->right);
            
            // Try to fold if both operands are literals
            if (binary->left->type == EXPR_LITERAL && binary->right->type == EXPR_LITERAL) {
                LiteralExpr *left = &binary->left->as.literal;
                LiteralExpr *right = &binary->right->as.literal;
                
                if (left->value_type == VALUE_NUMBER && right->value_type == VALUE_NUMBER) {
                    double result = 0;
                    bool can_fold = true;
                    
                    if (strcmp(binary->operator, "+") == 0) {
                        result = left->as.number + right->as.number;
                    } else if (strcmp(binary->operator, "-") == 0) {
                        result = left->as.number - right->as.number;
                    } else if (strcmp(binary->operator, "*") == 0) {
                        result = left->as.number * right->as.number;
                    } else if (strcmp(binary->operator, "/") == 0) {
                        if (right->as.number != 0) {
                            result = left->as.number / right->as.number;
                        } else {
                            can_fold = false;
                        }
                    } else {
                        can_fold = false;
                    }
                    
                    if (can_fold) {
                        // Replace binary expression with literal
                        expr->type = EXPR_LITERAL;
                        expr->as.literal.value_type = VALUE_NUMBER;
                        expr->as.literal.as.number = result;
                    }
                }
            }
            break;
        }
        
        case EXPR_UNARY: {
            UnaryExpr *unary = &expr->as.unary;
            fold_constants_in_expression(unary->operand);
            
            if (unary->operand->type == EXPR_LITERAL) {
                LiteralExpr *operand = &unary->operand->as.literal;
                
                if (strcmp(unary->operator, "-") == 0 && operand->value_type == VALUE_NUMBER) {
                    expr->type = EXPR_LITERAL;
                    expr->as.literal.value_type = VALUE_NUMBER;
                    expr->as.literal.as.number = -operand->as.number;
                }
            }
            break;
        }
        
        case EXPR_CALL: {
            CallExpr *call = &expr->as.call;
            for (size_t i = 0; i < call->arg_count; i++) {
                fold_constants_in_expression(call->args[i]);
            }
            break;
        }
        
        default:
            break;
    }
}

static void unroll_loops(Function *func) {
    // Simple loop unrolling for small loops
    unroll_loops_in_statement(func->body);
}

static void unroll_loops_in_statement(Stmt *stmt) {
    switch (stmt->type) {
        case STMT_WHILE: {
            WhileStmt *while_stmt = &stmt->as.while_stmt;
            
            // Check if it's a simple counting loop
            if (is_simple_counting_loop(while_stmt)) {
                int loop_count = get_loop_count(while_stmt);
                if (loop_count > 0 && loop_count <= MAX_UNROLL_COUNT) {
                    unroll_while_loop(while_stmt, loop_count);
                }
            }
            break;
        }
        
        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->as.block.stmt_count; i++) {
                unroll_loops_in_statement(stmt->as.block.statements[i]);
            }
            break;
            
        default:
            break;
    }
}

static void inline_functions(Function *func) {
    // Inline small functions to reduce call overhead
    inline_functions_in_statement(func->body);
}

static void inline_functions_in_statement(Stmt *stmt) {
    switch (stmt->type) {
        case STMT_EXPRESSION:
            inline_functions_in_expression(stmt->as.expression.expr);
            break;
            
        case STMT_BLOCK:
            for (size_t i = 0; i < stmt->as.block.stmt_count; i++) {
                inline_functions_in_statement(stmt->as.block.statements[i]);
            }
            break;
            
        default:
            break;
    }
}

static void inline_functions_in_expression(Expr *expr) {
    switch (expr->type) {
        case EXPR_CALL: {
            CallExpr *call = &expr->as.call;
            
            // Check if function is small enough to inline
            if (call->callee->type == EXPR_IDENTIFIER) {
                Function *func = lookup_function(call->callee->as.identifier.name);
                if (func && is_inlinable(func)) {
                    inline_function_call(expr, func, call);
                }
            }
            break;
        }
        
        case EXPR_BINARY:
            inline_functions_in_expression(expr->as.binary.left);
            inline_functions_in_expression(expr->as.binary.right);
            break;
            
        case EXPR_UNARY:
            inline_functions_in_expression(expr->as.unary.operand);
            break;
            
        default:
            break;
    }
}

// JIT statistics and profiling
void jit_print_statistics(JITCompiler *jit) {
    printf("JIT Compiler Statistics:\n");
    printf("  Total compilations: %zu\n", jit->total_compilations);
    printf("  Total compile time: %f ms\n", 
           (double)jit->total_compile_time / CLOCKS_PER_SEC * 1000);
    printf("  Average compile time: %f ms\n", 
           (double)jit->total_compile_time / jit->total_compilations / CLOCKS_PER_SEC * 1000);
    printf("  Code cache size: %zu/%zu\n", jit->cache_size, jit->cache_capacity);
    printf("  Memory usage: %zu/%d bytes\n", jit->memory_position, JIT_CODE_SIZE);
    
    printf("\nCompiled functions:\n");
    for (size_t i = 0; i < jit->cache_size; i++) {
        CompiledFunction *func = &jit->code_cache[i];
        printf("  %s: %zu calls, %zu bytes\n", 
               func->function->name, func->call_count, func->code_size);
    }
}

// Hot path detection
bool should_compile_function(Function *func) {
    return func->call_count >= JIT_THRESHOLD && !func->jit_compiled;
}

void update_function_profile(Function *func) {
    func->call_count++;
    
    if (should_compile_function(func)) {
        jit_compile_function(global_jit, func);
    }
}

// Deoptimization support
void jit_deoptimize_function(Function *func) {
    if (func->jit_compiled) {
        func->jit_compiled = false;
        func->native_code = NULL;
        
        // Remove from cache
        for (size_t i = 0; i < global_jit->cache_size; i++) {
            if (global_jit->code_cache[i].function == func) {
                // Shift remaining entries
                memmove(&global_jit->code_cache[i], 
                        &global_jit->code_cache[i + 1],
                        (global_jit->cache_size - i - 1) * sizeof(CompiledFunction));
                global_jit->cache_size--;
                break;
            }
        }
    }
}

// Helper functions for interpreter integration
Value call_function_interpreted(Interpreter *interp, Function *func, Value *args, size_t arg_count) {
    // Create new environment for function
    Environment *func_env = environment_create(interp->current_env);
    
    // Bind parameters
    for (size_t i = 0; i < func->param_count && i < arg_count; i++) {
        environment_define(func_env, func->params[i].name, args[i]);
    }
    
    // Execute function body
    Environment *prev_env = interp->current_env;
    interp->current_env = func_env;
    
    Value result = execute_statement(interp, func->body);
    
    interp->current_env = prev_env;
    
    return result;
}