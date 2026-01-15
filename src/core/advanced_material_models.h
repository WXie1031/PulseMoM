#ifndef ADVANCED_MATERIAL_MODELS_H
#define ADVANCED_MATERIAL_MODELS_H

#include <complex.h>
#include <stdbool.h>
#include "pcb_structure.h"

typedef enum {
    DISPERSION_NONE,
    DISPERSION_DEBYE,
    DISPERSION_LORENTZ,
    DISPERSION_COLE_COLE,
    DISPERSION_DJORDJEVIC,
    DISPERSION_WIDE_BAND
} DispersionModelType;

typedef enum {
    CONDUCTIVITY_CONSTANT,
    CONDUCTIVITY_FREQUENCY_DEPENDENT,
    CONDUCTIVITY_TEMPERATURE_DEPENDENT
} ConductivityModelType;

typedef struct {
    double epsilon_infinity;
    double epsilon_static;
    double relaxation_time;
    double conductivity_dc;
} DebyeModel;

typedef struct {
    double epsilon_infinity;
    double epsilon_static;
    double resonant_frequency;
    double damping_factor;
    double oscillator_strength;
} LorentzModel;

typedef struct {
    double epsilon_infinity;
    double epsilon_static;
    double relaxation_time;
    double distribution_parameter;
} ColeColeModel;

typedef struct {
    double epsilon_infinity;
    double epsilon_static;
    double relaxation_time;
    double low_freq_conductivity;
    double high_freq_conductivity;
} DjordjevicModel;

typedef struct {
    double complex permittivity;
    double conductivity;
    double loss_tangent;
    double frequency;
} MaterialProperties;

typedef struct {
    char name[64];
    DispersionModelType dispersion_type;
    union {
        DebyeModel debye;
        LorentzModel lorentz;
        ColeColeModel cole_cole;
        DjordjevicModel djordjevic;
    } model;
    ConductivityModelType conductivity_type;
    double temperature_coefficient;
    double reference_temperature;
    double frequency_range[2];
    bool is_anisotropic;
    double permittivity_tensor[3][3];
    double permeability_tensor[3][3];
} AdvancedMaterial;

typedef struct {
    AdvancedMaterial* materials;
    int num_materials;
    double temperature;
    double frequency;
    bool use_gpu_acceleration;
} MaterialDatabase;

typedef struct {
    double frequency;
    double complex epsilon;
    double complex mu;
    double conductivity;
    double loss_tangent;
    double wave_impedance;
    double propagation_constant;
} MaterialFrequencyResponse;

MaterialDatabase* create_material_database(int max_materials);
void destroy_material_database(MaterialDatabase* db);

AdvancedMaterial* add_material_to_database(
    MaterialDatabase* db,
    const char* name,
    DispersionModelType dispersion_type
);

int set_debye_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double relaxation_time,
    double conductivity_dc
);

int set_lorentz_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double resonant_frequency,
    double damping_factor,
    double oscillator_strength
);

int set_cole_cole_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double relaxation_time,
    double distribution_parameter
);

int set_djordjevic_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double relaxation_time,
    double low_freq_conductivity,
    double high_freq_conductivity
);

MaterialProperties calculate_material_properties(
    const AdvancedMaterial* material,
    double frequency,
    double temperature
);

MaterialFrequencyResponse* calculate_frequency_response(
    const AdvancedMaterial* material,
    const double* frequencies,
    int num_frequencies,
    double temperature
);

double complex calculate_effective_permittivity(
    const AdvancedMaterial* substrate,
    const AdvancedMaterial* superstrate,
    double frequency,
    double thickness_ratio
);

double complex calculate_surface_impedance(
    const AdvancedMaterial* material,
    double frequency,
    double thickness
);

int extract_material_parameters_from_sparameters(
    const double complex* sparameters,
    const double* frequencies,
    int num_frequencies,
    AdvancedMaterial* material,
    double initial_guess[4]
);

int validate_material_causality(
    const MaterialFrequencyResponse* response,
    int num_frequencies
);

int validate_material_passivity(
    const MaterialFrequencyResponse* response,
    int num_frequencies
);

typedef struct {
    char manufacturer[64];
    char product_name[64];
    double nominal_permittivity;
    double nominal_loss_tangent;
    double thickness;
    double copper_roughness;
    bool is_reinforced;
    char reinforcement_type[32];
} CommercialPCBMaterial;

CommercialPCBMaterial* load_commercial_materials(const char* filename, int* num_materials);
int save_material_database(const MaterialDatabase* db, const char* filename);
MaterialDatabase* load_material_database(const char* filename);

void apply_material_to_pcb_layer(
    PCBLayer* layer,
    const AdvancedMaterial* material,
    double thickness
);

void apply_frequency_dependent_materials(
    PCBEMModel* em_model,
    const MaterialDatabase* material_db,
    double frequency
);

#endif