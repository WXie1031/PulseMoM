#include "metamaterial_extraction.h"
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

MetamaterialExtractionSolver* metamaterial_extraction_create(MetamaterialExtractionConfig* config) {
    if (!config) return NULL;
    
    MetamaterialExtractionSolver* solver = (MetamaterialExtractionSolver*)calloc(1, sizeof(MetamaterialExtractionSolver));
    if (!solver) return NULL;
    
    memcpy(&solver->config, config, sizeof(MetamaterialExtractionConfig));
    
    solver->num_unit_cell_types = 0;
    solver->extraction_completed = 0;
    solver->computation_time = 0.0;
    solver->memory_usage = 0.0;
    solver->convergence_status = 0;
    solver->matrix_size = 0;
    
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

void metamaterial_extraction_destroy(MetamaterialExtractionSolver* solver) {
    if (!solver) return;
    
    if (solver->frequency_vector) free(solver->frequency_vector);
    if (solver->scattering_matrix) free(solver->scattering_matrix);
    if (solver->impedance_matrix) free(solver->impedance_matrix);
    if (solver->admittance_matrix) free(solver->admittance_matrix);
    if (solver->bloch_vector) free(solver->bloch_vector);
    if (solver->mom_solver) mom_solver_destroy(solver->mom_solver);
    
    free(solver);
}

Complex metamaterial_nicolson_ross_weir_epsilon(SParameters* s_params, double thickness, double frequency) {
    if (!s_params) return complex_create(0.0, 0.0);
    
    double omega = 2.0 * M_PI * frequency;
    double k0 = omega * sqrt(MU_0 * EPSILON_0);
    
    Complex s11 = s_params->s11;
    Complex s21 = s_params->s21;
    
    Complex v1 = complex_add(s11, s21);
    Complex v2 = complex_subtract(s11, s21);
    
    Complex one = complex_create(1.0, 0.0);
    Complex two = complex_create(2.0, 0.0);
    
    Complex gamma = complex_acosh(complex_divide(complex_subtract(one, complex_multiply(v1, v2)), two));
    
    Complex exp_gamma_d = complex_exp(complex_multiply(gamma, complex_create(-thickness, 0.0)));
    
    Complex numerator = complex_subtract(complex_multiply(v1, exp_gamma_d), v2);
    Complex denominator = complex_subtract(exp_gamma_d, one);
    
    Complex z = complex_divide(numerator, denominator);
    
    Complex epsilon = complex_divide(complex_multiply(gamma, z), complex_create(0.0, k0));
    
    return epsilon;
}

Complex metamaterial_nicolson_ross_weir_mu(SParameters* s_params, double thickness, double frequency) {
    if (!s_params) return complex_create(0.0, 0.0);
    
    double omega = 2.0 * M_PI * frequency;
    double k0 = omega * sqrt(MU_0 * EPSILON_0);
    
    Complex s11 = s_params->s11;
    Complex s21 = s_params->s21;
    
    Complex v1 = complex_add(s11, s21);
    Complex v2 = complex_subtract(s11, s21);
    
    Complex one = complex_create(1.0, 0.0);
    Complex two = complex_create(2.0, 0.0);
    
    Complex gamma = complex_acosh(complex_divide(complex_subtract(one, complex_multiply(v1, v2)), two));
    
    Complex exp_gamma_d = complex_exp(complex_multiply(gamma, complex_create(-thickness, 0.0)));
    
    Complex numerator = complex_subtract(complex_multiply(v1, exp_gamma_d), v2);
    Complex denominator = complex_subtract(exp_gamma_d, one);
    
    Complex z = complex_divide(numerator, denominator);
    
    Complex mu = complex_divide(complex_multiply(gamma, one), complex_multiply(complex_create(0.0, k0), z));
    
    return mu;
}

int metamaterial_setup_srr(MetamaterialExtractionSolver* solver, double outer_radius, double inner_radius, double gap_size) {
    if (!solver) return -1;
    
    solver->config.type = METAMATERIAL_TYPE_SRR;
    
    UnitCellGeometry* geom = &solver->config.geometry;
    geom->outer_radius = outer_radius;
    geom->inner_radius = inner_radius;
    geom->gap_size = gap_size;
    
    geom->length = 4.0 * outer_radius;
    geom->width = 4.0 * outer_radius;
    geom->height = outer_radius - inner_radius;
    
    solver->num_unit_cell_types = 1;
    
    return 0;
}

int metamaterial_compute_s_parameters(MetamaterialExtractionSolver* solver, double frequency) {
    if (!solver) return -1;
    
    double omega = 2.0 * M_PI * frequency;
    double k0 = omega * sqrt(MU_0 * EPSILON_0);
    
    int freq_idx = -1;
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        if (fabs(solver->frequency_vector[i] - frequency) < 1e-6) {
            freq_idx = i;
            break;
        }
    }
    
    if (freq_idx < 0) return -1;
    
    SParameters* s_params = &solver->s_params[freq_idx];
    s_params->frequency = frequency;
    
    Complex z_load = complex_create(50.0, 0.0);
    Complex z_source = complex_create(50.0, 0.0);
    
    double thickness = solver->config.geometry.thickness;
    
    Complex epsilon_eff = solver->effective_params[freq_idx].epsilon_xx;
    Complex mu_eff = solver->effective_params[freq_idx].mu_xx;
    
    Complex gamma = complex_sqrt(complex_multiply(epsilon_eff, mu_eff));
    gamma = complex_multiply(gamma, complex_create(0.0, k0));
    
    Complex eta = complex_sqrt(complex_divide(mu_eff, epsilon_eff));
    
    Complex z_in = complex_multiply(eta, complex_tanh(complex_multiply(gamma, complex_create(thickness, 0.0))));
    
    Complex gamma_load = complex_divide(complex_subtract(z_load, eta), complex_add(z_load, eta));
    Complex gamma_source = complex_divide(complex_subtract(z_in, z_source), complex_add(z_in, z_source));
    
    s_params->s11 = gamma_source;
    s_params->s21 = complex_divide(complex_multiply(complex_create(1.0 + creal(gamma_source), 0.0), 
                                                   complex_exp(complex_multiply(gamma, complex_create(-thickness, 0.0)))),
                                  complex_create(1.0 + creal(gamma_source) * creal(gamma_load), 0.0));
    
    s_params->s12 = s_params->s21;
    s_params->s22 = s_params->s11;
    
    double power_incident = 1.0;
    double power_reflected = cabs(s_params->s11) * cabs(s_params->s11);
    double power_transmitted = cabs(s_params->s21) * cabs(s_params->s21);
    
    s_params->power_incident = power_incident;
    s_params->power_reflected = power_reflected;
    s_params->power_transmitted = power_transmitted;
    
    return 0;
}

