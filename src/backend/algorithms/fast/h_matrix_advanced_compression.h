#ifndef H_MATRIX_ADVANCED_COMPRESSION_H
#define H_MATRIX_ADVANCED_COMPRESSION_H

#include <stdbool.h>
#include "layered_greens_function.h"
#include "basis_functions.h"

// Advanced compression parameters
#define ACA_PLUS_ENHANCED     // Use enhanced ACA+ algorithm
#define H2_MATRIX_COMPRESSION  // Use H² matrix compression
#define PARALLEL_COMPRESSION  // Enable parallel compression
#define ADAPTIVE_RANK_SELECTION // Adaptive rank selection based on accuracy
#define BLOCK_SIZE_OPTIMIZATION // Optimize block sizes for cache efficiency

// Advanced H-matrix parameters with machine learning optimization
typedef struct {
    // Basic parameters
    int cluster_size;                    // Minimum cluster size
    double tolerance;                    // Compression tolerance
    int max_rank;                       // Maximum rank for low-rank approximation
    int admissibility;                  // Admissibility parameter
    
    // Advanced parameters
    int compression_algorithm;           // 0: Standard ACA, 1: ACA+, 2: H², 3: Hybrid
    bool use_adaptive_rank;             // Enable adaptive rank selection
    bool use_machine_learning;          // Use ML for parameter optimization
    int min_block_size;                 // Minimum block size for compression
    int max_block_size;                 // Maximum block size for compression
    
    // Performance parameters
    int n_threads;                      // Number of threads for parallel compression
    bool use_vectorization;             // Enable SIMD vectorization
    bool use_gpu_acceleration;          // Enable GPU acceleration
    int memory_limit_mb;               // Memory limit for compression
    
    // Accuracy parameters
    double relative_tolerance;           // Relative accuracy tolerance
    double absolute_tolerance;           // Absolute accuracy tolerance
    bool use_error_estimation;          // Enable error estimation
    int error_estimation_samples;       // Number of samples for error estimation
} AdvancedHMatrixParams;

// Enhanced cluster tree with geometric and algebraic clustering
typedef struct {
    int *indices;                        // Cluster indices
    int n_indices;                      // Number of indices in cluster
    double *centroid;                   // Cluster centroid coordinates
    double radius;                      // Cluster radius
    double *bounding_box;               // [xmin, xmax, ymin, ymax, zmin, zmax]
    int level;                          // Tree level
    
    // Advanced clustering information
    double *principal_components;        // Principal component directions
    double *singular_values;            // Singular values for dimensionality
    int effective_dimension;            // Effective cluster dimension
    
    // Algebraic clustering
    double *algebraic_centroid;         // Centroid in algebraic sense
    double algebraic_radius;             // Algebraic radius
    int *strong_admissibility_nodes;    // Strongly admissible nodes
    int n_strong_admissible;           // Number of strongly admissible nodes
    
    struct EnhancedClusterNode *left, *right;  // Child nodes
} EnhancedClusterNode;

typedef struct {
    EnhancedClusterNode *root;           // Root of cluster tree
    int max_depth;                       // Maximum tree depth
    int n_clusters;                      // Total number of clusters
    
    // Tree statistics
    double *cluster_quality_metrics;     // Quality metrics for each level
    int *cluster_distribution;           // Distribution of cluster sizes
    double tree_balance_factor;          // Tree balance metric
} EnhancedClusterTree;

