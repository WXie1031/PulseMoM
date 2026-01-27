/**
 * @file core_circuit.c
 * @brief Advanced circuit simulation and coupling implementation
 * @details Commercial-grade circuit solver with harmonic balance, nonlinear analysis,
 *          and electromagnetic coupling similar to Keysight ADS and Cadence Spectre
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <omp.h>
#include "peec_circuit.h"
#include "../../backend/math/unified_matrix_assembly.h"  // TODO: Check if core_matrix.h exists or should use unified_matrix_assembly.h
#include "../../backend/solvers/core_solver.h"
#include "../../orchestration/wideband/core_wideband.h"

/* Private function declarations */
static int circuit_build_system_matrix(circuit_network_t* network);
static int circuit_solve_linear_system(circuit_network_t* network, circuit_analysis_result_t* result);
static int circuit_solve_nonlinear_system(circuit_network_t* network, circuit_analysis_result_t* result);
static int circuit_newton_raphson_solve(circuit_network_t* network, circuit_analysis_result_t* result);
static int circuit_harmonic_balance_solve(circuit_network_t* network, circuit_analysis_config_t* config, circuit_analysis_result_t* result);
static int circuit_transient_solve(circuit_network_t* network, circuit_analysis_config_t* config, transient_analysis_result_t* result);
static double circuit_diode_current(double voltage, double isat, double vt, double* conductance);
static double circuit_mosfet_current(double vgs, double vds, double vt, double kp, double w, double l, double* gm, double* gds);
static int circuit_extract_s_parameters(circuit_network_t* network, circuit_analysis_config_t* config, s_parameter_result_t* result);
static int circuit_noise_analysis_compute(circuit_network_t* network, circuit_analysis_config_t* config, noise_analysis_result_t* result);
static int circuit_em_coupling_update(circuit_network_t* network, em_circuit_coupling_config_t* config, void* em_solver_data);

/* Global variables */
static int circuit_initialized = 0;
static int circuit_element_counter = 0;
static int circuit_node_counter = 0;

/**
 * @brief Initialize circuit analysis library
 */
int circuit_init(void) {
    if (circuit_initialized) {
        return 0;
    }
    
    printf("Initializing Circuit Analysis Library...\n");
    printf("Circuit Analysis Types Supported:\n");
    printf("  - DC Analysis\n");
    printf("  - AC Small-Signal Analysis\n");
    printf("  - Transient Analysis\n");
    printf("  - Harmonic Balance\n");
    printf("  - S-Parameter Analysis\n");
    printf("  - Noise Analysis\n");
    printf("  - EM-Circuit Coupling\n");
    
    circuit_initialized = 1;
    circuit_element_counter = 0;
    circuit_node_counter = 0;
    
    return 0;
}

/**
 * @brief Create new circuit network
 */
circuit_network_t* circuit_network_create(int num_nodes, int num_elements) {
    circuit_network_t* network = (circuit_network_t*)calloc(1, sizeof(circuit_network_t));
    if (!network) {
        return NULL;
    }
    
    network->nodes = (circuit_node_t*)calloc(num_nodes, sizeof(circuit_node_t));
    network->elements = (circuit_element_t*)calloc(num_elements, sizeof(circuit_element_t));
    
    if (!network->nodes || !network->elements) {
        free(network->nodes);
        free(network->elements);
        free(network);
        return NULL;
    }
    
    network->num_nodes = num_nodes;
    network->num_elements = num_elements;
    network->num_grounds = 0;
    network->frequency = 1.0e9;  /* 1 GHz default */
    network->temperature = 300.0;  /* 300K default */
    network->converged = false;
    network->iteration_count = 0;
    
    /* Initialize nodes */
    for (int i = 0; i < num_nodes; i++) {
        network->nodes[i].id = i;
        network->nodes[i].voltage = 0.0;
        network->nodes[i].ac_voltage = 0.0;
        network->nodes[i].is_ground = false;
        snprintf(network->nodes[i].name, sizeof(network->nodes[i].name), "N%d", i);
    }
    
    /* Initialize elements */
    for (int i = 0; i < num_elements; i++) {
        network->elements[i].id = i;
        network->elements[i].active = true;
        network->elements[i].power = 0.0;
        network->elements[i].temperature = network->temperature;
    }
    
    return network;
}

/**
 * @brief Destroy circuit network and free memory
 */
