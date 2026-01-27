/********************************************************************************
 * Matrix-Vector Product Operator Implementation (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements matrix-vector product operators.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines operators, numerical implementation is in L4.
 ********************************************************************************/

#include "matvec_operator.h"
#include "../../common/types.h"
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

int matvec_operator_apply(
    const operator_matrix_t* A,
    const operator_vector_t* x,
    operator_vector_t* y) {
    
    if (!A || !x || !y) return STATUS_ERROR_INVALID_INPUT;
    if (A->num_rows != y->size || A->num_cols != x->size) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer defines the operator: y = A * x
    // Numerical implementation details are in L4
    
    if (A->type == MATRIX_TYPE_DENSE && A->data.dense) {
        // Dense matrix-vector product
        // Initialize output
        for (int i = 0; i < y->size; i++) {
            #if defined(_MSC_VER)
            y->data[i].re = 0.0;
            y->data[i].im = 0.0;
            #else
            y->data[i] = 0.0 + 0.0 * I;
            #endif
        }
        
        // Compute y = A * x
        int i;
        #ifdef _OPENMP
        #pragma omp parallel for
        #endif
        for (i = 0; i < A->num_rows; i++) {
            complex_t sum;
            #if defined(_MSC_VER)
            sum.re = 0.0;
            sum.im = 0.0;
            #else
            sum = 0.0 + 0.0 * I;
            #endif
            
            for (int j = 0; j < A->num_cols; j++) {
                int idx = i * A->num_cols + j;
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = A->data.dense[idx].re * x->data[j].re - A->data.dense[idx].im * x->data[j].im;
                prod.im = A->data.dense[idx].re * x->data[j].im + A->data.dense[idx].im * x->data[j].re;
                sum.re += prod.re;
                sum.im += prod.im;
                #else
                sum += A->data.dense[idx] * x->data[j];
                #endif
            }
            
            y->data[i] = sum;
        }
        
        return STATUS_SUCCESS;
    }
    
    // Sparse matrix support (CSR format)
    if (A->type == MATRIX_TYPE_SPARSE && A->data.sparse) {
        // Sparse matrix structure (CSR format)
        // This is a simplified interface - full implementation in L4
        typedef struct {
            int num_rows;
            int num_cols;
            int nnz;              // Number of non-zeros
            int* row_ptr;        // Row pointer array (size: num_rows + 1)
            int* col_idx;         // Column indices (size: nnz)
            complex_t* values;    // Non-zero values (size: nnz)
        } sparse_matrix_csr_t;
        
        sparse_matrix_csr_t* sparse = (sparse_matrix_csr_t*)A->data.sparse;
        
        // Initialize output
        for (int i = 0; i < y->size; i++) {
            #if defined(_MSC_VER)
            y->data[i].re = 0.0;
            y->data[i].im = 0.0;
            #else
            y->data[i] = 0.0 + 0.0 * I;
            #endif
        }
        
        // Compute y = A * x (CSR format)
        int i;
        #ifdef _OPENMP
        #pragma omp parallel for
        #endif
        for (i = 0; i < sparse->num_rows; i++) {
            complex_t sum;
            #if defined(_MSC_VER)
            sum.re = 0.0;
            sum.im = 0.0;
            #else
            sum = 0.0 + 0.0 * I;
            #endif
            
            // Process row i: from row_ptr[i] to row_ptr[i+1]
            for (int k = sparse->row_ptr[i]; k < sparse->row_ptr[i + 1]; k++) {
                int j = sparse->col_idx[k];
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = sparse->values[k].re * x->data[j].re - sparse->values[k].im * x->data[j].im;
                prod.im = sparse->values[k].re * x->data[j].im + sparse->values[k].im * x->data[j].re;
                sum.re += prod.re;
                sum.im += prod.im;
                #else
                sum += sparse->values[k] * x->data[j];
                #endif
            }
            
            y->data[i] = sum;
        }
        
        return STATUS_SUCCESS;
    }
    
    // Compressed matrices (ACA, H-matrix, etc.)
    // L3 layer defines interface, L4 backend implements numerical methods
    if (A->type == MATRIX_TYPE_COMPRESSED && A->data.compressed) {
        // Compressed matrix interface
        // The actual compressed matrix structure is defined in L4 backend
        // L3 layer only provides the operator interface
        
        // L4 backend should implement:
        // - ACA (Adaptive Cross Approximation) matrix-vector product
        // - H-matrix matrix-vector product
        // - Fast multipole method (FMM) matrix-vector product
        // - Hierarchical matrix operations
        
        // For now, compressed matrices require L4 backend implementation
        // This is intentional: L3 defines operators, L4 implements numerical methods
        return STATUS_ERROR_NOT_IMPLEMENTED;
    }
    
    return STATUS_ERROR_NOT_IMPLEMENTED;
}

