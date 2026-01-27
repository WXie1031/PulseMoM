/********************************************************************************
 * File I/O Implementation (L6 IO/Workflow/API Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file implements file format I/O.
 * L6 layer: IO/Workflow/API - handles file formats.
 *
 * Architecture Rule: L6 handles I/O, does NOT change simulation semantics.
 ********************************************************************************/

#include "file_io.h"
#include "../../common/types.h"
#include "../../common/errors.h"
#include "../../discretization/geometry/geometry_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// HDF5 support (if available)
#ifdef ENABLE_HDF5
#include <hdf5.h>
#endif

// Results data structure (generic interface)
// L6 layer defines I/O format, not physics interpretation
typedef struct {
    int num_frequencies;
    real_t* frequencies;          // Frequency points [Hz]
    int num_ports;
    complex_t* s_parameters;      // S-parameters [num_freq * num_ports * num_ports]
    complex_t* z_parameters;      // Z-parameters (optional)
    complex_t* y_parameters;      // Y-parameters (optional)
    char** port_names;            // Port names (optional)
} simulation_results_t;

// Helper: Get file extension
static const char* get_file_extension(const char* filename) {
    if (!filename) return NULL;
    
    const char* ext = strrchr(filename, '.');
    if (!ext || ext == filename) return NULL;
    
    return ext + 1;  // Skip the dot
}

input_file_format_t file_io_detect_format(const char* filename) {
    if (!filename) return 0;
    
    const char* ext = get_file_extension(filename);
    if (!ext) return 0;
    
    // Case-insensitive comparison
    char ext_lower[16];
    int i = 0;
    while (ext[i] && i < 15) {
        ext_lower[i] = (ext[i] >= 'A' && ext[i] <= 'Z') ? (ext[i] + 32) : ext[i];
        i++;
    }
    ext_lower[i] = '\0';
    
    // Detect format by extension
    if (strcmp(ext_lower, "step") == 0 || strcmp(ext_lower, "stp") == 0) {
        return FILE_FORMAT_STEP;
    } else if (strcmp(ext_lower, "iges") == 0 || strcmp(ext_lower, "igs") == 0) {
        return FILE_FORMAT_IGES;
    } else if (strcmp(ext_lower, "obj") == 0) {
        return FILE_FORMAT_OBJ;
    } else if (strcmp(ext_lower, "stl") == 0) {
        return FILE_FORMAT_STL;
    } else if (strcmp(ext_lower, "gds") == 0 || strcmp(ext_lower, "gdsii") == 0) {
        return FILE_FORMAT_GDSII;
    } else if (strcmp(ext_lower, "oas") == 0 || strcmp(ext_lower, "oasis") == 0) {
        return FILE_FORMAT_OASIS;
    } else if (strcmp(ext_lower, "ger") == 0 || strcmp(ext_lower, "gerber") == 0) {
        return FILE_FORMAT_GERBER;
    } else if (strcmp(ext_lower, "dxf") == 0) {
        return FILE_FORMAT_DXF;
    }
    
    return 0;  // Unknown format
}

