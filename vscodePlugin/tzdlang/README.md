# TzdLang 语言支持插件

为 TzdLang 脚本提供专属的 VS Code 开发支持。

## 功能

- **语法高亮**：完整的关键字、类型、字符串、注释、数字和运算符高亮。
- **快捷执行**：按 `F5` 或点击编辑器工具栏播放按钮，一键调用 `TzdTools.exe` 运行当前脚本。
- **路径配置**：通过 `TzdLang: 重新设置 TzdTools 路径` 命令指定解释器位置，插件会自动记忆。
- **智能路径查找**：自动在 `../x64/Release/`、workspace 邻近目录、PATH 环境变量中搜索 `TzdTools.exe`。
- **标准库支持**：自动加载 TzdTools 同级的 `stdlib` 目录，确保 `import` 语句正确工作。
- **输出通道**：运行结果展示在专用的 `TzdLang` 输出通道中，支持错误高亮显示。

## 配置

1. 按 `Ctrl+Shift+P` 打开命令面板。
2. 执行 `TzdLang: 重新设置 TzdTools 路径`。
3. 在弹出的文件选择对话框中定位到 `TzdTools.exe`。
4. 也可在 VS Code 设置中直接搜索 `tzdlang.toolsPath` 进行配置。

## 使用

- 打开 `.tzd` 或 `.tzdlang` 文件。
- 按 `F5` 或点击编辑器右上角的播放按钮运行。
- 运行结果会显示在 `TzdLang` 输出面板中。

## 开发

### 项目结构

```
tzdlang/
  extension.js                  # 扩展主入口
  package.json                  # 扩展清单与配置
  syntaxes/
    tzdlang.tmLanguage.json     # TextMate 语法高亮
    language-configuration.json # 语言配置（括号、注释等）
  test/
    extension.test.js           # 扩展测试
```

### 构建和测试

```bash
npm install
npm run lint
npm test
```

### 打包

```bash
npx vsce package
```