int matvec_operator_apply_transpose(
    const operator_matrix_t* A,
    const operator_vector_t* x,
    operator_vector_t* y) {
    
    if (!A || !x || !y) return STATUS_ERROR_INVALID_INPUT;
    if (A->num_cols != y->size || A->num_rows != x->size) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer defines the operator: y = A^T * x
    if (A->type == MATRIX_TYPE_DENSE && A->data.dense) {
        // Initialize output
        for (int i = 0; i < y->size; i++) {
            #if defined(_MSC_VER)
            y->data[i].re = 0.0;
            y->data[i].im = 0.0;
            #else
            y->data[i] = 0.0 + 0.0 * I;
            #endif
        }
        
        // Compute y = A^T * x
        int j;
        #ifdef _OPENMP
        #pragma omp parallel for
        #endif
        for (j = 0; j < A->num_cols; j++) {
            complex_t sum;
            #if defined(_MSC_VER)
            sum.re = 0.0;
            sum.im = 0.0;
            #else
            sum = 0.0 + 0.0 * I;
            #endif
            
            for (int i = 0; i < A->num_rows; i++) {
                int idx = i * A->num_cols + j;
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = A->data.dense[idx].re * x->data[i].re - A->data.dense[idx].im * x->data[i].im;
                prod.im = A->data.dense[idx].re * x->data[i].im + A->data.dense[idx].im * x->data[i].re;
                sum.re += prod.re;
                sum.im += prod.im;
                #else
                sum += A->data.dense[idx] * x->data[i];
                #endif
            }
            
            y->data[j] = sum;
        }
        
        return STATUS_SUCCESS;
    }
    
    // Sparse matrix transpose support (CSR format)
    if (A->type == MATRIX_TYPE_SPARSE && A->data.sparse) {
        typedef struct {
            int num_rows;
            int num_cols;
            int nnz;
            int* row_ptr;
            int* col_idx;
            complex_t* values;
        } sparse_matrix_csr_t;
        
        sparse_matrix_csr_t* sparse = (sparse_matrix_csr_t*)A->data.sparse;
        
        // Initialize output
        for (int i = 0; i < y->size; i++) {
            #if defined(_MSC_VER)
            y->data[i].re = 0.0;
            y->data[i].im = 0.0;
            #else
            y->data[i] = 0.0 + 0.0 * I;
            #endif
        }
        
        // Compute y = A^T * x (CSR format)
        // For transpose, iterate over all non-zeros
        for (int i = 0; i < sparse->num_rows; i++) {
            for (int k = sparse->row_ptr[i]; k < sparse->row_ptr[i + 1]; k++) {
                int j = sparse->col_idx[k];
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = sparse->values[k].re * x->data[i].re - sparse->values[k].im * x->data[i].im;
                prod.im = sparse->values[k].re * x->data[i].im + sparse->values[k].im * x->data[i].re;
                y->data[j].re += prod.re;
                y->data[j].im += prod.im;
                #else
                y->data[j] += sparse->values[k] * x->data[i];
                #endif
            }
        }
        
        return STATUS_SUCCESS;
    }
    
    // Compressed matrices transpose
    if (A->type == MATRIX_TYPE_COMPRESSED && A->data.compressed) {
        // L4 backend implements compressed matrix transpose operations
        // For compressed formats (ACA, H-matrix), transpose requires
        // format-specific handling in L4 numerical backend
        return STATUS_ERROR_NOT_IMPLEMENTED;
    }
    
    return STATUS_ERROR_NOT_IMPLEMENTED;
}

