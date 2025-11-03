# Rubolt SDK API Documentation

## Native Extensions API

### Value Creation

```c
RbValue rb_int(int64_t val);
RbValue rb_float(double val);
RbValue rb_string(const char* str);
RbValue rb_bool(bool val);
RbValue rb_null(void);
```

### Value Access

```c
int64_t rb_as_int(RbValue v);
double rb_as_float(RbValue v);
const char* rb_as_string(RbValue v);
bool rb_as_bool(RbValue v);
RbType rb_typeof(RbValue v);
```

### Collections

```c
// Lists
RbValue rb_list(int size);
void rb_list_append(RbValue list, RbValue item);
RbValue rb_list_get(RbValue list, int index);
int rb_list_len(RbValue list);

// Dictionaries
RbValue rb_dict(void);
void rb_dict_set(RbValue dict, const char* key, RbValue val);
RbValue rb_dict_get(RbValue dict, const char* key);
```

### Module Registration

```c
typedef RbValue (*RbNativeFunc)(int argc, RbValue* argv);

typedef struct {
    const char* name;
    RbNativeFunc func;
} RbModuleFunc;

void rb_register_module(const char* module_name, RbModuleFunc* funcs, int count);
```

### Example

```c
#include "rubolt_api.h"

static RbValue my_add(int argc, RbValue* argv) {
    if (argc != 2) {
        rb_raise_error("add() requires 2 arguments");
        return rb_null();
    }
    
    int64_t a = rb_as_int(argv[0]);
    int64_t b = rb_as_int(argv[1]);
    return rb_int(a + b);
}

static RbModuleFunc my_funcs[] = {
    {"add", my_add}
};

void rb_init_mymodule(void) {
    rb_register_module("mymodule", my_funcs, 1);
}
```

## Shared Modules

### common.rbo

```rubolt
import shared.modules.common as common

common.clamp(value, min, max)      # Clamp value to range
common.is_empty(str)                # Check if string is empty
common.join_path(parts)             # Join path components
common.safe_divide(a, b, default)   # Division with zero check
```

### logger.rbo

```rubolt
import shared.modules.logger as log

log.info(message)
log.warn(message)
log.error(message)
log.debug(message)
```

## SDK Utilities

### Profiler

```rubolt
import shared.sdk.utils.profiler as prof

prof.start("operation_name")
# ... code to profile ...
prof.stop("operation_name")
prof.report()  # Print timing report
```

## SDK Bindings

### SQLite

```rubolt
import shared.sdk.bindings.sqlite as db

conn = db.connect("database.db")
db.execute(conn, "CREATE TABLE users (id INT, name TEXT)")
rows = db.query(conn, "SELECT * FROM users WHERE id = ?", [1])
db.close(conn)
```

### HTTP

```rubolt
import shared.sdk.bindings.http as http

response = http.get("https://api.example.com/data", {})
response = http.post("https://api.example.com/users", body, headers)
http.download("https://example.com/file.zip", "file.zip")
```

## Global Configuration

```rubolt
import shared.globals.config as config

print(config.VERSION)
print(config.DEBUG_MODE)
print(config.MAX_WORKERS)
```
