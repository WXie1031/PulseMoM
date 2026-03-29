/******************************************************************************
 * MoM Solver Time-Domain Analysis
 * 
 * Implements time-domain analysis for MoM solver using FFT
 * 
 * Method: Frequency-domain to time-domain via inverse FFT
 ******************************************************************************/

#ifndef MOM_TIME_DOMAIN_H
#define MOM_TIME_DOMAIN_H

#include "../../solvers/mom/mom_solver.h"
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
 * @param frequencies Frequency points for FFT (Hz), one per bin; use DFT-aligned grid from
 *                    mom_time_domain_build_dft_aligned_frequencies_hz() so IFFT matches physics
 * @param num_frequencies Number of frequency points
 * @param config Time-domain configuration
 * @param time_results Output: time-domain results
 * @param use_band_mask If non-zero, bins with f not in [band_fmin_hz, band_fmax_hz] are zeroed (no MoM solve)
 * @param band_fmin_hz Lower band edge (Hz), valid when use_band_mask
 * @param band_fmax_hz Upper band edge (Hz), valid when use_band_mask
 * @return 0 on success, negative on error
 */
int mom_solver_solve_time_domain(
    mom_solver_t* solver,
    const double* frequencies,
    int num_frequencies,
    const mom_time_domain_config_t* config,
    mom_time_domain_results_t* time_results,
    int use_band_mask,
    double band_fmin_hz,
    double band_fmax_hz
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

/**
 * Build DFT/IFFT-aligned frequencies (Hz) for N time samples on [time_start, time_stop]:
 * f[k] = k / (N * dt), dt = (time_stop - time_start) / (N - 1), k = 0..N-1.
 * Matches bin k of an N-point inverse DFT with uniform spacing dt.
 * f[0] is 0 Hz (DC); mom_solver_solve_time_domain skips DC for MoM.
 * Caller must free *out_freqs with free().
 */
int mom_time_domain_build_dft_aligned_frequencies_hz(double time_start, double time_stop, int n,
                                                     double** out_freqs);

#ifdef __cplusplus
}
#endif

#endif // MOM_TIME_DOMAIN_H
