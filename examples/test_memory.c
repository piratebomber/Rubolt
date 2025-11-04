#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../gc/gc.h"
#include "../rc/rc.h"

/* Example destructor */
void string_destructor(void *data) {
    printf("Destroying string: %s\n", (char *)data);
}

/* Test garbage collector */
void test_gc() {
    printf("=== Testing Garbage Collector ===\n");
    
    GarbageCollector gc;
    gc_init(&gc);
    
    /* Allocate some objects */
    char *str1 = (char *)gc_alloc(&gc, 20);
    strcpy(str1, "Hello, GC!");
    printf("Allocated: %s\n", str1);
    
    int *nums = (int *)gc_alloc(&gc, sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        nums[i] = i * 10;
    }
    printf("Allocated array of 10 integers\n");
    
    /* Test memory pool allocation (small objects) */
    for (int i = 0; i < 100; i++) {
        void *small = gc_alloc(&gc, 8);
        if (i == 0) printf("Allocated small object from pool\n");
    }
    
    /* Get stats before GC */
    GCStats stats;
    gc_get_stats(&gc, &stats);
    printf("\nBefore GC:\n");
    printf("  Total allocated: %zu bytes\n", stats.total_allocated);
    printf("  Objects: %zu\n", stats.num_objects);
    printf("  Heap allocated: %zu bytes\n", stats.heap_allocated);
    
    /* Add some roots */
    gc_add_root(&gc, str1);
    gc_add_root(&gc, nums);
    
    /* Run garbage collection */
    size_t freed = gc_collect(&gc);
    printf("\nGC collected %zu objects\n", freed);
    
    /* Get stats after GC */
    gc_get_stats(&gc, &stats);
    printf("\nAfter GC:\n");
    printf("  Total allocated: %zu bytes\n", stats.total_allocated);
    printf("  Objects: %zu\n", stats.num_objects);
    printf("  Heap allocated: %zu bytes\n", stats.heap_allocated);
    
    /* Cleanup */
    gc_shutdown(&gc);
    printf("\nGC test completed!\n\n");
}

/* Test reference counter */
void test_rc() {
    printf("=== Testing Reference Counter ===\n");
    
    RefCounter rc;
    rc_init(&rc);
    
    /* Create reference-counted string */
    char *data = strdup("Hello, RC!");
    RCObject *obj1 = rc_new(&rc, data, 0, string_destructor);
    printf("Created RC object: %s (refs: %zu)\n", (char *)obj1->data, rc_get_count(obj1));
    
    /* Retain (acquire) */
    rc_retain(&rc, obj1);
    printf("After retain: refs = %zu\n", rc_get_count(obj1));
    
    /* Create weak reference */
    RCWeakRef *weak = rc_weak_new(&rc, obj1);
    printf("Created weak reference\n");
    
    /* Release */
    rc_release(&rc, obj1);
    printf("After release: refs = %zu\n", rc_get_count(obj1));
    
    /* Try to lock weak ref */
    RCObject *locked = rc_weak_lock(weak);
    if (locked) {
        printf("Weak ref still valid, promoted to strong\n");
    }
    
    /* Release again - should free */
    printf("Final release:\n");
    rc_release(&rc, obj1);
    
    /* Try to lock again */
    locked = rc_weak_lock(weak);
    if (!locked) {
        printf("Weak ref now invalid (object freed)\n");
    }
    
    /* Get stats */
    RCStats stats;
    rc_get_stats(&rc, &stats);
    printf("\nRC Stats:\n");
    printf("  Total objects: %zu\n", stats.total_objects);
    printf("  Total refs: %zu\n", stats.total_refs);
    printf("  Cycle buffer size: %zu\n", stats.cycle_buffer_size);
    
    /* Cleanup */
    rc_weak_release(&rc, weak);
    rc_shutdown(&rc);
    printf("\nRC test completed!\n\n");
}

/* Test cycle detection */
void test_cycle_detection() {
    printf("=== Testing Cycle Detection ===\n");
    
    RefCounter rc;
    rc_init(&rc);
    
    /* Create circular reference (simplified) */
    char *data1 = strdup("Object A");
    char *data2 = strdup("Object B");
    
    RCObject *obj_a = rc_new(&rc, data1, 0, string_destructor);
    RCObject *obj_b = rc_new(&rc, data2, 0, string_destructor);
    
    printf("Created two objects\n");
    printf("Object A refs: %zu\n", rc_get_count(obj_a));
    printf("Object B refs: %zu\n", rc_get_count(obj_b));
    
    /* Create circular reference */
    rc_retain(&rc, obj_a);
    rc_retain(&rc, obj_b);
    
    printf("\nAfter mutual retain:\n");
    printf("Object A refs: %zu\n", rc_get_count(obj_a));
    printf("Object B refs: %zu\n", rc_get_count(obj_b));
    
    /* Mark for cycle detection */
    rc_mark_for_cycle_detection(&rc, obj_a);
    rc_mark_for_cycle_detection(&rc, obj_b);
    
    /* Try to detect cycles */
    size_t cycles = rc_collect_cycles(&rc);
    printf("Detected and collected %zu cycles\n", cycles);
    
    /* Cleanup */
    rc_release(&rc, obj_a);
    rc_release(&rc, obj_b);
    rc_shutdown(&rc);
    
    printf("\nCycle detection test completed!\n\n");
}

int main() {
    printf("╔═══════════════════════════════════════╗\n");
    printf("║   Rubolt Memory Management Tests     ║\n");
    printf("╚═══════════════════════════════════════╝\n\n");
    
    test_gc();
    test_rc();
    test_cycle_detection();
    
    printf("╔═══════════════════════════════════════╗\n");
    printf("║      All Tests Completed!             ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    
    return 0;
}
