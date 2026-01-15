/********************************************************************************
 *  PulseEM - Gmsh 3D Surface Mesh Integration Test
 *
 *  Comprehensive test suite for Gmsh 3D surface mesh generation
 *  Tests CAD import, surface meshing, quality optimization, and MoM adaptation
 ********************************************************************************/

#include "../src/mesh/gmsh_surface_mesh.h"
#include "../src/geometry/geom_geometry.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstring>

// Test configuration
struct TestConfig {
    const char* name;
    bool (*test_func)();
    bool critical;
};

// Forward declarations for test functions
bool test_gmsh_availability();
bool test_simple_surface_mesh();
bool test_mom_frequency_adaptation();
bool test_cad_import_stl();
bool test_quality_optimization();
bool test_adaptive_refinement();
bool test_complex_geometry();
bool test_performance_benchmark();

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

static geom_geometry_t* create_test_box_surface(double width, double height, double depth) {
    geom_geometry_t* geometry = (geom_geometry_t*)calloc(1, sizeof(geom_geometry_t));
    if (!geometry) return nullptr;
    
    geometry->type = GEOM_TYPE_SURFACE;
    geometry->surface_data = calloc(1, sizeof(geom_surface_t));
    if (!geometry->surface_data) {
        free(geometry);
        return nullptr;
    }
    
    geom_surface_t* surface = (geom_surface_t*)geometry->surface_data;
    surface->num_loops = 6;  // 6 faces of the box
    surface->loops = (geom_loop_t*)calloc(6, sizeof(geom_loop_t));
    if (!surface->loops) {
        free(geometry->surface_data);
        free(geometry);
        return nullptr;
    }
    
    // Define the 6 faces of a box
    double vertices[8][3] = {
        {-width/2, -height/2, -depth/2},  // 0
        { width/2, -height/2, -depth/2},  // 1
        { width/2,  height/2, -depth/2},  // 2
        {-width/2,  height/2, -depth/2},  // 3
        {-width/2, -height/2,  depth/2},  // 4
        { width/2, -height/2,  depth/2},  // 5
        { width/2,  height/2,  depth/2},  // 6
        {-width/2,  height/2,  depth/2}   // 7
    };
    
    // Face definitions (counter-clockwise when viewed from outside)
    int face_vertices[6][4] = {
        {0, 1, 2, 3},  // Bottom face (z = -depth/2)
        {4, 7, 6, 5},  // Top face (z = depth/2)
        {0, 4, 5, 1},  // Front face (y = -height/2)
        {2, 6, 7, 3},  // Back face (y = height/2)
        {0, 3, 7, 4},  // Left face (x = -width/2)
        {1, 5, 6, 2}   // Right face (x = width/2)
    };
    
    for (int face = 0; face < 6; face++) {
        geom_loop_t* loop = &surface->loops[face];
        loop->num_vertices = 4;
        loop->vertices = (geom_vertex_t*)calloc(4, sizeof(geom_vertex_t));
        if (!loop->vertices) {
            // Cleanup on failure
            for (int i = 0; i < face; i++) {
                free(surface->loops[i].vertices);
            }
            free(surface->loops);
            free(geometry->surface_data);
            free(geometry);
            return nullptr;
        }
        
        for (int v = 0; v < 4; v++) {
            int vertex_idx = face_vertices[face][v];
            loop->vertices[v].x = vertices[vertex_idx][0];
            loop->vertices[v].y = vertices[vertex_idx][1];
            loop->vertices[v].z = vertices[vertex_idx][2];
            loop->vertices[v].id = face * 4 + v;
        }
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
 * Test: Gmsh Availability
 *********************************************************************/

bool test_gmsh_availability() {
    print_test_header("Gmsh API Availability");
    
    bool available = gmsh_mesh_is_available();
    
    if (available) {
        std::cout << "  Gmsh API is available and functional" << std::endl;
        
        // Test version information
        gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
        if (engine) {
            int major, minor, patch;
            if (gmsh_get_version(engine, &major, &minor, &patch)) {
                std::cout << "  Gmsh version: " << major << "." << minor << "." << patch << std::endl;
            }
            gmsh_mesh_engine_destroy(engine);
        }
    } else {
        std::cout << "  Gmsh API is not available" << std::endl;
    }
    
    return available;
}

/*********************************************************************
 * Test: Simple Surface Mesh
 *********************************************************************/

bool test_simple_surface_mesh() {
    print_test_header("Simple Surface Mesh Generation");
    
    // Create a simple box surface
    geom_geometry_t* geometry = create_test_box_surface(1.0, 1.0, 1.0);
    if (!geometry) return false;
    
    // Create Gmsh engine
    gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Set parameters
    gmsh_mesh_parameters_t params = {0};
    params.element_size = 0.1;
    params.element_size_min = 0.05;
    params.element_size_max = 0.2;
    params.algorithm_2d = GMSH_ALGORITHM_DELAUNAY;
    params.optimization = GMSH_OPTIMIZATION_LAPLACE;
    params.min_angle = 25.0;
    params.surface_mesh_only = true;
    params.preserve_surface_curvature = true;
    params.surface_optimization = true;
    
    // Generate mesh
    gmsh_mesh_result_t* result = gmsh_generate_surface_mesh(engine, geometry, &params);
    
    bool success = false;
    if (result) {
        if (result->error_code == 0) {
            std::cout << "  Generated " << result->num_vertices << " vertices, " 
                      << result->num_elements << " elements" << std::endl;
            std::cout << "  Triangles: " << result->num_triangles 
                      << ", Quadrangles: " << result->num_quadrangles << std::endl;
            std::cout << "  Quality: min=" << result->min_quality 
                      << ", avg=" << result->avg_quality << std::endl;
            std::cout << "  Time: " << result->generation_time * 1000 << "ms" << std::endl;
            std::cout << "  MoM compatible: " << (result->mom_compatible ? "Yes" : "No") << std::endl;
            
            // Validate results
            success = (result->num_vertices > 100 && 
                      result->num_elements > 50 &&
                      result->num_triangles > 0 &&
                      result->min_quality > 0.1 &&
                      result->avg_quality > 0.5 &&
                      result->mom_compatible);
        } else {
            std::cout << "  Error: " << result->error_message << std::endl;
        }
        
        free(result);
    }
    
    gmsh_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return success;
}

/*********************************************************************
 * Test: MoM Frequency Adaptation
 *********************************************************************/

bool test_mom_frequency_adaptation() {
    print_test_header("MoM Frequency Adaptation");
    
    // Create a simple patch antenna geometry
    geom_geometry_t* geometry = create_test_box_surface(0.1, 0.1, 0.001);  // 10cm x 10cm x 1mm
    if (!geometry) return false;
    
    // Create Gmsh engine
    gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Test different frequencies
    double frequencies[] = {1.0e9, 3.0e9, 10.0e9};  // 1 GHz, 3 GHz, 10 GHz
    bool all_tests_passed = true;
    
    for (int freq_idx = 0; freq_idx < 3; freq_idx++) {
        gmsh_mesh_result_t* result = gmsh_generate_mom_mesh(engine, geometry, 
                                                           frequencies[freq_idx], 
                                                           10.0, nullptr);
        
        if (result && result->error_code == 0) {
            double wavelength = 3.0e8 / frequencies[freq_idx];
            double element_size = wavelength / 10.0;  // 10 elements per wavelength
            
            std::cout << "  Frequency " << frequencies[freq_idx]/1e9 << " GHz: " 
                      << result->num_vertices << " vertices, " 
                      << result->num_elements << " elements" << std::endl;
            std::cout << "    Element size: " << element_size*1000 << " mm" << std::endl;
            std::cout << "    Quality: min=" << result->min_quality 
                      << ", avg=" << result->avg_quality << std::endl;
            
            // Higher frequency should produce more elements
            bool density_ok = (result->num_elements > 100 * (freq_idx + 1));
            bool quality_ok = (result->min_quality > 0.15 && result->avg_quality > 0.6);
            
            if (!density_ok || !quality_ok) {
                all_tests_passed = false;
            }
            
            free(result);
        } else {
            all_tests_passed = false;
            if (result) {
                std::cout << "  Error: " << result->error_message << std::endl;
                free(result);
            }
        }
    }
    
    gmsh_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return all_tests_passed;
}

/*********************************************************************
 * Test: CAD Import STL
 *********************************************************************/

bool test_cad_import_stl() {
    print_test_header("CAD Import STL");
    
    // Create a simple STL file for testing
    const char* stl_content = R"(solid test_box
  facet normal 0 0 -1
    outer loop
      vertex -0.5 -0.5 -0.5
      vertex 0.5 -0.5 -0.5
      vertex 0.5 0.5 -0.5
    endloop
  endfacet
  facet normal 0 0 -1
    outer loop
      vertex -0.5 -0.5 -0.5
      vertex 0.5 0.5 -0.5
      vertex -0.5 0.5 -0.5
    endloop
  endfacet
  facet normal 0 0 1
    outer loop
      vertex -0.5 -0.5 0.5
      vertex -0.5 0.5 0.5
      vertex 0.5 0.5 0.5
    endloop
  endfacet
  facet normal 0 0 1
    outer loop
      vertex -0.5 -0.5 0.5
      vertex 0.5 0.5 0.5
      vertex 0.5 -0.5 0.5
    endloop
  endfacet
endsolid test_box)";
    
    // Write temporary STL file
    FILE* stl_file = fopen("test_geometry.stl", "w");
    if (!stl_file) {
        std::cout << "  Failed to create test STL file" << std::endl;
        return false;
    }
    
    fprintf(stl_file, "%s", stl_content);
    fclose(stl_file);
    
    // Create Gmsh engine
    gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
    if (!engine) {
        remove("test_geometry.stl");
        return false;
    }
    
    // Set parameters for STL import
    gmsh_mesh_parameters_t params = {0};
    params.element_size = 0.1;
    params.import_cad = true;
    params.heal_geometry = true;
    params.remove_small_features = true;
    params.cad_tolerance = 1e-6;
    params.algorithm_2d = GMSH_ALGORITHM_DELAUNAY;
    params.optimization = GMSH_OPTIMIZATION_LAPLACE;
    
    // Import and mesh
    gmsh_mesh_result_t* result = gmsh_import_and_mesh(engine, "test_geometry.stl", &params);
    
    bool success = false;
    if (result) {
        if (result->error_code == 0) {
            std::cout << "  Imported and meshed STL successfully" << std::endl;
            std::cout << "  Generated " << result->num_vertices << " vertices, " 
                      << result->num_elements << " elements" << std::endl;
            std::cout << "  Quality: min=" << result->min_quality 
                      << ", avg=" << result->avg_quality << std::endl;
            std::cout << "  Time: " << result->generation_time * 1000 << "ms" << std::endl;
            
            success = (result->num_vertices > 50 && 
                      result->num_elements > 20 &&
                      result->min_quality > 0.1 &&
                      result->avg_quality > 0.5);
        } else {
            std::cout << "  Error: " << result->error_message << std::endl;
        }
        
        free(result);
    }
    
    gmsh_mesh_engine_destroy(engine);
    remove("test_geometry.stl");
    
    return success;
}

/*********************************************************************
 * Test: Quality Optimization
 *********************************************************************/

bool test_quality_optimization() {
    print_test_header("Quality Optimization");
    
    // Create a challenging geometry (star shape)
    geom_geometry_t* geometry = create_test_box_surface(1.0, 1.0, 0.1);  // Thin box
    if (!geometry) return false;
    
    // Create Gmsh engine
    gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Test with and without optimization
    gmsh_optimization_t optimization_types[] = {
        GMSH_OPTIMIZATION_NONE,
        GMSH_OPTIMIZATION_LAPLACE
    };
    
    bool all_tests_passed = true;
    
    for (int opt_idx = 0; opt_idx < 2; opt_idx++) {
        gmsh_mesh_parameters_t params = {0};
        params.element_size = 0.1;
        params.min_angle = 20.0;
        params.optimization = optimization_types[opt_idx];
        params.surface_optimization = true;
        
        gmsh_mesh_result_t* result = gmsh_generate_surface_mesh(engine, geometry, &params);
        
        if (result && result->error_code == 0) {
            std::cout << "  Optimization " << (opt_idx == 0 ? "OFF" : "ON") << ": " 
                      << result->num_vertices << " vertices, " 
                      << result->num_elements << " elements" << std::endl;
            std::cout << "    Quality: min=" << result->min_quality 
                      << ", avg=" << result->avg_quality << std::endl;
            std::cout << "    Poor elements: " << result->poor_quality_elements << std::endl;
            
            if (opt_idx == 1) {
                // With optimization, quality should be better
                bool quality_ok = (result->avg_quality > 0.6 && result->poor_quality_elements < 10);
                if (!quality_ok) {
                    all_tests_passed = false;
                }
            }
            
            free(result);
        } else {
            all_tests_passed = false;
            if (result) {
                std::cout << "  Error: " << result->error_message << std::endl;
                free(result);
            }
        }
    }
    
    gmsh_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return all_tests_passed;
}

/*********************************************************************
 * Test: Adaptive Refinement
 *********************************************************************/

bool test_adaptive_refinement() {
    print_test_header("Adaptive Refinement");
    
    // Create a geometry with varying complexity
    geom_geometry_t* geometry = create_test_box_surface(2.0, 1.0, 0.5);  // Rectangular box
    if (!geometry) return false;
    
    // Create Gmsh engine
    gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Generate initial mesh
    gmsh_mesh_parameters_t params = {0};
    params.element_size = 0.2;
    params.adaptive_refinement = true;
    params.refinement_levels = 2;
    params.refinement_threshold = 0.1;
    
    gmsh_mesh_result_t* result = gmsh_generate_surface_mesh(engine, geometry, &params);
    
    bool success = false;
    if (result && result->error_code == 0) {
        std::cout << "  Adaptive refinement completed" << std::endl;
        std::cout << "  Generated " << result->num_vertices << " vertices, " 
                  << result->num_elements << " elements" << std::endl;
        std::cout << "  Refinement steps: " << result->refinement_steps << std::endl;
        std::cout << "  Quality: min=" << result->min_quality 
                  << ", avg=" << result->avg_quality << std::endl;
        
        success = (result->num_vertices > 200 && 
                  result->num_elements > 100 &&
                  result->refinement_steps >= 0 &&
                  result->min_quality > 0.15);
        
        free(result);
    } else {
        if (result) {
            std::cout << "  Error: " << result->error_message << std::endl;
            free(result);
        }
    }
    
    gmsh_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return success;
}

/*********************************************************************
 * Test: Complex Geometry
 *********************************************************************/

bool test_complex_geometry() {
    print_test_header("Complex Geometry Processing");
    
    // Create a more complex surface (multiple connected faces)
    geom_geometry_t* geometry = create_test_box_surface(1.0, 2.0, 0.5);  // Elongated box
    if (!geometry) return false;
    
    // Create Gmsh engine
    gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
    if (!engine) {
        destroy_test_geometry(geometry);
        return false;
    }
    
    // Test with different algorithms
    gmsh_algorithm_t algorithms[] = {
        GMSH_ALGORITHM_DELAUNAY,
        GMSH_ALGORITHM_FRONTAL,
        GMSH_ALGORITHM_BAMG
    };
    
    bool all_tests_passed = true;
    
    for (int alg_idx = 0; alg_idx < 3; alg_idx++) {
        gmsh_mesh_parameters_t params = {0};
        params.element_size = 0.15;
        params.algorithm_2d = algorithms[alg_idx];
        params.optimization = GMSH_OPTIMIZATION_LAPLACE;
        params.preserve_surface_curvature = true;
        params.min_angle = 20.0;
        
        gmsh_mesh_result_t* result = gmsh_generate_surface_mesh(engine, geometry, &params);
        
        if (result && result->error_code == 0) {
            std::cout << "  Algorithm " << alg_idx << ": " 
                      << result->num_vertices << " vertices, " 
                      << result->num_elements << " elements" << std::endl;
            std::cout << "    Quality: min=" << result->min_quality 
                      << ", avg=" << result->avg_quality << std::endl;
            
            bool quality_ok = (result->min_quality > 0.1 && result->avg_quality > 0.5);
            if (!quality_ok) {
                all_tests_passed = false;
            }
            
            free(result);
        } else {
            all_tests_passed = false;
            if (result) {
                std::cout << "  Error: " << result->error_message << std::endl;
                free(result);
            }
        }
    }
    
    gmsh_mesh_engine_destroy(engine);
    destroy_test_geometry(geometry);
    
    return all_tests_passed;
}

/*********************************************************************
 * Test: Performance Benchmark
 *********************************************************************/

bool test_performance_benchmark() {
    print_test_header("Performance Benchmark");
    
    // Create increasingly complex geometries
    double sizes[] = {0.5, 1.0, 2.0};
    bool all_tests_passed = true;
    
    for (int size_idx = 0; size_idx < 3; size_idx++) {
        geom_geometry_t* geometry = create_test_box_surface(sizes[size_idx], sizes[size_idx], sizes[size_idx]);
        if (!geometry) {
            all_tests_passed = false;
            continue;
        }
        
        gmsh_mesh_engine_t* engine = gmsh_mesh_engine_create();
        if (!engine) {
            destroy_test_geometry(geometry);
            all_tests_passed = false;
            continue;
        }
        
        gmsh_mesh_parameters_t params = {0};
        params.element_size = 0.1;
        params.optimization = GMSH_OPTIMIZATION_LAPLACE;
        params.num_threads = 4;  // Use 4 threads
        params.parallel_meshing = true;
        
        clock_t start_time = clock();
        gmsh_mesh_result_t* result = gmsh_generate_surface_mesh(engine, geometry, &params);
        clock_t end_time = clock();
        double total_time = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;
        
        if (result && result->error_code == 0) {
            std::cout << "  Size " << sizes[size_idx] << "x" << sizes[size_idx] << "x" << sizes[size_idx] << ": " 
                      << result->num_vertices << " vertices, " 
                      << result->num_elements << " elements" << std::endl;
            std::cout << "    Time: " << total_time << "ms" << std::endl;
            std::cout << "    Memory: " << result->memory_usage_mb << " MB" << std::endl;
            std::cout << "    Vertices/sec: " << (result->num_vertices / (total_time / 1000.0)) << std::endl;
            
            // Performance criteria
            bool speed_ok = (total_time < 5000.0);  // Less than 5 seconds
            bool memory_ok = (result->memory_usage_mb < 100.0);  // Less than 100 MB
            bool quality_ok = (result->min_quality > 0.1);
            
            if (!speed_ok || !memory_ok || !quality_ok) {
                all_tests_passed = false;
            }
            
            free(result);
        } else {
            all_tests_passed = false;
            if (result) {
                std::cout << "  Error: " << result->error_message << std::endl;
                free(result);
            }
        }
        
        gmsh_mesh_engine_destroy(engine);
        destroy_test_geometry(geometry);
    }
    
    return all_tests_passed;
}

/*********************************************************************
 * Main Test Runner
 *********************************************************************/

static TestConfig g_test_configs[] = {
    {"Gmsh API Availability", test_gmsh_availability, true},
    {"Simple Surface Mesh", test_simple_surface_mesh, true},
    {"MoM Frequency Adaptation", test_mom_frequency_adaptation, true},
    {"CAD Import STL", test_cad_import_stl, true},
    {"Quality Optimization", test_quality_optimization, false},
    {"Adaptive Refinement", test_adaptive_refinement, false},
    {"Complex Geometry", test_complex_geometry, true},
    {"Performance Benchmark", test_performance_benchmark, false},
};

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << "  Gmsh 3D Surface Mesh Integration Test" << std::endl;
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
        std::cout << "\n✓ ALL TESTS PASSED - Gmsh 3D surface mesh integration is working correctly!" << std::endl;
        return 0;
    } else if (g_test_stats.critical_failures == 0) {
        std::cout << "\n⚠ Some non-critical tests failed, but core functionality is working." << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ CRITICAL TESTS FAILED - Implementation needs fixes!" << std::endl;
        return 1;
    }
}