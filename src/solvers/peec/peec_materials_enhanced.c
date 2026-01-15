/**
 * @file peec_materials_enhanced.c
 * @brief Enhanced material support implementation for satellite PEEC solver
 * @details Comprehensive material database and property evaluation
 */

#include "peec_materials_enhanced.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#define MU_0 (4.0 * M_PI * 1e-7)
#define EPS_0 (8.854187817e-12)
#define CONDUCTOR_THRESHOLD 1e6  // S/m - threshold for good conductor

/*********************************************************************
 * Material Database Management
 *********************************************************************/

/**
 * @brief Create material database
 */
peec_material_database_t* peec_materials_create_database(int max_materials) {
    peec_material_database_t* db = (peec_material_database_t*)calloc(1, sizeof(peec_material_database_t));
    if (!db) {
        fprintf(stderr, "Failed to allocate material database\n");
        return NULL;
    }
    
    db->max_materials = max_materials;
    db->materials = (peec_material_properties_t*)calloc(max_materials, sizeof(peec_material_properties_t));
    if (!db->materials) {
        fprintf(stderr, "Failed to allocate material storage\n");
        free(db);
        return NULL;
    }
    
    db->num_materials = 0;
    
    // Initialize predefined material indices to -1 (not found)
    db->predefined.copper = -1;
    db->predefined.aluminum = -1;
    db->predefined.silver = -1;
    db->predefined.gold = -1;
    db->predefined.steel = -1;
    db->predefined.pec = -1;
    db->predefined.air = -1;
    db->predefined.vacuum = -1;
    db->predefined.water = -1;
    db->predefined.fr4 = -1;
    db->predefined.silicon = -1;
    db->predefined.gallium_arsenide = -1;
    
    return db;
}

/**
 * @brief Destroy material database
 */
void peec_materials_destroy_database(peec_material_database_t* db) {
    if (!db) return;
    
    if (db->materials) {
        // Free frequency-dependent data for each material
        for (int i = 0; i < db->num_materials; i++) {
            peec_material_properties_t* mat = &db->materials[i];
            free(mat->frequency_points);
            free(mat->eps_real);
            free(mat->eps_imag);
            free(mat->mu_real);
            free(mat->mu_imag);
        }
        free(db->materials);
    }
    
    free(db);
}

/**
 * @brief Add material to database
 */
int peec_materials_add_material(peec_material_database_t* db, const peec_material_properties_t* material) {
    if (!db || !material || db->num_materials >= db->max_materials) {
        return -1;
    }
    
    int id = db->num_materials;
    db->materials[id] = *material;
    db->materials[id].id = id;
    
    // Copy frequency-dependent data
    if (material->num_frequency_points > 0 && material->frequency_points) {
        db->materials[id].frequency_points = (double*)malloc(material->num_frequency_points * sizeof(double));
        db->materials[id].eps_real = (double*)malloc(material->num_frequency_points * sizeof(double));
        db->materials[id].eps_imag = (double*)malloc(material->num_frequency_points * sizeof(double));
        db->materials[id].mu_real = (double*)malloc(material->num_frequency_points * sizeof(double));
        db->materials[id].mu_imag = (double*)malloc(material->num_frequency_points * sizeof(double));
        
        if (!db->materials[id].frequency_points || !db->materials[id].eps_real ||
            !db->materials[id].eps_imag || !db->materials[id].mu_real || !db->materials[id].mu_imag) {
            // Allocation failed, clean up
            free(db->materials[id].frequency_points);
            free(db->materials[id].eps_real);
            free(db->materials[id].eps_imag);
            free(db->materials[id].mu_real);
            free(db->materials[id].mu_imag);
            return -1;
        }
        
        memcpy(db->materials[id].frequency_points, material->frequency_points, 
               material->num_frequency_points * sizeof(double));
        memcpy(db->materials[id].eps_real, material->eps_real, 
               material->num_frequency_points * sizeof(double));
        memcpy(db->materials[id].eps_imag, material->eps_imag, 
               material->num_frequency_points * sizeof(double));
        memcpy(db->materials[id].mu_real, material->mu_real, 
               material->num_frequency_points * sizeof(double));
        memcpy(db->materials[id].mu_imag, material->mu_imag, 
               material->num_frequency_points * sizeof(double));
    }
    
    db->num_materials++;
    return id;
}

