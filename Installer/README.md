# Rubolt Installer

Install Rubolt from source, a local build (build/*), or a prepacked dist (dist/*).

## Quick start (Windows, PowerShell)

- Install from build or dist automatically to `%USERPROFILE%\Rubolt`:

```powershell
pwsh -File Installer/install.ps1 -Mode auto -Prefix "$env:USERPROFILE\Rubolt" -AddToPath
```

- Install the full source and build it:

```powershell
pwsh -File Installer/install.ps1 -Mode source -Build -Prefix "$env:USERPROFILE\Rubolt-src" -AddToPath
```

- Uninstall:

```powershell
pwsh -File Installer/uninstall.ps1 -Prefix "$env:USERPROFILE\Rubolt"
```

## Quick start (macOS/Linux)

- Install from build or dist to `/usr/local` (may prompt for sudo):

```bash
bash Installer/install.sh --mode auto --prefix /usr/local --add-to-path
```

- Uninstall:

```bash
bash Installer/uninstall.sh --prefix /usr/local
```

## Modes

- `auto`  — prefer `dist/` then `build/`, else fallback to `source`
- `dist`  — install from prepacked `dist/` layout
- `build` — install from locally built artifacts (e.g., `src/rubolt`, `rbcli`)
- `source`— copy the entire repo and optionally build

## Layout installed

- `<prefix>/bin/`      — rubolt, rbcli and helpers
- `<prefix>/lib/rubolt`— StdLib, Objects, Modules, Grammar tests (optional)
- `<prefix>/share/rubolt` — docs, examples, vscode extension
