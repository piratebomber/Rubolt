#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../gc/gc.h"
#include "../gc/type_info.h"
#include "../rc/rc.h"

/* ========== EXAMPLE DATA STRUCTURES ========== */

/* Simple node structure */
typedef struct Node {
    int value;
    char *name;
    struct Node *next;
    struct Node *prev;
} Node;

/* Tree node structure */
typedef struct TreeNode {
    int data;
    struct TreeNode *left;
    struct TreeNode *right;
    struct TreeNode *parent;
} TreeNode;

/* Circular list for testing cycles */
typedef struct CircularNode {
    int id;
    struct CircularNode *next;
    struct CircularNode *partner;  /* Creates cycles */
} CircularNode;

/* ========== TYPE REGISTRATION ========== */

/* Register Node type */
TypeInfo *register_node_type(TypeRegistry *registry) {
    static TypeInfo node_type;
    static FieldInfo node_fields[4];
    
    /* Define fields */
    node_fields[0] = field_primitive("value", FIELD_OFFSET(Node, value), FIELD_SIZE(Node, value));
    node_fields[1] = field_string("name", FIELD_OFFSET(Node, name));
    node_fields[2] = field_pointer("next", FIELD_OFFSET(Node, next), &node_type);
    node_fields[3] = field_pointer("prev", FIELD_OFFSET(Node, prev), &node_type);
    
    /* Define type */
    node_type.name = "Node";
    node_type.size = sizeof(Node);
    node_type.field_count = 4;
    node_type.fields = node_fields;
    node_type.destructor = NULL;
    node_type.registered = false;
    node_type.next = NULL;
    
    type_register(registry, &node_type);
    
    /* Fix self-references after registration */
    node_fields[2].referenced_type = &node_type;
    node_fields[3].referenced_type = &node_type;
    
    return &node_type;
}

/* Register TreeNode type */
TypeInfo *register_tree_node_type(TypeRegistry *registry) {
    static TypeInfo tree_type;
    static FieldInfo tree_fields[4];
    
    tree_fields[0] = field_primitive("data", FIELD_OFFSET(TreeNode, data), FIELD_SIZE(TreeNode, data));
    tree_fields[1] = field_pointer("left", FIELD_OFFSET(TreeNode, left), &tree_type);
    tree_fields[2] = field_pointer("right", FIELD_OFFSET(TreeNode, right), &tree_type);
    tree_fields[3] = field_pointer("parent", FIELD_OFFSET(TreeNode, parent), &tree_type);
    
    tree_type.name = "TreeNode";
    tree_type.size = sizeof(TreeNode);
    tree_type.field_count = 4;
    tree_type.fields = tree_fields;
    tree_type.destructor = NULL;
    tree_type.registered = false;
    tree_type.next = NULL;
    
    type_register(registry, &tree_type);
    
    /* Fix self-references */
    tree_fields[1].referenced_type = &tree_type;
    tree_fields[2].referenced_type = &tree_type;
    tree_fields[3].referenced_type = &tree_type;
    
    return &tree_type;
}

/* Register CircularNode type */
TypeInfo *register_circular_node_type(TypeRegistry *registry) {
    static TypeInfo circular_type;
    static FieldInfo circular_fields[3];
    
    circular_fields[0] = field_primitive("id", FIELD_OFFSET(CircularNode, id), FIELD_SIZE(CircularNode, id));
    circular_fields[1] = field_pointer("next", FIELD_OFFSET(CircularNode, next), &circular_type);
    circular_fields[2] = field_pointer("partner", FIELD_OFFSET(CircularNode, partner), &circular_type);
    
    circular_type.name = "CircularNode";
    circular_type.size = sizeof(CircularNode);
    circular_type.field_count = 3;
    circular_type.fields = circular_fields;
    circular_type.destructor = NULL;
    circular_type.registered = false;
    circular_type.next = NULL;
    
    type_register(registry, &circular_type);
    
    circular_fields[1].referenced_type = &circular_type;
    circular_fields[2].referenced_type = &circular_type;
    
    return &circular_type;
}

/* ========== GC TESTS ========== */

