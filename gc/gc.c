#include "gc.h"
#include "type_info.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global GC instance */
GarbageCollector *rubolt_gc = NULL;

/* Pool size lookup table */
static const size_t pool_sizes[GC_NUM_POOLS] = {8, 16, 32, 64, 128, 256};

/* Get the appropriate pool class for a size */
int gc_get_pool_class(size_t size) {
    for (int i = 0; i < GC_NUM_POOLS; i++) {
        if (size <= pool_sizes[i]) {
            return i;
        }
    }
    return -1; /* Too large for pooling */
}

/* Initialize a memory pool */
void gc_pool_init(GCPool *pool, size_t object_size) {
    pool->object_size = object_size;
    pool->blocks = NULL;
    pool->free_list = NULL;
}

/* Shutdown and free a memory pool */
void gc_pool_shutdown(GCPool *pool) {
    GCPoolBlock *block = pool->blocks;
    while (block) {
        GCPoolBlock *next = block->next;
        free(block->data);
        free(block);
        block = next;
    }
    pool->blocks = NULL;
    pool->free_list = NULL;
}

/* Allocate a new pool block */
static GCPoolBlock *gc_pool_block_create(size_t object_size) {
    GCPoolBlock *block = (GCPoolBlock *)malloc(sizeof(GCPoolBlock));
    if (!block) return NULL;
    
    block->capacity = GC_POOL_BLOCK_SIZE;
    block->data = (unsigned char *)malloc(block->capacity);
    if (!block->data) {
        free(block);
        return NULL;
    }
    
    block->used = 0;
    block->next = NULL;
    return block;
}

/* Allocate from a memory pool */
void *gc_pool_alloc(GCPool *pool) {
    /* Try free list first */
    if (pool->free_list && *pool->free_list) {
        void *ptr = *pool->free_list;
        pool->free_list = (void **)*pool->free_list;
        return ptr;
    }
    
    /* Find or create a block with space */
    GCPoolBlock *block = pool->blocks;
    while (block) {
        if (block->used + pool->object_size <= block->capacity) {
            void *ptr = block->data + block->used;
            block->used += pool->object_size;
            return ptr;
        }
        block = block->next;
    }
    
    /* Need a new block */
    GCPoolBlock *new_block = gc_pool_block_create(pool->object_size);
    if (!new_block) return NULL;
    
    new_block->next = pool->blocks;
    pool->blocks = new_block;
    
    void *ptr = new_block->data;
    new_block->used = pool->object_size;
    return ptr;
}

/* Free back to pool (add to free list) */
void gc_pool_free(GCPool *pool, void *ptr, size_t object_size) {
    if (!ptr) return;
    
    /* Add to free list */
    void **node = (void **)ptr;
    *node = pool->free_list;
    pool->free_list = node;
}

/* Initialize the garbage collector */
void gc_init(GarbageCollector *gc) {
    gc->objects = NULL;
    gc->bytes_allocated = 0;
    gc->next_gc = GC_INITIAL_THRESHOLD;
    gc->num_objects = 0;
    gc->gc_enabled = true;
    
    /* Initialize memory pools */
    for (int i = 0; i < GC_NUM_POOLS; i++) {
        gc_pool_init(&gc->pools[i], pool_sizes[i]);
    }
    
    /* Initialize roots */
    gc->root_capacity = 16;
    gc->roots = (void **)malloc(sizeof(void *) * gc->root_capacity);
    gc->num_roots = 0;
}

/* Shutdown the garbage collector */
void gc_shutdown(GarbageCollector *gc) {
    /* Free all objects */
    GCObjectHeader *obj = gc->objects;
    while (obj) {
        GCObjectHeader *next = obj->next;
        if (!obj->pooled) {
            free(obj);
        }
        obj = next;
    }
    
    /* Free memory pools */
    for (int i = 0; i < GC_NUM_POOLS; i++) {
        gc_pool_shutdown(&gc->pools[i]);
    }
    
    /* Free roots */
    free(gc->roots);
    
    gc->objects = NULL;
    gc->bytes_allocated = 0;
    gc->num_objects = 0;
}

/* Get object header from user pointer */
static inline GCObjectHeader *gc_get_header(void *ptr) {
    if (!ptr) return NULL;
    return (GCObjectHeader *)((char *)ptr - sizeof(GCObjectHeader));
}

/* Get user pointer from object header */
static inline void *gc_get_pointer(GCObjectHeader *header) {
    if (!header) return NULL;
    return (void *)((char *)header + sizeof(GCObjectHeader));
}

