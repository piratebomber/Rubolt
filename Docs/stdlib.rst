==================
Standard Library
==================

The Rubolt standard library provides essential modules for file I/O, JSON processing, HTTP requests, time operations, and more. This document provides a comprehensive reference for all available modules and functions.

.. contents:: Table of Contents
   :local:
   :depth: 3

Core Modules
============

File Module
-----------

The ``file`` module provides comprehensive file system operations with error handling.

**Import:**

.. code-block:: rubolt

   import file

**Functions:**

``read(path: string) -> string``
  Read entire file content as a string. Returns ``null`` if file doesn't exist or can't be read.

  .. code-block:: rubolt

     let content = file.read("data.txt");
     if (content != null) {
         print("File content: " + content);
     }

``write(path: string, content: string) -> bool``
  Write content to file, creating or overwriting as needed. Returns ``true`` on success.

  .. code-block:: rubolt

     let success = file.write("output.txt", "Hello, World!");
     if (success) {
         print("File written successfully");
     }

``append(path: string, content: string) -> bool``
  Append content to the end of a file. Creates file if it doesn't exist.

  .. code-block:: rubolt

     file.append("log.txt", "New log entry\n");

``exists(path: string) -> bool``
  Check if a file or directory exists.

  .. code-block:: rubolt

     if (file.exists("config.json")) {
         let config = json.parse(file.read("config.json"));
     }

``size(path: string) -> number``
  Get file size in bytes. Returns ``-1`` if file doesn't exist.

  .. code-block:: rubolt

     let size = file.size("data.txt");
     print("File size: " + size + " bytes");

``delete(path: string) -> bool``
  Delete a file. Returns ``true`` on success.

  .. code-block:: rubolt

     if (file.delete("temp.txt")) {
         print("Temporary file deleted");
     }

``copy(src: string, dst: string) -> bool``
  Copy a file from source to destination.

  .. code-block:: rubolt

     file.copy("important.txt", "backup.txt");

``readlines(path: string) -> string[]``
  Read file and return array of lines (without newline characters).

  .. code-block:: rubolt

     let lines = file.readlines("data.csv");
     for (line in lines) {
         let fields = line.split(",");
         process_csv_row(fields);
     }

**Advanced File Operations:**

.. code-block:: rubolt

   // Check file permissions
   if (file.is_readable("config.json")) {
       let config = file.read("config.json");
   }

   // Get file metadata
   let info = file.stat("document.pdf");
   print("Modified: " + time.format(info.modified_time));
   print("Size: " + info.size + " bytes");

JSON Module
-----------

The ``json`` module provides fast JSON parsing and serialization with comprehensive error handling.

**Import:**

.. code-block:: rubolt

   import json

**Functions:**

``parse(json_string: string) -> any``
  Parse JSON string to Rubolt value. Returns ``null`` for invalid JSON.

  .. code-block:: rubolt

     let data = json.parse('{"name": "Alice", "age": 30}');
     if (data != null) {
         print("Name: " + data["name"]);
         print("Age: " + data["age"]);
     }

``stringify(value: any) -> string``
  Convert Rubolt value to JSON string. Handles nested objects and arrays.

  .. code-block:: rubolt

     let person = {
         "name": "Bob",
         "age": 25,
         "hobbies": ["reading", "coding", "music"]
     };
     let json_str = json.stringify(person);
     file.write("person.json", json_str);

**Advanced JSON Operations:**

.. code-block:: rubolt

   // Pretty-print JSON with indentation
   let formatted = json.stringify(data, {indent: 2});

   // Parse with error handling
   try {
       let config = json.parse(file.read("config.json"));
       return config;
   } catch (JsonParseError e) {
       print("Invalid JSON: " + e.message);
       return default_config();
   }

   // Validate JSON schema
   let schema = {
       "type": "object",
       "required": ["name", "email"],
       "properties": {
           "name": {"type": "string"},
           "email": {"type": "string", "format": "email"}
       }
   };
   
   if (json.validate(data, schema)) {
       process_user_data(data);
   }

HTTP Module
-----------

