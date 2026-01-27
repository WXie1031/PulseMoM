/**
 * @file kernel_mfie.c
 * @brief Magnetic Field Integral Equation (MFIE) kernel implementation
 * @details Implements MFIE kernels for closed conductors and cavity problems
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "core_kernels.h"
#include "../../discretization/geometry/core_geometry.h"
#include "core_common.h"
#include "../integration/integration_utils.h"
#include "../integration/integration_utils_optimized.h"
#include <math.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Precomputed constants
// Note: M_PI, MU0, and INV_4PI are defined in core_common.h

// Thresholds for singularity detection
#define SINGULARITY_THRESHOLD_SQ 1e-18  // Square of distance threshold for singularity
#define NEAR_SINGULAR_RATIO 0.01        // Ratio of distance to triangle size for near-singular detection

/*********************************************************************
 * MFIE Kernel: Neumann Boundary Term (∂G/∂n')
 *********************************************************************/

/**
 * @brief Compute Neumann boundary term: n' · ∇'G(r, r')
 * @param r Observation point
 * @param r_prime Source point
 * @param n_prime Normal vector at source point
 * @param k Wavenumber
 * @return Complex kernel value
 * 
 * Formula: n' · ∇'G = n' · (r - r') * (1 + jkR) * exp(-jkR) / (4πR³)
 */
complex_t kernel_mfie_neumann_term(const geom_point_t* r,
                                   const geom_point_t* r_prime,
                                   const geom_point_t* n_prime,
                                   double k) {
    if (!r || !r_prime || !n_prime) {
        return complex_zero();
    }
    
    // Compute R = |r - r'|
    double dx = r->x - r_prime->x;
    double dy = r->y - r_prime->y;
    double dz = r->z - r_prime->z;
    double R_sq = dx*dx + dy*dy + dz*dz;
    
    // Avoid singularity
    if (R_sq < SINGULARITY_THRESHOLD_SQ) {
        return complex_zero();
    }
    
    double R = sqrt(R_sq);
    double R_cubed = R_sq * R;
    
    // Compute n' · (r - r')
    double n_dot_R = n_prime->x * dx + n_prime->y * dy + n_prime->z * dz;
    
    // Compute exp(-jkR)
    double kR = k * R;
    double cos_kR = cos(kR);
    double sin_kR = sin(kR);
    
    // (1 + jkR) * exp(-jkR) = (1 + jkR) * (cos(kR) - j*sin(kR))
    // = cos(kR) + kR*sin(kR) + j*(kR*cos(kR) - sin(kR))
    double real_part = cos_kR + kR * sin_kR;
    double imag_part = kR * cos_kR - sin_kR;
    
    // Final result: n' · (r - r') * (1 + jkR) * exp(-jkR) / (4πR³)
    double scale = n_dot_R * INV_4PI / R_cubed;
    
    complex_t result = {
        scale * real_part,
        scale * imag_part
    };
    
    return result;
}

/*********************************************************************
 * MFIE Kernel: Double Gradient Term (∇∇G)
 *********************************************************************/

/**
 * @brief Compute double gradient kernel: ∇∇G(r, r')
 * @param r Observation point
 * @param r_prime Source point
 * @param k Wavenumber
 * @param result Output 3x3 tensor (stored as 9-element array: [xx, xy, xz, yx, yy, yz, zx, zy, zz])
 * 
 * Formula: ∇∇G = [3(r-r')(r-r')^T / R^5 - I/R^3] * (1 + jkR - k²R²) * exp(-jkR) / (4π)
 */
