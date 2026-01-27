# PulseEM 解决方案说明

## 概述

`PulseEM.sln` 是统一的 Visual Studio 解决方案，包含：

- **PulseIE_Core** - 积分方程公共核心库（包含所有 MoM 和 PEEC 的公共代码）
- **PulseMoM** - MoM 求解器可执行文件（生成 `PulseMoM.exe`）
- **PulsePEEC** - PEEC 求解器可执行文件（生成 `PulsePEEC.exe`）

## 快速开始

### 1. 打开解决方案

在 Visual Studio 中打开 `PulseEM.sln`

### 2. 构建

- 选择配置：**Release** 或 **Debug**
- 选择平台：**x64**（推荐）
- 构建解决方案：**Ctrl+Shift+B**

### 3. 运行

构建完成后，可执行文件位于：
- `build\x64\Release\PulseMoM.exe`
- `build\x64\Release\PulsePEEC.exe`

## 项目结构

```
PulseEM.sln
│
├── PulseIE_Core (静态库)
│   └── 包含所有积分方程源代码和公共库
│
├── PulseMoM (可执行文件)
│   ├── 主程序: src/apps/mom_cli/main_mom.c
│   └── 链接到: PulseIE_Core.lib
│
└── PulsePEEC (可执行文件)
    ├── 主程序: src/apps/peec_cli/main_peec.c
    └── 链接到: PulseIE_Core.lib
```

## 使用可执行文件

### MoM Solver

```powershell
# 创建配置文件
echo geometry_file=tests/patch_antenna_2x2.stl > config.txt
echo frequency=10e9 >> config.txt
echo tolerance=1e-6 >> config.txt

# 运行
.\build\x64\Release\PulseMoM.exe config.txt
```

### PEEC Solver

```powershell
# 创建配置文件
echo geometry_file=tests/pcb_cst_exp2.stp > config.txt
echo frequency=1e9 >> config.txt
echo tolerance=1e-6 >> config.txt

# 运行
.\build\x64\Release\PulsePEEC.exe config.txt
```

## 配置文件格式

配置文件是简单的键值对格式：

```
geometry_file=path/to/geometry.stl
geometry_format=stl
frequency=10e9
tolerance=1e-6
max_iterations=1000
mesh_density=10
```

## 优势

1. **代码共享**: 公共代码在 `PulseIE_Core` 中，两个 exe 共享
2. **独立部署**: 每个 exe 可以独立分发
3. **统一管理**: 一个解决方案管理所有项目
4. **清晰结构**: 每个求解器有明确的主程序入口

## 注意事项

- 构建时，`PulseIE_Core` 会自动先构建（因为依赖关系）
- 两个 exe 都链接到相同的 `PulseIE_Core.lib`
- IE (Integral Equation) 表示积分方程，MoM 和 PEEC 都是积分方程方法
- 确保外部库（OpenBLAS, HDF5等）已正确安装和配置