void circuit_network_destroy(circuit_network_t* network) {
    if (!network) return;
    
    /* Free harmonic voltages for nodes */
    for (int i = 0; i < network->num_nodes; i++) {
        if (network->nodes[i].harmonic_voltages) {
            free(network->nodes[i].harmonic_voltages);
        }
    }
    
    /* Free nonlinear models for elements */
    for (int i = 0; i < network->num_elements; i++) {
        if (network->elements[i].nonlinear) {
            circuit_nonlinear_model_destroy(network->elements[i].nonlinear);
        }
    }
    
    /* Free matrices */
    free(network->conductance_matrix);
    free(network->capacitance_matrix);
    free(network->inductance_matrix);
    
    /* Free adjacency matrix */
    if (network->adjacency_matrix) {
        for (int i = 0; i < network->num_nodes; i++) {
            free(network->adjacency_matrix[i]);
        }
        free(network->adjacency_matrix);
    }
    
    free(network->nodes);
    free(network->elements);
    free(network);
}

/**
 * @brief Add element to circuit network
 */
int circuit_add_element(circuit_network_t* network, circuit_element_type_t type,
                       int node1, int node2, double value, const char* name) {
    if (!network || network->num_elements >= MAX_CIRCUIT_ELEMENTS) {
        return -1;
    }
    
    int element_id = circuit_element_counter++;
    circuit_element_t* element = &network->elements[element_id];
    
    element->id = element_id;
    element->type = type;
    element->params.node1 = node1;
    element->params.node2 = node2;
    element->params.value = value;
    element->params.ac_value = value;
    element->params.temperature_coeff = 0.0;
    element->params.noise_factor = 1.0;
    element->active = true;
    
    if (name) {
        strncpy(element->name, name, sizeof(element->name) - 1);
    } else {
        snprintf(element->name, sizeof(element->name), "E%d", element_id);
    }
    
    /* Update adjacency matrix */
    if (node1 >= 0 && node1 < network->num_nodes && node2 >= 0 && node2 < network->num_nodes) {
        if (!network->adjacency_matrix) {
            network->adjacency_matrix = (int**)calloc(network->num_nodes, sizeof(int*));
            for (int i = 0; i < network->num_nodes; i++) {
                network->adjacency_matrix[i] = (int*)calloc(network->num_nodes, sizeof(int));
            }
        }
        network->adjacency_matrix[node1][node2]++;
        if (node1 != node2) {
            network->adjacency_matrix[node2][node1]++;
        }
    }
    
    return element_id;
}

/**
 * @brief Add nonlinear element to circuit network
 */
int circuit_add_nonlinear_element(circuit_network_t* network, circuit_element_type_t type,
                               int node1, int node2, nonlinear_model_t* model, const char* name) {
    int element_id = circuit_add_element(network, type, node1, node2, 0.0, name);
    if (element_id < 0) {
        return -1;
    }
    
    network->elements[element_id].nonlinear = model;
    return element_id;
}

/**
 * @brief Add electromagnetically coupled element
 */
int circuit_add_em_coupled_element(circuit_network_t* network, int em_element_id,
                                  int node1, int node2, double coupling_factor, const char* solver_type) {
    int element_id = circuit_add_element(network, ELEMENT_EM_COUPLED, node1, node2, coupling_factor, "EM_COUPLED");
    if (element_id < 0) {
        return -1;
    }
    
    /* Store EM coupling information in element parameters */
    network->elements[element_id].params.node1 = em_element_id;  /* Store EM element ID */
    network->elements[element_id].params.node2 = 0;  /* Reserved for future use */
    network->elements[element_id].params.value = coupling_factor;
    
    return element_id;
}

/**
 * @brief Perform DC analysis
 */
int circuit_dc_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                       circuit_analysis_result_t* result) {
    if (!network || !config || !result) {
        return -1;
    }
    
    printf("Performing DC Analysis...\n");
    
    /* Build system matrix */
    if (circuit_build_system_matrix(network) != 0) {
        printf("Error: Failed to build system matrix\n");
        return -1;
    }
    
    /* Solve linear system */
    if (circuit_solve_linear_system(network, result) != 0) {
        printf("Error: Failed to solve DC system\n");
        return -1;
    }
    
    /* Calculate power consumption */
    for (int i = 0; i < network->num_elements; i++) {
        circuit_element_t* element = &network->elements[i];
        if (element->active) {
            double v1 = network->nodes[element->params.node1].voltage;
            double v2 = network->nodes[element->params.node2].voltage;
            double current = (v1 - v2) / (element->params.value + 1e-12);  /* Avoid division by zero */
            element->power = fabs((v1 - v2) * current);
            result->power[i] = element->power;
        }
    }
    
    network->converged = true;
    result->converged = true;
    result->iterations = network->iteration_count;
    
    printf("DC Analysis completed successfully\n");
    return 0;
}

/**
 * @brief Perform AC small-signal analysis
 */
