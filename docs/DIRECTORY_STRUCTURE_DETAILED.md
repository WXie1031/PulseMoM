# PulseMoM 项目目录结构详细文档

## 文档说明

本文档详细描述了 PulseMoM 项目的完整目录结构和文件层次，按照六层架构模型组织。

**生成时间**: 2025-01-XX  
**架构版本**: 六层架构模型 (L1-L6)  
**项目根目录**: `d:\Project\MoM\PulseMoM`

---

## 一、六层架构概览

```
L1  Physics Definition        物理定义层        → src/physics/
L2  Discretization            离散与建模层      → src/discretization/
L3  Operator / Update Eq.     算子/更新方程层   → src/operators/
L4  Numerical Backend          数值后端层        → src/backend/
L5  Execution Orchestration   执行编排层        → src/orchestration/ + src/solvers/
L6  IO / Workflow / API       输入输出层        → src/io/
```

**公共层**:
- `src/common/` - 公共定义和接口
- `src/utils/` - 工具函数
- `src/materials/` - 材料定义
- `src/modeling/` - 建模功能
- `src/applications/` - 应用层
- `src/plugins/` - 插件框架

---

## 二、详细目录结构

### 2.1 L1 物理定义层 (`src/physics/`)

**职责**: 定义物理方程、边界条件、材料属性（WHAT - 物理是什么）

#### 目录结构
```
src/physics/
├── mom/                          # MoM物理模型
│   ├── mom_physics.c
│   └── mom_physics.h              # EFIE, MFIE, CFIE方程定义
│
├── peec/                         # PEEC物理模型
│   ├── peec_physics.c
│   ├── peec_physics.h            # PEEC物理方程定义
│   ├── peec_circuit.c
│   ├── peec_circuit.h            # PEEC电路模型定义
│   ├── peec_circuit_coupling.h   # 电路耦合接口
│   └── circuit_coupling_simulation.c  # 电路-EM耦合仿真（可能属于L5）
│
├── mtl/                          # MTL物理模型
│   ├── mtl_physics.c
│   └── mtl_physics.h             # 传输线方程（Telegrapher方程）
│
├── excitation/                   # 激励源物理定义
│   ├── excitation_plane_wave.c
│   └── excitation_plane_wave.h   # 平面波激励物理定义
│
├── hybrid/                       # 混合物理模型
│   ├── hybrid_physics_boundary.c
│   └── hybrid_physics_boundary.h # 混合边界条件定义
│
└── materials/                    # 材料物理模型
    ├── advanced_models.c
    └── advanced_models.h         # 高级材料模型（频散、非线性等）
```

**文件统计**: 6个目录，16个文件（8个.c，8个.h）

**关键文件说明**:
- `mom_physics.h`: 定义MoM的物理方程（EFIE, MFIE, CFIE）
- `peec_physics.h`: 定义PEEC的物理模型（R/L/C/G提取）
- `mtl_physics.h`: 定义MTL的传输线方程

---

### 2.2 L2 离散与建模层 (`src/discretization/`)

**职责**: 将连续物理空间转换为自由度（HOW - 如何离散化）

#### 目录结构
```
src/discretization/
├── geometry/                     # 几何处理
│   ├── core_geometry.c
│   ├── core_geometry.h           # 核心几何操作
│   ├── geometry_engine.c
│   ├── geometry_engine.h         # 几何引擎
│   ├── pcb_ic_structures.c
│   ├── pcb_ic_structures.h       # PCB/IC结构定义
│   ├── port_support_extended.c
│   ├── port_support_extended.h   # 端口支持
│   ├── opencascade_cad_import.cpp
│   └── opencascade_cad_import.h  # OpenCASCADE CAD导入
│
├── mesh/                         # 网格生成
│   ├── mesh_engine.c
│   ├── mesh_engine.h             # 网格引擎
│   ├── mesh_pipeline.c
│   ├── mesh_pipeline.h           # 网格生成流水线
│   ├── mesh_subsystem.c
│   ├── mesh_subsystem.h          # 网格子系统
│   ├── mesh_algorithms.c         # 网格算法
│   ├── core_mesh.h               # 统一网格接口
│   ├── cad_mesh_generation.c
│   ├── cad_mesh_generation.h      # CAD网格生成
│   ├── triangular_mesh_peec.c    # PEEC三角网格
│   ├── manhattan_mesh_peec.c     # PEEC Manhattan网格
│   ├── triangular_mesh_solver.c  # 求解器三角网格
│   ├── cgal_surface_mesh.cpp     # CGAL表面网格
│   ├── cgal_surface_mesh_enhanced.cpp
│   ├── cgal_mesh_engine.h
│   ├── gmsh_surface_mesh.cpp     # Gmsh表面网格
│   ├── gmsh_surface_mesh.h
│   ├── clipper2_triangle_2d.cpp # Clipper2 2D三角网格
│   └── clipper2_triangle_2d.h
│
└── basis/                        # 基函数
    ├── rwg_basis.c
    ├── rwg_basis.h               # RWG基函数
    ├── rooftop_basis.c
    ├── rooftop_basis.h           # Rooftop基函数
    ├── higher_order_basis.c
    ├── higher_order_basis.h      # 高阶基函数
    ├── core_basis_functions.c
    └── core_basis_functions.h    # 核心基函数接口
```

