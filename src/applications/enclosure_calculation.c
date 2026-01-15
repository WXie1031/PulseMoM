#include "enclosure_calculation.h"
#include "../math_utils.h"
#include "../constants.h"
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

EnclosureCalculationSolver* enclosure_calculation_create(EnclosureCalculationConfig* config) {
    if (!config) return NULL;
    
    EnclosureCalculationSolver* solver = (EnclosureCalculationSolver*)calloc(1, sizeof(EnclosureCalculationSolver));
    if (!solver) return NULL;
    
    memcpy(&solver->config, config, sizeof(EnclosureCalculationConfig));
    
    solver->num_segments = 0;
    solver->num_cavity_modes = 0;
    solver->num_apertures = 0;
    solver->num_conductors = 0;
    solver->num_frequencies = 0;
    solver->calculation_completed = 0;
    solver->computation_time = 0.0;
    solver->memory_usage = 0.0;
    solver->convergence_status = 0;
    
    solver->frequency_vector = (double*)calloc(config->num_frequencies, sizeof(double));
    if (!solver->frequency_vector) {
        free(solver);
        return NULL;
    }
    
    double freq_step = (config->frequency_max - config->frequency_min) / (config->num_frequencies - 1);
    for (int i = 0; i < config->num_frequencies; i++) {
        solver->frequency_vector[i] = config->frequency_min + i * freq_step;
    }
    
    solver->mom_solver = mom_solver_create(config->max_iterations, config->convergence_tolerance);
    if (!solver->mom_solver) {
        free(solver->frequency_vector);
        free(solver);
        return NULL;
    }
    
    return solver;
}

void enclosure_calculation_destroy(EnclosureCalculationSolver* solver) {
    if (!solver) return;
    
    if (solver->frequency_vector) free(solver->frequency_vector);
    if (solver->impedance_matrix) free(solver->impedance_matrix);
    if (solver->admittance_matrix) free(solver->admittance_matrix);
    if (solver->scattering_matrix) free(solver->scattering_matrix);
    if (solver->mom_solver) mom_solver_destroy(solver->mom_solver);
    
    free(solver);
}

int enclosure_setup_rectangular(EnclosureCalculationSolver* solver, double length, double width, double height) {
    if (!solver) return -1;
    
    solver->config.type = ENCLOSURE_TYPE_RECTANGULAR;
    solver->config.enclosure_length = length;
    solver->config.enclosure_width = width;
    solver->config.enclosure_height = height;
    solver->num_segments = 0;
    
    int nx = (int)(length / solver->config.mesh_density);
    int ny = (int)(width / solver->config.mesh_density);
    int nz = (int)(height / solver->config.mesh_density);
    
    if (nx < 5) nx = 5;
    if (ny < 5) ny = 5;
    if (nz < 5) nz = 5;
    
    int segment_id = 0;
    
    for (int i = 0; i <= nx; i++) {
        double x = i * length / nx;
        for (int j = 0; j <= ny; j++) {
            double y = j * width / ny;
            for (int k = 0; k <= nz; k++) {
                double z = k * height / nz;
                
                if (i == 0 || i == nx || j == 0 || j == ny || k == 0 || k == nz) {
                    if (segment_id < MAX_ENCLOSURE_SEGMENTS) {
                        EnclosureSegment* seg = &solver->segments[segment_id];
                        seg->x = x;
                        seg->y = y;
                        seg->z = z;
                        seg->length = solver->config.mesh_density;
                        seg->width = solver->config.mesh_density;
                        seg->height = solver->config.mesh_density;
                        seg->thickness = solver->config.wall_thickness;
                        seg->conductivity = solver->config.conductivity;
                        seg->permeability = solver->config.permeability;
                        seg->permittivity = solver->config.wall_permittivity;
                        seg->boundary_condition = BOUNDARY_CONDITION_PERFECT_ELECTRIC_CONDUCTOR;
                        seg->segment_id = segment_id;
                        seg->num_connections = 0;
                        
                        segment_id++;
                        solver->num_segments++;
                    }
                }
            }
        }
    }
    
    return 0;
}

