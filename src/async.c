#include "async.h"
#include "interpreter.h"
#include "event_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

// Global async runtime
static AsyncRuntime *global_runtime = NULL;

// Task management
AsyncTask *async_create_task(Interpreter *interp, Expr *expr) {
    AsyncTask *task = malloc(sizeof(AsyncTask));
    task->id = generate_task_id();
    task->state = TASK_PENDING;
    task->expression = expr;
    task->interpreter = interp;
    task->result = value_null();
    task->error = NULL;
    task->continuation = NULL;
    task->stack_pointer = NULL;
    task->next = NULL;
    task->prev = NULL;
    task->created_at = get_current_time_ms();
    task->started_at = 0;
    task->completed_at = 0;
    
    return task;
}

void async_schedule_task(AsyncTask *task) {
    if (!global_runtime) {
        global_runtime = async_runtime_create();
    }
    
    pthread_mutex_lock(&global_runtime->task_queue_mutex);
    
    // Add to ready queue
    if (global_runtime->ready_queue_tail) {
        global_runtime->ready_queue_tail->next = task;
        task->prev = global_runtime->ready_queue_tail;
    } else {
        global_runtime->ready_queue_head = task;
    }
    global_runtime->ready_queue_tail = task;
    global_runtime->ready_task_count++;
    
    pthread_mutex_unlock(&global_runtime->task_queue_mutex);
    
    // Signal event loop
    pthread_cond_signal(&global_runtime->task_available);
}

AsyncTask *async_dequeue_ready_task(AsyncRuntime *runtime) {
    pthread_mutex_lock(&runtime->task_queue_mutex);
    
    AsyncTask *task = runtime->ready_queue_head;
    if (task) {
        runtime->ready_queue_head = task->next;
        if (runtime->ready_queue_head) {
            runtime->ready_queue_head->prev = NULL;
        } else {
            runtime->ready_queue_tail = NULL;
        }
        
        task->next = NULL;
        task->prev = NULL;
        runtime->ready_task_count--;
    }
    
    pthread_mutex_unlock(&runtime->task_queue_mutex);
    return task;
}

void async_complete_task(AsyncTask *task, Value result) {
    task->state = TASK_COMPLETED;
    task->result = result;
    task->completed_at = get_current_time_ms();
    
    // Notify waiting tasks
    if (task->continuation) {
        async_schedule_task(task->continuation);
    }
}

void async_fail_task(AsyncTask *task, const char *error) {
    task->state = TASK_FAILED;
    task->error = strdup(error);
    task->completed_at = get_current_time_ms();
    
    // Notify waiting tasks
    if (task->continuation) {
        async_schedule_task(task->continuation);
    }
}

// Promise implementation
Promise *promise_create(void) {
    Promise *promise = malloc(sizeof(Promise));
    promise->state = PROMISE_PENDING;
    promise->value = value_null();
    promise->error = NULL;
    promise->then_callbacks = NULL;
    promise->catch_callbacks = NULL;
    promise->callback_count = 0;
    pthread_mutex_init(&promise->mutex, NULL);
    
    return promise;
}

void promise_resolve(Promise *promise, Value value) {
    pthread_mutex_lock(&promise->mutex);
    
    if (promise->state == PROMISE_PENDING) {
        promise->state = PROMISE_RESOLVED;
        promise->value = value;
        
        // Execute then callbacks
        for (size_t i = 0; i < promise->callback_count; i++) {
            if (promise->then_callbacks[i]) {
                AsyncTask *task = async_create_task(NULL, NULL);
                task->state = TASK_RUNNING;
                // Execute callback with value
                promise->then_callbacks[i](value);
                async_complete_task(task, value_null());
            }
        }
    }
    
    pthread_mutex_unlock(&promise->mutex);
}

void promise_reject(Promise *promise, const char *error) {
    pthread_mutex_lock(&promise->mutex);
    
    if (promise->state == PROMISE_PENDING) {
        promise->state = PROMISE_REJECTED;
        promise->error = strdup(error);
        
        // Execute catch callbacks
        for (size_t i = 0; i < promise->callback_count; i++) {
            if (promise->catch_callbacks[i]) {
                AsyncTask *task = async_create_task(NULL, NULL);
                task->state = TASK_RUNNING;
                // Execute callback with error
                promise->catch_callbacks[i](error);
                async_complete_task(task, value_null());
            }
        }
    }
    
    pthread_mutex_unlock(&promise->mutex);
}

