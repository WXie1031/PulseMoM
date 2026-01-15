/**
 * @file checkpoint_recovery.h
 * @brief Comprehensive checkpoint and recovery system for electromagnetic solvers
 * @details Robust state persistence for long-running simulations
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CHECKPOINT_RECOVERY_H
#define CHECKPOINT_RECOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include <complex.h>
#include <time.h>
#include "core_common.h"
#include "sparse_direct_solver.h"
#include "out_of_core_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

// Checkpoint version for compatibility
typedef enum {
    CHECKPOINT_VERSION_1_0 = 0x0100,
    CHECKPOINT_VERSION_1_1 = 0x0101,
    CHECKPOINT_VERSION_2_0 = 0x0200
} checkpoint_version_t;

// Checkpoint types
typedef enum {
    CHECKPOINT_TYPE_FULL,           // Complete solver state
    CHECKPOINT_TYPE_INCREMENTAL,    // Only changed data
    CHECKPOINT_TYPE_MEMORY_DUMP,    // Memory snapshot
    CHECKPOINT_TYPE_ITERATIVE,      // Iterative solver state only
    CHECKPOINT_TYPE_FACTORIZATION,  // Matrix factorization only
    CHECKPOINT_TYPE_SOLUTION        // Solution vectors only
} checkpoint_type_t;

// Checkpoint compression methods
typedef enum {
    CHECKPOINT_COMPRESS_NONE,       // No compression
    CHECKPOINT_COMPRESS_GZIP,       // GZIP compression
    CHECKPOINT_COMPRESS_BZIP2,      // BZIP2 compression
    CHECKPOINT_COMPRESS_LZ4,        // LZ4 compression (fast)
    CHECKPOINT_COMPRESS_ZSTD        // Zstandard compression
} checkpoint_compress_t;

// Checkpoint encryption methods
typedef enum {
    CHECKPOINT_ENCRYPT_NONE,        // No encryption
    CHECKPOINT_ENCRYPT_AES256,      // AES-256 encryption
    CHECKPOINT_ENCRYPT_CHACHA20     // ChaCha20 encryption
} checkpoint_encrypt_t;

// Solver state information
typedef struct {
    // Basic information
    checkpoint_version_t version;
    checkpoint_type_t type;
    char solver_name[64];           // Solver name/type
    char simulation_id[128];        // Unique simulation identifier
    
    // Timing information
    time_t creation_time;
    double simulation_time;         // Simulation time in seconds
    double wall_clock_time;         // Wall clock time in seconds
    int iteration_count;            // Current iteration number
    
    // Convergence information
    double current_residual;        // Current residual norm
    double initial_residual;        // Initial residual norm
    double convergence_tolerance;   // Convergence tolerance
    bool converged;                 // Convergence status
    
    // Matrix information
    int matrix_size;                // Matrix dimension
    int64_t matrix_nnz;             // Number of non-zeros
    char matrix_type[32];           // Matrix type (dense, sparse, etc.)
    
    // Memory usage
    size_t memory_usage;            // Current memory usage in bytes
    size_t peak_memory_usage;       // Peak memory usage in bytes
    
    // Solver-specific flags
    bool factorization_complete;    // Matrix factorized
    bool analysis_complete;          // Matrix analyzed
    bool preprocessing_complete;     // Preprocessing done
    
} checkpoint_header_t;

// Checkpoint configuration
typedef struct {
    // Basic settings
    checkpoint_type_t checkpoint_type;
    checkpoint_compress_t compression;
    checkpoint_encrypt_t encryption;
    
    // Timing settings
    int checkpoint_interval;        // Checkpoint every N iterations
    double checkpoint_time_interval; // Checkpoint every N seconds
    bool checkpoint_on_convergence;  // Create checkpoint when converged
    bool checkpoint_on_error;        // Create checkpoint on error
    
    // Storage settings
    char checkpoint_directory[512];   // Checkpoint storage directory
    int max_checkpoints;             // Maximum number of checkpoints to keep
    bool auto_cleanup;               // Automatically clean old checkpoints
    
    // Performance settings
    bool use_async_checkpoint;       // Use asynchronous checkpointing
    int compression_level;           // Compression level (1-9)
    bool verify_integrity;           // Verify checkpoint integrity
    
    // Advanced settings
    bool incremental_checkpoints;    // Use incremental checkpoints
    bool differential_checkpoints;   // Store only changes
    bool parallel_checkpoint;        // Use parallel checkpointing
    int num_checkpoint_threads;      // Number of checkpoint threads
    
    // Recovery settings
    bool auto_recovery;              // Automatically recover from checkpoints
    int max_recovery_attempts;       // Maximum recovery attempts
    bool validate_on_load;           // Validate checkpoint on load
    
} checkpoint_config_t;

// Checkpoint handle
typedef struct checkpoint_manager checkpoint_manager_t;

// Checkpoint information
typedef struct {
    char filename[256];              // Checkpoint filename
    checkpoint_header_t header;      // Checkpoint header
    size_t file_size;               // File size in bytes
    bool is_valid;                   // Validity flag
    bool is_corrupted;               // Corruption flag
    char checksum[64];               // File checksum
} checkpoint_info_t;

// Recovery statistics
typedef struct {
    int successful_recoveries;        // Number of successful recoveries
    int failed_recoveries;           // Number of failed recoveries
    int partial_recoveries;          // Number of partial recoveries
    double total_recovery_time;        // Total recovery time
    double average_recovery_time;      // Average recovery time
    size_t total_data_recovered;      // Total data recovered
    int checkpoints_used;              // Number of checkpoints used
} recovery_stats_t;

// Function prototypes

/**
 * @brief Initialize checkpoint manager
 * @param config Checkpoint configuration
 * @return Checkpoint manager handle or NULL on failure
 */
