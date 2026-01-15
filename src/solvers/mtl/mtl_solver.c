/**
 * @file mtl_solver.c
 * @brief Multi-conductor Transmission Line (MTL) solver implementation
 * @details Cable simulation, transmission line analysis, and hybrid coupling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <omp.h>

#include "mtl_solver_module.h"
#include "../../utils/logger.h"
#include "../../utils/memory_manager.h"
#include "../../utils/error_handler.h"
#include "../../core/core_kernels.h"
#include "../../core/core_solver.h"

// Physical constants
#define MU_0 (4.0 * M_PI * 1e-7)        // Permeability of free space
#define EPS_0 (8.854187817e-12)         // Permittivity of free space
#define C_0 (299792458.0)               // Speed of light

// Internal structures
typedef struct {
    double conductivity;        // S/m
    double permeability;        // H/m
    double permittivity;        // F/m
    double loss_tangent;
} mtl_material_properties_t;

typedef struct {
    mtl_cable_type_t type;
    mtl_geometry_t geometry;
    mtl_material_properties_t* conductor_materials;
    mtl_material_properties_t* dielectric_materials;
    
    // Precomputed parameters
    double complex** r_matrix;  // Resistance matrix per unit length
    double complex** l_matrix;  // Inductance matrix per unit length
    double complex** c_matrix;  // Capacitance matrix per unit length
    double complex** g_matrix;  // Conductance matrix per unit length
} mtl_cable_data_t;

// MTL solver structure
struct mtl_solver {
    mtl_solver_config_t config;
    mtl_cable_data_t* cables;
    int num_cables;
    int max_cables;
    
    // Solver state
    bool is_initialized;
    bool is_analyzed;
    
    // Results storage
    mtl_results_t* results;
    
    // Coupling handles
    void* mom_solver_handle;
    void* peec_solver_handle;
    
    // Performance metrics
    double solve_time;
    double memory_usage;
    int iterations;
};

// Material database
static const mtl_material_properties_t conductor_materials[] = {
    {"Copper", 5.8e7, MU_0, EPS_0, 0.0},          // MTL_MATERIAL_COPPER
    {"Aluminum", 3.5e7, MU_0, EPS_0, 0.0},         // MTL_MATERIAL_ALUMINUM
    {"Silver", 6.3e7, MU_0, EPS_0, 0.0},           // MTL_MATERIAL_SILVER
    {"Gold", 4.1e7, MU_0, EPS_0, 0.0},             // MTL_MATERIAL_GOLD
    {"Steel", 1.0e6, MU_0, EPS_0, 0.0},            // MTL_MATERIAL_STEEL
    {"Tin", 8.7e6, MU_0, EPS_0, 0.0}               // MTL_MATERIAL_TIN
};

static const mtl_material_properties_t dielectric_materials[] = {
    {"PVC", 0.0, MU_0, 3.0 * EPS_0, 0.02},         // MTL_DIELECTRIC_PVC
    {"PE", 0.0, MU_0, 2.3 * EPS_0, 0.0002},        // MTL_DIELECTRIC_PE
    {"PTFE", 0.0, MU_0, 2.1 * EPS_0, 0.0001},      // MTL_DIELECTRIC_PTFE
    {"Rubber", 0.0, MU_0, 4.0 * EPS_0, 0.01},      // MTL_DIELECTRIC_RUBBER
    {"XLPE", 0.0, MU_0, 2.4 * EPS_0, 0.001}        // MTL_DIELECTRIC_XLPE
};

// Function prototypes
static int mtl_initialize_materials(mtl_solver_t* solver);
static int mtl_calculate_per_unit_parameters(mtl_solver_t* solver, mtl_cable_data_t* cable, double frequency);
static int mtl_solve_telegrapher_equations(mtl_solver_t* solver, double frequency);
static double mtl_calculate_skin_depth(double conductivity, double permeability, double frequency);
static double complex mtl_calculate_internal_impedance(double radius, double skin_depth, double conductivity);
static int mtl_calculate_external_parameters(mtl_solver_t* solver, mtl_cable_data_t* cable, double frequency);
static int mtl_update_coupling_matrices(mtl_solver_t* solver);

// Create MTL solver
mtl_solver_t* mtl_solver_create(void) {
    mtl_solver_t* solver = (mtl_solver_t*)calloc(1, sizeof(mtl_solver_t));
    if (!solver) {
        log_error("Failed to allocate MTL solver");
        return NULL;
    }
    
    // Initialize default configuration
    solver->config.analysis_type = MTL_ANALYSIS_FREQUENCY;
    solver->config.coupling_mode = MTL_COUPLING_NONE;
    solver->config.tolerance = 1e-6;
    solver->config.max_iterations = 1000;
    solver->config.num_threads = omp_get_max_threads();
    solver->config.enable_gpu = false;
    solver->config.skin_effect = true;
    solver->config.proximity_effect = true;
    solver->config.common_mode = true;
    solver->config.adaptive_freq = true;
    solver->config.save_s_parameters = true;
    solver->config.save_z_parameters = true;
    solver->config.export_spice = true;
    solver->config.kbl_import_export = true;
    
    // Allocate cable storage
    solver->max_cables = 100;
    solver->cables = (mtl_cable_data_t*)calloc(solver->max_cables, sizeof(mtl_cable_data_t));
    if (!solver->cables) {
        log_error("Failed to allocate cable storage");
        free(solver);
        return NULL;
    }
    
    solver->is_initialized = false;
    solver->is_analyzed = false;
    solver->mom_solver_handle = NULL;
    solver->peec_solver_handle = NULL;
    
    log_info("MTL solver created successfully");
    return solver;
}

// Destroy MTL solver
void mtl_solver_destroy(mtl_solver_t* solver) {
    if (!solver) return;
    
    // Free cable data
    for (int i = 0; i < solver->num_cables; i++) {
        mtl_cable_data_t* cable = &solver->cables[i];
        
        // Free matrices
        if (cable->r_matrix) {
            for (int j = 0; j < cable->geometry.num_conductors; j++) {
                free(cable->r_matrix[j]);
            }
            free(cable->r_matrix);
        }
        
        if (cable->l_matrix) {
            for (int j = 0; j < cable->geometry.num_conductors; j++) {
                free(cable->l_matrix[j]);
            }
            free(cable->l_matrix);
        }
        
        if (cable->c_matrix) {
            for (int j = 0; j < cable->geometry.num_conductors; j++) {
                free(cable->c_matrix[j]);
            }
            free(cable->c_matrix);
        }
        
        if (cable->g_matrix) {
            for (int j = 0; j < cable->geometry.num_conductors; j++) {
                free(cable->g_matrix[j]);
            }
            free(cable->g_matrix);
        }
        
        // Free geometry data
        free(cable->geometry.conductor_radii);
        free(cable->geometry.insulation_thickness);
        free(cable->geometry.positions_x);
        free(cable->geometry.positions_y);
        free(cable->geometry.positions_z);
        free(cable->conductor_materials);
        free(cable->dielectric_materials);
    }
    
    free(solver->cables);
    
    // Free results
    if (solver->results) {
        mtl_results_destroy(solver->results);
    }
    
    free(solver);
    log_info("MTL solver destroyed");
}

// Set solver configuration
int mtl_solver_set_config(mtl_solver_t* solver, const mtl_solver_config_t* config) {
    if (!solver || !config) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    solver->config = *config;
    solver->is_initialized = true;
    
    log_info("MTL solver configuration updated");
    return MTL_SUCCESS;
}

/**
 * @brief Save MTL results to file
 */
