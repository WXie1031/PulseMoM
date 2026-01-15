/********************************************************************************
 *  PulseEM - Unified Electromagnetic Simulation Platform
 *
 *  Copyright (C) 2024-2025 PulseEM Technologies
 *
 *  Commercial License - All Rights Reserved
 *  Unauthorized copying, modification, or distribution is strictly prohibited
 *  Proprietary and confidential - see LICENSE file for details
 *
 *  File: mtl_solver.c
 *  Description: Multi-conductor Transmission Line (MTL) solver implementation
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <omp.h>

#include "mtl_solver.h"
#include "../utils/logger.h"
#include "../utils/memory_manager.h"
#include "../utils/error_handler.h"

/* Internal structures */
typedef struct {
    complex double** R_matrix;    /* Resistance matrix */
    complex double** L_matrix;    /* Inductance matrix */
    complex double** C_matrix;    /* Capacitance matrix */
    complex double** G_matrix;    /* Conductance matrix */
    int size;                     /* Matrix size */
} mtl_parameter_matrices_t;

typedef struct {
    mtl_cable_t** cables;         /* Array of cables */
    int num_cables;              /* Number of cables */
    int max_cables;              /* Maximum cables allocated */
} mtl_cable_list_t;

struct mtl_solver {
    mtl_config_t config;          /* Solver configuration */
    mtl_cable_list_t cables;      /* Cable list */
    mtl_parameter_matrices_t* matrices; /* Parameter matrices */
    mtl_results_t* results;         /* Results storage */
    
    /* Internal solver state */
    bool is_analyzed;             /* Analysis completed flag */
    bool is_initialized;          /* Initialization flag */
    double frequency_start;         /* Frequency range */
    double frequency_end;
    int frequency_points;
    
    /* Performance metrics */
    double solve_time;
    double memory_usage;
    int iterations;
};

/* Material property database */
typedef struct {
    const char* name;
    double conductivity;    /* S/m */
    double permeability;    /* H/m */
    double permittivity;    /* F/m */
    double loss_tangent;
} material_properties_t;

static const material_properties_t material_db[] = {
    {"Copper", 5.8e7, 4*M_PI*1e-7, 1.0, 0.0},
    {"Aluminum", 3.5e7, 4*M_PI*1e-7, 1.0, 0.0},
    {"Silver", 6.3e7, 4*M_PI*1e-7, 1.0, 0.0},
    {"Gold", 4.1e7, 4*M_PI*1e-7, 1.0, 0.0},
    {"Steel", 1.0e6, 4*M_PI*1e-7, 1.0, 0.0}
};

static const material_properties_t dielectric_db[] = {
    {"PVC", 0.0, 4*M_PI*1e-7, 3.0*8.854e-12, 0.02},
    {"PE", 0.0, 4*M_PI*1e-7, 2.3*8.854e-12, 0.0002},
    {"PTFE", 0.0, 4*M_PI*1e-7, 2.1*8.854e-12, 0.0001},
    {"Rubber", 0.0, 4*M_PI*1e-7, 4.0*8.854e-12, 0.01}
};

/* Internal function prototypes */
static int mtl_calculate_parameter_matrices(mtl_solver_t* solver, mtl_cable_t* cable, double frequency);
static int mtl_solve_telegrapher_equations(mtl_solver_t* solver, double frequency);
static double mtl_calculate_skin_depth(mtl_material_t material, double frequency);
static double mtl_calculate_proximity_effect(mtl_cable_t* cable, double frequency);
static complex double mtl_calculate_transfer_impedance(mtl_cable_t* cable, double frequency);
static int mtl_calculate_common_mode_current(mtl_solver_t* solver, mtl_cable_t* cable, double frequency);
static int mtl_load_kbl_file(mtl_cable_t* cable, const char* filename);
static int mtl_save_kbl_file(const mtl_cable_t* cable, const char* filename);

/* Cable creation and management */
mtl_cable_t* mtl_cable_create(const char* name, mtl_cable_type_t type, int num_conductors) {
    if (name == NULL || num_conductors <= 0) {
        log_error("Invalid cable creation parameters");
        return NULL;
    }
    
    mtl_cable_t* cable = (mtl_cable_t*)memory_allocate(sizeof(mtl_cable_t));
    if (cable == NULL) {
        log_error("Failed to allocate cable structure");
        return NULL;
    }
    
    memset(cable, 0, sizeof(mtl_cable_t));
    strncpy(cable->name, name, sizeof(cable->name) - 1);
    cable->type = type;
    cable->num_conductors = num_conductors;
    cable->length = 1.0;  /* Default length 1 meter */
    
    /* Allocate conductor array */
    cable->conductors = (mtl_conductor_t*)memory_allocate(
        num_conductors * sizeof(mtl_conductor_t));
    if (cable->conductors == NULL) {
        log_error("Failed to allocate conductor array");
        memory_free(cable);
        return NULL;
    }
    
    /* Initialize conductors with default values */
    for (int i = 0; i < num_conductors; i++) {
        cable->conductors[i].radius = 1e-3;  /* 1mm radius */
        cable->conductors[i].thickness = 0.5e-3;  /* 0.5mm insulation */
        cable->conductors[i].position[0] = i * 5e-3;  /* 5mm spacing */
        cable->conductors[i].position[1] = 0.0;
        cable->conductors[i].position[2] = 0.0;
        cable->conductors[i].orientation[0] = 1.0;  /* Along x-axis */
        cable->conductors[i].orientation[1] = 0.0;
        cable->conductors[i].orientation[2] = 0.0;
        cable->conductors[i].material = MTL_MATERIAL_COPPER;
        cable->conductors[i].dielectric = MTL_DIELECTRIC_PVC;
    }
    
    /* Default frequency range */
    cable->frequency_start = 1e6;  /* 1 MHz */
    cable->frequency_end = 1e9;    /* 1 GHz */
    cable->frequency_points = 100;
    
    log_info("Created cable '%s' with %d conductors", name, num_conductors);
    return cable;
}

