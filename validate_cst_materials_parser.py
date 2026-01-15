#!/usr/bin/env python3
"""
Validation script for CST material parser implementation
Tests parsing of CST .mtd files and validates electromagnetic properties
"""

import os
import re
import sys

def parse_cst_material_file(filename):
    """Parse CST material file (.mtd format)"""
    material = {
        'name': os.path.basename(filename).replace('.mtd', ''),
        'type': 'Unknown',
        'frequency_type': 'all',
        'epsr': 1.0,
        'mur': 1.0,
        'sigma': 0.0,
        'tand': 0.0,
        'tand_freq': 0.0,
        'rho': 0.0,
        'thermal_conductivity': 0.0,
        'color_r': 0.5,
        'color_g': 0.5,
        'color_b': 0.5,
        'disp_model_eps': 'None',
        'tand_model': 'ConstKappa'
    }
    
    try:
        with open(filename, 'r') as f:
            content = f.read()
        
        # Parse Definition section
        def_section = re.search(r'\[Definition\](.*?)(?=\[|\Z)', content, re.DOTALL)
        if def_section:
            def_text = def_section.group(1)
            
            # Extract properties
            type_match = re.search(r'\.Type\s+"([^"]+)"', def_text)
            if type_match:
                material['type'] = type_match.group(1)
            
            frqtype_match = re.search(r'\.FrqType\s+"([^"]+)"', def_text)
            if frqtype_match:
                material['frequency_type'] = frqtype_match.group(1)
            
            eps_match = re.search(r'\.Epsilon\s+"([\d.]+)"', def_text)
            if eps_match:
                material['epsr'] = float(eps_match.group(1))
            
            mu_match = re.search(r'\.Mu\s+"([\d.]+)"', def_text)
            if mu_match:
                material['mur'] = float(mu_match.group(1))
            
            sigma_match = re.search(r'\.Sigma\s+"([\d.e+-]+)"', def_text)
            if sigma_match:
                material['sigma'] = float(sigma_match.group(1))
            
            kappa_match = re.search(r'\.Kappa\s+"([\d.e+-]+)"', def_text)
            if kappa_match and material['sigma'] == 0:
                material['sigma'] = float(kappa_match.group(1))
            
            tand_match = re.search(r'\.TanD\s+"([\d.]+)"', def_text)
            if tand_match:
                material['tand'] = float(tand_match.group(1))
            
            tandfreq_match = re.search(r'\.TanDFreq\s+"([\d.]+)"', def_text)
            if tandfreq_match:
                material['tand_freq'] = float(tandfreq_match.group(1))
            
            rho_match = re.search(r'\.Rho\s+"([\d.]+)"', def_text)
            if rho_match:
                material['rho'] = float(rho_match.group(1))
            
            therm_cond_match = re.search(r'\.ThermalConductivity\s+"([\d.]+)"', def_text)
            if therm_cond_match:
                material['thermal_conductivity'] = float(therm_cond_match.group(1))
            
            color_match = re.search(r'\.Colour\s+"([\d.]+)"\s*,\s*"([\d.]+)"\s*,\s*"([\d.]+)"', def_text)
            if color_match:
                material['color_r'] = float(color_match.group(1))
                material['color_g'] = float(color_match.group(2))
                material['color_b'] = float(color_match.group(3))
            
            disp_match = re.search(r'\.DispModelEps\s+"([^"]+)"', def_text)
            if disp_match:
                material['disp_model_eps'] = disp_match.group(1)
            
            tand_model_match = re.search(r'\.TanDModel\s+"([^"]+)"', def_text)
            if tand_model_match:
                material['tand_model'] = tand_model_match.group(1)
        
        return material
        
    except Exception as e:
        print(f"Error parsing {filename}: {e}")
        return None

def classify_material(material, frequency):
    """Classify material based on electromagnetic properties"""
    epsr = material['epsr']
    mur = material['mur']
    sigma = material['sigma']
    
    # PEC: Perfect Electric Conductor
    if material['type'] == 'PEC' or (epsr == 1.0 and mur == 1.0 and sigma > 1e12):
        return 'PEC'
    
    # Conductor: High conductivity
    if sigma > 1e6:
        return 'CONDUCTOR'
    
    # Magnetic material: High permeability
    if mur > 10:
        return 'MAGNETIC'
    
    # Dielectric: Low conductivity, significant permittivity
    if sigma < 1 and epsr > 1.1:
        return 'DIELECTRIC'
    
    # Semiconductor: Intermediate conductivity
    if 1e-4 < sigma < 1e6:
        return 'SEMICONDUCTOR'
    
    # Default to dielectric
    return 'DIELECTRIC'

