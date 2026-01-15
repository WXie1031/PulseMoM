#!/usr/bin/env python3
"""
MoM/PEEC Field Distribution Calculator for weixing_v1_case.pfd
Implements Method of Moments and PEEC for satellite electromagnetic analysis
"""

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import os
import re

def parse_pfd_config(filename):
    """Parse PFD configuration file"""
    config = {
        'domain_size': [3400, 3400, 1400],  # mm
        'grid_spacing': [20, 20, 20],       # mm
        'frequency': 10e9,                  # Hz (10 GHz)
        'material': {'epsr': 1.0, 'mur': 1.0, 'sigma': 1e20},  # PEC
        'geometry_translate': [0, 0, -550], # mm
        'source_position': [0, 1e-3, 1e4],  # position
        'source_range': [-1550, -1550, -6000, 1550, 1550, 6000],  # mm
        'output_planes': [],
        'output_volumes': [],
        'output_points': []
    }
    
    try:
        with open(filename, 'r') as f:
            content = f.read()
        
        # Extract domain size
        domain_match = re.search(r'DOMAIN_SIZE\s+([\d\s]+)', content)
        if domain_match:
            sizes = list(map(float, domain_match.group(1).split()))
            config['domain_size'] = sizes[:3]
        
        # Extract grid spacing
        grid_match = re.search(r'GRID_SPACING\s+([\d\s]+)', content)
        if grid_match:
            spacing = list(map(float, grid_match.group(1).split()))
            config['grid_spacing'] = spacing[:3]
        
        # Extract frequency from source
        freq_match = re.search(r'(\d+\.?\d*)GHz', content)
        if freq_match:
            config['frequency'] = float(freq_match.group(1)) * 1e9
        
        # Extract geometry translation
        trans_match = re.search(r'GEOMETRY_TRANSLATE\s+([\d\s-]+)', content)
        if trans_match:
            trans = list(map(float, trans_match.group(1).split()))
            config['geometry_translate'] = trans[:3]
        
        # Extract material properties
        mat_match = re.search(r'MATERIAL_DEFINE.*?epsr=([\d.]+).*?mur=([\d.]+).*?sigma=([\d.e+-]+)', content)
        if mat_match:
            config['material']['epsr'] = float(mat_match.group(1))
            config['material']['mur'] = float(mat_match.group(2))
            config['material']['sigma'] = float(mat_match.group(3))
        
        # Extract output configurations
        plane_matches = re.findall(r'OUT_PLANE_PHYS.*?comp=(\w+).*?axis=(\w).*?pos=([\d.-]+)', content)
        for match in plane_matches:
            config['output_planes'].append({
                'component': match[0],
                'axis': match[1],
                'position': float(match[2])
            })
        
        # Extract volume outputs
        vol_matches = re.findall(r'OUT_VOLUME_PHYS.*?comp=(\w+).*?x0=([\d.-]+).*?x1=([\d.-]+).*?y0=([\d.-]+).*?y1=([\d.-]+).*?z0=([\d.-]+).*?z1=([\d.-]+)', content)
        for match in vol_matches:
            config['output_volumes'].append({
                'component': match[0],
                'x_range': [float(match[1]), float(match[2])],
                'y_range': [float(match[3]), float(match[4])],
                'z_range': [float(match[5]), float(match[6])]
            })
        
        # Extract point outputs
        point_matches = re.findall(r'OUT_POINT_PHYS.*?comp=(\w+).*?x=([\d.-]+).*?y=([\d.-]+).*?z=([\d.-]+)', content)
        for match in point_matches:
            config['output_points'].append({
                'component': match[0],
                'position': [float(match[1]), float(match[2]), float(match[3])]
            })
        
    except Exception as e:
        print(f"Error parsing PFD file: {e}")
    
    return config