Promise *promise_then(Promise *promise, PromiseCallback callback) {
    Promise *new_promise = promise_create();
    
    pthread_mutex_lock(&promise->mutex);
    
    if (promise->state == PROMISE_RESOLVED) {
        // Already resolved, execute immediately
        Value result = callback(promise->value);
        promise_resolve(new_promise, result);
    } else if (promise->state == PROMISE_PENDING) {
        // Add to callback list
        promise->callback_count++;
        promise->then_callbacks = realloc(promise->then_callbacks, 
                                         sizeof(PromiseCallback) * promise->callback_count);
        promise->then_callbacks[promise->callback_count - 1] = callback;
    }
    
    pthread_mutex_unlock(&promise->mutex);
    return new_promise;
}

Promise *promise_catch(Promise *promise, PromiseErrorCallback callback) {
    Promise *new_promise = promise_create();
    
    pthread_mutex_lock(&promise->mutex);
    
    if (promise->state == PROMISE_REJECTED) {
        // Already rejected, execute immediately
        callback(promise->error);
        promise_resolve(new_promise, value_null());
    } else if (promise->state == PROMISE_PENDING) {
        // Add to callback list
        promise->callback_count++;
        promise->catch_callbacks = realloc(promise->catch_callbacks, 
                                          sizeof(PromiseErrorCallback) * promise->callback_count);
        promise->catch_callbacks[promise->callback_count - 1] = callback;
    }
    
    pthread_mutex_unlock(&promise->mutex);
    return new_promise;
}

Promise *promise_all(Promise **promises, size_t count) {
    Promise *result_promise = promise_create();
    
    if (count == 0) {
        promise_resolve(result_promise, value_null());
        return result_promise;
    }
    
    PromiseAllState *state = malloc(sizeof(PromiseAllState));
    state->result_promise = result_promise;
    state->total_count = count;
    state->completed_count = 0;
    state->results = malloc(sizeof(Value) * count);
    state->failed = false;
    pthread_mutex_init(&state->mutex, NULL);
    
    for (size_t i = 0; i < count; i++) {
        promise_then(promises[i], create_promise_all_callback(state, i));
        promise_catch(promises[i], create_promise_all_error_callback(state));
    }
    
    return result_promise;
}

Promise *promise_race(Promise **promises, size_t count) {
    Promise *result_promise = promise_create();
    
    if (count == 0) {
        promise_resolve(result_promise, value_null());
        return result_promise;
    }
    
    for (size_t i = 0; i < count; i++) {
        promise_then(promises[i], create_promise_race_callback(result_promise));
        promise_catch(promises[i], create_promise_race_error_callback(result_promise));
    }
    
    return result_promise;
}

// Async/await implementation
Value async_await(AsyncTask *task) {
    if (!task) return value_null();
    
    // If task is already completed, return result immediately
    if (task->state == TASK_COMPLETED) {
        return task->result;
    }
    
    if (task->state == TASK_FAILED) {
        // Throw exception
        throw_async_exception(task->error);
        return value_null();
    }
    
    // Suspend current task and wait for completion
    AsyncTask *current_task = get_current_task();
    if (current_task) {
        current_task->state = TASK_SUSPENDED;
        task->continuation = current_task;
        
        // Save execution state
        save_task_state(current_task);
        
        // Yield to event loop
        yield_to_event_loop();
        
        // Execution resumes here when awaited task completes
        restore_task_state(current_task);
        
        if (task->state == TASK_COMPLETED) {
            return task->result;
        } else if (task->state == TASK_FAILED) {
            throw_async_exception(task->error);
        }
    }
    
    return value_null();
}

AsyncTask *async_spawn(AsyncFunction func, void *args) {
    AsyncTask *task = malloc(sizeof(AsyncTask));
    task->id = generate_task_id();
    task->state = TASK_PENDING;
    task->async_function = func;
    task->args = args;
    task->interpreter = current_interpreter;
    task->result = value_null();
    task->error = NULL;
    task->continuation = NULL;
    task->next = NULL;
    task->prev = NULL;
    task->created_at = get_current_time_ms();
    
    async_schedule_task(task);
    return task;
}

