# PulseMoM 目录树形结构视图

## 快速导航

本文档提供项目的树形目录结构，便于快速浏览和查找文件。

---

## 完整目录树

```
src/
│
├── 📁 applications/                    # 应用层
│   ├── enclosure_calculation.c/h
│   └── metamaterial_extraction.c/h
│
├── 📁 backend/                         # L4 数值后端层
│   ├── 📁 algorithms/
│   │   ├── 📁 adaptive/
│   │   │   ├── adaptive_calculation.c/h
│   │   │   └── adaptive_optimization.c/h
│   │   ├── 📁 fast/
│   │   │   ├── aca.c/h
│   │   │   ├── h_matrix_compression.c/h
│   │   │   └── hmatrix.c/h
│   │   ├── fast_multipole_algorithm.h
│   │   └── structure_algorithms.c/h
│   │
│   ├── 📁 gpu/
│   │   ├── gpu_acceleration.c/h
│   │   ├── gpu_kernels.cu/h
│   │   ├── gpu_linear_algebra.c/h
│   │   └── multi_gpu_distribution.c/h
│   │
│   ├── 📁 math/
│   │   ├── blas_interface.c/h
│   │   ├── math_backend_selector.h
│   │   └── unified_matrix_assembly.c/h
│   │
│   ├── 📁 memory/
│   │   ├── memory_pool.c/h
│   │   └── memory_optimization.c/h
│   │
│   ├── 📁 optimization/
│   │   └── core_optimization.c
│   │
│   └── 📁 solvers/                     # L4 数值求解器
│       ├── solver_interface.c/h
│       ├── core_solver.c/h
│       ├── direct_solver.c/h
│       ├── iterative_solver.c/h
│       ├── sparse_direct_solver.c/h
│       └── out_of_core_solver.c/h
│
├── 📁 common/                          # 公共定义
│   ├── types.h
│   ├── constants.h
│   ├── core_common.h
│   ├── errors.h
│   └── layer_interfaces.h
│
├── 📁 discretization/                   # L2 离散与建模层
│   ├── 📁 basis/                       # 基函数
│   │   ├── rwg_basis.c/h
│   │   ├── rooftop_basis.c/h
│   │   ├── higher_order_basis.c/h
│   │   └── core_basis_functions.c/h
│   │
│   ├── 📁 geometry/                    # 几何处理
│   │   ├── core_geometry.c/h
│   │   ├── geometry_engine.c/h
│   │   ├── pcb_ic_structures.c/h
│   │   ├── port_support_extended.c/h
│   │   └── opencascade_cad_import.cpp/h
│   │
│   └── 📁 mesh/                        # 网格生成
│       ├── mesh_engine.c/h
│       ├── mesh_pipeline.c/h
│       ├── core_mesh.h
│       ├── cad_mesh_generation.c/h
│       ├── triangular_mesh_peec.c
│       ├── manhattan_mesh_peec.c
│       ├── cgal_surface_mesh.cpp
│       └── gmsh_surface_mesh.cpp/h
│
├── 📁 io/                              # L6 输入输出层
│   ├── 📁 file_formats/
│   │   ├── file_io.c/h
│   │   ├── export_hdf5.c/h
│   │   ├── export_vtk.c/h
│   │   └── touchstone_io.c/h
│   │
│   ├── 📁 api/
│   │   └── c_api.c/h
│   │
│   ├── 📁 cli/
│   │   └── cli_main.c/h
│   │
│   ├── 📁 analysis/
│   │   ├── emc_analysis.c/h
│   │   └── enhanced_sparameter_extraction.c/h
│   │
│   ├── 📁 postprocessing/
│   │   └── field_postprocessing.c/h
│   │
│   ├── 📁 results/
│   │   └── result_bundle.c/h
│   │
│   ├── pcb_file_io.c/h
│   └── advanced_file_formats.c/h
│
├── 📁 materials/                       # 材料定义
│   ├── material_library.c/h
│   └── cst_materials_parser.c/h
│
├── 📁 modeling/                        # 建模功能
│   └── 📁 pcb/
│       └── pcb_electromagnetic_modeling.c/h
│
├── 📁 operators/                       # L3 算子/更新方程层
│   ├── 📁 kernels/                     # 积分核
│   │   ├── core_kernels.c/h
│   │   ├── electromagnetic_kernels.c/h
│   │   ├── mom_kernel.c/h
│   │   ├── peec_kernel.c/h
│   │   ├── kernel_cfie.c/h
│   │   ├── kernel_mfie.c/h
│   │   ├── kernel_cavity_waveguide.c/h
│   │   ├── greens_function.c/h
│   │   └── windowed_greens_function.c/h
│   │
│   ├── 📁 integration/                 # 积分计算
│   │   ├── integration_utils.c/h
│   │   ├── integral_nearly_singular.c/h
│   │   ├── integral_logarithmic_singular.c/h
│   │   ├── singular_integration.c/h
│   │   └── periodic_ewald.c/h
│   │
│   ├── 📁 greens/                      # Green函数
│   │   ├── layered_greens_function.h
│   │   └── layered_greens_function_unified.c
│   │
│   ├── 📁 assembler/                   # 矩阵组装
│   │   ├── core_assembler.c/h
│   │   └── matrix_assembler.c/h
│   │
│   ├── 📁 matvec/                      # 矩阵向量积
│   │   └── matvec_operator.c/h
│   │
│   └── 📁 coupling/                    # 耦合算子
│       ├── coupling_operator.c/h
│       └── quasistatic_coupling.c/h
│
├── 📁 orchestration/                   # L5 执行编排层（核心）
│   ├── 📁 hybrid_solver/               # 混合求解器编排
│   │   ├── hybrid_solver.c/h
│   │   ├── coupling_manager.c/h
│   │   └── 📁 mtl_coupling/
│   │       └── mtl_hybrid_coupling.c/h
│   │
│   ├── 📁 workflow/                    # 工作流编排
│   │   ├── workflow_engine.c/h
│   │   └── 📁 pcb/
│   │       └── pcb_simulation_workflow.c/h
│   │
│   ├── 📁 execution/                   # 执行管理
│   │   ├── execution_order.c/h
│   │   └── data_flow.c/h
│   │
│   ├── 📁 multiphysics/                # 多物理耦合
│   │   └── multiphysics_coupling.c/h
│   │
│   ├── 📁 checkpoint/                  # 检查点/恢复
│   │   └── checkpoint_recovery.c/h
│   │
│   └── 📁 wideband/                    # 宽带仿真
│       └── wideband_simulation_optimization.c/h
│
├── 📁 physics/                         # L1 物理定义层
│   ├── 📁 mom/                         # MoM物理模型
│   │   └── mom_physics.c/h
│   │
│   ├── 📁 peec/                        # PEEC物理模型
│   │   ├── peec_physics.c/h
│   │   ├── peec_circuit.c/h
│   │   └── peec_circuit_coupling.h
│   │
│   ├── 📁 mtl/                         # MTL物理模型
│   │   └── mtl_physics.c/h
│   │
│   ├── 📁 excitation/                  # 激励源
│   │   └── excitation_plane_wave.c/h
│   │
│   ├── 📁 hybrid/                      # 混合物理模型
│   │   └── hybrid_physics_boundary.c/h
│   │
│   └── 📁 materials/                   # 材料物理模型
│       └── advanced_models.c/h
│
├── 📁 plugins/                         # 插件框架
│   ├── plugin_framework.c/h
│   ├── 📁 mom/
│   │   └── mom_solver_plugin.cpp
│   └── 📁 peec/
│       └── peec_solver_plugin.cpp
│
├── 📁 probes/                          # 第三方库探测
│   ├── boost_probe.cpp
│   ├── cgal_probe.cpp
│   ├── gmsh_probe.c
│   └── openblas_probe.c
│
├── 📁 solvers/                         # L5 执行编排层（特定物理方法）
│   ├── 📁 mom/                         # MoM求解器编排
│   │   ├── mom_solver.h
│   │   ├── mom_solver_unified.c        # 统一MoM求解器
│   │   ├── mom_solver_min.c
│   │   ├── mom_aca.c/h
│   │   ├── mom_hmatrix.c/h
│   │   ├── mom_mlfmm.c/h
│   │   └── mom_time_domain.c/h
│   │
│   ├── 📁 peec/                        # PEEC求解器编排
│   │   ├── peec_solver.h
│   │   ├── peec_solver_unified.c       # 统一PEEC求解器
│   │   ├── peec_integration.c/h
│   │   ├── peec_non_manhattan_geometry.c/h
│   │   ├── peec_via_modeling.c/h
│   │   └── peec_time_domain.c/h
│   │
│   └── 📁 mtl/                         # MTL求解器编排
│       ├── mtl_solver_module.h
│       ├── mtl_solver.c
│       ├── mtl_parameter_import.c/h
│       └── mtl_wideband.c/h
│
└── 📁 utils/                           # 工具函数
    ├── 📁 evaluation/
    │   └── quantitative_metrics.c/h
    │
    ├── 📁 performance/
    │   ├── performance_monitor.c/h
    │   └── performance_monitoring_optimized.c
    │
    └── 📁 validation/
        ├── commercial_validation.c/h
        └── industrial_validation_benchmarks.h
```

