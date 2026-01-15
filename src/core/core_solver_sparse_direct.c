/**
 * @file core_solver_sparse_direct.c
 * @brief Implementation of enhanced linear solver with sparse direct solver support
 * @details Unified interface integrating MUMPS/PARDISO with existing solver framework
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "core_solver_sparse_direct.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Internal enhanced solver structure
struct enhanced_linear_solver {
    enhanced_solver_config_t config;
    
    // Solver components
    sparse_direct_solver_t *sparse_solver;
    linear_solver_t *native_solver;
    
    // Matrix storage
    sparse_matrix_t sparse_matrix;
    double complex *dense_matrix;
    
    // Solver state
    bool is_factorized;
    bool use_sparse_solver;
    solver_type_t active_solver_type;
    
    // Performance metrics
    double factorization_time;
    double solve_time;
    size_t memory_usage;
    
    // Working arrays
    double complex *work_vector;
    size_t work_vector_size;
};

// Utility functions
static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static size_t get_memory_usage(void) {
    // Simple memory estimation - can be enhanced with actual memory tracking
    return 0;
}

// Solver selection logic
bool enhanced_linear_solver_should_use_sparse_direct(int matrix_size,
                                                    int64_t nnz,
                                                    int num_rhs,
                                                    size_t memory_available) {
    // Use sparse direct solver if:
    // 1. Matrix is large enough (sparse solver overhead is worth it)
    // 2. Matrix is sparse (nnz << matrix_size^2)
    // 3. Multiple RHS (sparse direct solvers are efficient for multiple RHS)
    // 4. Enough memory available
    
    const int MIN_SIZE_SPARSE = 1000;      // Minimum matrix size
    const double MAX_DENSITY = 0.1;        // Maximum density for sparse solver
    const size_t MIN_MEMORY_SPARSE = 1ULL * 1024 * 1024 * 1024;  // 1GB minimum
    
    if (matrix_size < MIN_SIZE_SPARSE) {
        return false;
    }
    
    if (nnz > 0) {
        double density = (double)nnz / (double)(matrix_size * matrix_size);
        if (density > MAX_DENSITY) {
            return false;
        }
    }
    
    if (num_rhs > 1) {
        // Multiple RHS favors sparse direct solvers
        return true;
    }
    
    if (memory_available < MIN_MEMORY_SPARSE) {
        return false;
    }
    
    return true;
}

int enhanced_linear_solver_estimate_memory(int matrix_size,
                                         int64_t nnz,
                                         solver_type_t solver_type,
                                         size_t *memory_needed) {
    if (!memory_needed) {
        return -1;
    }
    
    *memory_needed = 0;
    
    switch (solver_type) {
        case SOLVER_TYPE_DENSE_LU:
            // Dense matrix: n^2 * 16 bytes (complex) + factorization overhead
            *memory_needed = (size_t)matrix_size * matrix_size * 16 * 2;  // 2x for factors
            break;
            
        case SOLVER_TYPE_SPARSE_LU:
            if (nnz > 0) {
                // Sparse matrix: nnz * 16 bytes + fill-in overhead (typically 3-5x)
                *memory_needed = (size_t)nnz * 16 * 5;  // 5x for fill-in
            } else {
                // Estimate nnz as 1% of dense matrix
                int64_t estimated_nnz = (int64_t)matrix_size * matrix_size / 100;
                *memory_needed = (size_t)estimated_nnz * 16 * 5;
            }
            break;
            
        case SOLVER_TYPE_ITERATIVE_GMRES:
            // Iterative solver: matrix + work vectors (restart_size * n)
            *memory_needed = (size_t)matrix_size * matrix_size * 16 + (size_t)matrix_size * 30 * 16;
            break;
            
        default:
            return -1;
    }
    
    return 0;
}

int enhanced_linear_solver_auto_select(int matrix_size,
                                      int64_t nnz,
                                      int num_rhs,
                                      size_t memory_available,
                                      enhanced_solver_config_t *config) {
    if (!config) {
        return -1;
    }
    
    // Get default configuration
    enhanced_linear_solver_get_default_config(config);
    
    // Determine if sparse direct solver should be used
    if (enhanced_linear_solver_should_use_sparse_direct(matrix_size, nnz, num_rhs, memory_available)) {
        config->use_sparse_direct_for_large_problems = true;
        config->sparse_solver_type = SPARSE_SOLVER_MUMPS;  // Default to MUMPS
        
        // Estimate memory requirements
        size_t sparse_memory, dense_memory;
        enhanced_linear_solver_estimate_memory(matrix_size, nnz, SOLVER_TYPE_SPARSE_LU, &sparse_memory);
        enhanced_linear_solver_estimate_memory(matrix_size, nnz, SOLVER_TYPE_DENSE_LU, &dense_memory);
        
        // Choose solver based on memory constraints
        if (sparse_memory < memory_available * 0.8) {  // Use 80% of available memory
            config->sparse_solver_type = SPARSE_SOLVER_MUMPS;
        } else if (dense_memory < memory_available * 0.8) {
            config->sparse_solver_type = SPARSE_SOLVER_PARDISO;
        } else {
            // Fall back to iterative solver
            config->base_type = SOLVER_TYPE_ITERATIVE_GMRES;
        }
    } else {
        // Use dense or iterative solver
        if (matrix_size < 1000) {
            config->base_type = SOLVER_TYPE_DENSE_LU;
        } else {
            config->base_type = SOLVER_TYPE_ITERATIVE_GMRES;
        }
    }
    
    return 0;
}

// Main enhanced solver functions
enhanced_linear_solver_t* enhanced_linear_solver_init(const enhanced_solver_config_t *config) {
    if (!config) {
        return NULL;
    }
    
    enhanced_linear_solver_t *solver = (enhanced_linear_solver_t *)calloc(1, sizeof(enhanced_linear_solver_t));
    if (!solver) {
        return NULL;
    }
    
    // Copy configuration
    memcpy(&solver->config, config, sizeof(enhanced_solver_config_t));
    
    // Initialize sparse direct solver if configured
    if (config->use_sparse_direct_for_large_problems) {
        solver->sparse_solver = sparse_direct_solver_init(&config->sparse_config);
        if (!solver->sparse_solver) {
            free(solver);
            return NULL;
        }
    }
    
    // Initialize native solver as fallback
    linear_solver_config_t native_config = {
        .type = config->base_type,
        .backend = config->backend,
        .precision = config->precision,
        .tolerance = 1e-12,
        .max_iterations = 1000,
        .restart_size = 30,
        .use_preconditioner = true,
        .preconditioner_type = "ILU",
        .use_parallel = true,
        .num_threads = 4,
        .verbose = config->sparse_config.verbose
    };
    
    solver->native_solver = linear_solver_init(&native_config);
    if (!solver->native_solver && !solver->sparse_solver) {
        free(solver);
        return NULL;
    }
    
    solver->is_factorized = false;
    solver->use_sparse_solver = false;
    solver->active_solver_type = config->base_type;
    
    return solver;
}

int enhanced_linear_solver_solve(enhanced_linear_solver_t *solver,
                                  void *matrix,
                                  const double complex *rhs,
                                  double complex *solution,
                                  int matrix_size,
                                  int num_rhs) {
    if (!solver || !matrix || !rhs || !solution || matrix_size <= 0) {
        return -1;
    }
    
    double start_time = get_time();
    
    // Determine which solver to use
    bool use_sparse = false;
    
    if (solver->config.use_sparse_direct_for_large_problems && solver->sparse_solver) {
        // Check if matrix is sparse
        sparse_matrix_t *sparse_matrix = (sparse_matrix_t *)matrix;
        if (sparse_matrix && sparse_matrix->format == SPARSE_FORMAT_CSR) {
            use_sparse = enhanced_linear_solver_should_use_sparse_direct(
                matrix_size, sparse_matrix->nnz, num_rhs, 8ULL * 1024 * 1024 * 1024);
        }
    }
    
    int result = -1;
    
    if (use_sparse && solver->sparse_solver) {
        // Use sparse direct solver
        solver->use_sparse_solver = true;
        solver->active_solver_type = SOLVER_TYPE_SPARSE_LU;
        
        // Set matrix
        sparse_matrix_t *sparse_matrix = (sparse_matrix_t *)matrix;
        result = sparse_direct_solver_set_matrix(solver->sparse_solver, sparse_matrix);
        if (result != 0) {
            return result;
        }
        
        // Analyze if not already done
        if (!solver->is_factorized) {
            result = sparse_direct_solver_analyze(solver->sparse_solver);
            if (result != 0) {
                return result;
            }
            
            result = sparse_direct_solver_factorize(solver->sparse_solver);
            if (result != 0) {
                return result;
            }
            
            solver->is_factorized = true;
        }
        
        // Solve
        result = sparse_direct_solver_solve(solver->sparse_solver, rhs, solution, num_rhs);
        
        solver->solve_time = get_time() - start_time;
        
    } else if (solver->native_solver) {
        // Use native solver
        solver->use_sparse_solver = false;
        solver->active_solver_type = solver->config.base_type;
        
        // For now, convert to dense if needed and use native solver
        // This is a simplified implementation - full implementation would handle sparse matrices
        result = linear_solver_solve(solver->native_solver, matrix, rhs, solution, matrix_size);
        
        solver->solve_time = get_time() - start_time;
    }
    
    return result;
}

int enhanced_linear_solver_solve_multiple(enhanced_linear_solver_t *solver,
                                         void *matrix,
                                         const double complex *rhs,
                                         double complex *solution,
                                         int matrix_size,
                                         int num_rhs) {
    return enhanced_linear_solver_solve(solver, matrix, rhs, solution, matrix_size, num_rhs);
}

int enhanced_linear_solver_factorize(enhanced_linear_solver_t *solver,
                                    void *matrix,
                                    int matrix_size) {
    if (!solver || !matrix || matrix_size <= 0) {
        return -1;
    }
    
    double start_time = get_time();
    
    // Check if sparse solver should be used
    bool use_sparse = false;
    
    if (solver->config.use_sparse_direct_for_large_problems && solver->sparse_solver) {
        sparse_matrix_t *sparse_matrix = (sparse_matrix_t *)matrix;
        if (sparse_matrix && sparse_matrix->format == SPARSE_FORMAT_CSR) {
            use_sparse = enhanced_linear_solver_should_use_sparse_direct(
                matrix_size, sparse_matrix->nnz, 1, 8ULL * 1024 * 1024 * 1024);
        }
    }
    
    int result = -1;
    
    if (use_sparse && solver->sparse_solver) {
        // Factorize using sparse direct solver
        sparse_matrix_t *sparse_matrix = (sparse_matrix_t *)matrix;
        
        result = sparse_direct_solver_set_matrix(solver->sparse_solver, sparse_matrix);
        if (result != 0) {
            return result;
        }
        
        result = sparse_direct_solver_analyze(solver->sparse_solver);
        if (result != 0) {
            return result;
        }
        
        result = sparse_direct_solver_factorize(solver->sparse_solver);
        if (result != 0) {
            return result;
        }
        
        solver->is_factorized = true;
        solver->factorization_time = get_time() - start_time;
        
    } else if (solver->native_solver) {
        // Factorize using native solver
        result = linear_solver_factorize(solver->native_solver, matrix, matrix_size);
        solver->factorization_time = get_time() - start_time;
    }
    
    return result;
}

int enhanced_linear_solver_solve_factorized(enhanced_linear_solver_t *solver,
                                           const double complex *rhs,
                                           double complex *solution,
                                           int num_rhs) {
    if (!solver || !rhs || !solution || !solver->is_factorized) {
        return -1;
    }
    
    double start_time = get_time();
    
    int result = -1;
    
    if (solver->use_sparse_solver && solver->sparse_solver) {
        // Solve using pre-factorized sparse direct solver
        result = sparse_direct_solver_solve(solver->sparse_solver, rhs, solution, num_rhs);
        
    } else if (solver->native_solver) {
        // Solve using pre-factorized native solver
        result = linear_solver_solve_factorized(solver->native_solver, rhs, solution, num_rhs);
    }
    
    solver->solve_time = get_time() - start_time;
    
    return result;
}

int enhanced_linear_solver_refactorize(enhanced_linear_solver_t *solver,
                                      const double complex *new_values,
                                      int matrix_size) {
    if (!solver || !new_values || matrix_size <= 0 || !solver->is_factorized) {
        return -1;
    }
    
    double start_time = get_time();
    
    int result = -1;
    
    if (solver->use_sparse_solver && solver->sparse_solver) {
        // Refactorize using sparse direct solver
        result = sparse_direct_solver_refactorize(solver->sparse_solver, new_values);
        
    } else if (solver->native_solver) {
        // Refactorize using native solver
        result = linear_solver_refactorize(solver->native_solver, new_values, matrix_size);
    }
    
    solver->factorization_time = get_time() - start_time;
    
    return result;
}

int enhanced_linear_solver_get_stats(enhanced_linear_solver_t *solver,
                                    sparse_solver_stats_t *stats) {
    if (!solver || !stats) {
        return -1;
    }
    
    // Get stats from active solver
    if (solver->use_sparse_solver && solver->sparse_solver) {
        return sparse_direct_solver_get_stats(solver->sparse_solver, stats);
    } else if (solver->native_solver) {
        // For native solver, create basic stats
        stats->analysis_time = 0.0;
        stats->factorization_time = solver->factorization_time;
        stats->solve_time = solver->solve_time;
        stats->total_time = solver->factorization_time + solver->solve_time;
        stats->memory_usage = get_memory_usage();
        
        return 0;
    }
    
    return -1;
}

void enhanced_linear_solver_get_default_config(enhanced_solver_config_t *config) {
    if (!config) {
        return;
    }
    
    memset(config, 0, sizeof(enhanced_solver_config_t));
    
    // Base configuration
    config->base_type = SOLVER_TYPE_ITERATIVE_GMRES;
    config->backend = SOLVER_BACKEND_NATIVE;
    config->precision = SOLVER_PRECISION_DOUBLE;
    
    // Sparse direct solver configuration
    config->sparse_solver_type = SPARSE_SOLVER_MUMPS;
    sparse_direct_solver_get_default_config(SPARSE_SOLVER_MUMPS, &config->sparse_config);
    
    // Integration options
    config->use_sparse_direct_for_large_problems = true;
    config->sparse_direct_threshold_size = 1000;
    config->use_sparse_direct_for_multiple_rhs = true;
    
    // Out-of-core options
    config->enable_out_of_core = false;
    config->memory_threshold = 4ULL * 1024 * 1024 * 1024;  // 4GB
    
    // Performance tuning
    config->auto_select_solver = true;
    config->profile_performance = false;
}

void enhanced_linear_solver_destroy(enhanced_linear_solver_t *solver) {
    if (!solver) {
        return;
    }
    
    // Cleanup sparse direct solver
    if (solver->sparse_solver) {
        sparse_direct_solver_destroy(solver->sparse_solver);
    }
    
    // Cleanup native solver
    if (solver->native_solver) {
        linear_solver_destroy(solver->native_solver);
    }
    
    // Cleanup matrix storage
    sparse_matrix_free(&solver->sparse_matrix);
    free(solver->dense_matrix);
    free(solver->work_vector);
    
    free(solver);
}