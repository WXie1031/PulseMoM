/********************************************************************************
 * PulseEM - Unified Electromagnetic Simulation Platform
 *
 * Copyright (C) 2024-2025 PulseEM Technologies
 *
 * Commercial License - All Rights Reserved
 * Unauthorized copying, modification, or distribution is strictly prohibited
 * Proprietary and confidential - see LICENSE file for details
 *
 * File: main.c
 * Description: Main entry point for PulseEM electromagnetic simulation platform
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <time.h>

#include "layered_greens_function.h"
#include "basis_functions.h"
#include "h_matrix_compression.h"
#include "validation_tests.h"

// Main simulation function
void run_microstrip_simulation(void) {
    printf("=== PulseMoM: PCB Electromagnetic Simulation ===\n\n");
    
    // Create test case
    MicrostripTestCase test_case;
    test_case.width = 1.5e-3;      // 1.5 mm
    test_case.height = 35e-6;      // 35 um copper
    test_case.separation = 0.0;    // Single-ended
    test_case.length = 10e-3;      // 10 mm
    test_case.substrate_height = 0.8e-3; // 0.8 mm
    test_case.substrate_er = 4.4; // FR4
    test_case.substrate_tan_delta = 0.02;
    test_case.frequency_start = 1e9;  // 1 GHz
    test_case.frequency_stop = 10e9;  // 10 GHz
    test_case.n_frequency_points = 11;
    
    printf("Microstrip Geometry:\n");
    printf("  Width: %.3f mm\n", test_case.width * 1000);
    printf("  Substrate height: %.3f mm\n", test_case.substrate_height * 1000);
    printf("  Substrate εr: %.1f\n", test_case.substrate_er);
    printf("  Frequency: %.1f GHz\n\n", test_case.frequency_start / 1e9);
    
    // Analytical analysis
    printf("Running analytical analysis...\n");
    MicrostripResults *analytical = analytical_microstrip_analysis(&test_case);
    
    printf("Analytical Results:\n");
    printf("  Characteristic impedance: %.1f Ω\n", analytical->z0_single);
    printf("  Effective permittivity: %.2f\n", analytical->effective_er);
    printf("  Phase velocity: %.2e m/s\n", analytical->phase_velocity);
    printf("  Attenuation: %.2f dB/m\n\n", analytical->attenuation);
    
    // Numerical analysis
    printf("Running numerical MoM analysis...\n");
    
    // Set up H-matrix parameters
    HMatrixParams h_params;
    h_params.cluster_size = 32;
    h_params.tolerance = 1e-4;
    h_params.max_rank = 50;
    h_params.admissibility = 2;
    
    // Set up solver parameters
    IterativeSolverParams solver_params;
    solver_params.max_iterations = 1000;
    solver_params.tolerance = 1e-6;
    solver_params.restart_parameter = 30;
    solver_params.use_preconditioner = true;
    solver_params.convergence_check_interval = 10;
    
    // Run numerical simulation
    MicrostripResults *numerical = numerical_microstrip_analysis(&test_case, &h_params, &solver_params);
    
    printf("Numerical Results:\n");
    printf("  Characteristic impedance: %.1f Ω\n", numerical->z0_single);
    printf("  Effective permittivity: %.2f\n", numerical->effective_er);
    printf("  Phase velocity: %.2e m/s\n", numerical->phase_velocity);
    printf("  Attenuation: %.2f dB/m\n\n", numerical->attenuation);
    
    // Compare results
    double impedance_error = fabs(analytical->z0_single - numerical->z0_single) / analytical->z0_single * 100.0;
    double er_error = fabs(analytical->effective_er - numerical->effective_er) / analytical->effective_er * 100.0;
    
    printf("Comparison:\n");
    printf("  Impedance error: %.2f%%\n", impedance_error);
    printf("  Effective εr error: %.2f%%\n\n", er_error);
    
    // Performance benchmark
    printf("Running performance benchmark...\n");
    PerformanceMetrics *metrics = benchmark_microstrip_simulation(&test_case, &h_params, &solver_params);
    
    printf("Performance Metrics:\n");
    printf("  Total time: %.3f seconds\n", metrics->total_time);
    printf("  Matrix fill time: %.3f seconds\n", metrics->matrix_fill_time);
    printf("  Compression time: %.3f seconds\n", metrics->compression_time);
    printf("  Solve time: %.3f seconds\n", metrics->solve_time);
    printf("  Memory usage: %d MB\n", metrics->memory_usage_mb);
    printf("  Unknowns: %d\n", metrics->n_unknowns);
    printf("  Matrix rank: %d\n\n", metrics->matrix_rank);
    
    // Validation tests
    printf("Running validation test suite...\n");
    bool validation_passed = run_validation_tests();
    
    printf("\n=== Simulation Complete ===\n");
    printf("Validation result: %s\n", validation_passed ? "PASS" : "FAIL");
    
    // Clean up
    free(analytical);
    free(numerical);
    free(metrics);
}

// Test Green's function
void test_greens_function(void) {
    printf("\n=== Testing Layered Medium Green's Function ===\n\n");
    
    // Create simple layered medium
    LayeredMedium *medium = (LayeredMedium*)malloc(sizeof(LayeredMedium));
    medium->num_layers = 2;
    medium->thickness = (double*)malloc(2 * sizeof(double));
    medium->epsilon_r = (double*)malloc(2 * sizeof(double));
    medium->mu_r = (double*)malloc(2 * sizeof(double));
    medium->sigma = (double*)malloc(2 * sizeof(double));
    medium->tan_delta = (double*)malloc(2 * sizeof(double));
    
    // Substrate layer
    medium->thickness[0] = 1.6e-3;
    medium->epsilon_r[0] = 4.4;
    medium->mu_r[0] = 1.0;
    medium->sigma[0] = 0.0;
    medium->tan_delta[0] = 0.02;
    
    // Air layer
    medium->thickness[1] = 1e-3;
    medium->epsilon_r[1] = 1.0;
    medium->mu_r[1] = 1.0;
    medium->sigma[1] = 0.0;
    medium->tan_delta[1] = 0.0;
    
    // Frequency domain
    FrequencyDomain freq;
    freq.freq = 1e9;
    freq.omega = 2.0 * M_PI * freq.freq;
    freq.k0 = 2.0 * M_PI * freq.freq / 299792458.0;
    freq.eta0 = 376.73;
    
    // Test points
    GreensFunctionPoints points;
    points.x = points.xp = 0.0;
    points.y = points.yp = 0.0;
    points.z = 0.8e-3;
    points.zp = 0.8e-3;
    points.layer_src = 0;
    points.layer_obs = 0;
    
    // Green's function parameters
    GreensFunctionParams gf_params;
    gf_params.n_points = 50;
    gf_params.krho_max = 100.0;
    gf_params.krho_points = (double*)malloc(gf_params.n_points * sizeof(double));
    gf_params.weights = (double*)malloc(gf_params.n_points * sizeof(double));
    
    for (int i = 0; i < gf_params.n_points; i++) {
        gf_params.krho_points[i] = (i + 1) * gf_params.krho_max / gf_params.n_points;
        gf_params.weights[i] = gf_params.krho_max / gf_params.n_points;
    }
    
    gf_params.use_dcim = true;
    gf_params.n_images = 10;
    gf_params.amplitudes = (double complex*)malloc(gf_params.n_images * sizeof(double complex));
    gf_params.exponents = (double complex*)malloc(gf_params.n_images * sizeof(double complex));
    
    // Calculate Green's function
    clock_t start_time = clock();
    GreensFunctionDyadic *gf = greens_function_layered(medium, &freq, &points, &gf_params);
    clock_t end_time = clock();
    
    double computation_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("Green's Function Results:\n");
    printf("  Computation time: %.3f seconds\n", computation_time);
    printf("  G_ee[0][0]: %.3e + %.3ei\n", creal(gf->G_ee[0][0]), cimag(gf->G_ee[0][0]));
    printf("  G_ee[1][1]: %.3e + %.3ei\n", creal(gf->G_ee[1][1]), cimag(gf->G_ee[1][1]));
    printf("  G_ee[2][2]: %.3e + %.3ei\n", creal(gf->G_ee[2][2]), cimag(gf->G_ee[2][2]));
    
    // Clean up
    free_layered_medium(medium);
    free_greens_function_dyadic(gf);
    free_greens_function_params(&gf_params);
}

int main(int argc, char *argv[]) {
    printf("PulseMoM - PCB Electromagnetic Simulation Tool\n");
    printf("Version 1.0 - Spectral Domain Method of Moments\n\n");
    
    // Test Green's function
    test_greens_function();
    
    // Run main simulation
    run_microstrip_simulation();
    
    // Optional: run PCB workflow if input file provided
    if (argc > 1) {
        extern PCBWorkflowController* create_pcb_workflow_controller(void);
        extern int initialize_pcb_workflow(PCBWorkflowController* controller, const PCBWorkflowParams* params);
        extern int run_complete_pcb_simulation(PCBWorkflowController* controller);
        extern PCBFileFormat detect_pcb_file_format(const char* filename);
        extern void destroy_pcb_workflow_controller(PCBWorkflowController* controller);
        PCBWorkflowController* ctrl = create_pcb_workflow_controller();
        if (ctrl) {
            PCBWorkflowParams p = ctrl->params;
            snprintf(p.pcb_input_file, sizeof(p.pcb_input_file), "%s", argv[1]);
            p.input_format = detect_pcb_file_format(argv[1]);
            initialize_pcb_workflow(ctrl, &p);
            run_complete_pcb_simulation(ctrl);
            destroy_pcb_workflow_controller(ctrl);
        }
    }
    
    return 0;
}
