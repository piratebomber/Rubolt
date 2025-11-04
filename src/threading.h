#ifndef RUBOLT_THREADING_H
#define RUBOLT_THREADING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
typedef HANDLE thread_t;
typedef CRITICAL_SECTION mutex_t;
typedef CONDITION_VARIABLE cond_t;
#else
#include <pthread.h>
typedef pthread_t thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t cond_t;
#endif

/* Thread states */
typedef enum {
    THREAD_STATE_CREATED,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_FINISHED
} ThreadState;

/* Thread */
typedef struct Thread {
    int id;
    thread_t native_thread;
    char *name;
    ThreadState state;
    void *(*func)(void *);
    void *args;
    void *result;
    bool daemon;
    bool joined;
    struct Thread *next;
} Thread;

/* Global Interpreter Lock (GIL) */
typedef struct GIL {
    mutex_t lock;
    cond_t cond;
    Thread *owner;
    int lock_count;             /* Reentrant lock count */
    bool initialized;
} GIL;

/* Thread pool */
typedef struct ThreadPool {
    Thread **threads;
    size_t thread_count;
    size_t max_threads;
    
    /* Work queue */
    struct WorkItem {
        void *(*func)(void *);
        void *args;
        void (*on_complete)(void *result, void *context);
        void *context;
        struct WorkItem *next;
    } *work_queue;
    
    mutex_t queue_mutex;
    cond_t work_available;
    cond_t work_complete;
    
    size_t pending_work;
    size_t completed_work;
    bool shutdown;
} ThreadPool;

/* Thread local storage */
typedef struct ThreadLocal {
    void **values;
    size_t capacity;
    mutex_t lock;
} ThreadLocal;

/* ========== GIL MANAGEMENT ========== */

/* Initialize GIL */
void gil_init(GIL *gil);

/* Destroy GIL */
void gil_destroy(GIL *gil);

/* Acquire GIL */
void gil_acquire(GIL *gil, Thread *thread);

/* Release GIL */
void gil_release(GIL *gil, Thread *thread);

/* Try acquire GIL (non-blocking) */
bool gil_try_acquire(GIL *gil, Thread *thread);

/* Check if thread owns GIL */
bool gil_is_owner(GIL *gil, Thread *thread);

/* ========== THREAD MANAGEMENT ========== */

/* Create thread */
Thread *thread_create(void *(*func)(void *), void *args, const char *name);

/* Start thread */
bool thread_start(Thread *thread);

/* Join thread (wait for completion) */
void *thread_join(Thread *thread);

/* Detach thread */
void thread_detach(Thread *thread);

/* Get current thread */
Thread *thread_current(void);

/* Get thread ID */
int thread_get_id(Thread *thread);

/* Set thread name */
void thread_set_name(Thread *thread, const char *name);

/* Check if thread is alive */
bool thread_is_alive(Thread *thread);

/* Sleep current thread */
void thread_sleep(uint64_t ms);

/* Yield to other threads */
void thread_yield(void);

/* ========== THREAD POOL ========== */

/* Create thread pool */
ThreadPool *thread_pool_create(size_t num_threads);

/* Destroy thread pool */
void thread_pool_destroy(ThreadPool *pool);

/* Submit work to pool */
bool thread_pool_submit(ThreadPool *pool, void *(*func)(void *), void *args);

/* Submit work with callback */
bool thread_pool_submit_callback(ThreadPool *pool, void *(*func)(void *), void *args,
                                 void (*on_complete)(void *, void *), void *context);

/* Wait for all work to complete */
void thread_pool_wait(ThreadPool *pool);

/* Get pool statistics */
typedef struct ThreadPoolStats {
    size_t thread_count;
    size_t pending_work;
    size_t completed_work;
    size_t total_submitted;
} ThreadPoolStats;

void thread_pool_get_stats(ThreadPool *pool, ThreadPoolStats *stats);

/* ========== SYNCHRONIZATION PRIMITIVES ========== */

/* Mutex */
typedef struct {
    mutex_t native_mutex;
    bool initialized;
} Mutex;

Mutex *mutex_create(void);
void mutex_destroy(Mutex *mtx);
void mutex_lock(Mutex *mtx);
void mutex_unlock(Mutex *mtx);
bool mutex_try_lock(Mutex *mtx);

/* Condition variable */
typedef struct {
    cond_t native_cond;
    bool initialized;
} CondVar;

CondVar *condvar_create(void);
void condvar_destroy(CondVar *cv);
void condvar_wait(CondVar *cv, Mutex *mtx);
bool condvar_wait_timeout(CondVar *cv, Mutex *mtx, uint64_t timeout_ms);
void condvar_signal(CondVar *cv);
void condvar_broadcast(CondVar *cv);

/* Semaphore */
typedef struct {
    Mutex *mutex;
    CondVar *cond;
    int count;
    int max_count;
} Semaphore;

Semaphore *semaphore_create(int initial_count, int max_count);
void semaphore_destroy(Semaphore *sem);
void semaphore_wait(Semaphore *sem);
bool semaphore_try_wait(Semaphore *sem);
bool semaphore_wait_timeout(Semaphore *sem, uint64_t timeout_ms);
void semaphore_post(Semaphore *sem);

/* Read-Write Lock */
typedef struct {
    Mutex *mutex;
    CondVar *read_cond;
    CondVar *write_cond;
    int readers;
    int writers;
    int waiting_writers;
} RWLock;

RWLock *rwlock_create(void);
void rwlock_destroy(RWLock *rwl);
void rwlock_read_lock(RWLock *rwl);
void rwlock_read_unlock(RWLock *rwl);
void rwlock_write_lock(RWLock *rwl);
void rwlock_write_unlock(RWLock *rwl);

/* ========== THREAD LOCAL STORAGE ========== */

/* Create thread-local storage */
ThreadLocal *thread_local_create(void);

/* Destroy thread-local storage */
void thread_local_destroy(ThreadLocal *tls);

/* Get thread-local value */
void *thread_local_get(ThreadLocal *tls);

/* Set thread-local value */
void thread_local_set(ThreadLocal *tls, void *value);

/* ========== ATOMIC OPERATIONS ========== */

/* Atomic integer */
typedef struct {
    volatile int value;
    Mutex *lock;
} AtomicInt;

AtomicInt *atomic_int_create(int initial);
void atomic_int_destroy(AtomicInt *ai);
int atomic_int_get(AtomicInt *ai);
void atomic_int_set(AtomicInt *ai, int value);
int atomic_int_add(AtomicInt *ai, int delta);
int atomic_int_increment(AtomicInt *ai);
int atomic_int_decrement(AtomicInt *ai);
bool atomic_int_compare_and_swap(AtomicInt *ai, int expected, int new_val);

/* ========== UTILITIES ========== */

/* Get number of CPU cores */
int thread_cpu_count(void);

/* Set thread affinity */
bool thread_set_affinity(Thread *thread, int cpu);

/* Set thread priority */
bool thread_set_priority(Thread *thread, int priority);

/* Global GIL instance */
extern GIL *global_gil;

/* Global thread pool */
extern ThreadPool *global_thread_pool;

#endif /* RUBOLT_THREADING_H */
