# Standard Library Reference Manual

TzdLang ships with a modular standard library located under the `stdlib/` directory.

---

## 1. Built-in Core Functions

These functions are globally available in all scripts without requiring explicit imports:

| Function | Signature | Description |
|---|---|---|
| `print` | `print(value)` | Prints stringified value to standard output |
| `clock` | `clock() -> double` | Returns monotonic system time in milliseconds |
| `sleep` | `sleep(milliseconds)` | Suspends the calling thread for specified duration |
| `toString` | `toString(val) -> string` | Converts any primitive or object to string |
| `toInt` | `toInt(val) -> int` | Coerces floating-point or string to 64-bit integer |
| `toDouble` | `toDouble(val) -> double` | Coerces integer or string to 64-bit float |
| `length` | `length(container) -> int` | Returns number of elements in array or string |

---

## 2. Core Library (`stdlib/core/`)

### `core/Error.tzd`
Base class for structured exception handling:

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

#### Usage:
```tzd
import "core/Error.tzd";

try {
    throw new Error("File not found", "ERR_FILE_NOT_FOUND");
} catch (e) {
    if (e in Error) {
        print(e.toString());
    }
}
```

---

## 3. Thread Library (`stdlib/thread/`)

### `thread/Thread.tzd`
Wrapper over native OS thread primitives:

```tzd
class Thread {
    var worker_fn;
    var thread_handle;

    Thread(worker_fn) {
        this.worker_fn = worker_fn;
    }

    fun start() {
        // Dispatches worker_fn onto an OS worker thread
    }

    fun join() {
        // Blocks until worker thread terminates
    }
}
```

#### Usage:
```tzd
import "thread/Thread.tzd";

fun backgroundWorker() {
    for (i = 0; i < 5; i++) {
        print("Worker running: " + toString(i));
        sleep(100);
    }
}

var worker = new Thread(backgroundWorker);
worker.start();
worker.join();
```

---

## 4. Deep Learning Library (`stdlib/torch/`)

### `stdlib/torch/nn.tzd`
Provides high-level neural network abstractions built on LibTorch:

- `nn_Linear(in_features, out_features, bias=true)`: Fully connected affine layer.
- `nn_Sequential(layers...)`: Container cascading sequential layers.
- `optim_SGD(parameters, lr, momentum)`: Stochastic gradient descent optimizer.
- `optim_Adam(parameters, lr, beta1=0.9, beta2=0.999)`: Adaptive moment estimation optimizer.
- `optim_AdamW(parameters, lr, weight_decay)`: Adam with decoupled weight decay.
