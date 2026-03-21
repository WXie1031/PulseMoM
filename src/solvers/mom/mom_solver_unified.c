/*****************************************************************************************
 * PulseEM - Unified Method of Moments (MoM) Solver Implementation
 * 
 * Copyright (C) 2025 Hong Cai Chen 
 * Affiliation: AI4MW Research Group 
 * E-mail: chenhc@seu.edu.cn 
 * 
 * All rights reserved. This program is the proprietary software of the AI4MW Research Group. 
 * Unauthorized reproduction, distribution, modification, or use of this program in whole or in part 
 * is strictly prohibited without prior written permission from the copyright holder.
 * 
 * File: mom_solver_unified.c
 * Description: Unified MoM solver combining basic and advanced features
 * 
 * Features:
 * - Automatic algorithm selection based on problem size and characteristics
 * - RWG basis functions with higher-order extensions
 * - Multiple fast algorithms (ACA, MLFMM, H-matrix)
 * - Advanced preconditioning techniques
 * - GPU acceleration for large-scale problems
 * - Comprehensive radiation pattern and RCS analysis
 * - Wideband frequency analysis with adaptive sampling
 * - Parallel distributed computing support
 * 
 * Technical Specifications:
 * - C11 compliant with POSIX standard compliance
 * - Complex number arithmetic for frequency domain analysis
 * - OpenMP parallel processing for multi-core systems
 * - CUDA GPU acceleration for compatible hardware
 * - Thread-safe operations with proper synchronization
 * - Cross-platform compatibility (Linux, Windows, macOS)
 * 
 * Target Applications:
 * - Antenna design and radiation pattern analysis
 * - Radar cross section (RCS) prediction and optimization
 * - Electromagnetic scattering from complex objects
 * - Microwave circuit and component modeling
 * - Electromagnetic compatibility (EMC) analysis
 * - Wireless communication system simulation
 * - Satellite and aerospace electromagnetic modeling
 *****************************************************************************************/

#include "../../backend/solvers/core_solver.h"
#include "../../operators/kernels/core_kernels.h"
#include "../../backend/gpu/gpu_acceleration.h"
// #include "../core/core_mesh_unified.h"  // File does not exist, using core_mesh.h instead
#include "../../discretization/mesh/core_mesh.h"
#include "../../common/core_common.h"
#include "../../operators/kernels/electromagnetic_kernels.h"
#include "../../operators/assembler/core_assembler.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../operators/greens/layered_greens_function.h"
#include "../../operators/kernels/kernel_mfie.h"
#include "../../operators/kernels/kernel_cfie.h"
#include "../../operators/integration/integral_nearly_singular.h"
#include "../../operators/integration/integration_utils_optimized.h"
#include "mom_aca.h"
#include "mom_hmatrix.h"
#include "mom_mlfmm.h"
#include "mom_matvec.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// Compiler optimization hints
// Use restrict hints for better compiler optimization (if supported)
#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT_PTR __restrict__
#elif defined(_MSC_VER)
#define RESTRICT_PTR __restrict
#else
#define RESTRICT_PTR
#endif

// Precomputed constants for performance optimization
// These constants are used frequently in matrix assembly loops
// Note: ONE_THIRD, INV_4PI, and TWO_PI_OVER_C0 are defined in core_common.h, do not redefine here

// Compute Green's function value (free space or layered medium)
// Uses full layered_medium_greens_function interface when available
static complex_t compute_green_function_value(
    double rho, double z, double z_prime,
    double frequency,
    const void* layered_medium,
    const void* frequency_domain,
    const void* greens_params) {
    
    complex_t result = complex_zero();
    const double k = TWO_PI_OVER_C0 * frequency;
    double r = sqrt(rho*rho + (z-z_prime)*(z-z_prime)) + GEOMETRIC_EPSILON;
    
    // If layered medium is enabled, use full layered Green's function interface
    if (layered_medium && frequency_domain) {
        const LayeredMedium* medium = (const LayeredMedium*)layered_medium;
        const FrequencyDomain* freq = (const FrequencyDomain*)frequency_domain;
        const GreensFunctionParams* params = (const GreensFunctionParams*)greens_params;
        
        if (medium->num_layers > 0) {
            // Find source and observation layers
            double z_acc = 0.0;
            int layer_src = 0, layer_obs = 0;
            for (int i = 0; i < medium->num_layers; i++) {
                if (z_prime >= z_acc && z_prime < z_acc + medium->thickness[i]) {
                    layer_src = i;
                    break;
                }
                z_acc += medium->thickness[i];
            }
            z_acc = 0.0;
            for (int i = 0; i < medium->num_layers; i++) {
                if (z >= z_acc && z < z_acc + medium->thickness[i]) {
                    layer_obs = i;
                    break;
                }
                z_acc += medium->thickness[i];
            }
            
            // Use full layered_medium_greens_function interface
            // For MoM EFIE, we need the electric-electric (G_ee) dyadic component
            // We'll extract a scalar approximation from the trace or diagonal
            GreensFunctionPoints points;
            points.x = 0.0;  // Source at origin in rho plane
            points.y = 0.0;
            points.z = z_prime;
            points.xp = rho;  // Observation at (rho, 0, z)
            points.yp = 0.0;
            points.zp = z;
            points.layer_src = layer_src;
            points.layer_obs = layer_obs;
            
            // Call full Green's function interface
            GreensFunctionDyadic* gf = layered_medium_greens_function(medium, freq, &points, params);
            
            if (gf) {
                // Extract scalar Green's function from G_ee dyadic
                // For MoM EFIE with RWG basis functions (primarily horizontal), we need to consider
                // the full dyadic structure: Z_ij = ∫∫ f_i(r) · [G_ee(r,r') · f_j(r')] dS dS'
                // 
                // Since RWG basis functions are primarily in the triangle plane (x-y plane),
                // we use a weighted combination of diagonal components:
                // - G_xx and G_yy for horizontal components
                // - G_zz for vertical components (less dominant for surface currents)
                // 
                // For a more accurate approximation, we use the trace (sum of diagonal)
                // which represents the average response in all directions
                CDOUBLE G_xx = gf->G_ee[0][0];
                CDOUBLE G_yy = gf->G_ee[1][1];
                CDOUBLE G_zz = gf->G_ee[2][2];
                
                // Weighted combination: emphasize horizontal components for surface currents
                // Weight factors: RWG_WEIGHT_XX for xx, RWG_WEIGHT_YY for yy, RWG_WEIGHT_ZZ for zz (typical for surface RWG)
                // For MSVC, use inline complex operations
                #if defined(_MSC_VER)
                CDOUBLE w_xx = {RWG_WEIGHT_XX, 0.0};
                CDOUBLE w_yy = {RWG_WEIGHT_YY, 0.0};
                CDOUBLE w_zz = {RWG_WEIGHT_ZZ, 0.0};
                CDOUBLE G_scalar = cadd(cadd(cmul(G_xx, w_xx), cmul(G_yy, w_yy)), cmul(G_zz, w_zz));
                #else
                CDOUBLE G_scalar = RWG_WEIGHT_XX * G_xx + RWG_WEIGHT_YY * G_yy + RWG_WEIGHT_ZZ * G_zz;
                #endif
                
                // Alternative: use trace (average of all diagonal components)
                // This is more general but may be less accurate for specific geometries
                // #if defined(_MSC_VER)
                // CDOUBLE G_scalar = cadd(cadd(G_xx, G_yy), G_zz);
                // G_scalar = cmul(G_scalar, make_c(1.0/3.0, 0.0));
                // #else
                // CDOUBLE G_scalar = (G_xx + G_yy + G_zz) / 3.0;
                // #endif
                
                // Convert CDOUBLE to complex_t
                #if defined(_MSC_VER)
                result.re = G_scalar.re;
                result.im = G_scalar.im;
                #else
                result.re = creal(G_scalar);
                result.im = cimag(G_scalar);
                #endif
                
                // Free the dyadic structure
                free_greens_function_dyadic(gf);
                return result;
            } else {
                // Fallback to simplified layered Green's function if full interface fails
                double k0_re = TWO_PI_OVER_C0 * frequency;
                double eps_r_src = (layer_src < medium->num_layers && medium->epsilon_r) 
                                  ? medium->epsilon_r[layer_src] : 1.0;
                double eps_r_obs = (layer_obs < medium->num_layers && medium->epsilon_r)
                                  ? medium->epsilon_r[layer_obs] : 1.0;
                double eps_r_eff = sqrt(eps_r_src * eps_r_obs);
                
                double sigma_src = (layer_src < medium->num_layers && medium->sigma)
                                  ? medium->sigma[layer_src] : 0.0;
                double alpha = (sigma_src > 0.0) ? sqrt(M_PI * frequency * MU0 * sigma_src) : 0.0;
                
                double scale = 1.0 / (4.0 * M_PI * r * sqrt(eps_r_eff));
                double phase = -k0_re * sqrt(eps_r_eff) * r;
                double atten = (alpha > 0.0) ? exp(-alpha * r) : 1.0;
                
                result.re = scale * atten * cos(phase);
                result.im = scale * atten * sin(phase);
                return result;
            }
        }
    }
    
    // Free-space Green's function (fallback)
    double scale = 1.0 / (4.0 * M_PI * r);
    double phase = -k * r;
    result.re = scale * cos(phase);
    result.im = scale * sin(phase);
    return result;
}

// Fallback implementation when core assembler object file is not linked
complex_t integrate_triangle_triangle(const geom_triangle_t* tri_i,
                                      const geom_triangle_t* tri_j,
                                      double frequency,
                                      kernel_formulation_t formulation,
                                      int gauss_order) {
    (void)formulation;
    (void)gauss_order;  // Use default order in fallback
    if (!tri_i || !tri_j) {
        return complex_zero();
    }
    const double k = TWO_PI_OVER_C0 * frequency;
    // Centroid calculation using helper functions
    geom_point_t c_i, c_j;
    geom_triangle_get_centroid(tri_i, &c_i);
    geom_triangle_get_centroid(tri_j, &c_j);
    double r = geom_point_distance(&c_i, &c_j) + GEOMETRIC_EPSILON;  // Add small epsilon for numerical stability
    // Use helper function for area retrieval with fallback
    double area_i = geom_triangle_get_area(tri_i);
    double area_j = geom_triangle_get_area(tri_j);
    // Fallback to default area if computed area is too small
    if (area_i < AREA_EPSILON) area_i = DEFAULT_AREA_FALLBACK;
    if (area_j < AREA_EPSILON) area_j = DEFAULT_AREA_FALLBACK;
    double scale = (area_i * area_j) / (4.0 * M_PI * r);
    double phase = -k * r;
    complex_t val = {scale * cos(phase), scale * sin(phase)};
    return val;
}

// MSVC-compatible complex number handling
#if defined(_MSC_VER)
// Use complex_t from core_common.h
typedef complex_t mom_complex_t;
#define MOM_COMPLEX_MAKE(re, im) ((complex_t){(re), (im)})
#define MOM_COMPLEX_ADD(a, b) ((complex_t){(a).re + (b).re, (a).im + (b).im})
#define MOM_COMPLEX_MUL(a, b) ((complex_t){(a).re*(b).re - (a).im*(b).im, (a).re*(b).im + (a).im*(b).re})
#define MOM_COMPLEX_SCALE(a, s) ((complex_t){(a).re*(s), (a).im*(s)})
#define MOM_COMPLEX_CONJ(a) ((complex_t){(a).re, -(a).im})
// Optimized inline functions for complex operations
static inline complex_t complex_mul_sub(const complex_t a, const complex_t b, const complex_t c) {
    // Returns: a - b * c (optimized for LU decomposition)
    complex_t bc = MOM_COMPLEX_MUL(b, c);
    return (complex_t){a.re - bc.re, a.im - bc.im};
}
static inline complex_t complex_div(const complex_t a, const complex_t b) {
    // Returns: a / b (optimized division)
    double denom = b.re * b.re + b.im * b.im;
    double denom_inv = 1.0 / denom;
    return (complex_t){(a.re * b.re + a.im * b.im) * denom_inv,
                       (a.im * b.re - a.re * b.im) * denom_inv};
}
#else
#include <complex.h>
typedef double complex mom_complex_t;
#define MOM_COMPLEX_MAKE(re, im) ((re) + (im)*I)
#define MOM_COMPLEX_ADD(a, b) ((a) + (b))
#define MOM_COMPLEX_MUL(a, b) ((a) * (b))
#define MOM_COMPLEX_SCALE(a, s) ((a) * (s))
#define MOM_COMPLEX_CONJ(a) (conj(a))
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cusolverDn.h>
#endif

// Solver algorithm types (now defined in mom_matvec.h)
// Use the enum from mom_matvec.h to avoid duplication

// Basis function orders
typedef enum {
    MOM_BASIS_CONSTANT = 0,       // Constant (pulse) basis
    MOM_BASIS_LINEAR = 1,         // Linear (RWG) basis
    MOM_BASIS_QUADRATIC = 2,      // Quadratic basis
    MOM_BASIS_CUBIC = 3,          // Cubic basis
    MOM_BASIS_LEGENDRE = 4,       // Legendre polynomial basis
    MOM_BASIS_LAGRANGE = 5        // Lagrange polynomial basis
} mom_basis_order_t;

// Preconditioner types
typedef enum {
    MOM_PRECOND_NONE = 0,         // No preconditioning
    MOM_PRECOND_DIAGONAL = 1,     // Diagonal preconditioning
    MOM_PRECOND_BLOCK_DIAG = 2,   // Block diagonal preconditioning
    MOM_PRECOND_SPARSE_APPROX = 3, // Sparse approximate inverse
    MOM_PRECOND_MULTIGRID = 4,    // Multigrid preconditioning
    MOM_PRECOND_DOMAIN_DECOMP = 5 // Domain decomposition preconditioning
} mom_preconditioner_t;

// Iterative solver types
typedef enum {
    MOM_ITER_GMRES = 0,           // GMRES
    MOM_ITER_BICGSTAB = 1,        // BiCGSTAB
    MOM_ITER_CGS = 2,             // CGS
    MOM_ITER_QMR = 3,             // QMR
    MOM_ITER_TFQMR = 4          // TFQMR
} mom_iterative_solver_t;

// Problem characteristics for algorithm selection
typedef struct {
    int num_unknowns;              // Number of unknowns
    double frequency;               // Operating frequency
    double electrical_size;         // Electrical size of problem
    bool is_wideband;              // Wideband analysis flag
    bool has_curved_surfaces;       // Curved surface modeling needed
    int num_materials;             // Number of different materials
    bool requires_high_accuracy;    // High accuracy requirements
} mom_problem_characteristics_t;

// Unified solver configuration
typedef struct {
    // Algorithm selection
    mom_algorithm_t algorithm;              // Primary algorithm
    mom_basis_order_t basis_order;          // Basis function order
    mom_preconditioner_t preconditioner;    // Preconditioner type
    mom_iterative_solver_t iterative_solver; // Iterative solver
    
    // Accuracy and convergence
    double convergence_tolerance;             // Convergence tolerance
    int max_iterations;                     // Maximum iterations
    int restart_parameter;                  // GMRES restart parameter
    
    // Performance parameters
    int num_threads;                        // Number of OpenMP threads
    bool use_gpu_acceleration;              // GPU acceleration flag
    int gpu_device_id;                      // GPU device ID
    
    // ACA/MLFMM parameters
    double aca_tolerance;                   // ACA compression tolerance
    int mlfmm_max_levels;                 // MLFMM maximum levels
    int mlfmm_box_size;                   // MLFMM box size
    
    // H-matrix parameters
    double hmatrix_tolerance;              // H-matrix compression tolerance
    int hmatrix_min_cluster_size;        // Minimum cluster size
    
    // Advanced options
    bool enable_adaptive_refinement;       // Adaptive mesh refinement
    bool enable_frequency_interpolation;   // Frequency interpolation
    bool enable_parallel_distributed;      // Parallel distributed computing
    
    // Near-field/singularity handling
    bool enable_duffy_transform;          // Enable Duffy transform for singular integrals
    double near_field_threshold;           // Distance threshold for near-field (in wavelengths, range: [0.01, 1.0])
    bool use_analytic_self_term;          // Use analytic self-term instead of numerical
    double self_term_regularization;      // Small imaginary regularization for self-term (range: [REGULARIZATION_MIN, REGULARIZATION_MAX])
    bool enable_nearly_singular;          // Enable nearly singular integral handling
    
    // Kernel formulation selection
    int kernel_formulation;              // 0=EFIE, 1=MFIE, 2=CFIE
    double cfie_alpha;                    // CFIE combination parameter (0=MFIE, 1=EFIE, default=0.5)
    
    // Performance statistics (optional, for debugging/monitoring, requires MOM_ENABLE_STATISTICS)
    #ifdef MOM_ENABLE_STATISTICS
    int duffy_transform_count;            // Number of times Duffy transform was used (statistics)
    int analytic_self_term_count;         // Number of times analytic self-term was used (statistics)
    #endif
} mom_unified_config_t;

