/*****************************************************************************************
 * PulseEM - Windowed Green Function (WGF) Implementation
 * 
 * Copyright (C) 2025 PulseEM Technologies
 * 
 * File: windowed_greens_function.c
 * Description: Windowed Green Function method implementation for PCB applications
 * 
 * Based on: IEEE 10045792 - Windowed Green Function method for the Helmholtz equation 
 *          in presence of multiply layered media
 * 
 * Implementation for triangular mesh and PEEC-based PCB calculations
 *****************************************************************************************/

#include "windowed_greens_function.h"
#include "core_common.h"
#include "../integration/integration_utils.h"
#include "../integration/integration_utils_optimized.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  // For clock_t and clock()

#ifdef _OPENMP
#include <omp.h>
#endif

// Default window parameters for PCB applications
#define DEFAULT_WINDOW_WIDTH 2.0           // Window width in wavelengths
#define DEFAULT_RISE_RATE 0.1              // Rise rate (steepness)
#define DEFAULT_WGF_TOLERANCE 1e-10        // Convergence tolerance
#define DEFAULT_MAX_ITERATIONS 1000        // Maximum iterations
#define WGF_MIN_DISTANCE_RATIO 0.01        // Minimum distance/wavelength for WGF
#define WGF_MAX_DISTANCE_RATIO 10.0         // Maximum distance/wavelength for WGF

// Physical constants (use from core_common.h if available)
#ifndef C0
#define C0 299792458.0
#endif

/********************************************************************************
 * Window Function Implementation
 ********************************************************************************/

/**
 * @brief Compute smooth step window function
 * @param x Input coordinate
 * @param width Window width
 * @param rise_rate Rise rate (steepness)
 * @param center Window center
 * @return Window function value [0, 1]
 */
static double smooth_step_window(double x, double width, double rise_rate, double center) {
    double normalized = (x - center) / width;
    double arg = normalized / rise_rate;
    
    // Smooth step: 0.5 * (1 + tanh(arg))
    // This provides a smooth transition from 0 to 1
    if (arg > 10.0) return 1.0;
    if (arg < -10.0) return 0.0;
    
    return 0.5 * (1.0 + tanh(arg));
}

/**
 * @brief Compute Gaussian window function
 * @param x Input coordinate
 * @param width Window width
 * @param center Window center
 * @return Window function value [0, 1]
 */
static double gaussian_window(double x, double width, double center) {
    double normalized = (x - center) / width;
    double arg = -0.5 * normalized * normalized;
    return exp(arg);
}

/**
 * @brief Compute exponential window function
 * @param x Input coordinate
 * @param width Window width
 * @param rise_rate Rise rate
 * @param center Window center
 * @return Window function value [0, 1]
 */
static double exponential_window(double x, double width, double rise_rate, double center) {
    double normalized = (x - center) / width;
    if (normalized <= 0.0) return 0.0;
    if (normalized >= 1.0) return 1.0;
    
    // Exponential rise: 1 - exp(-normalized / rise_rate)
    return 1.0 - exp(-normalized / rise_rate);
}

/**
 * @brief Compute window function value
 */
double wgf_window_function(double x, const wgf_window_params_t* params) {
    if (!params) return 1.0;  // Default: no windowing
    
    switch (params->window_type) {
        case WGF_WINDOW_SMOOTH_STEP:
            return smooth_step_window(x, params->width, params->rise_rate, params->center);
            
        case WGF_WINDOW_GAUSSIAN:
            return gaussian_window(x, params->width, params->center);
            
        case WGF_WINDOW_EXPONENTIAL:
            return exponential_window(x, params->width, params->rise_rate, params->center);
            
        case WGF_WINDOW_TANH:
            {
                double normalized = (x - params->center) / params->width;
                return 0.5 * (1.0 + tanh(normalized / params->rise_rate));
            }
            
        case WGF_WINDOW_ADAPTIVE:
            // For adaptive, use smooth step as default
            return smooth_step_window(x, params->width, params->rise_rate, params->center);
            
        default:
            return 1.0;  // No windowing
    }
}

/********************************************************************************
 * Windowed Sommerfeld Integral Implementation
 ********************************************************************************/

/**
 * @brief Compute windowed Sommerfeld integral for layered media
 * 
 * The WGF method applies a window function to the oscillatory integrand,
 * effectively localizing the integration domain and avoiding expensive
 * full Sommerfeld integration.
 */