int metamaterial_extract_epsilon_mu_nrw(MetamaterialExtractionSolver* solver, SParameters* s_params, int freq_idx) {
    if (!solver || !s_params || freq_idx < 0 || freq_idx >= solver->config.num_frequencies) return -1;
    
    double frequency = solver->frequency_vector[freq_idx];
    double thickness = solver->config.geometry.thickness;
    
    Complex epsilon = metamaterial_nicolson_ross_weir_epsilon(s_params, thickness, frequency);
    Complex mu = metamaterial_nicolson_ross_weir_mu(s_params, thickness, frequency);
    
    solver->effective_params[freq_idx].epsilon_xx = epsilon;
    solver->effective_params[freq_idx].epsilon_yy = epsilon;
    solver->effective_params[freq_idx].epsilon_zz = epsilon;
    
    solver->effective_params[freq_idx].mu_xx = mu;
    solver->effective_params[freq_idx].mu_yy = mu;
    solver->effective_params[freq_idx].mu_zz = mu;
    
    solver->effective_params[freq_idx].is_anisotropic = 0;
    solver->effective_params[freq_idx].is_bianisotropic = 0;
    solver->effective_params[freq_idx].is_chiral = 0;
    
    solver->config.epsilon_eff[freq_idx] = epsilon;
    solver->config.mu_eff[freq_idx] = mu;
    
    return 0;
}

int metamaterial_compute_resonance_frequency(MetamaterialExtractionSolver* solver) {
    if (!solver) return -1;
    
    double min_epsilon_real = 1e10;
    double max_mu_real = -1e10;
    double resonance_freq = 0.0;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        double freq = solver->frequency_vector[i];
        Complex epsilon = solver->config.epsilon_eff[i];
        Complex mu = solver->config.mu_eff[i];
        
        double epsilon_real = creal(epsilon);
        double mu_real = creal(mu);
        
        if (epsilon_real < min_epsilon_real) {
            min_epsilon_real = epsilon_real;
            resonance_freq = freq;
        }
        
        if (mu_real > max_mu_real) {
            max_mu_real = mu_real;
        }
    }
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        solver->metrics[i].resonance_frequency = resonance_freq;
    }
    
    return 0;
}

