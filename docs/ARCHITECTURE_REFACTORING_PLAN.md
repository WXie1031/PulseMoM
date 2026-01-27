# 架构重构详细计划

## 目标

根据架构决策树和商用EM软件标准，优化代码架构，降低：
- 认知负担（开发者）
- 新成员上手速度
- 文件归属判断成本

---

## 一、问题1：`solvers/` vs `orchestration/` 边界模糊

### 1.1 当前问题分析

#### `src/solvers/mom/mom_solver_unified.c`

**包含的编排逻辑**（应属于L5）：
- ✅ `select_algorithm()` - 算法选择（第515行）
- ✅ `compute_problem_characteristics()` - 问题特征分析（第552行）
- ✅ 宽带策略（`enable_frequency_interpolation`）
- ✅ 时域/频域切换逻辑

**应保留的核心逻辑**（属于L5但特定于MoM）：
- MoM矩阵组装流程
- MoM特定的求解步骤

#### `src/solvers/peec/peec_solver_unified.c`

**包含的编排逻辑**（应属于L5）：
- ✅ `select_peec_algorithm()` - PEEC算法选择（第302行）
- ✅ `compute_peec_problem_characteristics()` - 问题特征分析（第315行）
- ✅ 宽带策略
- ✅ 时域/频域切换

#### `src/solvers/mom/mom_time_domain.c` 和 `src/solvers/peec/peec_time_domain.c`

**问题**：时域/频域调度属于L5编排层，不应在 `solvers/` 中

#### `src/solvers/mom/mom_wideband.c` 和 `src/solvers/mtl/mtl_wideband.c`

**问题**：宽带策略属于L5编排层

### 1.2 重构方案

#### 方案A：引入 Solver Profile 概念（推荐）

**新结构**：
```
src/solvers/
├── mom/
│   ├── mom_solver_core.c/h          # 只做：MoM求解一次（单频点）
│   ├── mom_solver_core.h
│   └── mom_solver_capabilities.h    # 声明能力（支持哪些算法、特性）
│
├── peec/
│   ├── peec_solver_core.c/h         # 只做：PEEC求解一次
│   └── peec_solver_capabilities.h
│
└── mtl/
    ├── mtl_solver_core.c/h          # 只做：MTL求解一次
    └── mtl_solver_capabilities.h

src/orchestration/
├── solver_strategy/                  # 新增：求解器策略
│   ├── algorithm_selector.c/h       # 算法选择（通用）
│   ├── wideband_strategy.c/h        # 宽带策略（通用）
│   ├── time_frequency_scheduler.c/h # 时域/频域调度（通用）
│   └── solver_profile.c/h           # 求解器能力描述
│
├── mom_orchestration/                # MoM编排
│   ├── mom_wideband.c/h             # 从 solvers/mom/ 移入
│   ├── mom_time_domain.c/h          # 从 solvers/mom/ 移入
│   └── mom_algorithm_selector.c/h   # 从 mom_solver_unified.c 提取
│
├── peec_orchestration/               # PEEC编排
│   ├── peec_wideband.c/h
│   ├── peec_time_domain.c/h
│   └── peec_algorithm_selector.c/h
│
└── mtl_orchestration/                # MTL编排
    ├── mtl_wideband.c/h
    └── mtl_time_domain.c/h
```

**职责划分**：
- `solvers/` = "会算"（核心求解能力）
- `orchestration/` = "知道什么时候、怎么用它算"（编排决策）

### 1.3 具体迁移清单

#### 从 `solvers/mom/mom_solver_unified.c` 提取：

1. **算法选择逻辑** → `orchestration/mom_orchestration/mom_algorithm_selector.c/h`
   - `select_algorithm()` 函数
   - `compute_problem_characteristics()` 函数
   - `mom_problem_characteristics_t` 结构

2. **宽带策略** → `orchestration/mom_orchestration/mom_wideband.c/h`
   - 从 `mom_solver_unified.c` 中的宽带相关代码
   - `mom_time_domain.c` → `orchestration/mom_orchestration/mom_time_domain.c/h`

3. **保留在 `solvers/mom/`**：
   - 矩阵组装逻辑（调用L3算子）
   - 调用L4求解器
   - MoM特定的求解步骤

#### 从 `solvers/peec/peec_solver_unified.c` 提取：

1. **算法选择逻辑** → `orchestration/peec_orchestration/peec_algorithm_selector.c/h`
2. **宽带策略** → `orchestration/peec_orchestration/peec_wideband.c/h`
3. **时域分析** → `orchestration/peec_orchestration/peec_time_domain.c/h`

