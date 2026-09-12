# TzdLang Language Specification & Syntax Guide

This document provides a comprehensive guide to the syntax, semantics, and standard programming paradigms of the **TzdLang (TZD)** programming language.

---

## 1. Lexical Conventions & Comments

```tzd
// Single-line comment

/*
 * Multi-line
 * comment block
 */
```

Statements are terminated by semicolons `;`.

---

## 2. Variables & Type System

TzdLang supports dynamic typing with optional static type annotations:

```tzd
// Dynamically typed variables
var x = 100;
var name = "TzdLang";
var is_active = true;

// Statically annotated variables
var int count = 5;
var double pi = 3.1415926535;
var string title = "Compiler Engineer";
var bool flag = false;
```

### Supported Data Types
| Type | Description | Example |
|---|---|---|
| `int` | 64-bit signed integer | `42`, `-100` |
| `double` | 64-bit IEEE-754 floating point | `3.14`, `1e-5` |
| `string` | UTF-8 encoded text string | `"Hello World"` |
| `bool` | Boolean truth values | `true`, `false` |
| `array` | Dynamic array container | `[1, 2, 3, "four"]` |
| `map` | Key-value associative table | `{"key": "value"}` |
| `object` | User-defined class instance | `new Person("Alice")` |
| `tensor` | LibTorch multi-dimensional tensor | `torch_randn([3, 3])` |
| `null` | Null reference | `null` |

---

## 3. Control Flow

### If-Else Branches
```tzd
if (score >= 90) {
    print("Grade: A");
} else if (score >= 80) {
    print("Grade: B");
} else {
    print("Grade: C");
}
```

### While Loops
```tzd
var i = 0;
while (i < 10) {
    if (i == 5) {
        break; // Exit loop
    }
    print("Counter: " + toString(i));
    i = i + 1;
}
```

### For Loops
> **Important**: In TzdLang, loop variables in `for` expressions must not redeclare `var` inside the loop initializer:

```tzd
// Correct for loop syntax
for (i = 0; i < 10; i++) {
    print(i);
}

for (k = 0; k < 20; k = k + 2) {
    print(k);
}
```

---

## 4. Functions & Closures

Functions are declared using the `fun` keyword:

```tzd
fun add(a, b) {
    return a + b;
}

// Higher-order functions & lambda
fun apply_twice(f, val) {
    return f(f(val));
}

var result = add(10, 20); // 30
```

---

## 5. Object-Oriented Programming (Classes & Inheritance)

TzdLang provides single inheritance, constructors, virtual method dispatch, and constructor cascading via `super`:

```tzd
class BaseShape {
    var string name;

    BaseShape(name) {
        this.name = name;
    }

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

    Rectangle(name, w, h) : super(name) {
        this.width = w;
        this.height = h;
    }

    // Override virtual method
    fun area() {
        return this.width * this.height;
    }
}

var rect = new Rectangle("MyRect", 4.0, 5.0);
rect.describe(); // Output: Shape [MyRect] Area: 20
```

---

## 6. Exception Handling & Pattern Matching

Structured exception handling is provided by `try`, `catch`, and `throw`:

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
    // Type testing with the `in` operator
    if (e in CustomDatabaseError) {
        print("Caught Database Error: " + e.message);
    } else {
        print("Caught General Error: " + toString(e));
    }
}
```

---

## 7. Native Multi-Threading

TzdLang supports true operating system threads:

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

t1.start();
t2.start();

t1.join();
t2.join();
print("All worker threads joined successfully.");
```
