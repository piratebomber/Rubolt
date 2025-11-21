==============
Runtime System
==============

The Rubolt runtime system provides the foundation for program execution, memory management, module loading, and performance optimization. This document describes the runtime architecture, components, and configuration options.

.. contents:: Table of Contents
   :local:
   :depth: 3

Overview
========

The Rubolt runtime is a high-performance C-based system that provides:

* **Fast Interpreter**: Optimized bytecode execution with type-aware operations
* **JIT Compilation**: Just-in-time compilation to native code for hot paths
* **Memory Management**: Hybrid garbage collection and reference counting
* **Module System**: Dynamic loading and caching of native and Rubolt modules
* **Concurrency**: Async/await and threading support with synchronization
* **Interoperability**: Seamless integration with Python and C libraries

Architecture
============

Core Components
---------------

**Interpreter Engine** (``src/interpreter.c``)
  The main execution engine that processes Rubolt bytecode. Features include:
  
  * Stack-based virtual machine
  * Type-aware instruction dispatch
  * Inline caching for method/property access
  * Exception handling and stack unwinding
  * Debug information tracking

**Parser and AST** (``src/parser.c``, ``src/ast.c``)
  Converts source code to Abstract Syntax Tree and bytecode:
  
  * Recursive descent parser with error recovery
  * AST optimization passes
  * Bytecode generation with constant folding
  * Source location tracking for debugging

**Memory Management** (``gc/``, ``rc/``)
  Hybrid memory management system:
  
  * Mark-sweep garbage collector with generational collection
  * Reference counting for deterministic cleanup
  * Type-aware object traversal
  * Memory pools for allocation optimization

**JIT Compiler** (``src/jit_compiler.c``)
  Just-in-time compilation to native code:
  
  * Hot path detection with execution counters
  * x86-64 code generation
  * Optimization passes (dead code elimination, constant folding)
  * Deoptimization support for dynamic type changes

**Module System** (``src/module.c``)
  Dynamic module loading and management:
  
  * Native C module registration
  * Rubolt module compilation and caching
  * Dependency resolution
  * Import path management

Runtime Initialization
======================

Startup Sequence
----------------

1. **Environment Setup**
   
   * Parse command-line arguments
   * Load configuration files
   * Initialize global state
   * Set up signal handlers

2. **Memory System Initialization**
   
   * Initialize garbage collector
   * Set up memory pools
   * Configure reference counting
   * Enable memory debugging if requested

3. **Module Registry Setup**
   
   * Register built-in modules (file, json, http, etc.)
   * Scan for native extensions
   * Load prelude and standard library
   * Set up import paths

4. **JIT System Initialization**
   
   * Allocate executable memory regions
   * Initialize code cache
   * Set up profiling counters
   * Configure optimization thresholds

5. **Program Execution**
   
   * Parse and compile source code
   * Execute main function or REPL
   * Handle runtime events and signals
   * Perform cleanup on exit

Configuration
=============

Runtime configuration is controlled through multiple sources:

Project Configuration (.rbo.config)
------------------------------------

.. code-block:: json

   {
     "runtime": {
       "jit": {
         "enabled": true,
         "optimization_level": 2,
         "hot_threshold": 10,
         "max_code_cache": "64MB"
       },
       "memory": {
         "gc_enabled": true,
         "gc_mode": "generational",
         "gc_threshold": 1000000,
         "initial_heap": "16MB",
         "max_heap": "1GB",
         "debug_memory": false
       },
       "modules": {
         "cache_enabled": true,
         "cache_dir": ".rubolt_cache",
         "preload": ["prelude", "core"]
       },
       "concurrency": {
         "max_threads": 8,
         "async_enabled": true,
         "event_loop_threads": 2
       }
     }
   }

Environment Variables
---------------------

``RUBOLT_JIT``
  Control JIT compilation:
  
  * ``on`` - Always enable JIT
  * ``off`` - Disable JIT compilation
  * ``auto`` - Enable based on optimization level (default)

``RUBOLT_GC``
  Garbage collection mode:
  
  * ``mark-sweep`` - Traditional mark-sweep collector
  * ``generational`` - Generational garbage collector (default)
  * ``reference-counting`` - Reference counting only
  * ``hybrid`` - Combination of GC and RC

