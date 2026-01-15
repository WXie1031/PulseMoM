/******************************************************************************
 * HDF5 Export - Implementation
 * 
 * Note: Requires HDF5 library. This is a stub implementation.
 ******************************************************************************/

#include "export_hdf5.h"
#include "enhanced_sparameter_extraction.h"
#include "core_mesh.h"
#include "core_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// HDF5 library support (conditional compilation)
#ifdef HAVE_HDF5
#include "hdf5.h"
#define HDF5_ENABLED 1
#else
#define HDF5_ENABLED 0
#endif

export_hdf5_options_t export_hdf5_get_default_options(void) {
    export_hdf5_options_t opts = {0};
    opts.use_compression = false;
    opts.compression_level = 0;
    opts.include_metadata = true;
    opts.include_mesh = false;
    opts.group_name = NULL;
    return opts;
}

int export_hdf5_sparameters(
    const SParameterSet* sparams,
    const char* filename,
    const export_hdf5_options_t* options
) {
    if (!sparams || !filename) {
        return -1;
    }
    
    export_hdf5_options_t opts = options ? *options : export_hdf5_get_default_options();
    
#if HDF5_ENABLED
    // HDF5 implementation
    (void)opts;  // Used in HDF5 code below
    hid_t file_id, group_id, dataset_id, dataspace_id;
    herr_t status;
    hsize_t dims[3];
    
    // Create HDF5 file
    file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        return -1;
    }
    
    // Create group for S-parameters
    const char* group_name = opts.group_name ? opts.group_name : "/S_parameters";
    group_id = H5Gcreate2(file_id, group_name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (group_id < 0) {
        H5Fclose(file_id);
        return -1;
    }
    
    // Write frequencies (extract from SParameterData array)
    if (sparams->data && sparams->num_frequencies > 0) {
        double* frequencies = (double*)calloc(sparams->num_frequencies, sizeof(double));
        if (frequencies) {
            for (int f = 0; f < sparams->num_frequencies; f++) {
                frequencies[f] = sparams->data[f].frequency;
            }
            
            dims[0] = sparams->num_frequencies;
            dataspace_id = H5Screate_simple(1, dims, NULL);
            dataset_id = H5Dcreate2(group_id, "frequencies", H5T_NATIVE_DOUBLE, dataspace_id,
                                   H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if (dataset_id >= 0) {
                H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, frequencies);
                H5Dclose(dataset_id);
            }
            H5Sclose(dataspace_id);
            free(frequencies);
        }
    }
    
    // Write S-parameter matrix [num_freq, num_ports, num_ports, 2] (real, imag)
    dims[0] = sparams->num_frequencies;
    dims[1] = sparams->num_ports;
    dims[2] = sparams->num_ports * 2;  // Real and imaginary parts
    dataspace_id = H5Screate_simple(3, dims, NULL);
    
    // Create dataset with compression if requested
    hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);
    if (opts.use_compression && opts.compression_level > 0) {
        H5Pset_deflate(plist_id, opts.compression_level);
    }
    
    dataset_id = H5Dcreate2(group_id, "S_matrix", H5T_NATIVE_DOUBLE, dataspace_id,
                           H5P_DEFAULT, plist_id, H5P_DEFAULT);
    
    if (dataset_id >= 0 && sparams->data) {
        // Convert complex S-parameters to interleaved real/imag format
        double* s_data = (double*)calloc(sparams->num_frequencies * sparams->num_ports * sparams->num_ports * 2, sizeof(double));
        if (s_data) {
            int idx = 0;
            for (int f = 0; f < sparams->num_frequencies; f++) {
                if (sparams->data[f].s_matrix) {
                    for (int i = 0; i < sparams->num_ports; i++) {
                        for (int j = 0; j < sparams->num_ports; j++) {
                            // SParameterData uses column-major storage
                            int s_idx = j * sparams->num_ports + i;
                            s_data[idx++] = sparams->data[f].s_matrix[s_idx].re;
                            s_data[idx++] = sparams->data[f].s_matrix[s_idx].im;
                        }
                    }
                } else {
                    // Zero fill if s_matrix is NULL
                    for (int i = 0; i < sparams->num_ports * sparams->num_ports; i++) {
                        s_data[idx++] = 0.0;
                        s_data[idx++] = 0.0;
                    }
                }
            }
            H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, s_data);
            free(s_data);
        }
        H5Dclose(dataset_id);
    }
    H5Sclose(dataspace_id);
    H5Pclose(plist_id);
    
    // Write metadata as attributes
    if (opts.include_metadata) {
        int num_ports = sparams->num_ports;
        int num_freq = sparams->num_frequencies;
        // Create attribute dataspace
        hid_t attr_space = H5Screate(H5S_SCALAR);
        hid_t attr_id;
        
        // Write num_ports attribute
        attr_id = H5Acreate2(group_id, "num_ports", H5T_NATIVE_INT, attr_space,
                            H5P_DEFAULT, H5P_DEFAULT);
        if (attr_id >= 0) {
            H5Awrite(attr_id, H5T_NATIVE_INT, &num_ports);
            H5Aclose(attr_id);
        }
        
        // Write num_frequencies attribute
        attr_id = H5Acreate2(group_id, "num_frequencies", H5T_NATIVE_INT, attr_space,
                            H5P_DEFAULT, H5P_DEFAULT);
        if (attr_id >= 0) {
            H5Awrite(attr_id, H5T_NATIVE_INT, &num_freq);
            H5Aclose(attr_id);
        }
        
        H5Sclose(attr_space);
    }
    
    H5Gclose(group_id);
    H5Fclose(file_id);
    
    return 0;
