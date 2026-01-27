/********************************************************************************
 * Green's Function Operators Implementation (L3 Operators Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements Green's function operators.
 * L3 layer: Operator / Update Equation - defines HOW to compute operators.
 ********************************************************************************/

#include "greens_function.h"
#include "../../common/constants.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

complex_t greens_function_free_space(
    real_t r,
    real_t k) {
    
    if (r < NUMERICAL_EPSILON) {
        // Handle singularity
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // Operator: G(r) = exp(-jk*r) / (4*π*r)
    real_t kr = k * r;
    real_t phase = -kr;
    
    #if defined(_MSC_VER)
    complex_t result;
    result.re = cos(phase) / (4.0 * M_PI * r);
    result.im = sin(phase) / (4.0 * M_PI * r);
    #else
    complex_t result = cexp(I * phase) / (4.0 * M_PI * r);
    #endif
    
    return result;
}

void greens_function_gradient_free_space(
    real_t r,
    real_t k,
    const real_t* r_vec,
    complex_t* gradient) {
    
    if (!r_vec || !gradient || r < NUMERICAL_EPSILON) {
        if (gradient) {
            #if defined(_MSC_VER)
            gradient[0].re = gradient[0].im = 0.0;
            gradient[1].re = gradient[1].im = 0.0;
            gradient[2].re = gradient[2].im = 0.0;
            #else
            gradient[0] = gradient[1] = gradient[2] = 0.0 + 0.0 * I;
            #endif
        }
        return;
    }
    
    // Operator: ∇G(r) = -jk * (r_vec/r) * G(r)
    complex_t G = greens_function_free_space(r, k);
    
    real_t r_norm = r;
    
    #if defined(_MSC_VER)
    // MSVC: complex_t is a struct, use explicit components
    // factor = -jk = -k * I, where I is imaginary unit
    real_t G_re = G.re;
    real_t G_im = G.im;
    real_t factor_re = 0.0;  // Real part of -jk is 0
    real_t factor_im = -k;   // Imaginary part of -jk is -k
    
    for (int i = 0; i < 3; i++) {
        real_t r_comp = r_vec[i] / r_norm;
        gradient[i].re = (factor_re * G_re - factor_im * G_im) * r_comp;
        gradient[i].im = (factor_re * G_im + factor_im * G_re) * r_comp;
    }
    #else
    complex_t factor_c = -k * I;
    for (int i = 0; i < 3; i++) {
        real_t r_comp = r_vec[i] / r_norm;
        gradient[i] = factor_c * G * r_comp;
    }
    #endif
}

complex_t greens_function_layered_media(
    real_t rho,
    real_t z,
    real_t z_prime,
    real_t k0,
    int n_layers,
    const layered_medium_t* layers) {
    
    if (!layers || n_layers <= 0 || rho < 0.0) {
        #if defined(_MSC_VER)
        complex_t result = {0.0, 0.0};
        #else
        complex_t result = 0.0 + 0.0 * I;
        #endif
        return result;
    }
    
    // L3 layer defines the operator form (Sommerfeld integral representation)
    // Numerical evaluation is in L4 layer
    // This implementation uses effective medium approximation
    
    // Compute effective permittivity and permeability from layers
    complex_t eps_eff = {EPS0, 0.0};
    complex_t mu_eff = {MU0, 0.0};
    
    if (n_layers > 0 && layers) {
        // Compute effective properties based on layer positions
        // For observation and source points, determine which layers they're in
        // Use weighted average of layer properties based on distance
        
        // Determine layers containing observation and source points
        // Layers are assumed to be stacked in z-direction starting from z=0
        // Full implementation would compute proper layer-based Green's function (Sommerfeld integral or DCIM)
        int obs_layer = 0;
        int src_layer = 0;
        
        // Determine observation layer (handle boundary cases)
        real_t z_total = 0.0;
        for (int i = 0; i < n_layers; i++) {
            z_total += layers[i].thickness;
            if (z <= z_total + 1e-10) {  // Small tolerance for numerical precision
                obs_layer = i;
                break;
            }
        }
        // Clamp to valid range
        if (obs_layer >= n_layers) obs_layer = n_layers - 1;
        if (obs_layer < 0) obs_layer = 0;
        
        // Determine source layer (handle boundary cases)
        z_total = 0.0;
        for (int i = 0; i < n_layers; i++) {
            z_total += layers[i].thickness;
            if (z_prime <= z_total + 1e-10) {  // Small tolerance for numerical precision
                src_layer = i;
                break;
            }
        }
        // Clamp to valid range
        if (src_layer >= n_layers) src_layer = n_layers - 1;
        if (src_layer < 0) src_layer = 0;
        
        // Use average of source and observation layer properties
        #if defined(_MSC_VER)
        eps_eff.re = (layers[src_layer].permittivity.re + layers[obs_layer].permittivity.re) / 2.0;
        eps_eff.im = (layers[src_layer].permittivity.im + layers[obs_layer].permittivity.im) / 2.0;
        mu_eff.re = (layers[src_layer].permeability.re + layers[obs_layer].permeability.re) / 2.0;
        mu_eff.im = (layers[src_layer].permeability.im + layers[obs_layer].permeability.im) / 2.0;
        #else
        eps_eff = (layers[src_layer].permittivity + layers[obs_layer].permittivity) / 2.0;
        mu_eff = (layers[src_layer].permeability + layers[obs_layer].permeability) / 2.0;
        #endif
    }
    
    // Compute effective wavenumber: k_eff = omega * sqrt(eps_eff * mu_eff)
    real_t omega = k0 * C0;  // k0 = omega / c0, so omega = k0 * c0
    #if defined(_MSC_VER)
    real_t eps_mag = sqrt(eps_eff.re * eps_eff.re + eps_eff.im * eps_eff.im);
    real_t mu_mag = sqrt(mu_eff.re * mu_eff.re + mu_eff.im * mu_eff.im);
    real_t k_eff = omega * sqrt(eps_mag * mu_mag);
    #else
    complex_t eps_mu = eps_eff * mu_eff;
    real_t k_eff = omega * sqrt(creal(eps_mu));
    #endif
    
    // For layered media, use distance-based approximation with effective wavenumber
    // Full implementation would use Sommerfeld integral or DCIM
    real_t r = sqrt(rho * rho + (z - z_prime) * (z - z_prime));
    return greens_function_free_space(r, k_eff);
}

complex_t greens_function_periodic(
    real_t r,
    real_t k,
    const real_t* periodicity,
    int n_harmonics) {
    
    if (!periodicity || n_harmonics <= 0) {
        return greens_function_free_space(r, k);
    }
    
    // L3 layer defines the operator form (periodic extension using Floquet harmonics)
    // Numerical evaluation is in L4 layer
    // This implementation uses Floquet expansion
    
    if (!periodicity || n_harmonics <= 0) {
        return greens_function_free_space(r, k);
    }
    
    // Periodic Green's function: G_periodic = Σ_n G(r - R_n) * exp(j*k_bloch·R_n)
    // where R_n are lattice vectors, k_bloch is Bloch wavevector
    // For now, use first few harmonics
    complex_t result = greens_function_free_space(r, k);
    
    // Add contributions from periodic images using Floquet expansion
    // This is a simplified implementation for 1D periodicity
    // Full implementation would:
    // 1. Support 2D/3D periodicity (lattice vectors)
    // 2. Include proper Bloch wavevector (k_bloch)
    // 3. Use Ewald summation or spectral domain methods for convergence
    // 4. Handle near-field and far-field regimes differently
    int n_max = (n_harmonics < 3) ? n_harmonics : 3;  // Limit to first few harmonics for efficiency
    
    for (int n = -n_max; n <= n_max; n++) {
        if (n == 0) continue;  // Skip self-term (already included in result)
        
        // Lattice vector: R_n = n * periodicity (1D periodicity)
        // For 2D/3D, would use: R_n = n1*a1 + n2*a2 + n3*a3
        real_t R_n = n * periodicity[0];
        
        // Distance to periodic image
        real_t r_periodic = sqrt(r * r + R_n * R_n);
        
        // Bloch phase: exp(j*k_bloch·R_n)
        // Simplified: assume k_bloch = 0 (normal incidence)
        // Full implementation would include phase factor: exp(I * k_bloch * R_n)
        complex_t G_periodic = greens_function_free_space(r_periodic, k);
        
        #if defined(_MSC_VER)
        result.re += G_periodic.re;
        result.im += G_periodic.im;
        #else
        result += G_periodic;
        #endif
    }
    
    return result;
}
