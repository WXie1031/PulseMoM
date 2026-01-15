#include "quantitative_metrics.h"
#include "../utils/memory.h"
#include "../utils/math_utils.h"
#include "../utils/statistics.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_MAX_EVALUATIONS 1000
#define DEFAULT_ACCURACY_WEIGHT 0.3
#define DEFAULT_PERFORMANCE_WEIGHT 0.25
#define DEFAULT_SCALABILITY_WEIGHT 0.15
#define DEFAULT_STABILITY_WEIGHT 0.15
#define DEFAULT_EFFICIENCY_WEIGHT 0.1
#define DEFAULT_RELIABILITY_WEIGHT 0.05

#define MIN_ACCURACY_THRESHOLD 1e-12
#define MAX_ACCEPTABLE_ERROR 1.0
#define CONVERGENCE_RATE_THRESHOLD 0.1
#define STABILITY_THRESHOLD 0.95

typedef struct {
    ReportFormat format;
    const char* extension;
} ReportFormatInfo;

static const ReportFormatInfo REPORT_FORMATS[] = {
    {REPORT_FORMAT_TEXT, ".txt"},
    {REPORT_FORMAT_CSV, ".csv"},
    {REPORT_FORMAT_JSON, ".json"},
    {REPORT_FORMAT_XML, ".xml"},
    {REPORT_FORMAT_HTML, ".html"},
    {REPORT_FORMAT_LATEX, ".tex"}
};

EvaluationSuite* quantitative_metrics_create_suite(const char* name) {
    EvaluationSuite* suite = (EvaluationSuite*)safe_calloc(1, sizeof(EvaluationSuite));
    
    if (name) {
        strncpy(suite->benchmark_name, name, sizeof(suite->benchmark_name) - 1);
    } else {
        strcpy(suite->benchmark_name, "PCB_IC_Electromagnetic_Simulation_Benchmark");
    }
    
    suite->max_evaluations = DEFAULT_MAX_EVALUATIONS;
    suite->evaluations = (QuantitativeEvaluation*)safe_calloc(
        suite->max_evaluations, sizeof(QuantitativeEvaluation)
    );
    suite->num_evaluations = 0;
    suite->benchmark_reference = 1.0;
    suite->normalize_scores = true;
    suite->enable_weighting = true;
    
    // Set default weights
    suite->accuracy_weight = DEFAULT_ACCURACY_WEIGHT;
    suite->performance_weight = DEFAULT_PERFORMANCE_WEIGHT;
    suite->scalability_weight = DEFAULT_SCALABILITY_WEIGHT;
    suite->stability_weight = DEFAULT_STABILITY_WEIGHT;
    suite->efficiency_weight = DEFAULT_EFFICIENCY_WEIGHT;
    suite->reliability_weight = DEFAULT_RELIABILITY_WEIGHT;
    
    // Set evaluation date and evaluator
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    strftime(suite->evaluation_criteria, sizeof(suite->evaluation_criteria), 
             "Evaluation performed on %Y-%m-%d %H:%M:%S", timeinfo);
    
    return suite;
}

void quantitative_metrics_destroy_suite(EvaluationSuite* suite) {
    if (suite) {
        if (suite->evaluations) {
            safe_free(suite->evaluations);
        }
        safe_free(suite);
    }
}

int quantitative_metrics_add_evaluation(
    EvaluationSuite* suite,
    const QuantitativeEvaluation* evaluation
) {
    if (!suite || !evaluation) return -1;
    
    if (suite->num_evaluations >= suite->max_evaluations) {
        // Resize the array
        int new_max = suite->max_evaluations * 2;
        QuantitativeEvaluation* new_evaluations = (QuantitativeEvaluation*)safe_realloc(
            suite->evaluations, new_max * sizeof(QuantitativeEvaluation)
        );
        if (!new_evaluations) return -1;
        
        suite->evaluations = new_evaluations;
        suite->max_evaluations = new_max;
    }
    
    suite->evaluations[suite->num_evaluations] = *evaluation;
    suite->num_evaluations++;
    
    return 0;
}

