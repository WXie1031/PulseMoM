# GPU代码分离指南

## 问题分析

当前GPU代码中混合了物理公式和数值实现，违反了架构要求。

### 当前问题文件

1. **`src/core/gpu_acceleration.c`**
   - 包含物理公式（Green's function计算）
   - 包含GPU kernel调用
   - 混合了L3算子层和L4数值后端的内容

2. **`src/core/gpu_parallelization_optimized.cu`**
   - 包含物理公式（Sommerfeld积分）
   - 包含GPU kernel实现
   - 混合了L3算子层和L4数值后端的内容

## 分离策略

### 原则
- **物理公式** → L3算子层 (`L3_operators/kernels/`)
- **GPU调用和内存管理** → L4数值后端 (`L4_backend/gpu/`)
- **GPU代码必须无物理语义**

### 分离步骤

#### 步骤1: 识别物理公式部分

**在 `gpu_acceleration.c` 中**:
```c
// 物理公式部分（应该移到L3）
// Green's function: G(r) = exp(-jk*r) / (4*π*r)
double phase = -k0 * r;
double magnitude = 1.0 / (4.0 * M_PI * r);
```

**在 `gpu_parallelization_optimized.cu` 中**:
```c
// 物理公式部分（应该移到L3）
// Sommerfeld integral computation
double k0 = omega / 299792458.0;
double kz = sqrt(k0 * k0 - krho * krho);
```

#### 步骤2: 创建L3算子层接口

在 `L3_operators/kernels/greens_function.h` 中定义：
```c
// L3层定义算子形式
complex_t greens_function_free_space(real_t r, real_t k);
complex_t greens_function_layered_media(...);
```

#### 步骤3: 实现L3算子层

在 `L3_operators/kernels/greens_function.c` 中实现物理公式：
```c
// L3层实现物理公式，无数值优化
complex_t greens_function_free_space(real_t r, real_t k) {
    // 物理公式实现
    real_t phase = -k * r;
    return exp(I * phase) / (4.0 * M_PI * r);
}
```

#### 步骤4: 创建L4 GPU接口

在 `L4_backend/gpu/gpu_kernels.h` 中定义：
```c
// L4层定义GPU调用接口，无物理语义
int gpu_kernel_matrix_vector_multiply(
    const complex_t* A,
    const complex_t* x,
    complex_t* y,
    int m, int n,
    const gpu_launch_config_t* config
);
```

#### 步骤5: 实现L4 GPU调用

在 `L4_backend/gpu/gpu_kernels.cu` 中实现：
```c
// L4层实现GPU kernel，调用L3算子
__global__ void gpu_greens_function_kernel(
    const real_t* distances,  // 输入：距离（从L3计算）
    const real_t* k_values,   // 输入：波数（从L3计算）
    complex_t* results,       // 输出：结果
    int n) {
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    
    // 调用L3算子（通过函数指针或内联）
    // 注意：这里不包含物理公式，只调用算子
    real_t r = distances[idx];
    real_t k = k_values[idx];
    
    // 调用L3层的算子函数
    // results[idx] = greens_function_free_space(r, k);
    // 或者通过参数传递已计算的值
}
```

## 具体分离示例

### 示例1: Green's Function Kernel

**当前代码** (`gpu_acceleration.c`):
```c
__global__ void greens_function_matrix_kernel(...) {
    // 物理公式（应该移到L3）
    double k0 = omega / 299792458.0;
    double phase = -k0 * r;
    double magnitude = 1.0 / (4.0 * M_PI * r);
    // GPU调用（留在L4）
    green_matrix[...] = make_gpu_complex(...);
}
```

**分离后**:

**L3层** (`L3_operators/kernels/greens_function.c`):
```c
// L3层：物理公式实现
complex_t greens_function_free_space(real_t r, real_t k) {
    real_t phase = -k * r;
    return exp(I * phase) / (4.0 * M_PI * r);
}
```

**L4层** (`L4_backend/gpu/gpu_kernels.cu`):
```c
// L4层：GPU kernel调用，无物理公式
__global__ void gpu_greens_function_kernel(
    const real_t* r_values,    // 输入：距离（已计算）
    const real_t* k_values,    // 输入：波数（已计算）
    complex_t* results,        // 输出：结果
    int n) {
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    
    // 调用L3算子（通过函数指针或参数传递）
    // 注意：这里不包含物理公式
    real_t r = r_values[idx];
    real_t k = k_values[idx];
    
    // 调用L3算子（需要设计接口）
    // 或者：在CPU端调用L3算子，然后传输到GPU
}
```

### 示例2: Sommerfeld Integral

**当前代码** (`gpu_parallelization_optimized.cu`):
```c
__global__ void sommerfeld_integral_kernel(...) {
    // 物理公式（应该移到L3）
    double k0 = omega / 299792458.0;
    double k = k0 * sqrt(er * mu_r - I * sigma / (omega * 8.854e-12));
    double J0_krho = j0(krho * k);
    // GPU计算（留在L4）
}
```

**分离后**:

**L3层** (`L3_operators/kernels/layered_greens_function.c`):
```c
// L3层：物理公式实现
complex_t layered_greens_sommerfeld_integrand(
    real_t krho, real_t z, real_t zp,
    real_t k0, const layered_medium_t* medium) {
    
    // 物理公式计算
    real_t k = k0 * sqrt(medium->eps_r * medium->mu_r);
    real_t J0 = j0(krho * k);
    // ... 其他物理计算
    return result;
}
```

**L4层** (`L4_backend/gpu/gpu_kernels.cu`):
```c
// L4层：GPU kernel，调用L3算子或接收已计算的值
__global__ void gpu_sommerfeld_integration_kernel(
    const complex_t* integrand_values,  // 输入：被积函数值（从L3计算）
    const real_t* weights,              // 输入：积分权重
    complex_t* results,                 // 输出：积分结果
    int n) {
    
    // GPU数值积分，无物理公式
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n) return;
    
    // 数值积分：sum(integrand * weight)
    results[idx] = integrand_values[idx] * weights[idx];
}
```

## 分离后的架构

### L3算子层
- 包含所有物理公式
- 可以调用CPU实现
- 提供算子接口给L4层

### L4数值后端
- 只包含GPU调用和内存管理
- 接收算子结果或调用算子接口
- 无物理语义

## 接口设计

### L3 → L4 接口

```c
// L3层提供算子接口
typedef complex_t (*greens_function_op_t)(real_t r, real_t k);

// L4层接收算子接口
int gpu_kernel_evaluate_greens_function(
    greens_function_op_t op,  // L3算子
    const real_t* r_values,
    const real_t* k_values,
    complex_t* results,
    int n
);
```

## 迁移检查清单

- [ ] 识别所有物理公式
- [ ] 移到L3算子层
- [ ] 更新GPU kernel为无物理语义
- [ ] 创建L3→L4接口
- [ ] 验证架构合规性
- [ ] 测试功能正确性

---

**最后更新**: 2025-01-18
