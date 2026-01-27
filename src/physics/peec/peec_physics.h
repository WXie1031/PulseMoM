/********************************************************************************
 * PEEC Physics Definition (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file contains ONLY the physical equations and circuit model definitions
 * for PEEC. No numerical implementation, no solver calls.
 *
 * Architecture Rule: L1 layer defines WHAT the physics is, not HOW to compute it.
 ********************************************************************************/

#ifndef PEEC_PHYSICS_H
#define PEEC_PHYSICS_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Physical Equation Definitions
// ============================================================================

/**
 * PEEC Formulation Types
 * 
 * Physical meaning: Different ways to model electromagnetic coupling
 */
typedef enum {
    PEEC_FORMULATION_CLASSICAL = 1,  // Classical PEEC (quasi-static)
    PEEC_FORMULATION_MODIFIED = 2,    // Modified PEEC (includes retardation)
    PEEC_FORMULATION_FULL_WAVE = 3   // Full-wave PEEC
} peec_formulation_t;

/**
 * Partial Element Types
 * 
 * Physical meaning: Circuit elements extracted from geometry
 */
typedef enum {
    PEEC_ELEMENT_RESISTANCE = 1,      // Resistance (R)
    PEEC_ELEMENT_INDUCTANCE = 2,      // Self inductance (L)
    PEEC_ELEMENT_MUTUAL_INDUCTANCE = 3, // Mutual inductance (M)
    PEEC_ELEMENT_CAPACITANCE = 4,     // Capacitance (C)
    PEEC_ELEMENT_POTENTIAL_COEFF = 5  // Potential coefficient (P)
} peec_element_type_t;

/**
 * PEEC Circuit Model (Physical Definition)
 * 
 * This structure defines the PHYSICAL circuit model.
 * It does NOT contain any numerical implementation details.
 */
typedef struct {
    // Partial elements (physical quantities)
    real_t* resistance_matrix;        // Resistance matrix (Ohms)
    real_t* inductance_matrix;     // Inductance matrix (H)
    real_t* capacitance_matrix;      // Capacitance matrix (F)
    real_t* potential_coeff_matrix;  // Potential coefficient matrix (1/F)
    real_t* mutual_inductance_matrix; // Mutual inductance matrix (H)
    
    // Circuit topology
    int num_nodes;                    // Number of circuit nodes
    int num_branches;                 // Number of circuit branches
    int num_conductors;               // Number of conductors
    
    // Physical parameters
    peec_formulation_t formulation;   // PEEC formulation type
    real_t frequency;                 // Analysis frequency (Hz)
} peec_circuit_model_t;

/**
 * Material Properties for PEEC (Physical Definition)
 */
typedef struct {
    char name[64];           // Material name
    real_t conductivity;     // Electrical conductivity (S/m)
    real_t relative_permeability; // Relative permeability
    real_t skin_depth;       // Skin depth at frequency (m)
} peec_material_t;

/**
 * Frequency Domain Parameters for PEEC
 */
typedef struct {
    real_t frequency;        // Frequency (Hz)
    real_t omega;            // Angular frequency (rad/s)
    real_t k0;               // Free space wavenumber (rad/m)
} peec_frequency_domain_t;

// ============================================================================
// Physical Equation Interface (No Implementation)
// ============================================================================

/**
 * Initialize frequency domain parameters
 */
void peec_physics_init_frequency_domain(peec_frequency_domain_t* fd, real_t frequency);

/**
 * Compute skin depth (physical calculation)
 * 
 * Physical formula: delta = sqrt(2 / (omega * mu * sigma))
 */
real_t peec_physics_compute_skin_depth(
    real_t frequency,
    real_t conductivity,
    real_t permeability
);

/**
 * Compute material properties at frequency
 */
void peec_physics_compute_material_properties(
    const peec_material_t* material,
    const peec_frequency_domain_t* fd,
    peec_material_t* material_at_freq
);

#ifdef __cplusplus
}
#endif

#endif // PEEC_PHYSICS_H
