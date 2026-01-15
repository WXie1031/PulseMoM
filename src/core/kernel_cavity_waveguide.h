/********************************************************************************
 * Cavity and Waveguide Kernels for MoM
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * Implements Green's functions for cavity and waveguide problems
 * Supports rectangular and circular geometries
 ********************************************************************************/

#ifndef KERNEL_CAVITY_WAVEGUIDE_H
#define KERNEL_CAVITY_WAVEGUIDE_H

#include "core_common.h"
#include "core_geometry.h"

#ifdef __cplusplus
extern "C" {
#endif

// Cavity/Waveguide types
typedef enum {
    CAVITY_TYPE_RECTANGULAR,
    CAVITY_TYPE_CIRCULAR,
    CAVITY_TYPE_CYLINDRICAL
} cavity_type_t;

typedef enum {
    WAVEGUIDE_TYPE_RECTANGULAR,
    WAVEGUIDE_TYPE_CIRCULAR
} waveguide_type_t;

// Mode types
typedef enum {
    MODE_TYPE_TE,   // Transverse Electric
    MODE_TYPE_TM,   // Transverse Magnetic
    MODE_TYPE_TEM,  // Transverse Electromagnetic
    MODE_TYPE_HYBRID
} mode_type_t;

// Rectangular cavity parameters
typedef struct {
    double a, b, c;  // Dimensions (length, width, height)
    double eps_r;    // Relative permittivity
    double mu_r;     // Relative permeability
} rectangular_cavity_params_t;

// Rectangular waveguide parameters
typedef struct {
    double a, b;     // Dimensions (width, height)
    double eps_r;    // Relative permittivity
    double mu_r;     // Relative permeability
} rectangular_waveguide_params_t;

// Circular cavity parameters
typedef struct {
    double radius;   // Cavity radius
    double height;   // Cavity height (for cylindrical cavity)
    double eps_r;    // Relative permittivity
    double mu_r;     // Relative permeability
} circular_cavity_params_t;

// Circular waveguide parameters
typedef struct {
    double radius;   // Waveguide radius
    double eps_r;    // Relative permittivity
    double mu_r;     // Relative permeability
} circular_waveguide_params_t;

/**
 * @brief Compute rectangular cavity Green's function
 * 
 * @param r Observation point
 * @param r_prime Source point
 * @param params Cavity parameters
 * @param m, n, p Mode indices
 * @param k Wavenumber
 * @return Complex Green's function value
 */
complex_t kernel_cavity_rectangular_green(const geom_point_t* r,
                                         const geom_point_t* r_prime,
                                         const rectangular_cavity_params_t* params,
                                         int m, int n, int p,
                                         double k);

/**
 * @brief Compute rectangular waveguide Green's function
 * 
 * @param r Observation point
 * @param r_prime Source point
 * @param params Waveguide parameters
 * @param m, n Mode indices
 * @param mode_type Mode type (TE or TM)
 * @param k Wavenumber
 * @return Complex Green's function value
 */
complex_t kernel_waveguide_rectangular_green(const geom_point_t* r,
                                            const geom_point_t* r_prime,
                                            const rectangular_waveguide_params_t* params,
                                            int m, int n,
                                            mode_type_t mode_type,
                                            double k);

/**
 * @brief Compute cavity resonance frequency
 * 
 * @param params Cavity parameters
 * @param m, n, p Mode indices
 * @return Resonance frequency (Hz)
 */
double kernel_cavity_resonance_frequency(const rectangular_cavity_params_t* params,
                                         int m, int n, int p);

/**
 * @brief Compute waveguide cutoff frequency
 * 
 * @param params Waveguide parameters
 * @param m, n Mode indices
 * @param mode_type Mode type (TE or TM)
 * @return Cutoff frequency (Hz)
 */
double kernel_waveguide_cutoff_frequency(const rectangular_waveguide_params_t* params,
                                       int m, int n,
                                       mode_type_t mode_type);

/**
 * @brief Compute cavity mode expansion coefficient
 * 
 * @param params Cavity parameters
 * @param m, n, p Mode indices
 * @param k Wavenumber
 * @return Mode coefficient
 */
complex_t kernel_cavity_mode_coefficient(const rectangular_cavity_params_t* params,
                                        int m, int n, int p,
                                        double k);

/**
 * @brief Compute circular cavity Green's function
 * 
 * For circular cavity, the Green's function uses Bessel functions:
 * G(r, r') = Σ_{mnp} J_m(k_mn*ρ/a) * J_m(k_mn*ρ'/a) * 
 *            cos(m(φ-φ')) * sin(pπz/h) * sin(pπz'/h) / (k² - k_mnp²)
 * 
 * @param r Observation point (cylindrical coordinates: ρ, φ, z)
 * @param r_prime Source point
 * @param params Circular cavity parameters
 * @param m, n, p Mode indices (m: azimuthal, n: radial, p: axial)
 * @param k Wavenumber
 * @return Complex Green's function value
 */
complex_t kernel_cavity_circular_green(const geom_point_t* r,
                                       const geom_point_t* r_prime,
                                       const circular_cavity_params_t* params,
                                       int m, int n, int p,
                                       double k);

/**
 * @brief Compute circular waveguide Green's function
 * 
 * For circular waveguide, the Green's function depends on mode type:
 * - TE modes: G_TE = Σ_{mn} J_m(k_mn*ρ/a) * J_m(k_mn*ρ'/a) * 
 *             cos(m(φ-φ')) / (k² - k_c²)
 * - TM modes: G_TM = Σ_{mn} J_m(k'_mn*ρ/a) * J_m(k'_mn*ρ'/a) * 
 *             cos(m(φ-φ')) / (k² - k_c²)
 * 
 * @param r Observation point (cylindrical coordinates: ρ, φ, z)
 * @param r_prime Source point
 * @param params Circular waveguide parameters
 * @param m, n Mode indices (m: azimuthal, n: radial)
 * @param mode_type Mode type (TE or TM)
 * @param k Wavenumber
 * @return Complex Green's function value
 */
complex_t kernel_waveguide_circular_green(const geom_point_t* r,
                                          const geom_point_t* r_prime,
                                          const circular_waveguide_params_t* params,
                                          int m, int n,
                                          mode_type_t mode_type,
                                          double k);

/**
 * @brief Compute circular cavity resonance frequency
 * 
 * f_mnp = (c0 / (2 * sqrt(eps_r * mu_r))) * sqrt((k_mn/a)² + (pπ/h)²)
 * where k_mn is the n-th root of J_m or J'_m
 * 
 * @param params Circular cavity parameters
 * @param m, n, p Mode indices
 * @return Resonance frequency (Hz)
 */
double kernel_cavity_circular_resonance_frequency(const circular_cavity_params_t* params,
                                                  int m, int n, int p);

/**
 * @brief Compute circular waveguide cutoff frequency
 * 
 * f_c = (c0 / (2 * sqrt(eps_r * mu_r))) * k_mn / a
 * where k_mn is the n-th root of J_m (TE) or J'_m (TM)
 * 
 * @param params Circular waveguide parameters
 * @param m, n Mode indices
 * @param mode_type Mode type (TE or TM)
 * @return Cutoff frequency (Hz)
 */
double kernel_waveguide_circular_cutoff_frequency(const circular_waveguide_params_t* params,
                                                  int m, int n,
                                                  mode_type_t mode_type);

/**
 * @brief Compute Bessel function of first kind J_n(x)
 * Uses series expansion for small x, asymptotic for large x
 * 
 * @param n Order
 * @param x Argument
 * @return Bessel function value
 */
double bessel_jn(int n, double x);

/**
 * @brief Compute derivative of Bessel function J'_n(x)
 * 
 * @param n Order
 * @param x Argument
 * @return Bessel function derivative value
 */
double bessel_jn_prime(int n, double x);

/**
 * @brief Find n-th root of J_m(x) = 0 (for TE modes)
 * 
 * @param m Order
 * @param n Root index (1-based)
 * @return Root value
 */
double bessel_jn_root(int m, int n);

/**
 * @brief Find n-th root of J'_m(x) = 0 (for TM modes)
 * 
 * @param m Order
 * @param n Root index (1-based)
 * @return Root value
 */
double bessel_jn_prime_root(int m, int n);

#ifdef __cplusplus
}
#endif

#endif // KERNEL_CAVITY_WAVEGUIDE_H

