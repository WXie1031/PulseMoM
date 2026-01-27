/******************************************************************************
 * Adaptive Calculation Process Optimization Implementation
 * 
 * Implementation of dynamic optimization of electromagnetic simulation
 * parameters based on problem characteristics, accuracy requirements,
 * and computational resources
 ******************************************************************************/

#include "adaptive_optimization.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/******************************************************************************
 * Default Configuration Parameters
 ******************************************************************************/
static const AdaptiveParameters default_parameters = {
    /* Mesh parameters */
    .initial_mesh_size = 1.0e-3,
    .minimum_mesh_size = 1.0e-6,
    .maximum_mesh_size = 1.0e-1,
    .mesh_refinement_ratio = 0.5,
    .max_mesh_levels = 10,
    
    /* Solver parameters */
    .initial_tolerance = 1.0e-6,
    .minimum_tolerance = 1.0e-12,
    .maximum_tolerance = 1.0e-2,
    .tolerance_reduction_factor = 0.1,
    
    /* Preconditioner parameters */
    .preconditioner_update_frequency = 10,
    .preconditioner_drop_tolerance = 1.0e-4,
    .preconditioner_fill_level = 2,
    
    /* Parallel parameters */
    .initial_num_threads = 1,
    .max_num_threads = 16,
    .load_balancing_frequency = 100,
    
    /* Algorithm parameters */
    .primary_algorithm = ALGORITHM_MOMENT_METHOD,
    .fallback_algorithm = ALGORITHM_FINITE_ELEMENT,
    .algorithm_switch_threshold = 5
};

static const ConvergenceCriteria default_convergence_criteria = {
    /* Residual-based criteria */
    .relative_residual_tolerance = 1.0e-6,
    .absolute_residual_tolerance = 1.0e-12,
    .initial_residual = 1.0,
    .current_residual = 1.0,
    
    /* Solution change criteria */
    .relative_solution_tolerance = 1.0e-5,
    .absolute_solution_tolerance = 1.0e-11,
    .max_solution_change = 1.0e-3,
    
    /* Eigenvalue convergence */
    .eigenvalue_tolerance = 1.0e-8,
    .eigenvector_tolerance = 1.0e-6,
    .num_converged_eigenvalues = 0,
    
    /* Energy-based criteria */
    .energy_error_tolerance = 1.0e-4,
    .energy_change_tolerance = 1.0e-6,
    .current_energy = 0.0,
    .previous_energy = 0.0,
    
    /* Iteration limits */
    .max_iterations = 1000,
    .max_refinement_levels = 5,
    .current_iteration = 0,
    .current_refinement_level = 0,
    
    /* Time-based criteria */
    .max_wall_time = 3600.0, /* 1 hour */
    .current_wall_time = 0.0,
    .start_time = 0.0
};

/******************************************************************************
 * Controller Management Functions
 ******************************************************************************/

AdaptiveController* create_adaptive_controller(StructureCategory category, uint32_t type,
                                             double frequency, OptimizationObjective objective) {
    AdaptiveController* controller = (AdaptiveController*)calloc(1, sizeof(AdaptiveController));
    if (!controller) {
        return NULL;
    }
    
    /* Initialize basic parameters */
    controller->structure_category = category;
    controller->structure_type = type;
    controller->frequency = frequency;
    controller->objective = objective;
    controller->current_strategy = ADAPTIVE_STRATEGY_NONE;
    
    /* Copy default parameters */
    controller->parameters = default_parameters;
    controller->convergence_criteria = default_convergence_criteria;
    
    /* Initialize history arrays */
    controller->max_history_size = 100;
    controller->error_history = (double*)calloc(controller->max_history_size, sizeof(double));
    controller->time_history = (double*)calloc(controller->max_history_size, sizeof(double));
    controller->memory_history = (double*)calloc(controller->max_history_size, sizeof(double));
    
    if (!controller->error_history || !controller->time_history || !controller->memory_history) {
        destroy_adaptive_controller(controller);
        return NULL;
    }
    
    /* Initialize convergence criteria */
    controller->convergence_criteria.start_time = (double)clock() / CLOCKS_PER_SEC;
    
    /* Estimate problem complexity */
    controller->complexity_estimate = estimate_computation_complexity(category, type, frequency, 0.01);
    
    return controller;
}