``RUBOLT_MEMORY_DEBUG``
  Enable memory debugging:
  
  * ``1`` - Enable leak detection
  * ``2`` - Enable allocation tracking
  * ``3`` - Enable full memory debugging

``RUBOLT_PROFILE``
  Enable runtime profiling:
  
  * ``functions`` - Profile function calls
  * ``memory`` - Profile memory usage
  * ``jit`` - Profile JIT compilation
  * ``all`` - Enable all profiling

Memory Management
=================

Garbage Collection
------------------

The Rubolt runtime uses a sophisticated garbage collection system:

**Generational Collection:**

.. code-block:: c

   // Young generation (frequent, fast collection)
   typedef struct YoungGeneration {
       void *eden_space;
       void *survivor_space_0;
       void *survivor_space_1;
       size_t size;
   } YoungGeneration;

   // Old generation (infrequent, thorough collection)
   typedef struct OldGeneration {
       void *space;
       size_t size;
       size_t used;
   } OldGeneration;

**Collection Triggers:**

* Allocation threshold reached
* Explicit ``gc.collect()`` call
* Memory pressure from OS
* Before JIT compilation of large functions

**Optimization Features:**

* **Incremental collection** - Spread work across multiple cycles
* **Concurrent marking** - Mark phase runs in background thread
* **Write barriers** - Track inter-generational references
* **Card marking** - Optimize scanning of large objects

Reference Counting
------------------

Used for deterministic cleanup of resources:

.. code-block:: c

   typedef struct RCObject {
       size_t ref_count;
       void (*destructor)(struct RCObject *);
       // Object data follows
   } RCObject;

   // Increment reference count
   void rc_retain(RCObject *obj) {
       if (obj) {
           atomic_fetch_add(&obj->ref_count, 1);
       }
   }

   // Decrement and potentially free
   void rc_release(RCObject *obj) {
       if (obj && atomic_fetch_sub(&obj->ref_count, 1) == 1) {
           obj->destructor(obj);
           free(obj);
       }
   }

**Cycle Detection:**

The runtime includes cycle detection for reference-counted objects:

.. code-block:: c

   // Periodic cycle detection
   void rc_detect_cycles(void) {
       // Mark all objects as white
       mark_all_white();
       
       // Mark reachable objects as black
       mark_from_roots();
       
       // Collect white objects (unreachable cycles)
       collect_white_objects();
   }

JIT Compilation
===============

Hot Path Detection
------------------

The runtime tracks function execution to identify hot paths:

.. code-block:: c

   typedef struct FunctionProfile {
       size_t call_count;
       size_t total_time;
       bool is_hot;
       bool is_compiled;
       void *native_code;
   } FunctionProfile;

   // Called on each function entry
   void profile_function_entry(Function *func) {
       func->profile.call_count++;
       
       if (func->profile.call_count >= JIT_HOT_THRESHOLD && 
           !func->profile.is_compiled) {
           schedule_jit_compilation(func);
       }
   }

Code Generation
---------------

The JIT compiler generates optimized x86-64 machine code:

**Optimization Passes:**

1. **Dead Code Elimination** - Remove unreachable code
2. **Constant Folding** - Evaluate constant expressions at compile time
3. **Loop Unrolling** - Unroll small loops for better performance
4. **Inlining** - Inline small functions to reduce call overhead
5. **Register Allocation** - Optimize register usage

**Example Generated Code:**

.. code-block:: asm

   ; Original Rubolt: def add(a: number, b: number) -> number { return a + b; }
   ; Compiled to x86-64:
   
   add_jit:
       movq    %rdi, %rax      ; Load first parameter
       addq    %rsi, %rax      ; Add second parameter
       ret                     ; Return result

Inline Caching
---------------

Method and property access is optimized using inline caches:

.. code-block:: c

   typedef struct InlineCache {
       void *cached_class;     // Last seen class
       size_t cached_offset;   // Property offset
       void *cached_method;    // Method pointer
       size_t hit_count;       // Cache hit statistics
       size_t miss_count;      // Cache miss statistics
   } InlineCache;

   // Optimized property access
   Value get_property_cached(Object *obj, const char *name, InlineCache *cache) {
       if (obj->class == cache->cached_class) {
           // Cache hit - fast path
           return *(Value *)((char *)obj + cache->cached_offset);
       } else {
           // Cache miss - slow path, update cache
           size_t offset = find_property_offset(obj->class, name);
           cache->cached_class = obj->class;
           cache->cached_offset = offset;
           cache->miss_count++;
           return *(Value *)((char *)obj + offset);
       }
   }

