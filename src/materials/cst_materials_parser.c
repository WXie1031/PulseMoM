/**
 * @file cst_materials_parser.c
 * @brief CST material file parser implementation
 * @details Parses CST Studio Suite .mtd material files
 */

#include "cst_materials_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#define MU_0 (4.0 * M_PI * 1e-7)
#define EPS_0 (8.854187817e-12)

/**
 * @brief Trim whitespace from string
 */
static char* trim_whitespace(char* str) {
    char* end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

/**
 * @brief Extract quoted string from line
 */
static int extract_quoted_string(const char* line, char* result, int max_len) {
    const char* start = strchr(line, '"');
    if (!start) return -1;
    start++;
    const char* end = strchr(start, '"');
    if (!end) return -1;
    
    int len = end - start;
    if (len >= max_len) len = max_len - 1;
    strncpy(result, start, len);
    result[len] = '\0';
    return 0;
}

/**
 * @brief Parse CST material file
 */
cst_material_t* cst_materials_parse_file(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open CST material file: %s\n", filename);
        return NULL;
    }
    
    cst_material_t* material = (cst_material_t*)calloc(1, sizeof(cst_material_t));
    if (!material) {
        fclose(fp);
        return NULL;
    }
    
    // Initialize defaults
    strcpy(material->name, "Unknown");
    strcpy(material->filename, filename);
    strcpy(material->type, CST_TYPE_NORMAL);
    strcpy(material->frq_type, CST_FRQTYPE_ALL);
    strcpy(material->units.frequency, CST_UNIT_GHZ);
    strcpy(material->units.geometry, CST_UNIT_MM);
    strcpy(material->units.time, CST_UNIT_NS);
    strcpy(material->units.temperature, CST_UNIT_KELVIN);
    
    material->epsilon = 1.0;
    material->mu = 1.0;
    material->sigma = 0.0;
    material->sigma_m = 0.0;
    material->tan_d = 0.0;
    material->tan_d_m = 0.0;
    material->tan_d_freq = 0.0;
    material->tan_d_m_freq = 0.0;
    material->tan_d_given = false;
    material->tan_d_m_given = false;
    
    strcpy(material->tan_d_model, "ConstTanD");
    strcpy(material->tan_d_m_model, "ConstTanD");
    strcpy(material->disp_model_eps, "None");
    strcpy(material->disp_model_mu, "None");
    strcpy(material->dispersive_fitting_scheme_eps, "Nth Order");
    strcpy(material->dispersive_fitting_scheme_mu, "Nth Order");
    material->use_general_dispersion_eps = false;
    material->use_general_dispersion_mu = false;
    
    material->rho = 0.0;
    material->thermal_conductivity = 0.0;
    material->specific_heat = 0.0;
    material->thermal_expansion_rate = 0.0;
    material->youngs_modulus = 0.0;
    material->poissons_ratio = 0.0;
    
    strcpy(material->thermal_type, "Normal");
    strcpy(material->mechanics_type, "Isotropic");
    
    material->colour_r = 0.8;
    material->colour_g = 0.8;
    material->colour_b = 0.8;
    material->wireframe = false;
    material->transparency = false;
    material->transparency_value = 0.0;
    material->allow_outline = true;
    material->transparent_outline = false;
    
    strcpy(material->reference_coord_system, "Global");
    strcpy(material->coord_system_type, "Cartesian");
    
    material->nl_anisotropy = false;
    material->nla_stacking_factor = 1.0;
    material->nla_direction_x = 1.0;
    material->nla_direction_y = 0.0;
    material->nla_direction_z = 0.0;
    
    material->dispersive_data.frequencies = NULL;
    material->dispersive_data.epsilon_real = NULL;
    material->dispersive_data.epsilon_imag = NULL;
    material->dispersive_data.mu_real = NULL;
    material->dispersive_data.mu_imag = NULL;
    material->dispersive_data.num_points = 0;
    material->dispersive_data.max_points = 0;
    
    char line[CST_MAX_LINE_LENGTH];
    char section[64] = "";
    
    // Parse file line by line
    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = trim_whitespace(line);
        if (strlen(trimmed) == 0 || trimmed[0] == '#') continue;
        
        // Check for section headers
        if (trimmed[0] == '[' && trimmed[strlen(trimmed)-1] == ']') {
            sscanf(trimmed, "[%[^]]]", section);
            continue;
        }
        
        // Parse based on current section
        if (strcmp(section, "Definition") == 0) {
            if (strncmp(trimmed, ".FrqType", 8) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    strcpy(material->frq_type, value);
                }
            }
            else if (strncmp(trimmed, ".Type", 5) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    strcpy(material->type, value);
                }
            }
            else if (strncmp(trimmed, ".Epsilon", 8) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->epsilon = atof(value);
                }
            }
            else if (strncmp(trimmed, ".Mu", 3) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->mu = atof(value);
                }
            }
            else if (strncmp(trimmed, ".Sigma", 6) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->sigma = atof(value);
                }
            }
            else if (strncmp(trimmed, ".Kappa", 6) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->sigma = atof(value);
                }
            }
            else if (strncmp(trimmed, ".TanD", 5) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->tan_d = atof(value);
                    material->tan_d_given = true;
                }
            }
            else if (strncmp(trimmed, ".TanDFreq", 9) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->tan_d_freq = atof(value);
                }
            }
            else if (strncmp(trimmed, ".Rho", 4) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->rho = atof(value);
                }
            }
            else if (strncmp(trimmed, ".ThermalConductivity", 20) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->thermal_conductivity = atof(value);
                }
            }
            else if (strncmp(trimmed, ".SpecificHeat", 13) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->specific_heat = atof(value);
                }
            }
            else if (strncmp(trimmed, ".YoungsModulus", 14) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->youngs_modulus = atof(value);
                }
            }
            else if (strncmp(trimmed, ".PoissonsRatio", 14) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->poissons_ratio = atof(value);
                }
            }
            else if (strncmp(trimmed, ".ThermalExpansionRate", 20) == 0) {
                char value[64];
                if (extract_quoted_string(trimmed, value, sizeof(value)) == 0) {
                    material->thermal_expansion_rate = atof(value);
                }
            }
            else if (strncmp(trimmed, ".Colour", 7) == 0) {
                char r[64], g[64], b[64];
                if (sscanf(trimmed, ".Colour \"%[^\"]\" , \"%[^\"]\" , \"%[^\"]\"", r, g, b) == 3) {
                    material->colour_r = atof(r);
                    material->colour_g = atof(g);
                    material->colour_b = atof(b);
                }
            }
        }
    }
    
    fclose(fp);
    
    // Extract material name from filename
    const char* basename = strrchr(filename, '/');
    if (!basename) basename = strrchr(filename, '\\');
    if (!basename) basename = filename; else basename++;
    
    // Remove .mtd extension
    char* name_copy = strdup(basename);
    char* ext = strrchr(name_copy, '.');
    if (ext && strcmp(ext, ".mtd") == 0) *ext = '\0';
    strncpy(material->name, name_copy, CST_MAX_NAME_LENGTH - 1);
    material->name[CST_MAX_NAME_LENGTH - 1] = '\0';
    free(name_copy);
    
    return material;
}

