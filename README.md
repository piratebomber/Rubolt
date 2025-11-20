# Rubolt Programming Language

Rubolt is a modern, high-performance programming language that combines the best features of Python, C, and TypeScript. It features advanced pattern matching, generics, comprehensive error handling, JIT compilation, and a rich ecosystem of tools and libraries.

## Core Language Features

### Advanced Type System
- **Static typing** with type inference and helpful error messages
- **Generics** with type constraints, variance, and automatic instantiation
- **Pattern matching** with guards, destructuring, and type patterns
- **Union types** and algebraic data types
- **Null safety** with optional types and non-null constraints

### Performance & Runtime
- **Fast C interpreter/runtime** with optimized execution
- **JIT compilation** with hot path detection and native code generation
- **Inline caching** for method/attribute lookups with performance stats
- **Memory management** with both GC (mark-sweep) and RC (reference counting)
- **Type-aware object graph traversal** for efficient garbage collection

### Concurrency & Async
- **Async/await** with event loop and timer support
- **Threading** with GIL-like synchronization and thread pools
- **Atomic operations** for lock-free programming
- **Event-driven I/O** with non-blocking operations

### Interoperability
- **DLL/Shared library import**: `import mylib.dll` with automatic compilation
- **Python interop**: gcc→py compiler, py→C bridge, FFI helpers
- **C extension API** with native module development kit
- **Foreign Function Interface** with automatic wrapper generation

## Quick Start

### Installation

**Automated Installer (Windows):**
```powershell
pwsh -File Installer/install.ps1 -Mode auto -Prefix "$env:USERPROFILE\Rubolt" -AddToPath
```

**Automated Installer (macOS/Linux):**
```bash
bash Installer/install.sh --mode auto --prefix /usr/local --add-to-path
```

**Build from Source:**
```bash
# Windows
build_all.bat

# macOS/Linux
chmod +x build_all.sh
./build_all.sh
```

**Docker:**
```bash
docker build -t rubolt .
docker run --rm -it -v $PWD:/work rubolt rbcli run examples/hello.rbo
```

**VS Code Devcontainer:**
Open the repository in VS Code and select "Reopen in Container".

### Basic Usage

```bash
# Run a Rubolt file
rbcli run examples/hello.rbo

# Start interactive REPL
rbcli repl

# Create new project
rbcli init my-project

# Build project
rbcli build

# Run tests
rbcli test

# Start language server
rbcli lsp
```

## Language Features

### Pattern Matching

Comprehensive pattern matching with guards, destructuring, and type constraints:

```rubolt
def process_value(value: any) -> string {
    match value {
        null => "nothing";
        true => "yes";
        false => "no";
        42 => "the answer";
        x if x > 100 => "big number: " + x;
        (x, y) => "tuple: " + x + ", " + y;
        [head, ...tail] => "list starting with " + head;
        {name, age} => "person: " + name + " (" + age + ")";
        String => "it's a string";
        _ => "something else";
    }
}
```

### Generics System

Full generic programming with type parameters and constraints:

```rubolt
// Generic function with constraints
def compare<T: Comparable>(a: T, b: T) -> number {
    return a.compare(b);
}

// Generic class
class Optional<T> {
    value: T;
    has_value: bool;
    
    def map<U>(func: function(T) -> U) -> Optional<U> {
        if (this.has_value) {
            return Optional<U>(func(this.value));
        }
        return Optional<U>.none();
    }
}
```

### Error Handling

Comprehensive error handling with Result types and try/catch:

```rubolt
type Result<T, E> = Ok<T> | Error<E>;

def safe_divide(a: number, b: number) -> Result<number, string> {
    if (b == 0) {
        return Error("Division by zero");
    }
    return Ok(a / b);
}

// Try/catch with multiple error types
try {
    let content = file.read(filename);
    let data = json.parse(content);
} catch (FileNotFoundError e) {
    print("File not found: " + e.message);
} catch (JsonParseError e) {
    print("Invalid JSON: " + e.message);
}
```

### JIT Compilation

Automatic just-in-time compilation for performance-critical code:

