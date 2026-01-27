# 最终架构分析报告

## 分析依据

- 架构决策树 (`.claude/skills/01_architecture_layers.md`)
- 商用EM软件标准
- 用户反馈：降低认知负担、提高新成员上手速度、降低文件归属判断成本

---

## 一、核心问题总结

### 1.1 `solvers/` vs `orchestration/` 边界模糊

**问题**：
- `mom_solver_unified.c` 和 `peec_solver_unified.c` 包含编排逻辑（算法选择、宽带策略）
- 这些逻辑应该属于 L5 orchestration 层

**影响**：
- 新成员不知道算法选择逻辑在哪
- 文件归属判断成本高

**解决方案**：
- 提取编排逻辑到 `orchestration/<solver>_orchestration/`
- `solvers/` 只保留核心求解能力（"会算"）

### 1.2 `backend/algorithms/fast/` vs `operators/` 算法语义被拆裂

**问题**：
- ACA 和 H-matrix 的数学模型定义在 L4 层
- 但它们是算子近似（改变算子数学意义），应该属于 L3 层

**解决方案**：
- 创建 `operators/operator_approximation/` 存放数学模型
- `backend/algorithms/fast/` 只保留计算实现

### 1.3 `physics/` 层略"厚"

**问题**：
- `circuit_coupling_simulation.c` 包含数值实现和编排逻辑
- 应该只包含物理定义

**解决方案**：
- 拆分：物理定义保留在 `physics/`
- 数值实现和编排逻辑移动到 `orchestration/circuit_coupling/`

---

## 二、文件迁移详细清单

### 2.1 从 `solvers/mom/` 迁移到 `orchestration/mom_orchestration/`

| 源文件/函数 | 行号 | 目标文件 | 迁移内容 |
|------------|------|---------|---------|
| `mom_solver_unified.c::select_algorithm()` | 515 | `mom_algorithm_selector.c` | 算法选择函数 |
| `mom_solver_unified.c::compute_problem_characteristics()` | 552 | `mom_algorithm_selector.c` | 问题特征分析 |
| `mom_solver_unified.c::detect_curved_surfaces()` | 599 | `mom_algorithm_selector.c` | 辅助函数 |
| `mom_solver_unified.c::count_different_materials()` | 662 | `mom_algorithm_selector.c` | 辅助函数 |
| `mom_time_domain.c/h` | - | `mom_time_domain.c/h` | 完整文件迁移 |
| `mom_solver_unified.c::mom_solve_unified()` | 2695 | `mom_orchestration.c` | 主编排函数（拆分后） |

### 2.2 从 `solvers/peec/` 迁移到 `orchestration/peec_orchestration/`

| 源文件/函数 | 行号 | 目标文件 | 迁移内容 |
|------------|------|---------|---------|
| `peec_solver_unified.c::select_peec_algorithm()` | 302 | `peec_algorithm_selector.c` | 算法选择函数 |
| `peec_solver_unified.c::compute_peec_problem_characteristics()` | 315 | `peec_algorithm_selector.c` | 问题特征分析 |
| `peec_time_domain.c/h` | - | `peec_time_domain.c/h` | 完整文件迁移 |

### 2.3 从 `solvers/mtl/` 迁移到 `orchestration/mtl_orchestration/`

| 源文件 | 目标文件 |
|--------|---------|
| `mtl_wideband.c/h` | `mtl_wideband.c/h` |
| `mtl_time_domain.c/h` | `mtl_time_domain.c/h` |

### 2.4 从 `physics/peec/` 迁移到 `orchestration/circuit_coupling/`

| 源文件/函数 | 行号 | 目标文件 | 迁移内容 |
|------------|------|---------|---------|
| `circuit_coupling_simulation.c::build_mna_matrix()` | 424 | `circuit_coupling_orchestration.c` | MNA矩阵构建 |
| `circuit_coupling_simulation.c::solve_nonlinear_newton()` | 735 | `circuit_coupling_orchestration.c` | 非线性求解 |
| `circuit_coupling_simulation.c` (整体) | - | `circuit_coupling_orchestration.c` | 编排逻辑部分 |

**保留在 `physics/peec/`**：
- `peec_circuit_coupling.h` - 物理定义（已存在）

### 2.5 从 `backend/algorithms/fast/` 提取到 `operators/operator_approximation/`

| 源文件 | 目标文件 | 迁移内容 |
|--------|---------|---------|
| `aca.h` (数学模型部分) | `aca_operator_model.h` | ACA数学定义、误差控制理论 |
| `h_matrix_compression.h` (数学模型部分) | `hmatrix_operator_model.h` | H-matrix数学定义、层次结构 |

**保留在 `backend/algorithms/fast/`**：
- `aca.c` - ACA实现（并行、内存、GPU）
- `h_matrix_compression.c` - H-matrix实现

---

## 三、最终目录结构（优化后）

