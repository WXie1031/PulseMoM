/*****************************************************************************************
 * PEEC Comprehensive Geometry Support
 * 
 * Unified support for all geometry types in PEEC solver:
 * - Triangular (triangles)
 * - Quadrilateral (quads)
 * - Wire (line segments)
 * - Filament (thin wire elements)
 * - Manhattan rectangles (existing)
 *****************************************************************************************/

#include "peec_solver.h"
#include "../core/core_geometry.h"
#include "../core/core_mesh.h"
#include "../core/core_common.h"
#include "peec_non_manhattan_geometry.h"
#include "peec_integration.h"
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

// Physical constants
#define MU0 (4.0 * M_PI * 1e-7)
#ifndef EPS0
#define EPS0 (8.854187817e-12)
#endif

// Use point3d_t from core_common.h (already defined)
// For compatibility, ensure geom_point_t and point3d_t are compatible
// Both have x, y, z members, so direct assignment should work

// Forward declarations
int peec_extract_partial_elements_triangular(
    mesh_t* mesh,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix);

// Wire element structure for PEEC
typedef struct peec_wire_element {
    int wire_id;
    point3d_t start;              // Start point
    point3d_t end;                 // End point
    double radius;                 // Wire radius
    double length;                 // Wire length
    int material_id;               // Material ID
    int conductor_id;              // Conductor ID
    int node_start;                // Start node index
    int node_end;                  // End node index
} peec_wire_element_t;

// Filament element structure (thin wire approximation)
typedef struct peec_filament_element {
    int filament_id;
    point3d_t start;               // Start point
    point3d_t end;                  // End point
    double radius;                  // Filament radius (typically very small)
    double length;                  // Filament length
    int material_id;                // Material ID
    int conductor_id;               // Conductor ID
    int node_start;                // Start node index
    int node_end;                  // End node index
    int is_thin_wire;             // Thin wire approximation flag (use int for MSVC)
} peec_filament_element_t;

/********************************************************************************
 * Extract Partial Elements from Quadrilateral Mesh
 ********************************************************************************/
