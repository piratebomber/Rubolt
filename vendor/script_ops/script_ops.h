#ifndef SCRIPT_OPS_H
#define SCRIPT_OPS_H

#ifdef _WIN32
  #ifdef SCRIPT_OPS_EXPORTS
    #define SOP_API __declspec(dllexport)
  #else
    #define SOP_API __declspec(dllimport)
  #endif
#else
  #define SOP_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Returns 0 on success, non-zero on error
SOP_API int sop_write_text_file(const char* path, const char* text);
SOP_API int sop_read_text_file(const char* path, /*out*/char* buffer, int buffer_len);
SOP_API int sop_mkdirs(const char* path);
SOP_API int sop_list_dir(const char* path, /*out*/char* buffer, int buffer_len);

#ifdef __cplusplus
}
#endif

#endif // SCRIPT_OPS_H
