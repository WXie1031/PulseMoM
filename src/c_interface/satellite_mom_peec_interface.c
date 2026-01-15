/********************************************************************************
 * Satellite MoM/PEEC C Library Interface Implementation
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Commercial License - All Rights Reserved
 * Implements ctypes-compatible interface for calling C solvers from Python
 ********************************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include "satellite_mom_peec_interface.h"
#include "../solvers/mom/mom_solver.h"
#include "../solvers/peec/peec_solver.h"
#include "../mesh/mesh_engine.h"
#include "../core/core_geometry.h"
#include "../cad/cad_mesh_generation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Internal simulation structure
struct satellite_simulation {
    mom_solver_t* mom_solver;
    peec_solver_t* peec_solver;
    mesh_engine_t* mesh_engine;
    geom_geometry_t* geometry;
    mesh_t* mesh;
    
    // Configuration
    satellite_solver_config_t solver_config;
    satellite_material_t material;
    satellite_excitation_t excitation;
    satellite_mesh_params_t mesh_params;
    
    // Results
    satellite_currents_t* currents;
    satellite_fields_t* fields;
    satellite_rcs_t* rcs;
    satellite_performance_t* performance;
    satellite_rwg_basis_t* rwg_basis;
    
    // State
    bool mesh_generated;
    bool solver_configured;
    bool simulation_completed;
    char error_message[256];
};

/*********************************************************************
 * Utility Functions
 *********************************************************************/

const char* satellite_get_error_string(satellite_error_t error) {
    switch (error) {
        case SATELLITE_SUCCESS: return "Success";
        case SATELLITE_ERROR_INVALID_INPUT: return "Invalid input parameters";
        case SATELLITE_ERROR_FILE_NOT_FOUND: return "File not found";
        case SATELLITE_ERROR_MESH_GENERATION: return "Mesh generation failed";
        case SATELLITE_ERROR_SOLVER_FAILED: return "Solver failed to converge";
        case SATELLITE_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case SATELLITE_ERROR_INVALID_GEOMETRY: return "Invalid geometry";
        case SATELLITE_ERROR_INVALID_FREQUENCY: return "Invalid frequency";
        case SATELLITE_ERROR_LIBRARY_NOT_FOUND: return "Required library not found";
        default: return "Unknown error";
    }
}

const char* satellite_get_version(void) {
    return "Satellite MoM/PEEC Interface v1.0.0";
}

/*********************************************************************
 * Simulation Lifecycle
 *********************************************************************/

satellite_simulation_t* satellite_simulation_create(void) {
    satellite_simulation_t* sim = (satellite_simulation_t*)calloc(1, sizeof(satellite_simulation_t));
    if (!sim) {
        return NULL;
    }
    
    // Create solver instances with default configs
    mom_config_t mom_config = {0};
    mom_config.frequency = 1e9;  // Default 1 GHz
    mom_config.tolerance = 1e-6;
    mom_config.max_iterations = 1000;
    
    peec_config_t peec_config = {0};
    peec_config.frequency = 1e9;  // Default 1 GHz
    peec_config.circuit_tolerance = 1e-6;
    peec_config.circuit_max_iterations = 1000;
    
    sim->mom_solver = mom_solver_create(&mom_config);
    sim->peec_solver = peec_solver_create(&peec_config);
    sim->mesh_engine = mesh_engine_create();
    
    if (!sim->mom_solver || !sim->peec_solver || !sim->mesh_engine) {
        satellite_simulation_destroy(sim);
        return NULL;
    }
    
    // Set default configuration
    strcpy(sim->solver_config.solver_type, "mom");
    sim->solver_config.frequency = 10e9;  // 10 GHz
    sim->solver_config.basis_order = 1;
    strcpy(sim->solver_config.formulation, "efie");
    sim->solver_config.tolerance = 1e-6;
    sim->solver_config.max_iterations = 1000;
    sim->solver_config.use_preconditioner = true;
    sim->solver_config.use_fast_solver = false;
    
    // Set default mesh parameters
    sim->mesh_params.target_edge_length = 0.03;  // 3 cm at 10 GHz (lambda/10)
    sim->mesh_params.max_facets = 10000;
    sim->mesh_params.min_quality = 0.3;
    sim->mesh_params.adaptive_refinement = false;
    strcpy(sim->mesh_params.mesh_algorithm, "delaunay");
    
    // Set default material (PEC)
    strcpy(sim->material.name, "PEC");
    sim->material.eps_r = 1.0;
    sim->material.mu_r = 1.0;
    sim->material.sigma = 1e20;  // Perfect conductor
    sim->material.tan_delta = 0.0;
    sim->material.material_id = 1;
    
    // Set default excitation (10 GHz plane wave)
    sim->excitation.frequency = 10e9;
    sim->excitation.amplitude = 1.0;
    sim->excitation.phase = 0.0;
    sim->excitation.theta = 0.0;   // Normal incidence
    sim->excitation.phi = 0.0;     // X-polarized
    sim->excitation.polarization = 0.0;  // TE
    
    return sim;
}

