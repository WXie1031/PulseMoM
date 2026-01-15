/**
 * @file periodic_ewald.h
 * @brief Ewald summation for periodic structures
 * @details Implements Ewald summation method for computing periodic Green's functions
 *          in periodic electromagnetic structures (FSS, photonic crystals, etc.)
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef PERIODIC_EWALD_H
#define PERIODIC_EWALD_H

#include "core_common.h"
#include "core_geometry.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Lattice structure for periodic boundary conditions
 */
typedef struct {
    geom_point_t a1;      // First lattice vector
    geom_point_t a2;      // Second lattice vector (for 2D periodicity)
    geom_point_t a3;      // Third lattice vector (for 3D periodicity, optional)
    int dimension;        // Periodicity dimension (1, 2, or 3)
    bool is_3d;           // True if 3D periodicity, false for 1D/2D
} periodic_lattice_t;

/**
 * @brief Bloch wavevector for periodic structures
 */
typedef struct {
    double kx, ky, kz;   // Bloch wavevector components
} bloch_vector_t;

/**
 * @brief Ewald summation parameters
 */
typedef struct {
    double eta;           // Ewald splitting parameter (controls convergence)
    int n_max_real;       // Maximum number of real-space lattice vectors
    int n_max_reciprocal; // Maximum number of reciprocal-space vectors
    double tolerance;     // Convergence tolerance
} ewald_params_t;

/**
 * @brief Initialize Ewald parameters with default values
 * @param params Output parameter structure
 * @param lattice Lattice structure
 * @param k Wavenumber
 */
void ewald_params_init(ewald_params_t* params, const periodic_lattice_t* lattice, double k);

/**
 * @brief Compute periodic Green's function using Ewald summation
 * @param r Observation point
 * @param r_prime Source point
 * @param lattice Lattice structure
 * @param k_bloch Bloch wavevector
 * @param k Wavenumber
 * @param params Ewald parameters
 * @return Complex Green's function value
 * 
 * Computes: G_periodic(r, r') = Σ_n G(r, r' + R_n) * exp(i k_bloch · R_n)
 * where R_n are lattice vectors and the sum is computed using Ewald method
 */
complex_t ewald_periodic_green(const geom_point_t* r,
                                const geom_point_t* r_prime,
                                const periodic_lattice_t* lattice,
                                const bloch_vector_t* k_bloch,
                                double k,
                                const ewald_params_t* params);

/**
 * @brief Compute real-space contribution to Ewald sum
 * @param r Observation point
 * @param r_prime Source point
 * @param lattice Lattice structure
 * @param k_bloch Bloch wavevector
 * @param k Wavenumber
 * @param params Ewald parameters
 * @return Complex contribution from real space
 */
complex_t ewald_real_space_sum(const geom_point_t* r,
                                const geom_point_t* r_prime,
                                const periodic_lattice_t* lattice,
                                const bloch_vector_t* k_bloch,
                                double k,
                                const ewald_params_t* params);

/**
 * @brief Compute reciprocal-space contribution to Ewald sum
 * @param r Observation point
 * @param r_prime Source point
 * @param lattice Lattice structure
 * @param k_bloch Bloch wavevector
 * @param k Wavenumber
 * @param params Ewald parameters
 * @return Complex contribution from reciprocal space
 */
complex_t ewald_reciprocal_space_sum(const geom_point_t* r,
                                      const geom_point_t* r_prime,
                                      const periodic_lattice_t* lattice,
                                      const bloch_vector_t* k_bloch,
                                      double k,
                                      const ewald_params_t* params);

/**
 * @brief Compute self-term correction for Ewald sum
 * @param r Observation point
 * @param r_prime Source point
 * @param k Wavenumber
 * @param params Ewald parameters
 * @return Complex self-term correction
 */
complex_t ewald_self_term_correction(const geom_point_t* r,
                                     const geom_point_t* r_prime,
                                     double k,
                                     const ewald_params_t* params);

/**
 * @brief Compute reciprocal lattice vectors
 * @param lattice Real-space lattice
 * @param b1 Output: first reciprocal lattice vector
 * @param b2 Output: second reciprocal lattice vector
 * @param b3 Output: third reciprocal lattice vector (for 3D)
 */
void ewald_compute_reciprocal_lattice(const periodic_lattice_t* lattice,
                                      geom_point_t* b1,
                                      geom_point_t* b2,
                                      geom_point_t* b3);

#ifdef __cplusplus
}
#endif

#endif // PERIODIC_EWALD_H

