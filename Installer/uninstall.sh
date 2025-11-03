#!/usr/bin/env bash
set -euo pipefail

PREFIX=/usr/local

while [[ $# -gt 0 ]]; do
  case "$1" in
    --prefix) PREFIX="$2"; shift 2;;
    *) echo "Unknown arg: $1"; exit 1;;
  esac
done

bin="$PREFIX/bin"
lib="$PREFIX/lib/rubolt"
share="$PREFIX/share/rubolt"

for p in "$bin" "$lib" "$share"; do
  if [[ -e "$p" ]]; then
    echo "Removing $p"
    rm -rf "$p"
  fi
done

echo "Uninstall complete. You may remove $bin from PATH in your shell rc if added manually.
