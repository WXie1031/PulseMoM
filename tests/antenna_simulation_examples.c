/*********************************************************************
 * Antenna Simulation Integration Examples
 * 
 * This file demonstrates comprehensive usage of the antenna simulation
 * module for full-wave electromagnetic field calculations.
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include "../src/antenna/antenna_fullwave.h"
#include "../src/antenna/metamaterial_extraction.h"
#include "../src/cad/cad_mesh_generation.h"
#include "../src/io/parallel_io.h"
#include "../src/io/memory_optimization.h"

/*********************************************************************
 * Example 1: Basic Antenna Analysis - Dipole Antenna
 *********************************************************************/
int example_dipole_antenna_analysis() {
    printf("=== Example 1: Dipole Antenna Analysis ===\n");
    
    // Create antenna structure
    AntennaStructure* antenna = antenna_structure_create();
    if (!antenna) {
        printf("Failed to create antenna structure\n");
        return -1;
    }
    
    // Define dipole parameters
    DipoleParameters dipole_params = {
        .length = 0.15,          // 15 cm length
        .radius = 0.001,         // 1 mm radius
        .feed_point = 0.075,     // Feed at center
        .material = {
            .name = "copper",
            .conductivity = 5.8e7,
            .permittivity = 1.0,
            .permeability = 1.0
        }
    };
    
    // Create dipole geometry
    antenna_dipole_create(antenna, &dipole_params);
    
    // Define simulation parameters
    AntennaSimulationParams sim_params = {
        .frequency_start = 100.0e6,    // 100 MHz
        .frequency_stop = 1000.0e6,    // 1 GHz
        .frequency_points = 91,         // 91 frequency points
        .mesh_density = 20,             // 20 segments per wavelength
        .ground_plane = GROUND_NONE,
        .solver_type = SOLVER_MOM,
        .convergence_tolerance = 1e-6,
        .max_iterations = 1000
    };
    
    // Perform full-wave analysis
    printf("Performing full-wave analysis...\n");
    clock_t start = clock();
    
    AntennaResults* results = antenna_fullwave_analyze(antenna, &sim_params);
    
    clock_t end = clock();
    double analysis_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (results) {
        printf("Analysis completed in %.2f seconds\n", analysis_time);
        printf("Results:\n");
        
        // Display impedance vs frequency
        printf("Frequency (MHz) | Impedance (Ohm) | VSWR\n");
        printf("----------------|------------------|------\n");
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double freq_mhz = results->frequencies[i] / 1e6;
            double complex z = results->impedance[i];
            double z_mag = cabs(z);
            double z_real = creal(z);
            double z_imag = cimag(z);
            
            // Calculate VSWR
            double gamma = cabs((z - 50.0) / (z + 50.0));
            double vswr = (1.0 + gamma) / (1.0 - gamma);
            
            printf("%15.1f | %8.1f + j%7.1f | %5.2f\n", 
                   freq_mhz, z_real, z_imag, vswr);
        }
        
        // Find resonance frequency
        double min_vswr = 1e6;
        int resonance_idx = -1;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex z = results->impedance[i];
            double gamma = cabs((z - 50.0) / (z + 50.0));
            double vswr = (1.0 + gamma) / (1.0 - gamma);
            
            if (vswr < min_vswr) {
                min_vswr = vswr;
                resonance_idx = i;
            }
        }
        
        if (resonance_idx >= 0) {
            printf("\nResonance found at %.1f MHz (VSWR = %.2f)\n",
                   results->frequencies[resonance_idx] / 1e6, min_vswr);
        }
        
        // Calculate radiation patterns at resonance
        double resonance_freq = results->frequencies[resonance_idx];
        AntennaPattern* pattern = antenna_calculate_pattern(antenna, resonance_freq);
        
        if (pattern) {
            printf("\nRadiation Pattern at %.1f MHz:\n", resonance_freq / 1e6);
            printf("Gain: %.2f dBi\n", pattern->max_gain);
            printf("Beamwidth: %.1f degrees\n", pattern->beamwidth);
            printf("Front-to-back ratio: %.2f dB\n", pattern->front_to_back_ratio);
            
            antenna_pattern_destroy(pattern);
        }
        
        antenna_results_destroy(results);
    }
    
    antenna_structure_destroy(antenna);
    return 0;
}

