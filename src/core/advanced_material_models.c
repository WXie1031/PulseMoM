#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "advanced_material_models.h"
#include "../utils/memory_utils.h"
#include "../utils/math_utils.h"

#define MAX_DEBYE_TERMS 10
#define MAX_LORENTZ_TERMS 10
#define CAUSALITY_THRESHOLD 1e-6
#define PASSIVITY_THRESHOLD 1e-6
#define MAX_ITERATIONS 1000
#define TOLERANCE 1e-12

MaterialDatabase* create_material_database(int max_materials) {
    MaterialDatabase* db = (MaterialDatabase*)safe_malloc(sizeof(MaterialDatabase));
    db->materials = (AdvancedMaterial*)safe_malloc(max_materials * sizeof(AdvancedMaterial));
    db->num_materials = 0;
    db->temperature = 25.0;
    db->frequency = 1e9;
    db->use_gpu_acceleration = false;
    return db;
}

void destroy_material_database(MaterialDatabase* db) {
    if (db) {
        safe_free(db->materials);
        safe_free(db);
    }
}

AdvancedMaterial* add_material_to_database(
    MaterialDatabase* db,
    const char* name,
    DispersionModelType dispersion_type
) {
    if (!db || db->num_materials >= 1000) return NULL;
    
    AdvancedMaterial* material = &db->materials[db->num_materials];
    strncpy(material->name, name, 63);
    material->name[63] = '\0';
    material->dispersion_type = dispersion_type;
    material->conductivity_type = CONDUCTIVITY_CONSTANT;
    material->temperature_coefficient = 0.0;
    material->reference_temperature = 25.0;
    material->frequency_range[0] = 1e6;
    material->frequency_range[1] = 1e12;
    material->is_anisotropic = false;
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            material->permittivity_tensor[i][j] = (i == j) ? 1.0 : 0.0;
            material->permeability_tensor[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
    
    db->num_materials++;
    return material;
}

int set_debye_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double relaxation_time,
    double conductivity_dc
) {
    if (!material) return -1;
    
    material->model.debye.epsilon_infinity = epsilon_infinity;
    material->model.debye.epsilon_static = epsilon_static;
    material->model.debye.relaxation_time = relaxation_time;
    material->model.debye.conductivity_dc = conductivity_dc;
    
    return 0;
}

int set_lorentz_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double resonant_frequency,
    double damping_factor,
    double oscillator_strength
) {
    if (!material) return -1;
    
    material->model.lorentz.epsilon_infinity = epsilon_infinity;
    material->model.lorentz.epsilon_static = epsilon_static;
    material->model.lorentz.resonant_frequency = resonant_frequency;
    material->model.lorentz.damping_factor = damping_factor;
    material->model.lorentz.oscillator_strength = oscillator_strength;
    
    return 0;
}

int set_cole_cole_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double relaxation_time,
    double distribution_parameter
) {
    if (!material) return -1;
    
    material->model.cole_cole.epsilon_infinity = epsilon_infinity;
    material->model.cole_cole.epsilon_static = epsilon_static;
    material->model.cole_cole.relaxation_time = relaxation_time;
    material->model.cole_cole.distribution_parameter = distribution_parameter;
    
    return 0;
}

int set_djordjevic_model(
    AdvancedMaterial* material,
    double epsilon_infinity,
    double epsilon_static,
    double relaxation_time,
    double low_freq_conductivity,
    double high_freq_conductivity
) {
    if (!material) return -1;
    
    material->model.djordjevic.epsilon_infinity = epsilon_infinity;
    material->model.djordjevic.epsilon_static = epsilon_static;
    material->model.djordjevic.relaxation_time = relaxation_time;
    material->model.djordjevic.low_freq_conductivity = low_freq_conductivity;
    material->model.djordjevic.high_freq_conductivity = high_freq_conductivity;
    
    return 0;
}