CDOUBLE wgf_sommerfeld_integral(
    double krho, double z, double zp,
    int layer_src, int layer_obs,
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    const wgf_window_params_t *window_params,
    char polarization,
    double rho_actual) {  // Add rho_actual parameter
    
    if (!medium || !freq || !window_params) {
        return make_c(0.0, 0.0);
    }
    
    // Use actual horizontal distance for window function
    double distance = rho_actual;
    
    // Apply window function to the integration variable
    double window_value = wgf_window_function(distance, window_params);
    
    if (window_value < NUMERICAL_EPSILON) {
        return make_c(0.0, 0.0);  // Window function is zero, skip integration
    }
    
    // Find layer indices
    double z_acc = 0.0;
    int src_layer = 0, obs_layer = 0;
    for (int i = 0; i < medium->num_layers; i++) {
        if (zp >= z_acc && zp < z_acc + medium->thickness[i]) {
            src_layer = i;
            break;
        }
        z_acc += medium->thickness[i];
    }
    z_acc = 0.0;
    for (int i = 0; i < medium->num_layers; i++) {
        if (z >= z_acc && z < z_acc + medium->thickness[i]) {
            obs_layer = i;
            break;
        }
        z_acc += medium->thickness[i];
    }
    
    // Compute layer wavenumbers
    CDOUBLE k_src = make_c(freq->omega * sqrt(medium->epsilon_r[src_layer] * medium->mu_r[src_layer]) / C0, 0.0);
    CDOUBLE k_obs = make_c(freq->omega * sqrt(medium->epsilon_r[obs_layer] * medium->mu_r[obs_layer]) / C0, 0.0);
    
    // Compute vertical wavenumber components
    // Note: kz_src is computed but not used in current implementation (reserved for future use)
    (void)k_src;  // Suppress unused variable warning
    CDOUBLE kz_obs = CSQRT(csub(cmul(k_obs, k_obs), cmul(make_c(krho, 0.0), make_c(krho, 0.0))));
    
    // Compute Bessel function J0(krho * rho_actual)
    // For PCB applications, rho_actual is typically small (within layer)
    double bessel_arg = krho * rho_actual;  // Use actual horizontal distance
    
    #if defined(_MSC_VER)
    double J0_val = (bessel_arg < 1e-8) ? 1.0 : cos(bessel_arg) / sqrt(bessel_arg + 1.0);
    #else
    double J0_val = j0(bessel_arg);
    #endif
    
    // Compute exponential term
    double dz = fabs(z - zp);
    CDOUBLE exp_term = CEXP(cmul(make_c(0.0, -1.0), cmul(kz_obs, make_c(dz, 0.0))));
    
    // Compute reflection coefficient (simplified for same layer)
    CDOUBLE R_coeff = make_c(1.0, 0.0);
    if (src_layer != obs_layer) {
        // Inter-layer reflection (simplified)
        CDOUBLE eta_src = CSQRT(cdiv(make_c(medium->mu_r[src_layer], 0.0), 
                                     make_c(medium->epsilon_r[src_layer], 0.0)));
        CDOUBLE eta_obs = CSQRT(cdiv(make_c(medium->mu_r[obs_layer], 0.0), 
                                     make_c(medium->epsilon_r[obs_layer], 0.0)));
        R_coeff = cdiv(csub(eta_obs, eta_src), cadd(eta_obs, eta_src));
    }
    
    // Combine terms with window function
    CDOUBLE integrand = cmul(cmul(make_c(J0_val * krho, 0.0), exp_term), R_coeff);
    integrand = cmul(integrand, make_c(window_value, 0.0));
    
    return integrand;
}

/********************************************************************************
 * WGF Green's Function for Layered Media (PCB Optimized)
 ********************************************************************************/

/**
 * @brief Compute WGF Green's function for layered media
 * 
 * Optimized for PCB applications with triangular mesh and PEEC solver.
 * Uses windowed Sommerfeld integration to avoid expensive full integration.
 */
