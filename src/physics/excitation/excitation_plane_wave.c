/******************************************************************************
 * Plane Wave Excitation - Implementation
 ******************************************************************************/

#include "excitation_plane_wave.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include "core_common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

excitation_plane_wave_t excitation_plane_wave_create(
    double frequency,
    double amplitude,
    double theta,
    double phi,
    excitation_plane_wave_polarization_t polarization
) {
    excitation_plane_wave_t pw = {0};
    pw.frequency = frequency;
    pw.amplitude = amplitude;
    pw.theta = theta;
    pw.phi = phi;
    pw.polarization = polarization;
    pw.phase = 0.0;
    pw.polarization_angle = 0.0;
    pw.axial_ratio = 1.0;
    pw.x0 = 0.0;
    pw.y0 = 0.0;
    pw.z0 = 0.0;
    
    // Compute wave vector
    double theta_rad = theta * M_PI / 180.0;
    double phi_rad = phi * M_PI / 180.0;
    double k0 = 2.0 * M_PI * frequency / C0;
    
    pw.kx = k0 * sin(theta_rad) * cos(phi_rad);
    pw.ky = k0 * sin(theta_rad) * sin(phi_rad);
    pw.kz = k0 * cos(theta_rad);
    
    return pw;
}

int excitation_plane_wave_compute_electric_field(
    const excitation_plane_wave_t* excitation,
    double x, double y, double z,
    double E[3]
) {
    if (!excitation || !E) {
        return -1;
    }
    
    // Compute phase
    double phase = excitation->kx * (x - excitation->x0) +
                   excitation->ky * (y - excitation->y0) +
                   excitation->kz * (z - excitation->z0) +
                   excitation->phase * M_PI / 180.0;
    
    double cos_phase = cos(phase);
    double sin_phase = sin(phase);
    
    // Compute electric field based on polarization
    switch (excitation->polarization) {
        case EXCITATION_PLANE_WAVE_TE: {
            // Transverse Electric: E perpendicular to wave vector
            double theta_rad = excitation->theta * M_PI / 180.0;
            double phi_rad = excitation->phi * M_PI / 180.0;
            
            // Perpendicular direction
            E[0] = excitation->amplitude * (-sin(phi_rad)) * cos_phase;
            E[1] = excitation->amplitude * cos(phi_rad) * cos_phase;
            E[2] = 0.0;
            break;
        }
        case EXCITATION_PLANE_WAVE_TM: {
            // Transverse Magnetic: E parallel to wave vector projection
            double theta_rad = excitation->theta * M_PI / 180.0;
            double phi_rad = excitation->phi * M_PI / 180.0;
            
            E[0] = excitation->amplitude * cos(theta_rad) * cos(phi_rad) * cos_phase;
            E[1] = excitation->amplitude * cos(theta_rad) * sin(phi_rad) * cos_phase;
            E[2] = excitation->amplitude * (-sin(theta_rad)) * cos_phase;
            break;
        }
        case EXCITATION_PLANE_WAVE_CIRCULAR_LH:
        case EXCITATION_PLANE_WAVE_CIRCULAR_RH:
        case EXCITATION_PLANE_WAVE_ELLIPTICAL:
        default:
            // More complex polarization - simplified for now
            E[0] = excitation->amplitude * cos_phase;
            E[1] = excitation->amplitude * sin_phase;
            E[2] = 0.0;
            break;
    }
    
    return 0;
}

int excitation_plane_wave_compute_magnetic_field(
    const excitation_plane_wave_t* excitation,
    double x, double y, double z,
    double H[3]
) {
    if (!excitation || !H) {
        return -1;
    }
    
    // Compute electric field first
    double E[3];
    if (excitation_plane_wave_compute_electric_field(excitation, x, y, z, E) != 0) {
        return -1;
    }
    
    // H = (k × E) / (omega * mu0)
    double omega = 2.0 * M_PI * excitation->frequency;
    double k_mag = sqrt(excitation->kx * excitation->kx + 
                       excitation->ky * excitation->ky + 
                       excitation->kz * excitation->kz);
    
    // H = k × E / (omega * mu0)
    H[0] = (excitation->ky * E[2] - excitation->kz * E[1]) / (omega * MU0);
    H[1] = (excitation->kz * E[0] - excitation->kx * E[2]) / (omega * MU0);
    H[2] = (excitation->kx * E[1] - excitation->ky * E[0]) / (omega * MU0);
    
    return 0;
}

