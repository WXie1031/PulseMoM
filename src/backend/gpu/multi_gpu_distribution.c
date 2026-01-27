#include "multi_gpu_work_distribution.h"

#ifdef ENABLE_CUDA

// Initialize advanced multi-GPU scheduler with sophisticated load balancing
AdvancedMultiGPUScheduler* initialize_advanced_scheduler(
    int n_gpus,
    WorkDistributionStrategy strategy) {
    
    AdvancedMultiGPUScheduler *scheduler = (AdvancedMultiGPUScheduler*)malloc(
        sizeof(AdvancedMultiGPUScheduler));
    
    scheduler->n_gpus = n_gpus;
    scheduler->strategy = strategy;
    scheduler->enable_work_stealing = false;
    scheduler->enable_prefetching = true;
    scheduler->enable_overlapping = true;
    scheduler->prefetch_distance = 2;
    scheduler->rebalance_threshold = 0.1; // 10% imbalance threshold
    
    // Initialize GPU contexts
    scheduler->gpu_contexts = (GPUContext**)malloc(n_gpus * sizeof(GPUContext*));
    scheduler->performance_metrics = (GPUPerformanceMetrics*)malloc(
        n_gpus * sizeof(GPUPerformanceMetrics));
    
    for (int i = 0; i < n_gpus; i++) {
        scheduler->gpu_contexts[i] = initialize_gpu_context(i);
        
        // Initialize performance metrics
        scheduler->performance_metrics[i].compute_throughput = 0.0;
        scheduler->performance_metrics[i].memory_bandwidth = 0.0;
        scheduler->performance_metrics[i].current_utilization = 0.0;
        scheduler->performance_metrics[i].thermal_throttling = 0.0;
        scheduler->performance_metrics[i].power_consumption = 0.0;
        scheduler->performance_metrics[i].memory_usage = 0.0;
        scheduler->performance_metrics[i].queue_length = 0.0;
        scheduler->performance_metrics[i].average_task_time = 0.0;
        scheduler->performance_metrics[i].performance_degradation = 0.0;
    }
    
    // Initialize work queue management
    scheduler->work_queues = (WorkUnit**)malloc(n_gpus * sizeof(WorkUnit*));
    scheduler->queue_sizes = (int*)calloc(n_gpus, sizeof(int));
    scheduler->queue_capacities = (int*)malloc(n_gpus * sizeof(int));
    scheduler->queue_mutexes = (pthread_mutex_t*)malloc(n_gpus * sizeof(pthread_mutex_t));
    
    for (int i = 0; i < n_gpus; i++) {
        scheduler->queue_capacities[i] = 1000; // Initial capacity
        scheduler->work_queues[i] = (WorkUnit*)malloc(
            scheduler->queue_capacities[i] * sizeof(WorkUnit));
        pthread_mutex_init(&scheduler->queue_mutexes[i], NULL);
    }
    
    // Initialize performance weights for adaptive scheduling
    scheduler->performance_weights = (double*)malloc(n_gpus * sizeof(double));
    scheduler->memory_weights = (double*)malloc(n_gpus * sizeof(double));
    scheduler->thermal_weights = (double*)malloc(n_gpus * sizeof(double));
    
    for (int i = 0; i < n_gpus; i++) {
        scheduler->performance_weights[i] = 1.0;
        scheduler->memory_weights[i] = 1.0;
        scheduler->thermal_weights[i] = 1.0;
    }
    
    // Initialize performance monitoring
    scheduler->task_completion_times = (double*)calloc(n_gpus, sizeof(double));
    scheduler->task_counts = (int*)calloc(n_gpus, sizeof(int));
    scheduler->total_throughput = 0.0;
    scheduler->efficiency_score = 1.0;
    
    // Initialize fault tolerance
    scheduler->gpu_healthy = (bool*)malloc(n_gpus * sizeof(bool));
    scheduler->failure_counts = (int*)calloc(n_gpus, sizeof(int));
    scheduler->last_heartbeat = (double*)malloc(n_gpus * sizeof(double));
    
    for (int i = 0; i < n_gpus; i++) {
        scheduler->gpu_healthy[i] = true;
        scheduler->last_heartbeat[i] = 0.0;
    }
    
    // Initialize work stealing
    scheduler->stealing_threads = NULL;
    scheduler->stealing_active = false;
    
    printf("Advanced Multi-GPU Scheduler Initialized:\n");
    printf("  GPUs: %d\n", n_gpus);
    printf("  Strategy: %d\n", strategy);
    printf("  Work Stealing: %s\n", scheduler->enable_work_stealing ? "Enabled" : "Disabled");
    printf("  Prefetching: %s\n", scheduler->enable_prefetching ? "Enabled" : "Disabled");
    printf("  Overlapping: %s\n", scheduler->enable_overlapping ? "Enabled" : "Disabled");
    
    return scheduler;
}

