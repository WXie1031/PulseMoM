/*****************************************************************************************
 * PEEC Non-Manhattan Geometry Support
 * 
 * Enhanced support for non-rectangular geometries in PEEC solver
 * Supports: polygons, curved surfaces, arbitrary 3D shapes
 *****************************************************************************************/

#include "peec_solver.h"
#include "../../discretization/geometry/core_geometry.h"
#include "../../discretization/mesh/core_mesh.h"
#include "../../common/core_common.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// MSVC-compatible complex number handling
#if defined(_MSC_VER)
typedef complex_t peec_complex_t;
#else
#include <complex.h>
typedef double complex peec_complex_t;
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

/********************************************************************************
 * Detect Geometry Type
 ********************************************************************************/
int peec_detect_geometry_type(mesh_t* mesh, int* geom_type) {
    if (!mesh || !geom_type) return -1;
    
    // Geometry type constants (matching peec_solver_unified.c)
    const int PEEC_GEOM_MANHATTAN = 0;
    const int PEEC_GEOM_ORTHOGONAL = 1;
    const int PEEC_GEOM_GENERAL = 2;
    const int PEEC_GEOM_CURVED = 3;
    
    int has_curved = 0;
    int has_non_rect = 0;
    int num_rect = 0;
    int num_tri = 0;
    int num_poly = 0;
    
    // Analyze mesh elements
    for (int i = 0; i < mesh->num_elements; i++) {
        mesh_element_t* elem = &mesh->elements[i];
        
        switch (elem->type) {
            case MESH_ELEMENT_RECTANGLE:
                num_rect++;
                break;
            case MESH_ELEMENT_TRIANGLE:
                num_tri++;
                break;
            case MESH_ELEMENT_POLYGON:
                num_poly++;
                has_non_rect = 1;
                break;
            default:
                has_non_rect = 1;
                break;
        }
        
        // Check for curved surfaces (simplified - check normal variation)
        if (i > 0) {
            mesh_element_t* prev = &mesh->elements[i-1];
            double dot = elem->normal.x * prev->normal.x +
                        elem->normal.y * prev->normal.y +
                        elem->normal.z * prev->normal.z;
            if (dot < 0.9) {  // Significant normal variation
                has_curved = 1;
            }
        }
    }
    
    // Determine geometry type
    if (has_curved) {
        *geom_type = PEEC_GEOM_CURVED;
    } else if (has_non_rect || num_poly > 0 || num_tri > num_rect) {
        *geom_type = PEEC_GEOM_GENERAL;
    } else if (num_rect > 0 && num_tri == 0 && num_poly == 0) {
        *geom_type = PEEC_GEOM_MANHATTAN;
    } else {
        *geom_type = PEEC_GEOM_ORTHOGONAL;
    }
    
    return 0;
}

/********************************************************************************
 * Extract Partial Elements from Non-Manhattan Geometry
 ********************************************************************************/
int peec_extract_partial_elements_non_manhattan(
    peec_non_manhattan_element_t* elements,
    int num_elements,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix) {
    
    if (!elements || num_elements <= 0) return -1;
    
    // Extract partial elements using numerical integration
    for (int i = 0; i < num_elements; i++) {
        peec_non_manhattan_element_t* elem_i = &elements[i];
        
        for (int j = 0; j < num_elements; j++) {
            peec_non_manhattan_element_t* elem_j = &elements[j];
            
            // Compute distance between centroids
            double dx = elem_i->centroid.x - elem_j->centroid.x;
            double dy = elem_i->centroid.y - elem_j->centroid.y;
            double dz = elem_i->centroid.z - elem_j->centroid.z;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (i == j) {
                // Self terms
                // Resistance
                double resistivity = 1.7e-8; // Copper
                double cross_section = elem_i->area / elem_i->characteristic_length;
                resistance_matrix[i * num_elements + j] = 
                    resistivity * elem_i->characteristic_length / cross_section;
                
                // Self inductance (simplified)
                double mu = MU0;
                #if defined(_MSC_VER)
                peec_complex_t L_self = {
                    (mu * elem_i->characteristic_length / (2.0 * M_PI)) * 
                    log(2.0 * elem_i->characteristic_length / elem_i->min_dimension),
                    0.0
                };
                inductance_matrix[i * num_elements + j] = L_self;
                #else
                inductance_matrix[i * num_elements + j] = 
                    (mu * elem_i->characteristic_length / (2.0 * M_PI)) * 
                    log(2.0 * elem_i->characteristic_length / elem_i->min_dimension);
                #endif
                
                // Self capacitance
                double eps = EPS0;
                capacitance_matrix[i * num_elements + j] = 
                    eps * elem_i->area / elem_i->min_dimension;
            } else {
                // Mutual terms
                if (distance > 0) {
                    // Mutual inductance
                    double mu = MU0;
                    double length_avg = 0.5 * (elem_i->characteristic_length + 
                                              elem_j->characteristic_length);
                    #if defined(_MSC_VER)
                    peec_complex_t L_mutual = {(mu / (2.0 * M_PI)) * log(length_avg / distance), 0.0};
                    inductance_matrix[i * num_elements + j] = L_mutual;
                    #else
                    inductance_matrix[i * num_elements + j] = 
                        (mu / (2.0 * M_PI)) * log(length_avg / distance);
                    #endif
                    
                    // Mutual capacitance
                    double eps = EPS0;
                    double area_avg = 0.5 * (elem_i->area + elem_j->area);
                    capacitance_matrix[i * num_elements + j] = 
                        eps * area_avg / distance;
                }
            }
        }
    }
    
    return 0;
}

