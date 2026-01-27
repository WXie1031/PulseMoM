/********************************************************************************
 * MoM Integral Kernels Implementation (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements MoM integral equation kernels.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 ********************************************************************************/

#include "mom_kernel.h"
#include "greens_function.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../../physics/mom/mom_physics.h"
#include "../integration/integration_utils.h"
#include <math.h>
#include <stdlib.h>

mom_kernel_t* mom_kernel_create(
    mom_formulation_t formulation,
    real_t frequency) {
    
    mom_kernel_t* kernel = (mom_kernel_t*)calloc(1, sizeof(mom_kernel_t));
    if (!kernel) return NULL;
    
    kernel->formulation = formulation;
    kernel->frequency = frequency;
    kernel->k0 = 2.0 * M_PI * frequency / C0;  // Free-space wavenumber
    kernel->eta0 = ETA0;  // Free-space impedance
    kernel->medium = NULL;  // No medium by default (free space)
    kernel->n_layers = 0;
    
    return kernel;
}

/**
 * Set material properties for MoM kernel
 */
int mom_kernel_set_material(
    mom_kernel_t* kernel,
    const mom_material_t* material) {
    
    if (!kernel || !material) return STATUS_ERROR_INVALID_INPUT;
    
    // Compute material properties at frequency
    mom_frequency_domain_t fd;
    mom_physics_init_frequency_domain(&fd, kernel->frequency);
    
    complex_t eps_complex, mu_complex;
    mom_physics_compute_material_properties(material, &fd, &eps_complex, &mu_complex);
    
    // Update wavenumber and impedance for material
    // k = omega * sqrt(eps * mu)
    real_t omega = 2.0 * M_PI * kernel->frequency;
    #if defined(_MSC_VER)
    real_t eps_mag = sqrt(eps_complex.re * eps_complex.re + eps_complex.im * eps_complex.im);
    real_t mu_mag = sqrt(mu_complex.re * mu_complex.re + mu_complex.im * mu_complex.im);
    kernel->k0 = omega * sqrt(eps_mag * mu_mag);
    kernel->eta0 = sqrt(mu_mag / eps_mag);
    #else
    complex_t eps_mu = eps_complex * mu_complex;
    kernel->k0 = omega * sqrt(creal(eps_mu));
    kernel->eta0 = sqrt(creal(mu_complex) / creal(eps_complex));
    #endif
    
    return STATUS_SUCCESS;
}

/**
 * Set layered medium for MoM kernel
 */
int mom_kernel_set_layered_medium(
    mom_kernel_t* kernel,
    const layered_medium_t* medium,
    int n_layers) {
    
    if (!kernel) return STATUS_ERROR_INVALID_INPUT;
    
    kernel->medium = (layered_medium_t*)medium;  // Store pointer (caller owns data)
    kernel->n_layers = n_layers;
    
    return STATUS_SUCCESS;
}

void mom_kernel_destroy(mom_kernel_t* kernel) {
    if (!kernel) return;
    free(kernel);
}

