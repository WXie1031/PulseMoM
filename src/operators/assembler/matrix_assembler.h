/********************************************************************************
 * Matrix Assembler (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines matrix assembly operators.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines assembly, not solver calls or backend optimizations.
 ********************************************************************************/

#ifndef MATRIX_ASSEMBLER_H
#define MATRIX_ASSEMBLER_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../physics/mom/mom_physics.h"
#include "../../physics/peec/peec_physics.h"
#include "../../discretization/mesh/mesh_engine.h"
#include "../../discretization/basis/rwg_basis.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Matrix Assembly Types
// ============================================================================

/**
 * Matrix Type
 */
typedef enum {
    MATRIX_TYPE_DENSE = 1,       // Dense matrix
    MATRIX_TYPE_SPARSE = 2,      // Sparse matrix
    MATRIX_TYPE_COMPRESSED = 3   // Compressed representation
} matrix_type_t;

/**
 * Assembly Formulation
 */
typedef enum {
    ASSEMBLY_FORMULATION_EFIE = 1,     // Electric Field Integral Equation
    ASSEMBLY_FORMULATION_MFIE = 2,     // Magnetic Field Integral Equation
    ASSEMBLY_FORMULATION_CFIE = 3,     // Combined Field Integral Equation
    ASSEMBLY_FORMULATION_POTENTIAL = 4, // Scalar/Vector Potential
    ASSEMBLY_FORMULATION_CIRCUIT = 5    // Circuit network
} assembly_formulation_t;

/**
 * Matrix Assembly Specification
 * 
 * L3 layer defines assembly parameters, not solver parameters
 */
typedef struct {
    assembly_formulation_t formulation;
    matrix_type_t matrix_type;
    
    // Problem dimensions
    int num_unknowns;
    int num_rhs;
    
    // Frequency information
    real_t frequency;
    
    // Accuracy control
    real_t tolerance;
    int num_quadrature_points;
    
    // Parallel processing (L3 defines parallelization strategy, not implementation)
    bool use_parallel;
    int num_threads;
} matrix_assembly_spec_t;

/**
 * Operator Matrix Structure
 * 
 * L3 layer defines the matrix structure for operators
 */
typedef struct {
    matrix_type_t type;
    int num_rows;
    int num_cols;
    
    // Matrix data (format depends on type)
    union {
        complex_t* dense;        // Dense matrix [num_rows * num_cols]
        void* sparse;            // Sparse matrix structure (defined in L4)
        void* compressed;         // Compressed structure (defined in L4)
    } data;
    
    // Metadata
    bool is_symmetric;
    bool is_hermitian;
} operator_matrix_t;

/**
 * Operator Vector Structure
 */
typedef struct {
    int size;
    complex_t* data;             // Vector data [size]
} operator_vector_t;

// ============================================================================
// Matrix Assembly Interface
// ============================================================================

/**
 * Assemble MoM impedance matrix
 * 
 * L3 layer assembles operator matrix, not solver matrix
 */
int matrix_assembler_assemble_mom(
    const mesh_t* mesh,
    const rwg_basis_set_t* basis_set,
    const matrix_assembly_spec_t* spec,
    operator_matrix_t* matrix
);

/**
 * Assemble PEEC circuit matrix
 * 
 * L3 layer assembles operator matrix for PEEC
 */
int matrix_assembler_assemble_peec(
    const mesh_t* mesh,
    const matrix_assembly_spec_t* spec,
    operator_matrix_t* matrix
);

/**
 * Assemble excitation vector
 * 
 * L3 layer assembles RHS vector from excitation
 */
int matrix_assembler_assemble_excitation(
    const mesh_t* mesh,
    const rwg_basis_set_t* basis_set,
    const mom_plane_wave_t* excitation,
    operator_vector_t* rhs
);

/**
 * Create operator matrix
 */
operator_matrix_t* matrix_assembler_create_matrix(
    matrix_type_t type,
    int num_rows,
    int num_cols
);

/**
 * Destroy operator matrix
 */
void matrix_assembler_destroy_matrix(operator_matrix_t* matrix);

#ifdef __cplusplus
}
#endif

#endif // MATRIX_ASSEMBLER_H
