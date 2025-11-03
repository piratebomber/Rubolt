#!/usr/bin/env bash
# inject_py_loader.sh - Inject Rubolt loader into Python modules

set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <module.py> [--backup]"
  exit 1
fi

MODULE="$1"
BACKUP=false

if [[ "${2:-}" == "--backup" ]]; then
  BACKUP=true
fi

if [[ ! -f "$MODULE" ]]; then
  echo "Error: Module not found: $MODULE"
  exit 1
fi

echo "[inject] Injecting Rubolt loader into $MODULE"

if [[ "$BACKUP" == true ]]; then
  cp "$MODULE" "$MODULE.backup"
  echo "[inject] Backup created: $MODULE.backup"
fi

# Loader code to inject
LOADER_CODE='
# --- Rubolt Loader Injection ---
import sys
import importlib.util

class RuboltLoader:
    """Custom loader for .rbo modules"""
    
    @staticmethod
    def load_rbo_module(path):
        """Load a .rbo file and return module-like object"""
        import subprocess
        result = subprocess.run(["rubolt", path], capture_output=True, text=True)
        if result.returncode != 0:
            raise ImportError(f"Failed to load .rbo module: {path}")
        return result.stdout
    
    @classmethod
    def install(cls):
        """Install the loader into sys.meta_path"""
        if not any(isinstance(imp, cls) for imp in sys.meta_path):
            sys.meta_path.insert(0, cls())

# Auto-install on import
RuboltLoader.install()
# --- End Rubolt Loader ---
'

# Check if already injected
if grep -q "Rubolt Loader Injection" "$MODULE"; then
  echo "[inject] Module already has Rubolt loader, skipping"
  exit 0
fi

# Inject at the top after shebang/encoding
{
  # Preserve shebang and encoding declarations
  grep -E '^#!|^#.*coding[:=]' "$MODULE" || true
  echo "$LOADER_CODE"
  grep -vE '^#!|^#.*coding[:=]' "$MODULE"
} > "$MODULE.tmp"

mv "$MODULE.tmp" "$MODULE"

echo "[inject] Rubolt loader injected successfully"
