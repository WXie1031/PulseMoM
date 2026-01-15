/**
 * @file mtl_solver_module.h
 * @brief Multi-conductor Transmission Line (MTL) solver module
 * @details Cable simulation, transmission line analysis, and hybrid coupling
 */

#ifndef MTL_SOLVER_MODULE_H
#define MTL_SOLVER_MODULE_H

#include <stdint.h>
#include <stdbool.h>
#include "../../core/core_common.h"
#include "../../core/core_geometry.h"
#include "../../core/core_mesh.h"
#include "../../core/core_kernels.h"
#include "../../core/core_assembler.h"
#include "../../core/core_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

// MTL cable types
typedef enum {
    MTL_CABLE_COAXIAL,          // Coaxial cable
    MTL_CABLE_TWISTED_PAIR,     // Twisted pair
    MTL_CABLE_RIBBON,           // Ribbon cable
    MTL_CABLE_HARNESS,          // Cable harness (KBL format)
    MTL_CABLE_MULTICORE,        // Multicore cable
    MTL_CABEL_SHIELDED_PAIR,    // Shielded twisted pair
    MTL_CABLE_CUSTOM            // User-defined geometry
} mtl_cable_type_t;

// MTL conductor materials
typedef enum {
    MTL_MATERIAL_COPPER,        // Copper
    MTL_MATERIAL_ALUMINUM,      // Aluminum
    MTL_MATERIAL_SILVER,        // Silver
    MTL_MATERIAL_GOLD,          // Gold
    MTL_MATERIAL_STEEL,         // Steel
    MTL_MATERIAL_TIN,           // Tin-plated
    MTL_MATERIAL_CUSTOM         // User-defined properties
} mtl_conductor_material_t;

// MTL dielectric materials
typedef enum {
    MTL_DIELECTRIC_PVC,         // Polyvinyl chloride
    MTL_DIELECTRIC_PE,          // Polyethylene
    MTL_DIELECTRIC_PTFE,        // Teflon
    MTL_DIELECTRIC_RUBBER,      // Rubber
    MTL_DIELECTRIC_XLPE,        // Cross-linked polyethylene
    MTL_DIELECTRIC_CUSTOM       // User-defined properties
} mtl_dielectric_material_t;

// MTL analysis types
typedef enum {
    MTL_ANALYSIS_FREQUENCY,     // Frequency domain
    MTL_ANALYSIS_TIME_DOMAIN,   // Time domain
    MTL_ANALYSIS_SWEEP,         // Parameter sweep
    MTL_ANALYSIS_MONTE_CARLO,   // Statistical analysis
    MTL_ANALYSIS_OPTIMIZATION   // Optimization study
} mtl_analysis_type_t;

// MTL coupling modes for hybrid analysis
typedef enum {
    MTL_COUPLING_NONE,          // Standalone MTL
    MTL_COUPLING_MOM_FIELD,     // Couple to MoM field solver
    MTL_COUPLING_PEEC_CIRCUIT, // Couple to PEEC circuit solver
    MTL_COUPLING_FULL_HYBRID    // Full three-way coupling
} mtl_coupling_mode_t;

// MTL solver configuration
typedef struct {
    // Basic settings
    mtl_analysis_type_t analysis_type;
    mtl_coupling_mode_t coupling_mode;
    
    // Frequency settings
    double freq_start;          // Start frequency (Hz)
    double freq_stop;           // Stop frequency (Hz)
    int freq_points;          // Number of frequency points
    bool adaptive_freq;       // Adaptive frequency sampling
    
    // Physical effects
    bool skin_effect;           // Include skin effect
    bool proximity_effect;      // Include proximity effect
    bool common_mode;           // Include common mode analysis
    bool radiation_loss;        // Include radiation losses
    bool dielectric_loss;       // Include dielectric losses
    
    // Numerical settings
    double tolerance;           // Solution tolerance
    int max_iterations;       // Maximum iterations
    int max_mesh_elements;    // Maximum mesh elements per conductor
    
    // Parallel processing
    int num_threads;          // Number of OpenMP threads
    bool enable_gpu;          // Enable GPU acceleration
    
    // Output options
    bool save_s_parameters;     // Save S-parameters
    bool save_z_parameters;     // Save Z-parameters
    bool save_y_parameters;     // Save Y-parameters
    bool save_currents;       // Save current distributions
    bool save_fields;         // Save electromagnetic fields
    bool export_spice;        // Export SPICE model
    
    // Advanced options
    bool stochastic_placement;  // Enable random cable placement
    int random_seed;          // Random seed for reproducibility
    bool kbl_import_export;   // Enable KBL format support
} mtl_solver_config_t;

