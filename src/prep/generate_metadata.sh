#!/usr/bin/env bash
# generate_metadata.sh - Extract metadata from .rbo files

set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <file.rbo> [output.json]"
  exit 1
fi

INPUT="$1"
OUTPUT="${2:-${INPUT%.rbo}.meta.json}"

if [[ ! -f "$INPUT" ]]; then
  echo "Error: File not found: $INPUT"
  exit 1
fi

echo "[metadata] Extracting from $INPUT -> $OUTPUT"

# Extract functions
FUNCS=$(grep -oP '^\s*func\s+\K\w+' "$INPUT" || echo "")

# Extract imports
IMPORTS=$(grep -oP '^\s*import\s+\K[\w.]+' "$INPUT" || echo "")

# Extract exports
EXPORTS=$(grep -oP '^\s*export\s+\K.*' "$INPUT" | tr ',' '\n' | sed 's/^[[:space:]]*//; s/[[:space:]]*$//' || echo "")

# Count lines
TOTAL_LINES=$(wc -l < "$INPUT")
CODE_LINES=$(grep -v '^[[:space:]]*#' "$INPUT" | grep -v '^[[:space:]]*$' | wc -l)

# Generate JSON
cat > "$OUTPUT" <<EOF
{
  "file": "$INPUT",
  "lines": {
    "total": $TOTAL_LINES,
    "code": $CODE_LINES
  },
  "functions": [
EOF

first=true
for func in $FUNCS; do
  if [[ "$first" = true ]]; then
    first=false
  else
    echo "," >> "$OUTPUT"
  fi
  echo -n "    \"$func\"" >> "$OUTPUT"
done

echo "" >> "$OUTPUT"
echo "  ]," >> "$OUTPUT"
echo "  \"imports\": [" >> "$OUTPUT"

first=true
for imp in $IMPORTS; do
  if [[ "$first" = true ]]; then
    first=false
  else
    echo "," >> "$OUTPUT"
  fi
  echo -n "    \"$imp\"" >> "$OUTPUT"
done

echo "" >> "$OUTPUT"
echo "  ]," >> "$OUTPUT"
echo "  \"exports\": [" >> "$OUTPUT"

first=true
for exp in $EXPORTS; do
  if [[ "$first" = true ]]; then
    first=false
  else
    echo "," >> "$OUTPUT"
  fi
  echo -n "    \"$exp\"" >> "$OUTPUT"
done

echo "" >> "$OUTPUT"
echo "  ]" >> "$OUTPUT"
echo "}" >> "$OUTPUT"

echo "[metadata] Metadata written to $OUTPUT"
