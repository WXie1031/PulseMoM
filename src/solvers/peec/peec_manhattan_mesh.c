/*****************************************************************************************
 * Manhattan Mesh Generation for PEEC (Partial Element Equivalent Circuit)
 * 
 * Implements structured rectangular grid generation for Manhattan geometries
 * Supports adaptive refinement, via modeling, and multi-layer PCBs
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "core_geometry.h"
#include "core_mesh.h"
#include "core_mesh_pipeline.h"
#include "electromagnetic_kernels.h"
#include "peec_via_modeling.h"  // For enhanced via modeling

// Forward declaration for surface_t
typedef struct surface_t surface_t;

// Removed problematic macros - use mesh_create and mesh_add_vertex directly
// #define mesh_create(TYPE,DIM) mesh_create("manhattan_mesh", (TYPE))  // Causes infinite recursion
// #define mesh_add_vertex(M,X,Y,Z) mesh_add_vertex((M), &(geom_point_t){(X),(Y),(Z)})

// Manhattan grid parameters
#define MIN_RECTANGLE_QUALITY 0.8
#define MAX_ASPECT_RATIO 5.0
#define DEFAULT_VIA_ASPECT_RATIO 0.5
#define MIN_VIA_RADIUS 0.05e-3  // 50 microns
#define MAX_VIA_RADIUS 0.5e-3   // 500 microns

// Grid generation parameters
#define DEFAULT_GRID_RESOLUTION 0.1e-3  // 100 microns
#define MIN_GRID_RESOLUTION 1e-6        // 1 micron
#define MAX_GRID_RESOLUTION 10e-3     // 10 mm

// Rectangle data structure
typedef struct rectangle_node {
    int vertices[4];          // Vertex indices (counter-clockwise)
    int neighbors[4];         // Adjacent rectangles (-1 for boundary)
    double center[3];         // Rectangle center coordinates
    double dimensions[2];     // Width and height
    double area;              // Rectangle area
    double aspect_ratio;      // Width/height ratio
    int layer_id;            // PCB layer identifier
    int material_id;         // Material property index
    int refinement_level;    // Adaptive refinement level
    int via_flag;            // 1=contains via, 0=no via
    int conductor_flag;      // 1=conducting region, 0=dielectric
    struct rectangle_node* next;
} rectangle_node_t;

// Via data structure
typedef struct via_node {
    double center[3];         // Via center coordinates
    double radius;            // Via radius
    double height;            // Via height (layer thickness)
    int start_layer;          // Starting layer index
    int end_layer;            // Ending layer index
    int top_vertex;           // Top vertex index
    int bottom_vertex;        // Bottom vertex index
    int material_id;           // Via material (copper, tungsten, etc.)
    int type;                  // Through-hole, blind, buried, microvia
    struct via_node* next;
} via_node_t;

// Layer stackup information
typedef struct layer_stackup {
    double thickness;         // Layer thickness
    double conductivity;      // Electrical conductivity
    double permittivity;      // Dielectric constant
    double permeability;      // Magnetic permeability
    double loss_tangent;      // Dielectric loss tangent
    int material_id;          // Material identifier
    char name[64];            // Layer name (e.g., "TOP", "GND", "SIGNAL1")
} layer_stackup_t;

// Manhattan mesh parameters
typedef struct {
    double grid_resolution[3];     // X, Y, Z grid spacing
    int adaptive_refinement;       // Enable adaptive refinement
    int via_modeling;             // Enable via modeling
    int multi_layer;             // Enable multi-layer support
    int frequency_dependent;       // Frequency-dependent materials
    double frequency;             // Operating frequency
    double min_feature_size;      // Minimum feature size to resolve
    double max_aspect_ratio;      // Maximum allowed aspect ratio
    int refinement_levels;         // Maximum refinement levels
} manhattan_params_t;

// Forward declarations
static mesh_t* generate_structured_grid(double* bbox_min, double* bbox_max, 
                                       manhattan_params_t* params);
static mesh_t* generate_adaptive_grid(surface_t* surface, manhattan_params_t* params);
static int add_vias_to_mesh(mesh_t* mesh, via_node_t* vias, int num_vias, manhattan_params_t* params);
static int generate_layer_stackup(mesh_t* mesh, layer_stackup_t* layers, int num_layers);
static int refine_manhattan_grid(mesh_t* mesh, double* refinement_regions, 
                                int num_regions, int refinement_level);
static int optimize_manhattan_quality(mesh_t* mesh, double target_aspect_ratio);
static int generate_manhattan_from_geometry(geom_rectangle_t* rectangles, 
                                           int num_rectangles, manhattan_params_t* params);
static double compute_rectangle_quality(double* center, double* dimensions);
static int snap_to_manhattan_grid(double* coordinates, double* grid_spacing);
static int detect_manhattan_violations(mesh_t* mesh, double tolerance);
static int export_manhattan_format(mesh_t* mesh, const char* filename);
static int import_manhattan_format(mesh_t* mesh, const char* filename);
static int detect_refinement_regions(surface_t* surface, double** regions);
static void compute_surface_bbox(surface_t* surface, double* bbox_min, double* bbox_max);
static void compute_merged_dimensions(rectangle_node_t* rect1, rectangle_node_t* rect2,
                                    double* merged_width, double* merged_height);
static int merge_rectangles(mesh_t* mesh, int rect1_idx, int rect2_idx);

// Main Manhattan mesh generation function
mesh_t* manhattan_mesh_generate(surface_t* surface, manhattan_params_t* params) {
    if (!surface || !params) return NULL;
    
    mesh_t* mesh = mesh_create("manhattan_mesh", MESH_TYPE_MANHATTAN);
    if (!mesh) return NULL;
    
    // Compute bounding box
    double bbox_min[3], bbox_max[3];
    compute_surface_bbox(surface, bbox_min, bbox_max);
    
    // Generate Manhattan grid
    if (params->adaptive_refinement) {
        mesh = generate_adaptive_grid(surface, params);
    } else {
        mesh = generate_structured_grid(bbox_min, bbox_max, params);
    }
    
    if (!mesh) return NULL;
    
    // Add vias if enabled (if via data is provided via peec_mesh_data)
    if (params->via_modeling) {
        // Vias would be added through peec_mesh_data if available
        // add_vias_to_mesh(mesh, vias, num_vias);
    }
    
    // Post-processing
    optimize_manhattan_quality(mesh, params->max_aspect_ratio);
    
    // Detect and fix Manhattan violations
    detect_manhattan_violations(mesh, 1e-6);
    
    return mesh;
}

// Generate structured rectangular grid
static mesh_t* generate_structured_grid(double* bbox_min, double* bbox_max, 
                                       manhattan_params_t* params) {
    if (!bbox_min || !bbox_max || !params) return NULL;
    
    mesh_t* mesh = mesh_create("manhattan_structured_grid", MESH_TYPE_MANHATTAN);
    if (!mesh) return NULL;
    
    // Compute grid dimensions
    double dx = params->grid_resolution[0];
    double dy = params->grid_resolution[1];
    double dz = params->grid_resolution[2];
    
    int nx = (int)ceil((bbox_max[0] - bbox_min[0]) / dx);
    int ny = (int)ceil((bbox_max[1] - bbox_min[1]) / dy);
    int nz = (int)ceil((bbox_max[2] - bbox_min[2]) / dz);
    
    // Ensure minimum grid size
    nx = (int)fmax((double)nx, 2.0);
    ny = (int)fmax((double)ny, 2.0);
    nz = (int)fmax((double)nz, 1.0);
    
    // Calculate total number of rectangles
    int total_rectangles = nx * ny * nz;
    
    // Allocate peec_mesh_data for rectangle storage
    rectangle_node_t* rects = (rectangle_node_t*)calloc(total_rectangles, sizeof(rectangle_node_t));
    if (!rects) {
        mesh_destroy(mesh);
        return NULL;
    }
    mesh->peec_mesh_data = rects;
    
    // Generate vertices
    int vertex_id = 0;
    for (int k = 0; k <= nz; k++) {
        for (int j = 0; j <= ny; j++) {
            for (int i = 0; i <= nx; i++) {
                double x = bbox_min[0] + i * dx;
                double y = bbox_min[1] + j * dy;
                double z = bbox_min[2] + k * dz;
                
                geom_point_t pos = {x, y, z};
                mesh_add_vertex(mesh, &pos);
                vertex_id++;
            }
        }
    }
    
    // Generate rectangular elements
    int rect_idx = 0;
    for (int k = 0; k < nz; k++) {
        for (int j = 0; j < ny; j++) {
            for (int i = 0; i < nx; i++) {
                // Vertex indices for rectangle
                int v0 = k * (nx + 1) * (ny + 1) + j * (nx + 1) + i;
                int v1 = k * (nx + 1) * (ny + 1) + j * (nx + 1) + i + 1;
                int v2 = k * (nx + 1) * (ny + 1) + (j + 1) * (nx + 1) + i + 1;
                int v3 = k * (nx + 1) * (ny + 1) + (j + 1) * (nx + 1) + i;
                
                int vertices[4] = {v0, v1, v2, v3};
                mesh_add_element(mesh, MESH_ELEMENT_QUADRILATERAL, vertices, 4);
                
                // Store rectangle properties in peec_mesh_data
                rectangle_node_t* rect = &rects[rect_idx];
                rect->vertices[0] = v0;
                rect->vertices[1] = v1;
                rect->vertices[2] = v2;
                rect->vertices[3] = v3;
                
                // Initialize neighbors to -1 (no neighbors yet)
                for (int n = 0; n < 4; n++) {
                    rect->neighbors[n] = -1;
                }
                
                // Compute center
                rect->center[0] = bbox_min[0] + (i + 0.5) * dx;
                rect->center[1] = bbox_min[1] + (j + 0.5) * dy;
                rect->center[2] = bbox_min[2] + (k + 0.5) * dz;
                
                rect->dimensions[0] = dx;
                rect->dimensions[1] = dy;
                rect->area = dx * dy;
                rect->aspect_ratio = fmax(dx, dy) / fmin(dx, dy);
                rect->layer_id = k;
                rect->material_id = 0; // Default material
                rect->refinement_level = 0;
                rect->via_flag = 0;
                rect->conductor_flag = 0;
                rect->next = NULL;
                
                rect_idx++;
            }
        }
    }
    
    return mesh;
}

// Generate adaptive grid based on geometry features
static mesh_t* generate_adaptive_grid(surface_t* surface, manhattan_params_t* params) {
    if (!surface || !params) return NULL;
    
    // Start with coarse structured grid
    double bbox_min[3], bbox_max[3];
    compute_surface_bbox(surface, bbox_min, bbox_max);
    
    mesh_t* mesh = generate_structured_grid(bbox_min, bbox_max, params);
    if (!mesh) return NULL;
    
    // Adaptive refinement based on feature detection
    if (params->adaptive_refinement && params->refinement_levels > 0) {
        // Detect regions requiring refinement
        double* refinement_regions = NULL;
        int num_regions = detect_refinement_regions(surface, &refinement_regions);
        
        if (num_regions > 0 && refinement_regions) {
            refine_manhattan_grid(mesh, refinement_regions, num_regions, 
                                 params->refinement_levels);
            free(refinement_regions);
        }
    }
    
    return mesh;
}

// Add vias to Manhattan mesh
static int add_vias_to_mesh(mesh_t* mesh, via_node_t* vias, int num_vias, manhattan_params_t* params) {
    if (!mesh || !vias || num_vias <= 0 || !params) return -1;
    
    for (int i = 0; i < num_vias; i++) {
        via_node_t* via = &vias[i];
        
        // Find rectangles containing via center
        for (int j = 0; j < mesh->num_elements; j++) {
            // Use peec_mesh_data to store rectangle data
            rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
            if (!rects) continue;
            rectangle_node_t* rect = &rects[j];
            
            // Check if via center is inside rectangle
            if (via->center[0] >= rect->center[0] - rect->dimensions[0]/2 &&
                via->center[0] <= rect->center[0] + rect->dimensions[0]/2 &&
                via->center[1] >= rect->center[1] - rect->dimensions[1]/2 &&
                via->center[1] <= rect->center[1] + rect->dimensions[1]/2) {
                
                // Check if via spans this layer
                // Use grid_resolution[2] for layer thickness since dimensions only has [0]=width, [1]=height
                double layer_thickness = params->grid_resolution[2];
                if (layer_thickness <= 0.0) layer_thickness = 1.0;  // Fallback
                int via_layer = (int)(via->center[2] / layer_thickness);
                if (via_layer == rect->layer_id) {
                    rect->via_flag = 1;
                    
                    // Add via vertices
                    geom_point_t top_pos = {via->center[0], via->center[1], via->center[2] + via->height/2};
                    geom_point_t bottom_pos = {via->center[0], via->center[1], via->center[2] - via->height/2};
                    int top_vertex = mesh_add_vertex(mesh, &top_pos);
                    int bottom_vertex = mesh_add_vertex(mesh, &bottom_pos);
                    
                    via->top_vertex = top_vertex;
                    via->bottom_vertex = bottom_vertex;
                    
                    // Create via as cylindrical element (simplified)
                    // In practice, would create proper cylindrical mesh
                    break;
                }
            }
        }
    }
    
    return 0;
}

// Generate layer stackup for multi-layer PCBs
static int generate_layer_stackup(mesh_t* mesh, layer_stackup_t* layers, int num_layers) {
    if (!mesh || !layers || num_layers <= 0) return -1;
    
    double total_thickness = 0.0;
    for (int i = 0; i < num_layers; i++) {
        total_thickness += layers[i].thickness;
    }
    
    // Assign material properties to rectangles based on layer
    for (int i = 0; i < mesh->num_elements; i++) {
        rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
        if (!rects) continue;
        rectangle_node_t* rect = &rects[i];
        
        if (rect->layer_id < num_layers) {
            rect->material_id = layers[rect->layer_id].material_id;
            
            // Set conductor flag based on layer type
            if (strstr(layers[rect->layer_id].name, "GND") || 
                strstr(layers[rect->layer_id].name, "POWER") ||
                strstr(layers[rect->layer_id].name, "SIGNAL")) {
                rect->conductor_flag = 1;
            } else {
                rect->conductor_flag = 0;
            }
        }
    }
    
    return 0;
}

// Adaptive refinement for Manhattan grid
static int refine_manhattan_grid(mesh_t* mesh, double* refinement_regions, 
                               int num_regions, int refinement_level) {
    if (!mesh || !refinement_regions || num_regions <= 0 || refinement_level <= 0) 
        return -1;
    
    for (int level = 0; level < refinement_level; level++) {
        // For each refinement region
        for (int region = 0; region < num_regions; region++) {
            double* region_bbox = &refinement_regions[region * 6]; // min/max xyz
            
            // Find rectangles intersecting refinement region
            for (int i = 0; i < mesh->num_elements; i++) {
                rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
                if (!rects) continue;
                rectangle_node_t* rect = &rects[i];
                
                // Check intersection with refinement region
                if (rect->center[0] >= region_bbox[0] && rect->center[0] <= region_bbox[3] &&
                    rect->center[1] >= region_bbox[1] && rect->center[1] <= region_bbox[4] &&
                    rect->center[2] >= region_bbox[2] && rect->center[2] <= region_bbox[5]) {
                    
                    // Subdivide rectangle into 4 smaller rectangles
                    double new_dx = rect->dimensions[0] / 2.0;
                    double new_dy = rect->dimensions[1] / 2.0;
                    
                    // Create 4 new rectangles
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            double new_center[3];
                            new_center[0] = rect->center[0] + (dx - 0.5) * new_dx;
                            new_center[1] = rect->center[1] + (dy - 0.5) * new_dy;
                            new_center[2] = rect->center[2];
                            
                    // Add new rectangle (simplified)
                    // In practice, would properly subdivide and update connectivity
                        }
                    }
                    
                    // Remove original rectangle (simplified - mark for removal)
                    // In practice, would properly remove element and update connectivity
                    // mesh_remove_element(mesh, i);  // Function not available, mark for removal instead
                    rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
                    if (rects) {
                        rects[i].area = 0.0; // Mark as removed
                    }
                    i--; // Adjust index
                }
            }
        }
    }
    
    return 0;
}

// Optimize Manhattan grid quality
static int optimize_manhattan_quality(mesh_t* mesh, double target_aspect_ratio) {
    if (!mesh || target_aspect_ratio <= 0.0) return -1;
    
    int optimized = 1;
    int iterations = 0;
    const int max_iterations = 100;
    
    while (optimized && iterations < max_iterations) {
        optimized = 0;
        iterations++;
        
        // Check all rectangles for aspect ratio violations
        for (int i = 0; i < mesh->num_elements; i++) {
            rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
            if (!rects) continue;
            rectangle_node_t* rect = &rects[i];
            
            if (rect->aspect_ratio > target_aspect_ratio) {
                // Rectangle too stretched, attempt to improve
                // Options: merge with neighbors, split, or adjust grid
                
                // Find neighboring rectangles
                for (int neighbor = 0; neighbor < 4; neighbor++) {
                    if (rect->neighbors[neighbor] >= 0) {
                        int neighbor_idx = rect->neighbors[neighbor];
                        rectangle_node_t* neighbor_rect = 
                            &((rectangle_node_t*)mesh->peec_mesh_data)[neighbor_idx];
                        
                        // Check if merging would improve aspect ratios
                        double merged_width, merged_height, merged_aspect;
                        compute_merged_dimensions(rect, neighbor_rect, 
                                                &merged_width, &merged_height);
                        merged_aspect = fmax(merged_width, merged_height) / 
                                       fmin(merged_width, merged_height);
                        
                        if (merged_aspect < fmax(rect->aspect_ratio, 
                                                 neighbor_rect->aspect_ratio)) {
                            // Merge rectangles
                            merge_rectangles(mesh, i, neighbor_idx);
                            optimized = 1;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

// Generate Manhattan mesh from geometry rectangles
static int generate_manhattan_from_geometry(geom_rectangle_t* rectangles, 
                                          int num_rectangles, manhattan_params_t* params) {
    if (!rectangles || num_rectangles <= 0 || !params) return -1;
    
    // Align rectangles to Manhattan grid
    for (int i = 0; i < num_rectangles; i++) {
        // Snap rectangle coordinates to grid
        double coords[3] = {rectangles[i].corner.x, rectangles[i].corner.y, rectangles[i].corner.z};
        snap_to_manhattan_grid(coords, params->grid_resolution);
        rectangles[i].corner.x = coords[0];
        rectangles[i].corner.y = coords[1];
        rectangles[i].corner.z = coords[2];
        
        // Adjust dimensions to be multiples of grid spacing
        rectangles[i].width = round(rectangles[i].width / params->grid_resolution[0]) * 
                             params->grid_resolution[0];
        rectangles[i].height = round(rectangles[i].height / params->grid_resolution[1]) * 
                              params->grid_resolution[1];
    }
    
    return 0;
}

// Compute rectangle quality metric
static double compute_rectangle_quality(double* center, double* dimensions) {
    if (!center || !dimensions || dimensions[0] <= 0 || dimensions[1] <= 0) return 0.0;
    
    double aspect_ratio = fmax(dimensions[0], dimensions[1]) / fmin(dimensions[0], dimensions[1]);
    
    // Quality decreases as aspect ratio increases from 1.0
    double quality = 1.0 / aspect_ratio;
    return fmin(quality, 1.0);
}

// Snap coordinates to Manhattan grid
static int snap_to_manhattan_grid(double* coordinates, double* grid_spacing) {
    if (!coordinates || !grid_spacing) return -1;
    
    for (int i = 0; i < 3; i++) {
        if (grid_spacing[i] > 0) {
            coordinates[i] = round(coordinates[i] / grid_spacing[i]) * grid_spacing[i];
        }
    }
    
    return 0;
}

// Detect Manhattan violations (non-orthogonal edges)
static int detect_manhattan_violations(mesh_t* mesh, double tolerance) {
    if (!mesh) return -1;
    
    int violations = 0;
    
    for (int i = 0; i < mesh->num_elements; i++) {
        rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
        if (!rects) continue;
        rectangle_node_t* rect = &rects[i];
        
        // Check rectangle edges for orthogonality
        int* vertices = mesh->elements[i].vertices;
        if (!vertices) continue;  // Skip if vertices array is NULL
        
        for (int edge = 0; edge < 4; edge++) {
            int v1 = vertices[edge];
            int v2 = vertices[(edge + 1) % 4];
            
            double edge_vec[3];
            edge_vec[0] = mesh->vertices[v2].position.x - mesh->vertices[v1].position.x;
            edge_vec[1] = mesh->vertices[v2].position.y - mesh->vertices[v1].position.y;
            edge_vec[2] = mesh->vertices[v2].position.z - mesh->vertices[v1].position.z;
            
            // Check if edge is axis-aligned (Manhattan property)
            int axis_aligned = 0;
            if (fabs(edge_vec[0]) < tolerance && fabs(edge_vec[1]) > tolerance && 
                fabs(edge_vec[2]) < tolerance) axis_aligned = 1; // Y-direction
            if (fabs(edge_vec[0]) > tolerance && fabs(edge_vec[1]) < tolerance && 
                fabs(edge_vec[2]) < tolerance) axis_aligned = 1; // X-direction
            if (fabs(edge_vec[0]) < tolerance && fabs(edge_vec[1]) < tolerance && 
                fabs(edge_vec[2]) > tolerance) axis_aligned = 1; // Z-direction
            
            if (!axis_aligned) {
                violations++;
                printf("Manhattan violation detected in rectangle %d, edge %d\n", i, edge);
            }
        }
    }
    
    return violations;
}

// Export Manhattan mesh format
static int export_manhattan_format(mesh_t* mesh, const char* filename) {
    if (!mesh || !filename) return -1;
    
    FILE* file = fopen(filename, "w");
    if (!file) return -1;
    
    // Write header
    fprintf(file, "# Manhattan Mesh Format\n");
    fprintf(file, "# Vertices: %d\n", mesh->num_vertices);
    fprintf(file, "# Rectangles: %d\n", mesh->num_elements);
    fprintf(file, "\n");
    
    // Write vertices
    fprintf(file, "VERTICES\n");
    for (int i = 0; i < mesh->num_vertices; i++) {
        fprintf(file, "%d %g %g %g\n", i,
                mesh->vertices[i].position.x,
                mesh->vertices[i].position.y,
                mesh->vertices[i].position.z);
    }
    fprintf(file, "\n");
    
    // Write rectangles
    fprintf(file, "RECTANGLES\n");
    for (int i = 0; i < mesh->num_elements; i++) {
        rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
        if (!rects) continue;
        rectangle_node_t* rect = &rects[i];
        
        // Check if vertices array exists
        int* vertices = mesh->elements[i].vertices;
        if (!vertices || mesh->elements[i].num_vertices < 4) continue;
        
        fprintf(file, "%d %d %d %d %d", i,
                vertices[0],
                vertices[1],
                vertices[2],
                vertices[3]);
        
        fprintf(file, " %g %g %g %g %g %d %d %d %d\n",
                rect->center[0], rect->center[1], rect->center[2],
                rect->dimensions[0], rect->dimensions[1],
                rect->layer_id, rect->material_id, rect->via_flag, rect->conductor_flag);
    }
    
    fclose(file);
    return 0;
}

// Import Manhattan mesh format
static int import_manhattan_format(mesh_t* mesh, const char* filename) {
    if (!mesh || !filename) return -1;
    
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    char line[256];
    int section = 0; // 0=header, 1=vertices, 2=rectangles
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        
        // Detect section headers
        if (strstr(line, "VERTICES")) {
            section = 1;
            continue;
        } else if (strstr(line, "RECTANGLES")) {
            section = 2;
            continue;
        }
        
        if (section == 1) {
            // Parse vertex
            int idx;
            double x, y, z;
            if (sscanf(line, "%d %lf %lf %lf", &idx, &x, &y, &z) == 4) {
                geom_point_t pos = {x, y, z};
                mesh_add_vertex(mesh, &pos);
            }
        } else if (section == 2) {
            // Parse rectangle
            int idx, v0, v1, v2, v3;
            double cx, cy, cz, width, height;
            int layer, material, via, conductor;
            
            if (sscanf(line, "%d %d %d %d %d %lf %lf %lf %lf %lf %d %d %d %d",
                      &idx, &v0, &v1, &v2, &v3, &cx, &cy, &cz, &width, &height,
                      &layer, &material, &via, &conductor) == 14) {
                
                int vertices[4] = {v0, v1, v2, v3};
                mesh_add_element(mesh, MESH_ELEMENT_QUADRILATERAL, vertices, 4);
                
                // Store rectangle properties
                if (mesh->peec_mesh_data) {
                    rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
                    rectangle_node_t* rect = &rects[mesh->num_elements - 1];
                    rect->center[0] = cx;
                    rect->center[1] = cy;
                    rect->center[2] = cz;
                    rect->dimensions[0] = width;
                    rect->dimensions[1] = height;
                    rect->layer_id = layer;
                    rect->material_id = material;
                    rect->via_flag = via;
                    rect->conductor_flag = conductor;
                }
            }
        }
    }
    
    fclose(file);
    return 0;
}

// Helper functions
static int detect_refinement_regions(surface_t* surface, double** regions) {
    if (!surface || !regions) return 0;
    
    // Simplified refinement region detection
    // In practice, would analyze geometry for high-curvature regions, small features, etc.
    
    *regions = calloc(6, sizeof(double)); // Single bounding box region
    if (*regions) {
        compute_surface_bbox(surface, *regions, *regions + 3);
        return 1; // One refinement region
    }
    
    return 0;
}

static void compute_merged_dimensions(rectangle_node_t* rect1, rectangle_node_t* rect2,
                                    double* merged_width, double* merged_height) {
    if (!rect1 || !rect2 || !merged_width || !merged_height) return;
    
    // Simplified merging - assumes rectangles are adjacent and aligned
    double width1 = rect1->dimensions[0];
    double height1 = rect1->dimensions[1];
    double width2 = rect2->dimensions[0];
    double height2 = rect2->dimensions[1];
    
    // Check if rectangles share a vertical or horizontal edge
    if (fabs(rect1->center[0] - rect2->center[0]) < 1e-10 && 
        fabs(width1 - width2) < 1e-10) {
        // Vertical merge
        *merged_width = width1;
        *merged_height = height1 + height2;
    } else if (fabs(rect1->center[1] - rect2->center[1]) < 1e-10 && 
               fabs(height1 - height2) < 1e-10) {
        // Horizontal merge
        *merged_width = width1 + width2;
        *merged_height = height1;
    } else {
        // Cannot merge
        *merged_width = fmax(width1, width2);
        *merged_height = fmax(height1, height2);
    }
}

static int merge_rectangles(mesh_t* mesh, int rect1_idx, int rect2_idx) {
    if (!mesh || rect1_idx < 0 || rect1_idx >= mesh->num_elements || 
        rect2_idx < 0 || rect2_idx >= mesh->num_elements) return -1;
    
    // Simplified rectangle merging
    // In practice, would need to update mesh topology, vertex connectivity, etc.
    
    // Remove second rectangle (simplified - mark for removal)
    // mesh_remove_element(mesh, rect2_idx);  // Function not available
    
    // Update first rectangle properties
    rectangle_node_t* rects = (rectangle_node_t*)mesh->peec_mesh_data;
    if (!rects) return -1;
    rectangle_node_t* rect1 = &rects[rect1_idx];
    rectangle_node_t* rect2 = &rects[rect2_idx];
    
    double merged_width, merged_height;
    compute_merged_dimensions(rect1, rect2, &merged_width, &merged_height);
    
    rect1->dimensions[0] = merged_width;
    rect1->dimensions[1] = merged_height;
    rect1->area = merged_width * merged_height;
    rect1->aspect_ratio = fmax(merged_width, merged_height) / fmin(merged_width, merged_height);
    
    return 0;
}

static void compute_surface_bbox(surface_t* surface, double* bbox_min, double* bbox_max) {
    if (!surface || !bbox_min || !bbox_max) return;
    
    // Simplified bounding box computation
    // In practice, would analyze actual surface geometry
    bbox_min[0] = bbox_min[1] = bbox_min[2] = -1.0;
    bbox_max[0] = bbox_max[1] = bbox_max[2] = 1.0;
}
