/*********************************************************************
 * CAD Mesh Generation Integration Examples
 * 
 * This file demonstrates comprehensive usage of the CAD mesh generation
 * module for electromagnetic simulation applications.
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../src/cad/cad_mesh_generation.h"
#include "../src/io/advanced_file_formats.h"
#include "../src/io/parallel_io.h"
#include "../src/io/memory_optimization.h"

/*********************************************************************
 * Example 1: Basic CAD Geometry Import and Mesh Generation
 *********************************************************************/
int example_basic_cad_mesh_generation() {
    printf("=== Example 1: Basic CAD Geometry Import and Mesh Generation ===\n");
    
    // Initialize CAD mesh generator
    CADMeshGenerator* generator = cad_mesh_generator_create();
    if (!generator) {
        printf("Failed to create CAD mesh generator\n");
        return -1;
    }
    
    // Configure mesh parameters
    MeshParameters params = {
        .algorithm = MESH_ALGORITHM_DELAUNAY,
        .element_type = ELEMENT_TRIANGLE,
        .target_size = 0.1,  // 0.1 unit target element size
        .min_size = 0.05,
        .max_size = 0.5,
        .quality_threshold = 0.7,
        .max_iterations = 100,
        .adaptive_refinement = 1,
        .refinement_level = 2
    };
    
    // Import CAD geometry from DXF file
    const char* dxf_file = "pcb_antenna.dxf";
    CADGeometry* geometry = cad_geometry_import_from_dxf(generator, dxf_file);
    if (!geometry) {
        printf("Failed to import DXF file: %s\n", dxf_file);
        cad_mesh_generator_destroy(generator);
        return -1;
    }
    
    printf("Imported CAD geometry with %d entities\n", geometry->num_entities);
    
    // Generate mesh
    Mesh* mesh = cad_mesh_generate(generator, geometry, &params);
    if (!mesh) {
        printf("Failed to generate mesh\n");
        cad_geometry_destroy(geometry);
        cad_mesh_generator_destroy(generator);
        return -1;
    }
    
    // Analyze mesh quality
    MeshQuality quality = mesh_analyze_quality(mesh);
    printf("Generated mesh statistics:\n");
    printf("  Nodes: %d\n", mesh->num_nodes);
    printf("  Elements: %d\n", mesh->num_elements);
    printf("  Minimum quality: %.3f\n", quality.min_quality);
    printf("  Average quality: %.3f\n", quality.avg_quality);
    printf("  Maximum aspect ratio: %.3f\n", quality.max_aspect_ratio);
    
    // Export mesh for electromagnetic simulation
    const char* output_file = "pcb_antenna_mesh.msh";
    if (mesh_export_to_gmsh(mesh, output_file) == 0) {
        printf("Successfully exported mesh to %s\n", output_file);
    }
    
    // Cleanup
    mesh_destroy(mesh);
    cad_geometry_destroy(geometry);
    cad_mesh_generator_destroy(generator);
    
    return 0;
}

/*********************************************************************
 * Example 2: Multi-Layer PCB Mesh Generation with Material Properties
 *********************************************************************/