static double complex calculate_debye_permittivity(const DebyeModel* model, double omega) {
    double complex j_omega_tau = I * omega * model->relaxation_time;
    double complex epsilon_debye = (model->epsilon_static - model->epsilon_infinity) / (1.0 + j_omega_tau);
    return model->epsilon_infinity + epsilon_debye;
}

static double complex calculate_lorentz_permittivity(const LorentzModel* model, double omega) {
    double omega_0 = 2.0 * M_PI * model->resonant_frequency;
    double complex numerator = model->oscillator_strength * omega_0 * omega_0;
    double complex denominator = omega_0 * omega_0 - omega * omega + I * omega * model->damping_factor;
    return model->epsilon_infinity + numerator / denominator;
}

static double complex calculate_cole_cole_permittivity(const ColeColeModel* model, double omega) {
    double complex j_omega_tau_alpha = cpow(I * omega * model->relaxation_time, model->distribution_parameter);
    double complex epsilon_cc = (model->epsilon_static - model->epsilon_infinity) / (1.0 + j_omega_tau_alpha);
    return model->epsilon_infinity + epsilon_cc;
}

static double complex calculate_djordjevic_permittivity(const DjordjevicModel* model, double omega) {
    double complex j_omega_tau = I * omega * model->relaxation_time;
    double complex epsilon_dj = (model->epsilon_static - model->epsilon_infinity) / (1.0 + j_omega_tau);
    double complex conductivity_term = I * model->low_freq_conductivity / (omega * 8.854e-12);
    return model->epsilon_infinity + epsilon_dj + conductivity_term;
}

MaterialProperties calculate_material_properties(
    const AdvancedMaterial* material,
    double frequency,
    double temperature
) {
    MaterialProperties props = {0};
    if (!material) return props;
    
    double omega = 2.0 * M_PI * frequency;
    double complex epsilon = 1.0;
    
    switch (material->dispersion_type) {
        case DISPERSION_DEBYE:
            epsilon = calculate_debye_permittivity(&material->model.debye, omega);
            break;
        case DISPERSION_LORENTZ:
            epsilon = calculate_lorentz_permittivity(&material->model.lorentz, omega);
            break;
        case DISPERSION_COLE_COLE:
            epsilon = calculate_cole_cole_permittivity(&material->model.cole_cole, omega);
            break;
        case DISPERSION_DJORDJEVIC:
            epsilon = calculate_djordjevic_permittivity(&material->model.djordjevic, omega);
            break;
        case DISPERSION_WIDE_BAND:
            epsilon = material->model.debye.epsilon_infinity;
            break;
        default:
            epsilon = 4.4 - I * 0.088;
            break;
    }
    
    double temperature_factor = 1.0 + material->temperature_coefficient * (temperature - material->reference_temperature);
    epsilon *= temperature_factor;
    
    props.permittivity = epsilon;
    props.conductivity = creal(epsilon) * 8.854e-12 * omega * ctan(carg(epsilon));
    props.loss_tangent = -cimag(epsilon) / creal(epsilon);
    props.frequency = frequency;
    
    return props;
}

MaterialFrequencyResponse* calculate_frequency_response(
    const AdvancedMaterial* material,
    const double* frequencies,
    int num_frequencies,
    double temperature
) {
    if (!material || !frequencies || num_frequencies <= 0) return NULL;
    
    MaterialFrequencyResponse* response = (MaterialFrequencyResponse*)safe_malloc(
        num_frequencies * sizeof(MaterialFrequencyResponse)
    );
    
    for (int i = 0; i < num_frequencies; i++) {
        MaterialProperties props = calculate_material_properties(material, frequencies[i], temperature);
        response[i].frequency = frequencies[i];
        response[i].epsilon = props.permittivity;
        response[i].mu = 1.0;
        response[i].conductivity = props.conductivity;
        response[i].loss_tangent = props.loss_tangent;
        response[i].wave_impedance = sqrt(mu_0 / (props.permittivity * epsilon_0));
        response[i].propagation_constant = 2.0 * M_PI * frequencies[i] * sqrt(mu_0 * props.permittivity * epsilon_0);
    }
    
    return response;
}

