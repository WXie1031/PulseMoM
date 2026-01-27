/**
 * @file core_solver.h
 * @brief Unified solver interface for MoM and PEEC
 * @details Common linear solver interface with multiple backends
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CORE_SOLVER_H
#define CORE_SOLVER_H

#include <stdint.h>
#include <stdbool.h>
#include "../../common/core_common.h"
#include "../../operators/assembler/core_assembler.h"
#include "sparse_direct_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Solver types
typedef enum {
    SOLVER_TYPE_DENSE_LU,         // Dense LU decomposition
    SOLVER_TYPE_DENSE_QR,         // Dense QR decomposition
    SOLVER_TYPE_SPARSE_LU,        // Sparse LU (UMFPACK, KLU)
    SOLVER_TYPE_SPARSE_CHOLESKY,  // Sparse Cholesky
    SOLVER_TYPE_ITERATIVE_GMRES,  // GMRES
    SOLVER_TYPE_ITERATIVE_CG,     // Conjugate Gradient
    SOLVER_TYPE_ITERATIVE_BICGSTAB, // BiCGSTAB
    SOLVER_TYPE_ITERATIVE_TFQMR,  // TFQMR
    SOLVER_TYPE_CIRCUIT_MNA,      // Modified Nodal Analysis (PEEC)
    SOLVER_TYPE_CIRCUIT_STATE_SPACE, // State space (PEEC)
    SOLVER_TYPE_EIGENVALUE,       // Eigenvalue solver
    SOLVER_TYPE_SVD               // Singular Value Decomposition
} solver_type_t;

typedef enum {
    SOLVER_BACKEND_NATIVE,        // Native implementation
    SOLVER_BACKEND_MKL,           // Intel MKL
    SOLVER_BACKEND_OPENBLAS,      // OpenBLAS
    SOLVER_BACKEND_CUDA,          // NVIDIA CUDA
    SOLVER_BACKEND_CLBLAS,        // OpenCL BLAS
    SOLVER_BACKEND_UMFPACK,       // UMFPACK for sparse
    SOLVER_BACKEND_KLU,           // KLU for sparse
    SOLVER_BACKEND_PETSC,         // PETSc
    SOLVER_BACKEND_TRILINOS      // Trilinos
} solver_backend_t;

// Solver precision
typedef enum {
    SOLVER_PRECISION_SINGLE,
    SOLVER_PRECISION_DOUBLE,
    SOLVER_PRECISION_QUAD
} solver_precision_t;

// Matrix types for solver selection
typedef enum {
    MATRIX_TYPE_GENERAL,          // General matrix
    MATRIX_TYPE_SYMMETRIC,        // Symmetric matrix
    MATRIX_TYPE_HERMITIAN,        // Hermitian matrix
    MATRIX_TYPE_POSITIVE_DEFINITE, // Positive definite
    MATRIX_TYPE_DIAGONALLY_DOMINANT // Diagonally dominant
} matrix_type_t;

// Preconditioner types (moved before solver_config_t)
typedef enum {
    PRECONDITIONER_ILU,           // Incomplete LU
    PRECONDITIONER_IC,            // Incomplete Cholesky
    PRECONDITIONER_JACOBI,       // Jacobi
    PRECONDITIONER_GAUSS_SEIDEL,  // Gauss-Seidel
    PRECONDITIONER_SSOR,         // Symmetric SOR
    PRECONDITIONER_MULTIGRID,    // Multigrid
    PRECONDITIONER_DOMAIN_DECOMPOSITION // Domain decomposition
} preconditioner_type_t;

// Solver configuration
typedef struct {
    solver_type_t type;
    solver_backend_t backend;
    solver_precision_t precision;
    
    // Convergence criteria
    double tolerance;
    int max_iterations;
    int restart_size;             // For GMRES
    
    // Preconditioning
    bool use_preconditioner;
    preconditioner_type_t preconditioner_type;
    double preconditioner_drop_tolerance;
    int preconditioner_fill_level;
    
    // Parallel processing
    bool use_parallel;
    int num_threads;
    int num_processes;
    
    // GPU acceleration
    bool use_gpu;
    int gpu_device_id;
    
    // Memory management
    bool use_out_of_core;         // For large problems
    size_t memory_limit;
    
    // Output control
    bool verbose;
    int output_frequency;
    
    // Circuit-specific (PEEC)
    bool use_sparse_matrix;
    bool use_symmetric_solver;
    bool enforce_passivity;
} solver_config_t;

// Solver statistics
typedef struct {
    int iterations;
    double residual_norm;
    double solve_time;
    double setup_time;
    size_t memory_usage;
    int num_restarts;
    bool converged;
    char convergence_reason[256];
} solver_statistics_t;

// Solution vector
typedef struct {
    double* data;                 // Complex data (real, imag pairs)
    int length;
    bool is_complex;
    
    // For circuit solutions (PEEC)
    int num_nodes;
    int num_branches;
    double* node_voltages;
    double* branch_currents;
    
    // For electromagnetic solutions (MoM)
    int num_basis_functions;
    double* basis_coefficients;
    
    // Frequency domain data
    double frequency;
    bool is_frequency_domain;
} solution_vector_t;

// Matrix system
typedef struct {
    // Matrix data
    assembly_type_t matrix_type;
    int num_rows;
    int num_cols;
    int num_nonzeros;
    
    // Dense storage
    complex_t* dense_matrix;         // Complex values
    
    // Sparse storage (CSR)
    int* row_ptr;
    int* col_ind;
    complex_t* sparse_values;
    
    // Block storage (for ACA)
    int num_blocks;
    complex_t** block_matrices;
    int* block_rows;
    int* block_cols;
    
    // Compressed storage (ACA)
    complex_t** U_matrices;
    complex_t** V_matrices;
    int* ranks;
    
    // Right-hand side
    complex_t* rhs;                  // Complex RHS
    int num_rhs;
    
    // Matrix properties
    matrix_type_t properties;
    bool is_factorized;
    
    // Factorization data
    void* factorization_data;
    
    // Performance data
    double condition_number;
    double norm;
} matrix_system_t;

/*********************************************************************
 * Unified Solver Interface
 *********************************************************************/

