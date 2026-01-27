/*****************************************************************************************
 * PEEC Via Modeling - Enhanced Via Parasitic Parameter Calculation
 * 
 * Copyright (C) 2025 PulseEM Technologies
 * 
 * File: peec_via_modeling.c
 * Description: Implementation of precise via modeling for PCB applications
 *****************************************************************************************/

#include "peec_via_modeling.h"
#include "peec_solver.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../common/core_common.h"
#include "../../operators/integration/integration_utils.h"
#include "../../operators/kernels/electromagnetic_kernels.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Physical constants
#ifndef MU0
#define MU0 (4.0 * M_PI * 1e-7)  // Permeability of free space (H/m)
#endif
#ifndef EPS0
#define EPS0 (8.854187817e-12)    // Permittivity of free space (F/m)
#endif
#ifndef C0
#define C0 299792458.0            // Speed of light (m/s)
#endif

// Default via material (copper)
via_material_t peec_get_default_via_material(void) {
    via_material_t material;
    material.conductivity = 5.8e7;      // Copper conductivity (S/m)
    material.resistivity = 1.7e-8;       // Copper resistivity (Ohm·m)
    material.permeability = MU0;         // Non-magnetic
    material.permittivity = EPS0;        // Free space permittivity
    strncpy(material.name, "Copper", sizeof(material.name) - 1);
    return material;
}

/********************************************************************************
 * Via Parasitic Parameter Calculation
 ********************************************************************************/

/**
 * @brief Compute skin depth
 */
double peec_compute_skin_depth(double frequency, double conductivity, double permeability) {
    if (frequency <= 0.0 || conductivity <= 0.0) {
        return 1e-3;  // Default 1mm for DC
    }
    
    // Skin depth: δ = √(2 / (ω * μ * σ))
    // where ω = 2πf, μ = permeability, σ = conductivity
    double omega = TWO_PI * frequency;
    double denominator = omega * permeability * conductivity;
    if (denominator <= 0.0) {
        return 1e-3;
    }
    
    return sqrt(2.0 / denominator);
}

/**
 * @brief Compute via DC resistance
 * R = ρ * L / A, where ρ is resistivity, L is length, A is cross-sectional area
 */
double peec_compute_via_resistance_dc(const peec_via_t* via) {
    if (!via || via->radius <= 0.0 || via->height <= 0.0) {
        return 0.0;
    }
    
    double area = M_PI * via->radius * via->radius;
    double resistivity = via->material.resistivity;
    if (resistivity <= 0.0) {
        resistivity = via->material.conductivity > 0.0 
                     ? 1.0 / via->material.conductivity 
                     : 1.7e-8;  // Default copper resistivity
    }
    
    return resistivity * via->height / area;
}

/**
 * @brief Compute effective resistance with skin effect
 */
double peec_compute_via_resistance_skin_effect(
    const peec_via_t* via,
    double frequency
) {
    if (!via || frequency <= 0.0) {
        return peec_compute_via_resistance_dc(via);
    }
    
    double dc_resistance = peec_compute_via_resistance_dc(via);
    double conductivity = via->material.conductivity;
    if (conductivity <= 0.0) {
        conductivity = 5.8e7;  // Default copper conductivity
    }
    double permeability = via->material.permeability;
    if (permeability <= 0.0) {
        permeability = MU0;
    }
    
    double skin_depth = peec_compute_skin_depth(frequency, conductivity, permeability);
    double radius = via->radius;
    
    // For cylindrical conductor with skin effect:
    // R_eff = R_dc * (1 + radius / (2 * skin_depth)) for thin skin
    // More accurate: use Bessel functions for exact solution
    if (skin_depth >= radius) {
        // Skin depth larger than radius, use DC resistance
        return dc_resistance;
    }
    
    // Effective radius reduction due to skin effect
    double effective_radius = radius - skin_depth;
    if (effective_radius <= 0.0) {
        effective_radius = skin_depth;  // Minimum effective radius
    }
    
    // Effective area with skin effect
    double effective_area = M_PI * effective_radius * effective_radius;
    double resistivity = via->material.resistivity;
    if (resistivity <= 0.0) {
        resistivity = 1.0 / conductivity;
    }
    
    return resistivity * via->height / effective_area;
}

/**
 * @brief Compute via self inductance
 * Uses analytical formula for cylindrical conductor:
 * L = (μ₀ * h / (2π)) * [ln(2h/r) - 0.75] for h >> r
 */
