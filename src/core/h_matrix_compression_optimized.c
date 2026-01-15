/*********************************************************************
 * 优化版H矩阵压缩并行化实现
 * 包含自适应交叉近似(ACA)和分层矩阵操作的并行化优化
 *********************************************************************/

#include "gpu_parallelization_optimized.h"
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <omp.h>
#include <math.h>

// 优化的块聚类结构
typedef struct {
    int* indices;
    int size;
    double* bounding_box;
    int level;
    int parent;
    int* children;
    int num_children;
    double admissibility_parameter;
} OptimizedBlockCluster;

// 优化的ACA参数结构
typedef struct {
    double tolerance;
    int max_rank;
    int adaptive_rank;
    int block_size;
    int oversampling;
    double compression_ratio;
} OptimizedACAParams;

// 分层矩阵块结构
typedef struct {
    int row_start, row_end;
    int col_start, col_end;
    int rank;
    int is_low_rank;
    double* U;  // 左奇异向量
    double* V;  // 右奇异向量
    double* D;  // 稠密矩阵（如果是稠密块）
    double compression_error;
    int gpu_id;
    cudaStream_t stream;
} OptimizedHMatrixBlock;

// 优化的H矩阵结构
typedef struct {
    OptimizedHMatrixBlock* blocks;
    int num_blocks;
    int matrix_size;
    int max_rank;
    double total_compression_ratio;
    double avg_compression_time;
    int* gpu_assignments;
    OptimizedACAParams* aca_params;
} OptimizedHMatrix;

// 优化的自适应交叉近似算法
__global__ void optimized_aca_kernel(
    const double* __restrict__ matrix_data,
    const int* __restrict__ row_indices,
    const int* __restrict__ col_indices,
    int block_rows, int block_cols,
    double tolerance, int max_rank,
    double* U, double* V, int* actual_rank,
    double* workspace, int* convergence_flag
) {
    extern __shared__ double shared_mem[];
    
    int tid = threadIdx.x;
    int bid = blockIdx.x;
    int row = threadIdx.y;
    
    // 使用共享内存存储中间结果
    double* shared_row = shared_mem;
    double* shared_col = &shared_mem[block_cols];
    
    // 初始化
    if (tid == 0) {
        *actual_rank = 0;
        *convergence_flag = 0;
    }
    __syncthreads();
    
    // ACA主循环
    for (int k = 0; k < max_rank && !(*convergence_flag); k++) {
        // 选择主元行
        if (row < block_rows && tid == 0) {
            // 找到最大元素作为主元
            double max_val = 0.0;
            int pivot_col = 0;
            
            for (int j = 0; j < block_cols; j++) {
                int global_row = row_indices[row];
                int global_col = col_indices[j];
                double val = matrix_data[global_row * block_cols + global_col];
                
                if (fabs(val) > max_val) {
                    max_val = fabs(val);
                    pivot_col = j;
                }
            }
            
            // 检查收敛性
            if (max_val < tolerance) {
                *convergence_flag = 1;
            }
        }
        __syncthreads();
        
        if (*convergence_flag) break;
        
        // 计算U和V向量
        if (row < block_rows) {
            double u_val = 0.0;
            
            // 计算U向量的元素
            for (int j = tid; j < block_cols; j += blockDim.x) {
                int global_row = row_indices[row];
                int global_col = col_indices[j];
                u_val = matrix_data[global_row * block_cols + j];
                
                // 减去之前秩的贡献
                for (int r = 0; r < k; r++) {
                    u_val -= U[row * max_rank + r] * V[j * max_rank + r];
                }
                
                U[row * max_rank + k] = u_val;
            }
        }
        
        if (row < block_cols) {
            double v_val = 0.0;
            
            // 计算V向量的元素
            for (int i = tid; i < block_rows; i += blockDim.x) {
                int global_row = row_indices[i];
                int global_col = row_indices[row];
                v_val = matrix_data[global_row * block_cols + global_col];
                
                // 减去之前秩的贡献
                for (int r = 0; r < k; r++) {
                    v_val -= U[i * max_rank + r] * V[row * max_rank + r];
                }
                
                V[row * max_rank + k] = v_val;
            }
        }
        
        __syncthreads();
        
        // 更新实际秩
        if (tid == 0) {
            *actual_rank = k + 1;
        }
    }
}

