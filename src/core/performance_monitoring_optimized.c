/*********************************************************************
 * 优化版性能监控和自动调优实现
 * 包含实时性能分析、自动参数优化和自适应算法
 *********************************************************************/

#include "gpu_parallelization_optimized.h"
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <omp.h>
#include <math.h>
#include <time.h>

// 性能指标类型枚举
typedef enum {
    PERF_COMPUTE_UTILIZATION,
    PERF_MEMORY_BANDWIDTH,
    PERF_MEMORY_USAGE,
    PERF_KERNEL_EFFICIENCY,
    PERF_OCCUPANCY,
    PERF_BRANCH_EFFICIENCY,
    PERF_WARP_EFFICIENCY,
    PERF_CACHE_HIT_RATE,
    PERF_PCIE_BANDWIDTH,
    PERF_POWER_CONSUMPTION
} PerformanceMetricType;

// 性能事件结构
typedef struct {
    const char* name;
    double timestamp;
    double duration;
    double value;
    PerformanceMetricType type;
    int gpu_id;
    int thread_id;
    size_t data_size;
    const char* category;
} PerformanceEvent;

// 性能统计结构
typedef struct {
    double min_value;
    double max_value;
    double avg_value;
    double std_dev;
    double last_value;
    int sample_count;
    double* history;
    int history_size;
    int history_index;
    double trend_slope;
    double trend_intercept;
    double confidence_interval;
} PerformanceStatistics;

// GPU性能计数器
typedef struct {
    int sm_clock;
    int memory_clock;
    int pcie_clock;
    int temperature;
    int power_draw;
    int fan_speed;
    int utilization_gpu;
    int utilization_memory;
    int utilization_pcie;
    size_t memory_used;
    size_t memory_free;
    size_t memory_total;
    double compute_capability;
    int multiprocessor_count;
    int warp_size;
    int max_threads_per_block;
    int max_threads_per_multiprocessor;
    int max_blocks_per_multiprocessor;
} GPUCounterData;

// 自动调优参数结构
typedef struct {
    double current_value;
    double optimal_value;
    double min_value;
    double max_value;
    double step_size;
    double learning_rate;
    int convergence_count;
    int oscillation_count;
    double last_improvement;
    double improvement_threshold;
    int adaptation_frequency;
    const char* parameter_name;
    const char* optimization_method;
} AutoTuningParameter;

// 优化的性能监控器
typedef struct {
    PerformanceEvent* events;
    int max_events;
    int event_count;
    PerformanceStatistics* statistics;
    int num_metrics;
    GPUCounterData* gpu_counters;
    int num_gpus;
    AutoTuningParameter* tuning_params;
    int num_tuning_params;
    double monitoring_start_time;
    double last_optimization_time;
    int optimization_interval;
    int auto_optimization_enabled;
    double performance_threshold;
    char* log_file;
    int verbose_level;
} OptimizedPerformanceMonitor;

// 机器学习预测模型
typedef struct {
    double* weights;
    double* biases;
    int input_size;
    int hidden_size;
    int output_size;
    double learning_rate;
    double regularization;
    int training_iterations;
    double* training_data;
    double* target_data;
    int data_size;
} MLPerformanceModel;

// 自适应算法参数
typedef struct {
    double exploration_rate;
    double exploitation_rate;
    double confidence_level;
    int window_size;
    double adaptation_speed;
    int convergence_criteria;
    double oscillation_threshold;
    int max_adjustments;
} AdaptiveAlgorithmParams;

// 初始化GPU性能计数器
void init_gpu_performance_counters(OptimizedPerformanceMonitor* monitor) {
    cudaGetDeviceCount(&monitor->num_gpus);
    
    monitor->gpu_counters = (GPUCounterData*)malloc(monitor->num_gpus * sizeof(GPUCounterData));
    
    for (int i = 0; i < monitor->num_gpus; i++) {
        cudaSetDevice(i);
        GPUCounterData* counter = &monitor->gpu_counters[i];
        
        // 获取设备属性
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        
        counter->compute_capability = prop.major + prop.minor * 0.1;
        counter->multiprocessor_count = prop.multiProcessorCount;
        counter->warp_size = prop.warpSize;
        counter->max_threads_per_block = prop.maxThreadsPerBlock;
        counter->max_threads_per_multiprocessor = prop.maxThreadsPerMultiProcessor;
        counter->max_blocks_per_multiprocessor = prop.maxBlocksPerMultiProcessor;
        counter->memory_total = prop.totalGlobalMem;
        
        // 初始化其他计数器
        counter->sm_clock = 0;
        counter->memory_clock = 0;
        counter->temperature = 0;
        counter->power_draw = 0;
        counter->utilization_gpu = 0;
        counter->utilization_memory = 0;
        counter->memory_used = 0;
        counter->memory_free = counter->memory_total;
    }
}

