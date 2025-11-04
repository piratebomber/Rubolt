#!/usr/bin/env bash
set -euo pipefail

OUT_DIR=dist

while [[ $# -gt 0 ]]; do
  case "$1" in
    --out) OUT_DIR="$2"; shift 2;;
    *) echo "Unknown arg: $1"; exit 1;;
  esac
done

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR/bin" "$OUT_DIR/lib" "$OUT_DIR/share"

# Build if needed
if [[ ! -f src/rubolt && -f scripts/linux/build.sh ]]; then
  bash scripts/linux/build.sh || true
fi

# Bin
[[ -f src/rubolt ]] && install -m 0755 src/rubolt "$OUT_DIR/bin/rubolt"
[[ -f rbcli ]] && install -m 0755 rbcli "$OUT_DIR/bin/rbcli" || true
[[ -f cli/rbcli ]] && install -m 0755 cli/rbcli "$OUT_DIR/bin/rbcli" || true

# Lib content
for d in StdLib Objects Modules; do [[ -d "$d" ]] && cp -a "$d" "$OUT_DIR/lib/"; done

# Share content
for d in Docs examples vscode-rubolt python vendor; do [[ -d "$d" ]] && cp -a "$d" "$OUT_DIR/share/"; done

for f in LICENSE README.md; do [[ -f "$f" ]] && cp -a "$f" "$OUT_DIR/share/"; done

echo "[Package] dist prepared at $OUT_DIR"