// MTL path segment structure for arbitrary routing
typedef struct {
    double start_x, start_y, start_z;     // Start point (m)
    double end_x, end_y, end_z;           // End point (m)
    double length;                        // Segment length (m)
    double* conductor_radii;               // Conductor radii for this segment (m)
    double* insulation_thickness;         // Insulation thicknesses (m)
    mtl_conductor_material_t* materials;   // Conductor materials
    mtl_dielectric_material_t* dielectrics; // Dielectric materials
} mtl_path_segment_t;

// MTL geometry structure with arbitrary routing support
typedef struct {
    int num_conductors;         // Number of conductors
    int num_segments;           // Number of path segments
    mtl_path_segment_t* segments; // Array of path segments
    
    // Legacy support for simple straight cables
    double* conductor_radii;    // Conductor radii (m) - for backward compatibility
    double* insulation_thickness; // Insulation thicknesses (m)
    double* positions_x;      // X positions (m) - for backward compatibility
    double* positions_y;      // Y positions (m) - for backward compatibility
    double* positions_z;      // Z positions (m) - for backward compatibility
    mtl_conductor_material_t* materials; // Conductor materials
    mtl_dielectric_material_t* dielectrics; // Dielectric materials
    
    // Cable geometry parameters
    double total_length;        // Total cable length (m)
    double twist_rate;          // Twists per meter (for twisted pairs)
    double shield_radius;       // Shield radius (for shielded cables)
    
    // Path definition for arbitrary routing
    int num_path_nodes;         // Number of path definition nodes
    double* path_nodes_x;       // Path node X coordinates (m)
    double* path_nodes_y;       // Path node Y coordinates (m)  
    double* path_nodes_z;       // Path node Z coordinates (m)
    int* segment_divisions;     // Divisions per path segment
    
    // Stochastic parameters
    bool use_stochastic;      // Enable stochastic geometry
    double position_variance;   // Position variance for random placement
    double radius_variance;     // Radius variance
    
    // Routing type
    bool use_arbitrary_routing; // Enable arbitrary path routing
} mtl_geometry_t;

// MTL results structure
typedef struct {
    // Frequency domain results
    int num_frequencies;      // Number of frequency points
    double* frequencies;      // Frequency vector (Hz)
    
    // Network parameters
    double complex** s_matrix;  // S-parameters [num_ports x num_ports x num_freq]
    double complex** z_matrix;  // Z-parameters [num_ports x num_ports x num_freq]
    double complex** y_matrix;  // Y-parameters [num_ports x num_ports x num_freq]
    
    // Distributed parameters
    double* r_per_unit;       // Resistance per unit length (Ω/m)
    double* l_per_unit;       // Inductance per unit length (H/m)
    double* c_per_unit;       // Capacitance per unit length (F/m)
    double* g_per_unit;       // Conductance per unit length (S/m)
    
    // Current and voltage distributions
    double complex** currents;  // Current distribution [num_conductors x num_freq]
    double complex** voltages;  // Voltage distribution [num_conductors x num_freq]
    
    // Field quantities
    double complex** e_field;   // Electric field [num_points x num_freq]
    double complex** h_field;   // Magnetic field [num_points x num_freq]
    
    // Special effects
    double* skin_depth;       // Skin depth vs frequency (m)
    double* proximity_factor; // Proximity effect factor
    double* common_mode_current; // Common mode current (A)
    double* transfer_impedance;  // Transfer impedance (Ω/m)
    
    // Performance metrics
    double solve_time;        // Total solution time (s)
    int iterations;           // Number of iterations
    double memory_usage;      // Peak memory usage (MB)
    int num_conductors;       // Number of conductors processed
} mtl_results_t;

// MTL solver handle
typedef struct mtl_solver mtl_solver_t;

