/******************************************************************************
 * PEEC Solver Plane Wave Excitation
 * 
 * Implements plane wave excitation for PEEC solver
 * 
 * Method: Convert plane wave to equivalent current/voltage sources
 ******************************************************************************/

#ifndef PEEC_PLANE_WAVE_H
#define PEEC_PLANE_WAVE_H

#include "peec_solver.h"
#include "../../core/excitation_plane_wave.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Plane Wave Excitation Functions for PEEC
 ******************************************************************************/

/**
 * Add plane wave excitation to PEEC solver
 * 
 * @param solver PEEC solver
 * @param excitation Plane wave excitation structure
 * @return 0 on success, negative on error
 */
int peec_solver_add_plane_wave_excitation(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation
);

/**
 * Convert plane wave to equivalent current sources for PEEC
 * 
 * The plane wave induces currents on conductors, which are converted
 * to equivalent current sources in the PEEC circuit network.
 * 
 * @param solver PEEC solver
 * @param excitation Plane wave excitation
 * @param current_sources Output: equivalent current sources (allocated by caller)
 * @param num_sources Output: number of current sources
 * @return 0 on success, negative on error
 */
int peec_plane_wave_to_current_sources(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation,
    peec_scalar_complex_t** current_sources,
    int* num_sources
);

/**
 * Convert plane wave to equivalent voltage sources for PEEC
 * 
 * The plane wave can also be represented as equivalent voltage sources
 * in series with the partial inductances.
 * 
 * @param solver PEEC solver
 * @param excitation Plane wave excitation
 * @param voltage_sources Output: equivalent voltage sources (allocated by caller)
 * @param num_sources Output: number of voltage sources
 * @return 0 on success, negative on error
 */
int peec_plane_wave_to_voltage_sources(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation,
    peec_scalar_complex_t** voltage_sources,
    int* num_sources
);

/**
 * Compute induced current density from plane wave
 * 
 * @param solver PEEC solver
 * @param excitation Plane wave excitation
 * @param element_index Element index in mesh
 * @param current_density Output: induced current density [Jx, Jy, Jz]
 * @return 0 on success, negative on error
 */
int peec_plane_wave_compute_induced_current(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation,
    int element_index,
    peec_scalar_complex_t current_density[3]
);

#ifdef __cplusplus
}
#endif

#endif // PEEC_PLANE_WAVE_H
