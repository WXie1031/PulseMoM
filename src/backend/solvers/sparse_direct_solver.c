/**
 * @file sparse_direct_solver.c
 * @brief Implementation of commercial-grade sparse direct solver interface
 * @details Unified MUMPS/PARDISO integration with fallback options
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "sparse_direct_solver.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

// MUMPS interface (if available)
#ifdef HAVE_MUMPS
#include "dmumps_c.h"
#define ICNTL(I) icntl[(I)-1]  /* Macro for 1-based indexing */
#define CNTL(I) cntl[(I)-1]    /* Macro for 1-based indexing */
#endif

// PARDISO interface (if available)
#ifdef HAVE_PARDISO
#include "mkl_pardiso.h"
#include "mkl_types.h"
#include "mkl_spblas.h"
#endif

// Internal solver data structure
struct sparse_direct_solver {
    sparse_solver_config_t config;
    sparse_matrix_t matrix;
    
    // Solver-specific data
    union {
#ifdef HAVE_MUMPS
        DMUMPS_STRUC_C *mumps_data;
#endif
#ifdef HAVE_PARDISO
        void *pardiso_data[64];  // PARDISO handle array
#endif
        void *generic_data;
    } solver_data;
    
    // Solver state
    bool is_analyzed;
    bool is_factorized;
    bool is_singular;
    
    // Statistics
    sparse_solver_stats_t stats;
    
    // Working arrays
    double complex *work_vector;
    size_t work_vector_size;
    
    // Out-of-core support
    char temp_directory[512];
    bool use_ooc;
};

// Error codes
typedef enum {
    SPARSE_SOLVER_SUCCESS = 0,
    SPARSE_SOLVER_ERROR_MEMORY = -1,
    SPARSE_SOLVER_ERROR_SINGULAR = -2,
    SPARSE_SOLVER_ERROR_NOT_AVAILABLE = -3,
    SPARSE_SOLVER_ERROR_INVALID_INPUT = -4,
    SPARSE_SOLVER_ERROR_FACTORIZATION = -5,
    SPARSE_SOLVER_ERROR_ANALYSIS = -6,
    SPARSE_SOLVER_ERROR_OOC = -7,
    SPARSE_SOLVER_ERROR_LICENSE = -8
} sparse_solver_error_t;

// Utility functions
static double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static int create_temp_directory(char *path, size_t size) {
    const char *tmp_dir = getenv("TMPDIR");
    if (!tmp_dir) tmp_dir = "/tmp";
    
    snprintf(path, size, "%s/pulseem_sparse_solver_%d", tmp_dir, getpid());
    
    if (mkdir(path, 0700) != 0 && errno != EEXIST) {
        return -1;
    }
    
    return 0;
}

