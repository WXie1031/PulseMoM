/*****************************************************************************************
 * PulseEM - H-Matrix Compression for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_hmatrix.c
 * Description: Implementation of H-matrix hierarchical compression
 *****************************************************************************************/

#include "mom_hmatrix.h"
#include "mom_aca.h"
#include "../../operators/kernels/electromagnetic_kernels.h"
#include "../../operators/integration/integration_utils.h"
#include "../../common/core_common.h"  // For ONE_HALF constant
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Check admissibility condition for H-matrix blocks
bool hmatrix_is_admissible(
    int row_start, int row_end,
    int col_start, int col_end,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    double eta  // Admissibility parameter (typically 1.0-2.0)
) {
    // Compute bounding boxes for row and column clusters
    double row_min[3] = {1e30, 1e30, 1e30};
    double row_max[3] = {-1e30, -1e30, -1e30};
    double col_min[3] = {1e30, 1e30, 1e30};
    double col_max[3] = {-1e30, -1e30, -1e30};
    
    for (int i = row_start; i < row_end; i++) {
        if (elements[i].num_vertices >= 3) {
            for (int v = 0; v < 3; v++) {
                int vidx = elements[i].vertices[v];
                geom_point_t pos = vertices[vidx].position;
                if (pos.x < row_min[0]) row_min[0] = pos.x;
                if (pos.x > row_max[0]) row_max[0] = pos.x;
                if (pos.y < row_min[1]) row_min[1] = pos.y;
                if (pos.y > row_max[1]) row_max[1] = pos.y;
                if (pos.z < row_min[2]) row_min[2] = pos.z;
                if (pos.z > row_max[2]) row_max[2] = pos.z;
            }
        }
    }
    
    for (int j = col_start; j < col_end; j++) {
        if (elements[j].num_vertices >= 3) {
            for (int v = 0; v < 3; v++) {
                int vidx = elements[j].vertices[v];
                geom_point_t pos = vertices[vidx].position;
                if (pos.x < col_min[0]) col_min[0] = pos.x;
                if (pos.x > col_max[0]) col_max[0] = pos.x;
                if (pos.y < col_min[1]) col_min[1] = pos.y;
                if (pos.y > col_max[1]) col_max[1] = pos.y;
                if (pos.z < col_min[2]) col_min[2] = pos.z;
                if (pos.z > col_max[2]) col_max[2] = pos.z;
            }
        }
    }
    
    // Compute cluster centers and radii
    double row_center[3] = {
        (row_min[0] + row_max[0]) * 0.5,
        (row_min[1] + row_max[1]) * 0.5,
        (row_min[2] + row_max[2]) * 0.5
    };
    double col_center[3] = {
        (col_min[0] + col_max[0]) * 0.5,
        (col_min[1] + col_max[1]) * 0.5,
        (col_min[2] + col_max[2]) * 0.5
    };
    
    double row_radius = ONE_HALF * sqrt(
        (row_max[0] - row_min[0]) * (row_max[0] - row_min[0]) +
        (row_max[1] - row_min[1]) * (row_max[1] - row_min[1]) +
        (row_max[2] - row_min[2]) * (row_max[2] - row_min[2])
    );
    double col_radius = ONE_HALF * sqrt(
        (col_max[0] - col_min[0]) * (col_max[0] - col_min[0]) +
        (col_max[1] - col_min[1]) * (col_max[1] - col_min[1]) +
        (col_max[2] - col_min[2]) * (col_max[2] - col_min[2])
    );
    
    double dist = sqrt(
        (row_center[0] - col_center[0]) * (row_center[0] - col_center[0]) +
        (row_center[1] - col_center[1]) * (row_center[1] - col_center[1]) +
        (row_center[2] - col_center[2]) * (row_center[2] - col_center[2])
    );
    
    // Admissibility: dist >= eta * (row_radius + col_radius)
    return (dist >= eta * (row_radius + col_radius));
}

