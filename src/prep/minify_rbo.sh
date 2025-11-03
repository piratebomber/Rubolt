#!/usr/bin/env bash
# minify_rbo.sh - Minify .rbo source code

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

echo "[minify] Minifying $INPUT -> $OUTPUT"

# Remove single-line comments and blank lines
grep -v '^[[:space:]]*#' "$INPUT" | grep -v '^[[:space:]]*$' > "$OUTPUT.tmp"

# Compress multiple spaces (preserve strings)
# Simple version - just strip leading/trailing space per line
sed 's/^[[:space:]]*//; s/[[:space:]]*$//' "$OUTPUT.tmp" > "$OUTPUT"

rm -f "$OUTPUT.tmp"

ORIG_SIZE=$(wc -c < "$INPUT")
MIN_SIZE=$(wc -c < "$OUTPUT")
SAVED=$((ORIG_SIZE - MIN_SIZE))

echo "[minify] Original: $ORIG_SIZE bytes, Minified: $MIN_SIZE bytes (saved $SAVED bytes)"
