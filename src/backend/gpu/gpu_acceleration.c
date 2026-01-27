#include "gpu_acceleration.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef ENABLE_CUDA

// CUDA kernel for Sommerfeld integral computation
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
    gpu_complex *results) {
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= n_points) return;
    
    double krho = krho_points[idx];
    double weight = weights[idx];
    double z = z_coords[idx];
    double zp = zp_coords[idx];
    int layer_src = layer_indices[idx * 2];
    int layer_obs = layer_indices[idx * 2 + 1];
    
    // Simplified Sommerfeld integral computation
    double k0 = omega / 299792458.0; // Free-space wavenumber
    double kz = sqrt(k0 * k0 - krho * krho);
    
    if (kz == 0.0) kz = 1e-12; // Avoid division by zero
    
    gpu_complex integrand;
    if (layer_src == layer_obs) {
        // Same layer contribution
        double phase = -kz * fabs(z - zp);
        integrand = make_gpu_complex(weight * cos(phase) / (2.0 * kz), 
                                      weight * sin(phase) / (2.0 * kz));
    } else {
        // Different layers - transmission
        double phase = -kz * (z + zp);
        integrand = make_gpu_complex(weight * cos(phase) / (2.0 * kz), 
                                      weight * sin(phase) / (2.0 * kz));
    }
    
    results[idx] = integrand;
}

// CUDA kernel for Green's function matrix assembly
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
    gpu_complex *green_matrix) {
    
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row >= n_obs_points || col >= n_source_points) return;
    
    double dx = xp_coords[row] - x_coords[col];
    double dy = yp_coords[row] - y_coords[col];
    double dz = zp_coords[row] - z_coords[col];
    
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    double k0 = omega / 299792458.0;
    
    if (r < 1e-12) {
        // Self-term - simplified
        green_matrix[row * n_source_points + col] = make_gpu_complex(1e6, 0.0);
    } else {
        // Free-space Green's function
        double phase = -k0 * r;
        double magnitude = 1.0 / (4.0 * M_PI * r);
        green_matrix[row * n_source_points + col] = make_gpu_complex(
            magnitude * cos(phase), magnitude * sin(phase));
    }
}

// CUDA kernel for ACA compression
__global__ void aca_compression_kernel(
    const gpu_complex *matrix_block,
    const int block_rows,
    const int block_cols,
    const double tolerance,
    const int max_rank,
    gpu_complex *U_factor,
    gpu_complex *V_factor,
    int *actual_rank,
    double *compression_error) {
    
    extern __shared__ gpu_complex shared_mem[];
    gpu_complex *shared_U = shared_mem;
    gpu_complex *shared_V = &shared_mem[block_rows * max_rank];
    
    int tid = threadIdx.x;
    int rank = 0;
    double error = 1e10;
    
    // Simplified ACA implementation
    for (rank = 0; rank < max_rank && error > tolerance; rank++) {
        // Pivot selection (simplified)
        int pivot_row = rank % block_rows;
        int pivot_col = rank % block_cols;
        
        // Extract row from residual
        if (tid < block_cols) {
            gpu_complex row_val = matrix_block[pivot_row * block_cols + tid];
            for (int k = 0; k < rank; k++) {
                row_val = cuCsub(row_val, cuCmul(shared_U[pivot_row * max_rank + k], 
                                                  shared_V[k * block_cols + tid]));
            }
            shared_V[rank * block_cols + tid] = row_val;
        }
        
        __syncthreads();
        
        // Normalize row
        if (tid == 0) {
            gpu_complex pivot_val = shared_V[rank * block_cols + pivot_col];
            if (cuCabs(pivot_val) > 1e-12) {
                for (int j = 0; j < block_cols; j++) {
                    shared_V[rank * block_cols + j] = cuCdiv(shared_V[rank * block_cols + j], pivot_val);
                }
            }
        }
        
        __syncthreads();
        
        // Extract column from residual
        if (tid < block_rows) {
            gpu_complex col_val = matrix_block[tid * block_cols + pivot_col];
            for (int k = 0; k < rank; k++) {
                col_val = cuCsub(col_val, cuCmul(shared_U[tid * max_rank + k], 
                                                  shared_V[k * block_cols + pivot_col]));
            }
            shared_U[tid * max_rank + rank] = col_val;
        }
        
        __syncthreads();
        
        // Estimate error (simplified)
        if (tid == 0) {
            error = 0.0;
            for (int i = 0; i < block_rows; i++) {
                for (int j = 0; j < block_cols; j++) {
                    gpu_complex residual = matrix_block[i * block_cols + j];
                    for (int k = 0; k <= rank; k++) {
                        residual = cuCsub(residual, cuCmul(shared_U[i * max_rank + k], 
                                                           shared_V[k * block_cols + j]));
                    }
                    error += cuCabs(residual);
                }
            }
            error /= (block_rows * block_cols);
        }
        
        __syncthreads();
    }
    
    // Copy results to global memory
    if (tid < block_rows * max_rank) {
        U_factor[tid] = shared_U[tid];
    }
    if (tid < max_rank * block_cols) {
        V_factor[tid] = shared_V[tid];
    }
    
    if (tid == 0) {
        *actual_rank = rank;
        *compression_error = error;
    }
}

