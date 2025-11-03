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
gcc -Wall -Wextra -std=c11 -O2 -c bc_compiler.c -o bc_compiler.o
gcc -Wall -Wextra -std=c11 -O2 -c vm.c -o vm.o
gcc -Wall -Wextra -std=c11 -O2 -c ..\bopes\bopes.c -o bopes.o -I..
gcc -Wall -Wextra -std=c11 -O2 -c ..\runtime\manager.c -o manager.o -I..

echo Compiling C modules in Modules/...
gcc -Wall -Wextra -std=c11 -O2 -c ..\Modules\string_mod.c -o string_mod.o
gcc -Wall -Wextra -std=c11 -O2 -c ..\Modules\random_mod.c -o random_mod.o
gcc -Wall -Wextra -std=c11 -O2 -c ..\Modules\atomics_mod.c -o atomics_mod.o

gcc main.o lexer.o parser.o ast.o interpreter.o typechecker.o module.o modules_registry.o bc_compiler.o vm.o manager.o bopes.o ^
    string_mod.o random_mod.o atomics_mod.o -o rubolt.exe

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
