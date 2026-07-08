# TzdTools

TzdTools 是一个自定义编程语言 TzdLang 的开发工具链，包含语言解释器、VSCode 扩展支持和丰富的标准库。

## 项目概述

TzdLang 是一门面向对象的编程语言，具有现代语言特性，包括：

- 类继承和多态
- 异常处理机制
- 线程支持
- 动态类型系统
- JIT 编译支持

## 项目结构

```
TzdTools/
├── README.md                    # 项目说明文档
├── bench.py                     # Python 性能基准测试
├── bench.tzd                   # TzdLang 性能基准测试
├── TzdLang_Reference.pdf       # 语言参考手册
├── TzdTools/                   # C++ 源代码和构建产物
│   ├── stdlib/                 # 标准库
│   │   ├── core/               # 核心功能库
│   │   └── thread/             # 线程支持库
│   └── x64/                    # 构建输出目录
│       ├── Release/            # 发布版本
│       └── Debug/              # 调试版本
├── vscodePlugin/               # VSCode 扩展
│   └── tzdlang/                # TzdLang VSCode 扩展
├── examples/                   # 示例代码
├── test_*.tzd                  # 测试文件
└── Generated/                 # ANTLR 生成的代码
```

## 快速开始

### 构建项目

项目使用 C++ 开发，需要 Visual Studio 2019 或更高版本：

```bash
# 使用 Visual Studio 打开解决方案
# 或在命令行中构建
msbuild TzdTools.sln /p:Configuration=Release /p:Platform=x64
```

### VSCode 扩展安装

项目包含完整的 VSCode 扩展支持：

1. 安装扩展 (直接使用 .vsix 文件或开发模式)
2. 配置 TzdTools.exe 路径
3. 享受语法高亮、代码补全和调试功能

### 语言特性

#### 基本语法
```tzd
// 变量声明
var int x = 10;
var string name = "TzdLang";

// 函数定义
fun greet(name) {
    print("Hello, " + name + "!");
}

// 类定义
class Person {
    var string name;
    
    Person(name) {
        this.name = name;
    }
    
    fun introduce() {
        print("I am " + this.name);
    }
}
```

#### 控制流

**注意：for 循环的第一个表达式不能指定类型，包括 var**

正确的 for 循环语法：
```tzd
// 正确的语法
for (i = 0; i < 5; i = i + 1) {
    print(i);
}

// 正确的语法
for (j = 0; j < 10; j++) {
    if (j == 3) break;
    print(j);
}

// 错误的语法（不支持）
for (var i = 0; i < 5; i++) {
    // 这种写法会报错
}
```

**while 循环和 break 语句**
```tzd
// while 循环
var i = 0;
while (i < 5) {
    print("i = " + i);
    i = i + 1;
}

// 使用 break 退出循环
for (j = 0; j < 10; j++) {
    if (j == 3) break;
    print(j);
}
```

#### 继承和多态
```tzd
class Animal {
    var string name;
    Animal(name) { this.name = name; }
    fun speak() { print(this.name + " says ..."); }
}

class Dog extends Animal {
    Dog(name) : super(name) {}
    fun speak() { print(this.name + " barks! woof!"); }
}

fun testOop() {
    var d = new Dog("Buddy");
    d.speak();  // 输出: Buddy barks! woof!
}
```

#### 异常处理
```tzd
import "core/Error.tzd";

fun runDemo() {
    try {
        throw new Error("test error", "TEST");
    } catch (err) {
        print("caught: " + err.code + " " + err.message);
    }
}
```

#### 线程支持
```tzd
import "thread/Thread.tzd";

fun worker() {
    for (i = 0; i < 5; i++) {
        print("Thread: " + i);
        sleep(1000);
    }
}

var t = new Thread(worker);
t.start();
t.join();
```

### 调试功能

#### 调试命令
TzdLang 支持完整的调试功能，包括：

**断点管理**
- 设置断点：通过调试器界面或命令行
- 清除断点：支持按文件和行号删除
- 条件断点：支持基于条件的断点触发

**调试控制**
- `continue` - 继续执行到下一个断点
- `step into` - 进入当前函数调用
- `step over` - 跳过当前函数调用
- `step out` - 退出当前函数调用
- `pause` - 暂停执行

