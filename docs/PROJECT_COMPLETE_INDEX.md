# PulseMoM 工程完整索引

# PulseMoM 工程完整索引

> **快速导航**:
> - [混合求解器详细索引](./HYBRID_SOLVER_MODULE_INDEX.md) - 混合求解器模块的详细说明
> - 本文档提供整个工程的完整索引

## 工程概述

**项目名称**: PulseMoM (PEEC-MoM Unified Framework)  
**项目类型**: 电磁仿真统一框架  
**开发语言**: C/C++  
**构建系统**: CMake + Visual Studio  
**版本**: 1.0.0  
**版权**: Copyright (C) 2024-2025 PulseEM Technologies

### 核心功能
- **MoM (Method of Moments)**: 矩量法电磁仿真
- **PEEC (Partial Element Equivalent Circuit)**: 部分等效电路法
- **MTL (Multiconductor Transmission Lines)**: 多导体传输线
- **混合求解器**: 支持MoM-PEEC-MTL三路耦合
- **GPU加速**: CUDA/OpenCL支持
- **CAD集成**: CGAL, Gmsh, OpenCASCADE支持
- **工业级验证**: 商业软件对比验证

---

## 1. 目录结构

### 1.1 根目录
```
PulseMoM/
├── src/                    # 源代码目录
├── apps/                   # 应用程序
├── examples/               # 示例代码
├── tests/                  # 测试代码
├── docs/                   # 文档
├── build/                  # 构建输出
├── libs/                   # 第三方库
├── library/                # 库文件
├── python/                 # Python接口
├── python_interface/       # Python绑定
├── ref_code/               # 参考代码
├── reports/                # 报告文件
├── CMakeLists.txt          # CMake构建配置
├── vcpkg.json             # vcpkg依赖配置
└── *.bat                   # Windows构建脚本
```

### 1.2 源代码目录 (src/)
```
src/
├── core/                   # 核心模块
├── solvers/                # 求解器模块
│   ├── mom/               # MoM求解器
│   ├── peec/              # PEEC求解器
│   ├── mtl/               # MTL求解器
│   └── hybrid/            # 混合求解器
├── mesh/                   # 网格生成
├── io/                     # 输入输出
├── materials/              # 材料处理
├── cad/                    # CAD集成
├── geometry/               # 几何处理
├── math/                   # 数学库
├── computation/            # 计算算法
├── evaluation/             # 评估工具
├── validation/             # 验证工具
├── performance/            # 性能监控
├── utils/                  # 工具函数
├── api/                    # API接口
├── plugins/                 # 插件框架
├── workflows/              # 工作流
├── modeling/               # 建模工具
├── c_interface/            # C接口
├── applications/           # 应用模块
├── probes/                 # 探测工具
├── gui/                    # 图形界面
└── main.c                  # 主入口
```

---

## 2. 核心模块 (core/)

### 2.1 基础定义
**文件**: `core_common.h`, `core_common.c`
- 基本数据类型定义 (`real_t`, `complex_t`, `point3d_t`)
- 物理常数 (`C0`, `MU0`, `EPS0`, `ETA0`)
- 数值容差常量
- 几何结构 (`triangle_t`, `edge_t`, `rwg_basis_t`)
- 频率域结构 (`frequency_domain_t`)

### 2.2 几何处理
**文件**: `core_geometry.h/c`, `core_geometry_min.c`
- 几何体定义和操作
- 点、线、面处理
- 空间索引
- 几何变换

### 2.3 网格处理
**文件**: `core_mesh.h`, `core_mesh_pipeline.c/h`, `core_mesh_unified.c`, `core_mesh_advanced.h`
- 网格数据结构
- 网格生成管道
- 网格优化
- 网格验证

### 2.4 基函数
**文件**: `basis_functions.h/c`, `higher_order_basis.h/c`
- RWG基函数
- 高阶基函数
- 基函数计算和评估

### 2.5 电磁核函数
**文件**: `electromagnetic_kernels.h/c`, `electromagnetic_kernel_library.h/c`
- 自由空间格林函数
- 分层介质格林函数
- 核函数计算
- 核函数库管理

