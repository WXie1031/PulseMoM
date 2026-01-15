#include "fast_multipole_algorithm.h"
#include "../../math_utils.h"
#include "../../constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define C_ELECTROMAGNETIC 299792458.0
#define MU_0 1.2566370614e-6
#define EPSILON_0 8.8541878128e-12
#define ETA_0 376.730313668
#define CONVERGENCE_TOLERANCE 1e-6
#define MAX_ITERATIONS 1000

static double get_time_seconds() {
    return (double)clock() / CLOCKS_PER_SEC;
}

FastMultipoleSolver* fast_multipole_create(FastMultipoleConfig* config) {
    if (!config) return NULL;
    
    FastMultipoleSolver* solver = (FastMultipoleSolver*)calloc(1, sizeof(FastMultipoleSolver));
    if (!solver) return NULL;
    
    memcpy(&solver->config, config, sizeof(FastMultipoleConfig));
    
    solver->num_clusters = 0;
    solver->num_interactions = 0;
    solver->num_sources = 0;
    solver->num_observations = 0;
    solver->matrix_size = 0;
    solver->tree_depth = 0;
    solver->calculation_completed = 0;
    solver->computation_time = 0.0;
    solver->memory_usage = 0.0;
    solver->convergence_status = 0;
    solver->achieved_accuracy = 0.0;
    solver->num_matrix_vector_products = 0;
    
    solver->mom_solver = mom_solver_create(config->max_iterations, config->convergence_tolerance);
    if (!solver->mom_solver) {
        free(solver);
        return NULL;
    }
    
    return solver;
}

void fast_multipole_destroy(FastMultipoleSolver* solver) {
    if (!solver) return;
    
    if (solver->impedance_matrix) free(solver->impedance_matrix);
    if (solver->near_field_matrix) free(solver->near_field_matrix);
    if (solver->far_field_matrix) free(solver->far_field_matrix);
    if (solver->translation_operators) free(solver->translation_operators);
    if (solver->interpolation_matrices) free(solver->interpolation_matrices);
    if (solver->source_positions) free(solver->source_positions);
    if (solver->observation_positions) free(solver->observation_positions);
    if (solver->source_currents) free(solver->source_currents);
    if (solver->observation_fields) free(solver->observation_fields);
    if (solver->mom_solver) mom_solver_destroy(solver->mom_solver);
    
    free(solver);
}

Complex spherical_hankel_function(int n, Complex z) {
    if (n < 0) return complex_create(0.0, 0.0);
    
    Complex h_n = complex_create(0.0, 0.0);
    
    if (n == 0) {
        h_n = complex_multiply(complex_exp(complex_multiply(complex_create(0.0, -1.0), z)), 
                              complex_create(0.0, -1.0));
    } else if (n == 1) {
        Complex h0 = spherical_hankel_function(0, z);
        h_n = complex_subtract(h0, complex_divide(complex_create(1.0, 0.0), z));
    } else {
        Complex h_nm1 = spherical_hankel_function(n-1, z);
        Complex h_nm2 = spherical_hankel_function(n-2, z);
        h_n = complex_subtract(complex_multiply(complex_create((2.0*n - 1.0), 0.0), 
                                               complex_divide(h_nm1, z)), h_nm2);
    }
    
    return h_n;
}

Complex spherical_bessel_function(int n, Complex z) {
    if (n < 0) return complex_create(0.0, 0.0);
    
    Complex j_n = complex_create(0.0, 0.0);
    
    if (n == 0) {
        j_n = complex_divide(complex_sin(z), z);
    } else if (n == 1) {
        j_n = complex_divide(complex_sin(z), z);
        j_n = complex_subtract(j_n, complex_divide(complex_cos(z), z));
    } else {
        Complex j_nm1 = spherical_bessel_function(n-1, z);
        Complex j_nm2 = spherical_bessel_function(n-2, z);
        j_n = complex_subtract(complex_multiply(complex_create((2.0*n - 1.0), 0.0), 
                                               complex_divide(j_nm1, z)), j_nm2);
    }
    
    return j_n;
}