void kernel_mfie_double_gradient(const geom_point_t* r,
                                 const geom_point_t* r_prime,
                                 double k,
                                 complex_t* result) {
    if (!r || !r_prime || !result) {
        return;
    }
    
    // Compute R = |r - r'|
    double dx = r->x - r_prime->x;
    double dy = r->y - r_prime->y;
    double dz = r->z - r_prime->z;
    double R_sq = dx*dx + dy*dy + dz*dz;
    
    // Avoid singularity
    if (R_sq < SINGULARITY_THRESHOLD_SQ) {
        memset(result, 0, 9 * sizeof(complex_t));
        return;
    }
    
    double R = sqrt(R_sq);
    double R3 = R_sq * R;
    double R5 = R_sq * R_sq * R;
    
    // Compute exp(-jkR)
    double kR = k * R;
    double kR_sq = kR * kR;
    double cos_kR = cos(kR);
    double sin_kR = sin(kR);
    
    // (1 + jkR - k²R²) * exp(-jkR)
    // = (1 - k²R²) * cos(kR) + kR * sin(kR) + j*(kR * cos(kR) - (1 - k²R²) * sin(kR))
    double coeff_real = (1.0 - kR_sq) * cos_kR + kR * sin_kR;
    double coeff_imag = kR * cos_kR - (1.0 - kR_sq) * sin_kR;
    
    double scale = INV_4PI;
    
    // Compute tensor components
    // T_ij = [3 * (r_i - r'_i) * (r_j - r'_j) / R^5 - δ_ij / R^3] * coeff
    double dx_dx = dx * dx;
    double dx_dy = dx * dy;
    double dx_dz = dx * dz;
    double dy_dy = dy * dy;
    double dy_dz = dy * dz;
    double dz_dz = dz * dz;
    
    // xx component
    result[0].re = scale * (3.0 * dx_dx / R5 - 1.0 / R3) * coeff_real;
    result[0].im = scale * (3.0 * dx_dx / R5 - 1.0 / R3) * coeff_imag;
    
    // xy component
    result[1].re = scale * (3.0 * dx_dy / R5) * coeff_real;
    result[1].im = scale * (3.0 * dx_dy / R5) * coeff_imag;
    
    // xz component
    result[2].re = scale * (3.0 * dx_dz / R5) * coeff_real;
    result[2].im = scale * (3.0 * dx_dz / R5) * coeff_imag;
    
    // yx component (symmetric)
    result[3] = result[1];
    
    // yy component
    result[4].re = scale * (3.0 * dy_dy / R5 - 1.0 / R3) * coeff_real;
    result[4].im = scale * (3.0 * dy_dy / R5 - 1.0 / R3) * coeff_imag;
    
    // yz component
    result[5].re = scale * (3.0 * dy_dz / R5) * coeff_real;
    result[5].im = scale * (3.0 * dy_dz / R5) * coeff_imag;
    
    // zx component (symmetric)
    result[6] = result[2];
    
    // zy component (symmetric)
    result[7] = result[5];
    
    // zz component
    result[8].re = scale * (3.0 * dz_dz / R5 - 1.0 / R3) * coeff_real;
    result[8].im = scale * (3.0 * dz_dz / R5 - 1.0 / R3) * coeff_imag;
}

/*********************************************************************
 * MFIE Surface Integral
 *********************************************************************/

/**
 * @brief Compute MFIE surface integral over triangle
 * @param tri Source triangle
 * @param obs_point Observation point
 * @param n_prime Normal vector at source point
 * @param k Wavenumber
 * @return Complex integral value
 * 
 * MFIE: H(r) = J(r)/2 + ∫∫ n' × [∇'G × J(r')] dS'
 * For closed surfaces, the 1/2 term comes from the principal value
 */
