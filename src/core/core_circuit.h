/**
 * @file core_circuit.h
 * @brief Advanced circuit simulation and coupling capabilities
 * @details Commercial-grade circuit solver with harmonic balance, nonlinear analysis,
 *          and electromagnetic coupling similar to Keysight ADS and Cadence Spectre
 */

#ifndef CORE_CIRCUIT_H
#define CORE_CIRCUIT_H

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CIRCUIT_NODES 100000
#define MAX_CIRCUIT_ELEMENTS 500000
#define MAX_HARMONICS 50
#define MAX_NONLINEAR_TERMS 20
#define CIRCUIT_CONVERGENCE_TOLERANCE 1e-12
#define MAX_NEWTON_ITERATIONS 100

typedef enum {
    CIRCUIT_LINEAR = 0,
    CIRCUIT_NONLINEAR = 1,
    CIRCUIT_TIME_DOMAIN = 2,
    CIRCUIT_FREQUENCY_DOMAIN = 3,
    CIRCUIT_HARMONIC_BALANCE = 4,
    CIRCUIT_TRANSIENT = 5,
    CIRCUIT_DC = 6,
    CIRCUIT_AC = 7,
    CIRCUIT_S_PARAMETER = 8,
    CIRCUIT_NOISE = 9
} circuit_analysis_type_t;

typedef enum {
    ELEMENT_RESISTOR = 0,
    ELEMENT_CAPACITOR = 1,
    ELEMENT_INDUCTOR = 2,
    ELEMENT_VOLTAGE_SOURCE = 3,
    ELEMENT_CURRENT_SOURCE = 4,
    ELEMENT_VCVS = 5,      /* Voltage-controlled voltage source */
    ELEMENT_VCCS = 6,      /* Voltage-controlled current source */
    ELEMENT_CCCS = 7,      /* Current-controlled current source */
    ELEMENT_CCVS = 8,      /* Current-controlled voltage source */
    ELEMENT_DIODE = 9,
    ELEMENT_BJT = 10,
    ELEMENT_MOSFET = 11,
    ELEMENT_JFET = 12,
    ELEMENT_MESFET = 13,
    ELEMENT_OPAMP = 14,
    ELEMENT_TRANSFORMER = 15,
    ELEMENT_TRANSMISSION_LINE = 16,
    ELEMENT_SUBCIRCUIT = 17,
    ELEMENT_EM_COUPLED = 18,  /* Electromagnetically coupled element */
    ELEMENT_NONLINEAR = 19
} circuit_element_type_t;

typedef enum {
    NONLINEAR_EXPONENTIAL = 0,
    NONLINEAR_POLYNOMIAL = 1,
    NONLINEAR_PWL = 2,          /* Piecewise linear */
    NONLINEAR_TAYLOR = 3,
    NONLINEAR_CHEBYSHEV = 4,
    NONLINEAR_FOURIER = 5,
    NONLINEAR_LOOKUP = 6,
    NONLINEAR_CUSTOM = 7
} nonlinear_model_type_t;

typedef struct {
    int node1, node2;           /* Node connections */
    double value;                /* Element value */
    double complex ac_value;     /* AC analysis value */
    double temperature_coeff;    /* Temperature coefficient */
    double noise_factor;         /* Noise factor */
    void* nonlinear_model;       /* Nonlinear model pointer */
} circuit_element_params_t;

typedef struct {
    nonlinear_model_type_t type;
    int num_coefficients;
    double coefficients[MAX_NONLINEAR_TERMS];
    double breakpoints[10];      /* For PWL models */
    double lookup_table[100];    /* For lookup tables */
    int table_size;
    double temperature;          /* Operating temperature */
    double* custom_function;     /* Function pointer for custom models */
} nonlinear_model_t;

typedef struct {
    int id;
    circuit_element_type_t type;
    circuit_element_params_t params;
    nonlinear_model_t* nonlinear;
    char name[64];
    bool active;
    double power;                /* Dissipated power */
    double temperature;          /* Element temperature */
} circuit_element_t;

typedef struct {
    int id;
    double voltage;              /* Node voltage */
    double complex ac_voltage;  /* AC analysis voltage */
    double* harmonic_voltages;   /* Harmonic balance voltages */
    int num_harmonics;
    bool is_ground;
    char name[32];
} circuit_node_t;

typedef struct {
    int num_nodes;
    int num_elements;
    int num_grounds;
    circuit_node_t* nodes;
    circuit_element_t* elements;
    int** adjacency_matrix;      /* Circuit topology */
    double* conductance_matrix;  /* G matrix */
    double* capacitance_matrix;  /* C matrix */
    double* inductance_matrix;   /* L matrix */
    double frequency;            /* Operating frequency */
    double temperature;          /* Ambient temperature */
    bool converged;
    int iteration_count;
} circuit_network_t;

