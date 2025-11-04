#include "threading.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

GIL *global_gil = NULL;
ThreadPool *global_thread_pool = NULL;

#ifdef _WIN32
static DWORD WINAPI thread_start_routine(LPVOID arg) {
    Thread *t = (Thread *)arg; t->state = THREAD_STATE_RUNNING; void *res = NULL; if (t->func) res = t->func(t->args); t->result = res; t->state = THREAD_STATE_FINISHED; return 0;
}
#else
#include <unistd.h>
static void *thread_start_routine(void *arg) {
    Thread *t = (Thread *)arg; t->state = THREAD_STATE_RUNNING; void *res = NULL; if (t->func) res = t->func(t->args); t->result = res; t->state = THREAD_STATE_FINISHED; return NULL;
}
#endif

void gil_init(GIL *gil) {
#ifdef _WIN32
    InitializeCriticalSection(&gil->lock);
    InitializeConditionVariable(&gil->cond);
#else
    pthread_mutex_init(&gil->lock, NULL);
    pthread_cond_init(&gil->cond, NULL);
#endif
    gil->owner = NULL; gil->lock_count = 0; gil->initialized = true;
}

void gil_destroy(GIL *gil) {
#ifdef _WIN32
    DeleteCriticalSection(&gil->lock);
#else
    pthread_mutex_destroy(&gil->lock);
    pthread_cond_destroy(&gil->cond);
#endif
    gil->initialized = false;
}

void gil_acquire(GIL *gil, Thread *thread) {
#ifdef _WIN32
    EnterCriticalSection(&gil->lock);
#else
    pthread_mutex_lock(&gil->lock);
#endif
    if (gil->owner == thread) { gil->lock_count++; return; }
    while (gil->owner && gil->owner != thread) {
#ifdef _WIN32
        SleepConditionVariableCS(&gil->cond, &gil->lock, INFINITE);
#else
        pthread_cond_wait(&gil->cond, &gil->lock);
#endif
    }
    gil->owner = thread; gil->lock_count = 1;
#ifdef _WIN32
    LeaveCriticalSection(&gil->lock);
#else
    pthread_mutex_unlock(&gil->lock);
#endif
}

void gil_release(GIL *gil, Thread *thread) {
#ifdef _WIN32
    EnterCriticalSection(&gil->lock);
#else
    pthread_mutex_lock(&gil->lock);
#endif
    if (gil->owner == thread && gil->lock_count > 0) {
        gil->lock_count--; if (gil->lock_count == 0) { gil->owner = NULL;
#ifdef _WIN32
            WakeConditionVariable(&gil->cond);
#else
            pthread_cond_signal(&gil->cond);
#endif
        }
    }
#ifdef _WIN32
    LeaveCriticalSection(&gil->lock);
#else
    pthread_mutex_unlock(&gil->lock);
#endif
}

bool gil_try_acquire(GIL *gil, Thread *thread) {
#ifdef _WIN32
    if (!TryEnterCriticalSection(&gil->lock)) return false;
#else
    if (pthread_mutex_trylock(&gil->lock) != 0) return false;
#endif
    if (!gil->owner) { gil->owner = thread; gil->lock_count = 1;
#ifdef _WIN32
        LeaveCriticalSection(&gil->lock);
#else
        pthread_mutex_unlock(&gil->lock);
#endif
        return true; }
#ifdef _WIN32
    LeaveCriticalSection(&gil->lock);
#else
    pthread_mutex_unlock(&gil->lock);
#endif
    return false;
}

bool gil_is_owner(GIL *gil, Thread *thread) { return gil->owner == thread; }

Thread *thread_create(void *(*func)(void *), void *args, const char *name) {
    Thread *t = (Thread *)calloc(1, sizeof(Thread)); if (!t) return NULL; t->func = func; t->args = args; t->name = name ? _strdup(name) : NULL; t->state = THREAD_STATE_CREATED; return t;
}

bool thread_start(Thread *thread) {
#ifdef _WIN32
    thread->native_thread = CreateThread(NULL, 0, thread_start_routine, thread, 0, NULL);
    return thread->native_thread != NULL;
#else
    return pthread_create(&thread->native_thread, NULL, thread_start_routine, thread) == 0;
#endif
}

void *thread_join(Thread *thread) {
#ifdef _WIN32
    WaitForSingleObject(thread->native_thread, INFINITE);
#else
    pthread_join(thread->native_thread, NULL);
#endif
    thread->joined = true; return thread->result;
}

