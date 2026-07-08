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
    for (var i = 0; i < 5; i++) {
        print("Thread: " + i);
        sleep(1000);
    }
}

var t = new Thread(worker);
t.start();
t.join();
```

## VSCode 扩展功能

### 支持的命令
- **TzdLang: 运行当前脚本** (F5) - 运行当前打开的 .tzd 文件
- **TzdLang: 重新设置 TzdTools 路径** - 配置解释器路径
- **TzdLang: Hello World** - 显示问候信息

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
- 调试支持
- 一键运行

## 测试项目

项目包含丰富的测试用例，涵盖语言的各个方面：

- **基本功能测试**
  - `test_loop.tzd` - 循环测试
  - `test_bool.tzd` - 布尔值测试
  - `test_func_loop.tzd` - 函数和循环组合测试

- **高级特性测试**
  - `test_script.tzd` - 脚本功能测试
  - `test_error_demo.tzd` - 错误处理演示
  - `test_feat.tzd` - 功能特性测试

- **性能测试**
  - `bench.tzd` - 性能基准测试
  - `test_100k_while.tzd` - 大型循环性能测试

## 开发工具

### 性能基准测试

项目包含 Python 和 TzdLang 的对比基准测试：

```bash
# Python 版本
python bench.py

# TzdLang 版本
TzdTools.exe --runMainTzd=bench.tzd
```

### 代码生成

使用 ANTLR4 生成的解析器：
```bash
# 生成词法分析器和语法分析器
antlr4 -Dlanguage=Cpp -no-listener -visitor TzdLang.g4
```

## 标准库

### 核心库 (`stdlib/core/`)
- **Error.tzd** - 错误处理和异常机制
- **DatabaseConnectionError.tzd** - 数据库连接错误处理

### 线程库 (`stdlib/thread/`)
- **Thread.tzd** - 多线程支持

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