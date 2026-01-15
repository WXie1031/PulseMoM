/*********************************************************************
 * PEEC-MoM Unified Framework - Hybrid Coupling Interface
 * 
 * This module implements the coupling interface between PEEC and MoM
 * solvers for mixed-domain electromagnetic simulations.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#ifndef HYBRID_COUPLING_INTERFACE_H
#define HYBRID_COUPLING_INTERFACE_H

#include "../core/electromagnetic_kernel_library.h"
#include "../solvers/mom/mom_solver_module.h"
#include "../solvers/peec/peec_solver_module.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * Hybrid Coupling Configuration
 *********************************************************************/
typedef enum {
    COUPLING_METHOD_SCHUR_COMPLEMENT,    // Schur complement method
    COUPLING_METHOD_DOMAIN_DECOMPOSITION, // Domain decomposition
    COUPLING_METHOD_ITERATIVE_SUBDOMAIN, // Iterative subdomain
    COUPLING_METHOD_LAGRANGE_MULTIPLIERS, // Lagrange multipliers
    COUPLING_METHOD_PENALTY_METHOD,    // Penalty method
    COUPLING_METHOD_MORTAR_METHOD        // Mortar method
} HybridCouplingMethod;

typedef enum {
    COUPLING_TYPE_ELECTRIC_FIELD,        // Electric field coupling
    COUPLING_TYPE_MAGNETIC_FIELD,        // Magnetic field coupling
    COUPLING_TYPE_CURRENT_DENSITY,       // Current density coupling
    COUPLING_TYPE_VOLTAGE_POTENTIAL,     // Voltage potential coupling
    COUPLING_TYPE_POWER_FLOW,            // Power flow coupling
    COUPLING_TYPE_MIXED                  // Mixed coupling
} HybridCouplingType;

typedef enum {
    COUPLING_DOMAIN_PEEC_TO_MOM,         // PEEC domain to MoM domain
    COUPLING_DOMAIN_MOM_TO_PEEC,         // MoM domain to PEEC domain
    COUPLING_DOMAIN_BIDIRECTIONAL,       // Bidirectional coupling
    COUPLING_DOMAIN_OVERLAPPING          // Overlapping domains
} HybridCouplingDomain;

typedef struct {
    // Coupling method selection
    HybridCouplingMethod method;
    HybridCouplingType type;
    HybridCouplingDomain domain;
    
    // Convergence criteria
    int max_iterations;
    Real convergence_tolerance;
    Real relaxation_parameter;
    
    // Interface settings
    int num_interface_points;
    Real interface_tolerance;
    bool use_adaptive_interface;
    bool use_robin_interface;
    
    // Iterative solver settings
    bool use_gmres;
    bool use_bicgstab;
    int krylov_subspace_size;
    int restart_size;
    
    // Preconditioning
    bool use_preconditioner;
    int preconditioner_type;
    Real preconditioner_tolerance;
    
    // Parallel settings
    bool use_parallel_coupling;
    int num_coupling_threads;
    bool use_mpi;
    
    // Performance settings
    bool enable_profiling;
    bool save_intermediate_results;
    bool compute_interface_errors;
    
    // Accuracy settings
    Real accuracy_target;
    bool use_adaptive_accuracy;
    int adaptive_refinement_levels;
} HybridCouplingOptions;

/*********************************************************************
 * Interface Point Definition
 *********************************************************************/
typedef struct {
    int interface_id;
    int mom_entity_id;          // MoM triangle or edge ID
    int peec_entity_id;         // PEEC element or node ID
    
    // Physical coordinates
    Real position[3];
    Real normal[3];
    Real tangent[3];
    
    // Coupling coefficients
    Complex coupling_coefficient;
    Complex mom_to_peec_transfer;
    Complex peec_to_mom_transfer;
    
    // Field quantities
    Complex electric_field[3];
    Complex magnetic_field[3];
    Complex current_density[3];
    Complex voltage_potential;
    
    // Boundary conditions
    Complex dirichlet_value;
    Complex neumann_value;
    Complex robin_coefficient;
    
    // Error metrics
    Real interface_error;
    Real field_discontinuity;
    Real power_imbalance;
    
    // Flags
    bool is_active;
    bool is_dirichlet;
    bool is_neumann;
    bool is_robin;
    bool is_converged;
} HybridInterfacePoint;

