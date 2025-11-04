# Reference Counting (RC)

A reference counting system with automatic cleanup, weak references, and cycle detection.

## Features

- **Automatic Reference Counting**: Tracks how many references point to each object
- **Weak References**: References that don't prevent object destruction
- **Cycle Detection**: Identifies and collects reference cycles
- **Destructor Support**: Custom cleanup functions for objects
- **Auto-Pointers**: RAII-style automatic reference management (GCC/Clang)
- **Statistics**: Track objects, references, and cycles

## Usage

### Initialization

```c
#include "rc/rc.h"

RefCounter rc;
rc_init(&rc);
```

### Creating Objects

```c
// Create object with data
char *data = strdup("Hello");
RCObject *obj = rc_new(&rc, data, 0, free);
// Initial ref count is 1

// With custom destructor
void my_destructor(void *data) {
    // Custom cleanup
    free(data);
}
RCObject *obj2 = rc_new(&rc, my_data, sizeof(MyType), my_destructor);
```

### Reference Management

```c
// Increment reference count (retain/acquire)
rc_retain(&rc, obj);

// Decrement reference count (release)
rc_release(&rc, obj);  // Automatically freed when count reaches 0

// Get current reference count
size_t refs = rc_get_count(obj);
```

### Weak References

Weak references don't prevent object destruction:

```c
// Create weak reference
RCWeakRef *weak = rc_weak_new(&rc, obj);

// Try to promote weak ref to strong ref
RCObject *strong = rc_weak_lock(weak);
if (strong) {
    // Object still alive, can use it
    printf("Object data: %s\n", (char *)strong->data);
} else {
    // Object has been freed
    printf("Object was destroyed\n");
}

// Release weak reference
rc_weak_release(&rc, weak);
```

### Cycle Detection

Reference cycles can cause memory leaks. RC provides cycle detection:

```c
// Mark objects that may be in cycles
rc_mark_for_cycle_detection(&rc, obj1);
rc_mark_for_cycle_detection(&rc, obj2);

// Detect and collect cycles
size_t collected = rc_collect_cycles(&rc);

// Enable/disable cycle detection
rc_set_cycle_detection(&rc, true);
rc_set_cycle_detection(&rc, false);
```

### Auto-Pointers (GCC/Clang only)

Auto-pointers automatically release references when going out of scope:

```c
void my_function(RefCounter *rc, RCObject *obj) {
    RC_AUTO RCAutoPtr ptr = rc_auto_new(rc, obj);
    
    // Use ptr.obj
    printf("Data: %s\n", (char *)ptr.obj->data);
    
    // Automatically released when function returns
}
```

### Statistics

```c
RCStats stats;
rc_get_stats(&rc, &stats);

printf("Total objects: %zu\n", stats.total_objects);
printf("Total refs: %zu\n", stats.total_refs);
printf("Cycle buffer size: %zu\n", stats.cycle_buffer_size);
```

### Cleanup

```c
rc_shutdown(&rc);
```

## How It Works

### Reference Counting

Each object has a `ref_count`:
- Starts at 1 when created
- Increments with `rc_retain()`
- Decrements with `rc_release()`
- Freed when count reaches 0

### Weak References

Weak references:
- Don't increment `ref_count`
- Increment `weak_ref_count`
- Become invalid when object is freed
- Can be "locked" to promote to strong reference

### Cycle Detection

Cycles occur when objects reference each other circularly:

```
A → B → C → A
```

The cycle detector:
1. Marks objects that may be in cycles
2. Performs mark-and-scan to find unreachable cycles
3. Frees cyclic objects that have no external references

## Global Instance

Rubolt provides a global RC instance:

```c
extern RefCounter *rubolt_rc;

// Use in your code
RCObject *obj = rc_new(rubolt_rc, data, size, destructor);
```

## Example

```c
#include "rc/rc.h"

void string_destructor(void *data) {
    printf("Freeing: %s\n", (char *)data);
    free(data);
}

int main() {
    RefCounter rc;
    rc_init(&rc);
    
    // Create object
    char *str = strdup("Hello, RC!");
    RCObject *obj = rc_new(&rc, str, 0, string_destructor);
    
    // Retain
    rc_retain(&rc, obj);
    printf("Refs: %zu\n", rc_get_count(obj));  // 2
    
    // Release
    rc_release(&rc, obj);
    printf("Refs: %zu\n", rc_get_count(obj));  // 1
    
    // Final release (object freed, destructor called)
    rc_release(&rc, obj);
    
    rc_shutdown(&rc);
    return 0;
}
```

## Notes

- Reference counting is deterministic (immediate cleanup)
- Can't handle cycles without explicit detection
- More predictable than garbage collection
- Lower overhead for simple ownership patterns
- Best for tree-like data structures