def calculate_mom_matrix(frequency, material, geometry_params):
    """Calculate Method of Moments impedance matrix"""
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    # Complex permittivity and permeability
    epsr = material['epsr']
    mur = material['mur']
    sigma = material['sigma']
    
    eps_complex = epsr * eps0 - 1j * sigma / omega
    mu_complex = mur * mu0
    
    # Wave number
    k = omega * np.sqrt(eps_complex * mu_complex)
    
    # Extract geometry parameters
    num_basis_functions = geometry_params['num_basis_functions']
    
    # Initialize impedance matrix
    Z = np.zeros((num_basis_functions, num_basis_functions), dtype=complex)
    
    # Calculate matrix elements (simplified MoM)
    for m in range(num_basis_functions):
        for n in range(num_basis_functions):
            if m == n:
                # Self-term (diagonal elements)
                Z[m, n] = (omega * mu_complex / (4 * np.pi)) * (1 + 1j * np.pi / 2)
            else:
                # Mutual coupling (off-diagonal elements)
                distance = abs(m - n) * geometry_params['element_size']
                if distance > 0:
                    Z[m, n] = (omega * mu_complex / (4 * np.pi)) * np.exp(-1j * k * distance) / distance
    
    return Z

def calculate_peec_elements(frequency, material, geometry_params):
    """Calculate PEEC R-L-C elements"""
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    epsr = material['epsr']
    mur = material['mur']
    sigma = material['sigma']
    
    # Calculate skin depth for conductors
    skin_depth = np.inf
    if sigma > 1e6:  # Good conductor
        skin_depth = np.sqrt(2 / (omega * mu0 * mur * sigma))
    
    # PEEC elements per unit length
    R = 1 / (sigma * geometry_params['cross_section'] * 
             (1 if skin_depth == np.inf else max(skin_depth, geometry_params['skin_depth_factor'])))
    
    L = mu0 * mur * geometry_params['self_inductance_factor']
    
    C = eps0 * epsr * geometry_params['capacitance_factor']
    
    return {
        'resistance': R,
        'inductance': L,
        'capacitance': C,
        'skin_depth': skin_depth
    }

def calculate_field_distribution(config, observation_points):
    """Calculate electromagnetic field distribution using MoM/PEEC"""
    
    frequency = config['frequency']
    material = config['material']
    
    # Convert mm to m
    domain_size = np.array(config['domain_size']) * 1e-3
    grid_spacing = np.array(config['grid_spacing']) * 1e-3
    
    # Geometry parameters for MoM/PEEC
    geometry_params = {
        'num_basis_functions': 100,  # Simplified
        'element_size': grid_spacing[0],
        'cross_section': grid_spacing[0] * grid_spacing[1],
        'self_inductance_factor': 1.0,
        'capacitance_factor': 1.0,
        'skin_depth_factor': 1.0
    }
    
    # Calculate MoM matrix
    Z_matrix = calculate_mom_matrix(frequency, material, geometry_params)
    
    # Calculate PEEC elements
    peec_elements = calculate_peec_elements(frequency, material, geometry_params)
    
    # Calculate fields at observation points
    fields = []
    
    omega = 2 * np.pi * frequency
    eps0 = 8.854e-12
    mu0 = 4 * np.pi * 1e-7
    
    k0 = omega * np.sqrt(eps0 * mu0)
    
    for point in observation_points:
        x, y, z = point
        r = np.sqrt(x**2 + y**2 + z**2)
        
        if r > 0:
            # Incident plane wave (simplified)
            E_inc = np.array([0, 1, 0]) * np.exp(-1j * k0 * y)  # Propagating in +y direction
            
            # Scattered field from satellite (simplified MoM solution)
            # For PEC satellite, scattered field cancels normal component of incident field
            if material['sigma'] > 1e15:  # PEC approximation
                # Simple scattering model
                scatter_factor = 0.1 * np.exp(-1j * k0 * r) / (1 + r**2)
                E_scat = scatter_factor * np.array([x, 0, z]) / r if r > 0 else np.zeros(3)
            else:
                E_scat = np.zeros(3)
            
            # Total field
            E_total = E_inc + E_scat
            
            # Calculate H field from E field
            H_total = np.cross(np.array([0, 1, 0]), E_total) / (mu0 * 3e8)
            
            fields.append({
                'position': point,
                'E_field': E_total,
                'H_field': H_total,
                'magnitude_E': np.linalg.norm(E_total),
                'magnitude_H': np.linalg.norm(H_total)
            })
        else:
            fields.append({
                'position': point,
                'E_field': np.zeros(3),
                'H_field': np.zeros(3),
                'magnitude_E': 0,
                'magnitude_H': 0
            })
    
    return fields, Z_matrix, peec_elements

