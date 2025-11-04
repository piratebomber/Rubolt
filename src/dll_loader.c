/*
 * dll_loader.c - Dynamic DLL/SO Loading Implementation
 */

#include "dll_loader.h"
#include "native_registry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define DLL_EXT ".dll"
    #define PATH_SEP "\\"
    typedef HMODULE DLL_HANDLE;
    #define DLL_LOAD(path) LoadLibraryA(path)
    #define DLL_GET_SYMBOL(h, name) GetProcAddress(h, name)
    #define DLL_UNLOAD(h) FreeLibrary(h)
    #define MKDIR(p) _mkdir(p)
    #define DLL_ERROR() "Windows DLL error"
#else
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dlfcn.h>
    #ifdef __APPLE__
        #define DLL_EXT ".dylib"
    #else
        #define DLL_EXT ".so"
    #endif
    #define PATH_SEP "/"
    typedef void* DLL_HANDLE;
    #define DLL_LOAD(path) dlopen(path, RTLD_LAZY)
    #define DLL_GET_SYMBOL(h, name) dlsym(h, name)
    #define DLL_UNLOAD(h) dlclose(h)
    #define MKDIR(p) mkdir(p, 0755)
    #define DLL_ERROR() dlerror()
#endif

struct RbDllHandle {
    char* name;
    char* path;
    DLL_HANDLE handle;
    struct RbDllHandle* next;
};

static struct RbDllHandle* loaded_dlls = NULL;
static char last_error[512] = {0};

static void set_error(const char* msg) {
    snprintf(last_error, sizeof(last_error), "%s", msg);
}

const char* rb_dll_get_error(void) { return last_error; }

static char* find_dll(const char* name) {
    static char path[1024];
    const char* search_dirs[] = {
        "src" PATH_SEP "precompiled",
        "vendor" PATH_SEP "lib",
        "vendor" PATH_SEP "bin",
        ".",
        NULL
    };

    // If explicit path
    FILE* test = fopen(name, "rb");
    if (test) { fclose(test); snprintf(path, sizeof(path), "%s", name); return path; }

    // If name already has extension, try each search dir
    if (strstr(name, DLL_EXT)) {
        for (int i = 0; search_dirs[i]; i++) {
            snprintf(path, sizeof(path), "%s" PATH_SEP "%s", search_dirs[i], name);
            FILE* f = fopen(path, "rb"); if (f) { fclose(f); return path; }
        }
    }

    // Try name + extension in search dirs
    for (int i = 0; search_dirs[i]; i++) {
        snprintf(path, sizeof(path), "%s" PATH_SEP "%s%s", search_dirs[i], name, DLL_EXT);
        FILE* f = fopen(path, "rb"); if (f) { fclose(f); return path; }
    }

    // Fallback name+ext in cwd
    snprintf(path, sizeof(path), "%s%s", name, DLL_EXT);
    return path;
}

int rb_dll_is_loaded(const char* name) {
    for (struct RbDllHandle* h = loaded_dlls; h; h = h->next) {
        if (strcmp(h->name, name) == 0) return 1;
    }
    return 0;
}

struct RbDllHandle* rb_dll_load(const char* name) {
    for (struct RbDllHandle* h = loaded_dlls; h; h = h->next) {
        if (strcmp(h->name, name) == 0) return h;
    }

    char* path = find_dll(name);
    DLL_HANDLE handle = DLL_LOAD(path);
    if (!handle) { set_error(DLL_ERROR()); return NULL; }

    struct RbDllHandle* dll = (struct RbDllHandle*)malloc(sizeof(struct RbDllHandle));
    dll->name = strdup(name);
    dll->path = strdup(path);
    dll->handle = handle;
    dll->next = loaded_dlls;
    loaded_dlls = dll;
    return dll;
}

void rb_dll_unload(struct RbDllHandle* handle) {
    if (!handle) return;
    if (loaded_dlls == handle) loaded_dlls = handle->next; else {
        for (struct RbDllHandle* h = loaded_dlls; h; h = h->next) {
            if (h->next == handle) { h->next = handle->next; break; }
        }
    }
    DLL_UNLOAD(handle->handle);
    free(handle->name); free(handle->path); free(handle);
}

void* rb_dll_get_symbol(struct RbDllHandle* handle, const char* symbol_name) {
    if (!handle) return NULL;
    void* sym = (void*)DLL_GET_SYMBOL(handle->handle, symbol_name);
    if (!sym) set_error(DLL_ERROR());
    return sym;
}

typedef struct { const char* name; void* fn; } RbExport;

typedef RbExport* (*GetExportsFunc)(int* count);

int rb_dll_register_exports(struct RbDllHandle* handle, const char* module_name) {
    if (!handle) return -1;

    // Try new-style exporter
    GetExportsFunc getx = (GetExportsFunc)rb_dll_get_symbol(handle, "rubolt_get_exports");
    if (!getx) getx = (GetExportsFunc)rb_dll_get_symbol(handle, "rb_get_exports");
    if (getx) {
        int count = 0; RbExport* ex = getx(&count);
        if (ex && count > 0) {
            for (int i = 0; i < count; ++i) native_register(ex[i].name, (RbNativeFn)ex[i].fn);
            return 0;
        }
    }

    // Fallback: call rb_init_<module>
    char init_name[256];
    snprintf(init_name, sizeof(init_name), "rb_init_%s", module_name);
    typedef void (*InitFunc)(void);
    InitFunc init = (InitFunc)rb_dll_get_symbol(handle, init_name);
    if (init) { init(); return 0; }

    set_error("No exports found in DLL");
    return -1;
}

struct RbDllHandle* rb_dll_compile_and_load(const char* source_path) {
    char dll_path[1024]; char cmd[2048];
    // Ensure precompiled dir exists
    MKDIR("src" PATH_SEP "precompiled");

    // Derive output name
    const char* base = strrchr(source_path, PATH_SEP[0]); base = base ? base+1 : source_path;
    snprintf(dll_path, sizeof(dll_path), "src" PATH_SEP "precompiled" PATH_SEP "%s%s", base, DLL_EXT);
    // remove trailing .c before extension
    char* dotc = strstr(dll_path, ".c"); if (dotc) { memmove(dotc, dotc+2, strlen(dotc+2)+1); }

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "gcc -shared -O2 -o \"%s\" \"%s\" -I. -Isrc -Ishared/sdk/native", dll_path, source_path);
#else
    snprintf(cmd, sizeof(cmd), "gcc -shared -fPIC -O2 -o \"%s\" \"%s\" -I. -Isrc -Ishared/sdk/native", dll_path, source_path);
#endif
    int ret = system(cmd);
    if (ret != 0) { set_error("Compilation failed"); return NULL; }
    return rb_dll_load(dll_path);
}

void rb_dll_list_loaded(void) {
    printf("[dll_loader] Loaded DLLs:\n");
    for (struct RbDllHandle* h = loaded_dlls; h; h = h->next) printf("  - %s (%s)\n", h->name, h->path);
}
