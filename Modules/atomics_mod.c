#include "module.h"
#include <string.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#define MAX_ATOMICS 128

typedef struct {
    char* name;
    _Atomic long value;
    bool used;
} AtomicItem;

static AtomicItem g_atomics[MAX_ATOMICS];

static int find_or_create(const char* name) {
    for (int i = 0; i < MAX_ATOMICS; i++) {
        if (g_atomics[i].used && strcmp(g_atomics[i].name, name) == 0) return i;
    }
    for (int i = 0; i < MAX_ATOMICS; i++) {
        if (!g_atomics[i].used) {
            g_atomics[i].name = _strdup(name);
            atomic_store(&g_atomics[i].value, 0);
            g_atomics[i].used = true;
            return i;
        }
    }
    return -1;
}

static Value a_create(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_bool(false);
    const char* name = args[0].as.string;
    long initial = 0;
    if (arg_count >= 2 && args[1].type == VAL_NUMBER) initial = (long)args[1].as.number;
    int idx = find_or_create(name);
    if (idx < 0) return value_bool(false);
    atomic_store(&g_atomics[idx].value, initial);
    return value_bool(true);
}

static Value a_inc(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_number(0);
    const char* name = args[0].as.string;
    int idx = find_or_create(name);
    if (idx < 0) return value_number(0);
    long v = atomic_fetch_add(&g_atomics[idx].value, 1) + 1;
    return value_number((double)v);
}

static Value a_get(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 1 || args[0].type != VAL_STRING) return value_number(0);
    const char* name = args[0].as.string;
    int idx = find_or_create(name);
    if (idx < 0) return value_number(0);
    long v = atomic_load(&g_atomics[idx].value);
    return value_number((double)v);
}

static Value a_cas(Environment* env, Value* args, size_t arg_count) {
    if (arg_count < 3 || args[0].type != VAL_STRING || args[1].type != VAL_NUMBER || args[2].type != VAL_NUMBER) {
        return value_bool(false);
    }
    const char* name = args[0].as.string;
    long expected = (long)args[1].as.number;
    long desired = (long)args[2].as.number;
    int idx = find_or_create(name);
    if (idx < 0) return value_bool(false);
    long exp = expected;
    bool ok = atomic_compare_exchange_strong(&g_atomics[idx].value, &exp, desired);
    return value_bool(ok);
}

void register_mod_atomics(ModuleSystem* ms) {
    for (int i = 0; i < MAX_ATOMICS; i++) g_atomics[i].used = false;
    Module* m = module_system_load(ms, "atomics");
    module_register_native_function(m, "create", a_create);
    module_register_native_function(m, "inc", a_inc);
    module_register_native_function(m, "get", a_get);
    module_register_native_function(m, "cas", a_cas);
}
