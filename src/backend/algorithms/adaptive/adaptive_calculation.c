#include "adaptive_calculation.h"
#include "../../../utils/memory.h"
#include "../../../utils/math_utils.h"
#include "../../../utils/performance/performance_monitor.h"
// Implementation file can include L5 layer headers (higher layer dependency is allowed in .c files)
#include "../../../solvers/mom/mom_solver.h"
#include "../../../solvers/peec/peec_solver.h"
// Note: fem_solver and fdtd_solver may not exist yet - will need to be implemented or removed
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALGORITHMS 32
#define MAX_REFINEMENT_LEVELS 10
#define DEFAULT_CONVERGENCE_TOLERANCE 1e-6
#define DEFAULT_MAX_ITERATIONS 1000
#define ADAPTIVITY_THRESHOLD 0.1

static const double ALGORITHM_ACCURACY_TABLE[MAX_ALGORITHMS][5] = {
    {0.95, 0.98, 0.99, 0.995, 0.999},  // AUTO_SELECT
    {0.90, 0.95, 0.98, 0.99, 0.995},    // MOM_BASIC
    {0.92, 0.96, 0.985, 0.992, 0.997},  // MOM_FAST_MULTIPOLE
    {0.94, 0.97, 0.988, 0.994, 0.998},  // MOM_ADAPTIVE_CROSS_APPROXIMATION
    {0.88, 0.93, 0.97, 0.985, 0.992},  // FEM_STANDARD
    {0.92, 0.96, 0.985, 0.992, 0.997},  // FEM_HIGHER_ORDER
    {0.90, 0.94, 0.98, 0.99, 0.995},    // FEM_DISCONTINUOUS_GALERKIN
    {0.85, 0.90, 0.95, 0.98, 0.99},     // FDTD_STANDARD
    {0.87, 0.92, 0.96, 0.982, 0.992},   // FDTD_SUBGRID
    {0.89, 0.93, 0.97, 0.985, 0.994},   // FDTD_CONFORMAL
    {0.88, 0.92, 0.96, 0.98, 0.99},     // PEEC_BASIC
    {0.91, 0.95, 0.98, 0.99, 0.996},    // PEEC_ENHANCED
    {0.93, 0.97, 0.99, 0.994, 0.998},   // PEEC_WAVELET
    {0.94, 0.97, 0.99, 0.995, 0.999},   // HYBRID_MOM_FEM
    {0.93, 0.96, 0.988, 0.993, 0.997},  // HYBRID_MOM_FDTD
    {0.92, 0.95, 0.985, 0.992, 0.996},  // HYBRID_FEM_FDTD
    {0.80, 0.85, 0.90, 0.95, 0.98},     // ASYMPTOTIC_HIGH_FREQUENCY
    {0.85, 0.90, 0.95, 0.98, 0.99},     // ASYMPTOTIC_LOW_FREQUENCY
    {0.82, 0.87, 0.92, 0.96, 0.985}     // ASYMPTOTIC_QUASI_STATIC
};

