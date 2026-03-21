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
#include <math.h>

/* Minimal types for config parsing only (avoids pulling in headers that may
   conflict under MSVC when parsing this file as C). */
#include "../../common/types.h"

/* Local config struct and parser - defined before other project headers. */
typedef struct mom_cli_file_config {
    char geometry_file[512];
    char geometry_format[16];
    double frequency;
    double tolerance;
    int max_iterations;
    int mesh_density;
    int conductor_material_id;  /* -1: all conductor; >=0: only elements with this material_id (e.g. body=0, wheels=other) */
} mom_cli_file_config_t;

/* Build an output results file path in the same directory as the geometry file.
 * Example:
 *   geometry_file = "D:\\PulseMoM\\PulseMoM\\cartingCar\\cartingCar.stp"
 *   => output_path = "D:\\PulseMoM\\PulseMoM\\cartingCar\\cartingCar_results.txt"
 */
static void build_results_path(const char* geometry_file,
                               char* output_path,
                               size_t output_size) {
    if (!geometry_file || !output_path || output_size == 0) {
        return;
    }

    /* Find last directory separator */
    const char* last_slash = strrchr(geometry_file, '\\');
    const char* last_fwd   = strrchr(geometry_file, '/');
    const char* sep = last_slash ? last_slash : last_fwd;
    if (last_fwd && (!sep || last_fwd > sep)) {
        sep = last_fwd;
    }

    /* Directory part length (including separator if present) */
    size_t dir_len = 0;
    if (sep) {
        dir_len = (size_t)(sep - geometry_file + 1);
    }

    /* File name part (without extension) */
    const char* filename = geometry_file + dir_len;
    const char* last_dot = strrchr(filename, '.');
    size_t base_len = last_dot ? (size_t)(last_dot - filename) : strlen(filename);

    /* Compose: <dir><basename>_results.txt */
    if (dir_len + base_len + strlen("_results.txt") + 1 > output_size) {
        /* Fallback: just write into current directory */
        snprintf(output_path, output_size, "results.txt");
        return;
    }

    memcpy(output_path, geometry_file, dir_len);
    memcpy(output_path + dir_len, filename, base_len);
    output_path[dir_len + base_len] = '\0';
    strncat(output_path, "_results.txt", output_size - (dir_len + base_len) - 1);
}

/* Get output directory (same as geometry file directory), with trailing separator.
 * Example: geometry_file = "D:\\PulseMoM\\cartingCar\\cartingCar.stp"
 *          => dir_out = "D:\\PulseMoM\\cartingCar\\"
 */
static void get_output_dir(const char* geometry_file, char* dir_out, size_t dir_size) {
    if (!geometry_file || !dir_out || dir_size == 0) return;
    const char* last_slash = strrchr(geometry_file, '\\');
    const char* last_fwd   = strrchr(geometry_file, '/');
    const char* sep = last_slash ? last_slash : last_fwd;
    if (last_fwd && (!sep || last_fwd > sep)) sep = last_fwd;
    size_t dir_len = sep ? (size_t)(sep - geometry_file + 1) : 0;
    if (dir_len >= dir_size) { dir_out[0] = '\0'; return; }
    memcpy(dir_out, geometry_file, dir_len);
    dir_out[dir_len] = '\0';
}

static int parse_config_file(const char* filename, mom_cli_file_config_t* cfg) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open config file: %s\n", filename);
        return STATUS_ERROR_FILE_NOT_FOUND;
    }
    
    // 初始化默认值
    strcpy(cfg->geometry_format, "step");
    cfg->frequency = 10e9;
    cfg->tolerance = 1e-6;
    cfg->max_iterations = 1000;
    cfg->mesh_density = 10;
    cfg->conductor_material_id = -1;  /* -1: treat all as conductor */
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        // 移除换行符
        line[strcspn(line, "\r\n")] = 0;
        
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\0') continue;
        
        // 解析键值对（简单实现）
        if (strstr(line, "geometry_file")) {
            sscanf(line, "geometry_file=%s", cfg->geometry_file);
        } else if (strstr(line, "geometry_format")) {
            sscanf(line, "geometry_format=%s", cfg->geometry_format);
        } else if (strstr(line, "frequency")) {
            sscanf(line, "frequency=%lf", &cfg->frequency);
        } else if (strstr(line, "tolerance")) {
            sscanf(line, "tolerance=%lf", &cfg->tolerance);
        } else if (strstr(line, "max_iterations")) {
            sscanf(line, "max_iterations=%d", &cfg->max_iterations);
        } else if (strstr(line, "mesh_density")) {
            sscanf(line, "mesh_density=%d", &cfg->mesh_density);
        } else if (strstr(line, "conductor_material_id")) {
            sscanf(line, "conductor_material_id=%d", &cfg->conductor_material_id);
        }
    }
    
    fclose(f);
    return STATUS_SUCCESS;
}

