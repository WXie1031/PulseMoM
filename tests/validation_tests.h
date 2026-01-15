#ifndef VALIDATION_TESTS_H
#define VALIDATION_TESTS_H

#include "layered_greens_function.h"
#include "basis_functions.h"
#include "h_matrix_compression.h"

// Enhanced accuracy thresholds
#define IMPEDANCE_TOLERANCE 0.02    // 2% impedance accuracy
#define PHASE_VELOCITY_TOLERANCE 0.05  // 5% phase velocity accuracy
#define ATTENUATION_TOLERANCE 0.10   // 10% attenuation accuracy
#define S_PARAMETER_TOLERANCE 0.01   // 1% S-parameter accuracy
#define CONVERGENCE_TOLERANCE 1e-6   // Solver convergence tolerance

// Performance thresholds
#define MAX_MATRIX_FILL_TIME 10.0    // Maximum matrix fill time (seconds)
#define MAX_SOLVE_TIME 5.0          // Maximum solve time (seconds)
#define MAX_MEMORY_USAGE 2048       // Maximum memory usage (MB)
#define MIN_COMPRESSION_RATIO 0.85  // Minimum H-matrix compression ratio

typedef struct {
    const char *name;
    bool (*test_function)(void);
    double expected_accuracy;
    double max_execution_time;
    bool critical;              // Whether test failure is critical
    int retry_count;           // Number of retries allowed
} ValidationTest;

typedef struct {
    double width;          // Trace width [m]
    double height;         // Trace height [m]
    double separation;     // Differential pair separation [m]
    double length;         // Trace length [m]
    double substrate_height; // Substrate thickness [m]
    double substrate_er;   // Substrate relative permittivity
    double substrate_tan_delta; // Substrate loss tangent
    double frequency_start; // Start frequency [Hz]
    double frequency_stop;  // Stop frequency [Hz]
    int n_frequency_points; // Number of frequency points
    double temperature;     // Operating temperature [Celsius]
    double surface_roughness_rms; // Surface roughness [m]
} MicrostripTestCase;

typedef struct {
    double z0_single;      // Single-ended characteristic impedance [Ohm]
    double z0_diff;        // Differential characteristic impedance [Ohm]
    double z0_common;      // Common-mode characteristic impedance [Ohm]
    double attenuation;    // Attenuation [dB/m]
    double phase_velocity; // Phase velocity [m/s]
    double effective_er;   // Effective permittivity
    double conductor_loss; // Conductor loss [dB/m]
    double dielectric_loss; // Dielectric loss [dB/m]
    double radiation_loss; // Radiation loss [dB/m]
    double *frequency_response; // Frequency-dependent parameters
    int n_freq_points;   // Number of frequency points
} MicrostripResults;

// Enhanced test case categories
typedef enum {
    TEST_CATEGORY_SINGLE_ENDED,
    TEST_CATEGORY_DIFFERENTIAL,
    TEST_CATEGORY_MULTILAYER,
    TEST_CATEGORY_HIGH_FREQUENCY,
    TEST_CATEGORY_LARGE_SCALE,
    TEST_CATEGORY_VIA_STRUCTURES
} TestCategory;

typedef struct {
    TestCategory category;
    const char *description;
    double complexity_factor;  // Relative computational complexity
    int min_basis_functions;   // Minimum required basis functions
} TestCaseProfile;

// Advanced analytical models
MicrostripResults* analytical_microstrip_wheeler(
    const MicrostripTestCase *test_case
);

MicrostripResults* analytical_microstrip_schneider(
    const MicrostripTestCase *test_case
);

MicrostripResults* analytical_differential_pair_coupled(
    const MicrostripTestCase *test_case
);

MicrostripResults* analytical_microstrip_dispersive(
    const MicrostripTestCase *test_case,
    double *frequencies,
    int n_freq
);

// Enhanced numerical simulation with adaptive refinement
MicrostripResults* numerical_microstrip_adaptive(
    const MicrostripTestCase *test_case,
    const HMatrixParams *h_matrix_params,
    const IterativeSolverParams *solver_params,
    double target_accuracy,
    int max_refinement_levels
);

// Frequency domain analysis
typedef struct {
    double *frequencies;
    double complex *s_parameters;
    double complex *z_parameters;
    double complex *y_parameters;
    int n_ports;
    int n_freq_points;
} FrequencyDomainResults;

FrequencyDomainResults* analyze_frequency_response(
    const MicrostripTestCase *test_case,
    const HMatrixParams *h_matrix_params,
    const IterativeSolverParams *solver_params
);

// Test case library with varying complexity
MicrostripTestCase* create_comprehensive_test_suite(int *n_cases, TestCategory category);
TestCaseProfile* get_test_case_profiles(int *n_profiles);

// Enhanced validation test functions with statistical analysis
typedef struct {
    bool passed;
    double max_error;
    double mean_error;
    double std_deviation;
    double confidence_interval;
    int n_samples;
    double execution_time;
    int memory_peak_mb;
} StatisticalTestResult;

StatisticalTestResult test_microstrip_impedance_statistical(void);
StatisticalTestResult test_differential_pair_impedance_statistical(void);
StatisticalTestResult test_frequency_dispersion_statistical(void);
StatisticalTestResult test_attenuation_modeling_statistical(void);
StatisticalTestResult test_convergence_analysis_statistical(void);
StatisticalTestResult test_h_matrix_accuracy_statistical(void);
StatisticalTestResult test_iterative_solver_convergence_statistical(void);
StatisticalTestResult test_surface_roughness_model_statistical(void);

