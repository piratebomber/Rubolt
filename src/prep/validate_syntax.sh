#!/usr/bin/env bash
# validate_syntax.sh - Syntax validation for .rbo files

set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <file.rbo> [file2.rbo ...]"
  exit 1
fi

ERRORS=0

for FILE in "$@"; do
  if [[ ! -f "$FILE" ]]; then
    echo "Error: File not found: $FILE"
    ((ERRORS++))
    continue
  fi
  
  echo "[validate] Checking $FILE..."
  
  # Use rubolt if available, otherwise basic checks
  if command -v rubolt >/dev/null 2>&1; then
    if rubolt --check-syntax "$FILE" 2>&1; then
      echo "[validate] ✓ $FILE"
    else
      echo "[validate] ✗ $FILE - Syntax errors detected"
      ((ERRORS++))
    fi
  else
    # Fallback: basic checks
    # Check for balanced braces
    OPEN=$(grep -o '{' "$FILE" | wc -l || echo 0)
    CLOSE=$(grep -o '}' "$FILE" | wc -l || echo 0)
    
    if [[ $OPEN -ne $CLOSE ]]; then
      echo "[validate] ✗ $FILE - Unbalanced braces (open: $OPEN, close: $CLOSE)"
      ((ERRORS++))
    else
      echo "[validate] ✓ $FILE (basic check)"
    fi
  fi
done

if [[ $ERRORS -eq 0 ]]; then
  echo "[validate] All files passed validation"
else
  echo "[validate] $ERRORS file(s) failed validation"
  exit 1
fi
