#ifndef RB_COLLECTIONS_H
#define RB_COLLECTIONS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* ========== VALUE TYPES ========== */

typedef enum {
    RB_VAL_NULL,
    RB_VAL_INT,
    RB_VAL_FLOAT,
    RB_VAL_STRING,
    RB_VAL_BOOL,
    RB_VAL_PTR
} RbValueType;

/* Generic value container */
typedef struct RbValue {
    RbValueType type;
    union {
        int64_t i;
        double f;
        char *s;
        bool b;
        void *ptr;
    } data;
} RbValue;

/* ========== VALUE CONSTRUCTORS ========== */

RbValue rb_value_null(void);
RbValue rb_value_int(int64_t i);
RbValue rb_value_float(double f);
RbValue rb_value_string(const char *s);
RbValue rb_value_bool(bool b);
RbValue rb_value_ptr(void *ptr);

/* ========== VALUE OPERATIONS ========== */

/* Hash a value */
uint64_t rb_value_hash(RbValue val);

/* Compare two values for equality */
bool rb_value_equals(RbValue a, RbValue b);

/* Clone a value (deep copy for strings) */
RbValue rb_value_clone(RbValue val);

/* Free a value (only frees strings) */
void rb_value_free(RbValue val);

/* Convert value to string (returns allocated string) */
char *rb_value_to_string(RbValue val);

/* Print value to stdout */
void rb_value_print(RbValue val);

/* ========== HASH FUNCTIONS ========== */

/* FNV-1a hash for strings */
uint64_t rb_hash_string(const char *str);

/* Hash for integers */
uint64_t rb_hash_int(int64_t i);

/* Hash for floats */
uint64_t rb_hash_float(double f);

/* Hash for pointers */
uint64_t rb_hash_ptr(const void *ptr);

/* ========== COMPARISON FUNCTIONS ========== */

typedef int (*RbCompareFn)(const void *a, const void *b);

/* Compare integers */
int rb_compare_int(const void *a, const void *b);

/* Compare floats */
int rb_compare_float(const void *a, const void *b);

/* Compare strings */
int rb_compare_string(const void *a, const void *b);

#endif /* RB_COLLECTIONS_H */
