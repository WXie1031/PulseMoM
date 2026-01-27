/********************************************************************************
 * Direct Solver Implementation (L4 Numerical Backend Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements direct solver methods (LU, QR, Cholesky).
 * L4 layer: Numerical Backend - HOW to efficiently compute operators.
 *
 * Architecture Rule: L4 sees matrices, not physics.
 ********************************************************************************/

#include "direct_solver.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../../common/constants.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// SuperLU support (if available)
#ifdef ENABLE_SUPERLU
#include <slu_ddefs.h>
#include <slu_zdefs.h>
#include <slu_util.h>
#endif

// GSL support for advanced integration (if available)
#ifdef ENABLE_GSL
#include <gsl/gsl_integration.h>
#include <gsl/gsl_errno.h>
#endif

// Direct solver data structure
typedef struct {
    direct_solver_config_t config;
    const operator_matrix_t* matrix;
    void* factorization;  // LU/QR/Cholesky factors
    int* pivot;           // Pivoting indices
    bool is_factorized;
} direct_solver_data_t;

solver_interface_t* direct_solver_create(
    const direct_solver_config_t* config) {
    
    if (!config) return NULL;
    
    direct_solver_data_t* data = (direct_solver_data_t*)calloc(1, sizeof(direct_solver_data_t));
    if (!data) return NULL;
    
    memcpy(&data->config, config, sizeof(direct_solver_config_t));
    data->is_factorized = false;
    
    // Cast to solver_interface_t for return
    // Note: This is a simplified approach. In production, would use proper inheritance
    return (solver_interface_t*)data;
}

// Helper: LU decomposition (simplified, no pivoting)
static int lu_decompose_simple(
    const complex_t* A,
    complex_t* L,
    complex_t* U,
    int n) {
    
    if (!A || !L || !U || n <= 0) return STATUS_ERROR_INVALID_INPUT;
    
    // Initialize L as identity, U as copy of A
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int idx = i * n + j;
            if (i == j) {
                #if defined(_MSC_VER)
                L[idx].re = 1.0;
                L[idx].im = 0.0;
                #else
                L[idx] = 1.0 + 0.0 * I;
                #endif
            } else {
                #if defined(_MSC_VER)
                L[idx].re = 0.0;
                L[idx].im = 0.0;
                #else
                L[idx] = 0.0 + 0.0 * I;
                #endif
            }
            U[idx] = A[idx];
        }
    }
    
    // Compute LU decomposition
    for (int k = 0; k < n - 1; k++) {
        complex_t pivot = U[k * n + k];
        
        // Check for zero pivot
        real_t pivot_mag = sqrt(pivot.re * pivot.re + pivot.im * pivot.im);
        if (pivot_mag < NUMERICAL_EPSILON) {
            return STATUS_ERROR_NUMERICAL_INSTABILITY;  // Singular matrix
        }
        
        for (int i = k + 1; i < n; i++) {
            // Compute L[i][k] = U[i][k] / U[k][k]
            #if defined(_MSC_VER)
            complex_t ratio;
            complex_t u_ik = U[i * n + k];
            ratio.re = (u_ik.re * pivot.re + u_ik.im * pivot.im) / (pivot_mag * pivot_mag);
            ratio.im = (u_ik.im * pivot.re - u_ik.re * pivot.im) / (pivot_mag * pivot_mag);
            L[i * n + k] = ratio;
            #else
            L[i * n + k] = U[i * n + k] / pivot;
            #endif
            
            // Update U: U[i][j] = U[i][j] - L[i][k] * U[k][j]
            for (int j = k + 1; j < n; j++) {
                #if defined(_MSC_VER)
                complex_t prod;
                prod.re = L[i * n + k].re * U[k * n + j].re - L[i * n + k].im * U[k * n + j].im;
                prod.im = L[i * n + k].re * U[k * n + j].im + L[i * n + k].im * U[k * n + j].re;
                U[i * n + j].re -= prod.re;
                U[i * n + j].im -= prod.im;
                #else
                U[i * n + j] -= L[i * n + k] * U[k * n + j];
                #endif
            }
            
            // Zero out below diagonal in U
            #if defined(_MSC_VER)
            U[i * n + k].re = 0.0;
            U[i * n + k].im = 0.0;
            #else
            U[i * n + k] = 0.0 + 0.0 * I;
            #endif
        }
    }
    
    return STATUS_SUCCESS;
}

