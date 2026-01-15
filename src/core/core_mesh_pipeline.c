/**
 * @file core_mesh_pipeline.c
 * @brief Unified mesh generation pipeline implementation
 * 
 * Copyright (c) 2025 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#include "core_mesh_pipeline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Pipeline system state
static bool pipeline_initialized = false;

/*********************************************************************
 * Pipeline Initialization
 *********************************************************************/

int mesh_pipeline_init(void) {
    if (pipeline_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize core mesh system if needed
    // (mesh_unified_init() if it exists)
    
    pipeline_initialized = true;
    return 0;
}

void mesh_pipeline_cleanup(void) {
    if (!pipeline_initialized) {
        return;
    }
    
    pipeline_initialized = false;
}

/*********************************************************************
 * Configuration Validation
 *********************************************************************/

int mesh_pipeline_validate_config(const mesh_pipeline_config_t* config) {
    if (!config) {
        return -1;
    }
    
    // Validate solver type
    if (config->solver < SOLVER_MOM || config->solver > SOLVER_HYBRID) {
        return -2;
    }
    
    // Validate mesh type
    if (config->type < MESH_TYPE_TRIANGULAR || config->type > MESH_TYPE_HYBRID) {
        return -3;
    }
    
    // Validate sizing parameters
    if (config->target_size <= 0.0) {
        return -4;
    }
    
    if (config->min_size > 0.0 && config->min_size >= config->target_size) {
        return -5;
    }
    
    if (config->max_size > 0.0 && config->max_size < config->target_size) {
        return -6;
    }
    
    // Validate quality requirements
    if (config->quality.min_quality < 0.0 || config->quality.min_quality > 1.0) {
        return -7;
    }
    
    if (config->quality.max_aspect_ratio <= 0.0) {
        return -8;
    }
    
    // Validate geometry
    if (!config->geometry) {
        return -9;
    }
    
    return 0; // Valid
}

/*********************************************************************
 * Unified Pipeline Generation
 *********************************************************************/

int mesh_pipeline_generate(const mesh_pipeline_config_t* config, 
                          mesh_pipeline_result_t* result) {
    if (!config || !result) {
        return -1;
    }
    
    // Initialize result
    memset(result, 0, sizeof(mesh_pipeline_result_t));
    result->status = -1;
    
    // Validate configuration
    int validation_status = mesh_pipeline_validate_config(config);
    if (validation_status != 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Invalid configuration: error code %d", validation_status);
        return validation_status;
    }
    
    // Ensure pipeline is initialized
    if (!pipeline_initialized) {
        mesh_pipeline_init();
    }
    
    // Dispatch to solver-specific implementation
    int status = 0;
    switch (config->solver) {
        case SOLVER_MOM:
            status = mesh_pipeline_generate_mom(config, result);
            break;
            
        case SOLVER_PEEC:
            status = mesh_pipeline_generate_peec(config, result);
            break;
            
        case SOLVER_HYBRID:
            // For hybrid, generate mesh suitable for both solvers
            // Default to triangular for hybrid
            {
                mesh_pipeline_config_t mom_config = *config;
                mom_config.solver = SOLVER_MOM;
                mom_config.type = MESH_TYPE_TRIANGULAR;
                status = mesh_pipeline_generate_mom(&mom_config, result);
            }
            break;
            
        default:
            snprintf(result->error_message, sizeof(result->error_message),
                    "Unknown solver type: %d", config->solver);
            return -10;
    }
    
    if (status != 0 && result->error_message[0] == '\0') {
        snprintf(result->error_message, sizeof(result->error_message),
                "Mesh generation failed with status %d", status);
    }
    
    result->status = status;
    
    // Validate generated mesh if requested
    if (status == 0 && config->validate_mesh && result->mesh) {
        // Use mesh validation functions from core_mesh.h
        if (!mesh_validate_topology(result->mesh) || 
            !mesh_validate_geometry(result->mesh)) {
            snprintf(result->error_message, sizeof(result->error_message),
                    "Generated mesh failed validation");
            result->status = -11;
            return -11;
        }
    }
    
    // Export mesh if requested
    // Note: mesh export functions would need to be implemented
    if (status == 0 && config->export_mesh && result->mesh && config->export_filename) {
        // TODO: Implement mesh export
        // mesh_export(result->mesh, config->export_filename);
        (void)config->export_filename;  // Suppress unused parameter warning
    }
    
    return status;
}

/*********************************************************************
 * Result Management
 *********************************************************************/

void mesh_pipeline_result_destroy(mesh_pipeline_result_t* result) {
    if (!result) {
        return;
    }
    
    if (result->mesh) {
        mesh_destroy(result->mesh);
        result->mesh = NULL;
    }
    
    memset(result, 0, sizeof(mesh_pipeline_result_t));
}

/*********************************************************************
 * Recommended Configuration
 *********************************************************************/

int mesh_pipeline_get_recommended_config(solver_type_t solver,
                                        geom_element_type_t geometry_type,
                                        double frequency,
                                        mesh_pipeline_config_t* config) {
    if (!config) {
        return -1;
    }
    
    // Initialize with defaults
    memset(config, 0, sizeof(mesh_pipeline_config_t));
    
    config->solver = solver;
    config->algorithm = MESH_PIPELINE_ALGORITHM_AUTO;
    config->adaptive_refinement = true;
    config->max_refinement_levels = 5;
    config->refinement_threshold = 0.1;
    config->validate_mesh = true;
    
    // Quality requirements
    config->quality.min_quality = 0.3;
    config->quality.max_aspect_ratio = 10.0;
    config->quality.min_angle = 15.0;
    config->quality.max_angle = 150.0;
    config->quality.enforce_quality = true;
    
    // Solver-specific recommendations
    switch (solver) {
        case SOLVER_MOM:
            config->type = MESH_TYPE_TRIANGULAR;
            if (frequency > 0.0) {
                double wavelength = C0 / frequency;
                config->wavelength = wavelength;
                config->target_size = wavelength / 10.0;  // ~λ/10 for MoM
                config->min_size = wavelength / 20.0;
                config->max_size = wavelength / 5.0;
            } else {
                config->target_size = 1e-3;  // Default 1mm
                config->min_size = 0.5e-3;
                config->max_size = 2e-3;
            }
            break;
            
        case SOLVER_PEEC:
            // PEEC can use either Manhattan or Triangular
            // Default to Manhattan for regular PCB geometries
            config->type = MESH_TYPE_MANHATTAN;
            config->target_size = 0.1e-3;  // Default 100 microns
            config->min_size = 1e-6;        // 1 micron minimum
            config->max_size = 10e-3;       // 10 mm maximum
            config->grid_resolution[0] = 0.1e-3;
            config->grid_resolution[1] = 0.1e-3;
            config->grid_resolution[2] = 0.01e-3;  // Thinner in Z for layers
            break;
            
        case SOLVER_HYBRID:
            config->type = MESH_TYPE_HYBRID;
            if (frequency > 0.0) {
                double wavelength = C0 / frequency;
                config->wavelength = wavelength;
                config->target_size = wavelength / 10.0;
            } else {
                config->target_size = 1e-3;
            }
            break;
            
        default:
            return -2;
    }
    
    return 0;
}

/*********************************************************************
 * Helper Functions
 *********************************************************************/

const char* mesh_pipeline_solver_type_to_string(solver_type_t solver) {
    switch (solver) {
        case SOLVER_MOM: return "MoM";
        case SOLVER_PEEC: return "PEEC";
        case SOLVER_HYBRID: return "Hybrid";
        default: return "Unknown";
    }
}

const char* mesh_pipeline_mesh_type_to_string(mesh_type_t type) {
    switch (type) {
        case MESH_TYPE_TRIANGULAR: return "Triangular";
        case MESH_TYPE_MANHATTAN: return "Manhattan";
        case MESH_TYPE_TETRAHEDRAL: return "Tetrahedral";
        case MESH_TYPE_HYBRID: return "Hybrid";
        default: return "Unknown";
    }
}

const char* mesh_pipeline_algorithm_to_string(mesh_pipeline_algorithm_t algorithm) {
    switch (algorithm) {
        case MESH_PIPELINE_ALGORITHM_AUTO: return "Auto";
        case MESH_PIPELINE_ALGORITHM_DELAUNAY: return "Delaunay";
        case MESH_PIPELINE_ALGORITHM_ADVANCING_FRONT: return "Advancing Front";
        case MESH_PIPELINE_ALGORITHM_STRUCTURED_GRID: return "Structured Grid";
        case MESH_PIPELINE_ALGORITHM_ADAPTIVE: return "Adaptive";
        case MESH_PIPELINE_ALGORITHM_CURVATURE_BASED: return "Curvature Based";
        default: return "Unknown";
    }
}
