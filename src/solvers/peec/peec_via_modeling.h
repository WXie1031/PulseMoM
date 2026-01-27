/*****************************************************************************************
 * PEEC Via Modeling - Enhanced Via Parasitic Parameter Calculation
 * 
 * Copyright (C) 2025 PulseEM Technologies
 * 
 * File: peec_via_modeling.h
 * Description: Precise via modeling for PCB applications with parasitic parameter extraction
 * 
 * Features:
 * - Precise cylindrical via modeling
 * - Accurate parasitic parameter calculation (R, L, C, G)
 * - High-frequency effects (skin effect, proximity effect)
 * - Via-to-via coupling
 * - Via-to-plane coupling
 * - Support for through-hole, blind, buried, and microvia
 * - Integration with triangular mesh
 * - WGF support for layered media
 *****************************************************************************************/

#ifndef PEEC_VIA_MODELING_H
#define PEEC_VIA_MODELING_H

#include "../../common/core_common.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"  // For mesh_t type (must be included before using mesh_t)
#include "../../operators/greens/layered_greens_function.h"
#include <stdbool.h>

// Forward declaration if needed (mesh_t is defined in core_mesh.h)
// typedef struct mesh_t mesh_t;  // Not needed, already defined in core_mesh.h

#ifdef __cplusplus
extern "C" {
#endif

// Via types
typedef enum {
    VIA_TYPE_THROUGH_HOLE = 0,  // Through-hole via (connects all layers)
    VIA_TYPE_BLIND = 1,         // Blind via (from surface to inner layer)
    VIA_TYPE_BURIED = 2,        // Buried via (between inner layers)
    VIA_TYPE_MICROVIA = 3       // Microvia (small via, typically < 150um)
} via_type_t;

// Via material properties
typedef struct {
    double conductivity;        // Electrical conductivity (S/m)
    double resistivity;         // Electrical resistivity (Ohm*m)
    double permeability;        // Magnetic permeability (H/m)
    double permittivity;        // Electrical permittivity (F/m)
    char name[64];             // Material name (e.g., "Copper", "Tungsten")
} via_material_t;

// Enhanced via structure with parasitic parameters
typedef struct {
    // Geometry
    double center[3];           // Via center coordinates (x, y, z)
    double radius;              // Via radius (m)
    double height;              // Via height (m) - distance between start and end layers
    int start_layer;            // Starting layer index
    int end_layer;              // Ending layer index
    via_type_t type;            // Via type
    
    // Material
    via_material_t material;     // Via material properties
    
    // Mesh connectivity
    int top_vertex;             // Top vertex index in mesh
    int bottom_vertex;          // Bottom vertex index in mesh
    int num_segments;           // Number of cylindrical segments for precise modeling
    int* segment_indices;       // Indices of mesh segments representing via
    
    // Parasitic parameters (computed)
    double resistance;          // DC resistance (Ohms)
    double inductance;           // Self inductance (H)
    double capacitance;          // Self capacitance (F)
    double conductance;          // Self conductance (S)
    
    // Frequency-dependent parameters
    double* resistance_freq;    // Frequency-dependent resistance (array)
    double* inductance_freq;     // Frequency-dependent inductance (array)
    double* capacitance_freq;   // Frequency-dependent capacitance (array)
    double* frequencies;        // Frequency points (Hz)
    int num_freq_points;        // Number of frequency points
    
    // Coupling parameters (to other vias and planes)
    int num_coupled_vias;       // Number of coupled vias
    int* coupled_via_indices;   // Indices of coupled vias
    double* mutual_inductance;  // Mutual inductance to coupled vias (H)
    double* mutual_capacitance; // Mutual capacitance to coupled vias (F)
    
    // High-frequency effects
    double skin_depth;          // Skin depth at current frequency (m)
    double effective_resistance; // Effective resistance including skin effect (Ohms)
    double proximity_factor;     // Proximity effect factor
    
    // Flags
    bool computed;               // Whether parasitic parameters have been computed
    bool use_skin_effect;        // Enable skin effect calculation
    bool use_proximity_effect;   // Enable proximity effect calculation
} peec_via_t;

// Via parasitic parameters structure
typedef struct {
    double resistance;          // Resistance (Ohms)
    double inductance;          // Inductance (H)
    double capacitance;          // Capacitance (F)
    double conductance;          // Conductance (S)
    double skin_depth;          // Skin depth (m)
    double effective_resistance; // Effective resistance with skin effect (Ohms)
    double proximity_factor;     // Proximity effect factor
    bool converged;              // Convergence status
} via_parasitic_params_t;

// Layer stackup for via calculation
typedef struct {
    int num_layers;
    double* thickness;          // Layer thicknesses (m)
    double* epsilon_r;           // Relative permittivities
    double* mu_r;                // Relative permeabilities
    double* conductivity;        // Layer conductivities (S/m)
    double* loss_tangent;        // Loss tangents
} via_layer_stackup_t;

// Via-to-via coupling parameters
typedef struct {
    int via1_index;
    int via2_index;
    double mutual_inductance;   // Mutual inductance (H)
    double mutual_capacitance;  // Mutual capacitance (F)
    double coupling_coefficient; // Coupling coefficient
    double distance;             // Center-to-center distance (m)
} via_coupling_t;

/********************************************************************************
 * Via Parasitic Parameter Calculation
 ********************************************************************************/

/**
 * @brief Compute precise via parasitic parameters
 * @param via Via structure
 * @param layer_stackup Layer stackup information
 * @param frequency Operating frequency (Hz)
 * @param params Output parasitic parameters
 * @return 0 on success, error code otherwise
 */
int peec_compute_via_parasitics(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup,
    double frequency,
    via_parasitic_params_t* params
);

/**
 * @brief Compute via DC resistance
 * @param via Via structure
 * @return DC resistance (Ohms)
 */
double peec_compute_via_resistance_dc(const peec_via_t* via);

/**
 * @brief Compute via self inductance
 * @param via Via structure
 * @param layer_stackup Layer stackup information
 * @return Self inductance (H)
 */
double peec_compute_via_inductance(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup
);

/**
 * @brief Compute via self capacitance
 * @param via Via structure
 * @param layer_stackup Layer stackup information
 * @return Self capacitance (F)
 */
double peec_compute_via_capacitance(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup
);

/**
 * @brief Compute via-to-via mutual inductance
 * @param via1 First via
 * @param via2 Second via
 * @return Mutual inductance (H)
 */
double peec_compute_via_mutual_inductance(
    const peec_via_t* via1,
    const peec_via_t* via2
);

/**
 * @brief Compute via-to-via mutual capacitance
 * @param via1 First via
 * @param via2 Second via
 * @param layer_stackup Layer stackup information
 * @return Mutual capacitance (F)
 */
double peec_compute_via_mutual_capacitance(
    const peec_via_t* via1,
    const peec_via_t* via2,
    const via_layer_stackup_t* layer_stackup
);

/********************************************************************************
 * High-Frequency Effects
 ********************************************************************************/

/**
 * @brief Compute skin depth
 * @param frequency Operating frequency (Hz)
 * @param conductivity Material conductivity (S/m)
 * @param permeability Material permeability (H/m)
 * @return Skin depth (m)
 */
double peec_compute_skin_depth(double frequency, double conductivity, double permeability);

/**
 * @brief Compute effective resistance with skin effect
 * @param via Via structure
 * @param frequency Operating frequency (Hz)
 * @return Effective resistance (Ohms)
 */
double peec_compute_via_resistance_skin_effect(
    const peec_via_t* via,
    double frequency
);

/**
 * @brief Compute proximity effect factor
 * @param via1 First via
 * @param via2 Second via
 * @param frequency Operating frequency (Hz)
 * @return Proximity effect factor
 */
double peec_compute_proximity_effect_factor(
    const peec_via_t* via1,
    const peec_via_t* via2,
    double frequency
);

/********************************************************************************
 * Via Modeling and Mesh Integration
 ********************************************************************************/

/**
 * @brief Create precise cylindrical via mesh
 * @param via Via structure
 * @param num_segments Number of cylindrical segments
 * @param mesh Output mesh structure
 * @return 0 on success, error code otherwise
 */
int peec_create_cylindrical_via_mesh(
    const peec_via_t* via,
    int num_segments,
    mesh_t* mesh
);

/**
 * @brief Add via to triangular mesh
 * @param mesh Triangular mesh
 * @param via Via structure
 * @return 0 on success, error code otherwise
 */
int peec_add_via_to_triangular_mesh(
    mesh_t* mesh,
    const peec_via_t* via
);

/**
 * @brief Extract via partial elements from triangular mesh
 * @param mesh Triangular mesh
 * @param via Via structure
 * @param frequency Operating frequency (Hz)
 * @param params Output parasitic parameters
 * @return 0 on success, error code otherwise
 */
int peec_extract_via_partial_elements_triangular(
    const mesh_t* mesh,
    const peec_via_t* via,
    double frequency,
    via_parasitic_params_t* params
);

/********************************************************************************
 * Via-to-Plane Coupling
 ********************************************************************************/

/**
 * @brief Compute via-to-plane capacitance
 * @param via Via structure
 * @param plane_z Z-coordinate of plane
 * @param layer_stackup Layer stackup information
 * @return Via-to-plane capacitance (F)
 */
double peec_compute_via_to_plane_capacitance(
    const peec_via_t* via,
    double plane_z,
    const via_layer_stackup_t* layer_stackup
);

/**
 * @brief Compute via-to-plane inductance
 * @param via Via structure
 * @param plane_z Z-coordinate of plane
 * @return Via-to-plane inductance (H)
 */
double peec_compute_via_to_plane_inductance(
    const peec_via_t* via,
    double plane_z
);

/********************************************************************************
 * WGF Integration for Layered Media
 ********************************************************************************/

/**
 * @brief Compute via Green's function using WGF for layered media
 * @param via Via structure
 * @param layer_stackup Layer stackup information
 * @param frequency Operating frequency (Hz)
 * @param medium Layered medium structure
 * @param freq Frequency domain parameters
 * @return Green's function value
 */
complex_t peec_compute_via_greens_function_wgf(
    const peec_via_t* via,
    const via_layer_stackup_t* layer_stackup,
    double frequency,
    const LayeredMedium* medium,
    const FrequencyDomain* freq
);

/********************************************************************************
 * Utility Functions
 ********************************************************************************/

/**
 * @brief Initialize via structure with default values
 * @param via Via structure to initialize
 * @return 0 on success
 */
int peec_via_init(peec_via_t* via);

/**
 * @brief Free via structure memory
 * @param via Via structure to free
 */
void peec_via_free(peec_via_t* via);

/**
 * @brief Get default via material (copper)
 * @return Default via material structure
 */
via_material_t peec_get_default_via_material(void);

/**
 * @brief Convert via_node_t to peec_via_t
 * @param via_node Source via node
 * @param peec_via Output PEEC via structure
 * @return 0 on success
 */
int peec_convert_via_node_to_peec_via(
    const void* via_node,  // via_node_t from discretization/mesh/manhattan_mesh_peec.c
    peec_via_t* peec_via
);

#ifdef __cplusplus
}
#endif

#endif // PEEC_VIA_MODELING_H

