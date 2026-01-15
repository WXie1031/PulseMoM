# PulseMoM 代码组织和命名规范

## 命名规范

### 文件命名规则

1. **使用功能描述，不使用"advanced"、"enhanced"后缀**
   - ✅ `mom_layered_medium.h` - MoM分层介质支持
   - ✅ `mom_time_domain.h` - MoM时域分析
   - ✅ `peec_layered_medium.h` - PEEC分层介质支持
   - ✅ `mtl_parameter_import.h` - MTL参数矩阵导入
   - ✅ `coupling_quasistatic.h` - 准静态耦合
   - ✅ `excitation_plane_wave.h` - 平面波激励
   - ✅ `export_hdf5.h` - HDF5导出
   - ✅ `export_vtk.h` - VTK导出
   - ✅ `export_formats.h` - 扩展格式导出
   - ✅ `postprocessing_field.h` - 场后处理
   - ❌ `mom_multilayer_enhanced.h` - 已删除
   - ❌ `peec_multilayer_enhanced.h` - 已删除
   - ❌ `mtl_enhanced.h` - 已删除

2. **命名格式**: `<module>_<feature>.h` 或 `<feature>_<method>.h`
   - 模块名在前，功能在后
   - 例如: `mom_layered_medium.h`, `peec_time_domain.h`

3. **目录组织**:
   - 求解器特定功能: `src/solvers/<solver>/<feature>.h`
   - 核心通用功能: `src/core/<feature>.h`
   - I/O功能: `src/io/<feature>.h`

### 函数命名规则

1. **格式**: `<module>_<action>_<object>()`
   - 例如: `mom_solver_set_layered_medium()`
   - 例如: `peec_solver_configure_pcb()`
   - 例如: `mtl_solver_import_parameter_matrices()`

2. **动作动词**:
   - `create` - 创建
   - `destroy` / `free` - 销毁/释放
   - `set` - 设置
   - `get` - 获取
   - `configure` - 配置
   - `solve` - 求解
   - `compute` - 计算
   - `export` - 导出
   - `import` - 导入

3. **对象名词**:
   - 使用具体对象名，不使用"advanced"、"enhanced"
   - 例如: `layered_medium`, `time_domain`, `parameter_matrices`

### 结构体命名规则

1. **格式**: `<module>_<feature>_<type>_t`
   - 例如: `mom_time_domain_config_t`
   - 例如: `peec_layered_medium_config_t`
   - 例如: `mtl_parameter_matrices_t`

2. **类型后缀**:
   - `_config_t` - 配置结构
   - `_results_t` - 结果结构
   - `_matrix_t` - 矩阵结构
   - `_data_t` - 数据结构

### 枚举命名规则

1. **格式**: `<module>_<feature>_<value>`
   - 例如: `EXCITATION_PLANE_WAVE_TE`
   - 例如: `COUPLING_QUASISTATIC_CAPACITIVE`
   - 例如: `EXPORT_FORMAT_TOUCHSTONE`

---

## 代码组织

### 目录结构

```
src/
├── core/                    # 核心功能
│   ├── coupling_quasistatic.h/c      # 准静态耦合
│   ├── excitation_plane_wave.h/c     # 平面波激励
│   ├── export_hdf5.h/c               # HDF5导出
│   ├── export_vtk.h/c                # VTK导出
│   ├── export_formats.h/c            # 扩展格式导出
│   ├── postprocessing_field.h/c      # 场后处理
│   ├── touchstone_export.h/c         # Touchstone导出
│   └── port_support_extended.h/c     # 扩展端口支持
│
├── solvers/
│   ├── mom/
│   │   ├── mom_solver.h              # MoM基础接口
│   │   ├── mom_solver_unified.c      # MoM统一实现
│   │   ├── mom_layered_medium.h/c     # MoM分层介质
│   │   └── mom_time_domain.h/c       # MoM时域分析
│   │
│   ├── peec/
│   │   ├── peec_solver.h             # PEEC基础接口
│   │   ├── peec_solver_unified.c     # PEEC统一实现
│   │   ├── peec_layered_medium.h/c   # PEEC分层介质
│   │   ├── peec_time_domain.h/c      # PEEC时域分析
│   │   └── peec_time_domain.c        # PEEC时域实现（已存在）
│   │
│   └── mtl/
│       ├── mtl_solver.h              # MTL基础接口
│       ├── mtl_parameter_import.h/c  # MTL参数导入
│       ├── mtl_wideband.h/c          # MTL宽频分析
│       └── mtl_time_domain.h/c       # MTL时域分析
│
└── io/
    └── pcb_simulation_workflow.c      # PCB工作流
```