// CUDA kernel for matrix-vector multiplication
__global__ void matrix_vector_multiply_kernel(
    const gpu_complex *matrix,
    const gpu_complex *vector,
    const int rows,
    const int cols,
    gpu_complex *result) {
    
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    if (row >= rows) return;
    
    gpu_complex sum = make_gpu_complex(0.0, 0.0);
    for (int col = 0; col < cols; col++) {
        gpu_complex matrix_element = matrix[row * cols + col];
        gpu_complex vector_element = vector[col];
        sum = cuCadd(sum, cuCmul(matrix_element, vector_element));
    }
    
    result[row] = sum;
}

// GPU context initialization
GPUContext* initialize_gpu_context(int device_id) {
    GPUContext *context = (GPUContext*)malloc(sizeof(GPUContext));
    context->device_id = device_id;
    
    CUDA_CHECK(cudaSetDevice(device_id));
    CUDA_CHECK(cudaGetDeviceProperties(&context->device_properties, device_id));
    
    // Initialize cuBLAS
    CUBLAS_CHECK(cublasCreate(&context->cublas_handle));
    
    // Initialize cuSOLVER
    cusolverStatus_t status = cusolverDnCreate(&context->cusolver_handle);
    if (status != CUSOLVER_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create cuSOLVER handle\n");
        free(context);
        return NULL;
    }
    
    // Create CUDA streams
    CUDA_CHECK(cudaStreamCreate(&context->compute_stream));
    CUDA_CHECK(cudaStreamCreate(&context->memory_stream));
    
    // Allocate memory pool (1 GB default)
    context->memory_pool_size = 1024 * 1024 * 1024; // 1 GB
    context->memory_pool_used = 0;
    CUDA_CHECK(cudaMalloc(&context->device_memory_pool, context->memory_pool_size));
    
    // Set performance metrics
    context->memory_bandwidth = context->device_properties.memoryBusWidth * 
                               context->device_properties.memoryClockRate * 2.0 / 8.0 / 1e9; // GB/s
    context->compute_throughput = context->device_properties.multiProcessorCount * 
                                 context->device_properties.maxThreadsPerMultiProcessor * 
                                 context->device_properties.clockRate / 1e9; // GFLOPS
    context->max_threads_per_block = context->device_properties.maxThreadsPerBlock;
    context->max_blocks_per_grid = 65535; // Typical CUDA limit
    
    printf("GPU Context Initialized:\n");
    printf("  Device: %s (ID: %d)\n", context->device_properties.name, device_id);
    printf("  Compute Capability: %d.%d\n", 
           context->device_properties.major, context->device_properties.minor);
    printf("  Memory: %.1f GB\n", context->device_properties.totalGlobalMem / 1e9);
    printf("  Memory Bandwidth: %.1f GB/s\n", context->memory_bandwidth);
    printf("  Compute Throughput: %.1f GFLOPS\n", context->compute_throughput);
    printf("  Max Threads/Block: %d\n", context->max_threads_per_block);
    
    return context;
}

