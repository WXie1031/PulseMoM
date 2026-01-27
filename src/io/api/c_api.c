/********************************************************************************
 * C API Implementation (L6 IO/Workflow/API Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements C API interface.
 * L6 layer: IO/Workflow/API - provides API interface.
 *
 * Architecture Rule: L6 provides API, does NOT change simulation semantics.
 ********************************************************************************/

#include "c_api.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../../orchestration/workflow/workflow_engine.h"
#include "../../io/file_formats/file_io.h"
#include <stdlib.h>
#include <string.h>

// Simulation handle structure
struct simulation_handle {
    workflow_engine_t* workflow;
    void* geometry_data;
    void* solver_config;
    void* results_data;
    bool is_initialized;
};

simulation_handle_t* api_create_simulation(void) {
    simulation_handle_t* handle = (simulation_handle_t*)calloc(1, sizeof(simulation_handle_t));
    if (!handle) return NULL;
    
    // Create workflow engine
    handle->workflow = workflow_engine_create();
    if (!handle->workflow) {
        free(handle);
        return NULL;
    }
    
    handle->is_initialized = false;
    
    return handle;
}

void api_destroy_simulation(simulation_handle_t* handle) {
    if (!handle) return;
    
    if (handle->workflow) {
        workflow_engine_destroy(handle->workflow);
    }
    
    // Free data (would need proper cleanup based on data types)
    if (handle->geometry_data) {
        // Free geometry data
    }
    
    if (handle->solver_config) {
        // Free solver config
    }
    
    if (handle->results_data) {
        // Free results data
    }
    
    free(handle);
}

int api_load_geometry(
    simulation_handle_t* handle,
    const char* filename) {
    
    if (!handle || !filename) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L6 layer provides API, delegates to L6 file I/O
    input_file_format_t format = file_io_detect_format(filename);
    if (format == 0) {
        return STATUS_ERROR_INVALID_INPUT;  // Unknown format
    }
    
    // Allocate geometry data (simplified)
    // In full implementation, would allocate proper geometry structure
    handle->geometry_data = malloc(sizeof(void*));  // Placeholder
    
    // Read geometry file
    status_t status = file_io_read_geometry(
        filename,
        format,
        handle->geometry_data
    );
    
    if (status != STATUS_SUCCESS) {
        if (handle->geometry_data) {
            free(handle->geometry_data);
            handle->geometry_data = NULL;
        }
        return status;
    }
    
    // Add geometry import step to workflow
    status = workflow_engine_add_step(
        handle->workflow,
        WORKFLOW_STEP_GEOMETRY_IMPORT,
        handle->geometry_data
    );
    
    return status;
}

int api_set_solver_config(
    simulation_handle_t* handle,
    const char* solver_type,
    void* config) {
    
    if (!handle || !solver_type || !config) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L6 layer stores config, does NOT interpret it
    handle->solver_config = config;
    
    return STATUS_SUCCESS;
}

int api_run_simulation(
    simulation_handle_t* handle) {
    
    if (!handle) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (!handle->geometry_data) {
        return STATUS_ERROR_INVALID_STATE;  // Geometry not loaded
    }
    
    // L6 layer provides API, delegates to L5 workflow engine
    // Add workflow steps if not already added
    if (!handle->is_initialized) {
        // Add mesh generation step
        workflow_engine_add_step(
            handle->workflow,
            WORKFLOW_STEP_MESH_GENERATION,
            NULL
        );
        
        // Add matrix assembly step
        workflow_engine_add_step(
            handle->workflow,
            WORKFLOW_STEP_MATRIX_ASSEMBLY,
            handle->solver_config
        );
        
        // Add solution step
        workflow_engine_add_step(
            handle->workflow,
            WORKFLOW_STEP_SOLUTION,
            handle->solver_config
        );
        
        // Add post-processing step
        workflow_engine_add_step(
            handle->workflow,
            WORKFLOW_STEP_POSTPROCESSING,
            NULL
        );
        
        handle->is_initialized = true;
    }
    
    // Execute workflow
    return workflow_engine_execute(handle->workflow);
}

int api_get_results(
    simulation_handle_t* handle,
    void* results) {
    
    if (!handle || !results) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (!handle->results_data) {
        return STATUS_ERROR_INVALID_STATE;  // Results not available
    }
    
    // L6 layer provides API, does NOT change simulation semantics
    // Copy results (simplified)
    // In full implementation, would copy proper results structure
    memcpy(results, handle->results_data, sizeof(void*));
    
    return STATUS_SUCCESS;
}

int api_export_results(
    simulation_handle_t* handle,
    const char* filename,
    const char* format_str) {
    
    if (!handle || !filename || !format_str) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    if (!handle->results_data) {
        return STATUS_ERROR_INVALID_STATE;  // Results not available
    }
    
    // L6 layer provides API, delegates to L6 file I/O
    // Determine output format
    output_file_format_t format;
    if (strcmp(format_str, "touchstone") == 0 || strcmp(format_str, "s2p") == 0) {
        format = OUTPUT_FORMAT_TOUCHSTONE;
    } else if (strcmp(format_str, "csv") == 0) {
        format = OUTPUT_FORMAT_CSV;
    } else if (strcmp(format_str, "json") == 0) {
        format = OUTPUT_FORMAT_JSON;
    } else if (strcmp(format_str, "hdf5") == 0) {
        format = OUTPUT_FORMAT_HDF5;
    } else if (strcmp(format_str, "vtk") == 0) {
        format = OUTPUT_FORMAT_VTK;
    } else if (strcmp(format_str, "spice") == 0) {
        format = OUTPUT_FORMAT_SPICE;
    } else {
        return STATUS_ERROR_INVALID_INPUT;  // Unknown format
    }
    
    // Write results file
    return file_io_write_results(
        filename,
        format,
        handle->results_data
    );
}
