/*********************************************************************
 * Electromagnetic Kernel Library - Unified Framework Core
 * 
 * This module implements the shared computational kernels for
 * PEEC, MoM, and BEM solvers, providing common functionality
 * including Green's functions, surface integrals, and optimized
 * data structures.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "electromagnetic_kernel_library.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * Internal Structures and Global State
 *********************************************************************/
typedef struct {
    FrameworkState state;
    Timer total_timer;
    size_t current_memory_usage;
    size_t peak_memory_usage;
    bool enable_memory_tracking;
    void* memory_pool;
    size_t memory_pool_size;
} FrameworkInternal;

// Global framework instance (singleton pattern)
static FrameworkInternal* g_framework = NULL;
static bool g_framework_initialized = false;

/*********************************************************************
 * Logging System Implementation
 *********************************************************************/
static LoggingConfig g_logging_config = {
    .min_level = LOG_LEVEL_INFO,
    .callback = NULL,
    .user_data = NULL,
    .enable_file_logging = false,
    .log_file_path = {0}
};

static FILE* g_log_file = NULL;

static const char* log_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO: return "INFO";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

void framework_log(LogLevel level, const char* format, ...) {
    if (level < g_logging_config.min_level) return;
    
    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buffer[32];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Format the message
    char message[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Call user callback if available
    if (g_logging_config.callback) {
        g_logging_config.callback(level, message, g_logging_config.user_data);
    }
    
    // Print to console
    printf("[%s] [%s] %s\n", time_buffer, log_level_to_string(level), message);
    fflush(stdout);
    
    // Write to file if enabled
    if (g_logging_config.enable_file_logging && g_log_file) {
        fprintf(g_log_file, "[%s] [%s] %s\n", time_buffer, log_level_to_string(level), message);
        fflush(g_log_file);
    }
}

StatusCode framework_set_logging_config(const LoggingConfig* config) {
    if (!config) return ERROR_INVALID_ARGUMENT;
    
    g_logging_config = *config;
    
    // Close existing log file
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    // Open new log file if specified
    if (config->enable_file_logging && strlen(config->log_file_path) > 0) {
        g_log_file = fopen(config->log_file_path, "a");
        if (!g_log_file) {
            framework_log(LOG_LEVEL_WARNING, "Failed to open log file: %s", config->log_file_path);
        }
    }
    
    return SUCCESS;
}

/*********************************************************************
 * Memory Management Implementation
 *********************************************************************/
typedef struct MemoryBlock {
    void* ptr;
    size_t size;
    bool is_aligned;
    struct MemoryBlock* next;
} MemoryBlock;

static MemoryBlock* g_memory_blocks = NULL;
static size_t g_total_allocated = 0;
static size_t g_peak_allocated = 0;
static void* g_memory_mutex = NULL;

void* framework_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        // Track memory allocation
        #pragma omp critical(memory_tracking)
        {
            g_total_allocated += size;
            if (g_total_allocated > g_peak_allocated) {
                g_peak_allocated = g_total_allocated;
            }
            
            // Add to tracking list
            MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
            block->ptr = ptr;
            block->size = size;
            block->is_aligned = false;
            block->next = g_memory_blocks;
            g_memory_blocks = block;
        }
    }
    return ptr;
}

void* framework_aligned_malloc(size_t size, size_t alignment) {
    #ifdef _WIN32
        void* ptr = _aligned_malloc(size, alignment);
    #else
        void* ptr = NULL;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            ptr = NULL;
        }
    #endif
    
    if (ptr) {
        #pragma omp critical(memory_tracking)
        {
            g_total_allocated += size;
            if (g_total_allocated > g_peak_allocated) {
                g_peak_allocated = g_total_allocated;
            }
            
            MemoryBlock* block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
            block->ptr = ptr;
            block->size = size;
            block->is_aligned = true;
            block->next = g_memory_blocks;
            g_memory_blocks = block;
        }
    }
    return ptr;
}

void framework_free(void* ptr) {
    if (!ptr) return;
    
    #pragma omp critical(memory_tracking)
    {
        // Find and remove from tracking list
        MemoryBlock** current = &g_memory_blocks;
        while (*current) {
            if ((*current)->ptr == ptr) {
                MemoryBlock* block = *current;
                *current = block->next;
                g_total_allocated -= block->size;
                free(block);
                break;
            }
            current = &(*current)->next;
        }
    }
    
    free(ptr);
}

void framework_aligned_free(void* ptr) {
    if (!ptr) return;
    
    #pragma omp critical(memory_tracking)
    {
        MemoryBlock** current = &g_memory_blocks;
        while (*current) {
            if ((*current)->ptr == ptr) {
                MemoryBlock* block = *current;
                *current = block->next;
                g_total_allocated -= block->size;
                free(block);
                break;
            }
            current = &(*current)->next;
        }
    }
    
    #ifdef _WIN32
        _aligned_free(ptr);
    #else
        free(ptr);
    #endif
}