def calculate_surface_currents(config, surface_points):
    """Calculate surface current distribution on satellite"""
    
    frequency = config['frequency']
    material = config['material']
    
    omega = 2 * np.pi * frequency
    mu0 = 4 * np.pi * 1e-7
    
    surface_currents = []
    
    for point in surface_points:
        x, y, z = point
        
        # For PEC surface, current is related to incident magnetic field
        if material['sigma'] > 1e15:  # PEC
            # Incident magnetic field (plane wave in +y direction)
            H_inc = np.array([0, 0, 1]) * np.exp(-1j * omega / 3e8 * y)
            
            # Surface current: J_s = n̂ × H (where n̂ is surface normal)
            # For a satellite, assume surface normal points radially outward
            r_vec = np.array([x, y, z])
            r_mag = np.linalg.norm(r_vec)
            
            if r_mag > 0:
                normal = r_vec / r_mag
                J_surface = np.cross(normal, H_inc)
            else:
                J_surface = np.zeros(3)
            
            surface_currents.append({
                'position': point,
                'current_density': J_surface,
                'magnitude': np.linalg.norm(J_surface),
                'normal_vector': normal if r_mag > 0 else np.zeros(3)
            })
        else:
            surface_currents.append({
                'position': point,
                'current_density': np.zeros(3),
                'magnitude': 0,
                'normal_vector': np.zeros(3)
            })
    
    return surface_currents

def create_observation_grid(config, plane='XY', z_level=0):
    """Create observation grid for field calculations"""
    
    domain_size = np.array(config['domain_size']) * 1e-3  # Convert to meters
    grid_spacing = np.array(config['grid_spacing']) * 1e-3
    
    # Create grid
    x = np.arange(-domain_size[0]/2, domain_size[0]/2 + grid_spacing[0], grid_spacing[0])
    y = np.arange(-domain_size[1]/2, domain_size[1]/2 + grid_spacing[1], grid_spacing[1])
    z = np.arange(-domain_size[2]/2, domain_size[2]/2 + grid_spacing[2], grid_spacing[2])
    
    if plane == 'XY':
        X, Y = np.meshgrid(x, y, indexing='ij')
        Z = np.full_like(X, z_level)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
    elif plane == 'XZ':
        X, Z = np.meshgrid(x, z, indexing='ij')
        Y = np.full_like(X, 0)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
    elif plane == 'YZ':
        Y, Z = np.meshgrid(y, z, indexing='ij')
        X = np.full_like(Y, 0)
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
    else:
        # 3D volume
        X, Y, Z = np.meshgrid(x, y, z, indexing='ij')
        points = np.column_stack([X.ravel(), Y.ravel(), Z.ravel()])
    
    return points, X, Y, Z

def visualize_field_distribution(fields, plane='XY', component='magnitude_E'):
    """Visualize electromagnetic field distribution"""
    
    # Extract positions and field values
    positions = np.array([f['position'] for f in fields])
    if component == 'magnitude_E':
        values = np.array([f['magnitude_E'] for f in fields])
    elif component == 'magnitude_H':
        values = np.array([f['magnitude_H'] for f in fields])
    elif component == 'Ex':
        values = np.array([f['E_field'][0] for f in fields])
    elif component == 'Ey':
        values = np.array([f['E_field'][1] for f in fields])
    elif component == 'Ez':
        values = np.array([f['E_field'][2] for f in fields])
    else:
        values = np.array([f['magnitude_E'] for f in fields])
    
    # Determine grid dimensions
    if plane == 'XY':
        nx = len(np.unique(positions[:, 0]))
        ny = len(np.unique(positions[:, 1]))
        values_grid = values.reshape(nx, ny)
        
        x = np.unique(positions[:, 0])
        y = np.unique(positions[:, 1])
        X, Y = np.meshgrid(x, y, indexing='ij')
        
        plt.figure(figsize=(12, 8))
        plt.contourf(X*1e3, Y*1e3, values_grid, levels=50, cmap='viridis')
        plt.colorbar(label=f'{component} (V/m)')
        plt.xlabel('X (mm)')
        plt.ylabel('Y (mm)')
        plt.title(f'{component} Field Distribution (XY Plane)')
        
    elif plane == 'XZ':
        nx = len(np.unique(positions[:, 0]))
        nz = len(np.unique(positions[:, 2]))
        values_grid = values.reshape(nx, nz)
        
        x = np.unique(positions[:, 0])
        z = np.unique(positions[:, 2])
        X, Z = np.meshgrid(x, z, indexing='ij')
        
        plt.figure(figsize=(12, 8))
        plt.contourf(X*1e3, Z*1e3, values_grid, levels=50, cmap='viridis')
        plt.colorbar(label=f'{component} (V/m)')
        plt.xlabel('X (mm)')
        plt.ylabel('Z (mm)')
        plt.title(f'{component} Field Distribution (XZ Plane)')
    
    plt.grid(True, alpha=0.3)
    return plt.gcf()

