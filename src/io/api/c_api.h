/********************************************************************************
 * C API Interface (L6 IO/Workflow/API Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines C API interface.
 * L6 layer: IO/Workflow/API - provides API interface.
 *
 * Architecture Rule: L6 provides API, does NOT change simulation semantics.
 ********************************************************************************/

#ifndef C_API_H
#define C_API_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// API Handle Types
// ============================================================================

/**
 * Simulation Handle (opaque)
 */
typedef struct simulation_handle simulation_handle_t;

/**
 * Solver Handle (opaque)
 */
typedef struct solver_handle solver_handle_t;

// ============================================================================
// C API Interface
// ============================================================================

/**
 * Create simulation handle
 */
simulation_handle_t* api_create_simulation(void);

/**
 * Destroy simulation handle
 */
void api_destroy_simulation(simulation_handle_t* handle);

/**
 * Load geometry
 */
int api_load_geometry(
    simulation_handle_t* handle,
    const char* filename
);

/**
 * Set solver configuration
 */
int api_set_solver_config(
    simulation_handle_t* handle,
    const char* solver_type,
    void* config
);

/**
 * Run simulation
 */
int api_run_simulation(
    simulation_handle_t* handle
);

/**
 * Get results
 */
int api_get_results(
    simulation_handle_t* handle,
    void* results
);

/**
 * Export results
 */
int api_export_results(
    simulation_handle_t* handle,
    const char* filename,
    const char* format
);

#ifdef __cplusplus
}
#endif

#endif // C_API_H
