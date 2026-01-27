/**
 * @file peec_satellite.h
 * @brief Satellite HPM electromagnetic scattering PEEC solver extension
 * @details Handles PEC materials, coordinate mapping, and plane wave excitation
 */

#ifndef PEEC_SATELLITE_H
#define PEEC_SATELLITE_H

#include "peec_solver.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include <complex.h>

#ifdef __cplusplus
extern "C" {
#endif

// Satellite configuration parameters
typedef struct {
    // Frequency settings
    double frequency;           // 10 GHz for HPM
    double wavelength;          // Wavelength at 10 GHz
    
    // PEC material properties (Perfect Electric Conductor)
    double pec_permittivity;    // epsr = 1.0
    double pec_permeability;    // mur = 1.0  
    double pec_conductivity;  // sigma = 1e20 S/m
    
    // Coordinate system correction
    double geometry_translation[3]; // [1.7, 1.7, 0.14] m to center satellite
    bool apply_coordinate_correction;
    
    // Plane wave excitation
    struct {
        double direction[3];    // Wave vector direction (normalized)
        double polarization[3]; // Electric field polarization (normalized)
        double amplitude;       // Field amplitude (V/m)
        double phase;          // Phase in radians
    } plane_wave;
    
    // Domain settings
    struct {
        double size[3];         // [3.4, 3.4, 1.4] m
        double center[3];       // [1.7, 1.7, 0.7] m
        double resolution;      // Mesh resolution for PEEC
    } domain;
    
    // Observation points
    struct {
        geom_point_t* points;   // Observation point coordinates
        int num_points;        // Number of observation points
        bool include_scattered_field; // Include scattered field calculation
        bool include_total_field;     // Include total field calculation
    } observation;
    
    // Scattering analysis
    struct {
        bool compute_radar_cross_section;  // RCS calculation
        bool compute_scattering_pattern;   // 3D scattering pattern
        double* scattering_angles_theta;  // Theta angles for pattern
        double* scattering_angles_phi;    // Phi angles for pattern
        int num_theta_angles;
        int num_phi_angles;
    } scattering;
    
} peec_satellite_config_t;

// Satellite-specific PEEC element extensions
typedef struct {
    // Standard PEEC element
    peec_circuit_element_t base_element;
    
    // Satellite-specific properties
    bool is_pec_surface;        // True if this is a PEC surface element
    double surface_normal[3];   // Surface normal vector
    double area;                 // Surface area
    double* vertices;            // Surface vertices (for STL facets)
    int num_vertices;           // Number of vertices
    
    // Plane wave coupling
    double complex incident_field;  // Incident plane wave field
    double complex scattered_field; // Scattered field contribution
    double complex total_field;     // Total field (incident + scattered)
    
    // Material properties
    double relative_permittivity;
    double relative_permeability;
    double conductivity;
    
} peec_satellite_element_t;

// Satellite PEEC solver extension
typedef struct peec_satellite_solver peec_satellite_solver_t;

/*********************************************************************
 * Satellite Solver Lifecycle
 *********************************************************************/

peec_satellite_solver_t* peec_satellite_create(void);
void peec_satellite_destroy(peec_satellite_solver_t* solver);

/*********************************************************************
 * Configuration and Setup
 *********************************************************************/

int peec_satellite_configure(peec_satellite_solver_t* solver, 
                           const peec_satellite_config_t* config);
int peec_satellite_set_plane_wave(peec_satellite_solver_t* solver,
                                const double direction[3],
                                const double polarization[3],
                                double amplitude, double phase);
int peec_satellite_set_pec_materials(peec_satellite_solver_t* solver,
                                     double epsr, double mur, double sigma);
int peec_satellite_apply_coordinate_correction(peec_satellite_solver_t* solver,
                                             const double translation[3]);

/*********************************************************************
 * Geometry Processing
 *********************************************************************/

int peec_satellite_import_stl_geometry(peec_satellite_solver_t* solver,
                                     const char* filename,
                                     double scale_factor);
int peec_satellite_identify_pec_surfaces(peec_satellite_solver_t* solver);
int peec_satellite_create_surface_mesh(peec_satellite_solver_t* solver,
                                     double mesh_density);
int peec_satellite_validate_geometry(peec_satellite_solver_t* solver);

/*********************************************************************
 * Plane Wave Excitation
 *********************************************************************/

int peec_satellite_compute_incident_field(peec_satellite_solver_t* solver,
                                        const geom_point_t* point,
                                        double complex* e_field,
                                        double complex* h_field);
int peec_satellite_apply_plane_wave_excitation(peec_satellite_solver_t* solver);
double complex peec_satellite_get_plane_wave_field(const double point[3],
                                                 const double k_vector[3],
                                                 const double e0_vector[3],
                                                 double amplitude, double phase,
                                                 double frequency);

/*********************************************************************
 * PEC Surface Modeling
 *********************************************************************/

int peec_satellite_extract_pec_partial_elements(peec_satellite_solver_t* solver);
int peec_satellite_compute_pec_surface_currents(peec_satellite_solver_t* solver);
double complex peec_satellite_get_pec_surface_impedance(double frequency,
                                                        double conductivity);
int peec_satellite_enforce_pec_boundary_conditions(peec_satellite_solver_t* solver);

/*********************************************************************
 * Electromagnetic Scattering
 *********************************************************************/

int peec_satellite_compute_scattered_field(peec_satellite_solver_t* solver,
                                         const geom_point_t* observation_points,
                                         int num_points,
                                         double complex* scattered_e_field,
                                         double complex* scattered_h_field);
int peec_satellite_compute_total_field(peec_satellite_solver_t* solver,
                                     const geom_point_t* observation_points,
                                     int num_points,
                                     double complex* total_e_field,
                                     double complex* total_h_field);
int peec_satellite_compute_radar_cross_section(peec_satellite_solver_t* solver,
                                             double* rcs_values);
int peec_satellite_compute_scattering_pattern(peec_satellite_solver_t* solver);

/*********************************************************************
 * Field Visualization
 *********************************************************************/

int peec_satellite_export_field_data(peec_satellite_solver_t* solver,
                                   const char* filename,
                                   const char* format);
int peec_satellite_create_field_visualization(peec_satellite_solver_t* solver,
                                            const char* output_dir);
int peec_satellite_generate_scattering_plots(peec_satellite_solver_t* solver);

/*********************************************************************
 * Validation and Testing
 *********************************************************************/

int peec_satellite_validate_plane_wave_orthogonality(peec_satellite_solver_t* solver);
int peec_satellite_check_pec_boundary_conditions(peec_satellite_solver_t* solver);
int peec_satellite_verify_energy_conservation(peec_satellite_solver_t* solver);
double peec_satellite_compute_scattering_error(peec_satellite_solver_t* solver);

/*********************************************************************
 * Integration with Base PEEC Solver
 *********************************************************************/

int peec_satellite_integrate_with_base_solver(peec_satellite_solver_t* satellite_solver,
                                            peec_solver_t* base_solver);
int peec_satellite_transfer_results_to_base(peec_satellite_solver_t* satellite_solver,
                                          peec_result_t* base_results);

/*********************************************************************
 * Utility Functions
 *********************************************************************/

void peec_satellite_print_configuration(const peec_satellite_config_t* config);
void peec_satellite_print_element_info(const peec_satellite_element_t* element);
double peec_satellite_compute_wavelength(double frequency);
void peec_satellite_normalize_vector(double vector[3]);
double peec_satellite_dot_product(const double a[3], const double b[3]);
void peec_satellite_cross_product(const double a[3], const double b[3], double result[3]);

#ifdef __cplusplus
}
#endif

#endif // PEEC_SATELLITE_H