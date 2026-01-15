/******************************************************************************
 * Touchstone Format Export Module
 * 
 * Comprehensive Touchstone file format support for S-parameter export
 * Supports Touchstone v1.0 and v2.0 formats with multiple data formats
 * 
 * Features:
 * - Support for .s2p, .s4p, .s8p, etc. (N-port files)
 * - Multiple data formats: MA, DB, RI
 * - Frequency unit support: Hz, kHz, MHz, GHz
 * - Port impedance specification (per-port or uniform)
 * - Mixed-mode S-parameter export
 * - Comment and metadata support
 ******************************************************************************/

#ifndef TOUCHSTONE_EXPORT_H
#define TOUCHSTONE_EXPORT_H

#include "enhanced_sparameter_extraction.h"
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Touchstone Format Options
 ******************************************************************************/

typedef enum {
    TOUCHSTONE_FORMAT_MA,      // Magnitude-Angle (degrees)
    TOUCHSTONE_FORMAT_DB,      // dB-Angle (degrees)
    TOUCHSTONE_FORMAT_RI       // Real-Imaginary
} touchstone_format_t;

typedef enum {
    TOUCHSTONE_FREQ_HZ,        // Hertz
    TOUCHSTONE_FREQ_KHZ,       // Kilohertz
    TOUCHSTONE_FREQ_MHZ,       // Megahertz
    TOUCHSTONE_FREQ_GHZ        // Gigahertz
} touchstone_freq_unit_t;

typedef struct {
    touchstone_format_t data_format;      // MA, DB, or RI
    touchstone_freq_unit_t freq_unit;     // Hz, kHz, MHz, GHz
    double reference_impedance;           // Reference impedance (Ω)
    double* port_impedances;             // Per-port impedances (NULL for uniform)
    bool include_comments;                // Include comment lines
    bool include_port_names;              // Include port name comments
    char* description;                    // File description
    int precision;                        // Decimal precision (default: 6)
} touchstone_export_options_t;

/******************************************************************************
 * Touchstone Export Functions
 ******************************************************************************/

/**
 * Export S-parameters to Touchstone format file
 * 
 * @param sparams S-parameter data structure
 * @param filename Output filename (should have .sNp extension)
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int touchstone_export_file(
    const SParameterSet* sparams,
    const char* filename,
    const touchstone_export_options_t* options
);

/**
 * Export mixed-mode S-parameters to Touchstone format
 * 
 * @param mixed_mode Mixed-mode S-parameter structure
 * @param filename_base Base filename (extensions will be added)
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int touchstone_export_mixed_mode(
    const MixedModeSParameters* mixed_mode,
    const char* filename_base,
    const touchstone_export_options_t* options
);

/**
 * Get default export options
 * 
 * @return Default options structure
 */
touchstone_export_options_t touchstone_get_default_options(void);

/**
 * Auto-detect filename format from number of ports
 * Generates appropriate .sNp filename
 * 
 * @param base_filename Base filename without extension
 * @param num_ports Number of ports
 * @param output_buffer Output buffer (should be at least 256 chars)
 * @param buffer_size Size of output buffer
 * @return 0 on success, negative on error
 */
int touchstone_generate_filename(
    const char* base_filename,
    int num_ports,
    char* output_buffer,
    size_t buffer_size
);

/**
 * Validate S-parameters before export
 * 
 * @param sparams S-parameter data structure
 * @return true if valid, false otherwise
 */
bool touchstone_validate_sparams(const SParameterSet* sparams);

#ifdef __cplusplus
}
#endif

#endif // TOUCHSTONE_EXPORT_H
