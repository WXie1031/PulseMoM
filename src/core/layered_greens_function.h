/********************************************************************************
 * PulseEM - Unified Electromagnetic Simulation Platform
 *
 * Copyright (C) 2024-2025 PulseEM Technologies
 *
 * Commercial License - All Rights Reserved
 * Unauthorized copying, modification, or distribution is strictly prohibited
 * Proprietary and confidential - see LICENSE file for details
 *
 * File: layered_greens_function.h
 * Description: Layered Green's function implementation for multilayered media
 ********************************************************************************/

#ifndef LAYERED_GREENS_FUNCTION_H
#define LAYERED_GREENS_FUNCTION_H

#include <math.h>
#include <stdbool.h>

#if defined(_MSC_VER)
typedef struct { double re; double im; } cdouble;
#define CDOUBLE cdouble

// Complex arithmetic functions for MSVC compatibility
static CDOUBLE make_c(double re, double im) { CDOUBLE z; z.re = re; z.im = im; return z; }
static CDOUBLE cadd(CDOUBLE a, CDOUBLE b) { return make_c(a.re + b.re, a.im + b.im); }
static CDOUBLE csub(CDOUBLE a, CDOUBLE b) { return make_c(a.re - b.re, a.im - b.im); }
static CDOUBLE cmul(CDOUBLE a, CDOUBLE b) { return make_c(a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re); }
static CDOUBLE cdiv(CDOUBLE a, CDOUBLE b) { double d = b.re*b.re + b.im*b.im; return make_c((a.re*b.re + a.im*b.im)/d, (a.im*b.re - a.re*b.im)/d); }
static double cabs_cd(CDOUBLE a) { return sqrt(a.re*a.re + a.im*a.im); }
static CDOUBLE csqrt_cd(CDOUBLE a) {
    double r = cabs_cd(a);
    double t = sqrt((r + a.re) * 0.5);
    double u = sqrt(fmax(0.0, (r - a.re) * 0.5));
    double im = (a.im >= 0 ? u : -u);
    return make_c(t, im);
}
static CDOUBLE cexp_cd(CDOUBLE a) {
    double e = exp(a.re);
    return make_c(e * cos(a.im), e * sin(a.im));
}
#define COMPLEX_I make_c(0.0, 1.0)
#define CABS cabs_cd
#define CSQRT csqrt_cd
#define CEXP cexp_cd
#else
#include <complex.h>
#define CDOUBLE double complex
#define COMPLEX_I I
#define CABS cabs
#define CSQRT csqrt
#define CEXP cexp
#endif

typedef struct {
    int num_layers;
    double *thickness;      // Layer thicknesses [m]
    double *epsilon_r;      // Relative permittivities
    double *mu_r;          // Relative permeabilities
    double *sigma;         // Conductivities [S/m]
    double *tan_delta;     // Loss tangents
} LayeredMedium;

typedef struct {
    double freq;           // Frequency [Hz]
    double omega;          // Angular frequency
    CDOUBLE k0;     // Free-space wavenumber
    CDOUBLE eta0;   // Free-space impedance
} FrequencyDomain;

typedef struct {
    // Sommerfeld integration parameters
    int n_points;          // Number of integration points
    double krho_max;       // Maximum spectral variable
    double *krho_points;   // Spectral variable samples
    double *weights;       // Integration weights
    
    // DCIM parameters
    bool use_dcim;         // Enable DCIM acceleration
    int n_images;          // Number of complex images
    CDOUBLE *amplitudes;  // Image amplitudes
    CDOUBLE *exponents;   // Image exponents
} GreensFunctionParams;

typedef struct {
    double x, y, z;        // Source coordinates
    double xp, yp, zp;     // Observation coordinates
    int layer_src;         // Source layer index
    int layer_obs;         // Observation layer index
} GreensFunctionPoints;

typedef struct {
    CDOUBLE G_ee[3][3];  // Electric-electric dyadic
    CDOUBLE G_em[3][3];  // Electric-magnetic dyadic
    CDOUBLE G_me[3][3];  // Magnetic-electric dyadic
    CDOUBLE G_mm[3][3];  // Magnetic-magnetic dyadic
} GreensFunctionDyadic;

// Main Green's function interface
GreensFunctionDyadic* layered_medium_greens_function(
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    const GreensFunctionPoints *points,
    const GreensFunctionParams *params
);

// Sommerfeld integral evaluation
CDOUBLE sommerfeld_integral(
    double krho,
    double z, double zp,
    int layer_src, int layer_obs,
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    char polarization  // 'TE' or 'TM'
);

// DCIM complex image approximation
void dcim_approximation(
    double z, double zp,
    int layer_src, int layer_obs,
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    CDOUBLE *amplitudes,
    CDOUBLE *exponents,
    int *n_images
);

// Spectral domain reflection coefficients
void calculate_reflection_coefficients(
    double krho,
    int layer_src, int layer_obs,
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    CDOUBLE *R_TE, CDOUBLE *R_TM,
    CDOUBLE *T_TE, CDOUBLE *T_TM
);

// Integration path deformation for convergence
void deform_integration_path(
    double krho_start, double krho_end,
    CDOUBLE *path_points,
    double *path_weights,
    int *n_points
);

// Near-singularity treatment with Duffy transformation
CDOUBLE duffy_transformation(
    double krho,
    double z, double zp,
    int layer_src, int layer_obs,
    const LayeredMedium *medium,
    const FrequencyDomain *freq
);

// Surface wave pole extraction
void extract_surface_wave_poles(
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    CDOUBLE *poles,
    CDOUBLE *residues,
    int *n_poles
);

// Memory management
void free_greens_function_dyadic(GreensFunctionDyadic *gf);
void free_layered_medium(LayeredMedium *medium);
void free_greens_function_params(GreensFunctionParams *params);

#if defined(GREENS_IMPL_TMM)
#include "advanced_layered_greens_function.h"
#endif

#define greens_function_sommerfeld_layered layered_medium_greens_function
#define sommerfeld_integral_TE_TM sommerfeld_integral
#define reflection_coefficients_sommerfeld calculate_reflection_coefficients
#define dcim_vector_fit_simple dcim_approximation

#if defined(GREENS_IMPL_TMM)
#define greens_function_layered tmm_layered_medium_greens_function
#else
#define greens_function_layered greens_function_sommerfeld_layered
#endif

#endif // LAYERED_GREENS_FUNCTION_H
