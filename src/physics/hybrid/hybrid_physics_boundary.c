/********************************************************************************
 * Hybrid Physics Boundary Conditions Implementation (L1 Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements ONLY physical checks and calculations, no numerical solvers.
 ********************************************************************************/

#include "hybrid_physics_boundary.h"
#include <math.h>
#include <string.h>

bool hybrid_physics_check_interface_consistency(
    const hybrid_interface_point_t* point) {
    
    if (!point) return false;
    
    // Physical check: Normal vector should be unit vector
    real_t norm_mag = sqrt(point->normal.x * point->normal.x +
                          point->normal.y * point->normal.y +
                          point->normal.z * point->normal.z);
    
    if (fabs(norm_mag - 1.0) > 1e-6) {
        return false;  // Normal vector not normalized
    }
    
    // Physical check: Normal and tangent should be orthogonal
    real_t dot = point->normal.x * point->tangent.x +
                 point->normal.y * point->tangent.y +
                 point->normal.z * point->tangent.z;
    
    if (fabs(dot) > 1e-6) {
        return false;  // Not orthogonal
    }
    
    return true;
}

real_t hybrid_physics_compute_coupling_strength(
    const hybrid_interface_point_t* point1,
    const hybrid_interface_point_t* point2,
    real_t distance) {
    
    if (!point1 || !point2 || distance <= 0.0) {
        return 0.0;
    }
    
    // Physical calculation: Coupling strength decreases with distance
    // Simple inverse distance model (more sophisticated models in L3)
    real_t base_coupling = 1.0 / (1.0 + distance);
    
    // Adjust based on coupling type
    real_t factor = 1.0;
    switch (point1->coupling_type) {
        case HYBRID_COUPLING_ELECTRIC_FIELD:
            factor = 1.0;  // Standard coupling
            break;
        case HYBRID_COUPLING_MAGNETIC_FIELD:
            factor = 0.8;  // Slightly weaker
            break;
        case HYBRID_COUPLING_CURRENT_DENSITY:
            factor = 1.2;  // Stronger coupling
            break;
        default:
            factor = 1.0;
    }
    
    return base_coupling * factor;
}