// Cleanup advanced multi-GPU scheduler
void cleanup_advanced_scheduler(AdvancedMultiGPUScheduler *scheduler) {
    if (!scheduler) return;
    
    // Disable work stealing
    if (scheduler->stealing_active) {
        disable_work_stealing(scheduler);
    }
    
    // Cleanup GPU contexts
    for (int i = 0; i < scheduler->n_gpus; i++) {
        cleanup_gpu_context(scheduler->gpu_contexts[i]);
        pthread_mutex_destroy(&scheduler->queue_mutexes[i]);
        free(scheduler->work_queues[i]);
    }
    
    // Free allocated memory
    free(scheduler->gpu_contexts);
    free(scheduler->performance_metrics);
    free(scheduler->work_queues);
    free(scheduler->queue_sizes);
    free(scheduler->queue_capacities);
    free(scheduler->queue_mutexes);
    free(scheduler->performance_weights);
    free(scheduler->memory_weights);
    free(scheduler->thermal_weights);
    free(scheduler->task_completion_times);
    free(scheduler->task_counts);
    free(scheduler->gpu_healthy);
    free(scheduler->failure_counts);
    free(scheduler->last_heartbeat);
    free(scheduler->stealing_threads);
    
    free(scheduler);
}

// Update GPU performance metrics with real-time monitoring
void update_gpu_performance_metrics(
    AdvancedMultiGPUScheduler *scheduler,
    int gpu_id) {
    
    if (!scheduler || gpu_id < 0 || gpu_id >= scheduler->n_gpus) return;
    
    GPUContext *gpu_context = scheduler->gpu_contexts[gpu_id];
    GPUPerformanceMetrics *metrics = &scheduler->performance_metrics[gpu_id];
    
    // Get device properties for theoretical performance
    cudaDeviceProp device_props;
    CUDA_CHECK(cudaGetDeviceProperties(&device_props, gpu_id));
    
    // Calculate theoretical compute throughput (GFLOPS)
    double base_clock_ghz = device_props.clockRate / 1e6;
    int cuda_cores = device_props.multiProcessorCount * 
                     device_props.maxThreadsPerMultiProcessor;
    metrics->compute_throughput = base_clock_ghz * cuda_cores * 2.0; // 2 FLOPS per core per cycle
    
    // Calculate theoretical memory bandwidth (GB/s)
    double memory_clock_ghz = device_props.memoryClockRate / 1e6;
    int memory_bus_width = device_props.memoryBusWidth;
    metrics->memory_bandwidth = memory_clock_ghz * memory_bus_width * 2.0 / 8.0 / 1e3;
    
    // Get current utilization (if available)
    nvmlDevice_t nvml_device;
    nvmlDeviceGetHandleByIndex(gpu_id, &nvml_device);
    
    nvmlUtilization_t utilization;
    if (nvmlDeviceGetUtilizationRates(nvml_device, &utilization) == NVML_SUCCESS) {
        metrics->current_utilization = utilization.gpu / 100.0;
    }
    
    // Get thermal information
    unsigned int temperature;
    if (nvmlDeviceGetTemperature(nvml_device, NVML_TEMPERATURE_GPU, &temperature) == NVML_SUCCESS) {
        double thermal_limit = device_props.thermalThreshold;
        metrics->thermal_throttling = (temperature > thermal_limit * 0.9) ? 
                                      (temperature - thermal_limit * 0.9) / (thermal_limit * 0.1) : 0.0;
    }
    
    // Get power consumption
    unsigned int power_mw;
    if (nvmlDeviceGetPowerUsage(nvml_device, &power_mw) == NVML_SUCCESS) {
        metrics->power_consumption = power_mw / 1000.0; // Convert to Watts
    }
    
    // Calculate memory usage
    size_t free_mem, total_mem;
    CUDA_CHECK(cudaMemGetInfo(&free_mem, &total_mem));
    metrics->memory_usage = (total_mem - free_mem) / (double)total_mem;
    
    // Calculate queue length
    metrics->queue_length = scheduler->queue_sizes[gpu_id];
    
    // Update average task time
    if (scheduler->task_counts[gpu_id] > 0) {
        metrics->average_task_time = 
            scheduler->task_completion_times[gpu_id] / scheduler->task_counts[gpu_id];
    }
    
    // Calculate performance degradation
    double theoretical_throughput = metrics->compute_throughput;
    double actual_throughput = scheduler->task_counts[gpu_id] / 
                              (scheduler->task_completion_times[gpu_id] + 1e-6);
    metrics->performance_degradation = 
        (theoretical_throughput - actual_throughput) / theoretical_throughput;
    
    // Update heartbeat
    scheduler->last_heartbeat[gpu_id] = get_current_time();
}

