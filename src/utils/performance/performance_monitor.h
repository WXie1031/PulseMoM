/*********************************************************************
 * Performance Monitoring and Benchmarking System - Commercial-Grade PEEC-MoM
 * 
 * This module implements comprehensive performance monitoring, profiling,
 * and benchmarking capabilities for the unified PEEC-MoM framework.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Performance monitor version
#define PERFORMANCE_MONITOR_VERSION_MAJOR 1
#define PERFORMANCE_MONITOR_VERSION_MINOR 0
#define PERFORMANCE_MONITOR_VERSION_PATCH 0

// Performance categories
typedef enum {
    PERF_CATEGORY_MEMORY = 0x01,
    PERF_CATEGORY_CPU = 0x02,
    PERF_CATEGORY_GPU = 0x04,
    PERF_CATEGORY_DISK = 0x08,
    PERF_CATEGORY_NETWORK = 0x10,
    PERF_CATEGORY_PARALLEL = 0x20,
    PERF_CATEGORY_ALGORITHM = 0x40,
    PERF_CATEGORY_FRAMEWORK = 0x80,
    PERF_CATEGORY_ALL = 0xFF
} PerformanceCategory;

// Performance metrics types
typedef enum {
    PERF_METRIC_TIME = 0x01,
    PERF_METRIC_MEMORY = 0x02,
    PERF_METRIC_BANDWIDTH = 0x04,
    PERF_METRIC_THROUGHPUT = 0x08,
    PERF_METRIC_UTILIZATION = 0x10,
    PERF_METRIC_EFFICIENCY = 0x20,
    PERF_METRIC_SCALABILITY = 0x40,
    PERF_METRIC_ACCURACY = 0x80
} PerformanceMetricType;

// Benchmark types
typedef enum {
    BENCHMARK_TYPE_SOLVER = 0x01,
    BENCHMARK_TYPE_MESH_GENERATION = 0x02,
    BENCHMARK_TYPE_MATRIX_ASSEMBLY = 0x04,
    BENCHMARK_TYPE_LINEAR_SOLVER = 0x08,
    BENCHMARK_TYPE_PRECONDITIONER = 0x10,
    BENCHMARK_TYPE_PARALLEL = 0x20,
    BENCHMARK_TYPE_GPU_KERNEL = 0x40,
    BENCHMARK_TYPE_IO = 0x80,
    BENCHMARK_TYPE_HYBRID = 0x100,
    BENCHMARK_TYPE_ALL = 0x1FF
} BenchmarkType;

// Performance levels
typedef enum {
    PERF_LEVEL_MINIMAL = 0,
    PERF_LEVEL_BASIC = 1,
    PERF_LEVEL_DETAILED = 2,
    PERF_LEVEL_EXTENSIVE = 3,
    PERF_LEVEL_EXHAUSTIVE = 4
} PerformanceLevel;

// Forward declarations
typedef struct PerformanceMonitor PerformanceMonitor;
typedef struct PerformanceCounter PerformanceCounter;
typedef struct PerformanceEvent PerformanceEvent;
typedef struct PerformanceSnapshot PerformanceSnapshot;
typedef struct BenchmarkSuite BenchmarkSuite;
typedef struct BenchmarkResult BenchmarkResult;
typedef struct PerformanceReport PerformanceReport;

// Performance counter structure
typedef struct PerformanceCounter {
    char name[256];
    char description[1024];
    PerformanceCategory category;
    PerformanceMetricType metric_type;
    PerformanceLevel level;
    
    // Counter values
    uint64_t count;
    double value;
    double min_value;
    double max_value;
    double average_value;
    double total_value;
    
    // Timing information
    struct timeval start_time;
    struct timeval end_time;
    double duration_seconds;
    
    // Memory information
    size_t memory_peak;
    size_t memory_average;
    size_t memory_current;
    
    // Thread information
    int thread_id;
    int num_threads;
    
    // GPU information (if applicable)
    int gpu_device_id;
    size_t gpu_memory_used;
    double gpu_utilization;
    
    // Status flags
    bool is_running;
    bool is_paused;
    bool has_warnings;
    bool has_errors;
    
    // Custom data
    void* user_data;
    size_t user_data_size;
} PerformanceCounter;

// Performance event structure
typedef struct PerformanceEvent {
    char name[256];
    char description[1024];
    PerformanceCategory category;
    PerformanceLevel level;
    
    // Event timing
    struct timeval timestamp;
    double duration_seconds;
    
    // Event data
    PerformanceCounter* counters;
    int num_counters;
    
    // Event context
    int thread_id;
    int process_id;
    char source_file[256];
    int source_line;
    char function_name[256];
    
    // Event metadata
    void* metadata;
    size_t metadata_size;
} PerformanceEvent;

// Performance snapshot structure
typedef struct PerformanceSnapshot {
    char name[256];
    struct timeval timestamp;
    
    // System metrics
    double cpu_utilization_percent;
    double memory_utilization_percent;
    size_t memory_used_bytes;
    size_t memory_available_bytes;
    size_t disk_read_bytes;
    size_t disk_write_bytes;
    
    // Application metrics
    PerformanceCounter* counters;
    int num_counters;
    PerformanceEvent* events;
    int num_events;
    
    // Custom metrics
    void* custom_metrics;
    size_t custom_metrics_size;
} PerformanceSnapshot;

// Benchmark result structure
typedef struct BenchmarkResult {
    char name[256];
    char description[1024];
    BenchmarkType type;
    
    // Benchmark timing
    double setup_time_seconds;
    double execution_time_seconds;
    double teardown_time_seconds;
    double total_time_seconds;
    
    // Performance metrics
    double throughput_ops_per_second;
    double bandwidth_bytes_per_second;
    double efficiency_percent;
    double scalability_factor;
    
    // Resource usage
    size_t memory_peak_bytes;
    size_t memory_average_bytes;
    double cpu_utilization_percent;
    double gpu_utilization_percent;
    
    // Accuracy metrics
    double accuracy_score;
    double precision_score;
    double recall_score;
    double f1_score;
    
    // Comparison metrics
    double speedup_factor;
    double improvement_percent;
    double regression_percent;
    
    // Status and errors
    int status;
    char error_message[1024];
    int num_warnings;
    int num_errors;
    
    // Raw data
    PerformanceSnapshot* snapshots;
    int num_snapshots;
    
    // Metadata
    void* metadata;
    size_t metadata_size;
} BenchmarkResult;

// Benchmark suite structure
typedef struct BenchmarkSuite {
    char name[256];
    char description[1024];
    BenchmarkType types;
    PerformanceLevel level;
    
    // Benchmark configuration
    int num_iterations;
    int warmup_iterations;
    double timeout_seconds;
    bool enable_profiling;
    bool enable_memory_tracking;
    bool enable_gpu_monitoring;
    bool enable_parallel_analysis;
    
    // Benchmark results
    BenchmarkResult* results;
    int num_results;
    int max_results;
    
    // Comparison configuration
    char baseline_result_file[1024];
    bool compare_with_baseline;
    double acceptable_regression_percent;
    double expected_improvement_percent;
    
    // Custom benchmarks
    void* custom_benchmarks;
    size_t custom_benchmarks_size;
} BenchmarkSuite;

// Performance report structure
typedef struct PerformanceReport {
    char title[256];
    char description[1024];
    struct timeval generation_time;
    
    // Report sections
    PerformanceSnapshot* system_snapshot;
    BenchmarkResult* benchmark_results;
    int num_benchmarks;
    
    // Analysis results
    double overall_performance_score;
    double memory_efficiency_score;
    double cpu_efficiency_score;
    double gpu_efficiency_score;
    double parallel_efficiency_score;
    double scalability_score;
    
    // Recommendations
    char recommendations[4096];
    char warnings[2048];
    char errors[2048];
    
    // Export options
    bool export_html;
    bool export_json;
    bool export_csv;
    bool export_xml;
    bool export_pdf;
    
    // Custom sections
    void* custom_sections;
    size_t custom_sections_size;
} PerformanceReport;

// Performance monitor structure
typedef struct PerformanceMonitor {
    char name[256];
    bool is_active;
    PerformanceLevel level;
    
    // Monitoring configuration
    PerformanceCategory categories;
    int sampling_interval_ms;
    int max_events;
    int max_counters;
    bool enable_real_time_monitoring;
    bool enable_background_monitoring;
    
    // Data storage
    PerformanceCounter* counters;
    int num_counters;
    int max_counters;
    PerformanceEvent* events;
    int num_events;
    int max_events;
    PerformanceSnapshot* snapshots;
    int num_snapshots;
    int max_snapshots;
    
    // Benchmark suites
    BenchmarkSuite* benchmark_suites;
    int num_benchmark_suites;
    
    // Real-time monitoring
    pthread_t monitoring_thread;
    bool monitoring_thread_active;
    
    // Output configuration
    char output_directory[1024];
    bool enable_console_output;
    bool enable_file_output;
    bool enable_network_output;
    
    // Custom monitoring
    void* custom_monitors;
    size_t custom_monitors_size;
} PerformanceMonitor;

// Core performance monitoring functions
PerformanceMonitor* performance_monitor_create(const char* name, PerformanceLevel level);
void performance_monitor_destroy(PerformanceMonitor* monitor);

int performance_monitor_start(PerformanceMonitor* monitor);
int performance_monitor_stop(PerformanceMonitor* monitor);
int performance_monitor_pause(PerformanceMonitor* monitor);
int performance_monitor_resume(PerformanceMonitor* monitor);

// Counter management
PerformanceCounter* performance_monitor_create_counter(PerformanceMonitor* monitor, 
    const char* name, PerformanceCategory category, PerformanceMetricType type);
int performance_monitor_remove_counter(PerformanceMonitor* monitor, PerformanceCounter* counter);
int performance_monitor_start_counter(PerformanceMonitor* monitor, PerformanceCounter* counter);
int performance_monitor_stop_counter(PerformanceMonitor* monitor, PerformanceCounter* counter);
int performance_monitor_reset_counter(PerformanceMonitor* monitor, PerformanceCounter* counter);

// Event management
PerformanceEvent* performance_monitor_record_event(PerformanceMonitor* monitor, 
    const char* name, PerformanceCategory category);
int performance_monitor_start_event(PerformanceMonitor* monitor, PerformanceEvent* event);
int performance_monitor_end_event(PerformanceMonitor* monitor, PerformanceEvent* event);

// Snapshot management
PerformanceSnapshot* performance_monitor_take_snapshot(PerformanceMonitor* monitor, const char* name);
int performance_monitor_compare_snapshots(PerformanceMonitor* monitor, 
    PerformanceSnapshot* snapshot1, PerformanceSnapshot* snapshot2, PerformanceReport* report);

// Benchmark management
BenchmarkSuite* performance_monitor_create_benchmark_suite(PerformanceMonitor* monitor, 
    const char* name, BenchmarkType type, PerformanceLevel level);
int performance_monitor_run_benchmark_suite(PerformanceMonitor* monitor, BenchmarkSuite* suite);
int performance_monitor_run_single_benchmark(PerformanceMonitor* monitor, 
    BenchmarkSuite* suite, const char* benchmark_name);

// System monitoring
int performance_monitor_get_system_info(PerformanceMonitor* monitor, PerformanceSnapshot* snapshot);
int performance_monitor_get_memory_info(PerformanceMonitor* monitor, size_t* used, size_t* available);
int performance_monitor_get_cpu_info(PerformanceMonitor* monitor, double* utilization, int* num_cores);
int performance_monitor_get_gpu_info(PerformanceMonitor* monitor, int* num_devices, size_t* memory);

// Real-time monitoring
int performance_monitor_start_real_time_monitoring(PerformanceMonitor* monitor, int interval_ms);
int performance_monitor_stop_real_time_monitoring(PerformanceMonitor* monitor);

// Reporting and analysis
PerformanceReport* performance_monitor_generate_report(PerformanceMonitor* monitor, 
    const char* title, const char* description);
int performance_monitor_export_report(PerformanceMonitor* monitor, 
    PerformanceReport* report, const char* filename, const char* format);
int performance_monitor_print_summary(PerformanceMonitor* monitor);
int performance_monitor_print_detailed_report(PerformanceMonitor* monitor);

// Performance optimization
int performance_monitor_analyze_bottlenecks(PerformanceMonitor* monitor, PerformanceReport* report);
int performance_monitor_suggest_optimizations(PerformanceMonitor* monitor, PerformanceReport* report);
int performance_monitor_predict_performance(PerformanceMonitor* monitor, 
    const char* benchmark_name, int problem_size, double* predicted_time);

// Comparison and regression testing
int performance_monitor_compare_with_baseline(PerformanceMonitor* monitor, 
    const char* baseline_file, PerformanceReport* report);
int performance_monitor_detect_regressions(PerformanceMonitor* monitor, 
    PerformanceReport* current, PerformanceReport* baseline, double threshold_percent);
int performance_monitor_validate_performance_requirements(PerformanceMonitor* monitor, 
    PerformanceReport* report, double max_time_seconds, size_t max_memory_bytes);

// Custom monitoring
int performance_monitor_add_custom_counter(PerformanceMonitor* monitor, 
    const char* name, void* data, size_t data_size);
int performance_monitor_add_custom_event(PerformanceMonitor* monitor, 
    const char* name, void* data, size_t data_size);
int performance_monitor_add_custom_benchmark(PerformanceMonitor* monitor, 
    const char* name, void* benchmark_func, void* data);

// Utility functions
const char* performance_monitor_get_category_string(PerformanceCategory category);
const char* performance_monitor_get_metric_type_string(PerformanceMetricType type);
const char* performance_monitor_get_benchmark_type_string(BenchmarkType type);
const char* performance_monitor_get_level_string(PerformanceLevel level);

double performance_monitor_get_time_seconds(void);
size_t performance_monitor_get_memory_usage_bytes(void);
double performance_monitor_get_cpu_utilization_percent(void);

// Error handling
const char* performance_monitor_get_error_string(int error_code);
int performance_monitor_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // PERFORMANCE_MONITOR_H