/**
 * @brief Add predefined materials to database
 */
int peec_materials_add_predefined_materials(peec_material_database_t* db) {
    if (!db) return -1;
    
    int count = 0;
    
    // Add common conductor materials
    peec_material_properties_t copper = peec_materials_create_copper();
    db->predefined.copper = peec_materials_add_material(db, &copper);
    if (db->predefined.copper >= 0) count++;
    
    peec_material_properties_t aluminum = peec_materials_create_aluminum();
    db->predefined.aluminum = peec_materials_add_material(db, &aluminum);
    if (db->predefined.aluminum >= 0) count++;
    
    peec_material_properties_t silver = peec_materials_create_silver();
    db->predefined.silver = peec_materials_add_material(db, &silver);
    if (db->predefined.silver >= 0) count++;
    
    peec_material_properties_t gold = peec_materials_create_gold();
    db->predefined.gold = peec_materials_add_material(db, &gold);
    if (db->predefined.gold >= 0) count++;
    
    peec_material_properties_t steel = peec_materials_create_steel();
    db->predefined.steel = peec_materials_add_material(db, &steel);
    if (db->predefined.steel >= 0) count++;
    
    // Add dielectric materials
    peec_material_properties_t air = peec_materials_create_air();
    db->predefined.air = peec_materials_add_material(db, &air);
    if (db->predefined.air >= 0) count++;
    
    peec_material_properties_t vacuum = peec_materials_create_vacuum();
    db->predefined.vacuum = peec_materials_add_material(db, &vacuum);
    if (db->predefined.vacuum >= 0) count++;
    
    peec_material_properties_t fr4 = peec_materials_create_fr4();
    db->predefined.fr4 = peec_materials_add_material(db, &fr4);
    if (db->predefined.fr4 >= 0) count++;
    
    // Add semiconductor materials
    peec_material_properties_t silicon = peec_materials_create_silicon();
    db->predefined.silicon = peec_materials_add_material(db, &silicon);
    if (db->predefined.silicon >= 0) count++;
    
    peec_material_properties_t gaas = peec_materials_create_gallium_arsenide();
    db->predefined.gallium_arsenide = peec_materials_add_material(db, &gaas);
    if (db->predefined.gallium_arsenide >= 0) count++;
    
    // Add PEC
    peec_material_properties_t pec = peec_materials_create_pec();
    db->predefined.pec = peec_materials_add_material(db, &pec);
    if (db->predefined.pec >= 0) count++;
    
    return count;
}

/**
 * @brief Find material by name
 */