void destroy_adaptive_controller(AdaptiveController* controller) {
    if (!controller) {
        return;
    }
    
    if (controller->error_history) {
        free(controller->error_history);
    }
    if (controller->time_history) {
        free(controller->time_history);
    }
    if (controller->memory_history) {
        free(controller->memory_history);
    }
    
    free(controller);
}

void reset_adaptive_controller(AdaptiveController* controller) {
    if (!controller) {
        return;
    }
    
    /* Reset convergence criteria */
    controller->convergence_criteria = default_convergence_criteria;
    controller->convergence_criteria.start_time = (double)clock() / CLOCKS_PER_SEC;
    
    /* Reset metrics */
    memset(&controller->metrics, 0, sizeof(PerformanceMetrics));
    
    /* Reset history */
    controller->history_size = 0;
    controller->adaptation_count = 0;
    controller->current_strategy = ADAPTIVE_STRATEGY_NONE;
}

/******************************************************************************
 * Parameter Adaptation Functions
 ******************************************************************************/

bool adapt_mesh_parameters(AdaptiveController* controller, double current_error, double target_error) {
    if (!controller) {
        return false;
    }
    
    double current_mesh_size = controller->parameters.initial_mesh_size;
    double new_mesh_size = current_mesh_size;
    
    /* Calculate adaptation factor based on error ratio */
    double error_ratio = current_error / target_error;
    
    if (error_ratio > 2.0) {
        /* Error is too high - refine mesh */
        new_mesh_size = current_mesh_size * controller->parameters.mesh_refinement_ratio;
        
        /* Check minimum mesh size constraint */
        if (new_mesh_size < controller->parameters.minimum_mesh_size) {
            new_mesh_size = controller->parameters.minimum_mesh_size;
        }
        
        controller->current_strategy = ADAPTIVE_STRATEGY_MESH_REFINEMENT;
        
    } else if (error_ratio < 0.5) {
        /* Error is below target - can coarsen mesh */
        new_mesh_size = current_mesh_size / controller->parameters.mesh_refinement_ratio;
        
        /* Check maximum mesh size constraint */
        if (new_mesh_size > controller->parameters.maximum_mesh_size) {
            new_mesh_size = controller->parameters.maximum_mesh_size;
        }
        
        controller->current_strategy = ADAPTIVE_STRATEGY_MESH_REFINEMENT;
    }
    
    /* Update mesh size */
    controller->parameters.initial_mesh_size = new_mesh_size;
    
    /* Record in history */
    if (controller->history_size < controller->max_history_size) {
        controller->error_history[controller->history_size] = current_error;
        controller->time_history[controller->history_size] = controller->metrics.total_time;
        controller->memory_history[controller->history_size] = controller->metrics.peak_memory_usage;
        controller->history_size++;
    }
    
    controller->adaptation_count++;
    return true;
}

bool adapt_solver_parameters(AdaptiveController* controller, double current_residual, double target_residual) {
    if (!controller) {
        return false;
    }
    
    double current_tolerance = controller->parameters.initial_tolerance;
    double new_tolerance = current_tolerance;
    
    /* Calculate adaptation factor based on residual ratio */
    double residual_ratio = current_residual / target_residual;
    
    if (residual_ratio > 10.0) {
        /* Residual is too high - tighten tolerance */
        new_tolerance = current_tolerance * controller->parameters.tolerance_reduction_factor;
        
        /* Check minimum tolerance constraint */
        if (new_tolerance < controller->parameters.minimum_tolerance) {
            new_tolerance = controller->parameters.minimum_tolerance;
        }
        
    } else if (residual_ratio < 0.1) {
        /* Residual is below target - can relax tolerance */
        new_tolerance = current_tolerance / controller->parameters.tolerance_reduction_factor;
        
        /* Check maximum tolerance constraint */
        if (new_tolerance > controller->parameters.maximum_tolerance) {
            new_tolerance = controller->parameters.maximum_tolerance;
        }
    }
    
    /* Update tolerance */
    controller->parameters.initial_tolerance = new_tolerance;
    controller->convergence_criteria.relative_residual_tolerance = new_tolerance;
    
    controller->current_strategy = ADAPTIZATION_STRATEGY_SOLVER_SWITCH;
    controller->adaptation_count++;
    
    return true;
}

