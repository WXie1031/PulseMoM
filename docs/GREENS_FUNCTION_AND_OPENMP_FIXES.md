# Green's Function 和 OpenMP 修复总结

## 1. Green's Function 支持情况

### 支持的Green's Function类型

#### 1.1 Free-Space Green's Function ✅
- **函数**: `greens_function_free_space(r, k)`
- **算子**: G(r) = exp(-jk*r) / (4*π*r)
- **梯度**: `greens_function_gradient_free_space(r, k, r_vec, gradient)`
- **算子**: ∇G(r) = -jk * (r_vec/r) * G(r)
- **状态**: ✅ 完全实现

#### 1.2 Layered Media Green's Function ⚠️
- **函数**: `greens_function_layered_media(rho, z, z_prime, k0, n_layers, layers)`
- **算子**: 使用Sommerfeld积分表示
- **状态**: ✅ 接口已定义，⚠️ 简化实现（使用自由空间近似）
- **注意**: 完整数值评估（Sommerfeld/DCIM）属于L4后端

#### 1.3 Periodic Green's Function ⚠️
- **函数**: `greens_function_periodic(r, k, periodicity, n_harmonics)`
- **算子**: 使用Floquet谐波进行周期扩展
- **状态**: ✅ 接口已定义，⚠️ 简化实现（使用自由空间近似）
- **注意**: 完整数值评估属于L4后端

### Green's Function Kernel类型

定义了4种kernel类型用于奇异积分：

```c
typedef enum {
    KERNEL_G = 1,                    // Green's function G
    KERNEL_GRAD_G = 2,              // Gradient of Green's function ∇G
    KERNEL_G_R_R_PRIME = 3,         // G/r² for specific integrals
    KERNEL_DOUBLE_GRAD_G = 4        // Double gradient ∇∇G
} greens_kernel_type_t;
```

### 实现状态总结

| Green's Function类型 | 接口 | 实现 | 说明 |
|---------------------|------|------|------|
| Free-space | ✅ | ✅ 完整 | 完全实现 |
| Free-space gradient | ✅ | ✅ 完整 | 完全实现 |
| Layered media | ✅ | ⚠️ 简化 | 使用自由空间近似 |
| Periodic | ✅ | ⚠️ 简化 | 使用自由空间近似 |

### Skills文档更新

已更新 `.claude/skills/operators/greens_function_rules.md`，包含：
- ✅ 支持的Green's function类型
- ✅ 实现状态
- ✅ 架构规则（L3 vs L4）
- ✅ 使用示例

## 2. OpenMP 编译错误修复

### 问题
- **错误**: `error C3015: OpenMP"for"语句中的初始化格式不正确`
- **原因**: MSVC的OpenMP实现要求循环变量必须在循环外声明
- **位置**: `src/operators/matvec/matvec_operator.c` 第50、108、166行

### 修复

#### 修复1: `matvec_operator_apply` (第50行)
```c
// 修复前
#pragma omp parallel for
for (int i = 0; i < A->num_rows; i++) {

// 修复后
int i;
#pragma omp parallel for
for (i = 0; i < A->num_rows; i++) {
```

#### 修复2: `matvec_operator_apply_transpose` (第108行)
```c
// 修复前
#pragma omp parallel for
for (int j = 0; j < A->num_cols; j++) {

// 修复后
int j;
#pragma omp parallel for
for (j = 0; j < A->num_cols; j++) {
```

#### 修复3: `matvec_operator_apply_hermitian` (第166行)
```c
// 修复前
#pragma omp parallel for
for (int j = 0; j < A->num_cols; j++) {

// 修复后
int j;
#pragma omp parallel for
for (j = 0; j < A->num_cols; j++) {
```

### MSVC OpenMP要求

MSVC的OpenMP不支持在`#pragma omp parallel for`后面的for循环中声明变量：

**错误格式**:
```c
#pragma omp parallel for
for (int i = 0; i < n; i++) {  // ❌ MSVC不支持
```

**正确格式**:
```c
int i;
#pragma omp parallel for
for (i = 0; i < n; i++) {  // ✅ MSVC支持
```

## 修复的文件

1. ✅ `src/operators/matvec/matvec_operator.c` - 修复3处OpenMP错误
2. ✅ `.claude/skills/operators/greens_function_rules.md` - 更新Green's function文档

## 验证

修复后应该能够编译通过。如果仍有问题，请检查：

1. ✅ OpenMP循环变量是否在循环外声明
2. ✅ Green's function接口是否完整
3. ✅ 是否有其他OpenMP相关的编译错误

## 注意事项

1. **MSVC兼容性**: MSVC的OpenMP实现与GCC/Clang有差异
2. **变量作用域**: 循环变量在循环外声明，作用域扩展到函数结束
3. **Green's Function**: Layered media和Periodic的完整实现需要在L4后端完成
