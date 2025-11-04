#include "profiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
static uint64_t perf_frequency(void) {
    LARGE_INTEGER f; QueryPerformanceFrequency(&f); return (uint64_t)f.QuadPart;
}
static uint64_t time_ns_now(void) {
    static uint64_t freq = 0; if (!freq) freq = perf_frequency();
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return (uint64_t)((__int128) t.QuadPart * 1000000000ull / freq);
}
#else
#include <time.h>
static uint64_t time_ns_now(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}
#endif

Profiler *global_profiler = NULL;

static ProfileEntry *find_or_create_entry(Profiler *prof, const char *function_name) {
    ProfileEntry *e = prof->entries;
    while (e) { if (strcmp(e->function_name, function_name) == 0) return e; e = e->next; }
    e = (ProfileEntry *)calloc(1, sizeof(ProfileEntry));
    if (!e) return NULL;
    e->function_name = _strdup(function_name);
#ifdef _WIN32
    if (!e->function_name && function_name) e->function_name = _strdup(function_name);
#endif
    e->min_time_ns = UINT64_MAX;
    e->next = prof->entries;
    prof->entries = e;
    prof->entry_count++;
    return e;
}

void profiler_init(Profiler *prof) {
    prof->entries = NULL;
    prof->entry_count = 0;
    prof->enabled = true;
    prof->total_execution_time_ns = 0;
    prof->profiling_start_time = time_ns_now();
    prof->hot_spots = NULL;
    prof->hot_spot_count = 0;
    prof->hot_spot_capacity = 0;
}

void profiler_shutdown(Profiler *prof) {
    ProfileEntry *e = prof->entries;
    while (e) {
        ProfileEntry *next = e->next;
        free(e->function_name);
        free(e);
        e = next;
    }
    free(prof->hot_spots);
}

void profiler_enable(Profiler *prof) { prof->enabled = true; }
void profiler_disable(Profiler *prof) { prof->enabled = false; }

void profiler_reset(Profiler *prof) {
    profiler_shutdown(prof);
    profiler_init(prof);
}

uint64_t profiler_get_time_ns(void) { return time_ns_now(); }

void profiler_function_enter(Profiler *prof, const char *function_name) {
    if (!prof->enabled) return;
    ProfileEntry *e = find_or_create_entry(prof, function_name);
    if (!e) return;
    e->is_active = true;
    e->start_time_ns = time_ns_now();
}

void profiler_function_exit(Profiler *prof, const char *function_name) {
    if (!prof->enabled) return;
    ProfileEntry *e = find_or_create_entry(prof, function_name);
    if (!e || !e->is_active) return;
    uint64_t end = time_ns_now();
    uint64_t dur = end - e->start_time_ns;
    e->call_count++;
    e->total_time_ns += dur;
    if (dur < e->min_time_ns) e->min_time_ns = dur;
    if (dur > e->max_time_ns) e->max_time_ns = dur;
    e->is_active = false;
    prof->total_execution_time_ns += dur;
}

void profiler_record_execution(Profiler *prof, const char *function_name, uint64_t time_ns) {
    ProfileEntry *e = find_or_create_entry(prof, function_name);
    e->call_count++;
    e->total_time_ns += time_ns;
    if (time_ns < e->min_time_ns) e->min_time_ns = time_ns;
    if (time_ns > e->max_time_ns) e->max_time_ns = time_ns;
}

ProfileEntry *profiler_get_entry(Profiler *prof, const char *function_name) {
    return find_or_create_entry(prof, function_name);
}

static int cmp_by_time(const void *a, const void *b) {
    ProfileEntry *ea = *(ProfileEntry **)a;
    ProfileEntry *eb = *(ProfileEntry **)b;
    if (ea->total_time_ns < eb->total_time_ns) return 1;
    if (ea->total_time_ns > eb->total_time_ns) return -1;
    return 0;
}

static int cmp_by_calls(const void *a, const void *b) {
    ProfileEntry *ea = *(ProfileEntry **)a;
    ProfileEntry *eb = *(ProfileEntry **)b;
    if (ea->call_count < eb->call_count) return 1;
    if (ea->call_count > eb->call_count) return -1;
    return 0;
}

void profiler_print_report(Profiler *prof) {
    size_t n = prof->entry_count;
    ProfileEntry **arr = (ProfileEntry **)malloc(n * sizeof(ProfileEntry *));
    size_t i = 0; for (ProfileEntry *e = prof->entries; e; e = e->next) arr[i++] = e;
    qsort(arr, n, sizeof(ProfileEntry *), cmp_by_time);
    printf("Profiling report (by total time):\n");
    printf("%-30s %10s %15s %15s %15s\n", "Function", "Calls", "Total(ms)", "Avg(us)", "Max(us)");
    for (i = 0; i < n; i++) {
        ProfileEntry *e = arr[i];
        double total_ms = e->total_time_ns / 1e6;
        double avg_us = (e->call_count ? (e->total_time_ns / (double)e->call_count) : 0) / 1e3;
        double max_us = e->max_time_ns / 1e3;
        printf("%-30s %10llu %15.3f %15.3f %15.3f\n", e->function_name, (unsigned long long)e->call_count, total_ms, avg_us, max_us);
    }
    free(arr);
}

