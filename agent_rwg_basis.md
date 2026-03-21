## 1. RWG 基函数设计与接口调整

### 1.1 目标

- 在几何和网格层面，明确 **边（edge）为未知**，每条内部边对应一个 RWG 基函数。
- 在求解器配置与结果结构中，区分“基函数个数（num_basis_functions）”与“单元数（num_elements）”，不再简单等同于三角形数量。

### 1.2 需要关注的核心文件 / 函数

- `src/solvers/mom/mom_solver_unified.c`
  - `mom_unified_state_t` 中的 `num_basis_functions`
  - `build_rwg_mapping(...)` 及相关 RWG 几何工具
- `src/solvers/mom/mom_solver_min.c`
  - `mom_solver_set_mesh`（目前将 `num_unknowns` 设为 `mesh->num_elements`）
  - `mom_solver_allocate_linear_system`
  - `mom_solver_assemble_matrix`
  - `mom_solver_solve`
- `src/discretization/mesh/core_mesh.h`
  - `mesh_edge_t` 结构及 `mesh_t::num_edges`

### 1.3 待完成任务

1. **将 CLI 路径的未知量数量切换为“有效 RWG 边数”**
   - 在 `mom_solver_set_mesh` 中：
     - 调用统一的 `build_rwg_mapping_local` 或新建工具函数，筛选出参与 RWG 的边（两个相邻三角共享的内部边）。
     - 将 `solver->num_unknowns` 设置为“参与 RWG 的边数”，而不是 `mesh->num_elements`。
   - 在 `mom_solver_allocate_linear_system` 中：
     - 依据新的 `num_unknowns` 分配 `excitation_vector` 与 `current_coefficients`。

2. **为 RWG 基函数建立“边索引 → (plus, minus) 三角形”映射并纳入 state**
   - 在 unified solver 中已有 `build_rwg_mapping`，需要：
     - 确认其接口可复用，或为 CLI 路径保存一份轻量映射（例如保存在 `mom_solver_t` 内部或 `mom_result_t` 的扩展字段中）。

3. **在配置层明确 basis_type = MOM_BASIS_RWG 的语义**
   - 在 `main_mom.c` 中已经设置 `solver_config.basis_type = MOM_BASIS_RWG`，需要确保：
     - unified solver 根据此枚举真正选择 RWG 装配路径。