```rubolt
@jit_compile
def matrix_multiply(a: number[][], b: number[][]) -> number[][] {
    // This function will be compiled to native code
    let result = [];
    for (i in 0..a.length) {
        result[i] = [];
        for (j in 0..b[0].length) {
            let sum = 0;
            for (k in 0..b.length) {
                sum += a[i][k] * b[k][j];
            }
            result[i][j] = sum;
        }
    }
    return result;
}
```

## Standard Library

### File I/O Module

```rubolt
import file

// Basic operations
let content = file.read("data.txt");
file.write("output.txt", "Hello, World!");
file.append("log.txt", "New entry\n");

// File metadata
if (file.exists("config.json")) {
    let size = file.size("config.json");
    print("Config file size: " + size + " bytes");
}

// Advanced operations
file.copy("source.txt", "backup.txt");
let lines = file.readlines("data.csv");
```

### JSON Module

```rubolt
import json

// Parse and stringify
let data = json.parse('{"name": "John", "age": 30}');
let json_str = json.stringify({name: "Alice", hobbies: ["reading", "coding"]});
```

### HTTP Module

```rubolt
import http

// REST API calls
let response = http.get("https://api.github.com/users/octocat");
let result = http.post("https://api.example.com/users", 
                      json.stringify(user_data), 
                      "application/json");
```

### Time Module

```rubolt
import time

// Time operations
let now = time.now();
let formatted = time.format(now, "%Y-%m-%d %H:%M:%S");
time.sleep(2.0);

// Date components
print("Year: " + time.year(now));
print("Month: " + time.month(now));
```

## DLL/Native Library Support

Seamless integration with native libraries:

```rubolt
// Import DLL (auto-compiled if source available)
import mymath.dll
print(mymath.add(2, 40));

// Import from precompiled directory
import "src/precompiled/graphics.dll" as gfx
gfx.init_window(800, 600);
```

Place DLLs in `src/precompiled/` or let Rubolt compile from source automatically.

## Python Interoperability

Comprehensive Python integration with multiple tools:

### GCC-to-Python Compiler
```bash
python python/gimmicks/gcc_to_py.py input.c -o output.py
```

### C Wrapper Generator
```bash
python python/gimmicks/c_wrapper_gen.py mylib.so -o mylib_wrapper.py
```

### Python-C Bridge
```python
from python.gimmicks.py_to_c_bridge import call_c_function
result = call_c_function("my_native_func", [arg1, arg2])
```

### Type Conversion
```python
from python.gimmicks.type_converter import py_to_c, c_to_py
c_val = py_to_c(42, "int")
py_val = c_to_py(c_val, "int")
```

## Collections & Data Structures

### Lists
```rubolt
let numbers = [1, 2, 3, 4, 5];
numbers.append(6);
numbers.insert(0, 0);
let item = numbers.pop();
numbers.remove(2);
let slice = numbers[1:4];
numbers.sort();
numbers.reverse();
```

### Dictionaries
```rubolt
let person = {"name": "Alice", "age": 30};
person["email"] = "alice@example.com";
let keys = person.keys();
let values = person.values();
for (key, value in person.items()) {
    print(key + ": " + value);
}
```

### Sets
```rubolt
let unique_numbers = {1, 2, 3, 4, 5};
unique_numbers.add(6);
unique_numbers.remove(1);
let intersection = set1.intersection(set2);
let union = set1.union(set2);
```

### Tuples
```rubolt
let coordinates = (10, 20);
let (x, y) = coordinates; // Destructuring
let person_info = ("Alice", 30, "Engineer");
```

## Concurrency & Async Programming

### Async/Await
```rubolt
import async

async def fetch_data(url: string) -> string {
    let response = await http.get_async(url);
    return response;
}

async def main() {
    let tasks = [
        fetch_data("https://api1.example.com"),
        fetch_data("https://api2.example.com"),
        fetch_data("https://api3.example.com")
    ];
    
    let results = await Promise.all(tasks);
    for (result in results) {
        print("Received: " + result);
    }
}

// Run async function
async.run(main());
```

### Threading
```rubolt
import threading

def worker_function(data: any) -> any {
    // CPU-intensive work
    return process_data(data);
}

// Thread pool
let pool = threading.ThreadPool(4);
let futures = [];

for (item in large_dataset) {
    let future = pool.submit(worker_function, item);
    futures.append(future);
}

// Collect results
let results = [];
for (future in futures) {
    results.append(future.get());
}

pool.shutdown();
```

