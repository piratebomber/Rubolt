@echo off
echo Building example_math.dll ...
gcc -shared -O2 -o example_math.dll example_math.c
if %ERRORLEVEL% NEQ 0 (
  echo Failed to build example_math.dll
  exit /b 1
)
echo Done.