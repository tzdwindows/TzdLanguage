# VS Code Extension & DAP Debugger

TzdLang includes an integrated development environment (IDE) extension for Visual Studio Code, located under `vscodePlugin/tzdlang`.

---

## 1. Features & Capabilities

- 🎨 **Syntax Highlighting**: Complete TextMate grammar highlighting keywords, types, control flow, strings, classes, and built-ins.
- ⚡ **Language Server Protocol (LSP)**: Real-time syntax validation, diagnostics, and symbol completion.
- 🐞 **Debug Adapter Protocol (DAP)**: Native interactive debugger engine directly communicating with `TzdTools.exe`.
  - Set, toggle, and manage breakpoints by line number.
  - Step Into (`F11`), Step Over (`F10`), Step Out (`Shift+F11`), and Continue (`F5`).
  - Real-time call stack unwinding and inspection.
  - Local variable scope view and watch expressions.
  - Interactive expression evaluation in Debug Console.

---

## 2. Installation & Configuration

### 2.1 Installing the Extension (.vsix)
The pre-packaged extension is available at:
`vscodePlugin/tzdlang/tzdlang-0.0.1.vsix`

Install via command line:
```cmd
code --install-extension vscodePlugin/tzdlang/tzdlang-0.0.1.vsix
```

Or within VS Code:
1. Open the Extensions sidebar (`Ctrl+Shift+X`).
2. Click the `...` menu in the upper-right corner.
3. Select **Install from VSIX...** and choose `tzdlang-0.0.1.vsix`.

### 2.2 Configuring Interpreter Path
In your VS Code `settings.json`:
```json
{
    "tzdlang.toolsPath": "C:/path/to/TzdTools/x64/Release/TzdTools.exe"
}
```

---

## 3. Debug Configuration (`launch.json`)

To debug a TzdLang script, add the following configuration to `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug TzdLang Script",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "stopOnEntry": false,
            "console": "integratedTerminal"
        },
        {
            "name": "Debug with JIT Enabled",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "args": ["--jit"],
            "stopOnEntry": false,
            "console": "integratedTerminal"
        }
    ]
}
```
