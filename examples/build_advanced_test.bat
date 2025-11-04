@echo off
echo Building advanced memory management test...

gcc -Wall -Wextra -std=c11 -O2 -I.. -c ../gc/gc.c -o gc.o
gcc -Wall -Wextra -std=c11 -O2 -I.. -c ../gc/type_info.c -o type_info.o
gcc -Wall -Wextra -std=c11 -O2 -I.. -c ../rc/rc.c -o rc.o
gcc -Wall -Wextra -std=c11 -O2 -I.. test_advanced_memory.c gc.o type_info.o rc.o -o test_advanced_memory.exe

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo Build successful! Run test_advanced_memory.exe to test.
echo.
echo This test demonstrates:
echo   - Type information registration
echo   - Object graph traversal
echo   - Full cycle detection in GC and RC
echo   - Complex object relationships
