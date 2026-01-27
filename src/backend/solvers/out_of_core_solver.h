/**
 * @file out_of_core_solver.h
 * @brief Complete out-of-core solver implementation for large problems
 * @details Disk-backed matrix storage with streaming matrix-vector operations
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef OUT_OF_CORE_SOLVER_H
#define OUT_OF_CORE_SOLVER_H

#include <stdint.h>
#include <stdbool.h>
#include <complex.h>
#include "core_common.h"
#include "sparse_direct_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Out-of-core matrix block size
typedef enum {
    OOC_BLOCK_SIZE_SMALL = 256,     // 256x256 blocks
    OOC_BLOCK_SIZE_MEDIUM = 512,    // 512x512 blocks
    OOC_BLOCK_SIZE_LARGE = 1024,    // 1024x1024 blocks
    OOC_BLOCK_SIZE_EXTRA_LARGE = 2048 // 2048x2048 blocks
} ooc_block_size_t;

// Out-of-core storage format
typedef enum {
    OOC_FORMAT_CSR,      // Compressed Sparse Row blocks
    OOC_FORMAT_CSC,      // Compressed Sparse Column blocks
    OOC_FORMAT_DENSE,    // Dense blocks
    OOC_FORMAT_HYBRID    // Mixed sparse/dense blocks
} ooc_format_t;

// Out-of-core solver configuration
typedef struct {
    ooc_block_size_t block_size;
    ooc_format_t storage_format;
    
    // Memory management
    size_t max_memory_usage;        // Maximum memory usage in bytes
    size_t block_cache_size;      // Number of blocks to cache
    double memory_utilization;      // Target memory utilization (0.0-1.0)
    
    // I/O optimization
    int io_parallelism;            // Number of parallel I/O threads
    size_t io_buffer_size;         // I/O buffer size per thread
    bool use_async_io;             // Use asynchronous I/O
    bool use_memory_mapping;       // Use memory-mapped files
    
    // Storage paths
    char matrix_directory[512];      // Directory for matrix blocks
    char vector_directory[512];      // Directory for vector blocks
    char checkpoint_directory[512];  // Directory for checkpoints
    
    // Checkpoint settings
    bool enable_checkpoints;       // Enable checkpointing
    int checkpoint_frequency;       // Checkpoint every N iterations
    bool compress_checkpoints;     // Compress checkpoint files
    
    // Performance tuning
    double prefetch_threshold;      // Prefetch when memory usage > threshold
    int prefetch_lookahead;        // Number of blocks to prefetch
    bool use_block_pool;           // Use block memory pool
    
} ooc_solver_config_t;

// Out-of-core matrix block
typedef struct {
    int block_row;                  // Block row index
    int block_col;                  // Block column index
    int row_start;                  // Global row start index
    int row_end;                    // Global row end index
    int col_start;                  // Global column start index
    int col_end;                    // Global column end index
    
    // Block data
    union {
        struct {
            int *row_ptr;           // CSR row pointer
            int *col_idx;           // CSR column indices
            double complex *values; // CSR values
            int nnz;                // Number of non-zeros
        } csr;
        struct {
            double complex *data;   // Dense matrix data
        } dense;
    } data;
    
    size_t memory_size;             // Memory usage in bytes
    bool is_sparse;                 // True if sparse, false if dense
    bool is_cached;                 // True if currently in memory
    bool is_modified;               // True if modified since last save
    
} ooc_matrix_block_t;

// Out-of-core matrix
typedef struct ooc_matrix ooc_matrix_t;

// Out-of-core vector
typedef struct ooc_vector ooc_vector_t;

// Out-of-core solver handle
typedef struct ooc_solver ooc_solver_t;

// Block cache entry
typedef struct {
    ooc_matrix_block_t *block;
    double last_access_time;
    int access_count;
    bool is_locked;                 // Prevent eviction
} block_cache_entry_t;

// Solver statistics
typedef struct {
    // Performance metrics
    double total_time;
    double io_time;
    double computation_time;
    double cache_time;
    
    // I/O statistics
    int64_t blocks_read;
    int64_t blocks_written;
    int64_t bytes_read;
    int64_t bytes_written;
    int64_t io_operations;
    
    // Cache statistics
    int64_t cache_hits;
    int64_t cache_misses;
    double cache_hit_ratio;
    int64_t cache_evictions;
    
    // Memory statistics
    size_t peak_memory_usage;
    size_t current_memory_usage;
    int64_t memory_allocations;
    int64_t memory_deallocations;
    
    // Checkpoint statistics
    int64_t checkpoints_created;
    int64_t checkpoints_loaded;
    double checkpoint_time;
    
} ooc_solver_stats_t;

// Function prototypes

/**
 * @brief Initialize out-of-core solver
 * @param config Solver configuration
 * @return Solver handle or NULL on failure
 */
