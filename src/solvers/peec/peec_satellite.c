/**
 * @file peec_satellite.c
 * @brief Satellite HPM electromagnetic scattering PEEC solver implementation
 * @details C implementation for PEC materials, plane wave excitation, and scattering
 */

#include "peec_satellite.h"
#include "peec_solver.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../operators/kernels/core_kernels.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>

#define SATELLITE_SPEED_OF_LIGHT 299792458.0
#define SATELLITE_MU0 (4.0 * M_PI * 1e-7)
#define SATELLITE_EPS0 (8.854187817e-12)
#define SATELLITE_PEC_CONDUCTIVITY 1e20
#define SATELLITE_10GHZ_FREQ 10.0e9

/**
 * @brief Internal satellite solver state
 */
typedef struct peec_satellite_solver {
    peec_satellite_config_t config;              /**< Configuration */
    peec_solver_t* base_solver;                  /**< Base PEEC solver */
    
    /* Geometry data */
    geom_geometry_t* geometry;                     /**< Satellite geometry */
    mesh_t* surface_mesh;                        /**< Surface mesh for PEC */
    
    /* Satellite elements */
    peec_satellite_element_t* elements;          /**< Satellite PEEC elements */
    int num_elements;                            /**< Number of elements */
    int num_pec_elements;                        /**< Number of PEC elements */
    
    /* Plane wave data */
    double k_vector[3];                          /**< Wave vector */
    double e0_vector[3];                         /**< Electric field polarization */
    double h0_vector[3];                         /**< Magnetic field polarization */
    double complex plane_wave_phase;            /**< Plane wave phase factor */
    
    /* Surface currents */
    double complex* surface_currents;              /**< PEC surface currents */
    double complex* surface_charges;             /**< PEC surface charges */
    
    /* Field data */
    geom_point_t* observation_points;            /**< Observation points */
    double complex* incident_e_field;            /**< Incident E-field */
    double complex* scattered_e_field;           /**< Scattered E-field */
    double complex* total_e_field;               /**< Total E-field */
    double complex* incident_h_field;            /**< Incident H-field */
    double complex* scattered_h_field;           /**< Scattered H-field */
    double complex* total_h_field;               /**< Total H-field */
    
    /* Scattering data */
    double* radar_cross_section;                 /**< RCS values */
    double* scattering_pattern_theta;            /**< Theta angles for pattern */
    double* scattering_pattern_phi;              /**< Phi angles for pattern */
    double complex* scattering_matrix;           /**< Scattering matrix */
    
    /* Performance metrics */
    double setup_time;                           /**< Setup time */
    double element_extraction_time;              /**< Element extraction time */
    double field_computation_time;               /**< Field computation time */
    double total_time;                           /**< Total solution time */
    double memory_usage_mb;                      /**< Memory usage */
    
} peec_satellite_solver_internal_t;

/*********************************************************************
 * Utility Functions
 *********************************************************************/

/**
 * @brief Compute wavelength from frequency
 */
double peec_satellite_compute_wavelength(double frequency) {
    return SATELLITE_SPEED_OF_LIGHT / frequency;
}

/**
 * @brief Normalize a 3D vector
 */
void peec_satellite_normalize_vector(double vector[3]) {
    double norm = sqrt(vector[0]*vector[0] + vector[1]*vector[1] + vector[2]*vector[2]);
    if (norm > 1e-12) {
        vector[0] /= norm;
        vector[1] /= norm;
        vector[2] /= norm;
    }
}

/**
 * @brief Compute dot product of two 3D vectors
 */