int quantitative_metrics_compute_accuracy_metrics(
    QuantitativeEvaluation* eval,
    const Complex* computed_solution,
    const Complex* reference_solution,
    int num_points,
    const double* coordinates
) {
    if (!eval || !computed_solution || !reference_solution || num_points <= 0) return -1;
    
    double sum_squared_error = 0.0;
    double max_absolute_error = 0.0;
    double sum_relative_error = 0.0;
    double sum_absolute_error = 0.0;
    double max_error_magnitude = 0.0;
    double max_phase_error = 0.0;
    
    int max_error_index = 0;
    
    // Compute various error metrics
    for (int i = 0; i < num_points; i++) {
        Complex computed = computed_solution[i];
        Complex reference = reference_solution[i];
        
        // Absolute error
        Complex abs_error;
        abs_error.real = computed.real - reference.real;
        abs_error.imag = computed.imag - reference.imag;
        double abs_error_mag = cabs(abs_error);
        
        // Relative error
        double ref_mag = cabs(reference);
        double rel_error = (ref_mag > MIN_ACCURACY_THRESHOLD) ? 
                          abs_error_mag / ref_mag : abs_error_mag;
        
        // Phase error
        double computed_phase = carg(computed);
        double reference_phase = carg(reference);
        double phase_error = computed_phase - reference_phase;
        // Normalize phase error to [-π, π]
        while (phase_error > M_PI) phase_error -= 2.0 * M_PI;
        while (phase_error < -M_PI) phase_error += 2.0 * M_PI;
        double phase_error_deg = phase_error * 180.0 / M_PI;
        
        // Update accumulators
        sum_squared_error += abs_error_mag * abs_error_mag;
        sum_absolute_error += abs_error_mag;
        sum_relative_error += rel_error;
        
        if (abs_error_mag > max_absolute_error) {
            max_absolute_error = abs_error_mag;
            max_error_index = i;
        }
        
        if (rel_error > max_error_magnitude) {
            max_error_magnitude = rel_error;
        }
        
        if (fabs(phase_error_deg) > fabs(max_phase_error)) {
            max_phase_error = phase_error_deg;
        }
    }
    
    // Compute final metrics
    eval->accuracy.l2_error = sqrt(sum_squared_error / num_points);
    eval->accuracy.l_inf_error = max_absolute_error;
    eval->accuracy.relative_error = sum_relative_error / num_points;
    eval->accuracy.absolute_error = sum_absolute_error / num_points;
    eval->accuracy.rms_error = eval->accuracy.l2_error;
    eval->accuracy.magnitude_error_percent = max_error_magnitude * 100.0;
    eval->accuracy.phase_error_degrees = max_phase_error;
    
    // Store location of maximum error
    if (coordinates && max_error_index * 3 < num_points * 3) {
        eval->accuracy.max_error_location[0] = coordinates[max_error_index * 3];
        eval->accuracy.max_error_location[1] = coordinates[max_error_index * 3 + 1];
        eval->accuracy.max_error_location[2] = coordinates[max_error_index * 3 + 2];
    }
    
    eval->accuracy.error_sample_points = num_points;
    
    // Compute error distribution statistics
    double mean_error = sum_absolute_error / num_points;
    double variance = 0.0;
    for (int i = 0; i < num_points; i++) {
        Complex computed = computed_solution[i];
        Complex reference = reference_solution[i];
        Complex abs_error;
        abs_error.real = computed.real - reference.real;
        abs_error.imag = computed.imag - reference.imag;
        double abs_error_mag = cabs(abs_error);
        variance += (abs_error_mag - mean_error) * (abs_error_mag - mean_error);
    }
    eval->accuracy.error_distribution_std = sqrt(variance / num_points);
    
    return 0;
}

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
) {
    if (!eval || total_time <= 0 || memory_usage < 0) return -1;
    
    eval->performance.total_time_seconds = total_time;
    eval->performance.setup_time_seconds = setup_time;
    eval->performance.solve_time_seconds = solve_time;
    eval->performance.memory_usage_gb = memory_usage;
    eval->performance.memory_peak_gb = memory_peak;
    eval->performance.iterations = iterations;
    eval->performance.thread_count = threads;
    eval->performance.mpi_processes = mpi_processes;
    eval->performance.gpu_accelerated = gpu_accelerated;
    
    // Compute convergence rate (assuming exponential convergence)
    if (iterations > 1) {
        eval->performance.convergence_rate = log(10.0) / iterations;
    } else {
        eval->performance.convergence_rate = 0.0;
    }
    
    // Compute parallel efficiency (if applicable)
    if (threads > 1 || mpi_processes > 1) {
        int total_processes = threads * mpi_processes;
        eval->performance.parallel_speedup = total_processes * 0.8;  // Estimated
        eval->performance.parallel_efficiency = eval->performance.parallel_speedup / total_processes;
    } else {
        eval->performance.parallel_speedup = 1.0;
        eval->performance.parallel_efficiency = 1.0;
    }
    
    // Estimate CPU utilization (simplified)
    eval->performance.cpu_utilization_percent = (threads > 1) ? 85.0 : 95.0;
    
    // Estimate memory bandwidth utilization (simplified)
    eval->performance.memory_bandwidth_utilization = (memory_peak > memory_usage) ? 75.0 : 60.0;
    
    // Estimate cache hit rate (simplified)
    eval->performance.cache_hit_rate = (memory_usage < 1.0) ? 95.0 : 85.0;
    
    return 0;
}

