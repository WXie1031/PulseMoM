/********************************************************************************
 * H-Matrix Operator Model (L3 Operator Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the mathematical model for Hierarchical Matrices.
 * L3 layer: Operator - WHAT the operator approximation is mathematically.
 *
 * Architecture Rule: L3 defines the mathematical meaning of operator approximation,
 * not the computational implementation (which is in L4).
 ********************************************************************************/

#ifndef HMATRIX_OPERATOR_MODEL_H
#define HMATRIX_OPERATOR_MODEL_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// H-Matrix Mathematical Model
// ============================================================================

/**
 * H-Matrix represents a matrix M ∈ C^(n×n) in a hierarchical block structure:
 * 
 *   M = [M_11  M_12]
 *       [M_21  M_22]
 * 
 * where each block M_ij is either:
 *   - Dense (for near-field interactions)
 *   - Low-rank (for far-field interactions): M_ij ≈ U_ij * V_ij^T
 * 
 * The hierarchy is defined by a cluster tree that partitions the index set.
 */

/**
 * H-Matrix Parameters (Mathematical Definition)
 * 
 * These parameters define the mathematical properties of the H-matrix structure,
 * not the computational strategy.
 */
typedef struct {
    int cluster_size;      // Minimum cluster size for compression (mathematical constraint)
    double tolerance;      // Compression tolerance (mathematical error bound)
    int max_rank;         // Maximum rank for low-rank approximation (mathematical constraint)
    double admissibility; // Admissibility parameter η (mathematical criterion)
} hmatrix_operator_params_t;

/**
 * H-Matrix Block (Mathematical Model)
 * 
 * Represents a block in the H-matrix structure, which can be either
 * dense or low-rank.
 */
typedef struct {
    int *row_indices;      // Row indices for this block
    int *col_indices;      // Column indices for this block
    int n_rows, n_cols;    // Block dimensions
    
    bool is_low_rank;      // True if low-rank, false if dense
    
    union {
        struct {
            complex_t *U;  // Left singular vectors (for low-rank blocks)
            complex_t *V;  // Right singular vectors (for low-rank blocks)
            int rank;      // Actual rank (mathematical property)
        } low_rank;
        
        struct {
            complex_t *data;  // Dense matrix data (for dense blocks)
        } dense;
    } data;
} hmatrix_block_t;

/**
 * Cluster Tree (Mathematical Model)
 * 
 * Represents the hierarchical partitioning of the index set.
 * The tree structure defines which blocks are admissible for low-rank approximation.
 */
typedef struct {
    int *indices;          // Cluster indices
    int n_indices;         // Number of indices in cluster
    double *centroid;      // Cluster centroid coordinates
    double radius;         // Cluster radius
    int level;             // Tree level
    struct ClusterNode *left, *right;  // Child nodes
} ClusterNode;

typedef struct {
    ClusterNode *root;     // Root of cluster tree
    int max_depth;         // Maximum tree depth
    int n_clusters;        // Total number of clusters
} ClusterTree;

/**
 * Admissibility Condition for H-Matrix
 * 
 * Determines whether a matrix block is admissible for low-rank approximation.
 * 
 * A block (cluster_i, cluster_j) is admissible if:
 *   dist(centroid_i, centroid_j) > η * max(radius_i, radius_j)
 * 
 * where:
 *   dist = distance between cluster centroids
 *   radius = cluster radius
 *   η = admissibility parameter (typically 1.0-2.0)
 * 
 * @param cluster_i Cluster i
 * @param cluster_j Cluster j
 * @param eta Admissibility parameter
 * @return true if block is admissible for low-rank approximation
 */
bool hmatrix_is_admissible(
    const ClusterNode* cluster_i,
    const ClusterNode* cluster_j,
    double eta
);

/**
 * Compute H-Matrix approximation error (mathematical measure)
 * 
 * Error is typically measured as:
 *   ||M - M_H||_F / ||M||_F
 * 
 * where M_H is the H-matrix approximation and ||·||_F is the Frobenius norm.
 * 
 * @param original_matrix Original matrix M
 * @param n_rows Number of rows
 * @param n_cols Number of columns
 * @param hmatrix H-matrix approximation
 * @return Relative error (0.0 = perfect, 1.0 = 100% error)
 */
real_t hmatrix_compute_error(
    const complex_t* original_matrix,
    int n_rows, int n_cols,
    const void* hmatrix  // HMatrix* (forward declaration to avoid circular dependency)
);

#ifdef __cplusplus
}
#endif

#endif // HMATRIX_OPERATOR_MODEL_H
