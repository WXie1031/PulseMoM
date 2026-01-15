#ifndef GPU_PARALLELIZATION_OPTIMIZED_H
#define GPU_PARALLELIZATION_OPTIMIZED_H

#ifdef ENABLE_CUDA

#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <cooperative_groups.h>
#include "gpu_acceleration.h"

namespace cg = cooperative_groups;

// Optimized CUDA kernel configurations for maximum performance
#define OPTIMIZED_BLOCK_SIZE 256
#define OPTIMIZED_GRID_SIZE_MULTIPLIER 4
#define WARP_SIZE 32
#define MAX_SHARED_MEMORY 48*1024  // 48KB shared memory

// Memory access patterns optimization
typedef struct {
    int tile_size_m;
    int tile_size_n;
    int tile_size_k;
    int threads_per_block;
    int shared_memory_size;
    bool use_texture_memory;
    bool use_constant_memory;
    int vector_width;
} KernelOptimizationParams;

// Optimized Sommerfeld integral kernel with coalesced memory access
__global__ void optimized_sommerfeld_integral_kernel(
    const double * __restrict__ krho_points,
    const double * __restrict__ weights,
    const double * __restrict__ z_coords,
    const double * __restrict__ zp_coords,
    const int * __restrict__ layer_indices,
    const double * __restrict__ medium_params,
    const double omega,
    const int n_points,
    const int n_layers,
    cuDoubleComplex * __restrict__ results
);

// Optimized Green's function matrix kernel with shared memory tiling
__global__ void optimized_greens_function_matrix_kernel(
    const double * __restrict__ x_coords,
    const double * __restrict__ y_coords,
    const double * __restrict__ z_coords,
    const double * __restrict__ xp_coords,
    const double * __restrict__ yp_coords,
    const double * __restrict__ zp_coords,
    const int * __restrict__ layer_src_indices,
    const int * __restrict__ layer_obs_indices,
    const double * __restrict__ medium_params,
    const double omega,
    const int n_source_points,
    const int n_obs_points,
    const int n_layers,
    cuDoubleComplex * __restrict__ green_matrix,
    const KernelOptimizationParams params
);

// Optimized ACA compression with warp-level primitives
__global__ void optimized_aca_compression_kernel(
    const cuDoubleComplex * __restrict__ matrix_block,
    const int block_rows,
    const int block_cols,
    const double tolerance,
    const int max_rank,
    cuDoubleComplex * __restrict__ U_factor,
    cuDoubleComplex * __restrict__ V_factor,
    int * __restrict__ actual_rank,
    double * __restrict__ compression_error,
    const KernelOptimizationParams params
);

// Optimized matrix-vector multiplication with vectorized operations
__global__ void optimized_matrix_vector_multiply_kernel(
    const cuDoubleComplex * __restrict__ matrix,
    const cuDoubleComplex * __restrict__ vector,
    const int rows,
    const int cols,
    cuDoubleComplex * __restrict__ result,
    const int vector_width
);

// Optimized GMRES with reduced synchronization
__global__ void optimized_gmres_kernel(
    const cuDoubleComplex * __restrict__ A_matrix,
    const cuDoubleComplex * __restrict__ b_vector,
    cuDoubleComplex * __restrict__ x_vector,
    const int matrix_size,
    const int restart,
    const double tolerance,
    double * __restrict__ residual_norm,
    bool * __restrict__ converged,
    cuDoubleComplex * __restrict__ workspace,
    const int workspace_size
);

// Advanced memory access optimization with texture memory
texture<double, 1, cudaReadModeElementType> tex_krho;
texture<double, 1, cudaReadModeElementType> tex_weights;
texture<double, 1, cudaReadModeElementType> tex_z_coords;
texture<double, 1, cudaReadModeElementType> tex_zp_coords;

// Constant memory for frequently accessed parameters
__constant__ double const_medium_params[64];
__constant__ double const_omega;
__constant__ int const_n_layers;

// Optimized memory copy with async transfers and streams
void optimized_gpu_memory_copy_async(
    void *dst, const void *src, size_t size,
    cudaStream_t stream, int device_id
);

// Optimized kernel launch configuration
dim3 calculate_optimal_grid_size(
    int data_size,
    int block_size,
    int max_blocks_per_sm,
    int num_sms
);

