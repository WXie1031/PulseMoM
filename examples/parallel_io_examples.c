/*********************************************************************
 * Parallel I/O Processing Usage Examples
 * 
 * This file demonstrates comprehensive usage of the parallel I/O system
 * for high-performance file processing in electromagnetic simulations.
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "../src/io/parallel_io.h"
#include "../src/io/advanced_file_formats.h"
#include "../src/io/memory_optimization.h"
#include "../src/io/format_validation.h"

/*********************************************************************
 * Example 1: Basic Parallel File Reading
 *********************************************************************/
int example_basic_parallel_reading() {
    printf("=== Example 1: Basic Parallel File Reading ===\n");
    
    // Configure parallel I/O
    ParallelIOConfig config = {
        .mode = IO_MODE_PARALLEL_THREADS,
        .num_threads = 4,
        .chunk_size = 1024 * 1024,  // 1MB chunks
        .use_memory_mapping = 0,
        .buffer_size = 32 * 1024 * 1024  // 32MB buffer
    };
    
    // Initialize parallel I/O context
    ParallelIOContext* context = parallel_io_init(&config);
    if (!context) {
        printf("Failed to initialize parallel I/O\n");
        return -1;
    }
    
    // Open file for parallel reading
    const char* filename = "large_design.gds";
    ParallelFile* pfile = parallel_io_open(context, filename, "r");
    if (!pfile) {
        printf("Failed to open file: %s\n", filename);
        parallel_io_cleanup(context);
        return -1;
    }
    
    // Get file information
    FileInfo info = parallel_io_get_file_info(pfile);
    printf("File: %s\n", filename);
    printf("Size: %.2f MB\n", info.size / (1024.0 * 1024.0));
    printf("Chunks: %d\n", info.num_chunks);
    printf("Optimal chunk size: %d bytes\n", info.optimal_chunk_size);
    
    // Read file in parallel
    clock_t start = clock();
    
    void* buffer = malloc(info.size);
    if (!buffer) {
        printf("Memory allocation failed\n");
        parallel_io_close(pfile);
        parallel_io_cleanup(context);
        return -1;
    }
    
    // Parallel read operation
    size_t bytes_read = parallel_io_read(pfile, buffer, info.size);
    
    clock_t end = clock();
    double read_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Bytes read: %zu\n", bytes_read);
    printf("Read time: %.3f seconds\n", read_time);
    printf("Read speed: %.2f MB/s\n", (bytes_read / (1024.0 * 1024.0)) / read_time);
    
    // Analyze read performance
    ParallelIOStats stats = parallel_io_get_stats(context);
    printf("\nPerformance Statistics:\n");
    printf("  Total I/O operations: %d\n", stats.total_io_ops);
    printf("  Successful operations: %d\n", stats.successful_ops);
    printf("  Failed operations: %d\n", stats.failed_ops);
    printf("  Average chunk read time: %.3f ms\n", stats.avg_chunk_time_ms);
    printf("  Maximum chunk read time: %.3f ms\n", stats.max_chunk_time_ms);
    printf("  Thread utilization: %.1f%%\n", stats.thread_utilization * 100);
    
    // Cleanup
    free(buffer);
    parallel_io_close(pfile);
    parallel_io_cleanup(context);
    
    return 0;
}

/*********************************************************************
 * Example 2: Memory-Mapped I/O for Large Files
 *********************************************************************/
