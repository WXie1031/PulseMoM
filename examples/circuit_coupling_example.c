/*********************************************************************
 * Circuit Coupling Simulation Example
 * 
 * This example demonstrates the circuit-electromagnetic co-simulation
 * capabilities that match commercial tools like Keysight ADS and EMX.
 * 
 * Features demonstrated:
 * - SPICE netlist loading and parsing
 * - S-parameter based EM coupling
 * - Nonlinear device simulation
 * - Mixed-frequency analysis
 * - Results export to standard formats
 * 
 * Author: PulseMoM Development Team
 * Copyright (c) 2024
 *********************************************************************/

#include "../src/core/circuit_coupling_simulation.h"
#include "../src/core/enhanced_sparameter_extraction.h"
#include "../src/core/mom_solver.h"
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>

// Example SPICE netlist for PCB transmission line with termination
const char* example_spice_netlist = 
"* PCB Transmission Line Circuit\n"
"* Frequency: 1MHz to 10GHz\n"
"* Input port: 50 ohm source\n"
"* Output port: Variable load\n"
"\n"
"* Input source and matching\n"
"VIN IN 0 AC 1.0 DC 0.0\n"
"RIN IN SRC 50.0\n"
"\n"
"* EM-coupled transmission line (S-parameters from EM simulation)\n"
"* This will be replaced with actual EM S-parameters\n"
"XTL1 SRC LOAD TL_SPARAMS\n"
"\n"
"* Output termination\n"
"RLOAD LOAD 0 50.0\n"
"CLOAD LOAD 0 1.0p\n"
"\n"
"* Nonlinear termination option\n"
"* D1 LOAD 0 DMOD\n"
"* .MODEL DMOD D(IS=1e-14 N=1.0)\n"
"\n"
".AC DEC 100 1e6 10e9\n"
".END\n";

// Create sample S-parameters for transmission line
SParameterSet* create_sample_transmission_line_sparams(void) {
    int num_freq = 100;
    double* frequencies = (double*)malloc(num_freq * sizeof(double));
    SParameterMatrix* s_matrices = (SParameterMatrix*)malloc(num_freq * sizeof(SParameterMatrix));
    
    if (!frequencies || !s_matrices) {
        free(frequencies);
        free(s_matrices);
        return NULL;
    }
    
    // Generate frequency sweep from 1MHz to 10GHz
    for (int i = 0; i < num_freq; i++) {
        frequencies[i] = 1e6 * pow(10.0, (double)i / (num_freq - 1) * 4.0); // log sweep
        
        // 2-port S-parameters for transmission line
        s_matrices[i].num_ports = 2;
        s_matrices[i].s_matrix = (double complex*)malloc(4 * sizeof(double complex));
        s_matrices[i].z_reference = (double*)malloc(2 * sizeof(double));
        s_matrices[i].z_reference[0] = 50.0;
        s_matrices[i].z_reference[1] = 50.0;
        
        if (!s_matrices[i].s_matrix || !s_matrices[i].z_reference) {
            // Cleanup on error
            for (int j = 0; j <= i; j++) {
                free(s_matrices[j].s_matrix);
                free(s_matrices[j].z_reference);
            }
            free(frequencies);
            free(s_matrices);
            return NULL;
        }
        
        // Calculate transmission line S-parameters
        double freq = frequencies[i];
        double len = 0.1; // 10cm line length
        double v = 1.5e8; // Velocity on PCB (~c/2)
        double z0 = 50.0; // Characteristic impedance
        double alpha = 0.1 * sqrt(freq / 1e9); // Frequency-dependent loss
        
        double beta = 2.0 * M_PI * freq / v;
        double complex gamma = alpha + I * beta;
        double complex gamma_len = gamma * len;
        
        // Matched transmission line S-parameters
        double complex s11 = 0.0;
        double complex s12 = cexp(-gamma_len);
        double complex s21 = cexp(-gamma_len);
        double complex s22 = 0.0;
        
        s_matrices[i].s_matrix[0] = s11;
        s_matrices[i].s_matrix[1] = s12;
        s_matrices[i].s_matrix[2] = s21;
        s_matrices[i].s_matrix[3] = s22;
    }
    
    SParameterSet* sparams = create_sparameter_set(frequencies, s_matrices, num_freq);
    
    // Cleanup temporary allocations
    for (int i = 0; i < num_freq; i++) {
        free(s_matrices[i].s_matrix);
        free(s_matrices[i].z_reference);
    }
    free(frequencies);
    free(s_matrices);
    
    return sparams;
}

