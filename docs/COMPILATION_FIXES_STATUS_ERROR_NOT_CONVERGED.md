# STATUS_ERROR_NOT_CONVERGED 编译错误修复

## 问题
- **错误**: `error C2065: "STATUS_ERROR_NOT_CONVERGED": 未声明的标识符`
- **位置**: `src/backend/solvers/iterative_solver.c` 第258行
- **原因**: 代码使用了 `STATUS_ERROR_NOT_CONVERGED`，但该错误码未定义。应该使用 `STATUS_ERROR_CONVERGENCE_FAILURE`

## 修复

### src/backend/solvers/iterative_solver.c

将 `STATUS_ERROR_NOT_CONVERGED` 替换为 `STATUS_ERROR_CONVERGENCE_FAILURE`:

```c
// 修复前
return stats->converged ? STATUS_SUCCESS : STATUS_ERROR_NOT_CONVERGED;

// 修复后
return stats->converged ? STATUS_SUCCESS : STATUS_ERROR_CONVERGENCE_FAILURE;
```

## 错误码说明

### STATUS_ERROR_CONVERGENCE_FAILURE

**定义位置**: `src/common/types.h`

```c
typedef enum {
    // ...
    STATUS_ERROR_CONVERGENCE_FAILURE = -6,
    // ...
} status_t;
```

**用途**: 表示迭代求解器收敛失败，包括：
- 迭代求解器未能收敛
- 达到最大迭代次数仍未收敛
- 残差未减小
- 迭代过程发散

### 常见错误

**错误**: 使用 `STATUS_ERROR_NOT_CONVERGED`（未定义）
```c
return STATUS_ERROR_NOT_CONVERGED;  // ❌ 未定义
```

**正确**: 使用 `STATUS_ERROR_CONVERGENCE_FAILURE`
```c
return STATUS_ERROR_CONVERGENCE_FAILURE;  // ✅ 正确
```

## 使用场景

在迭代求解器中，当求解器未能在最大迭代次数内收敛时，应返回 `STATUS_ERROR_CONVERGENCE_FAILURE`：

```c
// 示例：CG 求解器
for (int k = 0; k < max_iter; k++) {
    // ... 迭代步骤 ...
    
    if (stats->residual_norm < tolerance) {
        stats->converged = true;
        break;
    }
}

return stats->converged ? STATUS_SUCCESS : STATUS_ERROR_CONVERGENCE_FAILURE;
```

## 验证

修复后应该能够编译通过。所有迭代求解器收敛失败的情况现在都使用正确的错误码。

## Skills 文档

已更新 `.claude/skills/common/error_codes.md`，明确说明：
- 不要使用 `STATUS_ERROR_NOT_CONVERGED`（未定义）
- 始终使用 `STATUS_ERROR_CONVERGENCE_FAILURE` 表示迭代求解器收敛失败
