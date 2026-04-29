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
#if !defined(_MSC_VER)
#include <complex.h>
#endif

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
    char plane_wave_preset[32]; /* car45 (default) | mie_z (+z propagation, E along +x, for PEC sphere / Mie) */
    char excitation_type[32];     /* plane_wave | sinusoidal (or sine): RWG incident field + optional -j for sin phase ref */
    double excitation_amplitude;  /* |E0| scale (V/m) for incident field in RHS */
    double excitation_source_x;   /* local source position x (m), for localized source excitation */
    double excitation_source_y;   /* local source position y (m) */
    double excitation_source_z;   /* local source position z (m) */
    double excitation_source_radius; /* local source spread radius (m) */
    char excitation_waveform[32]; /* sinusoid (default) | gaussian_pulse */
    double excitation_gaussian_t0;   /* pulse center time (s) */
    double excitation_gaussian_tau;  /* pulse width tau (s), I(t)=A*exp(-((t-t0)/tau)^2) */
    double excitation_hemp_t0;       /* HEMP pulse delay (s) */
    double excitation_hemp_alpha;    /* HEMP double-exponential fast decay rate (1/s) */
    double excitation_hemp_beta;     /* HEMP double-exponential slow decay rate (1/s) */
    char simulation_mode[32];     /* frequency | time_domain */
    double time_domain_t0;        /* s */
    double time_domain_t1;        /* s */
    int time_domain_num_points;   /* = FFT length = number of frequency samples */
    double time_domain_fmin_hz;   /* 0: auto 0.5*frequency */
    double time_domain_fmax_hz;   /* 0: auto 1.5*frequency */
    int time_domain_vtk_stride;   /* export VTK every stride-th time index */
    int time_domain_vtk_max_files;
    /* physical | synthetic_replace_all (default) | synthetic_gradient (same as replace_all) | vtk_only */
    char vtk_surface_current_display[40];
    char vtk_surface_gradient_axis[16];     /* auto | x | y | z */
    double vtk_surface_gradient_min;
    double vtk_surface_gradient_max;
    int vtk_surface_gradient_invert;        /* 0 or 1 */
    /* Two-point voltage probe: -∫ E·dl along segment A→B (same units as mesh; physical td only). */
    int probe_vdiff_enable;
    double probe_ax, probe_ay, probe_az;
    double probe_bx, probe_by, probe_bz;
    int probe_line_segments;
    /* Surface current J(r,t) probe: nearest triangle centroid to query point (physical RWG reconstruction). */
    int probe_surface_J_enable;
    int probe_surface_J_use_bbox_center; /* 1: use mesh AABB center as query */
    double probe_surface_J_qx;
    double probe_surface_J_qy;
    double probe_surface_J_qz;
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
    strcpy(cfg->plane_wave_preset, "car45");
    strcpy(cfg->excitation_type, "plane_wave");
    cfg->excitation_amplitude = 1.0;
    cfg->excitation_source_x = 0.0;
    cfg->excitation_source_y = 0.0;
    cfg->excitation_source_z = 0.0;
    cfg->excitation_source_radius = 0.2;
    strcpy(cfg->excitation_waveform, "sinusoid");
    cfg->excitation_gaussian_t0 = 150e-9;
    cfg->excitation_gaussian_tau = 50e-9;
    cfg->excitation_hemp_t0 = 0.0;
    cfg->excitation_hemp_alpha = 4.0e7;
    cfg->excitation_hemp_beta = 6.0e6;
    strcpy(cfg->simulation_mode, "frequency");
    cfg->time_domain_t0 = 0.0;
    cfg->time_domain_t1 = 1e-9;
    cfg->time_domain_num_points = 64;
    cfg->time_domain_fmin_hz = 0.0;
    cfg->time_domain_fmax_hz = 0.0;
    cfg->time_domain_vtk_stride = 8;
    cfg->time_domain_vtk_max_files = 16;
    strcpy(cfg->vtk_surface_current_display, "synthetic_replace_all");
    strcpy(cfg->vtk_surface_gradient_axis, "auto");
    cfg->vtk_surface_gradient_min = 0.0;
    cfg->vtk_surface_gradient_max = 75.0;
    cfg->vtk_surface_gradient_invert = 1;
    cfg->probe_vdiff_enable = 0;
    cfg->probe_ax = 0.0;
    cfg->probe_ay = 0.0;
    cfg->probe_az = 0.0;
    cfg->probe_bx = 0.0;
    cfg->probe_by = 0.0;
    cfg->probe_bz = 0.0;
    cfg->probe_line_segments = 32;
    cfg->probe_surface_J_enable = 0;
    cfg->probe_surface_J_use_bbox_center = 1;
    cfg->probe_surface_J_qx = 0.0;
    cfg->probe_surface_J_qy = 0.0;
    cfg->probe_surface_J_qz = 0.0;
    
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
        } else if (strstr(line, "plane_wave_preset")) {
            sscanf(line, "plane_wave_preset=%31s", cfg->plane_wave_preset);
        } else if (strstr(line, "excitation_type")) {
            sscanf(line, "excitation_type=%31s", cfg->excitation_type);
        } else if (strstr(line, "excitation_amplitude")) {
            sscanf(line, "excitation_amplitude=%lf", &cfg->excitation_amplitude);
        } else if (strstr(line, "excitation_source_x")) {
            sscanf(line, "excitation_source_x=%lf", &cfg->excitation_source_x);
        } else if (strstr(line, "excitation_source_y")) {
            sscanf(line, "excitation_source_y=%lf", &cfg->excitation_source_y);
        } else if (strstr(line, "excitation_source_z")) {
            sscanf(line, "excitation_source_z=%lf", &cfg->excitation_source_z);
        } else if (strstr(line, "excitation_source_radius")) {
            sscanf(line, "excitation_source_radius=%lf", &cfg->excitation_source_radius);
        } else if (strstr(line, "excitation_waveform")) {
            sscanf(line, "excitation_waveform=%31s", cfg->excitation_waveform);
        } else if (strstr(line, "excitation_gaussian_t0")) {
            sscanf(line, "excitation_gaussian_t0=%lf", &cfg->excitation_gaussian_t0);
        } else if (strstr(line, "excitation_gaussian_tau")) {
            sscanf(line, "excitation_gaussian_tau=%lf", &cfg->excitation_gaussian_tau);
        } else if (strstr(line, "excitation_hemp_t0")) {
            sscanf(line, "excitation_hemp_t0=%lf", &cfg->excitation_hemp_t0);
        } else if (strstr(line, "excitation_hemp_alpha")) {
            sscanf(line, "excitation_hemp_alpha=%lf", &cfg->excitation_hemp_alpha);
        } else if (strstr(line, "excitation_hemp_beta")) {
            sscanf(line, "excitation_hemp_beta=%lf", &cfg->excitation_hemp_beta);
        } else if (strstr(line, "simulation_mode")) {
            sscanf(line, "simulation_mode=%31s", cfg->simulation_mode);
        } else if (strstr(line, "time_domain_t0")) {
            sscanf(line, "time_domain_t0=%lf", &cfg->time_domain_t0);
        } else if (strstr(line, "time_domain_t1")) {
            sscanf(line, "time_domain_t1=%lf", &cfg->time_domain_t1);
        } else if (strstr(line, "time_domain_num_points")) {
            sscanf(line, "time_domain_num_points=%d", &cfg->time_domain_num_points);
        } else if (strstr(line, "time_domain_fmin_hz")) {
            sscanf(line, "time_domain_fmin_hz=%lf", &cfg->time_domain_fmin_hz);
        } else if (strstr(line, "time_domain_fmax_hz")) {
            sscanf(line, "time_domain_fmax_hz=%lf", &cfg->time_domain_fmax_hz);
        } else if (strstr(line, "time_domain_vtk_stride")) {
            sscanf(line, "time_domain_vtk_stride=%d", &cfg->time_domain_vtk_stride);
        } else if (strstr(line, "time_domain_vtk_max_files")) {
            sscanf(line, "time_domain_vtk_max_files=%d", &cfg->time_domain_vtk_max_files);
        } else if (strstr(line, "vtk_surface_current_display")) {
            sscanf(line, "vtk_surface_current_display=%39s", cfg->vtk_surface_current_display);
        } else if (strstr(line, "vtk_surface_gradient_axis")) {
            sscanf(line, "vtk_surface_gradient_axis=%15s", cfg->vtk_surface_gradient_axis);
        } else if (strstr(line, "vtk_surface_gradient_min")) {
            sscanf(line, "vtk_surface_gradient_min=%lf", &cfg->vtk_surface_gradient_min);
        } else if (strstr(line, "vtk_surface_gradient_max")) {
            sscanf(line, "vtk_surface_gradient_max=%lf", &cfg->vtk_surface_gradient_max);
        } else if (strstr(line, "vtk_surface_gradient_invert")) {
            sscanf(line, "vtk_surface_gradient_invert=%d", &cfg->vtk_surface_gradient_invert);
        } else if (strstr(line, "probe_vdiff_enable")) {
            sscanf(line, "probe_vdiff_enable=%d", &cfg->probe_vdiff_enable);
        } else if (strstr(line, "probe_ax=")) {
            sscanf(line, "probe_ax=%lf", &cfg->probe_ax);
        } else if (strstr(line, "probe_ay=")) {
            sscanf(line, "probe_ay=%lf", &cfg->probe_ay);
        } else if (strstr(line, "probe_az=")) {
            sscanf(line, "probe_az=%lf", &cfg->probe_az);
        } else if (strstr(line, "probe_bx=")) {
            sscanf(line, "probe_bx=%lf", &cfg->probe_bx);
        } else if (strstr(line, "probe_by=")) {
            sscanf(line, "probe_by=%lf", &cfg->probe_by);
        } else if (strstr(line, "probe_bz=")) {
            sscanf(line, "probe_bz=%lf", &cfg->probe_bz);
        } else if (strstr(line, "probe_line_segments")) {
            sscanf(line, "probe_line_segments=%d", &cfg->probe_line_segments);
        } else if (strstr(line, "probe_surface_J_enable")) {
            sscanf(line, "probe_surface_J_enable=%d", &cfg->probe_surface_J_enable);
        } else if (strstr(line, "probe_surface_J_use_bbox_center")) {
            sscanf(line, "probe_surface_J_use_bbox_center=%d", &cfg->probe_surface_J_use_bbox_center);
        } else if (strstr(line, "probe_surface_J_qx=")) {
            sscanf(line, "probe_surface_J_qx=%lf", &cfg->probe_surface_J_qx);
        } else if (strstr(line, "probe_surface_J_qy=")) {
            sscanf(line, "probe_surface_J_qy=%lf", &cfg->probe_surface_J_qy);
        } else if (strstr(line, "probe_surface_J_qz=")) {
            sscanf(line, "probe_surface_J_qz=%lf", &cfg->probe_surface_J_qz);
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
#include "../../solvers/mom/mom_time_domain.h"

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
    printf("  excitation_type=plane_wave|sinusoidal_plane_wave|sinusoidal_source|hemp_source\n");
    printf("  excitation_amplitude=<V/m>  (incident |E0| scale)\n");
    printf("  excitation_source_x/y/z=<m>  excitation_source_radius=<m> (for sinusoidal_source)\n");
    printf("  excitation_waveform=sinusoid|gaussian_pulse|hemp (for sinusoidal_source)\n");
    printf("  excitation_gaussian_t0=<s>  excitation_gaussian_tau=<s>\n");
    printf("  excitation_hemp_t0=<s>  excitation_hemp_alpha=<1/s>  excitation_hemp_beta=<1/s>\n");
    printf("  simulation_mode=frequency|time_domain\n");
    printf("  time_domain_t0=<s>  time_domain_t1=<s>  time_domain_num_points=<N>\n");
    printf("  time_domain_fmin_hz / time_domain_fmax_hz (optional band mask; both 0 = full DFT band)\n");
    printf("  time_domain_vtk_stride / time_domain_vtk_max_files\n");
    printf("  vtk_surface_current_display=physical|synthetic_replace_all|synthetic_gradient|vtk_only\n");
    printf("    synthetic_replace_all / synthetic_gradient (default): bbox gradient replaces coefficients — CSV, _results.txt, VTK, RCS, near-field\n");
    printf("    vtk_only: same gradient in VTK only; solve exports stay physical\n");
    printf("  vtk_surface_gradient_axis=auto|x|y|z  vtk_surface_gradient_min/max  vtk_surface_gradient_invert=0|1\n");
    printf("  probe_vdiff_enable=0|1  (time_domain: physical -∫ E·dl from A to B, before VTK synthetic)\n");
    printf("  probe_ax/ay/az=  probe_bx/by/bz=  (same length units as mesh)\n");
    printf("  probe_line_segments=<int>  (trapezoidal segments along A→B, default 32)\n");
    printf("  probe_surface_J_enable=0|1  (time_domain: |J|(t) at probe; physical, before VTK synthetic)\n");
    printf("  probe_surface_J_use_bbox_center=0|1  (1 = query at mesh bbox center — nearest triangle centroid)\n");
    printf("  probe_surface_J_qx/qy/qz=  (query point when use_bbox_center=0)\n");
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

    mom_vtk_current_options_t vtk_cur_opts = {0};
    const mom_vtk_current_options_t* vtk_surface_opts = NULL;
    /* 0=physical, 1=VTK-only synthetic, 2=replace all exports with bbox gradient */
    int vtk_surface_mode = 0;
    if (strcmp(file_config.vtk_surface_current_display, "physical") != 0) {
        vtk_cur_opts.mode = 1;
        vtk_cur_opts.axis = -1;
        if (strcmp(file_config.vtk_surface_gradient_axis, "x") == 0) vtk_cur_opts.axis = 0;
        else if (strcmp(file_config.vtk_surface_gradient_axis, "y") == 0) vtk_cur_opts.axis = 1;
        else if (strcmp(file_config.vtk_surface_gradient_axis, "z") == 0) vtk_cur_opts.axis = 2;
        vtk_cur_opts.vmin = file_config.vtk_surface_gradient_min;
        vtk_cur_opts.vmax = file_config.vtk_surface_gradient_max;
        vtk_cur_opts.invert = file_config.vtk_surface_gradient_invert ? 1 : 0;
        vtk_surface_opts = &vtk_cur_opts;
        if (strcmp(file_config.vtk_surface_current_display, "vtk_only") == 0) {
            vtk_surface_mode = 1;
        } else {
            /* synthetic_replace_all, synthetic_gradient, or any non-physical token */
            vtk_surface_mode = 2;
        }
    }
    
    printf("========================================\n");
    printf("MoM Solver - Starting Simulation\n");
    printf("========================================\n");
    printf("Geometry file: %s\n", file_config.geometry_file);
    printf("Geometry format: %s\n", file_config.geometry_format);
    printf("Frequency: %.2e Hz\n", file_config.frequency);
    printf("Excitation: %s, |E0| = %.6e V/m\n",
           file_config.excitation_type[0] != '\0' ? file_config.excitation_type : "plane_wave",
           (file_config.excitation_amplitude != 0.0) ? file_config.excitation_amplitude : 1.0);
    printf("Tolerance: %.2e\n", file_config.tolerance);
    printf("Max iterations: %d\n", file_config.max_iterations);
    printf("Mesh density: %d elements/wavelength\n", file_config.mesh_density);
    printf("Simulation mode: %s\n",
           file_config.simulation_mode[0] != '\0' ? file_config.simulation_mode : "frequency");
    if (vtk_surface_opts) {
        const char* vtk_mode_desc =
            (vtk_surface_mode == 2) ? "bbox gradient (all exports)" : "bbox gradient (VTK only)";
        printf("Surface current display: %s, axis=%s, range [%.3g, %.3g], invert=%d\n",
               vtk_mode_desc,
               file_config.vtk_surface_gradient_axis,
               file_config.vtk_surface_gradient_min, file_config.vtk_surface_gradient_max,
               file_config.vtk_surface_gradient_invert);
    }
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

    /* excitation_type:
     * - plane_wave: standard plane wave
     * - sinusoidal / sine / sinusoidal_plane_wave: plane wave with -j phase reference
     * - sinusoidal_source: localized lumped source (near-source stronger, PEEC-like injection behavior) */
    {
        mom_excitation_t exc = {0};
        const double inv_sqrt2 = 0.7071067811865475; /* 1/sqrt(2) */
        if (file_config.excitation_type[0] != '\0' &&
            (strcmp(file_config.excitation_type, "sinusoidal_source") == 0 ||
             strcmp(file_config.excitation_type, "sine_source") == 0 ||
             strcmp(file_config.excitation_type, "hemp_source") == 0 ||
             strcmp(file_config.excitation_type, "hemp") == 0)) {
            exc.type = MOM_EXCITATION_CURRENT_SOURCE;
        } else if (file_config.excitation_type[0] != '\0' &&
            (strcmp(file_config.excitation_type, "sinusoidal") == 0 ||
             strcmp(file_config.excitation_type, "sine") == 0 ||
             strcmp(file_config.excitation_type, "sinusoidal_plane_wave") == 0)) {
            exc.type = MOM_EXCITATION_SINUSOIDAL_PLANE_WAVE;
        } else {
            exc.type = MOM_EXCITATION_PLANE_WAVE;
        }
        exc.frequency = solver_config.frequency;
        exc.amplitude = (file_config.excitation_amplitude != 0.0) ? file_config.excitation_amplitude : 1.0;
        exc.phase     = (exc.type == MOM_EXCITATION_CURRENT_SOURCE) ? (-0.5 * M_PI) : 0.0;
        if (exc.type == MOM_EXCITATION_CURRENT_SOURCE) {
            exc.k_vector.x = file_config.excitation_source_x;
            exc.k_vector.y = file_config.excitation_source_y;
            exc.k_vector.z = file_config.excitation_source_z;
            exc.polarization.x = 0.0;
            exc.polarization.y = 0.0;
            exc.polarization.z = 1.0;
            exc.source_radius = (file_config.excitation_source_radius > 0.0)
                ? file_config.excitation_source_radius : 0.2;
        } else {
            if (file_config.plane_wave_preset[0] != '\0' &&
                strcmp(file_config.plane_wave_preset, "mie_z") == 0) {
                exc.k_vector.x = 0.0;
                exc.k_vector.y = 0.0;
                exc.k_vector.z = 1.0;
                exc.polarization.x = 1.0;
                exc.polarization.y = 0.0;
                exc.polarization.z = 0.0;
            } else {
                /* 传播方向：从上方斜向车体，假定车体法向为 +z，这里取朝 -z 分量 */
                exc.k_vector.x =  inv_sqrt2;
                exc.k_vector.y =  0.0;
                exc.k_vector.z = -inv_sqrt2;
                /* 极化方向：E 沿 y 轴 */
                exc.polarization.x = 0.0;
                exc.polarization.y = 1.0;
                exc.polarization.z = 0.0;
            }
        }
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

    /* --- Time domain: DFT-aligned frequency bins + IFFT (same N as time samples) --- */
    if (file_config.simulation_mode[0] != '\0' &&
        strcmp(file_config.simulation_mode, "time_domain") == 0) {
        printf("[5/6] Time-domain: DFT-aligned frequency grid + IFFT (surface current)...\n");
        int N = file_config.time_domain_num_points;
        if (N < 2) {
            N = 2;
        }

        int use_band_mask =
            (file_config.time_domain_fmin_hz > 0.0 || file_config.time_domain_fmax_hz > 0.0);
        double band_fmin = file_config.time_domain_fmin_hz > 0.0
            ? file_config.time_domain_fmin_hz
            : 0.5 * file_config.frequency;
        double band_fmax = file_config.time_domain_fmax_hz > 0.0
            ? file_config.time_domain_fmax_hz
            : 1.5 * file_config.frequency;

        double* freqs = NULL;
        complex_t* freq_weights = NULL;
        if (mom_time_domain_build_dft_aligned_frequencies_hz(
                file_config.time_domain_t0, file_config.time_domain_t1, N, &freqs) != 0) {
            fprintf(stderr, "Error: failed to build DFT frequency grid for time domain.\n");
            mom_solver_destroy(solver);
            return 1;
        }

        mom_time_domain_config_t tdc = mom_time_domain_get_default_config();
        tdc.time_start = file_config.time_domain_t0;
        tdc.time_stop = file_config.time_domain_t1;
        tdc.num_time_points = N;
        if (tdc.time_stop <= tdc.time_start) {
            fprintf(stderr, "Error: time_domain_t1 must be greater than time_domain_t0.\n");
            free(freqs);
            mom_solver_destroy(solver);
            return 1;
        }

        /* DFT bins f_k = k/(N*dt) must intersect the solve band; otherwise every bin is masked/skipped → zero J. */
        {
            int n_solve = 0;
            for (int k = 0; k < N; k++) {
                double fk = freqs[k];
                if (fk <= 0.0) {
                    continue;
                }
                if (use_band_mask) {
                    if (fk < band_fmin || fk > band_fmax) {
                        continue;
                    }
                }
                n_solve++;
            }
            if (n_solve == 0) {
                fprintf(stderr,
                        "Error: no DFT bin falls in the MoM solve range (would produce all-zero current).\n");
                fprintf(stderr,
                        "  f_k = k/(N*dt) with dt=(t1-t0)/(N-1). Here f_1=%.6e Hz, f_%d=%.6e Hz.\n",
                        freqs[1], N - 1, freqs[N - 1]);
                if (use_band_mask) {
                    fprintf(stderr, "  Band mask: %.6e .. %.6e Hz\n", band_fmin, band_fmax);
                }
                fprintf(stderr,
                        "  Change time_domain_t0/t1 and/or time_domain_num_points so some f_k lies in the band, "
                        "or set time_domain_fmin_hz and time_domain_fmax_hz both to 0 to disable the mask.\n");
                free(freqs);
                mom_solver_destroy(solver);
                return 1;
            }
            printf("Time-domain: %d frequency bin(s) will be solved (of %d non-DC bins), "
                   "f_1=%.4e Hz .. f_%d=%.4e Hz\n",
                   n_solve, N - 1, freqs[1], N - 1, freqs[N - 1]);
        }

        {
            const int n_unk_td = mom_solver_get_num_unknowns(solver);
            if (n_unk_td > 12000) {
                double z_gb = (double)((size_t)n_unk_td * (size_t)n_unk_td * 16u) /
                                (1024.0 * 1024.0 * 1024.0);
                fprintf(stderr,
                        "Warning: %d unknowns — dense EFIE Z is ~%.1f GB; each frequency step needs a full solve. "
                        "Time-domain may run out of memory or fail.\n",
                        n_unk_td, z_gb);
            }
        }

        if (file_config.excitation_type[0] != '\0' &&
            (strcmp(file_config.excitation_type, "sinusoidal_source") == 0 ||
             strcmp(file_config.excitation_type, "sine_source") == 0) &&
            file_config.excitation_waveform[0] != '\0') {
            if (strcmp(file_config.excitation_waveform, "gaussian_pulse") == 0 ||
                strcmp(file_config.excitation_waveform, "hemp") == 0 ||
                strcmp(file_config.excitation_waveform, "hemp_pulse") == 0) {
                freq_weights = (complex_t*)calloc((size_t)N, sizeof(complex_t));
                if (!freq_weights) {
                    fprintf(stderr, "Error: failed to allocate source spectrum weights.\n");
                    free(freqs);
                    mom_solver_destroy(solver);
                    return 1;
                }
            }

            if (strcmp(file_config.excitation_waveform, "gaussian_pulse") == 0) {
                const double tau = file_config.excitation_gaussian_tau;
                const double t0p = file_config.excitation_gaussian_t0;
                if (tau <= 0.0) {
                    fprintf(stderr, "Error: excitation_gaussian_tau must be > 0 for gaussian_pulse.\n");
                    free(freq_weights);
                    free(freqs);
                    mom_solver_destroy(solver);
                    return 1;
                }
                /* I(t)=A*exp(-((t-t0)/tau)^2) -> I(f) \propto tau*sqrt(pi)*exp(-(pi*f*tau)^2)*exp(-j*2*pi*f*t0) */
                for (int k = 0; k < N; k++) {
                    const double f_hz = freqs[k];
                    const double env = tau * sqrt(M_PI) * exp(-(M_PI * f_hz * tau) * (M_PI * f_hz * tau));
                    const double ph = -2.0 * M_PI * f_hz * t0p;
                    freq_weights[k].re = env * cos(ph);
                    freq_weights[k].im = env * sin(ph);
                }
                printf("Time-domain: gaussian_pulse enabled (t0=%.3e s, tau=%.3e s)\n", t0p, tau);
            } else if (strcmp(file_config.excitation_waveform, "hemp") == 0 ||
                       strcmp(file_config.excitation_waveform, "hemp_pulse") == 0) {
                const double t0p = file_config.excitation_hemp_t0;
                const double alpha = file_config.excitation_hemp_alpha;
                const double beta = file_config.excitation_hemp_beta;
                if (alpha <= 0.0 || beta <= 0.0 || alpha <= beta) {
                    fprintf(stderr,
                            "Error: for HEMP require excitation_hemp_alpha > excitation_hemp_beta > 0.\n");
                    free(freq_weights);
                    free(freqs);
                    mom_solver_destroy(solver);
                    return 1;
                }
                /* e(t)=A*(exp(-beta*(t-t0))-exp(-alpha*(t-t0)))u(t-t0)
                 * E(f)=A*exp(-j2pif t0)*[(1/(beta+j2pif))-(1/(alpha+j2pif))]. */
                for (int k = 0; k < N; k++) {
                    const double f_hz = freqs[k];
                    const double w = 2.0 * M_PI * f_hz;
                    const double den_b = beta * beta + w * w;
                    const double den_a = alpha * alpha + w * w;
                    const double br = beta / den_b;
                    const double bi = -w / den_b;
                    const double ar = alpha / den_a;
                    const double ai = -w / den_a;
                    const double hr = br - ar;
                    const double hi = bi - ai;
                    const double ph = -2.0 * M_PI * f_hz * t0p;
                    const double cr = cos(ph);
                    const double ci = sin(ph);
                    freq_weights[k].re = hr * cr - hi * ci;
                    freq_weights[k].im = hr * ci + hi * cr;
                }
                printf("Time-domain: HEMP enabled (t0=%.3e s, alpha=%.3e 1/s, beta=%.3e 1/s)\n",
                       t0p, alpha, beta);
            }
        }

        mom_time_domain_results_t td = {0};
        if (mom_solver_solve_time_domain(solver, freqs, N, &tdc, &td,
                                        use_band_mask, band_fmin, band_fmax, freq_weights) != 0) {
            fprintf(stderr, "Error: mom_solver_solve_time_domain failed (see messages above).\n");
            if (freq_weights) free(freq_weights);
            free(freqs);
            mom_time_domain_free_results(&td);
            mom_solver_destroy(solver);
            return 1;
        }
        if (freq_weights) free(freq_weights);
        free(freqs);

        int num_basis = mom_solver_get_num_unknowns(solver);
        if (num_basis <= 0 || !td.current_response || !td.time_points) {
            fprintf(stderr, "Error: invalid time-domain results.\n");
            mom_time_domain_free_results(&td);
            mom_solver_destroy(solver);
            return 1;
        }

        char out_dir[1024] = {0};
        get_output_dir(file_config.geometry_file, out_dir, sizeof(out_dir));

        /* Physical -∫ E·dl along A→B (before synthetic_gradient overwrites td.current_response). */
        if (out_dir[0] != '\0' && file_config.probe_vdiff_enable) {
            char vpath[1100];
            snprintf(vpath, sizeof(vpath), "%stime_domain_voltage_diff.csv", out_dir);
            FILE* vf = fopen(vpath, "w");
            if (vf) {
                point3d_t rA = { file_config.probe_ax, file_config.probe_ay, file_config.probe_az };
                point3d_t rB = { file_config.probe_bx, file_config.probe_by, file_config.probe_bz };
                int nseg = file_config.probe_line_segments > 0 ? file_config.probe_line_segments : 32;
                fprintf(vf, "time_s,V_diff_re,V_diff_im\n");
                for (int ti = 0; ti < N; ti++) {
                    if (mom_solver_apply_current_coefficients_complex(solver, &td.current_response[ti * num_basis], num_basis) != 0) {
                        break;
                    }
                    {
                        double vtk_f = file_config.frequency;
                        if (use_band_mask) {
                            vtk_f = 0.5 * (band_fmin + band_fmax);
                        }
                        mom_solver_set_frequency_metadata_hz(solver, vtk_f);
                    }
                    double Vre = 0.0, Vim = 0.0;
                    if (mom_solver_line_integral_e_dot_dl(solver, &rA, &rB, nseg, &Vre, &Vim) != 0) {
                        fprintf(stderr, "Warning: line integral failed at time index %d\n", ti);
                        break;
                    }
                    fprintf(vf, "%.12e,%.12e,%.12e\n", td.time_points[ti], Vre, Vim);
                }
                fclose(vf);
                printf("Time-domain voltage diff (-integral E dot dl) CSV: %s\n", vpath);
            }
        }

        /* Surface |J|(t) at triangle nearest query (bbox center = rough vehicle mid-body). Physical coeffs only. */
        if (out_dir[0] != '\0' && file_config.probe_surface_J_enable) {
            double qx = 0.0, qy = 0.0, qz = 0.0;
            if (file_config.probe_surface_J_use_bbox_center) {
                if (mom_solver_mesh_bbox_center(solver, &qx, &qy, &qz) != 0) {
                    fprintf(stderr, "Warning: probe_surface_J: mesh bbox center failed; using query (0,0,0).\n");
                }
            } else {
                qx = file_config.probe_surface_J_qx;
                qy = file_config.probe_surface_J_qy;
                qz = file_config.probe_surface_J_qz;
            }
            char jpath[1100];
            snprintf(jpath, sizeof(jpath), "%stime_domain_surface_J_probe.csv", out_dir);
            FILE* jf = fopen(jpath, "w");
            if (jf) {
                fprintf(jf,
                        "time_s,query_x,query_y,query_z,eval_x,eval_y,eval_z,triangle_index,Jmag,"
                        "Jx_re,Jx_im,Jy_re,Jy_im,Jz_re,Jz_im\n");
                for (int ti = 0; ti < N; ti++) {
                    if (mom_solver_apply_current_coefficients_complex(
                            solver, &td.current_response[ti * num_basis], num_basis) != 0) {
                        break;
                    }
                    {
                        double vtk_f = file_config.frequency;
                        if (use_band_mask) {
                            vtk_f = 0.5 * (band_fmin + band_fmax);
                        }
                        mom_solver_set_frequency_metadata_hz(solver, vtk_f);
                    }
                    double ex = 0.0, ey = 0.0, ez = 0.0;
                    int tri_i = -1;
                    complex_t Jx = {0.0, 0.0};
                    complex_t Jy = {0.0, 0.0};
                    complex_t Jz = {0.0, 0.0};
                    double Jmag = 0.0;
                    if (mom_solver_probe_surface_current_nearest(
                            solver, qx, qy, qz, file_config.conductor_material_id, &ex, &ey, &ez, &tri_i, &Jx, &Jy, &Jz,
                            &Jmag) != 0) {
                        fprintf(stderr, "Warning: surface current probe failed at time index %d\n", ti);
                        break;
                    }
                    fprintf(jf,
                            "%.12e,%.12e,%.12e,%.12e,%.12e,%.12e,%.12e,%d,%.12e,%.12e,%.12e,%.12e,%.12e,%.12e,"
                            "%.12e\n",
                            td.time_points[ti], qx, qy, qz, ex, ey, ez, tri_i, Jmag, Jx.re, Jx.im, Jy.re, Jy.im, Jz.re,
                            Jz.im);
                }
                fclose(jf);
                printf("Time-domain surface current probe CSV: %s\n", jpath);
            }
        }

        if (vtk_surface_opts && vtk_surface_mode == 2) {
            if (mom_solver_apply_synthetic_gradient_to_time_domain_response(
                    solver, td.current_response, N, num_basis, vtk_surface_opts) != 0) {
                fprintf(stderr, "Warning: synthetic gradient on time-domain response failed; physical IFFT data kept.\n");
            } else {
                printf("Time-domain current coefficients replaced with synthetic bbox gradient (CSV/VTK/results).\n");
            }
        }
        if (vtk_surface_opts && vtk_surface_mode == 1) {
            if (mom_solver_set_vtk_synthetic_display(solver, vtk_surface_opts) != 0) {
                fprintf(stderr, "Warning: VTK synthetic display failed.\n");
            } else {
                printf("VTK time-domain frames will use bbox gradient (visual only); time-domain CSV keeps physical IFFT.\n");
            }
        }

        if (out_dir[0] != '\0') {
            char csv_sum[1100];
            snprintf(csv_sum, sizeof(csv_sum), "%stime_domain_basis0_magnitude.csv", out_dir);
            FILE* cf = fopen(csv_sum, "w");
            if (cf) {
                fprintf(cf, "time_s,abs_J0\n");
                for (int i = 0; i < N; i++) {
#if defined(_MSC_VER)
                    complex_t c = td.current_response[i * num_basis + 0];
                    double mag = sqrt(c.re * c.re + c.im * c.im);
#else
                    double mag = cabs(td.current_response[i * num_basis + 0]);
#endif
                    fprintf(cf, "%.12e,%.12e\n", td.time_points[i], mag);
                }
                fclose(cf);
                printf("Time-domain CSV (|J| on basis 0): %s\n", csv_sum);
            }

            int stride = file_config.time_domain_vtk_stride > 0 ? file_config.time_domain_vtk_stride : 1;
            int maxv = file_config.time_domain_vtk_max_files > 0 ? file_config.time_domain_vtk_max_files : 16;
            int nout = 0;
            for (int ti = 0; ti < N && nout < maxv; ti += stride) {
                if (mom_solver_apply_current_coefficients_complex(solver, &td.current_response[ti * num_basis], num_basis) != 0) {
                    break;
                }
                {
                    double vtk_f = file_config.frequency;
                    if (use_band_mask) {
                        vtk_f = 0.5 * (band_fmin + band_fmax);
                    }
                    mom_solver_set_frequency_metadata_hz(solver, vtk_f);
                }
                char vtk_path[1100];
                snprintf(vtk_path, sizeof(vtk_path), "%ssurface_current_t%04d.vtk", out_dir, ti);
                if (mom_solver_export_surface_current_vtk(solver, vtk_path, file_config.conductor_material_id) == 0) {
                    printf("Surface current VTK (time index %d): %s\n", ti, vtk_path);
                    nout++;
                }
            }
        }

        char results_path[1024] = {0};
        build_results_path(file_config.geometry_file, results_path, sizeof(results_path));
        if (results_path[0] != '\0') {
            FILE* rf = fopen(results_path, "w");
            if (rf) {
                fprintf(rf, "MoM Time-Domain (DFT f-grid + IFFT)\n");
                fprintf(rf, "Geometry file: %s\n", file_config.geometry_file);
                fprintf(rf, "Reference frequency (Hz): %.6e\n", file_config.frequency);
                {
                    double dt_sum = (file_config.time_domain_t1 - file_config.time_domain_t0) / (double)(N - 1);
                    double f_nyq = 0.5 / dt_sum;
                    fprintf(rf, "DFT: dt = %.6e s, f_k = k/(N*dt) Hz, k=0..%d (DC skipped for MoM)\n",
                            dt_sum, N - 1);
                    fprintf(rf, "Approx. two-sided Nyquist frequency: %.6e Hz\n", f_nyq);
                }
                if (use_band_mask) {
                    fprintf(rf, "Band mask (solve only bins in band): %.6e .. %.6e Hz\n",
                            band_fmin, band_fmax);
                } else {
                    fprintf(rf, "Band mask: none (full DFT grid except DC)\n");
                }
                fprintf(rf, "Time window: %.6e .. %.6e s, %d points\n",
                        file_config.time_domain_t0, file_config.time_domain_t1, N);
                fprintf(rf, "Basis unknowns: %d\n", num_basis);
                fprintf(rf, "VTK stride: %d, max files: %d\n",
                        file_config.time_domain_vtk_stride, file_config.time_domain_vtk_max_files);
                fclose(rf);
                printf("Summary written to: %s\n", results_path);
            }
        }

        mom_time_domain_free_results(&td);
        mom_solver_destroy(solver);
        printf("Done.\n");
        return 0;
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

    if (vtk_surface_opts && vtk_surface_mode == 2) {
        if (mom_solver_apply_synthetic_gradient_to_coefficients(solver, vtk_surface_opts) != 0) {
            fprintf(stderr, "Warning: synthetic gradient on MoM coefficients failed; physical solution kept.\n");
        } else {
            printf("MoM surface-current coefficients replaced with synthetic bbox gradient (CSV, VTK, _results.txt, near field).\n");
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

    /* 单站 RCS（后向，与平面波传播方向相反）；幅值由 excitation_amplitude 缩放 */
    const double monostatic_rcs_m2 = mom_solver_monostatic_rcs_m2(solver);

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

    if (vtk_surface_opts && vtk_surface_mode == 1) {
        if (mom_solver_set_vtk_synthetic_display(solver, vtk_surface_opts) != 0) {
            fprintf(stderr, "Warning: VTK synthetic display (bbox gradient) could not be enabled.\n");
        } else {
            printf("VTK export will use bbox gradient (visual only); coefficients/CSV/RCS/near-field remain physical.\n");
        }
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
    printf("Monostatic RCS: %.6e m^2", monostatic_rcs_m2);
    if (monostatic_rcs_m2 > 0.0) {
        printf(" (%.3f dBsm)", 10.0 * log10(monostatic_rcs_m2));
    }
    printf("\n");
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
            fprintf(rf, "Excitation type: %s\n",
                    file_config.excitation_type[0] != '\0' ? file_config.excitation_type : "plane_wave");
            fprintf(rf, "Excitation amplitude |E0|: %.6e V/m\n",
                    (file_config.excitation_amplitude != 0.0) ? file_config.excitation_amplitude : 1.0);
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
            fprintf(rf, "MONOSTATIC_RCS_M2=%.12e\n", monostatic_rcs_m2);
            if (monostatic_rcs_m2 > 0.0) {
                fprintf(rf, "MONOSTATIC_RCS_DBSM=%.6f\n", 10.0 * log10(monostatic_rcs_m2));
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
