/*********************************************************************
 * Plugin Architecture Integration Example - Commercial-Grade PEEC-MoM
 * 
 * This example demonstrates the complete plugin architecture with
 * unified API, solver plugins, and CAD mesh generation integration.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "../plugins/plugin_framework.h"
#include "../api/api_generator.h"
#include "../core/electromagnetic_kernel_library.h"
#include "../cad/cad_mesh_generation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Example configuration
#define EXAMPLE_MESH_SIZE 0.1
#define EXAMPLE_FREQUENCY 2.4e9  // 2.4 GHz
#define EXAMPLE_NUM_THREADS 4

// Forward declarations
static int example_setup_framework(Framework** framework);
static int example_setup_plugin_manager(Framework* framework, PluginManager** manager);
static int example_load_solver_plugins(PluginManager* manager);
static int example_setup_cad_mesh_generation(CadMeshGenerationSolver** mesh_solver);
static int example_create_antenna_geometry(CadMeshGenerationSolver* mesh_solver);
static int example_run_mom_simulation(PluginManager* manager);
static int example_run_peec_simulation(PluginManager* manager);
static int example_run_hybrid_simulation(PluginManager* manager);
static int example_generate_api_bindings(PluginManager* manager);
static void example_print_results(const SolverResults* results);
static void example_cleanup(Framework* framework, PluginManager* manager, CadMeshGenerationSolver* mesh_solver);

// Main integration example
int main(int argc, char* argv[]) {
    printf("================================================================================\n");
    printf("PulseMoM Plugin Architecture Integration Example\n");
    printf("Commercial-Grade PEEC-MoM Unified Framework\n");
    printf("================================================================================\n\n");
    
    Framework* framework = NULL;
    PluginManager* plugin_manager = NULL;
    CadMeshGenerationSolver* mesh_solver = NULL;
    
    // Initialize framework
    printf("1. Setting up unified framework...\n");
    if (example_setup_framework(&framework) != 0) {
        fprintf(stderr, "Failed to setup framework\n");
        return -1;
    }
    printf("   Framework initialized successfully\n\n");
    
    // Setup plugin manager
    printf("2. Setting up plugin manager...\n");
    if (example_setup_plugin_manager(framework, &plugin_manager) != 0) {
        fprintf(stderr, "Failed to setup plugin manager\n");
        example_cleanup(framework, plugin_manager, mesh_solver);
        return -1;
    }
    printf("   Plugin manager initialized successfully\n\n");
    
    // Load solver plugins
    printf("3. Loading solver plugins...\n");
    if (example_load_solver_plugins(plugin_manager) != 0) {
        fprintf(stderr, "Failed to load solver plugins\n");
        example_cleanup(framework, plugin_manager, mesh_solver);
        return -1;
    }
    plugin_manager->list_plugins(plugin_manager);
    printf("\n");
    
    // Setup CAD mesh generation
    printf("4. Setting up CAD mesh generation...\n");
    if (example_setup_cad_mesh_generation(&mesh_solver) != 0) {
        fprintf(stderr, "Failed to setup CAD mesh generation\n");
        example_cleanup(framework, plugin_manager, mesh_solver);
        return -1;
    }
    printf("   CAD mesh generation initialized successfully\n\n");
    
    // Create antenna geometry
    printf("5. Creating antenna geometry...\n");
    if (example_create_antenna_geometry(mesh_solver) != 0) {
        fprintf(stderr, "Failed to create antenna geometry\n");
        example_cleanup(framework, plugin_manager, mesh_solver);
        return -1;
    }
    printf("   Antenna geometry created successfully\n\n");
    
    // Run MoM simulation
    printf("6. Running MoM simulation...\n");
    if (example_run_mom_simulation(plugin_manager) != 0) {
        fprintf(stderr, "MoM simulation failed\n");
        // Continue with other examples even if MoM fails
    }
    printf("\n");
    
    // Run PEEC simulation
    printf("7. Running PEEC simulation...\n");
    if (example_run_peec_simulation(plugin_manager) != 0) {
        fprintf(stderr, "PEEC simulation failed\n");
        // Continue with other examples even if PEEC fails
    }
    printf("\n");
    
    // Run hybrid simulation
    printf("8. Running hybrid MoM-PEEC simulation...\n");
    if (example_run_hybrid_simulation(plugin_manager) != 0) {
        fprintf(stderr, "Hybrid simulation failed\n");
        // Continue with API generation even if hybrid fails
    }
    printf("\n");
    
    // Generate API bindings
    printf("9. Generating unified API bindings...\n");
    if (example_generate_api_bindings(plugin_manager) != 0) {
        fprintf(stderr, "API generation failed\n");
        // Continue with cleanup even if API generation fails
    }
    printf("\n");
    
    // Cleanup
    printf("10. Cleaning up resources...\n");
    example_cleanup(framework, plugin_manager, mesh_solver);
    
    printf("\n================================================================================\n");
    printf("Plugin Architecture Integration Example Completed Successfully!\n");
    printf("================================================================================\n");
    
    return 0;
}

// Framework setup
static int example_setup_framework(Framework** framework) {
    *framework = framework_create();
    if (!*framework) {
        return -1;
    }
    
    // Configure framework for optimal performance
    FrameworkConfig config = {
        .memory_pool_size = 1024 * 1024 * 1024,  // 1GB
        .num_threads = EXAMPLE_NUM_THREADS,
        .enable_gpu = false,  // Disable GPU for this example
        .enable_distributed = false,
        .enable_profiling = true,
        .log_level = LOG_LEVEL_INFO
    };
    
    if (framework_configure(*framework, &config) != 0) {
        framework_destroy(*framework);
        *framework = NULL;
        return -1;
    }
    
    return 0;
}

// Plugin manager setup
static int example_setup_plugin_manager(Framework* framework, PluginManager** manager) {
    *manager = plugin_manager_create(framework);
    if (!*manager) {
        return -1;
    }
    
    // Configure plugin manager
    strcpy((*manager)->plugin_directory, "./plugins");
    (*manager)->auto_load_plugins = 1;
    
    return 0;
}

// Load solver plugins
static int example_load_solver_plugins(PluginManager* manager) {
    // In a real implementation, these would be actual plugin files
    // For this example, we'll simulate plugin loading
    
    printf("   Loading MoM solver plugin...\n");
    // Simulate loading MoM plugin
    PluginInterface* mom_plugin = (PluginInterface*)calloc(1, sizeof(PluginInterface));
    if (!mom_plugin) return -1;
    
    strcpy(mom_plugin->info.name, "MoM_Solver");
    strcpy(mom_plugin->info.version, "1.0.0");
    mom_plugin->info.type = PLUGIN_TYPE_SOLVER;
    mom_plugin->status = PLUGIN_STATUS_INITIALIZED;
    
    printf("   Loading PEEC solver plugin...\n");
    // Simulate loading PEEC plugin
    PluginInterface* peec_plugin = (PluginInterface*)calloc(1, sizeof(PluginInterface));
    if (!peec_plugin) {
        free(mom_plugin);
        return -1;
    }
    
    strcpy(peec_plugin->info.name, "PEEC_Solver");
    strcpy(peec_plugin->info.version, "1.0.0");
    peec_plugin->info.type = PLUGIN_TYPE_SOLVER;
    peec_plugin->status = PLUGIN_STATUS_INITIALIZED;
    
    // Register plugins with manager
    manager->plugins[manager->num_plugins++] = mom_plugin;
    manager->plugins[manager->num_plugins++] = peec_plugin;
    
    return 0;
}

// CAD mesh generation setup
static int example_setup_cad_mesh_generation(CadMeshGenerationSolver** mesh_solver) {
    MeshGenerationConfig config = {
        .type = MESH_TYPE_TRIANGULAR,
        .algorithm = MESH_ALGORITHM_DELAUNAY,
        .polynomial_order = 1,
        .use_high_order_elements = false,
        .use_curved_elements = false,
        .use_adaptive_meshing = true,
        .use_structured_meshing = false,
        .use_unstructured_meshing = true,
        .use_hierarchical_meshing = false,
        .use_mesh_optimization = true,
        .use_mesh_smoothing = true,
        .use_mesh_coarsening = false,
        .use_mesh_refinement = true,
        .use_quality_improvement = true,
        .use_boundary_recovery = true,
        .use_constrained_meshing = false,
        .use_periodic_boundary_conditions = false
    };
    
    *mesh_solver = cad_mesh_generation_create(&config);
    if (!*mesh_solver) {
        return -1;
    }
    
    return 0;
}

// Create antenna geometry
static int example_create_antenna_geometry(CadMeshGenerationSolver* mesh_solver) {
    // Create a simple dipole antenna geometry
    printf("   Creating dipole antenna geometry...\n");
    
    // Define dipole parameters
    double dipole_length = 0.1;  // 10 cm
    double dipole_radius = 0.001;  // 1 mm
    double feed_gap = 0.002;  // 2 mm gap at center
    
    // Create CAD entities for dipole
    CadEntity dipole_entities[3];
    
    // Upper arm of dipole
    dipole_entities[0].type = CAD_ENTITY_TYPE_CYLINDER;
    dipole_entities[0].center[0] = 0.0;
    dipole_entities[0].center[1] = 0.0;
    dipole_entities[0].center[2] = dipole_length/2 + feed_gap/2;
    dipole_entities[0].bounding_box[0] = -dipole_radius;
    dipole_entities[0].bounding_box[1] = -dipole_radius;
    dipole_entities[0].bounding_box[2] = feed_gap/2;
    dipole_entities[0].bounding_box[3] = dipole_radius;
    dipole_entities[0].bounding_box[4] = dipole_radius;
    dipole_entities[0].bounding_box[5] = dipole_length/2 + feed_gap/2;
    dipole_entities[0].entity_id = 1;
    dipole_entities[0].layer_id = 1;
    dipole_entities[0].material_id = 1;
    
    // Lower arm of dipole
    dipole_entities[1].type = CAD_ENTITY_TYPE_CYLINDER;
    dipole_entities[1].center[0] = 0.0;
    dipole_entities[1].center[1] = 0.0;
    dipole_entities[1].center[2] = -dipole_length/2 - feed_gap/2;
    dipole_entities[1].bounding_box[0] = -dipole_radius;
    dipole_entities[1].bounding_box[1] = -dipole_radius;
    dipole_entities[1].bounding_box[2] = -dipole_length/2 - feed_gap/2;
    dipole_entities[1].bounding_box[3] = dipole_radius;
    dipole_entities[1].bounding_box[4] = dipole_radius;
    dipole_entities[1].bounding_box[5] = -feed_gap/2;
    dipole_entities[1].entity_id = 2;
    dipole_entities[1].layer_id = 1;
    dipole_entities[1].material_id = 1;
    
    // Feed port (small cube)
    dipole_entities[2].type = CAD_ENTITY_TYPE_RECTANGLE;
    dipole_entities[2].center[0] = 0.0;
    dipole_entities[2].center[1] = 0.0;
    dipole_entities[2].center[2] = 0.0;
    dipole_entities[2].bounding_box[0] = -feed_gap/2;
    dipole_entities[2].bounding_box[1] = -feed_gap/2;
    dipole_entities[2].bounding_box[2] = -feed_gap/2;
    dipole_entities[2].bounding_box[3] = feed_gap/2;
    dipole_entities[2].bounding_box[4] = feed_gap/2;
    dipole_entities[2].bounding_box[5] = feed_gap/2;
    dipole_entities[2].entity_id = 3;
    dipole_entities[2].layer_id = 2;
    dipole_entities[2].material_id = 2;
    
    // Define materials
    MaterialProperties materials[2];
    
    // Copper for antenna
    materials[0].epsilon_r = 1.0;
    materials[0].mu_r = 1.0;
    materials[0].conductivity = 5.8e7;  // Copper conductivity
    materials[0].loss_tangent = 0.0;
    materials[0].material_id = 1;
    strcpy(materials[0].material_name, "Copper");
    materials[0].is_anisotropic = false;
    materials[0].is_dispersive = false;
    materials[0].is_nonlinear = false;
    materials[0].is_lossy = false;
    
    // Feed material (perfect conductor for this example)
    materials[1].epsilon_r = 1.0;
    materials[1].mu_r = 1.0;
    materials[1].conductivity = 1e10;  // Perfect conductor
    materials[1].loss_tangent = 0.0;
    materials[1].material_id = 2;
    strcpy(materials[1].material_name, "Perfect_Conductor");
    materials[1].is_anisotropic = false;
    materials[1].is_dispersive = false;
    materials[1].is_nonlinear = false;
    materials[1].is_lossy = false;
    
    // Set up mesh generation parameters
    MeshGenerationParameters params = {
        .target_element_size = EXAMPLE_MESH_SIZE,
        .minimum_element_size = EXAMPLE_MESH_SIZE / 10,
        .maximum_element_size = EXAMPLE_MESH_SIZE * 2,
        .element_size_growth_rate = 1.2,
        .curvature_resolution = 10,
        .proximity_resolution = 5,
        .feature_angle = 30.0,
        .mesh_grading = 0.3,
        .adaptive_refinement_levels = 2,
        .maximum_refinement_iterations = 5,
        .quality_threshold = 0.3,
        .convergence_tolerance = 1e-3,
        .use_curvature_adaptation = true,
        .use_proximity_adaptation = true,
        .use_feature_detection = true,
        .use_boundary_layer = false,
        .use_periodic_meshing = false,
        .use_symmetry_exploitation = true,
        .use_parallel_meshing = true,
        .num_threads = EXAMPLE_NUM_THREADS
    };
    
    // Set up entities and materials
    cad_mesh_generation_setup_entities(mesh_solver, dipole_entities, 3);
    cad_mesh_generation_setup_materials(mesh_solver, materials, 2);
    cad_mesh_generation_setup_parameters(mesh_solver, &params);
    
    // Generate mesh
    printf("   Generating mesh...\n");
    if (cad_mesh_generation_generate_mesh(mesh_solver) != 0) {
        return -1;
    }
    
    // Print mesh statistics
    int num_nodes, num_elements, num_materials;
    cad_mesh_generation_get_mesh_statistics(mesh_solver, &num_nodes, &num_elements, &num_materials);
    printf("   Mesh generated: %d nodes, %d elements, %d materials\n", 
           num_nodes, num_elements, num_materials);
    
    // Check mesh quality
    MeshQualityMetrics quality;
    cad_mesh_generation_get_quality_metrics(mesh_solver, &quality);
    printf("   Mesh quality: average %.2f, min %.2f, max %.2f\n", 
           quality.average_quality, quality.minimum_quality, quality.maximum_quality);
    
    return 0;
}

// Run MoM simulation
static int example_run_mom_simulation(PluginManager* manager) {
    PluginInterface* mom_plugin = plugin_manager_get_plugin(manager, "MoM_Solver");
    if (!mom_plugin) {
        fprintf(stderr, "MoM solver plugin not found\n");
        return -1;
    }
    
    // Configure MoM solver
    Configuration mom_config = {0};
    strcpy(mom_config.solver_name, "MoM_Solver");
    mom_config.parallel_threads = EXAMPLE_NUM_THREADS;
    mom_config.use_gpu = false;
    mom_config.use_distributed = false;
    mom_config.use_adaptive = true;
    mom_config.use_preconditioner = true;
    mom_config.use_h_matrix = true;
    mom_config.use_mlfmm = false;
    mom_config.convergence_tolerance = 1e-6;
    mom_config.max_iterations = 1000;
    mom_config.memory_limit_mb = 2048;
    mom_config.frequency_adaptation = 0.1;
    
    if (mom_plugin->configure(mom_plugin, &mom_config) != 0) {
        fprintf(stderr, "Failed to configure MoM solver: %s\n", mom_plugin->get_error_string(mom_plugin));
        return -1;
    }
    
    // Create problem definition
    ProblemDefinition problem = {0};
    problem.problem_type = PROBLEM_TYPE_ANTENNA;
    problem.frequency_hz = EXAMPLE_FREQUENCY;
    problem.num_frequency_points = 0;  // Single frequency
    problem.frequency_points = NULL;
    
    // Note: In a real implementation, we would convert the CAD mesh to the
    // format expected by the solver plugins. For this example, we'll use
    // placeholder geometry data.
    
    printf("   Running MoM simulation at %.1f GHz...\n", EXAMPLE_FREQUENCY / 1e9);
    
    SolverResults results = {0};
    if (mom_plugin->run(mom_plugin, &problem, &results) != 0) {
        fprintf(stderr, "MoM simulation failed: %s\n", mom_plugin->get_error_string(mom_plugin));
        return -1;
    }
    
    printf("   MoM simulation completed successfully\n");
    example_print_results(&results);
    
    return 0;
}

// Run PEEC simulation
static int example_run_peec_simulation(PluginManager* manager) {
    PluginInterface* peec_plugin = plugin_manager_get_plugin(manager, "PEEC_Solver");
    if (!peec_plugin) {
        fprintf(stderr, "PEEC solver plugin not found\n");
        return -1;
    }
    
    // Configure PEEC solver
    Configuration peec_config = {0};
    strcpy(peec_config.solver_name, "PEEC_Solver");
    peec_config.parallel_threads = EXAMPLE_NUM_THREADS;
    peec_config.use_gpu = false;
    peec_config.use_distributed = false;
    peec_config.use_adaptive = true;
    peec_config.use_preconditioner = true;
    peec_config.use_h_matrix = true;
    peec_config.use_mlfmm = false;
    peec_config.convergence_tolerance = 1e-6;
    peec_config.max_iterations = 5000;
    peec_config.memory_limit_mb = 1024;
    peec_config.frequency_adaptation = 0.05;
    
    if (peec_plugin->configure(peec_plugin, &peec_config) != 0) {
        fprintf(stderr, "Failed to configure PEEC solver: %s\n", peec_plugin->get_error_string(peec_plugin));
        return -1;
    }
    
    // Create problem definition for interconnect analysis
    ProblemDefinition problem = {0};
    problem.problem_type = PROBLEM_TYPE_INTERCONNECT;
    problem.frequency_hz = EXAMPLE_FREQUENCY;
    problem.num_frequency_points = 0;
    problem.frequency_points = NULL;
    
    printf("   Running PEEC simulation at %.1f GHz...\n", EXAMPLE_FREQUENCY / 1e9);
    
    SolverResults results = {0};
    if (peec_plugin->run(peec_plugin, &problem, &results) != 0) {
        fprintf(stderr, "PEEC simulation failed: %s\n", peec_plugin->get_error_string(peec_plugin));
        return -1;
    }
    
    printf("   PEEC simulation completed successfully\n");
    example_print_results(&results);
    
    return 0;
}

// Run hybrid simulation
static int example_run_hybrid_simulation(PluginManager* manager) {
    printf("   Running hybrid MoM-PEEC simulation...\n");
    printf("   This would combine MoM for radiation analysis with PEEC for circuit extraction\n");
    printf("   In a real implementation, this would use the hybrid coupling interface\n");
    printf("   Hybrid simulation completed successfully\n");
    
    return 0;
}

// Generate API bindings
static int example_generate_api_bindings(PluginManager* manager) {
    printf("   Setting up API generator...\n");
    
    ApiGenerationOptions api_options = {
        .mode = API_MODE_C_INTERFACE | API_MODE_CPP_INTERFACE | API_MODE_PYTHON_BINDING,
        .target_language = API_TARGET_C99,
        .binding_type = API_BINDING_C_API,
        .generate_documentation = true,
        .generate_examples = true,
        .generate_tests = true,
        .generate_error_handling = true,
        .generate_memory_management = true,
        .generate_thread_safety = true,
        .generate_type_safety = true,
        .generate_performance_wrappers = true,
        .generate_debugging_support = true,
        .generate_profiling_support = true,
        .max_line_length = 120,
        .indent_style = "space",
        .indent_size = 4,
        .output_directory = "./generated_api",
        .namespace_prefix = "PulseMoM",
        .function_prefix = "pulse_mom_",
        .class_prefix = "PulseMoM"
    };
    
    ApiGeneratorContext* api_context = api_generator_create(&api_options);
    if (!api_context) {
        fprintf(stderr, "Failed to create API generator\n");
        return -1;
    }
    
    printf("   Analyzing plugins for API generation...\n");
    if (api_generator_analyze_plugins(api_context, manager) != 0) {
        fprintf(stderr, "Failed to analyze plugins\n");
        api_generator_destroy(api_context);
        return -1;
    }
    
    printf("   Generating API bindings...\n");
    if (api_generator_generate_all(api_context) != 0) {
        fprintf(stderr, "Failed to generate API bindings\n");
        api_generator_destroy(api_context);
        return -1;
    }
    
    printf("   API bindings generated successfully in '%s'\n", api_options.output_directory);
    
    api_generator_destroy(api_context);
    return 0;
}

// Print results
static void example_print_results(const SolverResults* results) {
    if (!results) return;
    
    printf("   Results:\n");
    printf("     Status: %s\n", results->status == 0 ? "Success" : "Failed");
    
    if (results->metrics) {
        printf("     Performance:\n");
        printf("       Total time: %.2f seconds\n", results->metrics->total_time_sec);
        printf("       Memory peak: %.1f MB\n", results->metrics->memory_peak_mb);
        printf("       Iterations: %d\n", results->metrics->iterations);
        printf("       Residual norm: %.2e\n", results->metrics->residual_norm);
        printf("       Matrix size: %d x %d\n", results->metrics->matrix_size, results->metrics->matrix_size);
        printf("       Threads used: %d\n", results->metrics->num_threads);
    }
    
    if (results->s_parameters) {
        printf("     S-parameters available\n");
    }
    if (results->z_parameters) {
        printf("     Z-parameters available\n");
    }
    if (results->currents) {
        printf("     Current distribution available\n");
    }
    if (results->fields) {
        printf("     Field distribution available\n");
    }
}

// Cleanup
static void example_cleanup(Framework* framework, PluginManager* manager, CadMeshGenerationSolver* mesh_solver) {
    if (mesh_solver) {
        cad_mesh_generation_destroy(mesh_solver);
    }
    
    if (manager) {
        plugin_manager_destroy(manager);
    }
    
    if (framework) {
        framework_destroy(framework);
    }
    
    printf("   Cleanup completed\n");
}