int quantitative_metrics_compute_scalability_metrics(
    QuantitativeEvaluation* eval,
    int num_processes,
    double* execution_times,
    double* memory_usages,
    int num_measurements
) {
    if (!eval || num_processes <= 0 || !execution_times || num_measurements <= 1) return -1;
    
    eval->scalability.min_processes = 1;
    eval->scalability.max_processes = num_processes;
    
    // Compute strong scaling efficiency
    if (num_measurements >= 2) {
        double baseline_time = execution_times[0];
        double scaled_time = execution_times[num_measurements - 1];
        int baseline_processes = 1;
        int scaled_processes = num_processes;
        
        if (scaled_time > 0) {
            eval->scalability.strong_scaling_efficiency = 
                (baseline_time * baseline_processes) / (scaled_time * scaled_processes);
        } else {
            eval->scalability.strong_scaling_efficiency = 0.0;
        }
    }
    
    // Compute weak scaling efficiency
    if (memory_usages && num_measurements >= 2) {
        double baseline_memory = memory_usages[0];
        double scaled_memory = memory_usages[num_measurements - 1];
        
        if (baseline_memory > 0) {
            eval->scalability.weak_scaling_efficiency = 
                scaled_memory / (baseline_memory * num_processes);
        } else {
            eval->scalability.weak_scaling_efficiency = 0.0;
        }
    }
    
    // Compute memory scaling factor
    if (memory_usages && num_measurements >= 2) {
        double memory_ratio = memory_usages[num_measurements - 1] / memory_usages[0];
        eval->scalability.memory_scaling_factor = memory_ratio / num_processes;
    }
    
    // Compute time scaling factor
    if (num_measurements >= 2) {
        double time_ratio = execution_times[num_measurements - 1] / execution_times[0];
        eval->scalability.time_scaling_factor = time_ratio / num_processes;
    }
    
    // Estimate communication overhead
    eval->scalability.communication_overhead_percent = 100.0 * (1.0 - eval->scalability.strong_scaling_efficiency);
    
    // Estimate load imbalance
    eval->scalability.load_imbalance_factor = 0.1;  // Simplified estimate
    
    // Estimate optimal process count
    eval->scalability.optimal_process_count = num_processes / 2;  // Simplified estimate
    
    return 0;
}

