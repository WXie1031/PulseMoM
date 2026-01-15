/*********************************************************************
 * Metamaterial Parameter Extraction Examples
 * 
 * This file demonstrates comprehensive usage of the metamaterial parameter
 * extraction module for electromagnetic property characterization.
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include "../src/antenna/metamaterial_extraction.h"
#include "../src/antenna/antenna_fullwave.h"
#include "../src/cad/cad_mesh_generation.h"
#include "../src/io/parallel_io.h"

/*********************************************************************
 * Example 1: Split-Ring Resonator (SRR) Parameter Extraction
 *********************************************************************/
int example_srr_parameter_extraction() {
    printf("=== Example 1: Split-Ring Resonator (SRR) Parameter Extraction ===\n");
    
    // Create SRR metamaterial structure
    MetamaterialStructure* metamaterial = metamaterial_structure_create();
    if (!metamaterial) {
        printf("Failed to create metamaterial structure\n");
        return -1;
    }
    
    // Define SRR parameters
    SRRParameters srr_params = {
        .outer_radius = 2.5e-3,      // 2.5 mm outer radius
        .inner_radius = 1.5e-3,      // 1.5 mm inner radius
        .gap_width = 0.3e-3,         // 0.3 mm gap
        .metal_width = 0.2e-3,       // 0.2 mm metal width
        .substrate_thickness = 0.8e-3, // 0.8 mm substrate
        .substrate_epsilon_r = 4.4,   // FR4 substrate
        .metal_conductivity = 5.8e7,  // Copper
        .unit_cell_size = 5.0e-3,   // 5 mm unit cell
        .num_rings = 2               // Double SRR
    };
    
    // Create SRR metamaterial
    metamaterial_srr_create(metamaterial, &srr_params);
    
    // Define frequency range for extraction
    FrequencyRange freq_range = {
        .start = 1.0e9,    // 1 GHz
        .stop = 20.0e9,    // 20 GHz
        .points = 191,     // 191 frequency points
        .log_scale = 1     // Logarithmic frequency spacing
    };
    
    // Define extraction parameters
    ExtractionParameters extract_params = {
        .method = METHOD_NRW,          // Nicolson-Ross-Weir method
        .frequency_range = freq_range,
        .incidence_angle = 0.0,        // Normal incidence
        .polarization = POL_TE,       // TE polarization
        .reference_impedance = 50.0,
        .extraction_points = 5,        // 5 extraction points per wavelength
        .smoothing_window = 3,         // 3-point smoothing
        .phase_unwrapping = 1,         // Enable phase unwrapping
        .passivity_enforcement = 1     // Enforce passivity
    };
    
    printf("Performing SRR parameter extraction...\n");
    clock_t start = clock();
    
    // Extract effective parameters
    MetamaterialResults* results = metamaterial_extract_parameters(metamaterial, &extract_params);
    
    clock_t end = clock();
    double extraction_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (results) {
        printf("Extraction completed in %.2f seconds\n", extraction_time);
        printf("\nSRR Metamaterial Results:\n");
        
        // Find resonant frequency (where mu crosses zero)
        double resonance_freq = 0;
        double max_mu_negative = 0;
        int resonance_idx = -1;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double freq = results->frequencies[i];
            double complex epsilon_eff = results->epsilon_eff[i];
            double complex mu_eff = results->mu_eff[i];
            
            double mu_real = creal(mu_eff);
            
            // Find where mu crosses from negative to positive
            if (i > 0 && creal(results->mu_eff[i-1]) < 0 && mu_real > 0) {
                resonance_freq = freq;
                resonance_idx = i;
            }
            
            // Track maximum negative permeability
            if (mu_real < 0 && fabs(mu_real) > max_mu_negative) {
                max_mu_negative = fabs(mu_real);
            }
        }
        
        printf("Resonant Frequency: %.2f GHz\n", resonance_freq / 1e9);
        printf("Maximum Negative Permeability: %.2f\n", max_mu_negative);
        
        // Display effective parameters at resonance
        if (resonance_idx >= 0) {
            double complex epsilon_res = results->epsilon_eff[resonance_idx];
            double complex mu_res = results->mu_eff[resonance_idx];
            double complex eta_res = results->wave_impedance[resonance_idx];
            double complex n_res = results->refractive_index[resonance_idx];
            
            printf("\nAt Resonance (%.2f GHz):\n", resonance_freq / 1e9);
            printf("  Effective Permittivity: %.2f + j%.3f\n", 
                   creal(epsilon_res), cimag(epsilon_res));
            printf("  Effective Permeability: %.2f + j%.3f\n", 
                   creal(mu_res), cimag(mu_res));
            printf("  Refractive Index: %.2f + j%.3f\n", 
                   creal(n_res), cimag(n_res));
            printf("  Wave Impedance: %.1f + j%.1f Ω\n", 
                   creal(eta_res), cimag(eta_res));
        }
        
        // Analyze magnetic response region
        printf("\nMagnetic Response Analysis:\n");
        printf("Frequency (GHz) | μ' | μ'' | FOM\n");
        printf("----------------|-----|------|-----\n");
        
        for (int i = 0; i < results->num_frequencies; i += 10) {
            double freq = results->frequencies[i] / 1e9;
            double complex mu_eff = results->mu_eff[i];
            double mu_real = creal(mu_eff);
            double mu_imag = cimag(mu_eff);
            
            // Figure of Merit (FOM) for magnetic metamaterials
            double fom = fabs(mu_real) / mu_imag;
            
            printf("%15.2f | %5.2f | %6.3f | %5.2f\n", 
                   freq, mu_real, mu_imag, fom);
        }
        
        // Extract equivalent circuit parameters
        EquivalentCircuit circuit = metamaterial_extract_circuit_parameters(results);
        
        printf("\nEquivalent Circuit Parameters:\n");
        printf("  Series Inductance (L): %.2f nH\n", circuit.series_inductance * 1e9);
        printf("  Series Capacitance (C): %.2f pF\n", circuit.series_capacitance * 1e12);
        printf("  Series Resistance (R): %.2f Ω\n", circuit.series_resistance);
        printf("  Shunt Inductance (Lp): %.2f nH\n", circuit.shunt_inductance * 1e9);
        printf("  Resonant Frequency: %.2f GHz\n", circuit.resonant_frequency / 1e9);
        printf("  Quality Factor (Q): %.2f\n", circuit.quality_factor);
        
        // Validate extraction using passivity and causality
        ValidationResult validation = metamaterial_validate_extraction(results);
        
        printf("\nExtraction Validation:\n");
        printf("  Passivity: %s (violation: %.2f%%)\n", 
               validation.passivity_check ? "PASS" : "FAIL", 
               validation.passivity_violation * 100);
        printf("  Causality: %s (violation: %.2f%%)\n", 
               validation.causality_check ? "PASS" : "FAIL",
               validation.causality_violation * 100);
        printf("  Kramers-Kronig: %s (error: %.2f%%)\n", 
               validation.kk_consistency ? "PASS" : "FAIL",
               validation.kk_error * 100);
        
        metamaterial_results_destroy(results);
    }
    
    metamaterial_structure_destroy(metamaterial);
    return 0;
}

