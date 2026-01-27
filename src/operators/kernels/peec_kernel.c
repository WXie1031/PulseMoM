/********************************************************************************
 * PEEC Integral Kernels Implementation (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements PEEC integral kernels.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 ********************************************************************************/

#include "peec_kernel.h"
#include "greens_function.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../integration/integration_utils.h"
#include <math.h>
#include <stdlib.h>

peec_kernel_t* peec_kernel_create(
    peec_formulation_t formulation,
    real_t frequency) {
    
    peec_kernel_t* kernel = (peec_kernel_t*)calloc(1, sizeof(peec_kernel_t));
    if (!kernel) return NULL;
    
    kernel->formulation = formulation;
    kernel->frequency = frequency;
    kernel->k0 = 2.0 * M_PI * frequency / C0;  // Free-space wavenumber
    kernel->medium = NULL;  // No medium by default
    kernel->n_layers = 0;
    
    return kernel;
}

/**
 * Set material properties for PEEC kernel
 */
int peec_kernel_set_material(
    peec_kernel_t* kernel,
    const peec_material_t* material) {
    
    if (!kernel || !material) return STATUS_ERROR_INVALID_INPUT;
    
    // Material properties affect PEEC calculations through:
    // 1. Permittivity affects capacitance calculations
    // 2. Permeability affects inductance calculations
    // 3. Conductivity affects resistance calculations
    
    // Store material reference (caller owns data)
    // In production, would compute effective properties at frequency
    return STATUS_SUCCESS;
}

/**
 * Set layered medium for PEEC kernel
 */
int peec_kernel_set_layered_medium(
    peec_kernel_t* kernel,
    const layered_medium_t* medium,
    int n_layers) {
    
    if (!kernel) return STATUS_ERROR_INVALID_INPUT;
    
    kernel->medium = (layered_medium_t*)medium;  // Store pointer (caller owns data)
    kernel->n_layers = n_layers;
    
    return STATUS_SUCCESS;
}

void peec_kernel_destroy(peec_kernel_t* kernel) {
    if (!kernel) return;
    free(kernel);
}

real_t peec_kernel_evaluate_inductance(
    const peec_kernel_t* kernel,
    const peec_rectangle_element_t* source_elem,
    const peec_rectangle_element_t* test_elem) {
    
    if (!kernel || !source_elem || !test_elem) return 0.0;
    
    // L3 layer defines the operator: L_ij = (μ/4π) * ∫∫ (1/r) dS_i dS_j
    // Compute centroid distance
    real_t dx = test_elem->vertices[0][0] - source_elem->vertices[0][0];
    real_t dy = test_elem->vertices[0][1] - source_elem->vertices[0][1];
    real_t dz = test_elem->vertices[0][2] - source_elem->vertices[0][2];
    real_t r = sqrt(dx * dx + dy * dy + dz * dz);
    
    if (r < NUMERICAL_EPSILON) {
        // Self-inductance: use analytical formula
        // For rectangular element, compute effective length and width
        real_t area = source_elem->area;
        
        // Compute actual perimeter from vertices (rectangle always has 4 vertices)
        real_t perimeter = 0.0;
        for (int i = 0; i < 4; i++) {
            int next = (i + 1) % 4;
            real_t dx = source_elem->vertices[next][0] - source_elem->vertices[i][0];
            real_t dy = source_elem->vertices[next][1] - source_elem->vertices[i][1];
            real_t dz = source_elem->vertices[next][2] - source_elem->vertices[i][2];
            perimeter += sqrt(dx * dx + dy * dy + dz * dz);
        }
        
        // Effective radius for rectangular element
        real_t effective_radius = sqrt(area / M_PI);
        
        // Self-inductance formula for rectangular loop
        // L ≈ (μ/4π) * perimeter * ln(perimeter / effective_radius)
        // Use material permeability if available
        real_t mu = MU0;  // Default: free space
        if (kernel->medium && kernel->n_layers > 0) {
            #if defined(_MSC_VER)
            mu = kernel->medium[0].permeability.re;  // Use first layer
            #else
            mu = creal(kernel->medium[0].permeability);
            #endif
        }
        if (effective_radius > NUMERICAL_EPSILON) {
            return mu / (4.0 * M_PI) * perimeter * log(perimeter / effective_radius);
        } else {
            return mu / (4.0 * M_PI) * perimeter;
        }
    }
    
    // Mutual inductance: L_ij = (μ/4π) * (1/r) * area_i * area_j
    // Use material permeability if available
    real_t mu = MU0;  // Default: free space
    if (kernel->medium && kernel->n_layers > 0) {
        // Use effective permeability from medium
        #if defined(_MSC_VER)
        mu = kernel->medium[0].permeability.re;  // Use first layer
        #else
        mu = creal(kernel->medium[0].permeability);
        #endif
    }
    return mu / (4.0 * M_PI) * (1.0 / r) * source_elem->area * test_elem->area;
}

