# Rubolt Language Features

This document describes all the advanced features implemented in Rubolt, making it a modern, powerful programming language.

## Core Language Features

### Pattern Matching

Rubolt supports comprehensive pattern matching with guards, destructuring, and wildcard patterns.

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

**Pattern Types:**
- **Literal patterns**: `42`, `"hello"`, `true`
- **Identifier patterns**: `x`, `name` (binds variables)
- **Wildcard pattern**: `_` (matches anything)
- **Tuple patterns**: `(x, y, z)`
- **List patterns**: `[head, ...tail]`, `[a, b, c]`
- **Object patterns**: `{name, age}`, `{x, y, ...rest}`
- **Type patterns**: `String`, `Number`, `MyClass`
- **Guard patterns**: `x if x > 0`

### Generics System

Full generic programming support with type parameters, constraints, and automatic instantiation.

```rubolt
// Generic function
def identity<T>(value: T) -> T {
    return value;
}

// Generic class
class List<T> {
    items: T[];
    
    def add(item: T) -> void {
        this.items.append(item);
    }
    
    def get(index: number) -> T {
        return this.items[index];
    }
}

// Constrained generics
def compare<T: Comparable>(a: T, b: T) -> number {
    return a.compare(b);
}

// Multiple type parameters
def zip<A, B>(list1: A[], list2: B[]) -> (A, B)[] {
    let result = [];
    for (i in 0..min(list1.length, list2.length)) {
        result.append((list1[i], list2[i]));
    }
    return result;
}
```

**Generic Features:**
- **Type parameters**: `<T>`, `<T, U, V>`
- **Type constraints**: `<T: Comparable>`, `<T: Numeric>`
- **Generic functions**: Automatic type inference
- **Generic classes**: Parameterized data structures
- **Type substitution**: Compile-time specialization

### Error Handling

Comprehensive error handling with Result types and try/catch blocks.

```rubolt
// Result types
def safe_divide(a: number, b: number) -> Result<number, string> {
    if (b == 0) {
        return Error("Division by zero");
    }
    return Ok(a / b);
}

// Try/catch blocks
def process_file(filename: string) -> void {
    try {
        let content = file.read(filename);
        let data = json.parse(content);
        print("Processed: " + data.name);
    } catch (FileNotFoundError e) {
        print("File not found: " + e.message);
    } catch (JsonParseError e) {
        print("Invalid JSON: " + e.message);
    } finally {
        print("Cleanup complete");
    }
}

// Throwing errors
def validate_age(age: number) -> void {
    if (age < 0) {
        throw ValueError("Age cannot be negative");
    }
    if (age > 150) {
        throw ValueError("Age seems unrealistic");
    }
}
```

**Error Types:**
- `RuntimeError` - General runtime errors
- `TypeError` - Type-related errors
- `IndexError` - Array/list index errors
- `KeyError` - Dictionary key errors
- `NullError` - Null pointer errors
- `FileNotFoundError` - File system errors
- `NetworkError` - Network-related errors

## Performance Features

### JIT Compilation

Automatic just-in-time compilation for hot code paths with optimization.

```rubolt
// This function will be JIT compiled after 10+ executions
def fibonacci(n: number) -> number {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// Force JIT compilation
@jit_compile
def matrix_multiply(a: number[][], b: number[][]) -> number[][] {
    // Hot path - will be compiled to native code
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

**JIT Features:**
- **Hot path detection**: Automatic compilation of frequently executed code
- **Optimization passes**: Dead code elimination, constant folding
- **Native code generation**: x86-64 machine code
- **Inline caching**: Optimized method/property access
- **Performance profiling**: Execution statistics and timing

### Memory Management

Advanced memory management with both garbage collection and reference counting.

```rubolt
// Automatic memory management
def create_large_data() -> dict {
    let data = {};
    for (i in 0..1000000) {
        data[i] = "item_" + i;
    }
    return data; // Automatically managed
}

// Manual memory control
@no_gc
def performance_critical() -> void {
    // Disable GC for this function
    let buffer = allocate_buffer(1024 * 1024);
    // ... performance critical code ...
    free_buffer(buffer);
}

// Weak references
class Node {
    value: any;
    parent: weak Node; // Weak reference to prevent cycles
    children: Node[];
}
```

## Standard Library

### File I/O Module

Comprehensive file system operations with error handling.

```rubolt
import file

// Basic file operations
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
for (line in lines) {
    print("Processing: " + line);
}
```

### JSON Module

Fast JSON parsing and serialization.

```rubolt
import json

// Parse JSON
let data = json.parse('{"name": "John", "age": 30}');
print("Name: " + data.name);

// Generate JSON
let person = {
    name: "Alice",
    age: 25,
    hobbies: ["reading", "coding", "music"]
};
let json_str = json.stringify(person);
file.write("person.json", json_str);
```

### HTTP Module

Full-featured HTTP client with async support.

```rubolt
import http
import json

// Simple requests
let response = http.get("https://api.github.com/users/octocat");
let user = json.parse(response);

// POST with JSON
let new_user = {name: "John", email: "john@example.com"};
let result = http.post("https://api.example.com/users", 
                      json.stringify(new_user), 
                      "application/json");

