# 架构文件夹命名问题分析

## 问题描述

### 问题1：文件夹命名不符合规范
- **当前状态**：使用了 `L1_physics/`, `L2_discretization/`, `L3_operators/`, `L4_backend/`, `L5_orchestration/`, `L6_io/` 等带编号前缀的文件夹名
- **应该使用**：根据 `.claude/skills/01_architecture_layers.md`，应该使用描述性名称：
  - `physics/` 而不是 `L1_physics/`
  - `discretization/` 而不是 `L2_discretization/`
  - `operators/` 而不是 `L3_operators/`
  - `backend/` 而不是 `L4_backend/`
  - `orchestration/` 而不是 `L5_orchestration/`
  - `io/` 而不是 `L6_io/`

**原因**：
- 编号前缀（L1_、L2_）不够语义化
- 如果将来需要调整层次顺序，编号会变得混乱
- 描述性名称更符合软件工程最佳实践

### 问题2：VS项目未启用重构后的代码
- **当前状态**：`PulseMoM_Core.vcxproj` 中引用的都是旧路径：
  - `core\core_geometry.c`
  - `solvers\mom\mom_solver_unified.c`
  - `solvers\peec\peec_solver_unified.c`
  - 等等
- **应该引用**：重构后的新架构路径（重命名后）：
  - `physics/mom/mom_physics.c`
  - `discretization/mesh/mesh_engine.c`
  - `operators/kernels/mom_kernel.c`
  - 等等

## 影响范围

### 需要重命名的文件夹
1. `src/L1_physics/` → `src/physics/`
2. `src/L2_discretization/` → `src/discretization/`
3. `src/L3_operators/` → `src/operators/`
4. `src/L4_backend/` → `src/backend/`
5. `src/L5_orchestration/` → `src/orchestration/`
6. `src/L6_io/` → `src/io/` (注意：已存在 `src/io/`，需要合并或重命名)

### 需要更新的include路径
根据grep结果，以下文件包含 `L1_`、`L2_` 等路径引用：
- `src/L3_operators/assembler/matrix_assembler.h` - 引用 L1_physics, L2_discretization
- `src/L3_operators/kernels/mom_kernel.h` - 引用 L1_physics
- `src/L3_operators/kernels/peec_kernel.h` - 引用 L1_physics
- `src/L3_operators/coupling/coupling_operator.h` - 引用 L1_physics
- `src/L4_backend/solvers/solver_interface.h` - 引用 L3_operators
- `src/L4_backend/fast_algorithms/hmatrix.h` - 引用 L3_operators
- `src/L5_orchestration/hybrid_solver/hybrid_solver.h` - 引用 L1_physics, L3_operators, L4_backend
- `src/L5_orchestration/hybrid_solver/coupling_manager.h` - 引用 L1_physics, L3_operators
- `src/L5_orchestration/execution/data_flow.h` - 引用 L3_operators
- `src/L6_io/api/c_api.c` - 引用 L5_orchestration, L6_io
- 等等

### VS项目文件需要更新
- `src/PulseMoM_Core.vcxproj` 需要：
  1. 添加新的include路径（physics, discretization, operators, backend, orchestration, io）
  2. 添加新的源文件到编译列表
  3. 移除或标记为排除旧的源文件（如果已迁移）

## 修复计划

### 阶段1：重命名文件夹
1. 重命名 `L1_physics` → `physics`
2. 重命名 `L2_discretization` → `discretization`
3. 重命名 `L3_operators` → `operators`
4. 重命名 `L4_backend` → `backend`
5. 重命名 `L5_orchestration` → `orchestration`
6. 处理 `L6_io` → `io`（需要检查现有io文件夹，可能需要合并）

### 阶段2：更新include路径
1. 更新所有 `#include "../../L1_physics/...` → `#include "../../physics/...`
2. 更新所有 `#include "../../L2_discretization/...` → `#include "../../discretization/...`
3. 更新所有 `#include "../../L3_operators/...` → `#include "../../operators/...`
4. 更新所有 `#include "../../L4_backend/...` → `#include "../../backend/...`
5. 更新所有 `#include "../../L5_orchestration/...` → `#include "../../orchestration/...`
6. 更新所有 `#include "../../L6_io/...` → `#include "../../io/...`

### 阶段3：更新VS项目文件
1. 更新 `AdditionalIncludeDirectories`，添加新路径
2. 添加新的源文件到 `<ItemGroup><ClCompile>` 和 `<ItemGroup><ClInclude>`
3. 验证编译

## 注意事项

1. **io文件夹冲突**：已存在 `src/io/` 文件夹，需要：
   - 检查 `src/io/` 的内容
   - 决定是合并还是重命名现有文件夹
   - 确保没有文件冲突

2. **向后兼容**：如果旧代码还在使用，可能需要：
   - 保留旧路径一段时间
   - 或创建符号链接/适配器

3. **Git操作**：重命名文件夹时，Git应该能自动跟踪，但需要：
   - 确保所有更改都被正确跟踪
   - 提交时使用 `git mv` 或确保Git识别为重命名
