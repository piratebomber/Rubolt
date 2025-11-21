===============
Getting Started
===============

This guide will help you install Rubolt, set up your development environment, and write your first programs.

.. contents:: Table of Contents
   :local:
   :depth: 2

Installation
============

Automated Installers
--------------------

**Windows (PowerShell):**

.. code-block:: powershell

   # Install to user directory
   pwsh -File Installer/install.ps1 -Mode auto -Prefix "$env:USERPROFILE\Rubolt" -AddToPath

   # Install from source with build
   pwsh -File Installer/install.ps1 -Mode source -Build -Prefix "$env:USERPROFILE\Rubolt-src" -AddToPath

**macOS/Linux:**

.. code-block:: bash

   # Install to /usr/local (may require sudo)
   bash Installer/install.sh --mode auto --prefix /usr/local --add-to-path

   # Install to user directory
   bash Installer/install.sh --mode auto --prefix ~/.local --add-to-path

Build from Source
-----------------

**Prerequisites:**

* **Windows**: MinGW-w64 or MSVC, Python 3.8+
* **macOS**: Xcode Command Line Tools, Python 3.8+
* **Linux**: GCC 8.0+, Python 3.8+, Make

**Clone and Build:**

.. code-block:: bash

   git clone https://github.com/piratebomber/Rubolt.git
   cd Rubolt

   # Windows
   build_all.bat

   # macOS/Linux
   chmod +x build_all.sh
   ./build_all.sh

Docker Installation
-------------------

.. code-block:: bash

   # Build Docker image
   docker build -t rubolt .

   # Run Rubolt in container
   docker run --rm -it -v $PWD:/work rubolt rbcli run examples/hello.rbo

VS Code Devcontainer
--------------------

1. Open the Rubolt repository in VS Code
2. Install the "Remote - Containers" extension
3. Select "Reopen in Container" when prompted
4. The development environment will be automatically configured

Verification
------------

After installation, verify Rubolt is working:

.. code-block:: bash

   # Check version
   rbcli version

   # Run hello world
   rbcli run examples/hello.rbo

   # Start REPL
   rbcli repl

Your First Program
==================

Hello World
-----------

Create a file called ``hello.rbo``:

.. code-block:: rubolt

   // hello.rbo
   print("Hello, Rubolt!");

Run it:

.. code-block:: bash

   rbcli run hello.rbo

Basic Syntax
------------

**Variables and Types:**

.. code-block:: rubolt

   // Type inference
   let name = "Alice";
   let age = 30;
   let height = 5.6;

   // Explicit types
   let count: number = 42;
   let message: string = "Hello";
   let is_active: bool = true;

**Functions:**

.. code-block:: rubolt

   def greet(name: string) -> string {
       return "Hello, " + name + "!";
   }

   def add(a: number, b: number) -> number {
       return a + b;
   }

   // Usage
   let greeting = greet("World");
   let sum = add(10, 20);
   print(greeting);
   print("Sum: " + sum);

**Control Flow:**

.. code-block:: rubolt

   // Conditionals
   if (age >= 18) {
       print("Adult");
   } else {
       print("Minor");
   }

   // Loops
   for (i in 0..10) {
       print("Count: " + i);
   }

   let numbers = [1, 2, 3, 4, 5];
   for (num in numbers) {
       print("Number: " + num);
   }

**Collections:**

.. code-block:: rubolt

   // Lists
   let fruits = ["apple", "banana", "orange"];
   fruits.append("grape");
   print("First fruit: " + fruits[0]);

   // Dictionaries
   let person = {
       "name": "Alice",
       "age": 30,
       "city": "New York"
   };
   print("Name: " + person["name"]);

   // Sets
   let unique_numbers = {1, 2, 3, 4, 5};
   unique_numbers.add(6);

Working with Files
------------------

.. code-block:: rubolt

   import file

   // Write to file
   file.write("data.txt", "Hello, File!");

   // Read from file
   let content = file.read("data.txt");
   print("File content: " + content);

   // Check if file exists
   if (file.exists("data.txt")) {
       print("File exists!");
   }

JSON Processing
---------------

.. code-block:: rubolt

   import json

   // Create data
   let data = {
       "name": "Alice",
       "age": 30,
       "hobbies": ["reading", "coding"]
   };

   // Convert to JSON string
   let json_str = json.stringify(data);
   print("JSON: " + json_str);

   // Parse JSON
   let parsed = json.parse(json_str);
   print("Name from JSON: " + parsed["name"]);

HTTP Requests
-------------

.. code-block:: rubolt

   import http
   import json

   // Simple GET request
   let response = http.get("https://api.github.com/users/octocat");
   if (response != null) {
       let user = json.parse(response);
       print("GitHub user: " + user["login"]);
   }

Advanced Features
=================

Pattern Matching
-----------------

.. code-block:: rubolt

   def describe_value(value: any) -> string {
       match value {
           null => "nothing";
           true => "yes";
           false => "no";
           x: number if x > 100 => "big number: " + x;
           x: number => "number: " + x;
           s: string => "text: " + s;
           [head, ...tail] => "list with " + (tail.length + 1) + " items";
           {name, age} => "person: " + name + " (" + age + ")";
           _ => "something else";
       }
   }

   print(describe_value(42));        // "number: 42"
   print(describe_value(150));       // "big number: 150"
   print(describe_value("hello"));   // "text: hello"

Generics
--------

