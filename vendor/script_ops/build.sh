#!/usr/bin/env bash
set -euo pipefail

PREFIX=${1:-vendor}
OUTDIR="$PREFIX/bin"
mkdir -p "$OUTDIR"

cc_bin=""
if command -v gcc >/dev/null 2>&1; then cc_bin=gcc
elif command -v clang >/dev/null 2>&1; then cc_bin=clang
else
  echo "No gcc/clang found; skipping script_ops build"; exit 0
fi

uname_s=$(uname -s)
case "$uname_s" in
  Darwin*) soext=dylib; soflag="-dynamiclib";;
  Linux*) soext=so; soflag="-shared";;
  MINGW*|MSYS*|CYGWIN*) soext=dll; soflag="-shared";;
  *) soext=so; soflag="-shared";;
 esac

$cc_bin $soflag -O2 -fPIC vendor/script_ops/script_ops.c -Ivendor/script_ops -o "$OUTDIR/script_ops.$soext"

echo "Built $OUTDIR/script_ops.$soext"
