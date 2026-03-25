/******************************************************************************
 * MoM Solver Time-Domain Analysis
 * 
 * Implements time-domain analysis for MoM solver using FFT
 * 
 * Method: Frequency-domain to time-domain via inverse FFT
 ******************************************************************************/

#ifndef MOM_TIME_DOMAIN_H
#define MOM_TIME_DOMAIN_H

#include "mom_solver.h"
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
    double time_step;                     // Time step (s)
    int num_time_points;                  // Number of time points
    bool use_adaptive_stepping;           // Use adaptive time stepping
    double min_time_step;                 // Minimum time step (s)
    double max_time_step;                 // Maximum time step (s)
} mom_time_domain_config_t;

/******************************************************************************
 * Time-Domain Results
 ******************************************************************************/

typedef struct {
    double* time_points;                  // Time points (s)
    complex_t* current_response;         // Current response vs time
    complex_t* voltage_response;          // Voltage response vs time
    complex_t* field_response;            // Field response vs time
    int num_time_points;                  // Number of time points
    double sampling_rate;                 // Sampling rate (Hz)
} mom_time_domain_results_t;

/******************************************************************************
 * Time-Domain Functions
 ******************************************************************************/

/**
 * Solve time domain via FFT from frequency domain results
 * 
 * @param solver MoM solver
 * @param frequencies Frequency points for FFT (Hz)
 * @param num_frequencies Number of frequency points
 * @param config Time-domain configuration
 * @param time_results Output: time-domain results
 * @return 0 on success, negative on error
 */
int mom_solver_solve_time_domain(
    mom_solver_t* solver,
    const double* frequencies,
    int num_frequencies,
    const mom_time_domain_config_t* config,
    mom_time_domain_results_t* time_results
);

/**
 * Get default time-domain configuration
 * 
 * @return Default configuration
 */
mom_time_domain_config_t mom_time_domain_get_default_config(void);

/**
 * Free time-domain results
 * 
 * @param results Time-domain results to free
 */
void mom_time_domain_free_results(mom_time_domain_results_t* results);

/**
 * Build n linearly spaced frequency samples in [f_min_hz, f_max_hz] (Hz).
 * Caller must free *out_freqs with free().
 * @return 0 on success, negative on error
 */
int mom_time_domain_build_linear_frequencies_hz(double f_min_hz, double f_max_hz, int n,
                                                double** out_freqs);

#ifdef __cplusplus
}
#endif

#endif // MOM_TIME_DOMAIN_H
