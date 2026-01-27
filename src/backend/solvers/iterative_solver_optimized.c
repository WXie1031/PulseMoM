/*********************************************************************
 * 优化版迭代求解器并行化实现
 * 包含GMRES、BiCGSTAB和预处理技术的GPU加速
 *********************************************************************/

#include "gpu_parallelization_optimized.h"
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <omp.h>
#include <math.h>

// 优化的求解器参数结构
typedef struct {
    int max_iterations;
    double tolerance;
    int restart_frequency;
    int preconditioner_type;
    int orthogonalization_method;
    int adaptive_tolerance;
    double* residual_history;
    int residual_history_size;
    double convergence_rate;
    int auto_restart;
} OptimizedSolverParams;

// 优化的GMRES工作空间
typedef struct {
    double** V;          // Krylov子空间基向量
    double* H;           // Hessenberg矩阵
    double* cs;          // Givens旋转余弦
    double* sn;          // Givens旋转正弦
    double* s;           // 右手边向量
    double* y;           // 最小二乘解
    double* w;           // 工作向量
    double* residual;    // 残差向量
    int basis_size;
    int max_basis_size;
    double* d_V;         // 设备上的Krylov子空间
    double* d_H;         // 设备上的Hessenberg矩阵
    double* d_work;      // 设备工作空间
    cublasHandle_t cublas_handle;
    cusolverDnHandle_t cusolver_handle;
    cudaStream_t stream;
} OptimizedGMRESWorkspace;

// 优化的预处理器结构
typedef struct {
    int type;                    // 预处理器类型
    double* M_inv;              // 预处理矩阵逆
    double* LU_factors;         // LU分解因子
    int* pivot_indices;         // 主元索引
    int* gpu_assignments;       // GPU分配
    int num_blocks;             // 块数量
    double* d_M_inv;            // 设备上的逆矩阵
    double* d_LU_factors;       // 设备上的LU因子
    int* d_pivot_indices;       // 设备上的主元索引
    cublasHandle_t* cublas_handles;
    cusolverDnHandle_t* cusolver_handles;
    cudaStream_t* streams;
    int num_gpus;
} OptimizedPreconditioner;

// 优化的多GPU GMRES求解器
typedef struct {
    OptimizedGMRESWorkspace* workspace;
    OptimizedPreconditioner* preconditioner;
    OptimizedSolverParams* params;
    int num_gpus;
    int* gpu_assignments;
    double* convergence_history;
    int iteration_count;
    double final_residual;
    int converged;
} OptimizedGMRESSolver;

// 初始化优化的GMRES工作空间
OptimizedGMRESWorkspace* init_optimized_gmres_workspace(int max_basis_size, int vector_size, int num_gpus) {
    OptimizedGMRESWorkspace* workspace = (OptimizedGMRESWorkspace*)malloc(sizeof(OptimizedGMRESWorkspace));
    
    workspace->max_basis_size = max_basis_size;
    workspace->basis_size = 0;
    
    // 分配主机内存
    workspace->V = (double**)malloc(max_basis_size * sizeof(double*));
    workspace->H = (double*)malloc((max_basis_size + 1) * max_basis_size * sizeof(double));
    workspace->cs = (double*)malloc(max_basis_size * sizeof(double));
    workspace->sn = (double*)malloc(max_basis_size * sizeof(double));
    workspace->s = (double*)malloc((max_basis_size + 1) * sizeof(double));
    workspace->y = (double*)malloc(max_basis_size * sizeof(double));
    workspace->w = (double*)malloc(vector_size * sizeof(double));
    workspace->residual = (double*)malloc(vector_size * sizeof(double));
    
    // 初始化设备资源
    cudaSetDevice(0);
    cudaStreamCreate(&workspace->stream);
    cublasCreate(&workspace->cublas_handle);
    cublasSetStream(workspace->cublas_handle, workspace->stream);
    cusolverDnCreate(&workspace->cusolver_handle);
    cusolverDnSetStream(workspace->cusolver_handle, workspace->stream);
    
    // 分配设备内存
    size_t vector_bytes = vector_size * sizeof(double);
    size_t hessenberg_bytes = (max_basis_size + 1) * max_basis_size * sizeof(double);
    
    cudaMalloc(&workspace->d_V, max_basis_size * vector_bytes);
    cudaMalloc(&workspace->d_H, hessenberg_bytes);
    cudaMalloc(&workspace->d_work, vector_bytes * 3);  // 额外工作空间
    
    // 初始化Hessenberg矩阵
    cudaMemset(workspace->d_H, 0, hessenberg_bytes);
    
    return workspace;
}