.. code-block:: rubolt

   // Generic function
   def identity<T>(value: T) -> T {
       return value;
   }

   // Generic class
   class Box<T> {
       value: T;
       
       def init(value: T) {
           this.value = value;
       }
       
       def get() -> T {
           return this.value;
       }
   }

   let number_box = Box<number>(42);
   let string_box = Box<string>("hello");

Error Handling
--------------

.. code-block:: rubolt

   type Result<T, E> = Ok<T> | Error<E>;

   def safe_divide(a: number, b: number) -> Result<number, string> {
       if (b == 0) {
           return Error("Division by zero");
       }
       return Ok(a / b);
   }

   let result = safe_divide(10, 2);
   match result {
       Ok(value) => print("Result: " + value);
       Error(msg) => print("Error: " + msg);
   }

Project Management
==================

Creating a New Project
-----------------------

.. code-block:: bash

   # Create new project
   rbcli init my-project
   cd my-project

   # Project structure
   my-project/
   ├── src/
   │   └── main.rbo
   ├── tests/
   ├── package.json
   └── .rbo.config

**package.json:**

.. code-block:: json

   {
       "name": "my-project",
       "version": "1.0.0",
       "entry": "src/main.rbo",
       "dependencies": {},
       "devDependencies": {}
   }

Building and Running
--------------------

.. code-block:: bash

   # Build project
   rbcli build

   # Run project
   rbcli run

   # Run tests
   rbcli test

   # Format code
   rbcli format src/

   # Lint code
   rbcli lint src/

Package Management
------------------

.. code-block:: bash

   # Add dependency
   rbcli add http-client@2.1.0

   # Remove dependency
   rbcli remove old-package

   # Install all dependencies
   rbcli install

   # Update dependencies
   rbcli update

Development Tools
=================

Interactive REPL
-----------------

.. code-block:: bash

   rbcli repl

The REPL provides:

* **Command history** with up/down arrows
* **Tab completion** for keywords and functions
* **Multi-line input** for complex expressions
* **Variable inspection** with ``:inspect variable_name``
* **Help system** with ``:help`` or ``:help function_name``

**REPL Commands:**

.. code-block:: text

   :help           - Show help
   :quit           - Exit REPL
   :clear          - Clear screen
   :history        - Show command history
   :inspect <var>  - Inspect variable
   :type <expr>    - Show expression type
   :time <expr>    - Time expression execution

Debugger
--------

.. code-block:: bash

   rbcli debug my_program.rbo

**Debugger Commands:**

.. code-block:: text

   break <file>:<line>  - Set breakpoint
   step                 - Step into
   next                 - Step over
   continue             - Continue execution
   print <var>          - Print variable
   backtrace            - Show call stack
   quit                 - Exit debugger

**Setting Breakpoints in Code:**

.. code-block:: rubolt

   def my_function(x: number) -> number {
       breakpoint();  // Execution will pause here
       return x * 2;
   }

Profiler
--------

.. code-block:: bash

   rbcli profile my_program.rbo

Or in code:

.. code-block:: rubolt

   import profiler

   profiler.start("my_operation");
   // ... code to profile ...
   profiler.stop("my_operation");

   profiler.report();

VS Code Integration
===================

Installing the Extension
------------------------

1. Build the extension:

   .. code-block:: bash

      cd vscode-rubolt
      npm install
      npm run compile
      vsce package

2. Install in VS Code:
   
   * Open VS Code
   * Go to Extensions (Ctrl+Shift+X)
   * Click "..." → "Install from VSIX..."
   * Select the generated ``.vsix`` file

Features
--------

* **Syntax highlighting** for ``.rbo`` files
* **Code completion** with IntelliSense
* **Error diagnostics** with real-time validation
* **Debugging support** with breakpoints
* **Integrated terminal** with Rubolt REPL
* **Code formatting** and linting
* **Go to definition** and find references

**VS Code Commands:**

* ``Ctrl+Shift+P`` → "Rubolt: Run File"
* ``Ctrl+Shift+P`` → "Rubolt: Start REPL"
* ``Ctrl+Shift+P`` → "Rubolt: Format Document"

Configuration
=============

Project Configuration (.rbo.config)
------------------------------------

.. code-block:: json

   {
     "version": "1.0",
     "entry": "src/main.rbo",
     "strict": true,
     "typecheck": true,
     "optimize": true,
     "output": "build/",
     "jit": {
       "enabled": true,
       "optimization_level": 2,
       "hot_threshold": 10
     },
     "memory": {
       "gc_enabled": true,
       "gc_threshold": 1000000,
       "debug_memory": false
     }
   }

Linter Configuration (.rblint.json)
------------------------------------

.. code-block:: json

   {
     "rules": {
       "naming_convention": "snake_case",
       "max_line_length": 100,
       "max_complexity": 15,
       "require_type_annotations": true,
       "no_unused_variables": true
     },
     "ignore": ["build/", "vendor/", "*.generated.rbo"]
   }

Next Steps
==========

Now that you have Rubolt installed and understand the basics:

1. **Explore Examples**: Check out ``examples/`` for more complex programs
2. **Read the Language Reference**: :doc:`language_reference` for complete syntax
3. **Learn the Standard Library**: :doc:`stdlib` for available modules
4. **Set Up Your Editor**: Install VS Code extension for better development experience
5. **Join the Community**: Contribute to the project or ask questions on GitHub

**Useful Resources:**

* :doc:`language_reference` - Complete language documentation
* :doc:`stdlib` - Standard library reference
* :doc:`cli` - Command-line interface guide
* ``examples/complete_showcase.rbo`` - Comprehensive feature demonstration
* ``examples/production_ready_demo.rbo`` - Production-quality code examples