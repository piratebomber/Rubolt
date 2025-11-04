/*
 * dll_loader.h - Dynamic DLL/SO Loading System for Rubolt
 * Handles import [dllname].dll statements with cross-platform support
 */

#ifndef DLL_LOADER_H
#define DLL_LOADER_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for loaded DLL
typedef struct RbDllHandle RbDllHandle;

// Load a DLL/SO by name or path
// Searches: src/precompiled, vendor/lib, vendor/bin, cwd
RbDllHandle* rb_dll_load(const char* name);

// Unload a DLL
void rb_dll_unload(RbDllHandle* handle);

// Get function pointer from loaded DLL
void* rb_dll_get_symbol(RbDllHandle* handle, const char* symbol_name);

// Get last error message
const char* rb_dll_get_error(void);

// Register exports from DLL into native registry
int rb_dll_register_exports(RbDllHandle* handle, const char* module_name);

// Auto-compile and load a C file to DLL if precompiled version missing
RbDllHandle* rb_dll_compile_and_load(const char* source_path);

// Check if DLL is already loaded
int rb_dll_is_loaded(const char* name);

// List all loaded DLLs
void rb_dll_list_loaded(void);

#ifdef __cplusplus
}
#endif

#endif // DLL_LOADER_H