// Event loop implementation
AsyncRuntime *async_runtime_create(void) {
    AsyncRuntime *runtime = malloc(sizeof(AsyncRuntime));
    runtime->running = false;
    runtime->ready_queue_head = NULL;
    runtime->ready_queue_tail = NULL;
    runtime->ready_task_count = 0;
    runtime->suspended_tasks = NULL;
    runtime->suspended_task_count = 0;
    runtime->timers = NULL;
    runtime->timer_count = 0;
    runtime->io_watchers = NULL;
    runtime->io_watcher_count = 0;
    
    pthread_mutex_init(&runtime->task_queue_mutex, NULL);
    pthread_cond_init(&runtime->task_available, NULL);
    
    // Create epoll instance for I/O events
    runtime->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (runtime->epoll_fd == -1) {
        perror("epoll_create1");
        free(runtime);
        return NULL;
    }
    
    return runtime;
}

void async_runtime_run(AsyncRuntime *runtime) {
    runtime->running = true;
    
    while (runtime->running) {
        // Process ready tasks
        AsyncTask *task = async_dequeue_ready_task(runtime);
        if (task) {
            execute_async_task(runtime, task);
            continue;
        }
        
        // Process I/O events
        process_io_events(runtime);
        
        // Process timers
        process_timers(runtime);
        
        // If no tasks are ready, wait for events
        if (runtime->ready_task_count == 0 && 
            runtime->suspended_task_count == 0 && 
            runtime->timer_count == 0) {
            runtime->running = false;
            break;
        }
        
        // Wait for new tasks or events
        pthread_mutex_lock(&runtime->task_queue_mutex);
        if (runtime->ready_task_count == 0) {
            struct timespec timeout = {0, 1000000}; // 1ms timeout
            pthread_cond_timedwait(&runtime->task_available, 
                                  &runtime->task_queue_mutex, &timeout);
        }
        pthread_mutex_unlock(&runtime->task_queue_mutex);
    }
}

void execute_async_task(AsyncRuntime *runtime, AsyncTask *task) {
    task->state = TASK_RUNNING;
    task->started_at = get_current_time_ms();
    
    set_current_task(task);
    
    try {
        if (task->async_function) {
            // Execute async function
            task->result = task->async_function(task->args);
            async_complete_task(task, task->result);
        } else if (task->expression) {
            // Execute expression
            task->result = evaluate_expression(task->interpreter, task->expression);
            async_complete_task(task, task->result);
        }
    } catch (Exception *e) {
        async_fail_task(task, e->message);
    }
    
    set_current_task(NULL);
}

void process_io_events(AsyncRuntime *runtime) {
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(runtime->epoll_fd, events, MAX_EVENTS, 0);
    
    for (int i = 0; i < nfds; i++) {
        IOWatcher *watcher = (IOWatcher *)events[i].data.ptr;
        
        if (events[i].events & EPOLLIN) {
            // Data available for reading
            if (watcher->read_callback) {
                watcher->read_callback(watcher->fd, watcher->data);
            }
        }
        
        if (events[i].events & EPOLLOUT) {
            // Ready for writing
            if (watcher->write_callback) {
                watcher->write_callback(watcher->fd, watcher->data);
            }
        }
        
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            // Error or hangup
            if (watcher->error_callback) {
                watcher->error_callback(watcher->fd, watcher->data);
            }
        }
    }
}

void process_timers(AsyncRuntime *runtime) {
    uint64_t current_time = get_current_time_ms();
    
    for (size_t i = 0; i < runtime->timer_count; i++) {
        Timer *timer = &runtime->timers[i];
        
        if (timer->active && current_time >= timer->expires_at) {
            // Timer expired
            timer->callback(timer->data);
            
            if (timer->repeat) {
                // Reschedule repeating timer
                timer->expires_at = current_time + timer->interval;
            } else {
                // Remove one-shot timer
                timer->active = false;
            }
        }
    }
    
    // Compact timer array
    size_t write_index = 0;
    for (size_t read_index = 0; read_index < runtime->timer_count; read_index++) {
        if (runtime->timers[read_index].active) {
            if (write_index != read_index) {
                runtime->timers[write_index] = runtime->timers[read_index];
            }
            write_index++;
        }
    }
    runtime->timer_count = write_index;
}