// 优化的分层矩阵乘法
__global__ void optimized_hmatrix_multiplication(
    const OptimizedHMatrixBlock* __restrict__ hmatrix_blocks,
    const double* __restrict__ input_vector,
    double* __restrict__ output_vector,
    int num_blocks, int vector_size
) {
    int tid = threadIdx.x;
    int bid = blockIdx.x;
    
    if (bid >= num_blocks) return;
    
    __shared__ OptimizedHMatrixBlock shared_block;
    if (tid == 0) {
        shared_block = hmatrix_blocks[bid];
    }
    __syncthreads();
    
    OptimizedHMatrixBlock block = shared_block;
    
    if (block.is_low_rank) {
        // 低秩矩阵乘法：U * (V^T * x)
        extern __shared__ double shared_mem[];
        double* temp_vector = shared_mem;
        
        // 第一阶段：V^T * x
        for (int i = tid; i < block.rank; i += blockDim.x) {
            double sum = 0.0;
            
            for (int j = block.col_start; j <= block.col_end; j++) {
                int v_idx = (j - block.col_start) * block.rank + i;
                sum += block.V[v_idx] * input_vector[j];
            }
            
            temp_vector[i] = sum;
        }
        __syncthreads();
        
        // 第二阶段：U * temp_vector
        for (int i = tid + block.row_start; i <= block.row_end; i += blockDim.x) {
            double sum = 0.0;
            
            for (int k = 0; k < block.rank; k++) {
                int u_idx = (i - block.row_start) * block.rank + k;
                sum += block.U[u_idx] * temp_vector[k];
            }
            
            atomicAdd(&output_vector[i], sum);
        }
    } else {
        // 稠密矩阵乘法
        for (int i = tid + block.row_start; i <= block.row_end; i += blockDim.x) {
            double sum = 0.0;
            
            for (int j = block.col_start; j <= block.col_end; j++) {
                int d_idx = (i - block.row_start) * (block.col_end - block.col_start + 1) + 
                           (j - block.col_start);
                sum += block.D[d_idx] * input_vector[j];
            }
            
            atomicAdd(&output_vector[i], sum);
        }
    }
}

// 优化的块聚类算法
void optimized_block_clustering(
    const double* coordinates, int num_points,
    int max_level, double admissibility_parameter,
    OptimizedBlockCluster** clusters, int* num_clusters
) {
    *num_clusters = 0;
    int max_clusters = num_points * 2;
    *clusters = (OptimizedBlockCluster*)malloc(max_clusters * sizeof(OptimizedBlockCluster));
    
    // 递归空间分割聚类
    recursive_spatial_clustering(coordinates, num_points, 0, max_level,
                                admissibility_parameter, clusters, num_clusters, 0, -1);
}