complex_t mom_kernel_evaluate_efie(
    const mom_kernel_t* kernel,
    const mom_triangle_element_t* source_tri,
    const mom_triangle_element_t* test_tri,
    const real_t* source_point,
    const real_t* test_point) {
    
    if (!kernel || !source_tri || !test_tri || !source_point || !test_point) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // L3 layer defines the operator form: Z_ij = <f_i, E_operator(f_j)>
    // Compute distance
    real_t dx = test_point[0] - source_point[0];
    real_t dy = test_point[1] - source_point[1];
    real_t dz = test_point[2] - source_point[2];
    real_t r = sqrt(dx * dx + dy * dy + dz * dz);
    
    if (r < NUMERICAL_EPSILON) {
        // Self-term: handle singularity
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // Evaluate Green's function
    // Use layered media Green's function if medium is specified
    complex_t G;
    if (kernel->medium && kernel->n_layers > 0) {
        // Compute horizontal distance and z-coordinates
        real_t rho = sqrt(dx * dx + dy * dy);
        real_t z = test_point[2];
        real_t z_prime = source_point[2];
        
        // Use layered media Green's function
        G = greens_function_layered_media(
            rho, z, z_prime, kernel->k0,
            kernel->n_layers, kernel->medium
        );
    } else {
        // Use free-space Green's function
        G = greens_function_free_space(r, kernel->k0);
    }
    
    // EFIE operator: Z_ij = <f_i, E_operator(f_j)>
    // E_operator = -jωμ * G * J - (1/jωε) * ∇(∇·G*J)
    // For RWG basis functions, this becomes:
    // Z_ij = -jωμ * ∫∫ G(r,r') * f_i(r) · f_j(r') dS dS'
    //      - (1/jωε) * ∫∫ ∇G(r,r') * (∇·f_i(r)) * (∇·f_j(r')) dS dS'
    
    // Compute unit vector from source to test
    real_t r_vec[3] = {dx, dy, dz};
    real_t r_norm = r;
    if (r_norm < NUMERICAL_EPSILON) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    real_t r_hat[3] = {r_vec[0] / r_norm, r_vec[1] / r_norm, r_vec[2] / r_norm};
    
    // Compute Green's function gradient
    complex_t grad_G[3];
    greens_function_gradient_free_space(r, kernel->k0, r_vec, grad_G);
    
    // EFIE impedance matrix element
    // Z_ij = -jωμ * G * <f_i, f_j> - (1/jωε) * <∇·f_i, ∇·f_j> * G
    // For point evaluation, use:
    // Z_ij ≈ -jωμ * G * (f_i · f_j) - (1/jωε) * (∇·f_i) * (∇·f_j) * G
    
    // Compute basis function dot product
    // For RWG basis functions, f_i · f_j depends on triangle overlap
    // RWG basis function: f_n = (l_n / (2*A_n)) * ρ_n
    // where l_n is edge length, A_n is triangle area, ρ_n is position vector
    
    real_t basis_dot = 0.0;
    if (source_tri->area > NUMERICAL_EPSILON && test_tri->area > NUMERICAL_EPSILON) {
        // Check if triangles share an edge (for RWG basis functions)
        // If they share an edge, compute actual overlap
        // Otherwise, use distance-based approximation
        
        // Compute triangle centroids
        real_t source_center[3] = {
            (source_tri->vertices[0][0] + source_tri->vertices[1][0] + source_tri->vertices[2][0]) / 3.0,
            (source_tri->vertices[0][1] + source_tri->vertices[1][1] + source_tri->vertices[2][1]) / 3.0,
            (source_tri->vertices[0][2] + source_tri->vertices[1][2] + source_tri->vertices[2][2]) / 3.0
        };
        real_t test_center[3] = {
            (test_tri->vertices[0][0] + test_tri->vertices[1][0] + test_tri->vertices[2][0]) / 3.0,
            (test_tri->vertices[0][1] + test_tri->vertices[1][1] + test_tri->vertices[2][1]) / 3.0,
            (test_tri->vertices[0][2] + test_tri->vertices[1][2] + test_tri->vertices[2][2]) / 3.0
        };
        
        // Compute distance between centroids
        real_t dx = test_center[0] - source_center[0];
        real_t dy = test_center[1] - source_center[1];
        real_t dz = test_center[2] - source_center[2];
        real_t centroid_dist = sqrt(dx * dx + dy * dy + dz * dz);
        
        // Approximate edge length (from triangle area)
        real_t source_edge = sqrt(source_tri->area) * 2.0;
        real_t test_edge = sqrt(test_tri->area) * 2.0;
        
        // RWG basis function dot product approximation
        // For overlapping triangles: f_i · f_j ≈ (l_i * l_j) / (4 * A_i * A_j) * overlap_factor
        // For non-overlapping: use distance-based decay
        if (centroid_dist < (source_edge + test_edge) * 0.5) {
            // Triangles are close, assume some overlap
            real_t overlap_factor = 1.0 - centroid_dist / ((source_edge + test_edge) * 0.5);
            if (overlap_factor < 0.0) overlap_factor = 0.0;
            basis_dot = (source_edge * test_edge) / (4.0 * source_tri->area * test_tri->area) * overlap_factor;
        } else {
            // Non-overlapping: use distance-based approximation
            real_t decay = 1.0 / (1.0 + centroid_dist * centroid_dist);
            basis_dot = (source_edge * test_edge) / (4.0 * source_tri->area * test_tri->area) * decay;
        }
    }
    
    // Compute divergence terms
    // For RWG basis: ∇·f_n = l_n / A_n+ (on plus triangle) or -l_n / A_n- (on minus triangle)
    // where l_n is edge length, A_n is triangle area
    real_t edge_length_i = sqrt(source_tri->area) * 2.0;  // Approximate edge length
    real_t edge_length_j = sqrt(test_tri->area) * 2.0;     // Approximate edge length
    real_t div_f_i = edge_length_i / source_tri->area;    // Plus triangle divergence
    real_t div_f_j = edge_length_j / test_tri->area;      // Plus triangle divergence
    
    // EFIE operator: -jωμ * G * (f_i · f_j) - (1/jωε) * (∇·f_i) * (∇·f_j) * G
    real_t omega = 2.0 * M_PI * kernel->frequency;
    real_t omega_mu = omega * MU0;
    real_t omega_eps = omega * EPS0;
    
    #if defined(_MSC_VER)
    complex_t term1, term2, result;
    // Term 1: -jωμ * G * (f_i · f_j)
    term1.re = omega_mu * G.im * basis_dot;  // -j * G = (G.im, -G.re)
    term1.im = -omega_mu * G.re * basis_dot;
    
    // Term 2: - (1/jωε) * (∇·f_i) * (∇·f_j) * G = (j/ωε) * (∇·f_i) * (∇·f_j) * G
    real_t div_product = div_f_i * div_f_j;
    term2.re = -G.im * div_product / omega_eps;  // j * G = (-G.im, G.re)
    term2.im = G.re * div_product / omega_eps;
    
    result.re = term1.re + term2.re;
    result.im = term1.im + term2.im;
    #else
    complex_t I_complex = 0.0 + 1.0 * I;
    complex_t term1 = -I_complex * omega_mu * G * basis_dot;
    complex_t term2 = I_complex * (div_f_i * div_f_j) * G / omega_eps;
    complex_t result = term1 + term2;
    #endif
    
    return result;
}

complex_t mom_kernel_evaluate_mfie(
    const mom_kernel_t* kernel,
    const mom_triangle_element_t* source_tri,
    const mom_triangle_element_t* test_tri,
    const real_t* source_point,
    const real_t* test_point) {
    
    if (!kernel || !source_tri || !test_tri || !source_point || !test_point) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // L3 layer defines the operator form: Z_ij = <f_i, H_operator(f_j)>
    // MFIE operator: H = ∇ × (G * J)
    // Similar to EFIE but with curl operator
    
    real_t dx = test_point[0] - source_point[0];
    real_t dy = test_point[1] - source_point[1];
    real_t dz = test_point[2] - source_point[2];
    real_t r = sqrt(dx * dx + dy * dy + dz * dz);
    
    if (r < NUMERICAL_EPSILON) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // Evaluate Green's function
    // Use layered media Green's function if medium is specified
    complex_t G;
    if (kernel->medium && kernel->n_layers > 0) {
        // Compute horizontal distance and z-coordinates
        real_t rho = sqrt(dx * dx + dy * dy);
        real_t z = test_point[2];
        real_t z_prime = source_point[2];
        
        // Use layered media Green's function
        G = greens_function_layered_media(
            rho, z, z_prime, kernel->k0,
            kernel->n_layers, kernel->medium
        );
    } else {
        // Use free-space Green's function
        G = greens_function_free_space(r, kernel->k0);
    }
    
    // Evaluate Green's function gradient
    complex_t grad_G[3];
    real_t r_vec[3] = {dx, dy, dz};
    greens_function_gradient_free_space(r, kernel->k0, r_vec, grad_G);
    
    // MFIE operator: Z_ij = <f_i, H_operator(f_j)>
    // H_operator = ∇ × (G * J) = G * (∇ × J) + (∇G) × J
    // For RWG basis functions:
    // Z_ij = ∫∫ G(r,r') * f_i(r) × f_j(r') dS dS'
    //      + ∫∫ (∇G(r,r')) × f_i(r) · f_j(r') dS dS'
    
    // Compute basis function cross product
    // For RWG basis, f_i × f_j depends on edge vectors and triangle geometry
    // RWG basis function: f_n = (l_n / (2*A_n)) * ρ_n
    // Cross product: f_i × f_j = (l_i * l_j) / (4 * A_i * A_j) * (ρ_i × ρ_j)
    
    real_t basis_cross[3];
    
    // Compute position vectors from triangle centroids
    real_t source_center[3] = {
        (source_tri->vertices[0][0] + source_tri->vertices[1][0] + source_tri->vertices[2][0]) / 3.0,
        (source_tri->vertices[0][1] + source_tri->vertices[1][1] + source_tri->vertices[2][1]) / 3.0,
        (source_tri->vertices[0][2] + source_tri->vertices[1][2] + source_tri->vertices[2][2]) / 3.0
    };
    real_t test_center[3] = {
        (test_tri->vertices[0][0] + test_tri->vertices[1][0] + test_tri->vertices[2][0]) / 3.0,
        (test_tri->vertices[0][1] + test_tri->vertices[1][1] + test_tri->vertices[2][1]) / 3.0,
        (test_tri->vertices[0][2] + test_tri->vertices[1][2] + test_tri->vertices[2][2]) / 3.0
    };
    
    // Approximate edge vectors (from triangle geometry)
    // For RWG, edge vector points along the common edge
    real_t edge_vec_i[3] = {
        source_tri->vertices[1][0] - source_tri->vertices[0][0],
        source_tri->vertices[1][1] - source_tri->vertices[0][1],
        source_tri->vertices[1][2] - source_tri->vertices[0][2]
    };
    real_t edge_vec_j[3] = {
        test_tri->vertices[1][0] - test_tri->vertices[0][0],
        test_tri->vertices[1][1] - test_tri->vertices[0][1],
        test_tri->vertices[1][2] - test_tri->vertices[0][2]
    };
    
    // Normalize edge vectors
    real_t edge_i_mag = sqrt(edge_vec_i[0] * edge_vec_i[0] + 
                             edge_vec_i[1] * edge_vec_i[1] + 
                             edge_vec_i[2] * edge_vec_i[2]);
    real_t edge_j_mag = sqrt(edge_vec_j[0] * edge_vec_j[0] + 
                             edge_vec_j[1] * edge_vec_j[1] + 
                             edge_vec_j[2] * edge_vec_j[2]);
    
    if (edge_i_mag > NUMERICAL_EPSILON) {
        edge_vec_i[0] /= edge_i_mag;
        edge_vec_i[1] /= edge_i_mag;
        edge_vec_i[2] /= edge_i_mag;
    }
    if (edge_j_mag > NUMERICAL_EPSILON) {
        edge_vec_j[0] /= edge_j_mag;
        edge_vec_j[1] /= edge_j_mag;
        edge_vec_j[2] /= edge_j_mag;
    }
    
    // Cross product: edge_vec_i × edge_vec_j
    basis_cross[0] = edge_vec_i[1] * edge_vec_j[2] - edge_vec_i[2] * edge_vec_j[1];
    basis_cross[1] = edge_vec_i[2] * edge_vec_j[0] - edge_vec_i[0] * edge_vec_j[2];
    basis_cross[2] = edge_vec_i[0] * edge_vec_j[1] - edge_vec_i[1] * edge_vec_j[0];
    
    // Normalize
    real_t cross_mag = sqrt(basis_cross[0] * basis_cross[0] +
                           basis_cross[1] * basis_cross[1] +
                           basis_cross[2] * basis_cross[2]);
    if (cross_mag > NUMERICAL_EPSILON) {
        basis_cross[0] /= cross_mag;
        basis_cross[1] /= cross_mag;
        basis_cross[2] /= cross_mag;
    } else {
        // Fallback: use triangle normal
        basis_cross[0] = test_tri->normal[0];
        basis_cross[1] = test_tri->normal[1];
        basis_cross[2] = test_tri->normal[2];
    }
    
    // MFIE operator: G * (f_i × f_j) + (∇G) × f_i · f_j
    #if defined(_MSC_VER)
    complex_t term1, term2, result;
    
    // Term 1: G * (f_i × f_j) (scalar product with normal)
    real_t cross_dot_normal = basis_cross[0] * source_tri->normal[0] +
                              basis_cross[1] * source_tri->normal[1] +
                              basis_cross[2] * source_tri->normal[2];
    term1.re = G.re * cross_dot_normal;
    term1.im = G.im * cross_dot_normal;
    
    // Term 2: (∇G) × f_i · f_j
    // Cross product of grad_G with basis function
    // (∇G) × f_i = (∇G_y * f_i_z - ∇G_z * f_i_y, ...)
    // f_i is the RWG basis function vector (already computed above)
    real_t f_i_vec[3] = {edge_vec_i[0], edge_vec_i[1], edge_vec_i[2]};
    real_t grad_cross[3] = {
        grad_G[1].re * f_i_vec[2] - grad_G[2].re * f_i_vec[1],
        grad_G[2].re * f_i_vec[0] - grad_G[0].re * f_i_vec[2],
        grad_G[0].re * f_i_vec[1] - grad_G[1].re * f_i_vec[0]
    };
    // Dot product with f_j
    real_t f_j_vec[3] = {edge_vec_j[0], edge_vec_j[1], edge_vec_j[2]};
    real_t grad_cross_dot = grad_cross[0] * f_j_vec[0] +
                            grad_cross[1] * f_j_vec[1] +
                            grad_cross[2] * f_j_vec[2];
    term2.re = grad_cross_dot;
    // Include imaginary part from grad_G
    real_t grad_cross_im[3] = {
        grad_G[1].im * f_i_vec[2] - grad_G[2].im * f_i_vec[1],
        grad_G[2].im * f_i_vec[0] - grad_G[0].im * f_i_vec[2],
        grad_G[0].im * f_i_vec[1] - grad_G[1].im * f_i_vec[0]
    };
    term2.im = grad_cross_im[0] * f_j_vec[0] +
               grad_cross_im[1] * f_j_vec[1] +
               grad_cross_im[2] * f_j_vec[2];
    
    result.re = term1.re + term2.re;
    result.im = term1.im + term2.im;
    #else
    // Term 1: G * (f_i × f_j)
    real_t cross_dot_normal = basis_cross[0] * source_tri->normal[0] +
                              basis_cross[1] * source_tri->normal[1] +
                              basis_cross[2] * source_tri->normal[2];
    complex_t term1 = G * cross_dot_normal;
    
    // Term 2: (∇G) × f_i · f_j
    complex_t grad_cross[3] = {
        grad_G[1] * source_tri->normal[2] - grad_G[2] * source_tri->normal[1],
        grad_G[2] * source_tri->normal[0] - grad_G[0] * source_tri->normal[2],
        grad_G[0] * source_tri->normal[1] - grad_G[1] * source_tri->normal[0]
    };
    complex_t term2 = grad_cross[0] * test_tri->normal[0] +
                      grad_cross[1] * test_tri->normal[1] +
                      grad_cross[2] * test_tri->normal[2];
    
    complex_t result = term1 + term2;
    #endif
    
    return result;
}

complex_t mom_kernel_evaluate_cfie(
    const mom_kernel_t* kernel,
    const mom_triangle_element_t* source_tri,
    const mom_triangle_element_t* test_tri,
    const real_t* source_point,
    const real_t* test_point,
    real_t alpha) {
    
    if (!kernel || !source_tri || !test_tri || !source_point || !test_point) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // L3 layer defines the operator form: Z_ij = α * EFIE + (1-α) * MFIE
    complex_t efie = mom_kernel_evaluate_efie(kernel, source_tri, test_tri, source_point, test_point);
    complex_t mfie = mom_kernel_evaluate_mfie(kernel, source_tri, test_tri, source_point, test_point);
    
    #if defined(_MSC_VER)
    complex_t result;
    result.re = alpha * efie.re + (1.0 - alpha) * mfie.re;
    result.im = alpha * efie.im + (1.0 - alpha) * mfie.im;
    #else
    complex_t result = alpha * efie + (1.0 - alpha) * mfie;
    #endif
    
    return result;
}
