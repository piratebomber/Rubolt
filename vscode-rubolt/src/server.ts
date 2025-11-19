import {
    createConnection,
    TextDocuments,
    Diagnostic,
    DiagnosticSeverity,
    ProposedFeatures,
    InitializeParams,
    DidChangeConfigurationNotification,
    CompletionItem,
    CompletionItemKind,
    TextDocumentPositionParams,
    TextDocumentSyncKind,
    InitializeResult,
    DocumentDiagnosticReportKind,
    type DocumentDiagnosticReport,
    Hover,
    MarkupKind,
    SignatureHelp,
    SignatureInformation,
    ParameterInformation
} from 'vscode-languageserver/node';

import { TextDocument } from 'vscode-languageserver-textdocument';

const connection = createConnection(ProposedFeatures.all);
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

let hasConfigurationCapability = false;
let hasWorkspaceFolderCapability = false;
let hasDiagnosticRelatedInformationCapability = false;

connection.onInitialize((params: InitializeParams) => {
    const capabilities = params.capabilities;

    hasConfigurationCapability = !!(
        capabilities.workspace && !!capabilities.workspace.configuration
    );
    hasWorkspaceFolderCapability = !!(
        capabilities.workspace && !!capabilities.workspace.workspaceFolders
    );
    hasDiagnosticRelatedInformationCapability = !!(
        capabilities.textDocument &&
        capabilities.textDocument.publishDiagnostics &&
        capabilities.textDocument.publishDiagnostics.relatedInformation
    );

    const result: InitializeResult = {
        capabilities: {
            textDocumentSync: TextDocumentSyncKind.Incremental,
            completionProvider: {
                resolveProvider: true,
                triggerCharacters: ['.', '(']
            },
            hoverProvider: true,
            signatureHelpProvider: {
                triggerCharacters: ['(', ',']
            },
            diagnosticProvider: {
                interFileDependencies: false,
                workspaceDiagnostics: false
            }
        }
    };
    
    if (hasWorkspaceFolderCapability) {
        result.capabilities.workspace = {
            workspaceFolders: {
                supported: true
            }
        };
    }
    
    return result;
});

connection.onInitialized(() => {
    if (hasConfigurationCapability) {
        connection.client.register(DidChangeConfigurationNotification.type, undefined);
    }
    if (hasWorkspaceFolderCapability) {
        connection.workspace.onDidChangeWorkspaceFolders(_event => {
            connection.console.log('Workspace folder change event received.');
        });
    }
});

interface RuboltSettings {
    maxNumberOfProblems: number;
}

const defaultSettings: RuboltSettings = { maxNumberOfProblems: 1000 };
let globalSettings: RuboltSettings = defaultSettings;

const documentSettings: Map<string, Thenable<RuboltSettings>> = new Map();

connection.onDidChangeConfiguration(change => {
    if (hasConfigurationCapability) {
        documentSettings.clear();
    } else {
        globalSettings = <RuboltSettings>(
            (change.settings.rubolt || defaultSettings)
        );
    }
    connection.languages.diagnostics.refresh();
});

function getDocumentSettings(resource: string): Thenable<RuboltSettings> {
    if (!hasConfigurationCapability) {
        return Promise.resolve(globalSettings);
    }
    let result = documentSettings.get(resource);
    if (!result) {
        result = connection.workspace.getConfiguration({
            scopeUri: resource,
            section: 'rubolt'
        });
        documentSettings.set(resource, result);
    }
    return result;
}

documents.onDidClose(e => {
    documentSettings.delete(e.document.uri);
});

connection.languages.diagnostics.on(async (params) => {
    const document = documents.get(params.textDocument.uri);
    if (document !== undefined) {
        return {
            kind: DocumentDiagnosticReportKind.Full,
            items: await validateTextDocument(document)
        } satisfies DocumentDiagnosticReport;
    } else {
        return {
            kind: DocumentDiagnosticReportKind.Full,
            items: []
        } satisfies DocumentDiagnosticReport;
    }
});

documents.onDidChangeContent(change => {
    validateTextDocument(change.document);
});

