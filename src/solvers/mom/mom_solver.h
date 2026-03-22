/********************************************************************************
 * MoM Solver Interface
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Method of Moments solver for electromagnetic scattering analysis
 ********************************************************************************/

#ifndef MOM_SOLVER_H
#define MOM_SOLVER_H

#include "../../common/core_common.h"
#include "../../physics/mom/mom_physics.h"  // L1 layer: physical definitions
#include "../../operators/greens/layered_greens_function.h"  // Include to get LayeredMedium, FrequencyDomain, GreensFunctionParams

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct mom_solver mom_solver_t;
typedef struct mom_config mom_config_t;
typedef struct mom_excitation mom_excitation_t;
typedef struct mom_result mom_result_t;
// LayeredMedium, FrequencyDomain, GreensFunctionParams are defined in layered_greens_function.h

#if defined(_MSC_VER)
typedef complex_t mom_scalar_complex_t;
#else
#include <complex.h>
typedef double complex mom_scalar_complex_t;
#endif

// MoM configuration
typedef struct mom_config {
    int basis_type;              // RWG basis function type
    mom_formulation_t formulation; // EFIE, MFIE, CFIE (from physics layer)
    double frequency;            // Analysis frequency (Hz)
    int mesh_density;            // Elements per wavelength
    double edge_length;          // Target edge length (m)
    double tolerance;            // Solver tolerance
    int max_iterations;          // Maximum iterations
    bool use_preconditioner;     // Use preconditioning
    bool use_parallel;           // Enable parallel processing
    int num_threads;             // Number of threads
    bool compute_far_field;      // Compute far field
    bool compute_near_field;       // Compute near field
    bool compute_current_distribution; // Compute current distribution
    int gmres_restart;           // GMRES restart parameter
    double drop_tolerance;       // CSR drop tolerance
    bool use_sparse_solver;      // Use sparse GMRES+ILU path when iterations>0
    double assembly_drop_tolerance; // Drop small Z_ij during assembly
    double near_threshold;          // Near-singularity distance threshold
    int near_quadrature_points;     // Reserved for higher quadrature
    bool enable_duffy;              // Enable Duffy transform on near singularities
    double far_threshold_factor;    // Distance factor to treat pairs as far
    int far_quadrature_points;      // Use low-order quadrature for far interactions
    bool enable_self_term_analytic; // Enable analytic diagonal self term
    bool enable_aca;                // Enable ACA simplification after assembly
    double aca_tolerance;           // ACA tolerance
    int aca_max_rank;               // ACA maximum rank
} mom_config_t;

// Basis function types
#define MOM_BASIS_RWG 1
#define MOM_BASIS_ROOFTOP 2

// Formulation types - use mom_formulation_t from mom_physics.h
// Excitation types - use mom_excitation_type_t from mom_physics.h

// Excitation structure
typedef struct mom_excitation {
    int type;                    // Excitation type
    double frequency;            // Frequency (Hz)
    double amplitude;              // Amplitude
    double phase;                  // Phase (radians)
    point3d_t k_vector;          // Wave vector (for plane wave)
    point3d_t polarization;        // Polarization vector
    int source_index;              // Source index (for lumped sources)
} mom_excitation_t;

// Result structure
typedef struct mom_result {
    mom_scalar_complex_t* current_coefficients;  // Current coefficients
    double* current_magnitude;              // Current magnitudes
    double* current_phase;                  // Current phases
    int num_basis_functions;                // Number of basis functions
    
    // Near field data
    struct {
        mom_scalar_complex_t* e_field;  // Electric field (V/m)
        mom_scalar_complex_t* h_field;  // Magnetic field (A/m)
        int num_points;         // Number of field points
    } near_field;
    
    // Far field data
    struct {
        double* theta_angles;     // Theta angles (degrees)
        double* phi_angles;       // Phi angles (degrees)
        mom_scalar_complex_t* e_theta;  // E-theta component
        mom_scalar_complex_t* e_phi;    // E-phi component
        int num_theta;            // Number of theta points
        int num_phi;              // Number of phi points
    } far_field;
    
    // Radar cross section
    double* rcs_values;           // RCS values (m²)
    int num_rcs_points;           // Number of RCS points
    
    // Performance metrics
    double matrix_fill_time;      // Matrix filling time (s)
    double solve_time;            // Solution time (s)
    int iterations;               // Number of iterations
    bool converged;               // Convergence status
} mom_result_t;

