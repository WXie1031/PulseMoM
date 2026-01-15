/*****************************************************************************************
 * PulseEM - Optimized Matrix-Vector Product for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_matvec.c
 * Description: Implementation of optimized matrix-vector products
 *****************************************************************************************/

#include "mom_matvec.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Complex number operations (inline macros for MSVC compatibility)
#if defined(_MSC_VER)
#define MOM_COMPLEX_MUL(a, b) ((complex_t){(a).re*(b).re - (a).im*(b).im, (a).re*(b).im + (a).im*(b).re})
#define MOM_COMPLEX_ADD(a, b) ((complex_t){(a).re + (b).re, (a).im + (b).im})
#else
#include <complex.h>
#define MOM_COMPLEX_MUL(a, b) ((a) * (b))
#define MOM_COMPLEX_ADD(a, b) ((a) + (b))
#endif

// Optimized matrix-vector product for compressed matrices
void mom_matrix_vector_product(
    const complex_t* RESTRICT_PTR Z,
    const aca_block_t* RESTRICT_PTR aca_blocks,
    int num_aca_blocks,
    const hmatrix_block_t* RESTRICT_PTR hmatrix_blocks,
    int num_hmatrix_blocks,
    const mlfmm_tree_t* RESTRICT_PTR mlfmm_tree,
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    int n,
    mom_algorithm_t algorithm,
    int num_threads
) {
    // Initialize output vector
    int i;
    #pragma omp parallel for if(num_threads > 1) \
                             shared(y, n) \
                             schedule(static)
    for (i = 0; i < n; i++) {
        y[i] = complex_zero();
    }
    
    switch (algorithm) {
        case MOM_ALGO_BASIC:
        case MOM_ALGO_HMATRIX:
            // Dense matrix-vector product: y = Z * x
            // Optimized: cache-friendly access pattern, use restrict hints
            if (algorithm == MOM_ALGO_HMATRIX && hmatrix_blocks && num_hmatrix_blocks > 0) {
                hmatrix_matrix_vector_product(hmatrix_blocks, num_hmatrix_blocks, x, y, n);
            } else if (Z) {
                #pragma omp parallel for if(num_threads > 1) \
                                         shared(Z, x, y, n) \
                                         schedule(static)
                for (i = 0; i < n; i++) {
                    complex_t sum = complex_zero();
                    const complex_t* RESTRICT_PTR row_i = &Z[i * n];
                    for (int j = 0; j < n; j++) {
                        // sum += Z[i][j] * x[j]
                        complex_t prod = MOM_COMPLEX_MUL(row_i[j], x[j]);
                        sum = MOM_COMPLEX_ADD(sum, prod);
                    }
                    y[i] = sum;
                }
            }
            break;
            
        case MOM_ALGO_ACA:
            // ACA compressed matrix-vector product
            // For ACA: Z ≈ U * V^T, so y = U * (V^T * x)
            if (aca_blocks && num_aca_blocks > 0) {
                aca_matrix_vector_product(aca_blocks, num_aca_blocks, x, y, n);
            } else if (Z) {
                // Fallback to dense
                #pragma omp parallel for if(num_threads > 1) \
                                         shared(Z, x, y, n) \
                                         schedule(static)
                for (i = 0; i < n; i++) {
                    complex_t sum = complex_zero();
                    const complex_t* RESTRICT_PTR row_i = &Z[i * n];
                    for (int j = 0; j < n; j++) {
                        complex_t prod = MOM_COMPLEX_MUL(row_i[j], x[j]);
                        sum = MOM_COMPLEX_ADD(sum, prod);
                    }
                    y[i] = sum;
                }
            }
            break;
            
        case MOM_ALGO_MLFMM:
            // MLFMM matrix-vector product
            // Near-field: direct computation from stored matrix
            // Far-field: computed via multipole expansions
            if (mlfmm_tree && Z) {
                // For now, use near-field only (stored in Z)
                // Full MLFMM would compute far-field via multipole expansions
                #pragma omp parallel for if(num_threads > 1) \
                                         shared(Z, x, y, n) \
                                         schedule(static)
                for (i = 0; i < n; i++) {
                    complex_t sum = complex_zero();
                    const complex_t* RESTRICT_PTR row_i = &Z[i * n];
                    for (int j = 0; j < n; j++) {
                        // Only near-field interactions are stored
                        // Far-field would be computed via MLFMM
                        complex_t prod = MOM_COMPLEX_MUL(row_i[j], x[j]);
                        sum = MOM_COMPLEX_ADD(sum, prod);
                    }
                    y[i] = sum;
                }
            }
            break;
            
        default:
            // Fallback to dense
            if (Z) {
                #pragma omp parallel for if(num_threads > 1) \
                                         shared(Z, x, y, n) \
                                         schedule(static)
                for (i = 0; i < n; i++) {
                    complex_t sum = complex_zero();
                    const complex_t* RESTRICT_PTR row_i = &Z[i * n];
                    for (int j = 0; j < n; j++) {
                        complex_t prod = MOM_COMPLEX_MUL(row_i[j], x[j]);
                        sum = MOM_COMPLEX_ADD(sum, prod);
                    }
                    y[i] = sum;
                }
            }
            break;
    }
}