typedef struct {
    HybridInterfacePoint* points;
    int num_points;
    int max_points;
    
    // Spatial indexing
    void* spatial_index;
    
    // Error statistics
    Real max_interface_error;
    Real avg_interface_error;
    Real rms_interface_error;
    
    // Convergence status
    bool is_converged;
    int iteration_count;
    
    // Memory usage
    size_t memory_usage;
} HybridInterface;

/*********************************************************************
 * Coupling Matrix Structures
 *********************************************************************/
typedef struct {
    // Coupling matrices
    Complex* mom_to_peec_matrix;    // MoM to PEEC transfer
    Complex* peec_to_mom_matrix;    // PEEC to MoM transfer
    Complex* interface_matrix;      // Interface coupling
    Complex* schur_complement;      // Schur complement matrix
    
    // Matrix dimensions
    int mom_size;
    int peec_size;
    int interface_size;
    
    // Matrix properties
    bool is_symmetric;
    bool is_positive_definite;
    Real condition_number;
    
    // Storage format
    int storage_format;             // Dense, sparse, compressed
    size_t memory_usage;
    
    // Factorization data
    void* lu_factorization;
    void* cholesky_factorization;
    void* qr_factorization;
} HybridCouplingMatrix;

/*********************************************************************
 * Iterative Coupling Data
 *********************************************************************/
typedef struct {
    // Current iteration solutions
    Complex* mom_solution;
    Complex* peec_solution;
    Complex* interface_solution;
    
    // Previous iteration solutions
    Complex* mom_solution_prev;
    Complex* peec_solution_prev;
    Complex* interface_solution_prev;
    
    // Residual vectors
    Complex* mom_residual;
    Complex* peec_residual;
    Complex* interface_residual;
    
    // Search directions (for Krylov methods)
    Complex* mom_search_direction;
    Complex* peec_search_direction;
    Complex* interface_search_direction;
    
    // Convergence history
    Real* residual_history;
    Real* error_history;
    int history_size;
    int current_iteration;
    
    // Relaxation parameters
    Real relaxation_parameter;
    Real* adaptive_relaxation;
    bool use_adaptive_relaxation;
} HybridIterationData;

/*********************************************************************
 * Main Hybrid Solver Structure
 *********************************************************************/
typedef struct HybridSolver HybridSolver;

struct HybridSolver {
    // Configuration
    HybridCouplingOptions options;
    
    // Component solvers
    MomSolver* mom_solver;
    PeecSolver* peec_solver;
    
    // Interface data
    HybridInterface* interface;
    HybridCouplingMatrix* coupling_matrix;
    HybridIterationData* iteration_data;
    
    // Solution data
    Complex* coupled_solution;
    int solution_size;
    
    // Results
    Real final_error;
    int num_iterations;
    bool is_converged;
    double total_time;
    
    // Performance monitoring
    Timer coupling_timer;
    Timer iteration_timer;
    Timer total_timer;
    
    // Memory usage
    size_t peak_memory_usage;
    size_t current_memory_usage;
    
    // Thread safety
    void* mutex;
    
    // Internal state
    bool is_initialized;
    bool is_assembled;
    bool is_solved;
};

/*********************************************************************
 * Hybrid Solver API Functions
 *********************************************************************/

// Lifecycle functions
HybridSolver* hybrid_solver_create(const HybridCouplingOptions* options);
void hybrid_solver_destroy(HybridSolver* solver);

