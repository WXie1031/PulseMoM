# TriMesh2 集成指南

## 架构位置

根据六层架构原则，三角网格生成代码应该放在 **L2 Discretization Layer**：

```
src/discretization/mesh/
├── triangular_mesh_solver.c    # 现有的三角网格生成代码
├── mesh_algorithms.c            # 网格生成算法
├── mesh_engine.c                # 网格引擎
└── [trimesh2_integration.c]     # TriMesh2 集成代码（建议位置）
```

## 为什么放在 L2 层？

根据架构规则：
- **L2 Discretization Layer**: 负责将连续物理空间转换为自由度
- 三角网格生成是**几何离散化**，不是物理定义
- 网格生成是**求解器无关**的，MoM、PEEC、混合求解器都可以使用

## TriMesh2 集成方案

### 方案 1: 作为外部库（推荐）

如果 TriMesh2 是第三方库：

1. **库文件位置**:
   ```
   libs/trimesh2_windows_2022.03.04/
   ├── include/          # 头文件
   ├── lib/              # 库文件
   └── src/              # 源代码（如果需要）
   ```

2. **集成接口** (在 `src/discretization/mesh/` 中):
   ```c
   // trimesh2_wrapper.h
   #ifndef TRIMESH2_WRAPPER_H
   #define TRIMESH2_WRAPPER_H
   
   #include "core_mesh.h"
   
   // 使用 TriMesh2 生成三角网格
   int trimesh2_generate_triangular_mesh(
       const geom_geometry_t* geometry,
       const mesh_request_t* request,
       mesh_result_t* result
   );
   #endif
   ```

3. **项目文件配置**:
   - 添加 include 路径: `$(SolutionDir)libs\trimesh2_windows_2022.03.04\include`
   - 添加库路径: `$(SolutionDir)libs\trimesh2_windows_2022.03.04\lib`
   - 链接库文件: `trimesh2.lib`

### 方案 2: 集成到现有代码

如果要将 TriMesh2 的代码集成到项目中：

1. **文件位置**:
   ```
   src/discretization/mesh/
   ├── triangular_mesh_solver.c      # 现有代码
   ├── trimesh2_integration.c        # TriMesh2 集成代码
   └── trimesh2_integration.h        # TriMesh2 集成头文件
   ```

2. **接口设计**:
   ```c
   // trimesh2_integration.h
   #include "core_mesh.h"
   
   // 使用 TriMesh2 算法生成网格
   mesh_t* trimesh2_generate_mesh(
       const geom_geometry_t* geometry,
       const mesh_params_t* params
   );
   ```

## 当前状态

- ✅ `triangular_mesh_solver.c` 已在正确位置 (`src/discretization/mesh/`)
- ❌ `solvers/mom/tri_mesh.c` 引用已从项目文件中移除（文件不存在且位置错误）

## 下一步

1. **如果 TriMesh2 是外部库**:
   - 将库文件放到 `libs/trimesh2_windows_2022.03.04/`
   - 创建包装接口 `src/discretization/mesh/trimesh2_wrapper.h/c`
   - 更新项目文件配置

2. **如果要集成 TriMesh2 代码**:
   - 将代码放到 `src/discretization/mesh/trimesh2_integration.c/h`
   - 在 `mesh_algorithms.c` 中调用 TriMesh2 函数
   - 更新项目文件添加新文件

## 架构原则

- ✅ **L2 层**: 网格生成、几何离散化
- ❌ **L5 层 (solvers/)**: 不应包含网格生成代码
- ✅ **求解器无关**: 网格生成代码不依赖特定求解器

## 相关文件

- `src/discretization/mesh/triangular_mesh_solver.c` - 现有三角网格生成代码
- `src/discretization/mesh/mesh_algorithms.c` - 网格生成算法
- `src/discretization/mesh/mesh_engine.c` - 网格引擎
- `.claude/skills/discretization/mesh_rules.md` - 网格生成架构规则