double peec_compute_via_inductance(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup
) {
    if (!via || via->radius <= 0.0 || via->height <= 0.0) {
        return 0.0;
    }
    
    double h = via->height;
    double r = via->radius;
    
    // Analytical formula for self inductance of cylindrical conductor
    // L = (μ₀ * h / (2π)) * [ln(2h/r) - 0.75] for h >> r
    // For h ~ r, use more accurate formula
    double mu0 = MU0;
    if (layer_stackup && via->start_layer >= 0 && via->start_layer < layer_stackup->num_layers) {
        // Use layer permeability if available
        mu0 = MU0 * (layer_stackup->mu_r ? layer_stackup->mu_r[via->start_layer] : 1.0);
    }
    
    if (h > 10.0 * r) {
        // Long via: use standard formula
        double ln_term = log(2.0 * h / r);
        return (mu0 * h / TWO_PI) * (ln_term - 0.75);
    } else {
        // Short via: use more accurate formula
        double aspect_ratio = h / r;
        double ln_term = log(1.0 + sqrt(1.0 + aspect_ratio * aspect_ratio));
        double correction = 0.5 * (sqrt(1.0 + aspect_ratio * aspect_ratio) - 1.0);
        return (mu0 * h / TWO_PI) * (ln_term - correction);
    }
}

/**
 * @brief Compute via self capacitance
 * Uses analytical formula for cylindrical conductor in dielectric:
 * C = (2π * ε * h) / ln(r_outer / r_inner)
 * For via, r_outer is determined by surrounding dielectric
 */
double peec_compute_via_capacitance(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup
) {
    if (!via || via->radius <= 0.0 || via->height <= 0.0) {
        return 0.0;
    }
    
    if (!layer_stackup || layer_stackup->num_layers == 0) {
        return 0.0;
    }
    
    // Compute average permittivity across via layers
    double avg_epsilon_r = 1.0;
    int layer_count = 0;
    for (int i = via->start_layer; i <= via->end_layer && i < layer_stackup->num_layers; i++) {
        if (layer_stackup->epsilon_r && i >= 0) {
            avg_epsilon_r += layer_stackup->epsilon_r[i];
            layer_count++;
        }
    }
    if (layer_count > 0) {
        avg_epsilon_r /= layer_count;
    } else {
        avg_epsilon_r = 4.0;  // Default FR4 permittivity
    }
    
    double epsilon = EPS0 * avg_epsilon_r;
    double h = via->height;
    double r = via->radius;
    
    // Estimate outer radius (via pad radius or typical PCB via pad size)
    // Typical via pad is 1.5-2x via radius
    double r_outer = 1.5 * r;  // Default: 1.5x via radius
    if (r_outer <= r) {
        r_outer = r * 1.1;  // Minimum 10% larger
    }
    
    // Capacitance formula for cylindrical capacitor
    // C = (2π * ε * h) / ln(r_outer / r_inner)
    double ln_term = log(r_outer / r);
    if (ln_term <= 0.0) {
        ln_term = NUMERICAL_EPSILON;
    }
    
    return (TWO_PI * epsilon * h) / ln_term;
}

/**
 * @brief Compute via-to-via mutual inductance
 * Uses Neumann's formula for two parallel cylindrical conductors
 */
double peec_compute_via_mutual_inductance(
    const peec_via_t* via1,
    const peec_via_t* via2
) {
    if (!via1 || !via2) {
        return 0.0;
    }
    
    // Compute distance between via centers
    double dx = via1->center[0] - via2->center[0];
    double dy = via1->center[1] - via2->center[1];
    double dz = via1->center[2] - via2->center[2];
    double distance = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (distance < NUMERICAL_EPSILON) {
        return 0.0;  // Same via
    }
    
    // Average height for mutual inductance calculation
    double h_avg = (via1->height + via2->height) * ONE_HALF;
    double r1 = via1->radius;
    double r2 = via2->radius;
    
    // Mutual inductance between two parallel cylindrical conductors
    // M = (μ₀ * h / (2π)) * [ln(2h/d) - 1 + d/h] for h >> d, d >> r
    // where d is center-to-center distance
    double mu0 = MU0;
    
    if (distance > 10.0 * fmax(r1, r2)) {
        // Far-field: use standard formula
        double ln_term = log(2.0 * h_avg / distance);
        return (mu0 * h_avg / TWO_PI) * (ln_term - 1.0 + distance / h_avg);
    } else {
        // Near-field: use more accurate formula with geometric mean radius
        double geom_mean_radius = sqrt(r1 * r2);
        double effective_distance = sqrt(distance * distance + geom_mean_radius * geom_mean_radius);
        double ln_term = log(2.0 * h_avg / effective_distance);
        return (mu0 * h_avg / TWO_PI) * (ln_term - 1.0 + effective_distance / h_avg);
    }
}

/**
 * @brief Compute via-to-via mutual capacitance
 */
