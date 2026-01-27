/**
 * @file integral_nearly_singular.c
 * @brief Nearly singular integral handling for closely spaced elements
 * @details Implements specialized numerical integration for near-singular cases
 *          where elements are close but not touching (no true singularity)
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "core_common.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../operators/kernels/core_kernels.h"
#include "integration_utils.h"
#include "integration_utils_optimized.h"
#include <math.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Threshold for near-singular detection (relative to element size)
#define NEAR_SINGULAR_THRESHOLD 0.1  // 10% of characteristic size

// High-order Gauss quadrature for near-singular integrals
#define NEAR_SINGULAR_GAUSS_ORDER 8  // Use 8-point quadrature for accuracy

// Note: Triangle degeneracy threshold is now defined in core_common.h as NUMERICAL_EPSILON
// Use NUMERICAL_EPSILON from core_common.h instead of local definition

/*********************************************************************
 * Near-Singular Detection
 *********************************************************************/

/**
 * @brief Detect if two elements are in near-singular configuration
 * @param elem1 First element
 * @param elem2 Second element
 * @param threshold Distance threshold (relative to element size)
 * @return true if near-singular, false otherwise
 */
bool integral_detect_nearly_singular(const geom_triangle_t* elem1,
                                     const geom_triangle_t* elem2,
                                     double threshold) {
    if (!elem1 || !elem2) {
        return false;
    }
    
    // Compute centroids
    geom_point_t c1, c2;
    geom_triangle_get_centroid(elem1, &c1);
    geom_triangle_get_centroid(elem2, &c2);
    
    // Compute distance between centroids
    double dist = geom_point_distance(&c1, &c2);
    
    // Compute characteristic size (average of element sizes)
    double size1 = sqrt(elem1->area);
    double size2 = sqrt(elem2->area);
    double char_size = (size1 + size2) / 2.0;
    
    // Check if distance is below threshold
    return (dist < threshold * char_size);
}

/*********************************************************************
 * High-Order Gauss Quadrature for Near-Singular Integrals
 *********************************************************************/

// Note: Gauss quadrature points and weights are now provided by
// gauss_quadrature_triangle() in integration_utils.c

/**
 * @brief Compute near-singular integral using high-order quadrature
 * @param tri Source triangle
 * @param obs_point Observation point
 * @param kernel_func Kernel function to integrate
 * @param k Wavenumber
 * @param kernel_data Additional data for kernel function
 * @return Complex integral value
 */
complex_t integral_nearly_singular_triangle(const geom_triangle_t* tri,
                                           const geom_point_t* obs_point,
                                           complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                           double k,
                                           void* kernel_data) {
    // Input validation
    if (!tri || !obs_point || !kernel_func) {
        return complex_zero();
    }
    
    // Validate triangle area
    if (tri->area <= 0.0) {
        return complex_zero();
    }
    
    complex_t integral = complex_zero();
    
    // Use high-order Gauss quadrature (8-point for near-singular accuracy)
    // Try to use cached lookup table first for performance
    double gauss_points[NEAR_SINGULAR_GAUSS_ORDER][2];
    double gauss_weights[NEAR_SINGULAR_GAUSS_ORDER];
    if (!integration_get_cached_triangle_quadrature(NEAR_SINGULAR_GAUSS_ORDER, gauss_points, gauss_weights)) {
        // Fallback to regular function if not cached
        gauss_quadrature_triangle(NEAR_SINGULAR_GAUSS_ORDER, gauss_points, gauss_weights);
    }
    
    for (int i = 0; i < NEAR_SINGULAR_GAUSS_ORDER; i++) {
        double u = gauss_points[i][0];
        double v = gauss_points[i][1];
        double w = 1.0 - u - v;
        
        // Validate barycentric coordinates (should sum to 1, allow small numerical error)
        if (fabs(u + v + w - 1.0) > 1e-10) {
            // Skip invalid quadrature point
            continue;
        }
        
        // Interpolate point on triangle using barycentric coordinates
        geom_point_t r_prime;
        geom_triangle_interpolate_point(tri, u, v, w, &r_prime);
        
        // Evaluate kernel
        complex_t kernel_val = kernel_func(obs_point, &r_prime, k, kernel_data);
        
        // Accumulate with weight
        double weight = gauss_weights[i] * tri->area;
        integral.re += kernel_val.re * weight;
        integral.im += kernel_val.im * weight;
    }
    
    return integral;
}

/*********************************************************************
 * Adaptive Subdivision for Near-Singular Integrals
 *********************************************************************/

/**
 * @brief Compute near-singular integral with adaptive subdivision
 * @param tri Source triangle
 * @param obs_point Observation point
 * @param kernel_func Kernel function
 * @param k Wavenumber
 * @param kernel_data Kernel data
 * @param max_subdivisions Maximum subdivision levels
 * @param tolerance Error tolerance
 * @return Complex integral value
 */
complex_t integral_nearly_singular_adaptive(const geom_triangle_t* tri,
                                           const geom_point_t* obs_point,
                                           complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                           double k,
                                           void* kernel_data,
                                           int max_subdivisions,
                                           double tolerance) {
    // Input validation
    if (!tri || !obs_point || !kernel_func) {
        return complex_zero();
    }
    
    // Validate subdivision parameters
    if (max_subdivisions <= 0) {
        // If no subdivisions allowed, use standard quadrature
        return integral_nearly_singular_triangle(tri, obs_point, kernel_func, k, kernel_data);
    }
    
    // Validate triangle area
    if (tri->area <= 0.0) {
        return complex_zero();
    }
    
    // Start with standard high-order quadrature
    complex_t integral = integral_nearly_singular_triangle(tri, obs_point, kernel_func, k, kernel_data);
    
    // Check if subdivision is needed
    // Compute distance to triangle centroid
    geom_point_t centroid;
    geom_triangle_get_centroid(tri, &centroid);
    double dist = geom_point_distance(obs_point, &centroid);
    double char_size = sqrt(tri->area);
    
    // Validate inputs and check subdivision condition
    if (char_size <= 0.0 || dist < 0.0) {
        // Invalid triangle or distance, return zero
        return complex_zero();
    }
    
    // If very close, use subdivision
    if (dist < char_size * NEAR_SINGULAR_THRESHOLD && max_subdivisions > 0) {
        // Subdivide triangle into 4 sub-triangles
        geom_triangle_t sub_tri[4];
        geom_triangle_subdivide(tri, sub_tri);
        
        // Recursively integrate sub-triangles
        complex_t sub_integral = complex_zero();
        for (int i = 0; i < 4; i++) {
            complex_t sub_val = integral_nearly_singular_adaptive(
                &sub_tri[i], obs_point, kernel_func, k, kernel_data,
                max_subdivisions - 1, tolerance);
            sub_integral.re += sub_val.re;
            sub_integral.im += sub_val.im;
        }
        
        // Simple error estimation: compare with initial quadrature result
        // If the difference is small relative to tolerance, use subdivided result
        double error_estimate = sqrt((sub_integral.re - integral.re) * (sub_integral.re - integral.re) +
                                     (sub_integral.im - integral.im) * (sub_integral.im - integral.im));
        double result_magnitude = sqrt(sub_integral.re * sub_integral.re + sub_integral.im * sub_integral.im);
        
        // Use subdivided result if error is acceptable or if result is small
        if (error_estimate < tolerance * fmax(result_magnitude, 1.0) || result_magnitude < tolerance) {
            integral = sub_integral;
        }
        // Otherwise, keep the initial quadrature result (which may be less accurate but more stable)
    }
    
    return integral;
}
