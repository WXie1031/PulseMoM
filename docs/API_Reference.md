# PulseMoM API Reference

## 概述

本文档提供PulseMoM电磁仿真平台的完整API参考。PulseMoM是一个统一的电磁仿真框架，支持MoM、PEEC和MTL求解器。

**版本**: 1.0
**更新日期**: 2025年

---

## 目录

1. [核心模块](#核心模块)
2. [求解器接口](#求解器接口)
3. [网格生成](#网格生成)
4. [端口支持](#端口支持)
5. [后处理](#后处理)
6. [文件I/O](#文件io)
7. [示例代码](#示例代码)

---

## 核心模块

### 1.1 几何引擎 (`core_geometry.h`)

#### 创建和销毁几何

```c
// 创建几何对象
geom_geometry_t* geom_geometry_create(void);

// 销毁几何对象
void geom_geometry_destroy(geom_geometry_t* geom);
```

#### 添加实体

```c
// 添加几何实体
int geom_geometry_add_entity(geom_geometry_t* geom, const geom_entity_t* entity);

// 获取实体
int geom_geometry_get_entity(geom_geometry_t* geom, int entity_id, geom_entity_t** out);

// 移除实体
int geom_geometry_remove_entity(geom_geometry_t* geom, int entity_id);
```

#### 材料管理

```c
// 添加材料
int geom_geometry_add_material(geom_geometry_t* geom, const geom_material_t* material);

// 添加层
int geom_geometry_add_layer(geom_geometry_t* geom, const geom_layer_t* layer);
```

---

### 1.2 网格引擎 (`core_mesh.h`)

#### 创建和销毁网格

```c
// 创建网格
mesh_t* mesh_create(void);

// 销毁网格
void mesh_destroy(mesh_t* mesh);
```

#### 添加顶点和单元

```c
// 添加顶点
int mesh_add_vertex(mesh_t* mesh, double x, double y, double z);

// 添加单元
int mesh_add_element(mesh_t* mesh, mesh_element_type_t type, const int* vertex_indices, int num_vertices);
```

#### 网格导出

```c
// 导出网格到文件
int mesh_export_to_file(mesh_t* mesh, const char* filename, const char* format);
```

---

## 求解器接口

### 2.1 MoM求解器 (`mom_solver.h`)

#### 创建求解器

```c
// 创建MoM求解器
mom_solver_t* mom_solver_create(void);

// 销毁求解器
void mom_solver_destroy(mom_solver_t* solver);
```

#### 配置求解器

```c
// 设置频率
int mom_solver_set_frequency(mom_solver_t* solver, double frequency);

// 设置网格
int mom_solver_set_mesh(mom_solver_t* solver, const mesh_t* mesh);

// 设置激励
int mom_solver_set_excitation(mom_solver_t* solver, const complex_t* excitation);
```

#### 求解

```c
// 执行求解
int mom_solver_solve(mom_solver_t* solver);

// 获取结果
int mom_solver_get_results(mom_solver_t* solver, mom_result_t* results);
```

---

### 2.2 PEEC求解器 (`peec_solver.h`)

#### 创建求解器

```c
// 创建PEEC求解器
peec_solver_t* peec_solver_create(void);

// 销毁求解器
void peec_solver_destroy(peec_solver_t* solver);
```

#### 配置求解器

```c
// 设置配置
int peec_solver_set_config(peec_solver_t* solver, const peec_config_t* config);

// 设置几何
int peec_solver_set_geometry(peec_solver_t* solver, const geom_geometry_t* geometry);
```

#### 求解

```c
// 执行求解
int peec_solver_solve(peec_solver_t* solver);

// 获取结果
int peec_solver_get_results(peec_solver_t* solver, peec_result_t* results);
```

---

### 2.3 MTL求解器 (`mtl_solver.h`)

#### 创建求解器

```c
// 创建MTL求解器
mtl_solver_t* mtl_solver_create(void);

// 销毁求解器
void mtl_solver_destroy(mtl_solver_t* solver);
```

#### 配置求解器

```c
// 添加电缆
int mtl_solver_add_cable(mtl_solver_t* solver, const mtl_cable_t* cable);

// 设置频率
int mtl_solver_set_frequency(mtl_solver_t* solver, double frequency);
```

#### 求解

```c
// 执行求解
int mtl_solver_solve(mtl_solver_t* solver);

// 获取结果
int mtl_solver_get_results(mtl_solver_t* solver, mtl_results_t* results);
```

---

## 网格生成

### 3.1 网格引擎 (`mesh_engine.h`)

#### 创建网格引擎

```c
// 创建网格引擎
mesh_engine_t* mesh_engine_create(void);

// 销毁网格引擎
void mesh_engine_destroy(mesh_engine_t* engine);
```

#### 生成网格

```c
// 生成网格
mesh_result_t* mesh_engine_generate(mesh_engine_t* engine, const mesh_request_t* request);

// 销毁结果
void mesh_result_destroy(mesh_result_t* result);
```

#### 网格请求配置

```c
typedef struct mesh_request {
    const geom_geometry_t* geometry;  // 输入几何
    mesh_format_t format;             // 输入格式
    bool mom_enabled;                 // 启用MoM网格
    bool peec_enabled;                // 启用PEEC网格
    mesh_element_type_t preferred_type; // 首选单元类型
    mesh_strategy_t strategy;         // 网格策略
    double target_quality;            // 目标质量
    double global_size;               // 全局单元大小
    double min_size;                  // 最小单元大小
    double max_size;                  // 最大单元大小
    // ... 更多参数
} mesh_request_t;
```

---

## 端口支持

### 4.1 扩展端口支持 (`port_support_extended.h`)

#### 创建端口

```c
// 创建端口
extended_port_t* port_create(
    int port_id,
    const char* port_name,
    extended_port_type_t port_type
);

// 销毁端口
void port_destroy(extended_port_t* port);
```

#### 端口类型

```c
typedef enum {
    PORT_TYPE_VOLTAGE_SOURCE,      // 电压源端口
    PORT_TYPE_CURRENT_SOURCE,      // 电流源端口
    PORT_TYPE_S_PARAMETER,         // S参数端口
    PORT_TYPE_MICROSTRIP,          // 微带线端口
    PORT_TYPE_STRIPLINE,           // 带状线端口
    PORT_TYPE_COAXIAL,             // 同轴端口
    PORT_TYPE_WAVEGUIDE,           // 波导端口
    PORT_TYPE_DIFFERENTIAL,        // 差分端口对
    // ... 更多类型
} extended_port_type_t;
```

#### 频率相关阻抗

```c
// 设置频率相关阻抗
int port_set_frequency_dependent_impedance(
    extended_port_t* port,
    const double* frequencies,
    const double* impedances_real,
    const double* impedances_imag,
    int num_points
);

// 获取指定频率的阻抗
int port_get_impedance_at_frequency(
    const extended_port_t* port,
    double frequency,
    double* impedance_real,
    double* impedance_imag
);
```

#### 差分端口

```c
// 创建差分端口对
int port_create_differential_pair(
    int port_id_base,
    const char* port_name_base,
    extended_port_t** pos_port,
    extended_port_t** neg_port
);
```

---

## 后处理

### 5.1 增强后处理 (`post_processing_enhanced.h`)

#### 场数据导出

```c
// 导出场数据
int postprocess_export_field(
    const field_data_t* field_data,
    const mesh_t* mesh,
    const char* filename,
    visualization_format_t format
);

// 导出电流分布
int postprocess_export_current(
    const current_distribution_t* current_dist,
    const mesh_t* mesh,
    const char* filename,
    visualization_format_t format
);
```

#### 可视化格式

```c
typedef enum {
    EXPORT_FORMAT_VTK,            // VTK格式 (ParaView)
    EXPORT_FORMAT_CSV,            // CSV格式
    EXPORT_FORMAT_HDF5,           // HDF5格式
    EXPORT_FORMAT_TECPLOT,        // Tecplot格式
    EXPORT_FORMAT_ENSIGHT,        // EnSight格式
    EXPORT_FORMAT_OPENFOAM        // OpenFOAM格式
} visualization_format_t;
```

#### 功率流计算

```c
// 计算功率流
int postprocess_calculate_power_flow(
    const field_data_t* field_data,
    const double* surface_normal,
    int num_points,
    double* power_real,
    double* power_imag
);
```

#### 场统计

```c
// 计算场统计
int postprocess_calculate_field_statistics(
    const field_data_t* field_data,
    double stats[4]  // [min, max, mean, rms]
);
```

#### 近场到远场变换

```c
// 近场到远场变换
int postprocess_near_to_far_field(
    const field_data_t* near_field,
    const double* observation_angles,
    int num_angles,
    field_data_t* far_field
);
```

#### 辐射方向图

```c
// 计算辐射方向图
int postprocess_calculate_radiation_pattern(
    const field_data_t* far_field,
    const double* theta_angles,
    const double* phi_angles,
    int num_theta,
    int num_phi,
    double* gain_pattern
);
```

---

## 文件I/O

### 6.1 Touchstone格式导出 (`touchstone_export.h`)

#### 导出Touchstone文件

```c
// 导出Touchstone文件
int touchstone_export_file(
    const SParameterSet* sparams,
    const char* filename,
    const touchstone_export_options_t* options
);
```

#### 导出选项

```c
typedef struct {
    touchstone_format_t data_format;      // MA, DB, or RI
    touchstone_freq_unit_t freq_unit;    // Hz, kHz, MHz, GHz
    double reference_impedance;           // 参考阻抗 (Ω)
    double* port_impedances;             // 每端口阻抗
    bool include_comments;                // 包含注释
    bool include_port_names;              // 包含端口名称
    char* description;                    // 文件描述
    int precision;                        // 精度
} touchstone_export_options_t;
```

#### 获取默认选项

```c
// 获取默认选项
touchstone_export_options_t touchstone_get_default_options(void);
```

#### 生成文件名

```c
// 自动生成文件名
int touchstone_generate_filename(
    const char* base_filename,
    int num_ports,
    char* output_buffer,
    size_t buffer_size
);
```

---

### 6.2 PCB文件I/O (`pcb_file_io.h`)

#### 读取PCB文件

```c
// 读取PCB设计文件
PCBDesign* read_pcb_file(const char* filename, PCBFileFormat format);

// 检测文件格式
PCBFileFormat detect_pcb_file_format(const char* filename);
```

#### 支持的格式

```c
typedef enum {
    PCB_FORMAT_GERBER_RS274X,    // Gerber RS-274X
    PCB_FORMAT_GERBER_X2,       // Gerber X2
    PCB_FORMAT_DXF,             // AutoCAD DXF
    PCB_FORMAT_IPC2581,         // IPC-2581
    PCB_FORMAT_ODBPP,           // ODB++
    PCB_FORMAT_KICAD_PCB,       // KiCad PCB
    PCB_FORMAT_ALLEGRO,         // Cadence Allegro
    PCB_FORMAT_ALTIUM,          // Altium Designer
    PCB_FORMAT_EAGLE            // Autodesk EAGLE
} PCBFileFormat;
```

---

## 示例代码

### 7.1 基本MoM仿真

```c
#include "mom_solver.h"
#include "core_mesh.h"
#include "core_geometry.h"

int main() {
    // 创建几何
    geom_geometry_t* geometry = geom_geometry_create();
    
    // 创建网格
    mesh_t* mesh = mesh_create();
    // ... 添加顶点和单元 ...
    
    // 创建MoM求解器
    mom_solver_t* solver = mom_solver_create();
    mom_solver_set_mesh(solver, mesh);
    mom_solver_set_frequency(solver, 1e9);  // 1 GHz
    
    // 设置激励
    complex_t* excitation = (complex_t*)calloc(mesh->num_elements, sizeof(complex_t));
    // ... 设置激励 ...
    mom_solver_set_excitation(solver, excitation);
    
    // 求解
    mom_solver_solve(solver);
    
    // 获取结果
    mom_result_t results;
    mom_solver_get_results(solver, &results);
    
    // 清理
    free(excitation);
    mom_solver_destroy(solver);
    mesh_destroy(mesh);
    geom_geometry_destroy(geometry);
    
    return 0;
}
```

### 7.2 Touchstone格式导出

```c
#include "touchstone_export.h"
#include "enhanced_sparameter_extraction.h"

int export_sparams_to_touchstone(SParameterSet* sparams, const char* filename) {
    // 获取默认选项
    touchstone_export_options_t opts = touchstone_get_default_options();
    
    // 自定义选项
    opts.data_format = TOUCHSTONE_FORMAT_RI;
    opts.freq_unit = TOUCHSTONE_FREQ_GHZ;
    opts.reference_impedance = 50.0;
    opts.include_comments = true;
    opts.description = "PulseMoM S-parameter results";
    
    // 导出
    return touchstone_export_file(sparams, filename, &opts);
}
```

### 7.3 端口创建和配置

```c
#include "port_support_extended.h"

int create_port_example() {
    // 创建微带线端口
    extended_port_t* port = port_create(1, "Port1", PORT_TYPE_MICROSTRIP);
    
    // 设置位置和方向
    port->position = (geom_point_t){0.0, 0.0, 0.0};
    port->direction = (geom_point_t){0.0, 0.0, 1.0};
    port->width = 0.5e-3;  // 0.5 mm
    port->characteristic_impedance = 50.0;
    
    // 设置频率相关阻抗
    double frequencies[] = {1e9, 2e9, 5e9};
    double z_real[] = {50.0, 50.1, 50.2};
    double z_imag[] = {0.0, 0.1, 0.2};
    port_set_frequency_dependent_impedance(port, frequencies, z_real, z_imag, 3);
    
    // 清理
    port_destroy(port);
    
    return 0;
}
```

---

## 错误处理

### 错误代码

大多数函数返回整数错误代码：
- `0`: 成功
- 负数: 错误代码

### 常见错误

- `-1`: 无效参数
- `-2`: 内存分配失败
- `-3`: 文件I/O错误
- `-4`: 数值计算错误

---

## 线程安全

- 大多数函数不是线程安全的
- 多线程使用需要外部同步
- 某些函数（如网格生成）支持OpenMP并行

---

## 内存管理

- 创建函数返回的指针需要调用相应的销毁函数释放
- 某些函数返回的指针由库管理，不需要手动释放
- 检查函数文档了解具体的内存管理策略

---

**文档版本**: 1.0
**最后更新**: 2025年
