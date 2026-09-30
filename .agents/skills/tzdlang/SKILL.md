---
name: tzdlang
description: Comprehensive guide to TzdLanguage (TzdLang / TzdTools) syntax, OOP, standard library (stdlib), neural network training, JIT performance optimization, and common programming patterns.
---

# TzdLanguage (TzdLang) Developer Guide & AI Skill

TzdLanguage (`.tzd`) is a high-performance modern programming language featuring LLVM JIT compilation, native PyTorch C++ (libtorch) integration, object-oriented programming (OOP), high-precision math (BigInt, exact Rational fractions), and an extensive standard library.

---

## 1. Quick Start & Execution Modes

| Command | Description |
| :--- | :--- |
| `TzdTools.exe <script.tzd>` | Directly run a `.tzd` script with LLVM JIT enabled. |
| `TzdTools.exe` | Launch the interactive REPL (`Tzd>`). |
| `TzdTools.exe build <script.tzd> -o <app.exe>` | Compile script into a standalone native `.exe` binary. |
| `TzdTools.exe build <script.tzd> --buildCpu -o <app.exe>` | Compile standalone CPU-only executable (no CUDA/libtorch dependency). |
| `TzdTools.exe --no-jit <script.tzd>` | Run script in pure tree-walking interpreter mode. |

---

## 2. Basic Syntax & Grammar

### 2.1 Comments & Semicolons
- Every statement **must end with a semicolon `;`**.
- Single-line comments: `// comment`
- Multi-line comments: `/* comment */`

```tzd
// Single line comment
/* Multi-line
   comment */
var x = 42;
```

### 2.2 Variables & Type Declarations
Variables are declared using `var`. They can be dynamically typed or explicitly typed:

```tzd
// Dynamic / Inferred:
var name = "Alice";
var count = 100;
var pi = 3.1415926535;
var active = true;

// Explicitly Typed:
var int id = 1001;
var float rate = 0.05;
var string title = "Engineer";
var bool ok = false;
var BigInt bigVal = 123456789012345678901234567890;
var Rational fraction = 1 / 3;
```

### 2.3 Built-in Primitive Types
1. **`int`**: 64-bit signed integer.
2. **`float` / `double`**: 64-bit IEEE 754 floating-point.
3. **`string`**: UTF-8 string with automatic concatenation.
4. **`bool`**: `true` or `false`.
5. **`null`**: Null / None value.
6. **`BigInt`**: Arbitrary-precision integer supporting `+`, `-`, `*`, `/`, `%`, `^`.
7. **`Rational`**: Exact rational fraction (`num/den`) preventing floating-point rounding errors.

### 2.4 Collections: Array & Map
- **Arrays**: 0-indexed, dynamic sizing:
  ```tzd
  var arr = [10, 20, 30, 40];
  var first = arr[0];
  arr.push(50);
  var len = arr.length(); // or arr.size()
  var last = arr.pop();
  ```
- **Maps**: Key-value pairs:
  ```tzd
  var user = {
      "name": "Bob",
      "age": 28,
      "admin": true
  };
  print(user["name"]);
  user["email"] = "bob@example.com";
  ```

---

## 3. Operators & Control Flow

### 3.1 Operators
- Arithmetic: `+`, `-`, `*`, `/`, `%`, `^` (Power operator, e.g. `2 ^ 10 == 1024`).
- String concatenation: The `+` operator automatically coerces numbers to strings when concatenated with a string:
  ```tzd
  var msg = "Count is: " + toString(count) + "!";
  ```
- Comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical: `&&`, `||`, `!`
- Bitwise: `&`, `|`, `^`, `~`, `<<`, `>>`
- Ternary: `condition ? exprTrue : exprFalse`

### 3.2 If / Else
```tzd
if (score >= 90) {
    print("Grade A");
} else if (score >= 80) {
    print("Grade B");
} else {
    print("Grade C");
}
```

