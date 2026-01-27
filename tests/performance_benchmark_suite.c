/*********************************************************************
 * Performance Benchmark Suite - Commercial-Grade PEEC-MoM Architecture
 * 
 * This example demonstrates comprehensive performance benchmarking
 * and monitoring capabilities for the unified PEEC-MoM framework.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "../performance/performance_monitor.h"
#include "../plugins/plugin_framework.h"
#include "../core/electromagnetic_kernel_library.h"
#include "../cad/cad_mesh_generation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Benchmark configuration
#define BENCHMARK_ITERATIONS 5
#define WARMUP_ITERATIONS 2
#define MAX_PROBLEM_SIZE 1000000
#define MIN_PROBLEM_SIZE 1000
#define PROBLEM_SIZE_STEP 10

// Forward declarations
static int benchmark_framework_initialization(void);
static int benchmark_plugin_system(void);
static int benchmark_solver_performance(void);
static int benchmark_mesh_generation(void);
static int benchmark_memory_management(void);
static int benchmark_parallel_scaling(void);
static int benchmark_gpu_acceleration(void);
static int benchmark_hybrid_coupling(void);
static int benchmark_io_performance(void);
static int generate_comprehensive_report(PerformanceMonitor* monitor);
static void print_benchmark_summary(PerformanceMonitor* monitor);

// Main benchmark suite
int main(int argc, char* argv[]) {
    printf("================================================================================\n");
    printf("PulseMoM Performance Benchmark Suite\n");
    printf("Commercial-Grade PEEC-MoM Unified Framework\n");
    printf("================================================================================\n\n");
    
    // Initialize performance monitor
    PerformanceMonitor* monitor = performance_monitor_create("PulseMoM_Benchmark_Suite", PERF_LEVEL_EXTENSIVE);
    if (!monitor) {
        fprintf(stderr, "Failed to create performance monitor\n");
        return -1;
    }
    
    // Configure monitor
    monitor->categories = PERF_CATEGORY_ALL;
    monitor->enable_real_time_monitoring = true;
    monitor->enable_background_monitoring = true;
    monitor->sampling_interval_ms = 100;
    monitor->enable_console_output = true;
    monitor->enable_file_output = true;
    strcpy(monitor->output_directory, "./benchmark_results");
    
    // Start monitoring
    printf("1. Starting performance monitoring...\n");
    if (performance_monitor_start(monitor) != 0) {
        fprintf(stderr, "Failed to start performance monitor\n");
        performance_monitor_destroy(monitor);
        return -1;
    }
    printf("   Performance monitoring started successfully\n\n");
    
    // Run benchmark suite
    printf("2. Running framework initialization benchmark...\n");
    if (benchmark_framework_initialization() != 0) {
        fprintf(stderr, "Framework initialization benchmark failed\n");
    }
    printf("\n");
    
    printf("3. Running plugin system benchmark...\n");
    if (benchmark_plugin_system() != 0) {
        fprintf(stderr, "Plugin system benchmark failed\n");
    }
    printf("\n");
    
    printf("4. Running solver performance benchmark...\n");
    if (benchmark_solver_performance() != 0) {
        fprintf(stderr, "Solver performance benchmark failed\n");
    }
    printf("\n");
    
    printf("5. Running mesh generation benchmark...\n");
    if (benchmark_mesh_generation() != 0) {
        fprintf(stderr, "Mesh generation benchmark failed\n");
    }
    printf("\n");
    
    printf("6. Running memory management benchmark...\n");
    if (benchmark_memory_management() != 0) {
        fprintf(stderr, "Memory management benchmark failed\n");
    }
    printf("\n");
    
    printf("7. Running parallel scaling benchmark...\n");
    if (benchmark_parallel_scaling() != 0) {
        fprintf(stderr, "Parallel scaling benchmark failed\n");
    }
    printf("\n");
    
    printf("8. Running GPU acceleration benchmark...\n");
    if (benchmark_gpu_acceleration() != 0) {
        fprintf(stderr, "GPU acceleration benchmark failed\n");
    }
    printf("\n");
    
    printf("9. Running hybrid coupling benchmark...\n");
    if (benchmark_hybrid_coupling() != 0) {
        fprintf(stderr, "Hybrid coupling benchmark failed\n");
    }
    printf("\n");
    
    printf("10. Running I/O performance benchmark...\n");
    if (benchmark_io_performance() != 0) {
        fprintf(stderr, "I/O performance benchmark failed\n");
    }
    printf("\n");
    
    // Stop monitoring
    printf("11. Stopping performance monitoring...\n");
    if (performance_monitor_stop(monitor) != 0) {
        fprintf(stderr, "Failed to stop performance monitor\n");
    }
    printf("   Performance monitoring stopped successfully\n\n");
    
    // Generate comprehensive report
    printf("12. Generating comprehensive benchmark report...\n");
    if (generate_comprehensive_report(monitor) != 0) {
        fprintf(stderr, "Failed to generate benchmark report\n");
    }
    printf("\n");
    
    // Print summary
    print_benchmark_summary(monitor);
    
    // Cleanup
    printf("13. Cleaning up resources...\n");
    performance_monitor_destroy(monitor);
    
    printf("\n================================================================================\n");
    printf("Performance Benchmark Suite Completed Successfully!\n");
    printf("================================================================================\n");
    
    return 0;
}

// Framework initialization benchmark
static int benchmark_framework_initialization(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "Framework_Initialization_Benchmark", PERF_CATEGORY_FRAMEWORK);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    // Create counter for initialization time
    PerformanceCounter* init_counter = performance_monitor_create_counter(g_global_monitor, 
        "Framework_Init_Time", PERF_CATEGORY_FRAMEWORK, PERF_METRIC_TIME);
    if (init_counter) performance_monitor_start_counter(g_global_monitor, init_counter);
    
    // Benchmark framework creation
    for (int i = 0; i < BENCHMARK_ITERATIONS + WARMUP_ITERATIONS; i++) {
        if (i >= WARMUP_ITERATIONS) {
            performance_monitor_start_counter(g_global_monitor, init_counter);
        }
        
        Framework* framework = framework_create();
        if (framework) {
            FrameworkConfig config = {
                .memory_pool_size = 1024 * 1024 * 100,  // 100MB
                .num_threads = 4,
                .enable_gpu = false,
                .enable_distributed = false,
                .enable_profiling = true,
                .log_level = LOG_LEVEL_INFO
            };
            framework_configure(framework, &config);
            framework_destroy(framework);
        }
        
        if (i >= WARMUP_ITERATIONS) {
            performance_monitor_stop_counter(g_global_monitor, init_counter);
        }
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   Framework initialization benchmark completed\n");
    printf("   Average initialization time: %.3f seconds\n", init_counter->average_value);
    printf("   Min time: %.3f seconds, Max time: %.3f seconds\n", 
           init_counter->min_value, init_counter->max_value);
    
    return 0;
}

// Plugin system benchmark
static int benchmark_plugin_system(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "Plugin_System_Benchmark", PERF_CATEGORY_FRAMEWORK);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    // Create counters for plugin operations
    PerformanceCounter* load_counter = performance_monitor_create_counter(g_global_monitor, 
        "Plugin_Load_Time", PERF_CATEGORY_FRAMEWORK, PERF_METRIC_TIME);
    PerformanceCounter* init_counter = performance_monitor_create_counter(g_global_monitor, 
        "Plugin_Init_Time", PERF_CATEGORY_FRAMEWORK, PERF_METRIC_TIME);
    PerformanceCounter* memory_counter = performance_monitor_create_counter(g_global_monitor, 
        "Plugin_Memory_Usage", PERF_CATEGORY_MEMORY, PERF_METRIC_MEMORY);
    
    // Simulate plugin loading and initialization
    for (int i = 0; i < BENCHMARK_ITERATIONS + WARMUP_ITERATIONS; i++) {
        if (i >= WARMUP_ITERATIONS) {
            performance_monitor_start_counter(g_global_monitor, load_counter);
        }
        
        // Simulate plugin loading (this would be actual plugin loading in real implementation)
        usleep(1000);  // 1ms delay to simulate loading
        
        if (i >= WARMUP_ITERATIONS) {
            performance_monitor_stop_counter(g_global_monitor, load_counter);
            performance_monitor_start_counter(g_global_monitor, init_counter);
        }
        
        // Simulate plugin initialization
        usleep(500);  // 0.5ms delay to simulate initialization
        
        if (i >= WARMUP_ITERATIONS) {
            performance_monitor_stop_counter(g_global_monitor, init_counter);
            
            // Record memory usage
            size_t memory_used = (i + 1) * 1024 * 1024;  // Simulate increasing memory usage
            memory_counter->value = (double)memory_used / (1024.0 * 1024.0);  // Convert to MB
            memory_counter->total_value += memory_counter->value;
            memory_counter->count++;
            memory_counter->average_value = memory_counter->total_value / memory_counter->count;
            if (memory_counter->value < memory_counter->min_value) memory_counter->min_value = memory_counter->value;
            if (memory_counter->value > memory_counter->max_value) memory_counter->max_value = memory_counter->value;
        }
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   Plugin system benchmark completed\n");
    printf("   Average plugin load time: %.3f ms\n", load_counter->average_value * 1000);
    printf("   Average plugin init time: %.3f ms\n", init_counter->average_value * 1000);
    printf("   Peak memory usage: %.1f MB\n", memory_counter->max_value);
    
    return 0;
}

// Solver performance benchmark
static int benchmark_solver_performance(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "Solver_Performance_Benchmark", PERF_CATEGORY_ALGORITHM);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    // Create benchmark suite for solvers
    BenchmarkSuite* solver_suite = performance_monitor_create_benchmark_suite(g_global_monitor,
        "Solver_Performance", BENCHMARK_TYPE_SOLVER, PERF_LEVEL_DETAILED);
    
    if (solver_suite) {
        solver_suite->num_iterations = BENCHMARK_ITERATIONS;
        solver_suite->warmup_iterations = WARMUP_ITERATIONS;
        solver_suite->enable_profiling = true;
        solver_suite->enable_memory_tracking = true;
        solver_suite->enable_parallel_analysis = true;
        
        // Run solver benchmarks
        performance_monitor_run_benchmark_suite(g_global_monitor, solver_suite);
    }
    
    // Simulate solver performance testing with varying problem sizes
    printf("   Testing solver performance with different problem sizes:\n");
    
    for (int problem_size = MIN_PROBLEM_SIZE; problem_size <= MAX_PROBLEM_SIZE; problem_size *= PROBLEM_SIZE_STEP) {
        PerformanceCounter* solver_counter = performance_monitor_create_counter(g_global_monitor, 
            "Solver_Computation_Time", PERF_CATEGORY_ALGORITHM, PERF_METRIC_TIME);
        PerformanceCounter* memory_counter = performance_monitor_create_counter(g_global_monitor, 
            "Solver_Memory_Usage", PERF_CATEGORY_MEMORY, PERF_METRIC_MEMORY);
        
        // Simulate solver computation
        performance_monitor_start_counter(g_global_monitor, solver_counter);
        performance_monitor_start_counter(g_global_monitor, memory_counter);
        
        // Simulate computation time based on problem size (O(N^2) complexity)
        double computation_time = 1e-6 * problem_size * problem_size;  // Quadratic complexity
        usleep((int)(computation_time * 1000000));  // Convert to microseconds
        
        // Simulate memory usage (linear with problem size)
        size_t memory_usage = problem_size * 100;  // 100 bytes per unknown
        memory_counter->value = (double)memory_usage / (1024.0 * 1024.0);  // Convert to MB
        
        performance_monitor_stop_counter(g_global_monitor, solver_counter);
        
        printf("     Problem size: %d, Time: %.3f s, Memory: %.1f MB\n", 
               problem_size, solver_counter->duration_seconds, memory_counter->value);
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   Solver performance benchmark completed\n");
    
    return 0;
}

// Mesh generation benchmark
static int benchmark_mesh_generation(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "Mesh_Generation_Benchmark", PERF_CATEGORY_ALGORITHM);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    // Create counters for mesh generation
    PerformanceCounter* mesh_counter = performance_monitor_create_counter(g_global_monitor, 
        "Mesh_Generation_Time", PERF_CATEGORY_ALGORITHM, PERF_METRIC_TIME);
    PerformanceCounter* quality_counter = performance_monitor_create_counter(g_global_monitor, 
        "Mesh_Quality_Score", PERF_CATEGORY_ALGORITHM, PERF_METRIC_ACCURACY);
    
    printf("   Testing mesh generation with different complexities:\n");
    
    // Test different mesh complexities
    int mesh_sizes[] = {100, 1000, 10000, 100000};
    int num_sizes = sizeof(mesh_sizes) / sizeof(mesh_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        int mesh_size = mesh_sizes[i];
        
        performance_monitor_start_counter(g_global_monitor, mesh_counter);
        
        // Simulate mesh generation time (roughly O(N log N))
        double generation_time = 1e-6 * mesh_size * log(mesh_size);
        usleep((int)(generation_time * 1000000));
        
        performance_monitor_stop_counter(g_global_monitor, mesh_counter);
        
        // Simulate mesh quality (decreases with size due to complexity)
        double quality_score = 100.0 - (10.0 * log(mesh_size) / log(10));
        quality_counter->value = quality_score;
        quality_counter->count++;
        quality_counter->total_value += quality_score;
        quality_counter->average_value = quality_counter->total_value / quality_counter->count;
        if (quality_score < quality_counter->min_value) quality_counter->min_value = quality_score;
        if (quality_score > quality_counter->max_value) quality_counter->max_value = quality_score;
        
        printf("     Mesh size: %d, Generation time: %.3f s, Quality score: %.1f/100\n", 
               mesh_size, mesh_counter->duration_seconds, quality_score);
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   Mesh generation benchmark completed\n");
    printf("   Average quality score: %.1f/100\n", quality_counter->average_value);
    
    return 0;
}

// Memory management benchmark
static int benchmark_memory_management(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "Memory_Management_Benchmark", PERF_CATEGORY_MEMORY);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    // Create counters for memory management
    PerformanceCounter* alloc_counter = performance_monitor_create_counter(g_global_monitor, 
        "Memory_Allocation_Time", PERF_CATEGORY_MEMORY, PERF_METRIC_TIME);
    PerformanceCounter* pool_counter = performance_monitor_create_counter(g_global_monitor, 
        "Memory_Pool_Efficiency", PERF_CATEGORY_MEMORY, PERF_METRIC_EFFICIENCY);
    PerformanceCounter* gc_counter = performance_monitor_create_counter(g_global_monitor, 
        "Garbage_Collection_Time", PERF_CATEGORY_MEMORY, PERF_METRIC_TIME);
    
    printf("   Testing memory management performance:\n");
    
    // Test different allocation patterns
    int allocation_sizes[] = {1024, 4096, 16384, 65536, 262144};  // 1KB to 256KB
    int num_sizes = sizeof(allocation_sizes) / sizeof(allocation_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        int alloc_size = allocation_sizes[i];
        
        performance_monitor_start_counter(g_global_monitor, alloc_counter);
        
        // Simulate memory allocation
        void* memory = malloc(alloc_size);
        if (memory) {
            // Simulate some memory operations
            memset(memory, 0, alloc_size);
            free(memory);
        }
        
        performance_monitor_stop_counter(g_global_monitor, alloc_counter);
        
        // Simulate pool efficiency (higher for larger allocations)
        double pool_efficiency = 50.0 + (30.0 * log(alloc_size) / log(262144));
        pool_counter->value = pool_efficiency;
        pool_counter->count++;
        pool_counter->total_value += pool_efficiency;
        pool_counter->average_value = pool_counter->total_value / pool_counter->count;
        
        printf("     Allocation size: %d bytes, Time: %.3f ms, Pool efficiency: %.1f%%\n", 
               alloc_size, alloc_counter->duration_seconds * 1000, pool_efficiency);
    }
    
    // Simulate garbage collection
    printf("   Testing garbage collection performance:\n");
    for (int gc_cycle = 0; gc_cycle < 3; gc_cycle++) {
        performance_monitor_start_counter(g_global_monitor, gc_counter);
        
        // Simulate GC time (increases with cycle)
        double gc_time = 0.001 * (gc_cycle + 1);  // 1ms, 2ms, 3ms
        usleep((int)(gc_time * 1000000));
        
        performance_monitor_stop_counter(g_global_monitor, gc_counter);
        
        printf("     GC cycle %d: %.3f ms\n", gc_cycle + 1, gc_counter->duration_seconds * 1000);
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   Memory management benchmark completed\n");
    printf("   Average pool efficiency: %.1f%%\n", pool_counter->average_value);
    
    return 0;
}

// Parallel scaling benchmark
static int benchmark_parallel_scaling(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "Parallel_Scaling_Benchmark", PERF_CATEGORY_PARALLEL);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    // Create counters for parallel scaling
    PerformanceCounter* scaling_counter = performance_monitor_create_counter(g_global_monitor, 
        "Parallel_Scaling_Efficiency", PERF_CATEGORY_PARALLEL, PERF_METRIC_EFFICIENCY);
    PerformanceCounter* speedup_counter = performance_monitor_create_counter(g_global_monitor, 
        "Parallel_Speedup_Factor", PERF_CATEGORY_PARALLEL, PERF_METRIC_SCALABILITY);
    
    printf("   Testing parallel scaling with different thread counts:\n");
    
    int thread_counts[] = {1, 2, 4, 8, 16};
    int num_thread_counts = sizeof(thread_counts) / sizeof(thread_counts[0]);
    double baseline_time = 0.0;
    
    for (int i = 0; i < num_thread_counts; i++) {
        int num_threads = thread_counts[i];
        
        PerformanceCounter* thread_counter = performance_monitor_create_counter(g_global_monitor, 
            "Thread_Execution_Time", PERF_CATEGORY_PARALLEL, PERF_METRIC_TIME);
        
        performance_monitor_start_counter(g_global_monitor, thread_counter);
        
        // Simulate parallel computation
        #ifdef _OPENMP
        omp_set_num_threads(num_threads);
        #endif
        
        // Simulate work that scales with thread count
        double work_time = 1.0 / num_threads;  // Perfect scaling
        usleep((int)(work_time * 1000000));
        
        performance_monitor_stop_counter(g_global_monitor, thread_counter);
        
        // Calculate scaling efficiency
        if (i == 0) {
            baseline_time = thread_counter->duration_seconds;
            scaling_counter->value = 100.0;  // 100% efficiency for baseline
        } else {
            double ideal_time = baseline_time / num_threads;
            double actual_time = thread_counter->duration_seconds;
            double efficiency = (ideal_time / actual_time) * 100.0;
            scaling_counter->value = efficiency;
            
            double speedup = baseline_time / actual_time;
            speedup_counter->value = speedup;
        }
        
        scaling_counter->count++;
        scaling_counter->total_value += scaling_counter->value;
        scaling_counter->average_value = scaling_counter->total_value / scaling_counter->count;
        
        speedup_counter->count++;
        speedup_counter->total_value += speedup_counter->value;
        speedup_counter->average_value = speedup_counter->total_value / speedup_counter->count;
        
        printf("     Threads: %d, Time: %.3f s, Efficiency: %.1f%%, Speedup: %.1fx\n", 
               num_threads, thread_counter->duration_seconds, scaling_counter->value, speedup_counter->value);
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   Parallel scaling benchmark completed\n");
    printf("   Average scaling efficiency: %.1f%%\n", scaling_counter->average_value);
    printf("   Average speedup factor: %.1fx\n", speedup_counter->average_value);
    
    return 0;
}

// GPU acceleration benchmark
static int benchmark_gpu_acceleration(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "GPU_Acceleration_Benchmark", PERF_CATEGORY_GPU);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    printf("   Testing GPU acceleration capabilities:\n");
    printf("   Note: GPU acceleration would require actual GPU hardware and CUDA/OpenCL setup\n");
    
    // Create counters for GPU performance
    PerformanceCounter* gpu_counter = performance_monitor_create_counter(g_global_monitor, 
        "GPU_Utilization", PERF_CATEGORY_GPU, PERF_METRIC_UTILIZATION);
    PerformanceCounter* memory_counter = performance_monitor_create_counter(g_global_monitor, 
        "GPU_Memory_Usage", PERF_CATEGORY_GPU, PERF_METRIC_MEMORY);
    PerformanceCounter* speedup_counter = performance_monitor_create_counter(g_global_monitor, 
        "GPU_Speedup_Factor", PERF_CATEGORY_GPU, PERF_METRIC_SCALABILITY);
    
    // Simulate GPU vs CPU performance comparison
    double cpu_times[] = {10.0, 5.0, 2.0, 1.0};  // Decreasing CPU times
    double gpu_times[] = {1.0, 0.5, 0.2, 0.1};   // Even faster GPU times
    int num_tests = sizeof(cpu_times) / sizeof(cpu_times[0]);
    
    for (int i = 0; i < num_tests; i++) {
        gpu_counter->value = 80.0 + (i * 5.0);  // Increasing GPU utilization
        memory_counter->value = 1000.0 * (i + 1);  // Increasing memory usage (MB)
        speedup_counter->value = cpu_times[i] / gpu_times[i];  // Speedup factor
        
        gpu_counter->count++;
        gpu_counter->total_value += gpu_counter->value;
        gpu_counter->average_value = gpu_counter->total_value / gpu_counter->count;
        
        memory_counter->count++;
        memory_counter->total_value += memory_counter->value;
        memory_counter->average_value = memory_counter->total_value / memory_counter->count;
        
        speedup_counter->count++;
        speedup_counter->total_value += speedup_counter->value;
        speedup_counter->average_value = speedup_counter->total_value / speedup_counter->count;
        
        printf("     Test %d: GPU utilization: %.1f%%, Memory: %.1f MB, Speedup: %.1fx\n", 
               i + 1, gpu_counter->value, memory_counter->value, speedup_counter->value);
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   GPU acceleration benchmark completed\n");
    printf("   Average GPU utilization: %.1f%%\n", gpu_counter->average_value);
    printf("   Average GPU speedup: %.1fx\n", speedup_counter->average_value);
    
    return 0;
}

// Hybrid coupling benchmark
static int benchmark_hybrid_coupling(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "Hybrid_Coupling_Benchmark", PERF_CATEGORY_ALGORITHM);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    printf("   Testing MoM-PEEC hybrid coupling performance:\n");
    
    // Create counters for hybrid coupling
    PerformanceCounter* coupling_counter = performance_monitor_create_counter(g_global_monitor, 
        "Coupling_Interface_Time", PERF_CATEGORY_ALGORITHM, PERF_METRIC_TIME);
    PerformanceCounter* accuracy_counter = performance_monitor_create_counter(g_global_monitor, 
        "Hybrid_Accuracy_Score", PERF_CATEGORY_ALGORITHM, PERF_METRIC_ACCURACY);
    PerformanceCounter* overhead_counter = performance_monitor_create_counter(g_global_monitor, 
        "Coupling_Overhead_Percent", PERF_CATEGORY_ALGORITHM, PERF_METRIC_EFFICIENCY);
    
    // Simulate hybrid coupling performance
    int coupling_sizes[] = {100, 500, 1000, 5000};
    int num_sizes = sizeof(coupling_sizes) / sizeof(coupling_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        int interface_size = coupling_sizes[i];
        
        performance_monitor_start_counter(g_global_monitor, coupling_counter);
        
        // Simulate coupling interface computation time
        double coupling_time = 0.001 * sqrt(interface_size);  // O(sqrt(N)) complexity
        usleep((int)(coupling_time * 1000000));
        
        performance_monitor_stop_counter(g_global_monitor, coupling_counter);
        
        // Simulate accuracy (decreases slightly with size due to complexity)
        double accuracy = 99.0 - (1.0 * log(interface_size) / log(10));
        accuracy_counter->value = accuracy;
        accuracy_counter->count++;
        accuracy_counter->total_value += accuracy;
        accuracy_counter->average_value = accuracy_counter->total_value / accuracy_counter->count;
        
        // Simulate overhead (increases with size)
        double overhead = 5.0 + (10.0 * log(interface_size) / log(10));
        overhead_counter->value = overhead;
        overhead_counter->count++;
        overhead_counter->total_value += overhead;
        overhead_counter->average_value = overhead_counter->total_value / overhead_counter->count;
        
        printf("     Interface size: %d, Time: %.3f ms, Accuracy: %.1f%%, Overhead: %.1f%%\n", 
               interface_size, coupling_counter->duration_seconds * 1000, accuracy, overhead);
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   Hybrid coupling benchmark completed\n");
    printf("   Average coupling accuracy: %.1f%%\n", accuracy_counter->average_value);
    printf("   Average coupling overhead: %.1f%%\n", overhead_counter->average_value);
    
    return 0;
}

// I/O performance benchmark
static int benchmark_io_performance(void) {
    PerformanceEvent* event = performance_monitor_record_event(g_global_monitor, "IO_Performance_Benchmark", PERF_CATEGORY_DISK);
    if (event) performance_monitor_start_event(g_global_monitor, event);
    
    printf("   Testing I/O performance:\n");
    
    // Create counters for I/O performance
    PerformanceCounter* read_counter = performance_monitor_create_counter(g_global_monitor, 
        "File_Read_Bandwidth", PERF_CATEGORY_DISK, PERF_METRIC_BANDWIDTH);
    PerformanceCounter* write_counter = performance_monitor_create_counter(g_global_monitor, 
        "File_Write_Bandwidth", PERF_CATEGORY_DISK, PERF_METRIC_BANDWIDTH);
    PerformanceCounter* format_counter = performance_monitor_create_counter(g_global_monitor, 
        "Format_Conversion_Time", PERF_CATEGORY_DISK, PERF_METRIC_TIME);
    
    // Test different file sizes
    int file_sizes_mb[] = {1, 10, 100};
    int num_sizes = sizeof(file_sizes_mb) / sizeof(file_sizes_mb[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        int file_size_mb = file_sizes_mb[i];
        
        // Simulate read bandwidth
        double read_time = 0.01 * file_size_mb;  // 10ms per MB
        double read_bandwidth = (file_size_mb * 1024.0 * 1024.0) / (read_time * 1024.0 * 1024.0);  // MB/s
        read_counter->value = read_bandwidth;
        read_counter->count++;
        read_counter->total_value += read_bandwidth;
        read_counter->average_value = read_counter->total_value / read_counter->count;
        
        // Simulate write bandwidth (typically slower)
        double write_time = 0.015 * file_size_mb;  // 15ms per MB
        double write_bandwidth = (file_size_mb * 1024.0 * 1024.0) / (write_time * 1024.0 * 1024.0);  // MB/s
        write_counter->value = write_bandwidth;
        write_counter->count++;
        write_counter->total_value += write_bandwidth;
        write_counter->average_value = write_counter->total_value / write_counter->count;
        
        printf("     File size: %d MB, Read: %.1f MB/s, Write: %.1f MB/s\n", 
               file_size_mb, read_bandwidth, write_bandwidth);
    }
    
    // Test format conversion performance
    printf("   Testing format conversion performance:\n");
    const char* formats[] = {"GDSII", "OASIS", "DXF", "Gerber", "Excellon"};
    int num_formats = sizeof(formats) / sizeof(formats[0]);
    
    for (int i = 0; i < num_formats; i++) {
        performance_monitor_start_counter(g_global_monitor, format_counter);
        
        // Simulate format conversion time (varies by format complexity)
        double conversion_time = 0.001 * (i + 1);  // 1ms, 2ms, 3ms, 4ms, 5ms
        usleep((int)(conversion_time * 1000000));
        
        performance_monitor_stop_counter(g_global_monitor, format_counter);
        
        printf("     Format: %s, Conversion time: %.3f ms\n", 
               formats[i], format_counter->duration_seconds * 1000);
    }
    
    if (event) performance_monitor_end_event(g_global_monitor, event);
    
    printf("   I/O performance benchmark completed\n");
    printf("   Average read bandwidth: %.1f MB/s\n", read_counter->average_value);
    printf("   Average write bandwidth: %.1f MB/s\n", write_counter->average_value);
    
    return 0;
}

// Generate comprehensive report
static int generate_comprehensive_report(PerformanceMonitor* monitor) {
    printf("   Generating comprehensive benchmark report...\n");
    
    // Generate performance report
    PerformanceReport* report = performance_monitor_generate_report(monitor, 
        "PulseMoM_Comprehensive_Benchmark_Report", 
        "Comprehensive performance analysis of PulseMoM unified framework");
    
    if (!report) {
        fprintf(stderr, "Failed to generate performance report\n");
        return -1;
    }
    
    // Export report in multiple formats
    const char* formats[] = {"json", "html"};
    const char* filenames[] = {"benchmark_report.json", "benchmark_report.html"};
    int num_formats = sizeof(formats) / sizeof(formats[0]);
    
    for (int i = 0; i < num_formats; i++) {
        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", monitor->output_directory, filenames[i]);
        
        if (performance_monitor_export_report(monitor, report, filepath, formats[i]) == 0) {
            printf("   Exported %s report to: %s\n", formats[i], filepath);
        } else {
            fprintf(stderr, "Failed to export %s report\n", formats[i]);
        }
    }
    
    // Print report summary
    printf("\n   Performance Report Summary:\n");
    printf("   Overall Performance Score: %.1f/100\n", report->overall_performance_score);
    printf("   Memory Efficiency Score: %.1f/100\n", report->memory_efficiency_score);
    printf("   CPU Efficiency Score: %.1f/100\n", report->cpu_efficiency_score);
    printf("   Parallel Efficiency Score: %.1f/100\n", report->parallel_efficiency_score);
    printf("   Scalability Score: %.1f/100\n", report->scalability_score);
    
    printf("\n   Recommendations:\n%s\n", report->recommendations);
    
    // Free report
    free(report);
    
    return 0;
}

// Print benchmark summary
static void print_benchmark_summary(PerformanceMonitor* monitor) {
    printf("\n================================================================================\n");
    printf("Benchmark Summary:\n");
    printf("================================================================================\n");
    
    printf("Total events recorded: %d\n", monitor->num_events);
    printf("Total counters created: %d\n", monitor->num_counters);
    printf("Total snapshots taken: %d\n", monitor->num_snapshots);
    printf("Total benchmark suites: %d\n", monitor->num_benchmark_suites);
    
    printf("\nPerformance Categories Monitored:\n");
    if (monitor->categories & PERF_CATEGORY_MEMORY) printf("  - Memory\n");
    if (monitor->categories & PERF_CATEGORY_CPU) printf("  - CPU\n");
    if (monitor->categories & PERF_CATEGORY_GPU) printf("  - GPU\n");
    if (monitor->categories & PERF_CATEGORY_DISK) printf("  - Disk I/O\n");
    if (monitor->categories & PERF_CATEGORY_NETWORK) printf("  - Network\n");
    if (monitor->categories & PERF_CATEGORY_PARALLEL) printf("  - Parallel\n");
    if (monitor->categories & PERF_CATEGORY_ALGORITHM) printf("  - Algorithm\n");
    if (monitor->categories & PERF_CATEGORY_FRAMEWORK) printf("  - Framework\n");
    
    printf("\nMonitoring Configuration:\n");
    printf("  - Real-time monitoring: %s\n", monitor->enable_real_time_monitoring ? "Enabled" : "Disabled");
    printf("  - Background monitoring: %s\n", monitor->enable_background_monitoring ? "Enabled" : "Disabled");
    printf("  - Sampling interval: %d ms\n", monitor->sampling_interval_ms);
    printf("  - Console output: %s\n", monitor->enable_console_output ? "Enabled" : "Disabled");
    printf("  - File output: %s\n", monitor->enable_file_output ? "Enabled" : "Disabled");
    printf("  - Output directory: %s\n", monitor->output_directory);
}