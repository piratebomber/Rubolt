#ifndef RUBOLT_GC_H
#define RUBOLT_GC_H

#include <stddef.h>
#include <stdbool.h>
#include "type_info.h"

/* Memory pool size classes for small objects */
#define GC_POOL_8_BYTES     0
#define GC_POOL_16_BYTES    1
#define GC_POOL_32_BYTES    2
#define GC_POOL_64_BYTES    3
#define GC_POOL_128_BYTES   4
#define GC_POOL_256_BYTES   5
#define GC_NUM_POOLS        6

/* GC configuration constants */
#define GC_INITIAL_THRESHOLD    (1024 * 1024)  /* 1 MB */
#define GC_GROWTH_FACTOR        2.0
#define GC_MIN_THRESHOLD        (512 * 1024)   /* 512 KB */
#define GC_POOL_BLOCK_SIZE      4096           /* 4 KB blocks */

/* Object header for garbage collection */
typedef struct GCObjectHeader {
    struct GCObjectHeader *next;     /* Linked list of all objects */
    size_t size;                     /* Size of object in bytes */
    TypeInfo *type_info;             /* Type information for traversal */
    unsigned char marked : 1;        /* Mark bit for mark-sweep */
    unsigned char pooled : 1;        /* Is this from a memory pool? */
    unsigned char pool_class : 6;    /* Which pool (if pooled) */
} GCObjectHeader;

/* Memory pool for small objects */
typedef struct GCPoolBlock {
    struct GCPoolBlock *next;
    unsigned char *data;
    size_t capacity;
    size_t used;
} GCPoolBlock;

typedef struct GCPool {
    size_t object_size;
    GCPoolBlock *blocks;
    void **free_list;  /* Free list for quick allocation */
} GCPool;

/* Main garbage collector structure */
typedef struct GarbageCollector {
    GCObjectHeader *objects;         /* All allocated objects */
    GCPool pools[GC_NUM_POOLS];     /* Memory pools */
    size_t bytes_allocated;          /* Total bytes allocated */
    size_t next_gc;                  /* Threshold for next GC */
    size_t num_objects;              /* Number of tracked objects */
    bool gc_enabled;                 /* Can disable GC temporarily */
    void **roots;                    /* GC roots for marking */
    size_t num_roots;
    size_t root_capacity;
} GarbageCollector;

/* Initialize the garbage collector */
void gc_init(GarbageCollector *gc);

/* Shutdown and free all memory */
void gc_shutdown(GarbageCollector *gc);

/* Allocate memory with GC tracking */
void *gc_alloc(GarbageCollector *gc, size_t size);

/* Allocate memory with type information */
void *gc_alloc_typed(GarbageCollector *gc, size_t size, TypeInfo *type_info);

/* Allocate memory with GC tracking and zero initialization */
void *gc_alloc_zero(GarbageCollector *gc, size_t size);

/* Allocate typed memory with zero initialization */
void *gc_alloc_typed_zero(GarbageCollector *gc, size_t size, TypeInfo *type_info);

/* Reallocate memory (moves object if needed) */
void *gc_realloc(GarbageCollector *gc, void *ptr, size_t new_size);

/* Free a specific object (manual free) */
void gc_free(GarbageCollector *gc, void *ptr);

/* Run garbage collection cycle */
size_t gc_collect(GarbageCollector *gc);

/* Force a garbage collection */
size_t gc_collect_force(GarbageCollector *gc);

/* Add a GC root */
void gc_add_root(GarbageCollector *gc, void *root);

/* Remove a GC root */
void gc_remove_root(GarbageCollector *gc, void *root);

/* Temporarily disable/enable GC */
void gc_disable(GarbageCollector *gc);
void gc_enable(GarbageCollector *gc);

/* Get GC statistics */
typedef struct GCStats {
    size_t total_allocated;
    size_t num_objects;
    size_t pool_allocated[GC_NUM_POOLS];
    size_t heap_allocated;
    size_t next_gc_threshold;
    size_t objects_marked;
    size_t objects_swept;
    size_t pointers_traversed;
} GCStats;

void gc_get_stats(GarbageCollector *gc, GCStats *stats);

/* Mark an object as reachable (for mark phase) */
void gc_mark_object(GarbageCollector *gc, void *ptr);

/* Internal pool functions */
void gc_pool_init(GCPool *pool, size_t object_size);
void gc_pool_shutdown(GCPool *pool);
void *gc_pool_alloc(GCPool *pool);
void gc_pool_free(GCPool *pool, void *ptr, size_t object_size);

/* Get pool class for a given size */
int gc_get_pool_class(size_t size);

/* Global GC instance (can be used throughout Rubolt) */
extern GarbageCollector *rubolt_gc;

#endif /* RUBOLT_GC_H */
