/******************************************************************************
 * MTL Solver Wideband Frequency Analysis
 * 
 * Implements wideband frequency sweep for MTL solver
 * 
 * Method: Frequency sweep with adaptive sampling
 ******************************************************************************/

#ifndef MTL_WIDEBAND_H
#define MTL_WIDEBAND_H

#include "../../core/mtl_solver.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Wideband Configuration
 ******************************************************************************/

typedef struct {
    double freq_start;                   // Start frequency (Hz)
    double freq_stop;                    // Stop frequency (Hz)
    int num_points;                      // Number of frequency points
    bool use_log_scale;                  // Use logarithmic scale
    bool use_adaptive_sampling;          // Use adaptive frequency sampling
    double min_step;                     // Minimum frequency step (Hz)
    double max_step;                     // Maximum frequency step (Hz)
    double tolerance;                    // Adaptive sampling tolerance
} mtl_wideband_config_t;

/******************************************************************************
 * Wideband Functions
 ******************************************************************************/

/**
 * Perform wideband frequency analysis
 * 
 * @param solver MTL solver
 * @param config Wideband configuration
 * @param results Output: frequency-domain results
 * @return 0 on success, negative on error
 */
int mtl_solver_solve_wideband(
    mtl_solver_t* solver,
    const mtl_wideband_config_t* config,
    mtl_results_t* results
);

/**
 * Get default wideband configuration
 * 
 * @return Default configuration
 */
mtl_wideband_config_t mtl_wideband_get_default_config(void);

#ifdef __cplusplus
}
#endif

#endif // MTL_WIDEBAND_H
