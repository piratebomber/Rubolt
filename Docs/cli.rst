=======================
Command Line Interface
=======================

The Rubolt CLI (``rbcli``) is the primary tool for managing Rubolt projects, running programs, and accessing development tools. This document provides a comprehensive reference for all CLI commands and options.

.. contents:: Table of Contents
   :local:
   :depth: 3

Overview
========

The Rubolt CLI provides a unified interface for:

* **Project Management**: Creating, building, and managing Rubolt projects
* **Code Execution**: Running Rubolt programs and scripts
* **Development Tools**: Accessing debugger, profiler, formatter, and linter
* **Package Management**: Installing and managing dependencies
* **Testing**: Running test suites and benchmarks

Installation
============

The CLI is installed automatically with Rubolt. After installation, verify it's working:

.. code-block:: bash

   rbcli version

Core Commands
=============

Project Management
------------------

``rbcli init <name>``
  Create a new Rubolt project with the specified name.

  .. code-block:: bash

     rbcli init my-project
     cd my-project

  **Generated Structure:**

  .. code-block:: text

     my-project/
     ├── src/
     │   └── main.rbo
     ├── tests/
     ├── package.json
     ├── .rbo.config
     └── README.md

  **Options:**

  * ``--template <template>`` - Use specific project template
  * ``--no-git`` - Don't initialize Git repository
  * ``--description <desc>`` - Set project description

  **Examples:**

  .. code-block:: bash

     rbcli init web-server --template server
     rbcli init cli-tool --template cli --description "My CLI tool"

``rbcli build``
  Build the current project, compiling all source files and dependencies.

  .. code-block:: bash

     rbcli build

  **Options:**

  * ``--release`` - Build in release mode with optimizations
  * ``--debug`` - Build with debug information
  * ``--target <target>`` - Specify build target (native, wasm, etc.)
  * ``--output <dir>`` - Specify output directory

  **Examples:**

  .. code-block:: bash

     rbcli build --release
     rbcli build --target wasm --output dist/

Code Execution
--------------

``rbcli run <file>``
  Run a Rubolt file or project.

  .. code-block:: bash

     rbcli run examples/hello.rbo
     rbcli run  # Run current project

  **Options:**

  * ``--args <args>`` - Pass arguments to the program
  * ``--env <file>`` - Load environment variables from file
  * ``--profile`` - Enable profiling during execution
  * ``--jit`` - Force JIT compilation
  * ``--no-jit`` - Disable JIT compilation

  **Examples:**

  .. code-block:: bash

     rbcli run server.rbo --args "--port 8080 --host 0.0.0.0"
     rbcli run --profile --jit performance_test.rbo

``rbcli repl``
  Start the interactive Read-Eval-Print Loop.

  .. code-block:: bash

     rbcli repl

  **Options:**

  * ``--no-history`` - Disable command history
  * ``--no-completion`` - Disable tab completion
  * ``--script <file>`` - Load and execute script on startup

  **REPL Commands:**

  .. code-block:: text

     :help           - Show help
     :quit           - Exit REPL
     :clear          - Clear screen
     :history        - Show command history
     :inspect <var>  - Inspect variable
     :type <expr>    - Show expression type
     :time <expr>    - Time expression execution
     :load <file>    - Load and execute file
     :save <file>    - Save session to file

``rbcli compile <file>``
  Compile Rubolt source to bytecode or other formats.

  .. code-block:: bash

     rbcli compile src/main.rbo

  **Options:**

  * ``--output <file>`` - Specify output file
  * ``--format <format>`` - Output format (bytecode, ast, tokens)
  * ``--optimize`` - Enable optimizations

  **Examples:**

  .. code-block:: bash

     rbcli compile main.rbo --output main.rbc --optimize
     rbcli compile debug.rbo --format ast --output debug.ast

Development Tools
-----------------

``rbcli debug <file>``
  Start the debugger with the specified file.

  .. code-block:: bash

     rbcli debug my_program.rbo

  **Debugger Commands:**

  .. code-block:: text

     break <file>:<line>  - Set breakpoint
     break <function>     - Break on function entry
     step                 - Step into
     next                 - Step over
     continue             - Continue execution
     finish               - Step out of current function
     print <var>          - Print variable value
     set <var> <value>    - Set variable value
     backtrace            - Show call stack
     info locals          - Show local variables
     info breakpoints     - List all breakpoints
     delete <num>         - Delete breakpoint
     quit                 - Exit debugger

``rbcli profile <file>``
  Profile program execution and generate performance reports.

  .. code-block:: bash

     rbcli profile my_program.rbo

  **Options:**

  * ``--output <file>`` - Save profile data to file
  * ``--format <format>`` - Output format (text, json, flamegraph)
  * ``--functions`` - Profile function calls
  * ``--memory`` - Profile memory usage
  * ``--jit`` - Profile JIT compilation

  **Examples:**

  .. code-block:: bash

     rbcli profile --memory --output profile.json server.rbo
     rbcli profile --format flamegraph --output flame.svg compute.rbo

