# Vendor System

Automated vendor dependency manager for Rubolt.

- Downloads and installs C headers/libs, Python packages, and tools per OS
- Scans the project to detect needed includes and Python imports
- Builds low-level helpers in C (script_ops) for cross-platform ops

Structure:
- `vendor/install.ps1`, `vendor/install.sh` — main installers
- `vendor/analyze_project.py` — scans sources to produce a manifest
- `vendor/manifest.json` — project vendor manifest (editable)
- `vendor/registry.json` — maps detected headers to vendor packages and install methods
- `vendor/include`, `vendor/lib`, `vendor/bin`, `vendor/python`, `vendor/src` — install roots
- `vendor/script_ops/` — C helpers compiled into a shared library for the host OS
- `vendor/env.ps1`, `vendor/env.sh` — environment setup scripts

Quick start (Windows):

```powershell
pwsh -File vendor/install.ps1 -Analyze -Prefix vendor
# or: pwsh -File vendor/install.ps1 -Manifest vendor/manifest.json -Prefix vendor
```

Quick start (macOS/Linux):

```bash
bash vendor/install.sh --analyze --prefix vendor
# or: bash vendor/install.sh --manifest vendor/manifest.json --prefix vendor
```
