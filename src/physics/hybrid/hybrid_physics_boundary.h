/********************************************************************************
 * Hybrid Physics Boundary Conditions (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines the PHYSICAL boundary conditions for hybrid coupling
 * between MoM, PEEC, and MTL. No numerical implementation.
 *
 * Architecture Rule: L1 layer defines WHAT the physics is, not HOW to compute it.
 ********************************************************************************/

#ifndef HYBRID_PHYSICS_BOUNDARY_H
#define HYBRID_PHYSICS_BOUNDARY_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Physical Boundary Condition Definitions
// ============================================================================

/**
 * Hybrid Coupling Types (Physical Definition)
 * 
 * Physical meaning: How different domains interact at boundaries
 */
typedef enum {
    HYBRID_COUPLING_ELECTRIC_FIELD = 1,    // Electric field coupling
    HYBRID_COUPLING_MAGNETIC_FIELD = 2,    // Magnetic field coupling
    HYBRID_COUPLING_CURRENT_DENSITY = 3,   // Current density coupling
    HYBRID_COUPLING_VOLTAGE_POTENTIAL = 4, // Voltage potential coupling
    HYBRID_COUPLING_POWER_FLOW = 5,       // Power flow coupling
    HYBRID_COUPLING_MIXED = 6              // Mixed coupling
} hybrid_coupling_type_t;

/**
 * Boundary Condition Types (Physical Definition)
 */
typedef enum {
    HYBRID_BC_DIRICHLET = 1,    // Dirichlet boundary condition (field value)
    HYBRID_BC_NEUMANN = 2,      // Neumann boundary condition (field derivative)
    HYBRID_BC_ROBIN = 3,        // Robin boundary condition (mixed)
    HYBRID_BC_CONTINUITY = 4    // Continuity condition (field continuity)
} hybrid_boundary_condition_t;

/**
 * Interface Point (Physical Definition)
 * 
 * This structure defines the PHYSICAL properties at an interface point.
 * It does NOT contain any numerical implementation details.
 */
typedef struct {
    point3d_t position;          // Physical position
    point3d_t normal;            // Normal vector
    point3d_t tangent;           // Tangent vector
    
    // Physical field quantities at interface
    vector3d_complex_t electric_field;    // Electric field (V/m)
    vector3d_complex_t magnetic_field;     // Magnetic field (A/m)
    vector3d_complex_t current_density;    // Current density (A/m²)
    complex_t voltage_potential;          // Voltage potential (V)
    
    // Boundary condition type
    hybrid_boundary_condition_t bc_type;
    
    // Coupling type
    hybrid_coupling_type_t coupling_type;
} hybrid_interface_point_t;

/**
 * Physical Coupling Conditions
 * 
 * These define the PHYSICAL relationships between domains
 */
typedef struct {
    // Continuity conditions (physical laws)
    bool enforce_electric_field_continuity;   // E-field continuity
    bool enforce_magnetic_field_continuity;   // H-field continuity
    bool enforce_current_continuity;           // Current continuity
    bool enforce_voltage_continuity;          // Voltage continuity
    
    // Power conservation
    bool enforce_power_conservation;            // Power conservation at interface
    
    // Coupling strength
    real_t coupling_strength;                  // Physical coupling strength (0-1)
} hybrid_coupling_conditions_t;

// ============================================================================
// Physical Equation Interface (No Implementation)
// ============================================================================

/**
 * Check physical consistency at interface point
 * 
 * Physical check: Verify that boundary conditions are physically consistent
 */
bool hybrid_physics_check_interface_consistency(
    const hybrid_interface_point_t* point
);

/**
 * Compute coupling strength from physical parameters
 * 
 * Physical calculation: Based on material properties and geometry
 */
real_t hybrid_physics_compute_coupling_strength(
    const hybrid_interface_point_t* point1,
    const hybrid_interface_point_t* point2,
    real_t distance
);

#ifdef __cplusplus
}
#endif

#endif // HYBRID_PHYSICS_BOUNDARY_H