``rbcli format <files>``
  Format Rubolt source code according to style guidelines.

  .. code-block:: bash

     rbcli format src/main.rbo
     rbcli format src/**/*.rbo

  **Options:**

  * ``--write`` - Write changes to files (default: print to stdout)
  * ``--check`` - Check if files are formatted (exit code 1 if not)
  * ``--config <file>`` - Use custom formatting configuration
  * ``--indent-size <n>`` - Set indentation size
  * ``--max-line-length <n>`` - Set maximum line length

  **Examples:**

  .. code-block:: bash

     rbcli format --write src/
     rbcli format --check --max-line-length 120 src/main.rbo

``rbcli lint <files>``
  Analyze code for potential issues and style violations.

  .. code-block:: bash

     rbcli lint src/main.rbo
     rbcli lint src/**/*.rbo

  **Options:**

  * ``--config <file>`` - Use custom linting configuration
  * ``--fix`` - Automatically fix issues where possible
  * ``--severity <level>`` - Minimum severity to report (info, warning, error)
  * ``--rules <rules>`` - Enable specific rules

  **Examples:**

  .. code-block:: bash

     rbcli lint --fix --severity warning src/
     rbcli lint --rules "no-unused-vars,require-types" src/main.rbo

``rbcli lsp``
  Start the Language Server Protocol server for IDE integration.

  .. code-block:: bash

     rbcli lsp

  **Options:**

  * ``--port <port>`` - Use TCP instead of stdio
  * ``--log <file>`` - Log LSP messages to file
  * ``--verbose`` - Enable verbose logging

Testing
-------

``rbcli test``
  Run the project's test suite.

  .. code-block:: bash

     rbcli test

  **Options:**

  * ``--pattern <pattern>`` - Run tests matching pattern
  * ``--verbose`` - Show detailed test output
  * ``--coverage`` - Generate code coverage report
  * ``--parallel`` - Run tests in parallel
  * ``--timeout <seconds>`` - Set test timeout

  **Examples:**

  .. code-block:: bash

     rbcli test --pattern "*math*" --verbose
     rbcli test --coverage --parallel

``rbcli benchmark <files>``
  Run performance benchmarks.

  .. code-block:: bash

     rbcli benchmark benchmarks/
     rbcli benchmark benchmarks/sort.rbo

  **Options:**

  * ``--iterations <n>`` - Number of iterations per benchmark
  * ``--warmup <n>`` - Number of warmup iterations
  * ``--output <file>`` - Save results to file
  * ``--compare <file>`` - Compare with previous results

Package Management
------------------

``rbcli add <package>``
  Add a dependency to the project.

  .. code-block:: bash

     rbcli add http-client@2.1.0
     rbcli add json-parser

  **Options:**

  * ``--dev`` - Add as development dependency
  * ``--optional`` - Add as optional dependency
  * ``--registry <url>`` - Use custom package registry

``rbcli remove <package>``
  Remove a dependency from the project.

  .. code-block:: bash

     rbcli remove old-package

``rbcli install``
  Install all project dependencies.

  .. code-block:: bash

     rbcli install

  **Options:**

  * ``--production`` - Install only production dependencies
  * ``--force`` - Force reinstall all packages
  * ``--offline`` - Use only cached packages

``rbcli update``
  Update project dependencies to latest compatible versions.

  .. code-block:: bash

     rbcli update
     rbcli update specific-package

``rbcli publish``
  Publish the current project to a package registry.

  .. code-block:: bash

     rbcli publish

  **Options:**

  * ``--registry <url>`` - Publish to specific registry
  * ``--tag <tag>`` - Publish with specific tag
  * ``--dry-run`` - Show what would be published without actually publishing

Library Management
------------------

``rbcli newlib <name>``
  Create a new library project template.

  .. code-block:: bash

     rbcli newlib my-library

  **Options:**

  * ``--native`` - Create native C extension library
  * ``--sdk`` - Include SDK development tools
  * ``--template <template>`` - Use specific library template

  **Examples:**

  .. code-block:: bash

     rbcli newlib math-utils --native --sdk
     rbcli newlib web-framework --template server-lib

Advanced Commands
=================

Simulation and Analysis
-----------------------

``rbcli sim <file>``
  Run program in the Bopes virtual machine simulator.

  .. code-block:: bash

     rbcli sim examples/hello.rbo

  **Options:**

  * ``--memory <size>`` - Set virtual memory size
  * ``--trace`` - Enable instruction tracing
  * ``--debug`` - Enable debugging in simulator