### 3.3 Loops
```tzd
// While Loop
var i = 0;
while (i < 5) {
    print("i = " + toString(i));
    i = i + 1;
}

// For Loop
for (var j = 0; j < 10; j = j + 1) {
    if (j == 3) continue;
    if (j == 8) break;
    print("j = " + toString(j));
}
```

### 3.4 Switch / Case
```tzd
switch (status) {
    case 1:
        print("Starting");
        break;
    case 2:
        print("Running");
        break;
    default:
        print("Unknown status");
        break;
}
```

### 3.5 Exception Handling
```tzd
try {
    if (divisor == 0) {
        throw "Division by zero!";
    }
    var res = 100 / divisor;
} catch (e) {
    print("Caught error: " + toString(e));
}
```

---

## 4. Functions & Lambdas

### 4.1 Function Declarations
Defined using keyword `fun`:
```tzd
fun add(a, b) {
    return a + b;
}

// Typed parameters and return type:
fun float multiply(float x, float y) {
    return x * y;
}
```

### 4.2 First-class Functions & Closures
```tzd
var square = fun(n) {
    return n * n;
};
print(toString(square(5))); // 25
```

### 4.3 High-Performance Recursive Functions (JIT Optimized)
TzdLanguage LLVM JIT compiles purely numeric functions to unboxed native machine code with zero memory allocations per recursion step:
```tzd
fun fib(n) {
    if (n <= 2) {
        return 1.0;
    }
    return fib(n - 1) + fib(n - 2);
}
```

---

## 5. Object-Oriented Programming (OOP)

### 5.1 Classes, Fields & Constructors
- Declare classes with `class ClassName { ... }`.
- Fields are declared with `var`.
- Constructors use the class name directly: `ClassName(params) { ... }`.
- Member variables and methods are accessed via `this.field` or `this.method()`.

```tzd
class Person {
    var string name;
    var int age;

    Person(name, age) {
        this.name = name;
        this.age = age;
    }

    fun introduce() {
        print("Hello, I am " + this.name + ", age " + toString(this.age));
    }
}

var p = new Person("Charlie", 30);
p.introduce();
```

### 5.2 Inheritance & Super Calls
Inherit using `:` and call parent constructor with `super(...)`:
```tzd
class Employee : Person {
    var string department;

    Employee(name, age, dept) : super(name, age) {
        this.department = dept;
    }

    fun introduce() {
        super.introduce();
        print("Department: " + this.department);
    }
}
```

### 5.3 Static Methods
```tzd
class MathUtil {
    static fun clamp(val, low, high) {
        if (val < low) return low;
        if (val > high) return high;
        return val;
    }
}

var val = MathUtil.clamp(15, 0, 10); // 10
```

---

## 6. Standard Library (stdlib)

### 6.1 Module Imports
Modules are imported using `import "path";`:
```tzd
import "time/DateTime.tzd"; // Resolves from stdlib/time/DateTime.tzd
import "fs/File.tzd";        // Resolves from stdlib/fs/File.tzd
import "fs/Path.tzd";
import "sys/Process.tzd";
import "net/Http.tzd";
import "thread/Thread.tzd";
import "crypto/Crypto.tzd";
import "torch/Training.tzd";
```

> [!NOTE]
> `resolveImportPath` supports:
> 1. Standard paths: `import "time/DateTime.tzd";`
> 2. Dot notation: `import "time.DateTime";`
> 3. Short leaf names: `import "DateTime";` or `import "DateTime.tzd";`
> 4. Forward or backward slashes across Windows and Linux.

### 6.2 `time/DateTime.tzd`: Benchmarking & Time
Contains `DateTime`, `TimeSpan`, and `Stopwatch`:

