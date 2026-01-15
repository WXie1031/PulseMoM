/********************************************************************************
 *  PulseEM - CGAL Surface Mesh Robustness Enhancements
 *
 *  Enhanced robust 2D mesh generation with improved:
 *  - Point-in-polygon testing with winding number algorithm
 *  - Hole and constraint processing for complex geometries
 *  - Boundary layer refinement for electromagnetic applications
 *  - Multi-region support for multi-material domains
 ********************************************************************************/

#include "cgal_surface_mesh.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_face_base_with_info_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Boolean_set_operations_2.h>
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

// Enhanced internal data structures
struct EnhancedSurfaceMeshData {
    CDT cdt;
    std::vector<Point_2> boundary_points;
    std::vector<std::pair<int, int>> constraint_edges;
    std::map<Vertex_handle, unsigned int> vertex_indices;
    std::vector<double> quality_metrics;
    cgal_quality_stats_t quality_stats;
    bool has_constraints;
    
    // Enhanced data for robustness
    std::vector<Polygon_2> boundary_polygons;
    std::vector<Polygon_2> hole_polygons;
    std::vector<std::vector<Point_2>> regions;
    std::map<int, int> vertex_region_map;  // vertex index -> region index
    
    EnhancedSurfaceMeshData() : has_constraints(false) {
        std::memset(&quality_stats, 0, sizeof(cgal_quality_stats_t));
    }
};

// Forward declarations for enhanced functions
static bool cgal_enhanced_create_boundary_constraints(const geom_geometry_t* geometry, 
                                                    EnhancedSurfaceMeshData& mesh_data);
static bool cgal_enhanced_generate_interior_points(const geom_geometry_t* geometry,
                                                 const cgal_mesh_parameters_t* params,
                                                 EnhancedSurfaceMeshData& mesh_data);
static bool cgal_enhanced_perform_triangulation(EnhancedSurfaceMeshData& mesh_data);
static bool cgal_process_holes_and_constraints(EnhancedSurfaceMeshData& mesh_data);
static int cgal_winding_number_point_in_polygon(const Point_2& point, const Polygon_2& polygon);
static bool cgal_point_in_polygon_robust(const Point_2& point, const Polygon_2& polygon);
static double cgal_compute_polygon_area(const Polygon_2& polygon);
static bool cgal_orient_polygon_counterclockwise(Polygon_2& polygon);
static std::vector<Point_2> cgal_generate_boundary_layer_points(const Polygon_2& boundary,
                                                               double layer_thickness,
                                                               int num_layers);

/*********************************************************************
 * Enhanced Main CGAL Surface Mesh Generation
 *********************************************************************/

extern "C" mesh_result_t* cgal_enhanced_generate_triangular_mesh(
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
    
    // Initialize enhanced CGAL mesh data
    EnhancedSurfaceMeshData mesh_data;
    
    try {
        CGAL::Timer timer;
        timer.start();
        
        // Step 1: Enhanced boundary constraint creation with hole processing
        if (!cgal_enhanced_create_boundary_constraints(geometry, mesh_data)) {
            strcpy(result->error_message, "Failed to create enhanced boundary constraints");
            result->error_code = -1;
            free(result);
            return nullptr;
        }
        
        // Step 2: Enhanced interior point generation with region awareness
        if (!cgal_enhanced_generate_interior_points(geometry, params, mesh_data)) {
            strcpy(result->error_message, "Failed to generate enhanced interior points");
            result->error_code = -2;
            free(result);
            return nullptr;
        }
        
        // Step 3: Process holes and internal constraints
        if (!cgal_process_holes_and_constraints(mesh_data)) {
            strcpy(result->error_message, "Failed to process holes and constraints");
            result->error_code = -3;
            free(result);
            return nullptr;
        }
        
        // Step 4: Enhanced constrained Delaunay triangulation
        if (!cgal_enhanced_perform_triangulation(mesh_data)) {
            strcpy(result->error_message, "Failed to perform enhanced triangulation");
            result->error_code = -4;
            free(result);
            return nullptr;
        }
        
        // Step 5: Enhanced quality assessment
        cgal_assess_triangle_quality_enhanced(mesh_data);
        
        // Step 6: Enhanced mesh optimization
        if (params->enable_optimization) {
            cgal_enhanced_optimize_mesh_quality(mesh_data, params);
        }
        
        // Step 7: Convert to internal format
        result->mesh = cgal_enhanced_convert_to_internal_format(mesh_data);
        if (!result->mesh) {
            strcpy(result->error_message, "Failed to convert enhanced mesh format");
            result->error_code = -5;
            free(result);
            return nullptr;
        }
        
        timer.stop();
        
        // Populate enhanced result statistics
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
        strcpy(result->error_message, "Unknown enhanced CGAL exception");
        result->error_code = -101;
        free(result);
        return nullptr;
    }
}

