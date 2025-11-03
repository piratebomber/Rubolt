#!/usr/bin/env bash
# prepare_rbo.sh - Validate and prepare .rbo files for execution

set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <input.rbo> <output.rbo>"
  exit 1
fi

INPUT="$1"
OUTPUT="$2"

if [[ ! -f "$INPUT" ]]; then
  echo "Error: Input file not found: $INPUT"
  exit 1
fi

echo "[prep] Preparing $INPUT -> $OUTPUT"

# Step 1: Validate syntax
echo "[prep] Step 1: Validating syntax..."
if command -v rubolt >/dev/null 2>&1; then
  rubolt --check-syntax "$INPUT" || { echo "Syntax validation failed"; exit 1; }
else
  echo "[prep] Warning: rubolt not in PATH, skipping syntax check"
fi

# Step 2: Normalize line endings (LF)
echo "[prep] Step 2: Normalizing line endings..."
sed $'s/\r$//' "$INPUT" > "$OUTPUT.tmp"

# Step 3: Remove trailing whitespace
echo "[prep] Step 3: Removing trailing whitespace..."
sed 's/[[:space:]]*$//' "$OUTPUT.tmp" > "$OUTPUT.tmp2"

# Step 4: Ensure final newline
echo "[prep] Step 4: Ensuring final newline..."
if [[ -n "$(tail -c 1 "$OUTPUT.tmp2")" ]]; then
  echo "" >> "$OUTPUT.tmp2"
fi

mv "$OUTPUT.tmp2" "$OUTPUT"
rm -f "$OUTPUT.tmp"

echo "[prep] Done: $OUTPUT"
