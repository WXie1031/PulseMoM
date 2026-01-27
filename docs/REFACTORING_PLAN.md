# PulseMoM 代码重构计划

## 重构目标

根据 `ARCHITECTURE_GUARD.md` 和 skills 文档，将现有代码重构为严格的六层架构模型：

```
L1  Physics Definition        物理定义
L2  Discretization            离散与建模
L3  Operator / Update Eq.     算子 / 更新方程
L4  Numerical Backend         数值后端
L5  Execution Orchestration   执行与耦合编排
L6  IO / Workflow / API       输入输出与流程
```

## 当前问题分析

### 1. 架构违规问题

#### 问题1: 跨层依赖
- `solvers/mom/` 和 `solvers/peec/` 直接包含 `core/core_solver.h` (L4层)
- 物理层直接调用数值后端

#### 问题2: GPU代码位置错误
- `core/gpu_acceleration.c` 包含物理计算逻辑
- GPU kernel 中混合了物理公式和数值实现

#### 问题3: 混合求解器违规
- `solvers/hybrid/` 可能包含物理或数值代码
- 应该只包含编排逻辑

#### 问题4: 目录结构混乱
- `core/` 目录包含所有层的代码
- 没有清晰的层间边界

### 2. 代码重复问题
- 网格代码分散在多个位置
- 求解器代码有多个版本 (unified, min, module)

## 新目录结构设计

```
src/
├── L1_physics/                    # L1: 物理定义层
│   ├── mom/                      # MoM物理模型
│   │   ├── mom_physics.h/c       # MoM物理方程定义
│   │   └── mom_boundary_conditions.h/c
│   ├── peec/                     # PEEC物理模型
│   │   ├── peec_physics.h/c      # PEEC物理方程定义
│   │   └── peec_circuit_model.h/c
│   ├── mtl/                      # MTL物理模型
│   │   └── mtl_physics.h/c
│   └── hybrid/                   # 混合物理模型
│       └── hybrid_physics_boundary.h/c
│
├── L2_discretization/             # L2: 离散层
│   ├── geometry/                 # 几何处理
│   │   ├── geometry_engine.h/c
│   │   └── cad_import.h/c
│   ├── mesh/                     # 网格生成
│   │   ├── mesh_engine.h/c
│   │   ├── surface_mesh.h/c      # 表面网格 (MoM)
│   │   ├── wire_mesh.h/c         # 线网格 (PEEC)
│   │   └── volume_mesh.h/c       # 体网格 (FDTD)
│   └── basis/                    # 基函数
│       ├── rwg_basis.h/c
│       ├── rooftop_basis.h/c
│       └── higher_order_basis.h/c
│
├── L3_operators/                  # L3: 算子层
│   ├── kernels/                  # 积分核
│   │   ├── greens_function.h/c
│   │   ├── layered_greens_function.h/c
│   │   ├── mom_kernel.h/c        # MoM积分核
│   │   └── peec_kernel.h/c       # PEEC积分核
│   ├── integration/              # 积分计算
│   │   ├── integration_utils.h/c
│   │   ├── singular_integration.h/c
│   │   └── quadrature.h/c
│   ├── assembler/                # 矩阵组装
│   │   ├── matrix_assembler.h/c
│   │   └── excitation_assembler.h/c
│   ├── matvec/                   # 矩阵向量积
│   │   └── matvec_operator.h/c
│   └── coupling/                 # 耦合算子
│       └── coupling_operator.h/c
│
├── L4_backend/                    # L4: 数值后端
│   ├── solvers/                  # 求解器
│   │   ├── direct_solver.h/c     # 直接求解器
│   │   ├── iterative_solver.h/c  # 迭代求解器
│   │   └── solver_interface.h    # 统一求解器接口
│   ├── fast_algorithms/          # 快速算法
│   │   ├── fmm.h/c               # 快速多极子
│   │   ├── mlfmm.h/c             # 多层快速多极子
│   │   ├── aca.h/c               # 自适应交叉近似
│   │   └── hmatrix.h/c           # H-矩阵
│   ├── gpu/                      # GPU加速
│   │   ├── gpu_kernels.cu        # GPU kernel
│   │   ├── gpu_linear_algebra.h/c
│   │   └── gpu_memory.h/c
│   ├── memory/                   # 内存管理
│   │   ├── memory_pool.h/c
│   │   └── memory_optimization.h/c
│   └── math/                     # 数学库接口
│       ├── blas_interface.h/c
│       └── lapack_interface.h/c
│
├── L5_orchestration/             # L5: 执行编排层
│   ├── hybrid_solver/            # 混合求解器
│   │   ├── hybrid_solver.h/c
│   │   ├── coupling_manager.h/c
│   │   └── execution_graph.h/c
│   ├── workflow/                 # 工作流
│   │   ├── workflow_engine.h/c
│   │   └── pcb_workflow.h/c
│   └── execution/               # 执行管理
│       ├── execution_order.h/c
│       └── data_flow.h/c
│
├── L6_io/                        # L6: IO/Workflow/API层
│   ├── file_formats/             # 文件格式
│   │   ├── cad_io.h/c
│   │   ├── touchstone_io.h/c
│   │   ├── hdf5_io.h/c
│   │   └── vtk_io.h/c
│   ├── api/                      # API接口
│   │   ├── c_api.h/c
│   │   └── python_api.h
│   ├── cli/                      # 命令行接口
│   │   └── cli_main.c
│   └── gui/                      # 图形界面
│       └── ui_system.c
│
└── common/                       # 公共定义 (所有层共享)
    ├── types.h                   # 基本类型定义
    ├── constants.h               # 物理常数
    └── errors.h                  # 错误码定义
```

