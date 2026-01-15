/**
 * Clipper2 + Triangle 2D Constrained Triangulation Test Suite
 * 
 * Comprehensive testing of 2D constrained triangulation functionality including:
 * - Basic triangulation with quality guarantees
 * - Boolean operations on polygons
 * - Hole handling and internal boundaries
 * - Steiner point insertion
 * - Multi-region triangulation
 * - Mesh quality assessment
 * - Performance benchmarking
 * - Integration with electromagnetic simulation requirements
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iomanip>

// Include the Clipper2+Triangle integration header
#include "../src/mesh/clipper2_triangle_2d.h"

// Test result structure
struct TestResult {
    std::string test_name;
    bool passed;
    double execution_time_ms;
    std::string message;
    std::string details;
    int num_vertices;
    int num_triangles;
    double min_angle;
    double max_angle;
    double avg_angle;
    double total_area;
};

// Test statistics
struct TestStatistics {
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    double total_time_ms = 0.0;
    int total_vertices = 0;
    int total_triangles = 0;
    std::vector<TestResult> results;
};

// Global test statistics
static TestStatistics g_stats;

// Test helper functions
static void recordTestResult(const std::string& name, bool passed, double time_ms, 
                           const std::string& message = "", const std::string& details = "",
                           int vertices = 0, int triangles = 0, 
                           double min_angle = 0.0, double max_angle = 0.0, 
                           double avg_angle = 0.0, double total_area = 0.0) {
    TestResult result;
    result.test_name = name;
    result.passed = passed;
    result.execution_time_ms = time_ms;
    result.message = message;
    result.details = details;
    result.num_vertices = vertices;
    result.num_triangles = triangles;
    result.min_angle = min_angle;
    result.max_angle = max_angle;
    result.avg_angle = avg_angle;
    result.total_area = total_area;
    
    g_stats.results.push_back(result);
    g_stats.total_tests++;
    g_stats.total_time_ms += time_ms;
    
    if (vertices > 0) g_stats.total_vertices += vertices;
    if (triangles > 0) g_stats.total_triangles += triangles;
    
    if (passed) {
        g_stats.passed_tests++;
        std::cout << "[PASS] " << name << " (" << time_ms << "ms)";
        if (!message.empty()) std::cout << " - " << message;
        std::cout << std::endl;
    } else {
        g_stats.failed_tests++;
        std::cout << "[FAIL] " << name << " (" << time_ms << "ms)";
        if (!message.empty()) std::cout << " - " << message;
        std::cout << std::endl;
        if (!details.empty()) std::cout << "       Details: " << details << std::endl;
    }
}

static double getElapsedTime(std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

// Test 1: Library availability
static void testLibraryAvailability() {
    auto start = std::chrono::high_resolution_clock::now();
    
    bool available = clipper2_triangle_is_available();
    const char* version = clipper2_triangle_get_version();
    
    double elapsed = getElapsedTime(start);
    
    std::string message = "Clipper2+Triangle " + std::string(version) + " is " + 
                         (available ? "available" : "not available");
    
    recordTestResult("Library Availability", available, elapsed, message);
}

// Test 2: Basic triangulation
static void testBasicTriangulation() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create a simple square polygon
    clipper2_polygon_t square;
    clipper2_point_2d_t square_points[] = {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}
    };
    square.points = square_points;
    square.num_points = 5;
    
    // Set up triangulation parameters
    clipper2_triangle_2d_params_t params;
    clipper2_triangle_default_params(&params);
    params.polygons = &square;
    params.num_polygons = 1;
    params.min_angle = 20.0;  // 20 degrees minimum angle
    params.use_steiner_points = false;  // No Steiner points for this test
    params.verbose = false;
    
    // Perform triangulation
    clipper2_triangle_2d_result_t result;
    memset(&result, 0, sizeof(result));
    
    bool success = clipper2_triangle_triangulate_2d(&params, &result);
    
    double elapsed = getElapsedTime(start);
    
    if (success) {
        bool quality_ok = result.min_angle >= 15.0; // Allow some tolerance
        bool area_ok = std::abs(result.total_area - 1.0) < 0.01; // Unit square area
        
        std::string message = "Triangulated with " + std::to_string(result.num_vertices) + 
                             " vertices, " + std::to_string(result.num_triangles) + " triangles";
        
        std::string details = "Min angle: " + std::to_string(result.min_angle) + "°, " +
                             "Max angle: " + std::to_string(result.max_angle) + "°, " +
                             "Total area: " + std::to_string(result.total_area);
        
        recordTestResult("Basic Triangulation", quality_ok && area_ok, elapsed, 
                         message, details, result.num_vertices, result.num_triangles,
                         result.min_angle, result.max_angle, result.avg_angle, result.total_area);
        
        // Clean up
        clipper2_triangle_free_result(&result);
    } else {
        recordTestResult("Basic Triangulation", false, elapsed, "Triangulation failed");
    }
}

// Test 3: Triangulation with hole
static void testTriangulationWithHole() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create outer polygon (square)
    clipper2_polygon_t outer;
    clipper2_point_2d_t outer_points[] = {
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}, {0.0, 0.0}
    };
    outer.points = outer_points;
    outer.num_points = 5;
    
    // Create hole (smaller square)
    clipper2_polygon_t hole;
    clipper2_point_2d_t hole_points[] = {
        {0.5, 0.5}, {1.5, 0.5}, {1.5, 1.5}, {0.5, 1.5}, {0.5, 0.5}
    };
    hole.points = hole_points;
    hole.num_points = 5;
    
    // Set up triangulation parameters
    clipper2_triangle_2d_params_t params;
    clipper2_triangle_default_params(&params);
    params.polygons = &outer;
    params.num_polygons = 1;
    params.holes = &hole;
    params.num_holes = 1;
    params.min_angle = 20.0;
    params.use_steiner_points = true;  // Allow Steiner points for better quality
    params.max_steiner_points = 100;
    params.verbose = false;
    
    // Perform triangulation
    clipper2_triangle_2d_result_t result;
    memset(&result, 0, sizeof(result));
    
    bool success = clipper2_triangle_triangulate_2d(&params, &result);
    
    double elapsed = getElapsedTime(start);
    
    if (success) {
        // Expected area: outer area (4) - hole area (1) = 3
        double expected_area = 3.0;
        bool area_ok = std::abs(result.total_area - expected_area) < 0.1;
        bool quality_ok = result.min_angle >= 15.0;
        
        std::string message = "Triangulated with hole: " + std::to_string(result.num_vertices) + 
                             " vertices, " + std::to_string(result.num_triangles) + " triangles";
        
        std::string details = "Expected area: " + std::to_string(expected_area) + ", " +
                             "Actual area: " + std::to_string(result.total_area) + ", " +
                             "Min angle: " + std::to_string(result.min_angle) + "°";
        
        recordTestResult("Triangulation with Hole", area_ok && quality_ok, elapsed, 
                         message, details, result.num_vertices, result.num_triangles,
                         result.min_angle, result.max_angle, result.avg_angle, result.total_area);
        
        // Clean up
        clipper2_triangle_free_result(&result);
    } else {
        recordTestResult("Triangulation with Hole", false, elapsed, "Triangulation failed");
    }
}

// Test 4: Boolean operations
static void testBooleanOperations() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create two overlapping rectangles
    clipper2_polygon_t rect1;
    clipper2_point_2d_t rect1_points[] = {
        {0.0, 0.0}, {2.0, 0.0}, {2.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}
    };
    rect1.points = rect1_points;
    rect1.num_points = 5;
    
    clipper2_polygon_t rect2;
    clipper2_point_2d_t rect2_points[] = {
        {1.0, 0.5}, {3.0, 0.5}, {3.0, 1.5}, {1.0, 1.5}, {1.0, 0.5}
    };
    rect2.points = rect2_points;
    rect2.num_points = 5;
    
    // Test union operation
    clipper2_polygon_t* result_polygons = nullptr;
    int num_result_polygons = 0;
    
    bool success = clipper2_boolean_operation(&rect1, 1, &rect2, 1, 
                                             CLIPPER2_BOOLEAN_UNION,
                                             &result_polygons, &num_result_polygons);
    
    double elapsed = getElapsedTime(start);
    
    if (success && num_result_polygons > 0) {
        std::string message = "Boolean union successful: " + std::to_string(num_result_polygons) + 
                             " result polygons";
        
        // Calculate total area of result
        double total_area = 0.0;
        for (int i = 0; i < num_result_polygons; ++i) {
            total_area += clipper2_polygon_area(&result_polygons[i]);
        }
        
        std::string details = "Total area: " + std::to_string(total_area) + 
                             " (expected: ~2.5)";
        
        recordTestResult("Boolean Operations", true, elapsed, message, details);
        
        // Clean up
        clipper2_free_polygons(result_polygons, num_result_polygons);
    } else {
        recordTestResult("Boolean Operations", false, elapsed, "Boolean operation failed");
    }
}

// Test 5: Steiner point insertion
static void testSteinerPointInsertion() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create a polygon that would benefit from Steiner points
    clipper2_polygon_t polygon;
    clipper2_point_2d_t poly_points[] = {
        {0.0, 0.0}, {1.0, 0.0}, {2.0, 0.1}, {3.0, 0.0}, {4.0, 0.0},
        {4.0, 1.0}, {3.0, 1.0}, {2.0, 0.9}, {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0}
    };
    polygon.points = poly_points;
    polygon.num_points = 11;
    
    // Test without Steiner points
    clipper2_triangle_2d_params_t params_no_steiner;
    clipper2_triangle_default_params(&params_no_steiner);
    params_no_steiner.polygons = &polygon;
    params_no_steiner.num_polygons = 1;
    params_no_steiner.min_angle = 25.0;
    params_no_steiner.use_steiner_points = false;
    params_no_steiner.verbose = false;
    
    clipper2_triangle_2d_result_t result_no_steiner;
    memset(&result_no_steiner, 0, sizeof(result_no_steiner));
    
    auto start_no_steiner = std::chrono::high_resolution_clock::now();
    bool success_no_steiner = clipper2_triangle_triangulate_2d(&params_no_steiner, &result_no_steiner);
    double time_no_steiner = getElapsedTime(start_no_steiner);
    
    // Test with Steiner points
    clipper2_triangle_2d_params_t params_steiner;
    clipper2_triangle_default_params(&params_steiner);
    params_steiner.polygons = &polygon;
    params_steiner.num_polygons = 1;
    params_steiner.min_angle = 25.0;
    params_steiner.use_steiner_points = true;
    params_steiner.max_steiner_points = 50;
    params_steiner.verbose = false;
    
    clipper2_triangle_2d_result_t result_steiner;
    memset(&result_steiner, 0, sizeof(result_steiner));
    
    auto start_steiner = std::chrono::high_resolution_clock::now();
    bool success_steiner = clipper2_triangle_triangulate_2d(&params_steiner, &result_steiner);
    double time_steiner = getElapsedTime(start_steiner);
    
    double total_time = getElapsedTime(start);
    
    if (success_no_steiner && success_steiner) {
        bool quality_improved = result_steiner.min_angle > result_no_steiner.min_angle;
        bool area_preserved = std::abs(result_steiner.total_area - result_no_steiner.total_area) < 0.01;
        
        std::string message = "Steiner: " + std::to_string(result_steiner.num_vertices) + 
                             " vertices, No Steiner: " + std::to_string(result_no_steiner.num_vertices) + 
                             " vertices";
        
        std::string details = "Min angle improved: " + std::to_string(result_no_steiner.min_angle) + 
                             "° -> " + std::to_string(result_steiner.min_angle) + "°";
        
        recordTestResult("Steiner Point Insertion", quality_improved && area_preserved, total_time, 
                         message, details, result_steiner.num_vertices, result_steiner.num_triangles,
                         result_steiner.min_angle, result_steiner.max_angle, result_steiner.avg_angle, 
                         result_steiner.total_area);
        
        // Clean up
        clipper2_triangle_free_result(&result_no_steiner);
        clipper2_triangle_free_result(&result_steiner);
    } else {
        recordTestResult("Steiner Point Insertion", false, total_time, "Steiner point test failed");
    }
}

// Test 6: Multi-region triangulation
static void testMultiRegionTriangulation() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create multiple polygons representing different materials/regions
    clipper2_polygon_t regions[3];
    
    // Region 1: Left rectangle
    clipper2_point_2d_t region1_points[] = {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 2.0}, {0.0, 2.0}, {0.0, 0.0}
    };
    regions[0].points = region1_points;
    regions[0].num_points = 5;
    
    // Region 2: Middle rectangle
    clipper2_point_2d_t region2_points[] = {
        {1.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {1.0, 2.0}, {1.0, 0.0}
    };
    regions[1].points = region2_points;
    regions[1].num_points = 5;
    
    // Region 3: Right rectangle
    clipper2_point_2d_t region3_points[] = {
        {2.0, 0.0}, {3.0, 0.0}, {3.0, 2.0}, {2.0, 2.0}, {2.0, 0.0}
    };
    regions[2].points = region3_points;
    regions[2].num_points = 5;
    
    // Set up triangulation parameters
    clipper2_triangle_2d_params_t params;
    clipper2_triangle_default_params(&params);
    params.polygons = regions;
    params.num_polygons = 3;
    params.min_angle = 20.0;
    params.use_steiner_points = true;
    params.max_steiner_points = 100;
    params.verbose = false;
    
    // Perform triangulation
    clipper2_triangle_2d_result_t result;
    memset(&result, 0, sizeof(result));
    
    bool success = clipper2_triangle_triangulate_2d(&params, &result);
    
    double elapsed = getElapsedTime(start);
    
    if (success) {
        bool quality_ok = result.min_angle >= 15.0;
        bool area_ok = result.total_area > 5.5; // Should be close to 6.0
        
        std::string message = "Multi-region triangulation: " + std::to_string(result.num_vertices) + 
                             " vertices, " + std::to_string(result.num_triangles) + " triangles";
        
        std::string details = "Total area: " + std::to_string(result.total_area) + 
                             " (expected: ~6.0), Min angle: " + std::to_string(result.min_angle) + "°";
        
        recordTestResult("Multi-Region Triangulation", quality_ok && area_ok, elapsed, 
                         message, details, result.num_vertices, result.num_triangles,
                         result.min_angle, result.max_angle, result.avg_angle, result.total_area);
        
        // Clean up
        clipper2_triangle_free_result(&result);
    } else {
        recordTestResult("Multi-Region Triangulation", false, elapsed, "Multi-region triangulation failed");
    }
}

// Test 7: Quality optimization
static void testQualityOptimization() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create a challenging polygon for quality optimization
    clipper2_polygon_t polygon;
    clipper2_point_2d_t poly_points[] = {
        {0.0, 0.0}, {4.0, 0.0}, {4.0, 0.5}, {3.5, 0.5}, {3.5, 1.5}, {4.0, 1.5},
        {4.0, 2.0}, {0.0, 2.0}, {0.0, 1.5}, {0.5, 1.5}, {0.5, 0.5}, {0.0, 0.5}, {0.0, 0.0}
    };
    polygon.points = poly_points;
    polygon.num_points = 13;
    
    // Test with quality optimization
    clipper2_triangle_2d_params_t params;
    clipper2_triangle_default_params(&params);
    params.polygons = &polygon;
    params.num_polygons = 1;
    params.min_angle = 25.0;  // High quality requirement
    params.optimize_quality = true;
    params.use_steiner_points = true;
    params.max_steiner_points = 200;
    params.verbose = false;
    
    clipper2_triangle_2d_result_t result;
    memset(&result, 0, sizeof(result));
    
    bool success = clipper2_triangle_triangulate_2d(&params, &result);
    
    double elapsed = getElapsedTime(start);
    
    if (success) {
        bool quality_ok = result.min_angle >= 20.0;  // Should achieve close to target
        bool angles_reasonable = result.max_angle <= 120.0;  // No extremely large angles
        
        std::string message = "Quality optimization: " + std::to_string(result.num_vertices) + 
                             " vertices, " + std::to_string(result.num_triangles) + " triangles";
        
        std::string details = "Min angle: " + std::to_string(result.min_angle) + "° (target: 25°), " +
                             "Max angle: " + std::to_string(result.max_angle) + "°";
        
        recordTestResult("Quality Optimization", quality_ok && angles_reasonable, elapsed, 
                         message, details, result.num_vertices, result.num_triangles,
                         result.min_angle, result.max_angle, result.avg_angle, result.total_area);
        
        // Clean up
        clipper2_triangle_free_result(&result);
    } else {
        recordTestResult("Quality Optimization", false, elapsed, "Quality optimization failed");
    }
}

// Test 8: Performance benchmark
static void testPerformanceBenchmark() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create progressively more complex polygons
    std::vector<int> complexity_levels = {10, 50, 100, 200};
    std::vector<double> times;
    std::vector<int> vertices_generated;
    std::vector<int> triangles_generated;
    
    for (int complexity : complexity_levels) {
        // Create a polygon with specified number of vertices
        std::vector<clipper2_point_2d_t> points;
        for (int i = 0; i < complexity; ++i) {
            double angle = 2.0 * M_PI * i / complexity;
            double radius = 1.0 + 0.2 * std::sin(5.0 * angle); // Star-like shape
            points.push_back({
                radius * std::cos(angle),
                radius * std::sin(angle)
            });
        }
        // Close the polygon
        points.push_back(points[0]);
        
        clipper2_polygon_t polygon;
        polygon.points = points.data();
        polygon.num_points = points.size();
        
        auto test_start = std::chrono::high_resolution_clock::now();
        
        clipper2_triangle_2d_params_t params;
        clipper2_triangle_default_params(&params);
        params.polygons = &polygon;
        params.num_polygons = 1;
        params.min_angle = 20.0;
        params.use_steiner_points = true;
        params.max_steiner_points = complexity;
        params.verbose = false;
        
        clipper2_triangle_2d_result_t result;
        memset(&result, 0, sizeof(result));
        
        bool success = clipper2_triangle_triangulate_2d(&params, &result);
        
        double test_time = getElapsedTime(test_start);
        
        if (success) {
            times.push_back(test_time);
            vertices_generated.push_back(result.num_vertices);
            triangles_generated.push_back(result.num_triangles);
            
            clipper2_triangle_free_result(&result);
        }
    }
    
    double total_time = getElapsedTime(start);
    
    if (!times.empty()) {
        double avg_time = 0.0;
        int total_vertices = 0;
        int total_triangles = 0;
        
        for (size_t i = 0; i < times.size(); ++i) {
            avg_time += times[i];
            total_vertices += vertices_generated[i];
            total_triangles += triangles_generated[i];
        }
        avg_time /= times.size();
        
        std::string message = "Average time: " + std::to_string(avg_time) + "ms";
        
        std::string details = "Tested " + std::to_string(times.size()) + " complexity levels, " +
                             "Total vertices: " + std::to_string(total_vertices) + ", " +
                             "Total triangles: " + std::to_string(total_triangles);
        
        recordTestResult("Performance Benchmark", true, total_time, message, details);
    } else {
        recordTestResult("Performance Benchmark", false, total_time, "No successful tests");
    }
}

// Main test runner
int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "Clipper2 + Triangle 2D Triangulation Test Suite" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << std::endl;
    
    // Run all tests
    testLibraryAvailability();
    testBasicTriangulation();
    testTriangulationWithHole();
    testBooleanOperations();
    testSteinerPointInsertion();
    testMultiRegionTriangulation();
    testQualityOptimization();
    testPerformanceBenchmark();
    
    // Print summary
    std::cout << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Total tests: " << g_stats.total_tests << std::endl;
    std::cout << "Passed: " << g_stats.passed_tests << std::endl;
    std::cout << "Failed: " << g_stats.failed_tests << std::endl;
    std::cout << "Success rate: " << std::fixed << std::setprecision(1) 
              << (g_stats.total_tests > 0 ? (100.0 * g_stats.passed_tests / g_stats.total_tests) : 0.0) 
              << "%" << std::endl;
    std::cout << "Total execution time: " << std::fixed << std::setprecision(1) 
              << g_stats.total_time_ms << "ms" << std::endl;
    std::cout << "Average test time: " << std::fixed << std::setprecision(1) 
              << (g_stats.total_tests > 0 ? g_stats.total_time_ms / g_stats.total_tests : 0.0) 
              << "ms" << std::endl;
    std::cout << "Total vertices generated: " << g_stats.total_vertices << std::endl;
    std::cout << "Total triangles generated: " << g_stats.total_triangles << std::endl;
    
    // Print detailed results
    std::cout << std::endl;
    std::cout << "Detailed Results:" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    for (const auto& result : g_stats.results) {
        std::cout << (result.passed ? "[PASS] " : "[FAIL] ") 
                  << result.test_name << std::endl;
        if (!result.message.empty()) {
            std::cout << "       " << result.message << std::endl;
        }
        if (!result.details.empty()) {
            std::cout << "       " << result.details << std::endl;
        }
        if (result.num_vertices > 0) {
            std::cout << "       Vertices: " << result.num_vertices 
                      << ", Triangles: " << result.num_triangles << std::endl;
        }
        if (result.min_angle > 0) {
            std::cout << "       Angles: min=" << std::fixed << std::setprecision(1) 
                      << result.min_angle << "°, max=" << result.max_angle 
                      << "°, avg=" << result.avg_angle << "°" << std::endl;
        }
        if (result.total_area > 0) {
            std::cout << "       Area: " << std::fixed << std::setprecision(3) 
                      << result.total_area << std::endl;
        }
        std::cout << "       Execution time: " << std::fixed << std::setprecision(1) 
                  << result.execution_time_ms << "ms" << std::endl;
        std::cout << std::endl;
    }
    
    return (g_stats.failed_tests == 0) ? 0 : 1;
}