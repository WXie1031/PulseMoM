/******************************************************************************
 * PCB Complete Calculation Workflow Example
 * 
 * This example demonstrates the complete PCB analysis workflow using the
 * PulseMoM PCB interfaces, from file input to GPU-accelerated simulation.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "../src/io/pcb_file_io.h"
#include "../src/io/pcb_electromagnetic_modeling.h"
#include "../src/io/pcb_simulation_workflow.h"
#include "../src/io/pcb_gpu_acceleration.h"

/******************************************************************************
 * Example 1: Basic PCB Analysis Workflow
 ******************************************************************************/

void example_basic_pcb_analysis() {
    printf("=== Example 1: Basic PCB Analysis Workflow ===\n");
    
    // Step 1: Create a simple PCB design
    printf("Step 1: Creating PCB design...\n");
    PCBDesign* pcb = create_empty_pcb_design();
    
    strcpy(pcb->design_name, "Example_PCB_Filter");
    strcpy(pcb->version, "1.0");
    strcpy(pcb->created_by, "PulseMoM_Example");
    
    // Define 2-layer PCB (microstrip structure)
    pcb->num_layers = 2;
    pcb->layers = (PCBLayerInfo*)malloc(2 * sizeof(PCBLayerInfo));
    
    // Top layer: Copper signal layer
    strcpy(pcb->layers[0].layer_name, "Top_Signal");
    pcb->layers[0].layer_type = LAYER_TYPE_COPPER;
    pcb->layers[0].layer_function = LAYER_FUNCTION_SIGNAL;
    pcb->layers[0].elevation = 0.0;
    pcb->layers[0].thickness = 35e-6; // 35um
    pcb->layers[0].conductivity = 5.8e7; // Copper
    pcb->layers[0].permittivity = 1.0;
    
    // Bottom layer: Ground plane
    strcpy(pcb->layers[1].layer_name, "Bottom_Ground");
    pcb->layers[1].layer_type = LAYER_TYPE_COPPER;
    pcb->layers[1].layer_function = LAYER_FUNCTION_GROUND;
    pcb->layers[1].elevation = 0.2e-3; // 200um substrate
    pcb->layers[1].thickness = 35e-6;
    pcb->layers[1].conductivity = 5.8e7;
    pcb->layers[1].permittivity = 1.0;
    
    // Create microstrip filter geometry
    pcb->num_primitives_per_layer = (int*)malloc(2 * sizeof(int));
    pcb->primitives = (PCBPrimitive**)malloc(2 * sizeof(PCBPrimitive*));
    
    // Top layer: Microstrip filter
    pcb->num_primitives_per_layer[0] = 3;
    pcb->primitives[0] = (PCBPrimitive*)malloc(3 * sizeof(PCBPrimitive));
    
    // Input transmission line
    pcb->primitives[0][0].type = PRIMITIVE_TYPE_LINE;
    pcb->primitives[0][0].layer_index = 0;
    pcb->primitives[0][0].net_index = 0;
    pcb->primitives[0][0].data.line.start_x = 0.0;
    pcb->primitives[0][0].data.line.start_y = 0.005;
    pcb->primitives[0][0].data.line.end_x = 0.01;
    pcb->primitives[0][0].data.line.end_y = 0.005;
    pcb->primitives[0][0].data.line.width = 0.3e-3; // 50Ω microstrip
    
    // Resonator stub
    pcb->primitives[0][1].type = PRIMITIVE_TYPE_LINE;
    pcb->primitives[0][1].layer_index = 0;
    pcb->primitives[0][1].net_index = 0;
    pcb->primitives[0][1].data.line.start_x = 0.005;
    pcb->primitives[0][1].data.line.start_y = 0.005;
    pcb->primitives[0][1].data.line.end_x = 0.005;
    pcb->primitives[0][1].data.line.end_y = 0.015;
    pcb->primitives[0][1].data.line.width = 0.3e-3;
    
    // Output transmission line
    pcb->primitives[0][2].type = PRIMITIVE_TYPE_LINE;
    pcb->primitives[0][2].layer_index = 0;
    pcb->primitives[0][2].net_index = 0;
    pcb->primitives[0][2].data.line.start_x = 0.01;
    pcb->primitives[0][2].data.line.start_y = 0.005;
    pcb->primitives[0][2].data.line.end_x = 0.02;
    pcb->primitives[0][2].data.line.end_y = 0.005;
    pcb->primitives[0][2].data.line.width = 0.3e-3;
    
    // Bottom layer: Ground plane
    pcb->num_primitives_per_layer[1] = 1;
    pcb->primitives[1] = (PCBPrimitive*)malloc(1 * sizeof(PCBPrimitive));
    pcb->primitives[1][0].type = PRIMITIVE_TYPE_RECTANGLE;
    pcb->primitives[1][0].layer_index = 1;
    pcb->primitives[1][0].net_index = 1;
    pcb->primitives[1][0].data.rectangle.x = 0.0;
    pcb->primitives[1][0].data.rectangle.y = 0.0;
    pcb->primitives[1][0].data.rectangle.width = 0.02;
    pcb->primitives[1][0].data.rectangle.height = 0.02;
    pcb->primitives[1][0].data.rectangle.rotation = 0.0;
    
    // Set material properties
    pcb->base_material_er = 4.2; // FR4
    pcb->base_material_tan_delta = 0.02;
    pcb->copper_conductivity = 5.8e7;
    
    // Define nets
    pcb->num_nets = 2;
    pcb->net_names = (char**)malloc(2 * sizeof(char*));
    pcb->net_names[0] = strdup("RF_SIGNAL");
    pcb->net_names[1] = strdup("GROUND");
    
    printf("✓ PCB design created with %d layers and %d nets\n", pcb->num_layers, pcb->num_nets);
    
    // Step 2: Save PCB to file
    printf("Step 2: Saving PCB to Gerber file...\n");
    write_gerber_rs274x("example_filter.gbr", pcb);
    printf("✓ PCB saved to example_filter.gbr\n");
    
    // Step 3: Create electromagnetic model
    printf("Step 3: Creating electromagnetic model...\n");
    PCBEMModel* em_model = create_pcb_em_model(pcb);
    
    // Configure EM modeling parameters
    em_model->params.mesh_resolution = 0.1e-3; // 0.1mm resolution
    em_model->params.frequency_start = 1e9; // 1 GHz
    em_model->params.frequency_stop = 10e9; // 10 GHz
    em_model->params.frequency_points = 101;
    em_model->params.adaptive_refinement = 1;
    em_model->params.refinement_threshold = 0.1;
    
    printf("✓ EM model created with frequency range %.1f-%.1f GHz\n", 
           em_model->params.frequency_start/1e9, em_model->params.frequency_stop/1e9);
    
    // Step 4: Define ports
    printf("Step 4: Defining ports...\n");
    PCBPortDefinition ports[2];
    
    // Input port
    ports[0].port_id = 1;
    ports[0].port_name = strdup("INPUT");
    ports[0].layer_index = 0;
    ports[0].center_x = 0.002;
    ports[0].center_y = 0.005;
    ports[0].width = 0.3e-3;
    ports[0].impedance = 50.0;
    ports[0].port_type = PORT_TYPE_MICROSTRIP;
    
    // Output port
    ports[1].port_id = 2;
    ports[1].port_name = strdup("OUTPUT");
    ports[1].layer_index = 0;
    ports[1].center_x = 0.018;
    ports[1].center_y = 0.005;
    ports[1].width = 0.3e-3;
    ports[1].impedance = 50.0;
    ports[1].port_type = PORT_TYPE_MICROSTRIP;
    
    define_pcb_ports(em_model, ports, 2);
    printf("✓ 2 ports defined (INPUT and OUTPUT)\n");
    
    // Step 5: Generate mesh
    printf("Step 5: Generating computational mesh...\n");
    generate_pcb_mesh(em_model);
    printf("✓ Mesh generated with %d triangles\n", em_model->num_triangles);
    
    // Step 6: Run electromagnetic simulation
    printf("Step 6: Running electromagnetic simulation...\n");
    run_pcb_em_simulation(em_model);
    printf("✓ EM simulation completed\n");
    
    // Step 7: Extract S-parameters
    printf("Step 7: Extracting S-parameters...\n");
    PCBEMSimulationResults* results = extract_pcb_simulation_results(em_model);
    
    printf("✓ S-parameters extracted at %d frequency points\n", results->num_frequencies);
    printf("   S11 at 5 GHz: %.2f dB\n", results->s_parameters[50].s11_db);
    printf("   S21 at 5 GHz: %.2f dB\n", results->s_parameters[50].s21_db);
    
    // Step 8: Generate report
    printf("Step 8: Generating simulation report...\n");
    generate_pcb_simulation_report_html("example_filter_report.html", results);
    printf("✓ Report saved to example_filter_report.html\n");
    
    // Cleanup
    destroy_pcb_design(pcb);
    destroy_pcb_em_model(em_model);
    
    printf("✓ Basic PCB analysis workflow completed successfully!\n\n");
}

