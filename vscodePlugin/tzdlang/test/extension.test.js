const assert = require('assert');
const vscode = require('vscode');

suite('Extension Test Suite', () => {
    vscode.window.showInformationMessage('Start all tests.');

    test('Extension should be present', () => {
        assert.ok(vscode.extensions.getExtension('tzdlang'));
    });

    test('Should activate', async () => {
        const ext = vscode.extensions.getExtension('tzdlang');
        await ext.activate();
        assert.strictEqual(ext.isActive, true);
    });

    test('Should register all commands', async () => {
        const commands = await vscode.commands.getCommands(true);
        assert.ok(commands.includes('tzdlang.helloWorld'));
        assert.ok(commands.includes('tzdlang.runCurrentFile'));
        assert.ok(commands.includes('tzdlang.resetToolsPath'));
    });

    test('Hello World command should show message', async () => {
        const result = await vscode.commands.executeCommand('tzdlang.helloWorld');
        assert.ok(result !== undefined);
    });
});