Module System
=============

Module Loading
--------------

The runtime supports multiple module types:

**Native Modules (C):**

.. code-block:: c

   // Module registration
   void register_file_module(ModuleRegistry *registry) {
       Module *mod = module_create("file");
       module_add_function(mod, "read", file_read);
       module_add_function(mod, "write", file_write);
       module_registry_add(registry, mod);
   }

**Rubolt Modules:**

.. code-block:: rubolt

   // math_utils.rbo
   export def factorial(n: number) -> number {
       if (n <= 1) return 1;
       return n * factorial(n - 1);
   }

**DLL/Shared Library Modules:**

.. code-block:: rubolt

   // Automatic compilation and loading
   import mymath.dll  // Compiles mymath.c if needed

Module Caching
--------------

Compiled modules are cached for faster loading:

.. code-block:: text

   .rubolt_cache/
   ├── modules/
   │   ├── math_utils.rbc      # Compiled bytecode
   │   ├── http_client.rbc
   │   └── json_parser.rbc
   ├── native/
   │   ├── mymath.dll          # Compiled native libraries
   │   └── graphics.so
   └── metadata/
       ├── dependencies.json   # Dependency graph
       └── timestamps.json     # Modification times

Import Resolution
-----------------

The runtime searches for modules in this order:

1. **Current directory**
2. **Project-specific paths** (from ``.rbo.config``)
3. **Standard library** (``StdLib/``)
4. **Global module directory** (``~/.rubolt/modules/``)
5. **System-wide installation** (``/usr/local/lib/rubolt/``)

Concurrency Support
===================

Async/Await Implementation
--------------------------

The runtime provides cooperative multitasking through async/await:

.. code-block:: c

   typedef struct AsyncTask {
       enum TaskState state;   // Running, Suspended, Completed
       void *stack_pointer;    // Saved stack state
       void *continuation;     // Resume point
       Value result;           // Task result
       struct AsyncTask *next; // Task queue linkage
   } AsyncTask;

   // Event loop implementation
   void event_loop_run(EventLoop *loop) {
       while (loop->has_tasks) {
           AsyncTask *task = dequeue_ready_task(loop);
           if (task) {
               resume_task(task);
               if (task->state == TASK_COMPLETED) {
                   complete_task(task);
               }
           }
           
           process_io_events(loop);
           process_timers(loop);
       }
   }

Threading Support
-----------------

The runtime includes a threading system with GIL-like synchronization:

.. code-block:: c

   typedef struct ThreadState {
       pthread_t thread_id;
       bool gil_held;
       size_t instruction_count;
       struct ThreadState *next;
   } ThreadState;

   // Global Interpreter Lock
   static pthread_mutex_t gil_mutex = PTHREAD_MUTEX_INITIALIZER;
   static ThreadState *current_thread = NULL;

   // Acquire GIL before executing Rubolt code
   void acquire_gil(ThreadState *thread) {
       pthread_mutex_lock(&gil_mutex);
       current_thread = thread;
       thread->gil_held = true;
   }

   // Release GIL periodically to allow other threads
   void check_gil_release(ThreadState *thread) {
       if (++thread->instruction_count >= GIL_CHECK_INTERVAL) {
           thread->instruction_count = 0;
           if (other_threads_waiting()) {
               release_gil(thread);
               acquire_gil(thread);  // Re-acquire
           }
       }
   }

Error Handling
==============

Exception System
----------------

The runtime provides structured exception handling:

.. code-block:: c

   typedef struct Exception {
       const char *type;       // Exception type name
       const char *message;    // Error message
       StackTrace *stack_trace; // Call stack
       struct Exception *cause; // Chained exception
   } Exception;

   // Exception handling context
   typedef struct ExceptionHandler {
       jmp_buf jump_buffer;    // setjmp/longjmp context
       Exception *current_exception;
       struct ExceptionHandler *previous;
   } ExceptionHandler;

   // Throw exception
   void throw_exception(const char *type, const char *message) {
       Exception *ex = create_exception(type, message);
       ex->stack_trace = capture_stack_trace();
       
       ExceptionHandler *handler = get_current_handler();
       if (handler) {
           handler->current_exception = ex;
           longjmp(handler->jump_buffer, 1);
       } else {
           // Unhandled exception - terminate program
           print_exception(ex);
           exit(1);
       }
   }

