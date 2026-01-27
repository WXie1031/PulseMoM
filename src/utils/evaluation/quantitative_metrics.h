#ifndef QUANTITATIVE_METRICS_H
#define QUANTITATIVE_METRICS_H

#include "../../backend/algorithms/adaptive/adaptive_calculation.h"
#include "../../discretization/geometry/pcb_ic_structures.h"
#include "../../solvers/mom/mom_solver.h"
#include "../../solvers/fem/fem_solver.h"
#include "../../solvers/fdtd/fdtd_solver.h"
#include "../../solvers/peec/peec_solver.h"

typedef enum {
    METRIC_TYPE_ACCURACY,
    METRIC_TYPE_PERFORMANCE,
    METRIC_TYPE_MEMORY,
    METRIC_TYPE_CONVERGENCE,
    METRIC_TYPE_SCALABILITY,
    METRIC_TYPE_STABILITY,
    METRIC_TYPE_EFFICIENCY,
    METRIC_TYPE_RELIABILITY
} MetricType;

typedef enum {
    ACCURACY_METRIC_L2_ERROR,
    ACCURACY_METRIC_L_INF_ERROR,
    ACCURACY_METRIC_RELATIVE_ERROR,
    ACCURACY_METRIC_ABSOLUTE_ERROR,
    ACCURACY_METRIC_RMS_ERROR,
    ACCURACY_METRIC_PHASE_ERROR,
    ACCURACY_METRIC_MAGNITUDE_ERROR,
    ACCURACY_METRIC_VECTOR_ERROR,
    ACCURACY_METRIC_SCALAR_ERROR
} AccuracyMetric;

typedef enum {
    PERFORMANCE_METRIC_EXECUTION_TIME,
    PERFORMANCE_METRIC_SETUP_TIME,
    PERFORMANCE_METRIC_SOLVE_TIME,
    PERFORMANCE_METRIC_MEMORY_USAGE,
    PERFORMANCE_METRIC_MEMORY_PEAK,
    PERFORMANCE_METRIC_ITERATIONS,
    PERFORMANCE_METRIC_CONVERGENCE_RATE,
    PERFORMANCE_METRIC_PARALLEL_SPEEDUP,
    PERFORMANCE_METRIC_PARALLEL_EFFICIENCY
} PerformanceMetric;

typedef enum {
    SCALABILITY_METRIC_WEAK_SCALING,
    SCALABILITY_METRIC_STRONG_SCALING,
    SCALABILITY_METRIC_MEMORY_SCALING,
    SCALABILITY_METRIC_TIME_SCALING,
    SCALABILITY_METRIC_COMMUNICATION_OVERHEAD
} ScalabilityMetric;

typedef enum {
    STABILITY_METRIC_CONDITION_NUMBER,
    STABILITY_METRIC_RESIDUAL_HISTORY,
    STABILITY_METRIC_SOLUTION_VARIATION,
    STABILITY_METRIC_CONVERGENCE_STABILITY,
    STABILITY_METRIC_NUMERICAL_DISSIPATION,
    STABILITY_METRIC_ENERGY_CONSERVATION
} StabilityMetric;

typedef struct {
    double l2_error;
    double l_inf_error;
    double relative_error;
    double absolute_error;
    double rms_error;
    double phase_error_degrees;
    double magnitude_error_percent;
    double vector_error;
    double scalar_error;
    double max_error_location[3];
    double error_distribution_std;
    double error_correlation_coefficient;
    int error_sample_points;
} AccuracyMetrics;

typedef struct {
    double total_time_seconds;
    double setup_time_seconds;
    double solve_time_seconds;
    double memory_usage_gb;
    double memory_peak_gb;
    int iterations;
    double convergence_rate;
    double parallel_speedup;
    double parallel_efficiency;
    double cpu_utilization_percent;
    double memory_bandwidth_utilization;
    double cache_hit_rate;
    int thread_count;
    int mpi_processes;
    bool gpu_accelerated;
} PerformanceMetrics;

typedef struct {
    double weak_scaling_efficiency;
    double strong_scaling_efficiency;
    double memory_scaling_factor;
    double time_scaling_factor;
    double communication_overhead_percent;
    double load_imbalance_factor;
    double domain_decomposition_quality;
    int min_processes;
    int max_processes;
    int optimal_process_count;
} ScalabilityMetrics;

typedef struct {
    double condition_number;
    double residual_history_stability;
    double solution_variation_coefficient;
    double convergence_stability_index;
    double numerical_dissipation_rate;
    double energy_conservation_error;
    double time_step_stability_factor;
    double mesh_quality_factor;
    double solver_stability_margin;
    int stability_warnings;
    int stability_failures;
} StabilityMetrics;

typedef struct {
    double computational_efficiency;
    double memory_efficiency;
    double energy_efficiency;
    double algorithmic_efficiency;
    double numerical_efficiency;
    double parallel_efficiency;
    double load_balancing_efficiency;
    double cache_efficiency;
    double vectorization_efficiency;
    double memory_access_efficiency;
} EfficiencyMetrics;

typedef struct {
    double reliability_score;
    double robustness_score;
    double consistency_score;
    double repeatability_score;
    double fault_tolerance_score;
    double error_recovery_rate;
    int successful_computations;
    int failed_computations;
    int recovered_computations;
    double mean_time_between_failures;
    double mean_time_to_recovery;
} ReliabilityMetrics;

