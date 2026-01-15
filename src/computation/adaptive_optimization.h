/******************************************************************************
 * Adaptive Calculation Process Optimization
 * 
 * Dynamic optimization of electromagnetic simulation parameters based on
 * problem characteristics, accuracy requirements, and computational resources
 ******************************************************************************/

#ifndef ADAPTIVE_OPTIMIZATION_H
#define ADAPTIVE_OPTIMIZATION_H

#include "structure_algorithms.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Optimization Objectives
 ******************************************************************************/
typedef enum {
    OPTIMIZATION_OBJECTIVE_SPEED = 0,
    OPTIMIZATION_OBJECTIVE_ACCURACY,
    OPTIMIZATION_OBJECTIVE_MEMORY,
    OPTIMIZATION_OBJECTIVE_BALANCED,
    OPTIMIZATION_OBJECTIVE_CUSTOM
} OptimizationObjective;

/******************************************************************************
 * Adaptive Strategy Types
 ******************************************************************************/
typedef enum {
    ADAPTIVE_STRATEGY_NONE = 0,
    ADAPTIVE_STRATEGY_MESH_REFINEMENT,
    ADAPTIVE_STRATEGY_ORDER_INCREASE,
    ADAPTIVE_STRATEGY_DOMAIN_DECOMPOSITION,
    ADAPTIVE_STRATEGY_PRECONDITIONER_UPDATE,
    ADAPTIZATION_STRATEGY_SOLVER_SWITCH,
    ADAPTIVE_STRATEGY_PARALLELIZATION,
    ADAPTIVE_STRATEGY_HYBRID
} AdaptiveStrategy;

/******************************************************************************
 * Convergence Criteria
 ******************************************************************************/
typedef struct {
    /* Residual-based criteria */
    double relative_residual_tolerance;
    double absolute_residual_tolerance;
    double initial_residual;
    double current_residual;
    
    /* Solution change criteria */
    double relative_solution_tolerance;
    double absolute_solution_tolerance;
    double max_solution_change;
    
    /* Eigenvalue convergence */
    double eigenvalue_tolerance;
    double eigenvector_tolerance;
    uint32_t num_converged_eigenvalues;
    
    /* Energy-based criteria */
    double energy_error_tolerance;
    double energy_change_tolerance;
    double current_energy;
    double previous_energy;
    
    /* Iteration limits */
    uint32_t max_iterations;
    uint32_t max_refinement_levels;
    uint32_t current_iteration;
    uint32_t current_refinement_level;
    
    /* Time-based criteria */
    double max_wall_time;
    double current_wall_time;
    double start_time;
    
} ConvergenceCriteria;

/******************************************************************************
 * Adaptive Parameters
 ******************************************************************************/
typedef struct {
    /* Mesh parameters */
    double initial_mesh_size;
    double minimum_mesh_size;
    double maximum_mesh_size;
    double mesh_refinement_ratio;
    uint32_t max_mesh_levels;
    
    /* Solver parameters */
    double initial_tolerance;
    double minimum_tolerance;
    double maximum_tolerance;
    double tolerance_reduction_factor;
    
    /* Preconditioner parameters */
    uint32_t preconditioner_update_frequency;
    double preconditioner_drop_tolerance;
    uint32_t preconditioner_fill_level;
    
    /* Parallel parameters */
    uint32_t initial_num_threads;
    uint32_t max_num_threads;
    uint32_t load_balancing_frequency;
    
    /* Algorithm parameters */
    AlgorithmType primary_algorithm;
    AlgorithmType fallback_algorithm;
    uint32_t algorithm_switch_threshold;
    
} AdaptiveParameters;

/******************************************************************************
 * Performance Metrics
 ******************************************************************************/
typedef struct {
    /* Timing metrics */
    double setup_time;
    double solve_time;
    double total_time;
    double iteration_time;
    double preconditioner_time;
    double assembly_time;
    
    /* Memory metrics */
    double peak_memory_usage;
    double average_memory_usage;
    double memory_efficiency;
    uint64_t memory_allocations;
    uint64_t memory_deallocations;
    
    /* Accuracy metrics */
    double final_residual;
    double final_solution_error;
    double estimated_error;
    double effectivity_index;
    
    /* Convergence metrics */
    uint32_t iterations_required;
    uint32_t refinement_levels_used;
    double convergence_rate;
    bool converged;
    
    /* Scalability metrics */
    double parallel_efficiency;
    double speedup_factor;
    double load_imbalance;
    
} PerformanceMetrics;

/******************************************************************************
 * Adaptive Controller
 ******************************************************************************/
typedef struct {
    /* Current state */
    AdaptiveStrategy current_strategy;
    OptimizationObjective objective;
    ConvergenceCriteria convergence_criteria;
    AdaptiveParameters parameters;
    PerformanceMetrics metrics;
    
    /* Problem characteristics */
    StructureCategory structure_category;
    uint32_t structure_type;
    double frequency;
    double problem_size;
    double complexity_estimate;
    
    /* Resource constraints */
    double available_memory;
    double available_time;
    uint32_t available_cores;
    
    /* Adaptation history */
    uint32_t adaptation_count;
    double* error_history;
    double* time_history;
    double* memory_history;
    uint32_t history_size;
    uint32_t max_history_size;
    
} AdaptiveController;

/******************************************************************************
 * Optimization Functions
 ******************************************************************************/

