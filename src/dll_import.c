/*
 * dll_import.c - Rubolt import system integration for DLLs
 * Handles: import name.dll, import dll.name, import "path/to/lib.dll", import "file.c"
 */

#include "dll_import.h"
#include "dll_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int rb_is_dll_import(const char* import_spec) {
    return (strstr(import_spec, ".dll") != NULL ||
            strstr(import_spec, ".so") != NULL ||
            strstr(import_spec, ".dylib") != NULL ||
            strncmp(import_spec, "dll.", 4) == 0);
}

int rb_is_c_source_import(const char* import_spec) {
    const char* dotc = strstr(import_spec, ".c");
    return (dotc != NULL && dotc[2] == '\0') ? 1 : 0;
}

int rb_import_dll(const char* import_spec) {
    char namebuf[256];
    const char* dll_name = import_spec;

    if (strncmp(import_spec, "dll.", 4) == 0) {
        snprintf(namebuf, sizeof(namebuf), "%s", import_spec + 4);
        dll_name = namebuf;
    }

    struct RbDllHandle* handle = rb_dll_load(dll_name);
    if (!handle) {
        fprintf(stderr, "[import] Failed to load DLL '%s': %s\n", dll_name, rb_dll_get_error());
        return -1;
    }

    // module name from base
    const char* base = strrchr(dll_name, '/');
    if (!base) base = strrchr(dll_name, '\\');
    base = base ? base + 1 : dll_name;
    char modname[256]; snprintf(modname, sizeof(modname), "%s", base);
    char* dot = strchr(modname, '.'); if (dot) *dot = '\0';

    if (rb_dll_register_exports(handle, modname) != 0) {
        fprintf(stderr, "[import] Warning: no exports registered for '%s' (%s)\n", modname, rb_dll_get_error());
    }

    return 0;
}

int rb_import_c_source(const char* source_path) {
    struct RbDllHandle* handle = rb_dll_compile_and_load(source_path);
    if (!handle) {
        fprintf(stderr, "[import] Failed to compile '%s': %s\n", source_path, rb_dll_get_error());
        return -1;
    }
    // Derive module name
    const char* base = strrchr(source_path, '/');
    if (!base) base = strrchr(source_path, '\\');
    base = base ? base + 1 : source_path;
    char modname[256]; snprintf(modname, sizeof(modname), "%s", base);
    char* dot = strchr(modname, '.'); if (dot) *dot = '\0';
    if (rb_dll_register_exports(handle, modname) != 0) {
        fprintf(stderr, "[import] Warning: no exports registered for '%s' (%s)\n", modname, rb_dll_get_error());
    }
    return 0;
}
