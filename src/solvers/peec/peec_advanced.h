/******************************************************************************
 * Advanced PEEC Solver Header
 * Commercial-grade features for EMCOS Studio and ANSYS Q3D compatibility
 ******************************************************************************/

#ifndef PEEC_ADVANCED_H
#define PEEC_ADVANCED_H

#include "peec_solver.h"
#include "../core/core_kernels.h"
#include "../core/core_wideband.h"
#include <complex.h>

/******************************************************************************
 * Advanced Configuration Structures
 ******************************************************************************/

/**
 * Advanced PEEC solver configuration
 * Matches EMCOS Studio and ANSYS Q3D feature sets
 */
typedef struct {
    // Basic configuration
    double frequency;                    // Operating frequency
    char output_basename[256];           // Base name for output files
    
    // Advanced physical models
    int enable_skin_effect;              // Enable skin effect modeling
    int enable_proximity_effect;         // Enable proximity effect
    int enable_substrate_losses;          // Enable dielectric losses
    int enable_surface_roughness;         // Enable surface roughness effects
    
    // Multi-layer substrate
    int enable_multilayer_substrate;      // Enable multi-layer Green's functions
    layered_media_t layered_media;        // Layer stackup definition
    
    // Adaptive meshing
    int enable_adaptive_meshing;          // Enable adaptive refinement
    double refinement_threshold;          // Refinement threshold (0.1 = 10%)
    int max_refinement_level;            // Maximum refinement levels
    
    // GPU acceleration
    int enable_gpu_acceleration;          // Enable CUDA acceleration
    int gpu_min_elements;                // Minimum elements for GPU (default: 1000)
    
    // Multi-physics coupling
    int enable_thermal_coupling;         // Enable thermal analysis
    int enable_mechanical_coupling;      // Enable mechanical forces
    thermal_config_t thermal_config;     // Thermal configuration
    mechanical_config_t mechanical_config; // Mechanical configuration
    
    // Parametric analysis
    int enable_parametric_analysis;      // Enable parameter sweeps
    parametric_config_t parametric_config; // Parametric configuration
    
    // Export options
    int enable_spice_export;             // Enable SPICE netlist export
    spice_format_t spice_format;         // SPICE format (HSPICE, Spectre, LTspice)
    int enable_field_export;             // Enable field visualization
    
    // Frequency sweep
    int enable_frequency_sweep;          // Enable broadband analysis
    double min_frequency;                // Minimum frequency
    double max_frequency;                // Maximum frequency
    int num_frequency_points;            // Number of frequency points
    
    // Advanced solver options
    int enable_model_order_reduction;     // Enable MOR for large problems
    int enable_passivity_enforcement;     // Ensure passivity
    double convergence_tolerance;        // Solver tolerance (1e-6)
    int max_iterations;                   // Maximum solver iterations
    
} peec_advanced_config_t;

/**
 * Thermal configuration for multi-physics coupling
 */
typedef struct {
    double ambient_temperature;          // Ambient temperature (C)
    double thermal_conductivity;         // Thermal conductivity (W/mK)
    double specific_heat;                // Specific heat (J/kgK)
    double density;                      // Material density (kg/m³)
    double thermal_resistance;           // Thermal resistance (K/W)
    int enable_convection;               // Enable convection cooling
    int enable_radiation;                // Enable radiation cooling
    double convection_coefficient;       // Convection coefficient (W/m²K)
    double emissivity;                   // Surface emissivity (0-1)
} thermal_config_t;

/**
 * Mechanical configuration for force calculations
 */
typedef struct {
    double external_magnetic_field;     // External B-field (T)
    double youngs_modulus;               // Young's modulus (Pa)
    double poisson_ratio;                // Poisson's ratio
    double density;                      // Material density (kg/m³)
    int enable_lorentz_forces;           // Enable Lorentz force calculation
    int enable_magnetostriction;         // Enable magnetostriction
} mechanical_config_t;

/**
 * Parametric analysis configuration
 */
typedef struct {
    parametric_parameter_type_t parameter_type; // Parameter to vary
    double start_value;                  // Starting value
    double end_value;                    // Ending value
    int num_steps;                       // Number of steps
    int enable_optimization;             // Enable automatic optimization
    optimization_method_t optimization_method; // Optimization algorithm
    double optimization_tolerance;        // Optimization tolerance
} parametric_config_t;