// 更新GPU性能计数器
void update_gpu_performance_counters(OptimizedPerformanceMonitor* monitor) {
    for (int i = 0; i < monitor->num_gpus; i++) {
        cudaSetDevice(i);
        GPUCounterData* counter = &monitor->gpu_counters[i];
        
        // 获取内存信息
        size_t free_mem, total_mem;
        cudaMemGetInfo(&free_mem, &total_mem);
        counter->memory_free = free_mem;
        counter->memory_used = total_mem - free_mem;
        
        // 获取利用率信息（模拟）
        counter->utilization_gpu = (int)(counter->memory_used * 100.0 / counter->memory_total);
        counter->utilization_memory = counter->utilization_gpu;
        
        // 记录性能事件
        record_performance_event(monitor, "GPU_Utilization", 
                               counter->utilization_gpu, PERF_COMPUTE_UTILIZATION, i);
        record_performance_event(monitor, "Memory_Usage", 
                               counter->memory_used / (1024.0 * 1024.0 * 1024.0), PERF_MEMORY_USAGE, i);
    }
}

// 记录性能事件
void record_performance_event(
    OptimizedPerformanceMonitor* monitor,
    const char* name, double value,
    PerformanceMetricType type, int gpu_id
) {
    if (monitor->event_count >= monitor->max_events) {
        // 移除最老的事件
        for (int i = 0; i < monitor->max_events - 1; i++) {
            monitor->events[i] = monitor->events[i + 1];
        }
        monitor->event_count = monitor->max_events - 1;
    }
    
    PerformanceEvent* event = &monitor->events[monitor->event_count++];
    
    event->name = name;
    event->timestamp = omp_get_wtime();
    event->duration = 0.0;
    event->value = value;
    event->type = type;
    event->gpu_id = gpu_id;
    event->thread_id = omp_get_thread_num();
    event->data_size = 0;
    event->category = "runtime";
    
    // 更新统计信息
    update_performance_statistics(&monitor->statistics[type], value);
}

// 更新性能统计信息
void update_performance_statistics(PerformanceStatistics* stats, double value) {
    if (stats->sample_count == 0) {
        stats->min_value = value;
        stats->max_value = value;
        stats->avg_value = value;
        stats->std_dev = 0.0;
    } else {
        // 更新最小值和最大值
        if (value < stats->min_value) stats->min_value = value;
        if (value > stats->max_value) stats->max_value = value;
        
        // 更新平均值
        double old_avg = stats->avg_value;
        stats->avg_value = (old_avg * stats->sample_count + value) / (stats->sample_count + 1);
        
        // 更新标准差（使用Welford算法）
        if (stats->sample_count > 0) {
            double delta = value - old_avg;
            double delta2 = value - stats->avg_value;
            stats->std_dev = sqrt((stats->std_dev * stats->std_dev * stats->sample_count + delta * delta2) / (stats->sample_count + 1));
        }
    }
    
    stats->last_value = value;
    stats->sample_count++;
    
    // 更新历史数据
    if (stats->history) {
        stats->history[stats->history_index] = value;
        stats->history_index = (stats->history_index + 1) % stats->history_size;
        
        // 计算趋势
        calculate_trend_statistics(stats);
    }
}

