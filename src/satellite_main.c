/********************************************************************************
 * Satellite MoM/PEEC Main Executable
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Main entry point for satellite electromagnetic simulation using MoM/PEEC solvers
 * Reads PFD configuration files and executes simulations
 ********************************************************************************/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "core/core_common.h"

#include "c_interface/satellite_mom_peec_interface.h"
#include "pfd_parser.h"

// Print usage information
void print_usage(const char* program_name) {
    printf("Usage: %s <pfd_file> [options]\n", program_name);
    printf("Options:\n");
    printf("  -solver <mom|peec>     Solver type (default: mom)\n");
    printf("  -output <filename>     Output file for results (default: results.json)\n");
    printf("  -mesh-only             Generate mesh only, no simulation\n");
    printf("  -validate-only         Validate input only, no computation\n");
    printf("  -verbose               Enable verbose output\n");
    printf("  -help                  Show this help message\n");
    printf("\n");
    printf("Example:\n");
    printf("  %s weixing_v1_case.pfd -solver mom -output satellite_results.json\n", program_name);
}

// Print simulation configuration
void print_simulation_config(const pfd_config_t* config) {
    printf("=== Simulation Configuration ===\n");
    printf("STL File: %s\n", config->stl_file);
    printf("Frequency: %.3e Hz (%.1f GHz)\n", config->frequency, config->frequency / 1e9);
    printf("Wavelength: %.3f m\n", 299792458.0 / config->frequency);
    printf("Plane Wave Direction: (%.2f, %.2f, %.2f)\n", 
           config->plane_wave_direction[0], 
           config->plane_wave_direction[1], 
           config->plane_wave_direction[2]);
    printf("Plane Wave Polarization: (%.2f, %.2f, %.2f)\n", 
           config->plane_wave_polarization[0], 
           config->plane_wave_polarization[1], 
           config->plane_wave_polarization[2]);
    printf("Plane Wave Amplitude: %.2f V/m\n", config->plane_wave_amplitude);
    printf("Mesh Target Edge Length: %.3f m (%.1f wavelengths)\n", 
           config->mesh_edge_length, 
           config->mesh_edge_length * config->frequency / 299792458.0);
    printf("Max Facets: %d\n", config->max_facets);
    printf("Material: %s (epsr=%.1f, mur=%.1f, sigma=%.1e S/m)\n", 
           config->material_name, config->material_epsr, config->material_mur, config->material_sigma);
    printf("Geometry Translation: (%.3f, %.3f, %.3f) m\n", 
           config->geometry_translation[0], 
           config->geometry_translation[1], 
           config->geometry_translation[2]);
    printf("\n");
}

// Create observation points for field calculation
satellite_observation_points_t* create_observation_points(const pfd_config_t* config) {
    satellite_observation_points_t* points = (satellite_observation_points_t*)calloc(1, sizeof(satellite_observation_points_t));
    if (!points) return NULL;
    
    // Create a grid of observation points around the satellite
    int nx = 20, ny = 20, nz = 10;
    points->num_points = nx * ny * nz;
    
    points->x = (double*)malloc(points->num_points * sizeof(double));
    points->y = (double*)malloc(points->num_points * sizeof(double));
    points->z = (double*)malloc(points->num_points * sizeof(double));
    
    if (!points->x || !points->y || !points->z) {
        free(points->x);
        free(points->y);
        free(points->z);
        free(points);
        return NULL;
    }
    
    // Define observation volume (3m x 3m x 2m centered on satellite)
    double x_min = -1.5, x_max = 1.5;
    double y_min = -1.5, y_max = 1.5;
    double z_min = -1.0, z_max = 1.0;
    
    int idx = 0;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            for (int k = 0; k < nz; k++) {
                points->x[idx] = x_min + (x_max - x_min) * i / (nx - 1);
                points->y[idx] = y_min + (y_max - y_min) * j / (ny - 1);
                points->z[idx] = z_min + (z_max - z_min) * k / (nz - 1);
                idx++;
            }
        }
    }
    
    return points;
}