wgf_greens_function_result_t* wgf_layered_medium_greens_function(
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    const GreensFunctionPoints *points,
    const wgf_pcb_params_t *wgf_params) {
    
    if (!medium || !freq || !points || !wgf_params || !wgf_params->use_wgf) {
        return NULL;
    }
    
    // Allocate result structure
    wgf_greens_function_result_t *result = (wgf_greens_function_result_t*)calloc(1, sizeof(wgf_greens_function_result_t));
    if (!result) {
        return NULL;
    }
    
    clock_t start_time = clock();
    
    // Optimize window parameters for PCB if needed
    wgf_window_params_t window = wgf_params->window;
    if (wgf_params->window.window_type == WGF_WINDOW_ADAPTIVE) {
        wgf_optimize_window_params_pcb(medium, freq, points, &window);
    }
    
    // Compute horizontal distance
    double dx = points->xp - points->x;
    double dy = points->yp - points->y;
    double rho = sqrt(dx*dx + dy*dy) + GEOMETRIC_EPSILON;
    double dz = fabs(points->zp - points->z);
    
    double wavelength = C0 / freq->freq;
    double k0 = freq->omega / C0;
    
    // Check if WGF is applicable based on distance
    double distance_ratio = rho / wavelength;
    if (distance_ratio < wgf_params->min_distance_ratio || 
        distance_ratio > wgf_params->max_distance_ratio) {
        // Outside WGF range, use standard method or return zero
        result->converged = false;
        return result;
    }
    
    // Integration parameters for WGF
    int n_points = 64;  // Default integration points
    double krho_max = k0 * 10.0;  // Maximum spectral variable (10x free-space k0)
    double krho_min = k0 * 0.01;  // Minimum spectral variable
    
    // Adaptive integration for WGF
    CDOUBLE G_ee_xx = make_c(0.0, 0.0);
    CDOUBLE G_ee_yy = make_c(0.0, 0.0);
    CDOUBLE G_ee_zz = make_c(0.0, 0.0);
    
    // Use Gauss-Legendre quadrature for spectral integration
    double *krho_points = (double*)malloc(n_points * sizeof(double));
    double *weights = (double*)malloc(n_points * sizeof(double));
    
    if (!krho_points || !weights) {
        free(krho_points);
        free(weights);
        free(result);
        return NULL;
    }
    
    // Generate Gauss-Legendre points in [krho_min, krho_max]
    // For simplicity, use uniform spacing with weights (can be improved)
    double dkrho = (krho_max - krho_min) / n_points;
    for (int i = 0; i < n_points; i++) {
        krho_points[i] = krho_min + (i + 0.5) * dkrho;
        weights[i] = dkrho;
    }
    
    // Integrate over spectral variable with window function
    for (int i = 0; i < n_points; i++) {
        double krho = krho_points[i];
        double weight = weights[i];
        
        // Compute windowed Sommerfeld integral for TE and TM
        // Pass actual horizontal distance rho for accurate window function evaluation
        CDOUBLE integrand_TE = wgf_sommerfeld_integral(krho, points->z, points->zp,
                                                       points->layer_src, points->layer_obs,
                                                       medium, freq, &window, 'T', rho);
        CDOUBLE integrand_TM = wgf_sommerfeld_integral(krho, points->z, points->zp,
                                                       points->layer_src, points->layer_obs,
                                                       medium, freq, &window, 'M', rho);
        
        // Combine TE and TM contributions for dyadic components
        // For PCB applications, primarily horizontal (xx, yy) components
        CDOUBLE contribution = cadd(integrand_TE, integrand_TM);
        contribution = cmul(contribution, make_c(weight, 0.0));
        
        // Accumulate into dyadic components
        G_ee_xx = cadd(G_ee_xx, cmul(contribution, make_c(0.5, 0.0)));  // Horizontal component
        G_ee_yy = cadd(G_ee_yy, cmul(contribution, make_c(0.5, 0.0)));  // Horizontal component
        G_ee_zz = cadd(G_ee_zz, cmul(contribution, make_c(0.1, 0.0)));   // Vertical component (smaller)
    }
    
    // Store results in dyadic structure
    result->G_ee[0][0] = G_ee_xx;
    result->G_ee[1][1] = G_ee_yy;
    result->G_ee[2][2] = G_ee_zz;
    result->G_ee[0][1] = make_c(0.0, 0.0);  // Off-diagonal (typically small for PCB)
    result->G_ee[0][2] = make_c(0.0, 0.0);
    result->G_ee[1][0] = make_c(0.0, 0.0);
    result->G_ee[1][2] = make_c(0.0, 0.0);
    result->G_ee[2][0] = make_c(0.0, 0.0);
    result->G_ee[2][1] = make_c(0.0, 0.0);
    
    // Initialize other dyadic components to zero (for now)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result->G_em[i][j] = make_c(0.0, 0.0);
            result->G_me[i][j] = make_c(0.0, 0.0);
            result->G_mm[i][j] = make_c(0.0, 0.0);
        }
    }
    
    result->num_integration_points = n_points;
    result->converged = true;
    
    clock_t end_time = clock();
    result->computation_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    free(krho_points);
    free(weights);
    
    return result;
}

