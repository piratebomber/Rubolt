@echo off
echo Building Rubolt Collections Test...

gcc -Wall -Wextra -std=c11 -O2 -c rb_collections.c -o rb_collections.o
gcc -Wall -Wextra -std=c11 -O2 -c rb_list.c -o rb_list.o
gcc -Wall -Wextra -std=c11 -O2 test_collections.c rb_collections.o rb_list.o -o test_collections.exe

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo Build successful! Run test_collections.exe
echo.
echo Testing Python-like List operations:
echo   - Dynamic arrays with automatic resizing
echo   - Negative indexing (list[-1])
echo   - Slicing (list[start:end:step])
echo   - append, insert, pop, remove
echo   - sort, reverse, extend, copy
echo   - index, count, contains
