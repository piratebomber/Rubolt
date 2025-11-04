#include "repl.h"
#include <stdio.h>

int main(void) {
    ReplState repl;
    repl_init(&repl);
    repl_run(&repl);
    repl_shutdown(&repl);
    return 0;
}
