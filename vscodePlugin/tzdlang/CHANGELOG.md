# Change Log

All notable changes to the "tzdlang" extension will be documented in this file.

## [0.2.12] - 2026-09-24

### Added
- **IntelliJ IDEA 级智能上下文感知代码补全**：
  - 上下文严格隔离：注释（`//`、`/* ... */`）和字符串内部不弹出补全。
  - 空格/无前缀防干扰：输入空格或空行不再无故弹出全量符号候选。
  - 上下文语法感知：`obj.` 仅展示成员，`new ` 仅展示类名，`var`/`let` 仅展示数据类型。
  - 多级优先级精准排序与前缀评分匹配（成员 > 局部变量 > 全局函数 > 类型 > 关键字）。

## [0.2.11] - 2026-09-24

### Fixed
- 修复块注释 `/* ... */` 内部英文单词被错误标记为“未声明的变量”的诊断假阳性问题。

### Added
- 支持方法与函数文档注释（JSDoc 风格）提取与富文本渲染展示。
- 全面扩展签名帮助（Signature Help），支持类成员方法及普通函数的实时参数高亮提示。
- 补全项直观显示方法返回类型预测（`➔ ReturnType`）。

## [0.2.10] - 2026-09-23

### Fixed
- **修复链式调用中对返回类型的智能推断与定义跳转**：
  - 修复 `new Test().a().b()` 中当 `a()` 返回基本类型（如 `int`、`string` 等）时错误跳转到其他类 `b()` 方法的问题。
  - 点号成员访问严格基于调用对象推断的类型上下文解析，若目标类型无该方法则精准返回空，绝不错误向全局兜底匹配其他类的同名方法。

### Added
- **方法返回值对象与类型自动预测及代码补全**：
  - 自动分析方法体内的 `return` 表达式：精准推断 `new ClassName()`、`this`、`super`、变量引用、基本类型字面量以及嵌套链式调用。
  - 在用户输入 `new Test().a().` 时，实时根据 `a()` 的返回对象类型精确补齐下一级成员。
  - 增强成员悬停（Hover）文档提示：支持链式调用的精确类型与方法签名呈现。

## [0.2.9] - 2026-09-23

### Added
- **递归语法检测与 Gutter 循环箭头显示 (类似 IntelliJ IDEA)**：
  - 自动检测普通全局函数与面向对象类方法的递归调用。
  - 支持**自递归**（Direct Recursion）与**多函数/类方法互相递归**（Mutual/Indirect Recursion）。
  - 在编辑器左侧行号槽位（Gutter）显示类似 IntelliJ IDEA 的循环箭头图标（支持深色与浅色自适应主题）。
  - 函数声明行与递归调用点均显示详细气泡提示，直观展示完整调用环路（如 `Test.a() ➔ Test.b() ➔ Test.a()`）。
  - 双重引擎保障：基于 ANTLR ParseTree 的深度调用图环路分析 + 文本级零延迟快速扫描回退。

## [0.2.8] - 2026-09-23

### Fixed
- 修复 `findDefinition` 正则转义缺失引发的 LSP 错误 (`escaped is not defined`, Code: -32603)。
- 增强定义跳转：支持 `new ClassName().method()` 链式实例化直接跳转至目标方法。
- 支持类内部互相调用定义跳转（如 `fun a() { b(); }` 能够跳转至 `fun b()`）。


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