double peec_compute_via_mutual_capacitance(
    const peec_via_t* via1,
    const peec_via_t* via2,
    const via_layer_stackup_t* layer_stackup
) {
    if (!via1 || !via2 || !layer_stackup) {
        return 0.0;
    }
    
    // Compute distance between via centers
    double dx = via1->center[0] - via2->center[0];
    double dy = via1->center[1] - via2->center[1];
    double distance = sqrt(dx*dx + dy*dy);
    
    if (distance < NUMERICAL_EPSILON) {
        return 0.0;  // Same via
    }
    
    // Average permittivity
    double avg_epsilon_r = 1.0;
    int layer_count = 0;
    int start_layer = (via1->start_layer < via2->start_layer) ? via1->start_layer : via2->start_layer;
    int end_layer = (via1->end_layer > via2->end_layer) ? via1->end_layer : via2->end_layer;
    
    for (int i = start_layer; i <= end_layer && i < layer_stackup->num_layers; i++) {
        if (layer_stackup->epsilon_r && i >= 0) {
            avg_epsilon_r += layer_stackup->epsilon_r[i];
            layer_count++;
        }
    }
    if (layer_count > 0) {
        avg_epsilon_r /= layer_count;
    } else {
        avg_epsilon_r = 4.0;  // Default FR4
    }
    
    double epsilon = EPS0 * avg_epsilon_r;
    
    // Average height
    double h_avg = (via1->height + via2->height) * ONE_HALF;
    double r1 = via1->radius;
    double r2 = via2->radius;
    
    // Mutual capacitance between two parallel cylindrical conductors
    // C_m = (2π * ε * h) / ln(d / sqrt(r1 * r2))
    // where d is center-to-center distance
    double geom_mean_radius = sqrt(r1 * r2);
    double ln_term = log(distance / geom_mean_radius);
    if (ln_term <= 0.0) {
        ln_term = NUMERICAL_EPSILON;
    }
    
    return (TWO_PI * epsilon * h_avg) / ln_term;
}

/**
 * @brief Compute proximity effect factor
 */
double peec_compute_proximity_effect_factor(
    const peec_via_t* via1,
    const peec_via_t* via2,
    double frequency
) {
    if (!via1 || !via2 || frequency <= 0.0) {
        return 1.0;  // No proximity effect
    }
    
    // Compute distance
    double dx = via1->center[0] - via2->center[0];
    double dy = via1->center[1] - via2->center[1];
    double distance = sqrt(dx*dx + dy*dy);
    
    if (distance < NUMERICAL_EPSILON) {
        return 1.0;
    }
    
    // Compute skin depth for both vias
    double conductivity = via1->material.conductivity;
    if (conductivity <= 0.0) {
        conductivity = 5.8e7;  // Default copper
    }
    double permeability = via1->material.permeability;
    if (permeability <= 0.0) {
        permeability = MU0;
    }
    
    double skin_depth = peec_compute_skin_depth(frequency, conductivity, permeability);
    
    // Proximity effect increases resistance when conductors are close
    // Factor: 1 + (r / d) * exp(-d / skin_depth)
    double r_avg = (via1->radius + via2->radius) * ONE_HALF;
    double proximity_factor = 1.0 + (r_avg / distance) * exp(-distance / skin_depth);
    
    return proximity_factor;
}

/**
 * @brief Compute complete via parasitic parameters
 */
int peec_compute_via_parasitics(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup,
    double frequency,
    via_parasitic_params_t* params
) {
    if (!via || !layer_stackup || !params || frequency < 0.0) {
        return -1;
    }
    
    // Compute DC resistance
    params->resistance = peec_compute_via_resistance_dc(via);
    
    // Compute inductance
    params->inductance = peec_compute_via_inductance(via, layer_stackup);
    
    // Compute capacitance
    params->capacitance = peec_compute_via_capacitance(via, layer_stackup);
    
    // Compute conductance (from capacitance and loss tangent)
    params->conductance = 0.0;
    if (layer_stackup->loss_tangent && via->start_layer >= 0 && via->start_layer < layer_stackup->num_layers) {
        double tan_delta = layer_stackup->loss_tangent[via->start_layer];
        double omega = TWO_PI * frequency;
        params->conductance = omega * params->capacitance * tan_delta;
    }
    
    // Compute skin depth
    double conductivity = via->material.conductivity;
    if (conductivity <= 0.0) {
        conductivity = 5.8e7;  // Default copper
    }
    double permeability = via->material.permeability;
    if (permeability <= 0.0) {
        permeability = MU0;
    }
    params->skin_depth = peec_compute_skin_depth(frequency, conductivity, permeability);
    
    // Compute effective resistance with skin effect
    if (via->use_skin_effect && frequency > 0.0) {
        params->effective_resistance = peec_compute_via_resistance_skin_effect(via, frequency);
    } else {
        params->effective_resistance = params->resistance;
    }
    
    // Proximity effect (would need other vias for full calculation)
    params->proximity_factor = 1.0;  // Default: no proximity effect
    
    params->converged = true;
    
    return 0;
}

