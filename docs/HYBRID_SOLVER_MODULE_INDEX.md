# Hybrid Solver 模块详细索引

> **相关文档**: 
> - [PulseMoM 工程完整索引](./PROJECT_COMPLETE_INDEX.md) - 查看整个工程的结构和模块
> - 本文档专注于混合求解器模块的详细说明

## 文件信息
- **文件路径**: `src/solvers/hybrid/hybrid_solver.h`
- **模块名称**: PEEC-MoM Unified Framework - Hybrid Coupling Interface
- **功能描述**: 实现PEEC和MoM求解器之间的耦合接口，用于混合域电磁仿真
- **总行数**: 507行
- **头文件保护**: `HYBRID_COUPLING_INTERFACE_H`

---

## 1. 依赖关系

### 1.1 包含的头文件
```c
#include "../../core/electromagnetic_kernel_library.h"
#include "../mom/mom_solver_module.h"
#include "../peec/peec_solver_module.h"
#include "../mtl/mtl_solver_module.h"
```

### 1.2 语言兼容性
- 支持C/C++混合编程（`extern "C"`）

---

## 2. 枚举类型 (Enumerations)

### 2.1 HybridCouplingMethod (耦合方法)
**位置**: 第26-37行

| 枚举值 | 说明 |
|--------|------|
| `COUPLING_METHOD_SCHUR_COMPLEMENT` | Schur补方法 |
| `COUPLING_METHOD_DOMAIN_DECOMPOSITION` | 域分解方法 |
| `COUPLING_METHOD_ITERATIVE_SUBDOMAIN` | 迭代子域方法 |
| `COUPLING_METHOD_LAGRANGE_MULTIPLIERS` | 拉格朗日乘数法 |
| `COUPLING_METHOD_PENALTY_METHOD` | 惩罚方法 |
| `COUPLING_METHOD_MORTAR_METHOD` | Mortar方法 |
| `COUPLING_METHOD_MTL_FIELD_COUPLING` | MTL-MoM场耦合 |
| `COUPLING_METHOD_MTL_CIRCUIT_COUPLING` | MTL-PEEC电路耦合 |
| `COUPLING_METHOD_MTL_FULL_HYBRID` | MTL三路耦合 |

### 2.2 HybridCouplingType (耦合类型)
**位置**: 第39-46行

| 枚举值 | 说明 |
|--------|------|
| `COUPLING_TYPE_ELECTRIC_FIELD` | 电场耦合 |
| `COUPLING_TYPE_MAGNETIC_FIELD` | 磁场耦合 |
| `COUPLING_TYPE_CURRENT_DENSITY` | 电流密度耦合 |
| `COUPLING_TYPE_VOLTAGE_POTENTIAL` | 电压势耦合 |
| `COUPLING_TYPE_POWER_FLOW` | 功率流耦合 |
| `COUPLING_TYPE_MIXED` | 混合耦合 |

### 2.3 HybridCouplingDomain (耦合域)
**位置**: 第48-53行

| 枚举值 | 说明 |
|--------|------|
| `COUPLING_DOMAIN_PEEC_TO_MOM` | PEEC域到MoM域 |
| `COUPLING_DOMAIN_MOM_TO_PEEC` | MoM域到PEEC域 |
| `COUPLING_DOMAIN_BIDIRECTIONAL` | 双向耦合 |
| `COUPLING_DOMAIN_OVERLAPPING` | 重叠域 |

### 2.4 HybridCouplingCombination (求解器组合)
**位置**: 第361-366行

| 枚举值 | 说明 |
|--------|------|
| `COUPLING_COMBINATION_MOM_PEEC` | MoM + PEEC |
| `COUPLING_COMBINATION_MOM_MTL` | MoM + MTL |
| `COUPLING_COMBINATION_PEEC_MTL` | PEEC + MTL |
| `COUPLING_COMBINATION_MOM_PEEC_MTL` | MoM + PEEC + MTL (三路) |