``rbcli analyze <project>``
  Analyze project dependencies and structure.

  .. code-block:: bash

     rbcli analyze
     rbcli analyze /path/to/project

  **Options:**

  * ``--dependencies`` - Analyze dependency tree
  * ``--security`` - Check for security vulnerabilities
  * ``--performance`` - Analyze performance characteristics
  * ``--output <format>`` - Output format (text, json, html)

Documentation
-------------

``rbcli doc <files>``
  Generate documentation from source code.

  .. code-block:: bash

     rbcli doc src/
     rbcli doc --output docs/ src/**/*.rbo

  **Options:**

  * ``--output <dir>`` - Output directory for documentation
  * ``--format <format>`` - Documentation format (html, markdown, pdf)
  * ``--private`` - Include private members in documentation
  * ``--theme <theme>`` - Use specific documentation theme

Utility Commands
================

Information
-----------

``rbcli version``
  Show version information.

  .. code-block:: bash

     rbcli version

  **Options:**

  * ``--verbose`` - Show detailed version information including build details

``rbcli help [command]``
  Show help information for commands.

  .. code-block:: bash

     rbcli help
     rbcli help run
     rbcli help test

``rbcli config``
  Manage global CLI configuration.

  .. code-block:: bash

     rbcli config list
     rbcli config set editor.command "code"
     rbcli config get package.registry

  **Configuration Options:**

  * ``editor.command`` - Default editor command
  * ``package.registry`` - Default package registry URL
  * ``format.indent_size`` - Default indentation size
  * ``lint.severity`` - Default linting severity
  * ``test.parallel`` - Enable parallel testing by default

Environment Management
----------------------

``rbcli env``
  Manage development environments.

  .. code-block:: bash

     rbcli env list
     rbcli env create development
     rbcli env activate development
     rbcli env remove old-env

  **Options:**

  * ``--python <version>`` - Specify Python version for interop
  * ``--packages <packages>`` - Pre-install packages in environment

Configuration Files
===================

Project Configuration (.rbo.config)
------------------------------------

.. code-block:: json

   {
     "version": "1.0",
     "name": "my-project",
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
     },
     "linting": {
       "max_line_length": 100,
       "max_complexity": 15,
       "enforce_types": true
     },
     "testing": {
       "parallel": true,
       "timeout": 30,
       "coverage": true
     }
   }

Package Configuration (package.json)
-------------------------------------

.. code-block:: json

   {
     "name": "my-project",
     "version": "1.0.0",
     "description": "My Rubolt project",
     "author": "Your Name <your.email@example.com>",
     "license": "MIT",
     "entry": "src/main.rbo",
     "dependencies": {
       "http-client": "^2.1.0",
       "json-parser": "~1.5.2"
     },
     "devDependencies": {
       "test-framework": "^3.0.0"
     },
     "scripts": {
       "start": "rbcli run",
       "test": "rbcli test",
       "build": "rbcli build --release",
       "lint": "rbcli lint src/"
     },
     "keywords": ["rubolt", "example"],
     "repository": {
       "type": "git",
       "url": "https://github.com/user/my-project.git"
     }
   }

Linter Configuration (.rblint.json)
------------------------------------

.. code-block:: json

   {
     "rules": {
       "naming_convention": "snake_case",
       "max_line_length": 100,
       "max_function_length": 50,
       "max_complexity": 15,
       "require_type_annotations": true,
       "no_unused_variables": true,
       "no_dead_code": true,
       "prefer_const": true,
       "no_magic_numbers": true
     },
     "ignore": [
       "build/",
       "vendor/",
       "*.generated.rbo",
       "tests/fixtures/"
     ],
     "severity": {
       "naming_convention": "warning",
       "max_complexity": "error",
       "no_unused_variables": "warning"
     }
   }

Formatter Configuration (.rbformat.json)
-----------------------------------------

.. code-block:: json

   {
     "indent_size": 4,
     "indent_type": "spaces",
     "max_line_length": 100,
     "brace_style": "1tbs",
     "space_before_function_paren": false,
     "align_parameters": true,
     "trailing_comma": "multiline",
     "quote_style": "double",
     "semicolons": "always"
   }

Global Configuration
====================

The CLI stores global configuration in:

* **Windows**: ``%APPDATA%\Rubolt\config.json``
* **macOS**: ``~/Library/Application Support/Rubolt/config.json``
* **Linux**: ``~/.config/rubolt/config.json``

.. code-block:: json

   {
     "editor": {
       "command": "code",
       "args": ["-g", "{file}:{line}"]
     },
     "package": {
       "registry": "https://packages.rubolt.dev",
       "cache_dir": "~/.rubolt/cache"
     },
     "format": {
       "indent_size": 4,
       "max_line_length": 100
     },
     "lint": {
       "severity": "warning",
       "auto_fix": false
     },
     "test": {
       "parallel": true,
       "coverage": false
     }
   }

