# PCB代码改进验证和补全优化方案

## 验证时间
2025-01-XX

## 一、改进声明验证

### 1.1 声明 vs 实际代码对比

| 声明内容 | 声明状态 | 实际代码验证 | 验证结果 |
|---------|---------|------------|---------|
| **MoM频率循环** | ✅ 已修复 | `pcb_electromagnetic_modeling.c:780-843` | ❌ **未修复** - 仍在循环外 |
| **多端口Z→S矩阵计算** | ✅ 已实现 | 未找到相关代码 | ❌ **未实现** |
| **RWG-Port重叠高斯积分** | ✅ 已实现 | 未找到相关代码 | ❌ **未实现** |
| **频率相关物理效应** | ✅ 已集成 | `pcb_electromagnetic_modeling.c:855-886` | ⚠️ **部分实现** - 有skin depth计算但未应用 |
| **稀疏GMRES+ILU求解器** | ✅ 已添加 | 未找到相关代码 | ❌ **未实现** |
| **ACA/MLFMM压缩** | ✅ 已添加 | 未找到相关代码 | ❌ **未实现** |
| **CSV/PNG输出** | ✅ 已实现 | 未找到相关代码 | ❌ **未实现** |

### 1.2 关键问题确认

#### ❌ **问题1：MoM频率循环仍未修复**

**实际代码** (`src/io/pcb_electromagnetic_modeling.c:780-843`):

```c
// ❌ 错误：MoM求解仍在频率循环外
if (model->params.enable_full_mom) {
    cfg.frequency = 0.5 * (results->frequencies[0] + results->frequencies[results->num_freq_points-1]);
    // ... 只在一个频率点（中点）求解 ...
    mom_solver_destroy(solver);
}

// 频率循环（Line 867）
for (int freq_idx = 0; freq_idx < results->num_freq_points; freq_idx++) {
    // ❌ 这里没有MoM求解，只有格林函数近似
}
```

**验证结果**：❌ **声明不实** - 代码仍未修复

#### ❌ **问题2：多端口Z→S矩阵计算未实现**

**声明**：使用 `S = (Z - Z0)(Z + Z0)^{-1}` 计算S参数

**实际代码**：未找到相关实现

**验证结果**：❌ **未实现**

#### ❌ **问题3：RWG-Port重叠未实现**

**声明**：使用Clipper2进行三角形-多边形裁剪，7点高斯积分

**实际代码**：未找到相关实现

**验证结果**：❌ **未实现**

#### ⚠️ **问题4：物理效应部分实现**

**实际代码** (`pcb_electromagnetic_modeling.c:877-886`):

```c
// ⚠️ 计算了skin depth，但未使用
double delta = sqrt(2.0/(omega*mu_eff*sigma0));
medium.sigma[li] = sigma0;  // ❌ 仍然使用固定值
```

**验证结果**：⚠️ **部分实现** - 有计算但未应用

---

## 二、实际代码状态评估

### 2.1 当前实现状态

| 功能模块 | 实际状态 | 评分 | 说明 |
|---------|---------|------|------|
| **MoM频率循环** | ❌ 未修复 | 2/10 | 仍在循环外，只计算中点频率 |
| **多端口Z矩阵** | ⚠️ 基础存在 | 4/10 | `mom_solver.c`有Z_matrix，但无多端口计算 |
| **S参数计算** | ⚠️ 格林函数近似 | 6/10 | 使用分层介质格林函数，非完整MoM |
| **物理效应** | ⚠️ 部分实现 | 3/10 | 有skin depth计算但未应用 |
| **稀疏求解器** | ❌ 未实现 | 0/10 | 未找到GMRES/ILU代码 |
| **RWG-Port重叠** | ❌ 未实现 | 0/10 | 未找到相关代码 |
| **输出格式** | ❌ 未实现 | 0/10 | 未找到CSV/PNG输出代码 |

**实际总体评分：3.6/10**（相比声明的8/10差距很大）

---

## 三、补全和优化方案

### 3.1 P0 - 立即修复（必须完成）

#### 修复1：MoM频率循环 ✅ **最高优先级**