static const double ALGORITHM_SPEED_TABLE[MAX_ALGORITHMS][5] = {
    {1.0, 1.0, 1.0, 1.0, 1.0},           // AUTO_SELECT (normalized)
    {0.8, 0.7, 0.6, 0.5, 0.4},          // MOM_BASIC
    {2.5, 2.0, 1.8, 1.5, 1.2},          // MOM_FAST_MULTIPOLE
    {3.0, 2.5, 2.2, 1.8, 1.5},          // MOM_ADAPTIVE_CROSS_APPROXIMATION
    {1.2, 1.0, 0.8, 0.6, 0.5},          // FEM_STANDARD
    {0.9, 0.7, 0.6, 0.5, 0.4},          // FEM_HIGHER_ORDER
    {1.5, 1.2, 1.0, 0.8, 0.7},          // FEM_DISCONTINUOUS_GALERKIN
    {4.0, 3.5, 3.0, 2.5, 2.0},          // FDTD_STANDARD
    {3.2, 2.8, 2.4, 2.0, 1.6},         // FDTD_SUBGRID
    {2.8, 2.4, 2.0, 1.7, 1.4},         // FDTD_CONFORMAL
    {1.8, 1.5, 1.3, 1.1, 0.9},         // PEEC_BASIC
    {2.2, 1.9, 1.6, 1.3, 1.1},         // PEEC_ENHANCED
    {2.8, 2.4, 2.0, 1.6, 1.3},         // PEEC_WAVELET
    {1.5, 1.3, 1.1, 0.9, 0.8},         // HYBRID_MOM_FEM
    {1.8, 1.5, 1.3, 1.1, 0.9},         // HYBRID_MOM_FDTD
    {2.0, 1.7, 1.4, 1.2, 1.0},         // HYBRID_FEM_FDTD
    {8.0, 7.0, 6.0, 5.0, 4.0},         // ASYMPTOTIC_HIGH_FREQUENCY
    {6.0, 5.0, 4.0, 3.5, 3.0},         // ASYMPTOTIC_LOW_FREQUENCY
    {7.0, 6.0, 5.0, 4.2, 3.5}          // ASYMPTOTIC_QUASI_STATIC
};

AdaptiveCalculationContext* adaptive_calculation_create_context(void) {
    AdaptiveCalculationContext* context = (AdaptiveCalculationContext*)safe_calloc(1, sizeof(AdaptiveCalculationContext));
    
    context->adaptive_enabled = true;
    context->current_optimization = OPTIMIZATION_LEVEL_INTERMEDIATE;
    context->current_accuracy = ACCURACY_LEVEL_NORMAL;
    context->convergence_tolerance = DEFAULT_CONVERGENCE_TOLERANCE;
    context->max_iterations = DEFAULT_MAX_ITERATIONS;
    context->dynamic_refinement = true;
    context->refinement_threshold = ADAPTIVITY_THRESHOLD;
    
    memset(&context->problem, 0, sizeof(ProblemCharacteristics));
    memset(&context->recommendation, 0, sizeof(AlgorithmRecommendation));
    memset(&context->metrics, 0, sizeof(PerformanceMetrics));
    
    return context;
}

void adaptive_calculation_destroy_context(AdaptiveCalculationContext* context) {
    if (context) {
        safe_free(context);
    }
}

int adaptive_calculation_analyze_problem(
    AdaptiveCalculationContext* context,
    const PCBICStructure* structure,
    double frequency_min,
    double frequency_max,
    AccuracyLevel target_accuracy
) {
    if (!context || !structure) return -1;
    
    context->problem.frequency_min = frequency_min;
    context->problem.frequency_max = frequency_max;
    context->problem.structure_type = structure->type;
    context->problem.material_type = structure->material.type;
    context->problem.boundary_condition = structure->boundary_condition;
    
    // Analyze structure complexity
    int total_elements = 0;
    double min_size = 1e9, max_size = 0.0;
    
    for (int i = 0; i < structure->num_geometries; i++) {
        const Geometry* geom = &structure->geometries[i];
        total_elements += geom->num_mesh_elements;
        
        double size = 0.0;
        switch (geom->type) {
            case GEOMETRY_TYPE_LINE:
                size = sqrt(geom->params.line.width * geom->params.line.width);
                break;
            case GEOMETRY_TYPE_RECTANGLE:
                size = sqrt(geom->params.rectangle.width * geom->params.rectangle.height);
                break;
            case GEOMETRY_TYPE_CIRCLE:
                size = geom->params.circle.radius;
                break;
            case GEOMETRY_TYPE_POLYGON:
                size = sqrt(geom->params.polygon.area);
                break;
            default:
                size = 1e-3;
        }
        
        if (size < min_size) min_size = size;
        if (size > max_size) max_size = size;
    }
    
    context->problem.mesh_elements = total_elements;
    context->problem.structure_size_min = min_size;
    context->problem.structure_size_max = max_size;
    
    // Estimate unknowns based on mesh and algorithm
    int unknowns_per_element = 1;
    switch (target_accuracy) {
        case ACCURACY_LEVEL_COARSE:
            unknowns_per_element = 1;
            break;
        case ACCURACY_LEVEL_NORMAL:
            unknowns_per_element = 3;
            break;
        case ACCURACY_LEVEL_FINE:
            unknowns_per_element = 6;
            break;
        case ACCURACY_LEVEL_VERY_FINE:
            unknowns_per_element = 12;
            break;
        case ACCURACY_LEVEL_ULTRA_FINE:
            unknowns_per_element = 24;
            break;
    }
    
    context->problem.unknowns = total_elements * unknowns_per_element;
    context->current_accuracy = target_accuracy;
    
    // Calculate complexity score (0-100)
    double wavelength_min = 3e8 / frequency_max;
    double electrical_size = max_size / wavelength_min;
    double mesh_density = (double)total_elements / (max_size * max_size);
    
    context->problem.complexity_score = (int)(
        20.0 * electrical_size + 
        30.0 * log10(mesh_density + 1) + 
        20.0 * (target_accuracy / 4.0) +
        30.0 * ((double)total_elements / 10000.0)
    );
    
    if (context->problem.complexity_score > 100) {
        context->problem.complexity_score = 100;
    }
    
    return 0;
}