complex_t kernel_mfie_triangle_integral(const geom_triangle_t* tri,
                                       const geom_point_t* obs_point,
                                       const geom_point_t* n_prime,
                                       double k) {
    if (!tri || !obs_point || !n_prime) {
        return complex_zero();
    }
    
    // Use numerical integration (Gauss quadrature)
    // For singular cases, use Duffy transform (similar to EFIE)
    
    // Compute triangle centroid
    geom_point_t centroid;
    geom_triangle_get_centroid(tri, &centroid);
    
    // Check if observation point is on or very close to triangle
    double dist = geom_point_distance(obs_point, &centroid);
    double dist_sq = dist * dist;
    double tri_size_sq = tri->area * 4.0 / M_PI;  // Approximate characteristic size
    
    complex_t integral = complex_zero();
    
    if (dist_sq < tri_size_sq * NEAR_SINGULAR_RATIO) {
        // Near-singular or singular case: use Duffy transform for MFIE
        // Duffy transform removes the 1/R singularity in the MFIE kernel
        // Similar to EFIE, but for Neumann boundary term n' · ∇'G
        
        // Validate triangle area
        if (tri->area <= 0.0) {
            return integral;
        }
        
        // 4-point Gauss quadrature on unit square (2x2 grid) for Duffy transform
        // Points: (±1/√3, ±1/√3) in [-1,1]^2, mapped to [0,1]^2
        const double gauss_pts[2] = {-0.577350269189626, 0.577350269189626};  // ±1/√3
        const double gauss_wts[2] = {1.0, 1.0};
        
        // Duffy transform: map (u,v) in [0,1]^2 to triangle using barycentric coordinates
        // The transformation v -> v*(1-u) removes the 1/R singularity
        for (int p = 0; p < 2; p++) {
            for (int q = 0; q < 2; q++) {
                // Map Gauss points from [-1,1] to [0,1]
                double u = ONE_HALF * (gauss_pts[p] + 1.0);
                double v = ONE_HALF * (gauss_pts[q] + 1.0);
                
                // Barycentric coordinates with Duffy transform: (ξ, η, 1-ξ-η)
                // The factor (1-u) in eta removes the singularity
                double xi = u;
                double eta = v * (1.0 - u);  // Duffy transform: removes 1/R singularity
                double zeta = 1.0 - xi - eta;
                
                // Ensure barycentric coordinates are valid
                if (xi < 0.0) xi = 0.0;
                if (eta < 0.0) eta = 0.0;
                if (zeta < 0.0) {
                    double sum = xi + eta;
                    if (sum > 1.0 + NUMERICAL_EPSILON) {
                        double inv_sum = 1.0 / sum;
                        xi *= inv_sum;
                        eta *= inv_sum;
                    }
                    zeta = 1.0 - xi - eta;
                }
                if (zeta < 0.0) zeta = 0.0;
                
                // Map to physical coordinates on triangle
                geom_point_t r_prime;
                geom_triangle_interpolate_point(tri, xi, eta, zeta, &r_prime);
                
                // Compute MFIE Neumann term: n' · ∇'G
                complex_t kernel_val = kernel_mfie_neumann_term(obs_point, &r_prime, n_prime, k);
                
                // Duffy transform Jacobian: (1-u) factor from coordinate transformation
                // The weight includes both Gauss weight and Duffy transform Jacobian
                double jacobian = (1.0 - u) * tri->area;
                double weight = gauss_wts[p] * gauss_wts[q] * jacobian;
                
                integral.re += kernel_val.re * weight;
                integral.im += kernel_val.im * weight;
            }
        }
        
        return integral;
    } else {
        // Regular case: use standard Gauss quadrature
        // 4-point Gauss quadrature for triangles
        // Validate triangle area
        if (tri->area <= 0.0) {
            return integral;
        }
        
        double gauss_points[4][2];
        double gauss_weights[4];
        // Try to use cached lookup table first for performance
        if (!integration_get_cached_triangle_quadrature(4, gauss_points, gauss_weights)) {
            // Fallback to regular function if not cached
            gauss_quadrature_triangle(4, gauss_points, gauss_weights);
        }
        
        for (int i = 0; i < 4; i++) {
            double u = gauss_points[i][0];
            double v = gauss_points[i][1];
            double w = 1.0 - u - v;
            
            // Validate barycentric coordinates
            if (fabs(u + v + w - 1.0) > 1e-10) {
                continue;  // Skip invalid quadrature point
            }
            
            // Interpolate point on triangle using barycentric coordinates
            geom_point_t r_prime;
            geom_triangle_interpolate_point(tri, u, v, w, &r_prime);
            
            // Compute Neumann term
            complex_t kernel_val = kernel_mfie_neumann_term(obs_point, &r_prime, n_prime, k);
            
            // Accumulate integral
            double weight = gauss_weights[i] * tri->area;
            integral.re += kernel_val.re * weight;
            integral.im += kernel_val.im * weight;
        }
    }
    
    return integral;
}

/*********************************************************************
 * MFIE Self-Term (Principal Value)
 *********************************************************************/

/**
 * @brief Compute MFIE self-term for closed surface
 * @param tri Triangle element
 * @return Complex self-term value
 * 
 * For closed surfaces, MFIE has a 1/2 term from the principal value
 * This is the "jump term" in the boundary integral equation
 */
