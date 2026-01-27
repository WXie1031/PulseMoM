# vcpkg库支持分析

## 完成时间
2025-01-XX

## 概述

本文档分析项目所需的所有外部库，并检查它们在vcpkg中的支持情况。包括：
1. CAD几何处理库
2. 数值计算库
3. 积分相关库
4. 其他支持库

---

## 一、CAD几何处理库

### 1. ✅ OpenCascade (OCCT)
- **用途**: STEP、IGES、STL等CAD格式导入
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `opencascade`
- **最新版本**: 7.9.3
- **安装命令**:
  ```bash
  vcpkg install opencascade
  # 或带功能选项
  vcpkg install opencascade[freeimage,freetype,tbb]
  ```
- **功能选项**:
  - `freeimage`: FreeImage支持
  - `freetype`: 字体渲染支持
  - `rapidjson`: JSON解析支持
  - `samples`: 示例应用
  - `tbb`: Intel TBB并行支持
  - `vtk`: VTK可视化支持
- **项目状态**: 
  - ✅ 代码中已使用（`src/mesh/opencascade_cad_import.cpp`）
  - ⚠️ 当前使用本地安装（`libs/occt-vc14-64`）
  - ✅ 建议迁移到vcpkg

### 2. ✅ CGAL (Computational Geometry Algorithms Library)
- **用途**: 计算几何算法、网格生成
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `cgal`
- **最新版本**: v6.0.1#0 (2024-10-31)
- **安装命令**:
  ```bash
  vcpkg install cgal
  # 或带Qt支持
  vcpkg install cgal[qt]
  ```
- **功能选项**:
  - `qt`: Qt GUI/可视化组件
- **项目状态**: 
  - ✅ CMakeLists.txt中已配置查找CGAL
  - ⚠️ 当前使用本地安装（`libs/CGAL-6.1`）
  - ✅ 建议迁移到vcpkg

### 3. ✅ Gmsh
- **用途**: 网格生成
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `gmsh`
- **最新版本**: v4.13.1#0 (2024-11-11)
- **安装命令**:
  ```bash
  vcpkg install gmsh
  # 或带功能选项
  vcpkg install gmsh[occ,graphics]
  ```
- **功能选项**:
  - `graphics`: 图形库支持
  - `mpi`: 分布式并行支持（实验性）
  - `occ`: OpenCascade CAD模块支持
  - `zipper`: 压缩/解压支持
- **项目状态**: 
  - ✅ 代码中已使用（`libs/gmsh-4.15.0-Windows64-sdk`）
  - ⚠️ 当前使用本地安装
  - ✅ 建议迁移到vcpkg

### 4. ⚠️ GDSII/OASIS/GERBER解析器
- **用途**: 半导体/PCB布局文件解析
- **vcpkg支持**: ❌ **不支持**（需要查找替代方案）
- **替代方案**:
  - **libgdsii**: 可能需要从源码编译
  - **自定义解析器**: 项目已实现基本GERBER解析（`src/io/pcb_file_io.c`）
- **项目状态**: 
  - ⚠️ 接口已定义，实现待完成
  - ⚠️ 可能需要自定义实现或查找其他库

---

## 二、数值计算库

### 1. ✅ OpenBLAS
- **用途**: 高性能BLAS/LAPACK实现
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `openblas`
- **安装命令**:
  ```bash
  vcpkg install openblas
  ```
- **项目状态**: 
  - ✅ 已配置（`libs/OpenBLAS-0.3.30-x64`）
  - ✅ 代码中已集成（`src/backend/math/blas_interface.c`）
  - ✅ 建议迁移到vcpkg

### 2. ⚠️ Intel MKL
- **用途**: 高性能BLAS/LAPACK（商业）
- **vcpkg支持**: ❌ **不支持**（商业库）
- **替代方案**: 
  - 使用OpenBLAS（开源替代）
  - 或从Intel官网下载安装
- **项目状态**: 
  - ⚠️ 接口已定义，但未使用
  - ✅ 建议使用OpenBLAS作为开源替代