**文件统计**: 3个子目录，38个文件（17个.h，16个.c，5个.cpp）

**关键文件说明**:
- `core_geometry.h`: 核心几何操作接口
- `mesh_engine.h`: 网格生成引擎
- `rwg_basis.h`: RWG基函数定义（MoM标准）

---

### 2.3 L3 算子/更新方程层 (`src/operators/`)

**职责**: 定义算子形式（积分算子、耦合算子等）

#### 目录结构
```
src/operators/
├── kernels/                      # 积分核
│   ├── core_kernels.c
│   ├── core_kernels.h            # 核心积分核
│   ├── electromagnetic_kernels.c
│   ├── electromagnetic_kernels.h # 电磁积分核
│   ├── electromagnetic_kernel_library.c
│   ├── electromagnetic_kernel_library.h # 电磁核函数库
│   ├── mom_kernel.c
│   ├── mom_kernel.h              # MoM积分核
│   ├── peec_kernel.c
│   ├── peec_kernel.h             # PEEC积分核
│   ├── kernel_cfie.c
│   ├── kernel_cfie.h             # CFIE核函数
│   ├── kernel_mfie.c
│   ├── kernel_mfie.h             # MFIE核函数
│   ├── kernel_cavity_waveguide.c
│   ├── kernel_cavity_waveguide.h # 腔体/波导核函数
│   ├── greens_function.c
│   ├── greens_function.h         # Green函数
│   ├── windowed_greens_function.c
│   ├── windowed_greens_function.h # 窗函数Green函数
│   ├── core_solver_electromagnetic_kernels.c
│   ├── electromagnetic_kernels_solver.c
│   └── electromagnetic_kernels_solver_compat.h
│
├── integration/                  # 积分计算
│   ├── integration_utils.c
│   ├── integration_utils.h       # 积分工具函数
│   ├── integration_utils_optimized.c
│   ├── integration_utils_optimized.h # 优化积分工具
│   ├── integral_nearly_singular.c
│   ├── integral_nearly_singular.h # 近奇异积分
│   ├── integral_logarithmic_singular.c
│   ├── integral_logarithmic_singular.h # 对数奇异积分
│   ├── singular_integration.c
│   ├── singular_integration.h    # 奇异积分统一接口
│   ├── industrial_singular_integration.h
│   └── periodic_ewald.c          # 周期Ewald方法
│   └── periodic_ewald.h
│
├── greens/                       # Green函数
│   ├── layered_greens_function.h
│   ├── layered_greens_function_unified.c # 分层介质Green函数
│   ├── layered_greens_function_optimized.h
│   └── advanced_layered_greens_function.h
│
├── assembler/                    # 矩阵组装
│   ├── core_assembler.c
│   ├── core_assembler.h          # 核心组装器
│   ├── matrix_assembler.c
│   └── matrix_assembler.h        # 矩阵组装器
│
├── matvec/                       # 矩阵向量积
│   ├── matvec_operator.c
│   └── matvec_operator.h          # 矩阵向量积算子
│
└── coupling/                     # 耦合算子
    ├── coupling_operator.c
    ├── coupling_operator.h       # 耦合算子
    ├── quasistatic_coupling.c
    └── quasistatic_coupling.h    # 准静态耦合
```

**文件统计**: 6个子目录，50个文件（26个.h，24个.c）

**关键文件说明**:
- `mom_kernel.h`: MoM积分核函数
- `integration_utils.h`: 积分计算工具
- `matrix_assembler.h`: 矩阵组装接口

