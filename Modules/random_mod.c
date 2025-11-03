#include "module.h"
#include <stdlib.h>
#include <time.h>

static int seeded = 0;

static void ensure_seeded() {
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }
}

static Value rand_int(Environment* env, Value* args, size_t arg_count) {
    ensure_seeded();
    int min = 0, max = 100;
    if (arg_count >= 1 && args[0].type == VAL_NUMBER) min = (int)args[0].as.number;
    if (arg_count >= 2 && args[1].type == VAL_NUMBER) max = (int)args[1].as.number;
    if (max <= min) max = min + 1;
    int r = min + rand() % (max - min);
    return value_number((double)r);
}

static Value rand_float(Environment* env, Value* args, size_t arg_count) {
    ensure_seeded();
    double r = (double)rand() / (double)RAND_MAX;
    return value_number(r);
}

void register_mod_random(ModuleSystem* ms) {
    Module* m = module_system_load(ms, "random");
    module_register_native_function(m, "int", rand_int);
    module_register_native_function(m, "float", rand_float);
}
