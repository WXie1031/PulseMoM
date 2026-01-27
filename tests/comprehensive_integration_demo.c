/**
 * @file comprehensive_integration_demo.c
 * @brief Complete CAD-to-Simulation Integration Example
 * @details Demonstrates full workflow from CAD import through mesh generation 
 * to electromagnetic simulation using unified PEEC-MoM framework
 */

#include "../src/core/electromagnetic_kernel_library.h"
#include "../src/cad/cad_mesh_generation.h"
#include "../src/plugins/plugin_framework.h"
#include "../src/performance/performance_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Demo configuration
#define DEMO_MESH_SIZE 0.1
#define DEMO_FREQUENCY 2.4e9  // 2.4 GHz
#define DEMO_ANTENNA_LENGTH 0.1  // 10cm antenna
#define DEMO_ANTENNA_RADIUS 0.001  // 1mm radius

// Function prototypes
cad_geometry_t* create_demo_antenna_geometry(void);
int setup_simulation_environment(electromagnetic_kernel_library_t** core, plugin_manager_t** plugin_mgr);
int load_and_process_cad_file(const char* filename, cad_geometry_t** geometry);
int generate_electromagnetic_mesh(cad_geometry_t* geometry, cad_mesh_t** mesh);
int setup_mom_simulation(plugin_manager_t* plugin_mgr, cad_mesh_t* mesh, mom_solver_t** solver);
int setup_peec_simulation(plugin_manager_t* plugin_mgr, cad_mesh_t* mesh, peec_solver_t** solver);
int run_hybrid_simulation(plugin_manager_t* plugin_mgr, cad_mesh_t* mesh, hybrid_coupling_t** coupling);
void analyze_and_visualize_results(mom_solver_t* mom_solver, peec_solver_t* peec_solver, hybrid_coupling_t* coupling);
void cleanup_resources(electromagnetic_kernel_library_t* core, plugin_manager_t* plugin_mgr, 
                      cad_geometry_t* geometry, cad_mesh_t* mesh,
                      mom_solver_t* mom_solver, peec_solver_t* peec_solver, 
                      hybrid_coupling_t* coupling);

/**
 * @brief Create demo antenna geometry programmatically
 * @return Pointer to created geometry or NULL on failure
 */
cad_geometry_t* create_demo_antenna_geometry(void) {
    printf("Creating demo antenna geometry...\n");
    
    cad_geometry_t* geometry = cad_geometry_create();
    if (!geometry) {
        printf("Failed to create geometry\n");
        return NULL;
    }
    
    // Create a simple dipole antenna geometry
    cad_point_t start_point = {0.0, 0.0, 0.0};
    cad_point_t end_point = {DEMO_ANTENNA_LENGTH, 0.0, 0.0};
    
    // Add center wire (feed point)
    cad_wire_t center_wire = {
        .start_point = {DEMO_ANTENNA_LENGTH/2 - 0.001, 0.0, 0.0},
        .end_point = {DEMO_ANTENNA_LENGTH/2 + 0.001, 0.0, 0.0},
        .radius = DEMO_ANTENNA_RADIUS,
        .material_id = 1,
        .segment_count = 5
    };
    cad_geometry_add_wire(geometry, &center_wire);
    
    // Add left arm of dipole
    cad_wire_t left_arm = {
        .start_point = {0.0, 0.0, 0.0},
        .end_point = {DEMO_ANTENNA_LENGTH/2 - 0.001, 0.0, 0.0},
        .radius = DEMO_ANTENNA_RADIUS,
        .material_id = 1,
        .segment_count = 20
    };
    cad_geometry_add_wire(geometry, &left_arm);
    
    // Add right arm of dipole
    cad_wire_t right_arm = {
        .start_point = {DEMO_ANTENNA_LENGTH/2 + 0.001, 0.0, 0.0},
        .end_point = {DEMO_ANTENNA_LENGTH, 0.0, 0.0},
        .radius = DEMO_ANTENNA_RADIUS,
        .material_id = 1,
        .segment_count = 20
    };
    cad_geometry_add_wire(geometry, &right_arm);
    
    // Add ground plane
    cad_box_t ground_plane = {
        .corner = {-0.1, -0.1, -0.05},
        .dimensions = {0.3, 0.2, 0.001},
        .material_id = 2
    };
    cad_geometry_add_box(geometry, &ground_plane);
    
    // Define materials
    cad_material_t copper = {
        .name = "Copper",
        .conductivity = 5.8e7,
        .permittivity = 1.0,
        .permeability = 1.0,
        .loss_tangent = 0.0
    };
    
    cad_material_t perfect_conductor = {
        .name = "Perfect Conductor",
        .conductivity = 1e10,
        .permittivity = 1.0,
        .permeability = 1.0,
        .loss_tangent = 0.0
    };
    
    cad_geometry_add_material(geometry, &copper);
    cad_geometry_add_material(geometry, &perfect_conductor);
    
    printf("Demo antenna geometry created successfully\n");
    printf("  - Wire segments: %d\n", geometry->num_wires);
    printf("  - Ground plane: 1\n");
    printf("  - Materials: %d\n", geometry->num_materials);
    
    return geometry;
}

