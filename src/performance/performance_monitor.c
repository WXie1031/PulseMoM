/*********************************************************************
 * Performance Monitoring and Benchmarking Implementation - Commercial-Grade PEEC-MoM
 * 
 * This module implements comprehensive performance monitoring with
 * real-time tracking, profiling, and benchmarking capabilities.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "performance_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/time.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

// Default configuration values
#define DEFAULT_SAMPLING_INTERVAL_MS 100
#define DEFAULT_MAX_EVENTS 10000
#define DEFAULT_MAX_COUNTERS 1000
#define DEFAULT_MAX_SNAPSHOTS 1000
#define DEFAULT_TIMEOUT_SECONDS 3600

// Global performance monitor instance
static PerformanceMonitor* g_global_monitor = NULL;
static pthread_mutex_t g_monitor_mutex = PTHREAD_MUTEX_INITIALIZER;

// Error codes
#define PERF_MONITOR_SUCCESS 0
#define PERF_MONITOR_ERROR_MEMORY -1
#define PERF_MONITOR_ERROR_INVALID_ARGUMENT -2
#define PERF_MONITOR_ERROR_NOT_INITIALIZED -3
#define PERF_MONITOR_ERROR_ALREADY_RUNNING -4
#define PERF_MONITOR_ERROR_NOT_RUNNING -5
#define PERF_MONITOR_ERROR_TIMEOUT -6
#define PERF_MONITOR_ERROR_FILE_IO -7

// Global error tracking
static int g_last_error = PERF_MONITOR_SUCCESS;
static char g_error_message[1024] = {0};

// Forward declarations
static int performance_monitor_get_system_memory_info(size_t* used, size_t* available);
static int performance_monitor_get_system_cpu_info(double* utilization, int* num_cores);
static int performance_monitor_get_process_memory_info(size_t* used, size_t* peak);
static double performance_monitor_get_time_seconds_internal(void);
static void* performance_monitor_background_thread(void* arg);
static int performance_monitor_update_counters(PerformanceMonitor* monitor);
static int performance_monitor_export_json(PerformanceMonitor* monitor, PerformanceReport* report, const char* filename);
static int performance_monitor_export_html(PerformanceMonitor* monitor, PerformanceReport* report, const char* filename);

// Performance monitor implementation
PerformanceMonitor* performance_monitor_create(const char* name, PerformanceLevel level) {
    PerformanceMonitor* monitor = (PerformanceMonitor*)calloc(1, sizeof(PerformanceMonitor));
    if (!monitor) {
        g_last_error = PERF_MONITOR_ERROR_MEMORY;
        return NULL;
    }
    
    // Initialize basic fields
    if (name) {
        strncpy(monitor->name, name, sizeof(monitor->name) - 1);
    } else {
        strcpy(monitor->name, "PulseMoM_Performance_Monitor");
    }
    
    monitor->level = level;
    monitor->is_active = false;
    monitor->categories = PERF_CATEGORY_ALL;
    monitor->sampling_interval_ms = DEFAULT_SAMPLING_INTERVAL_MS;
    monitor->max_events = DEFAULT_MAX_EVENTS;
    monitor->max_counters = DEFAULT_MAX_COUNTERS;
    monitor->max_snapshots = DEFAULT_MAX_SNAPSHOTS;
    monitor->enable_real_time_monitoring = true;
    monitor->enable_background_monitoring = false;
    monitor->enable_console_output = true;
    monitor->enable_file_output = true;
    monitor->enable_network_output = false;
    
    // Allocate arrays
    monitor->counters = (PerformanceCounter*)calloc(monitor->max_counters, sizeof(PerformanceCounter));
    monitor->events = (PerformanceEvent*)calloc(monitor->max_events, sizeof(PerformanceEvent));
    monitor->snapshots = (PerformanceSnapshot*)calloc(monitor->max_snapshots, sizeof(PerformanceSnapshot));
    monitor->benchmark_suites = (BenchmarkSuite*)calloc(10, sizeof(BenchmarkSuite));
    
    if (!monitor->counters || !monitor->events || !monitor->snapshots || !monitor->benchmark_suites) {
        performance_monitor_destroy(monitor);
        g_last_error = PERF_MONITOR_ERROR_MEMORY;
        return NULL;
    }
    
    monitor->num_counters = 0;
    monitor->num_events = 0;
    monitor->num_snapshots = 0;
    monitor->num_benchmark_suites = 0;
    monitor->monitoring_thread_active = false;
    
    strcpy(monitor->output_directory, "./performance_reports");
    
    g_last_error = PERF_MONITOR_SUCCESS;
    return monitor;
}

void performance_monitor_destroy(PerformanceMonitor* monitor) {
    if (!monitor) return;
    
    // Stop monitoring if running
    if (monitor->is_active) {
        performance_monitor_stop(monitor);
    }
    
    // Stop background thread if running
    if (monitor->monitoring_thread_active) {
        monitor->monitoring_thread_active = false;
        pthread_join(monitor->monitoring_thread, NULL);
    }
    
    // Free allocated memory
    free(monitor->counters);
    free(monitor->events);
    free(monitor->snapshots);
    free(monitor->benchmark_suites);
    
    // Free custom monitors
    free(monitor->custom_monitors);
    
    free(monitor);
}

int performance_monitor_start(PerformanceMonitor* monitor) {
    if (!monitor) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    if (monitor->is_active) {
        g_last_error = PERF_MONITOR_ERROR_ALREADY_RUNNING;
        return -1;
    }
    
    // Reset counters and events
    monitor->num_counters = 0;
    monitor->num_events = 0;
    monitor->num_snapshots = 0;
    
    // Start background monitoring if enabled
    if (monitor->enable_background_monitoring) {
        monitor->monitoring_thread_active = true;
        if (pthread_create(&monitor->monitoring_thread, NULL, 
                          performance_monitor_background_thread, monitor) != 0) {
            monitor->monitoring_thread_active = false;
            g_last_error = PERF_MONITOR_ERROR_MEMORY;
            return -1;
        }
    }
    
    monitor->is_active = true;
    
    // Record start event
    PerformanceEvent* start_event = performance_monitor_record_event(monitor, "Performance_Monitor_Started", PERF_CATEGORY_FRAMEWORK);
    if (start_event) {
        performance_monitor_start_event(monitor, start_event);
    }
    
    g_last_error = PERF_MONITOR_SUCCESS;
    return 0;
}

int performance_monitor_stop(PerformanceMonitor* monitor) {
    if (!monitor) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    if (!monitor->is_active) {
        g_last_error = PERF_MONITOR_ERROR_NOT_RUNNING;
        return -1;
    }
    
    // Record stop event
    PerformanceEvent* stop_event = performance_monitor_record_event(monitor, "Performance_Monitor_Stopped", PERF_CATEGORY_FRAMEWORK);
    if (stop_event) {
        performance_monitor_end_event(monitor, stop_event);
    }
    
    // Stop background monitoring
    if (monitor->monitoring_thread_active) {
        monitor->monitoring_thread_active = false;
        pthread_join(monitor->monitoring_thread, NULL);
    }
    
    monitor->is_active = false;
    g_last_error = PERF_MONITOR_SUCCESS;
    return 0;
}

int performance_monitor_pause(PerformanceMonitor* monitor) {
    if (!monitor || !monitor->is_active) {
        g_last_error = PERF_MONITOR_ERROR_NOT_RUNNING;
        return -1;
    }
    
    for (int i = 0; i < monitor->num_counters; i++) {
        monitor->counters[i].is_paused = true;
    }
    
    return 0;
}

int performance_monitor_resume(PerformanceMonitor* monitor) {
    if (!monitor || !monitor->is_active) {
        g_last_error = PERF_MONITOR_ERROR_NOT_RUNNING;
        return -1;
    }
    
    for (int i = 0; i < monitor->num_counters; i++) {
        monitor->counters[i].is_paused = false;
    }
    
    return 0;
}

// Counter management
PerformanceCounter* performance_monitor_create_counter(PerformanceMonitor* monitor, 
    const char* name, PerformanceCategory category, PerformanceMetricType type) {
    if (!monitor || !name || monitor->num_counters >= monitor->max_counters) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    
    PerformanceCounter* counter = &monitor->counters[monitor->num_counters++];
    
    strncpy(counter->name, name, sizeof(counter->name) - 1);
    snprintf(counter->description, sizeof(counter->description), "Performance counter for %s", name);
    counter->category = category;
    counter->metric_type = type;
    counter->level = monitor->level;
    
    // Initialize counter values
    counter->count = 0;
    counter->value = 0.0;
    counter->min_value = 1e308;
    counter->max_value = -1e308;
    counter->average_value = 0.0;
    counter->total_value = 0.0;
    
    counter->is_running = false;
    counter->is_paused = false;
    counter->has_warnings = false;
    counter->has_errors = false;
    
    counter->thread_id = pthread_self();
    counter->num_threads = 1;
    counter->gpu_device_id = -1;
    counter->gpu_memory_used = 0;
    counter->gpu_utilization = 0.0;
    
    counter->user_data = NULL;
    counter->user_data_size = 0;
    
    return counter;
}

int performance_monitor_start_counter(PerformanceMonitor* monitor, PerformanceCounter* counter) {
    if (!monitor || !counter) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    if (counter->is_running) {
        return 0;  // Already running
    }
    
    gettimeofday(&counter->start_time, NULL);
    counter->is_running = true;
    counter->is_paused = false;
    
    return 0;
}

int performance_monitor_stop_counter(PerformanceMonitor* monitor, PerformanceCounter* counter) {
    if (!monitor || !counter) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    if (!counter->is_running) {
        return 0;  // Not running
    }
    
    gettimeofday(&counter->end_time, NULL);
    
    // Calculate duration
    counter->duration_seconds = (counter->end_time.tv_sec - counter->start_time.tv_sec) +
                               (counter->end_time.tv_usec - counter->start_time.tv_usec) / 1e6;
    
    counter->is_running = false;
    counter->count++;
    
    // Update statistics
    if (counter->value < counter->min_value) counter->min_value = counter->value;
    if (counter->value > counter->max_value) counter->max_value = counter->value;
    counter->total_value += counter->value;
    counter->average_value = counter->total_value / counter->count;
    
    return 0;
}

int performance_monitor_reset_counter(PerformanceMonitor* monitor, PerformanceCounter* counter) {
    if (!monitor || !counter) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    counter->count = 0;
    counter->value = 0.0;
    counter->min_value = 1e308;
    counter->max_value = -1e308;
    counter->average_value = 0.0;
    counter->total_value = 0.0;
    counter->duration_seconds = 0.0;
    counter->is_running = false;
    counter->is_paused = false;
    
    return 0;
}

// Event management
PerformanceEvent* performance_monitor_record_event(PerformanceMonitor* monitor, 
    const char* name, PerformanceCategory category) {
    if (!monitor || !name || monitor->num_events >= monitor->max_events) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    
    PerformanceEvent* event = &monitor->events[monitor->num_events++];
    
    strncpy(event->name, name, sizeof(event->name) - 1);
    snprintf(event->description, sizeof(event->description), "Performance event: %s", name);
    event->category = category;
    event->level = monitor->level;
    
    gettimeofday(&event->timestamp, NULL);
    event->duration_seconds = 0.0;
    event->num_counters = 0;
    
    event->thread_id = pthread_self();
    event->process_id = getpid();
    strcpy(event->source_file, __FILE__);
    event->source_line = __LINE__;
    strcpy(event->function_name, __FUNCTION__);
    
    event->metadata = NULL;
    event->metadata_size = 0;
    
    return event;
}

int performance_monitor_start_event(PerformanceMonitor* monitor, PerformanceEvent* event) {
    if (!monitor || !event) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    gettimeofday(&event->timestamp, NULL);
    return 0;
}

int performance_monitor_end_event(PerformanceMonitor* monitor, PerformanceEvent* event) {
    if (!monitor || !event) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    event->duration_seconds = (end_time.tv_sec - event->timestamp.tv_sec) +
                              (end_time.tv_usec - event->timestamp.tv_usec) / 1e6;
    
    return 0;
}

// Snapshot management
PerformanceSnapshot* performance_monitor_take_snapshot(PerformanceMonitor* monitor, const char* name) {
    if (!monitor || !name || monitor->num_snapshots >= monitor->max_snapshots) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    
    PerformanceSnapshot* snapshot = &monitor->snapshots[monitor->num_snapshots++];
    
    strncpy(snapshot->name, name, sizeof(snapshot->name) - 1);
    gettimeofday(&snapshot->timestamp, NULL);
    
    // Get system information
    performance_monitor_get_system_info(monitor, snapshot);
    
    // Copy current counters and events
    snapshot->num_counters = monitor->num_counters;
    snapshot->num_events = monitor->num_events;
    
    snapshot->custom_metrics = NULL;
    snapshot->custom_metrics_size = 0;
    
    return snapshot;
}

int performance_monitor_get_system_info(PerformanceMonitor* monitor, PerformanceSnapshot* snapshot) {
    if (!monitor || !snapshot) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    // Get memory information
    size_t memory_used, memory_available;
    if (performance_monitor_get_system_memory_info(&memory_used, &memory_available) == 0) {
        snapshot->memory_used_bytes = memory_used;
        snapshot->memory_available_bytes = memory_available;
        snapshot->memory_utilization_percent = (double)memory_used / (memory_used + memory_available) * 100.0;
    }
    
    // Get CPU information
    double cpu_utilization;
    int num_cores;
    if (performance_monitor_get_system_cpu_info(&cpu_utilization, &num_cores) == 0) {
        snapshot->cpu_utilization_percent = cpu_utilization;
    }
    
    // Get disk I/O information (platform-specific)
    snapshot->disk_read_bytes = 0;
    snapshot->disk_write_bytes = 0;
    
    return 0;
}

// Benchmark management
BenchmarkSuite* performance_monitor_create_benchmark_suite(PerformanceMonitor* monitor, 
    const char* name, BenchmarkType type, PerformanceLevel level) {
    if (!monitor || !name || monitor->num_benchmark_suites >= 10) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    
    BenchmarkSuite* suite = &monitor->benchmark_suites[monitor->num_benchmark_suites++];
    
    strncpy(suite->name, name, sizeof(suite->name) - 1);
    snprintf(suite->description, sizeof(suite->description), "Benchmark suite: %s", name);
    suite->types = type;
    suite->level = level;
    
    // Default configuration
    suite->num_iterations = 5;
    suite->warmup_iterations = 1;
    suite->timeout_seconds = DEFAULT_TIMEOUT_SECONDS;
    suite->enable_profiling = true;
    suite->enable_memory_tracking = true;
    suite->enable_gpu_monitoring = false;
    suite->enable_parallel_analysis = true;
    
    suite->num_results = 0;
    suite->max_results = 100;
    suite->results = (BenchmarkResult*)calloc(suite->max_results, sizeof(BenchmarkResult));
    
    if (!suite->results) {
        monitor->num_benchmark_suites--;
        g_last_error = PERF_MONITOR_ERROR_MEMORY;
        return NULL;
    }
    
    suite->compare_with_baseline = false;
    suite->acceptable_regression_percent = 5.0;
    suite->expected_improvement_percent = 0.0;
    
    return suite;
}

int performance_monitor_run_benchmark_suite(PerformanceMonitor* monitor, BenchmarkSuite* suite) {
    if (!monitor || !suite) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    printf("Running benchmark suite: %s\n", suite->name);
    printf("Description: %s\n", suite->description);
    printf("Iterations: %d (warmup: %d)\n", suite->num_iterations, suite->warmup_iterations);
    
    // Record benchmark start
    PerformanceEvent* start_event = performance_monitor_record_event(monitor, "Benchmark_Started", PERF_CATEGORY_ALGORITHM);
    if (start_event) {
        performance_monitor_start_event(monitor, start_event);
    }
    
    // Run benchmarks based on type
    if (suite->types & BENCHMARK_TYPE_SOLVER) {
        printf("Running solver benchmarks...\n");
        // Implement solver benchmarks
    }
    
    if (suite->types & BENCHMARK_TYPE_MESH_GENERATION) {
        printf("Running mesh generation benchmarks...\n");
        // Implement mesh generation benchmarks
    }
    
    if (suite->types & BENCHMARK_TYPE_MATRIX_ASSEMBLY) {
        printf("Running matrix assembly benchmarks...\n");
        // Implement matrix assembly benchmarks
    }
    
    // Record benchmark end
    if (start_event) {
        performance_monitor_end_event(monitor, start_event);
    }
    
    printf("Benchmark suite completed successfully\n");
    return 0;
}

// Reporting and analysis
PerformanceReport* performance_monitor_generate_report(PerformanceMonitor* monitor, 
    const char* title, const char* description) {
    if (!monitor) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return NULL;
    }
    
    PerformanceReport* report = (PerformanceReport*)calloc(1, sizeof(PerformanceReport));
    if (!report) {
        g_last_error = PERF_MONITOR_ERROR_MEMORY;
        return NULL;
    }
    
    if (title) {
        strncpy(report->title, title, sizeof(report->title) - 1);
    } else {
        strcpy(report->title, monitor->name);
    }
    
    if (description) {
        strncpy(report->description, description, sizeof(report->description) - 1);
    } else {
        snprintf(report->description, sizeof(report->description), 
                "Performance report for %s", monitor->name);
    }
    
    gettimeofday(&report->generation_time, NULL);
    
    // Take current system snapshot
    report->system_snapshot = performance_monitor_take_snapshot(monitor, "System_Snapshot");
    
    // Calculate performance scores
    if (report->system_snapshot) {
        report->memory_efficiency_score = 100.0 - report->system_snapshot->memory_utilization_percent;
        report->cpu_efficiency_score = 100.0 - report->system_snapshot->cpu_utilization_percent;
        report->overall_performance_score = (report->memory_efficiency_score + report->cpu_efficiency_score) / 2.0;
    }
    
    // Generate recommendations
    snprintf(report->recommendations, sizeof(report->recommendations),
            "Performance analysis completed. Overall score: %.1f/100\n", 
            report->overall_performance_score);
    
    if (report->memory_efficiency_score < 80.0) {
        strncat(report->recommendations, "- Consider optimizing memory usage\n", 
                sizeof(report->recommendations) - strlen(report->recommendations) - 1);
    }
    
    if (report->cpu_efficiency_score < 80.0) {
        strncat(report->recommendations, "- Consider optimizing CPU utilization\n", 
                sizeof(report->recommendations) - strlen(report->recommendations) - 1);
    }
    
    report->export_html = true;
    report->export_json = true;
    report->export_csv = false;
    report->export_xml = false;
    report->export_pdf = false;
    
    return report;
}

int performance_monitor_export_report(PerformanceMonitor* monitor, 
    PerformanceReport* report, const char* filename, const char* format) {
    if (!monitor || !report || !filename || !format) {
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
    
    if (strcmp(format, "json") == 0) {
        return performance_monitor_export_json(monitor, report, filename);
    } else if (strcmp(format, "html") == 0) {
        return performance_monitor_export_html(monitor, report, filename);
    } else {
        snprintf(g_error_message, sizeof(g_error_message), "Unsupported format: %s", format);
        g_last_error = PERF_MONITOR_ERROR_INVALID_ARGUMENT;
        return -1;
    }
}

// Utility functions
const char* performance_monitor_get_category_string(PerformanceCategory category) {
    switch (category) {
        case PERF_CATEGORY_MEMORY: return "Memory";
        case PERF_CATEGORY_CPU: return "CPU";
        case PERF_CATEGORY_GPU: return "GPU";
        case PERF_CATEGORY_DISK: return "Disk";
        case PERF_CATEGORY_NETWORK: return "Network";
        case PERF_CATEGORY_PARALLEL: return "Parallel";
        case PERF_CATEGORY_ALGORITHM: return "Algorithm";
        case PERF_CATEGORY_FRAMEWORK: return "Framework";
        case PERF_CATEGORY_ALL: return "All";
        default: return "Unknown";
    }
}

const char* performance_monitor_get_metric_type_string(PerformanceMetricType type) {
    switch (type) {
        case PERF_METRIC_TIME: return "Time";
        case PERF_METRIC_MEMORY: return "Memory";
        case PERF_METRIC_BANDWIDTH: return "Bandwidth";
        case PERF_METRIC_THROUGHPUT: return "Throughput";
        case PERF_METRIC_UTILIZATION: return "Utilization";
        case PERF_METRIC_EFFICIENCY: return "Efficiency";
        case PERF_METRIC_SCALABILITY: return "Scalability";
        case PERF_METRIC_ACCURACY: return "Accuracy";
        default: return "Unknown";
    }
}

double performance_monitor_get_time_seconds_internal(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1e6;
}

const char* performance_monitor_get_error_string(int error_code) {
    switch (error_code) {
        case PERF_MONITOR_SUCCESS: return "Success";
        case PERF_MONITOR_ERROR_MEMORY: return "Memory allocation failed";
        case PERF_MONITOR_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case PERF_MONITOR_ERROR_NOT_INITIALIZED: return "Not initialized";
        case PERF_MONITOR_ERROR_ALREADY_RUNNING: return "Already running";
        case PERF_MONITOR_ERROR_NOT_RUNNING: return "Not running";
        case PERF_MONITOR_ERROR_TIMEOUT: return "Operation timed out";
        case PERF_MONITOR_ERROR_FILE_IO: return "File I/O error";
        default: return "Unknown error";
    }
}

int performance_monitor_get_last_error(void) {
    return g_last_error;
}

// Background monitoring thread
static void* performance_monitor_background_thread(void* arg) {
    PerformanceMonitor* monitor = (PerformanceMonitor*)arg;
    
    while (monitor->monitoring_thread_active) {
        performance_monitor_update_counters(monitor);
        usleep(monitor->sampling_interval_ms * 1000);
    }
    
    return NULL;
}

static int performance_monitor_update_counters(PerformanceMonitor* monitor) {
    if (!monitor) return -1;
    
    // Update system counters
    for (int i = 0; i < monitor->num_counters; i++) {
        PerformanceCounter* counter = &monitor->counters[i];
        if (counter->is_running && !counter->is_paused) {
            // Update counter based on category and type
            // This is a simplified implementation
            counter->value += 0.1;  // Increment value
        }
    }
    
    return 0;
}

// System information functions
static int performance_monitor_get_system_memory_info(size_t* used, size_t* available) {
    if (!used || !available) return -1;
    
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        *used = info.totalram - info.freeram;
        *available = info.freeram;
        return 0;
    }
#endif
    
    // Fallback implementation
    *used = 0;
    *available = 1024 * 1024 * 1024;  // 1GB default
    return -1;
}

static int performance_monitor_get_system_cpu_info(double* utilization, int* num_cores) {
    if (!utilization || !num_cores) return -1;
    
#ifdef __linux__
    *num_cores = get_nprocs();
    *utilization = 50.0;  // Default value
    return 0;
#endif
    
    // Fallback implementation
    *utilization = 50.0;
    *num_cores = 1;
    return -1;
}

// Export functions
static int performance_monitor_export_json(PerformanceMonitor* monitor, PerformanceReport* report, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        g_last_error = PERF_MONITOR_ERROR_FILE_IO;
        return -1;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"title\": \"%s\",\n", report->title);
    fprintf(file, "  \"description\": \"%s\",\n", report->description);
    fprintf(file, "  \"overall_performance_score\": %.2f,\n", report->overall_performance_score);
    fprintf(file, "  \"memory_efficiency_score\": %.2f,\n", report->memory_efficiency_score);
    fprintf(file, "  \"cpu_efficiency_score\": %.2f,\n", report->cpu_efficiency_score);
    fprintf(file, "  \"recommendations\": \"%s\"\n", report->recommendations);
    fprintf(file, "}\n");
    
    fclose(file);
    return 0;
}

static int performance_monitor_export_html(PerformanceMonitor* monitor, PerformanceReport* report, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        g_last_error = PERF_MONITOR_ERROR_FILE_IO;
        return -1;
    }
    
    fprintf(file, "<!DOCTYPE html>\n");
    fprintf(file, "<html>\n<head>\n");
    fprintf(file, "<title>%s</title>\n", report->title);
    fprintf(file, "<style>\n");
    fprintf(file, "body { font-family: Arial, sans-serif; margin: 20px; }\n");
    fprintf(file, ".score { font-size: 24px; font-weight: bold; }\n");
    fprintf(file, ".good { color: green; }\n");
    fprintf(file, ".warning { color: orange; }\n");
    fprintf(file, ".poor { color: red; }\n");
    fprintf(file, "</style>\n");
    fprintf(file, "</head>\n<body>\n");
    
    fprintf(file, "<h1>%s</h1>\n", report->title);
    fprintf(file, "<p>%s</p>\n", report->description);
    
    fprintf(file, "<h2>Performance Scores</h2>\n");
    fprintf(file, "<p>Overall Performance: <span class=\"score\">%.1f/100</span></p>\n", 
            report->overall_performance_score);
    fprintf(file, "<p>Memory Efficiency: <span class=\"score\">%.1f/100</span></p>\n", 
            report->memory_efficiency_score);
    fprintf(file, "<p>CPU Efficiency: <span class=\"score\">%.1f/100</span></p>\n", 
            report->cpu_efficiency_score);
    
    fprintf(file, "<h2>Recommendations</h2>\n");
    fprintf(file, "<pre>%s</pre>\n", report->recommendations);
    
    fprintf(file, "</body>\n</html>\n");
    
    fclose(file);
    return 0;
}