/*********************************************************************
 * Example 2: Complementary Split-Ring Resonator (CSRR) Analysis
 *********************************************************************/
int example_csrr_analysis() {
    printf("\n=== Example 2: Complementary Split-Ring Resonator (CSRR) Analysis ===\n");
    
    // Create CSRR metamaterial structure
    MetamaterialStructure* metamaterial = metamaterial_structure_create();
    
    // Define CSRR parameters (etched in ground plane)
    CSRRParameters csrr_params = {
        .outer_radius = 3.0e-3,      // 3 mm outer radius
        .inner_radius = 2.0e-3,      // 2 mm inner radius
        .gap_width = 0.4e-3,         // 0.4 mm gap
        .slot_width = 0.3e-3,        // 0.3 mm slot width
        .substrate_thickness = 1.6e-3, // 1.6 mm substrate
        .substrate_epsilon_r = 4.4,   // FR4
        .metal_thickness = 35e-6,     // 35 μm copper
        .unit_cell_size = 7.0e-3,   // 7 mm unit cell
        .num_rings = 2               // Double CSRR
    };
    
    // Create CSRR metamaterial
    metamaterial_csrr_create(metamaterial, &csrr_params);
    
    // Define extraction parameters for CSRR
    FrequencyRange freq_range = {
        .start = 500.0e6,    // 500 MHz
        .stop = 15.0e9,     // 15 GHz
        .points = 146,
        .log_scale = 1
    };
    
    ExtractionParameters extract_params = {
        .method = METHOD_S_PARAMETER,  // S-parameter method for CSRR
        .frequency_range = freq_range,
        .incidence_angle = 0.0,
        .polarization = POL_TM,         // TM polarization for CSRR
        .reference_impedance = 50.0,
        .extraction_points = 7,
        .smoothing_window = 5,
        .phase_unwrapping = 1,
        .passivity_enforcement = 1
    };
    
    printf("Performing CSRR parameter extraction...\n");
    clock_t start = clock();
    
    MetamaterialResults* results = metamaterial_extract_parameters(metamaterial, &extract_params);
    
    clock_t end = clock();
    double extraction_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (results) {
        printf("CSRR extraction completed in %.2f seconds\n", extraction_time);
        
        // Find electric resonance (where epsilon crosses zero)
        double electric_resonance = 0;
        double max_epsilon_negative = 0;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double freq = results->frequencies[i];
            double complex epsilon_eff = results->epsilon_eff[i];
            double epsilon_real = creal(epsilon_eff);
            
            // Find electric resonance
            if (i > 0 && creal(results->epsilon_eff[i-1]) < 0 && epsilon_real > 0) {
                electric_resonance = freq;
            }
            
            // Track maximum negative permittivity
            if (epsilon_real < 0 && fabs(epsilon_real) > max_epsilon_negative) {
                max_epsilon_negative = fabs(epsilon_real);
            }
        }
        
        printf("\nCSRR Analysis Results:\n");
        printf("Electric Resonance: %.2f GHz\n", electric_resonance / 1e9);
        printf("Maximum Negative Permittivity: %.2f\n", max_epsilon_negative);
        
        // Analyze transmission characteristics
        printf("\nTransmission Characteristics:\n");
        printf("Frequency (GHz) | S21 (dB) | Phase (deg) | Group Delay (ns)\n");
        printf("----------------|----------|-------------|-----------------\n");
        
        for (int i = 0; i < results->num_frequencies; i += 15) {
            double freq = results->frequencies[i] / 1e9;
            double complex s21 = results->transmission_coeff[i];
            double s21_mag = cabs(s21);
            double s21_db = 20.0 * log10(s21_mag);
            double s21_phase = carg(s21) * 180.0 / M_PI;
            
            // Calculate group delay
            double group_delay = 0;
            if (i > 0 && i < results->num_frequencies - 1) {
                double df = results->frequencies[i+1] - results->frequencies[i-1];
                double dphase = carg(results->transmission_coeff[i+1]) - 
                               carg(results->transmission_coeff[i-1]);
                group_delay = -dphase / (2.0 * M_PI * df);
            }
            
            printf("%15.2f | %8.2f | %11.1f | %14.3f\n",
                   freq, s21_db, s21_phase, group_delay * 1e9);
        }
        
        // Band-stop filter characteristics
        printf("\nBand-Stop Filter Analysis:\n");
        
        double max_attenuation = 0;
        double center_frequency = 0;
        double bandwidth_3db = 0;
        double bandwidth_10db = 0;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex s21 = results->transmission_coeff[i];
            double s21_db = 20.0 * log10(cabs(s21));
            
            if (s21_db < max_attenuation) {
                max_attenuation = s21_db;
                center_frequency = results->frequencies[i];
            }
        }
        
        // Find bandwidths
        double freq_3db_low = 0, freq_3db_high = 0;
        double freq_10db_low = 0, freq_10db_high = 0;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex s21 = results->transmission_coeff[i];
            double s21_db = 20.0 * log10(cabs(s21));
            double freq = results->frequencies[i];
            
            // 3 dB bandwidth
            if (s21_db > -3.0 && freq < center_frequency && freq_3db_low == 0) {
                freq_3db_low = freq;
            }
            if (s21_db > -3.0 && freq > center_frequency) {
                freq_3db_high = freq;
            }
            
            // 10 dB bandwidth
            if (s21_db > -10.0 && freq < center_frequency && freq_10db_low == 0) {
                freq_10db_low = freq;
            }
            if (s21_db > -10.0 && freq > center_frequency) {
                freq_10db_high = freq;
            }
        }
        
        bandwidth_3db = freq_3db_high - freq_3db_low;
        bandwidth_10db = freq_10db_high - freq_10db_low;
        
        printf("  Center Frequency: %.2f GHz\n", center_frequency / 1e9);
        printf("  Maximum Attenuation: %.1f dB\n", max_attenuation);
        printf("  3 dB Bandwidth: %.2f GHz (%.1f%%)\n", 
               bandwidth_3db / 1e9, bandwidth_3db / center_frequency * 100);
        printf("  10 dB Bandwidth: %.2f GHz (%.1f%%)\n", 
               bandwidth_10db / 1e9, bandwidth_10db / center_frequency * 100);
        
        // Quality factor
        double q_factor = center_frequency / bandwidth_3db;
        printf("  Quality Factor (Q): %.1f\n", q_factor);
        
        metamaterial_results_destroy(results);
    }
    
    metamaterial_structure_destroy(metamaterial);
    return 0;
}