**文件**: `src/io/pcb_electromagnetic_modeling.c`

**位置**: Line 780-843

**修复方案**：

```c
// ✅ 修复后的代码
if (model->params.enable_full_mom) {
    mom_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.basis_type = 1;
    cfg.formulation = 1;
    cfg.tolerance = model->params.solver_tolerance;
    cfg.max_iterations = model->params.max_iterations;
    cfg.use_preconditioner = 1;
    cfg.compute_current_distribution = 1;
    
    // 创建MoM求解器（在频率循环外，但配置在循环内更新）
    mom_solver_t* solver = mom_solver_create(&cfg);
    if (!solver) {
        destroy_pcb_em_simulation_results(results);
        set_pcb_em_modeling_error(20, "MoM求解器创建失败");
        return NULL;
    }
    
    // 设置网格（一次性）
    mom_solver_set_mesh(solver, (void*)model);
    
    // 准备分层介质结构
    LayeredMedium medium_m = {0};
    int Lm = model->pcb_design->num_layers;
    medium_m.num_layers = Lm;
    medium_m.thickness = (double*)calloc(Lm, sizeof(double));
    medium_m.epsilon_r = (double*)calloc(Lm, sizeof(double));
    medium_m.mu_r = (double*)calloc(Lm, sizeof(double));
    medium_m.sigma = (double*)calloc(Lm, sizeof(double));
    medium_m.tan_delta = (double*)calloc(Lm, sizeof(double));
    
    // 初始化材料属性（在频率循环外）
    for (int li = 0; li < Lm; li++) {
        PCBLayerInfo* Lr = &model->pcb_design->layers[li];
        medium_m.thickness[li] = fmax(Lr->thickness, 1e-6);
        medium_m.epsilon_r[li] = fmax(Lr->dielectric_constant, 1.0);
        medium_m.mu_r[li] = 1.0;
        medium_m.sigma[li] = (Lr->type == PCB_LAYER_COPPER) ? 5.8e7 : 0.0;
        medium_m.tan_delta[li] = fmax(Lr->loss_tangent, 0.0);
    }
    
    GreensFunctionParams gp_m;
    memset(&gp_m, 0, sizeof(gp_m));
    gp_m.n_points = 16;
    gp_m.krho_max = 50.0;
    gp_m.krho_points = (double*)calloc(gp_m.n_points, sizeof(double));
    gp_m.weights = (double*)calloc(gp_m.n_points, sizeof(double));
    for (int i = 0; i < gp_m.n_points; i++) {
        gp_m.krho_points[i] = (i+1) * gp_m.krho_max / gp_m.n_points;
        gp_m.weights[i] = gp_m.krho_max / gp_m.n_points;
    }
    gp_m.use_dcim = true;
    
    // ✅ 关键修复：在频率循环内进行MoM求解
    for (int freq_idx = 0; freq_idx < results->num_freq_points; freq_idx++) {
        double freq = results->frequencies[freq_idx];
        
        // 更新频率配置
        cfg.frequency = freq;
        mom_solver_configure(solver, &cfg);
        
        // 更新频域参数
        FrequencyDomain fd_m;
        fd_m.freq = freq;
        fd_m.omega = 2.0 * M_PI * freq;
        fd_m.k0 = fd_m.omega / 299792458.0;
        fd_m.eta0 = 376.730313561;
        
        // ✅ 更新频率相关的材料属性
        if (model->params.enable_physical_effects) {
            update_frequency_dependent_material_properties(&medium_m, &fd_m, model);
        }
        
        // 更新分层介质
        mom_solver_set_layered_medium(solver, &medium_m, &fd_m, &gp_m);
        
        // 清除之前的激励，重新设置
        // 注意：需要MoM求解器支持清除激励的接口
        for (int pi = 0; pi < model->num_ports; pi++) {
            PCBPortDefinition* P = &model->ports[pi];
            point3d_t pos = { P->position.x, P->position.y, 
                            model->pcb_design->layers[P->layer_index].elevation };
            point3d_t pol = { P->pol_x, P->pol_y, 0.0 };
            double amp = (pi == 0) ? 1.0 : 0.0;  // 单端口激励
            double width = (P->width > 0.0) ? P->width : 0.5;
            mom_solver_add_lumped_excitation(solver, &pos, &pol, amp, width, P->layer_index);
        }
        
        // 装配和求解
        if (mom_solver_assemble_matrix(solver) != 0) {
            printf("警告：频率 %.3f GHz 矩阵装配失败\n", freq/1e9);
            continue;
        }
        
        if (mom_solver_solve(solver) != 0) {
            printf("警告：频率 %.3f GHz MoM求解失败\n", freq/1e9);
            continue;
        }
        
        // 提取结果
        const mom_result_t* mr = mom_solver_get_results(solver);
        if (mr) {
            // 保存电流分布
            for (int i = 0; i < results->num_basis_functions && i < mr->num_basis_functions; i++) {
                int idx = i * results->num_freq_points + freq_idx;
                results->current_magnitude[idx] = mr->current_magnitude ? 
                    mr->current_magnitude[i] : cabs(mr->current_coefficients[i]);
                results->current_phase[idx] = mr->current_phase ? 
                    mr->current_phase[i] : carg(mr->current_coefficients[i]);
            }
        }
        
        printf("频率 %.3f GHz MoM求解完成\n", freq/1e9);
    }
    
    // 清理
    free_layered_medium(&medium_m);
    free_greens_function_params(&gp_m);
    mom_solver_destroy(solver);
}
```