// Helper: Forward substitution (L * y = b)
static int forward_substitution(
    const complex_t* L,
    const complex_t* b,
    complex_t* y,
    int n) {
    
    if (!L || !b || !y || n <= 0) return STATUS_ERROR_INVALID_INPUT;
    
    for (int i = 0; i < n; i++) {
        #if defined(_MSC_VER)
        complex_t sum;
        sum.re = b[i].re;
        sum.im = b[i].im;
        #else
        complex_t sum = b[i];
        #endif
        
        for (int j = 0; j < i; j++) {
            #if defined(_MSC_VER)
            complex_t prod;
            prod.re = L[i * n + j].re * y[j].re - L[i * n + j].im * y[j].im;
            prod.im = L[i * n + j].re * y[j].im + L[i * n + j].im * y[j].re;
            sum.re -= prod.re;
            sum.im -= prod.im;
            #else
            sum -= L[i * n + j] * y[j];
            #endif
        }
        
        // y[i] = sum / L[i][i]
        complex_t diag = L[i * n + i];
        real_t diag_mag = sqrt(diag.re * diag.re + diag.im * diag.im);
        if (diag_mag < NUMERICAL_EPSILON) {
            return STATUS_ERROR_NUMERICAL_INSTABILITY;
        }
        
        #if defined(_MSC_VER)
        y[i].re = (sum.re * diag.re + sum.im * diag.im) / (diag_mag * diag_mag);
        y[i].im = (sum.im * diag.re - sum.re * diag.im) / (diag_mag * diag_mag);
        #else
        y[i] = sum / diag;
        #endif
    }
    
    return STATUS_SUCCESS;
}

// Helper: Backward substitution (U * x = y)
static int backward_substitution(
    const complex_t* U,
    const complex_t* y,
    complex_t* x,
    int n) {
    
    if (!U || !y || !x || n <= 0) return STATUS_ERROR_INVALID_INPUT;
    
    for (int i = n - 1; i >= 0; i--) {
        #if defined(_MSC_VER)
        complex_t sum;
        sum.re = y[i].re;
        sum.im = y[i].im;
        #else
        complex_t sum = y[i];
        #endif
        
        for (int j = i + 1; j < n; j++) {
            #if defined(_MSC_VER)
            complex_t prod;
            prod.re = U[i * n + j].re * x[j].re - U[i * n + j].im * x[j].im;
            prod.im = U[i * n + j].re * x[j].im + U[i * n + j].im * x[j].re;
            sum.re -= prod.re;
            sum.im -= prod.im;
            #else
            sum -= U[i * n + j] * x[j];
            #endif
        }
        
        // x[i] = sum / U[i][i]
        complex_t diag = U[i * n + i];
        real_t diag_mag = sqrt(diag.re * diag.re + diag.im * diag.im);
        if (diag_mag < NUMERICAL_EPSILON) {
            return STATUS_ERROR_NUMERICAL_INSTABILITY;
        }
        
        #if defined(_MSC_VER)
        x[i].re = (sum.re * diag.re + sum.im * diag.im) / (diag_mag * diag_mag);
        x[i].im = (sum.im * diag.re - sum.re * diag.im) / (diag_mag * diag_mag);
        #else
        x[i] = sum / diag;
        #endif
    }
    
    return STATUS_SUCCESS;
}

