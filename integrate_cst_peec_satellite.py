#!/usr/bin/env python3
"""
Integration test: CST Materials + PEEC Satellite Simulation
Demonstrates complete workflow from CST material files to electromagnetic simulation
"""

import os
import re
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

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

def calculate_skin_depth(material, frequency):
    """Calculate skin depth for conductive materials"""
    if material['sigma'] < 1e6:
        return float('inf')
    
    omega = 2.0 * np.pi * frequency
    mu0 = 4.0 * np.pi * 1e-7
    
    skin_depth = np.sqrt(2.0 / (omega * mu0 * material['sigma']))
    return skin_depth

def calculate_surface_impedance(material, frequency):
    """Calculate surface impedance for materials"""
    omega = 2.0 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4.0 * np.pi * 1e-7
    
    epsr = material['epsr']
    mur = material['mur']
    sigma = material['sigma']
    
    # Complex permittivity
    eps_complex = epsr * eps0 - 1j * sigma / omega
    mu_complex = mur * mu0
    
    # Wave impedance
    eta = np.sqrt(mu_complex / eps_complex)
    return eta

def peec_satellite_simulation(materials_dict, frequency=10e9):
    """Simulate satellite PEEC model with multiple materials"""
    
    print(f"\n=== PEEC Satellite Simulation at {frequency/1e9:.1f} GHz ===")
    
    # Satellite dimensions (from STL file analysis)
    satellite_length = 2.8  # m
    satellite_width = 2.8   # m
    satellite_height = 0.6  # m
    
    # Coordinate correction for satellite positioning
    coord_correction = np.array([1.7, 1.7, 0.4])  # m
    
    print(f"Satellite dimensions: {satellite_length} × {satellite_width} × {satellite_height} m")
    print(f"Coordinate correction: {coord_correction}")
    
    # Material regions in satellite
    material_regions = [
        {'name': 'Satellite Body', 'material': 'PEC', 'volume': 2.0, 'color': 'gray'},
        {'name': 'Solar Panels', 'material': 'Copper (pure)', 'volume': 0.5, 'color': 'orange'},
        {'name': 'Antenna', 'material': 'Aluminum', 'volume': 0.1, 'color': 'silver'},
        {'name': 'Coating', 'material': 'FR-4 (lossy)', 'volume': 0.2, 'color': 'green'},
        {'name': 'Substrate', 'material': 'Silicon (lossy)', 'volume': 0.3, 'color': 'blue'}
    ]
    
    # Simulation results
    results = []
    
    for region in material_regions:
        material_name = region['material']
        if material_name in materials_dict:
            material = materials_dict[material_name]
            
            # Calculate electromagnetic properties
            classification = classify_material(material, frequency)
            skin_depth = calculate_skin_depth(material, frequency)
            surface_impedance = calculate_surface_impedance(material, frequency)
            
            # Calculate equivalent circuit parameters (simplified PEEC model)
            omega = 2.0 * np.pi * frequency
            mu0 = 4.0 * np.pi * 1e-7
            
            # Resistance (based on skin effect)
            if skin_depth < float('inf'):
                resistance = 1.0 / (material['sigma'] * skin_depth * region['volume'])
            else:
                resistance = 1e6  # High resistance for dielectrics
            
            # Inductance (simplified)
            inductance = mu0 * material['mur'] * region['volume'] / 1.0  # H
            
            # Capacitance (simplified)
            eps0 = 8.854e-12
            capacitance = eps0 * material['epsr'] * region['volume'] / 1.0  # F
            
            results.append({
                'region': region['name'],
                'material': material_name,
                'classification': classification,
                'skin_depth': skin_depth,
                'surface_impedance': surface_impedance,
                'resistance': resistance,
                'inductance': inductance,
                'capacitance': capacitance,
                'color': region['color']
            })
            
            print(f"\nRegion: {region['name']}")
            print(f"  Material: {material_name}")
            print(f"  Classification: {classification}")
            print(f"  Skin depth: {skin_depth*1e6:.2f} μm")
            print(f"  Surface impedance: {abs(surface_impedance):.2f} Ω")
            print(f"  Equivalent R: {resistance:.2e} Ω")
            print(f"  Equivalent L: {inductance:.2e} H")
            print(f"  Equivalent C: {capacitance:.2e} F")
    
    return results