```
src/
│
├── physics/ (L1)                    # 只包含物理定义
│   ├── mom/mom_physics.c/h
│   ├── peec/
│   │   ├── peec_physics.c/h
│   │   ├── peec_circuit.h           # 电路模型定义（纯定义）
│   │   └── peec_circuit_coupling.h  # 耦合物理定义（纯定义）
│   └── mtl/mtl_physics.c/h
│
├── discretization/ (L2)             # 离散化
│   ├── geometry/
│   ├── mesh/
│   └── basis/
│
├── operators/ (L3)                  # 算子层
│   ├── kernels/
│   ├── integration/
│   ├── assembler/
│   ├── matvec/
│   ├── coupling/
│   └── operator_approximation/      # 新增：算子近似模型
│       ├── aca_operator_model.h     # ACA数学模型
│       ├── hmatrix_operator_model.h # H-matrix数学模型
│       └── approximation_interface.h
│
├── backend/ (L4)                    # 数值后端
│   ├── solvers/                     # 通用数值求解器
│   ├── algorithms/
│   │   └── fast/                    # 快速算法实现（保留）
│   ├── gpu/
│   └── math/
│
├── orchestration/ (L5)               # 执行编排（核心）
│   ├── solver_strategy/              # 新增：通用求解器策略
│   │   ├── algorithm_selector.c/h    # 通用算法选择框架
│   │   ├── wideband_strategy.c/h     # 通用宽带策略
│   │   ├── time_frequency_scheduler.c/h # 时域/频域调度
│   │   └── solver_profile.c/h       # 求解器能力描述
│   │
│   ├── mom_orchestration/            # 新增：MoM编排
│   │   ├── mom_algorithm_selector.c/h ✅ 已创建
│   │   ├── mom_wideband.c/h          # 待迁移
│   │   └── mom_time_domain.c/h        # 待迁移
│   │
│   ├── peec_orchestration/           # 新增：PEEC编排
│   │   ├── peec_algorithm_selector.c/h # 待创建
│   │   ├── peec_wideband.c/h          # 待迁移
│   │   └── peec_time_domain.c/h       # 待迁移
│   │
│   ├── mtl_orchestration/            # 新增：MTL编排
│   │   ├── mtl_wideband.c/h          # 待迁移
│   │   └── mtl_time_domain.c/h        # 待迁移
│   │
│   ├── circuit_coupling/              # 新增：电路耦合编排
│   │   └── circuit_coupling_orchestration.c/h # 待创建
│   │
│   ├── hybrid_solver/
│   ├── workflow/
│   └── execution/
│
├── solvers/ (L5)                     # 特定物理方法求解器核心
│   ├── mom/
│   │   ├── mom_solver_core.c/h       # 重命名：只做MoM求解一次
│   │   └── mom_solver_capabilities.h # 声明能力
│   ├── peec/
│   │   ├── peec_solver_core.c/h      # 重命名：只做PEEC求解一次
│   │   └── peec_solver_capabilities.h
│   └── mtl/
│       ├── mtl_solver_core.c/h       # 重命名：只做MTL求解一次
│       └── mtl_solver_capabilities.h
│
└── io/ (L6)                          # 输入输出
```

---

## 四、职责明确化（最终版）

### 4.1 `solvers/` - "会算"

**职责**：
- 特定物理方法的**核心求解能力**
- 矩阵组装（调用L3算子）
- 调用L4求解器
- 求解一次（单频点、单时间步）

**禁止**：
- ❌ 算法选择决策
- ❌ 宽带策略
- ❌ 时域/频域调度
- ❌ 多频点循环

### 4.2 `orchestration/` - "知道什么时候、怎么算"

**职责**：
- 算法选择决策
- 宽带策略
- 时域/频域调度
- 多频点循环
- 求解器能力描述

**禁止**：
- ❌ 数值kernel实现
- ❌ 物理公式实现

### 4.3 `operators/operator_approximation/` - "算子近似的数学模型"

**职责**：
- 算子近似的数学定义
- 误差控制理论
- 可容许性条件

**禁止**：
- ❌ 并行实现
- ❌ 内存管理
- ❌ GPU加速

### 4.4 `backend/algorithms/fast/` - "算子近似的计算实现"

**职责**：
- 算子近似的并行实现
- 内存管理
- GPU加速
- 调度策略

**禁止**：
- ❌ 算子数学定义

---

## 五、预期效果

### 5.1 降低认知负担

**之前**：
- "算法选择在哪？在 `solvers/` 还是 `orchestration/`？"
- "时域分析应该在哪？"

**之后**：
- "编排决策 → `orchestration/`"
- "核心求解 → `solvers/`"
- "时域/频域调度 → `orchestration/<solver>_orchestration/`"

### 5.2 提高新成员上手速度

**之前**：
- 需要理解 `mom_solver_unified.c` 中的混合逻辑（3000+行）
- 不清楚哪些是编排，哪些是核心求解

**之后**：
- 清晰的职责分离
- `mom_solver_core.c` - 只做核心求解（易于理解）
- `mom_orchestration/` - 编排逻辑（独立模块）

### 5.3 降低文件归属判断成本

**之前**：
- "这个函数应该放哪？" - 需要仔细分析代码

**之后**：
- 使用架构决策树，明确判断标准
- 编排逻辑 → `orchestration/`
- 核心求解 → `solvers/`

---

## 六、实施状态

### ✅ 已完成

1. ✅ 创建新目录结构
2. ✅ 创建 `mom_algorithm_selector.h/c`

### ⏳ 进行中

3. ⏳ 提取 MoM 编排逻辑
4. ⏳ 提取 PEEC 编排逻辑
5. ⏳ 提取 MTL 编排逻辑
6. ⏳ 拆分 physics 层
7. ⏳ 提取算子近似模型

### ⏳ 待执行

8. ⏳ 更新所有引用
9. ⏳ 更新项目文件
10. ⏳ 编译测试

---

**下一步**: 继续提取编排逻辑，然后更新引用和编译。