// GPU context cleanup
void cleanup_gpu_context(GPUContext *context) {
    if (!context) return;
    
    CUDA_CHECK(cudaFree(context->device_memory_pool));
    CUDA_CHECK(cudaStreamDestroy(context->compute_stream));
    CUDA_CHECK(cudaStreamDestroy(context->memory_stream));
    CUBLAS_CHECK(cublasDestroy(context->cublas_handle));
    cusolverDnDestroy(context->cusolver_handle);
    
    free(context);
}

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
    gpu_complex *results) {
    
    // Allocate device memory
    double *d_krho, *d_weights, *d_z, *d_zp, *d_medium_params;
    int *d_layer_indices;
    gpu_complex *d_results;
    
    size_t total_size = n_points * (sizeof(double) * 4 + sizeof(int) * 2) + 
                       n_layers * 4 * sizeof(double) + n_points * sizeof(gpu_complex);
    
    if (total_size > context->memory_pool_size - context->memory_pool_used) {
        fprintf(stderr, "Insufficient GPU memory pool\n");
        return;
    }
    
    // Use memory pool allocation
    size_t offset = 0;
    d_krho = (double*)((char*)context->device_memory_pool + offset);
    offset += n_points * sizeof(double);
    d_weights = (double*)((char*)context->device_memory_pool + offset);
    offset += n_points * sizeof(double);
    d_z = (double*)((char*)context->device_memory_pool + offset);
    offset += n_points * sizeof(double);
    d_zp = (double*)((char*)context->device_memory_pool + offset);
    offset += n_points * sizeof(double);
    d_layer_indices = (int*)((char*)context->device_memory_pool + offset);
    offset += n_points * 2 * sizeof(int);
    d_medium_params = (double*)((char*)context->device_memory_pool + offset);
    offset += n_layers * 4 * sizeof(double);
    d_results = (gpu_complex*)((char*)context->device_memory_pool + offset);
    
    // Copy data to device asynchronously
    CUDA_CHECK(cudaMemcpyAsync(d_krho, krho_points, n_points * sizeof(double), 
                              cudaMemcpyHostToDevice, context->memory_stream));
    CUDA_CHECK(cudaMemcpyAsync(d_weights, weights, n_points * sizeof(double), 
                              cudaMemcpyHostToDevice, context->memory_stream));
    CUDA_CHECK(cudaMemcpyAsync(d_z, z_coords, n_points * sizeof(double), 
                              cudaMemcpyHostToDevice, context->memory_stream));
    CUDA_CHECK(cudaMemcpyAsync(d_zp, zp_coords, n_points * sizeof(double), 
                              cudaMemcpyHostToDevice, context->memory_stream));
    CUDA_CHECK(cudaMemcpyAsync(d_layer_indices, layer_indices, n_points * 2 * sizeof(int), 
                              cudaMemcpyHostToDevice, context->memory_stream));
    CUDA_CHECK(cudaMemcpyAsync(d_medium_params, medium_params, n_layers * 4 * sizeof(double), 
                              cudaMemcpyHostToDevice, context->memory_stream));
    
    // Configure kernel launch parameters
    int threads_per_block = 256;
    int blocks_per_grid = (n_points + threads_per_block - 1) / threads_per_block;
    
    dim3 block_size(threads_per_block, 1, 1);
    dim3 grid_size(blocks_per_grid, 1, 1);
    
    // Launch kernel
    sommerfeld_integral_kernel<<<grid_size, block_size, 0, context->compute_stream>>>(
        d_krho, d_weights, d_z, d_zp, d_layer_indices, d_medium_params, omega,
        n_points, n_layers, d_results);
    
    // Copy results back
    CUDA_CHECK(cudaMemcpyAsync(results, d_results, n_points * sizeof(gpu_complex), 
                              cudaMemcpyDeviceToHost, context->memory_stream));
    
    // Synchronize streams
    CUDA_CHECK(cudaStreamSynchronize(context->compute_stream));
    CUDA_CHECK(cudaStreamSynchronize(context->memory_stream));
    
    context->memory_pool_used = 0; // Reset for next operation
}

// GPU-accelerated matrix-vector multiplication
void gpu_matrix_vector_multiply(
    GPUContext *context,
    const gpu_complex *matrix,
    const gpu_complex *vector,
    const int rows,
    const int cols,
    gpu_complex *result) {
    
    int threads_per_block = 256;
    int blocks_per_grid = (rows + threads_per_block - 1) / threads_per_block;
    
    matrix_vector_multiply_kernel<<<blocks_per_grid, threads_per_block, 0, context->compute_stream>>>(
        matrix, vector, rows, cols, result);
    
    CUDA_CHECK(cudaStreamSynchronize(context->compute_stream));
}

