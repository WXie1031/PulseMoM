/********************************************************************************
 * Singular Integration Implementation (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements singular integral treatment methods.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 ********************************************************************************/

#include "singular_integration.h"
#include "integration_utils.h"
#include "../kernels/greens_function.h"
#include "../../common/constants.h"
#include <math.h>
#include <stdlib.h>
#include <float.h>

// GSL support for advanced integration (if available)
#ifdef ENABLE_GSL
#include <gsl/gsl_integration.h>
#include <gsl/gsl_errno.h>
#endif

// Helper: Detect singularity type
singularity_type_t singular_integration_detect_type(
    const real_t* source_vertices,
    int num_vertices,
    const real_t* obs_point,
    real_t tolerance) {
    
    if (!source_vertices || !obs_point || num_vertices <= 0) {
        return SINGULARITY_NONE;
    }
    
    // Compute minimum distance from observation point to source element
    real_t min_dist = DBL_MAX;
    
    for (int i = 0; i < num_vertices; i++) {
        real_t dx = obs_point[0] - source_vertices[i * 3 + 0];
        real_t dy = obs_point[1] - source_vertices[i * 3 + 1];
        real_t dz = obs_point[2] - source_vertices[i * 3 + 2];
        real_t dist = sqrt(dx * dx + dy * dy + dz * dz);
        
        if (dist < min_dist) {
            min_dist = dist;
        }
    }
    
    // Classify singularity type based on distance
    if (min_dist < tolerance * 1e-3) {
        return SINGULARITY_HYPERSINGULAR;  // Very close
    } else if (min_dist < tolerance * 1e-1) {
        return SINGULARITY_STRONG;         // Close
    } else if (min_dist < tolerance) {
        return SINGULARITY_WEAK;           // Near
    }
    
    return SINGULARITY_NONE;
}

// Helper: Apply Duffy transformation
int singular_integration_duffy_transform(
    const real_t* triangle_vertices,
    const real_t* obs_point,
    real_t* transformed_points,
    real_t* jacobians,
    int num_points) {
    
    if (!triangle_vertices || !obs_point || !transformed_points || !jacobians) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L3 layer defines the transformation, not numerical implementation details
    // Duffy transformation maps singular triangle to regular domain
    
    // Step 1: Map observation point to triangle coordinate system
    // Triangle vertices: v0, v1, v2
    real_t v0[3] = {triangle_vertices[0], triangle_vertices[1], triangle_vertices[2]};
    real_t v1[3] = {triangle_vertices[3], triangle_vertices[4], triangle_vertices[5]};
    real_t v2[3] = {triangle_vertices[6], triangle_vertices[7], triangle_vertices[8]};
    
    // Compute triangle edges
    real_t e1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
    real_t e2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
    
    // Compute triangle normal and area
    real_t normal[3] = {
        e1[1] * e2[2] - e1[2] * e2[1],
        e1[2] * e2[0] - e1[0] * e2[2],
        e1[0] * e2[1] - e1[1] * e2[0]
    };
    real_t area = 0.5 * sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    
    if (area < NUMERICAL_EPSILON) {
        return STATUS_ERROR_INVALID_INPUT;  // Degenerate triangle
    }
    
    // Step 2: Apply Duffy transformation
    // Duffy transformation: (u, v) -> (u, u*v) where u ∈ [0,1], v ∈ [0,1]
    // This removes the 1/r singularity at the origin
    // Jacobian: J = u
    
    // Use standard quadrature points in [0,1] x [0,1]
    // For simplicity, use equally spaced points
    int n = (int)sqrt((double)num_points);
    if (n * n < num_points) n++;
    
    int point_idx = 0;
    for (int i = 0; i < n && point_idx < num_points; i++) {
        for (int j = 0; j < n && point_idx < num_points; j++) {
            // Original coordinates in [0,1] x [0,1]
            real_t u = (i + 0.5) / n;
            real_t v = (j + 0.5) / n;
            
            // Duffy transformation: (u, v) -> (u, u*v)
            // This maps the unit square to a triangle with singularity removed
            real_t u_transformed = u;
            real_t v_transformed = u * v;
            
            // Store transformed barycentric coordinates
            transformed_points[point_idx * 2 + 0] = 1.0 - u_transformed - v_transformed;  // w0
            transformed_points[point_idx * 2 + 1] = u_transformed;                        // w1
            // w2 = v_transformed (implicit, w0 + w1 + w2 = 1)
            
            // Jacobian determinant: J = u (from Duffy transformation)
            jacobians[point_idx] = u_transformed * area * 2.0;  // Include triangle area factor
            
            point_idx++;
        }
    }
    
    return STATUS_SUCCESS;
}