int peec_extract_partial_elements_quadrilateral(
    mesh_t* mesh,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix) {
    
    if (!mesh || !resistance_matrix || !inductance_matrix || !capacitance_matrix) {
        return -1;
    }
    
    int num_quads = 0;
    int* quad_indices = NULL;
    
    // Count quadrilaterals
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].type == MESH_ELEMENT_QUADRILATERAL) {
            num_quads++;
        }
    }
    
    if (num_quads == 0) {
        return 0; // No quadrilaterals found
    }
    
    // Allocate quad index array
    quad_indices = (int*)malloc(num_quads * sizeof(int));
    if (!quad_indices) return -1;
    
    // Collect quad indices
    int quad_idx = 0;
    for (int i = 0; i < mesh->num_elements; i++) {
        if (mesh->elements[i].type == MESH_ELEMENT_QUADRILATERAL) {
            quad_indices[quad_idx++] = i;
        }
    }
    
    // Extract partial elements for each quad pair
    for (int i = 0; i < num_quads; i++) {
        mesh_element_t* quad_i = &mesh->elements[quad_indices[i]];
        
        for (int j = 0; j < num_quads; j++) {
            mesh_element_t* quad_j = &mesh->elements[quad_indices[j]];
            
            // Compute quad centroids (using point3d_t from core_common.h)
            point3d_t centroid_i, centroid_j;
            centroid_i.x = quad_i->centroid.x;
            centroid_i.y = quad_i->centroid.y;
            centroid_i.z = quad_i->centroid.z;
            centroid_j.x = quad_j->centroid.x;
            centroid_j.y = quad_j->centroid.y;
            centroid_j.z = quad_j->centroid.z;
            
            // Compute distance between centroids using unified function
            double distance = vector3d_distance(&centroid_i, &centroid_j);
            
            if (i == j) {
                // Self terms
                // Resistance: R = ρ * l / A
                double resistivity = 1.7e-8; // Copper
                double area = quad_i->area;
                
                // Compute equivalent dimensions for quad
                double perimeter = 0.0;
                if (quad_i->num_vertices >= 4) {
                    for (int k = 0; k < 4; k++) {
                        int v1 = quad_i->vertices[k];
                        int v2 = quad_i->vertices[(k+1) % 4];
                        if (v1 < mesh->num_vertices && v2 < mesh->num_vertices) {
                            geom_point_t p1 = mesh->vertices[v1].position;
                            geom_point_t p2 = mesh->vertices[v2].position;
                            double edge_len = sqrt((p2.x-p1.x)*(p2.x-p1.x) + 
                                                  (p2.y-p1.y)*(p2.y-p1.y) + 
                                                  (p2.z-p1.z)*(p2.z-p1.z));
                            perimeter += edge_len;
                        }
                    }
                }
                
                double equivalent_length = perimeter / 4.0;
                double equivalent_thickness = sqrt(area) * 0.1; // Simplified
                double cross_section = equivalent_length * equivalent_thickness;
                
                if (cross_section > 0) {
                    resistance_matrix[i * num_quads + j] = 
                        resistivity * equivalent_length / cross_section;
                }
                
                // Self inductance using surface integral
                if (quad_i->num_vertices >= 4) {
                    point3d_t* vertices = (point3d_t*)malloc(4 * sizeof(point3d_t));
                    if (vertices) {
                        for (int k = 0; k < 4; k++) {
                            int v_idx = quad_i->vertices[k];
                            if (v_idx < mesh->num_vertices) {
                                vertices[k].x = mesh->vertices[v_idx].position.x;
                                vertices[k].y = mesh->vertices[v_idx].position.y;
                                vertices[k].z = mesh->vertices[v_idx].position.z;
                            }
                        }
                        // Normal vector (using point3d_t from core_common.h)
                        point3d_t normal;
                        normal.x = quad_i->normal.x;
                        normal.y = quad_i->normal.y;
                        normal.z = quad_i->normal.z;
                        inductance_matrix[i * num_quads + j] = 
                            peec_compute_partial_inductance_quad_integral(
                                vertices, 4, vertices, 4,
                                &normal, &normal, area, area);
                        free(vertices);
                    }
                }
                
                // Self capacitance using surface integral
                if (quad_i->num_vertices >= 4) {
                    point3d_t* vertices = (point3d_t*)malloc(4 * sizeof(point3d_t));
                    if (vertices) {
                        for (int k = 0; k < 4; k++) {
                            int v_idx = quad_i->vertices[k];
                            if (v_idx < mesh->num_vertices) {
                                vertices[k].x = mesh->vertices[v_idx].position.x;
                            vertices[k].y = mesh->vertices[v_idx].position.y;
                            vertices[k].z = mesh->vertices[v_idx].position.z;
                            }
                        }
                        capacitance_matrix[i * num_quads + j] = 
                            peec_compute_partial_capacitance_quad_integral(
                                vertices, 4, vertices, 4, area, area);
                        free(vertices);
                    }
                }
            } else {
                // Mutual terms using surface integrals
                if (quad_i->num_vertices >= 4 && quad_j->num_vertices >= 4) {
                    point3d_t* vertices_i = (point3d_t*)malloc(4 * sizeof(point3d_t));
                    point3d_t* vertices_j = (point3d_t*)malloc(4 * sizeof(point3d_t));
                    
                    if (vertices_i && vertices_j) {
                        for (int k = 0; k < 4; k++) {
                            int v_idx_i = quad_i->vertices[k];
                            int v_idx_j = quad_j->vertices[k];
                            if (v_idx_i < mesh->num_vertices) {
                                vertices_i[k].x = mesh->vertices[v_idx_i].position.x;
                                vertices_i[k].y = mesh->vertices[v_idx_i].position.y;
                                vertices_i[k].z = mesh->vertices[v_idx_i].position.z;
                            }
                            if (v_idx_j < mesh->num_vertices) {
                                vertices_j[k].x = mesh->vertices[v_idx_j].position.x;
                                vertices_j[k].y = mesh->vertices[v_idx_j].position.y;
                                vertices_j[k].z = mesh->vertices[v_idx_j].position.z;
                            }
                        }
                        
                        // Mutual inductance using surface integral
                        point3d_t normal_i, normal_j;
                        normal_i.x = quad_i->normal.x; normal_i.y = quad_i->normal.y; normal_i.z = quad_i->normal.z;
                        normal_j.x = quad_j->normal.x; normal_j.y = quad_j->normal.y; normal_j.z = quad_j->normal.z;
                        inductance_matrix[i * num_quads + j] = 
                            peec_compute_partial_inductance_quad_integral(
                                vertices_i, 4, vertices_j, 4,
                                &normal_i, &normal_j,
                                quad_i->area, quad_j->area);
                        
                        // Mutual capacitance using surface integral
                        capacitance_matrix[i * num_quads + j] = 
                            peec_compute_partial_capacitance_quad_integral(
                                vertices_i, 4, vertices_j, 4,
                                quad_i->area, quad_j->area);
                        
                        free(vertices_i);
                        free(vertices_j);
                    }
                }
            }
        }
    }
    
    free(quad_indices);
    return 0;
}

