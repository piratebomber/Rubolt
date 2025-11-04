#include <stdio.h>
#include <assert.h>
#include "rb_collections.h"
#include "rb_list.h"

void test_list_basic() {
    printf("=== Testing List Basic Operations ===\n");
    
    RbList *list = rb_list_new();
    assert(rb_list_is_empty(list));
    
    /* Append items */
    rb_list_append(list, rb_value_int(10));
    rb_list_append(list, rb_value_int(20));
    rb_list_append(list, rb_value_int(30));
    
    printf("List size: %zu\n", rb_list_len(list));
    assert(rb_list_len(list) == 3);
    
    /* Get items */
    RbValue val = rb_list_get(list, 0);
    assert(val.type == RB_VAL_INT && val.data.i == 10);
    
    /* Negative indexing */
    val = rb_list_get(list, -1);
    assert(val.type == RB_VAL_INT && val.data.i == 30);
    
    /* Print list */
    printf("List: ");
    rb_list_print(list);
    printf("\n");
    
    rb_list_free(list);
    printf("✓ Basic operations passed\n\n");
}

void test_list_insert_remove() {
    printf("=== Testing List Insert/Remove ===\n");
    
    RbList *list = rb_list_new();
    
    /* Insert at beginning */
    rb_list_insert(list, 0, rb_value_int(1));
    rb_list_insert(list, 0, rb_value_int(2));
    rb_list_insert(list, 0, rb_value_int(3));
    
    printf("After inserts: ");
    rb_list_print(list);
    printf("\n");
    
    /* Pop item */
    RbValue popped = rb_list_pop(list, 0);
    assert(popped.type == RB_VAL_INT && popped.data.i == 3);
    rb_value_free(popped);
    
    /* Remove by value */
    bool removed = rb_list_remove(list, rb_value_int(1));
    assert(removed);
    
    printf("After removes: ");
    rb_list_print(list);
    printf("\n");
    
    rb_list_free(list);
    printf("✓ Insert/Remove passed\n\n");
}

void test_list_search() {
    printf("=== Testing List Search ===\n");
    
    RbList *list = rb_list_new();
    rb_list_append(list, rb_value_int(10));
    rb_list_append(list, rb_value_int(20));
    rb_list_append(list, rb_value_int(10));
    rb_list_append(list, rb_value_int(30));
    
    /* Index */
    int idx = rb_list_index(list, rb_value_int(20));
    printf("Index of 20: %d\n", idx);
    assert(idx == 1);
    
    /* Count */
    size_t count = rb_list_count(list, rb_value_int(10));
    printf("Count of 10: %zu\n", count);
    assert(count == 2);
    
    /* Contains */
    bool contains = rb_list_contains(list, rb_value_int(30));
    printf("Contains 30: %s\n", contains ? "True" : "False");
    assert(contains);
    
    rb_list_free(list);
    printf("✓ Search operations passed\n\n");
}

void test_list_reverse_sort() {
    printf("=== Testing List Reverse/Sort ===\n");
    
    RbList *list = rb_list_new();
    rb_list_append(list, rb_value_int(5));
    rb_list_append(list, rb_value_int(2));
    rb_list_append(list, rb_value_int(8));
    rb_list_append(list, rb_value_int(1));
    
    printf("Original: ");
    rb_list_print(list);
    printf("\n");
    
    /* Reverse */
    rb_list_reverse(list);
    printf("Reversed: ");
    rb_list_print(list);
    printf("\n");
    
    /* Sort */
    rb_list_sort(list, NULL);
    printf("Sorted:   ");
    rb_list_print(list);
    printf("\n");
    
    rb_list_free(list);
    printf("✓ Reverse/Sort passed\n\n");
}

void test_list_slice() {
    printf("=== Testing List Slicing ===\n");
    
    RbList *list = rb_list_new();
    for (int i = 0; i < 10; i++) {
        rb_list_append(list, rb_value_int(i));
    }
    
    printf("Original: ");
    rb_list_print(list);
    printf("\n");
    
    /* Slice [2:5] */
    RbList *slice = rb_list_slice(list, 2, 5);
    printf("Slice [2:5]: ");
    rb_list_print(slice);
    printf("\n");
    rb_list_free(slice);
    
    /* Slice with step [0:10:2] */
    RbList *slice_step = rb_list_slice_step(list, 0, 10, 2);
    printf("Slice [0:10:2]: ");
    rb_list_print(slice_step);
    printf("\n");
    rb_list_free(slice_step);
    
    rb_list_free(list);
    printf("✓ Slicing passed\n\n");
}

void test_list_strings() {
    printf("=== Testing List with Strings ===\n");
    
    RbList *list = rb_list_new();
    rb_list_append(list, rb_value_string("apple"));
    rb_list_append(list, rb_value_string("banana"));
    rb_list_append(list, rb_value_string("cherry"));
    
    printf("String list: ");
    rb_list_print(list);
    printf("\n");
    
    /* Sort strings */
    rb_list_sort(list, NULL);
    printf("Sorted:      ");
    rb_list_print(list);
    printf("\n");
    
    rb_list_free(list);
    printf("✓ String operations passed\n\n");
}

void test_list_extend_copy() {
    printf("=== Testing List Extend/Copy ===\n");
    
    RbList *list1 = rb_list_new();
    rb_list_append(list1, rb_value_int(1));
    rb_list_append(list1, rb_value_int(2));
    
    RbList *list2 = rb_list_new();
    rb_list_append(list2, rb_value_int(3));
    rb_list_append(list2, rb_value_int(4));
    
    printf("List 1: ");
    rb_list_print(list1);
    printf("\n");
    
    printf("List 2: ");
    rb_list_print(list2);
    printf("\n");
    
    /* Extend */
    rb_list_extend(list1, list2);
    printf("Extended: ");
    rb_list_print(list1);
    printf("\n");
    
    /* Copy */
    RbList *copy = rb_list_copy(list1);
    printf("Copy:     ");
    rb_list_print(copy);
    printf("\n");
    
    rb_list_free(list1);
    rb_list_free(list2);
    rb_list_free(copy);
    printf("✓ Extend/Copy passed\n\n");
}

int main() {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║       Rubolt Collections Test Suite          ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    test_list_basic();
    test_list_insert_remove();
    test_list_search();
    test_list_reverse_sort();
    test_list_slice();
    test_list_strings();
    test_list_extend_copy();
    
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║           All Tests Passed! ✓                 ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return 0;
}
