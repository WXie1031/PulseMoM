#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <math.h>
#include <time.h>
#include "parallel_io.h"
#include "../utils/memory_utils.h"
#include "../utils/math_utils.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#endif

#define MIN_CHUNK_SIZE (1024 * 1024)
#define MAX_MEMORY_MAPPED_SIZE (1024ULL * 1024 * 1024 * 4)
#define THREAD_STACK_SIZE (1024 * 1024)
#define IO_TIMEOUT_SECONDS 300
#define PROGRESS_UPDATE_INTERVAL 100
#define CACHE_LINE_SIZE 64

ParallelIOProcessor* create_parallel_io_processor(uint32_t max_threads, uint32_t max_formats) {
    ParallelIOProcessor* processor = (ParallelIOProcessor*)safe_malloc(sizeof(ParallelIOProcessor));
    memset(processor, 0, sizeof(ParallelIOProcessor));
    
    processor->threads = (pthread_t*)safe_malloc(max_threads * sizeof(pthread_t));
    processor->formats = (AdvancedFileFormat**)safe_malloc(max_formats * sizeof(AdvancedFileFormat*));
    processor->error_messages = (char**)safe_malloc(max_threads * sizeof(char*));
    
    processor->num_formats = 0;
    processor->completed_operations = 0;
    processor->failed_operations = 0;
    processor->num_errors = 0;
    processor->is_parallel_processing = false;
    
    return processor;
}

void destroy_parallel_io_processor(ParallelIOProcessor* processor) {
    if (!processor) return;
    
    if (processor->is_parallel_processing) {
        stop_parallel_io_processing(processor);
    }
    
    for (uint32_t i = 0; i < processor->num_errors; i++) {
        safe_free(processor->error_messages[i]);
    }
    
    safe_free(processor->threads);
    safe_free(processor->formats);
    safe_free(processor->error_messages);
    safe_free(processor);
}

int configure_parallel_io(ParallelIOProcessor* processor, ParallelIOConfig* config) {
    if (!processor || !config) return -1;
    
    processor->config = config;
    
    uint32_t num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (config->num_threads == 0 || config->num_threads > num_cores * 2) {
        config->num_threads = num_cores;
    }
    
    if (config->chunk_size == 0) {
        config->chunk_size = DEFAULT_IO_CHUNK_SIZE;
    }
    
    if (config->max_memory_mb == 0) {
        config->max_memory_mb = 4096;
    }
    
    return 0;
}

static void update_progress(ParallelIOContext* context, double progress, const char* message) {
    pthread_mutex_lock(&context->progress_mutex);
    context->total_progress = progress;
    
    if (context->config && context->config->enable_progress_callback && context->config->progress_callback) {
        context->config->progress_callback(progress, message);
    }
    
    pthread_mutex_unlock(&context->progress_mutex);
}

static IOChunk* get_next_chunk(ParallelIOContext* context) {
    pthread_mutex_lock(&context->chunk_mutex);
    
    if (context->next_chunk_index >= context->num_chunks) {
        pthread_mutex_unlock(&context->chunk_mutex);
        return NULL;
    }
    
    IOChunk* chunk = &context->chunks[context->next_chunk_index];
    context->next_chunk_index++;
    
    pthread_mutex_unlock(&context->chunk_mutex);
    
    return chunk;
}

static void mark_chunk_completed(IOChunk* chunk, bool success, const char* error_message) {
    chunk->is_completed = true;
    chunk->has_error = !success;
    
    if (!success && error_message) {
        strncpy(chunk->error_message, error_message, sizeof(chunk->error_message) - 1);
        chunk->error_message[sizeof(chunk->error_message) - 1] = '\0';
    }
}

void* parallel_io_worker_thread(void* arg) {
    ParallelIOContext* context = (ParallelIOContext*)arg;
    
    while (!context->should_stop) {
        IOChunk* chunk = get_next_chunk(context);
        if (!chunk) {
            break;
        }
        
        clock_t chunk_start = clock();
        
        int result = process_io_chunk_parallel(chunk, context->format, context->config);
        
        clock_t chunk_end = clock();
        chunk->processing_time_ms = ((double)(chunk_end - chunk_start)) / CLOCKS_PER_SEC * 1000.0;
        
        mark_chunk_completed(chunk, result == 0, result != 0 ? "Chunk processing failed" : NULL);
        
        pthread_mutex_lock(&context->progress_mutex);
        context->total_bytes_processed += chunk->bytes_processed;
        context->total_features_parsed += chunk->features_parsed;
        
        double progress = (double)context->next_chunk_index / context->num_chunks;
        update_progress(context, progress, "Processing chunk");
        
        pthread_mutex_unlock(&context->progress_mutex);
    }
    
    return NULL;
}