// Core MTL solver functions
mtl_solver_t* mtl_solver_create(void);
void mtl_solver_destroy(mtl_solver_t* solver);
int mtl_solver_set_config(mtl_solver_t* solver, const mtl_solver_config_t* config);
int mtl_solver_set_geometry(mtl_solver_t* solver, const mtl_geometry_t* geometry);
int mtl_solver_add_cable(mtl_solver_t* solver, mtl_cable_type_t type, const mtl_geometry_t* geometry);

// Analysis functions
int mtl_solver_analyze(mtl_solver_t* solver);
int mtl_solver_solve_frequency_domain(mtl_solver_t* solver, double frequency);
int mtl_solver_solve_time_domain(mtl_solver_t* solver, double time_start, double time_stop, double time_step);

// Results access
mtl_results_t* mtl_solver_get_results(mtl_solver_t* solver);
void mtl_results_destroy(mtl_results_t* results);
int mtl_results_save_to_file(const mtl_results_t* results, const char* filename);
int mtl_results_export_spice(const mtl_results_t* results, const char* filename);

// Advanced analysis
int mtl_solver_calculate_skin_effect(mtl_solver_t* solver, int conductor_id, double frequency, double* skin_depth);
int mtl_solver_calculate_proximity_effect(mtl_solver_t* solver, int conductor_id, double frequency, double* proximity_factor);
int mtl_solver_calculate_transfer_impedance(mtl_solver_t* solver, int conductor_id, double frequency, double* transfer_impedance);
int mtl_solver_calculate_common_mode(mtl_solver_t* solver, double frequency, double* common_mode_current);

// Hybrid coupling interface
int mtl_solver_enable_coupling(mtl_solver_t* solver, mtl_coupling_mode_t mode);
int mtl_solver_set_mom_solver(mtl_solver_t* solver, void* mom_solver_handle);
int mtl_solver_set_peec_solver(mtl_solver_t* solver, void* peec_solver_handle);
int mtl_solver_update_coupling_matrix(mtl_solver_t* solver);
int mtl_solver_exchange_boundary_conditions(mtl_solver_t* solver);

// KBL format support
int mtl_solver_load_kbl_file(mtl_solver_t* solver, const char* filename);
int mtl_solver_export_kbl_file(const mtl_solver_t* solver, const char* filename);

// Utility functions
void mtl_solver_print_info(const mtl_solver_t* solver);
double mtl_solver_estimate_memory(const mtl_solver_t* solver);
int mtl_solver_validate_geometry(const mtl_geometry_t* geometry);

// Arbitrary routing support
int mtl_geometry_create_path(mtl_geometry_t* geometry, int num_nodes, 
                              const double* x_nodes, const double* y_nodes, const double* z_nodes,
                              const int* segment_divisions);
int mtl_geometry_add_segment(mtl_geometry_t* geometry, 
                             double start_x, double start_y, double start_z,
                             double end_x, double end_y, double end_z,
                             int divisions);
int mtl_geometry_discretize_path(mtl_geometry_t* geometry);
double mtl_geometry_compute_total_length(const mtl_geometry_t* geometry);
int mtl_geometry_get_segment_coordinates(const mtl_geometry_t* geometry, int segment_id,
                                        double* start_coords, double* end_coords, double* direction);

// Path segment analysis
int mtl_solver_analyze_segment(mtl_solver_t* solver, int segment_id, double frequency);
int mtl_solver_compute_segment_parameters(mtl_solver_t* solver, int segment_id,
                                         double* r_per_unit, double* l_per_unit, 
                                         double* c_per_unit, double* g_per_unit);

// Error handling
typedef enum {
    MTL_SUCCESS = 0,
    MTL_ERROR_INVALID_ARGUMENT = -1,
    MTL_ERROR_OUT_OF_MEMORY = -2,
    MTL_ERROR_CONVERGENCE = -3,
    MTL_ERROR_SINGULAR = -4,
    MTL_ERROR_FILE_IO = -5,
    MTL_ERROR_LICENSE = -6,
    MTL_ERROR_COUPLING_FAILED = -7,
    MTL_ERROR_INTERNAL = -99
} mtl_error_t;

const char* mtl_error_string(mtl_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* MTL_SOLVER_MODULE_H */