/********************************************************************************
 * Extract Partial Elements from Wire Elements
 ********************************************************************************/
int peec_extract_partial_elements_wire(
    peec_wire_element_t* wires,
    int num_wires,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix) {
    
    if (!wires || num_wires <= 0) return -1;
    
    // Extract partial elements using wire formulas
    for (int i = 0; i < num_wires; i++) {
        peec_wire_element_t* wire_i = &wires[i];
        
        for (int j = 0; j < num_wires; j++) {
            peec_wire_element_t* wire_j = &wires[j];
            
            // Compute distance between wire centers
            double dx = (wire_i->start.x + wire_i->end.x) / 2.0 - 
                       (wire_j->start.x + wire_j->end.x) / 2.0;
            double dy = (wire_i->start.y + wire_i->end.y) / 2.0 - 
                       (wire_j->start.y + wire_j->end.y) / 2.0;
            double dz = (wire_i->start.z + wire_i->end.z) / 2.0 - 
                       (wire_j->start.z + wire_j->end.z) / 2.0;
            double distance = sqrt(dx*dx + dy*dy + dz*dz);
            
            if (i == j) {
                // Self terms
                // Resistance: R = ρ * l / (π * r²)
                double resistivity = 1.7e-8; // Copper
                double cross_section = M_PI * wire_i->radius * wire_i->radius;
                if (cross_section > 0) {
                    resistance_matrix[i * num_wires + j] = 
                        resistivity * wire_i->length / cross_section;
                }
                
                // Self inductance using complete Neumann integral
                inductance_matrix[i * num_wires + j] = 
                    peec_compute_partial_inductance_wire_integral(
                        &wire_i->start, &wire_i->end,
                        &wire_i->start, &wire_i->end,
                        wire_i->radius, wire_i->radius);
                
                // Self capacitance (simplified)
                double eps = EPS0;
                double r_self = wire_i->radius;
                double l_self = wire_i->length;
                double separation = 2.0 * r_self; // Simplified
                if (separation > 0) {
                    capacitance_matrix[i * num_wires + j] = 
                        eps * M_PI * l_self * r_self / separation;
                }
            } else {
                // Mutual inductance using complete Neumann integral
                inductance_matrix[i * num_wires + j] = 
                    peec_compute_partial_inductance_wire_integral(
                        &wire_i->start, &wire_i->end,
                        &wire_j->start, &wire_j->end,
                        wire_i->radius, wire_j->radius);
                
                // Mutual capacitance (simplified)
                double eps = EPS0;
                double l_avg_cap = 0.5 * (wire_i->length + wire_j->length);
                double r_avg = 0.5 * (wire_i->radius + wire_j->radius);
                capacitance_matrix[i * num_wires + j] = 
                    eps * M_PI * l_avg_cap * r_avg / distance;
            }
        }
    }
    
    return 0;
}

/********************************************************************************
 * Extract Partial Elements from Filament Elements (Thin Wire)
 ********************************************************************************/