// Advanced H-matrix block with multiple compression formats
typedef struct {
    int *row_indices;                    // Row indices for this block
    int *col_indices;                    // Column indices for this block
    int n_rows, n_cols;                  // Block dimensions
    
    // Compression format selection
    enum {
        BLOCK_FORMAT_DENSE,              // Dense matrix
        BLOCK_FORMAT_LOW_RANK,           // Low-rank approximation
        BLOCK_FORMAT_H2,                 // H² matrix format
        BLOCK_FORMAT_SPARSE,             // Sparse format
        BLOCK_FORMAT_DIAGONAL,           // Diagonal format
        BLOCK_FORMAT_ZERO                // Zero block
    } compression_format;
    
    // Union of different storage formats
    union {
        struct {
            double complex *data;        // Dense matrix data
        } dense;
        
        struct {
            double complex *U;           // Left singular vectors
            double complex *V;           // Right singular vectors
            double *singular_values;     // Singular values
            int rank;                    // Actual rank
            double compression_error;    // Compression error estimate
        } low_rank;
        
        struct {
            double complex *basis_matrix; // Basis matrix
            double complex *coupling_matrix; // Coupling matrix
            int basis_rank;              // Basis rank
            int *basis_indices;          // Basis function indices
        } h2;
        
        struct {
            int *row_indices_sparse;     // Row indices for sparse format
            int *col_indices_sparse;     // Column indices for sparse format
            double complex *values_sparse; // Non-zero values
            int nnz;                     // Number of non-zeros
        } sparse;
        
        struct {
            double complex *diagonal;    // Diagonal elements
        } diagonal;
    } data;
    
    // Block metadata
    double estimated_error;              // Estimated approximation error
    int compression_time_ms;             // Time spent on compression
    int memory_usage_bytes;              // Memory usage for this block
    bool is_admissible;                  // Admissibility status
} AdvancedHMatrixBlock;

typedef struct {
    AdvancedHMatrixBlock *blocks;        // Array of matrix blocks
    int n_blocks;                        // Number of blocks
    int total_rows, total_cols;          // Total matrix dimensions
    AdvancedHMatrixParams params;        // Compression parameters
    
    // Matrix statistics
    double total_memory_usage;           // Total memory usage
    double compression_ratio;            // Overall compression ratio
    double average_block_rank;          // Average rank of low-rank blocks
    int max_block_rank;                  // Maximum rank encountered
    double construction_time;              // Total construction time
} AdvancedHMatrix;

// Enhanced ACA+ algorithm with improved convergence
typedef struct {
    double complex *U;                   // Left factor
    double complex *V;                   // Right factor
    int rank;                           // Actual rank
    double compression_error;            // Achieved compression error
    int n_pivot_selections;              // Number of pivot selections
    double construction_time;              // Construction time
    bool converged;                      // Convergence status
} ACAPlusResult;

ACAPlusResult adaptive_cross_approximation_plus(
    const double complex *matrix,
    int n_rows, int n_cols,
    const AdvancedHMatrixParams *params,
    int start_rank,
    int max_rank_increment
);

// H² matrix compression with nested bases
typedef struct {
    double complex **basis_matrices;     // Basis matrices for each cluster
    int *basis_ranks;                   // Rank of each basis
    double complex *coupling_matrices;   // Coupling matrices between clusters
    double compression_error;            // Overall compression error
    int total_memory_usage;              // Total memory usage
    double construction_time;              // Construction time
} H2CompressionResult;

H2CompressionResult h2_matrix_compression(
    const double complex *matrix,
    int n_rows, int n_cols,
    const EnhancedClusterTree *row_tree,
    const EnhancedClusterTree *col_tree,
    const AdvancedHMatrixParams *params
);

// Hybrid compression combining multiple algorithms
typedef struct {
    AdvancedHMatrixBlock *blocks;        // Optimally compressed blocks
    int n_blocks;                        // Number of blocks
    double total_compression_time;         // Total compression time
    double total_compression_error;       // Total compression error
    double total_memory_usage;            // Total memory usage
    int *algorithm_used_per_block;       // Algorithm used for each block
    double *compression_ratios_per_block; // Compression ratio per block
} HybridCompressionResult;

HybridCompressionResult hybrid_matrix_compression(
    const double complex *matrix,
    int n_rows, int n_cols,
    const EnhancedClusterTree *row_tree,
    const EnhancedClusterTree *col_tree,
    const AdvancedHMatrixParams *params
);

// Advanced H-matrix operations with optimized performance
void advanced_h_matrix_vector_multiply(
    const AdvancedHMatrix *h_matrix,
    const double complex *input_vector,
    double complex *output_vector,
    int n_threads,
    bool use_gpu
);

AdvancedHMatrix* advanced_h_matrix_multiply(
    const AdvancedHMatrix *A, const AdvancedHMatrix *B,
    const AdvancedHMatrixParams *params,
    int n_threads
);

