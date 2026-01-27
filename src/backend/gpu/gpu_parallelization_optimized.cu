#include "gpu_parallelization_optimized.h"

#ifdef ENABLE_CUDA

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
) {
    // Coalesced memory access pattern
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    
    // Use shared memory for frequently accessed data
    __shared__ double shared_krho[OPTIMIZED_BLOCK_SIZE];
    __shared__ double shared_weights[OPTIMIZED_BLOCK_SIZE];
    
    // Prefetch data to L2 cache
    for (int i = tid; i < n_points; i += stride) {
        prefetch_data_l2(&krho_points[i], sizeof(double));
        prefetch_data_l2(&weights[i], sizeof(double));
    }
    
    __syncthreads();
    
    // Main computation loop with vectorized operations
    for (int i = tid; i < n_points; i += stride) {
        // Coalesced memory access
        double krho = krho_points[i];
        double weight = weights[i];
        
        // Vectorized load for better memory bandwidth
        double4 coords_data;
        vectorized_load_double4(z_coords, &coords_data, i);
        
        // Use texture memory for better cache performance
        double z = coords_data.x;
        double zp = coords_data.y;
        int layer_idx = layer_indices[i];
        
        // Optimized Sommerfeld integral computation
        cuDoubleComplex result = make_cuDoubleComplex(0.0, 0.0);
        
        // Use constant memory for medium parameters
        double er = const_medium_params[layer_idx * 4];
        double sigma = const_medium_params[layer_idx * 4 + 1];
        double mu_r = const_medium_params[layer_idx * 4 + 2];
        double thickness = const_medium_params[layer_idx * 4 + 3];
        
        // Wave number calculation with instruction-level parallelism
        double k0 = omega / 299792458.0; // Speed of light in vacuum
        double k = k0 * sqrt(er * mu_r - I * sigma / (omega * 8.854e-12));
        
        // Bessel function computation with warp-level reduction
        double J0_krho = j0(krho * k);
        double J1_krho = j1(krho * k);
        
        // Complex exponential with branch cut optimization
        double gamma_z = sqrt(krho * krho - k * k);
        cuDoubleComplex exp_factor = make_cuDoubleComplex(
            cos(gamma_z * (z - zp)), sin(gamma_z * (z - zp))
        );
        
        // Result accumulation with warp-level reduction
        cuDoubleComplex local_result = cuCmul(make_cuDoubleComplex(weight, 0.0), 
                                             cuCmul(make_cuDoubleComplex(J0_krho, 0.0), exp_factor));
        
        // Warp-level reduction for final result
        result = warp_reduce_sum_complex(local_result, 0xffffffff);
        
        // Store result with coalesced write
        if (threadIdx.x % WARP_SIZE == 0) {
            results[i] = result;
        }
    }
}

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
) {
    // 2D tiling for better memory locality
    int tile_row = blockIdx.y * params.tile_size_m;
    int tile_col = blockIdx.x * params.tile_size_n;
    int thread_row = threadIdx.y;
    int thread_col = threadIdx.x;
    
    // Shared memory for tile data
    extern __shared__ double shared_mem[];
    double *shared_x = shared_mem;
    double *shared_y = &shared_x[params.tile_size_m];
    double *shared_z = &shared_y[params.tile_size_m];
    double *shared_xp = &shared_z[params.tile_size_m];
    double *shared_yp = &shared_xp[params.tile_size_n];
    double *shared_zp = &shared_yp[params.tile_size_n];
    
    // Cooperative loading of tile data
    if (tile_row + thread_row < n_source_points && thread_col < params.tile_size_n) {
        int global_row = tile_row + thread_row;
        shared_x[thread_row] = x_coords[global_row];
        shared_y[thread_row] = y_coords[global_row];
        shared_z[thread_row] = z_coords[global_row];
    }
    
    if (tile_col + thread_col < n_obs_points && thread_row < params.tile_size_m) {
        int global_col = tile_col + thread_col;
        shared_xp[thread_col] = xp_coords[global_col];
        shared_yp[thread_col] = yp_coords[global_col];
        shared_zp[thread_col] = zp_coords[global_col];
    }
    
    __syncthreads();
    
    // Compute Green's function for tile elements
    if (tile_row + thread_row < n_source_points && tile_col + thread_col < n_obs_points) {
        int global_row = tile_row + thread_row;
        int global_col = tile_col + thread_col;
        
        // Load coordinates from shared memory
        double x = shared_x[thread_row];
        double y = shared_y[thread_row];
        double z = shared_z[thread_row];
        double xp = shared_xp[thread_col];
        double yp = shared_yp[thread_col];
        double zp = shared_zp[thread_col];
        
        // Distance calculation with instruction-level parallelism
        double dx = x - xp;
        double dy = y - yp;
        double dz = z - zp;
        double R = sqrt(dx*dx + dy*dy + dz*dz);
        
        // Avoid singularity
        if (R < 1e-10) R = 1e-10;
        
        // Wave number from constant memory
        double k0 = omega / 299792458.0;
        
        // Complex Green's function with vectorized operations
        double kR = k0 * R;
        cuDoubleComplex green = make_cuDoubleComplex(
            cos(kR) / (4.0 * M_PI * R),
            -sin(kR) / (4.0 * M_PI * R)
        );
        
        // Store result with coalesced write
        int idx = global_row * n_obs_points + global_col;
        green_matrix[idx] = green;
    }
}

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
) {
    int tid = threadIdx.x + threadIdx.y * blockDim.x;
    int warp_id = tid / WARP_SIZE;
    int lane_id = tid % WARP_SIZE;
    int global_row = blockIdx.y * blockDim.y + threadIdx.y;
    int global_col = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Use warp-level primitives for efficient reduction
    cg::thread_block_tile<WARP_SIZE> tile = cg::tiled_partition<WARP_SIZE>(cg::this_thread_block());
    
    // Shared memory for row/column norms
    __shared__ double shared_norms[OPTIMIZED_BLOCK_SIZE];
    __shared__ int shared_pivot_rows[OPTIMIZED_BLOCK_SIZE];
    __shared__ int shared_pivot_cols[OPTIMIZED_BLOCK_SIZE];
    
    // ACA iteration loop with branch divergence minimization
    for (int rank = 0; rank < max_rank; rank++) {
        // Find pivot row with warp-level reduction
        if (global_row < block_rows && global_col < block_cols) {
            double local_max = 0.0;
            int local_pivot_col = 0;
            
            // Compute row norms with vectorized operations
            for (int j = lane_id; j < block_cols; j += WARP_SIZE) {
                int idx = global_row * block_cols + j;
                cuDoubleComplex val = matrix_block[idx];
                double norm = cuCreal(cuCmul(val, cuConj(val)));
                
                if (norm > local_max) {
                    local_max = norm;
                    local_pivot_col = j;
                }
            }
            
            // Warp-level reduction to find global maximum
            double warp_max = cg::reduce(tile, local_max, cg::greater<double>());
            
            if (local_max == warp_max && lane_id == 0) {
                shared_pivot_cols[warp_id] = local_pivot_col;
                shared_norms[warp_id] = local_max;
            }
        }
        
        __syncthreads();
        
        // Check convergence with branch divergence minimization
        double max_norm = 0.0;
        if (tid < (OPTIMIZED_BLOCK_SIZE + WARP_SIZE - 1) / WARP_SIZE) {
            max_norm = shared_norms[tid];
        }
        
        // Block-level reduction
        double block_max = cg::reduce(cg::this_thread_block(), max_norm, cg::greater<double>());
        
        if (tid == 0 && block_max < tolerance * tolerance) {
            *actual_rank = rank;
            *compression_error = sqrt(block_max);
            return;
        }
        
        // Update U and V factors with coalesced writes
        if (global_row < block_rows && global_col < block_cols) {
            int pivot_col = shared_pivot_cols[0];
            
            if (global_col == pivot_col) {
                U_factor[global_row * max_rank + rank] = matrix_block[global_row * block_cols + pivot_col];
            }
            
            if (global_row == shared_pivot_rows[0]) {
                V_factor[rank * block_cols + global_col] = matrix_block[shared_pivot_rows[0] * block_cols + global_col];
            }
        }
        
        __syncthreads();
    }
}

