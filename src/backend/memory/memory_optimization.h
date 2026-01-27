#ifndef MEMORY_OPTIMIZATION_H
#define MEMORY_OPTIMIZATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../io/advanced_file_formats.h"
#include "../../io/parallel_io.h"

#define MEMORY_POOL_BLOCK_SIZE (1024 * 1024)
#define MEMORY_POOL_ALIGNMENT 64
#define MEMORY_CACHE_LINE_SIZE 64
#define MEMORY_PAGE_SIZE 4096
#define MEMORY_LARGE_OBJECT_THRESHOLD (64 * 1024)
#define MEMORY_COMPRESSION_THRESHOLD (1024 * 1024)
#define MEMORY_MAX_POOLS 64
#define MEMORY_GC_THRESHOLD (512 * 1024 * 1024)

typedef enum {
    MEMORY_STRATEGY_POOL,
    MEMORY_STRATEGY_ARENA,
    MEMORY_STRATEGY_SLAB,
    MEMORY_STRATEGY_BUDDY,
    MEMORY_STRATEGY_COMPRESSION,
    MEMORY_STRATEGY_STREAMING,
    MEMORY_STRATEGY_HYBRID
} MemoryStrategy;

typedef enum {
    OBJECT_TYPE_GEOMETRY,
    OBJECT_TYPE_CELL,
    OBJECT_TYPE_LAYER,
    OBJECT_TYPE_PROPERTY,
    OBJECT_TYPE_STRING,
    OBJECT_TYPE_ARRAY,
    OBJECT_TYPE_MATRIX,
    OBJECT_TYPE_TREE,
    OBJECT_TYPE_GRAPH,
    OBJECT_TYPE_CACHE,
    OBJECT_TYPE_TEMPORARY,
    OBJECT_TYPE_PERSISTENT
} ObjectType;

typedef struct {
    void* start;
    void* current;
    void* end;
    size_t size;
    size_t used;
    size_t alignment;
    uint32_t allocation_count;
    uint32_t free_count;
    double fragmentation_ratio;
    bool is_compressed;
    bool is_locked;
    pthread_mutex_t mutex;
} MemoryPool;

typedef struct {
    MemoryPool* pools;
    uint32_t num_pools;
    uint32_t active_pools;
    size_t total_size;
    size_t total_used;
    size_t total_allocated;
    size_t total_freed;
    uint32_t allocation_count;
    uint32_t free_count;
    double average_fragmentation;
    MemoryStrategy strategy;
    bool enable_compression;
    bool enable_gc;
    size_t gc_threshold;
    size_t max_memory_limit;
    char* statistics_file;
} MemoryManager;

typedef struct {
    void* data;
    size_t original_size;
    size_t compressed_size;
    double compression_ratio;
    ObjectType object_type;
    uint32_t reference_count;
    bool is_compressed;
    bool is_pinned;
    time_t creation_time;
    time_t last_access_time;
    uint32_t access_count;
    char* description;
} MemoryObject;

typedef struct {
    MemoryObject** objects;
    uint32_t num_objects;
    uint32_t max_objects;
    size_t total_size;
    size_t compressed_size;
    double average_compression_ratio;
    uint32_t hits;
    uint32_t misses;
    uint32_t evictions;
    double hit_ratio;
    size_t max_size;
    bool enable_compression;
    bool enable_eviction;
    uint32_t eviction_policy;
    pthread_mutex_t mutex;
} MemoryCache;

typedef struct {
    void* base_address;
    size_t total_size;
    size_t used_size;
    size_t chunk_size;
    uint32_t num_chunks;
    uint32_t* free_bitmap;
    uint32_t* allocation_sizes;
    bool* is_allocated;
    pthread_mutex_t mutex;
} MemoryArena;

typedef struct {
    MemoryManager* manager;
    MemoryCache* cache;
    MemoryArena* arena;
    uint32_t num_pools;
    MemoryStrategy strategy;
    bool enable_statistics;
    bool enable_profiling;
    size_t peak_memory_usage;
    size_t current_memory_usage;
    double average_allocation_time;
    double average_deallocation_time;
    uint64_t total_allocations;
    uint64_t total_deallocations;
    size_t memory_limit;
    bool enable_gc;
    size_t gc_threshold;
} MemoryOptimizationContext;

