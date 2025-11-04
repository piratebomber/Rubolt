/*
 * type_macros.h - Typed constructor and helpers for Rubolt C API
 */
#ifndef TYPE_MACROS_H
#define TYPE_MACROS_H

#include <stdint.h>

/* Forward declare Rubolt API constructors if not already included */
#ifndef RUBOLT_SDK_API_H
/* Treat these as external functions resolved at link-time */
extern void* rb_int(int64_t v);
extern void* rb_float(double v);
extern void* rb_string(const char* s);
extern void* rb_bool(int v);
extern void* rb_null(void);
#endif

/* Convenience constructors */
#define RB_INT(v)    rb_int((int64_t)(v))
#define RB_FLOAT(v)  rb_float((double)(v))
#define RB_STR(s)    rb_string((const char*)(s))
#define RB_BOOL(b)   rb_bool((b) ? 1 : 0)
#define RB_NULL()    rb_null()

/* Argument guards */
#define RB_REQ_ARGS(actual, expected) \
    do { if ((actual) != (expected)) { \
        fprintf(stderr, "[ARG] Expected %d args, got %d\n", (expected), (actual)); \
        return RB_NULL(); \
    } } while(0)

#endif /* TYPE_MACROS_H */