// 递归空间分割聚类
void recursive_spatial_clustering(
    const double* coordinates, int num_points, int current_level, int max_level,
    double admissibility_parameter, OptimizedBlockCluster** clusters, int* num_clusters,
    int cluster_id, int parent_id
) {
    OptimizedBlockCluster* cluster = &(*clusters)[cluster_id];
    
    // 计算边界框
    compute_bounding_box(coordinates, num_points, cluster->bounding_box);
    
    cluster->level = current_level;
    cluster->parent = parent_id;
    cluster->admissibility_parameter = admissibility_parameter;
    
    // 检查是否需要进一步分割
    if (current_level < max_level && num_points > 32) {
        // 找到最长的维度进行分割
        int split_dim = find_longest_dimension(cluster->bounding_box);
        double split_value = (cluster->bounding_box[split_dim] + 
                             cluster->bounding_box[split_dim + 3]) / 2.0;
        
        // 分割点集
        int* left_indices = (int*)malloc(num_points * sizeof(int));
        int* right_indices = (int*)malloc(num_points * sizeof(int));
        int left_count = 0, right_count = 0;
        
        for (int i = 0; i < num_points; i++) {
            if (coordinates[i * 3 + split_dim] < split_value) {
                left_indices[left_count++] = i;
            } else {
                right_indices[right_count++] = i;
            }
        }
        
        // 创建子聚类
        cluster->num_children = 2;
        cluster->children = (int*)malloc(2 * sizeof(int));
        
        int left_child_id = (*num_clusters)++;
        int right_child_id = (*num_clusters)++;
        
        cluster->children[0] = left_child_id;
        cluster->children[1] = right_child_id;
        
        // 递归处理子聚类
        recursive_spatial_clustering(coordinates, left_count, current_level + 1, max_level,
                                    admissibility_parameter, clusters, num_clusters,
                                    left_child_id, cluster_id);
        recursive_spatial_clustering(coordinates, right_count, current_level + 1, max_level,
                                    admissibility_parameter, clusters, num_clusters,
                                    right_child_id, cluster_id);
        
        free(left_indices);
        free(right_indices);
    } else {
        // 叶节点，存储实际索引
        cluster->indices = (int*)malloc(num_points * sizeof(int));
        cluster->size = num_points;
        
        for (int i = 0; i < num_points; i++) {
            cluster->indices[i] = i;
        }
        
        cluster->num_children = 0;
        cluster->children = NULL;
    }
}

// 优化的可接受性检查
__device__ int optimized_admissibility_check(
    const OptimizedBlockCluster* cluster1,
    const OptimizedBlockCluster* cluster2,
    double parameter
) {
    // 计算两个聚类中心之间的距离
    double center1[3], center2[3];
    
    for (int dim = 0; dim < 3; dim++) {
        center1[dim] = (cluster1->bounding_box[dim] + cluster1->bounding_box[dim + 3]) / 2.0;
        center2[dim] = (cluster2->bounding_box[dim] + cluster2->bounding_box[dim + 3]) / 2.0;
    }
    
    double distance = sqrt(
        (center1[0] - center2[0]) * (center1[0] - center2[0]) +
        (center1[1] - center2[1]) * (center1[1] - center2[1]) +
        (center1[2] - center2[2]) * (center1[2] - center2[2])
    );
    
    // 计算两个聚类的直径
    double diameter1 = 0.0, diameter2 = 0.0;
    
    for (int dim = 0; dim < 3; dim++) {
        double dim_size1 = cluster1->bounding_box[dim + 3] - cluster1->bounding_box[dim];
        double dim_size2 = cluster2->bounding_box[dim + 3] - cluster2->bounding_box[dim];
        diameter1 += dim_size1 * dim_size1;
        diameter2 += dim_size2 * dim_size2;
    }
    
    diameter1 = sqrt(diameter1);
    diameter2 = sqrt(diameter2);
    
    double min_diameter = fmin(diameter1, diameter2);
    
    // 可接受性条件：距离 > 参数 * min(直径1, 直径2)
    return (distance > parameter * min_diameter);
}

