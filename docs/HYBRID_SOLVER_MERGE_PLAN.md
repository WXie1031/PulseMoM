# Hybrid Solver 合并实施计划

## 实施步骤

### 阶段 1: 扩展 hybrid_solver.h（已完成）
✅ 添加 MTL 求解器支持
✅ 添加 MTL 相关的耦合矩阵
✅ 扩展耦合方法枚举
✅ 添加三路耦合支持

### 阶段 2: 整合 mtl_coupling 功能
- [ ] 创建统一的耦合实现文件 `hybrid_solver.c`
- [ ] 迁移 `mtl_coupling` 的核心功能
- [ ] 统一接口风格
- [ ] 实现任意组合的耦合逻辑

### 阶段 3: 实现统一耦合算法
- [ ] 实现两两组合的耦合矩阵组装
  - [ ] MoM ↔ PEEC
  - [ ] MoM ↔ MTL
  - [ ] PEEC ↔ MTL
- [ ] 实现三路耦合迭代算法
- [ ] 统一收敛检查逻辑

### 阶段 4: 测试和验证
- [ ] 测试所有两两组合
- [ ] 测试三路耦合
- [ ] 性能对比测试
- [ ] 文档更新

## 代码迁移映射

### mtl_coupling 函数 → hybrid_solver 函数

| mtl_coupling | hybrid_solver |
|--------------|---------------|
| `mtl_coupling_initialize` | `hybrid_solver_initialize` (扩展) |
| `mtl_coupling_iterate` | `hybrid_solver_iterate_coupling` (扩展) |
| `mtl_coupling_compute_field_interaction` | `hybrid_solver_compute_mom_mtl_coupling` |
| `mtl_coupling_compute_circuit_interaction` | `hybrid_solver_compute_peec_mtl_coupling` |
| `mtl_coupling_update_boundary_conditions` | `hybrid_solver_update_interface` (扩展) |
| `mtl_coupling_check_convergence` | `hybrid_solver_check_convergence` (扩展) |

## 数据结构映射

### mtl_coupling_state_t → HybridIterationData
```c
// mtl_coupling_state_t
CDOUBLE** coupling_matrix_mom;
CDOUBLE** coupling_matrix_peec;
CDOUBLE* boundary_conditions;

// → HybridIterationData (扩展)
Complex* mom_solution;
Complex* peec_solution;
Complex* mtl_solution;  // 新增
Complex* interface_solution;
```

## 接口统一

### 统一设置求解器
```c
// 旧接口（mtl_coupling）
int mtl_coupling_initialize(mtl_solver_t* mtl_solver, 
                           void* external_solver, 
                           mtl_coupling_mode_t mode);

// 新接口（hybrid_solver）
StatusCode hybrid_solver_set_mom_solver(HybridSolver* solver, MomSolver* mom_solver);
StatusCode hybrid_solver_set_peec_solver(HybridSolver* solver, PeecSolver* peec_solver);
StatusCode hybrid_solver_set_mtl_solver(HybridSolver* solver, mtl_solver_t* mtl_solver);
StatusCode hybrid_solver_initialize(HybridSolver* solver);  // 自动检测组合
```

## 向后兼容性

### 保留 mtl_coupling 作为兼容层
```c
// mtl_hybrid_coupling.h (兼容层)
int mtl_coupling_initialize(mtl_solver_t* mtl_solver, 
                           void* external_solver, 
                           mtl_coupling_mode_t mode) {
    // 创建 HybridSolver
    HybridSolver* hybrid = hybrid_solver_create(...);
    hybrid_solver_set_mtl_solver(hybrid, mtl_solver);
    
    // 根据 mode 设置其他求解器
    if (mode == MTL_COUPLING_MOM_FIELD) {
        hybrid_solver_set_mom_solver(hybrid, (MomSolver*)external_solver);
    } else if (mode == MTL_COUPLING_PEEC_CIRCUIT) {
        hybrid_solver_set_peec_solver(hybrid, (PeecSolver*)external_solver);
    }
    
    return hybrid_solver_initialize(hybrid);
}
```

## 文件组织

```
src/solvers/hybrid/
├── hybrid_solver.h          # 统一接口（已扩展）
├── hybrid_solver.c          # 统一实现（待创建）
├── mtl_coupling/            # 保留作为兼容层
│   ├── mtl_hybrid_coupling.h
│   └── mtl_hybrid_coupling.c  # 调用 hybrid_solver
└── coupling/                 # 底层耦合实现
    ├── mom_peec_coupling.c
    ├── mom_mtl_coupling.c
    └── peec_mtl_coupling.c
```
