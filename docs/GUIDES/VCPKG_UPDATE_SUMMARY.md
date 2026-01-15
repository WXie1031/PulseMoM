# vcpkg 路径更新总结

## 更新日期
2025-01-XX

## vcpkg 新位置
`D:\Project\vcpkg-master`

## 已更新的文件

### 1. CMakeLists.txt
- ✅ 添加了 vcpkg 自动检测和工具链文件配置
- ✅ 设置默认 vcpkg 路径为 `D:\Project\vcpkg-master`
- ✅ 如果 vcpkg 不存在，会显示警告但不会阻止构建

**关键更改**：
```cmake
set(VCPKG_ROOT "D:/Project/vcpkg-master" CACHE PATH "vcpkg root directory")
if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()
```

### 2. build_windows.bat
- ✅ 添加了 vcpkg 工具链文件检测
- ✅ 如果 vcpkg 存在，自动使用工具链文件
- ✅ 如果不存在，回退到不使用 vcpkg 的构建方式

### 3. build_optimized.bat
- ✅ 添加了 vcpkg 工具链文件检测
- ✅ 在 CMake 配置参数中包含 vcpkg 工具链文件
- ✅ 添加了警告信息

### 4. build_gpu_enhanced.bat
- ✅ 添加了 vcpkg 工具链文件检测
- ✅ 在 CMake 配置参数中包含 vcpkg 工具链文件

### 5. vcpkg.json（新建）
- ✅ 创建了 vcpkg 清单文件
- ✅ 定义了所有项目依赖
- ✅ 指定了版本要求

**依赖列表**：
- boost-geometry (>=1.89.0)
- openblas (>=0.3.30)
- cgal (>=6.1.0)
- eigen3 (>=3.4.0)
- gmsh (>=4.15.0)
- opencascade (>=7.8.0)
- clipper2 (>=1.5.0)
- triangle (>=1.6.0)
- openmp (Windows)
- cuda (>=12.0, Windows x64)
- opencl (>=3.0)
- petsc (>=3.24.0)

### 6. setup_vcpkg.bat（新建）
- ✅ 创建了 vcpkg 设置脚本
- ✅ 自动检测 vcpkg 安装
- ✅ 设置环境变量
- ✅ 提供使用说明

### 7. VCPKG_SETUP.md（新建）
- ✅ 创建了详细的 vcpkg 配置文档
- ✅ 包含安装说明
- ✅ 包含故障排除指南

## 使用方法

### 快速开始

1. **运行设置脚本**（可选）：
   ```batch
   setup_vcpkg.bat
   ```

2. **安装依赖**：
   ```batch
   cd D:\Project\vcpkg-master
   .\vcpkg.exe install --triplet x64-windows
   ```

3. **构建项目**：
   ```batch
   build_windows.bat
   ```
   或
   ```batch
   build_optimized.bat
   ```

### 自动检测

项目现在会自动检测 vcpkg：
- 如果 vcpkg 存在于 `D:\Project\vcpkg-master`，会自动使用
- 如果不存在，会显示警告但继续构建（使用本地 libs 目录中的库）

## 库依赖分析

### 通过 vcpkg 可安装的库

以下库可以通过 vcpkg 安装：

1. **boost-geometry** - 几何计算
2. **openblas** - 线性代数
3. **cgal** - 计算几何
4. **eigen3** - 矩阵运算
5. **gmsh** - 网格生成
6. **opencascade** - CAD 导入
7. **clipper2** - 2D 多边形操作
8. **triangle** - 三角剖分
9. **openmp** - 并行计算
10. **cuda** - GPU 加速
11. **opencl** - GPU 计算
12. **petsc** - 科学计算

### 本地库（优先使用）

项目会优先使用 `libs` 目录中的库：
- `libs/CGAL-6.1`
- `libs/gmsh-4.15.0-Windows64-sdk`
- `libs/occt-vc14-64`
- `libs/boost_1_89_0`
- `libs/OpenBLAS-0.3.30-x64`
- `libs/petsc-3.24.1`

如果本地库不存在，CMake 会尝试通过 vcpkg 查找。

## 验证更新

### 检查 vcpkg 路径

```batch
dir D:\Project\vcpkg-master\scripts\buildsystems\vcpkg.cmake
```

应该看到文件存在。

### 检查 CMake 配置

运行构建脚本，应该看到：
```
Using vcpkg toolchain: D:\Project\vcpkg-master\scripts\buildsystems\vcpkg.cmake
```

### 检查已安装的包

```batch
D:\Project\vcpkg-master\vcpkg.exe list
```

## 故障排除

### 问题：CMake 找不到 vcpkg

**原因**：vcpkg 路径不正确或工具链文件不存在

**解决方案**：
1. 确认 vcpkg 位于 `D:\Project\vcpkg-master`
2. 检查 `D:\Project\vcpkg-master\scripts\buildsystems\vcpkg.cmake` 是否存在
3. 如果路径不同，修改 `CMakeLists.txt` 中的 `VCPKG_ROOT` 变量

### 问题：依赖包未找到

**原因**：vcpkg 中未安装所需的包

**解决方案**：
```batch
cd D:\Project\vcpkg-master
.\vcpkg.exe install --triplet x64-windows
```

### 问题：构建时使用错误的库版本

**原因**：本地库和 vcpkg 库版本冲突

**解决方案**：
1. 项目优先使用本地 `libs` 目录中的库
2. 如果需要使用 vcpkg 库，可以临时重命名或移动本地库目录

## 下一步

1. ✅ vcpkg 路径已更新到 `D:\Project\vcpkg-master`
2. ✅ 所有构建脚本已更新
3. ✅ vcpkg.json 清单文件已创建
4. ✅ 文档已更新

**建议操作**：
1. 运行 `setup_vcpkg.bat` 设置环境变量
2. 安装 vcpkg 依赖：`D:\Project\vcpkg-master\vcpkg.exe install --triplet x64-windows`
3. 测试构建：`build_windows.bat`

## 相关文件

- `CMakeLists.txt` - CMake 配置文件（已更新）
- `build_windows.bat` - Windows 构建脚本（已更新）
- `build_optimized.bat` - 优化构建脚本（已更新）
- `build_gpu_enhanced.bat` - GPU 增强构建脚本（已更新）
- `vcpkg.json` - vcpkg 清单文件（新建）
- `setup_vcpkg.bat` - vcpkg 设置脚本（新建）
- `VCPKG_SETUP.md` - vcpkg 配置文档（新建）