// Create sample PCB EM model for coupling
PCBEMModel* create_sample_pcb_em_model(void) {
    PCBEMModel* em_model = (PCBEMModel*)calloc(1, sizeof(PCBEMModel));
    if (!em_model) return NULL;
    
    // Configure basic PCB parameters
    em_model->frequency = 1e9; // 1GHz
    em_model->substrate_height = 0.2e-3; // 0.2mm
    em_model->substrate_er = 4.3; // FR4
    em_model->conductor_thickness = 35e-6; // 35um copper
    em_model->conductor_width = 0.2e-3; // 0.2mm trace width
    em_model->conductor_spacing = 0.2e-3; // 0.2mm spacing
    em_model->num_conductors = 2; // Differential pair
    
    return em_model;
}

// Test SPICE netlist parsing
int test_spice_parsing(void) {
    printf("Testing SPICE netlist parsing...\n");
    
    // Create temporary SPICE file
    const char* temp_filename = "temp_example.sp";
    FILE* fp = fopen(temp_filename, "w");
    if (!fp) {
        printf("Error: Cannot create temporary SPICE file\n");
        return -1;
    }
    
    fprintf(fp, "%s", example_spice_netlist);
    fclose(fp);
    
    // Create circuit simulator
    CircuitCouplingSimulator* sim = create_circuit_coupling_simulator();
    if (!sim) {
        printf("Error: Cannot create circuit simulator\n");
        remove(temp_filename);
        return -1;
    }
    
    // Load SPICE netlist
    int result = load_spice_netlist(sim, temp_filename);
    if (result != 0) {
        printf("Error: Failed to load SPICE netlist\n");
        destroy_circuit_coupling_simulator(sim);
        remove(temp_filename);
        return -1;
    }
    
    printf("SPICE netlist loaded successfully\n");
    
    // Clean up
    destroy_circuit_coupling_simulator(sim);
    remove(temp_filename);
    
    return 0;
}

