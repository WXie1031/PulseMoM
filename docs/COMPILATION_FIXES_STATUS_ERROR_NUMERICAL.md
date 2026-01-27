# STATUS_ERROR_NUMERICAL 编译错误修复

## 问题
- **错误**: `error C2065: "STATUS_ERROR_NUMERICAL": 未声明的标识符`
- **位置**: `src/backend/solvers/direct_solver.c` 第88、163、211行
- **原因**: 代码使用了 `STATUS_ERROR_NUMERICAL`，但该错误码未定义。实际应该使用 `STATUS_ERROR_NUMERICAL_INSTABILITY`

## 修复

### src/backend/solvers/direct_solver.c

将所有 `STATUS_ERROR_NUMERICAL` 替换为 `STATUS_ERROR_NUMERICAL_INSTABILITY`:

#### 修复1: `lu_decompose_simple` 函数 (第88行)

```c
// 修复前
if (pivot_mag < NUMERICAL_EPSILON) {
    return STATUS_ERROR_NUMERICAL;  // Singular matrix
}

// 修复后
if (pivot_mag < NUMERICAL_EPSILON) {
    return STATUS_ERROR_NUMERICAL_INSTABILITY;  // Singular matrix
}
```

#### 修复2: `forward_substitution` 函数 (第163行)

```c
// 修复前
if (diag_mag < NUMERICAL_EPSILON) {
    return STATUS_ERROR_NUMERICAL;
}

// 修复后
if (diag_mag < NUMERICAL_EPSILON) {
    return STATUS_ERROR_NUMERICAL_INSTABILITY;
}
```

#### 修复3: `backward_substitution` 函数 (第211行)

```c
// 修复前
if (diag_mag < NUMERICAL_EPSILON) {
    return STATUS_ERROR_NUMERICAL;
}

// 修复后
if (diag_mag < NUMERICAL_EPSILON) {
    return STATUS_ERROR_NUMERICAL_INSTABILITY;
}
```

## 错误码说明

### STATUS_ERROR_NUMERICAL_INSTABILITY

**定义位置**: `src/common/types.h`

```c
typedef enum {
    // ...
    STATUS_ERROR_NUMERICAL_INSTABILITY = -5,
    // ...
} status_t;
```

**用途**: 表示数值不稳定，包括：
- 奇异矩阵或接近奇异的矩阵
- 零主元或接近零的主元（LU分解）
- 零对角线元素（前向/后向替换）
- 除以零或接近零
- 数值溢出/下溢
- 条件数超过阈值

### 常见错误

**错误**: 使用 `STATUS_ERROR_NUMERICAL`（未定义）
```c
return STATUS_ERROR_NUMERICAL;  // ❌ 未定义
```

**正确**: 使用 `STATUS_ERROR_NUMERICAL_INSTABILITY`
```c
return STATUS_ERROR_NUMERICAL_INSTABILITY;  // ✅ 正确
```

## 验证

修复后应该能够编译通过。所有数值不稳定的情况现在都使用正确的错误码。

## Skills 文档更新

已更新 `.claude/skills/common/error_codes.md`，明确说明：
- 不要使用 `STATUS_ERROR_NUMERICAL`（未定义）
- 始终使用 `STATUS_ERROR_NUMERICAL_INSTABILITY`
- 列出了该错误码的所有适用场景
