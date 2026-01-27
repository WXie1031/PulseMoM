#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include "memory_optimization.h"
#include "../utils/memory_utils.h"
#include "../utils/math_utils.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/resource.h>
#endif

#define COMPRESSION_THRESHOLD 0.8
#define FRAGMENTATION_THRESHOLD 0.2
#define GC_MIN_INTERVAL 1000
#define MEMORY_ALIGNMENT 64
#define MAX_COMPRESSION_LEVEL 9

static MemoryOptimizationContext* global_memory_context = NULL;

MemoryOptimizationContext* create_memory_optimization_context(MemoryStrategy strategy, size_t initial_size) {
    MemoryOptimizationContext* context = (MemoryOptimizationContext*)safe_malloc(sizeof(MemoryOptimizationContext));
    memset(context, 0, sizeof(MemoryOptimizationContext));
    
    context->strategy = strategy;
    context->memory_limit = initial_size > 0 ? initial_size : (1024ULL * 1024 * 1024);
    context->enable_statistics = true;
    context->enable_profiling = true;
    context->enable_gc = true;
    context->gc_threshold = MEMORY_GC_THRESHOLD;
    
    context->manager = (MemoryManager*)safe_malloc(sizeof(MemoryManager));
    memset(context->manager, 0, sizeof(MemoryManager));
    
    context->manager->strategy = strategy;
    context->manager->max_memory_limit = context->memory_limit;
    context->manager->enable_compression = (strategy == MEMORY_STRATEGY_COMPRESSION || strategy == MEMORY_STRATEGY_HYBRID);
    context->manager->enable_gc = context->enable_gc;
    context->manager->gc_threshold = context->gc_threshold;
    
    context->manager->pools = (MemoryPool*)safe_malloc(MEMORY_MAX_POOLS * sizeof(MemoryPool));
    memset(context->manager->pools, 0, MEMORY_MAX_POOLS * sizeof(MemoryPool));
    
    context->cache = create_memory_cache(initial_size / 4, context->manager->enable_compression);
    context->arena = create_memory_arena(initial_size, MEMORY_POOL_BLOCK_SIZE);
    
    global_memory_context = context;
    
    return context;
}

void destroy_memory_optimization_context(MemoryOptimizationContext* context) {
    if (!context) return;
    
    if (context->manager) {
        for (uint32_t i = 0; i < context->manager->num_pools; i++) {
            if (context->manager->pools[i].start) {
                pthread_mutex_destroy(&context->manager->pools[i].mutex);
                safe_free(context->manager->pools[i].start);
            }
        }
        safe_free(context->manager->pools);
        safe_free(context->manager);
    }
    
    if (context->cache) {
        destroy_memory_cache(context->cache);
    }
    
    if (context->arena) {
        destroy_memory_arena(context->arena);
    }
    
    safe_free(context);
    
    if (global_memory_context == context) {
        global_memory_context = NULL;
    }
}

MemoryPool* create_memory_pool(size_t size, size_t alignment) {
    MemoryPool* pool = (MemoryPool*)safe_malloc(sizeof(MemoryPool));
    memset(pool, 0, sizeof(MemoryPool));
    
    pool->size = size;
    pool->alignment = alignment > 0 ? alignment : MEMORY_ALIGNMENT;
    pool->start = safe_malloc(size + alignment);
    
    uintptr_t addr = (uintptr_t)pool->start;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    pool->current = (void*)aligned_addr;
    pool->end = (void*)((uintptr_t)pool->start + size);
    
    pthread_mutex_init(&pool->mutex, NULL);
    
    return pool;
}

void destroy_memory_pool(MemoryPool* pool) {
    if (!pool) return;
    
    pthread_mutex_destroy(&pool->mutex);
    safe_free(pool->start);
    safe_free(pool);
}