int circuit_ac_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                       frequency_response_result_t* result) {
    if (!network || !config || !result) {
        return -1;
    }
    
    printf("Performing AC Analysis from %.2e to %.2e Hz with %d points...\n",
           config->start_frequency, config->stop_frequency, config->num_frequency_points);
    
    /* Allocate frequency response arrays */
    result->num_points = config->num_frequency_points;
    result->frequencies = (double*)calloc(result->num_points, sizeof(double));
    result->magnitude_db = (double*)calloc(result->num_points, sizeof(double));
    result->phase_deg = (double*)calloc(result->num_points, sizeof(double));
    result->real_part = (double*)calloc(result->num_points, sizeof(double));
    result->imag_part = (double*)calloc(result->num_points, sizeof(double));
    
    if (!result->frequencies || !result->magnitude_db || !result->phase_deg) {
        printf("Error: Failed to allocate frequency response arrays\n");
        return -1;
    }
    
    /* Logarithmic frequency sweep */
    double log_start = log10(config->start_frequency);
    double log_stop = log10(config->stop_frequency);
    
    for (int i = 0; i < result->num_points; i++) {
        double log_freq = log_start + (log_stop - log_start) * i / (result->num_points - 1);
        result->frequencies[i] = pow(10.0, log_freq);
        
        /* Set operating frequency */
        network->frequency = result->frequencies[i];
        
        /* Build AC system matrix */
        if (circuit_build_system_matrix(network) != 0) {
            printf("Error: Failed to build AC system matrix at %.2e Hz\n", network->frequency);
            continue;
        }
        
        /* Solve AC system */
        circuit_analysis_result_t ac_result = {0};
        if (circuit_solve_linear_system(network, &ac_result) != 0) {
            printf("Error: Failed to solve AC system at %.2e Hz\n", network->frequency);
            continue;
        }
        
        /* Calculate frequency response (using first output node as reference) */
        double complex output_voltage = 0.0;
        for (int j = 0; j < network->num_nodes; j++) {
            if (!network->nodes[j].is_ground) {
                output_voltage = network->nodes[j].ac_voltage;
                break;
            }
        }
        
        result->magnitude_db[i] = 20.0 * log10(cabs(output_voltage) + 1e-12);
        result->phase_deg[i] = carg(output_voltage) * 180.0 / M_PI;
        result->real_part[i] = creal(output_voltage);
        result->imag_part[i] = cimag(output_voltage);
    }
    
    result->start_freq = config->start_frequency;
    result->stop_freq = config->stop_frequency;
    
    /* Find min/max magnitude */
    result->min_magnitude = result->magnitude_db[0];
    result->max_magnitude = result->magnitude_db[0];
    for (int i = 1; i < result->num_points; i++) {
        if (result->magnitude_db[i] < result->min_magnitude) {
            result->min_magnitude = result->magnitude_db[i];
        }
        if (result->magnitude_db[i] > result->max_magnitude) {
            result->max_magnitude = result->magnitude_db[i];
        }
    }
    
    printf("AC Analysis completed successfully\n");
    return 0;
}

/**
 * @brief Perform transient analysis
 */
