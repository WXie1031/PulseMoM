# vcpkg库优化指南

## 完成时间
2025-01-XX

## 概述

本文档提供vcpkg库的完整集成和优化指南，包括代码实现细节和性能优化建议。

---

## 一、vcpkg库配置状态

### ✅ 已配置的库

| 库 | vcpkg包名 | 状态 | 预处理器定义 | 代码位置 |
|----|-----------|------|--------------|----------|
| OpenBLAS | `openblas` | ✅ 已配置 | `ENABLE_OPENBLAS` | `blas_interface.c` |
| HDF5 | `hdf5` | ✅ 已配置 | `ENABLE_HDF5` | `file_io.c` |
| SuperLU | `superlu` | ✅ 已配置 | `ENABLE_SUPERLU` | `direct_solver.c` |
| GSL | `gsl` | ✅ 已配置 | `ENABLE_GSL` | `singular_integration.c` |
| CGAL | `cgal` | ✅ 已配置 | `USE_CGAL` | CMakeLists.txt |
| Gmsh | `gmsh` | ✅ 已配置 | `USE_GMSH` | CMakeLists.txt |
| OpenCascade | `opencascade` | ✅ 已配置 | `USE_OPENCASCADE` | CMakeLists.txt |

### ⚠️ MUMPS处理

- **vcpkg支持**: ❌ 不支持
- **替代方案**: 使用SuperLU（已配置）
- **建议**: 优先使用SuperLU，MUMPS作为可选（从源码编译）

---

## 二、代码优化实现指南

### 2.1 OpenBLAS集成（高优先级）

#### 当前状态
- ✅ 头文件包含: `#include <cblas.h>`, `#include <lapacke.h>`
- ✅ 初始化代码: `openblas_set_num_threads()`
- ⚠️ 实际调用: 待实现

#### 实现步骤

**步骤1: 类型转换函数**

在 `src/backend/math/blas_interface.c` 中添加：

```c
#ifdef ENABLE_OPENBLAS
// Convert complex_t to double complex (for OpenBLAS)
static void complex_to_double_complex(const complex_t* src, double complex* dst, int n) {
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        dst[i] = src[i].re + I * src[i].im;
        #else
        dst[i] = src[i];
        #endif
    }
}

// Convert double complex to complex_t
static void double_complex_to_complex(const double complex* src, complex_t* dst, int n) {
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        dst[i].re = creal(src[i]);
        dst[i].im = cimag(src[i]);
        #else
        dst[i] = src[i];
        #endif
    }
}
#endif
```

**步骤2: 实现cblas_zgemv**

```c
case BLAS_BACKEND_OPENBLAS:
    #ifdef ENABLE_OPENBLAS
    {
        // Convert inputs
        double complex alpha_dc = alpha.re + I * alpha.im;
        double complex beta_dc = beta.re + I * beta.im;
        
        double complex* A_dc = (double complex*)malloc(m * n * sizeof(double complex));
        double complex* x_dc = (double complex*)malloc(n * sizeof(double complex));
        double complex* y_dc = (double complex*)malloc(m * sizeof(double complex));
        
        if (!A_dc || !x_dc || !y_dc) {
            if (A_dc) free(A_dc);
            if (x_dc) free(x_dc);
            if (y_dc) free(y_dc);
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        
        // Convert A, x, y
        complex_to_double_complex(A, A_dc, m * n);
        complex_to_double_complex(x, x_dc, n);
        complex_to_double_complex(y, y_dc, m);
        
        // Call OpenBLAS
        CBLAS_TRANSPOSE trans_cblas = (trans == 'T' || trans == 't') ? CblasTrans : CblasNoTrans;
        cblas_zgemv(CblasColMajor, trans_cblas, m, n, 
                    &alpha_dc, A_dc, lda, x_dc, incx, 
                    &beta_dc, y_dc, incy);
        
        // Convert result back
        double_complex_to_complex(y_dc, y, m);
        
        free(A_dc);
        free(x_dc);
        free(y_dc);
        
        return STATUS_SUCCESS;
    }
    #endif
    break;
```

**步骤3: 实现cblas_zgemm**（类似方法）

