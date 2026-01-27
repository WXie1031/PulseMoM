# 外部库依赖规则

## Scope
项目所需的外部库及其配置。这些库属于**L4数值后端层**和**L2离散化层**的依赖。

## Architecture Rules

### L4 Layer (Numerical Backend)
- ✅ 使用BLAS/LAPACK进行矩阵运算
- ✅ 使用稀疏求解器进行大规模问题求解
- ❌ 不包含物理假设，只看到矩阵

### L2 Layer (Discretization)
- ✅ 使用CAD库进行几何导入
- ✅ 使用网格生成库
- ❌ 不包含物理假设，只处理几何

## 必需库（Core Dependencies）

### 1. ✅ OpenMP
- **用途**: CPU并行计算
- **vcpkg**: 通过编译器提供，无需vcpkg安装
- **状态**: ✅ 已启用（所有配置）
- **配置**: `<OpenMPSupport>true</OpenMPSupport>`

### 2. ✅ OpenBLAS
- **用途**: 高性能BLAS/LAPACK实现
- **vcpkg包名**: `openblas`
- **安装**: `vcpkg install openblas`
- **状态**: ✅ 已配置，建议迁移到vcpkg
- **代码位置**: `src/backend/math/blas_interface.c`
- **使用**: 通过 `BLAS_BACKEND_OPENBLAS` 选择

## CAD几何处理库

### 3. ✅ OpenCascade (OCCT)
- **用途**: STEP、IGES、STL等CAD格式导入
- **vcpkg包名**: `opencascade`
- **安装**: `vcpkg install opencascade[freeimage,freetype,tbb]`
- **状态**: ⚠️ 当前使用本地安装，建议迁移到vcpkg
- **代码位置**: `src/mesh/opencascade_cad_import.cpp`
- **功能选项**:
  - `freeimage`: FreeImage支持
  - `freetype`: 字体渲染支持
  - `tbb`: Intel TBB并行支持
  - `vtk`: VTK可视化支持

### 4. ✅ CGAL
- **用途**: 计算几何算法、网格生成
- **vcpkg包名**: `cgal`
- **安装**: `vcpkg install cgal`
- **状态**: ⚠️ 当前使用本地安装，建议迁移到vcpkg
- **代码位置**: CMakeLists.txt中已配置查找

### 5. ✅ Gmsh
- **用途**: 网格生成
- **vcpkg包名**: `gmsh`
- **安装**: `vcpkg install gmsh[occ,graphics]`
- **状态**: ⚠️ 当前使用本地安装，建议迁移到vcpkg
- **功能选项**:
  - `occ`: OpenCascade CAD模块支持
  - `graphics`: 图形库支持
  - `mpi`: 分布式并行支持（实验性）

## 数值计算库

### 6. ✅ SuperLU
- **用途**: 稀疏矩阵LU分解
- **vcpkg包名**: `superlu`
- **安装**: `vcpkg install superlu`
- **状态**: ⚠️ 接口已定义，未集成
- **代码位置**: `src/backend/solvers/direct_solver.c`
- **优先级**: 高（大规模稀疏问题）

### 7. ⚠️ MUMPS
- **用途**: 多前沿方法稀疏求解器
- **vcpkg包名**: ❌ **不支持**（vcpkg中没有）
- **替代方案**: 
  - 使用SuperLU（vcpkg支持）
  - 或从源码编译MUMPS
- **状态**: ⚠️ 接口已定义，未实现
- **建议**: 优先使用SuperLU

### 8. ✅ Eigen (可选)
- **用途**: C++线性代数库
- **vcpkg包名**: `eigen3`
- **安装**: `vcpkg install eigen3`
- **状态**: ⚠️ 项目主要使用C，Eigen为可选

## 积分相关库

### 9. ✅ GSL (GNU Scientific Library)
- **用途**: 科学计算库（包含数值积分）
- **vcpkg包名**: `gsl`
- **安装**: `vcpkg install gsl`
- **状态**: ⚠️ 当前未使用
- **功能**: 
  - 数值积分（QUADPACK）
  - 特殊函数
  - 统计函数
- **优先级**: 中（用于高级积分方法）

## 数据存储库

### 10. ✅ HDF5
- **用途**: 大规模数据存储
- **vcpkg包名**: `hdf5`
- **安装**: `vcpkg install hdf5`
- **状态**: ✅ 已配置，建议迁移到vcpkg
- **代码位置**: `src/io/file_formats/file_io.c`
- **使用**: 通过 `OUTPUT_FORMAT_HDF5` 选择

## 其他可选库

### 11. ✅ Boost (可选)
- **用途**: C++工具库
- **vcpkg包名**: `boost-*` (多个包)
- **安装**: `vcpkg install boost-system boost-filesystem boost-thread`
- **状态**: ⚠️ 项目主要使用C，Boost为可选