**状态检查**
- 堆栈跟踪：显示函数调用堆栈
- 变量查看：检查当前作用域的变量值
- 表达式求值：实时计算表达式值

#### 调试示例
```tzd
fun greet(name) {
    print("Greeting started...");
    msg = "Hello " + name;
    print(msg);
    print("Greeting ended.");
}

print("Main script started.");
greet("TzdLang");
print("Main script ended.");
```

#### 性能监控
```tzd
fun testPerformance() {
    var start = clock();
    var i = 0;
    while (i < 50000) {
        if (i == 10000) { print("进度: 20%"); }
        if (i == 20000) { print("进度: 40%"); }
        if (i == 30000) { print("进度: 60%"); }
        if (i == 40000) { print("进度: 80%"); }
        i = i + 1;
    }
    var end = clock();
    print("完成，耗时: " + ((end - start) / 1000.0) + " s");
}
```

#### 异常处理调试
```tzd
fun testException() {
    var i = 0;
    while (i < 100000) {
        if (i == 10) {
            throw "测试异常";
        }
        i = i + 1;
    }
}

testException();
```

## VSCode 扩展功能

### 支持的命令
- **TzdLang: 运行当前脚本** (F5) - 运行当前打开的 .tzd 文件
- **TzdLang: 重新设置 TzdTools 路径** - 配置解释器路径
- **TzdLang: Hello World** - 显示问候信息

### 调试功能
完整的集成调试支持，包括：

**调试控制**
- 启动调试：F5 或运行按钮
- 暂停/继续：暂停执行和继续运行
- 单步执行：
  - Step Into (F11) - 进入函数
  - Step Over (F10) - 跳过函数
  - Step Out (Shift+F11) - 退出函数
- 断点管理：
  - 点击行号设置断点
  - 右键菜单管理断点
  - 条件断点支持

**调试信息显示**
- 变量监视：实时查看变量值
- 调用堆栈：显示函数调用层次
- 输出控制台：显示程序输出和调试信息
- 错误提示：语法和运行时错误提示

### 配置选项
```json
{
    "tzdlang.toolsPath": "C:/path/to/TzdTools.exe"
}
```

### 语言支持
- 语法高亮
- 代码自动补全
- 错误检查
- 完整的调试支持
- 一键运行
- 断点管理
- 堆栈跟踪
- 变量监视

### 使用示例

**基本调试**
1. 打开 .tzd 文件
2. 设置断点（点击行号）
3. 按 F5 启动调试
4. 使用调试控制按钮控制执行
5. 查看变量值和堆栈信息

**调试配置**
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "TzdLang Debug",
            "type": "tzdlang",
            "request": "launch",
            "program": "${file}",
            "stopOnEntry": true,
            "console": "integratedTerminal"
        }
    ]
}
```

## 测试项目

项目包含丰富的测试用例，涵盖语言的各个方面：

### 基本功能测试
- `test_loop.tzd` - while 循环语法测试
- `test_bool.tzd` - 布尔值和条件表达式测试
- `test_func_loop.tzd` - 函数和循环组合测试
- `test_break.tzd` - break 语句测试
- `test_1k_loop.tzd` - 基础循环性能测试

### 高级特性测试
- `test_script.tzd` - 脚本功能测试（继承、多态、异常处理）
- `test_error_demo.tzd` - 错误处理演示
- `test_feat.tzd` - 语言功能特性测试
- `test_debug.tzd` - 调试功能演示
- `test_throw_while.tzd` - 循环中的异常处理

### 控制流测试
- `test_progress_while.tzd` - 大型循环进度监控
- `test_100k_while.tzd` - 大型循环性能测试
- `test_100k_exit_no_semicolon.tzd` - 退出语句测试
- `test_no_loop.tzd` - 无循环代码测试

### 面向对象测试
- `examples/test.tzd` - 基础面向对象功能演示
- `test_script.tzd` - 类继承和方法重写

### 性能基准测试
- `bench.tzd` - 基本性能基准测试
- `bench_func.tzd` - 函数调用性能测试
- `math_loop.tzd` - 数学计算性能测试

### 运行测试
```bash
# 运行所有测试
TzdTools.exe --runMainTzd=test_all.tzd

# 运行单个测试
TzdTools.exe --runMainTzd=test_loop.tzd

