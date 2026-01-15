#include "mom_fullwave_solver.h"
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

AntennaFullwaveSolver* antenna_fullwave_create(AntennaSimulationConfig* config) {
    if (!config) return NULL;
    
    AntennaFullwaveSolver* solver = (AntennaFullwaveSolver*)calloc(1, sizeof(AntennaFullwaveSolver));
    if (!solver) return NULL;
    
    memcpy(&solver->config, config, sizeof(AntennaSimulationConfig));
    
    solver->num_segments = 0;
    solver->num_frequencies = 0;
    solver->num_near_field_points = 0;
    solver->simulation_completed = 0;
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

void antenna_fullwave_destroy(AntennaFullwaveSolver* solver) {
    if (!solver) return;
    
    if (solver->frequency_vector) free(solver->frequency_vector);
    if (solver->impedance_matrix) free(solver->impedance_matrix);
    if (solver->voltage_vector) free(solver->voltage_vector);
    if (solver->current_vector) free(solver->current_vector);
    if (solver->mom_solver) mom_solver_destroy(solver->mom_solver);
    
    free(solver);
}

WaveParameters antenna_fullwave_compute_wave_parameters(double frequency, double relative_permittivity, double conductivity) {
    WaveParameters params;
    
    params.frequency = frequency;
    params.wavelength = C_ELECTROMAGNETIC / frequency;
    params.k0 = 2.0 * M_PI / params.wavelength;
    params.eta0 = ETA_0;
    
    double omega = 2.0 * M_PI * frequency;
    double epsilon = EPSILON_0 * relative_permittivity;
    double mu = MU_0;
    
    Complex epsilon_complex = complex_create(epsilon, -conductivity / omega);
    Complex mu_complex = complex_create(mu, 0.0);
    
    params.propagation_constant = complex_sqrt(complex_multiply(complex_create(omega * omega, 0.0), 
                                                               complex_multiply(epsilon_complex, mu_complex)));
    
    params.characteristic_impedance = complex_sqrt(complex_divide(mu_complex, epsilon_complex));
    
    double skin_depth_factor = sqrt(2.0 / (omega * mu * conductivity));
    params.skin_depth = skin_depth_factor;
    params.surface_resistance = 1.0 / (conductivity * skin_depth_factor);
    
    return params;
}

Complex antenna_fullwave_compute_segment_impedance(AntennaSegment* segment, double frequency, WaveParameters* wave_params) {
    if (!segment || !wave_params) return complex_create(0.0, 0.0);
    
    double omega = 2.0 * M_PI * frequency;
    double length = segment->length;
    double radius = segment->radius;
    
    double k = creal(wave_params->propagation_constant);
    double eta = creal(wave_params->characteristic_impedance);
    
    double R_rad = 20.0 * M_PI * M_PI * pow(length / wave_params->wavelength, 2.0);
    
    double R_loss = wave_params->surface_resistance * length / (2.0 * M_PI * radius);
    
    double L = MU_0 * length / (2.0 * M_PI) * log(length / radius - 1.0);
    double X_L = omega * L;
    
    double C = 2.0 * M_PI * EPSILON_0 * length / log(length / radius - 1.0);
    double X_C = -1.0 / (omega * C);
    
    return complex_create(R_rad + R_loss, X_L + X_C);
}

Complex antenna_fullwave_compute_mutual_impedance(AntennaSegment* seg1, AntennaSegment* seg2, double frequency, WaveParameters* wave_params) {
    if (!seg1 || !seg2 || !wave_params) return complex_create(0.0, 0.0);
    
    double dx = seg2->x - seg1->x;
    double dy = seg2->y - seg1->y;
    double dz = seg2->z - seg1->z;
    double distance = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (distance < 1e-10) return complex_create(0.0, 0.0);
    
    double k = creal(wave_params->propagation_constant);
    double eta = creal(wave_params->characteristic_impedance);
    
    Complex exp_term = complex_exp(complex_create(0.0, -k * distance));
    
    double factor = -eta * seg1->length * seg2->length / (4.0 * M_PI * distance);
    
    return complex_multiply(complex_create(factor, 0.0), exp_term);
}

int antenna_fullwave_setup_dipole(AntennaFullwaveSolver* solver, double length, double radius, double height) {
    if (!solver) return -1;
    
    solver->config.type = ANTENNA_TYPE_DIPOLE;
    solver->num_segments = 0;
    
    int num_segments = solver->config.num_segments;
    if (num_segments < 10) num_segments = 20;
    
    double segment_length = length / num_segments;
    
    for (int i = 0; i < num_segments; i++) {
        AntennaSegment* seg = &solver->segments[i];
        
        seg->x = 0.0;
        seg->y = 0.0;
        seg->z = height - length/2.0 + i * segment_length + segment_length/2.0;
        seg->radius = radius;
        seg->length = segment_length;
        seg->impedance = complex_create(0.0, 0.0);
        seg->segment_id = i;
        seg->num_connections = 0;
        
        if (i > 0) {
            seg->connected_to[seg->num_connections++] = i - 1;
        }
        if (i < num_segments - 1) {
            seg->connected_to[seg->num_connections++] = i + 1;
        }
        
        solver->num_segments++;
    }
    
    return 0;
}

int antenna_fullwave_setup_monopole(AntennaFullwaveSolver* solver, double length, double radius) {
    if (!solver) return -1;
    
    solver->config.type = ANTENNA_TYPE_MONOPOLE;
    solver->config.ground_type = GROUND_TYPE_PERFECT_ELECTRIC_CONDUCTOR;
    solver->num_segments = 0;
    
    int num_segments = solver->config.num_segments;
    if (num_segments < 10) num_segments = 20;
    
    double segment_length = length / num_segments;
    
    for (int i = 0; i < num_segments; i++) {
        AntennaSegment* seg = &solver->segments[i];
        
        seg->x = 0.0;
        seg->y = 0.0;
        seg->z = i * segment_length + segment_length/2.0;
        seg->radius = radius;
        seg->length = segment_length;
        seg->impedance = complex_create(0.0, 0.0);
        seg->segment_id = i;
        seg->num_connections = 0;
        
        if (i > 0) {
            seg->connected_to[seg->num_connections++] = i - 1;
        }
        if (i < num_segments - 1) {
            seg->connected_to[seg->num_connections++] = i + 1;
        }
        
        solver->num_segments++;
    }
    
    return 0;
}

int antenna_fullwave_setup_loop(AntennaFullwaveSolver* solver, double radius, double wire_radius) {
    if (!solver) return -1;
    
    solver->config.type = ANTENNA_TYPE_LOOP;
    solver->num_segments = 0;
    
    int num_segments = solver->config.num_segments;
    if (num_segments < 20) num_segments = 36;
    
    double circumference = 2.0 * M_PI * radius;
    double segment_length = circumference / num_segments;
    
    for (int i = 0; i < num_segments; i++) {
        AntennaSegment* seg = &solver->segments[i];
        
        double angle = 2.0 * M_PI * i / num_segments;
        seg->x = radius * cos(angle);
        seg->y = radius * sin(angle);
        seg->z = 0.0;
        seg->radius = wire_radius;
        seg->length = segment_length;
        seg->impedance = complex_create(0.0, 0.0);
        seg->segment_id = i;
        seg->num_connections = 0;
        
        int prev_i = (i - 1 + num_segments) % num_segments;
        int next_i = (i + 1) % num_segments;
        
        seg->connected_to[seg->num_connections++] = prev_i;
        seg->connected_to[seg->num_connections++] = next_i;
        
        solver->num_segments++;
    }
    
    return 0;
}

int antenna_fullwave_setup_patch(AntennaFullwaveSolver* solver, double length, double width, double height) {
    if (!solver) return -1;
    
    solver->config.type = ANTENNA_TYPE_PATCH;
    solver->config.substrate_height = height;
    solver->num_segments = 0;
    
    int nx = (int)(length / solver->config.mesh_density);
    int ny = (int)(width / solver->config.mesh_density);
    
    if (nx < 5) nx = 5;
    if (ny < 5) ny = 5;
    
    int segment_id = 0;
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            AntennaSegment* seg = &solver->segments[segment_id];
            
            seg->x = -length/2.0 + i * (length/nx) + (length/nx)/2.0;
            seg->y = -width/2.0 + j * (width/ny) + (width/ny)/2.0;
            seg->z = height;
            seg->radius = solver->config.mesh_density / 2.0;
            seg->length = solver->config.mesh_density;
            seg->impedance = complex_create(0.0, 0.0);
            seg->segment_id = segment_id;
            seg->num_connections = 0;
            
            segment_id++;
            solver->num_segments++;
            
            if (solver->num_segments >= MAX_ANTENNA_SEGMENTS) break;
        }
        if (solver->num_segments >= MAX_ANTENNA_SEGMENTS) break;
    }
    
    return 0;
}

