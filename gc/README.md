# Garbage Collector (GC)

A mark-sweep garbage collector with specialized memory pools for small objects.

## Features

- **Mark-Sweep Algorithm**: Traces reachable objects from roots and frees unreachable ones
- **Memory Pools**: Optimized allocation for small objects (8, 16, 32, 64, 128, 256 bytes)
- **Automatic Triggering**: GC runs when allocation threshold is reached
- **Manual Control**: Can disable/enable GC and force collection
- **Statistics**: Track memory usage and GC performance

## Memory Pools

The GC uses 6 specialized pools for small objects:
- Pool 0: 8 bytes
- Pool 1: 16 bytes
- Pool 2: 32 bytes
- Pool 3: 64 bytes
- Pool 4: 128 bytes
- Pool 5: 256 bytes

Objects larger than 256 bytes are allocated from the heap using standard `malloc`.

## Usage

### Initialization

```c
#include "gc/gc.h"

GarbageCollector gc;
gc_init(&gc);
```

### Allocation

```c
// Allocate memory
char *str = (char *)gc_alloc(&gc, 100);

// Allocate zero-initialized memory
int *array = (int *)gc_alloc_zero(&gc, sizeof(int) * 10);

// Reallocate
str = (char *)gc_realloc(&gc, str, 200);
```

### GC Roots

Objects referenced from roots are kept alive during garbage collection:

```c
// Add root to prevent object from being collected
gc_add_root(&gc, str);

// Remove root when no longer needed
gc_remove_root(&gc, str);
```

### Manual Collection

```c
// Run GC cycle
size_t freed = gc_collect(&gc);

// Force GC even if disabled
size_t freed = gc_collect_force(&gc);

// Temporarily disable GC
gc_disable(&gc);
// ... do work ...
gc_enable(&gc);
```

### Statistics

```c
GCStats stats;
gc_get_stats(&gc, &stats);

printf("Total allocated: %zu bytes\n", stats.total_allocated);
printf("Objects: %zu\n", stats.num_objects);
printf("Heap allocated: %zu bytes\n", stats.heap_allocated);
```

### Cleanup

```c
gc_shutdown(&gc);
```

## Configuration

Constants in `gc.h`:
- `GC_INITIAL_THRESHOLD`: Initial threshold for first GC (1 MB)
- `GC_GROWTH_FACTOR`: Threshold growth factor after GC (2.0)
- `GC_MIN_THRESHOLD`: Minimum GC threshold (512 KB)
- `GC_POOL_BLOCK_SIZE`: Size of pool blocks (4 KB)

## Global Instance

Rubolt provides a global GC instance:

```c
extern GarbageCollector *rubolt_gc;

// Use in your code
void *ptr = gc_alloc(rubolt_gc, 100);
```

## Notes

- Objects are tracked in a linked list via hidden headers
- Mark phase requires type information to traverse object graphs
- Pool allocations use free lists for quick reuse
- GC automatically runs when `bytes_allocated >= next_gc`