AlgorithmRecommendation adaptive_calculation_select_algorithm(
    const AdaptiveCalculationContext* context,
    const ProblemCharacteristics* problem
) {
    AlgorithmRecommendation recommendation;
    memset(&recommendation, 0, sizeof(AlgorithmRecommendation));
    
    // Determine frequency regime
    double wavelength_min = 3e8 / problem->frequency_max;
    double electrical_size = problem->structure_size_max / wavelength_min;
    
    if (electrical_size < 0.01) {
        // Quasi-static regime
        recommendation.algorithm = ALGORITHM_ASYMPTOTIC_QUASI_STATIC;
    } else if (electrical_size > 10.0) {
        // High-frequency regime
        recommendation.algorithm = ALGORITHM_ASYMPTOTIC_HIGH_FREQUENCY;
    } else {
        // Full-wave regime - select based on complexity and accuracy requirements
        if (problem->complexity_score < 30) {
            if (context->current_accuracy <= ACCURACY_LEVEL_NORMAL) {
                recommendation.algorithm = ALGORITHM_MOM_BASIC;
            } else {
                recommendation.algorithm = ALGORITHM_MOM_FAST_MULTIPOLE;
            }
        } else if (problem->complexity_score < 60) {
            if (problem->mesh_elements < 10000) {
                recommendation.algorithm = ALGORITHM_MOM_FAST_MULTIPOLE;
            } else {
                recommendation.algorithm = ALGORITHM_MOM_ADAPTIVE_CROSS_APPROXIMATION;
            }
        } else {
            // High complexity - use advanced algorithms
            if (problem->structure_type == STRUCTURE_TYPE_TRANSMISSION_LINE ||
                problem->structure_type == STRUCTURE_TYPE_WAVEGUIDE) {
                recommendation.algorithm = ALGORITHM_FEM_HIGHER_ORDER;
            } else if (problem->structure_type == STRUCTURE_TYPE_ANTENNA) {
                recommendation.algorithm = ALGORITHM_FDTD_CONFORMAL;
            } else {
                recommendation.algorithm = ALGORITHM_HYBRID_MOM_FEM;
            }
        }
    }
    
    // Set optimization level based on problem characteristics
    if (problem->complexity_score < 20) {
        recommendation.optimization_level = OPTIMIZATION_LEVEL_BASIC;
    } else if (problem->complexity_score < 50) {
        recommendation.optimization_level = OPTIMIZATION_LEVEL_INTERMEDIATE;
    } else if (problem->complexity_score < 80) {
        recommendation.optimization_level = OPTIMIZATION_LEVEL_ADVANCED;
    } else {
        recommendation.optimization_level = OPTIMIZATION_LEVEL_AGGRESSIVE;
    }
    
    // Estimate performance metrics
    int accuracy_index = context->current_accuracy;
    recommendation.expected_accuracy = ALGORITHM_ACCURACY_TABLE[recommendation.algorithm][accuracy_index];
    recommendation.expected_time = ALGORITHM_SPEED_TABLE[recommendation.algorithm][accuracy_index];
    recommendation.memory_usage_gb = adaptive_calculation_estimate_memory_usage(recommendation.algorithm, problem);
    
    // Determine parallel capabilities
    recommendation.recommended_threads = 1;
    recommendation.supports_gpu = false;
    recommendation.supports_mpi = false;
    
    switch (recommendation.algorithm) {
        case ALGORITHM_MOM_FAST_MULTIPOLE:
        case ALGORITHM_MOM_ADAPTIVE_CROSS_APPROXIMATION:
        case ALGORITHM_FEM_HIGHER_ORDER:
        case ALGORITHM_FDTD_CONFORMAL:
        case ALGORITHM_PEEC_WAVELET:
        case ALGORITHM_HYBRID_MOM_FEM:
            recommendation.recommended_threads = 4;
            recommendation.supports_gpu = true;
            recommendation.supports_mpi = true;
            recommendation.parallel_efficiency = 0.8;
            break;
        case ALGORITHM_FEM_STANDARD:
        case ALGORITHM_FDTD_STANDARD:
        case ALGORITHM_FDTD_SUBGRID:
        case ALGORITHM_PEEC_BASIC:
        case ALGORITHM_PEEC_ENHANCED:
            recommendation.recommended_threads = 2;
            recommendation.supports_gpu = false;
            recommendation.supports_mpi = true;
            recommendation.parallel_efficiency = 0.7;
            break;
        default:
            recommendation.parallel_efficiency = 1.0;
            break;
    }
    
    return recommendation;
}

