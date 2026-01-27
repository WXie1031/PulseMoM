# PulseEM 解决方案使用指南

## 概述

`PulseEM.sln` 是一个统一的 Visual Studio 解决方案，包含三个项目：

1. **PulseIE_Core** - 积分方程公共核心库（静态库）
2. **PulseMoM** - MoM 求解器可执行文件
3. **PulsePEEC** - PEEC 求解器可执行文件

## 解决方案结构

```
PulseEM.sln
├── PulseIE_Core (静态库)
│   ├── 包含所有积分方程公共代码
│   ├── MoM 和 PEEC 求解器实现
│   └── 所有公共库和工具
│
├── PulseMoM (可执行文件)
│   ├── 主程序: src/apps/mom_cli/main_mom.c
│   ├── 链接到: PulseIE_Core.lib
│   └── 输出: PulseMoM.exe
│
└── PulsePEEC (可执行文件)
    ├── 主程序: src/apps/peec_cli/main_peec.c
    ├── 链接到: PulseIE_Core.lib
    └── 输出: PulsePEEC.exe
```

## 项目依赖关系

```
PulseMoM.exe
    └── 依赖 PulseIE_Core.lib

PulsePEEC.exe
    └── 依赖 PulseIE_Core.lib
```

**注意**：两个 exe 项目都依赖 `PulseMoM_Core`，因此构建时会自动先构建核心库。

## 构建步骤

### 1. 打开解决方案

在 Visual Studio 中打开 `PulseEM.sln`

### 2. 选择配置和平台

- **配置**: Debug 或 Release
- **平台**: x64（推荐）或 Win32

### 3. 构建解决方案

- **构建所有项目**: 右键解决方案 → "生成解决方案" (Ctrl+Shift+B)
- **构建单个项目**: 右键项目 → "生成"

### 4. 输出文件位置

构建完成后，可执行文件位于：

- **MoM**: `build\x64\Release\PulseMoM.exe` 或 `build\x64\Debug\PulseMoM.exe`
- **PEEC**: `build\x64\Release\PulsePEEC.exe` 或 `build\x64\Debug\PulsePEEC.exe`

## 设置启动项目

### 运行 MoM Solver

1. 右键 `PulseMoM` 项目
2. 选择 "设为启动项目"
3. 按 F5 运行（需要配置命令行参数）

### 运行 PEEC Solver

1. 右键 `PulsePEEC` 项目
2. 选择 "设为启动项目"
3. 按 F5 运行（需要配置命令行参数）

## 配置命令行参数

### 在 Visual Studio 中配置

1. 右键项目 → "属性"
2. 选择 "调试" → "命令参数"
3. 输入配置文件路径，例如：`config.txt`

### 命令行运行

```powershell
# MoM Solver
.\build\x64\Release\PulseMoM.exe config.txt

# PEEC Solver
.\build\x64\Release\PulsePEEC.exe config.txt
```

## 配置文件示例

创建 `config.txt`：

```
geometry_file=tests/patch_antenna_2x2.stl
geometry_format=stl
frequency=10e9
tolerance=1e-6
max_iterations=1000
mesh_density=10
```

## 项目文件位置

- **解决方案**: `PulseEM.sln`
- **核心库项目**: `src\PulseIE_Core.vcxproj`
- **MoM 项目**: `src\PulseMoM.vcxproj`
- **PEEC 项目**: `src\PulsePEEC.vcxproj`
- **MoM 主程序**: `src\apps\mom_cli\main_mom.c`
- **PEEC 主程序**: `src\apps\peec_cli\main_peec.c`

## 优势

1. **统一管理**: 一个解决方案包含所有项目
2. **代码共享**: 公共代码在 PulseMoM_Core 中，两个 exe 共享
3. **独立构建**: 可以单独构建每个 exe 项目
4. **清晰结构**: 每个 exe 有明确的主程序入口

## 注意事项

1. **构建顺序**: PulseMoM_Core 会自动先构建（因为依赖关系）
2. **库路径**: 确保 `PulseMoM_Core.lib` 在输出目录中
3. **包含路径**: 两个 exe 项目都配置了正确的包含路径
4. **链接库**: 两个 exe 都链接到 `PulseMoM_Core.lib` 和外部库

## 故障排除

### 问题：找不到 PulseIE_Core.lib

**解决**：
1. 确保 PulseIE_Core 项目已构建
2. 检查库输出路径：`build\x64\Release\PulseIE_Core.lib`
3. 检查项目依赖关系是否正确设置

### 问题：找不到头文件

**解决**：
1. 检查项目属性 → C/C++ → 常规 → 附加包含目录
2. 确保路径相对于 `$(ProjectDir)` 正确

### 问题：链接错误

**解决**：
1. 确保所有依赖库都已构建
2. 检查库路径设置
3. 检查外部库（OpenBLAS, HDF5等）是否正确安装

## 从旧解决方案迁移

如果您之前使用 `PulseMoM.sln`：

1. **保留旧解决方案**（可选）：`PulseMoM.sln` 仍然可用
2. **使用新解决方案**：打开 `PulseEM.sln`
3. **项目文件不变**：`PulseMoM_Core.vcxproj` 保持不变
4. **新增项目**：`PulseMoM.vcxproj` 和 `PulsePEEC.vcxproj` 是新添加的
