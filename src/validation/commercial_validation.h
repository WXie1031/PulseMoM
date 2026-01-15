/**
 * @file commercial_validation.h
 * @brief Commercial-grade validation and benchmarking suite for PEEC-MoM framework
 * @details Comprehensive validation against commercial tools like FEKO, EMX, ANSYS Q3D, EMCOS
 */

#ifndef COMMERCIAL_VALIDATION_H
#define COMMERCIAL_VALIDATION_H

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BENCHMARK_CASES 100
#define MAX_VALIDATION_METRICS 50
#define VALIDATION_TOLERANCE 1e-3
#define BENCHMARK_TOLERANCE 0.05  /* 5% tolerance */

/* Commercial tool reference data structures */
typedef enum {
    REFERENCE_FEKO = 0,
    REFERENCE_EMX = 1,
    REFERENCE_ANSYS_Q3D = 2,
    REFERENCE_EMCOS = 3,
    REFERENCE_HFSS = 4,
    REFERENCE_CST = 5,
    REFERENCE_MWO = 6,
    REFERENCE_ADS = 7,
    REFERENCE_SONNET = 8,
    REFERENCE_CUSTOM = 9
} reference_tool_t;

typedef enum {
    BENCHMARK_ANTENNA = 0,
    BENCHMARK_FILTER = 1,
    BENCHMARK_PACKAGE = 2,
    BENCHMARK_INTERCONNECT = 3,
    BENCHMARK_POWER_INTEGRITY = 4,
    BENCHMARK_SIGNAL_INTEGRITY = 5,
    BENCHMARK_EMC_EMI = 6,
    BENCHMARK_RF_MICROWAVE = 7,
    BENCHMARK_WIRELESS = 8,
    BENCHMARK_CUSTOM = 9
} benchmark_category_t;

typedef enum {
    METRIC_S_PARAMETERS = 0,
    METRIC_Z_PARAMETERS = 1,
    METRIC_Y_PARAMETERS = 2,
    METRIC_IMPEDANCE = 3,
    METRIC_ADMITTANCE = 4,
    METRIC_RESONANCE_FREQUENCY = 5,
    METRIC_BANDWIDTH = 6,
    METRIC_INSERTION_LOSS = 7,
    METRIC_RETURN_LOSS = 8,
    METRIC_ISOLATION = 9,
    METRIC_CURRENT_DISTRIBUTION = 10,
    METRIC_FIELD_DISTRIBUTION = 11,
    METRIC_RADIATION_PATTERN = 12,
    METRIC_GAIN = 13,
    METRIC_EFFICIENCY = 14,
    METRIC_POWER_CONSUMPTION = 15,
    METRIC_TEMPERATURE_RISE = 16,
    METRIC_COUPLING_COEFFICIENT = 17,
    METRIC_CROSSTALK = 18,
    METRIC_EYE_DIAGRAM = 19
} validation_metric_t;

typedef struct {
    char name[128];
    char description[256];
    benchmark_category_t category;
    reference_tool_t reference_tool;
    double* geometry_parameters;
    int num_geometry_params;
    double* material_properties;
    int num_materials;
    double frequency_start;
    double frequency_stop;
    int num_frequency_points;
    double* reference_results;    /* Reference solution data */
    double* reference_uncertainty; /* Reference solution uncertainty */
    int reference_data_size;
    char reference_filename[256];
    bool is_enabled;
    double complexity_score;      /* Problem complexity (1-10) */
    double computational_cost;     /* Estimated computational cost */
} benchmark_case_t;

typedef struct {
    validation_metric_t metric_type;
    char name[64];
    char unit[32];
    double frequency;
    double reference_value;
    double computed_value;
    double absolute_error;
    double relative_error;
    double uncertainty;
    bool is_within_tolerance;
    double tolerance_threshold;
    char status[32];              /* "PASS", "FAIL", "WARNING" */
} validation_result_t;

typedef struct {
    benchmark_case_t* benchmark;
    validation_result_t* results;
    int num_results;
    double overall_score;         /* Overall validation score (0-100) */
    double max_relative_error;
    double average_relative_error;
    double rms_error;
    double correlation_coefficient;
    int num_passed_tests;
    int num_failed_tests;
    int num_warning_tests;
    double computation_time;
    double memory_usage;
    int num_iterations;
    bool converged;
    char validation_date[64];
    char validation_status[32];  /* "PASSED", "FAILED", "PARTIAL" */
} validation_report_t;

