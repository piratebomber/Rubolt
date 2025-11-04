#include "async.h"
#include "event_loop.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

AsyncContext *global_async_context = NULL;

void async_init(AsyncContext *ctx) {
    ctx->coroutines = NULL;
    ctx->tasks = NULL;
    ctx->next_task_id = 1;
    ctx->task_count = 0;
    ctx->running = false;
}

void async_shutdown(AsyncContext *ctx) {
    while (ctx->tasks) {
        AsyncTask *n = ctx->tasks->next; free(ctx->tasks); ctx->tasks = n;
    }
    while (ctx->coroutines) {
        Coroutine *n = ctx->coroutines->next; free(ctx->coroutines); ctx->coroutines = n;
    }
}

void async_run(AsyncContext *ctx) {
    ctx->running = true;
    while (ctx->running && ctx->tasks) {
        AsyncTask *t = ctx->tasks;
        /* Execute task */
        if (t->coro && t->coro->func) {
            t->result = t->coro->func(t->coro->args);
            t->completed = true;
        }
        if (t->callback) t->callback(t->result);
        /* remove from list */
        ctx->tasks = t->next; free(t); ctx->task_count--;
    }
    ctx->running = false;
}

void async_run_until_complete(AsyncContext *ctx, AsyncTask *task) {
    (void)task; async_run(ctx);
}

void async_stop(AsyncContext *ctx) { ctx->running = false; }

Coroutine *coro_create(const char *name, void *(*func)(void *), void *args) {
    Coroutine *c = (Coroutine *)calloc(1, sizeof(Coroutine));
    if (!c) return NULL; c->name = (name ? _strdup(name) : NULL); c->func = func; c->args = args; c->state = CORO_STATE_CREATED; return c;
}

void coro_free(Coroutine *coro) { if (!coro) return; free(coro->name); free(coro); }
void *coro_resume(Coroutine *coro) { if (!coro || !coro->func) return NULL; coro->state = CORO_STATE_RUNNING; void *r = coro->func(coro->args); coro->state = CORO_STATE_COMPLETED; coro->result = r; return r; }
void coro_suspend(Coroutine *coro, int suspend_point) { if (coro) { coro->suspend_point = suspend_point; coro->state = CORO_STATE_SUSPENDED; } }
bool coro_is_done(Coroutine *coro) { return coro ? (coro->state == CORO_STATE_COMPLETED || coro->state == CORO_STATE_FAILED) : true; }
void *coro_get_result(Coroutine *coro) { return coro ? coro->result : NULL; }

AsyncTask *async_create_task(AsyncContext *ctx, Coroutine *coro) {
    AsyncTask *t = (AsyncTask *)calloc(1, sizeof(AsyncTask));
    if (!t) return NULL; t->id = ctx->next_task_id++; t->type = TASK_TYPE_COROUTINE; t->coro = coro; t->next = ctx->tasks; ctx->tasks = t; ctx->task_count++; return t;
}

void *async_await(AsyncContext *ctx, AsyncTask *task) { (void)ctx; if (!task) return NULL; if (task->coro && !task->completed) task->result = coro_resume(task->coro); task->completed = true; return task->result; }
void *async_await_coro(AsyncContext *ctx, Coroutine *coro) { AsyncTask *t = async_create_task(ctx, coro); return async_await(ctx, t); }

void **async_await_all(AsyncContext *ctx, AsyncTask **tasks, size_t count) {
    void **results = (void **)calloc(count, sizeof(void *));
    for (size_t i = 0; i < count; i++) results[i] = async_await(ctx, tasks[i]);
    return results;
}

void *async_await_first(AsyncContext *ctx, AsyncTask **tasks, size_t count) { (void)ctx; return count ? async_await(ctx, tasks[0]) : NULL; }