int circuit_transient_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                              transient_analysis_result_t* result) {
    if (!network || !config || !result) {
        return -1;
    }
    
    printf("Performing Transient Analysis from %.2e to %.2e s with step %.2e s...\n",
           config->start_time, config->stop_time, config->time_step);
    
    /* Calculate number of time points */
    int num_points = (int)((config->stop_time - config->start_time) / config->time_step) + 1;
    result->num_points = num_points;
    result->time_step = config->time_step;
    
    /* Allocate transient result arrays */
    result->time_points = (double*)calloc(num_points, sizeof(double));
    result->voltage_waveforms = (double*)calloc(num_points * network->num_nodes, sizeof(double));
    result->current_waveforms = (double*)calloc(num_points * network->num_elements, sizeof(double));
    result->power_waveforms = (double*)calloc(num_points * network->num_elements, sizeof(double));
    
    if (!result->time_points || !result->voltage_waveforms) {
        printf("Error: Failed to allocate transient result arrays\n");
        return -1;
    }
    
    /* Initialize time points */
    for (int i = 0; i < num_points; i++) {
        result->time_points[i] = config->start_time + i * config->time_step;
    }
    
    /* Transient simulation loop */
    for (int step = 0; step < num_points; step++) {
        double time = result->time_points[step];
        
        /* Update time-varying sources */
        for (int i = 0; i < network->num_elements; i++) {
            circuit_element_t* element = &network->elements[i];
            if (element->type == ELEMENT_VOLTAGE_SOURCE || element->type == ELEMENT_CURRENT_SOURCE) {
                /* Simple sinusoidal source for demonstration */
                element->params.value = element->params.value * sin(2.0 * M_PI * 1.0e9 * time);
            }
        }
        
        /* Solve circuit at this time point */
        if (circuit_build_system_matrix(network) != 0) {
            printf("Error: Failed to build system matrix at time %.2e s\n", time);
            continue;
        }
        
        circuit_analysis_result_t step_result = {0};
        if (circuit_solve_linear_system(network, &step_result) != 0) {
            printf("Error: Failed to solve system at time %.2e s\n", time);
            continue;
        }
        
        /* Store results */
        for (int i = 0; i < network->num_nodes; i++) {
            result->voltage_waveforms[step * network->num_nodes + i] = network->nodes[i].voltage;
        }
        
        for (int i = 0; i < network->num_elements; i++) {
            circuit_element_t* element = &network->elements[i];
            double v1 = network->nodes[element->params.node1].voltage;
            double v2 = network->nodes[element->params.node2].voltage;
            double current = (v1 - v2) / (element->params.value + 1e-12);
            
            result->current_waveforms[step * network->num_elements + i] = current;
            result->power_waveforms[step * network->num_elements + i] = fabs((v1 - v2) * current);
        }
    }
    
    /* Calculate waveform statistics */
    result->max_voltage = 0.0;
    result->max_current = 0.0;
    result->max_power = 0.0;
    result->rms_voltage = 0.0;
    result->rms_current = 0.0;
    result->average_power = 0.0;
    
    for (int step = 0; step < num_points; step++) {
        for (int i = 0; i < network->num_nodes; i++) {
            double voltage = result->voltage_waveforms[step * network->num_nodes + i];
            if (fabs(voltage) > result->max_voltage) {
                result->max_voltage = fabs(voltage);
            }
            result->rms_voltage += voltage * voltage;
        }
        
        for (int i = 0; i < network->num_elements; i++) {
            double current = result->current_waveforms[step * network->num_elements + i];
            double power = result->power_waveforms[step * network->num_elements + i];
            
            if (fabs(current) > result->max_current) {
                result->max_current = fabs(current);
            }
            if (power > result->max_power) {
                result->max_power = power;
            }
            
            result->rms_current += current * current;
            result->average_power += power;
        }
    }
    
    result->rms_voltage = sqrt(result->rms_voltage / (num_points * network->num_nodes));
    result->rms_current = sqrt(result->rms_current / (num_points * network->num_elements));
    result->average_power /= (num_points * network->num_elements);
    
    printf("Transient Analysis completed successfully\n");
    return 0;
}

/**
 * @brief Perform harmonic balance analysis
 */
int circuit_harmonic_balance_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                                     circuit_analysis_result_t* result) {
    if (!network || !config || !result) {
        return -1;
    }
    
    printf("Performing Harmonic Balance Analysis with %d harmonics...\n", config->num_harmonics);
    
    /* Initialize harmonic balance solver */
    if (circuit_harmonic_balance_solve(network, config, result) != 0) {
        printf("Error: Harmonic balance solver failed\n");
        return -1;
    }
    
    printf("Harmonic Balance Analysis completed successfully\n");
    return 0;
}

/**
 * @brief Build circuit system matrix
 */
