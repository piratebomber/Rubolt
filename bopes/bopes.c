#include "bopes.h"
#include "../runtime/runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void bopes_init(BopesVM* vm, size_t mem_size) {
    vm->memory = (unsigned char*)calloc(1, mem_size);
    vm->mem_size = mem_size;
    vm->tracing = 0;
}

void bopes_free(BopesVM* vm) {
    free(vm->memory);
    vm->memory = NULL;
    vm->mem_size = 0;
}

int bopes_run_source(BopesVM* vm, const char* source) {
    (void)vm; // Future: map IO and memory syscalls to VM devices
    return runtime_run_source(source);
}

int bopes_run_file(BopesVM* vm, const char* path) {
    (void)vm;
    return runtime_run_file(path);
}