// Dynamic work distribution with advanced load balancing
void distribute_work_advanced(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_units,
    int n_work_units) {
    
    if (!scheduler || !work_units || n_work_units <= 0) return;
    
    // Update performance metrics for all GPUs
    for (int i = 0; i < scheduler->n_gpus; i++) {
        update_gpu_performance_metrics(scheduler, i);
    }
    
    // Apply the selected scheduling strategy
    switch (scheduler->strategy) {
        case WORK_DIST_STATIC:
            schedule_work_static(scheduler, work_units, n_work_units);
            break;
        case WORK_DIST_DYNAMIC:
            schedule_work_dynamic(scheduler, work_units, n_work_units);
            break;
        case WORK_DIST_GUIDED:
            schedule_work_guided(scheduler, work_units, n_work_units);
            break;
        case WORK_DIST_ADAPTIVE:
            schedule_work_adaptive(scheduler, work_units, n_work_units);
            break;
        case WORK_DIST_PERFORMANCE_BASED:
            schedule_work_performance_based(scheduler, work_units, n_work_units);
            break;
        default:
            schedule_work_static(scheduler, work_units, n_work_units);
            break;
    }
    
    // Enable work stealing if configured
    if (scheduler->enable_work_stealing && !scheduler->stealing_active) {
        enable_work_stealing(scheduler);
    }
}

// Static work distribution (simple round-robin)
void schedule_work_static(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_units,
    int n_units) {
    
    int tasks_per_gpu = n_units / scheduler->n_gpus;
    int remainder = n_units % scheduler->n_gpus;
    
    int current_task = 0;
    for (int gpu_id = 0; gpu_id < scheduler->n_gpus; gpu_id++) {
        int gpu_tasks = tasks_per_gpu + (gpu_id < remainder ? 1 : 0);
        
        pthread_mutex_lock(&scheduler->queue_mutexes[gpu_id]);
        
        for (int i = 0; i < gpu_tasks; i++) {
            if (current_task < n_units) {
                work_units[current_task].assigned_gpu = gpu_id;
                scheduler->work_queues[gpu_id][scheduler->queue_sizes[gpu_id]++] = 
                    work_units[current_task];
                current_task++;
            }
        }
        
        pthread_mutex_unlock(&scheduler->queue_mutexes[gpu_id]);
    }
    
    printf("Static scheduling: %d tasks distributed across %d GPUs\n", 
           n_units, scheduler->n_gpus);
}

