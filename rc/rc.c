#include "rc.h"
#include "../gc/type_info.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Colors for tri-color marking */
#define COLOR_WHITE 0  /* Not visited */
#define COLOR_GRAY  1  /* Being visited */
#define COLOR_BLACK 2  /* Fully visited */

/* Global reference counter instance */
RefCounter *rubolt_rc = NULL;

/* Initialize reference counter */
void rc_init(RefCounter *rc) {
    rc->cycle_buffer = NULL;
    rc->cycle_buffer_size = 0;
    rc->object_registry = NULL;
    rc->total_objects = 0;
    rc->total_refs = 0;
    rc->cycles_detected = 0;
    rc->cycles_collected = 0;
    rc->cycle_detection_enabled = true;
}

/* Add object to registry */
static void rc_register_object(RefCounter *rc, RCObject *obj) {
    obj->registry_next = rc->object_registry;
    rc->object_registry = obj;
}

/* Remove object from registry */
static void rc_unregister_object(RefCounter *rc, RCObject *obj) {
    RCObject *prev = NULL;
    RCObject *curr = rc->object_registry;
    
    while (curr) {
        if (curr == obj) {
            if (prev) {
                prev->registry_next = curr->registry_next;
            } else {
                rc->object_registry = curr->registry_next;
            }
            break;
        }
        prev = curr;
        curr = curr->registry_next;
    }
}

/* Free an object and its data */
static void rc_free_object(RefCounter *rc, RCObject *obj) {
    if (!obj) return;
    
    /* Mark as freed */
    obj->magic = RC_FREED_MAGIC;
    
    /* Call destructor if present */
    if (obj->destructor && obj->data) {
        obj->destructor(obj->data);
    }
    
    /* Free the data */
    if (obj->data) {
        free(obj->data);
    }
    
    /* Unregister from object registry */
    rc_unregister_object(rc, obj);
    
    /* Free the object itself */
    free(obj);
}

/* Shutdown reference counter */
void rc_shutdown(RefCounter *rc) {
    /* Free all objects in registry */
    RCObject *obj = rc->object_registry;
    while (obj) {
        RCObject *next = obj->registry_next;
        
        /* Mark as freed */
        obj->magic = RC_FREED_MAGIC;
        
        /* Call destructor if present */
        if (obj->destructor && obj->data) {
            obj->destructor(obj->data);
        }
        
        /* Free the data */
        if (obj->data) {
            free(obj->data);
        }
        
        /* Free the object itself */
        free(obj);
        obj = next;
    }
    
    rc->object_registry = NULL;
    rc->cycle_buffer = NULL;
    rc->cycle_buffer_size = 0;
    rc->total_objects = 0;
    rc->total_refs = 0;
}

/* Create a new reference-counted object */
RCObject *rc_new(RefCounter *rc, void *data, size_t size, void (*destructor)(void *)) {
    RCObject *obj = (RCObject *)malloc(sizeof(RCObject));
    if (!obj) return NULL;
    
    /* Set magic number for validation */
    obj->magic = RC_MAGIC_NUMBER;
    
    /* Allocate and copy data if provided */
    if (data && size > 0) {
        obj->data = malloc(size);
        if (!obj->data) {
            free(obj);
            return NULL;
        }
        memcpy(obj->data, data, size);
    } else {
        obj->data = data; /* Use provided pointer directly */
    }
    
    obj->ref_count = 1; /* Starts with one reference */
    obj->weak_ref_count = 0;
    obj->internal_ref_count = 0;
    obj->type_info = NULL;
    obj->marked = false;
    obj->scanned = false;
    obj->in_cycle_buffer = false;
    obj->color = COLOR_WHITE;
    obj->next = NULL;
    obj->registry_next = NULL;
    obj->destructor = destructor;
    
    /* Register object in global registry */
    rc_register_object(rc, obj);
    
    rc->total_objects++;
    rc->total_refs++;
    
    return obj;
}

/* Create a new typed reference-counted object */
RCObject *rc_new_typed(RefCounter *rc, void *data, size_t size, TypeInfo *type_info, void (*destructor)(void *)) {
    RCObject *obj = rc_new(rc, data, size, destructor);
    if (obj) {
        obj->type_info = type_info;
    }
    return obj;
}

/* Increment reference count */
void rc_retain(RefCounter *rc, RCObject *obj) {
    if (!obj) return;
    
    obj->ref_count++;
    rc->total_refs++;
    
    /* Mark for cycle detection if enabled */
    if (rc->cycle_detection_enabled && obj->ref_count > 1) {
        rc_mark_for_cycle_detection(rc, obj);
    }
}

