#include "frozen.h"
#include <string.h>

typedef struct { const char* name; const char* data; } FrozenEntry;

static const char* prelude =
"// Prelude standard library loaded before all programs\n"
"def banner() -> void {\n"
"    print(\"Rubolt Runtime Loaded\");\n"
"}\n"
"def max(a: number, b: number) -> number { if (a > b) { return a; } else { return b; } }\n"
"def min(a: number, b: number) -> number { if (a < b) { return a; } else { return b; } }\n";

static FrozenEntry g_frozen[] = {
    {"StdLib/prelude.rbo", prelude},
    {"prelude", prelude},
};

const char* frozen_get(const char* name) {
    for (size_t i = 0; i < sizeof(g_frozen)/sizeof(g_frozen[0]); ++i) {
        if (strcmp(g_frozen[i].name, name) == 0) return g_frozen[i].data;
    }
    return NULL;
}
