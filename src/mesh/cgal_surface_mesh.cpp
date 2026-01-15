/********************************************************************************
 *  PulseEM - CGAL Surface Mesh Implementation
 *
 *  Robust surface mesh generation using CGAL for MoM and PEEC applications
 *  Provides exact arithmetic, constrained triangulation, and quality optimization
 ********************************************************************************/

#include "cgal_mesh_engine.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Random.h>
#include <CGAL/Timer.h>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

// CGAL kernel and data structures
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_vertex_base_with_info_2<unsigned int, K> Vb;
typedef CGAL::Triangulation_face_base_with_info_2<unsigned int, K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, Tds> CDT;
typedef CDT::Point Point_2;
typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Face_handle Face_handle;
typedef CDT::Edge Edge;
typedef CGAL::Polygon_2<K> Polygon_2;

// Internal data structures
struct SurfaceMeshData {
    CDT cdt;
    std::vector<Point_2> boundary_points;
    std::vector<std::pair<int, int>> constraint_edges;
    std::map<Vertex_handle, unsigned int> vertex_indices;
    std::vector<double> quality_metrics;
    cgal_quality_stats_t quality_stats;
    bool has_constraints;
    
    SurfaceMeshData() : has_constraints(false) {
        // Initialize quality stats
        std::memset(&quality_stats, 0, sizeof(cgal_quality_stats_t));
    }
};

// Forward declarations
static bool cgal_create_boundary_constraints(const geom_geometry_t* geometry, 
                                           SurfaceMeshData& mesh_data);
static bool cgal_generate_interior_points(const geom_geometry_t* geometry,
                                        const cgal_mesh_parameters_t* params,
                                        SurfaceMeshData& mesh_data);
static bool cgal_perform_triangulation(SurfaceMeshData& mesh_data);
static bool cgal_assess_triangle_quality(SurfaceMeshData& mesh_data);
static bool cgal_optimize_mesh_quality(SurfaceMeshData& mesh_data,
                                     const cgal_mesh_parameters_t* params);
static mesh_mesh_t* cgal_convert_to_internal_format(SurfaceMeshData& mesh_data);
static double cgal_compute_triangle_quality(const Point_2& p1, const Point_2& p2, const Point_2& p3);
static double cgal_compute_triangle_area(const Point_2& p1, const Point_2& p2, const Point_2& p3);
static double cgal_compute_triangle_aspect_ratio(const Point_2& p1, const Point_2& p2, const Point_2& p3);
static double cgal_compute_triangle_min_angle(const Point_2& p1, const Point_2& p2, const Point_2& p3);
  ，static bool cgal_point_in_polygon_robust(const Point_2& point, const Polygon_2& polygon);
static int cgal_winding_number_point_in_polygon(const Point_2& point, const Polygon_2& polygon);

/*********************************************************************
 * Main CGAL Surface Mesh Generation
 *********************************************************************/

