/********************************************************************************
 * Solver Interface (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the unified solver interface.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 sees matrices, not physics. No physics assumptions.
 ********************************************************************************/

#ifndef SOLVER_INTERFACE_H
#define SOLVER_INTERFACE_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../operators/assembler/matrix_assembler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Solver Types
// ============================================================================

/**
 * Solver Type
 * 
 * L4 layer defines numerical methods, not physics
 */
typedef enum {
    SOLVER_TYPE_DENSE_LU = 1,         // Dense LU decomposition
    SOLVER_TYPE_DENSE_QR = 2,         // Dense QR decomposition
    SOLVER_TYPE_SPARSE_LU = 3,        // Sparse LU
    SOLVER_TYPE_SPARSE_CHOLESKY = 4,  // Sparse Cholesky
    SOLVER_TYPE_ITERATIVE_GMRES = 5,  // GMRES
    SOLVER_TYPE_ITERATIVE_CG = 6,     // Conjugate Gradient
    SOLVER_TYPE_ITERATIVE_BICGSTAB = 7, // BiCGSTAB
    SOLVER_TYPE_ITERATIVE_TFQMR = 8   // TFQMR
} solver_type_t;

/**
 * Solver Backend
 */
typedef enum {
    SOLVER_BACKEND_NATIVE = 1,        // Native implementation
    SOLVER_BACKEND_MKL = 2,           // Intel MKL
    SOLVER_BACKEND_OPENBLAS = 3,      // OpenBLAS
    SOLVER_BACKEND_CUDA = 4,          // NVIDIA CUDA
    SOLVER_BACKEND_OPENCL = 5,        // OpenCL BLAS
    SOLVER_BACKEND_UMFPACK = 6,       // UMFPACK for sparse
    SOLVER_BACKEND_KLU = 7            // KLU for sparse
} solver_backend_t;

/**
 * Preconditioner Type
 */
typedef enum {
    PRECONDITIONER_NONE = 0,
    PRECONDITIONER_ILU = 1,           // Incomplete LU
    PRECONDITIONER_IC = 2,            // Incomplete Cholesky
    PRECONDITIONER_JACOBI = 3,        // Jacobi
    PRECONDITIONER_GAUSS_SEIDEL = 4,  // Gauss-Seidel
    PRECONDITIONER_SSOR = 5,          // Symmetric SOR
    PRECONDITIONER_MULTIGRID = 6      // Multigrid
} preconditioner_type_t;

// ============================================================================
// Solver Configuration
// ============================================================================

/**
 * Solver Configuration
 * 
 * L4 layer defines numerical parameters, not physics parameters
 */
typedef struct {
    solver_type_t type;
    solver_backend_t backend;
    
    // Convergence criteria
    real_t tolerance;
    int max_iterations;
    int restart_size;                 // For GMRES
    
    // Preconditioning
    bool use_preconditioner;
    preconditioner_type_t preconditioner_type;
    real_t preconditioner_drop_tolerance;
    int preconditioner_fill_level;
    
    // Parallel processing
    bool use_parallel;
    int num_threads;
    
    // GPU acceleration
    bool use_gpu;
    int gpu_device_id;
    
    // Memory management
    bool use_out_of_core;
    size_t memory_limit;
} solver_config_t;

/**
 * Solver Statistics
 */
typedef struct {
    int iterations;
    real_t residual_norm;
    real_t solve_time;
    real_t setup_time;
    size_t memory_usage;
    int num_restarts;
    bool converged;
} solver_statistics_t;

/**
 * Solver Result
 */
typedef struct {
    operator_vector_t* solution;      // Solution vector
    solver_statistics_t statistics;   // Solver statistics
    status_t status;                  // Solution status
} solver_result_t;

// ============================================================================
// Solver Interface
// ============================================================================

/**
 * Solver Interface Structure
 * 
 * L4 layer provides unified interface to numerical solvers
 */
typedef struct solver_interface solver_interface_t;

/**
 * Create solver interface
 */
solver_interface_t* solver_interface_create(
    const solver_config_t* config
);

/**
 * Destroy solver interface
 */
void solver_interface_destroy(solver_interface_t* solver);

/**
 * Set operator matrix
 * 
 * L4 layer receives operator matrix from L3, not physics
 */
int solver_interface_set_matrix(
    solver_interface_t* solver,
    const operator_matrix_t* matrix
);

/**
 * Set right-hand side vector
 */
int solver_interface_set_rhs(
    solver_interface_t* solver,
    const operator_vector_t* rhs
);

/**
 * Solve linear system
 * 
 * L4 layer solves numerically, no physics knowledge
 */
int solver_interface_solve(
    solver_interface_t* solver,
    solver_result_t* result
);

/**
 * Get solution vector
 */
int solver_interface_get_solution(
    const solver_interface_t* solver,
    operator_vector_t* solution
);

/**
 * Get solver statistics
 */
int solver_interface_get_statistics(
    const solver_interface_t* solver,
    solver_statistics_t* statistics
);

#ifdef __cplusplus
}
#endif

#endif // SOLVER_INTERFACE_H