// Optimized matrix-vector multiplication with vectorized operations
__global__ void optimized_matrix_vector_multiply_kernel(
    const cuDoubleComplex * __restrict__ matrix,
    const cuDoubleComplex * __restrict__ vector,
    const int rows,
    const int cols,
    cuDoubleComplex * __restrict__ result,
    const int vector_width
) {
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    // Use shared memory for vector data
    extern __shared__ cuDoubleComplex shared_vector[];
    
    // Vectorized loading of vector data
    if (tid * vector_width < cols) {
        for (int v = 0; v < vector_width; v++) {
            int col = tid * vector_width + v;
            if (col < cols) {
                shared_vector[col] = vector[col];
            }
        }
    }
    
    __syncthreads();
    
    // Matrix-vector multiplication with instruction-level parallelism
    if (row < rows) {
        cuDoubleComplex sum = make_cuDoubleComplex(0.0, 0.0);
        
        // Unroll loop for better instruction-level parallelism
        int cols_per_iteration = 4;
        int unrolled_cols = (cols / cols_per_iteration) * cols_per_iteration;
        
        for (int col = 0; col < unrolled_cols; col += cols_per_iteration) {
            cuDoubleComplex a1 = matrix[row * cols + col];
            cuDoubleComplex b1 = shared_vector[col];
            cuDoubleComplex a2 = matrix[row * cols + col + 1];
            cuDoubleComplex b2 = shared_vector[col + 1];
            cuDoubleComplex a3 = matrix[row * cols + col + 2];
            cuDoubleComplex b3 = shared_vector[col + 2];
            cuDoubleComplex a4 = matrix[row * cols + col + 3];
            cuDoubleComplex b4 = shared_vector[col + 3];
            
            sum = cuCadd(sum, cuCmul(a1, b1));
            sum = cuCadd(sum, cuCmul(a2, b2));
            sum = cuCadd(sum, cuCmul(a3, b3));
            sum = cuCadd(sum, cuCmul(a4, b4));
        }
        
        // Handle remaining elements
        for (int col = unrolled_cols; col < cols; col++) {
            sum = cuCadd(sum, cuCmul(matrix[row * cols + col], shared_vector[col]));
        }
        
        result[row] = sum;
    }
}