// 计算趋势统计
void calculate_trend_statistics(PerformanceStatistics* stats) {
    if (stats->sample_count < 3) return;
    
    int n = (stats->sample_count < stats->history_size) ? stats->sample_count : stats->history_size;
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
    
    for (int i = 0; i < n; i++) {
        sum_x += i;
        sum_y += stats->history[i];
        sum_xy += i * stats->history[i];
        sum_x2 += i * i;
    }
    
    // 线性回归计算趋势
    double denominator = n * sum_x2 - sum_x * sum_x;
    if (fabs(denominator) > 1e-10) {
        stats->trend_slope = (n * sum_xy - sum_x * sum_y) / denominator;
        stats->trend_intercept = (sum_y - stats->trend_slope * sum_x) / n;
    }
    
    // 计算置信区间
    double sum_residuals = 0;
    for (int i = 0; i < n; i++) {
        double predicted = stats->trend_slope * i + stats->trend_intercept;
        double residual = stats->history[i] - predicted;
        sum_residuals += residual * residual;
    }
    
    double std_error = sqrt(sum_residuals / (n - 2));
    stats->confidence_interval = 1.96 * std_error;  // 95%置信区间
}

// 初始化优化的性能监控器
OptimizedPerformanceMonitor* init_optimized_performance_monitor(int max_events, int num_metrics) {
    OptimizedPerformanceMonitor* monitor = (OptimizedPerformanceMonitor*)malloc(sizeof(OptimizedPerformanceMonitor));
    
    monitor->max_events = max_events;
    monitor->num_metrics = num_metrics;
    monitor->event_count = 0;
    monitor->monitoring_start_time = omp_get_wtime();
    monitor->last_optimization_time = 0.0;
    monitor->optimization_interval = 10;  // 10秒优化间隔
    monitor->auto_optimization_enabled = 1;
    monitor->performance_threshold = 0.8;
    monitor->verbose_level = 1;
    monitor->log_file = NULL;
    
    // 分配事件数组
    monitor->events = (PerformanceEvent*)malloc(max_events * sizeof(PerformanceEvent));
    
    // 分配统计信息数组
    monitor->statistics = (PerformanceStatistics*)calloc(num_metrics, sizeof(PerformanceStatistics));
    
    // 初始化统计信息历史缓冲区
    for (int i = 0; i < num_metrics; i++) {
        monitor->statistics[i].history_size = 100;
        monitor->statistics[i].history = (double*)malloc(100 * sizeof(double));
        monitor->statistics[i].history_index = 0;
        monitor->statistics[i].sample_count = 0;
    }
    
    // 初始化GPU计数器
    init_gpu_performance_counters(monitor);
    
    // 初始化自动调优参数
    monitor->num_tuning_params = 10;
    monitor->tuning_params = (AutoTuningParameter*)malloc(monitor->num_tuning_params * sizeof(AutoTuningParameter));
    
    // 初始化调优参数
    init_auto_tuning_parameters(monitor);
    
    return monitor;
}