**关键模块**:
- `kernel_cfie.h/c`: 组合场积分方程 (CFIE)
- `kernel_mfie.h/c`: 磁场积分方程 (MFIE)
- `kernel_cavity_waveguide.h/c`: 腔体和波导核函数

### 2.6 格林函数
**文件**: 
- `layered_greens_function.h`, `layered_greens_function_unified.c`
- `advanced_layered_greens_function.h`
- `windowed_greens_function.h/c`
- `periodic_ewald.c`

**功能**:
- 分层介质格林函数
- Sommerfeld积分
- DCIM (Discrete Complex Image Method)
- TMM (Transfer Matrix Method)
- 周期结构格林函数 (Ewald方法)

### 2.7 积分计算
**文件**:
- `integration_utils.h/c`
- `integration_utils_optimized.h/c`
- `integral_nearly_singular.h/c`
- `integral_logarithmic_singular.h/c`
- `industrial_singular_integration.h`

**功能**:
- 奇异积分处理
- 近奇异积分
- 对数奇异积分
- 数值积分优化
- 工业级精度积分

### 2.8 矩阵组装
**文件**: `core_assembler.h/c`
- 阻抗矩阵组装
- 激励向量组装
- 矩阵填充优化

### 2.9 求解器
**文件**:
- `core_solver.h/c`
- `core_solver_sparse_direct.h/c`
- `iterative_solver_optimized.c`
- `sparse_direct_solver.h/c`
- `unified_solver_interface.h/c`

**功能**:
- 统一求解器接口
- 直接求解器 (LU分解)
- 迭代求解器 (GMRES, BiCGSTAB)
- 稀疏矩阵求解
- 核外求解器 (`out_of_core_solver.h/c`)

### 2.10 矩阵压缩
**文件**:
- `h_matrix_compression.h/c`
- `h_matrix_compression_optimized.c`
- `h_matrix_advanced_compression.h`

**功能**:
- H-矩阵压缩
- ACA (Adaptive Cross Approximation)
- 低秩近似
- 矩阵压缩优化

### 2.11 快速算法
**文件**: `algorithms/fast_multipole_algorithm.h/c`
- 快速多极子方法 (FMM)
- MLFMM (Multi-Level Fast Multipole Method)
- 算法优化

### 2.12 自适应算法
**文件**: `algorithms/adaptive/adaptive_calculation.h/c`, `algorithms/adaptive/adaptive_optimization.h/c`
- 自适应计算
- 自适应优化
- 误差估计
- 自适应细化

### 2.13 结构算法
**文件**: `algorithms/structure_algorithms.h/c`
- 结构分析算法
- 算法优化

### 2.14 后处理
**文件**:
- `postprocessing_field.h/c`
- `export_formats.h/c`
- `export_vtk.h/c`
- `export_hdf5.h/c`
- `touchstone_export.h/c`
- `spice_export.c`

**功能**:
- 场计算和后处理
- VTK格式导出
- HDF5格式导出
- Touchstone格式导出
- SPICE格式导出

### 2.15 激励源
**文件**: `excitation_plane_wave.h/c`
- 平面波激励
- 点源激励
- 端口激励
- 激励源管理

### 2.16 端口支持
**文件**: `port_support_extended.c`
- 端口定义
- S参数提取
- 端口激励

### 2.17 电路耦合
**文件**:
- `core_circuit.h/c`
- `core_circuit_coupling.h`
- `coupling_quasistatic.h/c`

**功能**:
- 电路仿真集成
- 准静态耦合
- 电路-电磁场耦合

### 2.18 多物理场
**文件**: `core_multiphysics.h/c`
- 多物理场耦合
- 热-电耦合
- 结构-电磁耦合

### 2.19 宽频带仿真
**文件**: `core_wideband.h`, `wideband_simulation_optimization.c/h`
- 宽频带扫描
- 频率插值
- 宽频带优化

### 2.20 GPU加速
**文件**:
- `gpu_acceleration.h/c`
- `gpu_linalg_optimization.c/h`
- `gpu_parallelization_optimized.cu/h`
- `multi_gpu_work_distribution.c`

