/*****************************************************************************************
 * Math Backend Selector for Industrial-Grade Numerical Libraries
 * 
 * Provides unified interface to multiple numerical backends:
 * - OpenBLAS/MKL for dense linear algebra
 * - PETSc/Eigen for sparse systems
 * - H2Lib/HLIBpro for H-matrix compression
 * - cuBLAS/rocBLAS for GPU acceleration
 *****************************************************************************************/

#ifndef MATH_BACKEND_SELECTOR_H
#define MATH_BACKEND_SELECTOR_H

#include <complex.h>
#include <stddef.h>

// Backend selection
typedef enum {
    BACKEND_OPENBLAS,      // Open source, cross-platform
    BACKEND_MKL,           // Intel MKL (fastest on Intel)
    BACKEND_BLIS,          // Lightweight, portable
    BACKEND_CUBLAS,        // NVIDIA GPU
    BACKEND_ROCBLAS,       // AMD GPU
    BACKEND_ONEMKL,        // Intel oneAPI (cross-platform)
    BACKEND_PETSC,         // Sparse systems
    BACKEND_EIGEN,         // C++ template library
    BACKEND_GINKGO,        // Modern GPU/CPU solver
    BACKEND_H2LIB,         // H-matrix library
    BACKEND_HLIBPRO        // Commercial H-matrix
} math_backend_t;

// Matrix storage formats
typedef enum {
    MATRIX_FORMAT_DENSE,        // Standard dense matrix
    MATRIX_FORMAT_CSR,          // Compressed Sparse Row
    MATRIX_FORMAT_CSC,          // Compressed Sparse Column
    MATRIX_FORMAT_BSR,          // Block Sparse Row
    MATRIX_FORMAT_HMATRIX,      // Hierarchical matrix
    MATRIX_FORMAT_H2MATRIX      // H^2 matrix (more compressed)
} matrix_format_t;

// Precision types
typedef enum {
    PRECISION_SINGLE,           // float
    PRECISION_DOUBLE,           // double
    PRECISION_SINGLE_COMPLEX, // float complex
    PRECISION_DOUBLE_COMPLEX  // double complex
} precision_t;

// Unified matrix descriptor
typedef struct {
    void* data;                 // Backend-specific matrix pointer
    matrix_format_t format;     // Storage format
    precision_t precision;      // Numerical precision
    size_t rows, cols;          // Matrix dimensions
    size_t nnz;                 // Non-zero elements (for sparse)
    math_backend_t backend;     // Selected backend
    int device_id;              // GPU device ID (-1 for CPU)
} matrix_handle_t;

// Unified vector descriptor
typedef struct {
    void* data;                 // Backend-specific vector pointer
    precision_t precision;      // Numerical precision
    size_t size;                // Vector length
    math_backend_t backend;     // Selected backend
    int device_id;              // GPU device ID (-1 for CPU)
} vector_handle_t;

// Solver parameters
typedef struct {
    int max_iterations;         // Maximum iterations
    double tolerance;           // Convergence tolerance
    int restart_size;           // GMRES restart size
    int preconditioner_type;    // Preconditioner selection
    int verbose;                // Debug output level
    int use_gpu;               // Enable GPU acceleration
    int num_threads;           // CPU thread count
} solver_params_t;

// Backend initialization
int math_backend_init(math_backend_t backend, int device_id);
int math_backend_cleanup(math_backend_t backend);
const char* math_backend_get_name(math_backend_t backend);
int math_backend_get_capabilities(math_backend_t backend);

// Matrix operations
matrix_handle_t* matrix_create(math_backend_t backend, matrix_format_t format, 
                              precision_t precision, size_t rows, size_t cols);
void matrix_destroy(matrix_handle_t* matrix);
int matrix_set_data(matrix_handle_t* matrix, const void* data, size_t nnz);
int matrix_get_data(const matrix_handle_t* matrix, void* data);

// Level 1 BLAS operations
double matrix_norm(const matrix_handle_t* matrix, int norm_type);
double vector_norm(const vector_handle_t* vector, int norm_type);

