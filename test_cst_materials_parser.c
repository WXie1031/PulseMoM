/**
 * Test program for CST material parser with actual CST material files
 * Tests parsing of real CST .mtd files and validates electromagnetic properties
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "src/materials/cst_materials_parser.h"

// Test function prototypes
int test_copper_material();
int test_pec_material();
int test_fr4_material();
int test_air_material();
int test_material_properties();
int test_frequency_dependent_properties();
void print_material_summary(cst_material_t* material);

int main() {
    printf("=== CST Materials Parser Test ===\n");
    printf("Testing with actual CST material files from library\n\n");
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // Test 1: Copper material
    printf("Test 1: Copper (pure) material\n");
    if (test_copper_material()) {
        printf("✓ PASSED\n");
        passed_tests++;
    } else {
        printf("✗ FAILED\n");
    }
    total_tests++;
    printf("\n");
    
    // Test 2: PEC material
    printf("Test 2: PEC material\n");
    if (test_pec_material()) {
        printf("✓ PASSED\n");
        passed_tests++;
    } else {
        printf("✗ FAILED\n");
    }
    total_tests++;
    printf("\n");
    
    // Test 3: FR-4 material
    printf("Test 3: FR-4 (lossy) material\n");
    if (test_fr4_material()) {
        printf("✓ PASSED\n");
        passed_tests++;
    } else {
        printf("✗ FAILED\n");
    }
    total_tests++;
    printf("\n");
    
    // Test 4: Air material
    printf("Test 4: Air material\n");
    if (test_air_material()) {
        printf("✓ PASSED\n");
        passed_tests++;
    } else {
        printf("✗ FAILED\n");
    }
    total_tests++;
    printf("\n");
    
    // Test 5: Material properties validation
    printf("Test 5: Material properties validation\n");
    if (test_material_properties()) {
        printf("✓ PASSED\n");
        passed_tests++;
    } else {
        printf("✗ FAILED\n");
    }
    total_tests++;
    printf("\n");
    
    // Test 6: Frequency dependent properties
    printf("Test 6: Frequency dependent properties\n");
    if (test_frequency_dependent_properties()) {
        printf("✓ PASSED\n");
        passed_tests++;
    } else {
        printf("✗ FAILED\n");
    }
    total_tests++;
    printf("\n");
    
    // Summary
    printf("=== Test Summary ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success rate: %.1f%%\n", (double)passed_tests / total_tests * 100.0);
    
    return (passed_tests == total_tests) ? 0 : 1;
}

int test_copper_material() {
    const char* filename = "D:/Project/MoM/PulseMoM/library/Materials/Copper (pure).mtd";
    
    cst_material_t* material = cst_parse_material_file(filename);
    if (!material) {
        printf("Failed to parse copper material file\n");
        return 0;
    }
    
    printf("Parsed material: %s\n", material->name);
    printf("Type: %s\n", material->type);
    printf("Frequency type: %s\n", material->frequency_type);
    
    // Validate electromagnetic properties
    double expected_sigma = 5.96e7; // S/m
    double expected_mur = 1.0;
    double expected_epsr = 1.0;
    
    printf("Conductivity: %.2e S/m (expected: %.2e)\n", 
           material->sigma, expected_sigma);
    printf("Relative permeability: %.3f (expected: %.3f)\n", 
           material->mur, expected_mur);
    printf("Relative permittivity: %.3f (expected: %.3f)\n", 
           material->epsr, expected_epsr);
    
    int valid = 1;
    if (fabs(material->sigma - expected_sigma) > 1e6) valid = 0;
    if (fabs(material->mur - expected_mur) > 0.01) valid = 0;
    if (fabs(material->epsr - expected_epsr) > 0.01) valid = 0;
    
    // Check material classification
    cst_material_class_t class = cst_classify_material(material, 10.0e9);
    printf("Material classification at 10GHz: %s\n", 
           cst_get_material_class_name(class));
    
    if (class != CST_MATERIAL_CONDUCTOR) {
        printf("ERROR: Copper should be classified as conductor\n");
        valid = 0;
    }
    
    cst_free_material(material);
    return valid;
}

int test_pec_material() {
    const char* filename = "D:/Project/MoM/PulseMoM/library/Materials/PEC.mtd";
    
    cst_material_t* material = cst_parse_material_file(filename);
    if (!material) {
        printf("Failed to parse PEC material file\n");
        return 0;
    }
    
    printf("Parsed material: %s\n", material->name);
    printf("Type: %s\n", material->type);
    
    // PEC should have specific properties
    printf("Relative permittivity: %.3f\n", material->epsr);
    printf("Relative permeability: %.3f\n", material->mur);
    printf("Conductivity: %.2e S/m\n", material->sigma);
    
    int valid = 1;
    if (fabs(material->epsr - 1.0) > 0.01) valid = 0;
    if (fabs(material->mur - 1.0) > 0.01) valid = 0;
    
    // Check material classification
    cst_material_class_t class = cst_classify_material(material, 10.0e9);
    printf("Material classification at 10GHz: %s\n", 
           cst_get_material_class_name(class));
    
    if (class != CST_MATERIAL_PEC) {
        printf("ERROR: Should be classified as PEC\n");
        valid = 0;
    }
    
    cst_free_material(material);
    return valid;
}

int test_fr4_material() {
    const char* filename = "D:/Project/MoM/PulseMoM/library/Materials/FR-4 (lossy).mtd";
    
    cst_material_t* material = cst_parse_material_file(filename);
    if (!material) {
        printf("Failed to parse FR-4 material file\n");
        return 0;
    }
    
    printf("Parsed material: %s\n", material->name);
    printf("Type: %s\n", material->type);
    
    // Validate dielectric properties
    double expected_epsr = 4.3;
    double expected_tand = 0.025;
    double expected_tand_freq = 10.0; // GHz
    
    printf("Relative permittivity: %.3f (expected: %.3f)\n", 
           material->epsr, expected_epsr);
    printf("Loss tangent: %.4f (expected: %.4f)\n", 
           material->tand, expected_tand);
    printf("Loss tangent frequency: %.1f GHz\n", material->tand_freq);
    
    int valid = 1;
    if (fabs(material->epsr - expected_epsr) > 0.1) valid = 0;
    if (fabs(material->tand - expected_tand) > 0.005) valid = 0;
    if (fabs(material->tand_freq - expected_tand_freq) > 0.1) valid = 0;
    
    // Check material classification
    cst_material_class_t class = cst_classify_material(material, 10.0e9);
    printf("Material classification at 10GHz: %s\n", 
           cst_get_material_class_name(class));
    
    if (class != CST_MATERIAL_DIELECTRIC) {
        printf("ERROR: FR-4 should be classified as dielectric\n");
        valid = 0;
    }
    
    cst_free_material(material);
    return valid;
}

int test_air_material() {
    const char* filename = "D:/Project/MoM/PulseMoM/library/Materials/Air.mtd";
    
    cst_material_t* material = cst_parse_material_file(filename);
    if (!material) {
        printf("Failed to parse Air material file\n");
        return 0;
    }
    
    printf("Parsed material: %s\n", material->name);
    printf("Type: %s\n", material->type);
    
    // Air properties
    double expected_epsr = 1.00059;
    double expected_mur = 1.0;
    
    printf("Relative permittivity: %.5f (expected: %.5f)\n", 
           material->epsr, expected_epsr);
    printf("Relative permeability: %.3f (expected: %.3f)\n", 
           material->mur, expected_mur);
    printf("Density: %.3f kg/m^3\n", material->rho);
    
    int valid = 1;
    if (fabs(material->epsr - expected_epsr) > 0.001) valid = 0;
    if (fabs(material->mur - expected_mur) > 0.01) valid = 0;
    
    // Check material classification
    cst_material_class_t class = cst_classify_material(material, 10.0e9);
    printf("Material classification at 10GHz: %s\n", 
           cst_get_material_class_name(class));
    
    if (class != CST_MATERIAL_DIELECTRIC) {
        printf("ERROR: Air should be classified as dielectric\n");
        valid = 0;
    }
    
    cst_free_material(material);
    return valid;
}

int test_material_properties() {
    printf("Testing material property evaluation at different frequencies...\n");
    
    const char* filename = "D:/Project/MoM/PulseMoM/library/Materials/Copper (pure).mtd";
    cst_material_t* material = cst_parse_material_file(filename);
    if (!material) {
        printf("Failed to load copper material\n");
        return 0;
    }
    
    // Test at different frequencies
    double frequencies[] = {1e6, 10e6, 100e6, 1e9, 10e9, 100e9}; // Hz
    int num_freqs = sizeof(frequencies) / sizeof(double);
    
    printf("Frequency (GHz)\tConductivity (S/m)\tSkin depth (μm)\n");
    printf("----------------------------------------------------\n");
    
    for (int i = 0; i < num_freqs; i++) {
        double freq = frequencies[i];
        double omega = 2.0 * M_PI * freq;
        
        // Evaluate material properties at this frequency
        cst_evaluate_material_at_frequency(material, freq);
        
        // Calculate skin depth for good conductor
        double skin_depth = 0.0;
        if (material->sigma > 1e6) {
            skin_depth = sqrt(2.0 / (omega * 4.0 * M_PI * 1e-7 * material->sigma));
        }
        
        printf("%.1f\t\t%.2e\t\t%.1f\n", 
               freq/1e9, material->sigma, skin_depth*1e6);
    }
    
    cst_free_material(material);
    return 1;
}

int test_frequency_dependent_properties() {
    printf("Testing frequency-dependent material properties...\n");
    
    // Test FR-4 which has frequency-dependent loss tangent
    const char* filename = "D:/Project/MoM/PulseMoM/library/Materials/FR-4 (lossy).mtd";
    cst_material_t* material = cst_parse_material_file(filename);
    if (!material) {
        printf("Failed to load FR-4 material\n");
        return 0;
    }
    
    printf("Material: %s\n", material->name);
    printf("Dispersion model: %s\n", material->disp_model_eps);
    printf("Loss tangent model: %s\n", material->tand_model);
    
    // Test at different frequencies
    double frequencies[] = {1e9, 5e9, 10e9, 20e9, 50e9}; // Hz
    int num_freqs = sizeof(frequencies) / sizeof(double);
    
    printf("Frequency (GHz)\tEpsilon\tLoss Tangent\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < num_freqs; i++) {
        double freq = frequencies[i];
        cst_evaluate_material_at_frequency(material, freq);
        
        printf("%.1f\t\t%.3f\t%.4f\n", 
               freq/1e9, material->epsr, material->tand);
    }
    
    cst_free_material(material);
    return 1;
}

void print_material_summary(cst_material_t* material) {
    if (!material) return;
    
    printf("Material Summary:\n");
    printf("  Name: %s\n", material->name);
    printf("  Type: %s\n", material->type);
    printf("  Frequency Type: %s\n", material->frequency_type);
    printf("  εr: %.4f\n", material->epsr);
    printf("  μr: %.4f\n", material->mur);
    printf("  σ: %.2e S/m\n", material->sigma);
    printf("  tan δ: %.4f\n", material->tand);
    printf("  ρ: %.1f kg/m³\n", material->rho);
    printf("  Thermal Conductivity: %.2f W/K/m\n", material->thermal_conductivity);
    printf("  Color: RGB(%.2f, %.2f, %.2f)\n", 
           material->color_r, material->color_g, material->color_b);
}