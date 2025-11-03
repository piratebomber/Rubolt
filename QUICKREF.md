# Rubolt Quick Reference

## Variables

```rubolt
let name: string = "value";     // Mutable
const PI: number = 3.14159;     // Immutable
var count: number = 0;          // Mutable (alternative)
```

## Types

```rubolt
number    // 42, 3.14
string    // "hello", 'world'
bool      // true, false
void      // No return value
any       // Any type
null      // Null value
```

## Functions

```rubolt
// With types
def add(a: number, b: number) -> number {
    return a + b;
}

// Python-style
def greet(name: string) -> string:
    return "Hello, " + name

// No return type
def log(msg: string) {
    print(msg);
}
```

## Control Flow

```rubolt
// If/Else
if (condition) {
    // code
} else {
    // code
}

// For loop (C-style)
for (let i: number = 0; i < 10; i = i + 1) {
    print(i);
}

// While loop
while (condition) {
    // code
}
```

## Operators

### Arithmetic
```rubolt
+  -  *  /  %    // Add, Sub, Mul, Div, Mod
```

### Comparison
```rubolt
==  !=  <  >  <=  >=
```

### Logical (Two Styles)
```rubolt
// C-style
&&  ||  !

// Python-style
and  or  not
```

## Modules

### Import
```rubolt
import math
import os
import file
import time
import sys
```

### Usage
```rubolt
math.sqrt(16)
os.getcwd()
file.read("data.txt")
time.now()
sys.version()
```

## Standard Library

### Math
```rubolt
math.sqrt(x)              // Square root
math.pow(x, y)            // Power
math.abs(x)               // Absolute value
math.floor(x)             // Floor
math.ceil(x)              // Ceiling
math.sin(x), math.cos(x)  // Trig
```

### File
```rubolt
file.read(path)           // Read file
file.write(path, content) // Write file
file.exists(path)         // Check existence
```

### OS
```rubolt
os.getcwd()               // Current directory
os.getenv(name)           // Environment variable
os.system(cmd)            // Run command
```

### Time
```rubolt
time.now()                // Timestamp
time.sleep(seconds)       // Sleep
```

### Sys
```rubolt
sys.version()             // Version string
sys.exit(code)            // Exit program
```

## Built-in Functions

```rubolt
print(value)              // Print with newline
printf(value)             // Print without newline
```

## CLI Commands

```bash
rbcli init <name>         # Create project
rbcli run <file>          # Run file
rbcli build               # Build project
rbcli newlib <name>       # Create library
rbcli test                # Run tests
rbcli version             # Show version
```

## Project Structure

```
project/
├── .rbo.config           # Configuration
├── src/
│   └── main.rbo         # Entry point
├── lib/                  # Libraries
└── tests/                # Tests
```

## Comments

```rubolt
// Single line (C/JS style)

# Single line (Python style)

/* Multi-line
   comment */
```

## Examples

### Hello World
```rubolt
print("Hello, World!");
```

### Function
```rubolt
def fibonacci(n: number) -> number {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

print(fibonacci(10));
```

### File I/O
```rubolt
import file

let data: string = file.read("input.txt");
file.write("output.txt", data);
```

### Math Operations
```rubolt
import math

let x: number = math.pow(2, 8);
let y: number = math.sqrt(x);
print("Result: " + y);
```

## Type Annotations

```rubolt
// Variable
let name: string = "value";

// Function parameter
def process(data: string) -> number {
    return 42;
}

// Return type
def getValue() -> string {
    return "value";
}
```

## Error Handling

```rubolt
if (file.exists("config.json")) {
    let config: string = file.read("config.json");
} else {
    print("Config not found!");
}
```

## Common Patterns

### Loop
```rubolt
for (let i: number = 0; i < items; i = i + 1) {
    process(i);
}
```

### Conditional
```rubolt
let result: string = condition ? "yes" : "no";
// Note: Ternary not yet supported, use if/else
```

### Function Composition
```rubolt
def double(x: number) -> number {
    return x * 2;
}

def square(x: number) -> number {
    return x * x;
}

let value: number = square(double(5)); // 100
```

---

For complete documentation, see **ADVANCED.md**
