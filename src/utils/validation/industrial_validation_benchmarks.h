/*****************************************************************************************
 * Industrial Validation Benchmarks for MoM/PEEC Solver
 * 
 * Comprehensive validation suite based on commercial software standards:
 * - Single wire inductance (analytical solutions)
 * - Multi-wire coupling (published results)
 * - 2D parallel plates (capacitance formulas)
 * - 3D enclosures (cavity resonances)
 * - PCB traces (industry measurements)
 * - Package inductance (vendor data)
 * - High-speed interconnects (S-parameter validation)
 *****************************************************************************************/

#ifndef INDUSTRIAL_VALIDATION_BENCHMARKS_H
#define INDUSTRIAL_VALIDATION_BENCHMARKS_H

#include <complex.h>
#include <math.h>
#include "../../backend/math/unified_matrix_assembly.h"
#include "../../backend/math/industrial_solver_abstraction.h"

// Validation tolerance levels (industrial standards)
#define VALIDATION_TOLERANCE_LOOSE  0.05   // 5% for complex structures
#define VALIDATION_TOLERANCE_MEDIUM 0.02   // 2% for standard validation
#define VALIDATION_TOLERANCE_STRICT 0.005  // 0.5% for critical applications
#define VALIDATION_TOLERANCE_ULTRA  0.001  // 0.1% for research-grade

// Benchmark result structure
typedef struct {
    char name[128];              // Benchmark name
    char description[256];       // Detailed description
    double frequency;            // Test frequency (Hz)
    double reference_value;      // Reference/analytical value
    double computed_value;       // Computed value
    double relative_error;       // |computed - reference| / |reference|
    double absolute_error;       // |computed - reference|
    int passed;                  // Pass/fail flag
    char error_message[256];     // Error details if failed
    double execution_time;       // Computation time
    size_t memory_usage_mb;     // Memory usage
    int convergence_iterations;  // Solver iterations
    double condition_number;     // Matrix condition number
} validation_result_t;

// Benchmark categories
typedef enum {
    BENCHMARK_CATEGORY_ANALYTICAL,     // Analytical solutions
    BENCHMARK_CATEGORY_PUBLISHED,      // Published numerical results
    BENCHMARK_CATEGORY_MEASUREMENT,    // Physical measurements
    BENCHMARK_CATEGORY_COMMERCIAL,     // Commercial software results
    BENCHMARK_CATEGORY_INTERLAB,       // Inter-laboratory comparison
    BENCHMARK_CATEGORY_MANUFACTURER    // Manufacturer specifications
} benchmark_category_t;

// Single wire benchmark (analytical inductance)
typedef struct {
    double length;               // Wire length (m)
    double radius;               // Wire radius (m)
    double frequency;            // Test frequency (Hz)
    double analytical_L;         // Analytical inductance (H)
    double analytical_R;         // Analytical resistance (Ω)
} single_wire_benchmark_t;

// Multi-wire coupling benchmark
typedef struct {
    int num_wires;               // Number of wires
    double* lengths;             // Wire lengths (m)
    double* radii;               // Wire radii (m)
    double* separations;         // Wire separations (m)
    double frequency;            // Test frequency (Hz)
    double complex* reference_L_matrix; // Reference inductance matrix (H)
    double complex* reference_C_matrix; // Reference capacitance matrix (F)
} multi_wire_benchmark_t;

// 2D parallel plate capacitor
typedef struct {
    double plate_length;         // Plate length (m)
    double plate_width;          // Plate width (m)
    double plate_separation;     // Plate separation (m)
    double permittivity;         // Dielectric permittivity
    double frequency;            // Test frequency (Hz)
    double analytical_C;       // Analytical capacitance (F)
    double analytical_L;       // Analytical inductance (H)
} parallel_plate_benchmark_t;

// 3D enclosure benchmark
typedef struct {
    double enclosure_length;     // Enclosure length (m)
    double enclosure_width;      // Enclosure width (m)
    double enclosure_height;     // Enclosure height (m)
    double wall_thickness;       // Wall thickness (m)
    double frequency_start;      // Start frequency (Hz)
    double frequency_end;        // End frequency (Hz)
    int num_frequencies;         // Number of frequency points
    double* reference_resonances; // Reference resonance frequencies (Hz)
    int num_resonances;          // Number of resonances
} enclosure_benchmark_t;

// PCB trace benchmark
typedef struct {
    double trace_length;         // Trace length (m)
    double trace_width;          // Trace width (m)
    double trace_thickness;      // Trace thickness (m)
    double substrate_height;     // Substrate height (m)
    double substrate_er;         // Substrate relative permittivity
    double frequency_start;      // Start frequency (Hz)
    double frequency_end;        // End frequency (Hz)
    int num_frequencies;         // Number of frequency points
    double* reference_Z0;        // Reference characteristic impedance (Ω)
    double* reference_delay;     // Reference propagation delay (s/m)
} pcb_trace_benchmark_t;

// Package inductance benchmark
typedef struct {
    char package_type[32];       // Package type (QFN, BGA, etc.)
    int num_pins;                // Number of pins
    double package_length;       // Package length (m)
    double package_width;        // Package width (m)
    double package_height;       // Package height (m)
    double pin_pitch;            // Pin pitch (m)
    double frequency;            // Test frequency (Hz)
    double* reference_inductances; // Reference pin inductances (H)
    double* reference_mutuals;   // Reference mutual inductances (H)
} package_benchmark_t;

