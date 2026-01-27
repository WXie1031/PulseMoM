/*****************************************************************************************
 * Industrial-Grade Solver Abstraction Layer
 * 
 * Provides unified interface for multiple solver backends:
 * - Direct solvers (LU, QR, Cholesky)
 * - Iterative solvers (GMRES, CG, BiCGSTAB)
 * - H-matrix accelerated solvers
 * - GPU-accelerated solvers
 * 
 * Supports the unified MoM+PEEC matrix structure with mixed
 * dense, sparse, and H-matrix blocks
 *****************************************************************************************/

#ifndef INDUSTRIAL_SOLVER_ABSTRACTION_H
#define INDUSTRIAL_SOLVER_ABSTRACTION_H

#include <complex.h>
#include <stddef.h>
#include "math_backend_selector.h"
#include "unified_matrix_assembly.h"  // Now in same directory

// Solver types for industrial applications
typedef enum {
    SOLVER_TYPE_DIRECT_LU,          // LU decomposition
    SOLVER_TYPE_DIRECT_QR,          // QR decomposition  
    SOLVER_TYPE_DIRECT_CHOLESKY,    // Cholesky (for SPD matrices)
    SOLVER_TYPE_ITERATIVE_GMRES,      // GMRES (general)
    SOLVER_TYPE_ITERATIVE_CG,        // Conjugate Gradient (SPD)
    SOLVER_TYPE_ITERATIVE_BICGSTAB,  // BiCGSTAB (general)
    SOLVER_TYPE_ITERATIVE_TFQM,      // TFQMR (general)
    SOLVER_TYPE_HMATRIX_GMRES,       // H-matrix accelerated GMRES
    SOLVER_TYPE_GPU_GMRES,           // GPU-accelerated GMRES
    SOLVER_TYPE_HYBRID_DIRECT_ITERATIVE, // Mixed direct/iterative
    SOLVER_TYPE_MULTIGRID,           // Algebraic Multigrid
    SOLVER_TYPE_DOMAIN_DECOMPOSITION  // Domain decomposition
} solver_type_t;

// Preconditioner types
typedef enum {
    PRECONDITIONER_NONE,              // No preconditioning
    PRECONDITIONER_JACOBI,            // Jacobi (diagonal)
    PRECONDITIONER_GAUSS_SEIDEL,     // Gauss-Seidel
    PRECONDITIONER_SOR,              // Successive Over-Relaxation
    PRECONDITIONER_ILU0,             // Incomplete LU (no fill)
    PRECONDITIONER_ILUT,             // Incomplete LU (threshold)
    PRECONDITIONER_IC,               // Incomplete Cholesky
    PRECONDITIONER_BLOCK_JACOBI,     // Block Jacobi
    PRECONDITIONER_ADDITIVE_SCHWARZ, // Additive Schwarz
    PRECONDITIONER_MULTIGRID,        // Multigrid preconditioner
    PRECONDITIONER_HMATRIX_APPROX    // H-matrix approximation
} preconditioner_type_t;

// Matrix properties for solver selection
typedef struct {
    int is_symmetric;           // Symmetric matrix
    int is_positive_definite;   // Positive definite
    int is_hermitian;          // Hermitian matrix
    int is_sparse;             // Sparse matrix
    int is_complex;            // Complex-valued
    size_t condition_number_estimate; // Estimated condition number
    double sparsity_ratio;      // Fraction of non-zero elements
    int block_structure;       // Block matrix structure
    int has_hmatrix_blocks;      // Contains H-matrix blocks
} matrix_properties_t;

// Industrial solver parameters
typedef struct {
    // Solver selection
    solver_type_t solver_type;
    preconditioner_type_t preconditioner_type;
    
    // Convergence criteria
    double relative_tolerance;
    double absolute_tolerance;
    int max_iterations;
    int restart_size;          // For GMRES
    
    // Performance tuning
    int num_threads;
    int use_gpu;
    int gpu_device_id;
    size_t memory_limit_mb;
    
    // Industrial accuracy
    double singularity_tolerance;
    double near_singularity_tolerance;
    int adaptive_precision;
    
    // Debugging and profiling
    int verbose_level;
    int enable_profiling;
    int enable_residual_monitoring;
    
    // Restart and recovery
    int enable_restart;
    int max_restarts;
    double restart_tolerance_improvement;
} industrial_solver_params_t;

// Solver statistics for industrial validation
typedef struct {
    // Convergence metrics
    int iterations_taken;
    double final_residual;
    double initial_residual;
    int converged;
    
    // Performance metrics
    double setup_time;
    double solve_time;
    double total_time;
    size_t memory_used_mb;
    
    // Quality metrics
    double solution_accuracy;
    double condition_number_estimate;
    int singularity_detected;
    int near_singularity_count;
    
    // Industrial validation
    double max_solution_error;
    double avg_solution_error;
    int validation_passed;
} solver_statistics_t;

// Unified solver handle
typedef struct {
    solver_type_t type;
    preconditioner_type_t preconditioner;
    matrix_handle_t* matrix;        // System matrix
    matrix_handle_t* preconditioner_matrix; // Preconditioner
    industrial_solver_params_t* params; // Solver parameters
    void* backend_solver;           // Backend-specific solver handle
    solver_statistics_t* stats;     // Solver statistics
    int is_initialized;
    int is_factorized;
} industrial_solver_t;

// Core solver functions
industrial_solver_t* industrial_solver_create(matrix_handle_t* matrix,
                                            industrial_solver_params_t* params);
void industrial_solver_destroy(industrial_solver_t* solver);