int peec_extract_partial_elements_filament(
    peec_filament_element_t* filaments,
    int num_filaments,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix) {
    
    if (!filaments || num_filaments <= 0) return -1;
    
    // Filament is essentially a very thin wire, use thin-wire approximation
    // Convert to wire elements and use wire extraction
    peec_wire_element_t* wires = (peec_wire_element_t*)calloc(num_filaments, 
                                                               sizeof(peec_wire_element_t));
    if (!wires) return -1;
    
    for (int i = 0; i < num_filaments; i++) {
        wires[i].wire_id = filaments[i].filament_id;
        wires[i].start = filaments[i].start;
        wires[i].end = filaments[i].end;
        wires[i].radius = filaments[i].radius;
        wires[i].length = filaments[i].length;
        wires[i].material_id = filaments[i].material_id;
        wires[i].conductor_id = filaments[i].conductor_id;
        wires[i].node_start = filaments[i].node_start;
        wires[i].node_end = filaments[i].node_end;
    }
    
    // Use wire extraction (thin-wire approximation)
    int result = peec_extract_partial_elements_wire(wires, num_filaments, frequency,
                                                     resistance_matrix, 
                                                     inductance_matrix, 
                                                     capacitance_matrix);
    
    free(wires);
    return result;
}

/********************************************************************************
 * Convert Mesh to Wire Elements
 ********************************************************************************/
int peec_convert_mesh_to_wires(
    mesh_t* mesh,
    peec_wire_element_t** wires,
    int* num_wires) {
    
    if (!mesh || !wires || !num_wires) return -1;
    
    // Count edges that can be treated as wires
    int wire_count = 0;
    for (int i = 0; i < mesh->num_edges; i++) {
        mesh_edge_t* edge = &mesh->edges[i];
        if (edge->radius > 0) {  // Has radius, can be treated as wire
            wire_count++;
        }
    }
    
    if (wire_count == 0) {
        *num_wires = 0;
        *wires = NULL;
        return 0;
    }
    
    *num_wires = wire_count;
    *wires = (peec_wire_element_t*)calloc(wire_count, sizeof(peec_wire_element_t));
    if (!*wires) return -1;
    
    int wire_idx = 0;
    for (int i = 0; i < mesh->num_edges; i++) {
        mesh_edge_t* edge = &mesh->edges[i];
        if (edge->radius > 0) {
            peec_wire_element_t* wire = &(*wires)[wire_idx];
            
            wire->wire_id = wire_idx;
            if (edge->vertex1_id < mesh->num_vertices) {
                wire->start.x = mesh->vertices[edge->vertex1_id].position.x;
                wire->start.y = mesh->vertices[edge->vertex1_id].position.y;
                wire->start.z = mesh->vertices[edge->vertex1_id].position.z;
            }
            if (edge->vertex2_id < mesh->num_vertices) {
                wire->end.x = mesh->vertices[edge->vertex2_id].position.x;
                wire->end.y = mesh->vertices[edge->vertex2_id].position.y;
                wire->end.z = mesh->vertices[edge->vertex2_id].position.z;
            }
            wire->radius = edge->radius;
            wire->length = edge->length;
            wire->node_start = edge->vertex1_id;
            wire->node_end = edge->vertex2_id;
            
            wire_idx++;
        }
    }
    
    return 0;
}

/********************************************************************************
 * Convert Mesh to Filament Elements
 ********************************************************************************/