#### 优化收益
- 矩阵向量积: 加速2-5倍
- 矩阵矩阵积: 加速5-10倍
- 大规模问题: 显著提升

### 2.2 SuperLU集成（高优先级）

#### 当前状态
- ✅ 头文件包含: `#include <slu_ddefs.h>`, `#include <slu_zdefs.h>`
- ✅ 接口框架: 已添加
- ⚠️ 完整实现: 待完成

#### 实现步骤

**步骤1: CSR到SuperLU格式转换**

```c
#ifdef ENABLE_SUPERLU
// Convert CSR to SuperLU's CompCol format
static int csr_to_superlu(
    const sparse_matrix_csr_t* csr,
    SuperMatrix* A_superlu) {
    
    // SuperLU uses CompCol (column-compressed) format
    // We need to convert from CSR (row-compressed) to CompCol
    
    int n = csr->num_rows;
    int nnz = csr->nnz;
    
    // Allocate SuperLU arrays
    int* col_ptr = (int*)SUPERLU_MALLOC((n + 1) * sizeof(int));
    int* row_idx = (int*)SUPERLU_MALLOC(nnz * sizeof(int));
    doublecomplex* values = (doublecomplex*)SUPERLU_MALLOC(nnz * sizeof(doublecomplex));
    
    if (!col_ptr || !row_idx || !values) {
        if (col_ptr) SUPERLU_FREE(col_ptr);
        if (row_idx) SUPERLU_FREE(row_idx);
        if (values) SUPERLU_FREE(values);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Convert CSR to CompCol
    // ... (implementation details) ...
    
    // Create SuperMatrix
    dCreate_CompCol_Matrix(A_superlu, n, n, nnz, values, row_idx, col_ptr,
                          SLU_NC, SLU_Z, SLU_GE);
    
    return STATUS_SUCCESS;
}
#endif
```

**步骤2: LU分解和求解**

```c
#ifdef ENABLE_SUPERLU
// SuperLU factorization
static int superlu_factorize(
    SuperMatrix* A,
    SuperMatrix* L,
    SuperMatrix* U,
    int* perm_c,
    int* perm_r) {
    
    // SuperLU options
    superlu_options_t options;
    set_default_options(&options);
    
    // Workspace
    int info;
    SuperLUStat_t stat;
    StatInit(&stat);
    
    // Perform factorization
    dgstrf(&options, A, perm_c, perm_r, L, U, &stat, &info);
    
    StatFree(&stat);
    
    return (info == 0) ? STATUS_SUCCESS : STATUS_ERROR_NUMERICAL;
}

// SuperLU solve
static int superlu_solve(
    SuperMatrix* L,
    SuperMatrix* U,
    int* perm_c,
    int* perm_r,
    doublecomplex* b,
    doublecomplex* x) {
    
    // ... (implementation) ...
    
    return STATUS_SUCCESS;
}
#endif
```

#### 优化收益
- 大规模稀疏问题: 求解时间减少10-100倍
- 内存使用: 显著减少（不转换为密集矩阵）

### 2.3 GSL积分方法集成（中优先级）

#### 当前状态
- ✅ 头文件包含: `#include <gsl/gsl_integration.h>`
- ⚠️ 集成: 待完成

#### 实现步骤

**步骤1: GSL QUADPACK集成**

```c
#ifdef ENABLE_GSL
// GSL adaptive integration for singular integrals
static complex_t gsl_adaptive_integration(
    const real_t* triangle_vertices,
    const real_t* obs_point,
    real_t k,
    greens_kernel_type_t kernel_type) {
    
    gsl_integration_workspace* w = gsl_integration_workspace_alloc(1000);
    gsl_function F;
    F.function = &greens_function_wrapper;  // Wrapper function
    F.params = &integration_params;  // Parameters
    
    double result, error;
    gsl_integration_qag(&F, 0.0, 1.0, 1e-7, 1e-7, 1000, 
                       GSL_INTEG_GAUSS15, w, &result, &error);
    
    gsl_integration_workspace_free(w);
    
    // Convert to complex_t
    complex_t ret;
    ret.re = result;
    ret.im = 0.0;  // Adjust based on kernel type
    
    return ret;
}
#endif
```

