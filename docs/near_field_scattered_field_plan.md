## MoM 近场计算改造规划（含散射场贡献）

目标：基于当前 `mom_solver_min.c` 中的求解结果（表面电流系数），实现**叠加车辆散射贡献的近场计算**，并在 STEP 几何同级目录导出可用于论文实验的 3D 电场强度数据与图像。

---

### 一、现状梳理

- **求解器与结构**
  - `mom_solver_min.c` 定义 `mom_solver_t`，包含：
    - `mesh_t* mesh;` （来自 `core_mesh.h`，包含顶点、三角面等几何信息）
    - `mom_result_t result;`（见 `mom_solver.h`，包含 `current_coefficients`、`near_field`、`far_field` 等）
  - 当前 `mom_solver_set_mesh` 中：`solver->num_unknowns = solver->mesh->num_elements;`（暂按单元数作为未知数，后续按 RWG 统一）

- **基函数与网格**
  - `discretization/basis/rwg_basis.h`：定义 `rwg_basis_t` 和 `rwg_basis_set_t`，与 `mesh_t` 关联（通过三角形、边、顶点坐标）。
  - 装配层 `operators/assembler` 已经用 RWG + Green 函数生成矩阵。

- **当前近场接口**
  - `mom_solver_compute_near_field`（`mom_solver_min.c`）：
    - 仅使用 `solver->excitation`（平面波）生成**简化入射场近似**；
    - 完全未使用 `current_coefficients`、`mesh` 或 `rwg_basis`，**没有散射场**。

- **CLI / 导出**
  - `src/apps/mom_cli/main_mom.c`：负责
    - 创建 `mom_solver_t`，装配、求解；
    - 目前调用 `mom_solver_compute_near_field`，并将 E/H 以及电流系数导出到：
      - `*_results.txt`
      - `current_plot.csv`
      - `near_field_plot.csv`
  - `scripts/plot_results.py`：读取上述 CSV，生成图片（current、near field）。

---

### 二、设计目标与范围

1. **核心目标**
   - 重写 / 扩展 `mom_solver_compute_near_field`，使其在给定观察点集合 `{r_i}` 上计算：
     - 总场：`E_total(r_i) = E_inc(r_i) + E_scat(r_i)`
     - 其中 `E_scat` 基于已求解的 RWG 电流系数 `J_n` 与 Green 函数实现。

2. **近场实现策略**
   - **优先选择“简单但正确”的散射近似**：
     - 使用 RWG 基函数的**等效电流集中点近似**：将每个 RWG 基函数视作集中在其 `support_center` 或 `edge` 上的电流源；
     - 对观察点 `r` 采用点源/小面源的 Green 函数近似实现散射场；
     - 不在第一步就接入完整的逐三角形积分核（保持实现复杂度可控）。
   - 保留现有的入射场近似逻辑，用作 `E_inc`，并在新接口中显式叠加。

3. **结果导出与可视化**
   - 仍使用 CLI `main_mom.c` 驱动：
     - 在 STEP 模型附近建立 2D/3D 采样网格（如：某固定高度的 `x-y` 平面，或若干 `x-y` 截面）；
     - 调用新的近场接口，得到 `E_total`；
     - 将结果导出为：
       - 结构化 CSV：`near_field_scattered.csv`（含 `x,y,z, |E_total|, |E_inc|, |E_scat|` 等）；
       - 更新 / 扩展 Python 绘图脚本：生成 2D 伪彩色 / 3D 曲面图，用于论文插图。

4. **精度与性能取舍（本阶段）**
   - 不追求工业级高精度，而追求**物理上合理可解释**，适合作为论文中“自研 MoM 近场后处理”的演示。
   - 支持中等规模问题（数千基函数、数千场点），可接受 O(N_basis × N_points) 复杂度；如需更大规模，可再引入分块/快速算法。

---

### 三、技术方案（散射场近似）

#### 3.1 数学模型（简化版）

- 已知：
  - RWG 基函数 `f_n`，支撑在若干三角面上；
  - 电流系数 `J_n`（`current_coefficients[n]`）；
  - 观察点集合 `r_i`。