Promise *promise_new(void) { return (Promise *)calloc(1, sizeof(Promise)); }
void promise_free(Promise *p) { free(p); }
void promise_resolve(Promise *p, void *value) { if (!p) return; p->state = PROMISE_FULFILLED; p->value = value; if (p->on_fulfilled) p->on_fulfilled(value, p->context); }
void promise_reject(Promise *p, void *error) { if (!p) return; p->state = PROMISE_REJECTED; p->error = error; if (p->on_rejected) p->on_rejected(error, p->context); }
Promise *promise_then(Promise *p, void (*on_fulfilled)(void *, void *), void *context) { if (!p) return NULL; p->on_fulfilled = on_fulfilled; p->context = context; return p; }
Promise *promise_catch(Promise *p, void (*on_rejected)(void *, void *), void *context) { if (!p) return NULL; p->on_rejected = on_rejected; p->context = context; return p; }
Promise *promise_finally(Promise *p, void (*on_complete)(void *), void *context) { (void)on_complete; (void)context; return p; }
Future *future_from_promise(Promise *p) { Future *f = (Future *)calloc(1, sizeof(Future)); f->promise = p; f->ready = (p->state != PROMISE_PENDING); return f; }
bool future_is_ready(Future *f) { return f ? f->ready : false; }
void *future_get(Future *f) { return f && f->promise ? f->promise->value : NULL; }
void *future_get_timeout(Future *f, uint64_t timeout_ms) { (void)timeout_ms; return future_get(f); }

bool async_cancel_task(AsyncContext *ctx, AsyncTask *task) { (void)ctx; if (!task) return false; task->cancelled = true; return true; }
bool async_task_is_done(AsyncTask *task) { return task ? task->completed : true; }
void *async_task_result(AsyncTask *task) { return task ? task->result : NULL; }
void async_task_set_callback(AsyncTask *task, void (*callback)(void *), void *data) { if (task) { task->callback = callback; task->callback_data = data; } }

AsyncTask *async_sleep(AsyncContext *ctx, uint64_t ms) { (void)ms; Coroutine *c = coro_create("sleep", NULL, NULL); return async_create_task(ctx, c); }
AsyncTask *async_timer(AsyncContext *ctx, uint64_t ms, void (*callback)(void *), void *data) { AsyncTask *t = async_sleep(ctx, ms); t->callback = callback; t->callback_data = data; return t; }
AsyncTask *async_delay(AsyncContext *ctx, uint64_t ms, void *(*func)(void *), void *args) { (void)ms; Coroutine *c = coro_create("delay", func, args); return async_create_task(ctx, c); }

void **async_gather(AsyncContext *ctx, AsyncTask **tasks, size_t count) { return async_await_all(ctx, tasks, count); }

TaskGroup *task_group_new(void) { return (TaskGroup *)calloc(1, sizeof(TaskGroup)); }
void task_group_add(TaskGroup *group, AsyncTask *task) { if (!group) return; if (group->count + 1 > group->capacity) { size_t nc = group->capacity ? group->capacity * 2 : 4; group->tasks = (AsyncTask **)realloc(group->tasks, nc * sizeof(AsyncTask *)); group->capacity = nc; } group->tasks[group->count++] = task; }
void **task_group_run(AsyncContext *ctx, TaskGroup *group) { return group ? async_gather(ctx, group->tasks, group->count) : NULL; }
void task_group_free(TaskGroup *group) { if (!group) return; free(group->tasks); free(group); }

AsyncGenerator *async_generator_new(void *(*func)(void *), void *args) { Coroutine *c = coro_create("agen", func, args); AsyncGenerator *g = (AsyncGenerator *)calloc(1, sizeof(AsyncGenerator)); g->coro = c; return g; }
void *async_generator_next(AsyncContext *ctx, AsyncGenerator *gen) { if (!gen || !gen->coro) return NULL; if (coro_is_done(gen->coro)) { gen->exhausted = true; return NULL; } return async_await_coro(ctx, gen->coro); }
bool async_generator_has_next(AsyncGenerator *gen) { return gen ? !gen->exhausted : false; }
void async_generator_free(AsyncGenerator *gen) { if (!gen) return; coro_free(gen->coro); free(gen); }
