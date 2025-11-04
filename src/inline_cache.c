#include "inline_cache.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

InlineCacheManager *global_ic_manager = NULL;

void ic_manager_init(InlineCacheManager *mgr) {
    mgr->caches = NULL;
    mgr->cache_count = 0;
    mgr->next_site_id = 1;
    mgr->enabled = true;
    mgr->polymorphic_threshold = 4;
    mgr->total_lookups = 0;
    mgr->total_hits = 0;
    mgr->total_misses = 0;
}

void ic_manager_shutdown(InlineCacheManager *mgr) {
    InlineCache *c = mgr->caches;
    while (c) {
        InlineCache *next = c->next;
        CachedMethod *m = c->methods;
        while (m) { CachedMethod *mn = m->next; free(m); m = mn; }
        free(c->method_name);
        free(c);
        c = next;
    }
}

void ic_manager_enable(InlineCacheManager *mgr) { mgr->enabled = true; }
void ic_manager_disable(InlineCacheManager *mgr) { mgr->enabled = false; }

InlineCache *ic_create(InlineCacheManager *mgr, const char *method_name) {
    InlineCache *c = (InlineCache *)calloc(1, sizeof(InlineCache));
    if (!c) return NULL;
    c->method_name = _strdup(method_name);
#ifdef _WIN32
    if (!c->method_name && method_name) c->method_name = _strdup(method_name);
#endif
    c->state = IC_STATE_UNINITIALIZED;
    c->site_id = mgr->next_site_id++;
    c->next = mgr->caches;
    mgr->caches = c;
    mgr->cache_count++;
    return c;
}

InlineCache *ic_get_or_create(InlineCacheManager *mgr, uint32_t site_id, const char *method_name) {
    InlineCache *c = mgr->caches;
    while (c) { if (c->site_id == site_id) return c; c = c->next; }
    /* Create new with given id */
    c = ic_create(mgr, method_name);
    if (c) c->site_id = site_id;
    return c;
}

void ic_invalidate(InlineCache *cache) {
    CachedMethod *m = cache->methods; while (m) { CachedMethod *n = m->next; free(m); m = n; }
    cache->methods = NULL;
    cache->method_count = 0;
    cache->state = IC_STATE_UNINITIALIZED;
    cache->total_hits = cache->total_misses = 0;
}

void ic_invalidate_method(InlineCacheManager *mgr, const char *method_name) {
    for (InlineCache *c = mgr->caches; c; c = c->next) {
        if (strcmp(c->method_name, method_name) == 0) ic_invalidate(c);
    }
}

void ic_clear_all(InlineCacheManager *mgr) {
    for (InlineCache *c = mgr->caches; c; c = c->next) ic_invalidate(c);
}

void *ic_lookup(InlineCache *cache, void *type_id) {
    cache->total_hits += 0; /* no-op to silence unused warnings */
    for (CachedMethod *m = cache->methods; m; m = m->next) {
        if (m->type_id == type_id) { m->hit_count++; return m->method_ptr; }
    }
    return NULL;
}

void ic_update(InlineCache *cache, void *type_id, void *method_ptr) {
    if (cache->state == IC_STATE_UNINITIALIZED) {
        ic_transition_to_monomorphic(cache, type_id, method_ptr);
        return;
    }
    if (cache->state == IC_STATE_MONOMORPHIC) {
        /* if different type, go polymorphic */
        if (!cache->methods || cache->methods->type_id != type_id) {
            ic_transition_to_polymorphic(cache, type_id, method_ptr);
            return;
        }
    }
    if (cache->state == IC_STATE_POLYMORPHIC) {
        /* add another type or go megamorphic */
        if (cache->method_count >= cache->method_count + 1) { /* trivial */ }
        if (cache->method_count + 1 > 8) { ic_transition_to_megamorphic(cache); return; }
        CachedMethod *m = (CachedMethod *)calloc(1, sizeof(CachedMethod));
        m->type_id = type_id; m->method_ptr = method_ptr; m->next = cache->methods; cache->methods = m; cache->method_count++;
        return;
    }
    /* megamorphic: do nothing */
}