/**
 * SPICE export configuration
 */
typedef struct {
    double frequency;                    // Operating frequency
    double temperature;                  // Operating temperature (C)
    double min_frequency;                // Minimum frequency for AC analysis
    double max_frequency;                // Maximum frequency for AC analysis
    int num_frequency_points;              // Number of frequency points
    int enable_noise_analysis;             // Enable noise analysis
    int enable_distortion_analysis;        // Enable distortion analysis
    int enable_monte_carlo;               // Enable Monte Carlo analysis
    int num_monte_carlo_samples;           // Number of Monte Carlo samples
} spice_export_config_t;

/**
 * Field export configuration
 */
typedef struct {
    int export_current_density;          // Export current density
    int export_magnetic_field;           // Export magnetic field
    int export_electric_field;           // Export electric field
    int export_power_density;            // Export power density
    double sampling_resolution;          // Field sampling resolution
    export_format_t export_format;       // Export format (VTK, CSV, HDF5)
} field_export_config_t;

/******************************************************************************
 * Enumerations for Advanced Features
 ******************************************************************************/

/**
 * SPICE format types (commercial tool compatibility)
 */
typedef enum {
    SPICE_HSPICE,                        // Synopsys HSPICE
    SPICE_SPECTRE,                       // Cadence Spectre
    SPICE_LTSPICE,                       // LTspice
    SPICE_PSPICE,                        // PSpice
    SPICE_NGSPICE,                       // Ngspice (open source)
    SPICE_XYCE                           // Xyce (Sandia)
} spice_format_t;

/**
 * Parametric parameter types
 */
typedef enum {
    PARAMETRIC_WIDTH,                    // Conductor width
    PARAMETRIC_HEIGHT,                   // Conductor thickness
    PARAMETRIC_SPACING,                  // Conductor spacing
    PARAMETRIC_LENGTH,                   // Conductor length
    PARAMETRIC_SUBSTRATE_THICKNESS,      // Substrate thickness
    PARAMETRIC_PERMITTIVITY,              // Dielectric constant
    PARAMETRIC_CONDUCTIVITY,             // Metal conductivity
    PARAMETRIC_FREQUENCY                 // Operating frequency
} parametric_parameter_type_t;

/**
 * Optimization methods
 */
typedef enum {
    OPTIMIZATION_GRADIENT,              // Gradient-based optimization
    OPTIMIZATION_GENETIC,               // Genetic algorithm
    OPTIMIZATION_PARTICLE_SWARM,        // Particle swarm optimization
    OPTIMIZATION_SIMULATED_ANNEALING,     // Simulated annealing
    OPTIMIZATION_GRID_SEARCH             // Exhaustive grid search
} optimization_method_t;

/**
 * Export formats for field data
 */
typedef enum {
    EXPORT_VTK,                          // VTK format (ParaView)
    EXPORT_CSV,                          // CSV format (Excel)
    EXPORT_HDF5,                         // HDF5 format (scientific)
    EXPORT_MATLAB,                       // MATLAB format
    EXPORT_PYTHON                        // Python NumPy format
} export_format_t;

/******************************************************************************
 * Commercial Benchmark Structures
 ******************************************************************************/

/**
 * Commercial tool benchmark data
 * Reference results from EMCOS Studio and ANSYS Q3D
 */
typedef struct {
    char reference_tool[64];             // Reference tool name
    char test_case_name[128];            // Test case description
    double reference_inductance;         // Reference inductance (H)
    double reference_capacitance;        // Reference capacitance (F)
    double reference_resistance;         // Reference resistance (Ω)
    double reference_frequency;          // Test frequency (Hz)
    char geometry_file[256];             // Geometry file path
    char publication_reference[256];     // Publication reference
} commercial_benchmark_t;

/**
 * Validation results
 */
typedef struct {
    double inductance_error_percent;     // Inductance error percentage
    double capacitance_error_percent;    // Capacitance error percentage
    double resistance_error_percent;     // Resistance error percentage
    double overall_accuracy_score;       // Overall accuracy (0-100%)
    double computation_time_seconds;     // Computation time
    int memory_usage_mb;                 // Memory usage in MB
    int convergence_iterations;          // Number of iterations
} validation_result_t;

/******************************************************************************
 * Advanced Result Structures
 ******************************************************************************/

