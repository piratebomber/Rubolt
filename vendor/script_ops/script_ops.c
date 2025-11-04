#define _CRT_SECURE_NO_WARNINGS
#include "script_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
  #include <direct.h>
  #include <windows.h>
  #define MKDIR(path) _mkdir(path)
#else
  #include <sys/stat.h>
  #include <dirent.h>
  #include <unistd.h>
  #define MKDIR(path) mkdir(path, 0755)
#endif

static int create_dirs(const char* path){
    char tmp[1024];
    size_t len = strlen(path);
    if(len >= sizeof(tmp)) return -1;
    strcpy(tmp, path);
    for(size_t i=1;i<len;i++){
        if(tmp[i]=='/' || tmp[i]=='\\'){
            char old = tmp[i];
            tmp[i] = '\0';
            MKDIR(tmp);
            tmp[i] = old;
        }
    }
    return MKDIR(tmp) == 0 ? 0 : 0; // ignore errors if exist
}

SOP_API int sop_write_text_file(const char* path, const char* text){
    if(create_dirs(path) != 0) { /* best-effort */ }
    FILE* f = fopen(path, "wb");
    if(!f) return 1;
    size_t n = fwrite(text, 1, strlen(text), f);
    fclose(f);
    return n == strlen(text) ? 0 : 2;
}

SOP_API int sop_read_text_file(const char* path, char* buffer, int buffer_len){
    FILE* f = fopen(path, "rb");
    if(!f) return 1;
    size_t n = fread(buffer, 1, (size_t)(buffer_len-1), f);
    buffer[n] = '\0';
    fclose(f);
    return 0;
}

SOP_API int sop_mkdirs(const char* path){
    return create_dirs(path);
}

SOP_API int sop_list_dir(const char* path, char* buffer, int buffer_len){
#ifdef _WIN32
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s\\*", path);
    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if(h == INVALID_HANDLE_VALUE) return 1;
    int used = 0;
    do {
        const char* name = ffd.cFileName;
        if(strcmp(name, ".")==0 || strcmp(name, "..")==0) continue;
        int len = (int)strlen(name);
        if(used + len + 2 >= buffer_len) break;
        memcpy(buffer+used, name, len);
        used += len;
        buffer[used++]='\n';
    } while(FindNextFileA(h, &ffd));
    FindClose(h);
    buffer[used] = '\0';
    return 0;
#else
    DIR* d = opendir(path);
    if(!d) return 1;
    struct dirent* ent;
    int used = 0;
    while((ent = readdir(d))){
        const char* name = ent->d_name;
        if(strcmp(name, ".")==0 || strcmp(name, "..")==0) continue;
        int len = (int)strlen(name);
        if(used + len + 2 >= buffer_len) break;
        memcpy(buffer+used, name, len);
        used += len;
        buffer[used++]='\n';
    }
    closedir(d);
    buffer[used] = '\0';
    return 0;
#endif
}
