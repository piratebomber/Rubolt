#!/usr/bin/env bash
set -euo pipefail

ANALYZE=0
PREFIX="vendor"
MANIFEST="vendor/manifest.json"
PYTHON_BIN=${PYTHON:-python3}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --analyze) ANALYZE=1; shift;;
    --prefix) PREFIX="$2"; shift 2;;
    --manifest) MANIFEST="$2"; shift 2;;
    *) echo "Unknown arg: $1"; exit 1;;
  esac
done

root_dir=$(pwd)
include_dir="$PREFIX/include"
lib_dir="$PREFIX/lib"
bin_dir="$PREFIX/bin"
src_dir="$PREFIX/src"
py_dir="$PREFIX/python"

mkdir -p "$include_dir" "$lib_dir" "$bin_dir" "$src_dir" "$py_dir"

# Analyze project if requested
if [[ $ANALYZE -eq 1 ]]; then
  $PYTHON_BIN vendor/analyze_project.py --manifest "$MANIFEST" --out "$PREFIX/generated_manifest.json"
  MANIFEST="$PREFIX/generated_manifest.json"
fi

jq_present=0
if command -v jq >/dev/null 2>&1; then jq_present=1; fi

# Read manifest
if [[ ! -f "$MANIFEST" ]]; then
  echo "Manifest not found: $MANIFEST"; exit 1
fi

# Helper: download file
_download() {
  url="$1"; out="$2"
  echo "[vendor] Downloading $url -> $out"
  if command -v curl >/dev/null 2>&1; then
    curl -L "$url" -o "$out"
  elif command -v wget >/dev/null 2>&1; then
    wget -O "$out" "$url"
  else
    echo "No curl/wget found"; return 1
  fi
}

# Helper: extract archive
_extract() {
  file="$1"; dest="$2"
  mkdir -p "$dest"
  case "$file" in
    *.tar.gz|*.tgz) tar -xzf "$file" -C "$dest" ;;
    *.tar.bz2|*.tbz2) tar -xjf "$file" -C "$dest" ;;
    *.zip) unzip -q "$file" -d "$dest" ;;
    *) echo "Unknown archive: $file"; return 1 ;;
  esac
}

# Helper: install python packages
_install_python() {
  name="$1"; ver="$2"
  if [[ -n "$ver" ]]; then
    $PYTHON_BIN -m pip install -t "$py_dir" "$name$ver"
  else
    $PYTHON_BIN -m pip install -t "$py_dir" "$name"
  fi
}

# Install Python deps
if [[ $jq_present -eq 1 ]]; then
  python_len=$(jq '.python | length' "$MANIFEST")
  if [[ "$python_len" != "null" ]]; then
    for i in $(seq 0 $((python_len-1))); do
      name=$(jq -r ".python[$i].name" "$MANIFEST")
      ver=$(jq -r ".python[$i].version // \"\"" "$MANIFEST")
      echo "[vendor] Installing Python package: $name $ver"
      _install_python "$name" "$ver"
    done
  fi
else
  echo "[vendor] jq not found; skipping python installs from manifest"
fi

# Load registry
REG="vendor/registry.json"
if [[ ! -f "$REG" ]]; then
  echo "[vendor] registry.json not found"; REG=""
fi

# Resolve includes to vendor packages
vendors=()
if [[ $jq_present -eq 1 ]]; then
  inc_len=$(jq '.includes | length' "$MANIFEST")
  for i in $(seq 0 $((inc_len-1))); do
    hdr=$(jq -r ".includes[$i]" "$MANIFEST")
    if [[ -n "$REG" ]]; then
      pkg=$(jq -r ".header_map[\"$hdr\"] // \"\"" "$REG")
      if [[ -n "$pkg" && "$pkg" != "" ]]; then vendors+=("$pkg"); fi
    fi
  done
  # Add explicit vendors
  vlen=$(jq '.vendors | length' "$MANIFEST")
  for i in $(seq 0 $((vlen-1))); do
    name=$(jq -r ".vendors[$i].name" "$MANIFEST")
    vendors+=("$name")
  done
else
  echo "[vendor] jq not found; cannot resolve includes -> vendors"
fi

# Dedup vendors
uniq_vendors=()
declare -A seen
for v in "${vendors[@]}"; do
  if [[ -n "$v" && -z "${seen[$v]:-}" ]]; then
    seen[$v]=1
    uniq_vendors+=("$v")
  fi
done

# OS id
os_id="linux"
case "$(uname -s)" in
  Linux*) os_id="linux";;
  Darwin*) os_id="macos";;
  MINGW*|MSYS*|CYGWIN*) os_id="windows";;
esac

# Try to install each vendor
for v in "${uniq_vendors[@]}"; do
  if [[ "$v" == "script_ops" ]]; then
    echo "[vendor] Building script_ops..."
    bash vendor/script_ops/build.sh "$PREFIX" || true
    continue
  fi
  if [[ -z "$REG" ]]; then continue; fi
  echo "[vendor] Installing vendor: $v"
  url=$(jq -r ".packages[\"$v\"].assets.$os_id.url // \"\"" "$REG")
  if [[ -n "$url" && "$url" != "" ]]; then
    archive="$src_dir/$v-archive"
    _download "$url" "$archive"
    _extract "$archive" "$src_dir/$v"
    # Opportunistic copy: include/** -> include_dir, lib/** -> lib_dir, bin/** -> bin_dir
    rsync -a --exclude "*.md" "$src_dir/$v"/ "$src_dir/$v" >/dev/null 2>&1 || true
    find "$src_dir/$v" -type d -name include -exec cp -a {}/. "$include_dir/" \; 2>/dev/null || true
    find "$src_dir/$v" -type d -name lib -exec cp -a {}/. "$lib_dir/" \; 2>/dev/null || true
    find "$src_dir/$v" -type d -name bin -exec cp -a {}/. "$bin_dir/" \; 2>/dev/null || true
    continue
  fi
  # Fallback to package manager
  cmds=$(jq -r ".packages[\"$v\"].pkgman.$os_id[]?" "$REG" || echo "")
  if [[ -n "$cmds" ]]; then
    echo "$cmds" | while IFS= read -r cmd; do
      echo "[vendor] Running: $cmd"
      bash -lc "$cmd" || true
    done
  else
    echo "[vendor] No install method for $v on $os_id"
  fi
done

# Generate environment scripts
cat > "$PREFIX/env.sh" <<EOF
# Source this to use vendor deps
export RUBOLT_VENDOR_PREFIX="$PREFIX"
export RUBOLT_VENDOR_INCLUDE="$include_dir"
export RUBOLT_VENDOR_LIB="$lib_dir"
export RUBOLT_VENDOR_BIN="$bin_dir"
export PYTHONPATH="$py_dir:":${PYTHONPATH}
export PATH="$bin_dir:":${PATH}
EOF

echo "[vendor] Done. Activate with: source $PREFIX/env.sh"
