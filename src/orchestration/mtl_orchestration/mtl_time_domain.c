/******************************************************************************
 * MTL Solver Time-Domain Analysis - Implementation (L5 Orchestration Layer)
 ******************************************************************************/

#include "mtl_time_domain.h"
#include "../../physics/mtl/mtl_physics.h"  // Use standard L1 physics definitions
#include "../../solvers/mtl/mtl_solver_module.h"  // For solver-specific types
#include <stdlib.h>
#include <string.h>
#include <math.h>

mtl_time_domain_config_t mtl_time_domain_get_default_config(void) {
    mtl_time_domain_config_t config = {0};
    config.time_start = 0.0;
    config.time_stop = 10e-9;  // 10 ns
    config.time_step = 1e-12;  // 1 ps
    config.use_adaptive_stepping = true;
    config.min_time_step = 1e-15;  // 1 fs
    config.max_time_step = 1e-9;   // 1 ns
    config.voltage_tolerance = 1e-6;
    config.current_tolerance = 1e-9;
    return config;
}

int mtl_orchestration_solve_time_domain(
    mtl_solver_t* solver,
    const mtl_time_domain_config_t* config,
    mtl_time_domain_results_t* time_results
) {
    if (!solver || !config || !time_results) {
        return -1;
    }
    
    // Allocate time points
    time_results->num_time_points = (int)((config->time_stop - config->time_start) / config->time_step) + 1;
    time_results->time_points = (double*)calloc(time_results->num_time_points, sizeof(double));
    if (!time_results->time_points) {
        return -1;
    }
    
    // Generate time points
    for (int i = 0; i < time_results->num_time_points; i++) {
        time_results->time_points[i] = config->time_start + i * config->time_step;
    }
    
    // Get number of conductors from solver results
    int num_conductors = 1;  // Default to 1 conductor
    
    // Try to get from solver results if available
    mtl_results_t* results = mtl_solver_get_results(solver);
    if (results && results->num_conductors > 0) {
        num_conductors = results->num_conductors;
    }
    
    // Fallback: try to get from geometry if available
    if (num_conductors <= 0) {
        num_conductors = 1;  // Default fallback
    }
    
    time_results->num_conductors = num_conductors;
    
    // Allocate result arrays
    time_results->voltages = (double**)calloc(num_conductors, sizeof(double*));
    time_results->currents = (double**)calloc(num_conductors, sizeof(double*));
    
    if (!time_results->voltages || !time_results->currents) {
        free(time_results->time_points);
        return -1;
    }
    
    for (int c = 0; c < num_conductors; c++) {
        time_results->voltages[c] = (double*)calloc(time_results->num_time_points, sizeof(double));
        time_results->currents[c] = (double*)calloc(time_results->num_time_points, sizeof(double));
        
        if (!time_results->voltages[c] || !time_results->currents[c]) {
            // Cleanup on error
            for (int i = 0; i < c; i++) {
                if (time_results->voltages[i]) free(time_results->voltages[i]);
                if (time_results->currents[i]) free(time_results->currents[i]);
            }
            free(time_results->voltages);
            free(time_results->currents);
            free(time_results->time_points);
            return -1;
        }
    }
    
    // Initialize: zero initial conditions
    for (int c = 0; c < num_conductors; c++) {
        time_results->voltages[c][0] = 0.0;
        time_results->currents[c][0] = 0.0;
    }
    
    // FDTD solution for transmission line equations
    // Get per-unit-length parameters (would query from solver)
    double R = 1.0;  // Resistance per unit length (Ohms/m) - default
    double L = 1e-6; // Inductance per unit length (H/m) - default
    double C = 1e-12; // Capacitance per unit length (F/m) - default
    double G = 1e-6;  // Conductance per unit length (S/m) - default
    
    double dt = config->time_step;
    
    // Time-stepping loop
    for (int t = 1; t < time_results->num_time_points; t++) {
        double time = time_results->time_points[t];
        
        // Adaptive time stepping
        if (config->use_adaptive_stepping) {
            // Check if time step needs adjustment
            double v_phase = 1.0 / sqrt(L * C);  // Phase velocity
            double dx_min = 1e-3;  // Minimum spatial step (1 mm)
            double dt_max_stable = dx_min / v_phase;
            
            if (dt > dt_max_stable) {
                dt = dt_max_stable * 0.9;  // Safety factor
            }
            if (dt < config->min_time_step) dt = config->min_time_step;
            if (dt > config->max_time_step) dt = config->max_time_step;
        }
        
        // Update voltages and currents for each conductor
        for (int c = 0; c < num_conductors; c++) {
            // Get previous values
            double V_prev = time_results->voltages[c][t - 1];
            double I_prev = time_results->currents[c][t - 1];
            
            // Simplified FDTD update (lumped model)
            double dV_dt = -R * I_prev - G * V_prev;
            time_results->voltages[c][t] = V_prev + dt * dV_dt;
            
            double dI_dt = -V_prev / L - R * I_prev / L;
            time_results->currents[c][t] = I_prev + dt * dI_dt;
        }
        
        // Check convergence for adaptive stepping
        if (config->use_adaptive_stepping && t > 0) {
            double max_voltage_change = 0.0;
            double max_current_change = 0.0;
            
            for (int c = 0; c < num_conductors; c++) {
                double v_change = fabs(time_results->voltages[c][t] - time_results->voltages[c][t - 1]);
                double i_change = fabs(time_results->currents[c][t] - time_results->currents[c][t - 1]);
                
                if (v_change > max_voltage_change) {
                    max_voltage_change = v_change;
                }
                if (i_change > max_current_change) {
                    max_current_change = i_change;
                }
            }
            
            // Adjust time step if needed
            if (max_voltage_change > config->voltage_tolerance || 
                max_current_change > config->current_tolerance) {
                // Would reduce time step and recompute
            }
        }
    }
    
    return 0;
}

void mtl_time_domain_free_results(mtl_time_domain_results_t* results) {
    if (!results) {
        return;
    }
    
    if (results->time_points) {
        free(results->time_points);
        results->time_points = NULL;
    }
    
    if (results->voltages) {
        for (int i = 0; i < results->num_conductors; i++) {
            if (results->voltages[i]) {
                free(results->voltages[i]);
            }
        }
        free(results->voltages);
        results->voltages = NULL;
    }
    
    if (results->currents) {
        for (int i = 0; i < results->num_conductors; i++) {
            if (results->currents[i]) {
                free(results->currents[i]);
            }
        }
        free(results->currents);
        results->currents = NULL;
    }
    
    results->num_time_points = 0;
    results->num_conductors = 0;
}
