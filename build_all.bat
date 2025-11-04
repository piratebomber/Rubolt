@echo off
echo ╔═══════════════════════════════════════╗
echo ║  Building Complete Rubolt Toolchain   ║
echo ╚═══════════════════════════════════════╝
echo.

REM Build main interpreter
echo [1/3] Building Rubolt interpreter...
cd src
gcc -Wall -Wextra -std=c11 -O2 -c main.c -o main.o
gcc -Wall -Wextra -std=c11 -O2 -c lexer.c -o lexer.o
gcc -Wall -Wextra -std=c11 -O2 -c parser.c -o parser.o
gcc -Wall -Wextra -std=c11 -O2 -c ast.c -o ast.o
gcc -Wall -Wextra -std=c11 -O2 -c interpreter.c -o interpreter.o
gcc -Wall -Wextra -std=c11 -O2 -c typechecker.c -o typechecker.o
gcc -Wall -Wextra -std=c11 -O2 -c module.c -o module.o
gcc -Wall -Wextra -std=c11 -O2 -c modules_registry.c -o modules_registry.o
gcc -Wall -Wextra -std=c11 -O2 -c dll_loader.c -o dll_loader.o
gcc -Wall -Wextra -std=c11 -O2 -c dll_import.c -o dll_import.o
gcc -Wall -Wextra -std=c11 -O2 -c native_registry.c -o native_registry.o
gcc -Wall -Wextra -std=c11 -O2 -c bc_compiler.c -o bc_compiler.o
gcc -Wall -Wextra -std=c11 -O2 -c vm.c -o vm.o
gcc -Wall -Wextra -std=c11 -O2 -c ..\bopes\bopes.c -o bopes.o -I..
gcc -Wall -Wextra -std=c11 -O2 -c ..\runtime\manager.c -o manager.o -I..

echo Compiling memory management...
gcc -Wall -Wextra -std=c11 -O2 -c ..\gc\gc.c -o gc.o -I..
gcc -Wall -Wextra -std=c11 -O2 -c ..\gc\type_info.c -o type_info.o -I..
gcc -Wall -Wextra -std=c11 -O2 -c ..\rc\rc.c -o rc.o -I..

echo Compiling advanced features (exceptions, async, threading)...
gcc -Wall -Wextra -std=c11 -O2 -c exception.c -o exception.o
gcc -Wall -Wextra -std=c11 -O2 -c debugger.c -o debugger.o 2>nul || echo debugger.c not yet implemented
gcc -Wall -Wextra -std=c11 -O2 -c profiler.c -o profiler.o 2>nul || echo profiler.c not yet implemented
gcc -Wall -Wextra -std=c11 -O2 -c jit_compiler.c -o jit_compiler.o 2>nul || echo jit_compiler.c not yet implemented
gcc -Wall -Wextra -std=c11 -O2 -c inline_cache.c -o inline_cache.o 2>nul || echo inline_cache.c not yet implemented
gcc -Wall -Wextra -std=c11 -O2 -c python_bridge.c -o python_bridge.o 2>nul || echo python_bridge.c not yet implemented
gcc -Wall -Wextra -std=c11 -O2 -c async.c -o async.o 2>nul || echo async.c not yet implemented
gcc -Wall -Wextra -std=c11 -O2 -c event_loop.c -o event_loop.o 2>nul || echo event_loop.c not yet implemented
gcc -Wall -Wextra -std=c11 -O2 -c threading.c -o threading.o 2>nul || echo threading.c not yet implemented

echo Compiling collections...
gcc -Wall -Wextra -std=c11 -O2 -c ..\collections\rb_collections.c -o rb_collections.o -I..
gcc -Wall -Wextra -std=c11 -O2 -c ..\collections\rb_list.c -o rb_list.o -I..

echo Compiling C modules in Modules/...
gcc -Wall -Wextra -std=c11 -O2 -c ..\Modules\string_mod.c -o string_mod.o
gcc -Wall -Wextra -std=c11 -O2 -c ..\Modules\random_mod.c -o random_mod.o
gcc -Wall -Wextra -std=c11 -O2 -c ..\Modules\atomics_mod.c -o atomics_mod.o

echo Linking all components...
gcc main.o lexer.o parser.o ast.o interpreter.o typechecker.o module.o modules_registry.o bc_compiler.o vm.o ^
    dll_loader.o dll_import.o native_registry.o ^
    manager.o bopes.o ^
    gc.o type_info.o rc.o ^
    exception.o ^
    rb_collections.o rb_list.o ^
    string_mod.o random_mod.o atomics_mod.o ^
    -o rubolt.exe -lm 2>nul

if %ERRORLEVEL% NEQ 0 (
    echo Trying without advanced modules...
    gcc main.o lexer.o parser.o ast.o interpreter.o typechecker.o module.o modules_registry.o bc_compiler.o vm.o ^
        dll_loader.o dll_import.o native_registry.o ^
        manager.o bopes.o ^
        gc.o type_info.o rc.o ^
        exception.o ^
        rb_collections.o rb_list.o ^
        string_mod.o random_mod.o atomics_mod.o ^
        -o rubolt.exe -lm
)

if %ERRORLEVEL% NEQ 0 (
    echo ✗ Failed to build interpreter
    cd ..
    exit /b 1
)
echo ✓ Interpreter built: src\rubolt.exe
cd ..

REM Build CLI tool
echo.
echo [2/3] Building Rubolt CLI tool...
cd cli
gcc -Wall -Wextra -std=c11 -O2 rbcli.c -o rbcli.exe

if %ERRORLEVEL% NEQ 0 (
    echo ✗ Failed to build CLI
    cd ..
    exit /b 1
)
echo ✓ CLI built: cli\rbcli.exe
copy rbcli.exe ..\rbcli.exe > nul
cd ..

REM Create directories
echo.
echo [3/3] Setting up directories...
if not exist "lib" mkdir lib
if not exist "stdlib" mkdir stdlib
if not exist "build" mkdir build
echo ✓ Directories created

echo.
echo ╔═══════════════════════════════════════╗
echo ║     Build Completed Successfully!     ║
echo ╚═══════════════════════════════════════╝
echo.
echo Executables:
echo   src\rubolt.exe  - Rubolt interpreter
echo   rbcli.exe       - CLI tool
echo.
echo Quick start:
echo   rbcli init myproject
echo   rbcli run examples\hello.rbo
echo   rbcli newlib mylib
echo.
