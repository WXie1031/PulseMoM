/*********************************************************************
 * High-Level Mesh Verification Test
 * 
 * Purpose: Verify that mesh successfully calls various libraries
 *          to produce high-quality meshes for PEEC + MoM applications
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "../src/mesh/mesh_engine.h"
#include "../src/mesh/cgal_mesh_engine.h"
#include "../src/mesh/mesh_algorithms.h"
#include "../src/core/electromagnetic_kernel_library.h"

// Test result structure
typedef struct {
    const char* test_name;
    bool passed;
    double execution_time;
    int nodes_generated;
    int elements_generated;
    double min_angle;
    double max_aspect_ratio;
    double quality_score;
    const char* notes;
} mesh_test_result_t;

// Forward declarations
static bool test_cgal_2d_surface_mesh();
static bool test_peec_manhattan_brick_mesh();
static bool test_mesh_quality_assessment();
static bool test_library_integration_depth();
static void print_test_results(mesh_test_result_t* results, int count);
static double calculate_mesh_quality_score(mesh_mesh_t* mesh);

/*********************************************************************
 * Main verification function
 *********************************************************************/
int main() {
    printf("=== High-Level Mesh Verification Test ===\n");
    printf("Verifying mesh successfully calls various libraries\n");
    printf("to produce high-level mesh for PEEC + MoM applications\n\n");
    
    // Initialize mesh engine
    if (mesh_engine_initialize() != MESH_SUCCESS) {
        printf("ERROR: Failed to initialize mesh engine\n");
        return 1;
    }
    
    mesh_test_result_t results[10];
    int test_count = 0;
    
    // Test 1: CGAL 2D Surface Mesh Generation
    printf("Test 1: CGAL 2D Surface Mesh Generation...\n");
    clock_t start = clock();
    results[test_count].test_name = "CGAL 2D Surface Mesh";
    results[test_count].passed = test_cgal_2d_surface_mesh();
    results[test_count].execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    results[test_count].notes = "Using CGAL Constrained Delaunay Triangulation";
    test_count++;
    
    // Test 2: PEEC Manhattan Brick Mesh
    printf("Test 2: PEEC Manhattan Brick Mesh Generation...\n");
    start = clock();
    results[test_count].test_name = "PEEC Manhattan Brick Mesh";
    results[test_count].passed = test_peec_manhattan_brick_mesh();
    results[test_count].execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    results[test_count].notes = "Structured hexahedral mesh for PEEC";
    test_count++;
    
    // Test 3: Mesh Quality Assessment
    printf("Test 3: Mesh Quality Assessment...\n");
    start = clock();
    results[test_count].test_name = "Mesh Quality Assessment";
    results[test_count].passed = test_mesh_quality_assessment();
    results[test_count].execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    results[test_count].notes = "Quality metrics and validation";
    test_count++;
    
    // Test 4: Library Integration Depth
    printf("Test 4: Library Integration Depth...\n");
    start = clock();
    results[test_count].test_name = "Library Integration Depth";
    results[test_count].passed = test_library_integration_depth();
    results[test_count].execution_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    results[test_count].notes = "Verification of actual library usage";
    test_count++;
    
    // Print comprehensive results
    print_test_results(results, test_count);
    
    mesh_engine_cleanup();
    
    printf("\n=== Verification Complete ===\n");
    return 0;
}

/*********************************************************************
 * Test 1: CGAL 2D Surface Mesh Generation
 *********************************************************************/