#### 从 `solvers/mtl/` 提取：

1. `mtl_wideband.c/h` → `orchestration/mtl_orchestration/mtl_wideband.c/h`
2. `mtl_time_domain.c/h` → `orchestration/mtl_orchestration/mtl_time_domain.c/h`

---

## 二、问题2：`backend/algorithms/fast/` vs `operators/` 算法语义被拆裂

### 2.1 当前问题分析

#### `backend/algorithms/fast/aca.h` 和 `h_matrix_compression.h`

**问题**：
- ACA 和 H-matrix 在数学上是**算子近似**（改变算子数学意义）
- 应该属于 L3 层（算子层）
- 但当前放在 L4 层（数值后端）

**架构决策树 Q3**：
> Does this code approximate, compress, or reformulate an operator?
> ✅ YES → `operators/operator_approximation/` (L3)

### 2.2 重构方案

#### 创建 `operators/operator_approximation/` 目录

**新结构**：
```
src/operators/
├── operator_approximation/          # 新增：算子近似模型（L3）
│   ├── aca_operator_model.h         # ACA算子模型定义（数学）
│   ├── hmatrix_operator_model.h     # H-矩阵算子模型定义（数学）
│   ├── fmm_operator_model.h          # FMM算子模型定义（数学）
│   └── approximation_interface.h    # 算子近似统一接口
│
└── ... (其他算子)

src/backend/
├── algorithms/
│   └── fast/                        # 保留：快速算法实现（L4）
│       ├── aca_implementation.c/h   # ACA实现（并行、存储、调度）
│       ├── hmatrix_implementation.c/h # H-矩阵实现
│       └── fmm_implementation.c/h   # FMM实现
```

**职责划分**：
- **L3 (`operators/operator_approximation/`)**: 定义算子近似的**数学模型**
  - 低秩分解的数学定义
  - 可容许性条件
  - 误差控制理论
- **L4 (`backend/algorithms/fast/`)**: 实现算子近似的**计算方式**
  - 并行计算
  - 内存布局
  - GPU加速
  - 调度策略

### 2.3 具体迁移清单

#### 从 `backend/algorithms/fast/` 提取数学模型部分：

1. **ACA数学模型** → `operators/operator_approximation/aca_operator_model.h`
   - 低秩分解的数学定义
   - 误差控制理论
   - 可容许性条件

2. **H-matrix数学模型** → `operators/operator_approximation/hmatrix_operator_model.h`
   - 层次结构定义
   - 块聚类树定义
   - 可容许性条件

3. **保留在 `backend/algorithms/fast/`**：
   - 并行实现
   - 内存管理
   - GPU加速
   - 调度逻辑

---

## 三、问题3：`physics/` 层略"厚"

### 3.1 当前问题分析

#### `src/physics/peec/circuit_coupling_simulation.c` (756行)

**包含的内容**：
- ❌ MNA矩阵构建（数值实现）
- ❌ 非线性求解器（数值实现）
- ❌ 时间推进逻辑（编排）
- ❌ 耦合流程（编排）

**架构决策树分析**：
- Q1: 定义物理方程？ → 部分（电路耦合的物理定义）
- Q5: 决定执行顺序？ → ✅ YES → 应属于 L5

**结论**: 这个文件混合了L1（物理定义）和L5（编排），需要拆分。

#### `src/physics/peec/peec_circuit.c`

**需要检查**: 是否包含数值实现或编排逻辑

### 3.2 重构方案

#### L1层只允许三类文件：

1. **方程形式**（symbolic / weak form）
2. **本构关系**（constitutive relations）
3. **边界条件定义**

#### 从 `physics/peec/circuit_coupling_simulation.c` 拆分：

1. **物理定义部分** → 保留在 `physics/peec/`
   - 电路耦合的物理定义
   - 耦合方程的数学形式

2. **数值实现部分** → 移动到 `orchestration/circuit_coupling/`
   - MNA矩阵构建
   - 非线性求解器调用
   - 时间推进逻辑
   - 耦合流程编排

---

## 四、问题4：`common/` + `utils/` + `materials/` 可再收敛

### 4.1 当前问题分析

**语义漂移**：
- `materials/` - 有物理意义（材料本构关系）
- `common/` - 有接口意义（类型、常量、接口）
- `utils/` - 有工程意义（工具函数）

### 4.2 重构方案

#### 明确职责边界（不改变目录结构，只明确规则）