double peec_satellite_dot_product(const double a[3], const double b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/**
 * @brief Compute cross product of two 3D vectors
 */
void peec_satellite_cross_product(const double a[3], const double b[3], double result[3]) {
    result[0] = a[1]*b[2] - a[2]*b[1];
    result[1] = a[2]*b[0] - a[0]*b[2];
    result[2] = a[0]*b[1] - a[1]*b[0];
}

/**
 * @brief Compute plane wave field at a point
 */
double complex peec_satellite_get_plane_wave_field(const double point[3],
                                                 const double k_vector[3],
                                                 const double e0_vector[3],
                                                 double amplitude, double phase,
                                                 double frequency) {
    double k_dot_r = peec_satellite_dot_product(k_vector, point);
    double omega = 2.0 * M_PI * frequency;
    double k_magnitude = peec_satellite_dot_product(k_vector, k_vector);
    
    if (k_magnitude < 1e-12) return 0.0 + 0.0*I;
    
    double wave_phase = k_dot_r + phase;
    return amplitude * (cexp(I * wave_phase));
}

/*********************************************************************
 * Satellite Solver Lifecycle
 *********************************************************************/

/**
 * @brief Create satellite PEEC solver
 */
peec_satellite_solver_t* peec_satellite_create(void) {
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)
        calloc(1, sizeof(peec_satellite_solver_internal_t));
    
    if (!solver) {
        fprintf(stderr, "Failed to allocate satellite solver\n");
        return NULL;
    }
    
    /* Create base PEEC solver */
    solver->base_solver = peec_solver_create();
    if (!solver->base_solver) {
        fprintf(stderr, "Failed to create base PEEC solver\n");
        free(solver);
        return NULL;
    }
    
    /* Initialize default configuration */
    solver->config.frequency = SATELLITE_10GHZ_FREQ;
    solver->config.wavelength = peec_satellite_compute_wavelength(SATELLITE_10GHZ_FREQ);
    solver->config.pec_permittivity = 1.0;
    solver->config.pec_permeability = 1.0;
    solver->config.pec_conductivity = SATELLITE_PEC_CONDUCTIVITY;
    solver->config.apply_coordinate_correction = true;
    
    /* Default domain settings */
    solver->config.domain.size[0] = 3.4;
    solver->config.domain.size[1] = 3.4;
    solver->config.domain.size[2] = 1.4;
    solver->config.domain.center[0] = 1.7;
    solver->config.domain.center[1] = 1.7;
    solver->config.domain.center[2] = 0.7;
    solver->config.domain.resolution = 0.01; /* 1 cm resolution */
    
    /* Default geometry translation (center satellite) */
    solver->config.geometry_translation[0] = 1.7;
    solver->config.geometry_translation[1] = 1.7;
    solver->config.geometry_translation[2] = 0.14;
    
    /* Default plane wave (10GHz, x-polarized, z-propagating) */
    solver->config.plane_wave.direction[0] = 0.0;
    solver->config.plane_wave.direction[1] = 0.0;
    solver->config.plane_wave.direction[2] = 1.0;
    solver->config.plane_wave.polarization[0] = 1.0;
    solver->config.plane_wave.polarization[1] = 0.0;
    solver->config.plane_wave.polarization[2] = 0.0;
    solver->config.plane_wave.amplitude = 1.0;
    solver->config.plane_wave.phase = 0.0;
    
    return (peec_satellite_solver_t*)solver;
}

/**
 * @brief Destroy satellite PEEC solver
 */
void peec_satellite_destroy(peec_satellite_solver_t* solver_ptr) {
    if (!solver_ptr) return;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    /* Destroy base solver */
    if (solver->base_solver) {
        peec_solver_destroy(solver->base_solver);
    }
    
    /* Free geometry data */
    if (solver->geometry) {
        /* Assuming geometry has proper destructor */
        free(solver->geometry);
    }
    
    if (solver->surface_mesh) {
        /* Assuming mesh has proper destructor */
        free(solver->surface_mesh);
    }
    
    /* Free element data */
    if (solver->elements) {
        for (int i = 0; i < solver->num_elements; i++) {
            if (solver->elements[i].vertices) {
                free(solver->elements[i].vertices);
            }
        }
        free(solver->elements);
    }
    
    /* Free field data */
    free(solver->surface_currents);
    free(solver->surface_charges);
    free(solver->observation_points);
    free(solver->incident_e_field);
    free(solver->scattered_e_field);
    free(solver->total_e_field);
    free(solver->incident_h_field);
    free(solver->scattered_h_field);
    free(solver->total_h_field);
    
    /* Free scattering data */
    free(solver->radar_cross_section);
    free(solver->scattering_pattern_theta);
    free(solver->scattering_pattern_phi);
    free(solver->scattering_matrix);
    
    free(solver);
}

/*********************************************************************
 * Configuration and Setup
 *********************************************************************/

/**
 * @brief Configure satellite solver
 */