void test_gc_with_types() {
    printf("=== Testing GC with Type Information ===\n");
    
    GarbageCollector gc;
    gc_init(&gc);
    
    TypeRegistry registry;
    type_registry_init(&registry);
    
    TypeInfo *node_type = register_node_type(&registry);
    
    /* Allocate typed nodes */
    Node *node1 = (Node *)gc_alloc_typed_zero(&gc, sizeof(Node), node_type);
    node1->value = 100;
    node1->name = (char *)gc_alloc(&gc, 20);
    strcpy(node1->name, "First Node");
    
    Node *node2 = (Node *)gc_alloc_typed_zero(&gc, sizeof(Node), node_type);
    node2->value = 200;
    node2->name = (char *)gc_alloc(&gc, 20);
    strcpy(node2->name, "Second Node");
    
    /* Link nodes */
    node1->next = node2;
    node2->prev = node1;
    
    printf("Created linked nodes:\n");
    printf("  Node1: value=%d, name=%s\n", node1->value, node1->name);
    printf("  Node2: value=%d, name=%s\n", node2->value, node2->name);
    
    /* Add root */
    gc_add_root(&gc, node1);
    
    /* Get stats before GC */
    GCStats stats;
    gc_get_stats(&gc, &stats);
    printf("\nBefore GC:\n");
    printf("  Objects: %zu\n", stats.num_objects);
    printf("  Total allocated: %zu bytes\n", stats.total_allocated);
    printf("  Pointers traversed: %zu\n", stats.pointers_traversed);
    
    /* Run GC - should keep both nodes (reachable from node1) */
    size_t freed = gc_collect(&gc);
    printf("\nGC collected %zu objects\n", freed);
    
    gc_get_stats(&gc, &stats);
    printf("After GC:\n");
    printf("  Objects: %zu\n", stats.num_objects);
    printf("  Total allocated: %zu bytes\n", stats.total_allocated);
    
    gc_shutdown(&gc);
    printf("\nGC with types test completed!\n\n");
}

void test_gc_tree_traversal() {
    printf("=== Testing GC Tree Traversal ===\n");
    
    GarbageCollector gc;
    gc_init(&gc);
    
    TypeRegistry registry;
    type_registry_init(&registry);
    
    TypeInfo *tree_type = register_tree_node_type(&registry);
    
    /* Build a small tree */
    TreeNode *root = (TreeNode *)gc_alloc_typed_zero(&gc, sizeof(TreeNode), tree_type);
    root->data = 1;
    
    TreeNode *left = (TreeNode *)gc_alloc_typed_zero(&gc, sizeof(TreeNode), tree_type);
    left->data = 2;
    left->parent = root;
    
    TreeNode *right = (TreeNode *)gc_alloc_typed_zero(&gc, sizeof(TreeNode), tree_type);
    right->data = 3;
    right->parent = root;
    
    root->left = left;
    root->right = right;
    
    printf("Created tree with root=%d, left=%d, right=%d\n", root->data, left->data, right->data);
    
    /* Only add root */
    gc_add_root(&gc, root);
    
    GCStats stats;
    gc_get_stats(&gc, &stats);
    printf("Objects: %zu, Pointers: %zu\n", stats.num_objects, stats.pointers_traversed);
    
    /* Run GC - should keep all (reachable through tree) */
    size_t freed = gc_collect(&gc);
    printf("GC collected %zu objects (should be 0)\n", freed);
    
    gc_shutdown(&gc);
    printf("Tree traversal test completed!\n\n");
}

/* ========== RC TESTS ========== */

void test_rc_cycles() {
    printf("=== Testing RC Cycle Detection ===\n");
    
    RefCounter rc;
    rc_init(&rc);
    
    TypeRegistry registry;
    type_registry_init(&registry);
    
    TypeInfo *circular_type = register_circular_node_type(&registry);
    
    /* Create circular references */
    CircularNode *node_a = (CircularNode *)malloc(sizeof(CircularNode));
    node_a->id = 1;
    
    CircularNode *node_b = (CircularNode *)malloc(sizeof(CircularNode));
    node_b->id = 2;
    
    CircularNode *node_c = (CircularNode *)malloc(sizeof(CircularNode));
    node_c->id = 3;
    
    /* Create RC objects */
    RCObject *obj_a = rc_new_typed(&rc, node_a, 0, circular_type, free);
    RCObject *obj_b = rc_new_typed(&rc, node_b, 0, circular_type, free);
    RCObject *obj_c = rc_new_typed(&rc, node_c, 0, circular_type, free);
    
    printf("Created 3 RC objects with IDs: %d, %d, %d\n", node_a->id, node_b->id, node_c->id);
    
    /* Create cycle: A -> B -> C -> A */
    node_a->next = node_b;
    node_b->next = node_c;
    node_c->next = node_a;
    
    /* Also create partner links (more cycles) */
    node_a->partner = node_c;
    node_c->partner = node_a;
    
    /* Retain to create circular references */
    rc_retain(&rc, obj_b);
    rc_retain(&rc, obj_c);
    rc_retain(&rc, obj_a);
    
    printf("Created circular references\n");
    printf("  Object A refs: %zu\n", rc_get_count(obj_a));
    printf("  Object B refs: %zu\n", rc_get_count(obj_b));
    printf("  Object C refs: %zu\n", rc_get_count(obj_c));
    
    /* Mark for cycle detection */
    rc_mark_for_cycle_detection(&rc, obj_a);
    rc_mark_for_cycle_detection(&rc, obj_b);
    rc_mark_for_cycle_detection(&rc, obj_c);
    
    RCStats stats;
    rc_get_stats(&rc, &stats);
    printf("\nBefore cycle collection:\n");
    printf("  Total objects: %zu\n", stats.total_objects);
    printf("  Cycle buffer size: %zu\n", stats.cycle_buffer_size);
    printf("  Objects in cycles: %zu\n", stats.objects_in_cycles);
    
    /* Release external references */
    rc_release(&rc, obj_a);
    rc_release(&rc, obj_b);
    rc_release(&rc, obj_c);
    
    printf("\nAfter releasing external refs (objects still have internal refs)\n");
    printf("  Object A refs: %zu\n", rc_get_count(obj_a));
    printf("  Object B refs: %zu\n", rc_get_count(obj_b));
    printf("  Object C refs: %zu\n", rc_get_count(obj_c));
    
    /* Run cycle detection */
    size_t collected = rc_collect_cycles(&rc);
    printf("\nCycle collection freed %zu objects\n", collected);
    
    rc_get_stats(&rc, &stats);
    printf("\nAfter cycle collection:\n");
    printf("  Total objects: %zu\n", stats.total_objects);
    printf("  Cycles detected: %zu\n", stats.cycles_detected);
    printf("  Cycles collected: %zu\n", stats.cycles_collected);
    
    rc_shutdown(&rc);
    printf("\nRC cycle detection test completed!\n\n");
}

