/*****************************************************************************************
 * PulseEM - ACA (Adaptive Cross Approximation) Acceleration for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_aca.h
 * Description: Header for ACA low-rank matrix compression
 *****************************************************************************************/

#ifndef MOM_ACA_H
#define MOM_ACA_H

#include "../core/core_common.h"
#include "../core/core_mesh.h"

// Compiler optimization hints
#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT_PTR __restrict__
#elif defined(_MSC_VER)
#define RESTRICT_PTR __restrict
#else
#define RESTRICT_PTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ACA block structure
typedef struct {
    complex_t* U;           // Left factor matrix (n x rank)
    complex_t* V;           // Right factor matrix (rank x m)
    int rank;               // Current rank
    int max_rank;           // Maximum allowed rank
    double tolerance;       // Compression tolerance
    int* row_indices;       // Row indices for this block
    int* col_indices;       // Column indices for this block
    int n_rows, n_cols;     // Block dimensions
} aca_block_t;

// ACA parameters
typedef struct {
    double tolerance;       // Compression tolerance
    int max_rank;          // Maximum rank
    int min_block_size;    // Minimum block size for compression
    bool adaptive_rank;    // Use adaptive rank selection
} aca_params_t;

// ACA functions
int aca_compress_block(
    aca_block_t* block,
    int row_start, int row_end,
    int col_start, int col_end,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    const void* state,
    double freq,
    int form
);

void aca_block_free(aca_block_t* block);

// Matrix-vector product for ACA compressed matrix
void aca_matrix_vector_product(
    const aca_block_t* RESTRICT_PTR blocks,
    int num_blocks,
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    int n
);

#ifdef __cplusplus
}
#endif

#endif // MOM_ACA_H
