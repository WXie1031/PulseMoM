# Core目录分析报告

## 分析时间
2025-01-XX

## 概述

`src/core/` 目录包含50+文件，严重违反六层架构原则。本报告分析每个文件的归属层，为重构提供依据。

## 文件分类

### L1 物理层（Physics Definition）

应该迁移到 `src/physics/` 或 `src/solvers/`：

1. **`core_circuit.c/h`** - PEEC电路模型定义
2. **`core_circuit_coupling.h`** - 电路耦合定义
3. **`mtl_solver.c/h`** - MTL求解器（物理定义）
4. **`advanced_material_models.c/h`** - 高级材料模型定义
5. **`circuit_coupling_simulation.c`** - 电路耦合仿真（可能属于L5）

### L2 离散层（Discretization）

应该迁移到 `src/discretization/`：

1. **`core_geometry.c/h`** - 几何处理
2. **`core_geometry_min.c`** - 最小几何实现（可能过时）
3. **`core_mesh.h`** - 网格定义
4. **`core_mesh_pipeline.c/h`** - 网格生成流水线
5. **`core_mesh_unified.c`** - 统一网格实现（可能过时）
6. **`core_mesh_advanced.h`** - 高级网格定义（可能过时）
7. **`basis_functions.c/h`** - 基函数定义
8. **`port_support_extended.c/h`** - 端口支持（可能属于L1或L2）
9. **`core/geometry/pcb_ic_structures.c/h`** - PCB/IC结构定义（已统一）

### L3 算子层（Operators）

应该迁移到 `src/operators/`：

1. **`core_kernels.c/h`** - 积分核
2. **`electromagnetic_kernels.c/h`** - 电磁积分核
3. **`electromagnetic_kernels_solver.c`** - 求解器兼容的积分核
4. **`electromagnetic_kernels_solver_compat.h`** - 兼容性头文件
5. **`electromagnetic_kernel_library.c/h`** - 积分核库
6. **`core_assembler.c/h`** - 矩阵组装
7. **`coupling_quasistatic.c/h`** - 准静态耦合算子
8. **`periodic_ewald.c/h`** - 周期Ewald方法
9. **`windowed_greens_function.c/h`** - 窗口化格林函数
10. **`advanced_layered_greens_function.h`** - 高级分层格林函数
11. **`core_solver_electromagnetic_kernels.c`** - 求解器电磁核

### L4 数值后端层（Numerical Backend）

应该迁移到 `src/backend/`：

1. **`core_solver.c/h`** - 求解器接口
2. **`core_solver_sparse_direct.c/h`** - 稀疏直接求解器
3. **`sparse_direct_solver.c/h`** - 稀疏直接求解器（重复？）
4. **`unified_solver_interface.c/h`** - 统一求解器接口
5. **`core_optimization.c/h`** - 优化算法
6. **`out_of_core_solver.c/h`** - 外存求解器
7. **`multi_gpu_work_distribution.c/h`** - 多GPU工作分配
8. **`performance_monitoring_optimized.c`** - 性能监控优化
9. **`wideband_simulation_optimization.c/h`** - 宽带仿真优化
10. **`core_wideband.h`** - 宽带定义
11. **`core/algorithms/`** - 算法库（已存在）
12. **`core/math/`** - 数学库（已存在）
13. **`core/memory/`** - 内存管理（已存在）

### L5 编排层（Orchestration）

应该迁移到 `src/orchestration/`：

1. **`core_multiphysics.c/h`** - 多物理场耦合
2. **`checkpoint_recovery.c/h`** - 检查点恢复
3. **`circuit_coupling_simulation.c`** - 电路耦合仿真（可能属于L1）

### L6 IO层（IO/Workflow/API）

应该迁移到 `src/io/`：

1. **`touchstone_export.c/h`** - Touchstone格式导出
2. **`spice_export.c`** - SPICE格式导出
3. **`postprocessing_field.c/h`** - 后处理场
4. **`result_bundle.c/h`** - 结果打包

### 其他/待定

1. **`core_common.h`** - 公共定义（应该移到 `src/common/`）
2. **`core_errors.c/h`** - 错误处理（应该移到 `src/common/`）
3. **`core_mesh_advanced.h`** - 高级网格（可能过时）
4. **`core_geometry_min.c`** - 最小几何（可能过时）
5. **`core_mesh_unified.c`** - 统一网格（可能过时）
6. **`core/algorithms/`** - 算法库（应该移到 `src/backend/algorithms/`）
7. **`core/math/`** - 数学库（应该移到 `src/backend/math/`）
8. **`core/memory/`** - 内存管理（应该移到 `src/backend/memory/`）
9. **`core/geometry/`** - 几何定义（应该移到 `src/discretization/geometry/`）