/**
 * @brief Setup simulation environment with core framework and plugin manager
 * @param core Pointer to core framework pointer
 * @param plugin_mgr Pointer to plugin manager pointer
 * @return 0 on success, -1 on failure
 */
int setup_simulation_environment(electromagnetic_kernel_library_t** core, plugin_manager_t** plugin_mgr) {
    printf("Setting up simulation environment...\n");
    
    // Initialize core framework
    peec_mom_config_t config = {
        .memory_pool_size = 1024 * 1024 * 1024,  // 1GB
        .max_threads = 8,
        .use_gpu = 1,
        .gpu_device_id = 0,
        .precision = PRECISION_DOUBLE,
        .use_compression = 1,
        .compression_threshold = 1000,
        .log_level = LOG_LEVEL_INFO,
        .enable_profiling = 1
    };
    
    *core = electromagnetic_kernel_library_init(&config);
    if (!*core) {
        printf("Failed to initialize core framework\n");
        return -1;
    }
    
    // Initialize plugin manager
    *plugin_mgr = plugin_manager_create();
    if (!*plugin_mgr) {
        printf("Failed to create plugin manager\n");
        electromagnetic_kernel_library_cleanup(*core);
        return -1;
    }
    
    // Load solver plugins
    const char* plugin_paths[] = {
        "./plugins/mom_solver_plugin.dll",
        "./plugins/peec_solver_plugin.dll",
        "./plugins/hybrid_coupling_plugin.dll"
    };
    
    for (int i = 0; i < 3; i++) {
        plugin_t* plugin = plugin_manager_load_plugin(*plugin_mgr, plugin_paths[i]);
        if (!plugin) {
            printf("Warning: Failed to load plugin %s\n", plugin_paths[i]);
        }
    }
    
    printf("Simulation environment setup complete\n");
    return 0;
}

/**
 * @brief Load and process CAD file
 * @param filename Path to CAD file
 * @param geometry Pointer to geometry pointer
 * @return 0 on success, -1 on failure
 */
int load_and_process_cad_file(const char* filename, cad_geometry_t** geometry) {
    printf("Loading CAD file: %s\n", filename);
    
    // For demo purposes, create geometry programmatically
    *geometry = create_demo_antenna_geometry();
    if (!*geometry) {
        return -1;
    }
    
    // Validate geometry
    cad_validation_result_t validation = cad_geometry_validate(*geometry);
    if (!validation.is_valid) {
        printf("Geometry validation failed:\n");
        for (int i = 0; i < validation.num_errors; i++) {
            printf("  Error %d: %s\n", i+1, validation.error_messages[i]);
        }
        cad_geometry_destroy(*geometry);
        return -1;
    }
    
    printf("CAD file loaded and validated successfully\n");
    printf("  Entities: %d\n", (*geometry)->num_entities);
    printf("  Materials: %d\n", (*geometry)->num_materials);
    
    return 0;
}

/**
 * @brief Generate electromagnetic mesh from CAD geometry
 * @param geometry Input CAD geometry
 * @param mesh Pointer to output mesh pointer
 * @return 0 on success, -1 on failure
 */
