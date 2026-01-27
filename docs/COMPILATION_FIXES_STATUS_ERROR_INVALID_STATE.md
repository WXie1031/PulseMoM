# STATUS_ERROR_INVALID_STATE 编译错误修复

## 问题
- **错误**: `error C2065: "STATUS_ERROR_INVALID_STATE": 未声明的标识符`
- **位置**: `src/backend/solvers/solver_interface.c` 第203行
- **原因**: `status_t` 枚举中缺少 `STATUS_ERROR_INVALID_STATE` 定义

## 修复

### 1. src/common/types.h

在 `status_t` 枚举中添加 `STATUS_ERROR_INVALID_STATE`:

```c
// 修复前
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR_INVALID_INPUT = -1,
    STATUS_ERROR_MEMORY_ALLOCATION = -2,
    STATUS_ERROR_FILE_NOT_FOUND = -3,
    STATUS_ERROR_INVALID_FORMAT = -4,
    STATUS_ERROR_NUMERICAL_INSTABILITY = -5,
    STATUS_ERROR_CONVERGENCE_FAILURE = -6,
    STATUS_ERROR_NOT_IMPLEMENTED = -7
} status_t;

// 修复后
typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_ERROR_INVALID_INPUT = -1,
    STATUS_ERROR_MEMORY_ALLOCATION = -2,
    STATUS_ERROR_FILE_NOT_FOUND = -3,
    STATUS_ERROR_INVALID_FORMAT = -4,
    STATUS_ERROR_NUMERICAL_INSTABILITY = -5,
    STATUS_ERROR_CONVERGENCE_FAILURE = -6,
    STATUS_ERROR_NOT_IMPLEMENTED = -7,
    STATUS_ERROR_INVALID_STATE = -8
} status_t;
```

### 2. src/common/errors.h

添加向后兼容的宏定义:

```c
// 修复前
#define CORE_ERROR_CONVERGENCE_FAILURE STATUS_ERROR_CONVERGENCE_FAILURE

// 修复后
#define CORE_ERROR_CONVERGENCE_FAILURE STATUS_ERROR_CONVERGENCE_FAILURE
#define CORE_ERROR_INVALID_STATE STATUS_ERROR_INVALID_STATE
```

## STATUS_ERROR_INVALID_STATE 用途

`STATUS_ERROR_INVALID_STATE` 用于表示对象处于无效状态，常见场景：

1. **操作前未初始化**: 在对象初始化前调用操作
2. **状态不匹配**: 对象状态不允许执行该操作
3. **结果不可用**: 请求结果但尚未计算完成
4. **依赖缺失**: 缺少必要的依赖（如矩阵未设置就求解）

## 使用示例

### solver_interface.c 中的使用

```c
int solver_interface_get_solution(
    const solver_interface_t* solver,
    operator_vector_t* solution) {
    
    if (!solver || !solution) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (!solver->solution) {
        return STATUS_ERROR_INVALID_STATE;  // 解决方案尚未计算
    }
    
    // ...
}
```

## 其他使用位置

`STATUS_ERROR_INVALID_STATE` 在以下文件中也被使用：

1. `src/io/api/c_api.c` - 几何未加载、结果不可用
2. `src/orchestration/hybrid_solver/hybrid_solver.c` - 状态检查
3. `src/orchestration/hybrid_solver/coupling_manager.c` - 状态检查
4. `src/backend/gpu/gpu_linear_algebra.c` - GPU状态检查
5. `src/orchestration/workflow/workflow_engine.c` - 工作流状态检查
6. `src/orchestration/execution/execution_order.c` - 执行顺序检查
7. `src/backend/solvers/direct_solver.c` - 求解器状态检查
8. `src/backend/solvers/solver_interface.c` - 求解器接口状态检查

## 验证

修复后应该能够编译通过，所有使用 `STATUS_ERROR_INVALID_STATE` 的地方都能正确识别该错误码。

## Skills 文档更新

已创建 `.claude/skills/common/error_codes.md` 文档，详细记录了所有错误码及其使用指南。
