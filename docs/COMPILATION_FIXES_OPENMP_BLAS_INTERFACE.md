# BLAS Interface OpenMP 编译错误修复

## 问题
- **错误**: `error C3015: OpenMP"for"语句中的初始化格式不正确`
- **位置**: `src/backend/math/blas_interface.c` 第118、158、288行
- **原因**: MSVC的OpenMP实现要求循环变量必须在循环外声明

## 修复

### src/backend/math/blas_interface.c

#### 修复1: `native_gemv` 函数 - transpose 分支 (第118行)

```c
// 修复前
if (transpose) {
    // y = alpha * A^T * x
    #ifdef _OPENMP
    #pragma omp parallel for
    #endif
    for (int i = 0; i < n; i++) {

// 修复后
if (transpose) {
    // y = alpha * A^T * x
    int i;
    #ifdef _OPENMP
    #pragma omp parallel for
    #endif
    for (i = 0; i < n; i++) {
```

#### 修复2: `native_gemv` 函数 - 非 transpose 分支 (第158行)

```c
// 修复前
} else {
    // y = alpha * A * x
    #ifdef _OPENMP
    #pragma omp parallel for
    #endif
    for (int i = 0; i < m; i++) {

// 修复后
} else {
    // y = alpha * A * x
    int i;
    #ifdef _OPENMP
    #pragma omp parallel for
    #endif
    for (i = 0; i < m; i++) {
```

#### 修复3: `native_gemm` 函数 (第288行)

```c
// 修复前
// Compute C = alpha * A * B + beta * C
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (int i = 0; i < m; i++) {

// 修复后
// Compute C = alpha * A * B + beta * C
int i;
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (i = 0; i < m; i++) {
```

## MSVC OpenMP 要求

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

## 验证

修复后应该能够编译通过。

## 相关修复

这是OpenMP修复的第五、六、七处：
1. ✅ `matvec_operator.c` - `matvec_operator_apply` (第50行)
2. ✅ `matvec_operator.c` - `matvec_operator_apply_transpose` (第108行)
3. ✅ `matvec_operator.c` - `matvec_operator_apply_hermitian` (第166行)
4. ✅ `matrix_assembler.c` - `matrix_assembler_assemble_mom` (第103行)
5. ✅ `blas_interface.c` - `native_gemv` transpose分支 (第118行)
6. ✅ `blas_interface.c` - `native_gemv` 非transpose分支 (第158行)
7. ✅ `blas_interface.c` - `native_gemm` (第288行)
