/********************************************************************************
 * H-Matrix Compression Implementation (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements H-matrix compression for fast algorithms.
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 provides acceleration, must preserve operator semantics.
 ********************************************************************************/

#include "hmatrix.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../../backend/math/blas_interface.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

hmatrix_t* hmatrix_create(
    const complex_t* dense_matrix,
    int n_rows, int n_cols,
    const hmatrix_params_t* params) {
    
    if (!dense_matrix || n_rows <= 0 || n_cols <= 0 || !params) {
        return NULL;
    }
    
    hmatrix_t* hmatrix = (hmatrix_t*)calloc(1, sizeof(hmatrix_t));
    if (!hmatrix) return NULL;
    
    hmatrix->total_rows = n_rows;
    hmatrix->total_cols = n_cols;
    memcpy(&hmatrix->params, params, sizeof(hmatrix_params_t));
    
    // Simplified H-matrix construction
    // Full implementation would:
    // 1. Build cluster tree
    // 2. Determine admissible blocks
    // 3. Compress admissible blocks (low-rank)
    // 4. Keep inadmissible blocks dense
    
    // For now, create a single block (simplified)
    hmatrix->n_blocks = 1;
    hmatrix->blocks = (hmatrix_block_t*)calloc(1, sizeof(hmatrix_block_t));
    if (!hmatrix->blocks) {
        free(hmatrix);
        return NULL;
    }
    
    hmatrix_block_t* block = &hmatrix->blocks[0];
    block->n_rows = n_rows;
    block->n_cols = n_cols;
    block->is_low_rank = false;  // Start as dense
    
    // Allocate row and column indices
    block->row_indices = (int*)malloc(n_rows * sizeof(int));
    block->col_indices = (int*)malloc(n_cols * sizeof(int));
    if (!block->row_indices || !block->col_indices) {
        if (block->row_indices) free(block->row_indices);
        if (block->col_indices) free(block->col_indices);
        free(hmatrix->blocks);
        free(hmatrix);
        return NULL;
    }
    
    for (int i = 0; i < n_rows; i++) {
        block->row_indices[i] = i;
    }
    for (int j = 0; j < n_cols; j++) {
        block->col_indices[j] = j;
    }
    
    // Copy dense matrix data
    block->data.dense.data = (complex_t*)malloc(n_rows * n_cols * sizeof(complex_t));
    if (!block->data.dense.data) {
        free(block->row_indices);
        free(block->col_indices);
        free(hmatrix->blocks);
        free(hmatrix);
        return NULL;
    }
    
    memcpy(block->data.dense.data, dense_matrix, n_rows * n_cols * sizeof(complex_t));
    
    return hmatrix;
}

void hmatrix_destroy(hmatrix_t* hmatrix) {
    if (!hmatrix) return;
    
    if (hmatrix->blocks) {
        for (int i = 0; i < hmatrix->n_blocks; i++) {
            hmatrix_block_t* block = &hmatrix->blocks[i];
            
            if (block->row_indices) free(block->row_indices);
            if (block->col_indices) free(block->col_indices);
            
            if (block->is_low_rank) {
                if (block->data.low_rank.U) free(block->data.low_rank.U);
                if (block->data.low_rank.V) free(block->data.low_rank.V);
            } else {
                if (block->data.dense.data) free(block->data.dense.data);
            }
        }
        free(hmatrix->blocks);
    }
    
    free(hmatrix);
}

int hmatrix_vector_multiply(
    const hmatrix_t* hmatrix,
    const complex_t* input_vector,
    complex_t* output_vector) {
    
    if (!hmatrix || !input_vector || !output_vector) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Initialize output
    for (int i = 0; i < hmatrix->total_rows; i++) {
        #if defined(_MSC_VER)
        output_vector[i].re = 0.0;
        output_vector[i].im = 0.0;
        #else
        output_vector[i] = 0.0 + 0.0 * I;
        #endif
    }
    
    // L4 layer provides fast matvec, preserves operator semantics
    // Process each block
    for (int b = 0; b < hmatrix->n_blocks; b++) {
        const hmatrix_block_t* block = &hmatrix->blocks[b];
        
        if (block->is_low_rank) {
            // Low-rank block: y += U * (V^T * x)
            // Extract input subvector
            complex_t* x_sub = (complex_t*)malloc(block->n_cols * sizeof(complex_t));
            if (!x_sub) continue;
            
            for (int j = 0; j < block->n_cols; j++) {
                x_sub[j] = input_vector[block->col_indices[j]];
            }
            
            // Compute V^T * x
            complex_t* temp = (complex_t*)calloc(block->data.low_rank.rank, sizeof(complex_t));
            if (!temp) {
                free(x_sub);
                continue;
            }
            
            for (int k = 0; k < block->data.low_rank.rank; k++) {
                for (int j = 0; j < block->n_cols; j++) {
                    #if defined(_MSC_VER)
                    complex_t prod;
                    prod.re = block->data.low_rank.V[k * block->n_cols + j].re * x_sub[j].re -
                              block->data.low_rank.V[k * block->n_cols + j].im * x_sub[j].im;
                    prod.im = block->data.low_rank.V[k * block->n_cols + j].re * x_sub[j].im +
                              block->data.low_rank.V[k * block->n_cols + j].im * x_sub[j].re;
                    temp[k].re += prod.re;
                    temp[k].im += prod.im;
                    #else
                    temp[k] += block->data.low_rank.V[k * block->n_cols + j] * x_sub[j];
                    #endif
                }
            }
            
            // Compute U * temp
            for (int i = 0; i < block->n_rows; i++) {
                int row_idx = block->row_indices[i];
                for (int k = 0; k < block->data.low_rank.rank; k++) {
                    #if defined(_MSC_VER)
                    complex_t prod;
                    prod.re = block->data.low_rank.U[i * block->data.low_rank.rank + k].re * temp[k].re -
                              block->data.low_rank.U[i * block->data.low_rank.rank + k].im * temp[k].im;
                    prod.im = block->data.low_rank.U[i * block->data.low_rank.rank + k].re * temp[k].im +
                              block->data.low_rank.U[i * block->data.low_rank.rank + k].im * temp[k].re;
                    output_vector[row_idx].re += prod.re;
                    output_vector[row_idx].im += prod.im;
                    #else
                    output_vector[row_idx] += block->data.low_rank.U[i * block->data.low_rank.rank + k] * temp[k];
                    #endif
                }
            }
            
            free(x_sub);
            free(temp);
        } else {
            // Dense block: y += D * x
            for (int i = 0; i < block->n_rows; i++) {
                int row_idx = block->row_indices[i];
                for (int j = 0; j < block->n_cols; j++) {
                    int col_idx = block->col_indices[j];
                    int block_idx = i * block->n_cols + j;
                    
                    #if defined(_MSC_VER)
                    complex_t prod;
                    prod.re = block->data.dense.data[block_idx].re * input_vector[col_idx].re -
                              block->data.dense.data[block_idx].im * input_vector[col_idx].im;
                    prod.im = block->data.dense.data[block_idx].re * input_vector[col_idx].im +
                              block->data.dense.data[block_idx].im * input_vector[col_idx].re;
                    output_vector[row_idx].re += prod.re;
                    output_vector[row_idx].im += prod.im;
                    #else
                    output_vector[row_idx] += block->data.dense.data[block_idx] * input_vector[col_idx];
                    #endif
                }
            }
        }
    }
    
    return STATUS_SUCCESS;
}