typedef struct {
    circuit_analysis_type_t analysis_type;
    double start_frequency;
    double stop_frequency;
    int num_frequency_points;
    double start_time;
    double stop_time;
    double time_step;
    int num_harmonics;
    double convergence_tolerance;
    int max_iterations;
    bool use_initial_guess;
    bool enable_noise_analysis;
    bool enable_temperature_analysis;
    double temperature;
    double* frequency_points;
    double* time_points;
} circuit_analysis_config_t;

typedef struct {
    double complex* voltages;    /* Node voltages */
    double complex* currents;    /* Branch currents */
    double* power;               /* Power consumption */
    double* noise;               /* Noise spectral density */
    double* harmonics;           /* Harmonic components */
    double* s_parameters;        /* S-parameters */
    double* y_parameters;        /* Y-parameters */
    double* z_parameters;        /* Z-parameters */
    int num_ports;
    int num_frequencies;
    int num_harmonics;
    bool converged;
    int iterations;
    double simulation_time;
    double memory_usage;
} circuit_analysis_result_t;

typedef struct {
    int em_element_id;           /* EM coupled element ID */
    int circuit_element_id;    /* Circuit element ID */
    double coupling_factor;      /* EM coupling strength */
    double complex* frequency_response;
    double* spatial_coordinates;
    char em_solver_type[16];     /* "MOM" or "PEEC" */
    bool bidirectional;        /* Bidirectional coupling */
} em_circuit_coupling_t;

typedef struct {
    int num_couplings;
    em_circuit_coupling_t* couplings;
    double coupling_tolerance;
    bool enable_full_wave;
    bool enable_quasi_static;
    double max_coupling_distance;
    int update_frequency;
} em_circuit_coupling_config_t;

typedef struct {
    double* time_points;
    double* voltage_waveforms;
    double* current_waveforms;
    double* power_waveforms;
    int num_points;
    double time_step;
    double max_voltage;
    double max_current;
    double max_power;
    double rms_voltage;
    double rms_current;
    double average_power;
} transient_analysis_result_t;

typedef struct {
    double* frequencies;
    double* magnitude_db;
    double* phase_deg;
    double* group_delay;
    double* real_part;
    double* imag_part;
    int num_points;
    double start_freq;
    double stop_freq;
    double min_magnitude;
    double max_magnitude;
    double bandwidth;
    double center_frequency;
} frequency_response_result_t;

typedef struct {
    double* frequencies;
    double* noise_voltage;
    double* noise_current;
    double* noise_power;
    double* noise_figure;
    double* noise_temperature;
    int num_points;
    double total_noise;
    double spot_noise_1k;
    double spot_noise_10k;
    double integrated_noise;
} noise_analysis_result_t;

typedef struct {
    double* frequencies;
    double* s11_mag;
    double* s11_phase;
    double* s21_mag;
    double* s21_phase;
    double* s12_mag;
    double* s12_phase;
    double* s22_mag;
    double* s22_phase;
    int num_points;
    double min_s11;
    double max_s21;
    double bandwidth;
    double center_frequency;
} s_parameter_result_t;

/**
 * @brief Initialize circuit analysis library
 * @return 0 on success, -1 on failure
 */
int circuit_init(void);

/**
 * @brief Create new circuit network
 * @param num_nodes Expected number of nodes
 * @param num_elements Expected number of elements
 * @return Pointer to new circuit network, NULL on failure
 */
circuit_network_t* circuit_network_create(int num_nodes, int num_elements);

/**
 * @brief Destroy circuit network and free memory
 * @param network Circuit network to destroy
 */
void circuit_network_destroy(circuit_network_t* network);

/**
 * @brief Add element to circuit network
 * @param network Circuit network
 * @param type Element type
 * @param node1 First node
 * @param node2 Second node
 * @param value Element value
 * @param name Element name (optional)
 * @return Element ID on success, -1 on failure
 */
int circuit_add_element(circuit_network_t* network, circuit_element_type_t type,
                       int node1, int node2, double value, const char* name);

/**
 * @brief Add nonlinear element to circuit network
 * @param network Circuit network
 * @param type Element type
 * @param node1 First node
 * @param node2 Second node
 * @param model Nonlinear model
 * @param name Element name (optional)
 * @return Element ID on success, -1 on failure
 */
int circuit_add_nonlinear_element(circuit_network_t* network, circuit_element_type_t type,
                                 int node1, int node2, nonlinear_model_t* model, const char* name);

/**
 * @brief Add electromagnetically coupled element
 * @param network Circuit network
 * @param em_element_id EM solver element ID
 * @param node1 First circuit node
 * @param node2 Second circuit node
 * @param coupling_factor EM coupling strength
 * @param solver_type EM solver type ("MOM" or "PEEC")
 * @return Element ID on success, -1 on failure
 */
int circuit_add_em_coupled_element(circuit_network_t* network, int em_element_id,
                                  int node1, int node2, double coupling_factor, const char* solver_type);