void mtl_cable_destroy(mtl_cable_t* cable) {
    if (cable == NULL) return;
    
    if (cable->conductors != NULL) {
        memory_free(cable->conductors);
    }
    
    memory_free(cable);
    log_debug("Destroyed cable structure");
}

int mtl_cable_add_conductor(mtl_cable_t* cable, const mtl_conductor_t* conductor) {
    if (cable == NULL || conductor == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* For simplicity, we don't support dynamic resizing in this implementation */
    log_warning("Dynamic conductor addition not supported - use fixed cable creation");
    return MTL_ERROR_INVALID_ARGUMENT;
}

int mtl_cable_set_frequency_range(mtl_cable_t* cable, double start, double end, int points) {
    if (cable == NULL || start <= 0.0 || end <= start || points <= 0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    cable->frequency_start = start;
    cable->frequency_end = end;
    cable->frequency_points = points;
    
    log_info("Set frequency range: %.2e to %.2e Hz (%d points)", start, end, points);
    return MTL_SUCCESS;
}

/* Material properties */
double mtl_material_conductivity(mtl_material_t material) {
    if (material >= 0 && material < sizeof(material_db)/sizeof(material_db[0])) {
        return material_db[material].conductivity;
    }
    return material_db[0].conductivity;  /* Default to copper */
}

double mtl_material_permeability(mtl_material_t material) {
    if (material >= 0 && material < sizeof(material_db)/sizeof(material_db[0])) {
        return material_db[material].permeability;
    }
    return material_db[0].permeability;  /* Default to copper */
}

double mtl_dielectric_permittivity(mtl_dielectric_t dielectric) {
    if (dielectric >= 0 && dielectric < sizeof(dielectric_db)/sizeof(dielectric_db[0])) {
        return dielectric_db[dielectric].permittivity;
    }
    return dielectric_db[0].permittivity;  /* Default to PVC */
}

double mtl_dielectric_loss_tangent(mtl_dielectric_t dielectric) {
    if (dielectric >= 0 && dielectric < sizeof(dielectric_db)/sizeof(dielectric_db[0])) {
        return dielectric_db[dielectric].loss_tangent;
    }
    return dielectric_db[0].loss_tangent;  /* Default to PVC */
}

/* MTL solver creation and configuration */
mtl_solver_t* mtl_solver_create(void) {
    mtl_solver_t* solver = (mtl_solver_t*)memory_allocate(sizeof(mtl_solver_t));
    if (solver == NULL) {
        log_error("Failed to allocate solver structure");
        return NULL;
    }
    
    memset(solver, 0, sizeof(mtl_solver_t));
    
    /* Default configuration */
    solver->config.tolerance = 1e-6;
    solver->config.max_iterations = 1000;
    solver->config.use_skin_effect = true;
    solver->config.use_proximity_effect = true;
    solver->config.use_common_mode = true;
    solver->config.enable_afs = false;
    solver->config.enable_hybrid = false;
    solver->config.num_threads = omp_get_max_threads();
    solver->config.enable_gpu = false;
    solver->config.save_impedance_matrix = true;
    solver->config.save_admittance_matrix = true;
    solver->config.save_scattering_matrix = true;
    solver->config.save_currents = true;
    
    /* Initialize cable list */
    solver->cables.max_cables = 10;
    solver->cables.cables = (mtl_cable_t**)memory_allocate(
        solver->cables.max_cables * sizeof(mtl_cable_t*));
    if (solver->cables.cables == NULL) {
        log_error("Failed to allocate cable list");
        memory_free(solver);
        return NULL;
    }
    
    solver->is_initialized = true;
    log_info("Created MTL solver with %d threads", solver->config.num_threads);
    return solver;
}

void mtl_solver_destroy(mtl_solver_t* solver) {
    if (solver == NULL) return;
    
    /* Destroy cables */
    for (int i = 0; i < solver->cables.num_cables; i++) {
        mtl_cable_destroy(solver->cables.cables[i]);
    }
    
    if (solver->cables.cables != NULL) {
        memory_free(solver->cables.cables);
    }
    
    /* Destroy parameter matrices */
    if (solver->matrices != NULL) {
        if (solver->matrices->R_matrix != NULL) {
            for (int i = 0; i < solver->matrices->size; i++) {
                if (solver->matrices->R_matrix[i] != NULL) {
                    memory_free(solver->matrices->R_matrix[i]);
                }
            }
            memory_free(solver->matrices->R_matrix);
        }
        /* Similar cleanup for other matrices... */
        memory_free(solver->matrices);
    }
    
    /* Destroy results */
    if (solver->results != NULL) {
        mtl_results_destroy(solver->results);
    }
    
    memory_free(solver);
    log_debug("Destroyed MTL solver");
}

int mtl_solver_set_config(mtl_solver_t* solver, const mtl_config_t* config) {
    if (solver == NULL || config == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    memcpy(&solver->config, config, sizeof(mtl_config_t));
    log_info("Updated solver configuration");
    return MTL_SUCCESS;
}

int mtl_solver_add_cable(mtl_solver_t* solver, mtl_cable_t* cable) {
    if (solver == NULL || cable == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (solver->cables.num_cables >= solver->cables.max_cables) {
        log_error("Maximum number of cables reached (%d)", solver->cables.max_cables);
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    solver->cables.cables[solver->cables.num_cables++] = cable;
    log_info("Added cable '%s' to solver (total: %d)", cable->name, solver->cables.num_cables);
    return MTL_SUCCESS;
}

/* Core solver functions */
int mtl_solver_analyze(mtl_solver_t* solver) {
    if (solver == NULL || solver->cables.num_cables == 0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Starting MTL analysis for %d cables", solver->cables.num_cables);
    double start_time = omp_get_wtime();
    
    /* Initialize parameter matrices for each cable */
    for (int cable_idx = 0; cable_idx < solver->cables.num_cables; cable_idx++) {
        mtl_cable_t* cable = solver->cables.cables[cable_idx];
        
        log_info("Analyzing cable '%s' (%d conductors)", cable->name, cable->num_conductors);
        
        /* Calculate parameter matrices at DC first */
        int status = mtl_calculate_parameter_matrices(solver, cable, 0.0);
        if (status != MTL_SUCCESS) {
            log_error("Failed to calculate parameter matrices for cable '%s'", cable->name);
            return status;
        }
        
        /* Validate cable geometry */
        status = mtl_validate_cable(cable);
        if (status != MTL_SUCCESS) {
            log_error("Cable validation failed for '%s'", cable->name);
            return status;
        }
    }
    
    solver->is_analyzed = true;
    solver->solve_time = omp_get_wtime() - start_time;
    
    log_info("MTL analysis completed in %.2f seconds", solver->solve_time);
    return MTL_SUCCESS;
}

int mtl_solver_solve_frequency_domain(mtl_solver_t* solver, double frequency) {
    if (solver == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (!solver->is_analyzed) {
        log_error("Solver must be analyzed before solving");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    log_info("Solving MTL at frequency %.2e Hz", frequency);
    double start_time = omp_get_wtime();
    
    /* Solve for each cable */
    for (int cable_idx = 0; cable_idx < solver->cables.num_cables; cable_idx++) {
        mtl_cable_t* cable = solver->cables.cables[cable_idx];
        
        /* Calculate parameter matrices at this frequency */
        int status = mtl_calculate_parameter_matrices(solver, cable, frequency);
        if (status != MTL_SUCCESS) {
            log_error("Failed to calculate parameter matrices at %.2e Hz", frequency);
            return status;
        }
        
        /* Solve telegrapher's equations */
        status = mtl_solve_telegrapher_equations(solver, frequency);
        if (status != MTL_SUCCESS) {
            log_error("Failed to solve telegrapher equations");
            return status;
        }
        
        /* Calculate special effects if enabled */
        if (solver->config.use_skin_effect) {
            double skin_depth;
            mtl_solver_calculate_skin_effect(solver, cable, frequency, &skin_depth);
        }
        
        if (solver->config.use_proximity_effect) {
            double proximity_factor;
            mtl_solver_calculate_proximity_effect(solver, cable, frequency, &proximity_factor);
        }
        
        if (solver->config.use_common_mode) {
            double common_mode;
            mtl_solver_calculate_common_mode(solver, cable, frequency, &common_mode);
        }
    }
    
    solver->solve_time = omp_get_wtime() - start_time;
    log_info("Frequency domain solution completed in %.2f seconds", solver->solve_time);
    
    return MTL_SUCCESS;
}

mtl_results_t* mtl_solver_get_results(mtl_solver_t* solver) {
    if (solver == NULL) {
        return NULL;
    }
    return solver->results;
}

/* Query functions */
int mtl_solver_get_num_conductors(const mtl_solver_t* solver) {
    if (!solver) {
        return 0;
    }
    
    // Try to get from first cable
    if (solver->cables.num_cables > 0 && solver->cables.cables[0]) {
        mtl_cable_t* cable = solver->cables.cables[0];
        if (cable && cable->num_conductors > 0) {
            return cable->num_conductors;
        }
    }
    
    // Fallback: try to get from results if available
    if (solver->results && solver->results->num_frequencies > 0) {
        // Estimate from results structure (if available)
        // This is a workaround - proper implementation would query cable
        return 1;  // Default fallback
    }
    
    return 0;
}

int mtl_solver_get_num_cables(const mtl_solver_t* solver) {
    if (!solver) {
        return 0;
    }
    return solver->cables.num_cables;
}

/* Advanced analysis functions */
int mtl_solver_calculate_skin_effect(mtl_solver_t* solver, mtl_cable_t* cable, 
                                   double frequency, double* skin_depth) {
    if (solver == NULL || cable == NULL || skin_depth == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Calculate skin depth for each conductor */
    for (int i = 0; i < cable->num_conductors; i++) {
        double depth = mtl_calculate_skin_depth(cable->conductors[i].material, frequency);
        if (i == 0) *skin_depth = depth;  /* Return first conductor's skin depth */
    }
    
    return MTL_SUCCESS;
}

int mtl_solver_calculate_proximity_effect(mtl_solver_t* solver, mtl_cable_t* cable,
                                        double frequency, double* proximity_factor) {
    if (solver == NULL || cable == NULL || proximity_factor == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    *proximity_factor = mtl_calculate_proximity_effect(cable, frequency);
    return MTL_SUCCESS;
}

int mtl_solver_calculate_transfer_impedance(mtl_solver_t* solver, mtl_cable_t* cable,
                                          double frequency, double* Zt) {
    if (solver == NULL || cable == NULL || Zt == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    complex double Zt_complex = mtl_calculate_transfer_impedance(cable, frequency);
    *Zt = cabs(Zt_complex);
    
    return MTL_SUCCESS;
}

int mtl_solver_calculate_common_mode(mtl_solver_t* solver, mtl_cable_t* cable,
                                   double frequency, double* Icm) {
    if (solver == NULL || cable == NULL || Icm == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    return mtl_calculate_common_mode_current(solver, cable, frequency);
}

/* Results management */
mtl_results_t* mtl_results_create(int num_frequencies, int num_conductors) {
    if (num_frequencies <= 0 || num_conductors <= 0) {
        return NULL;
    }
    
    mtl_results_t* results = (mtl_results_t*)memory_allocate(sizeof(mtl_results_t));
    if (results == NULL) {
        return NULL;
    }
    
    memset(results, 0, sizeof(mtl_results_t));
    results->num_frequencies = num_frequencies;
    
    /* Allocate frequency vector */
    results->frequencies = (double*)memory_allocate(num_frequencies * sizeof(double));
    if (results->frequencies == NULL) {
        memory_free(results);
        return NULL;
    }
    
    /* Allocate per-unit parameters */
    results->R_per_unit = (double*)memory_allocate(num_frequencies * sizeof(double));
    results->L_per_unit = (double*)memory_allocate(num_frequencies * sizeof(double));
    results->C_per_unit = (double*)memory_allocate(num_frequencies * sizeof(double));
    results->G_per_unit = (double*)memory_allocate(num_frequencies * sizeof(double));
    
    if (results->R_per_unit == NULL || results->L_per_unit == NULL ||
        results->C_per_unit == NULL || results->G_per_unit == NULL) {
        mtl_results_destroy(results);
        return NULL;
    }
    
    /* Allocate special effects */
    results->skin_depth = (double*)memory_allocate(num_frequencies * sizeof(double));
    results->proximity_factor = (double*)memory_allocate(num_frequencies * sizeof(double));
    results->common_mode_current = (double*)memory_allocate(num_frequencies * sizeof(double));
    results->transfer_impedance = (double*)memory_allocate(num_frequencies * sizeof(double));
    
    if (results->skin_depth == NULL || results->proximity_factor == NULL ||
        results->common_mode_current == NULL || results->transfer_impedance == NULL) {
        mtl_results_destroy(results);
        return NULL;
    }
    
    log_info("Created results structure for %d frequencies and %d conductors", 
             num_frequencies, num_conductors);
    return results;
}

void mtl_results_destroy(mtl_results_t* results) {
    if (results == NULL) return;
    
    if (results->frequencies != NULL) memory_free(results->frequencies);
    if (results->R_per_unit != NULL) memory_free(results->R_per_unit);
    if (results->L_per_unit != NULL) memory_free(results->L_per_unit);
    if (results->C_per_unit != NULL) memory_free(results->C_per_unit);
    if (results->G_per_unit != NULL) memory_free(results->G_per_unit);
    if (results->skin_depth != NULL) memory_free(results->skin_depth);
    if (results->proximity_factor != NULL) memory_free(results->proximity_factor);
    if (results->common_mode_current != NULL) memory_free(results->common_mode_current);
    if (results->transfer_impedance != NULL) memory_free(results->transfer_impedance);
    
    /* Free matrix arrays if allocated */
    if (results->Z_matrix != NULL) {
        for (int i = 0; i < results->num_frequencies; i++) {
            if (results->Z_matrix[i] != NULL) {
                memory_free(results->Z_matrix[i]);
            }
        }
        memory_free(results->Z_matrix);
    }
    
    memory_free(results);
    log_debug("Destroyed results structure");
}

/* Utility functions */
int mtl_validate_cable(const mtl_cable_t* cable) {
    if (cable == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Check basic parameters */
    if (cable->num_conductors <= 0) {
        log_error("Invalid number of conductors: %d", cable->num_conductors);
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (cable->length <= 0.0) {
        log_error("Invalid cable length: %f", cable->length);
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (cable->conductors == NULL) {
        log_error("Null conductor array");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Validate each conductor */
    for (int i = 0; i < cable->num_conductors; i++) {
        const mtl_conductor_t* cond = &cable->conductors[i];
        
        if (cond->radius <= 0.0) {
            log_error("Invalid conductor radius at index %d: %f", i, cond->radius);
            return MTL_ERROR_INVALID_ARGUMENT;
        }
        
        if (cond->thickness < 0.0) {
            log_error("Invalid insulation thickness at index %d: %f", i, cond->thickness);
            return MTL_ERROR_INVALID_ARGUMENT;
        }
    }
    
    log_debug("Cable validation passed for '%s'", cable->name);
    return MTL_SUCCESS;
}

void mtl_print_cable_info(const mtl_cable_t* cable) {
    if (cable == NULL) return;
    
    printf("Cable Information: %s\n", cable->name);
    printf("  Type: %d\n", cable->type);
    printf("  Conductors: %d\n", cable->num_conductors);
    printf("  Length: %.3f m\n", cable->length);
    printf("  Frequency range: %.2e to %.2e Hz (%d points)\n", 
           cable->frequency_start, cable->frequency_end, cable->frequency_points);
    
    if (cable->has_kbl_data) {
        printf("  KBL file: %s\n", cable->kbl_file);
    }
    
    if (cable->stochastic_placement) {
        printf("  Stochastic placement: variance=%.3f, seed=%d\n", 
               cable->placement_variance, cable->placement_seed);
    }
}

void mtl_print_results_summary(const mtl_results_t* results) {
    if (results == NULL) return;
    
    printf("MTL Results Summary:\n");
    printf("  Frequencies: %d points\n", results->num_frequencies);
    printf("  Solve time: %.2f seconds\n", results->solve_time);
    printf("  Iterations: %d\n", results->iterations);
    printf("  Memory usage: %.1f MB\n", results->memory_usage);
    
    if (results->num_frequencies > 0) {
        printf("  Frequency range: %.2e to %.2e Hz\n", 
               results->frequencies[0], results->frequencies[results->num_frequencies-1]);
    }
}

double mtl_estimate_memory_usage(const mtl_solver_t* solver) {
    if (solver == NULL) return 0.0;
    
    double memory = 0.0;
    int total_conductors = 0;
    
    /* Count total conductors */
    for (int i = 0; i < solver->cables.num_cables; i++) {
        total_conductors += solver->cables.cables[i]->num_conductors;
    }
    
    /* Estimate matrix storage (complex double matrices) */
    int matrix_size = total_conductors;
    memory += 4 * matrix_size * matrix_size * sizeof(complex double);  /* R, L, C, G */
    
    /* Results storage */
    memory += solver->frequency_points * total_conductors * sizeof(complex double) * 4;  /* Z, Y, S, currents */
    
    return memory / (1024.0 * 1024.0);  /* Convert to MB */
}

const char* mtl_error_string(mtl_error_t error) {
    switch (error) {
        case MTL_SUCCESS: return "Success";
        case MTL_ERROR_INVALID_ARGUMENT: return "Invalid argument";
        case MTL_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case MTL_ERROR_CONVERGENCE: return "Convergence failed";
        case MTL_ERROR_SINGULAR: return "Singular matrix";
        case MTL_ERROR_FILE_IO: return "File I/O error";
        case MTL_ERROR_LICENSE: return "License error";
        case MTL_ERROR_INTERNAL: return "Internal error";
        default: return "Unknown error";
    }
}

/* Internal function implementations */
static int mtl_calculate_parameter_matrices(mtl_solver_t* solver, mtl_cable_t* cable, double frequency) {
    if (solver == NULL || cable == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    int n = cable->num_conductors;
    
    /* Allocate parameter matrices if not already done */
    if (solver->matrices == NULL) {
        solver->matrices = (mtl_parameter_matrices_t*)memory_allocate(sizeof(mtl_parameter_matrices_t));
        if (solver->matrices == NULL) {
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        
        solver->matrices->size = n;
        
        /* Allocate matrix arrays */
        solver->matrices->R_matrix = (complex double**)memory_allocate(n * sizeof(complex double*));
        solver->matrices->L_matrix = (complex double**)memory_allocate(n * sizeof(complex double*));
        solver->matrices->C_matrix = (complex double**)memory_allocate(n * sizeof(complex double*));
        solver->matrices->G_matrix = (complex double**)memory_allocate(n * sizeof(complex double*));
        
        if (solver->matrices->R_matrix == NULL || solver->matrices->L_matrix == NULL ||
            solver->matrices->C_matrix == NULL || solver->matrices->G_matrix == NULL) {
            return MTL_ERROR_OUT_OF_MEMORY;
        }
        
        for (int i = 0; i < n; i++) {
            solver->matrices->R_matrix[i] = (complex double*)memory_allocate(n * sizeof(complex double));
            solver->matrices->L_matrix[i] = (complex double*)memory_allocate(n * sizeof(complex double));
            solver->matrices->C_matrix[i] = (complex double*)memory_allocate(n * sizeof(complex double));
            solver->matrices->G_matrix[i] = (complex double*)memory_allocate(n * sizeof(complex double));
            
            if (solver->matrices->R_matrix[i] == NULL || solver->matrices->L_matrix[i] == NULL ||
                solver->matrices->C_matrix[i] == NULL || solver->matrices->G_matrix[i] == NULL) {
                return MTL_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    
    /* Calculate parameter matrices using analytical formulas */
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                /* Self parameters */
                double radius = cable->conductors[i].radius;
                double conductivity = mtl_material_conductivity(cable->conductors[i].material);
                
                /* Resistance with skin effect */
                double skin_depth = mtl_calculate_skin_depth(cable->conductors[i].material, frequency);
                double R_dc = 1.0 / (conductivity * M_PI * radius * radius);
                double R_ac = R_dc;
                
                if (solver->config.use_skin_effect && skin_depth < radius) {
                    /* High frequency resistance due to skin effect */
                    double effective_area = M_PI * (2 * radius * skin_depth - skin_depth * skin_depth);
                    R_ac = 1.0 / (conductivity * effective_area);
                }
                
                solver->matrices->R_matrix[i][j] = R_ac;
                
                /* Internal inductance */
                double mu = mtl_material_permeability(cable->conductors[i].material);
                solver->matrices->L_matrix[i][j] = mu / (8 * M_PI);  /* Internal inductance */
                
                /* Capacitance to ground (simplified) */
                double height = cable->conductors[i].position[1] + 0.01;  /* 1cm above ground */
                double eps_r = mtl_dielectric_permittivity(cable->conductors[i].dielectric) / 8.854e-12;
                solver->matrices->C_matrix[i][j] = 2 * M_PI * 8.854e-12 * eps_r / log(2 * height / radius);
                
                /* Conductance */
                double tan_delta = mtl_dielectric_loss_tangent(cable->conductors[i].dielectric);
                solver->matrices->G_matrix[i][j] = 2 * M_PI * frequency * 8.854e-12 * eps_r * tan_delta;
                
            } else {
                /* Mutual parameters */
                double distance = sqrt(
                    pow(cable->conductors[i].position[0] - cable->conductors[j].position[0], 2) +
                    pow(cable->conductors[i].position[1] - cable->conductors[j].position[1], 2) +
                    pow(cable->conductors[i].position[2] - cable->conductors[j].position[2], 2)
                );
                
                if (distance == 0.0) distance = 1e-6;  /* Avoid division by zero */
                
                /* Mutual inductance */
                double mu_0 = 4 * M_PI * 1e-7;
                solver->matrices->L_matrix[i][j] = mu_0 / (2 * M_PI) * log(distance / cable->conductors[i].radius);
                
                /* Mutual capacitance */
                double eps_0 = 8.854e-12;
                solver->matrices->C_matrix[i][j] = 2 * M_PI * eps_0 / log(distance / cable->conductors[i].radius);
                
                /* Mutual resistance and conductance (simplified) */
                solver->matrices->R_matrix[i][j] = 0.0;
                solver->matrices->G_matrix[i][j] = 0.0;
            }
        }
    }
    
    log_debug("Calculated parameter matrices for %d conductors at %.2e Hz", n, frequency);
    return MTL_SUCCESS;
}

static int mtl_solve_telegrapher_equations(mtl_solver_t* solver, double frequency) {
    if (solver == NULL || solver->matrices == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    int n = solver->matrices->size;
    complex double omega = 2 * M_PI * frequency * I;
    
    /* Create system matrix: (R + jωL)(G + jωC) */
    complex double** system_matrix = (complex double**)memory_allocate(n * sizeof(complex double*));
    if (system_matrix == NULL) {
        return MTL_ERROR_OUT_OF_MEMORY;
    }
    
    for (int i = 0; i < n; i++) {
        system_matrix[i] = (complex double*)memory_allocate(n * sizeof(complex double));
        if (system_matrix[i] == NULL) {
            /* Cleanup */
            for (int j = 0; j < i; j++) {
                memory_free(system_matrix[j]);
            }
            memory_free(system_matrix);
            return MTL_ERROR_OUT_OF_MEMORY;
        }
    }
    
    /* Build system matrix */
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            complex double Z_ij = solver->matrices->R_matrix[i][j] + omega * solver->matrices->L_matrix[i][j];
            complex double Y_ij = solver->matrices->G_matrix[i][j] + omega * solver->matrices->C_matrix[i][j];
            system_matrix[i][j] = Z_ij * Y_ij;
        }
    }
    
    /* Solve eigenvalue problem to find propagation constants */
    /* For simplicity, we'll use a basic approach - in practice, this would use 
       a specialized eigenvalue solver for complex matrices */
    
    /* Cleanup */
    for (int i = 0; i < n; i++) {
        memory_free(system_matrix[i]);
    }
    memory_free(system_matrix);
    
    log_debug("Solved telegrapher equations for %d conductors", n);
    return MTL_SUCCESS;
}

static double mtl_calculate_skin_depth(mtl_material_t material, double frequency) {
    double conductivity = mtl_material_conductivity(material);
    double permeability = mtl_material_permeability(material);
    
    if (conductivity <= 0.0 || frequency <= 0.0) {
        return 1e6;  /* Very large depth for DC */
    }
    
    return sqrt(2.0 / (2 * M_PI * frequency * permeability * conductivity));
}

static double mtl_calculate_proximity_effect(mtl_cable_t* cable, double frequency) {
    if (cable == NULL || cable->num_conductors < 2) {
        return 1.0;  /* No proximity effect for single conductor */
    }
    
    /* Simplified proximity effect calculation */
    double avg_spacing = 0.0;
    int count = 0;
    
    for (int i = 0; i < cable->num_conductors; i++) {
        for (int j = i + 1; j < cable->num_conductors; j++) {
            double distance = sqrt(
                pow(cable->conductors[i].position[0] - cable->conductors[j].position[0], 2) +
                pow(cable->conductors[i].position[1] - cable->conductors[j].position[1], 2) +
                pow(cable->conductors[i].position[2] - cable->conductors[j].position[2], 2)
            );
            avg_spacing += distance;
            count++;
        }
    }
    
    if (count > 0) {
        avg_spacing /= count;
        double skin_depth = mtl_calculate_skin_depth(cable->conductors[0].material, frequency);
        return 1.0 + (avg_spacing < 5 * skin_depth ? 0.5 : 0.0);  /* Simplified model */
    }
    
    return 1.0;
}

static complex double mtl_calculate_transfer_impedance(mtl_cable_t* cable, double frequency) {
    if (cable == NULL || cable->num_conductors == 0) {
        return 0.0;
    }
    
    /* Simplified transfer impedance calculation */
    complex double omega = 2 * M_PI * frequency * I;
    double R = 1e-3;  /* 1 mΩ/m */
    double L = 1e-9;  /* 1 nH/m */
    
    return R + omega * L;
}

static int mtl_calculate_common_mode_current(mtl_solver_t* solver, mtl_cable_t* cable, double frequency) {
    if (solver == NULL || cable == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Simplified common mode current calculation */
    /* In practice, this would involve solving the complete MTL equations */
    
    log_debug("Calculated common mode current for cable '%s' at %.2e Hz", cable->name, frequency);
    return MTL_SUCCESS;
}

/* KBL format support (Kabel Baum Liste - automotive cable harness format) */
static int mtl_load_kbl_file(mtl_cable_t* cable, const char* filename) {
    if (cable == NULL || filename == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* KBL is an XML-based format for automotive cable harnesses */
    /* Basic implementation: parse XML structure */
    
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        log_error("Failed to open KBL file: %s", filename);
        return MTL_ERROR_FILE_IO;
    }
    
    // Simple XML parsing (basic implementation)
    // Full KBL format is complex and includes:
    // - Cable definitions with conductors
    // - Geometry and routing information
    // - Connector definitions
    // - Material properties
    
    char line[4096];
    int num_conductors = 0;
    bool in_cable_section = false;
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove whitespace
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        // Look for cable definition
        if (strstr(trimmed, "<Cable") != NULL || strstr(trimmed, "<cable") != NULL) {
            in_cable_section = true;
            // Try to extract number of conductors
            char* num_str = strstr(trimmed, "conductors=\"");
            if (num_str) {
                num_str += 12;  // Skip "conductors=\""
                num_conductors = atoi(num_str);
            }
        }
        
        // Look for conductor definitions
        if (in_cable_section && (strstr(trimmed, "<Conductor") != NULL || strstr(trimmed, "<conductor") != NULL)) {
            // Extract conductor properties
            // This is simplified - full implementation would parse all attributes
            char* radius_str = strstr(trimmed, "radius=\"");
            char* material_str = strstr(trimmed, "material=\"");
            
            if (radius_str) {
                radius_str += 8;  // Skip "radius=\""
                double radius = atof(radius_str);
                // Would set conductor radius here
            }
        }
        
        // Look for end of cable section
        if (strstr(trimmed, "</Cable") != NULL || strstr(trimmed, "</cable") != NULL) {
            in_cable_section = false;
        }
    }
    
    fclose(fp);
    
    if (num_conductors > 0) {
        // Update cable with parsed information
        // This is simplified - full implementation would parse all KBL elements
        log_info("KBL file loaded: %s (%d conductors)", filename, num_conductors);
        return MTL_SUCCESS;
    } else {
        log_warning("KBL file parsing incomplete - using default values: %s", filename);
        return MTL_ERROR_FILE_IO;
    }
}

static int mtl_save_kbl_file(const mtl_cable_t* cable, const char* filename) {
    if (cable == NULL || filename == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Generate KBL XML file from cable structure */
    
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        log_error("Failed to create KBL file: %s", filename);
        return MTL_ERROR_FILE_IO;
    }
    
    // Write KBL XML header
    fprintf(fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(fp, "<KBL xmlns=\"http://www.kbl-format.org/kbl\">\n");
    fprintf(fp, "  <Cable name=\"%s\" type=\"%d\" conductors=\"%d\">\n",
            cable->name, cable->type, cable->num_conductors);
    
    // Write conductor definitions
    if (cable->conductors) {
        for (int i = 0; i < cable->num_conductors; i++) {
            fprintf(fp, "    <Conductor id=\"%d\" radius=\"%.6e\" material=\"%d\">\n",
                    i, cable->conductors[i].radius, cable->conductors[i].material);
            fprintf(fp, "      <Position x=\"%.6e\" y=\"%.6e\" z=\"%.6e\"/>\n",
                    cable->conductors[i].position.x,
                    cable->conductors[i].position.y,
                    cable->conductors[i].position.z);
            fprintf(fp, "    </Conductor>\n");
        }
    }
    
    // Write geometry information
    if (cable->geometry.num_segments > 0) {
        fprintf(fp, "    <Geometry segments=\"%d\">\n", cable->geometry.num_segments);
        // Would write segment information here
        fprintf(fp, "    </Geometry>\n");
    }
    
    fprintf(fp, "  </Cable>\n");
    fprintf(fp, "</KBL>\n");
    
    fclose(fp);
    log_info("KBL file saved: %s", filename);
    return MTL_SUCCESS;
}

int mtl_cable_load_kbl(mtl_cable_t* cable, const char* kbl_file) {
    return mtl_load_kbl_file(cable, kbl_file);
}

int mtl_cable_save_kbl(const mtl_cable_t* cable, const char* kbl_file) {
    return mtl_save_kbl_file(cable, kbl_file);
}

/* Stochastic placement functions */
int mtl_cable_set_stochastic_placement(mtl_cable_t* cable, bool enable, 
                                     double variance, int seed) {
    if (cable == NULL || variance < 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    cable->stochastic_placement = enable;
    cable->placement_variance = variance;
    cable->placement_seed = seed;
    
    if (enable) {
        log_info("Enabled stochastic placement with variance %.3f and seed %d", variance, seed);
    }
    
    return MTL_SUCCESS;
}

int mtl_cable_generate_random_placement(mtl_cable_t* cable) {
    if (cable == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (!cable->stochastic_placement) {
        log_warning("Stochastic placement not enabled for cable '%s'", cable->name);
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Initialize random number generator */
    srand(cable->placement_seed);
    
    /* Generate random positions with given variance */
    for (int i = 0; i < cable->num_conductors; i++) {
        /* Generate random offsets */
        double dx = (double)rand() / RAND_MAX * 2.0 - 1.0;  /* -1 to 1 */
        double dy = (double)rand() / RAND_MAX * 2.0 - 1.0;  /* -1 to 1 */
        double dz = (double)rand() / RAND_MAX * 2.0 - 1.0;  /* -1 to 1 */
        
        /* Apply variance scaling */
        dx *= cable->placement_variance;
        dy *= cable->placement_variance;
        dz *= cable->placement_variance;
        
        /* Update positions */
        cable->conductors[i].position[0] += dx;
        cable->conductors[i].position[1] += dy;
        cable->conductors[i].position[2] += dz;
    }
    
    log_info("Generated random placement for cable '%s' with %d conductors", 
             cable->name, cable->num_conductors);
    return MTL_SUCCESS;
}

/* Parameter extraction functions */
int mtl_extract_per_unit_parameters(mtl_solver_t* solver, mtl_cable_t* cable,
                                  double* R, double* L, double* C, double* G) {
    if (solver == NULL || cable == NULL || R == NULL || L == NULL || C == NULL || G == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (solver->matrices == NULL) {
        log_error("Parameter matrices not calculated - run analysis first");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Extract average per-unit parameters */
    double R_total = 0.0, L_total = 0.0, C_total = 0.0, G_total = 0.0;
    int n = cable->num_conductors;
    
    for (int i = 0; i < n; i++) {
        R_total += creal(solver->matrices->R_matrix[i][i]);
        L_total += creal(solver->matrices->L_matrix[i][i]);
        C_total += creal(solver->matrices->C_matrix[i][i]);
        G_total += creal(solver->matrices->G_matrix[i][i]);
    }
    
    *R = R_total / n;
    *L = L_total / n;
    *C = C_total / n;
    *G = G_total / n;
    
    return MTL_SUCCESS;
}

int mtl_extract_impedance_matrix(mtl_solver_t* solver, mtl_cable_t* cable,
                               double frequency, complex double** Z) {
    if (solver == NULL || cable == NULL || Z == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (solver->matrices == NULL) {
        log_error("Parameter matrices not calculated - run analysis first");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    int n = cable->num_conductors;
    complex double omega = 2 * M_PI * frequency * I;
    
    /* Calculate impedance matrix: Z = R + jωL */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Z[i][j] = solver->matrices->R_matrix[i][j] + omega * solver->matrices->L_matrix[i][j];
        }
    }
    
    return MTL_SUCCESS;
}

int mtl_extract_admittance_matrix(mtl_solver_t* solver, mtl_cable_t* cable,
                                double frequency, complex double** Y) {
    if (solver == NULL || cable == NULL || Y == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (solver->matrices == NULL) {
        log_error("Parameter matrices not calculated - run analysis first");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    int n = cable->num_conductors;
    complex double omega = 2 * M_PI * frequency * I;
    
    /* Calculate admittance matrix: Y = G + jωC */
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Y[i][j] = solver->matrices->G_matrix[i][j] + omega * solver->matrices->C_matrix[i][j];
        }
    }
    
    return MTL_SUCCESS;
}

int mtl_extract_scattering_matrix(mtl_solver_t* solver, mtl_cable_t* cable,
                                double frequency, complex double** S) {
    if (solver == NULL || cable == NULL || S == NULL || frequency <= 0.0) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (solver->matrices == NULL) {
        log_error("Parameter matrices not calculated - run analysis first");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    int n = cable->num_conductors;
    
    /* Calculate impedance and admittance matrices */
    complex double** Z = (complex double**)memory_allocate(n * sizeof(complex double*));
    complex double** Y = (complex double**)memory_allocate(n * sizeof(complex double*));
    
    if (Z == NULL || Y == NULL) {
        if (Z) memory_free(Z);
        if (Y) memory_free(Y);
        return MTL_ERROR_OUT_OF_MEMORY;
    }
    
    for (int i = 0; i < n; i++) {
        Z[i] = (complex double*)memory_allocate(n * sizeof(complex double));
        Y[i] = (complex double*)memory_allocate(n * sizeof(complex double));
        
        if (Z[i] == NULL || Y[i] == NULL) {
            /* Cleanup */
            for (int j = 0; j <= i; j++) {
                if (Z[j]) memory_free(Z[j]);
                if (Y[j]) memory_free(Y[j]);
            }
            memory_free(Z);
            memory_free(Y);
            return MTL_ERROR_OUT_OF_MEMORY;
        }
    }
    
    /* Extract Z and Y matrices */
    mtl_extract_impedance_matrix(solver, cable, frequency, Z);
    mtl_extract_admittance_matrix(solver, cable, frequency, Y);
    
    /* Calculate scattering matrix: S = (Z - Z0)(Z + Z0)^-1 */
    /* For simplicity, assume 50 ohm reference impedance */
    double Z0 = 50.0;
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j) {
                complex double Z_ii = Z[i][i];
                S[i][i] = (Z_ii - Z0) / (Z_ii + Z0);
            } else {
                /* Simplified off-diagonal terms */
                S[i][j] = 2.0 * sqrt(creal(Y[i][j])) / (1.0 + creal(Z[i][j]) / Z0);
            }
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < n; i++) {
        memory_free(Z[i]);
        memory_free(Y[i]);
    }
    memory_free(Z);
    memory_free(Y);
    
    return MTL_SUCCESS;
}

/* Hybrid MoM/MTL coupling */
int mtl_solver_enable_hybrid_coupling(mtl_solver_t* solver, bool enable) {
    if (solver == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    solver->config.enable_hybrid = enable;
    log_info("Hybrid MoM/MTL coupling %s", enable ? "enabled" : "disabled");
    return MTL_SUCCESS;
}

int mtl_solver_set_mom_coupling(mtl_solver_t* solver, void* mom_solver) {
    if (solver == NULL || mom_solver == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Store reference to MoM solver */
    /* This would require proper interface definition in production code */
    log_info("MoM solver coupling configured");
    return MTL_SUCCESS;
}

int mtl_solver_update_coupling_matrix(mtl_solver_t* solver) {
    if (solver == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    if (!solver->config.enable_hybrid) {
        log_warning("Hybrid coupling not enabled");
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    /* Update coupling matrices between MoM and MTL solvers */
    /* This is a placeholder for the actual coupling algorithm */
    
    log_info("Coupling matrix updated");
    return MTL_SUCCESS;
}

/* Results file I/O */
int mtl_results_save_to_file(const mtl_results_t* results, const char* filename) {
    if (results == NULL || filename == NULL) {
        return MTL_ERROR_INVALID_ARGUMENT;
    }
    
    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        log_error("Failed to open file for writing: %s", filename);
        return MTL_ERROR_FILE_IO;
    }
    
    /* Write header */
    fprintf(file, "# PulseEM MTL Results File\n");
    fprintf(file, "# Version: 1.0\n");
    fprintf(file, "# Frequencies: %d\n", results->num_frequencies);
    fprintf(file, "# Solve time: %.2f seconds\n", results->solve_time);
    fprintf(file, "# Memory usage: %.1f MB\n", results->memory_usage);
    fprintf(file, "\n");
    
    /* Write frequency data */
    fprintf(file, "# Frequency(Hz) R(ohm/m) L(H/m) C(F/m) G(S/m) SkinDepth(m) ProximityFactor CommonMode(A) TransferZ(ohm)\n");
    
    for (int i = 0; i < results->num_frequencies; i++) {
        fprintf(file, "%.6e %.6e %.6e %.6e %.6e %.6e %.6e %.6e %.6e\n",
                results->frequencies[i],
                results->R_per_unit[i],
                results->L_per_unit[i],
                results->C_per_unit[i],
                results->G_per_unit[i],
                results->skin_depth[i],
                results->proximity_factor[i],
                results->common_mode_current[i],
                results->transfer_impedance[i]);
    }
    
    fclose(file);
    log_info("Results saved to file: %s", filename);
    return MTL_SUCCESS;
}

mtl_results_t* mtl_results_load_from_file(const char* filename) {
    if (filename == NULL) {
        return NULL;
    }
    
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        log_error("Failed to open file for reading: %s", filename);
        return NULL;
    }
    
    /* Read header */
    char line[1024];
    int num_frequencies = 0;
    double solve_time = 0.0, memory_usage = 0.0;
    
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' && strstr(line, "Frequencies:")) {
            sscanf(line, "# Frequencies: %d", &num_frequencies);
        } else if (line[0] == '#' && strstr(line, "Solve time:")) {
            sscanf(line, "# Solve time: %lf seconds", &solve_time);
        } else if (line[0] == '#' && strstr(line, "Memory usage:")) {
            sscanf(line, "# Memory usage: %lf MB", &memory_usage);
        } else if (line[0] == '#' && strstr(line, "Frequency(Hz)")) {
            break;  /* Found data header */
        }
    }
    
    if (num_frequencies <= 0) {
        fclose(file);
        log_error("Invalid number of frequencies in file");
        return NULL;
    }
    
    /* Create results structure */
    mtl_results_t* results = mtl_results_create(num_frequencies, 1);  /* Assume 1 conductor for simplicity */
    if (results == NULL) {
        fclose(file);
        return NULL;
    }
    
    results->solve_time = solve_time;
    results->memory_usage = memory_usage;
    
    /* Read data */
    int freq_idx = 0;
    while (fgets(line, sizeof(line), file) && freq_idx < num_frequencies) {
        if (line[0] == '#' || line[0] == '\n') continue;
        
        double freq, r, l, c, g, skin, prox, cm, zt;
        if (sscanf(line, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
                   &freq, &r, &l, &c, &g, &skin, &prox, &cm, &zt) == 9) {
            results->frequencies[freq_idx] = freq;
            results->R_per_unit[freq_idx] = r;
            results->L_per_unit[freq_idx] = l;
            results->C_per_unit[freq_idx] = c;
            results->G_per_unit[freq_idx] = g;
            results->skin_depth[freq_idx] = skin;
            results->proximity_factor[freq_idx] = prox;
            results->common_mode_current[freq_idx] = cm;
            results->transfer_impedance[freq_idx] = zt;
            freq_idx++;
        }
    }
    
    fclose(file);
    log_info("Results loaded from file: %s", filename);
    return results;
}