**功能**:
- CUDA加速
- OpenCL支持
- GPU线性代数
- 多GPU工作分配

### 2.21 内存管理
**文件**:
- `memory_pool.h/c`
- `memory_pool_optimized.c`
- `memory/memory_optimization.h/c`

**功能**:
- 内存池管理
- 内存优化
- 内存监控

### 2.22 数学库
**文件**: `math/`
- `unified_matrix_assembly.h/c`
- `math_backend_selector.h`
- `math_backend_implementation.c`
- `industrial_solver_abstraction.h`

**功能**:
- 统一矩阵组装
- 数学后端选择 (BLAS/LAPACK/MKL)
- 工业求解器抽象

### 2.23 结果处理
**文件**: `result_bundle.h/c`
- 结果数据结构
- 结果打包
- 结果访问

### 2.24 错误处理
**文件**: `core_errors.h/c`
- 错误码定义
- 错误处理机制
- 错误报告

### 2.25 检查点和恢复
**文件**: `checkpoint_recovery.h/c`
- 检查点保存
- 状态恢复
- 长时间仿真支持

### 2.26 S参数提取
**文件**: `enhanced_sparameter_extraction.h/c`
- S参数计算
- 增强提取算法

### 2.27 EMC分析
**文件**: `emc_analysis.c/h`
- EMC分析工具
- 电磁兼容性评估

---

## 3. 求解器模块 (solvers/)

### 3.1 MoM求解器 (solvers/mom/)

#### 核心文件
- `mom_solver.h`: MoM求解器主接口
- `mom_solver_module.h`: MoM求解器模块定义
- `mom_solver_unified.c`: 统一MoM求解器实现
- `mom_solver_min.c`: 最小化MoM求解器

#### 矩阵向量积
- `mom_matvec.h/c`: 矩阵向量积计算
- 支持H-矩阵和密集矩阵

#### 快速算法
- `mom_mlfmm.h/c`: 多层快速多极子方法
- `mom_aca.h/c`: 自适应交叉近似
- `mom_hmatrix.h/c`: H-矩阵实现

#### 特殊功能
- `mom_layered_medium.h/c`: 分层介质支持
- `mom_time_domain.h/c`: 时域MoM
- `mom_fullwave_solver.h/c`: 全波求解器

#### 网格处理
- `tri_mesh.c`: 三角网格处理

### 3.2 PEEC求解器 (solvers/peec/)

#### 核心文件
- `peec_solver.h`: PEEC求解器主接口
- `peec_solver_module.h`: PEEC求解器模块定义
- `peec_solver_unified.c`: 统一PEEC求解器
- `peec_solver_min.c`: 最小化PEEC求解器
- `peec_advanced.h`: 高级PEEC功能

#### 几何支持
- `peec_geometry_support.h/c`: 几何支持
- `peec_manhattan_mesh.c`: Manhattan网格
- `peec_triangular_mesh.c`: 三角网格
- `peec_non_manhattan_geometry.h/c`: 非Manhattan几何
- `manhattan_mesh.c`: Manhattan网格生成

#### 积分计算
- `peec_integration.h/c`: PEEC积分计算

#### 核函数
- `peec_directionality_kernels.h/c`: 方向性核函数

#### 材料处理
- `peec_materials_enhanced.h/c`: 增强材料处理

#### 特殊功能
- `peec_layered_medium.h/c`: 分层介质
- `peec_plane_wave.h/c`: 平面波激励
- `peec_time_domain.h/c`: 时域PEEC
- `peec_via_modeling.h/c`: 过孔建模
- `peec_satellite.h/c`: 卫星应用
- `peec_nonlinear.c`: 非线性PEEC
- `peec_statistical.c`: 统计PEEC

### 3.3 MTL求解器 (solvers/mtl/)

#### 核心文件
- `mtl_solver_module.h`: MTL求解器模块定义
- `mtl_solver.c`: MTL求解器实现

#### 特殊功能
- `mtl_wideband.h/c`: 宽频带MTL
- `mtl_time_domain.h/c`: 时域MTL
- `mtl_parameter_import.h/c`: 参数导入
- `mtl_hybrid_coupling.h/c`: 混合耦合

