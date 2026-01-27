/*********************************************************************
 * PEEC-MoM Unified Framework - MoM Solver Module
 * 
 * This module implements the Method of Moments solver with
 * advanced features including H-matrix compression, MLFMM,
 * and GPU acceleration.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#ifndef MOM_SOLVER_MODULE_H
#define MOM_SOLVER_MODULE_H

#include "../../operators/kernels/electromagnetic_kernel_library.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * MoM Configuration and Options
 *********************************************************************/
typedef enum {
    MOM_BASIS_RWG,              // Rao-Wilton-Glisson basis
    MOM_BASIS_ROOFTOP,          // Rooftop basis functions
    MOM_BASIS_LOOP_TREE,        // Loop-tree decomposition
    MOM_BASIS_HIGHER_ORDER,     // Higher-order basis
    MOM_BASIS_HIERARCHICAL      // Hierarchical basis
} MomBasisType;

typedef enum {
    MOM_INTEGRATION_STANDARD,   // Standard quadrature
    MOM_INTEGRATION_SINGULAR,   // Singular integration
    MOM_INTEGRATION_ADAPTIVE,    // Adaptive integration
    MOM_INTEGRATION_SEMIANALYTIC // Semi-analytical
} MomIntegrationType;

typedef enum {
    MOM_ACCELERATION_NONE,      // Direct assembly
    MOM_ACCELERATION_H_MATRIX,  // Hierarchical matrices
    MOM_ACCELERATION_MLFMM,     // Multilevel Fast Multipole
    MOM_ACCELERATION_ACA,       // Adaptive Cross Approximation
    MOM_ACCELERATION_GPU        // GPU acceleration
} MomAccelerationType;

typedef enum {
    MOM_PRECONDITIONER_NONE,    // No preconditioning
    MOM_PRECONDITIONER_DIAGONAL, // Diagonal scaling
    MOM_PRECONDITIONER_CALDERON,  // Calderon projector
    MOM_PRECONDITIONER_SPAI,     // Sparse Approximate Inverse
    MOM_PRECONDITIONER_ILU,      // Incomplete LU
    MOM_PRECONDITIONER_BLOCK_JACOBI // Block Jacobi
} MomPreconditionerType;

typedef struct {
    // Basis function settings
    MomBasisType basis_type;
    int basis_order;
    bool use_loop_tree;
    
    // Integration settings
    MomIntegrationType integration_type;
    int integration_order;
    Real integration_tolerance;
    bool use_singularity_subtraction;
    
    // Acceleration settings
    MomAccelerationType acceleration_type;
    Real compression_tolerance;
    int h_matrix_max_rank;
    int mlfmm_max_level;
    int aca_max_rank;
    
    // Preconditioner settings
    MomPreconditionerType preconditioner_type;
    int preconditioner_levels;
    Real preconditioner_tolerance;
    
    // Solver settings
    int max_iterations;
    Real convergence_tolerance;
    int restart_size;           // For GMRES
    bool use_bicgstab;
    bool use_parallel_solver;
    
    // Frequency settings
    Real frequency;             // Hz
    Real frequency_start;
    Real frequency_stop;
    int num_frequency_points;
    bool use_adaptive_frequency;
    
    // Accuracy settings
    Real accuracy_target;
    bool use_adaptive_accuracy;
    int adaptive_refinement_levels;
    
    // Performance settings
    int num_threads;
    bool use_gpu;
    bool use_mpi;
    size_t memory_limit;
    bool enable_performance_monitoring;
    
    // Output settings
    bool save_matrix_data;
    bool save_basis_functions;
    bool compute_far_field;
    bool compute_near_field;
    bool compute_surface_currents;
} MomSolverOptions;

/*********************************************************************
 * MoM Data Structures
 *********************************************************************/
typedef struct {
    int triangle_id;
    int edge_ids[3];          // Triangle edges
    int node_ids[3];          // Triangle vertices
    int basis_function_ids[3]; // Associated basis functions
    Real area;
    Real centroid[3];
    Real normal[3];
    int material_id;
    int physical_group;
    bool is_valid;
} MomTriangle;

