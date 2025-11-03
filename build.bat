@echo off
echo Building Rubolt...

cd src

echo Compiling C sources...
gcc -Wall -Wextra -std=c11 -O2 -c main.c -o main.o
gcc -Wall -Wextra -std=c11 -O2 -c lexer.c -o lexer.o
gcc -Wall -Wextra -std=c11 -O2 -c parser.c -o parser.o
gcc -Wall -Wextra -std=c11 -O2 -c ast.c -o ast.o
gcc -Wall -Wextra -std=c11 -O2 -c interpreter.c -o interpreter.o

echo Linking...
gcc main.o lexer.o parser.o ast.o interpreter.o -o rubolt.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful! Executable: src\rubolt.exe
    echo.
    echo To run examples:
    echo   src\rubolt.exe examples\hello.rbo
) else (
    echo Build failed!
    exit /b 1
)

cd ..