Environment Variables
=====================

The CLI recognizes these environment variables:

``RUBOLT_HOME``
  Override default Rubolt installation directory.

``RUBOLT_PATH``
  Additional directories to search for modules.

``RUBOLT_REGISTRY``
  Default package registry URL.

``RUBOLT_CACHE_DIR``
  Directory for caching downloaded packages.

``RUBOLT_CONFIG``
  Path to global configuration file.

``RUBOLT_DEBUG``
  Enable debug output (set to ``1`` or ``true``).

``RUBOLT_JIT``
  Control JIT compilation (``on``, ``off``, ``auto``).

``RUBOLT_GC``
  Garbage collection mode (``mark-sweep``, ``reference-counting``, ``hybrid``).

Examples and Workflows
======================

Creating a New Project
-----------------------

.. code-block:: bash

   # Create new project
   rbcli init my-web-server --template server
   cd my-web-server

   # Install dependencies
   rbcli add http-server@3.0.0
   rbcli add json-parser@2.1.0 --dev

   # Set up development environment
   rbcli env create development
   rbcli env activate development

   # Run in development mode
   rbcli run --args "--port 3000 --debug"

Development Workflow
--------------------

.. code-block:: bash

   # Format code
   rbcli format --write src/

   # Lint code
   rbcli lint --fix src/

   # Run tests
   rbcli test --coverage

   # Profile performance
   rbcli profile --memory src/main.rbo

   # Build for production
   rbcli build --release

Debugging Workflow
------------------

.. code-block:: bash

   # Start debugger
   rbcli debug src/main.rbo

   # In debugger:
   # break main.rbo:25
   # run
   # step
   # print variable_name
   # continue

Library Development
-------------------

.. code-block:: bash

   # Create native library
   rbcli newlib math-utils --native --sdk

   # Build library
   cd math-utils
   rbcli build

   # Test library
   rbcli test

   # Generate documentation
   rbcli doc --output docs/ src/

   # Publish library
   rbcli publish

CI/CD Integration
=================

GitHub Actions
--------------

.. code-block:: yaml

   name: CI
   on: [push, pull_request]
   
   jobs:
     test:
       runs-on: ubuntu-latest
       steps:
         - uses: actions/checkout@v2
         - name: Install Rubolt
           run: |
             curl -sSL https://install.rubolt.dev | bash
             echo "$HOME/.rubolt/bin" >> $GITHUB_PATH
         - name: Install dependencies
           run: rbcli install
         - name: Lint
           run: rbcli lint src/
         - name: Test
           run: rbcli test --coverage
         - name: Build
           run: rbcli build --release

GitLab CI
---------

.. code-block:: yaml

   stages:
     - test
     - build
   
   test:
     stage: test
     script:
       - curl -sSL https://install.rubolt.dev | bash
       - export PATH="$HOME/.rubolt/bin:$PATH"
       - rbcli install
       - rbcli lint src/
       - rbcli test --coverage
   
   build:
     stage: build
     script:
       - rbcli build --release
     artifacts:
       paths:
         - build/

Troubleshooting
===============

Common Issues
-------------

**Command not found:**

.. code-block:: bash

   # Check if Rubolt is in PATH
   echo $PATH
   
   # Add to PATH manually
   export PATH="$HOME/.rubolt/bin:$PATH"
   
   # Or reinstall with --add-to-path
   bash Installer/install.sh --add-to-path

**Build failures:**

.. code-block:: bash

   # Clean build directory
   rm -rf build/
   
   # Rebuild with verbose output
   rbcli build --verbose
   
   # Check dependencies
   rbcli install --force

**Performance issues:**

.. code-block:: bash

   # Profile to identify bottlenecks
   rbcli profile --functions --memory src/main.rbo
   
   # Enable JIT compilation
   rbcli run --jit src/main.rbo
   
   # Check memory usage
   rbcli run --profile src/main.rbo

**Test failures:**

.. code-block:: bash

   # Run tests with verbose output
   rbcli test --verbose
   
   # Run specific test
   rbcli test --pattern "test_name"
   
   # Debug test
   rbcli debug tests/test_file.rbo

Getting Help
============

* **Built-in help**: ``rbcli help [command]``
* **Documentation**: https://docs.rubolt.dev
* **GitHub Issues**: https://github.com/piratebomber/Rubolt/issues
* **Community Forum**: https://forum.rubolt.dev
* **Discord**: https://discord.gg/rubolt

The Rubolt CLI is designed to be intuitive and powerful, providing all the tools you need for productive Rubolt development. For more advanced usage and customization options, refer to the individual tool documentation and configuration references.