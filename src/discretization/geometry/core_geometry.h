/**
 * @file core_geometry.h
 * @brief Unified geometry engine for MoM and PEEC solvers
 * @details Provides shared geometric primitives, topology, and CAD interface
 * 
 * Copyright (c) 2024 PulseEM Technology Group
 * SPDX-License-Identifier: Proprietary
 */

#ifndef CORE_GEOMETRY_H
#define CORE_GEOMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include "core_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Geometry types
typedef enum {
    GEOM_TYPE_POINT,
    GEOM_TYPE_LINE,
    GEOM_TYPE_TRIANGLE,
    GEOM_TYPE_QUADRILATERAL,
    GEOM_TYPE_RECTANGLE,     // PEEC Manhattan geometry
    GEOM_TYPE_TETRAHEDRON,
    GEOM_TYPE_HEXAHEDRON,
    GEOM_TYPE_PRISM,
    GEOM_TYPE_PYRAMID,
    GEOM_TYPE_POLYGON,
    GEOM_TYPE_CIRCLE,
    GEOM_TYPE_MAX
} geom_element_type_t;

typedef enum {
    GEOM_FORMAT_STEP,
    GEOM_FORMAT_IGES,
    GEOM_FORMAT_OBJ,
    GEOM_FORMAT_STL,
    GEOM_FORMAT_OFF,
    GEOM_FORMAT_PLY,
    GEOM_FORMAT_GMSH,
    GEOM_FORMAT_GDSII,
    GEOM_FORMAT_OASIS,
    GEOM_FORMAT_GERBER,
    GEOM_FORMAT_EXCELLON,
    GEOM_FORMAT_DXF,
    GEOM_FORMAT_IPC2581,
    GEOM_FORMAT_CUSTOM
} geom_format_t;

// Basic geometric primitives
typedef struct {
    double x, y, z;
} geom_point_t;
typedef struct {
    int id;                    // Vertex identifier/index
    double coordinates[3];
} geom_vertex_t;

typedef struct {
    geom_point_t start;
    geom_point_t end;
    double radius;        // For wire models
    int segment_count;    // Discretization
} geom_line_t;

typedef struct {
    geom_point_t vertices[3];
    geom_point_t normal;
    double area;
} geom_triangle_t;

typedef struct {
    geom_point_t vertices[4];
    geom_point_t normal;
    double area;
    int is_rectangular;  // For Manhattan geometry (use int for MSVC compatibility)
} geom_quadrilateral_t;

typedef struct {
    geom_point_t corner;
    double width, height;
    geom_point_t normal;
} geom_rectangle_t;     // PEEC Manhattan geometry

// Topology and connectivity
typedef struct {
    int* vertices;        // Vertex indices
    int* edges;          // Edge indices  
    int* faces;          // Face indices
    int num_vertices;
    int num_edges;
    int num_faces;
} geom_topology_t;


// Unified geometry entity
typedef struct geom_entity {
    int id;
    geom_element_type_t type;
    union {
        geom_point_t point;
        geom_line_t line;
        geom_triangle_t triangle;
        geom_quadrilateral_t quad;
        geom_rectangle_t rectangle;
    } data;
    int global_id;
    int num_vertices;
    geom_vertex_t* vertices;
    double center[3];
    double bbox_min[3];
    double bbox_max[3];
    double radius;
    int entity_type;
    int material_id;
    int layer_id;
    int net_id;
    int is_mom_domain;
    int is_peec_domain;
    int is_hybrid_iface;
    struct geom_entity* next;
} geom_entity_t;

// Material properties
typedef struct geom_material {
    int id;
    char name[64];
    
    // Electrical properties
    double conductivity;      // S/m
    double permittivity;      // Relative
    double permeability;      // Relative
    double loss_tangent;
    
    // Frequency-dependent properties
    double* freq_points;      // Hz
    double* eps_real;         // Real permittivity
    double* eps_imag;         // Imaginary permittivity
    double* mu_real;          // Real permeability
    double* mu_imag;          // Imaginary permeability
    int num_freq_points;
    
    // Physical properties
    double density;           // kg/m³
    double thermal_conductivity;
} geom_material_t;

// Layer/stackup information (for PCB/IC)
typedef struct geom_layer {
    int id;
    char name[64];
    double thickness;         // m
    double elevation;         // m (bottom z-coordinate)
    int material_id;
    int is_conducting;        // Boolean as int for MSVC compatibility
    int is_dielectric;        // Boolean as int for MSVC compatibility
    double roughness;         // Surface roughness for skin effect
} geom_layer_t;

// Port/terminal definitions
typedef struct geom_port {
    int id;
    char name[64];
    geom_point_t position;
    geom_point_t direction;   // Normal vector
    double characteristic_impedance;
    int positive_node;
    int negative_node;
    int num_modes;            // For multi-mode ports
} geom_port_t;

