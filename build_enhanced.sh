#!/bin/bash

echo "Building Enhanced Rubolt with Nested Functions and Loops..."

# Create build directory
mkdir -p build

# Compile core components
echo "Compiling core components..."
gcc -c -Wall -Wextra -std=c99 -O2 -I. src/ast.c -o build/ast.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. src/lexer.c -o build/lexer.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. src/parser.c -o build/parser.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. src/interpreter.c -o build/interpreter.o

# Compile garbage collector and memory management
echo "Compiling memory management..."
gcc -c -Wall -Wextra -std=c99 -O2 -I. gc/gc.c -o build/gc.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. gc/type_info.c -o build/type_info.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. rc/rc.c -o build/rc.o

# Compile runtime and collections
echo "Compiling runtime and collections..."
gcc -c -Wall -Wextra -std=c99 -O2 -I. runtime/runtime.c -o build/runtime.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. runtime/manager.c -o build/manager.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. collections/rb_list.c -o build/rb_list.o
gcc -c -Wall -Wextra -std=c99 -O2 -I. collections/rb_collections.c -o build/rb_collections.o

# Compile main executable
echo "Compiling main executable..."
gcc -c -Wall -Wextra -std=c99 -O2 -I. src/main.c -o build/main.o

# Link everything together
echo "Linking enhanced Rubolt interpreter..."
gcc -Wall -Wextra -std=c99 -O2 build/*.o -o build/rubolt-enhanced -lm

if [ $? -eq 0 ]; then
    echo
    echo "✓ Enhanced Rubolt interpreter built successfully!"
    echo "✓ Executable: build/rubolt-enhanced"
    echo
    echo "Testing with nested functions demo..."
    ./build/rubolt-enhanced examples/test_nested_loops.rbo
else
    echo
    echo "✗ Build failed with errors."
    exit 1
fi

echo
echo "Build complete!"