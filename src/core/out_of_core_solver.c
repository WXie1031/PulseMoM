/**
 * @file out_of_core_solver.c
 * @brief Implementation of complete out-of-core solver for large problems
 * @details Disk-backed matrix storage with streaming matrix-vector operations
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "out_of_core_solver.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

// Internal structures
struct ooc_matrix {
    ooc_solver_t *solver;           // Parent solver
    int num_rows;                   // Total number of rows
    int num_cols;                   // Total number of columns
    int64_t nnz;                    // Total number of non-zeros
    
    // Block structure
    int num_block_rows;             // Number of block rows
    int num_block_cols;             // Number of block columns
    int block_size;                 // Block size
    
    // Block storage
    ooc_matrix_block_t **blocks;    // Block storage array [block_row][block_col]
    bool **block_exists;            // Block existence matrix
    
    // Statistics
    int64_t blocks_created;
    int64_t blocks_accessed;
    double creation_time;
};

struct ooc_vector {
    ooc_solver_t *solver;           // Parent solver
    int size;                       // Vector size
    int num_blocks;                 // Number of blocks
    int block_size;                 // Block size
    
    // Block storage
    double complex **blocks;        // Block data
    bool *block_cached;             // Block cache status
    bool *block_modified;           // Block modification status
    
    // File handles
    FILE **block_files;             // Block file handles
    char **block_filenames;         // Block filenames
    
    // Statistics
    int64_t blocks_accessed;
    double creation_time;
};

struct ooc_solver {
    ooc_solver_config_t config;     // Configuration
    
    // Cache management
    block_cache_entry_t *cache;     // Block cache
    int cache_size;                 // Current cache size
    int max_cache_size;             // Maximum cache size
    pthread_mutex_t cache_mutex;    // Cache mutex
    
    // I/O management
    pthread_t *io_threads;          // I/O worker threads
    int num_io_threads;              // Number of I/O threads
    bool io_threads_running;        // I/O threads status
    
    // Performance tracking
    ooc_solver_stats_t stats;       // Statistics
    double start_time;              // Solver start time
    
    // Working directories
    char matrix_dir[512];           // Matrix storage directory
    char vector_dir[512];           // Vector storage directory
    char checkpoint_dir[512];       // Checkpoint directory
    
    // Iterative solver state
    double *residual_history;       // Residual history for convergence
    int residual_history_size;      // Size of residual history
    double complex **krylov_basis;  // Krylov subspace basis
    int krylov_dimension;           // Krylov subspace dimension
    
    // Convergence tracking
    int current_iteration;          // Current iteration
    double current_residual;        // Current residual norm
    bool converged;                 // Convergence flag
};

// Utility functions
static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int create_directory(const char *path) {
    struct stat st = {0};
    
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0700) != 0 && errno != EEXIST) {
            return -1;
        }
    }
    
    return 0;
}

static char* generate_block_filename(const char *directory, const char *prefix,
                                   int block_row, int block_col, const char *suffix) {
    char *filename = (char *)malloc(512);
    if (!filename) return NULL;
    
    snprintf(filename, 512, "%s/%s_block_%d_%d.%s", directory, prefix, block_row, block_col, suffix);
    return filename;
}

static int save_matrix_block_to_file(const ooc_matrix_block_t *block, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        return -1;
    }
    
    // Write block metadata
    fwrite(&block->block_row, sizeof(int), 1, file);
    fwrite(&block->block_col, sizeof(int), 1, file);
    fwrite(&block->row_start, sizeof(int), 1, file);
    fwrite(&block->row_end, sizeof(int), 1, file);
    fwrite(&block->col_start, sizeof(int), 1, file);
    fwrite(&block->col_end, sizeof(int), 1, file);
    fwrite(&block->is_sparse, sizeof(bool), 1, file);
    
    int block_rows = block->row_end - block->row_start;
    int block_cols = block->col_end - block->col_start;
    
    if (block->is_sparse) {
        // Save sparse block (CSR format)
        fwrite(&block->data.csr.nnz, sizeof(int), 1, file);
        fwrite(block->data.csr.row_ptr, sizeof(int), block_rows + 1, file);
        fwrite(block->data.csr.col_idx, sizeof(int), block->data.csr.nnz, file);
        fwrite(block->data.csr.values, sizeof(double complex), block->data.csr.nnz, file);
    } else {
        // Save dense block
        int total_elements = block_rows * block_cols;
        fwrite(block->data.dense.data, sizeof(double complex), total_elements, file);
    }
    
    fclose(file);
    return 0;
}

static int load_matrix_block_from_file(ooc_matrix_block_t *block, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return -1;
    }
    
    // Read block metadata
    fread(&block->block_row, sizeof(int), 1, file);
    fread(&block->block_col, sizeof(int), 1, file);
    fread(&block->row_start, sizeof(int), 1, file);
    fread(&block->row_end, sizeof(int), 1, file);
    fread(&block->col_start, sizeof(int), 1, file);
    fread(&block->col_end, sizeof(int), 1, file);
    fread(&block->is_sparse, sizeof(bool), 1, file);
    
    int block_rows = block->row_end - block->row_start;
    int block_cols = block->col_end - block->col_start;
    
    if (block->is_sparse) {
        // Load sparse block (CSR format)
        fread(&block->data.csr.nnz, sizeof(int), 1, file);
        
        block->data.csr.row_ptr = (int *)malloc((block_rows + 1) * sizeof(int));
        block->data.csr.col_idx = (int *)malloc(block->data.csr.nnz * sizeof(int));
        block->data.csr.values = (double complex *)malloc(block->data.csr.nnz * sizeof(double complex));
        
        if (!block->data.csr.row_ptr || !block->data.csr.col_idx || !block->data.csr.values) {
            free(block->data.csr.row_ptr);
            free(block->data.csr.col_idx);
            free(block->data.csr.values);
            fclose(file);
            return -1;
        }
        
        fread(block->data.csr.row_ptr, sizeof(int), block_rows + 1, file);
        fread(block->data.csr.col_idx, sizeof(int), block->data.csr.nnz, file);
        fread(block->data.csr.values, sizeof(double complex), block->data.csr.nnz, file);
    } else {
        // Load dense block
        int total_elements = block_rows * block_cols;
        block->data.dense.data = (double complex *)malloc(total_elements * sizeof(double complex));
        
        if (!block->data.dense.data) {
            fclose(file);
            return -1;
        }
        
        fread(block->data.dense.data, sizeof(double complex), total_elements, file);
    }
    
    block->memory_size = block->is_sparse ? 
        (block->data.csr.nnz * (sizeof(double complex) + sizeof(int)) + (block_rows + 1) * sizeof(int)) :
        (block_rows * block_cols * sizeof(double complex));
    
    block->is_cached = true;
    block->is_modified = false;
    
    fclose(file);
    return 0;
}

static void matrix_block_free(ooc_matrix_block_t *block) {
    if (!block) return;
    
    if (block->is_sparse) {
        free(block->data.csr.row_ptr);
        free(block->data.csr.col_idx);
        free(block->data.csr.values);
    } else {
        free(block->data.dense.data);
    }
    
    block->is_cached = false;
}

static ooc_matrix_block_t* create_matrix_block(ooc_solver_t *solver, int block_row, int block_col,
                                             int row_start, int row_end, int col_start, int col_end) {
    ooc_matrix_block_t *block = (ooc_matrix_block_t *)calloc(1, sizeof(ooc_matrix_block_t));
    if (!block) return NULL;
    
    block->block_row = block_row;
    block->block_col = block_col;
    block->row_start = row_start;
    block->row_end = row_end;
    block->col_start = col_start;
    block->col_end = col_end;
    block->is_cached = true;
    block->is_modified = false;
    
    int block_rows = row_end - row_start;
    int block_cols = col_end - col_start;
    
    // Initially create as dense block (can be converted to sparse later)
    block->is_sparse = false;
    block->data.dense.data = (double complex *)calloc(block_rows * block_cols, sizeof(double complex));
    
    if (!block->data.dense.data) {
        free(block);
        return NULL;
    }
    
    block->memory_size = block_rows * block_cols * sizeof(double complex);
    
    return block;
}

static int evict_lru_block(ooc_solver_t *solver) {
    pthread_mutex_lock(&solver->cache_mutex);
    
    // Find least recently used unlocked block
    int lru_index = -1;
    double oldest_time = get_time();
    
    for (int i = 0; i < solver->cache_size; i++) {
        if (!solver->cache[i].is_locked && solver->cache[i].block &&
            solver->cache[i].last_access_time < oldest_time) {
            oldest_time = solver->cache[i].last_access_time;
            lru_index = i;
        }
    }
    
    if (lru_index >= 0) {
        ooc_matrix_block_t *block = solver->cache[lru_index].block;
        
        // Save to disk if modified
        if (block->is_modified) {
            char *filename = generate_block_filename(solver->matrix_dir, "matrix",
                                                   block->block_row, block->block_col, "blk");
            if (filename) {
                save_matrix_block_to_file(block, filename);
                free(filename);
            }
        }
        
        // Free memory
        matrix_block_free(block);
        free(block);
        
        // Remove from cache
        solver->cache[lru_index].block = NULL;
        solver->stats.cache_evictions++;
    }
    
    pthread_mutex_unlock(&solver->cache_mutex);
    return lru_index;
}

static int cache_matrix_block(ooc_solver_t *solver, ooc_matrix_block_t *block) {
    pthread_mutex_lock(&solver->cache_mutex);
    
    // Check if cache is full
    if (solver->cache_size >= solver->max_cache_size) {
        // Evict LRU block
        evict_lru_block(solver);
    }
    
    // Find empty cache slot
    int cache_index = -1;
    for (int i = 0; i < solver->max_cache_size; i++) {
        if (solver->cache[i].block == NULL) {
            cache_index = i;
            break;
        }
    }
    
    if (cache_index >= 0) {
        solver->cache[cache_index].block = block;
        solver->cache[cache_index].last_access_time = get_time();
        solver->cache[cache_index].access_count = 1;
        solver->cache[cache_index].is_locked = false;
        
        if (cache_index >= solver->cache_size) {
            solver->cache_size = cache_index + 1;
        }
        
        solver->stats.cache_hits++;
    }
    
    pthread_mutex_unlock(&solver->cache_mutex);
    return cache_index;
}

static ooc_matrix_block_t* get_cached_block(ooc_solver_t *solver, int block_row, int block_col) {
    pthread_mutex_lock(&solver->cache_mutex);
    
    // Search for block in cache
    ooc_matrix_block_t *block = NULL;
    for (int i = 0; i < solver->cache_size; i++) {
        if (solver->cache[i].block &&
            solver->cache[i].block->block_row == block_row &&
            solver->cache[i].block->block_col == block_col) {
            
            // Update access statistics
            solver->cache[i].last_access_time = get_time();
            solver->cache[i].access_count++;
            block = solver->cache[i].block;
            solver->stats.cache_hits++;
            break;
        }
    }
    
    pthread_mutex_unlock(&solver->cache_mutex);
    return block;
}

// Main implementation
ooc_solver_t* ooc_solver_init(const ooc_solver_config_t *config) {
    ooc_solver_t *solver = (ooc_solver_t *)calloc(1, sizeof(ooc_solver_t));
    if (!solver) return NULL;
    
    // Copy configuration
    memcpy(&solver->config, config, sizeof(ooc_solver_config_t));
    
    // Create working directories
    snprintf(solver->matrix_dir, sizeof(solver->matrix_dir), "%s/matrix", config->matrix_directory);
    snprintf(solver->vector_dir, sizeof(solver->vector_dir), "%s/vector", config->vector_directory);
    snprintf(solver->checkpoint_dir, sizeof(solver->checkpoint_dir), "%s/checkpoint", config->checkpoint_directory);
    
    if (create_directory(config->matrix_directory) != 0 ||
        create_directory(config->vector_directory) != 0 ||
        create_directory(config->checkpoint_directory) != 0 ||
        create_directory(solver->matrix_dir) != 0 ||
        create_directory(solver->vector_dir) != 0 ||
        create_directory(solver->checkpoint_dir) != 0) {
        free(solver);
        return NULL;
    }
    
    // Initialize cache
    solver->max_cache_size = config->block_cache_size;
    solver->cache = (block_cache_entry_t *)calloc(solver->max_cache_size, sizeof(block_cache_entry_t));
    if (!solver->cache) {
        free(solver);
        return NULL;
    }
    
    // Initialize mutex
    pthread_mutex_init(&solver->cache_mutex, NULL);
    
    // Initialize statistics
    solver->stats.cache_hits = 0;
    solver->stats.cache_misses = 0;
    solver->stats.cache_evictions = 0;
    solver->stats.blocks_read = 0;
    solver->stats.blocks_written = 0;
    solver->stats.io_operations = 0;
    
    solver->start_time = get_time();
    solver->current_iteration = 0;
    solver->converged = false;
    
    return solver;
}

ooc_matrix_t* ooc_matrix_create(ooc_solver_t *solver, int num_rows, int num_cols, int64_t nnz) {
    if (!solver || num_rows <= 0 || num_cols <= 0) return NULL;
    
    ooc_matrix_t *matrix = (ooc_matrix_t *)calloc(1, sizeof(ooc_matrix_t));
    if (!matrix) return NULL;
    
    matrix->solver = solver;
    matrix->num_rows = num_rows;
    matrix->num_cols = num_cols;
    matrix->nnz = nnz;
    
    // Calculate block structure
    matrix->block_size = solver->config.block_size;
    matrix->num_block_rows = (num_rows + matrix->block_size - 1) / matrix->block_size;
    matrix->num_block_cols = (num_cols + matrix->block_size - 1) / matrix->block_size;
    
    // Allocate block storage
    matrix->blocks = (ooc_matrix_block_t **)calloc(matrix->num_block_rows, sizeof(ooc_matrix_block_t *));
    matrix->block_exists = (bool **)calloc(matrix->num_block_rows, sizeof(bool *));
    
    if (!matrix->blocks || !matrix->block_exists) {
        free(matrix->blocks);
        free(matrix->block_exists);
        free(matrix);
        return NULL;
    }
    
    for (int i = 0; i < matrix->num_block_rows; i++) {
        matrix->blocks[i] = (ooc_matrix_block_t *)calloc(matrix->num_block_cols, sizeof(ooc_matrix_block_t));
        matrix->block_exists[i] = (bool *)calloc(matrix->num_block_cols, sizeof(bool));
        
        if (!matrix->blocks[i] || !matrix->block_exists[i]) {
            // Cleanup on failure
            for (int j = 0; j <= i; j++) {
                free(matrix->blocks[j]);
                free(matrix->block_exists[j]);
            }
            free(matrix->blocks);
            free(matrix->block_exists);
            free(matrix);
            return NULL;
        }
    }
    
    matrix->creation_time = get_time();
    matrix->blocks_created = 0;
    matrix->blocks_accessed = 0;
    
    return matrix;
}

ooc_vector_t* ooc_vector_create(ooc_solver_t *solver, int size) {
    if (!solver || size <= 0) return NULL;
    
    ooc_vector_t *vector = (ooc_vector_t *)calloc(1, sizeof(ooc_vector_t));
    if (!vector) return NULL;
    
    vector->solver = solver;
    vector->size = size;
    vector->block_size = solver->config.block_size;
    vector->num_blocks = (size + vector->block_size - 1) / vector->block_size;
    
    // Allocate block storage
    vector->blocks = (double complex **)calloc(vector->num_blocks, sizeof(double complex *));
    vector->block_cached = (bool *)calloc(vector->num_blocks, sizeof(bool));
    vector->block_modified = (bool *)calloc(vector->num_blocks, sizeof(bool));
    vector->block_files = (FILE **)calloc(vector->num_blocks, sizeof(FILE *));
    vector->block_filenames = (char **)calloc(vector->num_blocks, sizeof(char *));
    
    if (!vector->blocks || !vector->block_cached || !vector->block_modified ||
        !vector->block_files || !vector->block_filenames) {
        free(vector->blocks);
        free(vector->block_cached);
        free(vector->block_modified);
        free(vector->block_files);
        free(vector->block_filenames);
        free(vector);
        return NULL;
    }
    
    vector->creation_time = get_time();
    vector->blocks_accessed = 0;
    
    return vector;
}

int ooc_matrix_set_element(ooc_matrix_t *matrix, int row, int col, double complex value) {
    if (!matrix || row < 0 || row >= matrix->num_rows || col < 0 || col >= matrix->num_cols) {
        return -1;
    }
    
    // Calculate block indices
    int block_row = row / matrix->block_size;
    int block_col = col / matrix->block_size;
    int local_row = row % matrix->block_size;
    int local_col = col % matrix->block_size;
    
    // Get or create block
    ooc_matrix_block_t *block = &matrix->blocks[block_row][block_col];
    
    if (!matrix->block_exists[block_row][block_col]) {
        // Create new block
        int row_start = block_row * matrix->block_size;
        int row_end = (block_row + 1) * matrix->block_size;
        int col_start = block_col * matrix->block_size;
        int col_end = (block_col + 1) * matrix->block_size;
        
        if (row_end > matrix->num_rows) row_end = matrix->num_rows;
        if (col_end > matrix->num_cols) col_end = matrix->num_cols;
        
        ooc_matrix_block_t *new_block = create_matrix_block(matrix->solver, block_row, block_col,
                                                           row_start, row_end, col_start, col_end);
        if (!new_block) return -1;
        
        // Copy to matrix storage
        memcpy(block, new_block, sizeof(ooc_matrix_block_t));
        free(new_block);
        
        matrix->block_exists[block_row][block_col] = true;
        matrix->blocks_created++;
    } else if (!block->is_cached) {
        // Load from disk if not cached
        char *filename = generate_block_filename(matrix->solver->matrix_dir, "matrix",
                                               block_row, block_col, "blk");
        if (filename) {
            load_matrix_block_from_file(block, filename);
            free(filename);
        }
    }
    
    // Set element value
    if (block->is_sparse) {
        // For sparse blocks, we would need to handle dynamic insertion
        // For now, convert to dense if needed
        if (block->data.csr.nnz > 0) {
            // Convert sparse to dense
            int block_rows = block->row_end - block->row_start;
            int block_cols = block->col_end - block->col_start;
            double complex *dense_data = (double complex *)calloc(block_rows * block_cols, sizeof(double complex));
            
            if (dense_data) {
                // Convert CSR to dense
                for (int i = 0; i < block_rows; i++) {
                    for (int j = block->data.csr.row_ptr[i]; j < block->data.csr.row_ptr[i+1]; j++) {
                        int col = block->data.csr.col_idx[j];
                        dense_data[i * block_cols + col] = block->data.csr.values[j];
                    }
                }
                
                // Free sparse data
                free(block->data.csr.row_ptr);
                free(block->data.csr.col_idx);
                free(block->data.csr.values);
                
                // Switch to dense
                block->is_sparse = false;
                block->data.dense.data = dense_data;
            }
        }
    }
    
    if (!block->is_sparse) {
        int block_rows = block->row_end - block->row_start;
        block->data.dense.data[local_row * block_rows + local_col] = value;
        block->is_modified = true;
    }
    
    return 0;
}

int ooc_matrix_get_element(ooc_matrix_t *matrix, int row, int col, double complex *value) {
    if (!matrix || row < 0 || row >= matrix->num_rows || col < 0 || col >= matrix->num_cols || !value) {
        return -1;
    }
    
    // Calculate block indices
    int block_row = row / matrix->block_size;
    int block_col = col / matrix->block_size;
    int local_row = row % matrix->block_size;
    int local_col = col % matrix->block_size;
    
    // Check if block exists
    if (!matrix->block_exists[block_row][block_col]) {
        *value = 0.0 + 0.0 * I;
        return 0;
    }
    
    // Get block
    ooc_matrix_block_t *block = &matrix->blocks[block_row][block_col];
    
    if (!block->is_cached) {
        // Load from disk
        char *filename = generate_block_filename(matrix->solver->matrix_dir, "matrix",
                                               block_row, block_col, "blk");
        if (filename) {
            load_matrix_block_from_file(block, filename);
            free(filename);
        }
    }
    
    // Get element value
    if (block->is_sparse) {
        // Search in CSR format
        int block_rows = block->row_end - block->row_start;
        for (int j = block->data.csr.row_ptr[local_row]; j < block->data.csr.row_ptr[local_row+1]; j++) {
            if (block->data.csr.col_idx[j] == local_col) {
                *value = block->data.csr.values[j];
                return 0;
            }
        }
        *value = 0.0 + 0.0 * I;
    } else {
        int block_rows = block->row_end - block->row_start;
        *value = block->data.dense.data[local_row * block_rows + local_col];
    }
    
    matrix->blocks_accessed++;
    return 0;
}

int ooc_vector_set_element(ooc_vector_t *vector, int index, double complex value) {
    if (!vector || index < 0 || index >= vector->size) {
        return -1;
    }
    
    int block_idx = index / vector->block_size;
    int local_idx = index % vector->block_size;
    
    // Allocate block if needed
    if (!vector->blocks[block_idx]) {
        vector->blocks[block_idx] = (double complex *)calloc(vector->block_size, sizeof(double complex));
        if (!vector->blocks[block_idx]) return -1;
        vector->block_cached[block_idx] = true;
    }
    
    vector->blocks[block_idx][local_idx] = value;
    vector->block_modified[block_idx] = true;
    
    return 0;
}

int ooc_vector_get_element(ooc_vector_t *vector, int index, double complex *value) {
    if (!vector || index < 0 || index >= vector->size || !value) {
        return -1;
    }
    
    int block_idx = index / vector->block_size;
    int local_idx = index % vector->block_size;
    
    if (!vector->blocks[block_idx]) {
        *value = 0.0 + 0.0 * I;
        return 0;
    }
    
    *value = vector->blocks[block_idx][local_idx];
    vector->blocks_accessed++;
    
    return 0;
}

int ooc_matrix_vector_multiply(ooc_matrix_t *matrix, ooc_vector_t *x, ooc_vector_t *y) {
    if (!matrix || !x || !y || x->size != matrix->num_cols || y->size != matrix->num_rows) {
        return -1;
    }
    
    // Initialize result vector to zero
    for (int i = 0; i < y->size; i++) {
        ooc_vector_set_element(y, i, 0.0 + 0.0 * I);
    }
    
    // Perform block-wise multiplication
    for (int block_row = 0; block_row < matrix->num_block_rows; block_row++) {
        for (int block_col = 0; block_col < matrix->num_block_cols; block_col++) {
            if (!matrix->block_exists[block_row][block_col]) continue;
            
            ooc_matrix_block_t *block = &matrix->blocks[block_row][block_col];
            
            // Load block if not cached
            if (!block->is_cached) {
                char *filename = generate_block_filename(matrix->solver->matrix_dir, "matrix",
                                                       block_row, block_col, "blk");
                if (filename) {
                    load_matrix_block_from_file(block, filename);
                    free(filename);
                }
            }
            
            // Perform block multiplication
            int block_rows = block->row_end - block->row_start;
            int block_cols = block->col_end - block->col_start;
            
            if (block->is_sparse) {
                // Sparse matrix-vector multiplication
                for (int i = 0; i < block_rows; i++) {
                    double complex sum = 0.0 + 0.0 * I;
                    for (int j = block->data.csr.row_ptr[i]; j < block->data.csr.row_ptr[i+1]; j++) {
                        int col = block->data.csr.col_idx[j];
                        double complex x_val;
                        ooc_vector_get_element(x, block->col_start + col, &x_val);
                        sum += block->data.csr.values[j] * x_val;
                    }
                    
                    double complex y_val;
                    ooc_vector_get_element(y, block->row_start + i, &y_val);
                    ooc_vector_set_element(y, block->row_start + i, y_val + sum);
                }
            } else {
                // Dense matrix-vector multiplication
                for (int i = 0; i < block_rows; i++) {
                    double complex sum = 0.0 + 0.0 * I;
                    for (int j = 0; j < block_cols; j++) {
                        double complex x_val;
                        ooc_vector_get_element(x, block->col_start + j, &x_val);
                        sum += block->data.dense.data[i * block_rows + j] * x_val;
                    }
                    
                    double complex y_val;
                    ooc_vector_get_element(y, block->row_start + i, &y_val);
                    ooc_vector_set_element(y, block->row_start + i, y_val + sum);
                }
            }
        }
    }
    
    return 0;
}

int ooc_solver_solve(ooc_solver_t *solver, ooc_matrix_t *matrix, ooc_vector_t *rhs,
                    ooc_vector_t *solution, double tolerance, int max_iterations) {
    if (!solver || !matrix || !rhs || !solution) {
        return -1;
    }
    
    solver->current_iteration = 0;
    solver->converged = false;
    
    // Initialize solution to zero
    for (int i = 0; i < solution->size; i++) {
        ooc_vector_set_element(solution, i, 0.0 + 0.0 * I);
    }
    
    // Simple iterative solver (can be enhanced with GMRES, etc.)
    ooc_vector_t *residual = ooc_vector_create(solver, rhs->size);
    ooc_vector_t *temp = ooc_vector_create(solver, rhs->size);
    
    if (!residual || !temp) {
        ooc_vector_destroy(residual);
        ooc_vector_destroy(temp);
        return -1;
    }
    
    for (int iter = 0; iter < max_iterations; iter++) {
        solver->current_iteration = iter;
        
        // Compute residual: r = b - A*x
        ooc_matrix_vector_multiply(matrix, solution, residual);
        
        for (int i = 0; i < rhs->size; i++) {
            double complex rhs_val, residual_val;
            ooc_vector_get_element(rhs, i, &rhs_val);
            ooc_vector_get_element(residual, i, &residual_val);
            ooc_vector_set_element(residual, i, rhs_val - residual_val);
        }
        
        // Check convergence
        double residual_norm = 0.0;
        for (int i = 0; i < rhs->size; i++) {
            double complex val;
            ooc_vector_get_element(residual, i, &val);
            residual_norm += creal(val * conj(val));
        }
        residual_norm = sqrt(residual_norm);
        solver->current_residual = residual_norm;
        
        if (residual_norm < tolerance) {
            solver->converged = true;
            break;
        }
        
        // Simple update: x = x + alpha * r (Jacobi-like)
        double alpha = 0.1;  // Simple relaxation parameter
        for (int i = 0; i < solution->size; i++) {
            double complex sol_val, res_val;
            ooc_vector_get_element(solution, i, &sol_val);
            ooc_vector_get_element(residual, i, &res_val);
            ooc_vector_set_element(solution, i, sol_val + alpha * res_val);
        }
        
        // Create checkpoint if enabled
        if (solver->config.enable_checkpoints && 
            iter % solver->config.checkpoint_frequency == 0) {
            char checkpoint_name[64];
            snprintf(checkpoint_name, sizeof(checkpoint_name), "iter_%d", iter);
            ooc_solver_checkpoint(solver, checkpoint_name);
        }
    }
    
    ooc_vector_destroy(residual);
    ooc_vector_destroy(temp);
    
    return solver->converged ? 0 : -1;
}

int ooc_solver_checkpoint(ooc_solver_t *solver, const char *checkpoint_name) {
    if (!solver || !checkpoint_name) return -1;
    
    double checkpoint_start = get_time();
    
    // Create checkpoint file
    char checkpoint_file[512];
    snprintf(checkpoint_file, sizeof(checkpoint_file), "%s/%s.chk", solver->checkpoint_dir, checkpoint_name);
    
    FILE *file = fopen(checkpoint_file, "wb");
    if (!file) return -1;
    
    // Write solver state
    fwrite(&solver->current_iteration, sizeof(int), 1, file);
    fwrite(&solver->current_residual, sizeof(double), 1, file);
    fwrite(&solver->converged, sizeof(bool), 1, file);
    
    // Write residual history
    fwrite(&solver->residual_history_size, sizeof(int), 1, file);
    if (solver->residual_history_size > 0) {
        fwrite(solver->residual_history, sizeof(double), solver->residual_history_size, file);
    }
    
    fclose(file);
    
    solver->stats.checkpoints_created++;
    solver->stats.checkpoint_time += get_time() - checkpoint_start;
    
    return 0;
}

int ooc_solver_load_checkpoint(ooc_solver_t *solver, const char *checkpoint_name) {
    if (!solver || !checkpoint_name) return -1;
    
    // Open checkpoint file
    char checkpoint_file[512];
    snprintf(checkpoint_file, sizeof(checkpoint_file), "%s/%s.chk", solver->checkpoint_dir, checkpoint_name);
    
    FILE *file = fopen(checkpoint_file, "rb");
    if (!file) return -1;
    
    // Read solver state
    fread(&solver->current_iteration, sizeof(int), 1, file);
    fread(&solver->current_residual, sizeof(double), 1, file);
    fread(&solver->converged, sizeof(bool), 1, file);
    
    // Read residual history
    fread(&solver->residual_history_size, sizeof(int), 1, file);
    if (solver->residual_history_size > 0) {
        solver->residual_history = (double *)realloc(solver->residual_history, 
                                                    solver->residual_history_size * sizeof(double));
        fread(solver->residual_history, sizeof(double), solver->residual_history_size, file);
    }
    
    fclose(file);
    
    solver->stats.checkpoints_loaded++;
    
    return 0;
}

int ooc_solver_get_stats(ooc_solver_t *solver, ooc_solver_stats_t *stats) {
    if (!solver || !stats) return -1;
    
    memcpy(stats, &solver->stats, sizeof(ooc_solver_stats_t));
    
    // Calculate derived statistics
    int64_t total_cache_accesses = stats->cache_hits + stats->cache_misses;
    if (total_cache_accesses > 0) {
        stats->cache_hit_ratio = (double)stats->cache_hits / (double)total_cache_accesses;
    } else {
        stats->cache_hit_ratio = 0.0;
    }
    
    stats->total_time = get_time() - solver->start_time;
    
    return 0;
}

int ooc_solver_flush_cache(ooc_solver_t *solver) {
    if (!solver) return -1;
    
    pthread_mutex_lock(&solver->cache_mutex);
    
    // Flush all modified blocks to disk
    for (int i = 0; i < solver->cache_size; i++) {
        if (solver->cache[i].block && solver->cache[i].block->is_modified) {
            ooc_matrix_block_t *block = solver->cache[i].block;
            
            char *filename = generate_block_filename(solver->matrix_dir, "matrix",
                                                   block->block_row, block->block_col, "blk");
            if (filename) {
                save_matrix_block_to_file(block, filename);
                free(filename);
                block->is_modified = false;
            }
        }
    }
    
    pthread_mutex_unlock(&solver->cache_mutex);
    return 0;
}

int ooc_solver_clear_cache(ooc_solver_t *solver) {
    if (!solver) return -1;
    
    pthread_mutex_lock(&solver->cache_mutex);
    
    // Flush all modified blocks
    ooc_solver_flush_cache(solver);
    
    // Free all cached blocks
    for (int i = 0; i < solver->cache_size; i++) {
        if (solver->cache[i].block) {
            matrix_block_free(solver->cache[i].block);
            free(solver->cache[i].block);
            solver->cache[i].block = NULL;
        }
    }
    
    solver->cache_size = 0;
    
    pthread_mutex_unlock(&solver->cache_mutex);
    return 0;
}

void ooc_solver_get_default_config(ooc_solver_config_t *config) {
    if (!config) return;
    
    memset(config, 0, sizeof(ooc_solver_config_t));
    
    config->block_size = OOC_BLOCK_SIZE_MEDIUM;
    config->storage_format = OOC_FORMAT_HYBRID;
    
    config->max_memory_usage = 4ULL * 1024 * 1024 * 1024;  // 4GB
    config->block_cache_size = 64;  // Cache 64 blocks
    config->memory_utilization = 0.8;  // Use 80% of available memory
    
    config->io_parallelism = 4;  // 4 I/O threads
    config->io_buffer_size = 64 * 1024 * 1024;  // 64MB per thread
    config->use_async_io = true;
    config->use_memory_mapping = false;
    
    strcpy(config->matrix_directory, "/tmp/pulseem_ooc");
    strcpy(config->vector_directory, "/tmp/pulseem_ooc");
    strcpy(config->checkpoint_directory, "/tmp/pulseem_ooc");
    
    config->enable_checkpoints = true;
    config->checkpoint_frequency = 10;
    config->compress_checkpoints = false;
    
    config->prefetch_threshold = 0.7;
    config->prefetch_lookahead = 2;
    config->use_block_pool = true;
}

void ooc_solver_destroy(ooc_solver_t *solver) {
    if (!solver) return;
    
    // Flush and clear cache
    ooc_solver_flush_cache(solver);
    ooc_solver_clear_cache(solver);
    
    // Free cache
    free(solver->cache);
    pthread_mutex_destroy(&solver->cache_mutex);
    
    // Free working arrays
    free(solver->residual_history);
    if (solver->krylov_basis) {
        for (int i = 0; i < solver->krylov_dimension; i++) {
            free(solver->krylov_basis[i]);
        }
        free(solver->krylov_basis);
    }
    
    // Cleanup directories (optional - could be kept for debugging)
    // remove_directory(solver->matrix_dir);
    // remove_directory(solver->vector_dir);
    // remove_directory(solver->checkpoint_dir);
    
    free(solver);
}

void ooc_matrix_destroy(ooc_matrix_t *matrix) {
    if (!matrix) return;
    
    // Flush all blocks to disk
    for (int i = 0; i < matrix->num_block_rows; i++) {
        for (int j = 0; j < matrix->num_block_cols; j++) {
            if (matrix->block_exists[i][j]) {
                ooc_matrix_block_t *block = &matrix->blocks[i][j];
                if (block->is_cached && block->is_modified) {
                    char *filename = generate_block_filename(matrix->solver->matrix_dir, "matrix",
                                                           i, j, "blk");
                    if (filename) {
                        save_matrix_block_to_file(block, filename);
                        free(filename);
                    }
                }
                
                if (block->is_cached) {
                    matrix_block_free(block);
                }
            }
        }
        free(matrix->blocks[i]);
        free(matrix->block_exists[i]);
    }
    
    free(matrix->blocks);
    free(matrix->block_exists);
    free(matrix);
}

void ooc_vector_destroy(ooc_vector_t *vector) {
    if (!vector) return;
    
    // Save modified blocks to disk
    for (int i = 0; i < vector->num_blocks; i++) {
        if (vector->blocks[i] && vector->block_modified[i]) {
            // Save block to disk (implementation needed)
        }
        
        free(vector->blocks[i]);
        free(vector->block_filenames[i]);
        if (vector->block_files[i]) {
            fclose(vector->block_files[i]);
        }
    }
    
    free(vector->blocks);
    free(vector->block_cached);
    free(vector->block_modified);
    free(vector->block_files);
    free(vector->block_filenames);
    free(vector);
}