/* Allocate memory with GC tracking */
void *gc_alloc(GarbageCollector *gc, size_t size) {
    if (size == 0) return NULL;
    
    /* Check if we should run GC */
    if (gc->gc_enabled && gc->bytes_allocated >= gc->next_gc) {
        gc_collect(gc);
    }
    
    GCObjectHeader *header = NULL;
    int pool_class = gc_get_pool_class(size + sizeof(GCObjectHeader));
    
    if (pool_class >= 0) {
        /* Use memory pool */
        header = (GCObjectHeader *)gc_pool_alloc(&gc->pools[pool_class]);
        if (header) {
            header->pooled = 1;
            header->pool_class = pool_class;
            gc->bytes_allocated += pool_sizes[pool_class];
        }
    } else {
        /* Use standard malloc */
        header = (GCObjectHeader *)malloc(sizeof(GCObjectHeader) + size);
        if (header) {
            header->pooled = 0;
            header->pool_class = 0;
            gc->bytes_allocated += sizeof(GCObjectHeader) + size;
        }
    }
    
    if (!header) return NULL;
    
    header->size = size;
    header->type_info = NULL;
    header->marked = 0;
    header->next = gc->objects;
    gc->objects = header;
    gc->num_objects++;
    
    return gc_get_pointer(header);
}

/* Allocate memory with type information */
void *gc_alloc_typed(GarbageCollector *gc, size_t size, TypeInfo *type_info) {
    void *ptr = gc_alloc(gc, size);
    if (ptr) {
        GCObjectHeader *header = gc_get_header(ptr);
        header->type_info = type_info;
    }
    return ptr;
}

/* Allocate typed memory with zero initialization */
void *gc_alloc_typed_zero(GarbageCollector *gc, size_t size, TypeInfo *type_info) {
    void *ptr = gc_alloc_zero(gc, size);
    if (ptr) {
        GCObjectHeader *header = gc_get_header(ptr);
        header->type_info = type_info;
    }
    return ptr;
}