int quantitative_metrics_compute_stability_metrics(
    QuantitativeEvaluation* eval,
    const double* residual_history,
    const Complex* solution_history,
    int history_length,
    double time_step,
    double final_residual
) {
    if (!eval || !residual_history || history_length <= 0) return -1;
    
    // Compute residual history stability
    double residual_variance = 0.0;
    double residual_mean = 0.0;
    
    for (int i = 0; i < history_length; i++) {
        residual_mean += residual_history[i];
    }
    residual_mean /= history_length;
    
    for (int i = 0; i < history_length; i++) {
        double deviation = residual_history[i] - residual_mean;
        residual_variance += deviation * deviation;
    }
    residual_variance /= history_length;
    
    eval->stability.residual_history_stability = 1.0 / (1.0 + sqrt(residual_variance));
    
    // Compute condition number (simplified estimate)
    eval->stability.condition_number = 1.0 / (final_residual + MIN_ACCURACY_THRESHOLD);
    
    // Compute convergence stability
    if (history_length >= 2) {
        double convergence_trend = 0.0;
        for (int i = 1; i < history_length; i++) {
            if (residual_history[i-1] > MIN_ACCURACY_THRESHOLD) {
                convergence_trend += residual_history[i] / residual_history[i-1];
            }
        }
        convergence_trend /= (history_length - 1);
        eval->stability.convergence_stability_index = convergence_trend;
    }
    
    // Compute numerical dissipation (for time-domain methods)
    if (solution_history && time_step > 0) {
        double energy_initial = 0.0;
        double energy_final = 0.0;
        
        for (int i = 0; i < history_length; i++) {
            double energy = cabs(solution_history[i]);
            if (i == 0) energy_initial = energy;
            energy_final = energy;
        }
        
        if (energy_initial > MIN_ACCURACY_THRESHOLD) {
            eval->stability.numerical_dissipation_rate = 
                (energy_initial - energy_final) / energy_initial;
        }
    }
    
    // Estimate mesh quality factor
    eval->stability.mesh_quality_factor = 0.9;  // Simplified estimate
    
    // Estimate solver stability margin
    eval->stability.solver_stability_margin = 0.8;  // Simplified estimate
    
    // Count stability warnings and failures
    eval->stability.stability_warnings = 0;
    eval->stability.stability_failures = 0;
    
    for (int i = 0; i < history_length; i++) {
        if (residual_history[i] > 1.0) {
            eval->stability.stability_warnings++;
        }
        if (isnan(residual_history[i]) || isinf(residual_history[i])) {
            eval->stability.stability_failures++;
        }
    }
    
    return 0;
}

int quantitative_metrics_compute_efficiency_metrics(
    QuantitativeEvaluation* eval,
    double theoretical_peak_performance,
    double theoretical_peak_memory_bandwidth,
    double theoretical_peak_cache_bandwidth,
    double achieved_performance,
    double achieved_memory_bandwidth,
    double achieved_cache_bandwidth
) {
    if (!eval || theoretical_peak_performance <= 0) return -1;
    
    // Compute computational efficiency
    eval->efficiency.computational_efficiency = 
        achieved_performance / theoretical_peak_performance;
    
    // Compute memory efficiency
    if (theoretical_peak_memory_bandwidth > 0) {
        eval->efficiency.memory_efficiency = 
            achieved_memory_bandwidth / theoretical_peak_memory_bandwidth;
    }
    
    // Compute cache efficiency
    if (theoretical_peak_cache_bandwidth > 0) {
        eval->efficiency.cache_efficiency = 
            achieved_cache_bandwidth / theoretical_peak_cache_bandwidth;
    }
    
    // Estimate algorithmic efficiency
    eval->efficiency.algorithmic_efficiency = 0.85;  // Simplified estimate
    
    // Estimate numerical efficiency
    eval->efficiency.numerical_efficiency = 0.90;  // Simplified estimate
    
    // Estimate parallel efficiency
    eval->efficiency.parallel_efficiency = eval->performance.parallel_efficiency;
    
    // Estimate load balancing efficiency
    eval->efficiency.load_balancing_efficiency = 0.88;  // Simplified estimate
    
    // Estimate vectorization efficiency
    eval->efficiency.vectorization_efficiency = 0.75;  // Simplified estimate
    
    // Estimate memory access efficiency
    eval->efficiency.memory_access_efficiency = eval->efficiency.memory_efficiency;
    
    return 0;
}

