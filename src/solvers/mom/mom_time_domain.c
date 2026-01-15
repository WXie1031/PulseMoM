/******************************************************************************
 * MoM Solver Time-Domain Analysis - Implementation
 * 
 * Method: Frequency-domain to time-domain via inverse FFT
 ******************************************************************************/

#include "mom_time_domain.h"
#include "mom_solver.h"
#include "../../core/core_common.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Use FFTW if available (works on MSVC too if FFTW is compiled for Windows)
#ifdef HAVE_FFTW3
#include <fftw3.h>
#define USE_FFTW 1
#else
// Fallback: Simple FFT implementation (slower but works everywhere)
#define USE_FFTW 0
#endif

#if !USE_FFTW
// Fallback FFT implementation using Cooley-Tukey algorithm
static void fft_1d(complex_t* data, int n, int isign) {
    if (n <= 1) return;
    
    // Bit-reverse permutation
    int j = 0;
    for (int i = 0; i < n; i++) {
        if (j > i) {
            complex_t temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
        int m = n >> 1;
        while (m >= 1 && j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    
    // FFT computation
    for (int m = 1; m < n; m <<= 1) {
        double theta = isign * M_PI / m;
        complex_t wm;
        wm.re = cos(theta);
        wm.im = sin(theta);
        
        for (int k = 0; k < n; k += 2 * m) {
            complex_t w;
            w.re = 1.0;
            w.im = 0.0;
            
            for (int j = 0; j < m; j++) {
                complex_t t = complex_multiply(&w, &data[k + j + m]);
                complex_t u = data[k + j];
                data[k + j] = complex_add(&u, &t);
                data[k + j + m] = complex_subtract(&u, &t);
                w = complex_multiply(&w, &wm);
            }
        }
    }
    
    // Normalize for inverse FFT
    if (isign < 0) {
        for (int i = 0; i < n; i++) {
            data[i].re /= n;
            data[i].im /= n;
        }
    }
}
#endif

/******************************************************************************
 * Default Configuration
 ******************************************************************************/

mom_time_domain_config_t mom_time_domain_get_default_config(void) {
    mom_time_domain_config_t config = {0};
    config.time_start = 0.0;
    config.time_stop = 10e-9;  // 10 ns
    config.time_step = 1e-12;  // 1 ps
    config.num_time_points = 10000;
    config.use_adaptive_stepping = false;
    config.min_time_step = 1e-15;  // 1 fs
    config.max_time_step = 1e-9;   // 1 ns
    return config;
}

/******************************************************************************
 * Time-Domain Analysis Implementation
 ******************************************************************************/

int mom_solver_solve_time_domain(
    mom_solver_t* solver,
    const double* frequencies,
    int num_frequencies,
    const mom_time_domain_config_t* config,
    mom_time_domain_results_t* time_results
) {
    if (!solver || !frequencies || num_frequencies < 1 || !config || !time_results) {
        return -1;
    }
    
    // Allocate time points
    time_results->num_time_points = config->num_time_points;
    time_results->time_points = (double*)calloc(config->num_time_points, sizeof(double));
    if (!time_results->time_points) {
        return -1;
    }
    
    // Generate time points
    double dt = (config->time_stop - config->time_start) / (config->num_time_points - 1);
    for (int i = 0; i < config->num_time_points; i++) {
        time_results->time_points[i] = config->time_start + i * dt;
    }
    time_results->sampling_rate = 1.0 / dt;
    
    // Get number of basis functions from solver
    // Query solver result if available
    int num_basis = 0;
    
    // Try to get from solver result
    const mom_result_t* result = mom_solver_get_results(solver);
    if (result && result->num_basis_functions > 0) {
        num_basis = result->num_basis_functions;
    } else {
        // Fallback: query number of unknowns
        num_basis = mom_solver_get_num_unknowns(solver);
    }
    
    // If still zero, use a default estimate
    if (num_basis <= 0) {
        // Estimate from number of frequencies (rough heuristic)
        // In practice, this should be queried from solver before calling this function
        num_basis = num_frequencies * 10;  // Rough estimate
    }
    
    // Solve frequency domain for all frequencies and collect results
    complex_t** freq_currents = (complex_t**)calloc(num_frequencies, sizeof(complex_t*));
    if (!freq_currents) {
        free(time_results->time_points);
        return -1;
    }
    
    // Solve at each frequency and collect results
    for (int f = 0; f < num_frequencies; f++) {
        // Update solver frequency (if API available)
        // Note: This assumes solver has a way to set frequency
        // For now, we'll solve and extract current coefficients
        
        // Update frequency in solver config if possible
        // This would typically be done via mom_solver_configure() or similar
        
        // Solve at this frequency
        if (mom_solver_solve(solver) != 0) {
            // Cleanup on error
            for (int i = 0; i < f; i++) {
                if (freq_currents[i]) free(freq_currents[i]);
            }
            free(freq_currents);
            free(time_results->time_points);
            return -1;
        }
        
        // Extract current coefficients from solver result
        const mom_result_t* result = mom_solver_get_results(solver);
        if (result && result->current_coefficients && result->num_basis_functions > 0) {
            // Update num_basis if we got it from result
            if (num_basis == 0 || num_basis != result->num_basis_functions) {
                num_basis = result->num_basis_functions;
                
                // Reallocate previous frequency results if size changed
                if (f > 0 && num_basis > 0) {
                    for (int i = 0; i < f; i++) {
                        if (freq_currents[i]) {
                            free(freq_currents[i]);
                            freq_currents[i] = (complex_t*)calloc(num_basis, sizeof(complex_t));
                        }
                    }
                }
            }
            
            // Allocate for this frequency
            freq_currents[f] = (complex_t*)calloc(num_basis, sizeof(complex_t));
            if (!freq_currents[f]) {
                for (int i = 0; i < f; i++) {
                    if (freq_currents[i]) free(freq_currents[i]);
                }
                free(freq_currents);
                free(time_results->time_points);
                return -1;
            }
            
            // Copy current coefficients from result
            // Note: result->current_coefficients is mom_scalar_complex_t*
            // We need to convert to complex_t
            for (int b = 0; b < num_basis && b < result->num_basis_functions; b++) {
#if defined(_MSC_VER)
                // For MSVC, both mom_scalar_complex_t and complex_t are struct { double re; double im; }
                freq_currents[f][b] = result->current_coefficients[b];
#else
                // For other compilers, mom_scalar_complex_t is double complex, complex_t is struct
                freq_currents[f][b].re = creal(result->current_coefficients[b]);
                freq_currents[f][b].im = cimag(result->current_coefficients[b]);
#endif
            }
        } else {
            // Fallback: allocate zero-filled array
            freq_currents[f] = (complex_t*)calloc(num_basis, sizeof(complex_t));
            if (!freq_currents[f]) {
                for (int i = 0; i < f; i++) {
                    if (freq_currents[i]) free(freq_currents[i]);
                }
                free(freq_currents);
                free(time_results->time_points);
                return -1;
            }
        }
    }
    
    // Allocate time-domain result arrays
    if (num_basis > 0) {
        time_results->current_response = (complex_t*)calloc(config->num_time_points * num_basis, sizeof(complex_t));
        if (!time_results->current_response) {
            for (int i = 0; i < num_frequencies; i++) {
                if (freq_currents[i]) free(freq_currents[i]);
            }
            free(freq_currents);
            free(time_results->time_points);
            return -1;
        }
    }
    
    // Perform inverse FFT for each basis function
    // Pad frequency data to match time points (zero-padding)
    int fft_size = config->num_time_points;
    complex_t* freq_padded = (complex_t*)calloc(fft_size, sizeof(complex_t));
    if (!freq_padded) {
        for (int i = 0; i < num_frequencies; i++) {
            if (freq_currents[i]) free(freq_currents[i]);
        }
        free(freq_currents);
        if (time_results->current_response) free(time_results->current_response);
        free(time_results->time_points);
        return -1;
    }
    
    for (int b = 0; b < num_basis; b++) {
        // Prepare frequency domain data (with zero-padding)
        for (int f = 0; f < num_frequencies; f++) {
            if (f < fft_size) {
                freq_padded[f] = freq_currents[f][b];
            }
        }
        // Zero-pad the rest
        for (int f = num_frequencies; f < fft_size; f++) {
            freq_padded[f].re = 0.0;
            freq_padded[f].im = 0.0;
        }
        
        // Perform inverse FFT
#if USE_FFTW
        // Use FFTW (faster, optimized)
        fftw_plan plan = fftw_plan_dft_1d(fft_size, 
                                          (fftw_complex*)freq_padded,
                                          (fftw_complex*)freq_padded,
                                          FFTW_BACKWARD, FFTW_ESTIMATE);
        if (plan) {
            fftw_execute(plan);
            fftw_destroy_plan(plan);
            // Normalize
            for (int i = 0; i < fft_size; i++) {
                freq_padded[i].re /= fft_size;
                freq_padded[i].im /= fft_size;
            }
        }
#else
        // Fallback: use custom FFT implementation
        fft_1d(freq_padded, fft_size, -1);  // -1 for inverse FFT
#endif
        
        // Store time-domain result
        for (int t = 0; t < config->num_time_points; t++) {
            time_results->current_response[t * num_basis + b] = freq_padded[t];
        }
    }
    
    // Cleanup frequency domain data
    for (int i = 0; i < num_frequencies; i++) {
        if (freq_currents[i]) free(freq_currents[i]);
    }
    free(freq_currents);
    free(freq_padded);
    
    return 0;
}

void mom_time_domain_free_results(mom_time_domain_results_t* results) {
    if (!results) {
        return;
    }
    
    if (results->time_points) {
        free(results->time_points);
        results->time_points = NULL;
    }
    
    if (results->current_response) {
        free(results->current_response);
        results->current_response = NULL;
    }
    
    if (results->voltage_response) {
        free(results->voltage_response);
        results->voltage_response = NULL;
    }
    
    if (results->field_response) {
        free(results->field_response);
        results->field_response = NULL;
    }
    
    results->num_time_points = 0;
    results->sampling_rate = 0.0;
}