/******************************************************************************
 * Example 2: Advanced Multi-layer PCB with GPU Acceleration
 ******************************************************************************/

void example_advanced_multilayer_pcb() {
    printf("=== Example 2: Advanced Multi-layer PCB with GPU Acceleration ===\n");
    
    // Step 1: Create workflow controller
    printf("Step 1: Creating workflow controller with GPU acceleration...\n");
    PCBWorkflowController* controller = create_pcb_workflow_controller();
    
    // Configure for GPU acceleration
    PCBWorkflowParams params;
    params.frequency_start = 100e6; // 100 MHz
    params.frequency_stop = 20e9;   // 20 GHz
    params.frequency_points = 201;
    params.mesh_density = 1.0;      // Standard density
    params.convergence_tolerance = 1e-6;
    params.max_iterations = 200;
    params.use_gpu_acceleration = 1;
    params.num_gpus = 1;
    
    configure_pcb_simulation(controller, &params);
    printf("✓ Workflow controller configured with GPU acceleration\n");
    
    // Step 2: Load PCB design from file
    printf("Step 2: Loading multi-layer PCB design...\n");
    PCBDesign* pcb = read_gerber_rs274x("advanced_pcb.gbr");
    if (!pcb) {
        printf("   Creating example multi-layer PCB...\n");
        pcb = create_multilayer_pcb_design();
    }
    
    load_pcb_design(controller, pcb);
    printf("✓ PCB design loaded: %d layers, %d nets\n", pcb->num_layers, pcb->num_nets);
    
    // Step 3: Initialize GPU context
    printf("Step 3: Initializing GPU acceleration...\n");
    PCBGPUContext* gpu_context = create_pcb_gpu_context(
        controller->em_model->num_triangles,
        controller->em_model->num_ports,
        params.frequency_points
    );
    
    optimize_pcb_gpu_performance(gpu_context);
    printf("✓ GPU context optimized: batch_size=%d\n", gpu_context->optimal_batch_size);
    
    // Step 4: Run complete simulation workflow
    printf("Step 4: Running complete simulation workflow...\n");
    
    clock_t start = clock();
    run_complete_pcb_simulation(controller);
    clock_t end = clock();
    
    double simulation_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("✓ Simulation completed in %.2f seconds\n", simulation_time);
    
    // Step 5: Analyze results
    printf("Step 5: Analyzing simulation results...\n");
    PCBEMSimulationResults* results = get_pcb_simulation_results(controller);
    
    // Find resonant frequencies
    printf("   Resonant frequencies:\n");
    for (int i = 1; i < results->num_frequencies; i++) {
        if (results->s_parameters[i].s11_db < -10.0 && 
            results->s_parameters[i-1].s11_db >= -10.0) {
            printf("   - %.2f GHz (S11 < -10 dB)\n", 
                   results->frequencies[i] / 1e9);
        }
    }
    
    // Calculate bandwidth
    double min_s11 = 0.0;
    int min_idx = 0;
    for (int i = 0; i < results->num_frequencies; i++) {
        if (results->s_parameters[i].s11_db < min_s11) {
            min_s11 = results->s_parameters[i].s11_db;
            min_idx = i;
        }
    }
    
    printf("   Minimum S11: %.2f dB at %.2f GHz\n", 
           min_s11, results->frequencies[min_idx] / 1e9);
    
    // Step 6: Extract coupling analysis
    printf("Step 6: Performing coupling analysis...\n");
    PCBCouplingParameters coupling_params;
    extract_pcb_coupling_parameters(controller->em_model, &coupling_params);
    
    printf("   Layer-to-layer coupling:\n");
    for (int i = 0; i < pcb->num_layers - 1; i++) {
        printf("   - Layer %d to %d: %.2f dB\n", 
               i, i+1, coupling_params.layer_coupling[i]);
    }
    
    // Step 7: Signal integrity analysis
    printf("Step 7: Signal integrity analysis...\n");
    PCBSignalIntegrityResults si_results;
    analyze_pcb_signal_integrity(controller->em_model, &si_results);
    
    printf("   Signal integrity metrics:\n");
    printf("   - Insertion loss: %.2f dB at 10 GHz\n", si_results.insertion_loss_10ghz);
    printf("   - Return loss: %.2f dB at 10 GHz\n", si_results.return_loss_10ghz);
    printf("   - Crosstalk: %.2f dB at 10 GHz\n", si_results.crosstalk_10ghz);
    
    // Step 8: Generate comprehensive report
    printf("Step 8: Generating comprehensive report...\n");
    generate_pcb_simulation_report_html("advanced_pcb_report.html", results);
    generate_pcb_simulation_report_csv("advanced_pcb_data.csv", results);
    generate_pcb_simulation_report_json("advanced_pcb_analysis.json", controller);
    
    printf("✓ Reports generated:\n");
    printf("   - HTML report: advanced_pcb_report.html\n");
    printf("   - CSV data: advanced_pcb_data.csv\n");
    printf("   - JSON analysis: advanced_pcb_analysis.json\n");
    
    // Cleanup
    destroy_pcb_workflow_controller(controller);
    destroy_pcb_gpu_context(gpu_context);
    
    printf("✓ Advanced multi-layer PCB analysis completed successfully!\n\n");
}