// Linear solver structure
typedef struct linear_solver {
    int solver_type;           // Solver algorithm type
    int matrix_type;           // Matrix storage type
    int matrix_size;           // Size of the matrix system
    int num_rhs;               // Number of right-hand sides
    int converged;             // Convergence flag
    
    solver_config_t config;    // Solver configuration
    void* solver_data;         // Solver-specific data
    matrix_system_t* matrix_system; // Matrix system reference
} linear_solver_t;

// Solver creation and configuration
linear_solver_t* linear_solver_create(const solver_config_t* config);
void linear_solver_destroy(linear_solver_t* solver);

// Matrix setup
int linear_solver_set_matrix(linear_solver_t* solver, matrix_system_t* matrix);
int linear_solver_set_rhs(linear_solver_t* solver, const solution_vector_t* rhs);

// Solution
int linear_solver_solve(linear_solver_t* solver, solution_vector_t* solution);
int linear_solver_solve_multiple_rhs(linear_solver_t* solver, 
                                     solution_vector_t** solutions, int num_rhs);

// Factorization
int linear_solver_factorize(linear_solver_t* solver);
int linear_solver_refactorize(linear_solver_t* solver);  // For updated matrices

// Solution queries
int linear_solver_get_solution(linear_solver_t* solver, solution_vector_t* solution);
solver_statistics_t linear_solver_get_statistics(const linear_solver_t* solver);

// Performance monitoring
double linear_solver_get_memory_usage(const linear_solver_t* solver);
double linear_solver_get_computation_time(const linear_solver_t* solver);

/*********************************************************************
 * Specialized Solvers
 *********************************************************************/

// Dense matrix solvers
linear_solver_t* dense_lu_solver_create(const solver_config_t* config);
linear_solver_t* dense_qr_solver_create(const solver_config_t* config);

// Sparse matrix solvers
linear_solver_t* sparse_lu_solver_create(const solver_config_t* config);
linear_solver_t* sparse_cholesky_solver_create(const solver_config_t* config);

// Iterative solvers
linear_solver_t* gmres_solver_create(const solver_config_t* config);
linear_solver_t* conjugate_gradient_solver_create(const solver_config_t* config);
linear_solver_t* bicgstab_solver_create(const solver_config_t* config);