void satellite_simulation_destroy(satellite_simulation_t* sim) {
    if (!sim) return;
    
    // Free results
    if (sim->currents) satellite_free_currents(sim->currents);
    if (sim->fields) satellite_free_fields(sim->fields);
    if (sim->rcs) satellite_free_rcs(sim->rcs);
    if (sim->performance) satellite_free_performance(sim->performance);
    if (sim->rwg_basis) satellite_free_rwg_basis(sim->rwg_basis);
    
    // Destroy solvers
    if (sim->mom_solver) mom_solver_destroy(sim->mom_solver);
    if (sim->peec_solver) peec_solver_destroy(sim->peec_solver);
    if (sim->mesh_engine) mesh_engine_destroy(sim->mesh_engine);
    
    // Free geometry and mesh
    if (sim->geometry) {
        // Implementation depends on geometry structure
        free(sim->geometry);
    }
    if (sim->mesh) {
        // Implementation depends on mesh structure
        free(sim->mesh);
    }
    
    free(sim);
}

/*********************************************************************
 * Geometry and Mesh
 *********************************************************************/

satellite_error_t satellite_load_stl(satellite_simulation_t* sim, const char* stl_filename) {
    if (!sim || !stl_filename) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    // Import STL file using CAD interface
    int result = mom_solver_import_cad(sim->mom_solver, stl_filename, "STL");
    if (result != 0) {
        snprintf(sim->error_message, sizeof(sim->error_message), 
                "Failed to import STL file: %s", stl_filename);
        return SATELLITE_ERROR_FILE_NOT_FOUND;
    }
    
    // Also import for PEEC solver if needed
    if (strcmp(sim->solver_config.solver_type, "peec") == 0) {
        result = peec_solver_import_cad(sim->peec_solver, stl_filename, "STL");
        if (result != 0) {
            snprintf(sim->error_message, sizeof(sim->error_message), 
                    "Failed to import STL file for PEEC: %s", stl_filename);
            return SATELLITE_ERROR_FILE_NOT_FOUND;
        }
    }
    
    return SATELLITE_SUCCESS;
}

satellite_error_t satellite_set_material(satellite_simulation_t* sim, const satellite_material_t* material) {
    if (!sim || !material) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    memcpy(&sim->material, material, sizeof(satellite_material_t));
    
    // Configure material in solvers (implementation depends on solver API)
    // This would involve setting material properties in the respective solvers
    
    return SATELLITE_SUCCESS;
}

