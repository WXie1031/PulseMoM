# MoM 和 PEEC 拆分为独立 exe 的可行性分析

## 问题

是否可以将 MoM 和 PEEC 拆分成两个独立的 exe？它们有很多公用部分，可以继续使用。

## 当前架构分析

### 1. 当前构建结构

**库结构：**
```
electromagnetic_kernel_library (静态库)
├── 核心电磁核函数
├── 积分函数
└── Green's 函数

geometry_processing_library (静态库)
├── 几何处理
└── 几何操作

mesh_generation_library (静态库)
├── 网格生成
└── 网格操作

cad_import_library (静态库，可选)
├── STL 导入
├── STEP 导入
└── IGES 导入

mom_solver (静态库)
├── 依赖: electromagnetic_kernel_library
├── 依赖: geometry_processing_library
└── MoM 特定算法

peec_solver (静态库)
├── 依赖: mesh_generation_library
└── PEEC 特定算法
```

### 2. 公共依赖分析

#### 完全共享的模块（两个 exe 都需要）

1. **基础库**
   - `common/` - 公共定义、类型、常量
   - `utils/` - 工具函数
   - `materials/` - 材料定义

2. **几何和网格**
   - `geometry_processing_library` - 几何处理
   - `mesh_generation_library` - 网格生成
   - `cad_import_library` - CAD 导入（可选）

3. **电磁核函数**
   - `electromagnetic_kernel_library` - 核心电磁计算
   - Green's 函数
   - 积分函数

4. **后端库**
   - `backend/` - 数值后端（求解器、GPU等）
   - BLAS/LAPACK 接口

5. **IO 和 API**
   - `io/` - 文件 I/O
   - `orchestration/workflow/` - 工作流引擎

#### MoM 特定的模块

- `solvers/mom/` - MoM 求解器实现
- MoM 特定的基函数（RWG等）
- MoM 特定的加速算法（MLFMM, H-matrix等）

#### PEEC 特定的模块

- `solvers/peec/` - PEEC 求解器实现
- PEEC 特定的电路模型
- PEEC 特定的积分方法

## 拆分方案

### 方案 A：静态库共享（推荐）

**结构：**
```
公共库（静态库）:
├── core_base_library (公共基础库)
│   ├── common/
│   ├── utils/
│   └── materials/
├── geometry_processing_library
├── mesh_generation_library
├── electromagnetic_kernel_library
└── backend_library (数值后端)

mom_solver.exe:
├── 链接: core_base_library
├── 链接: geometry_processing_library
├── 链接: electromagnetic_kernel_library
├── 链接: backend_library
├── 链接: mom_solver (静态库)
└── main_mom.c (MoM 主程序)

peec_solver.exe:
├── 链接: core_base_library
├── 链接: mesh_generation_library
├── 链接: electromagnetic_kernel_library
├── 链接: backend_library
├── 链接: peec_solver (静态库)
└── main_peec.c (PEEC 主程序)
```

**优点：**
- ✅ 代码完全共享，无重复
- ✅ 编译时链接，运行时独立
- ✅ 每个 exe 只包含需要的代码
- ✅ 易于维护和更新

**缺点：**
- ⚠️ 每个 exe 会包含公共库的副本（静态链接）
- ⚠️ 总文件大小较大

### 方案 B：动态库共享

**结构：**
```
公共库（动态库）:
├── pulsemom_core.dll (公共基础库)
├── pulsemom_geometry.dll (几何处理)
├── pulsemom_mesh.dll (网格生成)
├── pulsemom_kernels.dll (电磁核函数)
└── pulsemom_backend.dll (数值后端)

mom_solver.exe:
├── 依赖: pulsemom_core.dll
├── 依赖: pulsemom_geometry.dll
├── 依赖: pulsemom_kernels.dll
├── 依赖: pulsemom_backend.dll
├── 链接: mom_solver (静态库)
└── main_mom.c

peec_solver.exe:
├── 依赖: pulsemom_core.dll
├── 依赖: pulsemom_mesh.dll
├── 依赖: pulsemom_kernels.dll
├── 依赖: pulsemom_backend.dll
├── 链接: peec_solver (静态库)
└── main_peec.c
```

