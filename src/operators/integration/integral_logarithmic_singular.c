/********************************************************************************
 * Logarithmic Singularity Integration Implementation
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Implements specialized quadrature for log(r) type singularities
 * Used in 2D problems and edge effects
 ********************************************************************************/

#include "integral_logarithmic_singular.h"
#include "../../discretization/geometry/core_geometry.h"
#include "integration_utils.h"
#include "integration_utils_optimized.h"
#include <math.h>
#include <stdlib.h>

// Logarithmic singularity detection threshold
#define LOG_SINGULAR_THRESHOLD 1e-6
#define LOG_SINGULAR_RATIO 0.01  // Ratio of distance to edge length for singularity detection

// Gauss quadrature order for logarithmic singular integrals
#define GAUSS_ORDER_LOG 8  // High-order for logarithmic singularity (triangle)
#define GAUSS_ORDER_1D_LOG 8  // High-order for logarithmic singularity (1D edge)

/**
 * @brief Detect logarithmic singularity
 */
int integral_detect_logarithmic_singular(const geom_triangle_t* tri,
                                         const geom_point_t* obs_point,
                                         double threshold) {
    if (!tri || !obs_point) return 0;
    
    // Check if observation point is very close to triangle edges
    // Logarithmic singularity occurs when obs_point is on or very close to an edge
    
    double tri_area = geom_triangle_get_area(tri);
    if (tri_area < AREA_EPSILON) return 0;
    
    // Compute distances to edges
    geom_point_t v0 = tri->vertices[0];
    geom_point_t v1 = tri->vertices[1];
    geom_point_t v2 = tri->vertices[2];
    
    // Distance to edge v0-v1
    double dist_to_edge01 = geom_point_distance(obs_point, &v0) + geom_point_distance(obs_point, &v1);
    double edge01_len = geom_point_distance(&v0, &v1);
    if (edge01_len > NUMERICAL_EPSILON && dist_to_edge01 < threshold * edge01_len) {
        return 1;
    }
    
    // Distance to edge v1-v2
    double dist_to_edge12 = geom_point_distance(obs_point, &v1) + geom_point_distance(obs_point, &v2);
    double edge12_len = geom_point_distance(&v1, &v2);
    if (edge12_len > NUMERICAL_EPSILON && dist_to_edge12 < threshold * edge12_len) {
        return 1;
    }
    
    // Distance to edge v2-v0
    double dist_to_edge20 = geom_point_distance(obs_point, &v2) + geom_point_distance(obs_point, &v0);
    double edge20_len = geom_point_distance(&v2, &v0);
    if (edge20_len > NUMERICAL_EPSILON && dist_to_edge20 < threshold * edge20_len) {
        return 1;
    }
    
    return 0;
}

/**
 * @brief Compute logarithmic singular integral using specialized quadrature
 * 
 * Uses product integration rule: ∫ log(r) * f(r) dr
 * where r is the distance from observation point to integration point
 */
complex_t integral_logarithmic_singular_triangle(const geom_triangle_t* tri,
                                                  const geom_point_t* obs_point,
                                                  complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                                  double k,
                                                  void* data) {
    if (!tri || !obs_point || !kernel_func) {
        return complex_zero();
    }
    
    double tri_area = geom_triangle_get_area(tri);
    if (tri_area < AREA_EPSILON) {
        return complex_zero();
    }
    
    // Use high-order Gauss quadrature with logarithmic weight
    // For logarithmic singularities, we use product integration:
    // ∫ log(r) * f(r) dA ≈ Σ w_i * log(r_i) * f(r_i)
    
    double gauss_points[GAUSS_TRIANGLE_MAX_POINTS][2];
    double gauss_weights[GAUSS_TRIANGLE_MAX_POINTS];
    
    // Try to use cached lookup table first for performance
    if (!integration_get_cached_triangle_quadrature(GAUSS_ORDER_LOG, gauss_points, gauss_weights)) {
        // Fallback to regular function if not cached
        gauss_quadrature_triangle(GAUSS_ORDER_LOG, gauss_points, gauss_weights);
    }
    
    complex_t integral = complex_zero();
    int nq = gauss_quadrature_triangle_num_points(GAUSS_ORDER_LOG);
    for (int i = 0; i < nq; i++) {
        double xi = gauss_points[i][0];
        double eta = gauss_points[i][1];
        double zeta = 1.0 - xi - eta;
        
        // Ensure valid barycentric coordinates
        if (xi < 0.0) xi = 0.0;
        if (eta < 0.0) eta = 0.0;
        if (zeta < 0.0) {
            double sum = xi + eta;
            if (sum > NUMERICAL_EPSILON) {
                xi /= sum;
                eta /= sum;
                zeta = 0.0;
            } else {
                xi = ONE_THIRD;
                eta = ONE_THIRD;
                zeta = ONE_THIRD;
            }
        }
        
        // Interpolate point on triangle
        geom_point_t r_prime;
        geom_triangle_interpolate_point(tri, xi, eta, zeta, &r_prime);
        
        // Compute distance
        double r = geom_point_distance(obs_point, &r_prime) + NUMERICAL_EPSILON;
        
        // Evaluate kernel
        complex_t kernel_val = kernel_func(obs_point, &r_prime, k, data);
        
        // Logarithmic weight: log(r) for logarithmic singularity
        double log_weight = log(r);
        
        // Accumulate integral
        double weight = gauss_weights[i] * tri_area;
        integral.re += kernel_val.re * log_weight * weight;
        integral.im += kernel_val.im * log_weight * weight;
    }
    
    return integral;
}

