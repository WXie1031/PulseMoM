/******************************************************************************
 * VTK Export Module
 * 
 * Comprehensive VTK format export for visualization
 * 
 * Method: VTK file format (ASCII, Binary, XML) for ParaView
 ******************************************************************************/

#ifndef EXPORT_VTK_H
#define EXPORT_VTK_H

#include "../../discretization/mesh/core_mesh.h"
#include "../postprocessing/field_postprocessing.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * VTK Format Types
 ******************************************************************************/

typedef enum {
    EXPORT_VTK_FORMAT_ASCII,           // ASCII format
    EXPORT_VTK_FORMAT_BINARY,          // Binary format
    EXPORT_VTK_FORMAT_XML              // XML format
} export_vtk_format_t;

/******************************************************************************
 * VTK Export Options
 ******************************************************************************/

typedef struct {
    export_vtk_format_t format;                // VTK format type
    bool include_vectors;                // Include vector data
    bool include_scalars;                // Include scalar data
    bool include_tensors;                // Include tensor data
    bool use_compression;                // Use compression (for XML)
    int precision;                      // Floating point precision
} export_vtk_options_t;

/******************************************************************************
 * VTK Export Functions
 ******************************************************************************/

/**
 * Export field data to VTK format
 * 
 * @param field_data Field data structure
 * @param mesh Mesh structure
 * @param filename Output filename
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int export_vtk_field(
    const postprocessing_field_data_t* field_data,
    const mesh_t* mesh,
    const char* filename,
    const export_vtk_options_t* options
);

/**
 * Export current distribution to VTK format
 * 
 * @param current_dist Current distribution
 * @param mesh Mesh structure
 * @param filename Output filename
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int export_vtk_current(
    const postprocessing_current_distribution_t* current_dist,
    const mesh_t* mesh,
    const char* filename,
    const export_vtk_options_t* options
);

/**
 * Export mesh with attributes to VTK format
 * 
 * @param mesh Mesh structure
 * @param attributes Attribute data
 * @param num_attributes Number of attributes
 * @param filename Output filename
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int export_vtk_mesh(
    const mesh_t* mesh,
    const double** attributes,
    int num_attributes,
    const char** attribute_names,
    const char* filename,
    const export_vtk_options_t* options
);

/**
 * Export time-series data to VTK format
 * 
 * @param time_data Time-series data
 * @param mesh Mesh structure
 * @param filename_base Base filename (will append time step)
 * @param options Export options (NULL for defaults)
 * @return 0 on success, negative on error
 */
int export_vtk_timeseries(
    const void* time_data,
    const mesh_t* mesh,
    const char* filename_base,
    const export_vtk_options_t* options
);

/**
 * Get default export options
 * 
 * @return Default options
 */
export_vtk_options_t export_vtk_get_default_options(void);

#ifdef __cplusplus
}
#endif

#endif // EXPORT_VTK_H
