/*
 * rubolt_string.h - String Library Header for Rubolt
 * String manipulation functions
 */

#ifndef RUBOLT_STRING_H
#define RUBOLT_STRING_H

#include <stddef.h>

// String operations
char* rb_str_concat(char* dest, const char* src);
int rb_str_length(const char* str);
int rb_str_compare(const char* s1, const char* s2);
char* rb_str_copy(char* dest, const char* src);
char* rb_str_reverse(char* str);

// String searching
int rb_str_find(const char* haystack, const char* needle);
char* rb_str_replace(char* str, const char* old, const char* new);

// Case conversion
char* rb_str_upper(char* str);
char* rb_str_lower(char* str);

// Memory operations
void* rb_mem_alloc(size_t size);
void rb_mem_free(void* ptr);

#endif // RUBOLT_STRING_H