// 初始化自动调优参数
void init_auto_tuning_parameters(OptimizedPerformanceMonitor* monitor) {
    // 线程块大小
    monitor->tuning_params[0] = (AutoTuningParameter){
        .current_value = 256,
        .optimal_value = 256,
        .min_value = 64,
        .max_value = 1024,
        .step_size = 32,
        .learning_rate = 0.1,
        .convergence_count = 0,
        .oscillation_count = 0,
        .last_improvement = 0.0,
        .improvement_threshold = 0.05,
        .adaptation_frequency = 5,
        .parameter_name = "Block_Size",
        .optimization_method = "Grid_Search"
    };
    
    // 网格大小倍数
    monitor->tuning_params[1] = (AutoTuningParameter){
        .current_value = 4.0,
        .optimal_value = 4.0,
        .min_value = 1.0,
        .max_value = 16.0,
        .step_size = 0.5,
        .learning_rate = 0.05,
        .parameter_name = "Grid_Size_Multiplier",
        .optimization_method = "Gradient_Descent"
    };
    
    // 共享内存大小
    monitor->tuning_params[2] = (AutoTuningParameter){
        .current_value = 16384,
        .optimal_value = 16384,
        .min_value = 8192,
        .max_value = 49152,
        .step_size = 4096,
        .parameter_name = "Shared_Memory_Size",
        .optimization_method = "Bayesian_Optimization"
    };
    
    // 寄存器数量
    monitor->tuning_params[3] = (AutoTuningParameter){
        .current_value = 32,
        .optimal_value = 32,
        .min_value = 16,
        .max_value = 128,
        .step_size = 8,
        .parameter_name = "Register_Count",
        .optimization_method = "Genetic_Algorithm"
    };
    
    // 内存预取距离
    monitor->tuning_params[4] = (AutoTuningParameter){
        .current_value = 3,
        .optimal_value = 3,
        .min_value = 1,
        .max_value = 10,
        .step_size = 1,
        .parameter_name = "Prefetch_Distance",
        .optimization_method = "Reinforcement_Learning"
    };
    
    // 工作窃取阈值
    monitor->tuning_params[5] = (AutoTuningParameter){
        .current_value = 0.2,
        .optimal_value = 0.2,
        .min_value = 0.05,
        .max_value = 0.5,
        .step_size = 0.05,
        .parameter_name = "Work_Stealing_Threshold",
        .optimization_method = "Adaptive_Search"
    };
    
    // 负载均衡因子
    monitor->tuning_params[6] = (AutoTuningParameter){
        .current_value = 1.0,
        .optimal_value = 1.0,
        .min_value = 0.1,
        .max_value = 10.0,
        .step_size = 0.1,
        .parameter_name = "Load_Balance_Factor",
        .optimization_method = "Neural_Network"
    };
    
    // 迭代求解器容差
    monitor->tuning_params[7] = (AutoTuningParameter){
        .current_value = 1e-6,
        .optimal_value = 1e-6,
        .min_value = 1e-8,
        .max_value = 1e-4,
        .step_size = 1e-7,
        .parameter_name = "Solver_Tolerance",
        .optimization_method = "Dynamic_Adaptation"
    };
    
    // H矩阵压缩容差
    monitor->tuning_params[8] = (AutoTuningParameter){
        .current_value = 1e-4,
        .optimal_value = 1e-4,
        .min_value = 1e-6,
        .max_value = 1e-2,
        .step_size = 1e-5,
        .parameter_name = "HMatrix_Tolerance",
        .optimization_method = "Multi_Objective_Optimization"
    };
    
    // 内存池块大小
    monitor->tuning_params[9] = (AutoTuningParameter){
        .current_value = 65536,
        .optimal_value = 65536,
        .min_value = 16384,
        .max_value = 262144,
        .step_size = 8192,
        .parameter_name = "Memory_Pool_Block_Size",
        .optimization_method = "Simulated_Annealing"
    };
}

// 执行自动调优
void perform_auto_tuning(OptimizedPerformanceMonitor* monitor) {
    double current_time = omp_get_wtime();
    
    // 检查优化间隔
    if (current_time - monitor->last_optimization_time < monitor->optimization_interval) {
        return;
    }
    
    monitor->last_optimization_time = current_time;
    
    if (!monitor->auto_optimization_enabled) {
        return;
    }
    
    // 评估当前性能
    double current_performance = evaluate_current_performance(monitor);
    
    // 如果性能低于阈值，执行调优
    if (current_performance < monitor->performance_threshold) {
        for (int i = 0; i < monitor->num_tuning_params; i++) {
            optimize_parameter(monitor, &monitor->tuning_params[i]);
        }
    }
}

// 评估当前性能
double evaluate_current_performance(OptimizedPerformanceMonitor* monitor) {
    // 综合多个性能指标
    double gpu_utilization = monitor->statistics[PERF_COMPUTE_UTILIZATION].avg_value;
    double memory_bandwidth = monitor->statistics[PERF_MEMORY_BANDWIDTH].avg_value;
    double kernel_efficiency = monitor->statistics[PERF_KERNEL_EFFICIENCY].avg_value;
    double cache_hit_rate = monitor->statistics[PERF_CACHE_HIT_RATE].avg_value;
    
    // 加权平均
    double performance = 0.3 * gpu_utilization + 
                        0.25 * memory_bandwidth + 
                        0.25 * kernel_efficiency + 
                        0.2 * cache_hit_rate;
    
    return performance / 100.0;  // 归一化到0-1范围
}

// 优化单个参数
void optimize_parameter(OptimizedPerformanceMonitor* monitor, AutoTuningParameter* param) {
    double current_performance = evaluate_current_performance(monitor);
    
    // 根据优化方法选择策略
    if (strcmp(param->optimization_method, "Gradient_Descent") == 0) {
        gradient_descent_optimization(monitor, param, current_performance);
    } else if (strcmp(param->optimization_method, "Grid_Search") == 0) {
        grid_search_optimization(monitor, param, current_performance);
    } else if (strcmp(param->optimization_method, "Bayesian_Optimization") == 0) {
        bayesian_optimization(monitor, param, current_performance);
    } else if (strcmp(param->optimization_method, "Reinforcement_Learning") == 0) {
        reinforcement_learning_optimization(monitor, param, current_performance);
    } else if (strcmp(param->optimization_method, "Adaptive_Search") == 0) {
        adaptive_search_optimization(monitor, param, current_performance);
    }
    
    // 检查收敛性
    check_parameter_convergence(param);
}