async function validateTextDocument(textDocument: TextDocument): Promise<Diagnostic[]> {
    const settings = await getDocumentSettings(textDocument.uri);
    const text = textDocument.getText();
    const diagnostics: Diagnostic[] = [];
    
    const lines = text.split(/\\r?\\n/g);
    let problems = 0;
    
    for (let i = 0; i < lines.length && problems < settings.maxNumberOfProblems; i++) {
        const line = lines[i];
        
        // Check for syntax errors
        if (line.includes('def ') && !line.includes('->')) {
            const diagnostic: Diagnostic = {
                severity: DiagnosticSeverity.Warning,
                range: {
                    start: { line: i, character: 0 },
                    end: { line: i, character: line.length }
                },
                message: 'Function definition should include return type annotation',
                source: 'rubolt'
            };
            diagnostics.push(diagnostic);
            problems++;
        }
        
        // Check for missing semicolons
        if (line.trim().length > 0 && 
            !line.trim().endsWith(';') && 
            !line.trim().endsWith('{') && 
            !line.trim().endsWith('}') &&
            !line.trim().startsWith('//') &&
            !line.trim().startsWith('def ') &&
            !line.trim().startsWith('class ') &&
            !line.trim().startsWith('type ')) {
            
            const diagnostic: Diagnostic = {
                severity: DiagnosticSeverity.Error,
                range: {
                    start: { line: i, character: line.length },
                    end: { line: i, character: line.length }
                },
                message: 'Missing semicolon',
                source: 'rubolt'
            };
            diagnostics.push(diagnostic);
            problems++;
        }
    }
    
    return diagnostics;
}

connection.onDidChangeWatchedFiles(_change => {
    connection.console.log('We received a file change event');
});

connection.onCompletion(
    (_textDocumentPosition: TextDocumentPositionParams): CompletionItem[] => {
        return [
            {
                label: 'def',
                kind: CompletionItemKind.Keyword,
                data: 1,
                insertText: 'def ${1:function_name}(${2:params}) -> ${3:return_type} {\\n    ${4:// body}\\n}'
            },
            {
                label: 'class',
                kind: CompletionItemKind.Keyword,
                data: 2,
                insertText: 'class ${1:ClassName} {\\n    ${2:// body}\\n}'
            },
            {
                label: 'if',
                kind: CompletionItemKind.Keyword,
                data: 3,
                insertText: 'if (${1:condition}) {\\n    ${2:// body}\\n}'
            },
            {
                label: 'for',
                kind: CompletionItemKind.Keyword,
                data: 4,
                insertText: 'for (${1:item} in ${2:iterable}) {\\n    ${3:// body}\\n}'
            },
            {
                label: 'while',
                kind: CompletionItemKind.Keyword,
                data: 5,
                insertText: 'while (${1:condition}) {\\n    ${2:// body}\\n}'
            },
            {
                label: 'import',
                kind: CompletionItemKind.Keyword,
                data: 6,
                insertText: 'import ${1:module};'
            },
            {
                label: 'print',
                kind: CompletionItemKind.Function,
                data: 7,
                insertText: 'print(${1:value});'
            },
            {
                label: 'let',
                kind: CompletionItemKind.Keyword,
                data: 8,
                insertText: 'let ${1:variable} = ${2:value};'
            }
        ];
    }
);

connection.onCompletionResolve(
    (item: CompletionItem): CompletionItem => {
        if (item.data === 1) {
            item.detail = 'Function definition';
            item.documentation = 'Define a new function with parameters and return type';
        } else if (item.data === 2) {
            item.detail = 'Class definition';
            item.documentation = 'Define a new class';
        }
        return item;
    }
);

connection.onHover((_params): Hover => {
    return {
        contents: {
            kind: MarkupKind.Markdown,
            value: [
                '**Rubolt Language**',
                '',
                'Hover over symbols for documentation'
            ].join('\\n')
        }
    };
});

connection.onSignatureHelp((_params): SignatureHelp => {
    return {
        signatures: [
            {
                label: 'print(value: any) -> void',
                documentation: 'Print a value to the console',
                parameters: [
                    {
                        label: 'value',
                        documentation: 'The value to print'
                    }
                ]
            }
        ],
        activeSignature: 0,
        activeParameter: 0
    };
});

documents.listen(connection);
connection.listen();