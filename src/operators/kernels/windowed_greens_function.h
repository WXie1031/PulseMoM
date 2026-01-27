/*****************************************************************************************
 * PulseEM - Windowed Green Function (WGF) Implementation
 * 
 * Copyright (C) 2025 PulseEM Technologies
 * 
 * File: windowed_greens_function.h
 * Description: Windowed Green Function method for Helmholtz equation in multiply layered media
 * 
 * Based on: IEEE 10045792 - Windowed Green Function method for the Helmholtz equation 
 *          in presence of multiply layered media
 * 
 * Features:
 * - Window function implementation for oscillatory integrals
 * - WGF-Sommerfeld integration for layered media
 * - Optimized for PCB applications with triangular mesh and PEEC
 * - Automatic window parameter selection
 * 
 * Technical Specifications:
 * - C11 compliant with POSIX standard compliance
 * - Complex number support with MSVC compatibility
 * - OpenMP parallel processing for multi-threaded evaluation
 * - Cross-platform compatibility (Linux, Windows, macOS)
 *****************************************************************************************/

#ifndef WINDOWED_GREENS_FUNCTION_H
#define WINDOWED_GREENS_FUNCTION_H

#include "core_common.h"
#include "../discretization//geometry/core_geometry.h"
#include "../operators/greens/layered_greens_function.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Window function types
typedef enum {
    WGF_WINDOW_SMOOTH_STEP,      // Smooth step function (default)
    WGF_WINDOW_GAUSSIAN,          // Gaussian window
    WGF_WINDOW_EXPONENTIAL,       // Exponential window
    WGF_WINDOW_TANH,              // Hyperbolic tangent window
    WGF_WINDOW_ADAPTIVE           // Adaptive window (auto-select)
} wgf_window_type_t;

// Window function parameters
typedef struct {
    double width;                 // Window width (in wavelengths)
    double rise_rate;             // Rise rate (steepness of transition)
    double center;                // Window center position
    wgf_window_type_t window_type; // Window function type
    double tolerance;              // Convergence tolerance
    int max_iterations;           // Maximum iterations for adaptive selection
} wgf_window_params_t;

// WGF-specific parameters for PCB applications
typedef struct {
    bool use_wgf;                 // Enable WGF method
    wgf_window_params_t window;    // Window function parameters
    double pcb_layer_thickness;    // Typical PCB layer thickness (for optimization)
    bool optimize_for_triangular; // Optimize for triangular mesh
    bool optimize_for_peec;        // Optimize for PEEC solver
    double min_distance_ratio;    // Minimum distance/wavelength ratio for WGF
    double max_distance_ratio;     // Maximum distance/wavelength ratio for WGF
} wgf_pcb_params_t;

// WGF Green's function result structure
typedef struct {
    CDOUBLE G_ee[3][3];           // Electric-electric dyadic (same as GreensFunctionDyadic)
    CDOUBLE G_em[3][3];           // Electric-magnetic dyadic
    CDOUBLE G_me[3][3];           // Magnetic-electric dyadic
    CDOUBLE G_mm[3][3];           // Magnetic-magnetic dyadic
    double computation_time;      // Computation time (for performance analysis)
    int num_integration_points;   // Number of integration points used
    bool converged;               // Convergence flag
} wgf_greens_function_result_t;

/**
 * @brief Compute window function value
 * @param x Input coordinate
 * @param params Window function parameters
 * @return Window function value [0, 1]
 */
double wgf_window_function(double x, const wgf_window_params_t* params);

/**
 * @brief Compute windowed Sommerfeld integral for layered media
 * @param krho Spectral variable
 * @param z Observation z-coordinate
 * @param zp Source z-coordinate
 * @param layer_src Source layer index
 * @param layer_obs Observation layer index
 * @param medium Layered medium structure
 * @param freq Frequency domain parameters
 * @param window_params Window function parameters
 * @param polarization 'T' for TE, 'M' for TM
 * @return Complex integral value
 */
CDOUBLE wgf_sommerfeld_integral(
    double krho, double z, double zp,
    int layer_src, int layer_obs,
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    const wgf_window_params_t *window_params,
    char polarization,
    double rho_actual  // Actual horizontal distance for window function
);

/**
 * @brief Compute WGF Green's function for layered media (PCB optimized)
 * @param medium Layered medium structure
 * @param freq Frequency domain parameters
 * @param points Source and observation points
 * @param wgf_params WGF parameters (including window and PCB-specific settings)
 * @return WGF Green's function result (caller must free)
 */
wgf_greens_function_result_t* wgf_layered_medium_greens_function(
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    const GreensFunctionPoints *points,
    const wgf_pcb_params_t *wgf_params
);

/**
 * @brief Optimize window parameters for PCB application
 * @param medium Layered medium structure
 * @param freq Frequency domain parameters
 * @param points Source and observation points
 * @param optimized_params Output optimized window parameters
 * @return 0 on success, error code otherwise
 */
int wgf_optimize_window_params_pcb(
    const LayeredMedium *medium,
    const FrequencyDomain *freq,
    const GreensFunctionPoints *points,
    wgf_window_params_t *optimized_params
);

/**
 * @brief Convert WGF result to standard GreensFunctionDyadic format
 * @param wgf_result WGF result structure
 * @return Standard GreensFunctionDyadic structure (caller must free)
 */
GreensFunctionDyadic* wgf_result_to_dyadic(const wgf_greens_function_result_t *wgf_result);

/**
 * @brief Free WGF Green's function result
 * @param result Result structure to free
 */
void wgf_free_greens_function_result(wgf_greens_function_result_t *result);

/**
 * @brief Initialize default WGF parameters for PCB application
 * @param params Output WGF parameters structure
 * @param pcb_layer_thickness Typical PCB layer thickness (m)
 * @param frequency Operating frequency (Hz)
 */
void wgf_init_pcb_params(wgf_pcb_params_t *params, double pcb_layer_thickness, double frequency);

#ifdef __cplusplus
}
#endif

#endif // WINDOWED_GREENS_FUNCTION_H

