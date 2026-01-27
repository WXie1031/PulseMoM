/**
 * Clipper2 + Triangle 2D Constrained Triangulation Integration
 * 
 * This module provides comprehensive 2D constrained triangulation capabilities
 * using the Clipper2 library for polygon operations and Triangle for high-quality
 * constrained Delaunay triangulation.
 * 
 * Features:
 * - Boolean operations on polygons (union, intersection, difference)
 * - Constrained Delaunay triangulation with quality guarantees
 * - Hole handling and internal boundary support
 * - Steiner point insertion for quality improvement
 * - Multi-region triangulation with material properties
 * - Mesh quality optimization and smoothing
 * - Integration with electromagnetic simulation requirements
 */

#include "clipper2_triangle_2d.h"
#include <clipper2/clipper.h>
#include <triangle.h>

#include <vector>
#include <map>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <limits>

// Use Clipper2 namespaces
using namespace Clipper2Lib;

// Triangle library interface
extern "C" {
    void triangulate(char*, struct triangulateio*, struct triangulateio*, struct triangulateio*);
}

// Internal data structures
struct InternalTriangulationData {
    std::vector<Point64> points;
    std::vector<std::vector<Point64>> holes;
    std::vector<std::vector<Point64>> regions;
    std::map<int, int> point_id_map;
    std::vector<double> point_z_values;
    double min_angle;
    double max_area;
    bool use_steiner_points;
    int max_steiner_points;
};

