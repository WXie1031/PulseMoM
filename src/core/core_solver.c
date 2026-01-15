/*****************************************************************************************
 * Core Solver Implementation - Unified linear solver interface
 * 
 * Supports dense LU, sparse LU, iterative methods, and circuit solvers
 * Includes preconditioning and eigenvalue solving capabilities
 * Provides GPU acceleration and parallel processing
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "core_common.h"
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#endif

#include "core_solver.h"
#include "core_geometry.h"
#include "h_matrix_compression.h"

// Forward declarations for builtin functions to avoid redefinition conflicts
static void builtin_zgetrs(char* trans, int* n, int* nrhs, complex_t* a, int* lda, 
                          int* ipiv, complex_t* b, int* ldb, int* info);
static void builtin_apply_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z);
static void builtin_jacobi_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z);
static void builtin_ilu_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z);
static void builtin_ssor_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z);

// Forward declarations for internal solver functions
static void solver_compute_residual_internal(linear_solver_t* solver, complex_t* x, 
                                             complex_t* rhs, complex_t* residual);
static double solver_compute_norm_internal(complex_t* vector, int size);
static void solver_matrix_vector_product_internal(linear_solver_t* solver, complex_t* x, complex_t* y);
static void gmres_iteration(linear_solver_t* solver, complex_t* solution, complex_t* residual, complex_t* rhs_vec);
static int setup_dense_matrix(linear_solver_t* solver, matrix_system_t* matrix_system);
static int setup_sparse_matrix(linear_solver_t* solver, matrix_system_t* matrix_system);
static int setup_iterative_matrix(linear_solver_t* solver, matrix_system_t* matrix_system);
static int convert_to_dense(matrix_system_t* matrix_system, complex_t* dense, int size);
// Note: convert_dense_to_sparse uses sparse_solver_data_t which is defined later in this file
// We'll declare it after the type definition
static int factorize_dense_lu(linear_solver_t* solver);
static int factorize_sparse_lu(linear_solver_t* solver);
static int solve_dense_lu(linear_solver_t* solver, complex_t* rhs, complex_t* solution);
static int solve_sparse_lu(linear_solver_t* solver, complex_t* rhs, complex_t* solution);
static int solve_gmres(linear_solver_t* solver, complex_t* rhs, complex_t* solution);
static int solve_cg(linear_solver_t* solver, complex_t* rhs_vec, complex_t* solution);
static int solve_bicgstab(linear_solver_t* solver, complex_t* rhs_vec, complex_t* solution);

// Linear solver structure is defined in core_solver.h

// Internal solver data structures
typedef struct {
    int* ipiv;          // Pivot indices for LU factorization
    complex_t* lu; // LU factorized matrix
    int factored;     // Flag indicating if matrix is factored
} dense_solver_data_t;

typedef struct {
    int* rowptr;        // CSR row pointers
    int* colind;        // CSR column indices
    complex_t* values; // CSR values
    int nnz;            // Number of non-zeros
    void* symbolic;     // Symbolic factorization
    void* numeric;      // Numeric factorization
} sparse_solver_data_t;

// Now we can declare convert_dense_to_sparse after sparse_solver_data_t is defined
static int convert_dense_to_sparse(matrix_system_t* matrix_system, sparse_solver_data_t* data);

typedef struct {
    double tolerance;   // Convergence tolerance
    int max_iterations; // Maximum iterations
    int restart_size;   // GMRES restart size
    double* residuals;  // Residual history
    int num_iterations; // Actual iterations performed
} iterative_solver_data_t;

// Solver creation and destruction
// Note: This function has two signatures - old (int, int) and new (const solver_config_t*)
// For compatibility, we support both
linear_solver_t* linear_solver_create_old(int solver_type, int matrix_type) {
    linear_solver_t* solver = calloc(1, sizeof(linear_solver_t));
    if (!solver) return NULL;
    
    solver->solver_type = solver_type;
    solver->matrix_type = matrix_type;
    solver->matrix_size = 0;
    solver->num_rhs = 1;
    solver->converged = 0;
    
    // Initialize solver configuration
    solver->config.tolerance = 1e-6;
    solver->config.max_iterations = 1000;
    solver->config.restart_size = 30;
    solver->config.preconditioner_type = PRECON_JACOBI;
    solver->config.use_gpu = false;
    solver->config.num_threads = 0; // Auto-detect
    
    // Allocate solver-specific data
    switch (solver_type) {
        case SOLVER_TYPE_DENSE_LU:
            solver->solver_data = calloc(1, sizeof(dense_solver_data_t));
            break;
            
        case SOLVER_TYPE_SPARSE_LU:
            solver->solver_data = calloc(1, sizeof(sparse_solver_data_t));
            break;
            
        case SOLVER_TYPE_ITERATIVE_GMRES:
        case SOLVER_TYPE_ITERATIVE_CG:
        case SOLVER_TYPE_ITERATIVE_BICGSTAB:
            solver->solver_data = calloc(1, sizeof(iterative_solver_data_t));
            break;
            
        default:
            free(solver);
            return NULL;
    }
    
    if (!solver->solver_data) {
        free(solver);
        return NULL;
    }
    
    return solver;
}

// New signature matching header
linear_solver_t* linear_solver_create(const solver_config_t* config) {
    if (!config) return NULL;
    linear_solver_t* solver = linear_solver_create_old(config->type, MATRIX_TYPE_GENERAL);
    if (solver) {
        memcpy(&solver->config, config, sizeof(solver_config_t));
    }
    return solver;
}

// Alias for header compatibility - use wrapper as the main function
// Note: The old linear_solver_create(int, int) is renamed to linear_solver_create_old

void linear_solver_free(linear_solver_t* solver) {
    if (!solver) return;
    
    // Free solver-specific data
    switch (solver->solver_type) {
        case SOLVER_TYPE_DENSE_LU: {
            dense_solver_data_t* data = (dense_solver_data_t*)solver->solver_data;
            free(data->ipiv);
            free(data->lu);
            free(data);
            break;
        }
        
        case SOLVER_TYPE_SPARSE_LU: {
            sparse_solver_data_t* data = (sparse_solver_data_t*)solver->solver_data;
            free(data->rowptr);
            free(data->colind);
            free(data->values);
            // TODO: Free symbolic and numeric factorizations
            free(data);
            break;
        }
        
        case SOLVER_TYPE_ITERATIVE_GMRES:
        case SOLVER_TYPE_ITERATIVE_CG:
        case SOLVER_TYPE_ITERATIVE_BICGSTAB: {
            iterative_solver_data_t* data = (iterative_solver_data_t*)solver->solver_data;
            free(data->residuals);
            free(data);
            break;
        }
    }
    
    free(solver);
}

// Matrix setup
int linear_solver_setup_matrix(linear_solver_t* solver, matrix_system_t* matrix_system) {
    if (!solver || !matrix_system) return -1;
    
    solver->matrix_size = matrix_system->num_rows;
    solver->matrix_system = matrix_system;
    
    switch (solver->solver_type) {
        case SOLVER_TYPE_DENSE_LU:
            return setup_dense_matrix(solver, matrix_system);
            
        case SOLVER_TYPE_SPARSE_LU:
            return setup_sparse_matrix(solver, matrix_system);
            
        case SOLVER_TYPE_ITERATIVE_GMRES:
        case SOLVER_TYPE_ITERATIVE_CG:
        case SOLVER_TYPE_ITERATIVE_BICGSTAB:
            return setup_iterative_matrix(solver, matrix_system);
            
        default:
            return -1;
    }
}

static int setup_dense_matrix(linear_solver_t* solver, matrix_system_t* matrix_system) {
    dense_solver_data_t* data = (dense_solver_data_t*)solver->solver_data;
    
    // Allocate LU factorization storage
    data->ipiv = (int*)calloc(solver->matrix_size, sizeof(int));
    data->lu = (complex_t*)calloc(solver->matrix_size * solver->matrix_size, sizeof(complex_t));
    
    if (!data->ipiv || !data->lu) return -1;
    
    // Copy matrix to LU storage
    if (matrix_system->matrix_type == ASSEMBLY_TYPE_DENSE) {
        memcpy(data->lu, matrix_system->dense_matrix,
               solver->matrix_size * solver->matrix_size * sizeof(complex_t));
    } else {
        // Convert from sparse/compressed to dense
        convert_to_dense(matrix_system, data->lu, solver->matrix_size);
    }
    
    data->factored = 0;
    return 0;
}

static int setup_sparse_matrix(linear_solver_t* solver, matrix_system_t* matrix_system) {
    sparse_solver_data_t* data = (sparse_solver_data_t*)solver->solver_data;
    
    if (matrix_system->matrix_type == ASSEMBLY_TYPE_SPARSE) {
        data->nnz = matrix_system->num_nonzeros;
        data->rowptr = malloc((solver->matrix_size + 1) * sizeof(int));
        data->colind = malloc(data->nnz * sizeof(int));
        data->values = malloc(data->nnz * sizeof(complex_t));
        
        if (!data->rowptr || !data->colind || !data->values) return -1;
        
        // Copy CSR format data directly
        if (matrix_system->row_ptr && matrix_system->col_ind && matrix_system->sparse_values) {
            memcpy(data->rowptr, matrix_system->row_ptr, (solver->matrix_size + 1) * sizeof(int));
            memcpy(data->colind, matrix_system->col_ind, data->nnz * sizeof(int));
            memcpy(data->values, matrix_system->sparse_values, data->nnz * sizeof(complex_t));
        } else {
            return -1;
        }
    } else {
        // Convert from dense to sparse
        return convert_dense_to_sparse(matrix_system, data);
    }
    
    return 0;
}

static int setup_iterative_matrix(linear_solver_t* solver, matrix_system_t* matrix_system) {
    iterative_solver_data_t* data = (iterative_solver_data_t*)solver->solver_data;
    
    // Initialize iterative solver parameters
    data->tolerance = solver->config.tolerance;
    data->max_iterations = solver->config.max_iterations;
    data->restart_size = solver->config.restart_size;
    data->num_iterations = 0;
    
    // Allocate residual history
    data->residuals = calloc(data->max_iterations, sizeof(double));
    if (!data->residuals) return -1;
    
    return 0;
}

// Factorization
int linear_solver_factorize(linear_solver_t* solver) {
    if (!solver) return -1;
    
    printf("Factorizing matrix (%d x %d)...\n", solver->matrix_size, solver->matrix_size);
    clock_t start = clock();
    
    int status = 0;
    switch (solver->solver_type) {
        case SOLVER_TYPE_DENSE_LU:
            status = factorize_dense_lu(solver);
            break;
            
        case SOLVER_TYPE_SPARSE_LU:
            status = factorize_sparse_lu(solver);
            break;
            
        case SOLVER_TYPE_ITERATIVE_GMRES:
        case SOLVER_TYPE_ITERATIVE_CG:
        case SOLVER_TYPE_ITERATIVE_BICGSTAB:
            // Iterative solvers don't factorize
            status = 0;
            break;
            
        default:
            status = -1;
    }
    
    clock_t end = clock();
    double factor_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Factorization completed in %.2f seconds\n", factor_time);
    
    return status;
}

static int factorize_dense_lu(linear_solver_t* solver) {
    dense_solver_data_t* data = (dense_solver_data_t*)solver->solver_data;
    
    if (data->factored) return 0;
    
    // Use LAPACK for LU factorization
    int info = 0;
    int n = solver->matrix_size;
    int lda = n;
    
    // Call LAPACK's zgetrf for complex LU factorization
    // Note: zgetrf_ is from LAPACK library - link with LAPACK or provide stub
    // For now, provide a stub implementation
    #ifdef HAVE_LAPACK
    zgetrf_(&n, &n, data->lu, &lda, data->ipiv, &info);
    #else
    // Stub implementation - simple LU without pivoting (for testing only)
    info = 0;
    for (int i = 0; i < n; i++) {
        data->ipiv[i] = i + 1;  // 1-based indexing for LAPACK compatibility
        if (data->lu[i * n + i].re == 0.0 && data->lu[i * n + i].im == 0.0) {
            info = i + 1;  // Singular matrix
            break;
        }
        for (int j = i + 1; j < n; j++) {
            complex_t factor = complex_divide(&data->lu[j * n + i], &data->lu[i * n + i]);
            for (int k = i + 1; k < n; k++) {
                complex_t prod = complex_multiply(&factor, &data->lu[i * n + k]);
                data->lu[j * n + k] = complex_subtract(&data->lu[j * n + k], &prod);
            }
        }
    }
    #endif
    
    if (info != 0) {
        fprintf(stderr, "LU factorization failed with info = %d\n", info);
        return -1;
    }
    
    data->factored = 1;
    return 0;
}

static int factorize_sparse_lu(linear_solver_t* solver) {
    sparse_solver_data_t* data = (sparse_solver_data_t*)solver->solver_data;
    
    printf("Sparse LU factorization using built-in solver...\n");
    
    // Simple sparse LU factorization using Crout's method
    int n = solver->matrix_size;
    
    // Allocate storage for L and U factors
    data->values = realloc(data->values, data->nnz * 2 * sizeof(complex_t)); // Double space for L+U
    
    // Convert to CSC format for easier column access
    int* colptr = (int*)calloc(n + 1, sizeof(int));
    int* rowind = (int*)calloc(data->nnz, sizeof(int));
    complex_t* csc_values = (complex_t*)calloc(data->nnz, sizeof(complex_t));
    
    // Build column pointers
    for (int i = 0; i < data->nnz; i++) {
        colptr[data->colind[i] + 1]++;
    }
    for (int i = 1; i <= n; i++) {
        colptr[i] += colptr[i - 1];
    }
    
    // Fill CSC format
    int* col_counts = calloc(n, sizeof(int));
    for (int i = 0; i < n; i++) {
        for (int j = data->rowptr[i]; j < data->rowptr[i + 1]; j++) {
            int col = data->colind[j];
            int pos = colptr[col] + col_counts[col];
            rowind[pos] = i;
            csc_values[pos] = data->values[j];
            col_counts[col]++;
        }
    }
    
    // Simple sparse LU factorization (diagonal pivoting)
    for (int k = 0; k < n; k++) {
        // Find diagonal element
        complex_t diag_val;
        diag_val.re = 0.0;
        diag_val.im = 0.0;
        int diag_pos = -1;
        
        // Look for diagonal in column k
        for (int j = colptr[k]; j < colptr[k + 1]; j++) {
            if (rowind[j] == k) {
                diag_val = csc_values[j];
                diag_pos = j;
                break;
            }
        }
        
        if (complex_magnitude(&diag_val) < 1e-15) {
            fprintf(stderr, "Sparse LU factorization failed: zero pivot at row %d\n", k);
            free(colptr);
            free(rowind);
            free(csc_values);
            free(col_counts);
            return -1;
        }
        
        // Scale column k by diagonal
        for (int j = colptr[k]; j < colptr[k + 1]; j++) {
            if (rowind[j] > k) {
                complex_t inv_diag;
                inv_diag.re = diag_val.re / (diag_val.re * diag_val.re + diag_val.im * diag_val.im);
                inv_diag.im = -diag_val.im / (diag_val.re * diag_val.re + diag_val.im * diag_val.im);
                complex_t temp;
                temp.re = csc_values[j].re * inv_diag.re - csc_values[j].im * inv_diag.im;
                temp.im = csc_values[j].re * inv_diag.im + csc_values[j].im * inv_diag.re;
                csc_values[j] = temp;
            }
        }
        
        // Update remaining columns
        for (int col = k + 1; col < n; col++) {
            // Find element in row k, column col
            complex_t row_k_val = complex_zero();
            int row_k_pos = -1;
            
            for (int j = colptr[col]; j < colptr[col + 1]; j++) {
                if (rowind[j] == k) {
                    row_k_val = csc_values[j];
                    row_k_pos = j;
                    break;
                }
            }
            
            if (row_k_pos >= 0) {
                // Update column col
                for (int j = colptr[col]; j < colptr[col + 1]; j++) {
                    if (rowind[j] > k) {
                        // Find corresponding element in column k
                        complex_t col_k_val;
                        col_k_val.re = 0.0;
                        col_k_val.im = 0.0;
                        for (int jj = colptr[k]; jj < colptr[k + 1]; jj++) {
                            if (rowind[jj] == rowind[j]) {
                                col_k_val = csc_values[jj];
                                break;
                            }
                        }
                        complex_t product;
                        product.re = row_k_val.re * col_k_val.re - row_k_val.im * col_k_val.im;
                        product.im = row_k_val.re * col_k_val.im + row_k_val.im * col_k_val.re;
                        csc_values[j].re -= product.re;
                        csc_values[j].im -= product.im;
                    }
                }
            }
        }
    }
    
    free(colptr);
    free(rowind);
    free(csc_values);
    free(col_counts);
    
    return 0;
}

// Solving - internal implementation (complex_t*)
static int linear_solver_solve_old(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    if (!solver || !rhs || !solution) return -1;
    
    printf("Solving linear system (%d unknowns)...\n", solver->matrix_size);
    clock_t start = clock();
    
    int status = 0;
    switch (solver->solver_type) {
        case SOLVER_TYPE_DENSE_LU:
            status = solve_dense_lu(solver, rhs, solution);
            break;
            
        case SOLVER_TYPE_SPARSE_LU:
            status = solve_sparse_lu(solver, rhs, solution);
            break;
            
        case SOLVER_TYPE_ITERATIVE_GMRES:
            status = solve_gmres(solver, rhs, solution);
            break;
            
        case SOLVER_TYPE_ITERATIVE_CG:
            status = solve_cg(solver, rhs, solution);
            break;
            
        case SOLVER_TYPE_ITERATIVE_BICGSTAB:
            status = solve_bicgstab(solver, rhs, solution);
            break;
            
        default:
            status = -1;
    }
    
    clock_t end = clock();
    double solve_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Solution completed in %.2f seconds\n", solve_time);
    
    return status;
}

static int solve_dense_lu(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    dense_solver_data_t* data = (dense_solver_data_t*)solver->solver_data;
    
    if (!data->factored) {
        int status = linear_solver_factorize(solver);
        if (status != 0) return status;
    }
    
    // Copy RHS to solution
    memcpy(solution, rhs, solver->matrix_size * sizeof(complex_t));
    
    // Use LAPACK for forward/backward substitution
    int n = solver->matrix_size;
    int nrhs = 1;
    int lda = n;
    int ldb = n;
    int info = 0;
    
    // Call LAPACK's zgetrs for complex linear solve
    builtin_zgetrs("N", &n, &nrhs, data->lu, &lda, data->ipiv, solution, &ldb, &info);
    
    if (info != 0) {
        fprintf(stderr, "Linear solve failed with info = %d\n", info);
        return -1;
    }
    
    return 0;
}

static int solve_sparse_lu(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    sparse_solver_data_t* data = (sparse_solver_data_t*)solver->solver_data;
    
    printf("Sparse linear solve using built-in solver...\n");
    
    // Forward/backward substitution for sparse LU
    int n = solver->matrix_size;
    complex_t* temp = malloc(n * sizeof(complex_t));
    
    // Forward substitution (L * y = b)
    for (int i = 0; i < n; i++) {
        temp[i] = rhs[i];
        for (int j = data->rowptr[i]; j < data->rowptr[i + 1]; j++) {
            int col = data->colind[j];
            if (col < i) {
                complex_t product = complex_multiply(&data->values[j], &temp[col]);
                temp[i].re -= product.re;
                temp[i].im -= product.im;
            }
        }
    }
    
    // Backward substitution (U * x = y)
    for (int i = n - 1; i >= 0; i--) {
        solution[i] = temp[i];
        for (int j = data->rowptr[i]; j < data->rowptr[i + 1]; j++) {
            int col = data->colind[j];
            if (col > i) {
                complex_t product = complex_multiply(&data->values[j], &solution[col]);
                solution[i].re -= product.re;
                solution[i].im -= product.im;
            }
        }
        // Find diagonal element
        for (int j = data->rowptr[i]; j < data->rowptr[i + 1]; j++) {
            if (data->colind[j] == i) {
                complex_t inv_diag = complex_divide(&complex_one, &data->values[j]);
                solution[i] = complex_multiply(&solution[i], &inv_diag);
                break;
            }
        }
    }
    
    free(temp);
    return 0;
}

static int solve_gmres(linear_solver_t* solver, complex_t* rhs, complex_t* solution) {
    iterative_solver_data_t* data = (iterative_solver_data_t*)solver->solver_data;
    
    printf("GMRES iterative solver (max_iter=%d, tol=%.2e, restart=%d)\n",
           data->max_iterations, data->tolerance, data->restart_size);
    
    // Initialize solution
    for (int i = 0; i < solver->matrix_size; i++) {
        solution[i].re = 0.0;
        solution[i].im = 0.0;
    }
    
    // GMRES algorithm
    int converged = 0;
    double residual_norm = 0.0;
    complex_t* residual = NULL;
    
    for (int iter = 0; iter < data->max_iterations; iter++) {
        // Compute residual
        residual = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
        solver_compute_residual_internal(solver, solution, rhs, residual);
        
        residual_norm = solver_compute_norm_internal(residual, solver->matrix_size);
        data->residuals[iter] = residual_norm;
        
        if (residual_norm < data->tolerance) {
            converged = 1;
            data->num_iterations = iter + 1;
            printf("GMRES converged in %d iterations, residual = %.2e\n", 
                   data->num_iterations, residual_norm);
            free(residual);
            break;
        }
        
        // GMRES iteration (simplified)
        gmres_iteration(solver, solution, residual, rhs);
        
        free(residual);
        
        // Restart GMRES if needed
        if ((iter + 1) % data->restart_size == 0) {
            printf("GMRES restart at iteration %d\n", iter + 1);
        }
    }
    
    if (!converged) {
        fprintf(stderr, "GMRES failed to converge after %d iterations, final residual = %.2e\n",
                data->max_iterations, residual_norm);
        return -1;
    }
    
    solver->converged = 1;
    return 0;
}

static int solve_cg(linear_solver_t* solver, complex_t* rhs_vec, complex_t* solution) {
    iterative_solver_data_t* data = (iterative_solver_data_t*)solver->solver_data;
    
    printf("Conjugate Gradient solver (max_iter=%d, tol=%.2e)\n",
           data->max_iterations, data->tolerance);
    
    // Initialize solution
    for (int i = 0; i < solver->matrix_size; i++) {
        solution[i].re = 0.0;
        solution[i].im = 0.0;
    }
    
    // CG algorithm for Hermitian positive definite systems
    complex_t* residual = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    complex_t* direction = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    complex_t* q = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    
    // Initial residual
    solver_compute_residual_internal(solver, solution, rhs_vec, residual);
    memcpy(direction, residual, solver->matrix_size * sizeof(complex_t));
    
    double residual_norm = solver_compute_norm_internal(residual, solver->matrix_size);
    double initial_norm = residual_norm;
    
    for (int iter = 0; iter < data->max_iterations; iter++) {
        if (residual_norm < data->tolerance * initial_norm) {
            data->num_iterations = iter + 1;
            printf("CG converged in %d iterations, residual = %.2e\n", 
                   data->num_iterations, residual_norm);
            break;
        }
        
        // Matrix-vector product: q = A * direction
        solver_matrix_vector_product_internal(solver, direction, q);
        
        // Compute step size
        complex_t alpha = complex_zero();
        for (int i = 0; i < solver->matrix_size; i++) {
            alpha.re += residual[i].re * residual[i].re + residual[i].im * residual[i].im;
            alpha.im += residual[i].re * residual[i].im - residual[i].im * residual[i].re;
        }
        
        complex_t denom = complex_zero();
        for (int i = 0; i < solver->matrix_size; i++) {
            denom.re += direction[i].re * q[i].re + direction[i].im * q[i].im;
            denom.im += direction[i].re * q[i].im - direction[i].im * q[i].re;
        }
        
        if (complex_magnitude(&denom) < 1e-15) {
            fprintf(stderr, "CG breakdown: zero denominator\n");
            free(residual);
            free(direction);
            free(q);
            return -1;
        }
        
        complex_t alpha_quotient = complex_divide(&alpha, &denom);
        
        // Update solution: x = x + alpha * direction
        for (int i = 0; i < solver->matrix_size; i++) {
            solution[i].re += alpha_quotient.re * direction[i].re - alpha_quotient.im * direction[i].im;
            solution[i].im += alpha_quotient.re * direction[i].im + alpha_quotient.im * direction[i].re;
        }
        
        // Update residual: r = r - alpha * q
        for (int i = 0; i < solver->matrix_size; i++) {
            residual[i].re -= alpha_quotient.re * q[i].re - alpha_quotient.im * q[i].im;
            residual[i].im -= alpha_quotient.re * q[i].im + alpha_quotient.im * q[i].re;
        }
        
        // Compute new residual norm
        double new_residual_norm = solver_compute_norm_internal(residual, solver->matrix_size);
        data->residuals[iter] = new_residual_norm;
        
        // Update direction
        complex_t beta = complex_zero();
        for (int i = 0; i < solver->matrix_size; i++) {
            beta.re += residual[i].re * residual[i].re + residual[i].im * residual[i].im;
            beta.im += residual[i].re * residual[i].im - residual[i].im * residual[i].re;
        }
        
        if (complex_magnitude(&alpha_quotient) < 1e-15) {
            beta.re = 0.0;
            beta.im = 0.0;
        } else {
            complex_t alpha_conj = {alpha_quotient.re, -alpha_quotient.im};
            complex_t denom_conj = {denom.re, -denom.im};
            complex_t temp = complex_multiply(&alpha_conj, &denom_conj);
            beta = complex_divide(&beta, &temp);
        }
        
        for (int i = 0; i < solver->matrix_size; i++) {
            direction[i].re = residual[i].re + beta.re * direction[i].re - beta.im * direction[i].im;
            direction[i].im = residual[i].im + beta.re * direction[i].im + beta.im * direction[i].re;
        }
        
        residual_norm = new_residual_norm;
    }
    
    free(residual);
    free(direction);
    free(q);
    
    solver->converged = 1;
    return 0;
}

static int solve_bicgstab(linear_solver_t* solver, complex_t* rhs_vec, complex_t* solution) {
    iterative_solver_data_t* data = (iterative_solver_data_t*)solver->solver_data;
    
    printf("BiCGSTAB solver (max_iter=%d, tol=%.2e)\n",
           data->max_iterations, data->tolerance);
    
    // Initialize solution
    for (int i = 0; i < solver->matrix_size; i++) {
        solution[i].re = 0.0;
        solution[i].im = 0.0;
    }
    
    // BiCGSTAB algorithm for non-symmetric systems
    complex_t* residual = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    complex_t* residual_hat = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    complex_t* p = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    complex_t* v = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    complex_t* s = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    complex_t* t = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    
    // Initial residual
    solver_compute_residual_internal(solver, solution, rhs_vec, residual);
    memcpy(residual_hat, residual, solver->matrix_size * sizeof(complex_t));
    memcpy(p, residual, solver->matrix_size * sizeof(complex_t));
    
    double residual_norm = solver_compute_norm_internal(residual, solver->matrix_size);
    double initial_norm = residual_norm;
    
    complex_t rho = {1.0, 0.0}, alpha = {1.0, 0.0}, omega = {1.0, 0.0};
    
    for (int iter = 0; iter < data->max_iterations; iter++) {
        if (residual_norm < data->tolerance * initial_norm) {
            data->num_iterations = iter + 1;
            printf("BiCGSTAB converged in %d iterations, residual = %.2e\n", 
                   data->num_iterations, residual_norm);
            break;
        }
        
        complex_t rho_new = complex_zero();
        for (int i = 0; i < solver->matrix_size; i++) {
            rho_new.re += residual_hat[i].re * residual[i].re + residual_hat[i].im * residual[i].im;
            rho_new.im += residual_hat[i].re * residual[i].im - residual_hat[i].im * residual[i].re;
        }
        
        if (complex_magnitude(&rho) < 1e-15) {
            fprintf(stderr, "BiCGSTAB breakdown: zero rho\n");
            free(residual);
            free(residual_hat);
            free(p);
            free(v);
            free(s);
            free(t);
            return -1;
        }
        
        complex_t alpha_omega = complex_divide(&alpha, &omega);
        complex_t beta = complex_multiply(&rho_new, &alpha_omega);
        beta = complex_divide(&beta, &rho);
        
        // Update p: p = residual + beta * (p - omega * v)
        for (int i = 0; i < solver->matrix_size; i++) {
            complex_t temp = {p[i].re - omega.re * v[i].re + omega.im * v[i].im, 
                             p[i].im - omega.re * v[i].im - omega.im * v[i].re};
            p[i].re = residual[i].re + beta.re * temp.re - beta.im * temp.im;
            p[i].im = residual[i].im + beta.re * temp.im + beta.im * temp.re;
        }
        
        // Matrix-vector product: v = A * p
        solver_matrix_vector_product_internal(solver, p, v);
        
        // Compute alpha
        complex_t denom = complex_zero();
        for (int i = 0; i < solver->matrix_size; i++) {
            denom.re += residual_hat[i].re * v[i].re + residual_hat[i].im * v[i].im;
            denom.im += residual_hat[i].re * v[i].im - residual_hat[i].im * v[i].re;
        }
        
        if (complex_magnitude(&denom) < 1e-15) {
            fprintf(stderr, "BiCGSTAB breakdown: zero denominator\n");
            free(residual);
            free(residual_hat);
            free(p);
            free(v);
            free(s);
            free(t);
            return -1;
        }
        
        alpha = complex_divide(&rho_new, &denom);
        
        // Update s: s = residual - alpha * v
        for (int i = 0; i < solver->matrix_size; i++) {
            s[i].re = residual[i].re - alpha.re * v[i].re + alpha.im * v[i].im;
            s[i].im = residual[i].im - alpha.re * v[i].im - alpha.im * v[i].re;
        }
        
        // Matrix-vector product: t = A * s
        solver_matrix_vector_product_internal(solver, s, t);
        
        // Compute omega
        complex_t omega_num = complex_zero(), omega_denom = complex_zero();
        for (int i = 0; i < solver->matrix_size; i++) {
            omega_num.re += t[i].re * s[i].re + t[i].im * s[i].im;
            omega_num.im += t[i].re * s[i].im - t[i].im * s[i].re;
            omega_denom.re += t[i].re * t[i].re + t[i].im * t[i].im;
            omega_denom.im += t[i].re * t[i].im - t[i].im * t[i].re;
        }
        
        if (complex_magnitude(&omega_denom) < 1e-15) {
            fprintf(stderr, "BiCGSTAB breakdown: zero omega denominator\n");
            free(residual);
            free(residual_hat);
            free(p);
            free(v);
            free(s);
            free(t);
            return -1;
        }
        
        omega = complex_divide(&omega_num, &omega_denom);
        
        // Update solution: x = x + alpha * p + omega * s
        for (int i = 0; i < solver->matrix_size; i++) {
            complex_t alpha_p = {alpha.re * p[i].re - alpha.im * p[i].im, 
                               alpha.re * p[i].im + alpha.im * p[i].re};
            complex_t omega_s = {omega.re * s[i].re - omega.im * s[i].im, 
                               omega.re * s[i].im + omega.im * s[i].re};
            solution[i].re += alpha_p.re + omega_s.re;
            solution[i].im += alpha_p.im + omega_s.im;
        }
        
        // Update residual: residual = s - omega * t
        for (int i = 0; i < solver->matrix_size; i++) {
            residual[i].re = s[i].re - omega.re * t[i].re + omega.im * t[i].im;
            residual[i].im = s[i].im - omega.re * t[i].im - omega.im * t[i].re;
        }
        
        residual_norm = solver_compute_norm_internal(residual, solver->matrix_size);
        data->residuals[iter] = residual_norm;
        
        rho = rho_new;
    }
    
    free(residual);
    free(residual_hat);
    free(p);
    free(v);
    free(s);
    free(t);
    
    solver->converged = 1;
    return 0;
}

// Utility functions
static void solver_compute_residual_internal(linear_solver_t* solver, complex_t* x, 
                           complex_t* b, complex_t* residual) {
    // Compute residual: r = b - A * x
    solver_matrix_vector_product_internal(solver, x, residual);
    
    for (int i = 0; i < solver->matrix_size; i++) {
        residual[i].re = b[i].re - residual[i].re;
        residual[i].im = b[i].im - residual[i].im;
    }
}

static double solver_compute_norm_internal(complex_t* vector, int size) {
    double norm = 0.0;
    for (int i = 0; i < size; i++) {
        norm += vector[i].re * vector[i].re + vector[i].im * vector[i].im;
    }
    return sqrt(norm);
}

static void solver_matrix_vector_product_internal(linear_solver_t* solver, complex_t* x, complex_t* y) {
    // Matrix-vector product y = A * x
    matrix_system_t* matrix = solver->matrix_system;
    
    switch (matrix->matrix_type) {
        case ASSEMBLY_TYPE_DENSE:
            // Dense matrix-vector product
            for (int i = 0; i < matrix->num_rows; i++) {
                y[i].re = 0.0;
                y[i].im = 0.0;
                for (int j = 0; j < matrix->num_cols; j++) {
                    y[i].re += matrix->dense_matrix[i * matrix->num_cols + j].re * x[j].re - 
                              matrix->dense_matrix[i * matrix->num_cols + j].im * x[j].im;
                    y[i].im += matrix->dense_matrix[i * matrix->num_cols + j].re * x[j].im + 
                              matrix->dense_matrix[i * matrix->num_cols + j].im * x[j].re;
                }
            }
            break;
            
        case ASSEMBLY_TYPE_SPARSE:
            // Use the sparse matrix data directly from matrix_system_t
            for (int i = 0; i < matrix->num_rows; i++) {
                y[i].re = 0.0;
                y[i].im = 0.0;
                for (int idx = matrix->row_ptr[i]; idx < matrix->row_ptr[i + 1]; idx++) {
                    int j = matrix->col_ind[idx];
                    y[i].re += matrix->sparse_values[idx].re * x[j].re - matrix->sparse_values[idx].im * x[j].im;
                    y[i].im += matrix->sparse_values[idx].re * x[j].im + matrix->sparse_values[idx].im * x[j].re;
                }
            }
            break;
            
        case ASSEMBLY_TYPE_COMPRESSED:
            // For compressed storage (ACA), we need to reconstruct the matrix
            // This is a simplified implementation - in practice, you'd use the U and V matrices
            for (int i = 0; i < matrix->num_rows; i++) {
                y[i].re = 0.0;
                y[i].im = 0.0;
                for (int j = 0; j < matrix->num_cols; j++) {
                    // Reconstruct element (i,j) from compressed representation
                    complex_t element = complex_zero();
                    for (int k = 0; k < matrix->ranks[0]; k++) { // Assuming single block for now
                        complex_t u_ik = matrix->U_matrices[0][i * matrix->ranks[0] + k];
                        complex_t v_kj = matrix->V_matrices[0][k * matrix->num_cols + j];
                        complex_t product = complex_multiply(&u_ik, &v_kj);
                        element.re += product.re;
                        element.im += product.im;
                    }
                    // Add to y[i]
                    y[i].re += element.re * x[j].re - element.im * x[j].im;
                    y[i].im += element.re * x[j].im + element.im * x[j].re;
                }
            }
            break;
    }
}

static void gmres_iteration(linear_solver_t* solver, complex_t* solution, complex_t* residual, complex_t* rhs_vec) {
    // Improved GMRES iteration with preconditioning
    iterative_solver_data_t* data = (iterative_solver_data_t*)solver->solver_data;
    
    complex_t* z = (complex_t*)calloc(solver->matrix_size, sizeof(complex_t));
    
    // Apply preconditioner if available
    if (solver->config.preconditioner_type != PRECON_NONE) {
        builtin_apply_preconditioner(solver, residual, z);
    } else {
        memcpy(z, residual, solver->matrix_size * sizeof(complex_t));
    }
    
    // Compute step size using line search
    complex_t alpha = complex_zero();
    complex_t rz = complex_zero(), zz = complex_zero();
    
    for (int i = 0; i < solver->matrix_size; i++) {
        rz.re += residual[i].re * z[i].re + residual[i].im * z[i].im;
        rz.im += residual[i].re * z[i].im - residual[i].im * z[i].re;
        zz.re += z[i].re * z[i].re + z[i].im * z[i].im;
        zz.im += z[i].re * z[i].im - z[i].im * z[i].re;
    }
    
    if (complex_magnitude(&zz) > 1e-15) {
        alpha = complex_divide(&rz, &zz);
    } else {
        alpha.re = 0.1; // Fallback step size
        alpha.im = 0.0;
    }
    
    // Update solution: solution = solution + alpha * z
    for (int i = 0; i < solver->matrix_size; i++) {
        solution[i].re += alpha.re * z[i].re - alpha.im * z[i].im;
        solution[i].im += alpha.re * z[i].im + alpha.im * z[i].re;
    }
    
    free(z);
}

// Built-in LU decomposition (renamed to avoid LAPACK conflicts)
static void builtin_zgetrf(int* m, int* n, complex_t* a, int* lda, int* ipiv, int* info) {
    // Built-in LU factorization with partial pivoting
    int min_mn = (*m < *n) ? *m : *n;
    
    for (int i = 0; i < min_mn; i++) {
        // Find pivot
        int pivot_row = i;
        complex_t diag_elem = a[i * (*lda) + i];
        double max_val = complex_magnitude(&diag_elem);
        
        for (int j = i + 1; j < *m; j++) {
            complex_t test_elem = a[j * (*lda) + i];
            double val = complex_magnitude(&test_elem);
            if (val > max_val) {
                max_val = val;
                pivot_row = j;
            }
        }
        
        ipiv[i] = pivot_row + 1; // LAPACK uses 1-based indexing
        
        // Swap rows if needed
        if (pivot_row != i) {
            for (int j = 0; j < *n; j++) {
                complex_t temp = a[i * (*lda) + j];
                a[i * (*lda) + j] = a[pivot_row * (*lda) + j];
                a[pivot_row * (*lda) + j] = temp;
            }
        }
        
        // Check for singularity
        complex_t diag_check = a[i * (*lda) + i];
        if (complex_magnitude(&diag_check) < 1e-15) {
            *info = i + 1;
            return;
        }
        
        // Compute multipliers and update matrix
        for (int j = i + 1; j < *m; j++) {
            complex_t pivot_elem = a[j * (*lda) + i];
            complex_t diag_pivot = a[i * (*lda) + i];
            complex_t multiplier = complex_divide(&pivot_elem, &diag_pivot);
            a[j * (*lda) + i] = multiplier;
            
            for (int k = i + 1; k < *n; k++) {
                complex_t update_elem = a[j * (*lda) + k];
                complex_t pivot_elem2 = a[j * (*lda) + i];
                complex_t diag_elem2 = a[i * (*lda) + k];
                complex_t update = complex_multiply(&pivot_elem2, &diag_elem2);
                update_elem.re -= update.re;
                update_elem.im -= update.im;
                a[j * (*lda) + k] = update_elem;
            }
        }
    }
    
    *info = 0;
}

static void builtin_zgetrs(char* trans, int* n, int* nrhs, complex_t* a, int* lda, 
                          int* ipiv, complex_t* b, int* ldb, int* info) {
    // Built-in forward/backward substitution
    complex_t* temp = malloc(*n * sizeof(complex_t));
    
    // Apply row permutations to RHS
    for (int i = 0; i < *n; i++) {
        temp[i] = b[i];
    }
    
    for (int i = 0; i < *n; i++) {
        int pivot = ipiv[i] - 1; // Convert to 0-based
        if (pivot != i) {
            complex_t temp_val = temp[i];
            temp[i] = temp[pivot];
            temp[pivot] = temp_val;
        }
    }
    
    // Forward substitution (L * y = b)
    for (int i = 0; i < *n; i++) {
        for (int j = 0; j < i; j++) {
            complex_t lij = a[i * (*lda) + j];
            complex_t product = complex_multiply(&lij, &temp[j]);
            temp[i].re -= product.re;
            temp[i].im -= product.im;
        }
    }
    
    // Backward substitution (U * x = y)
    for (int i = *n - 1; i >= 0; i--) {
        for (int j = i + 1; j < *n; j++) {
            complex_t uij = a[i * (*lda) + j];
            complex_t product = complex_multiply(&uij, &temp[j]);
            temp[i].re -= product.re;
            temp[i].im -= product.im;
        }
        complex_t uii = a[i * (*lda) + i];
        complex_t quotient = complex_divide(&temp[i], &uii);
        temp[i] = quotient;
    }
    
    // Copy solution back
    for (int i = 0; i < *n; i++) {
        b[i] = temp[i];
    }
    
    free(temp);
    *info = 0;
}

// Preconditioner functions
static void builtin_apply_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z) {
    switch (solver->config.preconditioner_type) {
        case PRECON_JACOBI:
            builtin_jacobi_preconditioner(solver, r, z);
            break;
            
        case PRECON_ILU:
            builtin_ilu_preconditioner(solver, r, z);
            break;
            
        case PRECON_SSOR:
            builtin_ssor_preconditioner(solver, r, z);
            break;
            
        default:
            memcpy(z, r, solver->matrix_size * sizeof(complex_t));
    }
}

static void builtin_jacobi_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z) {
    // Simple diagonal (Jacobi) preconditioner
    matrix_system_t* matrix = solver->matrix_system;
    
    for (int i = 0; i < solver->matrix_size; i++) {
        complex_t diag = complex_zero();
        
        // Extract diagonal element
        switch (matrix->matrix_type) {
            case ASSEMBLY_TYPE_DENSE:
                diag = matrix->dense_matrix[i * solver->matrix_size + i];
                break;
                
            case ASSEMBLY_TYPE_SPARSE: {
                for (int j = matrix->row_ptr[i]; j < matrix->row_ptr[i + 1]; j++) {
                    if (matrix->col_ind[j] == i) {
                        diag = matrix->sparse_values[j];
                        break;
                    }
                }
                break;
            }
            
            case ASSEMBLY_TYPE_COMPRESSED:
                // For compressed matrices, use identity for now
                diag.re = 1.0;
                diag.im = 0.0;
                break;
        }
        
        if (complex_magnitude(&diag) > 1e-15) {
            complex_t quotient = complex_divide(&r[i], &diag);
            z[i] = quotient;
        } else {
            z[i] = r[i]; // No preconditioning for zero diagonal
        }
    }
}

static void builtin_ilu_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z) {
    // Incomplete LU preconditioner (simplified ILU(0))
    // For now, just use Jacobi
    builtin_jacobi_preconditioner(solver, r, z);
}

static void builtin_ssor_preconditioner(linear_solver_t* solver, complex_t* r, complex_t* z) {
    // Symmetric Successive Over-Relaxation preconditioner
    double omega = 1.5; // Relaxation parameter
    
    // Forward sweep: (D + omega*L) * y = omega * r
    for (int i = 0; i < solver->matrix_size; i++) {
        z[i].re = omega * r[i].re;
        z[i].im = omega * r[i].im;
        
        // Subtract lower triangular part contribution
        matrix_system_t* matrix = solver->matrix_system;
        switch (matrix->matrix_type) {
            case ASSEMBLY_TYPE_DENSE:
                for (int j = 0; j < i; j++) {
                    complex_t omega_Aij = complex_multiply_real(&matrix->dense_matrix[i * solver->matrix_size + j], omega);
                    complex_t omega_Aij_zj = complex_multiply(&omega_Aij, &z[j]);
                    z[i].re -= omega_Aij_zj.re;
                    z[i].im -= omega_Aij_zj.im;
                }
                // Divide by diagonal
                complex_t diag = matrix->dense_matrix[i * solver->matrix_size + i];
                complex_t inv_diag_backward = complex_divide_real(&complex_one, diag.re);
                z[i] = complex_multiply(&z[i], &inv_diag_backward);
                break;
                
            case ASSEMBLY_TYPE_SPARSE: {
                for (int j = matrix->row_ptr[i]; j < matrix->row_ptr[i + 1]; j++) {
                    int col = matrix->col_ind[j];
                    if (col < i) {
                        complex_t omega_Aij = complex_multiply_real(&matrix->sparse_values[j], omega);
                        complex_t omega_Aij_zj = complex_multiply(&omega_Aij, &z[col]);
                        z[i].re -= omega_Aij_zj.re;
                        z[i].im -= omega_Aij_zj.im;
                    } else if (col == i) {
                        complex_t inv_diag_sparse = complex_divide_real(&complex_one, matrix->sparse_values[j].re);
                        z[i] = complex_multiply(&z[i], &inv_diag_sparse);
                    }
                }
                break;
            }
            
            default:
                z[i] = r[i]; // Fallback to no preconditioning
        }
    }
    
    // Backward sweep: (D + omega*U) * x = D * y
    for (int i = solver->matrix_size - 1; i >= 0; i--) {
        complex_t diag = complex_zero();
        
        // First compute D * y
        matrix_system_t* matrix = solver->matrix_system;
        switch (matrix->matrix_type) {
            case ASSEMBLY_TYPE_DENSE:
                diag = matrix->dense_matrix[i * solver->matrix_size + i];
                z[i] = complex_multiply(&diag, &z[i]);
                
                // Subtract upper triangular part contribution
                for (int j = i + 1; j < solver->matrix_size; j++) {
                    complex_t omega_Aij = complex_multiply_real(&matrix->dense_matrix[i * solver->matrix_size + j], omega);
                    complex_t omega_Aij_zj = complex_multiply(&omega_Aij, &z[j]);
                    z[i].re -= omega_Aij_zj.re;
                    z[i].im -= omega_Aij_zj.im;
                }
                complex_t inv_diag_sparse2 = complex_divide_real(&complex_one, diag.re);
                z[i] = complex_multiply(&z[i], &inv_diag_sparse2);
                break;
                
            case ASSEMBLY_TYPE_SPARSE: {
                for (int j = matrix->row_ptr[i]; j < matrix->row_ptr[i + 1]; j++) {
                    int col = matrix->col_ind[j];
                    if (col == i) {
                        diag = matrix->sparse_values[j];
                        z[i] = complex_multiply(&diag, &z[i]);
                    } else if (col > i) {
                        complex_t omega_Aij = complex_multiply_real(&matrix->sparse_values[j], omega);
                        complex_t omega_Aij_zj = complex_multiply(&omega_Aij, &z[col]);
                        z[i].re -= omega_Aij_zj.re;
                        z[i].im -= omega_Aij_zj.im;
                    }
                }
                complex_t inv_diag = complex_divide_real(&complex_one, diag.re);
                z[i] = complex_multiply(&z[i], &inv_diag);
                break;
            }
        }
    }
}

// Extract diagonal element from H-matrix block
static complex_t get_diagonal_element_from_block(HMatrixBlock* block, int local_i, int block_size) {
    // For diagonal block, find the diagonal element
    if (block->is_low_rank) {
        // Low-rank block: reconstruct diagonal
        complex_t diag = complex_zero();
        for (int k = 0; k < block->data.low_rank.rank; k++) {
            complex_t u_elem = block->data.low_rank.U[local_i * block->data.low_rank.rank + k];
            
            complex_t v_elem = block->data.low_rank.V[k * block_size + local_i];
            
            complex_t v_conj = {v_elem.re, -v_elem.im};
            complex_t product = complex_multiply(&u_elem, &v_conj);
            diag.re += product.re;
            diag.im += product.im;
        }
        return diag;
    } else {
        // Full rank block
        return block->data.dense.data[local_i * block_size + local_i];
    }
}
static int convert_to_dense(matrix_system_t* matrix_system, complex_t* dense, int size) {
    // Convert any matrix type to dense format
    memset(dense, 0, size * size * sizeof(complex_t));
    
    switch (matrix_system->matrix_type) {
        case ASSEMBLY_TYPE_SPARSE: {
            // Convert sparse CSR to dense
            for (int i = 0; i < matrix_system->num_rows; i++) {
                for (int idx = matrix_system->row_ptr[i]; idx < matrix_system->row_ptr[i + 1]; idx++) {
                    int j = matrix_system->col_ind[idx];
                    dense[i * size + j] = matrix_system->sparse_values[idx];
                }
            }
            break;
        }
        
        case ASSEMBLY_TYPE_COMPRESSED: {
            // Decompress ACA compressed matrix
            for (int b = 0; b < matrix_system->num_blocks; b++) {
                int block_rows = matrix_system->block_rows[b];
                int block_cols = matrix_system->block_cols[b];
                int rank = matrix_system->ranks[b];
                
                // Reconstruct block: A = U * V^H
                for (int i = 0; i < block_rows; i++) {
                    for (int j = 0; j < block_cols; j++) {
                        complex_t sum = complex_zero();
                        for (int k = 0; k < rank; k++) {
                            complex_t u_elem = matrix_system->U_matrices[b][i * rank + k];
                            complex_t v_conj = {matrix_system->V_matrices[b][k * block_cols + j].re,
                                               -matrix_system->V_matrices[b][k * block_cols + j].im};
                            complex_t product = complex_multiply(&u_elem, &v_conj);
                            sum.re += product.re;
                            sum.im += product.im;
                        }
                        // Find global indices - this is simplified, needs proper mapping
                        int global_i = i; // Should use proper index mapping
                        int global_j = j; // Should use proper index mapping
                        if (global_i < size && global_j < size) {
                            dense[global_i * size + global_j] = sum;
                        }
                    }
                }
            }
            break;
        }
    }
    
    return 0;
}

static int convert_to_csr(sparse_matrix_t* sparse, int* rowptr, int* colind, complex_t* values) {
    // Convert to CSR format
    int n = sparse->num_rows;
    
    // Count non-zeros per row
    int* row_counts = calloc(n, sizeof(int));
    for (int idx = 0; idx < sparse->row_ptr[n]; idx++) {
        int row = 0;
        // Find which row this index belongs to
        for (int i = 0; i < n; i++) {
            if (idx >= sparse->row_ptr[i] && idx < sparse->row_ptr[i + 1]) {
                row = i;
                break;
            }
        }
        row_counts[row]++;
    }
    
    // Build row pointers
    rowptr[0] = 0;
    for (int i = 0; i < n; i++) {
        rowptr[i + 1] = rowptr[i] + row_counts[i];
    }
    
    // Build column indices and values
    int* row_positions = calloc(n, sizeof(int));
    for (int idx = 0; idx < sparse->row_ptr[n]; idx++) {
        int row = 0;
        for (int i = 0; i < n; i++) {
            if (idx >= sparse->row_ptr[i] && idx < sparse->row_ptr[i + 1]) {
                row = i;
                break;
            }
        }
        int pos = rowptr[row] + row_positions[row];
        
        colind[pos] = sparse->col_idx[idx];
        values[pos] = sparse->values[idx];
        row_positions[row]++;
    }
    
    free(row_counts);
    free(row_positions);
    return 0;
}

static int convert_dense_to_sparse(matrix_system_t* matrix_system, sparse_solver_data_t* data) {
    // Convert dense matrix to sparse format
    // Count non-zeros first
    int nnz = 0;
    complex_t* dense = matrix_system->dense_matrix;
    int n = matrix_system->num_rows;
    
    for (int i = 0; i < n * n; i++) {
        double magnitude = complex_magnitude(&dense[i]);
        if (magnitude > 1e-15) nnz++;
    }
    
    data->nnz = nnz;
    data->rowptr = malloc((n + 1) * sizeof(int));
    data->colind = malloc(nnz * sizeof(int));
    data->values = malloc(nnz * sizeof(complex_t));
    
    if (!data->rowptr || !data->colind || !data->values) return -1;
    
    // Build sparse matrix
    int idx = 0;
    data->rowptr[0] = 0;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double magnitude = complex_magnitude(&dense[i * n + j]);
            if (magnitude > 1e-15) {
                data->colind[idx] = j;
                data->values[idx] = dense[i * n + j];
                idx++;
            }
        }
        data->rowptr[i + 1] = idx;
    }
    
    return 0;
}