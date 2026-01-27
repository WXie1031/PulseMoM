# Solvers和Physics目录清理进度报告

## 执行时间
2025-01-XX

## 发现的重复问题

### 1. 物理定义重复 ✅ 已修复

**问题**:
- `src/solvers/mom/mom_solver.h` 中定义了 `#define MOM_FORMULATION_EFIE/MFIE/CFIE`
- `src/physics/mom/mom_physics.h` 中定义了 `mom_formulation_t` 枚举
- `src/solvers/peec/peec_solver.h` 中定义了 `#define PEEC_FORMULATION_*`
- `src/physics/peec/peec_physics.h` 中定义了 `peec_formulation_t` 枚举
- `src/solvers/mtl/mtl_solver_module.h` 中定义了 `mtl_cable_type_t`, `mtl_conductor_material_t`, `mtl_dielectric_material_t`
- `src/physics/mtl/mtl_physics.h` 中定义了相同的枚举

**修复**:
- ✅ 在 `mom_solver.h` 中添加 `#include "../../physics/mom/mom_physics.h"`
- ✅ 删除 `#define MOM_FORMULATION_*`，改为使用 `mom_formulation_t` 枚举
- ✅ 更新 `mom_config.formulation` 类型为 `mom_formulation_t`
- ✅ 在 `peec_solver.h` 中添加 `#include "../../physics/peec/peec_physics.h"`
- ✅ 删除 `#define PEEC_FORMULATION_*`，改为使用 `peec_formulation_t` 枚举
- ✅ 更新 `peec_config.formulation` 类型为 `peec_formulation_t`
- ✅ 在 `mtl_solver_module.h` 中添加 `#include "../../physics/mtl/mtl_physics.h"`
- ✅ 删除重复的枚举定义，改为使用 `mtl_physics.h` 中的定义

### 2. 激励源重复 ✅ 已确认

**状态**:
- `excitation_plane_wave.h` 是L1层的物理定义（平面波的物理含义）
- `peec_plane_wave.h` 是PEEC求解器特定的实现（如何将平面波转换为PEEC激励）
- ✅ `peec_plane_wave.h` 已经正确引用了 `excitation_plane_wave.h`
- **结论**: 不需要删除，两者职责不同

### 3. 网格生成代码位置错误 ⏳ 待处理

**问题**:
- `src/solvers/mom/tri_mesh.c` - 三角网格生成（应该属于L2离散层）
- `src/solvers/peec/peec_manhattan_mesh.c` - Manhattan网格生成（应该属于L2离散层）
- `src/solvers/peec/peec_triangular_mesh.c` - PEEC三角网格支持（应该属于L2离散层）
- `src/solvers/peec/manhattan_mesh.c` - 另一个Manhattan网格文件（可能重复）

**状态**: ⏳ 待处理

## 清理统计

### 已完成
- **删除重复定义**: 3个文件中的物理定义重复
- **更新引用**: 改为使用physics层定义
- **更新类型**: 结构体成员类型改为使用physics层枚举

### 待处理
- **移动网格代码**: 3-4个网格生成文件需要移动到discretization目录

## 架构符合性改进

### 改进前
- ⚠️ solvers目录包含物理定义（违反架构）
- ⚠️ 物理定义重复在两个位置
- ⚠️ 使用#define而不是类型安全的枚举

### 改进后
- ✅ 所有物理定义统一在physics/目录（L1层）
- ✅ solvers目录引用physics层的定义
- ✅ 使用类型安全的枚举类型
