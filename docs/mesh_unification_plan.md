# 网格接口统一方案

## 问题分析

### 重复定义问题
- **位置1**: `src/solvers/mom/tri_mesh.c` - MoM特定的简化实现
- **位置2**: `src/core/core_mesh_unified.c` - 统一的完整实现

### 实现差异

**tri_mesh.c (已移除)**:
```c
static void tri_mesh_destroy_internal(mesh_t* mesh) {
    if (!mesh) return;
    if (mesh->vertices) free(mesh->vertices);
    if (mesh->elements) free(mesh->elements);
    free(mesh);
}
```
- 只清理基本资源（vertices, elements）
- 不清理扩展资源（edges, 拓扑关系, 分区信息等）

**core_mesh_unified.c (统一实现)**:
```c
void mesh_destroy(mesh_t* mesh) {
    if (!mesh) return;
    
    if (mesh->elements) {
        for (int i = 0; i < mesh->num_elements; i++) {
            free(mesh->elements[i].vertices);
            free(mesh->elements[i].edges);
        }
        free(mesh->elements);
    }
    free(mesh->vertices);
    free(mesh->edges);
    free(mesh->vertex_to_elements);
    free(mesh->element_to_elements);
    free(mesh->partition_offsets);
    free(mesh->partition_elements);
    free(mesh);
}
```
- 完整的资源清理
- 支持并行分区
- 支持拓扑关系
- 统一的接口

## 统一方案

### ✅ 已完成的统一

1. **移除重复实现**
   - 已从 `tri_mesh.c` 中移除 `tri_mesh_destroy_internal`
   - 所有代码现在使用 `core_mesh_unified.c` 中的统一实现

2. **统一接口声明**
   - 接口在 `core_mesh.h` 中声明
   - 所有模块通过 `#include "core_mesh.h"` 使用统一接口

3. **统一仿真Pipeline**
   ```
   ┌─────────────────────────────────────────┐
   │  统一网格接口 (core_mesh.h)              │
   │  - mesh_create()                        │
   │  - mesh_destroy()  ← 统一实现           │
   │  - mesh_add_vertex()                    │
   │  - mesh_add_element()                   │
   └──────────────┬──────────────────────────┘
                  │
      ┌───────────┴───────────┐
      │                       │
   ┌──▼──────────┐    ┌──────▼──────┐
   │  MoM Solver │    │ PEEC Solver │
   │             │    │             │
   │ tri_mesh.c  │    │ manhattan   │
   │ (Triangular)│    │ (Manhattan) │
   │             │    │             │
   │             │    │ triangular  │
   │             │    │ (Triangular)│
   │ (使用统一接口)│    │ (使用统一接口)│
   └─────────────┘    └─────────────┘
   
   注意：PEEC支持两种网格类型：
   - Manhattan矩形网格（传统PEEC）
   - Triangular三角形网格（通用PEEC）
   两者都使用统一的 mesh_destroy() 接口
   ```

### 统一的好处

1. **资源管理一致性**
   - 所有网格对象使用相同的清理逻辑
   - 避免内存泄漏
   - 确保所有资源都被正确释放

2. **代码维护性**
   - 单一实现点，易于维护
   - 修改只需在一个地方进行
   - 减少代码重复

3. **功能完整性**
   - 统一实现支持所有高级特性
   - 并行分区支持
   - 拓扑关系管理

4. **仿真Pipeline统一**
   - MoM和PEEC使用相同的网格接口
   - PEEC支持两种网格类型：
     * Manhattan矩形网格（传统PEEC，适合规则几何）
     * Triangular三角形网格（通用PEEC，适合复杂几何）
   - 混合仿真（MoM-PEEC耦合）更容易实现
   - 统一的错误处理
   - 统一的资源管理（mesh_destroy统一清理所有资源）

## 使用指南

### 正确的使用方式

```c
#include "core_mesh.h"  // 统一接口

// 创建网格
mesh_t* mesh = mesh_create("my_mesh", MESH_TYPE_TRIANGULAR);

// ... 使用网格 ...

// 销毁网格（使用统一接口）
mesh_destroy(mesh);  // 自动清理所有资源
```

### 避免的做法

```c
// ❌ 不要定义自己的 mesh_destroy
static void my_mesh_destroy(mesh_t* mesh) { ... }

// ❌ 不要直接操作内部结构
free(mesh->vertices);  // 应该使用 mesh_destroy()

// ✅ 使用统一接口
mesh_destroy(mesh);
```

## 验证清单

- [x] 移除 `tri_mesh.c` 中的重复实现
- [x] 确保所有模块包含 `core_mesh.h`
- [x] 验证编译无链接错误
- [ ] 运行测试确保功能正常
- [ ] 检查其他模块是否有类似重复

## 后续建议

1. **代码审查**
   - 检查其他模块是否有类似的重复实现
   - 统一所有网格相关接口

2. **测试验证**
   - 确保统一后的实现不影响现有功能
   - 测试内存泄漏情况

3. **文档更新**
   - 更新开发文档，说明统一接口的使用
   - 添加代码示例

## 总结

通过统一 `mesh_destroy` 实现，我们：
- ✅ 消除了重复定义
- ✅ 统一了仿真pipeline
- ✅ 提高了代码质量
- ✅ 简化了维护工作

这是向统一架构迈出的重要一步，为后续的MoM-PEEC混合仿真奠定了基础。
