CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
INCLUDES = -I./src -I./gc -I./rc -I./Modules
LIBS = -lcurl -ljson-c -lm -lpthread
LDFLAGS = -rdynamic

# Source directories
SRC_DIR = src
GC_DIR = gc
RC_DIR = rc
MODULES_DIR = Modules
TOOLS_DIR = tools
CLI_DIR = cli

# Core source files
CORE_SOURCES = $(wildcard $(SRC_DIR)/*.c)
GC_SOURCES = $(wildcard $(GC_DIR)/*.c)
RC_SOURCES = $(wildcard $(RC_DIR)/*.c)
MODULE_SOURCES = $(wildcard $(MODULES_DIR)/*.c)

# Object files
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)
GC_OBJECTS = $(GC_SOURCES:.c=.o)
RC_OBJECTS = $(RC_SOURCES:.c=.o)
MODULE_OBJECTS = $(MODULE_SOURCES:.c=.o)

ALL_OBJECTS = $(CORE_OBJECTS) $(GC_OBJECTS) $(RC_OBJECTS) $(MODULE_OBJECTS)

# Executables
RBCLI = rbcli
FORMATTER = $(TOOLS_DIR)/formatter
LINTER = $(TOOLS_DIR)/linter
LSP_SERVER = $(TOOLS_DIR)/rubolt-lsp

# Default target
all: $(RBCLI) $(FORMATTER) $(LINTER) $(LSP_SERVER)

# Core library
libruntime.a: $(ALL_OBJECTS)
	ar rcs $@ $^

# CLI executable
$(RBCLI): $(CLI_DIR)/rbcli.c libruntime.a
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lruntime $(LIBS) -o $@

# Tools
$(FORMATTER): $(TOOLS_DIR)/formatter.c libruntime.a
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lruntime $(LIBS) -o $@

$(LINTER): $(TOOLS_DIR)/linter.c libruntime.a
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lruntime $(LIBS) -o $@

$(LSP_SERVER): $(TOOLS_DIR)/rubolt_lsp.c libruntime.a
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -lruntime $(LIBS) -o $@

# Object file compilation
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Special compilation for modules with external dependencies
$(MODULES_DIR)/http_mod.o: $(MODULES_DIR)/http_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS)

$(MODULES_DIR)/json_mod.o: $(MODULES_DIR)/json_mod.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS)

# Tests
test: all
	@echo "Running Rubolt test suite..."
	./$(RBCLI) test
	@echo "Running formatter tests..."
	./$(FORMATTER) examples/hello.rbo --output /tmp/formatted.rbo
	@echo "Running linter tests..."
	./$(LINTER) examples/hello.rbo

# VS Code extension
vscode-extension:
	cd vscode-rubolt && npm install && npm run compile

# Installation
install: all
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(RBCLI) $(DESTDIR)/usr/local/bin/
	install -m 755 $(FORMATTER) $(DESTDIR)/usr/local/bin/rbformat
	install -m 755 $(LINTER) $(DESTDIR)/usr/local/bin/rblint
	install -m 755 $(LSP_SERVER) $(DESTDIR)/usr/local/bin/

install-all: install vscode-extension
	@echo "Installing VS Code extension..."
	cd vscode-rubolt && vsce package && code --install-extension *.vsix

# Development targets
debug: CFLAGS += -DDEBUG -g3 -O0
debug: all

release: CFLAGS += -DNDEBUG -O3 -flto
release: all

# Memory debugging
valgrind-test: debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(RBCLI) run examples/stdlib_demo.rbo

# Profiling
profile: CFLAGS += -pg
profile: all

# Documentation
docs:
	doxygen Doxyfile

# Benchmarks
benchmark: all
	@echo "Running performance benchmarks..."
	./$(RBCLI) run benchmarks/loop.rbo
	./$(RBCLI) run benchmarks/recursion.rbo
	./$(RBCLI) run benchmarks/io.rbo

# Clean targets
clean:
	rm -f $(ALL_OBJECTS)
	rm -f libruntime.a
	rm -f $(RBCLI) $(FORMATTER) $(LINTER) $(LSP_SERVER)
	rm -f *.core core.*
	cd vscode-rubolt && rm -rf node_modules out *.vsix

clean-all: clean
	rm -rf build/
	rm -rf .rubolt/

# Development utilities
format-code:
	find src -name "*.c" -o -name "*.h" | xargs clang-format -i

lint-code:
	find src -name "*.c" | xargs cppcheck --enable=all

# Package creation
package: release
	mkdir -p build/rubolt-$(VERSION)
	cp -r src gc rc Modules tools cli examples StdLib vscode-rubolt build/rubolt-$(VERSION)/
	cp README.md LICENSE Makefile build/rubolt-$(VERSION)/
	cd build && tar czf rubolt-$(VERSION).tar.gz rubolt-$(VERSION)

# Dependency tracking
depend:
	$(CC) $(CFLAGS) $(INCLUDES) -MM $(CORE_SOURCES) $(GC_SOURCES) $(RC_SOURCES) $(MODULE_SOURCES) > .depend

-include .depend

.PHONY: all test install install-all debug release valgrind-test profile docs benchmark clean clean-all format-code lint-code package depend vscode-extension