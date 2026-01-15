#include "result_bundle.h"
#include <string.h>

void result_bundle_init(result_bundle_t* bundle) {
    if (!bundle) return;
    memset(bundle, 0, sizeof(result_bundle_t));
}

int result_bundle_export_hdf5(const result_bundle_t* bundle,
                              const char* filename,
                              const export_hdf5_options_t* options) {
    if (!bundle || !filename) return -1;
    export_hdf5_options_t opts = options ? *options : export_hdf5_get_default_options();
    int status = 0;
    if (bundle->sparams) {
        status = export_hdf5_sparameters(bundle->sparams, filename, &opts);
        if (status != 0) return status;
    }
    if (bundle->field) {
        status = export_hdf5_field_data(bundle->field, filename, &opts);
        if (status != 0) return status;
    }
    if (bundle->mesh && opts.include_mesh) {
        status = export_hdf5_mesh(bundle->mesh, filename, &opts);
        if (status != 0) return status;
    }
    return status;
}

int result_bundle_export_vtk(const result_bundle_t* bundle,
                             const char* field_filename,
                             const char* current_filename,
                             const export_vtk_options_t* options) {
    if (!bundle || !bundle->mesh) return -1;
    export_vtk_options_t opts = options ? *options : export_vtk_get_default_options();
    int status = 0;
    if (bundle->field && field_filename) {
        status = export_vtk_field(bundle->field, bundle->mesh, field_filename, &opts);
        if (status != 0) return status;
    }
    if (bundle->currents && current_filename) {
        status = export_vtk_current(bundle->currents, bundle->mesh, current_filename, &opts);
        if (status != 0) return status;
    }
    return status;
}

int result_bundle_export_touchstone(const result_bundle_t* bundle,
                                    const char* filename_base,
                                    const touchstone_export_options_t* options) {
    if (!bundle || !bundle->sparams || !filename_base) return -1;
    touchstone_export_options_t opts = options ? *options : touchstone_get_default_options();
    char fname[256] = {0};
    if (touchstone_generate_filename(filename_base, bundle->sparams->num_ports, fname, sizeof(fname)) == 0) {
        return touchstone_export_file(bundle->sparams, fname, &opts);
    }
    return touchstone_export_file(bundle->sparams, filename_base, &opts);
}