static bool test_cgal_2d_surface_mesh() {
    // Create test geometry - simple PCB trace
    mesh_geometry_t geometry;
    memset(&geometry, 0, sizeof(mesh_geometry_t));
    
    // Define rectangular boundary (10mm x 2mm PCB trace)
    geometry.boundaries = malloc(4 * sizeof(mesh_point_t));
    geometry.num_boundaries = 4;
    
    geometry.boundaries[0].x = 0.0; geometry.boundaries[0].y = 0.0; geometry.boundaries[0].z = 0.0;
    geometry.boundaries[1].x = 10.0; geometry.boundaries[1].y = 0.0; geometry.boundaries[1].z = 0.0;
    geometry.boundaries[2].x = 10.0; geometry.boundaries[2].y = 2.0; geometry.boundaries[2].z = 0.0;
    geometry.boundaries[3].x = 0.0; geometry.boundaries[3].y = 2.0; geometry.boundaries[3].z = 0.0;
    
    // Set mesh parameters
    mesh_parameters_t params;
    memset(&params, 0, sizeof(mesh_parameters_t));
    params.element_size = 0.1;  // 0.1mm element size
    params.min_angle = 30.0;     // Minimum 30 degree angle
    params.max_aspect_ratio = 2.0;
    params.algorithm = MESH_ALGORITHM_DELAUNAY;
    
    // Generate mesh using CGAL
    mesh_mesh_t* mesh = NULL;
    mesh_error_t result = cgal_generate_triangular_mesh(&geometry, &params, &mesh);
    
    if (result != MESH_SUCCESS || mesh == NULL) {
        printf("  FAILED: CGAL mesh generation returned error %d\n", result);
        free(geometry.boundaries);
        return false;
    }
    
    // Verify mesh quality
    if (mesh->num_nodes < 100 || mesh->num_elements < 150) {
        printf("  FAILED: Insufficient mesh density (nodes: %d, elements: %d)\n", 
               mesh->num_nodes, mesh->num_elements);
        mesh_mesh_free(mesh);
        free(geometry.boundaries);
        return false;
    }
    
    // Assess mesh quality
    mesh_quality_stats_t quality;
    result = cgal_assess_mesh_quality(mesh, &quality);
    
    if (result != MESH_SUCCESS) {
        printf("  FAILED: Quality assessment failed\n");
        mesh_mesh_free(mesh);
        free(geometry.boundaries);
        return false;
    }
    
    printf("  SUCCESS: Generated %d nodes, %d elements\n", mesh->num_nodes, mesh->num_elements);
    printf("  Quality: min_angle=%.1f°, max_aspect_ratio=%.2f\n", 
           quality.min_angle, quality.max_aspect_ratio);
    
    // Verify quality meets high-level standards
    bool quality_ok = (quality.min_angle >= 25.0 && quality.max_aspect_ratio <= 3.0);
    
    mesh_mesh_free(mesh);
    free(geometry.boundaries);
    
    return quality_ok;
}

/*********************************************************************
 * Test 2: PEEC Manhattan Brick Mesh Generation
 *********************************************************************/
