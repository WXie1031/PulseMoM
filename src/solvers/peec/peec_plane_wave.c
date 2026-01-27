/******************************************************************************
 * PEEC Solver Plane Wave Excitation - Implementation
 ******************************************************************************/

#include "peec_plane_wave.h"
#include "peec_solver.h"
#include "../../physics/excitation/excitation_plane_wave.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../common/core_common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

int peec_solver_add_plane_wave_excitation(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation
) {
    if (!solver || !excitation) {
        return -1;
    }
    
    // Convert plane wave to current sources and add to solver
    peec_scalar_complex_t* current_sources = NULL;
    int num_sources = 0;
    
    if (peec_plane_wave_to_current_sources(solver, excitation, &current_sources, &num_sources) != 0) {
        return -1;
    }
    
    // Add current sources to PEEC circuit network
    // This would modify the solver's excitation vector
    // Implementation depends on PEEC solver's internal structure
    
    if (current_sources) {
        free(current_sources);
    }
    
    return 0;
}

int peec_plane_wave_to_current_sources(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation,
    peec_scalar_complex_t** current_sources,
    int* num_sources
) {
    if (!solver || !excitation || !current_sources || !num_sources) {
        return -1;
    }
    
    // Get mesh from solver
    const mesh_t* mesh = NULL;
    // Note: This assumes solver has a mesh pointer
    // mesh = solver->mesh;
    
    if (!mesh) {
        return -1;
    }
    
    // Allocate current sources array
    *num_sources = mesh->num_elements;
    *current_sources = (peec_scalar_complex_t*)calloc(*num_sources, sizeof(peec_scalar_complex_t));
    if (!*current_sources) {
        return -1;
    }
    
    // Compute induced current for each element
    for (int i = 0; i < mesh->num_elements; i++) {
        peec_scalar_complex_t current_density[3];
        if (peec_plane_wave_compute_induced_current(solver, excitation, i, current_density) == 0) {
            // Convert current density to equivalent current source
            // For PEEC, the current source is the integral of current density over element
            // Simplified: I = J * area
            double area = 1e-6;  // Estimate element area (1 mm²)
            
            // Compute magnitude of current density
            double J_mag = sqrt(current_density[0].re * current_density[0].re + 
                               current_density[0].im * current_density[0].im +
                               current_density[1].re * current_density[1].re + 
                               current_density[1].im * current_density[1].im +
                               current_density[2].re * current_density[2].re + 
                               current_density[2].im * current_density[2].im);
            
            // Convert to complex current source
            // Use the dominant component (simplified)
            (*current_sources)[i] = current_density[0];
            (*current_sources)[i].re *= area;
            (*current_sources)[i].im *= area;
        }
    }
    
    return 0;
}

int peec_plane_wave_to_voltage_sources(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation,
    peec_scalar_complex_t** voltage_sources,
    int* num_sources
) {
    if (!solver || !excitation || !voltage_sources || !num_sources) {
        return -1;
    }
    
    // Convert plane wave to voltage sources using Faraday's law
    // V_induced = -d(phi)/dt = -j*omega*B*A
    // where B is magnetic flux density and A is area
    
    const mesh_t* mesh = NULL;
    // mesh = solver->mesh;
    
    if (!mesh) {
        return -1;
    }
    
    double omega = 2.0 * M_PI * excitation->frequency;
    
    *num_sources = mesh->num_elements;
    *voltage_sources = (peec_scalar_complex_t*)calloc(*num_sources, sizeof(peec_scalar_complex_t));
    if (!*voltage_sources) {
        return -1;
    }
    
    // Compute induced voltage for each element
    for (int i = 0; i < mesh->num_elements; i++) {
        // Get element center
        point3d_t center = {0.0, 0.0, 0.0};
        int num_vertices = mesh->elements[i].num_vertices;
        
        if (num_vertices > 0) {
            for (int v = 0; v < num_vertices; v++) {
                int vidx = mesh->elements[i].vertices[v];
                if (vidx < mesh->num_vertices) {
                    center.x += mesh->vertices[vidx].position.x;
                    center.y += mesh->vertices[vidx].position.y;
                    center.z += mesh->vertices[vidx].position.z;
                }
            }
            center.x /= num_vertices;
            center.y /= num_vertices;
            center.z /= num_vertices;
        }
        
        // Compute magnetic field
        double H[3];
        if (excitation_plane_wave_compute_magnetic_field(excitation, 
                                                          center.x, center.y, center.z, 
                                                          H) != 0) {
            (*voltage_sources)[i].re = 0.0;
            (*voltage_sources)[i].im = 0.0;
            continue;
        }
        
        // Compute magnetic flux density: B = mu0 * H
        double B[3];
        B[0] = MU0 * H[0];
        B[1] = MU0 * H[1];
        B[2] = MU0 * H[2];
        
        // Estimate element area and normal
        double area = 1e-6;  // Default area
        double normal[3] = {0.0, 0.0, 1.0};  // Default normal (z-direction)
        
        // Compute flux: phi = B · n * A
        double flux = (B[0] * normal[0] + B[1] * normal[1] + B[2] * normal[2]) * area;
        
        // Induced voltage: V = -j*omega*phi
        (*voltage_sources)[i].re = omega * flux;  // -j*omega = omega * (0 - j) = omega * (imaginary part)
        (*voltage_sources)[i].im = 0.0;
    }
    
    return 0;
}