typedef struct {
    int basis_id;
    MomBasisType type;
    int triangle_id_plus;     // Positive triangle
    int triangle_id_minus;      // Negative triangle
    int edge_id;
    Real length;
    Real area_plus;
    Real area_minus;
    Real centroid_plus[3];
    Real centroid_minus[3];
    Real direction[3];
    bool is_boundary;
    bool is_internal;
} MomBasisFunction;

typedef struct {
    int observation_triangle;
    int source_triangle;
    Real distance;
    Real interaction_strength;
    bool is_near_field;
    bool is_self_term;
} MomInteraction;

typedef struct {
    // Geometry data
    MomTriangle* triangles;
    int num_triangles;
    
    // Basis functions
    MomBasisFunction* basis_functions;
    int num_basis_functions;
    
    // Material properties
    Complex* epsilon_r;         // Per triangle
    Complex* mu_r;              // Per triangle
    Complex* sigma;             // Per triangle
    
    // Frequency-dependent data
    Real frequency;
    Complex wavenumber;
    Complex impedance;
    
    // Mesh connectivity
    int** triangle_neighbors;
    int** basis_function_neighbors;
    
    // Spatial indexing
    void* spatial_index;        // For acceleration
    Real bounding_box[6];     // Min/max coordinates
} MomGeometry;

/*********************************************************************
 * MoM Matrix Structures
 *********************************************************************/
typedef struct {
    // System matrix (impedance matrix)
    Complex* Z_matrix;          // Dense or compressed format
    int matrix_size;
    
    // Right-hand side vector
    Complex* RHS_vector;
    int rhs_size;
    
    // Solution vector (current coefficients)
    Complex* solution_vector;
    int solution_size;
    
    // Matrix format information
    MomAccelerationType matrix_format;
    bool is_compressed;
    size_t memory_usage;
    
    // Compression data (for H-matrix/ACA)
    void* compression_data;
    int compression_rank;
    Real compression_error;
} MomMatrixSystem;

/*********************************************************************
 * MoM Solver Results
 *********************************************************************/
typedef struct {
    // Convergence information
    int num_iterations;
    Real final_residual;
    bool converged;
    Real convergence_time;
    
    // Solution quality
    Real solution_error;
    Real condition_number;
    Real matrix_rank;
    
    // Performance metrics
    double assembly_time;
    double solve_time;
    double total_time;
    size_t memory_usage;
    
    // Accuracy metrics
    Real far_field_error;
    Real near_field_error;
    Real power_conservation_error;
    
    // Frequency sweep results
    Complex* frequency_response;
    int num_frequency_points;
} MomSolverResults;

/*********************************************************************
 * MoM Excitation and Sources
 *********************************************************************/
typedef enum {
    MOM_EXCITATION_PLANE_WAVE,
    MOM_EXCITATION_POINT_SOURCE,
    MOM_EXCITATION_WAVEGUIDE_MODE,
    MOM_EXCITATION_VOLTAGE_SOURCE,
    MOM_EXCITATION_CURRENT_SOURCE,
    MOM_EXCITATION_PORT,
    MOM_EXCITATION_INCIDENT_FIELD
} MomExcitationType;

typedef struct {
    MomExcitationType type;
    Real frequency;             // Hz
    Complex amplitude;
    
    // Plane wave parameters
    Real direction[3];        // Propagation direction
    Real polarization[3];     // Electric field polarization
    
    // Point source parameters
    Real position[3];
    Real source_strength;
    
    // Port parameters
    int port_id;
    Real port_impedance;
    Real* port_modes;         // Modal expansion coefficients
    int num_modes;
    
    // Incident field parameters
    Complex* incident_field;  // Per triangle
    int num_triangles;
} MomExcitation;

/*********************************************************************
 * MoM Far-Field and Radar Cross Section
 *********************************************************************/
typedef struct {
    Real theta;                 // Elevation angle (radians)
    Real phi;                   // Azimuth angle (radians)
    Real r;                     // Distance (meters)
    Complex e_theta;            // Theta component of E-field
    Complex e_phi;               // Phi component of E-field
    Complex h_theta;             // Theta component of H-field
    Complex h_phi;               // Phi component of H-field
} MomFarFieldPoint;

