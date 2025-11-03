# Prep Scripts

Bash scripts for preparing `.rbo` files and managing Python loader modifications.

## Scripts

### .rbo Preparation

- `prepare_rbo.sh` — Validate and prepare .rbo files for execution
- `minify_rbo.sh` — Minify .rbo source by removing comments and whitespace
- `bundle_rbo.sh` — Bundle multiple .rbo files into a single distributable
- `validate_syntax.sh` — Syntax validation for .rbo files
- `generate_metadata.sh` — Extract metadata (functions, imports, exports)

### Python Loader Modifications

- `inject_py_loader.sh` — Inject custom Rubolt loader into Python modules
- `wrap_py_module.sh` — Wrap Python modules for Rubolt compatibility
- `generate_py_bridge.sh` — Generate Python-Rubolt bridge code
- `patch_py_import.sh` — Patch Python import system for .rbo loading

## Usage

### Prepare .rbo file

```bash
bash src/prep/prepare_rbo.sh input.rbo output.rbo
```

### Minify for distribution

```bash
bash src/prep/minify_rbo.sh source.rbo minified.rbo
```

### Bundle multiple files

```bash
bash src/prep/bundle_rbo.sh main.rbo lib1.rbo lib2.rbo -o bundle.rbo
```

### Generate Python bridge

```bash
bash src/prep/generate_py_bridge.sh mymodule.py mymodule_bridge.c
```

### Inject Python loader

```bash
bash src/prep/inject_py_loader.sh target_module.py
```
