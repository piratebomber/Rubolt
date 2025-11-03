#ifndef RUBOLT_VM_H
#define RUBOLT_VM_H

#include <stddef.h>

// Simple stack-based VM for Rubolt bytecode

typedef enum {
    OP_CONST = 0x01,
    OP_ADD   = 0x02,
    OP_PRINT = 0x03,
    OP_HALT  = 0xFF
} OpCode;

int vm_run(const unsigned char* code, size_t size);
int vm_run_file(const char* path);

#endif
