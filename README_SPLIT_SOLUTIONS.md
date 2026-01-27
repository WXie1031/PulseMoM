# MoM 和 PEEC 独立解决方案使用指南

## 快速开始

### 生成 MoM 解决方案

```powershell
# 方法 1: 使用脚本（推荐）
.\scripts\generate_mom_solution.bat

# 方法 2: 手动生成
mkdir build_mom
cd build_mom
cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_MOM_EXE=ON -DBUILD_PEEC_EXE=OFF ..
```

生成的解决方案文件：`build_mom\PEEC_MoM_Unified_Framework.sln`

### 生成 PEEC 解决方案

```powershell
# 方法 1: 使用脚本（推荐）
.\scripts\generate_peec_solution.bat

# 方法 2: 手动生成
mkdir build_peec
cd build_peec
cmake -G "Visual Studio 17 2022" -A x64 -DBUILD_MOM_EXE=OFF -DBUILD_PEEC_EXE=ON ..
```

生成的解决方案文件：`build_peec\PEEC_MoM_Unified_Framework.sln`

### 生成两个解决方案

```powershell
.\scripts\generate_both_solutions.bat
```

## 在 Visual Studio 中使用

### MoM Solver 解决方案

1. 打开 `build_mom\PEEC_MoM_Unified_Framework.sln`
2. 在解决方案资源管理器中，您会看到：
   - 公共库项目（core_base_library, electromagnetic_kernel_library, 等）
   - `mom_solver` 项目
   - `mom_solver_exe` 项目（可执行文件）
3. 设置 `mom_solver_exe` 为启动项目（右键 → 设为启动项目）
4. 构建解决方案（Ctrl+Shift+B）
5. 运行（F5）

### PEEC Solver 解决方案

1. 打开 `build_peec\PEEC_MoM_Unified_Framework.sln`
2. 在解决方案资源管理器中，您会看到：
   - 公共库项目
   - `peec_solver` 项目
   - `peec_solver_exe` 项目（可执行文件）
3. 设置 `peec_solver_exe` 为启动项目
4. 构建解决方案
5. 运行

## 项目结构

### MoM Solver 解决方案包含：

- **公共库**（两个解决方案共享）：
  - `core_base_library` - 基础库
  - `geometry_processing_library` - 几何处理
  - `mesh_generation_library` - 网格生成
  - `electromagnetic_kernel_library` - 电磁核函数

- **MoM 特定**：
  - `mom_solver` - MoM 求解器库
  - `mom_solver_exe` - MoM 可执行文件

### PEEC Solver 解决方案包含：

- **公共库**（与 MoM 相同）

- **PEEC 特定**：
  - `peec_solver` - PEEC 求解器库
  - `peec_solver_exe` - PEEC 可执行文件

## 可执行文件位置

构建后，可执行文件位于：

- **MoM**: `build_mom\Release\mom_solver_exe.exe` 或 `build_mom\Debug\mom_solver_exe.exe`
- **PEEC**: `build_peec\Release\peec_solver_exe.exe` 或 `build_peec\Debug\peec_solver_exe.exe`

## 使用可执行文件

### MoM Solver

```powershell
# 创建配置文件 config.txt
echo geometry_file=tests/patch_antenna_2x2.stl > config.txt
echo frequency=10e9 >> config.txt
echo tolerance=1e-6 >> config.txt

# 运行
.\build_mom\Release\mom_solver_exe.exe config.txt
```

### PEEC Solver

```powershell
# 创建配置文件 config.txt
echo geometry_file=tests/pcb_cst_exp2.stp > config.txt
echo frequency=1e9 >> config.txt
echo tolerance=1e-6 >> config.txt

# 运行
.\build_peec\Release\peec_solver_exe.exe config.txt
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

1. **独立开发**：每个求解器可以独立开发和测试
2. **清晰依赖**：每个解决方案只包含必要的项目
3. **快速构建**：不需要构建不需要的代码
4. **易于部署**：每个解决方案生成独立的 exe

## 注意事项

1. **公共库同步**：如果修改了公共库，需要重新构建两个解决方案
2. **路径配置**：确保两个解决方案使用相同的公共库路径
3. **版本一致性**：确保两个解决方案使用相同版本的公共库

## 故障排除

### 问题：CMake 找不到源文件

**解决**：确保在项目根目录运行脚本，或检查路径设置

### 问题：链接错误

**解决**：
1. 确保所有公共库都已构建
2. 检查项目依赖关系
3. 清理并重新生成解决方案

### 问题：找不到可执行文件

**解决**：
1. 检查构建配置（Debug/Release）
2. 检查输出目录设置
3. 确保构建成功完成
