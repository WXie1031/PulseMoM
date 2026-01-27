/********************************************************************************
 * PEEC Physics Implementation (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements ONLY physical calculations, no numerical solvers.
 ********************************************************************************/

#include "peec_physics.h"
#include <math.h>

void peec_physics_init_frequency_domain(peec_frequency_domain_t* fd, real_t frequency) {
    if (!fd) return;
    
    fd->frequency = frequency;
    fd->omega = 2.0 * M_PI * frequency;
    fd->k0 = fd->omega / C0;  // Free space wavenumber
}

real_t peec_physics_compute_skin_depth(
    real_t frequency,
    real_t conductivity,
    real_t permeability) {
    
    if (frequency <= 0.0 || conductivity <= 0.0) return 1e-3; // Default 1mm
    
    real_t omega = 2.0 * M_PI * frequency;
    real_t mu = MU0 * permeability;
    
    // Physical formula: delta = sqrt(2 / (omega * mu * sigma))
    return sqrt(2.0 / (omega * mu * conductivity));
}

void peec_physics_compute_material_properties(
    const peec_material_t* material,
    const peec_frequency_domain_t* fd,
    peec_material_t* material_at_freq) {
    
    if (!material || !fd || !material_at_freq) return;
    
    // Copy base properties
    *material_at_freq = *material;
    
    // Compute frequency-dependent skin depth
    material_at_freq->skin_depth = peec_physics_compute_skin_depth(
        fd->frequency,
        material->conductivity,
        material->relative_permeability
    );
}