/* Decrement reference count and free if zero */
void rc_release(RefCounter *rc, RCObject *obj) {
    if (!obj) return;
    
    obj->ref_count--;
    rc->total_refs--;
    
    if (obj->ref_count == 0) {
        /* Remove from cycle buffer if present */
        if (obj->in_cycle_buffer) {
            RCObject *prev = NULL;
            RCObject *curr = rc->cycle_buffer;
            
            while (curr) {
                if (curr == obj) {
                    if (prev) {
                        prev->next = curr->next;
                    } else {
                        rc->cycle_buffer = curr->next;
                    }
                    rc->cycle_buffer_size--;
                    break;
                }
                prev = curr;
                curr = curr->next;
            }
        }
        
        /* Free the object */
        rc->total_objects--;
        rc_free_object(rc, obj);
    }
}

/* Get current reference count */
size_t rc_get_count(RCObject *obj) {
    return obj ? obj->ref_count : 0;
}

/* Create a weak reference */
RCWeakRef *rc_weak_new(RefCounter *rc, RCObject *obj) {
    if (!obj) return NULL;
    
    RCWeakRef *weak = (RCWeakRef *)malloc(sizeof(RCWeakRef));
    if (!weak) return NULL;
    
    weak->object = obj;
    weak->next = NULL;
    
    obj->weak_ref_count++;
    
    return weak;
}

/* Release a weak reference */
void rc_weak_release(RefCounter *rc, RCWeakRef *weak) {
    if (!weak) return;
    
    if (weak->object) {
        weak->object->weak_ref_count--;
    }
    
    free(weak);
}

/* Try to promote weak ref to strong ref */
RCObject *rc_weak_lock(RCWeakRef *weak) {
    if (!weak || !weak->object) return NULL;
    
    /* Object still alive, can be promoted */
    if (weak->object->ref_count > 0) {
        return weak->object;
    }
    
    /* Object has been freed */
    weak->object = NULL;
    return NULL;
}

/* Mark object for cycle detection */
void rc_mark_for_cycle_detection(RefCounter *rc, RCObject *obj) {
    if (!obj || obj->in_cycle_buffer) return;
    
    /* Add to cycle buffer */
    obj->next = rc->cycle_buffer;
    rc->cycle_buffer = obj;
    obj->in_cycle_buffer = true;
    rc->cycle_buffer_size++;
}

/* Validate if a pointer points to a valid RCObject */
bool rc_is_valid_object(RefCounter *rc, void *ptr) {
    if (!ptr) return false;
    
    /* Check if pointer is aligned (objects should be aligned) */
    if ((uintptr_t)ptr % sizeof(void *) != 0) {
        return false;
    }
    
    /* Try to read magic number - this is unsafe but we wrap it */
    RCObject *obj = (RCObject *)ptr;
    
    /* First quick check: magic number */
    if (obj->magic != RC_MAGIC_NUMBER) {
        return false;
    }
    
    /* Second check: search in object registry */
    RCObject *curr = rc->object_registry;
    while (curr) {
        if (curr == obj) {
            /* Found in registry - it's valid */
            return true;
        }
        curr = curr->registry_next;
    }
    
    /* Not found in registry - invalid */
    return false;
}

/* Context for RC graph traversal */
typedef struct RCTraverseContext {
    RefCounter *rc;
    RCObject *current_obj;
} RCTraverseContext;

/* Visitor to count internal references */
static void count_internal_refs_visitor(void *object, void *pointer_field, void *context) {
    RCTraverseContext *ctx = (RCTraverseContext *)context;
    
    /* Verify that pointer_field actually points to a valid RCObject */
    if (!rc_is_valid_object(ctx->rc, pointer_field)) {
        /* Not a valid RCObject - skip it */
        return;
    }
    
    RCObject *referenced = (RCObject *)pointer_field;
    if (referenced && referenced != ctx->current_obj) {
        referenced->internal_ref_count++;
    }
}

/* Reset internal reference counts */
static void reset_internal_refs(RefCounter *rc) {
    RCObject *obj = rc->cycle_buffer;
    while (obj) {
        obj->internal_ref_count = 0;
        obj->color = COLOR_WHITE;
        obj->scanned = false;
        obj = obj->next;
    }
}

/* Calculate internal references by traversing all objects */
static void calculate_internal_refs(RefCounter *rc) {
    RCObject *obj = rc->cycle_buffer;
    
    while (obj) {
        if (obj->type_info && type_has_pointers(obj->type_info)) {
            RCTraverseContext ctx;
            ctx.rc = rc;
            ctx.current_obj = obj;
            
            type_traverse_pointers(obj->type_info, obj->data, count_internal_refs_visitor, &ctx);
        }
        obj = obj->next;
    }
}

