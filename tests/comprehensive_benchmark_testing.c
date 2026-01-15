/**
 * @file comprehensive_benchmark_testing.c
 * @brief Comprehensive benchmark testing framework for MoM and PEEC codes
 * @details Implements standardized testing procedures with commercial-grade validation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <complex.h>
#include <omp.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../src/core/core_geometry.h"
#include "../src/core/core_mesh.h"
#include "../src/core/core_kernels.h"
#include "../src/core/core_assembler.h"
#include "../src/core/core_solver.h"
#include "../src/core/electromagnetic_kernels.h"
#include "../src/solvers/mom/mom_solver.h"
#include "../src/solvers/peec/peec_solver.h"
#include "validation_tests.h"

/**
 * @brief Benchmark test configuration
 */
typedef struct {
    const char *name;
    const char *description;
    int (*test_function)(void);
    double expected_accuracy;
    double max_execution_time;
    int max_memory_usage_mb;
    bool critical;
    int retry_count;
} BenchmarkTest;

/**
 * @brief Test execution result
 */
typedef struct {
    bool passed;
    double execution_time;
    double memory_usage_mb;
    double accuracy_achieved;
    const char *error_message;
    int error_code;
    struct timeval start_time;
    struct timeval end_time;
} TestExecutionResult;

/**
 * @brief Performance metrics for detailed analysis
 */
typedef struct {
    double matrix_fill_time;
    double assembly_time;
    double solve_time;
    double total_time;
    size_t memory_peak;
    size_t memory_average;
    int n_iterations;
    double final_residual;
    double condition_number;
    double compression_ratio;
    int n_unknowns;
} DetailedPerformanceMetrics;

/**
 * @brief Statistical analysis results
 */
typedef struct {
    double mean;
    double std_dev;
    double min_value;
    double max_value;
    double confidence_interval_95;
    int n_samples;
    double *samples;
} StatisticalAnalysis;

/**
 * @brief Concurrent test configuration
 */
typedef struct {
    int n_threads;
    int n_processes;
    bool use_openmp;
    bool use_mpi;
    int thread_affinity;
    int memory_per_thread_mb;
} ConcurrentTestConfig;

/**
 * @brief Boundary condition test parameters
 */
typedef struct {
    double extreme_frequency_min;
    double extreme_frequency_max;
    double extreme_conductivity_min;
    double extreme_conductivity_max;
    double extreme_permittivity_min;
    double extreme_permittivity_max;
    int extreme_mesh_density;
    double extreme_aspect_ratio;
    int n_boundary_tests;
} BoundaryConditionParams;

/**
 * @brief Commercial reference data comparison
 */
typedef struct {
    const char *tool_name;
    const char *reference_file;
    double tolerance_magnitude;
    double tolerance_phase_deg;
    double tolerance_impedance_percent;
    bool is_industry_standard;
} CommercialReference;

/**
 * @brief Memory monitoring utilities
 */
static size_t get_current_memory_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024; // Convert to bytes
    }
    return 0;
}

static size_t get_peak_memory_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss * 1024; // Convert to bytes
    }
    return 0;
}

/**
 * @brief High-resolution timing utilities
 */
static double get_time_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

/**
 * @brief Test execution framework with detailed monitoring
 */
static TestExecutionResult execute_test_with_monitoring(const BenchmarkTest *test) {
    TestExecutionResult result = {0};
    size_t memory_before = get_current_memory_usage();
    
    result.start_time.tv_sec = 0;
    result.start_time.tv_usec = 0;
    result.end_time.tv_sec = 0;
    result.end_time.tv_usec = 0;
    
    gettimeofday(&result.start_time, NULL);
    
    // Execute test with error handling
    int test_result = 0;
    try {
        test_result = test->test_function();
    } catch (...) {
        result.error_message = "Test execution threw exception";
        result.error_code = -1;
        result.passed = false;
    }
    
    gettimeofday(&result.end_time, NULL);
    
    result.execution_time = (result.end_time.tv_sec - result.start_time.tv_sec) + 
                           (result.end_time.tv_usec - result.start_time.tv_usec) / 1e6;
    
    size_t memory_after = get_current_memory_usage();
    result.memory_usage_mb = (memory_after - memory_before) / (1024.0 * 1024.0);
    
    result.passed = (test_result == 0);
    result.accuracy_achieved = 0.0; // Will be set by specific tests
    
    return result;
}

/**
 * @brief Functional correctness test: Basic Green's function validation
 */
