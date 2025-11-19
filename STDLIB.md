# Rubolt Standard Library

The Rubolt standard library provides essential modules for file I/O, JSON processing, time operations, HTTP requests, and more.

## File Module

The `file` module provides comprehensive file system operations.

### Functions

- `read(path: string) -> string` - Read entire file content
- `write(path: string, content: string) -> bool` - Write content to file
- `append(path: string, content: string) -> bool` - Append content to file
- `exists(path: string) -> bool` - Check if file exists
- `size(path: string) -> number` - Get file size in bytes
- `delete(path: string) -> bool` - Delete file
- `copy(src: string, dst: string) -> bool` - Copy file
- `readlines(path: string) -> list` - Read file as list of lines

### Example

```rubolt
import file

// Write and read a file
file.write("hello.txt", "Hello, Rubolt!");
let content = file.read("hello.txt");
print(content); // "Hello, Rubolt!"

// Check file properties
if (file.exists("hello.txt")) {
    let size = file.size("hello.txt");
    print("File size: " + size + " bytes");
}

// Read lines
file.write("lines.txt", "Line 1\nLine 2\nLine 3");
let lines = file.readlines("lines.txt");
for (line in lines) {
    print(line);
}
```

## JSON Module

The `json` module provides JSON parsing and stringification.

### Functions

- `parse(json_string: string) -> any` - Parse JSON string to Rubolt value
- `stringify(value: any) -> string` - Convert Rubolt value to JSON string

### Example

```rubolt
import json

// Create and stringify data
let data = {
    "name": "Rubolt",
    "version": 1.0,
    "features": ["fast", "typed", "modern"]
};

let json_str = json.stringify(data);
print(json_str);

// Parse JSON
let parsed = json.parse(json_str);
print(parsed["name"]); // "Rubolt"
```

## Time Module

The `time` module provides date and time operations.

### Functions

- `now() -> number` - Current Unix timestamp
- `now_ms() -> number` - Current timestamp in milliseconds
- `sleep(seconds: number) -> void` - Sleep for specified seconds
- `format(timestamp: number, format?: string) -> string` - Format timestamp
- `parse(date_string: string, format?: string) -> number` - Parse date string
- `year(timestamp?: number) -> number` - Get year
- `month(timestamp?: number) -> number` - Get month (1-12)
- `day(timestamp?: number) -> number` - Get day of month
- `hour(timestamp?: number) -> number` - Get hour (0-23)
- `minute(timestamp?: number) -> number` - Get minute (0-59)
- `second(timestamp?: number) -> number` - Get second (0-59)

### Example

```rubolt
import time

// Get current time
let now = time.now();
print("Timestamp: " + now);

// Format time
let formatted = time.format(now, "%Y-%m-%d %H:%M:%S");
print("Formatted: " + formatted);

// Get date components
print("Year: " + time.year(now));
print("Month: " + time.month(now));
print("Day: " + time.day(now));

// Sleep
print("Sleeping for 2 seconds...");
time.sleep(2.0);
print("Done!");
```

## HTTP Module

The `http` module provides HTTP client functionality.

### Functions

- `get(url: string) -> string` - HTTP GET request
- `post(url: string, data: string, content_type?: string) -> string` - HTTP POST
- `put(url: string, data: string, content_type?: string) -> string` - HTTP PUT
- `delete(url: string) -> string` - HTTP DELETE

### Example

```rubolt
import http
import json

// Simple GET request
let response = http.get("https://api.github.com/users/octocat");
if (response != null) {
    let user = json.parse(response);
    print("User: " + user["login"]);
}

// POST JSON data
let post_data = {"name": "test", "value": 123};
let json_data = json.stringify(post_data);
let result = http.post("https://httpbin.org/post", json_data, "application/json");
```

## High-Level Utilities

The standard library also includes high-level utility functions in `StdLib/`:

### File Utilities (`file.rbo`)

```rubolt
import "StdLib/file.rbo"

// JSON file operations
let data = {"message": "Hello"};
write_json("data.json", data);
let loaded = read_json("data.json");

// File backup
backup_file("important.txt"); // Creates important.txt.bak.{timestamp}
```

### HTTP Utilities (`http.rbo`)

```rubolt
import "StdLib/http.rbo"

// JSON API calls
let user = get_json("https://api.example.com/user/1");
let result = post_json("https://api.example.com/users", {"name": "John"});

// REST client
let client = RestClient("https://api.example.com");
client.set_header("Authorization", "Bearer token");
let users = client.get("/users");
```

## Building the Standard Library

### Windows

```bash
build_stdlib.bat
```

### Unix/Linux/macOS

```bash
chmod +x build_stdlib.sh
./build_stdlib.sh
```

## Dependencies

The standard library requires the following system libraries:

- **libcurl** - For HTTP module
- **json-c** - For JSON module
- **Standard C library** - For file and time modules

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install libcurl4-openssl-dev libjson-c-dev
```

**macOS (Homebrew):**
```bash
brew install curl json-c
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-curl mingw-w64-x86_64-json-c
```

## Module Registration

All standard library modules are automatically registered when Rubolt starts. The registration happens in `src/modules_registry.c`.

## Error Handling

Standard library functions return appropriate error values:

- File operations return `null` or `false` on failure
- JSON parsing returns `null` for invalid JSON
- HTTP requests return `null` on network errors
- Time functions return `-1` for invalid inputs

Always check return values before using results:

```rubolt
let content = file.read("nonexistent.txt");
if (content != null) {
    print("File content: " + content);
} else {
    print("Failed to read file");
}
```