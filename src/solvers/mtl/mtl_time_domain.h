/******************************************************************************
 * MTL Solver Time-Domain Analysis
 * 
 * Implements time-domain transient analysis for MTL solver
 * 
 * Method: Transmission line transient analysis
 ******************************************************************************/

#ifndef MTL_TIME_DOMAIN_H
#define MTL_TIME_DOMAIN_H

#include "../../core/mtl_solver.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Time-Domain Configuration
 ******************************************************************************/

typedef struct {
    double time_start;                   // Start time (s)
    double time_stop;                    // Stop time (s)
    double time_step;                    // Initial time step (s)
    bool use_adaptive_stepping;          // Use adaptive time stepping
    double min_time_step;                // Minimum time step (s)
    double max_time_step;                // Maximum time step (s)
    double voltage_tolerance;            // Voltage convergence tolerance
    double current_tolerance;            // Current convergence tolerance
} mtl_time_domain_config_t;

/******************************************************************************
 * Time-Domain Results
 ******************************************************************************/

typedef struct {
    double* time_points;                  // Time points (s)
    double** voltages;                   // Voltages vs time [num_conductors][num_time_points]
    double** currents;                    // Currents vs time [num_conductors][num_time_points]
    int num_time_points;                  // Number of time points
    int num_conductors;                   // Number of conductors
} mtl_time_domain_results_t;

/******************************************************************************
 * Time-Domain Functions
 ******************************************************************************/

/**
 * Solve time domain transient analysis
 * 
 * @param solver MTL solver
 * @param config Time-domain configuration
 * @param time_results Output: time-domain results
 * @return 0 on success, negative on error
 */
int mtl_solver_solve_time_domain(
    mtl_solver_t* solver,
    const mtl_time_domain_config_t* config,
    mtl_time_domain_results_t* time_results
);

/**
 * Get default time-domain configuration
 * 
 * @return Default configuration
 */
mtl_time_domain_config_t mtl_time_domain_get_default_config(void);

/**
 * Free time-domain results
 * 
 * @param results Time-domain results to free
 */
void mtl_time_domain_free_results(mtl_time_domain_results_t* results);

#ifdef __cplusplus
}
#endif

#endif // MTL_TIME_DOMAIN_H
