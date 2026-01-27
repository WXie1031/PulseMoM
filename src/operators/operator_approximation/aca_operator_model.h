/********************************************************************************
 * ACA Operator Model (L3 Operator Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the mathematical model for Adaptive Cross Approximation.
 * L3 layer: Operator - WHAT the operator approximation is mathematically.
 *
 * Architecture Rule: L3 defines the mathematical meaning of operator approximation,
 * not the computational implementation (which is in L4).
 ********************************************************************************/

#ifndef ACA_OPERATOR_MODEL_H
#define ACA_OPERATOR_MODEL_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ACA Mathematical Model
// ============================================================================

/**
 * ACA approximates a matrix M ∈ C^(m×n) as a low-rank product:
 * 
 *   M ≈ U * V^T
 * 
 * where:
 *   U ∈ C^(m×k) - left factor
 *   V ∈ C^(n×k) - right factor
 *   k << min(m,n) - rank
 * 
 * The approximation is constructed adaptively by selecting pivot rows/columns
 * and computing residuals until convergence.
 */

/**
 * ACA Compression Parameters (Mathematical Definition)
 * 
 * These parameters define the mathematical properties of the approximation,
 * not the computational strategy.
 */
typedef struct {
    real_t tolerance;        // Compression tolerance (mathematical error bound)
    int max_rank;           // Maximum rank (mathematical constraint)
    bool use_partial_pivoting; // Use partial pivoting (mathematical strategy)
} aca_operator_params_t;

/**
 * ACA Factorization Result (Mathematical Model)
 * 
 * Represents the mathematical decomposition M ≈ U * V^T
 */
typedef struct {
    complex_t* U;           // Left factor [n_rows * rank]
    complex_t* V;           // Right factor [rank * n_cols]
    int rank;               // Actual rank (mathematical property)
    real_t compression_error; // Compression error (mathematical measure)
} aca_operator_result_t;

/**
 * Admissibility Condition for ACA
 * 
 * Determines whether a matrix block is admissible for low-rank approximation.
 * 
 * A block is admissible if:
 *   dist(cluster_i, cluster_j) > η * max(diam(cluster_i), diam(cluster_j))
 * 
 * where:
 *   dist = distance between cluster centroids
 *   diam = diameter of cluster
 *   η = admissibility parameter (typically 1.0-2.0)
 * 
 * @param cluster_i_center Center of cluster i
 * @param cluster_j_center Center of cluster j
 * @param cluster_i_radius Radius of cluster i
 * @param cluster_j_radius Radius of cluster j
 * @param eta Admissibility parameter
 * @return true if block is admissible for low-rank approximation
 */
bool aca_is_admissible(
    const double* cluster_i_center,
    const double* cluster_j_center,
    double cluster_i_radius,
    double cluster_j_radius,
    double eta
);

/**
 * Compute ACA approximation error (mathematical measure)
 * 
 * Error is typically measured as:
 *   ||M - U*V^T||_F / ||M||_F
 * 
 * where ||·||_F is the Frobenius norm.
 * 
 * @param original_matrix Original matrix M
 * @param n_rows Number of rows
 * @param n_cols Number of columns
 * @param aca_result ACA approximation result
 * @return Relative error (0.0 = perfect, 1.0 = 100% error)
 */
real_t aca_compute_error(
    const complex_t* original_matrix,
    int n_rows, int n_cols,
    const aca_operator_result_t* aca_result
);

#ifdef __cplusplus
}
#endif

#endif // ACA_OPERATOR_MODEL_H