typedef struct {
    char solver_name[64];
    char version[32];
    char build_date[64];
    int num_cores;
    double memory_available;
    double memory_used;
    double cpu_time;
    double wall_time;
    int num_iterations;
    bool converged;
    double final_residual;
    char precision[16];           /* "SINGLE", "DOUBLE", "QUAD" */
    char parallel_mode[32];      /* "SEQUENTIAL", "OPENMP", "MPI", "HYBRID" */
} solver_performance_t;

typedef struct {
    benchmark_case_t cases[MAX_BENCHMARK_CASES];
    int num_cases;
    validation_report_t reports[MAX_BENCHMARK_CASES];
    solver_performance_t performance;
    bool enable_parallel_validation;
    bool enable_detailed_reporting;
    bool enable_visualization;
    bool enable_regression_testing;
    char output_directory[256];
    double validation_tolerance;
    double benchmark_tolerance;
    int max_iterations;
    bool stop_on_failure;
} validation_suite_t;

typedef struct {
    double frequency;
    double s11_ref;
    double s11_comp;
    double s21_ref;
    double s21_comp;
    double phase_ref;
    double phase_comp;
    double z11_ref;
    double z11_comp;
    double y11_ref;
    double y11_comp;
} frequency_domain_data_t;

typedef struct {
    double time;
    double voltage_ref;
    double voltage_comp;
    double current_ref;
    double current_comp;
    double power_ref;
    double power_comp;
} time_domain_data_t;

/* Standard benchmark cases */

/**
 * @brief Initialize validation suite
 * @param suite Validation suite to initialize
 * @return 0 on success, -1 on failure
 */
int validation_suite_init(validation_suite_t* suite);

/**
 * @brief Load benchmark cases from configuration file
 * @param suite Validation suite
 * @param config_filename Configuration filename
 * @return 0 on success, -1 on failure
 */
int validation_load_benchmarks(validation_suite_t* suite, const char* config_filename);

/**
 * @brief Add standard benchmark case
 * @param suite Validation suite
 * @param case_name Benchmark case name
 * @param category Benchmark category
 * @param reference_tool Reference commercial tool
 * @return 0 on success, -1 on failure
 */
int validation_add_standard_case(validation_suite_t* suite, const char* case_name,
                               benchmark_category_t category, reference_tool_t reference_tool);

/**
 * @brief Run single benchmark validation
 * @param suite Validation suite
 * @param case_index Benchmark case index
 * @param report Output validation report
 * @return 0 on success, -1 on failure
 */
int validation_run_single_case(validation_suite_t* suite, int case_index, validation_report_t* report);

/**
 * @brief Run complete validation suite
 * @param suite Validation suite
 * @return 0 on success, -1 on failure
 */
int validation_run_complete_suite(validation_suite_t* suite);

/**
 * @brief Compare S-parameters with reference
 * @param computed_s Computed S-parameters
 * @param reference_s Reference S-parameters
 * @param num_frequencies Number of frequency points
 * @param tolerance Tolerance for comparison
 * @param results Output validation results
 * @return Number of failed comparisons
 */
int validation_compare_s_parameters(double complex* computed_s, double complex* reference_s,
                                  int num_frequencies, double tolerance, validation_result_t* results);

/**
 * @brief Compare impedance parameters with reference
 * @param computed_z Computed Z-parameters
 * @param reference_z Reference Z-parameters
 * @param num_frequencies Number of frequency points
 * @param tolerance Tolerance for comparison
 * @param results Output validation results
 * @return Number of failed comparisons
 */
int validation_compare_z_parameters(double complex* computed_z, double complex* reference_z,
                                  int num_frequencies, double tolerance, validation_result_t* results);

/**
 * @brief Compare current distribution with reference
 * @param computed_current Computed current distribution
 * @param reference_current Reference current distribution
 * @param num_points Number of spatial points
 * @param tolerance Tolerance for comparison
 * @param results Output validation results
 * @return Number of failed comparisons
 */
int validation_compare_current_distribution(double complex* computed_current,
                                          double complex* reference_current,
                                          int num_points, double tolerance, validation_result_t* results);

