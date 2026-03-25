/********************************************************************************
 * MoM Physics Definition (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file contains ONLY the physical equations and boundary conditions
 * for the Method of Moments. No numerical implementation, no solver calls.
 *
 * Architecture Rule: L1 layer defines WHAT the physics is, not HOW to compute it.
 ********************************************************************************/

#ifndef MOM_PHYSICS_H
#define MOM_PHYSICS_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Physical Equation Definitions
// ============================================================================

/**
 * Electric Field Integral Equation (EFIE)
 * 
 * Physical meaning: Enforces boundary condition on tangential electric field
 * 
 * Mathematical form:
 *   n × E_inc = n × E_scat
 * 
 * Where:
 *   E_inc: Incident electric field
 *   E_scat: Scattered electric field from surface currents
 */
typedef enum {
    MOM_FORMULATION_EFIE = 1,  // Electric Field Integral Equation
    MOM_FORMULATION_MFIE = 2,  // Magnetic Field Integral Equation
    MOM_FORMULATION_CFIE = 3   // Combined Field Integral Equation
} mom_formulation_t;

/**
 * Boundary Condition Types
 * 
 * L1 layer defines the physical meaning, not the numerical implementation
 */
typedef enum {
    MOM_BC_PEC = 1,        // Perfect Electric Conductor
    MOM_BC_PMC = 2,        // Perfect Magnetic Conductor
    MOM_BC_IMPEDANCE = 3,  // Impedance boundary condition
    MOM_BC_ABSORBING = 4   // Absorbing boundary condition
} mom_boundary_condition_t;

/**
 * Excitation Source Types (Physical Definition)
 */
typedef enum {
    MOM_EXCITATION_PLANE_WAVE = 1,    // Plane wave excitation
    MOM_EXCITATION_VOLTAGE_SOURCE = 2, // Voltage source (lumped element)
    MOM_EXCITATION_CURRENT_SOURCE = 3, // Current source (lumped element)
    MOM_EXCITATION_DIPOLE = 4,        // Dipole source
    /* Time-harmonic plane wave with sin(ωt) phase reference: same spatial e^{-jk·r} as plane wave,
     * RHS phasor multiplied by -j relative to plane wave (cos reference). */
    MOM_EXCITATION_SINUSOIDAL_PLANE_WAVE = 5
} mom_excitation_type_t;

/**
 * Plane Wave Excitation (Physical Definition)
 * 
 * This structure defines the PHYSICAL parameters of a plane wave.
 * It does NOT contain any numerical implementation details.
 */
typedef struct {
    point3d_t k_vector;      // Wave vector (direction of propagation)
    point3d_t polarization;  // Polarization vector (E-field direction)
    real_t amplitude;        // Amplitude (V/m)
    real_t phase;            // Phase (radians)
    real_t frequency;        // Frequency (Hz)
} mom_plane_wave_t;

/**
 * Voltage Source Excitation (Physical Definition)
 */
typedef struct {
    point3d_t position;      // Source position
    point3d_t direction;     // Current flow direction
    real_t voltage;          // Voltage amplitude (V)
    real_t frequency;        // Frequency (Hz)
    real_t phase;            // Phase (radians)
} mom_voltage_source_t;

/**
 * Material Properties (Physical Definition)
 * 
 * L1 layer defines material properties in physical terms.
 * Numerical implementation (complex permittivity calculation) is in L3.
 */
typedef struct {
    char name[64];           // Material name
    real_t eps_r;            // Relative permittivity
    real_t mu_r;             // Relative permeability
    real_t sigma;            // Conductivity (S/m)
    real_t tan_delta;        // Loss tangent
} mom_material_t;

/**
 * Frequency Domain Parameters (Physical Definition)
 */
typedef struct {
    real_t frequency;        // Frequency (Hz)
    real_t omega;            // Angular frequency (rad/s) = 2*pi*f
    real_t k0;               // Free space wavenumber (rad/m) = omega/c0
    real_t eta0;             // Free space impedance (Ohms) = sqrt(mu0/eps0)
} mom_frequency_domain_t;

// ============================================================================
// Physical Equation Interface (No Implementation)
// ============================================================================

/**
 * Initialize frequency domain parameters from frequency
 * 
 * This is a PHYSICAL calculation, not a numerical optimization.
 */
void mom_physics_init_frequency_domain(mom_frequency_domain_t* fd, real_t frequency);

/**
 * Compute material properties at given frequency
 * 
 * Physical calculation: eps = eps0 * eps_r * (1 - j*tan_delta)
 * Implementation details are in L3 layer.
 */
void mom_physics_compute_material_properties(
    const mom_material_t* material,
    const mom_frequency_domain_t* fd,
    complex_t* eps_complex,
    complex_t* mu_complex
);

#ifdef __cplusplus
}
#endif

#endif // MOM_PHYSICS_H
