# 代码迁移指南

## 迁移原则

1. **严格分层**: 每个文件必须且只能属于一层
2. **禁止跨层**: 不允许跨层直接调用
3. **接口清晰**: 层间通过明确的接口通信
4. **向后兼容**: 尽量保持API兼容性（通过适配层）

## 文件迁移映射表

### L1 物理层 (src/L1_physics/)

#### MoM物理定义
- `src/solvers/mom/mom_solver.h` → **拆分**:
  - 物理定义部分 → `L1_physics/mom/mom_physics.h`
  - 配置结构 → `L1_physics/mom/mom_physics.h` (保留)
  - 求解器接口 → 移除（属于L5编排层）

需要提取的内容：
- EFIE/MFIE/CFIE方程定义
- 边界条件定义
- 激励源物理定义

#### PEEC物理定义
- `src/solvers/peec/peec_solver.h` → **拆分**:
  - 物理定义部分 → `L1_physics/peec/peec_physics.h`
  - 电路模型定义 → `L1_physics/peec/peec_circuit_model.h`

#### MTL物理定义
- `src/solvers/mtl/mtl_solver_module.h` → `L1_physics/mtl/mtl_physics.h`

#### 混合物理
- `.claude/skills/physics/hybrid_physics_boundary.md` → `L1_physics/hybrid/hybrid_physics_boundary.h`

### L2 离散层 (src/L2_discretization/)

#### 几何处理
- `src/core/core_geometry.h/c` → `L2_discretization/geometry/geometry_engine.h/c`
- `src/cad/cad_mesh_generation.h/c` → `L2_discretization/geometry/cad_import.h/c`
- `src/mesh/opencascade_cad_import.h` → `L2_discretization/geometry/cad_import.h` (合并)

#### 网格生成
- `src/mesh/mesh_engine.h/c` → `L2_discretization/mesh/mesh_engine.h/c`
- `src/core/core_mesh.h` → `L2_discretization/mesh/mesh_engine.h` (合并)
- `src/solvers/mom/tri_mesh.c` → `L2_discretization/mesh/surface_mesh.h/c` (整合)
- `src/solvers/peec/peec_manhattan_mesh.c` → `L2_discretization/mesh/wire_mesh.h/c` (整合)
- `src/solvers/peec/peec_triangular_mesh.c` → `L2_discretization/mesh/surface_mesh.h/c` (整合)

#### 基函数
- `src/core/basis_functions.h/c` → `L2_discretization/basis/rwg_basis.h/c`
- `src/core/higher_order_basis.h/c` → `L2_discretization/basis/higher_order_basis.h/c`

### L3 算子层 (src/L3_operators/)

#### 积分核
- `src/core/electromagnetic_kernels.h/c` → `L3_operators/kernels/mom_kernel.h/c`
- `src/core/kernel_cfie.h/c` → `L3_operators/kernels/cfie_kernel.h/c`
- `src/core/kernel_mfie.h/c` → `L3_operators/kernels/mfie_kernel.h/c`
- `src/core/kernel_cavity_waveguide.h/c` → `L3_operators/kernels/cavity_kernel.h/c`
- `src/solvers/peec/peec_directionality_kernels.h/c` → `L3_operators/kernels/peec_kernel.h/c`

#### 格林函数
- `src/core/layered_greens_function.h` → `L3_operators/kernels/layered_greens_function.h`
- `src/core/layered_greens_function_unified.c` → `L3_operators/kernels/layered_greens_function.c`
- `src/core/windowed_greens_function.h/c` → `L3_operators/kernels/windowed_greens_function.h/c`
- `src/core/periodic_ewald.c` → `L3_operators/kernels/periodic_greens_function.c`

