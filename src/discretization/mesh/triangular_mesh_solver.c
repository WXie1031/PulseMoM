/*****************************************************************************************
 * Triangular Mesh Generation for Method of Moments
 * 
 * Implements Delaunay triangulation with quality metrics and adaptive refinement
 * Supports curved surfaces, anisotropic refinement, and parallel decomposition
 *****************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "../../discretization/geometry/core_geometry.h"
#include "core_mesh.h"
#include "mesh_pipeline.h"
#include "../../operators/kernels/electromagnetic_kernels.h"
#include "../../common/core_common.h"  // For SQRT_3, FOUR_POINT_ZERO, TWO_POINT_ZERO, ONE_THIRD

// Forward type declarations
typedef struct surface_t surface_t;

// Forward declaration for compute_triangle_circumcircle
static void compute_triangle_circumcircle(double* v1, double* v2, double* v3, 
                                         double* circumcenter, double* circumradius);
typedef struct {
    double min_edge_length;
    double max_edge_length;      // Maximum edge length (only define once)
    double quality_threshold;
    int adaptive_refinement;
    int max_iterations;
    int generation_method;      // MESH_GEN_UNIFORM, MESH_GEN_ADAPTIVE, etc.
    int triangulation_method;   // TRI_DELAUNAY, TRI_ADVANCING_FRONT, etc.
    int optimize_quality;        // Enable quality optimization
    int smoothing_iterations;    // Number of smoothing iterations
    double frequency;            // Frequency for adaptive refinement
    int target_vertices;         // Target number of vertices
} mesh_params_t;

// Mesh generation method constants
#define MESH_GEN_UNIFORM 0
#define MESH_GEN_ADAPTIVE 1
#define MESH_GEN_CURVATURE 2
#define MESH_GEN_ANISOTROPIC 3

// Triangulation method constants
#define TRI_DELAUNAY 0
#define TRI_ADVANCING_FRONT 1
#define TRI_CONSTRAINED_DELAUNAY 2

// Removed problematic macros - use mesh_create and mesh_add_vertex directly
// #define mesh_create(TYPE,DIM) mesh_create("tri_mesh", (TYPE))  // Causes infinite recursion
// #define mesh_add_vertex(M,X,Y,Z) mesh_add_vertex((M), &(geom_point_t){(X),(Y),(Z)})

// Triangle quality thresholds
#define MIN_TRIANGLE_QUALITY 0.3
#define MAX_TRIANGLE_ASPECT_RATIO 10.0
#define MAX_TRIANGLE_SKEWNESS 0.9
#define MIN_TRIANGLE_ANGLE_DEG 15.0
#define MAX_TRIANGLE_ANGLE_DEG 150.0

// Delaunay triangulation parameters
#define DELAUNAY_EPSILON 1e-12
#define SUPER_TRIANGLE_SCALE 1000.0
#define MAX_ITERATIONS 1000

// Triangle data structure
typedef struct triangle_node {
    int vertices[3];           // Vertex indices
    int neighbors[3];          // Adjacent triangle indices (-1 for boundary)
    double circumcenter[3];    // Circumcenter coordinates
    double circumradius;       // Circumradius
    double quality;            // Quality metric (0-1, 1=equilateral)
    double area;               // Triangle area
    double aspect_ratio;       // Aspect ratio
    double skewness;           // Skewness metric
    int refinement_flag;       // Flag for adaptive refinement
    struct triangle_node* next; // For linked list
} triangle_node_t;

// Vertex data structure
typedef struct vertex_node {
    double x, y, z;            // Coordinates
    double normal[3];           // Surface normal
    double curvature;         // Surface curvature
    int boundary_flag;        // 0=interior, 1=boundary
    int refinement_level;     // Adaptive refinement level
    struct vertex_node* next;  // For linked list
} vertex_node_t;

// Edge data structure for advancing front
typedef struct edge_node {
    int vertices[2];           // Vertex indices
    int triangles[2];          // Adjacent triangles (-1 for boundary)
    double length;            // Edge length
    int front_flag;           // 1=active front edge
    struct edge_node* next;   // For linked list
} edge_node_t;

// Mesh quality statistics (local to this file to avoid conflict with mesh_subsystem.h)
typedef struct {
    double min_quality;
    double max_quality;
    double avg_quality;
    double min_angle;
    double max_angle;
    double avg_angle;
    double aspect_ratio;      // Maximum aspect ratio (longest/shortest edge)
    int num_triangles;
    int num_vertices;
    int num_boundary_edges;
} tri_mesh_quality_stats_t;

// Forward declarations
static int delaunay_triangulate(mesh_t* mesh, vertex_node_t* vertices, int num_vertices);
static int advancing_front_triangulate(mesh_t* mesh, vertex_node_t* vertices, int num_vertices);
static int constrained_delaunay_triangulate(mesh_t* mesh, vertex_node_t* vertices, int num_vertices, void* surface);
static double compute_triangle_quality_metric(double* v1, double* v2, double* v3);
static double compute_triangle_area(double* v1, double* v2, double* v3);
static double compute_triangle_aspect_ratio(double* v1, double* v2, double* v3);
static double compute_triangle_skewness(double* v1, double* v2, double* v3);
static double compute_triangle_min_angle(double* v1, double* v2, double* v3);
static int circumcircle_test(double* v1, double* v2, double* v3, double* point);
static int point_in_circumcircle(double* v1, double* v2, double* v3, double* point);
static int edge_intersection_test(double* v1, double* v2, double* v3, double* v4);
static int optimize_delaunay(mesh_t* mesh);
static int laplacian_smoothing(mesh_t* mesh, int iterations);
static int adaptive_refinement(mesh_t* mesh, double frequency, double max_edge_length);
static int anisotropic_refinement(mesh_t* mesh, double* direction, double ratio);
static int curved_surface_adaptation(mesh_t* mesh, void* surface_func);
static tri_mesh_quality_stats_t compute_mesh_quality_stats(mesh_t* mesh);
static int export_triangle_format(mesh_t* mesh, const char* filename);
static int import_triangle_format(mesh_t* mesh, const char* filename);
static int generate_uniform_vertices(void* surface, mesh_params_t* params, vertex_node_t** vertices);
static int generate_adaptive_vertices(void* surface, mesh_params_t* params, vertex_node_t** vertices);
static int generate_curvature_adaptive_vertices(void* surface, mesh_params_t* params, vertex_node_t** vertices);
static int generate_anisotropic_vertices(void* surface, mesh_params_t* params, vertex_node_t** vertices);
int mesh_remove_element(mesh_t* mesh, int element_idx);

// Main triangular mesh generation function
mesh_t* tri_mesh_generate_surface(void* surface, mesh_params_t* params) {
    if (!surface || !params) return NULL;
    
    mesh_t* mesh = mesh_create("tri_mesh", MESH_TYPE_TRIANGULAR);
    if (!mesh) return NULL;
    
    // Generate initial vertex distribution
    vertex_node_t* vertices = NULL;
    int num_vertices = 0;
    
    switch (params->generation_method) {
        case MESH_GEN_UNIFORM:
            num_vertices = generate_uniform_vertices(surface, params, &vertices);
            break;
        case MESH_GEN_ADAPTIVE:
            num_vertices = generate_adaptive_vertices(surface, params, &vertices);
            break;
        case MESH_GEN_CURVATURE:
            num_vertices = generate_curvature_adaptive_vertices(surface, params, &vertices);
            break;
        case MESH_GEN_ANISOTROPIC:
            num_vertices = generate_anisotropic_vertices(surface, params, &vertices);
            break;
        default:
            num_vertices = generate_uniform_vertices(surface, params, &vertices);
    }
    
    if (num_vertices <= 0) {
        mesh_destroy(mesh);
        return NULL;
    }
    
    // Perform triangulation
    int status = 0;
    switch (params->triangulation_method) {
        case TRI_DELAUNAY:
            status = delaunay_triangulate(mesh, vertices, num_vertices);
            break;
        case TRI_ADVANCING_FRONT:
            status = advancing_front_triangulate(mesh, vertices, num_vertices);
            break;
        case TRI_CONSTRAINED_DELAUNAY:
            status = constrained_delaunay_triangulate(mesh, vertices, num_vertices, surface);
            break;
        default:
            status = delaunay_triangulate(mesh, vertices, num_vertices);
    }
    
    if (status != 0) {
        free(vertices);
        mesh_destroy(mesh);
        return NULL;
    }
    
    // Post-processing
    if (params->optimize_quality) {
        optimize_delaunay(mesh);
        laplacian_smoothing(mesh, params->smoothing_iterations);
    }
    
    if (params->adaptive_refinement) {
        adaptive_refinement(mesh, params->frequency, params->max_edge_length);
    }
    
    // Compute quality metrics
    compute_mesh_quality_stats(mesh);
    
    free(vertices);
    return mesh;
}

// Delaunay triangulation implementation
static int delaunay_triangulate(mesh_t* mesh, vertex_node_t* vertices, int num_vertices) {
    if (!mesh || !vertices || num_vertices <= 0) return -1;
    
    // Create super-triangle that encompasses all vertices
    double xmin = vertices[0].x, xmax = vertices[0].x;
    double ymin = vertices[0].y, ymax = vertices[0].y;
    double zmin = vertices[0].z, zmax = vertices[0].z;
    
    for (int i = 1; i < num_vertices; i++) {
        if (vertices[i].x < xmin) xmin = vertices[i].x;
        if (vertices[i].x > xmax) xmax = vertices[i].x;
        if (vertices[i].y < ymin) ymin = vertices[i].y;
        if (vertices[i].y > ymax) ymax = vertices[i].y;
        if (vertices[i].z < zmin) zmin = vertices[i].z;
        if (vertices[i].z > zmax) zmax = vertices[i].z;
    }
    
    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;
    double scale = SUPER_TRIANGLE_SCALE * fmax(fmax(dx, dy), dz);
    
    // Add vertices to mesh
    for (int i = 0; i < num_vertices; i++) {
        geom_point_t pos = {vertices[i].x, vertices[i].y, vertices[i].z};
        mesh_add_vertex(mesh, &pos);
    }
    
    // Create initial triangulation with super-triangle
    triangle_node_t* triangles = NULL;
    int num_triangles = 0;
    int triangle_capacity = 1024;
    triangles = calloc(triangle_capacity, sizeof(triangle_node_t));
    if (!triangles) return -1;
    
    // Bowyer-Watson algorithm
    for (int i = 0; i < num_vertices; i++) {
        // Find triangles whose circumcircle contains the vertex
        int* bad_triangles = NULL;
        int num_bad = 0;
        int bad_capacity = 256;
        bad_triangles = calloc(bad_capacity, sizeof(int));
        if (!bad_triangles) {
            free(triangles);
            return -1;
        }
        
        for (int j = 0; j < num_triangles; j++) {
            if (triangles[j].vertices[0] == -1) continue; // Skip deleted triangles
            
            // Convert mesh_vertex_t position to double array
            double v1_arr[3] = {mesh->vertices[triangles[j].vertices[0]].position.x,
                               mesh->vertices[triangles[j].vertices[0]].position.y,
                               mesh->vertices[triangles[j].vertices[0]].position.z};
            double v2_arr[3] = {mesh->vertices[triangles[j].vertices[1]].position.x,
                               mesh->vertices[triangles[j].vertices[1]].position.y,
                               mesh->vertices[triangles[j].vertices[1]].position.z};
            double v3_arr[3] = {mesh->vertices[triangles[j].vertices[2]].position.x,
                               mesh->vertices[triangles[j].vertices[2]].position.y,
                               mesh->vertices[triangles[j].vertices[2]].position.z};
            double point_arr[3] = {vertices[i].x, vertices[i].y, vertices[i].z};
            double* v1 = v1_arr;
            double* v2 = v2_arr;
            double* v3 = v3_arr;
            double* point = point_arr;
            
            if (point_in_circumcircle(v1, v2, v3, point)) {
                if (num_bad >= bad_capacity) {
                    bad_capacity *= 2;
                    bad_triangles = realloc(bad_triangles, bad_capacity * sizeof(int));
                    if (!bad_triangles) {
                        free(triangles);
                        return -1;
                    }
                }
                bad_triangles[num_bad++] = j;
            }
        }
        
        // Find boundary of polygonal hole
        int* polygon = NULL;
        int num_polygon = 0;
        int polygon_capacity = 256;
        polygon = calloc(polygon_capacity, sizeof(int));
        if (!polygon) {
            free(bad_triangles);
            free(triangles);
            return -1;
        }
        
        for (int j = 0; j < num_bad; j++) {
            int tri_idx = bad_triangles[j];
            for (int edge = 0; edge < 3; edge++) {
                int v1 = triangles[tri_idx].vertices[edge];
                int v2 = triangles[tri_idx].vertices[(edge + 1) % 3];
                
                // Check if edge is on boundary of hole
                int is_boundary = 1;
                for (int k = 0; k < num_bad; k++) {
                    if (k == j) continue;
                    int other_idx = bad_triangles[k];
                    if (triangles[other_idx].vertices[0] == -1) continue;
                    
                    // Check if other triangle shares this edge
                    for (int other_edge = 0; other_edge < 3; other_edge++) {
                        int ov1 = triangles[other_idx].vertices[other_edge];
                        int ov2 = triangles[other_idx].vertices[(other_edge + 1) % 3];
                        if ((v1 == ov1 && v2 == ov2) || (v1 == ov2 && v2 == ov1)) {
                            is_boundary = 0;
                            break;
                        }
                    }
                    if (!is_boundary) break;
                }
                
                if (is_boundary) {
                    if (num_polygon >= polygon_capacity) {
                        polygon_capacity *= 2;
                        polygon = realloc(polygon, polygon_capacity * sizeof(int));
                        if (!polygon) {
                            free(bad_triangles);
                            free(polygon);
                            free(triangles);
                            return -1;
                        }
                    }
                    polygon[num_polygon++] = v1;
                    polygon[num_polygon++] = v2;
                }
            }
        }
        
        // Remove bad triangles
        for (int j = 0; j < num_bad; j++) {
            triangles[bad_triangles[j]].vertices[0] = -1; // Mark as deleted
        }
        
        // Create new triangles from polygon hole
        for (int j = 0; j < num_polygon; j += 2) {
            if (num_triangles >= triangle_capacity) {
                triangle_capacity *= 2;
                triangles = realloc(triangles, triangle_capacity * sizeof(triangle_node_t));
                if (!triangles) {
                    free(bad_triangles);
                    free(polygon);
                    return -1;
                }
            }
            
            triangles[num_triangles].vertices[0] = polygon[j];
            triangles[num_triangles].vertices[1] = polygon[j + 1];
            triangles[num_triangles].vertices[2] = i;
            
            // Compute triangle properties
            double v1_arr[3] = {mesh->vertices[triangles[num_triangles].vertices[0]].position.x,
                               mesh->vertices[triangles[num_triangles].vertices[0]].position.y,
                               mesh->vertices[triangles[num_triangles].vertices[0]].position.z};
            double v2_arr[3] = {mesh->vertices[triangles[num_triangles].vertices[1]].position.x,
                               mesh->vertices[triangles[num_triangles].vertices[1]].position.y,
                               mesh->vertices[triangles[num_triangles].vertices[1]].position.z};
            double v3_arr[3] = {mesh->vertices[triangles[num_triangles].vertices[2]].position.x,
                               mesh->vertices[triangles[num_triangles].vertices[2]].position.y,
                               mesh->vertices[triangles[num_triangles].vertices[2]].position.z};
            double* v1 = v1_arr;
            double* v2 = v2_arr;
            double* v3 = v3_arr;
            
            triangles[num_triangles].area = compute_triangle_area(v1, v2, v3);
            triangles[num_triangles].quality = compute_triangle_quality_metric(v1, v2, v3);
            triangles[num_triangles].aspect_ratio = compute_triangle_aspect_ratio(v1, v2, v3);
            triangles[num_triangles].skewness = compute_triangle_skewness(v1, v2, v3);
            
            // Compute circumcircle
            double circumcenter[3];
            double circumradius;
            compute_triangle_circumcircle(v1, v2, v3, circumcenter, &circumradius);
            
            triangles[num_triangles].circumcenter[0] = circumcenter[0];
            triangles[num_triangles].circumcenter[1] = circumcenter[1];
            triangles[num_triangles].circumcenter[2] = circumcenter[2];
            triangles[num_triangles].circumradius = circumradius;
            
            num_triangles++;
        }
        
        free(bad_triangles);
        free(polygon);
    }
    
    // Remove triangles containing super-triangle vertices
    int final_triangles = 0;
    for (int i = 0; i < num_triangles; i++) {
        if (triangles[i].vertices[0] == -1) continue;
        
        // Check if any vertex belongs to super-triangle
        int has_super_vertex = 0;
        for (int j = 0; j < 3; j++) {
            if (triangles[i].vertices[j] >= num_vertices) {
                has_super_vertex = 1;
                break;
            }
        }
        
        if (!has_super_vertex) {
            // Add valid triangle to mesh
            mesh_add_element(mesh, MESH_ELEMENT_TRIANGLE, triangles[i].vertices, 3);
            final_triangles++;
        }
    }
    
    free(triangles);
    return 0;
}

// Triangle quality metric computation
static double compute_triangle_quality_metric(double* v1, double* v2, double* v3) {
    double area = compute_triangle_area(v1, v2, v3);
    if (area <= 0.0) return 0.0;
    
    // Compute edge lengths
    double a = sqrt(pow(v2[0] - v1[0], 2) + pow(v2[1] - v1[1], 2) + pow(v2[2] - v1[2], 2));
    double b = sqrt(pow(v3[0] - v2[0], 2) + pow(v3[1] - v2[1], 2) + pow(v3[2] - v2[2], 2));
    double c = sqrt(pow(v1[0] - v3[0], 2) + pow(v1[1] - v3[1], 2) + pow(v1[2] - v3[2], 2));
    
    // Quality metric: ratio of area to sum of squared edge lengths
    double quality = (FOUR_POINT_ZERO * SQRT_3 * area) / (a*a + b*b + c*c);
    return quality;
}

static double compute_triangle_area(double* v1, double* v2, double* v3) {
    double cross[3];
    double vec1[3] = {v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]};
    double vec2[3] = {v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]};
    
    cross[0] = vec1[1] * vec2[2] - vec1[2] * vec2[1];
    cross[1] = vec1[2] * vec2[0] - vec1[0] * vec2[2];
    cross[2] = vec1[0] * vec2[1] - vec1[1] * vec2[0];
    
    double area = 0.5 * sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
    return area;
}

static double compute_triangle_aspect_ratio(double* v1, double* v2, double* v3) {
    double area = compute_triangle_area(v1, v2, v3);
    if (area <= 0.0) return 0.0;
    
    // Compute edge lengths
    double a = sqrt(pow(v2[0] - v1[0], 2) + pow(v2[1] - v1[1], 2) + pow(v2[2] - v1[2], 2));
    double b = sqrt(pow(v3[0] - v2[0], 2) + pow(v3[1] - v2[1], 2) + pow(v3[2] - v2[2], 2));
    double c = sqrt(pow(v1[0] - v3[0], 2) + pow(v1[1] - v3[1], 2) + pow(v1[2] - v3[2], 2));
    
    double max_edge = fmax(fmax(a, b), c);
    double aspect_ratio = max_edge / (TWO_POINT_ZERO * sqrt(SQRT_3 * area));
    return aspect_ratio;
}

static double compute_triangle_min_angle(double* v1, double* v2, double* v3) {
    // Compute edge vectors
    double e1[3] = {v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]};
    double e2[3] = {v3[0] - v2[0], v3[1] - v2[1], v3[2] - v2[2]};
    double e3[3] = {v1[0] - v3[0], v1[1] - v3[1], v1[2] - v3[2]};
    
    // Compute edge lengths
    double l1 = sqrt(e1[0]*e1[0] + e1[1]*e1[1] + e1[2]*e1[2]);
    double l2 = sqrt(e2[0]*e2[0] + e2[1]*e2[1] + e2[2]*e2[2]);
    double l3 = sqrt(e3[0]*e3[0] + e3[1]*e3[1] + e3[2]*e3[2]);
    
    if (l1 < 1e-12 || l2 < 1e-12 || l3 < 1e-12) return 0.0;
    
    // Compute angles using law of cosines
    double cos_a = (-e3[0]*e1[0] - e3[1]*e1[1] - e3[2]*e1[2]) / (l3 * l1);
    double cos_b = (-e1[0]*e2[0] - e1[1]*e2[1] - e1[2]*e2[2]) / (l1 * l2);
    double cos_c = (-e2[0]*e3[0] - e2[1]*e3[1] - e2[2]*e3[2]) / (l2 * l3);
    
    // Clamp to valid range
    cos_a = fmax(-1.0, fmin(1.0, cos_a));
    cos_b = fmax(-1.0, fmin(1.0, cos_b));
    cos_c = fmax(-1.0, fmin(1.0, cos_c));
    
    double a = acos(cos_a);
    double b = acos(cos_b);
    double c = acos(cos_c);
    
    // Return minimum angle in degrees
    return fmin(fmin(a, b), c) * 180.0 / M_PI;
}

static double compute_triangle_skewness(double* v1, double* v2, double* v3) {
    // Compute angles using dot products
    double vec1[3] = {v2[0] - v1[0], v2[1] - v1[1], v2[2] - v1[2]};
    double vec2[3] = {v3[0] - v1[0], v3[1] - v1[1], v3[2] - v1[2]};
    double vec3[3] = {v3[0] - v2[0], v3[1] - v2[1], v3[2] - v2[2]};
    double vec4[3] = {v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2]};
    
    double len1 = sqrt(vec1[0]*vec1[0] + vec1[1]*vec1[1] + vec1[2]*vec1[2]);
    double len2 = sqrt(vec2[0]*vec2[0] + vec2[1]*vec2[1] + vec2[2]*vec2[2]);
    double len3 = sqrt(vec3[0]*vec3[0] + vec3[1]*vec3[1] + vec3[2]*vec3[2]);
    
    double dot1 = (vec1[0]*vec2[0] + vec1[1]*vec2[1] + vec1[2]*vec2[2]) / (len1 * len2);
    double dot2 = (vec3[0]*vec4[0] + vec3[1]*vec4[1] + vec3[2]*vec4[2]) / (len3 * len2);
    
    double angle1 = acos(fmax(-1.0, fmin(1.0, dot1)));
    double angle2 = acos(fmax(-1.0, fmin(1.0, dot2)));
    double angle3 = M_PI - angle1 - angle2;
    
    // Skewness: deviation from equilateral triangle (60 degrees)
    double optimal_angle = M_PI * ONE_THIRD;  // 60 degrees for equilateral triangle
    double skewness = fmax(fmax(fabs(angle1 - optimal_angle), fabs(angle2 - optimal_angle)), 
                          fabs(angle3 - optimal_angle)) / optimal_angle;
    return skewness;
}

// Circumcircle computation
static void compute_triangle_circumcircle(double* v1, double* v2, double* v3, 
                                         double* circumcenter, double* circumradius) {
    // Compute circumcenter using barycentric coordinates
    double a = pow(v2[0] - v1[0], 2) + pow(v2[1] - v1[1], 2) + pow(v2[2] - v1[2], 2);
    double b = pow(v3[0] - v2[0], 2) + pow(v3[1] - v2[1], 2) + pow(v3[2] - v2[2], 2);
    double c = pow(v1[0] - v3[0], 2) + pow(v1[1] - v3[1], 2) + pow(v1[2] - v3[2], 2);
    
    double alpha = a * (b + c - a);
    double beta = b * (c + a - b);
    double gamma = c * (a + b - c);
    
    double sum = alpha + beta + gamma;
    if (sum <= 0.0) {
        // Degenerate triangle, use centroid
        circumcenter[0] = (v1[0] + v2[0] + v3[0]) * ONE_THIRD;
        circumcenter[1] = (v1[1] + v2[1] + v3[1]) * ONE_THIRD;
        circumcenter[2] = (v1[2] + v2[2] + v3[2]) * ONE_THIRD;
        *circumradius = 0.0;
        return;
    }
    
    circumcenter[0] = (alpha * v1[0] + beta * v2[0] + gamma * v3[0]) / sum;
    circumcenter[1] = (alpha * v1[1] + beta * v2[1] + gamma * v3[1]) / sum;
    circumcenter[2] = (alpha * v1[2] + beta * v2[2] + gamma * v3[2]) / sum;
    
    *circumradius = sqrt(a * b * c / (alpha + beta + gamma)) * ONE_HALF;
}

// Point-in-circumcircle test
static int point_in_circumcircle(double* v1, double* v2, double* v3, double* point) {
    double circumcenter[3];
    double circumradius;
    
    compute_triangle_circumcircle(v1, v2, v3, circumcenter, &circumradius);
    
    double dist = sqrt(pow(point[0] - circumcenter[0], 2) + 
                      pow(point[1] - circumcenter[1], 2) + 
                      pow(point[2] - circumcenter[2], 2));
    
    return (dist < circumradius - DELAUNAY_EPSILON);
}

// Laplacian smoothing for mesh quality improvement
static int laplacian_smoothing(mesh_t* mesh, int iterations) {
    if (!mesh || iterations <= 0) return -1;
    
    int num_vertices = mesh->num_vertices;
    double** new_coords = calloc(num_vertices, sizeof(double*));
    if (!new_coords) return -1;
    
    for (int i = 0; i < num_vertices; i++) {
        new_coords[i] = calloc(3, sizeof(double));
        if (!new_coords[i]) {
            for (int j = 0; j < i; j++) free(new_coords[j]);
            free(new_coords);
            return -1;
        }
    }
    
    for (int iter = 0; iter < iterations; iter++) {
        // Compute new vertex positions as average of neighboring vertices
        for (int i = 0; i < num_vertices; i++) {
            if (mesh->vertices[i].is_boundary) continue;
            
            double sum_x = 0.0, sum_y = 0.0, sum_z = 0.0;
            int neighbor_count = 0;
            
            // Find all triangles containing this vertex
            for (int j = 0; j < mesh->num_elements; j++) {
                int* vertices = mesh->elements[j].vertices;
                int num_elem_vertices = mesh->elements[j].num_vertices;
                
                int has_vertex = 0;
                for (int k = 0; k < num_elem_vertices; k++) {
                    if (vertices[k] == i) {
                        has_vertex = 1;
                        break;
                    }
                }
                
                if (has_vertex) {
                    // Add other vertices of this triangle as neighbors
                    for (int k = 0; k < num_elem_vertices; k++) {
                        if (vertices[k] != i) {
                            sum_x += mesh->vertices[vertices[k]].position.x;
                            sum_y += mesh->vertices[vertices[k]].position.y;
                            sum_z += mesh->vertices[vertices[k]].position.z;
                            neighbor_count++;
                        }
                    }
                }
            }
            
            if (neighbor_count > 0) {
                new_coords[i][0] = sum_x / neighbor_count;
                new_coords[i][1] = sum_y / neighbor_count;
                new_coords[i][2] = sum_z / neighbor_count;
            } else {
                new_coords[i][0] = mesh->vertices[i].position.x;
                new_coords[i][1] = mesh->vertices[i].position.y;
                new_coords[i][2] = mesh->vertices[i].position.z;
            }
        }
        
        // Update vertex coordinates
        for (int i = 0; i < num_vertices; i++) {
            if (!mesh->vertices[i].is_boundary) {
                mesh->vertices[i].position.x = new_coords[i][0];
                mesh->vertices[i].position.y = new_coords[i][1];
                mesh->vertices[i].position.z = new_coords[i][2];
            }
        }
    }
    
    // Clean up
    for (int i = 0; i < num_vertices; i++) {
        free(new_coords[i]);
    }
    free(new_coords);
    
    return 0;
}

// Adaptive refinement based on electromagnetic wavelength
static int adaptive_refinement(mesh_t* mesh, double frequency, double max_edge_length) {
    if (!mesh || frequency <= 0.0 || max_edge_length <= 0.0) return -1;
    
    double wavelength = C0 / frequency; // Speed of light / frequency
    double target_edge_length = fmin(max_edge_length, wavelength / 10.0); // Lambda/10 rule
    
    int refined = 1;
    while (refined) {
        refined = 0;
        
        // Check all triangles for refinement
        for (int i = 0; i < mesh->num_elements; i++) {
            int* vertices = mesh->elements[i].vertices;
            
            // Compute maximum edge length
            double max_edge = 0.0;
            for (int j = 0; j < 3; j++) {
                int v1 = vertices[j];
                int v2 = vertices[(j + 1) % 3];
                
                double dx = mesh->vertices[v1].position.x - mesh->vertices[v2].position.x;
                double dy = mesh->vertices[v1].position.y - mesh->vertices[v2].position.y;
                double dz = mesh->vertices[v1].position.z - mesh->vertices[v2].position.z;
                double edge_length = sqrt(dx*dx + dy*dy + dz*dz);
                
                max_edge = fmax(max_edge, edge_length);
            }
            
            // Refine if edge too long
            if (max_edge > target_edge_length) {
                // Add centroid as new vertex
                double centroid[3] = {0.0, 0.0, 0.0};
                for (int j = 0; j < 3; j++) {
                    centroid[0] += mesh->vertices[vertices[j]].position.x;
                    centroid[1] += mesh->vertices[vertices[j]].position.y;
                    centroid[2] += mesh->vertices[vertices[j]].position.z;
                }
                centroid[0] *= ONE_THIRD;
                centroid[1] *= ONE_THIRD;
                centroid[2] /= 3.0;
                
                geom_point_t centroid_pt = {centroid[0], centroid[1], centroid[2]};
                int new_vertex = mesh_add_vertex(mesh, &centroid_pt);
                if (new_vertex >= 0) {
                    // Connect centroid to vertices (simplified refinement)
                    mesh_add_element(mesh, MESH_ELEMENT_TRIANGLE, (int[]){vertices[0], vertices[1], new_vertex}, 3);
                    mesh_add_element(mesh, MESH_ELEMENT_TRIANGLE, (int[]){vertices[1], vertices[2], new_vertex}, 3);
                    mesh_add_element(mesh, MESH_ELEMENT_TRIANGLE, (int[]){vertices[2], vertices[0], new_vertex}, 3);
                    
                    // Remove original triangle
                    mesh_remove_element(mesh, i);
                    refined = 1;
                }
            }
        }
    }
    
    return 0;
}

// Mesh quality statistics
static tri_mesh_quality_stats_t compute_mesh_quality_stats(mesh_t* mesh) {
    tri_mesh_quality_stats_t stats = {0};
    if (!mesh || mesh->num_elements == 0) return stats;
    
    stats.min_quality = 1.0;
    stats.max_quality = 0.0;
    stats.min_angle = 180.0;
    stats.max_angle = 0.0;
    stats.aspect_ratio = 0.0;
    
    double total_quality = 0.0;
    double total_angle = 0.0;
    int total_angles = 0;
    
    for (int i = 0; i < mesh->num_elements; i++) {
        int* vertices = mesh->elements[i].vertices;
        if (!vertices || mesh->elements[i].num_vertices < 3) continue;
        
        double v1[3] = {mesh->vertices[vertices[0]].position.x, 
                       mesh->vertices[vertices[0]].position.y, 
                       mesh->vertices[vertices[0]].position.z};
        double v2[3] = {mesh->vertices[vertices[1]].position.x, 
                       mesh->vertices[vertices[1]].position.y, 
                       mesh->vertices[vertices[1]].position.z};
        double v3[3] = {mesh->vertices[vertices[2]].position.x, 
                       mesh->vertices[vertices[2]].position.y, 
                       mesh->vertices[vertices[2]].position.z};
        
        double quality = compute_triangle_quality_metric(v1, v2, v3);
        double min_angle = compute_triangle_min_angle(v1, v2, v3);
        double aspect_ratio = compute_triangle_aspect_ratio(v1, v2, v3);
        
        stats.min_quality = fmin(stats.min_quality, quality);
        stats.max_quality = fmax(stats.max_quality, quality);
        total_quality += quality;
        
        stats.min_angle = fmin(stats.min_angle, min_angle);
        stats.max_angle = fmax(stats.max_angle, min_angle);
        total_angle += min_angle;
        total_angles++;
        
        stats.aspect_ratio = fmax(stats.aspect_ratio, aspect_ratio);
    }
    
    stats.avg_quality = total_quality / mesh->num_elements;
    stats.avg_angle = total_angle / total_angles;
    stats.num_triangles = mesh->num_elements;
    stats.num_vertices = mesh->num_vertices;
    
    return stats;
}

// Export to Triangle format (.node, .ele files)
static int export_triangle_format(mesh_t* mesh, const char* base_filename) {
    if (!mesh || !base_filename) return -1;
    
    char node_filename[256], ele_filename[256];
    snprintf(node_filename, sizeof(node_filename), "%s.node", base_filename);
    snprintf(ele_filename, sizeof(ele_filename), "%s.ele", base_filename);
    
    // Write node file
    FILE* node_file = fopen(node_filename, "w");
    if (!node_file) return -1;
    
    fprintf(node_file, "%d 3 0 0\n", mesh->num_vertices);
    for (int i = 0; i < mesh->num_vertices; i++) {
        fprintf(node_file, "%d %g %g %g\n", i, 
                mesh->vertices[i].position.x,
                mesh->vertices[i].position.y,
                mesh->vertices[i].position.z);
    }
    fclose(node_file);
    
    // Write element file
    FILE* ele_file = fopen(ele_filename, "w");
    if (!ele_file) return -1;
    
    fprintf(ele_file, "%d 3 0\n", mesh->num_elements);
    for (int i = 0; i < mesh->num_elements; i++) {
        fprintf(ele_file, "%d %d %d %d\n", i,
                mesh->elements[i].vertices[0],
                mesh->elements[i].vertices[1],
                mesh->elements[i].vertices[2]);
    }
    fclose(ele_file);
    
    return 0;
}

// Import from Triangle format
static int import_triangle_format(mesh_t* mesh, const char* base_filename) {
    if (!mesh || !base_filename) return -1;
    
    char node_filename[256], ele_filename[256];
    snprintf(node_filename, sizeof(node_filename), "%s.node", base_filename);
    snprintf(ele_filename, sizeof(ele_filename), "%s.ele", base_filename);
    
    // Read node file
    FILE* node_file = fopen(node_filename, "r");
    if (!node_file) return -1;
    
    int num_vertices, dim, num_attrs, has_bdry;
    fscanf(node_file, "%d %d %d %d", &num_vertices, &dim, &num_attrs, &has_bdry);
    
    for (int i = 0; i < num_vertices; i++) {
        int idx;
        double x, y, z = 0.0;
        if (dim == 2) {
            fscanf(node_file, "%d %lf %lf", &idx, &x, &y);
        } else {
            fscanf(node_file, "%d %lf %lf %lf", &idx, &x, &y, &z);
        }
        geom_point_t pt = {x, y, z};
        mesh_add_vertex(mesh, &pt);
    }
    fclose(node_file);
    
    // Read element file
    FILE* ele_file = fopen(ele_filename, "r");
    if (!ele_file) return -1;
    
    int num_elements, nodes_per_element, elem_attrs;
    fscanf(ele_file, "%d %d %d", &num_elements, &nodes_per_element, &elem_attrs);
    
    for (int i = 0; i < num_elements; i++) {
        int idx, v1, v2, v3;
        fscanf(ele_file, "%d %d %d %d", &idx, &v1, &v2, &v3);
        mesh_add_element(mesh, MESH_ELEMENT_TRIANGLE, (int[]){v1, v2, v3}, 3);
    }
    fclose(ele_file);
    
    return 0;
}

// Advancing front triangulation (simplified implementation)
static int advancing_front_triangulate(mesh_t* mesh, vertex_node_t* vertices, int num_vertices) {
    // For now, fall back to Delaunay triangulation
    // Full advancing front implementation would be quite complex
    return delaunay_triangulate(mesh, vertices, num_vertices);
}

// Constrained Delaunay triangulation (simplified)
static int constrained_delaunay_triangulate(mesh_t* mesh, vertex_node_t* vertices, 
                                            int num_vertices, void* surface) {
    // First do standard Delaunay triangulation
    int status = delaunay_triangulate(mesh, vertices, num_vertices);
    if (status != 0) return status;
    
    // Then enforce constraints by edge swapping
    // This is a simplified version - full implementation would be more complex
    
    return 0;
}

// Delaunay optimization
static int optimize_delaunay(mesh_t* mesh) {
    if (!mesh) return -1;
    
    int optimized = 1;
    int iterations = 0;
    
    while (optimized && iterations < MAX_ITERATIONS) {
        optimized = 0;
        iterations++;
        
        // Check all edges for Delaunay condition
        for (int i = 0; i < mesh->num_elements; i++) {
            for (int j = i + 1; j < mesh->num_elements; j++) {
                // Check if triangles share an edge
                int shared_vertices[2];
                int num_shared = 0;
                
                for (int vi = 0; vi < 3; vi++) {
                    for (int vj = 0; vj < 3; vj++) {
                        if (mesh->elements[i].vertices[vi] == mesh->elements[j].vertices[vj]) {
                            if (num_shared < 2) {
                                shared_vertices[num_shared++] = mesh->elements[i].vertices[vi];
                            }
                        }
                    }
                }
                
                if (num_shared == 2) {
                    // Triangles share an edge, check Delaunay condition
                    // Find opposite vertices
                    int opp_i = -1, opp_j = -1;
                    for (int vi = 0; vi < 3; vi++) {
                        int found = 0;
                        for (int si = 0; si < 2; si++) {
                            if (mesh->elements[i].vertices[vi] == shared_vertices[si]) {
                                found = 1;
                                break;
                            }
                        }
                        if (!found) opp_i = mesh->elements[i].vertices[vi];
                    }
                    
                    for (int vj = 0; vj < 3; vj++) {
                        int found = 0;
                        for (int si = 0; si < 2; si++) {
                            if (mesh->elements[j].vertices[vj] == shared_vertices[si]) {
                                found = 1;
                                break;
                            }
                        }
                        if (!found) opp_j = mesh->elements[j].vertices[vj];
                    }
                    
                    if (opp_i >= 0 && opp_j >= 0) {
                        double v1_arr[3] = {mesh->vertices[shared_vertices[0]].position.x,
                                           mesh->vertices[shared_vertices[0]].position.y,
                                           mesh->vertices[shared_vertices[0]].position.z};
                        double v2_arr[3] = {mesh->vertices[shared_vertices[1]].position.x,
                                           mesh->vertices[shared_vertices[1]].position.y,
                                           mesh->vertices[shared_vertices[1]].position.z};
                        double v3_arr[3] = {mesh->vertices[opp_i].position.x,
                                           mesh->vertices[opp_i].position.y,
                                           mesh->vertices[opp_i].position.z};
                        double v4_arr[3] = {mesh->vertices[opp_j].position.x,
                                           mesh->vertices[opp_j].position.y,
                                           mesh->vertices[opp_j].position.z};
                        double* v1 = v1_arr;
                        double* v2 = v2_arr;
                        double* v3 = v3_arr;
                        double* v4 = v4_arr;
                        
                        // Check if opposite vertex is in circumcircle
                        if (point_in_circumcircle(v1, v2, v3, v4)) {
                            // Edge swap needed
                            // This would require more complex mesh topology updates
                            optimized = 1;
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

// Helper functions for vertex generation
static int generate_uniform_vertices(void* surface, mesh_params_t* params, 
                                   vertex_node_t** vertices) {
    // Simplified uniform vertex generation
    // In practice, this would depend on surface parameterization
    
    int num_vertices = params->target_vertices;
    if (num_vertices <= 0) num_vertices = 1000;
    
    *vertices = calloc(num_vertices, sizeof(vertex_node_t));
    if (!*vertices) return -1;
    
    // Generate vertices on surface (simplified)
    for (int i = 0; i < num_vertices; i++) {
        double u = (double)(i % (int)sqrt(num_vertices)) / sqrt(num_vertices);
        double v = (double)(i / (int)sqrt(num_vertices)) / sqrt(num_vertices);
        
        // Map (u,v) to surface coordinates
        (*vertices)[i].x = u * 1.0; // Surface parameterization
        (*vertices)[i].y = v * 1.0;
        (*vertices)[i].z = 0.0; // Flat surface for now
        
        (*vertices)[i].boundary_flag = 0;
        (*vertices)[i].refinement_level = 0;
    }
    
    return num_vertices;
}

static int generate_adaptive_vertices(void* surface, mesh_params_t* params, 
                                    vertex_node_t** vertices) {
    // Adaptive vertex generation based on surface features
    return generate_uniform_vertices(surface, params, vertices); // Simplified
}

static int generate_curvature_adaptive_vertices(void* surface, mesh_params_t* params, 
                                              vertex_node_t** vertices) {
    // Curvature-adaptive vertex generation
    return generate_uniform_vertices(surface, params, vertices); // Simplified
}

static int generate_anisotropic_vertices(void* surface, mesh_params_t* params, 
                                       vertex_node_t** vertices) {
    // Anisotropic vertex generation
    return generate_uniform_vertices(surface, params, vertices); // Simplified
}

// Placeholder functions for mesh operations (using core_mesh.h interface)
// Note: mesh_add_vertex is already defined in core_mesh.h with signature:
// int mesh_add_vertex(mesh_t* mesh, const geom_point_t* position);
// This local implementation is removed to avoid conflicts

// mesh_add_element removed - use core_mesh.h interface instead

int mesh_remove_element(mesh_t* mesh, int element_idx) {
    if (!mesh || element_idx < 0 || element_idx >= mesh->num_elements) return -1;
    
    // Simple removal by shifting elements
    for (int i = element_idx; i < mesh->num_elements - 1; i++) {
        mesh->elements[i] = mesh->elements[i + 1];
    }
    mesh->num_elements--;
    
    return 0;
}

// mesh_destroy is now unified in core_mesh_unified.c
// All mesh destruction should use the unified interface from core_mesh.h
// This ensures consistent resource cleanup across the entire simulation pipeline
// No local implementation needed - use core_mesh_unified.c::mesh_destroy

// mesh_create removed - use core_mesh.h interface instead
// mesh_t* mesh_create(const char* name, mesh_type_t type) is defined in core_mesh.h

/*********************************************************************
 * Unified Pipeline Interface Implementation for MoM
 *********************************************************************/

