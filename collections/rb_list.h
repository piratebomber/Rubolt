#ifndef RB_LIST_H
#define RB_LIST_H

#include "rb_collections.h"
#include <stddef.h>
#include <stdbool.h>

/* Dynamic array structure */
typedef struct RbList {
    RbValue *items;
    size_t size;
    size_t capacity;
} RbList;

/* ========== CREATION & DESTRUCTION ========== */

/* Create a new empty list */
RbList *rb_list_new(void);

/* Create a list with initial capacity */
RbList *rb_list_with_capacity(size_t capacity);

/* Create a list from array */
RbList *rb_list_from_array(RbValue *items, size_t count);

/* Free a list */
void rb_list_free(RbList *list);

/* Clear all items from list */
void rb_list_clear(RbList *list);

/* ========== BASIC OPERATIONS ========== */

/* Get size of list */
size_t rb_list_len(const RbList *list);

/* Check if list is empty */
bool rb_list_is_empty(const RbList *list);

/* Get item at index */
RbValue rb_list_get(const RbList *list, int index);

/* Set item at index */
void rb_list_set(RbList *list, int index, RbValue value);

/* Append item to end */
void rb_list_append(RbList *list, RbValue value);

/* Insert item at index */
void rb_list_insert(RbList *list, int index, RbValue value);

/* Remove and return item at index */
RbValue rb_list_pop(RbList *list, int index);

/* Remove first occurrence of value */
bool rb_list_remove(RbList *list, RbValue value);

/* Extend list with another list */
void rb_list_extend(RbList *list, const RbList *other);

/* ========== SEARCH & COUNT ========== */

/* Find index of first occurrence of value (-1 if not found) */
int rb_list_index(const RbList *list, RbValue value);

/* Count occurrences of value */
size_t rb_list_count(const RbList *list, RbValue value);

/* Check if list contains value */
bool rb_list_contains(const RbList *list, RbValue value);

/* ========== SORTING & REVERSING ========== */

/* Reverse list in place */
void rb_list_reverse(RbList *list);

/* Sort list in place */
void rb_list_sort(RbList *list, RbCompareFn compare);

/* ========== SLICING & COPYING ========== */

/* Create a shallow copy */
RbList *rb_list_copy(const RbList *list);

/* Create a slice [start:end] */
RbList *rb_list_slice(const RbList *list, int start, int end);

/* Create a slice with step [start:end:step] */
RbList *rb_list_slice_step(const RbList *list, int start, int end, int step);

/* ========== PYTHON-LIKE METHODS ========== */

/* list.append(x) */
#define rb_list_py_append rb_list_append

/* list.pop() - removes and returns last item */
static inline RbValue rb_list_py_pop(RbList *list) {
    return rb_list_pop(list, -1);
}

/* list.clear() */
#define rb_list_py_clear rb_list_clear

/* list.reverse() */
#define rb_list_py_reverse rb_list_reverse

/* list.sort() */
#define rb_list_py_sort(list) rb_list_sort(list, NULL)

/* len(list) */
#define rb_list_py_len rb_list_len

/* ========== UTILITY ========== */

/* Print list */
void rb_list_print(const RbList *list);

/* Convert list to string */
char *rb_list_to_string(const RbList *list);

#endif /* RB_LIST_H */