### 3.4 混合求解器 (solvers/hybrid/)

#### 核心文件
- `hybrid_solver.h`: 混合求解器主接口 (507行)
- `hybrid_solver.c`: 混合求解器实现

#### 耦合模块
- `mtl_coupling/mtl_hybrid_coupling.h/c`: MTL混合耦合

**详细索引**: 参见 `docs/HYBRID_SOLVER_MODULE_INDEX.md`

---

## 4. 网格生成模块 (mesh/)

### 4.1 CAD集成
**文件**:
- `cad_mesh_generation.h/c`
- `cgal_mesh_engine.h`
- `cgal_surface_mesh.cpp`
- `cgal_surface_mesh_enhanced.cpp`
- `opencascade_cad_import.h`
- `gmsh_surface_mesh.h`

**功能**:
- CGAL网格生成
- OpenCASCADE CAD导入
- Gmsh网格生成
- CAD到网格转换

### 4.2 网格引擎
**文件**:
- `mesh_engine.h/c`
- `mesh_subsystem.c`
- `mesh_algorithms.c`
- `mesh_migration.c`
- `clipper2_triangle_2d.h`

**功能**:
- 统一网格接口
- 网格算法
- 网格迁移
- 2D三角剖分

### 4.3 CMake集成
**文件**: `cgal_cmake_integration.cmake`
- CGAL CMake配置

---

## 5. 输入输出模块 (io/)

### 5.1 文件格式
**文件**:
- `advanced_file_formats.h/c`
- `format_validation.h/c`
- `pcb_file_io.h/c`

**功能**:
- 多种文件格式支持
- 格式验证
- PCB文件I/O

### 5.2 并行I/O
**文件**: `parallel_io.h/c`
- 并行文件读写
- 大文件处理

### 5.3 内存优化I/O
**文件**: `memory_optimization.h/c`
- 内存优化的I/O操作

### 5.4 PCB工作流
**文件**:
- `pcb_simulation_workflow.h/c`
- `pcb_electromagnetic_modeling.h/c`
- `pcb_gpu_acceleration.h`

**功能**:
- PCB仿真工作流
- PCB电磁建模
- GPU加速支持

---

## 6. 材料处理 (materials/)

**文件**: `cst_materials_parser.h/c`
- CST材料文件解析
- 材料数据库
- 材料属性处理

---

## 7. CAD集成 (cad/)

**文件**: `cad_mesh_generation.h/c`
- CAD几何处理
- CAD到网格转换

---

## 8. 几何处理 (geometry/)

**文件**: `pcb_ic_structures.h/c`
- PCB和IC结构处理
- 几何建模

---

## 9. 计算算法 (computation/)

**文件**:
- `adaptive_calculation.h/c`
- `adaptive_optimization.h/c`
- `structure_algorithms.h/c`

**功能**:
- 自适应计算
- 自适应优化
- 结构算法

---

## 10. 评估工具 (evaluation/)

**文件**: `quantitative_metrics.h/c`
- 定量评估指标
- 性能评估
- 精度评估

---

## 11. 验证工具 (validation/)

**文件**:
- `commercial_validation.h/c`
- `industrial_validation_benchmarks.h`

**功能**:
- 商业软件对比验证
- 工业基准测试
- 验证报告生成

---

## 12. 性能监控 (performance/)

**文件**: `performance_monitor.h/c`
- 性能监控
- 性能分析
- 性能报告

---

## 13. 工具函数 (utils/)

### 13.1 基础工具
- `logger.h`: 日志系统
- `error_handler.h`: 错误处理
- `memory_manager.h`: 内存管理

### 13.2 评估工具
- `evaluation/quantitative_metrics.h/c`

### 13.3 性能工具
- `performance/performance_monitor.h/c`

### 13.4 验证工具
- `validation/commercial_validation.h/c`
- `validation/industrial_validation_benchmarks.h`

---

## 14. API接口 (api/)

**文件**: `api_generator.h/c`
- API代码生成
- 统一API接口
- 接口文档生成

---

## 15. 插件框架 (plugins/)