### 文件合并规则

1. **相同功能的文件应该合并**
   - 如果多个文件实现相同功能，合并为一个文件
   - 例如: `mom_solver_min.c` 和 `mom_solver_unified.c` 如果功能重复，应该合并

2. **版本文件处理**
   - 保留最新版本，删除旧版本
   - 或者合并功能到统一文件

---

## 实现状态

### 已重命名的文件

| 旧文件名 | 新文件名 | 位置 |
|---------|---------|------|
| `mom_multilayer_enhanced.h` | `mom_layered_medium.h` | `src/solvers/mom/` |
| `peec_multilayer_enhanced.h` | `peec_layered_medium.h` | `src/solvers/peec/` |
| `mtl_enhanced.h` | `mtl_parameter_import.h`<br>`mtl_wideband.h`<br>`mtl_time_domain.h` | `src/solvers/mtl/` |
| `quasistatic_coupling.h` | `coupling_quasistatic.h` | `src/core/` |
| `plane_wave_excitation.h` | `excitation_plane_wave.h` | `src/core/` |
| `hdf5_export.h` | `export_hdf5.h` | `src/core/` |
| `vtk_export_enhanced.h` | `export_vtk.h` | `src/core/` |
| `output_format_extended.h` | `export_formats.h` | `src/core/` |
| `post_processing_enhanced.h` | `postprocessing_field.h` | `src/core/` |

### 已实现的C文件

1. ✅ `src/solvers/mom/mom_layered_medium.c` - MoM分层介质实现
2. ✅ `src/solvers/mom/mom_time_domain.c` - MoM时域分析实现
3. ✅ `src/solvers/peec/peec_layered_medium.c` - PEEC分层介质实现
4. ✅ `src/solvers/peec/peec_time_domain.c` - PEEC时域分析实现
5. ✅ `src/solvers/mtl/mtl_parameter_import.c` - MTL参数导入实现
6. ✅ `src/solvers/mtl/mtl_wideband.c` - MTL宽频分析实现
7. ✅ `src/solvers/mtl/mtl_time_domain.c` - MTL时域分析实现
8. ✅ `src/core/coupling_quasistatic.c` - 准静态耦合实现
9. ✅ `src/core/excitation_plane_wave.c` - 平面波激励实现

### 待实现的C文件

1. ⚠️ `src/core/export_hdf5.c` - HDF5导出实现
2. ⚠️ `src/core/export_vtk.c` - VTK导出实现
3. ⚠️ `src/core/export_formats.c` - 扩展格式导出实现
4. ⚠️ `src/core/postprocessing_field.c` - 场后处理实现

---

## 使用示例

### MoM分层介质

```c
#include "mom_solver.h"
#include "mom_layered_medium.h"

// 创建MoM求解器
mom_solver_t* solver = mom_solver_create(&config);

// 配置分层介质（PCB模式）
mom_solver_configure_pcb(solver, pcb_layers, num_layers);

// 或配置天线基板
mom_solver_configure_antenna(solver, 1.6e-3, 4.4, 0.02);
```

### MoM时域分析

```c
#include "mom_time_domain.h"

// 配置时域分析
mom_time_domain_config_t td_config = mom_time_domain_get_default_config();
td_config.time_start = 0.0;
td_config.time_stop = 10e-9;

// 求解时域
mom_time_domain_results_t td_results = {0};
mom_solver_solve_time_domain(solver, frequencies, n_freq, &td_config, &td_results);
```

### 平面波激励

```c
#include "excitation_plane_wave.h"

// 创建平面波激励
excitation_plane_wave_t pw = excitation_plane_wave_create(
    1e9, 45.0, 0.0, EXCITATION_PLANE_WAVE_TE
);

// 计算电场
double E[3];
excitation_plane_wave_compute_electric_field(&pw, x, y, z, E);
```

### 准静态耦合

```c
#include "coupling_quasistatic.h"

// 配置准静态耦合
coupling_quasistatic_config_t config = coupling_quasistatic_get_default_config();

// 计算耦合矩阵
coupling_quasistatic_matrix_t coupling_matrix = {0};
coupling_quasistatic_compute_matrix(geometry, mesh, &config, &coupling_matrix);
```

---

## 代码质量要求

1. **一致性**: 所有文件遵循相同的命名规范
2. **清晰性**: 文件名和函数名直接反映功能
3. **可维护性**: 避免使用模糊的后缀（advanced, enhanced）
4. **模块化**: 功能分离到独立文件
5. **文档化**: 每个文件有清晰的注释说明

---

**文档版本**: 1.0
**最后更新**: 2025年