### Atomic Operations
```rubolt
import atomics

let counter = atomics.AtomicInt(0);

def increment_counter() {
    counter.fetch_add(1);
}

// Safe concurrent access
let threads = [];
for (i in 0..10) {
    threads.append(threading.Thread(increment_counter));
}

for (thread in threads) {
    thread.start();
}

for (thread in threads) {
    thread.join();
}

print("Final counter value: " + counter.load());
```

## Memory Management

### Garbage Collection
- **Mark-sweep collector** with pool optimization
- **Type-aware traversal** using runtime type information
- **Generational collection** for improved performance
- **Incremental collection** to reduce pause times

### Reference Counting
- **Strong references** for ownership
- **Weak references** to break cycles
- **Automatic cycle detection** and cleanup
- **Deterministic destruction** for resources

### Memory Debugging
```rubolt
// Enable memory debugging
enable_memory_debugging();

// Your code here
let data = create_large_structure();

// Check for leaks
print_memory_leaks();
disable_memory_debugging();
```

### Manual Memory Control
```rubolt
@no_gc
def performance_critical() -> void {
    // Disable GC for this function
    let buffer = allocate_buffer(1024 * 1024);
    // ... performance critical code ...
    free_buffer(buffer);
}
```

## Development Tools

### REPL (Read-Eval-Print Loop)
```bash
rbcli repl
```

Features:
- **Command history** with persistent storage
- **Tab completion** for keywords, functions, and modules
- **Multi-line input** support
- **Syntax highlighting** in terminal
- **Variable inspection** and help system

### Debugger
```rubolt
// Set breakpoints in code
breakpoint();

// Or use debugger commands
// (rbdb) break main.rbo:25
// (rbdb) step
// (rbdb) next
// (rbdb) print variable_name
// (rbdb) backtrace
// (rbdb) continue
```

Features:
- **Breakpoints** with conditional expressions
- **Step debugging** (step into, step over, step out)
- **Call stack inspection** with local variables
- **Variable watching** and modification
- **Post-mortem debugging** for crashes

### Profiler
```rubolt
import profiler

profiler.start("my_operation");
// ... code to profile ...
profiler.stop("my_operation");

// Generate report
profiler.report();
profiler.save_report("profile.json");
```

Features:
- **Function-level timing** with call counts
- **Memory allocation tracking**
- **Hot path identification** for JIT optimization
- **Flame graph generation**
- **Performance regression detection**

### Code Formatter
```bash
# Format single file
rbformat src/main.rbo

# Format entire project
rbformat src/**/*.rbo --write

# Custom formatting options
rbformat --indent-size 2 --max-line-length 120 src/
```

### Linter
```bash
# Lint files
rblint src/main.rbo

# Custom rules
rblint --max-complexity 15 --strict-types src/

# Configuration file
rblint --config .rblint.json src/
```

Lint rules include:
- Naming conventions
- Missing type annotations
- Unused variables and imports
- Dead code detection
- Cyclomatic complexity
- Code style consistency

## JIT Compilation & Optimization

### Just-In-Time Compilation
- **Hot path detection** with execution counters
- **Native code generation** for x86-64 architecture
- **Optimization passes**: dead code elimination, constant folding, loop unrolling
- **Deoptimization** support for dynamic type changes

### Inline Caching
- **Method lookup caching** with polymorphic inline caches
- **Property access optimization** with hidden class transitions
- **Call site specialization** based on argument types
- **Performance statistics** and optimization hints

### Optimization Annotations
```rubolt
@jit_compile
def compute_intensive_function(data: number[]) -> number {
    // Force JIT compilation
    let sum = 0;
    for (item in data) {
        sum += item * item;
    }
    return sum;
}

@hot_path
def frequently_called_function() {
    // Mark as hot path for optimization
}

@inline_always
def small_utility_function(x: number) -> number {
    return x * 2 + 1;
}
```

## Testing Framework

Comprehensive testing with unit tests, property-based testing, and benchmarks:

