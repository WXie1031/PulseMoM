# Core目录剩余文件迁移报告

## 执行时间
2025-01-XX

## 已完成的工作

### ✅ L3算子层文件迁移

#### 积分相关（迁移到 `src/operators/integration/`）
1. `integration_utils.c/h` → `src/operators/integration/integration_utils.c/h`
2. `integration_utils_optimized.c/h` → `src/operators/integration/integration_utils_optimized.c/h`
3. `integral_logarithmic_singular.c/h` → `src/operators/integration/integral_logarithmic_singular.c/h`
4. `integral_nearly_singular.c/h` → `src/operators/integration/integral_nearly_singular.c/h`
5. `industrial_singular_integration.h` → `src/operators/integration/industrial_singular_integration.h`

#### 格林函数相关（迁移到 `src/operators/greens/`）
1. `layered_greens_function.h` → `src/operators/greens/layered_greens_function.h`
2. `layered_greens_function_unified.c` → `src/operators/greens/layered_greens_function_unified.c`
3. `layered_greens_function_optimized.h` → `src/operators/greens/layered_greens_function_optimized.h`
4. `advanced_layered_greens_function.h` → `src/operators/greens/advanced_layered_greens_function.h`

#### 核函数相关（迁移到 `src/operators/kernels/`）
1. `kernel_cavity_waveguide.c/h` → `src/operators/kernels/kernel_cavity_waveguide.c/h`
2. `kernel_cfie.c/h` → `src/operators/kernels/kernel_cfie.c/h`
3. `kernel_mfie.c/h` → `src/operators/kernels/kernel_mfie.c/h`
4. `electromagnetic_kernel_library.c/h` → `src/operators/kernels/electromagnetic_kernel_library.c/h`
5. `electromagnetic_kernels_solver.c` → `src/operators/kernels/electromagnetic_kernels_solver.c`
6. `electromagnetic_kernels_solver_compat.h` → `src/operators/kernels/electromagnetic_kernels_solver_compat.h`
7. `core_solver_electromagnetic_kernels.c` → `src/operators/kernels/core_solver_electromagnetic_kernels.c`

### ✅ L4后端层文件迁移

#### H矩阵相关（迁移到 `src/backend/fast_algorithms/`）
1. `h_matrix_compression.c/h` → `src/backend/fast_algorithms/h_matrix_compression.c/h`
2. `h_matrix_compression_optimized.c` → `src/backend/fast_algorithms/h_matrix_compression_optimized.c`
3. `h_matrix_advanced_compression.h` → `src/backend/fast_algorithms/h_matrix_advanced_compression.h`

#### GPU相关（迁移到 `src/backend/gpu/`）
1. `gpu_acceleration.c/h` → `src/backend/gpu/gpu_acceleration.c/h`
2. `gpu_linalg_optimization.c/h` → `src/backend/gpu/gpu_linalg_optimization.c/h`
3. `gpu_parallelization_optimized.cu/h` → `src/backend/gpu/gpu_parallelization_optimized.cu/h`

#### 内存相关（迁移到 `src/backend/memory/`）
1. `memory_pool.c/h` → `src/backend/memory/memory_pool.c/h`
2. `memory_pool_optimized.c` → `src/backend/memory/memory_pool_optimized.c`

#### 求解器优化（迁移到 `src/backend/solvers/`）
1. `iterative_solver_optimized.c` → `src/backend/solvers/iterative_solver_optimized.c`

#### 性能监控（迁移到 `src/utils/performance/`）
1. `performance_monitoring_optimized.c` → `src/utils/performance/performance_monitoring_optimized.c`

### ✅ L2离散层文件迁移

#### 基函数相关（迁移到 `src/discretization/basis/`）
1. `higher_order_basis.c/h` → `src/discretization/basis/higher_order_basis.c/h`

#### 端口支持（迁移到 `src/discretization/geometry/`）
1. `port_support_extended.c/h` → `src/discretization/geometry/port_support_extended.c/h`

### ✅ L1物理层文件迁移

#### 激励相关（迁移到 `src/physics/excitation/`）
1. `excitation_plane_wave.c/h` → `src/physics/excitation/excitation_plane_wave.c/h`

### ✅ L6 IO层文件迁移

#### 导出格式（迁移到 `src/io/file_formats/`）
1. `export_formats.c/h` → `src/io/file_formats/export_formats.c/h`
2. `export_hdf5.c/h` → `src/io/file_formats/export_hdf5.c/h`
3. `export_vtk.c/h` → `src/io/file_formats/export_vtk.c/h`

#### EMC分析（迁移到 `src/io/analysis/`）
1. `emc_analysis.c/h` → `src/io/analysis/emc_analysis.c/h`

### ✅ L5编排层文件迁移

#### 宽带仿真（迁移到 `src/orchestration/wideband/`）
1. `wideband_simulation_optimization.c/h` → `src/orchestration/wideband/wideband_simulation_optimization.c/h`
2. `core_wideband.h` → `src/orchestration/wideband/core_wideband.h`

### ✅ Python文件迁移

1. `enhanced_implementations.py` → `python/enhanced_implementations.py`

## 清理统计

- **移动文件数**: 约40个文件
- **创建目录数**: 多个新目录（符合架构）

## 待处理的问题

### ⚠️ 需要进一步处理

1. **更新引用路径**: 需要搜索并更新所有对已迁移文件的引用
2. **更新移动文件的include路径**: 需要更新移动文件内部的include路径
3. **Core目录剩余文件**: 检查core目录中还有哪些文件需要处理

## 下一步建议

1. **更新引用路径**: 搜索并更新所有对已迁移文件的引用
2. **更新移动文件的include路径**: 检查并更新移动文件内部的include路径
3. **检查core目录**: 查看core目录中还有哪些文件需要处理
