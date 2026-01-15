/******************************************************************************
 * Memory Pool Allocator
 * 
 * Provides efficient memory allocation for frequently allocated/deallocated
 * objects of similar sizes
 ******************************************************************************/

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Memory Pool Structure
 ******************************************************************************/

typedef struct memory_pool memory_pool_t;

/******************************************************************************
 * Memory Pool Functions
 ******************************************************************************/

/**
 * Create a memory pool
 * 
 * @param block_size Size of each block in the pool
 * @param initial_blocks Number of initial blocks to allocate
 * @return Memory pool handle, NULL on error
 */
memory_pool_t* memory_pool_create(size_t block_size, size_t initial_blocks);

/**
 * Destroy a memory pool and free all memory
 * 
 * @param pool Memory pool to destroy
 */
void memory_pool_destroy(memory_pool_t* pool);

/**
 * Allocate a block from the pool
 * 
 * @param pool Memory pool
 * @return Pointer to allocated block, NULL on error
 */
void* memory_pool_alloc(memory_pool_t* pool);

/**
 * Free a block back to the pool
 * 
 * @param pool Memory pool
 * @param ptr Pointer to block to free
 */
void memory_pool_free(memory_pool_t* pool, void* ptr);

/**
 * Get pool statistics
 * 
 * @param pool Memory pool
 * @param total_blocks Output: total blocks in pool
 * @param used_blocks Output: currently used blocks
 * @param free_blocks Output: free blocks
 * @return 0 on success, negative on error
 */
int memory_pool_get_stats(memory_pool_t* pool, size_t* total_blocks,
                         size_t* used_blocks, size_t* free_blocks);

/**
 * Reset pool (free all blocks but keep pool structure)
 * 
 * @param pool Memory pool
 */
void memory_pool_reset(memory_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif // MEMORY_POOL_H