### Unit Tests
```rubolt
import test

test.suite("Math Operations", () => {
    test.case("Addition", () => {
        assert.equal(2 + 2, 4, "Basic addition");
        assert.equal(0 + 5, 5, "Addition with zero");
    });
    
    test.case("Division", () => {
        assert.equal(10 / 2, 5, "Basic division");
        assert.throws(() => 1 / 0, "DivisionByZeroError");
    });
});
```

### Property-Based Testing
```rubolt
test.property("Addition is commutative", 
    test.generate.int(-1000, 1000), 
    test.generate.int(-1000, 1000),
    (a, b) => a + b == b + a
);
```

### Benchmarks
```rubolt
test.benchmark("Fibonacci", () => {
    fibonacci(30);
}, {iterations: 100, warmup: 10});
```

### Mocking
```rubolt
let mock_api = test.mock();
mock_api.expect("get_user", [123], {name: "John", id: 123});

let result = mock_api.get_user(123);
assert.equal(result.name, "John");
assert.verify(mock_api);
```

## Package Management

### Project Configuration (package.json)
```json
{
    "name": "my-project",
    "version": "1.0.0",
    "dependencies": {
        "http-client": "^2.1.0",
        "json-parser": "~1.5.2"
    },
    "devDependencies": {
        "test-framework": "^3.0.0"
    }
}
```

### Package Commands
```bash
rbcli init my-project          # Create new project
rbcli add http-client@2.1.0    # Add dependency
rbcli remove old-package       # Remove dependency
rbcli install                  # Install all dependencies
rbcli update                   # Update dependencies
rbcli publish                  # Publish to registry
```

## CLI Reference

### Core Commands
```bash
rbcli init <name>         # Create new project
rbcli run <file>          # Run Rubolt file
rbcli repl                # Interactive REPL
rbcli build               # Build project
rbcli test                # Run test suite
rbcli compile <file>      # Compile to bytecode
rbcli version             # Show version info
```

### Development Commands
```bash
rbcli newlib <name>       # Create library template
rbcli format <files>      # Format source code
rbcli lint <files>        # Lint source code
rbcli lsp                 # Start language server
rbcli doc <files>         # Generate documentation
```

### Advanced Commands
```bash
rbcli sim <file>          # Run in Bopes simulator
rbcli profile <file>      # Profile execution
rbcli debug <file>        # Start debugger
rbcli benchmark <file>    # Run benchmarks
rbcli analyze <project>   # Analyze project dependencies
```

## Configuration

### Project Configuration (.rbo.config)
```json
{
  "version": "1.0",
  "entry": "src/main.rbo",
  "strict": true,
  "typecheck": true,
  "optimize": true,
  "output": "build/",
  "jit": {
    "enabled": true,
    "optimization_level": 2,
    "hot_threshold": 10
  },
  "memory": {
    "gc_enabled": true,
    "gc_threshold": 1000000,
    "debug_memory": false
  },
  "linting": {
    "max_line_length": 100,
    "max_complexity": 15,
    "enforce_types": true
  }
}
```

### Linter Configuration (.rblint.json)
```json
{
  "rules": {
    "naming_convention": "snake_case",
    "max_line_length": 100,
    "max_function_length": 50,
    "max_complexity": 15,
    "require_type_annotations": true,
    "no_unused_variables": true,
    "no_dead_code": true
  },
  "ignore": ["build/", "vendor/", "*.generated.rbo"]
}
```

### Formatter Configuration (.rbformat.json)
```json
{
  "indent_size": 4,
  "indent_type": "spaces",
  "max_line_length": 100,
  "brace_style": "1tbs",
  "space_before_function_paren": false,
  "align_parameters": true
}
```

## Development Environment

### VS Code Integration

**Extension Features:**
- **Syntax highlighting** for `.rbo` files
- **Code completion** with IntelliSense
- **Error diagnostics** with real-time validation
- **Debugging support** with breakpoints and variable inspection
- **Integrated terminal** with Rubolt REPL
- **Code formatting** and linting
- **Go to definition** and find references
- **Hover documentation** for functions and types

**Installation:**
```bash
cd vscode-rubolt
npm install
npm run compile
vsce package
# Install the generated .vsix file in VS Code
```

### Language Server Protocol (LSP)

Full LSP implementation for IDE integration:

