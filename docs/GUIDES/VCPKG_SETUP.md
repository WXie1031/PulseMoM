# vcpkg 配置说明

## vcpkg 位置

vcpkg 已移动到：`D:\Project\vcpkg-master`

## 自动配置

项目已配置为自动使用 vcpkg。CMakeLists.txt 会自动检测并使用 vcpkg 工具链文件。

## 手动设置

### 方法 1: 使用环境变量

运行 `setup_vcpkg.bat` 脚本来自动设置环境变量：

```batch
setup_vcpkg.bat
```

### 方法 2: 手动设置环境变量

在 PowerShell 中：

```powershell
$env:VCPKG_ROOT = "D:\Project\vcpkg-master"
$env:CMAKE_TOOLCHAIN_FILE = "$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
```

在 CMD 中：

```batch
set VCPKG_ROOT=D:\Project\vcpkg-master
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
```

永久设置（系统环境变量）：

1. 打开"系统属性" -> "高级" -> "环境变量"
2. 添加：
   - `VCPKG_ROOT` = `D:\Project\vcpkg-master`
   - `CMAKE_TOOLCHAIN_FILE` = `D:\Project\vcpkg-master\scripts\buildsystems\vcpkg.cmake`

## 安装依赖

使用 vcpkg 安装项目依赖：

```batch
cd D:\Project\vcpkg-master
.\vcpkg.exe install --triplet x64-windows
```

或者使用清单模式（推荐）：

```batch
cd D:\Project\MoM\PulseMoM
D:\Project\vcpkg-master\vcpkg.exe install --triplet x64-windows
```

## 项目依赖列表

项目通过 `vcpkg.json` 定义以下依赖：

- **boost-geometry** (>=1.89.0) - 几何计算库
- **openblas** (>=0.3.30) - 线性代数库
- **cgal** (>=6.1.0) - 计算几何算法库
- **eigen3** (>=3.4.0) - 矩阵运算库
- **gmsh** (>=4.15.0) - 网格生成库
- **opencascade** (>=7.8.0) - CAD 导入库
- **clipper2** (>=1.5.0) - 2D 多边形操作库
- **triangle** (>=1.6.0) - 三角剖分库
- **openmp** (Windows) - 并行计算
- **cuda** (>=12.0, Windows x64) - GPU 加速
- **opencl** (>=3.0) - GPU 计算
- **petsc** (>=3.24.0) - 科学计算库

## 构建项目

### 使用构建脚本（推荐）

```batch
build_windows.bat
```

或

```batch
build_optimized.bat
```

这些脚本会自动检测并使用 vcpkg。

### 手动构建

```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="D:\Project\vcpkg-master\scripts\buildsystems\vcpkg.cmake" ..
cmake --build . --config Release
```

## 验证 vcpkg 配置

运行以下命令验证 vcpkg 是否正确配置：

```batch
D:\Project\vcpkg-master\vcpkg.exe list
```

应该看到已安装的包列表。

## 故障排除

### 问题 1: CMake 找不到 vcpkg

**解决方案**：
1. 确认 vcpkg 路径正确：`D:\Project\vcpkg-master`
2. 检查工具链文件是否存在：`D:\Project\vcpkg-master\scripts\buildsystems\vcpkg.cmake`
3. 在 CMakeLists.txt 中，vcpkg 路径会自动检测，如果失败会显示警告

### 问题 2: 依赖包未找到

**解决方案**：
1. 确保已运行 `vcpkg install` 安装依赖
2. 检查 `vcpkg.json` 中的包名是否正确
3. 运行 `vcpkg list` 查看已安装的包

### 问题 3: 构建时找不到库

**解决方案**：
1. 确保使用正确的 triplet（x64-windows）
2. 检查 CMakeLists.txt 中的 `find_package` 调用
3. 项目会优先使用本地 libs 目录中的库，如果找不到才会使用 vcpkg

## 本地库 vs vcpkg 库

项目配置为优先使用本地 `libs` 目录中的库：
- `libs/CGAL-6.1`
- `libs/gmsh-4.15.0-Windows64-sdk`
- `libs/occt-vc14-64`
- `libs/boost_1_89_0`
- `libs/OpenBLAS-0.3.30-x64`
- `libs/petsc-3.24.1`

如果本地库不存在，CMake 会尝试通过 vcpkg 查找。

## 更新 vcpkg

```batch
cd D:\Project\vcpkg-master
git pull
.\bootstrap-vcpkg.bat
```

## 更多信息

- vcpkg 文档: https://vcpkg.io/
- vcpkg GitHub: https://github.com/microsoft/vcpkg