int adaptive_calculation_optimize_parameters(
    AdaptiveCalculationContext* context,
    const AlgorithmRecommendation* recommendation
) {
    if (!context || !recommendation) return -1;
    
    context->recommendation = *recommendation;
    context->current_optimization = recommendation->optimization_level;
    
    // Adjust convergence tolerance based on accuracy level
    switch (context->current_accuracy) {
        case ACCURACY_LEVEL_COARSE:
            context->convergence_tolerance = 1e-3;
            break;
        case ACCURACY_LEVEL_NORMAL:
            context->convergence_tolerance = 1e-4;
            break;
        case ACCURACY_LEVEL_FINE:
            context->convergence_tolerance = 1e-5;
            break;
        case ACCURACY_LEVEL_VERY_FINE:
            context->convergence_tolerance = 1e-6;
            break;
        case ACCURACY_LEVEL_ULTRA_FINE:
            context->convergence_tolerance = 1e-7;
            break;
    }
    
    // Adjust maximum iterations based on complexity
    if (context->problem.complexity_score < 30) {
        context->max_iterations = 500;
    } else if (context->problem.complexity_score < 60) {
        context->max_iterations = 1000;
    } else if (context->problem.complexity_score < 80) {
        context->max_iterations = 2000;
    } else {
        context->max_iterations = 5000;
    }
    
    return 0;
}