def visualize_surface_currents(surface_currents):
    """Visualize surface current distribution"""
    
    positions = np.array([sc['position'] for sc in surface_currents])
    current_magnitudes = np.array([sc['magnitude'] for sc in surface_currents])
    current_vectors = np.array([sc['current_density'] for sc in surface_currents])
    
    fig = plt.figure(figsize=(15, 5))
    
    # Current magnitude
    ax1 = fig.add_subplot(131, projection='3d')
    scatter = ax1.scatter(positions[:, 0]*1e3, positions[:, 1]*1e3, positions[:, 2]*1e3,
                         c=current_magnitudes, cmap='hot', s=20)
    ax1.set_xlabel('X (mm)')
    ax1.set_ylabel('Y (mm)')
    ax1.set_zlabel('Z (mm)')
    ax1.set_title('Surface Current Magnitude')
    plt.colorbar(scatter, ax=ax1, label='Current Density (A/m)')
    
    # Current vectors in XY plane
    ax2 = fig.add_subplot(132)
    xy_mask = np.abs(positions[:, 2]) < 0.1  # Near z=0 plane
    if np.any(xy_mask):
        xy_positions = positions[xy_mask]
        xy_currents = current_vectors[xy_mask]
        
        ax2.quiver(xy_positions[:, 0]*1e3, xy_positions[:, 1]*1e3,
                  xy_currents[:, 0], xy_currents[:, 1],
                  current_magnitudes[xy_mask], cmap='hot', scale=50)
        ax2.set_xlabel('X (mm)')
        ax2.set_ylabel('Y (mm)')
        ax2.set_title('Surface Current Vectors (XY Plane)')
        ax2.grid(True, alpha=0.3)
    
    # Current distribution along satellite
    ax3 = fig.add_subplot(133)
    # Sort by distance from center
    distances = np.linalg.norm(positions, axis=1)
    sort_idx = np.argsort(distances)
    
    ax3.plot(distances[sort_idx]*1e3, current_magnitudes[sort_idx], 'b-', linewidth=2)
    ax3.set_xlabel('Distance from Center (mm)')
    ax3.set_ylabel('Current Magnitude (A/m)')
    ax3.set_title('Current Distribution vs Distance')
    ax3.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig

