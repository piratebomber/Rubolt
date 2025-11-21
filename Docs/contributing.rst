=======================
Contributing to Rubolt
=======================

Thank you for your interest in contributing to Rubolt! This guide will help you get started with development and understand our contribution process.

.. contents:: Table of Contents
   :local:
   :depth: 2

Getting Started
===============

Development Environment Setup
-----------------------------

**Prerequisites:**

* **Windows**: MinGW-w64 or MSVC, Python 3.8+, Git
* **macOS**: Xcode Command Line Tools, Python 3.8+, Git  
* **Linux**: GCC 8.0+, Python 3.8+, Make, Git

**Clone and Build:**

.. code-block:: bash

   git clone https://github.com/piratebomber/Rubolt.git
   cd Rubolt
   
   # Windows
   build_all.bat
   
   # macOS/Linux
   chmod +x build_all.sh
   ./build_all.sh

**VS Code Setup:**

.. code-block:: bash

   cd vscode-rubolt
   npm install
   npm run compile

Code Style Guidelines
=====================

C Code Standards
----------------

* **Standard**: C11 compliant code
* **Compiler**: Must compile with ``-Wall -Wextra`` without warnings
* **Formatting**: 4 spaces, no tabs
* **Naming**: ``snake_case`` functions, ``PascalCase`` types, ``UPPER_SNAKE_CASE`` constants

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

**Memory Management:**

* Always free allocated memory
* Check for NULL after allocation
* Use GC (``gc/gc.h``) or RC (``rc/rc.h``) for managed objects

**Error Handling:**

* Return ``bool`` for success/failure
* Return ``NULL`` for pointer failures
* Always check return values

Python Code Standards
---------------------

* Follow PEP 8 style guide
* Use type hints (Python 3.8+)
* Docstrings for all public functions
* Maximum line length: 100 characters

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

Adding New Features
===================

Language Features
-----------------

1. **Update Lexer** (``src/lexer.c``) - Add token types and recognition
2. **Update Parser** (``src/parser.c``) - Add parsing rules and AST nodes
3. **Update Interpreter** (``src/interpreter.c``) - Implement execution logic
4. **Add Tests** - Create test cases in ``tests/``
5. **Update Documentation** - Document syntax in ``Docs/language_reference.rst``
6. **Update VS Code Grammar** - Add syntax highlighting support

Built-in Modules
-----------------

.. code-block:: c

   // Modules/mymodule_mod.c
   #include "../src/module.h"

   static Value mymodule_function(Environment *env, Value *args, size_t arg_count) {
       if (arg_count != 1) {
           fprintf(stderr, "mymodule.function() takes 1 argument\n");
           return value_null();
       }
       return value_number(42);
   }

   void register_mymodule(ModuleRegistry *registry) {
       Module *mod = module_create("mymodule");
       module_add_function(mod, "function", mymodule_function);
       module_registry_add(registry, mod);
   }

Register in ``src/modules_registry.c`` and update build scripts.

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

.. code-block:: rubolt

   # tests/test_myfeature.rbo
   
   def test_basic() -> void {
       let result: number = myfunction(5);
       assert(result == 10, "Expected 10");
   }
   
   test_basic();
   print("All tests passed!");

Memory Testing
--------------

**Linux/macOS:**

.. code-block:: bash

   valgrind --leak-check=full ./src/rubolt examples/test.rbo

**Windows:**

.. code-block:: batch

   drmemory -batch -- src\rubolt.exe examples\test.rbo

Pull Request Process
====================

Before Submitting
-----------------

1. **Create branch**: ``git checkout -b feature/my-feature``
2. **Test thoroughly**: ``rbcli test``
3. **Update documentation**: Add/update ``.rst`` files
4. **Commit with clear messages**: Use conventional commit format

.. code-block:: bash

   git commit -m "feat: add power operator (**) support

   - Added TOKEN_POWER to lexer
   - Implemented exponentiation in parser and interpreter  
   - Added tests for edge cases
   - Updated language reference documentation

   Closes #123"

Submitting the PR
-----------------

1. **Push to fork**: ``git push origin feature/my-feature``
2. **Open Pull Request** on GitHub
3. **Fill out PR template** with description, motivation, testing info
4. **Complete checklist**:

   - [ ] Code compiles without warnings
   - [ ] All tests pass
   - [ ] New tests added for new features
   - [ ] Documentation updated
   - [ ] Commit messages follow conventions
   - [ ] No merge conflicts
   - [ ] Memory leaks checked

Community Guidelines
====================

Code of Conduct
---------------

* Be respectful and inclusive
* Provide constructive feedback
* Focus on the code, not the person
* Help newcomers feel welcome
* Report unacceptable behavior to maintainers

Getting Help
------------

* **GitHub Issues**: Bug reports and feature requests
* **GitHub Discussions**: Questions and community chat
* **Documentation**: Check ``Docs/`` first
* **Examples**: See ``examples/`` for usage patterns

Reporting Bugs
--------------

Use the bug report template:

.. code-block:: text

   **Describe the bug**
   A clear description of what the bug is.

   **To Reproduce**
   Steps to reproduce:
   1. Run command '...'
   2. Execute code '...'
   3. See error

   **Expected behavior**
   What should happen.

   **Environment:**
   - OS: [e.g., Windows 11, Ubuntu 22.04]
   - Rubolt version: [e.g., 1.0.0]
   - GCC version: [e.g., 11.2]

Advanced Topics
===============

Working with Memory Systems
---------------------------

See ``gc/README.md`` and ``rc/README.md`` for detailed documentation.

Python Interop Development
---------------------------

See ``python/gimmicks/README.md`` for guides on C↔Python interop tools.

Vendor System
-------------

See ``vendor/README.md`` for dependency management development.

CI/CD Pipelines
---------------

GitHub Actions workflows in ``.github/workflows/``:

* ``build.yml`` - Multi-platform builds
* ``lint.yml`` - Code quality checks  
* ``test.yml`` - Test suite execution
* ``docs.yml`` - Documentation generation

License
=======

By contributing to Rubolt, you agree that your contributions will be licensed under the Apache 2.0 License.

Thank You!
==========

Your contributions make Rubolt better for everyone. Thank you for taking the time to contribute!