bool adapt_preconditioner(AdaptiveController* controller, uint32_t iteration_count, double convergence_rate) {
    if (!controller) {
        return false;
    }
    
    /* Analyze convergence rate */
    double target_convergence_rate = 0.1; /* Target 10% reduction per iteration */
    
    if (convergence_rate < target_convergence_rate * 0.5) {
        /* Slow convergence - improve preconditioner */
        controller->parameters.preconditioner_fill_level++;
        controller->parameters.preconditioner_drop_tolerance *= 0.5;
        
        /* Limit maximum fill level */
        if (controller->parameters.preconditioner_fill_level > 5) {
            controller->parameters.preconditioner_fill_level = 5;
        }
        
        /* Limit minimum drop tolerance */
        if (controller->parameters.preconditioner_drop_tolerance < 1.0e-8) {
            controller->parameters.preconditioner_drop_tolerance = 1.0e-8;
        }
        
    } else if (convergence_rate > target_convergence_rate * 2.0) {
        /* Fast convergence - can use simpler preconditioner */
        controller->parameters.preconditioner_fill_level--;
        controller->parameters.preconditioner_drop_tolerance *= 2.0;
        
        /* Limit minimum fill level */
        if (controller->parameters.preconditioner_fill_level < 0) {
            controller->parameters.preconditioner_fill_level = 0;
        }
        
        /* Limit maximum drop tolerance */
        if (controller->parameters.preconditioner_drop_tolerance > 1.0e-2) {
            controller->parameters.preconditioner_drop_tolerance = 1.0e-2;
        }
    }
    
    controller->current_strategy = ADAPTIVE_STRATEGY_PRECONDITIONER_UPDATE;
    controller->adaptation_count++;
    
    return true;
}

bool adapt_parallelization(AdaptiveController* controller, double load_imbalance, double parallel_efficiency) {
    if (!controller) {
        return false;
    }
    
    double target_efficiency = 0.7; /* 70% parallel efficiency */
    double max_imbalance = 0.3;     /* 30% load imbalance */
    
    if (parallel_efficiency < target_efficiency * 0.8 || load_imbalance > max_imbalance) {
        /* Poor parallel performance - adjust parameters */
        if (controller->parameters.initial_num_threads < controller->parameters.max_num_threads) {
            controller->parameters.initial_num_threads++;
        }
        
        /* Increase load balancing frequency */
        controller->parameters.load_balancing_frequency = 
            (controller->parameters.load_balancing_frequency * 3) / 4;
        
        if (controller->parameters.load_balancing_frequency < 10) {
            controller->parameters.load_balancing_frequency = 10;
        }
        
    } else if (parallel_efficiency > target_efficiency * 1.2 && load_imbalance < max_imbalance * 0.5) {
        /* Good parallel performance - can try more aggressive settings */
        if (controller->parameters.initial_num_threads < controller->parameters.max_num_threads) {
            controller->parameters.initial_num_threads++;
        }
    }
    
    controller->current_strategy = ADAPTIVE_STRATEGY_PARALLELIZATION;
    controller->adaptation_count++;
    
    return true;
}

bool adapt_algorithm_selection(AdaptiveController* controller, double current_performance, double target_performance) {
    if (!controller) {
        return false;
    }
    
    /* Check if we need to switch algorithms */
    if (current_performance < target_performance * 0.5) {
        /* Poor performance - try fallback algorithm */
        AlgorithmType temp = controller->parameters.primary_algorithm;
        controller->parameters.primary_algorithm = controller->parameters.fallback_algorithm;
        controller->parameters.fallback_algorithm = temp;
        
        controller->current_strategy = ADAPTIZATION_STRATEGY_SOLVER_SWITCH;
        controller->adaptation_count++;
        
        return true;
    }
    
    return false;
}

/******************************************************************************
 * Convergence Monitoring Functions
 ******************************************************************************/