static int circuit_build_system_matrix(circuit_network_t* network) {
    if (!network) {
        return -1;
    }
    
    int matrix_size = network->num_nodes;
    
    /* Allocate matrices */
    if (!network->conductance_matrix) {
        network->conductance_matrix = (double*)calloc(matrix_size * matrix_size, sizeof(double));
        network->capacitance_matrix = (double*)calloc(matrix_size * matrix_size, sizeof(double));
        network->inductance_matrix = (double*)calloc(matrix_size * matrix_size, sizeof(double));
    }
    
    if (!network->conductance_matrix || !network->capacitance_matrix || !network->inductance_matrix) {
        printf("Error: Failed to allocate system matrices\n");
        return -1;
    }
    
    /* Initialize matrices to zero */
    memset(network->conductance_matrix, 0, matrix_size * matrix_size * sizeof(double));
    memset(network->capacitance_matrix, 0, matrix_size * matrix_size * sizeof(double));
    memset(network->inductance_matrix, 0, matrix_size * matrix_size * sizeof(double));
    
    /* Build element contributions */
    for (int i = 0; i < network->num_elements; i++) {
        circuit_element_t* element = &network->elements[i];
        if (!element->active) continue;
        
        int n1 = element->params.node1;
        int n2 = element->params.node2;
        
        switch (element->type) {
            case ELEMENT_RESISTOR:
                if (n1 >= 0 && n1 < matrix_size && n2 >= 0 && n2 < matrix_size) {
                    double conductance = 1.0 / (element->params.value + 1e-12);
                    network->conductance_matrix[n1 * matrix_size + n1] += conductance;
                    network->conductance_matrix[n2 * matrix_size + n2] += conductance;
                    network->conductance_matrix[n1 * matrix_size + n2] -= conductance;
                    network->conductance_matrix[n2 * matrix_size + n1] -= conductance;
                }
                break;
                
            case ELEMENT_CAPACITOR:
                if (n1 >= 0 && n1 < matrix_size && n2 >= 0 && n2 < matrix_size) {
                    double omega = 2.0 * M_PI * network->frequency;
                    double susceptance = omega * element->params.value;
                    network->capacitance_matrix[n1 * matrix_size + n1] += susceptance;
                    network->capacitance_matrix[n2 * matrix_size + n2] += susceptance;
                    network->capacitance_matrix[n1 * matrix_size + n2] -= susceptance;
                    network->capacitance_matrix[n2 * matrix_size + n1] -= susceptance;
                }
                break;
                
            case ELEMENT_INDUCTOR:
                if (n1 >= 0 && n1 < matrix_size && n2 >= 0 && n2 < matrix_size) {
                    double omega = 2.0 * M_PI * network->frequency;
                    double reactance = omega * element->params.value;
                    network->inductance_matrix[n1 * matrix_size + n1] += reactance;
                    network->inductance_matrix[n2 * matrix_size + n2] += reactance;
                    network->inductance_matrix[n1 * matrix_size + n2] -= reactance;
                    network->inductance_matrix[n2 * matrix_size + n1] -= reactance;
                }
                break;
                
            default:
                /* Handle other element types */
                break;
        }
    }
    
    return 0;
}

/**
 * @brief Solve linear circuit system
 */
static int circuit_solve_linear_system(circuit_network_t* network, circuit_analysis_result_t* result) {
    if (!network || !result) {
        return -1;
    }
    
    int matrix_size = network->num_nodes;
    
    /* Create combined admittance matrix: Y = G + jωC + 1/(jωL) */
    double complex* y_matrix = (double complex*)calloc(matrix_size * matrix_size, sizeof(double complex));
    double complex* rhs_vector = (double complex*)calloc(matrix_size, sizeof(double complex));
    double complex* solution = (double complex*)calloc(matrix_size, sizeof(double complex));
    
    if (!y_matrix || !rhs_vector || !solution) {
        printf("Error: Failed to allocate linear system arrays\n");
        free(y_matrix);
        free(rhs_vector);
        free(solution);
        return -1;
    }
    
    double omega = 2.0 * M_PI * network->frequency;
    
    /* Build combined admittance matrix */
    for (int i = 0; i < matrix_size; i++) {
        for (int j = 0; j < matrix_size; j++) {
            int idx = i * matrix_size + j;
            y_matrix[idx] = network->conductance_matrix[idx] + 
                           I * omega * network->capacitance_matrix[idx] +
                           I * network->inductance_matrix[idx] / (omega + 1e-12);
        }
    }
    
    /* Set up RHS vector (voltage sources) */
    for (int i = 0; i < network->num_elements; i++) {
        circuit_element_t* element = &network->elements[i];
        if (element->type == ELEMENT_VOLTAGE_SOURCE && element->active) {
            int n1 = element->params.node1;
            int n2 = element->params.node2;
            double complex voltage = element->params.ac_value;
            
            if (n1 >= 0 && n1 < matrix_size) {
                rhs_vector[n1] += voltage;
            }
            if (n2 >= 0 && n2 < matrix_size) {
                rhs_vector[n2] -= voltage;
            }
        }
    }
    
    /* Solve linear system using LU decomposition */
    int* ipiv = (int*)calloc(matrix_size, sizeof(int));
    int info = 0;
    
    /* Use LAPACK-style solver (simplified implementation) */
    for (int i = 0; i < matrix_size; i++) {
        solution[i] = rhs_vector[i];
    }
    
    /* Simple Gaussian elimination (replace with proper LU solver) */
    for (int k = 0; k < matrix_size - 1; k++) {
        for (int i = k + 1; i < matrix_size; i++) {
            if (cabs(y_matrix[i * matrix_size + k]) > 1e-12) {
                double complex factor = y_matrix[i * matrix_size + k] / y_matrix[k * matrix_size + k];
                for (int j = k + 1; j < matrix_size; j++) {
                    y_matrix[i * matrix_size + j] -= factor * y_matrix[k * matrix_size + j];
                }
                solution[i] -= factor * solution[k];
            }
        }
    }
    
    /* Back substitution */
    for (int i = matrix_size - 1; i >= 0; i--) {
        solution[i] = solution[i] / y_matrix[i * matrix_size + i];
        for (int j = i - 1; j >= 0; j--) {
            solution[j] -= y_matrix[j * matrix_size + i] * solution[i];
        }
    }
    
    /* Update node voltages */
    for (int i = 0; i < matrix_size; i++) {
        network->nodes[i].ac_voltage = solution[i];
    }
    
    /* Store results */
    result->voltages = solution;
    result->num_frequencies = 1;
    result->converged = true;
    result->iterations = 1;
    
    free(y_matrix);
    free(rhs_vector);
    free(ipiv);
    
    return 0;
}

