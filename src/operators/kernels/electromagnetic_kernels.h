/**
 * @file electromagnetic_kernels.h
 * @brief Advanced electromagnetic computational kernels for PEEC, MoM, and BEM
 * @details High-performance implementations of Green's functions, surface integrals, and singular treatments
 */

#ifndef ELECTROMAGNETIC_KERNELS_H
#define ELECTROMAGNETIC_KERNELS_H

#include "../../common/core_common.h"
#include "greens_function.h"  // Include for greens_kernel_type_t
#include <stdbool.h>

// Export macros for DLL visibility
#ifdef _WIN32
  #ifdef BUILDING_ELECTROMAGNETIC_KERNELS_DLL
    #define EM_KERNEL_API __declspec(dllexport)
  #else
    #define EM_KERNEL_API __declspec(dllimport)
  #endif
#else
  #define EM_KERNEL_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct layered_media_t;
// triangle_element_t, rectangle_element_t, wire_element_t are defined below, not forward declared

// Element types for integration
// Note: Kernel types are defined in greens_function.h as greens_kernel_type_t
// This typedef provides backward compatibility
typedef greens_kernel_type_t integral_kernel_t;

// Compatibility macros
#define KERNEL_G GREENS_KERNEL_G
#define KERNEL_GRAD_G GREENS_KERNEL_GRAD_G
#define KERNEL_G_R_R_PRIME GREENS_KERNEL_G_R_R_PRIME
#define KERNEL_DOUBLE_GRAD_G GREENS_KERNEL_DOUBLE_GRAD_G

// Layered media structure
typedef struct layered_media_t {
    complex_t permittivity;    // Complex permittivity
    complex_t permeability;    // Complex permeability  
    complex_t impedance;       // Wave impedance
    double thickness;              // Layer thickness
    double conductivity;           // Conductivity
} layered_media_t;

// Element structures (simplified definitions)
typedef struct {
    double vertices[3][3];         // Triangle vertices
    double normal[3];              // Surface normal
    double area;                    // Triangle area
} triangle_element_t;

typedef struct {
    double vertices[4][3];         // Rectangle vertices
    double normal[3];              // Surface normal
    double area;                    // Rectangle area
} rectangle_element_t;

typedef struct {
    double start[3];               // Wire start point
    double end[3];                 // Wire end point
    double radius;                 // Wire radius
    double length;                 // Wire length
} wire_element_t;

/*********************************************************************
 * Core Green's Function Implementations
 *********************************************************************/

/**
 * @brief Free-space Green's function for electromagnetic problems
 * @param r Distance between source and observation points
 * @param k Wavenumber (2π/λ)
 * @return Complex Green's function value
 */
EM_KERNEL_API complex_t green_function_free_space(double r, double k);

/**
 * @brief Gradient of free-space Green's function
 * @param r Distance
 * @param k Wavenumber
 * @param r_vec Vector from source to observation point
 * @param gradient Output gradient vector (3 components)
 */
EM_KERNEL_API void green_function_gradient_free_space(double r, double k, const double *r_vec, double *gradient);

/**
 * @brief Advanced layered media Green's function with full Sommerfeld integration
 * @param rho Horizontal distance
 * @param z Observation point z-coordinate
 * @param z_prime Source point z-coordinate
 * @param k0 Free-space wavenumber
 * @param n_layers Number of layers
 * @param layers Layer properties array
 * @return Complex Green's function value
 */
EM_KERNEL_API complex_t green_function_layered_media(double rho, double z, double z_prime,
                                           double k0, int n_layers, const layered_media_t *layers);

/**
 * @brief Vector-fitted DCIM for fast layered media evaluation
 * @param error_tolerance Target accuracy (e.g., 0.01 for 1%)
 */
complex_t green_function_layered_dcim(double rho, double z, double z_prime,
                                         double k0, int n_layers, const layered_media_t *layers,
                                         double error_tolerance);

/**
 * @brief Fast Green's function with automatic method selection
 * @details Uses interpolation tables and adaptive accuracy
 */
complex_t green_function_layered_fast(double rho, double z, double z_prime,
                                         double k0, int n_layers, const layered_media_t *layers);

/**
 * @brief Periodic Green's function for array structures
 * @param r Distance
 * @param k Wavenumber
 * @param periodicity Array periodicity vector [2]
 * @param n_harmonics Number of Floquet harmonics to include
 */
complex_t green_function_periodic(double r, double k, const double *periodicity, int n_harmonics);

/*********************************************************************
 * Surface Integral Implementations
 *********************************************************************/

/**
 * @brief Surface integral over triangular element with singularity treatment
 * @details Uses Duffy transformation for singular integrals and adaptive quadrature
 */
complex_t integrate_triangle_singular(const triangle_element_t *triangle,
                                        const double *obs_point, double k,
                                        integral_kernel_t kernel_type);

/**
 * @brief Surface integral over rectangular (Manhattan) element
 * @details Used primarily in PEEC method with Gaussian quadrature
 */
complex_t integrate_rectangle_regular(const rectangle_element_t *rectangle,
                                          const double *obs_point, double k,
                                          integral_kernel_t kernel_type);

