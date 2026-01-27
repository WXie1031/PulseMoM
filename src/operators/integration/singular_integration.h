/********************************************************************************
 * Singular Integration (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines singular integral treatment methods.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 *
 * Architecture Rule: L3 defines integration methods, not solver optimizations.
 ********************************************************************************/

#ifndef SINGULAR_INTEGRATION_H
#define SINGULAR_INTEGRATION_H

#include "../../common/types.h"
#include "../../common/constants.h"
#include "integration_utils.h"
#include "../kernels/greens_function.h"  // For greens_kernel_type_t

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Singular Integration Types
// ============================================================================

/**
 * Singularity Type
 */
typedef enum {
    SINGULARITY_NONE = 0,           // No singularity
    SINGULARITY_WEAK = 1,           // Weak singularity (1/r)
    SINGULARITY_STRONG = 2,         // Strong singularity (1/r²)
    SINGULARITY_HYPERSINGULAR = 3   // Hypersingular (1/r³)
} singularity_type_t;

/**
 * Singular Integration Method
 */
typedef enum {
    SINGULAR_METHOD_DUFFY = 1,      // Duffy transformation
    SINGULAR_METHOD_POLAR = 2,      // Polar coordinate transformation
    SINGULAR_METHOD_ANALYTIC = 3,   // Analytic integration
    SINGULAR_METHOD_ADAPTIVE = 4    // Adaptive quadrature
} singular_method_t;

// ============================================================================
// Singular Integration Interface
// ============================================================================

/**
 * Integrate triangle with singularity treatment
 * 
 * L3 layer defines the integration method, not solver-specific optimizations
 */
complex_t singular_integration_triangle(
    const real_t* triangle_vertices,  // Triangle vertices [3][3]
    const real_t* obs_point,          // Observation point [3]
    real_t k,                         // Wavenumber
    greens_kernel_type_t kernel_type, // Kernel type
    singularity_type_t sing_type,     // Singularity type
    singular_method_t method          // Integration method
);

/**
 * Integrate rectangle with singularity treatment
 */
complex_t singular_integration_rectangle(
    const real_t* rect_vertices,      // Rectangle vertices [4][3]
    const real_t* obs_point,          // Observation point [3]
    real_t k,                         // Wavenumber
    greens_kernel_type_t kernel_type,
    singularity_type_t sing_type,
    singular_method_t method
);

/**
 * Detect singularity type
 * 
 * L3 layer determines singularity type based on geometry
 */
singularity_type_t singular_integration_detect_type(
    const real_t* source_vertices,
    int num_vertices,
    const real_t* obs_point,
    real_t tolerance
);

/**
 * Apply Duffy transformation
 * 
 * L3 layer defines the transformation, not numerical implementation details
 */
int singular_integration_duffy_transform(
    const real_t* triangle_vertices,
    const real_t* obs_point,
    real_t* transformed_points,      // Output: transformed quadrature points
    real_t* jacobians,                // Output: Jacobian determinants
    int num_points
);

#ifdef __cplusplus
}
#endif

#endif // SINGULAR_INTEGRATION_H