// Timer management
Timer *async_set_timeout(TimerCallback callback, uint64_t delay, void *data) {
    if (!global_runtime) {
        global_runtime = async_runtime_create();
    }
    
    AsyncRuntime *runtime = global_runtime;
    
    // Expand timer array if needed
    if (runtime->timer_count >= runtime->timer_capacity) {
        runtime->timer_capacity = runtime->timer_capacity ? runtime->timer_capacity * 2 : 16;
        runtime->timers = realloc(runtime->timers, 
                                 sizeof(Timer) * runtime->timer_capacity);
    }
    
    Timer *timer = &runtime->timers[runtime->timer_count++];
    timer->id = generate_timer_id();
    timer->callback = callback;
    timer->data = data;
    timer->expires_at = get_current_time_ms() + delay;
    timer->interval = delay;
    timer->repeat = false;
    timer->active = true;
    
    return timer;
}

Timer *async_set_interval(TimerCallback callback, uint64_t interval, void *data) {
    Timer *timer = async_set_timeout(callback, interval, data);
    timer->repeat = true;
    return timer;
}

void async_clear_timer(Timer *timer) {
    if (timer) {
        timer->active = false;
    }
}

// I/O watcher management
IOWatcher *async_watch_fd(int fd, uint32_t events, 
                         IOCallback read_cb, IOCallback write_cb, 
                         IOCallback error_cb, void *data) {
    if (!global_runtime) {
        global_runtime = async_runtime_create();
    }
    
    AsyncRuntime *runtime = global_runtime;
    
    IOWatcher *watcher = malloc(sizeof(IOWatcher));
    watcher->fd = fd;
    watcher->events = events;
    watcher->read_callback = read_cb;
    watcher->write_callback = write_cb;
    watcher->error_callback = error_cb;
    watcher->data = data;
    watcher->active = true;
    
    // Add to epoll
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = watcher;
    
    if (epoll_ctl(runtime->epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        perror("epoll_ctl");
        free(watcher);
        return NULL;
    }
    
    // Add to watcher list
    runtime->io_watcher_count++;
    runtime->io_watchers = realloc(runtime->io_watchers, 
                                  sizeof(IOWatcher *) * runtime->io_watcher_count);
    runtime->io_watchers[runtime->io_watcher_count - 1] = watcher;
    
    return watcher;
}

void async_unwatch_fd(IOWatcher *watcher) {
    if (!watcher || !global_runtime) return;
    
    AsyncRuntime *runtime = global_runtime;
    
    // Remove from epoll
    epoll_ctl(runtime->epoll_fd, EPOLL_CTL_DEL, watcher->fd, NULL);
    
    // Remove from watcher list
    for (size_t i = 0; i < runtime->io_watcher_count; i++) {
        if (runtime->io_watchers[i] == watcher) {
            memmove(&runtime->io_watchers[i], &runtime->io_watchers[i + 1],
                   (runtime->io_watcher_count - i - 1) * sizeof(IOWatcher *));
            runtime->io_watcher_count--;
            break;
        }
    }
    
    watcher->active = false;
    free(watcher);
}

// Async HTTP client
AsyncTask *async_http_get(const char *url) {
    HTTPRequest *request = malloc(sizeof(HTTPRequest));
    request->method = strdup("GET");
    request->url = strdup(url);
    request->headers = NULL;
    request->body = NULL;
    
    return async_spawn(execute_http_request, request);
}

AsyncTask *async_http_post(const char *url, const char *body, const char *content_type) {
    HTTPRequest *request = malloc(sizeof(HTTPRequest));
    request->method = strdup("POST");
    request->url = strdup(url);
    request->body = strdup(body);
    
    // Add content-type header
    request->headers = malloc(sizeof(HTTPHeader) * 2);
    request->headers[0].name = strdup("Content-Type");
    request->headers[0].value = strdup(content_type);
    request->headers[1].name = NULL; // Null terminator
    
    return async_spawn(execute_http_request, request);
}

Value execute_http_request(void *args) {
    HTTPRequest *request = (HTTPRequest *)args;
    
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return value_string("Error: Failed to create socket");
    }
    
    // Parse URL and connect
    URLComponents *url = parse_url(request->url);
    if (!url) {
        close(sockfd);
        return value_string("Error: Invalid URL");
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(url->port ? url->port : 80);
    
    if (inet_pton(AF_INET, url->host, &server_addr.sin_addr) <= 0) {
        // Try to resolve hostname
        struct hostent *host_entry = gethostbyname(url->host);
        if (!host_entry) {
            close(sockfd);
            free_url_components(url);
            return value_string("Error: Failed to resolve hostname");
        }
        memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    }
    
    // Make socket non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    // Connect (non-blocking)
    int result = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (result < 0 && errno != EINPROGRESS) {
        close(sockfd);
        free_url_components(url);
        return value_string("Error: Failed to connect");
    }
    
    // Wait for connection to complete
    if (result < 0) {
        AsyncTask *connect_task = get_current_task();
        IOWatcher *watcher = async_watch_fd(sockfd, EPOLLOUT, 
                                           NULL, http_connect_callback, 
                                           http_error_callback, connect_task);
        
        // Suspend until connection completes
        yield_to_event_loop();
        
        async_unwatch_fd(watcher);
    }
    
    // Build HTTP request
    char *http_request = build_http_request(request, url);
    
    // Send request
    size_t total_sent = 0;
    size_t request_len = strlen(http_request);
    
    while (total_sent < request_len) {
        ssize_t sent = send(sockfd, http_request + total_sent, 
                           request_len - total_sent, MSG_NOSIGNAL);
        
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Wait for socket to be ready for writing
                AsyncTask *send_task = get_current_task();
                IOWatcher *watcher = async_watch_fd(sockfd, EPOLLOUT,
                                                   NULL, http_send_callback,
                                                   http_error_callback, send_task);
                yield_to_event_loop();
                async_unwatch_fd(watcher);
                continue;
            } else {
                close(sockfd);
                free(http_request);
                free_url_components(url);
                return value_string("Error: Failed to send request");
            }
        }
        
        total_sent += sent;
    }
    
    free(http_request);
    
    // Receive response
    char *response = receive_http_response(sockfd);
    
    close(sockfd);
    free_url_components(url);
    
    return value_string(response ? response : "Error: Failed to receive response");
}