int example_multilayer_pcb_mesh() {
    printf("\n=== Example 2: Multi-Layer PCB Mesh Generation ===\n");
    
    CADMeshGenerator* generator = cad_mesh_generator_create();
    CADGeometry* geometry = cad_geometry_create();
    
    // Define PCB stackup (4-layer board)
    PCBLayer layers[] = {
        {.name = "Top", .thickness = 0.035, .material = "copper", .epsilon_r = 1.0, .conductivity = 5.8e7},
        {.name = "Dielectric1", .thickness = 0.2, .material = "FR4", .epsilon_r = 4.4, .conductivity = 0.0},
        {.name = "Inner1", .thickness = 0.035, .material = "copper", .epsilon_r = 1.0, .conductivity = 5.8e7},
        {.name = "Dielectric2", .thickness = 0.4, .material = "FR4", .epsilon_r = 4.4, .conductivity = 0.0},
        {.name = "Inner2", .thickness = 0.035, .material = "copper", .epsilon_r = 1.0, .conductivity = 5.8e7},
        {.name = "Dielectric3", .thickness = 0.2, .material = "FR4", .epsilon_r = 4.4, .conductivity = 0.0},
        {.name = "Bottom", .thickness = 0.035, .material = "copper", .epsilon_r = 1.0, .conductivity = 5.8e7}
    };
    
    int num_layers = sizeof(layers) / sizeof(PCBLayer);
    
    // Create via structures
    double via_radius = 0.1;
    double via_drill = 0.05;
    
    // Add microstrip transmission line on top layer
    CADEntity microstrip = {
        .type = CAD_LINE,
        .layer = 0,  // Top layer
        .material = &layers[0],
        .params.line.start_x = 0.0, .params.line.start_y = 0.0,
        .params.line.end_x = 10.0, .params.line.end_y = 0.0,
        .params.line.width = 0.3
    };
    cad_geometry_add_entity(geometry, &microstrip);
    
    // Add ground plane on bottom layer
    CADEntity ground_plane = {
        .type = CAD_RECTANGLE,
        .layer = 6,  // Bottom layer
        .material = &layers[6],
        .params.rectangle.x = -5.0, .params.rectangle.y = -5.0,
        .params.rectangle.width = 20.0, .params.rectangle.height = 10.0
    };
    cad_geometry_add_entity(geometry, &ground_plane);
    
    // Add via connecting top to bottom
    CADEntity via = {
        .type = CAD_CIRCLE,
        .layer = -1,  // All layers (through-hole)
        .material = NULL,  // Use layer materials
        .params.circle.x = 5.0, .params.circle.y = 0.0,
        .params.circle.radius = via_radius
    };
    cad_geometry_add_entity(geometry, &via);
    
    // Configure mesh parameters for PCB
    MeshParameters pcb_params = {
        .algorithm = MESH_ALGORITHM_DELAUNAY,
        .element_type = ELEMENT_TRIANGLE,
        .target_size = 0.05,  // Fine mesh for accurate EM simulation
        .min_size = 0.01,
        .max_size = 0.2,
        .quality_threshold = 0.8,
        .max_iterations = 200,
        .adaptive_refinement = 1,
        .refinement_level = 3,
        .preserve_edges = 1  // Important for conductor edges
    };
    
    // Generate mesh with material properties
    Mesh* mesh = cad_mesh_generate_with_materials(generator, geometry, &pcb_params, layers, num_layers);
    
    if (mesh) {
        printf("Generated multi-layer PCB mesh:\n");
        printf("  Total nodes: %d\n", mesh->num_nodes);
        printf("  Total elements: %d\n", mesh->num_elements);
        printf("  Layers processed: %d\n", num_layers);
        
        // Assign material properties to mesh regions
        mesh_assign_materials(mesh, layers, num_layers);
        
        // Export for electromagnetic simulation
        mesh_export_to_comsol(mesh, "multilayer_pcb.mph");
        mesh_export_to_hfss(mesh, "multilayer_pcb.aedt");
        
        mesh_destroy(mesh);
    }
    
    cad_geometry_destroy(geometry);
    cad_mesh_generator_destroy(generator);
    
    return 0;
}

/*********************************************************************
 * Example 3: Antenna Structure Mesh Generation with Adaptive Refinement
 *********************************************************************/
