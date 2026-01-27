# 目录重复分析报告

## 执行时间
2025-01-XX

## 发现的重复问题

### 1. solvers/hybrid vs orchestration/hybrid_solver ⚠️ 重复

**问题**:
- `src/solvers/hybrid/hybrid_solver.h/c` - 旧的混合求解器接口（包含物理耦合定义）
- `src/orchestration/hybrid_solver/hybrid_solver.h/c` - 新的L5编排层实现（符合架构）

**分析**:
- `solvers/hybrid/` 中的代码包含物理耦合定义，违反架构（应该是L5编排层）
- `orchestration/hybrid_solver/` 是正确的L5层实现
- 应该删除 `solvers/hybrid/`，只保留 `orchestration/hybrid_solver/`

### 2. solvers/mtl/mtl_hybrid_coupling vs solvers/hybrid/mtl_coupling ⚠️ 重复

**问题**:
- `src/solvers/mtl/mtl_hybrid_coupling.c/h` - MTL耦合实现
- `src/solvers/hybrid/mtl_coupling/mtl_hybrid_coupling.c/h` - 相同的MTL耦合实现

**分析**:
- 两个文件内容几乎相同，只是include路径不同
- 根据架构，MTL耦合应该属于L5编排层
- 应该统一到 `orchestration/hybrid_solver/mtl_coupling/`

### 3. backend/algorithms vs backend/fast_algorithms ⚠️ 可能合并

**问题**:
- `src/backend/algorithms/` - 包含结构算法、自适应算法、快速多极算法
- `src/backend/fast_algorithms/` - 包含ACA、H-matrix压缩

**分析**:
- 根据架构，这些都是L4层的快速算法
- `fast_algorithms` 是快速算法的具体实现
- `algorithms` 包含算法选择和结构算法
- 建议合并到 `backend/algorithms/`，创建子目录区分

### 4. modeling vs physics ⚠️ 职责不清

**问题**:
- `src/modeling/pcb/pcb_electromagnetic_modeling.c/h` - PCB建模
- `src/physics/` - 物理定义（L1层）

**分析**:
- `modeling/pcb/` 包含网格生成、求解器配置等，更像是L6层的工作流
- 需要确认是否应该移动到 `orchestration/workflow/pcb/` 或 `io/`

## 合并方案

### 方案1: 删除solvers/hybrid目录 ✅

**操作**:
1. 删除 `src/solvers/hybrid/` 目录
2. 更新所有引用 `solvers/hybrid/` 的代码，改为 `orchestration/hybrid_solver/`

### 方案2: 统一MTL耦合 ✅

**操作**:
1. 移动 `src/solvers/mtl/mtl_hybrid_coupling.c/h` → `src/orchestration/hybrid_solver/mtl_coupling/`
2. 删除 `src/solvers/hybrid/mtl_coupling/`（如果存在）
3. 更新所有引用

### 方案3: 合并fast_algorithms到algorithms ✅

**操作**:
1. 移动 `src/backend/fast_algorithms/*` → `src/backend/algorithms/fast/`
2. 删除 `src/backend/fast_algorithms/` 目录
3. 更新所有引用

### 方案4: 处理modeling目录 ⏳

**操作**:
1. 分析 `modeling/pcb/` 的职责
2. 如果属于L6层，移动到 `orchestration/workflow/pcb/` 或 `io/`
3. 如果属于L1层，移动到 `physics/`

## 架构符合性

### 改进前
- ⚠️ solvers/hybrid 包含物理耦合定义（违反架构）
- ⚠️ MTL耦合代码重复
- ⚠️ 快速算法分散在两个目录
- ⚠️ modeling职责不清

### 改进后
- ✅ 所有混合求解器代码在 orchestration/hybrid_solver/（L5层）
- ✅ MTL耦合统一到 orchestration/hybrid_solver/mtl_coupling/
- ✅ 所有快速算法在 backend/algorithms/（L4层）
- ✅ modeling职责明确