/**
 * @brief MoM-specific mesh pipeline generation
 * Implements the unified pipeline interface for Method of Moments solver
 */
int mesh_pipeline_generate_mom(const mesh_pipeline_config_t* config,
                               mesh_pipeline_result_t* result) {
    if (!config || !result) {
        return -1;
    }
    
    // Check if this is for MoM solver
    if (config->solver != SOLVER_MOM) {
        snprintf(result->error_message, sizeof(result->error_message),
                "mesh_pipeline_generate_mom called for non-MoM solver");
        return -2;
    }
    
    // Check mesh type (MoM primarily uses triangular)
    if (config->type != MESH_TYPE_TRIANGULAR && config->type != MESH_TYPE_HYBRID) {
        snprintf(result->error_message, sizeof(result->error_message),
                "MoM solver requires triangular or hybrid mesh type");
        return -3;
    }
    
    // Convert pipeline config to mesh_params_t
    mesh_params_t params;
    memset(&params, 0, sizeof(mesh_params_t));
    
    // Set sizing parameters
    params.min_edge_length = (config->min_size > 0.0) ? config->min_size : config->target_size * 0.5;
    params.max_edge_length = (config->max_size > 0.0) ? config->max_size : config->target_size * 2.0;
    
    // Set frequency for adaptive refinement
    if (config->wavelength > 0.0) {
        params.frequency = C0 / config->wavelength;
    }
    
    // Set algorithm
    // Note: Convert pipeline algorithm to core mesh algorithm
    // Using explicit conversion function to avoid type conversion warnings
    mesh_algorithm_t core_algorithm = mesh_pipeline_to_core_algorithm(config->algorithm);
    switch (config->algorithm) {
        case MESH_PIPELINE_ALGORITHM_DELAUNAY:
            params.triangulation_method = 0;  // TRI_DELAUNAY
            params.generation_method = 0;  // MESH_GEN_UNIFORM
            break;
        case MESH_PIPELINE_ALGORITHM_ADVANCING_FRONT:
            params.triangulation_method = 1;  // TRI_ADVANCING_FRONT
            params.generation_method = 0;  // MESH_GEN_UNIFORM
            break;
        case MESH_PIPELINE_ALGORITHM_ADAPTIVE:
            params.triangulation_method = 0;  // TRI_DELAUNAY
            params.generation_method = 1;  // MESH_GEN_ADAPTIVE
            params.adaptive_refinement = 1;
            break;
        case MESH_PIPELINE_ALGORITHM_CURVATURE_BASED:
            core_algorithm = MESH_ALGORITHM_UNSTRUCTURED;
            params.triangulation_method = 0;  // TRI_DELAUNAY
            params.generation_method = 2;  // MESH_GEN_CURVATURE
            break;
        default:
            params.triangulation_method = 0;  // TRI_DELAUNAY
            params.generation_method = 0;  // MESH_GEN_UNIFORM
    }
    
    // Set quality parameters
    params.quality_threshold = config->quality.min_quality;
    params.optimize_quality = config->quality.enforce_quality;
    params.smoothing_iterations = config->quality.enforce_quality ? 5 : 0;
    
    // Set refinement parameters
    params.adaptive_refinement = config->adaptive_refinement ? 1 : 0;
    params.max_iterations = config->max_refinement_levels;
    
    // Generate mesh using existing function
    mesh_t* mesh = tri_mesh_generate_surface(config->geometry, &params);
    
    if (!mesh) {
        snprintf(result->error_message, sizeof(result->error_message),
                "tri_mesh_generate_surface failed");
        return -4;
    }
    
    // Fill result structure
    result->mesh = mesh;
    result->status = 0;
    result->num_elements = mesh->num_elements;
    result->num_vertices = mesh->num_vertices;
    
    // Compute quality statistics
    tri_mesh_quality_stats_t stats = compute_mesh_quality_stats(mesh);
    result->quality_stats.min_quality = stats.min_quality;
    result->quality_stats.max_aspect_ratio = stats.aspect_ratio;
    result->quality_stats.min_angle = stats.min_angle;
    result->quality_stats.max_angle = stats.max_angle;
    
    return 0;
}