### 12. ⚠️ CUDA
- **用途**: GPU加速计算
- **vcpkg**: ❌ **不支持**（需要从NVIDIA官网安装）
- **状态**: ⚠️ 接口已定义，部分功能未实现
- **代码位置**: `src/backend/gpu/`

### 13. ✅ PETSc (可选)
- **用途**: 科学计算库（大规模并行）
- **vcpkg包名**: `petsc`
- **安装**: `vcpkg install petsc`
- **状态**: ⚠️ CMakeLists.txt中已配置，当前未使用
- **优先级**: 低（大规模并行场景）

## vcpkg安装命令

### 最小安装（开发/测试）
```bash
vcpkg install openblas hdf5 gmsh cgal opencascade
```

### 推荐安装（生产环境）
```bash
vcpkg install openblas \
              hdf5 \
              gmsh[occ,graphics] \
              cgal \
              opencascade[freeimage,freetype,tbb] \
              superlu \
              gsl
```

### 完整安装（高性能环境）
```bash
vcpkg install openblas \
              hdf5 \
              gmsh[occ,graphics,mpi] \
              cgal[qt] \
              opencascade[freeimage,freetype,tbb,vtk] \
              superlu \
              gsl \
              eigen3 \
              petsc
```

## 库依赖优先级

### 高优先级（立即集成）
1. **SuperLU** - 稀疏矩阵求解器，显著提升大规模问题性能
2. **GSL** - 高级数值积分方法

### 中优先级（近期集成）
3. **迁移到vcpkg** - OpenBLAS, HDF5, Gmsh, CGAL, OpenCascade

### 低优先级（可选）
4. **Eigen** - 如果增加C++代码
5. **PETSc** - 大规模并行场景

## CMake配置

### 使用vcpkg库
```cmake
# vcpkg工具链已在CMakeLists.txt中配置
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# 查找库
find_package(OpenBLAS REQUIRED)
find_package(HDF5 REQUIRED)
find_package(Gmsh REQUIRED)
find_package(CGAL REQUIRED)
find_package(OpenCascade REQUIRED)
find_package(SuperLU REQUIRED)
find_package(GSL REQUIRED)
```

### Visual Studio项目配置
- 使用vcpkg的include路径: `${VCPKG_ROOT}/installed/x64-windows/include`
- 使用vcpkg的lib路径: `${VCPKG_ROOT}/installed/x64-windows/lib`
- 或通过CMake生成项目文件

## 代码集成

### BLAS/LAPACK集成
- **位置**: `src/backend/math/blas_interface.c`
- **状态**: ✅ 接口已定义
- **待完成**: 完整实现OpenBLAS调用

### 稀疏求解器集成
- **位置**: `src/backend/solvers/direct_solver.c`
- **状态**: ⚠️ 接口已定义，未实现
- **待完成**: 集成SuperLU

### HDF5集成
- **位置**: `src/io/file_formats/file_io.c`
- **状态**: ✅ 基本框架已实现
- **待完成**: 完整实现复数类型处理

### GSL积分集成
- **位置**: `src/operators/integration/singular_integration.c`
- **状态**: ⚠️ 当前使用自定义实现
- **待完成**: 可选，使用GSL的QUADPACK

## 核心约束

- **层间分离**: L4使用数值库，L2使用几何库
- **可选依赖**: 所有库都应该是可选的，有fallback实现
- **条件编译**: 使用 `#ifdef` 检查库是否可用
- **统一管理**: 优先使用vcpkg统一管理依赖

## 文件位置

- **CMake配置**: `CMakeLists.txt`
- **Visual Studio配置**: `src/PulseMoM_Core.vcxproj`
- **BLAS接口**: `src/backend/math/blas_interface.c`
- **直接求解器**: `src/backend/solvers/direct_solver.c`
- **文件IO**: `src/io/file_formats/file_io.c`
- **CAD导入**: `src/mesh/opencascade_cad_import.cpp`

## 使用示例

### 检查库是否可用
```c
#ifdef ENABLE_OPENBLAS
    // 使用OpenBLAS
    blas_interface_init(BLAS_BACKEND_OPENBLAS);
#else
    // 使用原生实现
    blas_interface_init(BLAS_BACKEND_NATIVE);
#endif
```

### 使用SuperLU（待实现）
```c
#ifdef ENABLE_SUPERLU
    // 使用SuperLU进行稀疏LU分解
    // TODO: 实现SuperLU集成
#else
    // 使用密集矩阵转换
    // 当前实现
#endif
```

## 相关文档

- `docs/VCPKG_LIBRARY_ANALYSIS.md` - vcpkg库支持分析
- `docs/DEPENDENCIES_AND_OPTIMIZATION_ANALYSIS.md` - 依赖分析
- `docs/LIBRARY_CONFIGURATION_SUMMARY.md` - 库配置总结
