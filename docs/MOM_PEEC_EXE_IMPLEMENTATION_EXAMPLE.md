# MoM 和 PEEC 独立 exe 实现示例

## CMakeLists.txt 修改示例

在 `CMakeLists.txt` 中添加以下内容（在现有库定义之后）：

```cmake
# ============================================================================
# MoM 和 PEEC 独立可执行文件
# ============================================================================

# MoM Solver Executable
add_executable(mom_solver_exe
    src/apps/mom_cli/main_mom.c
)

target_link_libraries(mom_solver_exe
    PRIVATE
    mom_solver                    # MoM 求解器库
    electromagnetic_kernel_library  # 电磁核函数库（公共）
    geometry_processing_library     # 几何处理库（公共）
    mesh_generation_library        # 网格生成库（MoM 需要）
    core_base_library             # 基础库（公共）
)

# 如果使用 CAD 导入
if(TARGET cad_import_library)
    target_link_libraries(mom_solver_exe PRIVATE cad_import_library)
endif()

# 外部库依赖
if(OpenBLAS_FOUND)
    target_link_libraries(mom_solver_exe PRIVATE ${OpenBLAS_LIBRARIES})
endif()

if(HDF5_FOUND)
    target_link_libraries(mom_solver_exe PRIVATE ${HDF5_LIBRARIES})
endif()

target_link_libraries(mom_solver_exe PRIVATE OpenMP::OpenMP_C Threads::Threads)

# PEEC Solver Executable
add_executable(peec_solver_exe
    src/apps/peec_cli/main_peec.c
)

target_link_libraries(peec_solver_exe
    PRIVATE
    peec_solver                   # PEEC 求解器库
    electromagnetic_kernel_library  # 电磁核函数库（公共）
    mesh_generation_library        # 网格生成库（PEEC 需要）
    geometry_processing_library     # 几何处理库（公共）
    core_base_library             # 基础库（公共）
)

# 如果使用 CAD 导入
if(TARGET cad_import_library)
    target_link_libraries(peec_solver_exe PRIVATE cad_import_library)
endif()

# 外部库依赖
if(OpenBLAS_FOUND)
    target_link_libraries(peec_solver_exe PRIVATE ${OpenBLAS_LIBRARIES})
endif()

if(HDF5_FOUND)
    target_link_libraries(peec_solver_exe PRIVATE ${HDF5_LIBRARIES})
endif()

target_link_libraries(peec_solver_exe PRIVATE OpenMP::OpenMP_C Threads::Threads)
```

## 主程序实现

### MoM 主程序

创建文件：`src/apps/mom_cli/main_mom.c`

```c
/********************************************************************************
 * MoM Solver Command-Line Interface
 *
 * Standalone executable for MoM (Method of Moments) electromagnetic simulation
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mom_solver.h"
#include "workflow_engine.h"
#include "core_common.h"
#include "core_errors.h"

// 简单的 JSON 解析（可以使用 cJSON 库）
typedef struct {
    char geometry_file[256];
    double frequency;
    double tolerance;
    int max_iterations;
} mom_config_t;

static int parse_config_file(const char* filename, mom_config_t* config) {
    // 简化实现：实际应该使用 JSON 解析库
    FILE* f = fopen(filename, "r");
    if (!f) return STATUS_ERROR_FILE_NOT_FOUND;
    
    // 简单的键值对解析
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "geometry_file")) {
            sscanf(line, "geometry_file: %s", config->geometry_file);
        } else if (strstr(line, "frequency")) {
            sscanf(line, "frequency: %lf", &config->frequency);
        } else if (strstr(line, "tolerance")) {
            sscanf(line, "tolerance: %lf", &config->tolerance);
        } else if (strstr(line, "max_iterations")) {
            sscanf(line, "max_iterations: %d", &config->max_iterations);
        }
    }
    
    fclose(f);
    return STATUS_SUCCESS;
}

static void print_usage(const char* program_name) {
    printf("MoM Solver - Method of Moments Electromagnetic Simulation\n");
    printf("Usage: %s <config_file.json> [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("\nExample:\n");
    printf("  %s config.json\n", program_name);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // 处理命令行选项
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("MoM Solver version 1.0.0\n");
        return 0;
    }
    
    // 解析配置文件
    mom_config_t config = {0};
    config.frequency = 10e9;  // 默认值
    config.tolerance = 1e-6;
    config.max_iterations = 1000;
    
    if (parse_config_file(argv[1], &config) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to parse config file: %s\n", argv[1]);
        return 1;
    }
    
    printf("MoM Solver - Starting simulation\n");
    printf("================================\n");
    printf("Geometry file: %s\n", config.geometry_file);
    printf("Frequency: %.2e Hz\n", config.frequency);
    printf("Tolerance: %.2e\n", config.tolerance);
    printf("\n");
    
    // 创建 MoM solver
    mom_solver_t* solver = mom_solver_create();
    if (!solver) {
        fprintf(stderr, "Error: Failed to create MoM solver\n");
        return 1;
    }
    
    // 配置 solver
    mom_config_t solver_config = {0};
    solver_config.frequency = config.frequency;
    solver_config.tolerance = config.tolerance;
    solver_config.max_iterations = config.max_iterations;
    
    if (mom_solver_configure(solver, &solver_config) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to configure solver\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 加载几何
    printf("Loading geometry...\n");
    if (mom_solver_import_cad(solver, config.geometry_file, "stl") != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to load geometry file: %s\n", config.geometry_file);
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 生成网格
    printf("Generating mesh...\n");
    // (网格生成逻辑)
    
    // 运行仿真
    printf("Running simulation...\n");
    if (mom_solver_solve(solver) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Simulation failed\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 获取结果
    const mom_result_t* results = mom_solver_get_results(solver);
    if (!results) {
        fprintf(stderr, "Error: Failed to get results\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 输出结果
    printf("\nSimulation completed successfully!\n");
    printf("================================\n");
    printf("Basis functions: %d\n", results->num_basis_functions);
    printf("Matrix fill time: %.2f seconds\n", results->matrix_fill_time);
    printf("Solve time: %.2f seconds\n", results->solve_time);
    printf("Iterations: %d\n", results->iterations);
    printf("Converged: %s\n", results->converged ? "Yes" : "No");
    
    // 保存结果（如果需要）
    // ...
    
    // 清理
    mom_solver_destroy(solver);
    
    printf("\nDone.\n");
    return 0;
}
```