## 重构步骤

### 阶段1: 分析和准备
1. ✅ 分析当前代码架构问题
2. ✅ 设计新目录结构
3. 创建迁移映射表

### 阶段2: 创建新目录结构
1. 创建 L1-L6 目录
2. 创建 common/ 目录
3. 更新 .gitignore

### 阶段3: 迁移 L1 物理层
1. 提取 MoM 物理定义
2. 提取 PEEC 物理定义
3. 提取 MTL 物理定义
4. 提取混合物理边界条件

### 阶段4: 迁移 L2 离散层
1. 整合几何处理代码
2. 整合网格生成代码
3. 整合基函数代码

### 阶段5: 迁移 L3 算子层
1. 提取积分核代码
2. 提取积分计算代码
3. 提取矩阵组装代码
4. 提取耦合算子代码

### 阶段6: 迁移 L4 数值后端
1. 提取求解器代码
2. 提取快速算法代码
3. 提取GPU代码 (分离物理和数值)
4. 提取内存管理代码

### 阶段7: 迁移 L5 编排层
1. 重构混合求解器 (移除物理/数值代码)
2. 重构工作流代码
3. 创建执行管理代码

### 阶段8: 迁移 L6 IO层
1. 整合文件格式代码
2. 整合API代码
3. 整合CLI代码

### 阶段9: 更新接口和依赖
1. 创建层间接口定义
2. 更新所有 include 路径
3. 修复编译错误

### 阶段10: 更新构建系统
1. 更新 CMakeLists.txt
2. 更新依赖关系
3. 验证构建

### 阶段11: 测试和验证
1. 运行单元测试
2. 运行集成测试
3. 验证架构合规性

## 层间接口规范

### L1 → L2 接口
- 物理模型需要几何/网格信息时，通过接口获取
- 禁止直接访问网格数据结构

### L2 → L3 接口
- 离散化结果传递给算子层
- 禁止算子层修改网格

### L3 → L4 接口
- 算子计算结果传递给数值后端
- 禁止数值后端修改算子定义

### L4 → L5 接口
- 数值后端提供统一求解接口
- 禁止编排层直接调用内部实现

### L5 → L6 接口
- 编排层提供执行结果
- 禁止IO层修改执行逻辑

## 禁止模式

1. ❌ L1 直接调用 L4 (物理层调用求解器)
2. ❌ L3 直接调用 L4 (算子层直接调用GPU)
3. ❌ L5 包含物理或数值代码
4. ❌ 跨层直接访问数据结构
5. ❌ 在GPU kernel中写物理公式
6. ❌ 在求解器中写物理方程

## 验证清单

每个文件迁移后需要验证：
- [ ] 文件属于且仅属于一层
- [ ] 没有跨层依赖
- [ ] 没有违反禁止模式
- [ ] 接口清晰明确
- [ ] 编译通过
- [ ] 测试通过
