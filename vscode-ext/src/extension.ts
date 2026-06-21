import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import { LanguageClient, LanguageClientOptions, ServerOptions, TransportKind } from 'vscode-languageclient/node';
import { MemoryViewProvider } from './memoryWebview';

let client: LanguageClient;

const LAUNCH_TEMPLATE = {
    version: '0.2.0',
    configurations: [
        {
            name: 'Debug Compiler (current file)',
            type: 'cppdbg',
            request: 'launch',
            program: '${workspaceFolder}/build/meta-c',
            args: ['${file}', '-o', '${workspaceFolder}/build/${fileBasenameNoExtension}.c'],
            cwd: '${workspaceFolder}',
            MIMode: 'gdb',
            setupCommands: [
                { description: 'Enable pretty-printing for gdb', text: '-enable-pretty-printing', ignoreFailures: true }
            ],
            preLaunchTask: 'Build Meta-C',
        },
        {
            name: 'Debug Compiled Program',
            type: 'cppdbg',
            request: 'launch',
            program: '${workspaceFolder}/build/${fileBasenameNoExtension}',
            args: [],
            cwd: '${workspaceFolder}',
            MIMode: 'gdb',
            setupCommands: [
                { description: 'Enable pretty-printing', text: '-enable-pretty-printing', ignoreFailures: true },
                { description: 'Load Meta-C printers', text: 'source ${workspaceFolder}/debugger/.gdbinit', ignoreFailures: true },
            ],
            preLaunchTask: 'Compile this .mc file',
        },
        {
            name: 'Run Compiled Program',
            type: 'cppdbg',
            request: 'launch',
            program: '${workspaceFolder}/build/${fileBasenameNoExtension}',
            args: [],
            cwd: '${workspaceFolder}',
            MIMode: 'gdb',
            preLaunchTask: 'Compile this .mc file (release)',
        },
    ],
};

const TASKS_TEMPLATE = {
    version: '2.0.0',
    tasks: [
        {
            label: 'Build Meta-C',
            type: 'shell',
            command: 'scons',
            group: { kind: 'build', isDefault: true },
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Build (debug)',
            type: 'shell',
            command: 'scons profile=debug',
            group: 'build',
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Compile this .mc file',
            type: 'shell',
            command: './build/meta-c "${file}" -o "build/${fileBasenameNoExtension}.c" && gcc -g -Iruntime "build/${fileBasenameNoExtension}.c" runtime/block_memory.c runtime/io.c runtime/hot_reload.c -o "build/${fileBasenameNoExtension}" -ldl',
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Compile this .mc file (release)',
            type: 'shell',
            command: './build/meta-c "${file}" -o "build/${fileBasenameNoExtension}.c" && gcc -O3 -Iruntime "build/${fileBasenameNoExtension}.c" runtime/block_memory.c runtime/io.c -o "build/${fileBasenameNoExtension}"',
            problemMatcher: ['$gcc'],
        },
        {
            label: 'Run this .mc file',
            type: 'shell',
            command: './build/meta-c "${file}" -o "${fileBasenameNoExtension}.c" && gcc -O3 "build/${fileBasenameNoExtension}.c" runtime/block_memory.c runtime/io.c -o "build/${fileBasenameNoExtension}" && "build/${fileBasenameNoExtension}"',
            problemMatcher: [],
        },
    ],
};

