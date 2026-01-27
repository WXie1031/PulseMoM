/******************************************************************************
 * Quasistatic Coupling - Implementation
 ******************************************************************************/

#include "quasistatic_coupling.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../common/core_common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

coupling_quasistatic_config_t coupling_quasistatic_get_default_config(void) {
    coupling_quasistatic_config_t config = {0};
    config.coupling_type = COUPLING_QUASISTATIC_BOTH;
    config.frequency_threshold = 1e6;  // 1 MHz
    config.distance_threshold = 1e-3;  // 1 mm
    config.include_self_coupling = true;
    config.use_analytical_formulas = true;
    config.relative_tolerance = 1e-6;
    return config;
}

int coupling_quasistatic_compute_matrix(
    const geom_geometry_t* geometry,
    const mesh_t* mesh,
    const coupling_quasistatic_config_t* config,
    coupling_quasistatic_matrix_t* coupling_matrix
) {
    if (!geometry || !mesh || !config || !coupling_matrix) {
        return -1;
    }
    
    int num_elements = mesh->num_elements;
    
    // Allocate matrices
    coupling_matrix->num_elements = num_elements;
    coupling_matrix->C_matrix = (double**)calloc(num_elements, sizeof(double*));
    coupling_matrix->L_matrix = (double**)calloc(num_elements, sizeof(double*));
    coupling_matrix->K_matrix = (double**)calloc(num_elements, sizeof(double*));
    
    if (!coupling_matrix->C_matrix || !coupling_matrix->L_matrix || !coupling_matrix->K_matrix) {
        return -1;
    }
    
    for (int i = 0; i < num_elements; i++) {
        coupling_matrix->C_matrix[i] = (double*)calloc(num_elements, sizeof(double));
        coupling_matrix->L_matrix[i] = (double*)calloc(num_elements, sizeof(double));
        coupling_matrix->K_matrix[i] = (double*)calloc(num_elements, sizeof(double));
        
        if (!coupling_matrix->C_matrix[i] || !coupling_matrix->L_matrix[i] || !coupling_matrix->K_matrix[i]) {
            coupling_quasistatic_free_matrix(coupling_matrix);
            return -1;
        }
    }
    
    // Compute coupling matrices
    // For each pair of elements, compute capacitive and inductive coupling
    
    for (int i = 0; i < num_elements; i++) {
        for (int j = 0; j < num_elements; j++) {
            if (i == j && !config->include_self_coupling) {
                coupling_matrix->C_matrix[i][j] = 0.0;
                coupling_matrix->L_matrix[i][j] = 0.0;
                coupling_matrix->K_matrix[i][j] = 0.0;
                continue;
            }
            
            // Get element centers
            point3d_t center_i = {0.0, 0.0, 0.0};
            point3d_t center_j = {0.0, 0.0, 0.0};
            
            if (i < mesh->num_elements && mesh->elements[i].num_vertices > 0) {
                // Compute element center
                for (int v = 0; v < mesh->elements[i].num_vertices; v++) {
                    int vidx = mesh->elements[i].vertices[v];
                    if (vidx < mesh->num_vertices) {
                        center_i.x += mesh->vertices[vidx].position.x;
                        center_i.y += mesh->vertices[vidx].position.y;
                        center_i.z += mesh->vertices[vidx].position.z;
                    }
                }
                center_i.x /= mesh->elements[i].num_vertices;
                center_i.y /= mesh->elements[i].num_vertices;
                center_i.z /= mesh->elements[i].num_vertices;
            }
            
            if (j < mesh->num_elements && mesh->elements[j].num_vertices > 0) {
                for (int v = 0; v < mesh->elements[j].num_vertices; v++) {
                    int vidx = mesh->elements[j].vertices[v];
                    if (vidx < mesh->num_vertices) {
                        center_j.x += mesh->vertices[vidx].position.x;
                        center_j.y += mesh->vertices[vidx].position.y;
                        center_j.z += mesh->vertices[vidx].position.z;
                    }
                }
                center_j.x /= mesh->elements[j].num_vertices;
                center_j.y /= mesh->elements[j].num_vertices;
                center_j.z /= mesh->elements[j].num_vertices;
            }
            
            // Compute distance
            double dx = center_j.x - center_i.x;
            double dy = center_j.y - center_i.y;
            double dz = center_j.z - center_i.z;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            // Skip if too far
            if (distance > config->distance_threshold && i != j) {
                coupling_matrix->C_matrix[i][j] = 0.0;
                coupling_matrix->L_matrix[i][j] = 0.0;
                coupling_matrix->K_matrix[i][j] = 0.0;
                continue;
            }
            
            // Compute capacitance (simplified: parallel plate approximation)
            if (config->coupling_type == COUPLING_QUASISTATIC_CAPACITIVE || 
                config->coupling_type == COUPLING_QUASISTATIC_BOTH) {
                if (i == j) {
                    // Self-capacitance (simplified)
                    double area = 1e-6;  // Estimate element area (1 mm²)
                    double eps_r = 1.0;  // Relative permittivity (default: air)
                    coupling_matrix->C_matrix[i][j] = EPS0 * eps_r * area / (distance + 1e-6);
                } else {
                    // Mutual capacitance
                    double area = 1e-6;
                    double eps_r = 1.0;
                    coupling_matrix->C_matrix[i][j] = EPS0 * eps_r * area / (distance + 1e-6);
                }
            }
            
            // Compute inductance (simplified: two-wire approximation)
            if (config->coupling_type == COUPLING_QUASISTATIC_INDUCTIVE || 
                config->coupling_type == COUPLING_QUASISTATIC_BOTH) {
                if (i == j) {
                    // Self-inductance (simplified)
                    double length = 1e-3;  // Estimate element length (1 mm)
                    double radius = 1e-5;  // Conductor radius (10 um)
                    coupling_matrix->L_matrix[i][j] = MU0 * length / (2.0 * M_PI) * 
                                                       log(length / radius + sqrt(1.0 + length*length/(radius*radius)));
                } else {
                    // Mutual inductance (two parallel wires)
                    double length = 1e-3;
                    double radius = 1e-5;
                    if (distance > radius) {
                        coupling_matrix->L_matrix[i][j] = MU0 * length / (2.0 * M_PI) * 
                                                           log(distance / radius);
                    } else {
                        coupling_matrix->L_matrix[i][j] = 0.0;
                    }
                }
            }
            
            // Compute coupling coefficient
            if (coupling_matrix->L_matrix[i][i] > 0.0 && coupling_matrix->L_matrix[j][j] > 0.0) {
                coupling_matrix->K_matrix[i][j] = coupling_matrix->L_matrix[i][j] / 
                                                  sqrt(coupling_matrix->L_matrix[i][i] * 
                                                       coupling_matrix->L_matrix[j][j]);
            } else {
                coupling_matrix->K_matrix[i][j] = 0.0;
            }
        }
    }
    
    coupling_matrix->is_symmetric = true;
    
    return 0;
}

