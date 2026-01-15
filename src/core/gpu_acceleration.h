#ifndef GPU_ACCELERATION_H
#define GPU_ACCELERATION_H

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <cuComplex.h>

// GPU memory management for complex numbers
typedef cuDoubleComplex gpu_complex;

// Convert between CPU and GPU complex types
__host__ __device__ static __inline__ gpu_complex make_gpu_complex(double r, double i) {
    return make_cuDoubleComplex(r, i);
}

__host__ __device__ static __inline__ double gpu_complex_real(gpu_complex x) {
    return cuCreal(x);
}

__host__ __device__ static __inline__ double gpu_complex_imag(gpu_complex x) {
    return cuCimag(x);
}

// GPU kernel for Sommerfeld integral computation
__global__ void sommerfeld_integral_kernel(
    const double *krho_points,
    const double *weights,
    const double *z_coords,
    const double *zp_coords,
    const int *layer_indices,
    const double *medium_params,
    const double omega,
    const int n_points,
    const int n_layers,
    gpu_complex *results
);

// GPU kernel for Green's function matrix assembly
__global__ void greens_function_matrix_kernel(
    const double *x_coords,
    const double *y_coords,
    const double *z_coords,
    const double *xp_coords,
    const double *yp_coords,
    const double *zp_coords,
    const int *layer_src_indices,
    const int *layer_obs_indices,
    const double *medium_params,
    const double omega,
    const int n_source_points,
    const int n_obs_points,
    const int n_layers,
    gpu_complex *green_matrix
);

// GPU kernel for H-matrix compression using ACA
__global__ void aca_compression_kernel(
    const gpu_complex *matrix_block,
    const int block_rows,
    const int block_cols,
    const double tolerance,
    const int max_rank,
    gpu_complex *U_factor,
    gpu_complex *V_factor,
    int *actual_rank,
    double *compression_error
);

// GPU kernel for matrix-vector multiplication
__global__ void matrix_vector_multiply_kernel(
    const gpu_complex *matrix,
    const gpu_complex *vector,
    const int rows,
    const int cols,
    gpu_complex *result
);

// GPU kernel for iterative solver (GMRES)
__global__ void gmres_iteration_kernel(
    const gpu_complex *A_matrix,
    const gpu_complex *b_vector,
    gpu_complex *x_vector,
    const int matrix_size,
    const int iteration,
    const double tolerance,
    double *residual_norm,
    bool *converged
);

// CUDA error checking macro
#define CUDA_CHECK(err) \
    do { \
        cudaError_t err_ = (err); \
        if (err_ != cudaSuccess) { \
            fprintf(stderr, "CUDA error: %s (%d) at %s:%d\n", \
                    cudaGetErrorString(err_), err_, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

// CUBLAS error checking macro
#define CUBLAS_CHECK(err) \
    do { \
        cublasStatus_t err_ = (err); \
        if (err_ != CUBLAS_STATUS_SUCCESS) { \
            fprintf(stderr, "CUBLAS error: %d at %s:%d\n", \
                    err_, __FILE__, __LINE__); \
            exit(1); \
        } \
    } while (0)

// GPU context for managing CUDA resources
typedef struct {
    int device_id;
    cudaDeviceProp device_properties;
    cublasHandle_t cublas_handle;
    cusolverDnHandle_t cusolver_handle;
    
    // Memory pools
    void *device_memory_pool;
    size_t memory_pool_size;
    size_t memory_pool_used;
    
    // Stream for asynchronous operations
    cudaStream_t compute_stream;
    cudaStream_t memory_stream;
    
    // Performance metrics
    double memory_bandwidth;
    double compute_throughput;
    int max_threads_per_block;
    int max_blocks_per_grid;
} GPUContext;

// Initialize GPU context
GPUContext* initialize_gpu_context(int device_id);

// Cleanup GPU context
void cleanup_gpu_context(GPUContext *context);

// Allocate GPU memory with alignment
void* gpu_malloc_aligned(GPUContext *context, size_t size, size_t alignment);

// Free GPU memory
void gpu_free_aligned(GPUContext *context, void *ptr);

// Copy data to GPU with async transfer
void gpu_memcpy_to_device_async(GPUContext *context, void *dst, const void *src, size_t size);

// Copy data from GPU with async transfer
void gpu_memcpy_from_device_async(GPUContext *context, void *dst, const void *src, size_t size);

// Synchronize GPU operations
void gpu_synchronize(GPUContext *context);

// Get optimal block size for kernel
int get_optimal_block_size(GPUContext *context, const char *kernel_name);

// Get optimal grid size for kernel
int get_optimal_grid_size(GPUContext *context, int data_size, int block_size);

// GPU-accelerated Green's function computation
void gpu_greens_function_computation(
    GPUContext *context,
    const double *krho_points,
    const double *weights,
    const double *z_coords,
    const double *zp_coords,
    const int *layer_indices,
    const double *medium_params,
    const double omega,
    const int n_points,
    const int n_layers,
    gpu_complex *results
);

// GPU-accelerated H-matrix compression
void gpu_hmatrix_compression(
    GPUContext *context,
    const gpu_complex *matrix,
    const int rows,
    const int cols,
    const double tolerance,
    const int max_rank,
    gpu_complex *U,
    gpu_complex *V,
    int *rank,
    double *error
);

// GPU-accelerated matrix-vector multiplication
void gpu_matrix_vector_multiply(
    GPUContext *context,
    const gpu_complex *matrix,
    const gpu_complex *vector,
    const int rows,
    const int cols,
    gpu_complex *result
);

// GPU-accelerated iterative solver
void gpu_iterative_solver(
    GPUContext *context,
    const gpu_complex *A_matrix,
    const gpu_complex *b_vector,
    gpu_complex *x_vector,
    const int matrix_size,
    const int max_iterations,
    const double tolerance,
    int *iterations,
    double *final_residual,
    bool *converged
);

// Multi-GPU support
typedef struct {
    int n_gpus;
    GPUContext **gpu_contexts;
    int *work_distribution;
    double *performance_balancing;
} MultiGPUContext;

// Initialize multi-GPU context
MultiGPUContext* initialize_multi_gpu(int n_gpus);

// Cleanup multi-GPU context
void cleanup_multi_gpu(MultiGPUContext *context);

// Distribute work across multiple GPUs
void distribute_work_multi_gpu(
    MultiGPUContext *context,
    const void *data,
    const size_t data_size,
    void **distributed_data,
    size_t **distributed_sizes
);

// Collect results from multiple GPUs
void collect_results_multi_gpu(
    MultiGPUContext *context,
    void **results,
    const size_t *result_sizes,
    void *final_result
);

#endif // ENABLE_CUDA

#endif // GPU_ACCELERATION_H