def visualize_satellite_materials(materials_dict, simulation_results):
    """Visualize satellite with material regions"""
    
    fig = plt.figure(figsize=(15, 10))
    
    # 3D visualization of satellite with materials
    ax1 = fig.add_subplot(221, projection='3d')
    
    # Satellite body
    satellite_dims = [2.8, 2.8, 0.6]
    coord_offset = [1.7, 1.7, 0.4]
    
    # Create wireframe for satellite
    x = np.linspace(0, satellite_dims[0], 10) + coord_offset[0] - satellite_dims[0]/2
    y = np.linspace(0, satellite_dims[1], 10) + coord_offset[1] - satellite_dims[1]/2
    z = np.linspace(0, satellite_dims[2], 6) + coord_offset[2] - satellite_dims[2]/2
    
    X, Y = np.meshgrid(x, y)
    Z_top = np.full_like(X, coord_offset[2] + satellite_dims[2]/2)
    Z_bottom = np.full_like(X, coord_offset[2] - satellite_dims[2]/2)
    
    # Plot satellite surfaces
    ax1.plot_surface(X, Y, Z_top, alpha=0.3, color='gray', label='Satellite Body')
    ax1.plot_surface(X, Y, Z_bottom, alpha=0.3, color='gray')
    
    # Add material regions as colored patches
    for result in simulation_results:
        if result['region'] == 'Solar Panels':
            # Solar panels on top
            panel_z = coord_offset[2] + satellite_dims[2]/2 + 0.1
            ax1.plot_surface(X, Y, np.full_like(X, panel_z), alpha=0.7, color='orange')
        elif result['region'] == 'Antenna':
            # Antenna at center
            ant_x = coord_offset[0]
            ant_y = coord_offset[1]
            ant_z = coord_offset[2] + satellite_dims[2]/2 + 0.2
            ax1.scatter([ant_x], [ant_y], [ant_z], c='red', s=100, label='Antenna')
    
    ax1.set_xlabel('X (m)')
    ax1.set_ylabel('Y (m)')
    ax1.set_zlabel('Z (m)')
    ax1.set_title('Satellite with Material Regions')
    
    # Material properties comparison
    ax2 = fig.add_subplot(222)
    
    materials = [result['material'] for result in simulation_results]
    conductivities = [materials_dict[mat]['sigma'] for mat in materials]
    permittivities = [materials_dict[mat]['epsr'] for mat in materials]
    
    x_pos = np.arange(len(materials))
    ax2.bar(x_pos, conductivities, color=['gray', 'orange', 'silver', 'green', 'blue'])
    ax2.set_yscale('log')
    ax2.set_xlabel('Material')
    ax2.set_ylabel('Conductivity (S/m)')
    ax2.set_title('Material Conductivities')
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels([mat.split()[0] for mat in materials], rotation=45)
    
    # Skin depth analysis
    ax3 = fig.add_subplot(223)
    
    frequencies = np.logspace(6, 11, 50)  # 1 MHz to 100 GHz
    for result in simulation_results:
        material_name = result['material']
        material = materials_dict[material_name]
        
        skin_depths = []
        for freq in frequencies:
            skin_depth = calculate_skin_depth(material, freq)
            skin_depths.append(skin_depth * 1e6 if skin_depth < float('inf') else np.nan)
        
        ax3.loglog(frequencies/1e9, skin_depths, 
                  label=f"{material_name.split()[0]}", 
                  color=result['color'])
    
    ax3.set_xlabel('Frequency (GHz)')
    ax3.set_ylabel('Skin Depth (μm)')
    ax3.set_title('Skin Depth vs Frequency')
    ax3.legend()
    ax3.grid(True, alpha=0.3)
    
    # Surface impedance analysis
    ax4 = fig.add_subplot(224)
    
    freq_10ghz = 10e9
    materials_short = []
    impedances = []
    
    for result in simulation_results:
        material_name = result['material']
        material = materials_dict[material_name]
        
        surface_impedance = calculate_surface_impedance(material, freq_10ghz)
        materials_short.append(material_name.split()[0])
        impedances.append(abs(surface_impedance))
    
    x_pos = np.arange(len(materials_short))
    bars = ax4.bar(x_pos, impedances, color=['gray', 'orange', 'silver', 'green', 'blue'])
    ax4.set_yscale('log')
    ax4.set_xlabel('Material')
    ax4.set_ylabel('Surface Impedance (Ω)')
    ax4.set_title(f'Surface Impedance at {freq_10ghz/1e9:.0f} GHz')
    ax4.set_xticks(x_pos)
    ax4.set_xticklabels(materials_short, rotation=45)
    
    # Add values on bars
    for bar, imp in zip(bars, impedances):
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width()/2., height,
                f'{imp:.1e}', ha='center', va='bottom', fontsize=8)
    
    plt.tight_layout()
    plt.savefig('satellite_cst_materials_analysis.png', dpi=300, bbox_inches='tight')
    plt.show()
    
    return fig

