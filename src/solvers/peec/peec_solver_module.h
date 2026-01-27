/*********************************************************************
 * PEEC-MoM Unified Framework - PEEC Solver Module
 * 
 * This module implements the Partial Element Equivalent Circuit
 * solver with advanced circuit extraction and coupling capabilities.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#ifndef PEEC_SOLVER_MODULE_H
#define PEEC_SOLVER_MODULE_H

#include "../../operators/kernels/electromagnetic_kernel_library.h"
#include "../../physics/peec/peec_physics.h"  // L1 layer: physical definitions

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * PEEC Configuration and Options
 *********************************************************************/
typedef enum {
    PEEC_ELEMENT_WIRE,          // Wire segment
    PEEC_ELEMENT_SURFACE,       // Surface patch
    PEEC_ELEMENT_VOLUME,        // Volume cell
    PEEC_ELEMENT_VIA,           // Via structure
    PEEC_ELEMENT_PORT           // Circuit port
} PeecElementType;

// Note: PEEC formulation types are defined in physics/peec/peec_physics.h
// Use peec_formulation_t enum from physics layer instead of PeecFormulationType
typedef peec_formulation_t PeecFormulationType;  // Alias for backward compatibility

typedef enum {
    PEEC_CIRCUIT_SOLVER_MNA,    // Modified Nodal Analysis
    PEEC_CIRCUIT_SOLVER_MESH,   // Mesh analysis
    PEEC_CIRCUIT_SOLVER_STATE_SPACE // State-space formulation
} PeecCircuitSolverType;

typedef enum {
    PEEC_TIME_DOMAIN_TRANSIENT, // Transient analysis
    PEEC_TIME_DOMAIN_IMPULSE,   // Impulse response
    PEEC_TIME_DOMAIN_STEP,      // Step response
    PEEC_FREQUENCY_DOMAIN_AC,   // AC analysis
    PEEC_FREQUENCY_DOMAIN_DC    // DC analysis
} PeecAnalysisType;

typedef struct {
    // Element settings
    PeecElementType element_type;
    peec_formulation_t formulation;  // Use physics layer type
    bool include_retardation;
    bool include_radiation;
    bool include_losses;
    
    // Discretization settings
    Real min_segment_length;
    Real max_segment_length;
    Real skin_depth_ratio;
    int adaptive_refinement_levels;
    
    // Circuit solver settings
    PeecCircuitSolverType circuit_solver;
    PeecAnalysisType analysis_type;
    int max_circuit_iterations;
    Real circuit_convergence_tolerance;
    
    // Time domain settings
    Real time_start;
    Real time_stop;
    Real time_step;
    int num_time_points;
    bool use_adaptive_time_stepping;
    
    // Frequency domain settings
    Real frequency_start;
    Real frequency_stop;
    int num_frequency_points;
    bool use_logarithmic_spacing;
    
    // Accuracy settings
    Real accuracy_target;
    bool use_adaptive_accuracy;
    int refinement_levels;
    
    // Performance settings
    int num_threads;
    bool use_gpu;
    bool use_sparse_matrices;
    size_t memory_limit;
    
    // Output settings
    bool save_circuit_netlist;
    bool save_impedance_matrix;
    bool compute_s_parameters;
    bool compute_y_parameters;
    bool compute_z_parameters;
} PeecSolverOptions;

/*********************************************************************
 * PEEC Element Structures
 *********************************************************************/
typedef struct {
    int element_id;
    PeecElementType type;
    
    // Node connections
    int node_plus;              // Positive node
    int node_minus;             // Negative node
    int* additional_nodes;      // For higher-order elements
    int num_additional_nodes;
    
    // Geometric properties
    Real length;                // For wire elements
    Real width;                 // For surface elements
    Real height;                // For volume elements
    Real cross_section_area;
    Real surface_area;
    Real volume;
    
    // Material properties
    int material_id;
    Real conductivity;
    Real permeability;
    Real permittivity;
    
    // Electrical properties (computed)
    Real resistance;            // R
    Real self_inductance;       // L
    Real self_capacitance;      // C
    Real conductance;           // G
    
    // Frequency-dependent properties
    Complex impedance;
    Complex admittance;
    
    // Position and orientation
    Real centroid[3];
    Real direction[3];
    Real normal[3];
    
    // Flags
    bool is_active;
    bool is_ground;
    bool is_port;
    bool is_lumped;
} PeecElement;

