/******************************************************************************
 * Extended Port Support Implementation
 ******************************************************************************/

#include "port_support_extended.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/******************************************************************************
 * Port Creation and Destruction
 ******************************************************************************/

extended_port_t* port_create(
    int port_id,
    const char* port_name,
    extended_port_type_t port_type
) {
    if (!port_name) {
        return NULL;
    }
    
    extended_port_t* port = (extended_port_t*)calloc(1, sizeof(extended_port_t));
    if (!port) {
        return NULL;
    }
    
    port->port_id = port_id;
    strncpy(port->port_name, port_name, sizeof(port->port_name) - 1);
    port->port_type = port_type;
    port->characteristic_impedance = 50.0;  // Default
    port->excitation_magnitude = 1.0;
    port->excitation_phase = 0.0;
    port->is_active = true;
    port->differential_pair_id = -1;
    port->is_positive_leg = false;
    port->needs_calibration = false;
    port->layer_index = -1;
    port->num_modes = 1;
    
    return port;
}

void port_destroy(extended_port_t* port) {
    if (!port) {
        return;
    }
    
    if (port->freq_dep_z) {
        if (port->freq_dep_z->frequencies) {
            free(port->freq_dep_z->frequencies);
        }
        if (port->freq_dep_z->impedances_real) {
            free(port->freq_dep_z->impedances_real);
        }
        if (port->freq_dep_z->impedances_imag) {
            free(port->freq_dep_z->impedances_imag);
        }
        free(port->freq_dep_z);
    }
    
    if (port->mode_impedances) {
        free(port->mode_impedances);
    }
    
    free(port);
}

/******************************************************************************
 * Frequency-Dependent Impedance
 ******************************************************************************/

int port_set_frequency_dependent_impedance(
    extended_port_t* port,
    const double* frequencies,
    const double* impedances_real,
    const double* impedances_imag,
    int num_points
) {
    if (!port || !frequencies || !impedances_real || !impedances_imag || num_points < 1) {
        return -1;
    }
    
    // Allocate frequency-dependent impedance structure
    if (!port->freq_dep_z) {
        port->freq_dep_z = (frequency_dependent_impedance_t*)calloc(1, sizeof(frequency_dependent_impedance_t));
        if (!port->freq_dep_z) {
            return -1;
        }
    }
    
    // Free existing data if any
    if (port->freq_dep_z->frequencies) {
        free(port->freq_dep_z->frequencies);
    }
    if (port->freq_dep_z->impedances_real) {
        free(port->freq_dep_z->impedances_real);
    }
    if (port->freq_dep_z->impedances_imag) {
        free(port->freq_dep_z->impedances_imag);
    }
    
    // Allocate new arrays
    port->freq_dep_z->frequencies = (double*)malloc(num_points * sizeof(double));
    port->freq_dep_z->impedances_real = (double*)malloc(num_points * sizeof(double));
    port->freq_dep_z->impedances_imag = (double*)malloc(num_points * sizeof(double));
    
    if (!port->freq_dep_z->frequencies || !port->freq_dep_z->impedances_real || !port->freq_dep_z->impedances_imag) {
        // Cleanup on error
        if (port->freq_dep_z->frequencies) free(port->freq_dep_z->frequencies);
        if (port->freq_dep_z->impedances_real) free(port->freq_dep_z->impedances_real);
        if (port->freq_dep_z->impedances_imag) free(port->freq_dep_z->impedances_imag);
        return -1;
    }
    
    // Copy data
    memcpy(port->freq_dep_z->frequencies, frequencies, num_points * sizeof(double));
    memcpy(port->freq_dep_z->impedances_real, impedances_real, num_points * sizeof(double));
    memcpy(port->freq_dep_z->impedances_imag, impedances_imag, num_points * sizeof(double));
    port->freq_dep_z->num_points = num_points;
    port->freq_dep_z->interpolate = true;
    
    return 0;
}

int port_get_impedance_at_frequency(
    const extended_port_t* port,
    double frequency,
    double* impedance_real,
    double* impedance_imag
) {
    if (!port || !impedance_real || !impedance_imag) {
        return -1;
    }
    
    // If frequency-dependent impedance is not set, use constant impedance
    if (!port->freq_dep_z || port->freq_dep_z->num_points == 0) {
        *impedance_real = port->characteristic_impedance;
        *impedance_imag = 0.0;
        return 0;
    }
    
    // Linear interpolation
    const double* freqs = port->freq_dep_z->frequencies;
    const double* z_real = port->freq_dep_z->impedances_real;
    const double* z_imag = port->freq_dep_z->impedances_imag;
    int n = port->freq_dep_z->num_points;
    
    // Check if frequency is outside range
    if (frequency <= freqs[0]) {
        *impedance_real = z_real[0];
        *impedance_imag = z_imag[0];
        return 0;
    }
    if (frequency >= freqs[n-1]) {
        *impedance_real = z_real[n-1];
        *impedance_imag = z_imag[n-1];
        return 0;
    }
    
    // Find interpolation interval
    for (int i = 0; i < n - 1; i++) {
        if (frequency >= freqs[i] && frequency <= freqs[i+1]) {
            double t = (frequency - freqs[i]) / (freqs[i+1] - freqs[i]);
            *impedance_real = z_real[i] + t * (z_real[i+1] - z_real[i]);
            *impedance_imag = z_imag[i] + t * (z_imag[i+1] - z_imag[i]);
            return 0;
        }
    }
    
    // Should not reach here
    *impedance_real = port->characteristic_impedance;
    *impedance_imag = 0.0;
    return 0;
}

/******************************************************************************
 * Differential Port Pair
 ******************************************************************************/

int port_create_differential_pair(
    int port_id_base,
    const char* port_name_base,
    extended_port_t** pos_port,
    extended_port_t** neg_port
) {
    if (!port_name_base || !pos_port || !neg_port) {
        return -1;
    }
    
    char pos_name[64], neg_name[64];
    snprintf(pos_name, sizeof(pos_name), "%s_P", port_name_base);
    snprintf(neg_name, sizeof(neg_name), "%s_N", port_name_base);
    
    *pos_port = port_create(port_id_base, pos_name, PORT_TYPE_DIFFERENTIAL);
    *neg_port = port_create(port_id_base + 1, neg_name, PORT_TYPE_DIFFERENTIAL);
    
    if (!*pos_port || !*neg_port) {
        if (*pos_port) port_destroy(*pos_port);
        if (*neg_port) port_destroy(*neg_port);
        *pos_port = NULL;
        *neg_port = NULL;
        return -1;
    }
    
    // Set differential pair properties
    (*pos_port)->differential_pair_id = port_id_base;
    (*pos_port)->is_positive_leg = true;
    (*neg_port)->differential_pair_id = port_id_base;
    (*neg_port)->is_positive_leg = false;
    
    return 0;
}

/******************************************************************************
 * Port Validation
 ******************************************************************************/

bool port_validate(const extended_port_t* port) {
    if (!port) {
        return false;
    }
    
    // Check port name
    if (strlen(port->port_name) == 0) {
        return false;
    }
    
    // Check impedance
    if (port->characteristic_impedance <= 0.0) {
        return false;
    }
    
    // Check position (should be valid)
    // Position validation can be added here
    
    // Check frequency-dependent impedance if set
    if (port->freq_dep_z) {
        if (port->freq_dep_z->num_points < 1) {
            return false;
        }
        if (!port->freq_dep_z->frequencies || !port->freq_dep_z->impedances_real || !port->freq_dep_z->impedances_imag) {
            return false;
        }
    }
    
    return true;
}