int peec_plane_wave_compute_induced_current(
    peec_solver_t* solver,
    const excitation_plane_wave_t* excitation,
    int element_index,
    peec_scalar_complex_t current_density[3]
) {
    if (!solver || !excitation || !current_density || element_index < 0) {
        return -1;
    }
    
    const mesh_t* mesh = NULL;
    // mesh = solver->mesh;
    
    if (!mesh || element_index >= mesh->num_elements) {
        return -1;
    }
    
    // Get element center
    point3d_t center = {0.0, 0.0, 0.0};
    int num_vertices = mesh->elements[element_index].num_vertices;
    
    if (num_vertices > 0) {
        for (int v = 0; v < num_vertices; v++) {
            int vidx = mesh->elements[element_index].vertices[v];
            if (vidx < mesh->num_vertices) {
                center.x += mesh->vertices[vidx].position.x;
                center.y += mesh->vertices[vidx].position.y;
                center.z += mesh->vertices[vidx].position.z;
            }
        }
        center.x /= num_vertices;
        center.y /= num_vertices;
        center.z /= num_vertices;
    }
    
    // Compute incident electric field
    double E_inc[3];
    if (excitation_plane_wave_compute_electric_field(excitation, 
                                                      center.x, center.y, center.z, 
                                                      E_inc) != 0) {
        current_density[0].re = 0.0;
        current_density[0].im = 0.0;
        current_density[1].re = 0.0;
        current_density[1].im = 0.0;
        current_density[2].re = 0.0;
        current_density[2].im = 0.0;
        return -1;
    }
    
    // For PEC (Perfect Electric Conductor), induced current density is:
    // J = sigma * E_inc (for finite conductivity)
    // For perfect conductor, use surface current: J_s = n × H
    // Simplified: use E_inc directly (assuming surface impedance model)
    
    double conductivity = 5.8e7;  // Copper conductivity (S/m)
    double skin_depth = sqrt(2.0 / (2.0 * M_PI * excitation->frequency * MU0 * conductivity));
    
    // Surface current density: J_s ≈ E_inc / Z_surface
    // Z_surface = (1+j) / (sigma * skin_depth)
    double Z_surface_real = 1.0 / (conductivity * skin_depth);
    double Z_surface_imag = 1.0 / (conductivity * skin_depth);
    
    // Convert to complex
    for (int i = 0; i < 3; i++) {
        // J = E / Z_surface
        double denom = Z_surface_real * Z_surface_real + Z_surface_imag * Z_surface_imag;
        if (denom > 1e-12) {
            current_density[i].re = (E_inc[i] * Z_surface_real) / denom;
            current_density[i].im = -(E_inc[i] * Z_surface_imag) / denom;
        } else {
            current_density[i].re = 0.0;
            current_density[i].im = 0.0;
        }
    }
    
    return 0;
}