int example_memory_mapped_io() {
    printf("\n=== Example 2: Memory-Mapped I/O for Large Files ===\n");
    
    // Configure memory-mapped I/O
    ParallelIOConfig config = {
        .mode = IO_MODE_MEMORY_MAPPED,
        .num_threads = 1,  // Single thread for memory mapping
        .chunk_size = 64 * 1024 * 1024,  // 64MB mapping chunks
        .use_memory_mapping = 1,
        .buffer_size = 128 * 1024 * 1024  // 128MB buffer
    };
    
    ParallelIOContext* context = parallel_io_init(&config);
    
    const char* filename = "huge_simulation_data.bin";
    ParallelFile* pfile = parallel_io_open(context, filename, "r");
    
    if (pfile) {
        FileInfo info = parallel_io_get_file_info(pfile);
        printf("Memory-mapped file: %s\n", filename);
        printf("Size: %.2f GB\n", info.size / (1024.0 * 1024.0 * 1024.0));
        printf("Mapped chunks: %d\n", info.num_chunks);
        
        // Access file content through memory mapping
        clock_t start = clock();
        
        // Process file in chunks without copying to user buffer
        size_t total_processed = 0;
        for (int chunk = 0; chunk < info.num_chunks; chunk++) {
            void* mapped_data = parallel_io_map_chunk(pfile, chunk);
            if (mapped_data) {
                size_t chunk_size = parallel_io_get_chunk_size(pfile, chunk);
                
                // Process mapped data directly (no copy)
                // Example: validate checksum, extract metadata, etc.
                size_t processed = process_chunk_data(mapped_data, chunk_size);
                total_processed += processed;
                
                // Unmap when done
                parallel_io_unmap_chunk(pfile, chunk, mapped_data);
            }
        }
        
        clock_t end = clock();
        double process_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        
        printf("Total processed: %.2f MB\n", total_processed / (1024.0 * 1024.0));
        printf("Processing time: %.3f seconds\n", process_time);
        printf("Processing speed: %.2f MB/s\n", (total_processed / (1024.0 * 1024.0)) / process_time);
        
        // Memory mapping statistics
        MemoryMapStats mm_stats = parallel_io_get_memory_map_stats(context);
        printf("\nMemory Mapping Statistics:\n");
        printf("  Total mappings: %d\n", mm_stats.total_mappings);
        printf("  Active mappings: %d\n", mm_stats.active_mappings);
        printf("  Page faults: %d\n", mm_stats.page_faults);
        printf("  Memory saved: %.2f MB\n", mm_stats.memory_saved / (1024.0 * 1024.0));
        
        parallel_io_close(pfile);
    }
    
    parallel_io_cleanup(context);
    return 0;
}

/*********************************************************************
 * Example 3: Producer-Consumer Pattern for Processing Pipeline
 *********************************************************************/

// Data structure for processing pipeline
typedef struct {
    int chunk_id;
    void* data;
    size_t size;
    int processed;
    double processing_time_ms;
} ProcessingChunk;

// Producer thread function
void* producer_thread(void* arg) {
    ProducerConsumerContext* ctx = (ProducerConsumerContext*)arg;
    ParallelFile* pfile = (ParallelFile*)ctx->user_data;
    
    printf("Producer thread started\n");
    
    int chunk_id = 0;
    while (!parallel_io_eof(pfile)) {
        // Allocate chunk
        ProcessingChunk* chunk = malloc(sizeof(ProcessingChunk));
        chunk->chunk_id = chunk_id++;
        chunk->data = malloc(ctx->chunk_size);
        chunk->processed = 0;
        
        // Read data
        chunk->size = parallel_io_read_chunk(pfile, chunk->data, ctx->chunk_size);
        if (chunk->size == 0) {
            free(chunk->data);
            free(chunk);
            break;
        }
        
        // Add to queue
        producer_consumer_produce(ctx, chunk);
    }
    
    printf("Producer thread finished\n");
    return NULL;
}

// Consumer thread function
void* consumer_thread(void* arg) {
    ProducerConsumerContext* ctx = (ProducerConsumerContext*)arg;
    
    printf("Consumer thread started\n");
    
    int processed_count = 0;
    while (1) {
        // Get chunk from queue
        ProcessingChunk* chunk = (ProcessingChunk*)producer_consumer_consume(ctx);
        if (!chunk) break; // End of data
        
        clock_t start = clock();
        
        // Process chunk (example: extract polygons, validate data)
        int result = process_cad_data_chunk(chunk->data, chunk->size);
        chunk->processed = result;
        
        clock_t end = clock();
        chunk->processing_time_ms = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
        
        processed_count++;
        if (processed_count % 10 == 0) {
            printf("Processed %d chunks\n", processed_count);
        }
        
        // Free chunk memory
        free(chunk->data);
        free(chunk);
    }
    
    printf("Consumer thread finished\n");
    return NULL;
}