// 优化的H矩阵构建
OptimizedHMatrix* build_optimized_hmatrix(
    const double* impedance_matrix, int matrix_size,
    const double* coordinates, int max_rank, double tolerance
) {
    OptimizedHMatrix* hmatrix = (OptimizedHMatrix*)malloc(sizeof(OptimizedHMatrix));
    hmatrix->matrix_size = matrix_size;
    hmatrix->max_rank = max_rank;
    hmatrix->total_compression_ratio = 0.0;
    hmatrix->avg_compression_time = 0.0;
    
    // 初始化ACA参数
    hmatrix->aca_params = (OptimizedACAParams*)malloc(sizeof(OptimizedACAParams));
    hmatrix->aca_params->tolerance = tolerance;
    hmatrix->aca_params->max_rank = max_rank;
    hmatrix->aca_params->adaptive_rank = 1;
    hmatrix->aca_params->block_size = 64;
    hmatrix->aca_params->oversampling = 5;
    hmatrix->aca_params->compression_ratio = 0.0;
    
    // 构建块聚类
    OptimizedBlockCluster* clusters;
    int num_clusters;
    optimized_block_clustering(coordinates, matrix_size, 8, 1.0, &clusters, &num_clusters);
    
    // 分配GPU资源
    int num_gpus;
    cudaGetDeviceCount(&num_gpus);
    hmatrix->gpu_assignments = (int*)malloc(num_clusters * num_clusters * sizeof(int));
    
    // 并行构建H矩阵块
    int total_blocks = 0;
    for (int i = 0; i < num_clusters; i++) {
        for (int j = 0; j < num_clusters; j++) {
            if (optimized_admissibility_check(&clusters[i], &clusters[j], 1.0)) {
                total_blocks++;
            }
        }
    }
    
    hmatrix->num_blocks = total_blocks;
    hmatrix->blocks = (OptimizedHMatrixBlock*)malloc(total_blocks * sizeof(OptimizedHMatrixBlock));
    
    // 使用OpenMP并行构建块
    int block_idx = 0;
    #pragma omp parallel for num_threads(num_gpus) schedule(dynamic)
    for (int i = 0; i < num_clusters; i++) {
        for (int j = 0; j < num_clusters; j++) {
            if (optimized_admissibility_check(&clusters[i], &clusters[j], 1.0)) {
                int gpu_id = omp_get_thread_num();
                cudaSetDevice(gpu_id);
                
                OptimizedHMatrixBlock* block = &hmatrix->blocks[block_idx];
                
                // 设置块参数
                block->row_start = clusters[i].indices[0];
                block->row_end = clusters[i].indices[clusters[i].size - 1];
                block->col_start = clusters[j].indices[0];
                block->col_end = clusters[j].indices[clusters[j].size - 1];
                block->gpu_id = gpu_id;
                
                // 在GPU上执行ACA压缩
                double compression_start = omp_get_wtime();
                perform_optimized_aca_compression(
                    impedance_matrix, block, hmatrix->aca_params);
                double compression_end = omp_get_wtime();
                
                hmatrix->avg_compression_time += (compression_end - compression_start);
                
                #pragma omp atomic
                block_idx++;
            }
        }
    }
    
    hmatrix->avg_compression_time /= total_blocks;
    
    // 清理聚类结构
    for (int i = 0; i < num_clusters; i++) {
        if (clusters[i].indices) free(clusters[i].indices);
        if (clusters[i].children) free(clusters[i].children);
    }
    free(clusters);
    
    return hmatrix;
}

