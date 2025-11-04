#ifndef RUBOLT_PROFILER_H
#define RUBOLT_PROFILER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* Profiler entry for a function */
typedef struct ProfileEntry {
    char *function_name;
    uint64_t call_count;
    uint64_t total_time_ns;     /* Total time in nanoseconds */
    uint64_t self_time_ns;      /* Time excluding called functions */
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    uint64_t start_time_ns;     /* For current call */
    bool is_active;
    struct ProfileEntry *next;
} ProfileEntry;

/* Hot spot detection */
typedef struct HotSpot {
    char *location;             /* Function or code location */
    uint64_t execution_count;
    uint64_t total_time_ns;
    float percentage;           /* % of total execution time */
    bool jit_candidate;         /* Should be JIT compiled */
} HotSpot;

/* Profiler state */
typedef struct Profiler {
    ProfileEntry *entries;
    size_t entry_count;
    bool enabled;
    uint64_t total_execution_time_ns;
    uint64_t profiling_start_time;
    HotSpot *hot_spots;
    size_t hot_spot_count;
    size_t hot_spot_capacity;
} Profiler;

/* ========== PROFILER LIFECYCLE ========== */

/* Initialize profiler */
void profiler_init(Profiler *prof);

/* Shutdown profiler */
void profiler_shutdown(Profiler *prof);

/* Enable/disable profiler */
void profiler_enable(Profiler *prof);
void profiler_disable(Profiler *prof);

/* Reset profiler statistics */
void profiler_reset(Profiler *prof);

/* ========== TIMING FUNCTIONS ========== */

/* Get current time in nanoseconds */
uint64_t profiler_get_time_ns(void);

/* Start timing a function */
void profiler_function_enter(Profiler *prof, const char *function_name);

/* End timing a function */
void profiler_function_exit(Profiler *prof, const char *function_name);

/* Record single execution time */
void profiler_record_execution(Profiler *prof, const char *function_name, uint64_t time_ns);

/* ========== STATISTICS ========== */

/* Get profile entry for function */
ProfileEntry *profiler_get_entry(Profiler *prof, const char *function_name);

/* Print profiling report */
void profiler_print_report(Profiler *prof);

/* Print top N functions by time */
void profiler_print_top_functions(Profiler *prof, size_t n);

/* Print top N functions by call count */
void profiler_print_top_calls(Profiler *prof, size_t n);

/* ========== HOT SPOT DETECTION ========== */

/* Analyze and identify hot spots */
void profiler_analyze_hot_spots(Profiler *prof, float threshold_percentage);

/* Get hot spots */
HotSpot *profiler_get_hot_spots(Profiler *prof, size_t *count);

/* Mark function for JIT compilation */
void profiler_mark_for_jit(Profiler *prof, const char *function_name);

/* Check if function should be JIT compiled */
bool profiler_should_jit_compile(Profiler *prof, const char *function_name);

/* ========== EXPORT/IMPORT ========== */

/* Export profile data to JSON */
bool profiler_export_json(Profiler *prof, const char *filename);

/* Export profile data to CSV */
bool profiler_export_csv(Profiler *prof, const char *filename);

/* ========== UTILITIES ========== */

/* Format time (ns to human readable) */
char *profiler_format_time(uint64_t time_ns);

/* Calculate average time per call */
uint64_t profiler_average_time(ProfileEntry *entry);

/* Get total profiled time */
uint64_t profiler_total_time(Profiler *prof);

/* Global profiler instance */
extern Profiler *global_profiler;

/* Macro for easy profiling */
#define PROFILE_FUNCTION(prof, func_name) \
    profiler_function_enter(prof, func_name); \
    /* function code here */ \
    profiler_function_exit(prof, func_name)

/* RAII-style profiling (GCC/Clang) */
#ifdef __GNUC__
typedef struct {
    Profiler *prof;
    const char *name;
} ProfileScope;

static inline void profile_scope_end(ProfileScope *scope) {
    if (scope->prof) {
        profiler_function_exit(scope->prof, scope->name);
    }
}

#define PROFILE_SCOPE(prof, name) \
    ProfileScope __attribute__((cleanup(profile_scope_end))) \
    _prof_scope = {prof, name}; \
    profiler_function_enter(prof, name)
#endif

#endif /* RUBOLT_PROFILER_H */
