/******************************************************************************
 * Quasistatic Coupling Module
 * 
 * Implements quasistatic coupling for electromagnetic simulation
 * 
 * Method: Quasistatic approximation for low-frequency coupling
 ******************************************************************************/

#ifndef COUPLING_QUASISTATIC_H
#define COUPLING_QUASISTATIC_H

#include "core_geometry.h"
#include "core_mesh.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Coupling Types
 ******************************************************************************/

typedef enum {
    COUPLING_QUASISTATIC_CAPACITIVE,      // Capacitive coupling only
    COUPLING_QUASISTATIC_INDUCTIVE,       // Inductive coupling only
    COUPLING_QUASISTATIC_BOTH             // Both capacitive and inductive
} coupling_quasistatic_type_t;

/******************************************************************************
 * Coupling Matrix Structure
 ******************************************************************************/

typedef struct {
    double** C_matrix;           // Capacitance matrix (F)
    double** L_matrix;           // Inductance matrix (H)
    double** K_matrix;           // Coupling coefficient matrix
    int num_elements;            // Number of elements
    bool is_symmetric;           // Matrix symmetry flag
} coupling_quasistatic_matrix_t;

/******************************************************************************
 * Coupling Configuration
 ******************************************************************************/

typedef struct {
    coupling_quasistatic_type_t coupling_type;  // Coupling type
    double frequency_threshold;                  // Frequency threshold for quasistatic (Hz)
    double distance_threshold;                   // Distance threshold (m)
    bool include_self_coupling;                  // Include self-coupling terms
    bool use_analytical_formulas;                // Use analytical formulas when possible
    double relative_tolerance;                   // Relative tolerance for numerical integration
} coupling_quasistatic_config_t;

/******************************************************************************
 * Coupling Functions
 ******************************************************************************/

/**
 * Compute quasistatic coupling matrix
 * 
 * @param geometry Geometry structure
 * @param mesh Mesh structure
 * @param config Coupling configuration
 * @param coupling_matrix Output: coupling matrix
 * @return 0 on success, negative on error
 */
int coupling_quasistatic_compute_matrix(
    const geom_geometry_t* geometry,
    const mesh_t* mesh,
    const coupling_quasistatic_config_t* config,
    coupling_quasistatic_matrix_t* coupling_matrix
);

/**
 * Compute capacitive coupling between two conductors
 * 
 * @param conductor1 First conductor geometry
 * @param conductor2 Second conductor geometry
 * @param medium Medium properties (permittivity)
 * @param capacitance Output: mutual capacitance (F)
 * @return 0 on success, negative on error
 */
int coupling_quasistatic_compute_capacitive(
    const void* conductor1,
    const void* conductor2,
    const void* medium,
    double* capacitance
);

/**
 * Compute inductive coupling between two conductors
 * 
 * @param conductor1 First conductor geometry
 * @param conductor2 Second conductor geometry
 * @param medium Medium properties (permeability)
 * @param inductance Output: mutual inductance (H)
 * @return 0 on success, negative on error
 */
int coupling_quasistatic_compute_inductive(
    const void* conductor1,
    const void* conductor2,
    const void* medium,
    double* inductance
);

/**
 * Compute coupling coefficient
 * 
 * @param L12 Mutual inductance (H)
 * @param L11 Self-inductance of conductor 1 (H)
 * @param L22 Self-inductance of conductor 2 (H)
 * @param k Output: coupling coefficient
 * @return 0 on success, negative on error
 */
int coupling_quasistatic_compute_coefficient(
    double L12,
    double L11,
    double L22,
    double* k
);

/**
 * Free coupling matrix
 * 
 * @param coupling_matrix Coupling matrix to free
 */
void coupling_quasistatic_free_matrix(coupling_quasistatic_matrix_t* coupling_matrix);

/**
 * Get default configuration
 * 
 * @return Default configuration
 */
coupling_quasistatic_config_t coupling_quasistatic_get_default_config(void);

#ifdef __cplusplus
}
#endif

#endif // COUPLING_QUASISTATIC_H
