# Rubolt Language Server

The Rubolt Language Server provides IDE support through the Language Server Protocol (LSP), enabling features like syntax highlighting, code completion, error detection, and more in VS Code and other LSP-compatible editors.

## Features

### Code Completion
- **Keywords**: `def`, `class`, `if`, `else`, `for`, `while`, `return`, `import`, `let`, `const`
- **Built-in Functions**: `print`, `len`, `type`, `str`, `int`, `float`
- **Standard Library Modules**: `file`, `json`, `time`, `http`, `string`
- **Trigger Characters**: `.` for member access, `(` for function calls

### Diagnostics
- **Syntax Errors**: Missing semicolons, malformed expressions
- **Type Warnings**: Missing return type annotations
- **Undefined Variables**: Basic undefined variable detection
- **Real-time Validation**: Errors shown as you type

### Code Formatting
- **Automatic Indentation**: 4-space indentation
- **Brace Alignment**: Consistent brace formatting
- **Line Formatting**: Proper spacing and alignment

### VS Code Integration
- **Run Commands**: Execute Rubolt files directly from VS Code
- **REPL Integration**: Start interactive Rubolt sessions
- **Syntax Highlighting**: Full syntax highlighting for `.rbo` files
- **File Association**: Automatic recognition of Rubolt files

## Installation

### Building the Language Server

**Windows:**
```bash
cd tools
gcc -Wall -Wextra -std=c99 -O2 -I../src rubolt_lsp.c -o rubolt-lsp.exe -ljson-c
```

**Unix/Linux/macOS:**
```bash
cd tools
make rubolt-lsp
sudo make install-lsp  # Optional: install globally
```

### Installing VS Code Extension

1. **Build the Extension:**
   ```bash
   cd vscode-rubolt
   npm install
   npm run compile
   ```

2. **Package the Extension:**
   ```bash
   npm install -g vsce
   vsce package
   ```

3. **Install in VS Code:**
   - Open VS Code
   - Go to Extensions view (`Ctrl+Shift+X`)
   - Click "..." menu → "Install from VSIX..."
   - Select the generated `.vsix` file

### Manual Installation

If you prefer not to package the extension:

1. Copy the `vscode-rubolt` folder to your VS Code extensions directory:
   - **Windows**: `%USERPROFILE%\.vscode\extensions\`
   - **macOS**: `~/.vscode/extensions/`
   - **Linux**: `~/.vscode/extensions/`

2. Restart VS Code

## Configuration

### VS Code Settings

Add these settings to your VS Code `settings.json`:

```json
{
    "rubolt.executablePath": "rbcli",
    "rubolt.enableLSP": true,
    "[rubolt]": {
        "editor.tabSize": 4,
        "editor.insertSpaces": true,
        "editor.formatOnSave": true
    }
}
```

### Language Server Configuration

The language server can be configured through LSP client settings:

```json
{
    "rubolt.languageServer": {
        "maxNumberOfProblems": 1000,
        "trace.server": "verbose"
    }
}
```

## Usage

### Running Rubolt Files

1. Open a `.rbo` file in VS Code
2. Use `Ctrl+Shift+P` → "Rubolt: Run File"
3. Or use the command palette: `Rubolt: Run Rubolt File`

### Starting REPL

1. Use `Ctrl+Shift+P` → "Rubolt: Start REPL"
2. Interactive Rubolt session opens in terminal

### Code Completion

1. Type keywords, function names, or module names
2. Press `Ctrl+Space` to trigger completion
3. Use `.` after module names for member completion
4. Use `(` after function names for parameter hints

### Error Detection

- Errors appear with red squiggly underlines
- Warnings appear with yellow squiggly underlines
- Hover over errors for detailed messages
- Check the Problems panel (`Ctrl+Shift+M`) for all issues

## Language Server Protocol Implementation

### Supported LSP Features

- **textDocument/completion** - Code completion
- **textDocument/hover** - Hover information
- **textDocument/signatureHelp** - Function signatures
- **textDocument/diagnostic** - Error and warning detection
- **textDocument/formatting** - Code formatting

### Message Format

The language server communicates using JSON-RPC over stdio:

```json
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "textDocument/completion",
    "params": {
        "textDocument": {"uri": "file:///path/to/file.rbo"},
        "position": {"line": 10, "character": 5}
    }
}
```

### Completion Item Kinds

- `1` - Text
- `3` - Function
- `9` - Module
- `14` - Keyword
- `15` - Snippet

## Extending the Language Server

### Adding New Completions

Edit `tools/rubolt_lsp.c` and modify the `get_completions` function:

```c
CompletionItem* get_completions(const char* text, Position pos, int* completion_count) {
    // Add your custom completions here
    completions[*completion_count] = (CompletionItem){
        .label = strdup("my_function"),
        .kind = 3, // Function
        .detail = strdup("Custom function"),
        .documentation = strdup("My custom function description")
    };
    (*completion_count)++;
}
```

### Adding New Diagnostics

Modify the `validate_document` function:

```c
Diagnostic* validate_document(const char* text, int* diagnostic_count) {
    // Add custom validation logic
    if (/* your condition */) {
        diagnostics[*diagnostic_count] = (Diagnostic){
            .range = {{line, start}, {line, end}},
            .message = strdup("Custom error message"),
            .severity = 1 // Error
        };
        (*diagnostic_count)++;
    }
}
```

## Troubleshooting

### Language Server Not Starting

1. **Check Dependencies**: Ensure `json-c` library is installed
2. **Check Path**: Verify `rubolt-lsp` is in PATH or configured correctly
3. **Check Logs**: Enable LSP logging in VS Code settings
4. **Rebuild**: Clean and rebuild the language server

### Completions Not Working

1. **Check File Extension**: Ensure file has `.rbo` extension
2. **Check Language Mode**: Verify VS Code recognizes file as Rubolt
3. **Restart Extension**: Reload VS Code window
4. **Check Settings**: Verify `rubolt.enableLSP` is true

### Syntax Highlighting Issues

1. **Check Grammar**: Verify `syntaxes/rubolt.tmLanguage.json` is valid
2. **Reload Extension**: Use "Developer: Reload Window"
3. **Check File Association**: Ensure `.rbo` files are associated with Rubolt

### Performance Issues

1. **Limit Diagnostics**: Reduce `maxNumberOfProblems` setting
2. **Disable Features**: Turn off unused LSP features
3. **Update Code**: Use latest language server version

## Development

### Building from Source

```bash
# Clone repository
git clone https://github.com/piratebomber/Rubolt.git
cd Rubolt

# Build everything
./build_stdlib.sh  # Unix/Linux/macOS
# or
build_stdlib.bat   # Windows
```

### Testing

```bash
# Test language server
cd tools
./rubolt-lsp < test_input.json

# Test VS Code extension
cd vscode-rubolt
npm test
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## Dependencies

### Runtime Dependencies
- **json-c** - JSON parsing library
- **libcurl** - HTTP client (for HTTP module)

### Build Dependencies
- **GCC** or compatible C compiler
- **Node.js** and **npm** (for VS Code extension)
- **TypeScript** compiler

### VS Code Extension Dependencies
- **vscode-languageclient** - LSP client library
- **vscode-languageserver** - LSP server library