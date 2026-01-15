/******************************************************************************
 * Plane Wave Excitation Module
 * 
 * Implements plane wave excitation for electromagnetic simulation
 * 
 * Method: Plane wave source with arbitrary incidence and polarization
 ******************************************************************************/

#ifndef EXCITATION_PLANE_WAVE_H
#define EXCITATION_PLANE_WAVE_H

#include "core_geometry.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Polarization Types
 ******************************************************************************/

typedef enum {
    EXCITATION_PLANE_WAVE_TE,              // Transverse Electric (perpendicular)
    EXCITATION_PLANE_WAVE_TM,              // Transverse Magnetic (parallel)
    EXCITATION_PLANE_WAVE_CIRCULAR_LH,     // Left-hand circular
    EXCITATION_PLANE_WAVE_CIRCULAR_RH,     // Right-hand circular
    EXCITATION_PLANE_WAVE_ELLIPTICAL       // Elliptical polarization
} excitation_plane_wave_polarization_t;

/******************************************************************************
 * Plane Wave Excitation Structure
 ******************************************************************************/

typedef struct {
    double frequency;                   // Frequency (Hz)
    double amplitude;                   // Electric field amplitude (V/m)
    
    // Incident direction (spherical coordinates)
    double theta;                       // Elevation angle (degrees, 0-180)
    double phi;                         // Azimuth angle (degrees, 0-360)
    
    // Polarization
    excitation_plane_wave_polarization_t polarization; // Polarization type
    double polarization_angle;         // Polarization angle for elliptical (degrees)
    double axial_ratio;                 // Axial ratio for elliptical
    
    // Phase
    double phase;                       // Phase offset (degrees)
    
    // Wave vector
    double kx, ky, kz;                 // Wave vector components
    
    // Reference point
    double x0, y0, z0;                 // Reference point (m)
    
} excitation_plane_wave_t;

/******************************************************************************
 * Plane Wave Functions
 ******************************************************************************/

/**
 * Create plane wave excitation
 * 
 * @param frequency Frequency (Hz)
 * @param amplitude Electric field amplitude (V/m)
 * @param theta Elevation angle (degrees)
 * @param phi Azimuth angle (degrees)
 * @param polarization Polarization type
 * @return Plane wave excitation structure
 */
excitation_plane_wave_t excitation_plane_wave_create(
    double frequency,
    double amplitude,
    double theta,
    double phi,
    excitation_plane_wave_polarization_t polarization
);

/**
 * Compute incident electric field at observation point
 * 
 * @param excitation Plane wave excitation
 * @param x Observation point x (m)
 * @param y Observation point y (m)
 * @param z Observation point z (m)
 * @param E Output: electric field vector [Ex, Ey, Ez]
 * @return 0 on success, negative on error
 */
int excitation_plane_wave_compute_electric_field(
    const excitation_plane_wave_t* excitation,
    double x, double y, double z,
    double E[3]
);

/**
 * Compute incident magnetic field at observation point
 * 
 * @param excitation Plane wave excitation
 * @param x Observation point x (m)
 * @param y Observation point y (m)
 * @param z Observation point z (m)
 * @param H Output: magnetic field vector [Hx, Hy, Hz]
 * @return 0 on success, negative on error
 */
int excitation_plane_wave_compute_magnetic_field(
    const excitation_plane_wave_t* excitation,
    double x, double y, double z,
    double H[3]
);

/**
 * Convert plane wave to MoM excitation vector
 * 
 * @param excitation Plane wave excitation
 * @param mesh Mesh structure
 * @param excitation_vector Output: excitation vector
 * @return 0 on success, negative on error
 */
int excitation_plane_wave_to_mom(
    const excitation_plane_wave_t* excitation,
    const void* mesh,
    void* excitation_vector
);

/**
 * Set polarization parameters
 * 
 * @param excitation Plane wave excitation
 * @param polarization Polarization type
 * @param polarization_angle Polarization angle (degrees)
 * @param axial_ratio Axial ratio (for elliptical)
 * @return 0 on success, negative on error
 */
int excitation_plane_wave_set_polarization(
    excitation_plane_wave_t* excitation,
    excitation_plane_wave_polarization_t polarization,
    double polarization_angle,
    double axial_ratio
);

#ifdef __cplusplus
}
#endif

#endif // EXCITATION_PLANE_WAVE_H