/*********************************************************************
 * Example 2: Patch Antenna with Substrate
 *********************************************************************/
int example_patch_antenna_with_substrate() {
    printf("\n=== Example 2: Patch Antenna with Substrate ===\n");
    
    // Create patch antenna structure
    AntennaStructure* antenna = antenna_structure_create();
    
    // Define substrate properties
    SubstrateParameters substrate = {
        .thickness = 1.6e-3,     // 1.6 mm FR4 substrate
        .epsilon_r = 4.4,
        .mu_r = 1.0,
        .loss_tangent = 0.02,
        .conductivity = 0.0
    };
    
    // Define patch parameters
    PatchParameters patch_params = {
        .width = 0.038,          // 38 mm width
        .length = 0.028,         // 28 mm length
        .thickness = 35e-6,      // 35 μm copper
        .substrate = substrate,
        .feed_type = FEED_MICROSTRIP,
        .feed_position = 0.009,  // 9 mm from edge
        .ground_plane_size = 0.1  // 10 cm ground plane
    };
    
    // Create patch antenna
    antenna_patch_create(antenna, &patch_params);
    
    // Define simulation parameters for patch antenna
    AntennaSimulationParams sim_params = {
        .frequency_start = 2000.0e6,   // 2 GHz
        .frequency_stop = 3000.0e6,    // 3 GHz
        .frequency_points = 101,
        .mesh_density = 30,            // Fine mesh for accuracy
        .substrate_modeling = 1,       // Include substrate effects
        .solver_type = SOLVER_MOM,
        .convergence_tolerance = 1e-8,
        .max_iterations = 2000
    };
    
    // Perform analysis
    printf("Analyzing patch antenna...\n");
    clock_t start = clock();
    
    AntennaResults* results = antenna_fullwave_analyze(antenna, &sim_params);
    
    clock_t end = clock();
    double analysis_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (results) {
        printf("Analysis completed in %.2f seconds\n", analysis_time);
        
        // Find resonant frequency and bandwidth
        double min_vswr = 1e6;
        int resonance_idx = -1;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex z = results->impedance[i];
            double gamma = cabs((z - 50.0) / (z + 50.0));
            double vswr = (1.0 + gamma) / (1.0 - gamma);
            
            if (vswr < min_vswr) {
                min_vswr = vswr;
                resonance_idx = i;
            }
        }
        
        // Calculate bandwidth (VSWR < 2.0)
        double bandwidth_start = 0, bandwidth_stop = 0;
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex z = results->impedance[i];
            double gamma = cabs((z - 50.0) / (z + 50.0));
            double vswr = (1.0 + gamma) / (1.0 - gamma);
            
            if (vswr < 2.0) {
                if (bandwidth_start == 0) bandwidth_start = results->frequencies[i];
                bandwidth_stop = results->frequencies[i];
            }
        }
        
        double bandwidth = bandwidth_stop - bandwidth_start;
        double center_freq = (bandwidth_start + bandwidth_stop) / 2.0;
        double fractional_bw = bandwidth / center_freq * 100.0;
        
        printf("\nPatch Antenna Results:\n");
        printf("Resonant Frequency: %.1f MHz\n", 
               results->frequencies[resonance_idx] / 1e6);
        printf("Minimum VSWR: %.2f\n", min_vswr);
        printf("Bandwidth (VSWR<2): %.1f MHz (%.1f%%)\n", 
               bandwidth / 1e6, fractional_bw);
        
        // Calculate radiation efficiency
        double efficiency = antenna_calculate_efficiency(antenna, 
                                                         results->frequencies[resonance_idx]);
        printf("Radiation Efficiency: %.1f%%\n", efficiency * 100.0);
        
        // 3D radiation pattern
        AntennaPattern3D* pattern_3d = antenna_calculate_pattern_3d(antenna, 
                                                                     results->frequencies[resonance_idx]);
        if (pattern_3d) {
            printf("\n3D Radiation Pattern:\n");
            printf("Peak Gain: %.2f dBi\n", pattern_3d->peak_gain);
            printf("Average Gain: %.2f dBi\n", pattern_3d->average_gain);
            printf("Directivity: %.2f dBi\n", pattern_3d->directivity);
            
            antenna_pattern_3d_destroy(pattern_3d);
        }
        
        antenna_results_destroy(results);
    }
    
    antenna_structure_destroy(antenna);
    return 0;
}