int quantitative_metrics_compute_reliability_metrics(
    QuantitativeEvaluation* eval,
    int num_runs,
    double* execution_times,
    double* accuracy_results,
    int* convergence_status,
    int* error_codes
) {
    if (!eval || num_runs <= 0 || !execution_times) return -1;
    
    int successful_runs = 0;
    int failed_runs = 0;
    int recovered_runs = 0;
    
    // Count successful and failed runs
    for (int i = 0; i < num_runs; i++) {
        if (convergence_status && convergence_status[i] == 1) {
            successful_runs++;
        } else if (error_codes && error_codes[i] != 0) {
            failed_runs++;
        } else {
            recovered_runs++;
        }
    }
    
    eval->reliability.successful_computations = successful_runs;
    eval->reliability.failed_computations = failed_runs;
    eval->reliability.recovered_computations = recovered_runs;
    
    // Compute reliability score
    if (num_runs > 0) {
        eval->reliability.reliability_score = (double)successful_runs / num_runs;
        eval->reliability.error_recovery_rate = (double)recovered_runs / (failed_runs + recovered_runs);
    }
    
    // Compute consistency score (based on execution time variation)
    if (num_runs >= 2) {
        double mean_time = 0.0;
        for (int i = 0; i < num_runs; i++) {
            mean_time += execution_times[i];
        }
        mean_time /= num_runs;
        
        double variance = 0.0;
        for (int i = 0; i < num_runs; i++) {
            double deviation = execution_times[i] - mean_time;
            variance += deviation * deviation;
        }
        variance /= num_runs;
        
        double coefficient_of_variation = sqrt(variance) / mean_time;
        eval->reliability.consistency_score = 1.0 / (1.0 + coefficient_of_variation);
    }
    
    // Estimate robustness score
    eval->reliability.robustness_score = 0.9;  // Simplified estimate
    
    // Estimate repeatability score
    eval->reliability.repeatability_score = eval->reliability.consistency_score;
    
    // Estimate fault tolerance score
    eval->reliability.fault_tolerance_score = eval->reliability.error_recovery_rate;
    
    // Estimate mean time between failures (simplified)
    eval->reliability.mean_time_between_failures = mean_time * successful_runs;
    
    // Estimate mean time to recovery (simplified)
    eval->reliability.mean_time_to_recovery = mean_time * 2.0;  // Simplified estimate
    
    return 0;
}

double quantitative_metrics_compute_overall_score(
    const QuantitativeEvaluation* eval,
    double accuracy_weight,
    double performance_weight,
    double scalability_weight,
    double stability_weight,
    double efficiency_weight,
    double reliability_weight
) {
    if (!eval) return 0.0;
    
    // Normalize weights
    double total_weight = accuracy_weight + performance_weight + scalability_weight + 
                         stability_weight + efficiency_weight + reliability_weight;
    if (total_weight <= 0) return 0.0;
    
    accuracy_weight /= total_weight;
    performance_weight /= total_weight;
    scalability_weight /= total_weight;
    stability_weight /= total_weight;
    efficiency_weight /= total_weight;
    reliability_weight /= total_weight;
    
    // Compute weighted score
    double accuracy_score = 1.0 / (1.0 + eval->accuracy.l2_error + eval->accuracy.relative_error);
    double performance_score = 1.0 / (1.0 + eval->performance.total_time_seconds / 100.0);
    double scalability_score = eval->scalability.strong_scaling_efficiency;
    double stability_score = eval->stability.residual_history_stability;
    double efficiency_score = eval->efficiency.computational_efficiency;
    double reliability_score = eval->reliability.reliability_score;
    
    double overall_score = 
        accuracy_weight * accuracy_score +
        performance_weight * performance_score +
        scalability_weight * scalability_score +
        stability_weight * stability_score +
        efficiency_weight * efficiency_score +
        reliability_weight * reliability_score;
    
    return overall_score;
}

int quantitative_metrics_rank_evaluations(EvaluationSuite* suite) {
    if (!suite || suite->num_evaluations <= 0) return -1;
    
    // Compute overall scores for all evaluations
    for (int i = 0; i < suite->num_evaluations; i++) {
        suite->evaluations[i].overall_score = quantitative_metrics_compute_overall_score(
            &suite->evaluations[i],
            suite->accuracy_weight,
            suite->performance_weight,
            suite->scalability_weight,
            suite->stability_weight,
            suite->efficiency_weight,
            suite->reliability_weight
        );
    }
    
    // Sort evaluations by overall score (descending)
    for (int i = 0; i < suite->num_evaluations - 1; i++) {
        for (int j = i + 1; j < suite->num_evaluations; j++) {
            if (suite->evaluations[i].overall_score < suite->evaluations[j].overall_score) {
                QuantitativeEvaluation temp = suite->evaluations[i];
                suite->evaluations[i] = suite->evaluations[j];
                suite->evaluations[j] = temp;
            }
        }
    }
    
    // Assign rankings
    for (int i = 0; i < suite->num_evaluations; i++) {
        suite->evaluations[i].ranking = i + 1;
    }
    
    return 0;
}