/**
 * @brief Compare radiation pattern with reference
 * @param computed_pattern Computed radiation pattern
 * @param reference_pattern Reference radiation pattern
 * @param num_theta Number of theta points
 * @param num_phi Number of phi points
 * @param tolerance Tolerance for comparison
 * @param results Output validation results
 * @return Number of failed comparisons
 */
int validation_compare_radiation_pattern(double* computed_pattern, double* reference_pattern,
                                       int num_theta, int num_phi, double tolerance, validation_result_t* results);

/**
 * @brief Calculate validation metrics
 * @param computed Computed values
 * @param reference Reference values
 * @param num_points Number of data points
 * @param metrics Output validation metrics
 * @return 0 on success, -1 on failure
 */
int validation_calculate_metrics(double* computed, double* reference, int num_points,
                                validation_result_t* metrics);

/**
 * @brief Generate validation report
 * @param report Validation report
 * @param filename Output filename
 * @param format Report format ("HTML", "PDF", "CSV", "XML")
 * @return 0 on success, -1 on failure
 */
int validation_generate_report(validation_report_t* report, const char* filename, const char* format);

/**
 * @brief Print validation summary
 * @param suite Validation suite
 */
void validation_print_summary(validation_suite_t* suite);

/**
 * @brief Check validation status
 * @param suite Validation suite
 * @return Overall validation status (0=passed, 1=warnings, 2=failed)
 */
int validation_check_status(validation_suite_t* suite);

/**
 * @brief Export validation data for external analysis
 * @param suite Validation suite
 * @param filename Output filename
 * @param format Export format ("MAT", "CSV", "JSON")
 * @return 0 on success, -1 on failure
 */
int validation_export_data(validation_suite_t* suite, const char* filename, const char* format);

/**
 * @brief Import reference data from commercial tools
 * @param filename Reference data filename
 * @param tool_type Commercial tool type
 * @param reference_data Output reference data
 * @param num_points Number of data points
 * @return 0 on success, -1 on failure
 */
int validation_import_reference_data(const char* filename, reference_tool_t tool_type,
                                    double* reference_data, int* num_points);

/**
 * @brief Perform regression testing
 * @param suite Validation suite
 * @param baseline_results Baseline results for comparison
 * @return 0 on success, -1 on failure
 */
int validation_regression_test(validation_suite_t* suite, validation_report_t* baseline_results);

/**
 * @brief Benchmark solver performance
 * @param suite Validation suite
 * @param performance Performance metrics output
 * @return 0 on success, -1 on failure
 */
int validation_benchmark_performance(validation_suite_t* suite, solver_performance_t* performance);

/**
 * @brief Validate against IEEE standards
 * @param suite Validation suite
 * @param standard_name IEEE standard name
 * @param results Validation results
 * @return 0 on success, -1 on failure
 */
int validation_validate_ieee_standard(validation_suite_t* suite, const char* standard_name,
                                    validation_result_t* results);

/**
 * @brief Cleanup validation suite
 * @param suite Validation suite to cleanup
 */
void validation_suite_cleanup(validation_suite_t* suite);

/* Specific benchmark implementations */

/**
 * @brief Standard dipole antenna benchmark
 * @param suite Validation suite
 * @param report Output validation report
 * @return 0 on success, -1 on failure
 */
int benchmark_dipole_antenna(validation_suite_t* suite, validation_report_t* report);

/**
 * @brief Standard microstrip filter benchmark
 * @param suite Validation suite
 * @param report Output validation report
 * @return 0 on success, -1 on failure
 */
int benchmark_microstrip_filter(validation_suite_t* suite, validation_report_t* report);

/**
 * @brief Standard IC package benchmark
 * @param suite Validation suite
 * @param report Output validation report
 * @return 0 on success, -1 on failure
 */
int benchmark_ic_package(validation_suite_t* suite, validation_report_t* report);

/**
 * @brief Standard power distribution network benchmark
 * @param suite Validation suite
 * @param report Output validation report
 * @return 0 on success, -1 on failure
 */
int benchmark_power_distribution(validation_suite_t* suite, validation_report_t* report);

/**
 * @brief Standard signal integrity benchmark
 * @param suite Validation suite
 * @param report Output validation report
 * @return 0 on success, -1 on failure
 */
int benchmark_signal_integrity(validation_suite_t* suite, validation_report_t* report);

#ifdef __cplusplus
}
#endif

#endif /* COMMERCIAL_VALIDATION_H */