```bash
# Start language server
rbcli lsp

# Or build manually
cd tools
gcc -Wall -Wextra -std=c99 -O2 -I../src rubolt_lsp.c -o rubolt-lsp -ljson-c
```

**LSP Features:**
- Code completion with context awareness
- Real-time error diagnostics
- Hover information with type details
- Go to definition and find references
- Document formatting and range formatting
- Signature help for function parameters

### Docker & Containers

**Dockerfile for reproducible builds:**
```bash
docker build -t rubolt .
docker run --rm -it -v $PWD:/work rubolt
```

**VS Code Devcontainer:**
```json
{
  "name": "Rubolt Development",
  "dockerFile": "Dockerfile",
  "extensions": [
    "rubolt.rubolt-vscode"
  ],
  "settings": {
    "terminal.integrated.shell.linux": "/bin/bash"
  }
}
```

### CI/CD Workflows

**GitHub Actions:**
- **Build workflow**: Multi-platform builds (Windows, macOS, Linux)
- **Test workflow**: Unit tests, integration tests, property-based tests
- **Lint workflow**: Code quality checks and style validation
- **Security scan**: Static analysis and vulnerability detection
- **Documentation**: Auto-generated docs and API reference
- **Fuzz testing**: Automated fuzzing for robustness
- **Performance**: Benchmark tracking and regression detection

### Build Scripts

**Cross-platform build automation:**
```bash
# Complete build (all components)
build_complete.bat    # Windows
build_complete.sh     # Unix/Linux/macOS

# Standard library only
build_stdlib.bat      # Windows
build_stdlib.sh       # Unix/Linux/macOS

# Clean build
scripts/clean.sh

# Coverage report
scripts/coverage.sh
```

## Advanced Features

### Bopes Virtual Machine

Sandboxed execution environment for testing and simulation:

```bash
# Run in Bopes simulator
rbcli sim examples/hello.rbo
```

**Features:**
- **Virtual memory** with configurable size
- **Instruction tracing** for debugging
- **Sandboxed execution** for security
- **Performance profiling** in isolated environment

### Vendor System

Automated dependency management for C libraries and tools:

```bash
# Analyze project dependencies
python vendor/analyze_project.py

# Install dependencies (Windows)
pwsh -File vendor/install.ps1 -Analyze -Prefix vendor

# Install dependencies (Unix/Linux/macOS)
bash vendor/install.sh --analyze --prefix vendor
```

**Features:**
- **Automatic detection** of required headers and libraries
- **Cross-platform installers** for Windows, macOS, and Linux
- **Registry system** for package management
- **Build integration** with automatic compilation

### Compile-time Macros

Preprocessor macros for code generation and debugging:

```c
// Type helper macros
#include "compiletime/macros/type_macros.h"
TYPE_ASSERT(int, 4);  // Assert int is 4 bytes

// Symbol mapping
#include "compiletime/macros/symbol_map.h"
SYMBOL_MAP(my_function, optimized_my_function);

// Find and replace macros
#include "compiletime/macros/find_replace_macros.h"
REPLACE_TOKEN(DEBUG_PRINT, printf);
```

### Source Preparation Tools

Bash scripts for .rbo file processing:

```bash
# Prepare .rbo file for execution
bash src/prep/prepare_rbo.sh input.rbo output.rbo

# Minify for distribution
bash src/prep/minify_rbo.sh source.rbo minified.rbo

# Bundle multiple files
bash src/prep/bundle_rbo.sh main.rbo lib1.rbo lib2.rbo -o bundle.rbo

# Generate Python bridge
bash src/prep/generate_py_bridge.sh mymodule.py mymodule_bridge.c

# Validate syntax
bash src/prep/validate_syntax.sh source.rbo
```

## SDK & Extension Development

### Native Extension API

Build C extensions with the Rubolt SDK:

```bash
# Create native extension
rbcli newlib myext --native --sdk
cd myext

# Edit src/myext.c with your native functions
#include "shared/sdk/native/rubolt_api.h"

RUBOLT_FUNCTION(my_add) {
    int a = rubolt_get_int_arg(args, 0);
    int b = rubolt_get_int_arg(args, 1);
    return rubolt_make_int(a + b);
}

# Build extension
make
```

### SDK Components

