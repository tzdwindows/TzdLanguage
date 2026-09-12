# 深度学习与 LibTorch 原生集成

<p align="right">
  <a href="Deep-Learning-and-PyTorch.md"><strong>English</strong></a> | <a href="Deep-Learning-and-PyTorch-zh.md"><strong>中文</strong></a>
</p>

TzdLang 原生深度集成了 PyTorch 官方底层 C++ 库（**LibTorch**），使开发者能够在 TzdLang 内部直接构建、训练、评估并部署现代深度学习神经网络，彻底摆脱 Python 解释器的 GIL 锁与高昂运行时调度开销。

---

## 1. 零锁内存生命周期管理架构（Zero-Lock Memory Architecture）

在动态类型语言解释器中嵌入 LibTorch 时，传统方案通常依赖全局张量注册表或类似 GIL 的互斥锁，多线程高频创建或销毁张量极易导致严重的锁争用（Lock Contention）。

TzdLang 从底层彻底攻克了这一性能瓶颈：
- **无锁侵入式指针桥接（Lock-Free Intrusive Pointer）**：将核心值对象 `TzdValue` 直接与 `at::TensorImpl` 的原子引用计数通过 `c10::intrusive_ptr` 绑定。
- **C 语言链接钩子**：`tzdTensorRetain()` 与 `tzdTensorRelease()` 直接内联织入 `TzdValue` 的拷贝构造函数、移动语义以及析构函数。
- **显存与内存零泄漏保证**：一旦包含张量的 `TzdValue` 超出局部作用域被析构，LibTorch 内部原子引用计数立刻减一，无垃圾回收停顿地释放 CPU 与 GPU 显存资源。

---

## 2. 原生张量运算与多维张量

### 2.1 张量创建
```tzd
import "stdlib/torch/nn.tzd";

// 常见多维张量创建初始化
var zeros = torch_zeros([2, 4]);
var ones  = torch_ones([3, 3]);
var eye   = torch_eye(4);
var rand  = torch_randn([10, 10]);

// 将 Tzd 原生动态数组转换为张量
var t = torch_tensor([1.0, 2.0, 3.0, 4.0]);
```

### 2.2 线性代数与逐元素算子
```tzd
var a = torch_randn([4, 8]);
var b = torch_randn([8, 2]);

// 矩阵乘法 (GEMM)
var c = torch_matmul(a, b); // 输出张量形状: [4, 2]

// 逐元素运算与非线性激活函数
var d = a + 2.5;
var e = torch_relu(d);
var s = torch_sigmoid(e);
```

### 2.3 GPU 硬件加速
```tzd
if (torch_cuda_is_available()) {
    var gpu_tensor = a.cuda();
    print("张量已成功载入 GPU 显存: " + toString(gpu_tensor.device));
}
```

---

## 3. 自动微分引擎（Autograd）

TzdLang 原生无缝打通了反向传播计算图追踪：

```tzd
// 开启梯度追踪 (requires_grad = true)
var x = torch_tensor([2.0, 3.0], true);
var y = x * x + 3.0 * x + 1.0;

// 执行反向传播自动求导
y.backward();

// 检视求导结果 (dy/dx = 2*x + 3 = [7.0, 9.0])
print("计算梯度: " + toString(x.grad()));
```

---

## 4. 神经网络模块与优化器全流程

```tzd
import "stdlib/torch/nn.tzd";

// 声明多层感知机 (MLP)
class SimpleClassifier {
    var fc1;
    var fc2;

    SimpleClassifier(in_features, hidden_dim, num_classes) {
        this.fc1 = new nn_Linear(in_features, hidden_dim);
        this.fc2 = new nn_Linear(hidden_dim, num_classes);
    }

    fun forward(x) {
        var h = torch_relu(this.fc1.forward(x));
        return this.fc2.forward(h);
    }
}

// 实例化模型与优化器
var model = new SimpleClassifier(784, 128, 10);
var optimizer = new optim_Adam(model.parameters(), 0.001);

// 单步训练循环
optimizer.zero_grad();
var input_batch = torch_randn([32, 784]);
var labels = torch_randint(0, 10, [32]);

var logits = model.forward(input_batch);
var loss = torch_cross_entropy_loss(logits, labels);

loss.backward();
optimizer.step();

print("当前迭代损失 Loss: " + toString(loss.item()));
```
