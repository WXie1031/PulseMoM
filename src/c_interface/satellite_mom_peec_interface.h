/********************************************************************************
 * Satellite MoM/PEEC C Library Interface
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Commercial License - All Rights Reserved
 * Provides ctypes-compatible interface for calling C solvers from Python
 ********************************************************************************/

#ifndef SATELLITE_MOM_PEEC_INTERFACE_H
#define SATELLITE_MOM_PEEC_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/core_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
typedef enum {
    SATELLITE_SUCCESS = 0,
    SATELLITE_ERROR_INVALID_INPUT = -1,
    SATELLITE_ERROR_FILE_NOT_FOUND = -2,
    SATELLITE_ERROR_MESH_GENERATION = -3,
    SATELLITE_ERROR_SOLVER_FAILED = -4,
    SATELLITE_ERROR_MEMORY_ALLOCATION = -5,
    SATELLITE_ERROR_INVALID_GEOMETRY = -6,
    SATELLITE_ERROR_INVALID_FREQUENCY = -7,
    SATELLITE_ERROR_LIBRARY_NOT_FOUND = -8
} satellite_error_t;

// Material properties (compatible with CST format)
typedef struct {
    char name[64];
    double eps_r;      // Relative permittivity
    double mu_r;       // Relative permeability  
    double sigma;      // Conductivity (S/m)
    double tan_delta;  // Loss tangent
    int material_id;
} satellite_material_t;

// Plane wave excitation
typedef struct {
    double frequency;     // Hz
    double amplitude;   // V/m
    double phase;       // radians
    double theta;       // Incident angle (degrees)
    double phi;         // Azimuth angle (degrees)
    double polarization; // 0=TE, 1=TM
} satellite_excitation_t;

// Mesh parameters
typedef struct {
    double target_edge_length;    // Target triangle edge length (m)
    int max_facets;              // Maximum number of facets
    double min_quality;          // Minimum triangle quality
    bool adaptive_refinement;    // Enable adaptive refinement
    char mesh_algorithm[32];     // "delaunay", "frontal", "meshadapt"
} satellite_mesh_params_t;

// Solver configuration
typedef struct {
    char solver_type[16];        // "mom" or "peec"
    double frequency;             // Analysis frequency (Hz)
    int basis_order;              // RWG basis function order (1 or 2)
    char formulation[16];          // "efie", "mfie", "cfie"
    double tolerance;             // Solver tolerance
    int max_iterations;           // Maximum iterations
    bool use_preconditioner;     // Enable preconditioning
    bool use_fast_solver;        // Enable fast algorithms (MLFMM/ACA)
} satellite_solver_config_t;

// Observation points for field calculation
typedef struct {
    double* x;  // Array of x coordinates
    double* y;  // Array of y coordinates  
    double* z;  // Array of z coordinates
    int num_points;
} satellite_observation_points_t;

// RWG basis function data
typedef struct {
    int* triangle_plus;   // Positive triangle indices
    int* triangle_minus;  // Negative triangle indices
    int* edge_indices;    // Edge indices
    double* edge_lengths; // Edge lengths
    double* areas;        // Triangle areas
    int num_basis;
} satellite_rwg_basis_t;

// Current distribution results
typedef struct {
    complex_t* currents;     // Current coefficients (A)
    double* magnitude;            // Current magnitude
    double* phase;                // Current phase (radians)
    int num_basis;
} satellite_currents_t;

// Field results
typedef struct {
    complex_t* e_field;      // Electric field (V/m)
    complex_t* h_field;      // Magnetic field (A/m)
    double* e_magnitude;          // E-field magnitude
    double* h_magnitude;          // H-field magnitude
    int num_points;
} satellite_fields_t;

// Radar cross section results
typedef struct {
    double* theta_angles;         // Theta angles (degrees)
    double* phi_angles;           // Phi angles (degrees)
    double* rcs_values;           // RCS values (m²)
    int num_theta;
    int num_phi;
} satellite_rcs_t;

// Performance metrics
typedef struct {
    double mesh_generation_time;    // Mesh generation time (s)
    double matrix_assembly_time;    // Matrix assembly time (s)
    double solver_time;            // Solver time (s)
    double total_time;             // Total time (s)
    size_t memory_usage;          // Memory usage (bytes)
    int num_unknowns;              // Number of unknowns
    bool converged;               // Convergence status
} satellite_performance_t;