typedef struct {
    char test_case_name[256];
    char algorithm_name[128];
    char problem_type[128];
    char mesh_type[64];
    int mesh_elements;
    int unknowns;
    double frequency_hz;
    double problem_size_meters;
    int dimensions;
    bool complex_geometry;
    bool multiscale;
    bool multiphysics;
} TestCaseInfo;

typedef struct {
    TestCaseInfo test_info;
    AccuracyMetrics accuracy;
    PerformanceMetrics performance;
    ScalabilityMetrics scalability;
    StabilityMetrics stability;
    EfficiencyMetrics efficiency;
    ReliabilityMetrics reliability;
    double overall_score;
    double weighted_score;
    int ranking;
    char evaluation_date[64];
    char evaluator[128];
    char notes[1024];
} QuantitativeEvaluation;

typedef struct {
    QuantitativeEvaluation* evaluations;
    int num_evaluations;
    int max_evaluations;
    double benchmark_reference;
    char benchmark_name[256];
    char evaluation_criteria[1024];
    bool normalize_scores;
    bool enable_weighting;
    double accuracy_weight;
    double performance_weight;
    double scalability_weight;
    double stability_weight;
    double efficiency_weight;
    double reliability_weight;
} EvaluationSuite;

EvaluationSuite* quantitative_metrics_create_suite(const char* name);
void quantitative_metrics_destroy_suite(EvaluationSuite* suite);

int quantitative_metrics_add_evaluation(
    EvaluationSuite* suite,
    const QuantitativeEvaluation* evaluation
);

int quantitative_metrics_compute_accuracy_metrics(
    QuantitativeEvaluation* eval,
    const Complex* computed_solution,
    const Complex* reference_solution,
    int num_points,
    const double* coordinates
);

int quantitative_metrics_compute_performance_metrics(
    QuantitativeEvaluation* eval,
    double total_time,
    double setup_time,
    double solve_time,
    double memory_usage,
    double memory_peak,
    int iterations,
    int threads,
    int mpi_processes,
    bool gpu_accelerated
);

int quantitative_metrics_compute_scalability_metrics(
    QuantitativeEvaluation* eval,
    int num_processes,
    double* execution_times,
    double* memory_usages,
    int num_measurements
);

int quantitative_metrics_compute_stability_metrics(
    QuantitativeEvaluation* eval,
    const double* residual_history,
    const Complex* solution_history,
    int history_length,
    double time_step,
    double final_residual
);

int quantitative_metrics_compute_efficiency_metrics(
    QuantitativeEvaluation* eval,
    double theoretical_peak_performance,
    double theoretical_peak_memory_bandwidth,
    double theoretical_peak_cache_bandwidth,
    double achieved_performance,
    double achieved_memory_bandwidth,
    double achieved_cache_bandwidth
);

int quantitative_metrics_compute_reliability_metrics(
    QuantitativeEvaluation* eval,
    int num_runs,
    double* execution_times,
    double* accuracy_results,
    int* convergence_status,
    int* error_codes
);

double quantitative_metrics_compute_overall_score(
    const QuantitativeEvaluation* eval,
    double accuracy_weight,
    double performance_weight,
    double scalability_weight,
    double stability_weight,
    double efficiency_weight,
    double reliability_weight
);

int quantitative_metrics_rank_evaluations(EvaluationSuite* suite);

int quantitative_metrics_generate_report(
    const EvaluationSuite* suite,
    const char* filename,
    ReportFormat format
);

int quantitative_metrics_compare_algorithms(
    const EvaluationSuite* suite,
    const char* algorithm1,
    const char* algorithm2,
    ComparisonResult* result
);

int quantitative_metrics_benchmark_against_reference(
    QuantitativeEvaluation* eval,
    const QuantitativeEvaluation* reference,
    BenchmarkResult* result
);

double quantitative_metrics_estimate_uncertainty(
    const QuantitativeEvaluation* eval,
    UncertaintyType type
);

int quantitative_metrics_validate_results(
    const QuantitativeEvaluation* eval,
    ValidationResult* result
);

int quantitative_metrics_export_to_csv(
    const EvaluationSuite* suite,
    const char* filename
);

int quantitative_metrics_import_from_csv(
    EvaluationSuite* suite,
    const char* filename
);

int quantitative_metrics_plot_metrics(
    const EvaluationSuite* suite,
    const char* output_dir,
    PlotType plot_type
);

double quantitative_metrics_get_metric_value(
    const QuantitativeEvaluation* eval,
    MetricType type,
    int sub_metric
);

int quantitative_metrics_set_weights(
    EvaluationSuite* suite,
    double accuracy_weight,
    double performance_weight,
    double scalability_weight,
    double stability_weight,
    double efficiency_weight,
    double reliability_weight
);

int quantitative_metrics_normalize_suite(EvaluationSuite* suite);

int quantitative_metrics_compute_statistics(
    const EvaluationSuite* suite,
    MetricStatistics* stats
);

int quantitative_metrics_identify_outliers(
    const EvaluationSuite* suite,
    double threshold,
    int** outlier_indices,
    int* num_outliers
);

int quantitative_metrics_generate_recommendations(
    const EvaluationSuite* suite,
    RecommendationResult* recommendations
);

#endif