### 3. ✅ SuperLU
- **用途**: 稀疏矩阵LU分解
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `superlu`
- **安装命令**:
  ```bash
  vcpkg install superlu
  ```
- **项目状态**: 
  - ⚠️ 接口已定义（`src/backend/solvers/direct_solver.c`）
  - ⚠️ 当前使用密集矩阵转换（较慢）
  - ✅ 建议集成SuperLU

### 4. ✅ MUMPS
- **用途**: 多前沿方法稀疏求解器
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `mumps`
- **安装命令**:
  ```bash
  vcpkg install mumps
  ```
- **项目状态**: 
  - ⚠️ 接口已定义，但未实现
  - ✅ 建议集成MUMPS

### 5. ⚠️ PARDISO
- **用途**: Intel MKL中的并行直接求解器
- **vcpkg支持**: ❌ **不支持**（包含在MKL中）
- **替代方案**: 
  - 使用SuperLU或MUMPS（开源替代）
  - 或安装Intel MKL
- **项目状态**: 
  - ⚠️ 接口已定义，但未使用

### 6. ✅ Eigen
- **用途**: C++线性代数库（可选，用于C++代码）
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `eigen3`
- **安装命令**:
  ```bash
  vcpkg install eigen3
  ```
- **项目状态**: 
  - ⚠️ 当前项目主要使用C，Eigen为可选

---

## 三、积分相关库

### 1. ⚠️ 数值积分库
- **用途**: 高级数值积分方法
- **vcpkg支持**: 需要查找
- **可能的选项**:
  - **GSL (GNU Scientific Library)**: ✅ vcpkg支持 (`gsl`)
  - **QUADPACK**: 可能需要从源码编译
  - **自定义实现**: 项目已实现基本积分方法
- **项目状态**: 
  - ✅ 项目已实现基本积分方法（`src/operators/integration/`）
  - ⚠️ 高级积分方法可能需要GSL

### 2. ✅ GSL (GNU Scientific Library)
- **用途**: 科学计算库（包含数值积分）
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `gsl`
- **安装命令**:
  ```bash
  vcpkg install gsl
  ```
- **功能**: 
  - 数值积分（QUADPACK）
  - 特殊函数
  - 统计函数
  - 线性代数
- **项目状态**: 
  - ⚠️ 当前未使用
  - ✅ 可选，用于高级积分方法

---

## 四、其他支持库

### 1. ✅ HDF5
- **用途**: 大规模数据存储
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `hdf5`
- **安装命令**:
  ```bash
  vcpkg install hdf5
  ```
- **项目状态**: 
  - ✅ 已配置（`C:\Program Files\HDF_Group\HDF5\1.14.6`）
  - ✅ 代码中已集成（`src/io/file_formats/file_io.c`）
  - ✅ 建议迁移到vcpkg

### 2. ✅ Boost
- **用途**: C++工具库（可选）
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `boost-*` (多个包)
- **安装命令**:
  ```bash
  vcpkg install boost-system boost-filesystem boost-thread
  ```
- **项目状态**: 
  - ⚠️ 当前使用本地安装（`libs/boost_1_89_0`）
  - ⚠️ 项目主要使用C，Boost为可选

### 3. ✅ OpenMP
- **用途**: CPU并行计算
- **vcpkg支持**: ✅ **完全支持**（通过编译器）
- **说明**: OpenMP通常包含在编译器中，无需单独安装
- **项目状态**: 
  - ✅ 已启用（所有配置）

### 4. ⚠️ CUDA
- **用途**: GPU加速计算
- **vcpkg支持**: ❌ **不支持**（需要从NVIDIA官网安装）
- **安装**: 从NVIDIA官网下载CUDA Toolkit
- **项目状态**: 
  - ⚠️ 接口已定义（`src/backend/gpu/`）
  - ⚠️ 部分功能未实现

### 5. ✅ PETSc (可选)
- **用途**: 科学计算库（大规模并行）
- **vcpkg支持**: ✅ **完全支持**
- **包名**: `petsc`
- **安装命令**:
  ```bash
  vcpkg install petsc
  ```