checkpoint_manager_t* checkpoint_manager_init(const checkpoint_config_t *config);

/**
 * @brief Create checkpoint of solver state
 * @param manager Checkpoint manager
 * @param solver_data Solver state data
 * @param checkpoint_name Checkpoint name (optional)
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_create(checkpoint_manager_t *manager, void *solver_data, const char *checkpoint_name);

/**
 * @brief Load solver state from checkpoint
 * @param manager Checkpoint manager
 * @param checkpoint_name Checkpoint name or NULL for latest
 * @param solver_data Output solver state data
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_load(checkpoint_manager_t *manager, const char *checkpoint_name, void *solver_data);

/**
 * @brief List available checkpoints
 * @param manager Checkpoint manager
 * @param checkpoints Output checkpoint list
 * @param max_checkpoints Maximum number of checkpoints to return
 * @return Number of checkpoints found, -1 on error
 */
int checkpoint_manager_list(checkpoint_manager_t *manager, checkpoint_info_t *checkpoints, int max_checkpoints);

/**
 * @brief Verify checkpoint integrity
 * @param manager Checkpoint manager
 * @param checkpoint_name Checkpoint name
 * @return true if valid, false otherwise
 */
bool checkpoint_manager_verify(checkpoint_manager_t *manager, const char *checkpoint_name);

/**
 * @brief Delete checkpoint
 * @param manager Checkpoint manager
 * @param checkpoint_name Checkpoint name
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_delete(checkpoint_manager_t *manager, const char *checkpoint_name);

/**
 * @brief Cleanup old checkpoints
 * @param manager Checkpoint manager
 * @param keep_count Number of recent checkpoints to keep
 * @return Number of checkpoints deleted, -1 on error
 */
int checkpoint_manager_cleanup(checkpoint_manager_t *manager, int keep_count);

/**
 * @brief Get checkpoint statistics
 * @param manager Checkpoint manager
 * @param stats Output statistics
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_get_stats(checkpoint_manager_t *manager, recovery_stats_t *stats);

/**
 * @brief Create incremental checkpoint
 * @param manager Checkpoint manager
 * @param base_checkpoint_name Base checkpoint name
 * @param incremental_name Incremental checkpoint name
 * @param solver_data Solver state data
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_create_incremental(checkpoint_manager_t *manager, const char *base_checkpoint_name,
                                        const char *incremental_name, void *solver_data);

/**
 * @brief Merge incremental checkpoints
 * @param manager Checkpoint manager
 * @param base_checkpoint Base checkpoint name
 * @param incremental_checkpoint Incremental checkpoint name
 * @param merged_name Merged checkpoint name
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_merge_incremental(checkpoint_manager_t *manager, const char *base_checkpoint,
                                        const char *incremental_checkpoint, const char *merged_name);

/**
 * @brief Get latest checkpoint name
 * @param manager Checkpoint manager
 * @param checkpoint_name Output checkpoint name
 * @param max_length Maximum name length
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_get_latest(checkpoint_manager_t *manager, char *checkpoint_name, int max_length);

/**
 * @brief Set checkpoint callback function
 * @param manager Checkpoint manager
 * @param callback Callback function (called before checkpoint creation)
 * @param user_data User data for callback
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_set_callback(checkpoint_manager_t *manager, 
                                   int (*callback)(void *user_data), void *user_data);

/**
 * @brief Pause checkpoint creation
 * @param manager Checkpoint manager
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_pause(checkpoint_manager_t *manager);

/**
 * @brief Resume checkpoint creation
 * @param manager Checkpoint manager
 * @return 0 on success, error code on failure
 */