**预计工作量**：1周

---

#### 修复2：实现多端口Z→S矩阵计算

**新增文件**: `src/io/pcb_multiport_calculation.c`

**功能**：从MoM求解结果计算多端口Z和S参数

**实现方案**：

```c
// 多端口Z矩阵计算
int calculate_multiport_z_matrix(
    mom_solver_t* solver,
    const PCBEMModel* model,
    double complex* z_matrix,  // 输出：num_ports × num_ports
    int freq_idx) {
    
    int num_ports = model->num_ports;
    const mom_result_t* mr = mom_solver_get_results(solver);
    
    if (!mr || !mr->current_coefficients) {
        return -1;
    }
    
    // 对每个端口进行激励，计算端口电流
    for (int src_port = 0; src_port < num_ports; src_port++) {
        // 设置端口激励（电压源，幅度1V）
        // ... 设置激励 ...
        
        // 求解
        mom_solver_solve(solver);
        
        // 计算端口电流（通过RWG-Port重叠积分）
        for (int obs_port = 0; obs_port < num_ports; obs_port++) {
            double complex port_current = calculate_port_current(
                solver, obs_port, mr);
            
            // Z_ij = V_i / I_j (当端口j激励，端口i的电压/电流比)
            double complex V_i = (src_port == obs_port) ? 1.0 : 0.0;
            z_matrix[src_port * num_ports + obs_port] = V_i / port_current;
        }
    }
    
    return 0;
}

// S参数计算：S = (Z - Z0)(Z + Z0)^{-1}
int calculate_s_parameters_from_z(
    const double complex* z_matrix,
    double complex* s_matrix,
    int num_ports,
    double z0) {  // 参考阻抗，默认50Ω
    
    // 构建 (Z + Z0*I)
    double complex* z_plus_z0 = (double complex*)calloc(
        num_ports * num_ports, sizeof(double complex));
    double complex* z_minus_z0 = (double complex*)calloc(
        num_ports * num_ports, sizeof(double complex));
    
    for (int i = 0; i < num_ports; i++) {
        for (int j = 0; j < num_ports; j++) {
            int idx = i * num_ports + j;
            if (i == j) {
                z_plus_z0[idx] = z_matrix[idx] + z0;
                z_minus_z0[idx] = z_matrix[idx] - z0;
            } else {
                z_plus_z0[idx] = z_matrix[idx];
                z_minus_z0[idx] = z_matrix[idx];
            }
        }
    }
    
    // 计算 (Z + Z0)^{-1}
    double complex* z_plus_z0_inv = matrix_inverse_complex(z_plus_z0, num_ports);
    if (!z_plus_z0_inv) {
        free(z_plus_z0);
        free(z_minus_z0);
        return -1;
    }
    
    // S = (Z - Z0) * (Z + Z0)^{-1}
    matrix_multiply_complex(z_minus_z0, z_plus_z0_inv, s_matrix, 
                            num_ports, num_ports, num_ports);
    
    free(z_plus_z0);
    free(z_minus_z0);
    free(z_plus_z0_inv);
    
    return 0;
}
```