int example_producer_consumer_pipeline() {
    printf("\n=== Example 3: Producer-Consumer Pipeline ===\n");
    
    // Configure parallel I/O with producer-consumer pattern
    ParallelIOConfig config = {
        .mode = IO_MODE_PARALLEL_THREADS,
        .num_threads = 6,  // 2 producers, 4 consumers
        .chunk_size = 256 * 1024,  // 256KB chunks
        .use_memory_mapping = 0,
        .buffer_size = 64 * 1024 * 1024
    };
    
    ParallelIOContext* context = parallel_io_init(&config);
    
    const char* filename = "cad_data_stream.dxf";
    ParallelFile* pfile = parallel_io_open(context, filename, "r");
    
    if (pfile) {
        FileInfo info = parallel_io_get_file_info(pfile);
        printf("Processing file: %s (%.2f MB)\n", filename, info.size / (1024.0 * 1024.0));
        
        // Create producer-consumer context
        ProducerConsumerConfig pc_config = {
            .queue_size = 20,  // Maximum 20 chunks in queue
            .num_producers = 2,
            .num_consumers = 4,
            .chunk_size = config.chunk_size,
            .user_data = pfile
        };
        
        ProducerConsumerContext* pc_ctx = producer_consumer_init(&pc_config);
        
        clock_t start = clock();
        
        // Start producer-consumer pipeline
        producer_consumer_start(pc_ctx, producer_thread, consumer_thread);
        
        // Wait for completion
        producer_consumer_wait(pc_ctx);
        
        clock_t end = clock();
        double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        
        printf("Pipeline processing completed in %.3f seconds\n", total_time);
        printf("Processing rate: %.2f MB/s\n", (info.size / (1024.0 * 1024.0)) / total_time);
        
        // Get pipeline statistics
        ProducerConsumerStats pc_stats = producer_consumer_get_stats(pc_ctx);
        printf("\nPipeline Statistics:\n");
        printf("  Total chunks processed: %d\n", pc_stats.total_processed);
        printf("  Average queue depth: %.1f\n", pc_stats.avg_queue_depth);
        printf("  Maximum queue depth: %d\n", pc_stats.max_queue_depth);
        printf("  Producer wait time: %.1f%%\n", pc_stats.producer_wait_ratio * 100);
        printf("  Consumer wait time: %.1f%%\n", pc_stats.consumer_wait_ratio * 100);
        
        producer_consumer_cleanup(pc_ctx);
        parallel_io_close(pfile);
    }
    
    parallel_io_cleanup(context);
    return 0;
}

/*********************************************************************
 * Example 4: Parallel Format Validation
 *********************************************************************/
