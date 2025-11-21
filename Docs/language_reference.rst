==================
Language Reference
==================

This document provides a complete reference for the Rubolt programming language syntax, semantics, and features.

.. contents:: Table of Contents
   :local:
   :depth: 3

Lexical Structure
=================

Comments
--------

Rubolt supports three comment styles:

.. code-block:: rubolt

   // Single-line comment
   # Shell-style comment
   
   /*
    * Multi-line comment
    * Can span multiple lines
    */

Identifiers
-----------

Identifiers must start with a letter or underscore, followed by letters, digits, or underscores:

.. code-block:: rubolt

   valid_identifier
   _private_var
   MyClass
   variable123
   __special__

Keywords
--------

Reserved keywords in Rubolt:

.. code-block:: text

   and       as        async     await     break     case      catch
   class     const     continue  def       else      enum      false
   finally   for       function  if        import    in        let
   match     null      or        return    super     this      throw
   true      try       type      void      while     yield

Literals
--------

**Numbers:**

.. code-block:: rubolt

   42          // Integer
   3.14159     // Float
   0xFF        // Hexadecimal
   0b1010      // Binary
   0o755       // Octal
   1e6         // Scientific notation
   1_000_000   // Underscores for readability

**Strings:**

.. code-block:: rubolt

   "double quoted"
   'single quoted'
   "string with \"escaped\" quotes"
   'string with \'escaped\' quotes'
   "multiline\nstring"
   "unicode: \u03B1\u03B2\u03B3"

**String Interpolation:**

.. code-block:: rubolt

   let name = "Alice";
   let age = 30;
   let message = "Hello, ${name}! You are ${age} years old.";

**Booleans:**

.. code-block:: rubolt

   true
   false

**Null:**

.. code-block:: rubolt

   null

Type System
===========

Basic Types
-----------

.. code-block:: rubolt

   // Primitive types
   let num: number = 42;
   let text: string = "hello";
   let flag: bool = true;
   let nothing: null = null;
   let anything: any = "could be anything";
   let empty: void = void;  // Only for function returns

Type Inference
--------------

Rubolt can infer types in many contexts:

.. code-block:: rubolt

   let x = 42;        // Inferred as number
   let y = "hello";   // Inferred as string
   let z = [1, 2, 3]; // Inferred as number[]

Explicit typing is required for function parameters and return types:

.. code-block:: rubolt

   def add(a: number, b: number) -> number {
       return a + b;
   }

Union Types
-----------

.. code-block:: rubolt

   type StringOrNumber = string | number;
   type Result<T> = T | null;

   def process(value: StringOrNumber) -> string {
       match value {
           s: string => "String: " + s;
           n: number => "Number: " + n;
       }
   }

Optional Types
--------------

.. code-block:: rubolt

   type Optional<T> = T | null;

   def find_user(id: number) -> Optional<User> {
       // Returns User or null
   }

Array Types
-----------

.. code-block:: rubolt

   let numbers: number[] = [1, 2, 3, 4, 5];
   let strings: string[] = ["a", "b", "c"];
   let matrix: number[][] = [[1, 2], [3, 4]];

Function Types
--------------

.. code-block:: rubolt

   type BinaryOp = function(number, number) -> number;
   type Predicate<T> = function(T) -> bool;

   def apply_op(a: number, b: number, op: BinaryOp) -> number {
       return op(a, b);
   }

   let add: BinaryOp = (x, y) => x + y;
   let result = apply_op(5, 3, add);

Generics
========

Generic Functions
-----------------

.. code-block:: rubolt

   def identity<T>(value: T) -> T {
       return value;
   }

   def swap<T, U>(pair: (T, U)) -> (U, T) {
       let (first, second) = pair;
       return (second, first);
   }

   // Usage
   let num = identity<number>(42);
   let str = identity("hello");  // Type inferred
   let swapped = swap((1, "hello"));  // Returns ("hello", 1)

Generic Classes
---------------

.. code-block:: rubolt

   class Container<T> {
       private items: T[];
       
       def init() {
           this.items = [];
       }
       
       def add(item: T) -> void {
           this.items.append(item);
       }
       
       def get(index: number) -> T {
           return this.items[index];
       }
       
       def size() -> number {
           return this.items.length;
       }
   }

   let numbers = Container<number>();
   numbers.add(42);
   numbers.add(100);

Type Constraints
----------------