satellite_error_t satellite_generate_mesh(satellite_simulation_t* sim, const satellite_mesh_params_t* params) {
    if (!sim || !params) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    // Store mesh parameters
    memcpy(&sim->mesh_params, params, sizeof(satellite_mesh_params_t));
    
    // Create mesh request
    mesh_request_t request = {0};
    request.geometry = sim->geometry;
    request.format = MESH_FORMAT_STL;
    request.mom_enabled = (strcmp(sim->solver_config.solver_type, "mom") == 0);
    request.peec_enabled = (strcmp(sim->solver_config.solver_type, "peec") == 0);
    request.preferred_type = MESH_ELEMENT_TRIANGLE;
    request.strategy = MESH_STRATEGY_QUALITY;
    request.target_quality = params->min_quality;
    request.global_size = params->target_edge_length;
    request.frequency = sim->solver_config.frequency;
    request.elements_per_wavelength = sim->solver_config.frequency * 0.03 / 299792458.0; // lambda/10
    request.enable_adaptivity = params->adaptive_refinement;
    request.num_threads = 4;  // Default thread count
    request.validate_quality = true;
    request.compute_statistics = true;
    
    // Generate mesh
    mesh_result_t* result = mesh_engine_generate(sim->mesh_engine, &request);
    if (!result || result->error_code != 0) {
        snprintf(sim->error_message, sizeof(sim->error_message), 
                "Mesh generation failed: %s", 
                result ? result->error_message : "Unknown error");
        if (result) mesh_result_destroy(result);
        return SATELLITE_ERROR_MESH_GENERATION;
    }
    
    // Store mesh
    sim->mesh = result->mesh;
    sim->mesh_generated = true;
    
    // Clean up result
    mesh_result_destroy(result);
    
    return SATELLITE_SUCCESS;
}

satellite_error_t satellite_get_mesh_info(satellite_simulation_t* sim, int* num_vertices, int* num_triangles) {
    if (!sim || !num_vertices || !num_triangles) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    if (!sim->mesh_generated || !sim->mesh) {
        return SATELLITE_ERROR_INVALID_GEOMETRY;
    }
    
    // Implementation depends on mesh structure
    // For now, return placeholder values
    *num_vertices = 1000;  // Would come from actual mesh
    *num_triangles = 2000;  // Would come from actual mesh
    
    return SATELLITE_SUCCESS;
}

/*********************************************************************
 * Solver Configuration
 *********************************************************************/

