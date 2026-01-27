/**
 * @file kernel_cfie.c
 * @brief Combined Field Integral Equation (CFIE) kernel implementation
 * @details Implements CFIE as combination of EFIE and MFIE to avoid interior resonance
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "core_kernels.h"
#include "kernel_mfie.h"
#include "../../discretization/geometry/core_geometry.h"
#include "core_common.h"
#include "electromagnetic_kernels.h"
#include "../../operators/assembler/core_assembler.h"
#include <math.h>
#include <string.h>

// Default CFIE combination parameter (α = 0.5 is common)
// Note: DEFAULT_CFIE_ALPHA is defined in core_common.h

/*********************************************************************
 * CFIE Kernel: Combined EFIE + MFIE
 *********************************************************************/

/**
 * @brief Compute CFIE kernel: α * EFIE + (1-α) * MFIE
 * @param tri Source triangle
 * @param obs_point Observation point
 * @param n_prime Normal vector at source point
 * @param k Wavenumber
 * @param alpha Combination parameter (0 = pure MFIE, 1 = pure EFIE)
 * @param efie_kernel EFIE kernel value (precomputed)
 * @param mfie_kernel MFIE kernel value (precomputed)
 * @return Combined kernel value
 */
complex_t kernel_cfie_combined(const geom_triangle_t* tri,
                               const geom_point_t* obs_point,
                               const geom_point_t* n_prime,
                               double k,
                               double alpha,
                               complex_t efie_kernel,
                               complex_t mfie_kernel) {
    if (!tri || !obs_point || !n_prime) {
        return complex_zero();
    }
    
    // Clamp alpha to [0, 1]
    if (alpha < 0.0) alpha = 0.0;
    if (alpha > 1.0) alpha = 1.0;
    
    // CFIE = α * EFIE + (1-α) * MFIE
    complex_t result = {
        alpha * efie_kernel.re + (1.0 - alpha) * mfie_kernel.re,
        alpha * efie_kernel.im + (1.0 - alpha) * mfie_kernel.im
    };
    
    return result;
}

/**
 * @brief Compute CFIE triangle integral
 * @param tri Source triangle
 * @param obs_point Observation point
 * @param n_prime Normal vector at source point
 * @param k Wavenumber
 * @param alpha Combination parameter
 * @return Combined integral value
 */
complex_t kernel_cfie_triangle_integral(const geom_triangle_t* tri,
                                        const geom_point_t* obs_point,
                                        const geom_point_t* n_prime,
                                        double k,
                                        double alpha) {
    if (!tri || !obs_point || !n_prime) {
        return complex_zero();
    }
    
    // Compute EFIE kernel using double surface integral
    // For single triangle integral, we use a dummy triangle at observation point
    // This is a simplified approach; full implementation would use proper EFIE kernel
    geom_triangle_t dummy_tri = {
        .vertices[0] = *obs_point,
        .vertices[1] = *obs_point,
        .vertices[2] = *obs_point,
        .area = 0.0,
        .normal = {0.0, 0.0, 0.0}  // Note: This is a struct initializer, not a function call
    };
    
    // Use integrate_triangle_triangle for EFIE (double surface integral)
    // Note: This computes the full double integral, not just the kernel
    // For proper CFIE, we would need a dedicated EFIE kernel function
    // For now, we approximate by using the triangle-triangle integral
    double frequency = k * C0 / (2.0 * M_PI);
    complex_t efie_integral = integrate_triangle_triangle(tri, &dummy_tri, frequency, KERNEL_FORMULATION_EFIELD, 4);  // Default order
    
    // For CFIE, we need the kernel value, not the full integral
    // Approximate by dividing by triangle area (this is a simplification)
    complex_t efie_kernel = complex_zero();
    if (tri->area > AREA_EPSILON) {
        // Normalize by triangle area to get kernel value
        efie_kernel.re = efie_integral.re / tri->area;
        efie_kernel.im = efie_integral.im / tri->area;
    }
    
    // Compute MFIE kernel
    complex_t mfie_kernel = kernel_mfie_triangle_integral(tri, obs_point, n_prime, k);
    
    // Combine
    return kernel_cfie_combined(tri, obs_point, n_prime, k, alpha, efie_kernel, mfie_kernel);
}

/**
 * @brief Compute CFIE self-term
 * @param tri Triangle element
 * @param alpha Combination parameter (0.0 = pure MFIE, 1.0 = pure EFIE)
 * @param k Wave number (2πf/c), if <= 0.0 uses default frequency
 * @return Combined self-term
 */
complex_t kernel_cfie_self_term(const geom_triangle_t* tri, double alpha, double k) {
    if (!tri) {
        return complex_zero();
    }
    
    // EFIE self-term: use analytic formula similar to RWG basis function
    // Z_ii = (j*k*eta0/(4π)) * (area^2 / (2*edge_length)) + regularization
    // This is the standard EFIE self-term for triangular elements
    // Use helper function for area retrieval
    double area = geom_triangle_get_area(tri);
    if (area < AREA_EPSILON) {
        return complex_zero();
    }
    
    // Compute average edge length
    double avg_edge_len = geom_triangle_compute_average_edge_length(tri);
    if (avg_edge_len < AREA_EPSILON) {
        avg_edge_len = sqrt(area) * 1.519671371; // Approximate for equilateral triangle
    }
    
    // Use provided k, or default frequency if k <= 0.0
    // Default: k = 2π/λ with λ = 1m (1 GHz)
    if (k <= 0.0) {
        double default_freq = 1e9;
        k = TWO_PI_OVER_C0 * default_freq;
    }
    double eta0 = ETA0;
    
    double area_sq = area * area;
    double denom = 2.0 * avg_edge_len + NUMERICAL_EPSILON;
    double scale = area_sq / denom;
    double k_eta0_4pi = k * eta0 / (4.0 * M_PI);
    
    complex_t efie_self;
    efie_self.re = scale / (4.0 * M_PI);  // Small real part from static contribution
    efie_self.im = k_eta0_4pi * scale;    // Dominant imaginary part (reactive term)
    
    // MFIE self-term
    complex_t mfie_self = kernel_mfie_self_term(tri);
    
    // Combine: CFIE = α * EFIE + (1-α) * MFIE
    complex_t result = {
        alpha * efie_self.re + (1.0 - alpha) * mfie_self.re,
        alpha * efie_self.im + (1.0 - alpha) * mfie_self.im
    };
    
    return result;
}

/*********************************************************************
 * CFIE Configuration
 *********************************************************************/

/**
 * @brief Get recommended CFIE alpha parameter
 * @param frequency Operating frequency
 * @param geometry_type Geometry type
 * @return Recommended alpha value
 * 
 * Common choices:
 * - α = 0.5: Balanced combination (default)
 * - α = 0.2-0.3: More MFIE (better for closed conductors)
 * - α = 0.7-0.8: More EFIE (better for open structures)
 */
double kernel_cfie_get_recommended_alpha(double frequency, 
                                        geom_element_type_t geometry_type) {
    // Default to balanced combination
    double alpha = DEFAULT_CFIE_ALPHA;
    
    // Adjust based on geometry type
    // Note: Using simplified logic since geom_element_type_t doesn't have
    // explicit CLOSED_SURFACE/OPEN_SURFACE distinction
    // In practice, this would be determined from geometry analysis
    // For now, use default balanced combination
    (void)geometry_type;  // Suppress unused parameter warning
    alpha = DEFAULT_CFIE_ALPHA;
    
    return alpha;
}
