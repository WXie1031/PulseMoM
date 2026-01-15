/******************************************************************************
 * HDF5 Export Module
 * 
 * Provides HDF5 format export for simulation results
 * 
 * Method: HDF5 file format with compression and metadata support
 ******************************************************************************/

#ifndef EXPORT_HDF5_H
#define EXPORT_HDF5_H

#include "enhanced_sparameter_extraction.h"
#include "core_mesh.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * HDF5 Export Options
 ******************************************************************************/

typedef struct {
    bool use_compression;                // Enable compression
    int compression_level;               // Compression level (0-9)
    bool include_metadata;               // Include metadata
    bool include_mesh;                   // Include mesh data
    char* group_name;                    // HDF5 group name
} export_hdf5_options_t;

/******************************************************************************
 * HDF5 Export Functions
 ******************************************************************************/

/**
 * Export S-parameters to HDF5 format
 * 
 * @param sparams S-parameter data
 * @param filename Output filename
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int export_hdf5_sparameters(
    const SParameterSet* sparams,
    const char* filename,
    const export_hdf5_options_t* options
);

/**
 * Export field data to HDF5 format
 * 
 * @param field_data Field data structure
 * @param filename Output filename
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int export_hdf5_field_data(
    const void* field_data,
    const char* filename,
    const export_hdf5_options_t* options
);

/**
 * Export mesh to HDF5 format
 * 
 * @param mesh Mesh structure
 * @param filename Output filename
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int export_hdf5_mesh(
    const mesh_t* mesh,
    const char* filename,
    const export_hdf5_options_t* options
);

/**
 * Get default export options
 * 
 * @return Default options
 */
export_hdf5_options_t export_hdf5_get_default_options(void);

#ifdef __cplusplus
}
#endif

#endif // EXPORT_HDF5_H
