#ifndef GPU_LINALG_OPTIMIZATION_H
#define GPU_LINALG_OPTIMIZATION_H

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <cusparse.h>
#include "gpu_acceleration.h"

// Advanced cuBLAS operations for electromagnetic simulations
typedef struct {
    cublasHandle_t cublas_handle;
    cusolverDnHandle_t cusolver_handle;
    cusparseHandle_t cusparse_handle;
    cusparseMatDescr_t matrix_descriptor;
    
    // Preconditioners
    gpu_complex *ilu_factors;
    gpu_complex *preconditioner_matrix;
    int *preconditioner_pivots;
    
    // Batch operations
    int batch_size;
    gpu_complex **batch_matrices;
    gpu_complex **batch_vectors;
    
    // Performance tuning
    int optimal_block_size;
    int optimal_grid_size;
    double performance_threshold;
} GPULinalgContext;

// Initialize GPU linear algebra context
GPULinalgContext* initialize_gpu_linalg_context(int device_id);

// Cleanup GPU linear algebra context
void cleanup_gpu_linalg_context(GPULinalgContext *context);

// Optimized matrix-matrix multiplication for impedance matrices
void gpu_zgemm_optimized(
    GPULinalgContext *context,
    const gpu_complex *A, const gpu_complex *B,
    gpu_complex *C,
    const int m, const int n, const int k,
    const gpu_complex alpha, const gpu_complex beta,
    const bool transpose_A, const bool transpose_B
);

// Optimized matrix-vector multiplication for MoM operations
void gpu_zgemv_optimized(
    GPULinalgContext *context,
    const gpu_complex *A, const gpu_complex *x,
    gpu_complex *y,
    const int m, const int n,
    const gpu_complex alpha, const gpu_complex beta,
    const bool transpose_A
);

// Batch matrix operations for multiple frequency points
void gpu_batch_zgemm(
    GPULinalgContext *context,
    const gpu_complex **A_array, const gpu_complex **B_array,
    gpu_complex **C_array,
    const int batch_size,
    const int m, const int n, const int k,
    const gpu_complex alpha, const gpu_complex beta
);

// Sparse matrix operations for compressed impedance matrices
void gpu_sparse_matrix_multiply(
    GPULinalgContext *context,
    const cusparseSpMatDescr_t sparse_A,
    const cusparseDnVecDescr_t dense_x,
    cusparseDnVecDescr_t dense_y,
    const gpu_complex alpha, const gpu_complex beta
);

// LU factorization with partial pivoting for direct solvers
int gpu_zgetrf_optimized(
    GPULinalgContext *context,
    gpu_complex *A, const int n,
    int *ipiv, gpu_complex *work, const int lwork
);

// Triangular solve for forward/backward substitution
void gpu_zgetrs_optimized(
    GPULinalgContext *context,
    const gpu_complex *A, const int n,
    const int *ipiv, const gpu_complex *b,
    gpu_complex *x, const int nrhs,
    const bool transpose
);

// QR factorization for least squares problems
int gpu_zgeqrf_optimized(
    GPULinalgContext *context,
    gpu_complex *A, const int m, const int n,
    gpu_complex *tau, gpu_complex *work, const int lwork
);

// Eigenvalue decomposition for resonance analysis
int gpu_zheevd_optimized(
    GPULinalgContext *context,
    gpu_complex *A, const int n,
    double *eigenvalues, gpu_complex *work, const int lwork,
    double *rwork, const int lrwork, int *iwork, const int liwork
);

// Singular Value Decomposition (SVD) for matrix conditioning
int gpu_zgesvd_optimized(
    GPULinalgContext *context,
    gpu_complex *A, const int m, const int n,
    double *singular_values,
    gpu_complex *U, const int ldu,
    gpu_complex *VT, const int ldvt,
    gpu_complex *work, const int lwork, double *rwork
);

// Preconditioned iterative solver with ILU preconditioning
void gpu_preconditioned_gmres(
    GPULinalgContext *context,
    const gpu_complex *A, const gpu_complex *b,
    gpu_complex *x, const int n,
    const int max_iterations, const double tolerance,
    int *iterations, double *final_residual, bool *converged
);

// Mixed-precision iterative solver for memory efficiency
void gpu_mixed_precision_solver(
    GPULinalgContext *context,
    const gpu_complex *A, const gpu_complex *b,
    gpu_complex *x, const int n,
    const int max_iterations, const double tolerance,
    int *iterations, double *final_residual, bool *converged
);

