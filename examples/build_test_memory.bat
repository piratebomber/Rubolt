@echo off
echo Building memory management test...

gcc -Wall -Wextra -std=c11 -O2 -I.. -c ../gc/gc.c -o gc.o
gcc -Wall -Wextra -std=c11 -O2 -I.. -c ../rc/rc.c -o rc.o
gcc -Wall -Wextra -std=c11 -O2 -I.. test_memory.c gc.o rc.o -o test_memory.exe

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo Build successful! Run test_memory.exe to test.