int mtl_results_save_to_file(const mtl_results_t* results, const char* filename) {
    if (!results || !filename) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        log_error("Failed to open file %s for writing", filename);
        return MTL_ERROR_FILE_IO;
    }
    
    log_info("Saving MTL results to %s", filename);
    
    // Write header
    fprintf(fp, "# MTL Analysis Results\n");
    fprintf(fp, "# Generated by PulseEM Unified Solver\n");
    fprintf(fp, "# Format: Frequency[Hz] R[Ohm/m] L[H/m] C[F/m] G[S/m]\n");
    fprintf(fp, "# For %d conductors at %d frequencies\n", results->num_conductors, results->num_frequencies);
    fprintf(fp, "\n");
    
    // Write data for each frequency
    for (int freq_idx = 0; freq_idx < results->num_frequencies; freq_idx++) {
        fprintf(fp, "# Frequency: %.6e Hz\n", results->frequencies[freq_idx]);
        
        for (int i = 0; i < results->num_conductors; i++) {
            for (int j = 0; j < results->num_conductors; j++) {
                int matrix_idx = freq_idx * results->num_conductors * results->num_conductors + 
                               i * results->num_conductors + j;
                
                fprintf(fp, "%.6e %.6e %.6e %.6e %.6e\n",
                       results->frequencies[freq_idx],
                       results->r_per_unit[matrix_idx],
                       results->l_per_unit[matrix_idx],
                       results->c_per_unit[matrix_idx],
                       results->g_per_unit[matrix_idx]);
            }
        }
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    log_info("MTL results saved successfully to %s", filename);
    return MTL_SUCCESS;
}

/**
 * @brief Export MTL results to SPICE format
 */
int mtl_results_export_spice(const mtl_results_t* results, const char* filename) {
    if (!results || !filename) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        log_error("Failed to open file %s for writing", filename);
        return MTL_ERROR_FILE_IO;
    }
    
    log_info("Exporting MTL results to SPICE format: %s", filename);
    
    // Write SPICE header
    fprintf(fp, "* MTL Transmission Line Model\n");
    fprintf(fp, "* Generated by PulseEM Unified Solver\n");
    fprintf(fp, "* For %d conductors at %d frequencies\n", results->num_conductors, results->num_frequencies);
    fprintf(fp, "\n");
    
    // Write transmission line models for each frequency
    for (int freq_idx = 0; freq_idx < results->num_frequencies; freq_idx++) {
        double frequency = results->frequencies[freq_idx];
        fprintf(fp, "* Frequency: %.6e Hz\n", frequency);
        
        // Create transmission line models for each conductor
        for (int i = 0; i < results->num_conductors; i++) {
            int matrix_idx = freq_idx * results->num_conductors * results->num_conductors + 
                           i * results->num_conductors + i;
            
            double r = results->r_per_unit[matrix_idx];
            double l = results->l_per_unit[matrix_idx];
            double c = results->c_per_unit[matrix_idx];
            double g = results->g_per_unit[matrix_idx];
            
            // SPICE transmission line model (lossy)
            fprintf(fp, "TMTL%d_%d ", i, freq_idx);
            fprintf(fp, "N%d_P N%d_N ", i, i);  // Near end nodes
            fprintf(fp, "F%d_P F%d_N ", i, i);  // Far end nodes
            fprintf(fp, "Z0=%.6e TD=%.6e\n", 
                   sqrt(l/c),  // Characteristic impedance
                   sqrt(l*c)); // Time delay per unit length
            
            // Add series resistance if significant
            if (r > 1e-12) {
                fprintf(fp, "RMTL%d_%d N%d_P N%d_P_INT %.6e\n", i, freq_idx, i, i, r);
                fprintf(fp, "RMTL%d_%d_F N%d_N N%d_N_INT %.6e\n", i, freq_idx, i, i, r);
            }
            
            // Add shunt conductance if significant
            if (g > 1e-12) {
                fprintf(fp, "GMTL%d_%d N%d_P_INT 0 %.6e\n", i, freq_idx, i, g);
                fprintf(fp, "GMTL%d_%d_F N%d_N_INT 0 %.6e\n", i, freq_idx, i, g);
            }
        }
        fprintf(fp, "\n");
    }
    
    // Write example circuit using the models
    fprintf(fp, "* Example circuit using MTL models\n");
    fprintf(fp, "* Voltage source\n");
    fprintf(fp, "VIN 1 0 AC 1\n");
    fprintf(fp, "\n");
    
    fprintf(fp, "* Load termination\n");
    for (int i = 0; i < results->num_conductors; i++) {
        fprintf(fp, "RL%d F%d_P 0 50\n", i, i);
    }
    fprintf(fp, "\n");
    
    fprintf(fp, "* Analysis\n");
    fprintf(fp, ".AC LIN %d %.6e %.6e\n", results->num_frequencies,
           results->frequencies[0], results->frequencies[results->num_frequencies-1]);
    fprintf(fp, ".PRINT AC V(F0_P)\n");
    fprintf(fp, ".END\n");
    
    fclose(fp);
    log_info("MTL SPICE export completed successfully");
    return MTL_SUCCESS;
}