int matvec_operator_apply_hermitian(
    const operator_matrix_t* A,
    const operator_vector_t* x,
    operator_vector_t* y) {
    
    if (!A || !x || !y) return STATUS_ERROR_INVALID_INPUT;
    if (A->num_cols != y->size || A->num_rows != x->size) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer defines the operator: y = A^H * x
    // A^H = conjugate(A^T)
    if (A->type == MATRIX_TYPE_DENSE && A->data.dense) {
        // Initialize output
        for (int i = 0; i < y->size; i++) {
            #if defined(_MSC_VER)
            y->data[i].re = 0.0;
            y->data[i].im = 0.0;
            #else
            y->data[i] = 0.0 + 0.0 * I;
            #endif
        }
        
        // Compute y = A^H * x = conjugate(A^T) * x
        int j;
        #ifdef _OPENMP
        #pragma omp parallel for
        #endif
        for (j = 0; j < A->num_cols; j++) {
            complex_t sum;
            #if defined(_MSC_VER)
            sum.re = 0.0;
            sum.im = 0.0;
            #else
            sum = 0.0 + 0.0 * I;
            #endif
            
            for (int i = 0; i < A->num_rows; i++) {
                int idx = i * A->num_cols + j;
                // Conjugate of A[i][j]
                #if defined(_MSC_VER)
                complex_t A_conj;
                A_conj.re = A->data.dense[idx].re;
                A_conj.im = -A->data.dense[idx].im;
                
                complex_t prod;
                prod.re = A_conj.re * x->data[i].re - A_conj.im * x->data[i].im;
                prod.im = A_conj.re * x->data[i].im + A_conj.im * x->data[i].re;
                sum.re += prod.re;
                sum.im += prod.im;
                #else
                complex_t A_conj = conj(A->data.dense[idx]);
                sum += A_conj * x->data[i];
                #endif
            }
            
            y->data[j] = sum;
        }
        
        return STATUS_SUCCESS;
    }
    
    // Sparse matrix hermitian support (CSR format)
    if (A->type == MATRIX_TYPE_SPARSE && A->data.sparse) {
        typedef struct {
            int num_rows;
            int num_cols;
            int nnz;
            int* row_ptr;
            int* col_idx;
            complex_t* values;
        } sparse_matrix_csr_t;
        
        sparse_matrix_csr_t* sparse = (sparse_matrix_csr_t*)A->data.sparse;
        
        // Initialize output
        for (int i = 0; i < y->size; i++) {
            #if defined(_MSC_VER)
            y->data[i].re = 0.0;
            y->data[i].im = 0.0;
            #else
            y->data[i] = 0.0 + 0.0 * I;
            #endif
        }
        
        // Compute y = A^H * x = conjugate(A^T) * x (CSR format)
        for (int i = 0; i < sparse->num_rows; i++) {
            for (int k = sparse->row_ptr[i]; k < sparse->row_ptr[i + 1]; k++) {
                int j = sparse->col_idx[k];
                // Conjugate of A[i][j]
                #if defined(_MSC_VER)
                complex_t A_conj;
                A_conj.re = sparse->values[k].re;
                A_conj.im = -sparse->values[k].im;
                
                complex_t prod;
                prod.re = A_conj.re * x->data[i].re - A_conj.im * x->data[i].im;
                prod.im = A_conj.re * x->data[i].im + A_conj.im * x->data[i].re;
                y->data[j].re += prod.re;
                y->data[j].im += prod.im;
                #else
                complex_t A_conj = conj(sparse->values[k]);
                y->data[j] += A_conj * x->data[i];
                #endif
            }
        }
        
        return STATUS_SUCCESS;
    }
    
    // Compressed matrices hermitian
    if (A->type == MATRIX_TYPE_COMPRESSED && A->data.compressed) {
        // L4 backend implements compressed matrix hermitian operations
        // For compressed formats, hermitian transpose requires
        // format-specific handling in L4 numerical backend
        return STATUS_ERROR_NOT_IMPLEMENTED;
    }
    
    return STATUS_ERROR_NOT_IMPLEMENTED;
}

operator_vector_t* matvec_operator_create_vector(int size) {
    if (size <= 0) return NULL;
    
    operator_vector_t* vec = (operator_vector_t*)calloc(1, sizeof(operator_vector_t));
    if (!vec) return NULL;
    
    vec->size = size;
    vec->data = (complex_t*)calloc(size, sizeof(complex_t));
    if (!vec->data) {
        free(vec);
        return NULL;
    }
    
    return vec;
}

void matvec_operator_destroy_vector(operator_vector_t* vector) {
    if (!vector) return;
    
    if (vector->data) {
        free(vector->data);
    }
    free(vector);
}
