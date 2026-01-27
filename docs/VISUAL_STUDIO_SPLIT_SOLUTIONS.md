# Visual Studio 独立解决方案创建指南

## 概述

本指南说明如何创建两个独立的 Visual Studio 解决方案：
1. **MoM_Solver.sln** - 仅包含 MoM 求解器和其依赖
2. **PEEC_Solver.sln** - 仅包含 PEEC 求解器和其依赖

## 方法 1：使用 CMake 生成（推荐）

### 步骤 1：创建 MoM 解决方案

```powershell
# 创建 MoM 构建目录
mkdir build_mom
cd build_mom

# 使用专门的 CMakeLists 文件生成解决方案
cmake -G "Visual Studio 17 2022" -A x64 ..\CMakeLists_MoM_Solver.txt

# 或者直接指定源目录
cmake -G "Visual Studio 17 2022" -A x64 -S .. -B . -DCMAKE_BUILD_TYPE=Release
```

这将在 `build_mom` 目录生成 `MoM_Solver.sln`

### 步骤 2：创建 PEEC 解决方案

```powershell
# 创建 PEEC 构建目录
mkdir build_peec
cd build_peec

# 生成 PEEC 解决方案
cmake -G "Visual Studio 17 2022" -A x64 ..\CMakeLists_PEEC_Solver.txt

# 或者直接指定源目录
cmake -G "Visual Studio 17 2022" -A x64 -S .. -B . -DCMAKE_BUILD_TYPE=Release
```

这将在 `build_peec` 目录生成 `PEEC_Solver.sln`

## 方法 2：手动创建解决方案（如果 CMake 方法不适用）

### 创建 MoM_Solver.sln

1. 在 Visual Studio 中创建新解决方案
2. 添加以下项目（从现有的 .vcxproj 文件）：
   - `core_base_library.vcxproj`
   - `geometry_processing_library.vcxproj`
   - `mesh_generation_library.vcxproj`
   - `electromagnetic_kernel_library.vcxproj`
   - `mom_solver.vcxproj`
   - `mom_solver_exe.vcxproj` (新建)

3. 设置项目依赖关系：
   - `mom_solver_exe` 依赖所有库项目
   - `mom_solver` 依赖公共库
   - 公共库按依赖顺序排列

### 创建 PEEC_Solver.sln

类似地创建 PEEC 解决方案，包含：
   - `core_base_library.vcxproj`
   - `geometry_processing_library.vcxproj`
   - `mesh_generation_library.vcxproj`
   - `electromagnetic_kernel_library.vcxproj`
   - `peec_solver.vcxproj`
   - `peec_solver_exe.vcxproj` (新建)

## 项目文件创建

### MoM Solver Executable 项目

创建 `src/apps/mom_cli/mom_solver_exe.vcxproj`，包含：
- 源文件：`main_mom.c`
- 链接库：mom_solver, electromagnetic_kernel_library, geometry_processing_library, mesh_generation_library, core_base_library
- 输出类型：Application (.exe)

### PEEC Solver Executable 项目

创建 `src/apps/peec_cli/peec_solver_exe.vcxproj`，包含：
- 源文件：`main_peec.c`
- 链接库：peec_solver, electromagnetic_kernel_library, mesh_generation_library, geometry_processing_library, core_base_library
- 输出类型：Application (.exe)

## 使用脚本自动生成

### generate_mom_solution.bat

```batch
@echo off
echo Generating MoM Solver Solution...

if not exist build_mom mkdir build_mom
cd build_mom

cmake -G "Visual Studio 17 2022" -A x64 ..\CMakeLists_MoM_Solver.txt

if %ERRORLEVEL% EQU 0 (
    echo.
    echo MoM Solver solution generated successfully!
    echo Location: %CD%\MoM_Solver.sln
) else (
    echo.
    echo Failed to generate solution
    exit /b 1
)

cd ..
```

### generate_peec_solution.bat

```batch
@echo off
echo Generating PEEC Solver Solution...

if not exist build_peec mkdir build_peec
cd build_peec

cmake -G "Visual Studio 17 2022" -A x64 ..\CMakeLists_PEEC_Solver.txt

if %ERRORLEVEL% EQU 0 (
    echo.
    echo PEEC Solver solution generated successfully!
    echo Location: %CD%\PEEC_Solver.sln
) else (
    echo.
    echo Failed to generate solution
    exit /b 1
)

cd ..
```

## 目录结构

```
PulseMoM/
├── build_mom/              # MoM 解决方案构建目录
│   └── MoM_Solver.sln      # MoM 解决方案文件
├── build_peec/             # PEEC 解决方案构建目录
│   └── PEEC_Solver.sln     # PEEC 解决方案文件
├── build/                  # 原始统一构建目录（可选保留）
├── src/
│   ├── apps/
│   │   ├── mom_cli/
│   │   │   ├── main_mom.c
│   │   │   └── mom_solver_exe.vcxproj
│   │   └── peec_cli/
│   │       ├── main_peec.c
│   │       └── peec_solver_exe.vcxproj
│   └── ...
└── ...
```

## 构建和使用

### 构建 MoM Solver

1. 打开 `build_mom/MoM_Solver.sln`
2. 选择配置（Debug/Release）和平台（x64）
3. 构建解决方案（Ctrl+Shift+B）
4. 可执行文件位于：`build_mom/Release/mom_solver_exe.exe`

### 构建 PEEC Solver

1. 打开 `build_peec/PEEC_Solver.sln`
2. 选择配置（Debug/Release）和平台（x64）
3. 构建解决方案（Ctrl+Shift+B）
4. 可执行文件位于：`build_peec/Release/peec_solver_exe.exe`

## 优势

1. **独立开发**：每个求解器可以独立开发和测试
2. **清晰依赖**：每个解决方案只包含必要的项目
3. **快速构建**：不需要构建不需要的代码
4. **易于部署**：每个解决方案生成独立的 exe

## 注意事项

1. **公共库同步**：如果修改了公共库，需要重新构建两个解决方案
2. **路径配置**：确保两个解决方案使用相同的公共库路径
3. **版本一致性**：确保两个解决方案使用相同版本的公共库

## 故障排除

### 问题：找不到公共库

**解决**：确保公共库项目在解决方案中，或者设置正确的库路径

### 问题：链接错误

**解决**：检查项目依赖关系，确保所有依赖的库都已构建

### 问题：CMake 生成失败

**解决**：
1. 检查 CMakeLists.txt 语法
2. 确保所有依赖库都已定义
3. 检查 CMake 版本（需要 3.16+）
