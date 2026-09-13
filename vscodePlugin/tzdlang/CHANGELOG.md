# Change Log

All notable changes to the "tzdlang" extension will be documented in this file.

## [0.2.0] - 2026-09-13

### Added
- **全功能 DAP 交互式断点调试器**：实现完整的 Debug Adapter Protocol。
- **可视化行断点设置**：支持编辑器行号槽位（Gutter）点击添加/移除断点，自动同步至解释器内核。
- **交互式单步控制**：单步跳过 (Step Over `F10`)、单步进入 (Step In `F11`)、单步跳出 (Step Out `Shift+F11`)、继续执行 (`F5`)、暂停与终止。
- **调用栈展示 (Call Stack)**：实时查看调用栈层级与文件行号。
- **变量作用域监控 (Variables & Scopes)**：在 VS Code 调试侧边栏实时监控 Locals 局部变量和 Globals 全局变量。
- **调试控制台与监视表达式 (Debug Console & Watch)**：支持在 VS Code 调试控制台求值表达式与监控变量值。
- **全格式 Launch 预设与配置**：
  - 默认 JIT 调试启动 (`launch`)
  - CPU 极限 NTT 高性能大数计算调试启动 (`--experimental-compute --bigTime`)
  - GPU 加速 NTT 大数计算调试启动 (`--forceGPU --bigTime`)
  - 附加到正在运行的调试服务器 (`attach`)
- **快捷键与动作**：
  - `F5` 快速启动当前脚本调试
  - `Ctrl+F5` 普通免调试运行
  - 编辑器右上角一键调试与运行按钮
- **自动化回归测试**：添加完整的 DAP 端到端测试套件 (`test/dap_test.js`)。

## [0.1.0] - 2026-06-05

### Added
- 完整的 LSP (Language Server Protocol) 支持，基于 ANTLR4 语法分析器
- 实时语法诊断：语法错误即时红色波浪线提醒
- 代码补全：关键字、类型、语句片段（Snippet）
- 悬停信息：关键字文档说明
- 文档符号：函数、类、变量、方法、字段、构造函数的符号列表
- 跳转定义：在函数/类名上点击跳转到定义
- 语义令牌（Semantic Tokens）：精确的语法高亮
- import 路径补全：支持 stdlib 路径提示

### Changed
- 重构为 LSP 架构，分离服务端和客户端
- ANTLR4 JavaScript 文件从 .js 重命名为 .mjs（ES Module）

### Fixed
- 修复了 class extends 继承语法的解析支持
- 修复了 ANTLR4 多个同名字规则引用时的索引访问

## [0.0.1] - 2026-06-05

### Added
- TzdLang 语言语法高亮
- 语言配置（括号匹配、自动闭合、注释切换）
- 运行当前脚本命令（F5 快捷键）
- TzdTools 路径设置命令
- 自动检测 TzdTools.exe
- 自动加载 stdlib 标准库
- 输出通道展示脚本运行结果
- 状态栏指示