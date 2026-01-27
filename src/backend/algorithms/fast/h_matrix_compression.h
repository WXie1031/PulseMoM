#ifndef H_MATRIX_COMPRESSION_H
#define H_MATRIX_COMPRESSION_H

#include <stdbool.h>
#include "core_common.h"
#include "../../operators/greens/layered_greens_function.h"
#include "../../discretization/basis/core_basis_functions.h"

typedef struct {
    int cluster_size;      // Minimum cluster size for compression
    double tolerance;      // Compression tolerance
    int max_rank;         // Maximum rank for low-rank approximation
    int admissibility;    // Admissibility parameter
} HMatrixParams;

typedef struct {
    int *row_indices;      // Row indices for this block
    int *col_indices;      // Column indices for this block
    int n_rows, n_cols;    // Block dimensions
    
    bool is_low_rank;      // True if low-rank, false if dense
    
    union {
        struct {
            complex_t *U;  // Left singular vectors
            complex_t *V;  // Right singular vectors
            int rank;           // Actual rank
        } low_rank;
        
        struct {
            complex_t *data;  // Dense matrix data
        } dense;
    } data;
} HMatrixBlock;

typedef struct {
    HMatrixBlock *blocks;   // Array of matrix blocks
    int n_blocks;          // Number of blocks
    int total_rows, total_cols;
    HMatrixParams params;
} HMatrix;

typedef struct {
    int *indices;          // Cluster indices
    int n_indices;       // Number of indices in cluster
    double *centroid;    // Cluster centroid coordinates
    double radius;       // Cluster radius
    int level;           // Tree level
    struct ClusterNode *left, *right;  // Child nodes
} ClusterNode;

typedef struct {
    ClusterNode *root;     // Root of cluster tree
    int max_depth;        // Maximum tree depth
    int n_clusters;       // Total number of clusters
} ClusterTree;

// H-matrix construction
HMatrix* create_h_matrix(
    const complex_t *dense_matrix,
    int n_rows, int n_cols,
    const HMatrixParams *params
);

// Cluster tree construction
ClusterTree* build_cluster_tree(
    const double *coordinates,  // [n_points][3] array
    int n_points,
    int max_depth,
    int min_cluster_size
);

// Adaptive cross approximation (ACA)
void adaptive_cross_approximation(
    const complex_t *matrix,
    int n_rows, int n_cols,
    double tolerance,
    int max_rank,
    complex_t **U, complex_t **V,
    int *actual_rank
);

// H-matrix vector multiplication
void h_matrix_vector_multiply(
    const HMatrix *h_matrix,
    const complex_t *input_vector,
    complex_t *output_vector
);

// H-matrix matrix multiplication
HMatrix* h_matrix_multiply(
    const HMatrix *A, const HMatrix *B,
    const HMatrixParams *params
);

// H-matrix LU decomposition (approximate)
typedef struct {
    HMatrix *L;  // Lower triangular
    HMatrix *U;  // Upper triangular
    int *pivot;  // Pivot indices
} HMatrixLU;

HMatrixLU* h_matrix_lu_decompose(
    const HMatrix *h_matrix,
    const HMatrixParams *params
);

// Iterative solver with H-matrix preconditioning
typedef struct {
    int max_iterations;
    double tolerance;
    int restart_parameter;  // For GMRES
    bool use_preconditioner;
    int convergence_check_interval;
} IterativeSolverParams;

typedef struct {
    complex_t *solution;
    int iterations;
    double residual_norm;
    bool converged;
} SolverResult;

// GMRES solver with H-matrix compression
SolverResult* gmres_solver_hmatrix(
    const HMatrix *h_matrix,
    const complex_t *rhs_vector,
    const complex_t *initial_guess,
    const IterativeSolverParams *params,
    const HMatrixLU *preconditioner
);

// BiCGSTAB solver alternative
SolverResult* bicgstab_solver_hmatrix(
    const HMatrix *h_matrix,
    const complex_t *rhs_vector,
    const complex_t *initial_guess,
    const IterativeSolverParams *params,
    const HMatrixLU *preconditioner
);

// H-matrix based MoM solver
typedef struct {
    HMatrix *impedance_matrix;
    HMatrixLU *lu_factorization;
    SolverResult *last_solution;
    double frequency;
    int n_unknowns;
} MoMSolver;

MoMSolver* create_mom_solver(
    const MeshData *mesh,
    const GreensFunctionDyadic *greens_func,
    const FrequencyDomain *freq,
    const HMatrixParams *h_params,
    const IterativeSolverParams *solver_params
);

SolverResult* solve_mom_system(
    MoMSolver *solver,
    const complex_t *excitation_vector,
    const complex_t *initial_guess
);

// Parallel H-matrix operations (OpenMP)
void parallel_h_matrix_construction(
    HMatrix *h_matrix,
    const complex_t *dense_matrix,
    int n_threads
);

void parallel_h_matrix_vector_multiply(
    const HMatrix *h_matrix,
    const complex_t *input_vector,
    complex_t *output_vector,
    int n_threads
);

// Memory management
void free_cluster_tree(ClusterTree *tree);
void free_h_matrix(HMatrix *h_matrix);
void free_h_matrix_lu(HMatrixLU *lu);
void free_solver_result(SolverResult *result);
void free_mom_solver(MoMSolver *solver);

#endif // H_MATRIX_COMPRESSION_H