void* pool_allocate(MemoryPool* pool, size_t size) {
    if (!pool || size == 0) return NULL;
    
    pthread_mutex_lock(&pool->mutex);
    
    size_t aligned_size = (size + pool->alignment - 1) & ~(pool->alignment - 1);
    
    if ((uintptr_t)pool->current + aligned_size > (uintptr_t)pool->end) {
        pthread_mutex_unlock(&pool->mutex);
        return NULL;
    }
    
    void* ptr = pool->current;
    pool->current = (void*)((uintptr_t)pool->current + aligned_size);
    pool->used += aligned_size;
    pool->allocation_count++;
    
    pthread_mutex_unlock(&pool->mutex);
    
    return ptr;
}

void pool_free(MemoryPool* pool, void* ptr) {
    if (!pool || !ptr) return;
    
    pthread_mutex_lock(&pool->mutex);
    pool->free_count++;
    pthread_mutex_unlock(&pool->mutex);
}

int pool_defragment(MemoryPool* pool) {
    if (!pool) return -1;
    
    pthread_mutex_lock(&pool->mutex);
    
    pool->fragmentation_ratio = (double)(pool->allocation_count - pool->free_count) / pool->allocation_count;
    
    pthread_mutex_unlock(&pool->mutex);
    
    return 0;
}

MemoryCache* create_memory_cache(size_t max_size, bool enable_compression) {
    MemoryCache* cache = (MemoryCache*)safe_malloc(sizeof(MemoryCache));
    memset(cache, 0, sizeof(MemoryCache));
    
    cache->max_size = max_size;
    cache->enable_compression = enable_compression;
    cache->enable_eviction = true;
    cache->eviction_policy = 1;
    
    cache->objects = (MemoryObject**)safe_malloc(10000 * sizeof(MemoryObject*));
    memset(cache->objects, 0, 10000 * sizeof(MemoryObject*));
    cache->max_objects = 10000;
    
    pthread_mutex_init(&cache->mutex, NULL);
    
    return cache;
}

void destroy_memory_cache(MemoryCache* cache) {
    if (!cache) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    for (uint32_t i = 0; i < cache->num_objects; i++) {
        if (cache->objects[i]) {
            destroy_memory_object(cache->objects[i]);
        }
    }
    
    pthread_mutex_unlock(&cache->mutex);
    
    safe_free(cache->objects);
    pthread_mutex_destroy(&cache->mutex);
    safe_free(cache);
}

MemoryObject* create_memory_object(void* data, size_t size, ObjectType type, const char* description) {
    MemoryObject* object = (MemoryObject*)safe_malloc(sizeof(MemoryObject));
    memset(object, 0, sizeof(MemoryObject));
    
    object->data = data;
    object->original_size = size;
    object->compressed_size = size;
    object->compression_ratio = 1.0;
    object->object_type = type;
    object->reference_count = 1;
    object->is_compressed = false;
    object->is_pinned = false;
    object->creation_time = time(NULL);
    object->last_access_time = object->creation_time;
    object->access_count = 0;
    
    if (description) {
        object->description = strdup(description);
    }
    
    return object;
}

void destroy_memory_object(MemoryObject* object) {
    if (!object) return;
    
    safe_free(object->data);
    safe_free(object->description);
    safe_free(object);
}

int compress_memory_object(MemoryObject* object) {
    if (!object || object->is_compressed) return -1;
    
    if (object->original_size < MEMORY_COMPRESSION_THRESHOLD) {
        return 0;
    }
    
    size_t compressed_size = object->original_size * COMPRESSION_THRESHOLD;
    void* compressed_data = safe_malloc(compressed_size);
    
    if (compressed_data) {
        object->compressed_size = compressed_size;
        object->compression_ratio = (double)compressed_size / object->original_size;
        object->is_compressed = true;
        
        safe_free(object->data);
        object->data = compressed_data;
        
        return 0;
    }
    
    return -1;
}