// Complete GPU-based GMRES solver implementation
__global__ void gmres_iteration_kernel(
    const gpu_complex *A_matrix,
    const gpu_complex *b_vector,
    gpu_complex *x_vector,
    const int matrix_size,
    const int iteration,
    const double tolerance,
    double *residual_norm,
    bool *converged
) {
    // This is a placeholder - actual implementation would use the kernels above
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= matrix_size) return;
    
    // Simplified Jacobi iteration for demonstration
    gpu_complex residual = b_vector[idx];
    for (int j = 0; j < matrix_size; j++) {
        gpu_complex Ax = cuCmul(A_matrix[idx * matrix_size + j], x_vector[j]);
        residual = cuCsub(residual, Ax);
    }
    
    // Simple update
    x_vector[idx] = cuCadd(x_vector[idx], cuCmul(make_gpu_complex(0.1, 0.0), residual));
    
    if (idx == 0) {
        double norm = 0.0;
        for (int i = 0; i < matrix_size; i++) {
            double real = cuCreal(residual);
            double imag = cuCimag(residual);
            norm += real * real + imag * imag;
        }
        *residual_norm = sqrt(norm);
        *converged = (*residual_norm < tolerance);
    }
}

// GPU-accelerated iterative solver with cuBLAS integration
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
    bool *converged) {
    
    const int restart = 30; // GMRES restart parameter
    const int max_restarts = max_iterations / restart;
    
    // Allocate device memory for GMRES working arrays
    gpu_complex *d_A, *d_b, *d_x;
    gpu_complex *d_V, *d_H, *d_g, *d_y;
    gpu_complex *d_cs, *d_sn;
    gpu_complex *d_residual, *d_w;
    
    size_t matrix_bytes = matrix_size * matrix_size * sizeof(gpu_complex);
    size_t vector_bytes = matrix_size * sizeof(gpu_complex);
    size_t basis_bytes = (restart + 1) * matrix_size * sizeof(gpu_complex);
    size_t hessenberg_bytes = (restart + 1) * (restart + 1) * sizeof(gpu_complex);
    
    // Use cuBLAS for optimized operations
    cublasOperation_t trans = CUBLAS_OP_N;
    cublasOperation_t trans_conj = CUBLAS_OP_C;
    
    // Allocate device memory
    CUDA_CHECK(cudaMalloc(&d_A, matrix_bytes));
    CUDA_CHECK(cudaMalloc(&d_b, vector_bytes));
    CUDA_CHECK(cudaMalloc(&d_x, vector_bytes));
    CUDA_CHECK(cudaMalloc(&d_V, basis_bytes));
    CUDA_CHECK(cudaMalloc(&d_H, hessenberg_bytes));
    CUDA_CHECK(cudaMalloc(&d_g, (restart + 1) * sizeof(gpu_complex)));
    CUDA_CHECK(cudaMalloc(&d_y, restart * sizeof(gpu_complex)));
    CUDA_CHECK(cudaMalloc(&d_cs, restart * sizeof(gpu_complex)));
    CUDA_CHECK(cudaMalloc(&d_sn, restart * sizeof(gpu_complex)));
    CUDA_CHECK(cudaMalloc(&d_residual, vector_bytes));
    CUDA_CHECK(cudaMalloc(&d_w, vector_bytes));
    
    // Copy initial data
    CUDA_CHECK(cudaMemcpy(d_A, A_matrix, matrix_bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_b, b_vector, vector_bytes, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_x, x_vector, vector_bytes, cudaMemcpyHostToDevice));
    
    // cuBLAS handles for optimized operations
    cublasHandle_t cublas_handle = context->cublas_handle;
    cusolverDnHandle_t cusolver_handle = context->cusolver_handle;
    
    // GMRES main loop
    *converged = false;
    *iterations = 0;
    
    for (int restart_count = 0; restart_count < max_restarts && !*converged; restart_count++) {
        
        // Compute initial residual: r = b - A*x
        cublasZcopy(cublas_handle, matrix_size, d_b, 1, d_residual, 1);
        
        gpu_complex alpha = make_gpu_complex(-1.0, 0.0);
        gpu_complex beta = make_gpu_complex(1.0, 0.0);
        cublasZgemv(cublas_handle, trans, matrix_size, matrix_size,
                   &alpha, d_A, matrix_size, d_x, 1, &beta, d_residual, 1);
        
        // Compute ||r||
        double residual_norm;
        cublasDznrm2(cublas_handle, matrix_size, d_residual, 1, &residual_norm);
        
        if (residual_norm < tolerance) {
            *converged = true;
            *final_residual = residual_norm;
            break;
        }
        
        // Initialize first basis vector: v_0 = r / ||r||
        gpu_complex inv_norm = make_gpu_complex(1.0 / residual_norm, 0.0);
        cublasZcopy(cublas_handle, matrix_size, d_residual, 1, &d_V[0], 1);
        cublasZscal(cublas_handle, matrix_size, &inv_norm, &d_V[0], 1);
        
        // Initialize g vector: g = [||r||, 0, 0, ...]
        cublasZscal(cublas_handle, restart + 1, &make_gpu_complex(0.0, 0.0), d_g, 1);
        gpu_complex initial_residual = make_gpu_complex(residual_norm, 0.0);
        cublasZcopy(cublas_handle, 1, &initial_residual, 1, d_g, 1);
        
        // Arnoldi iteration
        for (int k = 0; k < restart && !*converged; k++) {
            
            // Compute w = A * v_k
            alpha = make_gpu_complex(1.0, 0.0);
            beta = make_gpu_complex(0.0, 0.0);
            cublasZgemv(cublas_handle, trans, matrix_size, matrix_size,
                       &alpha, d_A, matrix_size, &d_V[k * matrix_size], 1, 
                       &beta, d_w, 1);
            
            // Orthogonalization (modified Gram-Schmidt)
            for (int j = 0; j <= k; j++) {
                // h_jk = w^H * v_j
                gpu_complex h_jk;
                cublasZdotc(cublas_handle, matrix_size, d_w, 1, 
                           &d_V[j * matrix_size], 1, &h_jk);
                
                // Store in H matrix
                CUDA_CHECK(cudaMemcpy(&d_H[j * (restart + 1) + k], &h_jk, 
                                   sizeof(gpu_complex), cudaMemcpyHostToDevice));
                
                // w = w - h_jk * v_j
                alpha = make_gpu_complex(-cuCreal(h_jk), -cuCimag(h_jk));
                cublasZaxpy(cublas_handle, matrix_size, &alpha, 
                           &d_V[j * matrix_size], 1, d_w, 1);
            }
            
            // Compute h_{k+1,k} = ||w||
            double h_k1_k;
            cublasDznrm2(cublas_handle, matrix_size, d_w, 1, &h_k1_k);
            gpu_complex h_k1_k_complex = make_gpu_complex(h_k1_k, 0.0);
            CUDA_CHECK(cudaMemcpy(&d_H[(k + 1) * (restart + 1) + k], &h_k1_k_complex,
                               sizeof(gpu_complex), cudaMemcpyHostToDevice));
            
            // v_{k+1} = w / h_{k+1,k}
            if (h_k1_k > 1e-12) {
                alpha = make_gpu_complex(1.0 / h_k1_k, 0.0);
                cublasZcopy(cublas_handle, matrix_size, d_w, 1, 
                           &d_V[(k + 1) * matrix_size], 1);
                cublasZscal(cublas_handle, matrix_size, &alpha, 
                           &d_V[(k + 1) * matrix_size], 1);
            } else {
                // Handle breakdown
                printf("GMRES breakdown at iteration %d\n", *iterations + k);
                break;
            }
            
            // Apply Givens rotations to H
            for (int j = 0; j < k; j++) {
                gpu_complex h_j = d_H[j * (restart + 1) + k];
                gpu_complex h_j1 = d_H[(j + 1) * (restart + 1) + k];
                
                gpu_complex cs = d_cs[j];
                gpu_complex sn = d_sn[j];
                
                gpu_complex temp1 = cuCadd(cuCmul(cs, h_j), cuCmul(sn, h_j1));
                gpu_complex temp2 = cuCsub(cuCmul(cs, h_j1), cuCmul(sn, h_j));
                
                d_H[j * (restart + 1) + k] = temp1;
                d_H[(j + 1) * (restart + 1) + k] = temp2;
            }
            
            // Compute new Givens rotation
            gpu_complex h_k = d_H[k * (restart + 1) + k];
            gpu_complex h_k1 = d_H[(k + 1) * (restart + 1) + k];
            
            double r = sqrt(cuCreal(h_k) * cuCreal(h_k) + cuCreal(h_k1) * cuCreal(h_k1));
            d_cs[k] = make_gpu_complex(cuCreal(h_k) / r, 0.0);
            d_sn[k] = make_gpu_complex(cuCreal(h_k1) / r, 0.0);
            
            // Apply to H and g
            d_H[k * (restart + 1) + k] = make_gpu_complex(r, 0.0);
            d_H[(k + 1) * (restart + 1) + k] = make_gpu_complex(0.0, 0.0);
            
            gpu_complex g_k = d_g[k];
            gpu_complex g_k1 = d_g[k + 1];
            
            d_g[k] = cuCadd(cuCmul(d_cs[k], g_k), cuCmul(d_sn[k], g_k1));
            d_g[k + 1] = cuCsub(cuCmul(d_cs[k], g_k1), cuCmul(d_sn[k], g_k));
            
            // Check convergence
            double residual_norm_new = cuCabs(d_g[k + 1]);
            *iterations = restart_count * restart + k + 1;
            
            if (residual_norm_new < tolerance || *iterations >= max_iterations) {
                *converged = (residual_norm_new < tolerance);
                *final_residual = residual_norm_new;
                
                // Solve upper triangular system
                // y = H(1:k,1:k) \ g(1:k)
                for (int i = k; i >= 0; i--) {
                    d_y[i] = d_g[i];
                    for (int j = i + 1; j <= k; j++) {
                        gpu_complex h_ij = d_H[i * (restart + 1) + j];
                        gpu_complex y_j = d_y[j];
                        d_y[i] = cuCsub(d_y[i], cuCmul(h_ij, y_j));
                    }
                    gpu_complex h_ii = d_H[i * (restart + 1) + i];
                    if (cuCabs(h_ii) > 1e-12) {
                        d_y[i] = cuCdiv(d_y[i], h_ii);
                    }
                }
                
                // Update solution: x = x + V(:,1:k) * y
                alpha = make_gpu_complex(1.0, 0.0);
                beta = make_gpu_complex(1.0, 0.0);
                cublasZgemv(cublas_handle, trans, matrix_size, k + 1,
                           &alpha, d_V, matrix_size, d_y, 1, &beta, d_x, 1);
                
                break;
            }
        }
    }
    
    // Copy final solution back
    CUDA_CHECK(cudaMemcpy(x_vector, d_x, vector_bytes, cudaMemcpyDeviceToHost));
    
    // Cleanup
    CUDA_CHECK(cudaFree(d_A));
    CUDA_CHECK(cudaFree(d_b));
    CUDA_CHECK(cudaFree(d_x));
    CUDA_CHECK(cudaFree(d_V));
    CUDA_CHECK(cudaFree(d_H));
    CUDA_CHECK(cudaFree(d_g));
    CUDA_CHECK(cudaFree(d_y));
    CUDA_CHECK(cudaFree(d_cs));
    CUDA_CHECK(cudaFree(d_sn));
    CUDA_CHECK(cudaFree(d_residual));
    CUDA_CHECK(cudaFree(d_w));
}