**预计工作量**：2周

---

#### 修复3：实现RWG-Port重叠积分

**新增文件**: `src/io/pcb_port_overlap.c`

**功能**：计算RWG基函数与端口窗口的重叠积分

**实现方案**：

```c
// RWG-Port重叠积分
double complex calculate_rwg_port_overlap(
    const RWGBasisFunction* rwg,
    const PCBPortDefinition* port,
    const Triangle* mesh_triangles) {
    
    // 1. 定义端口窗口多边形（矩形或圆形）
    clipper2_polygon_t port_window;
    if (port->port_shape == PORT_SHAPE_RECTANGLE) {
        // 创建矩形窗口
        double w = port->width;
        double h = port->height;
        port_window = create_rectangle_polygon(
            port->position.x - w/2, port->position.y - h/2,
            port->position.x + w/2, port->position.y + h/2);
    } else {
        // 圆形窗口
        port_window = pcb_circle_to_polygon(port->position, port->width/2, 36);
    }
    
    // 2. 获取RWG基函数的两个三角形
    Triangle* tri_plus = &mesh_triangles[rwg->plus_triangle_idx];
    Triangle* tri_minus = &mesh_triangles[rwg->minus_triangle_idx];
    
    double complex integral = 0.0 + 0.0*I;
    
    // 3. 对每个三角形进行裁剪和积分
    for (int tri_idx = 0; tri_idx < 2; tri_idx++) {
        Triangle* tri = (tri_idx == 0) ? tri_plus : tri_minus;
        
        // 3.1 将三角形转换为Clipper2多边形
        clipper2_polygon_t tri_poly = triangle_to_clipper2_polygon(tri);
        
        // 3.2 计算交集：port_window ∩ tri_poly
        clipper2_polygons_t intersection = clipper2_intersect(
            port_window, tri_poly, clipper2_fill_non_zero);
        
        // 3.3 对每个交集多边形进行三角剖分
        for (int poly_idx = 0; poly_idx < intersection.size; poly_idx++) {
            clipper2_polygon_t overlap_poly = intersection.polygons[poly_idx];
            
            // 三角剖分
            TriangleResult sub_triangles = triangulate_polygon(overlap_poly);
            
            // 3.4 在每个子三角形上进行7点高斯积分
            for (int st = 0; st < sub_triangles.num_triangles; st++) {
                Triangle* sub_tri = &sub_triangles.triangles[st];
                
                // 7点高斯积分点和权重
                double gauss_points[7][2] = {
                    {0.333333333333333, 0.333333333333333},
                    {0.797426985353087, 0.101286507323456},
                    {0.101286507323456, 0.797426985353087},
                    {0.101286507323456, 0.101286507323456},
                    {0.059715871789770, 0.470142064105115},
                    {0.470142064105115, 0.059715871789770},
                    {0.470142064105115, 0.470142064105115}
                };
                double gauss_weights[7] = {
                    0.225000000000000,
                    0.125939180544827,
                    0.125939180544827,
                    0.125939180544827,
                    0.132394152788506,
                    0.132394152788506,
                    0.132394152788506
                };
                
                // 在子三角形上积分
                for (int gp = 0; gp < 7; gp++) {
                    double xi = gauss_points[gp][0];
                    double eta = gauss_points[gp][1];
                    double weight = gauss_weights[gp];
                    
                    // 计算积分点坐标
                    point3d_t r = interpolate_triangle_point(sub_tri, xi, eta);
                    
                    // 计算RWG基函数值
                    point3d_t rwg_value = evaluate_rwg_basis(rwg, r, tri_idx);
                    
                    // 计算端口方向投影
                    point3d_t port_dir = {port->pol_x, port->pol_y, 0.0};
                    double dot_product = dot(rwg_value, port_dir);
                    
                    // 累加积分
                    integral += weight * dot_product * sub_tri->area;
                }
            }
            
            free_triangle_result(&sub_triangles);
        }
        
        clipper2_free_polygons(&intersection);
    }
    
    return integral;
}

// 计算端口电流
double complex calculate_port_current(
    mom_solver_t* solver,
    int port_idx,
    const mom_result_t* result) {
    
    const PCBEMModel* model = (const PCBEMModel*)mom_solver_get_mesh(solver);
    PCBPortDefinition* port = &model->ports[port_idx];
    
    double complex port_current = 0.0 + 0.0*I;
    
    // 对所有RWG基函数求和
    for (int i = 0; i < result->num_basis_functions; i++) {
        // 计算RWG-Port重叠
        double complex overlap = calculate_rwg_port_overlap(
            &rwg_functions[i], port, model->triangles);
        
        // 乘以电流系数
        port_current += result->current_coefficients[i] * overlap;
    }
    
    return port_current;
}
```

