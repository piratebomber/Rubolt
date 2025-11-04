# Python Gimmicks

Python tools for GCC-to-Python compilation and handling Python-to-C calls and operations for Rubolt.

## Components

### GCC-to-Python Compiler (`gcc_to_py.py`)
Compiles C code to Python bytecode/calls for Rubolt runtime.

### Python-to-C Bridge (`py_to_c_bridge.py`)
Handles Python→C function calls and type marshalling.

### C-to-Python Wrapper Generator (`c_wrapper_gen.py`)
Auto-generates Python wrappers for C functions.

### FFI Helper (`ffi_helper.py`)
Foreign Function Interface utilities using ctypes/cffi.

### Type Converter (`type_converter.py`)
Bidirectional type conversion between Python, C, and Rubolt types.

### Operation Dispatcher (`op_dispatcher.py`)
Routes Rubolt operations to appropriate C/Python implementations.

## Usage

### Compile C to Python

```bash
python python/gimmicks/gcc_to_py.py input.c -o output.py
```

### Generate C wrapper

```bash
python python/gimmicks/c_wrapper_gen.py mylib.so -o mylib_wrapper.py
```

### Bridge Python calls to C

```python
from python.gimmicks.py_to_c_bridge import call_c_function

result = call_c_function("my_native_func", [arg1, arg2])
```

### Type conversion

```python
from python.gimmicks.type_converter import py_to_c, c_to_py

c_val = py_to_c(42, "int")
py_val = c_to_py(c_val, "int")
```