// Async file I/O
AsyncTask *async_file_read(const char *filename) {
    FileIORequest *request = malloc(sizeof(FileIORequest));
    request->filename = strdup(filename);
    request->operation = FILE_OP_READ;
    request->data = NULL;
    
    return async_spawn(execute_file_io, request);
}

AsyncTask *async_file_write(const char *filename, const char *data) {
    FileIORequest *request = malloc(sizeof(FileIORequest));
    request->filename = strdup(filename);
    request->operation = FILE_OP_WRITE;
    request->data = strdup(data);
    
    return async_spawn(execute_file_io, request);
}

Value execute_file_io(void *args) {
    FileIORequest *request = (FileIORequest *)args;
    
    if (request->operation == FILE_OP_READ) {
        // Open file for reading
        int fd = open(request->filename, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            return value_string("Error: Failed to open file");
        }
        
        // Read file asynchronously
        char *buffer = malloc(FILE_BUFFER_SIZE);
        size_t total_read = 0;
        
        while (true) {
            ssize_t bytes_read = read(fd, buffer + total_read, 
                                     FILE_BUFFER_SIZE - total_read - 1);
            
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Wait for data to be available
                    AsyncTask *read_task = get_current_task();
                    IOWatcher *watcher = async_watch_fd(fd, EPOLLIN,
                                                       file_read_callback, NULL,
                                                       file_error_callback, read_task);
                    yield_to_event_loop();
                    async_unwatch_fd(watcher);
                    continue;
                } else {
                    close(fd);
                    free(buffer);
                    return value_string("Error: Failed to read file");
                }
            } else if (bytes_read == 0) {
                // End of file
                break;
            }
            
            total_read += bytes_read;
            
            // Expand buffer if needed
            if (total_read >= FILE_BUFFER_SIZE - 1) {
                buffer = realloc(buffer, FILE_BUFFER_SIZE * 2);
            }
        }
        
        buffer[total_read] = '\0';
        close(fd);
        
        Value result = value_string(buffer);
        free(buffer);
        return result;
        
    } else if (request->operation == FILE_OP_WRITE) {
        // Open file for writing
        int fd = open(request->filename, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, 0644);
        if (fd < 0) {
            return value_string("Error: Failed to open file for writing");
        }
        
        // Write data asynchronously
        size_t data_len = strlen(request->data);
        size_t total_written = 0;
        
        while (total_written < data_len) {
            ssize_t bytes_written = write(fd, request->data + total_written,
                                         data_len - total_written);
            
            if (bytes_written < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Wait for socket to be ready for writing
                    AsyncTask *write_task = get_current_task();
                    IOWatcher *watcher = async_watch_fd(fd, EPOLLOUT,
                                                       NULL, file_write_callback,
                                                       file_error_callback, write_task);
                    yield_to_event_loop();
                    async_unwatch_fd(watcher);
                    continue;
                } else {
                    close(fd);
                    return value_string("Error: Failed to write file");
                }
            }
            
            total_written += bytes_written;
        }
        
        close(fd);
        return value_string("File written successfully");
    }
    
    return value_null();
}