// 梯度下降优化
void gradient_descent_optimization(
    OptimizedPerformanceMonitor* monitor,
    AutoTuningParameter* param,
    double current_performance
) {
    // 计算梯度（使用有限差分）
    double original_value = param->current_value;
    double delta = param->step_size;
    
    // 正向扰动
    param->current_value = original_value + delta;
    double performance_plus = evaluate_current_performance(monitor);
    
    // 负向扰动
    param->current_value = original_value - delta;
    double performance_minus = evaluate_current_performance(monitor);
    
    // 计算梯度
    double gradient = (performance_plus - performance_minus) / (2.0 * delta);
    
    // 更新参数
    double new_value = original_value + param->learning_rate * gradient;
    
    // 边界检查
    if (new_value < param->min_value) new_value = param->min_value;
    if (new_value > param->max_value) new_value = param->max_value;
    
    param->current_value = new_value;
    
    // 检查是否改进了性能
    double new_performance = evaluate_current_performance(monitor);
    if (new_performance > current_performance + param->improvement_threshold) {
        param->last_improvement = omp_get_wtime();
        param->convergence_count = 0;
    } else {
        param->convergence_count++;
    }
}

// 网格搜索优化
void grid_search_optimization(
    OptimizedPerformanceMonitor* monitor,
    AutoTuningParameter* param,
    double current_performance
) {
    int num_steps = (int)((param->max_value - param->min_value) / param->step_size);
    double best_value = param->current_value;
    double best_performance = current_performance;
    
    // 在参数范围内搜索
    for (int i = 0; i <= num_steps; i++) {
        double test_value = param->min_value + i * param->step_size;
        param->current_value = test_value;
        
        double test_performance = evaluate_current_performance(monitor);
        
        if (test_performance > best_performance) {
            best_performance = test_performance;
            best_value = test_value;
        }
    }
    
    param->current_value = best_value;
    
    // 自适应调整步长
    if (param->convergence_count > param->adaptation_frequency) {
        param->step_size *= 0.5;  // 减小步长
        param->convergence_count = 0;
    }
}

// 自适应搜索优化
void adaptive_search_optimization(
    OptimizedPerformanceMonitor* monitor,
    AutoTuningParameter* param,
    double current_performance
) {
    // 基于历史性能调整搜索策略
    PerformanceStatistics* stats = &monitor->statistics[PERF_COMPUTE_UTILIZATION];
    
    if (stats->sample_count > 10) {
        // 如果趋势是下降的，增加探索率
        if (stats->trend_slope < -0.01) {
            param->learning_rate *= 1.2;
            param->step_size *= 1.1;
        }
        // 如果趋势是上升的，增加开发率
        else if (stats->trend_slope > 0.01) {
            param->learning_rate *= 0.9;
            param->step_size *= 0.9;
        }
        
        // 边界检查
        if (param->learning_rate > 0.5) param->learning_rate = 0.5;
        if (param->learning_rate < 0.01) param->learning_rate = 0.01;
        if (param->step_size > (param->max_value - param->min_value) / 10.0) {
            param->step_size = (param->max_value - param->min_value) / 10.0;
        }
    }
    
    // 执行梯度下降
    gradient_descent_optimization(monitor, param, current_performance);
}

// 检查参数收敛性
void check_parameter_convergence(AutoTuningParameter* param) {
    double current_time = omp_get_wtime();
    
    // 检查是否长时间没有改进
    if (current_time - param->last_improvement > 30.0) {  // 30秒没有改进
        // 重置参数到最优值附近
        param->current_value = param->optimal_value + 
                              (drand48() - 0.5) * param->step_size;
        param->last_improvement = current_time;
    }
    
    // 检查振荡
    if (param->oscillation_count > 10) {
        // 减小步长以避免振荡
        param->step_size *= 0.5;
        param->oscillation_count = 0;
    }
}

