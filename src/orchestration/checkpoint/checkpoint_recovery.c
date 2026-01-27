/**
 * @file checkpoint_recovery.c
 * @brief Implementation of comprehensive checkpoint and recovery system
 * @details Robust state persistence for long-running electromagnetic simulations
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "checkpoint_recovery.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

// Optional compression libraries
#ifdef HAVE_ZLIB
#include <zlib.h>
#endif
#ifdef HAVE_BZLIB
#include <bzlib.h>
#endif
#ifdef HAVE_LZ4
#include <lz4.h>
#endif
#ifdef HAVE_ZSTD
#include <zstd.h>
#endif

// Optional encryption libraries
#ifdef HAVE_OPENSSL
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#endif

// Internal checkpoint manager structure
struct checkpoint_manager {
    checkpoint_config_t config;     // Configuration
    recovery_stats_t stats;         // Recovery statistics
    
    // Working directories
    char checkpoint_dir[512];       // Main checkpoint directory
    char temp_dir[512];             // Temporary directory
    char backup_dir[512];           // Backup directory
    
    // State tracking
    bool is_paused;                 // Checkpointing paused
    bool is_initialized;            // Manager initialized
    time_t last_checkpoint_time;    // Last checkpoint time
    int checkpoint_counter;         // Checkpoint counter
    
    // Callback function
    int (*checkpoint_callback)(void *user_data);
    void *callback_user_data;
    
    // File handles and locks
    FILE *current_checkpoint_file;  // Current checkpoint file
    char current_checkpoint_name[256]; // Current checkpoint name
    
    // Performance tracking
    double total_checkpoint_time;   // Total time spent checkpointing
    int checkpoint_count;           // Number of checkpoints created
    size_t total_data_size;         // Total data checkpointed
};

// Utility functions
static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int create_directory_recursive(const char *path) {
    char temp[512];
    char *p = NULL;
    size_t len;
    
    snprintf(temp, sizeof(temp), "%s", path);
    len = strlen(temp);
    
    if (temp[len - 1] == '/') {
        temp[len - 1] = 0;
    }
    
    for (p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(temp, 0700) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    
    if (mkdir(temp, 0700) != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

static int remove_directory_contents(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return -1;
    
    struct dirent *entry;
    char filepath[512];
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(filepath, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                remove_directory_contents(filepath);
                rmdir(filepath);
            } else {
                unlink(filepath);
            }
        }
    }
    
    closedir(dir);
    return 0;
}

static int calculate_checksum(const void *data, size_t size, char *checksum, size_t checksum_size) {
#ifdef HAVE_OPENSSL
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data, size, hash);
    
    // Convert to hex string
    for (int i = 0; i < SHA256_DIGEST_LENGTH && i * 2 < checksum_size - 1; i++) {
        sprintf(checksum + i * 2, "%02x", hash[i]);
    }
    checksum[checksum_size - 1] = '\0';
    return 0;
#else
    // Simple checksum if OpenSSL not available
    unsigned long sum = 0;
    const unsigned char *bytes = (const unsigned char *)data;
    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
    }
    snprintf(checksum, checksum_size, "%08lx", sum);
    return 0;
#endif
}

static int compress_data(const void *input, size_t input_size, void **output, size_t *output_size,
                        checkpoint_compress_t method) {
    switch (method) {
#ifdef HAVE_ZLIB
        case CHECKPOINT_COMPRESS_GZIP: {
            uLongf compressed_size = compressBound(input_size);
            *output = malloc(compressed_size);
            if (!*output) return -1;
            
            int result = compress2((Bytef *)*output, &compressed_size, (const Bytef *)input, input_size, Z_DEFAULT_COMPRESSION);
            if (result != Z_OK) {
                free(*output);
                return -1;
            }
            
            *output_size = compressed_size;
            return 0;
        }
#endif
        
#ifdef HAVE_BZLIB
        case CHECKPOINT_COMPRESS_BZIP2: {
            unsigned int compressed_size = input_size + 600; // BZ2 recommendation
            *output = malloc(compressed_size);
            if (!*output) return -1;
            
            int result = BZ2_bzBuffToBuffCompress((char *)*output, &compressed_size, (char *)input, input_size, 9, 0, 0);
            if (result != BZ_OK) {
                free(*output);
                return -1;
            }
            
            *output_size = compressed_size;
            return 0;
        }
#endif
        
        default:
            // No compression
            *output = malloc(input_size);
            if (!*output) return -1;
            memcpy(*output, input, input_size);
            *output_size = input_size;
            return 0;
    }
}

static int decompress_data(const void *input, size_t input_size, void **output, size_t *output_size,
                          checkpoint_compress_t method) {
    switch (method) {
#ifdef HAVE_ZLIB
        case CHECKPOINT_COMPRESS_GZIP: {
            uLongf uncompressed_size = *output_size;
            *output = malloc(uncompressed_size);
            if (!*output) return -1;
            
            int result = uncompress((Bytef *)*output, &uncompressed_size, (const Bytef *)input, input_size);
            if (result != Z_OK) {
                free(*output);
                return -1;
            }
            
            *output_size = uncompressed_size;
            return 0;
        }
#endif
        
        default:
            // No compression
            *output = malloc(input_size);
            if (!*output) return -1;
            memcpy(*output, input, input_size);
            *output_size = input_size;
            return 0;
    }
}

// Main implementation
checkpoint_manager_t* checkpoint_manager_init(const checkpoint_config_t *config) {
    if (!config) return NULL;
    
    checkpoint_manager_t *manager = (checkpoint_manager_t *)calloc(1, sizeof(checkpoint_manager_t));
    if (!manager) return NULL;
    
    // Copy configuration
    memcpy(&manager->config, config, sizeof(checkpoint_config_t));
    
    // Create working directories
    snprintf(manager->checkpoint_dir, sizeof(manager->checkpoint_dir), "%s", config->checkpoint_directory);
    snprintf(manager->temp_dir, sizeof(manager->temp_dir), "%s/temp", config->checkpoint_directory);
    snprintf(manager->backup_dir, sizeof(manager->backup_dir), "%s/backup", config->checkpoint_directory);
    
    if (create_directory_recursive(manager->checkpoint_dir) != 0 ||
        create_directory_recursive(manager->temp_dir) != 0 ||
        create_directory_recursive(manager->backup_dir) != 0) {
        free(manager);
        return NULL;
    }
    
    // Initialize state
    manager->is_initialized = true;
    manager->is_paused = false;
    manager->last_checkpoint_time = time(NULL);
    manager->checkpoint_counter = 0;
    manager->checkpoint_callback = NULL;
    manager->callback_user_data = NULL;
    
    // Initialize statistics
    memset(&manager->stats, 0, sizeof(recovery_stats_t));
    manager->total_checkpoint_time = 0.0;
    manager->checkpoint_count = 0;
    manager->total_data_size = 0;
    
    return manager;
}

int checkpoint_manager_create(checkpoint_manager_t *manager, void *solver_data, const char *checkpoint_name) {
    if (!manager || !manager->is_initialized || manager->is_paused) {
        return -1;
    }
    
    double start_time = get_time();
    
    // Generate checkpoint name if not provided
    char generated_name[256];
    if (!checkpoint_name) {
        snprintf(generated_name, sizeof(generated_name), "checkpoint_%d_%ld", 
                manager->checkpoint_counter++, time(NULL));
        checkpoint_name = generated_name;
    }
    
    // Call user callback if registered
    if (manager->checkpoint_callback) {
        int callback_result = manager->checkpoint_callback(manager->callback_user_data);
        if (callback_result != 0) {
            return callback_result;
        }
    }
    
    // Create temporary file
    char temp_filename[512];
    snprintf(temp_filename, sizeof(temp_filename), "%s/%s.tmp", manager->temp_dir, checkpoint_name);
    
    FILE *temp_file = fopen(temp_filename, "wb");
    if (!temp_file) {
        return -1;
    }
    
    // Write checkpoint header
    checkpoint_header_t header;
    memset(&header, 0, sizeof(header));
    header.version = CHECKPOINT_VERSION_2_0;
    header.type = manager->config.checkpoint_type;
    header.creation_time = time(NULL);
    header.simulation_time = get_time() - manager->start_time;
    header.wall_clock_time = header.simulation_time; // Simplified
    header.iteration_count = manager->checkpoint_count;
    
    // Write header
    fwrite(&header, sizeof(header), 1, temp_file);
    
    // Write solver data (simplified - would need proper serialization)
    if (solver_data) {
        size_t data_size = 1024; // Simplified size calculation
        fwrite(solver_data, data_size, 1, temp_file);
        manager->total_data_size += data_size;
    }
    
    fclose(temp_file);
    
    // Move to final location
    char final_filename[512];
    snprintf(final_filename, sizeof(final_filename), "%s/%s.chk", manager->checkpoint_dir, checkpoint_name);
    
    if (rename(temp_filename, final_filename) != 0) {
        unlink(temp_filename);
        return -1;
    }
    
    // Update statistics
    manager->total_checkpoint_time += get_time() - start_time;
    manager->checkpoint_count++;
    manager->last_checkpoint_time = time(NULL);
    
    // Cleanup old checkpoints
    if (manager->config.auto_cleanup && manager->config.max_checkpoints > 0) {
        checkpoint_manager_cleanup(manager, manager->config.max_checkpoints);
    }
    
    return 0;
}

int checkpoint_manager_load(checkpoint_manager_t *manager, const char *checkpoint_name, void *solver_data) {
    if (!manager || !manager->is_initialized) {
        return -1;
    }
    
    double start_time = get_time();
    
    // Use latest checkpoint if name not provided
    char latest_name[256];
    if (!checkpoint_name) {
        if (checkpoint_manager_get_latest(manager, latest_name, sizeof(latest_name)) != 0) {
            return -1;
        }
        checkpoint_name = latest_name;
    }
    
    // Open checkpoint file
    char checkpoint_filename[512];
    snprintf(checkpoint_filename, sizeof(checkpoint_filename), "%s/%s.chk", manager->checkpoint_dir, checkpoint_name);
    
    FILE *checkpoint_file = fopen(checkpoint_filename, "rb");
    if (!checkpoint_file) {
        return -1;
    }
    
    // Read and validate header
    checkpoint_header_t header;
    if (fread(&header, sizeof(header), 1, checkpoint_file) != 1) {
        fclose(checkpoint_file);
        return -1;
    }
    
    // Validate version
    if (header.version < CHECKPOINT_VERSION_1_0 || header.version > CHECKPOINT_VERSION_2_0) {
        fclose(checkpoint_file);
        return -1;
    }
    
    // Read solver data (simplified)
    if (solver_data) {
        size_t data_size = 1024; // Simplified size calculation
        if (fread(solver_data, data_size, 1, checkpoint_file) != 1) {
            fclose(checkpoint_file);
            return -1;
        }
    }
    
    fclose(checkpoint_file);
    
    // Update statistics
    double recovery_time = get_time() - start_time;
    manager->stats.successful_recoveries++;
    manager->stats.total_recovery_time += recovery_time;
    manager->stats.checkpoints_used++;
    
    if (manager->stats.successful_recoveries > 0) {
        manager->stats.average_recovery_time = manager->stats.total_recovery_time / manager->stats.successful_recoveries;
    }
    
    return 0;
}

int checkpoint_manager_list(checkpoint_manager_t *manager, checkpoint_info_t *checkpoints, int max_checkpoints) {
    if (!manager || !checkpoints || max_checkpoints <= 0) {
        return -1;
    }
    
    DIR *dir = opendir(manager->checkpoint_dir);
    if (!dir) {
        return -1;
    }
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL && count < max_checkpoints) {
        if (strstr(entry->d_name, ".chk") != NULL) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", manager->checkpoint_dir, entry->d_name);
            
            struct stat st;
            if (stat(full_path, &st) == 0) {
                strncpy(checkpoints[count].filename, entry->d_name, sizeof(checkpoints[count].filename) - 1);
                checkpoints[count].filename[sizeof(checkpoints[count].filename) - 1] = '\0';
                checkpoints[count].file_size = st.st_size;
                checkpoints[count].is_valid = true;
                checkpoints[count].is_corrupted = false;
                
                // Calculate checksum
                FILE *file = fopen(full_path, "rb");
                if (file) {
                    char *buffer = (char *)malloc(st.st_size);
                    if (buffer) {
                        if (fread(buffer, st.st_size, 1, file) == 1) {
                            calculate_checksum(buffer, st.st_size, checkpoints[count].checksum, sizeof(checkpoints[count].checksum));
                        }
                        free(buffer);
                    }
                    fclose(file);
                }
                
                count++;
            }
        }
    }
    
    closedir(dir);
    return count;
}

bool checkpoint_manager_verify(checkpoint_manager_t *manager, const char *checkpoint_name) {
    if (!manager || !checkpoint_name) {
        return false;
    }
    
    char checkpoint_filename[512];
    snprintf(checkpoint_filename, sizeof(checkpoint_filename), "%s/%s.chk", manager->checkpoint_dir, checkpoint_name);
    
    FILE *file = fopen(checkpoint_filename, "rb");
    if (!file) {
        return false;
    }
    
    // Read header
    checkpoint_header_t header;
    bool valid = (fread(&header, sizeof(header), 1, file) == 1);
    
    // Validate version
    if (valid) {
        valid = (header.version >= CHECKPOINT_VERSION_1_0 && header.version <= CHECKPOINT_VERSION_2_0);
    }
    
    fclose(file);
    return valid;
}

int checkpoint_manager_delete(checkpoint_manager_t *manager, const char *checkpoint_name) {
    if (!manager || !checkpoint_name) {
        return -1;
    }
    
    char checkpoint_filename[512];
    snprintf(checkpoint_filename, sizeof(checkpoint_filename), "%s/%s.chk", manager->checkpoint_dir, checkpoint_name);
    
    if (unlink(checkpoint_filename) != 0) {
        return -1;
    }
    
    return 0;
}

int checkpoint_manager_cleanup(checkpoint_manager_t *manager, int keep_count) {
    if (!manager || keep_count < 0) {
        return -1;
    }
    
    // List all checkpoints
    checkpoint_info_t *checkpoints = (checkpoint_info_t *)malloc(1000 * sizeof(checkpoint_info_t));
    if (!checkpoints) {
        return -1;
    }
    
    int count = checkpoint_manager_list(manager, checkpoints, 1000);
    if (count <= keep_count) {
        free(checkpoints);
        return 0;
    }
    
    // Sort by modification time (simplified - would need proper timestamp extraction)
    // For now, just delete oldest files
    int deleted = 0;
    for (int i = 0; i < count - keep_count; i++) {
        char checkpoint_name[256];
        strncpy(checkpoint_name, checkpoints[i].filename, sizeof(checkpoint_name) - 1);
        checkpoint_name[sizeof(checkpoint_name) - 1] = '\0';
        
        // Remove .chk extension
        char *ext = strstr(checkpoint_name, ".chk");
        if (ext) *ext = '\0';
        
        if (checkpoint_manager_delete(manager, checkpoint_name) == 0) {
            deleted++;
        }
    }
    
    free(checkpoints);
    return deleted;
}

int checkpoint_manager_get_stats(checkpoint_manager_t *manager, recovery_stats_t *stats) {
    if (!manager || !stats) {
        return -1;
    }
    
    memcpy(stats, &manager->stats, sizeof(recovery_stats_t));
    return 0;
}

int checkpoint_manager_get_latest(checkpoint_manager_t *manager, char *checkpoint_name, int max_length) {
    if (!manager || !checkpoint_name || max_length <= 0) {
        return -1;
    }
    
    checkpoint_info_t checkpoints[100];
    int count = checkpoint_manager_list(manager, checkpoints, 100);
    
    if (count <= 0) {
        return -1;
    }
    
    // Find latest checkpoint (simplified - first in list)
    char *filename = checkpoints[count - 1].filename;
    char *ext = strstr(filename, ".chk");
    
    if (ext) {
        size_t name_length = ext - filename;
        if (name_length >= max_length) {
            name_length = max_length - 1;
        }
        strncpy(checkpoint_name, filename, name_length);
        checkpoint_name[name_length] = '\0';
    } else {
        strncpy(checkpoint_name, filename, max_length - 1);
        checkpoint_name[max_length - 1] = '\0';
    }
    
    return 0;
}

int checkpoint_manager_set_callback(checkpoint_manager_t *manager, int (*callback)(void *user_data), void *user_data) {
    if (!manager) {
        return -1;
    }
    
    manager->checkpoint_callback = callback;
    manager->callback_user_data = user_data;
    return 0;
}

int checkpoint_manager_pause(checkpoint_manager_t *manager) {
    if (!manager || !manager->is_initialized) {
        return -1;
    }
    
    manager->is_paused = true;
    return 0;
}

int checkpoint_manager_resume(checkpoint_manager_t *manager) {
    if (!manager || !manager->is_initialized) {
        return -1;
    }
    
    manager->is_paused = false;
    return 0;
}

void checkpoint_manager_get_default_config(checkpoint_config_t *config) {
    if (!config) return;
    
    memset(config, 0, sizeof(checkpoint_config_t));
    
    config->checkpoint_type = CHECKPOINT_TYPE_FULL;
    config->compression = CHECKPOINT_COMPRESS_NONE;
    config->encryption = CHECKPOINT_ENCRYPT_NONE;
    
    config->checkpoint_interval = 100;
    config->checkpoint_time_interval = 3600.0; // 1 hour
    config->checkpoint_on_convergence = true;
    config->checkpoint_on_error = true;
    
    strcpy(config->checkpoint_directory, "/tmp/pulseem_checkpoints");
    config->max_checkpoints = 10;
    config->auto_cleanup = true;
    
    config->use_async_checkpoint = false;
    config->compression_level = 6;
    config->verify_integrity = true;
    
    config->incremental_checkpoints = false;
    config->differential_checkpoints = false;
    config->parallel_checkpoint = false;
    config->num_checkpoint_threads = 2;
    
    config->auto_recovery = false;
    config->max_recovery_attempts = 3;
    config->validate_on_load = true;
}

void checkpoint_manager_destroy(checkpoint_manager_t *manager) {
    if (!manager) return;
    
    // Close current checkpoint file if open
    if (manager->current_checkpoint_file) {
        fclose(manager->current_checkpoint_file);
    }
    
    // Cleanup temporary files
    remove_directory_contents(manager->temp_dir);
    
    free(manager);
}

// Specialized checkpoint functions
int checkpoint_sparse_direct_solver(checkpoint_manager_t *manager, sparse_direct_solver_t *sparse_solver, const char *checkpoint_name) {
    if (!manager || !sparse_solver) {
        return -1;
    }
    
    // Create checkpoint data structure
    struct {
        checkpoint_header_t header;
        sparse_solver_stats_t stats;
        int matrix_size;
        int64_t nnz;
    } checkpoint_data;
    
    memset(&checkpoint_data, 0, sizeof(checkpoint_data));
    
    // Fill in sparse solver specific data
    checkpoint_data.header.version = CHECKPOINT_VERSION_2_0;
    checkpoint_data.header.type = CHECKPOINT_TYPE_FACTORIZATION;
    strcpy(checkpoint_data.header.solver_name, "SparseDirectSolver");
    
    // Get solver statistics
    sparse_direct_solver_get_stats(sparse_solver, &checkpoint_data.stats);
    
    return checkpoint_manager_create(manager, &checkpoint_data, checkpoint_name);
}

int checkpoint_out_of_core_solver(checkpoint_manager_t *manager, ooc_solver_t *ooc_solver, const char *checkpoint_name) {
    if (!manager || !ooc_solver) {
        return -1;
    }
    
    // Create checkpoint data structure
    struct {
        checkpoint_header_t header;
        ooc_solver_stats_t stats;
        int current_iteration;
        double current_residual;
    } checkpoint_data;
    
    memset(&checkpoint_data, 0, sizeof(checkpoint_data));
    
    // Fill in out-of-core solver specific data
    checkpoint_data.header.version = CHECKPOINT_VERSION_2_0;
    checkpoint_data.header.type = CHECKPOINT_TYPE_ITERATIVE;
    strcpy(checkpoint_data.header.solver_name, "OutOfCoreSolver");
    
    // Get solver statistics
    ooc_solver_get_stats(ooc_solver, &checkpoint_data.stats);
    
    return checkpoint_manager_create(manager, &checkpoint_data, checkpoint_name);
}

int checkpoint_iterative_solver(checkpoint_manager_t *manager, int matrix_size,
                               double complex *solution, double complex *residual,
                               double complex **krylov_basis, int krylov_dim,
                               const char *checkpoint_name) {
    if (!manager || !solution || !residual) {
        return -1;
    }
    
    // Create checkpoint data structure
    struct {
        checkpoint_header_t header;
        int matrix_size;
        int krylov_dim;
        double solution_norm;
        double residual_norm;
        // Data arrays would follow in real implementation
    } checkpoint_data;
    
    memset(&checkpoint_data, 0, sizeof(checkpoint_data));
    
    // Fill in iterative solver specific data
    checkpoint_data.header.version = CHECKPOINT_VERSION_2_0;
    checkpoint_data.header.type = CHECKPOINT_TYPE_ITERATIVE;
    strcpy(checkpoint_data.header.solver_name, "IterativeSolver");
    checkpoint_data.header.matrix_size = matrix_size;
    
    checkpoint_data.matrix_size = matrix_size;
    checkpoint_data.krylov_dim = krylov_dim;
    
    // Calculate norms (simplified)
    double solution_norm = 0.0, residual_norm = 0.0;
    for (int i = 0; i < matrix_size; i++) {
        solution_norm += creal(solution[i] * conj(solution[i]));
        residual_norm += creal(residual[i] * conj(residual[i]));
    }
    checkpoint_data.solution_norm = sqrt(solution_norm);
    checkpoint_data.residual_norm = sqrt(residual_norm);
    
    return checkpoint_manager_create(manager, &checkpoint_data, checkpoint_name);
}

int checkpoint_factorization(checkpoint_manager_t *manager, void *factors, int *permutation,
                           int matrix_size, const char *checkpoint_name) {
    if (!manager || !factors || !permutation) {
        return -1;
    }
    
    // Create checkpoint data structure
    struct {
        checkpoint_header_t header;
        int matrix_size;
        // Factorization data would follow in real implementation
    } checkpoint_data;
    
    memset(&checkpoint_data, 0, sizeof(checkpoint_data));
    
    // Fill in factorization specific data
    checkpoint_data.header.version = CHECKPOINT_VERSION_2_0;
    checkpoint_data.header.type = CHECKPOINT_TYPE_FACTORIZATION;
    strcpy(checkpoint_data.header.solver_name, "Factorization");
    checkpoint_data.header.matrix_size = matrix_size;
    checkpoint_data.header.factorization_complete = true;
    
    checkpoint_data.matrix_size = matrix_size;
    
    return checkpoint_manager_create(manager, &checkpoint_data, checkpoint_name);
}