double adaptive_calculation_estimate_memory_usage(
    AlgorithmType algorithm,
    const ProblemCharacteristics* problem
) {
    double base_memory_gb = 0.1;  // Base memory in GB
    double unknowns_factor = (double)problem->unknowns / 10000.0;
    
    double algorithm_factor = 1.0;
    switch (algorithm) {
        case ALGORITHM_MOM_BASIC:
            algorithm_factor = 8.0;  // O(N^2) matrix storage
            break;
        case ALGORITHM_MOM_FAST_MULTIPOLE:
            algorithm_factor = 2.0;  // O(N log N) compression
            break;
        case ALGORITHM_MOM_ADAPTIVE_CROSS_APPROXIMATION:
            algorithm_factor = 1.5;  // O(N log N) compression
            break;
        case ALGORITHM_FEM_STANDARD:
            algorithm_factor = 4.0;  // Sparse matrix storage
            break;
        case ALGORITHM_FEM_HIGHER_ORDER:
            algorithm_factor = 6.0;  // Higher order elements
            break;
        case ALGORITHM_FEM_DISCONTINUOUS_GALERKIN:
            algorithm_factor = 5.0;  // Discontinuous elements
            break;
        case ALGORITHM_FDTD_STANDARD:
            algorithm_factor = 3.0;  // Field storage
            break;
        case ALGORITHM_FDTD_SUBGRID:
            algorithm_factor = 4.0;  // Subgrid storage
            break;
        case ALGORITHM_FDTD_CONFORMAL:
            algorithm_factor = 3.5;  // Conformal storage
            break;
        case ALGORITHM_PEEC_BASIC:
            algorithm_factor = 6.0;  // Circuit matrix
            break;
        case ALGORITHM_PEEC_ENHANCED:
            algorithm_factor = 7.0;  // Enhanced storage
            break;
        case ALGORITHM_PEEC_WAVELET:
            algorithm_factor = 2.5;  // Wavelet compression
            break;
        case ALGORITHM_HYBRID_MOM_FEM:
            algorithm_factor = 5.0;  // Hybrid storage
            break;
        case ALGORITHM_HYBRID_MOM_FDTD:
            algorithm_factor = 4.5;  // Hybrid storage
            break;
        case ALGORITHM_HYBRID_FEM_FDTD:
            algorithm_factor = 4.0;  // Hybrid storage
            break;
        case ALGORITHM_ASYMPTOTIC_HIGH_FREQUENCY:
            algorithm_factor = 1.0;  // Ray-based
            break;
        case ALGORITHM_ASYMPTOTIC_LOW_FREQUENCY:
            algorithm_factor = 1.2;  // Circuit-based
            break;
        case ALGORITHM_ASYMPTOTIC_QUASI_STATIC:
            algorithm_factor = 1.0;  // Static-based
            break;
        default:
            algorithm_factor = 1.0;
    }
    
    return base_memory_gb + algorithm_factor * unknowns_factor;
}

