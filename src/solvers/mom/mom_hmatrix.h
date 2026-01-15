/*****************************************************************************************
 * PulseEM - H-Matrix Compression for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_hmatrix.h
 * Description: Header for H-matrix hierarchical compression
 *****************************************************************************************/

#ifndef MOM_HMATRIX_H
#define MOM_HMATRIX_H

#include "../core/core_common.h"
#include "../core/core_mesh.h"
#include <stdbool.h>

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

// H-matrix block structure
typedef struct {
    int row_start, row_end;
    int col_start, col_end;
    bool is_low_rank;      // True if low-rank, false if dense
    int rank;              // Rank for low-rank blocks
    complex_t* U;          // Left factor (if low-rank)
    complex_t* V;          // Right factor (if low-rank)
    complex_t* dense_data; // Dense data (if dense block)
    double compression_error;
} hmatrix_block_t;

// H-matrix parameters
typedef struct {
    double tolerance;         // Compression tolerance
    int min_cluster_size;     // Minimum cluster size
    double eta;              // Admissibility parameter (typically 1.0-2.0)
    bool use_aca;            // Use ACA for low-rank blocks
} hmatrix_params_t;

// Check admissibility condition for H-matrix blocks
bool hmatrix_is_admissible(
    int row_start, int row_end,
    int col_start, int col_end,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    double eta
);

// H-matrix functions
int hmatrix_compress_block(
    hmatrix_block_t* block,
    int row_start, int row_end,
    int col_start, int col_end,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    const void* state,
    double freq,
    int form,
    const hmatrix_params_t* params
);

void hmatrix_block_free(hmatrix_block_t* block);

// Matrix-vector product for H-matrix
void hmatrix_matrix_vector_product(
    const hmatrix_block_t* RESTRICT_PTR blocks,
    int num_blocks,
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    int n
);

#ifdef __cplusplus
}
#endif

#endif // MOM_HMATRIX_H