- **项目状态**: 
  - ⚠️ CMakeLists.txt中已配置查找（`libs/petsc-3.24.1`）
  - ⚠️ 当前未使用

---

## 五、vcpkg安装建议

### 5.1 最小安装（开发/测试）

```bash
# CAD库
vcpkg install opencascade
vcpkg install cgal
vcpkg install gmsh

# 数值计算库
vcpkg install openblas

# 数据存储
vcpkg install hdf5
```

### 5.2 推荐安装（生产环境）

```bash
# CAD库（带功能选项）
vcpkg install opencascade[freeimage,freetype,tbb]
vcpkg install cgal
vcpkg install gmsh[occ,graphics]

# 数值计算库
vcpkg install openblas
vcpkg install superlu
vcpkg install mumps

# 数据存储
vcpkg install hdf5

# 可选：科学计算库
vcpkg install gsl
```

### 5.3 完整安装（高性能环境）

```bash
# CAD库（完整功能）
vcpkg install opencascade[freeimage,freetype,tbb,vtk]
vcpkg install cgal[qt]
vcpkg install gmsh[occ,graphics,mpi]

# 数值计算库
vcpkg install openblas
vcpkg install superlu
vcpkg install mumps
vcpkg install eigen3

# 数据存储
vcpkg install hdf5

# 科学计算库
vcpkg install gsl

# 可选：大规模并行
vcpkg install petsc
```

---

## 六、迁移到vcpkg的步骤

### 6.1 当前状态

项目当前使用本地安装的库：
- `libs/OpenBLAS-0.3.30-x64` → 迁移到 `vcpkg install openblas`
- `libs/gmsh-4.15.0-Windows64-sdk` → 迁移到 `vcpkg install gmsh`
- `libs/CGAL-6.1` → 迁移到 `vcpkg install cgal`
- `libs/occt-vc14-64` → 迁移到 `vcpkg install opencascade`
- `C:\Program Files\HDF_Group\HDF5\1.14.6` → 迁移到 `vcpkg install hdf5`

### 6.2 迁移步骤

1. **安装vcpkg**（如果未安装）:
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat  # Windows
   ```

2. **安装所需库**:
   ```bash
   vcpkg install opencascade openblas gmsh cgal hdf5 superlu mumps
   ```

3. **更新CMakeLists.txt**:
   ```cmake
   # 使用vcpkg工具链
   set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
   
   # 使用find_package查找库
   find_package(OpenCascade REQUIRED)
   find_package(OpenBLAS REQUIRED)
   find_package(Gmsh REQUIRED)
   find_package(CGAL REQUIRED)
   find_package(HDF5 REQUIRED)
   find_package(SuperLU REQUIRED)
   find_package(MUMPS REQUIRED)
   ```

4. **更新Visual Studio项目配置**:
   - 移除本地库路径
   - 使用vcpkg提供的路径

---

## 七、vcpkg支持总结表

| 库 | 用途 | vcpkg支持 | 包名 | 当前状态 | 建议 |
|----|------|-----------|------|----------|------|
| **CAD库** |
| OpenCascade | STEP/IGES导入 | ✅ | `opencascade` | 本地安装 | 迁移到vcpkg |
| CGAL | 计算几何 | ✅ | `cgal` | 本地安装 | 迁移到vcpkg |
| Gmsh | 网格生成 | ✅ | `gmsh` | 本地安装 | 迁移到vcpkg |
| GDSII解析器 | 半导体布局 | ❌ | - | 未实现 | 自定义实现 |
| **数值计算库** |
| OpenBLAS | BLAS/LAPACK | ✅ | `openblas` | 本地安装 | 迁移到vcpkg |
| Intel MKL | BLAS/LAPACK | ❌ | - | 未使用 | 使用OpenBLAS |
| SuperLU | 稀疏LU | ✅ | `superlu` | 未集成 | 集成 |
| MUMPS | 稀疏求解器 | ✅ | `mumps` | 未集成 | 集成 |
| PARDISO | 并行求解器 | ❌ | - | 未使用 | 使用SuperLU |
| Eigen | 线性代数 | ✅ | `eigen3` | 未使用 | 可选 |
| **积分库** |
| GSL | 科学计算 | ✅ | `gsl` | 未使用 | 可选 |
| **其他库** |
| HDF5 | 数据存储 | ✅ | `hdf5` | 系统安装 | 迁移到vcpkg |
| Boost | C++工具库 | ✅ | `boost-*` | 本地安装 | 可选 |
| OpenMP | 并行计算 | ✅ | 编译器 | 已启用 | 保持 |
| CUDA | GPU加速 | ❌ | - | 部分实现 | 从NVIDIA安装 |
| PETSc | 大规模并行 | ✅ | `petsc` | 未使用 | 可选 |

---

## 八、推荐行动

### 立即行动（高优先级）

1. **迁移到vcpkg**:
   - ✅ OpenBLAS → `vcpkg install openblas`
   - ✅ HDF5 → `vcpkg install hdf5`
   - ✅ Gmsh → `vcpkg install gmsh`

2. **集成稀疏求解器**:
   - ✅ SuperLU → `vcpkg install superlu`
   - ✅ MUMPS → `vcpkg install mumps`

### 近期行动（中优先级）

3. **迁移CAD库**:
   - ✅ OpenCascade → `vcpkg install opencascade`
   - ✅ CGAL → `vcpkg install cgal`

4. **可选库**:
   - ⚠️ GSL → `vcpkg install gsl`（用于高级积分）

### 未来考虑（低优先级）

5. **大规模并行**:
   - ⚠️ PETSc → `vcpkg install petsc`（如果需要）

---

## 九、vcpkg安装示例脚本

### Windows PowerShell脚本

```powershell
# 设置vcpkg路径
$VCPKG_ROOT = "D:\Project\vcpkg"

