#ifndef RUBOLT_BOPES_H
#define RUBOLT_BOPES_H

#include <stddef.h>

// Bopes: Rubolt Virtual Environment Simulator
// Provides a sandboxed execution environment with virtual memory/devices.

typedef struct {
    unsigned char* memory;   // linear memory
    size_t mem_size;
    int tracing;             // enable instruction tracing
} BopesVM;

void bopes_init(BopesVM* vm, size_t mem_size);
void bopes_free(BopesVM* vm);
int  bopes_run_file(BopesVM* vm, const char* path);
int  bopes_run_source(BopesVM* vm, const char* source);

#endif