---

## 3. 数据结构 (Data Structures)

### 3.1 HybridCouplingOptions (耦合配置选项)
**位置**: 第55-97行

#### 主要字段分类：

**耦合方法配置**
- `method`: 耦合方法选择
- `type`: 耦合类型
- `domain`: 耦合域

**收敛控制**
- `max_iterations`: 最大迭代次数
- `convergence_tolerance`: 收敛容差
- `relaxation_parameter`: 松弛参数

**接口设置**
- `num_interface_points`: 接口点数量
- `interface_tolerance`: 接口容差
- `use_adaptive_interface`: 是否使用自适应接口
- `use_robin_interface`: 是否使用Robin接口

**迭代求解器设置**
- `use_gmres`: 是否使用GMRES
- `use_bicgstab`: 是否使用BiCGSTAB
- `krylov_subspace_size`: Krylov子空间大小
- `restart_size`: 重启大小

**预条件设置**
- `use_preconditioner`: 是否使用预条件器
- `preconditioner_type`: 预条件器类型
- `preconditioner_tolerance`: 预条件器容差

**并行设置**
- `use_parallel_coupling`: 是否使用并行耦合
- `num_coupling_threads`: 耦合线程数
- `use_mpi`: 是否使用MPI

**性能设置**
- `enable_profiling`: 是否启用性能分析
- `save_intermediate_results`: 是否保存中间结果
- `compute_interface_errors`: 是否计算接口误差

**精度设置**
- `accuracy_target`: 精度目标
- `use_adaptive_accuracy`: 是否使用自适应精度
- `adaptive_refinement_levels`: 自适应细化级别

### 3.2 HybridInterfacePoint (接口点定义)
**位置**: 第102-139行

#### 字段分类：

**标识信息**
- `interface_id`: 接口ID
- `mom_entity_id`: MoM三角形或边ID
- `peec_entity_id`: PEEC单元或节点ID

**几何信息**
- `position[3]`: 物理坐标
- `normal[3]`: 法向量
- `tangent[3]`: 切向量

**耦合系数**
- `coupling_coefficient`: 耦合系数
- `mom_to_peec_transfer`: MoM到PEEC传递
- `peec_to_mom_transfer`: PEEC到MoM传递

**场量**
- `electric_field[3]`: 电场
- `magnetic_field[3]`: 磁场
- `current_density[3]`: 电流密度
- `voltage_potential`: 电压势

**边界条件**
- `dirichlet_value`: Dirichlet值
- `neumann_value`: Neumann值
- `robin_coefficient`: Robin系数

**误差指标**
- `interface_error`: 接口误差
- `field_discontinuity`: 场不连续性
- `power_imbalance`: 功率不平衡

**标志位**
- `is_active`: 是否激活
- `is_dirichlet`: 是否Dirichlet边界
- `is_neumann`: 是否Neumann边界
- `is_robin`: 是否Robin边界
- `is_converged`: 是否收敛

### 3.3 HybridInterface (接口集合)
**位置**: 第141-160行

**字段**:
- `points`: 接口点数组
- `num_points`: 接口点数量
- `max_points`: 最大接口点数
- `spatial_index`: 空间索引
- `max_interface_error`: 最大接口误差
- `avg_interface_error`: 平均接口误差
- `rms_interface_error`: RMS接口误差
- `is_converged`: 是否收敛
- `iteration_count`: 迭代计数
- `memory_usage`: 内存使用量

### 3.4 HybridCouplingMatrix (耦合矩阵结构)
**位置**: 第165-195行

#### 耦合矩阵 (支持所有配对)
- `mom_to_peec_matrix`: MoM到PEEC传递矩阵
- `peec_to_mom_matrix`: PEEC到MoM传递矩阵
- `mom_to_mtl_matrix`: MoM到MTL传递矩阵
- `mtl_to_mom_matrix`: MTL到MoM传递矩阵
- `peec_to_mtl_matrix`: PEEC到MTL传递矩阵
- `mtl_to_peec_matrix`: MTL到PEEC传递矩阵
- `interface_matrix`: 接口耦合矩阵
- `schur_complement`: Schur补矩阵

