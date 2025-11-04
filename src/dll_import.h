/*
 * dll_import.h - Rubolt DLL Import System
 */

#ifndef DLL_IMPORT_H
#define DLL_IMPORT_H

#ifdef __cplusplus
extern "C" {
#endif

int rb_import_dll(const char* import_spec);
int rb_import_c_source(const char* source_path);
int rb_is_dll_import(const char* import_spec);
int rb_is_c_source_import(const char* import_spec);

#ifdef __cplusplus
}
#endif

#endif // DLL_IMPORT_H