// Geometry engine
typedef struct {
    geom_entity_t* entities;
    int num_entities;
    int entity_capacity;
    geom_layer_t* layers;
    int num_layers;
    int layer_capacity;
    geom_material_t* materials;
    int num_materials;
    int material_capacity;
    geom_port_t* ports;
    int num_ports;
    int port_capacity;
    void* internal;
    geom_point_t min_bound;
    geom_point_t max_bound;
    geom_topology_t* topology;
    int num_domains;
    int* domain_offsets;
    int* domain_ids;
    struct {
        double tolerance;
        double min_feature_size;
        double max_curvature;
        int validate_geometry;
        int check_manhattan;
    } config;
    int is_valid;
    char* validation_errors;
} geom_geometry_t;

/*
*********************************************************************
 * Core Geometry Functions
 *********************************************************************
*/
// Geometry creation and management
geom_geometry_t* geom_geometry_create(void);
void geom_geometry_destroy(geom_geometry_t* geom);

// Entity operations
int geom_geometry_add_entity(geom_geometry_t* geom, const geom_entity_t* entity);
int geom_geometry_get_entity(geom_geometry_t* geom, int entity_id, geom_entity_t** out);
int geom_geometry_remove_entity(geom_geometry_t* geom, int entity_id);

// Material and layer management
int geom_geometry_add_material(geom_geometry_t* geom, const geom_material_t* material);
int geom_geometry_add_layer(geom_geometry_t* geom, const geom_layer_t* layer);
geom_material_t* geom_geometry_get_material(geom_geometry_t* geom, int material_id);
geom_layer_t* geom_geometry_get_layer(geom_geometry_t* geom, int layer_id);

// Port definitions
int geom_geometry_add_port(geom_geometry_t* geom, const geom_port_t* port);
geom_port_t* geom_geometry_get_port(geom_geometry_t* geom, int port_id);
int geom_geometry_get_num_ports(geom_geometry_t* geom);

// CAD import/export
int geom_geometry_import_from_file(geom_geometry_t* geom, const char* filename, geom_format_t format);
int geom_geometry_export_to_file(const geom_geometry_t* geom, const char* filename, geom_format_t format);

// Validation and analysis
int geom_geometry_validate(geom_geometry_t* geom);  // Returns 1 for true, 0 for false
const char* geom_geometry_get_validation_errors(geom_geometry_t* geom);
void geom_geometry_compute_bounding_box(geom_geometry_t* geom);

// Domain decomposition
int geom_geometry_partition_domains(geom_geometry_t* geom, int num_domains);
int geom_geometry_get_domain_for_entity(geom_geometry_t* geom, int entity_id);

// Utility functions
double geom_geometry_compute_total_area(const geom_geometry_t* geom);
double geom_geometry_compute_total_volume(const geom_geometry_t* geom);
int geom_geometry_get_statistics(const geom_geometry_t* geom, int* num_points, int* num_lines, 
                                int* num_triangles, int* num_quadrilaterals);

// Manhattan geometry support (PEEC)
int geom_geometry_create_manhattan_grid(geom_geometry_t* geom, const geom_rectangle_t* bounds, double grid_size);
int geom_geometry_convert_to_manhattan(geom_geometry_t* geom, double tolerance);
int geom_geometry_is_manhattan(const geom_geometry_t* geom);  // Returns 1 for true, 0 for false

// Wire model support (shared between MoM and PEEC)
int geom_geometry_add_wire(geom_geometry_t* geom, const geom_line_t* wire, int material_id);
int geom_geometry_create_wire_bundle(geom_geometry_t* geom, const geom_point_t* center, 
                                   double radius, int num_wires, double wire_radius);

// Triangle metrics helpers (for MoM EFIE self/near-term handling)
double geom_triangle_compute_area(const geom_triangle_t* tri);
// Get triangle area, using cached value if available, otherwise compute
// This is a convenience function that encapsulates the common pattern:
// area = (tri->area > 0.0) ? tri->area : geom_triangle_compute_area(tri)
double geom_triangle_get_area(const geom_triangle_t* tri);
double geom_triangle_compute_edge_length(const geom_triangle_t* tri, int edge_idx);
double geom_triangle_compute_average_edge_length(const geom_triangle_t* tri);
double geom_triangle_compute_centroid_distance(const geom_triangle_t* tri_i, const geom_triangle_t* tri_j);

// Triangle geometry helpers
void geom_triangle_get_centroid(const geom_triangle_t* tri, geom_point_t* centroid);
void geom_triangle_interpolate_point(const geom_triangle_t* tri, double u, double v, double w, geom_point_t* result);
void geom_triangle_get_edge_midpoint(const geom_triangle_t* tri, int edge_idx, geom_point_t* midpoint);
double geom_point_distance(const geom_point_t* p1, const geom_point_t* p2);
void geom_triangle_subdivide(const geom_triangle_t* tri, geom_triangle_t sub_tri[4]);

// Mesh generation interface
struct mesh_engine;  // Forward declaration
typedef struct mesh_engine mesh_engine_t;

int geom_geometry_create_mesh(geom_geometry_t* geom, mesh_engine_t* mesh_engine, 
                             double mesh_size, int refinement_level);

#ifdef __cplusplus
}
#endif

#endif // CORE_GEOMETRY_H