### PEEC 主程序

创建文件：`src/apps/peec_cli/main_peec.c`

```c
/********************************************************************************
 * PEEC Solver Command-Line Interface
 *
 * Standalone executable for PEEC (Partial Element Equivalent Circuit) simulation
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "peec_solver.h"
#include "workflow_engine.h"
#include "core_common.h"
#include "core_errors.h"

// PEEC 配置结构（类似 MoM）
typedef struct {
    char geometry_file[256];
    double frequency;
    double tolerance;
    int max_iterations;
} peec_config_t;

static int parse_config_file(const char* filename, peec_config_t* config) {
    // 简化实现：实际应该使用 JSON 解析库
    FILE* f = fopen(filename, "r");
    if (!f) return STATUS_ERROR_FILE_NOT_FOUND;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "geometry_file")) {
            sscanf(line, "geometry_file: %s", config->geometry_file);
        } else if (strstr(line, "frequency")) {
            sscanf(line, "frequency: %lf", &config->frequency);
        } else if (strstr(line, "tolerance")) {
            sscanf(line, "tolerance: %lf", &config->tolerance);
        } else if (strstr(line, "max_iterations")) {
            sscanf(line, "max_iterations: %d", &config->max_iterations);
        }
    }
    
    fclose(f);
    return STATUS_SUCCESS;
}

static void print_usage(const char* program_name) {
    printf("PEEC Solver - Partial Element Equivalent Circuit Simulation\n");
    printf("Usage: %s <config_file.json> [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("\nExample:\n");
    printf("  %s config.json\n", program_name);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // 处理命令行选项
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }
    
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        printf("PEEC Solver version 1.0.0\n");
        return 0;
    }
    
    // 解析配置文件
    peec_config_t config = {0};
    config.frequency = 1e9;  // 默认值
    config.tolerance = 1e-6;
    config.max_iterations = 1000;
    
    if (parse_config_file(argv[1], &config) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to parse config file: %s\n", argv[1]);
        return 1;
    }
    
    printf("PEEC Solver - Starting simulation\n");
    printf("=================================\n");
    printf("Geometry file: %s\n", config.geometry_file);
    printf("Frequency: %.2e Hz\n", config.frequency);
    printf("Tolerance: %.2e\n", config.tolerance);
    printf("\n");
    
    // 创建 PEEC solver
    peec_solver_t* solver = peec_solver_create();
    if (!solver) {
        fprintf(stderr, "Error: Failed to create PEEC solver\n");
        return 1;
    }
    
    // 配置和运行（类似 MoM）
    // ...
    
    // 清理
    peec_solver_destroy(solver);
    
    printf("\nDone.\n");
    return 0;
}
```

## 目录结构

```
src/
├── apps/
│   ├── mom_cli/
│   │   └── main_mom.c
│   └── peec_cli/
│       └── main_peec.c
├── solvers/
│   ├── mom/          # MoM 特定代码
│   └── peec/         # PEEC 特定代码
├── common/           # 公共代码
├── discretization/   # 公共代码
├── operators/        # 公共代码
└── backend/         # 公共代码
```

## 构建和使用

### 构建

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 使用

```bash
# MoM solver
./mom_solver_exe config_mom.json

# PEEC solver
./peec_solver_exe config_peec.json
```

## 优势

1. **独立部署**：每个 exe 可以独立分发
2. **代码共享**：公共库只编译一次
3. **清晰结构**：每个求解器有明确的主程序
4. **易于维护**：修改一个求解器不影响另一个

## 注意事项

1. **公共库版本**：确保两个 exe 使用相同版本的公共库
2. **接口稳定性**：公共库接口应该保持稳定
3. **测试**：需要分别测试每个 exe
4. **文档**：需要为每个 exe 编写使用文档
