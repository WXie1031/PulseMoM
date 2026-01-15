#ifndef MULTI_GPU_WORK_DISTRIBUTION_H
#define MULTI_GPU_WORK_DISTRIBUTION_H

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <pthread.h>
#include "gpu_acceleration.h"
#include "gpu_linalg_optimization.h"

// Advanced work distribution strategies
typedef enum {
    WORK_DIST_STATIC,
    WORK_DIST_DYNAMIC,
    WORK_DIST_GUIDED,
    WORK_DIST_ADAPTIVE,
    WORK_DIST_PERFORMANCE_BASED
} WorkDistributionStrategy;

// Work unit for electromagnetic simulation tasks
typedef struct {
    int task_id;
    int start_index;
    int end_index;
    size_t data_size;
    void *data_ptr;
    double estimated_compute_time;
    double priority;
    int assigned_gpu;
    bool completed;
    double actual_compute_time;
} WorkUnit;

// GPU performance metrics for load balancing
typedef struct {
    double compute_throughput;     // GFLOPS
    double memory_bandwidth;       // GB/s
    double current_utilization;    // Percentage
    double thermal_throttling;     // Percentage
    double power_consumption;      // Watts
    double memory_usage;          // GB
    double queue_length;          // Number of pending tasks
    double average_task_time;     // Seconds
    double performance_degradation; // Percentage from peak
} GPUPerformanceMetrics;

// Advanced multi-GPU scheduler
typedef struct {
    int n_gpus;
    GPUContext **gpu_contexts;
    GPUPerformanceMetrics *performance_metrics;
    WorkDistributionStrategy strategy;
    
    // Work queue management
    WorkUnit **work_queues;
    int *queue_sizes;
    int *queue_capacities;
    pthread_mutex_t *queue_mutexes;
    
    // Dynamic load balancing
    double *performance_weights;
    double *memory_weights;
    double *thermal_weights;
    double rebalance_threshold;
    
    // Work stealing
    bool enable_work_stealing;
    pthread_t *stealing_threads;
    volatile bool stealing_active;
    
    // Performance monitoring
    double *task_completion_times;
    int *task_counts;
    double total_throughput;
    double efficiency_score;
    
    // Fault tolerance
    bool *gpu_healthy;
    int *failure_counts;
    double *last_heartbeat;
    
    // Advanced scheduling
    bool enable_prefetching;
    bool enable_overlapping;
    int prefetch_distance;
    
} AdvancedMultiGPUScheduler;

// Initialize advanced multi-GPU scheduler
AdvancedMultiGPUScheduler* initialize_advanced_scheduler(
    int n_gpus,
    WorkDistributionStrategy strategy
);

// Cleanup advanced multi-GPU scheduler
void cleanup_advanced_scheduler(AdvancedMultiGPUScheduler *scheduler);

// Update GPU performance metrics
void update_gpu_performance_metrics(
    AdvancedMultiGPUScheduler *scheduler,
    int gpu_id
);

// Dynamic work distribution with load balancing
void distribute_work_advanced(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_units,
    int n_work_units
);

// Adaptive work redistribution
void rebalance_work_distribution(
    AdvancedMultiGPUScheduler *scheduler
);

// Work stealing implementation
void* work_stealing_thread(void *arg);
void enable_work_stealing(AdvancedMultiGPUScheduler *scheduler);
void disable_work_stealing(AdvancedMultiGPUScheduler *scheduler);

// Advanced scheduling algorithms
void schedule_work_static(AdvancedMultiGPUScheduler *scheduler, WorkUnit *work_units, int n_units);
void schedule_work_dynamic(AdvancedMultiGPUScheduler *scheduler, WorkUnit *work_units, int n_units);
void schedule_work_guided(AdvancedMultiGPUScheduler *scheduler, WorkUnit *work_units, int n_units);
void schedule_work_adaptive(AdvancedMultiGPUScheduler *scheduler, WorkUnit *work_units, int n_units);
void schedule_work_performance_based(AdvancedMultiGPUScheduler *scheduler, WorkUnit *work_units, int n_units);

// Performance-based task assignment
int select_optimal_gpu(
    AdvancedMultiGPUScheduler *scheduler,
    const WorkUnit *work_unit
);

