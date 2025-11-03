#include <stdio.h>
#include <stdlib.h>
#include "../src/bc_compiler.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <in.rbo> <out.bin>\n", argv[0]);
        return 1;
    }
    return bc_compile_file(argv[1], argv[2]);
}
