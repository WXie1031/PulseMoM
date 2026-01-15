#ifndef ADAPTIVE_CALCULATION_H
#define ADAPTIVE_CALCULATION_H

#include "../geometry/pcb_ic_structures.h"
#include "../mom/mom_solver.h"
#include "../fem/fem_solver.h"
#include "../fdtd/fdtd_solver.h"
#include "../peec/peec_solver.h"

typedef enum {
    ALGORITHM_AUTO_SELECT,
    ALGORITHM_MOM_BASIC,
    ALGORITHM_MOM_FAST_MULTIPOLE,
    ALGORITHM_MOM_ADAPTIVE_CROSS_APPROXIMATION,
    ALGORITHM_FEM_STANDARD,
    ALGORITHM_FEM_HIGHER_ORDER,
    ALGORITHM_FEM_DISCONTINUOUS_GALERKIN,
    ALGORITHM_FDTD_STANDARD,
    ALGORITHM_FDTD_SUBGRID,
    ALGORITHM_FDTD_CONFORMAL,
    ALGORITHM_PEEC_BASIC,
    ALGORITHM_PEEC_ENHANCED,
    ALGORITHM_PEEC_WAVELET,
    ALGORITHM_HYBRID_MOM_FEM,
    ALGORITHM_HYBRID_MOM_FDTD,
    ALGORITHM_HYBRID_FEM_FDTD,
    ALGORITHM_ASYMPTOTIC_HIGH_FREQUENCY,
    ALGORITHM_ASYMPTOTIC_LOW_FREQUENCY,
    ALGORITHM_ASYMPTOTIC_QUASI_STATIC
} AlgorithmType;

typedef enum {
    OPTIMIZATION_LEVEL_NONE = 0,
    OPTIMIZATION_LEVEL_BASIC,
    OPTIMIZATION_LEVEL_INTERMEDIATE,
    OPTIMIZATION_LEVEL_ADVANCED,
    OPTIMIZATION_LEVEL_AGGRESSIVE
} OptimizationLevel;

typedef enum {
    ACCURACY_LEVEL_COARSE = 0,
    ACCURACY_LEVEL_NORMAL,
    ACCURACY_LEVEL_FINE,
    ACCURACY_LEVEL_VERY_FINE,
    ACCURACY_LEVEL_ULTRA_FINE
} AccuracyLevel;

typedef struct {
    double frequency_min;
    double frequency_max;
    double structure_size_min;
    double structure_size_max;
    int mesh_elements;
    int unknowns;
    double memory_required_gb;
    double estimated_time_seconds;
    int complexity_score;
    PCBICStructureType structure_type;
    MaterialType material_type;
    BoundaryCondition boundary_condition;
} ProblemCharacteristics;

typedef struct {
    AlgorithmType algorithm;
    double expected_accuracy;
    double expected_time;
    double memory_usage_gb;
    double parallel_efficiency;
    int recommended_threads;
    bool supports_gpu;
    bool supports_mpi;
    OptimizationLevel optimization_level;
} AlgorithmRecommendation;

typedef struct {
    double total_time_seconds;
    double setup_time_seconds;
    double solve_time_seconds;
    double memory_peak_gb;
    double memory_average_gb;
    int iterations;
    double final_residual;
    double accuracy_achieved;
    int threads_used;
    bool gpu_accelerated;
    bool mpi_parallel;
} PerformanceMetrics;

typedef struct {
    ProblemCharacteristics problem;
    AlgorithmRecommendation recommendation;
    PerformanceMetrics metrics;
    bool adaptive_enabled;
    OptimizationLevel current_optimization;
    AccuracyLevel current_accuracy;
    double convergence_tolerance;
    int max_iterations;
    bool dynamic_refinement;
    double refinement_threshold;
} AdaptiveCalculationContext;

typedef struct {
    double frequency_adaptivity_threshold;
    double size_adaptivity_threshold;
    double complexity_adaptivity_threshold;
    double accuracy_adaptivity_threshold;
    double memory_adaptivity_threshold;
    double time_adaptivity_threshold;
    int min_refinement_level;
    int max_refinement_level;
    double refinement_ratio;
    double coarsening_ratio;
    bool enable_h_adaptivity;
    bool enable_p_adaptivity;
    bool enable_hp_adaptivity;
} AdaptiveParameters;

AdaptiveCalculationContext* adaptive_calculation_create_context(void);
void adaptive_calculation_destroy_context(AdaptiveCalculationContext* context);

int adaptive_calculation_analyze_problem(
    AdaptiveCalculationContext* context,
    const PCBICStructure* structure,
    double frequency_min,
    double frequency_max,
    AccuracyLevel target_accuracy
);

AlgorithmRecommendation adaptive_calculation_select_algorithm(
    const AdaptiveCalculationContext* context,
    const ProblemCharacteristics* problem
);

int adaptive_calculation_optimize_parameters(
    AdaptiveCalculationContext* context,
    const AlgorithmRecommendation* recommendation
);

int adaptive_calculation_execute(
    AdaptiveCalculationContext* context,
    PCBICStructure* structure,
    double frequency,
    Complex* result
);

int adaptive_calculation_monitor_convergence(
    AdaptiveCalculationContext* context,
    const PerformanceMetrics* current_metrics
);

int adaptive_calculation_adapt_mesh(
    AdaptiveCalculationContext* context,
    PCBICStructure* structure
);

int adaptive_calculation_refine_solution(
    AdaptiveCalculationContext* context,
    PCBICStructure* structure,
    Complex* current_solution
);

int adaptive_calculation_estimate_error(
    const AdaptiveCalculationContext* context,
    const Complex* solution,
    double* error_estimate
);

int adaptive_calculation_get_recommendations(
    const AdaptiveCalculationContext* context,
    AlgorithmRecommendation** recommendations,
    int* num_recommendations
);

void adaptive_calculation_print_statistics(
    const AdaptiveCalculationContext* context,
    FILE* output
);

int adaptive_calculation_export_metrics(
    const AdaptiveCalculationContext* context,
    const char* filename
);

int adaptive_calculation_import_metrics(
    AdaptiveCalculationContext* context,
    const char* filename
);

void adaptive_calculation_reset_context(AdaptiveCalculationContext* context);

bool adaptive_calculation_is_algorithm_supported(
    AlgorithmType algorithm,
    const ProblemCharacteristics* problem
);

double adaptive_calculation_estimate_memory_usage(
    AlgorithmType algorithm,
    const ProblemCharacteristics* problem
);

double adaptive_calculation_estimate_computation_time(
    AlgorithmType algorithm,
    const ProblemCharacteristics* problem
);

int adaptive_calculation_set_parameters(
    AdaptiveCalculationContext* context,
    const AdaptiveParameters* parameters
);

int adaptive_calculation_get_parameters(
    const AdaptiveCalculationContext* context,
    AdaptiveParameters* parameters
);

#endif