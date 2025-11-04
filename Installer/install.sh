#!/usr/bin/env bash
set -euo pipefail

MODE=auto
PREFIX=/usr/local
ADD_TO_PATH=0
BUILD=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode) MODE="$2"; shift 2;;
    --prefix) PREFIX="$2"; shift 2;;
    --add-to-path) ADD_TO_PATH=1; shift;;
    --build) BUILD=1; shift;;
    *) echo "Unknown arg: $1"; exit 1;;
  esac
done

bin="$PREFIX/bin"
lib="$PREFIX/lib/rubolt"
share="$PREFIX/share/rubolt"

mkdir -p "$bin" "$lib" "$share"

if [[ "$MODE" == auto ]]; then
  if [[ -d dist ]]; then MODE=dist
  elif [[ -d build ]]; then MODE=build
  else MODE=source; fi
fi

echo "[Installer] mode=$MODE prefix=$PREFIX"

case "$MODE" in
  dist)
    [[ -d dist/bin ]] && cp -a dist/bin/. "$bin/"
    [[ -d dist/lib ]] && cp -a dist/lib/. "$lib/"
    [[ -d dist/share ]] && cp -a dist/share/. "$share/"
    ;;
  build)
    [[ -f src/rubolt ]] && install -m 0755 src/rubolt "$bin/rubolt"
    [[ -f rbcli ]] && install -m 0755 rbcli "$bin/rbcli" || true
    [[ -f cli/rbcli ]] && install -m 0755 cli/rbcli "$bin/rbcli" || true
    for d in StdLib Objects Modules; do [[ -d "$d" ]] && cp -a "$d" "$lib/"; done
    for d in Docs examples vscode-rubolt python vendor; do [[ -d "$d" ]] && cp -a "$d" "$share/"; done
    ;;
  source)
    rsync -a --exclude '.git' . "$PREFIX/Rubolt/"
    if [[ "$BUILD" -eq 1 ]]; then
      (cd "$PREFIX/Rubolt" && bash scripts/linux/build.sh || make -C src || true)
      [[ -f "$PREFIX/Rubolt/src/rubolt" ]] && install -m 0755 "$PREFIX/Rubolt/src/rubolt" "$bin/rubolt"
      [[ -f "$PREFIX/Rubolt/rbcli" ]] && install -m 0755 "$PREFIX/Rubolt/rbcli" "$bin/rbcli"
    fi
    ;;
  *) echo "Unknown mode: $MODE"; exit 1;;
 esac

if [[ $ADD_TO_PATH -eq 1 ]]; then
  if ! command -v rubolt >/dev/null 2>&1; then
    echo "export PATH=\"$bin:\$PATH\"" | sudo tee /etc/profile.d/rubolt.sh >/dev/null || true
    echo "Added to PATH via /etc/profile.d/rubolt.sh (new shells required)"
  fi
fi

echo "[Installer] Done."