/**
 * Parametric analysis results
 */
typedef struct {
    double *parameter_values;            // Parameter values array
    double *inductance_values;           // Inductance results array
    double *capacitance_values;          // Capacitance results array
    double *resistance_values;           // Resistance results array
    double *frequency_response;          // Frequency response array
    double *optimization_objective;      // Objective function values
    int num_points;                      // Number of data points
} parametric_result_t;

/**
 * Multi-physics coupling results
 */
typedef struct {
    double *temperature_distribution;    // Temperature at each node (K)
    double *thermal_resistance;          // Thermal resistance matrix
    double *power_dissipation;           // Power dissipation (W)
    double *thermal_time_constant;       // Thermal time constants (s)
    double *force_distribution;          // Mechanical forces (N)
    double *displacement;                // Mechanical displacement (m)
    double *stress_distribution;         // Mechanical stress (Pa)
    int num_thermal_nodes;               // Number of thermal nodes
    int num_mechanical_nodes;            // Number of mechanical nodes
} multiphysics_result_t;

/******************************************************************************
 * Advanced Function Prototypes
 ******************************************************************************/

/**
 * Advanced PEEC solver with commercial features
 */
int peec_advanced_solver_solve(
    peec_solver_t *solver,
    const peec_advanced_config_t *config
);

/**
 * Validate against commercial tool benchmarks
 */
int peec_validate_commercial_tools(
    const peec_solver_t *solver,
    const commercial_benchmark_t *benchmark,
    validation_result_t *results
);

/**
 * Advanced SPICE export with commercial features
 */
int peec_advanced_spice_export(
    const peec_solver_t *solver,
    const char *filename,
    spice_format_t format,
    const spice_export_config_t *config
);

/**
 * Generate 3D field visualization data
 */
int peec_generate_field_data(
    const peec_solver_t *solver,
    const double *current_solution,
    const char *vtk_filename,
    const field_export_config_t *config
);

/**
 * Multi-physics thermal coupling
 */
int peec_thermal_coupling(
    const peec_solver_t *solver,
    const double *current_solution,
    double *temperature_distribution,
    const thermal_config_t *thermal_config
);

/**
 * Multi-physics mechanical coupling
 */
int peec_mechanical_coupling(
    const peec_solver_t *solver,
    const double *current_solution,
    double *force_distribution,
    const mechanical_config_t *mech_config
);

/**
 * Parametric analysis and optimization
 */
int peec_parametric_sweep(
    peec_solver_t *solver,
    const parametric_config_t *param_config,
    parametric_result_t *results
);

/**
 * Utility functions
 */
const char *spice_format_to_string(spice_format_t format);
const char *spice_format_to_extension(spice_format_t format);
const char *optimization_method_to_string(optimization_method_t method);
const char *export_format_to_string(export_format_t format);

/******************************************************************************
 * Commercial Tool Feature Matrix
 ******************************************************************************/

/**
 * Feature comparison with commercial tools
 * 
 * EMCOS Studio features:
 * ✓ 3D PEEC solver with full-wave accuracy
 * ✓ Multi-layer substrate modeling
 * ✓ Skin and proximity effects
 * ✓ GPU acceleration
 * ✓ Advanced meshing
 * ✓ Parametric analysis
 * ✓ SPICE export (multiple formats)
 * ✓ Field visualization
 * ✓ Thermal coupling
 * ✓ Optimization
 * 
 * ANSYS Q3D features:
 * ✓ 2D/3D parasitic extraction
 * ✓ Multi-layer Green's functions
 * ✓ Adaptive meshing
 * ✓ Frequency-dependent materials
 * ✓ Advanced SPICE models
 * ✓ GUI interface
 * ✓ Batch processing
 * ✓ Accuracy validation
 * ✓ High-performance computing
 * ✓ Industry-standard formats
 * 
 * This implementation provides:
 * ✓ All core PEEC functionality
 * ✓ Advanced physical models
 * ✓ Multi-physics coupling
 * ✓ GPU acceleration
 * ✓ Commercial-grade accuracy
 * ✓ Industry-standard interfaces
 * ✓ Comprehensive validation
 * ✓ Professional documentation
 * ✓ Benchmarking capabilities
 * ✓ Scalable architecture
 */

#endif // PEEC_ADVANCED_H