---

## 层级标注

- **L1** = 物理定义层
- **L2** = 离散与建模层
- **L3** = 算子/更新方程层
- **L4** = 数值后端层
- **L5** = 执行编排层
- **L6** = 输入输出层

---

## 文件统计（估算）

| 层级 | 目录 | 文件数 |
|------|------|--------|
| L1 | `physics/` | ~16 |
| L2 | `discretization/` | ~38 |
| L3 | `operators/` | ~50 |
| L4 | `backend/` | ~50+ |
| L5 | `orchestration/` + `solvers/` | ~70+ |
| L6 | `io/` | ~33 |
| 公共 | `common/`, `utils/`, `materials/`, `modeling/` | ~30 |
| 其他 | `applications/`, `plugins/`, `probes/` | ~10 |

**总计**: 约300+个源文件

---

## 快速查找

### 按功能查找

- **物理方程**: `physics/`
- **网格生成**: `discretization/mesh/`
- **积分核**: `operators/kernels/`
- **数值求解**: `backend/solvers/`
- **MoM求解**: `solvers/mom/`
- **PEEC求解**: `solvers/peec/`
- **文件I/O**: `io/file_formats/`

### 按层级查找

- **L1**: `physics/`
- **L2**: `discretization/`
- **L3**: `operators/`
- **L4**: `backend/`
- **L5**: `orchestration/`, `solvers/`
- **L6**: `io/`

---

**相关文档**: `docs/DIRECTORY_STRUCTURE_DETAILED.md` - 详细目录结构文档