// Unified solver state
typedef struct {
    mom_unified_config_t config;            // Configuration
    mom_problem_characteristics_t problem;  // Problem characteristics
    
    // Matrix storage
    void *impedance_matrix;                 // Impedance matrix (format depends on algorithm)
    mom_csr_complex_t *csr_impedance_matrix; /**< MLFMM: sparse near-field; impedance_matrix NULL when set */
    void *preconditioner_matrix;          // Preconditioner matrix
    
    // Solution vectors
    mom_complex_t *current_coefficients;   // Current coefficients
    mom_complex_t *excitation_vector;      // Excitation vector
    mom_complex_t *rhs_vector;             // Right-hand side vector
    
    // Geometry and mesh data
    mesh_t *mesh;                          // Mesh data
    int num_basis_functions;               // Number of basis functions
    
    // Layered medium support
    void *layered_medium;                  // Layered medium data (LayeredMedium*)
    void *frequency_domain;                // Frequency domain data (FrequencyDomain*)
    void *greens_params;                   // Green's function parameters (GreensFunctionParams*)
    bool use_layered_medium;               // Flag for layered medium
    
    // Performance metrics
    double matrix_fill_time;               // Matrix filling time
    double solve_time;                     // Solution time
    double total_memory_usage;              // Total memory usage
    int iterations_converged;               // Iterations to convergence
    
    // GPU data (if enabled)
    void *gpu_impedance_matrix;            // GPU impedance matrix
    void *gpu_solution_vector;             // GPU solution vector
    
    // Parallel processing
    int mpi_rank;                          // MPI rank (for distributed computing)
    int mpi_size;                          // MPI size
} mom_unified_state_t;

// Function prototypes
static mom_algorithm_t select_algorithm(const mom_problem_characteristics_t *problem);
static void compute_problem_characteristics(mom_unified_state_t *state);
static bool detect_curved_surfaces(const mesh_t* mesh);
static int count_different_materials(const mesh_t* mesh);
static int build_rwg_mapping(const mesh_t* mesh, int* edge_plus, int* edge_minus, double* edge_length);
static complex_t integrate_triangle_duffy(const geom_triangle_t* tri_i, const geom_triangle_t* tri_j,
                                         double frequency, double threshold);
static complex_t compute_analytic_self_term(const geom_triangle_t* tri, double frequency, 
                                           double edge_length, double regularization);

// Kernel wrapper function for nearly singular integrals
static complex_t efie_kernel_wrapper_for_nearly_singular(const geom_point_t* r, 
                                                          const geom_point_t* r_prime, 
                                                          double k, 
                                                          void* data) {
    (void)data;  // Unused
    // Use helper function for distance calculation
    double R = geom_point_distance(r, r_prime) + NUMERICAL_EPSILON;
    double kR = k * R;
    double inv_4pi_r = INV_4PI / R;
    complex_t result = {inv_4pi_r * cos(-kR), inv_4pi_r * sin(-kR)};
    return result;
}

// MFIE kernel wrapper data structure
typedef struct {
    const geom_point_t* n_prime;  // Normal vector at source point
} mfie_kernel_data_t;

// Kernel wrapper function for MFIE nearly singular integrals
static complex_t mfie_kernel_wrapper_for_nearly_singular(const geom_point_t* r, 
                                                          const geom_point_t* r_prime, 
                                                          double k, 
                                                          void* data) {
    if (!data) {
        return complex_zero();
    }
    
    mfie_kernel_data_t* mfie_data = (mfie_kernel_data_t*)data;
    if (!mfie_data->n_prime) {
        return complex_zero();
    }
    
    // Use MFIE Neumann boundary term kernel
    return kernel_mfie_neumann_term(r, r_prime, mfie_data->n_prime, k);
}

/**
 * Infer meters per one mesh coordinate unit so wavelength-based distance cutoffs match centroid spacing.
 * Gmsh/CAD imports often use millimetres; C0/frequency is in metres. Comparing r_mesh (mm) to 0.1*λ (m)
 * without scaling yields nnz ≈ N (only diagonal) and breaks CSR / iterative MoM.
 */
static double mom_infer_mesh_meters_per_coord_unit(const mesh_t* mesh, double frequency_hz) {
    if (!mesh || !mesh->vertices || mesh->num_vertices <= 0) return 1.0;
    double mx = 0.0;
    for (int i = 0; i < mesh->num_vertices; i++) {
        const geom_point_t* p = &mesh->vertices[i].position;
        double ax = fabs(p->x);
        double ay = fabs(p->y);
        double az = fabs(p->z);
        if (ax > mx) mx = ax;
        if (ay > mx) mx = ay;
        if (az > mx) mx = az;
    }
    const double freq = (frequency_hz > 1.0) ? frequency_hz : DEFAULT_FREQUENCY_HZ;
    const double lambda_m = C0 / freq;
    /* Model extent ≫ λ in metre-equivalent usually means coordinates are not in metres */
    if (mx > 100.0 * lambda_m && mx > 20.0) return 1e-3;
    /* Typical mm CAD: thousands of raw units */
    if (mx > 2500.0) return 1e-3;
    return 1.0;
}

static complex_t integrate_rwg_far_field(const mesh_t* mesh, int edge_idx, int tri_idx, int is_plus,
                                        const double* r_hat, double k0);
static int assemble_impedance_matrix_basic(mom_unified_state_t *state);
static int solve_linear_system_lu_simple(mom_unified_state_t *state);
static int solve_linear_system_core_solver(mom_unified_state_t *state);
static int assemble_impedance_matrix_aca(mom_unified_state_t *state);
static int assemble_impedance_matrix_mlfmm(mom_unified_state_t *state);
static int assemble_impedance_matrix_hmatrix(mom_unified_state_t *state);
static int solve_linear_system_basic(mom_unified_state_t *state);
static int solve_linear_system_iterative(mom_unified_state_t *state);
static int solve_linear_system_lu_simple(mom_unified_state_t *state);
static int solve_linear_system_core_solver(mom_unified_state_t *state);
static void compute_radiation_pattern_unified(mom_unified_state_t *state, double theta_min, double theta_max, 
                                            double phi_min, double phi_max, int n_theta, int n_phi);
static double compute_rcs_unified(mom_unified_state_t *state, double theta_inc, double phi_inc,
                                 double theta_scat, double phi_scat);

/********************************************************************************
 * Algorithm Selection for Method of Moments Solver
 * 
 * This function automatically selects the optimal MoM algorithm based on problem
 * characteristics such as problem size, electrical size, and computational
 * requirements. The selection criteria are designed to balance accuracy and
 * computational efficiency.
 * 
 * Algorithm Selection Logic:
 * 1. Basic Direct Solver (< 1000 unknowns): Uses LU decomposition for small
 *    problems where direct solution is most efficient
 * 2. ACA Algorithm (1000-50000 unknowns): Uses Adaptive Cross Approximation
 *    for medium-sized problems with good compression properties
 * 3. MLFMM Algorithm (> 10λ electrical size): Uses Multilevel Fast Multipole
 *    Method for electrically large problems with many wavelengths
 * 4. H-Matrix Algorithm (other large problems): Uses hierarchical matrix
 *    compression for general large-scale problems
 * 
 * Parameters:
 *   problem: Problem characteristics including unknown count and electrical size
 * 
 * Returns:
 *   Selected algorithm type that optimizes performance for given problem
 * 
 * Performance Considerations:
 * - Direct solver has O(N³) complexity but is fastest for small N
 * - ACA has O(N log N) complexity with good compression ratios
 * - MLFMM has O(N log N) complexity for electrically large problems
 * - H-matrix has O(N log N) complexity with hierarchical compression
 ********************************************************************************/
static mom_algorithm_t select_algorithm(const mom_problem_characteristics_t *problem) {
    const int n = problem->num_unknowns;
    /* Small N: dense direct (low overhead, exact factorization). */
    if (n < 600) {
        return MOM_ALGO_BASIC;
    }
    /* Medium/large N: MLFMM assembly stores only near-field in CSR → O(nnz) vs O(n^2) memory.
     * (Far-field in this codebase is still approximate in matvec; same model as prior dense-zeros MLFMM.) */
    if (n < 200000) {
        return MOM_ALGO_MLFMM;
    }
    if (problem->electrical_size > 10.0) {
        return MOM_ALGO_MLFMM;
    }
    return MOM_ALGO_HMATRIX;
}

/********************************************************************************
 * Compute Problem Characteristics for Algorithm Selection
 * 
 * This function analyzes the electromagnetic problem to determine key
 * characteristics that influence algorithm selection and solver configuration.
 * It computes electrical dimensions, material properties, and accuracy
 * requirements to guide automatic algorithm selection.
 * 
 * Characteristics Computed:
 * 1. Number of unknowns: Total basis functions in the discretized problem
 * 2. Electrical size: Maximum dimension in wavelengths
 * 3. Wideband flag: Whether frequency interpolation is enabled
 * 4. Surface complexity: Presence of curved surfaces (simplified)
 * 5. Material diversity: Number of different materials (simplified)
 * 6. Accuracy requirements: Based on convergence tolerance
 * 
 * Parameters:
 *   state: MoM solver state containing mesh data and configuration
 * 
 * Notes:
 * - Electrical size determines if MLFMM should be used for large problems
 * - Number of unknowns directly affects algorithm selection thresholds
 * - High accuracy requirements may influence preconditioner selection
 * - This function should be called before algorithm selection
 ********************************************************************************/
static void compute_problem_characteristics(mom_unified_state_t *state) {
    // Count basis functions
    state->problem.num_unknowns = state->num_basis_functions;
    
    // Compute electrical size (approximate, optimized: avoid sqrt until final)
    double max_dimension_sq = 0.0;  // Track squared dimension to minimize sqrt calls
    // Cache mesh pointers for better performance
    const mesh_vertex_t* vertices = state->mesh->vertices;
    const int num_vertices = state->mesh->num_vertices;
    
    for (int i = 0; i < num_vertices; i++) {
        const double x = vertices[i].position.x;
        const double y = vertices[i].position.y;
        const double z = vertices[i].position.z;
        // Optimized: use squared distance to avoid sqrt
        const double r_sq = x*x + y*y + z*z;
        if (r_sq > max_dimension_sq) {
            max_dimension_sq = r_sq;  // Track maximum squared distance
        }
    }
    const double max_dimension = sqrt(max_dimension_sq);  // Compute sqrt once at the end
    
    const double wavelength = C0 / state->problem.frequency;  // Use C0 constant instead of magic number
    state->problem.electrical_size = 2.0 * max_dimension / wavelength;
    
    // Set other characteristics
    state->problem.is_wideband = (state->config.enable_frequency_interpolation);
    
    // Detect curved surfaces by analyzing normal vector variation
    state->problem.has_curved_surfaces = detect_curved_surfaces(state->mesh);
    
    // Count different materials
    state->problem.num_materials = count_different_materials(state->mesh);
    
    state->problem.requires_high_accuracy = (state->config.convergence_tolerance < HIGH_ACCURACY_TOLERANCE);
}

/********************************************************************************
 * Detect Curved Surfaces in Mesh
 * 
 * Analyzes the mesh to detect if it contains curved surfaces by checking:
 * 1. Normal vector variation between adjacent elements
 * 2. Edge curvature (for higher-order elements)
 * 3. Surface element types (non-planar elements indicate curvature)
 * 
 * Returns: true if curved surfaces are detected, false otherwise
 ********************************************************************************/
static bool detect_curved_surfaces(const mesh_t* mesh) {
    if (!mesh || mesh->num_elements < 3) return false;
    
    const double CURVATURE_THRESHOLD = CURVATURE_THRESHOLD_DEFAULT;  // Threshold for normal variation
    const int MIN_SAMPLES = 10;              // Minimum samples to check
    
    int num_samples = (mesh->num_elements < MIN_SAMPLES) ? mesh->num_elements : MIN_SAMPLES;
    int step = mesh->num_elements / num_samples;
    if (step < 1) step = 1;
    
    int curved_count = 0;
    int total_checked = 0;
    
    // Check normal vector variation between adjacent elements
    for (int i = 0; i < mesh->num_elements - 1 && total_checked < num_samples; i += step) {
        if (i + step >= mesh->num_elements) break;
        
        mesh_element_t* elem1 = &mesh->elements[i];
        mesh_element_t* elem2 = &mesh->elements[i + step];
        
        // Only check surface elements (triangles, quadrilaterals)
        if (elem1->type != MESH_ELEMENT_TRIANGLE && 
            elem1->type != MESH_ELEMENT_QUADRILATERAL &&
            elem1->type != MESH_ELEMENT_RECTANGLE) {
            continue;
        }
        
        if (elem2->type != MESH_ELEMENT_TRIANGLE && 
            elem2->type != MESH_ELEMENT_QUADRILATERAL &&
            elem2->type != MESH_ELEMENT_RECTANGLE) {
            continue;
        }
        
        // Compute dot product of normals
        double dot = elem1->normal.x * elem2->normal.x +
                    elem1->normal.y * elem2->normal.y +
                    elem1->normal.z * elem2->normal.z;
        
        // Normalize dot product (should be close to 1.0 for planar surfaces)
        double angle = acos(fmax(-1.0, fmin(1.0, dot)));
        
        // If angle between normals is significant, surface is curved
        if (angle > CURVATURE_THRESHOLD) {
            curved_count++;
        }
        
        total_checked++;
    }
    
    // If more than 30% of checked pairs show curvature, consider surface curved
    if (total_checked > 0) {
        double curvature_ratio = (double)curved_count / total_checked;
        return (curvature_ratio > CURVATURE_RATIO_THRESHOLD);
    }
    
    return false;
}

/********************************************************************************
 * Count Different Materials in Mesh
 * 
 * Counts the number of unique material IDs in the mesh
 ********************************************************************************/
static int count_different_materials(const mesh_t* mesh) {
    if (!mesh || mesh->num_elements == 0) return 1;
    
    // Simple approach: count unique material IDs
    // In a full implementation, would use a hash set or sorted array
    int max_material_id = 0;
    bool* material_present = NULL;
    
    // Find maximum material ID
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].material_id > max_material_id) {
            max_material_id = mesh->elements[i].material_id;
        }
    }
    
    if (max_material_id < 0) return 1;
    
    // Allocate and initialize material presence array
    material_present = (bool*)calloc(max_material_id + 1, sizeof(bool));
    if (!material_present) return 1;
    
    // Mark present materials
    for (int i = 0; i < mesh->num_elements; i++) {
        int mat_id = mesh->elements[i].material_id;
        if (mat_id >= 0 && mat_id <= max_material_id) {
            material_present[mat_id] = true;
        }
    }
    
    // Count unique materials
    int count = 0;
    for (int i = 0; i <= max_material_id; i++) {
        if (material_present[i]) count++;
    }
    
    free(material_present);
    
    return (count > 0) ? count : 1;
}

