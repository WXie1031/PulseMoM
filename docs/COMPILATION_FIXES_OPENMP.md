# OpenMP 编译错误修复报告

## 问题分析

### 错误: OpenMP for循环初始化格式不正确
- **错误**: `error C3015: OpenMP"for"语句中的初始化格式不正确`
- **原因**: MSVC的OpenMP实现要求循环变量必须在循环外声明
- **位置**: `src/operators/matvec/matvec_operator.c` 第50、108、166行

## MSVC OpenMP 要求

MSVC的OpenMP不支持在`#pragma omp parallel for`后面的for循环中声明变量。必须将循环变量声明移到循环外。

### 错误格式
```c
#pragma omp parallel for
for (int i = 0; i < n; i++) {  // ❌ MSVC不支持
    // ...
}
```

### 正确格式
```c
int i;
#pragma omp parallel for
for (i = 0; i < n; i++) {  // ✅ MSVC支持
    // ...
}
```

## 修复内容

### src/operators/matvec/matvec_operator.c

**修复1**: `matvec_operator_apply` 函数 (第50行)
```c
// 修复前
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (int i = 0; i < A->num_rows; i++) {

// 修复后
int i;
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (i = 0; i < A->num_rows; i++) {
```

**修复2**: `matvec_operator_apply_transpose` 函数 (第108行)
```c
// 修复前
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (int j = 0; j < A->num_cols; j++) {

// 修复后
int j;
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (j = 0; j < A->num_cols; j++) {
```

**修复3**: `matvec_operator_apply_hermitian` 函数 (第166行)
```c
// 修复前
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (int j = 0; j < A->num_cols; j++) {

// 修复后
int j;
#ifdef _OPENMP
#pragma omp parallel for
#endif
for (j = 0; j < A->num_cols; j++) {
```

## 验证

修复后应该能够编译通过。如果仍有问题，请检查：

1. ✅ 循环变量是否在循环外声明
2. ✅ OpenMP pragma格式是否正确
3. ✅ 是否有其他OpenMP相关的编译错误

## 注意事项

1. **MSVC兼容性**: MSVC的OpenMP实现与GCC/Clang有差异
2. **变量作用域**: 循环变量在循环外声明，作用域扩展到函数结束
3. **其他编译器**: GCC/Clang支持在循环中声明变量，但为了兼容性，建议统一使用循环外声明
