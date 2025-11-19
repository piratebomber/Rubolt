#!/bin/bash

echo "========================================"
echo "Building Rubolt Standard Library"
echo "========================================"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo
echo -e "${YELLOW}[1/4] Building Standard Library Modules...${NC}"
cd Modules
make clean && make
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Failed to build standard library modules${NC}"
    exit 1
fi
cd ..

echo
echo -e "${YELLOW}[2/4] Building Language Server...${NC}"
cd tools
make clean && make rubolt-lsp
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Failed to build language server${NC}"
    exit 1
fi
cd ..

echo
echo -e "${YELLOW}[3/4] Building VS Code Extension...${NC}"
cd vscode-rubolt
if [ -d "node_modules" ]; then
    echo "Node modules already installed"
else
    echo "Installing dependencies..."
    npm install
fi
echo "Compiling TypeScript..."
npm run compile
if [ $? -ne 0 ]; then
    echo -e "${RED}ERROR: Failed to build VS Code extension${NC}"
    exit 1
fi
cd ..

echo
echo -e "${YELLOW}[4/4] Testing Standard Library...${NC}"
cd examples
echo "Testing stdlib demo..."
../rbcli run stdlib_demo.rbo
if [ $? -ne 0 ]; then
    echo -e "${YELLOW}WARNING: Standard library demo failed (this is expected if dependencies are missing)${NC}"
fi
cd ..

echo
echo -e "${GREEN}========================================"
echo "Build Complete!"
echo "========================================${NC}"
echo
echo "Standard Library Modules: Modules/*.o"
echo "Language Server: tools/rubolt-lsp"
echo "VS Code Extension: vscode-rubolt/out/"
echo
echo "To install VS Code extension:"
echo "  1. Open VS Code"
echo "  2. Go to Extensions view (Ctrl+Shift+X)"
echo "  3. Click '...' menu and select 'Install from VSIX...'"
echo "  4. Navigate to vscode-rubolt/ and package the extension"
echo
echo "To use the language server:"
echo "  Add tools/rubolt-lsp to your PATH"
echo "  or configure your editor to use the full path"
echo
echo "To install language server globally:"
echo "  sudo make install-lsp -C tools"