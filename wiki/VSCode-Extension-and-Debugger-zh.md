# VS Code 插件与 DAP 调试器指南

<p align="right">
  <a href="VSCode-Extension-and-Debugger.md"><strong>English</strong></a> | <a href="VSCode-Extension-and-Debugger-zh.md"><strong>中文</strong></a>
</p>

TzdLang 配套提供了专属的 Visual Studio Code 官方集成开发插件，源码位于仓库 `vscodePlugin/tzdlang`。

---

## 1. 插件特性与能力支持

- 🎨 **语法高亮着色**：基于 TextMate 语法规则，全面支持关键字、原生类型、控制流、字符串插值、类定义与内置函数高亮。
- ⚡ **语言服务器（LSP）**：提供实时语法校验、错误诊断与智能符号补全。
- 🐞 **调试适配器协议（DAP）引擎**：原生调试内核直接与 `TzdTools.exe` 双向交互：
  - 行级断点设置、启用、禁用与条件触发。
  - 单步进入（`F11`）、单步跳过（`F10`）、单步跳出（`Shift+F11`）与继续执行（`F5`）。
  - 实时调用栈回溯与帧切换（Call Stack Inspection）。
  - 局部变量作用域实时监视与 Watch 表达式。
  - 调试控制台（Debug Console）动态表达式求值。

---

## 2. 安装与环境配置

### 2.1 安装扩展包（.vsix）
已预编译打包的 VSIX 插件文件位于：
`vscodePlugin/tzdlang/tzdlang-0.0.1.vsix`

可通过命令行快速安装：
```cmd
code --install-extension vscodePlugin/tzdlang/tzdlang-0.0.1.vsix
```

或在 VS Code 界面中安装：
1. 打开扩展面板（快捷键 `Ctrl+Shift+X`）。
2. 点击右上角的 `...` 操作菜单。
3. 选择 **从 VSIX 安装... (Install from VSIX...)** 并选择 `tzdlang-0.0.1.vsix`。

### 2.2 配置 TzdTools 解释器可执行文件路径
在 VS Code 的 `settings.json` 中配置：
```json
{
    "tzdlang.toolsPath": "C:/path/to/TzdTools/x64/Release/TzdTools.exe"
}
```

---

## 3. 调试运行配置 (`launch.json`)

在项目 `.vscode/launch.json` 中添加如下调试配置，即可直接一键启动调试：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "调试 TzdLang 脚本",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "stopOnEntry": false,
            "console": "integratedTerminal"
        },
        {
            "name": "启用 JIT 调试运行",
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