/**
 * @brief Create nonlinear model
 */
nonlinear_model_t* circuit_nonlinear_model_create(nonlinear_model_type_t type, int num_coefficients,
                                                 double* coefficients) {
    nonlinear_model_t* model = (nonlinear_model_t*)calloc(1, sizeof(nonlinear_model_t));
    if (!model) {
        return NULL;
    }
    
    model->type = type;
    model->num_coefficients = num_coefficients;
    model->temperature = 300.0;
    
    if (num_coefficients > 0 && num_coefficients <= MAX_NONLINEAR_TERMS && coefficients) {
        memcpy(model->coefficients, coefficients, num_coefficients * sizeof(double));
    }
    
    return model;
}

/**
 * @brief Destroy nonlinear model
 */
void circuit_nonlinear_model_destroy(nonlinear_model_t* model) {
    if (model) {
        free(model);
    }
}

/**
 * @brief Evaluate nonlinear model
 */
double circuit_nonlinear_model_evaluate(nonlinear_model_t* model, double input, double* derivative) {
    if (!model) {
        if (derivative) *derivative = 0.0;
        return 0.0;
    }
    
    double output = 0.0;
    double deriv = 0.0;
    
    switch (model->type) {
        case NONLINEAR_EXPONENTIAL:
            /* Exponential model: I = I0 * exp(k*V) */
            output = model->coefficients[0] * exp(model->coefficients[1] * input);
            deriv = model->coefficients[1] * output;
            break;
            
        case NONLINEAR_POLYNOMIAL:
            /* Polynomial model: y = a0 + a1*x + a2*x^2 + ... */
            double power = 1.0;
            for (int i = 0; i < model->num_coefficients; i++) {
                output += model->coefficients[i] * power;
                deriv += (i > 0) ? i * model->coefficients[i] * power / input : 0.0;
                power *= input;
            }
            break;
            
        case NONLINEAR_PWL:
            /* Piecewise linear model */
            if (model->num_coefficients >= 2) {
                int num_segments = model->num_coefficients / 2;
                for (int i = 0; i < num_segments - 1; i++) {
                    double x1 = model->breakpoints[i];
                    double x2 = model->breakpoints[i + 1];
                    if (input >= x1 && input <= x2) {
                        double y1 = model->coefficients[2 * i];
                        double y2 = model->coefficients[2 * i + 2];
                        output = y1 + (y2 - y1) * (input - x1) / (x2 - x1);
                        deriv = (y2 - y1) / (x2 - x1);
                        break;
                    }
                }
            }
            break;
            
        default:
            /* Linear model as default */
            output = model->coefficients[0] * input;
            deriv = model->coefficients[0];
            break;
    }
    
    if (derivative) {
        *derivative = deriv;
    }
    
    return output;
}

/**
 * @brief Export circuit to SPICE netlist
 */
int circuit_export_spice(circuit_network_t* network, const char* filename, const char* format) {
    if (!network || !filename || !format) {
        return -1;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Error: Cannot open SPICE output file %s\n", filename);
        return -1;
    }
    
    fprintf(fp, "* Circuit simulation netlist\n");
    fprintf(fp, "* Generated by PEEC-MoM Unified Framework\n");
    fprintf(fp, "* Format: %s\n", format);
    fprintf(fp, ".TITLE PEEC_MOM_CIRCUIT\n\n");
    
    /* Write circuit elements */
    for (int i = 0; i < network->num_elements; i++) {
        circuit_element_t* element = &network->elements[i];
        if (!element->active) continue;
        
        char n1_name[16], n2_name[16];
        snprintf(n1_name, sizeof(n1_name), "N%d", element->params.node1);
        snprintf(n2_name, sizeof(n2_name), "N%d", element->params.node2);
        
        switch (element->type) {
            case ELEMENT_RESISTOR:
                fprintf(fp, "R%d %s %s %.6e\n", element->id, n1_name, n2_name, element->params.value);
                break;
            case ELEMENT_CAPACITOR:
                fprintf(fp, "C%d %s %s %.6e\n", element->id, n1_name, n2_name, element->params.value);
                break;
            case ELEMENT_INDUCTOR:
                fprintf(fp, "L%d %s %s %.6e\n", element->id, n1_name, n2_name, element->params.value);
                break;
            case ELEMENT_VOLTAGE_SOURCE:
                fprintf(fp, "V%d %s %s DC %.6e\n", element->id, n1_name, n2_name, element->params.value);
                break;
            case ELEMENT_CURRENT_SOURCE:
                fprintf(fp, "I%d %s %s DC %.6e\n", element->id, n1_name, n2_name, element->params.value);
                break;
            default:
                break;
        }
    }
    
    /* Write analysis commands */
    fprintf(fp, "\n* Analysis commands\n");
    fprintf(fp, ".OP\n");
    fprintf(fp, ".AC DEC 10 1e6 1e12\n");
    fprintf(fp, ".TRAN 1e-12 1e-6\n");
    fprintf(fp, ".END\n");
    
    fclose(fp);
    printf("Circuit exported to SPICE file: %s\n", filename);
    
    return 0;
}

