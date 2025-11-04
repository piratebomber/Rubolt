#include "rb_collections.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========== VALUE CONSTRUCTORS ========== */

RbValue rb_value_null(void) {
    RbValue val;
    val.type = RB_VAL_NULL;
    val.data.ptr = NULL;
    return val;
}

RbValue rb_value_int(int64_t i) {
    RbValue val;
    val.type = RB_VAL_INT;
    val.data.i = i;
    return val;
}

RbValue rb_value_float(double f) {
    RbValue val;
    val.type = RB_VAL_FLOAT;
    val.data.f = f;
    return val;
}

RbValue rb_value_string(const char *s) {
    RbValue val;
    val.type = RB_VAL_STRING;
    val.data.s = s ? strdup(s) : NULL;
    return val;
}

RbValue rb_value_bool(bool b) {
    RbValue val;
    val.type = RB_VAL_BOOL;
    val.data.b = b;
    return val;
}

RbValue rb_value_ptr(void *ptr) {
    RbValue val;
    val.type = RB_VAL_PTR;
    val.data.ptr = ptr;
    return val;
}

/* ========== HASH FUNCTIONS ========== */

/* FNV-1a hash constants */
#define FNV_OFFSET_BASIS 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

uint64_t rb_hash_string(const char *str) {
    if (!str) return 0;
    
    uint64_t hash = FNV_OFFSET_BASIS;
    while (*str) {
        hash ^= (uint64_t)(unsigned char)(*str++);
        hash *= FNV_PRIME;
    }
    return hash;
}

uint64_t rb_hash_int(int64_t i) {
    /* Mix the bits */
    uint64_t x = (uint64_t)i;
    x = ((x >> 32) ^ x) * 0x45d9f3b3335b369dULL;
    x = ((x >> 32) ^ x) * 0x3335b36945d9f3b3ULL;
    x = (x >> 32) ^ x;
    return x;
}

uint64_t rb_hash_float(double f) {
    uint64_t bits;
    memcpy(&bits, &f, sizeof(double));
    return rb_hash_int((int64_t)bits);
}

uint64_t rb_hash_ptr(const void *ptr) {
    return rb_hash_int((int64_t)(uintptr_t)ptr);
}

/* ========== VALUE OPERATIONS ========== */

uint64_t rb_value_hash(RbValue val) {
    switch (val.type) {
        case RB_VAL_NULL:
            return 0;
        case RB_VAL_INT:
            return rb_hash_int(val.data.i);
        case RB_VAL_FLOAT:
            return rb_hash_float(val.data.f);
        case RB_VAL_STRING:
            return rb_hash_string(val.data.s);
        case RB_VAL_BOOL:
            return val.data.b ? 1 : 0;
        case RB_VAL_PTR:
            return rb_hash_ptr(val.data.ptr);
        default:
            return 0;
    }
}

bool rb_value_equals(RbValue a, RbValue b) {
    if (a.type != b.type) return false;
    
    switch (a.type) {
        case RB_VAL_NULL:
            return true;
        case RB_VAL_INT:
            return a.data.i == b.data.i;
        case RB_VAL_FLOAT:
            return a.data.f == b.data.f;
        case RB_VAL_STRING:
            if (a.data.s == NULL && b.data.s == NULL) return true;
            if (a.data.s == NULL || b.data.s == NULL) return false;
            return strcmp(a.data.s, b.data.s) == 0;
        case RB_VAL_BOOL:
            return a.data.b == b.data.b;
        case RB_VAL_PTR:
            return a.data.ptr == b.data.ptr;
        default:
            return false;
    }
}

RbValue rb_value_clone(RbValue val) {
    if (val.type == RB_VAL_STRING && val.data.s) {
        return rb_value_string(val.data.s);
    }
    return val;
}

void rb_value_free(RbValue val) {
    if (val.type == RB_VAL_STRING && val.data.s) {
        free(val.data.s);
    }
}

char *rb_value_to_string(RbValue val) {
    char buffer[256];
    
    switch (val.type) {
        case RB_VAL_NULL:
            return strdup("None");
        case RB_VAL_INT:
            snprintf(buffer, sizeof(buffer), "%lld", (long long)val.data.i);
            return strdup(buffer);
        case RB_VAL_FLOAT:
            snprintf(buffer, sizeof(buffer), "%.6f", val.data.f);
            return strdup(buffer);
        case RB_VAL_STRING:
            return val.data.s ? strdup(val.data.s) : strdup("None");
        case RB_VAL_BOOL:
            return strdup(val.data.b ? "True" : "False");
        case RB_VAL_PTR:
            snprintf(buffer, sizeof(buffer), "<ptr %p>", val.data.ptr);
            return strdup(buffer);
        default:
            return strdup("<unknown>");
    }
}

void rb_value_print(RbValue val) {
    char *str = rb_value_to_string(val);
    printf("%s", str);
    free(str);
}

/* ========== COMPARISON FUNCTIONS ========== */

int rb_compare_int(const void *a, const void *b) {
    int64_t ia = *(const int64_t *)a;
    int64_t ib = *(const int64_t *)b;
    if (ia < ib) return -1;
    if (ia > ib) return 1;
    return 0;
}

int rb_compare_float(const void *a, const void *b) {
    double fa = *(const double *)a;
    double fb = *(const double *)b;
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

int rb_compare_string(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    if (sa == NULL && sb == NULL) return 0;
    if (sa == NULL) return -1;
    if (sb == NULL) return 1;
    return strcmp(sa, sb);
}