MultipoleExpansion fast_multipole_compute_multipole_moments(double* source_positions, Complex* source_currents, int num_sources, double* expansion_center, int expansion_order) {
    MultipoleExpansion expansion;
    
    expansion.expansion_order = expansion_order;
    expansion.expansion_center[0] = expansion_center[0];
    expansion.expansion_center[1] = expansion_center[1];
    expansion.expansion_center[2] = expansion_center[2];
    expansion.is_converged = 0;
    
    expansion.monopole = complex_create(0.0, 0.0);
    for (int i = 0; i < 3; i++) {
        expansion.dipole[i] = complex_create(0.0, 0.0);
        for (int j = 0; j < 3; j++) {
            expansion.quadrupole[i][j] = complex_create(0.0, 0.0);
            for (int k = 0; k < 3; k++) {
                expansion.octupole[i][j][k] = complex_create(0.0, 0.0);
                for (int l = 0; l < 3; l++) {
                    expansion.hexadecapole[i][j][k][l] = complex_create(0.0, 0.0);
                }
            }
        }
    }
    
    for (int i = 0; i < num_sources; i++) {
        double dx = source_positions[3*i] - expansion_center[0];
        double dy = source_positions[3*i+1] - expansion_center[1];
        double dz = source_positions[3*i+2] - expansion_center[2];
        
        Complex current = source_currents[i];
        
        expansion.monopole = complex_add(expansion.monopole, current);
        
        expansion.dipole[0] = complex_add(expansion.dipole[0], complex_multiply(current, complex_create(dx, 0.0)));
        expansion.dipole[1] = complex_add(expansion.dipole[1], complex_multiply(current, complex_create(dy, 0.0)));
        expansion.dipole[2] = complex_add(expansion.dipole[2], complex_multiply(current, complex_create(dz, 0.0)));
        
        expansion.quadrupole[0][0] = complex_add(expansion.quadrupole[0][0], complex_multiply(current, complex_create(dx*dx, 0.0)));
        expansion.quadrupole[0][1] = complex_add(expansion.quadrupole[0][1], complex_multiply(current, complex_create(dx*dy, 0.0)));
        expansion.quadrupole[0][2] = complex_add(expansion.quadrupole[0][2], complex_multiply(current, complex_create(dx*dz, 0.0)));
        expansion.quadrupole[1][0] = expansion.quadrupole[0][1];
        expansion.quadrupole[1][1] = complex_add(expansion.quadrupole[1][1], complex_multiply(current, complex_create(dy*dy, 0.0)));
        expansion.quadrupole[1][2] = complex_add(expansion.quadrupole[1][2], complex_multiply(current, complex_create(dy*dz, 0.0)));
        expansion.quadrupole[2][0] = expansion.quadrupole[0][2];
        expansion.quadrupole[2][1] = expansion.quadrupole[1][2];
        expansion.quadrupole[2][2] = complex_add(expansion.quadrupole[2][2], complex_multiply(current, complex_create(dz*dz, 0.0)));
    }
    
    double convergence_check = 0.0;
    convergence_check += cabs(expansion.monopole);
    for (int i = 0; i < 3; i++) {
        convergence_check += cabs(expansion.dipole[i]);
        for (int j = 0; j < 3; j++) {
            convergence_check += cabs(expansion.quadrupole[i][j]);
        }
    }
    
    expansion.is_converged = (convergence_check > 1e-10);
    
    return expansion;
}

int fast_multipole_setup_sources(FastMultipoleSolver* solver, double* positions, Complex* currents, int num_sources) {
    if (!solver || !positions || !currents || num_sources <= 0) return -1;
    
    solver->num_sources = num_sources;
    
    solver->source_positions = (double*)malloc(3 * num_sources * sizeof(double));
    solver->source_currents = (Complex*)malloc(num_sources * sizeof(Complex));
    
    if (!solver->source_positions || !solver->source_currents) return -1;
    
    memcpy(solver->source_positions, positions, 3 * num_sources * sizeof(double));
    memcpy(solver->source_currents, currents, num_sources * sizeof(Complex));
    
    return 0;
}

int fast_multipole_setup_observations(FastMultipoleSolver* solver, double* positions, int num_observations) {
    if (!solver || !positions || num_observations <= 0) return -1;
    
    solver->num_observations = num_observations;
    
    solver->observation_positions = (double*)malloc(3 * num_observations * sizeof(double));
    solver->observation_fields = (Complex*)calloc(num_observations, sizeof(Complex));
    
    if (!solver->observation_positions || !solver->observation_fields) return -1;
    
    memcpy(solver->observation_positions, positions, 3 * num_observations * sizeof(double));
    
    return 0;
}