int antenna_fullwave_add_feed(AntennaFullwaveSolver* solver, double x, double y, double z, FeedType type) {
    if (!solver) return -1;
    
    solver->config.feed_type = type;
    
    double min_distance = 1e10;
    int closest_segment = -1;
    
    for (int i = 0; i < solver->num_segments; i++) {
        AntennaSegment* seg = &solver->segments[i];
        double distance = sqrt(pow(seg->x - x, 2) + pow(seg->y - y, 2) + pow(seg->z - z, 2));
        
        if (distance < min_distance) {
            min_distance = distance;
            closest_segment = i;
        }
    }
    
    if (closest_segment >= 0) {
        solver->segments[closest_segment].impedance = complex_create(50.0, 0.0);
        return closest_segment;
    }
    
    return -1;
}

int antenna_fullwave_compute_impedance_matrix(AntennaFullwaveSolver* solver, double frequency) {
    if (!solver) return -1;
    
    int n = solver->num_segments;
    solver->matrix_size = n;
    
    if (solver->impedance_matrix) free(solver->impedance_matrix);
    solver->impedance_matrix = (Complex*)calloc(n * n, sizeof(Complex));
    if (!solver->impedance_matrix) return -1;
    
    WaveParameters wave_params = antenna_fullwave_compute_wave_parameters(frequency, 
                                                                          solver->config.substrate_permittivity,
                                                                          solver->config.substrate_conductivity);
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Complex z_ij;
            
            if (i == j) {
                z_ij = antenna_fullwave_compute_segment_impedance(&solver->segments[i], frequency, &wave_params);
            } else {
                z_ij = antenna_fullwave_compute_mutual_impedance(&solver->segments[i], &solver->segments[j], frequency, &wave_params);
            }
            
            solver->impedance_matrix[i * n + j] = z_ij;
        }
    }
    
    return 0;
}