// 优化的Arnoldi过程
void optimized_arnoldi_process(
    OptimizedGMRESWorkspace* workspace,
    const double* A, const double* v,
    int vector_size, int current_k,
    cublasHandle_t handle
) {
    double alpha = 1.0, beta = 0.0;
    
    // 复制当前向量到设备
    cudaMemcpyAsync(workspace->d_V + current_k * vector_size, v, 
                   vector_size * sizeof(double), cudaMemcpyHostToDevice, workspace->stream);
    
    // 矩阵向量乘法: w = A * v_k
    cublasDgemv(handle, CUBLAS_OP_N, vector_size, vector_size,
                &alpha, A, vector_size,
                workspace->d_V + current_k * vector_size, 1,
                &beta, workspace->d_work, 1);
    
    cudaStreamSynchronize(workspace->stream);
    
    // 正交化过程（使用改进的Gram-Schmidt）
    for (int j = 0; j <= current_k; j++) {
        double h_jk = 0.0;
        
        // 计算内积: h_jk = v_j^T * w
        cublasDdot(handle, vector_size,
                   workspace->d_V + j * vector_size, 1,
                   workspace->d_work, 1,
                   &h_jk);
        
        // 更新Hessenberg矩阵
        workspace->H[j * workspace->max_basis_size + current_k] = h_jk;
        
        // 更新工作向量: w = w - h_jk * v_j
        alpha = -h_jk;
        cublasDaxpy(handle, vector_size,
                   &alpha,
                   workspace->d_V + j * vector_size, 1,
                   workspace->d_work, 1);
    }
    
    // 计算当前列的范数
    double h_norm = 0.0;
    cublasDnrm2(handle, vector_size, workspace->d_work, 1, &h_norm);
    
    workspace->H[(current_k + 1) * workspace->max_basis_size + current_k] = h_norm;
    
    // 归一化得到下一个基向量
    if (h_norm > 1e-12) {
        alpha = 1.0 / h_norm;
        cublasDscal(handle, vector_size, &alpha, workspace->d_work, 1);
        
        // 复制到下一个位置
        cudaMemcpyAsync(workspace->d_V + (current_k + 1) * vector_size,
                       workspace->d_work, vector_size * sizeof(double),
                       cudaMemcpyDeviceToDevice, workspace->stream);
    }
    
    cudaStreamSynchronize(workspace->stream);
}

// 优化的Givens旋转
void apply_givens_rotation(
    double* H, double* cs, double* sn,
    int k, int max_basis_size
) {
    // 在设备上执行Givens旋转
    double* d_H_k = workspace->d_H + k * max_basis_size;
    
    for (int i = 0; i < k; i++) {
        double temp = cs[i] * H[i * max_basis_size + k] + sn[i] * H[(i + 1) * max_basis_size + k];
        H[(i + 1) * max_basis_size + k] = -sn[i] * H[i * max_basis_size + k] + cs[i] * H[(i + 1) * max_basis_size + k];
        H[i * max_basis_size + k] = temp;
    }
    
    // 计算新的Givens旋转
    double nu = sqrt(H[k * max_basis_size + k] * H[k * max_basis_size + k] + 
                    H[(k + 1) * max_basis_size + k] * H[(k + 1) * max_basis_size + k]);
    
    cs[k] = H[k * max_basis_size + k] / nu;
    sn[k] = H[(k + 1) * max_basis_size + k] / nu;
    
    // 应用新的旋转
    H[k * max_basis_size + k] = cs[k] * H[k * max_basis_size + k] + sn[k] * H[(k + 1) * max_basis_size + k];
    H[(k + 1) * max_basis_size + k] = 0.0;
}

