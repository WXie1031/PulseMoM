# Matrix Assembler OpenMP 编译错误修复

## 问题
- **错误**: `error C3015: OpenMP"for"语句中的初始化格式不正确`
- **位置**: `src/operators/assembler/matrix_assembler.c` 第103行
- **原因**: MSVC的OpenMP实现要求循环变量必须在循环外声明

## 修复

### src/operators/assembler/matrix_assembler.c

**修复**: `matrix_assembler_assemble_mom` 函数 (第103行)

```c
// 修复前
#ifdef _OPENMP
#pragma omp parallel for if(spec->use_parallel)
#endif
for (int i = 0; i < num_basis; i++) {

// 修复后
int i;
#ifdef _OPENMP
#pragma omp parallel for if(spec->use_parallel)
#endif
for (i = 0; i < num_basis; i++) {
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

这是OpenMP修复的第四处：
1. ✅ `matvec_operator.c` - `matvec_operator_apply` (第50行)
2. ✅ `matvec_operator.c` - `matvec_operator_apply_transpose` (第108行)
3. ✅ `matvec_operator.c` - `matvec_operator_apply_hermitian` (第166行)
4. ✅ `matrix_assembler.c` - `matrix_assembler_assemble_mom` (第103行)
