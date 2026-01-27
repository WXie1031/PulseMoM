# Hybrid Solver 统一化总结

## 已完成的工作

### 1. 扩展 hybrid_solver.h 支持 MTL

#### 1.1 添加 MTL 求解器支持
```c
struct HybridSolver {
    // ...
    MomSolver* mom_solver;
    PeecSolver* peec_solver;
    mtl_solver_t* mtl_solver;  // ✅ 新增
    
    // 求解器启用标志
    bool mom_enabled;
    bool peec_enabled;
    bool mtl_enabled;
    
    // 耦合矩阵（支持任意组合）
    HybridCouplingMatrix* mom_peec_matrix;   // MoM ↔ PEEC
    HybridCouplingMatrix* mom_mtl_matrix;     // MoM ↔ MTL
    HybridCouplingMatrix* peec_mtl_matrix;   // PEEC ↔ MTL
}
```

#### 1.2 扩展耦合方法枚举
```c
typedef enum {
    COUPLING_METHOD_SCHUR_COMPLEMENT,
    COUPLING_METHOD_DOMAIN_DECOMPOSITION,
    COUPLING_METHOD_ITERATIVE_SUBDOMAIN,
    COUPLING_METHOD_LAGRANGE_MULTIPLIERS,
    COUPLING_METHOD_PENALTY_METHOD,
    COUPLING_METHOD_MORTAR_METHOD,
    // ✅ MTL 特定方法
    COUPLING_METHOD_MTL_FIELD_COUPLING,      // MTL-MoM 场耦合
    COUPLING_METHOD_MTL_CIRCUIT_COUPLING,    // MTL-PEEC 电路耦合
    COUPLING_METHOD_MTL_FULL_HYBRID          // MTL 三路耦合
} HybridCouplingMethod;
```

#### 1.3 扩展耦合矩阵结构
```c
typedef struct {
    // ✅ 支持所有两两组合
    Complex* mom_to_peec_matrix;
    Complex* peec_to_mom_matrix;
    Complex* mom_to_mtl_matrix;     // 新增
    Complex* mtl_to_mom_matrix;     // 新增
    Complex* peec_to_mtl_matrix;    // 新增
    Complex* mtl_to_peec_matrix;    // 新增
    // ...
    int mtl_size;  // 新增
} HybridCouplingMatrix;
```

#### 1.4 新增 API 函数
```c
// ✅ 设置 MTL 求解器
StatusCode hybrid_solver_set_mtl_solver(HybridSolver* solver, mtl_solver_t* mtl_solver);

// ✅ 统一求解方法（自动检测组合）
StatusCode hybrid_solver_solve(HybridSolver* solver);

// ✅ 三路耦合
StatusCode hybrid_solver_solve_three_way(HybridSolver* solver);

// ✅ 组合检测
StatusCode hybrid_solver_detect_combination(const HybridSolver* solver, 
                                            HybridCouplingCombination* combination);
bool hybrid_solver_has_mom(const HybridSolver* solver);
bool hybrid_solver_has_peec(const HybridSolver* solver);
bool hybrid_solver_has_mtl(const HybridSolver* solver);
```

#### 1.5 添加组合枚举
```c
typedef enum {
    COUPLING_COMBINATION_MOM_PEEC,      // MoM + PEEC
    COUPLING_COMBINATION_MOM_MTL,       // MoM + MTL
    COUPLING_COMBINATION_PEEC_MTL,      // PEEC + MTL
    COUPLING_COMBINATION_MOM_PEEC_MTL   // MoM + PEEC + MTL (三路)
} HybridCouplingCombination;
```

## 2. 架构对比分析

### 2.1 hybrid_solver.h vs mtl_coupling

| 特性 | hybrid_solver.h (扩展后) | mtl_coupling |
|------|-------------------------|--------------|
| **支持的求解器** | ✅ PEEC, MoM, MTL | ✅ MTL, MoM, PEEC |
| **设计中心** | ✅ 通用框架 | MTL 专用 |
| **耦合方法** | ✅ 9 种（包含 MTL 特定） | 5 种 |
| **实现状态** | 接口定义 | ✅ 有完整实现 |
| **三路耦合** | ✅ 支持 | ✅ 支持 |
| **任意组合** | ✅ 支持 | ✅ 支持 |
| **统一接口** | ✅ 是 | ❌ 否 |