int example_antenna_mesh_adaptive() {
    printf("\n=== Example 3: Antenna Mesh with Adaptive Refinement ===\n");
    
    CADMeshGenerator* generator = cad_mesh_generator_create();
    CADGeometry* geometry = cad_geometry_create();
    
    // Create patch antenna geometry
    // Ground plane
    CADEntity ground = {
        .type = CAD_RECTANGLE,
        .layer = 0,
        .material = NULL,
        .params.rectangle.x = -20.0, .params.rectangle.y = -20.0,
        .params.rectangle.width = 40.0, .params.rectangle.height = 40.0
    };
    cad_geometry_add_entity(geometry, &ground);
    
    // Patch radiator
    CADEntity patch = {
        .type = CAD_RECTANGLE,
        .layer = 1,
        .material = NULL,
        .params.rectangle.x = -7.5, .params.rectangle.y = -5.0,
        .params.rectangle.width = 15.0, .params.rectangle.height = 10.0
    };
    cad_geometry_add_entity(geometry, &patch);
    
    // Feed point
    CADEntity feed = {
        .type = CAD_POINT,
        .layer = 1,
        .material = NULL,
        .params.point.x = 0.0, .params.point.y = -2.5
    };
    cad_geometry_add_entity(geometry, &feed);
    
    // Configure adaptive mesh parameters
    AdaptiveMeshParameters adaptive_params = {
        .base_params = {
            .algorithm = MESH_ALGORITHM_DELAUNAY,
            .element_type = ELEMENT_TRIANGLE,
            .target_size = 0.5,
            .min_size = 0.01,
            .max_size = 2.0,
            .quality_threshold = 0.85,
            .max_iterations = 500,
            .adaptive_refinement = 1,
            .refinement_level = 4
        },
        .refinement_criteria = REFINEMENT_CURVATURE | REFINEMENT_FIELD_GRADIENT | REFINEMENT_GEOMETRY_FEATURE,
        .curvature_threshold = 0.1,
        .field_gradient_threshold = 0.2,
        .feature_size_threshold = 0.05,
        .min_element_quality = 0.3,
        .max_refinement_levels = 5,
        .refinement_regions = NULL,
        .num_refinement_regions = 0
    };
    
    // Define refinement regions around critical areas
    RefinementRegion regions[] = {
        {
            .type = REGION_RECTANGLE,
            .params.rectangle = {.x = -7.5, .y = -5.0, .width = 15.0, .height = 10.0},
            .target_size = 0.05,
            .refinement_level = 3
        },
        {
            .type = REGION_CIRCLE,
            .params.circle = {.x = 0.0, .y = -2.5, .radius = 2.0},
            .target_size = 0.02,
            .refinement_level = 4
        }
    };
    
    adaptive_params.refinement_regions = regions;
    adaptive_params.num_refinement_regions = 2;
    
    // Generate mesh with adaptive refinement
    Mesh* mesh = cad_mesh_generate_adaptive(generator, geometry, &adaptive_params);
    
    if (mesh) {
        printf("Generated adaptive antenna mesh:\n");
        printf("  Nodes: %d\n", mesh->num_nodes);
        printf("  Elements: %d\n", mesh->num_elements);
        
        // Analyze mesh quality distribution
        MeshQualityDistribution quality_dist = mesh_analyze_quality_distribution(mesh);
        printf("  Quality distribution:\n");
        printf("    Excellent (0.9-1.0): %.1f%%\n", quality_dist.excellent_ratio * 100);
        printf("    Good (0.7-0.9): %.1f%%\n", quality_dist.good_ratio * 100);
        printf("    Fair (0.5-0.7): %.1f%%\n", quality_dist.fair_ratio * 100);
        printf("    Poor (0.3-0.5): %.1f%%\n", quality_dist.poor_ratio * 100);
        printf("    Bad (0.0-0.3): %.1f%%\n", quality_dist.bad_ratio * 100);
        
        // Export refined mesh
        mesh_export_to_gmsh(mesh, "patch_antenna_adaptive.msh");
        mesh_export_to_patran(mesh, "patch_antenna_adaptive.pat");
        
        mesh_destroy(mesh);
    }
    
    cad_geometry_destroy(geometry);
    cad_mesh_generator_destroy(generator);
    
    return 0;
}