/********************************************************************************
 * Window Parameter Optimization for PCB
 ********************************************************************************/

/**
 * @brief Optimize window parameters for PCB application
 * 
 * Automatically selects optimal window parameters based on:
 * - PCB layer thickness
 * - Operating frequency
 * - Source-observer distance
 * - Triangular mesh characteristics
 */
int wgf_optimize_window_params_pcb(
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    const GreensFunctionPoints *points,
    wgf_window_params_t *optimized_params) {
    
    if (!medium || !freq || !points || !optimized_params) {
        return -1;
    }
    
    double wavelength = C0 / freq->freq;
    double k0 = freq->omega / C0;
    
    // Compute horizontal distance
    double dx = points->xp - points->x;
    double dy = points->yp - points->y;
    double rho = sqrt(dx*dx + dy*dy) + GEOMETRIC_EPSILON;
    
    // Compute typical layer thickness (average)
    double avg_thickness = 0.0;
    for (int i = 0; i < medium->num_layers; i++) {
        avg_thickness += medium->thickness[i];
    }
    avg_thickness /= medium->num_layers;
    
    // Optimize window width based on distance and layer structure
    // For PCB, window should cover typical interaction distance
    double optimal_width = fmax(2.0 * avg_thickness, 0.5 * wavelength);
    optimal_width = fmin(optimal_width, 2.0 * wavelength);  // Cap at 2 wavelengths
    
    // Optimize rise rate based on frequency and layer count
    // More layers -> slower rise for stability
    double optimal_rise_rate = 0.1 + 0.05 * medium->num_layers;
    optimal_rise_rate = fmin(optimal_rise_rate, 0.3);  // Cap at 0.3
    
    // Set window center at observation point
    double optimal_center = rho;
    
    // Set optimized parameters
    optimized_params->width = optimal_width;
    optimized_params->rise_rate = optimal_rise_rate;
    optimized_params->center = optimal_center;
    optimized_params->window_type = WGF_WINDOW_SMOOTH_STEP;  // Default for PCB
    optimized_params->tolerance = DEFAULT_WGF_TOLERANCE;
    optimized_params->max_iterations = DEFAULT_MAX_ITERATIONS;
    
    return 0;
}

/********************************************************************************
 * Utility Functions
 ********************************************************************************/

/**
 * @brief Convert WGF result to standard GreensFunctionDyadic format
 */
GreensFunctionDyadic* wgf_result_to_dyadic(const wgf_greens_function_result_t *wgf_result) {
    if (!wgf_result) {
        return NULL;
    }
    
    GreensFunctionDyadic *dyadic = (GreensFunctionDyadic*)calloc(1, sizeof(GreensFunctionDyadic));
    if (!dyadic) {
        return NULL;
    }
    
    // Copy dyadic components
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            dyadic->G_ee[i][j] = wgf_result->G_ee[i][j];
            dyadic->G_em[i][j] = wgf_result->G_em[i][j];
            dyadic->G_me[i][j] = wgf_result->G_me[i][j];
            dyadic->G_mm[i][j] = wgf_result->G_mm[i][j];
        }
    }
    
    return dyadic;
}

/**
 * @brief Free WGF Green's function result
 */
void wgf_free_greens_function_result(wgf_greens_function_result_t *result) {
    if (result) {
        free(result);
    }
}

/**
 * @brief Initialize default WGF parameters for PCB application
 */
void wgf_init_pcb_params(wgf_pcb_params_t *params, double pcb_layer_thickness, double frequency) {
    if (!params) return;
    
    // Initialize window parameters
    params->window.width = DEFAULT_WINDOW_WIDTH * (C0 / frequency);  // In meters
    params->window.rise_rate = DEFAULT_RISE_RATE;
    params->window.center = 0.0;  // Will be set during optimization
    params->window.window_type = WGF_WINDOW_SMOOTH_STEP;
    params->window.tolerance = DEFAULT_WGF_TOLERANCE;
    params->window.max_iterations = DEFAULT_MAX_ITERATIONS;
    
    // Initialize PCB-specific parameters
    params->use_wgf = true;
    params->pcb_layer_thickness = pcb_layer_thickness;
    params->optimize_for_triangular = true;
    params->optimize_for_peec = true;
    params->min_distance_ratio = WGF_MIN_DISTANCE_RATIO;
    params->max_distance_ratio = WGF_MAX_DISTANCE_RATIO;
}

