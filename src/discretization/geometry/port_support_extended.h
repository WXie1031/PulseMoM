/******************************************************************************
 * Extended Port Support Module
 * 
 * Comprehensive port type support for electromagnetic simulation
 * 
 * Features:
 * - Multiple port types (voltage, current, S-parameter, transmission line)
 * - Frequency-dependent port impedance
 * - Multi-mode port support
 * - Differential and common-mode ports
 * - Port calibration support
 ******************************************************************************/

#ifndef PORT_SUPPORT_EXTENDED_H
#define PORT_SUPPORT_EXTENDED_H

#include "core_geometry.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Extended Port Types
 ******************************************************************************/

typedef enum {
    PORT_TYPE_VOLTAGE_SOURCE,      // Voltage source port
    PORT_TYPE_CURRENT_SOURCE,      // Current source port
    PORT_TYPE_S_PARAMETER,         // S-parameter port
    PORT_TYPE_IMPEDANCE,           // Impedance port
    PORT_TYPE_MICROSTRIP,          // Microstrip transmission line port
    PORT_TYPE_STRIPLINE,           // Stripline transmission line port
    PORT_TYPE_COAXIAL,             // Coaxial port
    PORT_TYPE_WAVEGUIDE,           // Waveguide port
    PORT_TYPE_COPLANAR_WAVEGUIDE,  // CPW port
    PORT_TYPE_DIFFERENTIAL,        // Differential port pair
    PORT_TYPE_COMMON_MODE,         // Common-mode port
    PORT_TYPE_PLANE_WAVE,          // Plane wave excitation
    PORT_TYPE_DIPOLE,              // Electric/magnetic dipole
    PORT_TYPE_NEAR_FIELD           // Near-field source
} extended_port_type_t;

/******************************************************************************
 * Frequency-Dependent Port Impedance
 ******************************************************************************/

typedef struct {
    double* frequencies;           // Frequency points (Hz)
    double* impedances_real;        // Real part of impedance
    double* impedances_imag;       // Imaginary part of impedance
    int num_points;                // Number of frequency points
    bool interpolate;              // Interpolate between points
} frequency_dependent_impedance_t;

/******************************************************************************
 * Extended Port Definition
 ******************************************************************************/

typedef struct {
    int port_id;                                   // Port identifier
    char port_name[64];                            // Port name
    extended_port_type_t port_type;                // Port type
    
    // Geometry
    geom_point_t position;                         // Port position
    geom_point_t direction;                         // Port direction/normal
    double width;                                  // Port width (for transmission lines)
    double length;                                 // Port length
    
    // Impedance
    double characteristic_impedance;               // Characteristic impedance (Ω)
    frequency_dependent_impedance_t* freq_dep_z;   // Frequency-dependent impedance
    
    // Circuit connection
    int positive_node;                             // Positive circuit node
    int negative_node;                             // Negative circuit node
    char positive_node_name[64];                   // Positive node name
    char negative_node_name[64];                   // Negative node name
    
    // Multi-mode support
    int num_modes;                                 // Number of modes
    double* mode_impedances;                       // Impedance per mode
    
    // Excitation
    double excitation_magnitude;                   // Excitation magnitude
    double excitation_phase;                      // Excitation phase (degrees)
    bool is_active;                                // Active port flag
    
    // Differential port support
    int differential_pair_id;                      // Differential pair ID (-1 if single-ended)
    bool is_positive_leg;                          // True for positive leg of differential pair
    
    // Calibration
    bool needs_calibration;                        // Calibration required flag
    char calibration_type[32];                     // Calibration type (SOLT, TRL, etc.)
    
    // Layer information
    int layer_index;                               // PCB layer index
    char net_name[64];                             // Connected net name
    
} extended_port_t;

/******************************************************************************
 * Port Management Functions
 ******************************************************************************/

/**
 * Create a new port with default settings
 * 
 * @param port_id Port identifier
 * @param port_name Port name
 * @param port_type Port type
 * @return Pointer to created port, NULL on error
 */
extended_port_t* port_create(
    int port_id,
    const char* port_name,
    extended_port_type_t port_type
);

/**
 * Destroy a port and free resources
 * 
 * @param port Port to destroy
 */
void port_destroy(extended_port_t* port);

/**
 * Set frequency-dependent impedance for a port
 * 
 * @param port Port structure
 * @param frequencies Frequency points (Hz)
 * @param impedances_real Real part of impedance
 * @param impedances_imag Imaginary part of impedance
 * @param num_points Number of frequency points
 * @return 0 on success, negative on error
 */
int port_set_frequency_dependent_impedance(
    extended_port_t* port,
    const double* frequencies,
    const double* impedances_real,
    const double* impedances_imag,
    int num_points
);

/**
 * Get port impedance at a specific frequency
 * 
 * @param port Port structure
 * @param frequency Frequency (Hz)
 * @param impedance_real Output: real part of impedance
 * @param impedance_imag Output: imaginary part of impedance
 * @return 0 on success, negative on error
 */
int port_get_impedance_at_frequency(
    const extended_port_t* port,
    double frequency,
    double* impedance_real,
    double* impedance_imag
);

/**
 * Create a differential port pair
 * 
 * @param port_id_base Base port ID
 * @param port_name_base Base port name
 * @param pos_port Output: positive port
 * @param neg_port Output: negative port
 * @return 0 on success, negative on error
 */
int port_create_differential_pair(
    int port_id_base,
    const char* port_name_base,
    extended_port_t** pos_port,
    extended_port_t** neg_port
);

/**
 * Validate port configuration
 * 
 * @param port Port to validate
 * @return true if valid, false otherwise
 */
bool port_validate(const extended_port_t* port);

#ifdef __cplusplus
}
#endif

#endif // PORT_SUPPORT_EXTENDED_H