bool check_convergence(AdaptiveController* controller) {
    if (!controller) {
        return false;
    }
    
    const ConvergenceCriteria* criteria = &controller->convergence_criteria;
    
    /* Check residual convergence */
    if (criteria->current_residual < criteria->relative_residual_tolerance * criteria->initial_residual ||
        criteria->current_residual < criteria->absolute_residual_tolerance) {
        controller->metrics.converged = true;
        return true;
    }
    
    /* Check solution change convergence */
    if (criteria->max_solution_change < criteria->relative_solution_tolerance ||
        criteria->max_solution_change < criteria->absolute_solution_tolerance) {
        controller->metrics.converged = true;
        return true;
    }
    
    /* Check energy convergence */
    if (criteria->current_energy > 0.0 && criteria->previous_energy > 0.0) {
        double energy_change = fabs(criteria->current_energy - criteria->previous_energy) / criteria->previous_energy;
        if (energy_change < criteria->energy_change_tolerance) {
            controller->metrics.converged = true;
            return true;
        }
    }
    
    /* Check iteration limit */
    if (criteria->current_iteration >= criteria->max_iterations) {
        controller->metrics.converged = false;
        return true; /* Forced stop */
    }
    
    /* Check time limit */
    double current_time = (double)clock() / CLOCKS_PER_SEC;
    if (current_time - criteria->start_time > criteria->max_wall_time) {
        controller->metrics.converged = false;
        return true; /* Timeout */
    }
    
    return false;
}