// Solver lifecycle
mom_solver_t* mom_solver_create(const mom_config_t* config);
void mom_solver_destroy(mom_solver_t* solver);

// Configuration
int mom_solver_configure(mom_solver_t* solver, const mom_config_t* config);

// Geometry and mesh
int mom_solver_import_cad(mom_solver_t* solver, const char* filename, const char* format);
int mom_solver_import_msh(mom_solver_t* solver, const char* filename);  // Direct .msh mesh import
int mom_solver_set_geometry(mom_solver_t* solver, void* geometry);  // Generic geometry pointer
int mom_solver_set_mesh(mom_solver_t* solver, void* mesh);  // Generic mesh pointer

// Excitation
int mom_solver_add_excitation(mom_solver_t* solver, const mom_excitation_t* excitation);
int mom_solver_add_lumped_excitation(mom_solver_t* solver, const point3d_t* position,
                                     const point3d_t* polarization,
                                     double amplitude, double width, int layer_index);
// Port template support (requires port_support_extended.h)
struct extended_port_t;
int mom_solver_add_port(mom_solver_t* solver, struct extended_port_t* port);

// Matrix assembly
int mom_solver_assemble_matrix(mom_solver_t* solver);

// Solution
int mom_solver_solve(mom_solver_t* solver);
int mom_solver_enable_preconditioner(mom_solver_t* solver, bool enable);
int mom_solver_compute_port_current(mom_solver_t* solver, double px, double py, double width, int layer_index, double* Iport);

// Field computation
int mom_solver_compute_near_field(mom_solver_t* solver, const point3d_t* points, int num_points);
int mom_solver_compute_far_field(mom_solver_t* solver, double theta_min, double theta_max, int n_theta,
                                 double phi_min, double phi_max, int n_phi);

/**
 * Monostatic RCS (m²) for plane-wave excitation: observation along -k̂ (same as incident propagation axis).
 * Uses RWG far-field post-processing; requires successful solve. Returns 0 on failure.
 */
double mom_solver_monostatic_rcs_m2(mom_solver_t* solver);

// Results
const mom_result_t* mom_solver_get_results(const mom_solver_t* solver);

// Utility functions
int mom_solver_get_num_unknowns(const mom_solver_t* solver);
double mom_solver_get_memory_usage(const mom_solver_t* solver);

/* Export surface current (per element) with geometry information for visualization.
 * CSV columns:
 *   element_index, cx, cy, cz, Re_J, Im_J, magnitude_J
 */
int mom_solver_export_surface_current(const mom_solver_t* solver, const char* csv_path);

/* Export surface mesh for 3D plot: vertices + triangle faces with current magnitude.
 * File format: first line "num_vertices num_triangles", then num_vertices lines "x y z",
 * then num_triangles lines "i j k mag" (0-based vertex indices). Used by plot_trisurf.
 */
int mom_solver_export_surface_mesh_for_plot(const mom_solver_t* solver, const char* txt_path);

/* Export surface current to VTK (continuous triangle mesh + CELL_DATA) for ParaView.
 * conductor_material_id: -1 = all conductor; >=0 = zero current on elements with material_id != this.
 */
int mom_solver_export_surface_current_vtk(const mom_solver_t* solver, const char* vtk_path, int conductor_material_id);

// Layered medium integration
int mom_solver_set_layered_medium(mom_solver_t* solver,
                                  const LayeredMedium* medium,
                                  const FrequencyDomain* freq,
                                  const GreensFunctionParams* params);

#ifdef __cplusplus
}
#endif

#endif // MOM_SOLVER_H