/*********************************************************************
 * Example 3: Antenna Array Analysis
 *********************************************************************/
int example_antenna_array_analysis() {
    printf("\n=== Example 3: Antenna Array Analysis ===\n");
    
    // Create antenna array structure
    AntennaArray* array = antenna_array_create();
    
    // Define array parameters
    ArrayParameters array_params = {
        .element_type = ELEMENT_PATCH,
        .num_elements_x = 4,
        .num_elements_y = 4,
        .spacing_x = 0.05,       // 5 cm spacing
        .spacing_y = 0.05,
        .frequency = 2500.0e6,    // 2.5 GHz
        .scan_angle_theta = 0.0,  // Broadside
        .scan_angle_phi = 0.0,
        .amplitude_taper = TAPER_CHEBYSHEV,
        .side_lobe_level = -20.0  // -20 dB side lobes
    };
    
    // Create individual patch elements
    PatchParameters element_params = {
        .width = 0.036,
        .length = 0.028,
        .thickness = 35e-6,
        .substrate = {
            .thickness = 1.6e-3,
            .epsilon_r = 4.4,
            .loss_tangent = 0.02
        }
    };
    
    // Build array
    antenna_array_build(array, &array_params, &element_params);
    
    // Define simulation parameters
    AntennaSimulationParams sim_params = {
        .frequency_start = 2000.0e6,
        .frequency_stop = 3000.0e6,
        .frequency_points = 51,
        .mesh_density = 25,
        .include_mutual_coupling = 1,  // Important for arrays
        .solver_type = SOLVER_MOM,
        .convergence_tolerance = 1e-7
    };
    
    // Perform array analysis
    printf("Analyzing 4x4 antenna array...\n");
    clock_t start = clock();
    
    AntennaArrayResults* results = antenna_array_analyze(array, &sim_params);
    
    clock_t end = clock();
    double analysis_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (results) {
        printf("Array analysis completed in %.2f seconds\n", analysis_time);
        
        // Display array characteristics
        printf("\nArray Characteristics:\n");
        printf("Number of elements: %d\n", results->num_elements);
        printf("Array gain: %.2f dBi\n", results->array_gain);
        printf("Beamwidth: %.1f x %.1f degrees\n", 
               results->beamwidth_elevation, results->beamwidth_azimuth);
        printf("Side lobe level: %.1f dB\n", results->side_lobe_level);
        
        // Calculate array factor and radiation patterns
        ArrayPattern* array_pattern = antenna_array_calculate_pattern(array, 2500.0e6);
        
        if (array_pattern) {
            printf("\nArray Pattern Analysis:\n");
            printf("Peak directivity: %.2f dBi\n", array_pattern->directivity);
            printf("Beam steering range: ±%.1f degrees\n", array_pattern->scan_range);
            printf("Grating lobes: %s\n", array_pattern->has_grating_lobes ? "YES" : "NO");
            
            // Show scan performance
            printf("\nScan Performance:\n");
            for (int angle = 0; angle <= 60; angle += 15) {
                ScanPerformance scan = antenna_array_calculate_scan_performance(array, angle, 0.0);
                printf("  Scan angle %2d°: Gain = %5.2f dBi, SLL = %5.1f dB\n",
                       angle, scan.gain, scan.side_lobe_level);
            }
            
            antenna_array_pattern_destroy(array_pattern);
        }
        
        // Calculate mutual coupling matrix
        MutualCouplingMatrix* coupling = antenna_array_calculate_coupling(array);
        if (coupling) {
            printf("\nMutual Coupling Analysis:\n");
            printf("Maximum coupling: %.1f dB\n", coupling->max_coupling);
            printf("Average coupling: %.1f dB\n", coupling->avg_coupling);
            
            // Show coupling between adjacent elements
            printf("Coupling between adjacent elements:\n");
            for (int i = 0; i < array_params.num_elements_x - 1; i++) {
                for (int j = 0; j < array_params.num_elements_y; j++) {
                    int elem1 = j * array_params.num_elements_x + i;
                    int elem2 = j * array_params.num_elements_x + i + 1;
                    double coupling_db = coupling->magnitude[elem1][elem2];
                    printf("  Element (%d,%d) to (%d,%d): %.1f dB\n",
                           i, j, i+1, j, coupling_db);
                }
            }
            
            mutual_coupling_matrix_destroy(coupling);
        }
        
        antenna_array_results_destroy(results);
    }
    
    antenna_array_destroy(array);
    return 0;
}