int process_io_chunk_sequential(IOChunk* chunk, AdvancedFileFormat* format, ParallelIOConfig* config) {
    if (!chunk || !format) return -1;
    
    clock_t start_time = clock();
    
    FILE* fp = fopen(config->filename, "rb");
    if (!fp) return -1;
    
    if (fseek(fp, chunk->start_offset, SEEK_SET) != 0) {
        fclose(fp);
        return -1;
    }
    
    size_t bytes_to_read = chunk->end_offset - chunk->start_offset;
    chunk->buffer = (uint8_t*)safe_malloc(bytes_to_read + 1);
    
    size_t bytes_read = fread(chunk->buffer, 1, bytes_to_read, fp);
    fclose(fp);
    
    if (bytes_read != bytes_to_read) {
        safe_free(chunk->buffer);
        return -1;
    }
    
    chunk->buffer_size = bytes_read;
    chunk->bytes_processed = bytes_read;
    
    switch (format->format) {
        case FORMAT_GDSII_BINARY:
            for (size_t i = 0; i < bytes_read - 4; i += 2) {
                uint16_t record_length = ntohs(*(uint16_t*)(chunk->buffer + i));
                if (record_length >= 4 && record_length <= 65536) {
                    chunk->features_parsed++;
                }
            }
            break;
        case FORMAT_OASIS_BINARY:
            for (size_t i = 0; i < bytes_read; i++) {
                if (chunk->buffer[i] == 0x1A) {
                    chunk->features_parsed++;
                }
            }
            break;
        case FORMAT_GERBER_RS274X:
            for (size_t i = 0; i < bytes_read; i++) {
                if (chunk->buffer[i] == '*' || chunk->buffer[i] == '\n') {
                    chunk->features_parsed++;
                }
            }
            break;
        default:
            chunk->features_parsed = 1;
            break;
    }
    
    chunk->processing_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    
    return 0;
}

int process_io_chunk_parallel(IOChunk* chunk, AdvancedFileFormat* format, ParallelIOConfig* config) {
    return process_io_chunk_sequential(chunk, format, config);
}

IOChunk* create_io_chunks(const char* filename, uint64_t file_size, ParallelIOConfig* config, uint32_t* num_chunks) {
    if (!filename || !config || !num_chunks) return NULL;
    
    uint64_t optimal_chunk_size = config->chunk_size;
    if (optimal_chunk_size < MIN_CHUNK_SIZE) {
        optimal_chunk_size = MIN_CHUNK_SIZE;
    }
    
    uint32_t chunks_needed = (file_size + optimal_chunk_size - 1) / optimal_chunk_size;
    if (chunks_needed > MAX_IO_THREADS * 4) {
        chunks_needed = MAX_IO_THREADS * 4;
        optimal_chunk_size = file_size / chunks_needed;
    }
    
    IOChunk* chunks = (IOChunk*)safe_malloc(chunks_needed * sizeof(IOChunk));
    memset(chunks, 0, chunks_needed * sizeof(IOChunk));
    
    uint64_t current_offset = 0;
    for (uint32_t i = 0; i < chunks_needed; i++) {
        chunks[i].start_offset = current_offset;
        chunks[i].end_offset = (i == chunks_needed - 1) ? file_size : (current_offset + optimal_chunk_size);
        chunks[i].chunk_size = chunks[i].end_offset - chunks[i].start_offset;
        chunks[i].thread_id = -1;
        chunks[i].is_completed = false;
        chunks[i].has_error = false;
        chunks[i].processing_time_ms = 0.0;
        chunks[i].bytes_processed = 0;
        chunks[i].features_parsed = 0;
        
        current_offset = chunks[i].end_offset;
    }
    
    *num_chunks = chunks_needed;
    return chunks;
}

void destroy_io_chunks(IOChunk* chunks, uint32_t num_chunks) {
    if (!chunks) return;
    
    for (uint32_t i = 0; i < num_chunks; i++) {
        safe_free(chunks[i].buffer);
    }
    
    safe_free(chunks);
}