int antenna_fullwave_compute_voltage_vector(AntennaFullwaveSolver* solver, double frequency) {
    if (!solver || !solver->impedance_matrix) return -1;
    
    int n = solver->matrix_size;
    
    if (solver->voltage_vector) free(solver->voltage_vector);
    solver->voltage_vector = (Complex*)calloc(n, sizeof(Complex));
    if (!solver->voltage_vector) return -1;
    
    double omega = 2.0 * M_PI * frequency;
    double voltage_amplitude = 1.0;
    
    for (int i = 0; i < n; i++) {
        AntennaSegment* seg = &solver->segments[i];
        
        if (cabs(seg->impedance) > 0.0) {
            solver->voltage_vector[i] = complex_create(voltage_amplitude, 0.0);
        } else {
            solver->voltage_vector[i] = complex_create(0.0, 0.0);
        }
    }
    
    return 0;
}

int antenna_fullwave_solve_current_distribution(AntennaFullwaveSolver* solver) {
    if (!solver || !solver->impedance_matrix || !solver->voltage_vector) return -1;
    
    int n = solver->matrix_size;
    
    if (solver->current_vector) free(solver->current_vector);
    solver->current_vector = (Complex*)calloc(n, sizeof(Complex));
    if (!solver->current_vector) return -1;
    
    Matrix* Z_matrix = matrix_create(n, n, COMPLEX);
    if (!Z_matrix) return -1;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matrix_set_complex(Z_matrix, i, j, solver->impedance_matrix[i * n + j]);
        }
    }
    
    Matrix* V_vector = matrix_create(n, 1, COMPLEX);
    if (!V_vector) {
        matrix_destroy(Z_matrix);
        return -1;
    }
    
    for (int i = 0; i < n; i++) {
        matrix_set_complex(V_vector, i, 0, solver->voltage_vector[i]);
    }
    
    Matrix* I_vector = matrix_create(n, 1, COMPLEX);
    if (!I_vector) {
        matrix_destroy(Z_matrix);
        matrix_destroy(V_vector);
        return -1;
    }
    
    int status = matrix_solve_linear_complex(Z_matrix, V_vector, I_vector);
    
    if (status == 0) {
        for (int i = 0; i < n; i++) {
            solver->current_vector[i] = matrix_get_complex(I_vector, i, 0);
        }
        solver->convergence_status = 1;
    } else {
        solver->convergence_status = 0;
    }
    
    matrix_destroy(Z_matrix);
    matrix_destroy(V_vector);
    matrix_destroy(I_vector);
    
    return status;
}

