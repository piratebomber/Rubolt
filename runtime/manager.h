#ifndef RUBOLT_RUNTIME_MANAGER_H
#define RUBOLT_RUNTIME_MANAGER_H

#include <stddef.h>

// Runtime modes
typedef enum {
    RM_INTERPRETER = 0,
    RM_VM_BYTECODE = 1
} RuntimeMode;

typedef struct {
    RuntimeMode mode;
    int strict;
    int typecheck;
    // search paths
    char** search_paths;
    size_t search_path_count;
} RuntimeManager;

void rtman_init(RuntimeManager* rm);
void rtman_free(RuntimeManager* rm);
int  rtman_load_config(RuntimeManager* rm, const char* config_path);
void rtman_add_search_path(RuntimeManager* rm, const char* path);

// Execute .rbo via interpreter runtime
int  rtman_run_rbo(RuntimeManager* rm, const char* file);
// Execute .rbc via bytecode VM
int  rtman_run_rbc(RuntimeManager* rm, const char* file);

#endif