// Dynamic work distribution with task stealing
void schedule_work_dynamic(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_units,
    int n_units) {
    
    // Sort work units by estimated compute time (descending)
    qsort(work_units, n_units, sizeof(WorkUnit), compare_work_unit_compute_time);
    
    // Assign largest tasks to fastest GPUs first
    for (int i = 0; i < n_units; i++) {
        int optimal_gpu = select_optimal_gpu(scheduler, &work_units[i]);
        
        pthread_mutex_lock(&scheduler->queue_mutexes[optimal_gpu]);
        
        work_units[i].assigned_gpu = optimal_gpu;
        scheduler->work_queues[optimal_gpu][scheduler->queue_sizes[optimal_gpu]++] = 
            work_units[i];
        
        pthread_mutex_unlock(&scheduler->queue_mutexes[optimal_gpu]);
    }
}

// Guided work distribution with decreasing chunk sizes
void schedule_work_guided(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_units,
    int n_units) {
    
    int remaining_tasks = n_units;
    int min_chunk_size = 1;
    
    while (remaining_tasks > 0) {
        // Calculate chunk size (decreases as work progresses)
        int chunk_size = remaining_tasks / (scheduler->n_gpus * 2);
        chunk_size = (chunk_size < min_chunk_size) ? min_chunk_size : chunk_size;
        chunk_size = (chunk_size > remaining_tasks) ? remaining_tasks : chunk_size;
        
        // Find GPU with minimum load
        int min_load_gpu = 0;
        int min_load = scheduler->queue_sizes[0];
        
        for (int i = 1; i < scheduler->n_gpus; i++) {
            if (scheduler->queue_sizes[i] < min_load) {
                min_load = scheduler->queue_sizes[i];
                min_load_gpu = i;
            }
        }
        
        // Assign chunk to GPU with minimum load
        int start_idx = n_units - remaining_tasks;
        
        pthread_mutex_lock(&scheduler->queue_mutexes[min_load_gpu]);
        
        for (int j = 0; j < chunk_size; j++) {
            work_units[start_idx + j].assigned_gpu = min_load_gpu;
            scheduler->work_queues[min_load_gpu][scheduler->queue_sizes[min_load_gpu]++] = 
                work_units[start_idx + j];
        }
        
        pthread_mutex_unlock(&scheduler->queue_mutexes[min_load_gpu]);
        
        remaining_tasks -= chunk_size;
    }
}

// Adaptive work distribution with real-time load balancing
void schedule_work_adaptive(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_units,
    int n_units) {
    
    // Initial distribution based on current performance metrics
    for (int i = 0; i < n_units; i++) {
        int optimal_gpu = select_optimal_gpu(scheduler, &work_units[i]);
        
        pthread_mutex_lock(&scheduler->queue_mutexes[optimal_gpu]);
        
        work_units[i].assigned_gpu = optimal_gpu;
        scheduler->work_queues[optimal_gpu][scheduler->queue_sizes[optimal_gpu]++] = 
            work_units[i];
        
        pthread_mutex_unlock(&scheduler->queue_mutexes[optimal_gpu]);
    }
    
    // Trigger rebalancing after initial distribution
    rebalance_work_distribution(scheduler);
}