int peec_satellite_configure(peec_satellite_solver_t* solver_ptr, 
                           const peec_satellite_config_t* config) {
    if (!solver_ptr || !config) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    solver->config = *config;
    
    /* Update wavelength from frequency */
    solver->config.wavelength = peec_satellite_compute_wavelength(config->frequency);
    
    /* Compute wave vector */
    double k_magnitude = 2.0 * M_PI / solver->config.wavelength;
    for (int i = 0; i < 3; i++) {
        solver->k_vector[i] = k_magnitude * config->plane_wave.direction[i];
    }
    
    /* Compute magnetic field polarization (H0 = (k x E0) / (omega * mu0)) */
    double omega = 2.0 * M_PI * config->frequency;
    double cross[3];
    peec_satellite_cross_product(config->plane_wave.direction, config->plane_wave.polarization, cross);
    
    for (int i = 0; i < 3; i++) {
        solver->h0_vector[i] = cross[i] / (omega * SATELLITE_MU0);
    }
    
    /* Normalize H0 vector */
    peec_satellite_normalize_vector(solver->h0_vector);
    
    /* Compute plane wave phase factor */
    solver->plane_wave_phase = config->plane_wave.amplitude * cexp(I * config->plane_wave.phase);
    
    return 0;
}

/**
 * @brief Set plane wave excitation
 */
int peec_satellite_set_plane_wave(peec_satellite_solver_t* solver_ptr,
                                const double direction[3],
                                const double polarization[3],
                                double amplitude, double phase) {
    if (!solver_ptr || !direction || !polarization) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    /* Copy direction and polarization */
    for (int i = 0; i < 3; i++) {
        solver->config.plane_wave.direction[i] = direction[i];
        solver->config.plane_wave.polarization[i] = polarization[i];
    }
    solver->config.plane_wave.amplitude = amplitude;
    solver->config.plane_wave.phase = phase;
    
    /* Normalize direction and polarization */
    peec_satellite_normalize_vector(solver->config.plane_wave.direction);
    peec_satellite_normalize_vector(solver->config.plane_wave.polarization);
    
    /* Update wave vector */
    double k_magnitude = 2.0 * M_PI / solver->config.wavelength;
    for (int i = 0; i < 3; i++) {
        solver->k_vector[i] = k_magnitude * solver->config.plane_wave.direction[i];
    }
    
    /* Update magnetic field polarization */
    double omega = 2.0 * M_PI * solver->config.frequency;
    double cross[3];
    peec_satellite_cross_product(solver->config.plane_wave.direction, 
                               solver->config.plane_wave.polarization, cross);
    
    for (int i = 0; i < 3; i++) {
        solver->h0_vector[i] = cross[i] / (omega * SATELLITE_MU0);
    }
    peec_satellite_normalize_vector(solver->h0_vector);
    
    /* Update plane wave phase factor */
    solver->plane_wave_phase = amplitude * cexp(I * phase);
    
    return 0;
}

/**
 * @brief Set PEC material properties
 */
int peec_satellite_set_pec_materials(peec_satellite_solver_t* solver_ptr,
                                     double epsr, double mur, double sigma) {
    if (!solver_ptr) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    solver->config.pec_permittivity = epsr;
    solver->config.pec_permeability = mur;
    solver->config.pec_conductivity = sigma;
    
    return 0;
}

/**
 * @brief Apply coordinate correction for satellite positioning
 */
int peec_satellite_apply_coordinate_correction(peec_satellite_solver_t* solver_ptr,
                                             const double translation[3]) {
    if (!solver_ptr || !translation) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    for (int i = 0; i < 3; i++) {
        solver->config.geometry_translation[i] = translation[i];
    }
    solver->config.apply_coordinate_correction = true;
    
    return 0;
}

/*********************************************************************
 * PEC Surface Modeling
 *********************************************************************/

/**
 * @brief Compute PEC surface impedance
 */
double complex peec_satellite_get_pec_surface_impedance(double frequency,
                                                        double conductivity) {
    if (conductivity < 1e12) {
        /* For high conductivity materials, use surface impedance approximation */
        double omega = 2.0 * M_PI * frequency;
        double skin_depth = sqrt(2.0 / (omega * SATELLITE_MU0 * conductivity));
        double surface_resistance = 1.0 / (conductivity * skin_depth);
        double surface_reactance = surface_resistance; /* Equal for good conductors */
        
        return surface_resistance + I * surface_reactance;
    } else {
        /* For perfect conductors, impedance is zero */
        return 0.0 + 0.0*I;
    }
}

/**
 * @brief Extract PEC partial elements
 */