.. code-block:: rubolt

   // Constraint interfaces
   interface Comparable {
       def compare(other: Self) -> number;
   }

   interface Serializable {
       def serialize() -> string;
   }

   // Constrained generics
   def sort<T: Comparable>(items: T[]) -> T[] {
       // Can call compare() on T instances
       return items.sort((a, b) => a.compare(b));
   }

   def save<T: Serializable>(item: T, filename: string) -> void {
       let data = item.serialize();
       file.write(filename, data);
   }

   // Multiple constraints
   def process<T: Comparable + Serializable>(item: T) -> void {
       // T must implement both interfaces
   }

Pattern Matching
================

Basic Patterns
--------------

.. code-block:: rubolt

   def describe(value: any) -> string {
       match value {
           null => "nothing";
           true => "yes";
           false => "no";
           42 => "the answer";
           "hello" => "greeting";
           _ => "something else";
       }
   }

Variable Binding
----------------

.. code-block:: rubolt

   match value {
       x: number => "Number: " + x;
       s: string => "String: " + s;
       _ => "Other type";
   }

Guard Patterns
--------------

.. code-block:: rubolt

   match value {
       x: number if x > 0 => "Positive: " + x;
       x: number if x < 0 => "Negative: " + x;
       x: number => "Zero";
       _ => "Not a number";
   }

Destructuring Patterns
----------------------

**Tuples:**

.. code-block:: rubolt

   match point {
       (0, 0) => "Origin";
       (x, 0) => "On X-axis at " + x;
       (0, y) => "On Y-axis at " + y;
       (x, y) => "Point at (" + x + ", " + y + ")";
   }

**Lists:**

.. code-block:: rubolt

   match list {
       [] => "Empty list";
       [x] => "Single item: " + x;
       [first, second] => "Two items: " + first + ", " + second;
       [head, ...tail] => "Head: " + head + ", tail length: " + tail.length;
   }

**Objects:**

.. code-block:: rubolt

   match person {
       {name: "Alice"} => "Hello Alice!";
       {name, age: 18} => name + " just turned 18";
       {name, age} if age >= 65 => name + " is a senior";
       {name, ...rest} => name + " with extra data";
   }

Nested Patterns
---------------

.. code-block:: rubolt

   match data {
       {users: [{name: "admin", ...}, ...rest]} => {
           return "Admin user found, " + rest.length + " others";
       };
       {users: users} if users.length > 10 => {
           return "Many users: " + users.length;
       };
       _ => "No match";
   }

Error Handling
==============

Result Types
------------

.. code-block:: rubolt

   type Result<T, E> = Ok<T> | Error<E>;

   def safe_divide(a: number, b: number) -> Result<number, string> {
       if (b == 0) {
           return Error("Division by zero");
       }
       return Ok(a / b);
   }

   // Usage with pattern matching
   let result = safe_divide(10, 2);
   match result {
       Ok(value) => print("Result: " + value);
       Error(msg) => print("Error: " + msg);
   }

Try-Catch Blocks
----------------

.. code-block:: rubolt

   try {
       let content = file.read("config.json");
       let config = json.parse(content);
       return config;
   } catch (FileNotFoundError e) {
       print("Config file not found: " + e.message);
       return default_config();
   } catch (JsonParseError e) {
       print("Invalid JSON in config: " + e.message);
       return default_config();
   } finally {
       print("Config loading attempt completed");
   }

Custom Exceptions
-----------------

.. code-block:: rubolt

   class ValidationError {
       message: string;
       field: string;
       
       def init(message: string, field: string) {
           this.message = message;
           this.field = field;
       }
   }

   def validate_user(user: dict) -> void {
       if (!user.has_key("email")) {
           throw ValidationError("Email is required", "email");
       }
       if (user["age"] < 0) {
           throw ValidationError("Age must be positive", "age");
       }
   }

Functions
=========

Function Declaration
--------------------

.. code-block:: rubolt

   def function_name(param1: Type1, param2: Type2) -> ReturnType {
       // Function body
       return value;
   }

   // Void functions
   def print_message(msg: string) -> void {
       print(msg);
   }

   // Functions without explicit return type (inferred as void)
   def log_info(info: string) {
       print("[INFO] " + info);
   }

Parameters
----------

**Default Parameters:**

.. code-block:: rubolt

   def greet(name: string, greeting: string = "Hello") -> string {
       return greeting + ", " + name + "!";
   }

   greet("Alice");              // "Hello, Alice!"
   greet("Bob", "Hi");          // "Hi, Bob!"

**Variadic Parameters:**