// Helper function to convert between Clipper2 and Triangle formats
static void convertClipperToTriangle(const InternalTriangulationData& data, 
                                   struct triangulateio* in, 
                                   struct triangulateio* out) {
    // Count total points
    int total_points = data.points.size();
    for (const auto& hole : data.holes) {
        total_points += hole.size();
    }
    for (const auto& region : data.regions) {
        total_points += region.size();
    }
    
    // Allocate memory for input
    in->numberofpoints = total_points;
    in->pointlist = (REAL*)malloc(total_points * 2 * sizeof(REAL));
    in->pointmarkerlist = (int*)malloc(total_points * sizeof(int));
    
    // Copy points
    int point_index = 0;
    
    // Main polygon points
    for (size_t i = 0; i < data.points.size(); ++i) {
        in->pointlist[point_index * 2] = static_cast<REAL>(data.points[i].x);
        in->pointlist[point_index * 2 + 1] = static_cast<REAL>(data.points[i].y);
        in->pointmarkerlist[point_index] = 1; // Boundary marker
        point_index++;
    }
    
    // Hole points
    for (const auto& hole : data.holes) {
        for (const auto& pt : hole) {
            in->pointlist[point_index * 2] = static_cast<REAL>(pt.x);
            in->pointlist[point_index * 2 + 1] = static_cast<REAL>(pt.y);
            in->pointmarkerlist[point_index] = 2; // Hole marker
            point_index++;
        }
    }
    
    // Region points
    for (const auto& region : data.regions) {
        for (const auto& pt : region) {
            in->pointlist[point_index * 2] = static_cast<REAL>(pt.x);
            in->pointlist[point_index * 2 + 1] = static_cast<REAL>(pt.y);
            in->pointmarkerlist[point_index] = 3; // Region marker
            point_index++;
        }
    }
    
    // Set up segments (constrained edges)
    int total_segments = 0;
    total_segments += data.points.size(); // Main polygon
    for (const auto& hole : data.holes) {
        total_segments += hole.size();
    }
    for (const auto& region : data.regions) {
        total_segments += region.size();
    }
    
    in->numberofsegments = total_segments;
    in->segmentlist = (int*)malloc(total_segments * 2 * sizeof(int));
    in->segmentmarkerlist = (int*)malloc(total_segments * sizeof(int));
    
    int segment_index = 0;
    int current_point = 0;
    
    // Main polygon segments
    for (size_t i = 0; i < data.points.size(); ++i) {
        in->segmentlist[segment_index * 2] = current_point + i;
        in->segmentlist[segment_index * 2 + 1] = current_point + ((i + 1) % data.points.size());
        in->segmentmarkerlist[segment_index] = 1; // Boundary segment
        segment_index++;
    }
    current_point += data.points.size();
    
    // Hole segments
    for (const auto& hole : data.holes) {
        for (size_t i = 0; i < hole.size(); ++i) {
            in->segmentlist[segment_index * 2] = current_point + i;
            in->segmentlist[segment_index * 2 + 1] = current_point + ((i + 1) % hole.size());
            in->segmentmarkerlist[segment_index] = 2; // Hole segment
            segment_index++;
        }
        current_point += hole.size();
    }
    
    // Region segments
    for (const auto& region : data.regions) {
        for (size_t i = 0; i < region.size(); ++i) {
            in->segmentlist[segment_index * 2] = current_point + i;
            in->segmentlist[segment_index * 2 + 1] = current_point + ((i + 1) % region.size());
            in->segmentmarkerlist[segment_index] = 3; // Region segment
            segment_index++;
        }
        current_point += region.size();
    }
    
    // Set up holes
    in->numberofholes = data.holes.size();
    if (in->numberofholes > 0) {
        in->holelist = (REAL*)malloc(in->numberofholes * 2 * sizeof(REAL));
        
        int hole_point_index = data.points.size();
        for (size_t i = 0; i < data.holes.size(); ++i) {
            // Use a point inside the hole (first point)
            if (!data.holes[i].empty()) {
                in->holelist[i * 2] = static_cast<REAL>(data.holes[i][0].x + 1.0);
                in->holelist[i * 2 + 1] = static_cast<REAL>(data.holes[i][0].y + 1.0);
            }
            hole_point_index += data.holes[i].size();
        }
    } else {
        in->holelist = nullptr;
    }
    
    // Set up regions
    in->numberofregions = data.regions.size();
    if (in->numberofregions > 0) {
        in->regionlist = (REAL*)malloc(in->numberofregions * 4 * sizeof(REAL));
        
        int region_point_index = data.points.size() + data.holes.size();
        for (size_t i = 0; i < data.regions.size(); ++i) {
            // Use a point inside the region (first point)
            if (!data.regions[i].empty()) {
                in->regionlist[i * 4] = static_cast<REAL>(data.regions[i][0].x + 1.0);
                in->regionlist[i * 4 + 1] = static_cast<REAL>(data.regions[i][0].y + 1.0);
                in->regionlist[i * 4 + 2] = static_cast<REAL>(i); // Region attribute
                in->regionlist[i * 4 + 3] = 0.0; // Area constraint
            }
        }
    } else {
        in->regionlist = nullptr;
    }
    
    // Initialize output structure
    out->pointlist = nullptr;
    out->pointattributelist = nullptr;
    out->pointmarkerlist = nullptr;
    out->trianglelist = nullptr;
    out->triangleattributelist = nullptr;
    out->neighborlist = nullptr;
    out->segmentlist = nullptr;
    out->segmentmarkerlist = nullptr;
    out->edgelist = nullptr;
    out->edgemarkerlist = nullptr;
    out->normlist = nullptr;
}

