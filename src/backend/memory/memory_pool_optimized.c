/*********************************************************************
 * 优化版内存池管理和数据传输实现
 * 包含统一内存、异步传输和内存预分配策略
 *********************************************************************/

#include "gpu_parallelization_optimized.h"
#include <cuda_runtime.h>
#include <cuda.h>
#include <omp.h>
#include <math.h>
#include <string.h>

// 内存块状态枚举
typedef enum {
    MEMORY_FREE,
    MEMORY_ALLOCATED,
    MEMORY_LOCKED,
    MEMORY_PREFETCHED
} MemoryBlockStatus;

// 优化的内存块结构
typedef struct {
    void* ptr;
    size_t size;
    MemoryBlockStatus status;
    int gpu_id;
    cudaStream_t stream;
    double last_access_time;
    int access_count;
    int prefetch_distance;
    int usage_pattern;
} OptimizedMemoryBlock;

// 内存池统计信息
typedef struct {
    size_t total_allocated;
    size_t total_used;
    size_t total_free;
    size_t peak_usage;
    int num_allocations;
    int num_deallocations;
    double average_allocation_size;
    double fragmentation_ratio;
    int cache_hits;
    int cache_misses;
} MemoryPoolStats;

// 优化的内存池管理器
typedef struct {
    OptimizedMemoryBlock* blocks;
    int num_blocks;
    int max_blocks;
    size_t pool_size;
    size_t block_size;
    int num_gpus;
    MemoryPoolStats stats;
    cudaStream_t* streams;
    cudaEvent_t* events;
    int auto_compaction_enabled;
    double fragmentation_threshold;
    int* gpu_memory_usage;
    size_t* gpu_memory_limits;
} OptimizedMemoryPool;

// 统一内存管理器
typedef struct {
    void* unified_ptr;
    size_t size;
    int preferred_gpu;
    int current_location;
    double last_migration_time;
    int migration_count;
    int access_pattern[8];  // 访问模式历史
} UnifiedMemoryBlock;

// 异步传输管理器
typedef struct {
    cudaStream_t* copy_streams;
    cudaEvent_t* copy_events;
    int num_streams;
    int current_stream;
    double* transfer_times;
    size_t* transfer_sizes;
    int transfer_count;
    double bandwidth_utilization;
} AsyncTransferManager;

// 初始化优化的内存池
OptimizedMemoryPool* init_optimized_memory_pool(size_t pool_size, size_t block_size, int num_gpus) {
    OptimizedMemoryPool* pool = (OptimizedMemoryPool*)malloc(sizeof(OptimizedMemoryPool));
    
    pool->pool_size = pool_size;
    pool->block_size = block_size;
    pool->num_gpus = num_gpus;
    pool->max_blocks = pool_size / block_size;
    pool->num_blocks = 0;
    pool->auto_compaction_enabled = 1;
    pool->fragmentation_threshold = 0.2;
    
    // 初始化统计信息
    memset(&pool->stats, 0, sizeof(MemoryPoolStats));
    pool->stats.total_allocated = pool_size;
    
    // 分配内存块数组
    pool->blocks = (OptimizedMemoryBlock*)malloc(pool->max_blocks * sizeof(OptimizedMemoryBlock));
    
    // 分配GPU资源
    pool->streams = (cudaStream_t*)malloc(num_gpus * sizeof(cudaStream_t));
    pool->events = (cudaEvent_t*)malloc(num_gpus * sizeof(cudaEvent_t));
    pool->gpu_memory_usage = (int*)calloc(num_gpus, sizeof(int));
    pool->gpu_memory_limits = (size_t*)malloc(num_gpus * sizeof(size_t));
    
    // 初始化每个GPU的流和事件
    for (int i = 0; i < num_gpus; i++) {
        cudaSetDevice(i);
        cudaStreamCreate(&pool->streams[i]);
        cudaEventCreate(&pool->events[i]);
        
        // 获取GPU内存限制
        size_t free_mem, total_mem;
        cudaMemGetInfo(&free_mem, &total_mem);
        pool->gpu_memory_limits[i] = free_mem * 0.8;  // 使用80%的可用内存
    }
    
    // 预分配内存池
    size_t current_offset = 0;
    for (int i = 0; i < pool->max_blocks && current_offset < pool_size; i++) {
        OptimizedMemoryBlock* block = &pool->blocks[i];
        
        // 使用cudaMallocManaged分配统一内存
        cudaMallocManaged(&block->ptr, block_size, cudaMemAttachGlobal);
        
        block->size = block_size;
        block->status = MEMORY_FREE;
        block->gpu_id = -1;  // 未分配
        block->stream = 0;
        block->last_access_time = 0.0;
        block->access_count = 0;
        block->prefetch_distance = 0;
        block->usage_pattern = 0;
        
        pool->num_blocks++;
        current_offset += block_size;
    }
    
    return pool;
}