int peec_satellite_extract_pec_partial_elements(peec_satellite_solver_t* solver_ptr) {
    if (!solver_ptr) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    if (!solver->surface_mesh || solver->num_pec_elements == 0) {
        fprintf(stderr, "No PEC surfaces available for element extraction\n");
        return -1;
    }
    
    /* Allocate satellite elements */
    solver->elements = (peec_satellite_element_t*)calloc(solver->num_pec_elements, 
                                                           sizeof(peec_satellite_element_t));
    if (!solver->elements) {
        fprintf(stderr, "Failed to allocate satellite elements\n");
        return -1;
    }
    
    /* Extract partial elements for each PEC surface */
    for (int i = 0; i < solver->num_pec_elements; i++) {
        peec_satellite_element_t* elem = &solver->elements[i];
        
        /* Initialize as PEC surface */
        elem->is_pec_surface = true;
        elem->relative_permittivity = solver->config.pec_permittivity;
        elem->relative_permeability = solver->config.pec_permeability;
        elem->conductivity = solver->config.pec_conductivity;
        
        /* Set surface impedance for PEC */
        elem->base_element.impedance = peec_satellite_get_pec_surface_impedance(
            solver->config.frequency, elem->conductivity);
        
        /* Set element type and material */
        elem->base_element.type = PEEC_ELEMENT_IMPEDANCE;
        elem->base_element.material_id = 1; /* PEC material ID */
        
        /* Initialize field values */
        elem->incident_field = 0.0 + 0.0*I;
        elem->scattered_field = 0.0 + 0.0*I;
        elem->total_field = 0.0 + 0.0*I;
    }
    
    solver->num_elements = solver->num_pec_elements;
    
    return 0;
}

/**
 * @brief Compute incident field at a point
 */
int peec_satellite_compute_incident_field(peec_satellite_solver_t* solver_ptr,
                                        const geom_point_t* point,
                                        double complex* e_field,
                                        double complex* h_field) {
    if (!solver_ptr || !point || !e_field || !h_field) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    /* Apply coordinate correction if needed */
    double corrected_point[3];
    if (solver->config.apply_coordinate_correction) {
        for (int i = 0; i < 3; i++) {
            corrected_point[i] = point->coords[i] + solver->config.geometry_translation[i];
        }
    } else {
        for (int i = 0; i < 3; i++) {
            corrected_point[i] = point->coords[i];
        }
    }
    
    /* Compute incident electric field */
    double complex e_field_value = peec_satellite_get_plane_wave_field(
        corrected_point, solver->k_vector, solver->config.plane_wave.polarization,
        solver->config.plane_wave.amplitude, solver->config.plane_wave.phase,
        solver->config.frequency);
    
    /* Compute incident magnetic field (H = (k x E) / (omega * mu0)) */
    double omega = 2.0 * M_PI * solver->config.frequency;
    double complex h_field_value = e_field_value / (omega * SATELLITE_MU0);
    
    /* Set field components */
    for (int i = 0; i < 3; i++) {
        e_field[i] = e_field_value * solver->config.plane_wave.polarization[i];
        h_field[i] = h_field_value * solver->h0_vector[i];
    }
    
    return 0;
}

/**
 * @brief Apply plane wave excitation to PEC surfaces
 */
int peec_satellite_apply_plane_wave_excitation(peec_satellite_solver_t* solver_ptr) {
    if (!solver_ptr) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    if (!solver->elements || solver->num_elements == 0) {
        fprintf(stderr, "No elements available for excitation\n");
        return -1;
    }
    
    /* Apply plane wave excitation to each PEC element */
    for (int i = 0; i < solver->num_elements; i++) {
        peec_satellite_element_t* elem = &solver->elements[i];
        
        if (!elem->is_pec_surface) continue;
        
        /* Compute element center position */
        double element_center[3];
        /* This would need to be implemented based on actual element geometry */
        element_center[0] = 0.0; /* Placeholder */
        element_center[1] = 0.0;
        element_center[2] = 0.0;
        
        /* Apply coordinate correction */
        if (solver->config.apply_coordinate_correction) {
            for (int j = 0; j < 3; j++) {
                element_center[j] += solver->config.geometry_translation[j];
            }
        }
        
        /* Compute incident field at element center */
        double complex incident_e[3], incident_h[3];
        geom_point_t center_point;
        for (int j = 0; j < 3; j++) {
            center_point.coords[j] = element_center[j];
        }
        
        if (peec_satellite_compute_incident_field(solver_ptr, &center_point, 
                                                 incident_e, incident_h) != 0) {
            continue;
        }
        
        /* Project incident field onto surface normal */
        double complex normal_component = 0.0 + 0.0*I;
        for (int j = 0; j < 3; j++) {
            normal_component += incident_e[j] * elem->surface_normal[j];
        }
        
        elem->incident_field = normal_component;
        
        /* For PEC surfaces, total tangential E-field should be zero */
        /* This will be enforced in the boundary condition step */
    }
    
    return 0;
}