export function activate(context: vscode.ExtensionContext) {
    startLanguageClient(context);

    const provider = new MemoryViewProvider(context.extensionUri);

    context.subscriptions.push(
        vscode.window.registerWebviewViewProvider(
            MemoryViewProvider.viewType,
            provider,
            { webviewOptions: { retainContextWhenHidden: true } }
        )
    );

    context.subscriptions.push(
        vscode.debug.onDidChangeActiveDebugSession(session => {
            console.log('[Meta-C] onDidChangeActiveDebugSession:', session ? session.id : 'null');
            provider.update();
        })
    );

    // Real-time updates on pause / step / breakpoint
    context.subscriptions.push(
        vscode.debug.onDidChangeActiveStackItem(() => {
            console.log('[Meta-C] onDidChangeActiveStackItem');
            if (vscode.debug.activeDebugSession) {
                provider.update();
            }
        })
    );

    // Auto-update when debug session starts
    context.subscriptions.push(
        vscode.debug.onDidStartDebugSession(() => {
            console.log('[Meta-C] onDidStartDebugSession');
            provider.update();
        })
    );

    // Also update when debug session terminates
    context.subscriptions.push(
        vscode.debug.onDidTerminateDebugSession(() => {
            console.log('[Meta-C] onDidTerminateDebugSession');
            provider.update();
        })
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('meta-c.initWorkspace', initWorkspace)
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('meta-c.debugProgram', () => {
            const editor = vscode.window.activeTextEditor;
            if (editor && editor.document.languageId === 'meta-c') {
                const filePath = editor.document.uri.fsPath;
                const fileName = path.basename(filePath, '.mc');
                vscode.debug.startDebugging(undefined, {
                    type: 'cppdbg',
                    name: 'Debug Meta-C Program',
                    request: 'launch',
                    program: `${vscode.workspace.workspaceFolders?.[0]?.uri.fsPath}/build/${fileName}`,
                    args: [],
                    cwd: '${workspaceFolder}',
                    MIMode: 'gdb',
                    setupCommands: [
                        { description: 'Enable pretty-printing', text: '-enable-pretty-printing', ignoreFailures: true },
                        { description: 'Load Meta-C printers', text: 'source ${workspaceFolder}/debugger/.gdbinit', ignoreFailures: true },
                    ],
                    preLaunchTask: 'Compile Meta-C (debug)',
                });
            } else {
                vscode.window.showErrorMessage('Open a .mc file first');
            }
        })
    );

    // Suggest workspace setup when opening .mc files without config
    context.subscriptions.push(
        vscode.workspace.onDidOpenTextDocument(doc => {
            if (doc.languageId === 'meta-c' && vscode.workspace.workspaceFolders) {
                const wsPath = vscode.workspace.workspaceFolders[0].uri.fsPath;
                const vscodeDir = path.join(wsPath, '.vscode');
                const launchPath = path.join(vscodeDir, 'launch.json');
                if (!fs.existsSync(launchPath)) {
                    setTimeout(() => {
                        vscode.window.showInformationMessage(
                            'Meta-C workspace not configured. Add build & debug configs?',
                            'Set up workspace'
                        ).then(selection => {
                            if (selection === 'Set up workspace') {
                                vscode.commands.executeCommand('meta-c.initWorkspace');
                            }
                        });
                    }, 1000);
                }
            }
        })
    );
}

function initWorkspace() {
    const wsFolders = vscode.workspace.workspaceFolders;
    if (!wsFolders) {
        vscode.window.showErrorMessage('Open a folder first');
        return;
    }

    const wsPath = wsFolders[0].uri.fsPath;
    const vscodeDir = path.join(wsPath, '.vscode');

    if (!fs.existsSync(vscodeDir)) {
        fs.mkdirSync(vscodeDir, { recursive: true });
    }

    const launchPath = path.join(vscodeDir, 'launch.json');
    const tasksPath = path.join(vscodeDir, 'tasks.json');

    if (!fs.existsSync(launchPath)) {
        fs.writeFileSync(launchPath, JSON.stringify(LAUNCH_TEMPLATE, null, 4));
    }

    if (!fs.existsSync(tasksPath)) {
        fs.writeFileSync(tasksPath, JSON.stringify(TASKS_TEMPLATE, null, 4));
    }

    // Reload window to pick up the new files
    vscode.window.showInformationMessage(
        'Meta-C workspace created! Reload window to activate.',
        'Reload Now'
    ).then(selection => {
        if (selection === 'Reload Now') {
            vscode.commands.executeCommand('workbench.action.reloadWindow');
        }
    });
}

function startLanguageClient(context: vscode.ExtensionContext) {
    const serverModule = context.asAbsolutePath('server/out/server.js');

    const serverOptions: ServerOptions = {
        run: { module: serverModule, transport: TransportKind.ipc },
        debug: {
            module: serverModule,
            transport: TransportKind.ipc,
            options: { execArgv: ['--nolazy', '--inspect=6009'] },
        },
    };

    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'meta-c' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.mc'),
        },
    };

    client = new LanguageClient(
        'meta-c-language-server',
        'Meta-C Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
}

export function deactivate(): Thenable<void> | undefined {
    return client?.stop();
}
