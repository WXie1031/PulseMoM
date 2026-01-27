# Execution Graph 结构体编译错误修复

## 问题
- **错误**: `error C2039: "capacity": 不是 "execution_graph_t" 的成员`
- **错误**: `error C2039: "task_ids": 不是 "execution_graph_t" 的成员`
- **位置**: `src/orchestration/execution/execution_order.c` 多处
- **原因**: 头文件中的 `execution_graph_t` 结构体定义缺少 `task_ids` 和 `capacity` 字段，但实现代码中使用了这些字段

## 修复

### 1. src/orchestration/execution/execution_order.h

在 `execution_graph_t` 结构体中添加缺失的字段：

```c
// 修复前
typedef struct {
    execution_task_t* tasks;
    task_dependency_t* dependencies;
    int num_tasks;
    int num_dependencies;
} execution_graph_t;

// 修复后
typedef struct {
    execution_task_t* tasks;
    task_dependency_t* dependencies;
    int* task_ids;              // Array of task IDs (parallel to tasks array)
    int num_tasks;
    int num_dependencies;
    int capacity;               // Capacity of arrays (for dynamic resizing)
} execution_graph_t;
```

### 2. src/orchestration/execution/execution_order.c

删除未使用的内部结构体定义：

```c
// 删除以下未使用的结构体定义
struct execution_graph {
    execution_task_t* tasks;
    task_dependency_t* dependencies;
    int* task_ids;
    int num_tasks;
    int num_dependencies;
    int capacity;
};
```

## 字段说明

### task_ids
- **类型**: `int*`
- **用途**: 存储任务ID数组，与 `tasks` 数组并行
- **使用**: 用于通过任务ID查找任务索引，管理任务依赖关系

### capacity
- **类型**: `int`
- **用途**: 存储动态数组的容量，用于动态调整数组大小
- **使用**: 当 `num_tasks >= capacity` 时，重新分配更大的数组

## 结构体设计

`execution_graph_t` 结构体设计用于管理执行任务图和依赖关系：

- `tasks`: 任务类型数组
- `task_ids`: 任务ID数组（与 `tasks` 并行）
- `dependencies`: 任务依赖关系数组
- `num_tasks`: 当前任务数量
- `num_dependencies`: 当前依赖关系数量
- `capacity`: 数组容量（用于动态调整）

## 验证

修复后应该能够编译通过。所有对 `task_ids` 和 `capacity` 字段的访问现在都能正确识别。

## 相关文件

- `src/orchestration/execution/execution_order.h` - 结构体定义
- `src/orchestration/execution/execution_order.c` - 实现代码
