=======================
Contributing to Rubolt
=======================

Thank you for your interest in contributing to Rubolt! This guide will help you get started with development.

.. contents:: Table of Contents
   :local:
   :depth: 2

Getting Started
===============

Development Environment Setup
-----------------------------

Prerequisites
~~~~~~~~~~~~~

**Windows:**

- MinGW-w64 or MSVC with GCC 8.0+
- Python 3.8+ (for build scripts and gimmicks)
- Git for version control
- VS Code (recommended) or your preferred editor

**Linux/macOS:**

- GCC 8.0+ or Clang 10.0+
- Python 3.8+
- Git
- Make

Cloning the Repository
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   git clone https://github.com/piratebomber/Rubolt.git
   cd Rubolt

Building from Source
~~~~~~~~~~~~~~~~~~~~

**Windows:**

.. code-block:: batch

   build_all.bat

**Linux/macOS:**

.. code-block:: bash

   chmod +x build_all.sh
   ./build_all.sh

This builds:

- ``src/rubolt.exe`` - Main interpreter
- ``rbcli.exe`` - CLI tool
- ``rubolt-repl.exe`` - Interactive REPL

Using Docker/Devcontainer
~~~~~~~~~~~~~~~~~~~~~~~~~

For a reproducible development environment:

.. code-block:: bash

   # Using Docker
   docker build -t rubolt .
   docker run -it -v $PWD:/work rubolt

   # Using VS Code Devcontainer
   # Open folder in VS Code and "Reopen in Container"

Code Style Guidelines
=====================

C Code Standards
----------------

General Rules
~~~~~~~~~~~~~

- **Standard:** C11 compliant code only
- **Compiler:** Must compile with ``-Wall -Wextra`` without warnings
- **Formatting:** Use EditorConfig settings (4 spaces, no tabs)
- **Naming:**
  - Functions: ``snake_case``
  - Types: ``PascalCase``
  - Constants/Macros: ``UPPER_SNAKE_CASE``
  - Variables: ``snake_case``

Example:

.. code-block:: c

   #define MAX_BUFFER_SIZE 1024

   typedef struct ReplState {
       char *current_line;
       size_t cursor_pos;
   } ReplState;

   void repl_init(ReplState *repl) {
       repl->current_line = NULL;
       repl->cursor_pos = 0;
   }

Memory Management
~~~~~~~~~~~~~~~~~

- Always free allocated memory
- Use ``malloc/calloc/realloc/free`` for manual management
- Use GC (``gc/gc.h``) or RC (``rc/rc.h``) for managed objects
- Check for NULL after allocation
- Avoid memory leaks - use Valgrind or similar tools

Example:

.. code-block:: c

   char *buffer = malloc(size);
   if (!buffer) {
       fprintf(stderr, "Memory allocation failed\n");
       return NULL;
   }
   // ... use buffer ...
   free(buffer);

Error Handling
~~~~~~~~~~~~~~

- Return ``bool`` for success/failure
- Return ``NULL`` for pointer failures
- Use negative values for error codes
- Always check return values

Example:

.. code-block:: c

   bool parse_config(const char *path, Config *out) {
       FILE *f = fopen(path, "r");
       if (!f) {
           fprintf(stderr, "Failed to open %s\n", path);
           return false;
       }
       // ... parse config ...
       fclose(f);
       return true;
   }

Python Code Standards
---------------------

- Follow PEP 8 style guide
- Use type hints (Python 3.8+)
- Docstrings for all public functions
- Maximum line length: 100 characters

Example:

.. code-block:: python

   def parse_header(header_path: str) -> List[FunctionDef]:
       """Parse C header file and extract function definitions.
       
       Args:
           header_path: Path to .h file
           
       Returns:
           List of FunctionDef objects
       """
       with open(header_path, 'r') as f:
           return parse_functions(f.read())

Project Structure
=================

Key Directories
---------------