**预计工作量**：3周

---

#### 修复4：实现频率相关物理效应

**新增文件**: `src/io/pcb_physical_effects.c`

**功能**：计算频率相关的材料属性

**实现方案**：

```c
// 更新频率相关的材料属性
void update_frequency_dependent_material_properties(
    LayeredMedium* medium,
    const FrequencyDomain* fd,
    const PCBEMModel* model) {
    
    double omega = fd->omega;
    double freq = fd->freq;
    
    for (int li = 0; li < medium->num_layers; li++) {
        PCBLayerInfo* L = &model->pcb_design->layers[li];
        
        // 1. 导体：Skin Effect（趋肤效应）
        if (L->type == PCB_LAYER_COPPER) {
            double sigma0 = 5.8e7;  // 铜的基础电导率
            double mu_r = 1.0;
            double mu_eff = mu_r * 4.0 * M_PI * 1e-7;
            
            // 计算skin depth
            double skin_depth = sqrt(2.0 / (omega * mu_eff * sigma0));
            
            // 有效电导率（考虑skin effect）
            double t = L->copper_thickness * 1e-6;  // 转换为米
            double effective_conductivity;
            
            if (t > 3.0 * skin_depth) {
                // 厚导体：使用skin depth修正
                effective_conductivity = sigma0 * (1.0 - exp(-2.0 * t / skin_depth));
            } else {
                // 薄导体：使用完整厚度
                effective_conductivity = sigma0;
            }
            
            // 表面粗糙度修正（Huray模型）
            if (L->surface_roughness > 0.0) {
                double Rq = L->surface_roughness * 1e-6;  // 转换为米
                double roughness_factor = 1.0 + 2.0 * pow(Rq / skin_depth, 2.0);
                // 表面电阻修正
                double R_s = sqrt(omega * mu_eff / (2.0 * sigma0));
                double R_s_eff = R_s * roughness_factor;
                // 转换为有效电导率
                effective_conductivity = omega * mu_eff / (2.0 * R_s_eff * R_s_eff);
            }
            
            medium->sigma[li] = effective_conductivity;
        }
        
        // 2. 介质：频率相关的介电常数（色散）
        if (L->type == PCB_LAYER_DIELECTRIC) {
            if (L->dispersion_model == DISPERSION_DEBYE) {
                // Debye模型：ε(ω) = ε∞ + (εs - ε∞) / (1 + jωτ)
                double eps_s = L->epsilon_r_static;
                double eps_inf = L->epsilon_r_inf;
                double tau = L->tau;
                
                double complex eps_complex = eps_inf + 
                    (eps_s - eps_inf) / (1.0 + I * omega * tau);
                
                medium->epsilon_r[li] = creal(eps_complex);
                medium->tan_delta[li] = -cimag(eps_complex) / creal(eps_complex);
                
            } else if (L->dispersion_model == DISPERSION_DRUDE) {
                // Drude模型：ε(ω) = 1 - ωp² / (ω² + jγω)
                double omega_p = L->plasma_frequency;
                double gamma = L->damping_factor;
                
                double complex eps_complex = 1.0 - 
                    (omega_p * omega_p) / (omega * omega + I * omega * gamma);
                
                medium->epsilon_r[li] = creal(eps_complex);
                medium->tan_delta[li] = -cimag(eps_complex) / creal(eps_complex);
                
            } else if (L->frequency_table) {
                // 用户提供的频率表
                medium->epsilon_r[li] = interpolate_frequency_table(
                    L->frequency_table, freq, EPSILON_R);
                medium->tan_delta[li] = interpolate_frequency_table(
                    L->frequency_table, freq, TAN_DELTA);
            } else {
                // 无色散：使用固定值
                medium->epsilon_r[li] = L->dielectric_constant;
                medium->tan_delta[li] = L->loss_tangent;
            }
        }
    }
}
```