### 15.1 框架
**文件**: `plugin_framework.h/c`
- 插件系统框架
- 插件加载和管理

### 15.2 MoM插件
**文件**: `mom/*.cpp`
- MoM求解器插件

### 15.3 PEEC插件
**文件**: `peec/peec_solver_plugin.cpp`
- PEEC求解器插件

---

## 16. 工作流 (workflows/)

**文件**: `pcb/pcb_simulation_workflow.h/c`
- PCB仿真工作流
- 工作流管理

---

## 17. 建模工具 (modeling/)

**文件**: `pcb/pcb_electromagnetic_modeling.h/c`
- PCB电磁建模
- 建模工具

---

## 18. C接口 (c_interface/)

**文件**: `satellite_mom_peec_interface.h/c`
- 卫星MoM-PEEC接口
- C语言接口封装

---

## 19. 应用模块 (applications/)

**文件**:
- `metamaterial_extraction.h/c`
- `enclosure_calculation.h/c`

**功能**:
- 超材料提取
- 机箱计算

---

## 20. 探测工具 (probes/)

**文件**:
- `boost_probe.cpp`
- `cgal_probe.cpp`
- `gmsh_probe.c`
- `openblas_probe.c`

**功能**:
- 第三方库探测
- 库可用性检查

---

## 21. 图形界面 (gui/)

**文件**: `advanced_ui_system.c`
- 高级UI系统
- 图形界面

---

## 22. 应用程序 (apps/)

### 22.1 MoM CLI
**文件**: `mom_cli.c`
- MoM命令行接口
- 完整的MoM仿真工作流

### 22.2 PEEC CLI
**文件**: `peec_cli.c`, `peec_cli_msvc_stub.c`
- PEEC命令行接口

### 22.3 混合CLI
**文件**: `hybrid_cli.c`
- 混合求解器命令行接口

### 22.4 PulseEM CLI
**文件**: `pulseem_cli.c`
- PulseEM统一命令行接口

---

## 23. 示例代码 (examples/)

### 23.1 基础示例
- `antenna_simulation_examples.c`: 天线仿真示例
- `circuit_coupling_example.c`: 电路耦合示例
- `peec_mom_integration_examples.c`: PEEC-MoM集成示例

### 23.2 高级示例
- `commercial_grade_demo.c`: 商业级演示
- `comprehensive_integration_demo.c`: 综合集成演示
- `gpu_optimization_demo.c`: GPU优化演示
- `wideband_optimization_example.c`: 宽频带优化示例

### 23.3 专业示例
- `cad_mesh_integration.c`: CAD网格集成
- `metamaterial_extraction_examples.c`: 超材料提取
- `mtl_arbitrary_routing_example.c`: MTL任意布线
- `pcb_workflow_examples.c`: PCB工作流
- `parallel_io_examples.c`: 并行I/O
- `performance_benchmark_suite.c`: 性能基准测试
- `plugin_architecture_integration.c`: 插件架构集成

---

## 24. 测试代码 (tests/)

### 24.1 单元测试
- `validation_tests.c/h`: 验证测试
- `test_mesh_generation.c`: 网格生成测试
- `test_pcb_interfaces.c`: PCB接口测试
- `test_layered_green_min.c`: 分层格林函数测试

### 24.2 集成测试
- `test_hpm/`: HPM测试套件
- `test_mlfma_comprehensive.py`: MLFMA综合测试
- `test_opencascade_cad_import.cpp`: OpenCASCADE导入测试

### 24.3 验证测试
- `verify_engine_fix.c`: 引擎修复验证
- `verify_libraries_*.bat`: 库验证脚本

---

## 25. Python接口

### 25.1 Python脚本
**目录**: `python/`
- 27个Python脚本文件
- 仿真自动化
- 结果分析
- 可视化

### 25.2 Python绑定
**目录**: `python_interface/`
- Python C扩展
- Python API绑定

---

## 26. 构建系统

### 26.1 CMake配置
**文件**: `CMakeLists.txt`
- 主CMake配置
- 依赖管理
- 编译选项

### 26.2 vcpkg配置
**文件**: `vcpkg.json`
- vcpkg依赖配置
- 第三方库管理