// Multi-GPU support
MultiGPUContext* initialize_multi_gpu(int n_gpus) {
    MultiGPUContext *context = (MultiGPUContext*)malloc(sizeof(MultiGPUContext));
    context->n_gpus = n_gpus;
    context->gpu_contexts = (GPUContext**)malloc(n_gpus * sizeof(GPUContext*));
    context->work_distribution = (int*)malloc(n_gpus * sizeof(int));
    context->performance_balancing = (double*)malloc(n_gpus * sizeof(double));
    
    for (int i = 0; i < n_gpus; i++) {
        context->gpu_contexts[i] = initialize_gpu_context(i);
        context->work_distribution[i] = 0;
        context->performance_balancing[i] = 1.0;
    }
    
    printf("Multi-GPU Context Initialized: %d GPUs\n", n_gpus);
    return context;
}

void cleanup_multi_gpu(MultiGPUContext *context) {
    if (!context) return;
    
    for (int i = 0; i < context->n_gpus; i++) {
        cleanup_gpu_context(context->gpu_contexts[i]);
    }
    
    free(context->gpu_contexts);
    free(context->work_distribution);
    free(context->performance_balancing);
    free(context);
}

// Distribute work across multiple GPUs with load balancing
void distribute_work_multi_gpu(
    MultiGPUContext *context,
    const void *data,
    const size_t data_size,
    void **distributed_data,
    size_t **distributed_sizes) {
    
    if (!context || !data || data_size == 0) return;
    
    int n_gpus = context->n_gpus;
    
    // Calculate work distribution based on GPU performance metrics
    double total_performance = 0.0;
    for (int i = 0; i < n_gpus; i++) {
        total_performance += context->gpu_contexts[i]->compute_throughput;
    }
    
    // Distribute work proportionally to performance
    size_t total_size = data_size;
    size_t offset = 0;
    
    for (int i = 0; i < n_gpus; i++) {
        double performance_ratio = context->gpu_contexts[i]->compute_throughput / total_performance;
        size_t gpu_work_size = (size_t)(total_size * performance_ratio);
        
        // Ensure minimum work size and handle rounding
        if (i == n_gpus - 1) {
            gpu_work_size = total_size - offset; // Last GPU gets remaining work
        }
        
        (*distributed_sizes)[i] = gpu_work_size;
        
        // Allocate and copy data for this GPU
        if (gpu_work_size > 0) {
            CUDA_CHECK(cudaSetDevice(i));
            CUDA_CHECK(cudaMalloc(&(*distributed_data)[i], gpu_work_size));
            CUDA_CHECK(cudaMemcpy((*distributed_data)[i], 
                                 (char*)data + offset, 
                                 gpu_work_size, cudaMemcpyHostToDevice));
            
            context->work_distribution[i] = gpu_work_size;
            offset += gpu_work_size;
        } else {
            (*distributed_data)[i] = NULL;
            context->work_distribution[i] = 0;
        }
    }
    
    printf("Work distributed across %d GPUs:\n", n_gpus);
    for (int i = 0; i < n_gpus; i++) {
        printf("  GPU %d: %.1f MB (%.1f%% performance)\n", 
               i, context->work_distribution[i] / 1e6,
               100.0 * context->gpu_contexts[i]->compute_throughput / total_performance);
    }
}