The ``http`` module provides a full-featured HTTP client with support for all HTTP methods, headers, and async operations.

**Import:**

.. code-block:: rubolt

   import http

**Functions:**

``get(url: string) -> string``
  Perform HTTP GET request. Returns response body or ``null`` on error.

  .. code-block:: rubolt

     let response = http.get("https://api.github.com/users/octocat");
     if (response != null) {
         let user = json.parse(response);
         print("GitHub user: " + user["login"]);
     }

``post(url: string, data: string, content_type?: string) -> string``
  Perform HTTP POST request with data.

  .. code-block:: rubolt

     let user_data = {"name": "Alice", "email": "alice@example.com"};
     let json_data = json.stringify(user_data);
     let response = http.post("https://api.example.com/users", 
                              json_data, 
                              "application/json");

``put(url: string, data: string, content_type?: string) -> string``
  Perform HTTP PUT request.

``delete(url: string) -> string``
  Perform HTTP DELETE request.

**Advanced HTTP Operations:**

.. code-block:: rubolt

   // Custom headers
   let headers = {
       "Authorization": "Bearer " + token,
       "User-Agent": "Rubolt-App/1.0"
   };
   let response = http.get("https://api.example.com/data", {headers: headers});

   // Async HTTP requests
   async def fetch_multiple_urls() -> dict[] {
       let urls = [
           "https://api1.example.com/data",
           "https://api2.example.com/data",
           "https://api3.example.com/data"
       ];
       
       let tasks = [];
       for (url in urls) {
           tasks.append(http.get_async(url));
       }
       
       let responses = await Promise.all(tasks);
       return responses.map(r => json.parse(r));
   }

   // REST client class
   class ApiClient {
       base_url: string;
       default_headers: dict;
       
       def init(base_url: string) {
           this.base_url = base_url;
           this.default_headers = {
               "Content-Type": "application/json",
               "Accept": "application/json"
           };
       }
       
       def get<T>(endpoint: string) -> Result<T, string> {
           let url = this.base_url + endpoint;
           let response = http.get(url, {headers: this.default_headers});
           
           if (response == null) {
               return Error("Network request failed");
           }
           
           try {
               let data = json.parse(response);
               return Ok(data);
           } catch (JsonParseError e) {
               return Error("Invalid JSON response");
           }
       }
   }

Time Module
-----------

The ``time`` module provides comprehensive date and time operations with formatting and parsing capabilities.

**Import:**

.. code-block:: rubolt

   import time

**Functions:**

``now() -> number``
  Get current Unix timestamp (seconds since epoch).

  .. code-block:: rubolt

     let timestamp = time.now();
     print("Current time: " + timestamp);

``now_ms() -> number``
  Get current timestamp in milliseconds.

``sleep(seconds: number) -> void``
  Sleep for specified number of seconds (supports fractional seconds).

  .. code-block:: rubolt

     print("Starting...");
     time.sleep(2.5);  // Sleep for 2.5 seconds
     print("Done!");

``format(timestamp: number, format?: string) -> string``
  Format timestamp as string. Default format is ISO 8601.

  .. code-block:: rubolt

     let now = time.now();
     let iso = time.format(now);                    // "2024-01-15T14:30:00Z"
     let custom = time.format(now, "%Y-%m-%d %H:%M:%S");  // "2024-01-15 14:30:00"
     let readable = time.format(now, "%B %d, %Y");        // "January 15, 2024"

``parse(date_string: string, format?: string) -> number``
  Parse date string to timestamp.

  .. code-block:: rubolt

     let timestamp = time.parse("2024-01-15 14:30:00", "%Y-%m-%d %H:%M:%S");
     let iso_timestamp = time.parse("2024-01-15T14:30:00Z");

**Date Component Functions:**

``year(timestamp?: number) -> number``
  Get year (current time if no timestamp provided).

``month(timestamp?: number) -> number``
  Get month (1-12).

``day(timestamp?: number) -> number``
  Get day of month (1-31).

``hour(timestamp?: number) -> number``
  Get hour (0-23).

``minute(timestamp?: number) -> number``
  Get minute (0-59).

