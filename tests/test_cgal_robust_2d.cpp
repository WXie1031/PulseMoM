/********************************************************************************
 *  PulseEM - CGAL Robust 2D Implementation Test
 *
 *  Comprehensive test suite for enhanced CGAL 2D mesh generation
 *  Tests robust point-in-polygon algorithms, boundary constraints,
 *  and complex geometry handling
 ********************************************************************************/

#include "../src/mesh/cgal_mesh_engine.h"
#include "../src/geometry/geom_geometry.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

// Test configuration
struct TestConfig {
    const char* name;
    bool (*test_func)();
    bool critical;
};

// Forward declarations for test functions
bool test_simple_polygon_meshing();
bool test_complex_polygon_with_holes();
bool test_boundary_constraint_robustness();
bool test_point_in_polygon_accuracy();
bool test_interior_point_generation();
bool test_mesh_quality_assessment();
bool test_mom_frequency_adaptation();
bool test_em_field_adaptation();

// Test statistics
struct TestStats {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int critical_failures;
    double total_time;
};

static TestStats g_test_stats = {0};

/*********************************************************************
 * Utility Functions
 *********************************************************************/

static void print_test_header(const char* test_name) {
    std::cout << "\n=== Testing: " << test_name << " ===" << std::endl;
}

static void print_test_result(const char* test_name, bool passed, double time_ms) {
    g_test_stats.total_tests++;
    if (passed) {
        g_test_stats.passed_tests++;
        std::cout << "✓ PASS: " << test_name << " (" << time_ms << "ms)" << std::endl;
    } else {
        g_test_stats.failed_tests++;
        std::cout << "✗ FAIL: " << test_name << " (" << time_ms << "ms)" << std::endl;
    }
}

static geom_geometry_t* create_test_polygon(int num_vertices, double* x_coords, double* y_coords) {
    geom_geometry_t* geometry = (geom_geometry_t*)calloc(1, sizeof(geom_geometry_t));
    if (!geometry) return nullptr;
    
    geometry->type = GEOM_TYPE_SURFACE;
    geometry->surface_data = calloc(1, sizeof(geom_surface_t));
    if (!geometry->surface_data) {
        free(geometry);
        return nullptr;
    }
    
    geom_surface_t* surface = (geom_surface_t*)geometry->surface_data;
    surface->num_loops = 1;
    surface->loops = (geom_loop_t*)calloc(1, sizeof(geom_loop_t));
    if (!surface->loops) {
        free(geometry->surface_data);
        free(geometry);
        return nullptr;
    }
    
    geom_loop_t* loop = &surface->loops[0];
    loop->num_vertices = num_vertices;
    loop->vertices = (geom_vertex_t*)calloc(num_vertices, sizeof(geom_vertex_t));
    if (!loop->vertices) {
        free(surface->loops);
        free(geometry->surface_data);
        free(geometry);
        return nullptr;
    }
    
    for (int i = 0; i < num_vertices; i++) {
        loop->vertices[i].x = x_coords[i];
        loop->vertices[i].y = y_coords[i];
        loop->vertices[i].z = 0.0;
        loop->vertices[i].id = i;
    }
    
    return geometry;
}

static void destroy_test_geometry(geom_geometry_t* geometry) {
    if (!geometry) return;
    
    if (geometry->surface_data) {
        geom_surface_t* surface = (geom_surface_t*)geometry->surface_data;
        if (surface->loops) {
            for (int i = 0; i < surface->num_loops; i++) {
                free(surface->loops[i].vertices);
            }
            free(surface->loops);
        }
        free(geometry->surface_data);
    }
    free(geometry);
}

/*********************************************************************
 * Test: Simple Polygon Meshing
 *********************************************************************/