**Native Extensions (`shared/sdk/native/`):**
- C API for creating Rubolt extensions
- Memory management helpers
- Type conversion utilities
- Error handling macros

**Bindings (`shared/sdk/bindings/`):**
- Pre-built bindings for popular libraries
- SQLite database integration
- HTTP client with curl
- Regular expressions (PCRE)
- Graphics (SDL2, OpenGL)

**Utilities (`shared/sdk/utils/`):**
- Code generator for boilerplate
- AST inspector for analysis
- Performance profiler
- Memory debugger

**Templates (`shared/sdk/templates/`):**
- Library project template
- CLI application template
- Web server template
- Native extension template

### Shared Modules

**Global Configuration (`shared/globals/config.rbo`):**
```rubolt
// Global configuration accessible across projects
let APP_VERSION = "1.0.0";
let DEBUG_MODE = true;
let MAX_CONNECTIONS = 100;
```

**Common Utilities (`shared/modules/common.rbo`):**
```rubolt
// Shared utility functions
def clamp(value: number, min: number, max: number) -> number {
    return value < min ? min : (value > max ? max : value);
}

def retry<T>(operation: function() -> T, max_attempts: number) -> T {
    for (attempt in 1..max_attempts + 1) {
        try {
            return operation();
        } catch (Exception e) {
            if (attempt == max_attempts) throw e;
            time.sleep(attempt * 0.1); // Exponential backoff
        }
    }
}
```

**Logging (`shared/modules/logger.rbo`):**
```rubolt
// Structured logging with levels
class Logger {
    level: string;
    
    def info(message: string) -> void {
        this.log("INFO", message);
    }
    
    def error(message: string) -> void {
        this.log("ERROR", message);
    }
    
    def debug(message: string) -> void {
        if (DEBUG_MODE) {
            this.log("DEBUG", message);
        }
    }
}
```

## Installation & Deployment

### Automated Installers

**Windows (PowerShell):**
```powershell
# Install from build/dist to user directory
pwsh -File Installer/install.ps1 -Mode auto -Prefix "$env:USERPROFILE\Rubolt" -AddToPath

# Install from source with build
pwsh -File Installer/install.ps1 -Mode source -Build -Prefix "$env:USERPROFILE\Rubolt-src" -AddToPath

# Uninstall
pwsh -File Installer/uninstall.ps1 -Prefix "$env:USERPROFILE\Rubolt"
```

**Unix/Linux/macOS:**
```bash
# Install to /usr/local (may require sudo)
bash Installer/install.sh --mode auto --prefix /usr/local --add-to-path

# Install to user directory
bash Installer/install.sh --mode auto --prefix ~/.local --add-to-path

# Uninstall
bash Installer/uninstall.sh --prefix /usr/local
```

### Installation Modes

- **`auto`**: Prefer `dist/` then `build/`, fallback to `source`
- **`dist`**: Install from prepacked `dist/` layout
- **`build`**: Install from locally built artifacts
- **`source`**: Copy entire repository and optionally build

### Installed Layout

```
<prefix>/
├── bin/                    # Executables (rubolt, rbcli)
├── lib/rubolt/            # Standard library and modules
│   ├── StdLib/            # Core standard library
│   ├── Objects/           # Built-in object types
│   ├── Modules/           # Native modules
│   └── Grammar/           # Grammar tests
└── share/rubolt/          # Documentation and examples
    ├── docs/              # Documentation
    ├── examples/          # Example programs
    └── vscode-extension/  # VS Code extension
```

## Project Structure

