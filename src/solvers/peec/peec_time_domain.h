/******************************************************************************
 * PEEC Solver Time-Domain Analysis
 * 
 * Implements time-domain transient analysis for PEEC solver
 * 
 * Method: Circuit transient analysis with adaptive time stepping
 ******************************************************************************/

#ifndef PEEC_TIME_DOMAIN_H
#define PEEC_TIME_DOMAIN_H

#include "peec_solver.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Time-Domain Configuration
 ******************************************************************************/

typedef struct {
    double time_start;                    // Start time (s)
    double time_stop;                     // Stop time (s)
    double time_step;                     // Initial time step (s)
    bool use_adaptive_stepping;           // Use adaptive time stepping
    double min_time_step;                 // Minimum time step (s)
    double max_time_step;                 // Maximum time step (s)
    double voltage_tolerance;             // Voltage convergence tolerance
    double current_tolerance;              // Current convergence tolerance
    int max_iterations;                   // Maximum NR iterations per step
} peec_time_domain_config_t;

/******************************************************************************
 * Time-Domain Results
 ******************************************************************************/

typedef struct {
    double* time_points;                  // Time points (s)
    double* node_voltages;               // Node voltages vs time [num_nodes][num_time_points]
    double* branch_currents;             // Branch currents vs time [num_branches][num_time_points]
    int num_time_points;                  // Number of time points
    int num_nodes;                        // Number of nodes
    int num_branches;                     // Number of branches
} peec_time_domain_results_t;

/******************************************************************************
 * Time-Domain Functions
 ******************************************************************************/

/**
 * Solve time domain transient analysis
 * 
 * @param solver PEEC solver
 * @param config Time-domain configuration
 * @param time_results Output: time-domain results
 * @return 0 on success, negative on error
 */
int peec_solver_solve_time_domain(
    peec_solver_t* solver,
    const peec_time_domain_config_t* config,
    peec_time_domain_results_t* time_results
);

/**
 * Get default time-domain configuration
 * 
 * @return Default configuration
 */
peec_time_domain_config_t peec_time_domain_get_default_config(void);

/**
 * Free time-domain results
 * 
 * @param results Time-domain results to free
 */
void peec_time_domain_free_results(peec_time_domain_results_t* results);

#ifdef __cplusplus
}
#endif

#endif // PEEC_TIME_DOMAIN_H