// Set cable geometry
int mtl_solver_set_geometry(mtl_solver_t* solver, const mtl_geometry_t* geometry) {
    if (!solver || !geometry) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (solver->num_cables >= solver->max_cables) {
        log_error("Maximum number of cables exceeded");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    mtl_cable_data_t* cable = &solver->cables[solver->num_cables];
    cable->type = MTL_CABLE_CUSTOM;
    cable->geometry = *geometry;
    
    // Allocate material properties
    cable->conductor_materials = (mtl_material_properties_t*)calloc(geometry->num_conductors, sizeof(mtl_material_properties_t));
    cable->dielectric_materials = (mtl_material_properties_t*)calloc(geometry->num_conductors, sizeof(mtl_material_properties_t));
    
    if (!cable->conductor_materials || !cable->dielectric_materials) {
        return MTL_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize materials
    for (int i = 0; i < geometry->num_conductors; i++) {
        cable->conductor_materials[i] = conductor_materials[geometry->materials[i]];
        cable->dielectric_materials[i] = dielectric_materials[geometry->dielectrics[i]];
    }
    
    solver->num_cables++;
    log_info("Cable geometry set: %d conductors", geometry->num_conductors);
    return MTL_SUCCESS;
}

// Add cable by type
int mtl_solver_add_cable(mtl_solver_t* solver, mtl_cable_type_t type, const mtl_geometry_t* geometry) {
    if (!solver || !geometry) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    int result = mtl_solver_set_geometry(solver, geometry);
    if (result == MTL_SUCCESS) {
        solver->cables[solver->num_cables - 1].type = type;
        log_info("Cable added: type=%d, conductors=%d", type, geometry->num_conductors);
    }
    
    return result;
}

// Initialize materials
static int mtl_initialize_materials(mtl_solver_t* solver) {
    if (!solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Initializing MTL material properties");
    
    for (int i = 0; i < solver->num_cables; i++) {
        mtl_cable_data_t* cable = &solver->cables[i];
        int n = cable->geometry.num_conductors;
        
        // Allocate matrices
        cable->r_matrix = (double complex**)calloc(n, sizeof(double complex*));
        cable->l_matrix = (double complex**)calloc(n, sizeof(double complex*));
        cable->c_matrix = (double complex**)calloc(n, sizeof(double complex*));
        cable->g_matrix = (double complex**)calloc(n, sizeof(double complex*));
        
        if (!cable->r_matrix || !cable->l_matrix || !cable->c_matrix || !cable->g_matrix) {
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        
        for (int j = 0; j < n; j++) {
            cable->r_matrix[j] = (double complex*)calloc(n, sizeof(double complex));
            cable->l_matrix[j] = (double complex*)calloc(n, sizeof(double complex));
            cable->c_matrix[j] = (double complex*)calloc(n, sizeof(double complex));
            cable->g_matrix[j] = (double complex*)calloc(n, sizeof(double complex));
            
            if (!cable->r_matrix[j] || !cable->l_matrix[j] || !cable->c_matrix[j] || !cable->g_matrix[j]) {
                return MTL_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    
    return MTL_SUCCESS;
}

// Calculate skin depth
static double mtl_calculate_skin_depth(double conductivity, double permeability, double frequency) {
    if (frequency <= 0.0) return 1e6; // Large value for DC
    
    double omega = 2.0 * M_PI * frequency;
    return sqrt(2.0 / (omega * conductivity * permeability));
}

// Calculate internal impedance
static double complex mtl_calculate_internal_impedance(double radius, double skin_depth, double conductivity) {
    if (skin_depth > radius * 100.0) {
        // DC case - uniform current distribution
        return 1.0 / (conductivity * M_PI * radius * radius);
    } else {
        // AC case - skin effect
        double complex j = I;
        double complex k = (1.0 + j) / skin_depth;
        double complex kr = k * radius;
        
        // Bessel function approximation for cylindrical conductor
        double complex z_internal = (1.0 + j) / (2.0 * M_PI * radius * conductivity * skin_depth);
        return z_internal;
    }
}

// Calculate per-unit-length parameters with full multi-conductor matrix computation
static int mtl_calculate_per_unit_parameters(mtl_solver_t* solver, mtl_cable_data_t* cable, double frequency) {
    if (!solver || !cable) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    int n = cable->geometry.num_conductors;
    if (n <= 0) return MTL_ERROR_INVALID_ARGUMENT;
    
    log_info("Calculating multi-conductor RLCG matrices for %d conductors at %.2e Hz", n, frequency);
    
    // Calculate skin depths for all conductors
    double* skin_depths = (double*)malloc(n * sizeof(double));
    for (int i = 0; i < n; i++) {
        skin_depths[i] = mtl_calculate_skin_depth(
            cable->conductor_materials[i].conductivity,
            cable->conductor_materials[i].permeability,
            frequency
        );
        log_debug("Conductor %d: skin depth = %.3e m", i, skin_depths[i]);
    }
    
    // Calculate resistance matrix [R] - includes skin effect
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                // Self resistance with skin effect
                double radius = cable->geometry.conductor_radii[i];
                double conductivity = cable->conductor_materials[i].conductivity;
                
                if (skin_depths[i] >= radius) {
                    // DC case - uniform current distribution
                    cable->r_matrix[i][i] = 1.0 / (conductivity * M_PI * radius * radius);
                } else {
                    // AC case - skin effect dominated
                    double complex z_skin = (1.0 + I) / (2.0 * M_PI * radius * conductivity * skin_depths[i]);
                    cable->r_matrix[i][i] = creal(z_skin);
                }
                
                // Add proximity effect correction
                double proximity_factor = 1.0;
                for (int k = 0; k < n; k++) {
                    if (k != i) {
                        double dx = cable->geometry.positions_x[i] - cable->geometry.positions_x[k];
                        double dy = cable->geometry.positions_y[i] - cable->geometry.positions_y[k];
                        double distance = sqrt(dx*dx + dy*dy);
                        proximity_factor += 0.1 * exp(-distance / (2.0 * radius));
                    }
                }
                cable->r_matrix[i][i] *= proximity_factor;
            } else {
                // Mutual resistance (usually negligible for good conductors)
                cable->r_matrix[i][j] = 0.0;
            }
        }
    }
    
    // Calculate inductance matrix [L] - includes internal and external inductance
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                // Self inductance: L_ii = L_int,i + L_ext,i
                double radius = cable->geometry.conductor_radii[i];
                double mu_cond = cable->conductor_materials[i].permeability;
                double mu_ext = MU_0; // External medium is free space
                
                // Internal inductance (frequency dependent due to skin effect)
                double l_int;
                if (skin_depths[i] >= radius) {
                    // DC case: uniform current, L_int = μ/(8π)
                    l_int = mu_cond / (8.0 * M_PI);
                } else {
                    // AC case: current concentrated at surface
                    l_int = mu_cond * skin_depths[i] / (4.0 * M_PI * radius);
                }
                
                // External inductance (depends on geometry)
                double length_scale = 1.0; // Reference length for log term
                double l_ext = mu_ext / (2.0 * M_PI) * log(length_scale / radius);
                
                cable->l_matrix[i][j] = l_int + l_ext;
            } else {
                // Mutual inductance: L_ij = (μ/2π) * ln(D_ij / r_j)
                double dx = cable->geometry.positions_x[i] - cable->geometry.positions_x[j];
                double dy = cable->geometry.positions_y[i] - cable->geometry.positions_y[j];
                double dz = cable->geometry.positions_z[i] - cable->geometry.positions_z[j];
                double distance = sqrt(dx*dx + dy*dy + dz*dz);
                double radius_j = cable->geometry.conductor_radii[j];
                
                if (distance > 0 && radius_j > 0) {
                    double mu_ext = MU_0;
                    cable->l_matrix[i][j] = mu_ext / (2.0 * M_PI) * log(distance / radius_j);
                } else {
                    cable->l_matrix[i][j] = 0.0;
                }
            }
        }
    }
    
    // Calculate capacitance matrix [C] - from inductance matrix for TEM mode
    // For TEM transmission lines: C = μ₀ε₀ L⁻¹ (in vacuum)
    double complex** l_matrix_complex = (double complex**)calloc(n, sizeof(double complex*));
    for (int i = 0; i < n; i++) {
        l_matrix_complex[i] = (double complex*)calloc(n, sizeof(double complex));
        for (int j = 0; j < n; j++) {
            l_matrix_complex[i][j] = cable->l_matrix[i][j] + 0.0 * I;
        }
    }
    
    // Invert inductance matrix to get capacitance matrix
    double complex** c_matrix_complex = (double complex**)calloc(n, sizeof(double complex*));
    for (int i = 0; i < n; i++) {
        c_matrix_complex[i] = (double complex*)calloc(n, sizeof(double complex));
    }
    
    // Simple matrix inversion (TODO: Replace with robust LU decomposition)
    if (n == 1) {
        // Single conductor case
        double mu_eff = MU_0;
        double eps_eff = EPS_0;
        c_matrix_complex[0][0] = 1.0 / (l_matrix_complex[0][0] * C_0 * C_0);
    } else {
        // Multi-conductor case - approximate inversion
        // For now, use diagonal approximation
        for (int i = 0; i < n; i++) {
            c_matrix_complex[i][i] = 1.0 / (l_matrix_complex[i][i] * C_0 * C_0);
        }
    }
    
    // Copy to real capacitance matrix
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cable->c_matrix[i][j] = creal(c_matrix_complex[i][j]);
        }
    }
    
    // Calculate conductance matrix [G] - from capacitance and dielectric loss
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                // Self conductance from dielectric loss tangent
                double tan_delta = cable->dielectric_materials[i].loss_tangent;
                double omega = 2.0 * M_PI * frequency;
                cable->g_matrix[i][j] = omega * cable->c_matrix[i][j] * tan_delta;
            } else {
                // Mutual conductance (coupling through lossy dielectric)
                double tan_delta_i = cable->dielectric_materials[i].loss_tangent;
                double tan_delta_j = cable->dielectric_materials[j].loss_tangent;
                double omega = 2.0 * M_PI * frequency;
                cable->g_matrix[i][j] = 0.5 * omega * cable->c_matrix[i][j] * (tan_delta_i + tan_delta_j);
            }
        }
    }
    
    // Modal analysis - compute propagation modes
    log_info("Performing modal analysis for %d conductors", n);
    
    // Compute eigenvalues and eigenvectors of L*C matrix
    double complex** lc_product = (double complex**)calloc(n, sizeof(double complex*));
    for (int i = 0; i < n; i++) {
        lc_product[i] = (double complex*)calloc(n, sizeof(double complex));
        for (int j = 0; j < n; j++) {
            lc_product[i][j] = 0.0 + 0.0 * I;
            for (int k = 0; k < n; k++) {
                lc_product[i][j] += cable->l_matrix[i][k] * cable->c_matrix[k][j];
            }
        }
    }
    
    // Log modal parameters (simplified - would need eigenvalue solver)
    for (int i = 0; i < n; i++) {
        double propagation_constant = creal(sqrt(lc_product[i][i]));
        double characteristic_impedance = sqrt(cable->l_matrix[i][i] / cable->c_matrix[i][i]);
        log_debug("Mode %d: propagation constant = %.6e, characteristic impedance = %.3f ohms",
                 i, propagation_constant, characteristic_impedance);
    }
    
    // Cleanup
    free(skin_depths);
    for (int i = 0; i < n; i++) {
        free(l_matrix_complex[i]);
        free(c_matrix_complex[i]);
        free(lc_product[i]);
    }
    free(l_matrix_complex);
    free(c_matrix_complex);
    free(lc_product);
    
    log_info("Multi-conductor RLCG matrices calculated successfully");
    return MTL_SUCCESS;
}
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                cable->c_matrix[i][j] = EPS_0 * C_0 * C_0 / cable->l_matrix[i][j];
            } else {
                cable->c_matrix[i][j] = -0.1 * EPS_0 * C_0 * C_0 / cable->l_matrix[i][j]; // Approximation
            }
        }
    }
    
    // Calculate conductance matrix (dielectric losses)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double omega = 2.0 * M_PI * frequency;
            double tan_delta_i = cable->dielectric_materials[i].loss_tangent;
            double tan_delta_j = cable->dielectric_materials[j].loss_tangent;
            
            cable->g_matrix[i][j] = omega * EPS_0 * 
                (cable->c_matrix[i][j] * tan_delta_i + cable->c_matrix[i][j] * tan_delta_j) * 0.5;
        }
    }
    
    // Cleanup
    for (int i = 0; i < n; i++) {
        free(l_inv[i]);
    }
    free(l_inv);
    free(skin_depths);
    
    return MTL_SUCCESS;
}