// Circuit solvers (PEEC)
linear_solver_t* mna_solver_create(const solver_config_t* config);
linear_solver_t* state_space_solver_create(const solver_config_t* config);

/*********************************************************************
 * Preconditioner Interface
 *********************************************************************/

// Preconditioner type constants for backward compatibility
#define PRECON_NONE 0
#define PRECON_JACOBI 1
#define PRECON_GAUSS_SEIDEL 2
#define PRECON_SSOR 3
#define PRECON_ILU 4
#define PRECON_IC 5
#define PRECON_MULTIGRID 6

typedef struct preconditioner preconditioner_t;

// Preconditioner structure
struct preconditioner {
    preconditioner_type_t type;
    solver_config_t config;
    void* data;
    matrix_system_t* matrix;
};

preconditioner_t* preconditioner_create(preconditioner_type_t type, 
                                     const solver_config_t* config);
void preconditioner_destroy(preconditioner_t* preconditioner);

int preconditioner_set_matrix(preconditioner_t* preconditioner, 
                            matrix_system_t* matrix);
int preconditioner_apply(preconditioner_t* preconditioner, 
                      const solution_vector_t* rhs, solution_vector_t* result);

// Specialized preconditioners
preconditioner_t* ilu_preconditioner_create(const solver_config_t* config);
preconditioner_t* ic_preconditioner_create(const solver_config_t* config);
preconditioner_t* jacobi_preconditioner_create(const solver_config_t* config);
preconditioner_t* multigrid_preconditioner_create(const solver_config_t* config);

/*********************************************************************
 * Eigenvalue Solver Interface
 *********************************************************************/

typedef struct eigen_solver eigen_solver_t;

typedef enum {
    EIGEN_TYPE_STANDARD,          // Ax = λx
    EIGEN_TYPE_GENERALIZED,       // Ax = λBx
    EIGEN_TYPE_QUADRATIC,         // λ²A + λB + C = 0
    EIGEN_TYPE_NONLINEAR          // Nonlinear eigenvalue problem
} eigen_type_t;

typedef struct {
    eigen_type_t type;
    int num_eigenvalues;
    int num_arnoldi_vectors;
    double tolerance;
    int max_iterations;
    bool use_shift_invert;
    double shift_value;
    bool compute_eigenvectors;
} eigen_config_t;

typedef struct {
    double* eigenvalues;            // Complex eigenvalues
    double* eigenvectors;           // Complex eigenvectors
    int num_eigenvalues;
    int num_eigenvectors;
    bool converged;
} eigen_result_t;

eigen_solver_t* eigen_solver_create(const eigen_config_t* config);
void eigen_solver_destroy(eigen_solver_t* solver);

int eigen_solver_set_matrix(eigen_solver_t* solver, matrix_system_t* matrix);
int eigen_solver_set_b_matrix(eigen_solver_t* solver, matrix_system_t* b_matrix);
int eigen_solver_solve(eigen_solver_t* solver, eigen_result_t* result);
void eigen_result_free(eigen_result_t* result);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

// Matrix operations
int solver_matrix_multiply(const matrix_system_t* A, const solution_vector_t* x, 
                          solution_vector_t* y);
int solver_matrix_transpose_multiply(const matrix_system_t* A, const solution_vector_t* x,
                                   solution_vector_t* y);

// Vector operations
int solver_vector_add(const solution_vector_t* x, const solution_vector_t* y,
                     solution_vector_t* result);
int solver_vector_scale(const solution_vector_t* x, double scale,
                       solution_vector_t* result);
double solver_vector_dot(const solution_vector_t* x, const solution_vector_t* y);
double solver_vector_norm(const solution_vector_t* x);

// Solution analysis
double solver_compute_residual(const matrix_system_t* A, const solution_vector_t* x,
                              const solution_vector_t* b);
double solver_estimate_condition_number(const matrix_system_t* A);

// Memory management
size_t solver_estimate_memory_usage(int num_rows, int num_cols, 
                                   assembly_type_t matrix_type);

#ifdef __cplusplus
}
#endif

#endif // CORE_SOLVER_H