# Change Log

All notable changes to the "tzdlang" extension will be documented in this file.

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