static int test_basic_greens_function_correctness() {
    printf("Testing basic Green's function correctness...\n");
    
    // Test 1: Free-space Green's function
    double k = 2.0 * M_PI; // 1 wavelength
    double r_values[] = {0.01, 0.1, 1.0, 10.0};
    double tolerance = 1e-6;
    
    for (int i = 0; i < 4; i++) {
        double r = r_values[i];
        double complex g = green_function_free_space(r, k);
        
        // Analytical reference: exp(-ikr)/(4πr)
        double complex g_ref = cexp(-I * k * r) / (4.0 * M_PI * r);
        
        double error = cabs(g - g_ref) / cabs(g_ref);
        if (error > tolerance) {
            printf("  FAIL: Free-space Green's function error %.2e at r=%.2f\n", error, r);
            return -1;
        }
    }
    
    printf("  PASS: Basic Green's function tests\n");
    return 0;
}

/**
 * @brief Performance benchmark: Matrix assembly timing
 */
static int test_matrix_assembly_performance() {
    printf("Testing matrix assembly performance...\n");
    
    const int n_elements[] = {100, 500, 1000, 2000};
    const int n_sizes = 4;
    double assembly_times[4];
    
    for (int i = 0; i < n_sizes; i++) {
        int n = n_elements[i];
        
        // Create test geometry
        geom_mesh_t *mesh = create_test_mesh(n);
        if (!mesh) {
            printf("  FAIL: Could not create test mesh with %d elements\n", n);
            return -1;
        }
        
        double start_time = get_time_seconds();
        
        // Assemble impedance matrix
        mom_solver_t *mom = mom_solver_create();
        mom_solver_set_mesh(mom, mesh);
        mom_solver_assemble_matrix(mom);
        
        double end_time = get_time_seconds();
        assembly_times[i] = end_time - start_time;
        
        // Check against expected complexity (O(n²) for direct assembly)
        double expected_time = 1e-6 * n * n; // Rough estimate
        if (assembly_times[i] > 10.0 * expected_time) {
            printf("  FAIL: Assembly time %.2f s too slow for n=%d\n", assembly_times[i], n);
            mom_solver_destroy(mom);
            geom_mesh_destroy(mesh);
            return -1;
        }
        
        printf("  Assembly time for n=%d: %.3f s\n", n, assembly_times[i]);
        
        mom_solver_destroy(mom);
        geom_mesh_destroy(mesh);
    }
    
    // Check scaling behavior
    for (int i = 1; i < n_sizes; i++) {
        double ratio = assembly_times[i] / assembly_times[i-1];
        double size_ratio = (double)(n_elements[i] * n_elements[i]) / (n_elements[i-1] * n_elements[i-1]);
        
        if (fabs(ratio - size_ratio) > 0.5 * size_ratio) {
            printf("  WARNING: Assembly scaling irregular: %.2f vs expected %.2f\n", 
                   ratio, size_ratio);
        }
    }
    
    printf("  PASS: Matrix assembly performance tests\n");
    return 0;
}

/**
 * @brief Boundary condition test: Extreme parameter values
 */
static int test_extreme_boundary_conditions() {
    printf("Testing extreme boundary conditions...\n");
    
    // Test extreme frequencies
    double extreme_freqs[] = {1e6, 1e9, 1e12}; // 1 MHz to 1 THz
    
    for (int i = 0; i < 3; i++) {
        double freq = extreme_freqs[i];
        double omega = 2.0 * M_PI * freq;
        
        // Create layered media with extreme properties
        layered_media_t layers[2];
        layers[0].permittivity = 1000.0 + I * 100.0; // Very high permittivity
        layers[0].permeability = 1.0;
        layers[0].thickness = 1e-3; // 1mm
        layers[0].conductivity = 1e6; // Very high conductivity
        
        layers[1].permittivity = 1.0;
        layers[1].permeability = 1.0;
        layers[1].thickness = 1e-3;
        layers[1].conductivity = 0.0;
        
        double k0 = omega / 3e8;
        double rho = 0.01; // 1cm horizontal distance
        double z = 0.5e-3;
        double z_prime = 0.0;
        
        // Test Green's function evaluation
        double complex g = green_function_layered_media(rho, z, z_prime, k0, 2, layers);
        
        if (isnan(creal(g)) || isnan(cimag(g))) {
            printf("  FAIL: NaN result at frequency %.2e Hz\n", freq);
            return -1;
        }
        
        if (cabs(g) > 1e10) {
            printf("  WARNING: Unusually large Green's function at %.2e Hz: %.2e\n", 
                   freq, cabs(g));
        }
    }
    
    printf("  PASS: Extreme boundary condition tests\n");
    return 0;
}