/*********************************************************************
 * Example 3: Wire Medium (Artificial Plasma) Analysis
 *********************************************************************/
int example_wire_medium_analysis() {
    printf("\n=== Example 3: Wire Medium (Artificial Plasma) Analysis ===\n");
    
    // Create wire medium metamaterial
    MetamaterialStructure* metamaterial = metamaterial_structure_create();
    
    // Define wire medium parameters
    WireMediumParameters wire_params = {
        .wire_radius = 25e-6,        // 25 μm wire radius
        .wire_length = 10.0e-3,      // 10 mm wire length
        .lattice_constant = 2.0e-3,   // 2 mm lattice constant
        .substrate_thickness = 0.8e-3, // 0.8 mm substrate
        .substrate_epsilon_r = 2.2,   // Rogers substrate
        .metal_conductivity = 5.8e7,  // Copper
        .wire_orientation = WIRE_VERTICAL, // Vertical wires
        .unit_cell_size = 2.5e-3     // 2.5 mm unit cell
    };
    
    // Create wire medium
    metamaterial_wire_medium_create(metamaterial, &wire_params);
    
    // Define extraction parameters for plasma behavior
    FrequencyRange freq_range = {
        .start = 100.0e6,    // 100 MHz
        .stop = 25.0e9,      // 25 GHz
        .points = 250,
        .log_scale = 1
    };
    
    ExtractionParameters extract_params = {
        .method = METHOD_RETRIEVAL,     // Retrieval method for plasma
        .frequency_range = freq_range,
        .incidence_angle = 0.0,
        .polarization = POL_TE,         // TE for wire medium
        .reference_impedance = 377.0,   // Free space impedance
        .extraction_points = 10,
        .smoothing_window = 7,
        .phase_unwrapping = 1,
        .passivity_enforcement = 1
    };
    
    printf("Performing wire medium parameter extraction...\n");
    clock_t start = clock();
    
    MetamaterialResults* results = metamaterial_extract_parameters(metamaterial, &extract_params);
    
    clock_t end = clock();
    double extraction_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (results) {
        printf("Wire medium extraction completed in %.2f seconds\n", extraction_time);
        
        // Find plasma frequency
        double plasma_freq = 0;
        double max_epsilon_negative = 0;
        
        for (int i = 1; i < results->num_frequencies; i++) {
            double freq = results->frequencies[i];
            double complex epsilon_eff = results->epsilon_eff[i];
            double epsilon_real = creal(epsilon_eff);
            
            // Find where epsilon approaches zero (plasma frequency)
            if (epsilon_real > -0.1 && epsilon_real < 0.1 && freq > 1.0e9) {
                plasma_freq = freq;
                break;
            }
            
            // Track maximum negative permittivity
            if (epsilon_real < 0 && fabs(epsilon_real) > max_epsilon_negative) {
                max_epsilon_negative = fabs(epsilon_real);
            }
        }
        
        printf("\nWire Medium Analysis:\n");
        printf("Plasma Frequency: %.2f GHz\n", plasma_freq / 1e9);
        printf("Maximum Negative Permittivity: %.2f\n", max_epsilon_negative);
        
        // Analyze Drude-like behavior
        printf("\nDrude Model Analysis:\n");
        printf("Frequency (GHz) | ε' | ε'' | ωp/ω | Loss Tangent\n");
        printf("----------------|-----|------|-------|-------------\n");
        
        for (int i = 0; i < results->num_frequencies; i += 20) {
            double freq = results->frequencies[i] / 1e9;
            double complex epsilon_eff = results->epsilon_eff[i];
            double epsilon_real = creal(epsilon_eff);
            double epsilon_imag = cimag(epsilon_eff);
            
            // Normalized frequency
            double omega_p_over_omega = plasma_freq / results->frequencies[i];
            
            // Loss tangent
            double loss_tangent = epsilon_imag / fabs(epsilon_real);
            if (epsilon_real > 0) loss_tangent = 0;
            
            printf("%15.2f | %5.2f | %6.3f | %6.3f | %12.4f\n",
                   freq, epsilon_real, epsilon_imag, omega_p_over_omega, loss_tangent);
        }
        
        // Fit Drude model
        DrudeModel drude = metamaterial_fit_drude_model(results);
        
        printf("\nDrude Model Parameters:\n");
        printf("  Plasma Frequency (ωp): %.2f GHz\n", drude.plasma_frequency / 1e9);
        printf("  Collision Frequency (γ): %.2f GHz\n", drude.collision_frequency / 1e9);
        printf("  High-frequency ε∞: %.3f\n", drude.epsilon_infinity);
        printf("  Quality Factor: %.1f\n", drude.quality_factor);
        
        // Compare with theoretical plasma frequency
        double theoretical_fp = 1.0 / (2.0 * M_PI) * 
                               sqrt(wire_params.wire_radius / 
                                   (wire_params.lattice_constant * 
                                    wire_params.lattice_constant * 
                                    wire_params.substrate_epsilon_r)) * 
                               sqrt(wire_params.metal_conductivity / 
                                   (M_PI * 4e-7));
        
        printf("\nTheoretical vs Extracted Plasma Frequency:\n");
        printf("  Theoretical: %.2f GHz\n", theoretical_fp / 1e9);
        printf("  Extracted: %.2f GHz\n", drude.plasma_frequency / 1e9);
        printf("  Error: %.1f%%\n", 
               fabs(theoretical_fp - drude.plasma_frequency) / theoretical_fp * 100);
        
        // Surface wave analysis for wire medium
        printf("\nSurface Wave Characteristics:\n");
        printf("Frequency (GHz) | β/k0 | Attenuation (dB/cm) | Type\n");
        printf("----------------|------|-------------------|------\n");
        
        for (int i = 0; i < results->num_frequencies; i += 25) {
            double freq = results->frequencies[i] / 1e9;
            double complex epsilon_eff = results->epsilon_eff[i];
            
            // Calculate surface wave properties
            SurfaceWaveProps sw = metamaterial_calculate_surface_waves(
                results->frequencies[i], epsilon_eff, 1.0);
            
            const char* wave_type = (sw.beta_over_k0 > 1.0) ? "Slow wave" : "Fast wave";
            
            printf("%15.2f | %6.3f | %17.3f | %s\n",
                   freq, sw.beta_over_k0, sw.attenuation_db_per_cm, wave_type);
        }
        
        metamaterial_results_destroy(results);
    }
    
    metamaterial_structure_destroy(metamaterial);
    return 0;
}

