/*****************************************************************************************
 * PEEC Comprehensive Geometry Support Header
 *****************************************************************************************/

#ifndef PEEC_GEOMETRY_SUPPORT_H
#define PEEC_GEOMETRY_SUPPORT_H

#include "../../common/core_common.h"
#include "../../discretization/mesh/core_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct peec_wire_element peec_wire_element_t;
typedef struct peec_filament_element peec_filament_element_t;

// Function declarations

// Quadrilateral support
int peec_extract_partial_elements_quadrilateral(
    mesh_t* mesh,
    double frequency,
    double* resistance_matrix,
    void* inductance_matrix,  // peec_complex_t*
    double* capacitance_matrix);

// Wire support
int peec_extract_partial_elements_wire(
    void* wires,  // peec_wire_element_t*
    int num_wires,
    double frequency,
    double* resistance_matrix,
    void* inductance_matrix,  // peec_complex_t*
    double* capacitance_matrix);

int peec_convert_mesh_to_wires(
    mesh_t* mesh,
    void** wires,  // peec_wire_element_t**
    int* num_wires);

// Filament support
int peec_extract_partial_elements_filament(
    void* filaments,  // peec_filament_element_t*
    int num_filaments,
    double frequency,
    double* resistance_matrix,
    void* inductance_matrix,  // peec_complex_t*
    double* capacitance_matrix);

int peec_convert_mesh_to_filaments(
    mesh_t* mesh,
    void** filaments,  // peec_filament_element_t**
    int* num_filaments);

// Unified geometry detection and extraction
int peec_detect_all_geometry_types(
    mesh_t* mesh,
    int* has_triangular,
    int* has_quadrilateral,
    int* has_wire,
    int* has_filament,
    int* has_manhattan);

int peec_extract_partial_elements_unified(
    mesh_t* mesh,
    double frequency,
    double* resistance_matrix,
    void* inductance_matrix,  // peec_complex_t*
    double* capacitance_matrix);

#ifdef __cplusplus
}
#endif

#endif // PEEC_GEOMETRY_SUPPORT_H

