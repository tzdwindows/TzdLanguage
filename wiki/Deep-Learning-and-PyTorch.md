# Deep Learning & LibTorch Native Integration

<p align="right">
  <a href="Deep-Learning-and-PyTorch.md"><strong>English</strong></a> | <a href="Deep-Learning-and-PyTorch-zh.md"><strong>中文</strong></a>
</p>

TzdLang provides native, first-class integration with PyTorch's C++ library (**LibTorch**), allowing developers to build, train, and deploy deep learning models directly within TzdLang without Python runtime overhead.

---

## 1. Zero-Lock Memory Management Architecture

Historically, embedding LibTorch into dynamically typed language interpreters suffered from severe lock contention due to global tensor registration tables and GIL-like synchronization mutexes.

TzdLang eliminates this bottleneck entirely:
- **Lock-Free Intrusive Pointer Lifecycle**: Directly bridges `TzdValue` to `at::TensorImpl`'s atomic reference count via `c10::intrusive_ptr`.
- **C-Linkage Hooks**: `tzdTensorRetain()` and `tzdTensorRelease()` are directly integrated into `TzdValue` copy constructors, assignments, and destructors.
- **Zero VRAM Leak Guarantee**: Destruction of a `TzdValue` containing a tensor automatically decrements the internal LibTorch reference count atomically, immediately reclaiming CPU and GPU memory.

---

## 2. Tensor Operations

### 2.1 Tensor Creation
```tzd
import "stdlib/torch/nn.tzd";

// Create tensors of various shapes and initializations
var zeros = torch_zeros([2, 4]);
var ones  = torch_ones([3, 3]);
var eye   = torch_eye(4);
var rand  = torch_randn([10, 10]);

// Convert native arrays into Tensors
var t = torch_tensor([1.0, 2.0, 3.0, 4.0]);
```

### 2.2 Arithmetic & Linear Algebra
```tzd
var a = torch_randn([4, 8]);
var b = torch_randn([8, 2]);

// Matrix multiplication
var c = torch_matmul(a, b); // Result shape: [4, 2]

// Element-wise operations
var d = a + 2.5;
var e = torch_relu(d);
var s = torch_sigmoid(e);
```

### 2.3 GPU Acceleration
```tzd
if (torch_cuda_is_available()) {
    var gpu_tensor = a.cuda();
    print("Tensor successfully allocated on GPU: " + toString(gpu_tensor.device));
}
```

---

## 3. Automatic Differentiation (Autograd)

TzdLang seamlessly supports reverse-mode automatic differentiation:

```tzd
// Enable gradient tracking
var x = torch_tensor([2.0, 3.0], true); // requires_grad = true
var y = x * x + 3.0 * x + 1.0;

// Compute gradients
y.backward();

// Inspect gradients (dy/dx = 2*x + 3 = [7.0, 9.0])
print("Gradients: " + toString(x.grad()));
```

---

## 4. Neural Network Modules & Optimizers

```tzd
import "stdlib/torch/nn.tzd";

// Define a Multi-Layer Perceptron (MLP)
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

// Instantiate model and optimizer
var model = new SimpleClassifier(784, 128, 10);
var optimizer = new optim_Adam(model.parameters(), 0.001);

// Training iteration step
optimizer.zero_grad();
var input_batch = torch_randn([32, 784]);
var labels = torch_randint(0, 10, [32]);

var logits = model.forward(input_batch);
var loss = torch_cross_entropy_loss(logits, labels);

loss.backward();
optimizer.step();

print("Iteration Loss: " + toString(loss.item()));
```