// 优化的预处理技术
void apply_optimized_preconditioner(
    OptimizedPreconditioner* preconditioner,
    const double* r, double* z,
    int vector_size, int gpu_id
) {
    cudaSetDevice(gpu_id);
    cudaStream_t stream = preconditioner->streams[gpu_id];
    cublasHandle_t handle = preconditioner->cublas_handles[gpu_id];
    
    double* d_r, *d_z;
    cudaMalloc(&d_r, vector_size * sizeof(double));
    cudaMalloc(&d_z, vector_size * sizeof(double));
    
    cudaMemcpyAsync(d_r, r, vector_size * sizeof(double), cudaMemcpyHostToDevice, stream);
    
    switch (preconditioner->type) {
        case 0:  // 无预处理器
            cublasDcopy(handle, vector_size, d_r, 1, d_z, 1);
            break;
            
        case 1:  // 对角预处理器
            apply_diagonal_preconditioner(handle, preconditioner->d_M_inv, d_r, d_z, vector_size);
            break;
            
        case 2:  // ILU预处理器
            apply_ilu_preconditioner(handle, preconditioner, d_r, d_z, vector_size, gpu_id);
            break;
            
        case 3:  // 块Jacobi预处理器
            apply_block_jacobi_preconditioner(handle, preconditioner, d_r, d_z, vector_size, gpu_id);
            break;
    }
    
    cudaMemcpyAsync(z, d_z, vector_size * sizeof(double), cudaMemcpyDeviceToHost, stream);
    cudaStreamSynchronize(stream);
    
    cudaFree(d_r);
    cudaFree(d_z);
}

// 对角预处理器
void apply_diagonal_preconditioner(
    cublasHandle_t handle, const double* d_M_inv,
    const double* d_r, double* d_z, int size
) {
    // 元素级乘法: z = M_inv * r
    cublasDtbm(handle, CUBLAS_SIDE_LEFT, CUBLAS_FILL_MODE_UPPER,
               CUBLAS_OP_N, CUBLAS_DIAG_NON_UNIT,
               size, 0, d_M_inv, 1, d_r, 1, d_z, 1);
}

// ILU预处理器
void apply_ilu_preconditioner(
    cublasHandle_t handle, OptimizedPreconditioner* preconditioner,
    const double* d_r, double* d_z, int size, int gpu_id
) {
    // 前向替换: L * y = r
    double* d_y;
    cudaMalloc(&d_y, size * sizeof(double));
    
    cublasDtrsv(handle, CUBLAS_FILL_MODE_LOWER, CUBLAS_OP_N, CUBLAS_DIAG_UNIT,
                size, preconditioner->d_LU_factors, size, d_r, 1);
    
    // 后向替换: U * z = y
    cublasDtrsv(handle, CUBLAS_FILL_MODE_UPPER, CUBLAS_OP_N, CUBLAS_DIAG_NON_UNIT,
                size, preconditioner->d_LU_factors, size, d_y, 1, d_z, 1);
    
    cudaFree(d_y);
}

// 块Jacobi预处理器
void apply_block_jacobi_preconditioner(
    cublasHandle_t handle, OptimizedPreconditioner* preconditioner,
    const double* d_r, double* d_z, int size, int gpu_id
) {
    int block_size = size / preconditioner->num_blocks;
    int remainder = size % preconditioner->num_blocks;
    
    // 并行处理每个块
    #pragma omp parallel for num_threads(preconditioner->num_gpus)
    for (int block = 0; block < preconditioner->num_blocks; block++) {
        int thread_gpu = omp_get_thread_num();
        cudaSetDevice(thread_gpu);
        
        int start_idx = block * block_size + (block < remainder ? block : remainder);
        int end_idx = start_idx + block_size + (block < remainder ? 1 : 0);
        int local_size = end_idx - start_idx;
        
        if (local_size > 0) {
            double* d_block_r = d_r + start_idx;
            double* d_block_z = d_z + start_idx;
            
            // 求解块系统
            cublasDgetrs(preconditioner->cusolver_handles[thread_gpu],
                        CUBLAS_OP_N, local_size, 1,
                        preconditioner->d_LU_factors + block * block_size * block_size,
                        block_size, preconditioner->d_pivot_indices + block * block_size,
                        d_block_r, block_size, d_block_z, block_size);
        }
    }
}

