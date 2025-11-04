#!/usr/bin/env bash
set -e
OS=$(uname -s)
if [[ "$OS" == Darwin ]]; then EXT=dylib; else EXT=so; fi
gcc -shared -fPIC -O2 -o "example_math.$EXT" example_math.c
