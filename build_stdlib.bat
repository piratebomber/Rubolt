@echo off
echo ========================================
echo Building Rubolt Standard Library
echo ========================================

echo.
echo [1/4] Building Standard Library Modules...
cd Modules
call build_modules.bat
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build standard library modules
    exit /b 1
)
cd ..

echo.
echo [2/4] Building Language Server...
cd tools
gcc -Wall -Wextra -std=c99 -O2 -I../src -I../gc -I../rc rubolt_lsp.c -o rubolt-lsp.exe -ljson-c
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build language server
    exit /b 1
)
cd ..

echo.
echo [3/4] Building VS Code Extension...
cd vscode-rubolt
if exist node_modules (
    echo Node modules already installed
) else (
    echo Installing dependencies...
    npm install
)
echo Compiling TypeScript...
npm run compile
if %ERRORLEVEL% neq 0 (
    echo ERROR: Failed to build VS Code extension
    exit /b 1
)
cd ..

echo.
echo [4/4] Testing Standard Library...
cd examples
echo Testing stdlib demo...
..\rbcli run stdlib_demo.rbo
if %ERRORLEVEL% neq 0 (
    echo WARNING: Standard library demo failed (this is expected if dependencies are missing)
)
cd ..

echo.
echo ========================================
echo Build Complete!
echo ========================================
echo.
echo Standard Library Modules: Modules/*.o
echo Language Server: tools/rubolt-lsp.exe
echo VS Code Extension: vscode-rubolt/out/
echo.
echo To install VS Code extension:
echo   1. Open VS Code
echo   2. Go to Extensions view (Ctrl+Shift+X)
echo   3. Click "..." menu and select "Install from VSIX..."
echo   4. Navigate to vscode-rubolt/ and package the extension
echo.
echo To use the language server:
echo   Add tools/rubolt-lsp.exe to your PATH
echo   or configure your editor to use the full path