**预计工作量**：2周

---

### 3.2 P1 - 重要优化（提升性能）

#### 优化1：稀疏GMRES+ILU求解器

**新增文件**: `src/solvers/mom/mom_sparse_solver.c`

**功能**：实现稀疏矩阵存储和GMRES迭代求解

**实现方案**：

```c
// CSR格式稀疏矩阵
typedef struct {
    int num_rows;
    int num_cols;
    int nnz;  // 非零元素数量
    double complex* values;  // 非零值
    int* col_indices;  // 列索引
    int* row_ptr;  // 行指针
} CSRMatrix;

// 从密集矩阵转换为CSR
CSRMatrix* dense_to_csr(
    const double complex* dense_matrix,
    int n,
    double threshold) {  // 阈值，小于此值的元素视为0
    
    // 统计非零元素
    int nnz = 0;
    for (int i = 0; i < n * n; i++) {
        if (cabs(dense_matrix[i]) > threshold) {
            nnz++;
        }
    }
    
    // 分配CSR矩阵
    CSRMatrix* csr = (CSRMatrix*)calloc(1, sizeof(CSRMatrix));
    csr->num_rows = n;
    csr->num_cols = n;
    csr->nnz = nnz;
    csr->values = (double complex*)calloc(nnz, sizeof(double complex));
    csr->col_indices = (int*)calloc(nnz, sizeof(int));
    csr->row_ptr = (int*)calloc(n + 1, sizeof(int));
    
    // 填充CSR矩阵
    int idx = 0;
    csr->row_ptr[0] = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double complex val = dense_matrix[i * n + j];
            if (cabs(val) > threshold) {
                csr->values[idx] = val;
                csr->col_indices[idx] = j;
                idx++;
            }
        }
        csr->row_ptr[i + 1] = idx;
    }
    
    return csr;
}

// GMRES迭代求解
int gmres_solve(
    const CSRMatrix* A,
    const double complex* b,
    double complex* x,
    int n,
    double tolerance,
    int max_iter,
    int restart) {
    
    // 初始化
    double complex* r = (double complex*)calloc(n, sizeof(double complex));
    double complex* v = (double complex*)calloc(n * (restart + 1), sizeof(double complex));
    double complex* h = (double complex*)calloc((restart + 1) * restart, sizeof(double complex));
    double complex* y = (double complex*)calloc(restart, sizeof(double complex));
    
    // 初始残差
    csr_matrix_vector_multiply(A, x, r);  // r = A * x
    for (int i = 0; i < n; i++) {
        r[i] = b[i] - r[i];
    }
    
    double beta = cnorm(r, n);
    if (beta < tolerance) {
        free(r); free(v); free(h); free(y);
        return 0;  // 已收敛
    }
    
    // 归一化第一个基向量
    for (int i = 0; i < n; i++) {
        v[i] = r[i] / beta;
    }
    
    // GMRES迭代
    for (int outer = 0; outer < max_iter / restart; outer++) {
        // Arnoldi过程
        for (int j = 0; j < restart; j++) {
            // 计算 A * v_j
            double complex* Av = (double complex*)calloc(n, sizeof(double complex));
            csr_matrix_vector_multiply(A, &v[j * n], Av);
            
            // 正交化
            for (int i = 0; i <= j; i++) {
                h[i * restart + j] = cdot(&v[i * n], Av, n);
                for (int k = 0; k < n; k++) {
                    Av[k] -= h[i * restart + j] * v[i * n + k];
                }
            }
            
            // 归一化
            double h_norm = cnorm(Av, n);
            h[(j + 1) * restart + j] = h_norm;
            
            if (h_norm > 1e-12) {
                for (int k = 0; k < n; k++) {
                    v[(j + 1) * n + k] = Av[k] / h_norm;
                }
            }
            
            free(Av);
        }
        
        // 求解最小二乘问题：H * y = beta * e1
        // 使用Givens旋转
        solve_least_squares_givens(h, y, beta, restart);
        
        // 更新解
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < restart; j++) {
                x[i] += y[j] * v[j * n + i];
            }
        }
        
        // 检查收敛
        // ... 计算残差 ...
        if (residual < tolerance) {
            free(r); free(v); free(h); free(y);
            return 0;
        }
    }
    
    free(r); free(v); free(h); free(y);
    return -1;  // 未收敛
}

// ILU预条件
CSRMatrix* ilu_preconditioner(
    const CSRMatrix* A,
    int fill_level) {
    
    // ILU(0)或ILU(k)分解
    // ... 实现ILU分解 ...
    
    return L;  // 返回下三角矩阵
}
```

