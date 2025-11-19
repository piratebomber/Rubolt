#include "jit_engine.h"
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#define JIT_THRESHOLD 10
#define INITIAL_BUFFER_SIZE 4096

void jit_compiler_init(JitCompiler* compiler) {
    compiler->functions = NULL;
    compiler->function_count = 0;
    jit_buffer_init(&compiler->code_buffer, INITIAL_BUFFER_SIZE);
    compiler->caches = NULL;
    compiler->cache_count = 0;
}

void jit_compiler_free(JitCompiler* compiler) {
    for (size_t i = 0; i < compiler->function_count; i++) {
        jit_function_free(&compiler->functions[i]);
    }
    free(compiler->functions);
    jit_buffer_free(&compiler->code_buffer);
    free(compiler->caches);
}

void jit_buffer_init(JitCodeBuffer* buffer, size_t initial_size) {
    buffer->capacity = initial_size;
    buffer->size = 0;
    buffer->executable = false;
    buffer->memory = mmap(NULL, buffer->capacity, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void jit_buffer_free(JitCodeBuffer* buffer) {
    if (buffer->memory) {
        munmap(buffer->memory, buffer->capacity);
    }
}

void jit_buffer_ensure_capacity(JitCodeBuffer* buffer, size_t needed) {
    if (buffer->size + needed > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        while (new_capacity < buffer->size + needed) {
            new_capacity *= 2;
        }
        
        uint8_t* new_memory = mmap(NULL, new_capacity, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        memcpy(new_memory, buffer->memory, buffer->size);
        munmap(buffer->memory, buffer->capacity);
        
        buffer->memory = new_memory;
        buffer->capacity = new_capacity;
    }
}

void jit_buffer_make_executable(JitCodeBuffer* buffer) {
    if (!buffer->executable) {
        mprotect(buffer->memory, buffer->capacity, PROT_READ | PROT_EXEC);
        buffer->executable = true;
    }
}

JitFunction* jit_function_create() {
    JitFunction* func = malloc(sizeof(JitFunction));
    func->instructions = NULL;
    func->instruction_count = 0;
    func->capacity = 0;
    func->native_code = NULL;
    func->native_size = 0;
    func->execution_count = 0;
    func->total_time = 0.0;
    return func;
}

void jit_function_free(JitFunction* func) {
    free(func->instructions);
    free(func);
}

void jit_function_add_instruction(JitFunction* func, JitOpcode opcode, int64_t operand) {
    if (func->instruction_count >= func->capacity) {
        func->capacity = func->capacity == 0 ? 16 : func->capacity * 2;
        func->instructions = realloc(func->instructions, 
                                   func->capacity * sizeof(JitInstruction));
    }
    
    func->instructions[func->instruction_count].opcode = opcode;
    func->instructions[func->instruction_count].operand.int_operand = operand;
    func->instruction_count++;
}

// x86-64 instruction encoding
void emit_x86_prologue(JitCodeBuffer* buffer) {
    jit_buffer_ensure_capacity(buffer, 16);
    
    // push rbp
    buffer->memory[buffer->size++] = 0x55;
    
    // mov rbp, rsp
    buffer->memory[buffer->size++] = 0x48;
    buffer->memory[buffer->size++] = 0x89;
    buffer->memory[buffer->size++] = 0xe5;
}

void emit_x86_epilogue(JitCodeBuffer* buffer) {
    jit_buffer_ensure_capacity(buffer, 16);
    
    // pop rbp
    buffer->memory[buffer->size++] = 0x5d;
    
    // ret
    buffer->memory[buffer->size++] = 0xc3;
}

void emit_x86_load_immediate(JitCodeBuffer* buffer, int reg, int64_t value) {
    jit_buffer_ensure_capacity(buffer, 16);
    
    // mov reg, immediate (64-bit)
    buffer->memory[buffer->size++] = 0x48 | ((reg & 8) >> 3);
    buffer->memory[buffer->size++] = 0xb8 | (reg & 7);
    
    // 64-bit immediate value
    memcpy(&buffer->memory[buffer->size], &value, 8);
    buffer->size += 8;
}

void emit_x86_add_reg_reg(JitCodeBuffer* buffer, int dst, int src) {
    jit_buffer_ensure_capacity(buffer, 8);
    
    // add dst, src
    buffer->memory[buffer->size++] = 0x48;
    buffer->memory[buffer->size++] = 0x01;
    buffer->memory[buffer->size++] = 0xc0 | ((src & 7) << 3) | (dst & 7);
}

void emit_x86_sub_reg_reg(JitCodeBuffer* buffer, int dst, int src) {
    jit_buffer_ensure_capacity(buffer, 8);
    
    // sub dst, src
    buffer->memory[buffer->size++] = 0x48;
    buffer->memory[buffer->size++] = 0x29;
    buffer->memory[buffer->size++] = 0xc0 | ((src & 7) << 3) | (dst & 7);
}

void emit_x86_mul_reg_reg(JitCodeBuffer* buffer, int dst, int src) {
    jit_buffer_ensure_capacity(buffer, 8);
    
    // imul dst, src
    buffer->memory[buffer->size++] = 0x48;
    buffer->memory[buffer->size++] = 0x0f;
    buffer->memory[buffer->size++] = 0xaf;
    buffer->memory[buffer->size++] = 0xc0 | ((dst & 7) << 3) | (src & 7);
}

void emit_x86_return(JitCodeBuffer* buffer) {
    jit_buffer_ensure_capacity(buffer, 4);
    buffer->memory[buffer->size++] = 0xc3; // ret
}

void jit_function_compile(JitFunction* func, JitCodeBuffer* buffer) {
    size_t start_offset = buffer->size;
    
    emit_x86_prologue(buffer);
    
    // Simple register allocation - use RAX for primary operations
    int reg_rax = 0;
    int reg_rbx = 3;
    
    for (size_t i = 0; i < func->instruction_count; i++) {
        JitInstruction* instr = &func->instructions[i];
        
        switch (instr->opcode) {
            case JIT_OP_LOAD_CONST:
                emit_x86_load_immediate(buffer, reg_rax, instr->operand.int_operand);
                break;
                
            case JIT_OP_ADD:
                // Assume second operand is in RBX
                emit_x86_add_reg_reg(buffer, reg_rax, reg_rbx);
                break;
                
            case JIT_OP_SUB:
                emit_x86_sub_reg_reg(buffer, reg_rax, reg_rbx);
                break;
                
            case JIT_OP_MUL:
                emit_x86_mul_reg_reg(buffer, reg_rax, reg_rbx);
                break;
                
            case JIT_OP_RETURN:
                emit_x86_epilogue(buffer);
                break;
                
            case JIT_OP_LOAD_VAR: {
                // Load variable from environment (simplified)
                emit_x86_load_immediate(buffer, reg_rax, 0); // Placeholder
                break;
            }
            
            case JIT_OP_STORE_VAR: {
                // Store variable to environment (simplified)
                jit_buffer_ensure_capacity(buffer, 1);
                buffer->memory[buffer->size++] = 0x90; // nop placeholder
                break;
            }
            
            case JIT_OP_CALL: {
                // Function call (simplified)
                emit_x86_call(buffer, (void*)0); // Placeholder
                break;
            }
            
            case JIT_OP_JUMP: {
                // Unconditional jump
                emit_x86_jump(buffer, instr->operand.int_operand);
                break;
            }
            
            case JIT_OP_JUMP_IF_FALSE: {
                // Conditional jump
                emit_x86_test_rax(buffer);
                emit_x86_jump_if_zero(buffer, instr->operand.int_operand);
                break;
            }
            
            case JIT_OP_COMPARE_EQ: {
                emit_x86_compare_reg_reg(buffer, reg_rax, reg_rbx);
                emit_x86_set_equal(buffer, reg_rax);
                break;
            }
            
            case JIT_OP_COMPARE_LT: {
                emit_x86_compare_reg_reg(buffer, reg_rax, reg_rbx);
                emit_x86_set_less(buffer, reg_rax);
                break;
            }
            
            case JIT_OP_COMPARE_GT: {
                emit_x86_compare_reg_reg(buffer, reg_rax, reg_rbx);
                emit_x86_set_greater(buffer, reg_rax);
                break;
            }
            
            case JIT_OP_NEG: {
                emit_x86_neg_reg(buffer, reg_rax);
                break;
            }
            
            case JIT_OP_NOT: {
                emit_x86_not_reg(buffer, reg_rax);
                break;
            }
            
            case JIT_OP_PRINT: {
                // Call print function
                emit_x86_call(buffer, (void*)runtime_print_value);
                break;
            }
            
            case JIT_OP_LOAD_STRING: {
                // Load string constant
                emit_x86_load_immediate(buffer, reg_rax, instr->operand.int_operand);
                break;
            }
            
            case JIT_OP_SHIFT_LEFT: {
                emit_x86_shift_left_reg(buffer, reg_rax, instr->operand.int_operand);
                break;
            }
            
            default:
                runtime_panic_with_type(PANIC_INVALID_OPERATION, 
                                       "Unknown JIT instruction: %d", instr->opcode);
                break;
        }
    }
    
    // Ensure function ends with return
    if (func->instruction_count == 0 || 
        func->instructions[func->instruction_count - 1].opcode != JIT_OP_RETURN) {
        emit_x86_epilogue(buffer);
    }
    
    func->native_code = &buffer->memory[start_offset];
    func->native_size = buffer->size - start_offset;
}

JitFunction* compile_function_to_jit(FunctionStmt* func_stmt) {
    JitFunction* jit_func = jit_function_create();
    
    // Compile function body to JIT instructions
    for (size_t i = 0; i < func_stmt->body_count; i++) {
        compile_stmt_to_jit(func_stmt->body[i], jit_func);
    }
    
    // Add return if not present
    if (jit_func->instruction_count == 0 ||
        jit_func->instructions[jit_func->instruction_count - 1].opcode != JIT_OP_RETURN) {
        jit_function_add_instruction(jit_func, JIT_OP_RETURN, 0);
    }
    
    return jit_func;
}

void compile_stmt_to_jit(Stmt* stmt, JitFunction* jit_func) {
    switch (stmt->type) {
        case STMT_RETURN:
            if (stmt->as.return_stmt.value) {
                compile_expr_to_jit(stmt->as.return_stmt.value, jit_func);
            }
            jit_function_add_instruction(jit_func, JIT_OP_RETURN, 0);
            break;
            
        case STMT_EXPR:
            compile_expr_to_jit(stmt->as.expression, jit_func);
            break;
            
        default:
            // Other statements not yet implemented in JIT
            break;
    }
}

void compile_expr_to_jit(Expr* expr, JitFunction* jit_func) {
    switch (expr->type) {
        case EXPR_NUMBER: {
            // Convert double to int64 for simplicity
            int64_t int_value = (int64_t)expr->as.number;
            jit_function_add_instruction(jit_func, JIT_OP_LOAD_CONST, int_value);
            break;
        }
        
        case EXPR_BINARY: {
            // Compile left operand
            compile_expr_to_jit(expr->as.binary.left, jit_func);
            
            // Compile right operand (would need proper register allocation)
            compile_expr_to_jit(expr->as.binary.right, jit_func);
            
            // Emit operation
            if (strcmp(expr->as.binary.op, "+") == 0) {
                jit_function_add_instruction(jit_func, JIT_OP_ADD, 0);
            } else if (strcmp(expr->as.binary.op, "-") == 0) {
                jit_function_add_instruction(jit_func, JIT_OP_SUB, 0);
            } else if (strcmp(expr->as.binary.op, "*") == 0) {
                jit_function_add_instruction(jit_func, JIT_OP_MUL, 0);
            } else if (strcmp(expr->as.binary.op, "/") == 0) {
                jit_function_add_instruction(jit_func, JIT_OP_DIV, 0);
            }
            break;
        }
        
        case EXPR_IDENTIFIER: {
            // Load variable from environment
            jit_function_add_instruction(jit_func, JIT_OP_LOAD_VAR, 
                                        (int64_t)strdup(expr->as.identifier));
            break;
        }
        
        case EXPR_CALL: {
            // Compile arguments first
            for (size_t i = 0; i < expr->as.call.arg_count; i++) {
                compile_expr_to_jit(expr->as.call.args[i], jit_func);
            }
            
            // Compile callee
            compile_expr_to_jit(expr->as.call.callee, jit_func);
            
            // Emit call instruction
            jit_function_add_instruction(jit_func, JIT_OP_CALL, expr->as.call.arg_count);
            break;
        }
        
        case EXPR_ASSIGN: {
            // Compile value expression
            compile_expr_to_jit(expr->as.assign.value, jit_func);
            
            // Store to variable
            jit_function_add_instruction(jit_func, JIT_OP_STORE_VAR,
                                        (int64_t)strdup(expr->as.assign.name));
            break;
        }
        
        case EXPR_UNARY: {
            // Compile operand
            compile_expr_to_jit(expr->as.unary.operand, jit_func);
            
            // Emit unary operation
            if (strcmp(expr->as.unary.op, "-") == 0) {
                jit_function_add_instruction(jit_func, JIT_OP_NEG, 0);
            } else if (strcmp(expr->as.unary.op, "!") == 0) {
                jit_function_add_instruction(jit_func, JIT_OP_NOT, 0);
            }
            break;
        }
        
        case EXPR_BOOL:
            jit_function_add_instruction(jit_func, JIT_OP_LOAD_CONST, expr->as.boolean ? 1 : 0);
            break;
            
        case EXPR_NULL:
            jit_function_add_instruction(jit_func, JIT_OP_LOAD_CONST, 0);
            break;
            
        case EXPR_STRING: {
            // Store string in constant pool and load reference
            int64_t string_id = add_string_constant(jit_func, expr->as.string);
            jit_function_add_instruction(jit_func, JIT_OP_LOAD_STRING, string_id);
            break;
        }
        
        default:
            runtime_panic_with_type(PANIC_INVALID_OPERATION, "Unknown expression type in JIT compilation: %d", expr->type);
            break;
    }
}

bool is_hot_path(JitFunction* func) {
    return func->execution_count >= JIT_THRESHOLD;
}

void update_execution_stats(JitFunction* func, double execution_time) {
    func->execution_count++;
    func->total_time += execution_time;
}

Value execute_jit_function(JitFunction* func, Value* args, size_t arg_count) {
    if (!func->native_code) {
        return value_null(); // Not compiled yet
    }
    
    clock_t start = clock();
    
    // Cast to function pointer and execute
    JitNativeFunction native_func = (JitNativeFunction)func->native_code;
    Value result = native_func(args, arg_count);
    
    clock_t end = clock();
    double execution_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    update_execution_stats(func, execution_time);
    
    return result;
}

// Optimization passes
void optimize_jit_function(JitFunction* func) {
    dead_code_elimination(func);
    constant_folding(func);
}

void dead_code_elimination(JitFunction* func) {
    // Mark reachable instructions
    bool* reachable = calloc(func->instruction_count, sizeof(bool));
    
    // Mark first instruction as reachable
    if (func->instruction_count > 0) {
        reachable[0] = true;
    }
    
    // Propagate reachability
    for (size_t i = 0; i < func->instruction_count; i++) {
        if (!reachable[i]) continue;
        
        JitInstruction* instr = &func->instructions[i];
        
        switch (instr->opcode) {
            case JIT_OP_JUMP:
                // Mark jump target as reachable
                if (instr->operand.int_operand < (int64_t)func->instruction_count) {
                    reachable[instr->operand.int_operand] = true;
                }
                break;
                
            case JIT_OP_JUMP_IF_FALSE:
                // Mark both next instruction and jump target
                if (i + 1 < func->instruction_count) {
                    reachable[i + 1] = true;
                }
                if (instr->operand.int_operand < (int64_t)func->instruction_count) {
                    reachable[instr->operand.int_operand] = true;
                }
                break;
                
            case JIT_OP_RETURN:
                // Return doesn't make next instruction reachable
                break;
                
            default:
                // Most instructions make next instruction reachable
                if (i + 1 < func->instruction_count) {
                    reachable[i + 1] = true;
                }
                break;
        }
    }
    
    // Remove unreachable instructions
    size_t write_pos = 0;
    for (size_t read_pos = 0; read_pos < func->instruction_count; read_pos++) {
        if (reachable[read_pos]) {
            if (write_pos != read_pos) {
                func->instructions[write_pos] = func->instructions[read_pos];
            }
            write_pos++;
        }
    }
    
    func->instruction_count = write_pos;
    free(reachable);
}

typedef struct ConstantPool {
    Value* constants;
    size_t count;
    size_t capacity;
} ConstantPool;

typedef struct RegisterAllocator {
    int* register_map;
    bool* register_used;
    size_t register_count;
    size_t next_virtual_reg;
} RegisterAllocator;

typedef struct OptimizationContext {
    JitFunction* function;
    ConstantPool* constants;
    RegisterAllocator* allocator;
    bool* instruction_used;
    int* instruction_map;
} OptimizationContext;

void constant_folding_advanced(JitFunction* func) {
    OptimizationContext ctx = {0};
    ctx.function = func;
    ctx.instruction_used = calloc(func->instruction_count, sizeof(bool));
    ctx.instruction_map = malloc(func->instruction_count * sizeof(int));
    
    // Mark all instructions as initially used
    for (size_t i = 0; i < func->instruction_count; i++) {
        ctx.instruction_used[i] = true;
        ctx.instruction_map[i] = i;
    }
    
    bool changed = true;
    while (changed) {
        changed = false;
        
        for (size_t i = 0; i < func->instruction_count - 2; i++) {
            if (!ctx.instruction_used[i]) continue;
            
            JitInstruction* instr1 = &func->instructions[i];
            JitInstruction* instr2 = &func->instructions[i + 1];
            JitInstruction* instr3 = &func->instructions[i + 2];
            
            // Pattern: LOAD_CONST, LOAD_CONST, BINARY_OP
            if (instr1->opcode == JIT_OP_LOAD_CONST &&
                instr2->opcode == JIT_OP_LOAD_CONST &&
                ctx.instruction_used[i + 1] && ctx.instruction_used[i + 2]) {
                
                int64_t result = 0;
                bool can_fold = true;
                bool is_comparison = false;
                
                switch (instr3->opcode) {
                    case JIT_OP_ADD:
                        result = instr1->operand.int_operand + instr2->operand.int_operand;
                        break;
                    case JIT_OP_SUB:
                        result = instr1->operand.int_operand - instr2->operand.int_operand;
                        break;
                    case JIT_OP_MUL:
                        result = instr1->operand.int_operand * instr2->operand.int_operand;
                        break;
                    case JIT_OP_DIV:
                        if (instr2->operand.int_operand != 0) {
                            result = instr1->operand.int_operand / instr2->operand.int_operand;
                        } else {
                            can_fold = false;
                        }
                        break;
                    case JIT_OP_COMPARE_EQ:
                        result = (instr1->operand.int_operand == instr2->operand.int_operand) ? 1 : 0;
                        is_comparison = true;
                        break;
                    case JIT_OP_COMPARE_LT:
                        result = (instr1->operand.int_operand < instr2->operand.int_operand) ? 1 : 0;
                        is_comparison = true;
                        break;
                    case JIT_OP_COMPARE_GT:
                        result = (instr1->operand.int_operand > instr2->operand.int_operand) ? 1 : 0;
                        is_comparison = true;
                        break;
                    default:
                        can_fold = false;
                        break;
                }
                
                if (can_fold) {
                    // Replace with single constant load
                    instr1->operand.int_operand = result;
                    
                    // Mark other instructions as unused
                    ctx.instruction_used[i + 1] = false;
                    ctx.instruction_used[i + 2] = false;
                    
                    changed = true;
                }
            }
            
            // Pattern: LOAD_CONST 0, JUMP_IF_FALSE -> unconditional jump
            if (instr1->opcode == JIT_OP_LOAD_CONST &&
                instr2->opcode == JIT_OP_JUMP_IF_FALSE &&
                ctx.instruction_used[i + 1]) {
                
                if (instr1->operand.int_operand == 0) {
                    // Always jump
                    instr1->opcode = JIT_OP_JUMP;
                    instr1->operand = instr2->operand;
                    ctx.instruction_used[i + 1] = false;
                    changed = true;
                } else {
                    // Never jump, remove both instructions
                    ctx.instruction_used[i] = false;
                    ctx.instruction_used[i + 1] = false;
                    changed = true;
                }
            }
            
            // Pattern: LOAD_CONST 1, JUMP_IF_FALSE -> remove both
            if (instr1->opcode == JIT_OP_LOAD_CONST &&
                instr2->opcode == JIT_OP_JUMP_IF_FALSE &&
                instr1->operand.int_operand != 0 &&
                ctx.instruction_used[i + 1]) {
                
                ctx.instruction_used[i] = false;
                ctx.instruction_used[i + 1] = false;
                changed = true;
            }
        }
    }
    
    // Compact instructions
    size_t write_pos = 0;
    for (size_t read_pos = 0; read_pos < func->instruction_count; read_pos++) {
        if (ctx.instruction_used[read_pos]) {
            if (write_pos != read_pos) {
                func->instructions[write_pos] = func->instructions[read_pos];
                
                // Update jump targets
                if (func->instructions[write_pos].opcode == JIT_OP_JUMP ||
                    func->instructions[write_pos].opcode == JIT_OP_JUMP_IF_FALSE) {
                    
                    size_t old_target = func->instructions[write_pos].operand.int_operand;
                    size_t new_target = 0;
                    
                    // Find new target position
                    for (size_t j = 0; j <= old_target && j < func->instruction_count; j++) {
                        if (ctx.instruction_used[j]) {
                            if (j == old_target) {
                                break;
                            }
                            new_target++;
                        }
                    }
                    
                    func->instructions[write_pos].operand.int_operand = new_target;
                }
            }
            write_pos++;
        }
    }
    
    func->instruction_count = write_pos;
    
    free(ctx.instruction_used);
    free(ctx.instruction_map);
}

void strength_reduction(JitFunction* func) {
    for (size_t i = 0; i < func->instruction_count - 2; i++) {
        JitInstruction* instr1 = &func->instructions[i];
        JitInstruction* instr2 = &func->instructions[i + 1];
        JitInstruction* instr3 = &func->instructions[i + 2];
        
        // Pattern: LOAD_CONST power_of_2, MUL -> SHIFT_LEFT
        if (instr1->opcode == JIT_OP_LOAD_CONST &&
            instr3->opcode == JIT_OP_MUL) {
            
            int64_t value = instr1->operand.int_operand;
            if (value > 0 && (value & (value - 1)) == 0) {
                // Power of 2, convert to shift
                int shift_amount = 0;
                while ((1LL << shift_amount) < value) {
                    shift_amount++;
                }
                
                instr1->operand.int_operand = shift_amount;
                instr3->opcode = JIT_OP_SHIFT_LEFT;
            }
        }
        
        // Pattern: LOAD_CONST 1, MUL -> remove (identity)
        if (instr1->opcode == JIT_OP_LOAD_CONST &&
            instr1->operand.int_operand == 1 &&
            instr3->opcode == JIT_OP_MUL) {
            
            // Remove multiplication by 1
            for (size_t j = i; j < func->instruction_count - 2; j++) {
                func->instructions[j] = func->instructions[j + 2];
            }
            func->instruction_count -= 2;
            i--; // Recheck this position
        }
        
        // Pattern: LOAD_CONST 0, ADD -> remove (identity)
        if (instr1->opcode == JIT_OP_LOAD_CONST &&
            instr1->operand.int_operand == 0 &&
            instr3->opcode == JIT_OP_ADD) {
            
            // Remove addition of 0
            for (size_t j = i; j < func->instruction_count - 2; j++) {
                func->instructions[j] = func->instructions[j + 2];
            }
            func->instruction_count -= 2;
            i--;
        }
    }
}

void loop_optimization(JitFunction* func) {
    // Identify loops and apply optimizations
    for (size_t i = 0; i < func->instruction_count; i++) {
        JitInstruction* instr = &func->instructions[i];
        
        if (instr->opcode == JIT_OP_JUMP) {
            size_t target = instr->operand.int_operand;
            
            // Backward jump indicates a loop
            if (target < i) {
                // Found a loop from target to i
                optimize_loop_body(func, target, i);
            }
        }
    }
}

void optimize_loop_body(JitFunction* func, size_t start, size_t end) {
    // Loop invariant code motion
    for (size_t i = start; i < end; i++) {
        JitInstruction* instr = &func->instructions[i];
        
        // Check if instruction is loop invariant
        if (is_loop_invariant(func, instr, start, end)) {
            // Move instruction before loop
            move_instruction_before_loop(func, i, start);
        }
    }
}

bool is_loop_invariant(JitFunction* func, JitInstruction* instr, size_t start, size_t end) {
    // Simple heuristic: constant loads are loop invariant
    if (instr->opcode == JIT_OP_LOAD_CONST) {
        return true;
    }
    
    // Variable loads are invariant if the variable is not modified in the loop
    if (instr->opcode == JIT_OP_LOAD_VAR) {
        char* var_name = (char*)instr->operand.ptr_operand;
        
        // Check if variable is modified in loop
        for (size_t i = start; i < end; i++) {
            JitInstruction* loop_instr = &func->instructions[i];
            if (loop_instr->opcode == JIT_OP_STORE_VAR) {
                char* store_var = (char*)loop_instr->operand.ptr_operand;
                if (strcmp(var_name, store_var) == 0) {
                    return false; // Variable is modified
                }
            }
        }
        return true;
    }
    
    return false;
}

void move_instruction_before_loop(JitFunction* func, size_t instr_pos, size_t loop_start) {
    JitInstruction temp = func->instructions[instr_pos];
    
    // Shift instructions
    for (size_t i = instr_pos; i > loop_start; i--) {
        func->instructions[i] = func->instructions[i - 1];
    }
    
    func->instructions[loop_start] = temp;
}

void constant_folding(JitFunction* func) {
    constant_folding_advanced(func);
    strength_reduction(func);
    loop_optimization(func);
}