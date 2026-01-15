/*********************************************************************
 * Plugin Framework - Commercial-Grade PEEC-MoM Unified Architecture
 * 
 * This module implements the plugin architecture for solver modules,
 * providing standardized interfaces and dynamic loading capabilities.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#ifndef PLUGIN_FRAMEWORK_H
#define PLUGIN_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/electromagnetic_kernel_library.h"

#ifdef __cplusplus
extern "C" {
#endif

// Plugin framework version
#define PLUGIN_FRAMEWORK_VERSION_MAJOR 1
#define PLUGIN_FRAMEWORK_VERSION_MINOR 0
#define PLUGIN_FRAMEWORK_VERSION_PATCH 0

// Plugin API version for compatibility checking
#define PLUGIN_API_VERSION 0x010000  // 1.0.0

// Plugin types
typedef enum {
    PLUGIN_TYPE_SOLVER = 0x01,
    PLUGIN_TYPE_PREPROCESSOR = 0x02,
    PLUGIN_TYPE_POSTPROCESSOR = 0x04,
    PLUGIN_TYPE_VISUALIZATION = 0x08,
    PLUGIN_TYPE_IO = 0x10,
    PLUGIN_TYPE_MATERIAL = 0x20,
    PLUGIN_TYPE_MESH_GENERATION = 0x40,
    PLUGIN_TYPE_OPTIMIZATION = 0x80
} PluginType;

// Solver capabilities flags
typedef enum {
    SOLVER_CAPABILITY_FREQUENCY_DOMAIN = 0x01,
    SOLVER_CAPABILITY_TIME_DOMAIN = 0x02,
    SOLVER_CAPABILITY_EIGENVALUE = 0x04,
    SOLVER_CAPABILITY_NONLINEAR = 0x08,
    SOLVER_CAPABILITY_PARAMETRIC = 0x10,
    SOLVER_CAPABILITY_OPTIMIZATION = 0x20,
    SOLVER_CAPABILITY_PARALLEL = 0x40,
    SOLVER_CAPABILITY_GPU_ACCELERATED = 0x80,
    SOLVER_CAPABILITY_DISTRIBUTED = 0x100,
    SOLVER_CAPABILITY_ADAPTIVE = 0x200,
    SOLVER_CAPABILITY_H_MATRIX = 0x400,
    SOLVER_CAPABILITY_MLFMM = 0x800,
    SOLVER_CAPABILITY_HYBRID = 0x1000
} SolverCapability;

// Plugin status
typedef enum {
    PLUGIN_STATUS_UNLOADED = 0,
    PLUGIN_STATUS_LOADED = 1,
    PLUGIN_STATUS_INITIALIZED = 2,
    PLUGIN_STATUS_RUNNING = 3,
    PLUGIN_STATUS_ERROR = -1,
    PLUGIN_STATUS_INCOMPATIBLE = -2
} PluginStatus;

// Forward declarations
typedef struct PluginInfo PluginInfo;
typedef struct PluginInterface PluginInterface;
typedef struct SolverCapabilities SolverCapabilities;
typedef struct ProblemDefinition ProblemDefinition;
typedef struct SolverResults SolverResults;
typedef struct Configuration Configuration;
typedef struct PerformanceMetrics PerformanceMetrics;
typedef struct PluginManager PluginManager;

// Plugin information structure
typedef struct PluginInfo {
    char name[256];
    char version[64];
    char author[256];
    char description[1024];
    char license[256];
    PluginType type;
    uint32_t api_version;
    uint32_t plugin_version;
    SolverCapabilities* solver_caps;
    size_t size_of_config;
    size_t size_of_results;
} PluginInfo;

// Solver capabilities structure
typedef struct SolverCapabilities {
    uint64_t flags;                    // Combination of SolverCapability flags
    int max_frequency_points;          // Maximum frequency points supported
    int max_mesh_elements;             // Maximum mesh elements supported
    int max_unknowns;                  // Maximum unknowns supported
    int min_frequency_hz;              // Minimum frequency in Hz
    int max_frequency_hz;              // Maximum frequency in Hz
    double min_element_size_m;         // Minimum element size in meters
    double max_element_size_m;         // Maximum element size in meters
    int supported_basis_functions;     // Bitmask of supported basis functions
    int supported_preconditioners;     // Bitmask of supported preconditioners
    int supported_matrix_formats;      // Bitmask of supported matrix formats
    char required_features[1024];      // Required CPU/GPU features
    double memory_requirement_gb;      // Typical memory requirement in GB
} SolverCapabilities;

// Problem definition structure
typedef struct ProblemDefinition {
    int problem_type;                  // Problem type identifier
    double frequency_hz;               // Operating frequency
    double* frequency_points;          // Frequency sweep points
    int num_frequency_points;          // Number of frequency points
    GeometryData* geometry;            // Geometry data
    MaterialData* materials;           // Material properties
    BoundaryConditions* boundaries;    // Boundary conditions
    ExcitationData* excitations;     // Sources and excitations
    MeshData* mesh;                    // Mesh data
    SolverParameters* parameters;      // Solver-specific parameters
    void* user_data;                   // User-defined data
    size_t user_data_size;              // Size of user data
} ProblemDefinition;

// Solver results structure
typedef struct SolverResults {
    int status;                          // Solution status
    double* frequency_points;            // Frequency points
    Complex* s_parameters;               // S-parameters
    Complex* z_parameters;               // Z-parameters
    Complex* y_parameters;               // Y-parameters
    double* currents;                    // Current distribution
    double* charges;                     // Charge distribution
    double* fields;                      // Electric/magnetic fields
    double* far_fields;                    // Far-field patterns
    double* radar_cross_section;          // RCS data
    double* eigenvalues;                   // Eigenvalues
    double* eigenvectors;                  // Eigenvectors
    Complex* impedance_matrix;            // Impedance matrix
    Complex* admittance_matrix;           // Admittance matrix
    PerformanceMetrics* metrics;         // Performance metrics
    char* error_message;                  // Error message if failed
    void* additional_data;                // Additional solver-specific data
    size_t additional_data_size;           // Size of additional data
} SolverResults;

// Configuration structure
typedef struct Configuration {
    char solver_name[256];               // Solver name
    char config_file[1024];              // Configuration file path
    int parallel_threads;                // Number of parallel threads
    int use_gpu;                         // Enable GPU acceleration
    int use_distributed;                 // Enable distributed computing
    int use_adaptive;                    // Enable adaptive refinement
    int use_preconditioner;              // Enable preconditioning
    int use_h_matrix;                    // Enable H-matrix compression
    int use_mlfmm;                       // Enable MLFMM acceleration
    double convergence_tolerance;         // Convergence tolerance
    int max_iterations;                   // Maximum iterations
    int memory_limit_mb;                  // Memory limit in MB
    double frequency_adaptation;          // Frequency adaptation parameter
    void* solver_specific;                // Solver-specific configuration
    size_t solver_specific_size;           // Size of solver-specific config
} Configuration;

// Performance metrics structure
typedef struct PerformanceMetrics {
    double setup_time_sec;               // Problem setup time
    double assembly_time_sec;            // Matrix assembly time
    double solve_time_sec;                 // Solution time
    double total_time_sec;                 // Total time
    double memory_peak_mb;                 // Peak memory usage
    double memory_average_mb;              // Average memory usage
    int iterations;                        // Number of iterations
    double residual_norm;                  // Final residual norm
    double condition_number;               // Matrix condition number
    int matrix_size;                       // Matrix size
    int num_threads;                       // Number of threads used
    int gpu_devices;                       // Number of GPU devices used
    double cpu_utilization_percent;        // CPU utilization
    double gpu_utilization_percent;        // GPU utilization
    double parallel_efficiency;            // Parallel efficiency
} PerformanceMetrics;

// Plugin interface - main plugin structure
typedef struct PluginInterface {
    // Plugin lifecycle functions
    int (*initialize)(PluginInterface* plugin, const Framework* framework);
    int (*configure)(PluginInterface* plugin, const Configuration* config);
    int (*run)(PluginInterface* plugin, const ProblemDefinition* problem, SolverResults* results);
    int (*cleanup)(PluginInterface* plugin);
    int (*shutdown)(PluginInterface* plugin);
    
    // Plugin information
    const PluginInfo* (*get_info)(PluginInterface* plugin);
    PluginStatus (*get_status)(PluginInterface* plugin);
    const char* (*get_error_string)(PluginInterface* plugin);
    
    // Solver-specific functions (for solver plugins)
    SolverCapabilities* (*get_capabilities)(PluginInterface* plugin);
    int (*validate_problem)(PluginInterface* plugin, const ProblemDefinition* problem);
    int (*estimate_resources)(PluginInterface* plugin, const ProblemDefinition* problem, ResourceEstimate* estimate);
    int (*get_progress)(PluginInterface* plugin, double* progress);
    int (*cancel)(PluginInterface* plugin);
    int (*pause)(PluginInterface* plugin);
    int (*resume)(PluginInterface* plugin);
    
    // Plugin data
    void* plugin_data;                      // Plugin-specific data
    PluginInfo info;                        // Plugin information
    PluginStatus status;                    // Current status
    PluginType type;                        // Plugin type
    void* handle;                           // Dynamic library handle
    char path[1024];                        // Plugin file path
} PluginInterface;

// Resource estimation structure
typedef struct ResourceEstimate {
    double memory_required_mb;              // Memory requirement in MB
    double disk_space_required_mb;          // Disk space requirement in MB
    int cpu_cores_recommended;              // Recommended CPU cores
    int gpu_devices_recommended;              // Recommended GPU devices
    double estimated_time_minutes;          // Estimated solution time
    int parallel_efficiency_percent;          // Expected parallel efficiency
} ResourceEstimate;

// Plugin manager structure
typedef struct PluginManager {
    PluginInterface** plugins;              // Array of loaded plugins
    int num_plugins;                        // Number of loaded plugins
    int max_plugins;                        // Maximum plugins capacity
    Framework* framework;                   // Reference to core framework
    char plugin_directory[1024];              // Plugin directory path
    int auto_load_plugins;                  // Auto-load plugins on startup
    void* (*load_plugin)(PluginManager* manager, const char* plugin_path);
    int (*unload_plugin)(PluginManager* manager, const char* plugin_name);
    PluginInterface* (*get_plugin)(PluginManager* manager, const char* plugin_name);
    int (*register_plugin)(PluginManager* manager, PluginInterface* plugin);
    int (*unregister_plugin)(PluginManager* manager, const char* plugin_name);
    void (*list_plugins)(PluginManager* manager);
    int (*validate_plugin)(PluginManager* manager, const char* plugin_path);
} PluginManager;

// Plugin API macros for easy plugin development
#define PLUGIN_EXPORT extern "C" __declspec(dllexport)
#define PLUGIN_ENTRY_POINT extern "C" __declspec(dllexport) PluginInterface* create_plugin_interface()
#define PLUGIN_DESTROY extern "C" __declspec(dllexport) void destroy_plugin_interface(PluginInterface* plugin)

// Standard plugin creation macro
#define DEFINE_PLUGIN(PluginClass, plugin_name, plugin_version, plugin_type) \
    PLUGIN_ENTRY_POINT { \
        PluginClass* plugin = new PluginClass(); \
        strcpy(plugin->info.name, plugin_name); \
        strcpy(plugin->info.version, plugin_version); \
        plugin->info.type = plugin_type; \
        plugin->info.api_version = PLUGIN_API_VERSION; \
        return plugin; \
    } \
    PLUGIN_DESTROY { \
        if (plugin) { \
            delete plugin; \
        } \
    }

// Plugin framework initialization and management
PluginManager* plugin_manager_create(Framework* framework);
void plugin_manager_destroy(PluginManager* manager);

int plugin_manager_load_directory(PluginManager* manager, const char* directory);
int plugin_manager_load_plugin(PluginManager* manager, const char* plugin_path);
int plugin_manager_unload_plugin(PluginManager* manager, const char* plugin_name);

PluginInterface* plugin_manager_get_plugin(PluginManager* manager, const char* plugin_name);
int plugin_manager_register_plugin(PluginManager* manager, PluginInterface* plugin);
int plugin_manager_unregister_plugin(PluginManager* manager, const char* plugin_name);

void plugin_manager_list_plugins(PluginManager* manager);
int plugin_manager_validate_plugin(PluginManager* manager, const char* plugin_path);

// Plugin utility functions
const char* plugin_get_type_string(PluginType type);
const char* plugin_get_status_string(PluginStatus status);
int plugin_check_compatibility(const PluginInfo* info);

// Error handling
const char* plugin_get_error_string(int error_code);
int plugin_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // PLUGIN_FRAMEWORK_H