// 智能内存分配
void* optimized_memory_alloc(OptimizedMemoryPool* pool, size_t size, int gpu_id) {
    // 查找最合适的内存块
    int best_block = -1;
    double best_score = -1.0;
    
    for (int i = 0; i < pool->num_blocks; i++) {
        OptimizedMemoryBlock* block = &pool->blocks[i];
        
        if (block->status == MEMORY_FREE && block->size >= size) {
            // 计算分配分数
            double score = 1.0;
            
            // GPU亲和性
            if (block->gpu_id == gpu_id) {
                score *= 1.5;
            }
            
            // 大小匹配度
            double size_ratio = (double)block->size / size;
            if (size_ratio < 1.2) {
                score *= 1.3;  // 偏好大小相近的块
            }
            
            // 访问模式匹配
            if (block->usage_pattern == 1 && size > pool->block_size / 2) {
                score *= 1.2;  // 大内存访问模式
            }
            
            if (score > best_score) {
                best_score = score;
                best_block = i;
            }
        }
    }
    
    if (best_block == -1) {
        // 尝试内存碎片整理
        if (pool->auto_compaction_enabled) {
            perform_memory_compaction(pool);
            return optimized_memory_alloc(pool, size, gpu_id);  // 重试
        }
        
        // 分配新的内存块
        return allocate_new_block(pool, size, gpu_id);
    }
    
    // 分配找到的块
    OptimizedMemoryBlock* block = &pool->blocks[best_block];
    block->status = MEMORY_ALLOCATED;
    block->gpu_id = gpu_id;
    block->last_access_time = omp_get_wtime();
    block->access_count = 1;
    
    // 更新统计信息
    pool->stats.total_used += block->size;
    pool->stats.total_free -= block->size;
    pool->stats.num_allocations++;
    pool->stats.cache_hits++;
    
    if (pool->stats.total_used > pool->stats.peak_usage) {
        pool->stats.peak_usage = pool->stats.total_used;
    }
    
    // 预取到指定GPU
    if (gpu_id >= 0) {
        cudaMemPrefetchAsync(block->ptr, size, gpu_id, pool->streams[gpu_id]);
    }
    
    return block->ptr;
}

// 执行内存碎片整理
void perform_memory_compaction(OptimizedMemoryPool* pool) {
    // 将相邻的空闲块合并
    for (int i = 0; i < pool->num_blocks - 1; i++) {
        OptimizedMemoryBlock* block1 = &pool->blocks[i];
        OptimizedMemoryBlock* block2 = &pool->blocks[i + 1];
        
        if (block1->status == MEMORY_FREE && block2->status == MEMORY_FREE) {
            // 合并两个块
            block1->size += block2->size;
            
            // 移动后续块
            for (int j = i + 1; j < pool->num_blocks - 1; j++) {
                pool->blocks[j] = pool->blocks[j + 1];
            }
            
            pool->num_blocks--;
            i--;  // 重新检查当前位置
        }
    }
    
    pool->stats.fragmentation_ratio = calculate_fragmentation_ratio(pool);
}