int checkpoint_manager_resume(checkpoint_manager_t *manager);

/**
 * @brief Get default configuration
 * @param config Output configuration
 */
void checkpoint_manager_get_default_config(checkpoint_config_t *config);

/**
 * @brief Cleanup checkpoint manager resources
 * @param manager Checkpoint manager
 */
void checkpoint_manager_destroy(checkpoint_manager_t *manager);

// Specialized checkpoint functions for different solver types

/**
 * @brief Create checkpoint for sparse direct solver
 * @param manager Checkpoint manager
 * @param sparse_solver Sparse direct solver
 * @param checkpoint_name Checkpoint name
 * @return 0 on success, error code on failure
 */
int checkpoint_sparse_direct_solver(checkpoint_manager_t *manager, 
                                   sparse_direct_solver_t *sparse_solver, const char *checkpoint_name);

/**
 * @brief Create checkpoint for out-of-core solver
 * @param manager Checkpoint manager
 * @param ooc_solver Out-of-core solver
 * @param checkpoint_name Checkpoint name
 * @return 0 on success, error code on failure
 */
int checkpoint_out_of_core_solver(checkpoint_manager_t *manager, 
                                ooc_solver_t *ooc_solver, const char *checkpoint_name);

/**
 * @brief Create checkpoint for iterative solver
 * @param manager Checkpoint manager
 * @param matrix_size Matrix dimension
 * @param solution Current solution vector
 * @param residual Current residual vector
 * @param krylov_basis Krylov subspace basis (optional)
 * @param krylov_dim Krylov subspace dimension
 * @param checkpoint_name Checkpoint name
 * @return 0 on success, error code on failure
 */
int checkpoint_iterative_solver(checkpoint_manager_t *manager, int matrix_size,
                               double complex *solution, double complex *residual,
                               double complex **krylov_basis, int krylov_dim,
                               const char *checkpoint_name);

/**
 * @brief Create checkpoint for factorization
 * @param manager Checkpoint manager
 * @param factors Matrix factors (L and U)
 * @param permutation Permutation vectors
 * @param matrix_size Matrix dimension
 * @param checkpoint_name Checkpoint name
 * @return 0 on success, error code on failure
 */
int checkpoint_factorization(checkpoint_manager_t *manager, void *factors, int *permutation,
                           int matrix_size, const char *checkpoint_name);

// Recovery functions

/**
 * @brief Recover sparse direct solver from checkpoint
 * @param manager Checkpoint manager
 * @param checkpoint_name Checkpoint name
 * @param sparse_solver Output sparse direct solver
 * @return 0 on success, error code on failure
 */
int recover_sparse_direct_solver(checkpoint_manager_t *manager, const char *checkpoint_name,
                                sparse_direct_solver_t **sparse_solver);

/**
 * @brief Recover out-of-core solver from checkpoint
 * @param manager Checkpoint manager
 * @param checkpoint_name Checkpoint name
 * @param ooc_solver Output out-of-core solver
 * @return 0 on success, error code on failure
 */
int recover_out_of_core_solver(checkpoint_manager_t *manager, const char *checkpoint_name,
                              ooc_solver_t **ooc_solver);

/**
 * @brief Recover iterative solver from checkpoint
 * @param manager Checkpoint manager
 * @param checkpoint_name Checkpoint name
 * @param matrix_size Output matrix dimension
 * @param solution Output solution vector (pre-allocated)
 * @param residual Output residual vector (pre-allocated)
 * @param krylov_basis Output Krylov subspace basis (optional)
 * @param max_krylov_dim Maximum Krylov dimension
 * @return 0 on success, error code on failure
 */
int recover_iterative_solver(checkpoint_manager_t *manager, const char *checkpoint_name,
                            int *matrix_size, double complex *solution, double complex *residual,
                            double complex **krylov_basis, int max_krylov_dim);

/**
 * @brief Recover factorization from checkpoint
 * @param manager Checkpoint manager
 * @param checkpoint_name Checkpoint name
 * @param factors Output matrix factors
 * @param permutation Output permutation vectors
 * @param matrix_size Output matrix dimension
 * @return 0 on success, error code on failure
 */
int recover_factorization(checkpoint_manager_t *manager, const char *checkpoint_name,
                         void **factors, int **permutation, int *matrix_size);

#ifdef __cplusplus
}
#endif

#endif // CHECKPOINT_RECOVERY_H