Stack Trace Capture
-------------------

The runtime maintains detailed stack traces for debugging:

.. code-block:: c

   typedef struct StackFrame {
       const char *function_name;
       const char *file_name;
       size_t line_number;
       size_t column_number;
       struct StackFrame *next;
   } StackFrame;

   StackTrace *capture_stack_trace(void) {
       StackTrace *trace = malloc(sizeof(StackTrace));
       trace->frames = NULL;
       
       // Walk the call stack
       void **frame_pointer = __builtin_frame_address(0);
       while (frame_pointer && trace->depth < MAX_STACK_DEPTH) {
           void *return_address = frame_pointer[1];
           DebugInfo *debug = find_debug_info(return_address);
           
           if (debug) {
               StackFrame *frame = create_stack_frame(debug);
               frame->next = trace->frames;
               trace->frames = frame;
               trace->depth++;
           }
           
           frame_pointer = (void **)frame_pointer[0];
       }
       
       return trace;
   }

Performance Monitoring
======================

Profiling Infrastructure
------------------------

The runtime includes comprehensive profiling capabilities:

.. code-block:: c

   typedef struct ProfileData {
       // Function profiling
       size_t function_calls;
       uint64_t function_time;
       
       // Memory profiling
       size_t allocations;
       size_t deallocations;
       size_t peak_memory;
       
       // JIT profiling
       size_t compilations;
       uint64_t compile_time;
       
       // GC profiling
       size_t gc_collections;
       uint64_t gc_time;
   } ProfileData;

   // Profile function entry/exit
   void profile_function_enter(const char *name) {
       ProfileData *data = get_profile_data(name);
       data->function_calls++;
       data->enter_time = get_high_resolution_time();
   }

   void profile_function_exit(const char *name) {
       ProfileData *data = get_profile_data(name);
       uint64_t exit_time = get_high_resolution_time();
       data->function_time += exit_time - data->enter_time;
   }

Performance Counters
--------------------

The runtime tracks various performance metrics:

.. code-block:: c

   typedef struct PerformanceCounters {
       // Execution statistics
       uint64_t instructions_executed;
       uint64_t function_calls;
       uint64_t method_lookups;
       
       // Memory statistics
       uint64_t objects_allocated;
       uint64_t bytes_allocated;
       uint64_t gc_collections;
       
       // JIT statistics
       uint64_t functions_compiled;
       uint64_t native_calls;
       uint64_t deoptimizations;
       
       // Cache statistics
       uint64_t inline_cache_hits;
       uint64_t inline_cache_misses;
   } PerformanceCounters;

Debugging Support
=================

Debug Information
-----------------

The runtime maintains debug information for source-level debugging:

.. code-block:: c

   typedef struct DebugInfo {
       const char *source_file;
       size_t line_number;
       size_t column_number;
       const char *function_name;
       LocalVariable *locals;
       size_t local_count;
   } DebugInfo;

   typedef struct LocalVariable {
       const char *name;
       Value *value_ptr;
       const char *type_name;
   } LocalVariable;

Breakpoint System
-----------------

The runtime supports breakpoints for debugging:

.. code-block:: c

   typedef struct Breakpoint {
       const char *file_name;
       size_t line_number;
       bool enabled;
       const char *condition;  // Optional condition
       size_t hit_count;
       struct Breakpoint *next;
   } Breakpoint;

   // Check for breakpoint during execution
   void check_breakpoint(const char *file, size_t line) {
       Breakpoint *bp = find_breakpoint(file, line);
       if (bp && bp->enabled) {
           if (!bp->condition || evaluate_condition(bp->condition)) {
               bp->hit_count++;
               enter_debugger(file, line);
           }
       }
   }

Runtime API
===========

The runtime provides a C API for embedding and extending Rubolt:

Embedding API
-------------

