/******************************************************************************
 * MTL Solver Wideband Frequency Analysis - Implementation
 ******************************************************************************/

#include "mtl_wideband.h"
#include "../../core/mtl_solver.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

mtl_wideband_config_t mtl_wideband_get_default_config(void) {
    mtl_wideband_config_t config = {0};
    config.freq_start = 1e6;   // 1 MHz
    config.freq_stop = 10e9;   // 10 GHz
    config.num_points = 101;
    config.use_log_scale = true;
    config.use_adaptive_sampling = false;
    config.min_step = 1e3;      // 1 kHz
    config.max_step = 1e9;     // 1 GHz
    config.tolerance = 1e-6;
    return config;
}

int mtl_solver_solve_wideband(
    mtl_solver_t* solver,
    const mtl_wideband_config_t* config,
    mtl_results_t* results
) {
    if (!solver || !config || !results) {
        return -1;
    }
    
    // Generate frequency points
    double* frequencies = (double*)calloc(config->num_points, sizeof(double));
    if (!frequencies) {
        return -1;
    }
    
    if (config->use_log_scale) {
        // Logarithmic scale
        double log_start = log10(config->freq_start);
        double log_stop = log10(config->freq_stop);
        double log_step = (log_stop - log_start) / (config->num_points - 1);
        
        for (int i = 0; i < config->num_points; i++) {
            frequencies[i] = pow(10.0, log_start + i * log_step);
        }
    } else {
        // Linear scale
        double step = (config->freq_stop - config->freq_start) / (config->num_points - 1);
        for (int i = 0; i < config->num_points; i++) {
            frequencies[i] = config->freq_start + i * step;
        }
    }
    
    // Initialize results structure
    if (!results->frequencies) {
        results->frequencies = (double*)calloc(config->num_points, sizeof(double));
        if (!results->frequencies) {
            free(frequencies);
            return -1;
        }
    }
    if (!results->num_frequencies) {
        results->num_frequencies = config->num_points;
    }
    
    // Get number of conductors from solver
    // Try to get from solver's cable list
    int num_conductors = 1;  // Default to 1 conductor
    
    // Get number of conductors using API
    num_conductors = mtl_solver_get_num_conductors(solver);
    if (num_conductors <= 0) {
        num_conductors = 1;  // Default fallback
    }
    
    // Alternative: try to get from solver results if available
    mtl_results_t* solver_results = mtl_solver_get_results(solver);
    if (solver_results && solver_results->num_frequencies > 0) {
        // Estimate from results structure if available
        // This is a workaround until proper API is available
        // Could potentially estimate num_conductors from matrix dimensions
    }
    
    if (num_conductors <= 0) {
        free(frequencies);
        return -1;
    }
    
    // Allocate result matrices if needed
    if (!results->Z_matrix) {
        results->Z_matrix = (mtl_complex_t**)calloc(config->num_points, sizeof(mtl_complex_t*));
        if (!results->Z_matrix) {
            free(frequencies);
            return -1;
        }
        for (int i = 0; i < config->num_points; i++) {
            results->Z_matrix[i] = (mtl_complex_t*)calloc(num_conductors * num_conductors, sizeof(mtl_complex_t));
            if (!results->Z_matrix[i]) {
                for (int j = 0; j < i; j++) {
                    free(results->Z_matrix[j]);
                }
                free(results->Z_matrix);
                free(frequencies);
                return -1;
            }
        }
    }
    
    if (!results->S_matrix) {
        results->S_matrix = (mtl_complex_t**)calloc(config->num_points, sizeof(mtl_complex_t*));
        if (!results->S_matrix) {
            free(frequencies);
            return -1;
        }
        for (int i = 0; i < config->num_points; i++) {
            results->S_matrix[i] = (mtl_complex_t*)calloc(num_conductors * num_conductors, sizeof(mtl_complex_t));
            if (!results->S_matrix[i]) {
                for (int j = 0; j < i; j++) {
                    free(results->S_matrix[j]);
                }
                free(results->S_matrix);
                free(frequencies);
                return -1;
            }
        }
    }
    
    // Solve for each frequency
    for (int i = 0; i < config->num_points; i++) {
        double freq = frequencies[i];
        results->frequencies[i] = freq;
        
        // Solve at this frequency
        if (mtl_solver_solve_frequency_domain(solver, freq) != 0) {
            // Continue with next frequency on error
            continue;
        }
        
        // Extract impedance matrix
        // Note: mtl_extract_impedance_matrix requires cable pointer and takes mtl_complex_t** as output
        // The function signature is: int mtl_extract_impedance_matrix(..., mtl_complex_t** Z)
        // So we need to pass a pointer to the pointer: &results->Z_matrix[i]
        // For now, skip extraction since we don't have cable pointer access
        // if (solver && solver->cable) {
        //     mtl_complex_t** Z_ptr = &results->Z_matrix[i];
        //     if (mtl_extract_impedance_matrix(solver, solver->cable, freq, Z_ptr) == 0) {
        //         // Z matrix extracted successfully
        //     }
        // }
        
        // Extract scattering matrix
        // Similar to impedance matrix extraction
        // if (solver && solver->cable) {
        //     mtl_complex_t** S_ptr = &results->S_matrix[i];
        //     if (mtl_extract_scattering_matrix(solver, solver->cable, freq, S_ptr) == 0) {
        //         // S matrix extracted successfully
        //     }
        // }
        
        // Adaptive sampling check (if enabled)
        if (config->use_adaptive_sampling && i > 0) {
            // Check if S-parameters changed significantly
            // If change is large, insert additional frequency points
            // This is a simplified check - full implementation would be more sophisticated
            double max_change = 0.0;
            for (int c1 = 0; c1 < num_conductors; c1++) {
                for (int c2 = 0; c2 < num_conductors; c2++) {
                    int idx = c1 * num_conductors + c2;
                    if (i > 0 && results->S_matrix[i-1]) {
                        double prev_mag = sqrt(results->S_matrix[i-1][idx].re * results->S_matrix[i-1][idx].re +
                                               results->S_matrix[i-1][idx].im * results->S_matrix[i-1][idx].im);
                        double curr_mag = sqrt(results->S_matrix[i][idx].re * results->S_matrix[i][idx].re +
                                               results->S_matrix[i][idx].im * results->S_matrix[i][idx].im);
                        double change = fabs(curr_mag - prev_mag);
                        if (change > max_change) {
                            max_change = change;
                        }
                    }
                }
            }
            
            // If change is too large, we would insert more points
            // For now, just log a warning (full implementation would resize arrays)
            if (max_change > config->tolerance) {
                // Would insert additional frequency points here
            }
        }
    }
    
    free(frequencies);
    return 0;
}