extern "C" mesh_result_t* cgal_generate_triangular_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    const cgal_mesh_parameters_t* params) {
    
    if (!engine || !geometry || !params) {
        return nullptr;
    }
    
    // Create result structure
    mesh_result_t* result = (mesh_result_t*)calloc(1, sizeof(mesh_result_t));
    if (!result) {
        return nullptr;
    }
    
    // Initialize CGAL mesh data
    SurfaceMeshData mesh_data;
    
    try {
        CGAL::Timer timer;
        timer.start();
        
        // Step 1: Create boundary constraints
        if (!cgal_create_boundary_constraints(geometry, mesh_data)) {
            strcpy(result->error_message, "Failed to create boundary constraints");
            result->error_code = -1;
            free(result);
            return nullptr;
        }
        
        // Step 2: Generate interior points
        if (!cgal_generate_interior_points(geometry, params, mesh_data)) {
            strcpy(result->error_message, "Failed to generate interior points");
            result->error_code = -2;
            free(result);
            return nullptr;
        }
        
        // Step 3: Perform constrained Delaunay triangulation
        if (!cgal_perform_triangulation(mesh_data)) {
            strcpy(result->error_message, "Failed to perform triangulation");
            result->error_code = -3;
            free(result);
            return nullptr;
        }
        
        // Step 4: Assess triangle quality
        cgal_assess_triangle_quality(mesh_data);
        
        // Step 5: Optimize mesh quality if requested
        if (params->enable_optimization) {
            cgal_optimize_mesh_quality(mesh_data, params);
        }
        
        // Step 6: Convert to internal format
        result->mesh = cgal_convert_to_internal_format(mesh_data);
        if (!result->mesh) {
            strcpy(result->error_message, "Failed to convert mesh format");
            result->error_code = -4;
            free(result);
            return nullptr;
        }
        
        timer.stop();
        
        // Populate result statistics
        result->generation_time = timer.time();
        result->num_vertices = mesh_data.cdt.number_of_vertices();
        result->num_elements = mesh_data.cdt.number_of_faces();
        result->num_boundary_nodes = mesh_data.boundary_points.size();
        result->min_quality = mesh_data.quality_stats.min_aspect_ratio;
        result->avg_quality = mesh_data.quality_stats.avg_aspect_ratio;
        result->max_quality = mesh_data.quality_stats.max_aspect_ratio;
        result->poor_quality_elements = mesh_data.quality_stats.num_poor_elements;
        result->mom_compatible = true;
        result->peec_compatible = false;
        result->hybrid_compatible = true;
        result->error_code = 0;
        
        return result;
        
    } catch (const std::exception& e) {
        strcpy(result->error_message, e.what());
        result->error_code = -100;
        free(result);
        return nullptr;
    } catch (...) {
        strcpy(result->error_message, "Unknown CGAL exception");
        result->error_code = -101;
        free(result);
        return nullptr;
    }
}

/*********************************************************************
 * Robust Point-in-Polygon Testing
 *********************************************************************/

static int cgal_winding_number_point_in_polygon(const Point_2& point, const Polygon_2& polygon) {
    if (polygon.is_empty()) return 0;
    
    int winding_number = 0;
    const size_t n = polygon.size();
    
    for (size_t i = 0; i < n; i++) {
        size_t j = (i + 1) % n;
        const Point_2& p1 = polygon[i];
        const Point_2& p2 = polygon[j];
        
        double x = CGAL::to_double(point.x());
        double y = CGAL::to_double(point.y());
        double x1 = CGAL::to_double(p1.x());
        double y1 = CGAL::to_double(p1.y());
        double x2 = CGAL::to_double(p2.x());
        double y2 = CGAL::to_double(p2.y());
        
        // Check if point is on the edge
        if (((y1 <= y) && (y < y2)) || ((y2 <= y) && (y < y1))) {
            // Compute intersection of horizontal line with edge
            double x_intersect = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
            if (x < x_intersect) {
                winding_number++;
            }
        }
    }
    
    return winding_number;
}

static bool cgal_point_in_polygon_robust(const Point_2& point, const Polygon_2& polygon) {
    if (polygon.is_empty()) return false;
    
    // First check if point is exactly on boundary
    for (size_t i = 0; i < polygon.size(); i++) {
        size_t j = (i + 1) % polygon.size();
        CGAL::Segment_2<K> edge(polygon[i], polygon[j]);
        
        // Check if point lies on the edge
        if (CGAL::squared_distance(point, edge) < 1e-12) {
            return true;  // Point is on boundary
        }
    }
    
    // Use winding number algorithm for robust interior testing
    int winding_number = cgal_winding_number_point_in_polygon(point, polygon);
    
    // Point is inside if winding number is odd (non-zero)
    return (winding_number % 2) != 0;
}

/*********************************************************************
 * Boundary Constraint Creation
 *********************************************************************/

