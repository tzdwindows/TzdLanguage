# TzdLang 自带内置函数自查大全

<p align="right">
  <a href="Builtin-Functions-Reference.md"><strong>English</strong></a> | <a href="Builtin-Functions-Reference-zh.md"><strong>中文</strong></a>
</p>

欢迎查阅 **TzdLang (TZD)** 自带内置函数全量自查检索手册。TzdLang 原生内置了涵盖核心系统、初等数学、符号求解、数论与大数、矩阵代数、字符串与正则、数组与高阶函数、字典与集合容器、文件系统、JSON 编解码以及 PyTorch 深度学习等 350+ 个原生高性能函数。

---

## 📑 快速目录索引

1. [核心输入输出与系统反射 (Core & System)](#1-核心输入输出与系统反射)
2. [数学计算与初等函数 (Math)](#2-数学计算与初等函数)
3. [数学常量 (Constants)](#3-数学常量)
4. [方程求解与符号微积分 (Symbolic & Equation Solving)](#4-方程求解与符号微积分)
5. [数论、高精度大数与有理数 (BigInt & Number Theory)](#5-数论高精度大数与有理数)
6. [矩阵与线性代数 (Matrix & Linear Algebra)](#6-矩阵与线性代数)
7. [字符串处理与正则表达式 (String & Regex)](#7-字符串处理与正则表达式)
8. [数组操作与高阶函数 (Array & Functional)](#8-数组操作与高阶函数)
9. [字典映射容器 (Map)](#9-字典映射容器)
10. [集合、队列与堆栈 (Set, Queue & Stack)](#10-集合队列与堆栈)
11. [文件系统与路径操作 (File System & Paths)](#11-文件系统与路径操作)
12. [数据类型转换与格式化 (Type Conversion & Formatting)](#12-数据类型转换与格式化)
13. [JSON 序列化与对象工具 (JSON & Utilities)](#13-json-序列化与对象工具)
14. [时间日期与性能度量 (Date, Time & Benchmark)](#14-时间日期与性能度量)
15. [LibTorch 原生深度学习算子 (PyTorch Integration)](#15-libtorch-原生深度学习算子)

---

## 1. 核心输入输出与系统反射

| 函数名 | 函数签名 | 返回类型 | 功能描述与用法示例 |
|---|---|---|---|
| `print` | `print(...args)` | `null` | 打印输出到标准控制台：`print("Hello", 123);` |
| `input` | `input([prompt])` | `string` | 阻塞并从控制台读取一行用户输入：`var name = input("Enter name: ");` |
| `clock` | `clock()` | `double` | 获取高精度系统单调运行时间（毫秒，精确到微秒级）：`var t = clock();` |
| `time` | `time()` | `double` | 获取当前 Unix 时间戳（秒）：`var sec = time();` |
| `sleep` | `sleep(ms)` | `null` | 阻塞挂起当前线程指定毫秒数：`sleep(500);` |
| `exit` | `exit([code])` | `null` | 立即退出当前进程并返回状态码（默认 0）：`exit(0);` |
| `len` | `len(obj)` | `int` | 获取字符串长度或数组元素个数：`len([1, 2, 3]); // 3` |
| `type` | `type(obj)` | `string` | 返回值的数据类型大写字符串（如 `"INT"`, `"FLOAT"`, `"STRING"`, `"ARRAY"`, `"MAP"`, `"CLASS"`, `"INSTANCE"`, `"TENSOR"`, `"NONE"`）：`type(100); // "INT"` |
| `getSymbols` | `getSymbols()` | `map` | 运行时反射获取当前作用域链中注册的所有类与函数列表 |
| `getClassInfo` | `getClassInfo(target)` | `map` | 反射检查类或对象实例的元信息（名称、父类、字段列表与方法列表） |
| `getFunctions` | `getFunctions()` | `array` | 获取当前运行时用户定义的函数名列表 |
| `getNativeFunctions` | `getNativeFunctions()` | `array` | 获取所有已注册的 C++ 原生内置函数名列表 |
| `sys_thread_start`| `sys_thread_start(fn)` | `pointer` | 启动底层操作系统原生工作线程 |
| `sys_thread_join` | `sys_thread_join(th)` | `null` | 阻塞等待指定操作系统工作线程执行结束 |
| `sys_thread_detach`| `sys_thread_detach(th)`| `null` | 将工作线程与主线程分离独立运行 |

---

## 2. 数学计算与初等函数

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `abs(x)` | `(double) -> double` | 计算绝对值：`abs(-5.5); // 5.5` |
| `sqrt(x)` | `(double) -> double` | 计算算术平方根：`sqrt(16.0); // 4.0` |
| `cbrt(x)` | `(double) -> double` | 计算立方根：`cbrt(27.0); // 3.0` |
| `hypot(x, y)` | `(double, double) -> double` | 计算直角三角形斜边长 $\sqrt{x^2+y^2}$ |
| `pow(x, y)` | `(double, double) -> double` | 幂运算 $x^y$：`pow(2, 10); // 1024` |
| `exp(x)` | `(double) -> double` | 自然指数 $e^x$ |
| `expm1(x)` | `(double) -> double` | 精确计算 $e^x - 1$（适合微小浮点数） |
| `log(x)` | `(double) -> double` | 自然对数 $\ln(x)$ |
| `log10(x)` | `(double) -> double` | 常用对数 $\log_{10}(x)$ |
| `log2(x)` | `(double) -> double` | 二进制对数 $\log_2(x)$ |
| `logBase(x, b)`| `(double, double) -> double` | 以 $b$ 为底的对数 $\log_b(x)$ |
| `log1p(x)` | `(double) -> double` | 精确计算 $\ln(1+x)$ |
| `sin(rad)` | `(double) -> double` | 正弦函数（弧度制） |
| `cos(rad)` | `(double) -> double` | 余弦函数（弧度制） |
| `tan(rad)` | `(double) -> double` | 正切函数（弧度制） |
| `asin(x)` | `(double) -> double` | 反正弦函数 $[-\frac{\pi}{2}, \frac{\pi}{2}]$ |
| `acos(x)` | `(double) -> double` | 反余弦函数 $[0, \pi]$ |
| `atan(x)` | `(double) -> double` | 反正切函数 |
| `atan2(y, x)` | `(double, double) -> double` | 四象限反正切角度值 |
| `degrees(rad)` | `(double) -> double` | 弧度转角度：`degrees(PI); // 180` |
| `radians(deg)` | `(double) -> double` | 角度转弧度：`radians(180); // 3.14159265` |
| `round(x)` | `(double) -> double` | 四舍五入到最近整数：`round(3.6); // 4` |
| `floor(x)` | `(double) -> double` | 向下取整：`floor(3.9); // 3` |
| `ceil(x)` | `(double) -> double` | 向上取整：`ceil(3.1); // 4` |
| `clamp(x, min, max)` | `(double, double, double) -> double` | 将数值截断在区间内：`clamp(15, 0, 10); // 10` |
| `lerp(a, b, t)` | `(double, double, double) -> double` | 线性插值 $a + t \cdot (b - a)$ |
| `sign(x)` | `(double) -> double` | 符号函数（正数返 1，负数返 -1，零返 0） |
| `erf(x)` | `(double) -> double` | 高斯误差函数 |
| `tgamma(x)` | `(double) -> double` | 伽马函数 $\Gamma(x)$ |

---

## 3. 数学常量

可以直接作为全局变量或调用获取：

| 常量名称 | 数值 | 描述 |
|---|---|---|
| `PI` | `3.141592653589793` | 圆周率 $\pi$ |
| `E` | `2.718281828459045` | 自然常数 $e$ |
| `TAU` | `6.283185307179586` | $2\pi$ 常数 |
| `SQRT2` | `1.414213562373095` | $\sqrt{2}$ 常数 |
| `GOLDEN_RATIO` | `1.618033988749895` | 黄金分割比 $\phi$ |
| `EPSILON` | `2.220446049250313e-16`| 机器浮点精度下界 |
| `INF` | 浮点正无穷大 ($\infty$) | `INF` |
| `NAN` | 非数值 (Not a Number) | `NAN` |

---

## 4. 方程求解与符号微积分

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `solveEq(f, guess)` | `(function, double) -> double` | 牛顿迭代法求解一元方程根 $f(x) = 0$：<br>`solveEq(fun(x) { return x * x - 2; }, 1.0); // 1.414213` |
| `solveSym(expr, var)`| `(string, string) -> string` | 符号代数解析方程求解：<br>`solveSym("2*x + 5 = 15", "x"); // "x = 5"` |
| `simplifySym(expr)` | `(string) -> string` | 符号表达式化简：<br>`simplifySym("2*x + 3*x"); // "5*x"` |
| `solveIneq(f, op, val, low, high)` | `(fn, str, dbl, dbl, dbl) -> string` | 求解数值不等式解集区间：<br>`solveIneq(fun(x){ return x*x; }, "<", 4, -10, 10); // "-2 < x < 2"` |
| `derivative(f, x)` | `(function, double) -> double` | 高精度数值差分求导 $f'(x)$：<br>`derivative(fun(x) { return x * x; }, 3.0); // 6.0` |

---

## 5. 数论、高精度大数与有理数

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `factorial(n)` | `(int) -> double/int` | 计算阶乘 $n!$：`factorial(5); // 120` |
| `bigint(val)` | `(string/number) -> bigint` | 构造任意精度高精度整数：`var a = bigint("12345678901234567890");` |
| `isBigint(val)` | `(any) -> bool` | 判断变量是否为 BigInt 类型 |
| `bigintFactorial(n)`| `(int) -> bigint` | 计算超大数阶乘（无溢出） |
| `bigintGcd(a, b)` | `(bigint, bigint) -> bigint` | 任意精度大整数最大公约数 |
| `setBigIntMaxDigits(n)` | `(int) -> null` | 设置大整数格式化输出最大展示位限制 |
| `getBigIntMaxDigits()` | `() -> int` | 获取当前大整数最大展示位限制 |
| `gcd(a, b)` | `(int, int) -> int` | 最大公约数：`gcd(48, 18); // 6` |
| `lcm(a, b)` | `(int, int) -> int` | 最小公倍数：`lcm(4, 6); // 12` |
| `isPrime(n)` | `(int) -> bool` | 米勒-拉宾高效素数检测：`isPrime(97); // true` |
| `powmod(b, e, m)` | `(int, int, int) -> int` | 快速模幂运算 $b^e \pmod m$ |
| `comb(n, k)` | `(int, int) -> int` | 组合数 $C_n^k = \frac{n!}{k!(n-k)!}$ |
| `perm(n, k)` | `(int, int) -> int` | 排列数 $A_n^k = \frac{n!}{(n-k)!}$ |
| `fib(n)` | `(int) -> int` | 快速计算第 $n$ 项斐波那契数 |
| `rational(n, d)` | `(int, int) -> rational` | 构造精确有理数分数 $\frac{n}{d}$：`rational(1, 3);` |
| `toFraction(num)` | `(double) -> string` | 将浮点数转换为最简分数形式：`toFraction(0.75); // "3/4"` |
| `rationalAdd(a, b)` | `(rat, rat) -> rat` | 有理数精确加法 |
| `rationalMul(a, b)` | `(rat, rat) -> rat` | 有理数精确乘法 |

---

## 6. 矩阵与线性代数

基于底层 Eigen 高性能 C++ 引擎封装：

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `identity(n)` | `(int) -> array` | 生成 $n \times n$ 单位矩阵：`identity(3);` |
| `zeros(r, [c])` | `(int, int) -> array` | 生成 $r \times c$ 全零矩阵：`zeros(2, 3);` |
| `ones(r, [c])` | `(int, int) -> array` | 生成 $r \times c$ 全一矩阵：`ones(3, 3);` |
| `matrixMul(A, B)` | `(arr, arr) -> array` | 矩阵乘法 $A \times B$ |
| `transpose(A)` | `(arr) -> array` | 矩阵转置 $A^T$ |
| `inverse(A)` | `(arr) -> array` | 矩阵求逆 $A^{-1}$（非方阵报错） |
| `det(A)` | `(arr) -> double` | 计算方阵行列式 $\det(A)$ |
| `trace(A)` | `(arr) -> double` | 计算矩阵的迹（主对角线元素之和） |
| `rank(A)` | `(arr) -> int` | 计算矩阵的代数秩 |
| `solve(A, b)` | `(arr, arr) -> array` | 求解线性方程组 $Ax = b$ |
| `norm(A)` | `(arr) -> double` | 计算矩阵或向量的 Frobenius 范数 |
| `dot(u, v)` | `(arr, arr) -> double` | 计算一维向量点积 |
| `reshape(A, r, c)` | `(arr, int, int) -> array` | 重构二维数组形状 |

---

## 7. 字符串处理与正则表达式

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `split(str, sep)` | `(str, str) -> array` | 按分隔符切分字符串：`split("a,b,c", ","); // ["a", "b", "c"]` |
| `join(arr, sep)` | `(array, str) -> string`| 数组元素拼接为字符串：`join(["a", "b"], "-"); // "a-b"` |
| `replace(s, old, new)` | `(str, str, str) -> string` | 全局子串替换：`replace("hello", "l", "w"); // "hewwo"` |
| `substring(s, start, [len])` | `(str, int, int) -> string` | 截取子字符串 |
| `trim(s)` | `(str) -> string` | 剔除字符串首尾空白字符 |
| `toUpper(s)` | `(str) -> string` | 转换为全大写：`toUpper("abc"); // "ABC"` |
| `toLower(s)` | `(str) -> string` | 转换为全小写：`toLower("ABC"); // "abc"` |
| `toTitleCase(s)` | `(str) -> string` | 首字母大写驼峰格式转换 |
| `toCamelCase(s)` | `(str) -> string` | 转换为小驼峰命名：`toCamelCase("user_id"); // "userId"` |
| `toSnakeCase(s)` | `(str) -> string` | 转换为蛇形下划线命名：`toSnakeCase("userId"); // "user_id"` |
| `contains(s, sub)` | `(str, str) -> bool` | 判断是否包含子串：`contains("hello", "ell"); // true` |
| `startsWith(s, pre)` | `(str, str) -> bool` | 判断是否以指定前缀开头 |
| `endsWith(s, suf)` | `(str, str) -> bool` | 判断是否以指定后缀结尾 |
| `indexOf(s, sub)` | `(str, str) -> int` | 查找子串首个出现下标（未找到返 -1） |
| `charAt(s, idx)` | `(str, int) -> string` | 获取指定下标处的单字符 |
| `reverseStr(s)` | `(str) -> string` | 反转字符串：`reverseStr("abc"); // "cba"` |
| `repeat(s, n)` | `(str, int) -> string` | 重复拼接指定次数：`repeat("=", 5); // "====="` |
| `padLeft(s, len, [pad])` | `(str, int, str) -> string` | 左侧补齐字符：`padLeft("7", 3, "0"); // "007"` |
| `padRight(s, len, [pad])`| `(str, int, str) -> string` | 右侧补齐字符 |
| `format(fmt, ...args)` | `(str, ...) -> string` | 类似 sprintf 的模板格式化 |
| `match(s, regex)` | `(str, str) -> array` | 正则表达式匹配结果捕获组列表 |
| `splitRegex(s, regex)` | `(str, str) -> array` | 按照正则表达式切分字符串 |
| `replaceRegex(s, re, rep)`| `(str, str, str) -> string`| 正则表达式全局替换 |
| `levenshtein(s1, s2)`| `(str, str) -> int` | 计算两字符串之间的莱文斯坦编辑距离 |
| `countSubstr(s, sub)` | `(str, str) -> int` | 统计子串在主串中出现的非重叠次数 |
| `wordCount(s)` | `(str) -> int` | 统计字符串中空格分隔的单词总数 |
| `escape(s)` / `unescape(s)` | `(str) -> string` | URL / 编码字符转义与还原 |
| `base64Encode(s)` / `base64Decode(s)` | `(str) -> string` | Base64 编码与解码 |
| `crc32(s)` / `hash(s)` | `(str) -> int/string` | 计算 CRC32 或哈希摘要值 |
| `uuid()` | `() -> string` | 生成符合标准的 RFC4122 v4 UUID 唯一标识符字符串 |

---

## 8. 数组操作与高阶函数

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `push(arr, val)` | `(arr, any) -> arr` | 末尾压入元素并返回数组本身 |
| `pop(arr)` | `(arr) -> any` | 弹出并返回数组末尾元素 |
| `shift(arr)` | `(arr) -> any` | 移除并返回数组首个元素 |
| `unshift(arr, val)` | `(arr, any) -> arr` | 在数组头部插入元素 |
| `slice(arr, start, [end])` | `(arr, int, int) -> arr` | 切片提取子数组（浅拷贝） |
| `concat(a1, a2)` | `(arr, arr) -> arr` | 拼接两个数组为新数组 |
| `reverse(arr)` | `(arr) -> arr` | 就地反转数组元素顺序 |
| `sort(arr, [cmp])` | `(arr, [fn]) -> arr` | 快速排序（支持自定义双参比较器） |
| `range(start, end, [step])`| `(int, int, int) -> arr` | 生成数值递增序列数组：`range(0, 5); // [0, 1, 2, 3, 4]` |
| `map(arr, fn)` | `(arr, fn) -> arr` | 高阶映射函数：`map([1, 2], fun(x){ return x*2; }); // [2, 4]` |
| `filter(arr, fn)` | `(arr, fn) -> arr` | 高阶过滤函数：`filter([1, 2, 3], fun(x){ return x % 2 == 1; });` |
| `reduce(arr, fn, [init])` | `(arr, fn, any) -> any` | 累加规约函数：`reduce([1, 2, 3, 4], fun(acc, x){ return acc + x; }, 0);` |
| `find(arr, fn)` | `(arr, fn) -> any` | 查找符合条件的第一个元素（无则返回 `null`） |
| `includes(arr, val)` | `(arr, any) -> bool` | 判断数组是否包含特定元素 |
| `indexOfArr(arr, val)` | `(arr, any) -> int` | 返回特定元素首次出现的下标索引 |
| `fill(arr, val)` | `(arr, any) -> arr` | 将整个数组填充为给定值 |
| `flatten(arr)` | `(arr) -> arr` | 将多维嵌套数组拍平成一维数组 |
| `zip(a1, a2)` | `(arr, arr) -> arr` | 将两数组对应位置打包为二维元组对列表 |
| `unique(arr)` | `(arr) -> arr` | 数组去重并保留唯一样本 |
| `sum(arr)` | `(arr) -> double` | 快速计算数值数组总和：`sum([1, 2, 3]); // 6` |
| `avg(arr)` | `(arr) -> double` | 快速计算数值数组算术平均值 |
| `minArr(arr)` / `maxArr(arr)`| `(arr) -> double` | 获取数组中的最小值 / 最大值 |
| `argmax(arr)` / `argmin(arr)`| `(arr) -> int` | 获取数组中最大值 / 最小值所在的下标 |
| `cumsum(arr)` | `(arr) -> arr` | 计算累积前缀和数组 |
| `diff(arr)` | `(arr) -> arr` | 计算相邻元素离散一阶差分数组 |
| `shuffle(arr)` | `(arr) -> arr` | 随机打乱数组元素排列 |
| `sample(arr, count)` | `(arr, int) -> arr` | 从数组中无放回随机抽样指定数量元素 |
| `linspace_arr(start, stop, n)`| `(num, num, int) -> arr`| 生成等间距线性插值数值数组 |

---

## 9. 字典映射容器 (Map)

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `mapKeys(m)` | `(map) -> array` | 获取 Map 中所有的键（Key）列表 |
| `mapValues(m)` | `(map) -> array` | 获取 Map 中所有的值（Value）列表 |
| `mapHas(m, key)` | `(map, any) -> bool` | 判断 Map 是否包含指定键 |
| `mapGet(m, key, [def])` | `(map, any, any) -> any` | 安全获取指定键的值，不存在时返回默认值 |
| `mapEntries(m)` | `(map) -> array` | 将 Map 转换为 `[[k1, v1], [k2, v2]]` 键值对数组 |
| `mapFromEntries(arr)` | `(array) -> map` | 将键值对元组数组恢复重构为 Map |
| `mapMerge(m1, m2)` | `(map, map) -> map` | 合并两个 Map，同名键覆盖合并 |
| `mapFilter(m, fn)` | `(map, fn) -> map` | 按照 `fun(k, v)` 谓词过滤字典键值对 |
| `mapMap(m, fn)` | `(map, fn) -> map` | 按照 `fun(k, v)` 变换生成新的映射值 |

---

## 10. 集合、队列与堆栈

### 集合 (Set)
| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `setCreate()` | `() -> map` | 创建一个空集合实例 |
| `setAdd(s, val)` | `(set, any) -> set` | 向集合中加入元素 |
| `setRemove(s, val)` | `(set, any) -> set` | 从集合中移除元素 |
| `setContains(s, val)` | `(set, any) -> bool`| 判断集合中是否存在某元素 |
| `setSize(s)` | `(set) -> int` | 获取集合中元素总数 |
| `setUnion(s1, s2)` | `(set, set) -> set` | 计算两集合的并集 $s_1 \cup s_2$ |
| `setIntersect(s1, s2)` | `(set, set) -> set` | 计算两集合的交集 $s_1 \cap s_2$ |
| `setDifference(s1, s2)`| `(set, set) -> set` | 计算两集合的差集 $s_1 \setminus s_2$ |

### 队列与堆栈 (Queue & Stack)
| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `queuePush(q, val)` | `(arr, any) -> arr` | 入队（末尾添加） |
| `queuePop(q)` | `(arr) -> any` | 出队（头部弹出先进先出 FIFO） |
| `queuePopAll(q)` | `(arr) -> arr` | 清空并返回队列中当前所有元素 |
| `stackPush(stk, val)`| `(arr, any) -> arr` | 压栈（末尾添加） |
| `stackPop(stk)` | `(arr) -> any` | 弹栈（末尾弹出后进先出 LIFO） |

---

## 11. 文件系统与路径操作

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `readFile(path)` | `(str) -> string` | 同步读取整个文本文件内容 |
| `writeFile(path, content)` | `(str, str) -> bool` | 覆盖写入文本内容至文件 |
| `appendFile(path, content)`| `(str, str) -> bool` | 在文本文件末尾追加写入内容 |
| `readLines(path)` | `(str) -> array` | 读取文本文件每一行存入字符串数组 |
| `writeLines(path, lines)` | `(str, arr) -> bool` | 将字符串数组按行写入文件 |
| `fileExists(path)` | `(str) -> bool` | 检测目标路径文件是否存在 |
| `dirExists(path)` | `(str) -> bool` | 检测目标路径目录是否存在 |
| `listDir(path)` | `(str) -> array` | 枚举目录下的所有子文件与子文件夹列表 |
| `makeDir(path)` | `(str) -> bool` | 创建多级文件夹目录 |
| `removeFile(path)` | `(str) -> bool` | 删除指定文件 |
| `removeDir(path)` | `(str) -> bool` | 递归删除整个文件夹及其中内容 |
| `copyFile(src, dst)` | `(str, str) -> bool` | 复制文件 |
| `moveFile(src, dst)` | `(str, str) -> bool` | 移动或重命名文件 |
| `fileSize(path)` | `(str) -> int` | 获取文件字节大小（Byte） |
| `currentDir()` | `() -> string` | 获取当前进程工作目录 |
| `changeDir(path)` | `(str) -> bool` | 切换当前工作目录 |
| `addIncludePath(path)` | `(str) -> null` | 添加模块脚本导入 (`import`) 搜索目录 |
| `getScriptPath()` | `() -> string` | 获取当前主脚本的绝对文件路径 |
| `getScriptDir()` | `() -> string` | 获取当前主脚本所在的目录路径 |

---

## 12. 数据类型转换与格式化

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `toInt(x)` | `(any) -> int` | 转换为 64 位整数：`toInt("123"); // 123` |
| `toDouble(x)` | `(any) -> double` | 转换为 64 位双精度浮点数：`toDouble("3.14");` |
| `toBool(x)` | `(any) -> bool` | 转换为布尔值（0、空串、null 判为 false） |
| `toString(x, [base])` | `(any, [int]) -> string` | 转换为文本字符串，支持基数转换（2~36 进制） |
| `parseInt(str, [base])` | `(str, int) -> int` | 按照指定进制解析整数字符串 |
| `parseFloat(str)` | `(str) -> double` | 解析浮点数字符串 |
| `toFixed(num, digits)` | `(double, int) -> string` | 保留指定小数位数格式化字符串：`toFixed(3.14159, 2); // "3.14"` |
| `toPrecision(num, prec)` | `(double, int) -> string` | 格式化为有效数字位数字符串 |
| `toHex(val)` / `fromHex(s)` | 进制转换 | 整数与十六进制字符串相互转换 |
| `toBinary(val)` / `fromBinary(s)` | 进制转换 | 整数与二进制字符串相互转换 |
| `charCode(s, [idx])` | `(str, int) -> int` | 获取指定字符的 ASCII / Unicode 编码值 |
| `fromCharCode(code)` | `(int) -> string` | 将整数编码值转换为单字符字符串 |
| `isFinite(x)` / `isNaN(x)` | `(double) -> bool` | 浮点数有限性与非数值检测 |

---

## 13. JSON 序列化与对象工具

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `jsonParse(str)` | `(string) -> any` | 将符合 JSON 规范的字符串解析为原生 Array/Map/基本类型 |
| `jsonStringify(obj)` | `(any) -> string` | 将原生数据结构递归序列化为标准 JSON 字符串 |
| `toJSON(obj)` | `(any) -> string` | `jsonStringify` 的快捷别名 |
| `fromJSON(str)` | `(string) -> any` | `jsonParse` 的快捷别名 |
| `deepCopy(obj)` | `(any) -> any` | 深拷贝对象或复杂嵌套容器，完全断开内存引用关联 |

---

## 14. 时间日期与性能度量

| 函数名 | 签名 | 说明与示例 |
|---|---|---|
| `now()` | `() -> double` | 获取高精度单调毫秒时间戳 |
| `timestamp()` | `() -> int` | 获取系统秒级 Unix 时间戳 |
| `formatTime(ts, [fmt])` | `(int, str) -> string` | 格式化时间戳输出日期字符串（如 `"%Y-%m-%d %H:%M:%S"`） |
| `dateParts(ts)` | `(int) -> map` | 解析时间戳为包含 `year`, `month`, `day`, `hour`, `minute`, `second` 的 Map |
| `dateDiff(t1, t2, unit)` | `(int, int, str) -> int`| 计算两时间戳之间的差值（单位可选 `"s"`, `"m"`, `"h"`, `"d"`） |
| `measure(fn)` | `(function) -> double` | 执行闭包函数并返回其实际运行耗时（单位：毫秒） |
| `getEnv(name)` / `setEnv(k, v)` | 环境变量访问 | 获取或设置当前进程的环境变量 |
| `getOsInfo()` | `() -> map` | 返回包含操作系统类型与架构信息的 Map |
| `assert_t(cond, [msg])` | `(bool, str) -> null` | 断言测试条件，若为 false 则抛出异常 |
| `warn(msg)` | `(string) -> null` | 输出格式化警告信息至控制台 |
| `plot(x, y, [title])` | `(arr, arr, str) -> null` | 在独立绘图窗口渲染数据散点/折线走势图 |

---

## 15. LibTorch 原生深度学习算子

TzdLang 原生封装了 300+ 个底层 `torch_*` 原生算子，支持在 CPU 与 NVIDIA CUDA GPU 之间零开销调用：

### 15.1 张量创建与生成
- `torch_tensor(data, [requires_grad])`：从原生数组构造一维或多维张量
- `torch_zeros(shape)` / `torch_ones(shape)` / `torch_empty(shape)`：构造全零、全一或空未初始化张量
- `torch_rand(shape)` / `torch_randn(shape)`：均匀分布或标准正态分布随机张量
- `torch_randint(low, high, shape)`：随机离散整数采样张量
- `torch_arange(start, end, [step])` / `torch_linspace(start, end, steps)`：数值等差生成
- `torch_eye(n)`：生成 $n \times n$ 单位对角张量
- `torch_full(fill_val, shape)`：生成固定标量填充的张量

### 15.2 数学运算与矩阵代数
- `torch_add(a, b)` / `torch_sub(a, b)` / `torch_mul(a, b)` / `torch_div(a, b)`：四则逐元素算子
- `torch_matmul(a, b)` / `torch_mm(a, b)` / `torch_bmm(a, b)`：二维与批量三维矩阵相乘
- `torch_pow(a, exp)` / `torch_sqrt(a)` / `torch_exp(a)` / `torch_log(a)`：初等逐元素数学算子
- `torch_inverse(a)` / `torch_det(a)` / `torch_cholesky(a)` / `torch_svd(a)`：线性代数与矩阵分解
- `torch_sum(a, [dim])` / `torch_mean(a, [dim])` / `torch_std(a)` / `torch_var(a)`：维度统计规约

### 15.3 深度学习激活函数与层
- `torch_relu(x)` / `torch_sigmoid(x)` / `torch_tanh(x)` / `torch_gelu(x)` / `torch_silu(x)`：非线性激活函数
- `torch_softmax(x, dim)` / `torch_log_softmax(x, dim)`：概率归一化
- `torch_linear(x, weight, [bias])`：全连接前向计算
- `torch_conv1d` / `torch_conv2d` / `torch_conv_transpose2d`：一维与二维卷积运算
- `torch_max_pool2d` / `torch_avg_pool2d` / `torch_adaptive_avg_pool2d`：池化算子
- `torch_layer_norm` / `torch_batch_norm`：张量归一化

### 15.4 自动微分、损失函数与优化器
- `torch_backward(tensor)`：触发反向传播自动微分求导
- `torch_cross_entropy(logits, targets)`：多分类交叉熵损失
- `torch_mse_loss(preds, targets)`：均方误差损失（MSE Loss）
- `torch_bce_loss(preds, targets)`：二分类交叉熵损失
- `torch_adam` / `torch_adamw` / `torch_sgd` / `torch_rmsprop`：内置神经网络参数优化器
- `torch_no_grad(fn)`：在关闭梯度追踪的作用域中高速推理

### 15.5 硬件加速与设备管理
- `torch_cuda_is_available()`：检测系统是否存在可用的 CUDA GPU 显卡
- `torch_device_count()`：获取系统中 GPU 显卡总数
- `torch_to_cuda(tensor)` / `torch_to_cpu(tensor)`：在 CPU 内存与 GPU 显存之间迁移数据
- `torch_cuda_memory_allocated()` / `torch_cuda_synchronize()`：显存分配查询与流同步
