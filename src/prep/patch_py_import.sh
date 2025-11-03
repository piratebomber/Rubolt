#!/usr/bin/env bash
# patch_py_import.sh - Patch Python import system for .rbo loading

set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <target_script.py> [--output patched.py]"
  exit 1
fi

INPUT="$1"
OUTPUT="$INPUT"

if [[ "$#" -ge 3 && "$2" == "--output" ]]; then
  OUTPUT="$3"
fi

if [[ ! -f "$INPUT" ]]; then
  echo "Error: Script not found: $INPUT"
  exit 1
fi

echo "[patch] Patching import system in $INPUT"

PATCH_CODE='
# --- Rubolt Import System Patch ---
import sys
import os
from importlib.abc import MetaPathFinder, Loader
from importlib.machinery import ModuleSpec

class RuboltImporter(MetaPathFinder, Loader):
    """Meta path finder for .rbo modules"""
    
    def find_spec(self, fullname, path, target=None):
        """Find .rbo module spec"""
        if path is None:
            path = sys.path
        
        for dir_path in path:
            rbo_path = os.path.join(dir_path, fullname.replace(".", os.sep) + ".rbo")
            if os.path.exists(rbo_path):
                return ModuleSpec(fullname, self, origin=rbo_path)
        return None
    
    def create_module(self, spec):
        """Create empty module"""
        return None  # Use default module creation
    
    def exec_module(self, module):
        """Execute .rbo module via Rubolt interpreter"""
        import subprocess
        import types
        
        rbo_path = module.__spec__.origin
        result = subprocess.run(
            ["rubolt", "--export-python", rbo_path],
            capture_output=True, text=True
        )
        
        if result.returncode != 0:
            raise ImportError(f"Failed to execute .rbo module: {rbo_path}")
        
        # Execute exported Python code in module namespace
        exec(result.stdout, module.__dict__)

# Install the importer
sys.meta_path.insert(0, RuboltImporter())
# --- End Rubolt Import Patch ---
'

# Check if already patched
if grep -q "Rubolt Import System Patch" "$INPUT"; then
  echo "[patch] Script already patched, skipping"
  exit 0
fi

# Create patched version
{
  # Preserve shebang/encoding
  grep -E '^#!|^#.*coding[:=]' "$INPUT" || true
  echo "$PATCH_CODE"
  grep -vE '^#!|^#.*coding[:=]' "$INPUT"
} > "$OUTPUT.tmp"

mv "$OUTPUT.tmp" "$OUTPUT"

echo "[patch] Import system patched: $OUTPUT"
echo "[patch] Python scripts can now import .rbo modules directly"