int enclosure_setup_circular(EnclosureCalculationSolver* solver, double radius, double height) {
    if (!solver) return -1;
    
    solver->config.type = ENCLOSURE_TYPE_CIRCULAR;
    solver->config.enclosure_length = 2.0 * radius;
    solver->config.enclosure_width = 2.0 * radius;
    solver->config.enclosure_height = height;
    solver->num_segments = 0;
    
    int nr = (int)(radius / solver->config.mesh_density);
    int nz = (int)(height / solver->config.mesh_density);
    int nphi = 36;
    
    if (nr < 5) nr = 5;
    if (nz < 5) nz = 5;
    
    int segment_id = 0;
    
    for (int i = 0; i <= nr; i++) {
        double r = i * radius / nr;
        for (int k = 0; k <= nz; k++) {
            double z = k * height / nz;
            for (int j = 0; j < nphi; j++) {
                double phi = 2.0 * M_PI * j / nphi;
                double x = r * cos(phi);
                double y = r * sin(phi);
                
                if (i == nr || k == 0 || k == nz) {
                    if (segment_id < MAX_ENCLOSURE_SEGMENTS) {
                        EnclosureSegment* seg = &solver->segments[segment_id];
                        seg->x = x;
                        seg->y = y;
                        seg->z = z;
                        seg->radius = solver->config.mesh_density;
                        seg->thickness = solver->config.wall_thickness;
                        seg->conductivity = solver->config.conductivity;
                        seg->permeability = solver->config.permeability;
                        seg->permittivity = solver->config.wall_permittivity;
                        seg->boundary_condition = BOUNDARY_CONDITION_PERFECT_ELECTRIC_CONDUCTOR;
                        seg->segment_id = segment_id;
                        seg->num_connections = 0;
                        
                        segment_id++;
                        solver->num_segments++;
                    }
                }
            }
        }
    }
    
    return 0;
}

int enclosure_setup_waveguide(EnclosureCalculationSolver* solver, double width, double height, double length) {
    if (!solver) return -1;
    
    solver->config.type = ENCLOSURE_TYPE_WAVEGUIDE;
    solver->config.enclosure_length = length;
    solver->config.enclosure_width = width;
    solver->config.enclosure_height = height;
    solver->num_segments = 0;
    
    int nx = (int)(length / solver->config.mesh_density);
    int ny = (int)(width / solver->config.mesh_density);
    int nz = (int)(height / solver->config.mesh_density);
    
    if (nx < 5) nx = 5;
    if (ny < 5) ny = 5;
    if (nz < 5) nz = 5;
    
    int segment_id = 0;
    
    for (int i = 0; i <= nx; i++) {
        double x = i * length / nx;
        for (int j = 0; j <= ny; j++) {
            double y = j * width / ny;
            for (int k = 0; k <= nz; k++) {
                double z = k * height / nz;
                
                if (j == 0 || j == ny || k == 0 || k == nz) {
                    if (segment_id < MAX_ENCLOSURE_SEGMENTS) {
                        EnclosureSegment* seg = &solver->segments[segment_id];
                        seg->x = x;
                        seg->y = y;
                        seg->z = z;
                        seg->length = solver->config.mesh_density;
                        seg->width = solver->config.mesh_density;
                        seg->height = solver->config.mesh_density;
                        seg->thickness = solver->config.wall_thickness;
                        seg->conductivity = solver->config.conductivity;
                        seg->permeability = solver->config.permeability;
                        seg->permittivity = solver->config.wall_permittivity;
                        seg->boundary_condition = BOUNDARY_CONDITION_PERFECT_ELECTRIC_CONDUCTOR;
                        seg->segment_id = segment_id;
                        seg->num_connections = 0;
                        
                        segment_id++;
                        solver->num_segments++;
                    }
                }
            }
        }
    }
    
    return 0;
}