ooc_solver_t* ooc_solver_init(const ooc_solver_config_t *config);

/**
 * @brief Create out-of-core matrix
 * @param solver Solver handle
 * @param num_rows Number of rows
 * @param num_cols Number of columns
 * @param nnz Number of non-zeros (estimated)
 * @return Matrix handle or NULL on failure
 */
ooc_matrix_t* ooc_matrix_create(ooc_solver_t *solver, int num_rows, int num_cols, int64_t nnz);

/**
 * @brief Create out-of-core vector
 * @param solver Solver handle
 * @param size Vector size
 * @return Vector handle or NULL on failure
 */
ooc_vector_t* ooc_vector_create(ooc_solver_t *solver, int size);

/**
 * @brief Set matrix element
 * @param matrix Matrix handle
 * @param row Row index
 * @param col Column index
 * @param value Value to set
 * @return 0 on success, error code on failure
 */
int ooc_matrix_set_element(ooc_matrix_t *matrix, int row, int col, double complex value);

/**
 * @brief Get matrix element
 * @param matrix Matrix handle
 * @param row Row index
 * @param col Column index
 * @param value Output value
 * @return 0 on success, error code on failure
 */
int ooc_matrix_get_element(ooc_matrix_t *matrix, int row, int col, double complex *value);

/**
 * @brief Set vector element
 * @param vector Vector handle
 * @param index Element index
 * @param value Value to set
 * @return 0 on success, error code on failure
 */
int ooc_vector_set_element(ooc_vector_t *vector, int index, double complex value);

/**
 * @brief Get vector element
 * @param vector Vector handle
 * @param index Element index
 * @param value Output value
 * @return 0 on success, error code on failure
 */
int ooc_vector_get_element(ooc_vector_t *vector, int index, double complex *value);

/**
 * @brief Perform matrix-vector multiplication: y = A * x
 * @param matrix Matrix handle
 * @param x Input vector
 * @param y Output vector
 * @return 0 on success, error code on failure
 */
int ooc_matrix_vector_multiply(ooc_matrix_t *matrix, ooc_vector_t *x, ooc_vector_t *y);

/**
 * @brief Perform matrix-vector multiplication with scaling: y = alpha * A * x + beta * y
 * @param matrix Matrix handle
 * @param alpha Scaling factor for A*x
 * @param x Input vector
 * @param beta Scaling factor for y
 * @param y Input/output vector
 * @return 0 on success, error code on failure
 */
int ooc_matrix_vector_multiply_scaled(ooc_matrix_t *matrix, double complex alpha,
                                    ooc_vector_t *x, double complex beta, ooc_vector_t *y);

/**
 * @brief Solve linear system using out-of-core solver
 * @param solver Solver handle
 * @param matrix Coefficient matrix
 * @param rhs Right-hand side vector
 * @param solution Solution vector
 * @param tolerance Convergence tolerance
 * @param max_iterations Maximum number of iterations
 * @return 0 on success, error code on failure
 */
int ooc_solver_solve(ooc_solver_t *solver, ooc_matrix_t *matrix, ooc_vector_t *rhs,
                    ooc_vector_t *solution, double tolerance, int max_iterations);

