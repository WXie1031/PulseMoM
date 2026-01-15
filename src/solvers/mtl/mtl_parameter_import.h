/******************************************************************************
 * MTL Solver Parameter Matrix Import
 * 
 * Implements parameter matrix import for MTL solver
 * 
 * Supports: R, L, C, G matrices from external sources
 ******************************************************************************/

#ifndef MTL_PARAMETER_IMPORT_H
#define MTL_PARAMETER_IMPORT_H

#include "../../core/mtl_solver.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Parameter Matrix Structure
 ******************************************************************************/

typedef struct {
    double* frequencies;                  // Frequency points (Hz)
    double** R_matrix;                    // Resistance matrix per frequency [num_freq][num_cond*num_cond]
    double** L_matrix;                    // Inductance matrix per frequency [num_freq][num_cond*num_cond]
    double** C_matrix;                    // Capacitance matrix per frequency [num_freq][num_cond*num_cond]
    double** G_matrix;                    // Conductance matrix per frequency [num_freq][num_cond*num_cond]
    int num_frequencies;                  // Number of frequency points
    int num_conductors;                   // Number of conductors
    bool interpolate;                     // Interpolate between frequencies
} mtl_parameter_matrices_t;

/******************************************************************************
 * Parameter Import Functions
 ******************************************************************************/

/**
 * Import parameter matrices from structure
 * 
 * @param solver MTL solver
 * @param matrices Parameter matrices structure
 * @return 0 on success, negative on error
 */
int mtl_solver_import_parameter_matrices(
    mtl_solver_t* solver,
    const mtl_parameter_matrices_t* matrices
);

/**
 * Import parameter matrices from file
 * 
 * @param solver MTL solver
 * @param filename Parameter matrix file
 * @param format File format ("CSV", "HDF5", "MATLAB")
 * @return 0 on success, negative on error
 */
int mtl_solver_import_parameter_matrices_from_file(
    mtl_solver_t* solver,
    const char* filename,
    const char* format
);

/**
 * Get parameter matrices at specific frequency
 * 
 * @param solver MTL solver
 * @param frequency Frequency (Hz)
 * @param R Output: resistance matrix (allocated by caller)
 * @param L Output: inductance matrix (allocated by caller)
 * @param C Output: capacitance matrix (allocated by caller)
 * @param G Output: conductance matrix (allocated by caller)
 * @return 0 on success, negative on error
 */
int mtl_solver_get_parameter_matrices_at_frequency(
    mtl_solver_t* solver,
    double frequency,
    double* R,
    double* L,
    double* C,
    double* G
);

/**
 * Free parameter matrices
 * 
 * @param matrices Parameter matrices to free
 */
void mtl_parameter_matrices_free(mtl_parameter_matrices_t* matrices);

#ifdef __cplusplus
}
#endif

#endif // MTL_PARAMETER_IMPORT_H