/**
 * @brief Perform DC analysis
 * @param network Circuit network
 * @param config Analysis configuration
 * @param result Analysis result
 * @return 0 on success, -1 on failure
 */
int circuit_dc_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                       circuit_analysis_result_t* result);

/**
 * @brief Perform AC small-signal analysis
 * @param network Circuit network
 * @param config Analysis configuration
 * @param result Analysis result
 * @return 0 on success, -1 on failure
 */
int circuit_ac_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                       frequency_response_result_t* result);

/**
 * @brief Perform transient analysis
 * @param network Circuit network
 * @param config Analysis configuration
 * @param result Analysis result
 * @return 0 on success, -1 on failure
 */
int circuit_transient_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                              transient_analysis_result_t* result);

/**
 * @brief Perform harmonic balance analysis
 * @param network Circuit network
 * @param config Analysis configuration
 * @param result Analysis result
 * @return 0 on success, -1 on failure
 */
int circuit_harmonic_balance_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                                     circuit_analysis_result_t* result);

/**
 * @brief Perform S-parameter analysis
 * @param network Circuit network
 * @param config Analysis configuration
 * @param result Analysis result
 * @return 0 on success, -1 on failure
 */
int circuit_s_parameter_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                                s_parameter_result_t* result);

/**
 * @brief Perform noise analysis
 * @param network Circuit network
 * @param config Analysis configuration
 * @param result Analysis result
 * @return 0 on success, -1 on failure
 */
int circuit_noise_analysis(circuit_network_t* network, circuit_analysis_config_t* config,
                          noise_analysis_result_t* result);

/**
 * @brief Update EM coupling in circuit network
 * @param network Circuit network
 * @param config EM coupling configuration
 * @param em_solver_data EM solver field data
 * @return 0 on success, -1 on failure
 */
int circuit_update_em_coupling(circuit_network_t* network, em_circuit_coupling_config_t* config,
                              void* em_solver_data);

/**
 * @brief Create nonlinear model
 * @param type Nonlinear model type
 * @param num_coefficients Number of coefficients
 * @param coefficients Model coefficients
 * @return Pointer to nonlinear model, NULL on failure
 */
nonlinear_model_t* circuit_nonlinear_model_create(nonlinear_model_type_t type, int num_coefficients,
                                                 double* coefficients);

/**
 * @brief Destroy nonlinear model
 * @param model Nonlinear model to destroy
 */
void circuit_nonlinear_model_destroy(nonlinear_model_t* model);

/**
 * @brief Evaluate nonlinear model
 * @param model Nonlinear model
 * @param input Input value
 * @param derivative Output derivative (optional)
 * @return Output value
 */
double circuit_nonlinear_model_evaluate(nonlinear_model_t* model, double input, double* derivative);

/**
 * @brief Export circuit to SPICE netlist
 * @param network Circuit network
 * @param filename Output filename
 * @param format SPICE format ("HSPICE", "Spectre", "LTspice")
 * @return 0 on success, -1 on failure
 */
int circuit_export_spice(circuit_network_t* network, const char* filename, const char* format);

/**
 * @brief Import circuit from SPICE netlist
 * @param filename Input filename
 * @param network Output circuit network
 * @return 0 on success, -1 on failure
 */
int circuit_import_spice(const char* filename, circuit_network_t** network);

/**
 * @brief Perform coupled EM-circuit simulation
 * @param em_solver_data EM solver data (MOM or PEEC)
 * @param circuit_network Circuit network
 * @param coupling_config Coupling configuration
 * @param analysis_config Analysis configuration
 * @param result Analysis result
 * @return 0 on success, -1 on failure
 */
int circuit_em_coupled_simulation(void* em_solver_data, circuit_network_t* circuit_network,
                                 em_circuit_coupling_config_t* coupling_config,
                                 circuit_analysis_config_t* analysis_config,
                                 circuit_analysis_result_t* result);

/**
 * @brief Get circuit element power dissipation
 * @param network Circuit network
 * @param element_id Element ID
 * @return Power dissipation in watts
 */
double circuit_get_element_power(circuit_network_t* network, int element_id);

/**
 * @brief Get circuit node voltage
 * @param network Circuit network
 * @param node_id Node ID
 * @return Node voltage
 */
double circuit_get_node_voltage(circuit_network_t* network, int node_id);

/**
 * @brief Check circuit convergence
 * @param network Circuit network
 * @param tolerance Convergence tolerance
 * @return true if converged, false otherwise
 */
bool circuit_check_convergence(circuit_network_t* network, double tolerance);

/**
 * @brief Print circuit analysis statistics
 * @param network Circuit network
 * @param result Analysis result
 */
void circuit_print_statistics(circuit_network_t* network, circuit_analysis_result_t* result);

/**
 * @brief Cleanup circuit analysis library
 */
void circuit_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* CORE_CIRCUIT_H */