#### 优化收益
- 高级积分方法: 提高精度
- 自适应积分: 自动选择积分点

---

## 三、其他优化库分析

### 3.1 不需要的库

#### Intel MKL
- **原因**: vcpkg不支持（商业库）
- **替代**: OpenBLAS（已配置）
- **建议**: 使用OpenBLAS

#### PARDISO
- **原因**: 包含在MKL中
- **替代**: SuperLU（已配置）
- **建议**: 使用SuperLU

#### MUMPS
- **原因**: vcpkg不支持
- **替代**: SuperLU（已配置）
- **建议**: 优先使用SuperLU

### 3.2 可选库

#### Eigen3
- **vcpkg包名**: `eigen3`
- **用途**: C++线性代数
- **建议**: 项目主要使用C，Eigen为可选

#### PETSc
- **vcpkg包名**: `petsc`
- **用途**: 大规模并行计算
- **建议**: 大规模并行场景使用

---

## 四、性能优化建议

### 4.1 立即实施（高优先级）

1. **完成OpenBLAS集成**
   - 实现类型转换函数
   - 实现cblas_zgemv和cblas_zgemm
   - **预期收益**: 矩阵运算加速2-10倍

2. **完成SuperLU集成**
   - 实现CSR到SuperLU格式转换
   - 实现稀疏LU分解和求解
   - **预期收益**: 大规模稀疏问题加速10-100倍

### 4.2 近期实施（中优先级）

3. **完成HDF5完整实现**
   - 处理复数类型（compound datatype）
   - **预期收益**: 大规模数据存储支持

4. **GSL积分方法集成**（可选）
   - 集成QUADPACK
   - **预期收益**: 提高积分精度

### 4.3 未来考虑（低优先级）

5. **完全迁移到vcpkg**
   - 移除本地库路径
   - **预期收益**: 统一依赖管理

---

## 五、验证和测试

### 5.1 编译验证

```bash
# 使用CMake配置
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=D:/Project/vcpkg-master/scripts/buildsystems/vcpkg.cmake

# 检查库是否找到
# 应该看到:
# - OpenBLAS found via vcpkg
# - HDF5 found via vcpkg
# - SuperLU found via vcpkg
# - GSL found via vcpkg
```

### 5.2 运行时验证

```c
// 检查库是否可用
#ifdef ENABLE_OPENBLAS
    printf("OpenBLAS: Enabled\n");
#else
    printf("OpenBLAS: Not enabled\n");
#endif

#ifdef ENABLE_SUPERLU
    printf("SuperLU: Enabled\n");
#else
    printf("SuperLU: Not enabled\n");
#endif

#ifdef ENABLE_GSL
    printf("GSL: Enabled\n");
#else
    printf("GSL: Not enabled\n");
#endif
```

---

## 六、总结

### 已完成的配置
- ✅ CMakeLists.txt: 优先使用vcpkg，fallback本地
- ✅ vcxproj: 添加vcpkg路径和预处理器定义
- ✅ 代码框架: 添加库的头文件包含和接口框架
- ✅ Skills文档: 创建外部库依赖规则文档

### 待完成的工作
- ⚠️ OpenBLAS: 完整实现BLAS调用（类型转换+函数调用）
- ⚠️ SuperLU: 完整实现稀疏LU分解（格式转换+分解+求解）
- ⚠️ HDF5: 完整实现复数类型处理
- ⚠️ GSL: 集成高级积分方法（可选）

### 优化收益预期
- **OpenBLAS**: 矩阵运算加速2-10倍
- **SuperLU**: 大规模稀疏问题加速10-100倍
- **GSL**: 提高积分精度
- **统一管理**: 简化依赖管理

---

## 相关文档

- `.claude/skills/backend/external_libraries_rules.md` - 外部库依赖规则
- `docs/VCPKG_LIBRARY_ANALYSIS.md` - vcpkg库支持分析
- `docs/VCPKG_INTEGRATION_SUMMARY.md` - vcpkg集成总结
- `docs/DEPENDENCIES_AND_OPTIMIZATION_ANALYSIS.md` - 依赖分析
