/******************************************************************************
 * Field Post-Processing Module
 * 
 * Comprehensive post-processing capabilities for electromagnetic simulation
 * 
 * Methods: Field visualization, power flow, statistics, near-to-far-field
 ******************************************************************************/

#ifndef POSTPROCESSING_FIELD_H
#define POSTPROCESSING_FIELD_H

#include "../../discretization/mesh/core_mesh.h"
#include "../../discretization/geometry/core_geometry.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Field Data Structures
 ******************************************************************************/

typedef struct {
    double* e_field_real;         // Electric field real part (3 components per point)
    double* e_field_imag;         // Electric field imaginary part (3 components per point)
    double* h_field_real;         // Magnetic field real part (3 components per point)
    double* h_field_imag;         // Magnetic field imaginary part (3 components per point)
    double* positions;             // Observation point positions (3 coordinates per point)
    int num_points;               // Number of observation points
    double frequency;             // Frequency (Hz)
} postprocessing_field_data_t;

typedef struct {
    double* current_real;          // Current real part (3 components per element)
    double* current_imag;          // Current imaginary part (3 components per element)
    double* current_magnitude;     // Current magnitude (per element)
    double* current_phase;        // Current phase (degrees)
    int num_elements;             // Number of mesh elements
    double frequency;             // Frequency (Hz)
    /* Optional: vertex-wise magnitude for smooth VTK display (单元间连续插值).
     * When set, export_vtk_current writes POINT_DATA so ParaView interpolates within cells. */
    double* current_magnitude_vertices;  /* length num_vertices, or NULL to use cell data only */
    int num_vertices;                    /* must match mesh->num_vertices when current_magnitude_vertices != NULL */
} postprocessing_current_distribution_t;

/******************************************************************************
 * Visualization Export Formats
 ******************************************************************************/

typedef enum {
    POSTPROCESSING_EXPORT_VTK,            // VTK format (ParaView)
    POSTPROCESSING_EXPORT_CSV,            // CSV format
    POSTPROCESSING_EXPORT_HDF5,           // HDF5 format
    POSTPROCESSING_EXPORT_TECPLOT,        // Tecplot format
    POSTPROCESSING_EXPORT_ENSIGHT,        // EnSight format
    POSTPROCESSING_EXPORT_OPENFOAM        // OpenFOAM format
} postprocessing_export_format_t;

/******************************************************************************
 * Field Analysis Functions
 ******************************************************************************/

/**
 * Export field data to visualization format
 * 
 * @param field_data Field data structure
 * @param mesh Mesh structure (for element connectivity)
 * @param filename Output filename
 * @param format Export format
 * @return 0 on success, negative on error
 */
int postprocessing_field_export(
    const postprocessing_field_data_t* field_data,
    const mesh_t* mesh,
    const char* filename,
    postprocessing_export_format_t format
);

/**
 * Export current distribution to visualization format
 * 
 * @param current_dist Current distribution structure
 * @param mesh Mesh structure
 * @param filename Output filename
 * @param format Export format
 * @return 0 on success, negative on error
 */
int postprocessing_current_export(
    const postprocessing_current_distribution_t* current_dist,
    const mesh_t* mesh,
    const char* filename,
    postprocessing_export_format_t format
);

/**
 * Calculate power flow through a surface
 * 
 * @param field_data Field data at surface points
 * @param surface_normal Surface normal vectors (3 components per point)
 * @param num_points Number of surface points
 * @param power_real Output: real part of power
 * @param power_imag Output: imaginary part of power
 * @return 0 on success, negative on error
 */
int postprocessing_field_calculate_power_flow(
    const postprocessing_field_data_t* field_data,
    const double* surface_normal,
    int num_points,
    double* power_real,
    double* power_imag
);

/**
 * Calculate field statistics
 * 
 * @param field_data Field data structure
 * @param stats Output: statistics (min, max, mean, rms)
 * @return 0 on success, negative on error
 */
int postprocessing_field_calculate_statistics(
    const postprocessing_field_data_t* field_data,
    double stats[4]  // [min, max, mean, rms]
);

/**
 * Transform near-field to far-field
 * 
 * @param near_field Near-field data
 * @param observation_angles Theta and phi angles (degrees)
 * @param num_angles Number of observation angles
 * @param far_field Output: far-field data
 * @return 0 on success, negative on error
 */
int postprocessing_field_near_to_far(
    const postprocessing_field_data_t* near_field,
    const double* observation_angles,  // [theta1, phi1, theta2, phi2, ...]
    int num_angles,
    postprocessing_field_data_t* far_field
);

/**
 * Calculate radiation pattern
 * 
 * @param far_field Far-field data
 * @param theta_angles Theta angles (degrees)
 * @param phi_angles Phi angles (degrees)
 * @param num_theta Number of theta angles
 * @param num_phi Number of phi angles
 * @param gain_pattern Output: gain pattern (dB)
 * @return 0 on success, negative on error
 */
int postprocessing_field_calculate_radiation_pattern(
    const postprocessing_field_data_t* far_field,
    const double* theta_angles,
    const double* phi_angles,
    int num_theta,
    int num_phi,
    double* gain_pattern
);

/**
 * Calculate EMC/EMI metrics
 * 
 * @param field_data Field data structure
 * @param frequency Frequency (Hz)
 * @param metrics Output: EMC metrics
 * @return 0 on success, negative on error
 */
int postprocessing_field_calculate_emc_metrics(
    const postprocessing_field_data_t* field_data,
    double frequency,
    double* metrics  // [max_E_field, max_H_field, total_power, etc.]
);

#ifdef __cplusplus
}
#endif

#endif // POSTPROCESSING_FIELD_H