**优点：**
- ✅ 公共库只加载一次（如果同时运行）
- ✅ 文件大小更小
- ✅ 可以独立更新公共库

**缺点：**
- ⚠️ 需要管理 DLL 依赖
- ⚠️ 部署更复杂（需要包含 DLL）
- ⚠️ 版本兼容性问题

### 方案 C：混合方案（推荐用于生产环境）

**结构：**
```
核心公共库（动态库）:
├── pulsemom_core.dll (基础库，体积大，变化少)

特定功能库（静态库）:
├── geometry_processing_library
├── mesh_generation_library
├── electromagnetic_kernel_library
└── backend_library

mom_solver.exe:
├── 依赖: pulsemom_core.dll
├── 静态链接: geometry + kernels + backend + mom_solver
└── main_mom.c

peec_solver.exe:
├── 依赖: pulsemom_core.dll
├── 静态链接: mesh + kernels + backend + peec_solver
└── main_peec.c
```

## 实施难度评估

### 难度：⭐⭐ (中等)

#### 1. CMake 配置修改（简单）

**需要修改：**
- 添加两个 `add_executable()` 目标
- 配置各自的链接库
- 创建主程序入口

**工作量：** 1-2 小时

#### 2. 主程序实现（简单）

**需要创建：**
- `src/apps/mom_cli/main_mom.c` - MoM 主程序
- `src/apps/peec_cli/main_peec.c` - PEEC 主程序

**工作量：** 2-4 小时

#### 3. 公共库组织（中等）

**需要：**
- 确保公共库接口清晰
- 避免循环依赖
- 测试独立编译

**工作量：** 4-8 小时

#### 4. 测试和验证（中等）

**需要：**
- 测试每个 exe 独立运行
- 验证功能完整性
- 性能对比测试

**工作量：** 4-8 小时

### 总工作量估算

- **方案 A（静态库）**: 8-16 小时
- **方案 B（动态库）**: 12-24 小时
- **方案 C（混合）**: 10-20 小时

## 实施步骤

### 阶段 1：准备（1-2 天）

1. **分析依赖关系**
   ```bash
   # 使用工具分析依赖
   # 确认哪些是公共的，哪些是特定的
   ```

2. **设计库结构**
   - 确定公共库边界
   - 设计接口
   - 规划主程序结构

### 阶段 2：实现（2-3 天）

1. **创建主程序**
   ```c
   // src/apps/mom_cli/main_mom.c
   #include "mom_solver.h"
   #include "workflow_engine.h"
   
   int main(int argc, char* argv[]) {
       // 解析命令行参数
       // 创建 workflow engine
       // 配置 MoM solver
       // 执行仿真
       // 输出结果
   }
   ```

2. **修改 CMakeLists.txt**
   ```cmake
   # MoM executable
   add_executable(mom_solver_exe
       src/apps/mom_cli/main_mom.c
   )
   target_link_libraries(mom_solver_exe
       PRIVATE
       mom_solver
       electromagnetic_kernel_library
       geometry_processing_library
       core_base_library
   )
   
   # PEEC executable
   add_executable(peec_solver_exe
       src/apps/peec_cli/main_peec.c
   )
   target_link_libraries(peec_solver_exe
       PRIVATE
       peec_solver
       electromagnetic_kernel_library
       mesh_generation_library
       core_base_library
   )
   ```

3. **测试编译**
   - 确保每个 exe 可以独立编译
   - 解决链接错误
   - 验证功能

### 阶段 3：优化（1-2 天）

1. **减少代码重复**
   - 提取更多公共代码
   - 优化库结构

2. **性能优化**
   - 减少不必要的依赖
   - 优化链接选项

3. **文档更新**
   - 更新构建文档
   - 更新使用文档

## 代码示例

### MoM 主程序示例