// 优化的GMRES求解器
int optimized_gmres_solver(
    const double* A, const double* b, double* x,
    int size, OptimizedGMRESSolver* solver
) {
    OptimizedGMRESWorkspace* workspace = solver->workspace;
    OptimizedPreconditioner* preconditioner = solver->preconditioner;
    OptimizedSolverParams* params = solver->params;
    
    double residual_norm, b_norm;
    int num_gpus = solver->num_gpus;
    
    // 计算初始残差: r = b - A * x
    compute_residual(A, b, x, workspace->residual, size);
    
    // 应用预处理器
    double* z = (double*)malloc(size * sizeof(double));
    apply_optimized_preconditioner(preconditioner, workspace->residual, z, size, 0);
    
    // 计算残差范数
    cublasDnrm2(workspace->cublas_handle, size, z, 1, &residual_norm);
    cublasDnrm2(workspace->cublas_handle, size, b, 1, &b_norm);
    
    double relative_tolerance = params->tolerance * b_norm;
    
    if (residual_norm < relative_tolerance) {
        free(z);
        return 0;  // 已经收敛
    }
    
    // 初始化右手边向量
    workspace->s[0] = residual_norm;
    for (int i = 1; i <= params->max_iterations; i++) {
        workspace->s[i] = 0.0;
    }
    
    // 归一化第一个基向量
    double alpha = 1.0 / residual_norm;
    cublasDscal(workspace->cublas_handle, size, &alpha, z, 1);
    
    // GMRES主循环
    int iteration;
    for (iteration = 0; iteration < params->max_iterations; iteration++) {
        int k = iteration % params->restart_frequency;
        
        // Arnoldi过程
        optimized_arnoldi_process(workspace, A, z, size, k, workspace->cublas_handle);
        
        // 应用Givens旋转
        apply_givens_rotation(workspace->H, workspace->cs, workspace->sn, k, workspace->max_basis_size);
        
        // 更新右手边向量
        workspace->s[k + 1] = -workspace->sn[k] * workspace->s[k];
        workspace->s[k] = workspace->cs[k] * workspace->s[k];
        
        // 检查收敛性
        residual_norm = fabs(workspace->s[k + 1]);
        
        // 记录收敛历史
        if (params->residual_history && iteration < params->residual_history_size) {
            params->residual_history[iteration] = residual_norm / b_norm;
        }
        
        // 自适应容差调整
        if (params->adaptive_tolerance && iteration > 10) {
            double convergence_rate = params->residual_history[iteration-1] / 
                                   params->residual_history[iteration-2];
            if (convergence_rate > 0.9) {
                params->tolerance *= 1.1;  // 减慢收敛
            } else if (convergence_rate < 0.5) {
                params->tolerance *= 0.9;  // 加快收敛
            }
        }
        
        if (residual_norm < relative_tolerance) {
            break;
        }
        
        // 重启检查
        if (k == params->restart_frequency - 1) {
            // 求解最小二乘问题并更新解
            solve_least_squares(workspace, k);
            update_solution(workspace, x, z, size, k);
            
            // 重新计算残差
            compute_residual(A, b, x, workspace->residual, size);
            apply_optimized_preconditioner(preconditioner, workspace->residual, z, size, 0);
            
            // 重置工作空间
            workspace->basis_size = 0;
            workspace->s[0] = residual_norm;
        }
    }
    
    // 最终解更新
    solve_least_squares(workspace, iteration % params->restart_frequency);
    update_solution(workspace, x, z, size, iteration % params->restart_frequency);
    
    solver->iteration_count = iteration;
    solver->final_residual = residual_norm / b_norm;
    solver->converged = (residual_norm < relative_tolerance);
    
    free(z);
    return solver->converged ? 0 : 1;
}

// 求解最小二乘问题
void solve_least_squares(OptimizedGMRESWorkspace* workspace, int k) {
    // 回代求解上三角系统
    for (int i = k; i >= 0; i--) {
        workspace->y[i] = workspace->s[i];
        
        for (int j = i + 1; j <= k; j++) {
            workspace->y[i] -= workspace->H[i * workspace->max_basis_size + j] * workspace->y[j];
        }
        
        workspace->y[i] /= workspace->H[i * workspace->max_basis_size + i];
    }
}

