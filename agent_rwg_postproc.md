## 3. 表面电流评估与 VTK 导出（基于 RWG 展开）

### 3.1 目标

- 不再将“未知量 = 单元常数电流”直接写入 VTK，而是：
  - 将 **RWG 边系数 \(I_n\)** 作为求解结果；
  - 在每个三角面上按基函数定义计算 \( \mathbf{J}(\mathbf{r}) = \sum_n I_n \mathbf{f}_n(\mathbf{r}) \)，
  - 在若干采样点（顶点 / 三角质心 / 自定义采样）上评估电流，并写入 VTK 的 **POINT_DATA / CELL_DATA**。

### 3.2 需要关注的核心文件 / 函数

- `src/solvers/mom/mom_solver_unified.c`
  - RWG 函数相关工具（边长度、 plus/minus 三角、三角几何）。
- `src/solvers/mom/mom_solver_min.c`
  - 当前的 `mom_solver_export_surface_current_vtk`（基于单元常数）。
- `src/io/file_formats/export_vtk.c`
  - `export_vtk_current(...)`（VTK 头与 POINT_DATA / CELL_DATA 写法）。

### 3.3 设计思路（后处理评估）

1. **在求解完成后，记录 RWG 系数与边-三角拓扑**
   - `mom_solver_solve` 结束后，`solver->result.current_coefficients[i]` 将代表第 i 条 RWG 边的电流系数 \(I_i\)。
   - 需要一份 `edge_plus[i], edge_minus[i]` 映射，供后处理使用。

2. **定义在三角形上的评估策略**
   - 对于每个三角形 \(T\)：
     - 找出所有“支撑在该三角上的 RWG 基函数”。
     - 在一个或多个采样点（例如：
       - 三角质心；
       - 三顶点；
       - 或将三角细分为若干子点）上评估：
       \[
         \mathbf{J}(\mathbf{r}_s) = \sum_{n \in \mathcal{N}(T)} I_n \mathbf{f}_n(\mathbf{r}_s)
       \]
   - 对 VTK：
     - 若在顶点采样：写入 **POINT_DATA**（点向量或标量）。
     - 若在质心采样：写入 **CELL_DATA**（与原有结构兼容）。

3. **简化初始版本：仅导出 |J| 的标量场**
   - 先实现标量 \(|\mathbf{J}(\mathbf{r})|\) 的评估和导出：
     - 例如在三角质心采样一次，写入 `current_magnitude`（CELL_DATA）。
     - 同时在三顶点采样，写入 `current_magnitude`（POINT_DATA），用于平滑显示。
   - 后续如有需要，再扩展导出向量场（3 分量）和相位。

### 3.4 具体任务清单

1. 在 unified solver 或公共模块中，整理出一个可复用的 **RWG 基函数评估函数**：
   - 输入：mesh、edge_plus/minus、边索引、采样点 \(\mathbf{r}\)；
   - 输出：基函数向量值 \(\mathbf{f}_n(\mathbf{r})\)。

2. 在 `mom_solver_export_surface_current_vtk` 中：
   - 改为遍历三角形或顶点，调用 RWG 评估函数，将 `solver->result.current_coefficients` 展开成 \(\mathbf{J}(\mathbf{r})\)。
   - 生成：
     - `current_magnitude_vertices[]`（POINT_DATA）；
     - `current_magnitude[]`（CELL_DATA，可选）。

3. 在 VTK 导出层（`export_vtk_current`）保持接口不变，只消费新的字段：
   - 若提供了顶点电流场：优先写 POINT_DATA；
   - 保留 CELL_DATA 作为参考。