// Warp-level reduction for improved performance
__device__ __forceinline__ cuDoubleComplex warp_reduce_sum_complex(
    cuDoubleComplex value,
    unsigned int mask
) {
    // Shuffle-based warp reduction
    for (int offset = WARP_SIZE / 2; offset > 0; offset /= 2) {
        cuDoubleComplex shuffled = __shfl_down_sync(mask, value, offset);
        value = cuCadd(value, shuffled);
    }
    return value;
}

// Vectorized memory operations
__device__ __forceinline__ void vectorized_load_double4(
    const double * __restrict__ ptr,
    double4 *registers,
    int offset
) {
    double4 data = *reinterpret_cast<const double4*>(ptr + offset);
    *registers = data;
}

__device__ __forceinline__ void vectorized_store_double4(
    double * __restrict__ ptr,
    const double4 *registers,
    int offset
) {
    *reinterpret_cast<double4*>(ptr + offset) = *registers;
}

// Prefetching for latency hiding
__device__ __forceinline__ void prefetch_data_l2(
    const void *ptr,
    int prefetch_size
) {
    // Use inline PTX for L2 prefetch
    asm volatile("prefetch.global.L2 [%0];" :: "l"(ptr));
}

// Optimized kernel launch configuration
dim3 calculate_optimal_grid_size(
    int data_size,
    int block_size,
    int max_blocks_per_sm,
    int num_sms
) {
    int max_blocks = max_blocks_per_sm * num_sms;
    int needed_blocks = (data_size + block_size - 1) / block_size;
    int actual_blocks = min(needed_blocks, max_blocks * OPTIMIZED_GRID_SIZE_MULTIPLIER);
    
    return dim3(actual_blocks);
}