// Multi-scale validation
typedef struct {
    double scale_factor;
    int n_unknowns;
    double accuracy;
    double computation_time;
    double memory_usage;
} ScaleAnalysisResult;

ScaleAnalysisResult* perform_multiscale_analysis(
    const MicrostripTestCase *base_case,
    double min_scale,
    double max_scale,
    int n_scale_points
);

// Advanced accuracy assessment
double calculate_impedance_error_weighted(
    double z0_analytical,
    double z0_numerical,
    double frequency,
    double weight_factor
);

double calculate_s_parameter_error_magnitude(
    const double complex *s_analytical,
    const double complex *s_numerical,
    int n_ports,
    double frequency
);

double calculate_s_parameter_error_phase(
    const double complex *s_analytical,
    const double complex *s_numerical,
    int n_ports,
    double frequency
);

// Enhanced performance benchmarking
typedef struct {
    double setup_time;
    double matrix_fill_time;
    double compression_time;
    double solve_time;
    double post_processing_time;
    double total_time;
    int memory_usage_mb;
    int memory_peak_mb;
    int n_unknowns;
    int matrix_rank;
    double compression_ratio;
    double speedup_factor;
    int n_iterations;
    double final_residual;
    bool converged;
} EnhancedPerformanceMetrics;

EnhancedPerformanceMetrics* benchmark_enhanced_simulation(
    const MicrostripTestCase *test_case,
    const HMatrixParams *h_matrix_params,
    const IterativeSolverParams *solver_params,
    int n_repetitions
);

// Parallel performance analysis
typedef struct {
    int n_threads;
    double speedup;
    double efficiency;
    double computation_time;
    double memory_usage;
    bool scaling_efficient;
} ParallelPerformanceResult;

ParallelPerformanceResult* analyze_parallel_scaling(
    const MicrostripTestCase *test_case,
    int min_threads,
    int max_threads,
    int thread_increment
);

// Advanced visualization and reporting
typedef struct {
    const char *test_name;
    double accuracy;
    double performance_score;
    double memory_efficiency;
    double reliability_score;
    bool passed;
    const char *recommendations;
} TestScore;

void generate_enhanced_validation_report(
    const ValidationTest *tests,
    int n_tests,
    const EnhancedPerformanceMetrics *metrics,
    const char *output_filename,
    bool include_plots
);

void plot_enhanced_frequency_response(
    const double *frequencies,
    const double complex *s_parameters,
    const double complex *reference_data,
    int n_points,
    const char *filename,
    bool log_scale
);

void plot_convergence_analysis(
    const int *n_unknowns,
    const double *errors,
    const double *computation_times,
    int n_points,
    const char *filename
);

// Regression and comparison suite
bool run_enhanced_regression_tests(void);
bool validate_against_multiple_reference_data(const char **reference_files, int n_files);
bool compare_with_multiple_commercial_tools(const char **tool_names, int n_tools);

// Specific enhanced test implementations
StatisticalTestResult test_case_1_enhanced_single_microstrip(void);
StatisticalTestResult test_case_2_enhanced_differential_pair(void);
StatisticalTestResult test_case_3_enhanced_multilayer_structure(void);
StatisticalTestResult test_case_4_enhanced_via_transition(void);
StatisticalTestResult test_case_5_enhanced_large_scale_array(void);

// Machine learning-based error prediction
typedef struct {
    double predicted_error;
    double confidence_level;
    const char *error_source;
    const char *mitigation_strategy;
} ErrorPrediction;

ErrorPrediction* predict_simulation_errors(
    const MicrostripTestCase *test_case,
    const HMatrixParams *h_matrix_params,
    const IterativeSolverParams *solver_params
);

// Adaptive parameter optimization
typedef struct {
    HMatrixParams optimal_h_matrix_params;
    IterativeSolverParams optimal_solver_params;
    double expected_accuracy;
    double expected_computation_time;
    double expected_memory_usage;
    bool parameters_reliable;
} OptimizedParameters;

OptimizedParameters* optimize_simulation_parameters(
    const MicrostripTestCase *test_case,
    double target_accuracy,
    double max_computation_time,
    int max_memory_usage_mb
);

// Enhanced error analysis with root cause identification
typedef struct {
    double max_error;
    double rms_error;
    double error_location[3];
    int error_frequency_index;
    const char *error_source;
    const char *root_cause;
    const char *mitigation_strategy;
    double confidence_level;
    bool is_critical_error;
} EnhancedErrorAnalysis;

EnhancedErrorAnalysis* analyze_enhanced_simulation_errors(
    const MicrostripResults *analytical,
    const MicrostripResults *numerical,
    const MicrostripTestCase *test_case,
    const FrequencyDomainResults *freq_results
);

// Test execution framework
bool run_all_validation_tests(void);
bool run_specific_test_category(TestCategory category);
bool run_performance_benchmarks(void);
bool run_regression_tests_with_comparison(void);

// Memory management for enhanced structures
void free_enhanced_performance_metrics(EnhancedPerformanceMetrics *metrics);
void free_frequency_domain_results(FrequencyDomainResults *results);
void free_scale_analysis_results(ScaleAnalysisResult *results, int n_results);
void free_enhanced_error_analysis(EnhancedErrorAnalysis *analysis);
void free_optimized_parameters(OptimizedParameters *params);

#endif // VALIDATION_TESTS_H