void profiler_print_top_functions(Profiler *prof, size_t n) {
    size_t m = prof->entry_count;
    ProfileEntry **arr = (ProfileEntry **)malloc(m * sizeof(ProfileEntry *));
    size_t i = 0; for (ProfileEntry *e = prof->entries; e; e = e->next) arr[i++] = e;
    qsort(arr, m, sizeof(ProfileEntry *), cmp_by_time);
    if (n > m) n = m;
    printf("Top %zu functions by time:\n", n);
    for (i = 0; i < n; i++) {
        ProfileEntry *e = arr[i];
        double total_ms = e->total_time_ns / 1e6;
        printf("  %s: %.3f ms (%llu calls)\n", e->function_name, total_ms, (unsigned long long)e->call_count);
    }
    free(arr);
}

void profiler_print_top_calls(Profiler *prof, size_t n) {
    size_t m = prof->entry_count;
    ProfileEntry **arr = (ProfileEntry **)malloc(m * sizeof(ProfileEntry *));
    size_t i = 0; for (ProfileEntry *e = prof->entries; e; e = e->next) arr[i++] = e;
    qsort(arr, m, sizeof(ProfileEntry *), cmp_by_calls);
    if (n > m) n = m;
    printf("Top %zu functions by call count:\n", n);
    for (i = 0; i < n; i++) {
        ProfileEntry *e = arr[i];
        printf("  %s: %llu calls (%.3f ms)\n", e->function_name, (unsigned long long)e->call_count, e->total_time_ns / 1e6);
    }
    free(arr);
}

void profiler_analyze_hot_spots(Profiler *prof, float threshold_percentage) {
    /* Simple implementation: mark functions exceeding threshold of total time */
    uint64_t total = prof->total_execution_time_ns;
    if (total == 0) return;
    /* Count eligible */
    size_t count = 0; for (ProfileEntry *e = prof->entries; e; e = e->next) {
        float pct = (float)(100.0 * (double)e->total_time_ns / (double)total);
        if (pct >= threshold_percentage) count++;
    }
    if (count == 0) return;
    prof->hot_spots = (HotSpot *)realloc(prof->hot_spots, count * sizeof(HotSpot));
    prof->hot_spot_capacity = count;
    prof->hot_spot_count = 0;
    for (ProfileEntry *e = prof->entries; e; e = e->next) {
        float pct = (float)(100.0 * (double)e->total_time_ns / (double)total);
        if (pct >= threshold_percentage) {
            HotSpot *hs = &prof->hot_spots[prof->hot_spot_count++];
            hs->location = e->function_name;
            hs->execution_count = e->call_count;
            hs->total_time_ns = e->total_time_ns;
            hs->percentage = pct;
            hs->jit_candidate = true;
        }
    }
}

HotSpot *profiler_get_hot_spots(Profiler *prof, size_t *count) {
    if (count) *count = prof->hot_spot_count; return prof->hot_spots;
}

void profiler_mark_for_jit(Profiler *prof, const char *function_name) { (void)prof; (void)function_name; }

bool profiler_should_jit_compile(Profiler *prof, const char *function_name) {
    size_t n; HotSpot *hs = profiler_get_hot_spots(prof, &n);
    for (size_t i = 0; i < n; i++) if (strcmp(hs[i].location, function_name) == 0) return true;
    return false;
}

bool profiler_export_json(Profiler *prof, const char *filename) {
    FILE *f = fopen(filename, "wb"); if (!f) return false;
    fprintf(f, "{\n  \"functions\": [\n");
    size_t i = 0; for (ProfileEntry *e = prof->entries; e; e = e->next) {
        fprintf(f, "    { \"name\": \"%s\", \"calls\": %llu, \"total_ns\": %llu }%s\n",
                e->function_name, (unsigned long long)e->call_count, (unsigned long long)e->total_time_ns,
                e->next ? "," : "");
        i++;
    }
    fprintf(f, "  ]\n}\n");
    fclose(f); return true;
}

bool profiler_export_csv(Profiler *prof, const char *filename) {
    FILE *f = fopen(filename, "wb"); if (!f) return false;
    fprintf(f, "name,calls,total_ns,min_ns,max_ns\n");
    for (ProfileEntry *e = prof->entries; e; e = e->next) {
        fprintf(f, "%s,%llu,%llu,%llu,%llu\n", e->function_name,
                (unsigned long long)e->call_count,
                (unsigned long long)e->total_time_ns,
                (unsigned long long)e->min_time_ns,
                (unsigned long long)e->max_time_ns);
    }
    fclose(f); return true;
}

char *profiler_format_time(uint64_t time_ns) {
    char *buf = (char *)malloc(64);
    if (time_ns < 1000ull) {
        snprintf(buf, 64, "%lluns", (unsigned long long)time_ns);
    } else if (time_ns < 1000000ull) {
        snprintf(buf, 64, "%.3fus", time_ns / 1000.0);
    } else if (time_ns < 1000000000ull) {
        snprintf(buf, 64, "%.3fms", time_ns / 1000000.0);
    } else {
        snprintf(buf, 64, "%.3fs", time_ns / 1000000000.0);
    }
    return buf;
}

uint64_t profiler_average_time(ProfileEntry *entry) {
    return entry && entry->call_count ? entry->total_time_ns / entry->call_count : 0;
}

uint64_t profiler_total_time(Profiler *prof) { return prof->total_execution_time_ns; }