```c
// src/apps/mom_cli/main_mom.c
#include <stdio.h>
#include <stdlib.h>
#include "mom_solver.h"
#include "workflow_engine.h"
#include "core_common.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <config_file.json>\n", argv[0]);
        return 1;
    }
    
    // 创建 MoM solver
    mom_solver_t* solver = mom_solver_create();
    if (!solver) {
        fprintf(stderr, "Failed to create MoM solver\n");
        return 1;
    }
    
    // 配置 solver
    mom_config_t config = {0};
    config.frequency = 10e9;
    config.tolerance = 1e-6;
    // ... 从配置文件加载
    
    if (mom_solver_configure(solver, &config) != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to configure solver\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 加载几何
    if (mom_solver_import_cad(solver, argv[1], "stl") != STATUS_SUCCESS) {
        fprintf(stderr, "Failed to load geometry\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 运行仿真
    if (mom_solver_solve(solver) != STATUS_SUCCESS) {
        fprintf(stderr, "Simulation failed\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 获取结果
    const mom_result_t* results = mom_solver_get_results(solver);
    printf("Simulation completed successfully\n");
    printf("Basis functions: %d\n", results->num_basis_functions);
    
    // 清理
    mom_solver_destroy(solver);
    return 0;
}
```

### PEEC 主程序示例

```c
// src/apps/peec_cli/main_peec.c
#include <stdio.h>
#include <stdlib.h>
#include "peec_solver.h"
#include "workflow_engine.h"
#include "core_common.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <config_file.json>\n", argv[0]);
        return 1;
    }
    
    // 创建 PEEC solver
    peec_solver_t* solver = peec_solver_create();
    if (!solver) {
        fprintf(stderr, "Failed to create PEEC solver\n");
        return 1;
    }
    
    // 配置和运行（类似 MoM）
    // ...
    
    peec_solver_destroy(solver);
    return 0;
}
```

## 优势分析

### 1. 代码复用

- ✅ 公共库只编译一次
- ✅ 公共代码只维护一份
- ✅ 减少代码重复

### 2. 部署灵活性

- ✅ 用户可以选择只安装需要的 exe
- ✅ 减少不必要的依赖
- ✅ 更小的部署包（如果使用动态库）

### 3. 开发维护

- ✅ 清晰的模块边界
- ✅ 独立的测试和调试
- ✅ 更容易理解代码结构

### 4. 性能

- ✅ 每个 exe 只包含需要的代码
- ✅ 更小的内存占用
- ✅ 更快的启动时间

## 潜在问题

### 1. 代码重复

**问题：** 如果使用静态库，公共代码会在每个 exe 中重复

**解决：**
- 使用动态库（方案 B 或 C）
- 或者接受一定的代码重复（通常不是问题）

### 2. 依赖管理

**问题：** 需要确保公共库的版本兼容

**解决：**
- 使用版本号
- 清晰的接口定义
- 向后兼容策略

### 3. 构建复杂度

**问题：** CMake 配置可能变得更复杂

**解决：**
- 良好的 CMake 组织
- 清晰的文档
- 自动化测试

## 建议

### 推荐方案：方案 A（静态库共享）

**理由：**
1. **实施简单**：工作量最小，风险最低
2. **维护容易**：不需要管理 DLL 依赖
3. **性能好**：静态链接，无运行时开销
4. **部署简单**：每个 exe 是独立的，不需要额外的 DLL

**适用场景：**
- 开发环境
- 内部使用
- 不需要频繁更新的场景

### 备选方案：方案 C（混合方案）

**适用场景：**
- 生产环境
- 需要频繁更新公共库
- 多个 exe 可能同时运行

## 总结

**可行性：** ✅ 完全可行

**难度：** ⭐⭐ (中等，1-3 天工作量)

**建议：**
1. 从方案 A 开始（静态库）
2. 如果后续需要，可以迁移到方案 C（混合）
3. 保持公共库接口稳定
4. 充分测试每个 exe 的独立性

**关键成功因素：**
- 清晰的模块边界
- 良好的 CMake 组织
- 充分的测试
- 详细的文档