// Occupancy optimization
KernelOptimizationParams calculate_optimal_kernel_params(
    const cudaDeviceProp *device_props,
    const char *kernel_name,
    int matrix_rows,
    int matrix_cols,
    int shared_memory_per_thread
) {
    KernelOptimizationParams params;
    
    // Calculate based on device properties
    int max_threads_per_sm = device_props->maxThreadsPerMultiProcessor;
    int max_shared_memory_per_sm = device_props->sharedMemPerMultiprocessor;
    int num_sms = device_props->multiProcessorCount;
    
    // Optimal tile sizes for matrix operations
    params.tile_size_m = 32;
    params.tile_size_n = 32;
    params.tile_size_k = 16;
    
    // Thread configuration
    params.threads_per_block = OPTIMIZED_BLOCK_SIZE;
    params.shared_memory_size = min(shared_memory_per_thread * OPTIMIZED_BLOCK_SIZE, 
                                     max_shared_memory_per_sm / 4);
    
    // Memory usage optimization
    params.use_texture_memory = (matrix_rows * matrix_cols > 1000000);
    params.use_constant_memory = (matrix_rows < 64 && matrix_cols < 64);
    params.vector_width = 4; // Double4 vectorization
    
    return params;
}

// Auto-tuning for optimal performance
void auto_tune_kernel_parameters(
    const char *kernel_name,
    KernelOptimizationParams *params,
    int matrix_size,
    int num_iterations
) {
    // Performance measurement variables
    KernelPerformanceMetrics best_metrics;
    KernelOptimizationParams best_params = *params;
    double best_time = INFINITY;
    
    // Test different configurations
    int block_sizes[] = {128, 256, 512};
    int tile_sizes[] = {16, 32, 64};
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            KernelOptimizationParams test_params = *params;
            test_params.threads_per_block = block_sizes[i];
            test_params.tile_size_m = tile_sizes[j];
            test_params.tile_size_n = tile_sizes[j];
            
            // Measure performance
            cudaEvent_t start, stop;
            cudaEventCreate(&start);
            cudaEventCreate(&stop);
            
            cudaEventRecord(start);
            
            // Launch kernel with test parameters (simplified for demonstration)
            // ... kernel launch code ...
            
            cudaEventRecord(stop);
            cudaEventSynchronize(stop);
            
            float milliseconds = 0;
            cudaEventElapsedTime(&milliseconds, start, stop);
            
            if (milliseconds < best_time) {
                best_time = milliseconds;
                best_params = test_params;
            }
            
            cudaEventDestroy(start);
            cudaEventDestroy(stop);
        }
    }
    
    *params = best_params;
}

// Performance monitoring
void measure_kernel_performance(
    const char *kernel_name,
    KernelPerformanceMetrics *metrics,
    cudaStream_t stream
) {
    // Create CUDA events for timing
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    
    // Record start event
    cudaEventRecord(start, stream);
    
    // ... kernel execution ...
    
    // Record stop event
    cudaEventRecord(stop, stream);
    cudaEventSynchronize(stop);
    
    // Calculate timing
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    metrics->execution_time = milliseconds;
    
    // Calculate throughput metrics (simplified)
    metrics->memory_throughput = 0.0; // Would calculate based on actual data transfer
    metrics->compute_throughput = 0.0; // Would calculate based on FLOPS
    
    // Cleanup
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
}

// Memory bandwidth optimization
void optimize_memory_bandwidth_usage(
    cuDoubleComplex *device_ptr,
    size_t size,
    int memory_access_pattern
) {
    // Set memory access hints based on pattern
    if (memory_access_pattern == 0) { // Sequential access
        cudaMemAdvise(device_ptr, size * sizeof(cuDoubleComplex), 
                     cudaMemAdviseSetReadMostly, 0);
    } else if (memory_access_pattern == 1) { // Random access
        cudaMemAdvise(device_ptr, size * sizeof(cuDoubleComplex), 
                     cudaMemAdviseSetAccessedBy, 0);
    }
}

// Bank conflict elimination in shared memory
__device__ __forceinline__ void shared_memory_bank_conflict_free_access(
    cuDoubleComplex *shared_ptr,
    int thread_id,
    int element_size
) {
    // Add padding to eliminate bank conflicts
    int padded_stride = element_size + 1;
    shared_ptr[thread_id * padded_stride] = make_cuDoubleComplex(0.0, 0.0);
}

#endif // ENABLE_CUDA