#include "../../solvers/mom/mom_solver.h"
#include "../../physics/mom/mom_physics.h"
#include "../../orchestration/workflow/workflow_engine.h"
#include "../../common/core_common.h"
#include "../../common/errors_core.h"

/* Near-field sampling plane parameters (demo version) */
#define NF_NX   41
#define NF_NY   41
#define NF_X_MIN   -0.5
#define NF_X_MAX    0.5
#define NF_Y_MIN   -0.5
#define NF_Y_MAX    0.5
#define NF_Z0       0.1

static void print_usage(const char* program_name) {
    printf("MoM Solver - Method of Moments Electromagnetic Simulation\n");
    printf("Version 1.0.0\n");
    printf("\nUsage: %s <config_file> [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help     Show this help message\n");
    printf("  -v, --version  Show version information\n");
    printf("\nConfig File Format:\n");
    printf("  geometry_file=<path>\n");
    printf("  geometry_format=step|stl|iges|msh  (msh = direct Gmsh mesh file)\n");
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
    mom_cli_file_config_t file_config = {0};
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
    printf("[1/6] Creating MoM solver configuration...\n");
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
    printf("[2/6] Creating MoM solver...\n");
    mom_solver_t* solver = mom_solver_create(&solver_config);
    if (!solver) {
        fprintf(stderr, "Error: Failed to create MoM solver\n");
        return 1;
    }

    /* 设置 45° 入射平面波激励：k 向量在 x-z 平面，与 +z 法向成 45°，极化沿 y 方向 */
    {
        mom_excitation_t exc = {0};
        const double inv_sqrt2 = 0.7071067811865475; /* 1/sqrt(2) */
        exc.type      = MOM_EXCITATION_PLANE_WAVE;
        exc.frequency = solver_config.frequency;
        exc.amplitude = 1.0;
        exc.phase     = 0.0;
        /* 传播方向：从上方斜向车体，假定车体法向为 +z，这里取朝 -z 方向 */
        exc.k_vector.x =  inv_sqrt2;
        exc.k_vector.y =  0.0;
        exc.k_vector.z = -inv_sqrt2;
        /* 极化方向：E 沿 y 轴 */
        exc.polarization.x = 0.0;
        exc.polarization.y = 1.0;
        exc.polarization.z = 0.0;
        mom_solver_add_excitation(solver, &exc);
    }
    
    /* Load geometry: .msh = direct mesh import; else CAD (STEP/IGES) + Gmsh or fallback */
    printf("[3/6] Loading geometry from %s...\n", file_config.geometry_file);
    {
        const char* path = file_config.geometry_file;
        size_t len = path ? strlen(path) : 0;
        int use_msh = (file_config.geometry_format[0] == 'm' && file_config.geometry_format[1] == 's' && file_config.geometry_format[2] == 'h')
                      || (len >= 4 && path[len-4] == '.' && (path[len-3] == 'm' || path[len-3] == 'M') && (path[len-2] == 's' || path[len-2] == 'S') && (path[len-1] == 'h' || path[len-1] == 'H'));
        if (use_msh) {
            if (mom_solver_import_msh(solver, file_config.geometry_file) != STATUS_SUCCESS) {
                fprintf(stderr, "Error: Failed to load .msh file: %s\n", file_config.geometry_file);
                mom_solver_destroy(solver);
                return 1;
            }
        } else {
            if (mom_solver_import_cad(solver, file_config.geometry_file, file_config.geometry_format) != STATUS_SUCCESS) {
                fprintf(stderr, "Error: Failed to load geometry file: %s\n", file_config.geometry_file);
                mom_solver_destroy(solver);
                return 1;
            }
        }
    }
    
    // 装配 MoM 系统矩阵
    printf("[4/6] Assembling MoM system matrix...\n");
    if (mom_solver_assemble_matrix(solver) != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to assemble MoM system matrix\n");
        mom_solver_destroy(solver);
        return 1;
    }
    
    // 运行仿真
    printf("[5/6] Running simulation...\n");
    {
        int solve_status = mom_solver_solve(solver);
        if (solve_status != STATUS_SUCCESS) {
            int num_unk = mom_solver_get_num_unknowns(solver);
            fprintf(stderr, "Error: Simulation failed (code %d).\n", solve_status);
            fprintf(stderr, "  Dense MoM stores a full complex Z matrix: about 16*N^2 bytes (N=unknowns).\n");
            fprintf(stderr, "  With %d unknowns Z alone is about %.2f GB; LAPACK path may briefly need a second N^2 buffer.\n",
                    num_unk,
                    (double)((size_t)num_unk * (size_t)num_unk * 16) / (1024.0*1024.0*1024.0));
            fprintf(stderr, "  Try a coarser .msh, or enable iterative/ACA paths if your build supports them.\n");
            mom_solver_destroy(solver);
            return 1;
        }
    }
    
    // 获取结果
    printf("[6/6] Retrieving results...\n");
    const mom_result_t* results = mom_solver_get_results(solver);
    if (!results) {
        fprintf(stderr, "Error: Failed to get results\n");
        mom_solver_destroy(solver);
        return 1;
    }

    /* 近场采样：在 z=NF_Z0 平面上建立 NF_NX × NF_NY 网格，用于 3D 场强分布演示 */
    {
        const int num_near = NF_NX * NF_NY;
        point3d_t near_pts[NF_NX * NF_NY];
        const double dx = (NF_X_MAX - NF_X_MIN) / (double)(NF_NX - 1);
        const double dy = (NF_Y_MAX - NF_Y_MIN) / (double)(NF_NY - 1);
        int idx = 0;
        for (int iy = 0; iy < NF_NY; ++iy) {
            double y = NF_Y_MIN + iy * dy;
            for (int ix = 0; ix < NF_NX; ++ix) {
                double x = NF_X_MIN + ix * dx;
                near_pts[idx].x = x;
                near_pts[idx].y = y;
                near_pts[idx].z = NF_Z0;
                ++idx;
            }
        }
        mom_solver_compute_near_field(solver, near_pts, num_near);
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

    /* Write a simple text results file next to the geometry file */
    char results_path[1024] = {0};
    build_results_path(file_config.geometry_file, results_path, sizeof(results_path));
    if (results_path[0] != '\0') {
        FILE* rf = fopen(results_path, "w");
        if (rf) {
            fprintf(rf, "MoM Simulation Results\n");
            fprintf(rf, "Geometry file: %s\n", file_config.geometry_file);
            fprintf(rf, "Geometry format: %s\n", file_config.geometry_format);
            fprintf(rf, "Frequency: %.6e Hz\n", file_config.frequency);
            fprintf(rf, "Tolerance: %.6e\n", file_config.tolerance);
            fprintf(rf, "Max iterations: %d\n", file_config.max_iterations);
            fprintf(rf, "Mesh density: %d elements/wavelength\n", file_config.mesh_density);
            fprintf(rf, "Surface mesh: %d triangles (this determines 3D surface current plot points)\n", results->num_basis_functions);
            fprintf(rf, "\n");
            fprintf(rf, "Basis functions: %d\n", results->num_basis_functions);
            if (results->matrix_fill_time > 0) {
                fprintf(rf, "Matrix fill time: %.6f seconds\n", results->matrix_fill_time);
            }
            if (results->solve_time > 0) {
                fprintf(rf, "Solve time: %.6f seconds\n", results->solve_time);
            }
            if (results->iterations > 0) {
                fprintf(rf, "Iterations: %d\n", results->iterations);
                fprintf(rf, "Converged: %s\n", results->converged ? "Yes" : "No");
            }
            fprintf(rf, "\n");

            /* Current coefficients for each basis function */
            if (results->current_coefficients && results->num_basis_functions > 0) {
                const double RAD2DEG = 180.0 / 3.14159265358979323846;
                fprintf(rf, "Current distribution (per basis function):\n");
                fprintf(rf, "Index, Re(J), Im(J), |J|, Phase_deg\n");
                for (int i = 0; i < results->num_basis_functions; ++i) {
                    mom_scalar_complex_t J = results->current_coefficients[i];
#if defined(_MSC_VER)
                    double re = J.re;
                    double im = J.im;
#else
                    double re = creal(J);
                    double im = cimag(J);
#endif
                    double mag = sqrt(re * re + im * im);
                    double phase_deg = atan2(im, re) * RAD2DEG;
                    fprintf(rf, "%d, %.12e, %.12e, %.12e, %.6f\n",
                            i, re, im, mag, phase_deg);
                }
            }

            /* Near field: point coordinates + E/H (V/m, A/m) */
            if (results->near_field.num_points > 0 && results->near_field.e_field && results->near_field.h_field) {
                fprintf(rf, "\nNear field (point coordinates and E, H):\n");
                fprintf(rf, "PointIndex, x, y, z, Ex_re, Ex_im, Ey_re, Ey_im, Ez_re, Ez_im, Hx_re, Hx_im, Hy_re, Hy_im, Hz_re, Hz_im, |E|, |H|\n");
                for (int i = 0; i < results->near_field.num_points; i++) {
                    double exr = results->near_field.e_field[i*3+0].re, exi = results->near_field.e_field[i*3+0].im;
                    double eyr = results->near_field.e_field[i*3+1].re, eyi = results->near_field.e_field[i*3+1].im;
                    double ezr = results->near_field.e_field[i*3+2].re, ezi = results->near_field.e_field[i*3+2].im;
                    double hxr = results->near_field.h_field[i*3+0].re, hxi = results->near_field.h_field[i*3+0].im;
                    double hyr = results->near_field.h_field[i*3+1].re, hyi = results->near_field.h_field[i*3+1].im;
                    double hzr = results->near_field.h_field[i*3+2].re, hzi = results->near_field.h_field[i*3+2].im;
                    double absE = sqrt(exr*exr + exi*exi + eyr*eyr + eyi*eyi + ezr*ezr + ezi*ezi);
                    double absH = sqrt(hxr*hxr + hxi*hxi + hyr*hyr + hyi*hyi + hzr*hzr + hzi*hzi);
                    /* 2D 采样平面上的网格点坐标，与上方 near_pts 生成方式一致 */
                    int ix = i % NF_NX;
                    int iy = i / NF_NX;
                    double dx = (NF_X_MAX - NF_X_MIN) / (double)(NF_NX - 1);
                    double dy = (NF_Y_MAX - NF_Y_MIN) / (double)(NF_NY - 1);
                    double px = NF_X_MIN + ix * dx;
                    double py = NF_Y_MIN + iy * dy;
                    double pz = NF_Z0;
                    fprintf(rf, "%d, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e, %.6e\n",
                            i, px, py, pz, exr, exi, eyr, eyi, ezr, ezi, hxr, hxi, hyr, hyi, hzr, hzi, absE, absH);
                }
            }
            fclose(rf);
            printf("Results written to: %s\n", results_path);

            /* 表面电流绘图用 CSV：与 step 同级目录（仅幅度 vs 索引） */
            char out_dir[1024] = {0};
            get_output_dir(file_config.geometry_file, out_dir, sizeof(out_dir));
            if (out_dir[0] != '\0' && results->current_coefficients && results->num_basis_functions > 0) {
                char csv_path[1024];
                snprintf(csv_path, sizeof(csv_path), "%scurrent_plot.csv", out_dir);
                FILE* cf = fopen(csv_path, "w");
                if (cf) {
                    const double RAD2DEG = 180.0 / 3.14159265358979323846;
                    fprintf(cf, "basis_index,Re_J,Im_J,magnitude_J,phase_deg\n");
                    for (int i = 0; i < results->num_basis_functions; i++) {
                        mom_scalar_complex_t J = results->current_coefficients[i];
#if defined(_MSC_VER)
                        double re = J.re, im = J.im;
#else
                        double re = creal(J), im = cimag(J);
#endif
                        double mag = sqrt(re*re + im*im);
                        double ph = atan2(im, re) * RAD2DEG;
                        fprintf(cf, "%d,%.12e,%.12e,%.12e,%.6f\n", i, re, im, mag, ph);
                    }
                    fclose(cf);
                }

                /* 带几何信息的表面电流导出：每个单元质心 + 电流，用于 3D 可视化 */
                char surf_path[1024];
                snprintf(surf_path, sizeof(surf_path), "%ssurface_current.csv", out_dir);
                if (mom_solver_export_surface_current(solver, surf_path) == 0) {
                    printf("Surface current (with geometry) written to: %s\n", surf_path);
                }
                /* 三角网格 + 电流，用于 3D 车体表面图（与车形一致） */
                char mesh_plot_path[1024];
                snprintf(mesh_plot_path, sizeof(mesh_plot_path), "%ssurface_mesh_plot.txt", out_dir);
                if (mom_solver_export_surface_mesh_for_plot(solver, mesh_plot_path) == 0) {
                    printf("Surface mesh for 3D plot written to: %s\n", mesh_plot_path);
                }
                /* VTK 表面电流：连续三角面 + CELL_DATA，用 ParaView 打开可得连续着色 */
                {
                    char vtk_path[1024];
                    snprintf(vtk_path, sizeof(vtk_path), "%ssurface_current.vtk", out_dir);
                    if (mom_solver_export_surface_current_vtk(solver, vtk_path, file_config.conductor_material_id) == 0)
                        printf("Surface current VTK (continuous mesh) written to: %s\n", vtk_path);
                }
            }

            /* 近场绘图用 CSV：点坐标 + |E|, |H| */
            if (out_dir[0] != '\0' && results->near_field.num_points > 0 && results->near_field.e_field && results->near_field.h_field) {
                char nf_path[1024];
                snprintf(nf_path, sizeof(nf_path), "%snear_field_plot.csv", out_dir);
                FILE* nf = fopen(nf_path, "w");
                if (nf) {
                    fprintf(nf, "point_index,x,y,z,abs_E,abs_H\n");
                    for (int i = 0; i < results->near_field.num_points; i++) {
                        double exr = results->near_field.e_field[i*3+0].re, exi = results->near_field.e_field[i*3+0].im;
                        double eyr = results->near_field.e_field[i*3+1].re, eyi = results->near_field.e_field[i*3+1].im;
                        double ezr = results->near_field.e_field[i*3+2].re, ezi = results->near_field.e_field[i*3+2].im;
                        double hxr = results->near_field.h_field[i*3+0].re, hxi = results->near_field.h_field[i*3+0].im;
                        double hyr = results->near_field.h_field[i*3+1].re, hyi = results->near_field.h_field[i*3+1].im;
                        double hzr = results->near_field.h_field[i*3+2].re, hzi = results->near_field.h_field[i*3+2].im;
                        double absE = sqrt(exr*exr + exi*exi + eyr*eyr + eyi*eyi + ezr*ezr + ezi*ezi);
                        double absH = sqrt(hxr*hxr + hxi*hxi + hyr*hyr + hyi*hyi + hzr*hzr + hzi*hzi);
                        int ix = i % NF_NX;
                        int iy = i / NF_NX;
                        double dx = (NF_X_MAX - NF_X_MIN) / (double)(NF_NX - 1);
                        double dy = (NF_Y_MAX - NF_Y_MIN) / (double)(NF_NY - 1);
                        double px = NF_X_MIN + ix * dx;
                        double py = NF_Y_MIN + iy * dy;
                        double pz = NF_Z0;
                        fprintf(nf, "%d,%.6e,%.6e,%.6e,%.6e,%.6e\n", i, px, py, pz, absE, absH);
                    }
                    fclose(nf);
                }
            }

            /* 提示：可用 Python 脚本在同一目录下生成曲线图 */
            if (out_dir[0] != '\0') {
                printf("To generate plots in %s run:\n  python scripts/plot_results.py \"%s\"\n",
                       out_dir, out_dir);
            }
        } else {
            fprintf(stderr, "Warning: Failed to open results file for writing: %s\n",
                    results_path);
        }
    }
    
    // 清理
    mom_solver_destroy(solver);
    
    printf("Done.\n");
    return 0;
}