## 迁移计划

### 阶段1: 迁移L6 IO层文件（低风险）

1. `touchstone_export.c/h` → `src/io/file_formats/touchstone_io.c/h`
2. `spice_export.c` → `src/io/file_formats/spice_io.c`
3. `postprocessing_field.c/h` → `src/io/postprocessing/field_postprocessing.c/h`
4. `result_bundle.c/h` → `src/io/results/result_bundle.c/h`

### 阶段2: 迁移L2离散层文件（中风险）

1. `core_geometry.c/h` → `src/discretization/geometry/core_geometry.c/h`
2. `core_mesh.h` → `src/discretization/mesh/core_mesh.h`
3. `core_mesh_pipeline.c/h` → `src/discretization/mesh/mesh_pipeline.c/h`
4. `basis_functions.c/h` → `src/discretization/basis/core_basis_functions.c/h`
5. `core/geometry/` → `src/discretization/geometry/`（合并）

### 阶段3: 迁移L3算子层文件（中风险）

1. `core_kernels.c/h` → `src/operators/kernels/core_kernels.c/h`
2. `electromagnetic_kernels.c/h` → `src/operators/kernels/electromagnetic_kernels.c/h`
3. `core_assembler.c/h` → `src/operators/assembler/core_assembler.c/h`
4. `coupling_quasistatic.c/h` → `src/operators/coupling/quasistatic_coupling.c/h`
5. `periodic_ewald.c/h` → `src/operators/integration/periodic_ewald.c/h`
6. `windowed_greens_function.c/h` → `src/operators/kernels/windowed_greens_function.c/h`

### 阶段4: 迁移L4后端层文件（高风险）

1. `core_solver.c/h` → `src/backend/solvers/core_solver.c/h`
2. `core_solver_sparse_direct.c/h` → `src/backend/solvers/sparse_direct_solver.c/h`
3. `unified_solver_interface.c/h` → `src/backend/solvers/unified_interface.c/h`
4. `core_optimization.c/h` → `src/backend/optimization/core_optimization.c/h`
5. `out_of_core_solver.c/h` → `src/backend/solvers/out_of_core_solver.c/h`
6. `multi_gpu_work_distribution.c/h` → `src/backend/gpu/multi_gpu_distribution.c/h`
7. `core/algorithms/` → `src/backend/algorithms/`
8. `core/math/` → `src/backend/math/`（合并到现有目录）
9. `core/memory/` → `src/backend/memory/`（合并到现有目录）

### 阶段5: 迁移L1物理层文件（高风险）

1. `core_circuit.c/h` → `src/physics/peec/peec_circuit.c/h`
2. `mtl_solver.c/h` → `src/physics/mtl/mtl_solver.c/h`
3. `advanced_material_models.c/h` → `src/physics/materials/advanced_models.c/h`

### 阶段6: 迁移L5编排层文件（中风险）

1. `core_multiphysics.c/h` → `src/orchestration/multiphysics/multiphysics_coupling.c/h`
2. `checkpoint_recovery.c/h` → `src/orchestration/checkpoint/checkpoint_recovery.c/h`

### 阶段7: 清理公共文件（低风险）

1. `core_common.h` → `src/common/core_common.h`（或合并到 `types.h`）
2. `core_errors.c/h` → `src/common/errors.c/h`（合并到现有文件）

### 阶段8: 删除过时文件（低风险）

1. `core_geometry_min.c` - 最小版本（可能过时）
2. `core_mesh_unified.c` - 统一版本（可能过时）
3. `core_mesh_advanced.h` - 高级版本（可能过时）

## 风险评估

### 高风险
- 迁移L4后端层文件（大量依赖）
- 迁移L1物理层文件（核心功能）

### 中风险
- 迁移L2离散层文件
- 迁移L3算子层文件
- 迁移L5编排层文件

### 低风险
- 迁移L6 IO层文件
- 清理公共文件
- 删除过时文件

## 执行建议

1. **先执行低风险迁移**（阶段1、7、8）
2. **然后执行中风险迁移**（阶段2、3、6）
3. **最后执行高风险迁移**（阶段4、5）

## 验证方法

1. **编译验证**: 确保所有代码能编译通过
2. **功能验证**: 运行测试套件
3. **架构验证**: 使用 `scripts/validate_architecture.py`
4. **依赖检查**: 检查所有include路径
