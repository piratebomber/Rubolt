@echo off
echo Building Rubolt Standard Library Modules...

set CC=gcc
set CFLAGS=-Wall -Wextra -std=c99 -fPIC -O2
set INCLUDES=-I../src -I../gc -I../rc
set LIBS=-lcurl -ljson-c

echo Compiling string module...
%CC% %CFLAGS% %INCLUDES% -c string_mod.c -o string_mod.o

echo Compiling random module...
%CC% %CFLAGS% %INCLUDES% -c random_mod.c -o random_mod.o

echo Compiling atomics module...
%CC% %CFLAGS% %INCLUDES% -c atomics_mod.c -o atomics_mod.o

echo Compiling file module...
%CC% %CFLAGS% %INCLUDES% -c file_mod.c -o file_mod.o

echo Compiling JSON module...
%CC% %CFLAGS% %INCLUDES% %LIBS% -c json_mod.c -o json_mod.o

echo Compiling time module...
%CC% %CFLAGS% %INCLUDES% -c time_mod.c -o time_mod.o

echo Compiling HTTP module...
%CC% %CFLAGS% %INCLUDES% %LIBS% -c http_mod.c -o http_mod.o

echo All modules compiled successfully!
echo Object files: *.o
echo.
echo To test modules, run: gcc -o test_modules test_modules.c *.o %LIBS%