// Block iterative solver for multiple right-hand sides
void gpu_block_gmres(
    GPULinalgContext *context,
    const gpu_complex *A, const gpu_complex *B,
    gpu_complex *X, const int n, const int nrhs,
    const int max_iterations, const double tolerance,
    int *iterations, double *final_residual, bool *converged
);

// GPU-accelerated H-matrix operations
void gpu_hmatrix_vector_product(
    GPULinalgContext *context,
    const void *hmatrix_structure,
    const gpu_complex *x, gpu_complex *y,
    const int n
);

// Advanced memory management with unified memory
void* gpu_malloc_managed(size_t size, int device_id);
void gpu_free_managed(void *ptr);
void gpu_prefetch_managed(void *ptr, size_t size, int device_id);

// Performance profiling and optimization
void gpu_linalg_profile_operation(
    GPULinalgContext *context,
    const char *operation_name,
    double *execution_time,
    double *memory_bandwidth,
    double *compute_throughput
);

// Auto-tuning for optimal block sizes
void gpu_auto_tune_block_size(
    GPULinalgContext *context,
    const char *kernel_name,
    int *optimal_block_size,
    int *optimal_grid_size
);

// Error checking and numerical stability
void gpu_check_numerical_stability(
    GPULinalgContext *context,
    const gpu_complex *matrix, const int n,
    double *condition_number,
    double *numerical_rank,
    bool *is_well_conditioned
);

// Mixed CPU-GPU hybrid algorithms
void gpu_cpu_hybrid_solver(
    GPULinalgContext *context,
    const gpu_complex *A, const gpu_complex *b,
    gpu_complex *x, const int n,
    const int cpu_threshold,
    const int max_iterations, const double tolerance,
    int *iterations, double *final_residual, bool *converged
);

// Advanced preconditioning techniques
typedef enum {
    PRECOND_NONE,
    PRECOND_JACOBI,
    PRECOND_ILU,
    PRECOND_ILUT,
    PRECOND_BLOCK_JACOBI,
    PRECOND_MULTIGRID,
    PRECOND_DOMAIN_DECOMPOSITION
} PreconditionerType;

void gpu_setup_preconditioner(
    GPULinalgContext *context,
    const gpu_complex *A, const int n,
    PreconditionerType precond_type,
    const double drop_tolerance,
    const int fill_level
);

void gpu_apply_preconditioner(
    GPULinalgContext *context,
    const gpu_complex *r, gpu_complex *z, const int n
);

// cuSPARSE integration for sparse matrix operations
typedef struct {
    cusparseSpMatDescr_t matrix;
    cusparseDnVecDescr_t input_vector;
    cusparseDnVecDescr_t output_vector;
    size_t buffer_size;
    void *external_buffer;
} GPUSparseContext;

GPUSparseContext* initialize_gpu_sparse_context(
    GPULinalgContext *linalg_context,
    const int *row_ptr, const int *col_ind, 
    const gpu_complex *values, const int n_rows, const int n_cols, const int nnz
);

void cleanup_gpu_sparse_context(GPUSparseContext *context);

void gpu_sparse_matrix_analysis(
    GPUSparseContext *context,
    cusparseSpSMDescr_t sm_desc,
    const cusparseOperation_t operation
);

// Advanced numerical linear algebra utilities
gpu_complex gpu_matrix_determinant(
    GPULinalgContext *context,
    gpu_complex *A, const int n
);

double gpu_matrix_trace(
    GPULinalgContext *context,
    const gpu_complex *A, const int n
);

void gpu_matrix_inverse(
    GPULinalgContext *context,
    const gpu_complex *A, gpu_complex *A_inv, const int n
);

// Spectral analysis for electromagnetic problems
void gpu_spectral_analysis(
    GPULinalgContext *context,
    const gpu_complex *A, const int n,
    double *eigenvalues_real, double *eigenvalues_imag,
    const int num_eigenvalues,
    const double frequency_range[2]
);

// Resonance detection and mode analysis
typedef struct {
    double frequency;
    double quality_factor;
    gpu_complex *mode_pattern;
    double mode_amplitude;
} ResonanceMode;

int gpu_find_resonance_modes(
    GPULinalgContext *context,
    const gpu_complex *impedance_matrix, const int n,
    ResonanceMode *modes, const int max_modes,
    const double frequency_min, const double frequency_max
);

#endif // ENABLE_CUDA

#endif // GPU_LINALG_OPTIMIZATION_H