/**
 * @brief Create material database
 */
cst_material_database_t* cst_materials_create_database(int max_materials, const char* library_path) {
    cst_material_database_t* db = (cst_material_database_t*)calloc(1, sizeof(cst_material_database_t));
    if (!db) return NULL;
    
    db->max_materials = max_materials;
    db->materials = (cst_material_t*)calloc(max_materials, sizeof(cst_material_t));
    if (!db->materials) {
        free(db);
        return NULL;
    }
    
    db->num_materials = 0;
    strncpy(db->library_path, library_path, CST_MAX_PATH_LENGTH - 1);
    db->library_path[CST_MAX_PATH_LENGTH - 1] = '\0';
    
    return db;
}

/**
 * @brief Destroy material database
 */
void cst_materials_destroy_database(cst_material_database_t* db) {
    if (!db) return;
    
    if (db->materials) {
        for (int i = 0; i < db->num_materials; i++) {
            cst_material_t* mat = &db->materials[i];
            if (mat->dispersive_data.frequencies) free(mat->dispersive_data.frequencies);
            if (mat->dispersive_data.epsilon_real) free(mat->dispersive_data.epsilon_real);
            if (mat->dispersive_data.epsilon_imag) free(mat->dispersive_data.epsilon_imag);
            if (mat->dispersive_data.mu_real) free(mat->dispersive_data.mu_real);
            if (mat->dispersive_data.mu_imag) free(mat->dispersive_data.mu_imag);
        }
        free(db->materials);
    }
    
    free(db);
}