#### 矩阵属性
- `mom_size`, `peec_size`, `mtl_size`, `interface_size`: 矩阵维度
- `is_symmetric`: 是否对称
- `is_positive_definite`: 是否正定
- `condition_number`: 条件数
- `storage_format`: 存储格式（密集、稀疏、压缩）
- `memory_usage`: 内存使用量

#### 分解数据
- `lu_factorization`: LU分解
- `cholesky_factorization`: Cholesky分解
- `qr_factorization`: QR分解

### 3.5 HybridIterationData (迭代耦合数据)
**位置**: 第200-231行

**当前迭代解**
- `mom_solution`: MoM解
- `peec_solution`: PEEC解
- `interface_solution`: 接口解

**前一次迭代解**
- `mom_solution_prev`: 前一次MoM解
- `peec_solution_prev`: 前一次PEEC解
- `interface_solution_prev`: 前一次接口解

**残差向量**
- `mom_residual`: MoM残差
- `peec_residual`: PEEC残差
- `interface_residual`: 接口残差

**搜索方向 (Krylov方法)**
- `mom_search_direction`: MoM搜索方向
- `peec_search_direction`: PEEC搜索方向
- `interface_search_direction`: 接口搜索方向

**收敛历史**
- `residual_history`: 残差历史
- `error_history`: 误差历史
- `history_size`: 历史大小
- `current_iteration`: 当前迭代

**松弛参数**
- `relaxation_parameter`: 松弛参数
- `adaptive_relaxation`: 自适应松弛
- `use_adaptive_relaxation`: 是否使用自适应松弛

### 3.6 HybridSolver (主混合求解器结构)
**位置**: 第238-288行

#### 配置
- `options`: 耦合配置选项

#### 组件求解器 (支持任意组合)
- `mom_solver`: MoM求解器
- `peec_solver`: PEEC求解器
- `mtl_solver`: MTL求解器

#### 求解器启用标志
- `mom_enabled`: MoM是否启用
- `peec_enabled`: PEEC是否启用
- `mtl_enabled`: MTL是否启用

#### 接口数据
- `interface`: 接口数据
- `coupling_matrix`: 耦合矩阵
- `iteration_data`: 迭代数据

#### 配对耦合矩阵 (支持任意组合)
- `mom_peec_matrix`: MoM ↔ PEEC
- `mom_mtl_matrix`: MoM ↔ MTL
- `peec_mtl_matrix`: PEEC ↔ MTL

#### 解数据
- `coupled_solution`: 耦合解
- `solution_size`: 解的大小

#### 结果
- `final_error`: 最终误差
- `num_iterations`: 迭代次数
- `is_converged`: 是否收敛
- `total_time`: 总时间

#### 性能监控
- `coupling_timer`: 耦合计时器
- `iteration_timer`: 迭代计时器
- `total_timer`: 总计时器

#### 内存使用
- `peak_memory_usage`: 峰值内存使用
- `current_memory_usage`: 当前内存使用

#### 线程安全
- `mutex`: 互斥锁

#### 内部状态
- `is_initialized`: 是否已初始化
- `is_assembled`: 是否已组装
- `is_solved`: 是否已求解

---

## 4. API函数分类