// Main simulation handle
typedef struct satellite_simulation satellite_simulation_t;

/*********************************************************************
 * Core Interface Functions
 *********************************************************************/

// Simulation lifecycle
satellite_simulation_t* satellite_simulation_create(void);
void satellite_simulation_destroy(satellite_simulation_t* sim);

// Geometry and mesh
satellite_error_t satellite_load_stl(satellite_simulation_t* sim, const char* stl_filename);
satellite_error_t satellite_set_material(satellite_simulation_t* sim, const satellite_material_t* material);
satellite_error_t satellite_generate_mesh(satellite_simulation_t* sim, const satellite_mesh_params_t* params);
satellite_error_t satellite_get_mesh_info(satellite_simulation_t* sim, int* num_vertices, int* num_triangles);

// Solver configuration  
satellite_error_t satellite_configure_solver(satellite_simulation_t* sim, const satellite_solver_config_t* config);
satellite_error_t satellite_set_excitation(satellite_simulation_t* sim, const satellite_excitation_t* excitation);

// Simulation execution
satellite_error_t satellite_run_simulation(satellite_simulation_t* sim);
satellite_error_t satellite_solve_frequency_sweep(satellite_simulation_t* sim, double* frequencies, int num_freqs);

// Results retrieval
satellite_error_t satellite_get_currents(satellite_simulation_t* sim, satellite_currents_t** currents);
satellite_error_t satellite_get_fields(satellite_simulation_t* sim, const satellite_observation_points_t* points, satellite_fields_t** fields);
satellite_error_t satellite_get_rcs(satellite_simulation_t* sim, satellite_rcs_t** rcs);
satellite_error_t satellite_get_performance(satellite_simulation_t* sim, satellite_performance_t** performance);

// RWG basis functions
satellite_error_t satellite_get_rwg_basis(satellite_simulation_t* sim, satellite_rwg_basis_t** basis);

// Memory management for results
void satellite_free_currents(satellite_currents_t* currents);
void satellite_free_fields(satellite_fields_t* fields);
void satellite_free_rcs(satellite_rcs_t* rcs);
void satellite_free_performance(satellite_performance_t* performance);
void satellite_free_rwg_basis(satellite_rwg_basis_t* basis);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

// Error handling
const char* satellite_get_error_string(satellite_error_t error);
const char* satellite_get_version(void);

// File I/O
satellite_error_t satellite_export_results(satellite_simulation_t* sim, const char* filename, const char* format);
satellite_error_t satellite_import_mesh(satellite_simulation_t* sim, const char* mesh_filename);

// Mesh operations
satellite_error_t satellite_translate_geometry(satellite_simulation_t* sim, double dx, double dy, double dz);
satellite_error_t satellite_scale_geometry(satellite_simulation_t* sim, double scale_factor);

// Validation
satellite_error_t satellite_validate_mesh(satellite_simulation_t* sim, double* min_quality, double* max_quality);
satellite_error_t satellite_check_convergence(satellite_simulation_t* sim, double* residual_norm);

/*********************************************************************
 * Advanced Functions for Professional Use
 *********************************************************************/

// Matrix access for debugging
satellite_error_t satellite_get_impedance_matrix_size(satellite_simulation_t* sim, int* size);
satellite_error_t satellite_get_impedance_matrix(satellite_simulation_t* sim, complex_t** matrix, int* size);

// Frequency domain analysis
satellite_error_t satellite_compute_far_field(satellite_simulation_t* sim, double theta_min, double theta_max, int n_theta,
                                            double phi_min, double phi_max, int n_phi);

// Time domain analysis (for PEEC)
satellite_error_t satellite_compute_time_domain(satellite_simulation_t* sim, double* time_points, int num_points, complex_t** time_response);

// Optimization
satellite_error_t satellite_set_optimization_params(satellite_simulation_t* sim, bool enable_adaptive_mesh, double refinement_threshold);

#ifdef __cplusplus
}
#endif

#endif // SATELLITE_MOM_PEEC_INTERFACE_H