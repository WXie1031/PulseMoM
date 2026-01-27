# Solvers 目录结构合理性分析

## 问题提出

用户质疑：`src/backend/solvers` (L4) 与 `src/solvers` 两个目录的区别是什么？为什么代码文件夹层次这么复杂？

## 当前目录结构

```
src/
├── backend/
│   └── solvers/          # L4 数值后端层
│       ├── solver_interface.h/c
│       ├── direct_solver.c/h
│       ├── iterative_solver.c/h
│       ├── core_solver.c/h
│       └── unified_interface.c/h  ⚠️ 架构违规
│
└── solvers/              # L5 执行编排层
    ├── mom/
    ├── peec/
    └── mtl/
```

## 架构职责分析

### 1. `src/backend/solvers` (L4 数值后端层)

**职责**：提供通用的线性系统求解算法

**特点**：
- ✅ **物理无关**：只处理矩阵，不关心物理含义
- ✅ **算法通用**：GMRES, CG, BiCGSTAB, LU, QR 等
- ✅ **接口抽象**：`solver_interface_t` 统一接口
- ✅ **可复用**：任何需要求解线性系统的场景都可以使用

**包含内容**：
- `solver_interface.h` - 统一求解器接口
- `direct_solver.c` - 直接求解器（LU, QR, Cholesky）
- `iterative_solver.c` - 迭代求解器（GMRES, CG, BiCGSTAB）
- `core_solver.c` - 核心求解器实现

**示例代码**：
```c
// L4层：只处理矩阵，不关心物理
solver_interface_t* solver = iterative_solver_create(&config);
int status = iterative_solver_solve(solver, matrix, rhs, solution, &stats);
```

### 2. `src/solvers` (L5 执行编排层)

**职责**：编排特定物理方法的完整求解流程

**特点**：
- ✅ **物理相关**：MoM, PEEC, MTL 特定方法
- ✅ **编排逻辑**：算法选择、矩阵组装、调用L4求解器
- ✅ **完整流程**：从输入到输出的完整仿真流程
- ✅ **使用L4**：调用 `backend/solvers` 的通用求解器

**包含内容**：
- `mom/mom_solver_unified.c` - MoM求解器编排（算法选择、矩阵组装、调用L4）
- `peec/peec_solver_unified.c` - PEEC求解器编排
- `mtl/mtl_solver.c` - MTL求解器编排

**示例代码**：
```c
// L5层：编排MoM求解流程
mom_solver_t* solver = mom_solver_create();
mom_config_t config = {...};  // 包含物理参数（频率、激励等）
mom_solver_analyze(solver, &config, geometry, &results);
// 内部会：
// 1. 选择算法（根据问题规模）
// 2. 组装矩阵（调用L3算子）
// 3. 调用L4求解器（backend/solvers）
```

## 架构关系图

```
┌─────────────────────────────────────────┐
│  src/physics (L1)                      │
│  - 定义物理方程 (EFIE, MFIE, CFIE)      │
│  - 定义边界条件                          │
└─────────────────────────────────────────┘
              ↓ 被使用
┌─────────────────────────────────────────┐
│  src/solvers (L5)                       │
│  - MoM/PEEC/MTL 求解器编排               │
│  - 算法选择逻辑                          │
│  - 矩阵组装编排                          │
│  - 调用 L4 求解器                        │
└─────────────────────────────────────────┘
              ↓ 使用
┌─────────────────────────────────────────┐
│  src/backend/solvers (L4)               │
│  - 通用数值求解算法                      │
│  - GMRES, CG, LU, QR 等                 │
│  - 只处理矩阵，不关心物理                │
└─────────────────────────────────────────┘
```

## 发现的问题

### ❌ 问题1：架构违规

**位置**：`src/backend/solvers/unified_interface.c`

**违规代码**：
```c
#include "../solvers/mom/mom_solver.h"      // L4 依赖 L5 ❌
#include "../solvers/peec/peec_solver.h"    // L4 依赖 L5 ❌
#include "../solvers/mtl/mtl_solver_module.h" // L4 依赖 L5 ❌
```

**问题**：
- L4层（数值后端）不应该依赖L5层（编排层）
- 违反了"低层不依赖高层"的架构原则
- `unified_interface.c` 试图在L4层统一L5层的接口，这是错误的

**解决方案**：
1. **选项A（推荐）**：将 `unified_interface.c` 移动到 `src/orchestration/` 或 `src/solvers/`
   - 统一接口属于编排层（L5），不属于数值后端（L4）
   
