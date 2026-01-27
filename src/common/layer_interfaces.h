/********************************************************************************
 * Layer Interfaces (Common)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the interfaces between layers.
 * Architecture Rule: Layers communicate ONLY through these interfaces.
 *
 * L1 → L2: Physics needs geometry/mesh information
 * L2 → L3: Discretization provides topology for operators
 * L3 → L4: Operators provide matrices/vectors for numerical backend
 * L4 → L5: Numerical backend provides solution interface
 * L5 → L6: Orchestration provides results for IO
 ********************************************************************************/

#ifndef LAYER_INTERFACES_H
#define LAYER_INTERFACES_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Forward Declarations (to avoid circular dependencies)
// ============================================================================

// L1 Physics Layer
typedef struct mom_physics_def mom_physics_def_t;
typedef struct peec_physics_def peec_physics_def_t;
typedef struct mtl_physics_def mtl_physics_def_t;

// L2 Discretization Layer
typedef struct geometry_engine geometry_engine_t;
typedef struct mesh_engine mesh_engine_t;
typedef struct rwg_basis_set rwg_basis_set_t;

// L3 Operators Layer
typedef struct operator_matrix operator_matrix_t;
typedef struct operator_vector operator_vector_t;

// L4 Numerical Backend
typedef struct solver_interface solver_interface_t;
typedef struct solver_result solver_result_t;

// L5 Orchestration
typedef struct execution_context execution_context_t;

// ============================================================================
// L1 → L2 Interface
// ============================================================================

/**
 * L1 Physics requests geometry information from L2
 * 
 * Architecture: L1 defines WHAT physics needs, L2 provides HOW to get it
 */
typedef struct {
    // Request geometry entities
    int (*get_geometry_entities)(
        geometry_engine_t* engine,
        int domain_type,  // MOM_DOMAIN, PEEC_DOMAIN, etc.
        void** entities,
        int* num_entities
    );
    
    // Request mesh information
    int (*get_mesh_info)(
        mesh_engine_t* engine,
        int domain_type,
        int* num_vertices,
        int* num_elements
    );
} l1_to_l2_interface_t;

// ============================================================================
// L2 → L3 Interface
// ============================================================================

/**
 * L2 Discretization provides topology to L3 Operators
 * 
 * Architecture: L2 provides discretization, L3 uses it for operators
 */
typedef struct {
    // Provide mesh topology
    int (*provide_mesh_topology)(
        mesh_engine_t* engine,
        void* operator_context
    );
    
    // Provide basis function information
    int (*provide_basis_functions)(
        rwg_basis_set_t* basis_set,
        void* operator_context
    );
    
    // Provide geometric properties
    int (*provide_geometric_properties)(
        geometry_engine_t* engine,
        void* operator_context
    );
} l2_to_l3_interface_t;

// ============================================================================
// L3 → L4 Interface
// ============================================================================

/**
 * L3 Operators provide matrices/vectors to L4 Numerical Backend
 * 
 * Architecture: L3 computes operators, L4 solves numerically
 */
typedef struct {
    // Provide operator matrix
    int (*provide_operator_matrix)(
        operator_matrix_t* matrix,
        solver_interface_t* solver
    );
    
    // Provide right-hand side vector
    int (*provide_rhs_vector)(
        operator_vector_t* vector,
        solver_interface_t* solver
    );
    
    // Request solution vector
    int (*request_solution_vector)(
        solver_interface_t* solver,
        operator_vector_t* solution
    );
} l3_to_l4_interface_t;

// ============================================================================
// L4 → L5 Interface
// ============================================================================

/**
 * L4 Numerical Backend provides solution interface to L5 Orchestration
 * 
 * Architecture: L4 solves, L5 orchestrates
 */
typedef struct {
    // Solve linear system
    int (*solve)(
        solver_interface_t* solver,
        solver_result_t* result
    );
    
    // Get solution status
    int (*get_solution_status)(
        solver_interface_t* solver,
        bool* converged,
        int* iterations,
        real_t* residual
    );
    
    // Get solution vector
    int (*get_solution_vector)(
        solver_interface_t* solver,
        complex_t* solution,
        int* size
    );
} l4_to_l5_interface_t;

// ============================================================================
// L5 → L6 Interface
// ============================================================================

/**
 * L5 Orchestration provides results to L6 IO
 * 
 * Architecture: L5 manages execution, L6 handles output
 */
typedef struct {
    // Provide execution results
    int (*provide_results)(
        execution_context_t* context,
        void* io_handler
    );
    
    // Provide performance metrics
    int (*provide_metrics)(
        execution_context_t* context,
        void* io_handler
    );
} l5_to_l6_interface_t;

// ============================================================================
// Interface Registration
// ============================================================================

/**
 * Register L1 → L2 interface
 */
int layer_interface_register_l1_to_l2(const l1_to_l2_interface_t* interface);

/**
 * Register L2 → L3 interface
 */
int layer_interface_register_l2_to_l3(const l2_to_l3_interface_t* interface);

/**
 * Register L3 → L4 interface
 */
int layer_interface_register_l3_to_l4(const l3_to_l4_interface_t* interface);

/**
 * Register L4 → L5 interface
 */
int layer_interface_register_l4_to_l5(const l4_to_l5_interface_t* interface);

/**
 * Register L5 → L6 interface
 */
int layer_interface_register_l5_to_l6(const l5_to_l6_interface_t* interface);

#ifdef __cplusplus
}
#endif

#endif // LAYER_INTERFACES_H