StatusCode hybrid_solver_set_mom_solver(HybridSolver* solver, MomSolver* mom_solver);
StatusCode hybrid_solver_set_peec_solver(HybridSolver* solver, PeecSolver* peec_solver);

StatusCode hybrid_solver_initialize(HybridSolver* solver);
StatusCode hybrid_solver_finalize(HybridSolver* solver);

// Interface management
StatusCode hybrid_solver_create_interface(HybridSolver* solver, 
                                        const MomGeometry* mom_geometry,
                                        const PeecGeometry* peec_geometry);
StatusCode hybrid_solver_refine_interface(HybridSolver* solver, Real error_threshold);
StatusCode hybrid_solver_adapt_interface(HybridSolver* solver, int target_points);

// Coupling matrix assembly
StatusCode hybrid_solver_assemble_coupling_matrices(HybridSolver* solver);
StatusCode hybrid_solver_factorize_coupling_matrices(HybridSolver* solver);
StatusCode hybrid_solver_compress_coupling_matrices(HybridSolver* solver, Real tolerance);

// Solution methods
StatusCode hybrid_solver_solve_schur_complement(HybridSolver* solver);
StatusCode hybrid_solver_solve_domain_decomposition(HybridSolver* solver);
StatusCode hybrid_solver_solve_iterative_subdomain(HybridSolver* solver);
StatusCode hybrid_solver_solve_lagrange_multipliers(HybridSolver* solver);

// Iterative coupling
StatusCode hybrid_solver_iterate_coupling(HybridSolver* solver);
StatusCode hybrid_solver_check_convergence(HybridSolver* solver);
StatusCode hybrid_solver_update_interface(HybridSolver* solver);

// Error analysis
StatusCode hybrid_solver_compute_interface_errors(HybridSolver* solver);
StatusCode hybrid_solver_estimate_coupling_error(HybridSolver* solver, Real* error_estimate);
StatusCode hybrid_solver_validate_coupling(HybridSolver* solver);

// Results access
const Complex* hybrid_solver_get_coupled_solution(const HybridSolver* solver);
int hybrid_solver_get_solution_size(const HybridSolver* solver);
Real hybrid_solver_get_final_error(const HybridSolver* solver);
int hybrid_solver_get_num_iterations(const HybridSolver* solver);
bool hybrid_solver_is_converged(const HybridSolver* solver);

// Performance monitoring
size_t hybrid_solver_get_memory_usage(const HybridSolver* solver);
size_t hybrid_solver_get_peak_memory_usage(const HybridSolver* solver);
double hybrid_solver_get_total_time(const HybridSolver* solver);
StatusCode hybrid_solver_reset_performance_counters(HybridSolver* solver);

// Utility functions
StatusCode hybrid_solver_print_info(const HybridSolver* solver);
StatusCode hybrid_solver_print_statistics(const HybridSolver* solver);
StatusCode hybrid_solver_export_interface(HybridSolver* solver, const char* filename);
StatusCode hybrid_solver_export_coupling_matrices(HybridSolver* solver, const char* filename);

/*********************************************************************
 * Advanced Coupling Methods
 *********************************************************************/

// Schur complement method
typedef struct {
    Complex* schur_matrix;
    Complex* schur_rhs;
    Complex* interface_solution;
    int interface_size;
    bool is_factorized;
    void* factorization_data;
} SchurComplementData;

StatusCode schur_complement_create(SchurComplementData** data, int interface_size);
void schur_complement_destroy(SchurComplementData* data);
StatusCode schur_complement_assemble(SchurComplementData* data, 
                                   const HybridCouplingMatrix* coupling_matrix);
StatusCode schur_complement_factorize(SchurComplementData* data);
StatusCode schur_complement_solve(SchurComplementData* data, 
                                const Complex* rhs, Complex* solution);