void test_rc_complex_graph() {
    printf("=== Testing RC Complex Object Graph ===\n");
    
    RefCounter rc;
    rc_init(&rc);
    
    TypeRegistry registry;
    type_registry_init(&registry);
    
    TypeInfo *node_type = register_node_type(&registry);
    
    /* Create a complex graph */
    Node *nodes[5];
    RCObject *rc_objs[5];
    
    for (int i = 0; i < 5; i++) {
        nodes[i] = (Node *)malloc(sizeof(Node));
        nodes[i]->value = i * 10;
        nodes[i]->name = (char *)malloc(20);
        snprintf(nodes[i]->name, 20, "Node %d", i);
        nodes[i]->next = NULL;
        nodes[i]->prev = NULL;
        
        rc_objs[i] = rc_new_typed(&rc, nodes[i], 0, node_type, free);
    }
    
    /* Create complex connections */
    nodes[0]->next = nodes[1];
    nodes[1]->prev = nodes[0];
    nodes[1]->next = nodes[2];
    nodes[2]->prev = nodes[1];
    nodes[2]->next = nodes[3];
    nodes[3]->prev = nodes[2];
    nodes[3]->next = nodes[4];
    nodes[4]->prev = nodes[3];
    
    /* Create a cycle */
    nodes[4]->next = nodes[1];  /* Back to node 1 */
    
    printf("Created graph with 5 nodes and a cycle\n");
    
    /* Add all to cycle buffer */
    for (int i = 0; i < 5; i++) {
        rc_mark_for_cycle_detection(&rc, rc_objs[i]);
    }
    
    /* Keep external ref to node 0 only */
    for (int i = 1; i < 5; i++) {
        rc_release(&rc, rc_objs[i]);
    }
    
    RCStats stats;
    rc_get_stats(&rc, &stats);
    printf("Before cycle detection: %zu objects, %zu in cycle buffer\n", 
           stats.total_objects, stats.cycle_buffer_size);
    
    /* Run cycle detection */
    size_t collected = rc_collect_cycles(&rc);
    printf("Collected %zu objects in cycles\n", collected);
    
    rc_get_stats(&rc, &stats);
    printf("After: %zu objects remain\n", stats.total_objects);
    
    /* Cleanup remaining */
    rc_release(&rc, rc_objs[0]);
    rc_shutdown(&rc);
    
    printf("Complex graph test completed!\n\n");
}

/* ========== MAIN ========== */

int main() {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  Advanced Memory Management Tests            ║\n");
    printf("║  (Type Registration & Cycle Detection)        ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    /* Initialize global type registry */
    global_type_registry = (TypeRegistry *)malloc(sizeof(TypeRegistry));
    type_registry_init(global_type_registry);
    
    /* GC tests */
    test_gc_with_types();
    test_gc_tree_traversal();
    
    /* RC tests */
    test_rc_cycles();
    test_rc_complex_graph();
    
    /* Cleanup */
    free(global_type_registry);
    
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║         All Tests Completed!                  ║\n");
    printf("╚═══════════════════════════════════════════════╝\n");
    
    return 0;
}
