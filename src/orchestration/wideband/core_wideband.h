/**
 * @file core_wideband.h
 * @brief Wideband simulation and model order reduction
 * @details Vector fitting, adaptive frequency sampling, and passive model extraction
 */

#ifndef CORE_WIDEBAND_H
#define CORE_WIDEBAND_H

#include <stdint.h>
#include <stdbool.h>
#include "core_common.h"
#include "../backend/solvers/core_solver.h"
#if !defined(_MSC_VER)
#include <complex.h>
#endif

#if defined(_MSC_VER)
typedef complex_t wideband_complex_t;
#else
typedef double complex wideband_complex_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Frequency domain data
typedef struct {
    double* frequencies;          // Hz, ascending order
    wideband_complex_t* values;       // Complex response values
    int num_points;
    double fmin, fmax;          // Frequency range
    bool is_uniform;            // Uniform frequency spacing
} wideband_data_t;

// Adaptive frequency sampling parameters
typedef struct {
    double fmin;                // Start frequency (Hz)
    double fmax;                // Stop frequency (Hz)
    int initial_points;         // Initial uniform samples
    int max_points;             // Maximum total samples
    double error_threshold;     // Convergence threshold
    double min_spacing;         // Minimum frequency spacing
    double max_spacing;         // Maximum frequency spacing
    int max_iterations;         // Maximum refinement iterations
    
    // Error estimation
    bool use_relative_error;
    double relative_tolerance;
    double absolute_tolerance;
    
    // Refinement strategy
    bool refine_max_error;      // Refine at maximum error
    bool refine_curvature;      // Refine at high curvature
    bool refine_edges;          // Refine near band edges
    
    // Parallel processing
    bool use_parallel;
    int num_threads;
} adaptive_sampling_params_t;

// Vector fitting parameters
typedef struct {
    int initial_poles;          // Initial number of poles
    int max_poles;              // Maximum number of poles
    int max_iterations;         // Maximum fitting iterations
    double tolerance;           // Convergence tolerance
    
    // Pole initialization
    bool use_auto_poles;        // Automatic pole placement
    bool use_log_poles;         // Logarithmic pole spacing
    bool use_chebyshev_poles;   // Chebyshev pole placement
    
    // Pole constraints
    bool enforce_stable_poles;  // Force left-half plane poles
    bool enforce_real_poles;    // Force real poles for real systems
    bool enforce_complex_pairs; // Force complex conjugate pairs
    
    // Iteration control
    double pole_relocation_tolerance;
    double residue_tolerance;
    int max_pole_relocations;
    
    // Weighting
    bool use_magnitude_weighting;
    bool use_frequency_weighting;
    double* weights;            // Custom weights
    
    // Output control
    bool verbose;
    int output_frequency;
} vector_fitting_params_t;

// Vector fitting result
typedef struct {
    wideband_complex_t* poles;          // Fitted poles (rad/s)
    wideband_complex_t* residues;       // Fitted residues
    wideband_complex_t constant;        // Constant term (D)
    wideband_complex_t proportional;    // Proportional term (E)
    
    int num_poles;
    int num_inputs;
    int num_outputs;
    
    // Fitting quality
    double rms_error;
    double max_error;
    int num_iterations;
    bool converged;
    
    // Pole properties
    bool* stable_poles;
    bool* real_poles;
    int num_stable_poles;
    int num_real_poles;
    
    // Frequency response of fitted model
    wideband_data_t* fitted_response;
    
    // State-space representation
    double* A_matrix;               // State matrix
    double* B_matrix;               // Input matrix
    double* C_matrix;               // Output matrix
    double* D_matrix;               // Feedthrough matrix
    int state_dimension;
    
    // Metadata
    double fitting_time;
    int condition_number;
} vector_fitting_result_t;

// Passivity checking and enforcement
typedef struct {
    bool check_passivity;
    double passivity_tolerance;
    bool enforce_passivity;
    bool enforce_by_pole_flipping;
    bool enforce_by_residue_perturbation;
    double perturbation_tolerance;
    
    // Hamiltonian matrix method
    bool use_hamiltonian_method;
    double hamiltonian_tolerance;
    int max_hamiltonian_iterations;
    
    // Positive real method
    bool use_positive_real_method;
    double positive_real_tolerance;
} passivity_params_t;

// Model order reduction parameters
typedef struct {
    int target_order;           // Target reduced order
    double error_tolerance;     // Maximum approximation error
    
    // Reduction method
    bool use_balanced_truncation;
    bool use_modal_truncation;
    bool use_singular_perturbation;
    bool use_optimal_hankel;
    
    // Frequency weighting
    bool use_frequency_weighting;
    double* weight_frequencies;
    double* weight_values;
    int num_weights;
    
    // Stability preservation
    bool preserve_stability;
    bool preserve_dc_gain;
    bool preserve_passivity;
    
    // Output control
    bool verbose;
} model_reduction_params_t;