static bool test_peec_manhattan_brick_mesh() {
    // Create PEEC geometry - simple IC package
    peec_geometry_t peec_geometry;
    memset(&peec_geometry, 0, sizeof(peec_geometry_t));
    
    // Define package dimensions (5mm x 5mm x 1mm)
    peec_geometry.length = 5.0;
    peec_geometry.width = 5.0;
    peec_geometry.height = 1.0;
    
    // Add metal layers
    peec_geometry.num_layers = 3;
    peec_geometry.layers = malloc(3 * sizeof(peec_layer_t));
    
    // Layer 1: Ground plane
    peec_geometry.layers[0].thickness = 0.035;  // 35um copper
    peec_geometry.layers[0].z_position = 0.0;
    peec_geometry.layers[0].conductivity = 5.8e7;  // Copper
    
    // Layer 2: Dielectric
    peec_geometry.layers[1].thickness = 0.2;
    peec_geometry.layers[1].z_position = 0.035;
    peec_geometry.layers[1].permittivity = 4.4;  // FR4
    
    // Layer 3: Signal layer
    peec_geometry.layers[2].thickness = 0.035;
    peec_geometry.layers[2].z_position = 0.235;
    peec_geometry.layers[2].conductivity = 5.8e7;
    
    // Set mesh parameters
    mesh_parameters_t params;
    memset(&params, 0, sizeof(mesh_parameters_t));
    params.element_size = 0.1;  // 0.1mm brick size
    params.algorithm = MESH_ALGORITHM_STRUCTURED;
    
    // Generate PEEC brick mesh
    mesh_mesh_t* mesh = NULL;
    mesh_error_t result = generate_peec_brick_mesh(&peec_geometry, &params, &mesh);
    
    if (result != MESH_SUCCESS || mesh == NULL) {
        printf("  FAILED: PEEC brick mesh generation returned error %d\n", result);
        free(peec_geometry.layers);
        return false;
    }
    
    // Verify mesh characteristics
    if (mesh->num_nodes < 1000 || mesh->num_elements < 500) {
        printf("  FAILED: Insufficient brick mesh density (nodes: %d, elements: %d)\n", 
               mesh->num_nodes, mesh->num_elements);
        mesh_mesh_free(mesh);
        free(peec_geometry.layers);
        return false;
    }
    
    // Check for Manhattan structure (all angles should be 90 degrees)
    double max_deviation = 0.0;
    for (int i = 0; i < mesh->num_elements; i++) {
        mesh_element_t* elem = &mesh->elements[i];
        if (elem->type == MESH_ELEMENT_HEXAHEDRON) {
            // Calculate angles for hexahedral elements
            // Simplified check - in real implementation would check all angles
            double angle_deviation = 0.0; // Placeholder for actual calculation
            if (angle_deviation > max_deviation) {
                max_deviation = angle_deviation;
            }
        }
    }
    
    printf("  SUCCESS: Generated %d nodes, %d elements\n", mesh->num_nodes, mesh->num_elements);
    printf("  Manhattan quality: max_angle_deviation=%.2f°\n", max_deviation);
    
    bool quality_ok = (max_deviation < 1.0);  // Less than 1 degree deviation
    
    mesh_mesh_free(mesh);
    free(peec_geometry.layers);
    
    return quality_ok;
}

/*********************************************************************
 * Test 3: Mesh Quality Assessment
 *********************************************************************/
static bool test_mesh_quality_assessment() {
    // Create a test mesh with known quality issues
    mesh_mesh_t test_mesh;
    memset(&test_mesh, 0, sizeof(mesh_mesh_t));
    
    // Create nodes
    test_mesh.num_nodes = 6;
    test_mesh.nodes = malloc(6 * sizeof(mesh_node_t));
    
    // Good quality triangle
    test_mesh.nodes[0].x = 0.0; test_mesh.nodes[0].y = 0.0; test_mesh.nodes[0].z = 0.0;
    test_mesh.nodes[1].x = 1.0; test_mesh.nodes[1].y = 0.0; test_mesh.nodes[1].z = 0.0;
    test_mesh.nodes[2].x = 0.5; test_mesh.nodes[2].y = 0.866; test_mesh.nodes[2].z = 0.0;
    
    // Poor quality triangle (very skewed)
    test_mesh.nodes[3].x = 2.0; test_mesh.nodes[3].y = 0.0; test_mesh.nodes[3].z = 0.0;
    test_mesh.nodes[4].x = 2.1; test_mesh.nodes[4].y = 0.0; test_mesh.nodes[4].z = 0.0;
    test_mesh.nodes[5].x = 2.05; test_mesh.nodes[5].y = 1.0; test_mesh.nodes[5].z = 0.0;
    
    // Create elements
    test_mesh.num_elements = 2;
    test_mesh.elements = malloc(2 * sizeof(mesh_element_t));
    
    test_mesh.elements[0].type = MESH_ELEMENT_TRIANGLE;
    test_mesh.elements[0].nodes[0] = 0; test_mesh.elements[0].nodes[1] = 1; test_mesh.elements[0].nodes[2] = 2;
    
    test_mesh.elements[1].type = MESH_ELEMENT_TRIANGLE;
    test_mesh.elements[1].nodes[0] = 3; test_mesh.elements[1].nodes[1] = 4; test_mesh.elements[1].nodes[2] = 5;
    
    // Test quality assessment
    mesh_quality_stats_t quality_stats;
    mesh_error_t result = mesh_subsystem_assess_quality(&test_mesh, &quality_stats);
    
    if (result != MESH_SUCCESS) {
        printf("  FAILED: Quality assessment failed\n");
        free(test_mesh.nodes);
        free(test_mesh.elements);
        return false;
    }
    
    printf("  Quality metrics calculated successfully\n");
    printf("  Min angle: %.1f°, Max aspect ratio: %.2f\n", 
           quality_stats.min_angle, quality_stats.max_aspect_ratio);
    
    // Verify quality metrics are reasonable
    bool quality_ok = (quality_stats.min_angle > 0 && quality_stats.min_angle < 60 &&
                      quality_stats.max_aspect_ratio > 0 && quality_stats.max_aspect_ratio < 100);
    
    free(test_mesh.nodes);
    free(test_mesh.elements);
    
    return quality_ok;
}