/*********************************************************************
 * Example 4: Wideband Antenna with Optimization
 *********************************************************************/
int example_wideband_antenna_optimization() {
    printf("\n=== Example 4: Wideband Antenna with Optimization ===\n");
    
    // Create wideband antenna structure (spiral antenna)
    AntennaStructure* antenna = antenna_structure_create();
    
    // Define spiral antenna parameters
    SpiralParameters spiral_params = {
        .inner_radius = 0.001,    // 1 mm inner radius
        .outer_radius = 0.05,     // 5 cm outer radius
        .num_turns = 5.0,
        .growth_rate = 1.1,
        .wire_diameter = 1e-3,    // 1 mm wire diameter
        .material = {
            .name = "copper",
            .conductivity = 5.8e7
        }
    };
    
    // Create spiral antenna
    antenna_spiral_create(antenna, &spiral_params);
    
    // Define wide frequency range
    double freq_start = 100.0e6;   // 100 MHz
    double freq_stop = 10000.0e6;  // 10 GHz
    int freq_points = 201;
    
    // Define optimization goals
    OptimizationGoals goals = {
        .target_vswr = 2.0,
        .min_bandwidth = 0.9,      // 90% bandwidth
        .min_gain = 0.0,           // 0 dBi minimum gain
        .max_size = 0.1,           // 10 cm maximum size
        .weight_vswr = 0.4,
        .weight_bandwidth = 0.3,
        .weight_gain = 0.2,
        .weight_size = 0.1
    };
    
    // Initial analysis
    printf("Performing initial wideband analysis...\n");
    AntennaSimulationParams sim_params = {
        .frequency_start = freq_start,
        .frequency_stop = freq_stop,
        .frequency_points = freq_points,
        .mesh_density = 40,        // High density for accuracy
        .solver_type = SOLVER_MOM,
        .convergence_tolerance = 1e-8,
        .adaptive_frequency = 1    // Adaptive frequency sampling
    };
    
    clock_t start = clock();
    AntennaResults* results = antenna_fullwave_analyze(antenna, &sim_params);
    clock_t end = clock();
    double analysis_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Initial analysis completed in %.2f seconds\n", analysis_time);
    
    if (results) {
        // Analyze VSWR performance across band
        printf("\nVSWR Performance Analysis:\n");
        printf("Frequency Range | Max VSWR | Avg VSWR | Bandwidth\n");
        printf("----------------|----------|----------|----------\n");
        
        // Analyze different frequency bands
        double bands[][2] = {
            {100e6, 500e6},    // VHF
            {500e6, 1000e6},   // UHF
            {1000e6, 3000e6},  // L-band
            {3000e6, 10000e6}  // S-band
        };
        const char* band_names[] = {"VHF", "UHF", "L-band", "S-band"};
        
        for (int b = 0; b < 4; b++) {
            double max_vswr = 0;
            double avg_vswr = 0;
            int count = 0;
            double bandwidth = 0;
            
            for (int i = 0; i < results->num_frequencies; i++) {
                double freq = results->frequencies[i];
                if (freq >= bands[b][0] && freq <= bands[b][1]) {
                    double complex z = results->impedance[i];
                    double gamma = cabs((z - 50.0) / (z + 50.0));
                    double vswr = (1.0 + gamma) / (1.0 - gamma);
                    
                    if (vswr > max_vswr) max_vswr = vswr;
                    avg_vswr += vswr;
                    count++;
                    
                    if (vswr < 2.0) bandwidth = freq;
                }
            }
            
            if (count > 0) {
                avg_vswr /= count;
                printf("%-14s | %8.2f | %8.2f | %7.1f%%\n",
                       band_names[b], max_vswr, avg_vswr, 
                       (bandwidth - bands[b][0]) / (bands[b][1] - bands[b][0]) * 100);
            }
        }
        
        // Calculate realized gain (considering mismatch)
        printf("\nRealized Gain Analysis:\n");
        double max_realized_gain = -1e6;
        double freq_max_gain = 0;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double freq = results->frequencies[i];
            double complex z = results->impedance[i];
            double gamma = cabs((z - 50.0) / (z + 50.0));
            
            // Calculate realized gain = gain * (1 - |gamma|^2)
            double realized_gain = results->gain[i] * (1.0 - gamma * gamma);
            
            if (realized_gain > max_realized_gain) {
                max_realized_gain = realized_gain;
                freq_max_gain = freq;
            }
        }
        
        printf("Maximum realized gain: %.2f dBi at %.1f MHz\n",
               max_realized_gain, freq_max_gain / 1e6);
        
        // Optimization suggestions
        printf("\nOptimization Suggestions:\n");
        
        // Find frequency ranges with good VSWR
        printf("Frequency ranges with VSWR < 2.0:\n");
        int in_good_range = 0;
        double good_start = 0;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex z = results->impedance[i];
            double gamma = cabs((z - 50.0) / (z + 50.0));
            double vswr = (1.0 + gamma) / (1.0 - gamma);
            
            if (vswr < 2.0 && !in_good_range) {
                good_start = results->frequencies[i];
                in_good_range = 1;
            } else if (vswr >= 2.0 && in_good_range) {
                printf("  %.0f - %.0f MHz\n", good_start / 1e6, results->frequencies[i-1] / 1e6);
                in_good_range = 0;
            }
        }
        
        if (in_good_range) {
            printf("  %.0f - %.0f MHz\n", good_start / 1e6, 
                   results->frequencies[results->num_frequencies-1] / 1e6);
        }
        
        antenna_results_destroy(results);
    }
    
    antenna_structure_destroy(antenna);
    return 0;
}