::

   Rubolt/
   ├── src/                    # Core interpreter (C)
   │   ├── lexer.c/h          # Tokenization
   │   ├── parser.c/h         # AST parsing
   │   ├── interpreter.c/h    # Execution engine
   │   ├── typechecker.c/h    # Static type checking
   │   └── module.c/h         # Module system
   ├── repl/                   # Interactive REPL
   │   ├── repl.c/h           # REPL implementation
   │   └── repl_main.c        # Entry point
   ├── cli/                    # CLI tool (rbcli)
   ├── gc/                     # Garbage collector
   ├── rc/                     # Reference counting
   ├── collections/            # Data structures
   ├── python/                 # Python bindings & gimmicks
   │   └── gimmicks/          # C↔Python interop tools
   ├── vendor/                 # Dependency manager
   ├── shared/                 # SDK and shared modules
   ├── compiletime/            # Preprocessor macros
   ├── Modules/                # Native C modules
   ├── Docs/                   # Documentation (reST)
   ├── examples/               # Example programs
   └── tests/                  # Test suite

Adding New Features
===================

Adding a Language Feature
-------------------------

1. **Update the Lexer** (``src/lexer.c``)
   - Add new token types to ``TokenType`` enum
   - Implement token recognition

2. **Update the Parser** (``src/parser.c``)
   - Add parsing rules for new syntax
   - Create AST node types

3. **Update the Interpreter** (``src/interpreter.c``)
   - Implement execution logic
   - Handle type checking

4. **Add Tests** (``tests/``)
   - Create test cases
   - Verify behavior

5. **Update Documentation** (``Docs/language_reference.rst``)
   - Document syntax
   - Provide examples

6. **Update TextMate Grammar** (``vscode-rubolt/syntaxes/rubolt.tmLanguage.json``)
   - Add syntax highlighting support

Example: Adding a new operator:

.. code-block:: c

   // 1. lexer.h - Add token type
   TOKEN_POWER,  // ** operator

   // 2. lexer.c - Recognize token
   if (*lexer->current == '*' && *(lexer->current + 1) == '*') {
       lexer->current += 2;
       return make_token(lexer, TOKEN_POWER);
   }

   // 3. parser.c - Parse binary expression
   if (match(&parser, TOKEN_POWER)) {
       Expr *right = parse_exponentiation(parser);
       left = expr_binary("**", left, right);
   }

   // 4. interpreter.c - Evaluate
   if (strcmp(expr->as.binary.op, "**") == 0) {
       return value_number(pow(left.as.number, right.as.number));
   }

Adding a Built-in Module
------------------------

1. Create module in ``Modules/mymodule_mod.c``:

.. code-block:: c

   #include "../src/module.h"
   #include "../src/ast.h"

   static Value mymodule_function(Environment *env, Value *args, size_t arg_count) {
       if (arg_count != 1) {
           fprintf(stderr, "mymodule.function() takes 1 argument\n");
           return value_null();
       }
       // Implementation
       return value_number(42);
   }

   void register_mymodule(ModuleRegistry *registry) {
       Module *mod = module_create("mymodule");
       module_add_function(mod, "function", mymodule_function);
       module_registry_add(registry, mod);
   }

2. Register in ``src/modules_registry.c``:

.. code-block:: c

   extern void register_mymodule(ModuleRegistry *registry);

   void modules_registry_init(ModuleRegistry *registry) {
       // ... existing modules ...
       register_mymodule(registry);
   }

3. Update build scripts (``build_all.bat``, ``Makefile``)

4. Document in ``Docs/stdlib.rst``

Adding REPL Commands
--------------------

1. Add command handler in ``repl/repl.c``:

.. code-block:: c

   void repl_cmd_mycommand(const char *args) {
       printf("Executing mycommand with args: %s\n", args);
       // Implementation
   }

2. Register in ``repl_init()``:

.. code-block:: c

   repl_register_command("mycommand", "Description", repl_cmd_mycommand);

Testing
=======

Running Tests
-------------

.. code-block:: bash

   # Run all tests
   rbcli test

   # Run specific test
   rbcli run tests/test_feature.rbo

Writing Tests
-------------

Create test files in ``tests/``:

.. code-block:: rubolt

   # tests/test_myfeature.rbo
   
   def test_basic() -> void {
       let result: number = myfunction(5);
       assert(result == 10, "Expected 10");
   }

   def test_edge_case() -> void {
       let result: number = myfunction(0);
       assert(result == 0, "Expected 0");
   }

   test_basic();
   test_edge_case();
   print("All tests passed!");