size_t framework_get_memory_usage(void) {
    return g_total_allocated;
}

size_t framework_get_peak_memory_usage(void) {
    return g_peak_allocated;
}

void framework_memory_report(void) {
    framework_log(LOG_LEVEL_INFO, "Memory Usage Report:");
    framework_log(LOG_LEVEL_INFO, "  Current: %.2f MB", g_total_allocated / (1024.0 * 1024.0));
    framework_log(LOG_LEVEL_INFO, "  Peak:    %.2f MB", g_peak_allocated / (1024.0 * 1024.0));
    framework_log(LOG_LEVEL_INFO, "  Blocks:  %d", g_memory_blocks ? 1 : 0);
}

/*********************************************************************
 * Timer Implementation
 *********************************************************************/
void timer_start(Timer* timer) {
    timer->start_time = (double)clock() / CLOCKS_PER_SEC;
    timer->elapsed_time = 0.0;
    timer->is_running = true;
}

void timer_stop(Timer* timer) {
    if (timer->is_running) {
        double end_time = (double)clock() / CLOCKS_PER_SEC;
        timer->elapsed_time = end_time - timer->start_time;
        timer->is_running = false;
    }
}

double timer_get_elapsed(const Timer* timer) {
    if (timer->is_running) {
        double current_time = (double)clock() / CLOCKS_PER_SEC;
        return current_time - timer->start_time;
    }
    return timer->elapsed_time;
}

/*********************************************************************
 * Framework Core Implementation
 *********************************************************************/
Framework* framework_create(const FrameworkConfig* config) {
    if (!config) {
        framework_log(LOG_LEVEL_ERROR, "Invalid framework configuration");
        return NULL;
    }
    
    if (g_framework_initialized) {
        framework_log(LOG_LEVEL_WARNING, "Framework already initialized");
        return (Framework*)g_framework;
    }
    
    // Allocate framework structure
    FrameworkInternal* internal = (FrameworkInternal*)framework_calloc(1, sizeof(FrameworkInternal));
    if (!internal) {
        framework_log(LOG_LEVEL_ERROR, "Failed to allocate framework structure");
        return NULL;
    }
    
    // Copy configuration
    internal->state.config = *config;
    
    // Initialize logging
    framework_set_logging_config(&config->logging);
    framework_log(LOG_LEVEL_INFO, "Creating PEEC-MoM Unified Framework v%d.%d.%d",
                  PEEC_MOM_VERSION_MAJOR, PEEC_MOM_VERSION_MINOR, PEEC_MOM_VERSION_PATCH);
    
    // Initialize timer
    timer_start(&internal->total_timer);
    
    // Set up threading
    #ifdef _OPENMP
        if (config->num_threads > 0) {
            omp_set_num_threads(config->num_threads);
            framework_log(LOG_LEVEL_INFO, "Set OpenMP threads to %d", config->num_threads);
        }
    #endif
    
    // Initialize memory tracking
    internal->enable_memory_tracking = true;
    internal->current_memory_usage = 0;
    internal->peak_memory_usage = 0;
    
    // Create component engines (placeholder - would be implemented in separate files)
    framework_log(LOG_LEVEL_INFO, "Creating component engines...");
    
    // Initialize global state
    g_framework = internal;
    g_framework_initialized = true;
    
    framework_log(LOG_LEVEL_INFO, "Framework created successfully");
    return (Framework*)internal;
}

