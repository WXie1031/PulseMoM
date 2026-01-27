/*****************************************************************************************
 * PEEC Non-Manhattan Geometry Support Header
 *****************************************************************************************/

#ifndef PEEC_NON_MANHATTAN_GEOMETRY_H
#define PEEC_NON_MANHATTAN_GEOMETRY_H

#include "../../common/core_common.h"
#include "../../discretization/mesh/core_mesh.h"

#ifdef __cplusplus
extern "C" {
#endif

// Non-Manhattan element types
typedef enum {
    PEEC_ELEM_POLYGON = 10,      // General polygon
    PEEC_ELEM_CURVED = 11,       // Curved surface element
    PEEC_ELEM_TRIANGLE = 12,     // Triangular element
    PEEC_ELEM_TETRAHEDRON = 13   // Tetrahedral volume element
} peec_non_manhattan_type_t;

// Non-Manhattan element structure
typedef struct {
    int element_id;
    peec_non_manhattan_type_t type;
    int num_vertices;
    point3d_t* vertices;         // Vertex coordinates
    point3d_t* normals;          // Surface normals (for curved elements)
    double area;                 // Element area
    double volume;               // Element volume (for 3D elements)
    int material_id;
    int layer_id;
    
    // Geometric properties for partial element extraction
    point3d_t centroid;          // Centroid
    double characteristic_length; // Characteristic dimension
    double min_dimension;        // Minimum dimension
    double max_dimension;        // Maximum dimension
} peec_non_manhattan_element_t;

// Function declarations
int peec_detect_geometry_type(mesh_t* mesh, int* geom_type);
int peec_extract_partial_elements_non_manhattan(
    peec_non_manhattan_element_t* elements,
    int num_elements,
    double frequency,
    double* resistance_matrix,
    void* inductance_matrix,  // peec_complex_t*
    double* capacitance_matrix);
int peec_convert_geometry_to_elements(
    mesh_t* mesh,
    peec_non_manhattan_element_t** elements,
    int* num_elements);

#ifdef __cplusplus
}
#endif

#endif // PEEC_NON_MANHATTAN_GEOMETRY_H

