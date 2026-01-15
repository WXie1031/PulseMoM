/*****************************************************************************************
 * PulseEM - Optimized Matrix-Vector Product for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_matvec.h
 * Description: Header for optimized matrix-vector products supporting compressed formats
 *****************************************************************************************/

#ifndef MOM_MATVEC_H
#define MOM_MATVEC_H

#include "../core/core_common.h"
#include "mom_aca.h"
#include "mom_hmatrix.h"
#include "mom_mlfmm.h"

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

// Algorithm types
typedef enum {
    MOM_ALGO_BASIC = 0,           // Basic direct solver
    MOM_ALGO_ACA = 1,             // Adaptive Cross Approximation
    MOM_ALGO_MLFMM = 2,           // Multilevel Fast Multipole Method
    MOM_ALGO_HMATRIX = 3,         // Hierarchical matrix
    MOM_ALGO_HODLR = 4,           // Hierarchically Off-Diagonal Low-Rank
    MOM_ALGO_SVD = 5              // SVD-based compression
} mom_algorithm_t;

// Optimized matrix-vector product for compressed matrices
// Supports dense, ACA, H-matrix, and MLFMM formats
void mom_matrix_vector_product(
    const complex_t* RESTRICT_PTR Z,      // Dense matrix (if algorithm is BASIC)
    const aca_block_t* RESTRICT_PTR aca_blocks,  // ACA blocks (if algorithm is ACA)
    int num_aca_blocks,
    const hmatrix_block_t* RESTRICT_PTR hmatrix_blocks,  // H-matrix blocks (if algorithm is HMATRIX)
    int num_hmatrix_blocks,
    const mlfmm_tree_t* RESTRICT_PTR mlfmm_tree,  // MLFMM tree (if algorithm is MLFMM)
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    int n,
    mom_algorithm_t algorithm,
    int num_threads
);

#ifdef __cplusplus
}
#endif

#endif // MOM_MATVEC_H
