/********************************************************************************
 * Green's Function Operators (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines Green's function operators.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines operators, not numerical backend optimizations.
 ********************************************************************************/

#ifndef GREENS_FUNCTION_H
#define GREENS_FUNCTION_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Green's Function Types
// ============================================================================

/**
 * Green's Function Kernel Types
 * 
 * L3 layer defines the mathematical form of operators
 */
typedef enum {
    GREENS_KERNEL_G = 1,                    // Green's function G
    GREENS_KERNEL_GRAD_G = 2,              // Gradient of Green's function ∇G
    GREENS_KERNEL_G_R_R_PRIME = 3,         // G/r² for specific integrals
    GREENS_KERNEL_DOUBLE_GRAD_G = 4        // Double gradient ∇∇G
} greens_kernel_type_t;

// Backward compatibility macros
#define KERNEL_G GREENS_KERNEL_G
#define KERNEL_GRAD_G GREENS_KERNEL_GRAD_G
#define KERNEL_G_R_R_PRIME GREENS_KERNEL_G_R_R_PRIME
#define KERNEL_DOUBLE_GRAD_G GREENS_KERNEL_DOUBLE_GRAD_G

/**
 * Layered Medium Structure
 * 
 * L3 layer defines the medium structure for operator evaluation
 */
typedef struct {
    complex_t permittivity;    // Complex permittivity
    complex_t permeability;    // Complex permeability  
    complex_t impedance;       // Wave impedance
    real_t thickness;          // Layer thickness
    real_t conductivity;      // Conductivity
} layered_medium_t;

// ============================================================================
// Green's Function Operator Interface
// ============================================================================

/**
 * Free-space Green's function
 * 
 * Operator: G(r) = exp(-jk*r) / (4*π*r)
 * 
 * L3 layer defines the operator, not the numerical implementation
 */
complex_t greens_function_free_space(
    real_t r,      // Distance between source and observation points
    real_t k       // Wavenumber (2π/λ)
);

/**
 * Gradient of free-space Green's function
 * 
 * Operator: ∇G(r) = -jk * (r_vec/r) * G(r)
 */
void greens_function_gradient_free_space(
    real_t r,                    // Distance
    real_t k,                    // Wavenumber
    const real_t* r_vec,         // Vector from source to observation point [3]
    complex_t* gradient          // Output gradient vector [3]
);

/**
 * Layered media Green's function
 * 
 * Operator: Uses Sommerfeld integral representation
 * 
 * L3 layer defines the operator form, numerical evaluation is in L4
 */
complex_t greens_function_layered_media(
    real_t rho,                  // Horizontal distance
    real_t z,                       // Observation point z-coordinate
    real_t z_prime,                 // Source point z-coordinate
    real_t k0,                      // Free-space wavenumber
    int n_layers,                   // Number of layers
    const layered_medium_t* layers  // Layer properties array
);

/**
 * Periodic Green's function
 * 
 * Operator: Periodic extension using Floquet harmonics
 */
complex_t greens_function_periodic(
    real_t r,                      // Distance
    real_t k,                      // Wavenumber
    const real_t* periodicity,     // Array periodicity vector [2]
    int n_harmonics                // Number of Floquet harmonics
);

#ifdef __cplusplus
}
#endif

#endif // GREENS_FUNCTION_H
