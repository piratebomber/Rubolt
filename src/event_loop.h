#ifndef RUBOLT_EVENT_LOOP_H
#define RUBOLT_EVENT_LOOP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* Event types */
typedef enum {
    EVENT_IO_READ,
    EVENT_IO_WRITE,
    EVENT_IO_EXCEPT,
    EVENT_TIMER,
    EVENT_SIGNAL,
    EVENT_CUSTOM
} EventType;

/* Event callback */
typedef void (*EventCallback)(void *data);

/* Event structure */
typedef struct Event {
    int id;
    EventType type;
    int fd;                     /* File descriptor for I/O events */
    uint64_t timeout_ms;        /* Timeout in milliseconds */
    uint64_t fire_time;         /* Absolute time to fire (for timers) */
    EventCallback callback;
    void *data;
    bool recurring;             /* For repeating timers */
    bool active;
    struct Event *next;
} Event;

/* I/O operation */
typedef struct IOOperation {
    int fd;
    void *buffer;
    size_t size;
    size_t offset;
    bool completed;
    int result;
    EventCallback on_complete;
    void *context;
    struct IOOperation *next;
} IOOperation;

/* Event loop */
typedef struct EventLoop {
    Event *events;
    int next_event_id;
    size_t event_count;
    bool running;
    uint64_t iteration_count;
    IOOperation *pending_io;
    
    /* Platform-specific poll data */
#ifdef _WIN32
    void *iocp_handle;          /* I/O completion port */
#else
    int epoll_fd;               /* epoll file descriptor */
#endif
    
    /* Timing */
    uint64_t last_tick_time;
    uint64_t next_timer_fire;
} EventLoop;

/* ========== EVENT LOOP LIFECYCLE ========== */

/* Create event loop */
EventLoop *event_loop_new(void);

/* Free event loop */
void event_loop_free(EventLoop *loop);

/* Run event loop */
void event_loop_run(EventLoop *loop);

/* Run one iteration */
bool event_loop_run_once(EventLoop *loop);

/* Run until callback returns true */
void event_loop_run_until(EventLoop *loop, bool (*condition)(void *), void *data);

/* Stop event loop */
void event_loop_stop(EventLoop *loop);

/* Check if running */
bool event_loop_is_running(EventLoop *loop);

/* ========== EVENT MANAGEMENT ========== */

/* Add I/O read event */
int event_loop_add_read(EventLoop *loop, int fd, EventCallback callback, void *data);

/* Add I/O write event */
int event_loop_add_write(EventLoop *loop, int fd, EventCallback callback, void *data);

/* Add timer event (one-shot) */
int event_loop_add_timer(EventLoop *loop, uint64_t timeout_ms, EventCallback callback, void *data);

/* Add recurring timer */
int event_loop_add_timer_recurring(EventLoop *loop, uint64_t interval_ms, 
                                   EventCallback callback, void *data);

/* Add custom event */
int event_loop_add_event(EventLoop *loop, EventType type, EventCallback callback, void *data);

/* Remove event */
bool event_loop_remove_event(EventLoop *loop, int event_id);

/* Remove all events for fd */
void event_loop_remove_fd_events(EventLoop *loop, int fd);

/* ========== ASYNC I/O ========== */

/* Schedule async read */
IOOperation *event_loop_async_read(EventLoop *loop, int fd, void *buffer, size_t size,
                                   EventCallback on_complete, void *context);

/* Schedule async write */
IOOperation *event_loop_async_write(EventLoop *loop, int fd, const void *buffer, size_t size,
                                    EventCallback on_complete, void *context);

/* Cancel I/O operation */
bool event_loop_cancel_io(EventLoop *loop, IOOperation *op);

/* ========== DEFERRED EXECUTION ========== */

/* Call soon (next iteration) */
int event_loop_call_soon(EventLoop *loop, EventCallback callback, void *data);

/* Call later (after delay) */
int event_loop_call_later(EventLoop *loop, uint64_t delay_ms, 
                          EventCallback callback, void *data);

/* Call at specific time */
int event_loop_call_at(EventLoop *loop, uint64_t timestamp_ms,
                       EventCallback callback, void *data);

/* ========== TIMING ========== */

/* Get current loop time (monotonic) */
uint64_t event_loop_time(EventLoop *loop);

/* Get time since loop started */
uint64_t event_loop_uptime(EventLoop *loop);

/* ========== MONITORING ========== */

/* Get event count */
size_t event_loop_event_count(EventLoop *loop);

/* Get iteration count */
uint64_t event_loop_iteration_count(EventLoop *loop);

/* Get next timer fire time */
uint64_t event_loop_next_timer(EventLoop *loop);

/* Print loop statistics */
void event_loop_print_stats(EventLoop *loop);

/* ========== INTEGRATION WITH ASYNC ========== */

/* Run async task in event loop */
void event_loop_run_async_task(EventLoop *loop, void *task);

/* Schedule coroutine */
void event_loop_schedule_coro(EventLoop *loop, void *coro);

/* ========== UTILITIES ========== */

/* Process events (internal) */
void event_loop_process_events(EventLoop *loop, uint64_t timeout_ms);

/* Fire timers (internal) */
void event_loop_fire_timers(EventLoop *loop);

/* Process I/O completions (internal) */
void event_loop_process_io(EventLoop *loop);

/* ========== PLATFORM SPECIFICS ========== */

#ifdef _WIN32
/* Windows IOCP functions */
bool event_loop_init_iocp(EventLoop *loop);
void event_loop_cleanup_iocp(EventLoop *loop);
#else
/* Linux epoll functions */
bool event_loop_init_epoll(EventLoop *loop);
void event_loop_cleanup_epoll(EventLoop *loop);
#endif

/* Global event loop */
extern EventLoop *global_event_loop;

#endif /* RUBOLT_EVENT_LOOP_H */