/********************************************************************************
 * Via-to-Plane Coupling
 ********************************************************************************/

double peec_compute_via_to_plane_capacitance(
    const peec_via_t* via,
    double plane_z,
    const via_layer_stackup_t* layer_stackup
) {
    if (!via || !layer_stackup) {
        return 0.0;
    }
    
    // Distance from via center to plane
    double distance = fabs(via->center[2] - plane_z);
    if (distance < NUMERICAL_EPSILON) {
        return 0.0;
    }
    
    // Average permittivity
    double avg_epsilon_r = 4.0;  // Default FR4
    if (layer_stackup->epsilon_r && via->start_layer >= 0 && via->start_layer < layer_stackup->num_layers) {
        avg_epsilon_r = layer_stackup->epsilon_r[via->start_layer];
    }
    double epsilon = EPS0 * avg_epsilon_r;
    
    // Via-to-plane capacitance: C = (2π * ε * r) / ln(2d / r)
    // where d is distance to plane, r is via radius
    double r = via->radius;
    double ln_term = log(2.0 * distance / r);
    if (ln_term <= 0.0) {
        ln_term = NUMERICAL_EPSILON;
    }
    
    return (TWO_PI * epsilon * r) / ln_term;
}

double peec_compute_via_to_plane_inductance(
    const peec_via_t* via,
    double plane_z
) {
    if (!via) {
        return 0.0;
    }
    
    // Distance from via center to plane
    double distance = fabs(via->center[2] - plane_z);
    if (distance < NUMERICAL_EPSILON) {
        return 0.0;
    }
    
    // Via-to-plane inductance: L = (μ₀ * h / (2π)) * ln(2d / r)
    double mu0 = MU0;
    double h = via->height;
    double r = via->radius;
    double ln_term = log(2.0 * distance / r);
    
    return (mu0 * h / TWO_PI) * ln_term;
}

/********************************************************************************
 * Utility Functions
 ********************************************************************************/

int peec_via_init(peec_via_t* via) {
    if (!via) {
        return -1;
    }
    
    memset(via, 0, sizeof(peec_via_t));
    via->material = peec_get_default_via_material();
    via->type = VIA_TYPE_THROUGH_HOLE;
    via->use_skin_effect = true;
    via->use_proximity_effect = true;
    via->num_segments = 8;  // Default: 8 segments for cylindrical modeling
    
    return 0;
}

void peec_via_free(peec_via_t* via) {
    if (!via) {
        return;
    }
    
    if (via->segment_indices) {
        free(via->segment_indices);
        via->segment_indices = NULL;
    }
    if (via->resistance_freq) {
        free(via->resistance_freq);
        via->resistance_freq = NULL;
    }
    if (via->inductance_freq) {
        free(via->inductance_freq);
        via->inductance_freq = NULL;
    }
    if (via->capacitance_freq) {
        free(via->capacitance_freq);
        via->capacitance_freq = NULL;
    }
    if (via->frequencies) {
        free(via->frequencies);
        via->frequencies = NULL;
    }
    if (via->coupled_via_indices) {
        free(via->coupled_via_indices);
        via->coupled_via_indices = NULL;
    }
    if (via->mutual_inductance) {
        free(via->mutual_inductance);
        via->mutual_inductance = NULL;
    }
    if (via->mutual_capacitance) {
        free(via->mutual_capacitance);
        via->mutual_capacitance = NULL;
    }
}

// Placeholder implementations for functions that need mesh integration
int peec_create_cylindrical_via_mesh(const peec_via_t* via, int num_segments, mesh_t* mesh) {
    // TODO: Implement precise cylindrical mesh generation
    return 0;
}

int peec_add_via_to_triangular_mesh(mesh_t* mesh, const peec_via_t* via) {
    // TODO: Implement via addition to triangular mesh
    return 0;
}

int peec_extract_via_partial_elements_triangular(
    const mesh_t* mesh,
    const peec_via_t* via,
    double frequency,
    via_parasitic_params_t* params
) {
    // TODO: Implement via partial element extraction from triangular mesh
    return 0;
}

complex_t peec_compute_via_greens_function_wgf(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup,
    double frequency,
    const LayeredMedium* medium,
    const FrequencyDomain* freq
) {
    // TODO: Implement WGF-based Green's function for via
    return complex_zero();
}

int peec_convert_via_node_to_peec_via(const void* via_node, peec_via_t* peec_via) {
    // TODO: Convert via_node_t from discretization/mesh/manhattan_mesh_peec.c to peec_via_t
    return 0;
}