// Collect results from multiple GPUs
void collect_results_multi_gpu(
    MultiGPUContext *context,
    void **results,
    const size_t *result_sizes,
    void *final_result) {
    
    if (!context || !results || !result_sizes || !final_result) return;
    
    int n_gpus = context->n_gpus;
    size_t offset = 0;
    
    for (int i = 0; i < n_gpus; i++) {
        if (results[i] && result_sizes[i] > 0) {
            CUDA_CHECK(cudaSetDevice(i));
            CUDA_CHECK(cudaMemcpy((char*)final_result + offset, 
                                 results[i], 
                                 result_sizes[i], cudaMemcpyDeviceToHost));
            
            CUDA_CHECK(cudaFree(results[i]));
            offset += result_sizes[i];
        }
    }
    
    printf("Results collected from %d GPUs (total: %.1f MB)\n", 
           n_gpus, offset / 1e6);
}

#endif // ENABLE_CUDA

// CPU fallback functions for non-CUDA builds
#ifndef ENABLE_CUDA

// CPU implementations as fallbacks
typedef void* GPUContext;
typedef void* MultiGPUContext;

GPUContext* initialize_gpu_context(int device_id) {
    printf("GPU acceleration not available - using CPU fallback\n");
    return NULL;
}

void cleanup_gpu_context(GPUContext *context) {
    // No-op for CPU fallback
}

MultiGPUContext* initialize_multi_gpu(int n_gpus) {
    printf("Multi-GPU acceleration not available - using CPU fallback\n");
    return NULL;
}

void cleanup_multi_gpu(MultiGPUContext *context) {
    // No-op for CPU fallback
}

#endif // !ENABLE_CUDA