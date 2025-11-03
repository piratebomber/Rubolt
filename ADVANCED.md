# Rubolt Advanced Features Guide

## Table of Contents
1. [Type System & Error Handling](#type-system--error-handling)
2. [Module System](#module-system)
3. [Standard Library](#standard-library)
4. [CLI Tools](#cli-tools)
5. [Creating Libraries](#creating-libraries)

---

## Type System & Error Handling

### Type Checking

Rubolt includes a static type checker that validates type annotations:

```rubolt
// Type mismatch error
let name: string = 123;  // Error: expected 'string', got 'number'

// Correct
let name: string = "Rubolt";
```

### Error Messages

The type checker provides detailed error messages with hints:

```
Type Errors Found:
─────────────────────────────────────────

✗ Error: Type mismatch for variable 'name': expected 'string', got 'number'
  → at line 5, column 10
  💡 Hint: Consider changing the type annotation or the initializer value
```

### Supported Types

- `number` - Integers and floats
- `string` - Text values
- `bool` - true/false
- `void` - No return value
- `any` - Any type (disables type checking)
- `null` - Null value

---

## Module System

### Importing Modules

Import standard library modules:

```rubolt
import math
import os
import file

let result: number = math.sqrt(16);
```

### Import Syntax

```rubolt
// Import entire module
import math

// Use module functions
let x: number = math.pow(2, 8);
```

### Module Search Paths

Rubolt searches for modules in:
1. `./lib/` - Project libraries
2. `./stdlib/` - Standard library
3. Current directory

---

## Standard Library

### Math Module

Mathematical operations:

```rubolt
import math

// Basic operations
math.sqrt(x: number) -> number     // Square root
math.pow(x: number, y: number) -> number  // Power
math.abs(x: number) -> number      // Absolute value
math.floor(x: number) -> number    // Floor
math.ceil(x: number) -> number     // Ceiling

// Trigonometry
math.sin(x: number) -> number      // Sine
math.cos(x: number) -> number      // Cosine

// Example
let result: number = math.sqrt(25);  // 5
let power: number = math.pow(2, 10); // 1024
```

### OS Module

Operating system interactions:

```rubolt
import os

// Functions
os.getcwd() -> string              // Get current directory
os.getenv(name: string) -> string  // Get environment variable
os.system(cmd: string) -> number   // Execute system command

// Example
let dir: string = os.getcwd();
print("Current directory: " + dir);

let home: string = os.getenv("HOME");
```

### File Module

File operations:

```rubolt
import file

// Functions
file.read(path: string) -> string           // Read file
file.write(path: string, content: string) -> bool  // Write file
file.exists(path: string) -> bool           // Check if file exists

// Example
let content: string = file.read("data.txt");
let success: bool = file.write("output.txt", "Hello!");

if (file.exists("config.json")) {
    print("Config found");
}
```

### Time Module

Time operations:

```rubolt
import time

// Functions
time.now() -> number               // Current timestamp
time.sleep(seconds: number) -> void  // Sleep

// Example
let timestamp: number = time.now();
print("Sleeping...");
time.sleep(2);
print("Done!");
```

### Sys Module

System utilities:

```rubolt
import sys

// Functions
sys.exit(code: number) -> void     // Exit program
sys.version() -> string            // Get Rubolt version

// Example
print(sys.version());

if (error) {
    sys.exit(1);
}
```

---

## CLI Tools

### rbcli - Rubolt CLI Tool

The `rbcli` command provides project management:

```bash
╔═══════════════════════════════════════╗
║         RUBOLT CLI TOOL v1.0          ║
║  Build, Run, and Manage Rubolt Apps  ║
╚═══════════════════════════════════════╝
```

### Commands

#### Run Files

```bash
rbcli run main.rbo
```

#### Initialize Project

Create a new Rubolt project with structure:

```bash
rbcli init my-project
```

Creates:
```
my-project/
├── src/
│   └── main.rbo
├── lib/
├── tests/
├── .rbo.config
└── README.md
```

#### Build Project

```bash
cd my-project
rbcli build
```

#### Run Tests

```bash
rbcli test
```

#### Version Info

```bash
rbcli version
```

---

## Creating Libraries

### Interactive Library Generator

Create a new library with guided prompts:

```bash
rbcli newlib mylib
```

Interactive prompts:
```
╔═══════════════════════════════════════╗
║   Rubolt Library Template Generator   ║
╚═══════════════════════════════════════╝

Creating library: mylib

Description (optional): My awesome library
Author (optional): Your Name
Include native C functions? (y/n): y
```

### Generated Structure

```
lib/mylib/
├── mylib.rbo          # Main library file
├── mylib_native.py    # Python/C bridge (if native)
├── README.md          # Documentation
└── example.rbo        # Usage example
```

### Library Template (.rbo)

Generated library structure:

```rubolt
// mylib Library
// My awesome library
// Author: Your Name

// Public API
def hello() -> string {
    return "Hello from mylib library!";
}

def version() -> string {
    return "1.0.0";
}

// Example function
def calculate(x: number, y: number) -> number {
    return x + y;
}
```

### Python Bridge (Optional)

For native C extensions:

```python
# mylib Native Extensions
# Python bridge for native C functions

def native_function(x):
    """Example native function."""
    return x * 2

def init():
    """Initialize the native module."""
    print("Native module 'mylib' loaded")
```

### Using Custom Libraries

```rubolt
import mylib

def main() -> void {
    print(mylib.hello());
    print("Version: " + mylib.version());
    
    let result: number = mylib.calculate(10, 20);
    print("Result: " + result);
}

main();
```

### Library Documentation Template

Generated README.md:

```markdown
# mylib Library

My awesome library

## Installation

```rubolt
import mylib
```

## Usage

```rubolt
import mylib

let msg: string = mylib.hello();
print(msg);
```

## API Reference

### Functions

- `hello() -> string` - Returns a greeting message
- `version() -> string` - Returns the library version
- `calculate(x: number, y: number) -> number` - Example calculation
```

---

## Building the Complete System

### Build All Components

Use the comprehensive build script:

**Windows:**
```bash
build_all.bat
```

**Linux/Mac:**
```bash
chmod +x build_all.sh
./build_all.sh
```

This builds:
1. Rubolt interpreter with type checking
2. Module system with standard library
3. CLI tool (rbcli)

### Quick Start Workflow

```bash
# 1. Build everything
build_all.bat

# 2. Create a new project
rbcli init my-app

# 3. Navigate to project
cd my-app

# 4. Run the project
rbcli run src/main.rbo

# 5. Create a library
rbcli newlib utils

# 6. Use the library in your code
# Edit src/main.rbo to add: import utils
```

---

## Configuration

### .rbo.config

Project configuration file:

```json
{
  "version": "1.0.0",
  "name": "my-project",
  "entry": "src/main.rbo",
  "output": "build/",
  "strict": true,
  "typecheck": true,
  "optimize": false
}
```

Options:
- `version` - Project version
- `name` - Project name
- `entry` - Main entry file
- `output` - Build output directory
- `strict` - Enable strict mode
- `typecheck` - Enable type checking
- `optimize` - Enable optimizations

---

## Best Practices

### Type Annotations

Always use type annotations for better error catching:

```rubolt
// Good
def calculate(x: number, y: number) -> number {
    return x + y;
}

// Avoid
def calculate(x, y) {
    return x + y;
}
```

### Module Organization

```
project/
├── src/
│   ├── main.rbo       # Entry point
│   ├── utils.rbo      # Utilities
│   └── config.rbo     # Configuration
├── lib/
│   └── custom/        # Custom libraries
└── tests/
    └── test_utils.rbo
```

### Error Handling

```rubolt
import file

def safe_read(path: string) -> string {
    if (file.exists(path)) {
        return file.read(path);
    } else {
        print("File not found: " + path);
        return "";
    }
}
```

---

## Troubleshooting

### Type Errors

If you encounter type errors, check:
1. Variable declarations match their types
2. Function return types are correct
3. Arguments match parameter types

### Import Errors

If modules don't load:
1. Check module exists in `lib/` or `stdlib/`
2. Verify module name matches file name
3. Check search paths in `.rbo.config`

### Build Errors

If compilation fails:
1. Ensure GCC is installed
2. Check all source files exist
3. Verify file permissions

---

## Examples

See the `examples/` directory for complete examples:

- `hello.rbo` - Basic syntax
- `control_flow.rbo` - Control structures
- `functions.rbo` - Function examples
- `types.rbo` - Type system
- `using_stdlib.rbo` - Standard library usage

---

For more information, visit the [Rubolt GitHub repository](https://github.com/rubolt/rubolt).