### 2.2 主要区别

**hybrid_solver.h（扩展后）：**
- ✅ 统一的接口设计
- ✅ 支持任意组合（PEEC、MoM、MTL）
- ✅ 通用的耦合框架
- ✅ 完整的接口管理、错误估计
- ⚠️ 需要实现代码

**mtl_coupling：**
- ✅ 有完整实现（约 900 行）
- ✅ MTL 特定的耦合逻辑
- ⚠️ 独立模块，与主框架不统一
- ⚠️ 以 MTL 为中心的设计

## 3. 合并建议

### 方案：完全统一（推荐）

**优点：**
1. ✅ 单一接口，易于使用
2. ✅ 支持任意组合
3. ✅ 代码复用率高
4. ✅ 维护成本低
5. ✅ 统一的错误处理和性能监控

**实施步骤：**

#### 阶段 1: 扩展接口（✅ 已完成）
- ✅ 扩展 `hybrid_solver.h` 支持 MTL
- ✅ 添加 MTL 相关的耦合矩阵
- ✅ 扩展耦合方法枚举

#### 阶段 2: 整合实现（待实施）
- [ ] 创建 `hybrid_solver.c` 实现文件
- [ ] 迁移 `mtl_coupling` 的核心功能
- [ ] 实现统一的耦合逻辑
- [ ] 统一接口风格

#### 阶段 3: 实现耦合算法（待实施）
- [ ] 实现两两组合的耦合矩阵组装
  - [ ] MoM ↔ PEEC
  - [ ] MoM ↔ MTL
  - [ ] PEEC ↔ MTL
- [ ] 实现三路耦合迭代算法
- [ ] 统一收敛检查逻辑

#### 阶段 4: 兼容层（可选）
- [ ] 保留 `mtl_coupling` 作为兼容层
- [ ] 内部调用 `hybrid_solver` 接口
- [ ] 逐步迁移用户代码

## 4. 支持的组合

### 4.1 两两组合
- ✅ **MoM ↔ PEEC**: 场-电路耦合
- ✅ **MoM ↔ MTL**: 场-传输线耦合
- ✅ **PEEC ↔ MTL**: 电路-传输线耦合

### 4.2 三路组合
- ✅ **MoM ↔ PEEC ↔ MTL**: 完整混合耦合

## 5. 使用示例

### 5.1 两路耦合（MoM + PEEC）
```c
HybridSolver* solver = hybrid_solver_create(&options);
hybrid_solver_set_mom_solver(solver, mom_solver);
hybrid_solver_set_peec_solver(solver, peec_solver);
hybrid_solver_initialize(solver);
hybrid_solver_solve(solver);  // 自动检测为 MoM-PEEC 组合
```

### 5.2 两路耦合（MoM + MTL）
```c
HybridSolver* solver = hybrid_solver_create(&options);
hybrid_solver_set_mom_solver(solver, mom_solver);
hybrid_solver_set_mtl_solver(solver, mtl_solver);
hybrid_solver_initialize(solver);
hybrid_solver_solve(solver);  // 自动检测为 MoM-MTL 组合
```

### 5.3 三路耦合（MoM + PEEC + MTL）
```c
HybridSolver* solver = hybrid_solver_create(&options);
hybrid_solver_set_mom_solver(solver, mom_solver);
hybrid_solver_set_peec_solver(solver, peec_solver);
hybrid_solver_set_mtl_solver(solver, mtl_solver);
hybrid_solver_initialize(solver);
hybrid_solver_solve_three_way(solver);  // 三路耦合
```

## 6. 下一步工作

1. **实现 hybrid_solver.c**
   - 迁移 `mtl_coupling` 的实现
   - 实现统一的耦合逻辑
   - 支持任意组合

2. **测试验证**
   - 测试所有两两组合
   - 测试三路耦合
   - 性能对比

3. **文档更新**
   - API 文档
   - 使用示例
   - 迁移指南

## 7. 总结

✅ **已完成：**
- 扩展 `hybrid_solver.h` 支持 MTL
- 添加任意组合支持
- 统一接口设计

📋 **待完成：**
- 实现统一的耦合逻辑
- 迁移 `mtl_coupling` 功能
- 测试验证

🎯 **目标：**
- 支持 PEEC、MoM、MTL 的任意组合
- 统一的接口和实现
- 易于使用和维护
