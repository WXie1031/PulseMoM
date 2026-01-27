# 逐文件架构分析报告

## 分析目标

根据架构决策树，逐个文件分析其正确的架构层级，识别需要迁移的代码。

---

## 一、`solvers/` 目录文件分析

### 1.1 `src/solvers/mom/mom_solver_unified.c` (3113行)

#### 架构决策树分析

**Q1**: 定义物理方程？ → ❌ NO  
**Q2**: 数值离散化？ → ❌ NO（离散化在L2层）  
**Q3**: 改变算子数学意义？ → ❌ NO  
**Q4**: 只改变执行方式？ → ❌ NO  
**Q5**: 决定何时/哪个求解器？ → ✅ **YES** → 应属于 L5 orchestration  
**Q6**: 特定求解器逻辑？ → ✅ **YES** → 但包含编排逻辑

#### 代码职责分析

| 函数/代码段 | 行号 | 职责 | 应属层级 | 建议操作 |
|------------|------|------|---------|---------|
| `select_algorithm()` | 515 | 算法选择决策 | L5 orchestration | 提取到 `orchestration/mom_orchestration/mom_algorithm_selector.c/h` |
| `compute_problem_characteristics()` | 552 | 问题特征分析（用于算法选择） | L5 orchestration | 同上 |
| `mom_solve_unified()` | 2695 | 主入口（包含算法选择） | L5 orchestration | 拆分：编排部分→orchestration，核心部分→solvers |
| `assemble_impedance_matrix_basic()` | 995 | MoM矩阵组装（核心求解） | L5 solvers | 保留，重命名为 `mom_solver_core.c` |
| `assemble_impedance_matrix_aca()` | 1466 | MoM矩阵组装（使用ACA） | L5 solvers | 保留 |
| `solve_linear_system_basic()` | 2092 | 调用L4求解器 | L5 solvers | 保留 |
| `solve_linear_system_iterative()` | 2353 | 调用L4求解器 | L5 solvers | 保留 |

#### 重构方案

**拆分策略**：
1. **提取编排逻辑** → `orchestration/mom_orchestration/`
   - `select_algorithm()` → `mom_algorithm_selector.c/h`
   - `compute_problem_characteristics()` → `mom_algorithm_selector.c/h`
   - `mom_solve_unified()` 中的算法选择部分 → `mom_orchestration.c/h`

2. **保留核心求解** → `solvers/mom/mom_solver_core.c/h`
   - `assemble_impedance_matrix_*()` 函数
   - `solve_linear_system_*()` 函数
   - MoM特定的矩阵组装流程

### 1.2 `src/solvers/mom/mom_time_domain.c/h`

#### 架构决策树分析

**Q5**: 决定时域/频域？ → ✅ **YES** → 应属于 L5 orchestration

#### 重构方案

**迁移**: `solvers/mom/mom_time_domain.c/h` → `orchestration/mom_orchestration/mom_time_domain.c/h`

### 1.3 `src/solvers/peec/peec_solver_unified.c`

#### 代码职责分析

| 函数/代码段 | 行号 | 职责 | 应属层级 | 建议操作 |
|------------|------|------|---------|---------|
| `select_peec_algorithm()` | 302 | PEEC算法选择决策 | L5 orchestration | 提取到 `orchestration/peec_orchestration/peec_algorithm_selector.c/h` |
| `compute_peec_problem_characteristics()` | 315 | 问题特征分析 | L5 orchestration | 同上 |
| `extract_partial_elements_*()` | - | PEEC部分元件提取（核心求解） | L5 solvers | 保留 |
| `assemble_circuit_matrix()` | - | 电路矩阵组装（核心求解） | L5 solvers | 保留 |

#### 重构方案

**拆分策略**：同 MoM，提取编排逻辑到 `orchestration/peec_orchestration/`

### 1.4 `src/solvers/peec/peec_time_domain.c/h`

#### 重构方案

**迁移**: `solvers/peec/peec_time_domain.c/h` → `orchestration/peec_orchestration/peec_time_domain.c/h`

### 1.5 `src/solvers/mtl/mtl_wideband.c/h` 和 `mtl_time_domain.c/h`

#### 重构方案

**迁移**: 
- `solvers/mtl/mtl_wideband.c/h` → `orchestration/mtl_orchestration/mtl_wideband.c/h`
- `solvers/mtl/mtl_time_domain.c/h` → `orchestration/mtl_orchestration/mtl_time_domain.c/h`

---

## 二、`physics/` 目录文件分析

### 2.1 `src/physics/peec/circuit_coupling_simulation.c` (756行)

#### 架构决策树分析

**Q1**: 定义物理方程？ → ⚠️ 部分（耦合方程的物理定义）  
**Q5**: 决定执行顺序？ → ✅ **YES** → 包含编排逻辑

#### 代码职责分析