### 4.1 生命周期函数 (Lifecycle Functions)
**位置**: 第294-303行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_create` | 创建混合求解器 | `options` |
| `hybrid_solver_destroy` | 销毁混合求解器 | `solver` |
| `hybrid_solver_set_mom_solver` | 设置MoM求解器 | `solver`, `mom_solver` |
| `hybrid_solver_set_peec_solver` | 设置PEEC求解器 | `solver`, `peec_solver` |
| `hybrid_solver_set_mtl_solver` | 设置MTL求解器 | `solver`, `mtl_solver` |
| `hybrid_solver_initialize` | 初始化求解器 | `solver` |
| `hybrid_solver_finalize` | 终结求解器 | `solver` |

### 4.2 接口管理函数 (Interface Management)
**位置**: 第305-311行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_create_interface` | 创建接口 | `solver`, `mom_geometry`, `peec_geometry` |
| `hybrid_solver_refine_interface` | 细化接口 | `solver`, `error_threshold` |
| `hybrid_solver_adapt_interface` | 自适应接口 | `solver`, `target_points` |

### 4.3 耦合矩阵组装函数 (Coupling Matrix Assembly)
**位置**: 第312-316行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_assemble_coupling_matrices` | 组装耦合矩阵 | `solver` |
| `hybrid_solver_factorize_coupling_matrices` | 分解耦合矩阵 | `solver` |
| `hybrid_solver_compress_coupling_matrices` | 压缩耦合矩阵 | `solver`, `tolerance` |

### 4.4 求解方法函数 (Solution Methods)
**位置**: 第317-332行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_solve_schur_complement` | Schur补方法求解 | `solver` |
| `hybrid_solver_solve_domain_decomposition` | 域分解方法求解 | `solver` |
| `hybrid_solver_solve_iterative_subdomain` | 迭代子域方法求解 | `solver` |
| `hybrid_solver_solve_lagrange_multipliers` | 拉格朗日乘数法求解 | `solver` |
| `hybrid_solver_solve` | 统一求解方法（自动检测求解器组合） | `solver` |
| `hybrid_solver_solve_three_way` | 三路耦合求解 (PEEC-MoM-MTL) | `solver` |
| `hybrid_solver_iterate_coupling` | 迭代耦合 | `solver` |
| `hybrid_solver_check_convergence` | 检查收敛性 | `solver` |
| `hybrid_solver_update_interface` | 更新接口 | `solver` |

### 4.5 误差分析函数 (Error Analysis)
**位置**: 第334-337行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_compute_interface_errors` | 计算接口误差 | `solver` |
| `hybrid_solver_estimate_coupling_error` | 估计耦合误差 | `solver`, `error_estimate` |
| `hybrid_solver_validate_coupling` | 验证耦合 | `solver` |

### 4.6 结果访问函数 (Results Access)
**位置**: 第339-344行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_get_coupled_solution` | 获取耦合解 | `solver` |
| `hybrid_solver_get_solution_size` | 获取解的大小 | `solver` |
| `hybrid_solver_get_final_error` | 获取最终误差 | `solver` |
| `hybrid_solver_get_num_iterations` | 获取迭代次数 | `solver` |
| `hybrid_solver_is_converged` | 检查是否收敛 | `solver` |

### 4.7 性能监控函数 (Performance Monitoring)
**位置**: 第346-350行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_get_memory_usage` | 获取内存使用量 | `solver` |
| `hybrid_solver_get_peak_memory_usage` | 获取峰值内存使用量 | `solver` |
| `hybrid_solver_get_total_time` | 获取总时间 | `solver` |
| `hybrid_solver_reset_performance_counters` | 重置性能计数器 | `solver` |

### 4.8 工具函数 (Utility Functions)
**位置**: 第352-356行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_print_info` | 打印信息 | `solver` |
| `hybrid_solver_print_statistics` | 打印统计信息 | `solver` |
| `hybrid_solver_export_interface` | 导出接口 | `solver`, `filename` |
| `hybrid_solver_export_coupling_matrices` | 导出耦合矩阵 | `solver`, `filename` |

### 4.9 求解器组合检测函数 (Solver Combination Detection)
**位置**: 第368-372行

| 函数名 | 功能 | 参数 |
|--------|------|------|
| `hybrid_solver_detect_combination` | 检测求解器组合 | `solver`, `combination` |
| `hybrid_solver_has_mom` | 检查是否有MoM | `solver` |
| `hybrid_solver_has_peec` | 检查是否有PEEC | `solver` |
| `hybrid_solver_has_mtl` | 检查是否有MTL | `solver` |