int enclosure_compute_cavity_modes(EnclosureCalculationSolver* solver, double frequency) {
    if (!solver) return -1;
    
    solver->num_cavity_modes = 0;
    
    double a = solver->config.enclosure_length;
    double b = solver->config.enclosure_width;
    double c = solver->config.enclosure_height;
    
    int max_modes = 10;
    
    for (int m = 0; m <= max_modes; m++) {
        for (int n = 0; n <= max_modes; n++) {
            for (int p = 0; p <= max_modes; p++) {
                if (m == 0 && n == 0 && p == 0) continue;
                
                double k_mnp = M_PI * sqrt((double)(m*m)/(a*a) + (double)(n*n)/(b*b) + (double)(p*p)/(c*c));
                double f_mnp = C_ELECTROMAGNETIC * k_mnp / (2.0 * M_PI);
                
                if (f_mnp <= 2.0 * frequency && solver->num_cavity_modes < MAX_CAVITY_MODES) {
                    CavityMode* mode = &solver->cavity_modes[solver->num_cavity_modes];
                    mode->m = m;
                    mode->n = n;
                    mode->p = p;
                    mode->frequency = f_mnp;
                    mode->propagation_constant = complex_create(0.0, k_mnp);
                    mode->quality_factor = 1000.0;
                    mode->attenuation = 0.01;
                    mode->phase_velocity = C_ELECTROMAGNETIC;
                    mode->group_velocity = C_ELECTROMAGNETIC;
                    mode->power_handling = 1e6;
                    mode->mode_type = CAVITY_MODE_TE;
                    mode->is_propagating = 1;
                    mode->is_resonant = 1;
                    mode->is_evanescent = 0;
                    mode->is_degenerate = 0;
                    
                    solver->num_cavity_modes++;
                }
            }
        }
    }
    
    return 0;
}

int enclosure_compute_cavity_resonances(EnclosureCalculationSolver* solver) {
    if (!solver) return -1;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        double frequency = solver->frequency_vector[i];
        
        int num_resonances = 0;
        double total_q = 0.0;
        
        for (int j = 0; j < solver->num_cavity_modes; j++) {
            CavityMode* mode = &solver->cavity_modes[j];
            
            if (fabs(mode->frequency - frequency) < 0.01 * frequency) {
                num_resonances++;
                total_q += mode->quality_factor;
            }
        }
        
        solver->params[i].frequency = frequency;
        solver->params[i].resonance_frequency = frequency;
        solver->params[i].quality_factor = (num_resonances > 0) ? total_q / num_resonances : 1000.0;
        solver->params[i].bandwidth = frequency / solver->params[i].quality_factor;
    }
    
    return 0;
}

int enclosure_compute_shielding_effectiveness(EnclosureCalculationSolver* solver, double frequency) {
    if (!solver) return -1;
    
    double omega = 2.0 * M_PI * frequency;
    double skin_depth = sqrt(2.0 / (omega * solver->config.conductivity * solver->config.permeability));
    
    double absorption_loss = 20.0 * log10(exp(solver->config.wall_thickness / skin_depth)) / log10(exp(1.0));
    
    double reflection_loss = 20.0 * log10(ETA_0 / (4.0 * sqrt(omega * solver->config.permeability / (2.0 * solver->config.conductivity))));
    
    double multiple_reflection_correction = 20.0 * log10(1.0 - exp(-2.0 * solver->config.wall_thickness / skin_depth));
    
    double shielding_effectiveness = absorption_loss + reflection_loss + multiple_reflection_correction;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        if (fabs(solver->frequency_vector[i] - frequency) < 1e-6) {
            solver->params[i].shielding_effectiveness = shielding_effectiveness;
            break;
        }
    }
    
    return 0;
}