/******************************************************************************
 * Example 3: PCB Design Optimization
 ******************************************************************************/

void example_pcb_design_optimization() {
    printf("=== Example 3: PCB Design Optimization ===\n");
    
    // Step 1: Load initial PCB design
    printf("Step 1: Loading initial PCB design...\n");
    PCBDesign* initial_pcb = read_gerber_rs274x("initial_filter.gbr");
    if (!initial_pcb) {
        initial_pcb = create_test_pcb_design();
    }
    
    // Step 2: Define optimization parameters
    printf("Step 2: Defining optimization parameters...\n");
    PCBOptimizationParams opt_params;
    opt_params.target_frequency = 2.4e9; // 2.4 GHz
    opt_params.bandwidth = 200e6; // 200 MHz
    opt_params.max_insertion_loss = -3.0; // 3 dB
    opt_params.min_return_loss = -15.0; // 15 dB
    opt_params.max_iterations = 50;
    opt_params.convergence_tolerance = 1e-4;
    
    printf("✓ Optimization target: %.1f GHz bandpass filter\n", opt_params.target_frequency/1e9);
    
    // Step 3: Create optimization workflow
    printf("Step 3: Creating optimization workflow...\n");
    PCBWorkflowController* optimizer = create_pcb_workflow_controller();
    
    PCBWorkflowParams sim_params;
    sim_params.frequency_start = 1e9;
    sim_params.frequency_stop = 5e9;
    sim_params.frequency_points = 101;
    sim_params.use_gpu_acceleration = 1;
    
    configure_pcb_simulation(optimizer, &sim_params);
    
    // Step 4: Run optimization iterations
    printf("Step 4: Running optimization iterations...\n");
    
    PCBDesign* current_pcb = copy_pcb_design(initial_pcb);
    double best_fitness = -1e9;
    PCBDesign* best_pcb = NULL;
    
    for (int iter = 0; iter < opt_params.max_iterations; iter++) {
        printf("   Iteration %d/%d...\r", iter + 1, opt_params.max_iterations);
        fflush(stdout);
        
        // Simulate current design
        load_pcb_design(optimizer, current_pcb);
        run_complete_pcb_simulation(optimizer);
        
        PCBEMSimulationResults* results = get_pcb_simulation_results(optimizer);
        
        // Calculate fitness
        double fitness = calculate_pcb_fitness(results, &opt_params);
        
        if (fitness > best_fitness) {
            best_fitness = fitness;
            if (best_pcb) destroy_pcb_design(best_pcb);
            best_pcb = copy_pcb_design(current_pcb);
        }
        
        // Optimize design parameters
        optimize_pcb_geometry(current_pcb, results, &opt_params);
        
        // Check convergence
        if (fitness > 0.95) break; // Good enough solution
    }
    printf("\n✓ Optimization completed: best fitness = %.3f\n", best_fitness);
    
    // Step 5: Validate optimized design
    printf("Step 5: Validating optimized design...\n");
    load_pcb_design(optimizer, best_pcb);
    run_complete_pcb_simulation(optimizer);
    
    PCBEMSimulationResults* final_results = get_pcb_simulation_results(optimizer);
    
    printf("   Optimized filter performance:\n");
    printf("   - Center frequency: %.2f GHz\n", 
           find_resonant_frequency(final_results) / 1e9);
    printf("   - 3-dB bandwidth: %.1f MHz\n", 
           calculate_3db_bandwidth(final_results) / 1e6);
    printf("   - Minimum insertion loss: %.2f dB\n", 
           find_minimum_insertion_loss(final_results));
    printf("   - Maximum return loss: %.2f dB\n", 
           find_maximum_return_loss(final_results));
    
    // Step 6: Save optimized design
    printf("Step 6: Saving optimized design...\n");
    write_gerber_rs274x("optimized_filter.gbr", best_pcb);
    generate_pcb_simulation_report_html("optimization_report.html", final_results);
    
    printf("✓ Optimized design saved to optimized_filter.gbr\n");
    
    // Cleanup
    destroy_pcb_design(initial_pcb);
    destroy_pcb_design(current_pcb);
    destroy_pcb_design(best_pcb);
    destroy_pcb_workflow_controller(optimizer);
    
    printf("✓ PCB design optimization completed successfully!\n\n");
}