/**
 * @brief Load material from library
 */
int cst_materials_load_from_library(cst_material_database_t* db, const char* material_name) {
    if (!db || !material_name || db->num_materials >= db->max_materials) return -1;
    
    char filepath[CST_MAX_PATH_LENGTH];
    snprintf(filepath, sizeof(filepath), "%s/%s.mtd", db->library_path, material_name);
    
    cst_material_t* parsed = cst_materials_parse_file(filepath);
    if (!parsed) return -1;
    
    // Copy to database
    cst_materials_copy_material(&db->materials[db->num_materials], parsed);
    db->num_materials++;
    
    free(parsed);
    return 0;
}

/**
 * @brief Copy material
 */
void cst_materials_copy_material(cst_material_t* dest, const cst_material_t* src) {
    if (!dest || !src) return;
    
    *dest = *src;
    
    // Deep copy dispersive data
    if (src->dispersive_data.num_points > 0) {
        int n = src->dispersive_data.num_points;
        dest->dispersive_data.frequencies = (double*)malloc(n * sizeof(double));
        dest->dispersive_data.epsilon_real = (double*)malloc(n * sizeof(double));
        dest->dispersive_data.epsilon_imag = (double*)malloc(n * sizeof(double));
        dest->dispersive_data.mu_real = (double*)malloc(n * sizeof(double));
        dest->dispersive_data.mu_imag = (double*)malloc(n * sizeof(double));
        
        if (dest->dispersive_data.frequencies && dest->dispersive_data.epsilon_real) {
            memcpy(dest->dispersive_data.frequencies, src->dispersive_data.frequencies, n * sizeof(double));
            memcpy(dest->dispersive_data.epsilon_real, src->dispersive_data.epsilon_real, n * sizeof(double));
            memcpy(dest->dispersive_data.epsilon_imag, src->dispersive_data.epsilon_imag, n * sizeof(double));
            memcpy(dest->dispersive_data.mu_real, src->dispersive_data.mu_real, n * sizeof(double));
            memcpy(dest->dispersive_data.mu_imag, src->dispersive_data.mu_imag, n * sizeof(double));
        }
    }
}

/**
 * @brief Get complex permittivity at frequency
 */
double complex cst_materials_get_epsilon(const cst_material_t* material, double frequency) {
    if (!material) return 1.0 + 0.0*I;
    
    double omega = 2 * M_PI * frequency;
    double complex eps = material->epsilon + 0.0*I;
    
    // Add conductivity contribution
    if (material->sigma > 0) {
        eps = eps - I * material->sigma / (EPS_0 * omega);
    }
    
    // Add loss tangent contribution if given
    if (material->tan_d_given && material->tan_d > 0) {
        eps = eps * (1 - I * material->tan_d);
    }
    
    return eps;
}

/**
 * @brief Get complex permeability at frequency
 */
double complex cst_materials_get_mu(const cst_material_t* material, double frequency) {
    if (!material) return 1.0 + 0.0*I;
    
    double complex mu = material->mu + 0.0*I;
    
    // Add magnetic loss tangent if given
    if (material->tan_d_m_given && material->tan_d_m > 0) {
        mu = mu * (1 - I * material->tan_d_m);
    }
    
    return mu;
}

/**
 * @brief Get conductivity at frequency
 */
double cst_materials_get_conductivity(const cst_material_t* material, double frequency) {
    if (!material) return 0.0;
    return material->sigma;
}

/**
 * @brief Get skin depth at frequency
 */
double cst_materials_get_skin_depth(const cst_material_t* material, double frequency) {
    if (!material || frequency <= 0) return 0.0;
    
    double omega = 2 * M_PI * frequency;
    double complex eps = cst_materials_get_epsilon(material, frequency);
    double complex mu = cst_materials_get_mu(material, frequency);
    
    if (material->sigma > 1e6) {  // Good conductor
        return sqrt(2.0 / (omega * creal(mu) * MU_0 * material->sigma));
    }
    
    return 0.0;  // Not a good conductor
}

/**
 * @brief Print material information
 */