// Performance-based work distribution using real-time metrics
void schedule_work_performance_based(
    AdvancedMultiGPUScheduler *scheduler,
    WorkUnit *work_units,
    int n_units) {
    
    // Calculate performance scores for each GPU
    double *performance_scores = (double*)malloc(scheduler->n_gpus * sizeof(double));
    double total_score = 0.0;
    
    for (int i = 0; i < scheduler->n_gpus; i++) {
        GPUPerformanceMetrics *metrics = &scheduler->performance_metrics[i];
        
        // Weighted performance score
        performance_scores[i] = 
            metrics->compute_throughput * 0.4 +
            metrics->memory_bandwidth * 0.3 +
            (1.0 - metrics->current_utilization) * 0.2 +
            (1.0 - metrics->thermal_throttling) * 0.1;
        
        total_score += performance_scores[i];
    }
    
    // Distribute work proportionally to performance scores
    int current_task = 0;
    for (int gpu_id = 0; gpu_id < scheduler->n_gpus; gpu_id++) {
        double gpu_share = performance_scores[gpu_id] / total_score;
        int gpu_tasks = (int)(n_units * gpu_share);
        
        // Ensure minimum tasks per GPU
        if (gpu_tasks == 0 && n_units > scheduler->n_gpus) {
            gpu_tasks = 1;
        }
        
        pthread_mutex_lock(&scheduler->queue_mutexes[gpu_id]);
        
        for (int i = 0; i < gpu_tasks && current_task < n_units; i++) {
            work_units[current_task].assigned_gpu = gpu_id;
            scheduler->work_queues[gpu_id][scheduler->queue_sizes[gpu_id]++] = 
                work_units[current_task];
            current_task++;
        }
        
        pthread_mutex_unlock(&scheduler->queue_mutexes[gpu_id]);
    }
    
    // Distribute remaining tasks
    while (current_task < n_units) {
        int optimal_gpu = select_optimal_gpu(scheduler, &work_units[current_task]);
        
        pthread_mutex_lock(&scheduler->queue_mutexes[optimal_gpu]);
        
        work_units[current_task].assigned_gpu = optimal_gpu;
        scheduler->work_queues[optimal_gpu][scheduler->queue_sizes[optimal_gpu]++] = 
            work_units[current_task];
        
        pthread_mutex_unlock(&scheduler->queue_mutexes[optimal_gpu]);
        current_task++;
    }
    
    free(performance_scores);
}

// Select optimal GPU for a given work unit
int select_optimal_gpu(
    AdvancedMultiGPUScheduler *scheduler,
    const WorkUnit *work_unit) {
    
    int best_gpu = 0;
    double best_score = -1e10;
    
    for (int i = 0; i < scheduler->n_gpus; i++) {
        if (!scheduler->gpu_healthy[i]) continue;
        
        GPUPerformanceMetrics *metrics = &scheduler->performance_metrics[i];
        
        // Calculate suitability score
        double score = 0.0;
        
        // Performance score
        score += metrics->compute_throughput * scheduler->performance_weights[i];
        
        // Memory availability
        if (work_unit->data_size > 0) {
            double memory_score = 1.0 - metrics->memory_usage;
            score += memory_score * scheduler->memory_weights[i];
        }
        
        // Thermal considerations
        score += (1.0 - metrics->thermal_throttling) * scheduler->thermal_weights[i];
        
        // Queue length penalty
        score -= metrics->queue_length * 0.1;
        
        // Estimated execution time
        double predicted_time = predict_execution_time(metrics, work_unit);
        score -= predicted_time * 0.01;
        
        if (score > best_score) {
            best_score = score;
            best_gpu = i;
        }
    }
    
    return best_gpu;
}

// Predict task execution time based on GPU performance and task characteristics
double predict_execution_time(
    const GPUPerformanceMetrics *metrics,
    const WorkUnit *work_unit) {
    
    // Simple prediction model based on compute intensity and data size
    double compute_time = work_unit->estimated_compute_time / metrics->compute_throughput;
    double memory_time = work_unit->data_size / (metrics->memory_bandwidth * 1e9);
    
    // Add overhead for queue waiting time
    double queue_wait_time = metrics->queue_length * metrics->average_task_time;
    
    return compute_time + memory_time + queue_wait_time;
}

