# Rubolt Programming Language

Rubolt is a modern programming language that combines the best features of Python, C, and TypeScript.

## Features

- **Hybrid Syntax**: Combines Python's readability, C's performance, and TypeScript's type system
- **C Interpreter**: Fast interpreter written in C
- **Python Bindings**: Use Rubolt from Python
- **VSCode Extension**: Full syntax highlighting and IntelliSense support

## File Extensions

- `.rbo` - Rubolt source files
- `.rbo.config` - Rubolt project configuration

## Quick Start

### Building the Interpreter

```bash
cd src
make
```

### Running Rubolt Code

```bash
./rubolt examples/hello.rbo
```

### Using from Python

```python
import rubolt
rubolt.run_file("script.rbo")
```

## Syntax Overview

```rubolt
// TypeScript-style variable declarations with types
let name: string = "Rubolt";
const version: number = 1.0;

// Python-style functions with type hints
def greet(name: string) -> string:
    return f"Hello, {name}!";

// C-style control flow
if (version >= 1.0) {
    printf("Production ready!\n");
}

// Hybrid loop syntax
for (let i = 0; i < 10; i++) {
    print(i);
}
```

## Configuration

Create a `.rbo.config` file in your project root:

```json
{
    "version": "1.0",
    "entry": "main.rbo",
    "strict": true,
    "output": "build/"
}
```

## VSCode Extension

Install the extension from `vscode-rubolt/` to get syntax highlighting for `.rbo` files.

## Advanced Features

### Type Checking & Error Handling

Rubolt includes comprehensive type checking with detailed error messages:

```rubolt
let x: string = 123;  // Type error with helpful hints!
```

### Standard Library

Built-in modules for common tasks:

```rubolt
import math
import os
import file
import time

let result: number = math.sqrt(16);
let dir: string = os.getcwd();
```

Available modules:
- **math** - Mathematical operations (sqrt, pow, sin, cos, etc.)
- **os** - Operating system interface (getcwd, getenv, system)
- **file** - File operations (read, write, exists)
- **time** - Time utilities (now, sleep)
- **sys** - System utilities (exit, version)

### CLI Tool

Powerful command-line tool for project management:

```bash
# Create new project
rbcli init my-project

# Run code
rbcli run main.rbo

# Create library with interactive template
rbcli newlib mylib

# Build project
rbcli build
```

### Library Development

Create custom libraries easily:

```bash
rbcli newlib mylib
```

Generates:
- `.rbo` library file with template
- Optional Python/C native bridge
- README documentation
- Example usage file

## Building

### Build Everything

**Windows:**
```bash
build_all.bat
```

This builds:
1. Rubolt interpreter with type checking
2. Module system + standard library
3. CLI tool (rbcli)

**Linux/Mac:**
```bash
chmod +x build_all.sh
./build_all.sh
```

### Build Individual Components

**Interpreter only:**
```bash
cd src
make
```

**CLI tool only:**
```bash
cd cli
make
```

## Documentation

- **README.md** - This file (overview)
- **ADVANCED.md** - Complete guide to advanced features
- **examples/** - Example programs

## Project Structure

```
Rubolt/
├── src/               # Interpreter source
│   ├── lexer.c/h     # Lexical analysis
│   ├── parser.c/h    # Syntax parsing
│   ├── ast.c/h       # Abstract syntax tree
│   ├── interpreter.c/h  # Interpreter
│   ├── typechecker.c/h  # Type checking
│   ├── module.c/h    # Module system
│   └── main.c        # Entry point
├── cli/              # CLI tool
│   ├── rbcli.c       # CLI implementation
│   └── Makefile
├── python/           # Python bindings
├── vscode-rubolt/    # VSCode extension
├── examples/         # Example programs
├── lib/              # User libraries
└── stdlib/           # Standard library
```

## License

MIT