int quantitative_metrics_export_to_csv(
    const EvaluationSuite* suite,
    const char* filename
) {
    if (!suite || !filename) return -1;
    
    FILE* file = fopen(filename, "w");
    if (!file) return -1;
    
    // Write header
    fprintf(file, "Test Case,Algorithm,Problem Type,Frequency (Hz),");
    fprintf(file, "Mesh Elements,Unknowns,Complexity Score,");
    fprintf(file, "L2 Error,L-inf Error,Relative Error,RMS Error,");
    fprintf(file, "Phase Error (deg),Magnitude Error (%%),");
    fprintf(file, "Total Time (s),Setup Time (s),Solve Time (s),");
    fprintf(file, "Memory Usage (GB),Memory Peak (GB),Iterations,");
    fprintf(file, "Convergence Rate,Parallel Speedup,Parallel Efficiency,");
    fprintf(file, "Strong Scaling,Weak Scaling,Memory Scaling,");
    fprintf(file, "Condition Number,Residual Stability,Convergence Stability,");
    fprintf(file, "Computational Efficiency,Memory Efficiency,Cache Efficiency,");
    fprintf(file, "Reliability Score,Consistency Score,Overall Score,Ranking\n");
    
    // Write data rows
    for (int i = 0; i < suite->num_evaluations; i++) {
        const QuantitativeEvaluation* eval = &suite->evaluations[i];
        
        fprintf(file, "%s,%s,%s,%.2e,",
                eval->test_info.test_case_name,
                eval->test_info.algorithm_name,
                eval->test_info.problem_type,
                eval->test_info.frequency_hz);
        
        fprintf(file, "%d,%d,%d,",
                eval->test_info.mesh_elements,
                eval->test_info.unknowns,
                eval->test_info.complex_geometry ? 1 : 0);
        
        fprintf(file, "%.6e,%.6e,%.6e,%.6e,",
                eval->accuracy.l2_error,
                eval->accuracy.l_inf_error,
                eval->accuracy.relative_error,
                eval->accuracy.rms_error);
        
        fprintf(file, "%.2f,%.2f,",
                eval->accuracy.phase_error_degrees,
                eval->accuracy.magnitude_error_percent);
        
        fprintf(file, "%.2f,%.2f,%.2f,",
                eval->performance.total_time_seconds,
                eval->performance.setup_time_seconds,
                eval->performance.solve_time_seconds);
        
        fprintf(file, "%.2f,%.2f,%d,",
                eval->performance.memory_usage_gb,
                eval->performance.memory_peak_gb,
                eval->performance.iterations);
        
        fprintf(file, "%.4f,%.2f,%.2f,",
                eval->performance.convergence_rate,
                eval->performance.parallel_speedup,
                eval->performance.parallel_efficiency);
        
        fprintf(file, "%.2f,%.2f,%.2f,",
                eval->scalability.strong_scaling_efficiency,
                eval->scalability.weak_scaling_efficiency,
                eval->scalability.memory_scaling_factor);
        
        fprintf(file, "%.2e,%.3f,%.3f,",
                eval->stability.condition_number,
                eval->stability.residual_history_stability,
                eval->stability.convergence_stability_index);
        
        fprintf(file, "%.3f,%.3f,%.3f,",
                eval->efficiency.computational_efficiency,
                eval->efficiency.memory_efficiency,
                eval->efficiency.cache_efficiency);
        
        fprintf(file, "%.3f,%.3f,%.3f,%d\n",
                eval->reliability.reliability_score,
                eval->reliability.consistency_score,
                eval->overall_score,
                eval->ranking);
    }
    
    fclose(file);
    return 0;
}