/* Depth-first search to mark reachable objects */
static void dfs_mark(RefCounter *rc, RCObject *obj) {
    if (!obj || obj->color != COLOR_WHITE) return;
    
    obj->color = COLOR_GRAY;
    
    /* Traverse pointers */
    if (obj->type_info && type_has_pointers(obj->type_info)) {
        for (size_t i = 0; i < obj->type_info->field_count; i++) {
            FieldInfo *field = &obj->type_info->fields[i];
            void *field_addr = (char *)obj->data + field->offset;
            
            if (field->type == FIELD_POINTER) {
                void *ptr = *(void **)field_addr;
                /* Validate pointer before following */
                if (ptr && rc_is_valid_object(rc, ptr)) {
                    RCObject *referenced = (RCObject *)ptr;
                    dfs_mark(rc, referenced);
                }
            }
        }
    }
    
    obj->color = COLOR_BLACK;
}

/* Mark phase - mark objects with external references */
static void rc_mark_phase(RefCounter *rc) {
    RCObject *obj = rc->cycle_buffer;
    
    while (obj) {
        /* If object has external references (not just internal) */
        size_t external_refs = obj->ref_count - obj->internal_ref_count;
        if (external_refs > 0) {
            /* Mark this object and all reachable from it */
            dfs_mark(rc, obj);
        }
        obj = obj->next;
    }
}

/* Scan phase - collect white (unreachable) objects */
static void rc_scan_cycles(RefCounter *rc, RCObject **cycles, size_t *cycle_count) {
    RCObject *obj = rc->cycle_buffer;
    RCObject *prev = NULL;
    
    *cycle_count = 0;
    
    while (obj) {
        RCObject *next = obj->next;
        
        if (obj->color == COLOR_WHITE && obj->ref_count > 0) {
            /* This object is part of a cycle */
            rc->cycles_detected++;
            (*cycle_count)++;
            
            /* Remove from cycle buffer */
            if (prev) {
                prev->next = next;
            } else {
                rc->cycle_buffer = next;
            }
            rc->cycle_buffer_size--;
            obj->in_cycle_buffer = false;
            
            /* Add to cycles list */
            obj->next = *cycles;
            *cycles = obj;
        } else {
            prev = obj;
        }
        
        obj = next;
    }
}

/* Collect reference cycles */
size_t rc_collect_cycles(RefCounter *rc) {
    if (!rc->cycle_detection_enabled || rc->cycle_buffer_size == 0) {
        return 0;
    }
    
    size_t collected = 0;
    size_t cycle_count = 0;
    RCObject *cycles = NULL;
    
    /* Reset internal reference counts */
    reset_internal_refs(rc);
    
    /* Calculate internal references */
    calculate_internal_refs(rc);
    
    /* Mark phase - mark objects with external references */
    rc_mark_phase(rc);
    
    /* Scan phase - find white (unreachable) objects in cycles */
    rc_scan_cycles(rc, &cycles, &cycle_count);
    
    /* Free cyclic objects */
    while (cycles) {
        RCObject *obj = cycles;
        cycles = cycles->next;
        
        /* Force free the object */
        rc->total_objects--;
        rc->total_refs -= obj->ref_count;
        rc_free_object(rc, obj);
        collected++;
    }
    
    rc->cycles_collected += collected;
    
    return collected;
}

/* Enable/disable cycle detection */
void rc_set_cycle_detection(RefCounter *rc, bool enabled) {
    rc->cycle_detection_enabled = enabled;
}

/* Get statistics */
void rc_get_stats(RefCounter *rc, RCStats *stats) {
    stats->total_objects = rc->total_objects;
    stats->total_refs = rc->total_refs;
    stats->cycle_buffer_size = rc->cycle_buffer_size;
    stats->cycles_detected = rc->cycles_detected;
    stats->cycles_collected = rc->cycles_collected;
    
    /* Count objects currently in cycles */
    stats->objects_in_cycles = 0;
    RCObject *obj = rc->cycle_buffer;
    while (obj) {
        if (obj->type_info && type_has_pointers(obj->type_info)) {
            stats->objects_in_cycles++;
        }
        obj = obj->next;
    }
}

/* Create auto-pointer */
RCAutoPtr rc_auto_new(RefCounter *rc, RCObject *obj) {
    RCAutoPtr ptr;
    ptr.rc = rc;
    ptr.obj = obj;
    
    if (obj) {
        rc_retain(rc, obj);
    }
    
    return ptr;
}

/* Release auto-pointer */
void rc_auto_release(RCAutoPtr *ptr) {
    if (ptr && ptr->obj) {
        rc_release(ptr->rc, ptr->obj);
        ptr->obj = NULL;
    }
}