### 26.3 构建脚本
- `build_windows.bat`: Windows构建
- `build_vs.bat`: Visual Studio构建
- `build_optimized.bat`: 优化构建
- `build_test_suite.bat`: 测试套件构建
- `build_gpu_enhanced.bat`: GPU增强构建
- `build_integration_optimized.bat`: 集成优化构建

---

## 27. 文档 (docs/)

### 27.1 架构文档
- `final_architecture_recommendations.md`: 架构建议
- `HYBRID_SOLVER_MODULE_INDEX.md`: 混合求解器索引
- `PROJECT_COMPLETE_INDEX.md`: 工程完整索引 (本文档)

### 27.2 优化文档
- `FINAL_OPTIMIZATION_STATUS.md`: 优化状态
- `OPTIMIZATION_COMPLETED.md`: 优化完成报告
- `REORGANIZATION_COMPLETED.md`: 重组完成报告

### 27.3 实现文档
- `HYBRID_SOLVER_IMPLEMENTATION_SUMMARY.md`: 混合求解器实现总结
- `HYBRID_SOLVER_UNIFICATION_SUMMARY.md`: 混合求解器统一总结
- `HYBRID_SOLVER_MERGE_PLAN.md`: 混合求解器合并计划

### 27.4 编译文档
- `COMPILATION_FIXES.md`: 编译修复
- `CODE_REORGANIZATION_ANALYSIS.md`: 代码重组分析

---

## 28. 第三方库 (libs/)

### 28.1 数学库
- BLAS/LAPACK
- MKL (Intel Math Kernel Library)
- OpenBLAS

### 28.2 网格库
- CGAL (Computational Geometry Algorithms Library)
- Gmsh
- OpenCASCADE

### 28.3 求解器库
- MUMPS
- PARDISO
- SuperLU

### 28.4 其他库
- Boost
- HDF5
- VTK

---

## 29. 主要特性

### 29.1 求解器支持
- ✅ MoM (Method of Moments)
- ✅ PEEC (Partial Element Equivalent Circuit)
- ✅ MTL (Multiconductor Transmission Lines)
- ✅ 混合求解器 (MoM-PEEC-MTL三路耦合)

### 29.2 快速算法
- ✅ MLFMM (Multi-Level Fast Multipole Method)
- ✅ ACA (Adaptive Cross Approximation)
- ✅ H-矩阵压缩
- ✅ 快速多极子方法

### 29.3 特殊功能
- ✅ 分层介质支持
- ✅ 时域仿真
- ✅ 宽频带扫描
- ✅ 非线性材料
- ✅ 统计分析

### 29.4 性能优化
- ✅ GPU加速 (CUDA/OpenCL)
- ✅ 并行计算 (OpenMP/MPI)
- ✅ 内存优化
- ✅ 核外求解器
- ✅ 检查点/恢复

### 29.5 CAD集成
- ✅ CGAL网格生成
- ✅ Gmsh支持
- ✅ OpenCASCADE导入
- ✅ STL文件支持

### 29.6 后处理
- ✅ VTK可视化
- ✅ HDF5数据导出
- ✅ Touchstone格式
- ✅ SPICE格式

---

## 30. 代码统计

### 30.1 文件统计
- **头文件 (.h)**: ~140个
- **源文件 (.c/.cpp)**: ~141个
- **Python脚本**: ~27个
- **文档文件**: ~89个
- **总代码行数**: 估计 > 50,000行

### 30.2 模块统计
- **核心模块**: ~100个文件
- **求解器模块**: ~40个文件
- **网格模块**: ~15个文件
- **I/O模块**: ~15个文件
- **工具模块**: ~20个文件

---

## 31. 使用流程

### 31.1 基本MoM仿真
```
1. 创建几何/导入CAD
2. 生成网格
3. 设置材料属性
4. 配置MoM求解器
5. 组装矩阵
6. 求解
7. 后处理和可视化
```

### 31.2 混合仿真
```
1. 创建/导入几何
2. 生成网格 (MoM和PEEC)
3. 设置材料属性
4. 创建混合求解器
5. 设置MoM和PEEC求解器
6. 创建接口
7. 组装耦合矩阵
8. 求解
9. 后处理
```