// Occupancy optimization
KernelOptimizationParams calculate_optimal_kernel_params(
    const cudaDeviceProp *device_props,
    const char *kernel_name,
    int matrix_rows,
    int matrix_cols,
    int shared_memory_per_thread
);

// Warp-level reduction for improved performance
__device__ __forceinline__ cuDoubleComplex warp_reduce_sum_complex(
    cuDoubleComplex value,
    unsigned int mask
);

// Shared memory optimized for bank conflicts
__shared__ union {
    cuDoubleComplex complex_data[OPTIMIZED_BLOCK_SIZE];
    double double_data[OPTIMIZED_BLOCK_SIZE * 2];
    int int_data[OPTIMIZED_BLOCK_SIZE * 2];
} shared_memory_union;

// Vectorized memory operations
__device__ __forceinline__ void vectorized_load_double4(
    const double * __restrict__ ptr,
    double4 *registers,
    int offset
);

__device__ __forceinline__ void vectorized_store_double4(
    double * __restrict__ ptr,
    const double4 *registers,
    int offset
);

// Prefetching for latency hiding
__device__ __forceinline__ void prefetch_data_l2(
    const void *ptr,
    int prefetch_size
);

// Optimized thread block configuration
typedef struct {
    int warps_per_block;
    int threads_per_warp;
    int shared_memory_per_block;
    int registers_per_thread;
    int max_blocks_per_sm;
} ThreadBlockConfig;

ThreadBlockConfig calculate_optimal_thread_block_config(
    const cudaDeviceProp *device_props,
    int shared_memory_needed,
    int registers_needed
);

// Performance monitoring and auto-tuning
typedef struct {
    double execution_time;
    double memory_throughput;
    double compute_throughput;
    double occupancy;
    int achieved_occupancy;
    int theoretical_occupancy;
    double warp_efficiency;
    double global_load_efficiency;
    double shared_load_efficiency;
} KernelPerformanceMetrics;

void measure_kernel_performance(
    const char *kernel_name,
    KernelPerformanceMetrics *metrics,
    cudaStream_t stream
);

// Auto-tuning for optimal performance
void auto_tune_kernel_parameters(
    const char *kernel_name,
    KernelOptimizationParams *params,
    int matrix_size,
    int num_iterations
);

// Advanced parallelization strategies
typedef enum {
    PARALLELIZATION_STRATEGY_1D,
    PARALLELIZATION_STRATEGY_2D_TILE,
    PARALLELIZATION_STRATEGY_WARP_SPECIALIZED,
    PARALLELIZATION_STRATEGY_COOPERATIVE_GROUPS,
    PARALLELIZATION_STRATEGY_DYNAMIC_PARALLELISM
} ParallelizationStrategy;

// Cooperative groups implementation
device__ void cooperative_greens_function_computation(
    cg::thread_block_tile<WARP_SIZE> &tile,
    const double * __restrict__ coords,
    cuDoubleComplex * __restrict__ results,
    const int tile_id,
    const int num_tiles
);

// Dynamic parallelism for adaptive workload
device__ void launch_child_kernel_for_block(
    const cuDoubleComplex * __restrict__ parent_data,
    cuDoubleComplex * __restrict__ child_results,
    const int parent_size,
    const int child_size
);

// Memory bandwidth optimization
void optimize_memory_bandwidth_usage(
    cuDoubleComplex *device_ptr,
    size_t size,
    int memory_access_pattern
);

// Cache utilization optimization
void optimize_cache_usage(
    const double * __restrict__ input_data,
    double * __restrict__ output_data,
    int data_size,
    int cache_line_size
);

// Bank conflict elimination in shared memory
__device__ __forceinline__ void shared_memory_bank_conflict_free_access(
    cuDoubleComplex *shared_ptr,
    int thread_id,
    int element_size
);

// Instruction-level parallelism
__device__ __forceinline__ void instruction_level_parallelism_optimization(
    cuDoubleComplex *data,
    int data_size
);

// Branch divergence minimization
__device__ __forceinline__ int branch_divergence_minimization(
    const cuDoubleComplex *data,
    int condition_mask
);

// Register pressure optimization
__device__ __forceinline__ void register_pressure_optimization(
    cuDoubleComplex *registers,
    int num_registers_needed
);

#endif // ENABLE_CUDA

#endif // GPU_PARALLELIZATION_OPTIMIZED_H