// 执行优化的ACA压缩
void perform_optimized_aca_compression(
    const double* matrix, OptimizedHMatrixBlock* block,
    OptimizedACAParams* params
) {
    int block_rows = block->row_end - block->row_start + 1;
    int block_cols = block->col_end - block->col_start + 1;
    
    // 检查是否适合低秩近似
    if (block_rows < params->block_size || block_cols < params->block_size) {
        block->is_low_rank = 0;
        
        // 分配稠密矩阵存储
        cudaMalloc(&block->D, block_rows * block_cols * sizeof(double));
        
        // 复制原始矩阵块
        double* host_D = (double*)malloc(block_rows * block_cols * sizeof(double));
        for (int i = 0; i < block_rows; i++) {
            for (int j = 0; j < block_cols; j++) {
                int global_i = block->row_start + i;
                int global_j = block->col_start + j;
                host_D[i * block_cols + j] = matrix[global_i * block_cols + global_j];
            }
        }
        
        cudaMemcpy(block->D, host_D, block_rows * block_cols * sizeof(double), cudaMemcpyHostToDevice);
        free(host_D);
        
        return;
    }
    
    // 执行ACA压缩
    block->is_low_rank = 1;
    
    // 分配设备内存
    double *d_matrix_block, *d_U, *d_V;
    int *d_actual_rank, *d_convergence_flag;
    double* d_workspace;
    
    cudaMalloc(&d_matrix_block, block_rows * block_cols * sizeof(double));
    cudaMalloc(&d_U, block_rows * params->max_rank * sizeof(double));
    cudaMalloc(&d_V, block_cols * params->max_rank * sizeof(double));
    cudaMalloc(&d_actual_rank, sizeof(int));
    cudaMalloc(&d_convergence_flag, sizeof(int));
    cudaMalloc(&d_workspace, (block_rows + block_cols) * sizeof(double));
    
    // 复制矩阵块到设备
    double* host_matrix_block = (double*)malloc(block_rows * block_cols * sizeof(double));
    for (int i = 0; i < block_rows; i++) {
        for (int j = 0; j < block_cols; j++) {
            int global_i = block->row_start + i;
            int global_j = block->col_start + j;
            host_matrix_block[i * block_cols + j] = matrix[global_i * block_cols + global_j];
        }
    }
    
    cudaMemcpy(d_matrix_block, host_matrix_block, 
               block_rows * block_cols * sizeof(double), cudaMemcpyHostToDevice);
    
    // 配置内核参数
    dim3 block_size(32, 8);
    dim3 grid_size(1);
    size_t shared_mem_size = (block_cols + block_rows) * sizeof(double);
    
    // 执行ACA内核
    optimized_aca_kernel<<<grid_size, block_size, shared_mem_size, block->stream>>>(
        d_matrix_block, NULL, NULL, block_rows, block_cols,
        params->tolerance, params->max_rank, d_U, d_V, d_actual_rank,
        d_workspace, d_convergence_flag
    );
    
    cudaStreamSynchronize(block->stream);
    
    // 获取结果
    cudaMemcpy(&block->rank, d_actual_rank, sizeof(int), cudaMemcpyDeviceToHost);
    
    // 分配主机内存并复制结果
    block->U = (double*)malloc(block_rows * block->rank * sizeof(double));
    block->V = (double*)malloc(block_cols * block->rank * sizeof(double));
    
    cudaMemcpy(block->U, d_U, block_rows * block->rank * sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(block->V, d_V, block_cols * block->rank * sizeof(double), cudaMemcpyDeviceToHost);
    
    // 计算压缩误差
    block->compression_error = compute_compression_error(
        host_matrix_block, block->U, block->V, block_rows, block_cols, block->rank);
    
    // 计算压缩比
    int original_size = block_rows * block_cols;
    int compressed_size = block->rank * (block_rows + block_cols);
    params->compression_ratio = (double)original_size / compressed_size;
    
    // 清理
    cudaFree(d_matrix_block);
    cudaFree(d_U);
    cudaFree(d_V);
    cudaFree(d_actual_rank);
    cudaFree(d_convergence_flag);
    cudaFree(d_workspace);
    free(host_matrix_block);
}

// 计算压缩误差
double compute_compression_error(
    const double* original, const double* U, const double* V,
    int rows, int cols, int rank
) {
    double max_error = 0.0;
    
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double approx_val = 0.0;
            
            for (int k = 0; k < rank; k++) {
                approx_val += U[i * rank + k] * V[j * rank + k];
            }
            
            double error = fabs(original[i * cols + j] - approx_val);
            if (error > max_error) {
                max_error = error;
            }
        }
    }
    
    return max_error;
}

