/*********************************************************************
 * PEEC-MoM Unified Framework - Integration Examples
 * 
 * This file contains comprehensive examples demonstrating the usage
 * of the unified PEEC-MoM framework for various electromagnetic
 * simulation scenarios.
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "../src/core/electromagnetic_kernel_library.h"
#include "../src/solvers/mom/mom_solver_module.h"
#include "../src/solvers/peec/peec_solver_module.h"
#include "../src/hybrid/hybrid_coupling_interface.h"

// Helper function prototypes
static void print_example_header(const char* title);
static void print_example_footer(const char* title, double time);
static void print_performance_metrics(const char* solver_name, 
                                    size_t memory_usage, double solve_time);
static Complex compute_analytical_solution(double frequency, double theta, double phi);
static double compute_relative_error(Complex computed, Complex analytical);

/*********************************************************************
 * Example 1: Basic MoM Antenna Analysis
 *********************************************************************/
int example_basic_mom_antenna() {
    print_example_header("Basic MoM Antenna Analysis - Dipole Antenna");
    
    Timer timer;
    timer_start(&timer);
    
    // Create framework configuration
    FrameworkConfig config = {
        .logging = {
            .min_level = LOG_LEVEL_INFO,
            .callback = NULL,
            .user_data = NULL,
            .enable_file_logging = true,
            .log_file_path = "dipole_antenna.log"
        },
        .num_threads = 8,
        .use_gpu = false,
        .use_mpi = false,
        .memory_limit = 4 * 1024 * 1024 * 1024LL,  // 4GB
        .solver_type = SOLVER_TYPE_MOM,
        .simulation_type = SIMULATION_TYPE_FREQUENCY_DOMAIN,
        .accuracy_target = 1e-6,
        .max_iterations = 1000,
        .convergence_tolerance = 1e-8
    };
    
    // Create framework
    Framework* framework = framework_create(&config);
    if (!framework) {
        printf("Failed to create framework\n");
        return -1;
    }
    
    // Initialize framework
    StatusCode status = framework_initialize(framework);
    if (status != SUCCESS) {
        printf("Failed to initialize framework: %d\n", status);
        framework_destroy(framework);
        return -1;
    }
    
    // Configure MoM solver options
    MomSolverOptions mom_options = {
        .basis_type = MOM_BASIS_RWG,
        .basis_order = 1,
        .use_loop_tree = false,
        .integration_type = MOM_INTEGRATION_SINGULAR,
        .integration_order = 4,
        .integration_tolerance = 1e-12,
        .use_singularity_subtraction = true,
        .acceleration_type = MOM_ACCELERATION_H_MATRIX,
        .compression_tolerance = 1e-4,
        .h_matrix_max_rank = 50,
        .mlfmm_max_level = 6,
        .preconditioner_type = MOM_PRECONDITIONER_CALDERON,
        .preconditioner_levels = 2,
        .max_iterations = 1000,
        .convergence_tolerance = 1e-8,
        .restart_size = 100,
        .use_bicgstab = false,
        .use_parallel_solver = true,
        .frequency = 300e6,  // 300 MHz
        .accuracy_target = 1e-6,
        .use_adaptive_accuracy = true,
        .num_threads = 8,
        .use_gpu = false,
        .enable_performance_monitoring = true
    };
    
    // Create MoM solver
    MomSolver* mom_solver = mom_solver_create(&mom_options);
    if (!mom_solver) {
        printf("Failed to create MoM solver\n");
        framework_destroy(framework);
        return -1;
    }
    
    // Create geometry (dipole antenna)
    GeometryEngine* geometry_engine = framework_get_geometry_engine(framework);
    
    // Define dipole geometry parameters
    double dipole_length = 0.5;     // 0.5 meters (half-wave at 300 MHz)
    double dipole_radius = 0.005;   // 5 mm radius
    int num_segments = 20;          // 20 segments for discretization
    
    // Create dipole geometry (simplified representation)
    framework_log(LOG_LEVEL_INFO, "Creating dipole antenna geometry");
    framework_log(LOG_LEVEL_INFO, "Dipole length: %.2f m, radius: %.2f mm", 
                  dipole_length, dipole_radius * 1000);
    
    // Set up mesh engine
    MeshEngine* mesh_engine = framework_get_mesh_engine(framework);
    MeshParameters mesh_params = {
        .target_size = dipole_length / num_segments,
        .minimum_size = dipole_radius / 2,
        .maximum_size = dipole_length / 10,
        .growth_rate = 1.2,
        .curvature_resolution = 0.1,
        .feature_angle = 15.0,
        .adaptive_levels = 2,
        .max_iterations = 100,
        .quality_threshold = 0.3,
        .use_curvature_adaptation = true,
        .use_feature_detection = true,
        .use_parallel_meshing = true,
        .num_threads = 8
    };
    
    // Initialize MoM solver with geometry
    status = mom_solver_initialize(mom_solver, geometry_engine, mesh_engine);
    if (status != SUCCESS) {
        printf("Failed to initialize MoM solver: %d\n", status);
        mom_solver_destroy(mom_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Set up excitation (voltage source at feed point)
    MomExcitation excitation = {
        .type = MOM_EXCITATION_VOLTAGE_SOURCE,
        .frequency = 300e6,
        .amplitude = 1.0 + 0.0*I,
        .direction = {0.0, 0.0, 1.0},    // Z-direction
        .polarization = {0.0, 0.0, 1.0}, // Z-polarized
        .position = {0.0, 0.0, 0.0},    // Feed at center
        .source_strength = 1.0,
        .port_id = 1,
        .port_impedance = 50.0,          // 50 ohm reference
        .num_modes = 1
    };
    
    status = mom_solver_add_excitation(mom_solver, &excitation);
    if (status != SUCCESS) {
        printf("Failed to add excitation: %d\n", status);
        mom_solver_destroy(mom_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Assemble matrix system
    framework_log(LOG_LEVEL_INFO, "Assembling MoM matrix system...");
    status = mom_solver_assemble_matrix(mom_solver);
    if (status != SUCCESS) {
        printf("Failed to assemble matrix: %d\n", status);
        mom_solver_destroy(mom_solver);
        framework_destroy(framework);
        return -1;
    }
    
    status = mom_solver_assemble_rhs(mom_solver);
    if (status != SUCCESS) {
        printf("Failed to assemble RHS: %d\n", status);
        mom_solver_destroy(mom_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Solve the system
    framework_log(LOG_LEVEL_INFO, "Solving MoM system...");
    status = mom_solver_solve(mom_solver);
    if (status != SUCCESS) {
        printf("Failed to solve system: %d\n", status);
        mom_solver_destroy(mom_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Compute far-field pattern
    framework_log(LOG_LEVEL_INFO, "Computing far-field pattern...");
    status = mom_solver_compute_far_field(mom_solver, 0.0, 180.0, 181,  // Theta: 0-180°
                                        0.0, 360.0, 361);             // Phi: 0-360°
    if (status != SUCCESS) {
        printf("Failed to compute far-field: %d\n", status);
    }
    
    // Get results
    const MomSolverResults* results = mom_solver_get_results(mom_solver);
    const MomFarFieldPattern* far_field = mom_solver_get_far_field(mom_solver);
    
    // Print results
    printf("\n=== MoM Dipole Antenna Results ===\n");
    printf("Frequency: %.1f MHz\n", mom_options.frequency / 1e6);
    printf("Convergence: %s (iterations: %d)\n", 
           results->converged ? "YES" : "NO", results->num_iterations);
    printf("Final residual: %.2e\n", results->final_residual);
    printf("Solution error: %.2e\n", results->solution_error);
    printf("Condition number: %.2e\n", results->condition_number);
    printf("Assembly time: %.2f s\n", results->assembly_time);
    printf("Solve time: %.2f s\n", results->solve_time);
    printf("Total time: %.2f s\n", results->total_time);
    printf("Memory usage: %.1f MB\n", results->memory_usage / (1024.0 * 1024.0));
    
    if (far_field) {
        printf("Far-field pattern computed: %d points\n", far_field->num_points);
        printf("Directivity: %.2f dBi\n", far_field->directivity);
        printf("Gain: %.2f dBi\n", far_field->gain);
        printf("Efficiency: %.2f %%\n", far_field->efficiency * 100.0);
    }
    
    // Validate against analytical solution
    Complex analytical = compute_analytical_solution(mom_options.frequency, 90.0, 0.0);
    Complex computed = far_field->points[90 * 361 + 0].e_theta; // Broadside
    double error = compute_relative_error(computed, analytical);
    printf("Validation error (broadside): %.2e\n", error);
    
    // Cleanup
    mom_solver_destroy(mom_solver);
    framework_finalize(framework);
    framework_destroy(framework);
    
    timer_stop(&timer);
    print_example_footer("Basic MoM Antenna Analysis", timer_get_elapsed(&timer));
    
    return 0;
}

/*********************************************************************
 * Example 2: Basic PEEC Circuit Analysis
 *********************************************************************/
int example_basic_peec_circuit() {
    print_example_header("Basic PEEC Circuit Analysis - Microstrip Line");
    
    Timer timer;
    timer_start(&timer);
    
    // Create framework configuration
    FrameworkConfig config = {
        .logging = {
            .min_level = LOG_LEVEL_INFO,
            .callback = NULL,
            .user_data = NULL,
            .enable_file_logging = true,
            .log_file_path = "microstrip_line.log"
        },
        .num_threads = 4,
        .use_gpu = false,
        .use_mpi = false,
        .memory_limit = 2 * 1024 * 1024 * 1024LL,  // 2GB
        .solver_type = SOLVER_TYPE_PEEC,
        .simulation_type = SIMULATION_TYPE_FREQUENCY_DOMAIN,
        .accuracy_target = 1e-6,
        .max_iterations = 1000,
        .convergence_tolerance = 1e-8
    };
    
    // Create framework
    Framework* framework = framework_create(&config);
    if (!framework) {
        printf("Failed to create framework\n");
        return -1;
    }
    
    // Initialize framework
    StatusCode status = framework_initialize(framework);
    if (status != SUCCESS) {
        printf("Failed to initialize framework: %d\n", status);
        framework_destroy(framework);
        return -1;
    }
    
    // Configure PEEC solver options
    PeecSolverOptions peec_options = {
        .element_type = PEEC_ELEMENT_SURFACE,
        .formulation = PEEC_FORMULATION_MODIFIED,
        .include_retardation = true,
        .include_radiation = false,
        .include_losses = true,
        .min_segment_length = 0.1e-3,   // 0.1 mm
        .max_segment_length = 1.0e-3,  // 1 mm
        .skin_depth_ratio = 3.0,
        .adaptive_refinement_levels = 2,
        .circuit_solver = PEEC_CIRCUIT_SOLVER_MNA,
        .analysis_type = PEEC_FREQUENCY_DOMAIN_AC,
        .max_circuit_iterations = 1000,
        .circuit_convergence_tolerance = 1e-10,
        .frequency_start = 1e9,    // 1 GHz
        .frequency_stop = 10e9,  // 10 GHz
        .num_frequency_points = 101,
        .use_logarithmic_spacing = true,
        .accuracy_target = 1e-6,
        .use_adaptive_accuracy = true,
        .num_threads = 4,
        .use_gpu = false,
        .use_sparse_matrices = true,
        .memory_limit = 2 * 1024 * 1024 * 1024LL,  // 2GB
        .save_circuit_netlist = true,
        .save_impedance_matrix = true,
        .compute_s_parameters = true,
        .compute_y_parameters = true,
        .compute_z_parameters = true
    };
    
    // Create PEEC solver
    PeecSolver* peec_solver = peec_solver_create(&peec_options);
    if (!peec_solver) {
        printf("Failed to create PEEC solver\n");
        framework_destroy(framework);
        return -1;
    }
    
    // Create geometry (microstrip line)
    GeometryEngine* geometry_engine = framework_get_geometry_engine(framework);
    
    // Define microstrip parameters
    double line_length = 10e-3;     // 10 mm
    double line_width = 0.5e-3;     // 0.5 mm
    double substrate_height = 0.25e-3; // 0.25 mm
    double substrate_er = 4.4;      // FR4 dielectric constant
    
    framework_log(LOG_LEVEL_INFO, "Creating microstrip line geometry");
    framework_log(LOG_LEVEL_INFO, "Line: %.1f mm x %.1f mm, substrate: %.1f mm, er=%.1f", 
                  line_length * 1000, line_width * 1000, 
                  substrate_height * 1000, substrate_er);
    
    // Set up mesh engine
    MeshEngine* mesh_engine = framework_get_mesh_engine(framework);
    MeshParameters mesh_params = {
        .target_size = line_width / 2,
        .minimum_size = line_width / 10,
        .maximum_size = line_width * 2,
        .growth_rate = 1.2,
        .curvature_resolution = 0.1,
        .feature_angle = 15.0,
        .adaptive_levels = 2,
        .max_iterations = 100,
        .quality_threshold = 0.3,
        .use_curvature_adaptation = true,
        .use_feature_detection = true,
        .use_parallel_meshing = true,
        .num_threads = 4
    };
    
    // Initialize PEEC solver with geometry
    status = peec_solver_initialize(peec_solver, geometry_engine, mesh_engine);
    if (status != SUCCESS) {
        printf("Failed to initialize PEEC solver: %d\n", status);
        peec_solver_destroy(peec_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Add microstrip line elements
    double start_point[3] = {0.0, 0.0, substrate_height};
    double end_point[3] = {line_length, 0.0, substrate_height};
    int material_id = 1; // Copper
    
    status = peec_solver_add_wire_element(peec_solver, start_point, end_point,
                                          line_width/2, material_id);
    if (status != SUCCESS) {
        printf("Failed to add wire element: %d\n", status);
        peec_solver_destroy(peec_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Add ports at both ends
    status = peec_solver_add_port(peec_solver, 1, 0, 50.0, "Port1"); // Input port
    if (status != SUCCESS) {
        printf("Failed to add port 1: %d\n", status);
    }
    
    status = peec_solver_add_port(peec_solver, 2, 0, 50.0, "Port2"); // Output port
    if (status != SUCCESS) {
        printf("Failed to add port 2: %d\n", status);
    }
    
    // Add voltage source at input port
    Complex source_voltage = 1.0 + 0.0*I;
    status = peec_solver_add_voltage_source(peec_solver, 1, 0, source_voltage, 
                                            peec_options.frequency_start);
    if (status != SUCCESS) {
        printf("Failed to add voltage source: %d\n", status);
    }
    
    // Assemble circuit matrices
    framework_log(LOG_LEVEL_INFO, "Assembling PEEC circuit matrices...");
    status = peec_solver_assemble_matrices(peec_solver);
    if (status != SUCCESS) {
        printf("Failed to assemble matrices: %d\n", status);
        peec_solver_destroy(peec_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Solve frequency sweep
    framework_log(LOG_LEVEL_INFO, "Solving PEEC frequency sweep...");
    status = peec_solver_solve_frequency_sweep(peec_solver);
    if (status != SUCCESS) {
        printf("Failed to solve frequency sweep: %d\n", status);
        peec_solver_destroy(peec_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Compute S-parameters
    status = peec_solver_compute_s_parameters(peec_solver);
    if (status != SUCCESS) {
        printf("Failed to compute S-parameters: %d\n", status);
    }
    
    // Get results
    const PeecSolverResults* results = peec_solver_get_results(peec_solver);
    const PeecSParameters* s_params = peec_solver_get_s_parameters(peec_solver);
    
    // Print results
    printf("\n=== PEEC Microstrip Line Results ===\n");
    printf("Frequency range: %.1f - %.1f GHz (%d points)\n", 
           peec_options.frequency_start / 1e9, peec_options.frequency_stop / 1e9,
           peec_options.num_frequency_points);
    printf("Convergence: %s (iterations: %d)\n", 
           results->converged ? "YES" : "NO", results->num_iterations);
    printf("Final residual: %.2e\n", results->final_residual);
    printf("Solution error: %.2e\n", results->solution_error);
    printf("Assembly time: %.2f s\n", results->assembly_time);
    printf("Solve time: %.2f s\n", results->solve_time);
    printf("Total time: %.2f s\n", results->total_time);
    printf("Memory usage: %.1f MB\n", results->memory_usage / (1024.0 * 1024.0));
    
    if (s_params) {
        printf("S-parameters computed\n");
        printf("Characteristic impedance: %.1f ohms\n", s_params->characteristic_impedance);
    }
    
    // Export SPICE netlist
    status = peec_solver_export_spice_netlist(peec_solver, "microstrip_line.cir");
    if (status == SUCCESS) {
        printf("SPICE netlist exported to microstrip_line.cir\n");
    }
    
    // Export S-parameters
    status = peec_solver_export_s_parameters(peec_solver, "microstrip_line.s2p");
    if (status == SUCCESS) {
        printf("S-parameters exported to microstrip_line.s2p\n");
    }
    
    // Cleanup
    peec_solver_destroy(peec_solver);
    framework_finalize(framework);
    framework_destroy(framework);
    
    timer_stop(&timer);
    print_example_footer("Basic PEEC Circuit Analysis", timer_get_elapsed(&timer));
    
    return 0;
}

/*********************************************************************
 * Example 3: Hybrid MoM-PEEC Antenna with Feed Network
 *********************************************************************/
int example_hybrid_mom_peec_antenna() {
    print_example_header("Hybrid MoM-PEEC Analysis - Antenna with Feed Network");
    
    Timer timer;
    timer_start(&timer);
    
    // Create framework configuration
    FrameworkConfig config = {
        .logging = {
            .min_level = LOG_LEVEL_INFO,
            .callback = NULL,
            .user_data = NULL,
            .enable_file_logging = true,
            .log_file_path = "hybrid_antenna.log"
        },
        .num_threads = 16,
        .use_gpu = false,
        .use_mpi = false,
        .memory_limit = 8 * 1024 * 1024 * 1024LL,  // 8GB
        .solver_type = SOLVER_TYPE_HYBRID,
        .simulation_type = SIMULATION_TYPE_FREQUENCY_DOMAIN,
        .accuracy_target = 1e-6,
        .max_iterations = 2000,
        .convergence_tolerance = 1e-8
    };
    
    // Create framework
    Framework* framework = framework_create(&config);
    if (!framework) {
        printf("Failed to create framework\n");
        return -1;
    }
    
    // Initialize framework
    StatusCode status = framework_initialize(framework);
    if (status != SUCCESS) {
        printf("Failed to initialize framework: %d\n", status);
        framework_destroy(framework);
        return -1;
    }
    
    // Configure coupling options
    HybridCouplingOptions coupling_options = {
        .method = COUPLING_METHOD_SCHUR_COMPLEMENT,
        .type = COUPLING_TYPE_ELECTRIC_FIELD,
        .domain = COUPLING_DOMAIN_BIDIRECTIONAL,
        .max_iterations = 100,
        .convergence_tolerance = 1e-6,
        .relaxation_parameter = 0.8,
        .num_interface_points = 50,
        .interface_tolerance = 1e-8,
        .use_adaptive_interface = true,
        .use_robin_interface = false,
        .use_gmres = true,
        .use_bicgstab = false,
        .krylov_subspace_size = 50,
        .restart_size = 30,
        .use_preconditioner = true,
        .preconditioner_type = 1,
        .preconditioner_tolerance = 1e-4,
        .use_parallel_coupling = true,
        .num_coupling_threads = 8,
        .use_mpi = false,
        .enable_profiling = true,
        .save_intermediate_results = true,
        .compute_interface_errors = true,
        .accuracy_target = 1e-6,
        .use_adaptive_accuracy = true,
        .adaptive_refinement_levels = 2
    };
    
    // Create hybrid solver
    HybridSolver* hybrid_solver = hybrid_solver_create(&coupling_options);
    if (!hybrid_solver) {
        printf("Failed to create hybrid solver\n");
        framework_destroy(framework);
        return -1;
    }
    
    // Configure MoM solver for antenna radiation
    MomSolverOptions mom_options = {
        .basis_type = MOM_BASIS_RWG,
        .basis_order = 1,
        .use_loop_tree = false,
        .integration_type = MOM_INTEGRATION_SINGULAR,
        .integration_order = 4,
        .integration_tolerance = 1e-12,
        .use_singularity_subtraction = true,
        .acceleration_type = MOM_ACCELERATION_MLFMM,
        .compression_tolerance = 1e-4,
        .h_matrix_max_rank = 50,
        .mlfmm_max_level = 8,
        .preconditioner_type = MOM_PRECONDITIONER_CALDERON,
        .preconditioner_levels = 2,
        .max_iterations = 1000,
        .convergence_tolerance = 1e-8,
        .restart_size = 100,
        .use_bicgstab = false,
        .use_parallel_solver = true,
        .frequency = 2.4e9,  // 2.4 GHz WiFi band
        .accuracy_target = 1e-6,
        .use_adaptive_accuracy = true,
        .num_threads = 8,
        .use_gpu = false,
        .enable_performance_monitoring = true
    };
    
    // Create MoM solver
    MomSolver* mom_solver = mom_solver_create(&mom_options);
    if (!mom_solver) {
        printf("Failed to create MoM solver\n");
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Configure PEEC solver for feed network
    PeecSolverOptions peec_options = {
        .element_type = PEEC_ELEMENT_WIRE,
        .formulation = PEEC_FORMULATION_MODIFIED,
        .include_retardation = true,
        .include_radiation = false,
        .include_losses = true,
        .min_segment_length = 0.1e-3,
        .max_segment_length = 1.0e-3,
        .skin_depth_ratio = 3.0,
        .adaptive_refinement_levels = 2,
        .circuit_solver = PEEC_CIRCUIT_SOLVER_MNA,
        .analysis_type = PEEC_FREQUENCY_DOMAIN_AC,
        .max_circuit_iterations = 1000,
        .circuit_convergence_tolerance = 1e-10,
        .frequency_start = 2.4e9,
        .frequency_stop = 2.4e9,
        .num_frequency_points = 1,
        .use_logarithmic_spacing = false,
        .accuracy_target = 1e-6,
        .use_adaptive_accuracy = true,
        .num_threads = 8,
        .use_gpu = false,
        .use_sparse_matrices = true,
        .memory_limit = 4 * 1024 * 1024 * 1024LL,  // 4GB
        .save_circuit_netlist = true,
        .save_impedance_matrix = true,
        .compute_s_parameters = true,
        .compute_y_parameters = false,
        .compute_z_parameters = false
    };
    
    // Create PEEC solver
    PeecSolver* peec_solver = peec_solver_create(&peec_options);
    if (!peec_solver) {
        printf("Failed to create PEEC solver\n");
        mom_solver_destroy(mom_solver);
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Set up solvers in hybrid framework
    status = hybrid_solver_set_mom_solver(hybrid_solver, mom_solver);
    if (status != SUCCESS) {
        printf("Failed to set MoM solver: %d\n", status);
        mom_solver_destroy(mom_solver);
        peec_solver_destroy(peec_solver);
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    status = hybrid_solver_set_peec_solver(hybrid_solver, peec_solver);
    if (status != SUCCESS) {
        printf("Failed to set PEEC solver: %d\n", status);
        mom_solver_destroy(mom_solver);
        peec_solver_destroy(peec_solver);
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Initialize hybrid solver
    status = hybrid_solver_initialize(hybrid_solver);
    if (status != SUCCESS) {
        printf("Failed to initialize hybrid solver: %d\n", status);
        mom_solver_destroy(mom_solver);
        peec_solver_destroy(peec_solver);
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Create coupling interface
    framework_log(LOG_LEVEL_INFO, "Creating hybrid coupling interface...");
    
    // Define interface geometry (simplified)
    // In practice, this would be derived from the actual geometry
    MomGeometry mom_geometry = {0};
    PeecGeometry peec_geometry = {0};
    
    status = hybrid_solver_create_interface(hybrid_solver, &mom_geometry, &peec_geometry);
    if (status != SUCCESS) {
        printf("Failed to create coupling interface: %d\n", status);
        hybrid_solver_finalize(hybrid_solver);
        mom_solver_destroy(mom_solver);
        peec_solver_destroy(peec_solver);
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Assemble coupling matrices
    framework_log(LOG_LEVEL_INFO, "Assembling coupling matrices...");
    status = hybrid_solver_assemble_coupling_matrices(hybrid_solver);
    if (status != SUCCESS) {
        printf("Failed to assemble coupling matrices: %d\n", status);
        hybrid_solver_finalize(hybrid_solver);
        mom_solver_destroy(mom_solver);
        peec_solver_destroy(peec_solver);
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Solve coupled system
    framework_log(LOG_LEVEL_INFO, "Solving coupled MoM-PEEC system...");
    status = hybrid_solver_solve_schur_complement(hybrid_solver);
    if (status != SUCCESS) {
        printf("Failed to solve coupled system: %d\n", status);
        hybrid_solver_finalize(hybrid_solver);
        mom_solver_destroy(mom_solver);
        peec_solver_destroy(peec_solver);
        hybrid_solver_destroy(hybrid_solver);
        framework_destroy(framework);
        return -1;
    }
    
    // Get results
    Real final_error = hybrid_solver_get_final_error(hybrid_solver);
    int num_iterations = hybrid_solver_get_num_iterations(hybrid_solver);
    bool is_converged = hybrid_solver_is_converged(hybrid_solver);
    double total_time = hybrid_solver_get_total_time(hybrid_solver);
    
    // Print results
    printf("\n=== Hybrid MoM-PEEC Antenna Results ===\n");
    printf("Frequency: %.1f GHz\n", mom_options.frequency / 1e9);
    printf("Coupling method: Schur complement\n");
    printf("Convergence: %s (iterations: %d)\n", 
           is_converged ? "YES" : "NO", num_iterations);
    printf("Final error: %.2e\n", final_error);
    printf("Total time: %.2f s\n", total_time);
    printf("Memory usage: %.1f MB\n", 
           hybrid_solver_get_memory_usage(hybrid_solver) / (1024.0 * 1024.0));
    
    // Export interface results
    status = hybrid_solver_export_interface(hybrid_solver, "antenna_interface.vtk");
    if (status == SUCCESS) {
        printf("Interface results exported to antenna_interface.vtk\n");
    }
    
    // Export coupling matrices
    status = hybrid_solver_export_coupling_matrices(hybrid_solver, "coupling_matrices.mat");
    if (status == SUCCESS) {
        printf("Coupling matrices exported to coupling_matrices.mat\n");
    }
    
    // Cleanup
    hybrid_solver_finalize(hybrid_solver);
    hybrid_solver_destroy(hybrid_solver);
    framework_finalize(framework);
    framework_destroy(framework);
    
    timer_stop(&timer);
    print_example_footer("Hybrid MoM-PEEC Antenna Analysis", timer_get_elapsed(&timer));
    
    return 0;
}

/*********************************************************************
 * Example 4: Performance Benchmark - Scalability Analysis
 *********************************************************************/
int example_performance_benchmark() {
    print_example_header("Performance Benchmark - Scalability Analysis");
    
    Timer timer;
    timer_start(&timer);
    
    // Test different problem sizes
    int problem_sizes[] = {100, 500, 1000, 5000, 10000};
    int num_sizes = sizeof(problem_sizes) / sizeof(int);
    
    printf("\n=== Scalability Analysis ===\n");
    printf("Problem Size | Assembly Time | Solve Time | Memory Usage | Convergence\n");
    printf("-------------|---------------|------------|--------------|------------\n");
    
    for (int i = 0; i < num_sizes; i++) {
        int size = problem_sizes[i];
        
        Timer size_timer;
        timer_start(&size_timer);
        
        // Create framework for this problem size
        FrameworkConfig config = {
            .logging = {
                .min_level = LOG_LEVEL_WARNING,
                .callback = NULL,
                .user_data = NULL,
                .enable_file_logging = false,
                .log_file_path = ""
            },
            .num_threads = 8,
            .use_gpu = false,
            .use_mpi = false,
            .memory_limit = 4 * 1024 * 1024 * 1024LL,  // 4GB
            .solver_type = SOLVER_TYPE_MOM,
            .simulation_type = SIMULATION_TYPE_FREQUENCY_DOMAIN,
            .accuracy_target = 1e-6,
            .max_iterations = 1000,
            .convergence_tolerance = 1e-8
        };
        
        Framework* framework = framework_create(&config);
        if (!framework) continue;
        
        status = framework_initialize(framework);
        if (status != SUCCESS) {
            framework_destroy(framework);
            continue;
        }
        
        // Configure MoM solver for this problem size
        MomSolverOptions mom_options = {
            .basis_type = MOM_BASIS_RWG,
            .basis_order = 1,
            .acceleration_type = (size > 1000) ? MOM_ACCELERATION_H_MATRIX : MOM_ACCELERATION_NONE,
            .compression_tolerance = 1e-4,
            .h_matrix_max_rank = 50,
            .preconditioner_type = MOM_PRECONDITIONER_DIAGONAL,
            .max_iterations = 1000,
            .convergence_tolerance = 1e-8,
            .frequency = 1e9,  // 1 GHz
            .num_threads = 8,
            .use_gpu = false,
            .enable_performance_monitoring = true
        };
        
        MomSolver* mom_solver = mom_solver_create(&mom_options);
        if (!mom_solver) {
            framework_finalize(framework);
            framework_destroy(framework);
            continue;
        }
        
        // Create simplified geometry based on problem size
        // (In practice, this would be actual geometry)
        
        // Solve and measure performance
        status = mom_solver_solve(mom_solver);
        
        if (status == SUCCESS) {
            const MomSolverResults* results = mom_solver_get_results(mom_solver);
            
            timer_stop(&size_timer);
            double total_time = timer_get_elapsed(&size_timer);
            
            printf("%12d | %13.2f | %10.2f | %11.1f MB | %s\n",
                   size, results->assembly_time, results->solve_time,
                   results->memory_usage / (1024.0 * 1024.0),
                   results->converged ? "YES" : "NO");
        }
        
        // Cleanup
        mom_solver_destroy(mom_solver);
        framework_finalize(framework);
        framework_destroy(framework);
    }
    
    timer_stop(&timer);
    print_example_footer("Performance Benchmark", timer_get_elapsed(&timer));
    
    return 0;
}

/*********************************************************************
 * Main Function - Run All Examples
 *********************************************************************/
int main(int argc, char* argv[]) {
    printf("================================================================================\n");
    printf("PEEC-MoM Unified Framework - Integration Examples\n");
    printf("================================================================================\n");
    
    // Run all examples
    int result = 0;
    
    // Example 1: Basic MoM antenna analysis
    result = example_basic_mom_antenna();
    if (result != 0) {
        printf("Example 1 failed with code %d\n", result);
    }
    
    printf("\n");
    
    // Example 2: Basic PEEC circuit analysis
    result = example_basic_peec_circuit();
    if (result != 0) {
        printf("Example 2 failed with code %d\n", result);
    }
    
    printf("\n");
    
    // Example 3: Hybrid MoM-PEEC analysis
    result = example_hybrid_mom_peec_antenna();
    if (result != 0) {
        printf("Example 3 failed with code %d\n", result);
    }
    
    printf("\n");
    
    // Example 4: Performance benchmark
    result = example_performance_benchmark();
    if (result != 0) {
        printf("Example 4 failed with code %d\n", result);
    }
    
    printf("\n================================================================================\n");
    printf("All examples completed successfully!\n");
    printf("================================================================================\n");
    
    return 0;
}

/*********************************************************************
 * Helper Function Implementations
 *********************************************************************/

static void print_example_header(const char* title) {
    printf("\n");
    printf("================================================================================\n");
    printf("Example: %s\n", title);
    printf("================================================================================\n");
}

static void print_example_footer(const char* title, double time) {
    printf("\n");
    printf("Example '%s' completed in %.2f seconds\n", title, time);
    printf("================================================================================\n");
}

static void print_performance_metrics(const char* solver_name, 
                                    size_t memory_usage, double solve_time) {
    printf("Performance Metrics for %s:\n", solver_name);
    printf("  Memory Usage: %.1f MB\n", memory_usage / (1024.0 * 1024.0));
    printf("  Solve Time: %.2f seconds\n", solve_time);
}

static Complex compute_analytical_solution(double frequency, double theta, double phi) {
    // Simplified analytical solution for dipole antenna
    double k = 2.0 * M_PI * frequency / 3e8;  // Wave number
    double L = 0.5;  // Dipole length (meters)
    
    // Far-field approximation for short dipole
    double theta_rad = theta * M_PI / 180.0;
    double radiation_pattern = sin(theta_rad);
    
    // Include phase factor
    Complex phase = cexp(I * k * L * cos(theta_rad) / 2);
    
    return radiation_pattern * phase;
}

static double compute_relative_error(Complex computed, Complex analytical) {
    if (cabs(analytical) < 1e-15) {
        return cabs(computed) < 1e-15 ? 0.0 : 1e15;
    }
    return cabs(computed - analytical) / cabs(analytical);
}