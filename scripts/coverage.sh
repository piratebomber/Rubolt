#!/usr/bin/env bash
set -euo pipefail

# Build with coverage instrumentation
export CFLAGS="-fprofile-arcs -ftest-coverage -O0 -g"
export LDFLAGS="-lgcov"

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

pushd "$ROOT_DIR/src" >/dev/null
make clean || true
make CFLAGS="-Wall -Wextra -std=c11 -O0 -g -fprofile-arcs -ftest-coverage" LDFLAGS="-lgcov"
popd >/dev/null

# Run tests
python3 "$ROOT_DIR/tools/run_tests.py" || true

# Generate coverage report (gcovr)
mkdir -p "$ROOT_DIR/build/coverage"
if command -v gcovr >/dev/null 2>&1; then
  gcovr -r "$ROOT_DIR" --xml -o "$ROOT_DIR/build/coverage/coverage.xml" || true
  gcovr -r "$ROOT_DIR" --html --html-details -o "$ROOT_DIR/build/coverage/index.html" || true
fi

echo "Coverage artifacts in build/coverage"