/********************************************************************************
 * Adaptive Cross Approximation (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines ACA compression for fast algorithms.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 provides acceleration, must preserve operator semantics.
 ********************************************************************************/

#ifndef ACA_H
#define ACA_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ACA Parameters
// ============================================================================

/**
 * ACA Compression Parameters
 */
typedef struct {
    real_t tolerance;        // Compression tolerance
    int max_rank;           // Maximum rank
    bool use_partial_pivoting; // Use partial pivoting
} aca_params_t;

/**
 * ACA Factorization Result
 */
typedef struct {
    complex_t* U;           // Left factor [n_rows * rank]
    complex_t* V;           // Right factor [rank * n_cols]
    int rank;               // Actual rank
    real_t compression_error; // Compression error
} aca_result_t;

// ============================================================================
// ACA Interface
// ============================================================================

/**
 * Perform ACA compression
 * 
 * L4 layer compresses matrix, preserves operator semantics
 */
int aca_compress(
    const complex_t* matrix,
    int n_rows, int n_cols,
    const aca_params_t* params,
    aca_result_t* result
);

/**
 * Destroy ACA result
 */
void aca_destroy_result(aca_result_t* result);

/**
 * ACA matrix-vector multiplication
 */
int aca_vector_multiply(
    const aca_result_t* aca_result,
    int n_rows, int n_cols,
    const complex_t* input_vector,
    complex_t* output_vector
);

#ifdef __cplusplus
}
#endif

#endif // ACA_H
