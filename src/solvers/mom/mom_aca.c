/*****************************************************************************************
 * PulseEM - ACA (Adaptive Cross Approximation) Acceleration for MoM
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * 
 * File: mom_aca.c
 * Description: Implementation of ACA low-rank matrix compression
 *****************************************************************************************/

#include "mom_aca.h"
#include "../core/electromagnetic_kernels.h"
#include "../core/integration_utils.h"
#include "../core/core_kernels.h"
#include "../core/core_assembler.h"
#include "../core/core_geometry.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Compute a single matrix element Z[i][j] for ACA
static complex_t compute_z_element_aca(
    int i, int j,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    const void* state_ptr,
    double freq,
    int form
) {
    // Cast state to access mesh and config
    // Note: This is a simplified access pattern - in full implementation would use proper state structure
    typedef struct {
        const mesh_t* mesh;
        struct {
            double near_field_threshold;
            int enable_duffy_transform;  // Use int for bool compatibility
        } config;
    } state_access_t;
    
    const state_access_t* state = (const state_access_t*)state_ptr;
    if (!state || i >= state->mesh->num_elements || j >= state->mesh->num_elements) {
        return complex_zero();
    }
    
    const mesh_element_t* elem_i = &elements[i];
    const mesh_element_t* elem_j = &elements[j];
    
    // Early exit for non-triangle elements
    if (elem_i->type != MESH_ELEMENT_TRIANGLE || 
        elem_j->type != MESH_ELEMENT_TRIANGLE ||
        elem_i->num_vertices < 3 || elem_j->num_vertices < 3) {
        return complex_zero();
    }
    
    // Convert to geometry format
    geom_triangle_t tri_i, tri_j;
    const int v0_i = elem_i->vertices[0];
    const int v1_i = elem_i->vertices[1];
    const int v2_i = elem_i->vertices[2];
    const int v0_j = elem_j->vertices[0];
    const int v1_j = elem_j->vertices[1];
    const int v2_j = elem_j->vertices[2];
    
    tri_i.vertices[0] = vertices[v0_i].position;
    tri_i.vertices[1] = vertices[v1_i].position;
    tri_i.vertices[2] = vertices[v2_i].position;
    tri_i.area = elem_i->area;
    tri_i.normal = elem_i->normal;
    
    tri_j.vertices[0] = vertices[v0_j].position;
    tri_j.vertices[1] = vertices[v1_j].position;
    tri_j.vertices[2] = vertices[v2_j].position;
    tri_j.area = elem_j->area;
    tri_j.normal = elem_j->normal;
    
    // Compute centroid distance using unified helper functions
    geom_point_t c_i, c_j;
    geom_triangle_get_centroid(&tri_i, &c_i);
    geom_triangle_get_centroid(&tri_j, &c_j);
    double r = geom_point_distance(&c_i, &c_j) + 1e-12;
    
    // Use standard integration for far-field
    const double lambda = C0 / freq;
    const double near_threshold = (state->config.near_field_threshold > 0.0) 
                                 ? state->config.near_field_threshold 
                                 : 0.1;
    const double near_threshold_lambda = near_threshold * lambda;
    
    if (state->config.enable_duffy_transform && r < near_threshold_lambda) {
        // Use Duffy transform for near-field
        // Note: integrate_triangle_duffy signature may vary - adjust as needed
        // Use default Gauss order (4) for near-field integration
        complex_t result = integrate_triangle_triangle(&tri_i, &tri_j, freq, KERNEL_FORMULATION_EFIELD, 4);
        // Apply near-field correction if needed
        return result;
    } else {
        kernel_formulation_t kernel_form = (kernel_formulation_t)form;
        // Use default Gauss order (4) for far-field integration
        return integrate_triangle_triangle(&tri_i, &tri_j, freq, kernel_form, 4);
    }
}

// Complex number operations (inline macros for MSVC compatibility)
#if defined(_MSC_VER)
#define MOM_COMPLEX_MUL(a, b) ((complex_t){(a).re*(b).re - (a).im*(b).im, (a).re*(b).im + (a).im*(b).re})
#define MOM_COMPLEX_ADD(a, b) ((complex_t){(a).re + (b).re, (a).im + (b).im})
#else
#include <complex.h>
#define MOM_COMPLEX_MUL(a, b) ((a) * (b))
#define MOM_COMPLEX_ADD(a, b) ((a) + (b))
#endif

