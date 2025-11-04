#ifndef RUBOLT_ASYNC_H
#define RUBOLT_ASYNC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Forward declarations */
typedef struct AsyncTask AsyncTask;
typedef struct Promise Promise;
typedef struct Future Future;

/* Coroutine states */
typedef enum {
    CORO_STATE_CREATED,
    CORO_STATE_RUNNING,
    CORO_STATE_SUSPENDED,
    CORO_STATE_COMPLETED,
    CORO_STATE_FAILED
} CoroState;

/* Async task types */
typedef enum {
    TASK_TYPE_COROUTINE,
    TASK_TYPE_IO,
    TASK_TYPE_TIMER,
    TASK_TYPE_PROMISE
} TaskType;

/* Coroutine context */
typedef struct Coroutine {
    char *name;
    CoroState state;
    void *stack;
    size_t stack_size;
    void *(*func)(void *args);
    void *args;
    void *result;
    void *exception;
    int suspend_point;
    uint64_t resume_time;
    struct Coroutine *awaiting;
    struct Coroutine *next;
} Coroutine;

/* Async task */
struct AsyncTask {
    int id;
    TaskType type;
    Coroutine *coro;
    Promise *promise;
    bool completed;
    bool cancelled;
    void *result;
    void (*callback)(void *result);
    void *callback_data;
    uint64_t created_time;
    struct AsyncTask *next;
};

/* Promise states */
typedef enum {
    PROMISE_PENDING,
    PROMISE_FULFILLED,
    PROMISE_REJECTED
} PromiseState;

/* Promise */
struct Promise {
    PromiseState state;
    void *value;
    void *error;
    void (*on_fulfilled)(void *value, void *context);
    void (*on_rejected)(void *error, void *context);
    void *context;
    struct Promise *next_promise;
};

/* Future (read-only promise) */
struct Future {
    Promise *promise;
    bool ready;
};

/* Async context */
typedef struct AsyncContext {
    Coroutine *coroutines;
    AsyncTask *tasks;
    int next_task_id;
    size_t task_count;
    bool running;
} AsyncContext;

/* ========== ASYNC LIFECYCLE ========== */

/* Initialize async system */
void async_init(AsyncContext *ctx);

/* Shutdown async system */
void async_shutdown(AsyncContext *ctx);

/* Run async context (process tasks) */
void async_run(AsyncContext *ctx);

/* Run until complete */
void async_run_until_complete(AsyncContext *ctx, AsyncTask *task);

/* Stop async context */
void async_stop(AsyncContext *ctx);

/* ========== COROUTINE MANAGEMENT ========== */

/* Create coroutine */
Coroutine *coro_create(const char *name, void *(*func)(void *), void *args);

/* Free coroutine */
void coro_free(Coroutine *coro);

/* Resume coroutine */
void *coro_resume(Coroutine *coro);

/* Suspend coroutine */
void coro_suspend(Coroutine *coro, int suspend_point);

/* Check if coroutine is done */
bool coro_is_done(Coroutine *coro);

/* Get coroutine result */
void *coro_get_result(Coroutine *coro);

/* ========== ASYNC/AWAIT ========== */

/* Create async task */
AsyncTask *async_create_task(AsyncContext *ctx, Coroutine *coro);

/* Await task */
void *async_await(AsyncContext *ctx, AsyncTask *task);

/* Await coroutine */
void *async_await_coro(AsyncContext *ctx, Coroutine *coro);

/* Await multiple tasks */
void **async_await_all(AsyncContext *ctx, AsyncTask **tasks, size_t count);

/* Await first completed task */
void *async_await_first(AsyncContext *ctx, AsyncTask **tasks, size_t count);

/* ========== PROMISES & FUTURES ========== */

/* Create promise */
Promise *promise_new(void);

/* Free promise */
void promise_free(Promise *promise);

/* Resolve promise */
void promise_resolve(Promise *promise, void *value);

/* Reject promise */
void promise_reject(Promise *promise, void *error);

/* Add then handler */
Promise *promise_then(Promise *promise, void (*on_fulfilled)(void *, void *), void *context);

/* Add catch handler */
Promise *promise_catch(Promise *promise, void (*on_rejected)(void *, void *), void *context);

/* Add finally handler */
Promise *promise_finally(Promise *promise, void (*on_complete)(void *), void *context);

/* Create future from promise */
Future *future_from_promise(Promise *promise);

/* Check if future is ready */
bool future_is_ready(Future *future);

/* Get future result (blocking) */
void *future_get(Future *future);

/* Get future result with timeout */
void *future_get_timeout(Future *future, uint64_t timeout_ms);

/* ========== TASK MANAGEMENT ========== */

/* Cancel task */
bool async_cancel_task(AsyncContext *ctx, AsyncTask *task);

/* Check if task is done */
bool async_task_is_done(AsyncTask *task);

/* Get task result */
void *async_task_result(AsyncTask *task);

/* Set task callback */
void async_task_set_callback(AsyncTask *task, void (*callback)(void *), void *data);

/* ========== TIMING ========== */

/* Sleep (async) */
AsyncTask *async_sleep(AsyncContext *ctx, uint64_t ms);

/* Create timer task */
AsyncTask *async_timer(AsyncContext *ctx, uint64_t ms, void (*callback)(void *), void *data);

/* Delay execution */
AsyncTask *async_delay(AsyncContext *ctx, uint64_t ms, void *(*func)(void *), void *args);

/* ========== ASYNC UTILITIES ========== */

/* Gather results from multiple tasks */
void **async_gather(AsyncContext *ctx, AsyncTask **tasks, size_t count);

/* Create task group */
typedef struct {
    AsyncTask **tasks;
    size_t count;
    size_t capacity;
} TaskGroup;

TaskGroup *task_group_new(void);
void task_group_add(TaskGroup *group, AsyncTask *task);
void **task_group_run(AsyncContext *ctx, TaskGroup *group);
void task_group_free(TaskGroup *group);

/* ========== ASYNC GENERATORS ========== */

/* Async generator */
typedef struct AsyncGenerator {
    Coroutine *coro;
    bool exhausted;
    void *current_value;
} AsyncGenerator;

AsyncGenerator *async_generator_new(void *(*func)(void *), void *args);
void *async_generator_next(AsyncContext *ctx, AsyncGenerator *gen);
bool async_generator_has_next(AsyncGenerator *gen);
void async_generator_free(AsyncGenerator *gen);

/* ========== MACROS ========== */

/* Async function definition */
#define ASYNC_FUNC(name) void *name##_async(void *args)

/* Await macro (suspends coroutine) */
#define AWAIT(ctx, task) async_await(ctx, task)

/* Async return */
#define ASYNC_RETURN(value) return (value)

/* Yield (for generators) */
#define YIELD(value) \
    do { \
        /* Save yield point */ \
        return (value); \
    } while (0)

/* Global async context */
extern AsyncContext *global_async_context;

#endif /* RUBOLT_ASYNC_H */
