/******************************************************************************
 * Extended Output Format Support
 * 
 * Comprehensive output format support for commercial software compatibility
 * 
 * Methods: Multiple format exporters (Touchstone, CITI, Ansoft, CST, HFSS, etc.)
 ******************************************************************************/

#ifndef EXPORT_FORMATS_H
#define EXPORT_FORMATS_H

#include "../../io/analysis/enhanced_sparameter_extraction.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Supported Output Formats
 ******************************************************************************/

typedef enum {
    EXPORT_FORMAT_TOUCHSTONE,    // Touchstone (.s2p, .s4p, etc.)
    EXPORT_FORMAT_CSV,           // CSV format
    EXPORT_FORMAT_HDF5,          // HDF5 format
    EXPORT_FORMAT_VTK,           // VTK format
    EXPORT_FORMAT_MATLAB,        // MATLAB .mat format
    EXPORT_FORMAT_SPICE,         // SPICE netlist
    EXPORT_FORMAT_JSON,          // JSON format
    EXPORT_FORMAT_XML,           // XML format
    EXPORT_FORMAT_CITI,          // CITI format (Agilent/Keysight)
    EXPORT_FORMAT_ANSOFT,        // Ansoft format
    EXPORT_FORMAT_CST,           // CST format
    EXPORT_FORMAT_HFSS           // HFSS format
} export_format_type_t;

/******************************************************************************
 * Export Metadata
 ******************************************************************************/

typedef struct {
    char* simulation_name;        // Simulation name
    char* description;            // Description
    char* author;                 // Author
    char* date;                   // Date
    char* software_version;        // Software version
    char* solver_type;            // Solver type
    double frequency_range[2];    // Frequency range [start, stop]
    int num_ports;               // Number of ports
    char** port_names;           // Port names
    double* port_impedances;     // Port impedances
    void* custom_metadata;       // Custom metadata (key-value pairs)
} export_metadata_t;

/******************************************************************************
 * Export Options
 ******************************************************************************/

typedef struct {
    export_format_type_t format;       // Output format
    bool include_metadata;        // Include metadata
    bool use_compression;         // Use compression
    int compression_level;       // Compression level
    bool preserve_precision;      // Preserve full precision
    export_metadata_t* metadata; // Metadata structure
} export_formats_options_t;

/******************************************************************************
 * Export Functions
 ******************************************************************************/

/**
 * Export S-parameters in specified format
 * 
 * @param sparams S-parameter data
 * @param filename Output filename
 * @param options Export options
 * @return 0 on success, negative on error
 */
int export_formats_sparameters(
    const SParameterSet* sparams,
    const char* filename,
    const export_formats_options_t* options
);

/**
 * Export to multiple formats
 * 
 * @param sparams S-parameter data
 * @param filename_base Base filename
 * @param formats Array of formats to export
 * @param num_formats Number of formats
 * @param options Export options
 * @return 0 on success, negative on error
 */
int export_formats_multiple(
    const SParameterSet* sparams,
    const char* filename_base,
    const export_format_type_t* formats,
    int num_formats,
    const export_formats_options_t* options
);

/**
 * Create export metadata
 * 
 * @param metadata Output: metadata structure
 * @return 0 on success, negative on error
 */
int export_formats_create_metadata(export_metadata_t* metadata);

/**
 * Free export metadata
 * 
 * @param metadata Metadata to free
 */
void export_formats_free_metadata(export_metadata_t* metadata);

/**
 * Get default export options
 * 
 * @return Default options
 */
export_formats_options_t export_formats_get_default_options(void);

/**
 * Get format extension
 * 
 * @param format Output format
 * @return File extension (e.g., ".s2p")
 */
const char* export_formats_get_extension(export_format_type_t format);

#ifdef __cplusplus
}
#endif

#endif // EXPORT_FORMATS_H