// 更新解向量
void update_solution(
    OptimizedGMRESWorkspace* workspace,
    double* x, double* z, int size, int k
) {
    // x = x + V * y
    double alpha = 1.0, beta = 1.0;
    
    for (int i = 0; i <= k; i++) {
        cublasDaxpy(workspace->cublas_handle, size,
                   &workspace->y[i],
                   workspace->d_V + i * size, 1,
                   x, 1);
    }
}

// 计算残差
static void compute_residual(const double* A, const double* b, const double* x, double* r, int size) {
    // r = b - A * x
    double alpha = -1.0, beta = 1.0;
    cublasDgemv(workspace->cublas_handle, CUBLAS_OP_N, size, size,
                &alpha, A, size, x, 1,
                &beta, r, 1);
}

// 初始化优化的求解器参数
OptimizedSolverParams* init_optimized_solver_params() {
    OptimizedSolverParams* params = (OptimizedSolverParams*)malloc(sizeof(OptimizedSolverParams));
    
    params->max_iterations = 1000;
    params->tolerance = 1e-6;
    params->restart_frequency = 30;
    params->preconditioner_type = 3;  // 块Jacobi
    params->orthogonalization_method = 1;  // 改进的Gram-Schmidt
    params->adaptive_tolerance = 1;
    params->residual_history_size = 100;
    params->residual_history = (double*)malloc(params->residual_history_size * sizeof(double));
    params->convergence_rate = 0.0;
    params->auto_restart = 1;
    
    return params;
}

// 初始化优化的GMRES求解器
OptimizedGMRESSolver* init_optimized_gmres_solver(int size, int num_gpus) {
    OptimizedGMRESSolver* solver = (OptimizedGMRESSolver*)malloc(sizeof(OptimizedGMRESSolver));
    
    solver->num_gpus = num_gpus;
    solver->iteration_count = 0;
    solver->final_residual = 0.0;
    solver->converged = 0;
    
    // 初始化工作空间
    solver->workspace = init_optimized_gmres_workspace(30, size, num_gpus);
    
    // 初始化预处理器
    solver->preconditioner = init_optimized_preconditioner(size, num_gpus);
    
    // 初始化求解器参数
    solver->params = init_optimized_solver_params();
    
    // 分配GPU分配数组
    solver->gpu_assignments = (int*)malloc(size * sizeof(int));
    
    // 初始化收敛历史
    solver->convergence_history = (double*)malloc(1000 * sizeof(double));
    
    return solver;
}

// 初始化优化的预处理器
OptimizedPreconditioner* init_optimized_preconditioner(int size, int num_gpus) {
    OptimizedPreconditioner* preconditioner = (OptimizedPreconditioner*)malloc(sizeof(OptimizedPreconditioner));
    
    preconditioner->type = 3;  // 块Jacobi
    preconditioner->num_blocks = num_gpus * 4;  // 每个GPU4个块
    preconditioner->num_gpus = num_gpus;
    
    // 分配主机内存
    int block_size = size / preconditioner->num_blocks;
    preconditioner->M_inv = (double*)malloc(size * sizeof(double));
    preconditioner->LU_factors = (double*)malloc(size * size * sizeof(double));
    preconditioner->pivot_indices = (int*)malloc(size * sizeof(int));
    preconditioner->gpu_assignments = (int*)malloc(preconditioner->num_blocks * sizeof(int));
    
    // 分配设备资源
    preconditioner->cublas_handles = (cublasHandle_t*)malloc(num_gpus * sizeof(cublasHandle_t));
    preconditioner->cusolver_handles = (cusolverDnHandle_t*)malloc(num_gpus * sizeof(cusolverDnHandle_t));
    preconditioner->streams = (cudaStream_t*)malloc(num_gpus * sizeof(cudaStream_t));
    
    for (int i = 0; i < num_gpus; i++) {
        cudaSetDevice(i);
        cudaStreamCreate(&preconditioner->streams[i]);
        cublasCreate(&preconditioner->cublas_handles[i]);
        cublasSetStream(preconditioner->cublas_handles[i], preconditioner->streams[i]);
        cusolverDnCreate(&preconditioner->cusolver_handles[i]);
        cusolverDnSetStream(preconditioner->cusolver_handles[i], preconditioner->streams[i]);
    }
    
    // 分配设备内存
    cudaMalloc(&preconditioner->d_M_inv, size * sizeof(double));
    cudaMalloc(&preconditioner->d_LU_factors, size * size * sizeof(double));
    cudaMalloc(&preconditioner->d_pivot_indices, size * sizeof(int));
    
    return preconditioner;
}