// Enable work stealing for load balancing
void enable_work_stealing(AdvancedMultiGPUScheduler *scheduler) {
    if (!scheduler || scheduler->stealing_active) return;
    
    scheduler->stealing_active = true;
    scheduler->stealing_threads = (pthread_t*)malloc(scheduler->n_gpus * sizeof(pthread_t));
    
    for (int i = 0; i < scheduler->n_gpus; i++) {
        pthread_create(&scheduler->stealing_threads[i], NULL, 
                      work_stealing_thread, scheduler);
    }
    
    printf("Work stealing enabled for %d GPUs\n", scheduler->n_gpus);
}

// Disable work stealing
void disable_work_stealing(AdvancedMultiGPUScheduler *scheduler) {
    if (!scheduler || !scheduler->stealing_active) return;
    
    scheduler->stealing_active = false;
    
    // Wait for stealing threads to finish
    for (int i = 0; i < scheduler->n_gpus; i++) {
        pthread_join(scheduler->stealing_threads[i], NULL);
    }
    
    free(scheduler->stealing_threads);
    scheduler->stealing_threads = NULL;
    
    printf("Work stealing disabled\n");
}

// Work stealing thread implementation
void* work_stealing_thread(void *arg) {
    AdvancedMultiGPUScheduler *scheduler = (AdvancedMultiGPUScheduler*)arg;
    
    while (scheduler->stealing_active) {
        // Find victim GPU with maximum load
        int victim_gpu = -1;
        int max_load = 0;
        
        for (int i = 0; i < scheduler->n_gpus; i++) {
            if (scheduler->queue_sizes[i] > max_load) {
                max_load = scheduler->queue_sizes[i];
                victim_gpu = i;
            }
        }
        
        // Find thief GPU with minimum load
        int thief_gpu = -1;
        int min_load = INT_MAX;
        
        for (int i = 0; i < scheduler->n_gpus; i++) {
            if (scheduler->queue_sizes[i] < min_load) {
                min_load = scheduler->queue_sizes[i];
                thief_gpu = i;
            }
        }
        
        // Perform stealing if beneficial
        if (victim_gpu != -1 && thief_gpu != -1 && victim_gpu != thief_gpu) {
            if (max_load > min_load + 2) { // Stealing threshold
                pthread_mutex_lock(&scheduler->queue_mutexes[victim_gpu]);
                pthread_mutex_lock(&scheduler->queue_mutexes[thief_gpu]);
                
                // Steal half of the excess tasks
                int steal_count = (max_load - min_load) / 2;
                int victim_start = scheduler->queue_sizes[victim_gpu] - steal_count;
                
                for (int i = 0; i < steal_count; i++) {
                    WorkUnit stolen_task = scheduler->work_queues[victim_gpu][victim_start + i];
                    stolen_task.assigned_gpu = thief_gpu;
                    scheduler->work_queues[thief_gpu][scheduler->queue_sizes[thief_gpu]++] = stolen_task;
                }
                
                scheduler->queue_sizes[victim_gpu] -= steal_count;
                
                pthread_mutex_unlock(&scheduler->queue_mutexes[thief_gpu]);
                pthread_mutex_unlock(&scheduler->queue_mutexes[victim_gpu]);
                
                printf("Work stealing: GPU %d stole %d tasks from GPU %d\n", 
                       thief_gpu, steal_count, victim_gpu);
            }
        }
        
        // Sleep for a short interval
        usleep(100000); // 100ms
    }
    
    return NULL;
}

