/********************************************************************************
 * Iterative Solver (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines iterative solver implementations.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 sees matrices, not physics.
 ********************************************************************************/

#ifndef ITERATIVE_SOLVER_H
#define ITERATIVE_SOLVER_H

#include "solver_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Iterative Solver Types
// ============================================================================

/**
 * Iterative Solver Method
 */
typedef enum {
    ITERATIVE_METHOD_GMRES = 1,      // GMRES
    ITERATIVE_METHOD_CG = 2,         // Conjugate Gradient
    ITERATIVE_METHOD_BICGSTAB = 3,   // BiCGSTAB
    ITERATIVE_METHOD_TFQMR = 4       // TFQMR
} iterative_method_t;

/**
 * Iterative Solver Configuration
 */
typedef struct {
    iterative_method_t method;
    solver_backend_t backend;
    
    // Convergence criteria
    real_t tolerance;
    int max_iterations;
    int restart_size;                // For GMRES
    
    // Preconditioning
    bool use_preconditioner;
    preconditioner_type_t preconditioner_type;
    real_t preconditioner_tolerance;
    
    // Convergence monitoring
    bool monitor_residual;
    int check_interval;
} iterative_solver_config_t;

// ============================================================================
// Iterative Solver Interface
// ============================================================================

/**
 * Create iterative solver
 */
solver_interface_t* iterative_solver_create(
    const iterative_solver_config_t* config
);

/**
 * Set preconditioner
 */
int iterative_solver_set_preconditioner(
    solver_interface_t* solver,
    const operator_matrix_t* preconditioner_matrix
);

/**
 * Solve iteratively
 */
int iterative_solver_solve(
    solver_interface_t* solver,
    const operator_matrix_t* matrix,
    const operator_vector_t* rhs,
    operator_vector_t* solution,
    solver_statistics_t* statistics
);

#ifdef __cplusplus
}
#endif

#endif // ITERATIVE_SOLVER_H