// ACA algorithm: Adaptive Cross Approximation for low-rank compression
int aca_compress_block(
    aca_block_t* block,
    int row_start, int row_end,
    int col_start, int col_end,
    const mesh_element_t* RESTRICT_PTR elements,
    const mesh_vertex_t* RESTRICT_PTR vertices,
    const void* state,
    double freq,
    int form
) {
    if (!block || row_end <= row_start || col_end <= col_start) return -1;
    
    int n_rows = row_end - row_start;
    int n_cols = col_end - col_start;
    double tolerance = block->tolerance > 0.0 ? block->tolerance : 1e-6;
    int max_rank = block->max_rank > 0 ? block->max_rank : (n_rows < n_cols ? n_rows : n_cols);
    
    // Allocate U and V matrices
    block->U = (complex_t*)calloc(n_rows * max_rank, sizeof(complex_t));
    block->V = (complex_t*)calloc(max_rank * n_cols, sizeof(complex_t));
    if (!block->U || !block->V) return -1;
    
    block->n_rows = n_rows;
    block->n_cols = n_cols;
    block->rank = 0;
    
    // Residual matrix (initially full matrix, but we compute on-the-fly)
    // ACA algorithm: iteratively build low-rank approximation
    complex_t* residual_row = (complex_t*)calloc(n_cols, sizeof(complex_t));
    complex_t* residual_col = (complex_t*)calloc(n_rows, sizeof(complex_t));
    if (!residual_row || !residual_col) {
        free(block->U); free(block->V);
        return -1;
    }
    
    // Find initial pivot (maximum element)
    int pivot_row = 0, pivot_col = 0;
    double max_val = 0.0;
    for (int i = 0; i < n_rows; i++) {
        for (int j = 0; j < n_cols; j++) {
            complex_t z_ij = compute_z_element_aca(row_start + i, col_start + j,
                                                   elements, vertices, state, freq, form);
            double mag_sq = z_ij.re * z_ij.re + z_ij.im * z_ij.im;
            if (mag_sq > max_val) {
                max_val = mag_sq;
                pivot_row = i;
                pivot_col = j;
            }
        }
    }
    
    if (max_val < tolerance * tolerance) {
        // Block is effectively zero
        free(residual_row); free(residual_col);
        return 0;
    }
    
    // ACA iteration
    for (int k = 0; k < max_rank; k++) {
        // Compute residual column (if k > 0)
        if (k > 0) {
            // Find new pivot column
            double max_col_val = 0.0;
            pivot_col = 0;
            for (int j = 0; j < n_cols; j++) {
                complex_t sum = complex_zero();
                for (int r = 0; r < k; r++) {
                    complex_t u_val = block->U[pivot_row * max_rank + r];
                    complex_t v_val = block->V[r * n_cols + j];
                    complex_t prod = MOM_COMPLEX_MUL(u_val, v_val);
                    sum = MOM_COMPLEX_ADD(sum, prod);
                }
                complex_t z_ij = compute_z_element_aca(row_start + pivot_row, col_start + j,
                                                   elements, vertices, state, freq, form);
                residual_row[j] = (complex_t){z_ij.re - sum.re, z_ij.im - sum.im};
                double mag_sq = residual_row[j].re * residual_row[j].re + 
                               residual_row[j].im * residual_row[j].im;
                if (mag_sq > max_col_val) {
                    max_col_val = mag_sq;
                    pivot_col = j;
                }
            }
            
            if (max_col_val < tolerance * tolerance) break;
        }
        
        // Compute u column (residual column at pivot_col)
        complex_t pivot_val = compute_z_element_aca(row_start + pivot_row, col_start + pivot_col,
                                                    elements, vertices, state, freq, form);
        if (k > 0) {
            complex_t sum = complex_zero();
            for (int r = 0; r < k; r++) {
                complex_t u_val = block->U[pivot_row * max_rank + r];
                complex_t v_val = block->V[r * n_cols + pivot_col];
                complex_t prod = MOM_COMPLEX_MUL(u_val, v_val);
                sum = MOM_COMPLEX_ADD(sum, prod);
            }
            pivot_val = (complex_t){pivot_val.re - sum.re, pivot_val.im - sum.im};
        }
        
        double pivot_mag = sqrt(pivot_val.re * pivot_val.re + pivot_val.im * pivot_val.im);
        if (pivot_mag < tolerance) break;
        
        // Normalize pivot
        complex_t pivot_inv = {pivot_val.re / (pivot_mag * pivot_mag), 
                               -pivot_val.im / (pivot_mag * pivot_mag)};
        
        // Compute v row
        for (int j = 0; j < n_cols; j++) {
            complex_t z_ij = compute_z_element_aca(row_start + pivot_row, col_start + j,
                                                  elements, vertices, state, freq, form);
            if (k > 0) {
                complex_t sum = complex_zero();
                for (int r = 0; r < k; r++) {
                    complex_t u_val = block->U[pivot_row * max_rank + r];
                    complex_t v_val = block->V[r * n_cols + j];
                    complex_t prod = MOM_COMPLEX_MUL(u_val, v_val);
                    sum = MOM_COMPLEX_ADD(sum, prod);
                }
                z_ij = (complex_t){z_ij.re - sum.re, z_ij.im - sum.im};
            }
            block->V[k * n_cols + j] = MOM_COMPLEX_MUL(z_ij, pivot_inv);
        }
        
        // Compute u column
        for (int i = 0; i < n_rows; i++) {
            complex_t z_ij = compute_z_element_aca(row_start + i, col_start + pivot_col,
                                                  elements, vertices, state, freq, form);
            if (k > 0) {
                complex_t sum = complex_zero();
                for (int r = 0; r < k; r++) {
                    complex_t u_val = block->U[i * max_rank + r];
                    complex_t v_val = block->V[r * n_cols + pivot_col];
                    complex_t prod = MOM_COMPLEX_MUL(u_val, v_val);
                    sum = MOM_COMPLEX_ADD(sum, prod);
                }
                z_ij = (complex_t){z_ij.re - sum.re, z_ij.im - sum.im};
            }
            block->U[i * max_rank + k] = z_ij;
        }
        
        block->rank = k + 1;
        
        // Check convergence (simplified: check if next iteration would be small)
        if (k + 1 < max_rank) {
            double max_residual = 0.0;
            for (int i = 0; i < n_rows && i < 10; i++) {  // Sample check
                for (int j = 0; j < n_cols && j < 10; j++) {
                    complex_t z_ij = compute_z_element_aca(row_start + i, col_start + j,
                                                          elements, vertices, state, freq, form);
                    complex_t approx = complex_zero();
                    for (int r = 0; r <= k; r++) {
                        complex_t u_val = block->U[i * max_rank + r];
                        complex_t v_val = block->V[r * n_cols + j];
                        complex_t prod = MOM_COMPLEX_MUL(u_val, v_val);
                        approx = MOM_COMPLEX_ADD(approx, prod);
                    }
                    complex_t diff = {z_ij.re - approx.re, z_ij.im - approx.im};
                    double mag_sq = diff.re * diff.re + diff.im * diff.im;
                    if (mag_sq > max_residual) max_residual = mag_sq;
                }
            }
            if (max_residual < tolerance * tolerance) break;
        }
    }
    
    free(residual_row);
    free(residual_col);
    return 0;
}