int enclosure_compute_insertion_loss(EnclosureCalculationSolver* solver, double frequency) {
    if (!solver) return -1;
    
    double omega = 2.0 * M_PI * frequency;
    double k0 = omega * sqrt(EPSILON_0 * MU_0);
    
    double a = solver->config.enclosure_length;
    double b = solver->config.enclosure_width;
    double c = solver->config.enclosure_height;
    
    double cutoff_frequency_te10 = C_ELECTROMAGNETIC / (2.0 * a);
    double cutoff_frequency_te01 = C_ELECTROMAGNETIC / (2.0 * b);
    
    double cutoff_frequency = fmax(cutoff_frequency_te10, cutoff_frequency_te01);
    
    double propagation_constant = 0.0;
    if (frequency > cutoff_frequency) {
        propagation_constant = sqrt(k0*k0 - (M_PI/a)*(M_PI/a) - (M_PI/b)*(M_PI/b));
    } else {
        propagation_constant = -sqrt((M_PI/a)*(M_PI/a) + (M_PI/b)*(M_PI/b) - k0*k0);
    }
    
    double insertion_loss = 20.0 * log10(exp(-c * fabs(propagation_constant))) / log10(exp(1.0));
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        if (fabs(solver->frequency_vector[i] - frequency) < 1e-6) {
            solver->params[i].insertion_loss = insertion_loss;
            break;
        }
    }
    
    return 0;
}

int enclosure_compute_impedance_matrix(EnclosureCalculationSolver* solver, double frequency) {
    if (!solver) return -1;
    
    int n = solver->num_segments;
    solver->matrix_size = n;
    
    if (solver->impedance_matrix) free(solver->impedance_matrix);
    solver->impedance_matrix = (Complex*)calloc(n * n, sizeof(Complex));
    if (!solver->impedance_matrix) return -1;
    
    double omega = 2.0 * M_PI * frequency;
    double k = omega * sqrt(EPSILON_0 * MU_0);
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Complex z_ij;
            
            if (i == j) {
                double self_impedance = 100.0 + 0.01 * omega;
                z_ij = complex_create(self_impedance, 0.0);
            } else {
                EnclosureSegment* seg_i = &solver->segments[i];
                EnclosureSegment* seg_j = &solver->segments[j];
                
                double dx = seg_i->x - seg_j->x;
                double dy = seg_i->y - seg_j->y;
                double dz = seg_i->z - seg_j->z;
                double distance = sqrt(dx*dx + dy*dy + dz*dz);
                
                if (distance < 1e-10) {
                    z_ij = complex_create(0.0, 0.0);
                } else {
                    double mutual_impedance = 10.0 * exp(-k * distance) / distance;
                    z_ij = complex_create(mutual_impedance, 0.0);
                }
            }
            
            solver->impedance_matrix[i * n + j] = z_ij;
        }
    }
    
    return 0;
}

int enclosure_add_aperture(EnclosureCalculationSolver* solver, double x, double y, double z, double length, double width) {
    if (!solver || solver->num_apertures >= MAX_APERTURE_TYPES) return -1;
    
    ApertureStructure* aperture = &solver->apertures[solver->num_apertures];
    
    aperture->x = x;
    aperture->y = y;
    aperture->z = z;
    aperture->length = length;
    aperture->width = width;
    aperture->area = length * width;
    aperture->perimeter = 2.0 * (length + width);
    aperture->equivalent_impedance = complex_create(50.0, 0.0);
    aperture->equivalent_admittance = complex_create(0.02, 0.0);
    aperture->radiation_resistance = 377.0 / (length * width);
    aperture->leakage_coefficient = 0.1;
    aperture->aperture_id = solver->num_apertures;
    aperture->is_radiating = 1;
    aperture->is_leaking = 1;
    aperture->is_coupling = 0;
    
    solver->num_apertures++;
    
    return 0;
}

int enclosure_solve_frequency(EnclosureCalculationSolver* solver, double frequency) {
    if (!solver) return -1;
    
    double start_time = get_time_seconds();
    
    int status = enclosure_compute_cavity_modes(solver, frequency);
    if (status != 0) return status;
    
    status = enclosure_compute_impedance_matrix(solver, frequency);
    if (status != 0) return status;
    
    status = enclosure_compute_shielding_effectiveness(solver, frequency);
    if (status != 0) return status;
    
    status = enclosure_compute_insertion_loss(solver, frequency);
    if (status != 0) return status;
    
    double end_time = get_time_seconds();
    solver->computation_time = end_time - start_time;
    solver->calculation_completed = 1;
    
    return 0;
}

