# VS Code Extension & DAP Debugger

<p align="right">
  <a href="VSCode-Extension-and-Debugger.md"><strong>English</strong></a> | <a href="VSCode-Extension-and-Debugger-zh.md"><strong>中文</strong></a>
</p>

TzdLang includes an integrated development environment (IDE) extension for Visual Studio Code, located under `vscodePlugin/tzdlang`.

---

## 1. Features & Capabilities

- 🎨 **Syntax Highlighting**: Complete TextMate grammar highlighting keywords, types, control flow, strings, classes, and built-ins.
- ⚡ **Language Server Protocol (LSP)**: Real-time syntax validation, diagnostics, and symbol completion.
- 🚀 **Full Native JIT Debugging (v0.2.3+)**:
  - Direct breakpoint support under JIT compilation with **Selective Deoptimization**, preserving full native speed while ensuring breakpoints pause reliably.
  - Dedicated **`JIT Engine` Scope** inside the VS Code Variables panel showing active optimization level (`-O0` ~ `-O3`), inlining metrics, loop unrolling status, and compiled function virtual memory addresses.
  - Instant LLVM IR dumping command (`TzdLang: Dump Function LLVM IR`).
- 🐞 **Debug Adapter Protocol (DAP)**: Native interactive debugger engine directly communicating with `TzdTools.exe`.
  - Set, toggle, and manage breakpoints by line number.
  - Step Into (`F11`), Step Over (`F10`), Step Out (`Shift+F11`), and Continue (`F5`).
  - Real-time call stack unwinding, distinguishing `(JIT Compiled)` vs `(Interpreted)` stack frames.
  - Multi-tier variable inspection: Locals, Globals, and JIT Engine scopes.
  - Interactive expression evaluation in Debug Console and interactive `:jit` diagnostic commands.

---

## 2. Installation & Configuration

### 2.1 Installing the Extension (.vsix)
The pre-packaged extension is available at:
`vscodePlugin/tzdlang/tzdlang-0.2.3.vsix`

Install via command line:
```cmd
code --install-extension vscodePlugin/tzdlang/tzdlang-0.2.3.vsix
```

Or within VS Code:
1. Open the Extensions sidebar (`Ctrl+Shift+X`).
2. Click the `...` menu in the upper-right corner.
3. Select **Install from VSIX...** and choose `tzdlang-0.2.3.vsix`.

### 2.2 Global Settings (`settings.json`)
In your VS Code `settings.json`:
```json
{
    "tzdlang.toolsPath": "C:/path/to/TzdTools/x64/Release/TzdTools.exe",
    "tzdlang.enableJit": true,
    "tzdlang.optLevel": 3,
    "tzdlang.enableAstInlining": true
}
```

---

## 3. Debug Configuration (`launch.json`)

To debug a TzdLang script, use any of the pre-configured templates in `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "TzdLang: JIT Extreme Performance Debug (-O3 Inlining & Unroll)",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "jit": true,
            "optLevel": 3,
            "enableAstInlining": true,
            "enableMathIntrinsics": true,
            "enableLoopUnroll": true,
            "stopOnEntry": false,
            "console": "integratedTerminal"
        },
        {
            "name": "TzdLang: JIT Standard Debug (-O2)",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "jit": true,
            "optLevel": 2,
            "stopOnEntry": false,
            "console": "integratedTerminal"
        },
        {
            "name": "TzdLang: Interpreter Line-by-Line Debug (-O0 / No JIT)",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "jit": false,
            "stopOnEntry": true,
            "console": "integratedTerminal"
        }
    ]
}
```

### 3.1 JIT Configuration Properties

| Property | Type | Default | Description |
|---|---|---|---|
| `jit` | boolean | `true` | Enable/disable LLVM JIT compilation during debugging |
| `optLevel` | number | `3` | Optimization level (`0` = none, `1` = fast, `2` = standard, `3` = extreme) |
| `inlineThreshold` | number | `500` | LLVM interprocedural inlining cost budget |
| `enableAstInlining` | boolean | `true` | Enable frontend AST-level function inlining |
| `enableMathIntrinsics` | boolean | `true` | Inline math functions (`sqrt`, `sin`) into native FPU instructions |
| `enableLoopUnroll` | boolean | `true` | Enable LLVM loop unrolling pass |

---

## 4. Commands & Diagnostics

Press `Ctrl+Shift+P` (Command Palette) in VS Code to run:
- `TzdLang: Show JIT Status` (`:jit status`)
- `TzdLang: List JIT Functions` (`:jit list`)
- `TzdLang: Dump Function LLVM IR` (`:jit ir <name>`)

