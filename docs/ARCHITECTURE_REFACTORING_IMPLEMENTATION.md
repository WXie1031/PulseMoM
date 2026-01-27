# 架构重构实施计划

## 目标

根据架构决策树和商用EM软件标准，优化代码架构，降低认知负担和文件归属判断成本。

---

## 阶段1：创建新目录结构 ✅

**已完成**：
- ✅ `orchestration/solver_strategy/`
- ✅ `orchestration/mom_orchestration/`
- ✅ `orchestration/peec_orchestration/`
- ✅ `orchestration/mtl_orchestration/`
- ✅ `orchestration/circuit_coupling/`
- ✅ `operators/operator_approximation/`

---

## 阶段2：提取 MoM 编排逻辑

### 2.1 创建 MoM 算法选择器

**文件**: `orchestration/mom_orchestration/mom_algorithm_selector.c/h`

**从 `solvers/mom/mom_solver_unified.c` 提取**：
- `select_algorithm()` 函数（第515行）
- `compute_problem_characteristics()` 函数（第552行）
- `detect_curved_surfaces()` 函数（第599行）
- `count_different_materials()` 函数（第662行）
- `mom_problem_characteristics_t` 结构（第322行）

**依赖**：
- `discretization/mesh/core_mesh.h` (L2)
- `common/types.h` (公共层)

### 2.2 迁移时域/频域文件

**迁移**：
- `solvers/mom/mom_time_domain.c/h` → `orchestration/mom_orchestration/mom_time_domain.c/h`

---

## 阶段3：提取 PEEC 编排逻辑

### 3.1 创建 PEEC 算法选择器

**文件**: `orchestration/peec_orchestration/peec_algorithm_selector.c/h`

**从 `solvers/peec/peec_solver_unified.c` 提取**：
- `select_peec_algorithm()` 函数
- `compute_peec_problem_characteristics()` 函数

### 3.2 迁移时域文件

**迁移**：
- `solvers/peec/peec_time_domain.c/h` → `orchestration/peec_orchestration/peec_time_domain.c/h`

---

## 阶段4：提取 MTL 编排逻辑

### 4.1 迁移宽带和时域文件

**迁移**：
- `solvers/mtl/mtl_wideband.c/h` → `orchestration/mtl_orchestration/mtl_wideband.c/h`
- `solvers/mtl/mtl_time_domain.c/h` → `orchestration/mtl_orchestration/mtl_time_domain.c/h`

---

## 阶段5：拆分 physics 层

### 5.1 拆分 `circuit_coupling_simulation.c`

**保留在 `physics/peec/`**：
- 耦合方程的物理定义
- 物理接口声明

**移动到 `orchestration/circuit_coupling/`**：
- MNA矩阵构建逻辑
- 非线性求解器调用
- 时间推进逻辑
- 耦合流程编排

---

## 阶段6：提取算子近似模型

### 6.1 创建算子近似模型定义

**文件**: `operators/operator_approximation/aca_operator_model.h`

**从 `backend/algorithms/fast/aca.h` 提取**：
- ACA数学定义（低秩分解理论）
- 误差控制理论
- 可容许性条件

**保留在 `backend/algorithms/fast/`**：
- ACA实现（并行、内存、GPU）

### 6.2 创建 H-matrix 算子模型定义

**文件**: `operators/operator_approximation/hmatrix_operator_model.h`

**从 `backend/algorithms/fast/h_matrix_compression.h` 提取**：
- H-matrix数学定义
- 层次结构定义
- 块聚类树定义
- 可容许性条件

---

## 阶段7：更新引用和编译

### 7.1 更新 include 路径

更新所有引用迁移文件的 `#include` 语句。

### 7.2 更新项目文件

更新 `.vcxproj` 文件，添加新文件，移除旧引用。

### 7.3 编译测试

使用 task.json 编译项目，修复所有编译错误。

---

## 实施顺序

1. ✅ 创建新目录结构
2. ⏳ 提取 MoM 编排逻辑
3. ⏳ 提取 PEEC 编排逻辑
4. ⏳ 提取 MTL 编排逻辑
5. ⏳ 拆分 physics 层
6. ⏳ 提取算子近似模型
7. ⏳ 更新引用和编译

---

**当前状态**: 阶段1完成，开始阶段2
