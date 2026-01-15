/*********************************************************************
 * MoM Solver Plugin - Commercial-Grade PEEC-MoM Architecture
 * 
 * This module implements the Method of Moments solver as a plugin module
 * with standardized interfaces and advanced electromagnetic simulation
 * capabilities.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "../plugin_framework.h"
#include "../../solvers/mom/mom_solver_module.h"
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
    MomSolver* mom_solver;
    Configuration config;
    ProblemDefinition current_problem;
    SolverResults current_results;
    PerformanceMetrics metrics;
    ResourceEstimate resource_estimate;
    double progress;
    bool is_running;
    bool should_cancel;
    char error_message[1024];
} MomPluginData;

// Forward declarations
static int mom_plugin_initialize(PluginInterface* plugin, const Framework* framework);
static int mom_plugin_configure(PluginInterface* plugin, const Configuration* config);
static int mom_plugin_run(PluginInterface* plugin, const ProblemDefinition* problem, SolverResults* results);
static int mom_plugin_cleanup(PluginInterface* plugin);
static int mom_plugin_shutdown(PluginInterface* plugin);

static const PluginInfo* mom_plugin_get_info(PluginInterface* plugin);
static PluginStatus mom_plugin_get_status(PluginInterface* plugin);
static const char* mom_plugin_get_error_string(PluginInterface* plugin);

static SolverCapabilities* mom_plugin_get_capabilities(PluginInterface* plugin);
static int mom_plugin_validate_problem(PluginInterface* plugin, const ProblemDefinition* problem);
static int mom_plugin_estimate_resources(PluginInterface* plugin, const ProblemDefinition* problem, ResourceEstimate* estimate);
static int mom_plugin_get_progress(PluginInterface* plugin, double* progress);
static int mom_plugin_cancel(PluginInterface* plugin);

// Plugin information
static const PluginInfo mom_plugin_info = {
    .name = "MoM_Solver",
    .version = "1.0.0",
    .author = "PulseMoM Development Team",
    .description = "Method of Moments electromagnetic field solver with advanced capabilities",
    .license = "Commercial",
    .type = PLUGIN_TYPE_SOLVER,
    .api_version = PLUGIN_API_VERSION,
    .plugin_version = 0x010000,
    .solver_caps = NULL,  // Will be set during initialization
    .size_of_config = sizeof(Configuration),
    .size_of_results = sizeof(SolverResults)
};

// Solver capabilities
static const SolverCapabilities mom_solver_capabilities = {
    .flags = SOLVER_CAPABILITY_FREQUENCY_DOMAIN | SOLVER_CAPABILITY_PARALLEL | 
             SOLVER_CAPABILITY_GPU_ACCELERATED | SOLVER_CAPABILITY_ADAPTIVE |
             SOLVER_CAPABILITY_H_MATRIX | SOLVER_CAPABILITY_MLFMM | SOLVER_CAPABILITY_HYBRID,
    .max_frequency_points = 10000,
    .max_mesh_elements = 10000000,
    .max_unknowns = 5000000,
    .min_frequency_hz = 1e3,      // 1 kHz
    .max_frequency_hz = 1e15,     // 1 PHz
    .min_element_size_m = 1e-9,   // 1 nm
    .max_element_size_m = 1e3,    // 1 km
    .supported_basis_functions = 0xFF,  // All basis functions
    .supported_preconditioners = 0xFF,   // All preconditioners
    .supported_matrix_formats = 0xFF,    // All matrix formats
    .required_features = "AVX2, SSE4.2, OpenMP 4.0+",
    .memory_requirement_gb = 64.0
};

// Plugin interface implementation
static PluginInterface mom_plugin_interface = {
    // Lifecycle functions
    .initialize = mom_plugin_initialize,
    .configure = mom_plugin_configure,
    .run = mom_plugin_run,
    .cleanup = mom_plugin_cleanup,
    .shutdown = mom_plugin_shutdown,
    
    // Information functions
    .get_info = mom_plugin_get_info,
    .get_status = mom_plugin_get_status,
    .get_error_string = mom_plugin_get_error_string,
    
    // Solver-specific functions
    .get_capabilities = mom_plugin_get_capabilities,
    .validate_problem = mom_plugin_validate_problem,
    .estimate_resources = mom_plugin_estimate_resources,
    .get_progress = mom_plugin_get_progress,
    .cancel = mom_plugin_cancel,
    .pause = NULL,  // Not implemented
    .resume = NULL, // Not implemented
    
    // Plugin data
    .plugin_data = NULL,
    .info = mom_plugin_info,
    .status = PLUGIN_STATUS_UNLOADED,
    .type = PLUGIN_TYPE_SOLVER,
    .handle = NULL,
    .path = ""
};

// Plugin implementation functions
static int mom_plugin_initialize(PluginInterface* plugin, const Framework* framework) {
    if (!plugin || !framework) {
        return -1;
    }
    
    MomPluginData* data = (MomPluginData*)calloc(1, sizeof(MomPluginData));
    if (!data) {
        strcpy(plugin->error_message, "Memory allocation failed for MoM plugin data");
        return -1;
    }
    
    // Initialize MoM solver
    data->mom_solver = mom_solver_create();
    if (!data->mom_solver) {
        free(data);
        strcpy(plugin->error_message, "Failed to create MoM solver");
        return -1;
    }
    
    // Set up default configuration
    strcpy(data->config.solver_name, "MoM_Solver");
    data->config.parallel_threads = 4;
    data->config.use_gpu = 0;
    data->config.use_distributed = 0;
    data->config.use_adaptive = 1;
    data->config.use_preconditioner = 1;
    data->config.use_h_matrix = 1;
    data->config.use_mlfmm = 0;
    data->config.convergence_tolerance = 1e-6;
    data->config.max_iterations = 1000;
    data->config.memory_limit_mb = 16384;  // 16 GB
    data->config.frequency_adaptation = 0.1;
    
    data->progress = 0.0;
    data->is_running = false;
    data->should_cancel = false;
    
    plugin->plugin_data = data;
    plugin->status = PLUGIN_STATUS_INITIALIZED;
    
    return 0;
}

static int mom_plugin_configure(PluginInterface* plugin, const Configuration* config) {
    if (!plugin || !config || !plugin->plugin_data) {
        return -1;
    }
    
    MomPluginData* data = (MomPluginData*)plugin->plugin_data;
    
    // Copy configuration
    memcpy(&data->config, config, sizeof(Configuration));
    
    // Apply configuration to MoM solver
    if (data->mom_solver) {
        // Set solver parameters based on configuration
        mom_solver_set_threads(data->mom_solver, config->parallel_threads);
        mom_solver_set_tolerance(data->mom_solver, config->convergence_tolerance);
        mom_solver_set_max_iterations(data->mom_solver, config->max_iterations);
        mom_solver_enable_gpu(data->mom_solver, config->use_gpu);
        mom_solver_enable_h_matrix(data->mom_solver, config->use_h_matrix);
        mom_solver_enable_mlfmm(data->mom_solver, config->use_mlfmm);
        mom_solver_enable_preconditioner(data->mom_solver, config->use_preconditioner);
        
        // Set memory limit
        mom_solver_set_memory_limit(data->mom_solver, config->memory_limit_mb);
    }
    
    return 0;
}

static int mom_plugin_run(PluginInterface* plugin, const ProblemDefinition* problem, SolverResults* results) {
    if (!plugin || !problem || !results || !plugin->plugin_data) {
        return -1;
    }
    
    MomPluginData* data = (MomPluginData*)plugin->plugin_data;
    
    // Validate problem
    if (mom_plugin_validate_problem(plugin, problem) != 0) {
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
            mom_solver_set_geometry(data->mom_solver, problem->geometry);
            mom_solver_set_materials(data->mom_solver, problem->materials);
        }
        
        // Set up mesh
        data->progress = 0.2;
        if (problem->mesh) {
            mom_solver_set_mesh(data->mom_solver, problem->mesh);
        }
        
        // Set up boundary conditions and excitations
        data->progress = 0.3;
        if (problem->boundaries) {
            mom_solver_set_boundary_conditions(data->mom_solver, problem->boundaries);
        }
        if (problem->excitations) {
            mom_solver_set_excitations(data->mom_solver, problem->excitations);
        }
        
        // Set frequency
        data->progress = 0.4;
        if (problem->num_frequency_points > 0 && problem->frequency_points) {
            mom_solver_set_frequencies(data->mom_solver, problem->frequency_points, problem->num_frequency_points);
        } else {
            mom_solver_set_frequency(data->mom_solver, problem->frequency_hz);
        }
        
        // Solve the problem
        data->progress = 0.5;
        int solve_status = mom_solver_solve(data->mom_solver);
        
        if (solve_status != 0 || data->should_cancel) {
            strcpy(data->error_message, "MoM solver failed or was cancelled");
            data->is_running = false;
            return -1;
        }
        
        // Extract results
        data->progress = 0.8;
        extract_mom_results(data->mom_solver, results);
        
        // Get performance metrics
        data->progress = 0.9;
        mom_solver_get_performance_metrics(data->mom_solver, &data->metrics);
        results->metrics = &data->metrics;
        
        // Finalize
        data->progress = 1.0;
        results->status = 0;  // Success
        
    } catch (...) {
        strcpy(data->error_message, "Exception occurred during MoM solution");
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

static int mom_plugin_cleanup(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) {
        return -1;
    }
    
    MomPluginData* data = (MomPluginData*)plugin->plugin_data;
    
    // Clean up MoM solver
    if (data->mom_solver) {
        mom_solver_destroy(data->mom_solver);
        data->mom_solver = NULL;
    }
    
    // Reset state
    data->is_running = false;
    data->should_cancel = false;
    data->progress = 0.0;
    
    return 0;
}

static int mom_plugin_shutdown(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) {
        return -1;
    }
    
    // Clean up first
    mom_plugin_cleanup(plugin);
    
    // Free plugin data
    free(plugin->plugin_data);
    plugin->plugin_data = NULL;
    plugin->status = PLUGIN_STATUS_UNLOADED;
    
    return 0;
}

static const PluginInfo* mom_plugin_get_info(PluginInterface* plugin) {
    return &mom_plugin_info;
}

static PluginStatus mom_plugin_get_status(PluginInterface* plugin) {
    if (!plugin) return PLUGIN_STATUS_ERROR;
    return plugin->status;
}

static const char* mom_plugin_get_error_string(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) return "Unknown error";
    
    MomPluginData* data = (MomPluginData*)plugin->plugin_data;
    return data->error_message;
}

static SolverCapabilities* mom_plugin_get_capabilities(PluginInterface* plugin) {
    return (SolverCapabilities*)&mom_solver_capabilities;
}

static int mom_plugin_validate_problem(PluginInterface* plugin, const ProblemDefinition* problem) {
    if (!plugin || !problem) return -1;
    
    // Check frequency range
    if (problem->frequency_hz < mom_solver_capabilities.min_frequency_hz ||
        problem->frequency_hz > mom_solver_capabilities.max_frequency_hz) {
        strcpy(((MomPluginData*)plugin->plugin_data)->error_message, 
               "Frequency out of supported range");
        return -1;
    }
    
    // Check mesh elements
    if (problem->mesh && problem->mesh->num_elements > mom_solver_capabilities.max_mesh_elements) {
        strcpy(((MomPluginData*)plugin->plugin_data)->error_message, 
               "Too many mesh elements for MoM solver");
        return -1;
    }
    
    // Check geometry
    if (!problem->geometry) {
        strcpy(((MomPluginData*)plugin->plugin_data)->error_message, 
               "No geometry data provided");
        return -1;
    }
    
    return 0;
}

static int mom_plugin_estimate_resources(PluginInterface* plugin, const ProblemDefinition* problem, ResourceEstimate* estimate) {
    if (!plugin || !problem || !estimate) return -1;
    
    MomPluginData* data = (MomPluginData*)plugin->plugin_data;
    
    // Estimate based on problem size
    int num_unknowns = estimate_unknowns(problem);
    
    // Memory estimation (rough approximation)
    double memory_per_unknown = 16.0;  // bytes per unknown for double complex
    double matrix_memory = (double)num_unknowns * num_unknowns * memory_per_unknown / (1024.0 * 1024.0);  // MB
    double total_memory = matrix_memory * 3.0;  // Factor of 3 for workspace
    
    estimate->memory_required_mb = total_memory;
    estimate->disk_space_required_mb = total_memory * 0.1;  // 10% for temporary files
    estimate->cpu_cores_recommended = data->config.parallel_threads;
    estimate->gpu_devices_recommended = data->config.use_gpu ? 1 : 0;
    estimate->estimated_time_minutes = estimate_solution_time(num_unknowns, data->config.parallel_threads);
    estimate->parallel_efficiency_percent = 75;  // Typical efficiency
    
    return 0;
}

static int mom_plugin_get_progress(PluginInterface* plugin, double* progress) {
    if (!plugin || !progress || !plugin->plugin_data) return -1;
    
    MomPluginData* data = (MomPluginData*)plugin->plugin_data;
    *progress = data->progress;
    return 0;
}

static int mom_plugin_cancel(PluginInterface* plugin) {
    if (!plugin || !plugin->plugin_data) return -1;
    
    MomPluginData* data = (MomPluginData*)plugin->plugin_data;
    data->should_cancel = true;
    
    // Cancel MoM solver if running
    if (data->mom_solver && data->is_running) {
        mom_solver_cancel(data->mom_solver);
    }
    
    return 0;
}

// Helper functions
static int estimate_unknowns(const ProblemDefinition* problem) {
    if (!problem->mesh) return 0;
    
    // Estimate based on mesh elements and basis functions
    int elements_per_wavelength = 10;  // Typical density
    double wavelength = 3e8 / problem->frequency_hz;  // Speed of light / frequency
    double mesh_size = sqrt(problem->mesh->average_element_area);
    
    return (int)(problem->mesh->num_elements * (wavelength / mesh_size) / elements_per_wavelength);
}

static double estimate_solution_time(int num_unknowns, int num_threads) {
    // Rough estimation based on complexity O(N^3) for direct solvers, O(N^2) for iterative
    double complexity_factor = (num_unknowns > 100000) ? 2.0 : 3.0;  // Iterative vs direct
    double base_time = pow(num_unknowns / 1000.0, complexity_factor) / num_threads;
    return base_time / 60.0;  // Convert to minutes
}

static void extract_mom_results(MomSolver* mom_solver, SolverResults* results) {
    if (!mom_solver || !results) return;
    
    // Get S-parameters
    int num_ports = mom_solver_get_num_ports(mom_solver);
    if (num_ports > 0) {
        results->s_parameters = (Complex*)calloc(num_ports * num_ports, sizeof(Complex));
        mom_solver_get_s_parameters(mom_solver, results->s_parameters);
    }
    
    // Get current distribution
    int num_unknowns = mom_solver_get_num_unknowns(mom_solver);
    if (num_unknowns > 0) {
        results->currents = (double*)calloc(num_unknowns * 2, sizeof(double));  // Real and imag
        mom_solver_get_currents(mom_solver, results->currents);
    }
    
    // Get impedance matrix
    if (num_unknowns > 0) {
        results->impedance_matrix = (Complex*)calloc(num_unknowns * num_unknowns, sizeof(Complex));
        mom_solver_get_impedance_matrix(mom_solver, results->impedance_matrix);
    }
}

// Plugin export functions
extern "C" {
    PLUGIN_ENTRY_POINT {
        PluginInterface* plugin = (PluginInterface*)calloc(1, sizeof(PluginInterface));
        if (!plugin) return NULL;
        
        // Copy interface template
        memcpy(plugin, &mom_plugin_interface, sizeof(PluginInterface));
        
        // Set plugin info
        plugin->info = mom_plugin_info;
        plugin->info.solver_caps = (SolverCapabilities*)&mom_solver_capabilities;
        
        return plugin;
    }
    
    PLUGIN_DESTROY {
        if (plugin) {
            if (plugin->plugin_data) {
                mom_plugin_shutdown(plugin);
            }
            free(plugin);
        }
    }
}