/********************************************************************************
 * Convert General Geometry to PEEC Elements
 ********************************************************************************/
int peec_convert_geometry_to_elements(
    mesh_t* mesh,
    peec_non_manhattan_element_t** elements,
    int* num_elements) {
    
    if (!mesh || !elements || !num_elements) return -1;
    
    *num_elements = mesh->num_elements;
    *elements = (peec_non_manhattan_element_t*)calloc(*num_elements, 
                                                      sizeof(peec_non_manhattan_element_t));
    if (!*elements) return -1;
    
    for (int i = 0; i < *num_elements; i++) {
        mesh_element_t* mesh_elem = &mesh->elements[i];
        peec_non_manhattan_element_t* peec_elem = &(*elements)[i];
        
        peec_elem->element_id = i;
        peec_elem->area = mesh_elem->area;
        peec_elem->volume = mesh_elem->volume;
        peec_elem->centroid.x = mesh_elem->centroid.x;
        peec_elem->centroid.y = mesh_elem->centroid.y;
        peec_elem->centroid.z = mesh_elem->centroid.z;
        peec_elem->material_id = mesh_elem->material_id;
        peec_elem->layer_id = 0; // Default layer
        
        // Determine element type
        switch (mesh_elem->type) {
            case MESH_ELEMENT_TRIANGLE:
                peec_elem->type = PEEC_ELEM_TRIANGLE;
                break;
            case MESH_ELEMENT_QUADRILATERAL:
                peec_elem->type = PEEC_ELEM_POLYGON;  // Quad is a type of polygon
                break;
            case MESH_ELEMENT_TETRAHEDRON:
                peec_elem->type = PEEC_ELEM_TETRAHEDRON;
                break;
            case MESH_ELEMENT_POLYGON:
                peec_elem->type = PEEC_ELEM_POLYGON;
                break;
            default:
                peec_elem->type = PEEC_ELEM_POLYGON;
                break;
        }
        
        // Compute characteristic dimensions
        if (mesh_elem->num_vertices > 0) {
            peec_elem->num_vertices = mesh_elem->num_vertices;
            peec_elem->vertices = (point3d_t*)malloc(mesh_elem->num_vertices * sizeof(point3d_t));
            
            double min_dist = 1e10;
            double max_dist = 0.0;
            
            for (int j = 0; j < mesh_elem->num_vertices; j++) {
                int v_idx = mesh_elem->vertices[j];
                if (v_idx < mesh->num_vertices) {
                    // Convert geom_point_t to point3d_t (both have x, y, z members)
                    peec_elem->vertices[j].x = mesh->vertices[v_idx].position.x;
                    peec_elem->vertices[j].y = mesh->vertices[v_idx].position.y;
                    peec_elem->vertices[j].z = mesh->vertices[v_idx].position.z;
                    
                    // Compute distance from centroid
                    double dx = peec_elem->vertices[j].x - peec_elem->centroid.x;
                    double dy = peec_elem->vertices[j].y - peec_elem->centroid.y;
                    double dz = peec_elem->vertices[j].z - peec_elem->centroid.z;
                    double dist = sqrt(dx*dx + dy*dy + dz*dz);
                    
                    if (dist < min_dist) min_dist = dist;
                    if (dist > max_dist) max_dist = dist;
                }
            }
            
            peec_elem->min_dimension = 2.0 * min_dist;
            peec_elem->max_dimension = 2.0 * max_dist;
            peec_elem->characteristic_length = sqrt(peec_elem->area);
        }
    }
    
    return 0;
}

