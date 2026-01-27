/********************************************************************************
 * MoM Physics Implementation (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements ONLY physical calculations, no numerical solvers.
 ********************************************************************************/

#include "mom_physics.h"
#include <math.h>

void mom_physics_init_frequency_domain(mom_frequency_domain_t* fd, real_t frequency) {
    if (!fd) return;
    
    fd->frequency = frequency;
    fd->omega = 2.0 * M_PI * frequency;
    fd->k0 = fd->omega / C0;  // Free space wavenumber
    fd->eta0 = ETA0;          // Free space impedance
}

void mom_physics_compute_material_properties(
    const mom_material_t* material,
    const mom_frequency_domain_t* fd,
    complex_t* eps_complex,
    complex_t* mu_complex) {
    
    if (!material || !fd || !eps_complex || !mu_complex) return;
    
    // Physical calculation: complex permittivity
    // eps = eps0 * eps_r * (1 - j*tan_delta)
    real_t eps_r_real = material->eps_r;
    real_t eps_r_imag = -material->eps_r * material->tan_delta;
    
    #if defined(_MSC_VER)
    eps_complex->re = EPS0 * eps_r_real;
    eps_complex->im = EPS0 * eps_r_imag;
    #else
    *eps_complex = EPS0 * (eps_r_real + I * eps_r_imag);
    #endif
    
    // Physical calculation: complex permeability
    // mu = mu0 * mu_r (assuming no magnetic losses for now)
    #if defined(_MSC_VER)
    mu_complex->re = MU0 * material->mu_r;
    mu_complex->im = 0.0;
    #else
    *mu_complex = MU0 * material->mu_r;
    #endif
}
