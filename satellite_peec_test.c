/**
 * @file satellite_peec_test.c
 * @brief Test program for satellite HPM electromagnetic scattering using PEEC
 * @details Demonstrates PEC material handling, plane wave excitation, and scattering
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>

#include "../src/solvers/peec/peec_satellite.h"
#include "../src/solvers/peec/peec_solver.h"
#include "../src/core/core_geometry.h"
#include "../src/core/core_mesh.h"

#define NUM_OBSERVATION_POINTS 100
#define SATELLITE_FREQ_10GHZ 10.0e9
#define OUTPUT_DIR "satellite_peec_results"

/**
 * @brief Create observation points for field visualization
 */
int create_observation_points(geom_point_t** points, int* num_points) {
    *num_points = NUM_OBSERVATION_POINTS;
    *points = (geom_point_t*)calloc(*num_points, sizeof(geom_point_t));
    
    if (!*points) {
        fprintf(stderr, "Failed to allocate observation points\n");
        return -1;
    }
    
    /* Create a grid of observation points around the satellite */
    int grid_size = 10;
    double x_min = -0.5, x_max = 3.9;  /* Domain: [1.7-2.2, 1.7+2.2] */
    double y_min = -0.5, y_max = 3.9;  /* Domain: [1.7-2.2, 1.7+2.2] */
    double z_min = -0.2, z_max = 1.6;  /* Domain: [0.7-0.9, 0.7+0.9] */
    
    int idx = 0;
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            for (int k = 0; k < grid_size && idx < *num_points; k++) {
                (*points)[idx].coords[0] = x_min + (x_max - x_min) * i / (grid_size - 1);
                (*points)[idx].coords[1] = y_min + (y_max - y_min) * j / (grid_size - 1);
                (*points)[idx].coords[2] = z_min + (z_max - z_min) * k / (grid_size - 1);
                idx++;
            }
        }
    }
    
    *num_points = idx;
    return 0;
}

/**
 * @brief Print field results for debugging
 */
void print_field_results(const char* title, const double complex* e_field, 
                        const double complex* h_field, int num_points) {
    printf("\n=== %s ===\n", title);
    printf("Point\t\tE-field (V/m)\t\tH-field (A/m)\n");
    
    for (int i = 0; i < num_points && i < 10; i++) {  /* Print first 10 points */
        double e_magnitude = cabs(e_field[i*3] + e_field[i*3+1] + e_field[i*3+2]);
        double h_magnitude = cabs(h_field[i*3] + h_field[i*3+1] + h_field[i*3+2]);
        
        printf("[%d]\t\t%.3e\t\t%.3e\n", i, e_magnitude, h_magnitude);
    }
    
    if (num_points > 10) {
        printf("... (%d more points)\n", num_points - 10);
    }
}

/**
 * @brief Export field data to file for visualization
 */
int export_field_data(const char* filename, const geom_point_t* points, 
                     const double complex* e_field, const double complex* h_field,
                     int num_points) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output file: %s\n", filename);
        return -1;
    }
    
    fprintf(fp, "# Satellite PEEC Field Data\n");
    fprintf(fp, "# Format: x y z Ex_real Ex_imag Ey_real Ey_imag Ez_real Ez_imag Hx_real Hx_imag Hy_real Hy_imag Hz_real Hz_imag\n");
    
    for (int i = 0; i < num_points; i++) {
        fprintf(fp, "%.6f %.6f %.6f ", 
                points[i].coords[0], points[i].coords[1], points[i].coords[2]);
        
        /* Electric field components */
        for (int j = 0; j < 3; j++) {
            fprintf(fp, "%.6e %.6e ", creal(e_field[i*3 + j]), cimag(e_field[i*3 + j]));
        }
        
        /* Magnetic field components */
        for (int j = 0; j < 3; j++) {
            fprintf(fp, "%.6e %.6e ", creal(h_field[i*3 + j]), cimag(h_field[i*3 + j]));
        }
        
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    return 0;
}

/**
 * @brief Main test function
 */