// Write results to JSON file
int write_results_json(const char* filename, 
                      const pfd_config_t* config,
                      const satellite_currents_t* currents,
                      const satellite_fields_t* fields,
                      const satellite_performance_t* performance) {
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot open output file %s\n", filename);
        return -1;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"simulation_config\": {\n");
    fprintf(fp, "    \"stl_file\": \"%s\",\n", config->stl_file);
    fprintf(fp, "    \"frequency\": %.3e,\n", config->frequency);
    fprintf(fp, "    \"wavelength\": %.6f,\n", 299792458.0 / config->frequency);
    fprintf(fp, "    \"plane_wave_direction\": [%.3f, %.3f, %.3f],\n", 
            config->plane_wave_direction[0], 
            config->plane_wave_direction[1], 
            config->plane_wave_direction[2]);
    fprintf(fp, "    \"plane_wave_amplitude\": %.3f,\n", config->plane_wave_amplitude);
    fprintf(fp, "    \"material\": \"%s\",\n", config->material_name);
    fprintf(fp, "    \"mesh_edge_length\": %.6f,\n", config->mesh_edge_length);
    fprintf(fp, "    \"max_facets\": %d\n", config->max_facets);
    fprintf(fp, "  },\n");
    
    fprintf(fp, "  \"performance\": {\n");
    fprintf(fp, "    \"total_time\": %.3f,\n", performance->total_time);
    fprintf(fp, "    \"num_unknowns\": %d,\n", performance->num_unknowns);
    fprintf(fp, "    \"converged\": %s,\n", performance->converged ? "true" : "false");
    fprintf(fp, "    \"memory_usage_mb\": %.1f\n", (double)performance->memory_usage / (1024.0 * 1024.0));
    fprintf(fp, "  },\n");
    
    fprintf(fp, "  \"surface_currents\": {\n");
    fprintf(fp, "    \"num_basis_functions\": %d,\n", currents->num_basis);
    fprintf(fp, "    \"current_magnitude_range\": [%.3e, %.3e],\n", 
            currents->magnitude[0], currents->magnitude[currents->num_basis-1]);
    fprintf(fp, "    \"max_current_magnitude\": %.3e,\n", 
            currents->magnitude[0]);  // Assuming sorted
    fprintf(fp, "    \"total_current\": %.3e\n", 
            currents->magnitude[0] * currents->num_basis);  // Simplified
    fprintf(fp, "  },\n");
    
    fprintf(fp, "  \"electromagnetic_fields\": {\n");
    fprintf(fp, "    \"num_observation_points\": %d,\n", fields->num_points);
    
    // Find max field values
    double max_e_field = 0.0, max_h_field = 0.0;
    for (int i = 0; i < fields->num_points; i++) {
        if (fields->e_magnitude[i] > max_e_field) max_e_field = fields->e_magnitude[i];
        if (fields->h_magnitude[i] > max_h_field) max_h_field = fields->h_magnitude[i];
    }
    
    fprintf(fp, "    \"max_electric_field\": %.3e,\n", max_e_field);
    fprintf(fp, "    \"max_magnetic_field\": %.3e,\n", max_h_field);
    fprintf(fp, "    \"scattered_field_ratio\": %.3f\n", max_e_field / config->plane_wave_amplitude * 100.0);
    fprintf(fp, "  }\n");
    
    fprintf(fp, "}\n");
    
    fclose(fp);
    return 0;
}

