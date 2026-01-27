/********************************************************************************
 * MoM Integral Kernels (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines MoM integral equation kernels.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines operators, not numerical backend optimizations.
 ********************************************************************************/

#ifndef MOM_KERNEL_H
#define MOM_KERNEL_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "../../physics/mom/mom_physics.h"
#include "greens_function.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// MoM Kernel Element Types
// ============================================================================

/**
 * Triangle Element for Integration
 */
typedef struct {
    real_t vertices[3][3];    // Triangle vertices [3][x,y,z]
    real_t normal[3];         // Surface normal
    real_t area;              // Triangle area
} mom_triangle_element_t;

/**
 * MoM Integral Kernel
 * 
 * L3 layer defines the kernel operator for MoM equations
 */
typedef struct {
    mom_formulation_t formulation;  // EFIE, MFIE, or CFIE
    
    // Operator parameters
    real_t frequency;              // Frequency (Hz)
    real_t k0;                     // Free-space wavenumber
    real_t eta0;                   // Free-space impedance (real)
    
    // Medium parameters
    layered_medium_t* medium;      // Layered medium (if applicable)
    int n_layers;                  // Number of layers
} mom_kernel_t;

// ============================================================================
// MoM Kernel Operator Interface
// ============================================================================

/**
 * Create MoM kernel operator
 * 
 * L3 layer creates operator definition, not numerical implementation
 */
mom_kernel_t* mom_kernel_create(
    mom_formulation_t formulation,
    real_t frequency
);

/**
 * Destroy MoM kernel operator
 */
void mom_kernel_destroy(mom_kernel_t* kernel);

/**
 * Set material properties for MoM kernel
 * 
 * Updates wavenumber and impedance based on material properties
 */
int mom_kernel_set_material(
    mom_kernel_t* kernel,
    const mom_material_t* material
);

/**
 * Set layered medium for MoM kernel
 * 
 * Enables layered media Green's function evaluation
 */
int mom_kernel_set_layered_medium(
    mom_kernel_t* kernel,
    const layered_medium_t* medium,
    int n_layers
);

/**
 * Evaluate EFIE kernel
 * 
 * Operator: Z_ij = <f_i, E_operator(f_j)>
 * 
 * L3 layer defines the operator form
 * 
 * @param kernel Kernel operator
 * @param source_tri Source triangle element
 * @param test_tri Test triangle element
 * @param source_point Source point [3]
 * @param test_point Test point [3]
 */
complex_t mom_kernel_evaluate_efie(
    const mom_kernel_t* kernel,
    const mom_triangle_element_t* source_tri,
    const mom_triangle_element_t* test_tri,
    const real_t* source_point,
    const real_t* test_point
);

/**
 * Evaluate MFIE kernel
 * 
 * Operator: Z_ij = <f_i, H_operator(f_j)>
 */
complex_t mom_kernel_evaluate_mfie(
    const mom_kernel_t* kernel,
    const mom_triangle_element_t* source_tri,
    const mom_triangle_element_t* test_tri,
    const real_t* source_point,
    const real_t* test_point
);

/**
 * Evaluate CFIE kernel
 * 
 * Operator: Z_ij = α * EFIE + (1-α) * MFIE
 * 
 * @param kernel Kernel operator
 * @param source_tri Source triangle element
 * @param test_tri Test triangle element
 * @param source_point Source point [3]
 * @param test_point Test point [3]
 * @param alpha CFIE combination parameter
 */
complex_t mom_kernel_evaluate_cfie(
    const mom_kernel_t* kernel,
    const mom_triangle_element_t* source_tri,
    const mom_triangle_element_t* test_tri,
    const real_t* source_point,
    const real_t* test_point,
    real_t alpha
);

#ifdef __cplusplus
}
#endif

#endif // MOM_KERNEL_H
