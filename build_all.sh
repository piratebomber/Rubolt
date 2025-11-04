#!/bin/bash

# Rubolt Complete Build Script for Linux/macOS
# Builds interpreter, CLI, REPL, and all components

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Header
echo "╔═══════════════════════════════════════╗"
echo "║  Building Complete Rubolt Toolchain   ║"
echo "╚═══════════════════════════════════════╝"
echo ""

# Detect OS
OS="$(uname -s)"
case "${OS}" in
    Linux*)     MACHINE=Linux;;
    Darwin*)    MACHINE=Mac;;
    *)          MACHINE="UNKNOWN:${OS}"
esac
echo -e "${BLUE}Detected OS: ${MACHINE}${NC}"
echo ""

# Compiler setup
if command -v gcc &> /dev/null; then
    CC=gcc
    echo -e "${GREEN}Using GCC${NC}"
elif command -v clang &> /dev/null; then
    CC=clang
    echo -e "${GREEN}Using Clang${NC}"
else
    echo -e "${RED}Error: No C compiler found (gcc or clang required)${NC}"
    exit 1
fi

# Build flags
CFLAGS="-Wall -Wextra -std=c11 -O2"
LDFLAGS="-lm"

# Platform-specific settings
if [ "$MACHINE" = "Mac" ]; then
    LDFLAGS="$LDFLAGS -framework CoreFoundation"
fi

# Step 1: Build Main Interpreter
echo "[1/4] Building Rubolt interpreter..."
cd src

echo "Compiling core components..."
$CC $CFLAGS -c main.c -o main.o
$CC $CFLAGS -c lexer.c -o lexer.o
$CC $CFLAGS -c parser.c -o parser.o
$CC $CFLAGS -c ast.c -o ast.o
$CC $CFLAGS -c interpreter.c -o interpreter.o
$CC $CFLAGS -c typechecker.c -o typechecker.o
$CC $CFLAGS -c module.c -o module.o
$CC $CFLAGS -c modules_registry.c -o modules_registry.o
$CC $CFLAGS -c dll_loader.c -o dll_loader.o
$CC $CFLAGS -c dll_import.c -o dll_import.o
$CC $CFLAGS -c native_registry.c -o native_registry.o
$CC $CFLAGS -c bc_compiler.c -o bc_compiler.o
$CC $CFLAGS -c vm.c -o vm.o

echo "Compiling runtime and virtual environment..."
$CC $CFLAGS -c ../bopes/bopes.c -o bopes.o -I..
$CC $CFLAGS -c ../runtime/manager.c -o manager.o -I..

echo "Compiling memory management..."
$CC $CFLAGS -c ../gc/gc.c -o gc.o -I..
$CC $CFLAGS -c ../gc/type_info.c -o type_info.o -I..
$CC $CFLAGS -c ../rc/rc.c -o rc.o -I..

echo "Compiling advanced features (exceptions, async, threading)..."
$CC $CFLAGS -c exception.c -o exception.o 2>/dev/null || echo "exception.c not yet implemented"
$CC $CFLAGS -c debugger.c -o debugger.o 2>/dev/null || echo "debugger.c not yet implemented"
$CC $CFLAGS -c profiler.c -o profiler.o 2>/dev/null || echo "profiler.c not yet implemented"
$CC $CFLAGS -c jit_compiler.c -o jit_compiler.o 2>/dev/null || echo "jit_compiler.c not yet implemented"
$CC $CFLAGS -c inline_cache.c -o inline_cache.o 2>/dev/null || echo "inline_cache.c not yet implemented"
$CC $CFLAGS -c python_bridge.c -o python_bridge.o 2>/dev/null || echo "python_bridge.c not yet implemented"
$CC $CFLAGS -c async.c -o async.o 2>/dev/null || echo "async.c not yet implemented"
$CC $CFLAGS -c event_loop.c -o event_loop.o 2>/dev/null || echo "event_loop.c not yet implemented"
$CC $CFLAGS -c threading.c -o threading.o 2>/dev/null || echo "threading.c not yet implemented"

echo "Compiling collections..."
$CC $CFLAGS -c ../collections/rb_collections.c -o rb_collections.o -I..
$CC $CFLAGS -c ../collections/rb_list.c -o rb_list.o -I..

echo "Compiling C modules in Modules/..."
$CC $CFLAGS -c ../Modules/string_mod.c -o string_mod.o 2>/dev/null || echo "string_mod.c not found"
$CC $CFLAGS -c ../Modules/random_mod.c -o random_mod.o 2>/dev/null || echo "random_mod.c not found"
$CC $CFLAGS -c ../Modules/atomics_mod.c -o atomics_mod.o 2>/dev/null || echo "atomics_mod.c not found"

echo "Linking all components..."
OBJS="main.o lexer.o parser.o ast.o interpreter.o typechecker.o module.o modules_registry.o bc_compiler.o vm.o"
OBJS="$OBJS dll_loader.o dll_import.o native_registry.o"
OBJS="$OBJS manager.o bopes.o"
OBJS="$OBJS gc.o type_info.o rc.o"
OBJS="$OBJS rb_collections.o rb_list.o"