/* Allocate zeroed memory */
void *gc_alloc_zero(GarbageCollector *gc, size_t size) {
    void *ptr = gc_alloc(gc, size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/* Reallocate memory */
void *gc_realloc(GarbageCollector *gc, void *ptr, size_t new_size) {
    if (!ptr) return gc_alloc(gc, new_size);
    if (new_size == 0) {
        gc_free(gc, ptr);
        return NULL;
    }
    
    GCObjectHeader *old_header = gc_get_header(ptr);
    if (!old_header) return NULL;
    
    /* Allocate new memory */
    void *new_ptr = gc_alloc(gc, new_size);
    if (!new_ptr) return NULL;
    
    /* Copy old data */
    size_t copy_size = old_header->size < new_size ? old_header->size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    
    /* Free old memory */
    gc_free(gc, ptr);
    
    return new_ptr;
}

/* Free a specific object */
void gc_free(GarbageCollector *gc, void *ptr) {
    if (!ptr) return;
    
    GCObjectHeader *header = gc_get_header(ptr);
    if (!header) return;
    
    /* Remove from object list */
    if (gc->objects == header) {
        gc->objects = header->next;
    } else {
        GCObjectHeader *prev = gc->objects;
        while (prev && prev->next != header) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = header->next;
        }
    }
    
    /* Update stats */
    if (header->pooled) {
        gc->bytes_allocated -= pool_sizes[header->pool_class];
        gc_pool_free(&gc->pools[header->pool_class], header, pool_sizes[header->pool_class]);
    } else {
        gc->bytes_allocated -= sizeof(GCObjectHeader) + header->size;
        free(header);
    }
    
    gc->num_objects--;
}

/* Context for marking visitor */
typedef struct MarkContext {
    GarbageCollector *gc;
    size_t pointers_traversed;
} MarkContext;

/* Visitor function for marking referenced objects */
static void mark_visitor(void *object, void *pointer_field, void *context) {
    MarkContext *ctx = (MarkContext *)context;
    ctx->pointers_traversed++;
    
    /* Recursively mark the referenced object */
    gc_mark_object(ctx->gc, pointer_field);
}

/* Mark an object as reachable */
void gc_mark_object(GarbageCollector *gc, void *ptr) {
    if (!ptr) return;
    
    GCObjectHeader *header = gc_get_header(ptr);
    if (!header || header->marked) return;
    
    header->marked = 1;
    
    /* Traverse object contents if it has type information */
    if (header->type_info && type_has_pointers(header->type_info)) {
        MarkContext ctx;
        ctx.gc = gc;
        ctx.pointers_traversed = 0;
        
        type_traverse_pointers(header->type_info, ptr, mark_visitor, &ctx);
    }
}

/* Mark phase - mark all reachable objects from roots */
static void gc_mark_phase(GarbageCollector *gc) {
    /* Mark all objects reachable from roots */
    for (size_t i = 0; i < gc->num_roots; i++) {
        gc_mark_object(gc, gc->roots[i]);
    }
}

/* Sweep phase - free unmarked objects */
static size_t gc_sweep_phase(GarbageCollector *gc) {
    size_t freed = 0;
    GCObjectHeader *obj = gc->objects;
    GCObjectHeader *prev = NULL;
    
    while (obj) {
        GCObjectHeader *next = obj->next;
        
        if (!obj->marked) {
            /* Unreachable - free it */
            if (prev) {
                prev->next = next;
            } else {
                gc->objects = next;
            }
            
            if (obj->pooled) {
                gc->bytes_allocated -= pool_sizes[obj->pool_class];
                gc_pool_free(&gc->pools[obj->pool_class], obj, pool_sizes[obj->pool_class]);
            } else {
                gc->bytes_allocated -= sizeof(GCObjectHeader) + obj->size;
                free(obj);
            }
            
            gc->num_objects--;
            freed++;
        } else {
            /* Reachable - unmark for next cycle */
            obj->marked = 0;
            prev = obj;
        }
        
        obj = next;
    }
    
    return freed;
}

/* Run garbage collection cycle */
size_t gc_collect(GarbageCollector *gc) {
    if (!gc->gc_enabled) return 0;
    
    size_t before = gc->num_objects;
    
    /* Mark phase */
    gc_mark_phase(gc);
    
    /* Sweep phase */
    size_t freed = gc_sweep_phase(gc);
    
    /* Update GC threshold */
    gc->next_gc = gc->bytes_allocated * GC_GROWTH_FACTOR;
    if (gc->next_gc < GC_MIN_THRESHOLD) {
        gc->next_gc = GC_MIN_THRESHOLD;
    }
    
    return freed;
}

/* Force garbage collection */
size_t gc_collect_force(GarbageCollector *gc) {
    bool was_enabled = gc->gc_enabled;
    gc->gc_enabled = true;
    size_t freed = gc_collect(gc);
    gc->gc_enabled = was_enabled;
    return freed;
}

/* Add a GC root */
void gc_add_root(GarbageCollector *gc, void *root) {
    if (!root) return;
    
    /* Grow roots array if needed */
    if (gc->num_roots >= gc->root_capacity) {
        gc->root_capacity *= 2;
        gc->roots = (void **)realloc(gc->roots, sizeof(void *) * gc->root_capacity);
    }
    
    gc->roots[gc->num_roots++] = root;
}

/* Remove a GC root */
void gc_remove_root(GarbageCollector *gc, void *root) {
    for (size_t i = 0; i < gc->num_roots; i++) {
        if (gc->roots[i] == root) {
            /* Shift remaining roots */
            for (size_t j = i; j < gc->num_roots - 1; j++) {
                gc->roots[j] = gc->roots[j + 1];
            }
            gc->num_roots--;
            return;
        }
    }
}

/* Disable GC temporarily */
void gc_disable(GarbageCollector *gc) {
    gc->gc_enabled = false;
}

/* Enable GC */
void gc_enable(GarbageCollector *gc) {
    gc->gc_enabled = true;
}

/* Get GC statistics */
void gc_get_stats(GarbageCollector *gc, GCStats *stats) {
    stats->total_allocated = gc->bytes_allocated;
    stats->num_objects = gc->num_objects;
    stats->next_gc_threshold = gc->next_gc;
    stats->heap_allocated = 0;
    stats->objects_marked = 0;
    stats->objects_swept = 0;
    stats->pointers_traversed = 0;
    
    /* Calculate pool allocations */
    for (int i = 0; i < GC_NUM_POOLS; i++) {
        stats->pool_allocated[i] = 0;
        GCPoolBlock *block = gc->pools[i].blocks;
        while (block) {
            stats->pool_allocated[i] += block->used;
            block = block->next;
        }
    }
    
    /* Calculate heap allocations and count objects with type info */
    GCObjectHeader *obj = gc->objects;
    while (obj) {
        if (!obj->pooled) {
            stats->heap_allocated += sizeof(GCObjectHeader) + obj->size;
        }
        if (obj->type_info) {
            stats->pointers_traversed += type_count_pointers(obj->type_info);
        }
        obj = obj->next;
    }
}
