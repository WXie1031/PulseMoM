/********************************************************************************
 * PEEC Integral Kernels (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines PEEC integral kernels.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines operators, not numerical backend optimizations.
 ********************************************************************************/

#ifndef PEEC_KERNEL_H
#define PEEC_KERNEL_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../physics/peec/peec_physics.h"
#include "greens_function.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// PEEC Kernel Element Types
// ============================================================================

/**
 * Rectangle Element for PEEC Integration
 */
typedef struct {
    real_t vertices[4][3];    // Rectangle vertices [4][x,y,z]
    real_t normal[3];         // Surface normal
    real_t area;              // Rectangle area
} peec_rectangle_element_t;

/**
 * Wire Element for PEEC Integration
 */
typedef struct {
    real_t start[3];          // Wire start point
    real_t end[3];            // Wire end point
    real_t radius;            // Wire radius
    real_t length;            // Wire length
} peec_wire_element_t;

/**
 * PEEC Integral Kernel
 * 
 * L3 layer defines the kernel operator for PEEC equations
 */
typedef struct {
    peec_formulation_t formulation;  // Classical, Modified, or Full-wave
    
    // Operator parameters
    real_t frequency;                 // Frequency (Hz)
    real_t k0;                        // Free-space wavenumber
    
    // Medium parameters
    layered_medium_t* medium;         // Layered medium (if applicable)
    int n_layers;                     // Number of layers
} peec_kernel_t;

// ============================================================================
// PEEC Kernel Operator Interface
// ============================================================================

/**
 * Create PEEC kernel operator
 */
peec_kernel_t* peec_kernel_create(
    peec_formulation_t formulation,
    real_t frequency
);

/**
 * Destroy PEEC kernel operator
 */
void peec_kernel_destroy(peec_kernel_t* kernel);

/**
 * Set material properties for PEEC kernel
 * 
 * Updates capacitance, inductance, and resistance calculations
 */
int peec_kernel_set_material(
    peec_kernel_t* kernel,
    const peec_material_t* material
);

/**
 * Set layered medium for PEEC kernel
 * 
 * Enables layered media effects in PEEC calculations
 */
int peec_kernel_set_layered_medium(
    peec_kernel_t* kernel,
    const layered_medium_t* medium,
    int n_layers
);

/**
 * Evaluate partial inductance kernel
 * 
 * Operator: L_ij = (μ/4π) * ∫∫ (1/r) dS_i dS_j
 */
real_t peec_kernel_evaluate_inductance(
    const peec_kernel_t* kernel,
    const peec_rectangle_element_t* source_elem,
    const peec_rectangle_element_t* test_elem
);

/**
 * Evaluate potential coefficient kernel
 * 
 * Operator: P_ij = (1/4πε) * ∫∫ (1/r) dS_i dS_j
 */
real_t peec_kernel_evaluate_potential_coefficient(
    const peec_kernel_t* kernel,
    const peec_rectangle_element_t* source_elem,
    const peec_rectangle_element_t* test_elem
);

/**
 * Evaluate resistance kernel
 * 
 * Operator: R = ρ * l / A (with skin effect)
 */
real_t peec_kernel_evaluate_resistance(
    const peec_kernel_t* kernel,
    const peec_wire_element_t* wire,
    real_t conductivity,
    real_t skin_depth
);

#ifdef __cplusplus
}
#endif

#endif // PEEC_KERNEL_H