### 31.3 PCB仿真工作流
```
1. 导入PCB文件
2. 解析材料属性
3. 生成网格
4. 设置端口
5. 配置求解器
6. 运行仿真
7. 提取S参数
8. 导出结果
```

---

## 32. 开发指南

### 32.1 代码风格
- C11标准
- C++17标准
- 统一的命名约定
- 详细的注释

### 32.2 错误处理
- 统一的错误码系统
- 详细的错误消息
- 资源清理保证

### 32.3 内存管理
- 内存池管理
- 自动资源清理
- 内存泄漏检测

### 32.4 性能优化
- 算法优化
- 并行化
- GPU加速
- 内存优化

---

## 33. 构建和部署

### 33.1 依赖要求
- CMake 3.16+
- C11编译器
- C++17编译器
- vcpkg (可选)
- CUDA (可选，GPU支持)

### 33.2 构建步骤
```bash
# Windows (Visual Studio)
build_vs.bat

# 优化构建
build_optimized.bat

# GPU增强构建
build_gpu_enhanced.bat
```

### 33.3 测试
```bash
# 运行测试套件
build_test_suite.bat
```

---

## 34. 许可证和版权

**版权**: Copyright (C) 2024-2025 PulseEM Technologies  
**许可证**: 商业许可证  
**使用限制**: 未经授权禁止复制、修改或分发

---

## 35. 联系和支持

- **项目名称**: PulseMoM
- **开发团队**: PulseMoM Development Team
- **技术支持**: 参见项目文档

---

## 36. 版本历史

### v1.0.0 (当前版本)
- 完整的MoM-PEEC-MTL框架
- 混合求解器支持
- GPU加速
- CAD集成
- 工业级验证

---

## 37. 未来计划

### 37.1 功能增强
- 更多快速算法
- 更好的GPU支持
- 增强的CAD集成
- 更多后处理选项

### 37.2 性能优化
- 进一步并行化
- 更好的内存管理
- 算法优化

### 37.3 用户体验
- 更好的文档
- 更多示例
- 图形界面改进

---

## 附录

### A. 快速参考

#### A.1 主要头文件
```c
#include "core/core_common.h"           // 基础定义
#include "solvers/mom/mom_solver.h"     // MoM求解器
#include "solvers/peec/peec_solver.h"   // PEEC求解器
#include "solvers/hybrid/hybrid_solver.h" // 混合求解器
```

#### A.2 主要数据结构
- `MomSolver`: MoM求解器
- `PeecSolver`: PEEC求解器
- `HybridSolver`: 混合求解器
- `Mesh`: 网格结构
- `Geometry`: 几何结构

#### A.3 主要函数
- `mom_solver_create()`: 创建MoM求解器
- `peec_solver_create()`: 创建PEEC求解器
- `hybrid_solver_create()`: 创建混合求解器
- `hybrid_solver_solve()`: 求解

### B. 相关文档
- `HYBRID_SOLVER_MODULE_INDEX.md`: 混合求解器详细索引
- `FINAL_OPTIMIZATION_STATUS.md`: 优化状态
- `USER_GUIDE.md`: 用户指南

---

**文档生成时间**: 2024-2025  
**最后更新**: 基于工程当前状态  
**维护者**: PulseMoM Development Team

---

## 索引导航

### 按模块查找
- [核心模块](#2-核心模块-core)
- [求解器模块](#3-求解器模块-solvers)
- [网格模块](#4-网格生成模块-mesh)
- [I/O模块](#5-输入输出模块-io)
- [工具模块](#13-工具函数-utils)

### 按功能查找
- [快速算法](#29-主要特性)
- [GPU加速](#220-gpu加速)
- [CAD集成](#41-cad集成)
- [后处理](#214-后处理)

### 按文件类型查找
- [头文件](#301-文件统计)
- [源文件](#301-文件统计)
- [示例代码](#23-示例代码-examples)
- [测试代码](#24-测试代码-tests)

---

**注意**: 本文档是工程索引的完整版本。对于特定模块的详细信息，请参考相应的模块文档。