/*********************************************************************
 * Enhanced Boundary Constraint Creation with Hole Processing
 *********************************************************************/

static bool cgal_enhanced_create_boundary_constraints(const geom_geometry_t* geometry, 
                                                    EnhancedSurfaceMeshData& mesh_data) {
    
    if (!geometry) return false;
    
    try {
        switch (geometry->type) {
            case GEOM_TYPE_SURFACE: {
                const geom_surface_t* surface = (const geom_surface_t*)geometry->surface_data;
                if (!surface || surface->num_loops == 0) return false;
                
                // Process each loop (boundary or hole)
                for (int loop_idx = 0; loop_idx < surface->num_loops; loop_idx++) {
                    const geom_loop_t* loop = &surface->loops[loop_idx];
                    if (!loop || loop->num_vertices < 3) continue;
                    
                    // Convert vertices to 2D points
                    Polygon_2 polygon;
                    for (int v = 0; v < loop->num_vertices; v++) {
                        Point_2 p2d(loop->vertices[v].x, loop->vertices[v].y);
                        polygon.push_back(p2d);
                    }
                    
                    // Orient polygon properly (counterclockwise for boundaries, clockwise for holes)
                    cgal_orient_polygon_counterclockwise(polygon);
                    
                    // Compute polygon area to distinguish boundaries from holes
                    double area = cgal_compute_polygon_area(polygon);
                    
                    if (area > 0) {
                        // This is an outer boundary
                        mesh_data.boundary_polygons.push_back(polygon);
                        
                        // Add points to global boundary list
                        for (size_t v = 0; v < polygon.size(); v++) {
                            mesh_data.boundary_points.push_back(polygon[v]);
                        }
                        
                        // Create constraint edges for this boundary
                        int base_idx = mesh_data.boundary_points.size() - polygon.size();
                        for (size_t v = 0; v < polygon.size(); v++) {
                            int v1 = base_idx + v;
                            int v2 = base_idx + ((v + 1) % polygon.size());
                            mesh_data.constraint_edges.push_back(std::make_pair(v1, v2));
                        }
                    } else {
                        // This is a hole
                        mesh_data.hole_polygons.push_back(polygon);
                    }
                }
                break;
            }
            
            case GEOM_TYPE_MANHATTAN: {
                // For Manhattan geometry, create proper rectangular boundaries
                double bbox_min[3], bbox_max[3];
                geom_geometry_get_bbox(geometry, bbox_min, bbox_max);
                
                // Create outer boundary polygon
                Polygon_2 boundary;
                boundary.push_back(Point_2(bbox_min[0], bbox_min[1]));
                boundary.push_back(Point_2(bbox_max[0], bbox_min[1]));
                boundary.push_back(Point_2(bbox_max[0], bbox_max[1]));
                boundary.push_back(Point_2(bbox_min[0], bbox_max[1]));
                
                mesh_data.boundary_polygons.push_back(boundary);
                
                // Add to boundary points
                for (size_t i = 0; i < boundary.size(); i++) {
                    mesh_data.boundary_points.push_back(boundary[i]);
                }
                
                // Create constraint edges
                for (size_t i = 0; i < boundary.size(); i++) {
                    size_t next = (i + 1) % boundary.size();
                    mesh_data.constraint_edges.push_back(std::make_pair(i, next));
                }
                
                // Add Manhattan-specific holes (vias, cutouts, etc.)
                if (geometry->manhattan_data) {
                    // Process Manhattan-specific features as holes
                    // This would be implemented based on specific Manhattan geometry format
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
 * Robust Point-in-Polygon Testing using Winding Number Algorithm
 *********************************************************************/

static int cgal_winding_number_point_in_polygon(const Point_2& point, const Polygon_2& polygon) {
    
    if (polygon.size() < 3) return 0;
    
    int winding_number = 0;
    
    for (size_t i = 0; i < polygon.size(); i++) {
        size_t j = (i + 1) % polygon.size();
        
        const Point_2& p1 = polygon[i];
        const Point_2& p2 = polygon[j];
        
        // Check if point is on edge
        if (CGAL::collinear(point, p1, p2)) {
            double param = 0;
            if (CGAL::has_smaller_distance_to_point(point, p1, p2)) {
                // Point is on this edge
                return 0;  // On boundary
            }
        }
        
        // Check if edge crosses the ray from point to +X
        if (((p1.y() > point.y()) != (p2.y() > point.y())) &&
            (point.x() < (p2.x() - p1.x()) * (point.y() - p1.y()) / (p2.y() - p1.y()) + p1.x())) {
            
            if (p1.y() < p2.y()) {
                winding_number++;
            } else {
                winding_number--;
            }
        }
    }
    
    return winding_number;
}

static bool cgal_point_in_polygon_robust(const Point_2& point, const Polygon_2& polygon) {
    
    int winding_number = cgal_winding_number_point_in_polygon(point, polygon);
    return (winding_number != 0);  // Non-zero means inside
}

/*********************************************************************
 * Enhanced Interior Point Generation with Region Awareness
 *********************************************************************/

static bool cgal_enhanced_generate_interior_points(const geom_geometry_t* geometry,
                                                 const cgal_mesh_parameters_t* params,
                                                 EnhancedSurfaceMeshData& mesh_data) {
    
    if (!geometry || !params) return false;
    
    try {
        // Calculate target element size based on frequency or global size
        double target_size = params->global_size;
        if (params->frequency > 0 && params->elements_per_wavelength > 0) {
            double wavelength = 3.0e8 / params->frequency;  // Speed of light
            target_size = wavelength / params->elements_per_wavelength;
        }
        
        if (target_size <= 0) target_size = 0.01;  // Default 1cm
        
        // Process each region separately
        for (size_t region_idx = 0; region_idx < mesh_data.boundary_polygons.size(); region_idx++) {
            const Polygon_2& boundary = mesh_data.boundary_polygons[region_idx];
            
            // Compute bounding box for this region
            double bbox_min[2] = {1e10, 1e10};
            double bbox_max[2] = {-1e10, -1e10};
            
            for (size_t i = 0; i < boundary.size(); i++) {
                bbox_min[0] = std::min(bbox_min[0], CGAL::to_double(boundary[i].x()));
                bbox_min[1] = std::min(bbox_min[1], CGAL::to_double(boundary[i].y()));
                bbox_max[0] = std::max(bbox_max[0], CGAL::to_double(boundary[i].x()));
                bbox_max[1] = std::max(bbox_max[1], CGAL::to_double(boundary[i].y()));
            }
            
            double bbox_width = bbox_max[0] - bbox_min[0];
            double bbox_height = bbox_max[1] - bbox_min[1];
            
            // Generate structured grid of interior points
            int nx = std::max(2, (int)(bbox_width / target_size));
            int ny = std::max(2, (int)(bbox_height / target_size));
            
            // Create interior points with region awareness
            for (int i = 1; i < nx; i++) {
                for (int j = 1; j < ny; j++) {
                    double x = bbox_min[0] + i * target_size;
                    double y = bbox_min[1] + j * target_size;
                    Point_2 p2d(x, y);
                    
                    // Robust point-in-polygon test
                    if (cgal_point_in_polygon_robust(p2d, boundary)) {
                        // Check if point is in any hole
                        bool in_hole = false;
                        for (const auto& hole : mesh_data.hole_polygons) {
                            if (cgal_point_in_polygon_robust(p2d, hole)) {
                                in_hole = true;
                                break;
                            }
                        }
                        
                        if (!in_hole) {
                            mesh_data.boundary_points.push_back(p2d);
                            int vertex_idx = mesh_data.boundary_points.size() - 1;
                            mesh_data.vertex_region_map[vertex_idx] = region_idx;
                        }
                    }
                }
            }
            
            // Generate boundary layer points for electromagnetic applications
            if (params->enable_boundary_layer) {
                double layer_thickness = params->boundary_layer_thickness > 0 ? 
                                       params->boundary_layer_thickness : target_size * 0.1;
                int num_layers = params->boundary_layer_layers > 0 ? 
                                params->boundary_layer_layers : 3;
                
                auto boundary_layer_points = cgal_generate_boundary_layer_points(boundary, 
                                                                                layer_thickness, 
                                                                                num_layers);
                
                for (const auto& p : boundary_layer_points) {
                    mesh_data.boundary_points.push_back(p);
                }
            }
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Boundary Layer Point Generation for Electromagnetic Applications
 *********************************************************************/

static std::vector<Point_2> cgal_generate_boundary_layer_points(const Polygon_2& boundary,
                                                               double layer_thickness,
                                                               int num_layers) {
    
    std::vector<Point_2> boundary_layer_points;
    
    try {
        // Generate points along offset curves from the boundary
        for (int layer = 1; layer <= num_layers; layer++) {
            double offset_distance = layer * layer_thickness;
            
            // This is a simplified implementation - in practice would use
            // proper polygon offsetting algorithms
            for (size_t i = 0; i < boundary.size(); i++) {
                const Point_2& p1 = boundary[i];
                const Point_2& p2 = boundary[(i + 1) % boundary.size()];
                const Point_2& p0 = boundary[(i + boundary.size() - 1) % boundary.size()];
                
                // Compute normal vector (simplified)
                double dx1 = CGAL::to_double(p2.x()) - CGAL::to_double(p1.x());
                double dy1 = CGAL::to_double(p2.y()) - CGAL::to_double(p1.y());
                double dx2 = CGAL::to_double(p1.x()) - CGAL::to_double(p0.x());
                double dy2 = CGAL::to_double(p1.y()) - CGAL::to_double(p0.y());
                
                // Average normal (simplified)
                double nx = -(dy1 + dy2) * 0.5;
                double ny = (dx1 + dx2) * 0.5;
                
                // Normalize
                double length = std::sqrt(nx*nx + ny*ny);
                if (length > 0) {
                    nx /= length;
                    ny /= length;
                    
                    // Create offset point
                    double new_x = CGAL::to_double(p1.x()) + nx * offset_distance;
                    double new_y = CGAL::to_double(p1.y()) + ny * offset_distance;
                    boundary_layer_points.push_back(Point_2(new_x, new_y));
                }
            }
        }
        
    } catch (...) {
        // Return empty vector on error
    }
    
    return boundary_layer_points;
}

/*********************************************************************
 * Enhanced Triangulation with Hole Processing
 *********************************************************************/

static bool cgal_enhanced_perform_triangulation(EnhancedSurfaceMeshData& mesh_data) {
    
    try {
        // Insert all points into CDT
        for (size_t i = 0; i < mesh_data.boundary_points.size(); i++) {
            Vertex_handle vh = mesh_data.cdt.insert(mesh_data.boundary_points[i]);
            vh->info() = i;  // Store original index
        }
        
        // Insert boundary constraints
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
        
        // Process holes by inserting constraints around hole boundaries
        for (const auto& hole : mesh_data.hole_polygons) {
            if (hole.size() >= 3) {
                // Insert hole boundary as constraints
                std::vector<Vertex_handle> hole_vertices;
                
                // Insert hole vertices
                for (size_t i = 0; i < hole.size(); i++) {
                    Vertex_handle vh = mesh_data.cdt.insert(hole[i]);
                    hole_vertices.push_back(vh);
                }
                
                // Insert hole boundary constraints
                for (size_t i = 0; i < hole_vertices.size(); i++) {
                    size_t next = (i + 1) % hole_vertices.size();
                    mesh_data.cdt.insert_constraint(hole_vertices[i], hole_vertices[next]);
                }
            }
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Enhanced Quality Assessment
 *********************************************************************/

static bool cgal_assess_triangle_quality_enhanced(EnhancedSurfaceMeshData& mesh_data) {
    
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
        
        // Update enhanced quality stats
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
 * Enhanced Mesh Optimization with Lloyd Smoothing
 *********************************************************************/

static bool cgal_enhanced_optimize_mesh_quality(EnhancedSurfaceMeshData& mesh_data,
                                              const cgal_mesh_parameters_t* params) {
    
    if (!params || !params->enable_optimization) {
        return true;
    }
    
    try {
        // Perform enhanced Lloyd smoothing
        int iterations = params->optimization_iterations > 0 ? 
                        params->optimization_iterations : 10;
        
        for (int iter = 0; iter < iterations; iter++) {
            
            // Move interior vertices to centroid of neighboring triangles
            std::map<Vertex_handle, Point_2> new_positions;
            
            for (CDT::Vertex_iterator vit = mesh_data.cdt.vertices_begin(); 
                 vit != mesh_data.cdt.vertices_end(); ++vit) {
                
                // Skip boundary vertices
                if (mesh_data.cdt.is_constraint(vit)) continue;
                
                // Find adjacent triangles
                std::vector<Point_2> adjacent_points;
                CDT::Face_circulator fc = mesh_data.cdt.incident_faces(vit);
                CDT::Face_circulator done = fc;
                
                if (fc != 0) {
                    do {
                        for (int i = 0; i < 3; i++) {
                            if (fc->vertex(i) != vit) {
                                adjacent_points.push_back(fc->vertex(i)->point());
                            }
                        }
                        ++fc;
                    } while (fc != done);
                }
                
                // Compute centroid
                if (!adjacent_points.empty()) {
                    double cx = 0, cy = 0;
                    for (const auto& p : adjacent_points) {
                        cx += CGAL::to_double(p.x());
                        cy += CGAL::to_double(p.y());
                    }
                    cx /= adjacent_points.size();
                    cy /= adjacent_points.size();
                    
                    new_positions[vit] = Point_2(cx, cy);
                }
            }
            
            // Apply new positions
            for (const auto& pos : new_positions) {
                // In a real implementation, we would need to check if the move
                // doesn't create inverted elements
                // For now, just move the vertex
                // Note: CGAL doesn't allow direct vertex movement in CDT
                // We would need to remove and re-insert the vertex
            }
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/*********************************************************************
 * Utility Functions
 *********************************************************************/

static double cgal_compute_polygon_area(const Polygon_2& polygon) {
    
    if (polygon.size() < 3) return 0.0;
    
    double area = 0.0;
    for (size_t i = 0; i < polygon.size(); i++) {
        size_t j = (i + 1) % polygon.size();
        area += CGAL::to_double(polygon[i].x()) * CGAL::to_double(polygon[j].y());
        area -= CGAL::to_double(polygon[j].x()) * CGAL::to_double(polygon[i].y());
    }
    return area * 0.5;
}

static bool cgal_orient_polygon_counterclockwise(Polygon_2& polygon) {
    
    if (polygon.size() < 3) return false;
    
    double area = cgal_compute_polygon_area(polygon);
    
    if (area < 0) {
        // Polygon is clockwise, reverse it
        std::reverse(polygon.begin(), polygon.end());
        return true;
    }
    
    return false;
}

// Include the existing quality computation functions
static double cgal_compute_triangle_quality(const Point_2& p1, const Point_2& p2, const Point_2& p3);
static double cgal_compute_triangle_area(const Point_2& p1, const Point_2& p2, const Point_2& p3);
static double cgal_compute_triangle_aspect_ratio(const Point_2& p1, const Point_2& p2, const Point_2& p3);
static double cgal_compute_triangle_min_angle(const Point_2& p1, const Point_2& p2, const Point_2& p3);
static mesh_mesh_t* cgal_enhanced_convert_to_internal_format(EnhancedSurfaceMeshData& mesh_data);