/*********************************************************************
 * Example 4: High-Frequency Structure Mesh Generation
 *********************************************************************/
int example_hf_structure_mesh() {
    printf("\n=== Example 4: High-Frequency Structure Mesh ===\n");
    
    CADMeshGenerator* generator = cad_mesh_generator_create();
    CADGeometry* geometry = cad_geometry_create();
    
    // Create waveguide filter structure
    // Waveguide walls
    CADEntity waveguide_wall_1 = {
        .type = CAD_RECTANGLE,
        .layer = 0,
        .material = NULL,
        .params.rectangle.x = -10.0, .params.rectangle.y = -5.0,
        .params.rectangle.width = 20.0, .params.rectangle.height = 0.5
    };
    cad_geometry_add_entity(geometry, &waveguide_wall_1);
    
    CADEntity waveguide_wall_2 = {
        .type = CAD_RECTANGLE,
        .layer = 0,
        .material = NULL,
        .params.rectangle.x = -10.0, .params.rectangle.y = 4.5,
        .params.rectangle.width = 20.0, .params.rectangle.height = 0.5
    };
    cad_geometry_add_entity(geometry, &waveguide_wall_2);
    
    // Iris structures for filtering
    for (int i = 0; i < 3; i++) {
        CADEntity iris = {
            .type = CAD_RECTANGLE,
            .layer = 0,
            .material = NULL,
            .params.rectangle.x = -6.0 + i * 6.0, .params.rectangle.y = -2.0,
            .params.rectangle.width = 0.2, .params.rectangle.height = 4.0
        };
        cad_geometry_add_entity(geometry, &iris);
    }
    
    // Configure high-frequency mesh parameters
    HFMeshParameters hf_params = {
        .base_params = {
            .algorithm = MESH_ALGORITHM_DELAUNAY,
            .element_type = ELEMENT_TRIANGLE,
            .target_size = 0.02,  // Very fine mesh for HF
            .min_size = 0.005,
            .max_size = 0.1,
            .quality_threshold = 0.9,
            .max_iterations = 1000,
            .adaptive_refinement = 1,
            .refinement_level = 5
        },
        .frequency = 10.0e9,  // 10 GHz
        .wavelength = 0.03,   // 3cm wavelength in air
        .elements_per_wavelength = 20,
        .resolve_skin_depth = 1,
        .skin_depth_factor = 3.0,
        .resolve_edges = 1,
        .edge_resolution_factor = 0.1,
        .resolve_corners = 1,
        .corner_resolution_factor = 0.05
    };
    
    // Generate high-frequency mesh
    Mesh* mesh = cad_mesh_generate_hf(generator, geometry, &hf_params);
    
    if (mesh) {
        printf("Generated high-frequency structure mesh:\n");
        printf("  Nodes: %d\n", mesh->num_nodes);
        printf("  Elements: %d\n", mesh->num_elements);
        printf("  Frequency: %.1f GHz\n", hf_params.frequency / 1e9);
        printf("  Elements per wavelength: %d\n", hf_params.elements_per_wavelength);
        
        // Check if mesh satisfies HF requirements
        HFMeshSuitability suitability = mesh_check_hf_suitability(mesh, &hf_params);
        printf("  Mesh suitability analysis:\n");
        printf("    Resolves wavelength: %s\n", suitability.resolves_wavelength ? "YES" : "NO");
        printf("    Resolves skin depth: %s\n", suitability.resolves_skin_depth ? "YES" : "NO");
        printf("    Resolves geometry: %s\n", suitability.resolves_geometry ? "YES" : "NO");
        printf("    Overall quality: %.2f\n", suitability.overall_quality);
        
        mesh_export_to_cst(mesh, "waveguide_filter.cst");
        mesh_destroy(mesh);
    }
    
    cad_geometry_destroy(geometry);
    cad_mesh_generator_destroy(generator);
    
    return 0;
}