/**
 * @brief Adaptive subdivision for logarithmic singular integrals
 */
complex_t integral_logarithmic_singular_adaptive(const geom_triangle_t* tri,
                                                  const geom_point_t* obs_point,
                                                  complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                                  double k,
                                                  void* data,
                                                  int max_depth,
                                                  double tolerance) {
    if (!tri || !obs_point || !kernel_func || max_depth <= 0) {
        return complex_zero();
    }
    
    // Check if subdivision is needed
    double tri_area = geom_triangle_get_area(tri);
    if (tri_area < AREA_EPSILON) {
        return complex_zero();
    }
    
    // Compute integral on current triangle
    complex_t integral = integral_logarithmic_singular_triangle(tri, obs_point, kernel_func, k, data);
    
    // Check convergence (simplified: subdivide if area is large)
    if (max_depth > 0 && tri_area > tolerance) {
        // Subdivide triangle
        geom_triangle_t sub_tri[4];
        geom_triangle_subdivide(tri, sub_tri);
        int num_sub = 4;  // geom_triangle_subdivide always subdivides into 4 triangles
        
        if (num_sub > 0) {
            complex_t sub_integral = complex_zero();
            for (int i = 0; i < num_sub; i++) {
                complex_t sub_result = integral_logarithmic_singular_adaptive(&sub_tri[i], obs_point,
                                                                             kernel_func, k, data,
                                                                             max_depth - 1, tolerance);
                sub_integral.re += sub_result.re;
                sub_integral.im += sub_result.im;
            }
            
            // Use subdivided result if it's more accurate
            double integral_mag = sqrt(integral.re * integral.re + integral.im * integral.im);
            double sub_mag = sqrt(sub_integral.re * sub_integral.re + sub_integral.im * sub_integral.im);
            
            if (sub_mag > NUMERICAL_EPSILON && fabs(integral_mag - sub_mag) / sub_mag < tolerance) {
                return sub_integral;
            }
        }
    }
    
    return integral;
}

/**
 * @brief Compute 2D logarithmic singular integral for edge problems
 * 
 * For edge integrals: ∫ log(|r - r'|) * f(r') dl'
 * where integration is along an edge
 */
complex_t integral_logarithmic_singular_edge(const geom_point_t* edge_start,
                                              const geom_point_t* edge_end,
                                              const geom_point_t* obs_point,
                                              complex_t (*kernel_func)(const geom_point_t*, const geom_point_t*, double, void*),
                                              double k,
                                              void* data) {
    if (!edge_start || !edge_end || !obs_point || !kernel_func) {
        return complex_zero();
    }
    
    double edge_len = geom_point_distance(edge_start, edge_end);
    if (edge_len < NUMERICAL_EPSILON) {
        return complex_zero();
    }
    
    // Use 1D Gauss quadrature with logarithmic weight
    double gauss_points_1d[GAUSS_ORDER_1D_LOG];
    double gauss_weights_1d[GAUSS_ORDER_1D_LOG];
    
    gauss_quadrature_1d(GAUSS_ORDER_1D_LOG, gauss_points_1d, gauss_weights_1d);
    
    complex_t integral = complex_zero();
    
    for (int i = 0; i < GAUSS_ORDER_1D_LOG; i++) {
        // Map from [-1, 1] to [0, 1]
        double u = ONE_HALF * (gauss_points_1d[i] + 1.0);
        
        // Interpolate point on edge
        geom_point_t r_prime;
        r_prime.x = edge_start->x + u * (edge_end->x - edge_start->x);
        r_prime.y = edge_start->y + u * (edge_end->y - edge_start->y);
        r_prime.z = edge_start->z + u * (edge_end->z - edge_start->z);
        
        // Compute distance
        double r = geom_point_distance(obs_point, &r_prime) + NUMERICAL_EPSILON;
        
        // Evaluate kernel
        complex_t kernel_val = kernel_func(obs_point, &r_prime, k, data);
        
        // Logarithmic weight: log(r) for logarithmic singularity
        double log_weight = log(r);
        
        // Accumulate integral
        double weight = gauss_weights_1d[i] * edge_len * ONE_HALF;  // Factor of 0.5 from [-1,1] to [0,1] mapping
        integral.re += kernel_val.re * log_weight * weight;
        integral.im += kernel_val.im * log_weight * weight;
    }
    
    return integral;
}