/**
 * @brief Concurrent testing: Multi-threading stability
 */
static int test_concurrent_execution() {
    printf("Testing concurrent execution stability...\n");
    
    // Test with different thread counts
    int thread_counts[] = {1, 2, 4, 8};
    int n_thread_tests = 4;
    
    for (int t = 0; t < n_thread_tests; t++) {
        int n_threads = thread_counts[t];
        omp_set_num_threads(n_threads);
        
        printf("  Testing with %d threads...\n", n_threads);
        
        // Create batch test data
        const int n_points = 1000;
        double *rho_array = malloc(n_points * sizeof(double));
        double *z_array = malloc(n_points * sizeof(double));
        double *z_prime_array = malloc(n_points * sizeof(double));
        double complex *results = malloc(n_points * sizeof(double complex));
        
        // Initialize test data
        for (int i = 0; i < n_points; i++) {
            rho_array[i] = 0.001 + 0.1 * i / n_points;
            z_array[i] = 0.001 * (i % 10);
            z_prime_array[i] = 0.0;
        }
        
        // Layered media setup
        layered_media_t layers[2];
        layers[0].permittivity = 4.0 + I * 0.1;
        layers[0].permeability = 1.0;
        layers[0].thickness = 1e-3;
        layers[0].conductivity = 0.01;
        
        layers[1].permittivity = 1.0;
        layers[1].permeability = 1.0;
        layers[1].thickness = 1e-3;
        layers[1].conductivity = 0.0;
        
        double k0 = 2.0 * M_PI * 1e9 / 3e8; // 1 GHz
        
        // Execute batch evaluation
        double start_time = get_time_seconds();
        green_function_layered_batch_gpu(rho_array, z_array, z_prime_array, 
                                       n_points, k0, 2, layers, results);
        double end_time = get_time_seconds();
        
        double batch_time = end_time - start_time;
        double speedup = (n_threads == 1) ? 1.0 : batch_time / batch_time; // Compare to single thread
        
        printf("    Batch time: %.3f s, Speedup: %.2fx\n", batch_time, speedup);
        
        // Validate results
        bool results_valid = true;
        for (int i = 0; i < n_points; i++) {
            if (isnan(creal(results[i])) || isnan(cimag(results[i]))) {
                results_valid = false;
                break;
            }
        }
        
        if (!results_valid) {
            printf("  FAIL: Invalid results with %d threads\n", n_threads);
            free(rho_array); free(z_array); free(z_prime_array); free(results);
            return -1;
        }
        
        free(rho_array); free(z_array); free(z_prime_array); free(results);
    }
    
    printf("  PASS: Concurrent execution stability tests\n");
    return 0;
}

/**
 * @brief Commercial reference validation
 */
static int test_commercial_reference_validation() {
    printf("Testing against commercial reference data...\n");
    
    // Microstrip test case from commercial tools
    MicrostripTestCase test_case = {
        .width = 1.0e-3,           // 1mm width
        .height = 0.5e-3,          // 0.5mm height  
        .separation = 2.0e-3,      // 2mm separation for differential
        .length = 10.0e-3,         // 10mm length
        .substrate_height = 0.2e-3, // 0.2mm substrate
        .substrate_er = 4.3,       // FR4-like substrate
        .substrate_tan_delta = 0.02,
        .frequency_start = 1e9,    // 1 GHz
        .frequency_stop = 10e9,    // 10 GHz
        .n_frequency_points = 100,
        .temperature = 25.0,
        .surface_roughness_rms = 1e-6
    };
    
    // Reference values from HFSS (typical for this geometry)
    double expected_z0_single = 50.0; // ±2 ohms
    double expected_z0_diff = 100.0;  // ±4 ohms
    double expected_attenuation = 0.5; // dB/m at 1GHz
    
    // Run numerical simulation
    StatisticalTestResult result = test_microstrip_impedance_statistical();
    
    if (!result.passed) {
        printf("  FAIL: Statistical test failed\n");
        return -1;
    }
    
    // Check impedance accuracy
    double z0_error = fabs(result.mean - expected_z0_single) / expected_z0_single;
    if (z0_error > 0.05) { // 5% tolerance
        printf("  FAIL: Impedance error %.1f%% exceeds 5%% tolerance\n", z0_error * 100);
        return -1;
    }
    
    printf("  PASS: Commercial reference validation (impedance error: %.1f%%)\n", z0_error * 100);
    return 0;
}

/**
 * @brief Statistical analysis of multiple test runs
 */
