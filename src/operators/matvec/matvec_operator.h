/********************************************************************************
 * Matrix-Vector Product Operator (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines matrix-vector product operators.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines operators, not numerical backend optimizations.
 ********************************************************************************/

#ifndef MATVEC_OPERATOR_H
#define MATVEC_OPERATOR_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../assembler/matrix_assembler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Matrix-Vector Product Interface
// ============================================================================

/**
 * Compute matrix-vector product
 * 
 * L3 layer defines the operator, numerical implementation is in L4
 * 
 * Operator: y = A * x
 */
int matvec_operator_apply(
    const operator_matrix_t* A,   // Operator matrix
    const operator_vector_t* x,   // Input vector
    operator_vector_t* y           // Output vector
);

/**
 * Compute matrix-transpose-vector product
 * 
 * Operator: y = A^T * x
 */
int matvec_operator_apply_transpose(
    const operator_matrix_t* A,
    const operator_vector_t* x,
    operator_vector_t* y
);

/**
 * Compute matrix-hermitian-vector product
 * 
 * Operator: y = A^H * x
 */
int matvec_operator_apply_hermitian(
    const operator_matrix_t* A,
    const operator_vector_t* x,
    operator_vector_t* y
);

/**
 * Create operator vector
 */
operator_vector_t* matvec_operator_create_vector(int size);

/**
 * Destroy operator vector
 */
void matvec_operator_destroy_vector(operator_vector_t* vector);

#ifdef __cplusplus
}
#endif

#endif // MATVEC_OPERATOR_H
