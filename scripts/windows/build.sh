#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
echo "[Windows] Building Rubolt (via MSYS/MinGW bash)..."

pushd "$ROOT_DIR/src" >/dev/null

echo "Compiling core..."
gcc -Wall -Wextra -std=c11 -O2 -c main.c -o main.o
gcc -Wall -Wextra -std=c11 -O2 -c lexer.c -o lexer.o
gcc -Wall -Wextra -std=c11 -O2 -c parser.c -o parser.o
gcc -Wall -Wextra -std=c11 -O2 -c ast.c -o ast.o
gcc -Wall -Wextra -std=c11 -O2 -c interpreter.c -o interpreter.o
gcc -Wall -Wextra -std=c11 -O2 -c typechecker.c -o typechecker.o
gcc -Wall -Wextra -std=c11 -O2 -c module.c -o module.o
gcc -Wall -Wextra -std=c11 -O2 -c modules_registry.c -o modules_registry.o

echo "Compiling modules..."
gcc -Wall -Wextra -std=c11 -O2 -c ../Modules/string_mod.c -o string_mod.o
gcc -Wall -Wextra -std=c11 -O2 -c ../Modules/random_mod.c -o random_mod.o
gcc -Wall -Wextra -std=c11 -O2 -c ../Modules/atomics_mod.c -o atomics_mod.o

echo "Linking..."
gcc main.o lexer.o parser.o ast.o interpreter.o typechecker.o module.o modules_registry.o \
    string_mod.o random_mod.o atomics_mod.o -o rubolt.exe -lm

popd >/dev/null

echo "Done. Binary at src/rubolt.exe"