# 安装CAD库
vcpkg install opencascade[freeimage,freetype,tbb]
vcpkg install cgal
vcpkg install gmsh[occ,graphics]

# 安装数值计算库
vcpkg install openblas
vcpkg install superlu
vcpkg install mumps

# 安装数据存储
vcpkg install hdf5

# 可选：科学计算库
vcpkg install gsl
```

### Linux/Mac脚本

```bash
#!/bin/bash
# 设置vcpkg路径
export VCPKG_ROOT="$HOME/vcpkg"

# 安装CAD库
vcpkg install opencascade[freeimage,freetype,tbb]
vcpkg install cgal
vcpkg install gmsh[occ,graphics]

# 安装数值计算库
vcpkg install openblas
vcpkg install superlu
vcpkg install mumps

# 安装数据存储
vcpkg install hdf5

# 可选：科学计算库
vcpkg install gsl
```

---

## 十、总结

### vcpkg支持情况

- ✅ **CAD库**: 完全支持（OpenCascade, CGAL, Gmsh）
- ✅ **数值计算库**: 大部分支持（OpenBLAS, SuperLU, MUMPS）
- ✅ **数据存储**: 完全支持（HDF5）
- ⚠️ **商业库**: 不支持（Intel MKL, PARDISO）
- ⚠️ **GPU库**: 不支持（CUDA需要单独安装）
- ⚠️ **特殊格式**: 部分不支持（GDSII/OASIS需要自定义实现）

### 迁移建议

1. **优先迁移**: OpenBLAS, HDF5, Gmsh（已在使用）
2. **集成新库**: SuperLU, MUMPS（提升性能）
3. **迁移CAD库**: OpenCascade, CGAL（统一管理）
4. **保持现状**: CUDA（需要从NVIDIA安装）

### 优势

- ✅ 统一管理所有依赖
- ✅ 自动处理依赖关系
- ✅ 跨平台支持
- ✅ 版本管理
- ✅ 简化编译配置

---

## 相关文档

- `docs/DEPENDENCIES_AND_OPTIMIZATION_ANALYSIS.md` - 依赖分析
- `docs/LIBRARY_CONFIGURATION_SUMMARY.md` - 库配置总结
- `CMakeLists.txt` - CMake配置
- `.claude/skills/discretization/cad_import_rules.md` - CAD导入规则