typedef struct {
    int from_element;
    int to_element;
    Complex mutual_inductance;  // L_ij
    Complex mutual_capacitance; // C_ij
    Complex mutual_resistance;  // R_ij (if applicable)
    Real distance;
    Real coupling_strength;
} PeecCoupling;

/*********************************************************************
 * PEEC Circuit Structures
 *********************************************************************/
typedef struct {
    int node_id;
    char name[64];
    Real voltage;               // DC or RMS value
    Complex voltage_ac;         // AC phasor
    Real current;               // Current through node
    Complex current_ac;         // AC current phasor
    bool is_ground;
    bool is_port;
    bool is_external;
    int* connected_elements;
    int num_connected_elements;
} PeecNode;

typedef struct {
    int branch_id;
    int from_node;
    int to_node;
    char name[64];
    
    // Branch elements
    Real series_resistance;
    Real series_inductance;
    Real parallel_capacitance;
    Real parallel_conductance;
    
    // Controlled sources
    int controlling_branch;     // For controlled sources
    Real control_factor;
    
    // Independent sources
    Real voltage_source;
    Real current_source;
    Complex voltage_source_ac;
    Complex current_source_ac;
    
    bool is_active;
    bool is_controlling;
    bool is_controlled;
} PeecBranch;

typedef struct {
    Complex* G_matrix;          // Conductance matrix
    Complex* C_matrix;          // Capacitance matrix
    Complex* L_matrix;          // Inductance matrix
    Complex* R_matrix;          // Resistance matrix
    
    Complex* B_matrix;          // Source connectivity
    Complex* A_matrix;          // Incidence matrix
    
    int matrix_size;
    int num_nodes;
    int num_branches;
    bool is_sparse;
    size_t memory_usage;
} PeecCircuitMatrix;

/*********************************************************************
 * PEEC Geometry and Mesh
 *********************************************************************/
typedef struct {
    // Wire geometry
    Real* wire_vertices;      // 3D coordinates
    int num_wire_vertices;
    int* wire_segments;         // Connectivity
    int num_wire_segments;
    
    // Surface geometry
    Real* surface_vertices;
    int num_surface_vertices;
    int* surface_triangles;     // Triangle connectivity
    int num_surface_triangles;
    
    // Volume geometry
    Real* volume_vertices;
    int num_volume_vertices;
    int* volume_tetrahedra;     // Tetrahedron connectivity
    int num_volume_tetrahedra;
    
    // Via geometry
    Real* via_positions;
    Real* via_radii;
    Real* via_heights;
    int num_vias;
    
    // Layer information
    int* layer_assignments;
    int num_layers;
    Real* layer_thicknesses;
    Real* layer_elevations;
} PeecGeometry;

typedef struct {
    PeecElement* elements;
    int num_elements;
    int max_elements;
    
    PeecCoupling* couplings;
    int num_couplings;
    int max_couplings;
    
    // Discretization parameters
    Real min_segment_length;
    Real max_segment_length;
    Real skin_depth_ratio;
    
    // Quality metrics
    Real average_aspect_ratio;
    Real min_aspect_ratio;
    Real max_aspect_ratio;
    
    // Memory usage
    size_t memory_usage;
} PeecMesh;

/*********************************************************************
 * PEEC Solver Results
 *********************************************************************/
typedef struct {
    // Convergence information
    int num_iterations;
    Real final_residual;
    bool converged;
    Real convergence_time;
    
    // Solution quality
    Real solution_error;
    Real max_voltage_error;
    Real max_current_error;
    
    // Performance metrics
    double assembly_time;
    double solve_time;
    double total_time;
    size_t memory_usage;
    
    // Circuit metrics
    Real total_power;
    Real dissipated_power;
    Real stored_energy;
    
    // Frequency response
    Complex* frequency_response;
    int num_frequency_points;
    Real* frequencies;
} PeecSolverResults;

