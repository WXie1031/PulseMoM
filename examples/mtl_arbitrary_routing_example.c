/**
 * @file mtl_arbitrary_routing_example.c
 * @brief Example demonstrating MTL arbitrary routing capabilities
 * @details Shows how to create and analyze cables with complex 3D paths
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../../src/solvers/mtl/mtl_solver_module.h"

// Example: Create a serpentine cable path (like in automotive harness)
void example_serpentine_cable() {
    printf("=== Serpentine Cable Example ===\n");
    
    // Create MTL solver
    mtl_solver_t* solver = mtl_solver_create();
    if (!solver) {
        printf("Failed to create MTL solver\n");
        return;
    }
    
    // Configure solver
    mtl_solver_config_t config = {0};
    config.analysis_type = MTL_ANALYSIS_FREQUENCY;
    config.freq_start = 1e3;    // 1 kHz
    config.freq_stop = 1e8;     // 100 MHz
    config.freq_points = 100;
    config.skin_effect = true;
    config.proximity_effect = true;
    config.tolerance = 1e-6;
    config.num_threads = 4;
    
    mtl_solver_set_config(solver, &config);
    
    // Define serpentine path nodes (automotive harness style)
    double x_nodes[] = {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0};
    double y_nodes[] = {0.0, 0.3, 0.0, -0.3, 0.0, 0.3, 0.0};
    double z_nodes[] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    int num_nodes = 7;
    
    // Divisions per segment (finer resolution in curved sections)
    int segment_divisions[] = {20, 30, 30, 30, 30, 20};
    
    // Create geometry with arbitrary routing
    mtl_geometry_t geometry = {0};
    geometry.num_conductors = 4;  // 4-conductor cable
    geometry.conductor_radii = (double[]){1e-3, 1e-3, 1e-3, 1e-3};  // 1mm radius
    geometry.insulation_thickness = (double[]){0.5e-3, 0.5e-3, 0.5e-3, 0.5e-3};
    geometry.materials = (mtl_conductor_material_t[]){MTL_MATERIAL_COPPER, MTL_MATERIAL_COPPER, 
                                                       MTL_MATERIAL_COPPER, MTL_MATERIAL_COPPER};
    geometry.dielectrics = (mtl_dielectric_material_t[]){MTL_DIELECTRIC_PVC, MTL_DIELECTRIC_PVC,
                                                        MTL_DIELECTRIC_PVC, MTL_DIELECTRIC_PVC};
    
    // Create the arbitrary path
    int result = mtl_geometry_create_path(&geometry, num_nodes, x_nodes, y_nodes, z_nodes, segment_divisions);
    if (result != MTL_SUCCESS) {
        printf("Failed to create path: %s\n", mtl_error_string(result));
        mtl_solver_destroy(solver);
        return;
    }
    
    printf("Created serpentine path with %d segments\n", geometry.num_segments);
    printf("Total path length: %.3f meters\n", mtl_geometry_compute_total_length(&geometry));
    
    // Set geometry and analyze
    mtl_solver_set_geometry(solver, &geometry);
    
    result = mtl_solver_analyze(solver);
    if (result != MTL_SUCCESS) {
        printf("Analysis failed: %s\n", mtl_error_string(result));
    } else {
        printf("Analysis completed successfully\n");
        
        // Get and display results
        mtl_results_t* results = mtl_solver_get_results(solver);
        if (results) {
            printf("Number of conductors: %d\n", results->num_conductors);
            printf("Solve time: %.2f seconds\n", results->solve_time);
            printf("Memory usage: %.1f MB\n", results->memory_usage);
            
            // Analyze specific segments
            printf("\nSegment analysis:\n");
            for (int seg = 0; seg < geometry.num_segments; seg += 10) {
                double start_coords[3], end_coords[3], direction[3];
                mtl_geometry_get_segment_coordinates(&geometry, seg, start_coords, end_coords, direction);
                printf("Segment %d: (%.2f,%.2f,%.2f) -> (%.2f,%.2f,%.2f), length=%.3f\n",
                       seg, start_coords[0], start_coords[1], start_coords[2],
                       end_coords[0], end_coords[1], end_coords[2],
                       geometry.segments[seg].length);
            }
        }
    }
    
    // Cleanup
    mtl_solver_destroy(solver);
    printf("\n");
}

// Example: 3D spiral cable (helical path)
void example_helical_cable() {
    printf("=== Helical Cable Example ===\n");
    
    // Create MTL solver
    mtl_solver_t* solver = mtl_solver_create();
    if (!solver) {
        printf("Failed to create MTL solver\n");
        return;
    }
    
    // Configure solver for high-frequency analysis
    mtl_solver_config_t config = {0};
    config.analysis_type = MTL_ANALYSIS_FREQUENCY;
    config.freq_start = 1e6;    // 1 MHz
    config.freq_stop = 1e9;     // 1 GHz
    config.freq_points = 50;
    config.skin_effect = true;
    config.radiation_loss = true;
    config.tolerance = 1e-8;
    
    mtl_solver_set_config(solver, &config);
    
    // Create helical path
    int num_turns = 5;
    double radius = 0.1;  // 10cm radius
    double pitch = 0.05;  // 5cm pitch
    double height = num_turns * pitch;
    int points_per_turn = 20;
    int total_nodes = num_turns * points_per_turn + 1;
    
    double* x_nodes = malloc(total_nodes * sizeof(double));
    double* y_nodes = malloc(total_nodes * sizeof(double));
    double* z_nodes = malloc(total_nodes * sizeof(double));
    int* segment_divisions = malloc((total_nodes - 1) * sizeof(int));
    
    // Generate helical coordinates
    for (int i = 0; i < total_nodes; i++) {
        double t = (double)i / (total_nodes - 1);
        double angle = 2.0 * M_PI * num_turns * t;
        
        x_nodes[i] = radius * cos(angle);
        y_nodes[i] = radius * sin(angle);
        z_nodes[i] = height * t;
        
        if (i < total_nodes - 1) {
            segment_divisions[i] = 5;  // 5 divisions per segment
        }
    }
    
    // Create geometry
    mtl_geometry_t geometry = {0};
    geometry.num_conductors = 1;  // Single conductor
    geometry.conductor_radii = (double[]){2e-3};  // 2mm radius
    geometry.insulation_thickness = (double[]){1e-3};
    geometry.materials = (mtl_conductor_material_t[]){MTL_MATERIAL_COPPER};
    geometry.dielectrics = (mtl_dielectric_material_t[]){MTL_DIELECTRIC_PTFE};
    
    // Create the helical path
    int result = mtl_geometry_create_path(&geometry, total_nodes, x_nodes, y_nodes, z_nodes, segment_divisions);
    if (result != MTL_SUCCESS) {
        printf("Failed to create helical path: %s\n", mtl_error_string(result));
        free(x_nodes); free(y_nodes); free(z_nodes); free(segment_divisions);
        mtl_solver_destroy(solver);
        return;
    }
    
    printf("Created helical path with %d segments\n", geometry.num_segments);
    printf("Total path length: %.3f meters\n", mtl_geometry_compute_total_length(&geometry));
    
    // Analyze specific segments at different frequencies
    printf("\nFrequency-dependent segment analysis:\n");
    mtl_solver_set_geometry(solver, &geometry);
    
    double test_freqs[] = {1e6, 10e6, 100e6, 500e6, 1e9};  // 1MHz to 1GHz
    int num_test_freqs = 5;
    
    for (int f = 0; f < num_test_freqs; f++) {
        printf("Frequency: %.0f MHz\n", test_freqs[f] / 1e6);
        
        // Analyze middle segment
        int middle_segment = geometry.num_segments / 2;
        result = mtl_solver_analyze_segment(solver, middle_segment, test_freqs[f]);
        
        if (result == MTL_SUCCESS) {
            double r_unit, l_unit, c_unit, g_unit;
            mtl_solver_compute_segment_parameters(solver, middle_segment, &r_unit, &l_unit, &c_unit, &g_unit);
            printf("  Segment %d: R=%.3e Ω/m, L=%.3e μH/m, C=%.3e pF/m\n",
                   middle_segment, r_unit, l_unit * 1e6, c_unit * 1e12);
        }
    }
    
    // Cleanup
    free(x_nodes); free(y_nodes); free(z_nodes); free(segment_divisions);
    mtl_solver_destroy(solver);
    printf("\n");
}

// Example: Complex harness with branches
void example_harness_with_branches() {
    printf("=== Complex Harness with Branches Example ===\n");
    
    // This example demonstrates how to create a complex harness
    // by combining multiple MTL segments with different geometries
    
    // Create main trunk
    mtl_geometry_t main_trunk = {0};
    main_trunk.num_conductors = 8;  // 8-conductor main trunk
    main_trunk.conductor_radii = malloc(8 * sizeof(double));
    main_trunk.insulation_thickness = malloc(8 * sizeof(double));
    main_trunk.materials = malloc(8 * sizeof(mtl_conductor_material_t));
    main_trunk.dielectrics = malloc(8 * sizeof(mtl_dielectric_material_t));
    
    for (int i = 0; i < 8; i++) {
        main_trunk.conductor_radii[i] = (i < 4) ? 1e-3 : 0.5e-3;  // Mixed conductor sizes
        main_trunk.insulation_thickness[i] = 0.5e-3;
        main_trunk.materials[i] = MTL_MATERIAL_COPPER;
        main_trunk.dielectrics[i] = MTL_DIELECTRIC_PVC;
    }
    
    // Define main trunk path
    double trunk_x[] = {0.0, 1.0, 2.0, 3.0};
    double trunk_y[] = {0.0, 0.1, 0.2, 0.3};
    double trunk_z[] = {0.0, 0.0, 0.0, 0.0};
    int trunk_div[] = {25, 30, 25};
    
    mtl_geometry_create_path(&main_trunk, 4, trunk_x, trunk_y, trunk_z, trunk_div);
    
    printf("Created main trunk: %d conductors, %d segments, %.2f m\n",
           main_trunk.num_conductors, main_trunk.num_segments, 
           mtl_geometry_compute_total_length(&main_trunk));
    
    // Create branch 1 (power lines)
    mtl_geometry_t branch1 = {0};
    branch1.num_conductors = 2;
    branch1.conductor_radii = (double[]){2e-3, 2e-3};  // Power conductors
    branch1.insulation_thickness = (double[]){1e-3, 1e-3};
    branch1.materials = (mtl_conductor_material_t[]){MTL_MATERIAL_COPPER, MTL_MATERIAL_COPPER};
    branch1.dielectrics = (mtl_dielectric_material_t[]){MTL_DIELECTRIC_XLPE, MTL_DIELECTRIC_XLPE};
    
    double branch1_x[] = {2.0, 2.5, 3.0};  // Branch from main trunk at x=2m
    double branch1_y[] = {0.2, 0.5, 0.8};
    double branch1_z[] = {0.0, 0.1, 0.2};
    int branch1_div[] = {15, 20};
    
    mtl_geometry_create_path(&branch1, 3, branch1_x, branch1_y, branch1_z, branch1_div);
    
    printf("Created branch 1 (power): %d conductors, %d segments, %.2f m\n",
           branch1.num_conductors, branch1.num_segments,
           mtl_geometry_compute_total_length(&branch1));
    
    // Display segment details
    printf("\nSegment details:\n");
    for (int seg = 0; seg < main_trunk.num_segments; seg++) {
        double start[3], end[3], dir[3];
        mtl_geometry_get_segment_coordinates(&main_trunk, seg, start, end, dir);
        printf("Main trunk segment %d: (%.2f,%.2f,%.2f) -> (%.2f,%.2f,%.2f)\n",
               seg, start[0], start[1], start[2], end[0], end[1], end[2]);
    }
    
    // Cleanup
    free(main_trunk.conductor_radii);
    free(main_trunk.insulation_thickness);
    free(main_trunk.materials);
    free(main_trunk.dielectrics);
    
    printf("\nHarness example completed\n");
}

int main() {
    printf("MTL Arbitrary Routing Examples\n");
    printf("==============================\n\n");
    
    // Run examples
    example_serpentine_cable();
    example_helical_cable();
    example_harness_with_branches();
    
    printf("All examples completed successfully!\n");
    return 0;
}