/**
 * Clipper2 + Triangle 2D Constrained Triangulation Integration
 * 
 * This header provides the interface for 2D constrained triangulation using
 * Clipper2 for polygon operations and Triangle for high-quality triangulation.
 */

#ifndef CLIPPER2_TRIANGLE_2D_H
#define CLIPPER2_TRIANGLE_2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

// Boolean operation types
typedef enum {
    CLIPPER2_BOOLEAN_NONE = 0,
    CLIPPER2_BOOLEAN_UNION,
    CLIPPER2_BOOLEAN_INTERSECTION,
    CLIPPER2_BOOLEAN_DIFFERENCE,
    CLIPPER2_BOOLEAN_XOR
} clipper2_boolean_op_t;

// 2D point structure
typedef struct {
    double x, y;
} clipper2_point_2d_t;

// Polygon structure
typedef struct {
    clipper2_point_2d_t* points;
    int num_points;
} clipper2_polygon_t;

// Triangulation parameters
typedef struct {
    // Input geometry
    clipper2_polygon_t* polygons;
    int num_polygons;
    clipper2_polygon_t* holes;
    int num_holes;
    
    // Boolean operations
    clipper2_boolean_op_t boolean_operation;
    
    // Mesh quality parameters
    double min_angle;               // Minimum triangle angle in degrees
    double max_triangle_area;        // Maximum triangle area
    bool use_steiner_points;        // Allow Steiner point insertion
    int max_steiner_points;         // Maximum number of Steiner points
    
    // Scaling for numerical precision
    double scale_factor;            // Scale factor for integer operations
    
    // Output options
    bool optimize_quality;          // Enable quality optimization
    bool verbose;                   // Enable verbose output
} clipper2_triangle_2d_params_t;

// Vertex structure for triangulation results
typedef struct {
    double x, y;                  // Vertex coordinates
    int marker;                    // Vertex marker (boundary/Steiner)
    double attribute;              // Vertex attribute (if any)
} clipper2_triangle_2d_vertex_t;

// Triangle structure
typedef struct {
    int vertices[3];               // Vertex indices
    int region;                    // Region identifier
    double area;                   // Triangle area
} clipper2_triangle_2d_triangle_t;

// Segment structure (constrained edges)
typedef struct {
    int vertex1, vertex2;          // Vertex indices
    int marker;                    // Segment marker
} clipper2_triangle_2d_segment_t;

// Triangulation result
typedef struct {
    // Vertices
    clipper2_triangle_2d_vertex_t* vertices;
    int num_vertices;
    
    // Triangles
    clipper2_triangle_2d_triangle_t* triangles;
    int num_triangles;
    
    // Constrained segments
    clipper2_triangle_2d_segment_t* segments;
    int num_segments;
    
    // Quality metrics
    double min_angle;              // Minimum angle in degrees
    double max_angle;              // Maximum angle in degrees
    double avg_angle;              // Average angle in degrees
    double total_area;             // Total triangulated area
} clipper2_triangle_2d_result_t;

/**
 * Perform 2D constrained triangulation using Clipper2 + Triangle
 * 
 * @param params Input parameters including geometry and quality settings
 * @param result Output triangulation result (must be freed with clipper2_triangle_free_result)
 * @return true on success, false on failure
 */
bool clipper2_triangle_triangulate_2d(const clipper2_triangle_2d_params_t* params, 
                                    clipper2_triangle_2d_result_t* result);

/**
 * Perform boolean operations on polygons using Clipper2
 * 
 * @param polygons1 First set of input polygons
 * @param num_polygons1 Number of polygons in first set
 * @param polygons2 Second set of input polygons (optional)
 * @param num_polygons2 Number of polygons in second set (0 if not used)
 * @param operation Boolean operation to perform
 * @param result_polygons Output polygons (must be freed with clipper2_free_polygons)
 * @param num_result_polygons Number of result polygons
 * @return true on success, false on failure
 */
bool clipper2_boolean_operation(const clipper2_polygon_t* polygons1, int num_polygons1,
                               const clipper2_polygon_t* polygons2, int num_polygons2,
                               clipper2_boolean_op_t operation,
                               clipper2_polygon_t** result_polygons, int* num_result_polygons);

/**
 * Free triangulation result memory
 * 
 * @param result Triangulation result to free
 */
void clipper2_triangle_free_result(clipper2_triangle_2d_result_t* result);

/**
 * Free polygon memory from boolean operations
 * 
 * @param polygons Polygons to free
 * @param num_polygons Number of polygons
 */
void clipper2_free_polygons(clipper2_polygon_t* polygons, int num_polygons);

/**
 * Get library version string
 * 
 * @return Version string
 */
const char* clipper2_triangle_get_version(void);

/**
 * Check if Clipper2 and Triangle libraries are available
 * 
 * @return true if libraries are available, false otherwise
 */
bool clipper2_triangle_is_available(void);

/**
 * Create default triangulation parameters
 * 
 * @param params Parameters structure to initialize
 */
static inline void clipper2_triangle_default_params(clipper2_triangle_2d_params_t* params) {
    if (!params) return;
    
    memset(params, 0, sizeof(*params));
    
    // Default mesh quality parameters
    params->min_angle = 20.0;           // 20 degrees minimum angle
    params->max_triangle_area = 0.0;     // No area constraint
    params->use_steiner_points = true;    // Allow Steiner points
    params->max_steiner_points = 10000;  // Maximum Steiner points
    
    // Default scaling
    params->scale_factor = 1000.0;       // Scale for integer precision
    
    // Default output options
    params->optimize_quality = true;       // Enable quality optimization
    params->verbose = false;               // Disable verbose output
    
    // Default boolean operation
    params->boolean_operation = CLIPPER2_BOOLEAN_NONE;
}

/**
 * Calculate polygon area using shoelace formula
 * 
 * @param polygon Input polygon
 * @return Area of polygon (positive for CCW, negative for CW)
 */
static inline double clipper2_polygon_area(const clipper2_polygon_t* polygon) {
    if (!polygon || polygon->num_points < 3) return 0.0;
    
    double area = 0.0;
    for (int i = 0; i < polygon->num_points; ++i) {
        int j = (i + 1) % polygon->num_points;
        area += polygon->points[i].x * polygon->points[j].y;
        area -= polygon->points[j].x * polygon->points[i].y;
    }
    return area * 0.5;
}

bool clipper2_offset_polyline(const clipper2_point_2d_t* points,
                              int num_points,
                              double half_width,
                              double scale_factor,
                              clipper2_polygon_t** out_polygons,
                              int* out_count);

/**
 * Check if polygon is oriented counter-clockwise (positive area)
 * 
 * @param polygon Input polygon
 * @return true if CCW, false if CW
 */
static inline bool clipper2_polygon_is_ccw(const clipper2_polygon_t* polygon) {
    return clipper2_polygon_area(polygon) > 0;
}

/**
 * Reverse polygon orientation
 * 
 * @param polygon Polygon to reverse (in-place)
 */
static inline void clipper2_polygon_reverse(clipper2_polygon_t* polygon) {
    if (!polygon || polygon->num_points < 2) return;
    
    // Swap points from outside in
    for (int i = 0; i < polygon->num_points / 2; ++i) {
        int j = polygon->num_points - 1 - i;
        clipper2_point_2d_t temp = polygon->points[i];
        polygon->points[i] = polygon->points[j];
        polygon->points[j] = temp;
    }
}

#ifdef __cplusplus
}
#endif

#endif // CLIPPER2_TRIANGLE_2D_H
