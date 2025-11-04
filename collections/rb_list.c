#include "rb_list.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_CAPACITY 8
#define GROWTH_FACTOR 2

/* ========== HELPERS ========== */

static void ensure_capacity(RbList *list, size_t min_capacity) {
    if (list->capacity >= min_capacity) return;
    
    size_t new_capacity = list->capacity == 0 ? INITIAL_CAPACITY : list->capacity;
    while (new_capacity < min_capacity) {
        new_capacity *= GROWTH_FACTOR;
    }
    
    RbValue *new_items = (RbValue *)realloc(list->items, sizeof(RbValue) * new_capacity);
    if (!new_items) return;
    
    list->items = new_items;
    list->capacity = new_capacity;
}

static int normalize_index(int index, size_t size) {
    if (index < 0) {
        index += (int)size;
    }
    return index;
}

/* ========== CREATION & DESTRUCTION ========== */

RbList *rb_list_new(void) {
    RbList *list = (RbList *)malloc(sizeof(RbList));
    if (!list) return NULL;
    
    list->items = NULL;
    list->size = 0;
    list->capacity = 0;
    return list;
}

RbList *rb_list_with_capacity(size_t capacity) {
    RbList *list = rb_list_new();
    if (list) {
        ensure_capacity(list, capacity);
    }
    return list;
}

RbList *rb_list_from_array(RbValue *items, size_t count) {
    RbList *list = rb_list_with_capacity(count);
    if (list && items) {
        memcpy(list->items, items, sizeof(RbValue) * count);
        list->size = count;
    }
    return list;
}

void rb_list_free(RbList *list) {
    if (!list) return;
    
    /* Free string values */
    for (size_t i = 0; i < list->size; i++) {
        rb_value_free(list->items[i]);
    }
    
    free(list->items);
    free(list);
}

void rb_list_clear(RbList *list) {
    if (!list) return;
    
    for (size_t i = 0; i < list->size; i++) {
        rb_value_free(list->items[i]);
    }
    
    list->size = 0;
}

/* ========== BASIC OPERATIONS ========== */

size_t rb_list_len(const RbList *list) {
    return list ? list->size : 0;
}

bool rb_list_is_empty(const RbList *list) {
    return rb_list_len(list) == 0;
}

RbValue rb_list_get(const RbList *list, int index) {
    if (!list) return rb_value_null();
    
    index = normalize_index(index, list->size);
    if (index < 0 || (size_t)index >= list->size) {
        return rb_value_null();
    }
    
    return list->items[index];
}

void rb_list_set(RbList *list, int index, RbValue value) {
    if (!list) return;
    
    index = normalize_index(index, list->size);
    if (index < 0 || (size_t)index >= list->size) return;
    
    rb_value_free(list->items[index]);
    list->items[index] = rb_value_clone(value);
}

void rb_list_append(RbList *list, RbValue value) {
    if (!list) return;
    
    ensure_capacity(list, list->size + 1);
    list->items[list->size++] = rb_value_clone(value);
}

void rb_list_insert(RbList *list, int index, RbValue value) {
    if (!list) return;
    
    index = normalize_index(index, list->size);
    if (index < 0) index = 0;
    if ((size_t)index > list->size) index = (int)list->size;
    
    ensure_capacity(list, list->size + 1);
    
    /* Shift items right */
    memmove(&list->items[index + 1], &list->items[index], 
            sizeof(RbValue) * (list->size - index));
    
    list->items[index] = rb_value_clone(value);
    list->size++;
}

RbValue rb_list_pop(RbList *list, int index) {
    if (!list || list->size == 0) return rb_value_null();
    
    index = normalize_index(index, list->size);
    if (index < 0 || (size_t)index >= list->size) {
        return rb_value_null();
    }
    
    RbValue value = list->items[index];
    
    /* Shift items left */
    memmove(&list->items[index], &list->items[index + 1],
            sizeof(RbValue) * (list->size - index - 1));
    
    list->size--;
    return value;
}

bool rb_list_remove(RbList *list, RbValue value) {
    int index = rb_list_index(list, value);
    if (index < 0) return false;
    
    RbValue popped = rb_list_pop(list, index);
    rb_value_free(popped);
    return true;
}

void rb_list_extend(RbList *list, const RbList *other) {
    if (!list || !other) return;
    
    ensure_capacity(list, list->size + other->size);
    
    for (size_t i = 0; i < other->size; i++) {
        list->items[list->size++] = rb_value_clone(other->items[i]);
    }
}

/* ========== SEARCH & COUNT ========== */

