CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
INCLUDES = -I../src -I../gc -I../rc
LIBS = -ljson-c

# Language Server
LSP_SOURCES = rubolt_lsp.c
LSP_TARGET = rubolt-lsp

# Other tools
TOOLS = $(LSP_TARGET) rbcompile c_analyzer

all: $(TOOLS)

# Language Server Protocol implementation
$(LSP_TARGET): $(LSP_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

# Existing tools
rbcompile: rbcompile.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

c_analyzer: c_analyzer.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

# Install language server
install-lsp: $(LSP_TARGET)
	cp $(LSP_TARGET) /usr/local/bin/
	chmod +x /usr/local/bin/$(LSP_TARGET)

# Clean
clean:
	rm -f $(TOOLS) *.o

.PHONY: all install-lsp clean