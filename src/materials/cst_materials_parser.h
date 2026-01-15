/**
 * @file cst_materials_parser.h
 * @brief CST material file parser and management system
 * @details Based on CST Studio Suite material database format (.mtd files)
 */

#ifndef CST_MATERIALS_PARSER_H
#define CST_MATERIALS_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if !defined(_MSC_VER)
#include <complex.h>
typedef double complex cst_complex_t;
#else
// MSVC doesn't have complex.h, use custom complex type
typedef struct { double re; double im; } cst_complex_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// CST material property types
#define CST_TYPE_NORMAL "Normal"
#define CST_TYPE_LOSSY_METAL "Lossy metal"
#define CST_TYPE_PEC "PEC"
#define CST_TYPE_LOSSLESS_METAL "Lossless metal"
#define CST_TYPE_DISPERSIVE "Dispersive"
#define CST_TYPE_ANISOTROPIC "Anisotropic"

// CST frequency types
#define CST_FRQTYPE_STATIC "static"
#define CST_FRQTYPE_ALL "all"

// CST material units
#define CST_UNIT_GHZ "GHz"
#define CST_UNIT_MHZ "MHz"
#define CST_UNIT_HZ "Hz"
#define CST_UNIT_MM "mm"
#define CST_UNIT_M "m"
#define CST_UNIT_UM "um"
#define CST_UNIT_NS "ns"
#define CST_UNIT_S "s"
#define CST_UNIT_KELVIN "Kelvin"
#define CST_UNIT_CELSIUS "Celsius"

// Maximum line length for parsing
#define CST_MAX_LINE_LENGTH 1024
#define CST_MAX_NAME_LENGTH 256
#define CST_MAX_PATH_LENGTH 512

/**
 * @brief CST material definition structure
 * Based on CST Studio Suite material database format
 */
typedef struct {
    // Basic identification
    char name[CST_MAX_NAME_LENGTH];
    char filename[CST_MAX_PATH_LENGTH];
    char description[CST_MAX_LINE_LENGTH];
    
    // Material type and frequency range
    char type[64];
    char frq_type[64];
    
    // Units
    struct {
        char frequency[32];
        char geometry[32];
        char time[32];
        char temperature[32];
    } units;
    
    // Electromagnetic properties (frequency-independent)
    double epsilon;      // Relative permittivity
    double mu;          // Relative permeability
    double sigma;       // Electric conductivity (S/m)
    double sigma_m;     // Magnetic conductivity
    double tan_d;       // Dielectric loss tangent
    double tan_d_m;     // Magnetic loss tangent
    
    // Loss tangent frequency
    double tan_d_freq;      // Frequency for tan_d (GHz)
    double tan_d_m_freq;    // Frequency for tan_d_m (GHz)
    bool tan_d_given;
    bool tan_d_m_given;
    
    // Loss tangent models
    char tan_d_model[64];
    char tan_d_m_model[64];
    
    // Dispersion models
    char disp_model_eps[64];
    char disp_model_mu[64];
    char dispersive_fitting_scheme_eps[64];
    char dispersive_fitting_scheme_mu[64];
    bool use_general_dispersion_eps;
    bool use_general_dispersion_mu;
    
    // Physical properties
    double rho;                     // Density (kg/m³)
    double thermal_conductivity;    // Thermal conductivity (W/K/m)
    double specific_heat;           // Specific heat (J/K/kg)
    double thermal_expansion_rate;  // Thermal expansion coefficient (1e-6/K)
    double youngs_modulus;          // Young's modulus (kN/mm²)
    double poissons_ratio;          // Poisson's ratio
    
    // Thermal properties
    char thermal_type[64];
    
    // Mechanical properties
    char mechanics_type[64];
    
    // Color for visualization (RGB 0-1)
    double colour_r, colour_g, colour_b;
    
    // Material flags
    bool wireframe;
    bool transparency;
    double transparency_value;
    bool allow_outline;
    bool transparent_outline;
    
    // Coordinate system
    char reference_coord_system[64];
    char coord_system_type[64];
    
    // Anisotropy
    bool nl_anisotropy;
    double nla_stacking_factor;
    double nla_direction_x, nla_direction_y, nla_direction_z;
    
    // Dispersive data (if available)
    struct {
        double* frequencies;        // GHz
        double* epsilon_real;
        double* epsilon_imag;
        double* mu_real;
        double* mu_imag;
        int num_points;
        int max_points;
    } dispersive_data;
    
} cst_material_t;

/**
 * @brief CST material database
 */
typedef struct {
    cst_material_t* materials;
    int num_materials;
    int max_materials;
    char library_path[CST_MAX_PATH_LENGTH];
} cst_material_database_t;

/**
 * @brief CST material parser functions
 */

// Database management
cst_material_database_t* cst_materials_create_database(int max_materials, const char* library_path);
void cst_materials_destroy_database(cst_material_database_t* db);

// Material file parsing
cst_material_t* cst_materials_parse_file(const char* filename);
int cst_materials_load_from_library(cst_material_database_t* db, const char* material_name);
int cst_materials_load_all_from_directory(cst_material_database_t* db, const char* directory);

// Material creation and modification
cst_material_t* cst_materials_create_material(const char* name, const char* type);
int cst_materials_save_material(const cst_material_t* material, const char* filename);
void cst_materials_copy_material(cst_material_t* dest, const cst_material_t* src);

// Material lookup
cst_material_t* cst_materials_find_by_name(cst_material_database_t* db, const char* name);
cst_material_t* cst_materials_get_by_index(cst_material_database_t* db, int index);

// Property evaluation
cst_complex_t cst_materials_get_epsilon(const cst_material_t* material, double frequency);
cst_complex_t cst_materials_get_mu(const cst_material_t* material, double frequency);
double cst_materials_get_conductivity(const cst_material_t* material, double frequency);
double cst_materials_get_skin_depth(const cst_material_t* material, double frequency);

// Dispersive material support
int cst_materials_add_dispersive_point(cst_material_t* material, double freq, double eps_r, double eps_i, double mu_r, double mu_i);
int cst_materials_evaluate_at_frequency(const cst_material_t* material, double frequency, cst_complex_t* eps, cst_complex_t* mu);

// Utility functions
void cst_materials_print_material(const cst_material_t* material);
void cst_materials_print_database_summary(const cst_material_database_t* db);
const char* cst_materials_get_type_description(const char* type);
const char* cst_materials_get_frqtype_description(const char* frq_type);

// Predefined CST materials
cst_material_t* cst_materials_create_copper(void);
cst_material_t* cst_materials_create_aluminum(void);
cst_material_t* cst_materials_create_gold(void);
cst_material_t* cst_materials_create_silver(void);
cst_material_t* cst_materials_create_pec(void);
cst_material_t* cst_materials_create_fr4(void);
cst_material_t* cst_materials_create_air(void);
cst_material_t* cst_materials_create_vacuum(void);

// Material classification
bool cst_materials_is_conductor(const cst_material_t* material, double frequency);
bool cst_materials_is_dielectric(const cst_material_t* material, double frequency);
bool cst_materials_is_magnetic(const cst_material_t* material, double frequency);
bool cst_materials_is_dispersive(const cst_material_t* material);

// Conversion functions
double cst_materials_convert_units(double value, const char* from_unit, const char* to_unit);

#ifdef __cplusplus
}
#endif

#endif // CST_MATERIALS_PARSER_H