int perform_memory_mapped_io(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    clock_t start_time = clock();
    
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return -1;
    
    struct stat file_stat;
    if (fstat(fd, &file_stat) != 0) {
        close(fd);
        return -1;
    }
    
    size_t file_size = file_stat.st_size;
    if (file_size > MAX_MEMORY_MAPPED_SIZE) {
        close(fd);
        return -1;
    }
    
    void* mapped_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_data == MAP_FAILED) {
        close(fd);
        return -1;
    }
    
    uint8_t* data = (uint8_t*)mapped_data;
    uint64_t features_parsed = 0;
    
    switch (format->format) {
        case FORMAT_GDSII_BINARY:
            for (size_t i = 0; i < file_size - 4; i += 2) {
                uint16_t record_length = ntohs(*(uint16_t*)(data + i));
                if (record_length >= 4 && record_length <= 65536) {
                    features_parsed++;
                }
            }
            break;
        case FORMAT_OASIS_BINARY:
            for (size_t i = 0; i < file_size; i++) {
                if (data[i] == 0x1A) {
                    features_parsed++;
                }
            }
            break;
        default:
            features_parsed = 1;
            break;
    }
    
    munmap(mapped_data, file_size);
    close(fd);
    
    if (metrics) {
        metrics->parse_time_ms = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        metrics->num_features_parsed = features_parsed;
        metrics->file_size_bytes = file_size;
        metrics->success = true;
    }
    
    return 0;
}

int perform_threaded_io(const char* filename, AdvancedFileFormat* format, ParallelIOConfig* config, FormatPerformanceMetrics* metrics) {
    if (!filename || !format) return -1;
    
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) return -1;
    
    uint64_t file_size = file_stat.st_size;
    uint32_t num_chunks = 0;
    
    IOChunk* chunks = create_io_chunks(filename, file_size, config, &num_chunks);
    if (!chunks) return -1;
    
    ParallelIOContext context;
    memset(&context, 0, sizeof(context));
    context.config = config;
    context.format = format;
    context.chunks = chunks;
    context.num_chunks = num_chunks;
    context.next_chunk_index = 0;
    context.total_bytes_processed = 0;
    context.total_features_parsed = 0;
    context.should_stop = false;
    
    pthread_mutex_init(&context.progress_mutex, NULL);
    pthread_mutex_init(&context.chunk_mutex, NULL);
    pthread_cond_init(&context.chunk_available, NULL);
    
    context.start_time = get_time_seconds();
    
    uint32_t num_threads = config->num_threads;
    if (num_threads > num_chunks) {
        num_threads = num_chunks;
    }
    
    for (uint32_t i = 0; i < num_threads; i++) {
        pthread_create(&context.threads[i], NULL, parallel_io_worker_thread, &context);
    }
    
    for (uint32_t i = 0; i < num_threads; i++) {
        pthread_join(context.threads[i], NULL);
    }
    
    context.end_time = get_time_seconds();
    
    if (metrics) {
        metrics->parse_time_ms = (context.end_time - context.start_time) * 1000.0;
        metrics->num_features_parsed = context.total_features_parsed;
        metrics->file_size_bytes = file_size;
        metrics->success = true;
    }
    
    pthread_mutex_destroy(&context.progress_mutex);
    pthread_mutex_destroy(&context.chunk_mutex);
    pthread_cond_destroy(&context.chunk_available);
    
    destroy_io_chunks(chunks, num_chunks);
    
    return 0;
}

IOPerformanceAnalysis* analyze_io_performance(const char* filename, FileFormatType format, ParallelIOConfig* config) {
    if (!filename) return NULL;
    
    IOPerformanceAnalysis* analysis = (IOPerformanceAnalysis*)safe_malloc(sizeof(IOPerformanceAnalysis));
    memset(analysis, 0, sizeof(IOPerformanceAnalysis));
    
    struct stat file_stat;
    if (stat(filename, &file_stat) != 0) {
        safe_free(analysis);
        return NULL;
    }
    
    analysis->file_size = file_stat.st_size;
    analysis->num_chunks = (analysis->file_size + config->chunk_size - 1) / config->chunk_size;
    
    uint32_t num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    analysis->optimal_thread_count = (analysis->num_chunks < num_cores) ? analysis->num_chunks : num_cores;
    analysis->optimal_chunk_size = config->chunk_size;
    
    analysis->estimated_time_sequential = analysis->file_size / (100 * 1024 * 1024);
    analysis->estimated_time_parallel = analysis->estimated_time_sequential / analysis->optimal_thread_count;
    analysis->speedup_factor = analysis->estimated_time_sequential / analysis->estimated_time_parallel;
    
    analysis->memory_requirement = analysis->file_size * 2;
    analysis->is_memory_mappable = (analysis->file_size < MAX_MEMORY_MAPPED_SIZE);
    analysis->supports_parallel_processing = true;
    
    analysis->bottleneck_analysis = strdup("I/O bandwidth limited");
    
    return analysis;
}

