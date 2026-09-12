# TzdLang 语言规范与语法手册

<p align="right">
  <a href="Language-Specification.md"><strong>English</strong></a> | <a href="Language-Specification-zh.md"><strong>中文</strong></a>
</p>

本文档全面阐述 **TzdLang (TZD)** 编程语言的语法结构、语义特性、类型系统及标准编程范式。

---

## 1. 词法约定与注释

TzdLang 采用类 C 语言的词法规范：

```tzd
// 单行注释

/*
 * 多行块级
 * 注释示例
 */
```

- 语句必须以分号 `;` 结尾。
- 标识符支持字母、下划线及数字组合（首字符不可为数字）。

---

## 2. 变量与类型系统

TzdLang 采用以**动态类型为主、支持可选静态类型注解**的渐进式类型系统：

```tzd
// 动态类型声明（类型自动推断）
var x = 100;
var name = "TzdLang";
var is_active = true;

// 静态强类型注解（可在 JIT 编译器中触发直接解包优化）
var int count = 5;
var double pi = 3.1415926535;
var string title = "Compiler Engineer";
var bool flag = false;
```

### 核心支持的数据类型
| 类型名称 | 说明与特性 | 语法示例 |
|---|---|---|
| `int` | 64 位有符号整型（`int64_t`） | `42`, `-100` |
| `double` | 64 位 IEEE-754 双精度浮点型 | `3.14`, `1e-5` |
| `string` | UTF-8 编码字符串，支持动态拼接 | `"Hello World"` |
| `bool` | 布尔真值 | `true`, `false` |
| `array` | 动态顺序容器（类似动态向量） | `[1, 2, 3, "four"]` |
| `map` | 键值哈希字典（关联映射表） | `{"key": "value"}` |
| `object` | 用户自定义类实例对象引用 | `new Person("Alice")` |
| `tensor` | LibTorch 多维张量原生对象 | `torch_randn([3, 3])` |
| `null` | 空对象引用 | `null` |

---

## 3. 控制流

### If-Else 条件分支
```tzd
if (score >= 90) {
    print("Grade: A");
} else if (score >= 80) {
    print("Grade: B");
} else {
    print("Grade: C");
}
```

### While 循环
```tzd
var i = 0;
while (i < 10) {
    if (i == 5) {
        break; // 跳出循环
    }
    print("Counter: " + toString(i));
    i = i + 1;
}
```

### For 循环
> **重要语法规则**：在 TzdLang 中，`for` 循环头部的初始化表达式变量不需要且不可重复使用 `var` 声明：

```tzd
// 正确的 for 循环写法
for (i = 0; i < 10; i++) {
    print(i);
}

for (k = 0; k < 20; k = k + 2) {
    print(k);
}
```

---

## 4. 函数、闭包与高阶调用

函数通过 `fun` 关键字声明，函数是 TzdLang 的一等公民（First-Class Citizens）：

```tzd
fun add(a, b) {
    return a + b;
}

// 高阶函数与回调调用
fun apply_twice(f, val) {
    return f(f(val));
}

var result = add(10, 20); // 30
```

---

## 5. 面向对象编程（类、继承与构造级联）

TzdLang 原生支持单继承体系、构造函数级联、字段作用域绑定以及基于虚方法表的多态派发：

```tzd
class BaseShape {
    var string name;

    // 构造函数
    BaseShape(name) {
        this.name = name;
    }

    // 虚方法
    fun area() {
        return 0.0;
    }

    fun describe() {
        print("Shape [" + this.name + "] Area: " + toString(this.area()));
    }
}

class Rectangle extends BaseShape {
    var double width;
    var double height;

    // super 构造函数显式级联调用
    Rectangle(name, w, h) : super(name) {
        this.width = w;
        this.height = h;
    }

    // 重写父类虚方法
    fun area() {
        return this.width * this.height;
    }
}

var rect = new Rectangle("MyRect", 4.0, 5.0);
rect.describe(); // 输出: Shape [MyRect] Area: 20
```

---

## 6. 异常处理与模式匹配

TzdLang 具备现代结构化异常捕获机制，结合 `in` 运算符实现强大的类型模式匹配：

```tzd
import "core/Error.tzd";

class CustomDatabaseError extends Error {
    CustomDatabaseError(msg) : super(msg, "ERR_DB") {}
}

fun queryData(query) {
    if (query == "") {
        throw new CustomDatabaseError("Query string cannot be empty");
    }
    return "Data rows";
}

try {
    queryData("");
} catch (e) {
    // 利用 `in` 运算符进行异常类型检测
    if (e in CustomDatabaseError) {
        print("Caught Database Error: " + e.message);
    } else {
        print("Caught General Error: " + toString(e));
    }
}
```

---

## 7. 原生多线程并发（Thread）

TzdLang 标准库直接封装操作系统级内核线程（非伪协程），支持真并发执行：

```tzd
import "thread/Thread.tzd";

fun task1() {
    for (i = 0; i < 3; i++) {
        print("Task 1 processing: " + toString(i));
        sleep(200);
    }
}

fun task2() {
    for (j = 0; j < 3; j++) {
        print("Task 2 processing: " + toString(j));
        sleep(300);
    }
}

var t1 = new Thread(task1);
var t2 = new Thread(task2);

// 启动原生线程
t1.start();
t2.start();

// 等待线程同步汇聚
t1.join();
t2.join();
print("所有工作线程已成功退出同步。");
```