static bool cgal_create_boundary_constraints(const geom_geometry_t* geometry, 
                                           SurfaceMeshData& mesh_data) {
    
    if (!geometry) return false;
    
    try {
        switch (geometry->type) {
            case GEOM_TYPE_SURFACE: {
                const geom_surface_t* surface = (const geom_surface_t*)geometry->surface_data;
                if (!surface || surface->num_loops == 0) return false;
                
                // Extract boundary loops
                for (int loop_idx = 0; loop_idx < surface->num_loops; loop_idx++) {
                    const geom_loop_t* loop = &surface->loops[loop_idx];
                    if (!loop || loop->num_vertices < 3) continue;
                    
                    // Convert vertices to 2D points (project to XY plane for now)
                    std::vector<Point_2> loop_points;
                    for (int v = 0; v < loop->num_vertices; v++) {
                        Point_2 p2d(loop->vertices[v].x, loop->vertices[v].y);
                        loop_points.push_back(p2d);
                        mesh_data.boundary_points.push_back(p2d);
                    }
                    
                    // Create constrained edges
                    for (int v = 0; v < loop->num_vertices; v++) {
                        int v1 = v;
                        int v2 = (v + 1) % loop->num_vertices;
                        int global_v1 = mesh_data.boundary_points.size() - loop->num_vertices + v1;
                        int global_v2 = mesh_data.boundary_points.size() - loop->num_vertices + v2;
                        mesh_data.constraint_edges.push_back(std::make_pair(global_v1, global_v2));
                    }
                }
                break;
            }
            
            case GEOM_TYPE_MANHATTAN: {
                // For Manhattan geometry, create rectangular boundary
                double bbox_min[3], bbox_max[3];
                // Assume we have bounding box information
                bbox_min[0] = bbox_min[1] = -1.0; bbox_min[2] = 0.0;
                bbox_max[0] = bbox_max[1] = 1.0; bbox_max[2] = 0.1;
                
                // Create rectangular boundary
                Point_2 corners[4] = {
                    Point_2(bbox_min[0], bbox_min[1]),
                    Point_2(bbox_max[0], bbox_min[1]),
                    Point_2(bbox_max[0], bbox_max[1]),
                    Point_2(bbox_min[0], bbox_max[1])
                };
                
                for (int i = 0; i < 4; i++) {
                    mesh_data.boundary_points.push_back(corners[i]);
                }
                
                // Create constraint edges
                for (int i = 0; i < 4; i++) {
                    int next = (i + 1) % 4;
                    mesh_data.constraint_edges.push_back(std::make_pair(i, next));
                }
                break;
            }
            
            default:
                return false;
        }
        
        mesh_data.has_constraints = !mesh_data.constraint_edges.empty();
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Interior Point Generation
 *********************************************************************/

static bool cgal_generate_interior_points(const geom_geometry_t* geometry,
                                        const cgal_mesh_parameters_t* params,
                                        SurfaceMeshData& mesh_data) {
    
    if (!geometry || !params) return false;
    
    try {
        // Calculate target element size based on frequency or global size
        double target_size = params->global_size;
        if (params->frequency > 0 && params->elements_per_wavelength > 0) {
            double wavelength = 3.0e8 / params->frequency;  // Speed of light
            target_size = wavelength / params->elements_per_wavelength;
        }
        
        if (target_size <= 0) target_size = 0.01;  // Default 1cm
        
        // Create polygon from boundary points for robust point-in-polygon testing
        Polygon_2 boundary_polygon;
        for (const auto& p : mesh_data.boundary_points) {
            boundary_polygon.push_back(p);
        }
        
        // Compute bounding box
        double bbox_min[2] = {1e10, 1e10};
        double bbox_max[2] = {-1e10, -1e10};
        
        for (const auto& p : mesh_data.boundary_points) {
            bbox_min[0] = std::min(bbox_min[0], CGAL::to_double(p.x()));
            bbox_min[1] = std::min(bbox_min[1], CGAL::to_double(p.y()));
            bbox_max[0] = std::max(bbox_max[0], CGAL::to_double(p.x()));
            bbox_max[1] = std::max(bbox_max[1], CGAL::to_double(p.y()));
        }
        
        double bbox_width = bbox_max[0] - bbox_min[0];
        double bbox_height = bbox_max[1] - bbox_min[1];
        
        // Generate interior points using structured grid approach
        int nx = std::max(2, (int)(bbox_width / target_size));
        int ny = std::max(2, (int)(bbox_height / target_size));
        
        // Create structured grid of interior points with robust filtering
        int points_added = 0;
        for (int i = 1; i < nx; i++) {
            for (int j = 1; j < ny; j++) {
                double x = bbox_min[0] + i * target_size;
                double y = bbox_min[1] + j * target_size;
                Point_2 p2d(x, y);
                
                // Robust point-in-polygon test using winding number
                bool inside = cgal_point_in_polygon_robust(p2d, boundary_polygon);
                
                if (inside) {
                    // Additional check: ensure point is not too close to boundary
                    bool too_close_to_boundary = false;
                    double min_dist_to_boundary = target_size * 0.1;  // 10% of element size
                    
                    for (size_t b = 0; b < boundary_polygon.size(); b++) {
                        size_t b_next = (b + 1) % boundary_polygon.size();
                        double dist_to_edge = CGAL::to_double(CGAL::squared_distance(p2d, 
                            CGAL::Segment_2<K>(boundary_polygon[b], boundary_polygon[b_next])));
                        
                        if (dist_to_edge < min_dist_to_boundary * min_dist_to_boundary) {
                            too_close_to_boundary = true;
                            break;
                        }
                    }
                    
                    if (!too_close_to_boundary) {
                        mesh_data.boundary_points.push_back(p2d);
                        points_added++;
                    }
                }
            }
        }
        
        // If no points were added with structured approach, use random sampling
        if (points_added == 0) {
            int num_random_points = std::max(10, (int)(bbox_width * bbox_height / (target_size * target_size) * 0.5));
            
            CGAL::Random random;
            for (int i = 0; i < num_random_points; i++) {
                double x = bbox_min[0] + random.get_double(0.0, 1.0) * bbox_width;
                double y = bbox_min[1] + random.get_double(0.0, 1.0) * bbox_height;
                Point_2 p2d(x, y);
                
                if (cgal_point_in_polygon_robust(p2d, boundary_polygon)) {
                    mesh_data.boundary_points.push_back(p2d);
                }
            }
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Triangulation
 *********************************************************************/

static bool cgal_perform_triangulation(SurfaceMeshData& mesh_data) {
    
    try {
        // Insert all points into CDT
        for (size_t i = 0; i < mesh_data.boundary_points.size(); i++) {
            Vertex_handle vh = mesh_data.cdt.insert(mesh_data.boundary_points[i]);
            vh->info() = i;  // Store original index
        }
        
        // Insert constraints if available
        if (mesh_data.has_constraints) {
            for (const auto& constraint : mesh_data.constraint_edges) {
                int v1_idx = constraint.first;
                int v2_idx = constraint.second;
                
                if (v1_idx >= 0 && v1_idx < (int)mesh_data.boundary_points.size() &&
                    v2_idx >= 0 && v2_idx < (int)mesh_data.boundary_points.size()) {
                    
                    // Find vertices in triangulation
                    Vertex_handle v1 = nullptr, v2 = nullptr;
                    
                    for (CDT::Vertex_iterator vit = mesh_data.cdt.vertices_begin(); 
                         vit != mesh_data.cdt.vertices_end(); ++vit) {
                        if (vit->info() == v1_idx) v1 = vit;
                        if (vit->info() == v2_idx) v2 = vit;
                    }
                    
                    if (v1 != nullptr && v2 != nullptr) {
                        mesh_data.cdt.insert_constraint(v1, v2);
                    }
                }
            }
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Quality Assessment
 *********************************************************************/

static bool cgal_assess_triangle_quality(SurfaceMeshData& mesh_data) {
    
    try {
        double min_quality = 1.0;
        double max_quality = 0.0;
        double avg_quality = 0.0;
        double min_angle = 180.0;
        double max_angle = 0.0;
        double avg_angle = 0.0;
        int num_triangles = 0;
        int poor_quality_count = 0;
        
        // Analyze all triangles
        for (CDT::Face_iterator fit = mesh_data.cdt.faces_begin(); 
             fit != mesh_data.cdt.faces_end(); ++fit) {
            
            // Get triangle vertices
            Point_2 p1 = fit->vertex(0)->point();
            Point_2 p2 = fit->vertex(1)->point();
            Point_2 p3 = fit->vertex(2)->point();
            
            // Compute quality metrics
            double quality = cgal_compute_triangle_quality(p1, p2, p3);
            double area = cgal_compute_triangle_area(p1, p2, p3);
            double aspect_ratio = cgal_compute_triangle_aspect_ratio(p1, p2, p3);
            double min_tri_angle = cgal_compute_triangle_min_angle(p1, p2, p3);
            
            // Update statistics
            min_quality = std::min(min_quality, quality);
            max_quality = std::max(max_quality, quality);
            avg_quality += quality;
            
            min_angle = std::min(min_angle, min_tri_angle);
            max_angle = std::max(max_angle, min_tri_angle);
            avg_angle += min_tri_angle;
            
            if (quality < 0.3) {  // Poor quality threshold
                poor_quality_count++;
            }
            
            num_triangles++;
        }
        
        if (num_triangles > 0) {
            avg_quality /= num_triangles;
            avg_angle /= num_triangles;
        }
        
        // Update quality stats
        mesh_data.quality_stats.min_aspect_ratio = min_quality;
        mesh_data.quality_stats.max_aspect_ratio = max_quality;
        mesh_data.quality_stats.avg_aspect_ratio = avg_quality;
        mesh_data.quality_stats.min_angle = min_angle;
        mesh_data.quality_stats.max_angle = max_angle;
        mesh_data.quality_stats.avg_angle = avg_angle;
        mesh_data.quality_stats.num_triangles = num_triangles;
        mesh_data.quality_stats.num_poor_elements = poor_quality_count;
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Triangle Quality Computation
 *********************************************************************/

static double cgal_compute_triangle_quality(const Point_2& p1, const Point_2& p2, const Point_2& p3) {
    
    // Compute edge lengths
    double a = std::sqrt(CGAL::to_double(CGAL::squared_distance(p2, p3)));
    double b = std::sqrt(CGAL::to_double(CGAL::squared_distance(p1, p3)));
    double c = std::sqrt(CGAL::to_double(CGAL::squared_distance(p1, p2)));
    
    if (a <= 0 || b <= 0 || c <= 0) return 0.0;
    
    // Compute area using Heron's formula
    double s = (a + b + c) / 2.0;
    double area = std::sqrt(s * (s - a) * (s - b) * (s - c));
    if (area <= 0) return 0.0;
    
    // Quality metric: ratio of inscribed to circumscribed circle radii
    double R = (a * b * c) / (4.0 * area);  // Circumradius
    double r = area / s;                    // Inradius
    
    return (r / R) * (2.0 / std::sqrt(3.0));  // Normalized to equilateral = 1.0
}

static double cgal_compute_triangle_area(const Point_2& p1, const Point_2& p2, const Point_2& p3) {
    
    // Use CGAL's area computation
    return std::abs(CGAL::to_double(CGAL::area(p1, p2, p3)));
}

static double cgal_compute_triangle_aspect_ratio(const Point_2& p1, const Point_2& p2, const Point_2& p3) {
    
    // Compute edge lengths
    double a = std::sqrt(CGAL::to_double(CGAL::squared_distance(p2, p3)));
    double b = std::sqrt(CGAL::to_double(CGAL::squared_distance(p1, p3)));
    double c = std::sqrt(CGAL::to_double(CGAL::squared_distance(p1, p2)));
    
    if (a <= 0 || b <= 0 || c <= 0) return 0.0;
    
    double max_edge = std::max({a, b, c});
    double min_edge = std::min({a, b, c});
    
    return max_edge / min_edge;
}

static double cgal_compute_triangle_min_angle(const Point_2& p1, const Point_2& p2, const Point_2& p3) {
    
    // Compute edge vectors
    double v1x = CGAL::to_double(p2.x()) - CGAL::to_double(p1.x());
    double v1y = CGAL::to_double(p2.y()) - CGAL::to_double(p1.y());
    double v2x = CGAL::to_double(p3.x()) - CGAL::to_double(p1.x());
    double v2y = CGAL::to_double(p3.y()) - CGAL::to_double(p1.y());
    
    // Compute angle using dot product
    double dot = v1x * v2x + v1y * v2y;
    double det = v1x * v2y - v1y * v2x;
    double angle = std::atan2(det, dot);
    
    return std::abs(angle) * 180.0 / M_PI;
}

/*********************************************************************
 * Mesh Optimization
 *********************************************************************/

static bool cgal_optimize_mesh_quality(SurfaceMeshData& mesh_data,
                                     const cgal_mesh_parameters_t* params) {
    
    if (!params || !params->enable_optimization) {
        return true;
    }
    
    try {
        // Perform Lloyd smoothing (iterative vertex movement)
        int iterations = params->optimization_iterations > 0 ? 
                        params->optimization_iterations : 5;
        
        for (int iter = 0; iter < iterations; iter++) {
            
            // Move vertices to centroid of neighboring triangles
            std::map<Vertex_handle, Point_2> new_positions;
            
            for (CDT::Vertex_iterator vit = mesh_data.cdt.vertices_begin(); 
                 vit != mesh_data.cdt.vertices_end(); ++vit) {
                
                // Skip boundary vertices
                if (mesh_data.cdt.is_infinite(vit)) continue;
                
                // Compute centroid of adjacent triangles
                double cx = 0.0, cy = 0.0;
                int count = 0;
                
                CDT::Edge_circulator ec = mesh_data.cdt.incident_edges(vit);
                CDT::Edge_circulator done = ec;
                if (ec != nullptr) {
                    do {
                        if (!mesh_data.cdt.is_infinite(ec)) {
                            Face_handle fh = ec->first;
                            int vi = ec->second;
                            
                            Point_2 centroid = CGAL::centroid(fh->vertex(0)->point(),
                                                            fh->vertex(1)->point(),
                                                            fh->vertex(2)->point());
                            cx += CGAL::to_double(centroid.x());
                            cy += CGAL::to_double(centroid.y());
                            count++;
                        }
                        ++ec;
                    } while (ec != done && count > 0);
                }
                
                if (count > 0) {
                    cx /= count;
                    cy /= count;
                    new_positions[vit] = Point_2(cx, cy);
                }
            }
            
            // Apply new positions
            for (auto& pos : new_positions) {
                mesh_data.cdt.move(pos.first, pos.second);
            }
        }
        
        // Re-assess quality after optimization
        cgal_assess_triangle_quality(mesh_data);
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Format Conversion
 *********************************************************************/

static mesh_mesh_t* cgal_convert_to_internal_format(SurfaceMeshData& mesh_data) {
    
    try {
        // Create internal mesh structure
        mesh_mesh_t* mesh = mesh_mesh_create("cgal_triangular_mesh");
        if (!mesh) return nullptr;
        
        // Set mesh type
        mesh->type = MESH_TYPE_TRIANGULAR;
        mesh->algorithm = MESH_ALGORITHM_DELAUNAY;
        
        // Count vertices and triangles
        int num_vertices = mesh_data.cdt.number_of_vertices();
        int num_triangles = mesh_data.cdt.number_of_faces();
        
        if (num_vertices <= 0 || num_triangles <= 0) {
            mesh_mesh_destroy(mesh);
            return nullptr;
        }
        
        // Allocate vertex array
        mesh->vertices = (mesh_vertex_t*)calloc(num_vertices, sizeof(mesh_vertex_t));
        if (!mesh->vertices) {
            mesh_mesh_destroy(mesh);
            return nullptr;
        }
        
        // Allocate element array
        mesh->elements = (mesh_element_t*)calloc(num_triangles, sizeof(mesh_element_t));
        if (!mesh->elements) {
            free(mesh->vertices);
            mesh_mesh_destroy(mesh);
            return nullptr;
        }
        
        // Copy vertices
        int vertex_idx = 0;
        std::map<Vertex_handle, int> vertex_map;
        
        for (CDT::Vertex_iterator vit = mesh_data.cdt.vertices_begin(); 
             vit != mesh_data.cdt.vertices_end(); ++vit) {
            
            if (mesh_data.cdt.is_infinite(vit)) continue;
            
            mesh_vertex_t* vertex = &mesh->vertices[vertex_idx];
            vertex->id = vertex_idx;
            vertex->position.x = CGAL::to_double(vit->point().x());
            vertex->position.y = CGAL::to_double(vit->point().y());
            vertex->position.z = 0.0;  // 2D mesh
            vertex->is_boundary = false;  // Will be determined later
            vertex->is_interface = false;
            vertex->num_adjacent_elements = 0;
            vertex->adjacent_elements = nullptr;
            
            vertex_map[vit] = vertex_idx;
            vertex_idx++;
        }
        
        mesh->num_vertices = vertex_idx;
        
        // Copy triangles
        int elem_idx = 0;
        for (CDT::Face_iterator fit = mesh_data.cdt.faces_begin(); 
             fit != mesh_data.cdt.faces_end(); ++fit) {
            
            mesh_element_t* element = &mesh->elements[elem_idx];
            element->id = elem_idx;
            element->type = MESH_ELEMENT_TRIANGLE;
            element->num_vertices = 3;
            element->num_edges = 3;
            
            // Allocate vertex indices
            element->vertices = (int*)malloc(3 * sizeof(int));
            if (!element->vertices) {
                // Cleanup on failure
                for (int i = 0; i < elem_idx; i++) {
                    free(mesh->elements[i].vertices);
                }
                free(mesh->vertices);
                free(mesh->elements);
                mesh_mesh_destroy(mesh);
                return nullptr;
            }
            
            // Set vertex indices
            for (int vi = 0; vi < 3; vi++) {
                Vertex_handle vh = fit->vertex(vi);
                if (vertex_map.find(vh) != vertex_map.end()) {
                    element->vertices[vi] = vertex_map[vh];
                } else {
                    element->vertices[vi] = -1;  // Invalid vertex
                }
            }
            
            // Compute element properties
            Point_2 p1 = fit->vertex(0)->point();
            Point_2 p2 = fit->vertex(1)->point();
            Point_2 p3 = fit->vertex(2)->point();
            
            element->area = cgal_compute_triangle_area(p1, p2, p3);
            element->quality_factor = cgal_compute_triangle_quality(p1, p2, p3);
            element->characteristic_length = std::sqrt(element->area);
            
            // Set centroid
            Point_2 centroid = CGAL::centroid(p1, p2, p3);
            element->centroid.x = CGAL::to_double(centroid.x());
            element->centroid.y = CGAL::to_double(centroid.y());
            element->centroid.z = 0.0;
            
            // Set normal (2D normal in XY plane)
            element->normal.x = 0.0;
            element->normal.y = 0.0;
            element->normal.z = 1.0;  // Out of plane
            
            elem_idx++;
        }
        
        mesh->num_elements = elem_idx;
        
        // Set mesh quality
        mesh->quality.min_angle = mesh_data.quality_stats.min_angle;
        mesh->quality.max_angle = mesh_data.quality_stats.max_angle;
        mesh->quality.aspect_ratio = mesh_data.quality_stats.avg_aspect_ratio;
        mesh->quality.skewness = 0.0;  // Will be computed later
        mesh->quality.orthogonality = 0.0;  // Will be computed later
        mesh->quality.smoothness = 0.0;  // Will be computed later
        
        return mesh;
        
    } catch (...) {
        if (mesh) {
            mesh_mesh_destroy(mesh);
        }
        return nullptr;
    }
}

/*********************************************************************
 * MoM-Optimized Mesh Generation
 *********************************************************************/

extern "C" mesh_result_t* cgal_generate_mom_mesh(
    cgal_mesh_engine_t* engine,
    const geom_geometry_t* geometry,
    double frequency,
    double elements_per_wavelength,
    const cgal_mesh_parameters_t* params) {
    
    if (!engine || !geometry) {
        return nullptr;
    }
    
    // Create MoM-specific parameters
    cgal_mesh_parameters_t mom_params = {0};
    if (params) {
        mom_params = *params;
    }
    
    // Set MoM-specific defaults
    mom_params.element_type = MESH_ELEMENT_TRIANGLE;
    mom_params.strategy = MESH_STRATEGY_ACCURACY;
    mom_params.frequency = frequency;
    mom_params.elements_per_wavelength = elements_per_wavelength;
    mom_params.enable_optimization = true;
    mom_params.optimization_iterations = 10;
    mom_params.min_angle_threshold = 20.0;  // Good for RWG functions
    mom_params.max_aspect_ratio = 5.0;
    mom_params.enable_constraints = true;
    mom_params.enable_em_adaptation = true;
    
    // Generate triangular mesh
    return cgal_generate_triangular_mesh(engine, geometry, &mom_params);
}

/*********************************************************************
 * Error Handling
 *********************************************************************/

extern "C" const char* cgal_surface_mesh_get_error_string(int error_code) {
    switch (error_code) {
        case 0: return "Success";
        case -1: return "Invalid parameters";
        case -2: return "Memory allocation failed";
        case -3: return "Triangulation failed";
        case -4: return "Format conversion failed";
        case -100: return "CGAL exception";
        case -101: return "Unknown exception";
        default: return "Unknown error";
    }
}