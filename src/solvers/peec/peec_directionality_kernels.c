/**
 * @file peec_directionality_kernels.c
 * @brief PEEC directionality kernels for current direction-dependent coupling
 * @details Implements kernels that consider current flow direction in PEEC analysis
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "peec_solver.h"
#include "../core/core_geometry.h"
#include "../core/core_common.h"
#include "../core/core_kernels.h"
#include <math.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Physical constants
#define MU0 (4.0 * M_PI * 1e-7)
#define EPS0 (8.854187817e-12)

/*********************************************************************
 * Directional Partial Inductance
 *********************************************************************/

/**
 * @brief Compute directional partial inductance between two current segments
 * @param seg1 First current segment (with direction)
 * @param seg2 Second current segment (with direction)
 * @param direction1 Current direction vector for segment 1
 * @param direction2 Current direction vector for segment 2
 * @return Complex partial inductance value
 * 
 * Formula: L_p = (μ₀/4π) ∫∫ (dl₁ · dl₂) / |r₁ - r₂| dS₁ dS₂
 * where dl₁ and dl₂ are in the direction of current flow
 */
complex_t peec_directional_partial_inductance(const geom_line_t* seg1,
                                              const geom_line_t* seg2,
                                              const geom_point_t* direction1,
                                              const geom_point_t* direction2) {
    if (!seg1 || !seg2 || !direction1 || !direction2) {
        complex_t zero = complex_zero();
        return zero;
    }
    
    // Compute segment vectors
    geom_point_t dl1 = {
        seg1->end.x - seg1->start.x,
        seg1->end.y - seg1->start.y,
        seg1->end.z - seg1->start.z
    };
    
    geom_point_t dl2 = {
        seg2->end.x - seg2->start.x,
        seg2->end.y - seg2->start.y,
        seg2->end.z - seg2->start.z
    };
    
    // Normalize direction vectors
    double len1 = sqrt(direction1->x*direction1->x + direction1->y*direction1->y + direction1->z*direction1->z);
    double len2 = sqrt(direction2->x*direction2->x + direction2->y*direction2->y + direction2->z*direction2->z);
    
    if (len1 < 1e-12 || len2 < 1e-12) {
        complex_t zero = complex_zero();
        return zero;
    }
    
    geom_point_t dir1_norm = {
        direction1->x / len1,
        direction1->y / len1,
        direction1->z / len1
    };
    
    geom_point_t dir2_norm = {
        direction2->x / len2,
        direction2->y / len2,
        direction2->z / len2
    };
    
    // Compute dot product of direction vectors
    double dir_dot = dir1_norm.x * dir2_norm.x + 
                     dir1_norm.y * dir2_norm.y + 
                     dir1_norm.z * dir2_norm.z;
    
    // Compute segment lengths
    double l1 = sqrt(dl1.x*dl1.x + dl1.y*dl1.y + dl1.z*dl1.z);
    double l2 = sqrt(dl2.x*dl2.x + dl2.y*dl2.y + dl2.z*dl2.z);
    
    // Compute distance between segment centers
    geom_point_t c1 = {
        (seg1->start.x + seg1->end.x) / 2.0,
        (seg1->start.y + seg1->end.y) / 2.0,
        (seg1->start.z + seg1->end.z) / 2.0
    };
    
    geom_point_t c2 = {
        (seg2->start.x + seg2->end.x) / 2.0,
        (seg2->start.y + seg2->end.y) / 2.0,
        (seg2->start.z + seg2->end.z) / 2.0
    };
    
    double dx = c1.x - c2.x;
    double dy = c1.y - c2.y;
    double dz = c1.z - c2.z;
    double R = sqrt(dx*dx + dy*dy + dz*dz) + 1e-12;
    
    // Directional partial inductance: L_p = (μ₀/4π) * (dl₁ · dl₂) / R
    // The dot product accounts for current direction
    double L_real = (MU0 / (4.0 * M_PI)) * l1 * l2 * dir_dot / R;
    
    complex_t result = {L_real, 0.0};
    
    return result;
}