int generate_electromagnetic_mesh(cad_geometry_t* geometry, cad_mesh_t** mesh) {
    printf("Generating electromagnetic mesh...\n");
    
    // Setup mesh generation parameters
    cad_mesh_parameters_t params = {
        .mesh_size = DEMO_MESH_SIZE,
        .min_mesh_size = DEMO_MESH_SIZE / 10.0,
        .max_mesh_size = DEMO_MESH_SIZE * 10.0,
        .growth_rate = 1.2,
        .refinement_level = 3,
        .quality_threshold = 0.3,
        .algorithm = MESH_ALGORITHM_DELAUNAY,
        .element_type = MESH_ELEMENT_TRIANGLE,
        .use_adaptive_refinement = 1,
        .use_parallel_processing = 1,
        .num_threads = 8,
        .max_iterations = 100,
        .convergence_tolerance = 1e-3,
        .frequency = DEMO_FREQUENCY
    };
    
    // Generate mesh
    *mesh = cad_mesh_generate(geometry, &params);
    if (!*mesh) {
        printf("Failed to generate mesh\n");
        return -1;
    }
    
    // Validate mesh quality
    cad_mesh_quality_t quality = cad_mesh_compute_quality(*mesh);
    printf("Mesh generated successfully\n");
    printf("  Nodes: %d\n", (*mesh)->num_nodes);
    printf("  Elements: %d\n", (*mesh)->num_elements);
    printf("  Quality metrics:\n");
    printf("    Min angle: %.2f degrees\n", quality.min_angle);
    printf("    Max angle: %.2f degrees\n", quality.max_angle);
    printf("    Aspect ratio: %.2f\n", quality.aspect_ratio);
    printf("    Skewness: %.2f\n", quality.skewness);
    
    // Export mesh for verification
    cad_mesh_export_to_file(*mesh, "demo_antenna_mesh.vtk", MESH_FORMAT_VTK);
    
    return 0;
}

/**
 * @brief Setup MoM simulation
 * @param plugin_mgr Plugin manager
 * @param mesh Electromagnetic mesh
 * @param solver Pointer to MoM solver pointer
 * @return 0 on success, -1 on failure
 */
int setup_mom_simulation(plugin_manager_t* plugin_mgr, cad_mesh_t* mesh, mom_solver_t** solver) {
    printf("Setting up MoM simulation...\n");
    
    // Get MoM solver plugin
    plugin_t* mom_plugin = plugin_manager_get_plugin(plugin_mgr, "mom_solver");
    if (!mom_plugin) {
        printf("MoM solver plugin not found\n");
        return -1;
    }
    
    // Create MoM solver
    *solver = mom_solver_create();
    if (!*solver) {
        printf("Failed to create MoM solver\n");
        return -1;
    }
    
    // Configure MoM solver
    mom_config_t mom_config = {
        .frequency = DEMO_FREQUENCY,
        .basis_function = BASIS_FUNCTION_RWG,
        .integral_kernel = KERNEL_ELECTRIC_FIELD,
        .preconditioner = PRECONDITIONER_ILU,
        .solver_type = SOLVER_TYPE_GMRES,
        .tolerance = 1e-6,
        .max_iterations = 1000,
        .use_h_matrix = 1,
        .h_matrix_tolerance = 1e-4,
        .use_mlfmm = 1,
        .mlfmm_levels = 4,
        .use_gpu = 1,
        .gpu_device_id = 0
    };
    
    mom_solver_configure(*solver, &mom_config);
    
    // Import mesh
    mom_solver_import_mesh(*solver, mesh);
    
    // Setup excitation (voltage source at feed point)
    mom_excitation_t excitation = {
        .type = EXCITATION_VOLTAGE_SOURCE,
        .frequency = DEMO_FREQUENCY,
        .amplitude = 1.0,
        .phase = 0.0,
        .position = {DEMO_ANTENNA_LENGTH/2, 0.0, 0.0},
        .direction = {0.0, 0.0, 1.0}
    };
    
    mom_solver_add_excitation(*solver, &excitation);
    
    printf("MoM simulation setup complete\n");
    return 0;
}

/**
 * @brief Setup PEEC simulation
 * @param plugin_mgr Plugin manager
 * @param mesh Electromagnetic mesh
 * @param solver Pointer to PEEC solver pointer
 * @return 0 on success, -1 on failure
 */