/*********************************************************************
 * Example 5: Integration with Parallel I/O for Large Structures
 *********************************************************************/
int example_parallel_large_structure() {
    printf("\n=== Example 5: Parallel Processing for Large Structures ===\n");
    
    // Initialize parallel I/O system
    ParallelIOConfig parallel_config = {
        .mode = IO_MODE_PARALLEL_THREADS,
        .num_threads = 8,
        .chunk_size = 1024 * 1024,  // 1MB chunks
        .use_memory_mapping = 1,
        .buffer_size = 64 * 1024 * 1024  // 64MB buffer
    };
    
    ParallelIOContext* io_context = parallel_io_init(&parallel_config);
    
    // Create CAD mesh generator with parallel support
    CADMeshGenerator* generator = cad_mesh_generator_create_parallel(io_context);
    
    // Load large CAD file using parallel I/O
    const char* large_cad_file = "large_pcb_panel.dxf";
    CADGeometry* geometry = cad_geometry_import_parallel(generator, large_cad_file);
    
    if (geometry) {
        printf("Loaded large CAD geometry:\n");
        printf("  Entities: %d\n", geometry->num_entities);
        printf("  File size: %.1f MB\n", geometry->file_size / (1024.0 * 1024.0));
        printf("  Loading time: %.2f seconds\n", geometry->load_time);
        
        // Configure mesh parameters for large structure
        MeshParameters large_params = {
            .algorithm = MESH_ALGORITHM_OCTREE,
            .element_type = ELEMENT_TRIANGLE,
            .target_size = 0.2,
            .min_size = 0.05,
            .max_size = 1.0,
            .quality_threshold = 0.6,
            .max_iterations = 100,
            .adaptive_refinement = 1,
            .refinement_level = 2,
            .use_parallel_processing = 1,
            .parallel_chunk_size = 1000  // Process 1000 elements per chunk
        };
        
        // Generate mesh with memory optimization
        MemoryOptimizationConfig mem_config = {
            .strategy = MEMORY_STRATEGY_HYBRID,
            .max_memory_usage = 2.0 * 1024 * 1024 * 1024,  // 2GB limit
            .use_compression = 1,
            .compression_threshold = 1024,  // Compress objects > 1KB
            .enable_garbage_collection = 1,
            .gc_threshold = 0.8  // GC when 80% memory used
        };
        
        memory_optimization_init(&mem_config);
        
        Mesh* mesh = cad_mesh_generate_parallel(generator, geometry, &large_params);
        
        if (mesh) {
            printf("Generated large structure mesh:\n");
            printf("  Nodes: %d\n", mesh->num_nodes);
            printf("  Elements: %d\n", mesh->num_elements);
            printf("  Memory usage: %.1f MB\n", mesh->memory_usage / (1024.0 * 1024.0));
            printf("  Generation time: %.2f seconds\n", mesh->generation_time);
            
            // Export with parallel I/O
            parallel_mesh_export(mesh, "large_pcb_panel_parallel.msh", io_context);
            
            mesh_destroy(mesh);
        }
        
        cad_geometry_destroy(geometry);
    }
    
    // Cleanup
    memory_optimization_cleanup();
    cad_mesh_generator_destroy(generator);
    parallel_io_cleanup(io_context);
    
    return 0;
}

/*********************************************************************
 * Main function to run all examples
 *********************************************************************/
int main() {
    printf("CAD Mesh Generation Integration Examples\n");
    printf("========================================\n\n");
    
    // Run all examples
    int result = 0;
    
    result |= example_basic_cad_mesh_generation();
    result |= example_multilayer_pcb_mesh();
    result |= example_antenna_mesh_adaptive();
    result |= example_hf_structure_mesh();
    result |= example_parallel_large_structure();
    
    if (result == 0) {
        printf("\nAll examples completed successfully!\n");
    } else {
        printf("\nSome examples failed with errors.\n");
    }
    
    return result;
}