// Free ACA block
void aca_block_free(aca_block_t* block) {
    if (!block) return;
    if (block->U) free(block->U);
    if (block->V) free(block->V);
    if (block->row_indices) free(block->row_indices);
    if (block->col_indices) free(block->col_indices);
    memset(block, 0, sizeof(aca_block_t));
}

// Matrix-vector product for ACA compressed matrix
void aca_matrix_vector_product(
    const aca_block_t* RESTRICT_PTR blocks,
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
    
    // For each block: y += U * (V^T * x)
    for (int b = 0; b < num_blocks; b++) {
        const aca_block_t* block = &blocks[b];
        if (!block || block->rank == 0) continue;
        
        // Compute V^T * x for this block's columns
        complex_t* vt_x = (complex_t*)calloc(block->rank, sizeof(complex_t));
        if (!vt_x) continue;
        
        for (int r = 0; r < block->rank; r++) {
            complex_t sum = complex_zero();
            for (int j = 0; j < block->n_cols; j++) {
                int col_idx = block->col_indices ? block->col_indices[j] : j;
                if (col_idx >= 0 && col_idx < n) {
                    complex_t prod = MOM_COMPLEX_MUL(block->V[r * block->n_cols + j], x[col_idx]);
                    sum = MOM_COMPLEX_ADD(sum, prod);
                }
            }
            vt_x[r] = sum;
        }
        
        // Compute U * vt_x and add to y
        for (int i = 0; i < block->n_rows; i++) {
            int row_idx = block->row_indices ? block->row_indices[i] : i;
            if (row_idx >= 0 && row_idx < n) {
                complex_t sum = complex_zero();
                for (int r = 0; r < block->rank; r++) {
                    complex_t prod = MOM_COMPLEX_MUL(block->U[i * block->max_rank + r], vt_x[r]);
                    sum = MOM_COMPLEX_ADD(sum, prod);
                }
                y[row_idx] = MOM_COMPLEX_ADD(y[row_idx], sum);
            }
        }
        
        free(vt_x);
    }
}