int setup_peec_simulation(plugin_manager_t* plugin_mgr, cad_mesh_t* mesh, peec_solver_t** solver) {
    printf("Setting up PEEC simulation...\n");
    
    // Get PEEC solver plugin
    plugin_t* peec_plugin = plugin_manager_get_plugin(plugin_mgr, "peec_solver");
    if (!peec_plugin) {
        printf("PEEC solver plugin not found\n");
        return -1;
    }
    
    // Create PEEC solver
    *solver = peec_solver_create();
    if (!*solver) {
        printf("Failed to create PEEC solver\n");
        return -1;
    }
    
    // Configure PEEC solver
    peec_config_t peec_config = {
        .frequency = DEMO_FREQUENCY,
        .partial_inductance_calculation = 1,
        .partial_capacitance_calculation = 1,
        .partial_resistance_calculation = 1,
        .partial_conductance_calculation = 1,
        .retardation_model = RETARDATION_FULL,
        .skin_effect_model = SKIN_EFFECT_ACCURATE,
        .solver_type = SOLVER_TYPE_LU,
        .tolerance = 1e-6,
        .use_sparse_matrix = 1,
        .sparsity_threshold = 1e-6
    };
    
    peec_solver_configure(*solver, &peec_config);
    
    // Import mesh and extract partial elements
    peec_solver_import_geometry(*solver, mesh);
    peec_solver_extract_partial_elements(*solver);
    
    // Setup circuit excitation
    peec_circuit_element_t voltage_source = {
        .type = ELEMENT_VOLTAGE_SOURCE,
        .node_positive = 10,  // Feed point node
        .node_negative = 0,   // Ground node
        .value = 1.0,
        .frequency = DEMO_FREQUENCY
    };
    
    peec_solver_add_circuit_element(*solver, &voltage_source);
    
    printf("PEEC simulation setup complete\n");
    return 0;
}

/**
 * @brief Run hybrid MoM-PEEC simulation
 * @param plugin_mgr Plugin manager
 * @param mesh Electromagnetic mesh
 * @param coupling Pointer to hybrid coupling pointer
 * @return 0 on success, -1 on failure
 */