``second(timestamp?: number) -> number``
  Get second (0-59).

``weekday(timestamp?: number) -> number``
  Get day of week (0=Sunday, 6=Saturday).

**Advanced Time Operations:**

.. code-block:: rubolt

   // Date arithmetic
   let now = time.now();
   let tomorrow = now + (24 * 60 * 60);  // Add 24 hours
   let next_week = time.add_days(now, 7);
   let last_month = time.add_months(now, -1);

   // Time zones
   let utc_time = time.now();
   let local_time = time.to_local(utc_time);
   let ny_time = time.to_timezone(utc_time, "America/New_York");

   // Duration calculations
   let start = time.now();
   // ... some operation ...
   let end = time.now();
   let duration = end - start;
   print("Operation took " + duration + " seconds");

   // Relative time formatting
   let past_time = time.now() - (2 * 60 * 60);  // 2 hours ago
   print(time.relative(past_time));  // "2 hours ago"

String Module
-------------

The ``string`` module provides advanced string manipulation functions beyond basic operations.

**Import:**

.. code-block:: rubolt

   import string

**Functions:**

``len(s: string) -> number``
  Get string length (number of characters).

``upper(s: string) -> string``
  Convert to uppercase.

``lower(s: string) -> string``
  Convert to lowercase.

``trim(s: string) -> string``
  Remove whitespace from both ends.

``split(s: string, delimiter: string) -> string[]``
  Split string by delimiter.

``join(parts: string[], separator: string) -> string``
  Join array of strings with separator.

``replace(s: string, old: string, new: string) -> string``
  Replace all occurrences of old with new.

``starts_with(s: string, prefix: string) -> bool``
  Check if string starts with prefix.

``ends_with(s: string, suffix: string) -> bool``
  Check if string ends with suffix.

``contains(s: string, substring: string) -> bool``
  Check if string contains substring.

**Advanced String Operations:**

.. code-block:: rubolt

   // Regular expressions
   let pattern = string.regex(r"\d{3}-\d{2}-\d{4}");  // SSN pattern
   let matches = string.find_all("123-45-6789 and 987-65-4321", pattern);

   // String formatting
   let template = "Hello, {name}! You have {count} messages.";
   let message = string.format(template, {name: "Alice", count: 5});

   // Case conversions
   let title = string.title_case("hello world");      // "Hello World"
   let camel = string.camel_case("hello_world");       // "helloWorld"
   let snake = string.snake_case("HelloWorld");        // "hello_world"

   // String validation
   let is_email = string.is_email("user@example.com");
   let is_url = string.is_url("https://example.com");
   let is_numeric = string.is_numeric("12345");

Math Module
-----------

The ``math`` module provides mathematical functions and constants.

**Import:**

.. code-block:: rubolt

   import math

**Constants:**

.. code-block:: rubolt

   math.PI     // 3.14159265358979323846
   math.E      // 2.71828182845904523536
   math.TAU    // 6.28318530717958647692 (2 * PI)

**Functions:**

``abs(x: number) -> number``
  Absolute value.

``sqrt(x: number) -> number``
  Square root.

``pow(base: number, exponent: number) -> number``
  Power function.

``floor(x: number) -> number``
  Round down to nearest integer.

``ceil(x: number) -> number``
  Round up to nearest integer.

``round(x: number) -> number``
  Round to nearest integer.

**Trigonometric Functions:**

.. code-block:: rubolt

   math.sin(x)     // Sine
   math.cos(x)     // Cosine
   math.tan(x)     // Tangent
   math.asin(x)    // Arc sine
   math.acos(x)    // Arc cosine
   math.atan(x)    // Arc tangent
   math.atan2(y, x) // Arc tangent of y/x

**Logarithmic Functions:**

.. code-block:: rubolt

   math.log(x)      // Natural logarithm
   math.log10(x)    // Base-10 logarithm
   math.log2(x)     // Base-2 logarithm
   math.exp(x)      // e^x

**Advanced Math:**

