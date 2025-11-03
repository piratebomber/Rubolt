#include "modules_registry.h"

// Forward declarations of module registrars implemented in Modules/*.c
void register_mod_string(ModuleSystem* ms);
void register_mod_random(ModuleSystem* ms);
void register_mod_atomics(ModuleSystem* ms);

void register_custom_modules(ModuleSystem* ms) {
    register_mod_string(ms);
    register_mod_random(ms);
    register_mod_atomics(ms);
}