def main():
    """Main integration test"""
    print("=== CST Materials + PEEC Satellite Integration Test ===")
    print("Loading CST material files and performing PEEC simulation...\n")
    
    # List of key materials for satellite simulation
    material_files = [
        "D:/Project/MoM/PulseMoM/library/Materials/PEC.mtd",
        "D:/Project/MoM/PulseMoM/library/Materials/Copper (pure).mtd",
        "D:/Project/MoM/PulseMoM/library/Materials/Aluminum.mtd",
        "D:/Project/MoM/PulseMoM/library/Materials/FR-4 (lossy).mtd",
        "D:/Project/MoM/PulseMoM/library/Materials/Silicon (lossy).mtd"
    ]
    
    # Parse all materials
    materials_dict = {}
    print("Parsing CST material files:")
    
    for filename in material_files:
        material = parse_cst_material_file(filename)
        if material:
            materials_dict[material['name']] = material
            print(f"  ✓ {material['name']} - Type: {material['type']}")
        else:
            print(f"  ✗ Failed to parse {filename}")
    
    print(f"\nSuccessfully loaded {len(materials_dict)} materials")
    
    # Perform PEEC simulation
    simulation_results = peec_satellite_simulation(materials_dict, frequency=10e9)
    
    # Generate visualization
    print("\nGenerating visualization...")
    fig = visualize_satellite_materials(materials_dict, simulation_results)
    
    # Summary report
    print("\n=== SIMULATION SUMMARY ===")
    print(f"Simulation frequency: 10 GHz")
    print(f"Number of material regions: {len(simulation_results)}")
    
    total_resistance = sum(result['resistance'] for result in simulation_results)
    total_inductance = sum(result['inductance'] for result in simulation_results)
    total_capacitance = sum(result['capacitance'] for result in simulation_results)
    
    print(f"Total equivalent resistance: {total_resistance:.2e} Ω")
    print(f"Total equivalent inductance: {total_inductance:.2e} H")
    print(f"Total equivalent capacitance: {total_capacitance:.2e} F")
    
    # Calculate resonant frequency of satellite structure
    if total_inductance > 0 and total_capacitance > 0:
        resonant_freq = 1.0 / (2.0 * np.pi * np.sqrt(total_inductance * total_capacitance))
        print(f"Estimated resonant frequency: {resonant_freq/1e9:.3f} GHz")
    
    print(f"\nVisualization saved as: satellite_cst_materials_analysis.png")
    print("Integration test completed successfully!")
    
    return 0

if __name__ == "__main__":
    import sys
    sys.exit(main())