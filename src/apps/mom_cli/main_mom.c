/********************************************************************************
 * MoM Solver Command-Line Interface
 *
 * Standalone executable for MoM (Method of Moments) electromagnetic simulation
 * 
 * Copyright (C) 2025 PulseEM Technologies
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../solvers/mom/mom_solver.h"
#include "../../physics/mom/mom_physics.h"  // For MOM_FORMULATION_EFIE
#include "../../orchestration/workflow/workflow_engine.h"
#include "../../common/core_common.h"
#include "../../common/errors_core.h"

// 本地配置文件解析结构（不冲突于头文件中的 mom_config_t）
typedef struct {
    char geometry_file[512];
    char geometry_format[16];
    double frequency;
    double tolerance;
    int max_iterations;
    int mesh_density;
} file_config_t;

// 简单的配置文件解析（实际应该使用 JSON 库）
static int parse_config_file(const char* filename, file_config_t* config) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open config file: %s\n", filename);
        return STATUS_ERROR_FILE_NOT_FOUND;
    }
    
    // 初始化默认值
    strcpy(config->geometry_format, "stl");
    config->frequency = 10e9;
    config->tolerance = 1e-6;
    config->max_iterations = 1000;
    config->mesh_density = 10;
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // 移除换行符
        line[strcspn(line, "\r\n")] = 0;
        
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\0') continue;
        
        // 解析键值对（简单实现）
        if (strstr(line, "geometry_file")) {
            sscanf(line, "geometry_file=%s", config->geometry_file);
        } else if (strstr(line, "geometry_format")) {
            sscanf(line, "geometry_format=%s", config->geometry_format);
        } else if (strstr(line, "frequency")) {
            sscanf(line, "frequency=%lf", &config->frequency);
        } else if (strstr(line, "tolerance")) {
            sscanf(line, "tolerance=%lf", &config->tolerance);
        } else if (strstr(line, "max_iterations")) {
            sscanf(line, "max_iterations=%d", &config->max_iterations);
        } else if (strstr(line, "mesh_density")) {
            sscanf(line, "mesh_density=%d", &config->mesh_density);
        }
    }
    
    fclose(f);
    return STATUS_SUCCESS;
}

static void print_usage(const char* program_name) {
    printf("MoM Solver - Method of Moments Electromagnetic Simulation\n");
    printf("Version 1.0.0\n");
    printf("\nUsage: %s <config_file> [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("\nConfig File Format:\n");
    printf("  geometry_file=<path>\n");
    printf("  geometry_format=stl|step|iges\n");
    printf("  frequency=<frequency_in_hz>\n");
    printf("  tolerance=<tolerance>\n");
    printf("  max_iterations=<number>\n");
    printf("  mesh_density=<elements_per_wavelength>\n");
    printf("\nExample:\n");
    printf("  %s config.txt\n", program_name);
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
    file_config_t file_config = {0};
    if (parse_config_file(argv[1], &file_config) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to parse config file: %s\n", argv[1]);
        return 1;
    }
    
    printf("========================================\n");
    printf("MoM Solver - Starting Simulation\n");
    printf("========================================\n");
    printf("Geometry file: %s\n", file_config.geometry_file);
    printf("Geometry format: %s\n", file_config.geometry_format);
    printf("Frequency: %.2e Hz\n", file_config.frequency);
    printf("Tolerance: %.2e\n", file_config.tolerance);
    printf("Max iterations: %d\n", file_config.max_iterations);
    printf("Mesh density: %d elements/wavelength\n", file_config.mesh_density);
    printf("\n");
    
    // 创建 MoM solver 配置
    printf("[1/5] Creating MoM solver configuration...\n");
    mom_config_t solver_config = {0};
    solver_config.frequency = file_config.frequency;
    solver_config.tolerance = file_config.tolerance;
    solver_config.max_iterations = file_config.max_iterations;
    solver_config.mesh_density = file_config.mesh_density;
    solver_config.basis_type = MOM_BASIS_RWG;
    solver_config.formulation = MOM_FORMULATION_EFIE;  // 默认使用 EFIE
    solver_config.use_preconditioner = true;
    solver_config.use_parallel = true;
    solver_config.compute_current_distribution = true;
    
    // 创建 MoM solver
    printf("[2/5] Creating MoM solver...\n");
    mom_solver_t* solver = mom_solver_create(&solver_config);
    if (!solver) {
        fprintf(stderr, "Error: Failed to create MoM solver\n");
        return 1;
    }
    
    // 加载几何
    printf("[3/5] Loading geometry from %s...\n", file_config.geometry_file);
    if (mom_solver_import_cad(solver, file_config.geometry_file, file_config.geometry_format) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to load geometry file: %s\n", file_config.geometry_file);
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 运行仿真
    printf("[4/5] Running simulation...\n");
    if (mom_solver_solve(solver) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Simulation failed\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 获取结果
    printf("[5/5] Retrieving results...\n");
    const mom_result_t* results = mom_solver_get_results(solver);
    if (!results) {
        fprintf(stderr, "Error: Failed to get results\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 输出结果
    printf("\n========================================\n");
    printf("Simulation Completed Successfully!\n");
    printf("========================================\n");
    printf("Basis functions: %d\n", results->num_basis_functions);
    if (results->matrix_fill_time > 0) {
        printf("Matrix fill time: %.2f seconds\n", results->matrix_fill_time);
    }
    if (results->solve_time > 0) {
        printf("Solve time: %.2f seconds\n", results->solve_time);
    }
    if (results->iterations > 0) {
        printf("Iterations: %d\n", results->iterations);
        printf("Converged: %s\n", results->converged ? "Yes" : "No");
    }
    printf("\n");
    
    // 清理
    mom_solver_destroy(solver);
    
    printf("Done.\n");
    return 0;
}
