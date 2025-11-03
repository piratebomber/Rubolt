# Rubolt Language Support for VSCode

Provides syntax highlighting and language support for Rubolt (`.rbo`) files in Visual Studio Code.

## Features

- **Syntax Highlighting**: Full syntax highlighting for Rubolt's hybrid Python/C/TypeScript syntax
- **Auto-closing Pairs**: Automatic closing of brackets, parentheses, and quotes
- **Code Folding**: Support for code folding in functions and blocks
- **Comment Toggling**: Quick comment/uncomment with `Ctrl+/` (Cmd+/ on Mac)
- **Bracket Matching**: Highlight matching brackets and parentheses

## Installation

### From Source

1. Copy the `vscode-rubolt` folder to your VSCode extensions directory:
   - **Windows**: `%USERPROFILE%\.vscode\extensions\`
   - **macOS/Linux**: `~/.vscode/extensions/`

2. Restart VSCode

3. Open any `.rbo` file to see syntax highlighting

### From VSIX (if packaged)

```bash
vsce package
code --install-extension rubolt-1.0.0.vsix
```

## Syntax Examples

Rubolt combines syntax from Python, C, and TypeScript:

```rubolt
// TypeScript-style variable declarations
let name: string = "Rubolt";
const version: number = 1.0;

// Python-style functions with type hints
def greet(name: string) -> string:
    return f"Hello, {name}!"

// C-style control flow
if (version >= 1.0) {
    printf("Production ready!\n");
}

for (let i = 0; i < 10; i++) {
    print(i);
}
```

## Language Features

- Keywords: `let`, `const`, `var`, `def`, `function`, `if`, `else`, `for`, `while`, `return`
- Types: `string`, `number`, `bool`, `void`, `any`
- Operators: `+`, `-`, `*`, `/`, `%`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`
- Comments: `//` (line), `#` (Python-style), `/* */` (block)

## Contributing

Contributions are welcome! Please submit issues and pull requests on GitHub.

## License

MIT License