```
Rubolt/
├── src/                   # Core runtime and interpreter
│   ├── ast.c/h           # Abstract syntax tree
│   ├── parser.c/h        # Parser implementation
│   ├── interpreter.c/h   # Interpreter engine
│   ├── jit_compiler.c/h  # JIT compilation
│   ├── debugger.c/h      # Debugging support
│   ├── profiler.c/h      # Performance profiling
│   ├── async.c/h         # Async/await implementation
│   ├── threading.c/h     # Threading support
│   ├── dll_import.c/h    # DLL loading
│   └── prep/             # Source preparation scripts
├── python/               # Python interoperability
│   ├── gimmicks/         # Python-C bridge tools
│   └── rubolt_module.c   # Python extension module
├── collections/          # Collection data structures
│   ├── rb_list.c/h      # Dynamic arrays
│   ├── rb_collections.c/h # Hash tables, sets
│   └── test_collections.c # Collection tests
├── gc/                   # Garbage collector
│   ├── gc.c/h           # Mark-sweep collector
│   └── type_info.c/h    # Runtime type information
├── rc/                   # Reference counting
│   └── rc.c/h           # Reference counting system
├── runtime/              # Runtime management
│   ├── runtime.c/h      # Runtime initialization
│   └── manager.c/h      # Resource management
├── repl/                 # Interactive REPL
│   ├── repl.c/h         # REPL implementation
│   └── repl_main.c      # REPL entry point
├── cli/                  # Command-line interface
│   └── rbcli.c          # CLI implementation
├── tools/                # Development tools
│   ├── rubolt_lsp.c     # Language server
│   ├── formatter.c      # Code formatter
│   ├── linter.c         # Code linter
│   └── run_tests.py     # Test runner
├── bopes/                # Virtual machine simulator
│   ├── bopes.c/h        # Bopes VM implementation
├── vendor/               # Dependency management
│   ├── install.ps1/sh   # Dependency installers
│   ├── analyze_project.py # Project analyzer
│   ├── registry.json    # Package registry
│   └── script_ops/      # Cross-platform utilities
├── shared/               # SDK and shared resources
│   ├── sdk/             # Software development kit
│   ├── modules/         # Shared Rubolt modules
│   └── globals/         # Global configuration
├── vscode-rubolt/        # VS Code extension
│   ├── src/extension.ts # Extension implementation
│   ├── syntaxes/        # Syntax highlighting
│   └── package.json     # Extension manifest
├── compiletime/          # Compile-time tools
│   └── macros/          # Preprocessor macros
├── Installer/            # Installation scripts
│   ├── install.ps1/sh   # Cross-platform installers
│   └── package.ps1/sh   # Package builders
├── StdLib/               # Standard library (high-level)
│   ├── file.rbo         # File utilities
│   ├── http.rbo         # HTTP utilities
│   └── prelude.rbo      # Core prelude
├── Modules/              # Standard library (native)
│   ├── file_mod.c       # File I/O module
│   ├── json_mod.c       # JSON processing
│   ├── http_mod.c       # HTTP client
│   ├── time_mod.c       # Time operations
│   └── string_mod.c     # String utilities
├── Objects/              # Built-in object types
│   ├── array.rbo        # Array implementation
│   ├── string.rbo       # String implementation
│   └── number.rbo       # Number implementation
├── examples/             # Example programs
│   ├── hello.rbo        # Hello world
│   ├── complete_showcase.rbo # Feature showcase
│   └── production_ready_demo.rbo # Production example
├── benchmarks/           # Performance benchmarks
│   ├── loop.rbo         # Loop performance
│   ├── recursion.rbo    # Recursion performance
│   └── io.rbo           # I/O performance
├── Grammar/              # Language grammar
│   ├── tests/           # Grammar tests
│   └── CUtils/          # Grammar utilities
├── Docs/                 # Documentation (Sphinx)
│   ├── index.rst        # Main documentation
│   ├── getting_started.rst # Getting started guide
│   ├── language_reference.rst # Language reference
│   └── runtime.rst      # Runtime documentation
├── .github/              # GitHub workflows
│   └── workflows/       # CI/CD pipelines
├── .devcontainer/        # VS Code devcontainer
├── Dockerfile            # Docker build
├── Makefile             # Build system
└── build_all.bat/sh     # Build scripts
```

## Documentation

### Core Documentation
- **README.md** - This comprehensive overview
- **FEATURES.md** - Detailed feature documentation with examples
- **STDLIB.md** - Complete standard library reference
- **LANGUAGE_SERVER.md** - LSP implementation and IDE integration
- **ADVANCED.md** - Advanced language features and internals
- **QUICKREF.md** - Quick reference guide
- **PROJECT_SUMMARY.md** - Project overview and architecture