#### 积分计算
- `src/core/integration_utils.h/c` → `L3_operators/integration/integration_utils.h/c`
- `src/core/integration_utils_optimized.h/c` → `L3_operators/integration/integration_utils.h/c` (合并)
- `src/core/integral_nearly_singular.h/c` → `L3_operators/integration/singular_integration.h/c`
- `src/core/integral_logarithmic_singular.h/c` → `L3_operators/integration/singular_integration.h/c` (合并)
- `src/core/industrial_singular_integration.h` → `L3_operators/integration/singular_integration.h` (合并)

#### 矩阵组装
- `src/core/core_assembler.h/c` → `L3_operators/assembler/matrix_assembler.h/c`
- `src/core/excitation_plane_wave.h/c` → `L3_operators/assembler/excitation_assembler.h/c`

#### 矩阵向量积
- `src/solvers/mom/mom_matvec.h/c` → `L3_operators/matvec/matvec_operator.h/c`

#### 耦合算子
- `src/core/coupling_quasistatic.h/c` → `L3_operators/coupling/coupling_operator.h/c`

### L4 数值后端 (src/L4_backend/)

#### 求解器
- `src/core/core_solver.h/c` → `L4_backend/solvers/solver_interface.h` (接口)
- `src/core/core_solver_sparse_direct.h/c` → `L4_backend/solvers/direct_solver.h/c`
- `src/core/sparse_direct_solver.h/c` → `L4_backend/solvers/direct_solver.h/c` (合并)
- `src/core/iterative_solver_optimized.c` → `L4_backend/solvers/iterative_solver.h/c`
- `src/core/out_of_core_solver.h/c` → `L4_backend/solvers/out_of_core_solver.h/c`

#### 快速算法
- `src/core/h_matrix_compression.h/c` → `L4_backend/fast_algorithms/hmatrix.h/c`
- `src/solvers/mom/mom_mlfmm.h/c` → `L4_backend/fast_algorithms/mlfmm.h/c`
- `src/solvers/mom/mom_aca.h/c` → `L4_backend/fast_algorithms/aca.h/c`
- `src/solvers/mom/mom_hmatrix.h/c` → `L4_backend/fast_algorithms/hmatrix.h/c` (合并)
- `src/core/algorithms/fast_multipole_algorithm.h/c` → `L4_backend/fast_algorithms/fmm.h/c`

#### GPU加速
- `src/core/gpu_acceleration.h/c` → `L4_backend/gpu/gpu_linear_algebra.h/c`
  - **重要**: 需要分离物理计算和数值实现
  - GPU kernel中的物理公式 → 移到L3算子层
  - 只保留数值计算部分
- `src/core/gpu_parallelization_optimized.cu/h` → `L4_backend/gpu/gpu_kernels.cu/h`
- `src/core/gpu_linalg_optimization.c/h` → `L4_backend/gpu/gpu_linear_algebra.h/c` (合并)
- `src/core/multi_gpu_work_distribution.c` → `L4_backend/gpu/gpu_memory.h/c`

#### 内存管理
- `src/core/memory_pool.h/c` → `L4_backend/memory/memory_pool.h/c`
- `src/core/memory_pool_optimized.c` → `L4_backend/memory/memory_pool.h/c` (合并)
- `src/core/memory/memory_optimization.h/c` → `L4_backend/memory/memory_optimization.h/c`

#### 数学库接口
- `src/core/math/math_backend_selector.h` → `L4_backend/math/math_backend_selector.h`
- `src/core/math/math_backend_implementation.c` → `L4_backend/math/blas_interface.h/c`
- `src/core/math/unified_matrix_assembly.h/c` → `L4_backend/math/matrix_assembly.h/c`

### L5 编排层 (src/L5_orchestration/)

#### 混合求解器
- `src/solvers/hybrid/hybrid_solver.h/c` → `L5_orchestration/hybrid_solver/hybrid_solver.h/c`
  - **重要**: 移除所有物理和数值代码
  - 只保留编排逻辑
- `src/solvers/hybrid/mtl_coupling/mtl_hybrid_coupling.h/c` → `L5_orchestration/hybrid_solver/coupling_manager.h/c`