---

### 2.4 L4 数值后端层 (`src/backend/`)

**职责**: 高效计算已定义的算子（HOW - 如何高效计算）

#### 目录结构
```
src/backend/
├── solvers/                      # 数值求解器
│   ├── solver_interface.c
│   ├── solver_interface.h        # 统一求解器接口
│   ├── core_solver.c
│   ├── core_solver.h             # 核心求解器
│   ├── direct_solver.c
│   ├── direct_solver.h           # 直接求解器（LU, QR, Cholesky）
│   ├── iterative_solver.c
│   ├── iterative_solver.h        # 迭代求解器（GMRES, CG, BiCGSTAB）
│   ├── iterative_solver_optimized.c
│   ├── sparse_direct_solver.c
│   ├── sparse_direct_solver.h    # 稀疏直接求解器
│   ├── sparse_direct_solver_core.c
│   ├── sparse_direct_solver_core.h
│   ├── out_of_core_solver.c
│   └── out_of_core_solver.h      # 外存求解器
│
├── algorithms/                   # 数值算法
│   ├── fast/                     # 快速算法
│   │   ├── aca.c
│   │   ├── aca.h                 # 自适应交叉近似（ACA）
│   │   ├── h_matrix_compression.c
│   │   ├── h_matrix_compression.h # H-矩阵压缩
│   │   ├── h_matrix_compression_optimized.c
│   │   ├── h_matrix_advanced_compression.h
│   │   ├── hmatrix.c
│   │   └── hmatrix.h             # H-矩阵接口
│   │
│   ├── adaptive/                 # 自适应算法
│   │   ├── adaptive_calculation.c
│   │   ├── adaptive_calculation.h # 自适应计算
│   │   ├── adaptive_optimization.c
│   │   └── adaptive_optimization.h
│   │
│   ├── fast_multipole_algorithm.h # 快速多极算法
│   ├── structure_algorithms.c
│   └── structure_algorithms.h   # 结构算法
│
├── gpu/                          # GPU加速
│   ├── gpu_acceleration.c
│   ├── gpu_acceleration.h        # GPU加速接口
│   ├── gpu_kernels.cu
│   ├── gpu_kernels.h             # GPU核函数
│   ├── gpu_linear_algebra.c
│   ├── gpu_linear_algebra.h     # GPU线性代数
│   ├── gpu_linalg_optimization.c
│   ├── gpu_linalg_optimization.h
│   ├── gpu_parallelization_optimized.cu
│   ├── gpu_parallelization_optimized.h
│   ├── multi_gpu_distribution.c
│   └── multi_gpu_distribution.h  # 多GPU分布
│
├── math/                         # 数学后端
│   ├── blas_interface.c
│   ├── blas_interface.h          # BLAS接口
│   ├── math_backend_selector.h   # 数学后端选择器
│   ├── math_backend_implementation.c
│   ├── industrial_solver_abstraction.h
│   ├── unified_matrix_assembly.c
│   └── unified_matrix_assembly.h # 统一矩阵组装
│
├── memory/                       # 内存管理
│   ├── memory_pool.c
│   ├── memory_pool.h             # 内存池
│   ├── memory_pool_optimized.c
│   ├── memory_optimization.c
│   └── memory_optimization.h     # 内存优化
│
└── optimization/                 # 优化
    └── core_optimization.c       # 核心优化
```

**文件统计**: 6个子目录，约50+个文件

**关键文件说明**:
- `solver_interface.h`: 统一求解器接口（L4层核心）
- `iterative_solver.h`: 迭代求解器（GMRES, CG等）
- `aca.h`: 自适应交叉近似（快速算法）

---

### 2.5 L5 执行编排层

#### 2.5.1 `src/orchestration/` - 编排核心

**职责**: 执行顺序、多物理耦合、数据流管理