| 函数/代码段 | 行号 | 职责 | 应属层级 | 建议操作 |
|------------|------|------|---------|---------|
| `build_mna_matrix()` | 424 | MNA矩阵构建（数值实现） | L5 orchestration | 移动到 `orchestration/circuit_coupling/` |
| `solve_nonlinear_newton()` | 735 | 非线性求解（数值实现） | L5 orchestration | 移动到 `orchestration/circuit_coupling/` |
| `solve_nonlinear_homotopy()` | 767 | 非线性求解（数值实现） | L5 orchestration | 移动到 `orchestration/circuit_coupling/` |
| 耦合流程逻辑 | - | 电路-EM耦合编排 | L5 orchestration | 移动到 `orchestration/circuit_coupling/` |
| 物理定义部分 | - | 耦合方程的数学形式 | L1 physics | 保留在 `physics/peec/` |

#### 重构方案

**拆分策略**：
1. **提取编排逻辑** → `orchestration/circuit_coupling/circuit_coupling_orchestration.c/h`
   - MNA矩阵构建
   - 非线性求解器调用
   - 时间推进逻辑
   - 耦合流程编排

2. **保留物理定义** → `physics/peec/peec_circuit_coupling.h` (已存在)
   - 耦合方程的数学形式
   - 物理接口定义

### 2.2 `src/physics/peec/peec_circuit.c`

#### 需要检查

检查是否包含数值实现或编排逻辑。

---

## 三、`backend/algorithms/fast/` 目录文件分析

### 3.1 `src/backend/algorithms/fast/aca.h` 和 `aca.c`

#### 架构决策树分析

**Q3**: 改变算子数学意义？ → ✅ **YES** → 应属于 L3 operators

#### 代码职责分析

| 内容 | 职责 | 应属层级 | 建议操作 |
|------|------|---------|---------|
| ACA数学模型定义 | 低秩分解的数学定义 | L3 operators | 提取到 `operators/operator_approximation/aca_operator_model.h` |
| ACA实现（并行、内存） | 计算实现 | L4 backend | 保留在 `backend/algorithms/fast/aca_implementation.c/h` |

#### 重构方案

**拆分策略**：
1. **提取数学模型** → `operators/operator_approximation/aca_operator_model.h`
   - 低秩分解的数学定义
   - 误差控制理论
   - 可容许性条件

2. **保留实现** → `backend/algorithms/fast/aca_implementation.c/h`
   - 并行计算
   - 内存管理
   - GPU加速

### 3.2 `src/backend/algorithms/fast/h_matrix_compression.h`

#### 重构方案

**拆分策略**：同ACA
- 数学模型 → `operators/operator_approximation/hmatrix_operator_model.h`
- 实现 → `backend/algorithms/fast/hmatrix_implementation.c/h`

---

## 四、重复代码识别

### 4.1 算法选择逻辑重复

- `mom_solver_unified.c`: `select_algorithm()` (515行)
- `peec_solver_unified.c`: `select_peec_algorithm()` (302行)

**解决方案**: 提取通用算法选择框架到 `orchestration/solver_strategy/algorithm_selector.c/h`

### 4.2 时域/频域切换逻辑重复

- `mom_time_domain.c/h`
- `peec_time_domain.c/h`
- `mtl_time_domain.c/h`

**解决方案**: 提取通用时域/频域调度到 `orchestration/solver_strategy/time_frequency_scheduler.c/h`

---

## 五、冗余文件识别

### 5.1 `mom_solver_min.c` vs `mom_solver_unified.c`

**分析**: 
- `mom_solver_min.c` - 最小实现
- `mom_solver_unified.c` - 完整实现

**建议**: 
- 检查 `mom_solver_min.c` 是否仍被使用
- 如果未使用，标记为可删除
- 如果使用，保留但明确其用途

### 5.2 `peec_solver_min.c` vs `peec_solver_unified.c`

**同上**

---

## 六、实施优先级

### 高优先级（立即执行）

1. ✅ **创建新目录结构**
   - `orchestration/solver_strategy/`
   - `orchestration/mom_orchestration/`
   - `orchestration/peec_orchestration/`
   - `orchestration/mtl_orchestration/`
   - `orchestration/circuit_coupling/`
   - `operators/operator_approximation/`

2. ✅ **提取编排逻辑**
   - 从 `solvers/mom/mom_solver_unified.c` 提取算法选择
   - 迁移时域/频域文件到 orchestration

3. ✅ **拆分 physics 层**
   - 拆分 `circuit_coupling_simulation.c`

### 中优先级

4. ⏳ **提取算子近似模型**
   - 从 `backend/algorithms/fast/` 提取数学模型

5. ⏳ **重构核心求解器**
   - 重命名 `*_unified.c` → `*_core.c`

### 低优先级

6. ⏳ **清理冗余文件**
   - 检查 `*_min.c` 文件的使用情况

---

**下一步**: 开始实施高优先级任务。