#### 工作流
- `src/workflows/pcb/pcb_simulation_workflow.h/c` → `L5_orchestration/workflow/pcb_workflow.h/c`
- `src/io/pcb_simulation_workflow.h/c` → `L5_orchestration/workflow/pcb_workflow.h/c` (合并)

#### 执行管理
- 新建: `L5_orchestration/execution/execution_order.h/c`
- 新建: `L5_orchestration/execution/data_flow.h/c`

### L6 IO层 (src/L6_io/)

#### 文件格式
- `src/core/export_formats.h/c` → `L6_io/file_formats/export_formats.h/c`
- `src/core/export_vtk.h/c` → `L6_io/file_formats/vtk_io.h/c`
- `src/core/export_hdf5.h/c` → `L6_io/file_formats/hdf5_io.h/c`
- `src/core/touchstone_export.h/c` → `L6_io/file_formats/touchstone_io.h/c`
- `src/core/spice_export.c` → `L6_io/file_formats/spice_io.h/c`
- `src/io/advanced_file_formats.h/c` → `L6_io/file_formats/cad_io.h/c` (合并)
- `src/io/pcb_file_io.h/c` → `L6_io/file_formats/pcb_io.h/c`

#### API接口
- `src/api/api_generator.h/c` → `L6_io/api/api_generator.h/c`
- `src/c_interface/satellite_mom_peec_interface.h/c` → `L6_io/api/c_api.h/c`

#### CLI
- `src/apps/mom_cli.c` → `L6_io/cli/mom_cli.c`
- `src/apps/peec_cli.c` → `L6_io/cli/peec_cli.c`
- `src/apps/hybrid_cli.c` → `L6_io/cli/hybrid_cli.c`

#### GUI
- `src/gui/advanced_ui_system.c` → `L6_io/gui/ui_system.c`

## 迁移步骤

### 步骤1: 创建common目录
✅ 已完成

### 步骤2: 迁移common文件
✅ 已完成: types.h, constants.h, errors.h

### 步骤3: 迁移L1物理层
1. 提取MoM物理定义
2. 提取PEEC物理定义
3. 提取MTL物理定义
4. 提取混合物理边界条件

### 步骤4: 迁移L2离散层
1. 整合几何处理代码
2. 整合网格生成代码
3. 整合基函数代码

### 步骤5: 迁移L3算子层
1. 提取积分核代码
2. 提取积分计算代码
3. 提取矩阵组装代码
4. 提取耦合算子代码

### 步骤6: 迁移L4数值后端
1. 提取求解器代码
2. 提取快速算法代码
3. **分离GPU代码中的物理和数值部分**
4. 提取内存管理代码

### 步骤7: 迁移L5编排层
1. **重构混合求解器（移除物理/数值代码）**
2. 重构工作流代码
3. 创建执行管理代码

### 步骤8: 迁移L6 IO层
1. 整合文件格式代码
2. 整合API代码
3. 整合CLI代码

### 步骤9: 更新include路径
1. 更新所有文件的include路径
2. 创建层间接口头文件
3. 修复编译错误

### 步骤10: 更新构建系统
1. 更新CMakeLists.txt
2. 更新依赖关系
3. 验证构建

## 关键注意事项

### GPU代码分离
GPU代码中经常混合物理公式和数值实现。需要：
1. 物理公式部分 → L3算子层
2. GPU kernel调用和内存管理 → L4数值后端

### 混合求解器重构
混合求解器必须：
1. ❌ 不包含物理方程
2. ❌ 不包含数值实现
3. ✅ 只包含编排逻辑
4. ✅ 通过接口调用各层

### 向后兼容
为了保持API兼容性，可以：
1. 在旧位置创建适配层
2. 逐步迁移调用代码
3. 最终移除适配层

## 验证清单

每个文件迁移后：
- [ ] 文件属于且仅属于一层
- [ ] 没有跨层依赖
- [ ] include路径正确
- [ ] 编译通过
- [ ] 测试通过