int antenna_fullwave_compute_far_field(AntennaFullwaveSolver* solver, double frequency) {
    if (!solver || !solver->current_vector) return -1;
    
    WaveParameters wave_params = antenna_fullwave_compute_wave_parameters(frequency, 
                                                                          solver->config.substrate_permittivity,
                                                                          solver->config.substrate_conductivity);
    
    double k = wave_params.k0;
    double eta = wave_params.eta0;
    double wavelength = wave_params.wavelength;
    
    int theta_points = solver->config.num_far_field_angles;
    int phi_points = solver->config.num_far_field_angles;
    
    if (theta_points > FAR_FIELD_ANGLES) theta_points = FAR_FIELD_ANGLES;
    if (phi_points > FAR_FIELD_ANGLES) phi_points = FAR_FIELD_ANGLES;
    
    for (int i = 0; i < theta_points; i++) {
        double theta = M_PI * i / (theta_points - 1);
        
        for (int j = 0; j < phi_points; j++) {
            double phi = 2.0 * M_PI * j / (phi_points - 1);
            
            FarFieldPoint* ff = &solver->far_field[i][j];
            ff->theta = theta;
            ff->phi = phi;
            ff->r = 1000.0 * wavelength;
            
            Complex e_theta = complex_create(0.0, 0.0);
            Complex e_phi = complex_create(0.0, 0.0);
            
            for (int seg_idx = 0; seg_idx < solver->num_segments; seg_idx++) {
                AntennaSegment* seg = &solver->segments[seg_idx];
                Complex I_seg = solver->current_vector[seg_idx];
                
                double r_prime = sqrt(seg->x*seg->x + seg->y*seg->y + seg->z*seg->z);
                double cos_alpha = (seg->x * sin(theta) * cos(phi) + 
                                   seg->y * sin(theta) * sin(phi) + 
                                   seg->z * cos(theta)) / r_prime;
                
                double r_distance = ff->r - r_prime * cos_alpha;
                
                Complex phase_factor = complex_exp(complex_create(0.0, -k * r_distance));
                
                double theta_component = cos(theta) * cos(phi) * seg->x + 
                                       cos(theta) * sin(phi) * seg->y - 
                                       sin(theta) * seg->z;
                
                double phi_component = -sin(phi) * seg->x + cos(phi) * seg->y;
                
                Complex contrib_theta = complex_multiply(complex_create(theta_component, 0.0), 
                                                       complex_multiply(I_seg, phase_factor));
                Complex contrib_phi = complex_multiply(complex_create(phi_component, 0.0), 
                                                     complex_multiply(I_seg, phase_factor));
                
                e_theta = complex_add(e_theta, contrib_theta);
                e_phi = complex_add(e_phi, contrib_phi);
            }
            
            double field_factor = -k * eta / (4.0 * M_PI * ff->r);
            ff->e_theta = complex_multiply(complex_create(field_factor, 0.0), e_theta);
            ff->e_phi = complex_multiply(complex_create(field_factor, 0.0), e_phi);
            
            ff->h_theta = complex_divide(ff->e_phi, complex_create(eta, 0.0));
            ff->h_phi = complex_divide(complex_multiply(complex_create(-1.0, 0.0), ff->e_theta), complex_create(eta, 0.0));
            
            double e_mag_sq = cabs(ff->e_theta) * cabs(ff->e_theta) + cabs(ff->e_phi) * cabs(ff->e_phi);
            ff->power_density = 0.5 * e_mag_sq / eta;
            
            ff->poynting_vector[0] = complex_create(0.5 * creal(complex_multiply(ff->e_theta, conj(ff->h_phi))) -
                                                    0.5 * creal(complex_multiply(ff->e_phi, conj(ff->h_theta))), 0.0);
            ff->poynting_vector[1] = complex_create(0.5 * creal(complex_multiply(ff->e_phi, conj(ff->e_theta))) -
                                                    0.5 * creal(complex_multiply(ff->e_theta, conj(ff->e_phi))), 0.0);
            ff->poynting_vector[2] = complex_create(0.5 * creal(complex_multiply(ff->e_theta, conj(ff->e_theta))) +
                                                    0.5 * creal(complex_multiply(ff->e_phi, conj(ff->e_phi))), 0.0);
        }
    }
    
    return 0;
}