.. code-block:: rubolt

   def sum(...numbers: number[]) -> number {
       let total = 0;
       for (num in numbers) {
           total += num;
       }
       return total;
   }

   sum(1, 2, 3, 4, 5);  // 15

Lambda Functions
----------------

.. code-block:: rubolt

   // Arrow function syntax
   let add = (a: number, b: number) -> number => a + b;
   let square = (x: number) => x * x;

   // Multi-line lambdas
   let process = (data: string[]) => {
       let result = [];
       for (item in data) {
           result.append(item.upper());
       }
       return result;
   };

Higher-Order Functions
----------------------

.. code-block:: rubolt

   def map<T, U>(items: T[], func: function(T) -> U) -> U[] {
       let result = [];
       for (item in items) {
           result.append(func(item));
       }
       return result;
   }

   def filter<T>(items: T[], predicate: function(T) -> bool) -> T[] {
       let result = [];
       for (item in items) {
           if (predicate(item)) {
               result.append(item);
           }
       }
       return result;
   }

   // Usage
   let numbers = [1, 2, 3, 4, 5];
   let squares = map(numbers, x => x * x);
   let evens = filter(numbers, x => x % 2 == 0);

Classes and Objects
===================

Class Declaration
-----------------

.. code-block:: rubolt

   class Person {
       // Fields
       name: string;
       age: number;
       private ssn: string;  // Private field
       
       // Constructor
       def init(name: string, age: number, ssn: string) {
           this.name = name;
           this.age = age;
           this.ssn = ssn;
       }
       
       // Methods
       def greet() -> string {
           return "Hello, I'm " + this.name;
       }
       
       def get_age() -> number {
           return this.age;
       }
       
       // Private method
       private def validate_ssn() -> bool {
           return this.ssn.length == 9;
       }
   }

Inheritance
-----------

.. code-block:: rubolt

   class Employee extends Person {
       department: string;
       salary: number;
       
       def init(name: string, age: number, ssn: string, 
                department: string, salary: number) {
           super.init(name, age, ssn);
           this.department = department;
           this.salary = salary;
       }
       
       def greet() -> string {
           return super.greet() + " from " + this.department;
       }
       
       def get_annual_salary() -> number {
           return this.salary * 12;
       }
   }

Interfaces
----------

.. code-block:: rubolt

   interface Drawable {
       def draw() -> void;
       def get_bounds() -> (number, number, number, number);
   }

   interface Movable {
       def move(dx: number, dy: number) -> void;
   }

   class Rectangle implements Drawable, Movable {
       x: number;
       y: number;
       width: number;
       height: number;
       
       def draw() -> void {
           print("Drawing rectangle at (" + this.x + ", " + this.y + ")");
       }
       
       def get_bounds() -> (number, number, number, number) {
           return (this.x, this.y, this.width, this.height);
       }
       
       def move(dx: number, dy: number) -> void {
           this.x += dx;
           this.y += dy;
       }
   }

Abstract Classes
----------------

.. code-block:: rubolt

   abstract class Shape {
       x: number;
       y: number;
       
       def init(x: number, y: number) {
           this.x = x;
           this.y = y;
       }
       
       // Abstract method - must be implemented by subclasses
       abstract def area() -> number;
       
       // Concrete method
       def move(dx: number, dy: number) -> void {
           this.x += dx;
           this.y += dy;
       }
   }

   class Circle extends Shape {
       radius: number;
       
       def init(x: number, y: number, radius: number) {
           super.init(x, y);
           this.radius = radius;
       }
       
       def area() -> number {
           return 3.14159 * this.radius * this.radius;
       }
   }

Control Flow
============

Conditional Statements
----------------------

.. code-block:: rubolt

   // If-else
   if (condition) {
       // code
   } else if (other_condition) {
       // code
   } else {
       // code
   }

   // Ternary operator
   let result = condition ? value_if_true : value_if_false;

Loops
-----

**For Loops:**

.. code-block:: rubolt

   // Range-based for loop
   for (i in 0..10) {
       print(i);  // 0 to 9
   }

   for (i in 0..=10) {
       print(i);  // 0 to 10 (inclusive)
   }

   // Step
   for (i in 0..10 step 2) {
       print(i);  // 0, 2, 4, 6, 8
   }

   // Iterate over collections
   let items = ["a", "b", "c"];
   for (item in items) {
       print(item);
   }

   // With index
   for (i, item in items.enumerate()) {
       print(i + ": " + item);
   }

**While Loops:**

.. code-block:: rubolt

   let i = 0;
   while (i < 10) {
       print(i);
       i += 1;
   }

   // Do-while
   do {
       print("At least once");
   } while (false);

