#!/usr/bin/env python3
"""
analyze_project.py - Analyze Rubolt project for C headers and Python imports
Outputs a merged manifest JSON to stdout or file.
"""

import os
import re
import sys
import json
import argparse
from pathlib import Path

HEADER_PAT = re.compile(r"#\s*include\s*[<\"]([^>\"]+)[>\"]")
PY_IMPORT_PAT = re.compile(r"^(?:from\s+([\w\.]+)\s+import|import\s+([\w\.]+))", re.M)

HEADER_TO_VENDOR = {
    'sqlite3.h': 'sqlite',
    'curl/curl.h': 'curl',
    'zlib.h': 'zlib',
    'openssl/ssl.h': 'openssl',
    'pcre2.h': 'pcre2',
}

SEARCH_DIRS = [
    'src', 'runtime', 'Modules', 'module', 'include', 'shared', 'python'
]

PY_EXTS = ('.py',)
C_EXTS = ('.c', '.h', '.hpp', '.cc', '.cpp')
RBO_EXTS = ('.rbo',)


def scan_headers(root: Path):
    headers = set()
    for base in SEARCH_DIRS:
        d = root / base
        if not d.exists():
            continue
        for path in d.rglob('*'):
            if path.suffix.lower() in C_EXTS:
                try:
                    text = path.read_text(errors='ignore')
                except Exception:
                    continue
                for m in HEADER_PAT.finditer(text):
                    headers.add(m.group(1))
    return sorted(headers)


def scan_python_imports(root: Path):
    modules = set()
    for base in SEARCH_DIRS:
        d = root / base
        if not d.exists():
            continue
        for path in d.rglob('*'):
            if path.suffix.lower() in PY_EXTS:
                try:
                    text = path.read_text(errors='ignore')
                except Exception:
                    continue
                for m in PY_IMPORT_PAT.finditer(text):
                    mod = m.group(1) or m.group(2)
                    if mod:
                        modules.add(mod.split('.')[0])
    # Remove obvious stdlib guesses (very rough)
    stdlib_like = {'sys','os','time','json','re','subprocess','pathlib','typing','ctypes','argparse','shutil','tempfile'}
    modules = [m for m in modules if m not in stdlib_like]
    modules.sort()
    return modules


def build_vendors_from_headers(headers):
    vendors = set()
    for h in headers:
        if h in HEADER_TO_VENDOR:
            vendors.add(HEADER_TO_VENDOR[h])
    return sorted(vendors)


def merge_manifest(base_manifest, detected):
    def uniq(lst, key=None):
        seen = set()
        out = []
        for item in lst:
            k = item if key is None else item.get(key, json.dumps(item, sort_keys=True))
            if k not in seen:
                seen.add(k)
                out.append(item)
        return out

    result = {
        'vendors': uniq((base_manifest.get('vendors') or []) + [{'name': v} for v in detected['vendors']], key='name'),
        'python': uniq((base_manifest.get('python') or []) + [{'name': p} for p in detected['python']], key='name'),
        'includes': sorted(set((base_manifest.get('includes') or []) + detected['includes']))
    }
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest', default='vendor/manifest.json')
    ap.add_argument('--out', default='vendor/generated_manifest.json')
    args = ap.parse_args()

    root = Path.cwd()

    headers = scan_headers(root)
    py_mods = scan_python_imports(root)
    vendors = build_vendors_from_headers(headers)

    detected = { 'vendors': vendors, 'python': py_mods, 'includes': headers }

    base_manifest = {}
    mf = root / args.manifest
    if mf.exists():
        base_manifest = json.loads(mf.read_text())

    merged = merge_manifest(base_manifest, detected)

    Path(args.out).write_text(json.dumps(merged, indent=2))
    print(json.dumps(merged, indent=2))


if __name__ == '__main__':
    main()