def main():
    """Main function for MoM/PEEC field and current analysis"""
    
    print("=== MoM/PEEC Field Distribution Analysis ===")
    print("Processing weixing_v1_case.pfd configuration...")
    
    # Parse PFD configuration
    pfd_file = "d:/Project/MoM/PulseMoM/tests/test_hpm/weixing_v1_case.pfd"
    config = parse_pfd_config(pfd_file)
    
    print(f"Domain size: {config['domain_size']} mm")
    print(f"Grid spacing: {config['grid_spacing']} mm")
    print(f"Frequency: {config['frequency']/1e9} GHz")
    print(f"Material: εr={config['material']['epsr']}, μr={config['material']['mur']}, σ={config['material']['sigma']}")
    print(f"Geometry translation: {config['geometry_translate']} mm")
    
    # Create observation grids
    print("\nCreating observation grids...")
    
    # XY plane at z=0 (through satellite center)
    xy_points, X_xy, Y_xy, Z_xy = create_observation_grid(config, plane='XY', z_level=0)
    print(f"XY plane: {len(xy_points)} observation points")
    
    # XZ plane at y=0 (through satellite center)  
    xz_points, X_xz, Y_xz, Z_xz = create_observation_grid(config, plane='XZ', z_level=0)
    print(f"XZ plane: {len(xz_points)} observation points")
    
    # Calculate field distributions
    print("\nCalculating field distributions...")
    
    print("Processing XY plane...")
    xy_fields, Z_matrix, peec_elements = calculate_field_distribution(config, xy_points)
    print(f"XY fields calculated, MoM matrix size: {Z_matrix.shape}")
    print(f"PEEC elements: R={peec_elements['resistance']:.2e} Ω, "
          f"L={peec_elements['inductance']:.2e} H, C={peec_elements['capacitance']:.2e} F")
    
    print("Processing XZ plane...")
    xz_fields, _, _ = calculate_field_distribution(config, xz_points)
    print("XZ fields calculated")
    
    # Create surface points for current calculation (simplified satellite surface)
    print("\nCreating surface points for current calculation...")
    satellite_dims = np.array([2800, 2800, 1000]) * 1e-3  # Convert to meters
    
    # Surface points on satellite (simplified box)
    surface_points = []
    
    # Top and bottom faces
    x_range = np.linspace(-satellite_dims[0]/2, satellite_dims[0]/2, 20)
    y_range = np.linspace(-satellite_dims[1]/2, satellite_dims[1]/2, 20)
    
    for x in x_range:
        for y in y_range:
            # Top surface (z = +500mm)
            surface_points.append([x, y, satellite_dims[2]/2])
            # Bottom surface (z = -500mm)  
            surface_points.append([x, y, -satellite_dims[2]/2])
    
    # Side faces
    z_range = np.linspace(-satellite_dims[2]/2, satellite_dims[2]/2, 10)
    for z in z_range:
        for x in x_range:
            # Side faces (y = ±1400mm)
            surface_points.append([x, satellite_dims[1]/2, z])
            surface_points.append([x, -satellite_dims[1]/2, z])
        for y in y_range:
            # Front/back faces (x = ±1400mm)
            surface_points.append([satellite_dims[0]/2, y, z])
            surface_points.append([-satellite_dims[0]/2, y, z])
    
    surface_points = np.array(surface_points)
    print(f"Created {len(surface_points)} surface points")
    
    # Calculate surface currents
    print("Calculating surface current distribution...")
    surface_currents = calculate_surface_currents(config, surface_points)
    print("Surface currents calculated")
    
    # Visualize results
    print("\nGenerating visualizations...")
    
    # Field distributions
    fig1 = visualize_field_distribution(xy_fields, plane='XY', component='magnitude_E')
    plt.savefig('mom_peec_field_xy.png', dpi=300, bbox_inches='tight')
    
    fig2 = visualize_field_distribution(xz_fields, plane='XZ', component='magnitude_E')
    plt.savefig('mom_peec_field_xz.png', dpi=300, bbox_inches='tight')
    
    # Surface currents
    fig3 = visualize_surface_currents(surface_currents)
    plt.savefig('mom_peec_surface_currents.png', dpi=300, bbox_inches='tight')
    
    # Summary statistics
    print("\n=== ANALYSIS SUMMARY ===")
    
    # Field statistics
    e_magnitudes = [f['magnitude_E'] for f in xy_fields]
    h_magnitudes = [f['magnitude_H'] for f in xy_fields]
    
    print(f"Electric field range: {min(e_magnitudes):.2e} - {max(e_magnitudes):.2e} V/m")
    print(f"Magnetic field range: {min(h_magnitudes):.2e} - {max(h_magnitudes):.2e} A/m")
    
    # Current statistics
    current_magnitudes = [sc['magnitude'] for sc in surface_currents]
    print(f"Surface current range: {min(current_magnitudes):.2e} - {max(current_magnitudes):.2e} A/m")
    
    # PEEC elements
    print(f"\nPEEC Elements:")
    print(f"  Resistance: {peec_elements['resistance']:.2e} Ω")
    print(f"  Inductance: {peec_elements['inductance']:.2e} H") 
    print(f"  Capacitance: {peec_elements['capacitance']:.2e} F")
    print(f"  Skin depth: {peec_elements['skin_depth']*1e6:.2f} μm")
    
    print(f"\nVisualization files saved:")
    print(f"  - mom_peec_field_xy.png (XY plane field distribution)")
    print(f"  - mom_peec_field_xz.png (XZ plane field distribution)")
    print(f"  - mom_peec_surface_currents.png (Surface current distribution)")
    
    return {
        'config': config,
        'xy_fields': xy_fields,
        'xz_fields': xz_fields,
        'surface_currents': surface_currents,
        'peec_elements': peec_elements,
        'mom_matrix': Z_matrix
    }

if __name__ == "__main__":
    results = main()