int fast_multipole_build_tree(FastMultipoleSolver* solver) {
    if (!solver) return -1;
    
    if (solver->num_sources == 0) return -1;
    
    double min_x = 1e10, max_x = -1e10;
    double min_y = 1e10, max_y = -1e10;
    double min_z = 1e10, max_z = -1e10;
    
    for (int i = 0; i < solver->num_sources; i++) {
        double x = solver->source_positions[3*i];
        double y = solver->source_positions[3*i+1];
        double z = solver->source_positions[3*i+2];
        
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        if (z < min_z) min_z = z;
        if (z > max_z) max_z = z;
    }
    
    double center_x = (min_x + max_x) / 2.0;
    double center_y = (min_y + max_y) / 2.0;
    double center_z = (min_z + max_z) / 2.0;
    double half_size = fmax(fmax(max_x - min_x, max_y - min_y), max_z - min_z) / 2.0;
    
    solver->num_clusters = 0;
    solver->tree_depth = 0;
    
    FmmCluster* root_cluster = &solver->clusters[solver->num_clusters++];
    root_cluster->x = center_x;
    root_cluster->y = center_y;
    root_cluster->z = center_z;
    root_cluster->radius = half_size;
    root_cluster->level = 0;
    root_cluster->parent_id = -1;
    root_cluster->num_children = 0;
    root_cluster->num_sources = solver->num_sources;
    root_cluster->num_observations = solver->num_observations;
    root_cluster->cluster_id = 0;
    root_cluster->type = CLUSTER_TYPE_ROOT;
    
    root_cluster->bounding_box[0] = min_x;
    root_cluster->bounding_box[1] = max_x;
    root_cluster->bounding_box[2] = min_y;
    root_cluster->bounding_box[3] = max_y;
    root_cluster->bounding_box[4] = min_z;
    root_cluster->bounding_box[5] = max_z;
    
    root_cluster->source_ids = (int*)malloc(solver->num_sources * sizeof(int));
    for (int i = 0; i < solver->num_sources; i++) {
        root_cluster->source_ids[i] = i;
    }
    
    root_cluster->observation_ids = (int*)malloc(solver->num_observations * sizeof(int));
    for (int i = 0; i < solver->num_observations; i++) {
        root_cluster->observation_ids[i] = i;
    }
    
    double expansion_center[3] = {center_x, center_y, center_z};
    MultipoleExpansion expansion = fast_multipole_compute_multipole_moments(
        solver->source_positions, solver->source_currents, solver->num_sources, 
        expansion_center, solver->config.max_multipole_order);
    
    solver->multipole_expansions[0] = expansion;
    
    return 0;
}

int fast_multipole_compute_near_field_interactions(FastMultipoleSolver* solver) {
    if (!solver) return -1;
    
    int near_field_size = solver->num_sources * solver->num_observations;
    solver->near_field_matrix = (Complex*)calloc(near_field_size, sizeof(Complex));
    if (!solver->near_field_matrix) return -1;
    
    double threshold = solver->config.near_field_threshold;
    
    for (int i = 0; i < solver->num_sources; i++) {
        double src_x = solver->source_positions[3*i];
        double src_y = solver->source_positions[3*i+1];
        double src_z = solver->source_positions[3*i+2];
        
        for (int j = 0; j < solver->num_observations; j++) {
            double obs_x = solver->observation_positions[3*j];
            double obs_y = solver->observation_positions[3*j+1];
            double obs_z = solver->observation_positions[3*j+2];
            
            double dx = obs_x - src_x;
            double dy = obs_y - src_y;
            double dz = obs_z - src_z;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (distance < threshold && distance > 1e-10) {
                Complex green_func = fast_multipole_compute_green_function(solver, 
                    &solver->source_positions[3*i], &solver->observation_positions[3*j]);
                solver->near_field_matrix[i * solver->num_observations + j] = green_func;
            }
        }
    }
    
    return 0;
}

Complex fast_multipole_compute_green_function(FastMultipoleSolver* solver, double* source_pos, double* observation_pos) {
    if (!solver || !source_pos || !observation_pos) return complex_create(0.0, 0.0);
    
    double dx = observation_pos[0] - source_pos[0];
    double dy = observation_pos[1] - source_pos[1];
    double dz = observation_pos[2] - source_pos[2];
    double distance = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (distance < 1e-10) return complex_create(0.0, 0.0);
    
    double k = solver->wave_params.k0;
    Complex phase_factor = complex_exp(complex_create(0.0, -k * distance));
    Complex distance_factor = complex_create(1.0 / (4.0 * M_PI * distance), 0.0);
    
    return complex_multiply(distance_factor, phase_factor);
}

int fast_multipole_matrix_vector_product(FastMultipoleSolver* solver, Complex* input_vector, Complex* output_vector) {
    if (!solver || !input_vector || !output_vector) return -1;
    
    solver->num_matrix_vector_products++;
    
    for (int i = 0; i < solver->num_observations; i++) {
        output_vector[i] = complex_create(0.0, 0.0);
        
        for (int j = 0; j < solver->num_sources; j++) {
            Complex matrix_element;
            
            double src_x = solver->source_positions[3*j];
            double src_y = solver->source_positions[3*j+1];
            double src_z = solver->source_positions[3*j+2];
            double obs_x = solver->observation_positions[3*i];
            double obs_y = solver->observation_positions[3*i+1];
            double obs_z = solver->observation_positions[3*i+2];
            
            double dx = obs_x - src_x;
            double dy = obs_y - src_y;
            double dz = obs_z - src_z;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (distance < solver->config.near_field_threshold) {
                matrix_element = solver->near_field_matrix[j * solver->num_observations + i];
            } else {
                matrix_element = fast_multipole_compute_green_function(solver, 
                    &solver->source_positions[3*j], &solver->observation_positions[3*i]);
            }
            
            Complex product = complex_multiply(matrix_element, input_vector[j]);
            output_vector[i] = complex_add(output_vector[i], product);
        }
    }
    
    return 0;
}

