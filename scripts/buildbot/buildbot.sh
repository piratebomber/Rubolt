#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

bash "$ROOT_DIR/scripts/linux/build.sh"
python3 "$ROOT_DIR/tools/run_tests.py"
python3 "$ROOT_DIR/tools/run_benchmarks.py"

echo "Generating coverage (gcovr)..."
mkdir -p "$ROOT_DIR/build/coverage"
# Example: if built with coverage flags
if command -v gcovr >/dev/null 2>&1; then
  gcovr -r "$ROOT_DIR" --xml -o "$ROOT_DIR/build/coverage/coverage.xml" || true
  gcovr -r "$ROOT_DIR" --html --html-details -o "$ROOT_DIR/build/coverage/index.html" || true
fi

echo "Buildbot completed"
