/********************************************************************************
 * Material Library (L1/L3 Interface)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines a library of common materials for electromagnetic simulation.
 * Materials are defined in L1 layer (physics), used in L3 layer (operators).
 ********************************************************************************/

#ifndef MATERIAL_LIBRARY_H
#define MATERIAL_LIBRARY_H

#include "../common/types.h"
#include "../physics/mom/mom_physics.h"
#include "../physics/peec/peec_physics.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Material Library
// ============================================================================

/**
 * Common dielectric materials
 */
typedef enum {
    MATERIAL_FREE_SPACE = 0,
    MATERIAL_FR4 = 1,              // FR4 PCB substrate
    MATERIAL_ROGERS_RO4003 = 2,    // Rogers RO4003
    MATERIAL_ROGERS_RO4350 = 3,    // Rogers RO4350
    MATERIAL_TACONIC_TLY = 4,      // Taconic TLY
    MATERIAL_AIR = 5,              // Air
    MATERIAL_SILICON = 6,          // Silicon
    MATERIAL_GALLIUM_ARSENIDE = 7, // GaAs
    MATERIAL_ALUMINA = 8,          // Alumina (Al2O3)
    MATERIAL_TEFLON = 9            // Teflon (PTFE)
} material_type_t;

/**
 * Get material properties by type
 * 
 * L1 layer provides physical material definitions
 */
int material_library_get_mom_material(
    material_type_t type,
    mom_material_t* material
);

/**
 * Get PEEC material properties by type
 */
int material_library_get_peec_material(
    material_type_t type,
    peec_material_t* material
);

/**
 * Get material name
 */
const char* material_library_get_name(material_type_t type);

/**
 * Get material properties at frequency
 * 
 * For frequency-dependent materials, computes properties at given frequency
 */
int material_library_get_at_frequency(
    material_type_t type,
    real_t frequency,
    complex_t* eps_complex,
    complex_t* mu_complex,
    real_t* conductivity
);

#ifdef __cplusplus
}
#endif

#endif // MATERIAL_LIBRARY_H