---

## 5. 高级耦合方法 (Advanced Coupling Methods)

### 5.1 Schur补方法 (Schur Complement Method)
**位置**: 第378-395行

#### 数据结构: SchurComplementData
- `schur_matrix`: Schur矩阵
- `schur_rhs`: Schur右端项
- `interface_solution`: 接口解
- `interface_size`: 接口大小
- `is_factorized`: 是否已分解
- `factorization_data`: 分解数据

#### API函数
- `schur_complement_create`: 创建Schur补数据
- `schur_complement_destroy`: 销毁Schur补数据
- `schur_complement_assemble`: 组装Schur补
- `schur_complement_factorize`: 分解Schur补
- `schur_complement_solve`: 求解Schur补

### 5.2 域分解方法 (Domain Decomposition Method)
**位置**: 第397-412行

#### 数据结构: DomainDecompositionData
- `num_subdomains`: 子域数量
- `subdomain_sizes`: 子域大小数组
- `subdomain_offsets`: 子域偏移数组
- `subdomain_matrices`: 子域矩阵数组
- `interface_matrices`: 接口矩阵数组
- `is_parallel`: 是否并行
- `mpi_communicator`: MPI通信器

#### API函数
- `domain_decomposition_create`: 创建域分解数据
- `domain_decomposition_destroy`: 销毁域分解数据
- `domain_decomposition_setup`: 设置域分解
- `domain_decomposition_solve`: 求解域分解

### 5.3 迭代子域方法 (Iterative Subdomain Method)
**位置**: 第414-431行

#### 数据结构: IterativeSubdomainData
- `relaxation_parameter`: 松弛参数
- `adaptive_relaxation_factor`: 自适应松弛因子
- `use_adaptive_relaxation`: 是否使用自适应松弛
- `max_local_iterations`: 最大局部迭代次数
- `local_convergence_tolerance`: 局部收敛容差
- `use_gmres_acceleration`: 是否使用GMRES加速
- `gmres_restart`: GMRES重启大小

#### API函数
- `iterative_subdomain_create`: 创建迭代子域数据
- `iterative_subdomain_destroy`: 销毁迭代子域数据
- `iterative_subdomain_setup`: 设置迭代子域
- `iterative_subdomain_iterate`: 迭代子域

---

## 6. 接口映射和投影 (Interface Mapping and Projection)
**位置**: 第433-463行

### 6.1 数据结构: InterfaceProjectionData

**投影算子**
- `mom_to_peec_projector`: MoM到PEEC投影算子
- `peec_to_mom_projector`: PEEC到MoM投影算子

**插值矩阵**
- `mom_interpolation_matrix`: MoM插值矩阵
- `peec_interpolation_matrix`: PEEC插值矩阵

**积分权重**
- `quadrature_weights`: 积分权重
- `quadrature_points`: 积分点
- `num_quadrature_points`: 积分点数量

**误差估计**
- `projection_error`: 投影误差
- `interpolation_error`: 插值误差
- `quadrature_error`: 积分误差

### 6.2 API函数
- `interface_projection_create`: 创建接口投影数据
- `interface_projection_destroy`: 销毁接口投影数据
- `interface_projection_compute_projectors`: 计算投影算子
- `interface_projection_mom_to_peec`: MoM到PEEC投影
- `interface_projection_peec_to_mom`: PEEC到MoM投影

---

## 7. 误差估计和自适应 (Error Estimation and Adaptivity)
**位置**: 第465-501行

### 7.1 数据结构: ErrorEstimationData

**误差指示器**
- `local_error_indicators`: 局部误差指示器
- `global_error_indicators`: 全局误差指示器
- `num_indicators`: 指示器数量