# Add optional objects if they exist
[ -f exception.o ] && OBJS="$OBJS exception.o"
[ -f debugger.o ] && OBJS="$OBJS debugger.o"
[ -f profiler.o ] && OBJS="$OBJS profiler.o"
[ -f jit_compiler.o ] && OBJS="$OBJS jit_compiler.o"
[ -f inline_cache.o ] && OBJS="$OBJS inline_cache.o"
[ -f python_bridge.o ] && OBJS="$OBJS python_bridge.o"
[ -f async.o ] && OBJS="$OBJS async.o"
[ -f event_loop.o ] && OBJS="$OBJS event_loop.o"
[ -f threading.o ] && OBJS="$OBJS threading.o"
[ -f string_mod.o ] && OBJS="$OBJS string_mod.o"
[ -f random_mod.o ] && OBJS="$OBJS random_mod.o"
[ -f atomics_mod.o ] && OBJS="$OBJS atomics_mod.o"

$CC $OBJS -o rubolt $LDFLAGS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Interpreter built: src/rubolt${NC}"
else
    echo -e "${RED}✗ Failed to build interpreter${NC}"
    cd ..
    exit 1
fi

cd ..

# Step 2: Build CLI Tool
echo ""
echo "[2/4] Building Rubolt CLI tool..."
cd cli

$CC $CFLAGS rbcli.c -o rbcli $LDFLAGS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ CLI built: cli/rbcli${NC}"
    cp rbcli ../rbcli
else
    echo -e "${RED}✗ Failed to build CLI${NC}"
    cd ..
    exit 1
fi

cd ..

# Step 3: Build REPL
echo ""
echo "[3/4] Building Rubolt REPL..."
cd repl

echo "Compiling REPL components..."
$CC $CFLAGS -c repl.c -o repl.o -I..

# Create repl_main.c if it doesn't exist
if [ ! -f repl_main.c ]; then
    echo "Creating repl_main.c..."
    cat > repl_main.c << 'EOF'
#include "repl.h"
#include <stdio.h>

int main(void) {
    ReplState repl;
    repl_init(&repl);
    repl_run(&repl);
    repl_shutdown(&repl);
    return 0;
}
EOF
fi

$CC $CFLAGS -c repl_main.c -o repl_main.o -I..

echo "Linking REPL with interpreter..."
REPL_OBJS="repl.o repl_main.o"
REPL_OBJS="$REPL_OBJS ../src/lexer.o ../src/parser.o ../src/ast.o ../src/interpreter.o"
REPL_OBJS="$REPL_OBJS ../src/typechecker.o ../src/module.o ../src/modules_registry.o"
REPL_OBJS="$REPL_OBJS ../src/dll_loader.o ../src/dll_import.o ../src/native_registry.o"
REPL_OBJS="$REPL_OBJS ../src/bc_compiler.o ../src/vm.o"
REPL_OBJS="$REPL_OBJS ../bopes/bopes.o ../runtime/manager.o"
REPL_OBJS="$REPL_OBJS ../gc/gc.o ../gc/type_info.o ../rc/rc.o"
REPL_OBJS="$REPL_OBJS ../collections/rb_collections.o ../collections/rb_list.o"

# Add optional objects
[ -f ../src/exception.o ] && REPL_OBJS="$REPL_OBJS ../src/exception.o"
[ -f ../src/debugger.o ] && REPL_OBJS="$REPL_OBJS ../src/debugger.o"
[ -f ../src/profiler.o ] && REPL_OBJS="$REPL_OBJS ../src/profiler.o"
[ -f ../Modules/string_mod.o ] && REPL_OBJS="$REPL_OBJS ../Modules/string_mod.o"
[ -f ../Modules/random_mod.o ] && REPL_OBJS="$REPL_OBJS ../Modules/random_mod.o"
[ -f ../Modules/atomics_mod.o ] && REPL_OBJS="$REPL_OBJS ../Modules/atomics_mod.o"

$CC $REPL_OBJS -o rubolt-repl $LDFLAGS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ REPL built: repl/rubolt-repl${NC}"
    cp rubolt-repl ../rubolt-repl
else
    echo -e "${RED}✗ Failed to build REPL${NC}"
    cd ..
    exit 1
fi

cd ..

# Step 4: Setup directories
echo ""
echo "[4/4] Setting up directories..."
mkdir -p lib
mkdir -p stdlib
mkdir -p build
echo -e "${GREEN}✓ Directories created${NC}"

# Final summary
echo ""
echo "╔═══════════════════════════════════════╗"
echo "║     Build Completed Successfully!     ║"
echo "╚═══════════════════════════════════════╝"
echo ""
echo "Executables:"
echo "  src/rubolt        - Rubolt interpreter"
echo "  rbcli             - CLI tool"
echo "  rubolt-repl       - Interactive REPL shell"
echo ""
echo "Quick start:"
echo "  ./rbcli init myproject"
echo "  ./rbcli run examples/hello.rbo"
echo "  ./rubolt-repl     - Start interactive shell"
echo "  ./rbcli newlib mylib"
echo ""

# Make executables executable
chmod +x src/rubolt rbcli rubolt-repl 2>/dev/null || true

echo -e "${GREEN}Build complete!${NC}"