**`common/`** - 只允许：
- 类型定义（`types.h`）
- 常量定义（`constants.h`）
- 层间接口定义（`layer_interfaces.h`）
- 错误定义（`errors.h`）
- ❌ 禁止：工具函数、物理语义

**`utils/`** - 只允许：
- 无物理语义的工具函数
- 日志、性能监控、验证
- ❌ 禁止：物理常数、材料定义

**`materials/`** - 只允许：
- 材料数据（材料库）
- 材料本构模型（物理定义）
- ❌ 禁止：通用工具函数

---

## 五、问题5：重复代码和冗余文件

### 5.1 重复代码识别

#### 算法选择逻辑重复

- `mom_solver_unified.c` 中的 `select_algorithm()`
- `peec_solver_unified.c` 中的 `select_peec_algorithm()`

**解决方案**: 提取通用算法选择逻辑到 `orchestration/solver_strategy/algorithm_selector.c/h`

#### 时域/频域切换逻辑重复

- `mom_time_domain.c`
- `peec_time_domain.c`
- `mtl_time_domain.c`

**解决方案**: 提取通用时域/频域调度到 `orchestration/solver_strategy/time_frequency_scheduler.c/h`

### 5.2 冗余文件识别

#### `mom_solver_min.c` vs `mom_solver_unified.c`

**分析**: 
- `mom_solver_min.c` - 最小实现
- `mom_solver_unified.c` - 完整实现

**建议**: 
- 如果 `mom_solver_min.c` 仍在使用，保留
- 如果只是历史遗留，考虑删除或合并

#### `peec_solver_min.c` vs `peec_solver_unified.c`

**同上**

---

## 六、最终架构设计

### 6.1 目录结构（优化后）

```
src/
│
├── physics/ (L1)                    # 只包含物理定义
│   ├── mom/
│   │   └── mom_physics.c/h          # EFIE, MFIE, CFIE方程
│   ├── peec/
│   │   ├── peec_physics.c/h         # PEEC物理方程
│   │   ├── peec_circuit.h           # 电路模型定义（纯定义）
│   │   └── peec_circuit_coupling.h  # 耦合物理定义（纯定义）
│   └── mtl/
│       └── mtl_physics.c/h          # 传输线方程
│
├── discretization/ (L2)             # 离散化
│   ├── geometry/
│   ├── mesh/
│   └── basis/
│
├── operators/ (L3)                   # 算子层
│   ├── kernels/
│   ├── integration/
│   ├── assembler/
│   ├── matvec/
│   ├── coupling/
│   └── operator_approximation/      # 新增：算子近似模型
│       ├── aca_operator_model.h
│       ├── hmatrix_operator_model.h
│       └── approximation_interface.h
│
├── backend/ (L4)                     # 数值后端
│   ├── solvers/                      # 通用数值求解器
│   ├── algorithms/
│   │   └── fast/                     # 快速算法实现（保留）
│   ├── gpu/
│   └── math/
│
├── orchestration/ (L5)               # 执行编排（核心）
│   ├── solver_strategy/              # 新增：通用求解器策略
│   │   ├── algorithm_selector.c/h    # 通用算法选择
│   │   ├── wideband_strategy.c/h     # 通用宽带策略
│   │   ├── time_frequency_scheduler.c/h # 时域/频域调度
│   │   └── solver_profile.c/h        # 求解器能力描述
│   │
│   ├── mom_orchestration/            # 新增：MoM编排
│   │   ├── mom_algorithm_selector.c/h
│   │   ├── mom_wideband.c/h
│   │   └── mom_time_domain.c/h
│   │
│   ├── peec_orchestration/           # 新增：PEEC编排
│   │   ├── peec_algorithm_selector.c/h
│   │   ├── peec_wideband.c/h
│   │   └── peec_time_domain.c/h
│   │
│   ├── mtl_orchestration/            # 新增：MTL编排
│   │   ├── mtl_wideband.c/h
│   │   └── mtl_time_domain.c/h
│   │
│   ├── circuit_coupling/              # 新增：电路耦合编排
│   │   └── circuit_coupling_orchestration.c/h
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
│       ├── mtl_solver_core.c/h      # 重命名：只做MTL求解一次
│       └── mtl_solver_capabilities.h
│
├── io/ (L6)                          # 输入输出
│
├── common/                           # 公共定义（类型、常量、接口）
├── utils/                            # 工具函数（无物理语义）
└── materials/                        # 材料定义（物理语义）
```

### 6.2 职责明确化