double complex calculate_effective_permittivity(
    const AdvancedMaterial* substrate,
    const AdvancedMaterial* superstrate,
    double frequency,
    double thickness_ratio
) {
    if (!substrate || !superstrate) return 1.0;
    
    MaterialProperties sub_props = calculate_material_properties(substrate, frequency, 25.0);
    MaterialProperties sup_props = calculate_material_properties(superstrate, frequency, 25.0);
    
    return sub_props.permittivity * thickness_ratio + sup_props.permittivity * (1.0 - thickness_ratio);
}

double complex calculate_surface_impedance(
    const AdvancedMaterial* material,
    double frequency,
    double thickness
) {
    if (!material) return 0.0;
    
    MaterialProperties props = calculate_material_properties(material, frequency, 25.0);
    double omega = 2.0 * M_PI * frequency;
    double complex gamma = I * omega * sqrt(props.permittivity * epsilon_0 * mu_0);
    double complex eta = sqrt(mu_0 / (props.permittivity * epsilon_0));
    
    return eta * ctanh(gamma * thickness);
}

static double complex kramers_kronig_transform(const double complex* data, const double* freqs, int n, int index) {
    double complex result = 0.0;
    double omega = 2.0 * M_PI * freqs[index];
    
    for (int i = 0; i < n; i++) {
        if (i == index) continue;
        double omega_i = 2.0 * M_PI * freqs[i];
        result += data[i] / (omega_i - omega);
    }
    
    return result * (2.0 / M_PI);
}

int validate_material_causality(
    const MaterialFrequencyResponse* response,
    int num_frequencies
) {
    if (!response || num_frequencies < 2) return -1;
    
    double* freqs = (double*)safe_malloc(num_frequencies * sizeof(double));
    double complex* epsilon_real = (double complex*)safe_malloc(num_frequencies * sizeof(double complex));
    double complex* epsilon_imag = (double complex*)safe_malloc(num_frequencies * sizeof(double complex));
    
    for (int i = 0; i < num_frequencies; i++) {
        freqs[i] = response[i].frequency;
        epsilon_real[i] = creal(response[i].epsilon);
        epsilon_imag[i] = cimag(response[i].epsilon);
    }
    
    bool causal = true;
    for (int i = 0; i < num_frequencies; i++) {
        double complex predicted_imag = kramers_kronig_transform(epsilon_real, freqs, num_frequencies, i);
        double complex predicted_real = -kramers_kronig_transform(epsilon_imag, freqs, num_frequencies, i);
        
        if (cabs(predicted_imag - cimag(response[i].epsilon)) > CAUSALITY_THRESHOLD ||
            cabs(predicted_real - creal(response[i].epsilon)) > CAUSALITY_THRESHOLD) {
            causal = false;
            break;
        }
    }
    
    safe_free(freqs);
    safe_free(epsilon_real);
    safe_free(epsilon_imag);
    
    return causal ? 0 : -1;
}

int validate_material_passivity(
    const MaterialFrequencyResponse* response,
    int num_frequencies
) {
    if (!response || num_frequencies <= 0) return -1;
    
    bool passive = true;
    for (int i = 0; i < num_frequencies; i++) {
        double complex epsilon = response[i].epsilon;
        if (cimag(epsilon) > 0.0) {
            passive = false;
            break;
        }
    }
    
    return passive ? 0 : -1;
}