double adaptive_calculation_estimate_computation_time(
    AlgorithmType algorithm,
    const ProblemCharacteristics* problem
) {
    double base_time_seconds = 1.0;
    double unknowns_factor = (double)problem->unknowns / 10000.0;
    
    double complexity_exponent = 1.0;
    switch (algorithm) {
        case ALGORITHM_MOM_BASIC:
            complexity_exponent = 2.0;  // O(N^2)
            break;
        case ALGORITHM_MOM_FAST_MULTIPOLE:
            complexity_exponent = 1.1;  // O(N log N)
            break;
        case ALGORITHM_MOM_ADAPTIVE_CROSS_APPROXIMATION:
            complexity_exponent = 1.2;  // O(N log N)
            break;
        case ALGORITHM_FEM_STANDARD:
            complexity_exponent = 1.5;  // O(N^1.5)
            break;
        case ALGORITHM_FEM_HIGHER_ORDER:
            complexity_exponent = 1.7;  // O(N^1.7)
            break;
        case ALGORITHM_FEM_DISCONTINUOUS_GALERKIN:
            complexity_exponent = 1.6;  // O(N^1.6)
            break;
        case ALGORITHM_FDTD_STANDARD:
            complexity_exponent = 1.0;  // O(N)
            break;
        case ALGORITHM_FDTD_SUBGRID:
            complexity_exponent = 1.2;  // O(N^1.2)
            break;
        case ALGORITHM_FDTD_CONFORMAL:
            complexity_exponent = 1.1;  // O(N^1.1)
            break;
        case ALGORITHM_PEEC_BASIC:
            complexity_exponent = 1.8;  // O(N^1.8)
            break;
        case ALGORITHM_PEEC_ENHANCED:
            complexity_exponent = 1.6;  // O(N^1.6)
            break;
        case ALGORITHM_PEEC_WAVELET:
            complexity_exponent = 1.3;  // O(N^1.3)
            break;
        case ALGORITHM_HYBRID_MOM_FEM:
            complexity_exponent = 1.4;  // O(N^1.4)
            break;
        case ALGORITHM_HYBRID_MOM_FDTD:
            complexity_exponent = 1.3;  // O(N^1.3)
            break;
        case ALGORITHM_HYBRID_FEM_FDTD:
            complexity_exponent = 1.2;  // O(N^1.2)
            break;
        case ALGORITHM_ASYMPTOTIC_HIGH_FREQUENCY:
            complexity_exponent = 0.8;  // O(N^0.8)
            break;
        case ALGORITHM_ASYMPTOTIC_LOW_FREQUENCY:
            complexity_exponent = 0.9;  // O(N^0.9)
            break;
        case ALGORITHM_ASYMPTOTIC_QUASI_STATIC:
            complexity_exponent = 0.7;  // O(N^0.7)
            break;
        default:
            complexity_exponent = 1.0;
    }
    
    return base_time_seconds * pow(unknowns_factor, complexity_exponent);
}

bool adaptive_calculation_is_algorithm_supported(
    AlgorithmType algorithm,
    const ProblemCharacteristics* problem
) {
    if (!problem) return false;
    
    double wavelength_min = 3e8 / problem->frequency_max;
    double electrical_size = problem->structure_size_max / wavelength_min;
    
    switch (algorithm) {
        case ALGORITHM_ASYMPTOTIC_QUASI_STATIC:
            return electrical_size < 0.01;
        case ALGORITHM_ASYMPTOTIC_LOW_FREQUENCY:
            return electrical_size < 0.1;
        case ALGORITHM_ASYMPTOTIC_HIGH_FREQUENCY:
            return electrical_size > 10.0;
        case ALGORITHM_MOM_BASIC:
        case ALGORITHM_MOM_FAST_MULTIPOLE:
        case ALGORITHM_MOM_ADAPTIVE_CROSS_APPROXIMATION:
        case ALGORITHM_FEM_STANDARD:
        case ALGORITHM_FEM_HIGHER_ORDER:
        case ALGORITHM_FEM_DISCONTINUOUS_GALERKIN:
        case ALGORITHM_FDTD_STANDARD:
        case ALGORITHM_FDTD_SUBGRID:
        case ALGORITHM_FDTD_CONFORMAL:
        case ALGORITHM_PEEC_BASIC:
        case ALGORITHM_PEEC_ENHANCED:
        case ALGORITHM_PEEC_WAVELET:
        case ALGORITHM_HYBRID_MOM_FEM:
        case ALGORITHM_HYBRID_MOM_FDTD:
        case ALGORITHM_HYBRID_FEM_FDTD:
            return electrical_size >= 0.01 && electrical_size <= 10.0;
        default:
            return true;
    }
}