// Utility functions
uint64_t get_current_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

uint32_t generate_task_id(void) {
    static uint32_t next_id = 1;
    return __sync_fetch_and_add(&next_id, 1);
}

uint32_t generate_timer_id(void) {
    static uint32_t next_id = 1;
    return __sync_fetch_and_add(&next_id, 1);
}

AsyncTask *get_current_task(void) {
    // Thread-local storage for current task
    static __thread AsyncTask *current_task = NULL;
    return current_task;
}

void set_current_task(AsyncTask *task) {
    static __thread AsyncTask *current_task = NULL;
    current_task = task;
}

void save_task_state(AsyncTask *task) {
    // Save CPU registers and stack pointer
    // This is platform-specific and would need assembly code
    // For now, we'll use a simplified approach
    task->stack_pointer = __builtin_frame_address(0);
}

void restore_task_state(AsyncTask *task) {
    // Restore CPU registers and stack pointer
    // This is platform-specific and would need assembly code
    // For now, we'll use a simplified approach
}

void yield_to_event_loop(void) {
    // Switch to event loop thread
    // This would typically involve context switching
    // For now, we'll use a simplified approach with thread yield
    sched_yield();
}

void throw_async_exception(const char *message) {
    // Throw an exception in the async context
    Exception *ex = malloc(sizeof(Exception));
    ex->type = "AsyncError";
    ex->message = strdup(message);
    ex->stack_trace = capture_stack_trace();
    
    throw_exception_internal(ex);
}

// Cleanup functions
void async_task_cleanup(AsyncTask *task) {
    if (task->error) {
        free((char *)task->error);
    }
    free(task);
}

void async_runtime_cleanup(AsyncRuntime *runtime) {
    runtime->running = false;
    
    // Cleanup tasks
    AsyncTask *task = runtime->ready_queue_head;
    while (task) {
        AsyncTask *next = task->next;
        async_task_cleanup(task);
        task = next;
    }
    
    // Cleanup timers
    free(runtime->timers);
    
    // Cleanup I/O watchers
    for (size_t i = 0; i < runtime->io_watcher_count; i++) {
        free(runtime->io_watchers[i]);
    }
    free(runtime->io_watchers);
    
    close(runtime->epoll_fd);
    pthread_mutex_destroy(&runtime->task_queue_mutex);
    pthread_cond_destroy(&runtime->task_available);
    
    free(runtime);
}

// Main async runtime entry point
void async_run(AsyncFunction main_func, void *args) {
    if (!global_runtime) {
        global_runtime = async_runtime_create();
    }
    
    // Schedule main function
    AsyncTask *main_task = async_spawn(main_func, args);
    
    // Run event loop
    async_runtime_run(global_runtime);
    
    // Cleanup
    async_runtime_cleanup(global_runtime);
    global_runtime = NULL;
}