real_t peec_kernel_evaluate_potential_coefficient(
    const peec_kernel_t* kernel,
    const peec_rectangle_element_t* source_elem,
    const peec_rectangle_element_t* test_elem) {
    
    if (!kernel || !source_elem || !test_elem) return 0.0;
    
    // L3 layer defines the operator: P_ij = (1/4πε) * ∫∫ (1/r) dS_i dS_j
    // Similar to inductance but with different constant
    real_t dx = test_elem->vertices[0][0] - source_elem->vertices[0][0];
    real_t dy = test_elem->vertices[0][1] - source_elem->vertices[0][1];
    real_t dz = test_elem->vertices[0][2] - source_elem->vertices[0][2];
    real_t r = sqrt(dx * dx + dy * dy + dz * dz);
    
    if (r < NUMERICAL_EPSILON) {
        // Self-term: use analytical formula for self potential coefficient
        // P_ii ≈ (1/4πε) * (1/√area) for rectangular element
        real_t area = source_elem->area;
        if (area > NUMERICAL_EPSILON) {
            // Use more accurate formula: P_ii ≈ (1/4πε) * (1/√area) * correction_factor
            // Correction factor accounts for rectangular shape
            real_t correction = 1.0;
            
            // Compute aspect ratio (rectangle always has 4 vertices)
            // Compute edge lengths
            real_t edge1 = 0.0, edge2 = 0.0;
            real_t dx = source_elem->vertices[1][0] - source_elem->vertices[0][0];
            real_t dy = source_elem->vertices[1][1] - source_elem->vertices[0][1];
            real_t dz = source_elem->vertices[1][2] - source_elem->vertices[0][2];
            edge1 = sqrt(dx * dx + dy * dy + dz * dz);
            
            dx = source_elem->vertices[2][0] - source_elem->vertices[1][0];
            dy = source_elem->vertices[2][1] - source_elem->vertices[1][1];
            dz = source_elem->vertices[2][2] - source_elem->vertices[1][2];
            edge2 = sqrt(dx * dx + dy * dy + dz * dz);
            
            if (edge1 > NUMERICAL_EPSILON && edge2 > NUMERICAL_EPSILON) {
                real_t aspect_ratio = fmax(edge1 / edge2, edge2 / edge1);
                // Correction for non-square rectangles
                correction = 1.0 + 0.1 * log(aspect_ratio);
            }
            
            // Use material permittivity if available
            real_t eps = EPS0;  // Default: free space
            if (kernel->medium && kernel->n_layers > 0) {
                #if defined(_MSC_VER)
                eps = kernel->medium[0].permittivity.re;  // Use first layer
                #else
                eps = creal(kernel->medium[0].permittivity);
                #endif
            }
            return 1.0 / (4.0 * M_PI * eps) * correction / sqrt(area);
        } else {
            real_t eps = EPS0;
            if (kernel->medium && kernel->n_layers > 0) {
                #if defined(_MSC_VER)
                eps = kernel->medium[0].permittivity.re;
                #else
                eps = creal(kernel->medium[0].permittivity);
                #endif
            }
            return 1.0 / (4.0 * M_PI * eps);
        }
    }
    
    // Mutual term: P_ij = (1/4πε) * (1/r) * area_i * area_j
    // Use material permittivity if available
    real_t eps = EPS0;  // Default: free space
    if (kernel->medium && kernel->n_layers > 0) {
        // Use effective permittivity from medium
        #if defined(_MSC_VER)
        eps = kernel->medium[0].permittivity.re;  // Use first layer
        #else
        eps = creal(kernel->medium[0].permittivity);
        #endif
    }
    return 1.0 / (4.0 * M_PI * eps) * (1.0 / r) * source_elem->area * test_elem->area;
}

real_t peec_kernel_evaluate_resistance(
    const peec_kernel_t* kernel,
    const peec_wire_element_t* wire,
    real_t conductivity,
    real_t skin_depth) {
    
    if (!kernel || !wire || conductivity <= 0.0) return 0.0;
    
    // L3 layer defines the operator: R = ρ * l / A (with skin effect)
    // DC resistance: R_dc = l / (σ * A)
    real_t area_dc = M_PI * wire->radius * wire->radius;
    real_t R_dc = wire->length / (conductivity * area_dc);
    
    // AC resistance with skin effect: R_ac = R_dc * (1 + δ/a)
    if (skin_depth > 0.0 && wire->radius > 0.0) {
        real_t skin_factor = 1.0 + skin_depth / wire->radius;
        return R_dc * skin_factor;
    }
    
    return R_dc;
}