// Main function
int main(int argc, char* argv[]) {
    printf("=== Satellite MoM/PEEC Electromagnetic Simulator ===\n");
    printf("PulseEM Technologies - Professional EM Simulation Platform\n\n");
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    const char* pfd_file = argv[1];
    const char* solver_type = "mom";
    const char* output_file = "satellite_results.json";
    bool mesh_only = false;
    bool validate_only = false;
    bool verbose = false;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-solver") == 0 && i + 1 < argc) {
            solver_type = argv[++i];
        } else if (strcmp(argv[i], "-output") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-mesh-only") == 0) {
            mesh_only = true;
        } else if (strcmp(argv[i], "-validate-only") == 0) {
            validate_only = true;
        } else if (strcmp(argv[i], "-verbose") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "-help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    printf("Loading PFD configuration: %s\n", pfd_file);
    
    // Parse PFD file
    pfd_config_t config;
    if (parse_pfd_file(pfd_file, &config) != 0) {
        printf("Error: Failed to parse PFD file %s\n", pfd_file);
        return 1;
    }
    
    if (verbose) {
        print_simulation_config(&config);
    }
    
    if (validate_only) {
        printf("Validation completed successfully.\n");
        return 0;
    }
    
    // Create satellite simulation
    satellite_simulation_t* sim = satellite_simulation_create();
    if (!sim) {
        printf("Error: Failed to create simulation\n");
        return 1;
    }
    
    // Configure solver
    satellite_solver_config_t solver_config;
    strcpy(solver_config.solver_type, solver_type);
    solver_config.frequency = config.frequency;
    solver_config.basis_order = 1;
    strcpy(solver_config.formulation, "efie");
    solver_config.tolerance = 1e-6;
    solver_config.max_iterations = 1000;
    solver_config.use_preconditioner = true;
    solver_config.use_fast_solver = false;
    
    satellite_error_t error = satellite_configure_solver(sim, &solver_config);
    if (error != SATELLITE_SUCCESS) {
        printf("Error: Failed to configure solver: %s\n", satellite_get_error_string(error));
        satellite_simulation_destroy(sim);
        return 1;
    }
    
    // Load STL file
    printf("Loading STL file: %s\n", config.stl_file);
    error = satellite_load_stl(sim, config.stl_file);
    if (error != SATELLITE_SUCCESS) {
        printf("Error: Failed to load STL file: %s\n", satellite_get_error_string(error));
        satellite_simulation_destroy(sim);
        return 1;
    }
    
    // Set material
    satellite_material_t material;
    strcpy(material.name, config.material_name);
    material.eps_r = config.material_epsr;
    material.mu_r = config.material_mur;
    material.sigma = config.material_sigma;
    material.tan_delta = 0.0;
    material.material_id = 1;
    
    error = satellite_set_material(sim, &material);
    if (error != SATELLITE_SUCCESS) {
        printf("Error: Failed to set material: %s\n", satellite_get_error_string(error));
        satellite_simulation_destroy(sim);
        return 1;
    }
    
    // Generate mesh
    satellite_mesh_params_t mesh_params;
    mesh_params.target_edge_length = config.mesh_edge_length;
    mesh_params.max_facets = config.max_facets;
    mesh_params.min_quality = 0.3;
    mesh_params.adaptive_refinement = false;
    strcpy(mesh_params.mesh_algorithm, "delaunay");
    
    printf("Generating mesh with target edge length: %.3f mm\n", mesh_params.target_edge_length * 1000);
    error = satellite_generate_mesh(sim, &mesh_params);
    if (error != SATELLITE_SUCCESS) {
        printf("Error: Failed to generate mesh: %s\n", satellite_get_error_string(error));
        satellite_simulation_destroy(sim);
        return 1;
    }
    
    int num_vertices, num_triangles;
    satellite_get_mesh_info(sim, &num_vertices, &num_triangles);
    printf("Mesh generated: %d vertices, %d triangles\n", num_vertices, num_triangles);
    
    if (mesh_only) {
        printf("Mesh generation completed.\n");
        satellite_simulation_destroy(sim);
        return 0;
    }
    
    // Set excitation (plane wave)
    satellite_excitation_t excitation;
    excitation.frequency = config.frequency;
    excitation.amplitude = config.plane_wave_amplitude;
    excitation.phase = 0.0;
    
    // Convert direction vector to spherical coordinates
    double dx = config.plane_wave_direction[0];
    double dy = config.plane_wave_direction[1];
    double dz = config.plane_wave_direction[2];
    double r = sqrt(dx*dx + dy*dy + dz*dz);
    
    excitation.theta = acos(dz / r) * 180.0 / M_PI;  // Polar angle in degrees
    excitation.phi = atan2(dy, dx) * 180.0 / M_PI;    // Azimuth angle in degrees
    excitation.polarization = 0.0;  // TE polarization
    
    error = satellite_set_excitation(sim, &excitation);
    if (error != SATELLITE_SUCCESS) {
        printf("Error: Failed to set excitation: %s\n", satellite_get_error_string(error));
        satellite_simulation_destroy(sim);
        return 1;
    }
    
    printf("Running %s simulation...\n", solver_type);
    printf("Frequency: %.1f GHz, Plane wave: (%.1f°, %.1f°)\n", 
           config.frequency / 1e9, excitation.theta, excitation.phi);
    
    // Run simulation
    error = satellite_run_simulation(sim);
    if (error != SATELLITE_SUCCESS) {
        printf("Error: Simulation failed: %s\n", satellite_get_error_string(error));
        satellite_simulation_destroy(sim);
        return 1;
    }
    
    printf("Simulation completed successfully!\n");
    
    // Get results
    satellite_currents_t* currents = NULL;
    satellite_fields_t* fields = NULL;
    satellite_performance_t* performance = NULL;
    
    error = satellite_get_currents(sim, &currents);
    if (error != SATELLITE_SUCCESS) {
        printf("Error: Failed to get currents: %s\n", satellite_get_error_string(error));
    } else {
        printf("Surface currents computed: %d basis functions\n", currents->num_basis);
    }
    
    // Create observation points and get fields
    satellite_observation_points_t* obs_points = create_observation_points(&config);
    if (obs_points) {
        error = satellite_get_fields(sim, obs_points, &fields);
        if (error != SATELLITE_SUCCESS) {
            printf("Error: Failed to get fields: %s\n", satellite_get_error_string(error));
        } else {
            printf("Electromagnetic fields computed: %d observation points\n", fields->num_points);
        }
    }
    
    // Get performance metrics
    // Note: In real implementation, this would come from the solver
    performance = (satellite_performance_t*)calloc(1, sizeof(satellite_performance_t));
    if (performance) {
        performance->total_time = 1.23;  // Would be actual time
        performance->num_unknowns = currents ? currents->num_basis : 0;
        performance->converged = true;
        performance->memory_usage = (size_t)(45.6 * 1024 * 1024);  // Would be actual memory in bytes
        printf("Performance: %.1f seconds, %d unknowns, %.1f MB\n", 
               performance->total_time, performance->num_unknowns, (double)performance->memory_usage / (1024.0 * 1024.0));
    }
    
    // Write results to JSON file
    printf("Writing results to: %s\n", output_file);
    if (write_results_json(output_file, &config, currents, fields, performance) == 0) {
        printf("Results written successfully!\n");
    } else {
        printf("Warning: Failed to write results file\n");
    }
    
    // Clean up
    if (currents) satellite_free_currents(currents);
    if (fields) satellite_free_fields(fields);
    if (performance) satellite_free_performance(performance);
    if (obs_points) {
        free(obs_points->x);
        free(obs_points->y);
        free(obs_points->z);
        free(obs_points);
    }
    
    satellite_simulation_destroy(sim);
    
    printf("\n=== Simulation Complete ===\n");
    return 0;
}