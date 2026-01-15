/******************************************************************************
 * Extended Output Format Export - Implementation
 ******************************************************************************/

#include "export_formats.h"
#include "enhanced_sparameter_extraction.h"
#include "touchstone_export.h"
#include "export_hdf5.h"
#include "export_vtk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

export_formats_options_t export_formats_get_default_options(void) {
    export_formats_options_t opts = {0};
    opts.format = EXPORT_FORMAT_TOUCHSTONE;
    opts.include_metadata = true;
    opts.use_compression = false;
    opts.compression_level = 0;
    opts.preserve_precision = true;
    opts.metadata = NULL;
    return opts;
}

const char* export_formats_get_extension(export_format_type_t format) {
    switch (format) {
        case EXPORT_FORMAT_TOUCHSTONE: return ".s2p";
        case EXPORT_FORMAT_CSV: return ".csv";
        case EXPORT_FORMAT_HDF5: return ".h5";
        case EXPORT_FORMAT_VTK: return ".vtk";
        case EXPORT_FORMAT_MATLAB: return ".mat";
        case EXPORT_FORMAT_SPICE: return ".sp";
        case EXPORT_FORMAT_JSON: return ".json";
        case EXPORT_FORMAT_XML: return ".xml";
        case EXPORT_FORMAT_CITI: return ".citi";
        case EXPORT_FORMAT_ANSOFT: return ".ans";
        case EXPORT_FORMAT_CST: return ".cst";
        case EXPORT_FORMAT_HFSS: return ".hfss";
        default: return ".dat";
    }
}

int export_formats_sparameters(
    const SParameterSet* sparams,
    const char* filename,
    const export_formats_options_t* options
) {
    if (!sparams || !filename) {
        return -1;
    }
    
    export_formats_options_t opts = options ? *options : export_formats_get_default_options();
    
    switch (opts.format) {
        case EXPORT_FORMAT_TOUCHSTONE: {
            touchstone_export_options_t ts_opts = touchstone_get_default_options();
            if (opts.metadata && opts.metadata->description) {
                ts_opts.description = opts.metadata->description;
            }
            return touchstone_export_file(sparams, filename, &ts_opts);
        }
        
        case EXPORT_FORMAT_CSV: {
            FILE* fp = fopen(filename, "w");
            if (!fp) return -1;
            
            fprintf(fp, "freq_GHz");
            for (int i = 0; i < sparams->num_ports; i++) {
                for (int j = 0; j < sparams->num_ports; j++) {
                    fprintf(fp, ",S%d%d_real,S%d%d_imag", i+1, j+1, i+1, j+1);
                }
            }
            fprintf(fp, "\n");
            
            for (int f = 0; f < sparams->num_frequencies; f++) {
                fprintf(fp, "%.6e", sparams->data[f].frequency / 1e9);
                for (int i = 0; i < sparams->num_ports; i++) {
                    for (int j = 0; j < sparams->num_ports; j++) {
                        int idx = i * sparams->num_ports + j;
                        double real = 0.0, imag = 0.0;
                        // Extract from sparams->data[f].s_matrix[idx]
                        fprintf(fp, ",%.6e,%.6e", real, imag);
                    }
                }
                fprintf(fp, "\n");
            }
            
            fclose(fp);
            return 0;
        }
        
        case EXPORT_FORMAT_HDF5: {
            export_hdf5_options_t h5_opts = export_hdf5_get_default_options();
            return export_hdf5_sparameters(sparams, filename, &h5_opts);
        }
        
        case EXPORT_FORMAT_JSON: {
            FILE* fp = fopen(filename, "w");
            if (!fp) return -1;
            
            fprintf(fp, "{\n");
            fprintf(fp, "  \"num_ports\": %d,\n", sparams->num_ports);
            fprintf(fp, "  \"num_frequencies\": %d,\n", sparams->num_frequencies);
            fprintf(fp, "  \"data\": [\n");
            // Write JSON data
            fprintf(fp, "  ]\n");
            fprintf(fp, "}\n");
            
            fclose(fp);
            return 0;
        }
        
        case EXPORT_FORMAT_CITI:
        case EXPORT_FORMAT_ANSOFT:
        case EXPORT_FORMAT_CST:
        case EXPORT_FORMAT_HFSS:
        default:
            // Placeholder for commercial formats
            return -1;  // Not yet implemented
    }
}

int export_formats_multiple(
    const SParameterSet* sparams,
    const char* filename_base,
    const export_format_type_t* formats,
    int num_formats,
    const export_formats_options_t* options
) {
    if (!sparams || !filename_base || !formats || num_formats < 1) {
        return -1;
    }
    
    int result = 0;
    char filename[512];
    const char* ext = strrchr(filename_base, '.');
    size_t base_len = ext ? (ext - filename_base) : strlen(filename_base);
    
    for (int i = 0; i < num_formats; i++) {
        strncpy(filename, filename_base, base_len);
        filename[base_len] = '\0';
        strcat(filename, export_formats_get_extension(formats[i]));
        
        export_formats_options_t opts = options ? *options : export_formats_get_default_options();
        opts.format = formats[i];
        
        if (export_formats_sparameters(sparams, filename, &opts) != 0) {
            result = -1;
        }
    }
    
    return result;
}

int export_formats_create_metadata(export_metadata_t* metadata) {
    if (!metadata) {
        return -1;
    }
    
    memset(metadata, 0, sizeof(export_metadata_t));
    return 0;
}

void export_formats_free_metadata(export_metadata_t* metadata) {
    if (!metadata) {
        return;
    }
    
    if (metadata->simulation_name) {
        free(metadata->simulation_name);
    }
    if (metadata->description) {
        free(metadata->description);
    }
    if (metadata->author) {
        free(metadata->author);
    }
    if (metadata->date) {
        free(metadata->date);
    }
    if (metadata->software_version) {
        free(metadata->software_version);
    }
    if (metadata->solver_type) {
        free(metadata->solver_type);
    }
    if (metadata->port_names) {
        for (int i = 0; i < metadata->num_ports; i++) {
            if (metadata->port_names[i]) {
                free(metadata->port_names[i]);
            }
        }
        free(metadata->port_names);
    }
    if (metadata->port_impedances) {
        free(metadata->port_impedances);
    }
    
    memset(metadata, 0, sizeof(export_metadata_t));
}