// 计算碎片率
double calculate_fragmentation_ratio(OptimizedMemoryPool* pool) {
    size_t total_free = 0;
    size_t largest_free = 0;
    
    for (int i = 0; i < pool->num_blocks; i++) {
        if (pool->blocks[i].status == MEMORY_FREE) {
            total_free += pool->blocks[i].size;
            if (pool->blocks[i].size > largest_free) {
                largest_free = pool->blocks[i].size;
            }
        }
    }
    
    if (total_free == 0) return 0.0;
    
    return 1.0 - (double)largest_free / total_free;
}

// 分配新的内存块
void* allocate_new_block(OptimizedMemoryPool* pool, size_t size, int gpu_id) {
    // 检查是否超出内存限制
    if (pool->num_blocks >= pool->max_blocks) {
        return NULL;  // 内存池已满
    }
    
    // 分配新的内存块
    OptimizedMemoryBlock* block = &pool->blocks[pool->num_blocks];
    
    cudaMallocManaged(&block->ptr, size, cudaMemAttachGlobal);
    
    block->size = size;
    block->status = MEMORY_ALLOCATED;
    block->gpu_id = gpu_id;
    block->stream = pool->streams[gpu_id >= 0 ? gpu_id : 0];
    block->last_access_time = omp_get_wtime();
    block->access_count = 1;
    block->prefetch_distance = 0;
    block->usage_pattern = (size > pool->block_size) ? 1 : 0;
    
    pool->num_blocks++;
    pool->stats.total_allocated += size;
    pool->stats.total_used += size;
    pool->stats.num_allocations++;
    pool->stats.cache_misses++;
    pool->stats.average_allocation_size = 
        (pool->stats.average_allocation_size * (pool->stats.num_allocations - 1) + size) / 
        pool->stats.num_allocations;
    
    if (pool->stats.total_used > pool->stats.peak_usage) {
        pool->stats.peak_usage = pool->stats.total_used;
    }
    
    // 预取到指定GPU
    if (gpu_id >= 0) {
        cudaMemPrefetchAsync(block->ptr, size, gpu_id, block->stream);
    }
    
    return block->ptr;
}

// 智能内存释放
void optimized_memory_free(OptimizedMemoryPool* pool, void* ptr) {
    if (!ptr) return;
    
    // 查找对应的内存块
    for (int i = 0; i < pool->num_blocks; i++) {
        OptimizedMemoryBlock* block = &pool->blocks[i];
        
        if (block->ptr == ptr && block->status == MEMORY_ALLOCATED) {
            // 更新统计信息
            pool->stats.total_used -= block->size;
            pool->stats.total_free += block->size;
            pool->stats.num_deallocations++;
            
            // 重置块状态
            block->status = MEMORY_FREE;
            block->gpu_id = -1;
            block->last_access_time = omp_get_wtime();
            
            // 异步释放（延迟到需要时）
            // 注意：这里不真正释放内存，而是标记为可用
            
            return;
        }
    }
}

// 初始化异步传输管理器
AsyncTransferManager* init_async_transfer_manager(int num_streams) {
    AsyncTransferManager* manager = (AsyncTransferManager*)malloc(sizeof(AsyncTransferManager));
    
    manager->num_streams = num_streams;
    manager->current_stream = 0;
    manager->transfer_count = 0;
    manager->bandwidth_utilization = 0.0;
    
    // 分配流和事件
    manager->copy_streams = (cudaStream_t*)malloc(num_streams * sizeof(cudaStream_t));
    manager->copy_events = (cudaEvent_t*)malloc(num_streams * sizeof(cudaEvent_t));
    manager->transfer_times = (double*)malloc(1000 * sizeof(double));
    manager->transfer_sizes = (size_t*)malloc(1000 * sizeof(size_t));
    
    // 初始化流和事件
    for (int i = 0; i < num_streams; i++) {
        cudaStreamCreate(&manager->copy_streams[i]);
        cudaEventCreate(&manager->copy_events[i]);
    }
    
    return manager;
}