// Reduced model result
typedef struct {
    double* A_reduced;          // Reduced state matrix
    double* B_reduced;          // Reduced input matrix
    double* C_reduced;          // Reduced output matrix
    double* D_reduced;          // Reduced feedthrough matrix
    
    int reduced_order;
    int original_order;
    
    // Approximation error
    double max_error;
    double rms_error;
    wideband_data_t* reduced_response;
    
    // Properties preserved
    bool stability_preserved;
    bool passivity_preserved;
    bool dc_gain_preserved;
    
    // Reduction info
    double reduction_time;
    int num_singular_values;
    double* singular_values;
} model_reduction_result_t;

/*********************************************************************
 * Wideband Engine Interface
 *********************************************************************/

typedef struct wideband_engine wideband_engine_t;

// Engine creation and configuration
wideband_engine_t* wideband_engine_create(void);
void wideband_engine_destroy(wideband_engine_t* engine);

// Adaptive frequency sampling
wideband_data_t* wideband_adaptive_sampling(const wideband_data_t* initial_data,
                                            adaptive_sampling_params_t* params,
                                            wideband_engine_t* engine);

// Vector fitting
vector_fitting_result_t* wideband_vector_fitting(const wideband_data_t* data,
                                                 vector_fitting_params_t* params,
                                                 wideband_engine_t* engine);

// Passivity checking and enforcement
bool wideband_check_passivity(const vector_fitting_result_t* result,
                            passivity_params_t* params);
int wideband_enforce_passivity(vector_fitting_result_t* result,
                             passivity_params_t* params);

// Model order reduction
model_reduction_result_t* wideband_model_reduction(const vector_fitting_result_t* model,
                                                   model_reduction_params_t* params,
                                                   wideband_engine_t* engine);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

// Frequency data manipulation
wideband_data_t* wideband_data_create(int num_points);
void wideband_data_destroy(wideband_data_t* data);
int wideband_data_add_point(wideband_data_t* data, double frequency, wideband_complex_t value);
int wideband_data_interpolate(const wideband_data_t* data, double frequency, 
                            wideband_complex_t* interpolated_value);

// Parameter validation
bool wideband_validate_sampling_params(const adaptive_sampling_params_t* params);
bool wideband_validate_vector_fitting_params(const vector_fitting_params_t* params);
bool wideband_validate_passivity_params(const passivity_params_t* params);

// Result analysis
double wideband_compute_rms_error(const wideband_data_t* original, 
                                const wideband_data_t* fitted);
double wideband_compute_max_error(const wideband_data_t* original,
                                const wideband_data_t* fitted);

// State-space conversion
int wideband_vector_fitting_to_state_space(const vector_fitting_result_t* result,
                                         double** A, double** B, double** C, double** D);
int wideband_state_space_to_transfer_function(const double* A, const double* B, 
                                             const double* C, const double* D,
                                             int n_states, int n_inputs, int n_outputs,
                                             const double* frequencies,
                                             wideband_complex_t* transfer_function);

// Export functions
int wideband_export_touchstone(const vector_fitting_result_t* result,
                             const char* filename, int num_ports);
int wideband_export_spice(const vector_fitting_result_t* result,
                        const char* filename, const char* model_name);
int wideband_export_matlab(const vector_fitting_result_t* result,
                         const char* filename, const char* variable_name);

/*********************************************************************
 * Advanced Features
 *********************************************************************/

// Multi-port vector fitting
typedef struct {
    vector_fitting_result_t** port_results;
    int num_ports;
    wideband_complex_t** s_matrix;      // S-parameter matrix
    wideband_complex_t** y_matrix;      // Y-parameter matrix
    wideband_complex_t** z_matrix;      // Z-parameter matrix
} multiport_fitting_result_t;

multiport_fitting_result_t* wideband_multiport_fitting(const wideband_data_t** port_data,
                                                     int num_ports,
                                                     vector_fitting_params_t* params,
                                                     wideband_engine_t* engine);

// Rational approximation with constraints
typedef struct {
    bool enforce_real_coefficients;
    bool enforce_stable_poles;
    bool enforce_minimum_phase;
    bool enforce_positive_real;
    double* constraint_frequencies;
    double* constraint_values;
    int num_constraints;
} constrained_fitting_params_t;

vector_fitting_result_t* wideband_constrained_fitting(const wideband_data_t* data,
                                                    constrained_fitting_params_t* params,
                                                    wideband_engine_t* engine);

// Time-domain synthesis
int wideband_synthesize_impulse_response(const vector_fitting_result_t* result,
                                       double time_start, double time_end, int num_points,
                                       double** time_vector, double** impulse_response);

int wideband_synthesize_step_response(const vector_fitting_result_t* result,
                                    double time_start, double time_end, int num_points,
                                    double** time_vector, double** step_response);

// Performance monitoring
typedef struct {
    double total_time;
    double sampling_time;
    double fitting_time;
    double passivity_time;
    double memory_usage;
    int num_function_evaluations;
    int num_iterations;
} wideband_performance_t;

wideband_performance_t wideband_get_performance(const wideband_engine_t* engine);

#ifdef __cplusplus
}
#endif

#endif // CORE_WIDEBAND_H
