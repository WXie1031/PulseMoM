# File IO 和 Hybrid Solver 编译错误修复

## 问题

### 1. STATUS_ERROR_FILE_IO 未定义
- **错误**: `error C2065: "STATUS_ERROR_FILE_IO": 未声明的标识符`
- **位置**: `src/io/file_formats/file_io.c` 第129行
- **原因**: 代码使用了未定义的错误码 `STATUS_ERROR_FILE_IO`

### 2. Hybrid Solver 函数声明不匹配
- **警告**: `warning C4029: 声明的形参表不同于定义`
- **警告**: `warning C4013: "hybrid_solver_execute_*"未定义`
- **位置**: `src/orchestration/hybrid_solver/hybrid_solver.c`
- **原因**: 
  - 函数声明和定义参数不匹配
  - 前向声明位置错误（在调用之后）

## 修复

### 1. src/io/file_formats/file_io.c

将 `STATUS_ERROR_FILE_IO` 替换为 `STATUS_ERROR_FILE_NOT_FOUND`:

```c
// 修复前
FILE* file = fopen(filename, "w");
if (!file) {
    return STATUS_ERROR_FILE_IO;
}

// 修复后
FILE* file = fopen(filename, "w");
if (!file) {
    return STATUS_ERROR_FILE_NOT_FOUND;
}
```

**说明**: `fopen` 失败时（返回 NULL），通常表示文件无法打开，可能是文件不存在或权限问题，使用 `STATUS_ERROR_FILE_NOT_FOUND` 是合适的。

### 2. src/orchestration/hybrid_solver/hybrid_solver.h

修复函数声明，移除多余的参数:

```c
// 修复前
int hybrid_solver_execute(
    hybrid_solver_t* solver,
    execution_context_t* context
);

// 修复后
int hybrid_solver_execute(
    hybrid_solver_t* solver
);
```

**说明**: 实现中使用 `solver->execution_context` 访问上下文，不需要单独的参数。

### 3. src/orchestration/hybrid_solver/hybrid_solver.c

将前向声明移到文件开头（在调用之前）:

```c
// 在文件开头添加前向声明
#include "hybrid_solver.h"
// ... other includes ...

// Forward declarations
static int hybrid_solver_execute_schur_complement(hybrid_solver_t* solver);
static int hybrid_solver_execute_domain_decomposition(hybrid_solver_t* solver);
static int hybrid_solver_execute_iterative_subdomain(hybrid_solver_t* solver);

// ... rest of code ...
```

并删除函数定义之前的前向声明（第232-235行）。

## 错误码说明

### STATUS_ERROR_FILE_NOT_FOUND

**定义**: `src/common/types.h` - `STATUS_ERROR_FILE_NOT_FOUND = -3`

**用途**: 表示文件操作失败，包括：
- 文件不存在
- 文件无法打开（权限问题、路径错误等）
- 文件读取/写入失败

**注意**: 对于文件IO错误，统一使用 `STATUS_ERROR_FILE_NOT_FOUND`，而不是未定义的 `STATUS_ERROR_FILE_IO`。

## 函数签名一致性

### hybrid_solver_execute

**头文件声明**:
```c
int hybrid_solver_execute(hybrid_solver_t* solver);
```

**实现定义**:
```c
int hybrid_solver_execute(hybrid_solver_t* solver) {
    // 使用 solver->execution_context 访问上下文
    // ...
}
```

**说明**: 函数签名必须与声明完全匹配，包括参数数量和类型。

## 前向声明规则

在C语言中，如果函数在被调用之前定义，则不需要前向声明。但如果函数在被调用之后定义，则需要前向声明。

**正确顺序**:
1. 前向声明（如果需要）
2. 函数调用
3. 函数定义

**错误顺序**:
1. 函数调用
2. 前向声明  ❌
3. 函数定义

## 验证

修复后应该能够编译通过，所有警告和错误都应该消失。

## 相关文件

- `src/io/file_formats/file_io.c` - 文件IO实现
- `src/orchestration/hybrid_solver/hybrid_solver.h` - 混合求解器头文件
- `src/orchestration/hybrid_solver/hybrid_solver.c` - 混合求解器实现