/*********************************************************************
 * Electromagnetic Scattering
 *********************************************************************/

/**
 * @brief Compute scattered field at observation points
 */
int peec_satellite_compute_scattered_field(peec_satellite_solver_t* solver_ptr,
                                         const geom_point_t* observation_points,
                                         int num_points,
                                         double complex* scattered_e_field,
                                         double complex* scattered_h_field) {
    if (!solver_ptr || !observation_points || num_points <= 0 || 
        !scattered_e_field || !scattered_h_field) {
        return -1;
    }
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    if (!solver->surface_currents || solver->num_pec_elements == 0) {
        fprintf(stderr, "No surface currents available for scattered field computation\n");
        return -1;
    }
    
    /* Initialize scattered fields to zero */
    for (int i = 0; i < num_points * 3; i++) {
        scattered_e_field[i] = 0.0 + 0.0*I;
        scattered_h_field[i] = 0.0 + 0.0*I;
    }
    
    /* Compute scattered field from each PEC element */
    for (int elem_idx = 0; elem_idx < solver->num_pec_elements; elem_idx++) {
        peec_satellite_element_t* elem = &solver->elements[elem_idx];
        
        if (!elem->is_pec_surface) continue;
        
        /* Get element center and current */
        double element_center[3];
        /* This would need proper implementation based on element geometry */
        element_center[0] = 0.0; /* Placeholder */
        element_center[1] = 0.0;
        element_center[2] = 0.0;
        
        double complex element_current = solver->surface_currents[elem_idx];
        double complex element_charge = solver->surface_charges[elem_idx];
        
        /* Compute contribution to each observation point */
        for (int obs_idx = 0; obs_idx < num_points; obs_idx++) {
            double observation_point[3];
            
            /* Apply coordinate correction to observation point */
            if (solver->config.apply_coordinate_correction) {
                for (int j = 0; j < 3; j++) {
                    observation_point[j] = observation_points[obs_idx].coords[j] + 
                                       solver->config.geometry_translation[j];
                }
            } else {
                for (int j = 0; j < 3; j++) {
                    observation_point[j] = observation_points[obs_idx].coords[j];
                }
            }
            
            /* Compute distance vector */
            double r_vector[3];
            double distance = 0.0;
            for (int j = 0; j < 3; j++) {
                r_vector[j] = observation_point[j] - element_center[j];
                distance += r_vector[j] * r_vector[j];
            }
            distance = sqrt(distance);
            
            if (distance < 1e-6) continue; /* Skip self-interaction */
            
            /* Normalize distance vector */
            for (int j = 0; j < 3; j++) {
                r_vector[j] /= distance;
            }
            
            /* Compute Green's function */
            double k = 2.0 * M_PI / solver->config.wavelength;
            double complex green = cexp(I * k * distance) / (4.0 * M_PI * distance);
            
            /* Electric field contribution (simplified radiation formula) */
            double complex e_contrib[3];
            for (int j = 0; j < 3; j++) {
                e_contrib[j] = -I * omega * SATELLITE_MU0 * element_current * green;
                /* Add charge contribution */
                e_contrib[j] += r_vector[j] * element_charge * green / SATELLITE_EPS0;
            }
            
            /* Add to scattered field */
            for (int j = 0; j < 3; j++) {
                scattered_e_field[obs_idx * 3 + j] += e_contrib[j];
            }
        }
    }
    
    return 0;
}

/**
 * @brief Compute total field (incident + scattered)
 */
int peec_satellite_compute_total_field(peec_satellite_solver_t* solver_ptr,
                                     const geom_point_t* observation_points,
                                     int num_points,
                                     double complex* total_e_field,
                                     double complex* total_h_field) {
    if (!solver_ptr || !observation_points || num_points <= 0 || 
        !total_e_field || !total_h_field) {
        return -1;
    }
    
    /* First compute scattered field */
    double complex* scattered_e = (double complex*)calloc(num_points * 3, sizeof(double complex));
    double complex* scattered_h = (double complex*)calloc(num_points * 3, sizeof(double complex));
    
    if (!scattered_e || !scattered_h) {
        free(scattered_e);
        free(scattered_h);
        return -1;
    }
    
    if (peec_satellite_compute_scattered_field(solver_ptr, observation_points, num_points,
                                             scattered_e, scattered_h) != 0) {
        free(scattered_e);
        free(scattered_h);
        return -1;
    }
    
    /* Then compute incident field and add to scattered field */
    for (int i = 0; i < num_points; i++) {
        double complex incident_e[3], incident_h[3];
        
        if (peec_satellite_compute_incident_field(solver_ptr, &observation_points[i],
                                                 incident_e, incident_h) != 0) {
            continue;
        }
        
        /* Total field = incident + scattered */
        for (int j = 0; j < 3; j++) {
            total_e_field[i * 3 + j] = incident_e[j] + scattered_e[i * 3 + j];
            total_h_field[i * 3 + j] = incident_h[j] + scattered_h[i * 3 + j];
        }
    }
    
    free(scattered_e);
    free(scattered_h);
    
    return 0;
}