// Domain decomposition method
typedef struct {
    int num_subdomains;
    int* subdomain_sizes;
    int* subdomain_offsets;
    Complex** subdomain_matrices;
    Complex** interface_matrices;
    bool is_parallel;
    void* mpi_communicator;
} DomainDecompositionData;

StatusCode domain_decomposition_create(DomainDecompositionData** data, int num_subdomains);
void domain_decomposition_destroy(DomainDecompositionData* data);
StatusCode domain_decomposition_setup(DomainDecompositionData* data,
                                    const HybridCouplingMatrix* coupling_matrix);
StatusCode domain_decomposition_solve(DomainDecompositionData* data,
                                    const Complex* global_rhs, Complex* global_solution);

// Iterative subdomain method
typedef struct {
    Real relaxation_parameter;
    Real adaptive_relaxation_factor;
    bool use_adaptive_relaxation;
    int max_local_iterations;
    Real local_convergence_tolerance;
    bool use_gmres_acceleration;
    int gmres_restart;
} IterativeSubdomainData;

StatusCode iterative_subdomain_create(IterativeSubdomainData** data);
void iterative_subdomain_destroy(IterativeSubdomainData* data);
StatusCode iterative_subdomain_setup(IterativeSubdomainData* data,
                                   const HybridCouplingOptions* options);
StatusCode iterative_subdomain_iterate(IterativeSubdomainData* data,
                                     HybridSolver* solver);

/*********************************************************************
 * Interface Mapping and Projection
 *********************************************************************/
typedef struct {
    // Projection operators
    Complex* mom_to_peec_projector;
    Complex* peec_to_mom_projector;
    
    // Interpolation matrices
    Complex* mom_interpolation_matrix;
    Complex* peec_interpolation_matrix;
    
    // Quadrature weights for integration
    Real* quadrature_weights;
    Real* quadrature_points;
    int num_quadrature_points;
    
    // Error estimation
    Real projection_error;
    Real interpolation_error;
    Real quadrature_error;
} InterfaceProjectionData;

StatusCode interface_projection_create(InterfaceProjectionData** data, 
                                     int mom_size, int peec_size);
void interface_projection_destroy(InterfaceProjectionData* data);
StatusCode interface_projection_compute_projectors(InterfaceProjectionData* data,
                                                 const HybridInterface* interface);
StatusCode interface_projection_mom_to_peec(const InterfaceProjectionData* data,
                                          const Complex* mom_solution, Complex* peec_solution);
StatusCode interface_projection_peec_to_mom(const InterfaceProjectionData* data,
                                          const Complex* peec_solution, Complex* mom_solution);

/*********************************************************************
 * Error Estimation and Adaptivity
 *********************************************************************/
typedef struct {
    // Error indicators
    Real* local_error_indicators;
    Real* global_error_indicators;
    int num_indicators;
    
    // Adaptivity criteria
    Real refinement_threshold;
    Real coarsening_threshold;
    int max_refinement_level;
    
    // Error norms
    Real l2_error;
    Real h1_error;
    Real energy_error;
    Real maximum_error;
    
    // Convergence rates
    Real convergence_rate;
    Real previous_error;
    int iteration_count;
} ErrorEstimationData;

StatusCode error_estimation_create(ErrorEstimationData** data, int size);
void error_estimation_destroy(ErrorEstimationData* data);
StatusCode error_estimation_compute_local_errors(ErrorEstimationData* data,
                                               const HybridInterface* interface);
StatusCode error_estimation_compute_global_errors(ErrorEstimationData* data,
                                                const HybridSolver* solver);
StatusCode error_estimation_mark_for_refinement(ErrorEstimationData* data,
                                              int** elements_to_refine,
                                              int* num_elements);
StatusCode error_estimation_adapt_coupling(ErrorEstimationData* data,
                                         HybridSolver* solver);

#ifdef __cplusplus
}
#endif

#endif // HYBRID_COUPLING_INTERFACE_H