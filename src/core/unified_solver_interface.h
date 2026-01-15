/**
 * @file unified_solver_interface.h
 * @brief Unified interface for all PulseEM solvers (MoM, PEEC, MTL)
 * @details Provides consistent API across different electromagnetic solvers
 */

#ifndef UNIFIED_SOLVER_INTERFACE_H
#define UNIFIED_SOLVER_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct mom_solver mom_solver_t;
typedef struct peec_solver peec_solver_t;
typedef struct mtl_solver mtl_solver_t;

// Unified solver types
typedef enum {
    SOLVER_TYPE_MOM,        // Method of Moments
    SOLVER_TYPE_PEEC,       // Partial Element Equivalent Circuit
    SOLVER_TYPE_MTL,        // Multi-conductor Transmission Line
    SOLVER_TYPE_HYBRID      // Hybrid combination
} solver_type_t;

// Unified analysis types
typedef enum {
    ANALYSIS_FREQUENCY,     // Frequency domain
    ANALYSIS_TIME_DOMAIN,   // Time domain
    ANALYSIS_SWEEP,         // Parameter sweep
    ANALYSIS_MONTE_CARLO,   // Statistical analysis
    ANALYSIS_OPTIMIZATION   // Optimization study
} analysis_type_t;

// Unified result formats
typedef enum {
    RESULT_FORMAT_NATIVE,   // Solver-specific format
    RESULT_FORMAT_CSV,     // Comma-separated values
    RESULT_FORMAT_MATLAB,   // MATLAB format
    RESULT_FORMAT_SPICE,    // SPICE netlist
    RESULT_FORMAT_HDF5,     // HDF5 format
    RESULT_FORMAT_JSON      // JSON format
} result_format_t;

// Common solver configuration
typedef struct {
    // Basic settings
    solver_type_t solver_type;
    analysis_type_t analysis_type;
    
    // Frequency settings
    double freq_start;          // Start frequency (Hz)
    double freq_stop;           // Stop frequency (Hz)
    int freq_points;          // Number of frequency points
    bool adaptive_freq;       // Adaptive frequency sampling
    
    // Time settings (for time domain)
    double time_start;          // Start time (s)
    double time_stop;           // Stop time (s)
    double time_step;           // Time step (s)
    
    // Numerical settings
    double tolerance;           // Solution tolerance
    int max_iterations;       // Maximum iterations
    bool enable_gpu;           // Enable GPU acceleration
    int num_threads;          // Number of threads
    
    // Output settings
    result_format_t output_format;
    bool save_fields;         // Save electromagnetic fields
    bool save_currents;       // Save current distributions
    bool save_impedance;      // Save impedance matrix
    bool save_admittance;     // Save admittance matrix
    bool save_scattering;     // Save scattering matrix
    
    // Advanced options
    bool enable_coupling;     // Enable hybrid coupling
    bool enable_preconditioning; // Enable matrix preconditioning
    bool enable_compression;  // Enable matrix compression
    bool enable_parallel;     // Enable parallel processing
} unified_solver_config_t;

// Common results structure
typedef struct {
    // Frequency domain results
    int num_frequencies;      // Number of frequency points
    double* frequencies;      // Frequency vector (Hz)
    
    // Network parameters
    double complex** s_matrix;  // S-parameters [num_ports x num_ports x num_freq]
    double complex** z_matrix;  // Z-parameters [num_ports x num_ports x num_freq]
    double complex** y_matrix;  // Y-parameters [num_ports x num_ports x num_freq]
    
    // Field quantities
    double complex** e_field;   // Electric field [num_points x num_freq]
    double complex** h_field;   // Magnetic field [num_points x num_freq]
    double complex** j_current; // Current density [num_points x num_freq]
    
    // Current and voltage distributions
    double complex** currents;  // Current distribution [num_elements x num_freq]
    double complex** voltages;  // Voltage distribution [num_elements x num_freq]
    
    // Performance metrics
    double solve_time;        // Total solution time (s)
    int iterations;           // Number of iterations
    double memory_usage;      // Peak memory usage (MB)
    int num_elements;         // Number of elements/basis functions
    int num_ports;            // Number of ports
    
    // Solver-specific data
    void* solver_specific;    // Pointer to solver-specific results
} unified_results_t;

// Unified solver handle
typedef struct unified_solver unified_solver_t;

// Unified solver API
unified_solver_t* unified_solver_create(solver_type_t type);
void unified_solver_destroy(unified_solver_t* solver);

// Configuration
int unified_solver_set_config(unified_solver_t* solver, const unified_solver_config_t* config);
int unified_solver_get_config(const unified_solver_t* solver, unified_solver_config_t* config);

// Geometry and materials
int unified_solver_set_geometry(unified_solver_t* solver, const char* geometry_file);
int unified_solver_add_material(unified_solver_t* solver, const char* material_name, 
                               double epsilon_r, double mu_r, double sigma);

// Analysis
int unified_solver_analyze(unified_solver_t* solver);
int unified_solver_solve_frequency(unified_solver_t* solver, double frequency);
int unified_solver_solve_time_domain(unified_solver_t* solver, double time);

// Results
unified_results_t* unified_solver_get_results(const unified_solver_t* solver);
void unified_results_destroy(unified_results_t* results);
int unified_results_save(const unified_results_t* results, const char* filename, result_format_t format);

// Utility functions
const char* unified_solver_get_name(const unified_solver_t* solver);
solver_type_t unified_solver_get_type(const unified_solver_t* solver);
const char* unified_solver_get_version(const unified_solver_t* solver);
double unified_solver_estimate_memory(const unified_solver_t* solver);
int unified_solver_validate_input(const unified_solver_t* solver);

// Error handling
typedef enum {
    UNIFIED_SUCCESS = 0,
    UNIFIED_ERROR_INVALID_ARGUMENT = -1,
    UNIFIED_ERROR_OUT_OF_MEMORY = -2,
    UNIFIED_ERROR_CONVERGENCE = -3,
    UNIFIED_ERROR_SINGULAR = -4,
    UNIFIED_ERROR_FILE_IO = -5,
    UNIFIED_ERROR_LICENSE = -6,
    UNIFIED_ERROR_NOT_IMPLEMENTED = -7,
    UNIFIED_ERROR_INTERNAL = -99
} unified_error_t;

const char* unified_error_string(unified_error_t error);

// Solver-specific access
mom_solver_t* unified_solver_get_mom(unified_solver_t* solver);
peec_solver_t* unified_solver_get_peec(unified_solver_t* solver);
mtl_solver_t* unified_solver_get_mtl(unified_solver_t* solver);

// Hybrid coupling interface
int unified_solver_enable_coupling(unified_solver_t* solver1, unified_solver_t* solver2);
int unified_solver_set_coupling_config(unified_solver_t* solver, double tolerance, int max_iter);
int unified_solver_update_coupling(unified_solver_t* solver);

#ifdef __cplusplus
}
#endif

#endif /* UNIFIED_SOLVER_INTERFACE_H */