@echo off
echo ========================================
echo Building Complete Rubolt System
echo ========================================

set CC=gcc
set CFLAGS=-Wall -Wextra -std=c99 -O2
set INCLUDES=-I./src -I./gc -I./rc
set LIBS=-lcurl -ljson-c

echo.
echo [1/8] Building Core Runtime...
cd src
%CC% %CFLAGS% %INCLUDES% -c *.c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build core runtime
    exit /b 1
)
cd ..

echo.
echo [2/8] Building Standard Library Modules...
cd Modules
call build_modules.bat
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build standard library modules
    exit /b 1
)
cd ..

echo.
echo [3/8] Building JIT Compiler...
cd src
%CC% %CFLAGS% %INCLUDES% -c jit_engine.c pattern_match.c generics.c error_handling.c test_framework.c package_manager.c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build JIT compiler
    exit /b 1
)
cd ..

echo.
echo [4/8] Building Tools...
cd tools
%CC% %CFLAGS% %INCLUDES% formatter.c -o formatter.exe
%CC% %CFLAGS% %INCLUDES% linter.c -o linter.exe
%CC% %CFLAGS% %INCLUDES% %LIBS% rubolt_lsp.c -o rubolt-lsp.exe
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build tools
    exit /b 1
)
cd ..

echo.
echo [5/8] Building CLI...
cd cli
make
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build CLI
    exit /b 1
)
cd ..

echo.
echo [6/8] Building Package Manager...
cd src
%CC% %CFLAGS% %INCLUDES% %LIBS% package_manager.c -c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build package manager
    exit /b 1
)
cd ..

echo.
echo [7/8] Building VS Code Extension...
cd vscode-rubolt
if exist node_modules (
    echo Node modules already installed
) else (
    echo Installing dependencies...
    npm install
)
echo Compiling TypeScript...
npm run compile
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build VS Code extension
    exit /b 1
)
cd ..

echo.
echo [8/8] Running Tests...
cd examples
echo Testing pattern matching...
echo "match x { 1 => print(\"one\"); _ => print(\"other\"); }" > test_pattern.rbo

echo Testing generics...
echo "def identity<T>(x: T) -> T { return x; }" > test_generics.rbo

echo Testing error handling...
echo "try { throw RuntimeError(\"test\"); } catch (RuntimeError e) { print(\"caught\"); }" > test_errors.rbo

echo Testing stdlib...
..\rbcli run stdlib_demo.rbo
cd ..

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Built Components:
echo   ✓ Core Runtime (src/*.o)
echo   ✓ Standard Library (Modules/*.o)
echo   ✓ JIT Compiler (src/jit_engine.o)
echo   ✓ Pattern Matching (src/pattern_match.o)
echo   ✓ Generics System (src/generics.o)
echo   ✓ Error Handling (src/error_handling.o)
echo   ✓ Testing Framework (src/test_framework.o)
echo   ✓ Package Manager (src/package_manager.o)
echo   ✓ Code Formatter (tools/formatter.exe)
echo   ✓ Linter (tools/linter.exe)
echo   ✓ Language Server (tools/rubolt-lsp.exe)
echo   ✓ CLI (cli/rbcli.exe)
echo   ✓ VS Code Extension (vscode-rubolt/out/)
echo.
echo New Language Features:
echo   ✓ Pattern Matching - match expressions with guards
echo   ✓ Generics - parameterized types and functions
echo   ✓ Error Handling - Result types and try/catch
echo   ✓ JIT Compilation - hot path optimization
echo   ✓ Testing Framework - unit tests, property tests, benchmarks
echo   ✓ Package Manager - dependency resolution and registry
echo.
echo Developer Tools:
echo   ✓ Code Formatter - automatic code formatting
echo   ✓ Linter - static analysis and style checking
echo   ✓ Language Server - IDE integration with LSP
echo   ✓ VS Code Extension - full IDE support
echo.
echo To use:
echo   rbcli run examples/stdlib_demo.rbo
echo   tools/formatter.exe examples/hello.rbo
echo   tools/linter.exe examples/hello.rbo
echo   rbcli test  # Run test suite
echo   rbcli init my_project  # Create new project