.. code-block:: c

   // Initialize runtime
   RuboltRuntime *rubolt_init(RuboltConfig *config);
   
   // Execute code
   RuboltResult rubolt_execute_string(RuboltRuntime *rt, const char *code);
   RuboltResult rubolt_execute_file(RuboltRuntime *rt, const char *filename);
   
   // Call functions
   RuboltValue rubolt_call_function(RuboltRuntime *rt, const char *name, 
                                    RuboltValue *args, size_t arg_count);
   
   // Manage variables
   void rubolt_set_global(RuboltRuntime *rt, const char *name, RuboltValue value);
   RuboltValue rubolt_get_global(RuboltRuntime *rt, const char *name);
   
   // Cleanup
   void rubolt_cleanup(RuboltRuntime *rt);

Extension API
-------------

.. code-block:: c

   // Register native function
   void rubolt_register_function(RuboltRuntime *rt, const char *name,
                                 RuboltNativeFunction func);
   
   // Register native class
   void rubolt_register_class(RuboltRuntime *rt, const char *name,
                              RuboltClassDef *class_def);
   
   // Create native objects
   RuboltValue rubolt_create_object(RuboltRuntime *rt, const char *class_name);
   
   // Type conversion
   RuboltValue rubolt_make_number(double value);
   RuboltValue rubolt_make_string(const char *str);
   double rubolt_get_number(RuboltValue value);
   const char *rubolt_get_string(RuboltValue value);

Configuration and Tuning
=========================

Performance Tuning
-------------------

**JIT Optimization:**

.. code-block:: json

   {
     "jit": {
       "hot_threshold": 5,        // Lower for faster compilation
       "optimization_level": 3,   // Higher for better optimization
       "inline_threshold": 50,    // Inline functions under 50 instructions
       "unroll_loops": true,      // Enable loop unrolling
       "eliminate_bounds_checks": true
     }
   }

**Memory Optimization:**

.. code-block:: json

   {
     "memory": {
       "initial_heap": "32MB",    // Larger initial heap
       "gc_threshold": 2000000,   // Less frequent GC
       "young_gen_size": "8MB",   // Larger young generation
       "enable_compaction": true  // Reduce fragmentation
     }
   }

**Concurrency Tuning:**

.. code-block:: json

   {
     "concurrency": {
       "max_threads": 16,         // More threads for CPU-bound work
       "gil_check_interval": 100, // More frequent GIL checks
       "async_io_threads": 4      // Dedicated I/O threads
     }
   }

Troubleshooting
===============

Common Issues
-------------

**Memory Leaks:**

.. code-block:: bash

   # Enable memory debugging
   export RUBOLT_MEMORY_DEBUG=3
   rbcli run my_program.rbo
   
   # Check for leaks at exit
   # Runtime will print leak report

**Performance Issues:**

.. code-block:: bash

   # Profile execution
   export RUBOLT_PROFILE=all
   rbcli run my_program.rbo
   
   # Check JIT compilation
   export RUBOLT_JIT_VERBOSE=1
   rbcli run my_program.rbo

**Debugging Crashes:**

.. code-block:: bash

   # Enable core dumps
   ulimit -c unlimited
   
   # Run with debugging
   gdb rbcli
   (gdb) run my_program.rbo
   (gdb) bt  # Get stack trace on crash

Runtime Statistics
------------------

The runtime can output detailed statistics:

.. code-block:: bash

   # Enable statistics
   export RUBOLT_STATS=1
   rbcli run my_program.rbo

Example output:

.. code-block:: text

   Rubolt Runtime Statistics:
   ==========================
   
   Execution:
     Instructions executed: 1,234,567
     Function calls: 45,678
     Method lookups: 23,456
   
   Memory:
     Objects allocated: 12,345
     Peak memory usage: 64 MB
     GC collections: 23
     Average GC time: 2.3 ms
   
   JIT Compilation:
     Functions compiled: 15
     Compilation time: 45 ms
     Native execution: 78% of total time
   
   Cache Performance:
     Inline cache hit rate: 94.2%
     Method cache hit rate: 89.7%

The Rubolt runtime is designed for high performance and flexibility, providing the foundation for fast, reliable Rubolt program execution. For more detailed information about specific runtime components, refer to the source code documentation and developer guides.