// High-speed interconnect benchmark
typedef struct {
    double interconnect_length;  // Interconnect length (m)
    double conductor_width;      // Conductor width (m)
    double conductor_spacing;    // Conductor spacing (m)
    double dielectric_height;  // Dielectric height (m)
    double dielectric_er;        // Dielectric relative permittivity
    double frequency_start;    // Start frequency (Hz)
    double frequency_end;        // End frequency (Hz)
    int num_frequencies;       // Number of frequency points
    double complex* reference_S11; // Reference S11 (dB)
    double complex* reference_S21; // Reference S21 (dB)
    double complex* reference_Z;   // Reference impedance (Ω)
} interconnect_benchmark_t;

// Core validation functions

// Single wire validation
validation_result_t validate_single_wire_inductance(
    const single_wire_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

validation_result_t validate_single_wire_resistance(
    const single_wire_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

// Multi-wire coupling validation
validation_result_t validate_multi_wire_inductance_matrix(
    const multi_wire_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

validation_result_t validate_multi_wire_capacitance_matrix(
    const multi_wire_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

// 2D parallel plate validation
validation_result_t validate_parallel_plate_capacitance(
    const parallel_plate_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

validation_result_t validate_parallel_plate_inductance(
    const parallel_plate_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

// 3D enclosure validation
validation_result_t validate_enclosure_resonances(
    const enclosure_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

validation_result_t validate_enclosure_impedance(
    const enclosure_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

// PCB trace validation
validation_result_t validate_pcb_characteristic_impedance(
    const pcb_trace_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

validation_result_t validate_pcb_propagation_delay(
    const pcb_trace_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

// Package validation
validation_result_t validate_package_inductance(
    const package_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

validation_result_t validate_package_mutual_coupling(
    const package_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

// High-speed interconnect validation
validation_result_t validate_interconnect_s_parameters(
    const interconnect_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

validation_result_t validate_interconnect_impedance(
    const interconnect_benchmark_t* benchmark,
    unified_matrix_t* matrix,
    industrial_solver_params_t* solver_params);

// Advanced validation functions

// Convergence study
validation_result_t validate_convergence_study(
    const char* geometry_name,
    double mesh_density_start,
    double mesh_density_end,
    int num_mesh_densities,
    double reference_value,
    double tolerance);

// Frequency sweep validation
validation_result_t validate_frequency_sweep(
    const char* geometry_name,
    double frequency_start,
    double frequency_end,
    int num_frequencies,
    double complex* reference_values,
    double tolerance);

// Mesh independence study
validation_result_t validate_mesh_independence(
    const char* geometry_name,
    int* mesh_sizes,
    int num_mesh_sizes,
    double reference_value,
    double tolerance);

// Industrial benchmark suite
validation_result_t* run_complete_validation_suite(
    int* num_results,
    double tolerance_level);

validation_result_t* run_analytical_benchmarks(
    int* num_results,
    double tolerance);

validation_result_t* run_published_benchmarks(
    int* num_results,
    double tolerance);

validation_result_t* run_commercial_comparison(
    int* num_results,
    double tolerance);

// Statistical analysis of validation results
typedef struct {
    int total_benchmarks;
    int passed_benchmarks;
    int failed_benchmarks;
    double average_relative_error;
    double maximum_relative_error;
    double minimum_relative_error;
    double standard_deviation;
    double confidence_interval_95;
    double average_execution_time;
    double average_memory_usage;
} validation_statistics_t;

validation_statistics_t analyze_validation_results(
    validation_result_t* results,
    int num_results);

// Report generation
int generate_validation_report(
    validation_result_t* results,
    int num_results,
    const char* report_filename,
    int format); // 0=TXT, 1=CSV, 2=JSON, 3=HTML

int generate_benchmark_comparison_plot(
    validation_result_t* results,
    int num_results,
    const char* plot_filename);

// Industrial certification support
int validate_ieee_compliance(
    validation_result_t* results,
    int num_results);

int validate_iec_compliance(
    validation_result_t* results,
    int num_results);

int validate_ansi_compliance(
    validation_result_t* results,
    int num_results);

// Reference analytical formulas (for validation)

double analytical_wire_inductance(
    double length,
    double radius);

double analytical_wire_resistance(
    double length,
    double radius,
    double frequency);

double analytical_parallel_plate_capacitance(
    double area,
    double separation,
    double permittivity);

double analytical_cavity_resonance_frequency(
    double length,
    double width,
    double height,
    int mode_m,
    int mode_n,
    int mode_p);

double analytical_microstrip_characteristic_impedance(
    double width,
    double height,
    double thickness,
    double permittivity);

// Error estimation and uncertainty quantification
typedef struct {
    double systematic_error;
    double random_error;
    double discretization_error;
    double truncation_error;
    double total_uncertainty;
    double confidence_level;
} error_estimation_t;

error_estimation_t estimate_validation_uncertainty(
    validation_result_t* result,
    validation_result_t* similar_results,
    int num_similar);

#endif // INDUSTRIAL_VALIDATION_BENCHMARKS_H