// Helper function to convert Triangle output back to our format
static void convertTriangleToOutput(const struct triangulateio* out, 
                                  clipper2_triangle_2d_result_t* result) {
    // Copy vertices
    result->num_vertices = out->numberofpoints;
    result->vertices = (clipper2_triangle_2d_vertex_t*)malloc(
        result->num_vertices * sizeof(clipper2_triangle_2d_vertex_t));
    
    for (int i = 0; i < result->num_vertices; ++i) {
        result->vertices[i].x = out->pointlist[i * 2];
        result->vertices[i].y = out->pointlist[i * 2 + 1];
        result->vertices[i].marker = out->pointmarkerlist ? out->pointmarkerlist[i] : 0;
        result->vertices[i].attribute = out->pointattributelist ? out->pointattributelist[i] : 0.0;
    }
    
    // Copy triangles
    result->num_triangles = out->numberoftriangles;
    result->triangles = (clipper2_triangle_2d_triangle_t*)malloc(
        result->num_triangles * sizeof(clipper2_triangle_2d_triangle_t));
    
    for (int i = 0; i < result->num_triangles; ++i) {
        for (int j = 0; j < 3; ++j) {
            result->triangles[i].vertices[j] = out->trianglelist[i * 3 + j];
        }
        result->triangles[i].region = out->triangleattributelist ? 
            static_cast<int>(out->triangleattributelist[i]) : 0;
        result->triangles[i].area = 0.0; // Will be calculated later
        
        // Calculate triangle area
        int v0 = result->triangles[i].vertices[0];
        int v1 = result->triangles[i].vertices[1];
        int v2 = result->triangles[i].vertices[2];
        
        if (v0 >= 0 && v1 >= 0 && v2 >= 0 && 
            v0 < result->num_vertices && v1 < result->num_vertices && v2 < result->num_vertices) {
            
            double x0 = result->vertices[v0].x;
            double y0 = result->vertices[v0].y;
            double x1 = result->vertices[v1].x;
            double y1 = result->vertices[v1].y;
            double x2 = result->vertices[v2].x;
            double y2 = result->vertices[v2].y;
            
            result->triangles[i].area = 0.5 * std::abs(
                (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0));
        }
    }
    
    // Copy segments (constrained edges)
    result->num_segments = out->numberofsegments;
    result->segments = (clipper2_triangle_2d_segment_t*)malloc(
        result->num_segments * sizeof(clipper2_triangle_2d_segment_t));
    
    for (int i = 0; i < result->num_segments; ++i) {
        result->segments[i].vertex1 = out->segmentlist[i * 2];
        result->segments[i].vertex2 = out->segmentlist[i * 2 + 1];
        result->segments[i].marker = out->segmentmarkerlist ? 
            out->segmentmarkerlist[i] : 0;
    }
    
    // Calculate quality metrics
    result->min_angle = std::numeric_limits<double>::max();
    result->max_angle = 0.0;
    result->avg_angle = 0.0;
    
    int angle_count = 0;
    for (int i = 0; i < result->num_triangles; ++i) {
        for (int j = 0; j < 3; ++j) {
            int v0 = result->triangles[i].vertices[j];
            int v1 = result->triangles[i].vertices[(j + 1) % 3];
            int v2 = result->triangles[i].vertices[(j + 2) % 3];
            
            if (v0 >= 0 && v1 >= 0 && v2 >= 0 && 
                v0 < result->num_vertices && v1 < result->num_vertices && v2 < result->num_vertices) {
                
                double x0 = result->vertices[v0].x;
                double y0 = result->vertices[v0].y;
                double x1 = result->vertices[v1].x;
                double y1 = result->vertices[v1].y;
                double x2 = result->vertices[v2].x;
                double y2 = result->vertices[v2].y;
                
                // Calculate angle at vertex v0
                double dx1 = x1 - x0;
                double dy1 = y1 - y0;
                double dx2 = x2 - x0;
                double dy2 = y2 - y0;
                
                double dot_product = dx1 * dx2 + dy1 * dy2;
                double mag1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
                double mag2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
                
                if (mag1 > 0 && mag2 > 0) {
                    double cos_angle = dot_product / (mag1 * mag2);
                    cos_angle = std::max(-1.0, std::min(1.0, cos_angle)); // Clamp
                    double angle = std::acos(cos_angle) * 180.0 / M_PI;
                    
                    result->min_angle = std::min(result->min_angle, angle);
                    result->max_angle = std::max(result->max_angle, angle);
                    result->avg_angle += angle;
                    angle_count++;
                }
            }
        }
    }
    
    if (angle_count > 0) {
        result->avg_angle /= angle_count;
    }
    
    // Calculate total area
    result->total_area = 0.0;
    for (int i = 0; i < result->num_triangles; ++i) {
        result->total_area += result->triangles[i].area;
    }
}

