# PulseMoM 用户手册

## 概述

PulseMoM是一个统一的电磁仿真平台，支持Method of Moments (MoM)、Partial Element Equivalent Circuit (PEEC)和Multi-conductor Transmission Line (MTL)求解器。本手册提供使用PulseMoM进行电磁仿真的详细指南。

**版本**: 1.0
**更新日期**: 2025年

---

## 目录

1. [快速开始](#快速开始)
2. [基本概念](#基本概念)
3. [工作流程](#工作流程)
4. [端口配置](#端口配置)
5. [结果导出](#结果导出)
6. [后处理](#后处理)
7. [常见问题](#常见问题)
8. [示例教程](#示例教程)

---

## 快速开始

### 安装

PulseMoM支持Windows、Linux和macOS平台。安装步骤请参考安装文档。

### 第一个仿真

以下是一个简单的MoM仿真示例：

```c
#include "mom_solver.h"
#include "core_mesh.h"

int main() {
    // 1. 创建网格
    mesh_t* mesh = mesh_create();
    // ... 添加几何 ...
    
    // 2. 创建求解器
    mom_solver_t* solver = mom_solver_create();
    mom_solver_set_mesh(solver, mesh);
    mom_solver_set_frequency(solver, 1e9);  // 1 GHz
    
    // 3. 求解
    mom_solver_solve(solver);
    
    // 4. 获取结果
    mom_result_t results;
    mom_solver_get_results(solver, &results);
    
    // 5. 清理
    mom_solver_destroy(solver);
    mesh_destroy(mesh);
    
    return 0;
}
```

---

## 基本概念

### 求解器类型

#### Method of Moments (MoM)
- **适用场景**: 天线设计、RCS分析、电磁散射
- **特点**: 精确、支持快速算法（MLFMM、ACA）
- **网格要求**: 三角网格

#### Partial Element Equivalent Circuit (PEEC)
- **适用场景**: PCB分析、封装分析、电源完整性
- **特点**: 电路提取、支持多层介质
- **网格要求**: Manhattan网格或三角网格

#### Multi-conductor Transmission Line (MTL)
- **适用场景**: 电缆分析、线束分析
- **特点**: 传输线方程、支持多导体
- **网格要求**: 线网格

### 网格类型

- **三角网格**: 用于MoM求解器
- **Manhattan网格**: 用于PEEC求解器（矩形）
- **混合网格**: 支持多种单元类型

---

## 工作流程

### 1. 模型导入

支持多种CAD格式：
- **PCB格式**: Gerber, DXF, IPC-2581, ODB++, KiCad, Allegro, Altium, EAGLE
- **3D格式**: STEP, IGES, STL, OBJ

```c
// 读取PCB文件
PCBDesign* pcb = read_pcb_file("design.gbr", PCB_FORMAT_GERBER_RS274X);

// 或读取3D CAD文件
geom_geometry_t* geometry = geom_geometry_create();
// ... 导入几何 ...
```

### 2. 网格生成

```c
// 创建网格请求
mesh_request_t request = {0};
request.geometry = geometry;
request.mom_enabled = true;
request.preferred_type = MESH_ELEMENT_TRIANGLE;
request.global_size = 0.001;  // 1 mm

// 生成网格
mesh_engine_t* engine = mesh_engine_create();
mesh_result_t* result = mesh_engine_generate(engine, &request);
mesh_t* mesh = result->mesh;
```

### 3. 端口设置

```c
// 创建端口
extended_port_t* port = port_create(1, "Port1", PORT_TYPE_MICROSTRIP);
port->position = (geom_point_t){0.0, 0.0, 0.0};
port->characteristic_impedance = 50.0;
port->width = 0.5e-3;  // 0.5 mm
```

### 4. 求解

```c
// MoM求解
mom_solver_t* solver = mom_solver_create();
mom_solver_set_mesh(solver, mesh);
mom_solver_set_frequency(solver, 1e9);
mom_solver_solve(solver);
```

### 5. 结果导出

```c
// 导出Touchstone格式
SParameterSet* sparams = extract_sparameters_from_mom(...);
touchstone_export_options_t opts = touchstone_get_default_options();
touchstone_export_file(sparams, "results.s2p", &opts);
```

---

## 端口配置

### 端口类型

#### 1. 电压源端口
```c
extended_port_t* port = port_create(1, "V1", PORT_TYPE_VOLTAGE_SOURCE);
port->excitation_magnitude = 1.0;  // 1 V
port->excitation_phase = 0.0;     // 0 degrees
```

#### 2. 电流源端口
```c
extended_port_t* port = port_create(1, "I1", PORT_TYPE_CURRENT_SOURCE);
port->excitation_magnitude = 0.1;  // 0.1 A
```

#### 3. 微带线端口
```c
extended_port_t* port = port_create(1, "MS1", PORT_TYPE_MICROSTRIP);
port->width = 0.5e-3;  // 0.5 mm
port->characteristic_impedance = 50.0;
```

#### 4. 差分端口
```c
extended_port_t* pos_port, *neg_port;
port_create_differential_pair(1, "Diff1", &pos_port, &neg_port);
```

### 频率相关阻抗

```c
// 设置频率相关阻抗
double frequencies[] = {1e9, 2e9, 5e9};
double z_real[] = {50.0, 50.1, 50.2};
double z_imag[] = {0.0, 0.1, 0.2};
port_set_frequency_dependent_impedance(port, frequencies, z_real, z_imag, 3);
```

---

## 结果导出

### Touchstone格式

Touchstone格式是S参数的标准格式，支持多种工具（Keysight ADS、Cadence Spectre等）。

#### 基本导出

```c
touchstone_export_options_t opts = touchstone_get_default_options();
opts.data_format = TOUCHSTONE_FORMAT_RI;  // Real-Imaginary
opts.freq_unit = TOUCHSTONE_FREQ_GHZ;
opts.reference_impedance = 50.0;
touchstone_export_file(sparams, "results.s2p", &opts);
```

#### 数据格式选项

- **RI (Real-Imaginary)**: 实部-虚部格式
- **MA (Magnitude-Angle)**: 幅度-相位格式
- **DB (dB-Angle)**: 分贝-相位格式

#### 频率单位

- **Hz**: 赫兹
- **kHz**: 千赫兹
- **MHz**: 兆赫兹
- **GHz**: 吉赫兹

### CSV格式

```c
// 导出S参数到CSV
FILE* fp = fopen("sparams.csv", "w");
fprintf(fp, "freq_GHz,S11_real,S11_imag,S21_real,S21_imag\n");
// ... 写入数据 ...
fclose(fp);
```

### VTK格式（场可视化）

```c
// 导出场数据到VTK
field_data_t field_data = {0};
// ... 填充场数据 ...
postprocess_export_field(&field_data, mesh, "field.vtk", EXPORT_FORMAT_VTK);
```

---

## 后处理

### 场可视化

```c
// 导出场数据
postprocess_export_field(&field_data, mesh, "field.vtk", EXPORT_FORMAT_VTK);

// 导出电流分布
postprocess_export_current(&current_dist, mesh, "current.vtk", EXPORT_FORMAT_VTK);
```

### 功率流计算

```c
double power_real, power_imag;
postprocess_calculate_power_flow(&field_data, surface_normal, num_points, 
                                  &power_real, &power_imag);
printf("Power: %.3e + j%.3e W\n", power_real, power_imag);
```

### 场统计

```c
double stats[4];  // [min, max, mean, rms]
postprocess_calculate_field_statistics(&field_data, stats);
printf("Field statistics: min=%.3e, max=%.3e, mean=%.3e, rms=%.3e\n",
       stats[0], stats[1], stats[2], stats[3]);
```

### 近场到远场变换

```c
// 定义观察角度
double angles[] = {0.0, 0.0, 45.0, 0.0, 90.0, 0.0};  // [theta1, phi1, ...]
int num_angles = 3;

// 变换
field_data_t far_field = {0};
postprocess_near_to_far_field(&near_field, angles, num_angles, &far_field);
```

### 辐射方向图

```c
// 定义角度网格
double theta_angles[] = {0, 15, 30, ..., 180};
double phi_angles[] = {0, 15, 30, ..., 360};
int num_theta = 13, num_phi = 25;

// 计算增益方向图
double* gain_pattern = (double*)malloc(num_theta * num_phi * sizeof(double));
postprocess_calculate_radiation_pattern(&far_field, theta_angles, phi_angles,
                                        num_theta, num_phi, gain_pattern);
```

---

## 常见问题

### Q1: 如何选择合适的求解器？

- **MoM**: 用于天线、RCS、散射问题
- **PEEC**: 用于PCB、封装、电源完整性
- **MTL**: 用于电缆、线束分析

### Q2: 网格密度如何选择？

一般规则：
- 每个波长至少10个单元
- 关键区域（端口、边缘）需要更密的网格
- 使用自适应网格细化

### Q3: 如何验证结果？

- 检查S参数的无源性（passivity）
- 检查S参数的因果性（causality）
- 与解析解或测量结果对比

### Q4: 内存不足怎么办？

- 使用快速算法（MLFMM、ACA）
- 减少网格密度
- 使用GPU加速（如果可用）

---

## 示例教程

### 教程1: 简单PCB仿真

```c
// 1. 读取PCB文件
PCBDesign* pcb = read_pcb_file("pcb.gbr", PCB_FORMAT_GERBER_RS274X);

// 2. 创建电磁模型
PCBEMModel* model = create_pcb_em_model(pcb);

// 3. 生成网格
generate_pcb_mesh(model);

// 4. 设置端口
// ... 配置端口 ...

// 5. 运行仿真
PCBWorkflowController* controller = create_pcb_workflow_controller();
run_complete_pcb_simulation(controller);

// 6. 导出结果
export_pcb_simulation_data(controller, "touchstone");
```

### 教程2: 天线辐射方向图

```c
// 1. 创建MoM求解器
mom_solver_t* solver = mom_solver_create();
// ... 设置网格和激励 ...

// 2. 求解
mom_solver_solve(solver);

// 3. 计算远场
field_data_t far_field = {0};
postprocess_near_to_far_field(&near_field, angles, num_angles, &far_field);

// 4. 计算辐射方向图
double* gain = (double*)malloc(num_theta * num_phi * sizeof(double));
postprocess_calculate_radiation_pattern(&far_field, theta, phi, 
                                        num_theta, num_phi, gain);

// 5. 导出
postprocess_export_field(&far_field, mesh, "pattern.vtk", EXPORT_FORMAT_VTK);
```

### 教程3: 差分端口配置

```c
// 创建差分端口对
extended_port_t* pos_port, *neg_port;
port_create_differential_pair(1, "Diff1", &pos_port, &neg_port);

// 配置端口
pos_port->width = 0.2e-3;  // 0.2 mm
neg_port->width = 0.2e-3;
pos_port->characteristic_impedance = 100.0;  // 差分阻抗
neg_port->characteristic_impedance = 100.0;

// 设置频率相关阻抗
double freqs[] = {1e9, 5e9, 10e9};
double z_real[] = {100.0, 100.1, 100.2};
double z_imag[] = {0.0, 0.1, 0.2};
port_set_frequency_dependent_impedance(pos_port, freqs, z_real, z_imag, 3);
port_set_frequency_dependent_impedance(neg_port, freqs, z_real, z_imag, 3);
```

---

## 技术支持

如有问题，请参考：
- API参考文档: `docs/API_Reference.md`
- 代码分析报告: `docs/comprehensive_code_analysis_and_fecko_comparison.md`
- 项目GitHub页面

---

**文档版本**: 1.0
**最后更新**: 2025年
