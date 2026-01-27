/********************************************************************************
 * Hybrid Solver Orchestration (L5 Execution Orchestration Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines hybrid solver orchestration.
 * L5 layer: Execution Orchestration - manages execution order and coupling.
 *
 * Architecture Rule: L5 orchestrates, does NOT implement physics or numerical code.
 * Hybrid solver must NOT implement physics kernels or numerical solvers.
 ********************************************************************************/

#ifndef HYBRID_SOLVER_H
#define HYBRID_SOLVER_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../physics/hybrid/hybrid_physics_boundary.h"
#include "../../operators/coupling/coupling_operator.h"
#include "../../backend/solvers/solver_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Hybrid Coupling Methods (Orchestration Strategies)
// ============================================================================

/**
 * Coupling Method
 * 
 * L5 layer defines orchestration strategies, not physics
 */
typedef enum {
    COUPLING_METHOD_SCHUR_COMPLEMENT = 1,      // Schur complement method
    COUPLING_METHOD_DOMAIN_DECOMPOSITION = 2,  // Domain decomposition
    COUPLING_METHOD_ITERATIVE_SUBDOMAIN = 3,   // Iterative subdomain
    COUPLING_METHOD_LAGRANGE_MULTIPLIERS = 4,  // Lagrange multipliers
    COUPLING_METHOD_PENALTY_METHOD = 5,        // Penalty method
    COUPLING_METHOD_MORTAR_METHOD = 6          // Mortar method
} hybrid_coupling_method_t;

/**
 * Hybrid Solver Configuration
 * 
 * L5 layer defines orchestration parameters, not physics parameters
 */
typedef struct {
    hybrid_coupling_method_t method;
    hybrid_coupling_type_t coupling_type;
    
    // Convergence criteria (orchestration level)
    int max_iterations;
    real_t convergence_tolerance;
    real_t relaxation_parameter;
    
    // Interface settings
    int num_interface_points;
    real_t interface_tolerance;
    bool use_adaptive_interface;
    
    // Execution settings
    bool use_parallel;
    int num_threads;
    bool enable_profiling;
} hybrid_solver_config_t;

/**
 * Hybrid Solver Structure
 * 
 * L5 layer orchestrates, does NOT implement
 */
typedef struct hybrid_solver hybrid_solver_t;

/**
 * Domain Solver Handles (opaque pointers)
 * 
 * L5 layer holds references to domain solvers, does NOT implement them
 */
typedef struct {
    void* mom_solver;        // MoM solver handle (from L4)
    void* peec_solver;       // PEEC solver handle (from L4)
    void* mtl_solver;        // MTL solver handle (from L4)
    
    bool mom_enabled;
    bool peec_enabled;
    bool mtl_enabled;
} domain_solvers_t;

/**
 * Execution Context
 * 
 * L5 layer manages execution context
 */
typedef struct {
    domain_solvers_t solvers;
    coupling_matrix_t* coupling_matrices;
    hybrid_interface_point_t* interface_points;
    int num_interface_points;
    
    // Execution state
    int current_iteration;
    bool is_converged;
    real_t current_error;
} execution_context_t;

// ============================================================================
// Hybrid Solver Interface
// ============================================================================

/**
 * Create hybrid solver
 * 
 * L5 layer creates orchestration structure, not physics or numerical code
 */
hybrid_solver_t* hybrid_solver_create(
    const hybrid_solver_config_t* config
);

/**
 * Destroy hybrid solver
 */
void hybrid_solver_destroy(hybrid_solver_t* solver);

/**
 * Register domain solver
 * 
 * L5 layer registers solvers, does NOT implement them
 */
int hybrid_solver_register_mom(
    hybrid_solver_t* solver,
    void* mom_solver_handle
);

int hybrid_solver_register_peec(
    hybrid_solver_t* solver,
    void* peec_solver_handle
);

int hybrid_solver_register_mtl(
    hybrid_solver_t* solver,
    void* mtl_solver_handle
);

/**
 * Set interface points
 * 
 * L5 layer manages interface, does NOT compute physics
 */
int hybrid_solver_set_interface(
    hybrid_solver_t* solver,
    const hybrid_interface_point_t* points,
    int num_points
);

/**
 * Execute hybrid simulation
 * 
 * L5 layer orchestrates execution, does NOT implement physics or numerical code
 */
int hybrid_solver_execute(
    hybrid_solver_t* solver
);

/**
 * Get execution status
 */
int hybrid_solver_get_status(
    const hybrid_solver_t* solver,
    bool* converged,
    int* iterations,
    real_t* error
);

#ifdef __cplusplus
}
#endif

#endif // HYBRID_SOLVER_H