int quantitative_metrics_generate_report(
    const EvaluationSuite* suite,
    const char* filename,
    ReportFormat format
) {
    if (!suite || !filename) return -1;
    
    FILE* file = fopen(filename, "w");
    if (!file) return -1;
    
    switch (format) {
        case REPORT_FORMAT_TEXT:
            // Generate text report
            fprintf(file, "=== PCB/IC Electromagnetic Simulation Evaluation Report ===\n");
            fprintf(file, "Benchmark: %s\n", suite->benchmark_name);
            fprintf(file, "Evaluation Criteria: %s\n", suite->evaluation_criteria);
            fprintf(file, "Number of Evaluations: %d\n", suite->num_evaluations);
            fprintf(file, "\n");
            
            for (int i = 0; i < suite->num_evaluations; i++) {
                const QuantitativeEvaluation* eval = &suite->evaluations[i];
                fprintf(file, "Rank %d: %s (%s)\n", eval->ranking, 
                        eval->test_info.algorithm_name, eval->test_info.test_case_name);
                fprintf(file, "  Overall Score: %.3f\n", eval->overall_score);
                fprintf(file, "  L2 Error: %.2e\n", eval->accuracy.l2_error);
                fprintf(file, "  Total Time: %.2f s\n", eval->performance.total_time_seconds);
                fprintf(file, "  Memory Usage: %.2f GB\n", eval->performance.memory_usage_gb);
                fprintf(file, "\n");
            }
            break;
            
        case REPORT_FORMAT_CSV:
            return quantitative_metrics_export_to_csv(suite, filename);
            
        default:
            fclose(file);
            return -1;
    }
    
    fclose(file);
    return 0;
}

double quantitative_metrics_get_metric_value(
    const QuantitativeEvaluation* eval,
    MetricType type,
    int sub_metric
) {
    if (!eval) return 0.0;
    
    switch (type) {
        case METRIC_TYPE_ACCURACY:
            switch (sub_metric) {
                case ACCURACY_METRIC_L2_ERROR:
                    return eval->accuracy.l2_error;
                case ACCURACY_METRIC_L_INF_ERROR:
                    return eval->accuracy.l_inf_error;
                case ACCURACY_METRIC_RELATIVE_ERROR:
                    return eval->accuracy.relative_error;
                case ACCURACY_METRIC_RMS_ERROR:
                    return eval->accuracy.rms_error;
                case ACCURACY_METRIC_PHASE_ERROR:
                    return eval->accuracy.phase_error_degrees;
                case ACCURACY_METRIC_MAGNITUDE_ERROR:
                    return eval->accuracy.magnitude_error_percent;
                default:
                    return 0.0;
            }
            
        case METRIC_TYPE_PERFORMANCE:
            switch (sub_metric) {
                case PERFORMANCE_METRIC_EXECUTION_TIME:
                    return eval->performance.total_time_seconds;
                case PERFORMANCE_METRIC_SETUP_TIME:
                    return eval->performance.setup_time_seconds;
                case PERFORMANCE_METRIC_SOLVE_TIME:
                    return eval->performance.solve_time_seconds;
                case PERFORMANCE_METRIC_MEMORY_USAGE:
                    return eval->performance.memory_usage_gb;
                case PERFORMANCE_METRIC_ITERATIONS:
                    return eval->performance.iterations;
                case PERFORMANCE_METRIC_PARALLEL_EFFICIENCY:
                    return eval->performance.parallel_efficiency;
                default:
                    return 0.0;
            }
            
        case METRIC_TYPE_SCALABILITY:
            switch (sub_metric) {
                case SCALABILITY_METRIC_STRONG_SCALING:
                    return eval->scalability.strong_scaling_efficiency;
                case SCALABILITY_METRIC_WEAK_SCALING:
                    return eval->scalability.weak_scaling_efficiency;
                default:
                    return 0.0;
            }
            
        default:
            return 0.0;
    }
}

int quantitative_metrics_set_weights(
    EvaluationSuite* suite,
    double accuracy_weight,
    double performance_weight,
    double scalability_weight,
    double stability_weight,
    double efficiency_weight,
    double reliability_weight
) {
    if (!suite) return -1;
    
    // Validate weights (all must be non-negative)
    if (accuracy_weight < 0 || performance_weight < 0 || scalability_weight < 0 ||
        stability_weight < 0 || efficiency_weight < 0 || reliability_weight < 0) {
        return -1;
    }
    
    suite->accuracy_weight = accuracy_weight;
    suite->performance_weight = performance_weight;
    suite->scalability_weight = scalability_weight;
    suite->stability_weight = stability_weight;
    suite->efficiency_weight = efficiency_weight;
    suite->reliability_weight = reliability_weight;
    
    return 0;
}