/**
 * @brief Create checkpoint of current solver state
 * @param solver Solver handle
 * @param checkpoint_name Checkpoint name
 * @return 0 on success, error code on failure
 */
int ooc_solver_checkpoint(ooc_solver_t *solver, const char *checkpoint_name);

/**
 * @brief Load solver state from checkpoint
 * @param solver Solver handle
 * @param checkpoint_name Checkpoint name
 * @return 0 on success, error code on failure
 */
int ooc_solver_load_checkpoint(ooc_solver_t *solver, const char *checkpoint_name);

/**
 * @brief Get solver statistics
 * @param solver Solver handle
 * @param stats Output statistics
 * @return 0 on success, error code on failure
 */
int ooc_solver_get_stats(ooc_solver_t *solver, ooc_solver_stats_t *stats);

/**
 * @brief Flush all cached blocks to disk
 * @param solver Solver handle
 * @return 0 on success, error code on failure
 */
int ooc_solver_flush_cache(ooc_solver_t *solver);

/**
 * @brief Clear cache and free memory
 * @param solver Solver handle
 * @return 0 on success, error code on failure
 */
int ooc_solver_clear_cache(ooc_solver_t *solver);

/**
 * @brief Prefetch blocks for upcoming operations
 * @param solver Solver handle
 * @param block_rows Block rows to prefetch
 * @param num_blocks Number of blocks
 * @return 0 on success, error code on failure
 */
int ooc_solver_prefetch_blocks(ooc_solver_t *solver, int *block_rows, int num_blocks);

/**
 * @brief Get memory usage information
 * @param solver Solver handle
 * @param current_usage Current memory usage in bytes
 * @param peak_usage Peak memory usage in bytes
 * @param cache_size Cache size in bytes
 * @return 0 on success, error code on failure
 */
int ooc_solver_get_memory_info(ooc_solver_t *solver, size_t *current_usage,
                               size_t *peak_usage, size_t *cache_size);

/**
 * @brief Convert dense matrix to out-of-core format
 * @param solver Solver handle
 * @param dense_matrix Dense matrix data (row-major)
 * @param num_rows Number of rows
 * @param num_cols Number of columns
 * @param threshold Threshold for considering element as non-zero
 * @return Matrix handle or NULL on failure
 */
ooc_matrix_t* ooc_matrix_from_dense(ooc_solver_t *solver, double complex *dense_matrix,
                                   int num_rows, int num_cols, double threshold);

/**
 * @brief Convert sparse matrix to out-of-core format
 * @param solver Solver handle
 * @param row_ptr Row pointer array (CSR)
 * @param col_idx Column indices array (CSR)
 * @param values Values array (CSR)
 * @param num_rows Number of rows
 * @param num_cols Number of columns
 * @param nnz Number of non-zeros
 * @return Matrix handle or NULL on failure
 */
ooc_matrix_t* ooc_matrix_from_sparse(ooc_solver_t *solver, int *row_ptr, int *col_idx,
                                    double complex *values, int num_rows, int num_cols, int nnz);

/**
 * @brief Convert out-of-core matrix to dense format
 * @param matrix Matrix handle
 * @param dense_matrix Output dense matrix (pre-allocated)
 * @return 0 on success, error code on failure
 */
int ooc_matrix_to_dense(ooc_matrix_t *matrix, double complex *dense_matrix);

/**
 * @brief Get default configuration for out-of-core solver
 * @param config Output configuration
 */
void ooc_solver_get_default_config(ooc_solver_config_t *config);

/**
 * @brief Cleanup out-of-core solver resources
 * @param solver Solver handle
 */
void ooc_solver_destroy(ooc_solver_t *solver);

/**
 * @brief Cleanup matrix resources
 * @param matrix Matrix handle
 */
void ooc_matrix_destroy(ooc_matrix_t *matrix);

/**
 * @brief Cleanup vector resources
 * @param vector Vector handle
 */
void ooc_vector_destroy(ooc_vector_t *vector);

#ifdef __cplusplus
}
#endif

#endif // OUT_OF_CORE_SOLVER_H