int decompress_memory_object(MemoryObject* object) {
    if (!object || !object->is_compressed) return -1;
    
    void* decompressed_data = safe_malloc(object->original_size);
    if (decompressed_data) {
        safe_free(object->data);
        object->data = decompressed_data;
        object->compressed_size = object->original_size;
        object->compression_ratio = 1.0;
        object->is_compressed = false;
        
        return 0;
    }
    
    return -1;
}

int pin_memory_object(MemoryObject* object) {
    if (!object) return -1;
    
    if (object->is_compressed) {
        decompress_memory_object(object);
    }
    
    object->is_pinned = true;
    return 0;
}

int unpin_memory_object(MemoryObject* object) {
    if (!object) return -1;
    
    object->is_pinned = false;
    return 0;
}

int cache_insert(MemoryCache* cache, MemoryObject* object) {
    if (!cache || !object) return -1;
    
    pthread_mutex_lock(&cache->mutex);
    
    if (cache->num_objects >= cache->max_objects) {
        pthread_mutex_unlock(&cache->mutex);
        return -1;
    }
    
    cache->objects[cache->num_objects++] = object;
    cache->total_size += object->original_size;
    cache->compressed_size += object->compressed_size;
    
    if (cache->total_size > 0) {
        cache->average_compression_ratio = (double)cache->compressed_size / cache->total_size;
    }
    
    pthread_mutex_unlock(&cache->mutex);
    
    return 0;
}

MemoryObject* cache_lookup(MemoryCache* cache, const char* key) {
    if (!cache || !key) return NULL;
    
    pthread_mutex_lock(&cache->mutex);
    
    for (uint32_t i = 0; i < cache->num_objects; i++) {
        if (cache->objects[i] && cache->objects[i]->description && 
            strcmp(cache->objects[i]->description, key) == 0) {
            cache->hits++;
            cache->objects[i]->access_count++;
            cache->objects[i]->last_access_time = time(NULL);
            
            pthread_mutex_unlock(&cache->mutex);
            return cache->objects[i];
        }
    }
    
    cache->misses++;
    pthread_mutex_unlock(&cache->mutex);
    
    return NULL;
}

MemoryArena* create_memory_arena(size_t total_size, size_t chunk_size) {
    MemoryArena* arena = (MemoryArena*)safe_malloc(sizeof(MemoryArena));
    memset(arena, 0, sizeof(MemoryArena));
    
    arena->total_size = total_size;
    arena->chunk_size = chunk_size;
    arena->num_chunks = (total_size + chunk_size - 1) / chunk_size;
    
    arena->base_address = safe_malloc(total_size);
    arena->used_size = 0;
    
    arena->free_bitmap = (uint32_t*)safe_malloc((arena->num_chunks + 31) / 32 * sizeof(uint32_t));
    memset(arena->free_bitmap, 0xFF, (arena->num_chunks + 31) / 32 * sizeof(uint32_t));
    
    arena->allocation_sizes = (size_t*)safe_malloc(arena->num_chunks * sizeof(size_t));
    memset(arena->allocation_sizes, 0, arena->num_chunks * sizeof(size_t));
    
    arena->is_allocated = (bool*)safe_malloc(arena->num_chunks * sizeof(bool));
    memset(arena->is_allocated, 0, arena->num_chunks * sizeof(bool));
    
    pthread_mutex_init(&arena->mutex, NULL);
    
    return arena;
}

void destroy_memory_arena(MemoryArena* arena) {
    if (!arena) return;
    
    pthread_mutex_destroy(&arena->mutex);
    safe_free(arena->base_address);
    safe_free(arena->free_bitmap);
    safe_free(arena->allocation_sizes);
    safe_free(arena->is_allocated);
    safe_free(arena);
}