| 目录 | 职责 | 禁止 |
|------|------|------|
| `physics/` | 物理方程、本构关系、边界条件 | 数值实现、时间推进、编排逻辑 |
| `solvers/` | 特定求解器的核心能力（"会算"） | 算法选择、宽带策略、时域/频域调度 |
| `orchestration/` | 编排决策（"知道什么时候、怎么算"） | 数值kernel、物理公式 |
| `operators/operator_approximation/` | 算子近似的数学模型 | 并行实现、内存管理 |
| `backend/algorithms/fast/` | 算子近似的计算实现 | 算子数学定义 |

---

## 七、文件迁移清单

### 7.1 从 `solvers/` 迁移到 `orchestration/`

| 源文件 | 目标位置 | 迁移内容 |
|--------|---------|---------|
| `solvers/mom/mom_solver_unified.c` | `orchestration/mom_orchestration/mom_algorithm_selector.c/h` | `select_algorithm()`, `compute_problem_characteristics()` |
| `solvers/mom/mom_time_domain.c/h` | `orchestration/mom_orchestration/mom_time_domain.c/h` | 完整文件 |
| `solvers/peec/peec_solver_unified.c` | `orchestration/peec_orchestration/peec_algorithm_selector.c/h` | `select_peec_algorithm()`, `compute_peec_problem_characteristics()` |
| `solvers/peec/peec_time_domain.c/h` | `orchestration/peec_orchestration/peec_time_domain.c/h` | 完整文件 |
| `solvers/mtl/mtl_wideband.c/h` | `orchestration/mtl_orchestration/mtl_wideband.c/h` | 完整文件 |
| `solvers/mtl/mtl_time_domain.c/h` | `orchestration/mtl_orchestration/mtl_time_domain.c/h` | 完整文件 |

### 7.2 从 `physics/` 迁移到 `orchestration/`

| 源文件 | 目标位置 | 迁移内容 |
|--------|---------|---------|
| `physics/peec/circuit_coupling_simulation.c` | `orchestration/circuit_coupling/circuit_coupling_orchestration.c/h` | MNA矩阵构建、非线性求解、时间推进、耦合流程 |
| `physics/peec/circuit_coupling_simulation.c` | `physics/peec/` (保留) | 物理定义部分（耦合方程的数学形式） |

### 7.3 从 `backend/algorithms/fast/` 提取到 `operators/operator_approximation/`

| 源文件 | 目标位置 | 迁移内容 |
|--------|---------|---------|
| `backend/algorithms/fast/aca.h` | `operators/operator_approximation/aca_operator_model.h` | ACA数学模型定义 |
| `backend/algorithms/fast/h_matrix_compression.h` | `operators/operator_approximation/hmatrix_operator_model.h` | H-matrix数学模型定义 |

---

## 八、实施步骤

### 阶段1：分析和准备（当前阶段）

1. ✅ 完成文件分析
2. ⏳ 创建详细迁移清单
3. ⏳ 备份当前代码

### 阶段2：创建新目录结构

1. 创建 `orchestration/solver_strategy/`
2. 创建 `orchestration/mom_orchestration/`
3. 创建 `orchestration/peec_orchestration/`
4. 创建 `orchestration/mtl_orchestration/`
5. 创建 `orchestration/circuit_coupling/`
6. 创建 `operators/operator_approximation/`

### 阶段3：提取和迁移代码

1. 从 `solvers/` 提取编排逻辑到 `orchestration/`
2. 从 `physics/` 提取编排逻辑到 `orchestration/`
3. 从 `backend/algorithms/fast/` 提取数学模型到 `operators/operator_approximation/`

### 阶段4：重构核心求解器

1. 重构 `mom_solver_unified.c` → `mom_solver_core.c`
2. 重构 `peec_solver_unified.c` → `peec_solver_core.c`
3. 重构 `mtl_solver.c` → `mtl_solver_core.c`

### 阶段5：更新引用和编译

1. 更新所有 `#include` 路径
2. 更新项目文件（`.vcxproj`）
3. 编译测试，修复错误

---

## 九、预期效果

### 9.1 降低认知负担

- **之前**: "算法选择在哪？在 `solvers/` 还是 `orchestration/`？"
- **之后**: "编排决策 → `orchestration/`，核心求解 → `solvers/`"

### 9.2 提高新成员上手速度

- **之前**: 需要理解 `mom_solver_unified.c` 中的混合逻辑
- **之后**: 清晰的职责分离，易于理解

### 9.3 降低文件归属判断成本

- **之前**: "这个函数应该放哪？"
- **之后**: 使用架构决策树，明确判断标准

---

**下一步**: 开始实施阶段1，逐个文件分析并创建详细迁移清单。