int excitation_plane_wave_to_mom(
    const excitation_plane_wave_t* excitation,
    const void* mesh,
    void* excitation_vector
) {
    if (!excitation || !mesh || !excitation_vector) {
        return -1;
    }
    
    // Convert plane wave to MoM excitation vector
    // For RWG basis functions: V_i = ∫ E_inc · f_i dS
    // where f_i is the basis function and E_inc is the incident electric field
    
    const mesh_t* m = (const mesh_t*)mesh;
    complex_t* V = (complex_t*)excitation_vector;
    
    if (!m || !V) {
        return -1;
    }
    
    double omega = 2.0 * M_PI * excitation->frequency;
    double k0 = omega / C0;
    
    // Iterate over all elements (assuming each element has a basis function)
    for (int i = 0; i < m->num_elements; i++) {
        if (i >= m->num_elements) break;
        
        // Get element center for computing incident field
        point3d_t center = {0.0, 0.0, 0.0};
        int num_vertices = m->elements[i].num_vertices;
        
        if (num_vertices > 0) {
            for (int v = 0; v < num_vertices; v++) {
                int vidx = m->elements[i].vertices[v];
                if (vidx < m->num_vertices) {
                    center.x += m->vertices[vidx].position.x;
                    center.y += m->vertices[vidx].position.y;
                    center.z += m->vertices[vidx].position.z;
                }
            }
            center.x /= num_vertices;
            center.y /= num_vertices;
            center.z /= num_vertices;
        }
        
        // Compute incident electric field at element center
        double E_inc[3];
        if (excitation_plane_wave_compute_electric_field(excitation, 
                                                          center.x, center.y, center.z, 
                                                          E_inc) != 0) {
            V[i].re = 0.0;
            V[i].im = 0.0;
            continue;
        }
        
        // For RWG basis functions, the excitation is:
        // V_i = l_i * ∫ E_inc · (r - r+) dS
        // Simplified: V_i ≈ l_i * E_inc · (r_center - r_plus) * area
        
        // Estimate element area and edge length
        double area = 1e-6;  // Default area (1 mm²)
        double edge_length = 1e-3;  // Default edge length (1 mm)
        
        // Compute basis function direction (simplified: use element normal or edge direction)
        double basis_dir[3] = {1.0, 0.0, 0.0};  // Default direction
        
        // For RWG, the basis function is along an edge
        // Simplified: use element normal as basis direction
        if (num_vertices >= 3) {
            // Compute two edge vectors
            int v0 = m->elements[i].vertices[0];
            int v1 = m->elements[i].vertices[1];
            int v2 = m->elements[i].vertices[2];
            
            if (v0 < m->num_vertices && v1 < m->num_vertices && v2 < m->num_vertices) {
                double v1x = m->vertices[v1].position.x - m->vertices[v0].position.x;
                double v1y = m->vertices[v1].position.y - m->vertices[v0].position.y;
                double v1z = m->vertices[v1].position.z - m->vertices[v0].position.z;
                
                double v2x = m->vertices[v2].position.x - m->vertices[v0].position.x;
                double v2y = m->vertices[v2].position.y - m->vertices[v0].position.y;
                double v2z = m->vertices[v2].position.z - m->vertices[v0].position.z;
                
                // Cross product for normal
                basis_dir[0] = v1y * v2z - v1z * v2y;
                basis_dir[1] = v1z * v2x - v1x * v2z;
                basis_dir[2] = v1x * v2y - v1y * v2x;
                
                // Normalize
                double norm = sqrt(basis_dir[0]*basis_dir[0] + 
                                  basis_dir[1]*basis_dir[1] + 
                                  basis_dir[2]*basis_dir[2]);
                if (norm > 1e-12) {
                    basis_dir[0] /= norm;
                    basis_dir[1] /= norm;
                    basis_dir[2] /= norm;
                }
                
                // Estimate area from cross product magnitude
                area = 0.5 * norm;
            }
        }
        
        // Compute dot product: E_inc · basis_direction
        double dot_product = E_inc[0] * basis_dir[0] + 
                            E_inc[1] * basis_dir[1] + 
                            E_inc[2] * basis_dir[2];
        
        // Excitation vector element: V_i = l_i * E_inc · f_i
        // For RWG: f_i has magnitude l_i and direction along edge
        // Simplified: V_i ≈ edge_length * dot_product * area_factor
        double excitation_value = edge_length * dot_product * area;
        
        // Convert to complex (phasor representation)
        // The phase is already included in E_inc computation
        // For time-harmonic: V = |V| * exp(j*phase)
        double phase = excitation->kx * center.x + 
                       excitation->ky * center.y + 
                       excitation->kz * center.z +
                       excitation->phase * M_PI / 180.0;
        
        V[i].re = excitation_value * cos(phase);
        V[i].im = excitation_value * sin(phase);
    }
    
    return 0;
}

int excitation_plane_wave_set_polarization(
    excitation_plane_wave_t* excitation,
    excitation_plane_wave_polarization_t polarization,
    double polarization_angle,
    double axial_ratio
) {
    if (!excitation) {
        return -1;
    }
    
    excitation->polarization = polarization;
    excitation->polarization_angle = polarization_angle;
    excitation->axial_ratio = axial_ratio;
    
    return 0;
}