/*********************************************************************
 * Example 5: Antenna with Electromagnetic Bandgap (EBG) Structure
 *********************************************************************/
int example_antenna_with_ebg() {
    printf("\n=== Example 5: Antenna with EBG Structure ===\n");
    
    // Create antenna structure
    AntennaStructure* antenna = antenna_structure_create();
    
    // Create patch antenna first
    PatchParameters patch_params = {
        .width = 0.036,
        .length = 0.028,
        .thickness = 35e-6,
        .substrate = {
            .thickness = 1.6e-3,
            .epsilon_r = 4.4,
            .loss_tangent = 0.02
        }
    };
    
    antenna_patch_create(antenna, &patch_params);
    
    // Create EBG structure around antenna
    EBGParameters ebg_params = {
        .type = EBG_MUSHROOM,
        .patch_width = 0.01,       // 10 mm EBG patches
        .gap_width = 0.001,        // 1 mm gap
        .via_radius = 0.0005,     // 0.5 mm via radius
        .substrate = patch_params.substrate,
        .num_periods_x = 5,        // 5 periods in x
        .num_periods_y = 5,        // 5 periods in y
        .period_x = 0.011,         // 11 mm period
        .period_y = 0.011,         // 11 mm period
        .center_frequency = 2500.0e6  // 2.5 GHz center frequency
    };
    
    // Add EBG to antenna structure
    antenna_add_ebg_structure(antenna, &ebg_params);
    
    // Define simulation parameters
    AntennaSimulationParams sim_params = {
        .frequency_start = 1500.0e6,
        .frequency_stop = 3500.0e6,
        .frequency_points = 81,
        .mesh_density = 35,        // Fine mesh for EBG
        .include_ebg_effects = 1,  // Include EBG in simulation
        .solver_type = SOLVER_MOM,
        .convergence_tolerance = 1e-8,
        .periodic_boundary_conditions = 1  // Use periodic BC for EBG
    };
    
    // Perform analysis with and without EBG
    printf("Analyzing antenna without EBG...\n");
    
    // Temporarily disable EBG
    antenna_disable_ebg(antenna);
    AntennaResults* results_without_ebg = antenna_fullwave_analyze(antenna, &sim_params);
    
    printf("Analyzing antenna with EBG...\n");
    
    // Enable EBG
    antenna_enable_ebg(antenna);
    AntennaResults* results_with_ebg = antenna_fullwave_analyze(antenna, &sim_params);
    
    // Compare results
    if (results_without_ebg && results_with_ebg) {
        printf("\nEBG Performance Comparison:\n");
        printf("Frequency | Without EBG | With EBG | Improvement\n");
        printf("----------|-------------|----------|------------\n");
        
        for (int i = 0; i < results_without_ebg->num_frequencies; i += 10) {
            double freq = results_without_ebg->frequencies[i] / 1e6;
            
            // Without EBG
            double complex z1 = results_without_ebg->impedance[i];
            double gamma1 = cabs((z1 - 50.0) / (z1 + 50.0));
            double vswr1 = (1.0 + gamma1) / (1.0 - gamma1);
            
            // With EBG
            double complex z2 = results_with_ebg->impedance[i];
            double gamma2 = cabs((z2 - 50.0) / (z2 + 50.0));
            double vswr2 = (1.0 + gamma2) / (1.0 - gamma2);
            
            double improvement = (vswr1 - vswr2) / vswr1 * 100.0;
            
            printf("%9.0f | %11.2f | %8.2f | %9.1f%%\n",
                   freq, vswr1, vswr2, improvement);
        }
        
        // Calculate surface wave suppression
        printf("\nSurface Wave Analysis:\n");
        
        // Analyze surface wave propagation along substrate
        for (int freq_idx = 0; freq_idx < results_with_ebg->num_frequencies; freq_idx += 20) {
            double freq = results_with_ebg->frequencies[freq_idx];
            
            SurfaceWaveAnalysis* sw_analysis = antenna_analyze_surface_waves(antenna, freq);
            
            if (sw_analysis) {
                printf("%.0f MHz: Surface wave attenuation = %.1f dB/cm, ",
                       freq / 1e6, sw_analysis->attenuation_per_cm);
                printf("Bandgap width = %.1f%%\n", sw_analysis->bandgap_width * 100);
                
                surface_wave_analysis_destroy(sw_analysis);
            }
        }
        
        // Radiation pattern comparison at center frequency
        double center_freq = 2500.0e6;
        
        AntennaPattern* pattern_without = antenna_calculate_pattern(antenna, center_freq);
        
        printf("\nRadiation Pattern Comparison at %.1f MHz:\n", center_freq / 1e6);
        if (pattern_without) {
            printf("Without EBG - Gain: %.2f dBi, Efficiency: %.1f%%\n",
                   pattern_without->max_gain, pattern_without->efficiency * 100);
        }
        
        antenna_results_destroy(results_without_ebg);
        antenna_results_destroy(results_with_ebg);
    }
    
    antenna_structure_destroy(antenna);
    return 0;
}