**Loop Control:**

.. code-block:: rubolt

   for (i in 0..100) {
       if (i % 2 == 0) {
           continue;  // Skip even numbers
       }
       if (i > 50) {
           break;     // Exit loop
       }
       print(i);
   }

Collections
===========

Lists
-----

.. code-block:: rubolt

   // Creation
   let empty: number[] = [];
   let numbers = [1, 2, 3, 4, 5];
   let mixed: any[] = [1, "hello", true];

   // Methods
   numbers.append(6);           // Add to end
   numbers.insert(0, 0);        // Insert at index
   let last = numbers.pop();    // Remove and return last
   numbers.remove(3);           // Remove first occurrence
   
   // Slicing
   let slice = numbers[1:4];    // Elements 1, 2, 3
   let from_start = numbers[:3]; // First 3 elements
   let to_end = numbers[2:];    // From index 2 to end

   // Utility methods
   numbers.sort();              // Sort in place
   numbers.reverse();           // Reverse in place
   let length = numbers.length; // Get length
   let copy = numbers.copy();   // Shallow copy

Dictionaries
------------

.. code-block:: rubolt

   // Creation
   let empty: dict<string, number> = {};
   let person = {
       "name": "Alice",
       "age": 30,
       "city": "New York"
   };

   // Access
   let name = person["name"];
   person["email"] = "alice@example.com";

   // Methods
   let keys = person.keys();       // Get all keys
   let values = person.values();   // Get all values
   let items = person.items();     // Get key-value pairs

   // Iteration
   for (key in person.keys()) {
       print(key + ": " + person[key]);
   }

   for (key, value in person.items()) {
       print(key + " = " + value);
   }

Sets
----

.. code-block:: rubolt

   // Creation
   let empty: set<number> = {};
   let numbers = {1, 2, 3, 4, 5};

   // Methods
   numbers.add(6);              // Add element
   numbers.remove(3);           // Remove element
   let has_five = numbers.has(5); // Check membership

   // Set operations
   let other = {4, 5, 6, 7};
   let union = numbers.union(other);           // {1, 2, 4, 5, 6, 7}
   let intersection = numbers.intersection(other); // {4, 5}
   let difference = numbers.difference(other);     // {1, 2}

Tuples
------

.. code-block:: rubolt

   // Creation
   let point = (10, 20);
   let person = ("Alice", 30, "Engineer");

   // Destructuring
   let (x, y) = point;
   let (name, age, job) = person;

   // Access by index
   let first = point[0];
   let second = point[1];

Modules and Imports
===================

Import Statements
-----------------

.. code-block:: rubolt

   // Import entire module
   import file;
   import json;
   import time;

   // Import with alias
   import http as web;

   // Import specific functions
   import {read, write} from file;

   // Import from subdirectory
   import "utils/math.rbo" as math_utils;

Module Definition
-----------------

**math_utils.rbo:**

.. code-block:: rubolt

   // Export functions
   export def add(a: number, b: number) -> number {
       return a + b;
   }

   export def multiply(a: number, b: number) -> number {
       return a * b;
   }

   // Export constants
   export let PI = 3.14159;
   export let E = 2.71828;

   // Private function (not exported)
   def internal_helper() -> void {
       // Only available within this module
   }

**Using the module:**

.. code-block:: rubolt

   import "math_utils.rbo" as math;

   let sum = math.add(5, 3);
   let product = math.multiply(4, 7);
   let circle_area = math.PI * radius * radius;

Async Programming
=================

Async Functions
---------------

.. code-block:: rubolt

   import async;
   import http;

   async def fetch_user(id: number) -> dict {
       let url = "https://api.example.com/users/" + id;
       let response = await http.get_async(url);
       return json.parse(response);
   }

   async def main() -> void {
       let user = await fetch_user(123);
       print("User: " + user["name"]);
   }

   // Run async function
   async.run(main());

Promises
--------

.. code-block:: rubolt

   import async;

   // Create promise
   let promise = async.Promise<string>((resolve, reject) => {
       if (some_condition) {
           resolve("Success!");
       } else {
           reject("Error occurred");
       }
   });

   // Handle promise
   promise
       .then(result => print("Got: " + result))
       .catch(error => print("Error: " + error));

Concurrent Execution
--------------------