// Adaptive work redistribution based on real-time performance
void rebalance_work_distribution(
    AdvancedMultiGPUScheduler *scheduler) {
    
    if (!scheduler) return;
    
    // Calculate load imbalance
    double max_load = 0.0, min_load = 1e10, total_load = 0.0;
    
    for (int i = 0; i < scheduler->n_gpus; i++) {
        double gpu_load = scheduler->queue_sizes[i];
        max_load = (gpu_load > max_load) ? gpu_load : max_load;
        min_load = (gpu_load < min_load) ? gpu_load : min_load;
        total_load += gpu_load;
    }
    
    double average_load = total_load / scheduler->n_gpus;
    double imbalance_ratio = (max_load - min_load) / (average_load + 1e-6);
    
    // Trigger rebalancing if imbalance exceeds threshold
    if (imbalance_ratio > scheduler->rebalance_threshold) {
        printf("Load imbalance detected: %.2f%% (threshold: %.2f%%)\n", 
               imbalance_ratio * 100, scheduler->rebalance_threshold * 100);
        
        // Redistribute work based on current performance metrics
        // This is a simplified implementation - in practice, you would
        // migrate tasks between GPUs
        
        for (int i = 0; i < scheduler->n_gpus; i++) {
            double target_load = average_load;
            double current_load = scheduler->queue_sizes[i];
            
            if (fabs(current_load - target_load) > 1) {
                printf("GPU %d: Rebalancing from %.1f to %.1f tasks\n", 
                       i, current_load, target_load);
            }
        }
    }
}

// Helper function to compare work units by compute time
int compare_work_unit_compute_time(const void *a, const void *b) {
    const WorkUnit *work_a = (const WorkUnit*)a;
    const WorkUnit *work_b = (const WorkUnit*)b;
    
    if (work_a->estimated_compute_time > work_b->estimated_compute_time) return -1;
    if (work_a->estimated_compute_time < work_b->estimated_compute_time) return 1;
    return 0;
}

// Get current time in seconds
double get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

// Distribute Sommerfeld integral computation across multiple GPUs
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
    gpu_complex *results) {
    
    if (!scheduler) return;
    
    // Create work units for Sommerfeld computation
    WorkUnit *work_units = (WorkUnit*)malloc(n_points * sizeof(WorkUnit));
    
    for (int i = 0; i < n_points; i++) {
        work_units[i].task_id = i;
        work_units[i].start_index = i;
        work_units[i].end_index = i + 1;
        work_units[i].data_size = sizeof(double) * 6 + sizeof(int) * 2; // Input data size
        work_units[i].estimated_compute_time = 1e-6; // Rough estimate
        work_units[i].priority = 1.0;
        work_units[i].assigned_gpu = -1;
        work_units[i].completed = false;
    }
    
    // Distribute work
    distribute_work_advanced(scheduler, work_units, n_points);
    
    // Process work units on assigned GPUs
    for (int gpu_id = 0; gpu_id < scheduler->n_gpus; gpu_id++) {
        pthread_mutex_lock(&scheduler->queue_mutexes[gpu_id]);
        
        for (int j = 0; j < scheduler->queue_sizes[gpu_id]; j++) {
            WorkUnit *unit = &scheduler->work_queues[gpu_id][j];
            
            // Execute Sommerfeld computation on this GPU
            CUDA_CHECK(cudaSetDevice(gpu_id));
            
            int start = unit->start_index;
            int end = unit->end_index;
            int count = end - start;
            
            // Allocate GPU memory for this task
            double *d_krho, *d_weights, *d_z, *d_zp;
            int *d_layer_indices;
            gpu_complex *d_results;
            
            CUDA_CHECK(cudaMalloc(&d_krho, count * sizeof(double)));
            CUDA_CHECK(cudaMalloc(&d_weights, count * sizeof(double)));
            CUDA_CHECK(cudaMalloc(&d_z, count * sizeof(double)));
            CUDA_CHECK(cudaMalloc(&d_zp, count * sizeof(double)));
            CUDA_CHECK(cudaMalloc(&d_layer_indices, count * 2 * sizeof(int)));
            CUDA_CHECK(cudaMalloc(&d_results, count * sizeof(gpu_complex)));
            
            // Copy data to GPU
            CUDA_CHECK(cudaMemcpy(d_krho, &krho_points[start], count * sizeof(double), 
                               cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_weights, &weights[start], count * sizeof(double), 
                               cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_z, &z_coords[start], count * sizeof(double), 
                               cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_zp, &zp_coords[start], count * sizeof(double), 
                               cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_layer_indices, &layer_indices[start * 2], 
                               count * 2 * sizeof(int), cudaMemcpyHostToDevice));
            
            // Launch kernel
            int threads_per_block = 256;
            int blocks_per_grid = (count + threads_per_block - 1) / threads_per_block;
            
            sommerfeld_integral_kernel<<<blocks_per_grid, threads_per_block>>>(
                d_krho, d_weights, d_z, d_zp, d_layer_indices, 
                (double*)medium_params, omega, count, n_layers, d_results);
            
            // Copy results back
            CUDA_CHECK(cudaMemcpy(&results[start], d_results, count * sizeof(gpu_complex), 
                               cudaMemcpyDeviceToHost));
            
            // Cleanup
            CUDA_CHECK(cudaFree(d_krho));
            CUDA_CHECK(cudaFree(d_weights));
            CUDA_CHECK(cudaFree(d_z));
            CUDA_CHECK(cudaFree(d_zp));
            CUDA_CHECK(cudaFree(d_layer_indices));
            CUDA_CHECK(cudaFree(d_results));
            
            unit->completed = true;
        }
        
        pthread_mutex_unlock(&scheduler->queue_mutexes[gpu_id]);
    }
    
    free(work_units);
    printf("Sommerfeld computation distributed across %d GPUs completed\n", scheduler->n_gpus);
}