// 优化的H矩阵向量乘法
void optimized_hmatrix_vector_multiplication(
    OptimizedHMatrix* hmatrix, const double* input_vector,
    double* output_vector, int vector_size
) {
    // 初始化输出向量
    cudaMemset(output_vector, 0, vector_size * sizeof(double));
    
    // 并行处理所有块
    int num_gpus;
    cudaGetDeviceCount(&num_gpus);
    
    #pragma omp parallel num_threads(num_gpus)
    {
        int gpu_id = omp_get_thread_num();
        cudaSetDevice(gpu_id);
        
        // 为每个GPU创建流
        cudaStream_t stream;
        cudaStreamCreate(&stream);
        
        // 处理分配给该GPU的块
        #pragma omp for schedule(dynamic)
        for (int i = 0; i < hmatrix->num_blocks; i++) {
            OptimizedHMatrixBlock* block = &hmatrix->blocks[i];
            
            if (block->gpu_id == gpu_id) {
                // 复制输入向量块到设备
                double* d_input_block;
                double* d_output_block;
                
                int input_size = block->col_end - block->col_start + 1;
                int output_size = block->row_end - block->row_start + 1;
                
                cudaMalloc(&d_input_block, input_size * sizeof(double));
                cudaMalloc(&d_output_block, output_size * sizeof(double));
                
                cudaMemcpyAsync(d_input_block, 
                               &input_vector[block->col_start],
                               input_size * sizeof(double),
                               cudaMemcpyHostToDevice, stream);
                
                cudaMemsetAsync(d_output_block, 0, output_size * sizeof(double), stream);
                
                // 执行块乘法
                if (block->is_low_rank) {
                    // 低秩乘法
                    perform_low_rank_multiplication(block, d_input_block, d_output_block, stream);
                } else {
                    // 稠密乘法
                    perform_dense_multiplication(block, d_input_block, d_output_block, stream);
                }
                
                // 复制结果回主机
                cudaMemcpyAsync(&output_vector[block->row_start],
                               d_output_block,
                               output_size * sizeof(double),
                               cudaMemcpyDeviceToHost, stream);
                
                cudaFree(d_input_block);
                cudaFree(d_output_block);
            }
        }
        
        cudaStreamSynchronize(stream);
        cudaStreamDestroy(stream);
    }
}

// 执行低秩矩阵乘法
void perform_low_rank_multiplication(
    OptimizedHMatrixBlock* block,
    const double* d_input, double* d_output,
    cudaStream_t stream
) {
    cublasHandle_t handle;
    cublasCreate(&handle);
    cublasSetStream(handle, stream);
    
    int rows = block->row_end - block->row_start + 1;
    int cols = block->col_end - block->col_start + 1;
    int rank = block->rank;
    
    // 分配临时存储
    double* d_temp;
    cudaMalloc(&d_temp, rank * sizeof(double));
    
    // V^T * input
    double alpha = 1.0, beta = 0.0;
    cublasDgemv(handle, CUBLAS_OP_T,
                cols, rank,
                &alpha, block->V, cols,
                d_input, 1,
                &beta, d_temp, 1);
    
    // U * temp
    cublasDgemv(handle, CUBLAS_OP_N,
                rows, rank,
                &alpha, block->U, rows,
                d_temp, 1,
                &beta, d_output, 1);
    
    cudaFree(d_temp);
    cublasDestroy(handle);
}

// 执行稠密矩阵乘法
void perform_dense_multiplication(
    OptimizedHMatrixBlock* block,
    const double* d_input, double* d_output,
    cudaStream_t stream
) {
    cublasHandle_t handle;
    cublasCreate(&handle);
    cublasSetStream(handle, stream);
    
    int rows = block->row_end - block->row_start + 1;
    int cols = block->col_end - block->col_start + 1;
    
    // D * input
    double alpha = 1.0, beta = 0.0;
    cublasDgemv(handle, CUBLAS_OP_N,
                rows, cols,
                &alpha, block->D, rows,
                d_input, 1,
                &beta, d_output, 1);
    
    cublasDestroy(handle);
}

// 清理优化的H矩阵
void cleanup_optimized_hmatrix(OptimizedHMatrix* hmatrix) {
    for (int i = 0; i < hmatrix->num_blocks; i++) {
        OptimizedHMatrixBlock* block = &hmatrix->blocks[i];
        
        if (block->is_low_rank) {
            if (block->U) free(block->U);
            if (block->V) free(block->V);
        } else {
            if (block->D) cudaFree(block->D);
        }
    }
    
    free(hmatrix->blocks);
    free(hmatrix->gpu_assignments);
    free(hmatrix->aca_params);
    free(hmatrix);
}