/********************************************************************************
 * Material Library Implementation
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements a library of common materials.
 ********************************************************************************/

#include "material_library.h"
#include "../common/types.h"
#include "../common/errors.h"
#include "../common/constants.h"
#include "../physics/mom/mom_physics.h"
#include "../physics/peec/peec_physics.h"
#include <string.h>
#include <math.h>

int material_library_get_mom_material(
    material_type_t type,
    mom_material_t* material) {
    
    if (!material) return STATUS_ERROR_INVALID_INPUT;
    
    memset(material, 0, sizeof(mom_material_t));
    
    switch (type) {
        case MATERIAL_FREE_SPACE:
        case MATERIAL_AIR:
            strncpy(material->name, "Air", sizeof(material->name) - 1);
            material->eps_r = 1.0;
            material->mu_r = 1.0;
            material->sigma = 0.0;
            material->tan_delta = 0.0;
            break;
            
        case MATERIAL_FR4:
            strncpy(material->name, "FR4", sizeof(material->name) - 1);
            material->eps_r = 4.4;  // Typical at 1 GHz
            material->mu_r = 1.0;
            material->sigma = 0.0;  // Dielectric
            material->tan_delta = 0.02;  // Typical loss tangent
            break;
            
        case MATERIAL_ROGERS_RO4003:
            strncpy(material->name, "Rogers RO4003", sizeof(material->name) - 1);
            material->eps_r = 3.38;
            material->mu_r = 1.0;
            material->sigma = 0.0;
            material->tan_delta = 0.0027;  // Low loss
            break;
            
        case MATERIAL_ROGERS_RO4350:
            strncpy(material->name, "Rogers RO4350", sizeof(material->name) - 1);
            material->eps_r = 3.48;
            material->mu_r = 1.0;
            material->sigma = 0.0;
            material->tan_delta = 0.0037;
            break;
            
        case MATERIAL_TACONIC_TLY:
            strncpy(material->name, "Taconic TLY", sizeof(material->name) - 1);
            material->eps_r = 2.2;
            material->mu_r = 1.0;
            material->sigma = 0.0;
            material->tan_delta = 0.0009;  // Very low loss
            break;
            
        case MATERIAL_SILICON:
            strncpy(material->name, "Silicon", sizeof(material->name) - 1);
            material->eps_r = 11.7;
            material->mu_r = 1.0;
            material->sigma = 0.0;  // Intrinsic silicon (low conductivity)
            material->tan_delta = 0.0;
            break;
            
        case MATERIAL_GALLIUM_ARSENIDE:
            strncpy(material->name, "Gallium Arsenide", sizeof(material->name) - 1);
            material->eps_r = 12.9;
            material->mu_r = 1.0;
            material->sigma = 0.0;
            material->tan_delta = 0.0;
            break;
            
        case MATERIAL_ALUMINA:
            strncpy(material->name, "Alumina", sizeof(material->name) - 1);
            material->eps_r = 9.8;
            material->mu_r = 1.0;
            material->sigma = 0.0;
            material->tan_delta = 0.0001;  // Very low loss
            break;
            
        case MATERIAL_TEFLON:
            strncpy(material->name, "Teflon (PTFE)", sizeof(material->name) - 1);
            material->eps_r = 2.1;
            material->mu_r = 1.0;
            material->sigma = 0.0;
            material->tan_delta = 0.0002;  // Very low loss
            break;
            
        default:
            return STATUS_ERROR_INVALID_INPUT;
    }
    
    return STATUS_SUCCESS;
}

int material_library_get_peec_material(
    material_type_t type,
    peec_material_t* material) {
    
    if (!material) return STATUS_ERROR_INVALID_INPUT;
    
    memset(material, 0, sizeof(peec_material_t));
    
    // Convert from MoM material to PEEC material
    mom_material_t mom_mat;
    int status = material_library_get_mom_material(type, &mom_mat);
    if (status != STATUS_SUCCESS) return status;
    
    strncpy(material->name, mom_mat.name, sizeof(material->name) - 1);
    material->conductivity = mom_mat.sigma;
    material->relative_permeability = mom_mat.mu_r;
    material->skin_depth = 0.0;  // Will be computed at frequency
    
    return STATUS_SUCCESS;
}

const char* material_library_get_name(material_type_t type) {
    switch (type) {
        case MATERIAL_FREE_SPACE: return "Free Space";
        case MATERIAL_FR4: return "FR4";
        case MATERIAL_ROGERS_RO4003: return "Rogers RO4003";
        case MATERIAL_ROGERS_RO4350: return "Rogers RO4350";
        case MATERIAL_TACONIC_TLY: return "Taconic TLY";
        case MATERIAL_AIR: return "Air";
        case MATERIAL_SILICON: return "Silicon";
        case MATERIAL_GALLIUM_ARSENIDE: return "Gallium Arsenide";
        case MATERIAL_ALUMINA: return "Alumina";
        case MATERIAL_TEFLON: return "Teflon (PTFE)";
        default: return "Unknown";
    }
}

int material_library_get_at_frequency(
    material_type_t type,
    real_t frequency,
    complex_t* eps_complex,
    complex_t* mu_complex,
    real_t* conductivity) {
    
    if (!eps_complex || !mu_complex || !conductivity) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // Get material properties
    mom_material_t material;
    int status = material_library_get_mom_material(type, &material);
    if (status != STATUS_SUCCESS) return status;
    
    // Compute frequency-dependent properties
    mom_frequency_domain_t fd;
    mom_physics_init_frequency_domain(&fd, frequency);
    
    mom_physics_compute_material_properties(&material, &fd, eps_complex, mu_complex);
    *conductivity = material.sigma;
    
    // For frequency-dependent materials, could add dispersion models here
    // (Debye, Lorentz, etc.)
    
    return STATUS_SUCCESS;
}