int antenna_fullwave_compute_input_impedance(AntennaFullwaveSolver* solver, double frequency) {
    if (!solver || !solver->current_vector) return -1;
    
    int feed_segment = -1;
    for (int i = 0; i < solver->num_segments; i++) {
        if (cabs(solver->segments[i].impedance) > 0.0) {
            feed_segment = i;
            break;
        }
    }
    
    if (feed_segment < 0) return -1;
    
    Complex I_feed = solver->current_vector[feed_segment];
    Complex V_feed = solver->voltage_vector[feed_segment];
    
    Complex Z_in = complex_divide(V_feed, I_feed);
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        if (fabs(solver->frequency_vector[i] - frequency) < 1e-6) {
            solver->params[i].frequency = frequency;
            solver->params[i].zin = Z_in;
            break;
        }
    }
    
    return 0;
}

int antenna_fullwave_compute_vswr(AntennaFullwaveSolver* solver, double frequency) {
    if (!solver) return -1;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        if (fabs(solver->frequency_vector[i] - frequency) < 1e-6) {
            Complex Z_in = solver->params[i].zin;
            Complex Z0 = complex_create(50.0, 0.0);
            
            Complex gamma = complex_divide(complex_subtract(Z_in, Z0), complex_add(Z_in, Z0));
            double vswr = (1.0 + cabs(gamma)) / (1.0 - cabs(gamma));
            
            solver->params[i].gamma = gamma;
            solver->params[i].vswr = vswr;
            break;
        }
    }
    
    return 0;
}

int antenna_fullwave_compute_return_loss(AntennaFullwaveSolver* solver, double frequency) {
    if (!solver) return -1;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        if (fabs(solver->frequency_vector[i] - frequency) < 1e-6) {
            Complex gamma = solver->params[i].gamma;
            double return_loss = -20.0 * log10(cabs(gamma));
            
            solver->params[i].return_loss = return_loss;
            break;
        }
    }
    
    return 0;
}

int antenna_fullwave_compute_directivity(AntennaFullwaveSolver* solver) {
    if (!solver) return -1;
    
    double max_power_density = 0.0;
    double total_power = 0.0;
    
    int theta_points = solver->config.num_far_field_angles;
    int phi_points = solver->config.num_far_field_angles;
    
    for (int i = 0; i < theta_points; i++) {
        for (int j = 0; j < phi_points; j++) {
            double power_density = solver->far_field[i][j].power_density;
            
            if (power_density > max_power_density) {
                max_power_density = power_density;
            }
            
            double dtheta = M_PI / (theta_points - 1);
            double dphi = 2.0 * M_PI / (phi_points - 1);
            double sin_theta = sin(solver->far_field[i][j].theta);
            
            total_power += power_density * sin_theta * dtheta * dphi;
        }
    }
    
    double directivity = 4.0 * M_PI * max_power_density / total_power;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        solver->params[i].directivity = 10.0 * log10(directivity);
    }
    
    return 0;
}