/******************************************************************************
 * Example 4: Batch PCB Analysis
 ******************************************************************************/

void example_batch_pcb_analysis() {
    printf("=== Example 4: Batch PCB Analysis ===\n");
    
    // Step 1: Define batch of PCB designs
    printf("Step 1: Defining batch of PCB designs...\n");
    const char* pcb_files[] = {
        "filter_1ghz.gbr",
        "filter_2ghz.gbr",
        "filter_5ghz.gbr",
        "amplifier_pcb.gbr",
        "antenna_pcb.gbr"
    };
    
    int num_pcbs = sizeof(pcb_files) / sizeof(pcb_files[0]);
    printf("✓ Batch contains %d PCB designs\n", num_pcbs);
    
    // Step 2: Create batch workflow controller
    printf("Step 2: Creating batch workflow controller...\n");
    PCBWorkflowController* batch_controller = create_pcb_workflow_controller();
    
    PCBWorkflowParams batch_params;
    batch_params.frequency_start = 100e6;
    batch_params.frequency_stop = 20e9;
    batch_params.frequency_points = 201;
    batch_params.use_gpu_acceleration = 1;
    batch_params.num_gpus = 1;
    
    configure_pcb_simulation(batch_controller, &batch_params);
    
    // Step 3: Initialize batch processing
    printf("Step 3: Initializing batch processing...\n");
    PCBBatchResults* batch_results = create_pcb_batch_results(num_pcbs);
    
    clock_t batch_start = clock();
    
    // Step 4: Process each PCB in batch
    printf("Step 4: Processing PCB batch...\n");
    for (int i = 0; i < num_pcbs; i++) {
        printf("   Processing PCB %d/%d: %s\n", i + 1, num_pcbs, pcb_files[i]);
        
        // Load PCB
        PCBDesign* pcb = read_gerber_rs274x(pcb_files[i]);
        if (!pcb) {
            printf("   ⚠ Warning: Could not load %s, skipping...\n", pcb_files[i]);
            continue;
        }
        
        // Simulate
        load_pcb_design(batch_controller, pcb);
        run_complete_pcb_simulation(batch_controller);
        
        // Store results
        PCBEMSimulationResults* results = get_pcb_simulation_results(batch_controller);
        store_pcb_batch_result(batch_results, i, pcb_files[i], results);
        
        // Quick analysis
        double center_freq = find_resonant_frequency(results);
        double bandwidth = calculate_3db_bandwidth(results);
        double min_loss = find_minimum_insertion_loss(results);
        
        printf("   ✓ Analysis complete: %.1f GHz, %.1f MHz BW, %.1f dB loss\n",
               center_freq / 1e9, bandwidth / 1e6, min_loss);
        
        destroy_pcb_design(pcb);
    }
    
    clock_t batch_end = clock();
    double batch_time = ((double)(batch_end - batch_start)) / CLOCKS_PER_SEC;
    
    printf("✓ Batch processing completed in %.1f seconds (%.1f s/PCB)\n", 
           batch_time, batch_time / num_pcbs);
    
    // Step 5: Generate batch summary report
    printf("Step 5: Generating batch summary report...\n");
    
    printf("   Batch analysis summary:\n");
    printf("   %-20s %-12s %-12s %-12s %-12s\n", 
           "PCB Design", "Center Freq", "Bandwidth", "Min Loss", "Type");
    printf("   %-20s %-12s %-12s %-12s %-12s\n", 
           "----------", "-----------", "---------", "--------", "----");
    
    for (int i = 0; i < batch_results->num_results; i++) {
        PCBBatchResult* result = &batch_results->results[i];
        const char* type = classify_pcb_type(result);
        
        printf("   %-20s %-12.1f %-12.1f %-12.1f %-12s\n",
               result->pcb_name,
               result->center_frequency / 1e9,
               result->bandwidth / 1e6,
               result->minimum_insertion_loss,
               type);
    }
    
    // Step 6: Generate detailed batch report
    generate_pcb_batch_report_html("batch_analysis_report.html", batch_results);
    generate_pcb_batch_report_csv("batch_analysis_summary.csv", batch_results);
    generate_pcb_batch_report_json("batch_analysis_data.json", batch_results);
    
    printf("✓ Batch reports generated:\n");
    printf("   - HTML report: batch_analysis_report.html\n");
    printf("   - CSV summary: batch_analysis_summary.csv\n");
    printf("   - JSON data: batch_analysis_data.json\n");
    
    // Cleanup
    destroy_pcb_batch_results(batch_results);
    destroy_pcb_workflow_controller(batch_controller);
    
    printf("✓ Batch PCB analysis completed successfully!\n\n");
}