void thread_detach(Thread *thread) {
#ifdef _WIN32
    CloseHandle(thread->native_thread);
#else
    pthread_detach(thread->native_thread);
#endif
}

Thread *thread_current(void) { return NULL; }
int thread_get_id(Thread *thread) { (void)thread; return 0; }
void thread_set_name(Thread *thread, const char *name) { (void)thread; (void)name; }
bool thread_is_alive(Thread *thread) { return thread ? thread->state != THREAD_STATE_FINISHED : false; }

void thread_sleep(uint64_t ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    usleep(ms * 1000);
#endif
}

void thread_yield(void) {
#ifdef _WIN32
    SwitchToThread();
#else
    sched_yield();
#endif
}

Mutex *mutex_create(void) { Mutex *m = (Mutex *)calloc(1, sizeof(Mutex)); if (!m) return NULL; 
#ifdef _WIN32
    InitializeCriticalSection(&m->native_mutex);
#else
    pthread_mutex_init(&m->native_mutex, NULL);
#endif
    m->initialized = true; return m; }
void mutex_destroy(Mutex *mtx) { if (!mtx) return; 
#ifdef _WIN32
    DeleteCriticalSection(&mtx->native_mutex);
#else
    pthread_mutex_destroy(&mtx->native_mutex);
#endif
    free(mtx); }
void mutex_lock(Mutex *mtx) { 
#ifdef _WIN32
    EnterCriticalSection(&mtx->native_mutex);
#else
    pthread_mutex_lock(&mtx->native_mutex);
#endif
}
void mutex_unlock(Mutex *mtx) { 
#ifdef _WIN32
    LeaveCriticalSection(&mtx->native_mutex);
#else
    pthread_mutex_unlock(&mtx->native_mutex);
#endif
}
bool mutex_try_lock(Mutex *mtx) {
#ifdef _WIN32
    return TryEnterCriticalSection(&mtx->native_mutex) != 0;
#else
    return pthread_mutex_trylock(&mtx->native_mutex) == 0;
#endif
}

CondVar *condvar_create(void) { CondVar *cv = (CondVar *)calloc(1, sizeof(CondVar)); if (!cv) return NULL; 
#ifdef _WIN32
    InitializeConditionVariable(&cv->native_cond);
#else
    pthread_cond_init(&cv->native_cond, NULL);
#endif
    cv->initialized = true; return cv; }
void condvar_destroy(CondVar *cv) { if (!cv) return; 
#ifndef _WIN32
    pthread_cond_destroy(&cv->native_cond);
#endif
    free(cv); }
void condvar_wait(CondVar *cv, Mutex *mtx) { 
#ifdef _WIN32
    SleepConditionVariableCS(&cv->native_cond, &mtx->native_mutex, INFINITE);
#else
    pthread_cond_wait(&cv->native_cond, &mtx->native_mutex);
#endif
}
bool condvar_wait_timeout(CondVar *cv, Mutex *mtx, uint64_t timeout_ms) { 
#ifdef _WIN32
    return SleepConditionVariableCS(&cv->native_cond, &mtx->native_mutex, (DWORD)timeout_ms) != 0;
#else
    struct timespec ts; ts.tv_sec = timeout_ms / 1000; ts.tv_nsec = (timeout_ms % 1000) * 1000000; return pthread_cond_timedwait(&cv->native_cond, &mtx->native_mutex, &ts) == 0;
#endif
}
void condvar_signal(CondVar *cv) { 
#ifdef _WIN32
    WakeConditionVariable(&cv->native_cond);
#else
    pthread_cond_signal(&cv->native_cond);
#endif
}
void condvar_broadcast(CondVar *cv) { 
#ifdef _WIN32
    WakeAllConditionVariable(&cv->native_cond);
#else
    pthread_cond_broadcast(&cv->native_cond);
#endif
}

Semaphore *semaphore_create(int initial_count, int max_count) { Semaphore *s = (Semaphore *)calloc(1, sizeof(Semaphore)); if (!s) return NULL; s->mutex = mutex_create(); s->cond = condvar_create(); s->count = initial_count; s->max_count = max_count; return s; }
void semaphore_destroy(Semaphore *sem) { if (!sem) return; mutex_destroy(sem->mutex); condvar_destroy(sem->cond); free(sem); }
void semaphore_wait(Semaphore *sem) { mutex_lock(sem->mutex); while (sem->count == 0) condvar_wait(sem->cond, sem->mutex); sem->count--; mutex_unlock(sem->mutex); }
bool semaphore_try_wait(Semaphore *sem) { bool ok = false; mutex_lock(sem->mutex); if (sem->count > 0) { sem->count--; ok = true; } mutex_unlock(sem->mutex); return ok; }
bool semaphore_wait_timeout(Semaphore *sem, uint64_t timeout_ms) { bool ok = false; mutex_lock(sem->mutex); while (sem->count == 0) { if (!condvar_wait_timeout(sem->cond, sem->mutex, timeout_ms)) { ok = false; break; } } if (sem->count > 0) { sem->count--; ok = true; } mutex_unlock(sem->mutex); return ok; }
void semaphore_post(Semaphore *sem) { mutex_lock(sem->mutex); if (sem->count < sem->max_count) sem->count++; condvar_signal(sem->cond); mutex_unlock(sem->mutex); }

