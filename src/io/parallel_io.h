#ifndef PARALLEL_IO_H
#define PARALLEL_IO_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "advanced_file_formats.h"

#define MAX_IO_THREADS 64
#define DEFAULT_IO_CHUNK_SIZE (1024 * 1024)
#define PARALLEL_IO_BUFFER_SIZE (64 * 1024)
#define MEMORY_MAPPING_THRESHOLD (100 * 1024 * 1024)

typedef enum {
    IO_MODE_SEQUENTIAL,
    IO_MODE_PARALLEL_THREADS,
    IO_MODE_MEMORY_MAPPED,
    IO_MODE_ASYNC_IO,
    IO_MODE_DIRECT_IO
} ParallelIOMode;

typedef enum {
    IO_OPERATION_READ,
    IO_OPERATION_WRITE,
    IO_OPERATION_VALIDATE,
    IO_OPERATION_CONVERT,
    IO_OPERATION_MERGE
} ParallelIOOperation;

typedef struct {
    uint64_t start_offset;
    uint64_t end_offset;
    uint64_t chunk_size;
    uint8_t* buffer;
    size_t buffer_size;
    int thread_id;
    bool is_completed;
    bool has_error;
    char error_message[256];
    double processing_time_ms;
    uint64_t bytes_processed;
    uint64_t features_parsed;
} IOChunk;

typedef struct {
    char* filename;
    FileFormatType format;
    ParallelIOMode io_mode;
    uint32_t num_threads;
    uint64_t chunk_size;
    bool use_memory_mapping;
    bool use_compression;
    bool enable_validation;
    bool enable_error_recovery;
    bool enable_progress_callback;
    uint32_t max_memory_mb;
    uint32_t io_timeout_seconds;
    char* temp_directory;
    void (*progress_callback)(double progress, const char* message);
} ParallelIOConfig;

typedef struct {
    pthread_t* threads;
    IOChunk* chunks;
    uint32_t num_chunks;
    uint32_t num_active_threads;
    bool is_running;
    bool should_stop;
    double total_progress;
    pthread_mutex_t progress_mutex;
    pthread_mutex_t chunk_mutex;
    pthread_cond_t chunk_available;
    uint32_t next_chunk_index;
    uint64_t total_bytes_processed;
    uint64_t total_features_parsed;
    double start_time;
    double end_time;
} ParallelIOContext;

typedef struct {
    AdvancedFileFormat** formats;
    uint32_t num_formats;
    ParallelIOConfig* config;
    ParallelIOContext* context;
    FormatPerformanceMetrics* metrics;
    bool is_parallel_processing;
    uint32_t completed_operations;
    uint32_t failed_operations;
    char** error_messages;
    uint32_t num_errors;
} ParallelIOProcessor;

typedef struct {
    uint32_t chunk_id;
    uint64_t start_byte;
    uint64_t end_byte;
    uint32_t num_records;
    uint32_t num_errors;
    uint32_t num_warnings;
    double parse_time_ms;
    double validation_time_ms;
    size_t memory_used;
    bool is_compressed;
    char* format_specific_data;
} ChunkProcessingResult;

typedef struct {
    uint64_t file_size;
    uint32_t num_chunks;
    uint32_t optimal_thread_count;
    uint64_t optimal_chunk_size;
    double estimated_time_sequential;
    double estimated_time_parallel;
    double speedup_factor;
    uint64_t memory_requirement;
    bool is_memory_mappable;
    bool supports_parallel_processing;
    char* bottleneck_analysis;
} IOPerformanceAnalysis;

ParallelIOProcessor* create_parallel_io_processor(uint32_t max_threads, uint32_t max_formats);
void destroy_parallel_io_processor(ParallelIOProcessor* processor);

int configure_parallel_io(ParallelIOProcessor* processor, ParallelIOConfig* config);
int start_parallel_io_processing(ParallelIOProcessor* processor);
int wait_for_parallel_io_completion(ParallelIOProcessor* processor);
int stop_parallel_io_processing(ParallelIOProcessor* processor);

IOChunk* create_io_chunks(const char* filename, uint64_t file_size, ParallelIOConfig* config, uint32_t* num_chunks);
void destroy_io_chunks(IOChunk* chunks, uint32_t num_chunks);

void* parallel_io_worker_thread(void* arg);
int process_io_chunk_sequential(IOChunk* chunk, AdvancedFileFormat* format, ParallelIOConfig* config);
int process_io_chunk_parallel(IOChunk* chunk, AdvancedFileFormat* format, ParallelIOConfig* config);

int perform_memory_mapped_io(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);
int perform_threaded_io(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);
int perform_async_io(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);

IOPerformanceAnalysis* analyze_io_performance(const char* filename, FileFormatType format, ParallelIOConfig* config);
void destroy_io_performance_analysis(IOPerformanceAnalysis* analysis);

int optimize_io_parameters(const char* filename, FileFormatType format, ParallelIOConfig* config, IOPerformanceAnalysis* analysis);
int auto_tune_io_config(const char* filename, FileFormatType format, ParallelIOConfig* config);

int parallel_read_gdsii(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);
int parallel_write_gdsii(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);

int parallel_read_oasis(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);
int parallel_write_oasis(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);

int parallel_read_gerber(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);
int parallel_write_gerber(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);

int parallel_read_excellon(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);
int parallel_write_excellon(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics);

int parallel_format_conversion(const char* input_filename, FileFormatType input_format, const char* output_filename, FileFormatType output_format, ParallelIOConfig* config);
int parallel_batch_processing(const char** input_files, uint32_t num_files, FileFormatType* input_formats, const char* output_directory, ParallelIOConfig* config);

int enable_io_profiling(ParallelIOProcessor* processor, bool enable);
int get_io_statistics(ParallelIOProcessor* processor, double* throughput_mbps, double* efficiency_percent, uint64_t* total_bytes_processed);

int compare_io_modes(const char* filename, FileFormatType format, ParallelIOMode* modes, uint32_t num_modes, double* performance_results);
int benchmark_io_scalability(const char* filename, FileFormatType format, uint32_t min_threads, uint32_t max_threads, uint32_t step, double* scalability_results);

int implement_io_caching(ParallelIOProcessor* processor, uint64_t cache_size_mb, uint32_t cache_policy);
int clear_io_cache(ParallelIOProcessor* processor);

int set_io_priority(ParallelIOProcessor* processor, uint32_t priority_level);
int get_io_system_info(char** info_string);

#endif