int adaptive_calculation_execute(
    AdaptiveCalculationContext* context,
    PCBICStructure* structure,
    double frequency,
    Complex* result
) {
    if (!context || !structure || !result) return -1;
    
    // Update problem characteristics for current frequency
    context->problem.frequency_min = frequency;
    context->problem.frequency_max = frequency;
    
    // Select appropriate algorithm
    context->recommendation = adaptive_calculation_select_algorithm(context, &context->problem);
    
    // Optimize parameters
    adaptive_calculation_optimize_parameters(context, &context->recommendation);
    
    // Execute calculation based on selected algorithm
    int status = 0;
    switch (context->recommendation.algorithm) {
        case ALGORITHM_MOM_BASIC:
        case ALGORITHM_MOM_FAST_MULTIPOLE:
        case ALGORITHM_MOM_ADAPTIVE_CROSS_APPROXIMATION:
            // Call MOM solver
            status = mom_solver_solve_adaptive(structure, frequency, result, context);
            break;
        case ALGORITHM_FEM_STANDARD:
        case ALGORITHM_FEM_HIGHER_ORDER:
        case ALGORITHM_FEM_DISCONTINUOUS_GALERKIN:
            // Call FEM solver
            status = fem_solver_solve_adaptive(structure, frequency, result, context);
            break;
        case ALGORITHM_FDTD_STANDARD:
        case ALGORITHM_FDTD_SUBGRID:
        case ALGORITHM_FDTD_CONFORMAL:
            // Call FDTD solver
            status = fdtd_solver_solve_adaptive(structure, frequency, result, context);
            break;
        case ALGORITHM_PEEC_BASIC:
        case ALGORITHM_PEEC_ENHANCED:
        case ALGORITHM_PEEC_WAVELET:
            // Call PEEC solver
            status = peec_solver_solve_adaptive(structure, frequency, result, context);
            break;
        default:
            // Use asymptotic or hybrid methods
            status = -1;
            break;
    }
    
    return status;
}

int adaptive_calculation_monitor_convergence(
    AdaptiveCalculationContext* context,
    const PerformanceMetrics* current_metrics
) {
    if (!context || !current_metrics) return -1;
    
    context->metrics = *current_metrics;
    
    // Check convergence criteria
    if (current_metrics->final_residual < context->convergence_tolerance) {
        return 1;  // Converged
    }
    
    if (current_metrics->iterations >= context->max_iterations) {
        return -1;  // Maximum iterations reached
    }
    
    // Check if accuracy target is met
    if (current_metrics->accuracy_achieved >= context->recommendation.expected_accuracy) {
        return 1;  // Accuracy target achieved
    }
    
    return 0;  // Continue iteration
}

int adaptive_calculation_estimate_error(
    const AdaptiveCalculationContext* context,
    const Complex* solution,
    double* error_estimate
) {
    if (!context || !solution || !error_estimate) return -1;
    
    // Simple error estimation based on residual and convergence history
    double residual_error = context->metrics.final_residual;
    double iteration_error = 1.0 / (double)context->metrics.iterations;
    
    *error_estimate = sqrt(residual_error * residual_error + iteration_error * iteration_error);
    
    return 0;
}

