/**
 * @file sparse_direct_solver.h
 * @brief Commercial-grade sparse direct solver interface (MUMPS/PARDISO)
 * @details Unified interface for high-performance sparse direct solvers
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef SPARSE_DIRECT_SOLVER_H
#define SPARSE_DIRECT_SOLVER_H

#include <stdint.h>
#include <stdbool.h>
#include "core_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sparse matrix formats
typedef enum {
    SPARSE_FORMAT_CSR,      // Compressed Sparse Row
    SPARSE_FORMAT_CSC,      // Compressed Sparse Column
    SPARSE_FORMAT_COO,      // Coordinate format
    SPARSE_FORMAT_BSR,      // Block Sparse Row
    SPARSE_FORMAT_VBR       // Variable Block Row
} sparse_format_t;

// Solver types
typedef enum {
    SPARSE_SOLVER_MUMPS,    // MUMPS (MUltifrontal Massively Parallel Solver)
    SPARSE_SOLVER_PARDISO,  // Intel MKL PARDISO
    SPARSE_SOLVER_UMFPACK,  // UMFPACK
    SPARSE_SOLVER_KLU,      // KLU (SuiteSparse)
    SPARSE_SOLVER_SUPERLU   // SuperLU
} sparse_solver_type_t;

// Matrix properties
typedef enum {
    MATRIX_PROPERTY_GENERAL,
    MATRIX_PROPERTY_SYMMETRIC,
    MATRIX_PROPERTY_HERMITIAN,
    MATRIX_PROPERTY_POSITIVE_DEFINITE,
    MATRIX_PROPERTY_STRUCTURALLY_SYMMETRIC
} matrix_property_t;

// Solver phases for MUMPS/PARDISO
typedef enum {
    PHASE_ANALYSIS = 1,       // Symbolic analysis only
    PHASE_FACTORIZATION = 2,    // Numerical factorization
    PHASE_SOLUTION = 3,         // Forward/backward substitution
    PHASE_REFACTORIZATION = 4   // Re-factorization with new values
} solver_phase_t;

// Solver statistics
typedef struct {
    double analysis_time;
    double factorization_time;
    double solve_time;
    double total_time;
    
    int64_t nnz_L;            // Nonzeros in L factor
    int64_t nnz_U;            // Nonzeros in U factor
    int64_t memory_usage;       // Memory in bytes
    int64_t floating_point_ops; // Floating point operations
    
    int num_pivots;           // Number of pivots
    double condition_number;   // Estimated condition number
    int rank;                 // Numerical rank
    
    bool singular;            // Matrix is singular
    int singularity_level;    // Level of singularity
} sparse_solver_stats_t;

// Solver configuration
typedef struct {
    sparse_solver_type_t solver_type;
    matrix_property_t matrix_property;
    
    // Performance settings
    bool parallel_analysis;
    bool parallel_factorization;
    bool parallel_solution;
    int num_threads;
    
    // Memory management
    bool use_out_of_core;     // Enable out-of-core mode
    size_t memory_limit;      // Memory limit in bytes
    char working_directory[256]; // Working directory for OOC
    
    // Pivoting and scaling
    bool use_scaling;
    bool use_pivoting;
    double pivot_tolerance;
    
    // Iterative refinement
    bool use_iterative_refinement;
    int max_refinement_steps;
    double refinement_tolerance;
    
    // Output control
    bool verbose;
    int message_level;        // 0=silent, 1=error, 2=warning, 3=info, 4=debug
    
    // Advanced options
    bool use_metis_ordering;
    bool use_amd_ordering;
    int ordering_strategy;
    
    // GPU acceleration (PARDISO)
    bool use_gpu;
    int gpu_device_id;
    
} sparse_solver_config_t;

// Sparse matrix structure
typedef struct {
    sparse_format_t format;
    int num_rows;
    int num_cols;
    int64_t nnz;              // Number of non-zeros
    
    // CSR format
    int *row_ptr;             // Row pointer array (size: num_rows + 1)
    int *col_idx;             // Column indices (size: nnz)
    complex_t *values;    // Non-zero values (size: nnz)
    
    // Additional arrays for symmetric/Hermitian matrices
    char *uplo;               // Upper/lower triangular part
    
    // Matrix properties
    matrix_property_t property;
    bool is_complex;
    
} sparse_matrix_t;

// Solver handle
typedef struct sparse_direct_solver sparse_direct_solver_t;

// Function prototypes

/**
 * @brief Initialize sparse direct solver
 * @param config Solver configuration
 * @return Solver handle or NULL on failure
 */
sparse_direct_solver_t* sparse_direct_solver_init(const sparse_solver_config_t *config);

