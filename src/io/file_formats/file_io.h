/********************************************************************************
 * File I/O (IO/Workflow/API Layer)
 *
 * Copyright (C) 2025 PulseEM Technologies
 *
 * This file defines file format I/O.
 * L6 layer: IO/Workflow/API - handles file formats.
 *
 * Architecture Rule: L6 handles I/O, does NOT change simulation semantics.
 ********************************************************************************/

#ifndef FILE_IO_H
#define FILE_IO_H

#include "../../common/types.h"
#include "../../common/constants.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// File Format Types
// ============================================================================

/**
 * Input File Format
 */
typedef enum {
    FILE_FORMAT_STEP = 1,
    FILE_FORMAT_IGES = 2,
    FILE_FORMAT_OBJ = 3,
    FILE_FORMAT_STL = 4,
    FILE_FORMAT_GDSII = 5,
    FILE_FORMAT_OASIS = 6,
    FILE_FORMAT_GERBER = 7,
    FILE_FORMAT_DXF = 8
} input_file_format_t;

/**
 * Output File Format
 */
typedef enum {
    OUTPUT_FORMAT_TOUCHSTONE = 1,   // Touchstone (.s2p, .s4p)
    OUTPUT_FORMAT_HDF5 = 2,         // HDF5
    OUTPUT_FORMAT_VTK = 3,          // VTK
    OUTPUT_FORMAT_CSV = 4,          // CSV
    OUTPUT_FORMAT_SPICE = 5,        // SPICE netlist
    OUTPUT_FORMAT_JSON = 6          // JSON
} output_file_format_t;

// ============================================================================
// File I/O Interface
// ============================================================================

/**
 * Read geometry file
 * 
 * L6 layer reads file, does NOT interpret physics
 */
int file_io_read_geometry(
    const char* filename,
    input_file_format_t format,
    void* geometry_data      // Output: geometry data (L2 layer structure)
);

/**
 * Write results file
 * 
 * L6 layer writes file, does NOT change simulation semantics
 */
int file_io_write_results(
    const char* filename,
    output_file_format_t format,
    const void* results_data  // Input: results data (from L5)
);

/**
 * Detect file format
 */
input_file_format_t file_io_detect_format(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // FILE_IO_H