// Main triangulation function
bool clipper2_triangle_triangulate_2d(const clipper2_triangle_2d_params_t* params, 
                                    clipper2_triangle_2d_result_t* result) {
    if (!params || !result) {
        std::cerr << "[Clipper2+Triangle] Invalid parameters" << std::endl;
        return false;
    }
    
    try {
        std::cout << "[Clipper2+Triangle] Starting 2D constrained triangulation..." << std::endl;
        
        // Parse input polygons using Clipper2
        std::cout << "[Clipper2+Triangle] Parsing input polygons..." << std::endl;
        
        // Convert input paths to Clipper2 format
        std::vector<Path64> main_paths;
        std::vector<Path64> hole_paths;
        std::vector<Path64> region_paths;
        
        // Process main polygons
        for (int i = 0; i < params->num_polygons; ++i) {
            Path64 path;
            for (int j = 0; j < params->polygons[i].num_points; ++j) {
                path.push_back(Point64(
                    static_cast<int64_t>(params->polygons[i].points[j].x * params->scale_factor),
                    static_cast<int64_t>(params->polygons[i].points[j].y * params->scale_factor)
                ));
            }
            
            if (!path.empty() && path.front() == path.back()) {
                // Closed polygon - determine if it's a hole or main polygon
                double area = Area(path);
                if (area > 0) {
                    main_paths.push_back(path);
                } else if (area < 0) {
                    hole_paths.push_back(ReversePath(path));
                }
            }
        }
        
        // Process holes
        for (int i = 0; i < params->num_holes; ++i) {
            Path64 path;
            for (int j = 0; j < params->holes[i].num_points; ++j) {
                path.push_back(Point64(
                    static_cast<int64_t>(params->holes[i].points[j].x * params->scale_factor),
                    static_cast<int64_t>(params->holes[i].points[j].y * params->scale_factor)
                ));
            }
            
            if (!path.empty()) {
                hole_paths.push_back(path);
            }
        }
        
        // Perform boolean operations if requested
        if (params->boolean_operation != CLIPPER2_BOOLEAN_NONE) {
            std::cout << "[Clipper2+Triangle] Performing boolean operations..." << std::endl;
            
            ClipType clip_type;
            switch (params->boolean_operation) {
                case CLIPPER2_BOOLEAN_UNION: clip_type = ClipType::Union; break;
                case CLIPPER2_BOOLEAN_INTERSECTION: clip_type = ClipType::Intersection; break;
                case CLIPPER2_BOOLEAN_DIFFERENCE: clip_type = ClipType::Difference; break;
                case CLIPPER2_BOOLEAN_XOR: clip_type = ClipType::Xor; break;
                default: clip_type = ClipType::None;
            }
            
            if (clip_type != ClipType::None && main_paths.size() > 1) {
                Paths64 result_paths;
                
                // Perform boolean operation
                result_paths = BooleanOp(main_paths, clip_type, FillRule::NonZero);
                main_paths = result_paths;
            }
        }
        
        // Prepare data for Triangle
        InternalTriangulationData tri_data;
        
        // Convert back to double coordinates
        for (const auto& path : main_paths) {
            std::vector<Point64> scaled_path;
            for (const auto& pt : path) {
                scaled_path.push_back(Point64(
                    static_cast<int64_t>(pt.x),
                    static_cast<int64_t>(pt.y)
                ));
            }
            tri_data.points = scaled_path;
            break; // Use first path as main polygon for now
        }
        
        tri_data.holes = hole_paths;
        tri_data.min_angle = params->min_angle;
        tri_data.max_area = params->max_triangle_area;
        tri_data.use_steiner_points = params->use_steiner_points;
        tri_data.max_steiner_points = params->max_steiner_points;
        
        std::cout << "[Clipper2+Triangle] Prepared " << tri_data.points.size() 
                  << " points and " << tri_data.holes.size() << " holes" << std::endl;
        
        // Set up Triangle input/output structures
        struct triangulateio in, out;
        memset(&in, 0, sizeof(struct triangulateio));
        memset(&out, 0, sizeof(struct triangulateio));
        
        // Convert to Triangle format
        convertClipperToTriangle(tri_data, &in, &out);
        
        // Build Triangle options string
        std::string options = "pzQ";
        
        if (params->min_angle > 0) {
            options += "q" + std::to_string(static_cast<int>(params->min_angle));
        }
        
        if (params->max_triangle_area > 0) {
            options += "a" + std::to_string(params->max_triangle_area);
        }
        
        if (params->use_steiner_points) {
            options += "S";
            if (params->max_steiner_points > 0) {
                options += std::to_string(params->max_steiner_points);
            }
        }
        
        if (params->optimize_quality) {
            options += "O2"; // Quality optimization
        }
        
        if (params->verbose) {
            options += "V";
        }
        
        std::cout << "[Clipper2+Triangle] Triangle options: " << options << std::endl;
        
        // Perform triangulation
        std::cout << "[Clipper2+Triangle] Performing constrained triangulation..." << std::endl;
        
        char* options_cstr = strdup(options.c_str());
        triangulate(options_cstr, &in, &out, nullptr);
        free(options_cstr);
        
        // Convert results back
        convertTriangleToOutput(&out, result);
        
        std::cout << "[Clipper2+Triangle] Triangulation completed:" << std::endl;
        std::cout << "  - Vertices: " << result->num_vertices << std::endl;
        std::cout << "  - Triangles: " << result->num_triangles << std::endl;
        std::cout << "  - Segments: " << result->num_segments << std::endl;
        std::cout << "  - Total area: " << result->total_area << std::endl;
        std::cout << "  - Min angle: " << result->min_angle << "°" << std::endl;
        std::cout << "  - Max angle: " << result->max_angle << "°" << std::endl;
        std::cout << "  - Avg angle: " << result->avg_angle << "°" << std::endl;
        
        // Clean up Triangle memory
        free(in.pointlist);
        free(in.pointmarkerlist);
        free(in.segmentlist);
        free(in.segmentmarkerlist);
        free(in.holelist);
        free(in.regionlist);
        
        free(out.pointlist);
        free(out.pointattributelist);
        free(out.pointmarkerlist);
        free(out.trianglelist);
        free(out.triangleattributelist);
        free(out.neighborlist);
        free(out.segmentlist);
        free(out.segmentmarkerlist);
        free(out.edgelist);
        free(out.edgemarkerlist);
        free(out.normlist);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Clipper2+Triangle] Exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[Clipper2+Triangle] Unknown exception during triangulation" << std::endl;
        return false;
    }
}