int coupling_quasistatic_compute_capacitive(
    const void* conductor1,
    const void* conductor2,
    const void* medium,
    double* capacitance
) {
    if (!conductor1 || !conductor2 || !capacitance) {
        return -1;
    }
    
    // Simplified capacitance calculation
    // For parallel conductors, use: C = ε * A / d
    // For more complex geometries, would need numerical integration
    
    // Assume conductor1 and conductor2 are point3d_t or have position information
    // For now, use a simplified model
    
    double eps_r = 1.0;  // Default relative permittivity (air)
    if (medium) {
        // Extract permittivity from medium structure if available
        // eps_r = ((medium_properties_t*)medium)->epsilon_r;
    }
    
    // Simplified: assume parallel plate approximation
    // In practice, would compute based on actual conductor geometry
    double area = 1e-6;      // Effective area (1 mm²)
    double distance = 1e-3;  // Distance between conductors (1 mm)
    
    *capacitance = EPS0 * eps_r * area / distance;
    
    return 0;
}

int coupling_quasistatic_compute_inductive(
    const void* conductor1,
    const void* conductor2,
    const void* medium,
    double* inductance
) {
    if (!conductor1 || !conductor2 || !inductance) {
        return -1;
    }
    
    // Simplified inductance calculation
    // For two parallel wires: L = (μ₀ * l) / (2π) * ln(d/r)
    // where l is length, d is distance, r is wire radius
    
    double mu_r = 1.0;  // Default relative permeability
    if (medium) {
        // Extract permeability from medium structure if available
        // mu_r = ((medium_properties_t*)medium)->mu_r;
    }
    
    // Simplified: assume two parallel wires
    double length = 1e-3;     // Conductor length (1 mm)
    double distance = 1e-3;   // Distance between conductors (1 mm)
    double radius = 1e-5;     // Wire radius (10 um)
    
    if (distance > radius) {
        *inductance = MU0 * mu_r * length / (2.0 * M_PI) * log(distance / radius);
    } else {
        *inductance = 0.0;
    }
    
    return 0;
}

int coupling_quasistatic_compute_coefficient(
    double L12,
    double L11,
    double L22,
    double* k
) {
    if (!k || L11 <= 0.0 || L22 <= 0.0) {
        return -1;
    }
    
    // Coupling coefficient: k = L12 / sqrt(L11 * L22)
    *k = L12 / sqrt(L11 * L22);
    
    return 0;
}

void coupling_quasistatic_free_matrix(coupling_quasistatic_matrix_t* coupling_matrix) {
    if (!coupling_matrix) {
        return;
    }
    
    if (coupling_matrix->C_matrix) {
        for (int i = 0; i < coupling_matrix->num_elements; i++) {
            if (coupling_matrix->C_matrix[i]) {
                free(coupling_matrix->C_matrix[i]);
            }
        }
        free(coupling_matrix->C_matrix);
        coupling_matrix->C_matrix = NULL;
    }
    
    if (coupling_matrix->L_matrix) {
        for (int i = 0; i < coupling_matrix->num_elements; i++) {
            if (coupling_matrix->L_matrix[i]) {
                free(coupling_matrix->L_matrix[i]);
            }
        }
        free(coupling_matrix->L_matrix);
        coupling_matrix->L_matrix = NULL;
    }
    
    if (coupling_matrix->K_matrix) {
        for (int i = 0; i < coupling_matrix->num_elements; i++) {
            if (coupling_matrix->K_matrix[i]) {
                free(coupling_matrix->K_matrix[i]);
            }
        }
        free(coupling_matrix->K_matrix);
        coupling_matrix->K_matrix = NULL;
    }
    
    coupling_matrix->num_elements = 0;
    coupling_matrix->is_symmetric = false;
}
