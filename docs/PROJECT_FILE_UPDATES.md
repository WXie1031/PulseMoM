# 项目文件更新记录

## 更新内容

### 1. 移除旧文件引用

从 `PulseMoM_Core.vcxproj` 中移除了以下文件（已迁移到 orchestration 层）：

**源文件 (ClCompile)**:
- `solvers\mom\mom_time_domain.c` → 迁移到 `orchestration\mom_orchestration\mom_time_domain.c`
- `solvers\peec\peec_time_domain.c` → 迁移到 `orchestration\peec_orchestration\peec_time_domain.c`
- `solvers\mtl\mtl_wideband.c` → 迁移到 `orchestration\mtl_orchestration\mtl_wideband.c`
- `solvers\mtl\mtl_time_domain.c` → 迁移到 `orchestration\mtl_orchestration\mtl_time_domain.c`

**头文件 (ClInclude)**:
- `solvers\mom\mom_time_domain.h` → 迁移到 `orchestration\mom_orchestration\mom_time_domain.h`
- `solvers\peec\peec_time_domain.h` → 迁移到 `orchestration\peec_orchestration\peec_time_domain.h`
- `solvers\mtl\mtl_wideband.h` → 迁移到 `orchestration\mtl_orchestration\mtl_wideband.h`
- `solvers\mtl\mtl_time_domain.h` → 迁移到 `orchestration\mtl_orchestration\mtl_time_domain.h`

### 2. 添加新文件引用

**L5 Orchestration Layer - 编排逻辑**:

源文件 (ClCompile):
- `orchestration\mom_orchestration\mom_time_domain.c`
- `orchestration\mom_orchestration\mom_algorithm_selector.c`
- `orchestration\peec_orchestration\peec_time_domain.c`
- `orchestration\peec_orchestration\peec_algorithm_selector.c`
- `orchestration\mtl_orchestration\mtl_wideband.c`
- `orchestration\mtl_orchestration\mtl_time_domain.c`
- `orchestration\circuit_coupling\circuit_coupling_orchestration.c`

头文件 (ClInclude):
- `orchestration\mom_orchestration\mom_time_domain.h`
- `orchestration\mom_orchestration\mom_algorithm_selector.h`
- `orchestration\peec_orchestration\peec_time_domain.h`
- `orchestration\peec_orchestration\peec_algorithm_selector.h`
- `orchestration\mtl_orchestration\mtl_wideband.h`
- `orchestration\mtl_orchestration\mtl_time_domain.h`
- `orchestration\circuit_coupling\circuit_coupling_orchestration.h`

**L3 Operator Approximation Models - 算子近似模型**:

头文件 (ClInclude):
- `operators\operator_approximation\aca_operator_model.h`
- `operators\operator_approximation\hmatrix_operator_model.h`

---

## 架构对齐

所有新文件都按照六层架构正确放置：

- **L3 (Operators)**: 算子近似模型定义
- **L5 (Orchestration)**: 编排逻辑（算法选择、时域/频域调度）

---

## 下一步

1. ✅ 更新项目文件 - 已完成
2. ⏳ 编译测试 - 使用 task.json 编译项目
3. ⏳ 修复编译错误 - 根据编译结果修复

---

**状态**: 项目文件已更新，准备编译测试。