int example_parallel_validation() {
    printf("\n=== Example 4: Parallel Format Validation ===\n");
    
    // Configure parallel I/O for validation
    ParallelIOConfig config = {
        .mode = IO_MODE_PARALLEL_THREADS,
        .num_threads = 8,
        .chunk_size = 512 * 1024,  // 512KB chunks
        .use_memory_mapping = 1,
        .buffer_size = 128 * 1024 * 1024
    };
    
    ParallelIOContext* context = parallel_io_init(&config);
    
    const char* filename = "complex_design.oas";
    ParallelFile* pfile = parallel_io_open(context, filename, "r");
    
    if (pfile) {
        FileInfo info = parallel_io_get_file_info(pfile);
        printf("Validating file: %s (%.2f MB)\n", filename, info.size / (1024.0 * 1024.0));
        
        // Configure validation parameters
        ValidationConfig val_config = {
            .validation_level = VALIDATION_STANDARD,
            .enable_parallel_validation = 1,
            .max_parallel_chunks = 16,
            .error_recovery = RECOVERY_SKIP_INVALID,
            .max_errors = 100,
            .enable_checksum = 1,
            .enable_cross_reference = 1
        };
        
        clock_t start = clock();
        
        // Perform parallel validation
        ValidationResult result = parallel_validate_format(pfile, &val_config);
        
        clock_t end = clock();
        double validation_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        
        printf("Validation completed in %.3f seconds\n", validation_time);
        printf("Validation result: %s\n", result.is_valid ? "VALID" : "INVALID");
        printf("Total errors: %d\n", result.total_errors);
        printf("Critical errors: %d\n", result.critical_errors);
        printf("Warnings: %d\n", result.warnings);
        
        if (result.total_errors > 0) {
            printf("\nFirst 10 errors:\n");
            for (int i = 0; i < result.total_errors && i < 10; i++) {
                printf("  [%d] %s at offset %zu: %s\n", 
                       i + 1, 
                       get_error_type_string(result.errors[i].type),
                       result.errors[i].offset,
                       result.errors[i].message);
            }
        }
        
        // Validation statistics
        ValidationStats val_stats = parallel_validation_get_stats(context);
        printf("\nValidation Statistics:\n");
        printf("  Chunks validated: %d\n", val_stats.chunks_validated);
        printf("  Average chunk validation time: %.1f ms\n", val_stats.avg_chunk_validation_time_ms);
        printf("  Parallel efficiency: %.1f%%\n", val_stats.parallel_efficiency * 100);
        printf("  Memory usage: %.2f MB\n", val_stats.memory_usage / (1024.0 * 1024.0));
        
        // Generate validation report
        generate_validation_report(&result, "validation_report.html");
        printf("Validation report saved to: validation_report.html\n");
        
        free_validation_result(&result);
        parallel_io_close(pfile);
    }
    
    parallel_io_cleanup(context);
    return 0;
}

/*********************************************************************
 * Example 5: Parallel Processing with Memory Optimization
 *********************************************************************/
int example_parallel_with_memory_optimization() {
    printf("\n=== Example 5: Parallel Processing with Memory Optimization ===\n");
    
    // Configure memory optimization
    MemoryOptimizationConfig mem_config = {
        .strategy = MEMORY_STRATEGY_HYBRID,
        .max_memory_usage = 4 * 1024 * 1024 * 1024LL,  // 4GB limit
        .use_compression = 1,
        .compression_threshold = 4096,  // Compress objects > 4KB
        .enable_garbage_collection = 1,
        .gc_threshold = 0.85,  // GC when 85% memory used
        .enable_object_pooling = 1,
        .pool_block_size = 1024 * 1024  // 1MB pool blocks
    };
    
    // Initialize memory optimization
    MemoryOptimizer* optimizer = memory_optimizer_init(&mem_config);
    
    // Configure parallel I/O with memory optimization
    ParallelIOConfig io_config = {
        .mode = IO_MODE_PARALLEL_THREADS,
        .num_threads = 6,
        .chunk_size = 1024 * 1024,  // 1MB chunks
        .use_memory_mapping = 1,
        .buffer_size = 256 * 1024 * 1024,  // 256MB buffer
        .memory_optimizer = optimizer
    };
    
    ParallelIOContext* context = parallel_io_init(&io_config);
    
    // Process multiple files with memory optimization
    const char* files[] = {
        "design1.gds", "design2.oas", "design3.gbr",
        "design4.dxf", "design5.cif", "design6.kicad"
    };
    int num_files = sizeof(files) / sizeof(files[0]);
    
    printf("Processing %d files with memory optimization\n", num_files);
    
    clock_t start = clock();
    
    // Process files in parallel with memory optimization
    #pragma omp parallel for num_threads(io_config.num_threads)
    for (int i = 0; i < num_files; i++) {
        ParallelFile* pfile = parallel_io_open(context, files[i], "r");
        if (pfile) {
            FileInfo info = parallel_io_get_file_info(pfile);
            
            printf("[Thread %d] Processing: %s (%.2f MB)\n", 
                   omp_get_thread_num(), files[i], info.size / (1024.0 * 1024.0));
            
            // Allocate memory through optimizer
            void* buffer = memory_optimizer_alloc(optimizer, info.size);
            if (buffer) {
                // Read file with optimized memory
                size_t bytes_read = parallel_io_read(pfile, buffer, info.size);
                
                // Process data
                process_file_data(buffer, bytes_read, files[i]);
                
                // Free optimized memory
                memory_optimizer_free(optimizer, buffer, info.size);
            }
            
            parallel_io_close(pfile);
        }
    }
    
    clock_t end = clock();
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("\nParallel processing completed in %.3f seconds\n", total_time);
    
    // Memory optimization statistics
    MemoryOptimizationStats mem_stats = memory_optimizer_get_stats(optimizer);
    printf("\nMemory Optimization Statistics:\n");
    printf("  Total allocations: %zu\n", mem_stats.total_allocations);
    printf("  Total memory allocated: %.2f MB\n", mem_stats.total_allocated / (1024.0 * 1024.0));
    printf("  Peak memory usage: %.2f MB\n", mem_stats.peak_usage / (1024.0 * 1024.0));
    printf("  Current memory usage: %.2f MB\n", mem_stats.current_usage / (1024.0 * 1024.0));
    printf("  Compression savings: %.2f MB\n", mem_stats.compression_savings / (1024.0 * 1024.0));
    printf("  Garbage collections: %d\n", mem_stats.gc_count);
    printf("  Pool efficiency: %.1f%%\n", mem_stats.pool_efficiency * 100);
    
    // Cleanup
    memory_optimizer_cleanup(optimizer);
    parallel_io_cleanup(context);
    
    return 0;
}