**自适应准则**
- `refinement_threshold`: 细化阈值
- `coarsening_threshold`: 粗化阈值
- `max_refinement_level`: 最大细化级别

**误差范数**
- `l2_error`: L2误差
- `h1_error`: H1误差
- `energy_error`: 能量误差
- `maximum_error`: 最大误差

**收敛率**
- `convergence_rate`: 收敛率
- `previous_error`: 前一次误差
- `iteration_count`: 迭代计数

### 7.2 API函数
- `error_estimation_create`: 创建误差估计数据
- `error_estimation_destroy`: 销毁误差估计数据
- `error_estimation_compute_local_errors`: 计算局部误差
- `error_estimation_compute_global_errors`: 计算全局误差
- `error_estimation_mark_for_refinement`: 标记需要细化的单元
- `error_estimation_adapt_coupling`: 自适应耦合

---

## 8. 模块特性总结

### 8.1 支持的求解器组合
- ✅ MoM + PEEC (双向耦合)
- ✅ MoM + MTL (双向耦合)
- ✅ PEEC + MTL (双向耦合)
- ✅ MoM + PEEC + MTL (三路耦合)

### 8.2 支持的耦合方法
1. **Schur补方法**: 适用于强耦合问题
2. **域分解方法**: 支持并行计算
3. **迭代子域方法**: 适用于弱耦合问题
4. **拉格朗日乘数法**: 约束优化方法
5. **惩罚方法**: 简单但有效
6. **Mortar方法**: 非匹配网格
7. **MTL专用方法**: 场耦合、电路耦合、全混合

### 8.3 支持的耦合类型
- 电场耦合
- 磁场耦合
- 电流密度耦合
- 电压势耦合
- 功率流耦合
- 混合耦合

### 8.4 高级功能
- ✅ 自适应接口细化
- ✅ 误差估计和自适应
- ✅ 接口投影和映射
- ✅ 多种迭代求解器 (GMRES, BiCGSTAB)
- ✅ 预条件支持
- ✅ 并行计算支持 (MPI)
- ✅ 性能监控和分析
- ✅ 内存管理优化

### 8.5 数值方法
- LU分解
- Cholesky分解
- QR分解
- Krylov子空间方法
- 自适应松弛
- Robin边界条件

---

## 9. 代码统计

- **总行数**: 507行
- **数据结构数量**: 10个
- **枚举类型数量**: 4个
- **API函数数量**: 约50+个
- **高级方法模块**: 3个 (Schur补、域分解、迭代子域)
- **辅助模块**: 2个 (接口投影、误差估计)

---

## 10. 使用流程建议

### 10.1 基本使用流程
```
1. 创建求解器: hybrid_solver_create()
2. 设置组件求解器: hybrid_solver_set_*_solver()
3. 初始化: hybrid_solver_initialize()
4. 创建接口: hybrid_solver_create_interface()
5. 组装耦合矩阵: hybrid_solver_assemble_coupling_matrices()
6. 求解: hybrid_solver_solve()
7. 获取结果: hybrid_solver_get_*()
8. 清理: hybrid_solver_destroy()
```

### 10.2 高级使用流程（带自适应）
```
1-4. 同基本流程
5. 组装耦合矩阵
6. 求解
7. 计算误差: hybrid_solver_compute_interface_errors()
8. 如果误差过大，细化接口: hybrid_solver_refine_interface()
9. 重复步骤5-8直到收敛
10. 获取结果和清理
```

---

## 11. 注意事项

1. **内存管理**: 所有创建的数据结构都需要相应的销毁函数
2. **线程安全**: 结构体包含mutex，但需要正确使用
3. **求解器组合**: 使用前需要正确设置求解器组合
4. **接口创建**: 接口创建需要提供正确的几何信息
5. **收敛性**: 不同方法适用于不同问题，需要根据问题特性选择
6. **性能**: 并行计算需要正确配置MPI环境

---

**文档生成时间**: 2024
**最后更新**: 基于 hybrid_solver.h v1.0
