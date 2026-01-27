# CLI 程序修复说明

## 修复的问题

### 1. 类型重定义错误

**问题**：
- `main_mom.c` 和 `main_peec.c` 中重新定义了 `mom_config_t` 和 `peec_config_t`
- 这些类型已经在 `mom_solver.h` 和 `peec_solver.h` 中定义

**修复**：
- 将本地配置结构重命名为 `file_config_t`
- 用于解析配置文件，然后转换为实际的 solver 配置结构

### 2. 结构体成员不匹配

**问题**：
- 头文件中定义的 `mom_config` 和 `peec_config` 结构体没有 `geometry_file` 和 `geometry_format` 字段
- 这些字段是文件解析用的，不应该在 solver 配置中

**修复**：
- 使用 `file_config_t` 存储文件解析结果
- 创建 `mom_config_t` 和 `peec_config_t` 用于 solver 配置
- 从 `file_config_t` 复制相关字段到 solver 配置

### 3. 函数调用参数错误

**问题**：
- `mom_solver_create()` 和 `peec_solver_create()` 需要配置参数，但代码中没有传递

**修复**：
- 在创建 solver 之前先创建配置结构
- 正确初始化所有必要的配置字段
- 传递配置指针给创建函数

## 修复后的代码结构

### main_mom.c

```c
// 本地配置文件结构（不冲突于头文件）
typedef struct {
    char geometry_file[512];
    char geometry_format[16];
    double frequency;
    double tolerance;
    int max_iterations;
    int mesh_density;
} file_config_t;

// 解析配置文件到 file_config_t
static int parse_config_file(const char* filename, file_config_t* config);

// main 函数流程：
// 1. 解析配置文件 -> file_config_t
// 2. 创建 solver 配置 -> mom_config_t
// 3. 创建 solver -> mom_solver_create(&solver_config)
// 4. 导入几何 -> mom_solver_import_cad(...)
// 5. 求解 -> mom_solver_solve(...)
// 6. 获取结果 -> mom_solver_get_results(...)
```

### main_peec.c

```c
// 本地配置文件结构（不冲突于头文件）
typedef struct {
    char geometry_file[512];
    char geometry_format[16];
    double frequency;
    double tolerance;
    int max_iterations;
    int mesh_density;
} file_config_t;

// 解析配置文件到 file_config_t
static int parse_config_file(const char* filename, file_config_t* config);

// main 函数流程：
// 1. 解析配置文件 -> file_config_t
// 2. 创建 solver 配置 -> peec_config_t
// 3. 创建 solver -> peec_solver_create(&solver_config)
// 4. 导入几何 -> peec_solver_import_cad(...)
// 5. 提取部分元件 -> peec_solver_extract_partial_elements(...)
// 6. 构建电路网络 -> peec_solver_build_circuit_network(...)
// 7. 求解电路 -> peec_solver_solve_circuit(...)
// 8. 获取结果 -> peec_solver_get_results(...)
```

## 配置字段映射

### MoM Solver 配置

| file_config_t 字段 | mom_config_t 字段 | 说明 |
|-------------------|-------------------|------|
| frequency | frequency | 分析频率 |
| tolerance | tolerance | 求解器容差 |
| max_iterations | max_iterations | 最大迭代次数 |
| mesh_density | mesh_density | 网格密度 |
| - | basis_type | 基函数类型（默认 RWG） |
| - | formulation | 公式类型（默认 EFIE） |
| - | use_preconditioner | 使用预条件器（默认 true） |
| - | use_parallel | 并行处理（默认 true） |

### PEEC Solver 配置

| file_config_t 字段 | peec_config_t 字段 | 说明 |
|-------------------|---------------------|------|
| frequency | frequency | 分析频率 |
| tolerance | circuit_tolerance | 电路求解器容差 |
| max_iterations | circuit_max_iterations | 最大迭代次数 |
| mesh_density | mesh_density | 网格密度 |
| - | formulation | 公式类型（默认 CLASSICAL） |
| - | extract_resistance | 提取电阻（默认 true） |
| - | extract_inductance | 提取电感（默认 true） |
| - | extract_capacitance | 提取电容（默认 true） |

## 包含的头文件

### main_mom.c
```c
#include "../../solvers/mom/mom_solver.h"
#include "../../physics/mom/mom_physics.h"  // For MOM_FORMULATION_EFIE
#include "../../orchestration/workflow/workflow_engine.h"
#include "../../common/core_common.h"
#include "../../common/errors_core.h"
```

### main_peec.c
```c
#include "../../solvers/peec/peec_solver.h"
#include "../../physics/peec/peec_physics.h"  // For PEEC_FORMULATION_CLASSICAL
#include "../../orchestration/workflow/workflow_engine.h"
#include "../../common/core_common.h"
#include "../../common/errors_core.h"
```

## 测试建议

1. **编译测试**：
   ```powershell
   # 在 Visual Studio 中构建解决方案
   # 应该没有编译错误
   ```

2. **运行测试**：
   ```powershell
   # 创建测试配置文件
   echo geometry_file=tests/test_geometry.stl > config.txt
   echo frequency=10e9 >> config.txt
   echo tolerance=1e-6 >> config.txt
   
   # 运行 MoM solver
   .\build\x64\Debug\PulseMoM.exe config.txt
   
   # 运行 PEEC solver
   .\build\x64\Debug\PulsePEEC.exe config.txt
   ```

## 注意事项

1. **配置初始化**：使用 `{0}` 初始化结构体确保所有字段都被清零
2. **错误处理**：所有 solver 函数调用都检查返回值
3. **资源清理**：在错误情况下正确释放 solver 资源
4. **类型安全**：使用正确的类型，避免类型冲突
