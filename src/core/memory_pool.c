/******************************************************************************
 * Memory Pool Allocator - Implementation
 ******************************************************************************/

#include "memory_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

// Memory pool block structure
typedef struct pool_block {
    struct pool_block* next;
    uint8_t data[1];  // Flexible array member
} pool_block_t;

// Memory pool structure
struct memory_pool {
    size_t block_size;           // Size of each block
    size_t total_blocks;         // Total number of blocks
    size_t used_blocks;          // Currently used blocks
    pool_block_t* free_list;      // Free block list
    pool_block_t** block_array;  // Array of all blocks for tracking
    size_t block_array_size;     // Size of block array
};

memory_pool_t* memory_pool_create(size_t block_size, size_t initial_blocks) {
    if (block_size == 0 || initial_blocks == 0) {
        return NULL;
    }
    
    memory_pool_t* pool = (memory_pool_t*)calloc(1, sizeof(memory_pool_t));
    if (!pool) {
        return NULL;
    }
    
    pool->block_size = block_size;
    pool->total_blocks = initial_blocks;
    pool->used_blocks = 0;
    pool->free_list = NULL;
    pool->block_array_size = initial_blocks;
    pool->block_array = (pool_block_t**)calloc(initial_blocks, sizeof(pool_block_t*));
    
    if (!pool->block_array) {
        free(pool);
        return NULL;
    }
    
    // Allocate initial blocks
    for (size_t i = 0; i < initial_blocks; i++) {
        pool_block_t* block = (pool_block_t*)calloc(1, sizeof(pool_block_t) + block_size - 1);
        if (!block) {
            // Cleanup on error
            memory_pool_destroy(pool);
            return NULL;
        }
        
        // Add to free list
        block->next = pool->free_list;
        pool->free_list = block;
        
        // Track block
        pool->block_array[i] = block;
    }
    
    return pool;
}

void memory_pool_destroy(memory_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Free all blocks
    if (pool->block_array) {
        for (size_t i = 0; i < pool->total_blocks; i++) {
            if (pool->block_array[i]) {
                free(pool->block_array[i]);
            }
        }
        free(pool->block_array);
    }
    
    free(pool);
}

void* memory_pool_alloc(memory_pool_t* pool) {
    if (!pool) {
        return NULL;
    }
    
    // Get block from free list
    if (pool->free_list) {
        pool_block_t* block = pool->free_list;
        pool->free_list = block->next;
        pool->used_blocks++;
        return block->data;
    }
    
    // No free blocks - allocate new one
    pool_block_t* block = (pool_block_t*)calloc(1, sizeof(pool_block_t) + pool->block_size - 1);
    if (!block) {
        return NULL;
    }
    
    // Expand block array if needed
    if (pool->total_blocks >= pool->block_array_size) {
        size_t new_size = pool->block_array_size * 2;
        pool_block_t** new_array = (pool_block_t**)realloc(pool->block_array, 
                                                           new_size * sizeof(pool_block_t*));
        if (!new_array) {
            free(block);
            return NULL;
        }
        pool->block_array = new_array;
        pool->block_array_size = new_size;
    }
    
    // Track block
    pool->block_array[pool->total_blocks] = block;
    pool->total_blocks++;
    pool->used_blocks++;
    
    return block->data;
}

void memory_pool_free(memory_pool_t* pool, void* ptr) {
    if (!pool || !ptr) {
        return;
    }
    
    // Convert pointer to block
    // Calculate offset: data is at end of pool_block_t structure
    pool_block_t* block = (pool_block_t*)((uint8_t*)ptr - offsetof(pool_block_t, data));
    
    // Add to free list
    block->next = pool->free_list;
    pool->free_list = block;
    pool->used_blocks--;
}

int memory_pool_get_stats(memory_pool_t* pool, size_t* total_blocks,
                         size_t* used_blocks, size_t* free_blocks) {
    if (!pool || !total_blocks || !used_blocks || !free_blocks) {
        return -1;
    }
    
    *total_blocks = pool->total_blocks;
    *used_blocks = pool->used_blocks;
    *free_blocks = pool->total_blocks - pool->used_blocks;
    
    return 0;
}

void memory_pool_reset(memory_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Rebuild free list from all blocks
    pool->free_list = NULL;
    pool->used_blocks = 0;
    
    if (pool->block_array) {
        for (size_t i = 0; i < pool->total_blocks; i++) {
            if (pool->block_array[i]) {
                pool_block_t* block = pool->block_array[i];
                block->next = pool->free_list;
                pool->free_list = block;
            }
        }
    }
}