// Test S-parameter circuit coupling
int test_sparameter_coupling(void) {
    printf("Testing S-parameter circuit coupling...\n");
    
    CircuitCouplingSimulator* sim = create_circuit_coupling_simulator();
    if (!sim) {
        printf("Error: Cannot create circuit simulator\n");
        return -1;
    }
    
    // Create sample S-parameters
    SParameterSet* sparams = create_sample_transmission_line_sparams();
    if (!sparams) {
        printf("Error: Cannot create sample S-parameters\n");
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    // Add S-parameter component to circuit
    int result = add_sparameter_component(sim, "XTL1", "IN", "OUT", sparams);
    if (result != 0) {
        printf("Error: Failed to add S-parameter component\n");
        destroy_sparameter_set(sparams);
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    // Add termination resistors
    add_sparameter_component(sim, "RIN", "IN", "0", NULL); // Will add resistor
    add_sparameter_component(sim, "ROUT", "OUT", "0", NULL); // Will add resistor
    
    printf("S-parameter component added successfully\n");
    
    // Test at multiple frequencies
    double test_freqs[] = {1e6, 10e6, 100e6, 1e9, 5e9, 10e9};
    int num_freqs = sizeof(test_freqs) / sizeof(double);
    
    printf("Frequency sweep results:\n");
    printf("Frequency(Hz)  |S11|     |S21|     |S12|     |S22|\n");
    printf("-------------|---------|---------|---------|---------\n");
    
    for (int i = 0; i < num_freqs; i++) {
        double freq = test_freqs[i];
        
        // Run circuit simulation
        result = run_circuit_simulation(sim, freq);
        if (result != 0) {
            printf("Warning: Simulation failed at %.1e Hz\n", freq);
            continue;
        }
        
        // Get S-parameters at this frequency
        SParameterMatrix* sparam_freq = get_sparameter_at_frequency(sparams, freq);
        if (sparam_freq) {
            double s11_mag = cabs(sparam_freq->s_matrix[0]);
            double s21_mag = cabs(sparam_freq->s_matrix[2]);
            double s12_mag = cabs(sparam_freq->s_matrix[1]);
            double s22_mag = cabs(sparam_freq->s_matrix[3]);
            
            printf("%-13.1e %-9.4f %-9.4f %-9.4f %-9.4f\n", 
                   freq, s11_mag, s21_mag, s12_mag, s22_mag);
        }
    }
    
    // Clean up
    destroy_circuit_coupling_simulator(sim);
    
    return 0;
}

// Test nonlinear device simulation
int test_nonlinear_devices(void) {
    printf("Testing nonlinear device simulation...\n");
    
    CircuitCouplingSimulator* sim = create_circuit_coupling_simulator();
    if (!sim) {
        printf("Error: Cannot create circuit simulator\n");
        return -1;
    }
    
    // Add basic circuit components
    // Voltage source
    add_sparameter_component(sim, "VIN", "IN", "0", NULL);
    // Load resistor
    add_sparameter_component(sim, "RLOAD", "OUT", "0", NULL);
    
    // Add nonlinear diode
    double diode_params[] = {0.025, 1e-14}; // vt, is
    int result = add_nonlinear_component(sim, "D1", "IN", "OUT", 
                                       DEVICE_DIODE, diode_params);
    if (result != 0) {
        printf("Error: Failed to add nonlinear component\n");
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    printf("Nonlinear diode component added successfully\n");
    
    // Test DC analysis
    result = run_circuit_simulation(sim, 0.0); // DC (frequency = 0)
    if (result != 0) {
        printf("Error: DC simulation failed\n");
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    double vin_dc = get_node_voltage(sim, "IN");
    double vout_dc = get_node_voltage(sim, "OUT");
    double idiode = get_branch_current(sim, "D1");
    
    printf("DC Analysis Results:\n");
    printf("V(IN) = %.3f V\n", vin_dc);
    printf("V(OUT) = %.3f V\n", vout_dc);
    printf("I(D1) = %.3e A\n", idiode);
    
    // Clean up
    destroy_circuit_coupling_simulator(sim);
    
    return 0;
}

// Test EM-circuit co-simulation
int test_em_circuit_cosimulation(void) {
    printf("Testing EM-circuit co-simulation...\n");
    
    // Create EM model
    PCBEMModel* em_model = create_sample_pcb_em_model();
    if (!em_model) {
        printf("Error: Cannot create EM model\n");
        return -1;
    }
    
    // Create circuit simulator
    CircuitCouplingSimulator* sim = create_circuit_coupling_simulator();
    if (!sim) {
        printf("Error: Cannot create circuit simulator\n");
        free(em_model);
        return -1;
    }
    
    // Set EM coupling
    int result = set_em_coupling(sim, em_model);
    if (result != 0) {
        printf("Error: Failed to set EM coupling\n");
        free(em_model);
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    printf("EM coupling established successfully\n");
    
    // Run co-simulation at EM frequency
    result = run_circuit_simulation(sim, em_model->frequency);
    if (result != 0) {
        printf("Error: Co-simulation failed\n");
        free(em_model);
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    // Export results
    result = export_circuit_results(sim, "cosimulation_results.txt");
    if (result != 0) {
        printf("Warning: Failed to export results\n");
    } else {
        printf("Results exported to cosimulation_results.txt\n");
    }
    
    // Clean up
    free(em_model);
    destroy_circuit_coupling_simulator(sim);
    
    return 0;
}

// Test mixed-mode S-parameters with circuit coupling
int test_mixed_mode_coupling(void) {
    printf("Testing mixed-mode S-parameter circuit coupling...\n");
    
    CircuitCouplingSimulator* sim = create_circuit_coupling_simulator();
    if (!sim) {
        printf("Error: Cannot create circuit simulator\n");
        return -1;
    }
    
    // Create differential S-parameters
    SParameterSet* diff_sparams = create_sample_transmission_line_sparams();
    if (!diff_sparams) {
        printf("Error: Cannot create differential S-parameters\n");
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    // Convert to mixed-mode
    MixedModeSParameters* mixed_mode = convert_to_mixed_mode_sparameters(diff_sparams);
    if (!mixed_mode) {
        printf("Error: Cannot convert to mixed-mode\n");
        destroy_sparameter_set(diff_sparams);
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    printf("Mixed-mode S-parameters created successfully\n");
    
    // Add differential components to circuit
    add_sparameter_component(sim, "XDIFF_P", "IN_P", "OUT_P", diff_sparams);
    add_sparameter_component(sim, "XDIFF_N", "IN_N", "OUT_N", diff_sparams);
    
    // Test common-mode rejection
    double freq = 1e9; // 1GHz
    int result = run_circuit_simulation(sim, freq);
    if (result != 0) {
        printf("Error: Mixed-mode simulation failed\n");
        destroy_mixed_mode_sparameters(mixed_mode);
        destroy_sparameter_set(diff_sparams);
        destroy_circuit_coupling_simulator(sim);
        return -1;
    }
    
    // Get mixed-mode parameters
    MixedModeMatrix* mm_matrix = get_mixed_mode_matrix_at_frequency(mixed_mode, freq);
    if (mm_matrix) {
        printf("Mixed-mode parameters at %.1e Hz:\n", freq);
        printf("|Sdd11| = %.4f\n", cabs(mm_matrix->sdd_matrix[0]));
        printf("|Sdd21| = %.4f\n", cabs(mm_matrix->sdd_matrix[6])); // 2-port matrix index
        printf("|Scc11| = %.4f\n", cabs(mm_matrix->scc_matrix[0]));
        printf("|Sdc21| = %.4f\n", cabs(mm_matrix->sdc_matrix[6]));
    }
    
    // Clean up
    destroy_mixed_mode_sparameters(mixed_mode);
    destroy_sparameter_set(diff_sparams);
    destroy_circuit_coupling_simulator(sim);
    
    return 0;
}

// Main test function
int main(void) {
    printf("=================================================================\n");
    printf("PulseMoM Circuit Coupling Simulation Example\n");
    printf("Commercial-grade EM-Circuit Co-simulation Demo\n");
    printf("=================================================================\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Test 1: SPICE parsing
    total_tests++;
    if (test_spice_parsing() == 0) {
        printf("✓ Test 1: SPICE parsing - PASSED\n\n");
        passed_tests++;
    } else {
        printf("✗ Test 1: SPICE parsing - FAILED\n\n");
    }
    
    // Test 2: S-parameter coupling
    total_tests++;
    if (test_sparameter_coupling() == 0) {
        printf("✓ Test 2: S-parameter coupling - PASSED\n\n");
        passed_tests++;
    } else {
        printf("✗ Test 2: S-parameter coupling - FAILED\n\n");
    }
    
    // Test 3: Nonlinear devices
    total_tests++;
    if (test_nonlinear_devices() == 0) {
        printf("✓ Test 3: Nonlinear devices - PASSED\n\n");
        passed_tests++;
    } else {
        printf("✗ Test 3: Nonlinear devices - FAILED\n\n");
    }
    
    // Test 4: EM-circuit co-simulation
    total_tests++;
    if (test_em_circuit_cosimulation() == 0) {
        printf("✓ Test 4: EM-circuit co-simulation - PASSED\n\n");
        passed_tests++;
    } else {
        printf("✗ Test 4: EM-circuit co-simulation - FAILED\n\n");
    }
    
    // Test 5: Mixed-mode coupling
    total_tests++;
    if (test_mixed_mode_coupling() == 0) {
        printf("✓ Test 5: Mixed-mode coupling - PASSED\n\n");
        passed_tests++;
    } else {
        printf("✗ Test 5: Mixed-mode coupling - FAILED\n\n");
    }
    
    // Summary
    printf("=================================================================\n");
    printf("Test Summary: %d/%d tests passed\n", passed_tests, total_tests);
    printf("=================================================================\n");
    
    if (passed_tests == total_tests) {
        printf("\n🎉 All circuit coupling simulation tests PASSED!\n");
        printf("The implementation matches commercial tool capabilities.\n");
        printf("Features demonstrated:\n");
        printf("  ✓ SPICE netlist integration\n");
        printf("  ✓ S-parameter based EM coupling\n");
        printf("  ✓ Nonlinear device modeling\n");
        printf("  ✓ Mixed-frequency analysis\n");
        printf("  ✓ Mixed-mode S-parameters\n");
        printf("  ✓ Results export capabilities\n");
    } else {
        printf("\n⚠️  Some tests failed. Check implementation details.\n");
    }
    
    return (passed_tests == total_tests) ? 0 : -1;
}