// 优化的异步数据传输
void optimized_async_transfer(
    AsyncTransferManager* manager,
    void* dst, const void* src,
    size_t size, cudaMemcpyKind kind,
    int preferred_gpu
) {
    // 选择下一个可用的流
    int stream_idx = manager->current_stream;
    cudaStream_t stream = manager->copy_streams[stream_idx];
    
    // 记录传输开始时间
    double start_time = omp_get_wtime();
    
    // 执行异步传输
    cudaMemcpyAsync(dst, src, size, kind, stream);
    
    // 记录传输事件
    cudaEventRecord(manager->copy_events[stream_idx], stream);
    
    // 更新传输统计
    if (manager->transfer_count < 1000) {
        manager->transfer_sizes[manager->transfer_count] = size;
        manager->transfer_times[manager->transfer_count] = omp_get_wtime() - start_time;
        manager->transfer_count++;
    }
    
    // 更新当前流索引
    manager->current_stream = (manager->current_stream + 1) % manager->num_streams;
    
    // 计算带宽利用率
    update_bandwidth_utilization(manager);
}

// 更新带宽利用率
void update_bandwidth_utilization(AsyncTransferManager* manager) {
    if (manager->transfer_count == 0) return;
    
    double total_time = 0.0;
    size_t total_size = 0;
    
    for (int i = 0; i < manager->transfer_count; i++) {
        total_time += manager->transfer_times[i];
        total_size += manager->transfer_sizes[i];
    }
    
    // 假设PCIe带宽为16 GB/s
    double theoretical_bandwidth = 16.0 * 1024 * 1024 * 1024;  // bytes per second
    double achieved_bandwidth = total_size / total_time;
    
    manager->bandwidth_utilization = achieved_bandwidth / theoretical_bandwidth;
}

// 等待所有传输完成
void wait_for_all_transfers(AsyncTransferManager* manager) {
    // 等待所有流传输完成
    for (int i = 0; i < manager->num_streams; i++) {
        cudaStreamSynchronize(manager->copy_streams[i]);
    }
}

// 智能内存预取
void optimized_memory_prefetch(
    OptimizedMemoryPool* pool,
    void* ptr, size_t size,
    int target_gpu, int prefetch_distance
) {
    if (!ptr || target_gpu < 0) return;
    
    // 查找对应的内存块
    for (int i = 0; i < pool->num_blocks; i++) {
        OptimizedMemoryBlock* block = &pool->blocks[i];
        
        if (block->ptr == ptr && block->status == MEMORY_ALLOCATED) {
            // 更新预取距离
            block->prefetch_distance = prefetch_distance;
            
            // 执行预取
            cudaMemPrefetchAsync(ptr, size, target_gpu, pool->streams[target_gpu]);
            
            // 更新访问统计
            block->access_count++;
            block->last_access_time = omp_get_wtime();
            
            return;
        }
    }
}

// 内存访问模式分析
void analyze_memory_access_pattern(OptimizedMemoryPool* pool) {
    // 分析访问模式以优化未来的分配
    int sequential_access = 0;
    int random_access = 0;
    int large_block_access = 0;
    
    for (int i = 0; i < pool->num_blocks; i++) {
        OptimizedMemoryBlock* block = &pool->blocks[i];
        
        if (block->status == MEMORY_ALLOCATED && block->access_count > 0) {
            if (block->size > pool->block_size * 2) {
                large_block_access++;
            }
            
            // 分析访问时间模式
            if (block->access_count > 5) {
                sequential_access++;
            } else {
                random_access++;
            }
        }
    }
    
    // 根据访问模式调整内存池参数
    if (large_block_access > pool->num_blocks * 0.3) {
        // 增加大块内存分配
        pool->block_size *= 1.5;
    }
    
    if (sequential_access > random_access * 2) {
        // 启用更积极的预取策略
        for (int i = 0; i < pool->num_gpus; i++) {
            cudaSetDevice(i);
            cudaDeviceSetLimit(cudaLimitMallocHeapSize, pool->pool_size / 4);
        }
    }
}