// Function to perform boolean operations only
bool clipper2_boolean_operation(const clipper2_polygon_t* polygons1, int num_polygons1,
                               const clipper2_polygon_t* polygons2, int num_polygons2,
                               clipper2_boolean_op_t operation,
                               clipper2_polygon_t** result_polygons, int* num_result_polygons) {
    if (!polygons1 || num_polygons1 <= 0 || !result_polygons || !num_result_polygons) {
        return false;
    }
    
    try {
        // Convert input polygons to Clipper2 format
        Paths64 paths1, paths2;
        
        // Convert first set of polygons
        for (int i = 0; i < num_polygons1; ++i) {
            Path64 path;
            for (int j = 0; j < polygons1[i].num_points; ++j) {
                path.push_back(Point64(
                    static_cast<int64_t>(polygons1[i].points[j].x * 1000.0),
                    static_cast<int64_t>(polygons1[i].points[j].y * 1000.0)
                ));
            }
            if (!path.empty()) {
                paths1.push_back(path);
            }
        }
        
        // Convert second set of polygons (if provided)
        if (polygons2 && num_polygons2 > 0) {
            for (int i = 0; i < num_polygons2; ++i) {
                Path64 path;
                for (int j = 0; j < polygons2[i].num_points; ++j) {
                    path.push_back(Point64(
                        static_cast<int64_t>(polygons2[i].points[j].x * 1000.0),
                        static_cast<int64_t>(polygons2[i].points[j].y * 1000.0)
                    ));
                }
                if (!path.empty()) {
                    paths2.push_back(path);
                }
            }
        }
        
        // Determine clip type
        ClipType clip_type;
        switch (operation) {
            case CLIPPER2_BOOLEAN_UNION: clip_type = ClipType::Union; break;
            case CLIPPER2_BOOLEAN_INTERSECTION: clip_type = ClipType::Intersection; break;
            case CLIPPER2_BOOLEAN_DIFFERENCE: clip_type = ClipType::Difference; break;
            case CLIPPER2_BOOLEAN_XOR: clip_type = ClipType::Xor; break;
            default: return false;
        }
        
        // Perform boolean operation
        Paths64 result_paths;
        if (paths2.empty()) {
            // Single operand operation
            result_paths = Union(paths1, FillRule::NonZero);
        } else {
            // Two operand operation
            result_paths = BooleanOp(paths1, paths2, clip_type, FillRule::NonZero);
        }
        
        // Convert result back
        *num_result_polygons = result_paths.size();
        *result_polygons = (clipper2_polygon_t*)malloc(
            *num_result_polygons * sizeof(clipper2_polygon_t));
        
        for (int i = 0; i < *num_result_polygons; ++i) {
            (*result_polygons)[i].num_points = result_paths[i].size();
            (*result_polygons)[i].points = (clipper2_point_2d_t*)malloc(
                result_paths[i].size() * sizeof(clipper2_point_2d_t));
            
            for (size_t j = 0; j < result_paths[i].size(); ++j) {
                (*result_polygons)[i].points[j].x = result_paths[i][j].x / 1000.0;
                (*result_polygons)[i].points[j].y = result_paths[i][j].y / 1000.0;
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Clipper2] Boolean operation exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[Clipper2] Unknown exception during boolean operation" << std::endl;
        return false;
    }
}

// Function to free triangulation result
void clipper2_triangle_free_result(clipper2_triangle_2d_result_t* result) {
    if (!result) return;
    
    if (result->vertices) {
        free(result->vertices);
        result->vertices = nullptr;
    }
    
    if (result->triangles) {
        free(result->triangles);
        result->triangles = nullptr;
    }
    
    if (result->segments) {
        free(result->segments);
        result->segments = nullptr;
    }
    
    result->num_vertices = 0;
    result->num_triangles = 0;
    result->num_segments = 0;
}

// Function to free boolean operation result
void clipper2_free_polygons(clipper2_polygon_t* polygons, int num_polygons) {
    if (!polygons) return;
    
    for (int i = 0; i < num_polygons; ++i) {
        if (polygons[i].points) {
            free(polygons[i].points);
        }
    }
    
    free(polygons);
}

// Utility function to get library version
const char* clipper2_triangle_get_version() {
    return "Clipper2+Triangle Integration v1.0.0";
}

// Utility function to check if libraries are available
bool clipper2_triangle_is_available() {
    try {
        // Test basic Clipper2 functionality
        Path64 test_path;
        test_path.push_back(Point64(0, 0));
        test_path.push_back(Point64(1, 0));
        test_path.push_back(Point64(1, 1));
        test_path.push_back(Point64(0, 1));
        
        double area = Area(test_path);
        return area > 0;
        
    } catch (...) {
        return false;
    }
}

bool clipper2_offset_polyline(const clipper2_point_2d_t* points,
                              int num_points,
                              double half_width,
                              double scale_factor,
                              clipper2_polygon_t** out_polygons,
                              int* out_count) {
    if (!points || num_points < 2 || !out_polygons || !out_count) return false;
    try {
        Path64 path;
        for (int i = 0; i < num_points; ++i) {
            path.push_back(Point64(
                (int64_t)std::llround(points[i].x * scale_factor),
                (int64_t)std::llround(points[i].y * scale_factor)));
        }
        Paths64 src; src.push_back(path);
        double dist = half_width * scale_factor;
        Paths64 out = InflatePaths(src, dist, JoinType::Round, EndType::OpenRound, 10.0, 0.25);
        *out_count = (int)out.size();
        *out_polygons = (clipper2_polygon_t*)malloc((*out_count) * sizeof(clipper2_polygon_t));
        for (int i = 0; i < *out_count; ++i) {
            (*out_polygons)[i].num_points = (int)out[i].size();
            (*out_polygons)[i].points = (clipper2_point_2d_t*)malloc(out[i].size() * sizeof(clipper2_point_2d_t));
            for (size_t j = 0; j < out[i].size(); ++j) {
                (*out_polygons)[i].points[j].x = out[i][j].x / scale_factor;
                (*out_polygons)[i].points[j].y = out[i][j].y / scale_factor;
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}