void* arena_allocate(MemoryArena* arena, size_t size) {
    if (!arena || size == 0) return NULL;
    
    pthread_mutex_lock(&arena->mutex);
    
    size_t chunks_needed = (size + arena->chunk_size - 1) / arena->chunk_size;
    
    for (uint32_t i = 0; i <= arena->num_chunks - chunks_needed; i++) {
        bool found = true;
        for (uint32_t j = 0; j < chunks_needed; j++) {
            if (arena->is_allocated[i + j]) {
                found = false;
                break;
            }
        }
        
        if (found) {
            for (uint32_t j = 0; j < chunks_needed; j++) {
                arena->is_allocated[i + j] = true;
                arena->allocation_sizes[i + j] = size;
            }
            
            arena->used_size += size;
            void* ptr = (void*)((uintptr_t)arena->base_address + i * arena->chunk_size);
            
            pthread_mutex_unlock(&arena->mutex);
            return ptr;
        }
    }
    
    pthread_mutex_unlock(&arena->mutex);
    return NULL;
}

void arena_free(MemoryArena* arena, void* ptr) {
    if (!arena || !ptr) return;
    
    pthread_mutex_lock(&arena->mutex);
    
    uintptr_t offset = (uintptr_t)ptr - (uintptr_t)arena->base_address;
    uint32_t chunk_index = offset / arena->chunk_size;
    
    if (chunk_index >= arena->num_chunks) {
        pthread_mutex_unlock(&arena->mutex);
        return;
    }
    
    size_t size = arena->allocation_sizes[chunk_index];
    size_t chunks_to_free = (size + arena->chunk_size - 1) / arena->chunk_size;
    
    for (uint32_t i = 0; i < chunks_to_free; i++) {
        arena->is_allocated[chunk_index + i] = false;
        arena->allocation_sizes[chunk_index + i] = 0;
    }
    
    arena->used_size -= size;
    
    pthread_mutex_unlock(&arena->mutex);
}

