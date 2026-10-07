# VS Code 插件与 DAP 调试器指南

<p align="right">
  <a href="VSCode-Extension-and-Debugger.md"><strong>English</strong></a> | <a href="VSCode-Extension-and-Debugger-zh.md"><strong>中文</strong></a>
</p>

TzdLang 配套提供了专属的 Visual Studio Code 官方集成开发插件，源码位于仓库 `vscodePlugin/tzdlang`。

---

## 1. 插件特性与能力支持

- 🎨 **语法高亮着色**：基于 TextMate 语法规则，全面支持关键字、原生类型、控制流、字符串插值、类定义与内置函数高亮。
- ⚡ **语言服务器（LSP）**：提供实时语法校验、错误诊断与智能符号补全。
- 🚀 **完整 JIT 调试原生集成 (v0.2.3+)**：
  - 支持直接在 JIT 编译模式下设置行级断点，通过**选择性回退（Selective Deoptimization）**技术兼顾原生机器码极限执行吞吐与断点调试体验。
  - 在 VS Code 调试变量面板（Variables Pane）中独创提供 **`JIT 引擎 (JIT Engine)` 作用域**，直观呈现当前优化等级（`-O0` ~ `-O3`）、AST 内联指标、循环展开状态与已编译函数虚拟内存地址。
  - 动态 LLVM IR 导出与查看命令（`TzdLang: 查看指定函数的 JIT LLVM IR`）。
- 🐞 **调试适配器协议（DAP）引擎**：原生调试内核直接与 `TzdTools.exe` 双向交互：
  - 行级断点设置、启用、禁用与条件触发。
  - 单步进入（`F11`）、单步跳过（`F10`）、单步跳出（`Shift+F11`）与继续执行（`F5`）。
  - 实时调用栈回溯与帧切换（Call Stack Inspection），清晰区分 `(JIT Compiled)` 与 `(Interpreted)` 帧。
  - 局部变量（Locals）、全局变量（Globals）与 JIT 引擎状态多级作用域实时监视与 Watch 表达式。
  - 调试控制台（Debug Console）动态表达式求值与 `:jit` 交互式诊断指令。

---

## 2. 安装与环境配置

### 2.1 安装扩展包（.vsix）
已预编译打包的最新 VSIX 插件文件位于：
`vscodePlugin/tzdlang/tzdlang-0.2.3.vsix`

可通过命令行快速安装：
```cmd
code --install-extension vscodePlugin/tzdlang/tzdlang-0.2.3.vsix
```

或在 VS Code 界面中安装：
1. 打开扩展面板（快捷键 `Ctrl+Shift+X`）。
2. 点击右上角的 `...` 操作菜单。
3. 选择 **从 VSIX 安装... (Install from VSIX...)** 并选择 `tzdlang-0.2.3.vsix`。

### 2.2 配置选项 (`settings.json`)
在 VS Code 的 `settings.json` 中可对 TzdLang 工具链与 JIT 参数进行全局预设：
```json
{
    "tzdlang.toolsPath": "C:/path/to/TzdTools/x64/Release/TzdTools.exe",
    "tzdlang.enableJit": true,
    "tzdlang.optLevel": 3,
    "tzdlang.enableAstInlining": true
}
```

---

## 3. 调试运行配置 (`launch.json`)

在项目 `.vscode/launch.json` 中配置以下开箱即用的调试模式：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "TzdLang: JIT 极限性能调试 (-O3 极限内联与循环展开)",
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
            "name": "TzdLang: JIT 标准调试 (-O2)",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "jit": true,
            "optLevel": 2,
            "stopOnEntry": false,
            "console": "integratedTerminal"
        },
        {
            "name": "TzdLang: 解释器逐行步进调试 (-O0 / 无JIT)",
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

### 3.1 核心 JIT 调试参数说明

| 参数项 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `jit` | boolean | `true` | 是否启用 JIT 编译器进行调试 |
| `optLevel` | number | `3` | JIT 优化等级（`0` 无优化，`1` 快速，`2` 标准，`3` 极限优化） |
| `inlineThreshold` | number | `500` | LLVM 过程间内联成本预算阈值 |
| `enableAstInlining` | boolean | `true` | 是否开启前端 AST 树级函数内联 |
| `enableMathIntrinsics`| boolean | `true` | 是否将 `sqrt`/`sin` 等数学函数特化为 FPU 机器码 |
| `enableLoopUnroll` | boolean | `true` | 是否执行 LLVM 循环展开优化通道 |

---

## 4. 专有命令与 JIT 交互

在 VS Code 中按下 `Ctrl+Shift+P`（命令面板），可快速使用以下专属命令：

- `TzdLang: 查看当前 JIT 引擎运行状态 (Show JIT Status)`
- `TzdLang: 查看已编译 JIT 函数符号列表 (List JIT Functions)`
- `TzdLang: 查看指定函数的 JIT LLVM IR (Dump Function LLVM IR)`：弹出输入框输入函数名，插件将自动获取底层 LLVM 汇编 IR 并在新编辑器窗口中进行高亮展示。

