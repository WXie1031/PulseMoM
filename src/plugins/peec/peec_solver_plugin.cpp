/*********************************************************************
 * PEEC Solver Plugin - Commercial-Grade PEEC-MoM Architecture
 * 
 * This module implements the Partial Element Equivalent Circuit solver
 * as a plugin module with standardized interfaces and advanced circuit
 * extraction capabilities.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "../plugin_framework.h"
#include "../../solvers/peec/peec_solver_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Plugin-specific data structure
typedef struct {
    PeecSolver* peec_solver;
    Configuration config;
    ProblemDefinition current_problem;
    SolverResults current_results;
    PerformanceMetrics metrics;
    ResourceEstimate resource_estimate;
    double progress;
    bool is_running;
    bool should_cancel;
    char error_message[1024];
} PeecPluginData;

// Forward declarations
static int peec_plugin_initialize(PluginInterface* plugin, const Framework* framework);
static int peec_plugin_configure(PluginInterface* plugin, const Configuration* config);
static int peec_plugin_run(PluginInterface* plugin, const ProblemDefinition* problem, SolverResults* results);
static int peec_plugin_cleanup(PluginInterface* plugin);
static int peec_plugin_shutdown(PluginInterface* plugin);

static const PluginInfo* peec_plugin_get_info(PluginInterface* plugin);
static PluginStatus peec_plugin_get_status(PluginInterface* plugin);
static const char* peec_plugin_get_error_string(PluginInterface* plugin);

static SolverCapabilities* peec_plugin_get_capabilities(PluginInterface* plugin);
static int peec_plugin_validate_problem(PluginInterface* plugin, const ProblemDefinition* problem);
static int peec_plugin_estimate_resources(PluginInterface* plugin, const ProblemDefinition* problem, ResourceEstimate* estimate);
static int peec_plugin_get_progress(PluginInterface* plugin, double* progress);
static int peec_plugin_cancel(PluginInterface* plugin);

// Plugin information
static const PluginInfo peec_plugin_info = {
    .name = "PEEC_Solver",
    .version = "1.0.0",
    .author = "PulseMoM Development Team",
    .description = "Partial Element Equivalent Circuit solver with advanced circuit extraction",
    .license = "Commercial",
    .type = PLUGIN_TYPE_SOLVER,
    .api_version = PLUGIN_API_VERSION,
    .plugin_version = 0x010000,
    .solver_caps = NULL,  // Will be set during initialization
    .size_of_config = sizeof(Configuration),
    .size_of_results = sizeof(SolverResults)
};

// Solver capabilities
static const SolverCapabilities peec_solver_capabilities = {
    .flags = SOLVER_CAPABILITY_FREQUENCY_DOMAIN | SOLVER_CAPABILITY_TIME_DOMAIN |
             SOLVER_CAPABILITY_PARALLEL | SOLVER_CAPABILITY_GPU_ACCELERATED |
             SOLVER_CAPABILITY_PARAMETRIC | SOLVER_CAPABILITY_H_MATRIX,
    .max_frequency_points = 50000,
    .max_mesh_elements = 5000000,
    .max_unknowns = 2000000,
    .min_frequency_hz = 1e0,      // 1 Hz
    .max_frequency_hz = 1e12,     // 1 THz
    .min_element_size_m = 1e-6,   // 1 micron
    .max_element_size_m = 1e2,    // 100 m
    .supported_basis_functions = 0x0F,  // Low-order basis functions
    .supported_preconditioners = 0x07,   // Basic preconditioners
    .supported_matrix_formats = 0x03,    // Sparse and dense formats
    .required_features = "AVX2, SSE4.2, OpenMP 4.0+",
    .memory_requirement_gb = 32.0
};

// Plugin interface implementation
static PluginInterface peec_plugin_interface = {
    // Lifecycle functions
    .initialize = peec_plugin_initialize,
    .configure = peec_plugin_configure,
    .run = peec_plugin_run,
    .cleanup = peec_plugin_cleanup,
    .shutdown = peec_plugin_shutdown,
    
    // Information functions
    .get_info = peec_plugin_get_info,
    .get_status = peec_plugin_get_status,
    .get_error_string = peec_plugin_get_error_string,
    
    // Solver-specific functions
    .get_capabilities = peec_plugin_get_capabilities,
    .validate_problem = peec_plugin_validate_problem,
    .estimate_resources = peec_plugin_estimate_resources,
    .get_progress = peec_plugin_get_progress,
    .cancel = peec_plugin_cancel,
    .pause = NULL,  // Not implemented
    .resume = NULL, // Not implemented
    
    // Plugin data
    .plugin_data = NULL,
    .info = peec_plugin_info,
    .status = PLUGIN_STATUS_UNLOADED,
    .type = PLUGIN_TYPE_SOLVER,
    .handle = NULL,
    .path = ""
};

// Plugin implementation functions
static int peec_plugin_initialize(PluginInterface* plugin, const Framework* framework) {
    if (!plugin || !framework) {
        return -1;
    }
    
    PeecPluginData* data = (PeecPluginData*)calloc(1, sizeof(PeecPluginData));
    if (!data) {
        strcpy(plugin->error_message, "Memory allocation failed for PEEC plugin data");
        return -1;
    }
    
    // Initialize PEEC solver
    data->peec_solver = peec_solver_create();
    if (!data->peec_solver) {
        free(data);
        strcpy(plugin->error_message, "Failed to create PEEC solver");
        return -1;
    }
    
    // Set up default configuration
    strcpy(data->config.solver_name, "PEEC_Solver");
    data->config.parallel_threads = 4;
    data->config.use_gpu = 0;
    data->config.use_distributed = 0;
    data->config.use_adaptive = 1;
    data->config.use_preconditioner = 1;
    data->config.use_h_matrix = 1;
    data->config.use_mlfmm = 0;
    data->config.convergence_tolerance = 1e-6;
    data->config.max_iterations = 5000;
    data->config.memory_limit_mb = 8192;  // 8 GB
    data->config.frequency_adaptation = 0.05;
    
    data->progress = 0.0;
    data->is_running = false;
    data->should_cancel = false;
    
    plugin->plugin_data = data;
    plugin->status = PLUGIN_STATUS_INITIALIZED;
    
    return 0;
}

static int peec_plugin_configure(PluginInterface* plugin, const Configuration* config) {
    if (!plugin || !config || !plugin->plugin_data) {
        return -1;
    }
    
    PeecPluginData* data = (PeecPluginData*)plugin->plugin_data;
    
    // Copy configuration
    memcpy(&data->config, config, sizeof(Configuration));
    
    // Apply configuration to PEEC solver
    if (data->peec_solver) {
        // Set solver parameters based on configuration
        peec_solver_set_threads(data->peec_solver, config->parallel_threads);
        peec_solver_set_tolerance(data->peec_solver, config->convergence_tolerance);
        peec_solver_set_max_iterations(data->peec_solver, config->max_iterations);
        peec_solver_enable_gpu(data->peec_solver, config->use_gpu);
        peec_solver_enable_h_matrix(data->peec_solver, config->use_h_matrix);
        
        // Set memory limit
        peec_solver_set_memory_limit(data->peec_solver, config->memory_limit_mb);
    }
    
    return 0;
}

static int peec_plugin_run(PluginInterface* plugin, const ProblemDefinition* problem, SolverResults* results) {
    if (!plugin || !problem || !results || !plugin->plugin_data) {
        return -1;
    }
    
    PeecPluginData* data = (PeecPluginData*)plugin->plugin_data;
    
    // Validate problem
    if (peec_plugin_validate_problem(plugin, problem) != 0) {
        return -1;
    }
    
    // Store current problem
    memcpy(&data->current_problem, problem, sizeof(ProblemDefinition));
    
    // Start timing
    clock_t start_time = clock();
    data->is_running = true;
    data->should_cancel = false;
    data->progress = 0.0;
    
    // Initialize results
    memset(results, 0, sizeof(SolverResults));
    results->status = -1;  // Error status until successful completion
    
    try {
        // Set up geometry and materials
        data->progress = 0.1;
        if (problem->geometry && problem->materials) {
            peec_solver_set_geometry(data->peec_solver, problem->geometry);
            peec_solver_set_materials(data->peec_solver, problem->materials);
        }
        
        // Set up mesh
        data->progress = 0.2;
        if (problem->mesh) {
            peec_solver_set_mesh(data->peec_solver, problem->mesh);
        }
        
        // Set up boundary conditions and excitations
        data->progress = 0.3;
        if (problem->boundaries) {
            peec_solver_set_boundary_conditions(data->peec_solver, problem->boundaries);
        }
        if (problem->excitations) {
            peec_solver_set_excitations(data->peec_solver, problem->excitations);
        }
        
        // Set frequency
        data->progress = 0.4;
        if (problem->num_frequency_points > 0 && problem->frequency_points) {
            peec_solver_set_frequencies(data->peec_solver, problem->frequency_points, problem->num_frequency_points);
        } else {
            peec_solver_set_frequency(data->peec_solver, problem->frequency_hz);
        }
        
        // Extract partial elements
        data->progress = 0.5;
        int extract_status = peec_solver_extract_partial_elements(data->peec_solver);
        
        if (extract_status != 0 || data->should_cancel) {
            strcpy(data->error_message, "PEEC partial element extraction failed or was cancelled");
            data->is_running = false;
            return -1;
        }
        
        // Solve circuit equations
        data->progress = 0.7;
        int solve_status = peec_solver_solve_circuit(data->peec_solver);
        
        if (solve_status != 0 || data->should_cancel) {
            strcpy(data->error_message, "PEEC circuit solution failed or was cancelled");
            data->is_running = false;
            return -1;
        }
        
        // Extract results
        data->progress = 0.9;
        extract_peec_results(data->peec_solver, results);
        
        // Get performance metrics
        peec_solver_get_performance_metrics(data->peec_solver, &data->metrics);
        results->metrics = &data->metrics;
        
        // Finalize
        data->progress = 1.0;
        results->status = 0;  // Success
        
    } catch (...) {
        strcpy(data->error_message, "Exception occurred during PEEC solution");
        data->is_running = false;
        return -1;
    }
    
    // Update timing
    clock_t end_time = clock();
    data->metrics.total_time_sec = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    data->is_running = false;
    memcpy(&data->current_results, results, sizeof(SolverResults));
    
    return 0;
}

static int peec_plugin_cleanup(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) {
        return -1;
    }
    
    PeecPluginData* data = (PeecPluginData*)plugin->plugin_data;
    
    // Clean up PEEC solver
    if (data->peec_solver) {
        peec_solver_destroy(data->peec_solver);
        data->peec_solver = NULL;
    }
    
    // Reset state
    data->is_running = false;
    data->should_cancel = false;
    data->progress = 0.0;
    
    return 0;
}

static int peec_plugin_shutdown(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) {
        return -1;
    }
    
    // Clean up first
    peec_plugin_cleanup(plugin);
    
    // Free plugin data
    free(plugin->plugin_data);
    plugin->plugin_data = NULL;
    plugin->status = PLUGIN_STATUS_UNLOADED;
    
    return 0;
}

static const PluginInfo* peec_plugin_get_info(PluginInterface* plugin) {
    return &peec_plugin_info;
}

static PluginStatus peec_plugin_get_status(PluginInterface* plugin) {
    if (!plugin) return PLUGIN_STATUS_ERROR;
    return plugin->status;
}

static const char* peec_plugin_get_error_string(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) return "Unknown error";
    
    PeecPluginData* data = (PeecPluginData*)plugin->plugin_data;
    return data->error_message;
}

static SolverCapabilities* peec_plugin_get_capabilities(PluginInterface* plugin) {
    return (SolverCapabilities*)&peec_solver_capabilities;
}

static int peec_plugin_validate_problem(PluginInterface* plugin, const ProblemDefinition* problem) {
    if (!plugin || !problem) return -1;
    
    // Check frequency range
    if (problem->frequency_hz < peec_solver_capabilities.min_frequency_hz ||
        problem->frequency_hz > peec_solver_capabilities.max_frequency_hz) {
        strcpy(((PeecPluginData*)plugin->plugin_data)->error_message, 
               "Frequency out of supported range");
        return -1;
    }
    
    // Check mesh elements (PEEC typically handles fewer elements than MoM)
    if (problem->mesh && problem->mesh->num_elements > peec_solver_capabilities.max_mesh_elements) {
        strcpy(((PeecPluginData*)plugin->plugin_data)->error_message, 
               "Too many mesh elements for PEEC solver");
        return -1;
    }
    
    // Check geometry
    if (!problem->geometry) {
        strcpy(((PeecPluginData*)plugin->plugin_data)->error_message, 
               "No geometry data provided");
        return -1;
    }
    
    // PEEC is particularly good for interconnect and package problems
    // Check if problem is suitable for PEEC
    if (problem->problem_type != PROBLEM_TYPE_INTERCONNECT &&
        problem->problem_type != PROBLEM_TYPE_PACKAGE &&
        problem->problem_type != PROBLEM_TYPE_PCBS) {
        // Not an error, but might not be optimal
        printf("Warning: Problem type may not be optimal for PEEC solver\n");
    }
    
    return 0;
}

static int peec_plugin_estimate_resources(PluginInterface* plugin, const ProblemDefinition* problem, ResourceEstimate* estimate) {
    if (!plugin || !problem || !estimate) return -1;
    
    PeecPluginData* data = (PeecPluginData*)plugin->plugin_data;
    
    // Estimate based on problem size (PEEC typically has fewer unknowns than MoM)
    int num_nodes = estimate_nodes(problem);
    
    // Memory estimation for PEEC (circuit matrices are typically sparser)
    double memory_per_node = 8.0;  // bytes per node for circuit matrices
    double matrix_memory = (double)num_nodes * num_nodes * memory_per_node / (1024.0 * 1024.0);  // MB
    double total_memory = matrix_memory * 2.0;  // Factor of 2 for workspace (sparser than MoM)
    
    estimate->memory_required_mb = total_memory;
    estimate->disk_space_required_mb = total_memory * 0.05;  // 5% for temporary files (less than MoM)
    estimate->cpu_cores_recommended = data->config.parallel_threads;
    estimate->gpu_devices_recommended = data->config.use_gpu ? 1 : 0;
    estimate->estimated_time_minutes = estimate_peec_solution_time(num_nodes, data->config.parallel_threads);
    estimate->parallel_efficiency_percent = 80;  // PEEC typically has better parallel efficiency
    
    return 0;
}

static int peec_plugin_get_progress(PluginInterface* plugin, double* progress) {
    if (!plugin || !progress || !plugin->plugin_data) return -1;
    
    PeecPluginData* data = (PeecPluginData*)plugin->plugin_data;
    *progress = data->progress;
    return 0;
}

static int peec_plugin_cancel(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) return -1;
    
    PeecPluginData* data = (PeecPluginData*)plugin->plugin_data;
    data->should_cancel = true;
    
    // Cancel PEEC solver if running
    if (data->peec_solver && data->is_running) {
        peec_solver_cancel(data->peec_solver);
    }
    
    return 0;
}

// Helper functions
static int estimate_nodes(const ProblemDefinition* problem) {
    if (!problem->mesh) return 0;
    
    // For PEEC, we estimate based on nodes rather than elements
    // PEEC creates circuit nodes at conductor intersections
    return (int)(problem->mesh->num_elements * 0.6);  // Rough approximation
}

static double estimate_peec_solution_time(int num_nodes, int num_threads) {
    // PEEC solution time is typically faster than MoM due to sparser matrices
    double base_time = estimate_solution_time(num_nodes, num_threads) * 0.3;  // 30% of MoM time
    return base_time;
}

static void extract_peec_results(PeecSolver* peec_solver, SolverResults* results) {
    if (!peec_solver || !results) return;
    
    // Get circuit parameters
    int num_ports = peec_solver_get_num_ports(peec_solver);
    if (num_ports > 0) {
        results->s_parameters = (Complex*)calloc(num_ports * num_ports, sizeof(Complex));
        peec_solver_get_s_parameters(peec_solver, results->s_parameters);
        
        results->z_parameters = (Complex*)calloc(num_ports * num_ports, sizeof(Complex));
        peec_solver_get_z_parameters(peec_solver, results->z_parameters);
        
        results->y_parameters = (Complex*)calloc(num_ports * num_ports, sizeof(Complex));
        peec_solver_get_y_parameters(peec_solver, results->y_parameters);
    }
    
    // Get node voltages and branch currents
    int num_nodes = peec_solver_get_num_nodes(peec_solver);
    if (num_nodes > 0) {
        results->currents = (double*)calloc(num_nodes * 2, sizeof(double));  // Real and imag
        peec_solver_get_node_voltages(peec_solver, results->currents);
    }
    
    int num_branches = peec_solver_get_num_branches(peec_solver);
    if (num_branches > 0) {
        results->charges = (double*)calloc(num_branches * 2, sizeof(double));  // Real and imag
        peec_solver_get_branch_currents(peec_solver, results->charges);
    }
}

// Plugin export functions
extern "C" {
    PLUGIN_ENTRY_POINT {
        PluginInterface* plugin = (PluginInterface*)calloc(1, sizeof(PluginInterface));
        if (!plugin) return NULL;
        
        // Copy interface template
        memcpy(plugin, &peec_plugin_interface, sizeof(PluginInterface));
        
        // Set plugin info
        plugin->info = peec_plugin_info;
        plugin->info.solver_caps = (SolverCapabilities*)&peec_solver_capabilities;
        
        return plugin;
    }
    
    PLUGIN_DESTROY {
        if (plugin) {
            if (plugin->plugin_data) {
                peec_plugin_shutdown(plugin);
            }
            free(plugin);
        }
    }
}