Memory Testing
--------------

**Linux/macOS with Valgrind:**

.. code-block:: bash

   valgrind --leak-check=full ./src/rubolt examples/test.rbo

**Windows with Dr. Memory:**

.. code-block:: batch

   drmemory -batch -- src\rubolt.exe examples\test.rbo

Pull Request Process
====================

Before Submitting
-----------------

1. **Create a branch:**

   .. code-block:: bash

      git checkout -b feature/my-feature

2. **Make your changes**

3. **Test thoroughly:**

   .. code-block:: bash

      build_all.bat
      rbcli test

4. **Update documentation:**

   - Add/update ``.rst`` files in ``Docs/``
   - Update ``README.md`` if needed
   - Add inline code comments

5. **Commit with clear messages:**

   .. code-block:: bash

      git add .
      git commit -m "Add feature: description"

   Commit message format::

      <type>: <subject>

      <body>

      <footer>

   Types: ``feat``, ``fix``, ``docs``, ``style``, ``refactor``, ``test``, ``chore``

   Example::

      feat: add power operator (**) support

      - Added TOKEN_POWER to lexer
      - Implemented exponentiation in parser and interpreter
      - Added tests for edge cases
      - Updated language reference documentation

      Closes #123

Submitting the PR
-----------------

1. Push to your fork:

   .. code-block:: bash

      git push origin feature/my-feature

2. Open a Pull Request on GitHub

3. Fill out the PR template:

   - **Description:** What does this PR do?
   - **Motivation:** Why is this change needed?
   - **Testing:** How was this tested?
   - **Screenshots:** If UI changes
   - **Checklist:** Complete all items

PR Checklist
~~~~~~~~~~~~

- [ ] Code compiles without warnings
- [ ] All tests pass
- [ ] New tests added for new features
- [ ] Documentation updated
- [ ] Commit messages follow conventions
- [ ] No merge conflicts
- [ ] Code follows style guidelines
- [ ] Memory leaks checked (Valgrind/Dr. Memory)

Code Review Process
-------------------

- Maintainers will review within 1-3 days
- Address feedback promptly
- Be open to suggestions
- Update PR based on review comments
- Once approved, maintainer will merge

Community Guidelines
====================

Code of Conduct
---------------

- Be respectful and inclusive
- Provide constructive feedback
- Focus on the code, not the person
- Help newcomers feel welcome
- Report unacceptable behavior to maintainers

Getting Help
------------

- **GitHub Issues:** Bug reports and feature requests
- **GitHub Discussions:** Questions and community chat
- **Documentation:** Check ``Docs/`` first
- **Examples:** See ``examples/`` for usage patterns

Reporting Bugs
--------------

Use the bug report template:

::

   **Describe the bug**
   A clear description of what the bug is.

   **To Reproduce**
   Steps to reproduce:
   1. Run command '...'
   2. Execute code '...'
   3. See error

   **Expected behavior**
   What should happen.

   **Actual behavior**
   What actually happens.

   **Environment:**
   - OS: [e.g., Windows 11, Ubuntu 22.04]
   - Rubolt version: [e.g., 1.0.0]
   - GCC version: [e.g., 11.2]

   **Additional context**
   Any other relevant information.

Advanced Topics
===============

Working with the GC/RC Systems
-------------------------------

See ``gc/README.md`` and ``rc/README.md`` for detailed documentation on memory management systems.

Python Interop (Gimmicks)
--------------------------

See ``python/gimmicks/README.md`` for guides on:

- GCC-to-Python compilation
- C wrapper generation
- Type conversion
- FFI helpers

Vendor System
-------------

See ``vendor/README.md`` for dependency management.

CI/CD Pipelines
---------------

GitHub Actions workflows in ``.github/workflows/``:

- ``build.yml`` - Multi-platform builds
- ``lint.yml`` - Code quality checks
- ``test.yml`` - Test suite execution
- ``docs.yml`` - Documentation generation

License
=======

By contributing to Rubolt, you agree that your contributions will be licensed under the Apache 2.0 License.

Thank You!
==========

Your contributions make Rubolt better for everyone. Thank you for taking the time to contribute!
