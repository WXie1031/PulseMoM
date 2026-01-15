/**
 * @file core_solver_sparse_direct.h
 * @brief Integration of commercial-grade sparse direct solvers with core solver
 * @details Enhanced solver interface with MUMPS/PARDISO support
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CORE_SOLVER_SPARSE_DIRECT_H
#define CORE_SOLVER_SPARSE_DIRECT_H

#include "core_solver.h"
#include "sparse_direct_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Enhanced solver configuration with sparse direct solver support
typedef struct {
    // Base solver configuration
    solver_type_t base_type;
    solver_backend_t backend;
    solver_precision_t precision;
    
    // Sparse direct solver configuration
    sparse_solver_type_t sparse_solver_type;
    sparse_solver_config_t sparse_config;
    
    // Integration options
    bool use_sparse_direct_for_large_problems;
    int sparse_direct_threshold_size;  // Switch to sparse direct when matrix size > this
    bool use_sparse_direct_for_multiple_rhs;
    
    // Out-of-core options
    bool enable_out_of_core;
    size_t memory_threshold;  // Switch to OOC when memory usage > this
    
    // Performance tuning
    bool auto_select_solver;  // Automatically select best solver
    bool profile_performance;   // Enable performance profiling
    
} enhanced_solver_config_t;

// Enhanced solver handle
typedef struct enhanced_linear_solver enhanced_linear_solver_t;

// Function prototypes

/**
 * @brief Initialize enhanced linear solver with sparse direct solver support
 * @param config Enhanced solver configuration
 * @return Enhanced solver handle or NULL on failure
 */
enhanced_linear_solver_t* enhanced_linear_solver_init(const enhanced_solver_config_t *config);

/**
 * @brief Solve linear system using enhanced solver
 * @param solver Enhanced solver handle
 * @param matrix Matrix in sparse or dense format
 * @param rhs Right-hand side vector
 * @param solution Solution vector
 * @param matrix_size Matrix dimension
 * @param num_rhs Number of right-hand sides
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_solve(enhanced_linear_solver_t *solver,
                                  void *matrix,  // Can be sparse_matrix_t or double complex*
                                  const double complex *rhs,
                                  double complex *solution,
                                  int matrix_size,
                                  int num_rhs);

/**
 * @brief Solve with multiple right-hand sides efficiently
 * @param solver Enhanced solver handle
 * @param matrix Matrix
 * @param rhs Right-hand side matrix
 * @param solution Solution matrix
 * @param matrix_size Matrix dimension
 * @param num_rhs Number of right-hand sides
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_solve_multiple(enhanced_linear_solver_t *solver,
                                         void *matrix,
                                         const double complex *rhs,
                                         double complex *solution,
                                         int matrix_size,
                                         int num_rhs);

/**
 * @brief Factorize matrix for reuse
 * @param solver Enhanced solver handle
 * @param matrix Matrix to factorize
 * @param matrix_size Matrix dimension
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_factorize(enhanced_linear_solver_t *solver,
                                    void *matrix,
                                    int matrix_size);

/**
 * @brief Solve with pre-factorized matrix
 * @param solver Enhanced solver handle
 * @param rhs Right-hand side vector
 * @param solution Solution vector
 * @param num_rhs Number of right-hand sides
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_solve_factorized(enhanced_linear_solver_t *solver,
                                           const double complex *rhs,
                                           double complex *solution,
                                           int num_rhs);

/**
 * @brief Refactorize with new matrix values (same structure)
 * @param solver Enhanced solver handle
 * @param new_values New matrix values
 * @param matrix_size Matrix dimension
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_refactorize(enhanced_linear_solver_t *solver,
                                      const double complex *new_values,
                                      int matrix_size);

/**
 * @brief Get solver statistics and performance metrics
 * @param solver Enhanced solver handle
 * @param stats Output statistics
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_get_stats(enhanced_linear_solver_t *solver,
                                    sparse_solver_stats_t *stats);

/**
 * @brief Auto-select best solver for given problem
 * @param matrix_size Matrix dimension
 * @param nnz Number of non-zeros (for sparse matrices)
 * @param num_rhs Number of right-hand sides
 * @param memory_available Available memory in bytes
 * @param config Output recommended configuration
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_auto_select(int matrix_size,
                                      int64_t nnz,
                                      int num_rhs,
                                      size_t memory_available,
                                      enhanced_solver_config_t *config);

/**
 * @brief Check if sparse direct solver is recommended
 * @param matrix_size Matrix dimension
 * @param nnz Number of non-zeros
 * @param num_rhs Number of right-hand sides
 * @param memory_available Available memory in bytes
 * @return true if sparse direct solver is recommended
 */
bool enhanced_linear_solver_should_use_sparse_direct(int matrix_size,
                                                    int64_t nnz,
                                                    int num_rhs,
                                                    size_t memory_available);

/**
 * @brief Get default enhanced configuration
 * @param config Output configuration
 */
void enhanced_linear_solver_get_default_config(enhanced_solver_config_t *config);

/**
 * @brief Cleanup enhanced solver resources
 * @param solver Enhanced solver handle
 */
void enhanced_linear_solver_destroy(enhanced_linear_solver_t *solver);

// Utility functions for solver selection

/**
 * @brief Estimate memory requirements for different solvers
 * @param matrix_size Matrix dimension
 * @param nnz Number of non-zeros (0 for dense)
 * @param solver_type Solver type
 * @param memory_needed Output estimated memory in bytes
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_estimate_memory(int matrix_size,
                                          int64_t nnz,
                                          solver_type_t solver_type,
                                          size_t *memory_needed);

/**
 * @brief Benchmark different solvers for given problem
 * @param matrix Test matrix
 * @param rhs Test right-hand side
 * @param matrix_size Matrix dimension
 * @param results Output benchmark results
 * @return 0 on success, error code on failure
 */
int enhanced_linear_solver_benchmark_solvers(void *matrix,
                                           const double complex *rhs,
                                           int matrix_size,
                                           void *results);

#ifdef __cplusplus
}
#endif

#endif // CORE_SOLVER_SPARSE_DIRECT_H