.. code-block:: rubolt

   async def fetch_multiple() -> void {
       let tasks = [
           fetch_user(1),
           fetch_user(2),
           fetch_user(3)
       ];
       
       // Wait for all to complete
       let users = await Promise.all(tasks);
       
       for (user in users) {
           print("User: " + user["name"]);
       }
   }

   // Race - first to complete wins
   async def fetch_fastest() -> dict {
       let tasks = [
           fetch_from_server_a(),
           fetch_from_server_b(),
           fetch_from_server_c()
       ];
       
       return await Promise.race(tasks);
   }

JIT Compilation
===============

Optimization Annotations
------------------------

.. code-block:: rubolt

   // Force JIT compilation
   @jit_compile
   def matrix_multiply(a: number[][], b: number[][]) -> number[][] {
       // This function will be compiled to native code
       let result = [];
       for (i in 0..a.length) {
           result[i] = [];
           for (j in 0..b[0].length) {
               let sum = 0;
               for (k in 0..b.length) {
                   sum += a[i][k] * b[k][j];
               }
               result[i][j] = sum;
           }
       }
       return result;
   }

   // Mark as hot path for optimization
   @hot_path
   def frequently_called() -> number {
       // Will be optimized after multiple calls
       return expensive_computation();
   }

   // Always inline this function
   @inline_always
   def simple_helper(x: number) -> number {
       return x * 2 + 1;
   }

Memory Management
=================

Garbage Collection Control
--------------------------

.. code-block:: rubolt

   // Disable GC for performance-critical section
   @no_gc
   def performance_critical() -> void {
       let buffer = allocate_large_buffer();
       // ... performance critical code ...
       free_buffer(buffer);
   }

   // Manual GC trigger
   gc.collect();

   // GC statistics
   let stats = gc.get_stats();
   print("Collections: " + stats.collections);
   print("Memory freed: " + stats.bytes_freed);

Memory Debugging
----------------

.. code-block:: rubolt

   // Enable memory debugging
   enable_memory_debugging();

   // Your code here
   let data = create_large_structure();

   // Check for leaks
   print_memory_leaks();
   disable_memory_debugging();

Operators
=========

Arithmetic Operators
--------------------

.. code-block:: rubolt

   let a = 10;
   let b = 3;

   a + b    // Addition: 13
   a - b    // Subtraction: 7
   a * b    // Multiplication: 30
   a / b    // Division: 3.333...
   a % b    // Modulo: 1
   a ** b   // Exponentiation: 1000

Comparison Operators
--------------------

.. code-block:: rubolt

   a == b   // Equal: false
   a != b   // Not equal: true
   a < b    // Less than: false
   a <= b   // Less than or equal: false
   a > b    // Greater than: true
   a >= b   // Greater than or equal: true

Logical Operators
-----------------

.. code-block:: rubolt

   true && false   // Logical AND: false
   true || false   // Logical OR: true
   !true          // Logical NOT: false

Assignment Operators
--------------------

.. code-block:: rubolt

   let x = 10;
   x += 5;    // x = x + 5: 15
   x -= 3;    // x = x - 3: 12
   x *= 2;    // x = x * 2: 24
   x /= 4;    // x = x / 4: 6
   x %= 5;    // x = x % 5: 1

Bitwise Operators
-----------------

.. code-block:: rubolt

   let a = 0b1010;  // 10
   let b = 0b1100;  // 12

   a & b    // Bitwise AND: 0b1000 (8)
   a | b    // Bitwise OR: 0b1110 (14)
   a ^ b    // Bitwise XOR: 0b0110 (6)
   ~a       // Bitwise NOT: 0b...0101 (-11)
   a << 2   // Left shift: 0b101000 (40)
   a >> 1   // Right shift: 0b0101 (5)

Operator Overloading
--------------------

.. code-block:: rubolt

   class Vector {
       x: number;
       y: number;
       
       def init(x: number, y: number) {
           this.x = x;
           this.y = y;
       }
       
       // Overload + operator
       def __add__(other: Vector) -> Vector {
           return Vector(this.x + other.x, this.y + other.y);
       }
       
       // Overload == operator
       def __eq__(other: Vector) -> bool {
           return this.x == other.x && this.y == other.y;
       }
       
       // Overload string conversion
       def __str__() -> string {
           return "Vector(" + this.x + ", " + this.y + ")";
       }
   }

   let v1 = Vector(1, 2);
   let v2 = Vector(3, 4);
   let v3 = v1 + v2;  // Uses __add__
   print(v3);         // Uses __str__

Type Aliases
============