int metamaterial_compute_quality_factor(MetamaterialExtractionSolver* solver) {
    if (!solver) return -1;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        double freq = solver->frequency_vector[i];
        Complex epsilon = solver->config.epsilon_eff[i];
        Complex mu = solver->config.mu_eff[i];
        
        double epsilon_real = creal(epsilon);
        double epsilon_imag = cimag(epsilon);
        double mu_real = creal(mu);
        double mu_imag = cimag(mu);
        
        double q_epsilon = epsilon_real / fabs(epsilon_imag);
        double q_mu = mu_real / fabs(mu_imag);
        double q_avg = (q_epsilon + q_mu) / 2.0;
        
        solver->metrics[i].quality_factor = q_avg;
        
        double bandwidth = freq / q_avg;
        solver->metrics[i].bandwidth = bandwidth;
    }
    
    return 0;
}

int metamaterial_solve_frequency(MetamaterialExtractionSolver* solver, double frequency) {
    if (!solver) return -1;
    
    double start_time = get_time_seconds();
    
    int freq_idx = -1;
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        if (fabs(solver->frequency_vector[i] - frequency) < 1e-6) {
            freq_idx = i;
            break;
        }
    }
    
    if (freq_idx < 0) return -1;
    
    int status = metamaterial_compute_s_parameters(solver, frequency);
    if (status != 0) return status;
    
    SParameters* s_params = &solver->s_params[freq_idx];
    
    status = metamaterial_extract_epsilon_mu_nrw(solver, s_params, freq_idx);
    if (status != 0) return status;
    
    double end_time = get_time_seconds();
    solver->computation_time = end_time - start_time;
    solver->extraction_completed = 1;
    
    return 0;
}

int metamaterial_solve_frequency_sweep(MetamaterialExtractionSolver* solver) {
    if (!solver) return -1;
    
    for (int i = 0; i < solver->config.num_frequencies; i++) {
        double frequency = solver->frequency_vector[i];
        
        int status = metamaterial_solve_frequency(solver, frequency);
        if (status != 0) {
            printf("Error: Failed to extract parameters at frequency %.3e Hz\n", frequency);
            return status;
        }
    }
    
    metamaterial_compute_resonance_frequency(solver);
    metamaterial_compute_quality_factor(solver);
    
    return 0;
}

int metamaterial_extraction_simulate(MetamaterialExtractionSolver* solver) {
    if (!solver) return -1;
    
    return metamaterial_solve_frequency_sweep(solver);
}

void metamaterial_print_summary(MetamaterialExtractionSolver* solver) {
    if (!solver) return;
    
    printf("\n=== Metamaterial Parameter Extraction Summary ===\n");
    printf("Metamaterial Type: %d\n", solver->config.type);
    printf("Number of Unit Cell Types: %d\n", solver->num_unit_cell_types);
    printf("Frequency Range: %.3f MHz - %.3f MHz\n", 
           solver->config.frequency_min / 1e6, 
           solver->config.frequency_max / 1e6);
    printf("Number of Frequencies: %d\n", solver->config.num_frequencies);
    printf("Extraction Method: %d\n", solver->config.extraction_method);
    printf("Computation Time: %.3f seconds\n", solver->computation_time);
    printf("Memory Usage: %.2f MB\n", solver->memory_usage / (1024.0 * 1024.0));
    printf("Convergence Status: %s\n", solver->convergence_status ? "Converged" : "Not Converged");
    
    if (solver->config.num_frequencies > 0) {
        printf("\nEffective Parameters at Sample Frequencies:\n");
        for (int i = 0; i < solver->config.num_frequencies; i += solver->config.num_frequencies / 5) {
            double freq = solver->frequency_vector[i];
            Complex epsilon = solver->config.epsilon_eff[i];
            Complex mu = solver->config.mu_eff[i];
            double resonance = solver->metrics[i].resonance_frequency;
            double q_factor = solver->metrics[i].quality_factor;
            
            printf("  %.1f MHz: ε=%.2f%+.2fj, μ=%.2f%+.2fj, fres=%.1f MHz, Q=%.1f\n",
                   freq / 1e6, creal(epsilon), cimag(epsilon), creal(mu), cimag(mu), 
                   resonance / 1e6, q_factor);
        }
    }
    
    printf("\n");
}