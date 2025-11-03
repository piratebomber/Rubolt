#!/usr/bin/env bash
set -euo pipefail

# Dispatch to OS-specific build script
OS=$(uname -s | tr '[:upper:]' '[:lower:]')
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "$OS" in
  linux*)   "$SCRIPT_DIR/linux/build.sh" ;;
  darwin*)  "$SCRIPT_DIR/macos/build.sh" ;;
  msys*|mingw*|cygwin*) "$SCRIPT_DIR/windows/build.sh" ;;
  *) echo "Unsupported OS: $OS" >&2; exit 1 ;;
 esac