```
src/orchestration/
├── hybrid_solver/                # 混合求解器编排
│   ├── hybrid_solver.c
│   ├── hybrid_solver.h           # 混合求解器接口
│   ├── coupling_manager.c
│   ├── coupling_manager.h        # 耦合管理器
│   └── mtl_coupling/             # MTL耦合
│       ├── mtl_hybrid_coupling.c
│       └── mtl_hybrid_coupling.h
│
├── workflow/                     # 工作流编排
│   ├── workflow_engine.c
│   ├── workflow_engine.h         # 工作流引擎
│   └── pcb/                      # PCB工作流
│       ├── pcb_simulation_workflow.c
│       └── pcb_simulation_workflow.h
│
├── execution/                    # 执行管理
│   ├── execution_order.c
│   ├── execution_order.h         # 执行顺序
│   ├── data_flow.c
│   └── data_flow.h               # 数据流管理
│
├── multiphysics/                 # 多物理耦合
│   ├── multiphysics_coupling.c
│   └── multiphysics_coupling.h   # 多物理耦合
│
├── checkpoint/                   # 检查点/恢复
│   ├── checkpoint_recovery.c
│   └── checkpoint_recovery.h     # 检查点恢复
│
└── wideband/                     # 宽带仿真
    ├── wideband_simulation_optimization.c
    ├── wideband_simulation_optimization.h
    └── core_wideband.h
```

**文件统计**: 6个子目录，21个文件（11个.h，10个.c）

#### 2.5.2 `src/solvers/` - 特定物理方法求解器编排

**职责**: 编排特定物理方法的完整求解流程

```
src/solvers/
├── mom/                          # MoM求解器编排
│   ├── mom_solver.h              # MoM求解器接口
│   ├── mom_solver_unified.c      # 统一MoM求解器（算法选择、矩阵组装、调用L4）
│   ├── mom_solver_min.c          # 最小MoM求解器
│   ├── mom_solver_module.h       # MoM求解器模块
│   ├── mom_aca.c                 # MoM ACA加速
│   ├── mom_aca.h
│   ├── mom_hmatrix.c             # MoM H-矩阵
│   ├── mom_hmatrix.h
│   ├── mom_mlfmm.c               # MoM MLFMM
│   ├── mom_mlfmm.h
│   ├── mom_matvec.c              # MoM矩阵向量积
│   ├── mom_matvec.h
│   ├── mom_layered_medium.c      # MoM分层介质
│   ├── mom_layered_medium.h
│   ├── mom_time_domain.c         # MoM时域
│   ├── mom_time_domain.h
│   ├── mom_fullwave_solver.c
│   └── mom_fullwave_solver.h
│
├── peec/                         # PEEC求解器编排
│   ├── peec_solver.h             # PEEC求解器接口
│   ├── peec_solver_unified.c     # 统一PEEC求解器
│   ├── peec_solver_min.c         # 最小PEEC求解器
│   ├── peec_solver_module.h      # PEEC求解器模块
│   ├── peec_integration.c        # PEEC积分
│   ├── peec_integration.h
│   ├── peec_directionality_kernels.c
│   ├── peec_directionality_kernels.h
│   ├── peec_geometry_support.c
│   ├── peec_geometry_support.h
│   ├── peec_non_manhattan_geometry.c
│   ├── peec_non_manhattan_geometry.h
│   ├── peec_layered_medium.c
│   ├── peec_layered_medium.h
│   ├── peec_materials_enhanced.c
│   ├── peec_materials_enhanced.h
│   ├── peec_via_modeling.c
│   ├── peec_via_modeling.h
│   ├── peec_plane_wave.c
│   ├── peec_plane_wave.h
│   ├── peec_satellite.c
│   ├── peec_satellite.h
│   ├── peec_statistical.c
│   ├── peec_time_domain.c
│   ├── peec_time_domain.h
│   ├── peec_nonlinear.c
│   └── peec_advanced.h
│
└── mtl/                          # MTL求解器编排
    ├── mtl_solver_module.h       # MTL求解器模块
    ├── mtl_solver.c              # MTL求解器实现
    ├── mtl_parameter_import.c
    ├── mtl_parameter_import.h   # MTL参数导入
    ├── mtl_time_domain.c
    ├── mtl_time_domain.h         # MTL时域
    ├── mtl_wideband.c
    └── mtl_wideband.h            # MTL宽带
```

**文件统计**: 3个子目录，约50+个文件

**关键文件说明**:
- `mom_solver_unified.c`: MoM求解器编排（算法选择、矩阵组装、调用L4）
- `peec_solver_unified.c`: PEEC求解器编排
- `mtl_solver.c`: MTL求解器编排

---

### 2.6 L6 输入输出层 (`src/io/`)

**职责**: 文件I/O、用户接口、外部API

