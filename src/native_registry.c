#include "native_registry.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_NATIVE_FUNCS 512

typedef struct { char* name; RbNativeFn fn; } NativeEntry;
static NativeEntry g_entries[MAX_NATIVE_FUNCS];
static size_t g_count = 0;

void native_registry_init(void) {
    for (size_t i = 0; i < g_count; ++i) {
        free(g_entries[i].name);
        g_entries[i].name = NULL;
        g_entries[i].fn = NULL;
    }
    g_count = 0;
}

void native_registry_free(void) {
    native_registry_init();
}

static char* nr_strdup(const char* s){
#ifdef _WIN32
    char* p = _strdup(s);
#else
    char* p = strdup(s);
#endif
    return p;
}

int native_register(const char* name, RbNativeFn fn) {
    if (g_count >= MAX_NATIVE_FUNCS) return 0;
    // overwrite if exists
    for (size_t i = 0; i < g_count; ++i) {
        if (strcmp(g_entries[i].name, name) == 0) {
            g_entries[i].fn = fn;
            return 1;
        }
    }
    g_entries[g_count].name = nr_strdup(name);
    g_entries[g_count].fn = fn;
    g_count++;
    return 1;
}

RbNativeFn native_find(const char* name) {
    for (size_t i = 0; i < g_count; ++i) {
        if (strcmp(g_entries[i].name, name) == 0) return g_entries[i].fn;
    }
    return NULL;
}

void native_list(void) {
    printf("[native] Registered functions (%zu):\n", g_count);
    for (size_t i = 0; i < g_count; ++i) {
        printf("  - %s\n", g_entries[i].name);
    }
}
