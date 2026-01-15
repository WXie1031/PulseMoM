/**
 * @file peec_materials_enhanced.h
 * @brief Enhanced material support for satellite PEEC solver
 * @details Comprehensive material database and property handling for PEEC/MoM
 */

#ifndef PEEC_MATERIALS_ENHANCED_H
#define PEEC_MATERIALS_ENHANCED_H

#include "peec_satellite.h"
#include "../core/core_geometry.h"
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

// Material types for electromagnetic analysis
typedef enum {
    MATERIAL_TYPE_CONDUCTOR,    // Metals and good conductors
    MATERIAL_TYPE_DIELECTRIC,   // Insulators and dielectrics
    MATERIAL_TYPE_SEMICONDUCTOR,// Semiconductors
    MATERIAL_TYPE_MAGNETIC,     // Magnetic materials
    MATERIAL_TYPE_LOSSY,        // Lossy materials
    MATERIAL_TYPE_ANISOTROPIC, // Anisotropic materials
    MATERIAL_TYPE_DISPERSIVE,  // Frequency-dependent materials
    MATERIAL_TYPE_PEC,          // Perfect Electric Conductor
    MATERIAL_TYPE_PMC           // Perfect Magnetic Conductor
} peec_material_type_t;

// Enhanced material properties structure
typedef struct {
    // Basic properties
    int id;
    char name[64];
    peec_material_type_t type;
    
    // Electromagnetic properties (frequency-independent)
    double conductivity;        // S/m
    double permittivity;        // Relative permittivity (εr)
    double permeability;        // Relative permeability (μr)
    double loss_tangent;        // Dielectric loss tangent
    
    // Frequency-dependent properties
    bool is_dispersive;
    double* frequency_points;   // Hz
    double* eps_real;           // Real part of permittivity
    double* eps_imag;           // Imaginary part of permittivity
    double* mu_real;            // Real part of permeability
    double* mu_imag;            // Imaginary part of permeability
    int num_frequency_points;
    
    // Physical properties
    double density;             // kg/m³
    double thermal_conductivity; // W/(m·K)
    double temperature_coefficient; // 1/K
    
    // Surface properties (for conductors)
    double surface_roughness;   // m (RMS)
    double oxidation_layer;     // m
    
    // Temperature dependence
    double reference_temperature; // K
    bool temperature_dependent;
    
} peec_material_properties_t;

// Material database
typedef struct {
    peec_material_properties_t* materials;
    int num_materials;
    int max_materials;
    
    // Predefined materials
    struct {
        int copper;
        int aluminum;
        int silver;
        int gold;
        int steel;
        int pec;
        int air;
        int vacuum;
        int water;
        int fr4;
        int silicon;
        int gallium_arsenide;
    } predefined;
    
} peec_material_database_t;

// Frequency-dependent material evaluation
typedef struct {
    double frequency;
    double complex permittivity;
    double complex permeability;
    double conductivity;
    double skin_depth;
    double surface_impedance_real;
    double surface_impedance_imag;
} peec_material_frequency_data_t;

/*********************************************************************
 * Material Database Management
 *********************************************************************/

peec_material_database_t* peec_materials_create_database(int max_materials);
void peec_materials_destroy_database(peec_material_database_t* db);

// Add materials to database
int peec_materials_add_material(peec_material_database_t* db, const peec_material_properties_t* material);
int peec_materials_add_predefined_materials(peec_material_database_t* db);

// Material lookup
int peec_materials_find_by_name(peec_material_database_t* db, const char* name);
peec_material_properties_t* peec_materials_get_by_id(peec_material_database_t* db, int id);
peec_material_properties_t* peec_materials_get_by_name(peec_material_database_t* db, const char* name);

/*********************************************************************
 * Material Property Evaluation
 *********************************************************************/

int peec_materials_evaluate_at_frequency(peec_material_properties_t* material, double frequency,
                                       peec_material_frequency_data_t* result);

double complex peec_materials_get_permittivity(peec_material_properties_t* material, double frequency);
double complex peec_materials_get_permeability(peec_material_properties_t* material, double frequency);
double peec_materials_get_conductivity(peec_material_properties_t* material, double frequency);
double peec_materials_get_skin_depth(peec_material_properties_t* material, double frequency);
double complex peec_materials_get_surface_impedance(peec_material_properties_t* material, double frequency);

/*********************************************************************
 * Temperature Effects
 *********************************************************************/

double peec_materials_get_conductivity_temperature(peec_material_properties_t* material, double temperature);
double peec_materials_get_permittivity_temperature(peec_material_properties_t* material, double temperature);

/*********************************************************************
 * Material Classification and Validation
 *********************************************************************/