.. code-block:: rubolt

   // Statistics
   let numbers = [1, 2, 3, 4, 5];
   let mean = math.mean(numbers);
   let median = math.median(numbers);
   let std_dev = math.std_deviation(numbers);

   // Random numbers
   let random_int = math.random_int(1, 100);      // Random integer 1-100
   let random_float = math.random_float(0.0, 1.0); // Random float 0.0-1.0
   let random_choice = math.choice(["a", "b", "c"]); // Random element

Random Module
-------------

The ``random`` module provides random number generation and sampling functions.

**Import:**

.. code-block:: rubolt

   import random

**Functions:**

``int(min: number, max: number) -> number``
  Generate random integer in range [min, max].

``float(min: number, max: number) -> number``
  Generate random float in range [min, max).

``choice<T>(items: T[]) -> T``
  Choose random element from array.

``shuffle<T>(items: T[]) -> T[]``
  Return shuffled copy of array.

``sample<T>(items: T[], count: number) -> T[]``
  Return random sample of specified size.

**Advanced Random Operations:**

.. code-block:: rubolt

   // Seeded random for reproducible results
   random.seed(12345);
   let value1 = random.int(1, 100);
   
   random.seed(12345);  // Same seed
   let value2 = random.int(1, 100);  // Same as value1

   // Weighted random choice
   let items = ["apple", "banana", "cherry"];
   let weights = [0.5, 0.3, 0.2];
   let choice = random.weighted_choice(items, weights);

   // Random string generation
   let random_id = random.string(8, "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

Concurrency Modules
===================

Threading Module
----------------

The ``threading`` module provides thread-based parallelism with thread pools and synchronization primitives.

**Import:**

.. code-block:: rubolt

   import threading

**Thread Pool:**

.. code-block:: rubolt

   def worker_function(data: any) -> any {
       // CPU-intensive work
       return process_data(data);
   }

   // Create thread pool
   let pool = threading.ThreadPool(4);  // 4 worker threads

   // Submit tasks
   let futures = [];
   for (item in large_dataset) {
       let future = pool.submit(worker_function, item);
       futures.append(future);
   }

   // Collect results
   let results = [];
   for (future in futures) {
       results.append(future.get());  // Blocks until result available
   }

   // Cleanup
   pool.shutdown();

**Thread Synchronization:**

.. code-block:: rubolt

   // Mutex for thread-safe access
   let mutex = threading.Mutex();
   let shared_counter = 0;

   def increment_counter() -> void {
       mutex.lock();
       shared_counter += 1;
       mutex.unlock();
   }

   // Or use with statement for automatic unlock
   def increment_counter_safe() -> void {
       with (mutex) {
           shared_counter += 1;
       }
   }

   // Condition variables
   let condition = threading.Condition();
   let ready = false;

   def producer() -> void {
       // Produce data
       with (condition) {
           ready = true;
           condition.notify_all();
       }
   }

   def consumer() -> void {
       with (condition) {
           while (!ready) {
               condition.wait();
           }
           // Consume data
       }
   }

Atomics Module
--------------

The ``atomics`` module provides atomic operations for lock-free programming.

**Import:**

.. code-block:: rubolt

   import atomics

**Atomic Types:**

.. code-block:: rubolt

   // Atomic integer
   let counter = atomics.AtomicInt(0);
   
   counter.fetch_add(1);        // Atomically add 1, return old value
   counter.fetch_sub(1);        // Atomically subtract 1
   counter.load();              // Read current value
   counter.store(42);           // Set value
   
   // Compare and swap
   let old_value = 10;
   let new_value = 20;
   let success = counter.compare_and_swap(old_value, new_value);

   // Atomic boolean
   let flag = atomics.AtomicBool(false);
   flag.store(true);
   let was_set = flag.exchange(false);  // Set to false, return old value

**Lock-free Data Structures:**

.. code-block:: rubolt

   // Lock-free queue
   let queue = atomics.LockFreeQueue<string>();
   
   // Producer thread
   queue.enqueue("item1");
   queue.enqueue("item2");
   
   // Consumer thread
   let item = queue.dequeue();  // Returns null if empty

Async Module
------------

The ``async`` module provides async/await functionality with event loops and promises.

**Import:**

.. code-block:: rubolt

   import async

**Basic Async/Await:**

.. code-block:: rubolt

   async def fetch_data(url: string) -> string {
       let response = await http.get_async(url);
       return response;
   }

   async def main() -> void {
       let data = await fetch_data("https://api.example.com/data");
       print("Received: " + data);
   }

   // Run async function
   async.run(main());

**Promises:**

.. code-block:: rubolt

   // Create promise
   let promise = async.Promise<string>((resolve, reject) => {
       // Async operation
       if (success) {
           resolve("Success!");
       } else {
           reject("Error occurred");
       }
   });

   // Handle promise
   promise
       .then(result => print("Got: " + result))
       .catch(error => print("Error: " + error))
       .finally(() => print("Cleanup"));

**Concurrent Execution:**

.. code-block:: rubolt

   async def fetch_multiple() -> dict[] {
       let tasks = [
           fetch_data("https://api1.example.com"),
           fetch_data("https://api2.example.com"),
           fetch_data("https://api3.example.com")
       ];
       
       // Wait for all to complete
       let results = await Promise.all(tasks);
       return results.map(r => json.parse(r));
   }

   // Race - first to complete wins
   async def fetch_fastest() -> string {
       let tasks = [
           fetch_from_server_a(),
           fetch_from_server_b()
       ];
       
       return await Promise.race(tasks);
   }

Testing Module
==============

Test Module
-----------

The ``test`` module provides a comprehensive testing framework with unit tests, property-based testing, and benchmarks.

**Import:**

.. code-block:: rubolt

   import test

**Unit Tests:**

.. code-block:: rubolt

   test.suite("Math Operations", () => {
       test.case("Addition", () => {
           assert.equal(2 + 2, 4, "Basic addition");
           assert.equal(0 + 5, 5, "Addition with zero");
           assert.not_equal(2 + 2, 5, "Should not equal 5");
       });
       
       test.case("Division", () => {
           assert.equal(10 / 2, 5, "Basic division");
           assert.throws(() => 1 / 0, "DivisionByZeroError");
           assert.approximately(10 / 3, 3.333, 0.001, "Approximate division");
       });
   });

**Assertions:**

.. code-block:: rubolt

   // Basic assertions
   assert.true(condition, "Should be true");
   assert.false(condition, "Should be false");
   assert.equal(actual, expected, "Should be equal");
   assert.not_equal(actual, unexpected, "Should not be equal");
   assert.null(value, "Should be null");
   assert.not_null(value, "Should not be null");

   // Numeric assertions
   assert.approximately(actual, expected, tolerance, "Should be close");
   assert.greater_than(actual, threshold, "Should be greater");
   assert.less_than(actual, threshold, "Should be less");

   // Collection assertions
   assert.contains(collection, item, "Should contain item");
   assert.length(collection, expected_length, "Should have length");
   assert.empty(collection, "Should be empty");

   // Exception assertions
   assert.throws(() => risky_operation(), "ExpectedError");
   assert.no_throw(() => safe_operation(), "Should not throw");

**Property-Based Testing:**

.. code-block:: rubolt

   // Test properties with generated data
   test.property("Addition is commutative",
       test.generate.int(-1000, 1000),
       test.generate.int(-1000, 1000),
       (a, b) => a + b == b + a
   );

   test.property("String concatenation length",
       test.generate.string(0, 100),
       test.generate.string(0, 100),
       (s1, s2) => (s1 + s2).length == s1.length + s2.length
   );

   // Custom generators
   let user_generator = test.generate.object({
       name: test.generate.string(1, 50),
       age: test.generate.int(0, 120),
       email: test.generate.email()
   });

   test.property("User validation",
       user_generator,
       (user) => validate_user(user) || user.age < 0
   );

**Benchmarks:**

.. code-block:: rubolt

   test.benchmark("Fibonacci calculation", () => {
       fibonacci(30);
   }, {
       iterations: 100,
       warmup: 10,
       timeout: 5000
   });

   test.benchmark("Array sorting", () => {
       let arr = generate_random_array(1000);
       arr.sort();
   });

**Mocking:**

.. code-block:: rubolt

   // Create mock object
   let mock_api = test.mock();
   
   // Set expectations
   mock_api.expect("get_user", [123], {name: "John", id: 123});
   mock_api.expect("update_user", [123, {name: "Jane"}], true);
   
   // Use mock in tests
   let user = mock_api.get_user(123);
   assert.equal(user.name, "John");
   
   let success = mock_api.update_user(123, {name: "Jane"});
   assert.true(success);
   
   // Verify all expectations were met
   assert.verify(mock_api);

Utility Modules
===============

OS Module
---------

The ``os`` module provides operating system interface functions.

**Import:**

.. code-block:: rubolt

   import os

**Functions:**

.. code-block:: rubolt

   // Environment variables
   let home = os.getenv("HOME");
   os.setenv("MY_VAR", "value");

   // Current directory
   let cwd = os.getcwd();
   os.chdir("/path/to/directory");

   // Execute system commands
   let result = os.system("ls -la");
   let output = os.execute("git status");

   // Path operations
   let joined = os.path.join("home", "user", "documents");
   let dirname = os.path.dirname("/path/to/file.txt");
   let basename = os.path.basename("/path/to/file.txt");
   let extension = os.path.extension("file.txt");

Sys Module
----------

The ``sys`` module provides system-specific parameters and functions.

**Import:**

.. code-block:: rubolt

   import sys

**Functions and Properties:**

.. code-block:: rubolt

   // Version information
   print("Rubolt version: " + sys.version);
   print("Platform: " + sys.platform);

   // Command line arguments
   for (arg in sys.argv) {
       print("Argument: " + arg);
   }

   // Exit program
   sys.exit(0);  // Exit with code 0 (success)
   sys.exit(1);  // Exit with code 1 (error)

   // Memory information
   let memory_usage = sys.get_memory_usage();
   print("Memory used: " + memory_usage.used + " bytes");

Crypto Module
-------------

The ``crypto`` module provides cryptographic functions for hashing, encryption, and secure random generation.

**Import:**

.. code-block:: rubolt

   import crypto

**Hashing:**

.. code-block:: rubolt

   // Hash functions
   let md5_hash = crypto.md5("hello world");
   let sha1_hash = crypto.sha1("hello world");
   let sha256_hash = crypto.sha256("hello world");

   // HMAC
   let hmac = crypto.hmac_sha256("secret_key", "message");

**Encryption:**

.. code-block:: rubolt

   // AES encryption
   let key = crypto.generate_key(256);  // 256-bit key
   let encrypted = crypto.aes_encrypt("sensitive data", key);
   let decrypted = crypto.aes_decrypt(encrypted, key);

   // RSA encryption
   let (public_key, private_key) = crypto.generate_rsa_keypair(2048);
   let encrypted_rsa = crypto.rsa_encrypt("message", public_key);
   let decrypted_rsa = crypto.rsa_decrypt(encrypted_rsa, private_key);

**Secure Random:**

.. code-block:: rubolt

   // Cryptographically secure random
   let secure_bytes = crypto.random_bytes(32);
   let secure_string = crypto.random_string(16);
   let uuid = crypto.generate_uuid();

High-Level Utilities
====================

StdLib High-Level Modules
--------------------------

The ``StdLib/`` directory contains high-level Rubolt modules that build on the native modules.

**File Utilities (StdLib/file.rbo):**

.. code-block:: rubolt

   import "StdLib/file.rbo" as file_utils

   // JSON file operations
   let data = {message: "Hello"};
   file_utils.write_json("data.json", data);
   let loaded = file_utils.read_json("data.json");

   // File backup with timestamp
   file_utils.backup_file("important.txt");  // Creates important.txt.bak.{timestamp}

   // Directory operations
   file_utils.ensure_directory("logs/2024/01");
   let files = file_utils.list_files("*.rbo", recursive=true);

**HTTP Utilities (StdLib/http.rbo):**

.. code-block:: rubolt

   import "StdLib/http.rbo" as http_utils

   // JSON API calls
   let user = http_utils.get_json("https://api.example.com/user/1");
   let result = http_utils.post_json("https://api.example.com/users", {name: "John"});

   // REST client with automatic retry
   let client = http_utils.RestClient("https://api.example.com");
   client.set_header("Authorization", "Bearer " + token);
   client.set_retry_policy({max_attempts: 3, backoff: "exponential"});
   
   let users = client.get("/users");
   let new_user = client.post("/users", user_data);

**Configuration Management:**

.. code-block:: rubolt

   import "StdLib/config.rbo" as config

   // Load configuration from multiple sources
   let cfg = config.load([
       "config.json",           // JSON config
       "config.yaml",           // YAML config  
       "environment",           // Environment variables
       "command_line"           // Command line arguments
   ]);

   // Access with defaults
   let port = cfg.get("server.port", 8080);
   let debug = cfg.get("debug", false);

**Logging:**

.. code-block:: rubolt

   import "StdLib/logging.rbo" as logging

   // Configure logging
   logging.configure({
       level: "INFO",
       format: "[{timestamp}] {level}: {message}",
       outputs: ["console", "file:app.log"]
   });

   // Use logger
   let logger = logging.get_logger("MyApp");
   logger.info("Application started");
   logger.error("An error occurred", {error_code: 500});
   logger.debug("Debug information", {user_id: 123});

Error Handling Patterns
========================

The standard library follows consistent error handling patterns:

**Result Types:**

.. code-block:: rubolt

   // Many functions return Result<T, E> for explicit error handling
   let result = file.read_safe("config.json");
   match result {
       Ok(content) => {
           let config = json.parse(content);
           // Process config
       };
       Error(msg) => {
           print("Failed to read config: " + msg);
           use_default_config();
       };
   }

**Null Returns:**

.. code-block:: rubolt

   // Simple functions return null on failure
   let content = file.read("optional_file.txt");
   if (content != null) {
       process_content(content);
   }

**Exceptions:**

.. code-block:: rubolt

   // Some operations throw exceptions for unrecoverable errors
   try {
       let data = json.parse(malformed_json);
   } catch (JsonParseError e) {
       print("JSON parsing failed: " + e.message);
   }

Performance Considerations
==========================

**JIT-Optimized Functions:**

Many standard library functions are marked for JIT optimization:

.. code-block:: rubolt

   // These functions benefit from JIT compilation
   @jit_compile
   def process_large_array(data: number[]) -> number[] {
       return data.map(x => math.sqrt(x * x + 1));
   }

**Memory Management:**

.. code-block:: rubolt

   // Large operations can disable GC temporarily
   @no_gc
   def process_huge_dataset(data: any[]) -> any[] {
       // Process without GC interruption
       let result = expensive_processing(data);
       gc.collect();  // Manual collection when done
       return result;
   }

**Async Operations:**

.. code-block:: rubolt

   // Use async versions for I/O-bound operations
   async def process_urls(urls: string[]) -> dict[] {
       let tasks = urls.map(url => http.get_async(url));
       let responses = await Promise.all(tasks);
       return responses.map(r => json.parse(r));
   }

Building Custom Modules
========================

You can extend the standard library by creating custom modules:

**Native Module (C):**

.. code-block:: c

   // Modules/mymodule_mod.c
   #include "../src/module.h"

   static Value my_function(Environment *env, Value *args, size_t arg_count) {
       // Implementation
       return value_number(42);
   }

   void register_mymodule(ModuleRegistry *registry) {
       Module *mod = module_create("mymodule");
       module_add_function(mod, "my_function", my_function);
       module_registry_add(registry, mod);
   }

**Pure Rubolt Module:**

.. code-block:: rubolt

   // MyModule/utils.rbo
   export def helper_function(x: number) -> number {
       return x * 2 + 1;
   }

   export class MyClass {
       value: number;
       
       def init(value: number) {
           this.value = value;
       }
   }

This completes the comprehensive standard library reference. For more information about specific modules or functions, refer to the inline documentation in the source code or use the REPL's help system.