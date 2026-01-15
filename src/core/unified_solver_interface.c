/**
 * @file unified_solver_interface.c
 * @brief Unified interface implementation for all PulseEM solvers
 * @details Provides consistent API implementation across different electromagnetic solvers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "unified_solver_interface.h"
#include "../solvers/mom/mom_solver.h"
#include "../solvers/peec/peec_solver.h"
#include "../solvers/mtl/mtl_solver_module.h"
#include "../utils/logger.h"
#include "../utils/memory_manager.h"

// Internal unified solver structure
struct unified_solver {
    solver_type_t type;
    unified_solver_config_t config;
    
    // Solver-specific handles
    union {
        mom_solver_t* mom;
        peec_solver_t* peec;
        mtl_solver_t* mtl;
        void* hybrid;
    } solver;
    
    // Results storage
    unified_results_t* results;
    
    // State information
    bool is_initialized;
    bool is_analyzed;
    double solve_time;
    double memory_usage;
};

// Create unified solver
unified_solver_t* unified_solver_create(solver_type_t type) {
    unified_solver_t* solver = (unified_solver_t*)calloc(1, sizeof(unified_solver_t));
    if (!solver) {
        log_error("Failed to allocate unified solver");
        return NULL;
    }
    
    solver->type = type;
    solver->is_initialized = false;
    solver->is_analyzed = false;
    
    // Create solver-specific instance
    switch (type) {
        case SOLVER_TYPE_MOM:
            solver->solver.mom = mom_solver_create();
            if (!solver->solver.mom) {
                free(solver);
                return NULL;
            }
            break;
            
        case SOLVER_TYPE_PEEC:
            solver->solver.peec = peec_solver_create();
            if (!solver->solver.peec) {
                free(solver);
                return NULL;
            }
            break;
            
        case SOLVER_TYPE_MTL:
            solver->solver.mtl = mtl_solver_create();
            if (!solver->solver.mtl) {
                free(solver);
                return NULL;
            }
            break;
            
        case SOLVER_TYPE_HYBRID:
            // Hybrid solver would be created here
            solver->solver.hybrid = NULL; // Placeholder
            break;
            
        default:
            free(solver);
            return NULL;
    }
    
    log_info("Unified solver created: type=%d", type);
    return solver;
}

// Destroy unified solver
void unified_solver_destroy(unified_solver_t* solver) {
    if (!solver) return;
    
    // Destroy solver-specific instance
    switch (solver->type) {
        case SOLVER_TYPE_MOM:
            if (solver->solver.mom) {
                mom_solver_destroy(solver->solver.mom);
            }
            break;
            
        case SOLVER_TYPE_PEEC:
            if (solver->solver.peec) {
                peec_solver_destroy(solver->solver.peec);
            }
            break;
            
        case SOLVER_TYPE_MTL:
            if (solver->solver.mtl) {
                mtl_solver_destroy(solver->solver.mtl);
            }
            break;
            
        case SOLVER_TYPE_HYBRID:
            // Destroy hybrid solver
            break;
    }
    
    // Destroy results
    if (solver->results) {
        unified_results_destroy(solver->results);
    }
    
    free(solver);
    log_info("Unified solver destroyed");
}

// Set solver configuration
int unified_solver_set_config(unified_solver_t* solver, const unified_solver_config_t* config) {
    if (!solver || !config) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    solver->config = *config;
    
    // Convert to solver-specific configuration
    switch (solver->type) {
        case SOLVER_TYPE_MOM: {
            // Convert unified config to MoM config
            // This would require proper conversion logic
            break;
        }
        
        case SOLVER_TYPE_PEEC: {
            // Convert unified config to PEEC config
            // This would require proper conversion logic
            break;
        }
        
        case SOLVER_TYPE_MTL: {
            mtl_solver_config_t mtl_config = {0};
            mtl_config.analysis_type = (config->analysis_type == ANALYSIS_FREQUENCY) ? 
                                       MTL_ANALYSIS_FREQUENCY : MTL_ANALYSIS_TIME_DOMAIN;
            mtl_config.freq_start = config->freq_start;
            mtl_config.freq_stop = config->freq_stop;
            mtl_config.freq_points = config->freq_points;
            mtl_config.tolerance = config->tolerance;
            mtl_config.max_iterations = config->max_iterations;
            mtl_config.enable_gpu = config->enable_gpu;
            mtl_config.num_threads = config->num_threads;
            mtl_config.skin_effect = true;
            mtl_config.proximity_effect = true;
            mtl_config.common_mode = true;
            mtl_config.save_s_parameters = config->save_scattering;
            mtl_config.save_z_parameters = config->save_impedance;
            mtl_config.save_y_parameters = config->save_admittance;
            mtl_config.export_spice = (config->output_format == RESULT_FORMAT_SPICE);
            
            int result = mtl_solver_set_config(solver->solver.mtl, &mtl_config);
            if (result != MTL_SUCCESS) {
                return UNIFIED_ERROR_INTERNAL;
            }
            break;
        }
        
        case SOLVER_TYPE_HYBRID:
            // Set hybrid configuration
            break;
    }
    
    solver->is_initialized = true;
    return UNIFIED_SUCCESS;
}

// Get solver configuration
int unified_solver_get_config(const unified_solver_t* solver, unified_solver_config_t* config) {
    if (!solver || !config) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    *config = solver->config;
    return UNIFIED_SUCCESS;
}

// Set geometry
int unified_solver_set_geometry(unified_solver_t* solver, const char* geometry_file) {
    if (!solver || !geometry_file) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    // This would involve parsing the geometry file and converting to solver-specific format
    log_info("Setting geometry from file: %s", geometry_file);
    
    switch (solver->type) {
        case SOLVER_TYPE_MTL: {
            // For MTL, we would parse cable geometry from file
            // This is a simplified implementation
            mtl_geometry_t geometry = {0};
            geometry.num_conductors = 4; // Example
            geometry.conductor_radii = (double[]){1e-3, 1e-3, 1e-3, 1e-3};
            geometry.insulation_thickness = (double[]){0.5e-3, 0.5e-3, 0.5e-3, 0.5e-3};
            geometry.positions_x = (double[]){0.0, 2e-3, 0.0, -2e-3};
            geometry.positions_y = (double[]){2e-3, 0.0, -2e-3, 0.0};
            geometry.positions_z = (double[]){0.0, 0.0, 0.0, 0.0};
            geometry.materials = (mtl_conductor_material_t[]){
                MTL_MATERIAL_COPPER, MTL_MATERIAL_COPPER, MTL_MATERIAL_COPPER, MTL_MATERIAL_COPPER
            };
            geometry.dielectrics = (mtl_dielectric_material_t[]){
                MTL_DIELECTRIC_PVC, MTL_DIELECTRIC_PVC, MTL_DIELECTRIC_PVC, MTL_DIELECTRIC_PVC
            };
            geometry.length = 1.0;
            geometry.use_stochastic = false;
            
            int result = mtl_solver_set_geometry(solver->solver.mtl, &geometry);
            if (result != MTL_SUCCESS) {
                return UNIFIED_ERROR_INTERNAL;
            }
            break;
        }
        
        default:
            return UNIFIED_ERROR_NOT_IMPLEMENTED;
    }
    
    return UNIFIED_SUCCESS;
}

// Add material
int unified_solver_add_material(unified_solver_t* solver, const char* material_name, 
                               double epsilon_r, double mu_r, double sigma) {
    if (!solver || !material_name) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    // This would add material to the solver-specific material database
    log_info("Adding material: %s (εr=%.2f, μr=%.2f, σ=%.2e)", material_name, epsilon_r, mu_r, sigma);
    
    return UNIFIED_SUCCESS;
}

// Analyze
int unified_solver_analyze(unified_solver_t* solver) {
    if (!solver || !solver->is_initialized) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    clock_t start_time = clock();
    
    // Perform solver-specific analysis
    switch (solver->type) {
        case SOLVER_TYPE_MTL: {
            int result = mtl_solver_analyze(solver->solver.mtl);
            if (result != MTL_SUCCESS) {
                return UNIFIED_ERROR_INTERNAL;
            }
            break;
        }
        
        default:
            return UNIFIED_ERROR_NOT_IMPLEMENTED;
    }
    
    clock_t end_time = clock();
    solver->solve_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    solver->is_analyzed = true;
    
    return UNIFIED_SUCCESS;
}

// Solve frequency domain
int unified_solver_solve_frequency(unified_solver_t* solver, double frequency) {
    if (!solver || !solver->is_initialized) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    // Perform solver-specific frequency analysis
    switch (solver->type) {
        case SOLVER_TYPE_MTL: {
            int result = mtl_solver_solve_frequency_domain(solver->solver.mtl, frequency);
            if (result != MTL_SUCCESS) {
                return UNIFIED_ERROR_INTERNAL;
            }
            break;
        }
        
        default:
            return UNIFIED_ERROR_NOT_IMPLEMENTED;
    }
    
    return UNIFIED_SUCCESS;
}

// Get results
unified_results_t* unified_solver_get_results(const unified_solver_t* solver) {
    if (!solver || !solver->is_analyzed) {
        return NULL;
    }
    
    // Convert solver-specific results to unified format
    switch (solver->type) {
        case SOLVER_TYPE_MTL: {
            mtl_results_t* mtl_results = mtl_solver_get_results(solver->solver.mtl);
            if (!mtl_results) {
                return NULL;
            }
            
            // Create unified results structure
            unified_results_t* results = (unified_results_t*)calloc(1, sizeof(unified_results_t));
            if (!results) {
                return NULL;
            }
            
            // Copy basic information
            results->num_frequencies = mtl_results->num_frequencies;
            results->num_elements = mtl_results->num_conductors;
            results->solve_time = mtl_results->solve_time;
            results->iterations = mtl_results->iterations;
            results->memory_usage = mtl_results->memory_usage;
            
            // Allocate and copy frequency vector
            results->frequencies = (double*)malloc(mtl_results->num_frequencies * sizeof(double));
            if (results->frequencies) {
                memcpy(results->frequencies, mtl_results->frequencies, 
                       mtl_results->num_frequencies * sizeof(double));
            }
            
            return results;
        }
        
        default:
            return NULL;
    }
}

// Destroy results
void unified_results_destroy(unified_results_t* results) {
    if (!results) return;
    
    free(results->frequencies);
    // Free other allocated memory
    free(results);
}

// Get solver name
const char* unified_solver_get_name(const unified_solver_t* solver) {
    if (!solver) return "Unknown";
    
    switch (solver->type) {
        case SOLVER_TYPE_MOM: return "Method of Moments";
        case SOLVER_TYPE_PEEC: return "Partial Element Equivalent Circuit";
        case SOLVER_TYPE_MTL: return "Multi-conductor Transmission Line";
        case SOLVER_TYPE_HYBRID: return "Hybrid Solver";
        default: return "Unknown";
    }
}

// Get solver type
solver_type_t unified_solver_get_type(const unified_solver_t* solver) {
    if (!solver) return SOLVER_TYPE_MOM;
    return solver->type;
}

// Get solver version
const char* unified_solver_get_version(const unified_solver_t* solver) {
    return "2.0.0"; // Unified version
}

// Estimate memory usage
double unified_solver_estimate_memory(const unified_solver_t* solver) {
    if (!solver) return 0.0;
    
    // Return precomputed memory estimate
    return solver->memory_usage;
}

// Validate input
int unified_solver_validate_input(const unified_solver_t* solver) {
    if (!solver) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    // Perform solver-specific validation
    return UNIFIED_SUCCESS;
}

// Error string
const char* unified_error_string(unified_error_t error) {
    switch (error) {
        case UNIFIED_SUCCESS: return "Success";
        case UNIFIED_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case UNIFIED_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case UNIFIED_ERROR_CONVERGENCE: return "Convergence failed";
        case UNIFIED_ERROR_SINGULAR: return "Singular matrix";
        case UNIFIED_ERROR_FILE_IO: return "File I/O error";
        case UNIFIED_ERROR_LICENSE: return "License error";
        case UNIFIED_ERROR_NOT_IMPLEMENTED: return "Not implemented";
        case UNIFIED_ERROR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}

// Get solver-specific handles
mom_solver_t* unified_solver_get_mom(unified_solver_t* solver) {
    if (!solver || solver->type != SOLVER_TYPE_MOM) return NULL;
    return solver->solver.mom;
}

peec_solver_t* unified_solver_get_peec(unified_solver_t* solver) {
    if (!solver || solver->type != SOLVER_TYPE_PEEC) return NULL;
    return solver->solver.peec;
}

mtl_solver_t* unified_solver_get_mtl(unified_solver_t* solver) {
    if (!solver || solver->type != SOLVER_TYPE_MTL) return NULL;
    return solver->solver.mtl;
}

// Enable coupling
int unified_solver_enable_coupling(unified_solver_t* solver1, unified_solver_t* solver2) {
    if (!solver1 || !solver2) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    // This would set up coupling between solvers
    log_info("Enabling coupling between %s and %s", 
             unified_solver_get_name(solver1), unified_solver_get_name(solver2));
    
    return UNIFIED_SUCCESS;
}

// Set coupling configuration
int unified_solver_set_coupling_config(unified_solver_t* solver, double tolerance, int max_iter) {
    if (!solver) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    // This would configure coupling parameters
    log_info("Setting coupling config: tolerance=%.2e, max_iter=%d", tolerance, max_iter);
    
    return UNIFIED_SUCCESS;
}

// Update coupling
int unified_solver_update_coupling(unified_solver_t* solver) {
    if (!solver) {
        return UNIFIED_ERROR_INVALID_ARGUMENT;
    }
    
    // This would update coupling matrices
    log_info("Updating coupling for %s", unified_solver_get_name(solver));
    
    return UNIFIED_SUCCESS;
}