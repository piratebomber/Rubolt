CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lm

TARGET = rubolt
MODULE_SOURCES = ../Modules/string_mod.c ../Modules/random_mod.c ../Modules/atomics_mod.c
# Core sources
CORE_SOURCES = main.c lexer.c parser.c ast.c interpreter.c typechecker.c module.c modules_registry.c bc_compiler.c vm.c

# DLL and native support
NATIVE_SOURCES = dll_loader.c dll_import.c native_registry.c

# Memory management
MEM_SOURCES = ../gc/gc.c ../gc/type_info.c ../rc/rc.c

# Exception and async
ADVANCED_SOURCES = exception.c debugger.c profiler.c jit_compiler.c inline_cache.c python_bridge.c async.c event_loop.c threading.c

# Collections
COLLECTIONS_SOURCES = ../collections/rb_collections.c ../collections/rb_list.c

# Runtime
RUNTIME_SOURCES = ../runtime/runtime.c frozen.c ../bopes/bopes.c ../runtime/manager.c

SOURCES = $(CORE_SOURCES) $(NATIVE_SOURCES) $(MEM_SOURCES) $(ADVANCED_SOURCES) $(COLLECTIONS_SOURCES) $(RUNTIME_SOURCES) $(MODULE_SOURCES)
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET) $(TARGET).exe

lexer.o: lexer.c lexer.h
parser.o: parser.c parser.h lexer.h ast.h
ast.o: ast.c ast.h
interpreter.o: interpreter.c interpreter.h ast.h
typechecker.o: typechecker.c typechecker.h ast.h
module.o: module.c module.h interpreter.h ast.h
modules_registry.o: modules_registry.c modules_registry.h module.h
string_mod.o: ../Modules/string_mod.c src/module.h
random_mod.o: ../Modules/random_mod.c src/module.h
atomics_mod.o: ../Modules/atomics_mod.c src/module.h
main.o: main.c lexer.h parser.h interpreter.h