// MUMPS-specific implementation
#ifdef HAVE_MUMPS
static int mumps_init(sparse_direct_solver_t *solver) {
    solver->solver_data.mumps_data = (DMUMPS_STRUC_C *)calloc(1, sizeof(DMUMPS_STRUC_C));
    if (!solver->solver_data.mumps_data) {
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    DMUMPS_STRUC_C *mumps = solver->solver_data.mumps_data;
    
    // Initialize MUMPS
    mumps->par = 1;  // Host working
    mumps->sym = 0;  // Unsymmetric matrix
    mumps->job = -1; // Initialize
    
    dmumps_c(mumps);
    
    // Set default parameters
    mumps->ICNTL(1) = 6;   // Output stream for error messages
    mumps->ICNTL(2) = 0;   // Output stream for diagnostic messages
    mumps->ICNTL(3) = 6;   // Output stream for global information
    mumps->ICNTL(4) = solver->config.verbose ? 2 : 0; // Message level
    
    mumps->ICNTL(5) = 0;   // Input matrix format: assembled
    mumps->ICNTL(6) = 7;   // Matrix permutation: automatic
    mumps->ICNTL(7) = 5;   // Blas block size
    mumps->ICNTL(8) = 0;   // Row and column scaling: no
    mumps->ICNTL(9) = 1;   // Solution method: A x = b
    mumps->ICNTL(10) = 0;  // Iterative refinement: no
    mumps->ICNTL(11) = 0;  // Error analysis: no
    mumps->ICNTL(12) = 0;  // Solution improvement: no
    mumps->ICNTL(13) = 0;  // Out-of-core: no
    mumps->ICNTL(14) = 20; // Percentage increase in estimated working space
    mumps->ICNTL(15) = 0;  // Input matrix format: assembled
    mumps->ICNTL(16) = 0;  // Schur complement: no
    mumps->ICNTL(17) = 0;  // Matrix input format: centralized
    mumps->ICNTL(18) = 0;  // Matrix format: assembled
    mumps->ICNTL(19) = 0;  // Schur complement: no
    mumps->ICNTL(20) = 0;  // Right-hand side format: dense
    mumps->ICNTL(21) = 0;  // Solution vector: dense
    mumps->ICNTL(22) = 0;  // In-core/out-of-core: in-core
    mumps->ICNTL(23) = 0;  // Max size of working memory: default
    mumps->ICNTL(24) = 0;  // Null pivot detection: default
    mumps->ICNTL(25) = 0;  // Solve transposed system: no
    
    // Set out-of-core mode if requested
    if (solver->config.use_out_of_core) {
        mumps->ICNTL(22) = 1;  // Enable out-of-core
        strncpy(mumps->write_problem, solver->temp_directory, 255);
    }
    
    // Set number of threads
    mumps->ICNTL(28) = solver->config.num_threads;
    
    return SPARSE_SOLVER_SUCCESS;
}

static int mumps_set_matrix(sparse_direct_solver_t *solver) {
    DMUMPS_STRUC_C *mumps = solver->solver_data.mumps_data;
    const sparse_matrix_t *matrix = &solver->matrix;
    
    // Set matrix dimensions
    mumps->n = matrix->num_rows;
    mumps->nz = matrix->nnz;
    
    // Convert to 1-based indexing for MUMPS
    mumps->irn = (int *)malloc(matrix->nnz * sizeof(int));
    mumps->jcn = (int *)malloc(matrix->nnz * sizeof(int));
    mumps->a = (double *)malloc(matrix->nnz * sizeof(double));
    
    if (!mumps->irn || !mumps->jcn || !mumps->a) {
        free(mumps->irn);
        free(mumps->jcn);
        free(mumps->a);
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    // Convert CSR to coordinate format
    int idx = 0;
    for (int i = 0; i < matrix->num_rows; i++) {
        for (int j = matrix->row_ptr[i]; j < matrix->row_ptr[i+1]; j++) {
            mumps->irn[idx] = i + 1;  // 1-based
            mumps->jcn[idx] = matrix->col_idx[j] + 1;  // 1-based
            mumps->a[idx] = creal(matrix->values[j]);  // Real part only for now
            idx++;
        }
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

static int mumps_analyze(sparse_direct_solver_t *solver) {
    DMUMPS_STRUC_C *mumps = solver->solver_data.mumps_data;
    
    mumps->job = 1;  // Analysis only
    dmumps_c(mumps);
    
    if (mumps->info[0] < 0) {
        return SPARSE_SOLVER_ERROR_ANALYSIS;
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

static int mumps_factorize(sparse_direct_solver_t *solver) {
    DMUMPS_STRUC_C *mumps = solver->solver_data.mumps_data;
    
    mumps->job = 2;  // Factorization
    dmumps_c(mumps);
    
    if (mumps->info[0] < 0) {
        if (mumps->info[0] == -10) {
            solver->is_singular = true;
            return SPARSE_SOLVER_ERROR_SINGULAR;
        }
        return SPARSE_SOLVER_ERROR_FACTORIZATION;
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

static int mumps_solve(sparse_direct_solver_t *solver, const double complex *rhs, 
                      double complex *solution, int num_rhs) {
    DMUMPS_STRUC_C *mumps = solver->solver_data.mumps_data;
    
    // Set right-hand side
    mumps->nrhs = num_rhs;
    mumps->rhs = (double *)malloc(matrix->num_rows * num_rhs * sizeof(double));
    if (!mumps->rhs) {
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    // Convert complex RHS to real (for now)
    for (int i = 0; i < matrix->num_rows * num_rhs; i++) {
        mumps->rhs[i] = creal(rhs[i]);
    }
    
    mumps->job = 3;  // Solve
    dmumps_c(mumps);
    
    if (mumps->info[0] < 0) {
        free(mumps->rhs);
        return SPARSE_SOLVER_ERROR_SINGULAR;
    }
    
    // Copy solution back
    for (int i = 0; i < matrix->num_rows * num_rhs; i++) {
        solution[i] = mumps->rhs[i] + 0.0 * I;
    }
    
    free(mumps->rhs);
    return SPARSE_SOLVER_SUCCESS;
}

static void mumps_cleanup(sparse_direct_solver_t *solver) {
    if (solver->solver_data.mumps_data) {
        DMUMPS_STRUC_C *mumps = solver->solver_data.mumps_data;
        
        // Free MUMPS arrays
        free(mumps->irn);
        free(mumps->jcn);
        free(mumps->a);
        
        // Terminate MUMPS
        mumps->job = -2;
        dmumps_c(mumps);
        
        free(mumps);
        solver->solver_data.mumps_data = NULL;
    }
}
#endif // HAVE_MUMPS

// PARDISO-specific implementation
#ifdef HAVE_PARDISO
static int pardiso_init(sparse_direct_solver_t *solver) {
    // Initialize PARDISO handle
    for (int i = 0; i < 64; i++) {
        solver->solver_data.pardiso_data[i] = 0;
    }
    
    // Set default parameters
    int *iparm = (int *)solver->solver_data.pardiso_data;
    
    iparm[0] = 1;   // Use default values
    iparm[1] = 2;   // METIS fill-in reordering
    iparm[2] = solver->config.num_threads; // Number of threads
    iparm[3] = 0;   // No iterative-direct algorithm
    iparm[4] = 0;   // No user fill-in reducing permutation
    iparm[5] = 0;   // Write solution to b
    iparm[6] = 0;   // Not in use
    iparm[7] = 2;   // Max numbers of iterative refinement steps
    iparm[8] = 0;   // Not in use
    iparm[9] = 13;  // Perturb the pivot elements with 1E-13
    iparm[10] = 1;  // Use nonsymmetric permutation and scaling MPS
    iparm[11] = 0;  // Not in use
    iparm[12] = 0;  // Not in use
    iparm[13] = 0;  // Output: Number of perturbed pivots
    iparm[14] = 0;  // Not in use
    iparm[15] = 0;  // Not in use
    iparm[16] = 0;  // Not in use
    iparm[17] = -1; // Output: Number of nonzeros in the factor LU
    iparm[18] = -1; // Output: Mflops for LU factorization
    iparm[19] = 0;  // Output: Numbers of CG Iterations
    
    // Set out-of-core mode if requested
    if (solver->config.use_out_of_core) {
        iparm[59] = 2;  // Enable out-of-core
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

static int pardiso_set_matrix(sparse_direct_solver_t *solver) {
    // PARDISO uses CSR format directly
    return SPARSE_SOLVER_SUCCESS;
}

static int pardiso_analyze(sparse_direct_solver_t *solver) {
    const sparse_matrix_t *matrix = &solver->matrix;
    int *iparm = (int *)solver->solver_data.pardiso_data;
    void *pt = solver->solver_data.pardiso_data;
    
    int mtype = 13;  // Complex unsymmetric matrix
    int error = 0;
    int phase = 11;  // Analysis phase
    
    // Convert CSR to 1-based indexing for PARDISO
    int *row_ptr_1based = (int *)malloc((matrix->num_rows + 1) * sizeof(int));
    if (!row_ptr_1based) {
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    for (int i = 0; i <= matrix->num_rows; i++) {
        row_ptr_1based[i] = matrix->row_ptr[i] + 1;
    }
    
    pardiso(pt, &maxfct, &mnum, &mtype, &phase,
           &matrix->num_rows, matrix->values, row_ptr_1based, matrix->col_idx,
           NULL, &nrhs, iparm, &solver->config.message_level, NULL, NULL, &error);
    
    free(row_ptr_1based);
    
    if (error != 0) {
        return SPARSE_SOLVER_ERROR_ANALYSIS;
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

static int pardiso_factorize(sparse_direct_solver_t *solver) {
    const sparse_matrix_t *matrix = &solver->matrix;
    int *iparm = (int *)solver->solver_data.pardiso_data;
    void *pt = solver->solver_data.pardiso_data;
    
    int mtype = 13;  // Complex unsymmetric matrix
    int error = 0;
    int phase = 22;  // Factorization phase
    int maxfct = 1, mnum = 1, nrhs = 1;
    
    // Convert CSR to 1-based indexing for PARDISO
    int *row_ptr_1based = (int *)malloc((matrix->num_rows + 1) * sizeof(int));
    if (!row_ptr_1based) {
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    for (int i = 0; i <= matrix->num_rows; i++) {
        row_ptr_1based[i] = matrix->row_ptr[i] + 1;
    }
    
    pardiso(pt, &maxfct, &mnum, &mtype, &phase,
           &matrix->num_rows, matrix->values, row_ptr_1based, matrix->col_idx,
           NULL, &nrhs, iparm, &solver->config.message_level, NULL, NULL, &error);
    
    free(row_ptr_1based);
    
    if (error != 0) {
        if (error == -4) {
            solver->is_singular = true;
            return SPARSE_SOLVER_ERROR_SINGULAR;
        }
        return SPARSE_SOLVER_ERROR_FACTORIZATION;
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

static int pardiso_solve(sparse_direct_solver_t *solver, const double complex *rhs,
                        double complex *solution, int num_rhs) {
    const sparse_matrix_t *matrix = &solver->matrix;
    int *iparm = (int *)solver->solver_data.pardiso_data;
    void *pt = solver->solver_data.pardiso_data;
    
    int mtype = 13;  // Complex unsymmetric matrix
    int error = 0;
    int phase = 33;  // Solve phase
    int maxfct = 1, mnum = 1;
    
    // Convert CSR to 1-based indexing for PARDISO
    int *row_ptr_1based = (int *)malloc((matrix->num_rows + 1) * sizeof(int));
    if (!row_ptr_1based) {
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    for (int i = 0; i <= matrix->num_rows; i++) {
        row_ptr_1based[i] = matrix->row_ptr[i] + 1;
    }
    
    pardiso(pt, &maxfct, &mnum, &mtype, &phase,
           &matrix->num_rows, matrix->values, row_ptr_1based, matrix->col_idx,
           NULL, &num_rhs, iparm, &solver->config.message_level, 
           (double *)rhs, (double *)solution, &error);
    
    free(row_ptr_1based);
    
    if (error != 0) {
        return SPARSE_SOLVER_ERROR_SINGULAR;
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

static void pardiso_cleanup(sparse_direct_solver_t *solver) {
    if (solver->solver_data.pardiso_data[0] != 0) {
        const sparse_matrix_t *matrix = &solver->matrix;
        int *iparm = (int *)solver->solver_data.pardiso_data;
        void *pt = solver->solver_data.pardiso_data;
        
        int mtype = 13;  // Complex unsymmetric matrix
        int error = 0;
        int phase = -1;  // Release memory
        int maxfct = 1, mnum = 1, nrhs = 1;
        
        // Convert CSR to 1-based indexing for PARDISO
        int *row_ptr_1based = (int *)malloc((matrix->num_rows + 1) * sizeof(int));
        if (row_ptr_1based) {
            for (int i = 0; i <= matrix->num_rows; i++) {
                row_ptr_1based[i] = matrix->row_ptr[i] + 1;
            }
            
            pardiso(pt, &maxfct, &mnum, &mtype, &phase,
                   &matrix->num_rows, matrix->values, row_ptr_1based, matrix->col_idx,
                   NULL, &nrhs, iparm, &solver->config.message_level, NULL, NULL, &error);
            
            free(row_ptr_1based);
        }
        
        for (int i = 0; i < 64; i++) {
            solver->solver_data.pardiso_data[i] = 0;
        }
    }
}
#endif // HAVE_PARDISO

// Main implementation
sparse_direct_solver_t* sparse_direct_solver_init(const sparse_solver_config_t *config) {
    sparse_direct_solver_t *solver = (sparse_direct_solver_t *)calloc(1, sizeof(sparse_direct_solver_t));
    if (!solver) {
        return NULL;
    }
    
    // Copy configuration
    memcpy(&solver->config, config, sizeof(sparse_solver_config_t));
    
    // Create temporary directory for out-of-core if needed
    if (solver->config.use_out_of_core) {
        if (create_temp_directory(solver->temp_directory, sizeof(solver->temp_directory)) != 0) {
            free(solver);
            return NULL;
        }
        solver->use_ooc = true;
    }
    
    // Initialize solver-specific data
    int result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    
    switch (config->solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            result = mumps_init(solver);
            break;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            result = pardiso_init(solver);
            break;
#endif
        default:
            result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    }
    
    if (result != SPARSE_SOLVER_SUCCESS) {
        free(solver);
        return NULL;
    }
    
    return solver;
}

int sparse_direct_solver_set_matrix(sparse_direct_solver_t *solver, const sparse_matrix_t *matrix) {
    if (!solver || !matrix) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // Copy matrix structure
    memcpy(&solver->matrix, matrix, sizeof(sparse_matrix_t));
    
    // Allocate and copy matrix data
    size_t row_ptr_size = (matrix->num_rows + 1) * sizeof(int);
    size_t col_idx_size = matrix->nnz * sizeof(int);
    size_t values_size = matrix->nnz * sizeof(double complex);
    
    solver->matrix.row_ptr = (int *)malloc(row_ptr_size);
    solver->matrix.col_idx = (int *)malloc(col_idx_size);
    solver->matrix.values = (double complex *)malloc(values_size);
    
    if (!solver->matrix.row_ptr || !solver->matrix.col_idx || !solver->matrix.values) {
        sparse_matrix_free(&solver->matrix);
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    memcpy(solver->matrix.row_ptr, matrix->row_ptr, row_ptr_size);
    memcpy(solver->matrix.col_idx, matrix->col_idx, col_idx_size);
    memcpy(solver->matrix.values, matrix->values, values_size);
    
    // Set matrix in solver backend
    int result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    
    switch (solver->config.solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            result = mumps_set_matrix(solver);
            break;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            result = pardiso_set_matrix(solver);
            break;
#endif
        default:
            result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    }
    
    return result;
}

int sparse_direct_solver_analyze(sparse_direct_solver_t *solver) {
    if (!solver || !solver->matrix.values) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    double start_time = get_time();
    
    int result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    
    switch (solver->config.solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            result = mumps_analyze(solver);
            break;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            result = pardiso_analyze(solver);
            break;
#endif
        default:
            result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    }
    
    if (result == SPARSE_SOLVER_SUCCESS) {
        solver->is_analyzed = true;
        solver->stats.analysis_time = get_time() - start_time;
    }
    
    return result;
}

int sparse_direct_solver_factorize(sparse_direct_solver_t *solver) {
    if (!solver || !solver->is_analyzed) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    double start_time = get_time();
    
    int result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    
    switch (solver->config.solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            result = mumps_factorize(solver);
            break;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            result = pardiso_factorize(solver);
            break;
#endif
        default:
            result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    }
    
    if (result == SPARSE_SOLVER_SUCCESS) {
        solver->is_factorized = true;
        solver->stats.factorization_time = get_time() - start_time;
    }
    
    return result;
}

int sparse_direct_solver_solve(sparse_direct_solver_t *solver, const double complex *rhs,
                               double complex *solution, int num_rhs) {
    if (!solver || !solver->is_factorized || !rhs || !solution) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    double start_time = get_time();
    
    int result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    
    switch (solver->config.solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            result = mumps_solve(solver, rhs, solution, num_rhs);
            break;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            result = pardiso_solve(solver, rhs, solution, num_rhs);
            break;
#endif
        default:
            result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    }
    
    if (result == SPARSE_SOLVER_SUCCESS) {
        solver->stats.solve_time = get_time() - start_time;
    }
    
    return result;
}

int sparse_direct_solver_solve_multiple(sparse_direct_solver_t *solver, const double complex *rhs,
                                        double complex *solution, int num_rhs) {
    return sparse_direct_solver_solve(solver, rhs, solution, num_rhs);
}

int sparse_direct_solver_refactorize(sparse_direct_solver_t *solver, const double complex *new_values) {
    if (!solver || !solver->is_analyzed || !new_values) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // Update matrix values
    memcpy(solver->matrix.values, new_values, solver->matrix.nnz * sizeof(double complex));
    
    // Refactorize
    double start_time = get_time();
    
    int result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    
    switch (solver->config.solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            // MUMPS doesn't have separate refactorize phase, redo factorization
            result = mumps_factorize(solver);
            break;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            {
                int phase = 23;  // Refactorization and solve
                int error = 0;
                int maxfct = 1, mnum = 1, nrhs = 1;
                int mtype = 13;
                const sparse_matrix_t *matrix = &solver->matrix;
                int *iparm = (int *)solver->solver_data.pardiso_data;
                void *pt = solver->solver_data.pardiso_data;
                
                // Convert CSR to 1-based indexing for PARDISO
                int *row_ptr_1based = (int *)malloc((matrix->num_rows + 1) * sizeof(int));
                if (!row_ptr_1based) {
                    return SPARSE_SOLVER_ERROR_MEMORY;
                }
                
                for (int i = 0; i <= matrix->num_rows; i++) {
                    row_ptr_1based[i] = matrix->row_ptr[i] + 1;
                }
                
                pardiso(pt, &maxfct, &mnum, &mtype, &phase,
                       &matrix->num_rows, matrix->values, row_ptr_1based, matrix->col_idx,
                       NULL, &nrhs, iparm, &solver->config.message_level, NULL, NULL, &error);
                
                free(row_ptr_1based);
                
                if (error != 0) {
                    result = SPARSE_SOLVER_ERROR_FACTORIZATION;
                } else {
                    result = SPARSE_SOLVER_SUCCESS;
                }
            }
            break;
#endif
        default:
            result = SPARSE_SOLVER_ERROR_NOT_AVAILABLE;
    }
    
    if (result == SPARSE_SOLVER_SUCCESS) {
        solver->stats.factorization_time = get_time() - start_time;
    }
    
    return result;
}

int sparse_direct_solver_get_stats(const sparse_direct_solver_t *solver, sparse_solver_stats_t *stats) {
    if (!solver || !stats) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    memcpy(stats, &solver->stats, sizeof(sparse_solver_stats_t));
    return SPARSE_SOLVER_SUCCESS;
}

bool sparse_direct_solver_is_singular(const sparse_direct_solver_t *solver) {
    return solver ? solver->is_singular : false;
}

int sparse_direct_solver_get_condition_number(const sparse_direct_solver_t *solver, double *cond_number) {
    if (!solver || !cond_number) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    *cond_number = solver->stats.condition_number;
    return SPARSE_SOLVER_SUCCESS;
}

int sparse_direct_solver_set_parameter(sparse_direct_solver_t *solver, const char *param_name,
                                      double param_value) {
    if (!solver || !param_name) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // Handle common parameters
    if (strcmp(param_name, "tolerance") == 0) {
        solver->config.refinement_tolerance = param_value;
    } else if (strcmp(param_name, "pivot_tolerance") == 0) {
        solver->config.pivot_tolerance = param_value;
    } else {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

int sparse_direct_solver_get_parameter(const sparse_direct_solver_t *solver, const char *param_name,
                                      double *param_value) {
    if (!solver || !param_name || !param_value) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // Handle common parameters
    if (strcmp(param_name, "tolerance") == 0) {
        *param_value = solver->config.refinement_tolerance;
    } else if (strcmp(param_name, "pivot_tolerance") == 0) {
        *param_value = solver->config.pivot_tolerance;
    } else {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

void sparse_direct_solver_destroy(sparse_direct_solver_t *solver) {
    if (!solver) {
        return;
    }
    
    // Cleanup solver-specific data
    switch (solver->config.solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            mumps_cleanup(solver);
            break;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            pardiso_cleanup(solver);
            break;
#endif
        default:
            break;
    }
    
    // Free matrix data
    sparse_matrix_free(&solver->matrix);
    
    // Free work arrays
    free(solver->work_vector);
    
    // Remove temporary directory
    if (solver->use_ooc) {
        rmdir(solver->temp_directory);
    }
    
    free(solver);
}

// Matrix conversion functions
int sparse_matrix_convert(const sparse_matrix_t *matrix, sparse_format_t target_format,
                         sparse_matrix_t *output_matrix) {
    if (!matrix || !output_matrix) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // For now, only support CSR format
    if (target_format != SPARSE_FORMAT_CSR) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // Copy matrix
    memcpy(output_matrix, matrix, sizeof(sparse_matrix_t));
    
    // Allocate and copy data
    size_t row_ptr_size = (matrix->num_rows + 1) * sizeof(int);
    size_t col_idx_size = matrix->nnz * sizeof(int);
    size_t values_size = matrix->nnz * sizeof(double complex);
    
    output_matrix->row_ptr = (int *)malloc(row_ptr_size);
    output_matrix->col_idx = (int *)malloc(col_idx_size);
    output_matrix->values = (double complex *)malloc(values_size);
    
    if (!output_matrix->row_ptr || !output_matrix->col_idx || !output_matrix->values) {
        sparse_matrix_free(output_matrix);
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    memcpy(output_matrix->row_ptr, matrix->row_ptr, row_ptr_size);
    memcpy(output_matrix->col_idx, matrix->col_idx, col_idx_size);
    memcpy(output_matrix->values, matrix->values, values_size);
    
    return SPARSE_SOLVER_SUCCESS;
}

int sparse_matrix_from_dense(const double complex *dense_matrix, int num_rows, int num_cols,
                            double threshold, sparse_matrix_t *output_matrix) {
    if (!dense_matrix || !output_matrix || num_rows <= 0 || num_cols <= 0) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // Count non-zeros
    int64_t nnz = 0;
    for (int i = 0; i < num_rows * num_cols; i++) {
        if (cabs(dense_matrix[i]) > threshold) {
            nnz++;
        }
    }
    
    // Initialize matrix
    output_matrix->format = SPARSE_FORMAT_CSR;
    output_matrix->num_rows = num_rows;
    output_matrix->num_cols = num_cols;
    output_matrix->nnz = nnz;
    output_matrix->property = MATRIX_PROPERTY_GENERAL;
    output_matrix->is_complex = true;
    
    // Allocate arrays
    output_matrix->row_ptr = (int *)calloc(num_rows + 1, sizeof(int));
    output_matrix->col_idx = (int *)malloc(nnz * sizeof(int));
    output_matrix->values = (double complex *)malloc(nnz * sizeof(double complex));
    
    if (!output_matrix->row_ptr || !output_matrix->col_idx || !output_matrix->values) {
        sparse_matrix_free(output_matrix);
        return SPARSE_SOLVER_ERROR_MEMORY;
    }
    
    // Fill CSR arrays
    int idx = 0;
    for (int i = 0; i < num_rows; i++) {
        output_matrix->row_ptr[i] = idx;
        for (int j = 0; j < num_cols; j++) {
            double complex val = dense_matrix[i * num_cols + j];
            if (cabs(val) > threshold) {
                output_matrix->col_idx[idx] = j;
                output_matrix->values[idx] = val;
                idx++;
            }
        }
    }
    output_matrix->row_ptr[num_rows] = idx;
    
    return SPARSE_SOLVER_SUCCESS;
}

int sparse_matrix_to_dense(const sparse_matrix_t *sparse_matrix, double complex *output_dense_matrix) {
    if (!sparse_matrix || !output_dense_matrix) {
        return SPARSE_SOLVER_ERROR_INVALID_INPUT;
    }
    
    // Initialize to zero
    for (int i = 0; i < sparse_matrix->num_rows * sparse_matrix->num_cols; i++) {
        output_dense_matrix[i] = 0.0 + 0.0 * I;
    }
    
    // Fill non-zero values
    for (int i = 0; i < sparse_matrix->num_rows; i++) {
        for (int j = sparse_matrix->row_ptr[i]; j < sparse_matrix->row_ptr[i+1]; j++) {
            int col = sparse_matrix->col_idx[j];
            output_dense_matrix[i * sparse_matrix->num_cols + col] = sparse_matrix->values[j];
        }
    }
    
    return SPARSE_SOLVER_SUCCESS;
}

void sparse_matrix_free(sparse_matrix_t *matrix) {
    if (!matrix) {
        return;
    }
    
    free(matrix->row_ptr);
    free(matrix->col_idx);
    free(matrix->values);
    free(matrix->uplo);
    
    matrix->row_ptr = NULL;
    matrix->col_idx = NULL;
    matrix->values = NULL;
    matrix->uplo = NULL;
}

const char* sparse_direct_solver_get_version(sparse_solver_type_t solver_type) {
    switch (solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            return "MUMPS 5.6.0";
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            return "Intel MKL PARDISO";
#endif
        default:
            return "Unknown";
    }
}

bool sparse_direct_solver_is_available(sparse_solver_type_t solver_type) {
    switch (solver_type) {
#ifdef HAVE_MUMPS
        case SPARSE_SOLVER_MUMPS:
            return true;
#endif
#ifdef HAVE_PARDISO
        case SPARSE_SOLVER_PARDISO:
            return true;
#endif
        default:
            return false;
    }
}

void sparse_direct_solver_get_default_config(sparse_solver_type_t solver_type,
                                            sparse_solver_config_t *config) {
    if (!config) {
        return;
    }
    
    // Set defaults
    memset(config, 0, sizeof(sparse_solver_config_t));
    
    config->solver_type = solver_type;
    config->matrix_property = MATRIX_PROPERTY_GENERAL;
    
    config->parallel_analysis = true;
    config->parallel_factorization = true;
    config->parallel_solution = true;
    config->num_threads = 4;  // Default number of threads
    
    config->use_out_of_core = false;
    config->memory_limit = 8ULL * 1024 * 1024 * 1024;  // 8GB default
    strcpy(config->working_directory, "/tmp");
    
    config->use_scaling = true;
    config->use_pivoting = true;
    config->pivot_tolerance = 1e-12;
    
    config->use_iterative_refinement = true;
    config->max_refinement_steps = 2;
    config->refinement_tolerance = 1e-12;
    
    config->verbose = false;
    config->message_level = 1;  // Error messages only
    
    config->use_metis_ordering = true;
    config->use_amd_ordering = false;
    config->ordering_strategy = 2;  // METIS
    
    config->use_gpu = false;
    config->gpu_device_id = 0;
}