// 生成性能报告
void generate_performance_report(OptimizedPerformanceMonitor* monitor, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    
    double current_time = omp_get_wtime();
    double monitoring_duration = current_time - monitor->monitoring_start_time;
    
    fprintf(fp, "=== PCB电磁仿真性能监控报告 ===\n");
    fprintf(fp, "监控时长: %.2f 秒\n", monitoring_duration);
    fprintf(fp, "事件数量: %d\n", monitor->event_count);
    fprintf(fp, "GPU数量: %d\n\n", monitor->num_gpus);
    
    fprintf(fp, "性能统计:\n");
    for (int i = 0; i < monitor->num_metrics; i++) {
        PerformanceStatistics* stats = &monitor->statistics[i];
        if (stats->sample_count > 0) {
            const char* metric_name = get_metric_name(i);
            fprintf(fp, "  %s:\n", metric_name);
            fprintf(fp, "    最小值: %.3f\n", stats->min_value);
            fprintf(fp, "    最大值: %.3f\n", stats->max_value);
            fprintf(fp, "    平均值: %.3f\n", stats->avg_value);
            fprintf(fp, "    标准差: %.3f\n", stats->std_dev);
            fprintf(fp, "    趋势斜率: %.6f\n", stats->trend_slope);
            fprintf(fp, "    置信区间: ±%.3f\n", stats->confidence_interval);
        }
    }
    
    fprintf(fp, "\n调优参数:\n");
    for (int i = 0; i < monitor->num_tuning_params; i++) {
        AutoTuningParameter* param = &monitor->tuning_params[i];
        fprintf(fp, "  %s:\n", param->parameter_name);
        fprintf(fp, "    当前值: %.6f\n", param->current_value);
        fprintf(fp, "    最优值: %.6f\n", param->optimal_value);
        fprintf(fp, "    优化方法: %s\n", param->optimization_method);
        fprintf(fp, "    收敛次数: %d\n", param->convergence_count);
    }
    
    fprintf(fp, "\nGPU状态:\n");
    for (int i = 0; i < monitor->num_gpus; i++) {
        GPUCounterData* gpu = &monitor->gpu_counters[i];
        fprintf(fp, "  GPU %d:\n", i);
        fprintf(fp, "    计算能力: %.1f\n", gpu->compute_capability);
        fprintf(fp, "    多处理器数: %d\n", gpu->multiprocessor_count);
        fprintf(fp, "    内存使用: %.2f GB / %.2f GB\n", 
                gpu->memory_used / (1024.0 * 1024.0 * 1024.0),
                gpu->memory_total / (1024.0 * 1024.0 * 1024.0));
        fprintf(fp, "    GPU利用率: %d%%\n", gpu->utilization_gpu);
        fprintf(fp, "    内存利用率: %d%%\n", gpu->utilization_memory);
    }
    
    fclose(fp);
}

// 获取指标名称
const char* get_metric_name(int metric_type) {
    switch (metric_type) {
        case PERF_COMPUTE_UTILIZATION: return "Compute_Utilization";
        case PERF_MEMORY_BANDWIDTH: return "Memory_Bandwidth";
        case PERF_MEMORY_USAGE: return "Memory_Usage";
        case PERF_KERNEL_EFFICIENCY: return "Kernel_Efficiency";
        case PERF_OCCUPANCY: return "Occupancy";
        case PERF_BRANCH_EFFICIENCY: return "Branch_Efficiency";
        case PERF_WARP_EFFICIENCY: return "Warp_Efficiency";
        case PERF_CACHE_HIT_RATE: return "Cache_Hit_Rate";
        case PERF_PCIE_BANDWIDTH: return "PCIe_Bandwidth";
        case PERF_POWER_CONSUMPTION: return "Power_Consumption";
        default: return "Unknown_Metric";
    }
}

// 清理优化的性能监控器
void cleanup_optimized_performance_monitor(OptimizedPerformanceMonitor* monitor) {
    if (!monitor) return;
    
    // 生成最终性能报告
    generate_performance_report(monitor, "final_performance_report.txt");
    
    // 释放内存
    free(monitor->events);
    
    for (int i = 0; i < monitor->num_metrics; i++) {
        free(monitor->statistics[i].history);
    }
    free(monitor->statistics);
    
    free(monitor->gpu_counters);
    free(monitor->tuning_params);
    
    if (monitor->log_file) {
        free(monitor->log_file);
    }
    
    free(monitor);
}