```
src/io/
├── file_formats/                 # 文件格式
│   ├── file_io.c
│   ├── file_io.h                # 通用文件I/O
│   ├── export_formats.c
│   ├── export_formats.h         # 导出格式
│   ├── export_hdf5.c
│   ├── export_hdf5.h            # HDF5导出
│   ├── export_vtk.c
│   ├── export_vtk.h             # VTK导出
│   ├── touchstone_io.c
│   ├── touchstone_io.h          # Touchstone格式
│   └── spice_io.c               # SPICE格式
│
├── api/                          # API接口
│   ├── c_api.c
│   └── c_api.h                  # C API
│
├── cli/                          # 命令行接口
│   ├── cli_main.c
│   └── cli_main.h               # CLI主程序
│
├── analysis/                     # 分析功能
│   ├── emc_analysis.c
│   ├── emc_analysis.h            # EMC分析
│   ├── enhanced_sparameter_extraction.c
│   └── enhanced_sparameter_extraction.h # S参数提取
│
├── postprocessing/               # 后处理
│   ├── field_postprocessing.c
│   └── field_postprocessing.h   # 场后处理
│
├── results/                      # 结果处理
│   ├── result_bundle.c
│   └── result_bundle.h          # 结果打包
│
├── pcb_file_io.c                # PCB文件I/O
├── pcb_file_io.h
├── advanced_file_formats.c
├── advanced_file_formats.h       # 高级文件格式
├── format_validation.c
├── format_validation.h           # 格式验证
├── parallel_io.c
├── parallel_io.h                 # 并行I/O
├── memory_optimization.c
└── memory_optimization.h          # I/O内存优化
```

**文件统计**: 6个子目录，33个文件（17个.c，16个.h）

---

### 2.7 公共层

#### 2.7.1 `src/common/` - 公共定义

**职责**: 公共类型、常量、错误处理、层间接口

```
src/common/
├── types.h                       # 公共类型定义
├── constants.h                   # 物理常数
├── core_common.h                 # 核心公共定义
├── errors.h                      # 错误定义
├── errors_core.c                 # 错误处理实现
├── errors_core.h
├── compatibility_adapter.h       # 兼容性适配器
└── layer_interfaces.h            # 层间接口定义
```

**文件统计**: 8个文件（7个.h，1个.c）

#### 2.7.2 `src/utils/` - 工具函数

**职责**: 通用工具函数

```
src/utils/
├── evaluation/                   # 评估指标
│   ├── quantitative_metrics.c
│   └── quantitative_metrics.h   # 定量评估指标
│
├── performance/                  # 性能监控
│   ├── performance_monitor.c
│   ├── performance_monitor.h     # 性能监控器
│   └── performance_monitoring_optimized.c
│
├── validation/                   # 验证功能
│   ├── commercial_validation.c
│   ├── commercial_validation.h  # 商业软件验证
│   └── industrial_validation_benchmarks.h
│
├── error_handler.h               # 错误处理
├── logger.h                      # 日志记录
└── memory_manager.h              # 内存管理
```

**文件统计**: 3个子目录，约10个文件

#### 2.7.3 `src/materials/` - 材料定义

**职责**: 材料库和材料定义

```
src/materials/
├── material_library.c
├── material_library.h           # 材料库
├── cst_materials_parser.c
└── cst_materials_parser.h       # CST材料解析器
```

**文件统计**: 4个文件（2个.c，2个.h）

#### 2.7.4 `src/modeling/` - 建模功能

**职责**: 电磁建模（将几何转换为EM模型）

```
src/modeling/
└── pcb/
    ├── pcb_electromagnetic_modeling.c
    └── pcb_electromagnetic_modeling.h  # PCB电磁建模
```

**文件统计**: 1个子目录，2个文件

#### 2.7.5 `src/applications/` - 应用层

**职责**: 特定应用功能

```
src/applications/
├── enclosure_calculation.c
├── enclosure_calculation.h     # 机箱计算
├── metamaterial_extraction.c
└── metamaterial_extraction.h    # 超材料提取
```

**文件统计**: 4个文件（2个.c，2个.h）

#### 2.7.6 `src/plugins/` - 插件框架

**职责**: 插件系统

```
src/plugins/
├── plugin_framework.c
├── plugin_framework.h           # 插件框架
├── mom/
│   └── mom_solver_plugin.cpp    # MoM求解器插件
└── peec/
    └── peec_solver_plugin.cpp   # PEEC求解器插件
```

**文件统计**: 2个子目录，4个文件

#### 2.7.7 `src/probes/` - 探测工具

**职责**: 第三方库探测