satellite_error_t satellite_configure_solver(satellite_simulation_t* sim, const satellite_solver_config_t* config) {
    if (!sim || !config) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    memcpy(&sim->solver_config, config, sizeof(satellite_solver_config_t));
    
    // Configure MoM solver
    if (strcmp(config->solver_type, "mom") == 0) {
        mom_config_t mom_config = {0};
        mom_config.basis_type = MOM_BASIS_RWG;
        mom_config.formulation = MOM_FORMULATION_EFIE;
        mom_config.frequency = config->frequency;
        mom_config.mesh_density = 10;  // elements per wavelength
        mom_config.edge_length = 0.03;  // 3cm at 10GHz
        mom_config.tolerance = config->tolerance;
        mom_config.max_iterations = config->max_iterations;
        mom_config.use_preconditioner = config->use_preconditioner;
        mom_config.use_parallel = true;
        mom_config.num_threads = 4;
        mom_config.compute_far_field = true;
        mom_config.compute_near_field = true;
        mom_config.compute_current_distribution = true;
        
        int result = mom_solver_configure(sim->mom_solver, &mom_config);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
    }
    // Configure PEEC solver
    else if (strcmp(config->solver_type, "peec") == 0) {
        peec_config_t peec_config = {0};
        peec_config.formulation = PEEC_FORMULATION_CLASSICAL;
        peec_config.frequency = config->frequency;
        peec_config.extract_resistance = true;
        peec_config.extract_inductance = true;
        peec_config.extract_capacitance = true;
        peec_config.extract_mutual_inductance = true;
        peec_config.extract_mutual_capacitance = true;
        peec_config.mesh_density = 10;
        peec_config.circuit_tolerance = config->tolerance;
        peec_config.circuit_max_iterations = config->max_iterations;
        peec_config.use_parallel = true;
        peec_config.num_threads = 4;
        peec_config.export_spice = true;
        
        int result = peec_solver_configure(sim->peec_solver, &peec_config);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
    }
    else {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    sim->solver_configured = true;
    return SATELLITE_SUCCESS;
}

satellite_error_t satellite_set_excitation(satellite_simulation_t* sim, const satellite_excitation_t* excitation) {
    if (!sim || !excitation) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    memcpy(&sim->excitation, excitation, sizeof(satellite_excitation_t));
    
    // Create MoM excitation
    mom_excitation_t mom_exc = {0};
    mom_exc.type = MOM_EXCITATION_PLANE_WAVE;
    mom_exc.frequency = excitation->frequency;
    mom_exc.amplitude = excitation->amplitude;
    mom_exc.phase = excitation->phase;
    
    // Convert angles to wave vector
    double k = 2.0 * M_PI * excitation->frequency / 299792458.0;  // Free space wavenumber
    double theta_rad = excitation->theta * M_PI / 180.0;
    double phi_rad = excitation->phi * M_PI / 180.0;
    
    mom_exc.k_vector.x = -k * sin(theta_rad) * cos(phi_rad);
    mom_exc.k_vector.y = -k * sin(theta_rad) * sin(phi_rad);
    mom_exc.k_vector.z = -k * cos(theta_rad);
    
    // Set polarization
    if (excitation->polarization == 0) {  // TE
        mom_exc.polarization.x = -sin(phi_rad);
        mom_exc.polarization.y = cos(phi_rad);
        mom_exc.polarization.z = 0.0;
    } else {  // TM
        mom_exc.polarization.x = cos(theta_rad) * cos(phi_rad);
        mom_exc.polarization.y = cos(theta_rad) * sin(phi_rad);
        mom_exc.polarization.z = -sin(theta_rad);
    }
    
    int result = mom_solver_add_excitation(sim->mom_solver, &mom_exc);
    if (result != 0) {
        return SATELLITE_ERROR_SOLVER_FAILED;
    }
    
    return SATELLITE_SUCCESS;
}

/*********************************************************************
 * Simulation Execution
 *********************************************************************/

satellite_error_t satellite_run_simulation(satellite_simulation_t* sim) {
    if (!sim) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    if (!sim->mesh_generated || !sim->solver_configured) {
        return SATELLITE_ERROR_INVALID_GEOMETRY;
    }
    
    clock_t start_time = clock();
    
    // Run appropriate solver
    if (strcmp(sim->solver_config.solver_type, "mom") == 0) {
        // Set mesh for MoM solver
        int result = mom_solver_set_mesh(sim->mom_solver, sim->mesh);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        // Assemble matrix
        result = mom_solver_assemble_matrix(sim->mom_solver);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        // Solve
        result = mom_solver_solve(sim->mom_solver);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
    }
    else if (strcmp(sim->solver_config.solver_type, "peec") == 0) {
        // Set mesh for PEEC solver
        int result = peec_solver_set_mesh(sim->peec_solver, sim->mesh);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        // Extract partial elements
        result = peec_solver_extract_partial_elements(sim->peec_solver);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        // Build circuit network
        result = peec_solver_build_circuit_network(sim->peec_solver);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        // Solve circuit
        result = peec_solver_solve_circuit(sim->peec_solver);
        if (result != 0) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
    }
    
    clock_t end_time = clock();
    sim->simulation_completed = true;
    
    // Create performance metrics
    sim->performance = (satellite_performance_t*)calloc(1, sizeof(satellite_performance_t));
    if (sim->performance) {
        sim->performance->total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        sim->performance->converged = true;  // Would come from actual solver
        sim->performance->num_unknowns = 1000;  // Would come from actual solver
    }
    
    return SATELLITE_SUCCESS;
}

/*********************************************************************
 * Results Retrieval
 *********************************************************************/

satellite_error_t satellite_get_currents(satellite_simulation_t* sim, satellite_currents_t** currents) {
    if (!sim || !currents) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    if (!sim->simulation_completed) {
        return SATELLITE_ERROR_SOLVER_FAILED;
    }
    
    // Get results from appropriate solver
    if (strcmp(sim->solver_config.solver_type, "mom") == 0) {
        const mom_result_t* mom_result = mom_solver_get_results(sim->mom_solver);
        if (!mom_result) {
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        // Create currents structure
        *currents = (satellite_currents_t*)calloc(1, sizeof(satellite_currents_t));
        if (!*currents) {
            return SATELLITE_ERROR_MEMORY_ALLOCATION;
        }
        
        (*currents)->num_basis = mom_result->num_basis_functions;
        (*currents)->currents = (complex_t*)malloc((*currents)->num_basis * sizeof(complex_t));
        (*currents)->magnitude = (double*)malloc((*currents)->num_basis * sizeof(double));
        (*currents)->phase = (double*)malloc((*currents)->num_basis * sizeof(double));
        
        if (!(*currents)->currents || !(*currents)->magnitude || !(*currents)->phase) {
            satellite_free_currents(*currents);
            *currents = NULL;
            return SATELLITE_ERROR_MEMORY_ALLOCATION;
        }
        
        // Copy current data
        for (int i = 0; i < (*currents)->num_basis; i++) {
            (*currents)->currents[i].re = mom_result->current_coefficients[i].re;
            (*currents)->currents[i].im = mom_result->current_coefficients[i].im;
            (*currents)->magnitude[i] = complex_magnitude(&mom_result->current_coefficients[i]);
            (*currents)->phase[i] = atan2(mom_result->current_coefficients[i].im, mom_result->current_coefficients[i].re);
        }
    }
    else {
        // PEEC current extraction would go here
        return SATELLITE_ERROR_SOLVER_FAILED;
    }
    
    return SATELLITE_SUCCESS;
}

satellite_error_t satellite_get_fields(satellite_simulation_t* sim, const satellite_observation_points_t* points, satellite_fields_t** fields) {
    if (!sim || !points || !fields) {
        return SATELLITE_ERROR_INVALID_INPUT;
    }
    
    if (!sim->simulation_completed) {
        return SATELLITE_ERROR_SOLVER_FAILED;
    }
    
    // Convert observation points to solver format (point3d_t)
    point3d_t* solver_points = (point3d_t*)malloc(points->num_points * sizeof(point3d_t));
    if (!solver_points) {
        return SATELLITE_ERROR_MEMORY_ALLOCATION;
    }
    
    for (int i = 0; i < points->num_points; i++) {
        solver_points[i].x = points->x[i];
        solver_points[i].y = points->y[i];
        solver_points[i].z = points->z[i];
    }
    
    // Compute fields using appropriate solver
    if (strcmp(sim->solver_config.solver_type, "mom") == 0) {
        int result = mom_solver_compute_near_field(sim->mom_solver, solver_points, points->num_points);
        if (result != 0) {
            free(solver_points);
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        const mom_result_t* mom_result = mom_solver_get_results(sim->mom_solver);
        if (!mom_result || !mom_result->near_field.e_field) {
            free(solver_points);
            return SATELLITE_ERROR_SOLVER_FAILED;
        }
        
        // Create fields structure
        *fields = (satellite_fields_t*)calloc(1, sizeof(satellite_fields_t));
        if (!*fields) {
            free(solver_points);
            return SATELLITE_ERROR_MEMORY_ALLOCATION;
        }
        
        (*fields)->num_points = points->num_points;
        (*fields)->e_field = (complex_t*)malloc((*fields)->num_points * 3 * sizeof(complex_t));
        (*fields)->h_field = (complex_t*)malloc((*fields)->num_points * 3 * sizeof(complex_t));
        (*fields)->e_magnitude = (double*)malloc((*fields)->num_points * sizeof(double));
        (*fields)->h_magnitude = (double*)malloc((*fields)->num_points * sizeof(double));
        
        if (!(*fields)->e_field || !(*fields)->h_field || !(*fields)->e_magnitude || !(*fields)->h_magnitude) {
            satellite_free_fields(*fields);
            *fields = NULL;
            free(solver_points);
            return SATELLITE_ERROR_MEMORY_ALLOCATION;
        }
        
        // Copy field data
        for (int i = 0; i < (*fields)->num_points; i++) {
            // E-field (x, y, z components)
            (*fields)->e_field[i*3 + 0] = mom_result->near_field.e_field[i*3 + 0];
            (*fields)->e_field[i*3 + 1] = mom_result->near_field.e_field[i*3 + 1];
            (*fields)->e_field[i*3 + 2] = mom_result->near_field.e_field[i*3 + 2];
            
            // H-field (x, y, z components)
            (*fields)->h_field[i*3 + 0] = mom_result->near_field.h_field[i*3 + 0];
            (*fields)->h_field[i*3 + 1] = mom_result->near_field.h_field[i*3 + 1];
            (*fields)->h_field[i*3 + 2] = mom_result->near_field.h_field[i*3 + 2];
            
            // Magnitudes
            double ex_re = (*fields)->e_field[i*3 + 0].re, ex_im = (*fields)->e_field[i*3 + 0].im;
            double ey_re = (*fields)->e_field[i*3 + 1].re, ey_im = (*fields)->e_field[i*3 + 1].im;
            double ez_re = (*fields)->e_field[i*3 + 2].re, ez_im = (*fields)->e_field[i*3 + 2].im;
            (*fields)->e_magnitude[i] = sqrt(
                ex_re * ex_re + ex_im * ex_im +
                ey_re * ey_re + ey_im * ey_im +
                ez_re * ez_re + ez_im * ez_im
            );
            
            double hx_re = (*fields)->h_field[i*3 + 0].re, hx_im = (*fields)->h_field[i*3 + 0].im;
            double hy_re = (*fields)->h_field[i*3 + 1].re, hy_im = (*fields)->h_field[i*3 + 1].im;
            double hz_re = (*fields)->h_field[i*3 + 2].re, hz_im = (*fields)->h_field[i*3 + 2].im;
            (*fields)->h_magnitude[i] = sqrt(
                hx_re * hx_re + hx_im * hx_im +
                hy_re * hy_re + hy_im * hy_im +
                hz_re * hz_re + hz_im * hz_im
            );
        }
    }
    else {
        // PEEC field computation would go here
        free(solver_points);
        return SATELLITE_ERROR_SOLVER_FAILED;
    }
    
    free(solver_points);
    return SATELLITE_SUCCESS;
}

/*********************************************************************
 * Memory Management
 *********************************************************************/

void satellite_free_currents(satellite_currents_t* currents) {
    if (!currents) return;
    
    if (currents->currents) free(currents->currents);
    if (currents->magnitude) free(currents->magnitude);
    if (currents->phase) free(currents->phase);
    free(currents);
}

void satellite_free_fields(satellite_fields_t* fields) {
    if (!fields) return;
    
    if (fields->e_field) free(fields->e_field);
    if (fields->h_field) free(fields->h_field);
    if (fields->e_magnitude) free(fields->e_magnitude);
    if (fields->h_magnitude) free(fields->h_magnitude);
    free(fields);
}

void satellite_free_rcs(satellite_rcs_t* rcs) {
    if (!rcs) return;
    
    if (rcs->theta_angles) free(rcs->theta_angles);
    if (rcs->phi_angles) free(rcs->phi_angles);
    if (rcs->rcs_values) free(rcs->rcs_values);
    free(rcs);
}

void satellite_free_performance(satellite_performance_t* performance) {
    if (performance) {
        free(performance);
    }
}

void satellite_free_rwg_basis(satellite_rwg_basis_t* basis) {
    if (!basis) return;
    
    if (basis->triangle_plus) free(basis->triangle_plus);
    if (basis->triangle_minus) free(basis->triangle_minus);
    if (basis->edge_indices) free(basis->edge_indices);
    if (basis->edge_lengths) free(basis->edge_lengths);
    if (basis->areas) free(basis->areas);
    free(basis);
}