// Level 2 BLAS operations
int matrix_vector_multiply(const matrix_handle_t* matrix, const vector_handle_t* x, 
                          vector_handle_t* y, double alpha, double beta);

// Level 3 BLAS operations
int matrix_matrix_multiply(const matrix_handle_t* a, const matrix_handle_t* b, 
                          matrix_handle_t* c, double alpha, double beta);

// Linear system solvers
int solver_dense_lu(const matrix_handle_t* matrix, const vector_handle_t* b, 
                   vector_handle_t* x, solver_params_t* params);
int solver_dense_qr(const matrix_handle_t* matrix, const vector_handle_t* b, 
                   vector_handle_t* x, solver_params_t* params);
int solver_sparse_gmres(const matrix_handle_t* matrix, const vector_handle_t* b, 
                       vector_handle_t* x, solver_params_t* params);
int solver_sparse_cg(const matrix_handle_t* matrix, const vector_handle_t* b, 
                    vector_handle_t* x, solver_params_t* params);
int solver_sparse_bicgstab(const matrix_handle_t* matrix, const vector_handle_t* b, 
                          vector_handle_t* x, solver_params_t* params);

// H-matrix operations (when H2Lib/HLIBpro available)
#ifdef ENABLE_HMATRIX
matrix_handle_t* matrix_create_hmatrix(math_backend_t backend, size_t rows, size_t cols,
                                      double epsilon, int cluster_size);
int matrix_compress_hmatrix(matrix_handle_t* matrix, double epsilon);
int solver_hmatrix_gmres(const matrix_handle_t* matrix, const vector_handle_t* b, 
                          vector_handle_t* x, solver_params_t* params);
#endif

// GPU acceleration
typedef struct {
    int device_id;
    size_t memory_total;
    size_t memory_free;
    int multiprocessor_count;
    int max_threads_per_block;
    int max_block_size;
} gpu_info_t;

int gpu_get_device_count(void);
int gpu_get_device_info(int device_id, gpu_info_t* info);
int gpu_set_device(int device_id);
int gpu_memory_allocate(size_t bytes, void** ptr);
int gpu_memory_free(void* ptr);
int gpu_copy_to_device(const void* host_ptr, void* device_ptr, size_t bytes);
int gpu_copy_to_host(const void* device_ptr, void* host_ptr, size_t bytes);

// Performance monitoring
typedef struct {
    double flops;
    double memory_bandwidth;
    double execution_time;
    int num_operations;
    size_t memory_used;
} performance_stats_t;

int performance_get_stats(math_backend_t backend, performance_stats_t* stats);
void performance_reset_stats(math_backend_t backend);

// Error handling
typedef enum {
    MATH_SUCCESS = 0,
    MATH_ERROR_INVALID_BACKEND = -1,
    MATH_ERROR_OUT_OF_MEMORY = -2,
    MATH_ERROR_SINGULAR_MATRIX = -3,
    MATH_ERROR_CONVERGENCE_FAILED = -4,
    MATH_ERROR_INVALID_PARAMETER = -5,
    MATH_ERROR_GPU_NOT_AVAILABLE = -6,
    MATH_ERROR_LIBRARY_NOT_FOUND = -7
} math_error_t;

const char* math_error_get_string(math_error_t error);
void math_error_set_handler(void (*handler)(math_error_t, const char*));

// Convenience macros for common operations
#define MATRIX_VECTOR_MUL(backend, A, x, y) \
    matrix_vector_multiply(A, x, y, 1.0, 0.0)

#define MATRIX_VECTOR_MUL_ADD(backend, A, x, y, alpha, beta) \
    matrix_vector_multiply(A, x, y, alpha, beta)

#define SOLVE_GMRES(backend, A, b, x, params) \
    solver_sparse_gmres(A, b, x, params)

// Thread safety
int math_backend_set_thread_safe(math_backend_t backend, int thread_safe);
int math_backend_get_num_threads(math_backend_t backend);

#endif // MATH_BACKEND_SELECTOR_H