# Gimmicks Headers Directory

C header files (.h) and JSON function definitions for automatic Python wrapper generation.

## Usage

### Parse specific header

```bash
python ../c_wrapper_gen.py mylib.so -o wrapper.py -H rubolt_math.h
```

### Parse from JSON

```bash
python ../c_wrapper_gen.py mylib.so -o wrapper.py -f example_functions.json
```

### Auto-discover all headers

```bash
python ../c_wrapper_gen.py mylib.so -o wrapper.py --auto-headers
```

## Header Files

- `rubolt_math.h` — Math operations (add, subtract, power, trig, etc.)
- `rubolt_string.h` — String manipulation (concat, reverse, find, replace)
- `rubolt_io.h` — File and console I/O operations

## JSON Format

Function definitions in JSON should follow this schema:

```json
{
  "library": "optional_lib_name.so",
  "functions": [
    {
      "name": "function_name",
      "return_type": "int",
      "params": [
        {
          "name": "param_name",
          "type": "param_type"
        }
      ]
    }
  ]
}
```

Or as a simple array:

```json
[
  {
    "name": "func1",
    "return_type": "void",
    "params": []
  }
]
```

## Adding New Headers

1. Create `.h` file in this directory
2. Use standard C function declarations ending with `;`
3. Comments will be automatically stripped during parsing
4. Run with `--auto-headers` to include all headers