/*********************************************************************
 * Example 4: Metamaterial Absorber Design
 *********************************************************************/
int example_metamaterial_absorber() {
    printf("\n=== Example 4: Metamaterial Absorber Design ===\n");
    
    // Create metamaterial absorber structure
    MetamaterialStructure* metamaterial = metamaterial_structure_create();
    
    // Define absorber parameters (electric ring resonator + resistive film)
    AbsorberParameters absorber_params = {
        .ring_outer_radius = 4.0e-3,   // 4 mm outer radius
        .ring_width = 0.5e-3,          // 0.5 mm ring width
        .gap_size = 0.3e-3,             // 0.3 mm gap
        .resistive_sheet_rsq = 100.0,  // 100 Ω/sq resistive film
        .dielectric_spacer_thickness = 3.0e-3, // 3 mm spacer
        .dielectric_spacer_epsilon_r = 1.05,   // Foam spacer
        .ground_plane_thickness = 35e-6,       // Copper ground
        .substrate_thickness = 1.6e-3,         // 1.6 mm substrate
        .substrate_epsilon_r = 4.4,            // FR4
        .unit_cell_size = 10.0e-3             // 10 mm unit cell
    };
    
    // Create metamaterial absorber
    metamaterial_absorber_create(metamaterial, &absorber_params);
    
    // Define frequency range for absorber analysis
    FrequencyRange freq_range = {
        .start = 2.0e9,     // 2 GHz
        .stop = 40.0e9,     // 40 GHz
        .points = 191,
        .log_scale = 0     // Linear scale for narrowband analysis
    };
    
    ExtractionParameters extract_params = {
        .method = METHOD_RETRIEVAL,
        .frequency_range = freq_range,
        .incidence_angle = 0.0,
        .polarization = POL_TE,
        .reference_impedance = 377.0,
        .extraction_points = 15,
        .smoothing_window = 5,
        .phase_unwrapping = 1,
        .passivity_enforcement = 1
    };
    
    printf("Analyzing metamaterial absorber...\n");
    clock_t start = clock();
    
    MetamaterialResults* results = metamaterial_extract_parameters(metamaterial, &extract_params);
    
    clock_t end = clock();
    double extraction_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    if (results) {
        printf("Absorber analysis completed in %.2f seconds\n", extraction_time);
        
        // Find absorption peaks
        double max_absorption = 0;
        double absorption_freq = 0;
        double min_reflection = 0;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex s11 = results->reflection_coeff[i];
            double complex s21 = results->transmission_coeff[i];
            
            double reflection_mag = cabs(s11);
            double transmission_mag = cabs(s21);
            
            // Absorption (assuming no transmission through ground plane)
            double absorption = 1.0 - reflection_mag * reflection_mag - 
                               transmission_mag * transmission_mag;
            
            if (absorption > max_absorption) {
                max_absorption = absorption;
                absorption_freq = results->frequencies[i];
                min_reflection = reflection_mag;
            }
        }
        
        printf("\nAbsorber Performance:\n");
        printf("  Peak Absorption: %.1f%% at %.2f GHz\n", 
               max_absorption * 100, absorption_freq / 1e9);
        printf("  Minimum Reflection: %.1f dB\n", 20.0 * log10(min_reflection));
        
        // Detailed absorption analysis
        printf("\nAbsorption vs Frequency:\n");
        printf("Frequency (GHz) | Reflection (dB) | Absorption (%) | η (%)\n");
        printf("----------------|-----------------|------------------|------\n");
        
        for (int i = 0; i < results->num_frequencies; i += 10) {
            double freq = results->frequencies[i] / 1e9;
            double complex s11 = results->reflection_coeff[i];
            double complex eta = results->wave_impedance[i];
            
            double reflection_db = 20.0 * log10(cabs(s11));
            double absorption = 1.0 - cabs(s11) * cabs(s11);
            double eta_real = creal(eta) / 377.0 * 100.0; // Normalized to free space
            
            printf("%15.2f | %15.1f | %16.1f | %5.1f\n",
                   freq, reflection_db, absorption * 100, eta_real);
        }
        
        // Bandwidth analysis
        printf("\nAbsorption Bandwidth Analysis:\n");
        
        double freq_90_abs_low = 0, freq_90_abs_high = 0;
        double freq_80_abs_low = 0, freq_80_abs_high = 0;
        double freq_50_abs_low = 0, freq_50_abs_high = 0;
        
        for (int i = 0; i < results->num_frequencies; i++) {
            double complex s11 = results->reflection_coeff[i];
            double absorption = 1.0 - cabs(s11) * cabs(s11);
            double freq = results->frequencies[i];
            
            // 90% absorption bandwidth
            if (absorption >= 0.9) {
                if (freq_90_abs_low == 0) freq_90_abs_low = freq;
                freq_90_abs_high = freq;
            }
            
            // 80% absorption bandwidth
            if (absorption >= 0.8) {
                if (freq_80_abs_low == 0) freq_80_abs_low = freq;
                freq_80_abs_high = freq;
            }
            
            // 50% absorption bandwidth
            if (absorption >= 0.5) {
                if (freq_50_abs_low == 0) freq_50_abs_low = freq;
                freq_50_abs_high = freq;
            }
        }
        
        double bw_90_abs = freq_90_abs_high - freq_90_abs_low;
        double bw_80_abs = freq_80_abs_high - freq_80_abs_low;
        double bw_50_abs = freq_50_abs_high - freq_50_abs_low;
        
        printf("  90%% Absorption: %.2f - %.2f GHz (%.2f GHz)\n",
               freq_90_abs_low / 1e9, freq_90_abs_high / 1e9, bw_90_abs / 1e9);
        printf("  80%% Absorption: %.2f - %.2f GHz (%.2f GHz)\n",
               freq_80_abs_low / 1e9, freq_80_abs_high / 1e9, bw_80_abs / 1e9);
        printf("  50%% Absorption: %.2f - %.2f GHz (%.2f GHz)\n",
               freq_50_abs_low / 1e9, freq_50_abs_high / 1e9, bw_50_abs / 1e9);
        
        // Fractional bandwidths
        printf("\nFractional Bandwidths:\n");
        printf("  90%% BW: %.1f%%\n", bw_90_abs / absorption_freq * 100);
        printf("  80%% BW: %.1f%%\n", bw_80_abs / absorption_freq * 100);
        printf("  50%% BW: %.1f%%\n", bw_50_abs / absorption_freq * 100);
        
        // Angular stability analysis
        printf("\nAngular Stability Analysis:\n");
        printf("Angle (deg) | Peak Freq (GHz) | Absorption (%) | Reflection (dB)\n");
        printf("------------|------------------|----------------|-----------------\n");
        
        for (int angle = 0; angle <= 60; angle += 15) {
            // Modify extraction parameters for different angles
            extract_params.incidence_angle = angle * M_PI / 180.0;
            
            MetamaterialResults* angle_results = metamaterial_extract_parameters(
                metamaterial, &extract_params);
            
            if (angle_results) {
                // Find peak absorption for this angle
                double max_abs_angle = 0;
                double peak_freq_angle = 0;
                double min_refl_angle = 0;
                
                for (int i = 0; i < angle_results->num_frequencies; i++) {
                    double complex s11 = angle_results->reflection_coeff[i];
                    double absorption = 1.0 - cabs(s11) * cabs(s11);
                    
                    if (absorption > max_abs_angle) {
                        max_abs_angle = absorption;
                        peak_freq_angle = angle_results->frequencies[i];
                        min_refl_angle = cabs(s11);
                    }
                }
                
                printf("%11d | %16.2f | %14.1f | %15.1f\n",
                       angle, peak_freq_angle / 1e9, max_abs_angle * 100,
                       20.0 * log10(min_refl_angle));
                
                metamaterial_results_destroy(angle_results);
            }
        }
        
        metamaterial_results_destroy(results);
    }
    
    metamaterial_structure_destroy(metamaterial);
    return 0;
}