```
src/probes/
├── boost_probe.cpp              # Boost库探测
├── cgal_probe.cpp               # CGAL库探测
├── gmsh_probe.c                 # Gmsh库探测
└── openblas_probe.c             # OpenBLAS库探测
```

**文件统计**: 4个文件

---

## 三、目录层级关系图

```
src/
│
├── physics/ (L1) ────────────────┐
│                                 │
├── discretization/ (L2) ────────┤
│                                 │
├── operators/ (L3) ──────────────┤
│                                 │
├── backend/ (L4) ────────────────┤ 依赖方向: L6 → L5 → L4 → L3 → L2 → L1
│                                 │
├── orchestration/ (L5) ─────────┤
├── solvers/ (L5) ────────────────┤
│                                 │
├── io/ (L6) ────────────────────┘
│
├── common/ (公共层)
├── utils/ (公共层)
├── materials/ (公共层)
├── modeling/ (公共层)
├── applications/ (应用层)
├── plugins/ (插件层)
└── probes/ (工具层)
```

---

## 四、关键目录对比

### 4.1 `src/backend/solvers/` vs `src/solvers/`

| 维度 | `backend/solvers/` (L4) | `solvers/` (L5) |
|------|------------------------|-----------------|
| **层级** | L4 数值后端 | L5 执行编排 |
| **职责** | 通用线性系统求解算法 | 特定物理方法求解编排 |
| **输入** | 矩阵 + 右端项 | 几何 + 物理参数 + 配置 |
| **输出** | 解向量 | 完整的仿真结果 |
| **物理感知** | ❌ 无 | ✅ 有（MoM/PEEC/MTL特定） |
| **可复用性** | ✅ 高度可复用 | ⚠️ 特定方法专用 |
| **依赖关系** | 被L5层使用 | 使用L4层 |

### 4.2 目录命名说明

- `backend/solvers/`: L4层，通用数值算法（GMRES, CG, LU等）
- `solvers/`: L5层，特定物理方法编排（MoM, PEEC, MTL）
- 两者职责不同，不应合并

---

## 五、文件统计汇总

### 按层级统计

| 层级 | 目录 | 文件数（估算） | 主要文件类型 |
|------|------|--------------|-------------|
| L1 | `physics/` | ~16 | .c, .h |
| L2 | `discretization/` | ~38 | .c, .h, .cpp |
| L3 | `operators/` | ~50 | .c, .h |
| L4 | `backend/` | ~50+ | .c, .h, .cu |
| L5 | `orchestration/` + `solvers/` | ~70+ | .c, .h |
| L6 | `io/` | ~33 | .c, .h |
| 公共 | `common/`, `utils/`, `materials/`, `modeling/` | ~30 | .c, .h |
| 其他 | `applications/`, `plugins/`, `probes/` | ~10 | .c, .h, .cpp |

**总计**: 约300+个源文件

---

## 六、架构符合性检查

### ✅ 符合架构的部分

1. **层级分离清晰**: 各层职责明确，文件位置正确
2. **依赖方向正确**: L5 → L4 → L3 → L2 → L1 ✅
3. **目录命名合理**: 符合六层架构命名规范

### ⚠️ 需要注意的部分

1. **`physics/peec/circuit_coupling_simulation.c`**: 
   - 可能属于L5层（电路-EM耦合仿真）
   - 建议：进一步分析是否需要移动到 `orchestration/`

2. **`modeling/pcb/`**: 
   - 可能属于L2或L5层
   - 建议：明确其职责（建模 vs 编排）

---

## 七、文件命名规范

### 7.1 命名模式

- **核心文件**: `core_*.c/h` - 核心功能
- **统一接口**: `*_unified.c/h` - 统一实现
- **最小实现**: `*_min.c` - 最小功能实现
- **模块接口**: `*_module.h` - 模块接口
- **优化版本**: `*_optimized.c` - 优化实现

### 7.2 目录命名

- **物理方法**: `mom/`, `peec/`, `mtl/`
- **功能模块**: `kernels/`, `integration/`, `assembler/`
- **算法类型**: `fast/`, `adaptive/`

---

## 八、依赖关系图