/*********************************************************************
 * Example 6: Scalable I/O for Large-Scale Simulations
 *********************************************************************/
int example_scalable_large_scale() {
    printf("\n=== Example 6: Scalable I/O for Large-Scale Simulations ===\n");
    
    // Configure scalable I/O for very large datasets
    ScalableIOConfig scalable_config = {
        .base_config = {
            .mode = IO_MODE_ASYNC_IO,
            .num_threads = 16,
            .chunk_size = 4 * 1024 * 1024,  // 4MB chunks
            .use_memory_mapping = 1,
            .buffer_size = 1024 * 1024 * 1024  // 1GB buffer
        },
        .enable_distributed = 1,
        .num_nodes = 4,  // Simulate 4-node cluster
        .node_id = 0,    // This is node 0
        .enable_load_balancing = 1,
        .enable_fault_tolerance = 1,
        .checkpoint_interval = 60,  // Checkpoint every 60 seconds
        .enable_progress_tracking = 1
    };
    
    ScalableIOContext* scalable_ctx = scalable_io_init(&scalable_config);
    
    // Simulate large-scale electromagnetic simulation data
    const char* dataset_name = "em_simulation_100gb";
    size_t dataset_size = 100ULL * 1024 * 1024 * 1024;  // 100GB dataset
    
    printf("Processing large-scale dataset: %s (%.1f GB)\n", 
           dataset_name, dataset_size / (1024.0 * 1024.0 * 1024.0));
    
    // Create distributed file handle
    DistributedFile* dfile = scalable_io_create_distributed_file(scalable_ctx, dataset_name, dataset_size);
    
    if (dfile) {
        clock_t start = clock();
        
        // Process dataset in distributed manner
        size_t processed_size = 0;
        int chunk_id = 0;
        
        while (processed_size < dataset_size) {
            // Get next chunk assignment
            DistributedChunkAssignment assignment = scalable_io_get_chunk_assignment(dfile, chunk_id++);
            
            if (assignment.assigned_to_node == scalable_config.node_id) {
                // This node processes this chunk
                printf("Processing chunk %d (offset: %zu, size: %zu)\n", 
                       assignment.chunk_id, assignment.offset, assignment.size);
                
                // Map distributed chunk
                void* chunk_data = scalable_io_map_distributed_chunk(dfile, &assignment);
                
                if (chunk_data) {
                    // Process chunk data
                    process_distributed_chunk(chunk_data, assignment.size, assignment.chunk_id);
                    
                    // Report progress
                    processed_size += assignment.size;
                    scalable_io_report_progress(dfile, processed_size, dataset_size);
                    
                    // Unmap chunk
                    scalable_io_unmap_distributed_chunk(dfile, &assignment, chunk_data);
                }
            }
            
            // Checkpoint periodically
            if (chunk_id % 100 == 0) {
                scalable_io_checkpoint(dfile);
            }
        }
        
        clock_t end = clock();
        double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        
        printf("\nLarge-scale processing completed in %.3f seconds\n", total_time);
        printf("Processing rate: %.2f GB/s\n", (dataset_size / (1024.0 * 1024.0 * 1024.0)) / total_time);
        
        // Final checkpoint
        scalable_io_checkpoint(dfile);
        
        // Get scalable I/O statistics
        ScalableIOStats scalable_stats = scalable_io_get_stats(scalable_ctx);
        printf("\nScalable I/O Statistics:\n");
        printf("  Total chunks processed: %d\n", scalable_stats.total_chunks);
        printf("  Load balance efficiency: %.1f%%\n", scalable_stats.load_balance_efficiency * 100);
        printf("  Fault tolerance events: %d\n", scalable_stats.fault_tolerance_events);
        printf("  Checkpoint count: %d\n", scalable_stats.checkpoint_count);
        printf("  Recovery time: %.1f seconds\n", scalable_stats.recovery_time);
        printf("  Network overhead: %.1f%%\n", scalable_stats.network_overhead * 100);
        
        scalable_io_close_distributed_file(dfile);
    }
    
    scalable_io_cleanup(scalable_ctx);
    
    return 0;
}

