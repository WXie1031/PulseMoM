/********************************************************************************
 * MTL Physics Definition (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file contains ONLY the physical equations (Telegrapher equations)
 * for Multiconductor Transmission Lines. No numerical implementation.
 *
 * Architecture Rule: L1 layer defines WHAT the physics is, not HOW to compute it.
 ********************************************************************************/

#ifndef MTL_PHYSICS_H
#define MTL_PHYSICS_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Physical Equation Definitions
// ============================================================================

/**
 * Telegrapher Equations for MTL
 * 
 * Physical meaning: Describes voltage and current propagation along transmission lines
 * 
 * Mathematical form:
 *   dV/dz = -Z * I
 *   dI/dz = -Y * V
 * 
 * Where:
 *   V: Voltage vector
 *   I: Current vector
 *   Z: Series impedance matrix per unit length
 *   Y: Shunt admittance matrix per unit length
 */

/**
 * MTL Cable Types (Physical Classification)
 */
typedef enum {
    MTL_CABLE_COAXIAL = 1,          // Coaxial cable
    MTL_CABLE_TWISTED_PAIR = 2,     // Twisted pair
    MTL_CABLE_RIBBON = 3,           // Ribbon cable
    MTL_CABLE_HARNESS = 4,          // Cable harness
    MTL_CABLE_MULTICORE = 5,        // Multicore cable
    MTL_CABLE_SHIELDED_PAIR = 6,    // Shielded twisted pair
    MTL_CABLE_CUSTOM = 7            // User-defined geometry
} mtl_cable_type_t;

/**
 * MTL Conductor Material (Physical Definition)
 */
typedef enum {
    MTL_MATERIAL_COPPER = 1,        // Copper
    MTL_MATERIAL_ALUMINUM = 2,      // Aluminum
    MTL_MATERIAL_SILVER = 3,        // Silver
    MTL_MATERIAL_GOLD = 4,          // Gold
    MTL_MATERIAL_STEEL = 5,         // Steel
    MTL_MATERIAL_TIN = 6,           // Tin-plated
    MTL_MATERIAL_CUSTOM = 7         // User-defined properties
} mtl_conductor_material_t;

/**
 * MTL Dielectric Material (Physical Definition)
 */
typedef enum {
    MTL_DIELECTRIC_PVC = 1,         // Polyvinyl chloride
    MTL_DIELECTRIC_PE = 2,          // Polyethylene
    MTL_DIELECTRIC_PTFE = 3,        // Teflon
    MTL_DIELECTRIC_RUBBER = 4,      // Rubber
    MTL_DIELECTRIC_XLPE = 5,        // Cross-linked polyethylene
    MTL_DIELECTRIC_CUSTOM = 6       // User-defined properties
} mtl_dielectric_material_t;

/**
 * MTL Physical Parameters
 * 
 * This structure defines the PHYSICAL parameters of an MTL.
 * It does NOT contain any numerical implementation details.
 */
typedef struct {
    int num_conductors;              // Number of conductors
    real_t length;                   // Transmission line length (m)
    real_t frequency;                // Analysis frequency (Hz)
    
    // Per-unit-length parameters (physical quantities)
    complex_t* series_impedance;     // Z matrix (Ohms/m) - [num_conductors x num_conductors]
    complex_t* shunt_admittance;     // Y matrix (S/m) - [num_conductors x num_conductors]
    
    // Material properties
    mtl_conductor_material_t* conductor_materials; // Material for each conductor
    mtl_dielectric_material_t dielectric_material; // Dielectric material
    
    // Physical effects
    bool include_skin_effect;        // Include skin effect
    bool include_proximity_effect;    // Include proximity effect
    bool include_radiation_loss;     // Include radiation losses
    bool include_dielectric_loss;     // Include dielectric losses
} mtl_physical_model_t;

/**
 * Frequency Domain Parameters for MTL
 */
typedef struct {
    real_t frequency;        // Frequency (Hz)
    real_t omega;            // Angular frequency (rad/s)
    real_t k0;               // Free space wavenumber (rad/m)
} mtl_frequency_domain_t;

// ============================================================================
// Physical Equation Interface (No Implementation)
// ============================================================================

/**
 * Initialize frequency domain parameters
 */
void mtl_physics_init_frequency_domain(mtl_frequency_domain_t* fd, real_t frequency);

/**
 * Compute characteristic impedance (physical calculation)
 * 
 * Physical formula: Z0 = sqrt(Z / Y)
 * Where Z and Y are per-unit-length impedance and admittance
 */
void mtl_physics_compute_characteristic_impedance(
    const mtl_physical_model_t* model,
    complex_t* z0_matrix  // Output: [num_conductors x num_conductors]
);

/**
 * Compute propagation constant (physical calculation)
 * 
 * Physical formula: gamma = sqrt(Z * Y)
 */
void mtl_physics_compute_propagation_constant(
    const mtl_physical_model_t* model,
    complex_t* gamma_matrix  // Output: [num_conductors x num_conductors]
);

#ifdef __cplusplus
}
#endif

#endif // MTL_PHYSICS_H
