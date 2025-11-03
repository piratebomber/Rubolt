#include "manager.h"
#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// forward decl VM API
int vm_run_file(const char* path);

void rtman_init(RuntimeManager* rm) {
    rm->mode = RM_INTERPRETER;
    rm->strict = 1;
    rm->typecheck = 1;
    rm->search_paths = NULL;
    rm->search_path_count = 0;
}

void rtman_free(RuntimeManager* rm) {
    for (size_t i = 0; i < rm->search_path_count; ++i) free(rm->search_paths[i]);
    free(rm->search_paths);
}

static char* strdup_c(const char* s) {
    size_t n = strlen(s); char* p = (char*)malloc(n+1); memcpy(p,s,n+1); return p;
}

void rtman_add_search_path(RuntimeManager* rm, const char* path) {
    rm->search_paths = (char**)realloc(rm->search_paths, sizeof(char*) * (rm->search_path_count + 1));
    rm->search_paths[rm->search_path_count++] = strdup_c(path);
}

int rtman_load_config(RuntimeManager* rm, const char* config_path) {
    FILE* f = fopen(config_path, "rb");
    if (!f) return 0; // optional
    fseek(f, 0, SEEK_END); long sz = ftell(f); rewind(f);
    char* buf = (char*)malloc(sz+1); fread(buf,1,sz,f); buf[sz]='\0'; fclose(f);
    // Very small JSON-ish parser: search for keys
    if (strstr(buf, "\"strict\": false")) rm->strict = 0; else rm->strict = 1;
    if (strstr(buf, "\"typecheck\": false")) rm->typecheck = 0; else rm->typecheck = 1;
    if (strstr(buf, "\"target\": \"vm\"")) rm->mode = RM_VM_BYTECODE; else rm->mode = RM_INTERPRETER;
    free(buf);
    return 1;
}

int rtman_run_rbo(RuntimeManager* rm, const char* file) {
    (void)rm;
    return runtime_run_file(file);
}

int rtman_run_rbc(RuntimeManager* rm, const char* file) {
    (void)rm;
    return vm_run_file(file);
}
