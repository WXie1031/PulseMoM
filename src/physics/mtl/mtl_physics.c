/********************************************************************************
 * MTL Physics Implementation (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements ONLY physical calculations, no numerical solvers.
 ********************************************************************************/

#include "mtl_physics.h"
#include <math.h>
#include <string.h>

void mtl_physics_init_frequency_domain(mtl_frequency_domain_t* fd, real_t frequency) {
    if (!fd) return;
    
    fd->frequency = frequency;
    fd->omega = 2.0 * M_PI * frequency;
    fd->k0 = fd->omega / C0;  // Free space wavenumber
}

void mtl_physics_compute_characteristic_impedance(
    const mtl_physical_model_t* model,
    complex_t* z0_matrix) {
    
    if (!model || !z0_matrix || !model->series_impedance || !model->shunt_admittance) {
        return;
    }
    
    int n = model->num_conductors;
    
    // Physical formula: Z0 = sqrt(Z / Y)
    // This is a physical calculation, not a numerical solver
    // For now, simplified calculation (full matrix sqrt would be in L3/L4)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int idx = i * n + j;
            // Simplified: Z0_ij = sqrt(Z_ij / Y_ij)
            // Full matrix calculation would be done in L3/L4 layers
            #if defined(_MSC_VER)
            real_t z_re = model->series_impedance[idx].re;
            real_t z_im = model->series_impedance[idx].im;
            real_t y_re = model->shunt_admittance[idx].re;
            real_t y_im = model->shunt_admittance[idx].im;
            
            // Complex division: Z/Y
            real_t denom = y_re * y_re + y_im * y_im;
            if (denom < 1e-20) {
                z0_matrix[idx].re = 0.0;
                z0_matrix[idx].im = 0.0;
            } else {
                real_t div_re = (z_re * y_re + z_im * y_im) / denom;
                real_t div_im = (z_im * y_re - z_re * y_im) / denom;
                
                // Square root of complex number (simplified)
                real_t mag = sqrt(div_re * div_re + div_im * div_im);
                real_t phase = atan2(div_im, div_re) / 2.0;
                z0_matrix[idx].re = sqrt(mag) * cos(phase);
                z0_matrix[idx].im = sqrt(mag) * sin(phase);
            }
            #else
            complex_t z = model->series_impedance[idx];
            complex_t y = model->shunt_admittance[idx];
            z0_matrix[idx] = csqrt(z / y);
            #endif
        }
    }
}

void mtl_physics_compute_propagation_constant(
    const mtl_physical_model_t* model,
    complex_t* gamma_matrix) {
    
    if (!model || !gamma_matrix || !model->series_impedance || !model->shunt_admittance) {
        return;
    }
    
    int n = model->num_conductors;
    
    // Physical formula: gamma = sqrt(Z * Y)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int idx = i * n + j;
            // Simplified calculation (full matrix sqrt would be in L3/L4)
            #if defined(_MSC_VER)
            real_t z_re = model->series_impedance[idx].re;
            real_t z_im = model->series_impedance[idx].im;
            real_t y_re = model->shunt_admittance[idx].re;
            real_t y_im = model->shunt_admittance[idx].im;
            
            // Complex multiplication: Z * Y
            real_t prod_re = z_re * y_re - z_im * y_im;
            real_t prod_im = z_re * y_im + z_im * y_re;
            
            // Square root of complex number
            real_t mag = sqrt(prod_re * prod_re + prod_im * prod_im);
            real_t phase = atan2(prod_im, prod_re) / 2.0;
            gamma_matrix[idx].re = sqrt(mag) * cos(phase);
            gamma_matrix[idx].im = sqrt(mag) * sin(phase);
            #else
            complex_t z = model->series_impedance[idx];
            complex_t y = model->shunt_admittance[idx];
            gamma_matrix[idx] = csqrt(z * y);
            #endif
        }
    }
}