bool peec_materials_is_good_conductor(peec_material_properties_t* material, double frequency);
bool peec_materials_is_good_dielectric(peec_material_properties_t* material, double frequency);
bool peec_materials_is_magnetic(peec_material_properties_t* material, double frequency);
bool peec_materials_is_dispersive(peec_material_properties_t* material, double frequency_range[2]);

int peec_materials_validate_material(peec_material_properties_t* material, char* error_msg, int max_msg_len);

/*********************************************************************
 * Predefined Material Creation
 *********************************************************************/

// Create common materials
peec_material_properties_t peec_materials_create_copper(void);
peec_material_properties_t peec_materials_create_aluminum(void);
peec_material_properties_t peec_materials_create_silver(void);
peec_material_properties_t peec_materials_create_gold(void);
peec_material_properties_t peec_materials_create_steel(void);
peec_material_properties_t peec_materials_create_pec(void);
peec_material_properties_t peec_materials_create_air(void);
peec_material_properties_t peec_materials_create_vacuum(void);
peec_material_properties_t peec_materials_create_water(void);
peec_material_properties_t peec_materials_create_fr4(void);
peec_material_properties_t peec_materials_create_silicon(void);
peec_material_properties_t peec_materials_create_gallium_arsenide(void);

/*********************************************************************
 * Enhanced Satellite PEEC Integration
 *********************************************************************/

// Enhanced satellite configuration with material support
typedef struct {
    peec_satellite_config_t base_config;           // Original satellite config
    peec_material_database_t* material_db;        // Material database
    
    // Multi-material satellite structure
    struct {
        int satellite_body_material;              // Main satellite material (PEC/aluminum)
        int solar_panel_material;                 // Solar panel material
        int antenna_material;                     // Antenna material
        int coating_material;                     // Surface coating material
        int substrate_material;                   // Circuit board substrate
        bool use_coating;                         // Enable surface coating
        bool use_substrate;                       // Enable circuit boards
    } satellite_materials;
    
    // Material assignment strategy
    struct {
        bool auto_assign_materials;               // Automatically assign materials
        bool use_conductivity_threshold;          // Use conductivity for classification
        double conductivity_threshold;             // Threshold for conductor/dielectric
        bool validate_material_compatibility;      // Check material compatibility
    } material_assignment;
    
} peec_satellite_enhanced_config_t;

// Enhanced satellite solver with material support
typedef struct peec_satellite_enhanced_solver {
    peec_satellite_solver_t* base_solver;          // Original satellite solver
    peec_material_database_t* material_db;       // Material database
    peec_satellite_enhanced_config_t config;      // Enhanced configuration
    
    // Material-specific element data
    peec_material_frequency_data_t* element_material_data; // Per-element material data
    int* element_material_ids;                    // Material ID for each element
    
    // Multi-frequency analysis
    bool multi_frequency_enabled;
    double* frequency_points;                     // Frequency sweep points
    int num_frequency_points;
    
    // Enhanced field computation
    bool include_material_dispersion;              // Include frequency dispersion
    bool include_temperature_effects;             // Include temperature dependence
    bool include_surface_roughness;               // Include surface roughness
    
} peec_satellite_enhanced_solver_t;

/*********************************************************************
 * Enhanced Solver Interface
 *********************************************************************/

peec_satellite_enhanced_solver_t* peec_satellite_enhanced_create(void);
void peec_satellite_enhanced_destroy(peec_satellite_enhanced_solver_t* solver);

int peec_satellite_enhanced_configure(peec_satellite_enhanced_solver_t* solver,
                                    const peec_satellite_enhanced_config_t* config);

int peec_satellite_enhanced_set_material_database(peec_satellite_enhanced_solver_t* solver,
                                                  peec_material_database_t* db);

int peec_satellite_enhanced_assign_materials(peec_satellite_enhanced_solver_t* solver);

int peec_satellite_enhanced_compute_material_properties(peec_satellite_enhanced_solver_t* solver);

int peec_satellite_enhanced_solve_multi_frequency(peec_satellite_enhanced_solver_t* solver,
                                                double* frequencies, int num_freqs);

int peec_satellite_enhanced_export_material_data(peec_satellite_enhanced_solver_t* solver,
                                                 const char* filename, const char* format);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

void peec_materials_print_database(peec_material_database_t* db);
void peec_materials_print_material(peec_material_properties_t* material);
void peec_materials_print_frequency_data(peec_material_frequency_data_t* data);

const char* peec_materials_get_type_name(peec_material_type_t type);
const char* peec_materials_get_predefined_name(int predefined_id);

#ifdef __cplusplus
}
#endif

#endif // PEEC_MATERIALS_ENHANCED_H