complex_t kernel_mfie_self_term(const geom_triangle_t* tri) {
    if (!tri) {
        return complex_zero();
    }
    
    // For closed surfaces, the self-term is 1/2
    // This comes from the principal value of the surface integral
    complex_t result;
    result.re = MFIE_SELF_TERM_VALUE;
    result.im = 0.0;
    
    return result;
}

/*********************************************************************
 * MFIE Duffy Transform for Triangle-Triangle Integration
 * Handles 1/R³ singularity (stronger than EFIE's 1/R)
 *********************************************************************/

/**
 * @brief MFIE Duffy transform for triangle-triangle integration with 1/R³ singularity
 * @param tri_i First triangle (observation triangle)
 * @param tri_j Second triangle (source triangle)
 * @param frequency Operating frequency
 * @param threshold Distance threshold for Duffy transform
 * @return Complex integral value
 * 
 * This function implements a specialized Duffy transform for MFIE kernels
 * that have 1/R³ singularity (stronger than EFIE's 1/R singularity).
 * The transformation uses a modified Jacobian to handle the stronger singularity.
 * 
 * For MFIE, the kernel is n' · ∇'G = n' · (r - r') * (1 + jkR) * exp(-jkR) / (4πR³)
 * The 1/R³ singularity requires a stronger transformation than EFIE's 1/R.
 */
