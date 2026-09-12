# TzdLang Built-in Functions Reference Manual

<p align="right">
  <a href="Builtin-Functions-Reference.md"><strong>English</strong></a> | <a href="Builtin-Functions-Reference-zh.md"><strong>中文</strong></a>
</p>

This document provides a comprehensive cheat-sheet and lookup reference for all native built-in functions available in **TzdLang (TZD)**. TzdLang features over 350+ built-in functions covering system runtime, elementary mathematics, symbolic solving, matrix algebra, string processing, functional array operations, data containers, file I/O, JSON serialization, and LibTorch deep learning.

---

## 📑 Quick Navigation Index

1. [Core I/O & System Reflection](#1-core-io--system-reflection)
2. [Elementary Mathematics & Trigonometry](#2-elementary-mathematics--trigonometry)
3. [Mathematical Constants](#3-mathematical-constants)
4. [Symbolic & Equation Solving](#4-symbolic--equation-solving)
5. [Number Theory, BigInt & Rationals](#5-number-theory-bigint--rationals)
6. [Matrix & Linear Algebra](#6-matrix--linear-algebra)
7. [String Manipulation & Regular Expressions](#7-string-manipulation--regular-expressions)
8. [Array & Functional Algorithms](#8-array--functional-algorithms)
9. [Map & Key-Value Dictionary](#9-map--key-value-dictionary)
10. [Set, Queue & Stack Containers](#10-set-queue--stack-containers)
11. [File System & Path Utilities](#11-file-system--path-utilities)
12. [Type Conversion & Formatting](#12-type-conversion--formatting)
13. [JSON Serialization & Utilities](#13-json-serialization--utilities)
14. [Date, Time & Profiling](#14-date-time--profiling)
15. [LibTorch Deep Learning Primitives](#15-libtorch-deep-learning-primitives)

---

## 1. Core I/O & System Reflection

| Function | Signature | Return Type | Description & Example |
|---|---|---|---|
| `print` | `print(...args)` | `null` | Output values to standard console: `print("Result:", 42);` |
| `input` | `input([prompt])` | `string` | Read a line of input from stdin: `var name = input("Name: ");` |
| `clock` | `clock()` | `double` | High-resolution monotonic timestamp in milliseconds |
| `time` | `time()` | `double` | Unix timestamp in seconds |
| `sleep` | `sleep(ms)` | `null` | Suspend current thread for specified milliseconds: `sleep(200);` |
| `exit` | `exit([code])` | `null` | Terminate process with exit code (default 0): `exit(0);` |
| `len` | `len(obj)` | `int` | Length of string or count of array elements |
| `type` | `type(obj)` | `string` | Uppercase type string (`"INT"`, `"FLOAT"`, `"STRING"`, `"ARRAY"`, `"MAP"`, `"CLASS"`, `"INSTANCE"`, `"TENSOR"`, `"NONE"`) |
| `getSymbols` | `getSymbols()` | `map` | Reflection of all declared classes and functions in scope chain |
| `getClassInfo` | `getClassInfo(target)` | `map` | Inspect class or instance metadata (name, parent, fields, methods) |
| `getFunctions` | `getFunctions()` | `array` | List of user-defined function names |
| `getNativeFunctions` | `getNativeFunctions()` | `array` | List of registered C++ native function names |
| `sys_thread_start` | `sys_thread_start(fn)` | `pointer` | Start native operating system worker thread |
| `sys_thread_join` | `sys_thread_join(th)` | `null` | Wait for native worker thread termination |
| `sys_thread_detach`| `sys_thread_detach(th)`| `null` | Detach worker thread |

---

## 2. Elementary Mathematics & Trigonometry

| Function | Signature | Description & Example |
|---|---|---|
| `abs(x)` | `(double) -> double` | Absolute value: `abs(-5.5); // 5.5` |
| `sqrt(x)` | `(double) -> double` | Square root: `sqrt(16.0); // 4.0` |
| `cbrt(x)` | `(double) -> double` | Cube root: `cbrt(27.0); // 3.0` |
| `hypot(x, y)` | `(double, double) -> double` | Hypotenuse $\sqrt{x^2+y^2}$ |
| `pow(x, y)` | `(double, double) -> double` | Exponentiation $x^y$: `pow(2, 10); // 1024` |
| `exp(x)` | `(double) -> double` | Natural exponential $e^x$ |
| `expm1(x)` | `(double) -> double` | Compute $e^x - 1$ accurately for small $x$ |
| `log(x)` | `(double) -> double` | Natural logarithm $\ln(x)$ |
| `log10(x)` | `(double) -> double` | Base-10 logarithm $\log_{10}(x)$ |
| `log2(x)` | `(double) -> double` | Base-2 logarithm $\log_2(x)$ |
| `logBase(x, b)` | `(double, double) -> double` | Base-$b$ logarithm $\log_b(x)$ |
| `log1p(x)` | `(double) -> double` | Compute $\ln(1+x)$ accurately |
| `sin(rad)` | `(double) -> double` | Sine function (radians) |
| `cos(rad)` | `(double) -> double` | Cosine function (radians) |
| `tan(rad)` | `(double) -> double` | Tangent function (radians) |
| `asin(x)` | `(double) -> double` | Arc sine $[-\frac{\pi}{2}, \frac{\pi}{2}]$ |
| `acos(x)` | `(double) -> double` | Arc cosine $[0, \pi]$ |
| `atan(x)` | `(double) -> double` | Arc tangent |
| `atan2(y, x)` | `(double, double) -> double` | Four-quadrant inverse tangent |
| `degrees(rad)` | `(double) -> double` | Radians to degrees: `degrees(PI); // 180` |
| `radians(deg)` | `(double) -> double` | Degrees to radians: `radians(180); // 3.14159...` |
| `round(x)` | `(double) -> double` | Round to nearest integer: `round(3.6); // 4` |
| `floor(x)` | `(double) -> double` | Floor: `floor(3.9); // 3` |
| `ceil(x)` | `(double) -> double` | Ceiling: `ceil(3.1); // 4` |
| `clamp(x, min, max)` | `(double, double, double) -> double` | Clamp value to interval: `clamp(15, 0, 10); // 10` |
| `lerp(a, b, t)` | `(double, double, double) -> double` | Linear interpolation $a + t \cdot (b - a)$ |
| `sign(x)` | `(double) -> double` | Signum function (-1, 0, or 1) |
| `erf(x)` | `(double) -> double` | Error function |
| `tgamma(x)` | `(double) -> double` | Gamma function $\Gamma(x)$ |

---

## 3. Mathematical Constants

| Constant | Value | Description |
|---|---|---|
| `PI` | `3.141592653589793` | Archimedes' constant $\pi$ |
| `E` | `2.718281828459045` | Euler's number $e$ |
| `TAU` | `6.283185307179586` | Circle constant $2\pi$ |
| `SQRT2` | `1.414213562373095` | Square root of 2 |
| `GOLDEN_RATIO` | `1.618033988749895` | Golden ratio $\phi$ |
| `EPSILON` | `2.220446049250313e-16`| Machine epsilon |
| `INF` | $+\infty$ | Floating-point infinity |
| `NAN` | NaN | Not a Number |

---

## 4. Symbolic & Equation Solving

| Function | Signature | Description & Example |
|---|---|---|
| `solveEq(f, guess)` | `(function, double) -> double` | Newton-Raphson numerical root finding $f(x)=0$:<br>`solveEq(fun(x) { return x * x - 2; }, 1.0); // 1.414213` |
| `solveSym(expr, var)` | `(string, string) -> string` | Symbolic equation solver:<br>`solveSym("2*x + 5 = 15", "x"); // "x = 5"` |
| `simplifySym(expr)` | `(string) -> string` | Algebraic expression simplification:<br>`simplifySym("2*x + 3*x"); // "5*x"` |
| `solveIneq(f, op, val, low, high)` | `(fn, str, dbl, dbl, dbl) -> string` | Numerical inequality solver:<br>`solveIneq(fun(x){ return x*x; }, "<", 4, -10, 10); // "-2 < x < 2"` |
| `derivative(f, x)` | `(function, double) -> double` | Numerical derivative calculation $f'(x)$ |

---

## 5. Number Theory, BigInt & Rationals

| Function | Signature | Description & Example |
|---|---|---|
| `factorial(n)` | `(int) -> double/int` | Factorial $n!$: `factorial(5); // 120` |
| `bigint(val)` | `(string/number) -> bigint` | Construct arbitrary-precision integer |
| `isBigint(val)` | `(any) -> bool` | Check if variable is of BigInt type |
| `bigintFactorial(n)` | `(int) -> bigint` | Multi-thousand-digit factorial |
| `bigintGcd(a, b)` | `(bigint, bigint) -> bigint` | GCD for arbitrary-precision integers |
| `setBigIntMaxDigits(n)` | `(int) -> null` | Set max display digits limit for BigInt |
| `getBigIntMaxDigits()` | `() -> int` | Query max display digits limit |
| `gcd(a, b)` | `(int, int) -> int` | Greatest common divisor: `gcd(48, 18); // 6` |
| `lcm(a, b)` | `(int, int) -> int` | Least common multiple: `lcm(4, 6); // 12` |
| `isPrime(n)` | `(int) -> bool` | Miller-Rabin probabilistic primality test |
| `powmod(b, e, m)` | `(int, int, int) -> int` | Modular exponentiation $b^e \pmod m$ |
| `comb(n, k)` | `(int, int) -> int` | Combinations $C_n^k$ |
| `perm(n, k)` | `(int, int) -> int` | Permutations $A_n^k$ |
| `fib(n)` | `(int) -> int` | $n$-th Fibonacci number |
| `rational(n, d)` | `(int, int) -> rational` | Construct exact rational fraction $\frac{n}{d}$ |
| `toFraction(num)` | `(double) -> string` | Convert float to simplified fraction string |
| `rationalAdd(a, b)` | `(rat, rat) -> rat` | Exact rational addition |
| `rationalMul(a, b)` | `(rat, rat) -> rat` | Exact rational multiplication |

---

## 6. Matrix & Linear Algebra

Powered by Eigen high-performance C++ linear algebra:

| Function | Signature | Description & Example |
|---|---|---|
| `identity(n)` | `(int) -> array` | $n \times n$ identity matrix: `identity(3);` |
| `zeros(r, [c])` | `(int, int) -> array` | $r \times c$ zero matrix |
| `ones(r, [c])` | `(int, int) -> array` | $r \times c$ ones matrix |
| `matrixMul(A, B)` | `(arr, arr) -> array` | Matrix multiplication $A \times B$ |
| `transpose(A)` | `(arr) -> array` | Matrix transpose $A^T$ |
| `inverse(A)` | `(arr) -> array` | Matrix inversion $A^{-1}$ |
| `det(A)` | `(arr) -> double` | Determinant $\det(A)$ |
| `trace(A)` | `(arr) -> double` | Trace of square matrix |
| `rank(A)` | `(arr) -> int` | Matrix rank |
| `solve(A, b)` | `(arr, arr) -> array` | Solve system of linear equations $Ax = b$ |
| `norm(A)` | `(arr) -> double` | Frobenius norm |
| `dot(u, v)` | `(arr, arr) -> double` | Vector dot product |
| `reshape(A, r, c)` | `(arr, int, int) -> array` | Reshape array dimensions |

---

## 7. String Manipulation & Regular Expressions

| Function | Signature | Description & Example |
|---|---|---|
| `split(str, sep)` | `(str, str) -> array` | Split string by separator |
| `join(arr, sep)` | `(arr, str) -> string` | Join elements into string |
| `replace(s, old, new)` | `(str, str, str) -> string` | Substring replacement |
| `substring(s, start, [len])` | `(str, int, int) -> string` | Substring extraction |
| `trim(s)` | `(str) -> string` | Strip leading and trailing whitespaces |
| `toUpper(s)` | `(str) -> string` | Convert to uppercase |
| `toLower(s)` | `(str) -> string` | Convert to lowercase |
| `toTitleCase(s)` | `(str) -> string` | Capitalize initial letters |
| `toCamelCase(s)` | `(str) -> string` | Convert to camelCase |
| `toSnakeCase(s)` | `(str) -> string` | Convert to snake_case |
| `contains(s, sub)` | `(str, str) -> bool` | Check substring inclusion |
| `startsWith(s, pre)` | `(str, str) -> bool` | Check prefix |
| `endsWith(s, suf)` | `(str, str) -> bool` | Check suffix |
| `indexOf(s, sub)` | `(str, str) -> int` | Find index of substring (-1 if absent) |
| `charAt(s, idx)` | `(str, int) -> string` | Character at index |
| `reverseStr(s)` | `(str) -> string` | Reverse string |
| `repeat(s, n)` | `(str, int) -> string` | Repeat string $n$ times |
| `padLeft(s, len, [pad])` | `(str, int, str) -> string` | Pad on left |
| `padRight(s, len, [pad])` | `(str, int, str) -> string` | Pad on right |
| `format(fmt, ...args)` | `(str, ...) -> string` | Sprintf-style formatting |
| `match(s, regex)` | `(str, str) -> array` | Regex capture match |
| `splitRegex(s, regex)` | `(str, str) -> array` | Split with regular expression |
| `replaceRegex(s, re, rep)` | `(str, str, str) -> string` | Regex global replacement |
| `levenshtein(s1, s2)` | `(str, str) -> int` | Levenshtein edit distance |
| `countSubstr(s, sub)` | `(str, str) -> int` | Count non-overlapping occurrences |
| `wordCount(s)` | `(str) -> int` | Count whitespace-separated words |
| `escape(s)` / `unescape(s)` | `(str) -> string` | URI / string escaping & decoding |
| `base64Encode(s)` / `base64Decode(s)` | `(str) -> string` | Base64 encoding and decoding |
| `crc32(s)` / `hash(s)` | `(str) -> int/string` | CRC32 or hash digest |
| `uuid()` | `() -> string` | Generate RFC4122 v4 UUID string |

---

## 8. Array & Functional Algorithms

| Function | Signature | Description & Example |
|---|---|---|
| `push(arr, val)` | `(arr, any) -> arr` | Push element to back |
| `pop(arr)` | `(arr) -> any` | Pop element from back |
| `shift(arr)` | `(arr) -> any` | Remove element from front |
| `unshift(arr, val)` | `(arr, any) -> arr` | Insert element at front |
| `slice(arr, start, [end])` | `(arr, int, int) -> arr` | Shallow slice array |
| `concat(a1, a2)` | `(arr, arr) -> arr` | Concatenate two arrays |
| `reverse(arr)` | `(arr) -> arr` | Reverse array in-place |
| `sort(arr, [cmp])` | `(arr, [fn]) -> arr` | Sort array (optional comparator) |
| `range(start, end, [step])` | `(int, int, int) -> arr` | Generate sequence range |
| `map(arr, fn)` | `(arr, fn) -> arr` | Higher-order mapping function |
| `filter(arr, fn)` | `(arr, fn) -> arr` | Higher-order filter function |
| `reduce(arr, fn, [init])` | `(arr, fn, any) -> any` | Fold / reduction function |
| `find(arr, fn)` | `(arr, fn) -> any` | Find first matching element |
| `includes(arr, val)` | `(arr, any) -> bool` | Array membership check |
| `indexOfArr(arr, val)` | `(arr, any) -> int` | First index of element |
| `fill(arr, val)` | `(arr, any) -> arr` | Fill array with value |
| `flatten(arr)` | `(arr) -> arr` | Flatten nested arrays |
| `zip(a1, a2)` | `(arr, arr) -> arr` | Pairwise element zipping |
| `unique(arr)` | `(arr) -> arr` | Remove duplicate items |
| `sum(arr)` | `(arr) -> double` | Sum of numerical elements |
| `avg(arr)` | `(arr) -> double` | Arithmetic mean of elements |
| `minArr(arr)` / `maxArr(arr)` | `(arr) -> double` | Minimum / maximum element |
| `argmax(arr)` / `argmin(arr)` | `(arr) -> int` | Index of maximum / minimum |
| `cumsum(arr)` | `(arr) -> arr` | Cumulative prefix sums |
| `diff(arr)` | `(arr) -> arr` | First-order discrete differences |
| `shuffle(arr)` | `(arr) -> arr` | Random permutation of array |
| `sample(arr, count)` | `(arr, int) -> arr` | Random sample without replacement |
| `linspace_arr(start, stop, n)` | `(num, num, int) -> arr` | Evenly spaced numbers |

---

## 9. Map & Key-Value Dictionary

| Function | Signature | Description & Example |
|---|---|---|
| `mapKeys(m)` | `(map) -> array` | Array of all dictionary keys |
| `mapValues(m)` | `(map) -> array` | Array of all dictionary values |
| `mapHas(m, key)` | `(map, any) -> bool` | Check key presence |
| `mapGet(m, key, [def])` | `(map, any, any) -> any` | Safe key lookup with default fallback |
| `mapEntries(m)` | `(map) -> array` | Array of `[key, value]` pairs |
| `mapFromEntries(arr)` | `(arr) -> map` | Construct map from key-value pairs |
| `mapMerge(m1, m2)` | `(map, map) -> map` | Merge two maps |
| `mapFilter(m, fn)` | `(map, fn) -> map` | Filter entries by predicate `fun(k, v)` |
| `mapMap(m, fn)` | `(map, fn) -> map` | Transform map entries |

---

## 10. Set, Queue & Stack Containers

### Set Operations
- `setCreate()`: Create new set container
- `setAdd(s, val)`: Add element to set
- `setRemove(s, val)`: Remove element from set
- `setContains(s, val)`: Check membership in set
- `setSize(s)`: Element count
- `setUnion(s1, s2)`: Union $s_1 \cup s_2$
- `setIntersect(s1, s2)`: Intersection $s_1 \cap s_2$
- `setDifference(s1, s2)`: Difference $s_1 \setminus s_2$

### Queue & Stack Operations
- `queuePush(q, val)` / `queuePop(q)`: FIFO enqueue & dequeue
- `queuePopAll(q)`: Drain and return all queued items
- `stackPush(stk, val)` / `stackPop(stk)`: LIFO push & pop

---

## 11. File System & Path Utilities

- `readFile(path)`: Read text file synchronously
- `writeFile(path, content)`: Overwrite file with content
- `appendFile(path, content)`: Append string to file
- `readLines(path)`: Read file into array of line strings
- `writeLines(path, lines)`: Write array of lines to file
- `fileExists(path)` / `dirExists(path)`: Check path existence
- `listDir(path)`: Enumerate files and directories
- `makeDir(path)`: Create directory hierarchy
- `removeFile(path)` / `removeDir(path)`: Delete file or directory recursively
- `copyFile(src, dst)` / `moveFile(src, dst)`: File operations
- `fileSize(path)`: File size in bytes
- `currentDir()` / `changeDir(path)`: Working directory operations
- `addIncludePath(path)`: Add module search path
- `getScriptPath()` / `getScriptDir()`: Path reflection of executing script

---

## 12. Type Conversion & Formatting

- `toInt(val)`: Coerce to 64-bit integer
- `toDouble(val)`: Coerce to 64-bit float
- `toBool(val)`: Coerce to boolean
- `toString(val, [base])`: Convert to string representation (supports radix 2~36)
- `parseInt(str, [base])`: Parse integer string with optional radix
- `parseFloat(str)`: Parse floating-point string
- `toFixed(num, digits)`: Fixed decimal points format
- `toPrecision(num, prec)`: Significant figures format
- `toHex(val)` / `fromHex(s)`: Hexadecimal conversion
- `toBinary(val)` / `fromBinary(s)`: Binary string conversion
- `charCode(s, [idx])`: Character code point
- `fromCharCode(code)`: Code point to string
- `isFinite(x)` / `isNaN(x)`: Float validity tests

---

## 13. JSON Serialization & Utilities

- `jsonParse(str)` / `fromJSON(str)`: Parse JSON string into native object
- `jsonStringify(obj)` / `toJSON(obj)`: Serialize native object to JSON
- `deepCopy(obj)`: Deep-copy object graph without sharing references

---

## 14. Date, Time & Profiling

- `now()`: Monotonic timestamp in milliseconds
- `timestamp()`: Second-level Unix timestamp
- `formatTime(ts, [fmt])`: Format timestamp as readable string
- `dateParts(ts)`: Decompose timestamp into `{year, month, day, hour, minute, second}`
- `dateDiff(t1, t2, unit)`: Compute time difference (`"s"`, `"m"`, `"h"`, `"d"`)
- `measure(fn)`: Measure execution wall-clock time of closure in milliseconds
- `getEnv(key)` / `setEnv(key, val)`: Environment variable manipulation
- `getOsInfo()`: System platform and architecture info
- `assert_t(cond, [msg])`: Assertion check
- `warn(msg)`: Output runtime warning
- `plot(x, y, [title])`: Plot graph window

---

## 15. LibTorch Deep Learning Primitives

300+ native `torch_*` functions:
- **Tensors**: `torch_tensor`, `torch_zeros`, `torch_ones`, `torch_randn`, `torch_empty`, `torch_eye`, `torch_full`
- **Math & Linear Algebra**: `torch_add`, `torch_sub`, `torch_mul`, `torch_matmul`, `torch_inverse`, `torch_svd`, `torch_det`
- **Neural Layers & Activations**: `torch_relu`, `torch_sigmoid`, `torch_tanh`, `torch_gelu`, `torch_linear`, `torch_conv2d`, `torch_max_pool2d`
- **Autograd & Optimization**: `torch_backward`, `torch_cross_entropy`, `torch_mse_loss`, `torch_adam`, `torch_adamw`, `torch_sgd`, `torch_no_grad`
- **GPU Acceleration**: `torch_cuda_is_available`, `torch_to_cuda`, `torch_to_cpu`, `torch_cuda_synchronize`