// Get scheduling performance metrics
SchedulingPerformanceMetrics get_scheduling_performance(
    AdvancedMultiGPUScheduler *scheduler) {
    
    SchedulingPerformanceMetrics metrics = {0};
    
    if (!scheduler) return metrics;
    
    // Calculate total execution time
    double total_time = 0.0;
    for (int i = 0; i < scheduler->n_gpus; i++) {
        total_time += scheduler->task_completion_times[i];
    }
    metrics.total_execution_time = total_time;
    
    // Calculate load imbalance ratio
    double max_load = 0.0, min_load = 1e10, total_load = 0.0;
    for (int i = 0; i < scheduler->n_gpus; i++) {
        double gpu_load = scheduler->queue_sizes[i];
        max_load = (gpu_load > max_load) ? gpu_load : max_load;
        min_load = (gpu_load < min_load) ? gpu_load : min_load;
        total_load += gpu_load;
    }
    
    double average_load = total_load / scheduler->n_gpus;
    metrics.load_imbalance_ratio = (max_load - min_load) / (average_load + 1e-6);
    
    // Calculate average GPU utilization
    double total_utilization = 0.0;
    for (int i = 0; i < scheduler->n_gpus; i++) {
        total_utilization += scheduler->performance_metrics[i].current_utilization;
    }
    metrics.average_gpu_utilization = total_utilization / scheduler->n_gpus;
    
    // Calculate throughput
    int total_tasks = 0;
    for (int i = 0; i < scheduler->n_gpus; i++) {
        total_tasks += scheduler->task_counts[i];
    }
    metrics.throughput_gflops = total_tasks / (total_time + 1e-6);
    
    return metrics;
}

#endif // ENABLE_CUDA

// CPU fallback implementations
#ifndef ENABLE_CUDA

typedef void* AdvancedMultiGPUScheduler;
typedef void* WorkUnit;

AdvancedMultiGPUScheduler* initialize_advanced_scheduler(int n_gpus, int strategy) {
    printf("Advanced multi-GPU scheduling not available - using CPU fallback\n");
    return NULL;
}

void cleanup_advanced_scheduler(AdvancedMultiGPUScheduler *scheduler) {
    // No-op for CPU fallback
}

#endif // !ENABLE_CUDA