/*********************************************************************
 * Validation Functions
 *********************************************************************/

/**
 * @brief Validate plane wave orthogonality (k · E0 = 0)
 */
int peec_satellite_validate_plane_wave_orthogonality(peec_satellite_solver_t* solver_ptr) {
    if (!solver_ptr) return -1;
    
    peec_satellite_solver_internal_t* solver = (peec_satellite_solver_internal_t*)solver_ptr;
    
    double dot_product = peec_satellite_dot_product(solver->config.plane_wave.direction,
                                                   solver->config.plane_wave.polarization);
    
    if (fabs(dot_product) > 1e-6) {
        fprintf(stderr, "Warning: Plane wave direction and polarization not orthogonal (dot product = %e)\n", 
                dot_product);
        return 1;
    }
    
    return 0;
}

/**
 * @brief Print satellite configuration
 */
void peec_satellite_print_configuration(const peec_satellite_config_t* config) {
    if (!config) return;
    
    printf("=== Satellite PEEC Configuration ===\n");
    printf("Frequency: %.3e Hz (%.3f GHz)\n", config->frequency, config->frequency/1e9);
    printf("Wavelength: %.3f mm\n", config->wavelength * 1000);
    
    printf("\nPEC Material Properties:\n");
    printf("  Permittivity: %.1f\n", config->pec_permittivity);
    printf("  Permeability: %.1f\n", config->pec_permeability);
    printf("  Conductivity: %.1e S/m\n", config->pec_conductivity);
    
    printf("\nPlane Wave Excitation:\n");
    printf("  Direction: [%.3f, %.3f, %.3f]\n", 
           config->plane_wave.direction[0], 
           config->plane_wave.direction[1], 
           config->plane_wave.direction[2]);
    printf("  Polarization: [%.3f, %.3f, %.3f]\n",
           config->plane_wave.polarization[0],
           config->plane_wave.polarization[1], 
           config->plane_wave.polarization[2]);
    printf("  Amplitude: %.3f V/m\n", config->plane_wave.amplitude);
    printf("  Phase: %.3f rad\n", config->plane_wave.phase);
    
    printf("\nDomain Settings:\n");
    printf("  Size: [%.1f, %.1f, %.1f] m\n",
           config->domain.size[0],
           config->domain.size[1], 
           config->domain.size[2]);
    printf("  Center: [%.1f, %.1f, %.1f] m\n",
           config->domain.center[0],
           config->domain.center[1],
           config->domain.center[2]);
    
    if (config->apply_coordinate_correction) {
        printf("\nCoordinate Correction:\n");
        printf("  Translation: [%.3f, %.3f, %.3f] m\n",
               config->geometry_translation[0],
               config->geometry_translation[1], 
               config->geometry_translation[2]);
    }
    
    printf("=====================================\n");
}

/**
 * @brief Integration with base PEEC solver
 */
int peec_satellite_integrate_with_base_solver(peec_satellite_solver_t* satellite_solver_ptr,
                                            peec_solver_t* base_solver) {
    if (!satellite_solver_ptr || !base_solver) return -1;
    
    peec_satellite_solver_internal_t* satellite_solver = (peec_satellite_solver_internal_t*)satellite_solver_ptr;
    
    /* Configure base solver with satellite settings */
    peec_config_t base_config;
    if (peec_solver_get_config(base_solver, &base_config) != 0) {
        return -1;
    }
    
    /* Update base configuration */
    base_config.frequency = satellite_solver->config.frequency;
    base_config.formulation = PEEC_FORMULATION_FULL_WAVE;  // Use peec_formulation_t from physics layer
    base_config.include_retardation = true;
    base_config.retardation_tolerance = 1e-6;
    
    /* Apply configuration to base solver */
    if (peec_solver_configure(base_solver, &base_config) != 0) {
        return -1;
    }
    
    satellite_solver->base_solver = base_solver;
    
    return 0;
}