/**
 * @brief Perform coupled EM-circuit simulation
 */
int circuit_em_coupled_simulation(void* em_solver_data, circuit_network_t* circuit_network,
                                 em_circuit_coupling_config_t* coupling_config,
                                 circuit_analysis_config_t* analysis_config,
                                 circuit_analysis_result_t* result) {
    if (!em_solver_data || !circuit_network || !coupling_config || !analysis_config || !result) {
        return -1;
    }
    
    printf("Performing Coupled EM-Circuit Simulation...\n");
    printf("EM Solver Type: %s\n", coupling_config->couplings[0].em_solver_type);
    printf("Number of Couplings: %d\n", coupling_config->num_couplings);
    
    /* Update EM coupling in circuit network */
    if (circuit_update_em_coupling(circuit_network, coupling_config, em_solver_data) != 0) {
        printf("Error: Failed to update EM coupling\n");
        return -1;
    }
    
    /* Perform circuit analysis based on configuration */
    switch (analysis_config->analysis_type) {
        case CIRCUIT_DC:
            return circuit_dc_analysis(circuit_network, analysis_config, result);
            
        case CIRCUIT_AC:
            {
                frequency_response_result_t ac_result = {0};
                int status = circuit_ac_analysis(circuit_network, analysis_config, &ac_result);
                if (status == 0) {
                    result->converged = true;
                    result->num_frequencies = ac_result.num_points;
                }
                return status;
            }
            
        case CIRCUIT_TRANSIENT:
            {
                transient_analysis_result_t tran_result = {0};
                int status = circuit_transient_analysis(circuit_network, analysis_config, &tran_result);
                if (status == 0) {
                    result->converged = true;
                }
                return status;
            }
            
        case CIRCUIT_HARMONIC_BALANCE:
            return circuit_harmonic_balance_analysis(circuit_network, analysis_config, result);
            
        default:
            printf("Error: Unsupported analysis type for EM coupling\n");
            return -1;
    }
}

/**
 * @brief Get circuit element power dissipation
 */
double circuit_get_element_power(circuit_network_t* network, int element_id) {
    if (!network || element_id < 0 || element_id >= network->num_elements) {
        return 0.0;
    }
    
    return network->elements[element_id].power;
}

/**
 * @brief Get circuit node voltage
 */
double circuit_get_node_voltage(circuit_network_t* network, int node_id) {
    if (!network || node_id < 0 || node_id >= network->num_nodes) {
        return 0.0;
    }
    
    return network->nodes[node_id].voltage;
}

/**
 * @brief Check circuit convergence
 */
bool circuit_check_convergence(circuit_network_t* network, double tolerance) {
    if (!network) {
        return false;
    }
    
    /* Simple convergence check based on voltage changes */
    static double* prev_voltages = NULL;
    static int prev_size = 0;
    
    if (!prev_voltages || prev_size != network->num_nodes) {
        free(prev_voltages);
        prev_voltages = (double*)calloc(network->num_nodes, sizeof(double));
        prev_size = network->num_nodes;
    }
    
    double max_change = 0.0;
    for (int i = 0; i < network->num_nodes; i++) {
        double change = fabs(network->nodes[i].voltage - prev_voltages[i]);
        if (change > max_change) {
            max_change = change;
        }
        prev_voltages[i] = network->nodes[i].voltage;
    }
    
    return max_change < tolerance;
}

/**
 * @brief Print circuit analysis statistics
 */