- **简化散射场近似**：
  - 为每个 RWG 基函数定义等效集中点 `r_n`（用 `rwg_basis_t.support_center` 或由正/负三角形质心加权得到）；
  - 将基函数电流视为**小电流面源**，在远离源尺寸的观察点处，用点电流近似：
    \[
      E_\text{scat}(r_i) \approx \sum_n J_n \cdot G_\text{eff}(r_i, r_n, \hat{l}_n)
    \]
  - `G_eff` 为从集中点到观察点的标量或向量 Green 函数近似，可借用 `electromagnetic_kernels` 或在 `mom_solver_min.c` 内实现自由空间标量核（如 `e^{-jkR}/(4πR)`）并乘上合适的方向因子。

#### 3.2 具体实现步骤

1. **获取 RWG 基函数几何信息**
   - 利用 `rwg_basis_set_t` 和 `mesh_t`：
     - 为每个基函数计算或读取：
       - 集中点 `r_n`（`support_center` 或两三角质心加权平均）；
       - 有效方向向量 `u_n`（由 `edge_vector` 或三角面法向组合给出）；  
     - 将这些信息缓存到 `mom_solver_t` 内部（如新增 `rwg_basis_set_t* basis_set;` 或内部数组）。

2. **构建简化散射核**
   - 在 `mom_solver_min.c` 新增内部函数：
     - `static complex_t mom_nearfield_green_simple(const geom_point_t* r_obs, const geom_point_t* r_src, double k);`
     - 计算 `R = |r_obs - r_src|`，返回 `e^{-j k R} / (4π R)` 类型的标量核；
   - 如需简单方向性，可将 `J_n` 投影到观察方向或边方向上，得出标量等效源强度。

3. **重写 `mom_solver_compute_near_field`**
   - 函数签名不变：
     - `int mom_solver_compute_near_field(mom_solver_t* solver, const point3d_t* points, int num_points);`
   - 内部逻辑：
     1. 校验 `solver->mesh`、`solver->result.current_coefficients`、`solver->num_unknowns`；
     2. 初始化 / 重置 `result.near_field.e_field`, `h_field` 数组；
     3. 对每个观察点 `r_i`：
        - 先按当前逻辑计算入射场 `E_inc(r_i)`（保持兼容性）；
        - 计算散射场：
          - 遍历所有基函数 `n`：
            - 取等效源点 `r_n` 与系数 `J_n`；
            - 计算核 `G(r_i, r_n)`；
            - 累加到 `E_scat(r_i)`；
        - 令 `E_total = E_inc + E_scat`，写入 `near_field.e_field[i*3 + xyz]`；
        - `H_field` 先可用简化关系 `H ≈ (1/η0) (k̂ × E)`，保持与现有近似一致。

4. **在 `mom_solver_create` 或装配阶段构建 RWG 数据**
   - 方案一（轻量）：
     - 在 `mom_solver_set_mesh` 中调用 `rwg_basis_create(mesh, &solver->basis_set)`；
     - 在 `mom_solver_free_buffers` 中调用 `rwg_basis_destroy`。
   - 方案二（仅使用几何中心）：
     - 若不想引入完整 RWG 集合，可暂时根据 `mesh->elements` 直接定义“每个三角面 + 电流系数”作为等效面源（但需要保证 `num_unknowns` 对应关系合理）。

> 本阶段建议采用 **方案二**：先按“每个元素一个等效电流面源”近似，实现 `E_scat`，保证简单可用；后续如果需要更高精度，再替换为基于 `rwg_basis_set_t` 的版本。

---

### 四、CLI 与导出改造

1. **激励设置：45° 斜入射平面波**
   - 在 `main_mom.c` 中，在 `mom_solver_create` 之后新增：
     - 设置 `mom_excitation_t`：
       - `type = MOM_EXCITATION_PLANE_WAVE`
       - `frequency = solver_config.frequency`
       - `k_vector = (sin 45°, 0, -cos 45°)`（默认车身法向为 +z，入射朝向 -z，与法向夹角 45°）
       - `polarization = (0, 1, 0)`（E 沿 y）
       - `amplitude = 1.0`
     - 调用 `mom_solver_add_excitation(solver, &exc);`

2. **近场采样：2D 平面/3D 体积网格**
   - 在 `main_mom.c` 内：
     - 根据 `mesh->min_bound`/`max_bound` 或固定范围，定义一个或多个采样平面，例如：
       - 平面 z = z0（车内/车顶高度），网格点 `(x_i, y_j, z0)`；
     - 调用新的 `mom_solver_compute_near_field`，得到每个点的 `E_total`。