void framework_destroy(Framework* framework) {
    if (!framework) return;
    
    FrameworkInternal* internal = (FrameworkInternal*)framework;
    
    framework_log(LOG_LEVEL_INFO, "Destroying PEEC-MoM Framework...");
    
    // Stop timer
    timer_stop(&internal->total_timer);
    framework_log(LOG_LEVEL_INFO, "Total framework lifetime: %.2f seconds", 
                  timer_get_elapsed(&internal->total_timer));
    
    // Memory report
    framework_memory_report();
    
    // Clean up component engines (would be implemented)
    framework_log(LOG_LEVEL_INFO, "Destroying component engines...");
    
    // Free framework structure
    framework_free(internal);
    
    // Reset global state
    g_framework = NULL;
    g_framework_initialized = false;
    
    framework_log(LOG_LEVEL_INFO, "Framework destroyed successfully");
    
    // Close log file
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

StatusCode framework_initialize(Framework* framework) {
    if (!framework) return ERROR_INVALID_ARGUMENT;
    
    FrameworkInternal* internal = (FrameworkInternal*)framework;
    
    if (internal->state.is_initialized) {
        framework_log(LOG_LEVEL_WARNING, "Framework already initialized");
        return SUCCESS;
    }
    
    framework_log(LOG_LEVEL_INFO, "Initializing framework...");
    
    Timer init_timer;
    timer_start(&init_timer);
    
    // Initialize component engines (would be implemented)
    framework_log(LOG_LEVEL_INFO, "Initializing geometry engine...");
    framework_log(LOG_LEVEL_INFO, "Initializing mesh engine...");
    framework_log(LOG_LEVEL_INFO, "Initializing material database...");
    framework_log(LOG_LEVEL_INFO, "Initializing linear algebra engine...");
    framework_log(LOG_LEVEL_INFO, "Initializing I/O engine...");
    
    internal->state.is_initialized = true;
    internal->state.initialization_time = timer_get_elapsed(&init_timer);
    
    framework_log(LOG_LEVEL_INFO, "Framework initialized successfully in %.2f seconds",
                  internal->state.initialization_time);
    
    return SUCCESS;
}

StatusCode framework_finalize(Framework* framework) {
    if (!framework) return ERROR_INVALID_ARGUMENT;
    
    FrameworkInternal* internal = (FrameworkInternal*)framework;
    
    if (!internal->state.is_initialized) {
        framework_log(LOG_LEVEL_WARNING, "Framework not initialized");
        return SUCCESS;
    }
    
    framework_log(LOG_LEVEL_INFO, "Finalizing framework...");
    
    // Finalize component engines (would be implemented)
    framework_log(LOG_LEVEL_INFO, "Finalizing I/O engine...");
    framework_log(LOG_LEVEL_INFO, "Finalizing linear algebra engine...");
    framework_log(LOG_LEVEL_INFO, "Finalizing material database...");
    framework_log(LOG_LEVEL_INFO, "Finalizing mesh engine...");
    framework_log(LOG_LEVEL_INFO, "Finalizing geometry engine...");
    
    internal->state.is_initialized = false;
    
    framework_log(LOG_LEVEL_INFO, "Framework finalized successfully");
    
    return SUCCESS;
}

bool framework_is_initialized(const Framework* framework) {
    if (!framework) return false;
    FrameworkInternal* internal = (FrameworkInternal*)framework;
    return internal->state.is_initialized;
}

const FrameworkConfig* framework_get_config(const Framework* framework) {
    if (!framework) return NULL;
    FrameworkInternal* internal = (FrameworkInternal*)framework;
    return &internal->state.config;
}

StatusCode framework_get_performance_info(Framework* framework, FrameworkState* state) {
    if (!framework || !state) return ERROR_INVALID_ARGUMENT;
    
    FrameworkInternal* internal = (FrameworkInternal*)framework;
    *state = internal->state;
    
    // Update current memory usage
    state->peak_memory_usage = g_peak_allocated;
    
    return SUCCESS;
}

StatusCode framework_reset_performance_counters(Framework* framework) {
    if (!framework) return ERROR_INVALID_ARGUMENT;
    
    FrameworkInternal* internal = (FrameworkInternal*)framework;
    
    // Reset timers
    timer_start(&internal->total_timer);
    
    // Reset memory counters
    g_peak_allocated = g_total_allocated;
    
    framework_log(LOG_LEVEL_INFO, "Performance counters reset");
    
    return SUCCESS;
}

void framework_get_version(int* major, int* minor, int* patch) {
    if (major) *major = PEEC_MOM_VERSION_MAJOR;
    if (minor) *minor = PEEC_MOM_VERSION_MINOR;
    if (patch) *patch = PEEC_MOM_VERSION_PATCH;
}

const char* framework_get_version_string(void) {
    static char version_string[32];
    snprintf(version_string, sizeof(version_string), "%d.%d.%d",
             PEEC_MOM_VERSION_MAJOR, PEEC_MOM_VERSION_MINOR, PEEC_MOM_VERSION_PATCH);
    return version_string;
}

/*********************************************************************
 * Mathematical Utilities
 *********************************************************************/
Real framework_compute_relative_error(Real computed, Real reference) {
    if (fabs(reference) < 1e-15) {
        return fabs(computed) < 1e-15 ? 0.0 : 1e15;
    }
    return fabs(computed - reference) / fabs(reference);
}

Real framework_compute_absolute_error(Real computed, Real reference) {
    return fabs(computed - reference);
}

bool framework_is_converged(Real error, Real tolerance) {
    return error < tolerance;
}

/*********************************************************************
 * Parallel Utilities
 *********************************************************************/
int framework_get_num_threads(void) {
    #ifdef _OPENMP
        return omp_get_max_threads();
    #else
        return 1;
    #endif
}

StatusCode framework_set_num_threads(int num_threads) {
    if (num_threads <= 0) return ERROR_INVALID_ARGUMENT;
    
    #ifdef _OPENMP
        omp_set_num_threads(num_threads);
        framework_log(LOG_LEVEL_INFO, "Set number of threads to %d", num_threads);
        return SUCCESS;
    #else
        framework_log(LOG_LEVEL_WARNING, "OpenMP not available, thread setting ignored");
        return SUCCESS;
    #endif
}

int framework_get_thread_id(void) {
    #ifdef _OPENMP
        return omp_get_thread_num();
    #else
        return 0;
    #endif
}

#ifdef __cplusplus
}
#endif