.. code-block:: rubolt

   // Simple type alias
   type UserId = number;
   type UserName = string;

   // Generic type alias
   type Result<T> = T | null;
   type Callback<T> = function(T) -> void;

   // Complex type alias
   type ApiResponse<T> = {
       data: T;
       status: number;
       message: string;
   };

   // Usage
   def get_user(id: UserId) -> Result<User> {
       // Implementation
   }

   def process_response<T>(response: ApiResponse<T>) -> T {
       if (response.status == 200) {
           return response.data;
       }
       throw Error(response.message);
   }

Enums
=====

.. code-block:: rubolt

   // Simple enum
   enum Color {
       Red,
       Green,
       Blue
   }

   // Enum with values
   enum HttpStatus {
       Ok = 200,
       NotFound = 404,
       InternalError = 500
   }

   // String enum
   enum Direction {
       North = "north",
       South = "south",
       East = "east",
       West = "west"
   }

   // Usage
   let color = Color.Red;
   let status = HttpStatus.Ok;
   
   match color {
       Color.Red => print("Red color");
       Color.Green => print("Green color");
       Color.Blue => print("Blue color");
   }

Attributes and Decorators
=========================

.. code-block:: rubolt

   // Function attributes
   @deprecated("Use new_function instead")
   def old_function() -> void {
       // Implementation
   }

   @test
   def test_addition() -> void {
       assert(2 + 2 == 4);
   }

   // Class attributes
   @serializable
   class User {
       @required
       name: string;
       
       @optional
       email: string;
       
       @private
       password_hash: string;
   }

   // Custom attributes
   @cache(ttl=300)  // Cache for 5 minutes
   def expensive_computation(input: string) -> string {
       // Expensive operation
       return result;
   }

Compile-time Features
=====================

Macros
------

.. code-block:: rubolt

   // Simple macro
   macro debug_print(expr) {
       if (DEBUG_MODE) {
           print("DEBUG: " + stringify(expr) + " = " + expr);
       }
   }

   // Usage
   let x = 42;
   debug_print(x);  // Expands to debug print if DEBUG_MODE is true

Conditional Compilation
-----------------------

.. code-block:: rubolt

   #if DEBUG
   def debug_info() -> void {
       print("Debug information");
   }
   #endif

   #if PLATFORM == "windows"
   import "windows_specific.rbo";
   #elif PLATFORM == "linux"
   import "linux_specific.rbo";
   #endif

Static Assertions
-----------------

.. code-block:: rubolt

   // Compile-time assertions
   static_assert(sizeof(int) == 4, "Expected 32-bit integers");
   static_assert(VERSION >= 100, "Minimum version requirement not met");

Best Practices
==============

Code Style
----------

1. **Use meaningful names:**

   .. code-block:: rubolt

      // Good
      def calculate_monthly_payment(principal: number, rate: number, months: number) -> number

      // Bad
      def calc(p: number, r: number, m: number) -> number

2. **Prefer explicit types for public APIs:**

   .. code-block:: rubolt

      // Good - clear interface
      def process_user_data(data: UserData) -> ProcessingResult

      // Avoid - unclear what's expected
      def process_user_data(data: any) -> any

3. **Use pattern matching for complex conditionals:**

   .. code-block:: rubolt

      // Good
      match response {
          Ok(data) => process_data(data);
          Error(msg) => log_error(msg);
      }

      // Less clear
      if (response.is_ok()) {
          process_data(response.unwrap());
      } else {
          log_error(response.error());
      }

Error Handling
--------------

1. **Use Result types for operations that can fail:**

   .. code-block:: rubolt

      def parse_config(path: string) -> Result<Config, string> {
          // Implementation
      }

2. **Handle errors at appropriate levels:**

   .. code-block:: rubolt

      // Handle specific errors where you can recover
      match parse_config("config.json") {
          Ok(config) => use_config(config);
          Error(msg) => {
              log_warning("Config error: " + msg);
              use_default_config();
          };
      }

Performance
-----------

1. **Use JIT annotations for hot paths:**

   .. code-block:: rubolt

      @hot_path
      def inner_loop_function() -> number {
          // Performance-critical code
      }

2. **Prefer immutable data structures when possible:**

   .. code-block:: rubolt

      // Good - immutable
      def add_item(list: T[], item: T) -> T[] {
          return list + [item];
      }

      // Use mutable only when necessary
      def add_item_inplace(list: T[], item: T) -> void {
          list.append(item);
      }

This completes the comprehensive language reference for Rubolt. For more specific information about standard library functions, see :doc:`stdlib`, and for development tools, see :doc:`cli`.