// 清理优化的GMRES求解器
void cleanup_optimized_gmres_solver(OptimizedGMRESSolver* solver) {
    if (solver->workspace) {
        cudaFree(solver->workspace->d_V);
        cudaFree(solver->workspace->d_H);
        cudaFree(solver->workspace->d_work);
        
        cublasDestroy(solver->workspace->cublas_handle);
        cusolverDnDestroy(solver->workspace->cusolver_handle);
        cudaStreamDestroy(solver->workspace->stream);
        
        free(solver->workspace->V);
        free(solver->workspace->H);
        free(solver->workspace->cs);
        free(solver->workspace->sn);
        free(solver->workspace->s);
        free(solver->workspace->y);
        free(solver->workspace->w);
        free(solver->workspace->residual);
        
        free(solver->workspace);
    }
    
    if (solver->preconditioner) {
        for (int i = 0; i < solver->num_gpus; i++) {
            cudaSetDevice(i);
            cublasDestroy(solver->preconditioner->cublas_handles[i]);
            cusolverDnDestroy(solver->preconditioner->cusolver_handles[i]);
            cudaStreamDestroy(solver->preconditioner->streams[i]);
        }
        
        cudaFree(solver->preconditioner->d_M_inv);
        cudaFree(solver->preconditioner->d_LU_factors);
        cudaFree(solver->preconditioner->d_pivot_indices);
        
        free(solver->preconditioner->M_inv);
        free(solver->preconditioner->LU_factors);
        free(solver->preconditioner->pivot_indices);
        free(solver->preconditioner->gpu_assignments);
        free(solver->preconditioner->cublas_handles);
        free(solver->preconditioner->cusolver_handles);
        free(solver->preconditioner->streams);
        
        free(solver->preconditioner);
    }
    
    if (solver->params) {
        free(solver->params->residual_history);
        free(solver->params);
    }
    
    free(solver->gpu_assignments);
    free(solver->convergence_history);
    free(solver);
}

// 生成求解器性能报告
void generate_solver_performance_report(OptimizedGMRESSolver* solver, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "=== GMRES求解器性能报告 ===\n\n");
    fprintf(fp, "迭代次数: %d\n", solver->iteration_count);
    fprintf(fp, "最终残差: %.3e\n", solver->final_residual);
    fprintf(fp, "收敛状态: %s\n", solver->converged ? "已收敛" : "未收敛");
    fprintf(fp, "GPU数量: %d\n\n", solver->num_gpus);
    
    fprintf(fp, "求解器参数:\n");
    fprintf(fp, "  最大迭代次数: %d\n", solver->params->max_iterations);
    fprintf(fp, "  容差: %.3e\n", solver->params->tolerance);
    fprintf(fp, "  重启频率: %d\n", solver->params->restart_frequency);
    fprintf(fp, "  预处理器类型: %d\n", solver->params->preconditioner_type);
    fprintf(fp, "  自适应容差: %s\n", solver->params->adaptive_tolerance ? "启用" : "禁用");
    
    if (solver->iteration_count > 0 && solver->params->residual_history) {
        fprintf(fp, "\n收敛历史:\n");
        int step = solver->iteration_count / 20;  // 显示20个点
        if (step == 0) step = 1;
        
        for (int i = 0; i < solver->iteration_count; i += step) {
            fprintf(fp, "  迭代 %4d: 残差 = %.3e\n", 
                   i, solver->params->residual_history[i]);
        }
        fprintf(fp, "  迭代 %4d: 残差 = %.3e\n", 
               solver->iteration_count, solver->final_residual);
    }
    
    fclose(fp);
}