/*********************************************************************
 * Helper Functions for Examples
 *********************************************************************/

size_t process_chunk_data(void* data, size_t size) {
    // Simulate processing: count valid records, extract metadata
    size_t record_count = 0;
    size_t valid_records = 0;
    
    // Example: process CAD records in chunk
    unsigned char* bytes = (unsigned char*)data;
    for (size_t i = 0; i < size - 4; i++) {
        // Look for record markers
        if (bytes[i] == 0x00 && bytes[i+1] == 0x06) {  // GDSII record marker
            record_count++;
            if (validate_gdsii_record(bytes + i, size - i)) {
                valid_records++;
            }
        }
    }
    
    return valid_records;
}

int process_cad_data_chunk(void* data, size_t size) {
    // Simulate CAD data processing
    return process_chunk_data(data, size);
}

void process_file_data(void* buffer, size_t size, const char* filename) {
    printf("Processing file: %s (%.2f MB)\n", filename, size / (1024.0 * 1024.0));
    size_t processed = process_chunk_data(buffer, size);
    printf("  Processed %zu valid records\n", processed);
}

int validate_gdsii_record(unsigned char* data, size_t remaining_size) {
    // Simplified GDSII record validation
    if (remaining_size < 4) return 0;
    
    uint16_t record_length = (data[2] << 8) | data[3];
    if (record_length > remaining_size) return 0;
    
    return 1;
}

/*********************************************************************
 * Main function to run all examples
 *********************************************************************/
int main() {
    printf("Parallel I/O Processing Usage Examples\n");
    printf("=====================================\n\n");
    
    int result = 0;
    
    // Run all examples
    result |= example_basic_parallel_reading();
    result |= example_memory_mapped_io();
    result |= example_producer_consumer_pipeline();
    result |= example_parallel_validation();
    result |= example_parallel_with_memory_optimization();
    result |= example_scalable_large_scale();
    
    if (result == 0) {
        printf("\nAll parallel I/O examples completed successfully!\n");
    } else {
        printf("\nSome parallel I/O examples failed with errors.\n");
    }
    
    return result;
}