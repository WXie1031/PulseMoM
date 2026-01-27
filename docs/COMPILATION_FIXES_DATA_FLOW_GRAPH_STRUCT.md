# Data Flow Graph 结构体编译错误修复

## 问题
- **错误**: `error C2039: "capacity": 不是 "data_flow_graph_t" 的成员`
- **位置**: `src/orchestration/execution/data_flow.c` 多处
- **原因**: 头文件中的 `data_flow_graph_t` 结构体定义缺少 `capacity` 字段，但实现代码中使用了这个字段

## 修复

### 1. src/orchestration/execution/data_flow.h

在 `data_flow_graph_t` 结构体中添加缺失的字段：

```c
// 修复前
typedef struct {
    data_handle_t* data_handles;
    int num_data_handles;
} data_flow_graph_t;

// 修复后
typedef struct {
    data_handle_t* data_handles;
    int num_data_handles;
    int capacity;               // Capacity of data_handles array (for dynamic resizing)
} data_flow_graph_t;
```

### 2. src/orchestration/execution/data_flow.c

删除未使用的内部结构体定义：

```c
// 删除以下未使用的结构体定义
struct data_flow_graph {
    data_handle_t* data_handles;
    int num_data_handles;
    int capacity;
};
```

## 字段说明

### capacity
- **类型**: `int`
- **用途**: 存储 `data_handles` 数组的容量，用于动态调整数组大小
- **使用**: 当 `num_data_handles >= capacity` 时，重新分配更大的数组

## 结构体设计

`data_flow_graph_t` 结构体设计用于管理数据流图：

- `data_handles`: 数据句柄数组
- `num_data_handles`: 当前数据句柄数量
- `capacity`: 数组容量（用于动态调整）

## 使用场景

在 `data_flow_register_data` 函数中，当需要添加新的数据句柄时：

```c
// Resize if needed
if (graph->num_data_handles >= graph->capacity) {
    int new_capacity = graph->capacity * 2;
    data_handle_t* new_handles = (data_handle_t*)realloc(
        graph->data_handles, new_capacity * sizeof(data_handle_t));
    if (!new_handles) {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    graph->data_handles = new_handles;
    graph->capacity = new_capacity;
}
```

## 验证

修复后应该能够编译通过。所有对 `capacity` 字段的访问现在都能正确识别。

## 相关文件

- `src/orchestration/execution/data_flow.h` - 结构体定义
- `src/orchestration/execution/data_flow.c` - 实现代码

## 相关修复

这是类似的修复，与 `execution_order.h` 中的 `execution_graph_t` 修复相同：
- ✅ `execution_order.h` - 添加 `task_ids` 和 `capacity` 字段
- ✅ `data_flow.h` - 添加 `capacity` 字段
