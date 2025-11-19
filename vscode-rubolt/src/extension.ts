import * as vscode from 'vscode';
import * as path from 'path';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
    // Register commands
    const runCommand = vscode.commands.registerCommand('rubolt.run', () => {
        const editor = vscode.window.activeTextEditor;
        if (!editor || editor.document.languageId !== 'rubolt') {
            vscode.window.showErrorMessage('No Rubolt file is currently open');
            return;
        }
        
        const filePath = editor.document.fileName;
        const terminal = vscode.window.createTerminal('Rubolt');
        terminal.sendText(`rbcli run "${filePath}"`);
        terminal.show();
    });

    const replCommand = vscode.commands.registerCommand('rubolt.repl', () => {
        const terminal = vscode.window.createTerminal('Rubolt REPL');
        terminal.sendText('rbcli repl');
        terminal.show();
    });

    context.subscriptions.push(runCommand, replCommand);

    // Start language server
    const config = vscode.workspace.getConfiguration('rubolt');
    if (config.get('enableLSP')) {
        startLanguageServer(context);
    }

    // Register document formatting provider
    const formatProvider = vscode.languages.registerDocumentFormattingEditProvider('rubolt', {
        provideDocumentFormattingEdits(document: vscode.TextDocument): vscode.TextEdit[] {
            return formatRuboltDocument(document);
        }
    });

    context.subscriptions.push(formatProvider);
}

function startLanguageServer(context: vscode.ExtensionContext) {
    const serverModule = context.asAbsolutePath(path.join('out', 'server.js'));
    
    const serverOptions: ServerOptions = {
        run: { module: serverModule, transport: TransportKind.ipc },
        debug: { module: serverModule, transport: TransportKind.ipc, options: { execArgv: ['--nolazy', '--inspect=6009'] } }
    };

    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'rubolt' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/.rbo')
        }
    };

    client = new LanguageClient('ruboltLanguageServer', 'Rubolt Language Server', serverOptions, clientOptions);
    client.start();
}

function formatRuboltDocument(document: vscode.TextDocument): vscode.TextEdit[] {
    const edits: vscode.TextEdit[] = [];
    const text = document.getText();
    const lines = text.split('\n');
    
    let indentLevel = 0;
    const indentSize = 4;
    
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const trimmed = line.trim();
        
        if (trimmed === '') continue;
        
        // Decrease indent for closing braces
        if (trimmed.startsWith('}')) {
            indentLevel = Math.max(0, indentLevel - 1);
        }
        
        const expectedIndent = ' '.repeat(indentLevel * indentSize);
        const currentIndent = line.match(/^\\s*/)?.[0] || '';
        
        if (currentIndent !== expectedIndent) {
            const range = new vscode.Range(i, 0, i, currentIndent.length);
            edits.push(vscode.TextEdit.replace(range, expectedIndent));
        }
        
        // Increase indent for opening braces
        if (trimmed.endsWith('{')) {
            indentLevel++;
        }
    }
    
    return edits;
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}