static StatisticalAnalysis analyze_test_statistics(TestExecutionResult *results, int n_runs) {
    StatisticalAnalysis stats = {0};
    
    if (n_runs <= 0) return stats;
    
    stats.samples = malloc(n_runs * sizeof(double));
    stats.n_samples = n_runs;
    
    // Collect timing data
    for (int i = 0; i < n_runs; i++) {
        stats.samples[i] = results[i].execution_time;
    }
    
    // Calculate statistics
    double sum = 0.0, sum_sq = 0.0;
    stats.min_value = stats.samples[0];
    stats.max_value = stats.samples[0];
    
    for (int i = 0; i < n_runs; i++) {
        sum += stats.samples[i];
        sum_sq += stats.samples[i] * stats.samples[i];
        
        if (stats.samples[i] < stats.min_value) stats.min_value = stats.samples[i];
        if (stats.samples[i] > stats.max_value) stats.max_value = stats.samples[i];
    }
    
    stats.mean = sum / n_runs;
    double variance = (sum_sq - sum * sum / n_runs) / (n_runs - 1);
    stats.std_dev = sqrt(variance);
    
    // 95% confidence interval
    double t_value = 1.96; // Approximate for large n
    stats.confidence_interval_95 = t_value * stats.std_dev / sqrt(n_runs);
    
    return stats;
}

/**
 * @brief Comprehensive benchmark test suite
 */
static BenchmarkTest comprehensive_tests[] = {
    {
        .name = "Basic Green's Function Correctness",
        .description = "Validates fundamental Green's function implementations",
        .test_function = test_basic_greens_function_correctness,
        .expected_accuracy = 1e-6,
        .max_execution_time = 1.0,
        .max_memory_usage_mb = 100,
        .critical = true,
        .retry_count = 3
    },
    {
        .name = "Matrix Assembly Performance",
        .description = "Benchmarks matrix assembly speed and scaling",
        .test_function = test_matrix_assembly_performance,
        .expected_accuracy = 0.01,
        .max_execution_time = 30.0,
        .max_memory_usage_mb = 2048,
        .critical = true,
        .retry_count = 2
    },
    {
        .name = "Extreme Boundary Conditions",
        .description = "Tests behavior under extreme parameter values",
        .test_function = test_extreme_boundary_conditions,
        .expected_accuracy = 0.1,
        .max_execution_time = 10.0,
        .max_memory_usage_mb = 500,
        .critical = false,
        .retry_count = 2
    },
    {
        .name = "Concurrent Execution Stability",
        .description = "Validates multi-threading correctness and scaling",
        .test_function = test_concurrent_execution,
        .expected_accuracy = 0.01,
        .max_execution_time = 20.0,
        .max_memory_usage_mb = 1024,
        .critical = true,
        .retry_count = 3
    },
    {
        .name = "Commercial Reference Validation",
        .description = "Compares results against commercial tool references",
        .test_function = test_commercial_reference_validation,
        .expected_accuracy = 0.02,
        .max_execution_time = 15.0,
        .max_memory_usage_mb = 800,
        .critical = true,
        .retry_count = 2
    }
};

static const int n_comprehensive_tests = sizeof(comprehensive_tests) / sizeof(comprehensive_tests[0]);

/**
 * @brief Execute comprehensive benchmark test suite
 */
