/******************************************************************************
 * MTL Solver Wideband Frequency Analysis - Implementation (L5 Orchestration Layer)
 ******************************************************************************/

#include "mtl_wideband.h"
#include "../../physics/mtl/mtl_physics.h"  // Use standard L1 physics definitions
#include "../../solvers/mtl/mtl_solver_module.h"  // For solver-specific types
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
    
    // Get number of conductors from solver results
    int num_conductors = 1;  // Default to 1 conductor
    
    // Try to get from solver results if available
    mtl_results_t* solver_results = mtl_solver_get_results(solver);
    if (solver_results && solver_results->num_conductors > 0) {
        num_conductors = solver_results->num_conductors;
    }
    
    // Fallback: try to get from geometry if available
    if (num_conductors <= 0) {
        num_conductors = 1;  // Default fallback
    }
    
    // Allocate result matrices if needed
    if (!results->z_matrix) {
        results->z_matrix = (CDOUBLE**)calloc(config->num_points, sizeof(CDOUBLE*));
        if (!results->z_matrix) {
            free(frequencies);
            return -1;
        }
        for (int i = 0; i < config->num_points; i++) {
            results->z_matrix[i] = (CDOUBLE*)calloc(num_conductors * num_conductors, sizeof(CDOUBLE));
            if (!results->z_matrix[i]) {
                for (int j = 0; j < i; j++) {
                    free(results->z_matrix[j]);
                }
                free(results->z_matrix);
                free(frequencies);
                return -1;
            }
        }
    }
    
    if (!results->s_matrix) {
        results->s_matrix = (CDOUBLE**)calloc(config->num_points, sizeof(CDOUBLE*));
        if (!results->s_matrix) {
            free(frequencies);
            return -1;
        }
        for (int i = 0; i < config->num_points; i++) {
            results->s_matrix[i] = (CDOUBLE*)calloc(num_conductors * num_conductors, sizeof(CDOUBLE));
            if (!results->s_matrix[i]) {
                for (int j = 0; j < i; j++) {
                    free(results->s_matrix[j]);
                }
                free(results->s_matrix);
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
        
        // Adaptive sampling check (if enabled)
        if (config->use_adaptive_sampling && i > 0) {
            // Check if S-parameters changed significantly
            double max_change = 0.0;
            for (int c1 = 0; c1 < num_conductors; c1++) {
                for (int c2 = 0; c2 < num_conductors; c2++) {
                    int idx = c1 * num_conductors + c2;
                    if (i > 0 && results->s_matrix[i-1]) {
#if defined(_MSC_VER)
                        double prev_mag = sqrt(results->s_matrix[i-1][idx].re * results->s_matrix[i-1][idx].re +
                                               results->s_matrix[i-1][idx].im * results->s_matrix[i-1][idx].im);
                        double curr_mag = sqrt(results->s_matrix[i][idx].re * results->s_matrix[i][idx].re +
                                               results->s_matrix[i][idx].im * results->s_matrix[i][idx].im);
#else
                        double prev_mag = CABS(results->s_matrix[i-1][idx]);
                        double curr_mag = CABS(results->s_matrix[i][idx]);
#endif
                        double change = fabs(curr_mag - prev_mag);
                        if (change > max_change) {
                            max_change = change;
                        }
                    }
                }
            }
            
            // If change is too large, we would insert more points
            if (max_change > config->tolerance) {
                // Would insert additional frequency points here
            }
        }
    }
    
    free(frequencies);
    return 0;
}