void ic_record_hit(InlineCache *cache) { cache->total_hits++; }
void ic_record_miss(InlineCache *cache) { cache->total_misses++; }

void ic_transition_to_monomorphic(InlineCache *cache, void *type_id, void *method_ptr) {
    ic_invalidate(cache);
    CachedMethod *m = (CachedMethod *)calloc(1, sizeof(CachedMethod));
    m->type_id = type_id; m->method_ptr = method_ptr; cache->methods = m; cache->method_count = 1;
    cache->state = IC_STATE_MONOMORPHIC;
}

void ic_transition_to_polymorphic(InlineCache *cache, void *type_id, void *method_ptr) {
    if (cache->state == IC_STATE_UNINITIALIZED) ic_transition_to_monomorphic(cache, type_id, method_ptr);
    else {
        /* add current and new type */
        if (cache->state == IC_STATE_MONOMORPHIC) {
            CachedMethod *existing = cache->methods;
            CachedMethod *m = (CachedMethod *)calloc(1, sizeof(CachedMethod));
            m->type_id = type_id; m->method_ptr = method_ptr; m->next = existing; cache->methods = m; cache->method_count = 2;
        } else {
            CachedMethod *m = (CachedMethod *)calloc(1, sizeof(CachedMethod));
            m->type_id = type_id; m->method_ptr = method_ptr; m->next = cache->methods; cache->methods = m; cache->method_count++;
        }
        cache->state = IC_STATE_POLYMORPHIC;
    }
    if (cache->method_count > 8) ic_transition_to_megamorphic(cache);
}

void ic_transition_to_megamorphic(InlineCache *cache) {
    cache->state = IC_STATE_MEGAMORPHIC;
    /* Keep methods but no longer updated */
}

float ic_hit_rate(InlineCache *cache) {
    uint64_t total = cache->total_hits + cache->total_misses; if (!total) return 0.0f;
    return (float)(100.0 * (double)cache->total_hits / (double)total);
}

float ic_global_hit_rate(InlineCacheManager *mgr) {
    uint64_t total = mgr->total_hits + mgr->total_misses; if (!total) return 0.0f;
    return (float)(100.0 * (double)mgr->total_hits / (double)total);
}

void ic_print_stats(InlineCacheManager *mgr) {
    printf("Inline cache sites=%zu, hit-rate=%.2f%%\n", mgr->cache_count, ic_global_hit_rate(mgr));
}

void ic_print_cache_stats(InlineCache *cache) {
    printf("IC site %u for %s: state=%d hit-rate=%.2f%% types=%zu\n", cache->site_id, cache->method_name,
           (int)cache->state, ic_hit_rate(cache), cache->method_count);
}

InlineCache **ic_get_top_caches(InlineCacheManager *mgr, size_t n) {
    (void)mgr; (void)n; return NULL; /* TODO */
}

bool ic_should_inline(InlineCache *cache) { return cache->state == IC_STATE_MONOMORPHIC; }
bool ic_is_stable(InlineCache *cache) { return cache->state == IC_STATE_MONOMORPHIC; }
void *ic_get_stable_method(InlineCache *cache) { return cache->methods ? cache->methods->method_ptr : NULL; }

void ic_dump(InlineCache *cache) { ic_print_cache_stats(cache); }
void ic_dump_all(InlineCacheManager *mgr) { for (InlineCache *c = mgr->caches; c; c = c->next) ic_dump(c); }
bool ic_verify(InlineCache *cache) { (void)cache; return true; }

void ic_record_type(InlineCache *cache, void *type_id) { (void)cache; (void)type_id; }
void **ic_get_observed_types(InlineCache *cache, size_t *count) { (void)cache; if (count) *count = 0; return NULL; }
void *ic_get_primary_type(InlineCache *cache) { return cache->methods ? cache->methods->type_id : NULL; }