2. **选项B**：删除 `unified_interface.c`，在L5层实现统一接口
   - 每个物理方法的求解器已经在L5层，不需要在L4层统一

### ⚠️ 问题2：目录命名容易混淆

**当前命名**：
- `src/backend/solvers` - L4层
- `src/solvers` - L5层

**问题**：
- 两个目录都叫 "solvers"，容易混淆
- 新开发者可能不知道区别

**建议重命名**（可选）：
- `src/backend/solvers` → 保持不变（L4数值后端）
- `src/solvers` → `src/orchestration/solvers` 或 `src/solvers` 保持不变
  - 如果已经有 `src/orchestration/` 目录，可以考虑合并

## 架构合理性评估

### ✅ 合理的部分

1. **职责分离清晰**：
   - L4层：通用数值算法（可复用）
   - L5层：特定物理方法编排（业务逻辑）

2. **依赖方向正确**：
   - L5 → L4 ✅（高层使用低层）
   - L4 → L5 ❌（低层不应依赖高层，但 `unified_interface.c` 违规）

3. **符合六层架构**：
   - L4 数值后端：`backend/solvers` ✅
   - L5 执行编排：`solvers` ✅

### ⚠️ 需要改进的部分

1. **修复架构违规**：
   - 移动或删除 `backend/solvers/unified_interface.c`
   
2. **考虑重命名**（可选）：
   - 如果 `src/orchestration/` 已存在，考虑将 `src/solvers` 合并进去
   - 或者重命名为更明确的名称

## 建议的目录结构（优化后）

### 选项A：保持当前结构，修复违规

```
src/
├── backend/
│   └── solvers/          # L4 数值后端（保持不变）
│       ├── solver_interface.h/c
│       ├── direct_solver.c/h
│       ├── iterative_solver.c/h
│       └── core_solver.c/h
│       # ❌ 删除 unified_interface.c（移动到L5层）
│
└── solvers/              # L5 执行编排（保持不变）
    ├── mom/
    ├── peec/
    ├── mtl/
    └── unified_interface.c/h  # ✅ 从L4层移入
```

### 选项B：合并到 orchestration（如果存在）

```
src/
├── backend/
│   └── solvers/          # L4 数值后端
│
└── orchestration/        # L5 执行编排
    ├── solvers/          # 从 src/solvers 移入
    │   ├── mom/
    │   ├── peec/
    │   └── mtl/
    ├── hybrid_solver/    # 混合求解器
    └── workflows/        # 工作流
```

## 总结

### 两个目录的区别

| 维度 | `src/backend/solvers` (L4) | `src/solvers` (L5) |
|------|---------------------------|-------------------|
| **层级** | L4 数值后端 | L5 执行编排 |
| **职责** | 通用线性系统求解算法 | 特定物理方法求解编排 |
| **输入** | 矩阵 + 右端项 | 几何 + 物理参数 + 配置 |
| **输出** | 解向量 | 完整的仿真结果 |
| **物理感知** | ❌ 无 | ✅ 有（MoM/PEEC/MTL特定） |
| **可复用性** | ✅ 高度可复用 | ⚠️ 特定方法专用 |
| **依赖关系** | 被L5层使用 | 使用L4层 |

### 为什么这么复杂？

1. **架构分层需要**：
   - 通用算法（L4）和业务逻辑（L5）必须分离
   - 这是软件工程的最佳实践

2. **历史遗留**：
   - `unified_interface.c` 的位置错误（应该在L5，却在L4）
   - 目录命名不够清晰

3. **改进方向**：
   - ✅ 修复架构违规（移动 `unified_interface.c`）
   - ⚠️ 可选：重命名或合并目录以提高清晰度

### 最终建议

1. ✅ **已修复**：已删除 `src/backend/solvers/unified_interface.c/h`（架构违规）
2. ✅ **已修复**：已修复 `src/backend/algorithms/adaptive/adaptive_calculation.h`（使用前向声明）
3. **保持分离**：`backend/solvers` 和 `solvers` 的职责分离是正确的，应该保持
4. **文档说明**：在 README 或架构文档中明确说明两个目录的区别

## 修复状态

✅ **架构违规已修复**（详见 `docs/ARCHITECTURE_VIOLATION_FIX.md`）

- ✅ 删除 `src/backend/solvers/unified_interface.c/h`
- ✅ 修复 `src/backend/algorithms/adaptive/adaptive_calculation.h`（使用前向声明）