### Subsystem Documentation
- **gc/README.md** - Garbage collector implementation
- **rc/README.md** - Reference counting system
- **compiletime/README.md** - Compile-time macros and tools
- **vendor/README.md** - Vendor dependency system
- **python/gimmicks/README.md** - Python interoperability tools
- **shared/sdk/README.md** - SDK and extension development
- **Installer/README.md** - Installation system
- **src/prep/README.md** - Source preparation tools

### API Documentation
- **Docs/** - Sphinx-generated documentation
  - **getting_started.rst** - Getting started guide
  - **language_reference.rst** - Complete language reference
  - **runtime.rst** - Runtime system documentation
  - **stdlib.rst** - Standard library API
  - **cli.rst** - Command-line interface
  - **contributing.rst** - Contribution guidelines

### Build Documentation
```bash
# Generate documentation
cd Docs
make html

# View documentation
open _build/html/index.html
```

## Performance & Benchmarks

### JIT Compilation Performance
- **Hot path detection**: Functions compiled after 10+ executions
- **Native code generation**: Up to 10x performance improvement
- **Optimization passes**: Dead code elimination, constant folding
- **Inline caching**: 2-3x faster method/property access

### Memory Management
- **Garbage collection**: Sub-millisecond pause times
- **Reference counting**: Zero-overhead for acyclic structures
- **Memory pools**: Reduced allocation overhead
- **Type-aware traversal**: Faster GC with precise type information

### Benchmark Results
```bash
# Run performance benchmarks
rbcli benchmark benchmarks/

# Specific benchmarks
python tools/run_benchmarks.py
```

**Sample Results:**
- **Fibonacci (recursive)**: 50% faster than Python, 80% of C performance
- **Matrix multiplication**: 8x faster than Python with JIT
- **JSON parsing**: 3x faster than Python's json module
- **File I/O**: Comparable to native C performance

## Contributing

### Development Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/piratebomber/Rubolt.git
   cd Rubolt
   ```

2. **Build the project:**
   ```bash
   ./build_all.sh  # Unix/Linux/macOS
   # or
   build_all.bat   # Windows
   ```

3. **Run tests:**
   ```bash
   rbcli test
   python tools/run_tests.py
   ```

4. **Set up development environment:**
   ```bash
   # Install VS Code extension
   cd vscode-rubolt
   npm install
   npm run compile
   
   # Install pre-commit hooks
   pip install pre-commit
   pre-commit install
   ```

### Contribution Guidelines

- **Code style**: Follow existing conventions, use `rbformat` for formatting
- **Testing**: Add tests for new features, ensure all tests pass
- **Documentation**: Update documentation for new features
- **Commit messages**: Use conventional commit format
- **Pull requests**: Include description, tests, and documentation updates

### Areas for Contribution

- **Language features**: Pattern matching improvements, advanced generics
- **Standard library**: New modules and utilities
- **Performance**: JIT optimizations, memory management improvements
- **Tools**: IDE integration, debugging tools, profilers
- **Documentation**: Examples, tutorials, API documentation
- **Testing**: Property-based tests, fuzzing, benchmarks

## Roadmap

### Version 1.1 (Q2 2024)
- **Enhanced pattern matching** with more complex patterns
- **Improved JIT compiler** with better optimization passes
- **Package registry** with centralized package distribution
- **WebAssembly target** for browser execution

### Version 1.2 (Q3 2024)
- **Gradual typing** with optional static analysis
- **Coroutines** and advanced async programming
- **Foreign function interface** improvements
- **IDE plugins** for IntelliJ IDEA and Vim

### Version 2.0 (Q4 2024)
- **Self-hosting compiler** written in Rubolt
- **Advanced type system** with dependent types
- **Distributed computing** support
- **Mobile platform** support (iOS, Android)

## Community

- **GitHub**: [https://github.com/piratebomber/Rubolt](https://github.com/piratebomber/Rubolt)
- **Issues**: Report bugs and request features
- **Discussions**: Ask questions and share ideas
- **Wiki**: Community-maintained documentation
- **Discord**: Real-time chat and support (coming soon)

## License

Apache License 2.0 - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Inspiration**: Python's simplicity, C's performance, TypeScript's type system
- **Dependencies**: libcurl, json-c, and other open-source libraries
- **Contributors**: All the developers who have contributed to this project
- **Community**: Users who provide feedback and bug reports