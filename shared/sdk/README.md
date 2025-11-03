# Rubolt SDK

Development kit for building Rubolt extensions, native modules, and applications.

## Components

### Native Extensions API (`native/`)
Build C extensions for Rubolt with proper FFI bindings.

### Bindings (`bindings/`)
Pre-built bindings for popular libraries:
- SQLite
- HTTP/curl
- Regex (PCRE)
- Graphics (SDL2, OpenGL)

### Utilities (`utils/`)
Development utilities:
- Code generator
- AST inspector
- Performance profiler
- Memory debugger

### Templates (`templates/`)
Project scaffolding:
- Library template
- CLI app template
- Web server template
- Native extension template

### Examples (`examples/`)
Complete working examples demonstrating SDK features.

## Quick Start

### Create Native Extension

```bash
rbcli newlib myext --native --sdk
cd myext
# Edit src/myext.c with your native functions
make
```

### Use SDK Utilities

```rubolt
import shared.sdk.utils.profiler

profiler.start("my_operation")
# ... your code ...
profiler.stop("my_operation")
profiler.report()
```

### Load SDK Bindings

```rubolt
import shared.sdk.bindings.sqlite as db

let conn = db.connect("app.db")
db.execute(conn, "CREATE TABLE users (id INT, name TEXT)")
```

## API Documentation

See `docs/sdk_api.md` for complete API reference.
