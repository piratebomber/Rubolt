#ifndef RUBOLT_RC_H
#define RUBOLT_RC_H

#include <stddef.h>
#include <stdbool.h>
#include "../gc/type_info.h"

/* Reference counting object header */
typedef struct RCObject {
    size_t ref_count;           /* Number of references */
    size_t weak_ref_count;      /* Number of weak references */
    size_t internal_ref_count;  /* References from other managed objects */
    TypeInfo *type_info;        /* Type information for traversal */
    bool marked;                /* For cycle detection */
    bool scanned;               /* Already scanned in this cycle */
    bool in_cycle_buffer;       /* Is in cycle detection buffer */
    unsigned int color;         /* Tri-color marking: 0=white, 1=gray, 2=black */
    struct RCObject *next;      /* Link for cycle detection */
    void (*destructor)(void *); /* Destructor function */
    void *data;                 /* Actual object data */
} RCObject;

/* Weak reference wrapper */
typedef struct RCWeakRef {
    RCObject *object;           /* Pointed object (can be NULL if freed) */
    struct RCWeakRef *next;     /* Link in weak ref list */
} RCWeakRef;

/* Reference counter manager */
typedef struct RefCounter {
    RCObject *cycle_buffer;     /* Objects potentially in cycles */
    size_t cycle_buffer_size;   /* Size of cycle buffer */
    size_t total_objects;       /* Total tracked objects */
    size_t total_refs;          /* Total reference count */
    size_t cycles_detected;     /* Number of cycles found */
    size_t cycles_collected;    /* Number of cycles collected */
    bool cycle_detection_enabled; /* Enable/disable cycle detection */
} RefCounter;

/* Initialize reference counter */
void rc_init(RefCounter *rc);

/* Shutdown reference counter */
void rc_shutdown(RefCounter *rc);

/* Create a new reference-counted object */
RCObject *rc_new(RefCounter *rc, void *data, size_t size, void (*destructor)(void *));

/* Create a new typed reference-counted object */
RCObject *rc_new_typed(RefCounter *rc, void *data, size_t size, TypeInfo *type_info, void (*destructor)(void *));

/* Increment reference count (acquire) */
void rc_retain(RefCounter *rc, RCObject *obj);

/* Decrement reference count (release) */
void rc_release(RefCounter *rc, RCObject *obj);

/* Get current reference count */
size_t rc_get_count(RCObject *obj);

/* Create a weak reference */
RCWeakRef *rc_weak_new(RefCounter *rc, RCObject *obj);

/* Release a weak reference */
void rc_weak_release(RefCounter *rc, RCWeakRef *weak);

/* Try to promote weak ref to strong ref (returns NULL if object freed) */
RCObject *rc_weak_lock(RCWeakRef *weak);

/* Detect and collect reference cycles */
size_t rc_collect_cycles(RefCounter *rc);

/* Mark object for cycle detection */
void rc_mark_for_cycle_detection(RefCounter *rc, RCObject *obj);

/* Enable/disable cycle detection */
void rc_set_cycle_detection(RefCounter *rc, bool enabled);

/* Get statistics */
typedef struct RCStats {
    size_t total_objects;
    size_t total_refs;
    size_t cycle_buffer_size;
    size_t cycles_detected;
    size_t cycles_collected;
    size_t objects_in_cycles;
} RCStats;

void rc_get_stats(RefCounter *rc, RCStats *stats);

/* Auto-pointer wrapper for automatic reference management */
typedef struct RCAutoPtr {
    RCObject *obj;
    RefCounter *rc;
} RCAutoPtr;

/* Create auto-pointer (acquires reference) */
RCAutoPtr rc_auto_new(RefCounter *rc, RCObject *obj);

/* Release auto-pointer (releases reference) */
void rc_auto_release(RCAutoPtr *ptr);

/* Macro for automatic cleanup (requires compiler support) */
#ifdef __GNUC__
#define RC_AUTO __attribute__((cleanup(rc_auto_release)))
#else
#define RC_AUTO
#endif

/* Global reference counter instance */
extern RefCounter *rubolt_rc;

#endif /* RUBOLT_RC_H */