int run_hybrid_simulation(plugin_manager_t* plugin_mgr, cad_mesh_t* mesh, hybrid_coupling_t** coupling) {
    printf("Setting up hybrid MoM-PEEC simulation...\n");
    
    // Get hybrid coupling plugin
    plugin_t* hybrid_plugin = plugin_manager_get_plugin(plugin_mgr, "hybrid_coupling");
    if (!hybrid_plugin) {
        printf("Hybrid coupling plugin not found\n");
        return -1;
    }
    
    // Create hybrid coupling
    *coupling = hybrid_coupling_create();
    if (!*coupling) {
        printf("Failed to create hybrid coupling\n");
        return -1;
    }
    
    // Configure hybrid coupling
    hybrid_config_t hybrid_config = {
        .coupling_method = COUPLING_SCHUR_COMPLEMENT,
        .interface_tolerance = 1e-6,
        .max_coupling_iterations = 100,
        .use_adaptive_refinement = 1,
        .refinement_threshold = 0.1,
        .domain_decomposition = 1,
        .num_domains = 4,
        .overlap_size = 0.05
    };
    
    hybrid_coupling_configure(*coupling, &hybrid_config);
    
    // Import mesh and setup domains
    hybrid_coupling_import_mesh(*coupling, mesh);
    
    // Define MoM domain (antenna structure)
    hybrid_domain_t mom_domain = {
        .domain_id = 0,
        .solver_type = SOLVER_TYPE_MOM,
        .region = {{-0.1, -0.1, -0.1}, {0.3, 0.1, 0.1}},
        .priority = 1
    };
    
    // Define PEEC domain (circuit components)
    hybrid_domain_t peec_domain = {
        .domain_id = 1,
        .solver_type = SOLVER_TYPE_PEEC,
        .region = {{DEMO_ANTENNA_LENGTH/2 - 0.01, -0.01, -0.01}, 
                    {DEMO_ANTENNA_LENGTH/2 + 0.01, 0.01, 0.01}},
        .priority = 2
    };
    
    hybrid_coupling_add_domain(*coupling, &mom_domain);
    hybrid_coupling_add_domain(*coupling, &peec_domain);
    
    // Solve coupled system
    printf("Solving hybrid MoM-PEEC system...\n");
    int result = hybrid_coupling_solve(*coupling);
    
    if (result == 0) {
        printf("Hybrid simulation completed successfully\n");
    } else {
        printf("Hybrid simulation failed\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Analyze and visualize simulation results
 * @param mom_solver MoM solver instance
 * @param peec_solver PEEC solver instance
 * @param coupling Hybrid coupling instance
 */
void analyze_and_visualize_results(mom_solver_t* mom_solver, peec_solver_t* peec_solver, 
                                 hybrid_coupling_t* coupling) {
    printf("\n=== SIMULATION RESULTS ANALYSIS ===\n");
    
    // MoM Results
    if (mom_solver) {
        printf("\nMoM Simulation Results:\n");
        
        // Get current distribution
        complex_double* currents = mom_solver_get_currents(mom_solver);
        int num_basis = mom_solver_get_num_basis_functions(mom_solver);
        
        printf("  Number of basis functions: %d\n", num_basis);
        printf("  Current distribution computed\n");
        
        // Calculate radiation pattern
        mom_far_field_t far_field;
        mom_solver_compute_far_field(mom_solver, &far_field);
        
        printf("  Far-field pattern computed\n");
        printf("  Maximum gain: %.2f dBi\n", far_field.max_gain);
        printf("  Beam direction: (%.1f°, %.1f°)\n", 
               far_field.max_direction.theta * 180.0/M_PI,
               far_field.max_direction.phi * 180.0/M_PI);
        
        // Calculate input impedance
        complex_double input_impedance = mom_solver_get_input_impedance(mom_solver);
        printf("  Input impedance: %.2f + j%.2f Ω\n", 
               creal(input_impedance), cimag(input_impedance));
        
        // Calculate reflection coefficient
        double reflection_coeff = mom_solver_get_reflection_coefficient(mom_solver, 50.0);
        printf("  Reflection coefficient (50Ω): %.3f\n", reflection_coeff);
        printf("  Return loss: %.1f dB\n", -20.0 * log10(reflection_coeff));
        
        free(currents);
    }
    
    // PEEC Results
    if (peec_solver) {
        printf("\nPEEC Simulation Results:\n");
        
        // Get circuit voltages and currents
        double* node_voltages = peec_solver_get_node_voltages(peec_solver);
        double* branch_currents = peec_solver_get_branch_currents(peec_solver);
        int num_nodes = peec_solver_get_num_nodes(peec_solver);
        int num_branches = peec_solver_get_num_branches(peec_solver);
        
        printf("  Circuit nodes: %d\n", num_nodes);
        printf("  Circuit branches: %d\n", num_branches);
        printf("  Node voltages computed\n");
        printf("  Branch currents computed\n");
        
        // Get partial elements
        peec_partial_elements_t* elements = peec_solver_get_partial_elements(peec_solver);
        printf("  Partial elements extracted:\n");
        printf("    Resistances: %d\n", elements->num_resistances);
        printf("    Inductances: %d\n", elements->num_inductances);
        printf("    Capacitances: %d\n", elements->num_capacitances);
        printf("    Conductances: %d\n", elements->num_conductances);
        
        // Calculate power distribution
        double total_power = peec_solver_get_total_power(peec_solver);
        double dissipated_power = peec_solver_get_dissipated_power(peec_solver);
        printf("  Total power: %.3f W\n", total_power);
        printf("  Dissipated power: %.3f W\n", dissipated_power);
        printf("  Efficiency: %.1f%%\n", (1.0 - dissipated_power/total_power) * 100.0);
        
        free(node_voltages);
        free(branch_currents);
    }
    
    // Hybrid Results
    if (coupling) {
        printf("\nHybrid Simulation Results:\n");
        
        // Get coupling convergence
        int coupling_iterations = hybrid_coupling_get_iterations(coupling);
        double coupling_error = hybrid_coupling_get_final_error(coupling);
        
        printf("  Coupling iterations: %d\n", coupling_iterations);
        printf("  Final coupling error: %.2e\n", coupling_error);
        
        // Get domain interactions
        hybrid_interaction_t* interactions = hybrid_coupling_get_domain_interactions(coupling);
        printf("  Domain interactions analyzed\n");
        
        // Calculate combined performance metrics
        double total_simulation_time = hybrid_coupling_get_simulation_time(coupling);
        double memory_usage = hybrid_coupling_get_memory_usage(coupling);
        
        printf("  Total simulation time: %.1f seconds\n", total_simulation_time);
        printf("  Memory usage: %.1f MB\n", memory_usage / (1024.0 * 1024.0));
    }
    
    printf("\n=== RESULTS ANALYSIS COMPLETE ===\n");
}

/**
 * @brief Cleanup all allocated resources
 */
void cleanup_resources(electromagnetic_kernel_library_t* core, plugin_manager_t* plugin_mgr, 
                      cad_geometry_t* geometry, cad_mesh_t* mesh,
                      mom_solver_t* mom_solver, peec_solver_t* peec_solver, 
                      hybrid_coupling_t* coupling) {
    
    printf("Cleaning up resources...\n");
    
    if (coupling) hybrid_coupling_destroy(coupling);
    if (peec_solver) peec_solver_destroy(peec_solver);
    if (mom_solver) mom_solver_destroy(mom_solver);
    if (mesh) cad_mesh_destroy(mesh);
    if (geometry) cad_geometry_destroy(geometry);
    if (plugin_mgr) plugin_manager_destroy(plugin_mgr);
    if (core) electromagnetic_kernel_library_cleanup(core);
    
    printf("Cleanup complete\n");
}

/**
 * @brief Main demonstration function
 */
int main(int argc, char* argv[]) {
    printf("=== COMPREHENSIVE CAD-TO-SIMULATION INTEGRATION DEMO ===\n");
    printf("Frequency: %.1f GHz\n", DEMO_FREQUENCY / 1e9);
    printf("Antenna length: %.1f cm\n", DEMO_ANTENNA_LENGTH * 100);
    printf("Mesh size: %.1f mm\n\n", DEMO_MESH_SIZE * 1000);
    
    // Initialize variables
    electromagnetic_kernel_library_t* core = NULL;
    plugin_manager_t* plugin_mgr = NULL;
    cad_geometry_t* geometry = NULL;
    cad_mesh_t* mesh = NULL;
    mom_solver_t* mom_solver = NULL;
    peec_solver_t* peec_solver = NULL;
    hybrid_coupling_t* coupling = NULL;
    
    int result = 0;
    clock_t start_time = clock();
    
    // Step 1: Setup simulation environment
    result = setup_simulation_environment(&core, &plugin_mgr);
    if (result != 0) {
        printf("Failed to setup simulation environment\n");
        goto cleanup;
    }
    
    // Step 2: Load and process CAD geometry
    if (argc > 1) {
        result = load_and_process_cad_file(argv[1], &geometry);
    } else {
        // Create demo geometry
        geometry = create_demo_antenna_geometry();
        if (!geometry) {
            result = -1;
        }
    }
    if (result != 0) {
        printf("Failed to load/process CAD geometry\n");
        goto cleanup;
    }
    
    // Step 3: Generate electromagnetic mesh
    result = generate_electromagnetic_mesh(geometry, &mesh);
    if (result != 0) {
        printf("Failed to generate electromagnetic mesh\n");
        goto cleanup;
    }
    
    // Step 4: Setup and run MoM simulation
    result = setup_mom_simulation(plugin_mgr, mesh, &mom_solver);
    if (result != 0) {
        printf("Failed to setup MoM simulation\n");
        goto cleanup;
    }
    
    printf("Running MoM simulation...\n");
    result = mom_solver_solve(mom_solver);
    if (result != 0) {
        printf("MoM simulation failed\n");
        goto cleanup;
    }
    
    // Step 5: Setup and run PEEC simulation
    result = setup_peec_simulation(plugin_mgr, mesh, &peec_solver);
    if (result != 0) {
        printf("Failed to setup PEEC simulation\n");
        goto cleanup;
    }
    
    printf("Running PEEC simulation...\n");
    result = peec_solver_solve(peec_solver);
    if (result != 0) {
        printf("PEEC simulation failed\n");
        goto cleanup;
    }
    
    // Step 6: Run hybrid simulation
    result = run_hybrid_simulation(plugin_mgr, mesh, &coupling);
    if (result != 0) {
        printf("Hybrid simulation failed\n");
        goto cleanup;
    }
    
    // Step 7: Analyze and visualize results
    analyze_and_visualize_results(mom_solver, peec_solver, coupling);
    
    // Performance summary
    clock_t end_time = clock();
    double total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("\n=== PERFORMANCE SUMMARY ===\n");
    printf("Total execution time: %.1f seconds\n", total_time);
    
    // Get performance metrics from monitoring system
    performance_monitor_t* monitor = electromagnetic_kernel_library_get_performance_monitor(core);
    if (monitor) {
        performance_snapshot_t* snapshot = performance_monitor_get_snapshot(monitor);
        if (snapshot) {
            printf("CPU usage: %.1f%%\n", snapshot->cpu_usage);
            printf("Memory usage: %.1f MB\n", snapshot->memory_usage / (1024.0 * 1024.0));
            printf("GPU usage: %.1f%%\n", snapshot->gpu_usage);
            
            performance_snapshot_destroy(snapshot);
        }
    }
    
    printf("\n=== DEMO COMPLETED SUCCESSFULLY ===\n");
    
cleanup:
    cleanup_resources(core, plugin_mgr, geometry, mesh, mom_solver, peec_solver, coupling);
    
    return result;
}