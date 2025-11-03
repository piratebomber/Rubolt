#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

# Clean build outputs
pushd "$ROOT_DIR/src" >/dev/null
rm -f *.o rubolt rubolt.exe
popd >/dev/null

echo "Cleaned build artifacts."