// REST client
class ApiClient {
    base_url: string;
    
    def get_user(id: number) -> dict {
        let url = this.base_url + "/users/" + id;
        return json.parse(http.get(url));
    }
    
    def create_user(user: dict) -> dict {
        let url = this.base_url + "/users";
        return json.parse(http.post(url, json.stringify(user)));
    }
}
```

### Time Module

Date and time operations with formatting.

```rubolt
import time

// Current time
let now = time.now();
let formatted = time.format(now, "%Y-%m-%d %H:%M:%S");

// Date arithmetic
let tomorrow = now + (24 * 60 * 60); // Add 24 hours
let next_week = time.add_days(now, 7);

// Parsing
let parsed = time.parse("2024-01-15 14:30:00", "%Y-%m-%d %H:%M:%S");

// Components
print("Year: " + time.year(now));
print("Month: " + time.month(now));
print("Day: " + time.day(now));
```

## Testing Framework

Comprehensive testing with unit tests, property-based testing, and benchmarks.

```rubolt
import test

// Unit tests
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

// Property-based testing
test.property("Addition is commutative", 
    generate.int(-1000, 1000), 
    generate.int(-1000, 1000),
    (a, b) => a + b == b + a
);

// Benchmarks
test.benchmark("Fibonacci", () => {
    fibonacci(30);
}, {iterations: 100});

// Mocking
let mock_api = test.mock();
mock_api.expect("get_user", [123], {name: "John", id: 123});

let result = mock_api.get_user(123);
assert.equal(result.name, "John");
assert.verify(mock_api);
```

## Package Management

Full package manager with dependency resolution and semantic versioning.

```rubolt
// package.json
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

**Package Commands:**
```bash
rbcli init my-project          # Create new project
rbcli add http-client@2.1.0    # Add dependency
rbcli remove old-package       # Remove dependency
rbcli install                  # Install all dependencies
rbcli update                   # Update dependencies
rbcli publish                  # Publish to registry
```

## Developer Tools

### Code Formatter

Automatic code formatting with configurable style.

```bash
# Format file
rbformat src/main.rbo

# Format with custom style
rbformat --indent-size 2 --max-line-length 120 src/

# Format in place
rbformat --write src/**/*.rbo
```

### Linter

Static analysis and code quality checking.

```bash
# Lint file
rblint src/main.rbo

# Custom rules
rblint --max-complexity 15 --max-line-length 100 src/

# Configuration file
rblint --config .rblint.json src/
```

**Lint Rules:**
- Naming conventions (snake_case, camelCase)
- Missing return types
- Unused variables
- Dead code detection
- Cyclomatic complexity
- Line length limits
- Code style consistency

### Language Server

Full LSP implementation for IDE integration.

**Features:**
- **Code completion**: Keywords, functions, modules
- **Error diagnostics**: Real-time error detection
- **Hover information**: Type and documentation
- **Go to definition**: Navigate to symbols
- **Find references**: Find all usages
- **Code formatting**: Automatic formatting
- **Signature help**: Function parameter hints

### VS Code Extension

Complete VS Code integration with rich language support.

**Features:**
- Syntax highlighting
- Code completion
- Error squiggles
- Debugging support
- Integrated terminal
- Run commands
- REPL integration
- Project templates

## Advanced Examples

### Web Server

```rubolt
import http
import json
import file

class WebServer {
    port: number;
    routes: dict;
    
    def init(port: number) {
        this.port = port;
        this.routes = {};
    }
    
    def route(path: string, handler: function) -> void {
        this.routes[path] = handler;
    }
    
    def start() -> void {
        print("Server starting on port " + this.port);
        // Server implementation
    }
}

let server = WebServer(8080);

server.route("/api/users", (request) => {
    match request.method {
        "GET" => {
            let users = json.parse(file.read("users.json"));
            return {status: 200, body: json.stringify(users)};
        };
        "POST" => {
            let user = json.parse(request.body);
            // Save user logic
            return {status: 201, body: json.stringify(user)};
        };
        _ => {
            return {status: 405, body: "Method not allowed"};
        };
    }
});

server.start();
```

### Data Processing Pipeline

```rubolt
import file
import json
import time

class DataPipeline<T> {
    stages: function[];
    
    def add_stage(stage: function) -> DataPipeline<T> {
        this.stages.append(stage);
        return this;
    }
    
    def process(data: T[]) -> T[] {
        let result = data;
        for (stage in this.stages) {
            result = result.map(stage).filter(x => x != null);
        }
        return result;
    }
}

// Usage
let pipeline = DataPipeline<dict>()
    .add_stage(data => {
        // Validate data
        if (!data.name || !data.email) return null;
        return data;
    })
    .add_stage(data => {
        // Transform data
        data.email = data.email.lower();
        data.created_at = time.now();
        return data;
    })
    .add_stage(data => {
        // Enrich data
        data.domain = data.email.split("@")[1];
        return data;
    });

let raw_data = json.parse(file.read("input.json"));
let processed = pipeline.process(raw_data);
file.write("output.json", json.stringify(processed));
```

This comprehensive feature set makes Rubolt a modern, powerful programming language suitable for everything from scripting to large-scale application development.