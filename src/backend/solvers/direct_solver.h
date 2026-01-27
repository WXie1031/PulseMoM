/********************************************************************************
 * Direct Solver (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines direct solver implementations.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 sees matrices, not physics.
 ********************************************************************************/

#ifndef DIRECT_SOLVER_H
#define DIRECT_SOLVER_H

#include "solver_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Direct Solver Types
// ============================================================================

/**
 * Direct Solver Method
 */
typedef enum {
    DIRECT_METHOD_LU = 1,            // LU decomposition
    DIRECT_METHOD_QR = 2,            // QR decomposition
    DIRECT_METHOD_CHOLESKY = 3,      // Cholesky decomposition
    DIRECT_METHOD_SVD = 4            // Singular Value Decomposition
} direct_method_t;

/**
 * Direct Solver Configuration
 */
typedef struct {
    direct_method_t method;
    solver_backend_t backend;
    
    // Pivoting
    bool use_pivoting;
    real_t pivot_tolerance;
    
    // Memory management
    bool use_out_of_core;
    size_t memory_limit;
} direct_solver_config_t;

// ============================================================================
// Direct Solver Interface
// ============================================================================

/**
 * Create direct solver
 */
solver_interface_t* direct_solver_create(
    const direct_solver_config_t* config
);

/**
 * Factorize matrix
 * 
 * L4 layer performs numerical factorization, no physics
 */
int direct_solver_factorize(
    solver_interface_t* solver,
    const operator_matrix_t* matrix
);

/**
 * Solve using factorization
 */
int direct_solver_solve(
    solver_interface_t* solver,
    const operator_vector_t* rhs,
    operator_vector_t* solution
);

#ifdef __cplusplus
}
#endif

#endif // DIRECT_SOLVER_H