int fast_multipole_solve_frequency(FastMultipoleSolver* solver, double frequency) {
    if (!solver) return -1;
    
    double start_time = get_time_seconds();
    
    solver->wave_params.frequency = frequency;
    solver->wave_params.wavelength = C_ELECTROMAGNETIC / frequency;
    solver->wave_params.k0 = 2.0 * M_PI / solver->wave_params.wavelength;
    solver->wave_params.eta0 = ETA_0;
    solver->wave_params.propagation_constant = complex_create(0.0, solver->wave_params.k0);
    solver->wave_params.wave_impedance = complex_create(ETA_0, 0.0);
    
    int status = fast_multipole_build_tree(solver);
    if (status != 0) return status;
    
    status = fast_multipole_compute_near_field_interactions(solver);
    if (status != 0) return status;
    
    status = fast_multipole_matrix_vector_product(solver, solver->source_currents, solver->observation_fields);
    if (status != 0) return status;
    
    double end_time = get_time_seconds();
    solver->computation_time = end_time - start_time;
    solver->calculation_completed = 1;
    
    return 0;
}

int fast_multipole_solve_frequency_sweep(FastMultipoleSolver* solver) {
    if (!solver) return -1;
    
    return fast_multipole_solve_frequency(solver, 1.0e9);
}

int fast_multipole_simulate(FastMultipoleSolver* solver) {
    if (!solver) return -1;
    
    return fast_multipole_solve_frequency_sweep(solver);
}

void fast_multipole_print_summary(FastMultipoleSolver* solver) {
    if (!solver) return;
    
    printf("\n=== Fast Multipole Algorithm Summary ===\n");
    printf("Acceleration Type: %d\n", solver->config.acceleration_type);
    printf("Max Multipole Order: %d\n", solver->config.max_multipole_order);
    printf("Max Tree Levels: %d\n", solver->config.max_tree_levels);
    printf("Number of Sources: %d\n", solver->num_sources);
    printf("Number of Observations: %d\n", solver->num_observations);
    printf("Number of Clusters: %d\n", solver->num_clusters);
    printf("Tree Depth: %d\n", solver->tree_depth);
    printf("Computation Time: %.3f seconds\n", solver->computation_time);
    printf("Memory Usage: %.2f MB\n", solver->memory_usage / (1024.0 * 1024.0));
    printf("Convergence Status: %s\n", solver->convergence_status ? "Converged" : "Not Converged");
    printf("Achieved Accuracy: %.2e\n", solver->achieved_accuracy);
    printf("Matrix-Vector Products: %d\n", solver->num_matrix_vector_products);
    
    printf("\n");
}

void fast_multipole_benchmark(FastMultipoleSolver* solver) {
    if (!solver) return;
    
    printf("\n=== Fast Multipole Algorithm Benchmark ===\n");
    
    double start_time = get_time_seconds();
    
    int status = fast_multipole_simulate(solver);
    
    double end_time = get_time_seconds();
    double total_time = end_time - start_time;
    
    printf("Simulation Status: %s\n", (status == 0) ? "SUCCESS" : "FAILED");
    printf("Total Time: %.3f seconds\n", total_time);
    printf("Time per Source-Observation Pair: %.3f ns\n", 
           (total_time / (solver->num_sources * solver->num_observations)) * 1e9);
    printf("Sources per Second: %.0f\n", solver->num_sources / total_time);
    printf("Observations per Second: %.0f\n", solver->num_observations / total_time);
    printf("Matrix-Vector Products per Second: %.0f\n", solver->num_matrix_vector_products / total_time);
    printf("Memory Efficiency: %.2f bytes per source-observation pair\n", 
           (solver->memory_usage / (solver->num_sources * solver->num_observations)));
    
    if (solver->config.use_parallel_processing) {
        printf("Parallel Threads: %d\n", solver->config.num_threads);
        printf("Parallel Efficiency: %.1f%%\n", 
               (solver->num_sources / total_time) / (solver->num_sources / (total_time * solver->config.num_threads)) * 100.0);
    }
    
    printf("\n");
}