def evaluate_material_at_frequency(material, frequency):
    """Evaluate material properties at specific frequency"""
    # For this implementation, return the base properties
    # In a full implementation, this would handle dispersion models
    return {
        'epsr': material['epsr'],
        'mur': material['mur'],
        'sigma': material['sigma'],
        'tand': material['tand']
    }

def test_copper_material():
    """Test copper material parsing and validation"""
    print("=== Testing Copper (pure) Material ===")
    filename = "D:/Project/MoM/PulseMoM/library/Materials/Copper (pure).mtd"
    
    material = parse_cst_material_file(filename)
    if not material:
        print("✗ FAILED: Could not parse copper material")
        return False
    
    print(f"Parsed material: {material['name']}")
    print(f"Type: {material['type']}")
    print(f"Frequency type: {material['frequency_type']}")
    
    # Validate properties
    expected_sigma = 5.96e7  # S/m
    expected_mur = 1.0
    expected_epsr = 1.0
    
    print(f"Conductivity: {material['sigma']:.2e} S/m (expected: {expected_sigma:.2e})")
    print(f"Relative permeability: {material['mur']:.3f} (expected: {expected_mur:.3f})")
    print(f"Relative permittivity: {material['epsr']:.3f} (expected: {expected_epsr:.3f})")
    
    # Check classification
    classification = classify_material(material, 10e9)
    print(f"Material classification at 10GHz: {classification}")
    
    # Validation
    valid = True
    if abs(material['sigma'] - expected_sigma) > 1e6:
        print(f"✗ ERROR: Conductivity mismatch")
        valid = False
    if abs(material['mur'] - expected_mur) > 0.01:
        print(f"✗ ERROR: Permeability mismatch")
        valid = False
    if abs(material['epsr'] - expected_epsr) > 0.01:
        print(f"✗ ERROR: Permittivity mismatch")
        valid = False
    if classification != 'CONDUCTOR':
        print(f"✗ ERROR: Should be classified as CONDUCTOR")
        valid = False
    
    if valid:
        print("✓ PASSED: Copper material validation")
    
    return valid

def test_pec_material():
    """Test PEC material parsing and validation"""
    print("\n=== Testing PEC Material ===")
    filename = "D:/Project/MoM/PulseMoM/library/Materials/PEC.mtd"
    
    material = parse_cst_material_file(filename)
    if not material:
        print("✗ FAILED: Could not parse PEC material")
        return False
    
    print(f"Parsed material: {material['name']}")
    print(f"Type: {material['type']}")
    print(f"Relative permittivity: {material['epsr']:.3f}")
    print(f"Relative permeability: {material['mur']:.3f}")
    print(f"Conductivity: {material['sigma']:.2e} S/m")
    
    # Check classification
    classification = classify_material(material, 10e9)
    print(f"Material classification at 10GHz: {classification}")
    
    # Validation
    valid = True
    if material['type'] != 'PEC':
        print(f"✗ ERROR: Type should be PEC")
        valid = False
    if classification != 'PEC':
        print(f"✗ ERROR: Should be classified as PEC")
        valid = False
    
    if valid:
        print("✓ PASSED: PEC material validation")
    
    return valid

def test_fr4_material():
    """Test FR-4 material parsing and validation"""
    print("\n=== Testing FR-4 (lossy) Material ===")
    filename = "D:/Project/MoM/PulseMoM/library/Materials/FR-4 (lossy).mtd"
    
    material = parse_cst_material_file(filename)
    if not material:
        print("✗ FAILED: Could not parse FR-4 material")
        return False
    
    print(f"Parsed material: {material['name']}")
    print(f"Type: {material['type']}")
    
    # Validate dielectric properties
    expected_epsr = 4.3
    expected_tand = 0.025
    expected_tand_freq = 10.0  # GHz
    
    print(f"Relative permittivity: {material['epsr']:.3f} (expected: {expected_epsr:.3f})")
    print(f"Loss tangent: {material['tand']:.4f} (expected: {expected_tand:.4f})")
    print(f"Loss tangent frequency: {material['tand_freq']:.1f} GHz")
    print(f"Dispersion model: {material['disp_model_eps']}")
    print(f"Loss tangent model: {material['tand_model']}")
    
    # Check classification
    classification = classify_material(material, 10e9)
    print(f"Material classification at 10GHz: {classification}")
    
    # Validation
    valid = True
    if abs(material['epsr'] - expected_epsr) > 0.1:
        print(f"✗ ERROR: Permittivity mismatch")
        valid = False
    if abs(material['tand'] - expected_tand) > 0.005:
        print(f"✗ ERROR: Loss tangent mismatch")
        valid = False
    if abs(material['tand_freq'] - expected_tand_freq) > 0.1:
        print(f"✗ ERROR: Loss tangent frequency mismatch")
        valid = False
    if classification != 'DIELECTRIC':
        print(f"✗ ERROR: Should be classified as DIELECTRIC")
        valid = False
    
    if valid:
        print("✓ PASSED: FR-4 material validation")
    
    return valid