void destroy_io_performance_analysis(IOPerformanceAnalysis* analysis) {
    if (!analysis) return;
    safe_free(analysis->bottleneck_analysis);
    safe_free(analysis);
}

int start_parallel_io_processing(ParallelIOProcessor* processor) {
    if (!processor || processor->is_parallel_processing) return -1;
    
    processor->is_parallel_processing = true;
    processor->completed_operations = 0;
    processor->failed_operations = 0;
    
    return 0;
}

int wait_for_parallel_io_completion(ParallelIOProcessor* processor) {
    if (!processor || !processor->is_parallel_processing) return -1;
    
    while (processor->completed_operations + processor->failed_operations < processor->num_formats) {
        usleep(10000);
    }
    
    processor->is_parallel_processing = false;
    return 0;
}

int stop_parallel_io_processing(ParallelIOProcessor* processor) {
    if (!processor || !processor->is_parallel_processing) return -1;
    
    processor->is_parallel_processing = false;
    return 0;
}

int compare_io_modes(const char* filename, FileFormatType format, ParallelIOMode* modes, uint32_t num_modes, double* performance_results) {
    if (!filename || !modes || !performance_results) return -1;
    
    AdvancedFileFormat* aff = create_advanced_file_format(format);
    if (!aff) return -1;
    
    ParallelIOConfig config;
    memset(&config, 0, sizeof(config));
    config.filename = (char*)filename;
    config.format = format;
    config.num_threads = 4;
    config.chunk_size = DEFAULT_IO_CHUNK_SIZE;
    config.use_memory_mapping = false;
    config.use_compression = false;
    config.enable_validation = true;
    config.enable_error_recovery = true;
    
    for (uint32_t i = 0; i < num_modes; i++) {
        config.io_mode = modes[i];
        FormatPerformanceMetrics metrics;
        memset(&metrics, 0, sizeof(metrics));
        
        int result = -1;
        switch (modes[i]) {
            case IO_MODE_SEQUENTIAL:
                result = read_gdsii_file_advanced(filename, aff, &config, &metrics);
                break;
            case IO_MODE_MEMORY_MAPPED:
                result = perform_memory_mapped_io(filename, aff, &config, &metrics);
                break;
            case IO_MODE_PARALLEL_THREADS:
                result = perform_threaded_io(filename, aff, &config, &metrics);
                break;
            default:
                result = -1;
                break;
        }
        
        performance_results[i] = (result == 0) ? metrics.parse_time_ms : -1.0;
    }
    
    destroy_advanced_file_format(aff);
    return 0;
}

int benchmark_io_scalability(const char* filename, FileFormatType format, uint32_t min_threads, uint32_t max_threads, uint32_t step, double* scalability_results) {
    if (!filename || !scalability_results) return -1;
    
    AdvancedFileFormat* aff = create_advanced_file_format(format);
    if (!aff) return -1;
    
    ParallelIOConfig config;
    memset(&config, 0, sizeof(config));
    config.filename = (char*)filename;
    config.format = format;
    config.io_mode = IO_MODE_PARALLEL_THREADS;
    config.chunk_size = DEFAULT_IO_CHUNK_SIZE;
    config.use_memory_mapping = false;
    config.use_compression = false;
    config.enable_validation = true;
    config.enable_error_recovery = true;
    
    uint32_t result_index = 0;
    for (uint32_t threads = min_threads; threads <= max_threads; threads += step) {
        config.num_threads = threads;
        FormatPerformanceMetrics metrics;
        memset(&metrics, 0, sizeof(metrics));
        
        int result = perform_threaded_io(filename, aff, &config, &metrics);
        scalability_results[result_index++] = (result == 0) ? metrics.parse_time_ms : -1.0;
    }
    
    destroy_advanced_file_format(aff);
    return 0;
}