```tzd
import "time/DateTime.tzd";

// 1. High-Precision Benchmarking
var sw = new Stopwatch();
sw.start();

var res = fib(34);

sw.stop();
print("Fib(34) = " + toString(res) + " in " + toString(sw.elapsedMs()) + " ms");

// 2. Current Time & Formatting
var now = DateTime.now();
print("Current time: " + now.toString()); // e.g. 2026-09-30 17:30:00
print("Timestamp ms: " + toString(DateTime.nowMs()));
DateTime.sleep(100); // Sleep for 100 milliseconds
```

### 6.3 `fs/File.tzd` & `fs/Path.tzd`: File System
```tzd
import "fs/File.tzd";
import "fs/Path.tzd";

// Write & Read files
File.writeAllText("sample.txt", "Hello TzdLang!");
if (File.exists("sample.txt")) {
    var content = File.readAllText("sample.txt");
    print("Content: " + content);
}
```

### 6.4 `sys/Process.tzd`: Process Spawning
```tzd
import "sys/Process.tzd";

var proc = Process.run("hostname");
print("Exit code: " + toString(proc.exitCode));
print("Output: " + proc.stdout);
```

---

## 7. PyTorch Integration & Neural Network Training

TzdLanguage contains native C++ bindings for LibTorch (CUDA & CPU):

```tzd
import "torch/Training.tzd";

// 1. Tensor operations
var t1 = torch_randn([2, 3]);
var t2 = torch_randn([3, 4]);
var out = torch_matmul(t1, t2);
print("Output shape: " + toString(torch_shape(out)));

// 2. GPU acceleration
if (torch_cuda_is_available()) {
    var gpuTensor = torch_to_cuda(t1);
    print("Tensor on GPU: " + toString(torch_device(gpuTensor)));
}

// 3. Loss & Optimizers
var loss = torch_mse_loss(pred, target);
torch_backward(loss);
torch_optimizer_step(optimizer);
torch_optimizer_zero_grad(optimizer);
```

---

## 8. Built-in Functions Reference

| Function | Signature / Description |
| :--- | :--- |
| `print(...)` | Prints one or more expressions to standard output. |
| `toString(v)` | Converts any value (number, array, map, object) to string. |
| `toInt(v)` | Converts value to 64-bit integer. |
| `toFloat(v)` | Converts value to 64-bit floating point. |
| `toBool(v)` | Converts value to boolean. |
| `sqrt(x)` | Fast native hardware square root. |
| `abs(x)` | Absolute value. |
| `min(a, b)` | Minimum of two numbers. |
| `max(a, b)` | Maximum of two numbers. |
| `sin(x)`, `cos(x)`, `tan(x)` | Trigonometric functions. |
| `time_now_sec()` | Current UNIX timestamp in seconds (float). |
| `time_now_ms()` | Current UNIX timestamp in milliseconds (float). |
| `time_now_ns()` | Current timestamp in nanoseconds (float). |
| `time_sleep(ms)` | Pauses execution for `ms` milliseconds. |

---

## 9. Common Pitfalls to Avoid for AI Assistants

1. **Do NOT use Python or JavaScript function keywords**:
   - ❌ `def foo():` or `function foo() {}`
   - ✅ `fun foo() {}`
2. **Do NOT omit semicolons**:
   - Every statement requires a trailing semicolon `;`.
3. **Always use `this.` for member access**:
   - Inside class methods, access member variables as `this.myField`.
4. **Use forward slashes in string paths**:
   - In Windows paths, raw backslashes like `"D:\tools"` have `\t` interpreted as a tab character. Use `"D:/tools"` or `"D:\\tools"`.
5. **Constructors**:
   - Constructor name is identical to the class name: `Stopwatch() { ... }`, not `constructor()` or `__init__`.
6. **Stopwatch & DateTime are in `time/DateTime.tzd`**:
   - Whenever timing or benchmarking is needed, always start with:
     ```tzd
     import "time/DateTime.tzd";
     var sw = new Stopwatch();
     sw.start();
     // work
     sw.stop();
     print("Elapsed: " + toString(sw.elapsedMs()) + " ms");
     ```