CommercialPCBMaterial* load_commercial_materials(const char* filename, int* num_materials) {
    if (!filename || !num_materials) return NULL;
    
    FILE* fp = fopen(filename, "r");
    if (!fp) return NULL;
    
    int count = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] != '#' && strlen(line) > 10) count++;
    }
    
    rewind(fp);
    CommercialPCBMaterial* materials = (CommercialPCBMaterial*)safe_malloc(count * sizeof(CommercialPCBMaterial));
    
    int i = 0;
    while (fgets(line, sizeof(line), fp) && i < count) {
        if (line[0] == '#' || strlen(line) <= 10) continue;
        
        CommercialPCBMaterial* mat = &materials[i];
        sscanf(line, "%63[^,],%63[^,],%lf,%lf,%lf,%lf,%d,%31[^\n]",
               mat->manufacturer, mat->product_name,
               &mat->nominal_permittivity, &mat->nominal_loss_tangent,
               &mat->thickness, &mat->copper_roughness,
               (int*)&mat->is_reinforced, mat->reinforcement_type);
        i++;
    }
    
    fclose(fp);
    *num_materials = i;
    return materials;
}

int save_material_database(const MaterialDatabase* db, const char* filename) {
    if (!db || !filename) return -1;
    
    FILE* fp = fopen(filename, "w");
    if (!fp) return -1;
    
    fprintf(fp, "# PulseMoM Advanced Material Database\n");
    fprintf(fp, "# Temperature: %.1f C, Frequency: %.1f GHz\n", db->temperature, db->frequency/1e9);
    fprintf(fp, "# NumMaterials: %d\n\n", db->num_materials);
    
    for (int i = 0; i < db->num_materials; i++) {
        const AdvancedMaterial* mat = &db->materials[i];
        fprintf(fp, "Material: %s\n", mat->name);
        fprintf(fp, "DispersionType: %d\n", mat->dispersion_type);
        fprintf(fp, "ConductivityType: %d\n", mat->conductivity_type);
        fprintf(fp, "TemperatureCoeff: %.6e\n", mat->temperature_coefficient);
        fprintf(fp, "RefTemperature: %.1f\n", mat->reference_temperature);
        fprintf(fp, "FrequencyRange: %.1e %.1e\n", mat->frequency_range[0], mat->frequency_range[1]);
        fprintf(fp, "Anisotropic: %d\n", mat->is_anisotropic);
        
        switch (mat->dispersion_type) {
            case DISPERSION_DEBYE:
                fprintf(fp, "Debye: %.3f %.3f %.3e %.3e\n",
                        mat->model.debye.epsilon_infinity,
                        mat->model.debye.epsilon_static,
                        mat->model.debye.relaxation_time,
                        mat->model.debye.conductivity_dc);
                break;
            case DISPERSION_LORENTZ:
                fprintf(fp, "Lorentz: %.3f %.3f %.3e %.3e %.3f\n",
                        mat->model.lorentz.epsilon_infinity,
                        mat->model.lorentz.epsilon_static,
                        mat->model.lorentz.resonant_frequency,
                        mat->model.lorentz.damping_factor,
                        mat->model.lorentz.oscillator_strength);
                break;
            default:
                break;
        }
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    return 0;
}

void apply_material_to_pcb_layer(
    PCBLayer* layer,
    const AdvancedMaterial* material,
    double thickness
) {
    if (!layer || !material) return;
    
    MaterialProperties props = calculate_material_properties(material, 1e9, 25.0);
    layer->epsilon_r = creal(props.permittivity);
    layer->loss_tangent = props.loss_tangent;
    layer->conductivity = props.conductivity;
    layer->thickness = thickness;
    
    snprintf(layer->material_name, 63, "%s", material->name);
}

void apply_frequency_dependent_materials(
    PCBEMModel* em_model,
    const MaterialDatabase* material_db,
    double frequency
) {
    if (!em_model || !material_db) return;
    
    for (int i = 0; i < em_model->num_layers; i++) {
        for (int j = 0; j < material_db->num_materials; j++) {
            if (strcmp(em_model->layers[i].material_name, material_db->materials[j].name) == 0) {
                MaterialProperties props = calculate_material_properties(
                    &material_db->materials[j], frequency, material_db->temperature
                );
                em_model->layers[i].epsilon_r = creal(props.permittivity);
                em_model->layers[i].loss_tangent = props.loss_tangent;
                em_model->layers[i].conductivity = props.conductivity;
                break;
            }
        }
    }
}