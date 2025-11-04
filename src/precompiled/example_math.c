/*
 * example_math.c - Example Rubolt DLL Module using export table
 * Build (Windows): gcc -shared -O2 -o example_math.dll example_math.c
 * Build (Linux):   gcc -shared -fPIC -O2 -o libexample_math.so example_math.c
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Core type definitions from interpreter
typedef struct Environment Environment; // opaque

typedef struct {
    int type; // 0:null,1:bool,2:number,3:string
    union { int boolean; double number; const char* string; } as;
} Value;

// Signatures expected by core
typedef Value (*RbNativeFn)(Environment*, Value*, size_t);

typedef struct { const char* name; void* fn; } RbExport;

// Helpers
static Value v_number(double d){ Value v; v.type=2; v.as.number=d; return v; }
static Value v_int(int64_t i){ return v_number((double)i); }

static Value add(Environment* env, Value* argv, size_t argc){ (void)env; if(argc<2) return v_int(0); return v_number(argv[0].as.number + argv[1].as.number); }
static Value mul(Environment* env, Value* argv, size_t argc){ (void)env; if(argc<2) return v_int(0); return v_number(argv[0].as.number * argv[1].as.number); }

static RbExport EX[] = {
    {"add", (void*)add},
    {"mul", (void*)mul}
};

#ifdef _WIN32
__declspec(dllexport)
#endif
RbExport* rubolt_get_exports(int* count){ if(count) *count = (int)(sizeof(EX)/sizeof(EX[0])); return EX; }