int peec_materials_find_by_name(peec_material_database_t* db, const char* name) {
    if (!db || !name) return -1;
    
    for (int i = 0; i < db->num_materials; i++) {
        if (strcmp(db->materials[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Get material by ID
 */
peec_material_properties_t* peec_materials_get_by_id(peec_material_database_t* db, int id) {
    if (!db || id < 0 || id >= db->num_materials) return NULL;
    return &db->materials[id];
}

/**
 * @brief Get material by name
 */
peec_material_properties_t* peec_materials_get_by_name(peec_material_database_t* db, const char* name) {
    int id = peec_materials_find_by_name(db, name);
    return (id >= 0) ? peec_materials_get_by_id(db, id) : NULL;
}

/*********************************************************************
 * Material Property Evaluation
 *********************************************************************/

/**
 * @brief Evaluate material properties at specific frequency
 */
int peec_materials_evaluate_at_frequency(peec_material_properties_t* material, double frequency,
                                       peec_material_frequency_data_t* result) {
    if (!material || !result || frequency <= 0) return -1;
    
    result->frequency = frequency;
    
    // Handle frequency-dependent materials
    if (material->is_dispersive && material->num_frequency_points > 0 && 
        material->frequency_points && material->eps_real && material->eps_imag) {
        
        // Find frequency range for interpolation
        int idx_low = -1, idx_high = -1;
        for (int i = 0; i < material->num_frequency_points - 1; i++) {
            if (frequency >= material->frequency_points[i] && 
                frequency <= material->frequency_points[i+1]) {
                idx_low = i;
                idx_high = i+1;
                break;
            }
        }
        
        if (idx_low >= 0 && idx_high >= 0) {
            // Linear interpolation
            double f1 = material->frequency_points[idx_low];
            double f2 = material->frequency_points[idx_high];
            double alpha = (frequency - f1) / (f2 - f1);
            
            // Interpolate permittivity
            double eps_r1 = material->eps_real[idx_low];
            double eps_r2 = material->eps_real[idx_high];
            double eps_i1 = material->eps_imag[idx_low];
            double eps_i2 = material->eps_imag[idx_high];
            
            result->permittivity = (eps_r1 + alpha * (eps_r2 - eps_r1)) + 
                                   I * (eps_i1 + alpha * (eps_i2 - eps_i1));
            
            // Interpolate permeability if available
            if (material->mu_real && material->mu_imag) {
                double mu_r1 = material->mu_real[idx_low];
                double mu_r2 = material->mu_real[idx_high];
                double mu_i1 = material->mu_imag[idx_low];
                double mu_i2 = material->mu_imag[idx_high];
                
                result->permeability = (mu_r1 + alpha * (mu_r2 - mu_r1)) + 
                                      I * (mu_i1 + alpha * (mu_i2 - mu_i1));
            } else {
                result->permeability = material->permeability + 0.0*I;
            }
            
            // Conductivity typically doesn't vary much with frequency for conductors
            result->conductivity = material->conductivity;
        } else {
            // Extrapolate from nearest point
            int nearest_idx = 0;
            double min_diff = fabs(frequency - material->frequency_points[0]);
            
            for (int i = 1; i < material->num_frequency_points; i++) {
                double diff = fabs(frequency - material->frequency_points[i]);
                if (diff < min_diff) {
                    min_diff = diff;
                    nearest_idx = i;
                }
            }
            
            result->permittivity = material->eps_real[nearest_idx] + 
                                  I * material->eps_imag[nearest_idx];
            result->permeability = (material->mu_real && material->mu_imag) ? 
                                  (material->mu_real[nearest_idx] + I * material->mu_imag[nearest_idx]) :
                                  (material->permeability + 0.0*I);
            result->conductivity = material->conductivity;
        }
    } else {
        // Frequency-independent properties
        result->permittivity = material->permittivity + 0.0*I;
        result->permeability = material->permeability + 0.0*I;
        result->conductivity = material->conductivity;
    }
    
    // Calculate derived properties
    double omega = 2.0 * M_PI * frequency;
    double complex eps_complex = result->permittivity * EPS_0;
    double complex mu_complex = result->permeability * MU_0;
    
    // Skin depth for conductors
    if (result->conductivity > CONDUCTOR_THRESHOLD) {
        result->skin_depth = sqrt(2.0 / (omega * result->conductivity * creal(mu_complex)));
        
        // Surface impedance for good conductors
        double complex surface_impedance = sqrt((I * omega * mu_complex) / result->conductivity);
        result->surface_impedance_real = creal(surface_impedance);
        result->surface_impedance_imag = cimag(surface_impedance);
    } else {
        result->skin_depth = 1e10;  // Very large for dielectrics
        result->surface_impedance_real = 0.0;
        result->surface_impedance_imag = 0.0;
    }
    
    return 0;
}

/**
 * @brief Get permittivity at frequency
 */
double complex peec_materials_get_permittivity(peec_material_properties_t* material, double frequency) {
    peec_material_frequency_data_t result;
    if (peec_materials_evaluate_at_frequency(material, frequency, &result) != 0) {
        return material->permittivity + 0.0*I;
    }
    return result.permittivity;
}

/**
 * @brief Get permeability at frequency
 */
double complex peec_materials_get_permeability(peec_material_properties_t* material, double frequency) {
    peec_material_frequency_data_t result;
    if (peec_materials_evaluate_at_frequency(material, frequency, &result) != 0) {
        return material->permeability + 0.0*I;
    }
    return result.permeability;
}

/**
 * @brief Get conductivity at frequency
 */
double peec_materials_get_conductivity(peec_material_properties_t* material, double frequency) {
    peec_material_frequency_data_t result;
    if (peec_materials_evaluate_at_frequency(material, frequency, &result) != 0) {
        return material->conductivity;
    }
    return result.conductivity;
}

/**
 * @brief Get skin depth at frequency
 */
double peec_materials_get_skin_depth(peec_material_properties_t* material, double frequency) {
    peec_material_frequency_data_t result;
    if (peec_materials_evaluate_at_frequency(material, frequency, &result) != 0) {
        return 1e10;  // Very large for dielectrics
    }
    return result.skin_depth;
}

/**
 * @brief Get surface impedance at frequency
 */
double complex peec_materials_get_surface_impedance(peec_material_properties_t* material, double frequency) {
    peec_material_frequency_data_t result;
    if (peec_materials_evaluate_at_frequency(material, frequency, &result) != 0) {
        return 0.0 + 0.0*I;
    }
    return result.surface_impedance_real + I * result.surface_impedance_imag;
}

/*********************************************************************
 * Predefined Material Creation
 *********************************************************************/

/**
 * @brief Create copper material
 */
peec_material_properties_t peec_materials_create_copper(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Copper");
    material.type = MATERIAL_TYPE_CONDUCTOR;
    material.conductivity = 5.8e7;      // S/m
    material.permittivity = 1.0;        // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0;
    material.density = 8960.0;          // kg/m³
    material.thermal_conductivity = 401.0; // W/(m·K)
    material.temperature_coefficient = 0.00393; // 1/K
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 1e-6;  // 1 μm RMS
    material.is_dispersive = false;
    material.temperature_dependent = true;
    
    return material;
}

/**
 * @brief Create aluminum material
 */
peec_material_properties_t peec_materials_create_aluminum(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Aluminum");
    material.type = MATERIAL_TYPE_CONDUCTOR;
    material.conductivity = 3.5e7;      // S/m
    material.permittivity = 1.0;        // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0;
    material.density = 2700.0;          // kg/m³
    material.thermal_conductivity = 237.0; // W/(m·K)
    material.temperature_coefficient = 0.00429; // 1/K
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 2e-6;  // 2 μm RMS
    material.is_dispersive = false;
    material.temperature_dependent = true;
    
    return material;
}

/**
 * @brief Create silver material
 */
peec_material_properties_t peec_materials_create_silver(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Silver");
    material.type = MATERIAL_TYPE_CONDUCTOR;
    material.conductivity = 6.3e7;      // S/m
    material.permittivity = 1.0;        // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0;
    material.density = 10500.0;         // kg/m³
    material.thermal_conductivity = 429.0; // W/(m·K)
    material.temperature_coefficient = 0.00380; // 1/K
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 0.5e-6; // 0.5 μm RMS
    material.is_dispersive = false;
    material.temperature_dependent = true;
    
    return material;
}

/**
 * @brief Create gold material
 */
peec_material_properties_t peec_materials_create_gold(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Gold");
    material.type = MATERIAL_TYPE_CONDUCTOR;
    material.conductivity = 4.1e7;      // S/m
    material.permittivity = 1.0;        // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0;
    material.density = 19300.0;         // kg/m³
    material.thermal_conductivity = 317.0; // W/(m·K)
    material.temperature_coefficient = 0.00340; // 1/K
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 0.3e-6; // 0.3 μm RMS
    material.is_dispersive = false;
    material.temperature_dependent = true;
    
    return material;
}

/**
 * @brief Create steel material
 */
peec_material_properties_t peec_materials_create_steel(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Steel");
    material.type = MATERIAL_TYPE_CONDUCTOR;
    material.conductivity = 1.0e6;      // S/m (varies by type)
    material.permittivity = 1.0;        // Relative
    material.permeability = 100.0;      // Relative (ferromagnetic)
    material.loss_tangent = 0.0;
    material.density = 7850.0;          // kg/m³
    material.thermal_conductivity = 50.0; // W/(m·K)
    material.temperature_coefficient = 0.00500; // 1/K (approximate)
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 5e-6;  // 5 μm RMS
    material.is_dispersive = false;
    material.temperature_dependent = true;
    
    return material;
}

/**
 * @brief Create PEC material
 */
peec_material_properties_t peec_materials_create_pec(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "PEC");
    material.type = MATERIAL_TYPE_PEC;
    material.conductivity = 1e20;       // S/m (effectively infinite)
    material.permittivity = 1.0;        // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0;
    material.density = 1e10;              // kg/m³ (arbitrary large)
    material.thermal_conductivity = 1e10; // W/(m·K) (arbitrary large)
    material.temperature_coefficient = 0.0;
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 0.0;   // Perfectly smooth
    material.is_dispersive = false;
    material.temperature_dependent = false;
    
    return material;
}

/**
 * @brief Create air material
 */
peec_material_properties_t peec_materials_create_air(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Air");
    material.type = MATERIAL_TYPE_DIELECTRIC;
    material.conductivity = 0.0;        // S/m (ideal)
    material.permittivity = 1.00059;    // Relative (at STP)
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0;
    material.density = 1.225;           // kg/m³ (at STP)
    material.thermal_conductivity = 0.025; // W/(m·K)
    material.temperature_coefficient = 0.0;
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 0.0;
    material.is_dispersive = false;
    material.temperature_dependent = false;
    
    return material;
}

/**
 * @brief Create vacuum material
 */
peec_material_properties_t peec_materials_create_vacuum(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Vacuum");
    material.type = MATERIAL_TYPE_DIELECTRIC;
    material.conductivity = 0.0;        // S/m
    material.permittivity = 1.0;        // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0;
    material.density = 0.0;               // kg/m³
    material.thermal_conductivity = 0.0;  // W/(m·K)
    material.temperature_coefficient = 0.0;
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 0.0;
    material.is_dispersive = false;
    material.temperature_dependent = false;
    
    return material;
}

/**
 * @brief Create FR4 material
 */
peec_material_properties_t peec_materials_create_fr4(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "FR4");
    material.type = MATERIAL_TYPE_DIELECTRIC;
    material.conductivity = 1e-14;      // S/m (very low)
    material.permittivity = 4.3;        // Relative (at 1 MHz)
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.02;       // At 1 MHz
    material.density = 1850.0;          // kg/m³
    material.thermal_conductivity = 0.3; // W/(m·K)
    material.temperature_coefficient = 0.0;
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 1e-6;  // 1 μm RMS
    material.is_dispersive = true;      // Frequency-dependent
    material.temperature_dependent = false;
    
    return material;
}

/**
 * @brief Create silicon material
 */
peec_material_properties_t peec_materials_create_silicon(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "Silicon");
    material.type = MATERIAL_TYPE_SEMICONDUCTOR;
    material.conductivity = 1e-3;       // S/m (depends on doping)
    material.permittivity = 11.7;       // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.001;      // Low loss at RF
    material.density = 2330.0;          // kg/m³
    material.thermal_conductivity = 130.0; // W/(m·K)
    material.temperature_coefficient = 0.0;
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 0.1e-6; // 0.1 μm RMS
    material.is_dispersive = false;
    material.temperature_dependent = false;
    
    return material;
}

/**
 * @brief Create gallium arsenide material
 */
peec_material_properties_t peec_materials_create_gallium_arsenide(void) {
    peec_material_properties_t material = {0};
    strcpy(material.name, "GalliumArsenide");
    material.type = MATERIAL_TYPE_SEMICONDUCTOR;
    material.conductivity = 1e-6;       // S/m (semi-insulating)
    material.permittivity = 12.9;       // Relative
    material.permeability = 1.0;        // Relative
    material.loss_tangent = 0.0001;     // Very low loss
    material.density = 5320.0;          // kg/m³
    material.thermal_conductivity = 55.0; // W/(m·K)
    material.temperature_coefficient = 0.0;
    material.reference_temperature = 293.15; // K (20°C)
    material.surface_roughness = 0.05e-6; // 0.05 μm RMS
    material.is_dispersive = false;
    material.temperature_dependent = false;
    
    return material;
}

/*********************************************************************
 * Material Classification
 *********************************************************************/

/**
 * @brief Check if material is a good conductor
 */
bool peec_materials_is_good_conductor(peec_material_properties_t* material, double frequency) {
    if (!material || frequency <= 0) return false;
    
    double omega = 2.0 * M_PI * frequency;
    double complex eps_complex = (material->permittivity + 0.0*I) * EPS_0;
    double complex mu_complex = (material->permeability + 0.0*I) * MU_0;
    
    // Loss tangent comparison
    double conduction_loss = material->conductivity / (omega * cimag(eps_complex) + 1e-20);
    double dielectric_loss = material->loss_tangent;
    
    return (conduction_loss > 100.0) || (material->conductivity > CONDUCTOR_THRESHOLD);
}

/**
 * @brief Check if material is a good dielectric
 */
bool peec_materials_is_good_dielectric(peec_material_properties_t* material, double frequency) {
    if (!material || frequency <= 0) return false;
    
    return !peec_materials_is_good_conductor(material, frequency) && 
           (material->conductivity < 1.0);
}

/**
 * @brief Check if material is magnetic
 */
bool peec_materials_is_magnetic(peec_material_properties_t* material, double frequency) {
    if (!material) return false;
    
    return (material->permeability > 1.1) || (material->permeability < 0.9);
}

/**
 * @brief Check if material is dispersive
 */
bool peec_materials_is_dispersive(peec_material_properties_t* material, double frequency_range[2]) {
    if (!material) return false;
    
    return material->is_dispersive && (material->num_frequency_points > 0);
}

/*********************************************************************
 * Utility Functions
 *********************************************************************/

/**
 * @brief Print material database
 */
void peec_materials_print_database(peec_material_database_t* db) {
    if (!db) return;
    
    printf("=== PEEC Material Database ===\n");
    printf("Total materials: %d (max: %d)\n", db->num_materials, db->max_materials);
    printf("\nPredefined materials:\n");
    printf("  Copper: %d\n", db->predefined.copper);
    printf("  Aluminum: %d\n", db->predefined.aluminum);
    printf("  Silver: %d\n", db->predefined.silver);
    printf("  Gold: %d\n", db->predefined.gold);
    printf("  Steel: %d\n", db->predefined.steel);
    printf("  PEC: %d\n", db->predefined.pec);
    printf("  Air: %d\n", db->predefined.air);
    printf("  Vacuum: %d\n", db->predefined.vacuum);
    printf("  FR4: %d\n", db->predefined.fr4);
    printf("  Silicon: %d\n", db->predefined.silicon);
    printf("  GaAs: %d\n", db->predefined.gallium_arsenide);
    
    printf("\nAll materials:\n");
    for (int i = 0; i < db->num_materials; i++) {
        peec_materials_print_material(&db->materials[i]);
    }
}

/**
 * @brief Print material properties
 */
void peec_materials_print_material(peec_material_properties_t* material) {
    if (!material) return;
    
    printf("Material: %s (ID: %d, Type: %s)\n", 
           material->name, material->id, peec_materials_get_type_name(material->type));
    printf("  Conductivity: %.3e S/m\n", material->conductivity);
    printf("  Permittivity: %.3f (relative)\n", material->permittivity);
    printf("  Permeability: %.3f (relative)\n", material->permeability);
    printf("  Loss tangent: %.3e\n", material->loss_tangent);
    printf("  Density: %.1f kg/m³\n", material->density);
    printf("  Thermal conductivity: %.1f W/(m·K)\n", material->thermal_conductivity);
    printf("  Surface roughness: %.1e m\n", material->surface_roughness);
    printf("  Dispersive: %s\n", material->is_dispersive ? "Yes" : "No");
    printf("  Temperature dependent: %s\n", material->temperature_dependent ? "Yes" : "No");
    
    if (material->num_frequency_points > 0) {
        printf("  Frequency points: %d (%.1e to %.1e Hz)\n", 
               material->num_frequency_points,
               material->frequency_points[0],
               material->frequency_points[material->num_frequency_points-1]);
    }
}

/**
 * @brief Get material type name
 */
const char* peec_materials_get_type_name(peec_material_type_t type) {
    switch (type) {
        case MATERIAL_TYPE_CONDUCTOR: return "Conductor";
        case MATERIAL_TYPE_DIELECTRIC: return "Dielectric";
        case MATERIAL_TYPE_SEMICONDUCTOR: return "Semiconductor";
        case MATERIAL_TYPE_MAGNETIC: return "Magnetic";
        case MATERIAL_TYPE_LOSSY: return "Lossy";
        case MATERIAL_TYPE_ANISOTROPIC: return "Anisotropic";
        case MATERIAL_TYPE_DISPERSIVE: return "Dispersive";
        case MATERIAL_TYPE_PEC: return "PEC";
        case MATERIAL_TYPE_PMC: return "PMC";
        default: return "Unknown";
    }
}