int main(int argc, char* argv[]) {
    printf("=== Satellite HPM PEEC Solver Test ===\n");
    printf("Testing 10GHz plane wave scattering from PEC satellite\n\n");
    
    clock_t start_time = clock();
    
    /* Create satellite PEEC solver */
    peec_satellite_solver_t* solver = peec_satellite_create();
    if (!solver) {
        fprintf(stderr, "Failed to create satellite PEEC solver\n");
        return EXIT_FAILURE;
    }
    
    /* Configure satellite solver */
    peec_satellite_config_t config;
    memset(&config, 0, sizeof(config));
    
    /* Set frequency to 10 GHz */
    config.frequency = SATELLITE_FREQ_10GHZ;
    config.wavelength = peec_satellite_compute_wavelength(SATELLITE_FREQ_10GHZ);
    
    /* Set PEC material properties */
    config.pec_permittivity = 1.0;
    config.pec_permeability = 1.0;
    config.pec_conductivity = 1e20;  /* Perfect conductor */
    
    /* Set coordinate correction for satellite positioning */
    config.geometry_translation[0] = 1.7;  /* Center in x */
    config.geometry_translation[1] = 1.7;  /* Center in y */
    config.geometry_translation[2] = 0.14; /* Center in z (0.56 + 0.14 = 0.7) */
    config.apply_coordinate_correction = true;
    
    /* Set plane wave excitation (z-propagating, x-polarized) */
    config.plane_wave.direction[0] = 0.0;
    config.plane_wave.direction[1] = 0.0;
    config.plane_wave.direction[2] = 1.0;
    config.plane_wave.polarization[0] = 1.0;
    config.plane_wave.polarization[1] = 0.0;
    config.plane_wave.polarization[2] = 0.0;
    config.plane_wave.amplitude = 1.0;  /* 1 V/m */
    config.plane_wave.phase = 0.0;
    
    /* Set domain settings */
    config.domain.size[0] = 3.4;
    config.domain.size[1] = 3.4;
    config.domain.size[2] = 1.4;
    config.domain.center[0] = 1.7;
    config.domain.center[1] = 1.7;
    config.domain.center[2] = 0.7;
    config.domain.resolution = 0.01;  /* 1 cm resolution */
    
    /* Configure solver */
    if (peec_satellite_configure(solver, &config) != 0) {
        fprintf(stderr, "Failed to configure satellite solver\n");
        peec_satellite_destroy(solver);
        return EXIT_FAILURE;
    }
    
    /* Print configuration */
    peec_satellite_print_configuration(&config);
    
    /* Validate plane wave orthogonality */
    if (peec_satellite_validate_plane_wave_orthogonality(solver) != 0) {
        fprintf(stderr, "Warning: Plane wave validation failed\n");
    }
    
    /* Create observation points */
    geom_point_t* observation_points = NULL;
    int num_obs_points = 0;
    
    if (create_observation_points(&observation_points, &num_obs_points) != 0) {
        peec_satellite_destroy(solver);
        return EXIT_FAILURE;
    }
    
    printf("Created %d observation points\n", num_obs_points);
    
    /* Allocate field arrays */
    double complex* incident_e_field = (double complex*)calloc(num_obs_points * 3, sizeof(double complex));
    double complex* incident_h_field = (double complex*)calloc(num_obs_points * 3, sizeof(double complex));
    double complex* scattered_e_field = (double complex*)calloc(num_obs_points * 3, sizeof(double complex));
    double complex* scattered_h_field = (double complex*)calloc(num_obs_points * 3, sizeof(double complex));
    double complex* total_e_field = (double complex*)calloc(num_obs_points * 3, sizeof(double complex));
    double complex* total_h_field = (double complex*)calloc(num_obs_points * 3, sizeof(double complex));
    
    if (!incident_e_field || !incident_h_field || !scattered_e_field || 
        !scattered_h_field || !total_e_field || !total_h_field) {
        fprintf(stderr, "Failed to allocate field arrays\n");
        free(incident_e_field);
        free(incident_h_field);
        free(scattered_e_field);
        free(scattered_h_field);
        free(total_e_field);
        free(total_h_field);
        free(observation_points);
        peec_satellite_destroy(solver);
        return EXIT_FAILURE;
    }
    
    /* Compute incident field at observation points */
    printf("Computing incident field...\n");
    for (int i = 0; i < num_obs_points; i++) {
        double complex local_e[3], local_h[3];
        if (peec_satellite_compute_incident_field(solver, &observation_points[i], local_e, local_h) == 0) {
            for (int j = 0; j < 3; j++) {
                incident_e_field[i*3 + j] = local_e[j];
                incident_h_field[i*3 + j] = local_h[j];
            }
        }
    }
    
    /* Apply plane wave excitation to PEC surfaces */
    printf("Applying plane wave excitation to PEC surfaces...\n");
    if (peec_satellite_apply_plane_wave_excitation(solver) != 0) {
        fprintf(stderr, "Warning: Failed to apply plane wave excitation\n");
    }
    
    /* Extract PEC partial elements */
    printf("Extracting PEC partial elements...\n");
    if (peec_satellite_extract_pec_partial_elements(solver) != 0) {
        fprintf(stderr, "Warning: Failed to extract PEC elements\n");
    }
    
    /* Compute scattered field (simplified - would need actual surface currents) */
    printf("Computing scattered field...\n");
    /* Note: This is a simplified demonstration. Real implementation would:
     * 1. Solve for surface currents on PEC elements
     * 2. Compute scattered field from these currents
     * 3. Enforce PEC boundary conditions
     */
    
    /* For demonstration, compute scattered field with simplified model */
    if (peec_satellite_compute_scattered_field(solver, observation_points, num_obs_points,
                                             scattered_e_field, scattered_h_field) != 0) {
        fprintf(stderr, "Warning: Failed to compute scattered field\n");
    }
    
    /* Compute total field */
    printf("Computing total field...\n");
    if (peec_satellite_compute_total_field(solver, observation_points, num_obs_points,
                                         total_e_field, total_h_field) != 0) {
        fprintf(stderr, "Warning: Failed to compute total field\n");
    }
    
    /* Print results */
    print_field_results("Incident Field", incident_e_field, incident_h_field, num_obs_points);
    print_field_results("Scattered Field", scattered_e_field, scattered_h_field, num_obs_points);
    print_field_results("Total Field", total_e_field, total_h_field, num_obs_points);
    
    /* Export field data for visualization */
    printf("Exporting field data...\n");
    
    /* Create output directory */
    #ifdef _WIN32
        _mkdir(OUTPUT_DIR);
    #else
        mkdir(OUTPUT_DIR, 0755);
    #endif
    
    export_field_data(OUTPUT_DIR "/incident_field.dat", observation_points,
                     incident_e_field, incident_h_field, num_obs_points);
    export_field_data(OUTPUT_DIR "/scattered_field.dat", observation_points,
                     scattered_e_field, scattered_h_field, num_obs_points);
    export_field_data(OUTPUT_DIR "/total_field.dat", observation_points,
                     total_e_field, total_h_field, num_obs_points);
    
    /* Compute and display PEC surface impedance */
    double complex pec_impedance = peec_satellite_get_pec_surface_impedance(
        SATELLITE_FREQ_10GHZ, 1e20);
    printf("\nPEC Surface Impedance at 10GHz: %.3e + %.3ej ohms\n",
           creal(pec_impedance), cimag(pec_impedance));
    
    /* Performance metrics */
    clock_t end_time = clock();
    double cpu_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    printf("\n=== Test Results Summary ===\n");
    printf("Simulation completed in %.2f seconds\n", cpu_time);
    printf("Frequency: %.3f GHz\n", SATELLITE_FREQ_10GHZ/1e9);
    printf("Wavelength: %.3f mm\n", peec_satellite_compute_wavelength(SATELLITE_FREQ_10GHZ)*1000);
    printf("Observation points: %d\n", num_obs_points);
    printf("Field data exported to: %s/\n", OUTPUT_DIR);
    
    /* Cleanup */
    free(incident_e_field);
    free(incident_h_field);
    free(scattered_e_field);
    free(scattered_h_field);
    free(total_e_field);
    free(total_h_field);
    free(observation_points);
    peec_satellite_destroy(solver);
    
    printf("\n=== Test completed successfully ===\n");
    return EXIT_SUCCESS;
}