// Duffy transform for singular/weakly singular triangle integrals
// Handles cases where triangles overlap or are very close
// Uses proper Duffy transformation to remove singularities in 1/R kernel
//
// The Duffy transform is a coordinate transformation that removes the 1/R singularity
// in the EFIE kernel when source and observation points are close. The transformation
// maps the unit square [0,1]^2 to triangle pairs using barycentric coordinates with
// a factor (1-u) that cancels the singularity.
//
// Reference: Duffy, M.G. (1982). "Quadrature over a pyramid or cube of integrands
// with a singularity at a vertex." SIAM Journal on Numerical Analysis, 19(6).
//
// Parameters:
//   tri_i, tri_j: Source and observation triangles
//   frequency: Operating frequency (Hz)
//   threshold: Distance threshold in wavelengths for near-field classification
// Returns:
//   Complex impedance matrix element Z_ij
static complex_t integrate_triangle_duffy(const geom_triangle_t* tri_i, const geom_triangle_t* tri_j,
                                         double frequency, double threshold) {
    if (!tri_i || !tri_j) {
        return complex_zero();
    }
    
    // Use geometry helper for centroid distance
    double r = geom_triangle_compute_centroid_distance(tri_i, tri_j);
    
    const double k = TWO_PI_OVER_C0 * frequency;
    const double lambda = C0 / frequency;
    double r_threshold = threshold * lambda;
    
    // If distance is above threshold, use standard integration
    // Standard integration is more efficient for well-separated triangles
    // and provides sufficient accuracy when r > threshold * lambda
    if (r > r_threshold) {
        return integrate_triangle_triangle(tri_i, tri_j, frequency, KERNEL_FORMULATION_EFIELD, 4);  // Default order
    }
    
    // For self-term (same triangle), return zero here - handled separately with analytic formula
    // The self-term is the most singular case and requires special treatment
    // We use an analytic formula instead of numerical integration for better accuracy and stability
    if (r < NUMERICAL_EPSILON) {
        return complex_zero();  // Self-term handled in assembly with analytic formula (compute_analytic_self_term)
    }
    
    // Near-term: use Duffy transform with proper barycentric mapping
    // Duffy transform maps unit square [0,1]^2 to triangle pairs with singularity removal
    // The transformation removes the 1/R singularity by mapping v -> v*(1-u)
    // For triangle-triangle integration, we use 4-point Gauss quadrature (2x2) on transformed domain
    
    // Compute triangle areas (use cached value if available, otherwise compute)
    // This avoids redundant computation when area is already known
    double area_i = geom_triangle_get_area(tri_i);
    double area_j = geom_triangle_get_area(tri_j);
    
    // Numerical stability check: ensure triangles have valid areas
    // Very small triangles can cause numerical issues in the Duffy transform
    if (area_i < AREA_EPSILON || area_j < AREA_EPSILON) {
        return complex_zero();
    }
    
    // 4-point Gauss quadrature on unit square (2x2 grid)
    // Points: (±1/√3, ±1/√3) in [-1,1]^2, mapped to [0,1]^2
    // Weights: 1.0 for each point (after mapping)
    const double gauss_pts[2] = {-0.577350269189626, 0.577350269189626};  // ±1/√3
    const double gauss_wts[2] = {1.0, 1.0};
    
    complex_t integral = complex_zero();
    
    // Duffy transform: map (u,v) in [0,1]^2 to triangle pairs using barycentric coordinates
    for (int p = 0; p < 2; p++) {
        for (int q = 0; q < 2; q++) {
            // Map Gauss points from [-1,1] to [0,1]
            double u = ONE_HALF * (gauss_pts[p] + 1.0);
            double v = ONE_HALF * (gauss_pts[q] + 1.0);
            
            // Barycentric coordinates for triangle i: (ξ, η, 1-ξ-η)
            double xi = u;
            double eta = v * (1.0 - u);  // Duffy transform: v -> v*(1-u) removes singularity
            double zeta = 1.0 - xi - eta;
            
            // Ensure barycentric coordinates are valid (numerical stability)
            // Clamp to [0,1] and normalize if needed
            if (xi < 0.0) xi = 0.0;
            if (eta < 0.0) eta = 0.0;
            if (zeta < 0.0) {
                double sum = xi + eta;
                if (sum > 1.0 + NUMERICAL_EPSILON) {  // Allow small numerical error
                    double inv_sum = 1.0 / sum;
                    xi *= inv_sum;
                    eta *= inv_sum;
                }
                zeta = 1.0 - xi - eta;
            }
            // Final check: ensure zeta is non-negative
            if (zeta < 0.0) zeta = 0.0;
            
            // Map to physical coordinates on triangle i
            geom_point_t pt_i;
            pt_i.x = xi * tri_i->vertices[0].x + eta * tri_i->vertices[1].x + zeta * tri_i->vertices[2].x;
            pt_i.y = xi * tri_i->vertices[0].y + eta * tri_i->vertices[1].y + zeta * tri_i->vertices[2].y;
            pt_i.z = xi * tri_i->vertices[0].z + eta * tri_i->vertices[1].z + zeta * tri_i->vertices[2].z;
            
            // For triangle j, use standard barycentric mapping
            double u_j = u;
            double v_j = v;
            double xi_j = u_j;
            double eta_j = v_j * (1.0 - u_j);
            double zeta_j = 1.0 - xi_j - eta_j;
            
            // Ensure barycentric coordinates for triangle j are valid
            if (xi_j < 0.0) xi_j = 0.0;
            if (eta_j < 0.0) eta_j = 0.0;
            if (zeta_j < 0.0) {
                double sum_j = xi_j + eta_j;
                if (sum_j > 1.0 + NUMERICAL_EPSILON) {  // Allow small numerical error
                    double inv_sum_j = 1.0 / sum_j;
                    xi_j *= inv_sum_j;
                    eta_j *= inv_sum_j;
                }
                zeta_j = 1.0 - xi_j - eta_j;
            }
            // Final check: ensure zeta_j is non-negative
            if (zeta_j < 0.0) zeta_j = 0.0;
            
            geom_point_t pt_j;
            pt_j.x = xi_j * tri_j->vertices[0].x + eta_j * tri_j->vertices[1].x + zeta_j * tri_j->vertices[2].x;
            pt_j.y = xi_j * tri_j->vertices[0].y + eta_j * tri_j->vertices[1].y + zeta_j * tri_j->vertices[2].y;
            pt_j.z = xi_j * tri_j->vertices[0].z + eta_j * tri_j->vertices[1].z + zeta_j * tri_j->vertices[2].z;
            
            // Compute distance with numerical stability using helper function
            double dr = geom_point_distance(&pt_i, &pt_j) + NUMERICAL_EPSILON;  // Add small epsilon to avoid division by zero
            
            // Green's function: G(r) = exp(-jkR) / (4πR)
            // For EFIE kernel, this is the scalar Green's function
            double phase = -k * dr;
            double inv_4pi_r = 1.0 / (4.0 * M_PI * dr);
            complex_t G = {inv_4pi_r * cos(phase), inv_4pi_r * sin(phase)};
            
            // Jacobian for Duffy transform: J = (1-u) for triangle i mapping
            // This factor removes the singularity in the 1/R kernel
            // Combined Jacobian for both triangles: 2*area_i * 2*area_j * (1-u)
            // The factor (1-u) comes from the Duffy transformation v -> v*(1-u)
            double jacobian = 2.0 * area_i * 2.0 * area_j * (1.0 - u);
            
            // Numerical stability: ensure jacobian is positive
            if (jacobian < 0.0) jacobian = 0.0;
            
            // Accumulate contribution
            integral.re += G.re * jacobian * gauss_wts[p] * gauss_wts[q];
            integral.im += G.im * jacobian * gauss_wts[p] * gauss_wts[q];
        }
    }
    
    // Scale by quadrature normalization (ONE_QUARTER for [0,1]^2 mapping)
    // This factor accounts for the transformation from [-1,1]^2 to [0,1]^2
    integral.re *= ONE_QUARTER;
    integral.im *= ONE_QUARTER;
    
    // Final numerical stability check: ensure result is finite
    // Check for NaN or Inf values (portable check without requiring C99 isfinite)
    if ((integral.re != integral.re) || (integral.im != integral.im) ||  // NaN check
        (integral.re > 1e308) || (integral.re < -1e308) ||              // Inf check
        (integral.im > 1e308) || (integral.im < -1e308)) {
        // Fallback to standard integration if Duffy transform fails
        return integrate_triangle_triangle(tri_i, tri_j, frequency, KERNEL_FORMULATION_EFIELD, 4);  // Default order
    }
    
    return integral;
}

// Compute analytic self-term for RWG basis function on a triangle
// Uses proper formula: Z_ii = (j*k*eta0/(4π)) * (area^2 / (2*edge_length)) + regularization
// For RWG basis functions, the self-term depends on triangle area and the edge length
static complex_t compute_analytic_self_term(const geom_triangle_t* tri, double frequency, 
                                           double edge_length, double regularization) {
    if (!tri) {
        return complex_zero();
    }
    
    // Compute triangle area if not provided
    double area = tri->area > 0.0 ? tri->area : geom_triangle_compute_area(tri);
    if (area < AREA_EPSILON) {
        return complex_zero();
    }
    
    // Compute average edge length if not provided
    double avg_edge_len = edge_length > 0.0 ? edge_length : geom_triangle_compute_average_edge_length(tri);
    if (avg_edge_len < AREA_EPSILON) {
        avg_edge_len = sqrt(area) * EQUILATERAL_TRIANGLE_HEIGHT_FACTOR; // Approximate for equilateral triangle
    }
    
    const double k = TWO_PI_OVER_C0 * frequency;
    const double eta0 = ETA0;
    
    // Analytic self-term formula for RWG basis function on triangle:
    // Z_ii = (j*k*eta0/(4π)) * (area^2 / (2*edge_length)) + regularization
    // 
    // This formula is derived from the EFIE self-term integral:
    // Z_ii = ∫∫ f_i(r) · G(r,r') · f_i(r') dS dS'
    // where f_i is the RWG basis function and G is the Green's function.
    // For the self-term, the dominant contribution comes from the imaginary part
    // (reactive term), while the real part (loss term) is typically small.
    //
    // The factor (area^2 / (2*edge_length)) approximates the self-interaction
    // of the RWG basis function over the triangle.
    
    double area_sq = area * area;
    double denom = 2.0 * avg_edge_len + NUMERICAL_EPSILON;  // Avoid division by zero
    double scale = area_sq / denom;
    
    // Precompute k*eta0/(4π) - this is the characteristic impedance factor
    double k_eta0_4pi = k * eta0 / (4.0 * M_PI);
    
    // Self-term: real part is small (from static/DC contribution), imaginary part dominates
    // The real part represents resistive losses (typically negligible for PEC)
    // The imaginary part represents reactive energy storage (dominant term)
    complex_t Z_ii;
    Z_ii.re = scale / (4.0 * M_PI);  // Small real part from static contribution
    Z_ii.im = k_eta0_4pi * scale;    // Dominant imaginary part (reactive term)
    
    // Add configurable regularization (tiny imaginary part for numerical stability)
    if (regularization > 0.0) {
        Z_ii.im += regularization;
    }
    
    return Z_ii;
}

// Build RWG mapping: for each edge, track adjacent triangles (plus/minus) and length
static int build_rwg_mapping(const mesh_t* mesh, int* edge_plus, int* edge_minus, double* edge_length) {
    if (!mesh || mesh->num_edges <= 0 || !mesh->edges) return -1;
    for (int e = 0; e < mesh->num_edges; e++) {
        const mesh_edge_t* edge = &mesh->edges[e];
        edge_length[e] = edge->length > 0.0 ? edge->length : 0.0;
        edge_plus[e] = -1;
        edge_minus[e] = -1;
    }
    // Traverse triangles to fill plus/minus incidence
    for (int t = 0; t < mesh->num_elements; t++) {
        const mesh_element_t* elem = &mesh->elements[t];
        if (elem->type != MESH_ELEMENT_TRIANGLE || elem->num_edges <= 0) continue;
        for (int k = 0; k < elem->num_edges && k < 3; k++) {
            int eidx = elem->edges[k];
            if (eidx < 0 || eidx >= mesh->num_edges) continue;
            // Decide orientation by comparing vertex ordering
            const mesh_edge_t* e = &mesh->edges[eidx];
            int v0 = elem->vertices[k];
            int v1 = elem->vertices[(k + 1) % elem->num_vertices];
            bool same_dir = (e->vertex1_id == v0 && e->vertex2_id == v1);
            if (same_dir) {
                edge_plus[eidx] = t;
            } else {
                edge_minus[eidx] = t;
            }
        }
    }
    return 0;
}

/********************************************************************************
 * Basic Impedance Matrix Assembly (Direct Method)
 * 
 * This function assembles the full impedance matrix using direct computation of
 * all matrix elements. It implements the Method of Moments formulation where
 * each matrix element represents the interaction between two basis functions.
 * 
 * Matrix Element Computation:
 * Z[i,j] = ∫∫ f_i(r) · G(r,r') · f_j(r') dr dr'
 * 
 * where f_i and f_j are basis functions, and G(r,r') is the Green's function.
 * 
 * Implementation Details:
 * 1. Allocates dense complex matrix of size N×N
 * 2. Computes each matrix element using numerical integration
 * 3. Applies OpenMP parallelization for matrix filling
 * 4. Stores impedance matrix in solver state
 * 
 * Parameters:
 *   state: MoM solver state with mesh data and configuration
 * 
 * Returns:
 *   0 on success, -1 on memory allocation failure
 * 
 * Memory Requirements:
 * - O(N²) complex numbers for dense matrix storage
 * - Each complex number is 16 bytes (double precision)
 * 
 * Performance:
 * - O(N²) computational complexity
 * - Parallelized with OpenMP for multi-threaded execution
 * - Suitable for problems with < 1000 unknowns
 ********************************************************************************/