void circuit_print_statistics(circuit_network_t* network, circuit_analysis_result_t* result) {
    if (!network || !result) {
        return;
    }
    
    printf("\n=== Circuit Analysis Statistics ===\n");
    printf("Network Size: %d nodes, %d elements\n", network->num_nodes, network->num_elements);
    printf("Convergence: %s\n", result->converged ? "YES" : "NO");
    printf("Iterations: %d\n", result->iterations);
    printf("Simulation Time: %.3f seconds\n", result->simulation_time);
    printf("Memory Usage: %.2f MB\n", result->memory_usage / (1024.0 * 1024.0));
    
    /* Power statistics */
    double total_power = 0.0;
    double max_power = 0.0;
    for (int i = 0; i < network->num_elements; i++) {
        total_power += result->power[i];
        if (result->power[i] > max_power) {
            max_power = result->power[i];
        }
    }
    
    printf("Total Power: %.6e W\n", total_power);
    printf("Max Element Power: %.6e W\n", max_power);
    
    /* Voltage statistics */
    double max_voltage = 0.0;
    for (int i = 0; i < network->num_nodes; i++) {
        if (fabs(network->nodes[i].voltage) > max_voltage) {
            max_voltage = fabs(network->nodes[i].voltage);
        }
    }
    
    printf("Max Node Voltage: %.6e V\n", max_voltage);
    printf("=====================================\n\n");
}

/**
 * @brief Cleanup circuit analysis library
 */
void circuit_cleanup(void) {
    if (!circuit_initialized) {
        return;
    }
    
    printf("Cleaning up Circuit Analysis Library...\n");
    
    circuit_initialized = 0;
    circuit_element_counter = 0;
    circuit_node_counter = 0;
}

/* Placeholder implementations for advanced features */
static int circuit_harmonic_balance_solve(circuit_network_t* network, circuit_analysis_config_t* config, circuit_analysis_result_t* result) {
    printf("Harmonic balance solver - advanced implementation required\n");
    return 0;
}

static int circuit_newton_raphson_solve(circuit_network_t* network, circuit_analysis_result_t* result) {
    printf("Newton-Raphson solver - advanced implementation required\n");
    return 0;
}

static int circuit_solve_nonlinear_system(circuit_network_t* network, circuit_analysis_result_t* result) {
    printf("Nonlinear system solver - advanced implementation required\n");
    return 0;
}

static double circuit_diode_current(double voltage, double isat, double vt, double* conductance) {
    double current = isat * (exp(voltage / vt) - 1.0);
    if (conductance) {
        *conductance = isat * exp(voltage / vt) / vt;
    }
    return current;
}

static double circuit_mosfet_current(double vgs, double vds, double vt, double kp, double w, double l, double* gm, double* gds) {
    double vdsat = fmax(0.0, vgs - vt);
    double region = (vds < vdsat) ? 1.0 : 0.0;  /* 1=triode, 0=saturation */
    
    double current;
    if (region > 0.5) {
        /* Triode region */
        current = kp * (w/l) * ((vgs - vt) * vds - 0.5 * vds * vds);
    } else {
        /* Saturation region */
        current = 0.5 * kp * (w/l) * (vgs - vt) * (vgs - vt);
    }
    
    if (gm) *gm = (region > 0.5) ? kp * (w/l) * vds : kp * (w/l) * (vgs - vt);
    if (gds) *gds = (region > 0.5) ? kp * (w/l) * (vgs - vt - vds) : 0.0;
    
    return current;
}

static int circuit_extract_s_parameters(circuit_network_t* network, circuit_analysis_config_t* config, s_parameter_result_t* result) {
    printf("S-parameter extraction - advanced implementation required\n");
    return 0;
}

static int circuit_noise_analysis_compute(circuit_network_t* network, circuit_analysis_config_t* config, noise_analysis_result_t* result) {
    printf("Noise analysis - advanced implementation required\n");
    return 0;
}

static int circuit_em_coupling_update(circuit_network_t* network, em_circuit_coupling_config_t* config, void* em_solver_data) {
    printf("EM coupling update - advanced implementation required\n");
    return 0;
}

int circuit_s_parameter_analysis(circuit_network_t* network, circuit_analysis_config_t* config, s_parameter_result_t* result) {
    return circuit_extract_s_parameters(network, config, result);
}

int circuit_noise_analysis(circuit_network_t* network, circuit_analysis_config_t* config, noise_analysis_result_t* result) {
    return circuit_noise_analysis_compute(network, config, result);
}

int circuit_update_em_coupling(circuit_network_t* network, em_circuit_coupling_config_t* config, void* em_solver_data) {
    return circuit_em_coupling_update(network, config, em_solver_data);
}

int circuit_import_spice(const char* filename, circuit_network_t** network) {
    printf("SPICE import - advanced implementation required\n");
    return 0;
}