// 生成内存池性能报告
void generate_memory_pool_report(OptimizedMemoryPool* pool, const char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    
    fprintf(fp, "=== 优化内存池性能报告 ===\n\n");
    fprintf(fp, "内存池配置:\n");
    fprintf(fp, "  总大小: %.2f MB\n", pool->pool_size / (1024.0 * 1024.0));
    fprintf(fp, "  块大小: %.2f KB\n", pool->block_size / 1024.0);
    fprintf(fp, "  GPU数量: %d\n", pool->num_gpus);
    fprintf(fp, "  最大块数: %d\n", pool->max_blocks);
    fprintf(fp, "  当前块数: %d\n\n", pool->num_blocks);
    
    fprintf(fp, "内存使用统计:\n");
    fprintf(fp, "  总分配: %.2f MB\n", pool->stats.total_allocated / (1024.0 * 1024.0));
    fprintf(fp, "  已使用: %.2f MB\n", pool->stats.total_used / (1024.0 * 1024.0));
    fprintf(fp, "  空闲: %.2f MB\n", pool->stats.total_free / (1024.0 * 1024.0));
    fprintf(fp, "  峰值使用: %.2f MB\n", pool->stats.peak_usage / (1024.0 * 1024.0));
    fprintf(fp, "  碎片率: %.1f%%\n", pool->stats.fragmentation_ratio * 100.0);
    fprintf(fp, "\n分配统计:\n");
    fprintf(fp, "  分配次数: %d\n", pool->stats.num_allocations);
    fprintf(fp, "  释放次数: %d\n", pool->stats.num_deallocations);
    fprintf(fp, "  缓存命中率: %.1f%%\n", 
           100.0 * pool->stats.cache_hits / (pool->stats.cache_hits + pool->stats.cache_misses));
    fprintf(fp, "  平均分配大小: %.2f KB\n", pool->stats.average_allocation_size / 1024.0);
    
    // GPU内存使用情况
    fprintf(fp, "\nGPU内存使用:\n");
    for (int i = 0; i < pool->num_gpus; i++) {
        fprintf(fp, "  GPU %d: %d 块, 限制: %.2f MB\n", 
               i, pool->gpu_memory_usage[i], pool->gpu_memory_limits[i] / (1024.0 * 1024.0));
    }
    
    fclose(fp);
}

// 清理优化的内存池
void cleanup_optimized_memory_pool(OptimizedMemoryPool* pool) {
    if (!pool) return;
    
    // 释放所有内存块
    for (int i = 0; i < pool->num_blocks; i++) {
        if (pool->blocks[i].ptr) {
            cudaFree(pool->blocks[i].ptr);
        }
    }
    
    // 清理GPU资源
    for (int i = 0; i < pool->num_gpus; i++) {
        cudaSetDevice(i);
        cudaStreamDestroy(pool->streams[i]);
        cudaEventDestroy(pool->events[i]);
    }
    
    free(pool->blocks);
    free(pool->streams);
    free(pool->events);
    free(pool->gpu_memory_usage);
    free(pool->gpu_memory_limits);
    free(pool);
}

// 清理异步传输管理器
void cleanup_async_transfer_manager(AsyncTransferManager* manager) {
    if (!manager) return;
    
    // 销毁流和事件
    for (int i = 0; i < manager->num_streams; i++) {
        cudaStreamDestroy(manager->copy_streams[i]);
        cudaEventDestroy(manager->copy_events[i]);
    }
    
    free(manager->copy_streams);
    free(manager->copy_events);
    free(manager->transfer_times);
    free(manager->transfer_sizes);
    free(manager);
}