complex_t kernel_mfie_duffy_transform(const geom_triangle_t* tri_i,
                                     const geom_triangle_t* tri_j,
                                     double frequency,
                                     double threshold) {
    if (!tri_i || !tri_j) {
        return complex_zero();
    }
    
    // Use geometry helper for centroid distance
    double r = geom_triangle_compute_centroid_distance(tri_i, tri_j);
    
    const double k = TWO_PI_OVER_C0 * frequency;
    const double lambda = C0 / frequency;
    double r_threshold = threshold * lambda;
    
    // If distance is above threshold, use standard integration
    if (r > r_threshold) {
        // For far-field, use standard MFIE integration
        // Note: This would require calling integrate_triangle_triangle with MFIE formulation
        // For now, return zero as fallback (should be handled by caller)
        return complex_zero();
    }
    
    // For self-term (same triangle), return zero here - handled separately
    if (r < NUMERICAL_EPSILON) {
        return complex_zero();  // Self-term handled separately
    }
    
    // Compute triangle areas
    double area_i = geom_triangle_get_area(tri_i);
    double area_j = geom_triangle_get_area(tri_j);
    
    // Numerical stability check
    if (area_i < AREA_EPSILON || area_j < AREA_EPSILON) {
        return complex_zero();
    }
    
    // Get normal vectors (assume stored in triangle structure)
    // For MFIE, we need the normal vector at the source point
    geom_point_t n_j = tri_j->normal;
    
    // Use higher-order Gauss quadrature for MFIE (1/R³ singularity is stronger)
    // 8-point quadrature for better accuracy with stronger singularity
    double gauss_points[8][2];
    double gauss_weights[8];
    
    // Try to use cached lookup table first for performance
    if (!integration_get_cached_triangle_quadrature(8, gauss_points, gauss_weights)) {
        // Fallback to regular function if not cached
        gauss_quadrature_triangle(8, gauss_points, gauss_weights);
    }
    
    complex_t integral = complex_zero();
    
    // MFIE Duffy transform: map (u,v) in [0,1]^2 to triangle pairs
    // For 1/R³ singularity, we need a stronger transformation
    // Use double Duffy transform: v -> v*(1-u) and w -> w*(1-u)*(1-v)
    // This removes both 1/R and 1/R² terms, leaving only smooth integrand
    
    // For triangle-triangle integration, we need to integrate over both triangles
    // Outer loop: triangle i (observation)
    for (int i = 0; i < 8; i++) {
        double u_i = gauss_points[i][0];
        double v_i = gauss_points[i][1];
        double w_i = 1.0 - u_i - v_i;
        
        // Validate barycentric coordinates for triangle i
        if (u_i < 0.0) u_i = 0.0;
        if (v_i < 0.0) v_i = 0.0;
        if (w_i < 0.0) {
            double sum = u_i + v_i;
            if (sum > NUMERICAL_EPSILON) {
                double inv_sum = 1.0 / sum;
                u_i *= inv_sum;
                v_i *= inv_sum;
            } else {
                u_i = ONE_THIRD;
                v_i = ONE_THIRD;
            }
            w_i = 1.0 - u_i - v_i;
        }
        if (w_i < 0.0) w_i = 0.0;
        
        // Map to physical coordinates on triangle i
        geom_point_t pt_i;
        geom_triangle_interpolate_point(tri_i, u_i, v_i, w_i, &pt_i);
        
        // Inner loop: triangle j (source) with Duffy transform
        for (int j = 0; j < 8; j++) {
            // Map Gauss points to [0,1] for Duffy transform
            // Original Gauss points are in barycentric coordinates
            double u_j = gauss_points[j][0];
            double v_j = gauss_points[j][1];
            
            // Ensure u_j, v_j are in [0,1]
            if (u_j < 0.0) u_j = 0.0;
            if (u_j > 1.0) u_j = 1.0;
            if (v_j < 0.0) v_j = 0.0;
            if (v_j > 1.0) v_j = 1.0;
            
            // Duffy transform for 1/R³ singularity
            // Transformation: v -> v*(1-u) removes 1/R, additional factor removes 1/R²
            // For 1/R³, we need: eta = v*(1-u) and additional scaling
            double xi_j = u_j;
            double eta_j = v_j * (1.0 - u_j);  // First Duffy transform
            double zeta_j = 1.0 - xi_j - eta_j;
            
            // Additional transformation for stronger singularity
            // Scale by (1-u) again to handle 1/R³: eta -> eta*(1-u)
            // This gives us: eta = v*(1-u)^2
            eta_j = v_j * (1.0 - u_j) * (1.0 - u_j);  // Double Duffy transform for 1/R³
            zeta_j = 1.0 - xi_j - eta_j;
            
            // Ensure barycentric coordinates are valid
            if (xi_j < 0.0) xi_j = 0.0;
            if (eta_j < 0.0) eta_j = 0.0;
            if (zeta_j < 0.0) {
                double sum_j = xi_j + eta_j;
                if (sum_j > NUMERICAL_EPSILON) {
                    double inv_sum_j = 1.0 / sum_j;
                    xi_j *= inv_sum_j;
                    eta_j *= inv_sum_j;
                } else {
                    xi_j = ONE_THIRD;
                    eta_j = ONE_THIRD;
                }
                zeta_j = 1.0 - xi_j - eta_j;
            }
            if (zeta_j < 0.0) zeta_j = 0.0;
            
            // Map to physical coordinates on triangle j
            geom_point_t pt_j;
            geom_triangle_interpolate_point(tri_j, xi_j, eta_j, zeta_j, &pt_j);
            
            // Compute distance
            double dr = geom_point_distance(&pt_i, &pt_j) + NUMERICAL_EPSILON;
            
            // Compute MFIE Neumann term: n' · ∇'G
            complex_t kernel_val = kernel_mfie_neumann_term(&pt_i, &pt_j, &n_j, k);
            
            // Duffy transform Jacobian for 1/R³ singularity
            // For double Duffy transform: J = (1-u_j)^2 * (1-u_j) = (1-u_j)^3
            // Combined with triangle areas: 2*area_i * 2*area_j * (1-u_j)^3
            double jacobian = 2.0 * area_i * 2.0 * area_j * (1.0 - u_j) * (1.0 - u_j) * (1.0 - u_j);
            
            // Numerical stability: ensure jacobian is positive
            if (jacobian < 0.0) jacobian = 0.0;
            
            // Accumulate contribution
            double weight = gauss_weights[i] * gauss_weights[j] * jacobian;
            integral.re += kernel_val.re * weight;
            integral.im += kernel_val.im * weight;
        }
    }
    
    // Final numerical stability check
    if ((integral.re != integral.re) || (integral.im != integral.im) ||  // NaN check
        (integral.re > 1e308) || (integral.re < -1e308) ||              // Inf check
        (integral.im > 1e308) || (integral.im < -1e308)) {
        return complex_zero();  // Fallback to zero if transform fails
    }
    
    return integral;
}
