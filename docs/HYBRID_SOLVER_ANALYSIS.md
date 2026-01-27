# Hybrid Solver 架构分析与合并方案

## 1. 当前架构分析

### 1.1 `hybrid_solver.h` (PEEC-MoM 耦合框架)
**特点：**
- 只支持 **PEEC** 和 **MoM** 两种求解器的耦合
- 通用框架设计，支持多种耦合方法：
  - Schur complement
  - Domain decomposition
  - Iterative subdomain
  - Lagrange multipliers
  - Penalty method
  - Mortar method
- 完整的接口管理、错误估计、自适应功能
- 使用 `MomSolver*` 和 `PeecSolver*` 作为组件求解器

**结构：**
```c
struct HybridSolver {
    MomSolver* mom_solver;      // 只有 MoM
    PeecSolver* peec_solver;    // 只有 PEEC
    // 没有 MTL 支持
}
```

### 1.2 `mtl_hybrid_coupling` (MTL 专用耦合)
**特点：**
- 专门为 **MTL** 设计，以 MTL 为中心
- 支持 MTL-MoM 和 MTL-PEEC 耦合
- 支持三路耦合（MTL-MoM-PEEC）
- 有具体实现代码（约 900 行）
- 使用 `mtl_solver_t*` 作为主求解器

**结构：**
```c
// MTL 为中心的耦合
mtl_coupling_initialize(mtl_solver_t*, void* external_solver, ...)
// 支持：
// - MTL_COUPLING_MOM_FIELD
// - MTL_COUPLING_PEEC_CIRCUIT
// - MTL_COUPLING_FULL_HYBRID
```

## 2. 主要区别

| 特性 | hybrid_solver.h | mtl_coupling |
|------|----------------|--------------|
| **支持的求解器** | PEEC, MoM | MTL, MoM, PEEC |
| **设计中心** | 通用框架 | MTL 专用 |
| **耦合方法** | 6 种通用方法 | 5 种（部分重叠） |
| **实现状态** | 接口定义 | 有完整实现 |
| **三路耦合** | ❌ 不支持 | ✅ 支持 |
| **任意组合** | ❌ 仅 PEEC-MoM | ✅ MTL-任意 |

## 3. 合并方案

### 3.1 设计目标
支持 **PEEC、MoM、MTL 的任意组合**：
- PEEC ↔ MoM
- PEEC ↔ MTL
- MoM ↔ MTL
- PEEC ↔ MoM ↔ MTL (三路)

### 3.2 统一架构设计

#### 3.2.1 扩展 HybridSolver 结构
```c
typedef enum {
    SOLVER_TYPE_MOM,
    SOLVER_TYPE_PEEC,
    SOLVER_TYPE_MTL
} SolverType;

struct HybridSolver {
    // 配置
    HybridCouplingOptions options;
    
    // 组件求解器（支持任意组合）
    struct {
        bool enabled;
        SolverType type;
        void* solver;  // 通用指针
    } solvers[3];  // 最多3个求解器
    
    // 或者使用具体指针（更类型安全）
    MomSolver* mom_solver;
    PeecSolver* peec_solver;
    mtl_solver_t* mtl_solver;
    
    // 耦合矩阵（支持任意两两组合）
    struct {
        Complex* mom_peec_matrix;   // MoM ↔ PEEC
        Complex* mom_mtl_matrix;     // MoM ↔ MTL
        Complex* peec_mtl_matrix;   // PEEC ↔ MTL
    } coupling_matrices;
    
    // 接口数据
    HybridInterface* interface;
    HybridIterationData* iteration_data;
    // ...
};
```

#### 3.2.2 统一耦合接口
```c
// 设置求解器（支持任意组合）
StatusCode hybrid_solver_set_solver(HybridSolver* solver, 
                                    SolverType type, 
                                    void* solver_handle);

// 初始化耦合（自动检测组合）
StatusCode hybrid_solver_initialize(HybridSolver* solver);

// 求解（支持任意组合）
StatusCode hybrid_solver_solve(HybridSolver* solver);
```

### 3.3 实施步骤

#### 步骤 1: 扩展 hybrid_solver.h
- 添加 `mtl_solver_t*` 支持
- 添加 MTL 相关的耦合矩阵
- 扩展耦合方法枚举以包含 MTL 特定方法

#### 步骤 2: 整合 mtl_coupling 功能
- 将 `mtl_coupling` 的实现整合到统一的 hybrid solver 框架
- 保留 MTL 特定的耦合逻辑
- 统一接口风格

#### 步骤 3: 实现统一耦合逻辑
- 实现任意两两组合的耦合矩阵组装
- 实现三路耦合的迭代算法
- 统一收敛检查逻辑

## 4. 推荐方案

### 方案 A: 完全统一（推荐）
**优点：**
- 单一接口，易于使用
- 支持任意组合
- 代码复用率高
- 维护成本低

**实施：**
1. 扩展 `hybrid_solver.h` 支持 MTL
2. 将 `mtl_coupling` 代码迁移到 `hybrid_solver.c`
3. 统一接口和数据结构
4. 删除独立的 `mtl_coupling` 模块

### 方案 B: 分层设计
**优点：**
- 保持模块独立性
- 渐进式迁移

**实施：**
1. 保留 `mtl_coupling` 作为底层实现
2. `hybrid_solver` 调用 `mtl_coupling` 的接口
3. 逐步统一接口

## 5. 具体修改建议

### 5.1 hybrid_solver.h 修改
```c
// 添加 MTL 支持
struct HybridSolver {
    // ...
    MomSolver* mom_solver;
    PeecSolver* peec_solver;
    mtl_solver_t* mtl_solver;  // 新增
    
    // 耦合矩阵（支持任意组合）
    HybridCouplingMatrix* mom_peec_matrix;
    HybridCouplingMatrix* mom_mtl_matrix;
    HybridCouplingMatrix* peec_mtl_matrix;
    // ...
};

// 新增函数
StatusCode hybrid_solver_set_mtl_solver(HybridSolver* solver, mtl_solver_t* mtl_solver);
StatusCode hybrid_solver_solve_three_way(HybridSolver* solver);  // 三路耦合
```

### 5.2 耦合方法扩展
```c
typedef enum {
    COUPLING_METHOD_SCHUR_COMPLEMENT,
    COUPLING_METHOD_DOMAIN_DECOMPOSITION,
    COUPLING_METHOD_ITERATIVE_SUBDOMAIN,
    COUPLING_METHOD_LAGRANGE_MULTIPLIERS,
    COUPLING_METHOD_PENALTY_METHOD,
    COUPLING_METHOD_MORTAR_METHOD,
    // MTL 特定方法
    COUPLING_METHOD_MTL_FIELD_COUPLING,      // MTL-MoM 场耦合
    COUPLING_METHOD_MTL_CIRCUIT_COUPLING,    // MTL-PEEC 电路耦合
    COUPLING_METHOD_MTL_FULL_HYBRID         // MTL 三路耦合
} HybridCouplingMethod;
```

## 6. 总结

**当前问题：**
- `hybrid_solver.h` 不支持 MTL
- `mtl_coupling` 是独立模块，与主框架不统一
- 无法支持任意组合

**解决方案：**
- 扩展 `hybrid_solver.h` 以支持 MTL
- 整合 `mtl_coupling` 到统一框架
- 实现任意组合的耦合支持

**推荐：方案 A（完全统一）**
- 单一接口，易于使用和维护
- 支持 PEEC、MoM、MTL 的任意组合
- 代码结构更清晰