/*********************************************************************
 * Example 6: Parallel Antenna Simulation with Optimization
 *********************************************************************/
int example_parallel_antenna_simulation() {
    printf("\n=== Example 6: Parallel Antenna Simulation ===\n");
    
    // Initialize parallel processing
    int num_threads = 8;
    omp_set_num_threads(num_threads);
    
    // Create multiple antenna configurations for parallel analysis
    int num_configs = 12;
    AntennaConfig* configs = malloc(num_configs * sizeof(AntennaConfig));
    
    // Generate different patch antenna configurations
    for (int i = 0; i < num_configs; i++) {
        configs[i].type = ANTENNA_PATCH;
        configs[i].frequency = 2000.0e6 + i * 200.0e6;  // 2-4.4 GHz
        configs[i].params.patch.width = 0.03 + i * 0.002;   // Vary width
        configs[i].params.patch.length = 0.025 + i * 0.001; // Vary length
        configs[i].params.patch.substrate.epsilon_r = 3.5 + i * 0.1;
        configs[i].params.patch.substrate.thickness = 1.0e-3 + i * 0.1e-3;
        
        sprintf(configs[i].name, "Patch_%d", i + 1);
    }
    
    printf("Analyzing %d antenna configurations in parallel...\n", num_configs);
    
    // Parallel analysis
    clock_t start = clock();
    
    #pragma omp parallel for
    for (int i = 0; i < num_configs; i++) {
        int thread_id = omp_get_thread_num();
        
        printf("[Thread %d] Analyzing %s...\n", thread_id, configs[i].name);
        
        // Create antenna structure
        AntennaStructure* antenna = antenna_structure_create();
        antenna_patch_create(antenna, &configs[i].params.patch);
        
        // Define simulation parameters
        AntennaSimulationParams sim_params = {
            .frequency_start = configs[i].frequency * 0.8,
            .frequency_stop = configs[i].frequency * 1.2,
            .frequency_points = 41,
            .mesh_density = 25,
            .solver_type = SOLVER_MOM,
            .convergence_tolerance = 1e-6,
            .max_iterations = 1000
        };
        
        // Perform analysis
        AntennaResults* results = antenna_fullwave_analyze(antenna, &sim_params);
        
        if (results) {
            // Find best performance
            double best_vswr = 1e6;
            int best_idx = -1;
            
            for (int j = 0; j < results->num_frequencies; j++) {
                double complex z = results->impedance[j];
                double gamma = cabs((z - 50.0) / (z + 50.0));
                double vswr = (1.0 + gamma) / (1.0 - gamma);
                
                if (vswr < best_vswr) {
                    best_vswr = vswr;
                    best_idx = j;
                }
            }
            
            // Store results
            configs[i].results.best_vswr = best_vswr;
            configs[i].results.best_frequency = results->frequencies[best_idx];
            configs[i].results.max_gain = results->gain[best_idx];
            configs[i].results.analysis_time = omp_get_wtime() - start;
            
            printf("[Thread %d] %s: Best VSWR = %.2f at %.1f MHz\n",
                   thread_id, configs[i].name, best_vswr, 
                   results->frequencies[best_idx] / 1e6);
            
            antenna_results_destroy(results);
        }
        
        antenna_structure_destroy(antenna);
    }
    
    clock_t end = clock();
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("\nParallel analysis completed in %.2f seconds\n", total_time);
    printf("Average time per configuration: %.2f seconds\n", total_time / num_configs);
    
    // Find best overall configuration
    int best_config = 0;
    double best_overall_vswr = 1e6;
    
    for (int i = 0; i < num_configs; i++) {
        if (configs[i].results.best_vswr < best_overall_vswr) {
            best_overall_vswr = configs[i].results.best_vswr;
            best_config = i;
        }
    }
    
    printf("\nBest Configuration: %s\n", configs[best_config].name);
    printf("  Best VSWR: %.2f\n", configs[best_config].results.best_vswr);
    printf("  Best Frequency: %.1f MHz\n", configs[best_config].results.best_frequency / 1e6);
    printf("  Maximum Gain: %.2f dBi\n", configs[best_config].results.max_gain);
    
    // Performance summary
    printf("\nPerformance Summary:\n");
    printf("Configuration | Best VSWR | Freq (MHz) | Gain (dBi) | Time (s)\n");
    printf("--------------|-----------|------------|------------|----------\n");
    
    for (int i = 0; i < num_configs; i++) {
        printf("%-13s | %9.2f | %10.1f | %10.2f | %8.2f\n",
               configs[i].name,
               configs[i].results.best_vswr,
               configs[i].results.best_frequency / 1e6,
               configs[i].results.max_gain,
               configs[i].results.analysis_time);
    }
    
    free(configs);
    return 0;
}

/*********************************************************************
 * Main function to run all examples
 *********************************************************************/
int main() {
    printf("Antenna Simulation Integration Examples\n");
    printf("======================================\n\n");
    
    int result = 0;
    
    // Run all antenna simulation examples
    result |= example_dipole_antenna_analysis();
    result |= example_patch_antenna_with_substrate();
    result |= example_antenna_array_analysis();
    result |= example_wideband_antenna_optimization();
    result |= example_antenna_with_ebg();
    result |= example_parallel_antenna_simulation();
    
    if (result == 0) {
        printf("\nAll antenna simulation examples completed successfully!\n");
    } else {
        printf("\nSome antenna simulation examples failed with errors.\n");
    }
    
    return result;
}