// Predict task execution time
double predict_execution_time(
    const GPUPerformanceMetrics *metrics,
    const WorkUnit *work_unit
);

// Memory-aware scheduling
bool check_memory_availability(
    AdvancedMultiGPUScheduler *scheduler,
    int gpu_id,
    size_t required_memory
);

// Thermal-aware scheduling
bool check_thermal_constraints(
    AdvancedMultiGPUScheduler *scheduler,
    int gpu_id
);

// Fault-tolerant scheduling
bool check_gpu_health(
    AdvancedMultiGPUScheduler *scheduler,
    int gpu_id
);
void mark_gpu_unhealthy(
    AdvancedMultiGPUScheduler *scheduler,
    int gpu_id,
    const char *reason
);

// Advanced synchronization and coordination
void synchronize_all_gpus(AdvancedMultiGPUScheduler *scheduler);
void barrier_synchronization(AdvancedMultiGPUScheduler *scheduler);

// Performance monitoring and optimization
typedef struct {
    double total_execution_time;
    double load_imbalance_ratio;
    double average_gpu_utilization;
    double energy_efficiency;
    double throughput_gflops;
    double memory_efficiency;
    int work_stealing_events;
    int load_rebalance_events;
    int gpu_failures;
} SchedulingPerformanceMetrics;

SchedulingPerformanceMetrics get_scheduling_performance(
    AdvancedMultiGPUScheduler *scheduler
);

// Optimize scheduling parameters
void optimize_scheduling_parameters(
    AdvancedMultiGPUScheduler *scheduler
);

// Advanced memory management with NUMA awareness
void* allocate_numa_aware_memory(
    AdvancedMultiGPUScheduler *scheduler,
    size_t size,
    int preferred_gpu
);

void prefetch_data_to_gpu(
    AdvancedMultiGPUScheduler *scheduler,
    void *data,
    size_t size,
    int target_gpu
);

// Heterogeneous computing support (CPU + GPU)
typedef struct {
    bool enable_cpu_fallback;
    int cpu_thread_count;
    double cpu_performance_ratio;
    pthread_t *cpu_threads;
    WorkUnit **cpu_work_queues;
} HeterogeneousComputingSupport;

HeterogeneousComputingSupport* initialize_heterogeneous_support(
    AdvancedMultiGPUScheduler *scheduler,
    int cpu_threads
);

void cleanup_heterogeneous_support(
    HeterogeneousComputingSupport *hetero_support
);

// Advanced work distribution for electromagnetic simulations
void distribute_sommerfeld_computation(
    AdvancedMultiGPUScheduler *scheduler,
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

void distribute_greens_function_computation(
    AdvancedMultiGPUScheduler *scheduler,
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

void distribute_matrix_assembly(
    AdvancedMultiGPUScheduler *scheduler,
    const gpu_complex *basis_matrix,
    const gpu_complex *greens_matrix,
    const int n_basis,
    const int n_test,
    gpu_complex *impedance_matrix
);

// Advanced profiling and debugging
typedef struct {
    double timestamp;
    int gpu_id;
    int task_id;
    const char *operation;
    double duration;
    double memory_usage;
    double compute_utilization;
} TaskExecutionProfile;

void profile_task_execution(
    AdvancedMultiGPUScheduler *scheduler,
    TaskExecutionProfile *profile
);

void generate_scheduling_report(
    AdvancedMultiGPUScheduler *scheduler,
    const char *filename
);

// Real-time scheduling adaptation
void enable_real_time_adaptation(
    AdvancedMultiGPUScheduler *scheduler,
    double adaptation_interval
);

void disable_real_time_adaptation(
    AdvancedMultiGPUScheduler *scheduler
);

// Energy-aware scheduling
void enable_energy_aware_scheduling(
    AdvancedMultiGPUScheduler *scheduler,
    double power_budget
);

void disable_energy_aware_scheduling(
    AdvancedMultiGPUScheduler *scheduler
);

// Quality of Service (QoS) support
typedef struct {
    double deadline;
    double priority;
    double resource_requirement;
    bool hard_constraint;
} QoSRequirements;

void set_task_qos_requirements(
    WorkUnit *work_unit,
    const QoSRequirements *qos
);

bool qos_aware_scheduling(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_unit
);

#endif // ENABLE_CUDA

#endif // MULTI_GPU_WORK_DISTRIBUTION_H