// H-matrix block compression
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
) {
    if (!block || !params) return -1;
    
    block->row_start = row_start;
    block->row_end = row_end;
    block->col_start = col_start;
    block->col_end = col_end;
    
    // Check admissibility
    bool is_admissible = hmatrix_is_admissible(row_start, row_end, col_start, col_end,
                                               elements, vertices, params->eta);
    
    if (is_admissible && params->use_aca) {
        // Use ACA for low-rank compression
        block->is_low_rank = true;
        aca_block_t aca_block = {0};
        aca_block.tolerance = params->tolerance;
        aca_block.max_rank = (row_end - row_start) < (col_end - col_start) 
                            ? (row_end - row_start) : (col_end - col_start);
        if (aca_block.max_rank > 200) aca_block.max_rank = 200;
        
        if (aca_compress_block(&aca_block, row_start, row_end, col_start, col_end,
                              elements, vertices, state, freq, form) == 0) {
            block->rank = aca_block.rank;
            block->U = aca_block.U;
            block->V = aca_block.V;
            // Transfer ownership - don't free aca_block
            aca_block.U = NULL;
            aca_block.V = NULL;
            return 0;
        }
    }
    
    // Fallback to dense storage
    block->is_low_rank = false;
    block->rank = 0;
    int n_rows = row_end - row_start;
    int n_cols = col_end - col_start;
    block->dense_data = (complex_t*)calloc(n_rows * n_cols, sizeof(complex_t));
    if (!block->dense_data) return -1;
    
    // Compute dense block (would be done during assembly)
    return 0;
}

// Free H-matrix block
void hmatrix_block_free(hmatrix_block_t* block) {
    if (!block) return;
    if (block->U) free(block->U);
    if (block->V) free(block->V);
    if (block->dense_data) free(block->dense_data);
    memset(block, 0, sizeof(hmatrix_block_t));
}

// Matrix-vector product for H-matrix
void hmatrix_matrix_vector_product(
    const hmatrix_block_t* RESTRICT_PTR blocks,
    int num_blocks,
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    int n
) {
    // Initialize output
    int i;
    #pragma omp parallel for if(num_blocks > 10) \
                             shared(y, n) \
                             schedule(static)
    for (i = 0; i < n; i++) {
        y[i] = complex_zero();
    }
    
    // Process each block
    for (int b = 0; b < num_blocks; b++) {
        const hmatrix_block_t* block = &blocks[b];
        if (!block) continue;
        
        if (block->is_low_rank && block->rank > 0) {
            // Low-rank block: y += U * (V^T * x)
            complex_t* vt_x = (complex_t*)calloc(block->rank, sizeof(complex_t));
            if (!vt_x) continue;
            
            for (int r = 0; r < block->rank; r++) {
                complex_t sum = complex_zero();
                for (int j = 0; j < (block->col_end - block->col_start); j++) {
                    int col_idx = block->col_start + j;
                    if (col_idx >= 0 && col_idx < n) {
                        // Use complex multiplication
                        complex_t prod;
                        prod.re = block->V[r * (block->col_end - block->col_start) + j].re * x[col_idx].re 
                                 - block->V[r * (block->col_end - block->col_start) + j].im * x[col_idx].im;
                        prod.im = block->V[r * (block->col_end - block->col_start) + j].re * x[col_idx].im 
                                 + block->V[r * (block->col_end - block->col_start) + j].im * x[col_idx].re;
                        sum.re += prod.re;
                        sum.im += prod.im;
                    }
                }
                vt_x[r] = sum;
            }
            
            for (int i = 0; i < (block->row_end - block->row_start); i++) {
                int row_idx = block->row_start + i;
                if (row_idx >= 0 && row_idx < n) {
                    complex_t sum = complex_zero();
                    for (int r = 0; r < block->rank; r++) {
                        complex_t prod;
                        prod.re = block->U[i * block->rank + r].re * vt_x[r].re 
                                 - block->U[i * block->rank + r].im * vt_x[r].im;
                        prod.im = block->U[i * block->rank + r].re * vt_x[r].im 
                                 + block->U[i * block->rank + r].im * vt_x[r].re;
                        sum.re += prod.re;
                        sum.im += prod.im;
                    }
                    y[row_idx].re += sum.re;
                    y[row_idx].im += sum.im;
                }
            }
            
            free(vt_x);
        } else if (block->dense_data) {
            // Dense block
            for (int i = 0; i < (block->row_end - block->row_start); i++) {
                int row_idx = block->row_start + i;
                if (row_idx >= 0 && row_idx < n) {
                    complex_t sum = complex_zero();
                    for (int j = 0; j < (block->col_end - block->col_start); j++) {
                        int col_idx = block->col_start + j;
                        if (col_idx >= 0 && col_idx < n) {
                            complex_t prod;
                            prod.re = block->dense_data[i * (block->col_end - block->col_start) + j].re * x[col_idx].re 
                                     - block->dense_data[i * (block->col_end - block->col_start) + j].im * x[col_idx].im;
                            prod.im = block->dense_data[i * (block->col_end - block->col_start) + j].re * x[col_idx].im 
                                     + block->dense_data[i * (block->col_end - block->col_start) + j].im * x[col_idx].re;
                            sum.re += prod.re;
                            sum.im += prod.im;
                        }
                    }
                    y[row_idx].re += sum.re;
                    y[row_idx].im += sum.im;
                }
            }
        }
    }
}