int antenna_fullwave_solve_frequency(AntennaFullwaveSolver* solver, double frequency) {
    if (!solver) return -1;
    
    double start_time = get_time_seconds();
    
    int status = antenna_fullwave_compute_impedance_matrix(solver, frequency);
    if (status != 0) return status;
    
    status = antenna_fullwave_compute_voltage_vector(solver, frequency);
    if (status != 0) return status;
    
    status = antenna_fullwave_solve_current_distribution(solver);
    if (status != 0) return status;
    
    status = antenna_fullwave_compute_input_impedance(solver, frequency);
    if (status != 0) return status;
    
    status = antenna_fullwave_compute_vswr(solver, frequency);
    if (status != 0) return status;
    
    status = antenna_fullwave_compute_return_loss(solver, frequency);
    if (status != 0) return status;
    
    status = antenna_fullwave_compute_far_field(solver, frequency);
    if (status != 0) return status;
    
    status = antenna_fullwave_compute_directivity(solver);
    if (status != 0) return status;
    
    double end_time = get_time_seconds();
    solver->computation_time = end_time - start_time;
    solver->simulation_completed = 1;
    
    return 0;
}

int antenna_fullwave_solve_frequency_sweep(AntennaFullwaveSolver* solver) {
    if (!solver) return -1;
    
    solver->num_frequencies = 0;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        double frequency = solver->frequency_vector[i];
        
        int status = antenna_fullwave_solve_frequency(solver, frequency);
        if (status == 0) {
            solver->num_frequencies++;
        }
    }
    
    return (solver->num_frequencies > 0) ? 0 : -1;
}

int antenna_fullwave_simulate(AntennaFullwaveSolver* solver) {
    if (!solver) return -1;
    
    return antenna_fullwave_solve_frequency_sweep(solver);
}

void antenna_fullwave_print_summary(AntennaFullwaveSolver* solver) {
    if (!solver) return;
    
    printf("\n=== Antenna Full-Wave Simulation Summary ===\n");
    printf("Antenna Type: %d\n", solver->config.type);
    printf("Number of Segments: %d\n", solver->num_segments);
    printf("Frequency Range: %.3f MHz - %.3f MHz\n", 
           solver->config.frequency_min / 1e6, 
           solver->config.frequency_max / 1e6);
    printf("Number of Frequencies: %d\n", solver->config.num_frequencies);
    printf("Computation Time: %.3f seconds\n", solver->computation_time);
    printf("Memory Usage: %.2f MB\n", solver->memory_usage / (1024.0 * 1024.0));
    printf("Convergence Status: %s\n", solver->convergence_status ? "Converged" : "Not Converged");
    
    if (solver->num_frequencies > 0) {
        printf("\nFrequency Response:\n");
        for (int i = 0; i < solver->num_frequencies; i++) {
            double freq = solver->frequency_vector[i];
            Complex zin = solver->params[i].zin;
            double vswr = solver->params[i].vswr;
            double return_loss = solver->params[i].return_loss;
            double directivity = solver->params[i].directivity;
            
            printf("  %.1f MHz: Zin=%.1f%+.1fj Ω, VSWR=%.2f, RL=%.1f dB, D=%.1f dBi\n",
                   freq / 1e6, creal(zin), cimag(zin), vswr, return_loss, directivity);
        }
    }
    
    printf("\n");
}

void antenna_fullwave_benchmark(AntennaFullwaveSolver* solver) {
    if (!solver) return;
    
    printf("\n=== Antenna Full-Wave Benchmark ===\n");
    
    double start_time = get_time_seconds();
    
    int status = antenna_fullwave_simulate(solver);
    
    double end_time = get_time_seconds();
    double total_time = end_time - start_time;
    
    printf("Simulation Status: %s\n", (status == 0) ? "SUCCESS" : "FAILED");
    printf("Total Time: %.3f seconds\n", total_time);
    printf("Time per Frequency: %.3f ms\n", (total_time / solver->config.num_frequencies) * 1000.0);
    printf("Segments per Second: %.0f\n", solver->num_segments / total_time);
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