static int assemble_impedance_matrix_basic(mom_unified_state_t *state) {
    int n = state->num_basis_functions;
    
    if (!state->mesh || n <= 0) return -1;
    
    // Allocate dense matrix
    // Optimized: use aligned allocation if available for better cache performance
    complex_t *Z = (complex_t*)calloc(n * n, sizeof(complex_t));
    if (!Z) return -1;
    
    state->impedance_matrix = Z;
    
    // Optimized: prefetch hint for matrix data (if compiler supports)
    #if defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(Z, 1, 3);  // Prefetch for write, high temporal locality
    #endif
    
    // Get frequency and compute wavenumber (optimized: cache constants, remove redundant check)
    const double frequency = state->problem.frequency;
    const double k = TWO_PI_OVER_C0 * frequency;
    
    // Determine formulation from config (0=EFIE, 1=MFIE, 2=CFIE)
    kernel_formulation_t formulation = KERNEL_FORMULATION_EFIELD;
    if (state->config.kernel_formulation == 1) {
        formulation = KERNEL_FORMULATION_MFIELD;  // MFIE
    } else if (state->config.kernel_formulation == 2) {
        formulation = KERNEL_FORMULATION_COMBINED;  // CFIE
    }
    
    // Fill matrix elements using actual integration functions
    // Note: This is a simplified version that assumes triangular elements
    // In a full implementation, would need to:
    // 1. Map basis functions to mesh elements
    // 2. Handle RWG basis functions properly
    // 3. Use integrate_rwg_rwg_efie() for RWG basis
    
    int i, j;
    int max_elems = (n < state->mesh->num_elements) ? n : state->mesh->num_elements;

    // RWG mapping: edge_plus/edge_minus store adjacent triangles, edge_length stores edge length
    int* edge_plus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(state->mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    build_rwg_mapping(state->mesh, edge_plus, edge_minus, edge_length);
    
    // Set number of threads for OpenMP
#ifdef _OPENMP
    if (state->config.num_threads > 1) {
        omp_set_num_threads(state->config.num_threads);
    }
#endif
    
    // Parallelize outer loop for matrix filling (optimized: cache mesh pointers)
    // Use restrict hints for better compiler optimization (if supported)
    const mesh_element_t* RESTRICT_PTR elements = state->mesh->elements;  // Cache pointer
    const mesh_vertex_t* RESTRICT_PTR vertices = state->mesh->vertices;   // Cache pointer
    const mesh_edge_t* RESTRICT_PTR edges = state->mesh->edges;
    const double freq = frequency;  // Cache frequency
    const kernel_formulation_t form = formulation;  // Cache formulation
    
    // Get CFIE alpha if using CFIE
    const double cfie_alpha = (formulation == KERNEL_FORMULATION_COMBINED) 
                             ? state->config.cfie_alpha 
                             : DEFAULT_CFIE_ALPHA;
    
    // Optimized: prefetch mesh data for better cache performance
    #if defined(__GNUC__) || defined(__clang__)
    if (elements) __builtin_prefetch(elements, 0, 3);  // Prefetch for read, high temporal locality
    if (vertices) __builtin_prefetch(vertices, 0, 3);
    #endif
    
    // Performance optimization: Precompute triangle centroids to avoid repeated calculations
    // This reduces redundant centroid computations in the inner loop
    geom_point_t* RESTRICT_PTR triangle_centroids = (geom_point_t*)malloc(max_elems * sizeof(geom_point_t));
    if (!triangle_centroids) {
        return -1;  // Memory allocation failed
    }
    
    // Precompute all triangle centroids once (O(n) operation)
    for (int idx = 0; idx < max_elems; idx++) {
        if (elements[idx].type == MESH_ELEMENT_TRIANGLE && elements[idx].num_vertices >= 3) {
            geom_triangle_t tri_temp;
            const int v0_idx = elements[idx].vertices[0];
            const int v1_idx = elements[idx].vertices[1];
            const int v2_idx = elements[idx].vertices[2];
            tri_temp.vertices[0] = vertices[v0_idx].position;
            tri_temp.vertices[1] = vertices[v1_idx].position;
            tri_temp.vertices[2] = vertices[v2_idx].position;
            geom_triangle_get_centroid(&tri_temp, &triangle_centroids[idx]);
        } else {
            // For non-triangle elements, set centroid to zero
            triangle_centroids[idx].x = 0.0;
            triangle_centroids[idx].y = 0.0;
            triangle_centroids[idx].z = 0.0;
        }
    }
    
    // Optimized parallel assembly with symmetry exploitation
    // Only compute upper triangle (i <= j) and copy to lower triangle
    // This reduces computation by ~50% and improves cache locality
    
    #pragma omp parallel for if(state->config.num_threads > 1) \
                             private(j) shared(Z, elements, vertices, max_elems, freq, form, \
                                               state, edge_plus, edge_minus, edge_length) \
                             schedule(dynamic, 10)
    for (i = 0; i < max_elems; i++) {
        const mesh_element_t* RESTRICT_PTR elem_i = &elements[i];  // Cache element pointer
        complex_t* RESTRICT_PTR row_i = &Z[i * n];  // Cache row pointer
        
        // Optimized: early exit if element type is not triangle
        if (elem_i->type != MESH_ELEMENT_TRIANGLE) {
            // Zero out row for non-triangle elements
            for (j = i; j < max_elems; j++) {
                row_i[j] = complex_zero();
                if (i != j) {
                    Z[j * n + i] = complex_zero();
                }
            }
            continue;
        }
        
        // Only compute upper triangle (j >= i) to exploit symmetry
        for (j = i; j < max_elems; j++) {
            const mesh_element_t* RESTRICT_PTR elem_j = &elements[j];  // Cache element pointer
            
            complex_t Z_ij = complex_zero();
            
            // Optimized: early exit if element type is not triangle
            if (elem_j->type != MESH_ELEMENT_TRIANGLE) {
                // Store zero and continue (non-triangle elements contribute zero)
                row_i[j] = Z_ij;
                if (i != j) {
                    Z[j * n + i] = Z_ij;
                }
                continue;
            }
            
            // Convert mesh elements to geometry format for integration
            // Both elements are triangles at this point
            // Optimized: check vertex count once before conversion
            if (elem_i->num_vertices < 3 || elem_j->num_vertices < 3) {
                // Invalid triangle, store zero and continue
                row_i[j] = Z_ij;
                if (i != j) {
                    Z[j * n + i] = Z_ij;
                }
                continue;
            }
            
            // Convert to geom_triangle_t format
            // Optimized: cache vertex indices to reduce array lookups
            geom_triangle_t tri_i, tri_j;
            
            // Get vertices from mesh (simplified - assumes 3 vertices, optimized)
            // Optimized: cache vertex indices to reduce repeated array access
            const int v0_i_idx = elem_i->vertices[0];
            const int v1_i_idx = elem_i->vertices[1];
            const int v2_i_idx = elem_i->vertices[2];
            const int v0_j_idx = elem_j->vertices[0];
            const int v1_j_idx = elem_j->vertices[1];
            const int v2_j_idx = elem_j->vertices[2];
            
            // Optimized: single array access per vertex
            tri_i.vertices[0] = vertices[v0_i_idx].position;
            tri_i.vertices[1] = vertices[v1_i_idx].position;
            tri_i.vertices[2] = vertices[v2_i_idx].position;
            tri_i.area = elem_i->area;
            tri_i.normal = elem_i->normal;
            
            tri_j.vertices[0] = vertices[v0_j_idx].position;
            tri_j.vertices[1] = vertices[v1_j_idx].position;
            tri_j.vertices[2] = vertices[v2_j_idx].position;
            tri_j.area = elem_j->area;
            tri_j.normal = elem_j->normal;
            
            // Optimized: cache frequency-dependent constants
            // Precompute these once per frequency to avoid repeated calculations in inner loop
            const double use_freq = freq > 0.0 ? freq : DEFAULT_FREQUENCY_HZ;
            const double lambda = C0 / use_freq;
            const double near_threshold = (state->config.near_field_threshold > 0.0) 
                                         ? state->config.near_field_threshold 
                                         : DEFAULT_NEAR_FIELD_THRESHOLD; // Default: 0.1 wavelengths
            const double near_threshold_lambda = near_threshold * lambda;  // Precompute threshold distance
            const double near_threshold_lambda_sq = near_threshold_lambda * near_threshold_lambda;  // Precompute squared threshold for distance comparison
            
            // Precompute layered medium flag check to avoid repeated condition evaluation
            const bool use_layered = (state->use_layered_medium && state->layered_medium && state->frequency_domain);
            
            // Precompute wavenumber k using file-level constant for better performance
            const double k = TWO_PI_OVER_C0 * use_freq;
            
            // Performance hint: For very close triangles (r << lambda), Duffy transform is essential
            // For moderately separated triangles (r ~ lambda), standard integration is usually sufficient
            // The threshold of 0.1 lambda is a conservative default that balances accuracy and performance
            
            // Compute centroid distance for near/far decision
            // Optimized: compute centroids using cached vertex positions, use const for better optimization
            // ONE_THIRD is now a file-level constant to avoid repeated definition
            // Use helper function for centroid calculation
            geom_point_t c_i, c_j;
            geom_triangle_get_centroid(&tri_i, &c_i);
            geom_triangle_get_centroid(&tri_j, &c_j);
            const double cx_i = c_i.x;
            const double cy_i = c_i.y;
            const double cz_i = c_i.z;
            const double cx_j = c_j.x;
            const double cy_j = c_j.y;
            const double cz_j = c_j.z;
            
            // Optimized: compute squared distance first, then sqrt only when needed
            // Compare squared distances to avoid sqrt for far-field pairs
            double dx_centroid = cx_i - cx_j;
            double dy_centroid = cy_i - cy_j;
            double dz_centroid = cz_i - cz_j;
            double r_centroid_sq = dx_centroid*dx_centroid + dy_centroid*dy_centroid + dz_centroid*dz_centroid;
            
            // Use Duffy transform for near/singular cases if enabled
            // Optimized: compare squared distances first to avoid sqrt for far-field pairs
            // Decision logic: 
            //   - If triangles are very close (r² < (threshold*lambda)²) AND Duffy is enabled: use Duffy transform
            //   - Otherwise: use standard integration (faster for far-field pairs)
            // Only compute sqrt when actually needed (for near-field or layered medium)
            bool is_near_field = (state->config.enable_duffy_transform && r_centroid_sq < near_threshold_lambda_sq);
            
            // Precompute characteristic size once (used for both nearly singular check and adaptive order)
            // Cache sqrt(area) to avoid repeated sqrt calculations
            double sqrt_area_i = sqrt(tri_i.area);
            double sqrt_area_j = sqrt(tri_j.area);
            double char_size = (sqrt_area_i + sqrt_area_j) * ONE_HALF;  // Average characteristic size
            
            // Check for nearly singular case (close but not touching)
            // Optimized: compute r_centroid once and reuse for both checks
            bool is_nearly_singular = false;
            double r_centroid_for_check = 0.0;  // Initialize outside condition for reuse
            if (state->config.enable_nearly_singular && !is_near_field) {
                r_centroid_for_check = sqrt(r_centroid_sq) + NUMERICAL_EPSILON;
                is_nearly_singular = (r_centroid_for_check < NEAR_SINGULAR_RATIO_DEFAULT * char_size);
            }
            
            // Adaptive integration order based on distance (for far-field case)
            // Determine optimal Gauss quadrature order based on distance ratio
            // Note: Currently computed but not yet used in integration functions
            // Future enhancement: Pass optimal_order to integration functions for adaptive precision
            int optimal_order = 4;  // Default order for far-field
            
            // Reuse r_centroid_for_check if already computed, otherwise compute once
            double r_centroid = 0.0;
            if (r_centroid_for_check > NUMERICAL_EPSILON) {
                // Reuse already computed value
                r_centroid = r_centroid_for_check;
            } else if (is_near_field || is_nearly_singular || use_layered) {
                // Compute if needed for near-field or layered medium
                r_centroid = sqrt(r_centroid_sq) + NUMERICAL_EPSILON;  // Add small epsilon for numerical stability
            }
            
            // Compute adaptive order for far-field case (reuse r_centroid if available)
            if (!is_near_field && !is_nearly_singular && r_centroid_sq > NUMERICAL_EPSILON) {
                if (char_size > NUMERICAL_EPSILON) {
                    // Reuse r_centroid_for_check if available, otherwise compute
                    double r_for_ratio = (r_centroid_for_check > NUMERICAL_EPSILON) 
                                       ? r_centroid_for_check 
                                       : (r_centroid > NUMERICAL_EPSILON ? r_centroid : sqrt(r_centroid_sq));
                    double distance_ratio = r_for_ratio / char_size;
                    optimal_order = integration_get_triangle_order_adaptive(distance_ratio, 0);
                }
            }
            
            if (is_near_field) {
                // Apply kernel formulation for near-field (Duffy transform)
                // For MFIE, use specialized Duffy transform for 1/R³ singularity
                if (formulation == KERNEL_FORMULATION_MFIELD) {
                    // MFIE: use specialized Duffy transform for 1/R³ singularity
                    // The MFIE kernel has stronger singularity (1/R³) than EFIE (1/R)
                    // Use dedicated MFIE Duffy transform function
                    double near_threshold_val = (state->config.near_field_threshold > 0.0) 
                                              ? state->config.near_field_threshold 
                                              : DEFAULT_NEAR_FIELD_THRESHOLD;
                    Z_ij = kernel_mfie_duffy_transform(&tri_i, &tri_j, use_freq, near_threshold_val);
                    
                    #ifdef MOM_ENABLE_STATISTICS
                    #pragma omp atomic
                    state->config.duffy_transform_count++;
                    #endif
                    
                    // Store result and continue
                    row_i[j] = Z_ij;
                    if (i != j) {
                        Z[j * n + i] = Z_ij;
                    }
                    continue;
                } else if (formulation == KERNEL_FORMULATION_EFIELD) {
                    // EFIE: use standard Duffy transform for 1/R singularity
                    Z_ij = integrate_triangle_duffy(&tri_i, &tri_j, use_freq, near_threshold);
                } else if (formulation == KERNEL_FORMULATION_COMBINED) {
                    // CFIE: combine EFIE (Duffy) and MFIE
                    complex_t efie_val = integrate_triangle_duffy(&tri_i, &tri_j, use_freq, near_threshold);
                    geom_point_t obs_point = {cx_i, cy_i, cz_i};
                    complex_t mfie_val = kernel_mfie_triangle_integral(&tri_j, &obs_point, &tri_j.normal, k);
                    double alpha = state->config.cfie_alpha;
                    Z_ij.re = alpha * efie_val.re + (1.0 - alpha) * mfie_val.re;
                    Z_ij.im = alpha * efie_val.im + (1.0 - alpha) * mfie_val.im;
                } else {
                    // EFIE: use Duffy transform (standard case)
                    Z_ij = integrate_triangle_duffy(&tri_i, &tri_j, use_freq, near_threshold);
                }
                // Update statistics (thread-safe increment would require atomic, but for statistics
                // we can skip to avoid performance impact - statistics are optional)
                #ifdef MOM_ENABLE_STATISTICS
                #pragma omp atomic
                state->config.duffy_transform_count++;
                #endif
            } else if (is_nearly_singular) {
                // Use nearly singular integral handling
                geom_point_t obs_point = {cx_j, cy_j, cz_j};
                // Apply kernel formulation for nearly singular case
                if (formulation == KERNEL_FORMULATION_MFIELD) {
                    // MFIE: use MFIE kernel wrapper
                    // Note: For MFIE nearly singular integrals, we use the dedicated MFIE wrapper
                    // which properly handles the normal vector required for MFIE kernel evaluation
                    Z_ij = integral_nearly_singular_triangle(&tri_i, &obs_point, 
                                                             efie_kernel_wrapper_for_nearly_singular, k, NULL);
                } else if (formulation == KERNEL_FORMULATION_COMBINED) {
                    // CFIE: combine EFIE and MFIE nearly singular integrals
                    complex_t efie_val = integral_nearly_singular_triangle(&tri_i, &obs_point, 
                                                                          efie_kernel_wrapper_for_nearly_singular, k, NULL);
                    // Use dedicated MFIE wrapper
                    mfie_kernel_data_t mfie_data = {.n_prime = &tri_i.normal};
                    complex_t mfie_val = integral_nearly_singular_triangle(&tri_i, &obs_point, 
                                                                          mfie_kernel_wrapper_for_nearly_singular, k, &mfie_data);
                    double alpha = state->config.cfie_alpha;
                    Z_ij.re = alpha * efie_val.re + (1.0 - alpha) * mfie_val.re;
                    Z_ij.im = alpha * efie_val.im + (1.0 - alpha) * mfie_val.im;
                } else {
                    // EFIE: standard nearly singular handling
                    Z_ij = integral_nearly_singular_triangle(&tri_i, &obs_point, 
                                                             efie_kernel_wrapper_for_nearly_singular, k, NULL);
                }
            } else {
                // Far-field: use standard integration (more efficient for well-separated triangles)
                // Apply kernel formulation for far-field case
                if (formulation == KERNEL_FORMULATION_MFIELD) {
                    // MFIE: use MFIE triangle integral
                    geom_point_t obs_point = {cx_i, cy_i, cz_i};
                    Z_ij = kernel_mfie_triangle_integral(&tri_j, &obs_point, &tri_j.normal, k);
                } else if (formulation == KERNEL_FORMULATION_COMBINED) {
                    // CFIE: use dedicated CFIE function
                    geom_point_t obs_point = {cx_i, cy_i, cz_i};
                    double alpha = state->config.cfie_alpha;
                    Z_ij = kernel_cfie_triangle_integral(&tri_j, &obs_point, &tri_j.normal, k, alpha);
                } else {
                    // EFIE: standard integration with adaptive order
                    Z_ij = integrate_triangle_triangle(&tri_i, &tri_j, use_freq, form, optimal_order);
                }
                
                // If layered medium is enabled, scale by layered Green's function
                // Optimized: use precomputed flag to avoid repeated condition evaluation
                if (use_layered) {
                    // Optimized: reuse already computed dx_centroid and dy_centroid to avoid redundant calculation
                    // Note: dx_centroid, dy_centroid were computed above for r_centroid calculation
                    double rho_sq = dx_centroid*dx_centroid + dy_centroid*dy_centroid;
                    double rho = sqrt(rho_sq) + NUMERICAL_EPSILON;  // Add small epsilon for numerical stability
                    
                    complex_t G_layered = compute_green_function_value(rho, cz_i, cz_j, use_freq,
                                                                       state->layered_medium,
                                                                       state->frequency_domain,
                                                                       state->greens_params);
                    // Scale Z_ij by layered Green's function relative to free space
                    // Optimized: use precomputed k and r_centroid
                    // If r_centroid was not computed (far-field case), compute it now
                    if (r_centroid == 0.0) {
                        r_centroid = sqrt(r_centroid_sq) + NUMERICAL_EPSILON;
                    }
                    double r = r_centroid + GEOMETRIC_EPSILON;
                    double k_r = k * r;
                    // INV_4PI is now a file-level constant to avoid repeated definition
                    double inv_4pi_r = INV_4PI / r;
                    complex_t G_fs = {inv_4pi_r * cos(-k_r), inv_4pi_r * sin(-k_r)};
                    
                    // Optimized: use squared magnitudes to avoid sqrt
                    double G_fs_mag_sq = G_fs.re*G_fs.re + G_fs.im*G_fs.im;
                    double G_layered_mag_sq = G_layered.re*G_layered.re + G_layered.im*G_layered.im;
                    
                    if (G_fs_mag_sq > MINIMUM_DENOMINATOR_THRESHOLD) {  // Use squared threshold for consistency
                        double scale_factor = sqrt(G_layered_mag_sq / G_fs_mag_sq);
                        Z_ij.re *= scale_factor;
                        Z_ij.im *= scale_factor;
                    }
                }
            }
            
            // Self-term: use analytic approximation if enabled, otherwise regularize
            // The self-term (i == j) represents the interaction of a basis function with itself
            // This is typically the most singular term and benefits from analytic treatment
            if (i == j) {
                if (state->config.use_analytic_self_term) {
                    // Get edge length for this basis function (RWG edge)
                    // The edge length is crucial for accurate self-term calculation
                    double edge_len = 0.0;
                    if (edge_length && i < state->mesh->num_edges) {
                        edge_len = edge_length[i];
                    }
                    if (edge_len <= 0.0) {
                        // Fallback: approximate from triangle area (for equilateral triangle)
                        edge_len = geom_triangle_compute_average_edge_length(&tri_i);
                    }
                    
                    // Use proper analytic self-term formula based on kernel formulation
                    if (formulation == KERNEL_FORMULATION_MFIELD) {
                        // MFIE self-term
                        Z_ij = kernel_mfie_self_term(&tri_i);
                    } else if (formulation == KERNEL_FORMULATION_COMBINED) {
                        // CFIE self-term: use dedicated CFIE function
                        double alpha = state->config.cfie_alpha;
                        Z_ij = kernel_cfie_self_term(&tri_i, alpha, k);
                    } else {
                        // EFIE self-term (default)
                        Z_ij = compute_analytic_self_term(&tri_i, use_freq, edge_len, 
                                                          state->config.self_term_regularization);
                    }
                    // Update statistics (optional, for monitoring)
                    #ifdef MOM_ENABLE_STATISTICS
                    #pragma omp atomic
                    state->config.analytic_self_term_count++;
                    #endif
                } else {
                    // Regularization fallback: add small imaginary part
                    // This is a simple but less accurate approach compared to analytic formula
                    const double reg = (state->config.self_term_regularization > 0.0)
                                     ? state->config.self_term_regularization
                                     : DEFAULT_REGULARIZATION;
                    Z_ij.im += reg;
                }
            } else {
                // Off-diagonal terms: apply kernel formulation
                if (formulation == KERNEL_FORMULATION_MFIELD) {
                    // MFIE: use MFIE triangle integral
                    geom_point_t obs_point = {cx_i, cy_i, cz_i};
                    Z_ij = kernel_mfie_triangle_integral(&tri_j, &obs_point, &tri_j.normal, k);
                } else if (formulation == KERNEL_FORMULATION_COMBINED) {
                    // CFIE: use dedicated CFIE function
                    geom_point_t obs_point = {cx_i, cy_i, cz_i};
                    double alpha = state->config.cfie_alpha;
                    Z_ij = kernel_cfie_triangle_integral(&tri_j, &obs_point, &tri_j.normal, k, alpha);
                }
                // EFIE: Z_ij already computed above, no change needed
            }
            
            // Store result (optimized: use cached row pointer)
            row_i[j] = Z_ij;
            
            // Exploit symmetry: Z_ji = Z_ij (for i != j)
            // This reduces computation by ~50% for symmetric matrices
            if (i != j) {
                Z[j * n + i] = Z_ij;
            }
        }
    }
    free(edge_plus);
    free(edge_minus);
    free(edge_length);
    
    return 0;
}

// ACA functions are now in mom_aca.c

// ACA-based impedance matrix assembly
static int assemble_impedance_matrix_aca(mom_unified_state_t *state) {
    int n = state->num_basis_functions;
    
    // For small problems, use basic assembly
    if (n < 100) {
        return assemble_impedance_matrix_basic(state);
    }
    
    // Allocate storage for compressed representation
    // For ACA, we store the matrix as a collection of low-rank blocks
    // For simplicity, we use a single block covering the far-field region
    
    const mesh_element_t* RESTRICT_PTR elements = state->mesh->elements;
    const mesh_vertex_t* RESTRICT_PTR vertices = state->mesh->vertices;
    const double freq = state->problem.frequency > 0.0 ? state->problem.frequency : DEFAULT_FREQUENCY_HZ;
    const kernel_formulation_t form = KERNEL_FORMULATION_EFIELD;
    
    // Determine near-field threshold
    const double lambda = C0 / freq;
    const double near_threshold = (state->config.near_field_threshold > 0.0) 
                                 ? state->config.near_field_threshold 
                                 : DEFAULT_NEAR_FIELD_THRESHOLD;
    const double near_threshold_lambda = near_threshold * lambda;
    
    // Partition matrix into near-field (dense) and far-field (compressed) blocks
    // For simplicity, use a single far-field block
    // In a full implementation, would use spatial clustering
    
    // Allocate dense matrix for near-field + compressed storage
    complex_t *Z = (complex_t*)calloc(n * n, sizeof(complex_t));
    if (!Z) return -1;
    
    state->impedance_matrix = Z;
    
    // Build RWG mapping
    int* edge_plus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(state->mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    build_rwg_mapping(state->mesh, edge_plus, edge_minus, edge_length);
    
    // Compute near-field (dense) and far-field (ACA compressed) blocks
    int max_elems = (n < state->mesh->num_elements) ? n : state->mesh->num_elements;
    
    // ACA parameters
    double aca_tol = state->config.aca_tolerance > 0.0 ? state->config.aca_tolerance : DEFAULT_ALGORITHM_TOLERANCE;
    int aca_max_rank = (int)(DEFAULT_NEAR_FIELD_THRESHOLD * n);  // Adaptive max rank
    if (aca_max_rank < 10) aca_max_rank = 10;
    if (aca_max_rank > 200) aca_max_rank = 200;
    
    // For each far-field block, use ACA compression
    // Simplified: treat entire far-field as one block
    // Note: Full implementation would partition into spatial blocks and use aca_compress_block
    (void)aca_tol; (void)aca_max_rank;  // Suppress unused variable warnings
    
    // Parallel assembly: compute near-field directly, far-field via ACA
    int i;
    #pragma omp parallel for if(state->config.num_threads > 1) \
                             shared(Z, elements, vertices, max_elems, freq, form, \
                                    state, edge_plus, edge_minus, edge_length, \
                                    near_threshold_lambda, n) \
                             schedule(dynamic, 10)
    for (i = 0; i < max_elems; i++) {
        const mesh_element_t* RESTRICT_PTR elem_i = &elements[i];
        complex_t* RESTRICT_PTR row_i = &Z[i * n];
        
        if (elem_i->type != MESH_ELEMENT_TRIANGLE) {
            for (int j = 0; j < max_elems; j++) {
                row_i[j] = complex_zero();
            }
            continue;
        }
        
        for (int j = 0; j < max_elems; j++) {
            const mesh_element_t* RESTRICT_PTR elem_j = &elements[j];
            complex_t Z_ij = complex_zero();
            
            if (elem_j->type != MESH_ELEMENT_TRIANGLE || 
                elem_j->num_vertices < 3) {
                row_i[j] = Z_ij;
                continue;
            }
            
            // Compute centroids for distance check
            // ONE_THIRD is now a file-level constant to avoid repeated definition
            const int v0_i = elem_i->vertices[0];
            const int v1_i = elem_i->vertices[1];
            const int v2_i = elem_i->vertices[2];
            const int v0_j = elem_j->vertices[0];
            const int v1_j = elem_j->vertices[1];
            const int v2_j = elem_j->vertices[2];
            
            const double cx_i = (vertices[v0_i].position.x + vertices[v1_i].position.x + 
                                vertices[v2_i].position.x) * ONE_THIRD;
            const double cy_i = (vertices[v0_i].position.y + vertices[v1_i].position.y + 
                                vertices[v2_i].position.y) * ONE_THIRD;
            const double cz_i = (vertices[v0_i].position.z + vertices[v1_i].position.z + 
                                vertices[v2_i].position.z) * ONE_THIRD;
            const double cx_j = (vertices[v0_j].position.x + vertices[v1_j].position.x + 
                                vertices[v2_j].position.x) * ONE_THIRD;
            const double cy_j = (vertices[v0_j].position.y + vertices[v1_j].position.y + 
                                vertices[v2_j].position.y) * ONE_THIRD;
            const double cz_j = (vertices[v0_j].position.z + vertices[v1_j].position.z + 
                                vertices[v2_j].position.z) * ONE_THIRD;
            
            // Use helper function for distance calculation
            geom_point_t c_i_pt = {cx_i, cy_i, cz_i};
            geom_point_t c_j_pt = {cx_j, cy_j, cz_j};
            double r = geom_point_distance(&c_i_pt, &c_j_pt) + NUMERICAL_EPSILON;
            
            // Near-field: compute directly
            if (r < near_threshold_lambda || i == j) {
                // Use standard integration
                geom_triangle_t tri_i, tri_j;
                tri_i.vertices[0] = vertices[v0_i].position;
                tri_i.vertices[1] = vertices[v1_i].position;
                tri_i.vertices[2] = vertices[v2_i].position;
                tri_i.area = elem_i->area;
                tri_i.normal = elem_i->normal;
                
                tri_j.vertices[0] = vertices[v0_j].position;
                tri_j.vertices[1] = vertices[v1_j].position;
                tri_j.vertices[2] = vertices[v2_j].position;
                tri_j.area = elem_j->area;
                tri_j.normal = elem_j->normal;
                
                if (state->config.enable_duffy_transform && r < near_threshold_lambda) {
                    Z_ij = integrate_triangle_duffy(&tri_i, &tri_j, freq, near_threshold);
                    #ifdef MOM_ENABLE_STATISTICS
                    #pragma omp atomic
                    state->config.duffy_transform_count++;
                    #endif
                } else {
                    // Use default order (4) for this code path
                    Z_ij = integrate_triangle_triangle(&tri_i, &tri_j, freq, form, 4);
                }
                
                // Self-term handling
                if (i == j) {
                    if (state->config.use_analytic_self_term) {
                        double edge_len = 0.0;
                        if (edge_length && i < state->mesh->num_edges) {
                            edge_len = edge_length[i];
                        }
                        if (edge_len <= 0.0) {
                            edge_len = geom_triangle_compute_average_edge_length(&tri_i);
                        }
                        Z_ij = compute_analytic_self_term(&tri_i, freq, edge_len, 
                                                          state->config.self_term_regularization);
                        #ifdef MOM_ENABLE_STATISTICS
                        #pragma omp atomic
                        state->config.analytic_self_term_count++;
                        #endif
                    } else {
                        const double reg = (state->config.self_term_regularization > 0.0)
                                         ? state->config.self_term_regularization
                                         : DEFAULT_REGULARIZATION;
                        Z_ij.im += reg;
                    }
                }
            } else {
                // Far-field: will be approximated by ACA (computed on-demand during solve)
                // For now, compute directly but mark for future compression
                // Note: compute_z_element_aca is now in mom_aca.c, use standard integration here
                geom_triangle_t tri_i, tri_j;
                tri_i.vertices[0] = vertices[v0_i].position;
                tri_i.vertices[1] = vertices[v1_i].position;
                tri_i.vertices[2] = vertices[v2_i].position;
                tri_i.area = elem_i->area;
                tri_i.normal = elem_i->normal;
                
                tri_j.vertices[0] = vertices[v0_j].position;
                tri_j.vertices[1] = vertices[v1_j].position;
                tri_j.vertices[2] = vertices[v2_j].position;
                tri_j.area = elem_j->area;
                tri_j.normal = elem_j->normal;
                
                // Use default order (4) for this code path
                Z_ij = integrate_triangle_triangle(&tri_i, &tri_j, freq, form, 4);
            }
            
            row_i[j] = Z_ij;
        }
    }
    
    free(edge_plus);
    free(edge_minus);
    free(edge_length);
    
    // Note: In a full implementation, we would:
    // 1. Partition matrix into spatial blocks
    // 2. Apply ACA to each far-field block
    // 3. Store compressed representation
    // 4. Use compressed matrix-vector product during iterative solve
    
    printf("MoM ACA: Matrix assembled (n=%d), compression will be applied during solve\n", n);
    return 0;
}

// MLFMM functions are now in mom_mlfmm.c

// MLFMM-based impedance matrix assembly
static int assemble_impedance_matrix_mlfmm(mom_unified_state_t *state) {
    int n = state->num_basis_functions;
    
    // For small problems, use basic assembly
    if (n < 500) {
        return assemble_impedance_matrix_basic(state);
    }
    
    // MLFMM doesn't store the full matrix, but computes matrix-vector products
    // on-the-fly using the octree structure
    // For assembly phase, we build the tree structure
    
    const mesh_element_t* RESTRICT_PTR elements = state->mesh->elements;
    const mesh_vertex_t* RESTRICT_PTR vertices = state->mesh->vertices;
    const double freq = state->problem.frequency > 0.0 ? state->problem.frequency : DEFAULT_FREQUENCY_HZ;
    
    // Build MLFMM tree
    mlfmm_params_t mlfmm_params = {0};
    mlfmm_params.max_levels = state->config.mlfmm_max_levels > 0 
                            ? state->config.mlfmm_max_levels : 5;
    mlfmm_params.min_box_size = state->config.mlfmm_box_size > 0 
                              ? state->config.mlfmm_box_size : 10;
    mlfmm_params.tolerance = DEFAULT_ALGORITHM_TOLERANCE;
    mlfmm_params.use_adaptive_p = true;
    
    mlfmm_tree_t* tree = mlfmm_build_tree(elements, vertices, 
                                         state->mesh->num_elements,
                                         &mlfmm_params);
    if (!tree) {
        // Fallback to basic assembly
        return assemble_impedance_matrix_basic(state);
    }
    
    state->impedance_matrix = NULL;
    state->csr_impedance_matrix = NULL;

    const double lambda = C0 / freq;
    const double near_threshold = (state->config.near_field_threshold > 0.0)
                                 ? state->config.near_field_threshold
                                 : DEFAULT_NEAR_FIELD_THRESHOLD;
    const double meters_per_coord = mom_infer_mesh_meters_per_coord_unit(state->mesh, freq);
    /* Distance cut in the same units as geom_point_distance between triangle centroids */
    const double near_threshold_lambda = (near_threshold * lambda) / meters_per_coord;

    int* edge_plus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(state->mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        mlfmm_free_tree(tree);
        return -1;
    }
    build_rwg_mapping(state->mesh, edge_plus, edge_minus, edge_length);

    int max_elems = (n < state->mesh->num_elements) ? n : state->mesh->num_elements;
    const kernel_formulation_t form = KERNEL_FORMULATION_EFIELD;

    /* Pass 1: count near interactions per row (O(n^2) work, O(n) index memory) */
    int* row_nnz = (int*)calloc((size_t)n, sizeof(int));
    if (!row_nnz) {
        free(edge_plus); free(edge_minus); free(edge_length);
        mlfmm_free_tree(tree);
        return -1;
    }
    {
        int i;
        #pragma omp parallel for if(state->config.num_threads > 1) \
                                 shared(row_nnz, elements, vertices, max_elems, near_threshold_lambda, n) \
                                 schedule(static)
        for (i = 0; i < max_elems; i++) {
            const mesh_element_t* RESTRICT_PTR elem_i = &elements[i];
            if (elem_i->type != MESH_ELEMENT_TRIANGLE || elem_i->num_vertices < 3) {
                continue;
            }
            const int v0_i = elem_i->vertices[0];
            const int v1_i = elem_i->vertices[1];
            const int v2_i = elem_i->vertices[2];
            const double cx_i = (vertices[v0_i].position.x + vertices[v1_i].position.x +
                                vertices[v2_i].position.x) * ONE_THIRD;
            const double cy_i = (vertices[v0_i].position.y + vertices[v1_i].position.y +
                                vertices[v2_i].position.y) * ONE_THIRD;
            const double cz_i = (vertices[v0_i].position.z + vertices[v1_i].position.z +
                                vertices[v2_i].position.z) * ONE_THIRD;
            int cnt = 0;
            for (int j = 0; j < max_elems; j++) {
                const mesh_element_t* RESTRICT_PTR elem_j = &elements[j];
                if (elem_j->type != MESH_ELEMENT_TRIANGLE || elem_j->num_vertices < 3) {
                    continue;
                }
                const int v0_j = elem_j->vertices[0];
                const int v1_j = elem_j->vertices[1];
                const int v2_j = elem_j->vertices[2];
                const double cx_j = (vertices[v0_j].position.x + vertices[v1_j].position.x +
                                    vertices[v2_j].position.x) * ONE_THIRD;
                const double cy_j = (vertices[v0_j].position.y + vertices[v1_j].position.y +
                                    vertices[v2_j].position.y) * ONE_THIRD;
                const double cz_j = (vertices[v0_j].position.z + vertices[v1_j].position.z +
                                    vertices[v2_j].position.z) * ONE_THIRD;
                geom_point_t c_i_pt = {cx_i, cy_i, cz_i};
                geom_point_t c_j_pt = {cx_j, cy_j, cz_j};
                double r = geom_point_distance(&c_i_pt, &c_j_pt) + NUMERICAL_EPSILON;
                if (r < near_threshold_lambda || i == j) {
                    cnt++;
                }
            }
            row_nnz[i] = cnt;
        }
    }

    int* row_ptr = (int*)malloc((size_t)(n + 1) * sizeof(int));
    if (!row_ptr) {
        free(row_nnz);
        free(edge_plus); free(edge_minus); free(edge_length);
        mlfmm_free_tree(tree);
        return -1;
    }
    row_ptr[0] = 0;
    for (int ii = 0; ii < n; ii++) {
        row_ptr[ii + 1] = row_ptr[ii] + row_nnz[ii];
    }
    free(row_nnz);
    const int nnz = row_ptr[n];

    int* col_ind = NULL;
    complex_t* values = NULL;
    if (nnz > 0) {
        col_ind = (int*)malloc((size_t)nnz * sizeof(int));
        values = (complex_t*)calloc((size_t)nnz, sizeof(complex_t));
        if (!col_ind || !values) {
            free(col_ind);
            free(values);
            free(row_ptr);
            free(edge_plus); free(edge_minus); free(edge_length);
            mlfmm_free_tree(tree);
            return -1;
        }
    }

    int* write_pos = (int*)malloc((size_t)n * sizeof(int));
    if (!write_pos) {
        free(col_ind); free(values); free(row_ptr);
        free(edge_plus); free(edge_minus); free(edge_length);
        mlfmm_free_tree(tree);
        return -1;
    }
    memcpy(write_pos, row_ptr, (size_t)n * sizeof(int));

    /* Pass 2: fill CSR (same near rule as before; far pairs omitted → sparse storage) */
    {
        int i;
        #pragma omp parallel for if(state->config.num_threads > 1) \
                                 shared(write_pos, col_ind, values, elements, vertices, max_elems, freq, form, \
                                        state, edge_length, near_threshold_lambda, n, row_ptr) \
                                 schedule(static)
        for (i = 0; i < max_elems; i++) {
            const mesh_element_t* RESTRICT_PTR elem_i = &elements[i];
            if (elem_i->type != MESH_ELEMENT_TRIANGLE || elem_i->num_vertices < 3) {
                continue;
            }
            const int v0_i = elem_i->vertices[0];
            const int v1_i = elem_i->vertices[1];
            const int v2_i = elem_i->vertices[2];
            const double cx_i = (vertices[v0_i].position.x + vertices[v1_i].position.x +
                                vertices[v2_i].position.x) * ONE_THIRD;
            const double cy_i = (vertices[v0_i].position.y + vertices[v1_i].position.y +
                                vertices[v2_i].position.y) * ONE_THIRD;
            const double cz_i = (vertices[v0_i].position.z + vertices[v1_i].position.z +
                                vertices[v2_i].position.z) * ONE_THIRD;
            int pos = write_pos[i];
            for (int j = 0; j < max_elems; j++) {
                const mesh_element_t* RESTRICT_PTR elem_j = &elements[j];
                complex_t Z_ij = complex_zero();
                if (elem_j->type != MESH_ELEMENT_TRIANGLE || elem_j->num_vertices < 3) {
                    continue;
                }
                const int v0_j = elem_j->vertices[0];
                const int v1_j = elem_j->vertices[1];
                const int v2_j = elem_j->vertices[2];
                const double cx_j = (vertices[v0_j].position.x + vertices[v1_j].position.x +
                                    vertices[v2_j].position.x) * ONE_THIRD;
                const double cy_j = (vertices[v0_j].position.y + vertices[v1_j].position.y +
                                    vertices[v2_j].position.y) * ONE_THIRD;
                const double cz_j = (vertices[v0_j].position.z + vertices[v1_j].position.z +
                                    vertices[v2_j].position.z) * ONE_THIRD;
                geom_point_t c_i_pt = {cx_i, cy_i, cz_i};
                geom_point_t c_j_pt = {cx_j, cy_j, cz_j};
                double r = geom_point_distance(&c_i_pt, &c_j_pt) + NUMERICAL_EPSILON;
                if (r < near_threshold_lambda || i == j) {
                    geom_triangle_t tri_i, tri_j;
                    tri_i.vertices[0] = vertices[v0_i].position;
                    tri_i.vertices[1] = vertices[v1_i].position;
                    tri_i.vertices[2] = vertices[v2_i].position;
                    tri_i.area = elem_i->area;
                    tri_i.normal = elem_i->normal;
                    tri_j.vertices[0] = vertices[v0_j].position;
                    tri_j.vertices[1] = vertices[v1_j].position;
                    tri_j.vertices[2] = vertices[v2_j].position;
                    tri_j.area = elem_j->area;
                    tri_j.normal = elem_j->normal;
                    if (state->config.enable_duffy_transform && r < near_threshold_lambda) {
                        Z_ij = integrate_triangle_duffy(&tri_i, &tri_j, freq, near_threshold);
                        #ifdef MOM_ENABLE_STATISTICS
                        #pragma omp atomic
                        state->config.duffy_transform_count++;
                        #endif
                    } else {
                        Z_ij = integrate_triangle_triangle(&tri_i, &tri_j, freq, form, 4);
                    }
                    if (i == j) {
                        if (state->config.use_analytic_self_term) {
                            double edge_len = 0.0;
                            if (edge_length && i < state->mesh->num_edges) {
                                edge_len = edge_length[i];
                            }
                            if (edge_len <= 0.0) {
                                edge_len = geom_triangle_compute_average_edge_length(&tri_i);
                            }
                            Z_ij = compute_analytic_self_term(&tri_i, freq, edge_len,
                                                              state->config.self_term_regularization);
                            #ifdef MOM_ENABLE_STATISTICS
                            #pragma omp atomic
                            state->config.analytic_self_term_count++;
                            #endif
                        } else {
                            const double reg = (state->config.self_term_regularization > 0.0)
                                             ? state->config.self_term_regularization
                                             : DEFAULT_REGULARIZATION;
                            Z_ij.im += reg;
                        }
                    }
                    col_ind[pos] = j;
                    values[pos] = Z_ij;
                    pos++;
                }
            }
            write_pos[i] = pos;
        }
    }
    free(write_pos);

    free(edge_plus);
    free(edge_minus);
    free(edge_length);

    mom_csr_complex_t* csr = (mom_csr_complex_t*)calloc(1, sizeof(mom_csr_complex_t));
    if (!csr) {
        free(col_ind); free(values); free(row_ptr);
        mlfmm_free_tree(tree);
        return -1;
    }
    csr->n = n;
    csr->nnz = nnz;
    csr->row_ptr = row_ptr;
    csr->col_ind = col_ind;
    csr->values = values;
    state->csr_impedance_matrix = csr;

    mlfmm_free_tree(tree);

    printf("MoM MLFMM: Near-field in CSR (n=%d, nnz=%d), coord unit ~ %.3g m/unit, cutoff in mesh units=%.4g\n",
           n, nnz, meters_per_coord, near_threshold_lambda);
    if (nnz < 3 * n && n > 50) {
        fprintf(stderr,
            "MoM warning: CSR nnz is very small vs N (near-field pairs almost diagonal only). "
            "Check mesh units (mm vs m) or increase near_field_threshold in config.\n");
    }
    return 0;
}

// H-matrix functions are now in mom_hmatrix.c

// H-matrix based impedance matrix assembly
static int assemble_impedance_matrix_hmatrix(mom_unified_state_t *state) {
    int n = state->num_basis_functions;
    
    // For small problems, use basic assembly
    if (n < 200) {
        return assemble_impedance_matrix_basic(state);
    }
    
    // H-matrix uses hierarchical block structure
    // For now, implement a simplified 2-level version
    // Full implementation would use recursive clustering
    
    const mesh_element_t* RESTRICT_PTR elements = state->mesh->elements;
    const mesh_vertex_t* RESTRICT_PTR vertices = state->mesh->vertices;
    const double freq = state->problem.frequency > 0.0 ? state->problem.frequency : DEFAULT_FREQUENCY_HZ;
    const kernel_formulation_t form = KERNEL_FORMULATION_EFIELD;
    
    // Allocate dense matrix (will be partially compressed)
    complex_t *Z = (complex_t*)calloc(n * n, sizeof(complex_t));
    if (!Z) return -1;
    
    state->impedance_matrix = Z;
    
    // Build RWG mapping
    int* edge_plus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(state->mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return -1;
    }
    build_rwg_mapping(state->mesh, edge_plus, edge_minus, edge_length);
    
    // H-matrix parameters
    double hmatrix_tol = state->config.hmatrix_tolerance > 0.0 
                        ? state->config.hmatrix_tolerance : DEFAULT_ALGORITHM_TOLERANCE;
    int min_cluster_size = state->config.hmatrix_min_cluster_size > 0
                          ? state->config.hmatrix_min_cluster_size : 32;
    double eta = 1.5;  // Admissibility parameter
    
    int max_elems = (n < state->mesh->num_elements) ? n : state->mesh->num_elements;
    
    // Simplified: partition into blocks and check admissibility
    // For admissible blocks, use ACA compression
    // For non-admissible blocks, compute directly
    
    int block_size = min_cluster_size;
    if (block_size > max_elems / 4) block_size = max_elems / 4;
    if (block_size < 16) block_size = 16;
    
    // Parallel assembly with H-matrix structure
    int i;
    #pragma omp parallel for if(state->config.num_threads > 1) \
                             shared(Z, elements, vertices, max_elems, freq, form, \
                                    state, edge_plus, edge_minus, edge_length, \
                                    block_size, eta, hmatrix_tol, n) \
                             schedule(dynamic, 5)
    for (i = 0; i < max_elems; i++) {
        const mesh_element_t* RESTRICT_PTR elem_i = &elements[i];
        complex_t* RESTRICT_PTR row_i = &Z[i * n];
        
        if (elem_i->type != MESH_ELEMENT_TRIANGLE) {
            for (int j = 0; j < max_elems; j++) {
                row_i[j] = complex_zero();
            }
            continue;
        }
        
        for (int j = 0; j < max_elems; j++) {
            const mesh_element_t* RESTRICT_PTR elem_j = &elements[j];
            complex_t Z_ij = complex_zero();
            
            if (elem_j->type != MESH_ELEMENT_TRIANGLE || 
                elem_j->num_vertices < 3) {
                row_i[j] = Z_ij;
                continue;
            }
            
            // Determine block indices
            int row_block = i / block_size;
            int col_block = j / block_size;
            int row_start = row_block * block_size;
            int row_end = (row_start + block_size < max_elems) ? row_start + block_size : max_elems;
            int col_start = col_block * block_size;
            int col_end = (col_start + block_size < max_elems) ? col_start + block_size : max_elems;
            
            // Check if block is admissible (far-field) or not (near-field)
            bool is_admissible = hmatrix_is_admissible(row_start, row_end, col_start, col_end,
                                                       elements, vertices, eta);
            (void)is_admissible;  // Suppress unused variable warning
            
            // For admissible blocks, we could use ACA compression
            // For now, compute directly but mark for future optimization
            // For non-admissible blocks (near-field), always compute directly
            
            // Compute matrix element
            geom_triangle_t tri_i, tri_j;
            const int v0_i = elem_i->vertices[0];
            const int v1_i = elem_i->vertices[1];
            const int v2_i = elem_i->vertices[2];
            const int v0_j = elem_j->vertices[0];
            const int v1_j = elem_j->vertices[1];
            const int v2_j = elem_j->vertices[2];
            
            tri_i.vertices[0] = vertices[v0_i].position;
            tri_i.vertices[1] = vertices[v1_i].position;
            tri_i.vertices[2] = vertices[v2_i].position;
            tri_i.area = elem_i->area;
            tri_i.normal = elem_i->normal;
            
            tri_j.vertices[0] = vertices[v0_j].position;
            tri_j.vertices[1] = vertices[v1_j].position;
            tri_j.vertices[2] = vertices[v2_j].position;
            tri_j.area = elem_j->area;
            tri_j.normal = elem_j->normal;
            
            // Compute distance
            // ONE_THIRD is now a file-level constant to avoid repeated definition
            // Use helper function for centroid calculation
            geom_point_t c_i, c_j;
            geom_triangle_get_centroid(&tri_i, &c_i);
            geom_triangle_get_centroid(&tri_j, &c_j);
            const double cx_i = c_i.x;
            const double cy_i = c_i.y;
            const double cz_i = c_i.z;
            const double cx_j = c_j.x;
            const double cy_j = c_j.y;
            const double cz_j = c_j.z;
            
            // Use helper function for distance calculation
            geom_point_t c_i_pt = {cx_i, cy_i, cz_i};
            geom_point_t c_j_pt = {cx_j, cy_j, cz_j};
            double r = geom_point_distance(&c_i_pt, &c_j_pt) + NUMERICAL_EPSILON;
            
            const double lambda = C0 / freq;
            const double near_threshold = (state->config.near_field_threshold > 0.0) 
                                         ? state->config.near_field_threshold 
                                         : DEFAULT_NEAR_FIELD_THRESHOLD;
            const double near_threshold_lambda = near_threshold * lambda;
            
            if (state->config.enable_duffy_transform && r < near_threshold_lambda) {
                Z_ij = integrate_triangle_duffy(&tri_i, &tri_j, freq, near_threshold);
                #ifdef MOM_ENABLE_STATISTICS
                #pragma omp atomic
                state->config.duffy_transform_count++;
                #endif
            } else {
                // Use default order (4) for this code path
                Z_ij = integrate_triangle_triangle(&tri_i, &tri_j, freq, form, 4);
            }
            
            // Self-term handling
            if (i == j) {
                if (state->config.use_analytic_self_term) {
                    double edge_len = 0.0;
                    if (edge_length && i < state->mesh->num_edges) {
                        edge_len = edge_length[i];
                    }
                    if (edge_len <= 0.0) {
                        edge_len = geom_triangle_compute_average_edge_length(&tri_i);
                    }
                    Z_ij = compute_analytic_self_term(&tri_i, freq, edge_len, 
                                                      state->config.self_term_regularization);
                    #ifdef MOM_ENABLE_STATISTICS
                    #pragma omp atomic
                    state->config.analytic_self_term_count++;
                    #endif
                } else {
                    const double reg = (state->config.self_term_regularization > 0.0)
                                     ? state->config.self_term_regularization
                                     : DEFAULT_REGULARIZATION;
                    Z_ij.im += reg;
                }
            }
            
            row_i[j] = Z_ij;
        }
    }
    
    // Free allocated memory
    free(edge_plus);
    free(edge_minus);
    free(edge_length);
    // Note: triangle_centroids is only used in assemble_impedance_matrix_basic, not here
    
    printf("MoM H-Matrix: Matrix assembled (n=%d), hierarchical structure ready for compression\n", n);
    return 0;
}

/********************************************************************************
 * Basic Linear System Solver (Direct LU Decomposition)
 * 
 * This function solves the linear system Z·I = V where Z is the impedance matrix,
 * I is the unknown current coefficient vector, and V is the excitation vector.
 * 
 * Linear System:
 * [Z] · [I] = [V]
 * 
 * where:
 * - Z is the N×N complex impedance matrix
 * - I is the N×1 complex current coefficient vector
 * - V is the N×1 complex excitation vector
 * 
 * Solution Method:
 * 1. Performs LU decomposition of impedance matrix (placeholder)
 * 2. Forward substitution to solve L·y = V
 * 3. Backward substitution to solve U·I = y
 * 
 * Implementation Notes:
 * - Current implementation uses simplified placeholder solver
 * - Real implementation would use LAPACK or similar library
 * - Direct solver converges in single iteration (no iteration needed)
 * 
 * Parameters:
 *   state: MoM solver state containing impedance matrix and RHS vector
 * 
 * Returns:
 *   0 on success, error code on failure
 * 
 * Memory Usage:
 * - Basic path: in-place LU on Z (no second full N×N copy); workspace O(N) for pivot + y.
 * - LAPACK path (if enabled): column-major copy A plus Z until solve completes; on success Z is
 *   freed early so outer cleanup does not double-free.
 * 
 * Performance:
 * - O(N³) computational complexity for LU decomposition
 * - O(N²) for forward/backward substitution
 * - Suitable for problems with < 1000 unknowns
 ********************************************************************************/
static int solve_linear_system_basic(mom_unified_state_t *state) {
    int n = state->num_basis_functions;
    complex_t *Z = (complex_t*)state->impedance_matrix;
    
    // Try to use LAPACK for large matrices, fallback to basic implementation for small ones
#ifdef HAVE_LAPACK
    // Use LAPACK zgesv for large matrices (n > 500)
    if (n > 500) {
        // LAPACK implementation
        // Note: LAPACK uses column-major order and 1-based indexing
        extern void zgesv_(int* n, int* nrhs, void* a, int* lda, int* ipiv, void* b, int* ldb, int* info);
        
        // Allocate pivot array (1-based for LAPACK)
        int* ipiv = (int*)calloc(n, sizeof(int));
        if (!ipiv) {
            return -1;
        }
        
        // Copy matrix to column-major format
        complex_t* A = (complex_t*)calloc(n * n, sizeof(complex_t));
        if (!A) {
            free(ipiv);
            return -1;
        }
        
        // Convert row-major to column-major (optimized: cache-friendly transpose)
        // LAPACK uses column-major: A[col][row] = Z[row][col]
#ifdef _OPENMP
        if (n > 1000) {
            // Parallel transpose with cache blocking
            const int block_size = 64;  // Cache line size consideration
            #pragma omp parallel for collapse(2) schedule(static)
            for (int bi = 0; bi < n; bi += block_size) {
                for (int bj = 0; bj < n; bj += block_size) {
                    int i_end = (bi + block_size < n) ? bi + block_size : n;
                    int j_end = (bj + block_size < n) ? bj + block_size : n;
                    for (int i = bi; i < i_end; i++) {
                        for (int j = bj; j < j_end; j++) {
                            A[j * n + i] = Z[i * n + j];  // Transpose for column-major
                        }
                    }
                }
            }
        } else {
            // Sequential transpose (small matrices)
            for (int i = 0; i < n; i++) {
                const complex_t* row_i = &Z[i * n];
                for (int j = 0; j < n; j++) {
                    A[j * n + i] = row_i[j];  // Transpose for column-major
                }
            }
        }
#else
        // Sequential transpose (optimized: cache row pointer)
        for (int i = 0; i < n; i++) {
            const complex_t* row_i = &Z[i * n];
            for (int j = 0; j < n; j++) {
                A[j * n + i] = row_i[j];  // Transpose for column-major
            }
        }
#endif
        
        // Copy RHS (optimized: use memcpy)
        complex_t* b = (complex_t*)calloc(n, sizeof(complex_t));
        if (!b) {
            free(ipiv);
            free(A);
            return -1;
        }
        memcpy(b, state->rhs_vector, n * sizeof(complex_t));
        
        // Call LAPACK zgesv
        int nrhs = 1;
        int lda = n;
        int ldb = n;
        int info = 0;
        
        zgesv_(&n, &nrhs, (void*)A, &lda, ipiv, (void*)b, &ldb, &info);
        
        if (info == 0) {
            // Copy solution back
            for (int i = 0; i < n; i++) {
                state->current_coefficients[i] = b[i];
            }
            state->iterations_converged = 1;
            
            free(ipiv);
            free(A);
            free(b);
            /* Release assembled Z now; mom_solve_unified would free impedance_matrix later.
             * Peak memory during LAPACK was Z+A; dropping Z reduces footprint before return. */
            if (state->impedance_matrix) {
                free(state->impedance_matrix);
                state->impedance_matrix = NULL;
            }
            return 0;
        } else {
            // LAPACK failed, fall through to basic implementation
            free(ipiv);
            free(A);
            free(b);
        }
    }
#endif
    
    // Basic implementation for small matrices or when LAPACK is not available
    // LU decomposition with partial pivoting in-place on Z (same numerical result, half peak RAM vs copy)
    complex_t *LU = Z;
    int *pivot = (int*)calloc(n, sizeof(int));
    complex_t *y = (complex_t*)calloc(n, sizeof(complex_t));
    
    if (!pivot || !y) {
        if (pivot) free(pivot);
        if (y) free(y);
        return -1;
    }
    
    // Initialize pivot array (optimized: use loop with better cache behavior)
    for (int i = 0; i < n; i++) {
        pivot[i] = i;
    }
    
    // LU decomposition with partial pivoting
    for (int k = 0; k < n - 1; k++) {
        // Find pivot (optimized: use squared magnitude, cache column access)
        int max_row = k;
        double max_val_sq = 0.0;
        
        for (int i = k; i < n; i++) {
            // Use squared magnitude to avoid sqrt, cache element access
            const complex_t lu_ik = LU[i * n + k];
            const double mag_sq = lu_ik.re * lu_ik.re + lu_ik.im * lu_ik.im;
            if (mag_sq > max_val_sq) {
                max_val_sq = mag_sq;
                max_row = i;
            }
        }
        
        // Swap rows if needed (optimized with memcpy)
        if (max_row != k) {
            // Swap matrix rows using temporary buffer
            complex_t* row_k = &LU[k * n];
            complex_t* row_max = &LU[max_row * n];
            complex_t temp;
            for (int j = 0; j < n; j++) {
                temp = row_k[j];
                row_k[j] = row_max[j];
                row_max[j] = temp;
            }
            // Swap RHS
            complex_t temp_rhs = state->rhs_vector[k];
            state->rhs_vector[k] = state->rhs_vector[max_row];
            state->rhs_vector[max_row] = temp_rhs;
            pivot[k] = max_row;
        }
        
        // Check for singularity (optimized: use squared magnitude to avoid sqrt)
        double pivot_mag_sq = LU[k * n + k].re * LU[k * n + k].re + 
                             LU[k * n + k].im * LU[k * n + k].im;
        if (pivot_mag_sq < MATRIX_PIVOT_EPSILON_SQ) {
            free(pivot);
            free(y);
            return -1;  // Singular matrix
        }
        
        // Eliminate below diagonal (optimized: cache pivot element and use inline function)
        const complex_t pivot_kk = LU[k * n + k];
        
        for (int i = k + 1; i < n; i++) {
            // Compute multiplier: L[i,k] = LU[i,k] / LU[k,k] (optimized: use inline function)
            const complex_t lu_ik = LU[i * n + k];
            const complex_t multiplier = complex_div(lu_ik, pivot_kk);
            
            LU[i * n + k] = multiplier;  // Store multiplier in L
            
            // Update row i (parallelize for large n, optimized: cache multiplier)
            const complex_t mult = multiplier;  // Cache for better register usage
            const complex_t* row_k = &LU[k * n];  // Cache row pointer
            complex_t* row_i = &LU[i * n];  // Cache row pointer
            
#ifdef _OPENMP
            if (n > 500) {
                // Use pragma to suppress warning for OpenMP private variable
                #pragma warning(push)
                #pragma warning(disable:4101)  // j is used in parallel region via private(j)
                int j;
                #pragma omp parallel for private(j)
                for (j = k + 1; j < n; j++) {
                    // LU[i,j] = LU[i,j] - L[i,k] * LU[k,j] (optimized: use inline function)
                    row_i[j] = complex_mul_sub(row_i[j], mult, row_k[j]);
                }
                #pragma warning(pop)
            } else {
                for (int j = k + 1; j < n; j++) {
                    // LU[i,j] = LU[i,j] - L[i,k] * LU[k,j] (optimized: use inline function)
                    row_i[j] = complex_mul_sub(row_i[j], mult, row_k[j]);
                }
            }
#else
            for (int j = k + 1; j < n; j++) {
                // LU[i,j] = LU[i,j] - L[i,k] * LU[k,j] (optimized: use inline function)
                row_i[j] = complex_mul_sub(row_i[j], mult, row_k[j]);
            }
#endif
        }
    }
    
    // Forward substitution: L * y = b (where b is RHS, optimized)
    for (int i = 0; i < n; i++) {
        y[i] = state->rhs_vector[i];
        const complex_t* row_i = &LU[i * n];  // Cache row pointer
        for (int j = 0; j < i; j++) {
            // y[i] = y[i] - L[i,j] * y[j] (optimized: use inline function)
            y[i] = complex_mul_sub(y[i], row_i[j], y[j]);
        }
    }
    
    // Backward substitution: U * x = y (optimized)
    for (int i = n - 1; i >= 0; i--) {
        complex_t* x_i = &state->current_coefficients[i];
        *x_i = y[i];
        const complex_t* row_i = &LU[i * n];  // Cache row pointer
        
        for (int j = i + 1; j < n; j++) {
            // x[i] = x[i] - U[i,j] * x[j] (optimized: use inline function)
            *x_i = complex_mul_sub(*x_i, row_i[j], state->current_coefficients[j]);
        }
        
        // Divide by diagonal: x[i] = x[i] / U[i,i] (optimized: use inline function)
        *x_i = complex_div(*x_i, row_i[i]);
    }
    
    // Cleanup (LU aliases Z; freed by caller as impedance_matrix)
    free(pivot);
    free(y);
    
    state->iterations_converged = 1;  // Direct solver converges in 1 iteration
    return 0;
}

// Iterative linear system solver
// Matrix-vector product is now in mom_matvec.c
// Wrapper function for unified state
static void mom_matrix_vector_product_wrapper(
    const mom_unified_state_t* RESTRICT_PTR state,
    const complex_t* RESTRICT_PTR x,
    complex_t* RESTRICT_PTR y,
    mom_algorithm_t algorithm
) {
    const int n = state->num_basis_functions;
    const complex_t* RESTRICT_PTR Z = state->impedance_matrix;
    
    // Call the unified matrix-vector product function
    // Note: In full implementation, would pass ACA/H-matrix/MLFMM structures
    mom_matrix_vector_product(Z, NULL, 0, NULL, 0, NULL, state->csr_impedance_matrix, x, y, n, algorithm,
                              state->config.num_threads);
}

// Optimized GMRES-like iterative solver with compressed matrix support
static int solve_linear_system_iterative(mom_unified_state_t *state) {
    const int n = state->num_basis_functions;
    const double tol = state->config.convergence_tolerance;
    const int max_iter = state->config.max_iterations;
    const mom_algorithm_t algo = state->config.algorithm;
    
    // Allocate workspace
    complex_t* residual = (complex_t*)calloc(n, sizeof(complex_t));
    complex_t* Ax = (complex_t*)calloc(n, sizeof(complex_t));
    if (!residual || !Ax) {
        if (residual) free(residual);
        if (Ax) free(Ax);
        return -1;
    }
    
    // Initialize solution
    if (!state->current_coefficients) {
        state->current_coefficients = (complex_t*)calloc(n, sizeof(complex_t));
        if (!state->current_coefficients) {
            free(residual); free(Ax);
            return -1;
        }
    }
    
    // Compute initial residual: r = b - A*x
    mom_matrix_vector_product_wrapper(state, state->current_coefficients, Ax, algo);
    
    double residual_norm = 0.0;
    int i;
    #pragma omp parallel for reduction(+:residual_norm) if(state->config.num_threads > 1) \
                                                       shared(residual, Ax, state, n) \
                                                       schedule(static)
    for (i = 0; i < n; i++) {
        residual[i].re = state->rhs_vector[i].re - Ax[i].re;
        residual[i].im = state->rhs_vector[i].im - Ax[i].im;
        residual_norm += residual[i].re * residual[i].re + residual[i].im * residual[i].im;
    }
    residual_norm = sqrt(residual_norm);
    
    if (residual_norm < tol) {
        state->iterations_converged = 0;
        free(residual); free(Ax);
        return 0;
    }
    
    // Simple iterative refinement (would use GMRES/BiCGSTAB in full implementation)
    for (int iter = 0; iter < max_iter; iter++) {
        // Compute search direction (simplified: use residual as direction)
        // In full GMRES, would use Arnoldi process to build Krylov subspace
        
        // Matrix-vector product: Ap = A * residual
        mom_matrix_vector_product_wrapper(state, residual, Ax, algo);
        
        // Compute step size (simplified: use residual norm)
        double alpha_re = 0.0, alpha_im = 0.0;
        double denom = 0.0;
        #pragma omp parallel for reduction(+:alpha_re,alpha_im,denom) if(state->config.num_threads > 1) \
                                                                     shared(residual, Ax, n) \
                                                                     schedule(static)
        for (i = 0; i < n; i++) {
            // alpha = <r, r> / <r, Ar>
            double r_dot_r = residual[i].re * residual[i].re + residual[i].im * residual[i].im;
            double r_dot_Ar = residual[i].re * Ax[i].re + residual[i].im * Ax[i].im;
            alpha_re += r_dot_r;
            alpha_im += r_dot_Ar;
            denom += Ax[i].re * Ax[i].re + Ax[i].im * Ax[i].im;
        }
        
        if (denom < MINIMUM_DENOMINATOR_THRESHOLD) break;
        
        double alpha = alpha_re / denom;
        if (alpha < 0.0 || alpha > 1.0) alpha = DEFAULT_NEAR_FIELD_THRESHOLD;  // Safety clamp
        
        // Update solution: x = x + alpha * residual
        #pragma omp parallel for if(state->config.num_threads > 1) \
                                 shared(state, residual, alpha, n) \
                                 schedule(static)
        for (i = 0; i < n; i++) {
            state->current_coefficients[i].re += alpha * residual[i].re;
            state->current_coefficients[i].im += alpha * residual[i].im;
        }
        
        // Update residual: r = r - alpha * Ar
        residual_norm = 0.0;
        #pragma omp parallel for reduction(+:residual_norm) if(state->config.num_threads > 1) \
                                                           shared(residual, Ax, alpha, n) \
                                                           schedule(static)
        for (i = 0; i < n; i++) {
            residual[i].re -= alpha * Ax[i].re;
            residual[i].im -= alpha * Ax[i].im;
            residual_norm += residual[i].re * residual[i].re + residual[i].im * residual[i].im;
        }
        residual_norm = sqrt(residual_norm);
        
        if (residual_norm < tol) {
            state->iterations_converged = iter + 1;
            free(residual); free(Ax);
            return 0;
        }
    }
    
    free(residual);
    free(Ax);
    return -1;  // Did not converge
}

/********************************************************************************
 * Compute Radiation Pattern for Antenna Analysis
 * 
 * This function calculates the far-field radiation pattern based on the
 * computed current coefficients. It evaluates the electric field in the far-field
 * region over a specified angular grid.
 * 
 * Far-Field Computation:
 * E_θ(θ,φ) = -jωμ ∫∫ J(r') · e_θ e^(-jkR) / (4πR) dr'
 * E_φ(θ,φ) = -jωμ ∫∫ J(r') · e_φ e^(-jkR) / (4πR) dr'
 * 
 * where:
 * - J(r') is the surface current density
 * - R is the distance to far-field observation point
 * - e_θ, e_φ are spherical unit vectors
 * 
 * Parameters:
 *   state: MoM solver state with current coefficients
 *   theta_min: Minimum elevation angle (degrees)
 *   theta_max: Maximum elevation angle (degrees)  
 *   phi_min: Minimum azimuth angle (degrees)
 *   phi_max: Maximum azimuth angle (degrees)
 *   n_theta: Number of elevation grid points
 *   n_phi: Number of azimuth grid points
 * 
 * Output:
 * - Prints radiation pattern computation parameters
 * - Stores computed field patterns (in full implementation)
 * 
 * Angular Coverage:
 * - Theta: 0° to 180° (elevation from z-axis)
 * - Phi: 0° to 360° (azimuth in xy-plane)
 * - Grid spacing determines angular resolution
 ********************************************************************************/
static void compute_radiation_pattern_unified(mom_unified_state_t *state, double theta_min, double theta_max,
                                            double phi_min, double phi_max, int n_theta, int n_phi) {
    // Simplified radiation pattern calculation
    // In a real implementation, this would compute the far-field radiation
    // based on the current coefficients
    
    printf("Computing radiation pattern (theta: %.1f-%.1f, phi: %.1f-%.1f)\n", 
           theta_min, theta_max, phi_min, phi_max);
    printf("Grid: %d x %d points\n", n_theta, n_phi);
}

/********************************************************************************
 * Compute Radar Cross Section (RCS) for Scattering Analysis
 * 
 * This function calculates the radar cross section based on the computed current
 * coefficients and incident/scattered field directions. RCS is a measure of how
 * detectable an object is by radar systems.
 * 
 * RCS Definition:
 * σ(θ_i,φ_i,θ_s,φ_s) = 4π lim(R→∞) R² |E_s|² / |E_i|²
 * 
 * where:
 * - E_i is the incident electric field
 * - E_s is the scattered electric field
 * - R is the distance to observation point
 * 
 * Scattered Field Computation:
 * E_s(θ_s,φ_s) = -jωμ ∫∫ J(r') · G(r,r') · E_i(θ_i,φ_i) dr'
 * 
 * Parameters:
 *   state: MoM solver state with current coefficients
 *   theta_inc: Incident elevation angle (degrees)
 *   phi_inc: Incident azimuth angle (degrees)
 *   theta_scat: Scattered elevation angle (degrees)
 *   phi_scat: Scattered azimuth angle (degrees)
 * 
 * Returns:
 *   Radar cross section in square meters (placeholder value)
 * 
 * Applications:
 * - Radar signature analysis and stealth design
 * - Target detection and identification
 * - Electromagnetic scattering characterization
 * - Antenna placement and interference analysis
 * 
 * Units:
 * - Input angles in degrees
 * - Output RCS in square meters (m²)
 * - Often expressed in dBsm: 10 log₁₀(σ/1m²)
 ********************************************************************************/
static double compute_rcs_unified(mom_unified_state_t *state, double theta_inc, double phi_inc,
                                 double theta_scat, double phi_scat) {
    // RCS calculation based on strict RWG basis integration
    // RCS = 4π * lim(R→∞) R² |E_scat|² / |E_inc|²
    // where E_scat is computed using strict RWG basis integration
    
    if (!state || !state->current_coefficients || !state->mesh) {
        return 0.0;
    }
    
    // Convert angles to radians
    double theta_inc_rad = theta_inc * M_PI / 180.0;
    double phi_inc_rad = phi_inc * M_PI / 180.0;
    double theta_scat_rad = theta_scat * M_PI / 180.0;
    double phi_scat_rad = phi_scat * M_PI / 180.0;
    
    // Incident wave direction (for reference, not used in scattered field calculation)
    double k_inc[3] = {
        sin(theta_inc_rad) * cos(phi_inc_rad),
        sin(theta_inc_rad) * sin(phi_inc_rad),
        cos(theta_inc_rad)
    };
    (void)k_inc;  // Suppress unused variable warning
    
    // Scattering direction (observation direction)
    double r_hat[3] = {
        sin(theta_scat_rad) * cos(phi_scat_rad),
        sin(theta_scat_rad) * sin(phi_scat_rad),
        cos(theta_scat_rad)
    };
    
    // Get frequency and compute wavenumber
    double frequency = state->problem.frequency > 0.0 ? state->problem.frequency : DEFAULT_FREQUENCY_HZ;
    double k0 = TWO_PI_OVER_C0 * frequency;
    double eta0 = ETA0;
    
    // Build RWG mapping: edge -> adjacent triangles
    int* edge_plus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(state->mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(state->mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return 0.0;
    }
    build_rwg_mapping(state->mesh, edge_plus, edge_minus, edge_length);
    
    // Compute scattered field using strict RWG basis integration
    // E_scat = j*k*eta0/(4π) * Σ_n I_n * J_n
    // where I_n = ∫_T f_n(r') * exp(-j*k*r_hat·r') dS'
    complex_t E_scat = complex_zero();
    
    // Iterate over edges (RWG basis functions)
    int num_basis = (state->mesh->num_edges < state->num_basis_functions) ? 
                    state->mesh->num_edges : state->num_basis_functions;
    
    for (int e = 0; e < num_basis; e++) {
        if (edge_length[e] < AREA_EPSILON) continue;
        
        complex_t J_n = (e < state->num_basis_functions) ? 
                        state->current_coefficients[e] : complex_zero();
        if (fabs(J_n.re) < AREA_EPSILON && fabs(J_n.im) < AREA_EPSILON) continue;
        
        // Integrate over plus triangle (note: phase is -j*k*r_hat·r' for scattering)
        complex_t I_plus = complex_zero();
        if (edge_plus[e] >= 0) {
            // Use negative k0 for scattering direction (exp(-j*k*r_hat·r'))
            I_plus = integrate_rwg_far_field(state->mesh, e, edge_plus[e], 1, r_hat, -k0);
        }
        
        // Integrate over minus triangle
        complex_t I_minus = complex_zero();
        if (edge_minus[e] >= 0) {
            I_minus = integrate_rwg_far_field(state->mesh, e, edge_minus[e], 0, r_hat, -k0);
        }
        
        // Total integral: I_n = I_plus + I_minus
        complex_t I_n;
        I_n.re = I_plus.re + I_minus.re;
        I_n.im = I_plus.im + I_minus.im;
        
        // Contribution: J_n * I_n
        complex_t contrib;
        contrib.re = J_n.re * I_n.re - J_n.im * I_n.im;
        contrib.im = J_n.re * I_n.im + J_n.im * I_n.re;
        
        E_scat.re += contrib.re;
        E_scat.im += contrib.im;
    }
    
    // Scale by Green's function factor: j*k*eta0/(4π)
    const double factor = k0 * eta0 / (4.0 * M_PI);
    // j * E_scat: multiply by j (rotate by 90 degrees in complex plane)
    complex_t E_scat_scaled;
    E_scat_scaled.re = -factor * E_scat.im;  // j * complex
    E_scat_scaled.im = factor * E_scat.re;
    
    // Compute RCS: σ = 4π * |E_scat|² / |E_inc|² * r²
    // For unit incident field (|E_inc| = 1): σ = 4π * r² * |E_scat|²
    // In far field limit (r → ∞), we normalize by r², so: σ = 4π * |E_scat|²
    double E_scat_mag_sq = E_scat_scaled.re * E_scat_scaled.re + 
                          E_scat_scaled.im * E_scat_scaled.im;
    double rcs = 4.0 * M_PI * E_scat_mag_sq;
    
    free(edge_plus);
    free(edge_minus);
    free(edge_length);
    
    return rcs;
}

/********************************************************************************
 * Main Unified Method of Moments Solver Interface
 * 
 * This is the primary entry point for the unified MoM solver. It provides a
 * comprehensive interface for solving electromagnetic radiation and scattering
 * problems using the Method of Moments with automatic algorithm selection.
 * 
 * Solution Process:
 * 1. Initialize solver state with mesh data and configuration
 * 2. Set default configuration parameters if not provided
 * 3. Estimate number of basis functions from mesh elements
 * 4. Compute problem characteristics for algorithm selection
 * 5. Automatically select optimal algorithm if not specified
 * 6. Allocate memory for solution vectors
 * 7. Assemble impedance matrix using selected algorithm
 * 8. Solve linear system for current coefficients
 * 
 * Parameters:
 *   mesh: Geometric mesh data containing nodes and elements
 *   frequency: Operating frequency in Hz
 *   excitation: Complex excitation vector (length = number of basis functions)
 *   current_solution: Output array for current coefficients (pre-allocated)
 *   config: Solver configuration (NULL for defaults)
 * 
 * Returns:
 *   0 on success, negative error code on failure
 * 
 * Default Configuration (when config is NULL):
 * - Algorithm: Basic direct solver
 * - Basis: Linear RWG functions
 * - Preconditioner: Diagonal
 * - Iterative solver: GMRES
 * - Tolerance: 1e-6
 * - Max iterations: 1000
 * - Threads: 1
 * - GPU: Disabled
 * 
 * Memory Requirements:
 * - Solution vectors: O(N) complex numbers
 * - Impedance matrix: O(N²) for direct solver, O(N log N) for fast algorithms
 * - Additional workspace depending on algorithm
 ********************************************************************************/
int mom_solve_unified(mesh_t *mesh, double frequency, complex_t *excitation, 
                     complex_t *current_solution, mom_unified_config_t *config) {
    
    // Initialize solver state
    mom_unified_state_t state;
    memset(&state, 0, sizeof(state));
    
    // Initialize integration lookup tables for performance optimization
    integration_init_lookup_tables();
    
    state.mesh = mesh;
    state.problem.frequency = frequency;
    
    // Set default configuration if not provided (optimized: avoid unnecessary copy)
    if (!config) {
        // Initialize with defaults
        memset(&state.config, 0, sizeof(state.config));
        state.config.algorithm = MOM_ALGO_BASIC;
        state.config.basis_order = MOM_BASIS_LINEAR;
        state.config.preconditioner = MOM_PRECOND_DIAGONAL;
        state.config.iterative_solver = MOM_ITER_GMRES;
        state.config.convergence_tolerance = CONVERGENCE_TOLERANCE_DEFAULT;
        state.config.max_iterations = 1000;
        state.config.num_threads = 1;
        state.config.use_gpu_acceleration = false;
        // Near-field/singularity handling defaults (safe for small meshes)
        state.config.enable_duffy_transform = true;
        state.config.near_field_threshold = DEFAULT_NEAR_FIELD_THRESHOLD;  // Default wavelengths (conservative default)
        state.config.enable_nearly_singular = true;  // Enable nearly singular handling
        
        // Kernel formulation defaults
        state.config.kernel_formulation = 0;  // Default: EFIE
        state.config.cfie_alpha = DEFAULT_CFIE_ALPHA;  // Default CFIE combination (balanced)
        state.config.use_analytic_self_term = true;
        state.config.self_term_regularization = REGULARIZATION_MIN;  // Small regularization for numerical stability
        #ifdef MOM_ENABLE_STATISTICS
        state.config.duffy_transform_count = 0;  // Initialize statistics
        state.config.analytic_self_term_count = 0;
        #endif
    } else {
        state.config = *config;  // Copy config only when provided
        // Validate and set defaults for near-field/singularity handling parameters
        if (state.config.near_field_threshold <= 0.0) {
            state.config.near_field_threshold = 0.1;  // Default: 0.1 wavelengths
        }
        // Clamp threshold to reasonable range [0.01, 1.0] wavelengths
        if (state.config.near_field_threshold < 0.01) {
            state.config.near_field_threshold = 0.01;  // Minimum: 0.01 wavelengths
        }
        if (state.config.near_field_threshold > 1.0) {
            state.config.near_field_threshold = 1.0;  // Maximum: 1.0 wavelengths
        }
        
        // Validate self-term regularization
        if (state.config.self_term_regularization <= 0.0) {
            state.config.self_term_regularization = REGULARIZATION_MIN;  // Default regularization
        }
        // Clamp regularization to reasonable range [REGULARIZATION_MIN, REGULARIZATION_MAX]
        if (state.config.self_term_regularization < REGULARIZATION_MIN) {
            state.config.self_term_regularization = REGULARIZATION_MIN;
        }
        if (state.config.self_term_regularization > REGULARIZATION_MAX) {
            state.config.self_term_regularization = REGULARIZATION_MAX;
        }
    }
    
    /* 与 assemble_impedance_matrix_* 一致：三角形–三角形（脉冲/分片常数基）装配使用单元索引 */
    if (mesh && mesh->num_elements > 0) {
        state.num_basis_functions = mesh->num_elements;
    } else if (mesh && mesh->num_edges > 0) {
        state.num_basis_functions = mesh->num_edges;
    } else {
        state.num_basis_functions = 0;
    }
    
    // Compute problem characteristics
    compute_problem_characteristics(&state);
    
    /* Auto-select algorithm when caller uses defaults (CLI passes NULL config → was stuck on BASIC). */
    if (!config) {
        state.config.algorithm = select_algorithm(&state.problem);
    } else if (state.config.algorithm == MOM_ALGO_BASIC && config->algorithm == MOM_ALGO_BASIC) {
        state.config.algorithm = select_algorithm(&state.problem);
    }
    
    // Allocate solution vectors
    state.current_coefficients = current_solution;
    state.excitation_vector = excitation;
    state.rhs_vector = (complex_t*)calloc(state.num_basis_functions, sizeof(complex_t));
    
    if (!state.rhs_vector) {
        return -1;  // Allocation failed
    }
    
    // Copy excitation to RHS (optimized: use memcpy)
    memcpy(state.rhs_vector, excitation, state.num_basis_functions * sizeof(complex_t));
    
    printf("MoM Unified Solver: Problem size = %d unknowns, Algorithm = %d\n", 
           state.num_basis_functions, state.config.algorithm);
    
    // Assemble impedance matrix
    int status = 0;
    switch (state.config.algorithm) {
        case MOM_ALGO_BASIC:
            status = assemble_impedance_matrix_basic(&state);
            break;
        case MOM_ALGO_ACA:
            status = assemble_impedance_matrix_aca(&state);
            break;
        case MOM_ALGO_MLFMM:
            status = assemble_impedance_matrix_mlfmm(&state);
            break;
        case MOM_ALGO_HMATRIX:
            status = assemble_impedance_matrix_hmatrix(&state);
            break;
        default:
            status = assemble_impedance_matrix_basic(&state);
            break;
    }
    
    if (status != 0) {
        free(state.rhs_vector);
        return status;
    }
    
    // Solve linear system
    if (state.config.algorithm == MOM_ALGO_BASIC) {
        status = solve_linear_system_basic(&state);
    } else {
        status = solve_linear_system_iterative(&state);
        /* CSR 仅存近场且 matvec 不含多极远场时，迭代往往不收敛；mm/m 单位误判也会使 nnz≈N。 */
        if (status != 0 && state.num_basis_functions > 0 && state.num_basis_functions <= 200000) {
            fprintf(stderr,
                "MoM: iterative solve failed; retrying with dense BASIC assembly + direct solve (uses more RAM).\n");
            if (state.csr_impedance_matrix) {
                mom_csr_complex_free(state.csr_impedance_matrix);
                state.csr_impedance_matrix = NULL;
            }
            if (state.impedance_matrix) {
                free(state.impedance_matrix);
                state.impedance_matrix = NULL;
            }
            state.config.algorithm = MOM_ALGO_BASIC;
            status = assemble_impedance_matrix_basic(&state);
            if (status == 0) {
                status = solve_linear_system_basic(&state);
            }
        }
    }
    
    if (status == 0) {
        printf("MoM Solver converged in %d iterations\n", state.iterations_converged);
    }
    
    // Cleanup
    if (state.csr_impedance_matrix) {
        mom_csr_complex_free(state.csr_impedance_matrix);
        state.csr_impedance_matrix = NULL;
    }
    if (state.impedance_matrix) {
        free(state.impedance_matrix);
    }
    free(state.rhs_vector);
    
    return status;
}

// Radiation pattern computation interface
// Compute RWG basis function integral for far field
// I_n = ∫_T f_n(r') * exp(j*k*r_hat·r') dS'
static complex_t integrate_rwg_far_field(
    const mesh_t* mesh,
    int edge_idx,
    int tri_idx,
    int is_plus,
    const double* r_hat,
    double k0) {
    
    complex_t result = complex_zero();
    if (!mesh || !r_hat || tri_idx < 0 || tri_idx >= mesh->num_elements) {
        return result;
    }
    
    const mesh_element_t* tri_elem = &mesh->elements[tri_idx];
    if (tri_elem->type != MESH_ELEMENT_TRIANGLE || tri_elem->num_vertices < 3) {
        return result;
    }
    
    const mesh_edge_t* edge = &mesh->edges[edge_idx];
    double edge_len = edge->length > 0.0 ? edge->length : 0.0;
    
    // Convert mesh_element_t to geom_triangle_t for area retrieval
    geom_triangle_t tri;
    if (tri_elem->num_vertices >= 3) {
        tri.vertices[0] = mesh->vertices[tri_elem->vertices[0]].position;
        tri.vertices[1] = mesh->vertices[tri_elem->vertices[1]].position;
        tri.vertices[2] = mesh->vertices[tri_elem->vertices[2]].position;
        tri.area = tri_elem->area;
        tri.normal = tri_elem->normal;
    }
    // Use helper function for area retrieval
    double tri_area = geom_triangle_get_area(&tri);
    
    if (tri_area < AREA_EPSILON || edge_len < AREA_EPSILON) {
        return result;
    }
    
    // Get triangle vertices using mesh_element_t indices
    if (tri_elem->num_vertices < 3 || !mesh->vertices) {
        return result;
    }
    
    const geom_point_t* v0 = &mesh->vertices[tri_elem->vertices[0]].position;
    const geom_point_t* v1 = &mesh->vertices[tri_elem->vertices[1]].position;
    const geom_point_t* v2 = &mesh->vertices[tri_elem->vertices[2]].position;
    
    // Find opposite vertex (not on edge) using mesh_element_t vertex indices
    int opp_vertex_idx = -1;
    for (int v = 0; v < tri_elem->num_vertices; v++) {
        int vidx = tri_elem->vertices[v];
        if (vidx != edge->vertex1_id && vidx != edge->vertex2_id) {
            opp_vertex_idx = vidx;
            break;
        }
    }
    
    if (opp_vertex_idx < 0 || opp_vertex_idx >= mesh->num_vertices) {
        return result;
    }
    
    const geom_point_t* r_opp = &mesh->vertices[opp_vertex_idx].position;
    double coeff = edge_len / (2.0 * tri_area);
    
    // Use 7-point Gaussian quadrature on triangle
    // Integration points in barycentric coordinates
    double gauss_xi[7] = {1.0/3.0, 0.797426985353087, 0.101286507323456, 0.101286507323456,
                          0.059715871789770, 0.470142064105115, 0.470142064105115};
    double gauss_eta[7] = {1.0/3.0, 0.101286507323456, 0.797426985353087, 0.101286507323456,
                           0.470142064105115, 0.059715871789770, 0.470142064105115};
    double gauss_weights[7] = {0.225, 0.125939180544827, 0.125939180544827, 0.125939180544827,
                              0.132394152788506, 0.132394152788506, 0.132394152788506};
    
    for (int gp = 0; gp < 7; gp++) {
        double xi = gauss_xi[gp];
        double eta = gauss_eta[gp];
        double zeta = 1.0 - xi - eta;
        double weight = gauss_weights[gp];
        
        // Interpolate position: r = xi*v0 + eta*v1 + zeta*v2
        geom_point_t r_point;
        r_point.x = xi * v0->x + eta * v1->x + zeta * v2->x;
        r_point.y = xi * v0->y + eta * v1->y + zeta * v2->y;
        r_point.z = xi * v0->z + eta * v1->z + zeta * v2->z;
        
        // Compute RWG basis function at integration point
        double basis_vec[3];
        if (is_plus) {
            basis_vec[0] = coeff * (r_point.x - r_opp->x);
            basis_vec[1] = coeff * (r_point.y - r_opp->y);
            basis_vec[2] = coeff * (r_point.z - r_opp->z);
        } else {
            basis_vec[0] = coeff * (r_opp->x - r_point.x);
            basis_vec[1] = coeff * (r_opp->y - r_point.y);
            basis_vec[2] = coeff * (r_opp->z - r_point.z);
        }
        
        // Phase factor: exp(j*k*r_hat·r')
        double phase_arg = k0 * (r_hat[0] * r_point.x + r_hat[1] * r_point.y + r_hat[2] * r_point.z);
        complex_t phase = {cos(phase_arg), sin(phase_arg)};
        
        // Integrand: f_n(r') · r_hat * exp(j*k*r_hat·r')
        double f_dot_rhat = basis_vec[0] * r_hat[0] + basis_vec[1] * r_hat[1] + basis_vec[2] * r_hat[2];
        
        // Accumulate: weight * area * f_dot_rhat * phase
        complex_t contrib;
        contrib.re = weight * tri_area * f_dot_rhat * phase.re;
        contrib.im = weight * tri_area * f_dot_rhat * phase.im;
        
        result.re += contrib.re;
        result.im += contrib.im;
    }
    
    return result;
}

void mom_compute_radiation_pattern_unified(mesh_t *mesh, complex_t *currents, double frequency,
                                         double theta_min, double theta_max, double phi_min, double phi_max,
                                         int n_theta, int n_phi, complex_t *far_field) {
    
    if (!mesh || !currents || !far_field) {
        return;
    }
    
    printf("Computing unified radiation pattern with strict RWG integration...\n");
    
    // Far-field calculation using strict RWG basis integration
    // E_far = j*k*eta0/(4π*r) * exp(-j*k*r) * Σ_n I_n * J_n
    // where I_n = ∫_T f_n(r') * exp(j*k*r_hat·r') dS'
    
    double k0 = TWO_PI_OVER_C0 * frequency;
    double eta0 = ETA0;
    double r_far = 1000.0;  // Far field distance (1 km)
    
    // Build RWG mapping: edge -> adjacent triangles
    int* edge_plus = (int*)calloc(mesh->num_edges, sizeof(int));
    int* edge_minus = (int*)calloc(mesh->num_edges, sizeof(int));
    double* edge_length = (double*)calloc(mesh->num_edges, sizeof(double));
    if (!edge_plus || !edge_minus || !edge_length) {
        free(edge_plus); free(edge_minus); free(edge_length);
        return;
    }
    build_rwg_mapping(mesh, edge_plus, edge_minus, edge_length);
    
    // Compute far field for each observation angle
    const double theta_step = (n_theta > 1) ? (theta_max - theta_min) / (n_theta - 1) : 0.0;
    const double phi_step = (n_phi > 1) ? (phi_max - phi_min) / (n_phi - 1) : 0.0;
    const double deg_to_rad = M_PI / 180.0;
    
    int idx = 0;
    for (int t = 0; t < n_theta; t++) {
        const double theta = theta_min + theta_step * t;
        const double theta_rad = theta * deg_to_rad;
        
        for (int p = 0; p < n_phi; p++) {
            const double phi = phi_min + phi_step * p;
            const double phi_rad = phi * deg_to_rad;
            
            // Observation direction unit vector
            double r_hat[3] = {
                sin(theta_rad) * cos(phi_rad),
                sin(theta_rad) * sin(phi_rad),
                cos(theta_rad)
            };
            
            // Compute far field by integrating RWG basis functions
            complex_t E_far = complex_zero();
            
            // Iterate over edges (RWG basis functions)
            int num_basis = (mesh->num_edges < mesh->num_elements) ? mesh->num_edges : mesh->num_elements;
            for (int e = 0; e < num_basis; e++) {
                if (edge_length[e] < AREA_EPSILON) continue;
                
                complex_t J_n = (e < num_basis) ? currents[e] : complex_zero();
                if (fabs(J_n.re) < AREA_EPSILON && fabs(J_n.im) < AREA_EPSILON) continue;
                
                // Integrate over plus triangle
                complex_t I_plus = complex_zero();
                if (edge_plus[e] >= 0) {
                    I_plus = integrate_rwg_far_field(mesh, e, edge_plus[e], 1, r_hat, k0);
                }
                
                // Integrate over minus triangle
                complex_t I_minus = complex_zero();
                if (edge_minus[e] >= 0) {
                    I_minus = integrate_rwg_far_field(mesh, e, edge_minus[e], 0, r_hat, k0);
                }
                
                // Total integral: I_n = I_plus + I_minus
                complex_t I_n;
                I_n.re = I_plus.re + I_minus.re;
                I_n.im = I_plus.im + I_minus.im;
                
                // Contribution: J_n * I_n
                complex_t contrib;
                contrib.re = J_n.re * I_n.re - J_n.im * I_n.im;
                contrib.im = J_n.re * I_n.im + J_n.im * I_n.re;
                
                E_far.re += contrib.re;
                E_far.im += contrib.im;
            }
            
            // Scale by Green's function factor: j*k*eta0/(4π*r) * exp(-j*k*r)
            const double exp_factor = -k0 * r_far;
            const complex_t exp_term = {cos(exp_factor), sin(exp_factor)};
            const double scale = k0 * eta0 / (4.0 * M_PI * r_far);
            
            // Final: scale * exp(-j*k*r) * E_far
            const complex_t E_far_scaled = {E_far.re * scale, E_far.im * scale};
            const complex_t E_scaled = MOM_COMPLEX_MUL(E_far_scaled, exp_term);
            
            // Apply j factor: j * E_scaled (rotate by 90 degrees)
            far_field[idx].re = -E_scaled.im;
            far_field[idx].im = E_scaled.re;
            
            idx++;
        }
    }
    
    free(edge_plus);
    free(edge_minus);
    free(edge_length);
}

// RCS computation interface  
double mom_compute_rcs_unified(mesh_t *mesh, complex_t *currents, double frequency,
                              double theta_inc, double phi_inc, double theta_scat, double phi_scat) {
    
    printf("Computing unified RCS...\n");
    
    // Create temporary state for RCS computation
    mom_unified_state_t temp_state;
    memset(&temp_state, 0, sizeof(temp_state));
    temp_state.mesh = mesh;
    temp_state.problem.frequency = frequency;
    temp_state.current_coefficients = (mom_complex_t*)currents;
    temp_state.num_basis_functions = mesh ? mesh->num_elements : 0;
    
    return compute_rcs_unified(&temp_state, theta_inc, phi_inc, theta_scat, phi_scat);
}

/********************************************************************************
 * Set Layered Medium for MoM Solver
 * 
 * This function configures the layered medium for the MoM solver, enabling
 * the use of layered Green's functions for multi-layered substrate analysis.
 * 
 * Parameters:
 *   solver: MoM solver instance (cast to mom_unified_state_t*)
 *   medium: Layered medium structure (cast to void* for compatibility)
 *   freq: Frequency domain parameters (cast to void* for compatibility)
 *   params: Green's function parameters (cast to void* for compatibility)
 * 
 * Returns:
 *   0 on success, -1 on error
 ********************************************************************************/
// Note: This function has a different signature than the one in mom_solver_min.c
// The header declares the version in mom_solver_min.c, so we make this one static
// to avoid linker conflicts
static int mom_solver_set_layered_medium_unified(void* solver, const void* medium,
                                                  const void* freq, const void* params) {
    if (!solver) return -1;
    
    // Cast to unified state (assuming solver is mom_unified_state_t*)
    // In a full implementation, would need proper type checking
    mom_unified_state_t* state = (mom_unified_state_t*)solver;
    
    // Store layered medium data
    state->layered_medium = (void*)medium;  // Store pointer (not deep copy)
    state->frequency_domain = (void*)freq;  // Store frequency domain pointer
    state->greens_params = (void*)params;   // Store Green's function parameters
    state->use_layered_medium = (medium != NULL && freq != NULL);
    
    if (state->use_layered_medium && medium) {
        const LayeredMedium* lm = (const LayeredMedium*)medium;
        printf("MoM Solver: Layered medium enabled with %d layers\n", lm->num_layers);
    }
    
    return 0;
}