typedef struct {
    MomFarFieldPoint* points;
    int num_points;
    Real frequency;
    Real total_power;
    Real directivity;
    Real gain;
    Real efficiency;
} MomFarFieldPattern;

typedef struct {
    Real rcs_total;             // Total RCS (m²)
    Real rcs_theta;             // Theta polarization (m²)
    Real rcs_phi;               // Phi polarization (m²)
    Real rcs_hh;                // Horizontal-Horizontal (m²)
    Real rcs_vv;                // Vertical-Vertical (m²)
    Real rcs_hv;                // Horizontal-Vertical (m²)
    Real rcs_vh;                // Vertical-Horizontal (m²)
    Real rcs_db;                // RCS in dBsm
    Real frequency;
    Real incident_angle[2];     // Theta, Phi
} MomRCSData;

/*********************************************************************
 * Main MoM Solver Structure
 *********************************************************************/
typedef struct MomSolver MomSolver;

struct MomSolver {
    // Configuration
    MomSolverOptions options;
    
    // Geometry and mesh
    MomGeometry* geometry;
    MeshData* mesh_data;
    
    // Matrix system
    MomMatrixSystem* matrix_system;
    
    // Excitation
    MomExcitation* excitation;
    int num_excitations;
    
    // Solution
    Complex* surface_currents;
    int num_current_coefficients;
    