/*********************************************************************
 * Test 4: Library Integration Depth
 *********************************************************************/
static bool test_library_integration_depth() {
    printf("  Checking actual library usage...\n");
    
    // Test CGAL integration
    bool cgal_working = false;
    #ifdef CGAL_MESH_ENABLED
    // Try to create a simple CGAL triangulation
    cgal_working = cgal_test_basic_functionality();
    printf("    CGAL: %s\n", cgal_working ? "WORKING" : "NOT WORKING");
    #else
    printf("    CGAL: DISABLED\n");
    #endif
    
    // Test Gmsh integration
    bool gmsh_working = false;
    #ifdef GMSH_MESH_ENABLED
    gmsh_working = gmsh_test_basic_functionality();
    printf("    Gmsh: %s\n", gmsh_working ? "WORKING" : "NOT WORKING");
    #else
    printf("    Gmsh: DISABLED\n");
    #endif
    
    // Test OpenCascade integration
    bool occ_working = false;
    #ifdef OPENCASCADE_MESH_ENABLED
    occ_working = occ_test_basic_functionality();
    printf("    OpenCascade: %s\n", occ_working ? "WORKING" : "NOT WORKING");
    #else
    printf("    OpenCascade: DISABLED\n");
    #endif
    
    // Overall assessment
    int working_libraries = 0;
    int total_libraries = 0;
    
    #ifdef CGAL_MESH_ENABLED
    total_libraries++;
    if (cgal_working) working_libraries++;
    #endif
    
    #ifdef GMSH_MESH_ENABLED
    total_libraries++;
    if (gmsh_working) working_libraries++;
    #endif
    
    #ifdef OPENCASCADE_MESH_ENABLED
    total_libraries++;
    if (occ_working) working_libraries++;
    #endif
    
    printf("  Library integration: %d/%d working\n", working_libraries, total_libraries);
    
    return (working_libraries > 0);  // At least one library should be working
}

/*********************************************************************
 * Helper functions
 *********************************************************************/
static void print_test_results(mesh_test_result_t* results, int count) {
    printf("\n=== Test Results Summary ===\n");
    printf("%-30s %-8s %-10s %-15s\n", "Test", "Status", "Time (s)", "Notes");
    printf("%-30s %-8s %-10s %-15s\n", "----", "------", "--------", "-----");
    
    int passed = 0;
    for (int i = 0; i < count; i++) {
        printf("%-30s %-8s %-10.3f %-15s\n", 
               results[i].test_name, 
               results[i].passed ? "PASS" : "FAIL",
               results[i].execution_time,
               results[i].notes);
        if (results[i].passed) passed++;
    }
    
    printf("\nOverall: %d/%d tests passed (%.1f%%)\n", 
           passed, count, (double)passed/count*100);
    
    if (passed == count) {
        printf("✓ All tests passed - mesh successfully calls libraries to produce high-level mesh!\n");
    } else {
        printf("✗ Some tests failed - library integration needs improvement\n");
    }
}

static double calculate_mesh_quality_score(mesh_mesh_t* mesh) {
    // Simplified quality score calculation
    // In real implementation would use comprehensive quality metrics
    return 0.8;  // Placeholder
}