# TzdLang 语言支持插件

为 TzdLang 脚本提供专属的 VS Code 开发支持。

## 功能特性

- **语法高亮**：完整的关键字、类型、字符串、注释、数字和运算符高亮。
- **全功能 DAP 交互式断点调试**：
  - **行断点设置**：在编辑器行号左侧槽位（Gutter）单击即可打断点/取消断点，自动同步至解释器内核。
  - **断点命中暂停**：准确暂停在断点所在行，高亮停靠行。
  - **单步步进控制**：支持单步跳过 (Step Over `F10`)、单步进入 (Step In `F11`)、单步跳出 (Step Out `Shift+F11`)、继续运行 (`F5`)。
  - **调用栈查看 (Call Stack)**：VS Code 调试面板实时展示函数调用栈帧。
  - **变量监控 (Variables & Scopes)**：自动提取与展示当前作用域的局部变量（Locals）与全局变量（Globals）。
  - **调试控制台与监视表达式 (Debug Console & Watch)**：支持在调试控制台中即时求值表达式或查看变量内容。
- **全场景启动与附加格式**：
  - **标准调试 (`launch`)**：一键启动并附带 `--jit` 高性能即时编译。
  - **CPU 极限 NTT 大数计算调试**：配置预设支持 `--experimental-compute --bigTime`。
  - **GPU 加速 NTT 大数计算调试**：配置预设支持 `--forceGPU --bigTime`。
  - **进程附加 (`attach`)**：连接已启动的 TzdTools 调试服务器端口 (默认 54321)。
- **一键运行与快捷键**：
  - 按 `F5` 或点击右上角调试按钮：调试当前脚本。
  - 按 `Ctrl+F5`：免调试直接运行当前脚本。
- **智能路径查找**：自动在 `../x64/Release/`、workspace 邻近目录、PATH 环境变量中搜索 `TzdTools.exe`。
- **标准库自动关联**：自动加载 TzdTools 同级的 `stdlib` 目录，确保 `import` 语句正确工作。

## 调试配置 (launch.json)

在项目根目录 `.vscode/launch.json` 中可自由配置多种调试格式：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "TzdLang: 调试当前脚本",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "args": ["--jit"],
            "stopOnEntry": false
        },
        {
            "name": "TzdLang: CPU极限NTT大数调试",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "args": ["--experimental-compute", "--bigTime"],
            "stopOnEntry": false
        },
        {
            "name": "TzdLang: GPU加速NTT大数调试",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "args": ["--forceGPU", "--bigTime"],
            "stopOnEntry": false
        },
        {
            "name": "TzdLang: 附加到运行中服务 (Attach)",
            "type": "tzdlang",
            "request": "attach",
            "debugHost": "127.0.0.1",
            "debugPort": 54321
        }
    ]
}
```

## 配置 TzdTools 解释器

1. 按 `Ctrl+Shift+P` 打开命令面板。
2. 执行 `TzdLang: 重新设置 TzdTools 路径`。
3. 在弹出的文件选择对话框中定位到 `TzdTools.exe`。
4. 也可在 VS Code 设置中直接搜索 `tzdlang.toolsPath` 进行配置。

## 项目结构

```
tzdlang/
  extension.js                  # 扩展主入口与配置/调试适配器工厂
  debugAdapter.js               # DAP (Debug Adapter Protocol) 调试适配器核心实现
  package.json                  # 扩展清单、命令与调试器声明
  syntaxes/
    tzdlang.tmLanguage.json     # TextMate 语法高亮
    language-configuration.json # 语言配置（括号、注释等）
  test/
    dap_test.js                 # DAP 端到端自动化回归测试
```

## 构建和打包

```bash
npm install
npm test
npx @vscode/vsce package --no-dependencies
```