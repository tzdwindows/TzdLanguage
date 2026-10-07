# 标准库参考手册

<p align="right">
  <a href="Standard-Library-Reference.md"><strong>English</strong></a> | <a href="Standard-Library-Reference-zh.md"><strong>中文</strong></a>
</p>

TzdLang 附带了模块化现代标准库，位于仓库 `stdlib/` 目录下。

---

## 1. 原生全局内置函数

以下核心基础函数无需显式导入，在任何 TzdLang 脚本中均可全局直接调用：

| 函数名称 | 函数签名 | 功能描述 |
|---|---|---|
| `print` | `print(value)` | 将对象转换字符串并打印输出到标准控制台 |
| `clock` | `clock() -> double` | 获取高精度系统单调递增时间戳（单位：毫秒） |
| `sleep` | `sleep(milliseconds)` | 挂起当前线程指定时间（单位：毫秒） |
| `toString` | `toString(val) -> string` | 将任意原生类型或类实例转换为字符串 |
| `toInt` | `toInt(val) -> int` | 将浮点数或数字字符串强制转换为 64 位整型 |
| `toDouble` | `toDouble(val) -> double` | 将整数或数字字符串强制转换为双精度浮点型 |
| `length` | `length(container) -> int` | 返回动态数组元素个数或字符串字符长度 |

---

## 2. 核心基础设施库 (`stdlib/core/`)

### `core/Error.tzd`
面向对象结构化异常体系的基类定义：

```tzd
class Error {
    var string message;
    var string code;

    Error(message, code) {
        this.message = message;
        this.code = (code != null) ? code : "GENERAL_ERROR";
    }

    fun toString() {
        return "[" + this.code + "] " + this.message;
    }
}
```

#### 使用范式：
```tzd
import "core/Error.tzd";

try {
    throw new Error("找不到目标文件", "ERR_FILE_NOT_FOUND");
} catch (e) {
    if (e in Error) {
        print(e.toString());
    }
}
```

---

## 3. 并发多线程库 (`stdlib/thread/`)

### `thread/Thread.tzd`
底层操作系统原生线程封装：

```tzd
class Thread {
    var worker_fn;
    var thread_handle;

    Thread(worker_fn) {
        this.worker_fn = worker_fn;
    }

    fun start() {
        // 将 worker_fn 派发至独立 OS 系统工作线程执行
    }

    fun join() {
        // 阻塞当前主线程，直至工作线程退出完毕
    }
}
```

#### 使用范式：
```tzd
import "thread/Thread.tzd";

fun backgroundWorker() {
    for (i = 0; i < 5; i++) {
        print("工作线程运行中: " + toString(i));
        sleep(100);
    }
}

var worker = new Thread(backgroundWorker);
worker.start();
worker.join();
```

---

## 4. 深度学习算子库 (`stdlib/torch/`)

### `stdlib/torch/nn.tzd`
封装了基于 LibTorch 底层 C++ 的高级神经网络组件：

- `nn_Linear(in_features, out_features, bias=true)`：全连接仿射线性变换层。
- `nn_Sequential(layers...)`：按顺序级联的前向容器。
- `optim_SGD(parameters, lr, momentum)`：随机梯度下降优化器（支持动量）。
- `optim_Adam(parameters, lr, beta1=0.9, beta2=0.999)`：自适应矩估计优化器。
- `optim_AdamW(parameters, lr, weight_decay)`：解耦权重衰减项的高级 AdamW 优化器。