#else
    // Fallback: Write to text file when HDF5 is not available
    (void)opts;  // Suppress unused variable warning
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }
    
    fprintf(fp, "# HDF5 Export (HDF5 library not available - using text fallback)\n");
    fprintf(fp, "# S-parameters: %d ports, %d frequencies\n",
            sparams->num_ports, sparams->num_frequencies);
    
    if (sparams->data) {
        fprintf(fp, "# Format: frequency, S11_real, S11_imag, S12_real, S12_imag, ...\n");
        for (int f = 0; f < sparams->num_frequencies; f++) {
            fprintf(fp, "%.12e", sparams->data[f].frequency);
            if (sparams->data[f].s_matrix) {
                for (int i = 0; i < sparams->num_ports; i++) {
                    for (int j = 0; j < sparams->num_ports; j++) {
                        // Column-major storage
                        int idx = j * sparams->num_ports + i;
                        fprintf(fp, " %.12e %.12e", 
                               sparams->data[f].s_matrix[idx].re,
                               sparams->data[f].s_matrix[idx].im);
                    }
                }
            }
            fprintf(fp, "\n");
        }
    }
    
    fclose(fp);
    return 0;
#endif
}

int export_hdf5_field_data(
    const void* field_data,
    const char* filename,
    const export_hdf5_options_t* options
) {
    if (!field_data || !filename) {
        return -1;
    }
    
    // Assume field_data is postprocessing_field_data_t*
    typedef struct {
        double* e_field_real;
        double* e_field_imag;
        double* h_field_real;
        double* h_field_imag;
        double* positions;
        int num_points;
        double frequency;
    } postprocessing_field_data_t;
    
    postprocessing_field_data_t* field = (postprocessing_field_data_t*)field_data;
    export_hdf5_options_t opts = options ? *options : export_hdf5_get_default_options();
    
#if HDF5_ENABLED
    (void)opts;  // Used in HDF5 code below
    hid_t file_id, group_id, dataset_id, dataspace_id;
    herr_t status;
    hsize_t dims[2];
    
    // Create HDF5 file
    file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        return -1;
    }
    
    // Create group
    const char* group_name = opts.group_name ? opts.group_name : "/field_data";
    group_id = H5Gcreate2(file_id, group_name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (group_id < 0) {
        H5Fclose(file_id);
        return -1;
    }
    
    // Write positions [num_points, 3]
    if (field->positions) {
        dims[0] = field->num_points;
        dims[1] = 3;
        dataspace_id = H5Screate_simple(2, dims, NULL);
        dataset_id = H5Dcreate2(group_id, "positions", H5T_NATIVE_DOUBLE, dataspace_id,
                               H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dataset_id >= 0) {
            H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, field->positions);
            H5Dclose(dataset_id);
        }
        H5Sclose(dataspace_id);
    }
    
    // Write E field [num_points, 3, 2] (real, imag)
    if (field->e_field_real || field->e_field_imag) {
        dims[0] = field->num_points;
        dims[1] = 3 * 2;  // 3 components, real and imag
        dataspace_id = H5Screate_simple(2, dims, NULL);
        
        hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);
        if (opts.use_compression) {
            H5Pset_deflate(plist_id, opts.compression_level);
        }
        
        dataset_id = H5Dcreate2(group_id, "E_field", H5T_NATIVE_DOUBLE, dataspace_id,
                               H5P_DEFAULT, plist_id, H5P_DEFAULT);
        if (dataset_id >= 0) {
            double* e_data = (double*)calloc(field->num_points * 3 * 2, sizeof(double));
            if (e_data) {
                for (int i = 0; i < field->num_points * 3; i++) {
                    e_data[i * 2] = field->e_field_real ? field->e_field_real[i] : 0.0;
                    e_data[i * 2 + 1] = field->e_field_imag ? field->e_field_imag[i] : 0.0;
                }
                H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, e_data);
                free(e_data);
            }
            H5Dclose(dataset_id);
        }
        H5Sclose(dataspace_id);
        H5Pclose(plist_id);
    }
    
    // Write H field similarly
    if (field->h_field_real || field->h_field_imag) {
        dims[0] = field->num_points;
        dims[1] = 3 * 2;
        dataspace_id = H5Screate_simple(2, dims, NULL);
        
        hid_t plist_id = H5Pcreate(H5P_DATASET_CREATE);
        if (opts.use_compression) {
            H5Pset_deflate(plist_id, opts.compression_level);
        }
        
        dataset_id = H5Dcreate2(group_id, "H_field", H5T_NATIVE_DOUBLE, dataspace_id,
                               H5P_DEFAULT, plist_id, H5P_DEFAULT);
        if (dataset_id >= 0) {
            double* h_data = (double*)calloc(field->num_points * 3 * 2, sizeof(double));
            if (h_data) {
                for (int i = 0; i < field->num_points * 3; i++) {
                    h_data[i * 2] = field->h_field_real ? field->h_field_real[i] : 0.0;
                    h_data[i * 2 + 1] = field->h_field_imag ? field->h_field_imag[i] : 0.0;
                }
                H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, h_data);
                free(h_data);
            }
            H5Dclose(dataset_id);
        }
        H5Sclose(dataspace_id);
        H5Pclose(plist_id);
    }
    
    // Write metadata
    if (opts.include_metadata) {
        int num_points = field->num_points;
        double freq = field->frequency;
        hid_t attr_space = H5Screate(H5S_SCALAR);
        hid_t attr_id;
        
        attr_id = H5Acreate2(group_id, "num_points", H5T_NATIVE_INT, attr_space,
                            H5P_DEFAULT, H5P_DEFAULT);
        if (attr_id >= 0) {
            H5Awrite(attr_id, H5T_NATIVE_INT, &num_points);
            H5Aclose(attr_id);
        }
        
        attr_id = H5Acreate2(group_id, "frequency", H5T_NATIVE_DOUBLE, attr_space,
                            H5P_DEFAULT, H5P_DEFAULT);
        if (attr_id >= 0) {
            H5Awrite(attr_id, H5T_NATIVE_DOUBLE, &freq);
            H5Aclose(attr_id);
        }
        
        H5Sclose(attr_space);
    }
    
    H5Gclose(group_id);
    H5Fclose(file_id);
    
    return 0;