```
┌─────────────────────────────────────────┐
│  L6: io/                                │
│  - 文件I/O, API, CLI                    │
└─────────────────────────────────────────┘
              ↓ 使用
┌─────────────────────────────────────────┐
│  L5: orchestration/ + solvers/          │
│  - 执行编排, 求解器编排                  │
└─────────────────────────────────────────┘
              ↓ 使用
┌─────────────────────────────────────────┐
│  L4: backend/                           │
│  - 数值求解器, GPU, 快速算法             │
└─────────────────────────────────────────┘
              ↓ 使用
┌─────────────────────────────────────────┐
│  L3: operators/                          │
│  - 积分核, 积分计算, 矩阵组装            │
└─────────────────────────────────────────┘
              ↓ 使用
┌─────────────────────────────────────────┐
│  L2: discretization/                     │
│  - 几何, 网格, 基函数                    │
└─────────────────────────────────────────┘
              ↓ 使用
┌─────────────────────────────────────────┐
│  L1: physics/                            │
│  - 物理方程, 边界条件, 材料属性           │
└─────────────────────────────────────────┘
```

---

## 九、快速查找指南

### 9.1 按功能查找

- **物理方程定义**: `src/physics/`
- **网格生成**: `src/discretization/mesh/`
- **积分核函数**: `src/operators/kernels/`
- **数值求解器**: `src/backend/solvers/`
- **MoM求解器**: `src/solvers/mom/`
- **PEEC求解器**: `src/solvers/peec/`
- **文件I/O**: `src/io/file_formats/`

### 9.2 按层级查找

- **L1层**: `src/physics/`
- **L2层**: `src/discretization/`
- **L3层**: `src/operators/`
- **L4层**: `src/backend/`
- **L5层**: `src/orchestration/`, `src/solvers/`
- **L6层**: `src/io/`

---

## 十、维护建议

1. **新增文件**: 确保放在正确的层级目录
2. **移动文件**: 更新所有引用路径
3. **删除文件**: 检查是否有依赖
4. **架构验证**: 定期检查层间依赖关系

---

## 附录：完整文件列表

### A.1 L1层文件列表

详见 `src/physics/` 目录（16个文件）

### A.2 L2层文件列表

详见 `src/discretization/` 目录（38个文件）

### A.3 L3层文件列表

详见 `src/operators/` 目录（50个文件）

### A.4 L4层文件列表

详见 `src/backend/` 目录（50+个文件）

### A.5 L5层文件列表

详见 `src/orchestration/` 和 `src/solvers/` 目录（70+个文件）

### A.6 L6层文件列表

详见 `src/io/` 目录（33个文件）

---

---

## 十一、目录索引（按字母顺序）

### A
- `applications/` - 应用层功能
- `api/` (在 `io/` 下) - C API接口

### B
- `backend/` - L4数值后端层
- `basis/` (在 `discretization/` 下) - 基函数

### C
- `checkpoint/` (在 `orchestration/` 下) - 检查点/恢复
- `cli/` (在 `io/` 下) - 命令行接口
- `common/` - 公共定义
- `coupling/` (在 `operators/` 下) - 耦合算子

### D
- `discretization/` - L2离散与建模层

### E
- `execution/` (在 `orchestration/` 下) - 执行管理
- `excitation/` (在 `physics/` 下) - 激励源

### F
- `fast/` (在 `backend/algorithms/` 下) - 快速算法
- `file_formats/` (在 `io/` 下) - 文件格式

### G
- `geometry/` (在 `discretization/` 下) - 几何处理
- `gpu/` (在 `backend/` 下) - GPU加速
- `greens/` (在 `operators/` 下) - Green函数

### H
- `hybrid/` (在 `physics/` 下) - 混合物理模型
- `hybrid_solver/` (在 `orchestration/` 下) - 混合求解器编排

### I
- `integration/` (在 `operators/` 下) - 积分计算
- `io/` - L6输入输出层

### K
- `kernels/` (在 `operators/` 下) - 积分核

### M
- `materials/` - 材料定义
- `matvec/` (在 `operators/` 下) - 矩阵向量积
- `mesh/` (在 `discretization/` 下) - 网格生成
- `modeling/` - 建模功能
- `mom/` (在 `physics/` 和 `solvers/` 下) - MoM相关
- `mtl/` (在 `physics/` 和 `solvers/` 下) - MTL相关
- `multiphysics/` (在 `orchestration/` 下) - 多物理耦合

### O
- `operators/` - L3算子/更新方程层
- `orchestration/` - L5执行编排层（核心）

