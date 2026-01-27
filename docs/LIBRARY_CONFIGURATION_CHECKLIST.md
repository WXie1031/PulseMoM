# 库配置检查清单

## 配置完成状态

### ✅ 1. OpenBLAS配置
- [x] 库路径已添加到vcxproj: `$(SolutionDir)libs\OpenBLAS-0.3.30-x64\lib`
- [x] Include路径已添加: `$(SolutionDir)libs\OpenBLAS-0.3.30-x64\include`
- [x] 库依赖已添加: `libopenblas.lib`
- [x] 预处理器定义已添加: `ENABLE_OPENBLAS`
- [x] 代码头文件包含: `#include <cblas.h>`, `#include <lapacke.h>`
- [x] 初始化代码已添加: `openblas_set_num_threads()`
- [ ] 完整BLAS函数调用实现（需要类型转换）

### ✅ 2. HDF5配置
- [x] 库路径已添加到vcxproj: `C:\Program Files\HDF_Group\HDF5\1.14.6\lib`
- [x] Include路径已添加: `C:\Program Files\HDF_Group\HDF5\1.14.6\include`
- [x] 库依赖已添加: `hdf5.lib`, `hdf5_hl.lib`
- [x] 预处理器定义已添加: `ENABLE_HDF5`
- [x] 代码头文件包含: `#include <hdf5.h>`
- [x] 基本HDF5实现框架已添加
- [ ] 完整复数类型处理（需要compound datatype）

### ✅ 3. OpenMP配置
- [x] Debug|Win32: OpenMP支持已启用
- [x] Release|Win32: OpenMP支持已启用
- [x] Debug|x64: OpenMP支持已启用
- [x] Release|x64: OpenMP支持已启用
- [x] 代码中已使用 `#ifdef _OPENMP` 条件编译

---

## 验证步骤

### 步骤1: 编译验证
1. 打开Visual Studio
2. 加载项目 `src/PulseMoM_Core.vcxproj`
3. 选择配置（推荐: Release|x64）
4. 编译项目
5. 检查是否有链接错误

**预期结果**: 
- ✅ 应该能够找到所有头文件
- ✅ 应该能够链接所有库
- ⚠️ 如果有链接错误，检查库路径是否正确

### 步骤2: 运行时验证
1. 运行编译后的程序
2. 检查OpenBLAS是否初始化
3. 检查HDF5是否可用
4. 检查OpenMP线程数

**验证代码**:
```c
#ifdef ENABLE_OPENBLAS
    printf("OpenBLAS: Enabled\n");
    blas_interface_init(BLAS_BACKEND_OPENBLAS);
    printf("OpenBLAS threads: %d\n", openblas_get_num_threads());
#else
    printf("OpenBLAS: Not enabled\n");
#endif

#ifdef ENABLE_HDF5
    printf("HDF5: Enabled\n");
#else
    printf("HDF5: Not enabled\n");
#endif

#ifdef _OPENMP
    printf("OpenMP: Enabled, max threads: %d\n", omp_get_max_threads());
#else
    printf("OpenMP: Not enabled\n");
#endif
```

---

## 常见问题

### Q1: 编译时找不到头文件
**A**: 检查vcxproj中的AdditionalIncludeDirectories是否正确

### Q2: 链接时找不到库文件
**A**: 
- 检查AdditionalLibraryDirectories路径
- 确认库文件存在（libopenblas.lib, hdf5.lib等）
- 检查平台（x64 vs Win32）是否匹配

### Q3: 运行时找不到DLL
**A**: 
- OpenBLAS: 将 `libs\OpenBLAS-0.3.30-x64\lib` 添加到PATH，或复制dll到exe目录
- HDF5: 将 `C:\Program Files\HDF_Group\HDF5\1.14.6\bin` 添加到PATH

### Q4: OpenMP不工作
**A**: 
- 检查 `<OpenMPSupport>true</OpenMPSupport>` 是否在所有配置中
- 检查代码中是否使用了 `#pragma omp parallel for`

---

## 性能测试建议

### OpenBLAS性能测试
```c
// 测试矩阵乘法性能
// 使用BLAS_BACKEND_NATIVE vs BLAS_BACKEND_OPENBLAS
// 比较执行时间
```

### OpenMP性能测试
```c
// 测试并行vs串行性能
// 使用不同线程数
// 测量加速比
```

---

## 下一步

1. **编译项目**验证配置
2. **运行测试**验证功能
3. **性能测试**验证加速效果
4. **完善实现**（OpenBLAS完整调用，HDF5复数类型）

---

## 相关文档

- `docs/LIBRARY_CONFIGURATION_SUMMARY.md` - 详细配置说明
- `docs/DEPENDENCIES_AND_OPTIMIZATION_ANALYSIS.md` - 依赖分析
