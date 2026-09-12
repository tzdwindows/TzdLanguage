# JIT 编译器架构与底层优化设计

<p align="right">
  <a href="JIT-Compiler-Internals.md"><strong>English</strong></a> | <a href="JIT-Compiler-Internals-zh.md"><strong>中文</strong></a>
</p>

TzdLang 搭载了工业级分级编译（Tiered Compilation）混合执行引擎，兼具解释器的毫秒级即时启动特性与优化原生编译器的峰值运行吞吐能力。

---

## 1. 双层（Dual-Tier）执行架构全景

```mermaid
graph TD
    AST["ANTLR4 语法树 (AST)"] --> Tier0["Tier 0: 字节码虚拟机 (Bytecode VM)"]
    Tier0 --> RunVM["即时执行与热点分支分析 (Profiling)"]
    RunVM -- 达到热点阈值 (Hot Threshold) --> Tiering["分级调度引擎 (Tiering Engine)"]
    Tiering --> LLVMCodeGen["TzdCompiler (AST 转换为 LLVM IR)"]
    LLVMCodeGen --> SpecializedWorkers["生成原生特化 Worker 与调用存根 (Stubs)"]
    SpecializedWorkers --> Pipeline["LLVM 多阶段优化流水线 (Pass Pipeline)"]
    Pipeline --> ObjectCache["原生机器指令缓存 (.obj)"]
    ObjectCache --> LLJIT["LLVM ORC JIT 实时编译执行器"]
    LLJIT --> NativeExec["CPU 原生机器码直接高速执行"]
```

### Tier 0：紧凑型堆栈式字节码虚拟机
- 专为极致冷启动零延迟设计，免去前端编译停顿。
- 在解释执行期间低开销采集函数调用频率、循环回边计数等热点性能画像（Profile Data）。
- 负责执行动态脚本、顶层初始化以及低频冷路径。

### Tier 1：异步 LLVM ORC JIT 编译引擎
- 针对达到执行热点阈值的函数，触发异步编译管道将其编译为 x86_64 优化原生机器码。
- 引入 **三版本函数生成策略（Triple-Version Generation）**：
  1. **Entry 存根函数**：`void entry(void* interp, void* retVal)` —— C++ 运行时与虚拟机统一调用的标准入口。
  2. **Worker 装箱通用函数**：`double worker(void* interp, void* argArray, void* retVal)` —— 接收动态变长参数数组的通用通道。
  3. **Native Worker 原生无装箱函数**：`double worker_native(void* interp, double a, double b, ...)` —— 采用纯寄存器传参、直接操作原生基本类型的特化函数。

---

## 2. JIT 核心优化流水线（Phases A – F）

### Phase A: 运行时零开销内联（GEP + Store 内存直写）
彻底消除返回值存储过程中的运行时封装调用（`rt_store_native_to_ptr`）。编译器通过在编译期计算 `offsetof(TzdValue, type)`，直接发射 LLVM `GetElementPtr` 与原生 `Store` 指令写回结构体内存。

### Phase B: 函数特化与原生寄存器 Worker
自递归（Self-recursive）与已知调用图的跨过程调用彻底绕开动态参数数组分配：
- 直接生成免装箱（Unboxed）`double` 函数签名。
- 将堆/栈上的 `TzdValue` 装箱开销替换为纯 CPU 寄存器移动指令。
- 递归斐波那契或紧密数值计算循环完全在寄存器组中高速运转。

### Phase C: 深度 LLVM 内联与 SSA 寄存器提升
- 所有原生 Worker 函数默认标注 `AlwaysInline` 属性。
- 模块级 `AlwaysInlinerPass` 展开核心调用图展开。
- `mem2reg`（`PromotePass`）将局部变量从栈内存（`alloca`）彻底提升为纯 SSA 虚拟寄存器。
- `EarlyCSEPass` 与 `DCEPass` 消除公共子表达式与死代码分支。

### Phase D: 直接分发与硬件取模特化
- 函数名在编译期直接由原生注册表 `s_compiledNativeWorkers` 解析，规避哈希表运行时查找。
- 针对浮点取模 `%` 算子进行特化：若操作数能够安全转为整型，LLVM 发射 `FPToSI` + `SRem`（对应硬件原生 `idiv` 指令），避开沉重的 C 运行时 `fmod()` 库函数。

### Phase E: 快路径浮点等值比较
浮点比较（`==` 与 `!=`）直接降级（Lower）为硬件级的 `CreateFCmpOEQ` / `CreateFCmpONE` 指令，完全剔除装箱与动态类型分支开销。

### Phase F: 对象成员 LICM 与只读纯函数标记（ReadOnly Optimization）
对象字段读取（`rt_tzd_get_member`）与数值快速解包（`rt_to_double_fast`）被严谨赋予 LLVM `ReadOnly` 属性。LLVM 循环不变量外提（LICM）与公共子表达式消除（CSE）能够安全地将循环内部重复的成员寻址指令整体提升至循环外部。

---

## 3. 与 Oracle JDK 20 HotSpot C2 性能对比

测试环境：Windows 11 x64 (AMD Ryzen / Intel Core 平台):

| 测试用例 / 运行场景 | TzdLang (LLVM JIT) | JDK 20 (HotSpot C2) | 性能对比倍率 |
|---|---|---|---|
| 函数调用开销 `callOverhead(1M)` | **0.002 s** | 0.005 s | **TzdLang 快 2.5 倍** |
| 双重嵌套循环 `nestedLoop(1k × 1k)` | **0.003 s** | 0.005 s | **TzdLang 快 1.7 倍** |
| 累加循环 `sumLoop(1M)` | **0.002 s** | 0.003 s | **TzdLang 快 1.5 倍** |
| 阿克曼函数 `Ackermann(3, 6)` | **0.001 s** | 0.001 s | **TzdLang 快 1.2 倍** |
| 牛顿迭代开方 `sqrt(100k)` | **0.000006 s** | 0.000008 s | **TzdLang 快 1.3 倍** |
| 字段循环读取 `field(100k)` | 0.002 s | 0.0005 s | HotSpot C2 更优 |
| 递归斐波那契 `fib(35)` | 0.197 s | 0.061 s | HotSpot C2 更优 |