### P
- `peec/` (在 `physics/` 和 `solvers/` 下) - PEEC相关
- `performance/` (在 `utils/` 下) - 性能监控
- `physics/` - L1物理定义层
- `plugins/` - 插件框架
- `postprocessing/` (在 `io/` 下) - 后处理
- `probes/` - 第三方库探测

### R
- `results/` (在 `io/` 下) - 结果处理

### S
- `solvers/` - L5执行编排层（特定物理方法）

### U
- `utils/` - 工具函数

### V
- `validation/` (在 `utils/` 下) - 验证功能

### W
- `wideband/` (在 `orchestration/` 下) - 宽带仿真
- `workflow/` (在 `orchestration/` 下) - 工作流编排

---

## 十二、文件类型统计

### 按文件扩展名

| 扩展名 | 数量（估算） | 用途 |
|--------|------------|------|
| `.c` | ~200+ | C源文件 |
| `.h` | ~200+ | C头文件 |
| `.cpp` | ~10+ | C++源文件 |
| `.cu` | ~5+ | CUDA源文件 |
| `.hpp` | ~5+ | C++头文件 |

### 按功能分类

| 功能类别 | 文件数（估算） | 主要目录 |
|---------|--------------|---------|
| 物理定义 | ~16 | `physics/` |
| 离散化 | ~38 | `discretization/` |
| 算子 | ~50 | `operators/` |
| 数值后端 | ~50+ | `backend/` |
| 编排 | ~70+ | `orchestration/`, `solvers/` |
| I/O | ~33 | `io/` |
| 公共/工具 | ~40+ | `common/`, `utils/`, `materials/` |

---

## 十三、架构迁移历史

### 从 `core/` 目录迁移的文件

根据之前的重构，以下文件已从 `src/core/` 迁移到新位置：

| 原路径 | 新路径 | 层级 |
|--------|--------|------|
| `core/core_geometry.*` | `discretization/geometry/core_geometry.*` | L2 |
| `core/core_kernels.*` | `operators/kernels/core_kernels.*` | L3 |
| `core/core_mesh.h` | `discretization/mesh/core_mesh.h` | L2 |
| `core/core_mesh_unified.c` | 合并到 `discretization/mesh/mesh_engine.c` | L2 |
| `core/core_solver.*` | `backend/solvers/core_solver.*` | L4 |
| `core/electromagnetic_kernels.*` | `operators/kernels/electromagnetic_kernels.*` | L3 |
| `core/integral_*.c/h` | `operators/integration/integral_*.c/h` | L3 |
| `core/kernel_*.c/h` | `operators/kernels/kernel_*.c/h` | L3 |
| `core/layered_greens_function.*` | `operators/greens/layered_greens_function.*` | L3 |
| `core/windowed_greens_function.*` | `operators/kernels/windowed_greens_function.*` | L3 |
| `core/periodic_ewald.*` | `operators/integration/periodic_ewald.*` | L3 |

**注意**: `src/core/` 目录已不再使用，所有文件已迁移到相应的架构层。

---

## 十四、依赖关系矩阵

### 层间依赖（允许的依赖方向）

| 从 \ 到 | L1 | L2 | L3 | L4 | L5 | L6 |
|---------|----|----|----|----|----|----|
| L1 | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| L2 | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ |
| L3 | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ |
| L4 | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ |
| L5 | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| L6 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

**规则**: 低层不能依赖高层，只能依赖同层或更低层。

---

## 十五、维护指南

### 15.1 添加新文件

1. **确定层级**: 根据文件功能确定属于哪一层
2. **选择目录**: 在对应层级的合适子目录中创建文件
3. **命名规范**: 遵循现有命名规范
4. **更新文档**: 更新本文档

### 15.2 移动文件

1. **检查依赖**: 使用 `grep` 查找所有引用
2. **更新路径**: 更新所有 `#include` 路径
3. **更新项目文件**: 更新 `.vcxproj` 文件
4. **验证编译**: 确保编译通过

### 15.3 删除文件

1. **检查依赖**: 确认没有其他文件依赖
2. **更新引用**: 删除或更新所有引用
3. **更新文档**: 更新本文档

---

**文档版本**: 1.0  
**最后更新**: 2025-01-XX  
**维护者**: PulseMoM开发团队  
**相关文档**: 
- `.claude/ARCHITECTURE_GUARD.md` - 架构守护文档
- `docs/SOLVERS_DIRECTORY_ANALYSIS.md` - Solvers目录分析
- `docs/ARCHITECTURE_VIOLATION_FIX.md` - 架构违规修复报告