/******************************************************************************
 * Helper Functions
 ******************************************************************************/

PCBDesign* create_multilayer_pcb_design() {
    PCBDesign* pcb = create_empty_pcb_design();
    
    strcpy(pcb->design_name, "Example_Multilayer_PCB");
    strcpy(pcb->version, "1.0");
    strcpy(pcb->created_by, "PulseMoM_Example");
    
    // 6-layer PCB stackup
    pcb->num_layers = 6;
    pcb->layers = (PCBLayerInfo*)malloc(6 * sizeof(PCBLayerInfo));
    
    double total_thickness = 0.0;
    
    // Layer 1: Top signal
    strcpy(pcb->layers[0].layer_name, "L1_Top_Signal");
    pcb->layers[0].layer_type = LAYER_TYPE_COPPER;
    pcb->layers[0].layer_function = LAYER_FUNCTION_SIGNAL;
    pcb->layers[0].elevation = total_thickness;
    pcb->layers[0].thickness = 18e-6; // 0.5 oz copper
    pcb->layers[0].conductivity = 5.8e7;
    pcb->layers[0].permittivity = 1.0;
    total_thickness += pcb->layers[0].thickness;
    
    // Layer 2: Prepreg
    strcpy(pcb->layers[1].layer_name, "L2_Prepreg");
    pcb->layers[1].layer_type = LAYER_TYPE_DIELECTRIC;
    pcb->layers[1].layer_function = LAYER_FUNCTION_DIELECTRIC;
    pcb->layers[1].elevation = total_thickness;
    pcb->layers[1].thickness = 100e-6;
    pcb->layers[1].conductivity = 0.0;
    pcb->layers[1].permittivity = 4.2;
    total_thickness += pcb->layers[1].thickness;
    
    // Layer 3: Ground plane
    strcpy(pcb->layers[2].layer_name, "L3_Ground");
    pcb->layers[2].layer_type = LAYER_TYPE_COPPER;
    pcb->layers[2].layer_function = LAYER_FUNCTION_GROUND;
    pcb->layers[2].elevation = total_thickness;
    pcb->layers[2].thickness = 35e-6; // 1 oz copper
    pcb->layers[2].conductivity = 5.8e7;
    pcb->layers[2].permittivity = 1.0;
    total_thickness += pcb->layers[2].thickness;
    
    // Layer 4: Core
    strcpy(pcb->layers[3].layer_name, "L4_Core");
    pcb->layers[3].layer_type = LAYER_TYPE_DIELECTRIC;
    pcb->layers[3].layer_function = LAYER_FUNCTION_DIELECTRIC;
    pcb->layers[3].elevation = total_thickness;
    pcb->layers[3].thickness = 200e-6;
    pcb->layers[3].conductivity = 0.0;
    pcb->layers[3].permittivity = 4.2;
    total_thickness += pcb->layers[3].thickness;
    
    // Layer 5: Power plane
    strcpy(pcb->layers[4].layer_name, "L5_Power");
    pcb->layers[4].layer_type = LAYER_TYPE_COPPER;
    pcb->layers[4].layer_function = LAYER_FUNCTION_POWER;
    pcb->layers[4].elevation = total_thickness;
    pcb->layers[4].thickness = 35e-6;
    pcb->layers[4].conductivity = 5.8e7;
    pcb->layers[4].permittivity = 1.0;
    total_thickness += pcb->layers[4].thickness;
    
    // Layer 6: Bottom signal
    strcpy(pcb->layers[5].layer_name, "L6_Bottom_Signal");
    pcb->layers[5].layer_type = LAYER_TYPE_COPPER;
    pcb->layers[5].layer_function = LAYER_FUNCTION_SIGNAL;
    pcb->layers[5].elevation = total_thickness;
    pcb->layers[5].thickness = 18e-6;
    pcb->layers[5].conductivity = 5.8e7;
    pcb->layers[5].permittivity = 1.0;
    
    return pcb;
}

/******************************************************************************
 * Main Function
 ******************************************************************************/

int main() {
    printf("PulseMoM PCB Complete Calculation Workflow Examples\n");
    printf("==================================================\n\n");
    
    // Initialize CUDA if available
    printf("Initializing GPU acceleration...\n");
    int device_count;
    cudaGetDeviceCount(&device_count);
    if (device_count > 0) {
        printf("✓ Found %d CUDA-capable GPU(s)\n", device_count);
        cudaSetDevice(0);
    } else {
        printf("⚠ No CUDA-capable GPUs found, running CPU-only examples\n");
    }
    printf("\n");
    
    // Run examples
    example_basic_pcb_analysis();
    example_advanced_multilayer_pcb();
    example_pcb_design_optimization();
    example_batch_pcb_analysis();
    
    printf("==================================================\n");
    printf("All PCB workflow examples completed successfully!\n");
    printf("✓ Basic PCB analysis: Simple microstrip filter\n");
    printf("✓ Advanced multi-layer: 6-layer PCB with GPU acceleration\n");
    printf("✓ Design optimization: Automated parameter tuning\n");
    printf("✓ Batch analysis: Multiple PCB designs\n");
    printf("==================================================\n");
    
    return 0;
}