**预计工作量**：3-4周

---

#### 优化2：ACA/MLFMM压缩

**新增文件**: `src/solvers/mom/mom_fast_multipole.c`

**功能**：实现快速多极子方法压缩远场相互作用

**预计工作量**：2-3个月（复杂算法）

---

### 3.3 P2 - 输出和报告

#### 实现1：CSV输出

**新增文件**: `src/io/pcb_output_csv.c`

**功能**：输出S参数、Z参数等到CSV文件

**预计工作量**：1周

#### 实现2：PNG绘图

**新增文件**: `src/io/pcb_output_plot.c`

**功能**：生成S参数、Smith图等PNG图像

**预计工作量**：2周

---

## 四、实施计划

### 阶段1：核心修复（4-5周）

1. **Week 1**: 修复MoM频率循环
2. **Week 2-3**: 实现多端口Z→S矩阵计算
3. **Week 4**: 实现RWG-Port重叠积分
4. **Week 5**: 实现频率相关物理效应

### 阶段2：性能优化（6-8周）

5. **Week 6-9**: 实现稀疏GMRES+ILU求解器
6. **Week 10-12**: 实现ACA压缩（可选）

### 阶段3：输出和验证（2-3周）

7. **Week 13**: CSV/PNG输出
8. **Week 14-15**: 验证和测试

**总预计时间**：**15-16周**（约4个月）

---

## 五、验证清单

修复完成后，需要验证：

- [ ] MoM求解在每个频率点都执行
- [ ] 多端口Z矩阵正确计算
- [ ] S参数从Z矩阵计算，不使用格林函数近似
- [ ] RWG-Port重叠积分正确
- [ ] 频率相关物理效应正确应用
- [ ] 稀疏求解器正常工作
- [ ] CSV/PNG输出正确

---

## 六、总结

### 当前状态

- **声明评分**: 8/10
- **实际代码评分**: 3.6/10
- **差距**: **巨大**

### 关键问题

1. ❌ MoM频率循环未修复（最严重）
2. ❌ 多端口Z→S计算未实现
3. ❌ RWG-Port重叠未实现
4. ⚠️ 物理效应部分实现但未应用
5. ❌ 稀疏求解器未实现

### 修复优先级

1. **P0（立即）**: MoM频率循环、多端口计算、物理效应应用
2. **P1（重要）**: RWG-Port重叠、稀疏求解器
3. **P2（优化）**: 输出格式、压缩算法

**预计总工作量**：4个月（2-3名工程师）

---

**报告生成时间**: 2025-01-XX  
**验证人员**: AI Assistant  
**验证范围**: PCB代码改进声明验证和补全方案