void cst_materials_print_material(const cst_material_t* material) {
    if (!material) return;
    
    printf("CST Material: %s\n", material->name);
    printf("  Type: %s (%s)\n", material->type, material->frq_type);
    printf("  Electromagnetic Properties:\n");
    printf("    Epsilon: %.3f\n", material->epsilon);
    printf("    Mu: %.3f\n", material->mu);
    printf("    Conductivity: %.2e S/m\n", material->sigma);
    if (material->tan_d_given) {
        printf("    Loss Tangent: %.4f @ %.1f GHz\n", material->tan_d, material->tan_d_freq);
    }
    
    printf("  Physical Properties:\n");
    if (material->rho > 0) printf("    Density: %.1f kg/m³\n", material->rho);
    if (material->thermal_conductivity > 0) printf("    Thermal Conductivity: %.1f W/K/m\n", material->thermal_conductivity);
    if (material->specific_heat > 0) printf("    Specific Heat: %.1f J/K/kg\n", material->specific_heat);
    
    printf("  Color: RGB(%.2f, %.2f, %.2f)\n", material->colour_r, material->colour_g, material->colour_b);
    printf("  Coordinate System: %s (%s)\n", material->reference_coord_system, material->coord_system_type);
}

/**
 * @brief Create predefined copper material
 */
cst_material_t* cst_materials_create_copper(void) {
    cst_material_t* copper = (cst_material_t*)calloc(1, sizeof(cst_material_t));
    if (!copper) return NULL;
    
    strcpy(copper->name, "Copper");
    strcpy(copper->type, CST_TYPE_LOSSY_METAL);
    strcpy(copper->frq_type, CST_FRQTYPE_ALL);
    
    copper->epsilon = 1.0;
    copper->mu = 1.0;
    copper->sigma = 5.96e7;  // S/m
    copper->rho = 8930.0;    // kg/m³
    copper->thermal_conductivity = 401.0;  // W/K/m
    copper->specific_heat = 390.0;  // J/K/kg
    copper->youngs_modulus = 120.0;  // kN/mm²
    copper->poissons_ratio = 0.33;
    copper->thermal_expansion_rate = 17.0;  // 1e-6/K
    
    copper->colour_r = 1.0;
    copper->colour_g = 0.8;
    copper->colour_b = 0.0;
    
    return copper;
}

/**
 * @brief Create predefined PEC material
 */
cst_material_t* cst_materials_create_pec(void) {
    cst_material_t* pec = (cst_material_t*)calloc(1, sizeof(cst_material_t));
    if (!pec) return NULL;
    
    strcpy(pec->name, "PEC");
    strcpy(pec->type, CST_TYPE_PEC);
    strcpy(pec->frq_type, CST_FRQTYPE_ALL);
    
    pec->epsilon = 1.0;
    pec->mu = 1.0;
    pec->sigma = 1e20;  // Very high conductivity
    
    pec->colour_r = 0.8;
    pec->colour_g = 0.8;
    pec->colour_b = 0.8;
    
    return pec;
}

/**
 * @brief Create predefined FR4 material
 */
cst_material_t* cst_materials_create_fr4(void) {
    cst_material_t* fr4 = (cst_material_t*)calloc(1, sizeof(cst_material_t));
    if (!fr4) return NULL;
    
    strcpy(fr4->name, "FR4");
    strcpy(fr4->type, CST_TYPE_NORMAL);
    strcpy(fr4->frq_type, CST_FRQTYPE_ALL);
    
    fr4->epsilon = 4.3;
    fr4->mu = 1.0;
    fr4->sigma = 0.0;
    fr4->tan_d = 0.025;
    fr4->tan_d_freq = 10.0;  // 10 GHz
    fr4->tan_d_given = true;
    
    fr4->thermal_conductivity = 0.3;  // W/K/m
    
    fr4->colour_r = 0.94;
    fr4->colour_g = 0.82;
    fr4->colour_b = 0.76;
    
    return fr4;
}

/**
 * @brief Material classification
 */
bool cst_materials_is_conductor(const cst_material_t* material, double frequency) {
    if (!material) return false;
    return material->sigma > 1e6;  // S/m threshold
}

bool cst_materials_is_dielectric(const cst_material_t* material, double frequency) {
    if (!material) return false;
    return material->sigma < 1e-2 && material->epsilon > 1.0;
}

bool cst_materials_is_magnetic(const cst_material_t* material, double frequency) {
    if (!material) return false;
    return material->mu > 1.1;  // Significantly different from 1
}

bool cst_materials_is_dispersive(const cst_material_t* material) {
    if (!material) return false;
    return material->dispersive_data.num_points > 0;
}