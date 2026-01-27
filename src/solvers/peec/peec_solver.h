/********************************************************************************
 * PEEC Solver Interface
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Partial Element Equivalent Circuit solver for electromagnetic analysis
 ********************************************************************************/

#ifndef PEEC_SOLVER_H
#define PEEC_SOLVER_H

#include "../../common/core_common.h"
#include "../../physics/peec/peec_physics.h"  // L1 layer: physical definitions

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct peec_solver peec_solver_t;
typedef struct peec_config peec_config_t;
typedef struct peec_result peec_result_t;

// Include layered_greens_function.h for type definitions
// Use include guard to prevent multiple inclusions
#ifndef LAYERED_GREENS_FUNCTION_H
#include "../../operators/greens/layered_greens_function.h"
#endif

#if defined(_MSC_VER)
typedef complex_t peec_scalar_complex_t;
#else
#include <complex.h>
typedef double complex peec_scalar_complex_t;
#endif

// PEEC configuration
typedef struct peec_config {
    peec_formulation_t formulation; // PEEC formulation type (from physics layer)
    double frequency;            // Analysis frequency (Hz)
    bool extract_resistance;     // Extract resistance
    bool extract_inductance;     // Extract inductance
    bool extract_capacitance;    // Extract capacitance
    bool extract_mutual_inductance;   // Extract mutual inductance
    bool extract_mutual_capacitance;  // Extract mutual capacitance
    int mesh_density;            // Elements per wavelength
    double circuit_tolerance;    // Circuit solver tolerance
    int circuit_max_iterations;  // Maximum circuit iterations
    bool use_parallel;           // Enable parallel processing
    int num_threads;             // Number of threads
    bool export_spice;           // Export SPICE netlist
    bool use_layered_medium;     // Use layered medium kernels
    bool use_iterative_capacitance; // Use iterative method to approximate C=P^{-1}
    double cg_tolerance;           // Conjugate gradient tolerance
    int cg_max_iterations;         // Conjugate gradient max iterations
    double cg_drop_tolerance;      // Drop threshold in CG mat-vec for sparsity
} peec_config_t;

// Formulation types - use peec_formulation_t from peec_physics.h

// Result structure
typedef struct peec_result {
    // Partial elements
    double* resistance_matrix;      // Resistance matrix (Ohms)
    double* inductance_matrix;      // Inductance matrix (H)
    double* capacitance_matrix;     // Capacitance matrix (F)
    double* potential_coeff_matrix; // Potential coefficient matrix (1/F)
    
    // Circuit solution
    peec_scalar_complex_t* node_voltages;  // Node voltages (V)
    peec_scalar_complex_t* branch_currents; // Branch currents (A)
    int num_nodes;                  // Number of nodes
    int num_branches;               // Number of branches
    
    // Current distribution
    peec_scalar_complex_t* current_coefficients;  // Current coefficients
    int num_basis_functions;                // Number of basis functions
    
    // Performance metrics
    double extraction_time;         // Partial element extraction time (s)
    double solve_time;              // Circuit solution time (s)
    int iterations;                 // Number of iterations
    bool converged;               // Convergence status
} peec_result_t;

// Solver lifecycle
peec_solver_t* peec_solver_create(const peec_config_t* config);
void peec_solver_destroy(peec_solver_t* solver);

// Configuration
int peec_solver_configure(peec_solver_t* solver, const peec_config_t* config);
int peec_solver_set_layered_medium(peec_solver_t* solver,
                                   const LayeredMedium* medium,
                                   const FrequencyDomain* freq,
                                   const GreensFunctionParams* params);

// Geometry and mesh
int peec_solver_import_cad(peec_solver_t* solver, const char* filename, const char* format);
int peec_solver_set_mesh(peec_solver_t* solver, void* mesh);  // Generic mesh pointer

// Partial element extraction
int peec_solver_extract_partial_elements(peec_solver_t* solver);

// Circuit network building
int peec_solver_build_circuit_network(peec_solver_t* solver);

// Circuit solution
int peec_solver_solve_circuit(peec_solver_t* solver);

// Results
const peec_result_t* peec_solver_get_results(const peec_solver_t* solver);

// Utility functions
int peec_solver_get_num_nodes(const peec_solver_t* solver);
int peec_solver_get_num_branches(const peec_solver_t* solver);
double peec_solver_get_memory_usage(const peec_solver_t* solver);

// SPICE export
int peec_solver_export_spice(const peec_solver_t* solver, const char* filename);

int peec_solver_add_port_surface_coupling(peec_solver_t* solver,
                                          int port_node,
                                          int element_index,
                                          const double point_xyz[3],
                                          double frequency);

int peec_solver_batch_port_surface_coupling(peec_solver_t* solver,
                                            const int* port_nodes,
                                            int num_ports,
                                            const int* element_indices,
                                            int num_elements,
                                            const double* points_xyz,  // flattened array length 3*num_ports
                                            double frequency);

int peec_solver_add_port_line_coupling(peec_solver_t* solver,
                                       int port_node,
                                       int element_index,
                                       const double point_xyz[3],
                                       double frequency);

int peec_solver_batch_port_line_coupling(peec_solver_t* solver,
                                         const int* port_nodes,
                                         int num_ports,
                                         const int* element_indices,
                                         int num_elements,
                                         const double* points_xyz,
                                         double frequency);

int peec_solver_add_line_surface_coupling(peec_solver_t* solver,
                                          int line_element_index,
                                          int surface_element_index,
                                          double frequency);

int peec_solver_batch_line_surface_coupling(peec_solver_t* solver,
                                            const int* line_element_indices,
                                            int num_lines,
                                            const int* surface_element_indices,
                                            int num_surfaces,
                                            double frequency);

int peec_solver_export_couplings_csv(peec_solver_t* solver, const char* filename);

int peec_solver_export_nodes_csv(peec_solver_t* solver, const char* filename);

int peec_solver_export_branches_csv(peec_solver_t* solver, const char* filename);

int peec_solver_selftest_couplings(void);

#ifdef __cplusplus
}
#endif

#endif // PEEC_SOLVER_H