void adaptive_calculation_print_statistics(
    const AdaptiveCalculationContext* context,
    FILE* output
) {
    if (!context || !output) return;
    
    fprintf(output, "=== Adaptive Calculation Statistics ===\n");
    fprintf(output, "Problem Characteristics:\n");
    fprintf(output, "  Frequency Range: %.2e - %.2e Hz\n", 
            context->problem.frequency_min, context->problem.frequency_max);
    fprintf(output, "  Structure Size: %.2e - %.2e m\n", 
            context->problem.structure_size_min, context->problem.structure_size_max);
    fprintf(output, "  Mesh Elements: %d\n", context->problem.mesh_elements);
    fprintf(output, "  Unknowns: %d\n", context->problem.unknowns);
    fprintf(output, "  Complexity Score: %d/100\n", context->problem.complexity_score);
    fprintf(output, "  Structure Type: %d\n", context->problem.structure_type);
    
    fprintf(output, "\nAlgorithm Recommendation:\n");
    fprintf(output, "  Algorithm: %d\n", context->recommendation.algorithm);
    fprintf(output, "  Expected Accuracy: %.3f\n", context->recommendation.expected_accuracy);
    fprintf(output, "  Expected Time: %.2f seconds\n", context->recommendation.expected_time);
    fprintf(output, "  Memory Usage: %.2f GB\n", context->recommendation.memory_usage_gb);
    fprintf(output, "  Recommended Threads: %d\n", context->recommendation.recommended_threads);
    fprintf(output, "  GPU Support: %s\n", context->recommendation.supports_gpu ? "Yes" : "No");
    fprintf(output, "  MPI Support: %s\n", context->recommendation.supports_mpi ? "Yes" : "No");
    
    fprintf(output, "\nPerformance Metrics:\n");
    fprintf(output, "  Total Time: %.2f seconds\n", context->metrics.total_time_seconds);
    fprintf(output, "  Setup Time: %.2f seconds\n", context->metrics.setup_time_seconds);
    fprintf(output, "  Solve Time: %.2f seconds\n", context->metrics.solve_time_seconds);
    fprintf(output, "  Peak Memory: %.2f GB\n", context->metrics.memory_peak_gb);
    fprintf(output, "  Average Memory: %.2f GB\n", context->metrics.memory_average_gb);
    fprintf(output, "  Iterations: %d\n", context->metrics.iterations);
    fprintf(output, "  Final Residual: %.2e\n", context->metrics.final_residual);
    fprintf(output, "  Accuracy Achieved: %.3f\n", context->metrics.accuracy_achieved);
    fprintf(output, "  Threads Used: %d\n", context->metrics.threads_used);
    fprintf(output, "  GPU Accelerated: %s\n", context->metrics.gpu_accelerated ? "Yes" : "No");
    fprintf(output, "  MPI Parallel: %s\n", context->metrics.mpi_parallel ? "Yes" : "No");
    
    fprintf(output, "\nAdaptive Settings:\n");
    fprintf(output, "  Adaptive Enabled: %s\n", context->adaptive_enabled ? "Yes" : "No");
    fprintf(output, "  Current Optimization: %d\n", context->current_optimization);
    fprintf(output, "  Current Accuracy: %d\n", context->current_accuracy);
    fprintf(output, "  Convergence Tolerance: %.2e\n", context->convergence_tolerance);
    fprintf(output, "  Max Iterations: %d\n", context->max_iterations);
    fprintf(output, "  Dynamic Refinement: %s\n", context->dynamic_refinement ? "Yes" : "No");
    fprintf(output, "  Refinement Threshold: %.2e\n", context->refinement_threshold);
    
    fprintf(output, "========================================\n");
}

void adaptive_calculation_reset_context(AdaptiveCalculationContext* context) {
    if (!context) return;
    
    memset(&context->problem, 0, sizeof(ProblemCharacteristics));
    memset(&context->recommendation, 0, sizeof(AlgorithmRecommendation));
    memset(&context->metrics, 0, sizeof(PerformanceMetrics));
    
    context->convergence_tolerance = DEFAULT_CONVERGENCE_TOLERANCE;
    context->max_iterations = DEFAULT_MAX_ITERATIONS;
    context->refinement_threshold = ADAPTIVITY_THRESHOLD;
}

int adaptive_calculation_set_parameters(
    AdaptiveCalculationContext* context,
    const AdaptiveParameters* parameters
) {
    if (!context || !parameters) return -1;
    
    // Validate parameters
    if (parameters->min_refinement_level < 0 || 
        parameters->max_refinement_level > MAX_REFINEMENT_LEVELS ||
        parameters->min_refinement_level > parameters->max_refinement_level) {
        return -1;
    }
    
    if (parameters->refinement_ratio <= 0.0 || parameters->refinement_ratio >= 1.0) {
        return -1;
    }
    
    if (parameters->coarsening_ratio <= 0.0 || parameters->coarsening_ratio >= 1.0) {
        return -1;
    }
    
    // Apply parameters to context
    context->refinement_threshold = parameters->accuracy_adaptivity_threshold;
    context->dynamic_refinement = parameters->enable_h_adaptivity || 
                                  parameters->enable_p_adaptivity || 
                                  parameters->enable_hp_adaptivity;
    
    return 0;
}