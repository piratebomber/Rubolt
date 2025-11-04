/*
 * symbol_map.h - Direct macro aliases for compile-time replacement
 * Define `#define old new` pairs here. Guarded by RUBOLT_ENABLE_SYMBOL_ALIASES.
 */

#ifndef SYMBOL_MAP_H
#define SYMBOL_MAP_H

/* Enable only when explicitly requested */
#ifdef RUBOLT_ENABLE_SYMBOL_ALIASES

#define print rb_builtin_print
#define input rb_builtin_input
#define len   rb_builtin_le
#define str   rb_builtin_str


#endif /* RUBOLT_ENABLE_SYMBOL_ALIASES */

#endif /* SYMBOL_MAP_H */