/*********************************************************************
 * S-Parameter and Network Analysis
 *********************************************************************/
typedef struct {
    int port1;
    int port2;
    Complex s11, s12, s21, s22;
    Real frequency;
    Real characteristic_impedance;
} PeecSParameters;

typedef struct {
    Complex z11, z12, z21, z22;
    Complex y11, y12, y21, y22;
    Complex abcd11, abcd12, abcd21, abcd22;
    Real frequency;
} PeecNetworkParameters;

/*********************************************************************
 * Main PEEC Solver Structure
 *********************************************************************/
typedef struct PeecSolver PeecSolver;

struct PeecSolver {
    // Configuration
    PeecSolverOptions options;
    
    // Geometry and mesh
    PeecGeometry* geometry;
    PeecMesh* mesh;
    
    // Circuit components
    PeecNode* nodes;
    int num_nodes;
    PeecBranch* branches;
    int num_branches;
    
    // System matrices
    PeecCircuitMatrix* circuit_matrix;
    
    // Solution vectors
    Complex* node_voltages;
    Complex* branch_currents;
    Complex* node_voltages_dc;
    Complex* branch_currents_dc;
    
    // Time domain storage
    Complex** time_domain_voltages;
    Complex** time_domain_currents;
    Real* time_points;
    int current_time_step;
    
    // Results
    PeecSolverResults* results;
    PeecSParameters* s_parameters;
    PeecNetworkParameters* network_parameters;
    
    // Internal state
    bool is_assembled;
    bool is_solved;
    bool is_initialized;
    
    // Performance monitoring
    Timer assembly_timer;
    Timer solve_timer;
    Timer total_timer;
    
    // Memory usage
    size_t peak_memory_usage;
    size_t current_memory_usage;
    
    // Thread safety
    void* mutex;
};

/*********************************************************************
 * PEEC Solver API Functions
 *********************************************************************/

// Lifecycle functions
PeecSolver* peec_solver_create(const PeecSolverOptions* options);
void peec_solver_destroy(PeecSolver* solver);

StatusCode peec_solver_initialize(PeecSolver* solver, GeometryEngine* geometry, 
                                MeshEngine* mesh_engine);
StatusCode peec_solver_finalize(PeecSolver* solver);

// Geometry and mesh setup
StatusCode peec_solver_set_geometry(PeecSolver* solver, PeecGeometry* geometry);
StatusCode peec_solver_set_mesh(PeecSolver* solver, PeecMesh* mesh);
StatusCode peec_solver_set_materials(PeecSolver* solver, MaterialDatabase* materials);

// Element creation and modification
StatusCode peec_solver_add_wire_element(PeecSolver* solver, 
                                        const Real* start_point, const Real* end_point,
                                        Real radius, int material_id);
StatusCode peec_solver_add_surface_element(PeecSolver* solver,
                                         const int* vertex_ids, int num_vertices,
                                         int material_id);
StatusCode peec_solver_add_port(PeecSolver* solver, int node_plus, int node_minus,
                              Real impedance, const char* name);
StatusCode peec_solver_add_voltage_source(PeecSolver* solver, int from_node, int to_node,
                                        Complex voltage, Real frequency);
StatusCode peec_solver_add_current_source(PeecSolver* solver, int from_node, int to_node,
                                        Complex current, Real frequency);

// Matrix assembly
StatusCode peec_solver_assemble_matrices(PeecSolver* solver);
StatusCode peec_solver_assemble_partial_elements(PeecSolver* solver);
StatusCode peec_solver_assemble_couplings(PeecSolver* solver);

// Solution
StatusCode peec_solver_solve_dc(PeecSolver* solver);
StatusCode peec_solver_solve_ac(PeecSolver* solver, Real frequency);
StatusCode peec_solver_solve_transient(PeecSolver* solver);
StatusCode peec_solver_solve_frequency_sweep(PeecSolver* solver);