    // Results
    MomSolverResults* results;
    MomFarFieldPattern* far_field;
    MomRCSData* rcs_data;
    
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
 * MoM Solver API Functions
 *********************************************************************/

// Lifecycle functions
MomSolver* mom_solver_create(const MomSolverOptions* options);
void mom_solver_destroy(MomSolver* solver);

StatusCode mom_solver_initialize(MomSolver* solver, GeometryEngine* geometry, 
                               MeshEngine* mesh_engine);
StatusCode mom_solver_finalize(MomSolver* solver);

// Geometry and mesh setup
StatusCode mom_solver_set_geometry(MomSolver* solver, MomGeometry* geometry);
StatusCode mom_solver_set_mesh(MomSolver* solver, MeshData* mesh_data);
StatusCode mom_solver_set_materials(MomSolver* solver, MaterialDatabase* materials);

// Excitation setup
StatusCode mom_solver_add_excitation(MomSolver* solver, const MomExcitation* excitation);
StatusCode mom_solver_clear_excitations(MomSolver* solver);
StatusCode mom_solver_set_frequency(MomSolver* solver, Real frequency);

// Matrix assembly
StatusCode mom_solver_assemble_matrix(MomSolver* solver);
StatusCode mom_solver_assemble_rhs(MomSolver* solver);
StatusCode mom_solver_apply_preconditioner(MomSolver* solver);

// Solution
StatusCode mom_solver_solve(MomSolver* solver);
StatusCode mom_solver_solve_frequency_sweep(MomSolver* solver, 
                                          Real start_freq, Real stop_freq, 
                                          int num_points);

// Post-processing
StatusCode mom_solver_compute_far_field(MomSolver* solver, Real theta_start, 
                                      Real theta_stop, int num_theta,
                                      Real phi_start, Real phi_stop, int num_phi);
StatusCode mom_solver_compute_rcs(MomSolver* solver, Real theta, Real phi);
StatusCode mom_solver_compute_surface_currents(MomSolver* solver);

// Results access
const MomSolverResults* mom_solver_get_results(const MomSolver* solver);
const Complex* mom_solver_get_surface_currents(const MomSolver* solver);
const MomFarFieldPattern* mom_solver_get_far_field(const MomSolver* solver);
const MomRCSData* mom_solver_get_rcs(const MomSolver* solver);

// Utility functions
StatusCode mom_solver_set_options(MomSolver* solver, const MomSolverOptions* options);
StatusCode mom_solver_get_options(const MomSolver* solver, MomSolverOptions* options);
StatusCode mom_solver_print_info(const MomSolver* solver);
StatusCode mom_solver_print_statistics(const MomSolver* solver);

// Advanced functions
StatusCode mom_solver_adapt_mesh(MomSolver* solver, Real error_threshold);
StatusCode mom_solver_refine_solution(MomSolver* solver, Real accuracy_target);
StatusCode mom_solver_validate_solution(MomSolver* solver);

// Memory and performance
size_t mom_solver_get_memory_usage(const MomSolver* solver);
size_t mom_solver_get_peak_memory_usage(const MomSolver* solver);
StatusCode mom_solver_reset_performance_counters(MomSolver* solver);

/*********************************************************************
 * H-Matrix Implementation
 *********************************************************************/
typedef struct {
    int block_size;
    int max_rank;
    Real tolerance;
    bool use_full_rank_for_near_field;
    bool use_aca_for_far_field;
    int cluster_size;
    Real admissibility_parameter;
} HMatrixOptions;

typedef struct HMatrixBlock {
    bool is_low_rank;
    bool is_dense;
    bool is_zero;
    
    union {
        struct {
            Complex* U;         // Left singular vectors
            Complex* V;         // Right singular vectors
            int rank;
            int rows;
            int cols;
        } low_rank;
        
        struct {
            Complex* data;      // Dense matrix data
            int rows;
            int cols;
            int ld;             // Leading dimension
        } dense;
    } data;
    
    struct HMatrixBlock** children; // 4 children for 2D case
    int num_children;
} HMatrixBlock;

typedef struct {
    HMatrixBlock* root;
    int total_size;
    int num_blocks;
    int max_rank;
    Real compression_ratio;
    size_t memory_usage;
    Real compression_error;
} HMatrix;

// H-Matrix functions
HMatrix* h_matrix_create(int size, const HMatrixOptions* options);
void h_matrix_destroy(HMatrix* h_matrix);
StatusCode h_matrix_assemble(HMatrix* h_matrix, const Complex* dense_matrix);
StatusCode h_matrix_multiply_vector(const HMatrix* h_matrix, const Complex* x, Complex* y);
StatusCode h_matrix_compress(HMatrix* h_matrix, Real tolerance);
Real h_matrix_get_compression_ratio(const HMatrix* h_matrix);

/*********************************************************************
 * MLFMM Implementation
 *********************************************************************/
typedef struct {
    int max_level;
    int min_box_size;
    int expansion_order;
    Real tolerance;
    bool use_gpu;
    int num_threads;
    int cluster_size;
} MLFMMOptions;

typedef struct {
    Complex* multipole_expansions;
    Complex* local_expansions;
    int num_boxes;
    int max_level;
    int expansion_order;
    size_t memory_usage;
    Real accuracy;
} MLFMMData;

// MLFMM functions
MLFMMData* mlfmm_create(const MLFMMOptions* options);
void mlfmm_destroy(MLFMMData* mlfmm);
StatusCode mlfmm_setup_tree(MLFMMData* mlfmm, const MomGeometry* geometry);
StatusCode mlfmm_compute_interactions(MLFMMData* mlfmm, const Complex* sources, Complex* potentials);
Real mlfmm_get_accuracy(const MLFMMData* mlfmm);

/*********************************************************************
 * GPU Acceleration Interface
 *********************************************************************/
typedef struct {
    bool use_cuda;
    bool use_opencl;
    int device_id;
    size_t memory_limit;
    bool enable_profiling;
    int num_streams;
} GPUOptions;

typedef struct {
    void* device_matrix;
    void* device_vector;
    void* device_result;
    size_t memory_allocated;
    double computation_time;
    bool is_initialized;
} GPUData;

// GPU functions
GPUData* gpu_data_create(const GPUOptions* options);
void gpu_data_destroy(GPUData* gpu_data);
StatusCode gpu_matrix_vector_multiply(GPUData* gpu_data, const Complex* matrix, 
                                    const Complex* vector, Complex* result, int size);
StatusCode gpu_transfer_to_device(GPUData* gpu_data, const void* host_data, size_t size);
StatusCode gpu_transfer_from_device(GPUData* gpu_data, void* host_data, size_t size);

#ifdef __cplusplus
}
#endif

#endif // MOM_SOLVER_MODULE_H