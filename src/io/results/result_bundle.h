#ifndef RESULT_BUNDLE_H
#define RESULT_BUNDLE_H

#include "../analysis/enhanced_sparameter_extraction.h"
#include "../postprocessing/field_postprocessing.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../file_formats/export_hdf5.h"
#include "../file_formats/export_vtk.h"
#include "../file_formats/touchstone_io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const SParameterSet* sparams;
    const postprocessing_field_data_t* field;
    const postprocessing_current_distribution_t* currents;
    const mesh_t* mesh;
    const char* name;  // optional metadata/name
} result_bundle_t;

void result_bundle_init(result_bundle_t* bundle);

int result_bundle_export_hdf5(const result_bundle_t* bundle,
                              const char* filename,
                              const export_hdf5_options_t* options);

int result_bundle_export_vtk(const result_bundle_t* bundle,
                             const char* field_filename,
                             const char* current_filename,
                             const export_vtk_options_t* options);

int result_bundle_export_touchstone(const result_bundle_t* bundle,
                                    const char* filename_base,
                                    const touchstone_export_options_t* options);

#ifdef __cplusplus
}
#endif

#endif // RESULT_BUNDLE_H