// Post-processing
StatusCode peec_solver_compute_s_parameters(PeecSolver* solver);
StatusCode peec_solver_compute_network_parameters(PeecSolver* solver);
StatusCode peec_solver_compute_power(PeecSolver* solver);
StatusCode peec_solver_compute_currents(PeecSolver* solver);
StatusCode peec_solver_compute_voltages(PeecSolver* solver);

// Results access
const PeecSolverResults* peec_solver_get_results(const PeecSolver* solver);
const Complex* peec_solver_get_node_voltages(const PeecSolver* solver);
const Complex* peec_solver_get_branch_currents(const PeecSolver* solver);
const PeecSParameters* peec_solver_get_s_parameters(const PeecSolver* solver);
const PeecNetworkParameters* peec_solver_get_network_parameters(const PeecSolver* solver);

// Circuit export
StatusCode peec_solver_export_spice_netlist(PeecSolver* solver, const char* filename);
StatusCode peec_solver_export_s_parameters(PeecSolver* solver, const char* filename);
StatusCode peec_solver_export_impedance_matrix(PeecSolver* solver, const char* filename);

// Utility functions
StatusCode peec_solver_set_options(PeecSolver* solver, const PeecSolverOptions* options);
StatusCode peec_solver_get_options(const PeecSolver* solver, PeecSolverOptions* options);
StatusCode peec_solver_print_info(const PeecSolver* solver);
StatusCode peec_solver_print_statistics(const PeecSolver* solver);

// Advanced functions
StatusCode peec_solver_adapt_mesh(PeecSolver* solver, Real error_threshold);
StatusCode peec_solver_refine_solution(PeecSolver* solver, Real accuracy_target);
StatusCode peec_solver_validate_solution(PeecSolver* solver);

// Memory and performance
size_t peec_solver_get_memory_usage(const PeecSolver* solver);
size_t peec_solver_get_peak_memory_usage(const PeecSolver* solver);
StatusCode peec_solver_reset_performance_counters(PeecSolver* solver);

/*********************************************************************
 * Partial Element Extraction Functions
 *********************************************************************/
typedef struct {
    Real self_inductance;
    Real mutual_inductance;
    Real self_capacitance;
    Real mutual_capacitance;
    Real self_resistance;
    Real mutual_resistance;
    Real coupling_coefficient;
    Real distance;
} PartialElements;

// Partial element calculation functions
PartialElements compute_partial_inductance(const PeecElement* element1, 
                                         const PeecElement* element2,
                                         Real frequency);
PartialElements compute_partial_capacitance(const PeecElement* element1,
                                          const PeecElement* element2,
                                          Real frequency);
PartialElements compute_partial_resistance(const PeecElement* element1,
                                         const PeecElement* element2,
                                         Real frequency);

// Skin effect and proximity effect
Complex compute_skin_effect_impedance(const PeecElement* element, Real frequency);
Complex compute_proximity_effect_impedance(const PeecElement* element1,
                                         const PeecElement* element2,
                                         Real frequency);

/*********************************************************************
 * MoM-PEEC Coupling Interface
 *********************************************************************/
typedef struct {
    int mom_triangle_id;
    int peec_element_id;
    Complex coupling_coefficient;
    Real distance;
    Real interaction_strength;
} MomPeecCoupling;

typedef struct {
    MomPeecCoupling* couplings;
    int num_couplings;
    Complex* coupling_matrix;
    bool is_symmetric;
    size_t memory_usage;
} MomPeecInterface;

// Coupling interface functions
MomPeecInterface* mom_peec_interface_create(void);
void mom_peec_interface_destroy(MomPeecInterface* interface);
StatusCode mom_peec_interface_compute_couplings(MomPeecInterface* interface,
                                            const MomSolver* mom_solver,
                                            const PeecSolver* peec_solver);
StatusCode mom_peec_interface_assemble_coupling_matrix(MomPeecInterface* interface);
StatusCode mom_peec_interface_solve_coupled_system(MomPeecInterface* interface,
                                                   MomSolver* mom_solver,
                                                   PeecSolver* peec_solver);

#ifdef __cplusplus
}
#endif

#endif // PEEC_SOLVER_MODULE_H