double estimate_convergence_rate(AdaptiveController* controller) {
    if (!controller || controller->history_size < 2) {
        return 0.0;
    }
    
    /* Simple linear regression on error history */
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    uint32_t n = controller->history_size;
    
    for (uint32_t i = 0; i < n; i++) {
        double x = (double)i;
        double y = log(controller->error_history[i]);
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    return exp(slope); /* Convert back from log scale */
}

double predict_solution_time(AdaptiveController* controller, uint32_t remaining_iterations) {
    if (!controller || controller->history_size < 2) {
        return 1.0; /* Default estimate */
    }
    
    /* Simple linear prediction based on time history */
    double average_time_per_iteration = controller->metrics.iteration_time;
    if (average_time_per_iteration <= 0.0) {
        average_time_per_iteration = controller->metrics.total_time / controller->convergence_criteria.current_iteration;
    }
    
    return remaining_iterations * average_time_per_iteration;
}

/******************************************************************************
 * Performance Optimization Functions
 ******************************************************************************/

PerformanceMetrics optimize_performance(AdaptiveController* controller) {
    if (!controller) {
        PerformanceMetrics empty = {0};
        return empty;
    }
    
    PerformanceMetrics* metrics = &controller->metrics;
    
    /* Calculate efficiency metrics */
    if (metrics->total_time > 0.0) {
        metrics->memory_efficiency = metrics->average_memory_usage / metrics->peak_memory_usage;
        
        if (metrics->setup_time > 0.0) {
            double solve_efficiency = metrics->solve_time / metrics->total_time;
            double assembly_efficiency = metrics->assembly_time / metrics->total_time;
            
            /* Identify bottlenecks */
            if (solve_efficiency > 0.7) {
                /* Solver is the bottleneck */
                controller->current_strategy = ADAPTIZATION_STRATEGY_SOLVER_SWITCH;
            } else if (assembly_efficiency > 0.3) {
                /* Assembly is the bottleneck */
                controller->current_strategy = ADAPTIVE_STRATEGY_DOMAIN_DECOMPOSITION;
            }
        }
    }
    
    return *metrics;
}

AdaptiveParameters optimize_parameters(AdaptiveController* controller, const PerformanceMetrics* current_metrics) {
    if (!controller || !current_metrics) {
        return controller ? controller->parameters : default_parameters;
    }
    
    AdaptiveParameters new_params = controller->parameters;
    
    /* Optimize based on objective */
    switch (controller->objective) {
        case OPTIMIZATION_OBJECTIVE_SPEED:
            optimize_for_speed(&new_params, controller, current_metrics);
            break;
        case OPTIMIZATION_OBJECTIVE_ACCURACY:
            optimize_for_accuracy(&new_params, controller, current_metrics);
            break;
        case OPTIMIZATION_OBJECTIVE_MEMORY:
            optimize_for_memory(&new_params, controller, current_metrics);
            break;
        case OPTIMIZATION_OBJECTIVE_BALANCED:
            optimize_for_balance(&new_params, controller, current_metrics);
            break;
        default:
            break;
    }
    
    return new_params;
}

static void optimize_for_speed(AdaptiveParameters* params, AdaptiveController* controller, 
                              const PerformanceMetrics* metrics) {
    /* Coarsen mesh for speed */
    params->initial_mesh_size = params->maximum_mesh_size * 0.8;
    
    /* Relax solver tolerance */
    params->initial_tolerance = params->maximum_tolerance * 0.5;
    
    /* Use simpler preconditioner */
    params->preconditioner_fill_level = 0;
    params->preconditioner_drop_tolerance = 1.0e-3;
    
    /* Increase parallelization */
    if (params->initial_num_threads < params->max_num_threads) {
        params->initial_num_threads = params->max_num_threads;
    }
}

static void optimize_for_accuracy(AdaptiveParameters* params, AdaptiveController* controller,
                                const PerformanceMetrics* metrics) {
    /* Refine mesh for accuracy */
    params->initial_mesh_size = params->minimum_mesh_size * 2.0;
    
    /* Tighten solver tolerance */
    params->initial_tolerance = params->minimum_tolerance * 10.0;
    
    /* Use better preconditioner */
    params->preconditioner_fill_level = 3;
    params->preconditioner_drop_tolerance = 1.0e-5;
    
    /* Use more accurate algorithm */
    if (controller->parameters.primary_algorithm != ALGORITHM_MOMENT_METHOD) {
        controller->parameters.primary_algorithm = ALGORITHM_MOMENT_METHOD;
    }
}

static void optimize_for_memory(AdaptiveParameters* params, AdaptiveController* controller,
                               const PerformanceMetrics* metrics) {
    /* Use coarser mesh */
    params->initial_mesh_size = params->maximum_mesh_size * 0.5;
    
    /* Reduce preconditioner memory */
    params->preconditioner_fill_level = 1;
    params->preconditioner_drop_tolerance = 1.0e-3;
    
    /* Limit parallel threads */
    if (params->initial_num_threads > 4) {
        params->initial_num_threads = 4;
    }
}

static void optimize_for_balance(AdaptiveParameters* params, AdaptiveController* controller,
                                const PerformanceMetrics* metrics) {
    /* Balanced optimization - use current settings as baseline */
    /* This is a simplified implementation */
    double accuracy_factor = metrics->estimated_error > 0 ? 
                           1.0 / (1.0 + metrics->estimated_error) : 1.0;
    double speed_factor = metrics->total_time > 0 ? 
                         1.0 / (1.0 + log10(metrics->total_time)) : 1.0;
    double memory_factor = metrics->peak_memory_usage > 0 ? 
                          1.0 / (1.0 + log10(metrics->peak_memory_usage)) : 1.0;
    
    /* Adjust parameters based on weighted factors */
    if (accuracy_factor < 0.7) {
        params->initial_tolerance *= 0.5;
        params->initial_mesh_size *= 0.8;
    }
    
    if (speed_factor < 0.7) {
        params->initial_mesh_size *= 1.2;
        params->preconditioner_fill_level = 1;
    }
    
    if (memory_factor < 0.7) {
        params->initial_mesh_size *= 1.5;
        params->preconditioner_fill_level = 0;
    }
}

AdaptiveStrategy select_optimal_strategy(AdaptiveController* controller, const PerformanceMetrics* metrics) {
    if (!controller || !metrics) {
        return ADAPTIVE_STRATEGY_NONE;
    }
    
    /* Strategy selection based on current performance and bottlenecks */
    if (metrics->estimated_error > 1.0e-3) {
        return ADAPTIVE_STRATEGY_MESH_REFINEMENT;
    }
    
    if (metrics->iterations_required > 100) {
        return ADAPTIVE_STRATEGY_PRECONDITIONER_UPDATE;
    }
    
    if (metrics->parallel_efficiency < 0.5) {
        return ADAPTIVE_STRATEGY_PARALLELIZATION;
    }
    
    if (metrics->memory_efficiency < 0.5) {
        return ADAPTIVE_STRATEGY_DOMAIN_DECOMPOSITION;
    }
    
    return ADAPTIVE_STRATEGY_HYBRID;
}

/******************************************************************************
 * Error Estimation Functions
 ******************************************************************************/

double estimate_discretization_error(AdaptiveController* controller, const double* solution, uint32_t size) {
    if (!controller || !solution || size == 0) {
        return 1.0;
    }
    
    /* Simple Richardson extrapolation-based error estimate */
    /* This is a simplified implementation */
    double mesh_size = controller->parameters.initial_mesh_size;
    double frequency = controller->frequency;
    double wavelength = 3.0e8 / frequency;
    
    /* Estimate error based on mesh size relative to wavelength */
    double normalized_mesh_size = mesh_size / wavelength;
    double theoretical_error = normalized_mesh_size * normalized_mesh_size; /* O(h²) error */
    
    /* Adjust based on solution smoothness */
    double solution_variation = 0.0;
    if (size > 1) {
        for (uint32_t i = 1; i < size; i++) {
            double diff = fabs(solution[i] - solution[i-1]);
            solution_variation += diff;
        }
        solution_variation /= (size - 1);
    }
    
    return theoretical_error * (1.0 + solution_variation);
}

double estimate_modeling_error(AdaptiveController* controller, const double* solution, uint32_t size) {
    if (!controller) {
        return 0.0;
    }
    
    /* Estimate modeling error based on problem characteristics */
    double frequency = controller->frequency;
    double structure_size = controller->complexity_estimate;
    
    /* Higher frequencies and complex structures have larger modeling errors */
    double frequency_factor = frequency / 1.0e9; /* Relative to 1 GHz */
    double complexity_factor = log10(structure_size + 1.0);
    
    return 1.0e-3 * frequency_factor * complexity_factor; /* 0.1% base error */
}

double estimate_total_error(AdaptiveController* controller, const double* solution, uint32_t size) {
    if (!controller) {
        return 1.0;
    }
    
    double discretization_error = estimate_discretization_error(controller, solution, size);
    double modeling_error = estimate_modeling_error(controller, solution, size);
    
    /* Combine errors using root-sum-square */
    return sqrt(discretization_error * discretization_error + modeling_error * modeling_error);
}

bool control_error_propagation(AdaptiveController* controller, double estimated_error, double target_error) {
    if (!controller) {
        return false;
    }
    
    double error_ratio = estimated_error / target_error;
    
    if (error_ratio > 2.0) {
        /* Error is too high - need aggressive adaptation */
        adapt_mesh_parameters(controller, estimated_error, target_error);
        adapt_solver_parameters(controller, estimated_error, target_error);
        return true;
        
    } else if (error_ratio > 1.0) {
        /* Error is slightly above target - moderate adaptation */
        adapt_mesh_parameters(controller, estimated_error, target_error);
        return true;
        
    } else if (error_ratio < 0.5) {
        /* Error is well below target - can relax parameters */
        /* This would involve coarsening mesh or relaxing tolerance */
        return true;
    }
    
    return false;
}

/******************************************************************************
 * Resource Management Functions
 ******************************************************************************/

bool optimize_memory_usage(AdaptiveController* controller, double memory_limit) {
    if (!controller) {
        return false;
    }
    
    if (controller->metrics.peak_memory_usage > memory_limit) {
        /* Memory usage exceeds limit - optimize for memory */
        AdaptiveParameters new_params = controller->parameters;
        optimize_for_memory(&new_params, controller, &controller->metrics);
        controller->parameters = new_params;
        return true;
    }
    
    return false;
}

bool optimize_computation_time(AdaptiveController* controller, double time_limit) {
    if (!controller) {
        return false;
    }
    
    if (controller->metrics.total_time > time_limit) {
        /* Computation time exceeds limit - optimize for speed */
        AdaptiveParameters new_params = controller->parameters;
        optimize_for_speed(&new_params, controller, &controller->metrics);
        controller->parameters = new_params;
        return true;
    }
    
    return false;
}

bool balance_accuracy_speed_tradeoff(AdaptiveController* controller, double accuracy_weight, double speed_weight) {
    if (!controller) {
        return false;
    }
    
    /* Simple weighted optimization */
    double current_error = controller->metrics.estimated_error;
    double current_time = controller->metrics.total_time;
    
    /* Normalize weights */
    double total_weight = accuracy_weight + speed_weight;
    if (total_weight <= 0.0) {
        return false;
    }
    
    accuracy_weight /= total_weight;
    speed_weight /= total_weight;
    
    /* Calculate performance score */
    double error_score = 1.0 / (1.0 + current_error);
    double time_score = 1.0 / (1.0 + log10(current_time + 1.0));
    double performance_score = accuracy_weight * error_score + speed_weight * time_score;
    
    /* Adjust parameters based on performance score */
    if (performance_score < 0.6) {
        /* Poor performance - adjust parameters */
        AdaptiveParameters new_params = controller->parameters;
        
        if (accuracy_weight > speed_weight) {
            /* Favor accuracy */
            new_params.initial_tolerance *= 0.5;
            new_params.initial_mesh_size *= 0.8;
        } else {
            /* Favor speed */
            new_params.initial_mesh_size *= 1.2;
            new_params.preconditioner_fill_level = 1;
        }
        
        controller->parameters = new_params;
        return true;
    }
    
    return false;
}