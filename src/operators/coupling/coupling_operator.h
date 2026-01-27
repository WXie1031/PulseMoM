/********************************************************************************
 * Coupling Operators (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines coupling operators between different domains.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines coupling operators, not execution orchestration.
 ********************************************************************************/

#ifndef COUPLING_OPERATOR_H
#define COUPLING_OPERATOR_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../physics/hybrid/hybrid_physics_boundary.h"
#include "../assembler/matrix_assembler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Coupling Operator Types
// ============================================================================

/**
 * Coupling Operator
 * 
 * L3 layer defines the coupling operator between domains
 */
typedef struct {
    hybrid_coupling_type_t coupling_type;  // Coupling type
    
    // Source and target domains
    int source_domain;                     // MoM, PEEC, or MTL
    int target_domain;
    
    // Coupling parameters
    real_t coupling_strength;
    real_t frequency;
} coupling_operator_t;

/**
 * Coupling Matrix
 * 
 * L3 layer defines coupling matrix structure
 */
typedef struct {
    int source_size;                       // Size of source domain
    int target_size;                       // Size of target domain
    complex_t* matrix;                     // Coupling matrix [source_size * target_size]
} coupling_matrix_t;

// ============================================================================
// Coupling Operator Interface
// ============================================================================

/**
 * Create coupling operator
 */
coupling_operator_t* coupling_operator_create(
    hybrid_coupling_type_t coupling_type,
    int source_domain,
    int target_domain
);

/**
 * Destroy coupling operator
 */
void coupling_operator_destroy(coupling_operator_t* op);

/**
 * Assemble coupling matrix
 * 
 * L3 layer assembles coupling operator matrix
 */
int coupling_operator_assemble_matrix(
    const coupling_operator_t* op,
    const hybrid_interface_point_t* interface_points,
    int num_points,
    coupling_matrix_t* matrix
);

/**
 * Apply coupling operator
 * 
 * Operator: target = coupling_matrix * source
 */
int coupling_operator_apply(
    const coupling_matrix_t* coupling_matrix,
    const operator_vector_t* source_vector,
    operator_vector_t* target_vector
);

/**
 * Create coupling matrix
 */
coupling_matrix_t* coupling_operator_create_matrix(
    int source_size,
    int target_size
);

/**
 * Destroy coupling matrix
 */
void coupling_operator_destroy_matrix(coupling_matrix_t* matrix);

#ifdef __cplusplus
}
#endif

#endif // COUPLING_OPERATOR_H