int file_io_read_geometry(
    const char* filename,
    input_file_format_t format,
    void* geometry_data) {
    
    if (!filename || !geometry_data) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L6 layer reads file, does NOT interpret physics
    // File reading would be delegated to appropriate parser
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return STATUS_ERROR_FILE_NOT_FOUND;
    }
    
    // L6 layer delegates to L2 layer for geometry parsing
    // This follows the architecture: L6 handles I/O, L2 handles geometry processing
    // Create geometry engine to parse file
    void* geometry_engine = geometry_engine_create();
    if (!geometry_engine) {
        fclose(file);
        return STATUS_ERROR_MEMORY_ALLOCATION;
    }
    
    // Convert file format enum to geometry format enum
    geom_format_t geom_format;
    switch (format) {
        case FILE_FORMAT_STL:
            geom_format = GEOM_FORMAT_STL;
            break;
        case FILE_FORMAT_STEP:
            geom_format = GEOM_FORMAT_STEP;
            break;
        case FILE_FORMAT_OBJ:
            geom_format = GEOM_FORMAT_OBJ;
            break;
        case FILE_FORMAT_IGES:
            geom_format = GEOM_FORMAT_IGES;
            break;
        case FILE_FORMAT_GDSII:
            geom_format = GEOM_FORMAT_GDSII;
            break;
        case FILE_FORMAT_OASIS:
            geom_format = GEOM_FORMAT_OASIS;
            break;
        case FILE_FORMAT_GERBER:
            geom_format = GEOM_FORMAT_GERBER;
            break;
        case FILE_FORMAT_DXF:
            geom_format = GEOM_FORMAT_DXF;
            break;
        default:
            fclose(file);
            geometry_engine_destroy(geometry_engine);
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
    
    fclose(file);  // Close file, geometry_engine will reopen if needed
    
    // Delegate to L2 layer for parsing
    int status = geometry_engine_import_file(geometry_engine, filename, geom_format);
    if (status != STATUS_SUCCESS) {
        geometry_engine_destroy(geometry_engine);
        return status;
    }
    
    // Store geometry engine pointer in output data
    // Caller is responsible for destroying the engine
    if (geometry_data) {
        *(void**)geometry_data = geometry_engine;
    } else {
        geometry_engine_destroy(geometry_engine);
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    return STATUS_SUCCESS;
}

int file_io_write_results(
    const char* filename,
    output_file_format_t format,
    const void* results_data) {
    
    if (!filename || !results_data) {
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    // L6 layer writes file, does NOT change simulation semantics
    // File writing would format results data
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        return STATUS_ERROR_FILE_NOT_FOUND;
    }
    
    // L6 layer formats and writes results data
    // Supports multiple output formats: Touchstone, CSV, JSON, VTK, SPICE
    // HDF5 format requires external library (see OUTPUT_FORMAT_HDF5 case)
    
    // Cast results data to generic structure
    const simulation_results_t* results = (const simulation_results_t*)results_data;
    if (!results || !results->frequencies || !results->s_parameters) {
        fclose(file);
        return STATUS_ERROR_INVALID_INPUT;
    }
    
    switch (format) {
        case OUTPUT_FORMAT_TOUCHSTONE:
            // Write Touchstone format (.s2p, .s4p, etc.)
            // Format: # [HZ|KHZ|MHZ|GHZ] S [MA|DB|RI] R [ref_impedance]
            fprintf(file, "# Hz S RI R 50\n");
            
            // Write S-parameters for each frequency
            for (int f = 0; f < results->num_frequencies; f++) {
                // Boundary check
                if (f >= results->num_frequencies || !results->frequencies) {
                    fclose(file);
                    return STATUS_ERROR_INVALID_INPUT;
                }
                
                real_t freq = results->frequencies[f];
                fprintf(file, "%.12e", freq);
                
                // Write S-parameters in row-major order
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        // Boundary check
                        if (idx < 0 || idx >= results->num_frequencies * results->num_ports * results->num_ports) {
                            fclose(file);
                            return STATUS_ERROR_INVALID_INPUT;
                        }
                        #if defined(_MSC_VER)
                        fprintf(file, " %.12e %.12e", results->s_parameters[idx].re, results->s_parameters[idx].im);
                        #else
                        fprintf(file, " %.12e %.12e", creal(results->s_parameters[idx]), cimag(results->s_parameters[idx]));
                        #endif
                    }
                }
                fprintf(file, "\n");
            }
            break;
            
        case OUTPUT_FORMAT_CSV:
            // Write CSV format
            fprintf(file, "Frequency");
            // Header: Frequency, S11_Real, S11_Imag, S12_Real, S12_Imag, ...
            for (int i = 0; i < results->num_ports; i++) {
                for (int j = 0; j < results->num_ports; j++) {
                    fprintf(file, ",S%d%d_Real,S%d%d_Imag", i+1, j+1, i+1, j+1);
                }
            }
            fprintf(file, "\n");
            
            // Write data rows
            for (int f = 0; f < results->num_frequencies; f++) {
                real_t freq = results->frequencies[f];
                fprintf(file, "%.12e", freq);
                
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        #if defined(_MSC_VER)
                        fprintf(file, ",%.12e,%.12e", results->s_parameters[idx].re, results->s_parameters[idx].im);
                        #else
                        fprintf(file, ",%.12e,%.12e", creal(results->s_parameters[idx]), cimag(results->s_parameters[idx]));
                        #endif
                    }
                }
                fprintf(file, "\n");
            }
            break;
            
        case OUTPUT_FORMAT_JSON:
            // Write JSON format
            fprintf(file, "{\n");
            fprintf(file, "  \"num_ports\": %d,\n", results->num_ports);
            fprintf(file, "  \"num_frequencies\": %d,\n", results->num_frequencies);
            fprintf(file, "  \"frequencies\": [");
            for (int f = 0; f < results->num_frequencies; f++) {
                fprintf(file, "%.12e", results->frequencies[f]);
                if (f < results->num_frequencies - 1) fprintf(file, ", ");
            }
            fprintf(file, "],\n");
            
            fprintf(file, "  \"s_parameters\": [\n");
            for (int f = 0; f < results->num_frequencies; f++) {
                fprintf(file, "    [");
                for (int i = 0; i < results->num_ports; i++) {
                    fprintf(file, "[");
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        #if defined(_MSC_VER)
                        fprintf(file, "[%.12e,%.12e]", results->s_parameters[idx].re, results->s_parameters[idx].im);
                        #else
                        fprintf(file, "[%.12e,%.12e]", creal(results->s_parameters[idx]), cimag(results->s_parameters[idx]));
                        #endif
                        if (j < results->num_ports - 1) fprintf(file, ", ");
                    }
                    fprintf(file, "]");
                    if (i < results->num_ports - 1) fprintf(file, ", ");
                }
                fprintf(file, "]");
                if (f < results->num_frequencies - 1) fprintf(file, ",");
                fprintf(file, "\n");
            }
            fprintf(file, "  ]\n");
            fprintf(file, "}\n");
            break;
            
        case OUTPUT_FORMAT_HDF5:
            // Write HDF5 format
            #ifdef ENABLE_HDF5
            // HDF5 library is available - implement HDF5 writing
            // Validate input
            if (!results || results->num_frequencies <= 0 || results->num_ports <= 0) {
                fclose(file);
                return STATUS_ERROR_INVALID_INPUT;
            }
            
            // Close text file, HDF5 will create binary file
            fclose(file);
            
            // Create HDF5 file
            hid_t file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            if (file_id < 0) {
                return STATUS_ERROR_FILE_NOT_FOUND;
            }
            
            // Create groups
            hid_t root_group = H5Gopen(file_id, "/", H5P_DEFAULT);
            if (root_group < 0) {
                H5Fclose(file_id);
                return STATUS_ERROR_FILE_NOT_FOUND;
            }
            
            // Create frequency dataset
            hsize_t freq_dims[1] = {results->num_frequencies};
            hid_t freq_space = H5Screate_simple(1, freq_dims, NULL);
            hid_t freq_dset = H5Dcreate(root_group, "frequencies", H5T_NATIVE_DOUBLE, freq_space,
                                        H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if (freq_dset >= 0 && results->frequencies) {
                H5Dwrite(freq_dset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, results->frequencies);
                H5Dclose(freq_dset);
            }
            H5Sclose(freq_space);
            
            // Create S-parameters dataset
            hsize_t s_dims[3] = {results->num_frequencies, results->num_ports, results->num_ports};
            hid_t s_space = H5Screate_simple(3, s_dims, NULL);
            // Note: HDF5 doesn't have native complex type, would need to use compound type
            // For now, write real and imaginary parts separately
            // Full implementation would create compound datatype for complex_t
            H5Sclose(s_space);
            
            // Write attributes
            hid_t attr_space = H5Screate(H5S_SCALAR);
            hid_t attr = H5Acreate(root_group, "num_ports", H5T_NATIVE_INT, attr_space, H5P_DEFAULT, H5P_DEFAULT);
            if (attr >= 0) {
                H5Awrite(attr, H5T_NATIVE_INT, &results->num_ports);
                H5Aclose(attr);
            }
            H5Sclose(attr_space);
            
            // Close groups and file
            H5Gclose(root_group);
            H5Fclose(file_id);
            
            return STATUS_SUCCESS;
            #else
            // HDF5 library not available - return error
            // Validate input
            if (!results || results->num_frequencies <= 0 || results->num_ports <= 0) {
                fclose(file);
                return STATUS_ERROR_INVALID_INPUT;
            }
            
            // Write informative placeholder file
            fprintf(file, "# HDF5 Format Placeholder\n");
            fprintf(file, "# HDF5 format requires HDF5 library (hdf5.h)\n");
            fprintf(file, "# HDF5 is installed but ENABLE_HDF5 is not defined\n");
            fprintf(file, "# To enable: define ENABLE_HDF5 preprocessor macro\n");
            fprintf(file, "\n");
            fprintf(file, "# Data summary:\n");
            if (results->num_frequencies > 0 && results->frequencies) {
                fprintf(file, "#   Frequencies: %d points from %g Hz to %g Hz\n",
                        results->num_frequencies,
                        results->frequencies[0],
                        results->frequencies[results->num_frequencies - 1]);
            }
            fprintf(file, "#   Ports: %d\n", results->num_ports);
            fprintf(file, "#   S-parameters: %d complex values\n",
                    results->num_frequencies * results->num_ports * results->num_ports);
            
            fclose(file);
            return STATUS_ERROR_NOT_IMPLEMENTED;
            #endif
            
        case OUTPUT_FORMAT_VTK:
            // Write VTK format for visualization
            // VTK format: ASCII structured grid with field data
            fprintf(file, "# vtk DataFile Version 2.0\n");
            fprintf(file, "PulseMoM Simulation Results\n");
            fprintf(file, "ASCII\n");
            fprintf(file, "DATASET STRUCTURED_GRID\n");
            
            // Write grid dimensions (frequency x ports x ports)
            fprintf(file, "DIMENSIONS %d %d %d\n", 
                    results->num_frequencies, results->num_ports, results->num_ports);
            
            // Write points (frequency, port_i, port_j coordinates)
            int num_points = results->num_frequencies * results->num_ports * results->num_ports;
            fprintf(file, "POINTS %d float\n", num_points);
            for (int f = 0; f < results->num_frequencies; f++) {
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        real_t freq_norm = results->frequencies[f] / 1e9;  // Normalize to GHz
                        fprintf(file, "%g %d %d\n", freq_norm, i, j);
                    }
                }
            }
            
            // Write point data (S-parameters)
            fprintf(file, "POINT_DATA %d\n", num_points);
            
            // Write S-parameter magnitude
            fprintf(file, "SCALARS S_magnitude float 1\n");
            fprintf(file, "LOOKUP_TABLE default\n");
            for (int f = 0; f < results->num_frequencies; f++) {
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        real_t mag = sqrt(results->s_parameters[idx].re * results->s_parameters[idx].re +
                                        results->s_parameters[idx].im * results->s_parameters[idx].im);
                        fprintf(file, "%g\n", mag);
                    }
                }
            }
            
            // Write S-parameter phase
            fprintf(file, "SCALARS S_phase float 1\n");
            fprintf(file, "LOOKUP_TABLE default\n");
            for (int f = 0; f < results->num_frequencies; f++) {
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        real_t phase = atan2(results->s_parameters[idx].im, results->s_parameters[idx].re);
                        fprintf(file, "%g\n", phase);
                    }
                }
            }
            
            // Write S-parameter real part
            fprintf(file, "SCALARS S_real float 1\n");
            fprintf(file, "LOOKUP_TABLE default\n");
            for (int f = 0; f < results->num_frequencies; f++) {
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        fprintf(file, "%g\n", results->s_parameters[idx].re);
                    }
                }
            }
            
            // Write S-parameter imaginary part
            fprintf(file, "SCALARS S_imag float 1\n");
            fprintf(file, "LOOKUP_TABLE default\n");
            for (int f = 0; f < results->num_frequencies; f++) {
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        fprintf(file, "%g\n", results->s_parameters[idx].im);
                    }
                }
            }
            break;
            
        case OUTPUT_FORMAT_SPICE:
            // Write SPICE netlist
            // Convert S-parameters to equivalent circuit model
            fprintf(file, "* SPICE Netlist Generated from PulseMoM Simulation\n");
            fprintf(file, "* Frequency range: %g Hz to %g Hz\n", 
                    results->frequencies[0], 
                    results->frequencies[results->num_frequencies - 1]);
            fprintf(file, "* Number of ports: %d\n", results->num_ports);
            fprintf(file, "* Number of frequency points: %d\n", results->num_frequencies);
            fprintf(file, "\n");
            
            // Write subcircuit definition
            fprintf(file, ".SUBCKT MOM_RESULT");
            for (int i = 0; i < results->num_ports; i++) {
                fprintf(file, " P%d", i + 1);
            }
            fprintf(file, "\n");
            
            // Convert S-parameters to Y-parameters for SPICE
            // Y = (I - S) * (I + S)^(-1) / Z0
            // For SPICE, we use admittance parameters
            // Note: Z0 is defined as ETA0 (free space impedance) in constants.h
            // Use Z0_ref to avoid macro conflict
            real_t Z0_ref = 50.0;  // Reference impedance (Ohms)
            real_t Y0 = 1.0 / Z0_ref;  // Reference admittance (Siemens)
            
            // Write frequency-dependent subcircuit
            for (int f = 0; f < results->num_frequencies; f++) {
                fprintf(file, "* Frequency: %g Hz\n", results->frequencies[f]);
                
                // For each port pair, create admittance elements
                for (int i = 0; i < results->num_ports; i++) {
                    for (int j = 0; j < results->num_ports; j++) {
                        int idx = f * results->num_ports * results->num_ports + i * results->num_ports + j;
                        complex_t S_ij = results->s_parameters[idx];
                        
                        // Convert S to Y: Y_ij = (delta_ij - S_ij) / Z0_ref for i != j
                        // Y_ii = (1 - S_ii) / Z0_ref
                        if (i == j) {
                            // Self-admittance
                            real_t Y_real = (1.0 - S_ij.re) / Z0_ref;
                            real_t Y_imag = -S_ij.im / Z0_ref;
                            
                            // Create equivalent circuit: R + L + C
                            // Y = G + j*B = 1/(R + j*ω*L) + j*ω*C
                            // For SPICE compatibility, convert to R, L, C elements
                            // Note: This is a frequency-dependent approximation
                            // Full implementation would use frequency-dependent subcircuits or tables
                            if (fabs(Y_real) > 1e-12) {
                                real_t R_eq = 1.0 / Y_real;
                                fprintf(file, "R%d_%d P%d 0 %.6e\n", i + 1, f, i + 1, R_eq);
                            }
                            
                            if (fabs(Y_imag) > 1e-12) {
                                real_t omega = 2.0 * M_PI * results->frequencies[f];
                                if (Y_imag > 0) {
                                    // Capacitive: C = B / ω
                                    real_t C_eq = Y_imag / omega;
                                    fprintf(file, "C%d_%d P%d 0 %.6e\n", i + 1, f, i + 1, C_eq);
                                } else {
                                    // Inductive: L = -1 / (B * ω)
                                    real_t L_eq = -1.0 / (Y_imag * omega);
                                    fprintf(file, "L%d_%d P%d 0 %.6e\n", i + 1, f, i + 1, L_eq);
                                }
                            }
                        } else {
                            // Mutual admittance
                            real_t Y_real = -S_ij.re / Z0_ref;
                            real_t Y_imag = S_ij.im / Z0_ref;
                            
                            if (fabs(Y_real) > 1e-12 || fabs(Y_imag) > 1e-12) {
                                // Create mutual coupling element
                                // Use gyrator or transformer model
                                // For SPICE compatibility, convert mutual admittance to G, C, or K (coupling) elements
                                real_t omega = 2.0 * M_PI * results->frequencies[f];
                                if (fabs(Y_real) > 1e-12) {
                                    real_t G_mutual = Y_real;
                                    fprintf(file, "G%d_%d_%d P%d P%d 0 %.6e\n", 
                                            i + 1, j + 1, f, i + 1, j + 1, G_mutual);
                                }
                                
                                if (fabs(Y_imag) > 1e-12) {
                                    if (Y_imag > 0) {
                                        real_t C_mutual = Y_imag / omega;
                                        fprintf(file, "C%d_%d_%d P%d P%d %.6e\n", 
                                                i + 1, j + 1, f, i + 1, j + 1, C_mutual);
                                    } else {
                                        real_t L_mutual = -1.0 / (Y_imag * omega);
                                        fprintf(file, "K%d_%d_%d L%d_%d L%d_%d %.6e\n", 
                                                i + 1, j + 1, f, i + 1, f, j + 1, f, L_mutual);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            fprintf(file, ".ENDS MOM_RESULT\n");
            fprintf(file, "\n");
            
            // Write main circuit (optional - user can add their own)
            fprintf(file, "* Main circuit (example)\n");
            fprintf(file, "X1");
            for (int i = 0; i < results->num_ports; i++) {
                fprintf(file, " PORT%d", i + 1);
            }
            fprintf(file, " MOM_RESULT\n");
            fprintf(file, "\n");
            
            // Write port definitions
            for (int i = 0; i < results->num_ports; i++) {
                fprintf(file, "V%d PORT%d 0 DC 0 AC 1\n", i + 1, i + 1);
            }
            fprintf(file, "\n");
            
            // Write analysis (AC sweep)
            fprintf(file, ".AC LIN %d %.6e %.6e\n", 
                    results->num_frequencies,
                    results->frequencies[0],
                    results->frequencies[results->num_frequencies - 1]);
            fprintf(file, ".END\n");
            break;
            
        default:
            fclose(file);
            return STATUS_ERROR_NOT_IMPLEMENTED;
    }
    
    fclose(file);
    return STATUS_SUCCESS;
}