typedef struct {
    size_t original_size;
    size_t optimized_size;
    size_t savings_bytes;
    double savings_percentage;
    double compression_ratio;
    uint32_t objects_compressed;
    uint32_t objects_decompressed;
    double average_compression_time;
    double average_decompression_time;
    size_t peak_memory_usage;
    size_t current_memory_usage;
    double fragmentation_reduction;
    uint32_t gc_cycles;
    size_t gc_freed_bytes;
} MemoryOptimizationMetrics;

typedef struct {
    size_t total_memory;
    size_t used_memory;
    size_t free_memory;
    size_t cached_memory;
    size_t compressed_memory;
    size_t fragmented_memory;
    double fragmentation_ratio;
    double compression_ratio;
    uint32_t active_objects;
    uint32_t cached_objects;
    uint32_t compressed_objects;
    double average_object_size;
    double memory_efficiency;
    size_t peak_usage;
    size_t leak_detection_size;
    uint32_t potential_leaks;
} MemoryStatistics;

MemoryOptimizationContext* create_memory_optimization_context(MemoryStrategy strategy, size_t initial_size);
void destroy_memory_optimization_context(MemoryOptimizationContext* context);

void* optimized_malloc(MemoryOptimizationContext* context, size_t size, ObjectType type, const char* description);
void optimized_free(MemoryOptimizationContext* context, void* ptr);
void* optimized_realloc(MemoryOptimizationContext* context, void* ptr, size_t new_size);

MemoryObject* create_memory_object(void* data, size_t size, ObjectType type, const char* description);
void destroy_memory_object(MemoryObject* object);

int compress_memory_object(MemoryObject* object);
int decompress_memory_object(MemoryObject* object);
int pin_memory_object(MemoryObject* object);
int unpin_memory_object(MemoryObject* object);

MemoryPool* create_memory_pool(size_t size, size_t alignment);
void destroy_memory_pool(MemoryPool* pool);
void* pool_allocate(MemoryPool* pool, size_t size);
void pool_free(MemoryPool* pool, void* ptr);
int pool_defragment(MemoryPool* pool);

MemoryCache* create_memory_cache(size_t max_size, bool enable_compression);
void destroy_memory_cache(MemoryCache* cache);
int cache_insert(MemoryCache* cache, MemoryObject* object);
MemoryObject* cache_lookup(MemoryCache* cache, const char* key);
int cache_evict(MemoryCache* cache, uint32_t num_objects);
int cache_compress(MemoryCache* cache);
int cache_decompress(MemoryCache* cache);

MemoryArena* create_memory_arena(size_t total_size, size_t chunk_size);
void destroy_memory_arena(MemoryArena* arena);
void* arena_allocate(MemoryArena* arena, size_t size);
void arena_free(MemoryArena* arena, void* ptr);
int arena_defragment(MemoryArena* arena);
size_t arena_get_free_space(MemoryArena* arena);

int optimize_memory_layout(MemoryOptimizationContext* context);
int compress_memory_data(MemoryOptimizationContext* context);
int defragment_memory(MemoryOptimizationContext* context);
int perform_garbage_collection(MemoryOptimizationContext* context);

MemoryStatistics* get_memory_statistics(MemoryOptimizationContext* context);
void destroy_memory_statistics(MemoryStatistics* stats);
int export_memory_statistics(MemoryOptimizationContext* context, const char* filename);
int import_memory_statistics(MemoryOptimizationContext* context, const char* filename);

int benchmark_memory_performance(MemoryOptimizationContext* context, const char* test_file, MemoryOptimizationMetrics* metrics);
int compare_memory_strategies(MemoryOptimizationContext* context, MemoryStrategy* strategies, uint32_t num_strategies, MemoryOptimizationMetrics** results);

int optimize_file_format_memory(AdvancedFileFormat* format, MemoryOptimizationContext* context);
int optimize_parallel_io_memory(ParallelIOProcessor* processor, MemoryOptimizationContext* context);

int enable_memory_profiling(MemoryOptimizationContext* context, bool enable);
int enable_memory_debugging(MemoryOptimizationContext* context, bool enable);
int detect_memory_leaks(MemoryOptimizationContext* context);
int analyze_memory_fragmentation(MemoryOptimizationContext* context, double* fragmentation_ratio);

int set_memory_limit(MemoryOptimizationContext* context, size_t limit);
int get_memory_usage_breakdown(MemoryOptimizationContext* context, size_t* pools, size_t* cache, size_t* arena, size_t* overhead);
int optimize_for_file_format(MemoryOptimizationContext* context, FileFormatType format);

#endif