int peec_convert_mesh_to_filaments(
    mesh_t* mesh,
    peec_filament_element_t** filaments,
    int* num_filaments) {
    
    if (!mesh || !filaments || !num_filaments) return -1;
    
    // Filaments are very thin wires, similar to wire conversion
    // but with thin-wire approximation flag
    peec_wire_element_t* wires = NULL;
    int num_wires = 0;
    
    if (peec_convert_mesh_to_wires(mesh, &wires, &num_wires) != 0) {
        return -1;
    }
    
    if (num_wires == 0) {
        *num_filaments = 0;
        *filaments = NULL;
        return 0;
    }
    
    *num_filaments = num_wires;
    *filaments = (peec_filament_element_t*)calloc(num_wires, sizeof(peec_filament_element_t));
    if (!*filaments) {
        free(wires);
        return -1;
    }
    
    for (int i = 0; i < num_wires; i++) {
        peec_filament_element_t* filament = &(*filaments)[i];
        peec_wire_element_t* wire = &wires[i];
        
        filament->filament_id = i;
        filament->start = wire->start;
        filament->end = wire->end;
        filament->radius = wire->radius;
        filament->length = wire->length;
        filament->material_id = wire->material_id;
        filament->conductor_id = wire->conductor_id;
        filament->node_start = wire->node_start;
        filament->node_end = wire->node_end;
        filament->is_thin_wire = 1;  // Filament uses thin-wire approximation
    }
    
    free(wires);
    return 0;
}

/********************************************************************************
 * Unified Geometry Type Detection
 ********************************************************************************/
int peec_detect_all_geometry_types(
    mesh_t* mesh,
    int* has_triangular,
    int* has_quadrilateral,
    int* has_wire,
    int* has_filament,
    int* has_manhattan) {
    
    if (!mesh) return -1;
    
    *has_triangular = 0;
    *has_quadrilateral = 0;
    *has_wire = 0;
    *has_filament = 0;
    *has_manhattan = 0;
    
    // Check mesh elements
    for (int i = 0; i < mesh->num_elements; i++) {
        switch (mesh->elements[i].type) {
            case MESH_ELEMENT_TRIANGLE:
                *has_triangular = 1;
                break;
            case MESH_ELEMENT_QUADRILATERAL:
                *has_quadrilateral = 1;
                break;
            case MESH_ELEMENT_RECTANGLE:
                *has_manhattan = 1;
                break;
            default:
                break;
        }
    }
    
    // Check edges for wires/filaments
    for (int i = 0; i < mesh->num_edges; i++) {
        mesh_edge_t* edge = &mesh->edges[i];
        if (edge->radius > 0) {
            // Very thin radius indicates filament
            if (edge->radius < 1e-6) {  // Less than 1 micron
                *has_filament = 1;
            } else {
                *has_wire = 1;
            }
        }
    }
    
    return 0;
}

/********************************************************************************
 * Unified Partial Element Extraction (All Geometry Types)
 ********************************************************************************/
int peec_extract_partial_elements_unified(
    mesh_t* mesh,
    double frequency,
    double* resistance_matrix,
    peec_complex_t* inductance_matrix,
    double* capacitance_matrix) {
    
    if (!mesh) return -1;
    
    int has_tri, has_quad, has_wire, has_filament, has_manhattan;
    peec_detect_all_geometry_types(mesh, &has_tri, &has_quad, &has_wire, 
                                    &has_filament, &has_manhattan);
    
    // Extract based on detected geometry types
    if (has_tri) {
        peec_extract_partial_elements_triangular(mesh, frequency,
                                                  resistance_matrix,
                                                  inductance_matrix,
                                                  capacitance_matrix);
    }
    
    if (has_quad) {
        peec_extract_partial_elements_quadrilateral(mesh, frequency,
                                                     resistance_matrix,
                                                     inductance_matrix,
                                                     capacitance_matrix);
    }
    
    if (has_wire) {
        peec_wire_element_t* wires = NULL;
        int num_wires = 0;
        if (peec_convert_mesh_to_wires(mesh, &wires, &num_wires) == 0 && num_wires > 0) {
            peec_extract_partial_elements_wire(wires, num_wires, frequency,
                                                resistance_matrix,
                                                inductance_matrix,
                                                capacitance_matrix);
            free(wires);
        }
    }
    
    if (has_filament) {
        peec_filament_element_t* filaments = NULL;
        int num_filaments = 0;
        if (peec_convert_mesh_to_filaments(mesh, &filaments, &num_filaments) == 0 && num_filaments > 0) {
            peec_extract_partial_elements_filament(filaments, num_filaments, frequency,
                                                   resistance_matrix,
                                                   inductance_matrix,
                                                   capacitance_matrix);
            free(filaments);
        }
    }
    
    return 0;
}

