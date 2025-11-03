#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

// Simple C analyzer: counts functions and LOC in C files

static int is_c_file(const char* name) {
    const char* dot = strrchr(name, '.');
    return dot && (!strcmp(dot, ".c") || !strcmp(dot, ".h"));
}

int main(int argc, char** argv) {
    const char* root = argc > 1 ? argv[1] : "src";
    DIR* d = opendir(root);
    if (!d) { perror("opendir"); return 1; }
    struct dirent* e;
    int total_loc = 0, func_count = 0, files = 0;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.') continue;
        if (!is_c_file(e->d_name)) continue;
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", root, e->d_name);
        FILE* f = fopen(path, "r");
        if (!f) continue;
        files++;
        char line[2048];
        while (fgets(line, sizeof(line), f)) {
            int only_ws = 1; for (char* p=line; *p; ++p) if (!isspace((unsigned char)*p)) { only_ws=0; break; }
            if (!only_ws) total_loc++;
            if (strchr(line, '(') && strchr(line, ')') && strchr(line, '{') && !strstr(line, "if") && !strstr(line, "for") && !strstr(line, "while")) {
                func_count++;
            }
        }
        fclose(f);
    }
    closedir(d);
    printf("Files: %d\nFunctions (approx): %d\nLOC: %d\n", files, func_count, total_loc);
    return 0;
}