/**
 * @brief Wire integral for thin-wire approximations
 * @details Used in both MoM (thin-wire kernel) and PEEC (partial inductance)
 */
complex_t integrate_wire_thin(const wire_element_t *wire,
                                  const double *obs_point, double k,
                                  integral_kernel_t kernel_type);

/*********************************************************************
 * Double Integration Functions (for PEEC)
 *********************************************************************/

/**
 * @brief Double surface integral for partial capacitance (PEEC)
 * @details Computes C_ij = (1/4πε₀) ∫∫ dS_i dS_j / |r_i - r_j|
 */
double integrate_surface_surface_capacitance(
    const triangle_element_t *surf_i,
    const triangle_element_t *surf_j,
    double k);

/**
 * @brief Double surface integral for partial inductance (PEEC)
 * @details Computes L_ij = (μ₀/4π) ∫∫ (J_i · J_j) / |r_i - r_j| dS_i dS_j
 */
complex_t integrate_surface_surface_inductance(
    const triangle_element_t *surf_i,
    const triangle_element_t *surf_j,
    double k,
    int gauss_order);  // Optional: Gauss quadrature order (1, 4, 7, or 8). Default: 4 if <= 0

/**
 * @brief Neumann formula integral for wire-wire partial inductance (PEEC)
 * @details Computes L_ij = (μ₀/4π) ∫∫ (dl_i · dl_j) / |r_i - r_j|
 */
complex_t integrate_wire_wire_neumann(
    const wire_element_t *wire_i,
    const wire_element_t *wire_j,
    double k);

/**
 * @brief Double quadrilateral integral for partial elements (PEEC)
 */
complex_t integrate_quad_quad_double(
    const rectangle_element_t *quad_i,
    const rectangle_element_t *quad_j,
    double k);

/**
 * @brief Point-to-surface integral (for PEEC point-surface coupling)
 * @details Computes ∫ G(r, r') dS' where r is a point and S' is a surface
 */
complex_t integrate_point_surface(
    const double *point,
    const triangle_element_t *surface,
    double k,
    integral_kernel_t kernel_type);

/**
 * @brief Point-to-wire integral (for PEEC point-wire coupling)
 * @details Computes ∫ G(r, r') dl' where r is a point and l' is a wire
 */
complex_t integrate_point_wire(
    const double *point,
    const wire_element_t *wire,
    double k,
    integral_kernel_t kernel_type);

/**
 * @brief Wire-to-surface integral (for PEEC wire-surface coupling)
 * @details Computes ∫∫ G(r, r') dl dS' where l is a wire and S' is a surface
 */
complex_t integrate_wire_surface(
    const wire_element_t *wire,
    const triangle_element_t *surface,
    double k,
    integral_kernel_t kernel_type);

/*********************************************************************
 * MoM (Method of Moments) Specific Integration Functions
 *********************************************************************/

// Forward declaration for RWG basis function
// Note: rwg_basis_t is already defined in core_common.h
// We rely on core_common.h being included before this header
// No need to include it here to avoid redefinition issues

/**
 * @brief RWG basis function double integral for EFIE
 * @details Computes Z_ij^EFIE = jωμ ∫∫ f_i(r) · f_j(r') G(r,r') dS dS'
 *          - (1/jωε) ∫∫ (∇·f_i(r)) (∇'·f_j(r')) G(r,r') dS dS'
 * @param basis_i_ptr Pointer to RWG basis function i
 * @param basis_j_ptr Pointer to RWG basis function j
 */
complex_t integrate_rwg_rwg_efie(
    const triangle_element_t* tri_i_plus,
    const triangle_element_t* tri_i_minus,
    const triangle_element_t* tri_j_plus,
    const triangle_element_t* tri_j_minus,
    const void* basis_i_ptr,
    const void* basis_j_ptr,
    double frequency);

/**
 * @brief Same as integrate_rwg_rwg_efie but with exact opposite-vertex coords and edge lengths (RWG Galerkin).
 */
complex_t integrate_rwg_rwg_efie_explicit(
    const triangle_element_t* tri_i_plus,
    const triangle_element_t* tri_i_minus,
    const triangle_element_t* tri_j_plus,
    const triangle_element_t* tri_j_minus,
    const double opp_i_plus[3],
    const double opp_i_minus[3],
    const double opp_j_plus[3],
    const double opp_j_minus[3],
    double edge_len_i,
    double edge_len_j,
    double frequency);

/**
 * @brief RWG basis function double integral for MFIE
 * @details Computes Z_ij^MFIE = (1/2) ∫ f_i(r) · f_j(r) dS + ...
 */
complex_t integrate_rwg_rwg_mfie(
    const triangle_element_t* tri_i_plus,
    const triangle_element_t* tri_i_minus,
    const triangle_element_t* tri_j_plus,
    const triangle_element_t* tri_j_minus,
    const void* basis_i_ptr,
    const void* basis_j_ptr,
    double frequency);