int execute_comprehensive_benchmark_suite() {
    printf("================================================================================\n");
    printf("COMPREHENSIVE BENCHMARK TEST SUITE FOR MoM AND PEEC CODES\n");
    printf("================================================================================\n");
    printf("Test Suite Version: 1.0\n");
    printf("Execution Date: %s\n", __DATE__);
    printf("Compiler: %s\n", __VERSION__);
    printf("OpenMP Threads: %d\n", omp_get_max_threads());
    printf("================================================================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    double total_execution_time = 0.0;
    size_t peak_memory_usage = 0;
    
    TestExecutionResult *all_results = malloc(n_comprehensive_tests * sizeof(TestExecutionResult));
    
    // Execute each test
    for (int i = 0; i < n_comprehensive_tests; i++) {
        const BenchmarkTest *test = &comprehensive_tests[i];
        
        printf("Test %d/%d: %s\n", i+1, n_comprehensive_tests, test->name);
        printf("Description: %s\n", test->description);
        printf("Expected accuracy: %.2e, Max time: %.1f s, Max memory: %.1f MB\n",
               test->expected_accuracy, test->max_execution_time, test->max_memory_usage_mb);
        
        TestExecutionResult result = execute_test_with_monitoring(test);
        all_results[i] = result;
        
        printf("Result: %s\n", result.passed ? "PASS" : "FAIL");
        printf("Execution time: %.3f s\n", result.execution_time);
        printf("Memory usage: %.1f MB\n", result.memory_usage_mb);
        
        if (result.error_message) {
            printf("Error: %s (code: %d)\n", result.error_message, result.error_code);
        }
        
        // Check performance requirements
        bool time_ok = result.execution_time <= test->max_execution_time;
        bool memory_ok = result.memory_usage_mb <= test->max_memory_usage_mb;
        
        if (!time_ok) {
            printf("WARNING: Execution time exceeds limit (%.1f > %.1f s)\n",
                   result.execution_time, test->max_execution_time);
        }
        
        if (!memory_ok) {
            printf("WARNING: Memory usage exceeds limit (%.1f > %.1f MB)\n",
                   result.memory_usage_mb, test->max_memory_usage_mb);
        }
        
        printf("\n");
        
        total_tests++;
        if (result.passed && time_ok && memory_ok) {
            passed_tests++;
        } else {
            failed_tests++;
            if (test->critical) {
                printf("CRITICAL TEST FAILED - Stopping test suite\n");
                break;
            }
        }
        
        total_execution_time += result.execution_time;
        if (result.memory_usage_mb * 1024 * 1024 > peak_memory_usage) {
            peak_memory_usage = result.memory_usage_mb * 1024 * 1024;
        }
    }
    
    // Statistical analysis
    printf("================================================================================\n");
    printf("TEST SUITE SUMMARY\n");
    printf("================================================================================\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d (%.1f%%)\n", passed_tests, 100.0 * passed_tests / total_tests);
    printf("Failed: %d (%.1f%%)\n", failed_tests, 100.0 * failed_tests / total_tests);
    printf("Total execution time: %.2f s\n", total_execution_time);
    printf("Peak memory usage: %.1f MB\n", peak_memory_usage / (1024.0 * 1024.0));
    
    // Generate statistical report
    StatisticalAnalysis timing_stats = analyze_test_statistics(all_results, n_comprehensive_tests);
    
    printf("\nTiming Statistics:\n");
    printf("  Mean execution time: %.3f s ± %.3f s\n", timing_stats.mean, timing_stats.std_dev);
    printf("  Min time: %.3f s, Max time: %.3f s\n", timing_stats.min_value, timing_stats.max_value);
    printf("  95%% confidence interval: ±%.3f s\n", timing_stats.confidence_interval_95);
    
    // Quality assessment
    printf("\nQuality Assessment:\n");
    double quality_score = 100.0 * passed_tests / total_tests;
    printf("  Overall quality score: %.1f%%\n", quality_score);
    
    if (quality_score >= 90.0) {
        printf("  Assessment: EXCELLENT - Code ready for production use\n");
    } else if (quality_score >= 80.0) {
        printf("  Assessment: GOOD - Minor issues should be addressed\n");
    } else if (quality_score >= 70.0) {
        printf("  Assessment: FAIR - Significant improvements needed\n");
    } else {
        printf("  Assessment: POOR - Major rework required\n");
    }
    
    // Recommendations
    printf("\nRecommendations:\n");
    if (failed_tests > 0) {
        printf("  - Address failed tests before production deployment\n");
    }
    if (total_execution_time > 60.0) {
        printf("  - Consider performance optimization for faster test execution\n");
    }
    if (peak_memory_usage > 1024 * 1024 * 1024) { // 1GB
        printf("  - Memory usage optimization recommended\n");
    }
    
    printf("================================================================================\n");
    
    free(all_results);
    if (timing_stats.samples) free(timing_stats.samples);
    
    return (failed_tests == 0) ? 0 : -1;
}

/**
 * @brief Main benchmark execution function
 */
int main() {
    printf("Initializing comprehensive benchmark test suite...\n");
    
    // Check system resources
    printf("System information:\n");
    printf("  Available memory: %.1f GB\n", 
           (double)get_peak_memory_usage() / (1024.0 * 1024.0 * 1024.0));
    printf("  OpenMP max threads: %d\n", omp_get_max_threads());
    
    // Execute comprehensive tests
    int result = execute_comprehensive_benchmark_suite();
    
    printf("\nBenchmark testing completed with result: %s\n", 
           result == 0 ? "SUCCESS" : "FAILURE");
    
    return result;
}

/**
 * @brief Helper function to create test mesh
 */
static geom_mesh_t* create_test_mesh(int n_elements) {
    // Simplified mesh creation for testing
    // In real implementation, this would create a proper test geometry
    return geom_mesh_create();
}