/*********************************************************************
 * Directional Coupling Coefficient
 *********************************************************************/

/**
 * @brief Compute directional coupling coefficient between two conductors
 * @param cond1 First conductor (with current direction)
 * @param cond2 Second conductor (with current direction)
 * @param direction1 Current direction for conductor 1
 * @param direction2 Current direction for conductor 2
 * @return Coupling coefficient (0 = no coupling, 1 = maximum coupling)
 * 
 * This accounts for the geometric orientation and current flow direction
 */
double peec_directional_coupling_coefficient(const void* cond1,
                                              const void* cond2,
                                              const geom_point_t* direction1,
                                              const geom_point_t* direction2) {
    if (!cond1 || !cond2 || !direction1 || !direction2) {
        return 0.0;
    }
    
    // Normalize direction vectors
    double len1 = sqrt(direction1->x*direction1->x + direction1->y*direction1->y + direction1->z*direction1->z);
    double len2 = sqrt(direction2->x*direction2->x + direction2->y*direction2->y + direction2->z*direction2->z);
    
    if (len1 < 1e-12 || len2 < 1e-12) {
        return 0.0;
    }
    
    geom_point_t dir1_norm = {
        direction1->x / len1,
        direction1->y / len1,
        direction1->z / len1
    };
    
    geom_point_t dir2_norm = {
        direction2->x / len2,
        direction2->y / len2,
        direction2->z / len2
    };
    
    // Coupling coefficient is the absolute value of dot product
    double dot = dir1_norm.x * dir2_norm.x + 
                 dir1_norm.y * dir2_norm.y + 
                 dir1_norm.z * dir2_norm.z;
    
    return fabs(dot);
}

/*********************************************************************
 * High-Frequency Directionality Effects
 *********************************************************************/

/**
 * @brief Compute frequency-dependent directional coupling
 * @param seg1 First segment
 * @param seg2 Second segment
 * @param direction1 Current direction for segment 1
 * @param direction2 Current direction for segment 2
 * @param frequency Operating frequency
 * @return Complex coupling value (includes phase effects)
 * 
 * At high frequencies, current direction affects phase relationships
 */
complex_t peec_directional_coupling_frequency(const geom_line_t* seg1,
                                             const geom_line_t* seg2,
                                             const geom_point_t* direction1,
                                             const geom_point_t* direction2,
                                             double frequency) {
    if (!seg1 || !seg2 || !direction1 || !direction2 || frequency <= 0.0) {
        complex_t zero = complex_zero();
        return zero;
    }
    
    // Compute base directional inductance
    complex_t L_base = peec_directional_partial_inductance(seg1, seg2, direction1, direction2);
    
    // At high frequencies, add phase effects
    double omega = 2.0 * M_PI * frequency;
    
    // Compute propagation delay between segments
    geom_point_t c1 = {
        (seg1->start.x + seg1->end.x) / 2.0,
        (seg1->start.y + seg1->end.y) / 2.0,
        (seg1->start.z + seg1->end.z) / 2.0
    };
    
    geom_point_t c2 = {
        (seg2->start.x + seg2->end.x) / 2.0,
        (seg2->start.y + seg2->end.y) / 2.0,
        (seg2->start.z + seg2->end.z) / 2.0
    };
    
    double dx = c1.x - c2.x;
    double dy = c1.y - c2.y;
    double dz = c1.z - c2.z;
    double R = sqrt(dx*dx + dy*dy + dz*dz);
    
    double c0 = 299792458.0;  // Speed of light
    double delay = R / c0;
    double phase = omega * delay;
    
    // Apply phase shift: L(ω) = L₀ * exp(-jωR/c)
    double cos_phase = cos(phase);
    double sin_phase = sin(phase);
    
    complex_t result = {
        L_base.re * cos_phase + L_base.im * sin_phase,
        -L_base.re * sin_phase + L_base.im * cos_phase
    };
    
    return result;
}