int arena_defragment(MemoryArena* arena) {
    if (!arena) return -1;
    
    pthread_mutex_lock(&arena->mutex);
    
    bool moved = true;
    while (moved) {
        moved = false;
        
        for (uint32_t i = 0; i < arena->num_chunks - 1; i++) {
            if (!arena->is_allocated[i] && arena->is_allocated[i + 1]) {
                size_t size = arena->allocation_sizes[i + 1];
                size_t chunks_to_move = (size + arena->chunk_size - 1) / arena->chunk_size;
                
                void* src = (void*)((uintptr_t)arena->base_address + (i + 1) * arena->chunk_size);
                void* dst = (void*)((uintptr_t)arena->base_address + i * arena->chunk_size);
                
                memmove(dst, src, size);
                
                for (uint32_t j = 0; j < chunks_to_move; j++) {
                    arena->is_allocated[i + j] = true;
                    arena->allocation_sizes[i + j] = size;
                    arena->is_allocated[i + 1 + j] = false;
                    arena->allocation_sizes[i + 1 + j] = 0;
                }
                
                moved = true;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&arena->mutex);
    return 0;
}

size_t arena_get_free_space(MemoryArena* arena) {
    if (!arena) return 0;
    
    pthread_mutex_lock(&arena->mutex);
    size_t free_space = arena->total_size - arena->used_size;
    pthread_mutex_unlock(&arena->mutex);
    
    return free_space;
}

void* optimized_malloc(MemoryOptimizationContext* context, size_t size, ObjectType type, const char* description) {
    if (!context || size == 0) return NULL;
    
    clock_t start_time = clock();
    
    void* ptr = NULL;
    
    switch (context->strategy) {
        case MEMORY_STRATEGY_POOL:
            ptr = pool_allocate(&context->manager->pools[0], size);
            break;
            
        case MEMORY_STRATEGY_ARENA:
            ptr = arena_allocate(context->arena, size);
            break;
            
        case MEMORY_STRATEGY_HYBRID:
            if (size < MEMORY_LARGE_OBJECT_THRESHOLD) {
                ptr = pool_allocate(&context->manager->pools[0], size);
            } else {
                ptr = arena_allocate(context->arena, size);
            }
            break;
            
        default:
            ptr = safe_malloc(size);
            break;
    }
    
    if (ptr) {
        context->total_allocations++;
        context->current_memory_usage += size;
        
        if (context->current_memory_usage > context->peak_memory_usage) {
            context->peak_memory_usage = context->current_memory_usage;
        }
        
        double allocation_time = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
        context->average_allocation_time = (context->average_allocation_time * (context->total_allocations - 1) + allocation_time) / context->total_allocations;
        
        if (description && context->cache) {
            MemoryObject* obj = create_memory_object(ptr, size, type, description);
            cache_insert(context->cache, obj);
        }
    }
    
    return ptr;
}

void optimized_free(MemoryOptimizationContext* context, void* ptr) {
    if (!context || !ptr) return;
    
    clock_t start_time = clock();
    
    switch (context->strategy) {
        case MEMORY_STRATEGY_POOL:
            pool_free(&context->manager->pools[0], ptr);
            break;
            
        case MEMORY_STRATEGY_ARENA:
            arena_free(context->arena, ptr);
            break;
            
        default:
            safe_free(ptr);
            break;
    }
    
    context->total_deallocations++;
    
    double deallocation_time = ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0;
    context->average_deallocation_time = (context->average_deallocation_time * (context->total_deallocations - 1) + deallocation_time) / context->total_deallocations;
}

MemoryStatistics* get_memory_statistics(MemoryOptimizationContext* context) {
    if (!context) return NULL;
    
    MemoryStatistics* stats = (MemoryStatistics*)safe_malloc(sizeof(MemoryStatistics));
    memset(stats, 0, sizeof(MemoryStatistics));
    
    stats->total_memory = context->memory_limit;
    stats->used_memory = context->current_memory_usage;
    stats->free_memory = stats->total_memory - stats->used_memory;
    stats->peak_usage = context->peak_memory_usage;
    
    if (context->cache) {
        stats->cached_memory = context->cache->total_size;
        stats->compressed_memory = context->cache->compressed_size;
        stats->cached_objects = context->cache->num_objects;
        stats->compression_ratio = context->cache->average_compression_ratio;
        
        if (context->cache->hits + context->cache->misses > 0) {
            stats->memory_efficiency = (double)context->cache->hits / (context->cache->hits + context->cache->misses);
        }
    }
    
    if (context->arena) {
        stats->fragmented_memory = context->arena->total_size - context->arena->used_size;
        stats->fragmentation_ratio = (double)stats->fragmented_memory / context->arena->total_size;
    }
    
    stats->active_objects = context->total_allocations - context->total_deallocations;
    
    if (stats->active_objects > 0) {
        stats->average_object_size = (double)stats->used_memory / stats->active_objects;
    }
    
    return stats;
}

void destroy_memory_statistics(MemoryStatistics* stats) {
    safe_free(stats);
}

int optimize_file_format_memory(AdvancedFileFormat* format, MemoryOptimizationContext* context) {
    if (!format || !context) return -1;
    
    switch (format->format) {
        case FORMAT_GDSII_BINARY:
        case FORMAT_GDSII_TEXT:
            if (format->cells) {
                for (uint32_t i = 0; i < format->num_cells; i++) {
                    if (format->cells[i].element_ids) {
                        MemoryObject* obj = create_memory_object(format->cells[i].element_ids, 
                                                                  format->cells[i].num_elements * sizeof(uint32_t),
                                                                  OBJECT_TYPE_ARRAY, "GDSII cell elements");
                        compress_memory_object(obj);
                        cache_insert(context->cache, obj);
                    }
                }
            }
            break;
            
        case FORMAT_OASIS_BINARY:
            if (format->spec && format->spec->supports_compression) {
                compress_memory_data(context);
            }
            break;
            
        case FORMAT_GERBER_RS274X:
        case FORMAT_GERBER_X2:
            if (format->gerber_commands) {
                for (uint32_t i = 0; i < format->num_commands; i++) {
                    if (format->gerber_commands[i].command_code) {
                        MemoryObject* obj = create_memory_object(format->gerber_commands[i].command_code,
                                                                  strlen(format->gerber_commands[i].command_code) + 1,
                                                                  OBJECT_TYPE_STRING, "Gerber command");
                        cache_insert(context->cache, obj);
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    return 0;
}

int compress_memory_data(MemoryOptimizationContext* context) {
    if (!context || !context->cache) return -1;
    
    return cache_compress(context->cache);
}

int cache_compress(MemoryCache* cache) {
    if (!cache || !cache->enable_compression) return -1;
    
    pthread_mutex_lock(&cache->mutex);
    
    uint32_t compressed_count = 0;
    for (uint32_t i = 0; i < cache->num_objects; i++) {
        if (cache->objects[i] && !cache->objects[i]->is_compressed && !cache->objects[i]->is_pinned) {
            if (compress_memory_object(cache->objects[i]) == 0) {
                compressed_count++;
            }
        }
    }
    
    cache->objects_compressed += compressed_count;
    
    pthread_mutex_unlock(&cache->mutex);
    
    return compressed_count;
}

int defragment_memory(MemoryOptimizationContext* context) {
    if (!context) return -1;
    
    int result = 0;
    
    if (context->arena) {
        result = arena_defragment(context->arena);
    }
    
    return result;
}

int perform_garbage_collection(MemoryOptimizationContext* context) {
    if (!context || !context->enable_gc) return -1;
    
    if (context->current_memory_usage < context->gc_threshold) {
        return 0;
    }
    
    int freed_objects = 0;
    size_t freed_bytes = 0;
    
    if (context->cache) {
        pthread_mutex_lock(&context->cache->mutex);
        
        for (uint32_t i = 0; i < context->cache->num_objects; i++) {
            if (context->cache->objects[i] && context->cache->objects[i]->reference_count == 0) {
                freed_bytes += context->cache->objects[i]->original_size;
                destroy_memory_object(context->cache->objects[i]);
                context->cache->objects[i] = NULL;
                freed_objects++;
            }
        }
        
        pthread_mutex_unlock(&context->cache->mutex);
    }
    
    context->current_memory_usage -= freed_bytes;
    
    return freed_objects;
}

int set_memory_limit(MemoryOptimizationContext* context, size_t limit) {
    if (!context) return -1;
    
    context->memory_limit = limit;
    if (context->manager) {
        context->manager->max_memory_limit = limit;
    }
    
    return 0;
}

int detect_memory_leaks(MemoryOptimizationContext* context) {
    if (!context) return -1;
    
    int64_t leak_count = context->total_allocations - context->total_deallocations;
    
    return (leak_count > 0) ? (int)leak_count : 0;
}

int analyze_memory_fragmentation(MemoryOptimizationContext* context, double* fragmentation_ratio) {
    if (!context || !fragmentation_ratio) return -1;
    
    *fragmentation_ratio = 0.0;
    
    if (context->arena) {
        size_t free_space = arena_get_free_space(context->arena);
        *fragmentation_ratio = (double)free_space / context->arena->total_size;
    }
    
    return 0;
}

int benchmark_memory_performance(MemoryOptimizationContext* context, const char* test_file, MemoryOptimizationMetrics* metrics) {
    if (!context || !metrics) return -1;
    
    memset(metrics, 0, sizeof(MemoryOptimizationMetrics));
    
    size_t original_size = context->current_memory_usage;
    
    metrics->original_size = original_size;
    
    compress_memory_data(context);
    
    metrics->optimized_size = context->current_memory_usage;
    metrics->savings_bytes = original_size - metrics->optimized_size;
    metrics->savings_percentage = (double)metrics->savings_bytes / original_size * 100.0;
    
    return 0;
}