3. **导出格式**
   - 保持现有 `*_results.txt`、`current_plot.csv`、`near_field_plot.csv` 不变（向后兼容）。
   - 新增：
     - **`near_field_scattered.csv`**（与 STEP 同级）：
       - 表头：  
         `point_index, x, y, z, Ex_re, Ex_im, Ey_re, Ey_im, Ez_re, Ez_im, |E_inc|, |E_scat|, |E_total|`
       - 行数：所有采样点。
     - 可选：`near_field_scattered_plane_zX.csv`（多个平面时拆文件）。

4. **Python 绘图扩展**
   - 扩展 `scripts/plot_results.py`：
     - 增加读取 `near_field_scattered.csv`：
       - 支持绘制：
         - 2D 伪彩色图：在平面上画 `|E_total|` 分布；
         - 若采样为规则网格，使用 `imshow`/`pcolormesh`；
       - 输出：
         - `near_field_scattered_total_E.png`（|E_total| 分布图）
         - 可选：分别画 |E_inc|、|E_scat| 以对比散射贡献。

---

### 五、实施步骤（编码顺序）

1. **准备与重构**
   1.1 在 `docs/` 目录保存本规划（当前文件）。  
   1.2 在 `mom_solver_min.c` 中为近场相关代码增加清晰注释，标记“旧入射波近似实现”。

2. **实现简化散射场近场计算**
   2.1 在 `mom_solver_min.c` 中新增：
       - 简单 Green 函数：`mom_nearfield_green_simple`。  
   2.2 在 `mom_solver_min.c` 中新增内部帮助函数：
       - `static void mom_get_element_center(const mesh_t* mesh, int elem_id, point3d_t* center);`
   2.3 重写 `mom_solver_compute_near_field`：
       - 使用“每个三角单元 + 电流系数”作为等效面源近似：
         - 源点：`element.centroid`；
         - 源强：`current_coefficients[elem_id]`（与原矩阵未知对应）。
       - 对每个观察点：
         - 先算 `E_inc`（保留原逻辑）；
         - 再累加 `E_scat` 并写入 `near_field.e_field`（存 `E_total`）。
       - 保持 `near_field.h_field` 近似处理（基于总场）。

3. **CLI 激励与采样修改**
   3.1 在 `main_mom.c`：
       - 在 `mom_solver_create` 之后设置 45° 平面波激励（如上 2.1 所述）。  
   3.2 修改近场采样：
       - 从原先的一条线（1D）扩展为单个平面（2D 网格）：
         - 例如 `Nx × Ny` 个点，z 固定为 `z0`，x、y 范围来自 `mesh` 的 bounding box（可用固定范围简化）。  
       - 调用新的 `mom_solver_compute_near_field`。

4. **导出与脚本更新**
   4.1 在 `main_mom.c`：
       - 新增 `near_field_scattered.csv` 导出逻辑：
         - 对所有近场点，将坐标与 `E_inc`、`E_scat`（可由 `E_total - E_inc` 得出）和 `E_total` 的模写入文件。  
   4.2 更新 `scripts/plot_results.py`：
       - 增加一个函数：
         - `plot_near_field_scattered(csv_path)`，生成伪彩色图 `near_field_scattered_total_E.png`。

5. **测试与验证**
   5.1 使用简单几何（如单板/小平面）和已知入射波方向：
       - 检查近场在远离几何时是否接近纯入射波；  
       - 在靠近/内部区域观察 |E_total| 与 |E_inc| 的差异是否符合直觉（散射存在增强/减弱）。  
   5.2 对 `cartingCar` 模型：
       - 运行 CLI，生成 TXT/CSV/PNG；
       - 确认数据文件在 STEP 同级目录，图像可用于论文插图。

---

### 六、后续可选增强（非本轮必做）

- 用 `rwg_basis_set_t` 替代“每单元一个等效源”的粗略近似：
  - 将未知量真正绑定到 RWG 基函数，而非单元；
  - 使用 RWG 支撑区域与边向量改善散射方向性。
- 与装配核统一：
  - 复用 `integration_utils` 或 `electromagnetic_kernels` 中的“面源 → 场点”积分逻辑，提高精度。
- 性能优化：
  - 对近场点较多时，引入分块、快速多极/树分解等以降低复杂度。