/* Controller initialization */
AdaptiveController* create_adaptive_controller(StructureCategory category, uint32_t type,
                                             double frequency, OptimizationObjective objective);
void destroy_adaptive_controller(AdaptiveController* controller);
void reset_adaptive_controller(AdaptiveController* controller);

/* Parameter adaptation */
bool adapt_mesh_parameters(AdaptiveController* controller, double current_error, double target_error);
bool adapt_solver_parameters(AdaptiveController* controller, double current_residual, double target_residual);
bool adapt_preconditioner(AdaptiveController* controller, uint32_t iteration_count, double convergence_rate);
bool adapt_parallelization(AdaptiveController* controller, double load_imbalance, double parallel_efficiency);
bool adapt_algorithm_selection(AdaptiveController* controller, double current_performance, double target_performance);

/* Convergence monitoring */
bool check_convergence(AdaptiveController* controller);
ConvergenceCriteria update_convergence_criteria(AdaptiveController* controller, double current_error);
double estimate_convergence_rate(AdaptiveController* controller);
double predict_solution_time(AdaptiveController* controller, uint32_t remaining_iterations);

/* Performance optimization */
PerformanceMetrics optimize_performance(AdaptiveController* controller);
AdaptiveParameters optimize_parameters(AdaptiveController* controller, const PerformanceMetrics* current_metrics);
AdaptiveStrategy select_optimal_strategy(AdaptiveController* controller, const PerformanceMetrics* metrics);

/* Error estimation and control */
double estimate_discretization_error(AdaptiveController* controller, const double* solution, uint32_t size);
double estimate_modeling_error(AdaptiveController* controller, const double* solution, uint32_t size);
double estimate_total_error(AdaptiveController* controller, const double* solution, uint32_t size);
bool control_error_propagation(AdaptiveController* controller, double estimated_error, double target_error);

/* Resource management */
bool optimize_memory_usage(AdaptiveController* controller, double memory_limit);
bool optimize_computation_time(AdaptiveController* controller, double time_limit);
bool balance_accuracy_speed_tradeoff(AdaptiveController* controller, double accuracy_weight, double speed_weight);

/* Machine learning integration */
typedef struct {
    double* feature_vector;
    uint32_t feature_size;
    double predicted_performance;
    double confidence_level;
    double training_accuracy;
    uint64_t training_samples;
} MLModelPrediction;

MLModelPrediction predict_optimal_parameters(AdaptiveController* controller, const double* features, uint32_t feature_size);
bool update_ml_model(AdaptiveController* controller, const double* features, uint32_t feature_size,
                    const PerformanceMetrics* actual_metrics);

/* Advanced optimization strategies */
typedef struct {
    /* Multi-objective optimization */
    double* objective_weights;
    uint32_t num_objectives;
    
    /* Pareto frontier */
    double** pareto_solutions;
    double* pareto_objectives;
    uint32_t pareto_size;
    
    /* Genetic algorithm parameters */
    uint32_t population_size;
    uint32_t num_generations;
    double crossover_probability;
    double mutation_probability;
    
} MultiObjectiveOptimization;

MultiObjectiveOptimization* create_multi_objective_optimization(uint32_t num_objectives);
void destroy_multi_objective_optimization(MultiObjectiveOptimization* opt);
bool optimize_multi_objective(AdaptiveController* controller, MultiObjectiveOptimization* opt);

/* Real-time adaptation */
typedef struct {
    /* Real-time constraints */
    double real_time_deadline;
    double quality_of_service_requirement;
    uint32_t max_adaptation_latency;
    
    /* Monitoring data */
    double* performance_stream;
    double* resource_stream;
    uint32_t stream_size;
    uint64_t timestamp;
    
    /* Adaptation triggers */
    double performance_threshold;
    double resource_threshold;
    uint32_t adaptation_frequency;
    
} RealTimeAdaptation;

RealTimeAdaptation* create_real_time_adaptation(double deadline, double qos_requirement);
void destroy_real_time_adaptation(RealTimeAdaptation* rt);
bool adapt_in_real_time(AdaptiveController* controller, RealTimeAdaptation* rt);

/* Performance profiling and analysis */
typedef struct {
    /* Profiling data */
    double* execution_times;
    double* memory_usage;
    double* accuracy_measures;
    uint32_t* iteration_counts;
    uint32_t profile_size;
    
    /* Statistical analysis */
    double mean_time;
    double stddev_time;
    double mean_memory;
    double stddev_memory;
    double mean_accuracy;
    double stddev_accuracy;
    
    /* Bottleneck analysis */
    double setup_bottleneck;
    double solve_bottleneck;
    double assembly_bottleneck;
    double io_bottleneck;
    
} PerformanceProfile;

PerformanceProfile* create_performance_profile(uint32_t max_samples);
void destroy_performance_profile(PerformanceProfile* profile);
bool update_performance_profile(PerformanceProfile* profile, const PerformanceMetrics* metrics);
bool analyze_performance_bottlenecks(PerformanceProfile* profile, AdaptiveController* controller);

/* Export/Import functionality */
bool export_adaptive_controller(const AdaptiveController* controller, const char* filename);
AdaptiveController* import_adaptive_controller(const char* filename);
bool export_optimization_history(const AdaptiveController* controller, const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* ADAPTIVE_OPTIMIZATION_H */