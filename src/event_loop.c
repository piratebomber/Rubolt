#include "event_loop.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
static uint64_t now_ms(void){ return GetTickCount64(); }
#else
#include <time.h>
static uint64_t now_ms(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return (uint64_t)ts.tv_sec*1000ull + ts.tv_nsec/1000000ull; }
#endif

EventLoop *global_event_loop = NULL;

static Event *event_new(EventType type, EventCallback cb, void *data) {
    Event *e = (Event *)calloc(1, sizeof(Event)); if (!e) return NULL; e->type = type; e->callback = cb; e->data = data; e->active = true; return e;
}

EventLoop *event_loop_new(void) {
    EventLoop *loop = (EventLoop *)calloc(1, sizeof(EventLoop));
    if (!loop) return NULL; loop->running = false; loop->next_event_id = 1; loop->last_tick_time = now_ms(); return loop;
}

void event_loop_free(EventLoop *loop) {
    if (!loop) return; while (loop->events) { Event *n = loop->events->next; free(loop->events); loop->events = n; } free(loop);
}

void event_loop_run(EventLoop *loop) {
    loop->running = true; while (loop->running) { if (!event_loop_run_once(loop)) break; }
}

bool event_loop_run_once(EventLoop *loop) {
    event_loop_process_events(loop, 10);
    event_loop_fire_timers(loop);
    return loop->events != NULL || loop->running;
}

void event_loop_run_until(EventLoop *loop, bool (*condition)(void *), void *data) {
    while (loop->running && !condition(data)) event_loop_run_once(loop);
}

void event_loop_stop(EventLoop *loop) { loop->running = false; }

bool event_loop_is_running(EventLoop *loop) { return loop->running; }

int event_loop_add_read(EventLoop *loop, int fd, EventCallback callback, void *data) {
    (void)fd; Event *e = event_new(EVENT_IO_READ, callback, data); e->id = loop->next_event_id++; e->next = loop->events; loop->events = e; loop->event_count++; return e->id;
}

int event_loop_add_write(EventLoop *loop, int fd, EventCallback callback, void *data) {
    (void)fd; Event *e = event_new(EVENT_IO_WRITE, callback, data); e->id = loop->next_event_id++; e->next = loop->events; loop->events = e; loop->event_count++; return e->id;
}

int event_loop_add_timer(EventLoop *loop, uint64_t timeout_ms, EventCallback callback, void *data) {
    Event *e = event_new(EVENT_TIMER, callback, data); e->id = loop->next_event_id++; e->timeout_ms = timeout_ms; e->fire_time = now_ms() + timeout_ms; e->next = loop->events; loop->events = e; loop->event_count++; return e->id;
}

int event_loop_add_timer_recurring(EventLoop *loop, uint64_t interval_ms, EventCallback callback, void *data) {
    Event *e = event_new(EVENT_TIMER, callback, data); e->id = loop->next_event_id++; e->timeout_ms = interval_ms; e->fire_time = now_ms() + interval_ms; e->recurring = true; e->next = loop->events; loop->events = e; loop->event_count++; return e->id;
}

int event_loop_add_event(EventLoop *loop, EventType type, EventCallback callback, void *data) {
    Event *e = event_new(type, callback, data); e->id = loop->next_event_id++; e->next = loop->events; loop->events = e; loop->event_count++; return e->id;
}

bool event_loop_remove_event(EventLoop *loop, int event_id) {
    Event *prev = NULL, *e = loop->events; while (e) { if (e->id == event_id) { if (prev) prev->next = e->next; else loop->events = e->next; free(e); loop->event_count--; return true; } prev = e; e = e->next; } return false;
}

void event_loop_remove_fd_events(EventLoop *loop, int fd) { (void)loop; (void)fd; }

IOOperation *event_loop_async_read(EventLoop *loop, int fd, void *buffer, size_t size, EventCallback on_complete, void *context) {
    (void)fd; (void)size; IOOperation *op = (IOOperation *)calloc(1, sizeof(IOOperation)); op->buffer = buffer; op->on_complete = on_complete; op->context = context; op->completed = true; if (on_complete) on_complete(context); return op;
}

IOOperation *event_loop_async_write(EventLoop *loop, int fd, const void *buffer, size_t size, EventCallback on_complete, void *context) {
    (void)fd; (void)buffer; (void)size; IOOperation *op = (IOOperation *)calloc(1, sizeof(IOOperation)); op->on_complete = on_complete; op->context = context; op->completed = true; if (on_complete) on_complete(context); return op;
}

bool event_loop_cancel_io(EventLoop *loop, IOOperation *op) { (void)loop; if (!op) return false; free(op); return true; }

int event_loop_call_soon(EventLoop *loop, EventCallback callback, void *data) { return event_loop_add_event(loop, EVENT_CUSTOM, callback, data); }
int event_loop_call_later(EventLoop *loop, uint64_t delay_ms, EventCallback callback, void *data) { return event_loop_add_timer(loop, delay_ms, callback, data); }
int event_loop_call_at(EventLoop *loop, uint64_t timestamp_ms, EventCallback callback, void *data) { uint64_t now = now_ms(); if (timestamp_ms <= now) return event_loop_call_soon(loop, callback, data); else return event_loop_add_timer(loop, timestamp_ms - now, callback, data); }

uint64_t event_loop_time(EventLoop *loop) { (void)loop; return now_ms(); }
uint64_t event_loop_uptime(EventLoop *loop) { return now_ms() - loop->last_tick_time; }
size_t event_loop_event_count(EventLoop *loop) { return loop->event_count; }
uint64_t event_loop_iteration_count(EventLoop *loop) { return loop->iteration_count; }
uint64_t event_loop_next_timer(EventLoop *loop) { return loop->next_timer_fire; }

void event_loop_print_stats(EventLoop *loop) { printf("events=%zu iterations=%llu\n", loop->event_count, (unsigned long long)loop->iteration_count); }

void event_loop_run_async_task(EventLoop *loop, void *task) { (void)loop; (void)task; }
void event_loop_schedule_coro(EventLoop *loop, void *coro) { (void)loop; (void)coro; }

void event_loop_process_events(EventLoop *loop, uint64_t timeout_ms) {
    (void)timeout_ms; loop->iteration_count++;
    /* Process custom and I/O events immediately */
    Event *prev = NULL, *e = loop->events;
    uint64_t now = now_ms();
    while (e) {
        bool fired = false;
        if (e->type == EVENT_TIMER) {
            if (now >= e->fire_time) { fired = true; }
        } else if (e->type == EVENT_CUSTOM || e->type == EVENT_IO_READ || e->type == EVENT_IO_WRITE) {
            fired = true;
        }
        if (fired && e->callback) e->callback(e->data);
        if (fired && !e->recurring) {
            Event *to_free = e; e = e->next; if (prev) prev->next = e; else loop->events = e; free(to_free); loop->event_count--; continue;
        }
        if (fired && e->recurring) { e->fire_time = now + e->timeout_ms; }
        prev = e; e = e->next;
    }
}

void event_loop_fire_timers(EventLoop *loop) { (void)loop; }

#ifdef _WIN32
bool event_loop_init_iocp(EventLoop *loop) { (void)loop; return true; }
void event_loop_cleanup_iocp(EventLoop *loop) { (void)loop; }
#else
bool event_loop_init_epoll(EventLoop *loop) { (void)loop; return true; }
void event_loop_cleanup_epoll(EventLoop *loop) { (void)loop; }
#endif
