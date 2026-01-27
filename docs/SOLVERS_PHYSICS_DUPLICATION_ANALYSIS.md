# Solvers和Physics目录重复分析报告

## 执行时间
2025-01-XX

## 发现的重复问题

### 1. 物理定义重复 ⚠️

**问题**:
- `src/solvers/mom/mom_solver.h` 中定义了 `MOM_FORMULATION_EFIE`, `MOM_FORMULATION_MFIE`, `MOM_FORMULATION_CFIE`
- `src/physics/mom/mom_physics.h` 中定义了 `mom_formulation_t` 枚举（包含相同的值）
- `src/solvers/peec/peec_solver.h` 中定义了 `PEEC_FORMULATION_CLASSICAL`, `PEEC_FORMULATION_MODIFIED`, `PEEC_FORMULATION_FULL_WAVE`
- `src/physics/peec/peec_physics.h` 中定义了 `peec_formulation_t` 枚举（包含相同的值）
- `src/solvers/mtl/mtl_solver_module.h` 中定义了 `mtl_cable_type_t`, `mtl_conductor_material_t`, `mtl_dielectric_material_t`
- `src/physics/mtl/mtl_physics.h` 中定义了相同的枚举

**分析**:
- 根据架构，物理定义应该只在 `physics/` 目录（L1层）
- `solvers/` 目录应该只包含求解器实现（L4/L5层），不应该包含物理定义
- 应该删除 `solvers/` 中的物理定义，统一使用 `physics/` 中的定义

### 2. 激励源重复 ⚠️

**问题**:
- `src/solvers/peec/peec_plane_wave.c/h` - PEEC平面波激励
- `src/physics/excitation/excitation_plane_wave.c/h` - 通用平面波激励

**分析**:
- `excitation_plane_wave.h` 是L1层的物理定义（平面波的物理含义）
- `peec_plane_wave.h` 是PEEC求解器特定的实现（如何将平面波转换为PEEC激励）
- 应该保留两者，但需要确认 `peec_plane_wave.h` 是否正确引用了 `excitation_plane_wave.h`

### 3. 网格生成代码位置错误 ⚠️

**问题**:
- `src/solvers/mom/tri_mesh.c` - 三角网格生成（应该属于L2离散层）
- `src/solvers/peec/peec_manhattan_mesh.c` - Manhattan网格生成（应该属于L2离散层）
- `src/solvers/peec/peec_triangular_mesh.c` - PEEC三角网格支持（应该属于L2离散层）
- `src/solvers/peec/manhattan_mesh.c` - 另一个Manhattan网格文件（可能重复）

**分析**:
- 根据架构，网格生成属于L2离散层
- 这些文件应该移动到 `src/discretization/mesh/`
- 或者整合到现有的 `mesh_engine.c` 中

## 合并方案

### 方案1: 删除solvers中的物理定义 ✅

**操作**:
1. 删除 `solvers/mom/mom_solver.h` 中的 `#define MOM_FORMULATION_*`
2. 删除 `solvers/peec/peec_solver.h` 中的 `#define PEEC_FORMULATION_*`
3. 更新所有引用，改为使用 `physics/mom/mom_physics.h` 和 `physics/peec/peec_physics.h` 中的枚举

### 方案2: 统一MTL物理定义 ✅

**操作**:
1. 分析 `mtl_solver_physics.h` 和 `mtl_solver_module.h` 的差异
2. 如果 `mtl_solver_physics.h` 是物理定义，应该保留
3. 如果 `mtl_solver_module.h` 包含物理定义，应该删除并引用 `mtl_physics.h`

### 方案3: 移动网格代码到discretization ✅

**操作**:
1. 移动 `solvers/mom/tri_mesh.c` → `discretization/mesh/triangular_mesh.c`（或整合到现有文件）
2. 移动 `solvers/peec/peec_manhattan_mesh.c` → `discretization/mesh/manhattan_mesh.c`（或整合）
3. 移动 `solvers/peec/peec_triangular_mesh.c` → `discretization/mesh/peec_triangular_mesh.c`（或整合）
4. 检查 `manhattan_mesh.c` 是否与 `peec_manhattan_mesh.c` 重复

## 架构符合性

### 改进前
- ⚠️ solvers目录包含物理定义（违反架构）
- ⚠️ 物理定义重复在两个位置
- ⚠️ 网格生成代码在solvers目录（应该属于L2层）

### 改进后
- ✅ 所有物理定义统一在physics/目录（L1层）
- ✅ solvers目录只包含求解器实现（L4/L5层）
- ✅ 网格生成代码在discretization/mesh/（L2层）