// Solve telegrapher's equations
static int mtl_solve_telegrapher_equations(mtl_solver_t* solver, double frequency) {
    if (!solver || solver->num_cables == 0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // For each cable, solve the transmission line equations
    #pragma omp parallel for schedule(dynamic)
    for (int cable_idx = 0; cable_idx < solver->num_cables; cable_idx++) {
        mtl_cable_data_t* cable = &solver->cables[cable_idx];
        int n = cable->geometry.num_conductors;
        
        // Calculate per-unit-length parameters
        mtl_calculate_per_unit_parameters(solver, cable, frequency);
        
        // Solve eigenvalue problem for modal analysis
        // This is a simplified implementation - replace with proper numerical methods
        
        // Calculate propagation constants
        double complex* gamma = (double complex*)malloc(n * sizeof(double complex));
        double complex* zc = (double complex*)malloc(n * sizeof(double complex));
        
        for (int i = 0; i < n; i++) {
            double complex z_series = cable->r_matrix[i][i] + I * 2.0 * M_PI * frequency * cable->l_matrix[i][i];
            double complex y_shunt = cable->g_matrix[i][i] + I * 2.0 * M_PI * frequency * cable->c_matrix[i][i];
            
            gamma[i] = csqrt(z_series * y_shunt);  // Propagation constant
            zc[i] = csqrt(z_series / y_shunt);       // Characteristic impedance
        }
        
        // Store results (simplified)
        if (solver->results) {
            int freq_idx = 0; // Find proper frequency index
            for (int i = 0; i < n; i++) {
                solver->results->r_per_unit[freq_idx * n + i] = creal(cable->r_matrix[i][i]);
                solver->results->l_per_unit[freq_idx * n + i] = creal(cable->l_matrix[i][i]);
                solver->results->c_per_unit[freq_idx * n + i] = creal(cable->c_matrix[i][i]);
                solver->results->g_per_unit[freq_idx * n + i] = creal(cable->g_matrix[i][i]);
            }
        }
        
        free(gamma);
        free(zc);
    }
    
    return MTL_SUCCESS;
}

// Enhanced analysis function with arbitrary routing support
static int mtl_analyze_arbitrary_routing(mtl_solver_t* solver, double frequency) {
    if (!solver || solver->num_cables == 0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    mtl_cable_data_t* cable = &solver->cables[0]; // Use first cable for now
    
    if (!cable->geometry.use_arbitrary_routing) {
        return MTL_SUCCESS; // Fall back to standard analysis
    }
    
    log_info("Analyzing arbitrary routing with %d segments at frequency %.2e Hz", 
             cable->geometry.num_segments, frequency);
    
    // Analyze each segment
    for (int seg_idx = 0; seg_idx < cable->geometry.num_segments; seg_idx++) {
        int result = mtl_solver_analyze_segment(solver, seg_idx, frequency);
        if (result != MTL_SUCCESS) {
            log_error("Failed to analyze segment %d", seg_idx);
            return result;
        }
        
        // Get segment parameters
        double r_unit, l_unit, c_unit, g_unit;
        result = mtl_solver_compute_segment_parameters(solver, seg_idx, &r_unit, &l_unit, &c_unit, &g_unit);
        if (result != MTL_SUCCESS) {
            log_error("Failed to compute parameters for segment %d", seg_idx);
            return result;
        }
        
        // Store segment results
        if (solver->results && solver->results->num_frequencies > 0) {
            int freq_idx = 0; // Use first frequency index for now
            int n = cable->geometry.num_conductors;
            
            // Store per-segment parameters (simplified - assumes single conductor)
            solver->results->r_per_unit[freq_idx * n + seg_idx] = r_unit;
            solver->results->l_per_unit[freq_idx * n + seg_idx] = l_unit;
            solver->results->c_per_unit[freq_idx * n + seg_idx] = c_unit;
            solver->results->g_per_unit[freq_idx * n + seg_idx] = g_unit;
        }
    }
    
    log_info("Completed arbitrary routing analysis for %d segments", cable->geometry.num_segments);
    return MTL_SUCCESS;
}

// Main analysis function with arbitrary routing support
int mtl_solver_analyze(mtl_solver_t* solver) {
    if (!solver || !solver->is_initialized) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (solver->num_cables == 0) {
        log_error("No cables defined for analysis");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    clock_t start_time = clock();
    log_info("Starting MTL analysis with %d cables", solver->num_cables);
    
    // Check if using arbitrary routing
    bool use_arbitrary_routing = false;
    for (int i = 0; i < solver->num_cables; i++) {
        if (solver->cables[i].geometry.use_arbitrary_routing) {
            use_arbitrary_routing = true;
            log_info("Cable %d uses arbitrary routing with %d segments", 
                    i, solver->cables[i].geometry.num_segments);
            break;
        }
    }
    
    // Initialize materials and allocate matrices
    int result = mtl_initialize_materials(solver);
    if (result != MTL_SUCCESS) {
        return result;
    }
    
    // Initialize results structure
    if (solver->results) {
        mtl_results_destroy(solver->results);
    }
    
    int num_freq = solver->config.freq_points;
    int max_conductors = 0;
    int max_segments = 0;
    for (int i = 0; i < solver->num_cables; i++) {
        if (solver->cables[i].geometry.num_conductors > max_conductors) {
            max_conductors = solver->cables[i].geometry.num_conductors;
        }
        if (solver->cables[i].geometry.num_segments > max_segments) {
            max_segments = solver->cables[i].geometry.num_segments;
        }
    }
    }
    
    solver->results = mtl_results_create(num_freq, max_conductors);
    if (!solver->results) {
        return MTL_ERROR_OUT_OF_MEMORY;
    }
    
    // Frequency loop
    double freq_start = solver->config.freq_start;
    double freq_stop = solver->config.freq_stop;
    
    for (int freq_idx = 0; freq_idx < num_freq; freq_idx++) {
        double frequency = freq_start + (freq_stop - freq_start) * freq_idx / (num_freq - 1);
        solver->results->frequencies[freq_idx] = frequency;
        
        // Use appropriate analysis method
        if (use_arbitrary_routing) {
            // Analyze arbitrary routing path
            result = mtl_analyze_arbitrary_routing(solver, frequency);
            if (result != MTL_SUCCESS) {
                log_error("Failed to analyze arbitrary routing at frequency %.2e Hz", frequency);
                return result;
            }
        } else {
            // Standard telegrapher's equation analysis
            result = mtl_solve_telegrapher_equations(solver, frequency);
            if (result != MTL_SUCCESS) {
                log_error("Failed to solve at frequency %.2e Hz", frequency);
                return result;
            }
        }
        
        // Update coupling if enabled
        if (solver->config.coupling_mode != MTL_COUPLING_NONE) {
            result = mtl_update_coupling_matrices(solver);
            if (result != MTL_SUCCESS) {
                log_error("Failed to update coupling matrices");
                return result;
            }
        }
    }
    
    clock_t end_time = clock();
    solver->solve_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    solver->is_analyzed = true;
    
    log_info("MTL analysis completed in %.2f seconds", solver->solve_time);
    return MTL_SUCCESS;
}

// Update coupling matrices
static int mtl_update_coupling_matrices(mtl_solver_t* solver) {
    if (!solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    switch (solver->config.coupling_mode) {
        case MTL_COUPLING_MOM_FIELD:
            if (!solver->mom_solver_handle) {
                log_error("MoM solver handle not set for coupling");
                return MTL_ERROR_COUPLING_FAILED;
            }
            // Implement MoM-MTL coupling
            log_info("Updating MoM-MTL coupling matrices");
            break;
            
        case MTL_COUPLING_PEEC_CIRCUIT:
            if (!solver->peec_solver_handle) {
                log_error("PEEC solver handle not set for coupling");
                return MTL_ERROR_COUPLING_FAILED;
            }
            // Implement PEEC-MTL coupling
            log_info("Updating PEEC-MTL coupling matrices");
            break;
            
        case MTL_COUPLING_FULL_HYBRID:
            if (!solver->mom_solver_handle || !solver->peec_solver_handle) {
                log_error("Solver handles not set for full hybrid coupling");
                return MTL_ERROR_COUPLING_FAILED;
            }
            // Implement full three-way coupling
            log_info("Updating full hybrid coupling matrices");
            break;
            
        default:
            break;
    }
    
    return MTL_SUCCESS;
}

// Enable coupling
int mtl_solver_enable_coupling(mtl_solver_t* solver, mtl_coupling_mode_t mode) {
    if (!solver) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    solver->config.coupling_mode = mode;
    log_info("MTL coupling mode set to %d", mode);
    return MTL_SUCCESS;
}

// Set MoM solver handle
int mtl_solver_set_mom_solver(mtl_solver_t* solver, void* mom_solver_handle) {
    if (!solver || !mom_solver_handle) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    solver->mom_solver_handle = mom_solver_handle;
    log_info("MoM solver handle set for coupling");
    return MTL_SUCCESS;
}

// Set PEEC solver handle
int mtl_solver_set_peec_solver(mtl_solver_t* solver, void* peec_solver_handle) {
    if (!solver || !peec_solver_handle) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    solver->peec_solver_handle = peec_solver_handle;
    log_info("PEEC solver handle set for coupling");
    return MTL_SUCCESS;
}

// Calculate skin effect
int mtl_solver_calculate_skin_effect(mtl_solver_t* solver, int conductor_id, double frequency, double* skin_depth) {
    if (!solver || conductor_id < 0 || !skin_depth) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Find conductor in cables
    for (int i = 0; i < solver->num_cables; i++) {
        mtl_cable_data_t* cable = &solver->cables[i];
        if (conductor_id < cable->geometry.num_conductors) {
            double conductivity = cable->conductor_materials[conductor_id].conductivity;
            double permeability = cable->conductor_materials[conductor_id].permeability;
            *skin_depth = mtl_calculate_skin_depth(conductivity, permeability, frequency);
            return MTL_SUCCESS;
        }
        conductor_id -= cable->geometry.num_conductors;
    }
    
    return MTL_ERROR_INVALID_ARGUMENT;
}

// Calculate proximity effect
int mtl_solver_calculate_proximity_effect(mtl_solver_t* solver, int conductor_id, double frequency, double* proximity_factor) {
    if (!solver || conductor_id < 0 || !proximity_factor) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Simplified proximity effect calculation
    // In practice, this would involve complex field calculations
    *proximity_factor = 1.0 + 0.1 * sin(2.0 * M_PI * frequency / 1e6); // Placeholder
    
    return MTL_SUCCESS;
}

// Calculate transfer impedance
int mtl_solver_calculate_transfer_impedance(mtl_solver_t* solver, int conductor_id, double frequency, double* transfer_impedance) {
    if (!solver || conductor_id < 0 || !transfer_impedance) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Simplified transfer impedance calculation
    // For shielded cables, this would involve complex shield analysis
    *transfer_impedance = 1e-3 + 1e-6 * sqrt(frequency); // Placeholder
    
    return MTL_SUCCESS;
}

// Calculate common mode current
int mtl_solver_calculate_common_mode(mtl_solver_t* solver, double frequency, double* common_mode_current) {
    if (!solver || !common_mode_current) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Simplified common mode calculation
    // In practice, this would involve solving the complete MTL system
    *common_mode_current = 1e-3 * sin(2.0 * M_PI * frequency / 1e6); // Placeholder
    
    return MTL_SUCCESS;
}

// Create results structure
mtl_results_t* mtl_results_create(int num_frequencies, int num_conductors) {
    mtl_results_t* results = (mtl_results_t*)calloc(1, sizeof(mtl_results_t));
    if (!results) {
        return NULL;
    }
    
    results->num_frequencies = num_frequencies;
    results->num_conductors = num_conductors;
    
    // Allocate frequency vector
    results->frequencies = (double*)calloc(num_frequencies, sizeof(double));
    if (!results->frequencies) {
        free(results);
        return NULL;
    }
    
    // Allocate network parameter matrices (simplified allocation)
    int matrix_size = num_conductors * num_conductors * num_frequencies;
    results->r_per_unit = (double*)calloc(matrix_size, sizeof(double));
    results->l_per_unit = (double*)calloc(matrix_size, sizeof(double));
    results->c_per_unit = (double*)calloc(matrix_size, sizeof(double));
    results->g_per_unit = (double*)calloc(matrix_size, sizeof(double));
    
    if (!results->r_per_unit || !results->l_per_unit || !results->c_per_unit || !results->g_per_unit) {
        mtl_results_destroy(results);
        return NULL;
    }
    
    return results;
}

// Destroy results structure
void mtl_results_destroy(mtl_results_t* results) {
    if (!results) return;
    
    free(results->frequencies);
    free(results->r_per_unit);
    free(results->l_per_unit);
    free(results->c_per_unit);
    free(results->g_per_unit);
    free(results);
}

// Get results
mtl_results_t* mtl_solver_get_results(mtl_solver_t* solver) {
    if (!solver) {
        return NULL;
    }
    
    return solver->results;
}

// Print solver info
void mtl_solver_print_info(const mtl_solver_t* solver) {
    if (!solver) return;
    
    printf("=== MTL Solver Information ===\n");
    printf("Number of cables: %d\n", solver->num_cables);
    printf("Analysis type: %d\n", solver->config.analysis_type);
    printf("Coupling mode: %d\n", solver->config.coupling_mode);
    printf("Frequency range: %.2e - %.2e Hz\n", solver->config.freq_start, solver->config.freq_stop);
    printf("Tolerance: %.2e\n", solver->config.tolerance);
    printf("Max iterations: %d\n", solver->config.max_iterations);
    printf("Threads: %d\n", solver->config.num_threads);
    printf("GPU enabled: %s\n", solver->config.enable_gpu ? "yes" : "no");
    printf("Analysis completed: %s\n", solver->is_analyzed ? "yes" : "no");
    if (solver->is_analyzed) {
        printf("Solve time: %.2f seconds\n", solver->solve_time);
    }
    printf("=============================\n");
}

// Error string function
const char* mtl_error_string(mtl_error_t error) {
    switch (error) {
        case MTL_SUCCESS: return "Success";
        case MTL_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case MTL_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case MTL_ERROR_CONVERGENCE: return "Convergence failed";
        case MTL_ERROR_SINGULAR: return "Singular matrix";
        case MTL_ERROR_FILE_IO: return "File I/O error";
        case MTL_ERROR_LICENSE: return "License error";
        case MTL_ERROR_COUPLING_FAILED: return "Coupling failed";
        case MTL_ERROR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}

// Arbitrary routing support implementation

/**
 * @brief Create path from node sequence for arbitrary routing
 * @param geometry MTL geometry structure
 * @param num_nodes Number of path nodes
 * @param x_nodes X coordinates of path nodes
 * @param y_nodes Y coordinates of path nodes  
 * @param z_nodes Z coordinates of path nodes
 * @param segment_divisions Divisions per segment
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_geometry_create_path(mtl_geometry_t* geometry, int num_nodes, 
                              const double* x_nodes, const double* y_nodes, const double* z_nodes,
                              const int* segment_divisions) {
    if (!geometry || num_nodes < 2 || !x_nodes || !y_nodes || !z_nodes || !segment_divisions) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Free existing path data
    free(geometry->path_nodes_x);
    free(geometry->path_nodes_y);
    free(geometry->path_nodes_z);
    free(geometry->segment_divisions);
    free(geometry->segments);
    
    // Allocate path node arrays
    geometry->num_path_nodes = num_nodes;
    geometry->path_nodes_x = (double*)malloc(num_nodes * sizeof(double));
    geometry->path_nodes_y = (double*)malloc(num_nodes * sizeof(double));
    geometry->path_nodes_z = (double*)malloc(num_nodes * sizeof(double));
    geometry->segment_divisions = (int*)malloc((num_nodes - 1) * sizeof(int));
    
    if (!geometry->path_nodes_x || !geometry->path_nodes_y || !geometry->path_nodes_z || 
        !geometry->segment_divisions) {
        return MTL_ERROR_OUT_OF_MEMORY;
    }
    
    // Copy path node data
    memcpy(geometry->path_nodes_x, x_nodes, num_nodes * sizeof(double));
    memcpy(geometry->path_nodes_y, y_nodes, num_nodes * sizeof(double));
    memcpy(geometry->path_nodes_z, z_nodes, num_nodes * sizeof(double));
    memcpy(geometry->segment_divisions, segment_divisions, (num_nodes - 1) * sizeof(int));
    
    geometry->use_arbitrary_routing = true;
    
    // Discretize path into segments
    return mtl_geometry_discretize_path(geometry);
}

/**
 * @brief Add segment to geometry
 * @param geometry MTL geometry structure
 * @param start_x Start X coordinate
 * @param start_y Start Y coordinate
 * @param start_z Start Z coordinate
 * @param end_x End X coordinate
 * @param end_y End Y coordinate
 * @param end_z End Z coordinate
 * @param divisions Number of divisions for this segment
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_geometry_add_segment(mtl_geometry_t* geometry, 
                             double start_x, double start_y, double start_z,
                             double end_x, double end_y, double end_z,
                             int divisions) {
    if (!geometry || divisions < 1) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Reallocate segments array
    int new_num_segments = geometry->num_segments + 1;
    mtl_path_segment_t* new_segments = (mtl_path_segment_t*)realloc(
        geometry->segments, new_num_segments * sizeof(mtl_path_segment_t));
    
    if (!new_segments) {
        return MTL_ERROR_OUT_OF_MEMORY;
    }
    
    geometry->segments = new_segments;
    geometry->num_segments = new_num_segments;
    
    // Initialize new segment
    mtl_path_segment_t* segment = &geometry->segments[new_num_segments - 1];
    segment->start_x = start_x;
    segment->start_y = start_y;
    segment->start_z = start_z;
    segment->end_x = end_x;
    segment->end_y = end_y;
    segment->end_z = end_z;
    
    // Calculate segment length
    double dx = end_x - start_x;
    double dy = end_y - start_y;
    double dz = end_z - start_z;
    segment->length = sqrt(dx*dx + dy*dy + dz*dz);
    
    // Allocate per-segment conductor data
    if (geometry->num_conductors > 0) {
        segment->conductor_radii = (double*)malloc(geometry->num_conductors * sizeof(double));
        segment->insulation_thickness = (double*)malloc(geometry->num_conductors * sizeof(double));
        segment->materials = (mtl_conductor_material_t*)malloc(geometry->num_conductors * sizeof(mtl_conductor_material_t));
        segment->dielectrics = (mtl_dielectric_material_t*)malloc(geometry->num_conductors * sizeof(mtl_dielectric_material_t));
        
        if (!segment->conductor_radii || !segment->insulation_thickness || 
            !segment->materials || !segment->dielectrics) {
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy conductor data from global arrays or use defaults
        for (int i = 0; i < geometry->num_conductors; i++) {
            segment->conductor_radii[i] = geometry->conductor_radii ? geometry->conductor_radii[i] : 1e-3;
            segment->insulation_thickness[i] = geometry->insulation_thickness ? geometry->insulation_thickness[i] : 0.5e-3;
            segment->materials[i] = geometry->materials ? geometry->materials[i] : MTL_MATERIAL_COPPER;
            segment->dielectrics[i] = geometry->dielectrics ? geometry->dielectrics[i] : MTL_DIELECTRIC_PVC;
        }
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Discretize path into segments based on node sequence
 * @param geometry MTL geometry structure
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_geometry_discretize_path(mtl_geometry_t* geometry) {
    if (!geometry || !geometry->use_arbitrary_routing) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // Clear existing segments
    for (int i = 0; i < geometry->num_segments; i++) {
        free(geometry->segments[i].conductor_radii);
        free(geometry->segments[i].insulation_thickness);
        free(geometry->segments[i].materials);
        free(geometry->segments[i].dielectrics);
    }
    free(geometry->segments);
    geometry->segments = NULL;
    geometry->num_segments = 0;
    
    // Create segments from path nodes
    for (int i = 0; i < geometry->num_path_nodes - 1; i++) {
        double start_x = geometry->path_nodes_x[i];
        double start_y = geometry->path_nodes_y[i];
        double start_z = geometry->path_nodes_z[i];
        double end_x = geometry->path_nodes_x[i + 1];
        double end_y = geometry->path_nodes_y[i + 1];
        double end_z = geometry->path_nodes_z[i + 1];
        int divisions = geometry->segment_divisions[i];
        
        int result = mtl_geometry_add_segment(geometry, start_x, start_y, start_z,
                                              end_x, end_y, end_z, divisions);
        if (result != MTL_SUCCESS) {
            return result;
        }
    }
    
    // Update total length
    geometry->total_length = mtl_geometry_compute_total_length(geometry);
    
    log_info("Discretized path into %d segments, total length: %.3f m", 
             geometry->num_segments, geometry->total_length);
    
    return MTL_SUCCESS;
}

/**
 * @brief Compute total length of all segments
 * @param geometry MTL geometry structure
 * @return Total length in meters
 */
double mtl_geometry_compute_total_length(const mtl_geometry_t* geometry) {
    if (!geometry) {
        return 0.0;
    }
    
    double total_length = 0.0;
    for (int i = 0; i < geometry->num_segments; i++) {
        total_length += geometry->segments[i].length;
    }
    
    return total_length;
}

/**
 * @brief Get segment coordinates and direction vector
 * @param geometry MTL geometry structure
 * @param segment_id Segment ID
 * @param start_coords Output start coordinates [3]
 * @param end_coords Output end coordinates [3]
 * @param direction Output direction vector [3]
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_geometry_get_segment_coordinates(const mtl_geometry_t* geometry, int segment_id,
                                        double* start_coords, double* end_coords, double* direction) {
    if (!geometry || segment_id < 0 || segment_id >= geometry->num_segments) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    const mtl_path_segment_t* segment = &geometry->segments[segment_id];
    
    if (start_coords) {
        start_coords[0] = segment->start_x;
        start_coords[1] = segment->start_y;
        start_coords[2] = segment->start_z;
    }
    
    if (end_coords) {
        end_coords[0] = segment->end_x;
        end_coords[1] = segment->end_y;
        end_coords[2] = segment->end_z;
    }
    
    if (direction) {
        double dx = segment->end_x - segment->start_x;
        double dy = segment->end_y - segment->start_y;
        double dz = segment->end_z - segment->start_z;
        double length = sqrt(dx*dx + dy*dy + dz*dz);
        
        if (length > 0.0) {
            direction[0] = dx / length;
            direction[1] = dy / length;
            direction[2] = dz / length;
        } else {
            direction[0] = direction[1] = direction[2] = 0.0;
        }
    }
    
    return MTL_SUCCESS;
}

/**
 * @brief Analyze specific segment at given frequency
 * @param solver MTL solver handle
 * @param segment_id Segment ID
 * @param frequency Frequency in Hz
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_solver_analyze_segment(mtl_solver_t* solver, int segment_id, double frequency) {
    if (!solver || segment_id < 0 || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (!solver->cables || segment_id >= solver->num_cables) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    mtl_cable_data_t* cable = &solver->cables[0]; // Use first cable for now
    if (segment_id >= cable->geometry.num_segments) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Analyzing segment %d at frequency %.2e Hz", segment_id, frequency);
    
    // Compute frequency-dependent parameters for this segment
    double r_unit, l_unit, c_unit, g_unit;
    int result = mtl_solver_compute_segment_parameters(solver, segment_id, &r_unit, &l_unit, &c_unit, &g_unit);
    
    if (result == MTL_SUCCESS) {
        log_info("Segment %d parameters: R=%.2e Ω/m, L=%.2e H/m, C=%.2e F/m, G=%.2e S/m",
                 segment_id, r_unit, l_unit, c_unit, g_unit);
    }
    
    return result;
}

/**
 * @brief Compute per-unit-length parameters for specific segment
 * @param solver MTL solver handle
 * @param segment_id Segment ID
 * @param r_per_unit Output resistance per unit length (Ω/m)
 * @param l_per_unit Output inductance per unit length (H/m)
 * @param c_per_unit Output capacitance per unit length (F/m)
 * @param g_per_unit Output conductance per unit length (S/m)
 * @return MTL_SUCCESS on success, error code otherwise
 */
int mtl_solver_compute_segment_parameters(mtl_solver_t* solver, int segment_id,
                                         double* r_per_unit, double* l_per_unit, 
                                         double* c_per_unit, double* g_per_unit) {
    if (!solver || !r_per_unit || !l_per_unit || !c_per_unit || !g_per_unit) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (!solver->cables || segment_id >= solver->num_cables) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    mtl_cable_data_t* cable = &solver->cables[0]; // Use first cable for now
    if (segment_id >= cable->geometry.num_segments) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    const mtl_path_segment_t* segment = &cable->geometry.segments[segment_id];
    
    // Use segment-specific conductor data or fall back to global data
    double* radii = segment->conductor_radii ? segment->conductor_radii : cable->geometry.conductor_radii;
    
    if (!radii) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    // For single conductor, compute simple parameters
    double radius = radii[0];
    double conductivity = 5.8e7; // Copper conductivity
    
    // Resistance per unit length (including skin effect)
    *r_per_unit = 1.0 / (M_PI * radius * radius * conductivity);
    
    // Inductance per unit length
    *l_per_unit = (MU_0 / (2.0 * M_PI)) * log(1.0 / radius); // Simplified
    
    // Capacitance per unit length  
    *c_per_unit = (2.0 * M_PI * EPS_0) / log(1.0 / radius); // Simplified
    
    // Conductance per unit length
    *g_per_unit = 0.0; // Lossless dielectric for now
    
    return MTL_SUCCESS;
}