// Parallel LU decomposition with H-matrix structure
typedef struct {
    AdvancedHMatrix *L;                  // Lower triangular
    AdvancedHMatrix *U;                  // Upper triangular
    int *pivot;                          // Pivot indices
    double decomposition_time;             // Decomposition time
    double decomposition_error;            // Decomposition error
    int memory_usage;                    // Memory usage
} AdvancedHMatrixLU;

AdvancedHMatrixLU* advanced_h_matrix_lu_decompose(
    const AdvancedHMatrix *h_matrix,
    const AdvancedHMatrixParams *params,
    int n_threads
);

// Machine learning-enhanced iterative solver
typedef struct {
    // ML model parameters
    double *neural_network_weights;      // Neural network weights
    int *network_architecture;            // Network architecture
    double *preconditioner_prediction;    // Predicted preconditioner
    
    // Enhanced solver parameters
    int predicted_iterations;             // Predicted number of iterations
    double predicted_convergence_rate;    // Predicted convergence rate
    double *adaptive_tolerance_schedule; // Adaptive tolerance schedule
    int *restart_schedule;               // Adaptive restart schedule
} MLEnhancedSolverParams;

typedef struct {
    double complex *solution;
    int iterations;
    double residual_norm;
    bool converged;
    double convergence_rate;
    int n_preconditioner_updates;        // Number of preconditioner updates
    double solver_efficiency;              // Solver efficiency metric
    double *convergence_history;           // Convergence history
} MLEnhancedSolverResult;

MLEnhancedSolverResult* ml_enhanced_gmres_solver(
    const AdvancedHMatrix *h_matrix,
    const double complex *rhs_vector,
    const double complex *initial_guess,
    const MLEnhancedSolverParams *ml_params,
    const AdvancedHMatrixLU *preconditioner
);

// Adaptive H-matrix construction with online learning
typedef struct {
    AdvancedHMatrix *current_matrix;     // Current H-matrix
    double *accuracy_history;              // Accuracy history
    int *compression_algorithm_history;    // Algorithm selection history
    double *performance_history;           // Performance history
    
    // Online learning parameters
    double learning_rate;                  // Learning rate for adaptation
    int adaptation_frequency;              // How often to adapt
    double performance_threshold;          // Performance threshold for adaptation
} AdaptiveHMatrix;

AdaptiveHMatrix* create_adaptive_h_matrix(
    int initial_rows, int initial_cols,
    const AdvancedHMatrixParams *params
);

void update_adaptive_h_matrix(
    AdaptiveHMatrix *adaptive_matrix,
    const double complex *new_data,
    int row_start, int col_start,
    int n_rows, int n_cols
);

// Performance monitoring and profiling
typedef struct {
    double construction_time;              // Matrix construction time
    double compression_time;               // Compression time
    double multiplication_time;            // Matrix-vector multiplication time
    double memory_usage;                   // Memory usage
    double cache_hit_rate;                 // Cache hit rate
    int n_threads_used;                   // Number of threads used
    double parallel_efficiency;            // Parallel efficiency
    double *block_performance_metrics;     // Performance per block
    int *compression_algorithm_counts;     // Count of each algorithm used
} AdvancedHMatrixProfile;

AdvancedHMatrixProfile* get_advanced_h_matrix_profile(const AdvancedHMatrix *h_matrix);
void optimize_advanced_h_matrix_performance(AdvancedHMatrix *h_matrix);

// Memory management
void free_enhanced_cluster_tree(EnhancedClusterTree *tree);
void free_advanced_h_matrix(AdvancedHMatrix *h_matrix);
void free_advanced_h_matrix_lu(AdvancedHMatrixLU *lu);
void free_aca_plus_result(ACAPlusResult *result);
void free_h2_compression_result(H2CompressionResult *result);
void free_hybrid_compression_result(HybridCompressionResult *result);
void free_ml_enhanced_solver_result(MLEnhancedSolverResult *result);
void free_adaptive_h_matrix(AdaptiveHMatrix *matrix);

#endif // H_MATRIX_ADVANCED_COMPRESSION_H