/**
 * @brief Set matrix for solver
 * @param solver Solver handle
 * @param matrix Sparse matrix
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_set_matrix(sparse_direct_solver_t *solver, const sparse_matrix_t *matrix);

/**
 * @brief Perform symbolic analysis
 * @param solver Solver handle
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_analyze(sparse_direct_solver_t *solver);

/**
 * @brief Perform numerical factorization
 * @param solver Solver handle
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_factorize(sparse_direct_solver_t *solver);

/**
 * @brief Solve linear system Ax = b
 * @param solver Solver handle
 * @param rhs Right-hand side vector (size: num_rows)
 * @param solution Solution vector (size: num_cols)
 * @param num_rhs Number of right-hand sides
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_solve(sparse_direct_solver_t *solver, 
                              const complex_t *rhs, 
                              complex_t *solution, 
                              int num_rhs);

/**
 * @brief Solve with multiple right-hand sides
 * @param solver Solver handle
 * @param rhs Right-hand side matrix (size: num_rows * num_rhs)
 * @param solution Solution matrix (size: num_cols * num_rhs)
 * @param num_rhs Number of right-hand sides
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_solve_multiple(sparse_direct_solver_t *solver,
                                       const complex_t *rhs,
                                       complex_t *solution,
                                       int num_rhs);

/**
 * @brief Refactorize with new matrix values (same structure)
 * @param solver Solver handle
 * @param new_values New non-zero values (size: nnz)
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_refactorize(sparse_direct_solver_t *solver,
                                    const complex_t *new_values);

/**
 * @brief Get solver statistics
 * @param solver Solver handle
 * @param stats Output statistics structure
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_get_stats(const sparse_direct_solver_t *solver,
                                  sparse_solver_stats_t *stats);

/**
 * @brief Check if matrix is singular
 * @param solver Solver handle
 * @return true if singular, false otherwise
 */
bool sparse_direct_solver_is_singular(const sparse_direct_solver_t *solver);

/**
 * @brief Get condition number estimate
 * @param solver Solver handle
 * @param cond_number Output condition number
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_get_condition_number(const sparse_direct_solver_t *solver,
                                             double *cond_number);

/**
 * @brief Set solver parameter
 * @param solver Solver handle
 * @param param_name Parameter name
 * @param param_value Parameter value
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_set_parameter(sparse_direct_solver_t *solver,
                                      const char *param_name,
                                      double param_value);

/**
 * @brief Get solver parameter
 * @param solver Solver handle
 * @param param_name Parameter name
 * @param param_value Output parameter value
 * @return 0 on success, error code on failure
 */
int sparse_direct_solver_get_parameter(const sparse_direct_solver_t *solver,
                                      const char *param_name,
                                      double *param_value);

/**
 * @brief Cleanup solver resources
 * @param solver Solver handle
 */
void sparse_direct_solver_destroy(sparse_direct_solver_t *solver);

/**
 * @brief Convert matrix to different format
 * @param matrix Input matrix
 * @param target_format Target format
 * @param output_matrix Output matrix
 * @return 0 on success, error code on failure
 */
int sparse_matrix_convert(const sparse_matrix_t *matrix,
                         sparse_format_t target_format,
                         sparse_matrix_t *output_matrix);

/**
 * @brief Create sparse matrix from dense matrix
 * @param dense_matrix Dense matrix (row-major)
 * @param num_rows Number of rows
 * @param num_cols Number of columns
 * @param threshold Threshold for considering element as non-zero
 * @param output_matrix Output sparse matrix
 * @return 0 on success, error code on failure
 */
int sparse_matrix_from_dense(const complex_t *dense_matrix,
                            int num_rows, int num_cols,
                            double threshold,
                            sparse_matrix_t *output_matrix);

/**
 * @brief Create dense matrix from sparse matrix
 * @param sparse_matrix Input sparse matrix
 * @param output_dense_matrix Output dense matrix (row-major)
 * @return 0 on success, error code on failure
 */
int sparse_matrix_to_dense(const sparse_matrix_t *sparse_matrix,
                          complex_t *output_dense_matrix);

/**
 * @brief Free sparse matrix memory
 * @param matrix Sparse matrix
 */
void sparse_matrix_free(sparse_matrix_t *matrix);

/**
 * @brief Get solver version string
 * @param solver_type Solver type
 * @return Version string or NULL if not available
 */
const char* sparse_direct_solver_get_version(sparse_solver_type_t solver_type);

/**
 * @brief Check if solver is available
 * @param solver_type Solver type
 * @return true if available, false otherwise
 */
bool sparse_direct_solver_is_available(sparse_solver_type_t solver_type);

/**
 * @brief Get default configuration for solver
 * @param solver_type Solver type
 * @param config Output configuration
 */
void sparse_direct_solver_get_default_config(sparse_solver_type_t solver_type,
                                            sparse_solver_config_t *config);

#ifdef __cplusplus
}
#endif

#endif // SPARSE_DIRECT_SOLVER_H