# 运行性能测试
TzdTools.exe --runMainTzd=bench.tzd
```

## 开发工具

### 性能基准测试

项目包含 Python 和 TzdLang 的对比基准测试：

```bash
# Python 版本
python bench.py

# TzdLang 版本
TzdTools.exe --runMainTzd=bench.tzd

# 函数调用性能测试
TzdTools.exe --runMainTzd=bench_func.tzd

# 数学计算性能测试
TzdTools.exe --runMainTzd=math_loop.tzd
```

### 构建和编译

项目使用 Visual Studio 2019 或更高版本构建：

```bash
# 构建发布版本
msbuild TzdTools.sln /p:Configuration=Release /p:Platform=x64

# 构建调试版本
msbuild TzdTools.sln /p:Configuration=Debug /p:Platform=x64

# 清理构建文件
msbuild TzdTools.sln /p:Clean=true
```

### 代码生成

使用 ANTLR4 生成的解析器：

```bash
# 生成词法分析器和语法分析器
antlr4 -Dlanguage=Cpp -no-listener -visitor TzdLang.g4

# 生成访问者模式的代码
antlr4 -Dlanguage=Cpp -visitor TzdLang.g4

# 生成监听器模式的代码
antlr4 -Dlanguage=Cpp -listener TzdLang.g4
```

### 调试工具

**命令行调试**
```bash
# 启动调试模式
TzdTools.exe --debug --runMainTzd=script.tzd

# 连接调试客户端
TzdTools.exe --debug-server --port=8080

# 堆栈跟踪
TzdTools.exe --stack-trace --runMainTzd=script.tzd
```

**调试命令系统**
- `break <file>:<line>` - 设置断点
- `clear <file>:<line>` - 清除断点
- `continue` - 继续执行
- `step` - 单步执行
- `next` - 下一步
- `finish` - 完成当前函数
- `where` - 显示堆栈
- `print <expr>` - 打印表达式值

### 开发环境配置

**VSCode 扩展开发**
```bash
# 安装依赖
cd vscodePlugin/tzdlang
npm install

# 构建扩展
npm run compile

# 运行测试
npm test

# 打包扩展
npm run vsce-package
```

**IDE 配置**
```json
{
    "files.associations": {
        "*.tzd": "tzdlang",
        "*.tzdlang": "tzdlang"
    },
    "editor.formatOnSave": true,
    "editor.codeActionsOnSave": {
        "source.fixAll": true
    }
}
```

## 标准库

### 核心库 (`stdlib/core/`)

**Error.tzd** - 错误处理和异常机制
```tzd
// 自定义异常
throw new Error("错误消息", "错误代码");

// 异常捕获
try {
    // 可能抛出异常的代码
    riskyOperation();
} catch (err) {
    print("错误: " + err.message + " (代码: " + err.code + ")");
}
```

**DatabaseConnectionError.tzd** - 数据库连接错误处理
```tzd
// 数据库连接错误处理
import "core/DatabaseConnectionError.tzd";

try {
    // 数据库操作
    db.connect();
} catch (err) {
    if (err in DatabaseConnectionError) {
        print("数据库连接失败: " + err.message);
    }
}
```

### 线程库 (`stdlib/thread/`)

**Thread.tzd** - 多线程支持
```tzd
import "thread/Thread.tzd";

// 创建线程
fun workerThread() {
    for (var i = 0; i < 5; i++) {
        print("工作线程: " + i);
        sleep(1000);
    }
}

var t = new Thread(workerThread);
t.start();  // 启动线程
t.join();   // 等待线程完成
```

### 使用标准库
```tzd
// 导入标准库
import "core/Error.tzd";
import "thread/Thread.tzd";

// 使用标准库功能
fun main() {
    // 使用异常处理
    try {
        throw new Error("测试错误", "TEST_ERROR");
    } catch (err) {
        print("捕获错误: " + err.message);
    }
    
    // 使用线程
    var thread = new Thread(fun() {
        print("在线程中执行");
    });
    thread.start();
    thread.join();
}
```

## 使用场景

- 脚本开发
- 教育和演示
- 性能测试
- 原型开发
- 语言研究

## 许可证

[在此添加许可证信息]

## 贡献指南

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 发起 Pull Request

## 支持

如有问题或建议，请提交 Issue 或联系开发者。

---

TzdLang - 现代化的编程语言开发工具链