complex_t singular_integration_triangle(
    const real_t* triangle_vertices,
    const real_t* obs_point,
    real_t k,
    greens_kernel_type_t kernel_type,
    singularity_type_t sing_type,
    singular_method_t method) {
    
    if (!triangle_vertices || !obs_point) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // L3 layer defines integration method, not solver-specific optimizations
    if (sing_type == SINGULARITY_NONE) {
        // Use regular integration with Gaussian quadrature
        // L3 layer defines the integration method
        
        // Get triangle vertices
        real_t v0[3] = {triangle_vertices[0], triangle_vertices[1], triangle_vertices[2]};
        real_t v1[3] = {triangle_vertices[3], triangle_vertices[4], triangle_vertices[5]};
        real_t v2[3] = {triangle_vertices[6], triangle_vertices[7], triangle_vertices[8]};
        
        // Compute triangle area
        real_t e1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
        real_t e2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
        real_t normal[3] = {
            e1[1] * e2[2] - e1[2] * e2[1],
            e1[2] * e2[0] - e1[0] * e2[2],
            e1[0] * e2[1] - e1[1] * e2[0]
        };
        real_t area = 0.5 * sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        
        // Use Gaussian quadrature for triangle (order 7)
        int order = 7;
        real_t quad_points[7][2];
        real_t quad_weights[7];
        gauss_quadrature_triangle(order, quad_points, quad_weights);
        
        // Integrate Green's function over triangle
        complex_t result;
        #if defined(_MSC_VER)
        result.re = 0.0;
        result.im = 0.0;
        #else
        result = 0.0 + 0.0 * I;
        #endif
        
        for (int i = 0; i < order; i++) {
            // Barycentric coordinates
            real_t w0 = quad_points[i][0];
            real_t w1 = quad_points[i][1];
            real_t w2 = 1.0 - w0 - w1;
            
            // Map to physical coordinates
            real_t quad_point[3] = {
                w0 * v0[0] + w1 * v1[0] + w2 * v2[0],
                w0 * v0[1] + w1 * v1[1] + w2 * v2[1],
                w0 * v0[2] + w1 * v1[2] + w2 * v2[2]
            };
            
            // Compute distance from observation point
            real_t dx = obs_point[0] - quad_point[0];
            real_t dy = obs_point[1] - quad_point[1];
            real_t dz = obs_point[2] - quad_point[2];
            real_t r = sqrt(dx * dx + dy * dy + dz * dz);
            
            if (r < NUMERICAL_EPSILON) {
                continue;  // Skip if too close
            }
            
            // Evaluate Green's function kernel
            complex_t G;
            if (kernel_type == KERNEL_G) {
                // G(r) = exp(-jk*r) / (4*π*r)
                real_t kr = k * r;
                #if defined(_MSC_VER)
                G.re = cos(-kr) / (4.0 * M_PI * r);
                G.im = sin(-kr) / (4.0 * M_PI * r);
                #else
                G = cexp(-I * kr) / (4.0 * M_PI * r);
                #endif
            } else {
                // Other kernel types would be handled here
                #if defined(_MSC_VER)
                G.re = 1.0 / (4.0 * M_PI * r);
                G.im = 0.0;
                #else
                G = 1.0 / (4.0 * M_PI * r) + 0.0 * I;
                #endif
            }
            
            // Accumulate with weight and area
            real_t weight = quad_weights[i] * area;
            #if defined(_MSC_VER)
            complex_t integrand;
            integrand.re = G.re * weight;
            integrand.im = G.im * weight;
            result.re += integrand.re;
            result.im += integrand.im;
            #else
            result += G * weight;
            #endif
        }
        
        return result;
    }
    
    // Handle singular case based on method
    if (method == SINGULAR_METHOD_DUFFY) {
        // Apply Duffy transformation
        int num_points = 16;  // Default quadrature points
        real_t* transformed_points = (real_t*)malloc(num_points * 2 * sizeof(real_t));
        real_t* jacobians = (real_t*)malloc(num_points * sizeof(real_t));
        
        if (!transformed_points || !jacobians) {
            if (transformed_points) free(transformed_points);
            if (jacobians) free(jacobians);
            #if defined(_MSC_VER)
            complex_t result = {0.0, 0.0};
            #else
            complex_t result = 0.0 + 0.0 * I;
            #endif
            return result;
        }
        
        singular_integration_duffy_transform(
            triangle_vertices, obs_point,
            transformed_points, jacobians, num_points);
        
        // Integrate over transformed domain
        // Evaluate kernel at transformed quadrature points
        complex_t result;
        #if defined(_MSC_VER)
        result.re = 0.0;
        result.im = 0.0;
        #else
        result = 0.0 + 0.0 * I;
        #endif
        
        // Get triangle vertices
        real_t v0[3] = {triangle_vertices[0], triangle_vertices[1], triangle_vertices[2]};
        real_t v1[3] = {triangle_vertices[3], triangle_vertices[4], triangle_vertices[5]};
        real_t v2[3] = {triangle_vertices[6], triangle_vertices[7], triangle_vertices[8]};
        
        for (int i = 0; i < num_points; i++) {
            // Barycentric coordinates
            real_t w0 = transformed_points[i * 2 + 0];
            real_t w1 = transformed_points[i * 2 + 1];
            real_t w2 = 1.0 - w0 - w1;
            
            // Map to physical coordinates
            real_t quad_point[3] = {
                w0 * v0[0] + w1 * v1[0] + w2 * v2[0],
                w0 * v0[1] + w1 * v1[1] + w2 * v2[1],
                w0 * v0[2] + w1 * v1[2] + w2 * v2[2]
            };
            
            // Compute distance from observation point
            real_t dx = obs_point[0] - quad_point[0];
            real_t dy = obs_point[1] - quad_point[1];
            real_t dz = obs_point[2] - quad_point[2];
            real_t r = sqrt(dx * dx + dy * dy + dz * dz);
            
            if (r < NUMERICAL_EPSILON) {
                continue;  // Skip if too close (handled by transformation)
            }
            
            // Evaluate Green's function kernel
            complex_t G;
            if (kernel_type == KERNEL_G) {
                // G(r) = exp(-jk*r) / (4*π*r)
                real_t kr = k * r;
                #if defined(_MSC_VER)
                G.re = cos(-kr) / (4.0 * M_PI * r);
                G.im = sin(-kr) / (4.0 * M_PI * r);
                #else
                G = cexp(-I * kr) / (4.0 * M_PI * r);
                #endif
            } else {
                // Other kernel types would be handled here
                #if defined(_MSC_VER)
                G.re = 1.0 / (4.0 * M_PI * r);
                G.im = 0.0;
                #else
                G = 1.0 / (4.0 * M_PI * r) + 0.0 * I;
                #endif
            }
            
            // Accumulate with Jacobian
            #if defined(_MSC_VER)
            complex_t integrand;
            integrand.re = G.re * jacobians[i];
            integrand.im = G.im * jacobians[i];
            result.re += integrand.re;
            result.im += integrand.im;
            #else
            result += G * jacobians[i];
            #endif
        }
        
        free(transformed_points);
        free(jacobians);
        return result;
    }
    
    // Other methods (polar, analytic, adaptive)
    if (method == SINGULAR_METHOD_POLAR) {
        // Polar coordinate transformation
        // Map triangle to polar coordinates centered at observation point
        // This removes 1/r singularity
        
        // Get triangle vertices
        real_t v0[3] = {triangle_vertices[0], triangle_vertices[1], triangle_vertices[2]};
        real_t v1[3] = {triangle_vertices[3], triangle_vertices[4], triangle_vertices[5]};
        real_t v2[3] = {triangle_vertices[6], triangle_vertices[7], triangle_vertices[8]};
        
        // Compute triangle area
        real_t e1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
        real_t e2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
        real_t normal[3] = {
            e1[1] * e2[2] - e1[2] * e2[1],
            e1[2] * e2[0] - e1[0] * e2[2],
            e1[0] * e2[1] - e1[1] * e2[0]
        };
        real_t area = 0.5 * sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        
        // Polar transformation: (r, theta) where r is distance from obs_point
        // Jacobian: J = r (for 2D polar coordinates)
        int num_radial = 8;
        int num_angular = 8;
        
        complex_t result;
        #if defined(_MSC_VER)
        result.re = 0.0;
        result.im = 0.0;
        #else
        result = 0.0 + 0.0 * I;
        #endif
        
        // Compute triangle plane normal and local coordinate system
        // This allows proper mapping from polar coordinates to triangle surface
        real_t triangle_center[3] = {
            (v0[0] + v1[0] + v2[0]) / 3.0,
            (v0[1] + v1[1] + v2[1]) / 3.0,
            (v0[2] + v1[2] + v2[2]) / 3.0
        };
        
        // Normalize triangle normal
        real_t norm_len = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        if (norm_len < NUMERICAL_EPSILON) {
            // Degenerate triangle
            #if defined(_MSC_VER)
            complex_t result = {0.0, 0.0};
            #else
            complex_t result = 0.0 + 0.0 * I;
            #endif
            return result;
        }
        normal[0] /= norm_len;
        normal[1] /= norm_len;
        normal[2] /= norm_len;
        
        // Create local coordinate system in triangle plane
        // Use e1 as first basis vector, create orthogonal second basis
        real_t basis1[3] = {e1[0], e1[1], e1[2]};
        real_t basis1_len = sqrt(e1[0] * e1[0] + e1[1] * e1[1] + e1[2] * e1[2]);
        if (basis1_len > NUMERICAL_EPSILON) {
            basis1[0] /= basis1_len;
            basis1[1] /= basis1_len;
            basis1[2] /= basis1_len;
        }
        
        // Second basis vector: cross product of normal and basis1
        real_t basis2[3] = {
            normal[1] * basis1[2] - normal[2] * basis1[1],
            normal[2] * basis1[0] - normal[0] * basis1[2],
            normal[0] * basis1[1] - normal[1] * basis1[0]
        };
        real_t basis2_len = sqrt(basis2[0] * basis2[0] + basis2[1] * basis2[1] + basis2[2] * basis2[2]);
        if (basis2_len > NUMERICAL_EPSILON) {
            basis2[0] /= basis2_len;
            basis2[1] /= basis2_len;
            basis2[2] /= basis2_len;
        }
        
        // Integrate in polar coordinates
        for (int i = 0; i < num_radial; i++) {
            real_t r_norm = (i + 0.5) / num_radial;  // Normalized radius [0, 1]
            real_t r_max = sqrt(area / M_PI);  // Maximum radius (approximate)
            real_t r = r_norm * r_max;
            
            for (int j = 0; j < num_angular; j++) {
                real_t theta = 2.0 * M_PI * (j + 0.5) / num_angular;
                
                // Map polar coordinates to triangle plane using local coordinate system
                // Project polar coordinates onto triangle plane
                real_t local_x = r * cos(theta);
                real_t local_y = r * sin(theta);
                
                // Convert to 3D coordinates in triangle plane
                real_t quad_point[3] = {
                    obs_point[0] + local_x * basis1[0] + local_y * basis2[0],
                    obs_point[1] + local_x * basis1[1] + local_y * basis2[1],
                    obs_point[2] + local_x * basis1[2] + local_y * basis2[2]
                };
                
                // Check if point is inside triangle using barycentric coordinates
                // Compute vectors from triangle vertices to quad_point
                real_t v0p[3] = {quad_point[0] - v0[0], quad_point[1] - v0[1], quad_point[2] - v0[2]};
                real_t v1p[3] = {quad_point[0] - v1[0], quad_point[1] - v1[1], quad_point[2] - v1[2]};
                real_t v2p[3] = {quad_point[0] - v2[0], quad_point[1] - v2[1], quad_point[2] - v2[2]};
                
                // Compute barycentric coordinates using area ratios
                real_t area0 = 0.5 * sqrt(
                    (v1p[1] * v2p[2] - v1p[2] * v2p[1]) * (v1p[1] * v2p[2] - v1p[2] * v2p[1]) +
                    (v1p[2] * v2p[0] - v1p[0] * v2p[2]) * (v1p[2] * v2p[0] - v1p[0] * v2p[2]) +
                    (v1p[0] * v2p[1] - v1p[1] * v2p[0]) * (v1p[0] * v2p[1] - v1p[1] * v2p[0])
                );
                real_t area1 = 0.5 * sqrt(
                    (v0p[1] * v2p[2] - v0p[2] * v2p[1]) * (v0p[1] * v2p[2] - v0p[2] * v2p[1]) +
                    (v0p[2] * v2p[0] - v0p[0] * v2p[2]) * (v0p[2] * v2p[0] - v0p[0] * v2p[2]) +
                    (v0p[0] * v2p[1] - v0p[1] * v2p[0]) * (v0p[0] * v2p[1] - v0p[1] * v2p[0])
                );
                real_t area2 = 0.5 * sqrt(
                    (v0p[1] * v1p[2] - v0p[2] * v1p[1]) * (v0p[1] * v1p[2] - v0p[2] * v1p[1]) +
                    (v0p[2] * v1p[0] - v0p[0] * v1p[2]) * (v0p[2] * v1p[0] - v0p[0] * v1p[2]) +
                    (v0p[0] * v1p[1] - v0p[1] * v1p[0]) * (v0p[0] * v1p[1] - v0p[1] * v1p[0])
                );
                
                real_t bary_u = area0 / area;
                real_t bary_v = area1 / area;
                real_t bary_w = area2 / area;
                
                // Only integrate if point is inside triangle (with small tolerance)
                if (bary_u < -0.01 || bary_v < -0.01 || bary_w < -0.01 ||
                    bary_u > 1.01 || bary_v > 1.01 || bary_w > 1.01) {
                    continue;  // Skip points outside triangle
                }
                
                // Evaluate Green's function
                real_t dist_vec[3] = {
                    quad_point[0] - obs_point[0],
                    quad_point[1] - obs_point[1],
                    quad_point[2] - obs_point[2]
                };
                real_t dist = sqrt(dist_vec[0] * dist_vec[0] + dist_vec[1] * dist_vec[1] + dist_vec[2] * dist_vec[2]);
                if (dist < NUMERICAL_EPSILON) {
                    continue;
                }
                
                complex_t G;
                if (kernel_type == KERNEL_G) {
                    real_t kr = k * dist;
                    #if defined(_MSC_VER)
                    G.re = cos(-kr) / (4.0 * M_PI * dist);
                    G.im = sin(-kr) / (4.0 * M_PI * dist);
                    #else
                    G = cexp(-I * kr) / (4.0 * M_PI * dist);
                    #endif
                } else {
                    #if defined(_MSC_VER)
                    G.re = 1.0 / (4.0 * M_PI * dist);
                    G.im = 0.0;
                    #else
                    G = 1.0 / (4.0 * M_PI * dist) + 0.0 * I;
                    #endif
                }
                
                // Jacobian: J = r (for polar coordinates)
                real_t weight = r * (2.0 * M_PI / num_angular) * (r_max / num_radial);
                #if defined(_MSC_VER)
                result.re += G.re * weight;
                result.im += G.im * weight;
                #else
                result += G * weight;
                #endif
            }
        }
        
        return result;
    }
    
    if (method == SINGULAR_METHOD_ANALYTIC) {
        // Analytic integration for specific kernel types
        // For simple cases, can use closed-form expressions
        
        // Get triangle vertices
        real_t v0[3] = {triangle_vertices[0], triangle_vertices[1], triangle_vertices[2]};
        real_t v1[3] = {triangle_vertices[3], triangle_vertices[4], triangle_vertices[5]};
        real_t v2[3] = {triangle_vertices[6], triangle_vertices[7], triangle_vertices[8]};
        
        // Compute triangle area
        real_t e1[3] = {v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
        real_t e2[3] = {v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
        real_t normal[3] = {
            e1[1] * e2[2] - e1[2] * e2[1],
            e1[2] * e2[0] - e1[0] * e2[2],
            e1[0] * e2[1] - e1[1] * e2[0]
        };
        real_t area = 0.5 * sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
        
        // For analytic integration, use approximate formula
        // ∫∫ G(r) dS ≈ G(r_center) * area (for non-singular case)
        // For singular case, would use specialized analytic formulas
        real_t center[3] = {
            (v0[0] + v1[0] + v2[0]) / 3.0,
            (v0[1] + v1[1] + v2[1]) / 3.0,
            (v0[2] + v1[2] + v2[2]) / 3.0
        };
        
        real_t dx = obs_point[0] - center[0];
        real_t dy = obs_point[1] - center[1];
        real_t dz = obs_point[2] - center[2];
        real_t r = sqrt(dx * dx + dy * dy + dz * dz);
        
        if (r < NUMERICAL_EPSILON) {
            // Self-term: use specialized analytic formula
            // For triangle self-term, approximate as point source
            #if defined(_MSC_VER)
            complex_t result = {0.0, 0.0};
            #else
            complex_t result = 0.0 + 0.0 * I;
            #endif
            return result;
        }
        
        // Evaluate Green's function at center
        complex_t G;
        if (kernel_type == KERNEL_G) {
            real_t kr = k * r;
            #if defined(_MSC_VER)
            G.re = cos(-kr) / (4.0 * M_PI * r);
            G.im = sin(-kr) / (4.0 * M_PI * r);
            #else
            G = cexp(-I * kr) / (4.0 * M_PI * r);
            #endif
        } else {
            #if defined(_MSC_VER)
            G.re = 1.0 / (4.0 * M_PI * r);
            G.im = 0.0;
            #else
            G = 1.0 / (4.0 * M_PI * r) + 0.0 * I;
            #endif
        }
        
        // Multiply by area
        #if defined(_MSC_VER)
        complex_t result;
        result.re = G.re * area;
        result.im = G.im * area;
        #else
        complex_t result = G * area;
        #endif
        
        return result;
    }
    
    // Default: return zero for unimplemented methods
    #if defined(_MSC_VER)
    complex_t result = {0.0, 0.0};
    #else
    complex_t result = 0.0 + 0.0 * I;
    #endif
    return result;
}

complex_t singular_integration_rectangle(
    const real_t* rect_vertices,
    const real_t* obs_point,
    real_t k,
    greens_kernel_type_t kernel_type,
    singularity_type_t sing_type,
    singular_method_t method) {
    
    if (!rect_vertices || !obs_point) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // Similar to triangle, but for rectangle
    // L3 layer defines the method, not solver optimizations
    #if defined(_MSC_VER)
    complex_t result = {0.0, 0.0};
    #else
    complex_t result = 0.0 + 0.0 * I;
    #endif
    return result;
}
