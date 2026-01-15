# PulseMoM 用户指南

**版本**: 1.0  
**最后更新**: 2025年1月

## 目录

1. [快速开始](#快速开始)
2. [核心概念](#核心概念)
3. [MoM求解器使用](#mom求解器使用)
4. [PEEC求解器使用](#peec求解器使用)
5. [输入输出格式](#输入输出格式)
6. [性能优化](#性能优化)
7. [常见问题](#常见问题)

---

## 快速开始

### 系统要求

- **操作系统**: Windows 10/11, Linux, macOS
- **编译器**: MSVC 2019+, GCC 7+, Clang 10+
- **依赖**: CMake 3.16+, OpenMP (可选), BLAS/LAPACK (可选)

### 编译安装

```bash
# 克隆仓库
git clone <repository-url>
cd PulseMoM

# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build . --config Release
```

详细编译指南见：[快速启动指南](快速启动指南.md)

### 第一个示例

```c
#include "solvers/mom/mom_solver.h"

int main() {
    // 创建MoM求解器
    mom_solver_t* solver = mom_solver_create(NULL);
    
    // 设置频率
    solver->config.frequency = 1e9;  // 1 GHz
    
    // 加载几何
    // ... 加载网格或几何文件 ...
    
    // 求解
    mom_result_t* result = mom_solve_frequency(solver, 1e9);
    
    // 输出结果
    // ... 处理结果 ...
    
    // 清理
    mom_solver_destroy(solver);
    return 0;
}
```

---

## 核心概念

### MoM (Method of Moments)

**矩量法**是求解电磁积分方程的标准方法：

- **基函数**: RWG (Rao-Wilton-Glisson) 基函数
- **网格**: 三角形表面网格
- **应用**: 天线设计、RCS分析、散射计算

### PEEC (Partial Element Equivalent Circuit)

**部分元等效电路**将电磁问题转化为电路问题：

- **几何**: Manhattan矩形结构
- **电路元件**: 部分电感、电阻、电容
- **应用**: PCB分析、封装建模、互连分析

### 统一架构

PulseMoM采用统一的核心框架：
- 共享几何引擎
- 共享网格生成
- 共享核函数和格林函数
- 统一的线性求解器

---

## MoM求解器使用

### 基本配置

```c
mom_config_t config = {
    .frequency = 1e9,              // 频率 (Hz)
    .max_iterations = 200,        // 最大迭代次数
    .tolerance = 1e-6,            // 收敛容差
    .num_threads = 8,             // OpenMP线程数
    
    // EFIE近场/奇异项处理（2025年新增）
    .enable_duffy_transform = true,        // 启用Duffy变换
    .near_field_threshold = 0.1,          // 近场阈值（波长）
    .use_analytic_self_term = true,       // 使用解析自项
    .self_term_regularization = 1e-6      // 自项正则化
};
```

### 频率扫描

```c
double freq_start = 1e9;
double freq_stop = 10e9;
int n_points = 100;

for (int i = 0; i < n_points; i++) {
    double freq = freq_start + (freq_stop - freq_start) * i / (n_points - 1);
    mom_result_t* result = mom_solve_frequency(solver, freq);
    // 处理结果...
}
```

### 远场计算

```c
// 计算辐射方向图
mom_compute_radiation_pattern_unified(
    mesh, currents, frequency,
    0.0, M_PI,      // theta范围
    0.0, 2*M_PI,    // phi范围
    91, 181,        // 角度点数
    far_field       // 输出数组
);
```

### 性能优化选项

```c
// 使用加速算法
config.algorithm = MOM_ALGORITHM_ACA;      // ACA低秩压缩
config.aca_tolerance = 1e-6;               // ACA容差
config.aca_max_rank = 100;                 // 最大秩

// 或使用MLFMM
config.algorithm = MOM_ALGORITHM_MLFMM;    // 多层快速多极方法
config.mlfmm_order = 10;                   // 多极展开阶数
```

---

## PEEC求解器使用

### 基本配置

```c
peec_config_t config = {
    .frequency = 1e9,
    .skin_effect = true,          // 启用趋肤效应
    .proximity_effect = true,     // 启用邻近效应
    .manhattan_geometry = true    // Manhattan几何
};
```

### 电路提取

```c
peec_solver_t* solver = peec_solver_create(&config);

// 提取部分元
peec_result_t* result = peec_extract_partial_elements(solver);

// 导出SPICE网表
peec_export_spice(solver, "circuit.sp", frequency);
```

---

## 输入输出格式

### 支持的输入格式

- **几何文件**: GDSII, Gerber, DXF, OASIS, STEP, IGES, STL, OBJ
- **网格文件**: 自定义格式（通过mesh API）
- **材料文件**: CST格式、自定义XML

### 输出格式

- **Touchstone**: S参数（.s2p, .s4p等）
- **HDF5**: 完整结果数据
- **VTK**: 可视化数据（ParaView）
- **CSV**: 表格数据

### 示例：导出Touchstone

```c
touchstone_export_t export;
export.frequencies = frequencies;
export.sparams = sparams;
export.num_ports = 2;
export.num_freqs = n_freqs;

touchstone_export_to_file(&export, "results.s2p");
```

---

## 性能优化

### EFIE近场/奇异项处理（2025年优化）

PulseMoM已实现完整的近场/奇异项处理：

#### 配置参数

```c
// 近场阈值（波长单位）
config.near_field_threshold = 0.1;  // 默认0.1波长

// 使用建议：
// - 密集网格（间距 < 0.1λ）: 0.01-0.05 波长（高精度）
// - 中等网格（间距 ~ 0.1λ）: 0.1 波长（默认，平衡）
// - 稀疏网格（间距 > 0.5λ）: 0.5-1.0 波长（快速）

// 解析自项（强烈建议启用）
config.use_analytic_self_term = true;
config.self_term_regularization = 1e-6;  // 默认值通常足够
```

#### 性能影响

- **Duffy变换**: 比标准积分慢2-3倍，但只用于近场对
- **总体影响**: < 10%性能开销，但显著提高精度
- **优化效果**: 5-12%性能提升（通过常量预计算、距离优化等）

详见：[EFIE优化实现](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md#12-efie-近场奇异项处理实现细节2025年更新)

### 并行计算

```c
// 设置OpenMP线程数
config.num_threads = 8;  // 或使用 omp_set_num_threads(8)

// 对于大规模问题，建议：
// - 小问题（N < 1000）: 1-2线程
// - 中等问题（N < 10000）: 4-8线程
// - 大问题（N > 10000）: 8-16线程
```

### 内存优化

- 使用压缩算法（ACA/H-matrix）减少内存占用
- 对于大规模问题，考虑使用MLFMM
- 启用统计功能监控内存使用（`MOM_ENABLE_STATISTICS`）

---

## 常见问题

### Q1: 如何选择MoM还是PEEC？

**A**: 
- **MoM**: 适用于天线、散射、辐射问题（全波分析）
- **PEEC**: 适用于PCB、封装、互连问题（电路分析）
- **混合**: 复杂系统可使用混合方法

### Q2: 网格密度如何选择？

**A**: 
- **标准精度**: λ/10（10个点每波长）
- **高精度**: λ/20
- **极高精度**: λ/50
- 近场区域需要更密网格

### Q3: 如何验证结果正确性？

**A**:
1. 与解析解对比（如PEC球）
2. 与商用软件对比（FEKO/CST/HFSS）
3. 检查能量守恒、被动性、因果性
4. 网格收敛性测试

### Q4: 性能优化建议？

**A**:
1. 启用Duffy变换和解析自项（默认已启用）
2. 使用合适的近场阈值（默认0.1波长）
3. 对于大规模问题，使用加速算法（ACA/MLFMM）
4. 合理设置OpenMP线程数
5. 使用Release模式编译

### Q5: 如何处理多层介质？

**A**:
```c
// 设置分层介质
layered_medium_t* medium = layered_medium_create();
// ... 配置层结构 ...
mom_solver_set_layered_medium(solver, medium);
```

详见：[多层介质支持](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md)

---

## 更多资源

- **API参考**: [API_Reference.md](API_Reference.md)
- **架构文档**: [WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md](WORKFLOW_REFACTORING_AND_PLATFORM_ROADMAP.md)
- **代码分析**: [代码分析报告.md](../代码分析报告.md)
- **专项指南**: [GUIDES/](GUIDES/README.md) - 依赖管理、文件格式、算法等
- **问题反馈**: 提交Issue或联系开发团队

---

**注意**: 本文档基于PulseMoM最新版本（2025年1月）。如有疑问，请参考[主文档索引](MASTER_DOCUMENTATION_INDEX.md)。