/**
 * @brief RWG basis function double integral for CFIE
 * @details Computes Z_ij^CFIE = α * Z_ij^EFIE + (1-α) * η * Z_ij^MFIE
 */
complex_t integrate_rwg_rwg_cfie(
    const triangle_element_t* tri_i_plus,
    const triangle_element_t* tri_i_minus,
    const triangle_element_t* tri_j_plus,
    const triangle_element_t* tri_j_minus,
    const void* basis_i_ptr,
    const void* basis_j_ptr,
    double frequency,
    double alpha);

/**
 * @brief Plane wave excitation integral for RWG basis functions
 * @details Computes V_i = ∫ f_i(r) · E_inc(r) dS
 */
complex_t integrate_rwg_plane_wave(
    const triangle_element_t* tri_plus,
    const triangle_element_t* tri_minus,
    const void* basis_ptr,
    const double* E_inc,
    const double* k_vec,
    double frequency);

/**
 * @brief Point source excitation integral for RWG basis functions
 * @details Computes V_i = f_i(r_s) · J_s
 */
complex_t integrate_rwg_point_source(
    const triangle_element_t* tri_plus,
    const triangle_element_t* tri_minus,
    const void* basis_ptr,
    const double* r_source,
    const double* J_source);

/*********************************************************************
 * Singularity Treatment
 *********************************************************************/

/**
 * @brief Singularity extraction for 1/r type singularities
 * @details Extracts singular part: G(r) = G_singular + G_regular
 */
complex_t singularity_extraction_1overr(double r, double k);

/**
 * @brief Edge singularity treatment for triangular elements
 * @details Uses Graglia-Wilton-Peterson (GWP) specialized quadrature
 */
complex_t edge_singularity_treatment(const triangle_element_t *triangle,
                                       const double *obs_point, double k,
                                       int edge_index);

/*********************************************************************
 * Advanced Features
 *********************************************************************/

/**
 * @brief GPU-accelerated batch Green's function evaluation
 * @details Uses OpenMP parallelization (CUDA placeholder for GPU)
 */
void green_function_layered_batch_gpu(const double *rho_array, const double *z_array, const double *z_prime_array,
                                    int n_points, double k0, int n_layers, const layered_media_t *layers,
                                    complex_t *results);

/**
 * @brief Memory-efficient batch processing for large problems
 * @param batch_size Number of points to process per batch
 */
void green_function_layered_batch_memory_efficient(const double *rho_array, const double *z_array, 
                                                 const double *z_prime_array, int n_points, double k0,
                                                 int n_layers, const layered_media_t *layers,
                                                 complex_t *results, int batch_size);

/**
 * @brief Adaptive accuracy control for batch evaluations
 * @param target_accuracy Base accuracy requirement (adapts per point)
 */
void green_function_layered_batch_adaptive(const double *rho_array, const double *z_array, 
                                        const double *z_prime_array, int n_points, double k0,
                                        int n_layers, const layered_media_t *layers,
                                        complex_t *results, double target_accuracy);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

/**
 * @brief Find which layer contains a given z-coordinate
 * @param z Z-coordinate to check
 * @param n_layers Number of layers
 * @param layers Layer properties array
 * @return Layer index (-1 if outside all layers)
 */
int find_layer_containing_point(double z, int n_layers, const layered_media_t *layers);

/**
 * @brief Calculate surface wave poles for layered media
 * @details Uses Muller's method for complex root finding
 */
complex_t* find_surface_wave_poles(double k0, int n_layers, const layered_media_t *layers, int *n_poles_found);

/**
 * @brief Hankel function of second kind (outgoing wave)
 * @param x Argument
 * @return H0^(2)(x)
 */
complex_t hankel_function(double x);

/**
 * @brief Bessel function J0 for complex arguments
 * @param z Complex argument
 * @return J0(z)
 */
complex_t bessel_j0(complex_t z);

/*********************************************************************
 * Internal helper functions (for implementation)
 *********************************************************************/

/**
 * @brief Calculate reflection coefficient for upward propagation
 */
complex_t calculate_reflection_coefficient_upward(int start_layer, int n_layers, 
                                                 const layered_media_t *layers, double k0);

/**
 * @brief Calculate reflection coefficient for downward propagation  
 */
complex_t calculate_reflection_coefficient_downward(int start_layer, int n_layers,
                                                   const layered_media_t *layers, double k0);

/**
 * @brief Calculate surface wave contribution for layered media
 */
complex_t calculate_surface_wave_contribution(double rho, double z, double z_prime, 
                                             double k0, int n_layers, const layered_media_t *layers);

/**
 * @brief Calculate continuous spectrum contribution
 */
complex_t green_function_layered_continuous_spectrum(double rho, double z, double z_prime,
                                                    double k0, int n_layers, const layered_media_t *layers);

/**
 * @brief Spectral domain Green's function evaluation
 */
complex_t spectral_domain_green_function(complex_t k_rho, double z, double z_prime,
                                        double k0, int n_layers, const layered_media_t *layers,
                                        int src_layer, int obs_layer);

#ifdef __cplusplus
}
#endif

#endif // ELECTROMAGNETIC_KERNELS_H
