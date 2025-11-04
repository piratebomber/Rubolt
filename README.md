# Rubolt Programming Language

Rubolt is a modern language and toolchain blending Python, C, and TypeScript. It features a fast C runtime, static typing, rich standard library, JIT scaffolding, REPL/debugging/profiling, async + threading, DLL interop, and deep Python/C bridging.

## Highlights

- Hybrid syntax with static type checking and helpful errors
- Fast C interpreter/runtime with GC + RC and type-aware object graph traversal
- Collections (Lists, Dicts, Sets, Tuples) with Pythonic utilities
- REPL with debugger (breakpoints/step/inspect) and profiler
- JIT + inline caching scaffolding, hot path stats
- Async/await, event loop, threading with GIL-like lock
- DLL import: `import mylib.dll` (precompiled or compiled-on-demand)
- Python interop (gcc→py, py→C bridge, FFI, wrapper generator)
- Vendor system, compile-time macros, SDK, and VS Code support
- Devcontainer + Docker, CI/CD workflows, and installer scripts

## Quick Start

- Native build (Windows):
  ```bash
  build_all.bat
  ```
- Native build (macOS/Linux):
  ```bash
  chmod +x build_all.sh
  ./build_all.sh
  ```
- Docker:
  ```bash
  docker build -t rubolt .
  docker run --rm -it -v $PWD:/work rubolt rbcli run examples/hello.rbo
  ```
- Devcontainer: open the repo in VS Code and “Reopen in Container”.

Run a file:
```bash
rbcli run examples/hello.rbo
```
Enter the REPL:
```bash
rbcli repl
```

## Using DLLs

Place DLLs/shared libs in `src/precompiled/` or let Rubolt compile on import:
```rubolt
import mymath.dll
print(mymath.add(2, 40))
```
If missing, Rubolt will attempt to build from source (vendor/cases) and cache it in `src/precompiled/`.

## Python Interop (gimmicks)

- gcc→py compiler, py→C bridge, FFI helper, wrapper generator, type converter, op dispatcher.
- Parse C headers/JSON to generate robust Python/C bindings.
- Example:
  ```bash
  python python/gimmicks/c_wrapper_gen.py vendor/lib/mylib.so -o mylib_wrapper.py
  ```

## Collections

- Lists: append, insert, pop, remove, slice, copy, sort, reverse
- Dicts: hash table with string/int keys, iteration helpers
- Sets: hash-based unique containers
- Tuples: fixed-size arrays, hashable

## Concurrency and Async

- Async/await with a simple event loop (timers + I/O hooks)
- Threading API with a GIL-like lock and thread pool utilities

## Memory Management

- GC: pool-optimized mark-sweep with type-driven graph traversal
- RC: strong/weak refs, deterministic release + cycle detection
- Type info system describes layouts and pointer fields for accurate traversal

## REPL, Debugger, Profiler

- REPL with history and completion
- Debugger: breakpoints, step, call stack, inspect
- Profiler: timings, hotspots, reports

## JIT + Inline Cache (scaffolding)

- Executable memory allocation, placeholders for hot-path codegen
- Inline caches for method/attribute lookups with stats

## CLI Overview

```bash
rbcli init <name>         # New project
rbcli run <file>          # Run file
rbcli repl                # Interactive shell
rbcli build               # Build project
rbcli newlib <name>       # Library template (native optional)
rbcli test                # Run tests
rbcli sim <file>          # Run in Bopes simulator
rbcli compile <file>      # Emit/read .rbo binary
rbcli version             # Version info
```

## Configuration (.rbo.config)

```json
{
  "version": "1.0",
  "entry": "src/main.rbo",
  "strict": true,
  "typecheck": true,
  "optimize": false,
  "output": "build/"
}
```

## Development Environment

- .devcontainer for VS Code Remote Containers
- Dockerfile for reproducible builds
- VS Code: `.vscode/settings.json`, `tasks.json`, `launch.json`, `extensions.json`
- GitHub Actions: build, lint, tests, fuzz (placeholder), docs, security scan, stale issues

## Vendor and Compiletime

- `vendor/` installers for headers/libs per-OS, project analyzer, registry/manifest
- `compiletime/macros/` preprocessor macros (debugging, token aliasing, type helpers)

## SDK and Shared

- `shared/sdk/` with native extension API, bindings, utilities, templates, examples
- `shared/modules/`, `shared/globals/` for cross-project reuse

## Project Layout (abridged)

```
src/          # Runtime, parser, interpreter, modules
python/       # Python bindings and gimmicks
vendor/       # Vendor manager (installers, registry, assets)
shared/       # SDK, examples, shared modules/globals
vscode-rubolt/# VS Code extension
.gc/, rc/, type_info/ # Memory systems and type metadata
```

## Documentation

- README.md (this)
- ADVANCED.md, QUICKREF.md, PROJECT_SUMMARY.md
- Subsystem READMEs under `gc/`, `rc/`, `compiletime/`, `vendor/`, `python/gimmicks/`, `shared/sdk/`

## License

Apache 2.0