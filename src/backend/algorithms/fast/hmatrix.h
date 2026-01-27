/********************************************************************************
 * H-Matrix Compression (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines H-matrix compression for fast algorithms.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 provides acceleration, must preserve operator semantics.
 ********************************************************************************/

#ifndef HMATRIX_H
#define HMATRIX_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../operators/assembler/matrix_assembler.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// H-Matrix Parameters
// ============================================================================

/**
 * H-Matrix Compression Parameters
 * 
 * L4 layer defines compression parameters, not physics
 */
typedef struct {
    int cluster_size;        // Minimum cluster size
    real_t tolerance;        // Compression tolerance
    int max_rank;           // Maximum rank for low-rank approximation
    int admissibility;      // Admissibility parameter
} hmatrix_params_t;

/**
 * H-Matrix Block
 */
typedef struct {
    int* row_indices;        // Row indices for this block
    int* col_indices;        // Column indices for this block
    int n_rows, n_cols;      // Block dimensions
    
    bool is_low_rank;        // True if low-rank, false if dense
    
    union {
        struct {
            complex_t* U;    // Left singular vectors
            complex_t* V;    // Right singular vectors
            int rank;        // Actual rank
        } low_rank;
        
        struct {
            complex_t* data; // Dense matrix data
        } dense;
    } data;
} hmatrix_block_t;

/**
 * H-Matrix Structure
 */
typedef struct {
    hmatrix_block_t* blocks; // Array of matrix blocks
    int n_blocks;           // Number of blocks
    int total_rows, total_cols;
    hmatrix_params_t params;
} hmatrix_t;

// ============================================================================
// H-Matrix Interface
// ============================================================================

/**
 * Create H-matrix from dense matrix
 * 
 * L4 layer compresses matrix, preserves operator semantics
 */
hmatrix_t* hmatrix_create(
    const complex_t* dense_matrix,
    int n_rows, int n_cols,
    const hmatrix_params_t* params
);

/**
 * Destroy H-matrix
 */
void hmatrix_destroy(hmatrix_t* hmatrix);

/**
 * H-matrix vector multiplication
 * 
 * L4 layer provides fast matvec, preserves operator semantics
 */
int hmatrix_vector_multiply(
    const hmatrix_t* hmatrix,
    const complex_t* input_vector,
    complex_t* output_vector
);

/**
 * Get compression statistics
 */
int hmatrix_get_statistics(
    const hmatrix_t* hmatrix,
    real_t* compression_ratio,
    size_t* memory_usage
);

#ifdef __cplusplus
}
#endif

#endif // HMATRIX_H