#else
    // Fallback to text file
    (void)opts;  // Suppress unused variable warning
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }
    
    fprintf(fp, "# HDF5 Export (HDF5 library not available - using text fallback)\n");
    fprintf(fp, "# Field data: %d points, frequency: %.6e Hz\n",
            field->num_points, field->frequency);
    fprintf(fp, "# Format: x, y, z, Ex_real, Ex_imag, Ey_real, Ey_imag, Ez_real, Ez_imag\n");
    
    if (field->positions) {
        for (int i = 0; i < field->num_points; i++) {
            fprintf(fp, "%.12e %.12e %.12e",
                   field->positions[i * 3], field->positions[i * 3 + 1], field->positions[i * 3 + 2]);
            if (field->e_field_real) {
                for (int j = 0; j < 3; j++) {
                    fprintf(fp, " %.12e %.12e",
                           field->e_field_real[i * 3 + j],
                           field->e_field_imag ? field->e_field_imag[i * 3 + j] : 0.0);
                }
            }
            fprintf(fp, "\n");
        }
    }
    
    fclose(fp);
    return 0;
#endif
}

int export_hdf5_mesh(
    const mesh_t* mesh,
    const char* filename,
    const export_hdf5_options_t* options
) {
    if (!mesh || !filename) {
        return -1;
    }
    
    export_hdf5_options_t opts = options ? *options : export_hdf5_get_default_options();
    
#if HDF5_ENABLED
    (void)opts;  // Used in HDF5 code below
    hid_t file_id, group_id, dataset_id, dataspace_id;
    herr_t status;
    hsize_t dims[2];
    
    // Create HDF5 file
    file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if (file_id < 0) {
        return -1;
    }
    
    // Create group
    const char* group_name = opts.group_name ? opts.group_name : "/mesh";
    group_id = H5Gcreate2(file_id, group_name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    if (group_id < 0) {
        H5Fclose(file_id);
        return -1;
    }
    
    // Write vertices [num_vertices, 3]
    if (mesh->vertices) {
        dims[0] = mesh->num_vertices;
        dims[1] = 3;
        dataspace_id = H5Screate_simple(2, dims, NULL);
        dataset_id = H5Dcreate2(group_id, "vertices", H5T_NATIVE_DOUBLE, dataspace_id,
                               H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dataset_id >= 0) {
            double* vertices = (double*)calloc(mesh->num_vertices * 3, sizeof(double));
            if (vertices) {
                for (int i = 0; i < mesh->num_vertices; i++) {
                    vertices[i * 3] = mesh->vertices[i].position.x;
                    vertices[i * 3 + 1] = mesh->vertices[i].position.y;
                    vertices[i * 3 + 2] = mesh->vertices[i].position.z;
                }
                H5Dwrite(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, vertices);
                free(vertices);
            }
            H5Dclose(dataset_id);
        }
        H5Sclose(dataspace_id);
    }
    
    // Write elements (simplified - store as connectivity list)
    if (mesh->elements) {
        // Count total vertices in all elements
        int total_vertices = 0;
        for (int i = 0; i < mesh->num_elements; i++) {
            total_vertices += mesh->elements[i].num_vertices;
        }
        
        // Store element connectivity
        dims[0] = mesh->num_elements;
        dims[1] = 1;  // Will store num_vertices per element
        dataspace_id = H5Screate_simple(2, dims, NULL);
        dataset_id = H5Dcreate2(group_id, "element_num_vertices", H5T_NATIVE_INT, dataspace_id,
                               H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dataset_id >= 0) {
            int* num_verts = (int*)calloc(mesh->num_elements, sizeof(int));
            if (num_verts) {
                for (int i = 0; i < mesh->num_elements; i++) {
                    num_verts[i] = mesh->elements[i].num_vertices;
                }
                H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, num_verts);
                free(num_verts);
            }
            H5Dclose(dataset_id);
        }
        H5Sclose(dataspace_id);
        
        // Store connectivity
        dims[0] = total_vertices;
        dataspace_id = H5Screate_simple(1, dims, NULL);
        dataset_id = H5Dcreate2(group_id, "element_connectivity", H5T_NATIVE_INT, dataspace_id,
                               H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        if (dataset_id >= 0) {
            int* connectivity = (int*)calloc(total_vertices, sizeof(int));
            if (connectivity) {
                int idx = 0;
                for (int i = 0; i < mesh->num_elements; i++) {
                    for (int v = 0; v < mesh->elements[i].num_vertices; v++) {
                        connectivity[idx++] = mesh->elements[i].vertices[v];
                    }
                }
                H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, connectivity);
                free(connectivity);
            }
            H5Dclose(dataset_id);
        }
        H5Sclose(dataspace_id);
    }
    
    // Write metadata
    if (opts.include_metadata) {
        int num_vertices = mesh->num_vertices;
        int num_elements = mesh->num_elements;
        hid_t attr_space = H5Screate(H5S_SCALAR);
        hid_t attr_id;
        
        attr_id = H5Acreate2(group_id, "num_vertices", H5T_NATIVE_INT, attr_space,
                            H5P_DEFAULT, H5P_DEFAULT);
        if (attr_id >= 0) {
            H5Awrite(attr_id, H5T_NATIVE_INT, &num_vertices);
            H5Aclose(attr_id);
        }
        
        attr_id = H5Acreate2(group_id, "num_elements", H5T_NATIVE_INT, attr_space,
                            H5P_DEFAULT, H5P_DEFAULT);
        if (attr_id >= 0) {
            H5Awrite(attr_id, H5T_NATIVE_INT, &num_elements);
            H5Aclose(attr_id);
        }
        
        H5Sclose(attr_space);
    }
    
    H5Gclose(group_id);
    H5Fclose(file_id);
    
    return 0;
#else
    // Fallback to text file
    (void)opts;  // Suppress unused variable warning
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        return -1;
    }
    
    fprintf(fp, "# HDF5 Export (HDF5 library not available - using text fallback)\n");
    fprintf(fp, "# Mesh: %d vertices, %d elements\n", mesh->num_vertices, mesh->num_elements);
    
    // Write vertices
    fprintf(fp, "# Vertices:\n");
    if (mesh->vertices) {
        for (int i = 0; i < mesh->num_vertices; i++) {
            fprintf(fp, "%.12e %.12e %.12e\n",
                   mesh->vertices[i].position.x,
                   mesh->vertices[i].position.y,
                   mesh->vertices[i].position.z);
        }
    }
    
    // Write elements
    fprintf(fp, "# Elements:\n");
    if (mesh->elements) {
        for (int i = 0; i < mesh->num_elements; i++) {
            fprintf(fp, "%d", mesh->elements[i].num_vertices);
            for (int v = 0; v < mesh->elements[i].num_vertices; v++) {
                fprintf(fp, " %d", mesh->elements[i].vertices[v]);
            }
            fprintf(fp, "\n");
        }
    }
    
    fclose(fp);
    return 0;
#endif
}