def test_air_material():
    """Test Air material parsing and validation"""
    print("\n=== Testing Air Material ===")
    filename = "D:/Project/MoM/PulseMoM/library/Materials/Air.mtd"
    
    material = parse_cst_material_file(filename)
    if not material:
        print("✗ FAILED: Could not parse Air material")
        return False
    
    print(f"Parsed material: {material['name']}")
    print(f"Type: {material['type']}")
    
    # Air properties
    expected_epsr = 1.00059
    expected_mur = 1.0
    
    print(f"Relative permittivity: {material['epsr']:.5f} (expected: {expected_epsr:.5f})")
    print(f"Relative permeability: {material['mur']:.3f} (expected: {expected_mur:.3f})")
    print(f"Density: {material['rho']:.3f} kg/m^3")
    print(f"Thermal conductivity: {material['thermal_conductivity']:.3f} W/K/m")
    
    # Check classification
    classification = classify_material(material, 10e9)
    print(f"Material classification at 10GHz: {classification}")
    
    # Validation
    valid = True
    if abs(material['epsr'] - expected_epsr) > 0.001:
        print(f"✗ ERROR: Permittivity mismatch")
        valid = False
    if abs(material['mur'] - expected_mur) > 0.01:
        print(f"✗ ERROR: Permeability mismatch")
        valid = False
    if classification != 'DIELECTRIC':
        print(f"✗ ERROR: Should be classified as DIELECTRIC")
        valid = False
    
    if valid:
        print("✓ PASSED: Air material validation")
    
    return valid

def test_frequency_dependent_properties():
    """Test frequency-dependent material properties"""
    print("\n=== Testing Frequency-Dependent Properties ===")
    
    # Test copper conductivity and skin depth
    filename = "D:/Project/MoM/PulseMoM/library/Materials/Copper (pure).mtd"
    material = parse_cst_material_file(filename)
    if not material:
        print("✗ FAILED: Could not load copper material")
        return False
    
    print(f"Material: {material['name']}")
    print(f"Conductivity: {material['sigma']:.2e} S/m")
    
    frequencies = [1e6, 10e6, 100e6, 1e9, 10e9, 100e9]  # Hz
    
    print("\nFrequency (GHz)\tSkin Depth (μm)\tClassification")
    print("--------------------------------------------------")
    
    for freq in frequencies:
        classification = classify_material(material, freq)
        
        # Calculate skin depth for good conductor
        omega = 2.0 * 3.14159265359 * freq
        skin_depth = 0.0
        if material['sigma'] > 1e6:
            skin_depth = (2.0 / (omega * 4.0 * 3.14159265359 * 1e-7 * material['sigma']))**0.5
        
        print(f"{freq/1e9:8.1f}\t\t{skin_depth*1e6:8.1f}\t{classification}")
    
    return True

def main():
    """Main test function"""
    print("=== CST Materials Parser Validation ===")
    print("Testing with actual CST material files from library\n")
    
    tests = [
        ("Copper Material", test_copper_material),
        ("PEC Material", test_pec_material),
        ("FR-4 Material", test_fr4_material),
        ("Air Material", test_air_material),
        ("Frequency-Dependent Properties", test_frequency_dependent_properties)
    ]
    
    total_tests = len(tests)
    passed_tests = 0
    
    for test_name, test_func in tests:
        print(f"\n{'='*50}")
        print(f"Running: {test_name}")
        print('='*50)
        
        try:
            if test_func():
                passed_tests += 1
                print(f"\n✓ {test_name}: PASSED")
            else:
                print(f"\n✗ {test_name}: FAILED")
        except Exception as e:
            print(f"\n✗ {test_name}: ERROR - {e}")
    
    # Summary
    print(f"\n{'='*50}")
    print("=== TEST SUMMARY ===")
    print(f"Total tests: {total_tests}")
    print(f"Passed: {passed_tests}")
    print(f"Failed: {total_tests - passed_tests}")
    print(f"Success rate: {100.0 * passed_tests / total_tests:.1f}%")
    
    if passed_tests == total_tests:
        print("\n🎉 ALL TESTS PASSED!")
        return 0
    else:
        print(f"\n❌ {total_tests - passed_tests} test(s) failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())