#ifndef RUBOLT_INLINE_CACHE_H
#define RUBOLT_INLINE_CACHE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Cache entry states */
typedef enum {
    IC_STATE_UNINITIALIZED,
    IC_STATE_MONOMORPHIC,      /* Single type cached */
    IC_STATE_POLYMORPHIC,      /* Multiple types cached */
    IC_STATE_MEGAMORPHIC       /* Too many types, disable caching */
} ICState;

/* Cached method entry */
typedef struct CachedMethod {
    void *type_id;             /* Type identifier */
    void *method_ptr;          /* Cached method pointer */
    uint64_t hit_count;
    struct CachedMethod *next;
} CachedMethod;

/* Inline cache site */
typedef struct InlineCache {
    char *method_name;
    ICState state;
    CachedMethod *methods;
    size_t method_count;
    uint64_t total_hits;
    uint64_t total_misses;
    uint32_t site_id;          /* Unique ID for this call site */
    struct InlineCache *next;
} InlineCache;

/* Global inline cache manager */
typedef struct InlineCacheManager {
    InlineCache *caches;
    size_t cache_count;
    uint32_t next_site_id;
    bool enabled;
    size_t polymorphic_threshold;  /* Max cached types before megamorphic */
    uint64_t total_lookups;
    uint64_t total_hits;
    uint64_t total_misses;
} InlineCacheManager;

/* ========== MANAGER LIFECYCLE ========== */

/* Initialize inline cache manager */
void ic_manager_init(InlineCacheManager *mgr);

/* Shutdown inline cache manager */
void ic_manager_shutdown(InlineCacheManager *mgr);

/* Enable/disable inline caching */
void ic_manager_enable(InlineCacheManager *mgr);
void ic_manager_disable(InlineCacheManager *mgr);

/* ========== CACHE MANAGEMENT ========== */

/* Create new inline cache for a call site */
InlineCache *ic_create(InlineCacheManager *mgr, const char *method_name);

/* Get or create cache for site */
InlineCache *ic_get_or_create(InlineCacheManager *mgr, uint32_t site_id, 
                              const char *method_name);

/* Invalidate cache */
void ic_invalidate(InlineCache *cache);

/* Invalidate all caches for a method */
void ic_invalidate_method(InlineCacheManager *mgr, const char *method_name);

/* Clear all caches */
void ic_clear_all(InlineCacheManager *mgr);

/* ========== LOOKUP OPERATIONS ========== */

/* Lookup method in cache */
void *ic_lookup(InlineCache *cache, void *type_id);

/* Update cache with new method */
void ic_update(InlineCache *cache, void *type_id, void *method_ptr);

/* Record cache hit */
void ic_record_hit(InlineCache *cache);

/* Record cache miss */
void ic_record_miss(InlineCache *cache);

/* ========== STATE TRANSITIONS ========== */

/* Transition to monomorphic state */
void ic_transition_to_monomorphic(InlineCache *cache, void *type_id, void *method_ptr);

/* Transition to polymorphic state */
void ic_transition_to_polymorphic(InlineCache *cache, void *type_id, void *method_ptr);

/* Transition to megamorphic state */
void ic_transition_to_megamorphic(InlineCache *cache);

/* ========== STATISTICS ========== */

/* Get cache hit rate */
float ic_hit_rate(InlineCache *cache);

/* Get global hit rate */
float ic_global_hit_rate(InlineCacheManager *mgr);

/* Print cache statistics */
void ic_print_stats(InlineCacheManager *mgr);

/* Print cache statistics for specific site */
void ic_print_cache_stats(InlineCache *cache);

/* Get top N most-used caches */
InlineCache **ic_get_top_caches(InlineCacheManager *mgr, size_t n);

/* ========== OPTIMIZATION HINTS ========== */

/* Check if method should be inlined */
bool ic_should_inline(InlineCache *cache);

/* Check if method is stable (monomorphic) */
bool ic_is_stable(InlineCache *cache);

/* Get cached method for JIT compilation */
void *ic_get_stable_method(InlineCache *cache);

/* ========== DEBUGGING ========== */

/* Dump cache state */
void ic_dump(InlineCache *cache);

/* Dump all caches */
void ic_dump_all(InlineCacheManager *mgr);

/* Verify cache integrity */
bool ic_verify(InlineCache *cache);

/* ========== TYPE FEEDBACK ========== */

/* Record type at call site */
void ic_record_type(InlineCache *cache, void *type_id);

/* Get observed types */
void **ic_get_observed_types(InlineCache *cache, size_t *count);

/* Get most common type */
void *ic_get_primary_type(InlineCache *cache);

/* Global inline cache manager */
extern InlineCacheManager *global_ic_manager;

/* Macro for easy inline cache usage */
#define IC_LOOKUP(mgr, site_id, method_name, type_id, fallback) \
    ({ \
        InlineCache *_cache = ic_get_or_create(mgr, site_id, method_name); \
        void *_method = ic_lookup(_cache, type_id); \
        if (!_method) { \
            _method = fallback; \
            ic_update(_cache, type_id, _method); \
            ic_record_miss(_cache); \
        } else { \
            ic_record_hit(_cache); \
        } \
        _method; \
    })

#endif /* RUBOLT_INLINE_CACHE_H */