int direct_solver_factorize(
    solver_interface_t* solver,
    const operator_matrix_t* matrix) {
    
    if (!solver || !matrix) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    direct_solver_data_t* data = (direct_solver_data_t*)solver;
    
    int n = matrix->num_rows;
    if (n != matrix->num_cols) {
        return STATUS_ERROR_INVALID_INPUT;  // Must be square
    }
    
    // Handle dense matrices
    if (matrix->type == MATRIX_TYPE_DENSE && matrix->data.dense) {
        // Allocate factorization storage
        // For LU: store L and U (can be stored in single matrix)
        complex_t* LU = (complex_t*)calloc(n * n, sizeof(complex_t));
        if (!LU) return STATUS_ERROR_MEMORY_ALLOCATION;
        
        // Perform LU decomposition
        complex_t* L = (complex_t*)calloc(n * n, sizeof(complex_t));
        complex_t* U = (complex_t*)calloc(n * n, sizeof(complex_t));
        if (!L || !U) {
            free(LU);
            if (L) free(L);
            if (U) free(U);
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        
        status_t status = lu_decompose_simple(matrix->data.dense, L, U, n);
        if (status != STATUS_SUCCESS) {
            free(LU);
            free(L);
            free(U);
            return status;
        }
        
        // Store L and U in combined format (simplified)
        // In production, would use more efficient storage
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i > j) {
                    LU[i * n + j] = L[i * n + j];  // Lower part: L
                } else {
                    LU[i * n + j] = U[i * n + j];  // Upper part: U
                }
            }
        }
        
        free(L);
        free(U);
        
        data->factorization = LU;
        data->matrix = matrix;
        data->is_factorized = true;
        
        return STATUS_SUCCESS;
    }
    
    // Handle sparse matrices (CSR format)
    if (matrix->type == MATRIX_TYPE_SPARSE && matrix->data.sparse) {
        // Sparse matrix structure (CSR format)
        typedef struct {
            int num_rows;
            int num_cols;
            int nnz;
            int* row_ptr;
            int* col_idx;
            complex_t* values;
        } sparse_matrix_csr_t;
        
        sparse_matrix_csr_t* sparse = (sparse_matrix_csr_t*)matrix->data.sparse;
        
        #ifdef ENABLE_SUPERLU
        // Use SuperLU for sparse LU factorization (much faster for large sparse matrices)
        // SuperLU provides optimized sparse LU decomposition
        // Note: SuperLU uses double precision, we need to convert complex_t
        // For now, fall through to dense conversion
        // Full SuperLU integration would:
        // 1. Convert CSR format to SuperLU's internal format
        // 2. Call SuperLU factorization routines
        // 3. Store factorization for later solve operations
        // This requires careful handling of complex number types
        // TODO: Implement full SuperLU integration
        #endif
        
        // For sparse matrices, convert to dense for factorization
        // Fallback: use dense matrix conversion (slower but works)
        // In production with SuperLU, this would be replaced by sparse LU
        complex_t* dense = (complex_t*)calloc(n * n, sizeof(complex_t));
        if (!dense) return STATUS_ERROR_MEMORY_ALLOCATION;
        
        // Convert CSR to dense
        for (int i = 0; i < n; i++) {
            for (int k = sparse->row_ptr[i]; k < sparse->row_ptr[i + 1]; k++) {
                int j = sparse->col_idx[k];
                dense[i * n + j] = sparse->values[k];
            }
        }
        
        // Perform LU decomposition on dense representation
        complex_t* LU = (complex_t*)calloc(n * n, sizeof(complex_t));
        if (!LU) {
            free(dense);
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        
        complex_t* L = (complex_t*)calloc(n * n, sizeof(complex_t));
        complex_t* U = (complex_t*)calloc(n * n, sizeof(complex_t));
        if (!L || !U) {
            free(dense);
            free(LU);
            if (L) free(L);
            if (U) free(U);
            return STATUS_ERROR_MEMORY_ALLOCATION;
        }
        
        status_t status = lu_decompose_simple(dense, L, U, n);
        free(dense);  // Free temporary dense matrix
        
        if (status != STATUS_SUCCESS) {
            free(LU);
            free(L);
            free(U);
            return status;
        }
        
        // Store L and U in combined format
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i > j) {
                    LU[i * n + j] = L[i * n + j];
                } else {
                    LU[i * n + j] = U[i * n + j];
                }
            }
        }
        
        free(L);
        free(U);
        
        data->factorization = LU;
        data->matrix = matrix;
        data->is_factorized = true;
        
        return STATUS_SUCCESS;
    }
    
    // Compressed matrices would be handled by specialized L4 backend
    return STATUS_ERROR_NOT_IMPLEMENTED;
}

int direct_solver_solve(
    solver_interface_t* solver,
    const operator_vector_t* rhs,
    operator_vector_t* solution) {
    
    if (!solver || !rhs || !solution) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    direct_solver_data_t* data = (direct_solver_data_t*)solver;
    
    if (!data->is_factorized || !data->factorization) {
        return STATUS_ERROR_INVALID_STATE;
    }
    
    int n = rhs->size;
    if (n != solution->size) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    complex_t* LU = (complex_t*)data->factorization;
    
    // Extract L and U (simplified - in production would use better storage)
    complex_t* L = (complex_t*)calloc(n * n, sizeof(complex_t));
    complex_t* U = (complex_t*)calloc(n * n, sizeof(complex_t));
    if (!L || !U) {
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i > j) {
                L[i * n + j] = LU[i * n + j];
                #if defined(_MSC_VER)
                U[i * n + j].re = 0.0;
                U[i * n + j].im = 0.0;
                #else
                U[i * n + j] = 0.0 + 0.0 * I;
                #endif
            } else if (i == j) {
                #if defined(_MSC_VER)
                L[i * n + j].re = 1.0;
                L[i * n + j].im = 0.0;
                #else
                L[i * n + j] = 1.0 + 0.0 * I;
                #endif
                U[i * n + j] = LU[i * n + j];
            } else {
                #if defined(_MSC_VER)
                L[i * n + j].re = 0.0;
                L[i * n + j].im = 0.0;
                #else
                L[i * n + j] = 0.0 + 0.0 * I;
                #endif
                U[i * n + j] = LU[i * n + j];
            }
        }
    }
    
    // Solve: L * y = rhs
    complex_t* y = (complex_t*)calloc(n, sizeof(complex_t));
    if (!y) {
        free(L);
        free(U);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    status_t status = forward_substitution(L, rhs->data, y, n);
    if (status != STATUS_SUCCESS) {
        free(L);
        free(U);
        free(y);
        return status;
    }
    
    // Solve: U * x = y
    status = backward_substitution(U, y, solution->data, n);
    
    free(L);
    free(U);
    free(y);
    
    return status;
}