/*********************************************************************
 * Example 5: Parallel Metamaterial Parameter Extraction
 *********************************************************************/
int example_parallel_metamaterial_extraction() {
    printf("\n=== Example 5: Parallel Metamaterial Parameter Extraction ===\n");
    
    // Initialize parallel processing
    int num_threads = 8;
    omp_set_num_threads(num_threads);
    
    // Create multiple metamaterial configurations for parallel analysis
    int num_configs = 16;
    MetamaterialConfig* configs = malloc(num_configs * sizeof(MetamaterialConfig));
    
    // Generate different SRR configurations
    for (int i = 0; i < num_configs; i++) {
        configs[i].type = METAMATERIAL_SRR;
        configs[i].params.srr.outer_radius = 1.0e-3 + i * 0.2e-3;  // 1-4.2 mm
        configs[i].params.srr.inner_radius = 0.6e-3 + i * 0.12e-3; // 0.6-2.5 mm
        configs[i].params.srr.gap_width = 0.1e-3 + i * 0.02e-3;   // 0.1-0.42 mm
        configs[i].params.srr.metal_width = 0.1e-3 + i * 0.01e-3; // 0.1-0.25 mm
        configs[i].params.srr.substrate_epsilon_r = 2.2 + i * 0.2; // 2.2-5.4
        
        sprintf(configs[i].name, "SRR_%d", i + 1);
    }
    
    printf("Analyzing %d metamaterial configurations in parallel...\n", num_configs);
    
    // Parallel analysis
    clock_t start = clock();
    
    #pragma omp parallel for
    for (int i = 0; i < num_configs; i++) {
        int thread_id = omp_get_thread_num();
        
        printf("[Thread %d] Analyzing %s...\n", thread_id, configs[i].name);
        
        // Create metamaterial structure
        MetamaterialStructure* metamaterial = metamaterial_structure_create();
        metamaterial_srr_create(metamaterial, &configs[i].params.srr);
        
        // Define extraction parameters
        FrequencyRange freq_range = {
            .start = 500.0e6,
            .stop = 20.0e9,
            .points = 101,
            .log_scale = 1
        };
        
        ExtractionParameters extract_params = {
            .method = METHOD_NRW,
            .frequency_range = freq_range,
            .incidence_angle = 0.0,
            .polarization = POL_TE,
            .reference_impedance = 50.0,
            .extraction_points = 5,
            .smoothing_window = 3,
            .phase_unwrapping = 1,
            .passivity_enforcement = 1
        };
        
        // Perform extraction
        MetamaterialResults* results = metamaterial_extract_parameters(metamaterial, &extract_params);
        
        if (results) {
            // Find magnetic resonance
            double magnetic_resonance = 0;
            double max_mu_negative = 0;
            
            for (int j = 0; j < results->num_frequencies; j++) {
                double complex mu_eff = results->mu_eff[j];
                double mu_real = creal(mu_eff);
                
                if (j > 0 && creal(results->mu_eff[j-1]) < 0 && mu_real > 0) {
                    magnetic_resonance = results->frequencies[j];
                    break;
                }
                
                if (mu_real < 0 && fabs(mu_real) > max_mu_negative) {
                    max_mu_negative = fabs(mu_real);
                }
            }
            
            // Store results
            configs[i].results.resonant_frequency = magnetic_resonance;
            configs[i].results.max_parameter_value = max_mu_negative;
            configs[i].results.extraction_time = omp_get_wtime() - start;
            
            printf("[Thread %d] %s: Magnetic resonance = %.2f GHz\n",
                   thread_id, configs[i].name, magnetic_resonance / 1e9);
            
            metamaterial_results_destroy(results);
        }
        
        metamaterial_structure_destroy(metamaterial);
    }
    
    clock_t end = clock();
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("\nParallel analysis completed in %.2f seconds\n", total_time);
    printf("Average time per configuration: %.2f seconds\n", total_time / num_configs);
    
    // Find configuration with lowest resonance frequency
    int best_config = 0;
    double lowest_resonance = 1e12;
    
    for (int i = 0; i < num_configs; i++) {
        if (configs[i].results.resonant_frequency < lowest_resonance) {
            lowest_resonance = configs[i].results.resonant_frequency;
            best_config = i;
        }
    }
    
    printf("\nBest Configuration: %s\n", configs[best_config].name);
    printf("  Lowest Resonance: %.2f GHz\n", configs[best_config].results.resonant_frequency / 1e9);
    printf("  Maximum μ-negative: %.2f\n", configs[best_config].results.max_parameter_value);
    
    // Performance summary
    printf("\nPerformance Summary:\n");
    printf("Configuration | Resonance (GHz) | Max μ(-) | Time (s)\n");
    printf("--------------|-----------------|----------|----------\n");
    
    for (int i = 0; i < num_configs; i++) {
        printf("%-13s | %15.2f | %8.2f | %8.2f\n",
               configs[i].name,
               configs[i].results.resonant_frequency / 1e9,
               configs[i].results.max_parameter_value,
               configs[i].results.extraction_time);
    }
    
    free(configs);
    return 0;
}

/*********************************************************************
 * Main function to run all examples
 *********************************************************************/
int main() {
    printf("Metamaterial Parameter Extraction Examples\n");
    printf("==========================================\n\n");
    
    int result = 0;
    
    // Run all metamaterial extraction examples
    result |= example_srr_parameter_extraction();
    result |= example_csrr_analysis();
    result |= example_wire_medium_analysis();
    result |= example_metamaterial_absorber();
    result |= example_parallel_metamaterial_extraction();
    
    if (result == 0) {
        printf("\nAll metamaterial extraction examples completed successfully!\n");
    } else {
        printf("\nSome metamaterial extraction examples failed with errors.\n");
    }
    
    return result;
}