int enclosure_solve_frequency_sweep(EnclosureCalculationSolver* solver) {
    if (!solver) return -1;
    
    solver->num_frequencies = 0;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        double frequency = solver->frequency_vector[i];
        
        int status = enclosure_solve_frequency(solver, frequency);
        if (status == 0) {
            solver->num_frequencies++;
        }
    }
    
    enclosure_compute_cavity_resonances(solver);
    
    return (solver->num_frequencies > 0) ? 0 : -1;
}

int enclosure_calculation_simulate(EnclosureCalculationSolver* solver) {
    if (!solver) return -1;
    
    return enclosure_solve_frequency_sweep(solver);
}

void enclosure_print_summary(EnclosureCalculationSolver* solver) {
    if (!solver) return;
    
    printf("\n=== Enclosure Calculation Summary ===\n");
    printf("Enclosure Type: %d\n", solver->config.type);
    printf("Dimensions: %.3f x %.3f x %.3f m\n", 
           solver->config.enclosure_length,
           solver->config.enclosure_width,
           solver->config.enclosure_height);
    printf("Number of Segments: %d\n", solver->num_segments);
    printf("Number of Cavity Modes: %d\n", solver->num_cavity_modes);
    printf("Number of Apertures: %d\n", solver->num_apertures);
    printf("Number of Conductors: %d\n", solver->num_conductors);
    printf("Frequency Range: %.3f MHz - %.3f MHz\n", 
           solver->config.frequency_min / 1e6, 
           solver->config.frequency_max / 1e6);
    printf("Computation Time: %.3f seconds\n", solver->computation_time);
    printf("Memory Usage: %.2f MB\n", solver->memory_usage / (1024.0 * 1024.0));
    printf("Convergence Status: %s\n", solver->convergence_status ? "Converged" : "Not Converged");
    
    if (solver->num_frequencies > 0) {
        printf("\nFrequency Response:\n");
        for (int i = 0; i < solver->num_frequencies; i += solver->config.num_frequencies / 5) {
            double freq = solver->frequency_vector[i];
            double se = solver->params[i].shielding_effectiveness;
            double il = solver->params[i].insertion_loss;
            double q = solver->params[i].quality_factor;
            
            printf("  %.1f MHz: SE=%.1f dB, IL=%.1f dB, Q=%.1f\n",
                   freq / 1e6, se, il, q);
        }
    }
    
    printf("\n");
}

void enclosure_benchmark(EnclosureCalculationSolver* solver) {
    if (!solver) return;
    
    printf("\n=== Enclosure Calculation Benchmark ===\n");
    
    double start_time = get_time_seconds();
    
    int status = enclosure_calculation_simulate(solver);
    
    double end_time = get_time_seconds();
    double total_time = end_time - start_time;
    
    printf("Simulation Status: %s\n", (status == 0) ? "SUCCESS" : "FAILED");
    printf("Total Time: %.3f seconds\n", total_time);
    printf("Time per Frequency: %.3f ms\n", (total_time / solver->config.num_frequencies) * 1000.0);
    printf("Segments per Second: %.0f\n", solver->num_segments / total_time);
    printf("Cavity Modes per Second: %.0f\n", solver->num_cavity_modes / total_time);
    printf("Matrix Size: %d x %d\n", solver->matrix_size, solver->matrix_size);
    printf("Memory per Matrix Element: %.2f bytes\n", 
           (double)(solver->matrix_size * solver->matrix_size * sizeof(Complex)) / (solver->matrix_size * solver->matrix_size));
    
    if (solver->config.use_parallel_computation) {
        printf("Parallel Threads: %d\n", solver->config.num_threads);
        printf("Parallel Efficiency: %.1f%%\n", 
               (solver->num_segments / total_time) / (solver->num_segments / (total_time * solver->config.num_threads)) * 100.0);
    }
    
    printf("\n");
}