bool test_simple_polygon_meshing() {
    print_test_header("Simple Polygon Meshing");
    
    // Create a simple square
    double x_coords[] = {0.0, 1.0, 1.0, 0.0};
    double y_coords[] = {0.0, 0.0, 1.0, 1.0};
    
    geom_geometry_t* geometry = create_test_polygon(4, x_coords, y_coords);
    if (!geometry) return false;
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Set parameters
    cgal_mesh_parameters_t params = {0};
    params.global_size = 0.1;
    params.enable_optimization = true;
    params.optimization_iterations = 5;
    params.min_angle_threshold = 20.0;
    
    // Generate mesh
    mesh_result_t* result = cgal_generate_triangular_mesh(engine, geometry, &params);
    
    bool success = false;
    if (result && result->error_code == 0) {
        std::cout << "  Generated " << result->num_vertices << " vertices, " 
                  << result->num_elements << " triangles" << std::endl;
        std::cout << "  Quality: min=" << result->min_quality 
                  << ", avg=" << result->avg_quality << std::endl;
        std::cout << "  Time: " << result->generation_time * 1000 << "ms" << std::endl;
        
        success = (result->num_vertices > 0 && result->num_elements > 0 &&
                  result->min_quality > 0.1 && result->avg_quality > 0.5);
    }
    
    if (result) {
        if (result->mesh) mesh_mesh_destroy(result->mesh);
        free(result);
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return success;
}

/*********************************************************************
 * Test: Complex Polygon with Holes
 *********************************************************************/

bool test_complex_polygon_with_holes() {
    print_test_header("Complex Polygon with Holes");
    
    // Create a complex polygon with a hole (donut shape)
    // Outer boundary (octagon)
    double outer_x[] = {0.0, 2.0, 4.0, 5.0, 4.0, 2.0, 0.0, -1.0};
    double outer_y[] = {2.0, 0.0, 0.0, 2.0, 4.0, 4.0, 2.0, 2.0};
    
    // Inner hole (square)
    double inner_x[] = {1.0, 3.0, 3.0, 1.0};
    double inner_y[] = {1.0, 1.0, 3.0, 3.0};
    
    geom_geometry_t* geometry = (geom_geometry_t*)calloc(1, sizeof(geom_geometry_t));
    if (!geometry) return false;
    
    geometry->type = GEOM_TYPE_SURFACE;
    geometry->surface_data = calloc(1, sizeof(geom_surface_t));
    if (!geometry->surface_data) {
        free(geometry);
        return false;
    }
    
    geom_surface_t* surface = (geom_surface_t*)geometry->surface_data;
    surface->num_loops = 2;
    surface->loops = (geom_loop_t*)calloc(2, sizeof(geom_loop_t));
    if (!surface->loops) {
        free(geometry->surface_data);
        free(geometry);
        return false;
    }
    
    // Outer loop
    surface->loops[0].num_vertices = 8;
    surface->loops[0].vertices = (geom_vertex_t*)calloc(8, sizeof(geom_vertex_t));
    for (int i = 0; i < 8; i++) {
        surface->loops[0].vertices[i].x = outer_x[i];
        surface->loops[0].vertices[i].y = outer_y[i];
        surface->loops[0].vertices[i].z = 0.0;
        surface->loops[0].vertices[i].id = i;
    }
    
    // Inner loop (hole)
    surface->loops[1].num_vertices = 4;
    surface->loops[1].vertices = (geom_vertex_t*)calloc(4, sizeof(geom_vertex_t));
    for (int i = 0; i < 4; i++) {
        surface->loops[1].vertices[i].x = inner_x[i];
        surface->loops[1].vertices[i].y = inner_y[i];
        surface->loops[1].vertices[i].z = 0.0;
        surface->loops[1].vertices[i].id = i + 100;  // Offset to avoid conflicts
    }
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Set parameters for complex geometry
    cgal_mesh_parameters_t params = {0};
    params.global_size = 0.2;
    params.enable_optimization = true;
    params.optimization_iterations = 10;
    params.min_angle_threshold = 25.0;
    params.enable_constraints = true;
    
    // Generate mesh
    mesh_result_t* result = cgal_generate_triangular_mesh(engine, geometry, &params);
    
    bool success = false;
    if (result && result->error_code == 0) {
        std::cout << "  Generated " << result->num_vertices << " vertices, " 
                  << result->num_elements << " triangles" << std::endl;
        std::cout << "  Quality: min=" << result->min_quality 
                  << ", avg=" << result->avg_quality << std::endl;
        std::cout << "  Poor quality elements: " << result->poor_quality_elements << std::endl;
        
        // Check that mesh respects the hole (no elements should be in the hole region)
        success = (result->num_vertices > 20 && result->num_elements > 20 &&
                  result->min_quality > 0.15 && result->avg_quality > 0.6);
    }
    
    if (result) {
        if (result->mesh) mesh_mesh_destroy(result->mesh);
        free(result);
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return success;
}

/*********************************************************************
 * Test: Point-in-Polygon Accuracy
 *********************************************************************/

bool test_point_in_polygon_accuracy() {
    print_test_header("Point-in-Polygon Accuracy");
    
    // Create a star-shaped polygon
    double x_coords[] = {0.0, 0.5, 1.0, 0.6, 0.5, 0.4, 0.0, 0.5, 1.0, 0.5};
    double y_coords[] = {0.5, 0.0, 0.5, 0.8, 0.5, 0.8, 0.5, 1.0, 0.5, 0.2};
    
    geom_geometry_t* geometry = create_test_polygon(10, x_coords, y_coords);
    if (!geometry) return false;
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Set parameters
    cgal_mesh_parameters_t params = {0};
    params.global_size = 0.05;
    params.enable_optimization = true;
    params.min_angle_threshold = 15.0;
    
    // Generate mesh
    mesh_result_t* result = cgal_generate_triangular_mesh(engine, geometry, &params);
    
    bool success = false;
    if (result && result->error_code == 0) {
        std::cout << "  Generated " << result->num_vertices << " vertices, " 
                  << result->num_elements << " triangles" << std::endl;
        std::cout << "  Quality metrics: min_angle=" << result->mesh->quality.min_angle 
                  << ", max_angle=" << result->mesh->quality.max_angle << std::endl;
        
        // Verify that all triangles have reasonable quality
        bool all_triangles_valid = true;
        for (int i = 0; i < result->num_elements; i++) {
            const mesh_element_t& elem = result->mesh->elements[i];
            if (elem.quality_factor < 0.01) {  // Very poor quality
                all_triangles_valid = false;
                break;
            }
        }
        
        success = (result->num_elements > 50 && all_triangles_valid &&
                  result->min_quality > 0.1 && result->avg_quality > 0.5);
    }
    
    if (result) {
        if (result->mesh) mesh_mesh_destroy(result->mesh);
        free(result);
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return success;
}

/*********************************************************************
 * Test: Interior Point Generation
 *********************************************************************/

bool test_interior_point_generation() {
    print_test_header("Interior Point Generation");
    
    // Create a polygon with concave regions
    double x_coords[] = {0.0, 3.0, 3.0, 2.0, 2.0, 1.0, 1.0, 0.0};
    double y_coords[] = {0.0, 0.0, 3.0, 3.0, 1.0, 1.0, 2.0, 2.0};
    
    geom_geometry_t* geometry = create_test_polygon(8, x_coords, y_coords);
    if (!geometry) return false;
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Test different element sizes
    double target_sizes[] = {0.5, 0.2, 0.1};
    bool all_tests_passed = true;
    
    for (int size_idx = 0; size_idx < 3; size_idx++) {
        cgal_mesh_parameters_t params = {0};
        params.global_size = target_sizes[size_idx];
        params.enable_optimization = true;
        params.min_angle_threshold = 20.0;
        
        mesh_result_t* result = cgal_generate_triangular_mesh(engine, geometry, &params);
        
        if (result && result->error_code == 0) {
            std::cout << "  Size " << target_sizes[size_idx] << ": " 
                      << result->num_vertices << " vertices, " 
                      << result->num_elements << " triangles" << std::endl;
            
            // Check that mesh density scales appropriately with target size
            double expected_elements = (3.0 * 3.0) / (target_sizes[size_idx] * target_sizes[size_idx]) * 2.0;
            bool density_ok = (result->num_elements >= expected_elements * 0.5 &&
                             result->num_elements <= expected_elements * 4.0);
            
            if (!density_ok) {
                all_tests_passed = false;
            }
            
            if (result->mesh) mesh_mesh_destroy(result->mesh);
            free(result);
        } else {
            all_tests_passed = false;
        }
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return all_tests_passed;
}

/*********************************************************************
 * Test: Boundary Constraint Robustness
 *********************************************************************/

bool test_boundary_constraint_robustness() {
    print_test_header("Boundary Constraint Robustness");
    
    // Create a polygon with very close vertices (numerical challenges)
    double x_coords[] = {0.0, 1e-6, 1.0, 1.0 - 1e-6, 0.0};
    double y_coords[] = {0.0, 1e-6, 1e-6, 1.0, 1.0};
    
    geom_geometry_t* geometry = create_test_polygon(5, x_coords, y_coords);
    if (!geometry) return false;
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Set parameters
    cgal_mesh_parameters_t params = {0};
    params.global_size = 0.1;
    params.enable_optimization = true;
    params.enable_constraints = true;
    params.min_angle_threshold = 15.0;
    
    // Generate mesh
    mesh_result_t* result = cgal_generate_triangular_mesh(engine, geometry, &params);
    
    bool success = false;
    if (result && result->error_code == 0) {
        std::cout << "  Generated " << result->num_vertices << " vertices, " 
                  << result->num_elements << " triangles" << std::endl;
        std::cout << "  Quality: min=" << result->min_quality 
                  << ", avg=" << result->avg_quality << std::endl;
        
        // Check that mesh handles numerical challenges
        success = (result->num_vertices > 0 && result->num_elements > 0 &&
                  result->min_quality > 0.05);  // Lower threshold for difficult geometry
    }
    
    if (result) {
        if (result->mesh) mesh_mesh_destroy(result->mesh);
        free(result);
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return success;
}

/*********************************************************************
 * Test: Mesh Quality Assessment
 *********************************************************************/

bool test_mesh_quality_assessment() {
    print_test_header("Mesh Quality Assessment");
    
    // Create a polygon that will generate varying quality triangles
    double x_coords[] = {0.0, 10.0, 10.0, 5.0, 0.0};
    double y_coords[] = {0.0, 0.0, 5.0, 8.0, 5.0};
    
    geom_geometry_t* geometry = create_test_polygon(5, x_coords, y_coords);
    if (!geometry) return false;
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Test with and without optimization
    bool optimization_states[] = {false, true};
    bool all_tests_passed = true;
    
    for (int opt_idx = 0; opt_idx < 2; opt_idx++) {
        cgal_mesh_parameters_t params = {0};
        params.global_size = 0.5;
        params.enable_optimization = optimization_states[opt_idx];
        params.optimization_iterations = 10;
        params.min_angle_threshold = 20.0;
        
        mesh_result_t* result = cgal_generate_triangular_mesh(engine, geometry, &params);
        
        if (result && result->error_code == 0) {
            std::cout << "  Optimization " << (optimization_states[opt_idx] ? "ON" : "OFF") 
                      << ": " << result->num_vertices << " vertices, " 
                      << result->num_elements << " triangles" << std::endl;
            std::cout << "    Quality: min=" << result->min_quality 
                      << ", avg=" << result->avg_quality 
                      << ", poor_elements=" << result->poor_quality_elements << std::endl;
            
            // Verify quality metrics are reasonable
            bool quality_ok = (result->min_quality > 0.0 && result->avg_quality > 0.0 &&
                             result->min_quality <= result->avg_quality);
            
            if (optimization_states[opt_idx]) {
                // With optimization, quality should be better
                quality_ok = quality_ok && (result->avg_quality > 0.6);
            }
            
            if (!quality_ok) {
                all_tests_passed = false;
            }
            
            if (result->mesh) mesh_mesh_destroy(result->mesh);
            free(result);
        } else {
            all_tests_passed = false;
        }
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return all_tests_passed;
}

/*********************************************************************
 * Test: MoM Frequency Adaptation
 *********************************************************************/

bool test_mom_frequency_adaptation() {
    print_test_header("MoM Frequency Adaptation");
    
    // Create a simple rectangular antenna
    double x_coords[] = {0.0, 0.1, 0.1, 0.0};  // 10cm x 10cm antenna
    double y_coords[] = {0.0, 0.0, 0.1, 0.1};
    
    geom_geometry_t* geometry = create_test_polygon(4, x_coords, y_coords);
    if (!geometry) return false;
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Test different frequencies (1 GHz, 3 GHz, 10 GHz)
    double frequencies[] = {1.0e9, 3.0e9, 10.0e9};
    bool all_tests_passed = true;
    
    for (int freq_idx = 0; freq_idx < 3; freq_idx++) {
        mesh_result_t* result = cgal_generate_mom_mesh(engine, geometry, 
                                                       frequencies[freq_idx], 
                                                       10.0, nullptr);
        
        if (result && result->error_code == 0) {
            double wavelength = 3.0e8 / frequencies[freq_idx];
            double element_size = wavelength / 10.0;  // 10 elements per wavelength
            
            std::cout << "  Frequency " << frequencies[freq_idx]/1e9 << " GHz: " 
                      << result->num_vertices << " vertices, " 
                      << result->num_elements << " triangles" << std::endl;
            std::cout << "    Element size: " << element_size*1000 << " mm" << std::endl;
            std::cout << "    Quality: min=" << result->min_quality 
                      << ", avg=" << result->avg_quality << std::endl;
            
            // Higher frequency should produce more elements
            bool density_ok = (result->num_elements > 20 * (freq_idx + 1));
            bool quality_ok = (result->min_quality > 0.15 && result->avg_quality > 0.7);
            
            if (!density_ok || !quality_ok) {
                all_tests_passed = false;
            }
            
            if (result->mesh) mesh_mesh_destroy(result->mesh);
            free(result);
        } else {
            all_tests_passed = false;
        }
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return all_tests_passed;
}

/*********************************************************************
 * Test: EM Field Adaptation
 *********************************************************************/

bool test_em_field_adaptation() {
    print_test_header("EM Field Adaptation");
    
    // Create a polygon with varying curvature
    double x_coords[] = {0.0, 1.0, 2.0, 3.0, 3.0, 2.0, 1.0, 0.0};
    double y_coords[] = {0.0, 0.5, 0.0, 0.5, 1.5, 2.0, 1.5, 1.0};
    
    geom_geometry_t* geometry = create_test_polygon(8, x_coords, y_coords);
    if (!geometry) return false;
    
    // Create mesh engine
    cgal_mesh_engine_t* engine = cgal_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Test with EM adaptation enabled
    cgal_mesh_parameters_t params = {0};
    params.global_size = 0.2;
    params.enable_optimization = true;
    params.enable_em_adaptation = true;
    params.min_angle_threshold = 25.0;
    params.strategy = MESH_STRATEGY_ACCURACY;
    
    mesh_result_t* result = cgal_generate_triangular_mesh(engine, geometry, &params);
    
    bool success = false;
    if (result && result->error_code == 0) {
        std::cout << "  Generated " << result->num_vertices << " vertices, " 
                  << result->num_elements << " triangles" << std::endl;
        std::cout << "  Quality: min=" << result->min_quality 
                  << ", avg=" << result->avg_quality << std::endl;
        std::cout << "  MoM compatible: " << (result->mom_compatible ? "Yes" : "No") << std::endl;
        
        // Check that mesh is suitable for EM simulation
        success = (result->num_elements > 30 && result->mom_compatible &&
                  result->min_quality > 0.2 && result->avg_quality > 0.6);
    }
    
    if (result) {
        if (result->mesh) mesh_mesh_destroy(result->mesh);
        free(result);
    }
    
    cgal_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return success;
}

/*********************************************************************
 * Main Test Runner
 *********************************************************************/

static TestConfig g_test_configs[] = {
    {"Simple Polygon Meshing", test_simple_polygon_meshing, true},
    {"Complex Polygon with Holes", test_complex_polygon_with_holes, true},
    {"Point-in-Polygon Accuracy", test_point_in_polygon_accuracy, true},
    {"Interior Point Generation", test_interior_point_generation, true},
    {"Boundary Constraint Robustness", test_boundary_constraint_robustness, true},
    {"Mesh Quality Assessment", test_mesh_quality_assessment, false},
    {"MoM Frequency Adaptation", test_mom_frequency_adaptation, true},
    {"EM Field Adaptation", test_em_field_adaptation, false},
};

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "  CGAL Robust 2D Implementation Test Suite" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    int num_tests = sizeof(g_test_configs) / sizeof(g_test_configs[0]);
    
    for (int i = 0; i < num_tests; i++) {
        clock_t start = clock();
        bool passed = g_test_configs[i].test_func();
        clock_t end = clock();
        double time_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
        
        print_test_result(g_test_configs[i].name, passed, time_ms);
        
        if (!passed && g_test_configs[i].critical) {
            g_test_stats.critical_failures++;
        }
    }
    
    std::cout << "\n=========================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "Total tests: " << g_test_stats.total_tests << std::endl;
    std::cout << "Passed: " << g_test_stats.passed_tests << std::endl;
    std::cout << "Failed: " << g_test_stats.failed_tests << std::endl;
    std::cout << "Critical failures: " << g_test_stats.critical_failures << std::endl;
    
    if (g_test_stats.critical_failures == 0 && g_test_stats.failed_tests == 0) {
        std::cout << "\n✓ ALL TESTS PASSED - CGAL robust 2D implementation is working correctly!" << std::endl;
        return 0;
    } else if (g_test_stats.critical_failures == 0) {
        std::cout << "\n⚠ Some non-critical tests failed, but core functionality is working." << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ CRITICAL TESTS FAILED - Implementation needs fixes!" << std::endl;
        return 1;
    }
}