RWLock *rwlock_create(void) { RWLock *r = (RWLock *)calloc(1, sizeof(RWLock)); r->mutex = mutex_create(); r->read_cond = condvar_create(); r->write_cond = condvar_create(); return r; }
void rwlock_destroy(RWLock *r) { if (!r) return; mutex_destroy(r->mutex); condvar_destroy(r->read_cond); condvar_destroy(r->write_cond); free(r); }
void rwlock_read_lock(RWLock *r) { mutex_lock(r->mutex); while (r->writers > 0 || r->waiting_writers > 0) condvar_wait(r->read_cond, r->mutex); r->readers++; mutex_unlock(r->mutex); }
void rwlock_read_unlock(RWLock *r) { mutex_lock(r->mutex); r->readers--; if (r->readers == 0) condvar_signal(r->write_cond); mutex_unlock(r->mutex); }
void rwlock_write_lock(RWLock *r) { mutex_lock(r->mutex); r->waiting_writers++; while (r->readers > 0 || r->writers > 0) condvar_wait(r->write_cond, r->mutex); r->waiting_writers--; r->writers++; mutex_unlock(r->mutex); }
void rwlock_write_unlock(RWLock *r) { mutex_lock(r->mutex); r->writers--; if (r->waiting_writers > 0) condvar_signal(r->write_cond); else condvar_broadcast(r->read_cond); mutex_unlock(r->mutex); }

ThreadLocal *thread_local_create(void) { ThreadLocal *t = (ThreadLocal *)calloc(1, sizeof(ThreadLocal)); t->capacity = 1; t->values = (void **)calloc(t->capacity, sizeof(void *)); return t; }
void thread_local_destroy(ThreadLocal *tls) { if (!tls) return; free(tls->values); free(tls); }
void *thread_local_get(ThreadLocal *tls) { return tls && tls->capacity ? tls->values[0] : NULL; }
void thread_local_set(ThreadLocal *tls, void *value) { if (!tls) return; if (!tls->capacity) { tls->capacity = 1; tls->values = (void **)calloc(1, sizeof(void *)); } tls->values[0] = value; }

AtomicInt *atomic_int_create(int initial) { AtomicInt *ai = (AtomicInt *)calloc(1, sizeof(AtomicInt)); ai->lock = mutex_create(); ai->value = initial; return ai; }
void atomic_int_destroy(AtomicInt *ai) { if (!ai) return; mutex_destroy(ai->lock); free(ai); }
int atomic_int_get(AtomicInt *ai) { int v; mutex_lock(ai->lock); v = ai->value; mutex_unlock(ai->lock); return v; }
void atomic_int_set(AtomicInt *ai, int value) { mutex_lock(ai->lock); ai->value = value; mutex_unlock(ai->lock); }
int atomic_int_add(AtomicInt *ai, int delta) { mutex_lock(ai->lock); ai->value += delta; int v = ai->value; mutex_unlock(ai->lock); return v; }
int atomic_int_increment(AtomicInt *ai) { return atomic_int_add(ai, 1); }
int atomic_int_decrement(AtomicInt *ai) { return atomic_int_add(ai, -1); }
bool atomic_int_compare_and_swap(AtomicInt *ai, int expected, int new_val) { bool ok = false; mutex_lock(ai->lock); if (ai->value == expected) { ai->value = new_val; ok = true; } mutex_unlock(ai->lock); return ok; }

int thread_cpu_count(void) {
#ifdef _WIN32
    SYSTEM_INFO si; GetSystemInfo(&si); return (int)si.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN); return (int)(n > 0 ? n : 1);
#endif
}

bool thread_set_affinity(Thread *thread, int cpu) { (void)thread; (void)cpu; return false; }
bool thread_set_priority(Thread *thread, int priority) { (void)thread; (void)priority; return false; }