// Solver setup and factorization
int industrial_solver_setup(industrial_solver_t* solver);
int industrial_solver_factorize(industrial_solver_t* solver);
int industrial_solver_build_preconditioner(industrial_solver_t* solver);

// Main solve function
int industrial_solver_solve(industrial_solver_t* solver,
                           const vector_handle_t* rhs,
                           vector_handle_t* solution);

// Advanced solve functions
int industrial_solver_solve_multiple_rhs(industrial_solver_t* solver,
                                         const vector_handle_t** rhs_array,
                                         vector_handle_t** solution_array,
                                         int num_rhs);

int industrial_solver_solve_with_initial_guess(industrial_solver_t* solver,
                                               const vector_handle_t* rhs,
                                               vector_handle_t* solution,
                                               const vector_handle_t* initial_guess);

// Solver for unified MoM+PEEC matrix
int industrial_solver_solve_unified_matrix(industrial_solver_t* solver,
                                           const unified_matrix_t* matrix,
                                           const vector_handle_t* rhs,
                                           vector_handle_t* solution);

// Adaptive solver selection
solver_type_t industrial_solver_select_optimal_solver(
    const matrix_handle_t* matrix,
    const matrix_properties_t* properties,
    industrial_solver_params_t* params);

preconditioner_type_t industrial_solver_select_optimal_preconditioner(
    const matrix_handle_t* matrix,
    const matrix_properties_t* properties,
    solver_type_t solver_type);

// Matrix property analysis
int industrial_solver_analyze_matrix_properties(const matrix_handle_t* matrix,
                                               matrix_properties_t* properties);

// Industrial validation and benchmarking
int industrial_solver_validate_solution(const industrial_solver_t* solver,
                                        const vector_handle_t* computed_solution,
                                        const vector_handle_t* reference_solution,
                                        double tolerance);

int industrial_solver_benchmark_solver(industrial_solver_t* solver,
                                      const vector_handle_t* test_rhs,
                                      solver_statistics_t* benchmark_stats);

// Error handling and diagnostics
typedef struct {
    int error_code;
    char error_message[512];
    int singularity_detected;
    int ill_conditioning_detected;
    double estimated_condition_number;
    char recommended_action[256];
} solver_diagnostics_t;

int industrial_solver_get_diagnostics(const industrial_solver_t* solver,
                                     solver_diagnostics_t* diagnostics);

// Performance optimization
int industrial_solver_optimize_parameters(industrial_solver_t* solver,
                                         industrial_solver_params_t* optimized_params);

int industrial_solver_tune_for_accuracy(industrial_solver_t* solver,
                                       double target_accuracy);

int industrial_solver_tune_for_speed(industrial_solver_t* solver,
                                    double target_time);

int industrial_solver_tune_for_memory(industrial_solver_t* solver,
                                     size_t memory_limit_mb);

// GPU acceleration support
int industrial_solver_enable_gpu(industrial_solver_t* solver, int device_id);
int industrial_solver_disable_gpu(industrial_solver_t* solver);
int industrial_solver_is_gpu_accelerated(const industrial_solver_t* solver);

// H-matrix solver specialization
int industrial_solver_setup_hmatrix_solver(industrial_solver_t* solver,
                                          double compression_tolerance);

int industrial_solver_set_hmatrix_cluster_size(industrial_solver_t* solver,
                                                int cluster_size);

// Domain decomposition for very large problems
int industrial_solver_setup_domain_decomposition(industrial_solver_t* solver,
                                                int num_subdomains);

// Restart and recovery
int industrial_solver_save_state(const industrial_solver_t* solver,
                                const char* state_file);

int industrial_solver_load_state(industrial_solver_t* solver,
                                const char* state_file);

// Commercial-grade error codes
typedef enum {
    SOLVER_SUCCESS = 0,
    SOLVER_ERROR_CONVERGENCE = -1,
    SOLVER_ERROR_SINGULAR = -2,
    SOLVER_ERROR_ILL_CONDITIONED = -3,
    SOLVER_ERROR_OUT_OF_MEMORY = -4,
    SOLVER_ERROR_INVALID_PARAMETER = -5,
    SOLVER_ERROR_BACKEND_FAILURE = -6,
    SOLVER_ERROR_GPU_NOT_AVAILABLE = -7,
    SOLVER_ERROR_PRECONDITIONER_FAILURE = -8,
    SOLVER_ERROR_RESTART_FAILED = -9,
    SOLVER_ERROR_VALIDATION_FAILED = -10
} solver_error_t;

const char* industrial_solver_error_string(solver_error_t error);

// Industrial constants
#define SOLVER_DEFAULT_TOLERANCE 1e-6
#define SOLVER_DEFAULT_MAX_ITERATIONS 1000
#define SOLVER_DEFAULT_RESTART_SIZE 30
#define SOLVER_MEMORY_SAFETY_FACTOR 1.5
#define SOLVER_CONDITION_NUMBER_LIMIT 1e12
#define SOLVER_NEAR_SINGULARITY_THRESHOLD 1e-10

// Convenience macros for common operations
#define SOLVE_WITH_GMRES(matrix, rhs, solution, tolerance) \
    industrial_solver_solve_with_params(matrix, rhs, solution, SOLVER_TYPE_ITERATIVE_GMRES, tolerance)

#define SOLVE_WITH_GPU(matrix, rhs, solution) \
    industrial_solver_solve_with_gpu(matrix, rhs, solution, BACKEND_CUBLAS)

#endif // INDUSTRIAL_SOLVER_ABSTRACTION_H