int rb_list_index(const RbList *list, RbValue value) {
    if (!list) return -1;
    
    for (size_t i = 0; i < list->size; i++) {
        if (rb_value_equals(list->items[i], value)) {
            return (int)i;
        }
    }
    
    return -1;
}

size_t rb_list_count(const RbList *list, RbValue value) {
    if (!list) return 0;
    
    size_t count = 0;
    for (size_t i = 0; i < list->size; i++) {
        if (rb_value_equals(list->items[i], value)) {
            count++;
        }
    }
    
    return count;
}

bool rb_list_contains(const RbList *list, RbValue value) {
    return rb_list_index(list, value) >= 0;
}

/* ========== SORTING & REVERSING ========== */

void rb_list_reverse(RbList *list) {
    if (!list || list->size <= 1) return;
    
    for (size_t i = 0; i < list->size / 2; i++) {
        RbValue temp = list->items[i];
        list->items[i] = list->items[list->size - 1 - i];
        list->items[list->size - 1 - i] = temp;
    }
}

static int compare_values(const void *a, const void *b) {
    const RbValue *va = (const RbValue *)a;
    const RbValue *vb = (const RbValue *)b;
    
    if (va->type != vb->type) return va->type - vb->type;
    
    switch (va->type) {
        case RB_VAL_INT:
            return rb_compare_int(&va->data.i, &vb->data.i);
        case RB_VAL_FLOAT:
            return rb_compare_float(&va->data.f, &vb->data.f);
        case RB_VAL_STRING:
            return rb_compare_string(&va->data.s, &vb->data.s);
        default:
            return 0;
    }
}

void rb_list_sort(RbList *list, RbCompareFn compare) {
    if (!list || list->size <= 1) return;
    
    if (compare) {
        qsort(list->items, list->size, sizeof(RbValue), compare);
    } else {
        qsort(list->items, list->size, sizeof(RbValue), compare_values);
    }
}

/* ========== SLICING & COPYING ========== */

RbList *rb_list_copy(const RbList *list) {
    if (!list) return NULL;
    
    RbList *copy = rb_list_with_capacity(list->size);
    if (!copy) return NULL;
    
    for (size_t i = 0; i < list->size; i++) {
        copy->items[i] = rb_value_clone(list->items[i]);
    }
    copy->size = list->size;
    
    return copy;
}

RbList *rb_list_slice(const RbList *list, int start, int end) {
    if (!list) return NULL;
    
    start = normalize_index(start, list->size);
    end = normalize_index(end, list->size);
    
    if (start < 0) start = 0;
    if (end > (int)list->size) end = (int)list->size;
    if (start >= end) return rb_list_new();
    
    size_t slice_size = end - start;
    RbList *slice = rb_list_with_capacity(slice_size);
    if (!slice) return NULL;
    
    for (int i = start; i < end; i++) {
        slice->items[slice->size++] = rb_value_clone(list->items[i]);
    }
    
    return slice;
}

RbList *rb_list_slice_step(const RbList *list, int start, int end, int step) {
    if (!list || step == 0) return NULL;
    
    start = normalize_index(start, list->size);
    end = normalize_index(end, list->size);
    
    if (start < 0) start = 0;
    if (end > (int)list->size) end = (int)list->size;
    
    RbList *slice = rb_list_new();
    if (!slice) return NULL;
    
    if (step > 0) {
        for (int i = start; i < end; i += step) {
            rb_list_append(slice, list->items[i]);
        }
    } else {
        for (int i = start; i > end; i += step) {
            rb_list_append(slice, list->items[i]);
        }
    }
    
    return slice;
}

/* ========== UTILITY ========== */

void rb_list_print(const RbList *list) {
    char *str = rb_list_to_string(list);
    printf("%s", str);
    free(str);
}

char *rb_list_to_string(const RbList *list) {
    if (!list) return strdup("[]");
    
    if (list->size == 0) return strdup("[]");
    
    /* Calculate approximate size */
    size_t total_size = 2; /* [] */
    char **item_strs = (char **)malloc(sizeof(char *) * list->size);
    
    for (size_t i = 0; i < list->size; i++) {
        item_strs[i] = rb_value_to_string(list->items[i]);
        total_size += strlen(item_strs[i]) + 2; /* item + ", " */
    }
    
    char *result = (char *)malloc(total_size + 1);
    char *ptr = result;
    
    *ptr++ = '[';
    for (size_t i = 0; i < list->size; i++) {
        if (i > 0) {
            *ptr++ = ',';
            *ptr++ = ' ';
        }
        size_t len = strlen(item_strs[i]);
        memcpy(ptr, item_strs[i], len);
        ptr += len;
        free(item_strs[i]);
    }
    *ptr++ = ']';
    *ptr = '\0';
    
    free(item_strs);
    return result;
}
